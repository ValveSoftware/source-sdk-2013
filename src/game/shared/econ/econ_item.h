//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CEconItem, a shared object for econ items
//
//=============================================================================

#ifndef ECONITEM_H
#define ECONITEM_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/gcclientsdk.h"
#include "base_gcmessages.pb.h"

#include "econ_item_constants.h"
#include "econ_item_interface.h"
#include "econ_item_schema.h"

#include <typeinfo>			// needed for typeid()

#define ENABLE_TYPED_ATTRIBUTE_PARANOIA		1


namespace GCSDK
{
	class CColumnSet;
};

class CEconItem;
class CSOEconItem;
class CEconItemCustomData;
class CEconSessionItemAudit;

//-----------------------------------------------------------------------------
// Stats tracking for the attributes attached to CEconItem instances.
//-----------------------------------------------------------------------------
struct schema_attribute_stat_bucket_t
{
	const schema_attribute_stat_bucket_t *m_pNext;

	const char *m_pszDesc;
	uint64 m_unLiveInlineCount;
	uint64 m_unLifetimeInlineCount;
	uint64 m_unLiveHeapCount;
	uint64 m_unLifetimeHeapCount;

	void OnAllocateInlineInstance() { m_unLiveInlineCount++; m_unLifetimeInlineCount++; }
	void OnFreeInlineInstance() { Assert( m_unLiveInlineCount > 0 ); m_unLiveInlineCount--; }
	void OnAllocateHeapInstance() { m_unLiveHeapCount++; m_unLifetimeHeapCount++; }
	void OnFreeHeapInstance() { Assert( m_unLiveHeapCount ); m_unLiveHeapCount--; }
};

class CSchemaAttributeStats
{
public:
	template < typename TAttribStatsStorageClass, typename TAttribInMemoryType >
	static void RegisterAttributeType()
	{
		TAttribStatsStorageClass::s_InstanceStats.m_pszDesc = typeid( TAttribInMemoryType ).name();
		TAttribStatsStorageClass::s_InstanceStats.m_pNext = m_pHead;

		m_pHead = &TAttribStatsStorageClass::s_InstanceStats;
	}

	static const schema_attribute_stat_bucket_t *GetFirstStatBucket()
	{
		return m_pHead;
	}

private:
	static const schema_attribute_stat_bucket_t *m_pHead;
};

//-----------------------------------------------------------------------------
// Base class interface for attributes of a certain in-memory type.
//-----------------------------------------------------------------------------
unsigned int Internal_GetAttributeTypeUniqueIdentifierNextValue();

template < typename T >
unsigned int GetAttributeTypeUniqueIdentifier()
{
	static unsigned int s_unUniqueCounter = Internal_GetAttributeTypeUniqueIdentifierNextValue();
	return s_unUniqueCounter;
}

//-----------------------------------------------------------------------------
// Base class interface for attributes of a certain in-memory type.
//-----------------------------------------------------------------------------
template < typename TAttribInMemoryType >
class ISchemaAttributeTypeBase : public ISchemaAttributeType
{
	friend class CSchemaAttributeStats;

public:
	ISchemaAttributeTypeBase()
	{
		CSchemaAttributeStats::RegisterAttributeType< ISchemaAttributeTypeBase<TAttribInMemoryType>, TAttribInMemoryType >();

		// The implementation of the attributes-in-memory system is such that it may or may not behave according to
		// expectations. Rather than have to stare at all the details to answer questions about where memory is allocated
		// or managed, or when it will be freed, for all our current use cases it makes more sense to just disable raw
		// pointer types from being an attribute-in-memory type and instead steer people towards this message explaining
		// why.
		COMPILE_TIME_ASSERT( !IsPointerType<TAttribInMemoryType>::kValue );
	}


	virtual void LoadEconAttributeValue( CEconItem *pTargetItem, const CEconItemAttributeDefinition *pAttrDef, const union attribute_data_union_t& value ) const OVERRIDE;

	// Returns a unique identifier per run based on the type of <TAttribInMemoryType>.
	virtual unsigned int GetTypeUniqueIdentifier() const OVERRIDE
	{
		return GetAttributeTypeUniqueIdentifier<TAttribInMemoryType>();
	}

	// Takes the value specified in [typedValue] and stores it in the most appropriate way
	// somewhere attached to [out_pValue]. This may hit the heap. The storage itself is
	// intended to be opaque but can be reversed by calling GetTypedValueContentsFromEconAttributeValue(). 
	void ConvertTypedValueToEconAttributeValue( const TAttribInMemoryType& typedValue, attribute_data_union_t *out_pValue ) const
	{
		// If our type is smaller than an int, we don't know how to copy the memory into our flat structure. We could write
		// this code but we have no use case for it now so this is set up to fail so if someone does come up with a use case
		// they know where to fix.
		COMPILE_TIME_ASSERT( sizeof( TAttribInMemoryType ) >= sizeof( uint32 ) );

		// Do we fit in the bottom 32-bits?
		if ( sizeof( TAttribInMemoryType ) <= sizeof( uint32 ) )
		{
			*reinterpret_cast<TAttribInMemoryType *>( &out_pValue->asUint32 ) = typedValue;
		}
		// What about in the full 64-bits (if we're running a 64-bit build)?
		else if ( sizeof( TAttribInMemoryType ) <= sizeof( void * ) )
		{
			*reinterpret_cast<TAttribInMemoryType *>( &out_pValue->asBlobPointer ) = typedValue;
		}
		// We're too big for our flat structure. We need to allocate space somewhere outside our attribute instance and point
		// to that.
		else
		{
			Assert( out_pValue->asBlobPointer );
			*reinterpret_cast<TAttribInMemoryType *>( out_pValue->asBlobPointer ) = typedValue;
		}
	}

	// Guaranteed to return a valid reference (or assert/crash if calling code is behaving inappropriately and calling
	// this before an attribute value is allocated/set).
	const TAttribInMemoryType& GetTypedValueContentsFromEconAttributeValue( const attribute_data_union_t& value ) const
	{
		COMPILE_TIME_ASSERT( sizeof( TAttribInMemoryType ) >= sizeof( uint32 ) );

		// Do we fit in the bottom 32-bits?
		if ( sizeof( TAttribInMemoryType ) <= sizeof( uint32 ) )
			return *reinterpret_cast<const TAttribInMemoryType *>( &value.asUint32 );
		
		// What about in the full 64-bits (if we're running a 64-bit build)?
		if ( sizeof( TAttribInMemoryType ) <= sizeof( void * ) )
			return *reinterpret_cast<const TAttribInMemoryType *>( &value.asBlobPointer );

		// We don't expect to get to a "read value" call without having written a value, which would
		// have allocated this memory.
		Assert( value.asBlobPointer );

		return *reinterpret_cast<const TAttribInMemoryType *>( value.asBlobPointer );
	}

	void ConvertEconAttributeValueToTypedValue( const attribute_data_union_t& value, TAttribInMemoryType *out_pTypedValue ) const
	{
		Assert( out_pTypedValue );

		*out_pTypedValue = GetTypedValueContentsFromEconAttributeValue( value );
	}

	void InitializeNewEconAttributeValue( attribute_data_union_t *out_pValue ) const OVERRIDE
	{
		if ( sizeof( TAttribInMemoryType ) <= sizeof( uint32 ) )
		{
			new( &out_pValue->asUint32 ) TAttribInMemoryType;
			s_InstanceStats.OnAllocateInlineInstance();
		}
		else if ( sizeof( TAttribInMemoryType ) <= sizeof( void * ) )
		{
			new( &out_pValue->asBlobPointer ) TAttribInMemoryType;
			s_InstanceStats.OnAllocateInlineInstance();
		}
		else
		{
			out_pValue->asBlobPointer = reinterpret_cast<byte *>( new TAttribInMemoryType );
			s_InstanceStats.OnAllocateHeapInstance();
		}
	}

	virtual void UnloadEconAttributeValue( attribute_data_union_t *out_pValue ) const OVERRIDE
	{
		COMPILE_TIME_ASSERT( sizeof( TAttribInMemoryType ) >= sizeof( uint32 ) );

		// For smaller types, anything that fits inside the bits of a void pointer, we store the contents
		// inline and only have to worry about calling the correct destructor. We check against the small-/
		// size/medium-size values separately to not worry about which bits we're storing the uint32 in.
		if ( sizeof( TAttribInMemoryType ) <= sizeof( uint32 ) )
		{
			(reinterpret_cast<TAttribInMemoryType *>( &out_pValue->asUint32 ))->~TAttribInMemoryType();
			s_InstanceStats.OnFreeInlineInstance();
		}
		else if ( sizeof( TAttribInMemoryType ) <= sizeof( void * ) )
		{
			(reinterpret_cast<TAttribInMemoryType *>( &out_pValue->asBlobPointer ))->~TAttribInMemoryType();
			s_InstanceStats.OnFreeInlineInstance();
		}
		// For larger types, we have the memory stored on the heap somewhere. We don't have to manually
		// destruct, but we do have to manually free.
		else
		{
			Assert( out_pValue->asBlobPointer );

			delete reinterpret_cast<TAttribInMemoryType *>( out_pValue->asBlobPointer );
			s_InstanceStats.OnFreeHeapInstance();
		}
	}

	virtual bool OnIterateAttributeValue( IEconItemAttributeIterator *pIterator, const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value ) const OVERRIDE
	{
		Assert( pIterator );
		Assert( pAttrDef );

		// Call the appropriate virtual function on our iterator based on whatever type we represent.
		return pIterator->OnIterateAttributeValue( pAttrDef, GetTypedValueContentsFromEconAttributeValue( value ) );
	}

	virtual void LoadByteStreamToEconAttributeValue( CEconItem *pTargetItem, const CEconItemAttributeDefinition *pAttrDef, const std::string& sBytes ) const OVERRIDE;
	virtual void ConvertEconAttributeValueToByteStream( const attribute_data_union_t& value, ::std::string *out_psBytes ) const;

	virtual void ConvertTypedValueToByteStream( const TAttribInMemoryType& typedValue, ::std::string *out_psBytes ) const = 0;
	virtual void ConvertByteStreamToTypedValue( const ::std::string& sBytes, TAttribInMemoryType *out_pTypedValue ) const = 0;

private:
	static schema_attribute_stat_bucket_t s_InstanceStats;
};

// This function exists only to back-convert code that relies on the old untyped
// attribute system, doing things like shoving floating-point bits into a uint32
// value in the database.
//
// There is no reason to use this function moving forward! If you're writing new
// code and calling this function seems like the only way to get the effect you
// want, it probably just means that there is no attribute type for what you're
// trying to do yet.
template < typename T >	uint32 WrapDeprecatedUntypedEconItemAttribute( T tValue ) { COMPILE_TIME_ASSERT( sizeof( T ) == sizeof( uint32 ) ); return *reinterpret_cast<uint32 *>( &tValue ); }

template < typename TAttribInMemoryType >
schema_attribute_stat_bucket_t ISchemaAttributeTypeBase<TAttribInMemoryType>::s_InstanceStats;

class CEconItem : public GCSDK::CSharedObject, public CMaterialOverrideContainer< IEconItemInterface >
{

public:
	typedef GCSDK::CSharedObject BaseClass;

	struct attribute_t
	{
		attrib_definition_index_t m_unDefinitionIndex;			// stored as ints here for memory efficiency on the GC
		attribute_data_union_t m_value;

	private:
		void operator=( const attribute_t& rhs );
	};

	struct EquippedInstance_t
	{
		EquippedInstance_t() : m_unEquippedClass( 0 ), m_unEquippedSlot( INVALID_EQUIPPED_SLOT ) {}
		EquippedInstance_t( equipped_class_t unClass, equipped_slot_t unSlot ) : m_unEquippedClass( unClass ), m_unEquippedSlot( unSlot ) {}
		equipped_class_t m_unEquippedClass;
		equipped_slot_t m_unEquippedSlot;
	};


	const static int k_nTypeID = k_EEconTypeItem;
	virtual int GetTypeID() const { return k_nTypeID; }

	CEconItem();
	CEconItem( const CEconItem& rhs );
	virtual ~CEconItem();

	CEconItem &operator=( const CEconItem& rhs );

	//called to determine if this item is tradable or not. This will return the time after which it can be traded. If 0 it can be traded. This is
	//needed since the base implementation of this is protected
	RTime32 GetTradableAfterDateTime() const		 { return IEconItemInterface::GetTradableAfterDateTime(); }

	//called to set a tradable after date/time value onto this item (this avoids a lot of potential inefficiencies around this process)
	void SetTradableAfterDateTime( RTime32 rtTime );

	// IEconItemInterface interface.
	const GameItemDefinition_t *GetItemDefinition() const;
public:

	virtual void IterateAttributes( class IEconItemAttributeIterator *pIterator ) const OVERRIDE;
	virtual itemid_t GetID() const { return GetItemID(); }
	
	// Accessors/Settors
	itemid_t GetItemID() const { return m_ulID; }
	void SetItemID( uint64 ulID );

	itemid_t GetOriginalID() const;
	void SetOriginalID( uint64 ulOriginalID );

	uint32 GetAccountID() const { return m_unAccountID; }
	void SetAccountID( uint32 unAccountID ) { m_unAccountID = unAccountID; }

	uint32 GetDefinitionIndex() const { return m_unDefIndex; }
	void SetDefinitionIndex( uint32 unDefinitionIndex ) { m_unDefIndex = unDefinitionIndex; }

	uint32 GetItemLevel() const { return m_unLevel; }
	void SetItemLevel( uint32 unItemLevel ) { m_unLevel = unItemLevel; }

	int32 GetQuality() const { return m_nQuality; }
	void SetQuality( int32 nQuality ) { m_nQuality = nQuality; }

	uint32 GetInventoryToken() const { return m_unInventory; }
	void SetInventoryToken( uint32 unToken ) { m_unInventory = unToken; }

	int GetQuantity() const;
	void SetQuantity( uint16 unQuantity );

	uint8 GetFlags() const { return m_unFlags; }
	void SetFlags( uint8 unFlags ) { m_unFlags = unFlags; }

	void SetFlag( uint8 unFlag ) { m_unFlags |= unFlag; }
	void ClearFlag( uint8 unFlag ) { m_unFlags &= ~unFlag; }
	bool CheckFlags( uint8 unFlags ) const { return ( m_unFlags & unFlags ) != 0; } 

	eEconItemOrigin GetOrigin() const { return (eEconItemOrigin)m_unOrigin; }
	void SetOrigin( eEconItemOrigin unOrigin ) { m_unOrigin = unOrigin; Assert( m_unOrigin == unOrigin ); }
	bool IsForeign() const { return m_unOrigin == kEconItemOrigin_Foreign; }

	style_index_t GetStyle() const;
	void SetStyle( uint8 unStyle ) { m_unStyle = unStyle; DirtyIconURL(); }

	const char *GetIconURLSmall() const;
	const char *GetIconURLLarge() const;

	const char *GetCustomName() const;
	void SetCustomName( const char *pName );

	const char *GetCustomDesc() const;
	void SetCustomDesc( const char *pDesc );

	bool IsEquipped() const;
	bool IsEquippedForClass( equipped_class_t unClass ) const;
	equipped_slot_t GetEquippedPositionForClass( equipped_class_t unClass ) const;

	void Equip( equipped_class_t unClass, equipped_slot_t unSlot );
	void Unequip();
	void UnequipFromClass( equipped_class_t unClass );
	
	// This should really only used for the WebAPIs, debugging, etc. Data manipulation during gameplay should use
	// the above functions.
	int GetEquippedInstanceCount() const;
	const EquippedInstance_t &GetEquippedInstance( int iIdx ) const;

	virtual bool GetInUse() const;
	void SetInUse( bool bInUse );

	bool IsTradable() const;
	bool IsMarketable() const;
	bool IsCommodity() const;

	void AdoptMoreRestrictedTradabilityFromItem( const CEconItem *pOther, uint32 nTradabilityFlagsToAccept = 0xFFFFFFFF );
	void AdoptMoreRestrictedTradability( uint32 nTradabilityFlags, RTime32 nUntradableTime );
	bool IsUsableInCrafting() const;


	// --------------------------------------------------------------------------------------------
	// Typed attributes. These are methods for accessing and setting values of attributes with
	// some semblance of type information and type safety.
	// --------------------------------------------------------------------------------------------

	// Assign the value of the attribute [pAttrDef] to [value]. Passing in a type for [value] that
	// doesn't match the storage type specified by the attribute definition will fail asserts a bunch
	// of asserts all the way down the stack and may or may not crash -- it would be nice to make this
	// fail asserts at compile time.
	//
	// This function has undefined results (besides asserting) if called to add a dynamic version of
	// an attrib that's already specified statically.
	template < typename T >
	void SetDynamicAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const T& value )
	{
		Assert( pAttrDef );

		const ISchemaAttributeTypeBase<T> *pAttrType = GetTypedAttributeType<T>( pAttrDef );
		// Game clients and servers may be running code that doesn't have all of the types for the new attributes
		// for a GC that just propped. Because we're not authoritative over items here, about the best we can do
		// here is abort entirely. This means that the client may not display certain attributes at all, or even
		// have them in the attribute list in memory, but we don't understand those attributes anyway.
		if ( !pAttrType )
			return;
		
		// Fail right off the bat if we're trying to write a dynamic attribute value for an item that already
		// has this as a static value.
		AssertMsg4( !::FindAttribute( GetItemDefinition(), pAttrDef ),
				    "Item id %llu (%s) attempting to set dynamic attribute value for '%s' (%d) when static attribute exists!",
				    GetItemID(), GetItemDefinition()->GetDefinitionName(), pAttrDef->GetDefinitionName(), pAttrDef->GetDefinitionIndex() );
		
		// Alright, we have a data type match so we can safely store data. Some types may need to initialize
		// their data to a current state if it's the first time we're writing to this value (as opposed to
		// updating an existing value).
		attribute_t *pEconAttrib = FindDynamicAttributeInternal( pAttrDef );

		if ( !pEconAttrib )
		{
			pEconAttrib = &(AddDynamicAttributeInternal());
			pEconAttrib->m_unDefinitionIndex = pAttrDef->GetDefinitionIndex();
			pAttrType->InitializeNewEconAttributeValue( &pEconAttrib->m_value );
		}

		pAttrType->ConvertTypedValueToEconAttributeValue( value, &pEconAttrib->m_value );

#if ENABLE_TYPED_ATTRIBUTE_PARANOIA
		// Paranoia!: make sure that our read/write functions are mirrored correctly, and that if we attempt
		// to read back a value we get something identical to what we just wrote. We do this via converting
		// to strings and then comparing those because there may or not be equality comparisons for our type
		// T that make sense (ie., protobufs).
		{
			T readValue;
			DbgVerify( FindAttribute( pAttrDef, &readValue ) );

			std::string sBytes, sReadBytes;
			pAttrType->ConvertTypedValueToByteStream( value, &sBytes );
			pAttrType->ConvertTypedValueToByteStream( readValue, &sReadBytes );
			AssertMsg1( sBytes == sReadBytes, "SetDynamicAttributeValue(): read/write mismatch for attribute '%s'.", pAttrDef->GetDefinitionName() );
		}
#endif // ENABLE_TYPED_ATTRIBUTE_PARANOIA
	}

	// Called to set a time stamp dynamic attribute on this item. But it will first check the current value assigned to this item, and will
	// only set it if this new time extends beyond the current one
	void SetDynamicMaxTimeAttributeValue( const CEconItemAttributeDefinition *pAttrDef, RTime32 rtTime );

	// Remove an instance of an attribute from this item. This will also free any dynamic memory associated
	// with that instance if any was allocated.
	void RemoveDynamicAttribute( const CEconItemAttributeDefinition *pAttrDef );

	// Copy all attributes and values in a type-safe way from [source] to ourself. Attributes that we have
	// that don't exist on [source] will maintain their current values. All other attributes will get their
	// values set to whatever [source] specifies.
	void CopyAttributesFrom( const CEconItem& source );

	bool BHasDynamicAttributes() const { return GetDynamicAttributeCountInternal() > 0; }

private:
	const char* FindIconURL( bool bLarge ) const;

	void Init();

	template < typename T >
	static const ISchemaAttributeTypeBase<T> *GetTypedAttributeType( const CEconItemAttributeDefinition *pAttrDef )
	{
		// Make sure the type of data we're passing in matches the type of data we're claiming that we can
		// store in the attribute definition.
		const ISchemaAttributeType *pIAttr = pAttrDef->GetAttributeType();
		Assert( pIAttr );
		Assert( pIAttr->GetTypeUniqueIdentifier() == GetAttributeTypeUniqueIdentifier<T>() );

#if ENABLE_TYPED_ATTRIBUTE_PARANOIA
		return dynamic_cast<const ISchemaAttributeTypeBase<T> *>( pIAttr );
#else
		return static_cast<const ISchemaAttributeTypeBase<T> *>( pIAttr );
#endif
	}

public:
	void Compact();



	// these are overridden to handle attributes
	virtual bool BParseFromMessage( const CUtlBuffer &buffer ) OVERRIDE;
	virtual bool BParseFromMessage( const std::string &buffer ) OVERRIDE;
	virtual bool BUpdateFromNetwork( const CSharedObject & objUpdate ) OVERRIDE;


	virtual bool BIsKeyLess( const CSharedObject & soRHS ) const ;
	virtual void Copy( const CSharedObject & soRHS );
	virtual void Dump() const;
	virtual CUtlString GetDebugString() const OVERRIDE;

	void SerializeToProtoBufItem( CSOEconItem &msgItem ) const;
	void DeserializeFromProtoBufItem( const CSOEconItem &msgItem );


	// Return the ID of the interior item, regardless of if it is loaded.
	itemid_t GetInteriorItemID();
	// Non-yielding -- will return current interior item if it exists and is already loaded
	// but will make no attempt to load.
	CEconItem* GetInteriorItem();
	const CEconItem* GetInteriorItem() const { return const_cast<CEconItem *>(this)->GetInteriorItem(); }

	const CEconItemCustomData* GetCustomData() const { return m_pCustomData; }

	void OnTraded( uint32 unTradabilityDelaySeconds );
	void OnReceivedFromMarket( bool bFromRollback );

protected:

	// Call this when the appearance of this item changes (ex. paintkit, style, festive). This will
	// cause the icon to be lazily re-evaluated (ie. so that changing the style will change the icon)
	void DirtyIconURL() { m_pszLargeIcon = NULL; m_pszSmallIcon = NULL; } 
	// CSharedObject
	// adapted from CSchemaSharedObject
	void GetDirtyColumnSet( const CUtlVector< int > &fields, GCSDK::CColumnSet &cs ) const;

	void EnsureCustomDataExists();
	
	bool BYieldingLoadInteriorItem();	

	void OnTransferredOwnership();

	// Internal attribute interface.
	friend class CWebAPIStringExporterAttributeIterator;
	friend class CAttributeToStringIterator;

	attribute_t&	   AddDynamicAttributeInternal();													// add another chunk of data to our internal storage to store a new attribute -- initialization is the responsibility of the caller
	attribute_t		  *FindDynamicAttributeInternal( const CEconItemAttributeDefinition *pAttrDef );	// search for an instance of a dynamic attribute with this definition -- ignores static properties, etc. and will return NULL if not found
	int				   GetDynamicAttributeCountInternal() const;										// how many attributes are there attached to this instance?
	attribute_t&	   GetMutableDynamicAttributeInternal( int iAttrIndexIntoArray );					// get a writable version of our attribute memory base chunk (added by AddDynamicAttributeInternal) for this index (same "array" as GetDynamicAttributeCountInternal)
	const attribute_t& GetDynamicAttributeInternal( int iAttrIndexIntoArray ) const						// read-only version of our attribute memory base chunk for this index (same "array" as GetDynamicAttributeCountInternal)
	{
		return const_cast<CEconItem *>( this )->GetMutableDynamicAttributeInternal( iAttrIndexIntoArray );
	}

	const EquippedInstance_t *FindEquippedInstanceForClass( equipped_class_t nClass ) const;
	void InternalVerifyEquipInstanceIntegrity() const;

	struct dirty_bits_t
	{
		// other
		uint8 m_bInUse : 1;
		uint8 m_bHasEquipSingleton : 1;
		uint8 m_bHasAttribSingleton: 1;
	};

	mutable const char* m_pszSmallIcon;
	mutable const char* m_pszLargeIcon;
public:
	// data that is most commonly changed
	uint64 m_ulID;							// Item ID
	uint32 m_unAccountID;					// Item Owner
	uint32 m_unInventory;					// App managed int representing inventory placement
	item_definition_index_t m_unDefIndex;	// Item definition index
	uint8 m_unLevel;						// Item Level
	uint8 m_nQuality;						// Item quality (rarity)
	uint8 m_unFlags;						// Flags
	uint8 m_unOrigin;						// Origin (eEconItemOrigin)
	style_index_t m_unStyle;				// Style

	dirty_bits_t m_dirtyBits;	// dirty bits

	// Fields that we often have zero or one of, but not often more
	EquippedInstance_t m_EquipInstanceSingleton; // Where the item is equipped. Valid only if m_bHasEquipSingleton and there is no custom data
	attribute_t m_CustomAttribSingleton;	// Custom attribute. Valid only if m_bHasAttribSingleton and there is no custom data

	// optional data (custom name, additional attributes, etc.)
	CEconItemCustomData *m_pCustomData;

};

//-----------------------------------------------------------------------------
// Purpose: Storage for data that is not commonly changed in CEconItem, primarily
// as a memory savings mechanism.
//-----------------------------------------------------------------------------
class CEconItemCustomData
{
public:
	CEconItemCustomData()
		: m_pInteriorItem( NULL )
		, m_ulOriginalID( INVALID_ITEM_ID )
		, m_unQuantity( 1 )
		, m_vecAttributes( /* grow size: */ 1, /* init size: */ 0 )
		, m_vecEquipped( /* grow size: */ 1, /* init size: */ 0 )
	{}

	~CEconItemCustomData();

	CUtlVector< CEconItem::attribute_t > m_vecAttributes;
	CEconItem* m_pInteriorItem;
	uint64 m_ulOriginalID;	// Original Item ID
	uint16 m_unQuantity;	// Consumable stack count (ammo, money, etc)	

	CUtlVector<CEconItem::EquippedInstance_t> m_vecEquipped;

	static void FreeAttributeMemory( CEconItem::attribute_t *pAttrib );

};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template < typename TAttribInMemoryType >
/*virtual*/ void ISchemaAttributeTypeBase<TAttribInMemoryType>::LoadByteStreamToEconAttributeValue( CEconItem *pTargetItem, const CEconItemAttributeDefinition *pAttrDef, const std::string& sBytes ) const
{
	Assert( pTargetItem );
	Assert( pAttrDef );

	TAttribInMemoryType typedValue;
	ConvertByteStreamToTypedValue( sBytes, &typedValue );

	pTargetItem->SetDynamicAttributeValue( pAttrDef, typedValue );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template < typename TAttribInMemoryType >
/*virtual*/ void ISchemaAttributeTypeBase<TAttribInMemoryType>::ConvertEconAttributeValueToByteStream( const attribute_data_union_t& value, ::std::string *out_psBytes ) const
{
	ConvertTypedValueToByteStream( GetTypedValueContentsFromEconAttributeValue( value ), out_psBytes );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template < typename TAttribInMemoryType >
/*virtual*/ void ISchemaAttributeTypeBase<TAttribInMemoryType>::LoadEconAttributeValue( CEconItem *pTargetItem, const CEconItemAttributeDefinition *pAttrDef, const union attribute_data_union_t& value ) const
{
	pTargetItem->SetDynamicAttributeValue( pAttrDef, GetTypedValueContentsFromEconAttributeValue( value ) );
}


void YieldingAddAuditRecord( GCSDK::CSQLAccess *sqlAccess, CEconItem *pItem, uint32 unOwnerID, EItemAction eAction, uint32 unData );
void YieldingAddAuditRecord( GCSDK::CSQLAccess *sqlAccess, uint64 ulItemID, uint32 unOwnerID, EItemAction eAction, uint32 unData );
bool YieldingAddItemToDatabase( CEconItem *pItem, const CSteamID & steamID, EItemAction eAction, uint32 unData );

//-----------------------------------------------------------------------------
// Purpose: wrap the idea of "get a loot list from this item"; some loot lists
//			are static definitions and some are temporary heap-allocated objects
//			and this means you don't care which you're dealing with until we
//			come up with a better interface
//-----------------------------------------------------------------------------
class CCrateLootListWrapper
{
public:
	CCrateLootListWrapper( const IEconItemInterface *pEconItem )
		: m_pLootList( NULL )
		, m_unAuditDetailData( 0 )
		, m_bIsDynamicallyAllocatedLootList( false )
	{
		Assert( pEconItem );

		if ( !BAttemptCrateSeriesInitialization( pEconItem )
		  && !BAttemptLootListStringInitialization( pEconItem )
		  && !BAttemptLineItemInitialization( pEconItem ) )
		{
			// We don't actually have anything to do here. We'll return NULL when someone asks for our
			// loot list and we're done.
		}
	}

	~CCrateLootListWrapper()
	{
		if ( m_bIsDynamicallyAllocatedLootList )
		{
			delete m_pLootList;
		}
	}

	const IEconLootList *GetEconLootList() const
	{
		return m_pLootList;
	}

	uint32 GetAuditDetailData() const
	{
		return m_unAuditDetailData;
	}
	
private:
	CCrateLootListWrapper( const CCrateLootListWrapper& );		// intentionally unimplemented
	void operator=( const CCrateLootListWrapper& );				// intentionally unimplemented

private:
	// Look for an attribute that specifies a crate series.
	MUST_CHECK_RETURN bool BAttemptCrateSeriesInitialization( const IEconItemInterface *pEconItem );

	// Look for an attribute that specifies a loot list by string name.
	MUST_CHECK_RETURN bool BAttemptLootListStringInitialization( const IEconItemInterface *pEconItem );

	// Look for a line-item-per-attribute list.
	MUST_CHECK_RETURN bool BAttemptLineItemInitialization( const IEconItemInterface *pEconItem );

private:
	const IEconLootList *m_pLootList;
	uint32 m_unAuditDetailData;
	bool m_bIsDynamicallyAllocatedLootList;
};

//-----------------------------------------------------------------------------
// Purpose: Maintains a handle to an CEconItem.  If the item gets deleted, this
//			handle will return NULL when dereferenced
//-----------------------------------------------------------------------------
class CEconItemHandle : GCSDK::ISharedObjectListener
{
public:
	CEconItemHandle()
		: m_pItem( NULL )
		, m_iItemID( INVALID_ITEM_ID )
	{}

	CEconItemHandle( CEconItem* pItem )
		: m_pItem( pItem )
	{
		SetItem( pItem );
	}

	virtual ~CEconItemHandle();

	void SetItem( CEconItem* pItem );

	operator CEconItem *( void ) const
	{
		return m_pItem;
	}

	CEconItem* operator->( void ) const
	{
		return m_pItem;
	}

	CEconItem* operator=( CEconItem* pRhs )
	{
		SetItem( pRhs );
		return m_pItem;
	}

	virtual void SODestroyed( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;

	virtual void SOCacheUnsubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;

	virtual void PreSOUpdate( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE{}
	virtual void PostSOUpdate( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE{}
	virtual void SOCacheSubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE{}

private:

	void UnsubscribeFromSOEvents();

	CEconItem* m_pItem;			// The item
	itemid_t m_iItemID;			// The stored itemID
	CSteamID m_OwnerSteamID;	// Steam ID of the item owner.  Used for registering/unregistering from SOCache
};

#endif // ECONITEM_H
