//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "econ_item_view.h"
#include "econ_item_system.h"
#include "econ_item_description.h"
#include "econ_item_inventory.h"

#include "rtime.h"
#include "econ_gcmessages.h"
#include "gamestringpool.h"

// For localization
#include "tier3/tier3.h"
#include "vgui/ILocalize.h"

#include "isaverestore.h"
#include "dt_utlvector_send.h"
#include "dt_utlvector_recv.h"
#include <vgui_controls/Panel.h>

#ifdef CLIENT_DLL
#ifndef DEDICATED
#include "vgui/IScheme.h"
#endif
#endif

#if defined(TF_CLIENT_DLL)
#include "tf_duel_summary.h"
#include "econ_contribution.h"
#include "tf_player_info.h"
#include "tf_gcmessages.h"
#include "c_tf_freeaccount.h"
#include "c_tf_player.h"

static ConVar tf_hide_custom_decals( "tf_hide_custom_decals", "0", FCVAR_ARCHIVE );
#endif

#include "materialsystem/itexture.h"
#include "materialsystem/itexturecompositor.h"

#include "activitylist.h"

#ifdef CLIENT_DLL
#include "gc_clientsystem.h"
#endif // CLIENT_DLL


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"




// Networking tables for attributes
BEGIN_NETWORK_TABLE_NOBASE( CEconItemAttribute, DT_ScriptCreatedAttribute )

	// Note: we are networking the value as an int, even though it's a "float", because really it isn't
	// a float.  It's 32 raw bits.

#ifndef CLIENT_DLL
	SendPropInt( SENDINFO(m_iAttributeDefinitionIndex), -1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_NAME(m_flValue, m_iRawValue32), 32, SPROP_UNSIGNED ),
#if ENABLE_ATTRIBUTE_CURRENCY_TRACKING
	SendPropInt( SENDINFO(m_nRefundableCurrency), -1, SPROP_UNSIGNED ),
#endif // ENABLE_ATTRIBUTE_CURRENCY_TRACKING
#else
	RecvPropInt( RECVINFO(m_iAttributeDefinitionIndex) ),
	RecvPropInt( RECVINFO_NAME(m_flValue, m_iRawValue32) ),
	RecvPropFloat( RECVINFO(m_flValue), SPROP_NOSCALE ), // for demo compatibility only
#if ENABLE_ATTRIBUTE_CURRENCY_TRACKING
	RecvPropInt( RECVINFO(m_nRefundableCurrency) ),
#endif // ENABLE_ATTRIBUTE_CURRENCY_TRACKING
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemAttribute::CEconItemAttribute( void )
{
 	Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemAttribute::CEconItemAttribute( const attrib_definition_index_t iAttributeIndex, float flValue )
{
	Init();

	m_iAttributeDefinitionIndex = iAttributeIndex;
	SetValue( flValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemAttribute::CEconItemAttribute( const attrib_definition_index_t iAttributeIndex, uint32 unValue )
{
	Init();

	m_iAttributeDefinitionIndex = iAttributeIndex;
	SetIntValue( unValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemAttribute::SetValue( float flValue )
{
//	Assert( GetStaticData() && GetStaticData()->IsStoredAsFloat() );
	m_flValue = flValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemAttribute::SetIntValue( uint32 unValue )
{
	// @note we don't check the storage type here, because this is how it is set from the data file
	// Note that numbers approaching two billion cannot be stored in a float
	// representation because they will map to NaNs. Numbers below 16 million
	// will fail if denormals are disabled.
	m_flValue = *(float*)&unValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemAttribute::Init( void )
{
	m_iAttributeDefinitionIndex = INVALID_ATTRIB_DEF_INDEX;
	m_flValue = 0.0f;

#if ENABLE_ATTRIBUTE_CURRENCY_TRACKING
	m_nRefundableCurrency = 0;
#endif // ENABLE_ATTRIBUTE_CURRENCY_TRACKING
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemAttribute::operator=( const CEconItemAttribute &val )
{
	m_iAttributeDefinitionIndex = val.m_iAttributeDefinitionIndex;
	m_flValue = val.m_flValue;

#if ENABLE_ATTRIBUTE_CURRENCY_TRACKING
	m_nRefundableCurrency = val.m_nRefundableCurrency;
#endif // ENABLE_ATTRIBUTE_CURRENCY_TRACKING
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const CEconItemAttributeDefinition *CEconItemAttribute::GetStaticData( void ) const
{ 
	return GetItemSchema()->GetAttributeDefinition( m_iAttributeDefinitionIndex ); 
}

BEGIN_NETWORK_TABLE_NOBASE( CAttributeList, DT_AttributeList )
#if !defined( CLIENT_DLL )
SendPropUtlVectorDataTable( m_Attributes, MAX_ATTRIBUTES_PER_ITEM, DT_ScriptCreatedAttribute ),
#else
RecvPropUtlVectorDataTable( m_Attributes, MAX_ATTRIBUTES_PER_ITEM, DT_ScriptCreatedAttribute ),
#endif // CLIENT_DLL
END_NETWORK_TABLE()

BEGIN_DATADESC_NO_BASE( CAttributeList )
END_DATADESC()

//===========================================================================================================================
// SCRIPT CREATED ITEMS
//===========================================================================================================================
BEGIN_NETWORK_TABLE_NOBASE( CEconItemView, DT_ScriptCreatedItem )
#if !defined( CLIENT_DLL )
	SendPropInt( SENDINFO( m_iItemDefinitionIndex ), 20, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iEntityLevel ), 8 ),
	//SendPropInt( SENDINFO( m_iItemID ), 64, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iItemIDHigh ), 32, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iItemIDLow ), 32, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iAccountID ), 32, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iEntityQuality ), 5 ),
	SendPropBool( SENDINFO( m_bInitialized ) ),
	SendPropBool( SENDINFO( m_bOnlyIterateItemViewAttributes) ),
	SendPropDataTable(SENDINFO_DT(m_AttributeList), &REFERENCE_SEND_TABLE(DT_AttributeList)),
	SendPropInt( SENDINFO( m_iTeamNumber ) ),
	SendPropDataTable(SENDINFO_DT( m_NetworkedDynamicAttributesForDemos ), &REFERENCE_SEND_TABLE( DT_AttributeList ) ),
#else
	RecvPropInt( RECVINFO( m_iItemDefinitionIndex ) ),
	RecvPropInt( RECVINFO( m_iEntityLevel ) ),
	//RecvPropInt( RECVINFO( m_iItemID ) ),
	RecvPropInt( RECVINFO( m_iItemIDHigh ) ),
	RecvPropInt( RECVINFO( m_iItemIDLow ) ),
	RecvPropInt( RECVINFO( m_iAccountID ) ),
	RecvPropInt( RECVINFO( m_iEntityQuality ) ),
	RecvPropBool( RECVINFO( m_bInitialized ) ),
	RecvPropBool( RECVINFO( m_bOnlyIterateItemViewAttributes ) ),
	RecvPropDataTable(RECVINFO_DT(m_AttributeList), 0, &REFERENCE_RECV_TABLE(DT_AttributeList)),
	RecvPropInt( RECVINFO( m_iTeamNumber ) ),
	RecvPropDataTable( RECVINFO_DT( m_NetworkedDynamicAttributesForDemos ), 0, &REFERENCE_RECV_TABLE( DT_AttributeList ) ),
#endif // CLIENT_DLL
END_NETWORK_TABLE()

BEGIN_DATADESC_NO_BASE( CEconItemView )
	DEFINE_FIELD( m_iItemDefinitionIndex, FIELD_INTEGER ),
	DEFINE_FIELD( m_iEntityQuality, FIELD_INTEGER ),
	DEFINE_FIELD( m_iEntityLevel, FIELD_INTEGER ),
	DEFINE_FIELD( m_iItemID, FIELD_INTEGER ),
	//	DEFINE_FIELD( m_wszItemName, FIELD_STRING ),		Regenerated post-save
	//	DEFINE_FIELD( m_szItemName, FIELD_STRING ),		Regenerated post-save
	//	DEFINE_FIELD( m_szAttributeDescription, FIELD_STRING ),		Regenerated post-save
	//  m_AttributeLineColors	// Regenerated post-save
	//  m_Attributes			// Custom handling in Save()/Restore()
	DEFINE_FIELD( m_bInitialized, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bOnlyIterateItemViewAttributes, FIELD_BOOLEAN ),
	DEFINE_EMBEDDED( m_AttributeList ),
	DEFINE_EMBEDDED( m_NetworkedDynamicAttributesForDemos ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemView::CEconItemView( void )
{
	m_iItemDefinitionIndex = INVALID_ITEM_DEF_INDEX;
	m_iEntityQuality = (int)AE_UNDEFINED;
	m_iEntityLevel = 0;
	SetItemID( INVALID_ITEM_ID );
	m_iInventoryPosition = 0;
	m_bInitialized = false;
	m_bOnlyIterateItemViewAttributes = false;
	m_iAccountID = 0;
	m_pNonSOEconItem = NULL;
	m_bColorInit = false;
	m_bPaintOverrideInit = false;
	m_bHasPaintOverride = false;
	m_flOverrideIndex = 0.f;
#if defined( CLIENT_DLL )
	m_bIsTradeItem = false;
	m_iEntityQuantity = 1;
	m_unClientFlags = 0;
	m_unOverrideStyle = INVALID_STYLE_INDEX;
	m_unOverrideOrigin = kEconItemOrigin_Max;
#endif
#if BUILD_ITEM_NAME_AND_DESC
	m_pDescription = NULL;
	m_pszGrayedOutReason = NULL;
#endif

#ifdef CLIENT_DLL
	m_pWeaponSkinBase = NULL;
	m_pWeaponSkinBaseCompositor = NULL;
	m_iLastGeneratedTeamSkin = TF_TEAM_RED;
	m_bWeaponSkinUseHighRes = false;
	m_bWeaponSkinUseLowRes = false;
#endif // CLIENT_DLL

	m_iTeamNumber = TF_TEAM_RED;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemView::~CEconItemView( void )
{
#ifdef CLIENT_DLL
	SafeRelease( &m_pWeaponSkinBase );
	SafeRelease( &m_pWeaponSkinBaseCompositor );
#endif // CLIENT_DLL

	DestroyAllAttributes();

#if BUILD_ITEM_NAME_AND_DESC
	MarkDescriptionDirty();
	free( m_pszGrayedOutReason );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemView::CEconItemView( const CEconItemView &src )
{
#if BUILD_ITEM_NAME_AND_DESC
	m_pDescription = NULL;
	m_pszGrayedOutReason = NULL;
#endif

#ifdef CLIENT_DLL
	// Need to null these out here for initial behavior.
	m_pWeaponSkinBase = NULL;
	m_pWeaponSkinBaseCompositor = NULL;
	m_nWeaponSkinGeneration = 0;
	m_iLastGeneratedTeamSkin = TF_TEAM_RED;
	m_unWeaponSkinBaseCreateFlags = 0;
	m_bWeaponSkinUseHighRes = src.m_bWeaponSkinUseHighRes;
	m_bWeaponSkinUseLowRes  = src.m_bWeaponSkinUseLowRes;
#endif //CLIENT_DLL

	m_iTeamNumber = src.m_iTeamNumber; // keep the same team from the first creation

	*this = src;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemView::Init( int iDefIndex, int iQuality, int iLevel, uint32 iAccountID )
{
	m_AttributeList.Init();
	m_NetworkedDynamicAttributesForDemos.Init();

	m_iItemDefinitionIndex = iDefIndex;
	CEconItemDefinition *pData = GetStaticData();
	if ( !pData )
	{
		// We've got an item that we don't have static data for.
		return;
	}

	SetItemID( INVALID_ITEM_ID );
	m_bInitialized = true;
	m_iAccountID = iAccountID;

	if ( iQuality == AE_USE_SCRIPT_VALUE )
	{
		m_iEntityQuality = pData->GetQuality();

		// Kyle says: this is a horrible hack because AE_UNDEFINED will get stuffed into a uint8 when
		// loaded into the item definition, but then read back out into a regular int here.
		if ( m_iEntityQuality == (uint8)AE_UNDEFINED )
		{
			m_iEntityQuality = (int)AE_NORMAL;
		}
	}
	else if ( iQuality == k_unItemQuality_Any )
	{
		m_iEntityQuality = (int)AE_RARITY1;
	}
	else
	{
		m_iEntityQuality = iQuality;
	}

	if ( iLevel == AE_USE_SCRIPT_VALUE )
	{
		m_iEntityLevel = pData->RollItemLevel();
	}
	else
	{
		m_iEntityLevel = iLevel;
	}

	// We made changes to quality, level, etc. so mark the description as dirty.
	MarkDescriptionDirty();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemView& CEconItemView::operator=( const CEconItemView& src )
{
	m_iItemDefinitionIndex = src.m_iItemDefinitionIndex;
	m_iEntityQuality = src.m_iEntityQuality;
	m_iEntityLevel = src.m_iEntityLevel;
	SetItemID( src.GetItemID() );
	m_iInventoryPosition = src.m_iInventoryPosition;
	m_bInitialized = src.m_bInitialized;
	m_bOnlyIterateItemViewAttributes = src.m_bOnlyIterateItemViewAttributes;
	m_iAccountID = src.m_iAccountID;
	SetNonSOEconItem(src.m_pNonSOEconItem);
	m_bColorInit = false;		// reset Color init
	m_bPaintOverrideInit = false;
	m_bHasPaintOverride = false;
	m_flOverrideIndex = 0.f;
#ifdef CLIENT_DLL
	m_iLastGeneratedTeamSkin = src.m_iLastGeneratedTeamSkin;
	m_bIsTradeItem = src.m_bIsTradeItem;
	m_iEntityQuantity = src.m_iEntityQuantity;
	m_unClientFlags = src.m_unClientFlags;
	m_unOverrideStyle = src.m_unOverrideStyle;
	m_unOverrideOrigin = src.m_unOverrideOrigin;

	SafeAssign( &m_pWeaponSkinBase, src.m_pWeaponSkinBase );
	SafeAssign( &m_pWeaponSkinBaseCompositor, src.m_pWeaponSkinBaseCompositor );

	m_nWeaponSkinGeneration = src.m_nWeaponSkinGeneration;
	m_unWeaponSkinBaseCreateFlags = src.m_unWeaponSkinBaseCreateFlags;

	m_bWeaponSkinUseHighRes = src.m_bWeaponSkinUseHighRes;
	m_bWeaponSkinUseLowRes  = src.m_bWeaponSkinUseLowRes;

#endif // CLIENT_DLL

	m_iTeamNumber = src.m_iTeamNumber; // keep the same team from the first creation

	DestroyAllAttributes();

	m_AttributeList = src.m_AttributeList;
	m_NetworkedDynamicAttributesForDemos = src.m_NetworkedDynamicAttributesForDemos;

	// TODO: Copying the description pointer and refcounting it would work also.
	MarkDescriptionDirty();

	// Clear out any overrides we currently have, they'll get reset up on demand.
	ResetMaterialOverrides();
	return *this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconItemView::operator==( const CEconItemView &other ) const
{
	if ( IsValid() != other.IsValid() )
		return false;
	if ( ( GetItemID() != INVALID_ITEM_ID || other.GetItemID() != INVALID_ITEM_ID ) && GetItemID() != other.GetItemID() )
		return false;
	if ( GetItemDefIndex() != other.GetItemDefIndex() )
		return false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
GameItemDefinition_t *CEconItemView::GetStaticData( void ) const
{ 
	CEconItemDefinition	 *pRet		= GetItemSchema()->GetItemDefinition( m_iItemDefinitionIndex );
	GameItemDefinition_t *pTypedRet = dynamic_cast<GameItemDefinition_t *>( pRet );

	AssertMsg( pRet == pTypedRet, "Item definition of inappropriate type." );

	return pTypedRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int32 CEconItemView::GetQuality() const
{
	return GetSOCData()
		 ? GetSOCData()->GetQuality()
#ifdef TF_CLIENT_DLL
		 : GetFlags() & kEconItemFlagClient_StoreItem
		 ? AE_UNIQUE
#endif
		 : GetOrigin() != kEconItemOrigin_Invalid
		 ? GetItemQuality()
		 : AE_NORMAL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
style_index_t CEconItemView::GetStyle() const
{
	return GetItemStyle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
uint8 CEconItemView::GetFlags() const
{
	uint8 unSOCFlags = GetSOCData() ? GetSOCData()->GetFlags() : 0;

#if !defined( GAME_DLL )
	return unSOCFlags | m_unClientFlags;
#else // defined( GAME_DLL )
	return unSOCFlags;
#endif // !defined( GAME_DLL )
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
eEconItemOrigin CEconItemView::GetOrigin() const
{
#ifdef CLIENT_DLL
	if( m_unOverrideOrigin != kEconItemOrigin_Max )
	{
		return m_unOverrideOrigin;
	}
#endif//CLIENT_DLL

	return GetSOCData() ? GetSOCData()->GetOrigin() : kEconItemOrigin_Invalid;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEconItemView::GetQuantity() const
{
	return GetItemQuantity();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CEconItemView::GetCustomName() const
{
	return GetSOCData() ? GetSOCData()->GetCustomName() : NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CEconItemView::GetCustomDesc() const
{
	return GetSOCData() ? GetSOCData()->GetCustomDesc() : NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemView::IterateAttributes( class IEconItemAttributeIterator *pIterator ) const
{
	Assert( pIterator );

	// Note if we have network attribs, because m_NetworkedDynamicAttributesForDemos might be the iterator
	// which we're about to fill up below.
	const bool bHasNetworkedAttribsForDemos = m_NetworkedDynamicAttributesForDemos.GetNumAttributes() > 0;

	// First, we iterate over the attributes we have local copies of. If we have any attribute
	// values here they'll override whatever values we would otherwise have pulled from our
	// definition/CEconItem.
	const CAttributeList *pAttrList = GetAttributeList();
	if ( pAttrList )
	{
		pAttrList->IterateAttributes( pIterator );
	}

	if ( m_bOnlyIterateItemViewAttributes )
		return;

	// This wraps any other iterator class and will prevent double iteration of any attributes
	// that exist on us.
	class CEconItemAttributeIterator_EconItemViewWrapper : public IEconItemAttributeIterator
	{
	public:
		CEconItemAttributeIterator_EconItemViewWrapper( const CEconItemView *pEconItemView, IEconItemAttributeIterator *pIterator )
			: m_pEconItemView( pEconItemView )
			, m_pIterator( pIterator )
		{
			Assert( m_pEconItemView );
			Assert( m_pIterator );
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, attrib_value_t value )
		{
			Assert( pAttrDef );

			return m_pEconItemView->GetAttributeList()->GetAttributeByID( pAttrDef->GetDefinitionIndex() )
				 ? true
				 : m_pIterator->OnIterateAttributeValue( pAttrDef, value );
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, float value )
		{
			Assert( pAttrDef );

			return m_pEconItemView->GetAttributeList()->GetAttributeByID( pAttrDef->GetDefinitionIndex() )
				 ? true
				 : m_pIterator->OnIterateAttributeValue( pAttrDef, value );
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const uint64& value )
		{
			Assert( pAttrDef );

			return m_pEconItemView->GetAttributeList()->GetAttributeByID( pAttrDef->GetDefinitionIndex() )
				 ? true
				 : m_pIterator->OnIterateAttributeValue( pAttrDef, value );
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_String& value )
		{
			Assert( pAttrDef );

			return m_pEconItemView->GetAttributeList()->GetAttributeByID( pAttrDef->GetDefinitionIndex() )
				 ? true
				 : m_pIterator->OnIterateAttributeValue( pAttrDef, value );
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_DynamicRecipeComponent& value )
		{
			Assert( pAttrDef );

			return m_pEconItemView->GetAttributeList()->GetAttributeByID( pAttrDef->GetDefinitionIndex() )
				 ? true
				 : m_pIterator->OnIterateAttributeValue( pAttrDef, value );
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_ItemSlotCriteria& value )
		{
			Assert( pAttrDef );

			return m_pEconItemView->GetAttributeList()->GetAttributeByID( pAttrDef->GetDefinitionIndex() )
				 ? true
				 : m_pIterator->OnIterateAttributeValue( pAttrDef, value );
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_WorldItemPlacement& value )
		{
			Assert( pAttrDef );

			return m_pEconItemView->GetAttributeList()->GetAttributeByID( pAttrDef->GetDefinitionIndex() )
				 ? true
				 : m_pIterator->OnIterateAttributeValue( pAttrDef, value );
		}

	private:
		const CEconItemView *m_pEconItemView;
		IEconItemAttributeIterator *m_pIterator;
	};

	CEconItemAttributeIterator_EconItemViewWrapper iteratorWrapper( this, pIterator );

	// Next, iterate over our database-backed item if we have one... if we do have a DB
	// backing for our item here, that will also feed in the definition attributes.
	CEconItem *pEconItem = GetSOCData();
	if ( pEconItem )
	{
		pEconItem->IterateAttributes( &iteratorWrapper );
	}
	else if ( GetItemID() != INVALID_ITEM_ID && bHasNetworkedAttribsForDemos )
	{
		// Since there's no persistent data available, try the networked values!
		// note: only copies the default type and floats
		m_NetworkedDynamicAttributesForDemos.IterateAttributes( &iteratorWrapper );
	}
	// If we didn't have a DB backing, we can still iterate over our item definition
	// attributes ourselves. This can happen if we're previewing an item in the store, etc.
	else if ( GetStaticData() )
	{
		GetStaticData()->IterateAttributes( &iteratorWrapper );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemView::EnsureDescriptionIsBuilt() const
{
	tmZone( TELEMETRY_LEVEL1, TMZF_NONE, "%s", __FUNCTION__ );

#if BUILD_ITEM_NAME_AND_DESC
	if ( m_pDescription )
	{
		return;
	}

	m_pDescription = new CEconItemDescription;
#if defined( CLIENT_DLL )
	m_pDescription->SetIsToolTip( m_bIsToolTip );
#endif // CLIENT_DLL

	IEconItemDescription::YieldingFillOutEconItemDescription( m_pDescription, GLocalizationProvider(), this );

	// We use the empty string to mean "grey out but don't specify a user-facing reason".
	if ( m_pszGrayedOutReason && m_pszGrayedOutReason[0] )
	{
		m_pDescription->AddEmptyDescLine();
		m_pDescription->LocalizedAddDescLine( GLocalizationProvider(), m_pszGrayedOutReason, ATTRIB_COL_NEGATIVE, kDescLineFlag_Misc );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemView::MarkDescriptionDirty()
{
#if BUILD_ITEM_NAME_AND_DESC
	if ( m_pDescription )
	{
		delete m_pDescription;
		m_pDescription = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemView::SetGrayedOutReason( const char *pszGrayedOutReason )
{
#if BUILD_ITEM_NAME_AND_DESC
	if ( m_pszGrayedOutReason )
	{
		free( m_pszGrayedOutReason );
		m_pszGrayedOutReason = NULL;
	}

	if ( pszGrayedOutReason )
	{
		m_pszGrayedOutReason = strdup(pszGrayedOutReason);
	}

	MarkDescriptionDirty();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEconItemView::GetItemQuantity() const
{
	CEconItem *pSOCData = GetSOCData();
	if ( pSOCData )
	{
		return pSOCData->GetQuantity();
	}
#ifdef CLIENT_DLL
	return m_iEntityQuantity;
#else
	return 1;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
style_index_t CEconItemView::GetItemStyle() const
{

#ifdef CLIENT_DLL
	// Are we overriding the backing store style?
	if ( m_unOverrideStyle != INVALID_STYLE_INDEX )
		return m_unOverrideStyle;
#endif // CLIENT_DLL

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


	CEconItem *pSOCData = GetSOCData();
	if ( pSOCData )
		return pSOCData->GetStyle();

	return INVALID_STYLE_INDEX;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemView::SetClientItemFlags( uint8 unFlags )
{
	// Generally speaking, we have two uses for client flags:
	//
	//		- we don't have a backing store (a real CEconItem) but want to pretend we do
	//		  for purposes of generating tooltips, graying out icons, etc.
	//
	//		- we may or may not have a backing store but want to shove client-specific
	//		  information into the structure -- things like "this is the item being
	//		  actively previewed", etc.
	//
	// If neither of these two cases is true, then we're going to get unexpected
	// behavior where the GC and the client disagree about the item flags and then
	// Terrible Things happen. We assert to make sure we're in one of the above cases.
	Assert( !GetSOCData() || (unFlags & kEconItemFlags_CheckFlags_AllGCFlags) == 0 );

	m_unClientFlags = unFlags;
	MarkDescriptionDirty();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemView::SetItemStyleOverride( style_index_t unNewStyleOverride )
{
	// We should only ever override the style on items that don't have a real
	// backing store or we'll start getting disagreements about what the client
	// wants to happen and what's being stored on the GC. Unfortunately we can't
	// assert on this because we do it sometimes when previewing items.
	//Assert( !GetSOCData() );

	m_unOverrideStyle = unNewStyleOverride;
	MarkDescriptionDirty();
}


void CEconItemView::SetItemOriginOverride( eEconItemOrigin unNewOriginOverride )
{
	Assert( !GetSOCData() || m_pNonSOEconItem );
	Assert( unNewOriginOverride >= kEconItemOrigin_Invalid );
	Assert( unNewOriginOverride <= kEconItemOrigin_Max );	// Allow max.  We ignore this value if it's max

	m_unOverrideOrigin = unNewOriginOverride;
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItem *CEconItemView::GetSOCData( void ) const
{
	if ( m_pNonSOEconItem )
		return m_pNonSOEconItem;

#ifdef CLIENT_DLL
	// We need to find the inventory that contains this item. If we're not connected 
	// to a server, and the owner is the same as the local player, use the local inventory.
	// We need to do this for trading, since we are subscribed to the other person's cache.
	if ( !engine->IsInGame() && InventoryManager()->GetLocalInventory()->GetOwner().GetAccountID() == m_iAccountID )
		return InventoryManager()->GetLocalInventory()->GetSOCDataForItem( GetItemID() );
#endif // CLIENT_DLL

	// We're in-game. Find the inventory with our account ID.
	CPlayerInventory *pInventory = InventoryManager()->GetInventoryForAccount( m_iAccountID );
	if ( pInventory )
		return pInventory->GetSOCDataForItem( GetItemID() );

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Return the model to use for model panels containing this item
//-----------------------------------------------------------------------------
const char *CEconItemView::GetInventoryModel( void )
{
	if ( !GetStaticData() )
		return NULL;
	return GetStaticData()->GetInventoryModel();
}

//-----------------------------------------------------------------------------
// Purpose: Return the image to use for model panels containing this item
//-----------------------------------------------------------------------------
const char *CEconItemView::GetInventoryImage( void )
{
	if ( !GetStaticData() )
		return NULL;

	// Do we have a style set?
	const char* pStyleImage = NULL;
	if ( GetStaticData()->GetNumStyles() )
		pStyleImage = GetStaticData()->GetStyleInventoryImage( GetItemStyle() );
		
	if ( pStyleImage && *pStyleImage )
		return pStyleImage;

	return GetStaticData()->GetInventoryImage();
}

//-----------------------------------------------------------------------------
// Purpose: Return the drawing data for the image to use for model panels containing this item
//-----------------------------------------------------------------------------
bool CEconItemView::GetInventoryImageData( int *iPosition, int *iSize )
{
	if ( !GetStaticData() )
		return false;
	for ( int i = 0; i < 2; i++ )
	{
		iPosition[i] = GetStaticData()->GetInventoryImagePosition(i);
		iSize[i] = GetStaticData()->GetInventoryImageSize(i);
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Return the image to use for model panels containing this item
//-----------------------------------------------------------------------------
const char *CEconItemView::GetInventoryOverlayImage( int idx )
{
	if ( !GetStaticData() )
		return NULL;
	return GetStaticData()->GetInventoryOverlayImage( idx );
}

int	CEconItemView::GetInventoryOverlayImageCount( void )
{
	if ( !GetStaticData() )
		return 0;
	return GetStaticData()->GetInventoryOverlayImageCount();
}

//-----------------------------------------------------------------------------
// Purpose: Return the model to use when displaying this model on the player character model, if any
//-----------------------------------------------------------------------------
const char *CEconItemView::GetPlayerDisplayModel( int iClass, int iTeam ) const
{
	const CEconItemDefinition *pDef = GetStaticData();
	if ( !pDef )
		return NULL;

	// If we have styles, give the style system a chance to change the mesh used for this
	// player class.
	if ( pDef->GetNumStyles() )
	{
		style_index_t unStyle = GetItemStyle();

		const CEconStyleInfo *pStyle = pDef->GetStyleInfo( unStyle );

		// It's possible to get back a NULL pStyle if GetItemStyle() returns INVALID_STYLE_INDEX.
		if ( pStyle )
		{
#if defined( TF_DLL ) || defined( TF_CLIENT_DLL )
			// TF styles support per-class models.
			const CTFStyleInfo *pTFStyle = assert_cast<const CTFStyleInfo *>( pStyle );
			if ( pTFStyle->GetPlayerDisplayModel( iClass, iTeam ) )
				return pTFStyle->GetPlayerDisplayModel( iClass, iTeam );
#endif // defined( TF_DLL ) || defined( TF_CLIENT_DLL )

			if ( pStyle->GetBasePlayerDisplayModel() )
				return pStyle->GetBasePlayerDisplayModel();
		}
	}

#if defined( TF_DLL ) || defined( TF_CLIENT_DLL )
	// If we don't have a style, we still a couple potential overrides.
	if ( iClass >= 0 && iClass < LOADOUT_COUNT )
	{
		// We don't support overriding meshes in the visuals section, but we might still be overriding 
		// the model for each class at the schema level.
		const CTFItemDefinition *pTFDef = dynamic_cast<const CTFItemDefinition *>( pDef );
		if ( pTFDef )	
		{
			const char *pszModel = pTFDef->GetPlayerDisplayModel(iClass);
			if ( pszModel && pszModel[0] )
				return pszModel;
		}
	}
#endif // defined( TF_DLL ) || defined( TF_CLIENT_DLL )

	return pDef->GetBasePlayerDisplayModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEconItemView::GetSkin( int iTeam, bool bViewmodel /*= false*/ ) const
{
	int iDefaultSkin = -1;
#ifndef CSTRIKE_DLL
	// Immediately abort if we're out of range.
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS )
		return 0;

	// Do we have a style set?
	if ( GetStaticData()->GetNumStyles() )
		return GetStaticData()->GetStyleSkin( GetItemStyle(), iTeam, bViewmodel );
		
	iTeam = GetStaticData()->GetBestVisualTeamData( iTeam );
	if ( iTeam < 0 || iTeam >= TEAM_VISUAL_SECTIONS )
		return -1;

	// Do we have per-team skins set?
	const perteamvisuals_t *pVisData = GetStaticData()->GetPerTeamVisual( iTeam );
	if ( pVisData )
		return pVisData->iSkin;

	iDefaultSkin = GetItemDefinition()->GetDefaultSkin();
#endif

	// Fallback case.
	return iDefaultSkin;
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Handle assignment for textures, which involves some reference counting shenanigans.
//-----------------------------------------------------------------------------
void CEconItemView::SetWeaponSkinBase( ITexture* pBaseTex )
{
	SafeAssign( &m_pWeaponSkinBase, pBaseTex );
}

//-----------------------------------------------------------------------------
// Purpose: Handle assignment for compositors, which involves some reference counting shenanigans.
//-----------------------------------------------------------------------------
void CEconItemView::SetWeaponSkinBaseCompositor( ITextureCompositor * pTexCompositor )
{
	SafeAssign( &m_pWeaponSkinBaseCompositor, pTexCompositor );
}

//-----------------------------------------------------------------------------
// Purpose: Cancels a pending composite, if one is currently in process.
//-----------------------------------------------------------------------------
void CEconItemView::CancelWeaponSkinComposite( )
{
	SafeRelease( &m_pWeaponSkinBaseCompositor );
}
#endif // CLIENT_DLL


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CEconItemView::GetWorldDisplayModel() const
{
	CEconItemDefinition *pData = GetStaticData();
	if ( !pData )
		return NULL;

	return pData->GetWorldDisplayModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CEconItemView::GetExtraWearableModel() const
{
	CEconItemDefinition *pData = GetStaticData();
	if ( !pData )
		return NULL;

	return pData->GetExtraWearableModel();
}

const char *CEconItemView::GetExtraWearableViewModel() const
{
	CEconItemDefinition *pData = GetStaticData();
	if ( !pData )
		return NULL;

	return pData->GetExtraWearableViewModel();
}

const char *CEconItemView::GetVisionFilteredDisplayModel() const
{
	CEconItemDefinition *pData = GetStaticData();
	if ( !pData )
		return NULL;

	return pData->GetVisionFilteredDisplayModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEconItemView::GetQualityParticleType() const
{
	static CSchemaParticleHandle pSparkleSystem( "community_sparkle" );
	
	CEconItem* pItem = GetSOCData();
	if ( !pItem )
		return 0;

	if( GetSOCData()->GetQuality() == AE_SELFMADE || GetSOCData()->GetQuality() == AE_COMMUNITY )
		return pSparkleSystem ? pSparkleSystem->nSystemID : 0;
	else
		return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Return the animation set that this item wants the player to use (ie., melee, item1, pda)
//-----------------------------------------------------------------------------
int CEconItemView::GetAnimationSlot( void ) const
{
	if ( !GetStaticData() )
		return -1;

#if defined( CSTRIKE_DLL ) || defined( DOTA_DLL )
	return -1;
#else
	return GetStaticData()->GetAnimSlot();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Return an int that indicates whether the item should be dropped from a dead owner.
//-----------------------------------------------------------------------------
int CEconItemView::GetDropType( void )
{
	if ( !GetStaticData() )
		return 0;
	return GetStaticData()->GetDropType();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemView::DestroyAllAttributes( void )
{
	m_AttributeList.DestroyAllAttributes();
	m_NetworkedDynamicAttributesForDemos.DestroyAllAttributes();
	NetworkStateChanged();
	MarkDescriptionDirty();
}

extern const char *g_EffectTypes[NUM_EFFECT_TYPES];

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
const wchar_t *CEconItemView::GetItemName() const
{
	static const wchar_t *pwzDefaultName = L"";

	const CEconItemDescription *pDescription = GetDescription();
	if ( !pDescription )
		return pwzDefaultName;

	const econ_item_description_line_t *pNameDescLine = pDescription->GetFirstLineWithMetaType( kDescLineFlag_Name );
	if ( !pNameDescLine )
		return pwzDefaultName;

	return pNameDescLine->sText.Get();
}

//-----------------------------------------------------------------------------
void CEconItemView::GetRenderBounds( Vector& mins, Vector& maxs ) 
{ 
	const CEconItemDefinition *pDef = GetStaticData();
	if ( !pDef )
		return;

	int iClass = 0;
	int iTeam = 0;

#ifdef TF_CLIENT_DLL
	C_TFPlayer *pTFPlayer = ToTFPlayer( GetPlayerByAccountID( GetAccountID() ) );
	if ( pTFPlayer )
	{
		iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
		iTeam = pTFPlayer->GetTeamNumber();
	}
#endif // TF_CLIENT_DLL
	
	const char * pszModel = GetPlayerDisplayModel( iClass, iTeam );
	if ( !pszModel )
		return;

	int iIndex = modelinfo->GetModelIndex( pszModel );

	if ( iIndex == -1 )
	{
		// hard load the model to get its bounds
		MDLHandle_t hMDLFindResult = g_pMDLCache->FindMDL( pszModel );
		MDLHandle_t hMDL = pszModel ? hMDLFindResult : MDLHANDLE_INVALID;
		if ( g_pMDLCache->IsErrorModel( hMDL ) )
			return;

		const studiohdr_t * pStudioHdr = g_pMDLCache->GetStudioHdr( hMDL );
		VectorCopy( pStudioHdr->hull_min, mins );
		VectorCopy( pStudioHdr->hull_max, maxs );

		g_pMDLCache->Release( hMDLFindResult );
	}
	else
	{
		const model_t *pModel = modelinfo->GetModel( iIndex );
		modelinfo->GetModelRenderBounds( pModel, mins, maxs );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconItemView::InitNetworkedDynamicAttributesForDemos( void )
{
	if ( !GetSOCData() )
		return;

	class CEconDynamicAttributesForDemosIterator : public CEconItemSpecificAttributeIterator
	{
	public:
		CEconDynamicAttributesForDemosIterator( CAttributeList* out_NetworkedDynamicAttributesForDemos )
			: m_NetworkedDynamicAttributesForDemos( out_NetworkedDynamicAttributesForDemos )
		{
			m_bAdded = false;
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, attrib_value_t value ) OVERRIDE
		{
			CEconItemAttribute attribute( pAttrDef->GetDefinitionIndex(), value );
			m_NetworkedDynamicAttributesForDemos->AddAttribute( &attribute );
			m_bAdded = true;
			return true;
		}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, float value ) OVERRIDE
		{
			CEconItemAttribute attribute( pAttrDef->GetDefinitionIndex(), value );
			m_NetworkedDynamicAttributesForDemos->AddAttribute( &attribute );
			m_bAdded = true;
			return true;
		}

		bool BAdded( void ){ return m_bAdded; }

	private:
		bool m_bAdded;
		CAttributeList *m_NetworkedDynamicAttributesForDemos;
	};

	m_NetworkedDynamicAttributesForDemos.DestroyAllAttributes();

	CEconDynamicAttributesForDemosIterator it( &m_NetworkedDynamicAttributesForDemos );
	IterateAttributes( &it );
	
	if ( it.BAdded() )
	{
		NetworkStateChanged();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static int RemapOverridePaintIndexToRGB( uint32 unIndex, uint32 unTeamIndex )
{
	enum { kSamplePoints = 256, };

	static uint32 k_unWitchYellow[] = {
		5328971, 5328971, 5328971, 5328971, 5328971, 5328971, 5328971, 5328971, 
		5328971, 5328971, 5395018, 5591625, 5723465, 5855050, 5921095, 6052679, 
		6315591, 6447429, 6513222, 6710852, 7039299, 7170884, 7433793, 7631425, 
		7894336, 8222784, 8354622, 8618045, 8815420, 9078333, 9406779, 9604411, 
		9802040, 10064952, 10328376, 10459959, 10722870, 11051318, 11314483, 11511859, 
		11709490, 11841074, 12103729, 12301360, 12498479, 12630063, 12761901, 12959022, 
		13090604, 13156651, 13288236, 13485099, 13616683, 13682474, 13748010, 13813801, 
		13945386, 13945386, 14011433, 14011433, 14011433, 13945386, 13945386, 13879594, 
		13814314, 13813801, 13748010, 13682474, 13616681, 13616683, 13550890, 13550890, 
		13354027, 13288234, 13288236, 13222443, 13222443, 13156651, 13156651, 13025322, 
		13025324, 12959531, 12893740, 12893740, 12893740, 12762413, 12696621, 12696621, 
		12630828, 12630574, 12630574, 12499760, 12433713, 12433713, 12367922, 12302642, 
		12302387, 12236596, 12236598, 12171575, 12105528, 12039736, 11908409, 11974204, 
		11908413, 11842880, 11842880, 11842880, 11908421, 11776837, 11645765, 11579973, 
		11579975, 11514182, 11448137, 11382344, 11316298, 11184712, 11053387, 11053387, 
		10921802, 10855755, 10724941, 10659150, 10658896, 10527311, 10527057, 10395986, 
		10330195, 10264147, 10264147, 10264149, 10263895, 10198104, 10132313, 10198106, 
		10132058, 10263133, 10263133, 10328928, 10394721, 10394723, 10328675, 10394981, 
		10460263, 10526056, 10329190, 10394983, 10329188, 10394723, 10328928, 10328926, 
		10526042, 10526040, 10394452, 10459985, 10394446, 10460235, 10525511, 10525763, 
		10657089, 10657341, 10723131, 10854456, 10920502, 11183156, 11380273, 11512111, 
		11906350, 12038190, 12301100, 12629804, 12827436, 13221676, 13419306, 13616170, 
		13813803, 13945386, 14077226, 13879593, 13813801, 13616170, 13485099, 13221674, 
		12893227, 12630057, 12301098, 12103723, 11775018, 11511594, 11117353, 10853929, 
		10459431, 10130984, 9868327, 9670182, 9472807, 9275686, 9013027, 8815396, 
		8618019, 8420385, 8289569, 8223006, 8026399, 7894813, 7763484, 7829020, 
		7763738, 7894810, 7960857, 8158234, 8552475, 8684059, 8881690, 9079324, 
		9210908, 9407772, 9605405, 9671198, 9736989, 9736989, 9736991, 9605918, 
		9473565, 9341979, 9078809, 8684059, 8355096, 7763224, 7434010, 6973206, 
		6447895, 6118679, 5723416, 5394715, 5131549, 5000483, 5000234, 4999981, 
		5000242, 4999990, 4868407, 4671291, 4539707, 4539709, 4605502, 4671041, 
		4868420, 5000006, 5065799, 5131592, 5197385, 5263178, 5328971, 5263178, 
	};
	COMPILE_TIME_ASSERT( ARRAYSIZE( k_unWitchYellow ) == kSamplePoints );

	static uint32 k_unDistinctiveLackOfSanity[] =
	{
		5720667, 5720667, 5786460, 5720667, 5786460, 5786460, 5851996, 5851741, 
		6048606, 6048606, 6114145, 6114145, 6245474, 6311013, 6311013, 6441829, 
		6507624, 6573417, 6704491, 6704491, 6704493, 6901359, 6901359, 7097970, 
		7097970, 7163252, 7229045, 7294840, 7425656, 7491449, 7622523, 7688062, 
		7819137, 7950209, 8147077, 8277894, 8409226, 8540301, 8540303, 8671376, 
		8737171, 8737171, 8867985, 8671376, 8671374, 8540301, 8475019, 8475273, 
		8278408, 8147334, 8082309, 7885441, 7819902, 7754366, 7754621, 7558012, 
		7492217, 7426937, 7361142, 7164533, 7164531, 7164529, 7033456, 6967663, 
		6902382, 6836587, 6771564, 6771564, 6639978, 6574185, 6640232, 6508903, 
		6443110, 6574439, 6443110, 6442599, 6442599, 6573415, 6508392, 6573417, 
		6507624, 6572905, 6572905, 6572907, 6638187, 6703212, 6768494, 6834289, 
		6965105, 6964850, 7030132, 7029621, 7226486, 7160439, 7225720, 7356538, 
		7421820, 7421820, 7487102, 7617920, 7617921, 7748226, 7814021, 7879303, 
		7944328, 7944330, 8074635, 8271500, 8271246, 8336527, 8336529, 8598420, 
		8532629, 8597911, 8794521, 8859803, 8859803, 8990878, 9121440, 9120928, 
		9317793, 9317539, 9382823, 9448103, 9513898, 9644714, 9709995, 9840813, 
		9840815, 9906095, 9971890, 9971378, 10102194, 10167989, 10233269, 10364085, 
		10364087, 10363833, 10429369, 10429114, 10560186, 10625725, 10625468, 10691007, 
		10691007, 10756543, 10756289, 10756289, 10756289, 10756289, 10691007, 10625214, 
		10625468, 10559932, 10560186, 10429114, 10363576, 10298040, 10232501, 10101429, 
		10101683, 9970608, 9970608, 9773998, 9773998, 9642923, 9511848, 9511848, 
		9381030, 9381030, 9249956, 9053345, 8987806, 8922270, 8856731, 8725656, 
		8594839, 8463510, 8397971, 8201360, 8070287, 8070285, 7939211, 7807625, 
		7676805, 7545732, 7479937, 7283582, 7152508, 7086972, 6890361, 6759029, 
		6627700, 6496882, 6366064, 6300269, 6169194, 6103399, 5840997, 5775716, 
		5513312, 5448030, 5251162, 5054551, 5054806, 4857684, 4661073, 4595534, 
		4398666, 4267337, 4136776, 4070981, 4005186, 3874111, 3677502, 3480634, 
		3414840, 3283767, 3152951, 3152949, 3153203, 3153203, 3153460, 3153460, 
		3219253, 3285046, 3350839, 3416632, 3482425, 3614011, 3811133, 3942465, 
		4008260, 4336455, 4533321, 4664908, 4730701, 4993362, 5124948, 5256536, 
		5650013, 5781599, 6044003, 6175589, 6306920, 6504042, 6701167, 6898035, 
		7029621, 7292279, 7489404, 7686526, 7752064, 8014722, 8080261, 8277127,
	};
	COMPILE_TIME_ASSERT( ARRAYSIZE( k_unDistinctiveLackOfSanity ) == kSamplePoints );

	static uint32 k_unOverabundanceOfRottingFlesh[] =
	{
		12703514, 12703514, 12703514, 12703516, 12703516, 12703514, 12572700, 12506907, 
		12506907, 12506652, 12506907, 12506907, 12506652, 12506652, 12506907, 12506652, 
		12506652, 12506652, 12506654, 12506654, 12440861, 12441374, 12441374, 12375581, 
		12375581, 12375583, 12375583, 12375583, 12375583, 12309536, 12309536, 12309536, 
		12309536, 12309537, 12112928, 12243744, 12243744, 12177697, 12046881, 12046881, 
		11981090, 11915299, 11915299, 11915044, 11915044, 11849253, 11783206, 11783206, 
		11717415, 11717417, 11651624, 11585576, 11519785, 11519787, 11453994, 11388460, 
		11387948, 11256621, 11190830, 11190832, 11124785, 11058993, 11058995, 10992948, 
		10992950, 10861364, 10795572, 10729527, 10729527, 10663736, 10597690, 10466106, 
		10466106, 10465852, 10400062, 10334015, 10202431, 10202433, 10136640, 10136385, 
		10004801, 10004803, 10004549, 9938243, 10004037, 9938244, 9938246, 9872199, 
		9937478, 9937224, 9871431, 9871433, 9871433, 9805640, 9870921, 9805128, 
		9805128, 9805128, 9805128, 9805128, 9739337, 9804360, 9804360, 9804360, 
		9804360, 9804360, 9869640, 9869640, 9803847, 9803847, 9803847, 9803847, 
		9738054, 9803334, 9737541, 9672261, 9672261, 9672261, 9606468, 9606470, 
		9606470, 9540677, 9475397, 9475397, 9409604, 9409604, 9212739, 9212741, 
		9212741, 9146948, 9081668, 9081668, 8950596, 8950596, 9015875, 9015875, 
		9081668, 9081668, 9081664, 8950850, 8950848, 8885568, 8819775, 8951359, 
		8950846, 9016894, 9016892, 9016892, 9016890, 9147962, 9016376, 9147958, 
		9082165, 9213494, 9081907, 9147698, 9147698, 9278768, 9212975, 9212973, 
		9212716, 9278508, 9212713, 9212456, 9343272, 9212454, 9343524, 9343011, 
		9277218, 9408034, 9342241, 9407523, 9341730, 9341730, 9341219, 9275426, 
		9406242, 9339938, 9339938, 9339171, 9273380, 9272613, 9272613, 9141030, 
		9206310, 9140519, 9140265, 9205290, 9138985, 9138985, 9138987, 9073453, 
		9007662, 9007150, 9007150, 9137968, 9072945, 8941618, 9138484, 9007413, 
		9138999, 9204788, 9270581, 9205556, 9271604, 9272115, 9469490, 9404210, 
		9536049, 9536817, 9603119, 9734960, 9866542, 9801772, 9999147, 9999913, 
		10197288, 10263848, 10264613, 10462244, 10462754, 10463776, 10661152, 10792988, 
		10859547, 10991642, 11057688, 11190040, 11256086, 11388435, 11520274, 11521042, 
		11652625, 11784463, 11784974, 11916813, 11916813, 11982604, 12048397, 12114188, 
		12179981, 12246028, 12246028, 12311308, 12311308, 12311308, 12311308, 12377101, 
		12377101, 12377101, 12377101, 12377101, 12377101, 12377101, 12377101, 12377101, 
	};
	COMPILE_TIME_ASSERT( ARRAYSIZE( k_unOverabundanceOfRottingFlesh ) == kSamplePoints );

	// orange_flash
	static uint32 k_unTheFlamesBelow[] =
	{
		11548953, 11614745, 11746074, 11877659, 12008987, 12140572, 12337693, 12469278, 
		12666655, 12863776, 13060897, 13258274, 13455395, 13652772, 13849893, 14047014, 
		14244391, 14375976, 14573097, 14704681, 14836010, 14967595, 15033387, 15164716, 
		15164972, 15230508, 15230508, 15230508, 15164716, 15033387, 14967595, 14836010, 
		14638889, 14507304, 14310183, 14112806, 13915685, 13718308, 13455651, 13258274, 
		13060897, 12863776, 12666399, 12469278, 12272157, 12075036, 11943451, 11811866, 
		11680538, 11614745, 11548953, 11548953, 11548953, 11614745, 11680538, 11746330, 
		11877659, 12009244, 12206364, 12403485, 12600862, 12797983, 12995360, 13192482, 
		13389859, 13652772, 13849893, 14047270, 14244391, 14441512, 14638889, 14770474, 
		14901802, 15033387, 15099179, 15164972, 15230508, 15230508, 15098924, 14835755, 
		14572586, 14177832, 13783078, 13322789, 12862755, 12468001, 12139295, 11811101, 
		11614491, 11548954, 11549209, 11747351, 12011542, 12407317, 12869139, 13396754, 
		13924369, 14451983, 14979598, 15506957, 15902989, 16298508, 16562443, 16759819, 
		16760075, 16562188, 16166669, 15704590, 15110927, 14451217, 13791507, 13132053, 
		12538390, 12011032, 11681049, 11548953, 11614745, 11812888, 12142615, 12538646, 
		13066005, 13593875, 14187538, 14780944, 15308815, 15836430, 16232204, 16561932, 
		16694283, 16760075, 16561932, 16100621, 15506958, 14846992, 14121490, 13395732, 
		12670486, 12142359, 11746585, 11548953, 11614745, 11812632, 12076824, 12472343, 
		12934165, 13395988, 13923859, 14517265, 15044880, 15506702, 15968269, 16298508, 
		16562187, 16759819, 16760075, 16627979, 16298508, 15902733, 15375374, 14781967, 
		14188560, 13594897, 13066771, 12539156, 12077334, 11747351, 11549209, 11548954, 
		11680028, 11876637, 12139295, 12533793, 12928291, 13323045, 13783335, 14178088, 
		14572586, 14836011, 15098924, 15230508, 15230508, 15164972, 15099179, 15033387, 
		14901802, 14770474, 14638889, 14441768, 14244391, 14047270, 13849893, 13652772, 
		13455395, 13192482, 12995360, 12797983, 12600862, 12403485, 12206364, 12074780, 
		11877659, 11746330, 11680538, 11614745, 11548953, 11548953, 11548953, 11614745, 
		11680538, 11811866, 11943451, 12075036, 12272157, 12403742, 12600863, 12863776, 
		13060897, 13258274, 13455395, 13718308, 13915685, 14112806, 14310183, 14507304, 
		14638889, 14836010, 14967339, 15033387, 15164716, 15230508, 15230508, 15230508, 
		15164972, 15164716, 15033387, 14967595, 14836010, 14704681, 14573097, 14375976, 
		14244391, 14047014, 13849893, 13652772, 13455395, 13258274, 13060897, 12863776, 
		12666655, 12469278, 12337693, 12140572, 12008987, 11877659, 11746074, 11614745, 
	};
	COMPILE_TIME_ASSERT( ARRAYSIZE( k_unTheFlamesBelow ) == kSamplePoints );

	// green_pulse
	static uint32 k_unThatQueesyFeeling[] =
	{
		7439904, 7571489, 7703329, 7900706, 8032547, 8295716, 8493349, 8756518, 
		9019943, 9283369, 9546794, 9810219, 10073644, 10402606, 10666031, 10929200, 
		11192625, 11390259, 11653428, 11851061, 12048437, 12180278, 12312119, 12443703, 
		12509496, 12575288, 12575288, 12509496, 12443703, 12312119, 12180278, 11982901, 
		11785268, 11522099, 11258674, 10995249, 10731823, 10468398, 10139181, 9810219, 
		9546794, 9217576, 8954151, 8690726, 8427557, 8229924, 7966755, 7834914, 
		7637537, 7571488, 7440160, 7439904, 7439904, 7505696, 7637281, 7769121, 
		7900962, 8098339, 8361508, 8624933, 8888359, 9151784, 9415209, 9744171, 
		10073388, 10336814, 10666031, 10929456, 11192882, 11456051, 11719476, 11982645, 
		12114486, 12311863, 12443447, 12509496, 12575288, 12575288, 12509496, 12443703, 
		12311863, 12180278, 11982645, 11785012, 11521843, 11258674, 10995249, 10731823, 
		10402862, 10139181, 9810219, 9546794, 9283368, 8954151, 8690982, 8427557, 
		8229924, 8032291, 7834914, 7703073, 7571489, 7505696, 7439904, 7439904, 
		7505696, 7571745, 7703329, 7900706, 8098083, 8295716, 8559141, 8822310, 
		9085736, 9349161, 9678378, 9941804, 10270765, 10534447, 10863408, 11126833, 
		11390258, 11653428, 11851061, 12048694, 12246070, 12377911, 12509240, 12575288, 
		12575288, 12575288, 12509240, 12377911, 12246070, 12048694, 11851061, 11653428, 
		11390258, 11126833, 10863408, 10534447, 10270765, 9941804, 9678378, 9349161, 
		9085736, 8822310, 8559141, 8295716, 8098083, 7900706, 7703329, 7571745, 
		7505696, 7439904, 7439904, 7505696, 7571489, 7703073, 7834914, 8032291, 
		8229924, 8427557, 8690982, 8954151, 9283368, 9546794, 9810219, 10139181, 
		10402862, 10731823, 10995249, 11258674, 11521843, 11785012, 11982645, 12180278, 
		12311863, 12443703, 12509496, 12575288, 12575288, 12509496, 12443447, 12311863, 
		12180022, 11982645, 11719476, 11521843, 11258418, 10929456, 10666031, 10336814, 
		10073388, 9744427, 9480745, 9151784, 8888359, 8624933, 8361508, 8163875, 
		7900962, 7769121, 7637281, 7505696, 7439904, 7439904, 7440160, 7571488, 
		7637537, 7834914, 7966755, 8164132, 8427557, 8690726, 8954151, 9217576, 
		9546538, 9810219, 10139181, 10402606, 10731823, 10995249, 11258674, 11521843, 
		11785268, 11982645, 12180278, 12312119, 12443703, 12509496, 12575288, 12575288, 
		12509496, 12443703, 12312119, 12180278, 12048437, 11851061, 11653428, 11390259, 
		11192625, 10929200, 10666031, 10402606, 10073644, 9810219, 9546794, 9283369, 
		9019943, 8756518, 8493349, 8295716, 8032547, 7900706, 7703329, 7571489, 
	};
	COMPILE_TIME_ASSERT( ARRAYSIZE( k_unThatQueesyFeeling ) == kSamplePoints );

	// blue_pulse
	static uint32 k_unBubbleBubble[] =
	{
		9094364, 9160156, 9291485, 9357278, 9488607, 9685472, 9816801, 10013922, 
		10145251, 10342372, 10539237, 10736358, 10933479, 11130345, 11327466, 11458795, 
		11655916, 11852781, 11984366, 12115695, 12247024, 12378352, 12444145, 12509681, 
		12575474, 12641010, 12641010, 12575474, 12509681, 12444145, 12312816, 12181487, 
		12049902, 11853037, 11655916, 11458795, 11261930, 11064808, 10802151, 10605030, 
		10407908, 10211043, 10013922, 9816801, 9619936, 9488607, 9357022, 9225693, 
		9160156, 9094364, 9094364, 9094364, 9159900, 9225693, 9357022, 9553886, 
		9751008, 9948129, 10210786, 10473444, 10736102, 11064551, 11392745, 11655659, 
		11983853, 12312303, 12640496, 12903410, 13166068, 13494262, 13691383, 13954040, 
		14150905, 14282490, 14413819, 14545148, 14545148, 14545148, 14545148, 14413819, 
		14282490, 14151162, 13954040, 13756919, 13494262, 13231604, 12968947, 12706289, 
		12377839, 12115181, 11786732, 11458538, 11195880, 10867430, 10604773, 10342115, 
		10079458, 9816800, 9619679, 9422814, 9291485, 9160156, 9094364, 9094364, 
		9094364, 9094364, 9160157, 9291485, 9422814, 9554143, 9685472, 9882593, 
		10079458, 10276579, 10473701, 10736358, 10933479, 11130345, 11327466, 11590123, 
		11787245, 11918574, 12115695, 12247024, 12378352, 12509681, 12575218, 12641010, 
		12641010, 12641010, 12575218, 12509681, 12378352, 12247024, 12115695, 11918574, 
		11787245, 11590123, 11327466, 11130345, 10933479, 10736358, 10473701, 10276579, 
		10079458, 9882593, 9685472, 9554143, 9422814, 9291485, 9160157, 9094364, 
		9094364, 9094364, 9094364, 9160156, 9291485, 9422814, 9619679, 9816800, 
		10079458, 10342115, 10604773, 10867430, 11195880, 11458538, 11786732, 12115181, 
		12377839, 12706289, 12968947, 13231604, 13494262, 13756919, 13954040, 14151162, 
		14282490, 14413819, 14545148, 14545148, 14545148, 14545148, 14413819, 14282490, 
		14150905, 13954040, 13691383, 13494262, 13231604, 12903410, 12640497, 12312303, 
		11983853, 11721195, 11392745, 11064551, 10801638, 10473444, 10210786, 9948129, 
		9751008, 9553887, 9357022, 9225693, 9159900, 9094364, 9094364, 9094364, 
		9160156, 9225693, 9357022, 9488351, 9619935, 9816801, 10013922, 10210787, 
		10407908, 10605030, 10802151, 11064808, 11261674, 11458795, 11655916, 11853037, 
		12049902, 12181231, 12312560, 12443889, 12575217, 12641010, 12641010, 12641010, 
		12575474, 12575217, 12509681, 12378352, 12247024, 12115695, 11984366, 11852781, 
		11655916, 11524331, 11327466, 11130345, 10933479, 10736358, 10539237, 10342372, 
		10145251, 10013922, 9816801, 9685472, 9554143, 9422814, 9291485, 9160156, 
	};
	COMPILE_TIME_ASSERT( ARRAYSIZE( k_unBubbleBubble ) == kSamplePoints );

	// purple_orange_rand
	static uint32 k_unAfraidOfShadowsDark[] =
	{
		4536928, 4536928, 4602721, 4602721, 4668515, 4668515, 4734308, 4734308, 
		4734308, 4734309, 4734309, 4734309, 4865896, 4865896, 4931689, 4931689, 
		4997226, 4997226, 5063019, 5063019, 5128813, 5194606, 5194607, 5260400, 
		5260400, 5391730, 5457523, 5457524, 5457524, 5523317, 5589111, 5654904, 
		5654905, 5654905, 5720698, 5852028, 5917821, 5917822, 5983615, 5983615, 
		6115202, 6115202, 6115203, 6180996, 6312326, 6378119, 6378120, 6443913, 
		6444683, 6576785, 6577556, 6709913, 6710941, 6843555, 6844583, 6845868, 
		6912945, 6914230, 6915515, 6982592, 6983877, 6985161, 6986446, 6987986, 
		6989014, 6924762, 6926046, 6926817, 6862308, 6863079, 6863593, 6798827, 
		6799597, 6799083, 6929385, 6863079, 6927845, 6992866, 7057632, 7187677, 
		7252442, 7382487, 7447251, 7577296, 7641805, 7772362, 7836871, 7967172, 
		8097473, 8227774, 8292283, 8422841, 8487607, 8749750, 8814516, 8879794, 
		8945072, 9075888, 8946614, 8752575, 8427464, 8102353, 7842008, 7647197, 
		7582176, 7842777, 8233938, 8559562, 8950979, 9277117, 9537977, 9733814, 
		9734070, 9734070, 9734070, 9734070, 9734071, 9799863, 9799863, 9799863, 
		9799863, 9799863, 9799863, 9799863, 9799863, 9799604, 9799863, 9799604, 
		9799344, 9733548, 9733032, 9732771, 9732509, 9731992, 9731473, 9731211, 
		9796485, 9861502, 9861239, 9926256, 10057321, 10122338, 10253403, 10450004, 
		10580557, 10777414, 10974016, 11235897, 11432754, 11629612, 11825958, 12088353, 
		12350747, 12547606, 12875538, 13072654, 13203723, 13531656, 13729032, 13729032, 
		13794568, 13991688, 14188808, 14254344, 14385672, 14451464, 14648328, 14845704, 
		14977032, 15108360, 15239689, 15436809, 15568393, 15831050, 15962892, 16160781, 
		16226832, 16293140, 16294170, 16294943, 16295713, 16296229, 16297257, 16363566, 
		16364595, 16365365, 16431930, 16432957, 16433729, 16500037, 16501065, 16501836, 
		16502864, 16503634, 16569685, 16570712, 16571482, 16637789, 16638303, 16639072, 
		16639586, 16640355, 16640869, 16707173, 16707430, 16707943, 16708456, 16708712, 
		16709224, 16775016, 16775272, 16775784, 16776040, 16776552, 16776295, 16776551, 
		16776551, 16776039, 16709991, 16709736, 16709224, 16511336, 16247913, 16116074, 
		15786858, 15457899, 15193963, 14865004, 14601837, 14206573, 13811822, 13351022, 
		12890479, 12429935, 11969392, 11508591, 11048049, 10587504, 10192752, 9732208, 
		9337455, 8877166, 8482670, 8153453, 7758957, 7430252, 7101291, 6772586, 
		6509418, 6246505, 5983337, 5786216, 5588840, 5391719, 5194599, 5063014, 
	};
	COMPILE_TIME_ASSERT( ARRAYSIZE( k_unAfraidOfShadowsDark ) == kSamplePoints );

	static uint32 *k_pPointSampleContent_Team0[] =
	{
		&k_unWitchYellow[0],
		&k_unDistinctiveLackOfSanity[0],
		&k_unOverabundanceOfRottingFlesh[0],
		&k_unTheFlamesBelow[0],
		&k_unThatQueesyFeeling[0],
		&k_unAfraidOfShadowsDark[0],
	};

	static uint32 *k_pPointSampleContent_Team1[] =
	{
		&k_unWitchYellow[0],
		&k_unDistinctiveLackOfSanity[0],
		&k_unOverabundanceOfRottingFlesh[0],
		&k_unBubbleBubble[0],
		&k_unThatQueesyFeeling[0],
		&k_unAfraidOfShadowsDark[0],
	};
	COMPILE_TIME_ASSERT( ARRAYSIZE( k_pPointSampleContent_Team0 ) == ARRAYSIZE( k_pPointSampleContent_Team1 ) );

	if ( unIndex >= ARRAYSIZE( k_pPointSampleContent_Team0 ) )
		return 0;

	if ( unTeamIndex > 1 )
		return 0;

	const float fScaledTime = gpGlobals->curtime * 22.0f;				// arbitrary time scalar people liked
	const unsigned int unSamplePoint0 = (unsigned int)fScaledTime % kSamplePoints;
	const unsigned int unSamplePoint1 = (unSamplePoint0 + 1) % kSamplePoints;

	const float fDelta = fScaledTime - (unsigned int)fScaledTime;		// offset between two sample points for blending

	const uint32 *punData = (unTeamIndex == 0 ? k_pPointSampleContent_Team0 : k_pPointSampleContent_Team1)[unIndex];
		
	Color c0;
	c0.SetRawColor( punData[unSamplePoint0] );

	Color c1;
	c1.SetRawColor( punData[unSamplePoint1] );

	const Color cBlend( Lerp( fDelta, c0.r(), c1.r() ),
						Lerp( fDelta, c0.g(), c1.g() ),
						Lerp( fDelta, c0.b(), c1.b() ) );

	return cBlend.GetRawColor();
}

//-----------------------------------------------------------------------------
// Purpose: Get RGB modifying attribute value
//-----------------------------------------------------------------------------
int CEconItemView::GetModifiedRGBValue( bool bAltColor )
{
	enum
	{
		kPaintConstant_Default = 0,
		kPaintConstant_OldTeamColor = 1,
	};

	static CSchemaAttributeDefHandle pAttr_Paint( "set item tint rgb" );
	static CSchemaAttributeDefHandle pAttr_Paint2( "set item tint rgb 2" );

	static CSchemaAttributeDefHandle pAttr_PaintOverride( "SPELL: set item tint RGB" );

	// Do we have an override paint color? This takes precedence over base paints and team
	// paints.
#if defined( TF_DLL ) || defined( TF_CLIENT_DLL )
	extern bool TF_IsHolidayActive( /*EHoliday*/ int eHoliday );

	if ( TF_IsHolidayActive( kHoliday_HalloweenOrFullMoon ) )
#endif // defined( TF_DLL ) || defined( TF_CLIENT_DLL )
	{
		if ( !m_bPaintOverrideInit )
		{
			m_bHasPaintOverride = FindAttribute_UnsafeBitwiseCast<attrib_value_t>( this, pAttr_PaintOverride, &m_flOverrideIndex );
			m_bPaintOverrideInit = true;
		}
			
		if ( m_bHasPaintOverride )
			return RemapOverridePaintIndexToRGB( (uint32)m_flOverrideIndex, bAltColor ? 1 : 0 );
	}

	if ( !m_bColorInit )
	{
		// See if we also have a secondary paint color.
		uint32 unRGB = kPaintConstant_Default;
		uint32 unRGBAlt = kPaintConstant_Default;
		float fRGB;
		float fRGBAlt;

		// If we have no base paint color we don't do anything special.
		if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( this, pAttr_Paint, &fRGB ) )
		{
			unRGB = (uint32)fRGB;
			unRGBAlt = unRGB;
		}

		// Backwards compatibility for old team colored items.
		if ( unRGB == kPaintConstant_OldTeamColor )
		{
			unRGB = RGB_INT_RED;
			unRGBAlt = RGB_INT_BLUE;
		}
		else if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( this, pAttr_Paint2, &fRGBAlt ) )
		{
			unRGBAlt = (uint32)fRGBAlt;
		}
		else
		{
			// By default our secondary color will match our primary if we can't find a replacement.
			unRGBAlt = unRGB;
		}

		m_unRGB = unRGB;
		m_unAltRGB = unRGBAlt;

		m_bColorInit = true;
	}

	return bAltColor ? m_unAltRGB : m_unRGB;
}

uint64 CEconItemView::GetCustomUserTextureID()
{
	static CSchemaAttributeDefHandle pAttr_CustomTextureLo( "custom texture lo" );
	static CSchemaAttributeDefHandle pAttr_CustomTextureHi( "custom texture hi" );

#if defined( TF_CLIENT_DLL )
	if ( tf_hide_custom_decals.GetBool() )
	{
		CBasePlayer *pPlayer = GetPlayerByAccountID( m_iAccountID );
		if ( !pPlayer || ( pPlayer != C_BasePlayer::GetLocalPlayer() ) )
			return 0;
	}
#endif // TF_CLIENT_DLL

	uint32 unLowVal, unHighVal;
	const bool bHasLowVal = FindAttribute( pAttr_CustomTextureLo, &unLowVal ),
			   bHasHighVal = FindAttribute( pAttr_CustomTextureHi, &unHighVal );

	// We should have both, or neither.  We should never have just one
	Assert( bHasLowVal == bHasHighVal );

	if ( bHasLowVal && bHasHighVal )
	{
		return ((uint64)unHighVal << 32) | (uint64)unLowVal;
	}

	// No attribute set
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAttributeList::CAttributeList()
{
	m_pManager = NULL;
	m_Attributes.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeList::SetManager( CAttributeManager *pManager )
{
	m_pManager = pManager;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeList::Init()
{
	m_Attributes.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeList::IterateAttributes( class IEconItemAttributeIterator *pIterator ) const
{
	Assert( pIterator );

	FOR_EACH_VEC( m_Attributes, i )
	{
		const CEconItemAttribute *pAttrInst = &m_Attributes[i];

		const CEconItemAttributeDefinition *pAttrDef = pAttrInst->GetStaticData();
		if ( !pAttrDef )
			continue;

		const ISchemaAttributeType *pAttrType = pAttrDef->GetAttributeType();
		Assert( pAttrType );
		Assert( pAttrType->BSupportsGameplayModificationAndNetworking() );

		// We know (and assert) that we only need 32 bits of data to store this attribute
		// data. We don't know anything about the type but we'll let the type handle it
		// below.
		attribute_data_union_t value;
		value.asFloat = pAttrInst->m_flValue;

		if ( !pAttrType->OnIterateAttributeValue( pIterator, pAttrDef, value ) )
			return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeList::DestroyAllAttributes( void )
{
	if ( m_Attributes.Count() )
	{
		m_Attributes.Purge();
		NotifyManagerOfAttributeValueChanges();
		NetworkStateChanged();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeList::AddAttribute( CEconItemAttribute *pAttribute )
{
	Assert( pAttribute );

	// Only add attributes to the attribute list if they have a definition we can
	// pull data from.
	if ( !pAttribute->GetStaticData() )
		return;

	m_Attributes.AddToTail( *pAttribute );
	NetworkStateChanged();
	NotifyManagerOfAttributeValueChanges();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeList::SetRuntimeAttributeValue( const CEconItemAttributeDefinition *pAttrDef, float flValue )
{
	Assert( pAttrDef );
	
	// Look for an existing attribute.
	const int iAttributes = GetNumAttributes();
	for ( int i = 0; i < iAttributes; i++ )
	{
		CEconItemAttribute *pAttribute = GetAttribute(i);

		if ( pAttribute->GetAttribIndex() == pAttrDef->GetDefinitionIndex() )
		{
			// Found existing attribute -- change value.
			pAttribute->m_flValue = flValue;
			NotifyManagerOfAttributeValueChanges();
			return;
		}
	}

	// Couldn't find an existing attribute for this definition -- make a new one.
	CEconItemAttribute attribute;
	attribute.m_iAttributeDefinitionIndex = pAttrDef->GetDefinitionIndex();
	attribute.m_flValue = flValue;

	m_Attributes.AddToTail( attribute );
	NotifyManagerOfAttributeValueChanges();
}

#if ENABLE_ATTRIBUTE_CURRENCY_TRACKING
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeList::SetRuntimeAttributeRefundableCurrency( const CEconItemAttributeDefinition *pAttrDef, int iRefundableCurrency )
{
	Assert( pAttrDef );

	// Look for an existing attribute.
	const int iAttributes = GetNumAttributes();
	for ( int i = 0; i < iAttributes; i++ )
	{
		CEconItemAttribute *pAttribute = GetAttribute(i);

		if ( pAttribute->GetAttribIndex() == pAttrDef->GetDefinitionIndex() )
		{
			// Found existing attribute -- change value.
			pAttribute->m_nRefundableCurrency = iRefundableCurrency;
			return;
		}
	}

	AssertMsg1( false, "Unable to find attribute '%s' for setting currency!", pAttrDef->GetDefinitionName() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CAttributeList::GetRuntimeAttributeRefundableCurrency( const CEconItemAttributeDefinition *pAttrDef ) const
{
	const int iAttributes = GetNumAttributes();
	for ( int i = 0; i < iAttributes; i++ )
	{
		const CEconItemAttribute *pAttribute = GetAttribute(i);

		if ( pAttribute->GetAttribIndex() == pAttrDef->GetDefinitionIndex() )
			return pAttribute->m_nRefundableCurrency;
	}

	AssertMsg1( false, "Unable to find attribute '%s' for getting currency!", pAttrDef->GetDefinitionName() );
	return 0;
}
#endif // ENABLE_ATTRIBUTE_CURRENCY_TRACKING

//-----------------------------------------------------------------------------
// Purpose: Remove an attribute by name
//-----------------------------------------------------------------------------
void CAttributeList::RemoveAttribute( const CEconItemAttributeDefinition *pAttrDef )
{
	const int iAttributes = m_Attributes.Count();
	for ( int i = 0; i < iAttributes; i++ )
	{
		if ( m_Attributes[i].GetStaticData() == pAttrDef )
		{
			m_Attributes.Remove( i );
			NotifyManagerOfAttributeValueChanges();
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Remove an attribute by index
//-----------------------------------------------------------------------------
void CAttributeList::RemoveAttributeByIndex( int iIndex )
{
	if ( iIndex < 0 || iIndex >= GetNumAttributes() )
		return;

	m_Attributes.Remove( iIndex );
	NotifyManagerOfAttributeValueChanges();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const CEconItemAttribute *CAttributeList::GetAttributeByID( int iAttributeID ) const
{
	int iAttributes = m_Attributes.Count();
	for ( int i = 0; i < iAttributes; i++ )
	{
		const CEconItemAttributeDefinition *pData = m_Attributes[i].GetStaticData();

		if ( pData && ( pData->GetDefinitionIndex() == iAttributeID ) )
			return &m_Attributes[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const CEconItemAttribute *CAttributeList::GetAttributeByName( const char *pszAttribDefName ) const
{
	CEconItemAttributeDefinition *pDef = GetItemSchema()->GetAttributeDefinitionByName( pszAttribDefName );
	if ( !pDef )
		return NULL;

	int iAttributes = m_Attributes.Count();
	for ( int i = 0; i < iAttributes; i++ )
	{
		if ( m_Attributes[i].GetStaticData()->GetDefinitionIndex() == pDef->GetDefinitionIndex() )
			return &m_Attributes[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeList::operator=( const CAttributeList& src )
{
	m_Attributes = src.m_Attributes;

	// HACK: We deliberately don't copy managers, because attributelists are contained inside 
	// CEconItemViews, which we duplicate inside CItemModelPanels all the time. If the manager
	// is copied, copies will mess with the attribute caches of the copied item.
	// Our manager will be setup properly by the CAttributeManager itself if we have an associated entity.
	m_pManager = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeList::NotifyManagerOfAttributeValueChanges( void ) 
{ 
	if ( m_pManager ) 
	{
		m_pManager->OnAttributeValuesChanged();
	}
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool DoesItemPassSearchFilter( const IEconItemDescription *pDescription, const wchar_t* wszFilter )
{
	// check if item matches name filter
	if ( wszFilter && *wszFilter )
	{
		if ( !pDescription )
		{
			return false;
		}

		wchar_t wszBuffer[ 4096 ] = L"";
		for ( unsigned int i = 0; i < pDescription->GetLineCount(); i++ )
		{
			const econ_item_description_line_t& line = pDescription->GetLine(i);

			if ( !(line.unMetaType & ( kDescLineFlag_Collection | kDescLineFlag_CollectionCurrentItem ) ) )
			{
				V_wcscat_safe( wszBuffer, line.sText.Get() );
			}
		}

		V_wcslower( wszBuffer );
		if ( !wcsstr( wszBuffer, wszFilter ) )
		{
			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBasePlayer *GetPlayerByAccountID( uint32 unAccountID )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer == NULL )
			continue;

		CSteamID steamIDPlayer;
		if ( !pPlayer->GetSteamID( &steamIDPlayer ) )
			continue;

		// return the player with the matching ID
		if ( steamIDPlayer.GetAccountID() == unAccountID )
		{
			return pPlayer;
		}
	}

	return NULL;
}

#endif // CLIENT_DLL
