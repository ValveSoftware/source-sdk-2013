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

extern int EconWear_ToIntCategory( float flWear );
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
	m_unLevel = rhs.m_unLevel;
	m_nQuality = rhs.m_nQuality;
	m_unInventory = rhs.m_unInventory;
	SetQuantity( rhs.GetQuantity() );
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
int CEconItem::GetQuantity() const
{
	if ( m_pCustomData != NULL )
		return m_pCustomData->m_unQuantity;
	return 1;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItem::SetQuantity( uint16 unQuantity )
{
	if ( m_pCustomData )
	{
		m_pCustomData->m_unQuantity = unQuantity;
	}
	else if ( unQuantity > 1 )
	{
		EnsureCustomDataExists();
		m_pCustomData->m_unQuantity = unQuantity;
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
const char *CEconItem::GetCustomName() const
{
	static CSchemaAttributeDefHandle pAttrDef_CustomName( "custom name attr" );

	return GetCustomNameOrAttributeDesc( this, pAttrDef_CustomName );
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItem::SetCustomName( const char *pName )
{
	static CSchemaAttributeDefHandle pAttrDef_CustomName( "custom name attr" );

	SetCustomNameOrDescAttribute( this, pAttrDef_CustomName, pName );
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
const char *CEconItem::GetCustomDesc() const
{
	static CSchemaAttributeDefHandle pAttrDef_CustomDesc( "custom desc attr" );

	return GetCustomNameOrAttributeDesc( this, pAttrDef_CustomDesc );
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItem::SetCustomDesc( const char *pDesc )
{
	static CSchemaAttributeDefHandle pAttrDef_CustomDesc( "custom desc attr" );

	SetCustomNameOrDescAttribute( this, pAttrDef_CustomDesc, pDesc );
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

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
bool CEconItem::IsTradable() const
{
	return !m_dirtyBits.m_bInUse 
		&& IEconItemInterface::IsTradable();
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
void CEconItem::AdoptMoreRestrictedTradabilityFromItem( const CEconItem *pOther, uint32 nTradabilityFlagsToAccept /*= 0xFFFFFFFF*/ )
{
	if ( !pOther )
		return;

	int nOtherUntradability = pOther->GetUntradabilityFlags() & nTradabilityFlagsToAccept;
	RTime32 otherUntradableTime = pOther->GetTradableAfterDateTime();
	// Become untradable if the other item is untradable
	AdoptMoreRestrictedTradability( nOtherUntradability, otherUntradableTime );
}

// --------------------------------------------------------------------------
// Purpose:	Given untradability flags and a untradable time, set this item's
//			untradability.  This does not clear existing untradabilty.
// --------------------------------------------------------------------------
void CEconItem::AdoptMoreRestrictedTradability( uint32 nTradabilityFlags, RTime32 nUntradableTime )
{
	static CSchemaAttributeDefHandle pAttrib_CannotTrade( "cannot trade" );
	static CSchemaAttributeDefHandle pAttrib_TradableAfter( "tradable after date" );

	if ( !pAttrib_CannotTrade || !pAttrib_TradableAfter )
		return;

	// We're already permanently untradable.  We can't get more untradable, so we're done.
	if ( GetUntradabilityFlags() & k_Untradability_Permanent )
		return;

	if( nTradabilityFlags & k_Untradability_Permanent )
	{
		SetDynamicAttributeValue( pAttrib_CannotTrade, 0u );
	}
	else if ( nTradabilityFlags & k_Untradability_Temporary && nUntradableTime > GetTradableAfterDateTime() )
	{
		// Take the "tradable after date" if it's larger than ours
		SetDynamicAttributeValue( pAttrib_TradableAfter, nUntradableTime );
	}
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
bool CEconItem::IsMarketable() const
{
	return !m_dirtyBits.m_bInUse
		&& IEconItemInterface::IsMarketable();
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
bool CEconItem::IsCommodity() const
{
	return !m_dirtyBits.m_bInUse
		&& IEconItemInterface::IsCommodity();
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

	static CSchemaAttributeDefHandle pAttrDef_ItemStyleStrange( "style changes on strange level" );
	uint32 iMaxStyle = 0;
	if ( pAttrDef_ItemStyleStrange && FindAttribute( pAttrDef_ItemStyleStrange, &iMaxStyle ) )
	{
		// Use the strange prefix if the weapon has one.
		uint32 unScore = 0;
		if ( !FindAttribute( GetKillEaterAttr_Score( 0 ), &unScore ) )
			return 0;

		// What type of event are we tracking and how does it describe itself?
		uint32 unKillEaterEventType = 0;
		// This will overwrite our default 0 value if we have a value set but leave it if not.
		float fKillEaterEventType;
		if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( this, GetKillEaterAttr_Type( 0 ), &fKillEaterEventType ) )
		{
			unKillEaterEventType = fKillEaterEventType;
		}

		const char *pszLevelingDataName = GetItemSchema()->GetKillEaterScoreTypeLevelingDataName( unKillEaterEventType );
		if ( !pszLevelingDataName )
		{
			pszLevelingDataName = KILL_EATER_RANK_LEVEL_BLOCK_NAME;
		}

		const CItemLevelingDefinition *pLevelDef = GetItemSchema()->GetItemLevelForScore( pszLevelingDataName, unScore );
		if ( !pLevelDef )
			return 0;

		return Min( pLevelDef->GetLevel(), iMaxStyle );
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
	uint32 unPaintKitDefIndex;
	if ( GetPaintKitDefIndex( this, &unPaintKitDefIndex ) )
	{
		float flWear = 0;
		GetPaintKitWear( this, flWear );
		int iWearIndex = EconWear_ToIntCategory( flWear );
		const char* pszFmtStr = bIsFestivized ? "paintkit%d_item%d_wear%d_festive" : "paintkit%d_item%d_wear%d";

		// do we have a remap? use that instead
		if ( pDef->GetDefinitionIndex() != pDef->GetRemappedItemDefIndex() )
		{
			pDef = GetItemSchema()->GetItemDefinition( pDef->GetRemappedItemDefIndex() );
		}

		const char* pszValue = pDef->GetIconURL( CFmtStr( pszFmtStr, unPaintKitDefIndex, pDef->GetRemappedItemDefIndex(), iWearIndex ) );
		if ( pszValue )
			return pszValue;
	}

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
bool CEconItem::IsUsableInCrafting() const
{
	return !m_dirtyBits.m_bInUse
		&& IEconItemInterface::IsUsableInCrafting();
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
void CEconItem::SetTradableAfterDateTime( RTime32 rtTime )
{
	//don't bother if the time is in the past (this also covers the 0 case)
	if( rtTime < CRTime::RTime32TimeCur() )
		return;

	//the attribute we are going to assign
	static CSchemaAttributeDefHandle pAttrib_TradableAfter( "tradable after date" );
	if( !pAttrib_TradableAfter )
		return;

	//see if we have a STATIC cannot trade attribute (ignore dynamic, because that could change and be used
	// to short out the trade restriction). 

	//This is currently disabled so we can measure whether or not this is beneficial and if the savings justifies the corner case risk this exposes - JohnO 1/12/15
	/*
	const GameItemDefinition_t* pItemDef = GetItemDefinition();
	if( pItemDef )
	{
		static CSchemaAttributeDefHandle pAttrib_CannotTrade( "cannot trade" );
		uint32 unCannotTrade = 0;
		if( ::FindAttribute( pItemDef, pAttrib_CannotTrade, &unCannotTrade ) )
		{
			return;
		}
	}
	*/

	//now set it to the maximum time
	SetDynamicMaxTimeAttributeValue( pAttrib_TradableAfter, rtTime );
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
// Purpose: This item has been traded. Give it an opportunity to update any internal
//			properties in response.
// --------------------------------------------------------------------------
void CEconItem::OnTraded( uint32 unTradabilityDelaySeconds )
{
	// if Steam wants us to impose a tradability delay on the item
	if ( unTradabilityDelaySeconds != 0 )
	{
		RTime32 rtTradableAfter = ( ( CRTime::RTime32TimeCur() / k_nSecondsPerDay ) * k_nSecondsPerDay ) + unTradabilityDelaySeconds;
		SetTradableAfterDateTime( rtTradableAfter );
	}
	else
	{
		// If we have a "tradable after date" attribute and we were just traded, remove the date
		// limit as we're obviously past it.
		static CSchemaAttributeDefHandle pAttrib_TradableAfter( "tradable after date" );
		RemoveDynamicAttribute( pAttrib_TradableAfter );
	}

	OnTransferredOwnership();
}

// --------------------------------------------------------------------------
// Purpose: Ownership of this item has changed, so do whatever things are necessary
// --------------------------------------------------------------------------
void CEconItem::OnTransferredOwnership()
{
	// Reset all our strange scores.
	for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
	{
		const CEconItemAttributeDefinition *pAttrDef = GetKillEaterAttr_Score(i);

		// Skip over any attributes our schema doesn't understand. We ideally wouldn't ever
		// have this happen but if it does we don't want to ignore other valid attributes.
		if ( !pAttrDef )
			continue;

		// Ignore any attributes we don't have on this item.
		if ( !FindAttribute( pAttrDef ) )
			continue;

		// Zero out the value of this stat attribute.
		SetDynamicAttributeValue( pAttrDef, 0u );
	}

	// Free accounts have the ability to trade any item out that they received in a trade.
	SetFlag( kEconItemFlag_CanBeTradedByFreeAccounts );
}

// --------------------------------------------------------------------------
// Purpose: This item has been traded. Give it an opportunity to update any internal
//			properties in response.
// --------------------------------------------------------------------------
void CEconItem::OnReceivedFromMarket( bool bFromRollback )
{
	OnTransferredOwnership();

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
	msgItem.set_level( m_unLevel );
	msgItem.set_quality( m_nQuality );
	msgItem.set_inventory( m_unInventory );	
	msgItem.set_quantity( GetQuantity() );
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
		const char *pszCustomName = GetCustomName();
		if ( pszCustomName )
		{
			msgItem.set_custom_name( pszCustomName );
		}

		const char *pszCustomDesc = GetCustomDesc();
		if ( pszCustomDesc )
		{
			msgItem.set_custom_desc( pszCustomDesc );
		}

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
	m_unLevel = msgItem.level();
	m_nQuality = msgItem.quality();
	m_unInventory = msgItem.inventory();
	SetQuantity( msgItem.quantity() );
	m_unFlags = msgItem.flags();
	m_unOrigin = msgItem.origin();
	m_unStyle = msgItem.style();

	m_dirtyBits.m_bInUse = msgItem.in_use() ? 1 : 0;

	// set name if any
	if( msgItem.has_custom_name() )
	{
		SetCustomName( msgItem.custom_name().c_str() );
	}

	// set desc if any
	if( msgItem.has_custom_desc() )
	{
		SetCustomDesc( msgItem.custom_desc().c_str() );
	}

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


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCrateLootListWrapper::BAttemptCrateSeriesInitialization( const IEconItemInterface *pEconItem )
{
	Assert( m_pLootList == NULL );

	// Find out what series this crate belongs to.
	static CSchemaAttributeDefHandle pAttr_CrateSeries( "set supply crate series" );
	if ( !pAttr_CrateSeries )
		return false;

	int iCrateSeries;
	{
		float fCrateSeries;		// crate series ID is stored as a float internally because we hate ourselves
		if ( !FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pEconItem, pAttr_CrateSeries, &fCrateSeries ) || fCrateSeries == 0.0f )
			return false;

		iCrateSeries = fCrateSeries;
	}

	// Our index is an index into the revolving-loot-lists list. From that list we'll be able to
	// get a loot list name, which we'll use to look up the actual contents.
	const CEconItemSchema::RevolvingLootListDefinitionMap_t& mapRevolvingLootLists = GetItemSchema()->GetRevolvingLootLists();
	int idx = mapRevolvingLootLists.Find( iCrateSeries );
	if ( !mapRevolvingLootLists.IsValidIndex( idx ) )
		return false;

	const char *pszLootList = mapRevolvingLootLists.Element( idx );

	// Get the loot list.
	m_pLootList = GetItemSchema()->GetLootListByName( pszLootList );
	m_unAuditDetailData = iCrateSeries;
		
	return m_pLootList != NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCrateLootListWrapper::BAttemptLootListStringInitialization( const IEconItemInterface *pEconItem )
{
	Assert( m_pLootList == NULL );

	// Find out what series this crate belongs to.
	static CSchemaAttributeDefHandle pAttr_LootListName( "loot list name" );
	if ( !pAttr_LootListName )
		return false;

	CAttribute_String str;
	if ( !pEconItem->FindAttribute( pAttr_LootListName, &str ) )
		return false;

	m_pLootList = GetItemSchema()->GetLootListByName( str.value().c_str() );

	return m_pLootList != NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCrateLootListWrapper::BAttemptLineItemInitialization( const IEconItemInterface *pEconItem )
{
	Assert( m_pLootList == NULL );

	// Do we have at least one line item specified?
	if ( !pEconItem->FindAttribute( CAttributeLineItemLootList::s_pAttrDef_RandomDropLineItems[0] ) )
		return false;

	m_pLootList = new CAttributeLineItemLootList( pEconItem );
	m_bIsDynamicallyAllocatedLootList = true;
		
	return true;
}
