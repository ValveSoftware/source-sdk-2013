//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: EconItemSchema: Defines a schema for econ items
//
//=============================================================================

#include "cbase.h"
#include "econ_item_schema.h"
#include "tier1/fmtstr.h"
#include "tier1/UtlSortVector.h"
#include "tier2/tier2.h"
#include "filesystem.h"
#include "schemainitutils.h"
#include "gcsdk/gcsdk_auto.h"
#include "rtime.h"
#include "item_selection_criteria.h"
#include "checksum_sha1.h"

#include <google/protobuf/text_format.h>
#include <string.h>

#include "materialsystem/imaterialsystem.h"
#include "materialsystem/itexture.h"
#include "materialsystem/itexturecompositor.h"

#include "econ_paintkit.h"

#if ( defined( _MSC_VER ) && _MSC_VER >= 1900 )
#define timezone _timezone
#define daylight _daylight
#endif

// For holiday-limited loot lists.
#include "econ_holidays.h"

// Only used for startup testing.
#include "econ_item_tools.h"

#include "econ_quests.h"

#if defined(CLIENT_DLL) || defined(GAME_DLL)
	#include "econ_item_system.h"
	#include "econ_item.h"
	#include "activitylist.h"

	#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
		#include "tf_gcmessages.h"
	#endif
#endif


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace GCSDK;


CEconItemSchema & GEconItemSchema()
{
#if defined( EXTERNALTESTS_DLL )
	static CEconItemSchema g_econItemSchema;
	return g_econItemSchema;
#else
	return *ItemSystem()->GetItemSchema();
#endif
}

const char *g_szDropTypeStrings[] =
{
	"",		 // Blank and none mean the same thing: stay attached to the body.
	"none",
	"drop",	 // The item drops off the body.
	"break", // Not implemented, but an example of a type that could be added.
};

const char *g_TeamVisualSections[TEAM_VISUAL_SECTIONS] = 
{
	"visuals",		// TF_TEAM_UNASSIGNED. Visual changes applied to both teams.
	NULL,			// TF_TEAM_SPECTATOR. Unused.
	"visuals_red",	// TF_TEAM_RED
	"visuals_blu",	// TF_TEAM_BLUE
	"visuals_mvm_boss",	// Hack to override things in MvM at a general level
};

int GetTeamVisualsFromString( const char *pszString )
{
	for ( int i = 0; i < TEAM_VISUAL_SECTIONS; i++ )
	{
		// There's a NULL hidden in g_TeamVisualSections
		if ( g_TeamVisualSections[i] && !Q_stricmp( pszString, g_TeamVisualSections[i] ) )
			return i;
	}
	return -1;
}

#if defined(CLIENT_DLL) || defined(GAME_DLL)
// Used to convert strings to ints for wearable animation types
const char *g_WearableAnimTypeStrings[ NUM_WAP_TYPES ] =
{
	"on_spawn",			// WAP_ON_SPAWN,
	"start_building",	// WAP_START_BUILDING,
	"stop_building",	// WAP_STOP_BUILDING,
	"start_taunting",		// WAP_START_TAUNTING,
	"stop_taunting",	// WAP_STOP_TAUNTING,
};
#endif

const char *g_AttributeDescriptionFormats[] =
{
	"value_is_percentage",				// ATTDESCFORM_VALUE_IS_PERCENTAGE,
	"value_is_inverted_percentage",		// ATTDESCFORM_VALUE_IS_INVERTED_PERCENTAGE
	"value_is_additive",				// ATTDESCFORM_VALUE_IS_ADDITIVE
	"value_is_additive_percentage",		// ATTDESCFORM_VALUE_IS_ADDITIVE_PERCENTAGE
	"value_is_or",						// ATTDESCFORM_VALUE_IS_OR
	"value_is_date",					// ATTDESCFORM_VALUE_IS_DATE
	"value_is_account_id",				// ATTDESCFORM_VALUE_IS_ACCOUNT_ID
	"value_is_particle_index",			// ATTDESCFORM_VALUE_IS_PARTICLE_INDEX -> Could change to "string index"
	"value_is_killstreakeffect_index",	// ATTDESCFORM_VALUE_IS_KILLSTREAKEFFECT_INDEX -> Could change to "string index"
	"value_is_killstreak_idleeffect_index",  // ATTDESCFORM_VALUE_IS_KILLSTREAK_IDLEEFFECT_INDEX
	"value_is_item_def",				// ATTDESCFORM_VALUE_IS_ITEM_DEF
	"value_is_from_lookup_table",		// ATTDESCFORM_VALUE_IS_FROM_LOOKUP_TABLE
};

const char *g_EffectTypes[NUM_EFFECT_TYPES] =
{
	"unusual",		// ATTRIB_EFFECT_UNUSUAL,
	"strange",		// ATTRIB_EFFECT_STRANGE,
	"neutral",		// ATTRIB_EFFECT_NEUTRAL = 0,
	"positive",		// ATTRIB_EFFECT_POSITIVE,
	"negative",		// ATTRIB_EFFECT_NEGATIVE,
};

//-----------------------------------------------------------------------------
// Purpose: Set the capabilities bitfield based on whether the entry is true/false.
//-----------------------------------------------------------------------------
const char *g_Capabilities[] =
{
	"paintable",				// ITEM_CAP_PAINTABLE
	"nameable",					// ITEM_CAP_NAMEABLE
	"decodable",				// ITEM_CAP_DECODABLE
	"can_craft_if_purchased",	// ITEM_CAP_CAN_BE_CRAFTED_IF_PURCHASED
	"can_customize_texture",	// ITEM_CAP_CAN_CUSTOMIZE_TEXTURE
	"usable",					// ITEM_CAP_USABLE
	"usable_gc",				// ITEM_CAP_USABLE_GC
	"can_gift_wrap",			// ITEM_CAP_CAN_GIFT_WRAP
	"usable_out_of_game",		// ITEM_CAP_USABLE_OUT_OF_GAME
	"can_collect",				// ITEM_CAP_CAN_COLLECT
	"can_craft_count",			// ITEM_CAP_CAN_CRAFT_COUNT
	"can_craft_mark",			// ITEM_CAP_CAN_CRAFT_MARK
	"paintable_team_colors",	// ITEM_CAP_PAINTABLE_TEAM_COLORS
	"can_be_restored",			// ITEM_CAP_CAN_BE_RESTORED
	"strange_parts",			// ITEM_CAP_CAN_USE_STRANGE_PARTS
	"can_card_upgrade",			// ITEM_CAP_CAN_CARD_UPGRADE
	"can_strangify",			// ITEM_CAP_CAN_STRANGIFY
	"can_killstreakify",		// ITEM_CAP_CAN_KILLSTREAKIFY
	"can_consume",				// ITEM_CAP_CAN_CONSUME_ITEMS
	"can_spell_page",			// ITEM_CAP_CAN_SPELLBOOK_PAGE
	"has_slots",				// ITEM_CAP_HAS_SLOTS
	"duck_upgradable",			// ITEM_CAP_DUCK_UPGRADABLE
	"can_unusualify",			// ITEM_CAP_CAN_UNUSUALIFY
};
COMPILE_TIME_ASSERT( ARRAYSIZE(g_Capabilities) == NUM_ITEM_CAPS );

#define RETURN_ATTRIBUTE_STRING( attrib_name, default_string ) \
	static CSchemaAttributeDefHandle pAttribString( attrib_name ); \
	const char *pchResultAttribString = default_string; \
	FindAttribute_UnsafeBitwiseCast< CAttribute_String >( this, pAttribString, &pchResultAttribString ); \
	return pchResultAttribString;

#define RETURN_ATTRIBUTE_STRING_F( func_name, attrib_name, default_string ) \
	const char *func_name( void ) const { RETURN_ATTRIBUTE_STRING( attrib_name, default_string ) }

static void ParseCapability( item_capabilities_t &capsBitfield, KeyValues* pEntry )
{
	int idx = StringFieldToInt(  pEntry->GetName(), g_Capabilities, ARRAYSIZE(g_Capabilities) );
	if ( idx < 0 )
	{
		return;
	}
	int bit = 1 << idx;
	if ( pEntry->GetBool() )
	{
		(int&)capsBitfield |= bit;
	}
	else
	{
		(int&)capsBitfield &= ~bit;
	}
}


//-----------------------------------------------------------------------------
// Purpose: CEconItemSeriesDefinition
//-----------------------------------------------------------------------------
CEconItemSeriesDefinition::CEconItemSeriesDefinition( void )
	: m_nValue( INT_MAX )
{
}


//-----------------------------------------------------------------------------
// Purpose:	Copy constructor
//-----------------------------------------------------------------------------
CEconItemSeriesDefinition::CEconItemSeriesDefinition( const CEconItemSeriesDefinition &that )
{
	( *this ) = that;
}


//-----------------------------------------------------------------------------
// Purpose:	Operator=
//-----------------------------------------------------------------------------
CEconItemSeriesDefinition &CEconItemSeriesDefinition::operator=( const CEconItemSeriesDefinition &rhs )
{
	m_nValue = rhs.m_nValue;
	m_strName = rhs.m_strName;

	return *this;
}


//-----------------------------------------------------------------------------
// Purpose:	Initialize the quality definition
// Input:	pKVQuality - The KeyValues representation of the quality
//			schema - The overall item schema for this attribute
//			pVecErrors - An optional vector that will contain error messages if 
//				the init fails.
// Output:	True if initialization succeeded, false otherwise
//-----------------------------------------------------------------------------
bool CEconItemSeriesDefinition::BInitFromKV( KeyValues *pKVSeries, CUtlVector<CUtlString> *pVecErrors /* = NULL */ )
{

	m_nValue = pKVSeries->GetInt( "value", -1 );
	m_strName = pKVSeries->GetName();
	
	m_strLockKey = pKVSeries->GetString( "loc_key" );
	m_strUiFile = pKVSeries->GetString( "ui" );

	// Check for required fields
	SCHEMA_INIT_CHECK(
		NULL != pKVSeries->FindKey( "value" ),
		"Quality definition %s: Missing required field \"value\"", pKVSeries->GetName() );

	return SCHEMA_INIT_SUCCESS();
}


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CEconItemQualityDefinition::CEconItemQualityDefinition( void )
:	m_nValue( INT_MAX )
,	m_bCanSupportSet( false )
{
}


//-----------------------------------------------------------------------------
// Purpose:	Copy constructor
//-----------------------------------------------------------------------------
CEconItemQualityDefinition::CEconItemQualityDefinition( const CEconItemQualityDefinition &that )
{
	(*this) = that;
}


//-----------------------------------------------------------------------------
// Purpose:	Operator=
//-----------------------------------------------------------------------------
CEconItemQualityDefinition &CEconItemQualityDefinition::operator=( const CEconItemQualityDefinition &rhs )
{
	m_nValue = rhs.m_nValue; 
	m_strName =	rhs.m_strName; 
	m_bCanSupportSet = rhs.m_bCanSupportSet;

	return *this;
}


//-----------------------------------------------------------------------------
// Purpose:	Initialize the quality definition
// Input:	pKVQuality - The KeyValues representation of the quality
//			schema - The overall item schema for this attribute
//			pVecErrors - An optional vector that will contain error messages if 
//				the init fails.
// Output:	True if initialization succeeded, false otherwise
//-----------------------------------------------------------------------------
bool CEconItemQualityDefinition::BInitFromKV( KeyValues *pKVQuality, CUtlVector<CUtlString> *pVecErrors /* = NULL */ )
{

	m_nValue = pKVQuality->GetInt( "value", -1 );
	m_strName = pKVQuality->GetName();	
	m_bCanSupportSet = pKVQuality->GetBool( "canSupportSet" );

	// Check for required fields
	SCHEMA_INIT_CHECK( 
		NULL != pKVQuality->FindKey( "value" ), 
		"Quality definition %s: Missing required field \"value\"", pKVQuality->GetName() );

#if defined(CLIENT_DLL) || defined(GAME_DLL)
	return SCHEMA_INIT_SUCCESS();
#endif // GC_DLL

	// Check for data consistency
	SCHEMA_INIT_CHECK( 
		0 != Q_stricmp( GetName(), "any" ), 
		"Quality definition any: The quality name \"any\" is a reserved keyword and cannot be used." );

	SCHEMA_INIT_CHECK( 
		m_nValue != k_unItemQuality_Any, 
		"Quality definition %s: Invalid value (%d). It is reserved for Any", GetName(), k_unItemQuality_Any );

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// CEconItemRarityDefinition
//-----------------------------------------------------------------------------
CEconItemRarityDefinition::CEconItemRarityDefinition( void )
	: m_nValue( INT_MAX )
	, m_nLootlistWeight( 0 )
{
}

//-----------------------------------------------------------------------------
// Purpose:	Initialize the rarity definition
//-----------------------------------------------------------------------------
bool CEconItemRarityDefinition::BInitFromKV( KeyValues *pKVRarity, KeyValues *pKVRarityWeights, CEconItemSchema &pschema, CUtlVector<CUtlString> *pVecErrors /* = NULL */ )
{
	m_nValue = pKVRarity->GetInt( "value", -1 );
	m_strName = pKVRarity->GetName();
	m_strLocKey = pKVRarity->GetString( "loc_key" );
	m_strWepLocKey = pKVRarity->GetString( "loc_key_weapon" );

	m_iAttribColor = GetAttribColorIndexForName( pKVRarity->GetString( "color" ) );
	m_strDropSound = pKVRarity->GetString( "drop_sound" );
	m_strNextRarity = pKVRarity->GetString( "next_rarity" ); // Not required.

	//

	// Check for required fields
	SCHEMA_INIT_CHECK(
		NULL != pKVRarity->FindKey( "value" ),
		"Rarity definition %s: Missing required field \"value\"", pKVRarity->GetName() );

	SCHEMA_INIT_CHECK(
		NULL != pKVRarity->FindKey( "loc_key" ),
		"Rarity definition %s: Missing required field \"loc_key\"", pKVRarity->GetName() );

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconColorDefinition::BInitFromKV( KeyValues *pKVColor, CUtlVector<CUtlString> *pVecErrors /* = NULL */ )
{
	m_strName		= pKVColor->GetName();
	m_strColorName	= pKVColor->GetString( "color_name" );

	SCHEMA_INIT_CHECK(
		!m_strColorName.IsEmpty(),
		"Quality definition %s: missing \"color_name\"", GetName() );


	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
CEconItemSetDefinition::CEconItemSetDefinition( void )
	: m_strName( NULL )
	, m_pszLocalizedName( NULL )
	, m_iBundleItemDef( INVALID_ITEM_DEF_INDEX )
	, m_bIsHiddenSet( false )
{
}

//-----------------------------------------------------------------------------
// Purpose:	Copy constructor
//-----------------------------------------------------------------------------
CEconItemSetDefinition::CEconItemSetDefinition( const CEconItemSetDefinition &that )
{
	(*this) = that;
}

//-----------------------------------------------------------------------------
// Purpose:	Operator=
//-----------------------------------------------------------------------------
CEconItemSetDefinition &CEconItemSetDefinition::operator=( const CEconItemSetDefinition &other )
{
	m_strName = other.m_strName;
	m_pszLocalizedName = other.m_pszLocalizedName;
	m_iItemDefs = other.m_iItemDefs;
	m_iAttributes = other.m_iAttributes;
	m_iBundleItemDef = other.m_iBundleItemDef;
	m_bIsHiddenSet = other.m_bIsHiddenSet;

	return *this;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CEconItemSetDefinition::BInitFromKV( KeyValues *pKVItemSet, CUtlVector<CUtlString> *pVecErrors )
{
	m_strName = pKVItemSet->GetName();

	m_iBundleItemDef = INVALID_ITEM_DEF_INDEX;
	const char *pszBundleName = pKVItemSet->GetString( "store_bundle" );
	if ( pszBundleName && pszBundleName[0] )
	{
		const CEconItemDefinition *pDef = GetItemSchema()->GetItemDefinitionByName( pszBundleName );
		if ( pDef )
		{
			m_iBundleItemDef = pDef->GetDefinitionIndex();
		}

		SCHEMA_INIT_CHECK( 
			pDef != NULL,
			"Item set %s: Bundle definition \"%s\" was not found", m_strName.Get(), pszBundleName );
	}

	m_pszLocalizedName = pKVItemSet->GetString( "name", NULL );
	m_bIsHiddenSet = pKVItemSet->GetBool( "is_hidden_set", false );

	KeyValues *pKVItems = pKVItemSet->FindKey( "items" );
	if ( pKVItems )
	{
		FOR_EACH_SUBKEY( pKVItems, pKVItem )
		{
			const char *pszName = pKVItem->GetName();

			CEconItemDefinition *pDef = GetItemSchema()->GetItemDefinitionByName( pszName );

			SCHEMA_INIT_CHECK( 
				pDef != NULL,
				"Item set %s: Item definition \"%s\" was not found", m_strName.Get(), pszName );

			const item_definition_index_t unDefIndex = pDef->GetDefinitionIndex();

			SCHEMA_INIT_CHECK(
				!m_iItemDefs.IsValidIndex( m_iItemDefs.Find( unDefIndex ) ),
				"Item set %s: item definition \"%s\" appears multiple times", m_strName.Get(), pszName );
			SCHEMA_INIT_CHECK(
				!pDef->GetItemSetDefinition(),
				"Item set %s: item definition \"%s\" specified in multiple item sets", m_strName.Get(), pszName );

			m_iItemDefs.AddToTail( unDefIndex );
			pDef->SetItemSetDefinition( this );

			// FIXME: hack to work around crafting item criteria
			pDef->GetRawDefinition()->SetString( "item_set", m_strName );
		}
	}

	KeyValues *pKVAttributes = pKVItemSet->FindKey( "attributes" );
	if ( pKVAttributes )
	{
		FOR_EACH_SUBKEY( pKVAttributes, pKVAttribute )
		{
			const char *pszName = pKVAttribute->GetName();

			const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinitionByName( pszName );
			SCHEMA_INIT_CHECK( 
				pAttrDef != NULL,
				"Item set %s: Attribute definition \"%s\" was not found", m_strName.Get(), pszName );
			SCHEMA_INIT_CHECK(
				pAttrDef->BIsSetBonusAttribute(),
				"Item set %s: Attribute definition \"%s\" is not a set bonus attribute", m_strName.Get(), pszName );

			int iIndex = m_iAttributes.AddToTail();
			m_iAttributes[iIndex].m_iAttribDefIndex = pAttrDef->GetDefinitionIndex();
			m_iAttributes[iIndex].m_flValue = pKVAttribute->GetFloat( "value" );
		}
	}

	// Sanity check.
	SCHEMA_INIT_CHECK( m_pszLocalizedName != NULL,
	                   "Item set %s: Set contains no localized name", m_strName.Get() );
	SCHEMA_INIT_CHECK( m_iItemDefs.Count() > 0,
	                   "Item set %s: Set contains no items", m_strName.Get() );

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CEconItemSetDefinition::IterateAttributes( class IEconItemAttributeIterator *pIterator ) const
{
	FOR_EACH_VEC( m_iAttributes, i )
	{
		const itemset_attrib_t& itemsetAttrib = m_iAttributes[i];

		const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinition( itemsetAttrib.m_iAttribDefIndex );
		if ( !pAttrDef )
			continue;

		const ISchemaAttributeType *pAttrType = pAttrDef->GetAttributeType();
		Assert( pAttrType );

		// We know (and assert) that we only need 32 bits of data to store this attribute
		// data. We don't know anything about the type but we'll let the type handle it
		// below.
		attribute_data_union_t value;
		value.asFloat = itemsetAttrib.m_flValue;

		if ( !pAttrType->OnIterateAttributeValue( pIterator, pAttrDef, value ) )
			return;
	}
}

//-----------------------------------------------------------------------------
CEconItemCollectionDefinition::CEconItemCollectionDefinition( void )
	: m_strName( NULL )
	, m_pszLocalizedName( NULL )
	, m_pszLocalizedDesc( NULL )
	, m_iRarityMin( k_unItemRarity_Any )
	, m_iRarityMax( k_unItemRarity_Any )
{
}

//-----------------------------------------------------------------------------
// 

//-----------------------------------------------------------------------------
static int SortCollectionByRarity( item_definition_index_t const *a, item_definition_index_t const *b )
{
	Assert( a );
	Assert( *a );
	Assert( b );
	Assert( *b );

	CEconItemDefinition *pItemA = GetItemSchema()->GetItemDefinition( *a );
	CEconItemDefinition *pItemB = GetItemSchema()->GetItemDefinition( *b );

	if ( !pItemA || !pItemB )
	{
		AssertMsg( 0, "ItemDef Doesn't exist for sorting" );
		return 1;
	}

	bool bIsRarityEqual = ( pItemA->GetRarity() == pItemB->GetRarity() );
	
	// If same Rarity, leave in current position?
	uint32 unPaintKitDefIndexA, unPaintKitDefIndexB;
	if ( bIsRarityEqual && GetPaintKitDefIndex( pItemA, &unPaintKitDefIndexA ) && GetPaintKitDefIndex( pItemB, &unPaintKitDefIndexB ) )
	{
#ifdef CLIENT_DLL
		// Sort by localized name
		// paintkits sort by paintkit name
		const CPaintKitDefinition* pPaintKitDefA = assert_cast< const CPaintKitDefinition* >( GetProtoScriptObjDefManager()->GetDefinition( ProtoDefID_t( DEF_TYPE_PAINTKIT_DEFINITION, unPaintKitDefIndexA ) ) );
		const CPaintKitDefinition* pPaintKitDefB = assert_cast< const CPaintKitDefinition* >( GetProtoScriptObjDefManager()->GetDefinition( ProtoDefID_t( DEF_TYPE_PAINTKIT_DEFINITION, unPaintKitDefIndexB ) ) );

		locchar_t pPaintKitStrA[MAX_ITEM_NAME_LENGTH];
		locchar_t pPaintKitStrB[MAX_ITEM_NAME_LENGTH];

		const wchar_t *wpszFormatString = g_pVGuiLocalize->Find( "#ToolPaintKit_ItemDescFormat" );
		if ( !wpszFormatString )
		{
			wpszFormatString = L"%s1 %s2";
		}
		g_pVGuiLocalize->ConstructString_safe( pPaintKitStrA,
				wpszFormatString,
				2,
				g_pVGuiLocalize->Find( pPaintKitDefA->GetDescriptionToken() ),
				g_pVGuiLocalize->Find( pItemA->GetItemBaseName() ) );

		g_pVGuiLocalize->ConstructString_safe( pPaintKitStrB,
				wpszFormatString,
				2,
				g_pVGuiLocalize->Find( pPaintKitDefB->GetDescriptionToken() ),
				g_pVGuiLocalize->Find( pItemB->GetItemBaseName() ) );

		return V_wcscmp( pPaintKitStrA, pPaintKitStrB );
#else
		return 0;
#endif
	}

	// If same Rarity, leave in current position?
	if ( bIsRarityEqual )
		return 0;

	return ( pItemA->GetRarity() > pItemB->GetRarity() ) ? -1 : 1;
}


//-----------------------------------------------------------------------------
bool CEconItemCollectionDefinition::BInitFromKV( KeyValues *pKVPItemCollection, CUtlVector<CUtlString> *pVecErrors )
{
	m_strName = pKVPItemCollection->GetName();

	m_pszLocalizedName = pKVPItemCollection->GetString( "name", NULL );
	m_pszLocalizedDesc = pKVPItemCollection->GetString( "description", NULL );

	m_bIsReferenceCollection = pKVPItemCollection->GetBool( "is_reference_collection", false );

	KeyValues *pKVItems = pKVPItemCollection->FindKey( "items" );

	// Create a 'lootlist' from this collection
	KeyValues *pCollectionLootList = NULL;
	bool bIsLootList = false;
	if ( !m_bIsReferenceCollection )
	{
		pCollectionLootList = new KeyValues( m_strName );
	}

	if ( pKVItems )
	{
		// Traverse rarity items and set rarity
		// Create a lootlist if applicable
		FOR_EACH_TRUE_SUBKEY( pKVItems, pKVRarity )
		{
			bIsLootList = true;
			// Get the Rarity Value
			const CEconItemRarityDefinition *pRarity = GetItemSchema()->GetRarityDefinitionByName( pKVRarity->GetName() );
			SCHEMA_INIT_CHECK( pRarity != NULL, "Item collection %s: Rarity type \"%s\" was not found", m_strName.Get(), pKVRarity->GetName() );
			
			// Create a lootlist
			if ( !m_bIsReferenceCollection )
			{
				CFmtStr lootlistname( "%s_%s", m_strName.Get(), pRarity->GetName() );
				const char *pszName = V_strdup( lootlistname.Get() );
				pKVRarity->SetInt( "rarity", pRarity->GetDBValue() );
				SCHEMA_INIT_CHECK( GetItemSchema()->BInsertLootlist( pszName, pKVRarity, pVecErrors ), "Invalid collection lootlist %s", pszName );
				KeyValues *pTempRarityKey = pKVRarity->FindKey( "rarity" );
				Assert( pTempRarityKey );
				pKVRarity->RemoveSubKey( pTempRarityKey );
				pTempRarityKey->deleteThis();
				pCollectionLootList->SetInt( pszName, pRarity->GetLootlistWeight() );
			}

			// Items in the Rarity
			FOR_EACH_VALUE( pKVRarity, pKVItem )
			{
				const char *pszName = pKVItem->GetName();

				CEconItemDefinition *pDef = GetItemSchema()->GetItemDefinitionByName( pszName );

				SCHEMA_INIT_CHECK(
					pDef != NULL,
					"Item set %s: Item definition \"%s\" was not found", m_strName.Get(), pszName );

				const item_definition_index_t unDefIndex = pDef->GetDefinitionIndex();

				SCHEMA_INIT_CHECK(
					!m_iItemDefs.IsValidIndex( m_iItemDefs.Find( unDefIndex ) ),
					"Item Collection %s: item definition \"%s\" appears multiple times", m_strName.Get(), pszName );

				m_iItemDefs.AddToTail( unDefIndex );

				// Collection Reference
				if ( !m_bIsReferenceCollection )
				{
					SCHEMA_INIT_CHECK(
						!pDef->GetItemCollectionDefinition(),
						"Item Collection %s: item definition \"%s\" specified in multiple item sets", m_strName.Get(), pszName );
					pDef->SetItemCollectionDefinition( this );
				}

				// Item Rarity
				pDef->SetRarity( pRarity->GetDBValue() );
			}
		}

		// Loose Items
		FOR_EACH_VALUE( pKVItems, pKVItem )
		{
			const char *pszName = pKVItem->GetName();

			CEconItemDefinition *pDef = GetItemSchema()->GetItemDefinitionByName( pszName );

			SCHEMA_INIT_CHECK(
				pDef != NULL,
				"Item set %s: Item definition \"%s\" was not found", m_strName.Get(), pszName );

			const item_definition_index_t unDefIndex = pDef->GetDefinitionIndex();

			SCHEMA_INIT_CHECK(
				!m_iItemDefs.IsValidIndex( m_iItemDefs.Find( unDefIndex ) ),
				"Item Collection %s: item definition \"%s\" appears multiple times", m_strName.Get(), pszName );
			
			m_iItemDefs.AddToTail( unDefIndex );

			if ( !m_bIsReferenceCollection )
			{
				SCHEMA_INIT_CHECK(
					!pDef->GetItemCollectionDefinition(),
					"Item Collection %s: item definition \"%s\" specified in multiple item sets", m_strName.Get(), pszName );
				pDef->SetItemCollectionDefinition( this );
			}
		}
	}

	if ( !m_bIsReferenceCollection && bIsLootList )
	{
		// Insert collection lootlist
		GetItemSchema()->BInsertLootlist( m_strName, pCollectionLootList, pVecErrors );
	}

	if ( pCollectionLootList )
	{
		pCollectionLootList->deleteThis();
	}

	// Sanity check.
	SCHEMA_INIT_CHECK( m_pszLocalizedName != NULL,
		"Item Collection %s: Collection contains no localized name", m_strName.Get() );
	SCHEMA_INIT_CHECK( m_pszLocalizedDesc != NULL,
		"Item Collection %s: Collection contains no localized description", m_strName.Get() );
	SCHEMA_INIT_CHECK( m_iItemDefs.Count() > 0,
		"Item Collection %s: Collection contains no items", m_strName.Get() );

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
bool CEconItemCollectionDefinition::BPostSchemaInit( CUtlVector<CUtlString> *pVecErrors )
{
	// Sort by Rarity
	m_iItemDefs.Sort( &SortCollectionByRarity );

	// Sorted high to low
	m_iRarityMax = GetItemSchema()->GetItemDefinition( m_iItemDefs[ 0 ] )->GetRarity();
	m_iRarityMin = GetItemSchema()->GetItemDefinition( m_iItemDefs[ m_iItemDefs.Count() - 1] )->GetRarity();
	// Verify that there is no gaps in the Rarity (would cause crafting problems and makes no sense)

	if ( !m_bIsReferenceCollection )
	{
		int iRarityVerify = m_iRarityMax;
		FOR_EACH_VEC( m_iItemDefs, i )
		{
			int iNextRarity = GetItemSchema()->GetItemDefinition( m_iItemDefs[i] )->GetRarity();
			SCHEMA_INIT_CHECK( iRarityVerify - iNextRarity <= 1, "Items in Collection %s: Have a gap in rarity tiers", m_strName.Get() );
			iRarityVerify = iNextRarity;
		}
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// CEconOperationDefinition
//-----------------------------------------------------------------------------
CEconOperationDefinition::CEconOperationDefinition( void )
	: m_pszName( NULL )
	, m_unRequiredItemDefIndex( INVALID_ITEM_DEF_INDEX )
	, m_unGatewayItemDefIndex( INVALID_ITEM_DEF_INDEX )
{
}

//-----------------------------------------------------------------------------
CEconOperationDefinition::~CEconOperationDefinition( void )
{
	if ( m_pKVItem )
		m_pKVItem->deleteThis();
	m_pKVItem = NULL;
}

//-----------------------------------------------------------------------------
bool CEconOperationDefinition::BInitFromKV( KeyValues *pKVPOperation, CUtlVector<CUtlString> *pVecErrors )
{
	m_unOperationID = V_atoi( pKVPOperation->GetName() );

	m_pszName = pKVPOperation->GetString( "name", NULL );
	SCHEMA_INIT_CHECK( m_pszName != NULL, "OperationDefinition does not have 'name'" );

	// initialize required item def index if we specified one
	const char *pszRequiredName = pKVPOperation->GetString( "required_item_name", NULL );
	if ( pszRequiredName )
	{
		const CEconItemDefinition *pDef = GetItemSchema()->GetItemDefinitionByName( pszRequiredName );
		SCHEMA_INIT_CHECK( pDef != NULL, "OperationDefinition couldn't find item def from required name '%s'", pszRequiredName );

		m_unRequiredItemDefIndex = pDef->GetDefinitionIndex();
	}

	const char *pszGatewayItemName = pKVPOperation->GetString( "gateway_item_name", NULL );
	if ( pszGatewayItemName )
	{
		const CEconItemDefinition *pDef = GetItemSchema()->GetItemDefinitionByName( pszGatewayItemName );
		SCHEMA_INIT_CHECK( pDef != NULL, "OperationDefinition couldn't find item def from gateway name '%s'", pszGatewayItemName );

		m_unGatewayItemDefIndex = pDef->GetDefinitionIndex();
	}

	if ( m_unGatewayItemDefIndex != INVALID_ITEM_DEF_INDEX )
	{
		SCHEMA_INIT_CHECK( m_unRequiredItemDefIndex != INVALID_ITEM_DEF_INDEX, "If a gateway item is set, a required item must be set!  Mismatch in %d", m_unOperationID );
	}

	auto lambdaGetTime = [ &pKVPOperation, this, &pVecErrors ]( const char* pszKey ) -> RTime32
	{
		const char *pszTime = pKVPOperation->GetString( pszKey, NULL );
		SCHEMA_INIT_CHECK( pszTime != NULL, "OperationDefinition %s does not have '%s'", m_pszName, pszKey );
		return ( pszTime && pszTime[0] )	? CRTime::RTime32FromFmtString( "YYYY-MM-DD hh:mm:ss" , pszTime )
											: RTime32(0);
	};

	m_OperationStartDate		= lambdaGetTime( "operation_start_date" );
	m_StopGivingToPlayerDate	= lambdaGetTime( "stop_giving_to_player_date" );
	m_StopAddingToQueueDate		= lambdaGetTime( "stop_adding_to_queue_date" );
	m_ContractProgressEndDate	= lambdaGetTime( "contracts_end_date" );

	m_pszQuestLogResFile	= pKVPOperation->GetString( "quest_log_res_file", NULL );
	m_pszQuestListResFile	= pKVPOperation->GetString( "quest_list_res_file", NULL );

	m_pszOperationLootList	= pKVPOperation->GetString( "operation_lootlist", NULL );
	m_bUsesCredits			= pKVPOperation->GetBool( "uses_credits" );
	m_nKillEaterEventType_Contracts	= pKVPOperation->GetInt( "killeater_event_contracts", -1 );
	m_nKillEaterEventType_Points = pKVPOperation->GetInt( "killeater_event_points", -1 );

	SCHEMA_INIT_CHECK( ( !!m_pszOperationLootList || m_bUsesCredits ), "Operation %s does not specify a lootlist or that it uses credits.", GetName() );

	m_bIsCampaign			= pKVPOperation->GetBool( "is_campaign" );
	m_unMaxDropCount		= pKVPOperation->GetInt( "max_drop_count" );


	m_pKVItem = pKVPOperation->MakeCopy();

	return SCHEMA_INIT_SUCCESS();
}


bool BAddLootlistJobFromTemplates( const char *pszContext, CUtlVector<CLootlistJob*> &out_jobs, KeyValues *pLootlistJobKV, const CEconItemSchema *pschema, CUtlVector<CUtlString> *pVecErrors = NULL )
{
	if ( !pLootlistJobKV )
		return true;

	FOR_EACH_SUBKEY( pLootlistJobKV, pTemplateKV )
	{
		if ( pTemplateKV->GetBool() == false )
			continue;

		const char *pszJobName = pTemplateKV->GetName();

		// try to find attr by template name
		CLootlistJob *pJobTemplate = pschema->GetLootlistJobTemplateByName( pszJobName );
	
		SCHEMA_INIT_CHECK(
			NULL != pJobTemplate,
			"Context %s: Couldn't find CLootlistJob '%s' from lootlist_job_templates", pszContext, pszJobName );

		// create a copy of the template and add to the list
		CLootlistJob *pJob = new CLootlistJob( pszContext );
		*pJob = *pJobTemplate;
		out_jobs.AddToTail( pJob );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Dtor
//-----------------------------------------------------------------------------
CEconLootListDefinition::~CEconLootListDefinition( void )
{
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
static const char *g_pszDefaultRevolvingLootListHeader = "#Econ_Revolving_Loot_List";

bool CEconLootListDefinition::BInitFromKV( KeyValues *pKVLootList, CEconItemSchema &pschema, CUtlVector<CUtlString> *pVecErrors )
{
	m_strName = pKVLootList->GetName();
	m_pszLootListHeader = g_pszDefaultRevolvingLootListHeader;
	m_pszLootListFooter = NULL;
	m_pszCollectionReference = NULL;
	m_bPublicListContents = true;
	bool bCollectionLootList = false;

	FOR_EACH_SUBKEY( pKVLootList, pKVListItem )
	{
		const char *pszName = pKVListItem->GetName();
		
		if ( !Q_strcmp( pszName, "loot_list_header_desc" ) )
		{
			// Make sure we didn't specify multiple entries.
			SCHEMA_INIT_CHECK(
				g_pszDefaultRevolvingLootListHeader == m_pszLootListHeader,
				"Loot list %s: Multiple header descriptions specified", m_strName.Get() );

			m_pszLootListHeader = pKVListItem->GetString();

			SCHEMA_INIT_CHECK(
				NULL != m_pszLootListHeader,
				"Loot list %s: Invalid header description specified", m_strName.Get() );

			continue;
		}
		else if ( !Q_strcmp( pszName, "loot_list_footer_desc" ) )
		{
			// Make sure we didn't specify multiple entries.
			SCHEMA_INIT_CHECK(
				NULL == m_pszLootListFooter,
				"Loot list %s: Multiple footer descriptions specified", m_strName.Get() );

			m_pszLootListFooter = pKVListItem->GetString();

			SCHEMA_INIT_CHECK(
				NULL != m_pszLootListFooter,
				"Loot list %s: Invalid header description specified", m_strName.Get() );

			continue;
		}
		else if ( !Q_strcmp( pszName, "loot_list_collection" ) )
		{
			// Set name as the collection lootlist name
			pszName = pKVListItem->GetString();
			m_pszCollectionReference = pszName;
			bCollectionLootList = true;
		}
		else if ( !Q_strcmp( pszName, "hide_lootlist" ) )
		{
			m_bPublicListContents = !pKVListItem->GetBool( nullptr, true );
			continue;
		}
		else if ( !Q_strcmp( pszName, "rarity" ) )
		{
			// already parsed up top
			continue;
		}
		else if ( !V_strcmp( pszName, "lootlist_jobs" ) )
		{
			AddLootlistJob( pKVListItem, pschema, pVecErrors );
			continue;
		}
		else if ( !V_strcmp( pszName, "lootlist_job_templates" ) )
		{
			CFmtStr strContext( "Lootlist %s", m_strName.Get() );
			BAddLootlistJobFromTemplates( strContext.Get(), m_jobs, pKVListItem, &pschema, pVecErrors );
			continue;
		}

		int iDef = 0;
		// First, see if we've got a loot list name, for embedded loot lists
		int iIdx = 0;
		if ( GetItemSchema()->GetLootListByName( pszName, &iIdx ) )
		{
			// HACKY: Store loot list indices as negatives, starting from -1, because 0 is a valid item index
			iDef = 0 - 1 - iIdx;
		}
		else
		{
			// Not a loot list. See if it's an item index. Check the first character.
			if ( pszName[0] >= '0' && pszName[0] <= '9' )
			{
				iDef = atoi( pszName );
			}
			else
			{
				// Not a number. See if we can find an item def with a matching name.
				const CEconItemDefinition *pDef = GetItemSchema()->GetItemDefinitionByName( pszName );
				if ( pDef )
				{
					iDef = pDef->GetDefinitionIndex();
				}

				SCHEMA_INIT_CHECK( 
					pDef != NULL,
					"Loot list %s: Item definition \"%s\" was not found", m_strName.Get(), pszName );
			}
		}

		// Default to the start dropping at the start of time and end dropping at the end of time
		drop_period_t dropPeriod = { RTime32(0), ~RTime32(0) };

		// Make sure we never put non-enabled items into loot lists
		if ( iDef > 0 )
		{
			const CEconItemDefinition *pItemDef = GetItemSchema()->GetItemDefinition( iDef );
			SCHEMA_INIT_CHECK( 
				pItemDef != NULL,
				"Loot list %s: Item definition index \"%s\" (%d) was not found", m_strName.Get(), pszName, iDef );

			static CSchemaAttributeDefHandle pAttribDef_StartDropDate( "start drop date" );
			static CSchemaAttributeDefHandle pAttribDef_EndDropDate( "end drop date" );

			CAttribute_String value;
			// Check for start drop date attribute on this item
			if ( FindAttribute( pItemDef, pAttribDef_StartDropDate, &value ) )
			{
				const char* pszStartDate = value.value().c_str();
				dropPeriod.m_DropStartDate = CRTime::RTime32FromFmtString( "YYYY-MM-DD hh:mm:ss" , pszStartDate );

				// Check that if we convert back to a string, we get the same value
				char rtimeBuf[k_RTimeRenderBufferSize];
				SCHEMA_INIT_CHECK(
					Q_strcmp( CRTime::RTime32ToString( dropPeriod.m_DropStartDate, rtimeBuf ), pszStartDate ) == 0,
					"Malformed start drop date \"%s\" for item %s.  Must be of the form \"YYYY-MM-DD hh:mm:ss\"",
					pszStartDate, pItemDef->GetDefinitionName() );
			}

			// Check for end drop date attribute on this item
			if ( FindAttribute( pItemDef, pAttribDef_EndDropDate, &value ) )
			{
				const char* pszEndDate = value.value().c_str();
				dropPeriod.m_DropEndDate = CRTime::RTime32FromFmtString( "YYYY-MM-DD hh:mm:ss" , pszEndDate );

				// Check that if we convert back to a string, we get the same value
				char rtimeBuf[k_RTimeRenderBufferSize];
				SCHEMA_INIT_CHECK(
					Q_strcmp( CRTime::RTime32ToString( dropPeriod.m_DropEndDate, rtimeBuf ), pszEndDate ) == 0,
					"Malformed end drop date \"%s\" for item %s.  Must be of the form \"YYYY-MM-DD hh:mm:ss\"", pszEndDate, pItemDef->GetDefinitionName() );
			}

		}

		float fItemWeight = 0.f;

		// Add this item
		drop_item_t dropItem = { iDef, fItemWeight, dropPeriod };
		m_DropList.AddToTail( dropItem );
	}


	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconLootListDefinition::BPostInit( CUtlVector<CUtlString> *pVecErrors )
{

	return SCHEMA_INIT_SUCCESS();
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconLootListDefinition::AddLootlistJob( KeyValues *pLootlistJobKV, CEconItemSchema &pschema, CUtlVector<CUtlString> *pVecErrors /*= NULL*/ )
{
	const char *pszJobName = pLootlistJobKV->GetName();

	// We've found the lootlist_jobs block. Parse it.
	CLootlistJob *pJob = pschema.CreateLootlistJob( m_strName, pLootlistJobKV, pVecErrors );
	
	SCHEMA_INIT_CHECK(
		NULL != pJob,
		"Loot List %s: Failed to create CLootlistJob '%s'", m_strName.Get(), pszJobName );

	m_jobs.AddToTail( pJob );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool lootlist_attrib_t::BInitFromKV( const char *pszContext, KeyValues *pKVKey, CEconItemSchema &pschema, CUtlVector<CUtlString> *pVecErrors )
{
	SCHEMA_INIT_SUBSTEP( m_staticAttrib.BInitFromKV_MultiLine( pszContext, pKVKey, pVecErrors ) );


	m_flWeight = pKVKey->GetFloat( "weight" );

	return SCHEMA_INIT_SUCCESS();
}


//-----------------------------------------------------------------------------
// Purpose: check if an item pass all criterias
//-----------------------------------------------------------------------------
bool lootlist_attrib_t::BItemPassAllCriteria( const CEconItemDefinition* pItemDef ) const
{
	// we should check HasCriterias before calling this function
	Assert( m_pVecCriteria );
	if ( !m_pVecCriteria )
		return true;

	for ( int iCriteria=0; iCriteria<m_pVecCriteria->Count(); ++iCriteria )
	{
		const CItemSelectionCriteria *pCriteria = m_pVecCriteria->Element( iCriteria );
		if ( !pCriteria->BEvaluate( pItemDef ) )
		{
			return false;
		}
	}

	return true;
}


CLootlistJob::CLootlistJob( const char *pszOwnerName )
	: m_pszOwnerName( pszOwnerName )
{
}

CLootlistJob::~CLootlistJob()
{
	// make sure we clean up custom attributes that are not from templates
	FOR_EACH_VEC( m_vecAttributes, i )
	{
		if ( !m_vecAttributes[i].m_bFromTemplate )
		{
			delete m_vecAttributes[i].m_pRandomAttributes;
			m_vecAttributes[i].m_pRandomAttributes = NULL;
		}
	}
}

bool CLootlistJob::BInitFromKV( const char *pszContext, KeyValues *pKVKey, CEconItemSchema &pschema, CUtlVector<CUtlString> *pVecErrors )
{

	m_flChanceToRunJob = pKVKey->GetFloat( "chance" );


	FOR_EACH_TRUE_SUBKEY( pKVKey, pSubKey )
	{
		const char *pszName = pSubKey->GetName();

		if ( !V_strcmp( pszName, "random_attributes" ) )
		{
			AddRandomAtrributes( pSubKey, pschema, pVecErrors );
			continue;
		}
		else if ( !V_strcmp( pszName, "attribute_templates" ) )
		{
			FOR_EACH_SUBKEY( pSubKey, pKVAttributeTemplate )
			{
				if ( pKVAttributeTemplate->GetInt() == 0 )
					continue;

				bool bAdded = AddRandomAttributesFromTemplates( pKVAttributeTemplate, pschema, pVecErrors );
				SCHEMA_INIT_CHECK( bAdded, "%s: Failed to attribute_templates '%s'", m_pszOwnerName, pKVAttributeTemplate->GetName() );
			}

			continue;
		}
		else if ( !V_strcmp( pszName, "additional_drop" ) )
		{
			bool		bPremiumOnly	   = pSubKey->GetBool( "premium_only", false );
			const char *pszLootList		   = pSubKey->GetString( "loot_list", "" );
			const char *pszRequiredHoliday = pSubKey->GetString( "required_holiday", NULL );
			const char *pszDropPerdiodStartDate = pSubKey->GetString( "start_date", NULL );
			const char *pszDropPerdiodEndDate	= pSubKey->GetString( "end_date", NULL );

			int iRequiredHolidayIndex = pszRequiredHoliday
									  ? EconHolidays_GetHolidayForString( pszRequiredHoliday )
									  : kHoliday_None;

			RTime32 dropStartDate = ( pszDropPerdiodStartDate && pszDropPerdiodStartDate[0] )
							? CRTime::RTime32FromFmtString( "YYYY-MM-DD hh:mm:ss" , pszDropPerdiodStartDate )
							: RTime32(0);	// Default to the start of time

			// Check that if we convert back to a string, we get the same value
			char rtimeBuf[k_RTimeRenderBufferSize];
			SCHEMA_INIT_CHECK(
				pszDropPerdiodStartDate == NULL || Q_strcmp( CRTime::RTime32ToString( dropStartDate, rtimeBuf ), pszDropPerdiodStartDate ) == 0,
				"Malformed start drop date \"%s\" for additional_drop in lootlist %s.  Must be of the form \"YYYY-MM-DD hh:mm:ss\"", pszDropPerdiodStartDate, m_pszOwnerName );


			RTime32 dropEndDate = ( pszDropPerdiodEndDate && pszDropPerdiodEndDate[0] )
						  ? CRTime::RTime32FromFmtString( "YYYY-MM-DD hh:mm:ss" , pszDropPerdiodEndDate )
						  : ~RTime32(0);	// Default to the end of time

			// Check that if we convert back to a string, we get the same value
			SCHEMA_INIT_CHECK(
				pszDropPerdiodEndDate == NULL || Q_strcmp( CRTime::RTime32ToString( dropEndDate, rtimeBuf ), pszDropPerdiodEndDate ) == 0,
				"Malformed end drop date \"%s\" for additional_drop in lootlist %s.  Must be of the form \"YYYY-MM-DD hh:mm:ss\"", pszDropPerdiodEndDate, m_pszOwnerName );

			SCHEMA_INIT_CHECK(
				pszLootList && pszLootList[0],
				"Loot list %s: Missing \"additional_drop\" loot list name", m_pszOwnerName );

			SCHEMA_INIT_CHECK(
				(pszRequiredHoliday == NULL) == (iRequiredHolidayIndex == kHoliday_None),
				"Loot list %s: Unknown or missing holiday \"%s\"", m_pszOwnerName, pszRequiredHoliday ? pszRequiredHoliday : "(null)" );

			if ( pszLootList )
			{
				// just add it. we'll do validation check on PostInit
				drop_period_t dropPeriod = { dropStartDate, dropEndDate };
				loot_list_additional_drop_t additionalDrop = { bPremiumOnly, m_pszOwnerName, pszLootList, iRequiredHolidayIndex, dropPeriod };
				m_vecAdditionalDrops.AddToTail( additionalDrop );
			}
			continue;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CLootlistJob::BPostInit( CUtlVector<CUtlString> *pVecErrors )
{
	// all lootlists in additional drops must be valid
	FOR_EACH_VEC( m_vecAdditionalDrops, i )
	{
		const char *pszLootList = m_vecAdditionalDrops[i].m_pszLootListDefName;
		const CEconLootListDefinition *pLootListDef = GetItemSchema()->GetLootListByName( pszLootList );
		SCHEMA_INIT_CHECK(
			pLootListDef != NULL,
			"Loot list %s: Invalid \"additional_drop\" loot list \"%s\"", m_pszOwnerName, pszLootList );
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconLootListDefinition::EnumerateUserFacingPotentialDrops( IEconLootListIterator *pIt ) const
{
	Assert( pIt );

	// Loot lists have the option of specifying that their contents should not be publicly
	// listed. This is used on the GC for things like the "rare item drop list" to prevent
	// every single potentially-unusual hat from showing up.
	if ( !BPublicListContents() )
		return;

	FOR_EACH_VEC( GetLootListContents(), i )
	{
		const int iID = GetLootListContents()[i].m_iItemOrLootlistDef;

		// Nested loot lists are stored as negative indices.
		if ( iID < 0 )
		{
			const CEconLootListDefinition *pLootListDef = GetItemSchema()->GetLootListByIndex( (-iID) - 1 );
			if ( !pLootListDef )
				continue;

			pLootListDef->EnumerateUserFacingPotentialDrops( pIt );
		}
		else
		{
			pIt->OnIterate( iID );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CLootlistJob::AddRandomAtrributes( KeyValues *pRandomAttributesKV, CEconItemSchema &pschema, CUtlVector<CUtlString> *pVecErrors /*= NULL*/ )
{
	const char *pszAttrName = pRandomAttributesKV->GetName();

	// We've found the random attribute block. Parse it.
	random_attrib_t *pRandomAttr = pschema.CreateRandomAttribute( m_pszOwnerName, pRandomAttributesKV, pVecErrors );
	
	SCHEMA_INIT_CHECK(
		NULL != pRandomAttr,
		"Loot List %s: Failed to create random_attrib_t '%s'", m_pszOwnerName, pszAttrName );

	m_vecAttributes.AddToTail( RandomAttributeInfo_t{ pRandomAttr, false } );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CLootlistJob::AddRandomAttributesFromTemplates( KeyValues *pRandomAttributesKV, CEconItemSchema &pschema, CUtlVector<CUtlString> *pVecErrors /*= NULL*/ )
{
	const char *pszAttrName = pRandomAttributesKV->GetName();

	// try to find attr by template name
	random_attrib_t *pRandomAttrTemplate = pschema.GetRandomAttributeTemplateByName( pszAttrName );
	
	SCHEMA_INIT_CHECK(
		NULL != pRandomAttrTemplate,
		"Loot List %s: Couldn't find random_attrib_t '%s' from attribute_templates", m_pszOwnerName, pszAttrName );

	// craete a copy of the template and add to the list
	random_attrib_t *pRandomAttr = new random_attrib_t;
	*pRandomAttr = *pRandomAttrTemplate;
	m_vecAttributes.AddToTail( RandomAttributeInfo_t{ pRandomAttr, true } );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool GetClientLootListInfo( const CEconLootListDefinition *pLootList, LootListInfo_t &lootListInfo )
{
	if ( !pLootList )
		return true;

	class CLootListItemsIterator : public IEconLootList::IEconLootListIterator
	{
	public:
		virtual void OnIterate( item_definition_index_t unItemDefIndex ) OVERRIDE
		{
			if ( m_vecItems.Find( unItemDefIndex ) == m_vecItems.InvalidIndex() )
			{
				m_vecItems.AddToTail( unItemDefIndex );
			}
		}

		CUtlVector< item_definition_index_t > m_vecItems;
	};

	CLootListItemsIterator itr;
	pLootList->EnumerateUserFacingPotentialDrops( &itr );

	// copy items from itr
	lootListInfo.m_vecItems.AddVectorToTail( itr.m_vecItems );

	const auto& jobs = pLootList->GetLootlistJobs();
	FOR_EACH_VEC( jobs, iJob )
	{
		CLootlistJob *pJob = jobs[iJob];

		const auto& attributes = pJob->GetAttributes();
		FOR_EACH_VEC( attributes, iAttr )
		{
			lootListInfo.m_vecAttributes.AddToTail( attributes[iAttr].m_pRandomAttributes );
		}

		const auto& additionalDrops = pJob->GetAdditionalDrops();
		FOR_EACH_VEC( additionalDrops, iDrop )
		{
			LootListInfo_t tempAdditionalDrop;
			const char *pszAdditionalDropLootListName = additionalDrops[iDrop].m_pszLootListDefName;
			GetClientLootListInfo( pszAdditionalDropLootListName, tempAdditionalDrop );
			// add all unique item to additional item list
			FOR_EACH_VEC( tempAdditionalDrop.m_vecItems, iTemp )
			{
				item_definition_index_t tempDefIndex = tempAdditionalDrop.m_vecItems[iTemp];
				if ( lootListInfo.m_vecAdditionalItems.Find( tempDefIndex ) == lootListInfo.m_vecAdditionalItems.InvalidIndex() )
				{
					lootListInfo.m_vecAdditionalItems.AddToTail( tempDefIndex );
				}
			}
			FOR_EACH_VEC( tempAdditionalDrop.m_vecAdditionalItems, iTemp )
			{
				item_definition_index_t tempDefIndex = tempAdditionalDrop.m_vecAdditionalItems[iTemp];
				if ( lootListInfo.m_vecAdditionalItems.Find( tempDefIndex ) == lootListInfo.m_vecAdditionalItems.InvalidIndex() )
				{
					lootListInfo.m_vecAdditionalItems.AddToTail( tempDefIndex );
				}
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool GetClientLootListInfo( const char *pszLootListName, LootListInfo_t &lootListInfo )
{
	const CEconLootListDefinition *pLootList = GetItemSchema()->GetLootListByName( pszLootListName );
	return GetClientLootListInfo( pLootList, lootListInfo );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool GetClientLootListInfo( const IEconItemInterface *pEconItem, LootListInfo_t &lootListInfo )
{
	// global jobs from item definition
	const CEconItemDefinition *pEconItemDef = pEconItem->GetItemDefinition();
	if ( pEconItemDef )
	{
		const auto& jobs = pEconItemDef->GetLootlistJobs();
		FOR_EACH_VEC( jobs, iJob )
		{
			CLootlistJob *pJob = jobs[iJob];

			const auto& attributes = pJob->GetAttributes();
			FOR_EACH_VEC( attributes, iAttr )
			{
				lootListInfo.m_vecAttributes.AddToTail( attributes[iAttr].m_pRandomAttributes );
			}

			// no need to include global additional drops info?
			/*const auto& additionalDrops = pJob->GetAdditionalDrops();
			FOR_EACH_VEC( additionalDrops, iDrop )
			{
				LootListInfo_t *pAdditionalDrop = lootListInfo.m_vecAdditionalItems.AddToTailGetPtr();
				const char *pszAdditionalDropLootListName = additionalDrops[iDrop].m_pszLootListDefName;
				GetClientLootListInfo( pszAdditionalDropLootListName, *pAdditionalDrop );
			}*/
		}
	}

	// copy items from itr
	CCrateLootListWrapper LootListWrapper( pEconItem );
	const CEconLootListDefinition *pLootList = dynamic_cast< const CEconLootListDefinition* >( LootListWrapper.GetEconLootList() );
	return GetClientLootListInfo( pLootList, lootListInfo );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
/*static*/ CSchemaAttributeDefHandle CAttributeLineItemLootList::s_pAttrDef_RandomDropLineItems[] =
{
	CSchemaAttributeDefHandle( "random drop line item 0" ),
	CSchemaAttributeDefHandle( "random drop line item 1" ),
	CSchemaAttributeDefHandle( "random drop line item 2" ),
	CSchemaAttributeDefHandle( "random drop line item 3" ),
};

CSchemaAttributeDefHandle CAttributeLineItemLootList::s_pAttrDef_RandomDropLineItemFooterDesc( "random drop line item footer desc" );	

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAttributeLineItemLootList::EnumerateUserFacingPotentialDrops( IEconLootListIterator *pIt ) const
{
	Assert( pIt );
	
	for ( int i = 0; i < ARRAYSIZE( s_pAttrDef_RandomDropLineItems ); i++ )
	{
		uint32 unItemDef;
		COMPILE_TIME_ASSERT( sizeof( unItemDef ) >= sizeof( item_definition_index_t ) );

		// If we run out of attributes we have set we're done.
		if ( !m_pEconItem->FindAttribute( s_pAttrDef_RandomDropLineItems[i], &unItemDef ) )
			break;

		pIt->OnIterate( unItemDef );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CAttributeLineItemLootList::GetLootListHeaderLocalizationKey() const
{
	return g_pszDefaultRevolvingLootListHeader;
}

//-----------------------------------------------------------------------------
const char *CAttributeLineItemLootList::GetLootListFooterLocalizationKey() const
{
	CAttribute_String sFooter;
	const char* pszFooter = NULL;
	if ( FindAttribute_UnsafeBitwiseCast<CAttribute_String>( m_pEconItem, s_pAttrDef_RandomDropLineItemFooterDesc, &pszFooter ) )
	{
		return pszFooter;
	}
	return NULL;
}
//-----------------------------------------------------------------------------
const char *CAttributeLineItemLootList::GetLootListCollectionReference() const
{
	// TODO : Implement me!
	return NULL;
}
//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
const CEconLootListDefinition* CEconItemSchema::GetLootListByName( const char* pListName, int *out_piIndex ) const
{
	auto idx = m_dictLootLists.Find( pListName );
	if ( !m_dictLootLists.IsValidIndex( idx ) )
		return NULL;

	if ( out_piIndex )
	{
		*out_piIndex = idx;
	}

	return m_dictLootLists[idx];
}


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CEconCraftingRecipeDefinition::CEconCraftingRecipeDefinition( void )
	: m_nDefIndex( 0 )
{
}

//-----------------------------------------------------------------------------
// Purpose:	Initialize the attribute definition
// Input:	pKVAttribute - The KeyValues representation of the attribute
//			schema - The overall item schema for this attribute
//			pVecErrors - An optional vector that will contain error messages if 
//				the init fails.
// Output:	True if initialization succeeded, false otherwise
//-----------------------------------------------------------------------------
bool CEconCraftingRecipeDefinition::BInitFromKV( KeyValues *pKVRecipe, CUtlVector<CUtlString> *pVecErrors /* = NULL */ )
{
	m_nDefIndex = Q_atoi( pKVRecipe->GetName() );
	
	// Check for required fields
	SCHEMA_INIT_CHECK( 
		NULL != pKVRecipe->FindKey( "input_items" ), 
		"Recipe definition %d: Missing required field \"input_items\"", m_nDefIndex );

	SCHEMA_INIT_CHECK( 
		NULL != pKVRecipe->FindKey( "output_items" ), 
		"Recipe definition %d: Missing required field \"output_items\"", m_nDefIndex );

	m_bDisabled = pKVRecipe->GetBool( "disabled" );
	m_strName = pKVRecipe->GetString( "name" );	
	m_strN_A = pKVRecipe->GetString( "n_A" );	 
	m_strDescInputs = pKVRecipe->GetString( "desc_inputs" );	
	m_strDescOutputs = pKVRecipe->GetString( "desc_outputs" );	 
	m_strDI_A = pKVRecipe->GetString( "di_A" );	 
	m_strDI_B = pKVRecipe->GetString( "di_B" );	 
	m_strDI_C = pKVRecipe->GetString( "di_C" );	 
	m_strDO_A = pKVRecipe->GetString( "do_A" );	 
	m_strDO_B = pKVRecipe->GetString( "do_B" );	 
	m_strDO_C = pKVRecipe->GetString( "do_C" );	 

	m_bRequiresAllSameClass = pKVRecipe->GetBool( "all_same_class" );
	m_bRequiresAllSameSlot = pKVRecipe->GetBool( "all_same_slot" );
	m_iCacheClassUsageForOutputFromItem = pKVRecipe->GetInt( "add_class_usage_to_output", -1 );
	m_iCacheSlotUsageForOutputFromItem = pKVRecipe->GetInt( "add_slot_usage_to_output", -1 );
	m_iCacheSetForOutputFromItem = pKVRecipe->GetInt( "add_set_to_output", -1 );
	m_bPremiumAccountOnly = pKVRecipe->GetBool( "premium_only", false );
	m_iCategory = (recipecategories_t)StringFieldToInt( pKVRecipe->GetString("category"), g_szRecipeCategoryStrings, ARRAYSIZE(g_szRecipeCategoryStrings) );

	// Read in all the input items
	KeyValues *pKVInputItems = pKVRecipe->FindKey( "input_items" );
	if ( NULL != pKVInputItems )
	{
		FOR_EACH_TRUE_SUBKEY( pKVInputItems, pKVInputItem )
		{
			int index = m_InputItemsCriteria.AddToTail();
			SCHEMA_INIT_SUBSTEP( m_InputItemsCriteria[index].BInitFromKV( pKVInputItem ) );

			// Recipes ignore the enabled flag when generating items
			m_InputItemsCriteria[index].SetIgnoreEnabledFlag( true );

			index = m_InputItemDupeCounts.AddToTail();
			m_InputItemDupeCounts[index] = atoi( pKVInputItem->GetName() );
		}
	}

	// Read in all the output items
	KeyValues *pKVOutputItems = pKVRecipe->FindKey( "output_items" );
	if ( NULL != pKVOutputItems )
	{
		FOR_EACH_TRUE_SUBKEY( pKVOutputItems, pKVOutputItem )
		{
			int index = m_OutputItemsCriteria.AddToTail();
			SCHEMA_INIT_SUBSTEP( m_OutputItemsCriteria[index].BInitFromKV( pKVOutputItem ) );

			// Recipes ignore the enabled flag when generating items
			m_OutputItemsCriteria[index].SetIgnoreEnabledFlag( true );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}


//-----------------------------------------------------------------------------
// Purpose: Serializes the criteria to and from messages
//-----------------------------------------------------------------------------
bool CEconCraftingRecipeDefinition::BSerializeToMsg( CSOItemRecipe & msg ) const
{
	msg.set_def_index( m_nDefIndex );
	msg.set_name( m_strName );
	msg.set_n_a( m_strN_A );
	msg.set_desc_inputs( m_strDescInputs );
	msg.set_desc_outputs( m_strDescOutputs );
	msg.set_di_a( m_strDI_A );
	msg.set_di_b( m_strDI_B );
	msg.set_di_c( m_strDI_C );
	msg.set_do_a( m_strDO_A );
	msg.set_do_b( m_strDO_B );
	msg.set_do_c( m_strDO_C );
	msg.set_requires_all_same_class( m_bRequiresAllSameClass );
	msg.set_requires_all_same_slot( m_bRequiresAllSameSlot );
	msg.set_class_usage_for_output( m_iCacheClassUsageForOutputFromItem );
	msg.set_slot_usage_for_output( m_iCacheSlotUsageForOutputFromItem );
	msg.set_set_for_output( m_iCacheSetForOutputFromItem );

	FOR_EACH_VEC( m_InputItemsCriteria, i )
	{
		CSOItemCriteria *pCrit = msg.add_input_items_criteria();
		if ( !m_InputItemsCriteria[i].BSerializeToMsg( *pCrit ) )
			return false;
	}

	FOR_EACH_VEC( m_InputItemDupeCounts, i )
	{
		msg.add_input_item_dupe_counts( m_InputItemDupeCounts[i] );
	}

	FOR_EACH_VEC( m_OutputItemsCriteria, i )
	{
		CSOItemCriteria *pCrit = msg.add_output_items_criteria();
		if ( !m_OutputItemsCriteria[i].BSerializeToMsg( *pCrit ) )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Serializes the criteria to and from messages
//-----------------------------------------------------------------------------
bool CEconCraftingRecipeDefinition::BDeserializeFromMsg( const CSOItemRecipe & msg )
{
	m_nDefIndex = msg.def_index();
	m_strName = msg.name().c_str();
	m_strN_A = msg.n_a().c_str();
	m_strDescInputs = msg.desc_inputs().c_str();
	m_strDescOutputs = msg.desc_outputs().c_str();
	m_strDI_A = msg.di_a().c_str();
	m_strDI_B = msg.di_b().c_str();
	m_strDI_C = msg.di_c().c_str();
	m_strDO_A = msg.do_a().c_str();
	m_strDO_B = msg.do_b().c_str();
	m_strDO_C = msg.do_c().c_str();

	m_bRequiresAllSameClass = msg.requires_all_same_class();
	m_bRequiresAllSameSlot = msg.requires_all_same_slot();
	m_iCacheClassUsageForOutputFromItem = msg.class_usage_for_output();
	m_iCacheSlotUsageForOutputFromItem = msg.slot_usage_for_output();
	m_iCacheSetForOutputFromItem = msg.set_for_output();

	// Read how many input items there are
	uint32 unCount = msg.input_items_criteria_size();
	m_InputItemsCriteria.SetSize( unCount );
	for ( uint32 i = 0; i < unCount; i++ )
	{
		if ( !m_InputItemsCriteria[i].BDeserializeFromMsg( msg.input_items_criteria( i ) ) )
			return false;
	}

	// Read how many input item dupe counts there are
	unCount = msg.input_item_dupe_counts_size();
	m_InputItemDupeCounts.SetSize( unCount );
	for ( uint32 i = 0; i < unCount; i++ )
	{
		m_InputItemDupeCounts[i] = msg.input_item_dupe_counts( i );
	}

	// Read how many output items there are
	unCount = msg.output_items_criteria_size();
	m_OutputItemsCriteria.SetSize( unCount );
	for ( uint32 i = 0; i < unCount; i++ )
	{
		if ( !m_OutputItemsCriteria[i].BDeserializeFromMsg( msg.output_items_criteria( i ) ) )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the vector contains a set of items that matches the inputs for this recipe
//			Note it will fail if the vector contains extra items that aren't needed.
//
//-----------------------------------------------------------------------------
bool CEconCraftingRecipeDefinition::ItemListMatchesInputs( CUtlVector<CEconItem*> *vecCraftingItems, KeyValues *out_pkvCraftParams, bool bIgnoreSlop, CUtlVector<uint64> *vecChosenItems ) const
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
int CEconCraftingRecipeDefinition::GetTotalInputItemsRequired( void ) const
{
	int iCount = 0;
	FOR_EACH_VEC( m_InputItemsCriteria, i )
	{
		if ( m_InputItemDupeCounts[i] )
		{
			iCount += m_InputItemDupeCounts[i];
		}
		else
		{
			iCount++;
		}
	}
	return iCount;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
	#define GC_SCH_REFERENCE( TAttribSchType )

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int Internal_GetAttributeTypeUniqueIdentifierNextValue()
{
	static unsigned int s_unUniqueCounter = 0;

	unsigned int unCounter = s_unUniqueCounter;
	s_unUniqueCounter++;
	return unCounter;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template < GC_SCH_REFERENCE( typename TAttribSchType ) typename TAttribInMemoryType >
class CSchemaAttributeTypeBase : public ISchemaAttributeTypeBase<TAttribInMemoryType>
{
public:
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template < GC_SCH_REFERENCE( typename TAttribSchType ) typename TProtobufValueType >
class CSchemaAttributeTypeProtobufBase : public CSchemaAttributeTypeBase< GC_SCH_REFERENCE( TAttribSchType ) TProtobufValueType >
{
public:
	virtual void ConvertTypedValueToByteStream( const TProtobufValueType& typedValue, ::std::string *out_psBytes ) const OVERRIDE
	{
		DbgVerify( typedValue.SerializeToString( out_psBytes ) );
	}

	virtual void ConvertByteStreamToTypedValue( const ::std::string& sBytes, TProtobufValueType *out_pTypedValue ) const OVERRIDE
	{
		DbgVerify( out_pTypedValue->ParseFromString( sBytes ) );
	}

	virtual bool BConvertStringToEconAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const char *pszValue, union attribute_data_union_t *out_pValue, bool bEnableTerribleBackwardsCompatibilitySchemaParsingCode ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_pValue );
		
		std::string sValue( pszValue );
		TProtobufValueType typedValue;
		if ( !google::protobuf::TextFormat::ParseFromString( sValue, &typedValue ) )
			return false;

		this->ConvertTypedValueToEconAttributeValue( typedValue, out_pValue );
		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value, std::string *out_ps ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_ps );

		google::protobuf::TextFormat::PrintToString( this->GetTypedValueContentsFromEconAttributeValue( value ), out_ps );
	}
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CSchemaAttributeType_String : public CSchemaAttributeTypeProtobufBase< GC_SCH_REFERENCE( CSchItemAttributeString ) CAttribute_String >
{
public:

	// We intentionally override the convert-to-/convert-from-string functions for strings so that string literals can be
	// specified in the schema, etc. without worrying about the protobuf text format.
	virtual bool BConvertStringToEconAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const char *pszValue, union attribute_data_union_t *out_pValue, bool bEnableTerribleBackwardsCompatibilitySchemaParsingCode ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_pValue );

		CAttribute_String typedValue;
		typedValue.set_value( pszValue );

		this->ConvertTypedValueToEconAttributeValue( typedValue, out_pValue );

		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value, std::string *out_ps ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_ps );

		*out_ps = this->GetTypedValueContentsFromEconAttributeValue( value ).value().c_str();
	}
};

void CopyStringAttributeValueToCharPointerOutput( const CAttribute_String *pValue, const char **out_pValue )
{
	Assert( pValue );
	Assert( out_pValue );

	*out_pValue = pValue->value().c_str();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CSchemaAttributeType_DynamicRecipeComponentDefinedItem : public CSchemaAttributeTypeProtobufBase< GC_SCH_REFERENCE( CSchItemAttributeDynamicRecipeComponentDefinedItem ) CAttribute_DynamicRecipeComponent >
{
public:
};


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CSchemaAttributeType_ItemSlotCriteria : public CSchemaAttributeTypeProtobufBase< GC_SCH_REFERENCE( CSchItemAttributeItemSlotCriteria ) CAttribute_ItemSlotCriteria >
{
public:

	virtual bool BConvertStringToEconAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const char *pszValue, union attribute_data_union_t *out_pValue, bool bEnableTerribleBackwardsCompatibilitySchemaParsingCode ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_pValue );

		std::string sValue( pszValue );
		CAttribute_ItemSlotCriteria typedValue;
		if ( !google::protobuf::TextFormat::ParseFromString( sValue, &typedValue ) )
			return false;

		this->ConvertTypedValueToEconAttributeValue( typedValue, out_pValue );

		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value, std::string *out_ps ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_ps );

		this->ConvertEconAttributeValueToString( pAttrDef, value, out_ps );
	}
};


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CSchemaAttributeType_WorldItemPlacement : public CSchemaAttributeTypeProtobufBase < GC_SCH_REFERENCE( CSchItemAttributeWorldItemPlacement ) CAttribute_WorldItemPlacement >
{
public:

	virtual bool BConvertStringToEconAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const char *pszValue, union attribute_data_union_t *out_pValue, bool bEnableTerribleBackwardsCompatibilitySchemaParsingCode ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_pValue );

		CAttribute_WorldItemPlacement typedValue;

		uint32 unValue = ( pszValue ) ? atoi( pszValue ) : 0;
		
		// Item forcing us to create the attribute (via force_gc_to_generate)
		if ( unValue == 0 )
		{
			typedValue.set_original_item_id( INVALID_ITEM_ID );
			typedValue.set_pos_x( 0.f );
			typedValue.set_pos_y( 0.f );
			typedValue.set_pos_z( 0.f );
			typedValue.set_ang_x( 0.f );
			typedValue.set_ang_y( 0.f );
			typedValue.set_ang_z( 0.f );
		}
		else
		{
			std::string sValue( pszValue );
			if ( !google::protobuf::TextFormat::ParseFromString( sValue, &typedValue ) )
				return false;
		}
		
		this->ConvertTypedValueToEconAttributeValue( typedValue, out_pValue );
		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value, std::string *out_ps ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_ps );

		this->ConvertEconAttributeValueToString( pAttrDef, value, out_ps );
	}
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CSchemaAttributeType_Float : public CSchemaAttributeTypeBase< GC_SCH_REFERENCE( CSchItemAttributeFloat ) float >
{
public:

	virtual bool BConvertStringToEconAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const char *pszValue, union attribute_data_union_t *out_pValue, bool bEnableTerribleBackwardsCompatibilitySchemaParsingCode ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_pValue );

		out_pValue->asFloat = Q_atof( pszValue );
		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value, std::string *out_ps ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_ps );

		*out_ps = CFmtStr( "%f", value.asFloat ).Get();
	}
	
	virtual void ConvertTypedValueToByteStream( const float& typedValue, ::std::string *out_psBytes ) const OVERRIDE
	{
		Assert( out_psBytes );
		Assert( out_psBytes->size() == 0 );

		out_psBytes->resize( sizeof( float ) );
		*reinterpret_cast<float *>( &((*out_psBytes)[0]) ) = typedValue;		// overwrite string contents (sizeof( float ) bytes)
	}

	virtual void ConvertByteStreamToTypedValue( const ::std::string& sBytes, float *out_pTypedValue ) const OVERRIDE
	{
		Assert( out_pTypedValue );
		Assert( sBytes.size() == sizeof( float ) );

		*out_pTypedValue = *reinterpret_cast<const float *>( &sBytes[0] );
	}

	virtual bool BSupportsGameplayModificationAndNetworking() const OVERRIDE
	{
		return true;
	}
};


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CSchemaAttributeType_UInt64 : public CSchemaAttributeTypeBase< GC_SCH_REFERENCE( CSchItemAttributeUInt64 ) uint64 >
{
public:

	virtual bool BConvertStringToEconAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const char *pszValue, union attribute_data_union_t *out_pValue, bool bEnableTerribleBackwardsCompatibilitySchemaParsingCode ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_pValue );

		out_pValue->asUint32 = V_atoui64( pszValue );
		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value, std::string *out_ps ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_ps );

		uint64 ulValue;
		ConvertEconAttributeValueToTypedValue( value, &ulValue );

		*out_ps = CFmtStr( "%llu", ulValue ).Get();
	}
	
	virtual void ConvertTypedValueToByteStream( const uint64& typedValue, ::std::string *out_psBytes ) const OVERRIDE
	{
		Assert( out_psBytes );
		Assert( out_psBytes->size() == 0 );

		out_psBytes->resize( sizeof( uint64 ) );
		*reinterpret_cast<uint64 *>( &((*out_psBytes)[0]) ) = typedValue;		// overwrite string contents (sizeof( uint64 ) bytes)
	}

	virtual void ConvertByteStreamToTypedValue( const ::std::string& sBytes, uint64 *out_pTypedValue ) const OVERRIDE
	{
		Assert( out_pTypedValue );
		Assert( sBytes.size() == sizeof( uint64 ) );

		*out_pTypedValue = *reinterpret_cast<const uint64 *>( &sBytes[0] );
	}
};


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CSchemaAttributeType_Default : public CSchemaAttributeTypeBase< GC_SCH_REFERENCE( CSchItemAttribute ) attrib_value_t >
{
public:

	virtual bool BConvertStringToEconAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const char *pszValue, union attribute_data_union_t *out_pValue, bool bEnableTerribleBackwardsCompatibilitySchemaParsingCode ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_pValue );

		if ( bEnableTerribleBackwardsCompatibilitySchemaParsingCode )
		{
			// Not having any value specified is valid -- we interpret this as "default", or 0 as both an in int and a float.
			out_pValue->asFloat = pszValue
								? atof( pszValue )
								: 0.0f;
		}
		// This is terrible backwards-compatibility code to support the pulling of values from econ asset classes.
		else
		{
			if ( pAttrDef->IsStoredAsInteger() )
			{
				out_pValue->asUint32 = (uint32)Q_atoui64( pszValue );
			}
			else if ( pAttrDef->IsStoredAsFloat() )
			{
				out_pValue->asFloat = Q_atof( pszValue );
			}
			else
			{
				Assert( !"Unknown storage type for CSchemaAttributeType_Default::BConvertStringToEconAttributeValue()!" );
				return false;
			}
		}

		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconItemAttributeDefinition *pAttrDef, const attribute_data_union_t& value, std::string *out_ps ) const OVERRIDE
	{
		Assert( pAttrDef );
		Assert( out_ps );

		if( pAttrDef->IsStoredAsFloat() )
		{
			*out_ps = CFmtStr( "%f", value.asFloat ).Get();
		}
		else if( pAttrDef->IsStoredAsInteger() )
		{
			*out_ps = CFmtStr( "%u", value.asUint32 ).Get();
		}
		else
		{
			Assert( !"Unknown storage type for CSchemaAttributeType_Default::ConvertEconAttributeValueToString()!" );
		}
	}
	
	virtual void ConvertTypedValueToByteStream( const attrib_value_t& typedValue, ::std::string *out_psBytes ) const OVERRIDE
	{
		Assert( out_psBytes );
		Assert( out_psBytes->size() == 0 );

		out_psBytes->resize( sizeof( attrib_value_t ) );
		*reinterpret_cast<attrib_value_t *>( &((*out_psBytes)[0]) ) = typedValue;		// overwrite string contents (sizeof( attrib_value_t ) bytes)
	}

	virtual void ConvertByteStreamToTypedValue( const ::std::string& sBytes, attrib_value_t *out_pTypedValue ) const OVERRIDE
	{
		Assert( out_pTypedValue );
		// Game clients and servers may have partially out-of-date information, or may have downloaded a new schema
		// but not know how to parse an attribute of a certain type, etc. In these cases, because we know we
		// aren't on the GC, temporarily failing to load these values until the client shuts down and updates
		// is about the best we can hope for.
		if ( sBytes.size() < sizeof( attrib_value_t ) )
		{
			*out_pTypedValue = attrib_value_t();
			return;
		}

		*out_pTypedValue = *reinterpret_cast<const attrib_value_t *>( &sBytes[0] );
	}

	virtual bool BSupportsGameplayModificationAndNetworking() const OVERRIDE
	{
		return true;
	}
	
private:
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CEconItemAttributeDefinition::CEconItemAttributeDefinition( void )
:	m_pKVAttribute( NULL ),
	m_pAttrType( NULL ),
	m_bHidden( false ),
	m_bWebSchemaOutputForced( false ),
	m_bStoredAsInteger( false ),
	m_bInstanceData( false ),
	m_bIsSetBonus( false ),
	m_iUserGenerationType( 0 ),
	m_iEffectType( ATTRIB_EFFECT_NEUTRAL ),
	m_iDescriptionFormat( 0 ),
	m_pszDescriptionString( NULL ),
	m_pszArmoryDesc( NULL ),
	m_pszDefinitionName( NULL ),
	m_pszAttributeClass( NULL ),
	m_ItemDefinitionTag( INVALID_ECON_TAG_HANDLE ),
	m_bCanAffectMarketName( false ),
	m_bCanAffectRecipeComponentName( false )
  , m_iszAttributeClass( NULL_STRING )
{
}


//-----------------------------------------------------------------------------
// Purpose:	Copy constructor
//-----------------------------------------------------------------------------
CEconItemAttributeDefinition::CEconItemAttributeDefinition( const CEconItemAttributeDefinition &that )
{
	(*this) = that;
}


//-----------------------------------------------------------------------------
// Purpose:	Operator=
//-----------------------------------------------------------------------------
CEconItemAttributeDefinition &CEconItemAttributeDefinition::operator=( const CEconItemAttributeDefinition &rhs )
{
	m_nDefIndex = rhs.m_nDefIndex;
	m_pAttrType = rhs.m_pAttrType;
	m_bHidden = rhs.m_bHidden;
	m_bWebSchemaOutputForced = rhs.m_bWebSchemaOutputForced;
	m_bStoredAsInteger = rhs.m_bStoredAsInteger;
	m_iUserGenerationType = rhs.m_iUserGenerationType;
	m_bInstanceData = rhs.m_bInstanceData;
	m_bIsSetBonus = rhs.m_bIsSetBonus;
	m_iEffectType = rhs.m_iEffectType;
	m_iDescriptionFormat = rhs.m_iDescriptionFormat;
	m_pszDescriptionString = rhs.m_pszDescriptionString;
	m_pszArmoryDesc = rhs.m_pszArmoryDesc;
	m_pszDefinitionName = rhs.m_pszDefinitionName;
	m_pszAttributeClass = rhs.m_pszAttributeClass;
	m_ItemDefinitionTag = rhs.m_ItemDefinitionTag;
	m_bCanAffectMarketName = rhs.m_bCanAffectMarketName;
	m_bCanAffectRecipeComponentName = rhs.m_bCanAffectRecipeComponentName;
	m_iszAttributeClass = rhs.m_iszAttributeClass;

	m_pKVAttribute = NULL;
	if ( NULL != rhs.m_pKVAttribute )
	{
		m_pKVAttribute = rhs.m_pKVAttribute->MakeCopy();

		// Re-assign string pointers
		m_pszDefinitionName = m_pKVAttribute->GetString("name");
		m_pszDescriptionString = m_pKVAttribute->GetString( "description_string", NULL );
		m_pszArmoryDesc = m_pKVAttribute->GetString( "armory_desc", NULL );
		m_pszAttributeClass = m_pKVAttribute->GetString( "attribute_class", NULL );

		Assert( V_strcmp( m_pszDefinitionName, rhs.m_pszDefinitionName ) == 0 );
		Assert( V_strcmp( m_pszDescriptionString, rhs.m_pszDescriptionString ) == 0 );
		Assert( V_strcmp( m_pszArmoryDesc, rhs.m_pszArmoryDesc ) == 0 );
		Assert( V_strcmp( m_pszAttributeClass, rhs.m_pszAttributeClass ) == 0 );
	}
	else
	{
		Assert( m_pszDefinitionName == NULL );
		Assert( m_pszDescriptionString == NULL );
		Assert( m_pszArmoryDesc == NULL );
		Assert( m_pszAttributeClass == NULL );
	}
	return *this;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CEconItemAttributeDefinition::~CEconItemAttributeDefinition( void )
{
	if ( m_pKVAttribute )
		m_pKVAttribute->deleteThis();
	m_pKVAttribute = NULL;
}


//-----------------------------------------------------------------------------
// Purpose:	Initialize the attribute definition
// Input:	pKVAttribute - The KeyValues representation of the attribute
//			schema - The overall item schema for this attribute
//			pVecErrors - An optional vector that will contain error messages if 
//				the init fails.
// Output:	True if initialization succeeded, false otherwise
//-----------------------------------------------------------------------------
bool CEconItemAttributeDefinition::BInitFromKV( KeyValues *pKVAttribute, CUtlVector<CUtlString> *pVecErrors /* = NULL */ )
{
	m_pKVAttribute = pKVAttribute->MakeCopy();
	m_nDefIndex = Q_atoi( m_pKVAttribute->GetName() );
	
	m_pszDefinitionName = m_pKVAttribute->GetString("name", "(unnamed)");
	m_bHidden = m_pKVAttribute->GetInt( "hidden", 0 ) != 0;
	m_bWebSchemaOutputForced = m_pKVAttribute->GetInt( "force_output_description", 0 ) != 0;
	m_bStoredAsInteger = m_pKVAttribute->GetInt( "stored_as_integer", 0 ) != 0;
	m_bIsSetBonus = m_pKVAttribute->GetBool( "is_set_bonus", false );
	m_bCanAffectMarketName = m_pKVAttribute->GetBool( "can_affect_market_name", false );
	m_bCanAffectRecipeComponentName = m_pKVAttribute->GetBool( "can_affect_recipe_component_name", false );
	m_iUserGenerationType = m_pKVAttribute->GetInt( "is_user_generated", 0 );
	m_iEffectType = (attrib_effect_types_t)StringFieldToInt( m_pKVAttribute->GetString("effect_type"), g_EffectTypes, ARRAYSIZE(g_EffectTypes) );
	m_iDescriptionFormat = StringFieldToInt( m_pKVAttribute->GetString("description_format"), g_AttributeDescriptionFormats, ARRAYSIZE(g_AttributeDescriptionFormats) );
	m_pszDescriptionString = m_pKVAttribute->GetString( "description_string", NULL );
	m_pszArmoryDesc = m_pKVAttribute->GetString( "armory_desc", NULL );
	m_pszAttributeClass = m_pKVAttribute->GetString( "attribute_class", NULL );
	m_bInstanceData = pKVAttribute->GetBool( "instance_data", false );

	const char *pszTag = m_pKVAttribute->GetString( "apply_tag_to_item_definition", NULL );
	m_ItemDefinitionTag = pszTag ? GetItemSchema()->GetHandleForTag( pszTag ) : INVALID_ECON_TAG_HANDLE;

#if defined(CLIENT_DLL) || defined(GAME_DLL)
	m_iszAttributeClass = NULL_STRING;
#endif
	const char *pszAttrType = m_pKVAttribute->GetString( "attribute_type", NULL );		// NULL implies "default type" for backwards compatibility
	m_pAttrType = GetItemSchema()->GetAttributeType( pszAttrType );

	SCHEMA_INIT_CHECK( 
		NULL != m_pKVAttribute->FindKey( "name" ), 
		"Attribute definition %s: Missing required field \"name\"", m_pKVAttribute->GetName() );

	SCHEMA_INIT_CHECK(
		NULL != m_pAttrType,
		"Attribute definition %s: Unable to find attribute data type '%s'", m_pszDefinitionName, pszAttrType ? pszAttrType : "(default)" );

	if ( m_bIsSetBonus )
	{
		SCHEMA_INIT_CHECK(
			m_pAttrType->BSupportsGameplayModificationAndNetworking(),
			"Attribute definition %s: set as set bonus attribute but does not support gameplay modification/networking!", m_pszDefinitionName );
	}

	m_unAssetClassBucket = pKVAttribute->GetInt( "asset_class_bucket", 0 );
	m_eAssetClassAttrExportRule = k_EAssetClassAttrExportRule_Default;
	if ( char const *szRule = pKVAttribute->GetString( "asset_class_export", NULL ) )
	{
		if ( !V_stricmp( szRule, "skip" ) )
		{
			m_eAssetClassAttrExportRule = k_EAssetClassAttrExportRule_Skip;
		}
		else if ( !V_stricmp( szRule, "gconly" ) )
		{
			m_eAssetClassAttrExportRule = EAssetClassAttrExportRule_t( k_EAssetClassAttrExportRule_GCOnly | k_EAssetClassAttrExportRule_Skip );
		}
		else if ( !V_stricmp( szRule, "bucketed" ) )
		{
			SCHEMA_INIT_CHECK( m_unAssetClassBucket, "Attribute definition %s: Asset class export rule '%s' is incompatible", m_pszDefinitionName, szRule );
			m_eAssetClassAttrExportRule = k_EAssetClassAttrExportRule_Bucketed;
		}
		else if ( !V_stricmp( szRule, "default" ) )
		{
			m_eAssetClassAttrExportRule = k_EAssetClassAttrExportRule_Default;
		}
		else
		{
			SCHEMA_INIT_CHECK( false, "Attribute definition %s: Invalid asset class export rule '%s'", m_pszDefinitionName, szRule );
		}
	}

	// Check for misuse of asset class bucket
	SCHEMA_INIT_CHECK( ( !m_unAssetClassBucket || m_bInstanceData ), "Attribute definition %s: Cannot use \"asset_class_bucket\" on class-level attributes", m_pKVAttribute->GetName() );


	return SCHEMA_INIT_SUCCESS();
}



//-----------------------------------------------------------------------------
// Purpose:	Constructor
//-----------------------------------------------------------------------------
CEconItemDefinition::CEconItemDefinition( void )
:	m_pKVItem( NULL ),
m_bEnabled( false ),
m_unMinItemLevel( 1 ),
m_unMaxItemLevel( 1 ),
m_iArmoryRemap( 0 ),
m_iStoreRemap( 0 ),
m_nItemQuality( k_unItemQuality_Any ),
m_nForcedItemQuality( k_unItemQuality_Any ),
m_nDefaultDropQuantity( 1 ),
m_bLoadOnDemand( false ),
m_pTool( NULL ),
m_rtExpiration( 0 ),
m_BundleInfo( NULL ),
#ifdef TF_CLIENT_DLL
m_unNumConcreteItems( 0 ),
#endif // TF_CLIENT_DLL
m_nPopularitySeed( 0 ),
m_pszDefinitionName( NULL ),
m_pszItemClassname( NULL ),
m_pszClassToken( NULL ),
m_pszSlotToken( NULL ),
m_pszItemBaseName( NULL ),
m_pszItemTypeName( NULL ),
m_pszItemDesc( NULL ),
m_pszArmoryDesc( NULL ),
m_pszInventoryModel( NULL ),
m_pszInventoryImage( NULL ),
m_pszHolidayRestriction( NULL ),
m_iSubType( 0 ),
m_pszBaseDisplayModel( NULL ),
m_iDefaultSkin( -1 ),
m_pszWorldDisplayModel( NULL ),
m_pszWorldExtraWearableModel( NULL ),
m_pszWorldExtraWearableViewModel( NULL ),
m_pszVisionFilteredDisplayModel( NULL ),
m_pszBrassModelOverride( NULL ),
m_bHideBodyGroupsDeployedOnly( false ),
m_bAttachToHands( false ),
m_bAttachToHandsVMOnly( false ),
m_bProperName( false ),
m_bFlipViewModel( false ),
m_bActAsWearable( false ),
m_bActAsWeapon( false ),
m_iDropType( 1 ),
m_bHidden( false ),
m_bShouldShowInArmory( false ),
m_bIsPackBundle( false ),
m_pOwningPackBundle( NULL ),
m_bIsPackItem( false ),
m_bBaseItem( false ),
m_pszItemLogClassname( NULL ),
m_pszItemIconClassname( NULL ),
m_pszDatabaseAuditTable( NULL ),
m_bImported( false ),
m_pItemSetDef( NULL ),
m_pItemCollectionDef( NULL ),
m_pszArmoryRemap( NULL ),
m_pszStoreRemap( NULL ),
m_unSetItemRemapDefIndex( INVALID_ITEM_DEF_INDEX ),
m_pszXifierRemapClass( NULL ),
m_pszBaseFunctionalItemName( NULL ),
m_pszParticleSuffix( NULL ),
m_pszCollectionReference( NULL ),
m_nItemRarity( k_unItemRarity_Any ),
m_unItemSeries( 0 ),
m_bValidForShuffle( false ),
m_bValidForSelfMade( true ),
m_nRemappedDefIndex( INVALID_ITEM_DEF_INDEX )
{
	for ( int team = 0; team < TEAM_VISUAL_SECTIONS; team++ )
	{
		m_PerTeamVisuals[team] = NULL;
	}

	m_pDictIcons = new CUtlDict< CUtlString >;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CEconItemDefinition::~CEconItemDefinition( void )
{
	for ( int i = 0; i < ARRAYSIZE( m_PerTeamVisuals ); i++ )
		delete m_PerTeamVisuals[i];


	if ( m_pKVItem )
		m_pKVItem->deleteThis();
	m_pKVItem = NULL;
	delete m_pTool;
	delete m_BundleInfo;
	delete m_pDictIcons;
}

#if defined(CLIENT_DLL) || defined(GAME_DLL)
//-----------------------------------------------------------------------------
// Purpose: Stomp our base data with extra testing data specified by the player
//-----------------------------------------------------------------------------
bool CEconItemDefinition::BInitFromTestItemKVs( int iNewDefIndex, KeyValues *pKVItem, CUtlVector<CUtlString>* pVecErrors )
{
	// The KeyValues are stored in the player entity, so we can cache our name there

	m_nDefIndex = iNewDefIndex;
	m_unSetItemRemapDefIndex = m_nDefIndex;

	bool bTestingExistingItem = pKVItem->GetBool( "test_existing_item", false );
	if ( !bTestingExistingItem )
	{
		m_pszDefinitionName = pKVItem->GetString( "name", NULL );
		m_pszItemBaseName = pKVItem->GetString( "name", NULL );

#ifdef CLIENT_DLL
		pKVItem->SetString( "name", VarArgs("Test Item %d", iNewDefIndex) );
#else
		pKVItem->SetString( "name", UTIL_VarArgs("Test Item %d", iNewDefIndex) );
#endif

		m_pszBaseDisplayModel = pKVItem->GetString( "model_player", NULL );
		m_pszVisionFilteredDisplayModel = pKVItem->GetString( "model_vision_filtered", NULL );
		m_bAttachToHands = pKVItem->GetInt( "attach_to_hands", 0 ) != 0;

		BInitVisualBlockFromKV( pKVItem );
	}

	// Handle attributes
	m_vecStaticAttributes.Purge();
	int iPaintCanIndex = pKVItem->GetInt("paintcan_index", 0);
	if ( iPaintCanIndex )
	{
		static CSchemaAttributeDefHandle pAttrDef_PaintRGB( "set item tint RGB" );

		const CEconItemDefinition *pCanDef = GetItemSchema()->GetItemDefinition(iPaintCanIndex);
		
		float flRGBVal;
		if ( pCanDef && pAttrDef_PaintRGB && FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pCanDef, pAttrDef_PaintRGB, &flRGBVal ) )
		{
			static_attrib_t& StaticAttrib = m_vecStaticAttributes[ m_vecStaticAttributes.AddToTail() ];

			StaticAttrib.iDefIndex = pAttrDef_PaintRGB->GetDefinitionIndex();
			StaticAttrib.m_value.asFloat = flRGBVal;							// this is bad! but we're in crazy hack code for UI customization of item definitions that don't exist so
		}
	}

	int iUnusualEffectIndex = pKVItem->GetInt( "unusual_index", 0 );
	if ( iUnusualEffectIndex )
	{
		static CSchemaAttributeDefHandle pAttrDef_AttachParticleStatic( "attach particle effect static" );

		const attachedparticlesystem_t *pSystem = GetItemSchema()->GetAttributeControlledParticleSystem( iUnusualEffectIndex );

		if ( pAttrDef_AttachParticleStatic && pSystem )
		{
			static_attrib_t& StaticAttrib = m_vecStaticAttributes[ m_vecStaticAttributes.AddToTail() ];

			StaticAttrib.iDefIndex = pAttrDef_AttachParticleStatic->GetDefinitionIndex();
			StaticAttrib.m_value.asFloat = iUnusualEffectIndex;					// this is bad! but we're in crazy hack code for UI customization of item definitions that don't exist so
		}
	}

	return true;
}

animation_on_wearable_t *GetOrCreateAnimationActivity( perteamvisuals_t *pVisData, const char *pszActivityName )
{
	FOR_EACH_VEC( pVisData->m_Animations, i )
	{
		if ( Q_stricmp(pVisData->m_Animations[i].pszActivity, pszActivityName) == 0 )
			return &pVisData->m_Animations[i];
	}

	animation_on_wearable_t *pEntry = &pVisData->m_Animations[pVisData->m_Animations.AddToTail()];

	pEntry->iActivity = kActivityLookup_Unknown;	// We can't look it up yet, the activity list hasn't been populated.
	pEntry->pszActivity = pszActivityName;
	pEntry->iReplacement = kActivityLookup_Unknown;
	pEntry->pszReplacement = NULL;
	pEntry->pszSequence = NULL;
	pEntry->pszScene = NULL;
	pEntry->pszRequiredItem = NULL;

	return pEntry;
}

activity_on_wearable_t *GetOrCreatePlaybackActivity( perteamvisuals_t *pVisData, wearableanimplayback_t iPlayback )
{
	FOR_EACH_VEC( pVisData->m_Animations, i )
	{
		if ( pVisData->m_Activities[i].iPlayback == iPlayback )
			return &pVisData->m_Activities[i];
	}

	activity_on_wearable_t *pEntry = &pVisData->m_Activities[pVisData->m_Activities.AddToTail()];

	pEntry->iPlayback = iPlayback;
	pEntry->iActivity = kActivityLookup_Unknown;	// We can't look it up yet, the activity list hasn't been populated.
	pEntry->pszActivity = NULL;

	return pEntry;
}

#endif // defined(CLIENT_DLL) || defined(GAME_DLL)

//-----------------------------------------------------------------------------
// Purpose: Handle parsing the per-team visual block from the keyvalues
//-----------------------------------------------------------------------------
void CEconItemDefinition::BInitVisualBlockFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors )
{
	// Visuals
	for ( int team = 0; team < TEAM_VISUAL_SECTIONS; team++ )
	{
		m_PerTeamVisuals[team] = NULL;

		if ( !g_TeamVisualSections[team] )
			continue;

		KeyValues *pVisualsKV = pKVItem->FindKey( g_TeamVisualSections[team] );
		if ( pVisualsKV )
		{
			perteamvisuals_t *pVisData = new perteamvisuals_t();
#if defined(CLIENT_DLL) || defined(GAME_DLL)
			KeyValues *pKVEntry = pVisualsKV->GetFirstSubKey();
			while ( pKVEntry )
			{
				const char *pszEntry = pKVEntry->GetName();

				if ( !Q_stricmp( pszEntry, "use_visualsblock_as_base" ) )
				{
					// Start with a copy of an existing PerTeamVisuals
					const char *pszString = pKVEntry->GetString();
					int nOverrideTeam = GetTeamVisualsFromString( pszString );
					if ( nOverrideTeam != -1 )
					{
						*pVisData = *m_PerTeamVisuals[nOverrideTeam];
					}
					else
					{
						pVecErrors->AddToTail( CFmtStr( "Unknown visuals block: %s", pszString ).Access() );
					}
				}
				else if ( !Q_stricmp( pszEntry, "attached_models" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVAttachedModelData )
					{
						int iAtt = pVisData->m_AttachedModels.AddToTail();
						pVisData->m_AttachedModels[iAtt].m_iModelDisplayFlags = pKVAttachedModelData->GetInt( "model_display_flags", kAttachedModelDisplayFlag_MaskAll );
						pVisData->m_AttachedModels[iAtt].m_pszModelName = pKVAttachedModelData->GetString( "model", NULL );
					}
				}
				else if ( !Q_stricmp( pszEntry, "attached_models_festive" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVAttachedModelData )
					{
						int iAtt = pVisData->m_AttachedModelsFestive.AddToTail();
						pVisData->m_AttachedModelsFestive[iAtt].m_iModelDisplayFlags = pKVAttachedModelData->GetInt( "model_display_flags", kAttachedModelDisplayFlag_MaskAll );
						pVisData->m_AttachedModelsFestive[iAtt].m_pszModelName = pKVAttachedModelData->GetString( "model", NULL );
					}
				}
				else if ( !Q_stricmp( pszEntry, "attached_particlesystems" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVAttachedParticleSystemData )
					{
						int iAtt = pVisData->m_AttachedParticles.AddToTail();
						pVisData->m_AttachedParticles[iAtt].pszSystemName = pKVAttachedParticleSystemData->GetString( "system", NULL );
						pVisData->m_AttachedParticles[iAtt].pszControlPoints[0] = pKVAttachedParticleSystemData->GetString( "attachment", NULL );
						pVisData->m_AttachedParticles[iAtt].bFollowRootBone = pKVAttachedParticleSystemData->GetBool( "attach_to_rootbone" );
						pVisData->m_AttachedParticles[iAtt].iCustomType = 0;
					}
				}
				else if ( !Q_stricmp( pszEntry, "custom_particlesystem2" ) )
				{
					int iAtt = pVisData->m_AttachedParticles.AddToTail();
					pVisData->m_AttachedParticles[iAtt].pszSystemName = pKVEntry->GetString( "system", NULL );
					pVisData->m_AttachedParticles[iAtt].iCustomType = 2;
				}
				else if ( !Q_stricmp( pszEntry, "custom_particlesystem" ) )
				{
					int iAtt = pVisData->m_AttachedParticles.AddToTail();
					pVisData->m_AttachedParticles[iAtt].pszSystemName = pKVEntry->GetString( "system", NULL );
					pVisData->m_AttachedParticles[iAtt].iCustomType = 1;
				}
				else if ( !Q_stricmp( pszEntry, "playback_activity" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVSubKey )
					{
						int iPlaybackInt = StringFieldToInt( pKVSubKey->GetName(), g_WearableAnimTypeStrings, ARRAYSIZE(g_WearableAnimTypeStrings) );
						if ( iPlaybackInt >= 0 )
						{
							activity_on_wearable_t *pEntry = GetOrCreatePlaybackActivity( pVisData, (wearableanimplayback_t)iPlaybackInt );
							pEntry->pszActivity = pKVSubKey->GetString();
						}
					}
				}
				else if ( !Q_stricmp( pszEntry, "animation_replacement" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVSubKey )
					{
						animation_on_wearable_t *pEntry = GetOrCreateAnimationActivity( pVisData, pKVSubKey->GetName() );
						pEntry->pszReplacement = pKVSubKey->GetString();
					}
				}
				else if ( !Q_stricmp( pszEntry, "animation_sequence" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVSubKey )
					{
						animation_on_wearable_t *pEntry = GetOrCreateAnimationActivity( pVisData, pKVSubKey->GetName() );
						pEntry->pszSequence = pKVSubKey->GetString();
					}
				}
				else if ( !Q_stricmp( pszEntry, "animation_scene" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVSubKey )
					{
						animation_on_wearable_t *pEntry = GetOrCreateAnimationActivity( pVisData, pKVSubKey->GetName() );
						pEntry->pszScene = pKVSubKey->GetString();
					}
				}
				else if ( !Q_stricmp( pszEntry, "animation_required_item" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVSubKey )
					{
						animation_on_wearable_t *pEntry = GetOrCreateAnimationActivity( pVisData, pKVSubKey->GetName() );
						pEntry->pszRequiredItem = pKVSubKey->GetString();
					}
				}
				else if ( !Q_stricmp( pszEntry, "player_poseparam" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVSubKey )
					{
						poseparamtable_t *pPoseParam = pVisData->m_PlayerPoseParams.AddToTailGetPtr();
						pPoseParam->strName = pKVSubKey->GetName();
						pPoseParam->flValue = pKVSubKey->GetFloat();
					}
				}
				else if ( !Q_stricmp( pszEntry, "item_poseparam" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVSubKey )
					{
						poseparamtable_t *pPoseParam = pVisData->m_ItemPoseParams.AddToTailGetPtr();
						pPoseParam->strName = pKVSubKey->GetName();
						pPoseParam->flValue = pKVSubKey->GetFloat();
					}
				}
				else if ( !Q_stricmp( pszEntry, "player_bodygroups" ) )
				{
					FOR_EACH_SUBKEY( pKVEntry, pKVBodygroupKey )
					{
						const char *pszBodygroupName = pKVBodygroupKey->GetName();
						int iValue = pKVBodygroupKey->GetInt();

						// Track bodygroup information for this item in particular.
						pVisData->m_Maps.m_ModifiedBodyGroupNames.Insert( pszBodygroupName, iValue );

						// Track global schema state.
						GetItemSchema()->AssignDefaultBodygroupState( pszBodygroupName, iValue );
					}
				}
				else if ( !Q_stricmp( pszEntry, "skin" ) )
				{
					pVisData->iSkin = pKVEntry->GetInt();
				}
				else if ( !Q_stricmp( pszEntry, "use_per_class_bodygroups" ) )
				{
					pVisData->bUsePerClassBodygroups = pKVEntry->GetBool();
				}
				else if ( !Q_stricmp( pszEntry, "muzzle_flash" ) )
				{
					pVisData->pszMuzzleFlash = pKVEntry->GetString();
				}
				else if ( !Q_stricmp( pszEntry, "tracer_effect" ) )
				{
					pVisData->pszTracerEffect = pKVEntry->GetString();
				}
				else if ( !Q_stricmp( pszEntry, "particle_effect" ) )
				{
					pVisData->pszParticleEffect = pKVEntry->GetString();
				}
				else if ( !Q_strnicmp( pszEntry, "custom_sound", 12 ) )			// intentionally comparing prefixes
				{
					int iIndex = 0;
					if ( pszEntry[12] )
					{
						iIndex = clamp( atoi( &pszEntry[12] ), 0, MAX_VISUALS_CUSTOM_SOUNDS-1 );
					}
					pVisData->pszCustomSounds[iIndex] = pKVEntry->GetString();
				}
				else if ( !Q_stricmp( pszEntry, "material_override" ) )
				{
					pVisData->pszMaterialOverride = pKVEntry->GetString();
				}
				else if ( !Q_strnicmp( pszEntry, "sound_", 6 ) )				// intentionally comparing prefixes
				{
					int iIndex = GetWeaponSoundFromString( &pszEntry[6] );
					if ( iIndex != -1 )
					{
						pVisData->pszWeaponSoundReplacements[iIndex] = pKVEntry->GetString();
					}
				}
				else if ( !Q_stricmp( pszEntry, "code_controlled_bodygroup" ) )
				{
					const char *pBodyGroupName = pKVEntry->GetString( "bodygroup", NULL );
					const char *pFuncName = pKVEntry->GetString( "function", NULL );
					if ( pBodyGroupName && pFuncName )
					{
						codecontrolledbodygroupdata_t ccbgd = { pFuncName, NULL };
						pVisData->m_Maps.m_CodeControlledBodyGroupNames.Insert( pBodyGroupName, ccbgd );
					}
				}
				else if ( !Q_stricmp( pszEntry, "vm_bodygroup_override" ) )
				{
					pVisData->m_iViewModelBodyGroupOverride = pKVEntry->GetInt();
				}
				else if ( !Q_stricmp( pszEntry, "vm_bodygroup_state_override" ) )
				{
					pVisData->m_iViewModelBodyGroupStateOverride = pKVEntry->GetInt();
				}
				else if ( !Q_stricmp( pszEntry, "wm_bodygroup_override" ) )
				{
					pVisData->m_iWorldModelBodyGroupOverride = pKVEntry->GetInt();
				}
				else if ( !Q_stricmp( pszEntry, "wm_bodygroup_state_override" ) )
				{
					pVisData->m_iWorldModelBodyGroupStateOverride = pKVEntry->GetInt();
				}

				pKVEntry = pKVEntry->GetNextKey();
			}
#endif // defined(CLIENT_DLL) || defined(GAME_DLL)
			KeyValues *pStylesDataKV = pVisualsKV->FindKey( "styles" );
			if ( pStylesDataKV )
			{
				// Styles are only valid in the base "visuals" section.
				if ( team == 0 )
				{
					BInitStylesBlockFromKV( pStylesDataKV, pVisData, pVecErrors );
				}
				// ...but they used to be valid everywhere, so spit out a warning if people are trying to use
				// the old style of per-team styles.
				else
				{
					pVecErrors->AddToTail( "Per-team styles blocks are no longer valid. Use \"skin_red\" and \"skin_blu\" in a style entry instead." );
				}
			}

			m_PerTeamVisuals[team] = pVisData;
		}
	}
}

#if defined(CLIENT_DLL) || defined(GAME_DLL)
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconItemDefinition::GeneratePrecacheModelStrings( bool bDynamicLoad, CUtlVector<const char *> *out_pVecModelStrings ) const
{
	Assert( out_pVecModelStrings );

	// Add base model.
	out_pVecModelStrings->AddToTail( GetBasePlayerDisplayModel() );

	// Add styles.
	if ( GetNumStyles() )
	{
		for ( style_index_t i=0; i<GetNumStyles(); ++i )
		{
			const CEconStyleInfo *pStyle = GetStyleInfo( i );
			Assert( pStyle );

			pStyle->GeneratePrecacheModelStringsForStyle( out_pVecModelStrings );
		}
	}

	// Precache all the attached models
	for ( int team = 0; team < TEAM_VISUAL_SECTIONS; team++ )
	{
		perteamvisuals_t *pPerTeamVisuals = GetPerTeamVisual( team );
		if ( !pPerTeamVisuals )
			continue;

		for ( int model = 0; model < pPerTeamVisuals->m_AttachedModels.Count(); model++ )
		{
			out_pVecModelStrings->AddToTail( pPerTeamVisuals->m_AttachedModels[model].m_pszModelName );
		}

		// Festive
		for ( int model = 0; model < pPerTeamVisuals->m_AttachedModelsFestive.Count(); model++ )
		{
			out_pVecModelStrings->AddToTail( pPerTeamVisuals->m_AttachedModelsFestive[model].m_pszModelName );
		}
	}

	if ( GetExtraWearableModel() )
	{
		out_pVecModelStrings->AddToTail( GetExtraWearableModel() );
	}

	if ( GetExtraWearableViewModel() )
	{
		out_pVecModelStrings->AddToTail( GetExtraWearableViewModel() );
	}

	if ( GetVisionFilteredDisplayModel() )
	{
		out_pVecModelStrings->AddToTail( GetVisionFilteredDisplayModel() );
	}

	// We don't need to cache the inventory model, because it's never loaded by the game
}

void CEconItemDefinition::GeneratePrecacheSoundStrings( bool bDynamicLoad, CUtlVector<const char *> *out_pVecSoundStrings ) const
{
	Assert( out_pVecSoundStrings );

	for ( int iTeam = 0; iTeam < TEAM_VISUAL_SECTIONS; ++iTeam )
	{
		for ( int iSound = 0; iSound < MAX_VISUALS_CUSTOM_SOUNDS; ++iSound )
		{
			const char *pSoundName = GetCustomSound( iTeam, iSound );
			if ( pSoundName && pSoundName[ 0 ] != '\0' )
			{
				out_pVecSoundStrings->AddToTail( pSoundName );
			}
		}
	}
}
#endif // #if defined(CLIENT_DLL) || defined(GAME_DLL)

//-----------------------------------------------------------------------------
const char	*CEconItemDefinition::GetDefinitionString( const char *pszKeyName, const char *pszDefaultValue ) const
{
	// !FIXME! Here we could do a dynamic lookup to apply the prefab overlay logic.
	// This could save a lot of duplicated data
	if ( m_pKVItem )
		return m_pKVItem->GetString( pszKeyName, pszDefaultValue );
	return pszDefaultValue;
}

//-----------------------------------------------------------------------------
KeyValues	*CEconItemDefinition::GetDefinitionKey( const char *pszKeyName ) const
{
	// !FIXME! Here we could do a dynamic lookup to apply the prefab overlay logic.
	// This could save a lot of duplicated data
	if ( m_pKVItem )
		return m_pKVItem->FindKey( pszKeyName );
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Parse the styles sub-section of the visuals block.
//-----------------------------------------------------------------------------
void CEconItemDefinition::BInitStylesBlockFromKV( KeyValues *pKVStyles, perteamvisuals_t *pVisData, CUtlVector<CUtlString> *pVecErrors )
{
	FOR_EACH_SUBKEY( pKVStyles, pKVStyle )
	{
		CEconStyleInfo *pStyleInfo = GetItemSchema()->CreateEconStyleInfo();
		Assert( pStyleInfo );

		pStyleInfo->BInitFromKV( pKVStyle, pVecErrors );

		pVisData->m_Styles.AddToTail( pStyleInfo );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Parse one style from the styles block.
//-----------------------------------------------------------------------------
void CEconStyleInfo::BInitFromKV( KeyValues *pKVStyle, CUtlVector<CUtlString> *pVecErrors )
{
	enum { kInvalidSkinKey = -1, };

	Assert( pKVStyle );

	// A "skin" entry means "use this index for all of our teams, no matter how many we have".
	int iCommonSkin = pKVStyle->GetInt( "skin", kInvalidSkinKey );
	if ( iCommonSkin != kInvalidSkinKey )
	{
		for ( int i = 0; i < TEAM_VISUAL_SECTIONS; i++ )
		{
			m_iSkins[i] = iCommonSkin;
		}
	}

	int iCommonViewmodelSkin = pKVStyle->GetInt( "v_skin", kInvalidSkinKey );
	if ( iCommonViewmodelSkin != kInvalidSkinKey )
	{
		for ( int i=0; i<TEAM_VISUAL_SECTIONS; i++ )
		{
			m_iViewmodelSkins[i] = iCommonViewmodelSkin;
		}
	}

	// If we don't have a base entry, we look for a unique entry for each team. This will be
	// handled in a subclass if necessary.

	// Are we hiding additional bodygroups when this style is active?
	KeyValues *pKVHideBodygroups = pKVStyle->FindKey( "additional_hidden_bodygroups" );
	if ( pKVHideBodygroups )
	{
		FOR_EACH_SUBKEY( pKVHideBodygroups, pKVBodygroup )
		{
			m_vecAdditionalHideBodygroups.AddToTail( pKVBodygroup->GetName() );
		}
	}

	// Remaining common properties.
	m_pszName = pKVStyle->GetString( "name", "#TF_UnknownStyle" );
	m_pszBasePlayerModel = pKVStyle->GetString( "model_player", NULL );
	m_bIsSelectable = pKVStyle->GetBool( "selectable", true );
	m_pszInventoryImage = pKVStyle->GetString( "image_inventory", NULL );
	m_bUseSmokeParticleEffect = pKVStyle->GetBool( "use_smoke_particle_effect", true );

	KeyValues *pKVBodygroup = pKVStyle->FindKey( "bodygroup" );
	if ( pKVBodygroup )
	{
		m_pszBodygroupName = pKVBodygroup->GetString( "name", NULL );
		Assert( m_pszBodygroupName );
		m_iBodygroupSubmodelIndex = pKVBodygroup->GetInt( "submodel_index", -1 );
		Assert( m_iBodygroupSubmodelIndex != -1 );
	}
}

#if defined(CLIENT_DLL) || defined(GAME_DLL)
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconStyleInfo::GeneratePrecacheModelStringsForStyle( CUtlVector<const char *> *out_pVecModelStrings ) const
{
	Assert( out_pVecModelStrings );

	if ( GetBasePlayerDisplayModel() != NULL )
	{
		out_pVecModelStrings->AddToTail( GetBasePlayerDisplayModel() );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose:	Item definition initialization helpers.
//-----------------------------------------------------------------------------
static void RecursiveInheritKeyValues( KeyValues *out_pValues, KeyValues *pInstance )
{
	KeyValues *pPrevSubKey = NULL;
	for ( KeyValues * pSubKey = pInstance->GetFirstSubKey(); pSubKey != NULL; pPrevSubKey = pSubKey, pSubKey = pSubKey->GetNextKey() )
	{
		// If this assert triggers, you have an item that uses a prefab but has multiple keys with the same name
		AssertMsg2 ( !pPrevSubKey || pPrevSubKey->GetNameSymbol() != pSubKey->GetNameSymbol(),
			"Item definition \"%s\" has multiple attributes of the same name (%s) can't use prefabs", pInstance->GetName(), pSubKey->GetName() );

		KeyValues::types_t eType = pSubKey->GetDataType();
		switch ( eType )
		{
		case KeyValues::TYPE_STRING:		out_pValues->SetString( pSubKey->GetName(), pSubKey->GetString() );			break;
		case KeyValues::TYPE_INT:			out_pValues->SetInt( pSubKey->GetName(), pSubKey->GetInt() );				break;
		case KeyValues::TYPE_FLOAT:			out_pValues->SetFloat( pSubKey->GetName(), pSubKey->GetFloat() );			break;
		case KeyValues::TYPE_WSTRING:		out_pValues->SetWString( pSubKey->GetName(), pSubKey->GetWString() );		break;
		case KeyValues::TYPE_COLOR:			out_pValues->SetColor( pSubKey->GetName(), pSubKey->GetColor() ) ;			break;
		case KeyValues::TYPE_UINT64:		out_pValues->SetUint64( pSubKey->GetName(), pSubKey->GetUint64() ) ;		break;

		// "NONE" means "KeyValues"
		case KeyValues::TYPE_NONE:
		{
			// We may already have this part of the tree to stuff data into/overwrite, or we
			// may have to make a new block.
			KeyValues *pNewChild = out_pValues->FindKey( pSubKey->GetName() );
			if ( !pNewChild )
			{
				pNewChild = out_pValues->CreateNewKey();
				pNewChild->SetName( pSubKey->GetName() );
			}

			RecursiveInheritKeyValues( pNewChild, pSubKey );
			break;
		}

		case KeyValues::TYPE_PTR:
		default:
			Assert( !"Unhandled data type for KeyValues inheritance!" );
			break;
		}
	}
}

void MergeDefinitionPrefab( KeyValues *pKVWriteItem, KeyValues *pKVSourceItem )
{
	Assert( pKVWriteItem );
	Assert( pKVSourceItem );

	const char *svPrefabName = pKVSourceItem->GetString( "prefab", NULL );
	
	if ( svPrefabName )
	{
		CUtlStringList vecPrefabs;

		Q_SplitString( svPrefabName, " ", vecPrefabs );

		// Iterate backwards so adjectives get applied over the noun prefab
		// e.g. wet scared cat would apply cat first, then scared and wet.
		FOR_EACH_VEC_BACK( vecPrefabs, i )
		{
			KeyValues *pKVPrefab = GetItemSchema()->FindDefinitionPrefabByName( vecPrefabs[i] );
			AssertMsg1( pKVPrefab, "Unable to find prefab \"%s\".", vecPrefabs[i] );

			if ( pKVPrefab )
			{
				MergeDefinitionPrefab( pKVWriteItem, pKVPrefab );
			}
		}
	}

	RecursiveInheritKeyValues( pKVWriteItem, pKVSourceItem );
}

KeyValues *CEconItemSchema::FindDefinitionPrefabByName( const char *pszPrefabName ) const
{
	int iIndex = m_dictDefinitionPrefabs.Find( pszPrefabName );
	if ( m_dictDefinitionPrefabs.IsValidIndex( iIndex ) )
		return m_dictDefinitionPrefabs[iIndex];

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CEconItemSchema::FindStringTableEntry( const char *pszTableName, int iIndex ) const
{
	SchemaStringTableDict_t::IndexType_t i = m_dictStringTable.Find( pszTableName );
	if ( !m_dictStringTable.IsValidIndex( i ) )
		return NULL;

	const CUtlVector< schema_string_table_entry_t >& vec = *m_dictStringTable[i];
	FOR_EACH_VEC( vec, j )
	{
		if ( vec[j].m_iIndex == iIndex )
			return vec[j].m_pszStr;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:	Initialize the item definition
// Input:	pKVItem - The KeyValues representation of the item
//			schema - The overall item schema for this item
//			pVecErrors - An optional vector that will contain error messages if 
//				the init fails.
// Output:	True if initialization succeeded, false otherwise
//-----------------------------------------------------------------------------


#if defined( WITH_STREAMABLE_WEAPONS )
#if defined( CLIENT_DLL )
    ConVar tf_loadondemand_default("cl_loadondemand_default", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, "The default value for whether items should be delay loaded (1) or loaded now (0).");
#elif defined( GAME_DLL )
    // The server doesn't load on demand by default because it can crash sometimes when this is set. We need to run that down, but in the meantime 
    // we just have it load on demand.
    ConVar tf_loadondemand_default("sv_loadondemand_default", "0", FCVAR_ARCHIVE | FCVAR_GAMEDLL, "The default value for whether items should be delay loaded (1) or loaded now (0).");
#else
#error "Need to add support for streamable weapons to this configuration, or disable streamable weapons here."
#endif
#endif // WITH_STREAMABLE_WEAPONS

bool CEconItemDefinition::BInitFromKV( KeyValues *pKVItem, CUtlVector<CUtlString> *pVecErrors /* = NULL */ )
{
	// Set standard members
	m_pKVItem = new KeyValues( pKVItem->GetName() );
	MergeDefinitionPrefab( m_pKVItem, pKVItem );
	m_bEnabled = m_pKVItem->GetBool( "enabled" );

    // initializing this one first so that it will be available for all the errors below
    m_pszDefinitionName = m_pKVItem->GetString( "name", NULL );

#if defined( WITH_STREAMABLE_WEAPONS )
    bool bGotDefault = false;
    m_bLoadOnDemand = m_pKVItem->GetBool( "loadondemand", tf_loadondemand_default.GetBool(), &bGotDefault );

    // This logging is useful for tracking down bugs that crop up because we've (possibly) swapped the default value for loadondemand.
    // But it can be removed once we're satisfied there aren't any bugs as a result of the change (when we cleanup WITH_STREAMABLE_WEAPONS).
    if (bGotDefault)
    {
        DevMsg(10, "Item %s received default value for loadondemand\n", m_pszDefinitionName);
    }
#else
    // Keep the old behavior, which is that loadondemand is defaulted to false.
    m_bLoadOnDemand = m_pKVItem->GetBool("loadondemand");
#endif

	m_nDefIndex = Q_atoi( m_pKVItem->GetName() );
	m_unMinItemLevel = (uint32)m_pKVItem->GetInt( "min_ilevel", GetItemSchema()->GetMinLevel() );
	m_unMaxItemLevel = (uint32)m_pKVItem->GetInt( "max_ilevel", GetItemSchema()->GetMaxLevel() );
	m_nDefaultDropQuantity = m_pKVItem->GetInt( "default_drop_quantity", 1 );

	m_nPopularitySeed = m_pKVItem->GetInt( "popularity_seed", 0 );


#if defined(CLIENT_DLL) || defined(GAME_DLL)
	// We read this manually here in the game dlls. The GC reads it below while checking the global schema.
	GetItemSchema()->BGetItemQualityFromName( m_pKVItem->GetString( "item_quality" ), &m_nItemQuality );
	GetItemSchema()->BGetItemQualityFromName( m_pKVItem->GetString( "forced_item_quality" ), &m_nForcedItemQuality );
#endif

	// Check for required fields
	SCHEMA_INIT_CHECK( 
		NULL != m_pKVItem->FindKey( "name" ), 
		"Item definition %s: Missing required field \"name\"", m_pKVItem->GetName() );

	SCHEMA_INIT_CHECK( 
		NULL != m_pKVItem->FindKey( "item_class" ), 
		"Item definition %s: Missing required field \"item_class\"", m_pKVItem->GetName() );

	// Check value ranges
	SCHEMA_INIT_CHECK( 
		m_pKVItem->GetInt( "min_ilevel" ) >= 0, 
		"Item definition %s: \"min_ilevel\" must be greater than or equal to 0", GetDefinitionName() );

	SCHEMA_INIT_CHECK( 
		m_pKVItem->GetInt( "max_ilevel" ) >= 0, 
		"Item definition %s: \"max_ilevel\" must be greater than or equal to 0", GetDefinitionName() );

	// Check for consistency

	// Rarity
	// Get Index from this string and save the index
	if ( m_pKVItem->FindKey( "item_rarity" ) )
	{
		SCHEMA_INIT_CHECK(
			GetItemSchema()->BGetItemRarityFromName( m_pKVItem->GetString( "item_rarity" ), &m_nItemRarity ),
			"Item definition %s: Undefined item_rarity \"%s\"", GetDefinitionName(), m_pKVItem->GetString( "item_rarity" ) );
	}

	if ( m_pKVItem->FindKey( "item_series" ) )
	{
		// Make sure this is a valid series
		SCHEMA_INIT_CHECK(
			GetItemSchema()->BGetItemSeries( m_pKVItem->GetString( "item_series" ), &m_unItemSeries ),
			"Item definition %s: Undefined item_series \"%s\"", GetDefinitionName(), m_pKVItem->GetString( "item_series" ) );
	}

	// Get the item class
	m_pszItemClassname = m_pKVItem->GetString( "item_class", NULL );

	m_pszClassToken = m_pKVItem->GetString( "class_token_id", NULL );
	m_pszSlotToken = m_pKVItem->GetString( "slot_token_id", NULL );

	// expiration data
	const char *pchExpiration = m_pKVItem->GetString( "expiration_date", NULL );
	if( pchExpiration && pchExpiration[0] )
	{
		if ( pchExpiration[0] == '!' )
		{
			m_rtExpiration = GetItemSchema()->GetCustomExpirationDate( &pchExpiration[1] );
			SCHEMA_INIT_CHECK(
				m_rtExpiration != k_RTime32Nil,
				"Unknown/malformed expiration_date string \"%s\" in item %s.", pchExpiration, m_pszDefinitionName );
		}
		else
		{
			m_rtExpiration = CRTime::RTime32FromFmtString( "YYYY-MM-DD hh:mm:ss" , pchExpiration );

			// Check that if we convert back to a string, we get the same value.  Emit an error, but don't fail in the game code
			char rtimeBuf[k_RTimeRenderBufferSize];
			if ( Q_strcmp( CRTime::RTime32ToString( m_rtExpiration, rtimeBuf ), pchExpiration ) != 0 )
			{
#if ( defined( _MSC_VER ) && _MSC_VER >= 1900 )
#define timezone _timezone
#define daylight _daylight
#endif
				Assert( false );
				Warning( "Malformed expiration_date \"%s\" for expiration_date in item %s.  Must be of the form \"YYYY-MM-DD hh:mm:ss\".  Input: %s Output: %s InputTime: %u LocalTime: %u Timezone: %lu\n", pchExpiration, m_pszDefinitionName, pchExpiration, rtimeBuf, m_rtExpiration, CRTime::RTime32TimeCur(), timezone );
			}				
		}
	}

	// Display data
	m_pszItemBaseName = m_pKVItem->GetString( "item_name", "" ); // non-NULL to ensure we can sort
	m_pszItemTypeName = m_pKVItem->GetString( "item_type_name", "" ); // non-NULL to ensure we can sort
	m_pszItemDesc = m_pKVItem->GetString( "item_description", NULL );
	m_pszArmoryDesc = m_pKVItem->GetString( "armory_desc", NULL );
	m_pszInventoryModel = m_pKVItem->GetString( "model_inventory", NULL );
	m_pszInventoryImage = m_pKVItem->GetString( "image_inventory", NULL );

	const char* pOverlay = m_pKVItem->GetString( "image_inventory_overlay", NULL );
	if ( pOverlay )
	{
		m_pszInventoryOverlayImages.AddToTail( pOverlay );
	}
	pOverlay = m_pKVItem->GetString( "image_inventory_overlay2", NULL );
	if ( pOverlay )
	{
		m_pszInventoryOverlayImages.AddToTail( pOverlay );
	}

	m_iInventoryImagePosition[0] = atoi( m_pKVItem->GetString( "image_inventory_pos_x", "0" ) );
	m_iInventoryImagePosition[1] = atoi( m_pKVItem->GetString( "image_inventory_pos_y", "0" ) );
	m_iInventoryImageSize[0] = atoi( m_pKVItem->GetString( "image_inventory_size_w", "128" ) );
	m_iInventoryImageSize[1] = atoi( m_pKVItem->GetString( "image_inventory_size_h", "82" ) );
	m_iInspectPanelDistance = m_pKVItem->GetInt( "inspect_panel_dist", 70 );
	m_pszHolidayRestriction = m_pKVItem->GetString( "holiday_restriction", NULL );
	m_nVisionFilterFlags = m_pKVItem->GetInt( "vision_filter_flags", 0 );
	m_iSubType = atoi( m_pKVItem->GetString( "subtype", "0" ) );
	m_pszBaseDisplayModel = m_pKVItem->GetString( "model_player", NULL );
	m_iDefaultSkin = m_pKVItem->GetInt( "default_skin", -1 );
	m_pszWorldDisplayModel = m_pKVItem->GetString( "model_world", NULL ); // Not the ideal method. c_models are better, but this is to solve a retrofit problem with the sticky launcher.
	m_pszWorldExtraWearableModel = m_pKVItem->GetString( "extra_wearable", NULL ); 
	m_pszWorldExtraWearableViewModel = m_pKVItem->GetString( "extra_wearable_vm", NULL );
	m_pszVisionFilteredDisplayModel = pKVItem->GetString( "model_vision_filtered", NULL );
	m_pszBrassModelOverride = m_pKVItem->GetString( "brass_eject_model", NULL );
	m_bHideBodyGroupsDeployedOnly = m_pKVItem->GetBool( "hide_bodygroups_deployed_only" );
	m_bAttachToHands = m_pKVItem->GetInt( "attach_to_hands", 0 ) != 0;
	m_bAttachToHandsVMOnly = m_pKVItem->GetInt( "attach_to_hands_vm_only", 0 ) != 0;
	m_bProperName = m_pKVItem->GetInt( "propername", 0 ) != 0;
	m_bFlipViewModel = m_pKVItem->GetInt( "flip_viewmodel", 0 ) != 0;
	m_bActAsWearable = m_pKVItem->GetInt( "act_as_wearable", 0 ) != 0;
	m_bActAsWeapon = m_pKVItem->GetInt( "act_as_weapon", 0 ) != 0;
	m_bIsTool = m_pKVItem->GetBool( "is_tool", 0 ) || ( GetItemClass() && !V_stricmp( GetItemClass(), "tool" ) );
	m_iDropType = StringFieldToInt( m_pKVItem->GetString("drop_type"), g_szDropTypeStrings, ARRAYSIZE(g_szDropTypeStrings) );
	m_pszCollectionReference = m_pKVItem->GetString( "collection_reference", NULL );

	// Creation data
	m_bHidden = m_pKVItem->GetInt( "hidden", 0 ) != 0;
	m_bShouldShowInArmory = m_pKVItem->GetInt( "show_in_armory", 0 ) != 0;
	m_bBaseItem = m_pKVItem->GetInt( "baseitem", 0 ) != 0;
	m_pszItemLogClassname = m_pKVItem->GetString( "item_logname", NULL );
	m_pszItemIconClassname = m_pKVItem->GetString( "item_iconname", NULL );
	m_pszDatabaseAuditTable = m_pKVItem->GetString( "database_audit_table", NULL );
	m_bImported = m_pKVItem->FindKey( "import_from" ) != NULL;

	// Tool data
	m_pTool = NULL;
	KeyValues *pToolDataKV = m_pKVItem->FindKey( "tool" );
	if ( pToolDataKV )
	{
		const char *pszType = pToolDataKV->GetString( "type", NULL );
		SCHEMA_INIT_CHECK( pszType != NULL, "Tool '%s' missing required type.", m_pKVItem->GetName() );

		// Common-to-all-tools settings.
		const char *pszUseString = pToolDataKV->GetString( "use_string", NULL );
		const char *pszUsageRestriction = pToolDataKV->GetString( "restriction", NULL );
		KeyValues *pToolUsageKV = pToolDataKV->FindKey( "usage" );

		// Common-to-all-tools usage capability flags.
		item_capabilities_t usageCapabilities = (item_capabilities_t)ITEM_CAP_TOOL_DEFAULT;
		KeyValues *pToolUsageCapsKV = pToolDataKV->FindKey( "usage_capabilities" );
		if ( pToolUsageCapsKV )
		{
			KeyValues *pEntry = pToolUsageCapsKV->GetFirstSubKey();
			while ( pEntry )
			{
				ParseCapability( usageCapabilities, pEntry );
				pEntry = pEntry->GetNextKey();
			}
		}

		m_pTool = GetItemSchema()->CreateEconToolImpl( pszType, pszUseString, pszUsageRestriction, usageCapabilities, pToolUsageKV );
		SCHEMA_INIT_CHECK( m_pTool != NULL, "Unable to create tool implementation for '%s', of type '%s'.", m_pKVItem->GetName(), pszType );
	}

	// Bundle
	KeyValues *pBundleDataKV = m_pKVItem->FindKey( "bundle" );
	if ( pBundleDataKV )
	{
		m_BundleInfo = new bundleinfo_t();
		FOR_EACH_SUBKEY( pBundleDataKV, pKVCurItem )
		{
			CEconItemDefinition *pItemDef = GetItemSchema()->GetItemDefinitionByName( pKVCurItem->GetName() );
			SCHEMA_INIT_CHECK( pItemDef != NULL, "Unable to find item definition '%s' for bundle '%s'.", pKVCurItem->GetName(), m_pszDefinitionName );

			m_BundleInfo->vecItemDefs.AddToTail( pItemDef );
		}

		// Only check for pack bundle if the item is actually a bundle - note that we could do this programatically by checking that all items in the bundle are flagged as a "pack item" - but for now the bundle needs to be explicitly flagged as a pack bundle.
		m_bIsPackBundle = m_pKVItem->GetInt( "is_pack_bundle", 0 ) != 0;
	}

	// capabilities
	m_iCapabilities = (item_capabilities_t)ITEM_CAP_DEFAULT;
	KeyValues *pCapsKV = m_pKVItem->FindKey( "capabilities" );
	if ( pCapsKV )
	{
		KeyValues *pEntry = pCapsKV->GetFirstSubKey();
		while ( pEntry )
		{
			ParseCapability( m_iCapabilities, pEntry );
			pEntry = pEntry->GetNextKey();
		}
	}

	// item_set
	SCHEMA_INIT_CHECK( (!m_pKVItem->GetString( "item_set", NULL )), "Item definition '%s' specifies deprecated \"item_set\" field. Items sets are now specified only in the set itself, not on the definition.", GetDefinitionName() );

	const char *pszSetItemRemapDefIndexName = m_pKVItem->GetString( "set_item_remap", NULL );
	if ( pszSetItemRemapDefIndexName )
	{
		const CEconItemDefinition *pRemapItemDef = GetItemSchema()->GetItemDefinitionByName( pszSetItemRemapDefIndexName );
		m_unSetItemRemapDefIndex = pRemapItemDef ? pRemapItemDef->GetDefinitionIndex() : INVALID_ITEM_DEF_INDEX;

		SCHEMA_INIT_CHECK( m_unSetItemRemapDefIndex != INVALID_ITEM_DEF_INDEX, "Unable to find set item remap definition '%s' for '%s'.", pszSetItemRemapDefIndexName, GetDefinitionName() );
		SCHEMA_INIT_CHECK( m_unSetItemRemapDefIndex != GetDefinitionIndex(), "Unable to set set item remap for definition '%s' to itself.", GetDefinitionName() );
	}
	else
	{
		m_unSetItemRemapDefIndex = GetDefinitionIndex();
	}

	// cache item map names
	m_pszArmoryRemap = m_pKVItem->GetString( "armory_remap", NULL );
	m_pszStoreRemap = m_pKVItem->GetString( "store_remap", NULL );

	m_pszXifierRemapClass = m_pKVItem->GetString( "xifier_class_remap", NULL );
	m_pszBaseFunctionalItemName = m_pKVItem->GetString( "base_item_name", "" );
	m_pszParticleSuffix = m_pKVItem->GetString( "particle_suffix", NULL );

	m_bValidForShuffle = m_pKVItem->GetBool( "valid_for_shuffle", false );
	m_bValidForSelfMade = m_pKVItem->GetBool( "valid_for_self_made", true );

	m_pszRemappedDefItemName = m_pKVItem->GetString( "remapped_item_def_index", NULL );

	// Init our visuals blocks.
	BInitVisualBlockFromKV( m_pKVItem, pVecErrors );

	// Calculate our equip region mask.
	{
		m_unEquipRegionMask = 0;
		m_unEquipRegionConflictMask = 0;

		// Our equip region will come from one of two places -- either we have an "equip_regions" (plural) section,
		// in which case we have any number of regions specified; or we have an "equip_region" (singular) section
		// which will have one and exactly one region. If we have "equip_regions" (plural), we ignore whatever is
		// in "equip_region" (singular).
		//
		// Yes, this is sort of dumb.
		CUtlVector<const char *> vecEquipRegionNames;

		KeyValues *pKVMultiEquipRegions = m_pKVItem->FindKey( "equip_regions" ),
				  *pKVSingleEquipRegion = m_pKVItem->FindKey( "equip_region" );

		// Maybe we have multiple entries?
		if ( pKVMultiEquipRegions )
		{
			for ( KeyValues *pKVRegion = pKVMultiEquipRegions->GetFirstSubKey(); pKVRegion; pKVRegion = pKVRegion->GetNextKey() )
			{
				vecEquipRegionNames.AddToTail( pKVRegion->GetName() );
			}
		}
		// This is our one-and-only-one equip region.
		else if ( pKVSingleEquipRegion )
		{
			const char *pEquipRegionName = pKVSingleEquipRegion->GetString( (const char *)NULL, NULL );
			if ( pEquipRegionName )
			{
				vecEquipRegionNames.AddToTail( pEquipRegionName );
			}
		}

		// For each of our regions, add to our conflict mask both ourself and all the regions
		// that we conflict with.
		FOR_EACH_VEC( vecEquipRegionNames, i )
		{
			const char *pszEquipRegionName = vecEquipRegionNames[i];
			equip_region_mask_t unThisRegionMask = GetItemSchema()->GetEquipRegionMaskByName( pszEquipRegionName );

			SCHEMA_INIT_CHECK(
				unThisRegionMask != 0,
				"Item definition %s: Unable to find equip region mask for region named \"%s\"", GetDefinitionName(), vecEquipRegionNames[i] );

			m_unEquipRegionMask |= GetItemSchema()->GetEquipRegionBitMaskByName( pszEquipRegionName );
			m_unEquipRegionConflictMask |= unThisRegionMask;
		}
	}

	// Single-line static attribute parsing.
	{
		KeyValues *pKVStaticAttrsKey = m_pKVItem->FindKey( "static_attrs" );
		if ( pKVStaticAttrsKey )
		{
			FOR_EACH_SUBKEY( pKVStaticAttrsKey, pKVKey )
			{
				static_attrib_t staticAttrib;

				SCHEMA_INIT_SUBSTEP( staticAttrib.BInitFromKV_SingleLine( GetDefinitionName(), pKVKey, pVecErrors, false ) );
				m_vecStaticAttributes.AddToTail( staticAttrib );

				// Does this attribute specify a tag to apply to this item definition?
				Assert( staticAttrib.GetAttributeDefinition() );
			}
		}
	}

	// Old style attribute parsing. Really only useful now for GC-generated attributes.
	KeyValues *pKVAttribKey = m_pKVItem->FindKey( "attributes" );
	if ( pKVAttribKey )
	{
		FOR_EACH_SUBKEY( pKVAttribKey, pKVKey )
		{
			static_attrib_t staticAttrib;

			SCHEMA_INIT_SUBSTEP( staticAttrib.BInitFromKV_MultiLine( GetDefinitionName(), pKVKey, pVecErrors ) );
			m_vecStaticAttributes.AddToTail( staticAttrib );

			// Does this attribute specify a tag to apply to this item definition?
			Assert( staticAttrib.GetAttributeDefinition() );
		}
	}

	// Initialize tags based on all static attributes for this item.
	for ( const static_attrib_t& attr : m_vecStaticAttributes )
	{
		const econ_tag_handle_t tag = attr.GetAttributeDefinition()->GetItemDefinitionTag();
		if ( tag != INVALID_ECON_TAG_HANDLE )
		{
			m_vecTags.AddToTail( tag );
		}
	}

	// Auto-generate tags based on capabilities.
	for ( int i = 0; i < NUM_ITEM_CAPS; i++ )
	{
		if ( m_iCapabilities & (1 << i) )
		{
			m_vecTags.AddToTail( GetItemSchema()->GetHandleForTag( CFmtStr( "auto__cap_%s", g_Capabilities[i] ).Get() ) );
		}
	}

	// Initialize used-specified tags for this item if present.
	KeyValues *pKVTags = m_pKVItem->FindKey( "tags" );
	if ( pKVTags )
	{
		FOR_EACH_SUBKEY( pKVTags, pKVTag )
		{
			m_vecTags.AddToTail( GetItemSchema()->GetHandleForTag( pKVTag->GetName() ) );
		}
	}

	SCHEMA_INIT_SUBSTEP( BAddLootlistJobFromTemplates( m_pszItemBaseName, m_jobs, m_pKVItem->FindKey( "lootlist_job_templates" ), GetItemSchema(), pVecErrors ) );


	return SCHEMA_INIT_SUCCESS();
}

bool CEconItemDefinition::BPostInit( CUtlVector<CUtlString> *pVecErrors /*= NULL*/ )
{
	const IEconTool *pTool = GetEconTool();
	if ( pTool && !const_cast<IEconTool *>( pTool )->BFinishInitialization() )
	{
	}

	if ( m_pszRemappedDefItemName )
	{
		const CEconItemDefinition *pDef = GetItemSchema()->GetItemDefinitionByName( m_pszRemappedDefItemName );
		SCHEMA_INIT_CHECK( pDef != NULL, "Can't find remapped item %s", m_pszRemappedDefItemName );

		m_nRemappedDefIndex = pDef->GetDefinitionIndex();
	}

	return SCHEMA_INIT_SUCCESS();
}


bool static_attrib_t::BInitFromKV_MultiLine( const char *pszContext, KeyValues *pKVAttribute, CUtlVector<CUtlString> *pVecErrors )
{
	const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinitionByName( pKVAttribute->GetName() );

	SCHEMA_INIT_CHECK( 
		NULL != pAttrDef,
		"Context '%s': Attribute \"%s\" in \"attributes\" did not match any attribute definitions", pszContext, pKVAttribute->GetName() );

	if ( pAttrDef )
	{
		iDefIndex = pAttrDef->GetDefinitionIndex();
			
		const ISchemaAttributeType *pAttrType = pAttrDef->GetAttributeType();
		Assert( pAttrType );

		pAttrType->InitializeNewEconAttributeValue( &m_value );

		const char *pszValue = pKVAttribute->GetString( "value", NULL );
		const bool bSuccessfullyLoadedValue = pAttrType->BConvertStringToEconAttributeValue( pAttrDef, pszValue, &m_value, true );

		SCHEMA_INIT_CHECK(
			bSuccessfullyLoadedValue,
			"Context '%s': Attribute \"%s\" could not parse value \"%s\"!", pszContext, pKVAttribute->GetName(), pszValue ? pszValue : "(null)" );

		SCHEMA_INIT_CHECK(
			!pAttrDef->BIsSetBonusAttribute(),
			"Context '%s': Attribute \"%s\" is a set bonus attribute and not supported here", pszContext, pKVAttribute->GetName() );
	}

	return SCHEMA_INIT_SUCCESS();
}

bool static_attrib_t::BInitFromKV_SingleLine( const char *pszContext, KeyValues *pKVAttribute, CUtlVector<CUtlString> *pVecErrors, bool bEnableTerribleBackwardsCompatibilitySchemaParsingCode /* = true */ )
{
	const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinitionByName( pKVAttribute->GetName() );

	SCHEMA_INIT_CHECK( 
		NULL != pAttrDef,
		"Context '%s': Attribute \"%s\" in \"attributes\" did not match any attribute definitions", pszContext, pKVAttribute->GetName() );

	if ( pAttrDef )
	{
		iDefIndex = pAttrDef->GetDefinitionIndex();
			
		const ISchemaAttributeType *pAttrType = pAttrDef->GetAttributeType();
		Assert( pAttrType );

		pAttrType->InitializeNewEconAttributeValue( &m_value );

		const char *pszValue = pKVAttribute->GetString();
		const bool bSuccessfullyLoadedValue = pAttrType->BConvertStringToEconAttributeValue( pAttrDef, pszValue, &m_value, bEnableTerribleBackwardsCompatibilitySchemaParsingCode );

		SCHEMA_INIT_CHECK(
			bSuccessfullyLoadedValue,
			"Context '%s': Attribute \"%s\" could not parse value \"%s\"!", pszContext, pKVAttribute->GetName(), pszValue ? pszValue : "(null)" );

		SCHEMA_INIT_CHECK(
			!pAttrDef->BIsSetBonusAttribute(),
			"Context '%s': Attribute \"%s\" is a set bonus attribute and not supported here", pszContext, pKVAttribute->GetName() );

	}

	return SCHEMA_INIT_SUCCESS();
}



bool CEconItemDefinition::BInitItemMappings( CUtlVector<CUtlString> *pVecErrors )
{
	// Armory remapping
	if ( m_pszArmoryRemap && m_pszArmoryRemap[0] )
	{
		const CEconItemDefinition *pDef = GetItemSchema()->GetItemDefinitionByName( m_pszArmoryRemap );
		if ( pDef )
		{
			m_iArmoryRemap = pDef->GetDefinitionIndex();
		}

		SCHEMA_INIT_CHECK( 
			pDef != NULL,
			"Item %s: Armory remap definition \"%s\" was not found", m_pszItemBaseName, m_pszArmoryRemap );
	}

	// Store remapping
	if ( m_pszStoreRemap && m_pszStoreRemap[0] )
	{
		const CEconItemDefinition *pDef = GetItemSchema()->GetItemDefinitionByName( m_pszStoreRemap );
		if ( pDef )
		{
			m_iStoreRemap = pDef->GetDefinitionIndex();
		}

		SCHEMA_INIT_CHECK(
			pDef != NULL,
			"Item %s: Store remap definition \"%s\" was not found", m_pszItemBaseName, m_pszStoreRemap );
	}

	return SCHEMA_INIT_SUCCESS();
}

const char* CEconItemDefinition::GetIconURL( const char* pszKey ) const
{
	auto idx = m_pDictIcons->Find( pszKey );
	if ( idx == m_pDictIcons->InvalidIndex() )
	{
		return NULL;
	}

	return (*m_pDictIcons)[ idx ];
}

//-----------------------------------------------------------------------------
// Purpose: Generate and return a random level according to whatever leveling
//			curve this definition uses.
//-----------------------------------------------------------------------------
uint32 CEconItemDefinition::RollItemLevel( void ) const
{
	return RandomInt( GetMinLevel(), GetMaxLevel() );
}

const char *CEconItemDefinition::GetFirstSaleDate() const
{
	return GetDefinitionString( "first_sale_date", "1960/00/00" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconItemDefinition::IterateAttributes( IEconItemAttributeIterator *pIterator ) const
{
	FOR_EACH_VEC( GetStaticAttributes(), i )
	{
		const static_attrib_t& staticAttrib = GetStaticAttributes()[i];
		

		const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinition( staticAttrib.iDefIndex );
		if ( !pAttrDef )
			continue;

		const ISchemaAttributeType *pAttrType = pAttrDef->GetAttributeType();
		Assert( pAttrType );

		if ( !pAttrType->OnIterateAttributeValue( pIterator, pAttrDef, staticAttrib.m_value ) )
			return;
	}
}

#if defined(CLIENT_DLL) || defined(GAME_DLL)
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Activity CEconItemDefinition::GetActivityOverride( int iTeam, Activity baseAct ) const
{
	int iAnims = GetNumAnimations( iTeam );
	for ( int i = 0; i < iAnims; i++ )
	{
		animation_on_wearable_t *pData = GetAnimationData( iTeam, i );
		if ( !pData )
			continue;
		if ( pData->iActivity == kActivityLookup_Unknown )
		{
			pData->iActivity = ActivityList_IndexForName( pData->pszActivity );
		}

		if ( pData->iActivity == baseAct )
		{
			if ( pData->iReplacement == kActivityLookup_Unknown )
			{
				pData->iReplacement = ActivityList_IndexForName( pData->pszReplacement );
			}

			if ( pData->iReplacement > 0 )
			{
				return (Activity) pData->iReplacement;
			}
		}
	}

	return baseAct;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CEconItemDefinition::GetActivityOverride( int iTeam, const char *pszActivity ) const
{
	int iAnims = GetNumAnimations( iTeam );
	for ( int i = 0; i < iAnims; i++ )
	{
		animation_on_wearable_t *pData = GetAnimationData( iTeam, i );
		if ( Q_stricmp( pszActivity, pData->pszActivity ) == 0 )
			return pData->pszReplacement;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CEconItemDefinition::GetReplacementForActivityOverride( int iTeam, Activity baseAct ) const
{
	int iAnims = GetNumAnimations( iTeam );
	for ( int i = 0; i < iAnims; i++ )
	{
		animation_on_wearable_t *pData = GetAnimationData( iTeam, i );
		if ( pData->iActivity == kActivityLookup_Unknown )
		{
			pData->iActivity = ActivityList_IndexForName( pData->pszActivity );
		}
		if ( pData && pData->iActivity == baseAct )
			return pData->pszReplacement;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the content for this item view should be streamed. If false,
//			it should be preloaded.
//-----------------------------------------------------------------------------

// DO NOT MERGE THIS CONSOLE VARIABLE TO REL WE SHOULD NOT SHIP THIS OH GOD

bool CEconItemDefinition::IsContentStreamable() const
{
	if ( !BLoadOnDemand() )
		return false;
		
	return true;
}
#endif // defined(CLIENT_DLL) || defined(GAME_DLL)

RETURN_ATTRIBUTE_STRING_F( CEconItemDefinition::GetIconDisplayModel, "icon display model", m_pszWorldDisplayModel );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTimedItemRewardDefinition::CTimedItemRewardDefinition( void )
:	m_unMinFreq( 0 ),
	m_unMaxFreq( UINT_MAX ),
	m_flChance( 0.0f ),
	m_pLootList( NULL ),
	m_iRequiredItemDef(INVALID_ITEM_DEF_INDEX)
{
}


//-----------------------------------------------------------------------------
// Purpose:	Copy constructor
//-----------------------------------------------------------------------------
CTimedItemRewardDefinition::CTimedItemRewardDefinition( const CTimedItemRewardDefinition &that )
{
	(*this) = that;
}


//-----------------------------------------------------------------------------
// Purpose:	Operator=
//-----------------------------------------------------------------------------
CTimedItemRewardDefinition &CTimedItemRewardDefinition::operator=( const CTimedItemRewardDefinition &rhs )
{
	m_unMinFreq = rhs.m_unMinFreq;
	m_unMaxFreq = rhs.m_unMaxFreq;
	m_flChance = rhs.m_flChance;
	m_criteria = rhs.m_criteria;
	m_pLootList = rhs.m_pLootList;
	m_iRequiredItemDef = rhs.m_iRequiredItemDef;

	return *this;
}


//-----------------------------------------------------------------------------
// Purpose:	Initialize the attribute definition
// Input:	pKVTimedReward - The KeyValues representation of the timed reward
//			schema - The overall item schema
//			pVecErrors - An optional vector that will contain error messages if 
//				the init fails.
// Output:	True if initialization succeeded, false otherwise
//-----------------------------------------------------------------------------
bool CTimedItemRewardDefinition::BInitFromKV( KeyValues *pKVTimedReward, CUtlVector<CUtlString> *pVecErrors /* = NULL */ )
{
	// Parse the basic values
	m_flChance = pKVTimedReward->GetFloat( "pctChance" );
	m_unMinFreq = pKVTimedReward->GetInt( "value_min", 0 );
	m_unMaxFreq = pKVTimedReward->GetInt( "value_max", UINT_MAX );
	m_iRequiredItemDef = INVALID_ITEM_DEF_INDEX;

	const char *pszRequiredItem = pKVTimedReward->GetString( "required_item", NULL );
	if ( pszRequiredItem )
	{
		// Find the ItemDef
		const CEconItemDefinition *pDef = GetItemSchema()->GetItemDefinitionByName( pszRequiredItem );
		SCHEMA_INIT_CHECK( pDef != NULL, "Invalid Item Def Required for a for TimedReward Definition");
		m_iRequiredItemDef = pDef->GetDefinitionIndex();
	}

	// Check required fields
	SCHEMA_INIT_CHECK( 
		NULL != pKVTimedReward->FindKey( "value_min" ), 
		"Time reward %s: Missing required field \"value_min\"", pKVTimedReward->GetName() );
	SCHEMA_INIT_CHECK( 
		NULL != pKVTimedReward->FindKey( "value_max" ), 
		"Time reward %s: Missing required field \"value_max\"", pKVTimedReward->GetName() );

	SCHEMA_INIT_CHECK( 
		NULL != pKVTimedReward->FindKey( "pctChance" ), 
		"Time reward %s: Missing required field \"pctChance\"", pKVTimedReward->GetName() );

	SCHEMA_INIT_CHECK(
		NULL == pKVTimedReward->FindKey( "criteria" ),
		"Time reward %s: \"criteria\" is no longer supported. Restructure as \"loot_list\"?", pKVTimedReward->GetName() );
		
	SCHEMA_INIT_CHECK( 
		NULL != pKVTimedReward->FindKey( "loot_list" ), 
		"Time reward %s: Missing required field \"loot_list\" ", pKVTimedReward->GetName() );

	// Parse the loot list
	const char *pszLootList = pKVTimedReward->GetString("loot_list", NULL);
	if ( pszLootList && pszLootList[0] )
	{
		m_pLootList = GetItemSchema()->GetLootListByName( pszLootList );

		// Make sure the item index is correct because we use this index as a reference
		SCHEMA_INIT_CHECK( 
			NULL != m_pLootList,
			"Time Reward %s: loot_list (%s) does not exist", pKVTimedReward->GetName(), pszLootList );
	}

	// Other integrity checks
	SCHEMA_INIT_CHECK(
		m_flChance >= 0.0f,
		"Time Reward %s: pctChance (%f) must be greater or equal to 0.0", pKVTimedReward->GetName(), m_flChance );

	SCHEMA_INIT_CHECK(
		m_flChance <= 1.0f,
		"Time Reward %s: pctChance (%f) must be less than or equal to 1.0", pKVTimedReward->GetName(), m_flChance );

	SCHEMA_INIT_CHECK(
		pKVTimedReward->GetInt( "value_min" ) > 0, 
		"Time Reward %s: value_min (%d) must be greater than 0", pKVTimedReward->GetName(), m_unMinFreq );
	SCHEMA_INIT_CHECK(
		pKVTimedReward->GetInt( "value_max" ) > 0, 
		"Time Reward %s: value_max (%d) must be greater than 0", pKVTimedReward->GetName(), m_unMaxFreq );
	SCHEMA_INIT_CHECK(
		(m_unMaxFreq >= m_unMinFreq), 
		"Time Reward %s: value_max (%d) must be greater than or equal to value_min (%d)", pKVTimedReward->GetName(), m_unMaxFreq, m_unMinFreq );

	return SCHEMA_INIT_SUCCESS();
}


//-----------------------------------------------------------------------------
// Purpose: Adds a foreign item definition to local definition mapping for a 
//			foreign app
//-----------------------------------------------------------------------------
void CForeignAppImports::AddMapping( uint16 unForeignDefIndex, const CEconItemDefinition *pDefn )
{
	m_mapDefinitions.InsertOrReplace( unForeignDefIndex, pDefn );
}


//-----------------------------------------------------------------------------
// Purpose: Adds a foreign item definition to local definition mapping for a 
//			foreign app
//-----------------------------------------------------------------------------
const CEconItemDefinition *CForeignAppImports::FindMapping( uint16 unForeignDefIndex ) const
{
	int i = m_mapDefinitions.Find( unForeignDefIndex );
	if( m_mapDefinitions.IsValidIndex( i ) )
		return m_mapDefinitions[i];
	else
		return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:	Constructor
//-----------------------------------------------------------------------------
CEconItemSchema::CEconItemSchema( )
: 	m_unResetCount( 0 )
,	m_pKVRawDefinition( NULL )
,	m_mapItemSeries( DefLessFunc(int) )
,	m_mapRarities( DefLessFunc(int) )
,	m_mapQualities( DefLessFunc(int) )
,	m_mapAttributes( DefLessFunc(int) )
,	m_mapRecipes( DefLessFunc(int) )
,	m_mapQuestObjectives( DefLessFunc(int) )
,	m_mapItemsSorted( DefLessFunc(int) )
,	m_mapToolsItems( DefLessFunc(int) )
,	m_mapPaintKitTools( DefLessFunc(uint32) )
,	m_mapBaseItems( DefLessFunc(int) )
,	m_unVersion( 0 )
#if defined(CLIENT_DLL) || defined(GAME_DLL)
,	m_pDefaultItemDefinition( NULL )
#endif
,	m_dictItemSets( k_eDictCompareTypeCaseInsensitive )
,	m_dictItemCollections( k_eDictCompareTypeCaseInsensitive )
,	m_dictOperationDefinitions( k_eDictCompareTypeCaseInsensitive )
,   m_dictLootLists( k_eDictCompareTypeCaseInsensitive )
,	m_mapRevolvingLootLists( DefLessFunc(int) )
,	m_dictDefinitionPrefabs( k_eDictCompareTypeCaseInsensitive )
,	m_mapAchievementRewardsByData( DefLessFunc( uint32 ) )
,	m_mapAttributeControlledParticleSystems( DefLessFunc(int) )
,	m_dictDefaultBodygroupState( k_eDictCompareTypeCaseInsensitive )
#if   defined(CLIENT_DLL) || defined(GAME_DLL)
,	m_pDelayedSchemaData( NULL )
#endif
,	m_mapKillEaterScoreTypes( DefLessFunc( unsigned int ) )
,	m_mapCommunityMarketDefinitionIndexRemap( DefLessFunc( item_definition_index_t ) )
#ifdef CLIENT_DLL
,	m_mapSteamPackageLocalizationTokens( DefLessFunc( uint32 ) )
#endif
{
	Reset();
}

CQuestObjectiveDefinition *CEconItemSchema::CreateQuestDefinition()
{
	return new CQuestObjectiveDefinition; 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
IEconTool *CEconItemSchema::CreateEconToolImpl( const char *pszToolType, const char *pszUseString, const char *pszUsageRestriction, item_capabilities_t unCapabilities, KeyValues *pUsageKV )
{
	if ( pszToolType )
	{
		if ( !V_stricmp( pszToolType, "duel_minigame" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;
			if ( unCapabilities != ITEM_CAP_NONE )		return NULL;
			if ( pUsageKV )								return NULL;
				
			return new CEconTool_DuelingMinigame( pszToolType, pszUseString );
		}

		if ( !V_stricmp( pszToolType, "noise_maker" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;
			if ( unCapabilities != ITEM_CAP_NONE )		return NULL;
			if ( pUsageKV )								return NULL;

			return new CEconTool_Noisemaker( pszToolType, pszUseString );
		}

		if ( !V_stricmp( pszToolType, "wrapped_gift" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;

			return new CEconTool_WrappedGift( pszToolType, pszUseString, unCapabilities, pUsageKV );
		}

		if ( !V_stricmp( pszToolType, "backpack_expander" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;
			if ( unCapabilities != ITEM_CAP_NONE )		return NULL;

			return new CEconTool_BackpackExpander( pszToolType, pszUseString, pUsageKV );
		}

		if ( !V_stricmp( pszToolType, "account_upgrade_to_premium" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;
			if ( unCapabilities != ITEM_CAP_NONE )		return NULL;
			if ( pUsageKV )								return NULL;

			return new CEconTool_AccountUpgradeToPremium( pszToolType, pszUseString );
		}

		if ( !V_stricmp( pszToolType, "claimcode" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;
			if ( unCapabilities != ITEM_CAP_NONE )		return NULL;

			return new CEconTool_ClaimCode( pszToolType, pszUseString, pUsageKV );
		}

		if ( !V_stricmp( pszToolType, "gift" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;
			if ( unCapabilities != ITEM_CAP_NONE )		return NULL;

			return new CEconTool_Gift( pszToolType, pszUseString, pUsageKV );
		}

		if ( !V_stricmp( pszToolType, "keyless_case" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;
			if ( unCapabilities != ITEM_CAP_NONE )		return NULL;

			return new CEconTool_KeylessCase( pszToolType, pszUseString );
		}

		if ( !V_stricmp( pszToolType, "paint_can" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;
			if ( pUsageKV )								return NULL;

			return new CEconTool_PaintCan( pszToolType, unCapabilities );
		}

		if ( !V_stricmp( pszToolType, "name" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;
			if ( pUsageKV )								return NULL;

			return new CEconTool_NameTag( pszToolType, unCapabilities );
		}

		if ( !V_stricmp( pszToolType, "desc" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;
			if ( pUsageKV )								return NULL;

			return new CEconTool_DescTag( pszToolType, unCapabilities );
		}

		if ( !V_stricmp( pszToolType, "decoder_ring" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pUsageKV )								return NULL;

			return new CEconTool_CrateKey( pszToolType, pszUsageRestriction, unCapabilities );
		}

		if ( !V_stricmp( pszToolType, "customize_texture_item" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;
			if ( pUsageKV )								return NULL;

			return new CEconTool_CustomizeTexture( pszToolType, unCapabilities );
		}

		if ( !V_stricmp( pszToolType, "gift_wrap" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;

			return new CEconTool_GiftWrap( pszToolType, pszUseString, unCapabilities, pUsageKV );
		}

		if ( !V_stricmp( pszToolType, "wedding_ring" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;
			if ( pUsageKV )								return NULL;

			return new CEconTool_WeddingRing( pszToolType, pszUseString, unCapabilities );
		}

		if ( !V_stricmp( pszToolType, "strange_part" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;
			
			return new CEconTool_StrangePart( pszToolType, pszUseString, unCapabilities, pUsageKV );
		}

		if ( !V_stricmp( pszToolType, "strange_part_restriction" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;
			if ( !pUsageKV )							return NULL;		// required
			
			return new CEconTool_StrangePartRestriction( pszToolType, pszUseString, unCapabilities, pUsageKV );
		}

		if ( !V_stricmp( pszToolType, "apply_custom_attrib" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;
			
			return new CEconTool_UpgradeCard( pszToolType, pszUseString, unCapabilities, pUsageKV );
		}

		if ( !V_stricmp( pszToolType, "strangifier" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;

			return new CEconTool_Strangifier( pszToolType, pszUseString, unCapabilities, pUsageKV );
		}

		if ( !V_stricmp( pszToolType, "killstreakifier" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;

			return new CEconTool_KillStreakifier( pszToolType, pszUseString, unCapabilities, pUsageKV );
		}

		if( !V_stricmp( pszToolType, "dynamic_recipe" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;
			
			return new CEconTool_ItemDynamicRecipe( pszToolType, pszUseString, unCapabilities, pUsageKV );
		}

		if ( !V_stricmp( pszToolType, "item_eater_recharger" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;

			return new CEconTool_ItemEaterRecharger( pszToolType, pszUseString, unCapabilities, pUsageKV );
		}

		if ( !V_stricmp( pszToolType, "class_transmogrifier" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;

			return new CEconTool_ClassTransmogrifier( pszToolType, pszUseString, unCapabilities, pUsageKV );
		}

		if ( !V_stricmp( pszToolType, "duck_token" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;
			if ( pUsageKV )								return NULL;

			return new CEconTool_DuckToken( pszToolType, unCapabilities );
		}

		if ( !V_stricmp( pszToolType, "grant_operation_pass" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;

			return new CEconTool_GrantOperationPass( pszToolType, pszUseString, unCapabilities, pUsageKV );
		}

		if ( !V_stricmp( pszToolType, "strange_count_transfer" ) )
		{
			// Error checking -- make sure we aren't setting properties in the schema that we don't support.
			if ( pszUsageRestriction )					return NULL;
			if ( pUsageKV )								return NULL;

			return new CEconTool_StrangeCountTransfer( pszToolType, unCapabilities );
		}

		if ( !V_stricmp( pszToolType, "paintkit_weapon_festivizer" ) )
		{
			return new CEconTool_Festivizer( pszToolType, pszUseString, unCapabilities, pUsageKV );
		}

		if ( !V_stricmp( pszToolType, "unusualifier" ) )
		{
			return new CEconTool_Unusualifier( pszToolType, pszUseString, unCapabilities, pUsageKV );
		}

		if ( !V_stricmp( pszToolType, "paintkit" ) )
		{
			return new CEconTool_PaintKit( pszToolType, pszUseString, unCapabilities );
		}
	}

	// Default behavior.
	return new CEconTool_Default( pszToolType, pszUseString, pszUsageRestriction, unCapabilities );
}

CItemSelectionCriteria *CEconItemSchema::CreateItemCriteria( const char *pszContext, KeyValues *pItemCriteriaKV, CUtlVector<CUtlString> *pVecErrors /*= NULL*/ )
{
	CItemSelectionCriteria *pCriteria = new CItemSelectionCriteria;
	if ( pCriteria->BInitFromKV( pItemCriteriaKV ) )
	{
		return pCriteria;
	}

	delete pCriteria;
	return NULL;
}

random_attrib_t	*CEconItemSchema::CreateRandomAttribute( const char *pszContext, KeyValues *pRandomAttributesKV, CUtlVector<CUtlString> *pVecErrors /*= NULL*/ )
{
	float flTotalAttributeWeight = 0.f;
	LootListAttributeVec_t randomAttributes;
	ItemSelectionCriteriaVec_t vecCriteria;
	bool bAllowDuplicate = pRandomAttributesKV->GetBool( "allow_duplicate", false );

	FOR_EACH_TRUE_SUBKEY( pRandomAttributesKV, pKVAttribute )
	{
		const char *pszName = pKVAttribute->GetName();

		// Quick block list of attrs that have equal weight
		if ( !V_strcmp( pszName, "item_criteria_templates" ) )
		{
			FOR_EACH_VALUE( pKVAttribute, pKVCriteria )
			{
				const char *pszCriteriaName = pKVCriteria->GetName();
				int index = m_dictItemCriteriaTemplates.Find( pszCriteriaName );
				if ( index == m_dictItemCriteriaTemplates.InvalidIndex() )
				{
					CUtlString msg;
					msg.Format( "Failed to find %s from 'item_criteria_templates'", pszCriteriaName );
					if ( pVecErrors )
					{
						pVecErrors->AddToTail( msg );
					}
					else
					{
						AssertMsg( index != m_dictItemCriteriaTemplates.InvalidIndex(), "%s", msg.String() );
					}

					return NULL;
				}

				CItemSelectionCriteria *pCriteria = m_dictItemCriteriaTemplates[index];
				vecCriteria.AddToTail( pCriteria );
			}
		}
		else if ( !V_strcmp( pszName, "is_even_chance_attr" ) )
		{
			FOR_EACH_VALUE( pKVAttribute, pKVListItem )
			{
				const CEconItemAttributeDefinition *pDef = GetAttributeDefinitionByName( pKVListItem->GetName() );
				if ( pDef == NULL )
				{
					CUtlString msg;
					msg.Format( "Attribute definition \"%s\" was not found", pszName );
					if ( pVecErrors )
					{
						pVecErrors->AddToTail( msg );
					}
					else
					{
						AssertMsg( pDef != NULL, "%s", msg.String() );
					}

					return NULL;
				}

				lootlist_attrib_t lootListAttrib;
				if ( !lootListAttrib.m_staticAttrib.BInitFromKV_SingleLine( __FUNCTION__, pKVListItem, pVecErrors, false ) )
				{
					if ( pVecErrors )
					{
						pVecErrors->AddToTail( CFmtStr( " %s: error initializing line-item attribute from lootlist definition (possible attr template).\n", __FUNCTION__ ).Get() );
					}
					return NULL;
				}
				// Weight is set to 1 for even chance attr				
				lootListAttrib.m_flWeight = 1.0f;
				flTotalAttributeWeight += 1.0f;
				randomAttributes.AddToTail( lootListAttrib );
			}
		}
		else if ( !V_strcmp( pszName, "even_chance_templates" ) )
		{
			FOR_EACH_VALUE( pKVAttribute, pKVTemplate )
			{
				random_attrib_t *pTemplate = GetRandomAttributeTemplateByName( pKVTemplate->GetName() );
				if ( pTemplate == NULL )
				{
					CUtlString msg;
					msg.Format( "Failed to parse 'templates' blog from '%s'. '%s' attribute template doesn't exist", pRandomAttributesKV->GetName(), pKVTemplate->GetName() );
					if ( pVecErrors )
					{
						pVecErrors->AddToTail( msg );
					}
					else
					{
						AssertMsg( pTemplate != NULL, "%s", msg.String() );
					}

					return NULL;
				}

				// just add template to the list and sum the total weight
				randomAttributes.AddVectorToTail( pTemplate->m_RandomAttributes );
				vecCriteria.AddVectorToTail( pTemplate->m_vecCriteria );
				flTotalAttributeWeight += pTemplate->m_flTotalAttributeWeight;
			}
		}
		else
		{
			const CEconItemAttributeDefinition *pDef = GetAttributeDefinitionByName( pszName );
			if ( pDef == NULL )
			{
				CUtlString msg;
				msg.Format( "Attribute definition \"%s\" was not found", pszName );
				if ( pVecErrors )
				{
					pVecErrors->AddToTail( msg );
				}
				else
				{
					AssertMsg( pDef != NULL, "%s", msg.String() );
				}

				return NULL;
			}

			lootlist_attrib_t lootListAttrib;
			if ( !lootListAttrib.BInitFromKV( pszContext, pKVAttribute, *this, pVecErrors ) )
			{
				return NULL;
			}

			flTotalAttributeWeight += lootListAttrib.m_flWeight;
			randomAttributes.AddToTail( lootListAttrib );
		}
	}

	// make sure bAllowDuplicate flag gets chain to the top of the template stack
	FOR_EACH_VEC( randomAttributes, i )
	{
		randomAttributes[i].m_bAllowDuplicate |= bAllowDuplicate;
	}

	random_attrib_t *pRandomAttr = new random_attrib_t;
	pRandomAttr->m_flTotalAttributeWeight = flTotalAttributeWeight;
	pRandomAttr->m_RandomAttributes = randomAttributes;
	if ( vecCriteria.Count() > 0 )
	{
		pRandomAttr->m_vecCriteria = vecCriteria;

		// all children should know about parent's criteria
		FOR_EACH_VEC( pRandomAttr->m_RandomAttributes, i )
		{
			pRandomAttr->m_RandomAttributes[i].m_pVecCriteria = &pRandomAttr->m_vecCriteria;
		}
	}

	return pRandomAttr;
}

CLootlistJob *CEconItemSchema::CreateLootlistJob( const char *pszContext, KeyValues *pLootlistJobKV, CUtlVector<CUtlString> *pVecErrors /*= NULL*/ )
{
	CLootlistJob *pJob = new CLootlistJob( pszContext );
	if ( pJob->BInitFromKV( pszContext, pLootlistJobKV, *this, pVecErrors ) )
	{
		return pJob;
	}

	delete pJob;
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose:	Resets the schema to before BInit was called
//-----------------------------------------------------------------------------
void CEconItemSchema::Reset( void )
{
	++m_unResetCount;

	m_unFirstValidClass = 0;
	m_unLastValidClass = 0;
	m_unAccoutClassIndex = 0;
	m_unFirstValidClassItemSlot = 0;
	m_unLastValidClassItemSlot = 0;
	m_unFirstValidAccountItemSlot = 0;
	m_unLastValidAccountItemSlot = 0;
	m_unNumItemPresets = 0;
	m_unMinLevel = 0;
	m_unMaxLevel = 0;
	m_unVersion = 0;
	m_unSumQualityWeights = 0;
	FOR_EACH_VEC( m_vecAttributeTypes, i )
	{
		delete m_vecAttributeTypes[i].m_pAttrType;
	}
	m_vecAttributeTypes.Purge();
	m_mapItems.PurgeAndDeleteElements();
	m_mapItems.Purge();
	m_mapRarities.Purge();
	m_mapQualities.Purge();
	m_mapItemsSorted.Purge();
	m_mapToolsItems.Purge();
	m_mapPaintKitTools.Purge();
	m_mapBaseItems.Purge();
	m_mapRecipes.PurgeAndDeleteElements();
	m_vecTimedRewards.Purge();
	m_dictItemSets.PurgeAndDeleteElements();
	m_dictLootLists.PurgeAndDeleteElements();
	m_dictItemCriteriaTemplates.PurgeAndDeleteElements();
	m_dictRandomAttributeTemplates.PurgeAndDeleteElements();
	m_dictLootlistJobTemplates.PurgeAndDeleteElements();
	m_mapAttributeControlledParticleSystems.Purge();
	m_vecAttributeControlledParticleSystemsCosmetics.Purge();
	m_vecAttributeControlledParticleSystemsWeapons.Purge();
	m_vecAttributeControlledParticleSystemsTaunts.Purge();

	m_mapAttributes.Purge();
	if ( m_pKVRawDefinition )
	{
		m_pKVRawDefinition->deleteThis();
		m_pKVRawDefinition = NULL;
	}

#if defined(CLIENT_DLL) || defined(GAME_DLL)
	delete m_pDefaultItemDefinition;
	m_pDefaultItemDefinition = NULL;
#endif

	FOR_EACH_MAP_FAST( m_mapRecipes, i )
	{
		delete m_mapRecipes[i];
	}

	for ( int idx = m_dictDefinitionPrefabs.First(); m_dictDefinitionPrefabs.IsValidIndex( idx ); idx = m_dictDefinitionPrefabs.Next( idx ) )
	{
		m_dictDefinitionPrefabs[idx]->deleteThis();
	}
	m_dictDefinitionPrefabs.Purge();

	m_vecEquipRegionsList.Purge();
	m_vecItemLevelingData.PurgeAndDeleteElements();

	m_dictStringTable.PurgeAndDeleteElements();
	m_mapCommunityMarketDefinitionIndexRemap.Purge();
}


//-----------------------------------------------------------------------------
// Purpose:	Operator=
//-----------------------------------------------------------------------------
CEconItemSchema &CEconItemSchema::operator=( CEconItemSchema &rhs )
{
	Reset();
	BInitSchema( rhs.m_pKVRawDefinition );
	return *this;
}

//-----------------------------------------------------------------------------
// Initializes the schema, given KV filename
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInit( const char *fileName, const char *pathID, CUtlVector<CUtlString> *pVecErrors /* = NULL */)
{
	Reset();

	// Read the raw data
	CUtlBuffer bufRawData;
	bool bReadFileOK = g_pFullFileSystem->ReadFile( fileName, pathID, bufRawData );
	SCHEMA_INIT_CHECK( bReadFileOK, "Cannot load file '%s'", fileName );

	// Do we need to check the signature?
	#if defined(TF_DLL) || defined(TF_CLIENT_DLL)
	{
		bool bSignatureValid = TF_CheckSignature(fileName, pathID, bufRawData);
		SCHEMA_INIT_CHECK(bSignatureValid, "'%s' is corrupt.  Please verify your local game files.  (https://support.steampowered.com/kb_article.php?ref=2037-QEUH-3335)", fileName);
		
	}
	#endif

	// Compute version hash
	CSHA1 sha1;
	sha1.Update( (unsigned char *)bufRawData.Base(), bufRawData.Size() );
	sha1.Final();
	sha1.GetHash( m_schemaSHA.m_shaDigest );

	// Wrap it with a text buffer reader
	CUtlBuffer bufText( bufRawData.Base(), bufRawData.TellPut(), CUtlBuffer::READ_ONLY | CUtlBuffer::TEXT_BUFFER );

	// Use the standard init path
	return BInitTextBuffer( bufText, pVecErrors );
}

//-----------------------------------------------------------------------------
// Initializes the schema, given KV in binary form
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitBinaryBuffer( CUtlBuffer &buffer, CUtlVector<CUtlString> *pVecErrors /* = NULL */ )
{
	Reset();
	m_pKVRawDefinition = new KeyValues( "CEconItemSchema" );
	if ( m_pKVRawDefinition->ReadAsBinary( buffer ) )
	{
		return BInitSchema( m_pKVRawDefinition, pVecErrors )
			&& BPostSchemaInit( pVecErrors );
	}
	if ( pVecErrors )
	{
		pVecErrors->AddToTail( "Error parsing keyvalues" );
	}
	return false;
}

unsigned char g_sha1ItemSchemaText[ k_cubHash ];

//-----------------------------------------------------------------------------
// Initializes the schema, given KV in text form
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitTextBuffer( CUtlBuffer &buffer, CUtlVector<CUtlString> *pVecErrors /* = NULL */ )
{
	// Save off the hash into a global variable, so VAC can check it
	// later
	GenerateHash( g_sha1ItemSchemaText, buffer.Base(), buffer.TellPut() );

	Reset();
	m_pKVRawDefinition = new KeyValues( "CEconItemSchema" );
	if ( m_pKVRawDefinition->LoadFromBuffer( NULL, buffer ) )
	{
		return BInitSchema( m_pKVRawDefinition, pVecErrors )
			&& BPostSchemaInit( pVecErrors );
	}
	if ( pVecErrors )
	{
		pVecErrors->AddToTail( "Error parsing keyvalues" );
	}
	return false;
}

bool CEconItemSchema::DumpItems ( const char *fileName, const char *pathID )
{
	// create a write file
	FileHandle_t f = g_pFullFileSystem->Open(fileName, "wb", pathID);

	if ( f == FILESYSTEM_INVALID_HANDLE )
	{
		DevMsg(1, "CEconItemSchema::DumpItems: couldn't open file \"%s\" in path \"%s\".\n", 
			fileName?fileName:"NULL", pathID?pathID:"NULL" );
		return false;
	}

	CUtlSortVector< KeyValues*, CUtlSortVectorKeyValuesByName > vecSortedItems;

	FOR_EACH_MAP_FAST( m_mapItems, i )
	{
		vecSortedItems.InsertNoSort( m_mapItems[ i ]->GetRawDefinition() );
	}
	vecSortedItems.RedoSort();

	CUtlBuffer buf;
	FOR_EACH_VEC( vecSortedItems, i )
	{
		vecSortedItems[i]->RecursiveSaveToFile( buf, 0, true );
	}

	int iBufSize = buf.GetBytesRemaining();
	bool bSuccess = false;
	if ( g_pFullFileSystem->Write(buf.PeekGet(), iBufSize, f) == iBufSize )
		bSuccess = true;

	g_pFullFileSystem->Close(f);

	return bSuccess;
}

//-----------------------------------------------------------------------------
// Called once the price sheet's been loaded
//-----------------------------------------------------------------------------


#if defined(CLIENT_DLL) || defined(GAME_DLL)
//-----------------------------------------------------------------------------
// Set up the buffer to use to reinitialize our schema next time we can do so safely.
//-----------------------------------------------------------------------------
bool CEconItemSchema::MaybeInitFromBuffer( IDelayedSchemaData *pDelayedSchemaData )
{
	bool bDidInit = false;

	// Use whatever our most current data block is.
	if ( m_pDelayedSchemaData )
	{
		delete m_pDelayedSchemaData;
	}

	m_pDelayedSchemaData = pDelayedSchemaData;

#ifdef CLIENT_DLL
	// If we aren't in a game we can parse immediately now.
	if ( !engine->IsInGame() )
	{
		BInitFromDelayedBuffer();
		bDidInit = true;
	}
#endif // CLIENT_DLL

	return bDidInit;
}

//-----------------------------------------------------------------------------
// We're in a safe place to change the contents of the schema, so do so and clean
// up whatever memory we were using.
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitFromDelayedBuffer()
{
	if ( !m_pDelayedSchemaData )
		return true;

	bool bSuccess = m_pDelayedSchemaData->InitializeSchema( this );
	delete m_pDelayedSchemaData;
	m_pDelayedSchemaData = NULL;
	 
	// We just got a new schema.  We need another PostInit()
	ItemSystem()->PostInit();

	return bSuccess;
}
#endif // !GC_DLL

static void CalculateKeyValuesCRCRecursive( KeyValues *pKV, CRC32_t *crc, bool bIgnoreName = false )
{
	// Hash in the key name in LOWERCASE.  Keyvalues files are not deterministic due
	// to the case insensitivity of the keys and the dependence on the existing
	// state of the name table upon entry.
	if ( !bIgnoreName )
	{
		const char *s = pKV->GetName();  
		for (;;)
		{
			unsigned char x = tolower(*s);
			CRC32_ProcessBuffer( crc, &x, 1 ); // !SPEED! This is slow, but it works.
			if (*s == '\0') break;
			++s;
		}
	}

	// Now hash in value, depending on type
	// !FIXME! This is not byte-order independent!
	switch ( pKV->GetDataType() )
	{
	case KeyValues::TYPE_NONE:
		{
			FOR_EACH_SUBKEY( pKV, pChild )
			{
				CalculateKeyValuesCRCRecursive( pChild, crc );
			}
			break;
		}
	case KeyValues::TYPE_STRING:
		{
			const char *val = pKV->GetString();
			CRC32_ProcessBuffer( crc, val, strlen(val)+1 );
			break;
		}

	case KeyValues::TYPE_INT:
		{
			int val = pKV->GetInt();
			CRC32_ProcessBuffer( crc, &val, sizeof(val) );
			break;
		}

	case KeyValues::TYPE_UINT64:
		{
			uint64 val = pKV->GetUint64();
			CRC32_ProcessBuffer( crc, &val, sizeof(val) );
			break;
		}

	case KeyValues::TYPE_FLOAT:
		{
			float val = pKV->GetFloat();
			CRC32_ProcessBuffer( crc, &val, sizeof(val) );
			break;
		}
	case KeyValues::TYPE_COLOR:
		{
			int val = pKV->GetColor().GetRawColor();
			CRC32_ProcessBuffer( crc, &val, sizeof(val) );
			break;
		}

	default:
	case KeyValues::TYPE_PTR:
	case KeyValues::TYPE_WSTRING:
		{
			Assert( !"Unsupport data type!" );
			break;
		}
	}
}

uint32 CEconItemSchema::CalculateKeyValuesVersion( KeyValues *pKV )
{
	CRC32_t crc;
	CRC32_Init( &crc );

	// Calc CRC recursively.  Ignore the very top-most
	// key name, which isn't set consistently
	CalculateKeyValuesCRCRecursive( pKV, &crc, true );
	CRC32_Final( &crc );
	return crc;
}

EEquipType_t CEconItemSchema::GetEquipTypeFromClassIndex( int iClass ) const
{
	if ( iClass == GetAccountIndex() )
		return EEquipType_t::EQUIP_TYPE_ACCOUNT;

	if ( iClass >= GetFirstValidClass() && iClass <= GetLastValidClass() )
		return EEquipType_t::EQUIP_TYPE_CLASS;

	return EEquipType_t::EQUIP_TYPE_INVALID;
}

//-----------------------------------------------------------------------------
// Purpose:	Initializes the schema
// Input:	pKVRawDefinition - The raw KeyValues representation of the schema
//			pVecErrors - An optional vector that will contain error messages if 
//				the init fails.
// Output:	True if initialization succeeded, false otherwise
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitSchema( KeyValues *pKVRawDefinition, CUtlVector<CUtlString> *pVecErrors /* = NULL */ )
{
	double flInitSchemaTime = Plat_FloatTime();

	m_unMinLevel = pKVRawDefinition->GetInt( "item_level_min", 0 );
	m_unMaxLevel = pKVRawDefinition->GetInt( "item_level_max", 0 );

	m_unVersion = CalculateKeyValuesVersion( pKVRawDefinition );




	// Parse the prefabs block first so the prefabs will be populated in case anything else wants
	// to use them later.
	KeyValues *pKVPrefabs = pKVRawDefinition->FindKey( "prefabs" );
	if ( NULL != pKVPrefabs )
	{
		SCHEMA_INIT_SUBSTEP( BInitDefinitionPrefabs( pKVPrefabs, pVecErrors ) );
	}

	// Initialize the game info block
	KeyValues *pKVGameInfo = pKVRawDefinition->FindKey( "game_info" );
	SCHEMA_INIT_CHECK( NULL != pKVGameInfo, "Required key \"game_info\" missing.\n" );

	if ( NULL != pKVGameInfo )
	{
		SCHEMA_INIT_SUBSTEP( BInitGameInfo( pKVGameInfo, pVecErrors ) );
	}

	// Initialize our attribute types. We don't actually pull this data from the schema right now but it
	// still makes sense to initialize it at this point.
	SCHEMA_INIT_SUBSTEP( BInitAttributeTypes( pVecErrors ) );

	// Initialize the item series block
	KeyValues *pKVItemSeries = pKVRawDefinition->FindKey( "item_series_types" );
	SCHEMA_INIT_CHECK( NULL != pKVItemSeries, "Required key \"item_series_types\" missing.\n" );
	if ( NULL != pKVItemSeries )
	{
		SCHEMA_INIT_SUBSTEP( BInitItemSeries( pKVItemSeries, pVecErrors ) );
	}

	// Initialize the rarity block
	KeyValues *pKVRarities = pKVRawDefinition->FindKey( "rarities" );
	KeyValues *pKVRarityWeights = pKVRawDefinition->FindKey( "rarities_lootlist_weights" );
	SCHEMA_INIT_CHECK( NULL != pKVRarities, "Required key \"rarities\" missing.\n" );
	if ( NULL != pKVRarities )
	{
		SCHEMA_INIT_SUBSTEP( BInitRarities( pKVRarities, pKVRarityWeights, pVecErrors ) );
	}

	// Initialize the qualities block
	KeyValues *pKVQualities = pKVRawDefinition->FindKey( "qualities" );
	SCHEMA_INIT_CHECK( NULL != pKVQualities, "Required key \"qualities\" missing.\n" );

	if ( NULL != pKVQualities )
	{
		SCHEMA_INIT_SUBSTEP( BInitQualities( pKVQualities, pVecErrors ) );
	}

	// Initialize the colors block
	KeyValues *pKVColors = pKVRawDefinition->FindKey( "colors" );
	SCHEMA_INIT_CHECK( NULL != pKVColors, "Required key \"colors\" missing.\n" );

	if ( NULL != pKVColors )
	{
		SCHEMA_INIT_SUBSTEP( BInitColors( pKVColors, pVecErrors ) );
	}

	// Initialize the attributes block
	KeyValues *pKVAttributes = pKVRawDefinition->FindKey( "attributes" );
	SCHEMA_INIT_CHECK( NULL != pKVAttributes, "Required key \"attributes\" missing.\n" );

	if ( NULL != pKVAttributes )
	{
		SCHEMA_INIT_SUBSTEP( BInitAttributes( pKVAttributes, pVecErrors ) );
	}


	// Initialize the "equip_regions_list" block -- this is an optional block
	KeyValues *pKVEquipRegions = pKVRawDefinition->FindKey( "equip_regions_list" );
	if ( NULL != pKVEquipRegions )
	{
		SCHEMA_INIT_SUBSTEP( BInitEquipRegions( pKVEquipRegions, pVecErrors ) );
	}

	// Initialize the "equip_conflicts" block -- this is an optional block, though it doesn't
	// make any sense and will probably fail internally if there is no corresponding "equip_regions"
	// block as well
	KeyValues *pKVEquipRegionConflicts = pKVRawDefinition->FindKey( "equip_conflicts" );
	if ( NULL != pKVEquipRegionConflicts )
	{
		SCHEMA_INIT_SUBSTEP( BInitEquipRegionConflicts( pKVEquipRegionConflicts, pVecErrors ) );
	}

	// Parse the loot lists block (on the GC)
	// Must be BEFORE Item defs
	KeyValues *pKVItemCriteriaTemplates = pKVRawDefinition->FindKey( "item_criteria_templates" );
	SCHEMA_INIT_SUBSTEP( BInitItemCriteriaTemplates( pKVItemCriteriaTemplates, pVecErrors ) );

	KeyValues *pKVRandomAttributeTemplates = pKVRawDefinition->FindKey( "random_attribute_templates" );
	SCHEMA_INIT_SUBSTEP( BInitRandomAttributeTemplates( pKVRandomAttributeTemplates, pVecErrors ) );

	KeyValues *pKVLootlistJobTemplates = pKVRawDefinition->FindKey( "lootlist_job_template_definitions" );
	SCHEMA_INIT_SUBSTEP( BInitLootlistJobTemplates( pKVLootlistJobTemplates, pVecErrors ) );

	// Initialize the items block
	KeyValues *pKVItems = pKVRawDefinition->FindKey( "items" );
	SCHEMA_INIT_CHECK( NULL != pKVItems, "Required key \"items\" missing.\n" );

	if ( NULL != pKVItems )
	{
		SCHEMA_INIT_SUBSTEP( BInitItems( pKVItems, pVecErrors ) );
	}


	// Verify base item names are proper in item schema
	SCHEMA_INIT_SUBSTEP( BVerifyBaseItemNames( pVecErrors ) );

	// Parse the item_sets block.
	KeyValues *pKVItemSets = pKVRawDefinition->FindKey( "item_sets" );
	SCHEMA_INIT_SUBSTEP( BInitItemSets( pKVItemSets, pVecErrors ) );
	
	// Particles
	KeyValues *pKVParticleSystems = pKVRawDefinition->FindKey( "attribute_controlled_attached_particles" );
	SCHEMA_INIT_SUBSTEP( BInitAttributeControlledParticleSystems( pKVParticleSystems, pVecErrors ) );

	// Parse any recipes block
	KeyValues *pKVRecipes = pKVRawDefinition->FindKey( "recipes" );
	SCHEMA_INIT_SUBSTEP( BInitRecipes( pKVRecipes, pVecErrors ) );

	// Reset our loot lists.
	m_dictLootLists.RemoveAll();

	// Init Item Collections - Must be before lootlists since collections are lootlists themselves and are referenced by lootlists
	KeyValues *pKVItemCollections = pKVRawDefinition->FindKey( "item_collections" );
	if ( NULL != pKVItemCollections )
	{
		SCHEMA_INIT_SUBSTEP( BInitItemCollections( pKVItemCollections, pVecErrors ) );
	}


	// Parse the client loot lists block (everywhere)
	KeyValues *pKVClientLootLists = pKVRawDefinition->FindKey( "client_loot_lists" );
	SCHEMA_INIT_SUBSTEP( BInitLootLists( pKVClientLootLists, pVecErrors ) );

	// Parse the revolving loot lists block
	KeyValues *pKVRevolvingLootLists = pKVRawDefinition->FindKey( "revolving_loot_lists" );
	SCHEMA_INIT_SUBSTEP( BInitRevolvingLootLists( pKVRevolvingLootLists, pVecErrors ) );

	// Init Items that may reference Collections
	SCHEMA_INIT_SUBSTEP( BInitCollectionReferences( pVecErrors ) );

	// Validate Operation Pass	
	KeyValues *pKVOperationDefinitions = pKVRawDefinition->FindKey( "operations" );
	if ( NULL != pKVOperationDefinitions )
	{
		SCHEMA_INIT_SUBSTEP( BInitOperationDefinitions( pKVGameInfo, pKVOperationDefinitions, pVecErrors ) );
	}

#if   defined( CLIENT_DLL ) || defined( GAME_DLL )
	KeyValues *pKVArmoryData = pKVRawDefinition->FindKey( "armory_data" );
	SCHEMA_INIT_SUBSTEP( BInitArmoryData( pKVArmoryData, pVecErrors ) );
#endif // GC_DLL

	// Parse any achievement rewards
	KeyValues *pKVAchievementRewards = pKVRawDefinition->FindKey( "achievement_rewards" );
	SCHEMA_INIT_SUBSTEP( BInitAchievementRewards( pKVAchievementRewards, pVecErrors ) );

#ifdef TF_CLIENT_DLL
	// Compute the number of concrete items, for each item, and cache for quick access
	SCHEMA_INIT_SUBSTEP( BInitConcreteItemCounts( pVecErrors ) );

	// We don't have access to Steam's full library of app data on the client so initialize whichever packages
	// we want to reference.
	KeyValues *pKVSteamPackages = pKVRawDefinition->FindKey( "steam_packages" );
	SCHEMA_INIT_SUBSTEP( BInitSteamPackageLocalizationToken( pKVSteamPackages, pVecErrors ) );
#endif // TF_CLIENT_DLL

	// Parse the item levels block
	KeyValues *pKVItemLevels = pKVRawDefinition->FindKey( "item_levels" );
	SCHEMA_INIT_SUBSTEP( BInitItemLevels( pKVItemLevels, pVecErrors ) );

	// Parse the kill eater score types
	KeyValues *pKVKillEaterScoreTypes = pKVRawDefinition->FindKey( "kill_eater_score_types" );
	SCHEMA_INIT_SUBSTEP( BInitKillEaterScoreTypes( pKVKillEaterScoreTypes, pVecErrors ) );

	// Initialize the string tables, if present
	KeyValues *pKVStringTables = pKVRawDefinition->FindKey( "string_lookups" );
	SCHEMA_INIT_SUBSTEP( BInitStringTables( pKVStringTables, pVecErrors ) );

	// Initialize the community Market remaps, if present
	KeyValues *pKVCommunityMarketRemaps = pKVRawDefinition->FindKey( "community_market_item_remaps" );
	SCHEMA_INIT_SUBSTEP( BInitCommunityMarketRemaps( pKVCommunityMarketRemaps, pVecErrors ) );

	double flTotalTime = Plat_FloatTime() - flInitSchemaTime;

#ifdef GAME_DLL
	DevMsg( "*********Server InitSchema time = %f\n", flTotalTime );
#elif CLIENT_DLL
	DevMsg( "*********Client InitSchema time = %f\n", flTotalTime );
#else // GC_DLL
	DevMsg( "*********GC InitSchema time = %f\n", flTotalTime );
#endif

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:	Initializes the "game_info" section of the schema
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitGameInfo( KeyValues *pKVGameInfo, CUtlVector<CUtlString> *pVecErrors )
{
	m_unFirstValidClass = pKVGameInfo->GetInt( "first_valid_class", 0 );
	m_unLastValidClass = pKVGameInfo->GetInt( "last_valid_class", 0 );
	SCHEMA_INIT_CHECK( 0 < m_unFirstValidClass, "First valid class must be greater than 0." );
	SCHEMA_INIT_CHECK( m_unFirstValidClass <= m_unLastValidClass, "First valid class must be less than or equal to last valid class." );
	m_unAccoutClassIndex = pKVGameInfo->GetInt( "account_class_index", 0 );
	SCHEMA_INIT_CHECK( m_unAccoutClassIndex > m_unLastValidClass, "Account class index must be greater than 'last_valid_class'" );

	m_unFirstValidClassItemSlot = pKVGameInfo->GetInt( "first_valid_item_slot", INVALID_EQUIPPED_SLOT );
	m_unLastValidClassItemSlot = pKVGameInfo->GetInt( "last_valid_item_slot", INVALID_EQUIPPED_SLOT );
	SCHEMA_INIT_CHECK( INVALID_EQUIPPED_SLOT != m_unFirstValidClassItemSlot, "first_valid_item_slot not set!" );
	SCHEMA_INIT_CHECK( INVALID_EQUIPPED_SLOT != m_unFirstValidClassItemSlot, "last_valid_item_slot not set!" );
	SCHEMA_INIT_CHECK( m_unFirstValidClassItemSlot <= m_unLastValidClassItemSlot, "First valid item slot must be less than or equal to last valid item slot." );

	m_unFirstValidAccountItemSlot = pKVGameInfo->GetInt( "account_first_valid_item_slot", INVALID_EQUIPPED_SLOT );
	m_unLastValidAccountItemSlot  = pKVGameInfo->GetInt( "account_last_valid_item_slot", INVALID_EQUIPPED_SLOT );
	SCHEMA_INIT_CHECK( INVALID_EQUIPPED_SLOT != m_unFirstValidAccountItemSlot, "account_first_valid_item_slot not set!" );
	SCHEMA_INIT_CHECK( INVALID_EQUIPPED_SLOT != m_unLastValidAccountItemSlot, "account_last_valid_item_slot not set!" );
	SCHEMA_INIT_CHECK( m_unFirstValidAccountItemSlot <= m_unLastValidAccountItemSlot, "First vlid account item slot must be less than or equal to the last valid account item slot." );

	m_unNumItemPresets = pKVGameInfo->GetInt( "num_item_presets", -1 );
	SCHEMA_INIT_CHECK( (uint32)-1 != m_unNumItemPresets, "num_item_presets not set!" );

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitAttributeTypes( CUtlVector<CUtlString> *pVecErrors )
{
	FOR_EACH_VEC( m_vecAttributeTypes, i )
	{
		delete m_vecAttributeTypes[i].m_pAttrType;
	}
	m_vecAttributeTypes.Purge();

	m_vecAttributeTypes.AddToTail( attr_type_t( NULL,										new CSchemaAttributeType_Default ) );
	m_vecAttributeTypes.AddToTail( attr_type_t( "float",									new CSchemaAttributeType_Float ) );
	m_vecAttributeTypes.AddToTail( attr_type_t( "uint64",									new CSchemaAttributeType_UInt64 ) );
	m_vecAttributeTypes.AddToTail( attr_type_t( "string",									new CSchemaAttributeType_String ) );
	m_vecAttributeTypes.AddToTail( attr_type_t( "dynamic_recipe_component_defined_item",	new CSchemaAttributeType_DynamicRecipeComponentDefinedItem ) );
	m_vecAttributeTypes.AddToTail( attr_type_t( "item_slot_criteria",						new CSchemaAttributeType_ItemSlotCriteria ) );
	m_vecAttributeTypes.AddToTail( attr_type_t( "item_placement",							new CSchemaAttributeType_WorldItemPlacement ) );

	// Make sure that all attribute types specified have the item ID in the 0th column. We use this
	// when loading items to map between item IDs and the attributes they own.
	FOR_EACH_VEC( m_vecAttributeTypes, i )
	{
	}

	return SCHEMA_INIT_SUCCESS();
}


//-----------------------------------------------------------------------------
// Purpose:	Initializes the "prefabs" section of the schema
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitDefinitionPrefabs( KeyValues *pKVPrefabs, CUtlVector<CUtlString> *pVecErrors )
{
	FOR_EACH_TRUE_SUBKEY( pKVPrefabs, pKVPrefab )
	{
		const char *pszPrefabName = pKVPrefab->GetName();

		int nMapIndex = m_dictDefinitionPrefabs.Find( pszPrefabName );

		// Make sure the item index is correct because we use this index as a reference
		SCHEMA_INIT_CHECK( 
			!m_dictDefinitionPrefabs.IsValidIndex( nMapIndex ),
			"Duplicate prefab name (%s)", pszPrefabName );

		m_dictDefinitionPrefabs.Insert( pszPrefabName, pKVPrefab->MakeCopy() );
	}

	return SCHEMA_INIT_SUCCESS();
}


//-----------------------------------------------------------------------------
// Purpose:	Initializes the Item Series section of the schema
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitItemSeries( KeyValues *pKVSeries, CUtlVector<CUtlString> *pVecErrors )
{
	// initialize the item definitions
	if ( NULL != pKVSeries)
	{
		FOR_EACH_TRUE_SUBKEY( pKVSeries, pKVItem )
		{
			int nSeriesIndex = pKVItem->GetInt( "value" );
			int nMapIndex = m_mapItemSeries.Find( nSeriesIndex );

			// Make sure the item index is correct because we use this index as a reference
			SCHEMA_INIT_CHECK(
				!m_mapItemSeries.IsValidIndex( nMapIndex ),
				"Duplicate item series value (%d)", nSeriesIndex );

			nMapIndex = m_mapItemSeries.Insert( nMapIndex );
			SCHEMA_INIT_SUBSTEP( m_mapItemSeries[nMapIndex].BInitFromKV( pKVItem, pVecErrors ) );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:	Initializes the rarity section of the schema
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitRarities( KeyValues *pKVRarities, KeyValues *pKVRarityWeights, CUtlVector<CUtlString> *pVecErrors )
{
	// initialize the item definitions
	if ( NULL != pKVRarities )
	{
		FOR_EACH_TRUE_SUBKEY( pKVRarities, pKVRarity )
		{
			int nRarityIndex = pKVRarity->GetInt( "value" );
			int nMapIndex = m_mapRarities.Find( nRarityIndex );

			// Make sure the item index is correct because we use this index as a reference
			SCHEMA_INIT_CHECK(
				!m_mapRarities.IsValidIndex( nMapIndex ),
				"Duplicate rarity value (%d)", nRarityIndex );

			nMapIndex = m_mapRarities.Insert( nRarityIndex );
			SCHEMA_INIT_SUBSTEP( m_mapRarities[nMapIndex].BInitFromKV( pKVRarity, pKVRarityWeights, *this, pVecErrors ) );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:	Initializes the qualities section of the schema
// Input:	pKVQualities - The qualities section of the KeyValues 
//				representation of the schema
//			pVecErrors - An optional vector that will contain error messages if 
//				the init fails.
// Output:	True if initialization succeeded, false otherwise
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitQualities( KeyValues *pKVQualities, CUtlVector<CUtlString> *pVecErrors )
{
	// initialize the item definitions
	if ( NULL != pKVQualities )
	{
		FOR_EACH_TRUE_SUBKEY( pKVQualities, pKVQuality )
		{
			int nQualityIndex = pKVQuality->GetInt( "value" );
			int nMapIndex = m_mapQualities.Find( nQualityIndex );

			// Make sure the item index is correct because we use this index as a reference
			SCHEMA_INIT_CHECK( 
				!m_mapQualities.IsValidIndex( nMapIndex ),
				"Duplicate quality value (%d)", nQualityIndex );

			nMapIndex = m_mapQualities.Insert( nQualityIndex );
			SCHEMA_INIT_SUBSTEP( m_mapQualities[nMapIndex].BInitFromKV( pKVQuality, pVecErrors ) );
		}
	}

	// Check the integrity of the quality definitions

	// Check for duplicate quality names
	CUtlRBTree<const char *> rbQualityNames( CaselessStringLessThan );
	rbQualityNames.EnsureCapacity( m_mapQualities.Count() );
	FOR_EACH_MAP_FAST( m_mapQualities, i )
	{
		int iIndex = rbQualityNames.Find( m_mapQualities[i].GetName() );
		SCHEMA_INIT_CHECK( 
			!rbQualityNames.IsValidIndex( iIndex ),
			"Quality definition %d: Duplicate quality name %s", m_mapQualities[i].GetDBValue(), m_mapQualities[i].GetName() );

		if( !rbQualityNames.IsValidIndex( iIndex ) )
			rbQualityNames.Insert( m_mapQualities[i].GetName() );
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitColors( KeyValues *pKVColors, CUtlVector<CUtlString> *pVecErrors )
{
	// initialize the color definitions
	if ( NULL != pKVColors )
	{
		FOR_EACH_TRUE_SUBKEY( pKVColors, pKVColor )
		{
			CEconColorDefinition *pNewColorDef = new CEconColorDefinition;

			SCHEMA_INIT_SUBSTEP( pNewColorDef->BInitFromKV( pKVColor, pVecErrors ) );
			m_vecColorDefs.AddToTail( pNewColorDef );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CEconItemSchema::GetEquipRegionIndexByName( const char *pRegionName ) const
{
	FOR_EACH_VEC( m_vecEquipRegionsList, i )
	{
		const char *szEntryRegionName = m_vecEquipRegionsList[i].m_sName.Get();
		if ( !V_stricmp( szEntryRegionName, pRegionName ) )
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
equip_region_mask_t CEconItemSchema::GetEquipRegionBitMaskByName( const char *pRegionName ) const
{
	int iRegionIndex = GetEquipRegionIndexByName( pRegionName );
	if ( !m_vecEquipRegionsList.IsValidIndex( iRegionIndex ) )
		return 0;

	equip_region_mask_t unRegionMask = 1 << m_vecEquipRegionsList[iRegionIndex].m_unBitIndex;
	Assert( unRegionMask > 0 );

	return unRegionMask;
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
void CEconItemSchema::SetEquipRegionConflict( int iRegion, unsigned int unBit )
{
	Assert( m_vecEquipRegionsList.IsValidIndex( iRegion ) );

	equip_region_mask_t unRegionMask = 1 << unBit;
	Assert( unRegionMask > 0 );

	m_vecEquipRegionsList[iRegion].m_unMask |= unRegionMask;
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
equip_region_mask_t CEconItemSchema::GetEquipRegionMaskByName( const char *pRegionName ) const
{
	int iRegionIdx = GetEquipRegionIndexByName( pRegionName );
	if ( iRegionIdx < 0 )
		return 0;

	return m_vecEquipRegionsList[iRegionIdx].m_unMask;
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
void CEconItemSchema::AssignDefaultBodygroupState( const char *pszBodygroupName, int iValue )
{
	// Flip the value passed in -- if we specify in the schema that a region should be off, we assume that it's
	// on by default.
	// actually the schemas are all authored assuming that the default is 0, so let's use that
	int iDefaultValue = 0; //iValue == 0 ? 1 : 0;

	// Make sure that we're constantly reinitializing our default value to the same default value. This is sort
	// of dumb but it works for everything we've got now. In the event that conflicts start cropping up it would
	// be easy enough to make a new schema section.
	int iIndex = m_dictDefaultBodygroupState.Find( pszBodygroupName );
	if ( (m_dictDefaultBodygroupState.IsValidIndex( iIndex ) && m_dictDefaultBodygroupState[iIndex] != iDefaultValue) ||
		 (iValue < 0 || iValue > 1) )
	{
		EmitWarning( SPEW_GC, 4, "Unable to get accurate read on whether bodygroup '%s' is enabled or disabled by default. (The schema is fine, but the code is confused and could stand to be made smarter.)\n", pszBodygroupName );
	}

	if ( !m_dictDefaultBodygroupState.IsValidIndex( iIndex ) )
	{
		m_dictDefaultBodygroupState.Insert( pszBodygroupName, iDefaultValue );
	}
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitEquipRegions( KeyValues *pKVEquipRegions, CUtlVector<CUtlString> *pVecErrors )
{
	CUtlVector<const char *> vecNames;

	FOR_EACH_SUBKEY( pKVEquipRegions, pKVRegion )
	{
		const char *pRegionKeyName = pKVRegion->GetName();

		vecNames.Purge();

		// The "shared" name is special for equip regions -- it means that all of the sub-regions specified
		// will use the same bit to store equipped-or-not data, but that one bit can be accessed by a whole
		// bunch of different names. This is useful in TF where different classes have different regions, but
		// those regions cannot possibly conflict with each other. For example, "scout_backpack" cannot possibly
		// overlap with "pyro_shoulder" because they can't even be equipped on the same character.
		if ( pRegionKeyName && !Q_stricmp( pRegionKeyName, "shared" ) )
		{
			FOR_EACH_SUBKEY( pKVRegion, pKVSharedRegionName )
			{
				vecNames.AddToTail( pKVSharedRegionName->GetName() );
			}
		}
		// We have a standard name -- this one entry is its own equip region.
		else
		{
			vecNames.AddToTail( pRegionKeyName );
		}

		// What bit will this equip region use to mask against conflicts? If we don't have any equip regions
		// at all, we'll use the base bit, otherwise we just grab one higher than whatever we used last.
		unsigned int unNewBitIndex = m_vecEquipRegionsList.Count() <= 0 ? 0 : m_vecEquipRegionsList.Tail().m_unBitIndex + 1;
		
		FOR_EACH_VEC( vecNames, i )
		{
			const char *pRegionName = vecNames[i];

			// Make sure this name is unique.
			if ( GetEquipRegionIndexByName( pRegionName ) >= 0 )
			{
				pVecErrors->AddToTail( CFmtStr( "Duplicate equip region named \"%s\".", pRegionName ).Access() );
				continue;
			}

			// Make a new region.
			EquipRegion newEquipRegion;
			newEquipRegion.m_sName		= pRegionName;
			newEquipRegion.m_unMask		= 0;				// we'll update this mask later
			newEquipRegion.m_unBitIndex	= unNewBitIndex;

			int iIdx = m_vecEquipRegionsList.AddToTail( newEquipRegion );

			// Tag this region to conflict with itself so that if nothing else two items in the same
			// region can't equip over each other.
			SetEquipRegionConflict( iIdx, unNewBitIndex );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitEquipRegionConflicts( KeyValues *pKVEquipRegionConflicts, CUtlVector<CUtlString> *pVecErrors )
{
	FOR_EACH_TRUE_SUBKEY( pKVEquipRegionConflicts, pKVConflict )
	{
		// What region is the base of this conflict?
		const char *pRegionName = pKVConflict->GetName();
		int iRegionIdx = GetEquipRegionIndexByName( pRegionName );
		if ( iRegionIdx < 0 )
		{
			pVecErrors->AddToTail( CFmtStr( "Unable to find base equip region named \"%s\" for conflicts.", pRegionName ).Access() );
			continue;
		}

		FOR_EACH_SUBKEY( pKVConflict, pKVConflictOther )
		{
			const char *pOtherRegionName = pKVConflictOther->GetName();
			int iOtherRegionIdx = GetEquipRegionIndexByName( pOtherRegionName );
			if ( iOtherRegionIdx < 0 )
			{
				pVecErrors->AddToTail( CFmtStr( "Unable to find other equip region named \"%s\" for conflicts.", pOtherRegionName ).Access() );
				continue;
			}

			SetEquipRegionConflict( iRegionIdx,		 m_vecEquipRegionsList[iOtherRegionIdx].m_unBitIndex );
			SetEquipRegionConflict( iOtherRegionIdx, m_vecEquipRegionsList[iRegionIdx].m_unBitIndex );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:	Initializes the attributes section of the schema
// Input:	pKVAttributes - The attributes section of the KeyValues 
//				representation of the schema
//			pVecErrors - An optional vector that will contain error messages if 
//				the init fails.
// Output:	True if initialization succeeded, false otherwise
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitAttributes( KeyValues *pKVAttributes, CUtlVector<CUtlString> *pVecErrors )
{
	// Initialize the attribute definitions
	FOR_EACH_TRUE_SUBKEY( pKVAttributes, pKVAttribute )
	{
		int nAttrIndex = Q_atoi( pKVAttribute->GetName() );
		int nMapIndex = m_mapAttributes.Find( nAttrIndex );

		// Make sure the index is positive
		SCHEMA_INIT_CHECK( 
			nAttrIndex >= 0,
			"Attribute definition index %d must be greater than or equal to zero", nAttrIndex );

		// Make sure the attribute index is not repeated
		SCHEMA_INIT_CHECK( 
			!m_mapAttributes.IsValidIndex( nMapIndex ),
			"Duplicate attribute definition index (%d)", nAttrIndex );

		nMapIndex = m_mapAttributes.Insert( nAttrIndex );

		SCHEMA_INIT_SUBSTEP( m_mapAttributes[nMapIndex].BInitFromKV( pKVAttribute, pVecErrors ) );
	}

	// Check the integrity of the attribute definitions

	// Check for duplicate attribute definition names
	CUtlRBTree<const char *> rbAttributeNames( CaselessStringLessThan );
	rbAttributeNames.EnsureCapacity( m_mapAttributes.Count() );
	FOR_EACH_MAP_FAST( m_mapAttributes, i )
	{
		int iIndex = rbAttributeNames.Find( m_mapAttributes[i].GetDefinitionName() );
		SCHEMA_INIT_CHECK( 
			!rbAttributeNames.IsValidIndex( iIndex ),
			"Attribute definition %d: Duplicate name \"%s\"", m_mapAttributes.Key( i ), m_mapAttributes[i].GetDefinitionName() );
		if( !rbAttributeNames.IsValidIndex( iIndex ) )
			rbAttributeNames.Insert( m_mapAttributes[i].GetDefinitionName() );
	}

	return SCHEMA_INIT_SUCCESS();
}


//-----------------------------------------------------------------------------
// Purpose:	Initializes the items section of the schema
// Input:	pKVItems - The items section of the KeyValues 
//				representation of the schema
//			pVecErrors - An optional vector that will contain error messages if 
//				the init fails.
// Output:	True if initialization succeeded, false otherwise
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitItems( KeyValues *pKVItems, CUtlVector<CUtlString> *pVecErrors )
{
	m_mapItems.PurgeAndDeleteElements();
	m_mapItemsSorted.Purge();
	m_mapToolsItems.Purge();
	m_mapPaintKitTools.Purge();
	m_mapBaseItems.Purge();
	m_vecBundles.Purge();
	m_mapQuestObjectives.PurgeAndDeleteElements();

#if defined(CLIENT_DLL) || defined(GAME_DLL)
	if ( m_pDefaultItemDefinition )
	{
		delete m_pDefaultItemDefinition;
		m_pDefaultItemDefinition = NULL;
	}
#endif

	// initialize the item definitions
	if ( NULL != pKVItems )
	{
		FOR_EACH_TRUE_SUBKEY( pKVItems, pKVItem )
		{
			if ( Q_stricmp( pKVItem->GetName(), "default" ) == 0 )
			{
#if defined(CLIENT_DLL) || defined(GAME_DLL)
				SCHEMA_INIT_CHECK(
					m_pDefaultItemDefinition == NULL,
					"Duplicate 'default' item definition." );

				m_pDefaultItemDefinition = CreateEconItemDefinition();
				SCHEMA_INIT_SUBSTEP( m_pDefaultItemDefinition->BInitFromKV( pKVItem, pVecErrors ) );
#endif
			}
			else
			{
				int nItemIndex = Q_atoi( pKVItem->GetName() );
				int nMapIndex = m_mapItems.Find( nItemIndex );

				// Make sure the item index is correct because we use this index as a reference
				SCHEMA_INIT_CHECK( 
					!m_mapItems.IsValidIndex( nMapIndex ),
					"Duplicate item definition (%d)", nItemIndex );

				// Check to make sure the index is positive
				SCHEMA_INIT_CHECK( 
					nItemIndex >= 0,
					"Item definition index %d must be greater than or equal to zero", nItemIndex );

				CEconItemDefinition *pItemDef = CreateEconItemDefinition();
				nMapIndex = m_mapItems.Insert( nItemIndex, pItemDef );
				m_mapItemsSorted.Insert( nItemIndex, pItemDef );
				SCHEMA_INIT_SUBSTEP( m_mapItems[nMapIndex]->BInitFromKV( pKVItem, pVecErrors ) );

				// Cache off Tools references
				if ( pItemDef->IsTool() )
				{
					m_mapToolsItems.Insert( nItemIndex, pItemDef );

					// found paintkit tool, add to paintkit map
					if ( pItemDef->GetEconTool() && !V_strcmp( pItemDef->GetEconTool()->GetTypeName(), "paintkit" ) )
					{
						uint32 unPaintKitDefIndex;
						SCHEMA_INIT_CHECK( GetPaintKitDefIndex( pItemDef, &unPaintKitDefIndex ), "PaintKit Item [%d] is missing paintkit def index attr", pItemDef->GetDefinitionIndex() );
						int iMapIndex = m_mapPaintKitTools.Find( unPaintKitDefIndex );
						SCHEMA_INIT_CHECK( iMapIndex == m_mapPaintKitTools.InvalidIndex(), "Duplicate paintkit def index [%d]. Trying to add to item [%d], but item [%d] already has it.",
											unPaintKitDefIndex, pItemDef->GetDefinitionIndex(), m_mapPaintKitTools[ iMapIndex ]->GetDefinitionIndex() );

						m_mapPaintKitTools.Insert( unPaintKitDefIndex, pItemDef );
					}
				}

				if ( pItemDef->IsBaseItem() )
				{
					m_mapBaseItems.Insert( nItemIndex, pItemDef );
				}

				// Cache off bundles for the link phase below.
				if ( pItemDef->IsBundle() )
				{
					// Cache off the item def for the bundle, since we'll need both the bundle info and the item def index later.
					m_vecBundles.AddToTail( pItemDef );

					// If the bundle is a pack bundle, mark all the contained items as pack items / link to the owning pack bundle
					if ( pItemDef->IsPackBundle() )
					{
						const bundleinfo_t *pBundleInfo = pItemDef->GetBundleInfo();
						FOR_EACH_VEC( pBundleInfo->vecItemDefs, iCurItem )
						{
							CEconItemDefinition *pCurItemDef = pBundleInfo->vecItemDefs[ iCurItem ];
							SCHEMA_INIT_CHECK( NULL == pCurItemDef->m_pOwningPackBundle, "Pack item \"%s\" included in more than one pack bundle - not allowed!", pCurItemDef->GetDefinitionName() );
							pCurItemDef->m_pOwningPackBundle = pItemDef;
						}
					}
				}	
			}
		}
	}

	// Check the integrity of the item definitions
	CUtlRBTree<const char *> rbItemNames( CaselessStringLessThan );
	rbItemNames.EnsureCapacity( m_mapItems.Count() );
	FOR_EACH_MAP_FAST( m_mapItems, i )
	{
		CEconItemDefinition *pItemDef = m_mapItems[ i ];

		// Check for duplicate item definition names
		int iIndex = rbItemNames.Find( pItemDef->GetDefinitionName() );
		SCHEMA_INIT_CHECK( 
			!rbItemNames.IsValidIndex( iIndex ),
			"Item definition %s: Duplicate name on index %d", pItemDef->GetDefinitionName(), m_mapItems.Key( i ) );
		if( !rbItemNames.IsValidIndex( iIndex ) )
			rbItemNames.Insert( m_mapItems[i]->GetDefinitionName() );

		// Link up armory and store mappings for the item
		SCHEMA_INIT_SUBSTEP( pItemDef->BInitItemMappings( pVecErrors ) );
	}

#ifdef DOTA
	// Go through all regular (ie non-pack) bundles and ensure that if any pack items are included, *all* pack items in the owning pack bundle are included
	FOR_EACH_VEC( m_vecBundles, iBundle )
	{
		const CEconItemDefinition *pBundleItemDef = m_vecBundles[ iBundle ];
		if ( pBundleItemDef->IsPackBundle() )
			continue;

		// Go through all items in the bundle and look for pack items
		const bundleinfo_t *pBundle = pBundleItemDef->GetBundleInfo();
		if ( pBundle )
		{
			FOR_EACH_VEC( pBundle->vecItemDefs, iContainedBundleItem )
			{
				// Get the associated pack bundle
				const CEconItemDefinition *pContainedBundleItemDef = pBundle->vecItemDefs[ iContainedBundleItem ];

				// Ignore non-pack items
				if ( !pContainedBundleItemDef || !pContainedBundleItemDef->IsPackItem() )
					continue;

				// Get the pack bundle that contains this particular pack item
				const CEconItemDefinition *pOwningPackBundleItemDef = pContainedBundleItemDef->GetOwningPackBundle();

				// Make sure all items in the owning pack bundle are in pBundleItemDef's bundle info (pBundle)
				const bundleinfo_t *pOwningPackBundle = pOwningPackBundleItemDef->GetBundleInfo();
				FOR_EACH_VEC( pOwningPackBundle->vecItemDefs, iCurPackBundleItem )
				{
					CEconItemDefinition *pCurPackBundleItem = pOwningPackBundle->vecItemDefs[ iCurPackBundleItem ];
					if ( !pBundle->vecItemDefs.HasElement( pCurPackBundleItem ) )
					{
						SCHEMA_INIT_CHECK(
							false,
							"The bundle \"%s\" contains some, but not all pack items required specified by pack bundle \"%s.\"",
							pBundleItemDef->GetDefinitionName(),
							pOwningPackBundleItemDef->GetDefinitionName()
					   );
					}
				}
			}
		}
	}
#endif

	return SCHEMA_INIT_SUCCESS();
}

#if 0 // Compiled out until some DotA changes from the item editor are brought over
//-----------------------------------------------------------------------------
// Purpose:	Delete an item definition. Moderately dangerous as cached references will become bad.
// Intended for use by the item editor.
//-----------------------------------------------------------------------------
bool CEconItemSchema::DeleteItemDefinition( int iDefIndex )
{
	m_mapItemsSorted.Remove( iDefIndex );

	int nMapIndex = m_mapItems.Find( iDefIndex );
	if ( m_mapItems.IsValidIndex( nMapIndex ) )
	{
		CEconItemDefinition* pItemDef = m_mapItems[nMapIndex];
		if ( pItemDef )
		{
			m_mapItems.RemoveAt( nMapIndex );
			delete pItemDef;
			return true;
		}
	}
	return false;
}
#endif

//-----------------------------------------------------------------------------
// Purpose:	Parses the Item Sets section.
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitItemSets( KeyValues *pKVItemSets, CUtlVector<CUtlString> *pVecErrors )
{
	m_dictItemSets.RemoveAll();

	if ( NULL != pKVItemSets )
	{
		FOR_EACH_TRUE_SUBKEY( pKVItemSets, pKVItemSet )
		{
			const char* setName = pKVItemSet->GetName();

			SCHEMA_INIT_CHECK( setName != NULL, "All itemsets must have names." );
			SCHEMA_INIT_CHECK( m_dictItemSets.Find( setName ) == m_dictItemSets.InvalidIndex(), "Duplicate itemset name (%s) found!", setName );

			int idx = m_dictItemSets.Insert( setName, new CEconItemSetDefinition );
			SCHEMA_INIT_SUBSTEP( m_dictItemSets[idx]->BInitFromKV( pKVItemSet, pVecErrors ) );
		}

		// Once we've initialized all of our item sets, loop through all of our item definitions looking
		// for pseudo set items. For example, the Festive Holy Mackerel is a different item definition from
		// the regular Holy Mackerel, but for set completion and set listing purposes, we want it to show
		// as part of the base set.
		FOR_EACH_MAP_FAST( m_mapItems, i )
		{
			CEconItemDefinition *pItemDef = m_mapItems[i];
			Assert( pItemDef );

			// Items that point to themselves are the base set items and got initialized as part of the
			// set initialization above.
			if ( pItemDef->GetSetItemRemap() == pItemDef->GetDefinitionIndex() )
				continue;

			// Which item are we stealing set information from?
			const CEconItemDefinition *pRemappedSetItemDef = GetItemDefinition( pItemDef->GetSetItemRemap() );
			AssertMsg( pRemappedSetItemDef, "Somehow got through item and set initialization but have a broken set remap item!" );

			pItemDef->SetItemSetDefinition( pRemappedSetItemDef->GetItemSetDefinition() );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
bool CEconItemSchema::BVerifyBaseItemNames( CUtlVector<CUtlString> *pVecErrors )
{
	FOR_EACH_MAP_FAST( m_mapItems, i )
	{
		CEconItemDefinition *pItemDef = m_mapItems[i];

		// get base item name
		const char* pBaseName = pItemDef->GetBaseFunctionalItemName();

		if ( !pBaseName || pBaseName[0] == '\0' )
		{
			continue;
		}
		
		// look up base item name
		SCHEMA_INIT_CHECK( GetItemDefinitionByName( pBaseName ) != NULL, "Base item name not found %s.", pBaseName );
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitItemCollections( KeyValues *pKVItemCollections, CUtlVector<CUtlString> *pVecErrors )
{
	m_dictItemCollections.Purge();

	if ( NULL != pKVItemCollections )
	{
		FOR_EACH_TRUE_SUBKEY( pKVItemCollections, pKVItemCollection )
		{
			const char* setName = pKVItemCollection->GetName();

			SCHEMA_INIT_CHECK( setName != NULL, "All item collections must have names." );
			SCHEMA_INIT_CHECK( m_dictItemCollections.Find( setName ) == m_dictItemCollections.InvalidIndex(), "Duplicate item collection name (%s) found!", setName );

			int idx = m_dictItemCollections.Insert( setName, new CEconItemCollectionDefinition );
			SCHEMA_INIT_SUBSTEP( m_dictItemCollections[idx]->BInitFromKV( pKVItemCollection, pVecErrors ) );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitCollectionReferences( CUtlVector<CUtlString> *pVecErrors )
{
	FOR_EACH_MAP_FAST( m_mapItems, i )
	{
		CEconItemDefinition *pItemDef = m_mapItems[i];
		const char *pszCollectionName = pItemDef->GetCollectionReference();
		if ( pszCollectionName )
		{
			// Find the collection
			bool bFound = false;
			for ( int iCollectionIndex = m_dictItemCollections.First(); m_dictItemCollections.IsValidIndex( iCollectionIndex ); iCollectionIndex = m_dictItemCollections.Next( iCollectionIndex ) )
			{
				const char * pszTemp = m_dictItemCollections[iCollectionIndex]->m_strName;

				if ( !V_strcmp( pszTemp, pszCollectionName) )
				{
					bFound = true;
					pItemDef->SetItemCollectionDefinition( m_dictItemCollections[iCollectionIndex] );
					break;
				}
			}
			SCHEMA_INIT_CHECK( bFound == true, "Collection %s referenced by item %s not found", pszCollectionName, pItemDef->GetDefinitionName() );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}
//-----------------------------------------------------------------------------
const CEconItemCollectionDefinition *CEconItemSchema::GetCollectionByName( const char* pCollectionName )
{
	if ( !pCollectionName )
		return NULL;

	for ( int iCollectionIndex = m_dictItemCollections.First(); m_dictItemCollections.IsValidIndex( iCollectionIndex ); iCollectionIndex = m_dictItemCollections.Next( iCollectionIndex ) )
	{
		const char * pszTemp = m_dictItemCollections[iCollectionIndex]->m_strName;
		if ( !V_strcmp( pszTemp, pCollectionName ) )
		{
			return m_dictItemCollections[iCollectionIndex];
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitOperationDefinitions( KeyValues *pKVGameInfo, KeyValues *pKVOperationDefinitions, CUtlVector<CUtlString> *pVecErrors )
{
	m_dictOperationDefinitions.Purge();

	if ( NULL != pKVOperationDefinitions )
	{
		FOR_EACH_TRUE_SUBKEY( pKVOperationDefinitions, pKVOperation )
		{
			const char* setName = pKVOperation->GetName();
			SCHEMA_INIT_CHECK( setName != NULL, "All operations must have names." );
			SCHEMA_INIT_CHECK( m_dictOperationDefinitions.Find( setName ) == m_dictOperationDefinitions.InvalidIndex(), "Duplicate operation definition name (%s) found!", setName );

			CEconOperationDefinition *pNewOperation = new CEconOperationDefinition();
			SCHEMA_INIT_SUBSTEP( pNewOperation->BInitFromKV( pKVOperation, pVecErrors ) );

			m_dictOperationDefinitions.Insert( setName, pNewOperation );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}


//-----------------------------------------------------------------------------
// Purpose:	Initializes the timed rewards section of the schema
// Input:	pKVTimedRewards - The timed_rewards section of the KeyValues 
//				representation of the schema
//			pVecErrors - An optional vector that will contain error messages if 
//				the init fails.
// Output:	True if initialization succeeded, false otherwise
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitTimedRewards( KeyValues *pKVTimedRewards, CUtlVector<CUtlString> *pVecErrors )
{
	m_vecTimedRewards.RemoveAll();

	// initialize the rewards sections
	if ( NULL != pKVTimedRewards )
	{
		FOR_EACH_TRUE_SUBKEY( pKVTimedRewards, pKVTimedReward )
		{
			int index = m_vecTimedRewards.AddToTail();
			SCHEMA_INIT_SUBSTEP( m_vecTimedRewards[index].BInitFromKV( pKVTimedReward, pVecErrors ) );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
const CTimedItemRewardDefinition* CEconItemSchema::GetTimedReward( eTimedRewardType type ) const
{
	if ( (int)type < m_vecTimedRewards.Count() )
	{
		return &m_vecTimedRewards[type];
	}
	return NULL;
}



//-----------------------------------------------------------------------------
// Purpose:	Initializes the loot lists section of the schema
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitLootLists( KeyValues *pKVLootLists, CUtlVector<CUtlString> *pVecErrors )
{
	if ( NULL != pKVLootLists )
	{
 		FOR_EACH_TRUE_SUBKEY( pKVLootLists, pKVLootList )
		{
			const char* pListName = pKVLootList->GetName();
			SCHEMA_INIT_SUBSTEP( BInsertLootlist( pListName, pKVLootList, pVecErrors ) );
		}
	}

	for ( int idx = m_dictLootLists.First(); m_dictLootLists.IsValidIndex( idx ); idx = m_dictLootLists.Next( idx ) )
	{
		const CEconLootListDefinition *pLootList = m_dictLootLists[idx];
		BVerifyLootListItemDropDates( pLootList, pVecErrors );
	}

	return SCHEMA_INIT_SUCCESS();
}
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInsertLootlist( const char *pListName, KeyValues *pKVLootList, CUtlVector<CUtlString> *pVecErrors )
{
	SCHEMA_INIT_CHECK( pListName != NULL, "All lootlists must have names." );

	if ( m_dictLootLists.Count() > 0 )
	{
		SCHEMA_INIT_CHECK( GetLootListByName( pListName ) == NULL, "Duplicate lootlist name (%s) found!", pListName );
	}

	CEconLootListDefinition *pLootList = new CEconLootListDefinition;
	SCHEMA_INIT_SUBSTEP( pLootList->BInitFromKV( pKVLootList, *this, pVecErrors ) );
	m_dictLootLists.Insert( pListName, pLootList );

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:	Initializes the revolving loot lists section of the schema
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitRevolvingLootLists( KeyValues *pKVLootLists, CUtlVector<CUtlString> *pVecErrors )
{
	m_mapRevolvingLootLists.RemoveAll();

	if ( NULL != pKVLootLists )
	{
		FOR_EACH_SUBKEY( pKVLootLists, pKVList )
		{
			int iListIdx = pKVList->GetInt();
			const char* strListName = pKVList->GetName();
			m_mapRevolvingLootLists.Insert( iListIdx, strListName );
		}
	}

 	FOR_EACH_MAP_FAST( m_mapRevolvingLootLists, i )
	{
		const CEconLootListDefinition* pLootList = GetLootListByName(m_mapRevolvingLootLists[i]);
		BVerifyLootListItemDropDates( pLootList, pVecErrors );
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:	Verify that the contents of visible lootlist do not have drop dates
//			associated with them.  The thinking being that we dont want to have
//			items listed that could potentially not drop, or items disappear/appear
//			in from a list.
//-----------------------------------------------------------------------------
bool CEconItemSchema::BVerifyLootListItemDropDates( const CEconLootListDefinition* pLootList, CUtlVector<CUtlString> *pVecErrors ) const
{
	if ( pLootList && pLootList->BPublicListContents() )
	{
		BRecurseiveVerifyLootListItemDropDates( pLootList, pLootList, pVecErrors );
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:	Recursively dig through all entries in the passed in lootlist to see
//			if any of the containted items have drop dates.
//-----------------------------------------------------------------------------
bool CEconItemSchema::BRecurseiveVerifyLootListItemDropDates( const CEconLootListDefinition* pLootList, const CEconLootListDefinition* pRootLootList, CUtlVector<CUtlString> *pVecErrors ) const
{
	FOR_EACH_VEC( pLootList->GetLootListContents(), j )
	{
		const drop_item_t& item = pLootList->GetLootListContents()[j];
		// 0 and greater means item.  Less than 0 means nested lootlist
		if( item.m_iItemOrLootlistDef >= 0 )
		{
			const CEconItemDefinition* pItemDef = GetItemSchema()->GetItemDefinition( item.m_iItemOrLootlistDef );
			if( pItemDef )
			{
				static CSchemaAttributeDefHandle pAttribDef_StartDropDate( "start drop date" );
				static CSchemaAttributeDefHandle pAttribDef_EndDropDate( "end drop date" );

				CAttribute_String value;

				// Check for start drop date attribute on this item
				SCHEMA_INIT_CHECK( !FindAttribute( pItemDef, pAttribDef_StartDropDate, &value ),
					"Lootlist \"%s\" contains lootlist \"%s\", which contains item \"%s\", which has start drop date.", pRootLootList->GetName(), pLootList->GetName(), pItemDef->GetDefinitionName() );
				// Check for end drop date attribute on this item
				SCHEMA_INIT_CHECK( !FindAttribute( pItemDef, pAttribDef_EndDropDate, &value ),
					"Lootlist \"%s\" contains lootlist \"%s\", which contains item \"%s\", which has end drop date.", pRootLootList->GetName(), pLootList->GetName(), pItemDef->GetDefinitionName() );
			}
		}
		else
		{
			// Get the nested lootlist
			int iLLIndex = (item.m_iItemOrLootlistDef * -1) - 1;
			const CEconLootListDefinition *pNestedLootList = GetItemSchema()->GetLootListByIndex( iLLIndex );
			if ( !pNestedLootList )
				return SCHEMA_INIT_SUCCESS();

			// Dig through all of this lootlist's entries
			BRecurseiveVerifyLootListItemDropDates( pNestedLootList, pRootLootList, pVecErrors );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}


//-----------------------------------------------------------------------------
// Purpose:	Initializes the recipes section of the schema
// Input:	pKVRecipes - The recipes section of the KeyValues 
//				representation of the schema
//			pVecErrors - An optional vector that will contain error messages if 
//				the init fails.
// Output:	True if initialization succeeded, false otherwise
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitRecipes( KeyValues *pKVRecipes, CUtlVector<CUtlString> *pVecErrors )
{
	m_mapRecipes.RemoveAll();

	// initialize the rewards sections
	if ( NULL != pKVRecipes )
	{
		FOR_EACH_TRUE_SUBKEY( pKVRecipes, pKVRecipe )
		{
			int nRecipeIndex = Q_atoi( pKVRecipe->GetName() );
			int nMapIndex = m_mapRecipes.Find( nRecipeIndex );

			// Make sure the recipe index is correct because we use this index as a reference
			SCHEMA_INIT_CHECK( 
				!m_mapRecipes.IsValidIndex( nMapIndex ),
				"Duplicate recipe definition (%d)", nRecipeIndex );

			// Check to make sure the index is positive
			SCHEMA_INIT_CHECK( 
				nRecipeIndex >= 0,
				"Recipe definition index %d must be greater than or equal to zero", nRecipeIndex );

			CEconCraftingRecipeDefinition *recipeDef = CreateCraftingRecipeDefinition();
			SCHEMA_INIT_SUBSTEP( recipeDef->BInitFromKV( pKVRecipe, pVecErrors ) );

#ifdef _DEBUG
			// Sanity check in debug builds so that we know we aren't putting the same recipe in
			// multiple times.
			FOR_EACH_MAP_FAST( m_mapRecipes, i )
			{
				Assert( i != nRecipeIndex );
				Assert( m_mapRecipes[i] != recipeDef );
			}
#endif // _DEBUG

			// Store this recipe.
			m_mapRecipes.Insert( nRecipeIndex, recipeDef );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}


//-----------------------------------------------------------------------------
// Purpose:	Builds the name of a achievement in the form App<ID>.<AchName>
// Input:	unAppID - native app ID
//			pchNativeAchievementName - name of the achievement in its native app
// Returns: The combined achievement name
//-----------------------------------------------------------------------------
CUtlString CEconItemSchema::ComputeAchievementName( AppId_t unAppID, const char *pchNativeAchievementName ) 
{
	return CFmtStr1024( "App%u.%s", unAppID, pchNativeAchievementName ).Access();
}


//-----------------------------------------------------------------------------
// Purpose:	Initializes the achievement rewards section of the schema
// Input:	pKVAchievementRewards - The achievement_rewards section of the KeyValues 
//				representation of the schema
//			pVecErrors - An optional vector that will contain error messages if 
//				the init fails.
// Output:	True if initialization succeeded, false otherwise
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitAchievementRewards( KeyValues *pKVAchievementRewards, CUtlVector<CUtlString> *pVecErrors )
{
	m_dictAchievementRewards.RemoveAll();
	m_mapAchievementRewardsByData.PurgeAndDeleteElements();

	// initialize the rewards sections
	if ( NULL != pKVAchievementRewards )
	{
		FOR_EACH_SUBKEY( pKVAchievementRewards, pKVReward )
		{
			AchievementAward_t award;
			if( pKVReward->GetDataType() == KeyValues::TYPE_NONE )
			{
				int32 nItemIndex = pKVReward->GetInt( "DefIndex", -1 );
				if( nItemIndex != -1 )
				{
					award.m_vecDefIndex.AddToTail( (uint16)nItemIndex );			
				}
				else
				{
					KeyValues *pkvItems = pKVReward->FindKey( "Items" );
					SCHEMA_INIT_CHECK( 
						pkvItems != NULL,
						"Complex achievement %s must have an Items key or a DefIndex field", pKVReward->GetName() );
					if( !pkvItems )
					{
						continue;
					}

					FOR_EACH_VALUE( pkvItems, pkvItem )
					{
						award.m_vecDefIndex.AddToTail( (uint16)Q_atoi( pkvItem->GetName() ) );
					}
				}

			}
			else
			{
				award.m_vecDefIndex.AddToTail( (uint16)pKVReward->GetInt("", -1 ) );
			} 
		
			// make sure all the item types are valid
			bool bFoundAllItems = true;
			FOR_EACH_VEC( award.m_vecDefIndex, nItem )
			{
				const CEconItemDefinition *pDefn = GetItemDefinition( award.m_vecDefIndex[nItem] );
				SCHEMA_INIT_CHECK( 
					pDefn != NULL,
					"Item definition index %d in achievement reward %s was not found", award.m_vecDefIndex[nItem], pKVReward->GetName() );
				if( !pDefn )
				{
					bFoundAllItems = false;
				}
			}
			if( !bFoundAllItems )
				continue;

			SCHEMA_INIT_CHECK( 
				award.m_vecDefIndex.Count() > 0,
				"Achievement reward %s has no items!", pKVReward->GetName() );
			if( award.m_vecDefIndex.Count() == 0 )
				continue;

			award.m_unSourceAppId = k_uAppIdInvalid;
			if( pKVReward->GetDataType() == KeyValues::TYPE_NONE )
			{
				// cross game achievement
				award.m_sNativeName = pKVReward->GetName();
				award.m_unAuditData = pKVReward->GetInt( "AuditData", 0 );
				award.m_unSourceAppId = pKVReward->GetInt( "SourceAppID", award.m_unSourceAppId );
			}
			else
			{
				award.m_sNativeName = pKVReward->GetName();
				award.m_unAuditData = 0;
			}




			AchievementAward_t *pAward = new AchievementAward_t;
			*pAward = award;

			m_dictAchievementRewards.Insert( ComputeAchievementName( pAward->m_unSourceAppId, pAward->m_sNativeName ), pAward );
			m_mapAchievementRewardsByData.Insert( pAward->m_unAuditData, pAward );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}


bool CEconItemSchema::BInitItemCriteriaTemplates( KeyValues *pKVItemCriteriaTemplates, CUtlVector<CUtlString> *pVecErrors )
{
	m_dictItemCriteriaTemplates.PurgeAndDeleteElements();

	FOR_EACH_TRUE_SUBKEY( pKVItemCriteriaTemplates, pKVItemCriteriaTemplate )
	{
		const char *pszCriteriaName = pKVItemCriteriaTemplate->GetName();

		// try to create random attrib from template
		CItemSelectionCriteria *pCriteria = CreateItemCriteria( __FUNCTION__, pKVItemCriteriaTemplate, pVecErrors );
		SCHEMA_INIT_CHECK(
			NULL != pCriteria,
			"%s: Failed to create CItemSelectionCriteria '%s'", __FUNCTION__, pszCriteriaName );

		m_dictItemCriteriaTemplates.Insert( pszCriteriaName, pCriteria );
	}

	return true;
}

bool CEconItemSchema::BInitRandomAttributeTemplates( KeyValues *pKVRandomAttributeTemplates, CUtlVector<CUtlString> *pVecErrors )
{
	m_dictRandomAttributeTemplates.PurgeAndDeleteElements();

	FOR_EACH_TRUE_SUBKEY( pKVRandomAttributeTemplates, pKVAttributeTemplate )
	{
		const char *pszAttrName = pKVAttributeTemplate->GetName();

		// try to create random attrib from template
		random_attrib_t *pRandomAttr = CreateRandomAttribute( __FUNCTION__, pKVAttributeTemplate, pVecErrors );
		SCHEMA_INIT_CHECK(
			NULL != pRandomAttr,
			"%s: Failed to create random_attrib_t '%s'", __FUNCTION__, pszAttrName );

		m_dictRandomAttributeTemplates.Insert( pszAttrName, pRandomAttr );
	}

	return true;
}

bool CEconItemSchema::BInitLootlistJobTemplates( KeyValues *pKVLootlistJobTemplates, CUtlVector<CUtlString> *pVecErrors )
{
	m_dictLootlistJobTemplates.PurgeAndDeleteElements();

	FOR_EACH_TRUE_SUBKEY( pKVLootlistJobTemplates, pKVJobTemplate )
	{
		const char *pszJobName = pKVJobTemplate->GetName();

		// try to create random attrib from template
		CLootlistJob *pJob = CreateLootlistJob( __FUNCTION__, pKVJobTemplate, pVecErrors );
		SCHEMA_INIT_CHECK(
			NULL != pJob,
			"%s: Failed to create CreateLootlistJob '%s'", __FUNCTION__, pszJobName );

		m_dictLootlistJobTemplates.Insert( pszJobName, pJob );
	}

	return true;
}


#ifdef TF_CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Go through all items and cache the number of concrete items in each.
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitConcreteItemCounts( CUtlVector<CUtlString> *pVecErrors )
{
	FOR_EACH_MAP_FAST( m_mapItems, i )
	{
		CEconItemDefinition *pItemDef = m_mapItems[ i ];
		pItemDef->m_unNumConcreteItems = CalculateNumberOfConcreteItems( pItemDef );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the number of actual "real" items referenced by the item definition
// (i.e. items that would take up space in the inventory)
//-----------------------------------------------------------------------------
int CEconItemSchema::CalculateNumberOfConcreteItems( const CEconItemDefinition *pItemDef )
{
	AssertMsg( pItemDef, "NULL item definition!  This should not happen!" );
	if ( !pItemDef )
		return 0;

	if ( pItemDef->IsBundle() )
	{
		uint32 unNumConcreteItems = 0;
		
		const bundleinfo_t *pBundle = pItemDef->GetBundleInfo();
		Assert( pBundle );

		FOR_EACH_VEC( pBundle->vecItemDefs, i )
		{
			unNumConcreteItems += CalculateNumberOfConcreteItems( pBundle->vecItemDefs[i] );
		}

		return unNumConcreteItems;
	}

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitSteamPackageLocalizationToken( KeyValues *pKVSteamPackages, CUtlVector<CUtlString> *pVecErrors )
{
	if ( NULL != pKVSteamPackages )
	{
		FOR_EACH_TRUE_SUBKEY( pKVSteamPackages, pKVEntry )
		{
			// Check to make sure the index is positive
			int iRawPackageId = atoi( pKVEntry->GetName() );
			SCHEMA_INIT_CHECK( 
				iRawPackageId > 0,
				"Invalid package ID %i for localization", iRawPackageId );

			// Store off our data.
			uint32 unPackageId = (uint32)iRawPackageId;
			const char *pszLocalizationToken = pKVEntry->GetString( "localization_key" );

			m_mapSteamPackageLocalizationTokens.InsertOrReplace( unPackageId, pszLocalizationToken );

		}
	}
	return SCHEMA_INIT_SUCCESS();
}
#endif // TF_CLIENT_DLL

static const char *s_particle_controlpoint_names[] =
{
	"attachment",
	"control_point_1",
	"control_point_2",
	"control_point_3",
	"control_point_4",
	"control_point_5",
	"control_point_6",
};

//-----------------------------------------------------------------------------
// Purpose:	Initializes the attribute-controlled-particle-systems section of the schema
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitAttributeControlledParticleSystems( KeyValues *pKVParticleSystems, CUtlVector<CUtlString> *pVecErrors )
{
	m_mapAttributeControlledParticleSystems.RemoveAll();
	m_vecAttributeControlledParticleSystemsCosmetics.RemoveAll();
	m_vecAttributeControlledParticleSystemsWeapons.RemoveAll();
	m_vecAttributeControlledParticleSystemsTaunts.RemoveAll();
	
	CUtlVector< int > *pVec = NULL;

	// Addictional groups we are tracking for.
	// "cosmetic_unusual_effects"
	// "weapon_unusual_effects"
	// "taunt_unusual_effects"

	if ( NULL != pKVParticleSystems )
	{
		FOR_EACH_TRUE_SUBKEY( pKVParticleSystems, pKVCategory )
		{
			// There is 3 Categories we want to track with additional info
			if ( !V_strcmp( pKVCategory->GetName(), "cosmetic_unusual_effects" ) )
			{
				pVec = &m_vecAttributeControlledParticleSystemsCosmetics;
			} 
			else if ( !V_strcmp( pKVCategory->GetName(), "weapon_unusual_effects" ) )
			{
				pVec = &m_vecAttributeControlledParticleSystemsWeapons;
			}
			else if ( !V_strcmp( pKVCategory->GetName(), "taunt_unusual_effects" ) )
			{
				pVec = &m_vecAttributeControlledParticleSystemsTaunts;
			}
			else
			{
				pVec = NULL; // reset
			}
			
			FOR_EACH_TRUE_SUBKEY( pKVCategory, pKVEntry )
			{
				int32 nItemIndex = atoi( pKVEntry->GetName() );
				// Check to make sure the index is positive
				SCHEMA_INIT_CHECK( 
					nItemIndex > 0,
					"Particle system index %d greater than zero", nItemIndex );
				if ( nItemIndex <= 0 )
					continue;
				int iIndex = m_mapAttributeControlledParticleSystems.Insert( nItemIndex );
				attachedparticlesystem_t &system = m_mapAttributeControlledParticleSystems[iIndex];
				system.pszSystemName = pKVEntry->GetString( "system", NULL );
				system.bFollowRootBone = pKVEntry->GetInt( "attach_to_rootbone", 0 ) != 0;
				system.iCustomType = 0;
				system.nSystemID = nItemIndex;
				system.fRefireTime = pKVEntry->GetFloat( "refire_time", 0.0f );
				system.bDrawInViewModel = pKVEntry->GetBool( "draw_in_viewmodel", false );
				system.bUseSuffixName = pKVEntry->GetBool( "use_suffix_name", false );

				COMPILE_TIME_ASSERT( ARRAYSIZE( system.pszControlPoints ) == ARRAYSIZE( s_particle_controlpoint_names ) );
				for ( int i=0; i<ARRAYSIZE( system.pszControlPoints ); ++i )
				{
					system.pszControlPoints[i] = pKVEntry->GetString( s_particle_controlpoint_names[i], NULL );
				}

				if ( pVec )
				{
					pVec->AddToTail( nItemIndex );
				}
			}
		}
	}
	return SCHEMA_INIT_SUCCESS();
}

#ifdef CLIENT_DLL
locchar_t *CEconItemSchema::GetParticleSystemLocalizedName( int index ) const
{
	const attachedparticlesystem_t *pSystem = GetItemSchema()->GetAttributeControlledParticleSystem( index );
	if ( !pSystem )
		return NULL;

	char particleNameEntry[128];
	Q_snprintf( particleNameEntry, ARRAYSIZE( particleNameEntry ), "#Attrib_Particle%d", pSystem->nSystemID );

	return g_pVGuiLocalize->Find( particleNameEntry );
}

#endif
//-----------------------------------------------------------------------------
// Purpose: Inits data for items that can level up through kills, etc.
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitItemLevels( KeyValues *pKVItemLevels, CUtlVector<CUtlString> *pVecErrors )
{
	m_vecItemLevelingData.RemoveAll();

	// initialize the rewards sections
	if ( NULL != pKVItemLevels )
	{
		FOR_EACH_TRUE_SUBKEY( pKVItemLevels, pKVItemLevelBlock )
		{
			const char *pszLevelBlockName = pKVItemLevelBlock->GetName();
			SCHEMA_INIT_CHECK( GetItemLevelingData( pszLevelBlockName ) == NULL,
				"Duplicate leveling data block named \"%s\".", pszLevelBlockName );

			// Allocate a new structure for this block and assign it. We'll fill in the contents later.
			CUtlVector<CItemLevelingDefinition> *pLevelingData = new CUtlVector<CItemLevelingDefinition>;
			m_vecItemLevelingData.Insert( pszLevelBlockName, pLevelingData );

			FOR_EACH_TRUE_SUBKEY( pKVItemLevelBlock, pKVItemLevel )
			{
				int index = pLevelingData->AddToTail();
				SCHEMA_INIT_SUBSTEP( (*pLevelingData)[index].BInitFromKV( pKVItemLevel, pszLevelBlockName, pVecErrors ) );
			}
		}
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose: Inits data for kill eater types.
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitKillEaterScoreTypes( KeyValues *pKVKillEaterScoreTypes, CUtlVector<CUtlString> *pVecErrors )
{
	m_mapKillEaterScoreTypes.RemoveAll();

	// initialize the rewards sections
	if ( NULL != pKVKillEaterScoreTypes )
	{
		FOR_EACH_TRUE_SUBKEY( pKVKillEaterScoreTypes, pKVScoreType )
		{
			unsigned int unIndex = (unsigned int)atoi( pKVScoreType->GetName() );
			SCHEMA_INIT_CHECK( m_mapKillEaterScoreTypes.Find( unIndex ) == KillEaterScoreMap_t::InvalidIndex(),
				"Duplicate kill eater score type index %u.", unIndex );
			
			kill_eater_score_type_t ScoreType;
			ScoreType.m_pszTypeString = pKVScoreType->GetString( "type_name" );
			ScoreType.m_bAllowBotVictims = pKVScoreType->GetBool( "allow_bot_victims", false );

			const char *pszLevelBlockName = pKVScoreType->GetString( "level_data", "KillEaterRank" );
			SCHEMA_INIT_CHECK( GetItemLevelingData( pszLevelBlockName ) != NULL,
				"Unable to find leveling data block named \"%s\" for kill eater score type %u.", pszLevelBlockName, unIndex );

			ScoreType.m_pszLevelBlockName = pszLevelBlockName;

			m_mapKillEaterScoreTypes.Insert( unIndex, ScoreType );
		}
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitStringTables( KeyValues *pKVStringTables, CUtlVector<CUtlString> *pVecErrors )
{
	m_dictStringTable.PurgeAndDeleteElements();
	
	// initialize the rewards sections
	if ( NULL != pKVStringTables )
	{
		FOR_EACH_SUBKEY( pKVStringTables, pKVTable )
		{
			SCHEMA_INIT_CHECK( !m_dictStringTable.IsValidIndex( m_dictStringTable.Find( pKVTable->GetName() ) ),
				"Duplicate string table name '%s'.", pKVTable->GetName() );

			SchemaStringTableDict_t::IndexType_t i = m_dictStringTable.Insert( pKVTable->GetName(), new CUtlVector< schema_string_table_entry_t > );
			FOR_EACH_SUBKEY( pKVTable, pKVEntry )
			{
				schema_string_table_entry_t s = { atoi( pKVEntry->GetName() ), pKVEntry->GetString() };
				m_dictStringTable[i]->AddToTail( s );
			}
		}
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitCommunityMarketRemaps( KeyValues *pKVCommunityMarketRemaps, CUtlVector<CUtlString> *pVecErrors )
{
	m_mapCommunityMarketDefinitionIndexRemap.Purge();

	if ( NULL != pKVCommunityMarketRemaps )
	{
		FOR_EACH_SUBKEY( pKVCommunityMarketRemaps, pKVRemapBase )
		{
			const char *pszBaseDefName = pKVRemapBase->GetName();
			const CEconItemDefinition *pBaseItemDef = GetItemSchema()->GetItemDefinitionByName( pszBaseDefName );
			SCHEMA_INIT_CHECK( pBaseItemDef != NULL, "Unknown Market remap base definition '%s'.", pszBaseDefName );

			FOR_EACH_SUBKEY( pKVRemapBase, pKVRemap )
			{
				const char *pszDefName = pKVRemap->GetName();
				const CEconItemDefinition *pItemDef = GetItemSchema()->GetItemDefinitionByName( pszDefName );
				SCHEMA_INIT_CHECK( pItemDef != NULL, "Unknown Market remap definition '%s' (under '%s').", pszDefName, pszBaseDefName );
				SCHEMA_INIT_CHECK( m_mapCommunityMarketDefinitionIndexRemap.Find( pItemDef->GetDefinitionIndex() ) == m_mapCommunityMarketDefinitionIndexRemap.InvalidIndex(), "Duplicate Market remap definition '%s'.\n", pszDefName );

				m_mapCommunityMarketDefinitionIndexRemap.Insert( pItemDef->GetDefinitionIndex(), pBaseItemDef->GetDefinitionIndex() );
			}
		}
	}

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
item_definition_index_t CEconItemSchema::GetCommunityMarketRemappedDefinitionIndex( item_definition_index_t unSearchItemDef ) const
{
	CommunityMarketDefinitionRemapMap_t::IndexType_t index = m_mapCommunityMarketDefinitionIndexRemap.Find( unSearchItemDef );
	if ( index == m_mapCommunityMarketDefinitionIndexRemap.InvalidIndex() )
		return unSearchItemDef;

	return m_mapCommunityMarketDefinitionIndexRemap[index];
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const ISchemaAttributeType *CEconItemSchema::GetAttributeType( const char *pszAttrTypeName ) const
{
	FOR_EACH_VEC( m_vecAttributeTypes, i )
	{
		if ( m_vecAttributeTypes[i].m_sName == pszAttrTypeName )
			return m_vecAttributeTypes[i].m_pAttrType;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// CItemLevelingDefinition Accessor
//-----------------------------------------------------------------------------
const CItemLevelingDefinition *CEconItemSchema::GetItemLevelForScore( const char *pszLevelBlockName, uint32 unScore ) const
{
	const CUtlVector<CItemLevelingDefinition> *pLevelingData = GetItemLevelingData( pszLevelBlockName );
	if ( !pLevelingData )
		return NULL;

	if ( pLevelingData->Count() == 0 )
		return NULL;

	FOR_EACH_VEC( (*pLevelingData), i )
	{
		if ( unScore < (*pLevelingData)[i].GetRequiredScore() )
			return &(*pLevelingData)[i];
	}

	return &(*pLevelingData).Tail();
}

//-----------------------------------------------------------------------------
// Kill eater score type accessor
//-----------------------------------------------------------------------------
const kill_eater_score_type_t *CEconItemSchema::FindKillEaterScoreType( uint32 unScoreType ) const
{
	KillEaterScoreMap_t::IndexType_t i = m_mapKillEaterScoreTypes.Find( unScoreType );
	if ( i == KillEaterScoreMap_t::InvalidIndex() )
		return NULL;

	return &m_mapKillEaterScoreTypes[i];
}

//-----------------------------------------------------------------------------
// Kill eater score type accessor
//-----------------------------------------------------------------------------
const char *CEconItemSchema::GetKillEaterScoreTypeLocString( uint32 unScoreType ) const
{
	const kill_eater_score_type_t *pScoreType = FindKillEaterScoreType( unScoreType );

	return pScoreType
		 ? pScoreType->m_pszTypeString
		 : NULL;
}

//-----------------------------------------------------------------------------
// Kill eater score type accessor
//-----------------------------------------------------------------------------
const char *CEconItemSchema::GetKillEaterScoreTypeLevelingDataName( uint32 unScoreType ) const
{
	const kill_eater_score_type_t *pScoreType = FindKillEaterScoreType( unScoreType );

	return pScoreType
		 ? pScoreType->m_pszLevelBlockName
		 : NULL;
}

//-----------------------------------------------------------------------------
// Kill eater score type accessor
//-----------------------------------------------------------------------------
bool CEconItemSchema::GetKillEaterScoreTypeAllowsBotVictims( uint32 unScoreType ) const
{

	const kill_eater_score_type_t *pScoreType = FindKillEaterScoreType( unScoreType );

	return pScoreType
		 ? pScoreType->m_bAllowBotVictims
		 : false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
econ_tag_handle_t CEconItemSchema::GetHandleForTag( const char *pszTagName )
{
	EconTagDict_t::IndexType_t i = m_dictTags.Find( pszTagName );
	if ( m_dictTags.IsValidIndex( i ) )
		return i;

	return m_dictTags.Insert( pszTagName );
}


#if defined(CLIENT_DLL) || defined(GAME_DLL)
//-----------------------------------------------------------------------------
// Purpose:	Clones the specified item definition, and returns the new item def.
//-----------------------------------------------------------------------------
void CEconItemSchema::ItemTesting_CreateTestDefinition( int iCloneFromItemDef, int iNewDef, KeyValues *pNewKV )
{
	int nMapIndex = m_mapItems.Find( iNewDef );
	if ( !m_mapItems.IsValidIndex( nMapIndex ) )
	{
		nMapIndex = m_mapItems.Insert( iNewDef, CreateEconItemDefinition() );
		m_mapItemsSorted.Insert( iNewDef, m_mapItems[nMapIndex] );
	}

	// Find & copy the clone item def's data in
	CEconItemDefinition *pCloneDef = GetItemDefinition( iCloneFromItemDef );
	if ( !pCloneDef )
		return;
	m_mapItems[nMapIndex]->CopyPolymorphic( pCloneDef );

	// Then stomp it with the KV test contents
	m_mapItems[nMapIndex]->BInitFromTestItemKVs( iNewDef, pNewKV );
}

//-----------------------------------------------------------------------------
// Purpose:	Discards the specified item definition
//-----------------------------------------------------------------------------
void CEconItemSchema::ItemTesting_DiscardTestDefinition( int iDef )
{
	m_mapItems.Remove( iDef );
	m_mapItemsSorted.Remove( iDef );
}

//-----------------------------------------------------------------------------
// Purpose:	Initializes the armory data section of the schema
//-----------------------------------------------------------------------------
bool CEconItemSchema::BInitArmoryData( KeyValues *pKVArmoryData, CUtlVector<CUtlString> *pVecErrors )
{
	m_dictArmoryItemDataStrings.RemoveAll();
	m_dictArmoryAttributeDataStrings.RemoveAll();
	if ( NULL != pKVArmoryData )
	{
		KeyValues *pKVItemTypes = pKVArmoryData->FindKey( "armory_item_types" );
		if ( pKVItemTypes )
		{
			FOR_EACH_SUBKEY( pKVItemTypes, pKVEntry )
			{
				const char *pszDataKey = pKVEntry->GetName();
				const CUtlConstString sLocString( pKVEntry->GetString() );
				m_dictArmoryItemTypesDataStrings.Insert( pszDataKey, sLocString );
			}
		}

		pKVItemTypes = pKVArmoryData->FindKey( "armory_item_classes" );
		if ( pKVItemTypes )
		{
			FOR_EACH_SUBKEY( pKVItemTypes, pKVEntry )
			{
				const char *pszDataKey = pKVEntry->GetName();
				const CUtlConstString sLocString( pKVEntry->GetString() );
				m_dictArmoryItemClassesDataStrings.Insert( pszDataKey, sLocString );
			}
		}

		KeyValues *pKVAttribs = pKVArmoryData->FindKey( "armory_attributes" );
		if ( pKVAttribs )
		{
			FOR_EACH_SUBKEY( pKVAttribs, pKVEntry )
			{
				const char *pszDataKey = pKVEntry->GetName();
				const CUtlConstString sLocString( pKVEntry->GetString() );
				m_dictArmoryAttributeDataStrings.Insert( pszDataKey, sLocString );
			}
		}

		KeyValues *pKVItems = pKVArmoryData->FindKey( "armory_items" );
		if ( pKVItems )
		{
			FOR_EACH_SUBKEY( pKVItems, pKVEntry )
			{
				const char *pszDataKey = pKVEntry->GetName();
				const CUtlConstString sLocString( pKVEntry->GetString() );
				m_dictArmoryItemDataStrings.Insert( pszDataKey, sLocString );
			}
		}
	}
	return SCHEMA_INIT_SUCCESS();
}
#endif




//-----------------------------------------------------------------------------
// Purpose:	Returns the achievement award that matches the provided defindex or NULL
//			if there is no such award.
// Input:	unData - The data field that would be stored in ItemAudit
//-----------------------------------------------------------------------------
const AchievementAward_t *CEconItemSchema::GetAchievementRewardByDefIndex( uint16 usDefIndex ) const
{
	FOR_EACH_MAP_FAST( m_mapAchievementRewardsByData, nIndex )
	{
		if( m_mapAchievementRewardsByData[nIndex]->m_vecDefIndex.HasElement( usDefIndex ) )
			return m_mapAchievementRewardsByData[nIndex];
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:	Gets a rarity value for a name.
//-----------------------------------------------------------------------------
bool CEconItemSchema::BGetItemRarityFromName( const char *pchName, uint8 *nRarity ) const
{
	if ( 0 == Q_stricmp( "any", pchName ) )
	{
		*nRarity = k_unItemRarity_Any;
		return true;
	}

	FOR_EACH_MAP_FAST( m_mapRarities, i )
	{
		if ( 0 == Q_stricmp( m_mapRarities[i].GetName(), pchName ) )
		{
			*nRarity = m_mapRarities[i].GetDBValue();
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose:	Gets a quality value for a name.
// Input:	pchName - The name to translate.
//			nQuality - (out)The quality number for this name, if found.
// Output:	True if the string matched a quality for this schema, false otherwise.
//-----------------------------------------------------------------------------
bool CEconItemSchema::BGetItemQualityFromName( const char *pchName, uint8 *nQuality ) const
{
	if ( 0 == Q_stricmp( "any", pchName ) )
	{
		*nQuality = k_unItemQuality_Any;
		return true;
	}

	FOR_EACH_MAP_FAST( m_mapQualities, i )
	{
		if ( 0 == Q_stricmp( m_mapQualities[i].GetName(), pchName ) )
		{
			*nQuality = m_mapQualities[i].GetDBValue();
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose:	Gets a quality definition for an index
// Input:	nQuality - The quality to get.
// Output:	A pointer to the desired definition, or NULL if it is not found.
//-----------------------------------------------------------------------------
const CEconItemQualityDefinition *CEconItemSchema::GetQualityDefinition( int nQuality ) const
{ 
	int iIndex = m_mapQualities.Find( nQuality );
	if ( m_mapQualities.IsValidIndex( iIndex ) )
		return &m_mapQualities[iIndex]; 
	return NULL;
}

const CEconItemQualityDefinition *CEconItemSchema::GetQualityDefinitionByName( const char *pszDefName ) const
{
	FOR_EACH_MAP_FAST( m_mapQualities, i )
	{
		if ( V_stricmp( pszDefName, m_mapQualities[i].GetName()) == 0 )
			return &m_mapQualities[i]; 
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// ItemRarity
//-----------------------------------------------------------------------------
const CEconItemRarityDefinition *CEconItemSchema::GetRarityDefinitionByMapIndex( int nRarityIndex ) const
{
	if ( m_mapRarities.IsValidIndex( nRarityIndex ) )
		return &m_mapRarities[nRarityIndex];

	return NULL;
}
//-----------------------------------------------------------------------------
const CEconItemRarityDefinition *CEconItemSchema::GetRarityDefinition( int nRarity ) const
{
	int iIndex = m_mapRarities.Find( nRarity );
	if ( m_mapRarities.IsValidIndex( iIndex ) )
		return &m_mapRarities[iIndex];
	return NULL;
}
//-----------------------------------------------------------------------------
const CEconItemRarityDefinition *CEconItemSchema::GetRarityDefinitionByName( const char *pszDefName ) const
{
	FOR_EACH_MAP_FAST( m_mapRarities, i )
	{
		if ( !strcmp( pszDefName, m_mapRarities[i].GetName() ) )
			return &m_mapRarities[i];
	}
	return NULL;
}
//-----------------------------------------------------------------------------
const char* CEconItemSchema::GetRarityName( uint8 iRarity )
{
	const CEconItemRarityDefinition* pItemRarity = GetRarityDefinition( iRarity );
	if ( !pItemRarity )
		return NULL;
	else
		return pItemRarity->GetName();
}
//-----------------------------------------------------------------------------
const char* CEconItemSchema::GetRarityLocKey( uint8 iRarity )
{
	const CEconItemRarityDefinition* pItemRarity = GetRarityDefinition( iRarity );
	if ( !pItemRarity )
		return NULL;
	else
		return pItemRarity->GetLocKey();
}
//-----------------------------------------------------------------------------
const char* CEconItemSchema::GetRarityColor( uint8 iRarity )
{
	const CEconItemRarityDefinition* pItemRarity = GetRarityDefinition( iRarity );
	if ( !pItemRarity )
		return NULL;
	else
		return GetColorNameForAttribColor( pItemRarity->GetAttribColor() );
}
//-----------------------------------------------------------------------------
int CEconItemSchema::GetRarityIndex( const char* pszRarity )
{
	const CEconItemRarityDefinition* pRarity = GetRarityDefinitionByName( pszRarity );
	if ( pRarity )
		return pRarity->GetDBValue();
	else
		return 0;
}

//-----------------------------------------------------------------------------
bool CEconItemSchema::BGetItemSeries( const char* pchName, uint8 *nItemSeries ) const
{
	FOR_EACH_MAP_FAST( m_mapItemSeries, i )
	{
		if ( 0 == Q_stricmp( m_mapItemSeries[i].GetName(), pchName ) )
		{
			*nItemSeries = m_mapItemSeries[i].GetDBValue();
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
const CEconItemSeriesDefinition *CEconItemSchema::GetItemSeriesDefinition( int iSeries ) const
{
	int iIndex = m_mapItemSeries.Find( iSeries );
	if ( m_mapItemSeries.IsValidIndex( iIndex ) )
		return &m_mapItemSeries[iIndex];
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:	Gets an item definition for the specified definition index
// Input:	iItemIndex - The index of the desired definition.
// Output:	A pointer to the desired definition, or NULL if it is not found.
//-----------------------------------------------------------------------------
CEconItemDefinition *CEconItemSchema::GetItemDefinition( int iItemIndex )
{
#if defined(CLIENT_DLL) || defined(GAME_DLL)
#if !defined(CSTRIKE_DLL)
	AssertMsg( GetDefaultItemDefinition(), "No default item definition set up for item schema." );
#endif // CSTRIKE_DLL
#endif // defined(CLIENT_DLL) || defined(GAME_DLL)

	int iIndex = m_mapItems.Find( iItemIndex );
	if ( m_mapItems.IsValidIndex( iIndex ) )
		return m_mapItems[iIndex]; 

#if defined( GC_DLL ) || defined( EXTERNALTESTS_DLL )
	return NULL;
#else // !GC_DLL
	if ( GetDefaultItemDefinition() )
		return GetDefaultItemDefinition();

#if !defined(CSTRIKE_DLL)
	// We shouldn't ever get down here, but all the same returning a valid pointer is very slightly
	// a better plan than returning an invalid pointer to code that won't check to see if it's valid.
	static CEconItemDefinition *s_pEmptyDefinition = CreateEconItemDefinition();
	return s_pEmptyDefinition;
#else
	return NULL;
#endif // CSTRIKE_DLL

#endif // GC_DLL
}
const CEconItemDefinition *CEconItemSchema::GetItemDefinition( int iItemIndex ) const
{
	return const_cast<CEconItemSchema *>(this)->GetItemDefinition( iItemIndex );
}

//-----------------------------------------------------------------------------
// Purpose:	Gets an item definition that has a name matching the specified name.
// Input:	pszDefName - The name of the desired definition.
// Output:	A pointer to the desired definition, or NULL if it is not found.
//-----------------------------------------------------------------------------
CEconItemDefinition *CEconItemSchema::GetItemDefinitionByName( const char *pszDefName )
{
	// This shouldn't happen, but let's not crash if it ever does.
	Assert( pszDefName != NULL );
	if ( pszDefName == NULL )
		return NULL;

	FOR_EACH_MAP_FAST( m_mapItems, i )
	{
		if ( V_stricmp( pszDefName, m_mapItems[i]->GetDefinitionName()) == 0 )
			return m_mapItems[i]; 
	}
	return NULL;
}

const CEconItemDefinition *CEconItemSchema::GetItemDefinitionByName( const char *pszDefName ) const
{
	return const_cast<CEconItemSchema *>(this)->GetItemDefinitionByName( pszDefName );
}


random_attrib_t *CEconItemSchema::GetRandomAttributeTemplateByName( const char *pszAttrTemplateName ) const
{
	int index = m_dictRandomAttributeTemplates.Find( pszAttrTemplateName );
	if ( index != m_dictRandomAttributeTemplates.InvalidIndex() )
	{
		return m_dictRandomAttributeTemplates[index];
	}

	return NULL;
}

CLootlistJob *CEconItemSchema::GetLootlistJobTemplateByName( const char *pszLootlistJobTemplateName ) const
{
	int index = m_dictLootlistJobTemplates.Find( pszLootlistJobTemplateName );
	if ( index != m_dictLootlistJobTemplates.InvalidIndex() )
	{
		return m_dictLootlistJobTemplates[index];
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose:	Gets an attribute definition for an index
// Input:	iAttribIndex - The index of the desired definition.
// Output:	A pointer to the desired definition, or NULL if it is not found.
//-----------------------------------------------------------------------------
CEconItemAttributeDefinition *CEconItemSchema::GetAttributeDefinition( int iAttribIndex )
{ 
	int iIndex = m_mapAttributes.Find( iAttribIndex );
	if ( m_mapAttributes.IsValidIndex( iIndex ) )
		return &m_mapAttributes[iIndex]; 
	return NULL;
}
const CEconItemAttributeDefinition *CEconItemSchema::GetAttributeDefinition( int iAttribIndex ) const
{
	return const_cast<CEconItemSchema *>(this)->GetAttributeDefinition( iAttribIndex );
}

CEconItemAttributeDefinition *CEconItemSchema::GetAttributeDefinitionByName( const char *pszDefName )
{
	Assert( pszDefName );
	if ( !pszDefName )
		return NULL;

	VPROF_BUDGET( "CEconItemSchema::GetAttributeDefinitionByName", VPROF_BUDGETGROUP_STEAM );
	FOR_EACH_MAP_FAST( m_mapAttributes, i )
	{
		Assert( m_mapAttributes[i].GetDefinitionName() );
		if ( !m_mapAttributes[i].GetDefinitionName() )
			continue;

		if ( V_stricmp( pszDefName, m_mapAttributes[i].GetDefinitionName() ) == 0 )
			return &m_mapAttributes[i]; 
	}
	return NULL;
}
const CEconItemAttributeDefinition *CEconItemSchema::GetAttributeDefinitionByName( const char *pszDefName ) const
{
	return const_cast<CEconItemSchema *>(this)->GetAttributeDefinitionByName( pszDefName );
}

//-----------------------------------------------------------------------------
// Purpose:	Gets a recipe definition for an index
// Input:	iRecipeIndex - The index of the desired definition.
// Output:	A pointer to the desired definition, or NULL if it is not found.
//-----------------------------------------------------------------------------
CEconCraftingRecipeDefinition *CEconItemSchema::GetRecipeDefinition( int iRecipeIndex )
{ 
	int iIndex = m_mapRecipes.Find( iRecipeIndex );
	if ( m_mapRecipes.IsValidIndex( iIndex ) )
		return m_mapRecipes[iIndex]; 
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CEconColorDefinition *CEconItemSchema::GetColorDefinitionByName( const char *pszDefName )
{
	FOR_EACH_VEC( m_vecColorDefs, i )
	{
		if ( !Q_stricmp( m_vecColorDefs[i]->GetName(), pszDefName ) )
			return m_vecColorDefs[i];
	}
	return NULL;
}
const CEconColorDefinition *CEconItemSchema::GetColorDefinitionByName( const char *pszDefName ) const
{
	return const_cast<CEconItemSchema *>(this)->GetColorDefinitionByName( pszDefName );
}


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CEconItemSchema::GetSteamPackageLocalizationToken( uint32 unPackageId ) const
{
	SteamPackageLocalizationTokenMap_t::IndexType_t i = m_mapSteamPackageLocalizationTokens.Find( unPackageId );
	if ( m_mapSteamPackageLocalizationTokens.IsValidIndex( i ) )
		return m_mapSteamPackageLocalizationTokens[i]; 
	return NULL;
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:	Return the attribute specified attachedparticlesystem_t* associated with the given id.
//-----------------------------------------------------------------------------
attachedparticlesystem_t* CEconItemSchema::GetAttributeControlledParticleSystem( int id )
{
	int iIndex = m_mapAttributeControlledParticleSystems.Find( id );
	if ( m_mapAttributeControlledParticleSystems.IsValidIndex( iIndex ) )
		return &m_mapAttributeControlledParticleSystems[iIndex];
	return NULL;
}

attachedparticlesystem_t* CEconItemSchema::FindAttributeControlledParticleSystem( const char *pchSystemName )
{
	FOR_EACH_MAP_FAST( m_mapAttributeControlledParticleSystems, nSystem )
	{
		if( !Q_stricmp( m_mapAttributeControlledParticleSystems[nSystem].pszSystemName, pchSystemName ) )
			return &m_mapAttributeControlledParticleSystems[nSystem];
	}
	return NULL;
}

const CEconOperationDefinition* CEconItemSchema::GetOperationByName( const char* pszName ) const
{
	for ( int iOperation = m_dictOperationDefinitions.First(); m_dictOperationDefinitions.IsValidIndex( iOperation ); iOperation = m_dictOperationDefinitions.Next( iOperation ) )
	{
		CEconOperationDefinition *pOperation = m_dictOperationDefinitions[iOperation];
		if ( pOperation )
		{
			if ( Q_stricmp( pOperation->GetName(), pszName ) == 0 )
			{
				return pOperation;
			}
		}
	}

	return NULL;
}

#if defined(CLIENT_DLL) || defined(GAME_DLL)
bool CEconItemSchema::SetupPreviewItemDefinition( KeyValues *pKV )
{
	int nMapIndex = m_mapItems.Find( PREVIEW_ITEM_DEFINITION_INDEX );
	if ( !m_mapItems.IsValidIndex( nMapIndex ) )
	{
		nMapIndex = m_mapItems.Insert( PREVIEW_ITEM_DEFINITION_INDEX, CreateEconItemDefinition() );
	}

	CEconItemDefinition *pItemDef = m_mapItems[ nMapIndex ];
	return pItemDef->BInitFromKV( pKV );
}
#endif // defined(CLIENT_DLL) || defined(GAME_DLL)


bool CEconItemSchema::BCanStrangeFilterApplyToStrangeSlotInItem( uint32 /*strange_event_restriction_t*/ unRestrictionType, uint32 unRestrictionValue, const IEconItemInterface *pItem, int iStrangeSlot, uint32 *out_pOptionalScoreType ) const
{
	Assert( unRestrictionType != kStrangeEventRestriction_None );

	// Do we have a type for this slot? If not, move on, with the exception: all user-custom scores
	// we expect to have types, but certain weapons may not specify a base type and just use
	// kills implicitly.
	uint32 unStrangeScoreTypeBits = kKillEaterEvent_PlayerKill;
	if ( !pItem->FindAttribute( GetKillEaterAttr_Type( iStrangeSlot ), &unStrangeScoreTypeBits ) && iStrangeSlot != 0 )
		return false;

	// Do we have a restriction already in this slot? If so, move on.
	if ( pItem->FindAttribute( GetKillEaterAttr_Restriction( iStrangeSlot ) ) )
		return false;

	// We've found an open slot. Make sure that adding our restriction to this slot
	// won't result in a duplicate score-type/restriction-type entry.
	for ( int j = 0; j < GetKillEaterAttrCount(); j++ )
	{
		// Don't compare against ourself.
		if ( iStrangeSlot == j )
			continue;

		// Ignore this entry if we don't have a score type or if the score type differs from
		// our search criteria above.
		uint32 unAltStrangeScoreType;
		if ( !pItem->FindAttribute( GetKillEaterAttr_Type( j ), &unAltStrangeScoreType ) ||
			 unAltStrangeScoreType != unStrangeScoreTypeBits )
		{
			continue;
		}

		// This entry does have the same type, so tag us as a duplicate if we also have the same
		// restriction that we're trying to apply.
		uint32 unAltRestrictionType;
		uint32 unAltRestrictionValue;
		if ( pItem->FindAttribute( GetKillEaterAttr_Restriction( j ), &unAltRestrictionType ) &&
			 unAltRestrictionType == unRestrictionType &&
			 pItem->FindAttribute( GetKillEaterAttr_RestrictionValue( j ), &unAltRestrictionValue ) &&
			 unAltRestrictionValue == unRestrictionValue )
		{
			return false;
		}
	}	

	if ( out_pOptionalScoreType )
	{
		*out_pOptionalScoreType = *(float *)&unStrangeScoreTypeBits;
	}

	// Everything seems alright.
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Ensure that all of our internal structures are consistent, and
//			account for all memory that we've allocated.
// Input:	validator -		Our global validator object
//			pchName -		Our name (typically a member var in our container)
//-----------------------------------------------------------------------------
#ifdef DBGFLAG_VALIDATE
void CEconItemSchema::Validate( CValidator &validator, const char *pchName )
{
	VALIDATE_SCOPE();
	ValidateObj( m_mapQualities );

	FOR_EACH_MAP_FAST( m_mapQualities, i )
	{
		ValidateObj( m_mapQualities[i] );
	}

	ValidateObj( m_mapItems );

	FOR_EACH_MAP_FAST( m_mapItems, i )
	{
		ValidateObj( m_mapItems[i] );
	}

	ValidateObj( m_mapUpgradeableBaseItems );

	FOR_EACH_MAP_FAST( m_mapUpgradeableBaseItems, i )
	{
		ValidateObj( m_mapUpgradeableBaseItems[i] );
	}

	ValidateObj( m_mapAttributes );

	FOR_EACH_MAP_FAST( m_mapAttributes, i )
	{
		ValidateObj( m_mapAttributes[i] );
	}

	ValidateObj( m_mapRecipes );

	FOR_EACH_MAP_FAST( m_mapRecipes, i )
	{
		ValidateObj( m_mapRecipes[i] );
	}

	FOR_EACH_VEC( m_vecTimedRewards, i )
	{
		ValidateObj( m_vecTimedRewards[i] );
	}
	ValidateObj( m_vecTimedRewards );

}
#endif // DBGFLAG_VALIDATE


bool CEconItemSchema::BPostSchemaInit( CUtlVector<CUtlString> *pVecErrors )
{
	// We need the protodefs to be initialized
	if ( !GetProtoScriptObjDefManager()->BDefinitionsLoaded() )
	{
		GetProtoScriptObjDefManager()->BInitDefinitions();
	}
	else
	{
		// If they were already initialized, do another PostInit as this might
		// be a new schema
		GetProtoScriptObjDefManager()->BPostDefinitionsLoaded();
	}

	bool bAllSuccess = true;

	// Make sure all of our tools are valid. We have to do this after the whole schema is initialized so
	// that we don't run into circular reference problems with items referencing loot lists that reference
	// items, etc.
	FOR_EACH_MAP_FAST( m_mapItems, i )
	{
		if ( !m_mapItems[i]->BPostInit( pVecErrors ) )
		{
			bAllSuccess = false;
		}
	}

	for ( int idx = m_dictItemCollections.First(); m_dictItemCollections.IsValidIndex( idx ); idx = m_dictItemCollections.Next( idx ) )
	{
		if ( !m_dictItemCollections[idx]->BPostSchemaInit( pVecErrors ) )
		{
			bAllSuccess = false;
		}
	}

	// make sure all lootlist are valid
	for ( int idx = m_dictLootLists.First(); m_dictLootLists.IsValidIndex( idx ); idx = m_dictLootLists.Next( idx ) )
	{
		if ( !m_dictLootLists[idx]->BPostInit( pVecErrors ) )
		{
			bAllSuccess = false;
		}
	}


	return bAllSuccess;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
CItemLevelingDefinition::CItemLevelingDefinition( void )
{
}

//-----------------------------------------------------------------------------
// Purpose:	Copy constructor
//-----------------------------------------------------------------------------
CItemLevelingDefinition::CItemLevelingDefinition( const CItemLevelingDefinition &that )
{
	(*this) = that;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
CItemLevelingDefinition::~CItemLevelingDefinition()
{
	// Free up strdup() memory.
	free( m_pszLocalizedName_LocalStorage );
}

//-----------------------------------------------------------------------------
// Purpose:	Operator=
//-----------------------------------------------------------------------------
CItemLevelingDefinition &CItemLevelingDefinition::operator=( const CItemLevelingDefinition &other )
{
	m_unLevel = other.m_unLevel;
	m_unRequiredScore = other.m_unRequiredScore;
	m_pszLocalizedName_LocalStorage = strdup( other.m_pszLocalizedName_LocalStorage );

	return *this;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CItemLevelingDefinition::BInitFromKV( KeyValues *pKVItemLevel, const char *pszLevelBlockName, CUtlVector<CUtlString> *pVecErrors )
{
	m_unLevel = Q_atoi( pKVItemLevel->GetName() );
	m_unRequiredScore = pKVItemLevel->GetInt( "score" );
	m_pszLocalizedName_LocalStorage = strdup( pKVItemLevel->GetString( "rank_name", CFmtStr( "%s%i", pszLevelBlockName, m_unLevel ).Access() ) );

	return SCHEMA_INIT_SUCCESS();
}


static CSchemaAttributeDefHandle s_pAttrDef_Unusual[] =
{
	CSchemaAttributeDefHandle( "attach particle effect" ),
	CSchemaAttributeDefHandle( "hat only unusual effect" ),
	CSchemaAttributeDefHandle( "taunt only unusual effect" ),
	CSchemaAttributeDefHandle( "taunt attach particle index" ),
};

bool IsUnusualAttribute( const CEconItemAttributeDefinition *pAttrDef )
{
	for ( int i=0; i<ARRAYSIZE( s_pAttrDef_Unusual ); ++i )
	{
		if ( pAttrDef == s_pAttrDef_Unusual[i] )
		{
			return true;
		}
	}

	return false;
}

bool ItemHasUnusualAttribute( const IEconItemInterface *pItem, const CEconItemAttributeDefinition **pUnusualAttribute /*= NULL*/, uint32 *pUnAttributeValue /*= NULL*/ )
{
	for ( int i=0; i<ARRAYSIZE( s_pAttrDef_Unusual ); ++i )
	{
		uint32 unVal = 0;
		if ( pItem->FindAttribute( s_pAttrDef_Unusual[i], &unVal ) )
		{
			if ( pUnusualAttribute )
			{
				*pUnusualAttribute = s_pAttrDef_Unusual[i];
			}

			if ( pUnAttributeValue )
			{
				*pUnAttributeValue = unVal;
			}

			return true;
		}
	}
	
	return false;
}

bool IsPaintKitTool( const CEconItemDefinition *pItemDef )
{
	static CSchemaItemDefHandle pPaintkitToolItemDef( "Paintkit" );
	return pItemDef->GetRemappedItemDefIndex() == pPaintkitToolItemDef->GetDefinitionIndex();
}


const CEconItemDefinition *CEconItemSchema::GetPaintKitItemDefinition( uint32 unPaintKitDefIndex ) const
{
	int iIndex = m_mapPaintKitTools.Find( unPaintKitDefIndex );
	if ( iIndex != m_mapPaintKitTools.InvalidIndex() )
	{
		return m_mapPaintKitTools[ iIndex ];
	}

	return NULL;
}


const CEconItemCollectionDefinition *CEconItemSchema::GetPaintKitCollectionFromItem( const IEconItemInterface *pItem, uint32 *pUnPaintKitDefIndex /*= NULL*/ ) const
{
	Assert( pItem );
	
	const CEconItemCollectionDefinition *pCollection = NULL;
	uint32 unPaintKitDef;
	if ( GetPaintKitDefIndex( pItem, &unPaintKitDef ) )
	{
		const CEconItemDefinition *pPaintKitItemDef = GetPaintKitItemDefinition( unPaintKitDef );
		if ( pPaintKitItemDef )
		{
			pCollection = pPaintKitItemDef->GetItemCollectionDefinition();
		}

		if ( pUnPaintKitDefIndex )
		{
			*pUnPaintKitDefIndex = unPaintKitDef;
		}
	}

	return pCollection;
}

