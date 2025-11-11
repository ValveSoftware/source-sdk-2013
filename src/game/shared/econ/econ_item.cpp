//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CEconItem, a shared object for econ items
//
//=============================================================================

#include "cbase.h"
#include "econ_item.h"
#include "econ_item_schema.h"
#include "rtime.h"
#include "gcsdk/enumutils.h"
#include "smartptr.h"


#if defined( TF_CLIENT_DLL ) || defined( TF_DLL )
#include "tf_gcmessages.h"
#endif

using namespace GCSDK;


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/*static*/ const schema_attribute_stat_bucket_t *CSchemaAttributeStats::m_pHead;

//-----------------------------------------------------------------------------
// Purpose: Utility function to convert datafile strings to ints.
//-----------------------------------------------------------------------------
int StringFieldToInt( const char *szValue, const char **pValueStrings, int iNumStrings, bool bDontAssert ) 
{
	if ( !szValue || !szValue[0] )
		return -1;

	for ( int i = 0; i < iNumStrings; i++ )
	{
		if ( !Q_stricmp(szValue, pValueStrings[i]) )
			return i;
	}

	if ( !bDontAssert )
	{
		Assert( !"Missing value in StringFieldToInt()!" );
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Utility function to convert datafile strings to ints.
//-----------------------------------------------------------------------------
int StringFieldToInt( const char *szValue, const CUtlVector<const char *>& vecValueStrings, bool bDontAssert )
{
	return StringFieldToInt( szValue, (const char **)&vecValueStrings[0], vecValueStrings.Count(), bDontAssert );
}

// --------------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------------
CEconItem::CEconItem()
	: BaseClass( )
	, m_pCustomData( NULL )
	, m_ulID( INVALID_ITEM_ID )
	, m_unStyle( 0 )
	, m_pszSmallIcon( NULL )
	, m_pszLargeIcon( NULL )
{
	Init();
}

CEconItem::CEconItem( const CEconItem& rhs )
	: BaseClass( )
	, m_pCustomData( NULL )
	, m_ulID( INVALID_ITEM_ID )
	, m_unStyle( 0 )
	, m_pszSmallIcon( NULL )
	, m_pszLargeIcon( NULL )
{
	Init();
	(*this) = rhs;
}

void CEconItem::Init()
{
	memset( &m_dirtyBits, 0, sizeof( m_dirtyBits ) );

}

// --------------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------------
CEconItem::~CEconItem()
{
	// Free up any memory we may have allocated for our singleton attribute. Any other attributes
	// will be cleaned up as part of freeing the custom data object itself.
	if ( m_dirtyBits.m_bHasAttribSingleton )
	{
		CEconItemCustomData::FreeAttributeMemory( &m_CustomAttribSingleton );
	}

	// Free up any custom data we may have allocated. This will catch any attributes not
	// in our singleton.
	if ( m_pCustomData )
	{
		delete m_pCustomData;
	}
}

// --------------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------------
CEconItemCustomData::~CEconItemCustomData()
{
	FOR_EACH_VEC( m_vecAttributes, i )
	{
		FreeAttributeMemory( &m_vecAttributes[i] );
	}

	if ( m_pInteriorItem )
	{
		delete m_pInteriorItem;
	}
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItem::CopyAttributesFrom( const CEconItem& source )
{
	// Copy attributes -- each new instance needs to be allocated and then copied into by somewhere
	// that knows what the actual type is. Rather than do anything type-specific here, we just have each
	// attribute serialize it's value to a bytestream and then deserialize it. This is as safe as we can
	// make it but sort of silly wasteful.
	for ( int i = 0; i < source.GetDynamicAttributeCountInternal(); i++ )
	{
		const attribute_t& attr = source.GetDynamicAttributeInternal( i );

		const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinition( attr.m_unDefinitionIndex );
		Assert( pAttrDef );

		const ISchemaAttributeType *pAttrType = pAttrDef->GetAttributeType();
		Assert( pAttrType );

		std::string sBytes;
		pAttrType->ConvertEconAttributeValueToByteStream( attr.m_value, &sBytes );
		pAttrType->LoadByteStreamToEconAttributeValue( this, pAttrDef, sBytes );
	}
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
CEconItem &CEconItem::operator=( const CEconItem& rhs )
{
	// We do destructive operations on our local object, including freeing attribute memory, as part of
	// the copy, so we force self-copies to be a no-op.
	if ( &rhs == this )
		return *this;

	m_ulID = rhs.m_ulID;
	SetOriginalID( rhs.GetOriginalID() );
	m_unAccountID = rhs.m_unAccountID;
	m_unDefIndex = rhs.m_unDefIndex;
	m_unInventory = rhs.m_unInventory;
	m_unFlags = rhs.m_unFlags;
	m_unOrigin = rhs.m_unOrigin;
	m_unStyle = rhs.m_unStyle;
	m_EquipInstanceSingleton = rhs.m_EquipInstanceSingleton;

	// If we have memory allocated for a single attribute we free it manually.
	if ( m_dirtyBits.m_bHasAttribSingleton )
	{
		CEconItemCustomData::FreeAttributeMemory( &m_CustomAttribSingleton );
	}

	// Copy over our dirty bits but manually reset our attribute singleton state -- if we did have one,
	// we just deleted it above (and might replace it below); if we didn't have one, this won't affect
	// anything. Either way, because we have no attribute memory allocated at this point, we need this
	// to be reflected in the dirty bits so that if we do copy attributes, we copy them into the correct
	// place (either the singleton or the custom data, to be allocated later).
	m_dirtyBits = rhs.m_dirtyBits;
	m_dirtyBits.m_bHasAttribSingleton = false;

	// Free any custom memory we've allocated. This will also remove any custom attributes.
	if ( rhs.m_pCustomData == NULL )
	{
		delete m_pCustomData;
		m_pCustomData = NULL;
	}
	else
	{
		// Check for and copy in the equip instances from CustomData
		EnsureCustomDataExists();	
		m_pCustomData->m_vecEquipped = rhs.m_pCustomData->m_vecEquipped;
	}

	CopyAttributesFrom( rhs );

	// Reset our material overrides, they'll be set again on demand as needed.
	ResetMaterialOverrides();

	return *this;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItem::SetItemID( itemid_t ulID ) 
{
	uint64 ulOldID = m_ulID;
	m_ulID = ulID;
	// only overwrite if we don't have an original id currently and we are a new item cloned off an old item
	if ( ulOldID != INVALID_ITEM_ID && ulOldID != ulID && ( m_pCustomData == NULL || m_pCustomData->m_ulOriginalID == INVALID_ITEM_ID ) && ulID != INVALID_ITEM_ID && ulOldID != INVALID_ITEM_ID )
	{
		SetOriginalID( ulOldID );
	}

	ResetMaterialOverrides();	
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
itemid_t CEconItem::GetOriginalID() const
{
	if ( m_pCustomData != NULL && m_pCustomData->m_ulOriginalID != INVALID_ITEM_ID )
		return m_pCustomData->m_ulOriginalID; 
	return m_ulID;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItem::SetOriginalID( itemid_t ulOriginalID )
{
	if ( ulOriginalID != m_ulID )
	{
		EnsureCustomDataExists();
		m_pCustomData->m_ulOriginalID = ulOriginalID;
	}
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
static const char *GetCustomNameOrAttributeDesc( const CEconItem *pItem, const CEconItemAttributeDefinition *pAttrDef )
{
	if ( !pAttrDef )
	{
		// If we didn't specify the attribute in the schema we can't possibly have an
		// answer. This isn't really an error in that case.
		return NULL;
	}

	const char *pszStrContents;
	if ( FindAttribute_UnsafeBitwiseCast<CAttribute_String>( pItem, pAttrDef, &pszStrContents ) )
		return pszStrContents;

	return NULL;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
static void SetCustomNameOrDescAttribute( CEconItem *pItem, const CEconItemAttributeDefinition *pAttrDef, const char *pszNewValue )
{
	Assert( pItem );

	if ( !pAttrDef )
	{
		// If we didn't specify the attribute in the schema, that's fine if we're setting
		// the empty name/description string, but it isn't fine if we're trying to set
		// actual content.
		AssertMsg( !pszNewValue, "Attempt to set non-empty value for custom name/desc with no attribute present." );
		return;
	}

	// Removing existing value?
	if ( !pszNewValue || !pszNewValue[0] )
	{
		pItem->RemoveDynamicAttribute( pAttrDef );
		return;
	}

	CAttribute_String attrStr;
	attrStr.set_value( pszNewValue );

	pItem->SetDynamicAttributeValue( pAttrDef, attrStr );
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
bool CEconItem::IsEquipped() const
{
	for ( int i = 0; i < GetEquippedInstanceCount(); i++ )
	{
		const EquippedInstance_t &curEquipInstance = GetEquippedInstance( i );
		Assert( curEquipInstance.m_unEquippedSlot != INVALID_EQUIPPED_SLOT );

		if ( GetItemSchema()->IsValidClass( curEquipInstance.m_unEquippedClass ) )
			return true;
	}

	return false;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
bool CEconItem::IsEquippedForClass( equipped_class_t unClass ) const
{
	return NULL != FindEquippedInstanceForClass( unClass );
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
equipped_slot_t CEconItem::GetEquippedPositionForClass( equipped_class_t unClass ) const
{
	const EquippedInstance_t *pInstance = FindEquippedInstanceForClass( unClass );
	if ( pInstance )
		return pInstance->m_unEquippedSlot;

	return INVALID_EQUIPPED_SLOT;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
const CEconItem::EquippedInstance_t *CEconItem::FindEquippedInstanceForClass( equipped_class_t nClass ) const
{
	for ( int i = 0; i < GetEquippedInstanceCount(); i++ )
	{
		const EquippedInstance_t &curEquipInstance = GetEquippedInstance( i );
		if ( curEquipInstance.m_unEquippedClass == nClass )
			return &curEquipInstance;
	}

	return NULL;
}



//----------------------------------------------------------------------------
// Purpose:
//----------------------------------------------------------------------------
void CEconItem::InternalVerifyEquipInstanceIntegrity() const
{
	if ( m_dirtyBits.m_bHasEquipSingleton )
	{
		Assert( !m_pCustomData );
		Assert( m_EquipInstanceSingleton.m_unEquippedSlot != INVALID_EQUIPPED_SLOT );
	}
	else if ( m_pCustomData )
	{
		FOR_EACH_VEC( m_pCustomData->m_vecEquipped, i )
		{
			Assert( m_pCustomData->m_vecEquipped[i].m_unEquippedSlot != INVALID_EQUIPPED_SLOT );

			for ( int j = i + 1; j < m_pCustomData->m_vecEquipped.Count(); j++ )
			{
				Assert( m_pCustomData->m_vecEquipped[i].m_unEquippedClass != m_pCustomData->m_vecEquipped[j].m_unEquippedClass );
			}
		}
	}
	else
	{
		Assert( GetEquippedInstanceCount() == 0 );
	}
}

//----------------------------------------------------------------------------
// Purpose:
//----------------------------------------------------------------------------
void CEconItem::Equip( equipped_class_t unClass, equipped_slot_t unSlot )
{
	Assert( GetItemSchema()->IsValidClass( unClass ) );
	Assert( GetItemSchema()->IsValidItemSlot( unSlot, unClass ) );

	// First, make sure we don't have this item already equipped for this class.
	UnequipFromClass( unClass );

	// If we have no instances of this item equipped, we want to shove this into the
	// first empty slot we can find. If we already have a custom data allocated, we
	// use that. If not, we want to use the singleton if we can. Otherwise, we make
	// a new custom data and fall back to using that.
	if ( m_pCustomData )
	{
		m_pCustomData->m_vecEquipped.AddToTail( EquippedInstance_t( unClass, unSlot ) );
	}
	else if ( !m_dirtyBits.m_bHasEquipSingleton )
	{
		m_EquipInstanceSingleton = EquippedInstance_t( unClass, unSlot );
		m_dirtyBits.m_bHasEquipSingleton = true;
	}
	else
	{
		EnsureCustomDataExists();
		m_pCustomData->m_vecEquipped.AddToTail( EquippedInstance_t( unClass, unSlot ) );
	}

	InternalVerifyEquipInstanceIntegrity();


}

//----------------------------------------------------------------------------
// Purpose:
//----------------------------------------------------------------------------
void CEconItem::Unequip()
{
	if ( m_dirtyBits.m_bHasEquipSingleton )
	{
		Assert( !m_pCustomData );
		m_dirtyBits.m_bHasEquipSingleton = false;
	}
	else if ( m_pCustomData )
	{
		m_pCustomData->m_vecEquipped.Purge();
	}

	InternalVerifyEquipInstanceIntegrity();
}

//----------------------------------------------------------------------------
// Purpose:
//----------------------------------------------------------------------------
void CEconItem::UnequipFromClass( equipped_class_t unClass )
{
	Assert( GetItemSchema()->IsValidClass( unClass ) );

	// If we only have a single equipped class...
	if ( m_dirtyBits.m_bHasEquipSingleton )
	{
		// ...and that's the class we're trying to remove from...
		if ( m_EquipInstanceSingleton.m_unEquippedClass == unClass )
		{
			// ...we now have no equipped classes!
			m_dirtyBits.m_bHasEquipSingleton = false;
		}
	}
	else if ( m_pCustomData )
	{
		// ...otherwise, if we have multiple equipped classes...
		FOR_EACH_VEC( m_pCustomData->m_vecEquipped, i )
		{
			// ...then look through our list to find out if we have this class...
			if ( m_pCustomData->m_vecEquipped[i].m_unEquippedClass == unClass )
			{
				// ...and if we do, remove it.
				m_pCustomData->m_vecEquipped.FastRemove( i );
				break;
			}
		}
	}

	InternalVerifyEquipInstanceIntegrity();
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
int CEconItem::GetEquippedInstanceCount() const
{
	if ( m_pCustomData )
		return m_pCustomData->m_vecEquipped.Count();
	else 
		return m_dirtyBits.m_bHasEquipSingleton ? 1 : 0;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
const CEconItem::EquippedInstance_t &CEconItem::GetEquippedInstance( int iIdx ) const
{
	Assert( iIdx >= 0  && iIdx < GetEquippedInstanceCount() );

	if ( m_pCustomData )
		return m_pCustomData->m_vecEquipped[iIdx];
	else
		return m_EquipInstanceSingleton;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
bool CEconItem::GetInUse() const
{
	return ( m_dirtyBits.m_bInUse ) != 0;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItem::SetInUse( bool bInUse )
{
	if ( bInUse )
	{
		m_dirtyBits.m_bInUse = 1;
	}
	else
	{
		m_dirtyBits.m_bInUse = 0;
	}
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
const GameItemDefinition_t *CEconItem::GetItemDefinition() const
{
	const CEconItemDefinition  *pRet	  = GetItemSchema()->GetItemDefinition( GetDefinitionIndex() );
	const GameItemDefinition_t *pTypedRet = dynamic_cast<const GameItemDefinition_t *>( pRet );

	AssertMsg( pRet == pTypedRet, "Item definition of inappropriate type." );

	return pTypedRet;
}

void CEconItem::IterateAttributes( IEconItemAttributeIterator *pIterator ) const
{
	Assert( pIterator );

	// custom attributes?
	for ( int i = 0; i < GetDynamicAttributeCountInternal(); i++ )
	{
		const attribute_t &attrib = GetDynamicAttributeInternal( i );
		const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinition( attrib.m_unDefinitionIndex );
		if ( !pAttrDef )
			continue;

		if ( !pAttrDef->GetAttributeType()->OnIterateAttributeValue( pIterator, pAttrDef, attrib.m_value ) )
			return;
	}

	// in static attributes?
	const CEconItemDefinition *pItemDef = GetItemDefinition();
	if ( !pItemDef )
		return;

	pItemDef->IterateAttributes( pIterator );
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
style_index_t CEconItem::GetStyle() const
{
	static CSchemaAttributeDefHandle pAttrDef_ItemStyleOverride( "item style override" );
	float fStyleOverride = 0.f;
	if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( this, pAttrDef_ItemStyleOverride, &fStyleOverride ) )
	{
		return fStyleOverride;
	}

	return m_unStyle;
}

const char* CEconItem::FindIconURL( bool bLarge ) const
{
	const char* pszSize = bLarge ? "l" : "s";

	static CSchemaAttributeDefHandle pAttrDef_IsFestivized( "is_festivized" );
	bool bIsFestivized = pAttrDef_IsFestivized ? FindAttribute( pAttrDef_IsFestivized ) : false;

	const CEconItemDefinition *pDef = GetItemDefinition();

	// Go through and figure out all the different decorations on
	// this item and construct the key to lookup the icon.
	// NOTE:  These are not currently composable, so they return out when
	//		  a match is found.  Once items are more composable, we'll want
	//		  to keep adding all the components together to get the fully
	//		  composed icon (ie. add the strange token, and the festive token, etc.)

	const CEconStyleInfo *pStyle = pDef->GetStyleInfo( GetStyle() );
	if ( pStyle )
	{
		const char* pszValue = pDef->GetIconURL( CFmtStr( "%ss%d", pszSize, GetStyle() ) );
		if ( pszValue )
			return pszValue;
	}

	if ( bIsFestivized )
	{
		const char* pszValue = pDef->GetIconURL( CFmtStr( "%sf", pszSize ) );
		if ( pszValue )
			return pszValue;
	}

	return pDef->GetIconURL( CFmtStr( "%s", pszSize ) );
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
const char *CEconItem::GetIconURLSmall() const
{
	if ( m_pszSmallIcon == NULL )
	{
		m_pszSmallIcon = FindIconURL( false );
	}

	return m_pszSmallIcon;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
const char *CEconItem::GetIconURLLarge() const
{
	if ( m_pszLargeIcon == NULL )
	{
		m_pszLargeIcon = FindIconURL( true );
	}

	return m_pszLargeIcon;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
int CEconItem::GetDynamicAttributeCountInternal() const
{
	if ( m_pCustomData )
		return m_pCustomData->m_vecAttributes.Count();
	else
		return m_dirtyBits.m_bHasAttribSingleton ? 1 : 0;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
CEconItem::attribute_t &CEconItem::GetMutableDynamicAttributeInternal( int iAttrIndexIntoArray )
{
	Assert( iAttrIndexIntoArray >= 0 );
	Assert( iAttrIndexIntoArray < GetDynamicAttributeCountInternal() );

	if ( m_pCustomData )
		return m_pCustomData->m_vecAttributes[ iAttrIndexIntoArray ];
	else
		return m_CustomAttribSingleton;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
CEconItem::attribute_t *CEconItem::FindDynamicAttributeInternal( const CEconItemAttributeDefinition *pAttrDef )
{
	Assert( pAttrDef );

	if ( m_pCustomData )
	{
		FOR_EACH_VEC( m_pCustomData->m_vecAttributes, i )
		{
			if ( m_pCustomData->m_vecAttributes[i].m_unDefinitionIndex == pAttrDef->GetDefinitionIndex() )
				return &m_pCustomData->m_vecAttributes[i];
		}
	}
	else if ( m_dirtyBits.m_bHasAttribSingleton )
	{
		if ( m_CustomAttribSingleton.m_unDefinitionIndex == pAttrDef->GetDefinitionIndex() )
			return &m_CustomAttribSingleton;
	}

	return NULL;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
CEconItem::attribute_t &CEconItem::AddDynamicAttributeInternal()
{
	if ( 0 == GetDynamicAttributeCountInternal() && NULL == m_pCustomData )
	{
		m_dirtyBits.m_bHasAttribSingleton = true;
		return m_CustomAttribSingleton;
	}
	else
	{
		EnsureCustomDataExists();
		return m_pCustomData->m_vecAttributes[ m_pCustomData->m_vecAttributes.AddToTail() ];
	}
}

// --------------------------------------------------------------------------
void CEconItem::SetDynamicMaxTimeAttributeValue( const CEconItemAttributeDefinition *pAttrDef, RTime32 rtTime )
{
	RTime32 rtExistingTime = 0;
	if ( FindAttribute( pAttrDef, &rtExistingTime ) )
	{
		//we have the attribute already, and see if the value exceeds what we are going to set
		if ( rtExistingTime >= rtTime )
			return;
	}

	//it doesn't so we need to update
	SetDynamicAttributeValue( pAttrDef, rtTime );
}

// --------------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------------
void CEconItem::RemoveDynamicAttribute( const CEconItemAttributeDefinition *pAttrDef )
{
	Assert( pAttrDef );
	Assert( pAttrDef->GetDefinitionIndex() != INVALID_ATTRIB_DEF_INDEX );

	if ( m_pCustomData )
	{
		for ( int i = 0; i < m_pCustomData->m_vecAttributes.Count(); i++ )
		{
			if ( m_pCustomData->m_vecAttributes[i].m_unDefinitionIndex == pAttrDef->GetDefinitionIndex() )
			{
				CEconItemCustomData::FreeAttributeMemory( &m_pCustomData->m_vecAttributes[i] );
				m_pCustomData->m_vecAttributes.FastRemove( i );
				return;
			}
		}
	}
	else if ( m_dirtyBits.m_bHasAttribSingleton )
	{
		if ( m_CustomAttribSingleton.m_unDefinitionIndex == pAttrDef->GetDefinitionIndex() )
		{
			CEconItemCustomData::FreeAttributeMemory( &m_CustomAttribSingleton );
			m_dirtyBits.m_bHasAttribSingleton = false;
		}
	}
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
/*static*/ void CEconItemCustomData::FreeAttributeMemory( CEconItem::attribute_t *pAttrib )
{
	Assert( pAttrib );

	const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinition( pAttrib->m_unDefinitionIndex );
	Assert( pAttrDef );

	const ISchemaAttributeType *pAttrType = pAttrDef->GetAttributeType();
	Assert( pAttrType );

	pAttrType->UnloadEconAttributeValue( &pAttrib->m_value );
}

// --------------------------------------------------------------------------
// Purpose: Frees any unused memory in the internal structures
// --------------------------------------------------------------------------
void CEconItem::Compact()
{
	if ( m_pCustomData )
	{
		m_pCustomData->m_vecAttributes.Compact();
		m_pCustomData->m_vecEquipped.Compact();
	}
}


CEconItem* CEconItem::GetInteriorItem()
{
	return m_pCustomData ? m_pCustomData->m_pInteriorItem : NULL;
}

// --------------------------------------------------------------------------
// Purpose: Ownership of this item has changed, so do whatever things are necessary
// --------------------------------------------------------------------------
void CEconItem::OnTransferredOwnership()
{
	// Free accounts have the ability to trade any item out that they received in a trade.
	SetFlag( kEconItemFlag_CanBeTradedByFreeAccounts );
}

// --------------------------------------------------------------------------
// Purpose: Parses the bits required to create a econ item from the message. 
//			Overloaded to include support for attributes.
// --------------------------------------------------------------------------
bool CEconItem::BParseFromMessage( const CUtlBuffer & buffer ) 
{
	CSOEconItem msgItem;
	if( !msgItem.ParseFromArray( buffer.Base(), buffer.TellMaxPut() ) )
		return false;

	DeserializeFromProtoBufItem( msgItem );
	return true;
}

// --------------------------------------------------------------------------
// Purpose: Parses the bits required to create a econ item from the message. 
//			Overloaded to include support for attributes.
// --------------------------------------------------------------------------
bool CEconItem::BParseFromMessage( const std::string &buffer ) 
{
	CSOEconItem msgItem;
	if( !msgItem.ParseFromString( buffer ) )
		return false;

	DeserializeFromProtoBufItem( msgItem );
	return true;
}

//----------------------------------------------------------------------------
// Purpose: Overrides all the fields in msgLocal that are present in the 
//			network message
//----------------------------------------------------------------------------
bool CEconItem::BUpdateFromNetwork( const CSharedObject & objUpdate )
{
	const CEconItem & econObjUpdate = (const CEconItem &)objUpdate;

	*this = econObjUpdate;

	return true;
}


//----------------------------------------------------------------------------
// Purpose: Returns true if this is less than than the object in soRHS. This
//			comparison is deterministic, but it may not be pleasing to a user
//			since it is just going to compare raw memory. If you need a sort 
//			that is user-visible you will need to do it at a higher level that
//			actually knows what the data in these objects means.
//----------------------------------------------------------------------------
bool CEconItem::BIsKeyLess( const CSharedObject & soRHS ) const
{
	Assert( GetTypeID() == soRHS.GetTypeID() );
	const CEconItem & soSchemaRHS = (const CEconItem &)soRHS;

	return m_ulID < soSchemaRHS.m_ulID;
}

//----------------------------------------------------------------------------
// Purpose: Copy the data from the specified schema shared object into this. 
//			Both objects must be of the same type.
//----------------------------------------------------------------------------
void CEconItem::Copy( const CSharedObject & soRHS )
{
	*this = (const CEconItem &)soRHS;
}

//----------------------------------------------------------------------------
// Purpose: Dumps diagnostic information about the shared object
//----------------------------------------------------------------------------
void CEconItem::Dump() const
{
	CSOEconItem msgItem;
	SerializeToProtoBufItem( msgItem );
	CProtoBufSharedObjectBase::Dump( msgItem );
}


//----------------------------------------------------------------------------
// Purpose: Return short, identifying string about the object
//----------------------------------------------------------------------------
CUtlString CEconItem::GetDebugString() const
{
	CUtlString result;
	result.Format( "[CEconItem: ID=%llu, DefIdx=%d]", GetItemID(), GetDefinitionIndex() );
	return result;
}




// --------------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------------
void CEconItem::SerializeToProtoBufItem( CSOEconItem &msgItem ) const
{
	VPROF_BUDGET( "CEconItem::SerializeToProtoBufItem()", VPROF_BUDGETGROUP_STEAM );

	msgItem.set_id( m_ulID );
	if( m_ulID != GetOriginalID() )
		msgItem.set_original_id( GetOriginalID() );
	msgItem.set_account_id( m_unAccountID );
	msgItem.set_def_index( m_unDefIndex );
	msgItem.set_inventory( m_unInventory );	
	msgItem.set_flags( m_unFlags );
	msgItem.set_origin( m_unOrigin );
	msgItem.set_style( m_unStyle );
	msgItem.set_in_use( m_dirtyBits.m_bInUse );

	for( int nAttr = 0; nAttr < GetDynamicAttributeCountInternal(); nAttr++ )
	{
		const attribute_t & attr = GetDynamicAttributeInternal( nAttr );
		
		// skip over attributes we don't understand
		const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinition( attr.m_unDefinitionIndex );
		if ( !pAttrDef )
			continue;

		const ISchemaAttributeType *pAttrType = pAttrDef->GetAttributeType();
		Assert( pAttrType );

		CSOEconItemAttribute *pMsgAttr = msgItem.add_attribute();
		pMsgAttr->set_def_index( attr.m_unDefinitionIndex );

		std::string sBytes;
		pAttrType->ConvertEconAttributeValueToByteStream( attr.m_value, &sBytes );
		pMsgAttr->set_value_bytes( sBytes );
	}

	msgItem.set_contains_equipped_state_v2( true );
	for ( int i = 0; i < GetEquippedInstanceCount(); i++ )
	{
		const EquippedInstance_t &instance = GetEquippedInstance( i );
		CSOEconItemEquipped *pMsgEquipped = msgItem.add_equipped_state();
		pMsgEquipped->set_new_class( instance.m_unEquippedClass );
		pMsgEquipped->set_new_slot( instance.m_unEquippedSlot );
	}

	if ( m_pCustomData )
	{
		const CEconItem *pInteriorItem = GetInteriorItem();
		if ( pInteriorItem )
		{
			CSOEconItem *pMsgInteriorItem = msgItem.mutable_interior_item();
			pInteriorItem->SerializeToProtoBufItem( *pMsgInteriorItem );
		}
	}
}

// --------------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------------
void CEconItem::DeserializeFromProtoBufItem( const CSOEconItem &msgItem )
{
	VPROF_BUDGET( "CEconItem::DeserializeFromProtoBufItem()", VPROF_BUDGETGROUP_STEAM );

	// Start by resetting
	SAFE_DELETE( m_pCustomData );
	m_dirtyBits.m_bHasAttribSingleton = false;
	m_dirtyBits.m_bHasEquipSingleton = false;

	// Now copy from the message
	m_ulID = msgItem.id();
	SetOriginalID( msgItem.has_original_id() ? msgItem.original_id() : m_ulID );
	m_unAccountID = msgItem.account_id();
	m_unDefIndex = msgItem.def_index();
	m_unInventory = msgItem.inventory();
	m_unFlags = msgItem.flags();
	m_unOrigin = msgItem.origin();
	m_unStyle = msgItem.style();

	m_dirtyBits.m_bInUse = msgItem.in_use() ? 1 : 0;

	// read the attributes
	for( int nAttr = 0; nAttr < msgItem.attribute_size(); nAttr++ )
	{
		// skip over old-format messages
		const CSOEconItemAttribute& msgAttr = msgItem.attribute( nAttr );
		if ( msgAttr.has_value() || !msgAttr.has_value_bytes() )
			continue;

		// skip over attributes we don't understand
		const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinition( msgAttr.def_index() );
		if ( !pAttrDef )
			continue;

		const ISchemaAttributeType *pAttrType = pAttrDef->GetAttributeType();
		Assert( pAttrType );

		pAttrType->LoadByteStreamToEconAttributeValue( this, pAttrDef, msgAttr.value_bytes() );
	}

	// Check to see if the item has an interior object.
	if ( msgItem.has_interior_item() )
	{
		EnsureCustomDataExists();

		m_pCustomData->m_pInteriorItem = new CEconItem();
		m_pCustomData->m_pInteriorItem->DeserializeFromProtoBufItem( msgItem.interior_item() );
	}

	// update equipped state
	if ( msgItem.has_contains_equipped_state_v2() && msgItem.contains_equipped_state_v2() )
	{
		// unequip from everything...
		Unequip();

		// ...and re-equip to whatever our current state is
		for ( int i = 0; i < msgItem.equipped_state_size(); i++ )
		{
			Equip( msgItem.equipped_state(i).new_class(), msgItem.equipped_state(i).new_slot() );
		}
	}
}


// --------------------------------------------------------------------------
// Purpose: 
// --------------------------------------------------------------------------
void CEconItem::EnsureCustomDataExists()
{
	if ( m_pCustomData == NULL )
	{
		m_pCustomData = new CEconItemCustomData();
		
		if ( m_dirtyBits.m_bHasEquipSingleton )
		{
			m_pCustomData->m_vecEquipped.AddToTail( m_EquipInstanceSingleton );
			m_EquipInstanceSingleton = EquippedInstance_t();
			m_dirtyBits.m_bHasEquipSingleton = false;
		}
		if ( m_dirtyBits.m_bHasAttribSingleton )
		{
			m_pCustomData->m_vecAttributes.AddToTail( m_CustomAttribSingleton );
			m_dirtyBits.m_bHasAttribSingleton = false;
		}
	}
}

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "tf_gamerules.h"
#include <tf_weapon_grapplinghook.h>
#include <tf_item_powerup_bottle.h>

// This is the command the user will execute.
// We want this to happen on the client, before forwarding to the game server,
// since we don't trust the game server.
static void StartUseActionSlotItem( const CCommand &args )
{
	if ( !engine->IsInGame() )
	{
		return;
	}

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer == NULL )
	{
		return;
	}

	pLocalPlayer->SetUsingActionSlot( true );

	// Ghosts cant use action items!
	if ( pLocalPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
	{
		return;
	}

	// If we're in Mann Vs MAchine, and we're dead, we can use this to respawn
	// instantly.
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && pLocalPlayer->IsObserver() )
	{
		float flNextRespawn = TFGameRules()->GetNextRespawnWave(
		pLocalPlayer->GetTeamNumber(), pLocalPlayer );
		if ( flNextRespawn )
		{
			int iRespawnWait = ( flNextRespawn - gpGlobals->curtime );
			if ( iRespawnWait > 1.0 )
			{
				engine->ClientCmd_Unrestricted( "td_buyback\n" );
				return;
			}
		}
	}

	// trying to pick up a dropped weapon?
	if ( pLocalPlayer->GetDroppedWeaponInRange() != NULL )
	{
		KeyValues *kv = new KeyValues( "+use_action_slot_item_server" );
		engine->ServerCmdKeyValues( kv );
		return;
	}

	if ( TFGameRules() && TFGameRules()->IsUsingGrapplingHook() )
	{
		CTFGrapplingHook *pGrapplingHook = dynamic_cast<CTFGrapplingHook *>(
		pLocalPlayer->GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
		if ( pGrapplingHook )
		{
			if ( pLocalPlayer->GetActiveTFWeapon() != pGrapplingHook )
			{
				pLocalPlayer->Weapon_Switch( pGrapplingHook );
			}

			KeyValues *kv = new KeyValues( "+use_action_slot_item_server" );
			engine->ServerCmdKeyValues( kv );

			return;
		}
	}

	// otherwise, forward to game server
	KeyValues *kv = new KeyValues( "+use_action_slot_item_server" );
	engine->ServerCmdKeyValues( kv );
}

static ConCommand
start_use_action_slot_item( "+use_action_slot_item", StartUseActionSlotItem, "Use the item in the action slot." );

static void EndUseActionSlotItem( const CCommand &args )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	pLocalPlayer->SetUsingActionSlot( false );

	if ( TFGameRules() && TFGameRules()->IsUsingGrapplingHook() && pLocalPlayer->GetActiveTFWeapon() )
	{
		// if we're using the hook, switch back to the last weapon
		if ( pLocalPlayer->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_GRAPPLINGHOOK )
		{
			KeyValues *kv = new KeyValues( "-use_action_slot_item_server" );
			engine->ServerCmdKeyValues( kv );

			C_BaseCombatWeapon *pLastWeapon = pLocalPlayer->GetLastWeapon();

			// switch away from the hook
			if ( pLastWeapon && pLocalPlayer->Weapon_CanSwitchTo( pLastWeapon ) )
			{
				pLocalPlayer->Weapon_Switch( pLastWeapon );
			}
			else
			{
				// in case we failed to switch back to last weapon for some
				// reason, just find the next best
				pLocalPlayer->SwitchToNextBestWeapon( pLastWeapon );
			}

			return;
		}
	}

	// tell the game server we let go of the button if this wasn't a GC item
	KeyValues *kv = new KeyValues( "-use_action_slot_item_server" );
	engine->ServerCmdKeyValues( kv );
}

static ConCommand end_use_action_slot_item( "-use_action_slot_item", EndUseActionSlotItem );

static void StartContextAction( const CCommand &args )
{
	// Assume we're going to taunt
	bool bDoTaunt = true;

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			CTFPowerupBottle *pPowerupBottle = dynamic_cast<CTFPowerupBottle *>( pLocalPlayer->GetEquippedWearableForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
			if ( pPowerupBottle && pPowerupBottle->GetNumCharges() > 0 )
			{
				// They're in MvM and have a bottle with a charge, so do an
				// action instead
				bDoTaunt = false;
			}

			if ( pLocalPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) &&
				 pLocalPlayer->GetActiveTFWeapon() &&
				 pLocalPlayer->GetActiveTFWeapon()->GetWeaponID() ==
				 TF_WEAPON_MINIGUN )
			{
				int iRage = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pLocalPlayer, iRage,
											   generate_rage_on_dmg );
				if ( iRage )
				{
					if ( pLocalPlayer->m_Shared.GetRageMeter() >= 100.f &&
						 !pLocalPlayer->m_Shared.IsRageDraining() )
					{
						// They have rage ready to go, do the taunt
						bDoTaunt = true;
					}
				}
			}
		}
	}

	if ( bDoTaunt )
	{
		// Taunt
		engine->ClientCmd_Unrestricted( "+taunt\n" );
	}
	else
	{
		// Action item
		StartUseActionSlotItem( args );
	}
}

static ConCommand start_context_action( "+context_action", StartContextAction, "Use the item in the action slot." );

static void EndContextAction( const CCommand &args )
{
	// Undo both to be on the safe side
	EndUseActionSlotItem( args );
	engine->ClientCmd_Unrestricted( "-taunt\n" );
}

static ConCommand end_context_action( "-context_action", EndContextAction );
#endif