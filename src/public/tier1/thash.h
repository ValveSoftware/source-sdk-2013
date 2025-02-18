//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef THASH_H
#define THASH_H
#ifdef _WIN32
#pragma once
#endif

#include <typeinfo>

//#define DBGFLAG_THASH			// Perform extra sanity checks on the THash


// THash
// This is a heavyweight, templatized version of DHash.
// It differs from DHash in the following ways:
// - It's templetized, and automatically constructs and destructs its records as appropriate
// - It provides a scheduling service, which can be used to touch every record in the table
//		at a specified interval.  The scheduler is low-overhead, and provides a very smooth
//		distribution of touches across frames.

// Template arguments:
//		Data: the class to be stored in the hash table
//		I: the type of the primary key (uint32 by default)
template<class Data, typename I=uint32>
class CTHash
{
private:
	// RecHdr
	// We insert one of these at the beginning of every record.  It's used for
	// keeping the records in a linked list.
	typedef struct RecHdr_t
	{
		RecHdr_t *m_pRecHdrNext;		// Next item in our linked list
		RecHdr_t *m_pRecHdrPrev;		// Previous item in our linked list
		I m_unKey;						// Key of this item
		int m_iBucket;					// The bucket we're in
		int m_nRunRatio;				// We want to run 1 cycle out of every m_nRunRatio
										// cycles (not at all if 0).
#ifdef DBGFLAG_THASH
		uint m_iCycleLast;				// Last cycle we were visited (whether or not we ran)
#endif
	} RecHdr_t;

	// Bucket
	// Each hash bucket is represented by a Bucket structure, which points to the 
	// first record with the bucket's hash.
	typedef struct Bucket_t
	{
		RecHdr_t *m_pRecHdrFirst;		// First record in our list
	} Bucket_t;


public:
	// Constructors & destructors
	CTHash( int cFramesPerCycle );
	~CTHash();

	// Initializer
	void Init( int cRecordInit, int cBuckets );

	// Insert a record into the table
	Data *PvRecordInsert( I unKey );
	// Insert a record into the table and set the allocated object's pointer as the hash key
	Data *PvRecordInsertAutoKey();
	// Changes the key for a previously inserted item
	void ChangeKey( Data * pvRecord, I unOldKey, I unNewKey );

	// Remove a record
	void Remove( I unKey );
	// Remove a record
	void Remove( Data * pvRecord );
	// Remove all records
	void RemoveAll();
	
	// Find a record
	Data *PvRecordFind( I unKey ) const;

	// How many records do we have
	int Count() const { return m_cRecordInUse; }

	// Iterate through our members
	Data *PvRecordFirst() const;
	Data *PvRecordNext( Data *pvRecordCur ) const;

	// We provide a scheduling service.  Call StartFrameSchedule when you want to start running
	// records in a given frame, and repeatedly call PvRecordRun until it returns NULL (or you
	// run our of time).
	void SetRunRatio( Data *pvRecord, int nRunRatio );
	void SetMicroSecPerCycle( int cMicroSecPerCycle, int cMicroSecPerFrame ) { m_cFramesPerCycle = cMicroSecPerCycle / cMicroSecPerFrame; }
	void StartFrameSchedule( bool bNewFrame );
	Data *PvRecordRun();

	bool BCompletedPass();

#ifdef DBGFLAG_VALIDATE
	virtual void Validate( CValidator &validator, const char *pchName );
#endif // DBGFLAG_VALIDATE


private:
	// Insert a record into the table
	Data *PvRecordInsertInternal( RecHdr_t *pRecHdr, I unKey );

	// Get the record associated with a THashHdr
	Data *PvRecordFromPRecHdr( RecHdr_t *pRecHdr ) const { return ( Data * ) ( ( ( uint8 * ) pRecHdr + sizeof( RecHdr_t ) ) ); }

	// Get the RecHdr preceding a PvRecord
	RecHdr_t *PRecHdrFromPvRecord( Data *pvRecord ) const { return ( ( ( RecHdr_t * ) pvRecord ) - 1 ); }

	// Get the hash bucket corresponding to a key
	int IBucket( I unKey ) const;

	void InsertIntoHash( RecHdr_t *pRecHdr, I unKey );
	void RemoveFromHash( Data * pvRecord );

	int m_cBucket;						// # of hash buckets we have
	Bucket_t *m_pBucket;				// Big array of hash buckets

	CUtlMemoryPool *m_pMemoryPoolRecord;	// All our data records

	int m_cRecordInUse;					// # of records in use
	RecHdr_t m_RecHdrHead;				// Head of our linked list
	RecHdr_t m_RecHdrTail;				// Tail of our linked list

	int m_cFramesPerCycle;				// Run each of our records once every m_cFramesPerCycle frames
	RecHdr_t *m_pRecHdrRunNext;			// Next record to run (be careful-- this is more complicated than it sounds)
	int m_iBucketRunMax;				// Stop running when we get to this bucket
	uint m_iCycleCur;					// Current cycle (ie, how many times we've made a complete scheduler pass)
	uint m_iCycleLast;					// Our previous cycle
	uint m_iFrameCur;					// Our current frame (incremented once each StartFrameSchedule)
	uint m_iCycleLastReported;			// Last cycle checked by BCompletedPass()
};


//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input:	cMicroSecRunInterval -		How often we want the scheduler to run each of our records
//-----------------------------------------------------------------------------
template<class Data, class I>
CTHash<Data,I>::CTHash( int cFramesPerCycle )
{
	m_cBucket = 0;
	m_pBucket = NULL;
	m_pMemoryPoolRecord = NULL;
	m_cRecordInUse = 0;

	m_cFramesPerCycle = cFramesPerCycle;
	m_pRecHdrRunNext = &m_RecHdrTail;		// This will make us start at the beginning on our first frame
	m_iBucketRunMax = 0;
	m_iCycleCur = 0;
	m_iCycleLast = 0;
	m_iFrameCur = 0;
	m_iCycleLastReported = 0;

	m_RecHdrHead.m_pRecHdrPrev = NULL;
	m_RecHdrHead.m_pRecHdrNext = &m_RecHdrTail;
	m_RecHdrHead.m_iBucket = -1;

	m_RecHdrTail.m_pRecHdrPrev = &m_RecHdrHead;
	m_RecHdrTail.m_pRecHdrNext = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
template<class Data, class I>
CTHash<Data,I>::~CTHash()
{
	RemoveAll();

	if ( NULL != m_pBucket )
		FreePv( m_pBucket );
	m_pBucket = NULL;

	if ( NULL != m_pMemoryPoolRecord )
		delete( m_pMemoryPoolRecord );
	m_pMemoryPoolRecord = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Initializer.  Allocate our various arrays, and set up the free
//			list.
// Input:	cRecordInit -		Initial # of data records we can contain
//			cBucket -			# of hash buckets we should use
//-----------------------------------------------------------------------------
template<class Data, class I>
void CTHash<Data,I>::Init( int cRecordInit, int cBucket )
{
	Assert( cRecordInit > 0 );	// need to init with non-zero value or memory pool will never grow

	// Copy our parameters
	m_cBucket = cBucket;

	// Alloc our arrays
	m_pBucket = ( Bucket_t * ) PvAlloc( sizeof( Bucket_t ) * m_cBucket );
	m_pMemoryPoolRecord = new CUtlMemoryPool( sizeof( Data ) + sizeof( RecHdr_t ), cRecordInit,
		CUtlMemoryPool::GROW_SLOW );

	// Init the hash buckets
	for ( int iBucket = 0; iBucket < m_cBucket; iBucket++ )
		m_pBucket[iBucket].m_pRecHdrFirst = NULL;

	// Make the tail have an illegally large bucket
	m_RecHdrTail.m_iBucket = m_cBucket + 1;
}


//-----------------------------------------------------------------------------
// Purpose: Inserts a new record into the table
// Input:	unKey -			Primary key of the new record
// Output:	Pointer to the new record
//-----------------------------------------------------------------------------
template<class Data, class I>
Data *CTHash<Data,I>::PvRecordInsert( I unKey )
{
	Assert( PvRecordFind( unKey ) == NULL );	// keys are unique; no record with this key may exist

	// Find a free record
	RecHdr_t *pRecHdr = ( RecHdr_t * ) m_pMemoryPoolRecord->Alloc();
	
	return PvRecordInsertInternal( pRecHdr, unKey );
}


//-----------------------------------------------------------------------------
// Purpose: Inserts a new record into the table and sets its key to the pointer
//			value of the record
// Output:	Pointer to the new record
//-----------------------------------------------------------------------------
template<class Data, class I>
Data *CTHash<Data,I>::PvRecordInsertAutoKey()
{
	// Find a free record
	RecHdr_t *pRecHdr = ( RecHdr_t * ) m_pMemoryPoolRecord->Alloc();
	
	return PvRecordInsertInternal( pRecHdr, (I) PvRecordFromPRecHdr( pRecHdr ) );
}


//-----------------------------------------------------------------------------
// Purpose: Inserts an allocated record into the hash table with specified key
//			and calls the constructor of the allocated object
// Input:	pRecHdr - record to insert
//			unKey - hash key to use for record
// Output:	Pointer to the new record
//-----------------------------------------------------------------------------
template<class Data, class I>
Data *CTHash<Data,I>::PvRecordInsertInternal( RecHdr_t *pRecHdr, I unKey )
{
	InsertIntoHash( pRecHdr, unKey );

	// assert that we don't have too many items per bucket
	static bool s_bPerfWarning = false;
	if ( !s_bPerfWarning && Count() >= ( 5 * m_cBucket ) )
	{
		s_bPerfWarning = true;
		AssertMsg( false, "Performance warning: too many items, not enough buckets" );
		Msg( "not enough buckets in thash class %s (%d records, %d buckets)\n",
#ifdef _WIN32
		 typeid(*this).raw_name(),
#else
		typeid(*this).name(),
#endif
		Count(), m_cBucket );
	}

	// Construct ourselves
	Data *pData = PvRecordFromPRecHdr( pRecHdr );
	Construct<Data>( pData );
	return pData;
}

//-----------------------------------------------------------------------------
// Purpose: Changes key on previously inserted item
// Input:	pvRecord -		record to change key for
//			unOldKey -		old key (not strictly needed, but helpful consistency check)
//			unNewKey -		new key to use
//-----------------------------------------------------------------------------
template<class Data, class I>
void CTHash<Data,I>::ChangeKey( Data * pvRecord, I unOldKey, I unNewKey )
{
	Data * pvRecordFound = PvRecordFind( unOldKey );
	Assert( pvRecordFound == pvRecord );
	if ( pvRecordFound == pvRecord )
	{
		RemoveFromHash( pvRecord );
		InsertIntoHash( PRecHdrFromPvRecord( pvRecord), unNewKey );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Removes the entry with a specified key from the table
// Input:	unKey -		Key of the entry to remove
//-----------------------------------------------------------------------------
template<class Data, class I>
void CTHash<Data,I>::Remove( I unKey )
{
	Data *pvRemove = ( Data * ) PvRecordFind( unKey );
	Assert( pvRemove );
	if ( !pvRemove )
		return;
	Remove( pvRemove );
}


//-----------------------------------------------------------------------------
// Purpose: Removes the specified entry from the table
// Input:	pvRemove -		Pointer to the entry to remove
//-----------------------------------------------------------------------------
template<class Data, class I>
void CTHash<Data,I>::Remove( Data * pvRemove )
{
	// Destruct the record we're removing
	Destruct<Data>( pvRemove );

	RemoveFromHash( pvRemove );
	m_pMemoryPoolRecord->Free( PRecHdrFromPvRecord( pvRemove ) );
}


//-----------------------------------------------------------------------------
// Purpose: Removes all entries from the table
//-----------------------------------------------------------------------------
template<class Data, class I>
void CTHash<Data,I>::RemoveAll()
{
	Data * pvRecord = PvRecordFirst();
	while ( pvRecord )
	{
		Data *pvRecordNext = PvRecordNext( pvRecord );
		Remove( pvRecord );
		pvRecord = pvRecordNext;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Finds the entry with a specified key
// Input:	unKey -		Key to find
//-----------------------------------------------------------------------------
template<class Data, class I>
Data *CTHash<Data,I>::PvRecordFind( I unKey ) const
{
	// Find our hash bucket
	int iBucket = IBucket( unKey );

	// Walk the bucket's list looking for an exact match
	for ( RecHdr_t *pRecHdr = m_pBucket[iBucket].m_pRecHdrFirst;
		NULL != pRecHdr && pRecHdr->m_iBucket == iBucket;
		pRecHdr = pRecHdr->m_pRecHdrNext )
	{
		if ( unKey == pRecHdr->m_unKey )
			return PvRecordFromPRecHdr( pRecHdr );
	}

	// Didn't find a match
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Finds our first record
// Output:	Pointer to our first record
//-----------------------------------------------------------------------------
template<class Data, class I>
Data *CTHash<Data,I>::PvRecordFirst() const
{
	if ( &m_RecHdrTail != m_RecHdrHead.m_pRecHdrNext )
		return PvRecordFromPRecHdr( m_RecHdrHead.m_pRecHdrNext );
	else
		return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Iterates to the record after a given record
// Input:	Pointer to a current record
// Output:	Pointer to the next record
//-----------------------------------------------------------------------------
template<class Data, class I>
Data *CTHash<Data,I>::PvRecordNext( Data *pvRecordCur ) const
{
	RecHdr_t *pRecHdr = PRecHdrFromPvRecord( pvRecordCur );
	if ( &m_RecHdrTail == pRecHdr->m_pRecHdrNext )
		return NULL;

	return PvRecordFromPRecHdr( pRecHdr->m_pRecHdrNext );
}


//-----------------------------------------------------------------------------
// Purpose: Sets the run ratio of a particular record in the hash table.
//			The record will be run 1 cycle out of every nRunRatio cycles.
// Input:	pvRecord -		The record we're setting
//			nRunRatio -		The run ratio for this record
//-----------------------------------------------------------------------------
template<class Data, class I>
void CTHash<Data,I>::SetRunRatio( Data *pvRecord, int nRunRatio )
{
	PRecHdrFromPvRecord( pvRecord )->m_nRunRatio = nRunRatio;
}


//-----------------------------------------------------------------------------
// Purpose: Prepares us to run all records that are due to be run this frame.
//			Records are run at a particular time dependent on their hash bucket,
//			regardless of when they were last run.
// Input:	bNewFrame -		True if this is a new frame.  If false, we've run
//							off the end of the list and are checking whether
//							we need to keep going at the beginning.
//-----------------------------------------------------------------------------
template<class Data, class I>
void CTHash<Data,I>::StartFrameSchedule( bool bNewFrame )
{
	// Calculate our current frame and cycle cycle
	if ( bNewFrame )
	{
		m_iCycleLast = m_iCycleCur;
		m_iFrameCur++;
		m_iCycleCur = m_iFrameCur / m_cFramesPerCycle;
	}

	// Calculate the last bucket to run
	int iFrameInCycle = m_iFrameCur % m_cFramesPerCycle;
	m_iBucketRunMax = ( int ) ( ( ( int64 ) ( iFrameInCycle + 1 ) * ( int64 ) m_cBucket )
		/ ( int64 ) m_cFramesPerCycle );
	AssertFatal( m_iBucketRunMax >= 0 && m_iBucketRunMax <= m_cBucket );

	// Are we starting a new cycle?
	if ( m_iCycleCur > m_iCycleLast )
	{
#ifdef DBGFLAG_THASH
		Assert( m_iCycleCur == m_iCycleLast + 1 );
#endif

		// Did we finish the last cycle?
		if ( &m_RecHdrTail == m_pRecHdrRunNext )
		{
			m_pRecHdrRunNext = m_RecHdrHead.m_pRecHdrNext;
		}
		// No-- finish it up before moving on
		else
		{
			m_iBucketRunMax = m_cBucket + 1;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the next record to run, if any
// Output:	Pointer to the next record that needs to run (NULL if we're done)
//-----------------------------------------------------------------------------
template<class Data, class I>
Data *CTHash<Data,I>::PvRecordRun()
{
	// Loop until we find a record to run, or until we pass m_iBucketRunMax
	for ( ; ; )
	{
		// Are we past our stopping point?
		if ( m_pRecHdrRunNext->m_iBucket >= m_iBucketRunMax )
		{
			// If this cycle ran to the very end, see if we need to start over
			if ( m_iBucketRunMax > m_cBucket )
			{
				StartFrameSchedule( false );
				continue;
			}

			return NULL;
		}

#ifdef DBGFLAG_THASH
		Assert( m_pRecHdrRunNext->m_iBucket >= m_iBucketRunFirst );
		if ( 0 != m_pRecHdrRunNext->m_iCycleLast )
		{
			if ( m_pRecHdrRunNext->m_iCycleLast == m_iCycleCur )
			{
				DMsg( SPEW_CONSOLE,  1, "Double cycle: hdr = 0x%x, last frame = %d, curFrame = %d, first = %d, last = %d, bucket = %d\n",
					m_pRecHdrRunNext, m_pRecHdrRunNext->m_iFrameLast, m_iFrame,
					m_iBucketRunFirst, m_iBucketRunMax, m_pRecHdrRunNext->m_iBucket );
			}
			else if ( m_pRecHdrRunNext->m_iCycleLast != m_iCycleCur - 1 )
			{
				DMsg( SPEW_CONSOLE,  1, "Skipped cycle: hdr = 0x%x, cycleLast = %u, cycleCur = %u (missed %u cycles)\n",
					m_pRecHdrRunNext, m_pRecHdrRunNext->m_iCycleLast, m_iCycleCur,
					m_iCycleCur - m_pRecHdrRunNext->m_iCycleLast );
				Assert( false );
			}
		}
		m_pRecHdrRunNext->m_iCycleLast = m_iCycleCur;
		m_pRecHdrRunNext->m_iFrameLast = m_iFrame;
#endif

		// Set up the record to run next time
		RecHdr_t *pRecHdrCur = m_pRecHdrRunNext;
		m_pRecHdrRunNext = m_pRecHdrRunNext->m_pRecHdrNext;

		// Does this record need to run?
		if ( 0 == pRecHdrCur->m_nRunRatio )
			continue;

		if ( 0 == m_iCycleCur % pRecHdrCur->m_nRunRatio )
			return PvRecordFromPRecHdr( pRecHdrCur );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Return true if we've completed a scheduler pass since last called
//-----------------------------------------------------------------------------
template<class Data, class I>
bool CTHash<Data,I>::BCompletedPass()
{
	if ( m_iCycleCur != m_iCycleLastReported )
	{
		m_iCycleLastReported = m_iCycleCur;
		return true;
	}
	return false;
}


extern const unsigned char g_CTHashRandomValues[256];  // definition lives in globals.cpp

//-----------------------------------------------------------------------------
// Purpose: Returns the index of the hash bucket corresponding to a particular key
// Input:	unKey -		Key to find
// Output:	Index of the hash bucket corresponding to unKey
//-----------------------------------------------------------------------------
template<class Data, class I>
int CTHash<Data,I>::IBucket( I unKey ) const
{
	AssertFatal( m_cBucket > 0 );

	// This is a pearsons hash variant that returns a maximum of 32 bits
	size_t size = sizeof(I);
	const uint8 *	k    = (const uint8 *) &unKey;
	uint32 byte_one = 0, byte_two = 0, byte_three = 0, byte_four = 0, n;

	while (size)
	{
		--size;
		n    = *k++;
		byte_one = g_CTHashRandomValues[byte_one ^ n];
		
		if (size)
		{
			--size;
			n   = *k++;
			byte_two = g_CTHashRandomValues[byte_two ^ n];
		}
		else
			break;
		
		if (size)
		{
			--size;
			n   = *k++;
			byte_three = g_CTHashRandomValues[byte_three ^ n];
		}
		else
			break;
		
		if (size)
		{
			--size;
			n   = *k++;
			byte_four = g_CTHashRandomValues[byte_four ^ n];
		}
		else
			break;
	}

	uint32 idx = ( byte_four << 24 ) | ( byte_three << 16 ) | ( byte_two << 8 ) | byte_one;
	idx = idx % m_cBucket;
	return ( (int) idx ); 
}


#ifdef DBGFLAG_VALIDATE
//-----------------------------------------------------------------------------
// Purpose: Run a global validation pass on all of our data structures and memory
//			allocations.
// Input:	validator -		Our global validator object
//			pchName -		Our name (typically a member var in our container)
//-----------------------------------------------------------------------------
template<class Data, class I>
void CTHash<Data,I>::Validate( CValidator &validator, const char *pchName )
{
	VALIDATE_SCOPE();

	validator.ClaimMemory( m_pBucket );
	ValidatePtr( m_pMemoryPoolRecord );

#if defined( _DEBUG )
	// first verify m_cRecordInUse
	Data * pvRecord = PvRecordFirst();
	int cItems = 0;
	while ( pvRecord )
	{
		Data *pvRecordNext = PvRecordNext( pvRecord );
		cItems++;
		pvRecord = pvRecordNext;
	}
	Assert( m_cRecordInUse == cItems );
	// then ask the mempool to verify this
	if ( m_pMemoryPoolRecord )
		m_pMemoryPoolRecord->LeakCheck( cItems );
#endif // _DEBUG
}
#endif // DBGFLAG_VALIDATE


//-----------------------------------------------------------------------------
// Purpose: Inserts a new record into the table
// Input:	unKey -			Primary key of the new record
// Output:	Pointer to the new record
//-----------------------------------------------------------------------------
template<class Data, class I>
void CTHash<Data,I>::InsertIntoHash( RecHdr_t *pRecHdr, I unKey )
{
	m_cRecordInUse++;

	// Init the RecHdr
	pRecHdr->m_unKey = unKey;
	pRecHdr->m_nRunRatio = 1;

	// Find our hash bucket
	int iBucket = IBucket( unKey );
	pRecHdr->m_iBucket = iBucket;
#ifdef DBGFLAG_THASH
	pRecHdr->m_iCycleLast = 0;
#endif

	// Find where to insert ourselves in the linked list
	RecHdr_t *pRecHdrInsertBefore = &m_RecHdrTail;
	// Find the first bucket with anything in it that's at or after our bucket
	for ( int iBucketT = iBucket; iBucketT < m_cBucket; iBucketT++ )
	{
		if ( NULL != m_pBucket[iBucketT].m_pRecHdrFirst )
		{
			pRecHdrInsertBefore = m_pBucket[iBucketT].m_pRecHdrFirst;
			break;
		}
	}

	// Insert ourselves
	pRecHdr->m_pRecHdrNext = pRecHdrInsertBefore;
	pRecHdr->m_pRecHdrPrev = pRecHdrInsertBefore->m_pRecHdrPrev;
	pRecHdrInsertBefore->m_pRecHdrPrev = pRecHdr;
	pRecHdr->m_pRecHdrPrev->m_pRecHdrNext = pRecHdr;

	// Our bucket should point to us
	m_pBucket[iBucket].m_pRecHdrFirst = pRecHdr;
}


//-----------------------------------------------------------------------------
// Purpose: Removes the specified entry from the table
// Input:	pvRemove -		Pointer to the entry to remove
//-----------------------------------------------------------------------------
template<class Data, class I>
void CTHash<Data,I>::RemoveFromHash( Data * pvRemove )
{
	// Find our RecHdr
	RecHdr_t *pRecHdr = PRecHdrFromPvRecord( pvRemove );

	// If our bucket points to us, point it to the next record (or else NULL)
	int iBucket = IBucket( pRecHdr->m_unKey );
	if ( pRecHdr == m_pBucket[iBucket].m_pRecHdrFirst )
	{
		if ( pRecHdr->m_pRecHdrNext->m_iBucket == iBucket )
			m_pBucket[iBucket].m_pRecHdrFirst = pRecHdr->m_pRecHdrNext;
		else
			m_pBucket[iBucket].m_pRecHdrFirst = NULL;
	}

	// Remove us from the linked list
	pRecHdr->m_pRecHdrPrev->m_pRecHdrNext = pRecHdr->m_pRecHdrNext;
	pRecHdr->m_pRecHdrNext->m_pRecHdrPrev = pRecHdr->m_pRecHdrPrev;

	// Are we the next record to run?
	if ( pRecHdr == m_pRecHdrRunNext )
		m_pRecHdrRunNext = pRecHdr->m_pRecHdrNext;

	m_cRecordInUse--;
}

#endif // THASH_H
