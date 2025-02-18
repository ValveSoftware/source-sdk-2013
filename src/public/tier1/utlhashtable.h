//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: a fast growable hashtable with stored hashes, L2-friendly behavior.
// Useful as a string dictionary or a low-overhead set/map for small POD types.
//
// Usage notes:
// - handles are NOT STABLE across element removal! use RemoveAndAdvance()
//   if you are removing elements while iterating through the hashtable.
//   Use CUtlStableHashtable if you need stable handles (less efficient).
// - Insert() first searches for an existing match and returns it if found
// - a value type of "empty_t" can be used to eliminate value storage and
//   switch Element() to return const Key references instead of values
// - an extra user flag bit is accessible via Get/SetUserFlag()
// - hash function pointer / functor is exposed via GetHashRef()
// - comparison function pointer / functor is exposed via GetEqualRef()
// - if your value type cannot be copy-constructed, use key-only Insert()
//   to default-initialize the value and then manipulate it afterwards.
//
// Implementation notes:
// - overall hash table load is kept between .25 and .75
// - items which would map to the same ideal slot are chained together
// - chained items are stored sequentially in adjacent free spaces
// - "root" entries are prioritized over chained entries; if a
//   slot is not occupied by an item in its root position, the table
//   is guaranteed to contain no keys which would hash to that slot.
// - new items go at the head of the chain (ie, in their root slot)
//   and evict / "bump" any chained entries which occupy that slot
// - chain-following skips over unused holes and continues examining
//   table entries until a chain entry with FLAG_LAST is encountered
//
// CUtlHashtable< uint32 >       setOfIntegers;
// CUtlHashtable< const char* >  setOfStringPointers;
// CUtlHashtable< int, CUtlVector<blah_t> >  mapFromIntsToArrays;
//
// $NoKeywords: $
//
// A closed-form (open addressing) hashtable with linear sequential probing.
//=============================================================================//

#ifndef UTLHASHTABLE_H
#define UTLHASHTABLE_H
#pragma once

#include "utlcommon.h"
#include "utlmemory.h"
#include "mathlib/mathlib.h"
#include "utllinkedlist.h"

//-----------------------------------------------------------------------------
// Henry Goffin (henryg) was here. Questions? Bugs? Go slap him around a bit.
//-----------------------------------------------------------------------------

typedef unsigned int UtlHashHandle_t;

#define FOR_EACH_HASHTABLE( table, iter ) \
	for ( UtlHashHandle_t iter = (table).FirstHandle(); iter != (table).InvalidHandle(); iter = (table).NextHandle( iter ) )

// CUtlHashtableEntry selects between 16 and 32 bit storage backing
// for flags_and_hash depending on the size of the stored types.
template < typename KeyT, typename ValueT = empty_t >
class CUtlHashtableEntry
{
public:
	typedef CUtlKeyValuePair< KeyT, ValueT > KVPair;

	enum { INT16_STORAGE = ( sizeof( KVPair ) <= 2 ) };
	typedef typename CTypeSelect< INT16_STORAGE, int16, int32 >::type storage_t;

	enum
	{
		FLAG_FREE = INT16_STORAGE ? 0x8000 : 0x80000000, // must be high bit for IsValid and IdealIndex to work
		FLAG_LAST = INT16_STORAGE ? 0x4000 : 0x40000000,
		MASK_HASH = INT16_STORAGE ? 0x3FFF : 0x3FFFFFFF
	};

	storage_t flags_and_hash;
	storage_t data[ ( sizeof(KVPair) + sizeof(storage_t) - 1 ) / sizeof(storage_t) ];

	bool IsValid() const { return flags_and_hash >= 0; }
	void MarkInvalid() { int32 flag = FLAG_FREE; flags_and_hash = (storage_t)flag; }
	const KVPair *Raw() const { return reinterpret_cast< const KVPair * >( &data[0] ); }
	const KVPair *operator->() const { Assert( IsValid() ); return reinterpret_cast< const KVPair * >( &data[0] ); }
	KVPair *Raw() { return reinterpret_cast< KVPair * >( &data[0] ); }
	KVPair *operator->() { Assert( IsValid() ); return reinterpret_cast< KVPair * >( &data[0] ); }

	// Returns the ideal index of the data in this slot, or all bits set if invalid
	uint32 FORCEINLINE IdealIndex( uint32 slotmask ) const { return IdealIndex( flags_and_hash, slotmask ) | ( (int32)flags_and_hash >> 31 ); }

	// Use template tricks to fully define only one function that takes either 16 or 32 bits
	// and performs different logic without using "if ( INT16_STORAGE )", because GCC and MSVC
	// sometimes have trouble removing the constant branch, which is dumb... but whatever.
	// 16-bit hashes are simply too narrow for large hashtables; more mask bits than hash bits!
	// So we duplicate the hash bits. (Note: h *= MASK_HASH+2 is the same as h += h<<HASH_BITS)
	typedef typename CTypeSelect< INT16_STORAGE, int16, undefined_t >::type uint32_if16BitStorage;
	typedef typename CTypeSelect< INT16_STORAGE, undefined_t, int32 >::type uint32_if32BitStorage;
	static FORCEINLINE uint32 IdealIndex( uint32_if16BitStorage h, uint32 m ) { h &= MASK_HASH; h *= MASK_HASH + 2; return h & m; }
	static FORCEINLINE uint32 IdealIndex( uint32_if32BitStorage h, uint32 m ) { return h & m; }

	// More efficient than memcpy for the small types that are stored in a hashtable
	void MoveDataFrom( CUtlHashtableEntry &src )
	{
		storage_t * RESTRICT srcData = &src.data[0];
		for ( int i = 0; i < ARRAYSIZE( data ); ++i ) { data[i] = srcData[i]; }
	}
};

template <typename KeyT, typename ValueT = empty_t, typename KeyHashT = DefaultHashFunctor<KeyT>, typename KeyIsEqualT = DefaultEqualFunctor<KeyT>, typename AlternateKeyT = typename ArgumentTypeInfo<KeyT>::Alt_t >
class CUtlHashtable
{
public:
	typedef UtlHashHandle_t handle_t;

protected:
	typedef CUtlKeyValuePair<KeyT, ValueT> KVPair;
	typedef typename ArgumentTypeInfo<KeyT>::Arg_t KeyArg_t;
	typedef typename ArgumentTypeInfo<ValueT>::Arg_t ValueArg_t;
	typedef typename ArgumentTypeInfo<AlternateKeyT>::Arg_t KeyAlt_t;
	typedef CUtlHashtableEntry< KeyT, ValueT > entry_t;

	enum { FLAG_FREE = entry_t::FLAG_FREE };
	enum { FLAG_LAST = entry_t::FLAG_LAST };
	enum { MASK_HASH = entry_t::MASK_HASH };

	CUtlMemory< entry_t > m_table;
	int m_nUsed;
	int m_nMinSize;
	bool m_bSizeLocked;
	KeyIsEqualT m_eq;
	KeyHashT m_hash;

	// Allocate an empty table and then re-insert all existing entries.
	void DoRealloc( int size );

	// Move an existing entry to a free slot, leaving a hole behind
	void BumpEntry( unsigned int idx );

	// Insert an unconstructed KVPair at the primary slot
	int DoInsertUnconstructed( unsigned int h, bool allowGrow );

	// Implementation for Insert functions, constructs a KVPair
	// with either a default-construted or copy-constructed value
	template <typename KeyParamT> handle_t DoInsert( KeyParamT k, unsigned int h );
	template <typename KeyParamT> handle_t DoInsert( KeyParamT k, typename ArgumentTypeInfo<ValueT>::Arg_t v, unsigned int h, bool* pDidInsert );
	template <typename KeyParamT> handle_t DoInsertNoCheck( KeyParamT k, typename ArgumentTypeInfo<ValueT>::Arg_t v, unsigned int h );

	// Key lookup. Can also return previous-in-chain if result is chained.
	template <typename KeyParamT> handle_t DoLookup( KeyParamT x, unsigned int h, handle_t *pPreviousInChain ) const;

	// Remove single element by key + hash. Returns the index of the new hole
	// that was created. Returns InvalidHandle() if element was not found. 
	template <typename KeyParamT> int DoRemove( KeyParamT x, unsigned int h );

	// Friend CUtlStableHashtable so that it can call our Do* functions directly
	template < typename K, typename V, typename S, typename H, typename E, typename A > friend class CUtlStableHashtable;

public:
	explicit CUtlHashtable( int minimumSize = 32 )
		: m_nUsed(0), m_nMinSize(MAX(8, minimumSize)), m_bSizeLocked(false), m_eq(), m_hash() { }

	CUtlHashtable( int minimumSize, const KeyHashT &hash, KeyIsEqualT const &eq = KeyIsEqualT() )
		: m_nUsed(0), m_nMinSize(MAX(8, minimumSize)), m_bSizeLocked(false), m_eq(eq), m_hash(hash) { }

	CUtlHashtable( entry_t* pMemory, unsigned int nCount, const KeyHashT &hash = KeyHashT(), KeyIsEqualT const &eq = KeyIsEqualT() )
		: m_nUsed(0), m_nMinSize(8), m_bSizeLocked(false), m_eq(eq), m_hash(hash) { SetExternalBuffer( pMemory, nCount ); }

	~CUtlHashtable() { RemoveAll(); }

	CUtlHashtable &operator=( CUtlHashtable const &src );

	// Set external memory
	void SetExternalBuffer( byte* pRawBuffer, unsigned int nBytes, bool bAssumeOwnership = false, bool bGrowable = false );
	void SetExternalBuffer( entry_t* pBuffer, unsigned int nSize, bool bAssumeOwnership = false, bool bGrowable = false );

	// Functor/function-pointer access
	KeyHashT& GetHashRef() { return m_hash; }
	KeyIsEqualT& GetEqualRef() { return m_eq; }
	KeyHashT const &GetHashRef() const { return m_hash; }
	KeyIsEqualT const &GetEqualRef() const { return m_eq; }

	// Handle validation
	bool IsValidHandle( handle_t idx ) const { return (unsigned)idx < (unsigned)m_table.Count() && m_table[idx].IsValid(); }
	static handle_t InvalidHandle() { return (handle_t) -1; }

	// Iteration functions
	handle_t FirstHandle() const { return NextHandle( (handle_t) -1 ); }
	handle_t NextHandle( handle_t start ) const;

	// Returns the number of unique keys in the table
	int Count() const { return m_nUsed; }


	// Key lookup, returns InvalidHandle() if not found
	handle_t Find( KeyArg_t k ) const { return DoLookup<KeyArg_t>( k, m_hash(k), NULL ); }
	handle_t Find( KeyArg_t k, unsigned int hash) const { Assert( hash == m_hash(k) ); return DoLookup<KeyArg_t>( k, hash, NULL ); }
	// Alternate-type key lookup, returns InvalidHandle() if not found
	handle_t Find( KeyAlt_t k ) const { return DoLookup<KeyAlt_t>( k, m_hash(k), NULL ); }
	handle_t Find( KeyAlt_t k, unsigned int hash) const { Assert( hash == m_hash(k) ); return DoLookup<KeyAlt_t>( k, hash, NULL ); }

	// True if the key is in the table
	bool HasElement( KeyArg_t k ) const { return InvalidHandle() != Find( k ); }
	bool HasElement( KeyAlt_t k ) const { return InvalidHandle() != Find( k ); }
	
	// Key insertion or lookup, always returns a valid handle
	handle_t Insert( KeyArg_t k ) { return DoInsert<KeyArg_t>( k, m_hash(k) ); }
	handle_t Insert( KeyArg_t k, ValueArg_t v, bool *pDidInsert = NULL ) { return DoInsert<KeyArg_t>( k, v, m_hash(k), pDidInsert ); }
	handle_t Insert( KeyArg_t k, ValueArg_t v, unsigned int hash, bool *pDidInsert = NULL ) { Assert( hash == m_hash(k) ); return DoInsert<KeyArg_t>( k, v, hash, pDidInsert ); }
	// Alternate-type key insertion or lookup, always returns a valid handle
	handle_t Insert( KeyAlt_t k ) { return DoInsert<KeyAlt_t>( k, m_hash(k) ); }
	handle_t Insert( KeyAlt_t k, ValueArg_t v, bool *pDidInsert = NULL ) { return DoInsert<KeyAlt_t>( k, v, m_hash(k), pDidInsert ); }
	handle_t Insert( KeyAlt_t k, ValueArg_t v, unsigned int hash, bool *pDidInsert = NULL ) { Assert( hash == m_hash(k) ); return DoInsert<KeyAlt_t>( k, v, hash, pDidInsert ); }

	// Key removal, returns false if not found
	bool Remove( KeyArg_t k ) { return DoRemove<KeyArg_t>( k, m_hash(k) ) >= 0; }
	bool Remove( KeyArg_t k, unsigned int hash ) { Assert( hash == m_hash(k) ); return DoRemove<KeyArg_t>( k, hash ) >= 0; }
	// Alternate-type key removal, returns false if not found
	bool Remove( KeyAlt_t k ) { return DoRemove<KeyAlt_t>( k, m_hash(k) ) >= 0; }
	bool Remove( KeyAlt_t k, unsigned int hash ) { Assert( hash == m_hash(k) ); return DoRemove<KeyAlt_t>( k, hash ) >= 0; }

	// Remove while iterating, returns the next handle for forward iteration
	// Note: aside from this, ALL handles are invalid if an element is removed
	handle_t RemoveAndAdvance( handle_t idx );

	// Nuke contents
	void RemoveAll();

	// Nuke and release memory.
	void Purge() { RemoveAll(); m_table.Purge(); }

	// Reserve table capacity up front to avoid reallocation during insertions
	void Reserve( int expected ) { if ( expected > m_nUsed ) DoRealloc( expected * 4 / 3 ); }

	// Shrink to best-fit size, re-insert keys for optimal lookup
	void Compact( bool bMinimal ) { DoRealloc( bMinimal ? m_nUsed : ( m_nUsed * 4 / 3 ) ); }

	// Access functions. Note: if ValueT is empty_t, all functions return const keys.
	typedef typename KVPair::ValueReturn_t Element_t;
	KeyT const &Key( handle_t idx ) const { return m_table[idx]->m_key; }
	Element_t const &Element( handle_t idx ) const { return m_table[idx]->GetValue(); }
	Element_t &Element(handle_t idx) { return m_table[idx]->GetValue(); }
	Element_t const &operator[]( handle_t idx ) const { return m_table[idx]->GetValue(); }
	Element_t &operator[]( handle_t idx ) { return m_table[idx]->GetValue(); }

	void ReplaceKey( handle_t idx, KeyArg_t k ) { Assert( m_eq( m_table[idx]->m_key, k ) && m_hash( k ) == m_hash( m_table[idx]->m_key ) ); m_table[idx]->m_key = k; }
	void ReplaceKey( handle_t idx, KeyAlt_t k ) { Assert( m_eq( m_table[idx]->m_key, k ) && m_hash( k ) == m_hash( m_table[idx]->m_key ) ); m_table[idx]->m_key = k; }

	Element_t const &Get( KeyArg_t k, Element_t const &defaultValue ) const { handle_t h = Find( k ); if ( h != InvalidHandle() ) return Element( h ); return defaultValue; }
	Element_t const &Get( KeyAlt_t k, Element_t const &defaultValue ) const { handle_t h = Find( k ); if ( h != InvalidHandle() ) return Element( h ); return defaultValue; }

	Element_t const *GetPtr( KeyArg_t k ) const { handle_t h = Find(k); if ( h != InvalidHandle() ) return &Element( h ); return NULL; }
	Element_t const *GetPtr( KeyAlt_t k ) const { handle_t h = Find(k); if ( h != InvalidHandle() ) return &Element( h ); return NULL; }
	Element_t *GetPtr( KeyArg_t k ) { handle_t h = Find( k ); if ( h != InvalidHandle() ) return &Element( h ); return NULL; }
	Element_t *GetPtr( KeyAlt_t k ) { handle_t h = Find( k ); if ( h != InvalidHandle() ) return &Element( h ); return NULL; }

	// Swap memory and contents with another identical hashtable
	// (NOTE: if using function pointers or functors with state,
	//  it is up to the caller to ensure that they are compatible!)
	void Swap( CUtlHashtable &other ) { m_table.Swap(other.m_table); ::V_swap(m_nUsed, other.m_nUsed); }

#if _DEBUG
	// Validate the integrity of the hashtable
	void DbgCheckIntegrity() const;
#endif

private:
	CUtlHashtable(const CUtlHashtable& copyConstructorIsNotImplemented);
};


// Set external memory (raw byte buffer, best-fit)
template <typename KeyT, typename ValueT, typename KeyHashT, typename KeyIsEqualT, typename AltKeyT>
void CUtlHashtable<KeyT, ValueT, KeyHashT, KeyIsEqualT, AltKeyT>::SetExternalBuffer( byte* pRawBuffer, unsigned int nBytes, bool bAssumeOwnership, bool bGrowable )
{
	Assert( ((uintptr_t)pRawBuffer % __alignof(int)) == 0 );
	uint32 bestSize = LargestPowerOfTwoLessThanOrEqual( nBytes / sizeof(entry_t) );
	Assert( bestSize != 0 && bestSize*sizeof(entry_t) <= nBytes );

	return SetExternalBuffer( (entry_t*) pRawBuffer, bestSize, bAssumeOwnership, bGrowable );
}

// Set external memory (typechecked, must be power of two)
template <typename KeyT, typename ValueT, typename KeyHashT, typename KeyIsEqualT, typename AltKeyT>
void CUtlHashtable<KeyT, ValueT, KeyHashT, KeyIsEqualT, AltKeyT>::SetExternalBuffer( entry_t* pBuffer, unsigned int nSize, bool bAssumeOwnership, bool bGrowable )
{
	Assert( IsPowerOfTwo(nSize) );
	Assert( m_nUsed == 0 );
	for ( uint i = 0; i < nSize; ++i )
		pBuffer[i].MarkInvalid();
	if ( bAssumeOwnership )
		m_table.AssumeMemory( pBuffer, nSize );
	else
		m_table.SetExternalBuffer( pBuffer, nSize );
	m_bSizeLocked = !bGrowable;
}

// Allocate an empty table and then re-insert all existing entries.
template <typename KeyT, typename ValueT, typename KeyHashT, typename KeyIsEqualT, typename AltKeyT>
void CUtlHashtable<KeyT, ValueT, KeyHashT, KeyIsEqualT, AltKeyT>::DoRealloc( int size )
{
	Assert( !m_bSizeLocked ); 

	size = SmallestPowerOfTwoGreaterOrEqual( MAX( m_nMinSize, size ) );
	Assert( size > 0 && (uint)size <= entry_t::IdealIndex( ~0, 0x1FFFFFFF ) ); // reasonable power of 2
	Assert( size > m_nUsed );

	CUtlMemory<entry_t> oldTable;
	oldTable.Swap( m_table );
	entry_t * RESTRICT const pOldBase = oldTable.Base();

	m_table.EnsureCapacity( size );
	entry_t * const pNewBase = m_table.Base();
	for ( int i = 0; i < size; ++i )
		pNewBase[i].MarkInvalid();

	int nLeftToMove = m_nUsed;
	m_nUsed = 0;
	for ( int i = oldTable.Count() - 1; i >= 0; --i )
	{
		if ( pOldBase[i].IsValid() )
		{
			int newIdx = DoInsertUnconstructed( pOldBase[i].flags_and_hash, false );
			pNewBase[newIdx].MoveDataFrom( pOldBase[i] );
			if ( --nLeftToMove == 0 )
				break;
		}
	}
	Assert( nLeftToMove == 0 );
}


// Move an existing entry to a free slot, leaving a hole behind
template <typename KeyT, typename ValueT, typename KeyHashT, typename KeyIsEqualT, typename AltKeyT>
void CUtlHashtable<KeyT, ValueT, KeyHashT, KeyIsEqualT, AltKeyT>::BumpEntry( unsigned int idx )
{
	Assert( m_table[idx].IsValid() );
	Assert( m_nUsed < m_table.Count() );

	entry_t* table = m_table.Base();
	unsigned int slotmask = m_table.Count()-1;
	unsigned int new_flags_and_hash = table[idx].flags_and_hash & (FLAG_LAST | MASK_HASH);

	unsigned int chainid = entry_t::IdealIndex( new_flags_and_hash, slotmask );

	// Look for empty slots scanning forward, stripping FLAG_LAST as we go.
	// Note: this potentially strips FLAG_LAST from table[idx] if we pass it
	int newIdx = chainid; // start at ideal slot
	for ( ; ; newIdx = (newIdx + 1) & slotmask )
	{
		if ( table[newIdx].IdealIndex( slotmask ) == chainid )
		{
			if ( table[newIdx].flags_and_hash & FLAG_LAST )
			{
				table[newIdx].flags_and_hash &= ~FLAG_LAST;
				new_flags_and_hash |= FLAG_LAST;
			}
			continue;
		}
		if ( table[newIdx].IsValid() )
		{
			continue;
		}
		break;
	}

	// Did we pick something closer to the ideal slot, leaving behind a
	// FLAG_LAST bit on the current slot because we didn't scan past it?
	if ( table[idx].flags_and_hash & FLAG_LAST )
	{
#ifdef _DEBUG
		Assert( new_flags_and_hash & FLAG_LAST );
		// Verify logic: we must have moved to an earlier slot, right?
		uint offset = ((uint)idx - chainid + slotmask + 1) & slotmask;
		uint newOffset = ((uint)newIdx - chainid + slotmask + 1) & slotmask;
		Assert( newOffset < offset );
#endif
		// Scan backwards from old to new location, depositing FLAG_LAST on
		// the first match we find. (+slotmask) is the same as (-1) without
		// having to make anyone think about two's complement shenanigans.
		int scan = (idx + slotmask) & slotmask;
		while ( scan != newIdx )
		{
			if ( table[scan].IdealIndex( slotmask ) == chainid )
			{
				table[scan].flags_and_hash |= FLAG_LAST;
				new_flags_and_hash &= ~FLAG_LAST;
				break;
			}
			scan = (scan + slotmask) & slotmask;
		}
	}

	// Move entry to the free slot we found, leaving a hole at idx
	table[newIdx].flags_and_hash = new_flags_and_hash;
	table[newIdx].MoveDataFrom( table[idx] );
	table[idx].MarkInvalid();
}


// Insert a value at the root position for that value's hash chain.
template <typename KeyT, typename ValueT, typename KeyHashT, typename KeyIsEqualT, typename AltKeyT>
int CUtlHashtable<KeyT, ValueT, KeyHashT, KeyIsEqualT, AltKeyT>::DoInsertUnconstructed( unsigned int h, bool allowGrow )
{
	if ( allowGrow && !m_bSizeLocked )
	{
		// Keep the load factor between .25 and .75
		int newSize = m_nUsed + 1;
		if ( ( newSize*4 < m_table.Count() && m_table.Count() > m_nMinSize*2 ) || newSize*4 > m_table.Count()*3 )
		{
			DoRealloc( newSize * 4 / 3 );
		}
	}
	Assert( m_nUsed < m_table.Count() );
	++m_nUsed;

	entry_t* table = m_table.Base();
	unsigned int slotmask = m_table.Count()-1;
	unsigned int new_flags_and_hash = FLAG_LAST | (h & MASK_HASH);
	unsigned int idx = entry_t::IdealIndex( h, slotmask );
	if ( table[idx].IdealIndex( slotmask ) == idx )
	{
		// There is already an entry in this chain.
		new_flags_and_hash &= ~FLAG_LAST;
		BumpEntry(idx);
	}
	else if ( table[idx].IsValid() )
	{
		// Somebody else is living in our ideal index but does not belong
		// to our entry chain; move it out of the way, start a new chain.
		BumpEntry(idx);
	}
	table[idx].flags_and_hash = new_flags_and_hash;
	return idx;
}


// Key lookup. Can also return previous-in-chain if result is a chained slot.
template <typename KeyT, typename ValueT, typename KeyHashT, typename KeyIsEqualT, typename AltKeyT>
template <typename KeyParamT>
UtlHashHandle_t CUtlHashtable<KeyT, ValueT, KeyHashT, KeyIsEqualT, AltKeyT>::DoLookup( KeyParamT x, unsigned int h, handle_t *pPreviousInChain ) const
{
	if ( m_nUsed == 0 )
	{
		// Empty table.
		return (handle_t) -1;
	}

	const entry_t* table = m_table.Base();
	unsigned int slotmask = m_table.Count()-1;
	Assert( m_table.Count() > 0 && (slotmask & m_table.Count()) == 0 );
	unsigned int chainid = entry_t::IdealIndex( h, slotmask );

	unsigned int idx = chainid;
	if ( table[idx].IdealIndex( slotmask ) != chainid )
	{
		// Nothing in root position? No match.
		return (handle_t) -1;
	}

	// Linear scan until found or end of chain
	handle_t lastIdx = (handle_t) -1;
	while (1)
	{
		// Only examine this slot if it is valid and belongs to our hash chain
		if ( table[idx].IdealIndex( slotmask ) == chainid )
		{
			// Test the full-width hash to avoid unnecessary calls to m_eq()
			if ( ((table[idx].flags_and_hash ^ h) & MASK_HASH) == 0 && m_eq( table[idx]->m_key, x ) )
			{
				// Found match!
				if (pPreviousInChain)
					*pPreviousInChain = lastIdx;

				return (handle_t) idx;
			}

			if ( table[idx].flags_and_hash & FLAG_LAST )
			{
				// End of chain. No match.
				return (handle_t) -1;
			}

			lastIdx = (handle_t) idx;
		}
		idx = (idx + 1) & slotmask;
	}
}


// Key insertion, or return index of existing key if found
template <typename KeyT, typename ValueT, typename KeyHashT, typename KeyIsEqualT, typename AltKeyT>
template <typename KeyParamT>
UtlHashHandle_t CUtlHashtable<KeyT, ValueT, KeyHashT, KeyIsEqualT, AltKeyT>::DoInsert( KeyParamT k, unsigned int h )
{
	handle_t idx = DoLookup<KeyParamT>( k, h, NULL );
	if ( idx == (handle_t) -1 )
	{
		idx = (handle_t) DoInsertUnconstructed( h, true );
		ConstructOneArg( m_table[ idx ].Raw(), k );
	}
	return idx;
}

// Key insertion, or return index of existing key if found
template <typename KeyT, typename ValueT, typename KeyHashT, typename KeyIsEqualT, typename AltKeyT>
template <typename KeyParamT>
UtlHashHandle_t CUtlHashtable<KeyT, ValueT, KeyHashT, KeyIsEqualT, AltKeyT>::DoInsert( KeyParamT k, typename ArgumentTypeInfo<ValueT>::Arg_t v, unsigned int h, bool *pDidInsert )
{
	handle_t idx = DoLookup<KeyParamT>( k, h, NULL );
	if ( idx == (handle_t) -1 )
	{
		idx = (handle_t) DoInsertUnconstructed( h, true );
		ConstructTwoArg( m_table[ idx ].Raw(), k, v );
		if ( pDidInsert ) *pDidInsert = true;
	}
	else
	{
		if ( pDidInsert ) *pDidInsert = false;
	}
	return idx;
}

// Key insertion
template <typename KeyT, typename ValueT, typename KeyHashT, typename KeyIsEqualT, typename AltKeyT>
template <typename KeyParamT>
UtlHashHandle_t CUtlHashtable<KeyT, ValueT, KeyHashT, KeyIsEqualT, AltKeyT>::DoInsertNoCheck( KeyParamT k, typename ArgumentTypeInfo<ValueT>::Arg_t v, unsigned int h )
{
	Assert( DoLookup<KeyParamT>( k, h, NULL ) == (handle_t) -1 );
	handle_t idx = (handle_t) DoInsertUnconstructed( h, true );
	ConstructTwoArg( m_table[ idx ].Raw(), k, v );
	return idx;
}


// Remove single element by key + hash. Returns the location of the new empty hole.
template <typename KeyT, typename ValueT, typename KeyHashT, typename KeyIsEqualT, typename AltKeyT>
template <typename KeyParamT>
int CUtlHashtable<KeyT, ValueT, KeyHashT, KeyIsEqualT, AltKeyT>::DoRemove( KeyParamT x, unsigned int h )
{
	unsigned int slotmask = m_table.Count()-1;
	handle_t previous = (handle_t) -1;
	int idx = (int) DoLookup<KeyParamT>( x, h, &previous );
	if (idx == -1)
	{
		return -1;
	}

	enum { FAKEFLAG_ROOT = 1 };
	int nLastAndRootFlags = m_table[idx].flags_and_hash & FLAG_LAST;
	nLastAndRootFlags |= ( (uint)idx == m_table[idx].IdealIndex( slotmask ) );

	// Remove from table
	m_table[idx].MarkInvalid();
	Destruct( m_table[idx].Raw() );
	--m_nUsed;

	if ( nLastAndRootFlags == FLAG_LAST ) // last only, not root
	{
		// This was the end of the chain - mark previous as last.
		// (This isn't the root, so there must be a previous.)
		Assert( previous != (handle_t) -1 );
		m_table[previous].flags_and_hash |= FLAG_LAST;
	}

	if ( nLastAndRootFlags == FAKEFLAG_ROOT ) // root only, not last
	{
		// If we are removing the root and there is more to the chain,
		// scan to find the next chain entry and move it to the root.
		unsigned int chainid = entry_t::IdealIndex( h, slotmask );
		unsigned int nextIdx = idx;
		while (1)
		{
			nextIdx = (nextIdx + 1) & slotmask;
			if ( m_table[nextIdx].IdealIndex( slotmask ) == chainid )
			{
				break;
			}
		}
		Assert( !(m_table[nextIdx].flags_and_hash & FLAG_FREE) );

		// Leave a hole where the next entry in the chain was.
		m_table[idx].flags_and_hash = m_table[nextIdx].flags_and_hash;
		m_table[idx].MoveDataFrom( m_table[nextIdx] );
		m_table[nextIdx].MarkInvalid();
		return nextIdx;
	}

	// The hole is still where the element used to be.
	return idx;
}


// Assignment operator. It's up to the user to make sure that the hash and equality functors match.
template <typename K, typename V, typename H, typename E, typename A>
CUtlHashtable<K,V,H,E,A> &CUtlHashtable<K,V,H,E,A>::operator=( CUtlHashtable<K,V,H,E,A> const &src )
{
	if ( &src != this )
	{
		Assert( !m_bSizeLocked || m_table.Count() >= src.m_nUsed );
		if ( !m_bSizeLocked )
		{
			Purge();
			Reserve(src.m_nUsed);
		}
		else
		{
			RemoveAll();
		}

		const entry_t * srcTable = src.m_table.Base();
		for ( int i = src.m_table.Count() - 1; i >= 0; --i )
		{
			if ( srcTable[i].IsValid() )
			{
				// If this assert trips, double-check that both hashtables
				// have the same hash function pointers or hash functor state!
				Assert( m_hash(srcTable[i]->m_key) == src.m_hash(srcTable[i]->m_key) );
				int newIdx = DoInsertUnconstructed( srcTable[i].flags_and_hash , false );
				CopyConstruct( m_table[newIdx].Raw(), *srcTable[i].Raw() ); // copy construct KVPair
			}
		}
	}
	return *this;
}

// Remove and return the next valid iterator for a forward iteration.
template <typename KeyT, typename ValueT, typename KeyHashT, typename KeyIsEqualT, typename AltKeyT>
UtlHashHandle_t CUtlHashtable<KeyT, ValueT, KeyHashT, KeyIsEqualT, AltKeyT>::RemoveAndAdvance( UtlHashHandle_t idx )
{
	Assert( IsValidHandle( idx ) );

	// TODO optimize, implement DoRemoveAt that does not need to re-evaluate equality in DoLookup
	int hole = DoRemove< KeyArg_t >( m_table[idx]->m_key, m_table[idx].flags_and_hash & MASK_HASH );
	// DoRemove returns the index of the element that it moved to fill the hole, if any.
	if ( hole <= (int) idx )
	{
		// Didn't fill, or filled from a previously seen element.
		return NextHandle( idx );
	}
	else
	{
		// Do not advance; slot has a new un-iterated value.
		Assert( IsValidHandle(idx) );
		return idx;
	}
}

// Burn it with fire.
template <typename KeyT, typename ValueT, typename KeyHashT, typename KeyIsEqualT, typename AltKeyT>
void CUtlHashtable<KeyT, ValueT, KeyHashT, KeyIsEqualT, AltKeyT>::RemoveAll()
{
	int used = m_nUsed;
	if ( used != 0 )
	{
		entry_t* table = m_table.Base(); 
		for ( int i = m_table.Count() - 1; i >= 0; --i )
		{
			if ( table[i].IsValid() )
			{
				table[i].MarkInvalid();
				Destruct( table[i].Raw() );
				if ( --used == 0 )
					break;
			}
		}
		m_nUsed = 0;
	}
}

template <typename KeyT, typename ValueT, typename KeyHashT, typename KeyIsEqualT, typename AltKeyT>
UtlHashHandle_t CUtlHashtable<KeyT, ValueT, KeyHashT, KeyIsEqualT, AltKeyT>::NextHandle( handle_t start ) const
{
	const entry_t *table = m_table.Base();
	for ( int i = (int)start + 1; i < m_table.Count(); ++i )
	{
		if ( table[i].IsValid() )
			return (handle_t) i;
	}
	return (handle_t) -1;
}


#if _DEBUG
template <typename KeyT, typename ValueT, typename KeyHashT, typename KeyIsEqualT, typename AltKeyT>
void CUtlHashtable<KeyT, ValueT, KeyHashT, KeyIsEqualT, AltKeyT>::DbgCheckIntegrity() const
{
	// Stress test the hash table as a test of both container functionality
	// and also the validity of the user's Hash and Equal function objects.
	// NOTE: will fail if function objects require any sort of state!
	CUtlHashtable clone;
	unsigned int bytes = sizeof(entry_t)*max(16,m_table.Count());
	byte* tempbuf = (byte*) malloc(bytes);
	clone.SetExternalBuffer( tempbuf, bytes, false, false );
	clone = *this;

	int count = 0, roots = 0, ends = 0;
	int slotmask = m_table.Count() - 1;
	for (int i = 0; i < m_table.Count(); ++i)
	{
		if (!(m_table[i].flags_and_hash & FLAG_FREE)) ++count;
		if (m_table[i].IdealIndex(slotmask) == (uint)i) ++roots;
		if (m_table[i].flags_and_hash & FLAG_LAST) ++ends;
		if (m_table[i].IsValid())
		{
			Assert( Find(m_table[i]->m_key) == (handle_t)i );
			Verify( clone.Remove(m_table[i]->m_key) );
		}
		else
		{
			Assert( m_table[i].flags_and_hash == FLAG_FREE );
		}
	}
	Assert( count == Count() && count >= roots && roots == ends );
	Assert( clone.Count() == 0 );
	clone.Purge();
	free(tempbuf);
}
#endif

//-----------------------------------------------------------------------
// CUtlStableHashtable
//-----------------------------------------------------------------------

// Stable hashtables are less memory and cache efficient, but can be
// iterated quickly and their element handles are completely stable.
// Implemented as a hashtable which only stores indices, and a separate
// CUtlLinkedList data table which contains key-value pairs; this may
// change to a more efficient structure in the future if space becomes
// critical. I have some ideas about that but not the time to implement
// at the moment. -henryg

// Note: RemoveAndAdvance is slower than in CUtlHashtable because the
// key needs to be re-hashed under the current implementation.

template <typename KeyT, typename ValueT = empty_t, typename KeyHashT = DefaultHashFunctor<KeyT>, typename KeyIsEqualT = DefaultEqualFunctor<KeyT>, typename IndexStorageT = uint16, typename AlternateKeyT = typename ArgumentTypeInfo<KeyT>::Alt_t >
class CUtlStableHashtable
{
public:
	typedef typename ArgumentTypeInfo<KeyT>::Arg_t KeyArg_t;
	typedef typename ArgumentTypeInfo<ValueT>::Arg_t ValueArg_t;
	typedef typename ArgumentTypeInfo<AlternateKeyT>::Arg_t KeyAlt_t;
	typedef typename CTypeSelect< sizeof( IndexStorageT ) == 2, uint16, uint32 >::type IndexStorage_t;

protected:
	COMPILE_TIME_ASSERT( sizeof( IndexStorage_t ) == sizeof( IndexStorageT ) );

	typedef CUtlKeyValuePair< KeyT, ValueT > KVPair;
	struct HashProxy;
	struct EqualProxy;
	struct IndirectIndex;

	typedef CUtlHashtable< IndirectIndex, empty_t, HashProxy, EqualProxy, AlternateKeyT > Hashtable_t;
	typedef CUtlLinkedList< KVPair, IndexStorage_t > LinkedList_t;

	template <typename KeyArgumentT> bool DoRemove( KeyArgumentT k );
	template <typename KeyArgumentT> UtlHashHandle_t DoFind( KeyArgumentT k ) const;
	template <typename KeyArgumentT> UtlHashHandle_t DoInsert( KeyArgumentT k );
	template <typename KeyArgumentT, typename ValueArgumentT> UtlHashHandle_t DoInsert( KeyArgumentT k, ValueArgumentT v );

public:

	KeyHashT &GetHashRef() { return m_table.GetHashRef().m_hash; }
	KeyIsEqualT &GetEqualRef() { return m_table.GetEqualRef().m_eq; }
	KeyHashT const &GetHashRef() const { return m_table.GetHashRef().m_hash; }
	KeyIsEqualT const &GetEqualRef() const { return m_table.GetEqualRef().m_eq; }

	UtlHashHandle_t Insert( KeyArg_t k ) { return DoInsert<KeyArg_t>( k ); }
	UtlHashHandle_t Insert( KeyAlt_t k ) { return DoInsert<KeyAlt_t>( k ); }
	UtlHashHandle_t Insert( KeyArg_t k, ValueArg_t v ) { return DoInsert<KeyArg_t, ValueArg_t>( k, v ); }
	UtlHashHandle_t Insert( KeyAlt_t k, ValueArg_t v ) { return DoInsert<KeyAlt_t, ValueArg_t>( k, v ); }
	UtlHashHandle_t Find( KeyArg_t k ) const { return DoFind<KeyArg_t>( k ); }
	UtlHashHandle_t Find( KeyAlt_t k ) const { return DoFind<KeyAlt_t>( k );  }
	bool Remove( KeyArg_t k ) { return DoRemove<KeyArg_t>( k ); }
	bool Remove( KeyAlt_t k ) { return DoRemove<KeyAlt_t>( k ); }

	void RemoveAll() { m_table.RemoveAll(); m_data.RemoveAll(); }
	void Purge() { m_table.Purge(); m_data.Purge(); }
	int Count() const { return m_table.Count(); }

	typedef typename KVPair::ValueReturn_t Element_t;
	KeyT const &Key( UtlHashHandle_t idx ) const { return m_data[idx].m_key; }
	Element_t const &Element( UtlHashHandle_t idx ) const { return m_data[idx].GetValue(); }
	Element_t &Element( UtlHashHandle_t idx ) { return m_data[idx].GetValue(); }
	Element_t const &operator[]( UtlHashHandle_t idx ) const { return m_data[idx].GetValue(); }
	Element_t &operator[]( UtlHashHandle_t idx ) { return m_data[idx].GetValue(); }

	void ReplaceKey( UtlHashHandle_t idx, KeyArg_t k ) { Assert( GetEqualRef()( m_data[idx].m_key, k ) && GetHashRef()( k ) == GetHashRef()( m_data[idx].m_key ) ); m_data[idx].m_key = k; }
	void ReplaceKey( UtlHashHandle_t idx, KeyAlt_t k ) { Assert( GetEqualRef()( m_data[idx].m_key, k ) && GetHashRef()( k ) == GetHashRef()( m_data[idx].m_key ) ); m_data[idx].m_key = k; }

	Element_t const &Get( KeyArg_t k, Element_t const &defaultValue ) const { UtlHashHandle_t h = Find( k ); if ( h != InvalidHandle() ) return Element( h ); return defaultValue; }
	Element_t const &Get( KeyAlt_t k, Element_t const &defaultValue ) const { UtlHashHandle_t h = Find( k ); if ( h != InvalidHandle() ) return Element( h ); return defaultValue; }

	Element_t const *GetPtr( KeyArg_t k ) const { UtlHashHandle_t h = Find(k); if ( h != InvalidHandle() ) return &Element( h ); return NULL; }
	Element_t const *GetPtr( KeyAlt_t k ) const { UtlHashHandle_t h = Find(k); if ( h != InvalidHandle() ) return &Element( h ); return NULL; }
	Element_t *GetPtr( KeyArg_t k ) { UtlHashHandle_t h = Find( k ); if ( h != InvalidHandle() ) return &Element( h ); return NULL; }
	Element_t *GetPtr( KeyAlt_t k ) { UtlHashHandle_t h = Find( k ); if ( h != InvalidHandle() ) return &Element( h ); return NULL; }

	UtlHashHandle_t FirstHandle() const { return ExtendInvalidHandle( m_data.Head() ); }
	UtlHashHandle_t NextHandle( UtlHashHandle_t h ) const { return ExtendInvalidHandle( m_data.Next( h ) ); }
	bool IsValidHandle( UtlHashHandle_t h ) const { return m_data.IsValidIndex( h ); }
	UtlHashHandle_t InvalidHandle() const { return (UtlHashHandle_t)-1; }

	UtlHashHandle_t RemoveAndAdvance( UtlHashHandle_t h )
	{
		Assert( m_data.IsValidIndex( h ) );
		m_table.Remove( IndirectIndex( h ) );
		IndexStorage_t next = m_data.Next( h );
		m_data.Remove( h );
		return ExtendInvalidHandle(next);
	}

	void Compact( bool bMinimal ) { m_table.Compact( bMinimal ); /*m_data.Compact();*/ }

	void Swap( CUtlStableHashtable &other )
	{
		m_table.Swap(other.m_table);
		// XXX swapping CUtlLinkedList by block memory swap, ugh
		char buf[ sizeof(m_data) ];
		memcpy( buf, &m_data, sizeof(m_data) );
		memcpy( &m_data, &other.m_data, sizeof(m_data) );
		memcpy( &other.m_data, buf, sizeof(m_data) );
	}


protected:
	// Perform extension of 0xFFFF to 0xFFFFFFFF if necessary. Note: ( a < CONSTANT ) ? 0 : -1 is usually branchless
	static UtlHashHandle_t ExtendInvalidHandle( uint32 x ) { return x; }
	static UtlHashHandle_t ExtendInvalidHandle( uint16 x ) { uint32 a = x; return a | ( ( a < 0xFFFFu ) ? 0 : -1 ); }

	struct IndirectIndex
	{
		explicit IndirectIndex(IndexStorage_t i) : m_index(i) { }
		IndexStorage_t m_index;
	};

	struct HashProxy 
	{
		KeyHashT m_hash;
		unsigned int operator()( IndirectIndex idx ) const
		{
			const ptrdiff_t tableoffset = (uintptr_t)(&((Hashtable_t*)1024)->GetHashRef()) - 1024;
			const ptrdiff_t owneroffset = offsetof(CUtlStableHashtable, m_table) + tableoffset;
			CUtlStableHashtable* pOwner = (CUtlStableHashtable*)((uintptr_t)this - owneroffset);
			return m_hash( pOwner->m_data[ idx.m_index ].m_key );
		}
		unsigned int operator()( KeyArg_t k ) const { return m_hash( k ); }
		unsigned int operator()( KeyAlt_t k ) const { return m_hash( k ); }
	};

	struct EqualProxy
	{
		KeyIsEqualT m_eq;
		unsigned int operator()( IndirectIndex lhs, IndirectIndex rhs ) const
		{
			return lhs.m_index == rhs.m_index;
		}
		unsigned int operator()( IndirectIndex lhs, KeyArg_t rhs ) const
		{
			const ptrdiff_t tableoffset = (uintptr_t)(&((Hashtable_t*)1024)->GetEqualRef()) - 1024;
			const ptrdiff_t owneroffset = offsetof(CUtlStableHashtable, m_table) + tableoffset;
			CUtlStableHashtable* pOwner = (CUtlStableHashtable*)((uintptr_t)this - owneroffset);
			return m_eq( pOwner->m_data[ lhs.m_index ].m_key, rhs );
		}
		unsigned int operator()( IndirectIndex lhs, KeyAlt_t rhs ) const
		{
			const ptrdiff_t tableoffset = (uintptr_t)(&((Hashtable_t*)1024)->GetEqualRef()) - 1024;
			const ptrdiff_t owneroffset = offsetof(CUtlStableHashtable, m_table) + tableoffset;
			CUtlStableHashtable* pOwner = (CUtlStableHashtable*)((uintptr_t)this - owneroffset);
			return m_eq( pOwner->m_data[ lhs.m_index ].m_key, rhs );
		}
	};

	class CCustomLinkedList : public LinkedList_t
	{
	public:
		int AddToTailUnconstructed()
		{
			IndexStorage_t newNode = this->AllocInternal();
			if ( newNode != this->InvalidIndex() )
				this->LinkToTail( newNode );
			return newNode;
		}
	};

	Hashtable_t m_table;
	CCustomLinkedList m_data;
};

template <typename K, typename V, typename H, typename E, typename S, typename A>
template <typename KeyArgumentT>
inline bool CUtlStableHashtable<K,V,H,E,S,A>::DoRemove( KeyArgumentT k )
{
	unsigned int hash = m_table.GetHashRef()( k );
	UtlHashHandle_t h = m_table.template DoLookup<KeyArgumentT>( k, hash, NULL );
	if ( h == m_table.InvalidHandle() )
		return false;

	int idx = m_table[ h ].m_index;
	m_table.template DoRemove<IndirectIndex>( IndirectIndex( idx ), hash );
	m_data.Remove( idx );
	return true;
}

template <typename K, typename V, typename H, typename E, typename S, typename A>
template <typename KeyArgumentT>
inline UtlHashHandle_t CUtlStableHashtable<K,V,H,E,S,A>::DoFind( KeyArgumentT k ) const
{
	unsigned int hash = m_table.GetHashRef()( k );
	UtlHashHandle_t h = m_table.template DoLookup<KeyArgumentT>( k, hash, NULL );
	if ( h != m_table.InvalidHandle() )
		return m_table[ h ].m_index;

	return (UtlHashHandle_t) -1;
}

template <typename K, typename V, typename H, typename E, typename S, typename A>
template <typename KeyArgumentT>
inline UtlHashHandle_t CUtlStableHashtable<K,V,H,E,S,A>::DoInsert( KeyArgumentT k )
{
	unsigned int hash = m_table.GetHashRef()( k );
	UtlHashHandle_t h = m_table.template DoLookup<KeyArgumentT>( k, hash, NULL );
	if ( h != m_table.InvalidHandle() )
		return m_table[ h ].m_index;

	int idx = m_data.AddToTailUnconstructed();
	ConstructOneArg( &m_data[idx], k );
	m_table.template DoInsertNoCheck<IndirectIndex>( IndirectIndex( idx ), empty_t(), hash );
	return idx;
}

template <typename K, typename V, typename H, typename E, typename S, typename A>
template <typename KeyArgumentT, typename ValueArgumentT>
inline UtlHashHandle_t CUtlStableHashtable<K,V,H,E,S,A>::DoInsert( KeyArgumentT k, ValueArgumentT v )
{
	unsigned int hash = m_table.GetHashRef()( k );
	UtlHashHandle_t h = m_table.template DoLookup<KeyArgumentT>( k, hash, NULL );
	if ( h != m_table.InvalidHandle() )
		return m_table[ h ].m_index;

	int idx = m_data.AddToTailUnconstructed();
	ConstructTwoArg( &m_data[idx], k, v );
	m_table.template DoInsertNoCheck<IndirectIndex>( IndirectIndex( idx ), empty_t(), hash );
	return idx;
}

#endif // UTLHASHTABLE_H
