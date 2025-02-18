//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds constants for the econ item system
//
//=============================================================================

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *g_szQualityStrings[] =
{
	"Normal",
	"rarity1",		// Genuine
	"rarity2",		// Customized
	"vintage",		// Vintage has to stay at 3 for backwards compatibility
	"rarity3",		// Well-Designed
	"rarity4",		// Unusual
	"Unique",
	"community",
	"developer",
	"selfmade",
	"customized",
	"strange",
	"completed",
	"haunted",
	"collectors",
	"paintkitWeapon",

	"default",		// AE_RARITY_DEFAULT,
	"common",		// AE_RARITY_COMMON,
	"uncommon",		// AE_RARITY_UNCOMMON,
	"rare",			// AE_RARITY_RARE,
	"mythical",		// AE_RARITY_MYTHICAL,
	"legendary",	// AE_RARITY_LEGENDARY,
	"ancient",		// AE_RARITY_ANCIENT,
};

COMPILE_TIME_ASSERT( ARRAYSIZE( g_szQualityStrings ) == AE_MAX_TYPES );

const char *EconQuality_GetQualityString( EEconItemQuality eQuality )
{
	// This is a runtime check and not an assert because we could theoretically bounce the GC with new
	// qualities while the client is running.
	if ( eQuality >= 0 && eQuality < AE_MAX_TYPES )
		return g_szQualityStrings[ eQuality ];

	return NULL;
}

EEconItemQuality EconQuality_GetQualityFromString( const char* pszQuality )
{
	// Convert to lowercase
	CUtlString strLoweredInput( pszQuality );
	strLoweredInput.ToLower();

	// Guaranteed with the compile time assert above that AE_MAX_TYPES is
	// the size of the string qualities
	for( int i = 0; i < AE_MAX_TYPES; ++i )
	{
		// Convert to lowercase
		CUtlString strLoweredQuality( g_szQualityStrings[i] );
		strLoweredQuality.ToLower();

		if( !Q_stricmp( strLoweredInput.Get(), strLoweredQuality.Get() ) )
			return EEconItemQuality(i);
	}

	return AE_UNDEFINED;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *g_szQualityColorStrings[] =
{
	"QualityColorNormal",
	"QualityColorrarity1",
	"QualityColorrarity2",
	"QualityColorVintage",
	"QualityColorrarity3",
	"QualityColorrarity4",				// AE_UNUSUAL
	"QualityColorUnique",
	"QualityColorCommunity",
	"QualityColorDeveloper",
	"QualityColorSelfMade",
	"QualityColorSelfMadeCustomized",
	"QualityColorStrange",
	"QualityColorCompleted",
	"QualityColorHaunted",				// AE_HAUNTED
	"QualityColorCollectors",			// AE_COLLECTORS
	"QualityColorPaintkitWeapon",		// AE_PAINTKITWEAPON

	"ItemRarityDefault"		, // AE_RARITY_DEFAULT,
	"ItemRarityCommon"		, // AE_RARITY_COMMON,
	"ItemRarityUncommon"	, // AE_RARITY_UNCOMMON,
	"ItemRarityRare"		, // AE_RARITY_RARE,
	"ItemRarityMythical"	, // AE_RARITY_MYTHICAL,
	"ItemRarityLegendary"	, // AE_RARITY_LEGENDARY,
	"ItemRarityAncient"		, // AE_RARITY_ANCIENT,
};

COMPILE_TIME_ASSERT( ARRAYSIZE( g_szQualityColorStrings ) == AE_MAX_TYPES );

const char *EconQuality_GetColorString( EEconItemQuality eQuality )
{
	if ( eQuality >= 0 && eQuality < AE_MAX_TYPES )
		return g_szQualityColorStrings[ eQuality ];

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *g_szQualityLocalizationStrings[] =
{
	"#Normal",
	"#rarity1",		// Genuine
	"#rarity2",
	"#vintage",
	"#rarity3",		// Artisan
	"#rarity4",		// Unusual
	"#unique",
	"#community",
	"#developer",
	"#selfmade",
	"#customized",
	"#strange",
	"#completed",
	"#haunted",
	"#collectors",
	"#paintkitWeapon",

	"#Rarity_Default",
	"#Rarity_Common",
	"#Rarity_Uncommon",
	"#Rarity_Rare",
	"#Rarity_Mythical",
	"#Rarity_Legendary",
	"#Rarity_Ancient"
};

COMPILE_TIME_ASSERT( ARRAYSIZE( g_szQualityLocalizationStrings ) == AE_MAX_TYPES );

const char *EconQuality_GetLocalizationString( EEconItemQuality eQuality )
{
	if ( eQuality >= 0 && eQuality < AE_MAX_TYPES )
		return g_szQualityLocalizationStrings[ eQuality ];

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Sort order for rarities
// Small Numbers sort to front
//-----------------------------------------------------------------------------
int g_nRarityScores[] =
{
	15,		// AE_NORMAL,
	10,		// AE_RARITY1,	// Geniune
	102,	// AE_RARITY2,	// Customized (unused)
	11,		// AE_VINTAGE,
	101,	// AE_RARITY3,	// Artisan (unused)
	0,		// AE_UNUSUAL,
	14,		// AE_UNIQUE,
	-1,		// AE_COMMUNITY,
	-3,		// AE_DEVELOPER,
	-2,		// AE_SELFMADE,
	100,	// AE_CUSTOMIZED,		// Unused
	9,		// AE_STRANGE,
	103,	// AE_COMPLETED,		// Unused
	13,		// AE_HAUNTED
	12,		// AE_COLLECTORS
	8,		// AE_PAINTKITWEAPON
	7,		// AE_RARITY_DEFAULT,
	6,		// AE_RARITY_COMMON,
	5,		// AE_RARITY_UNCOMMON,
	4,		// AE_RARITY_RARE,
	3,		// AE_RARITY_MYTHICAL,
	2,		// AE_RARITY_LEGENDARY,
	1,		// AE_RARITY_ANCIENT,
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_nRarityScores ) == AE_MAX_TYPES );

//-----------------------------------------------------------------------------
int EconQuality_GetRarityScore( EEconItemQuality eQuality )
{
	if ( eQuality >= 0 && eQuality < AE_MAX_TYPES )
		return g_nRarityScores[ eQuality ];

	return 0;
}

//-----------------------------------------------------------------------------
const char *g_pchWearAmountStrings[] =
{
	"#TFUI_InvTooltip_None",
	"#TFUI_InvTooltip_FactoryNew",
	"#TFUI_InvTooltip_MinimalWear",
	"#TFUI_InvTooltip_FieldTested",
	"#TFUI_InvTooltip_WellWorn",
	"#TFUI_InvTooltip_BattleScared"
};

//-----------------------------------------------------------------------------
int EconWear_ToIntCategory( float flWear )
{
	if ( flWear <= 0.2f )
	{
		return 1;
	}
	else if ( flWear <= 0.4f )
	{
		return 2;
	}
	else if ( flWear <= 0.6f )
	{
		return 3;
	}
	else if ( flWear <= 0.8f )
	{
		return 4;
	}
	else if ( flWear <= 1.0f )
	{
		return 5;
	}

	return 3; // default wear
}

// -------------------------------------------------------------------------
// Shim to return a value for buckets.  For strange we bucket all of them in to 1 non-instance data group
int EconStrange_ToStrangeBucket( float value )
{
	return 0;
}
float EconStrange_FromStrangeBucket( int value )
{
	return 0;
}

//-----------------------------------------------------------------------------
const char *GetWearLocalizationString( float flWear )
{
	int nIndex = EconWear_ToIntCategory( flWear );
	return g_pchWearAmountStrings[ nIndex ];
}
//-----------------------------------------------------------------------------
bool EconWear_IsValidValue( int nWear )
{
	return nWear > 0 && nWear <= 5;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSchemaColorDefHandle g_AttribColorDefs[] =
{
	CSchemaColorDefHandle( "desc_level" ),				// ATTRIB_COL_LEVEL
	CSchemaColorDefHandle( "desc_attrib_neutral" ),		// ATTRIB_COL_NEUTRAL
	CSchemaColorDefHandle( "desc_attrib_positive" ),	// ATTRIB_COL_POSITIVE
	CSchemaColorDefHandle( "desc_attrib_negative" ),	// ATTRIB_COL_NEGATIVE
	CSchemaColorDefHandle( "desc_itemset_name" ),		// ATTRIB_COL_ITEMSET_NAME
	CSchemaColorDefHandle( "desc_itemset_equipped" ),	// ATTRIB_COL_ITEMSET_EQUIPPED
	CSchemaColorDefHandle( "desc_itemset_missing" ),	// ATTRIB_COL_ITEMSET_MISSING
	CSchemaColorDefHandle( "desc_bundle" ),				// ATTRIB_COL_BUNDLE_ITEM
	CSchemaColorDefHandle( "desc_limited_use" ),		// ATTRIB_COL_LIMITED_USE
	CSchemaColorDefHandle( "desc_flags" ),				// ATTRIB_COL_component_flags
	CSchemaColorDefHandle( "desc_limited_quantity" ),	// ATTRIB_COL_LIMITED_QUANTITY

	CSchemaColorDefHandle( "desc_default" ),			// ATTRIB_COL_RARITY_DEFAULT
	CSchemaColorDefHandle( "desc_common" ),				// ATTRIB_COL_RARITY_COMMON
	CSchemaColorDefHandle( "desc_uncommon" ),			// ATTRIB_COL_RARITY_UNCOMMON
	CSchemaColorDefHandle( "desc_rare" ),				// ATTRIB_COL_RARITY_RARE
	CSchemaColorDefHandle( "desc_mythical" ),			// ATTRIB_COL_RARITY_MYTHICAL
	CSchemaColorDefHandle( "desc_legendary" ),			// ATTRIB_COL_RARITY_LEGENDARY
	CSchemaColorDefHandle( "desc_ancient" ),			// ATTRIB_COL_RARITY_ANCIENT
	CSchemaColorDefHandle( "desc_immortal" ),			// ATTRIB_COL_RARITY_IMMORTAL
	CSchemaColorDefHandle( "desc_arcana" ),				// ATTRIB_COL_RARITY_ARCANA

	CSchemaColorDefHandle( "desc_strange" ),			// ATTRIB_COL_STRANGE
	CSchemaColorDefHandle( "desc_unusual" ),			// ATTRIB_COL_UNUSUAL
};

COMPILE_TIME_ASSERT( ARRAYSIZE( g_AttribColorDefs ) == NUM_ATTRIB_COLORS );

attrib_colors_t GetAttribColorIndexForName( const char* pszName )
{
	for ( int i = 0; i < NUM_ATTRIB_COLORS; ++i )
	{
		if ( !Q_strcmp( g_AttribColorDefs[i].GetName(), pszName ) )
			return (attrib_colors_t)i;
	}

	return (attrib_colors_t)0;
}

const char *GetColorNameForAttribColor( attrib_colors_t unAttribColor )
{
	Assert( unAttribColor >= 0 );
	Assert( unAttribColor < NUM_ATTRIB_COLORS );

	return g_AttribColorDefs[unAttribColor]
		 ? g_AttribColorDefs[unAttribColor]->GetColorName()
		 : "ItemAttribNeutral";
}

const char *GetHexColorForAttribColor( attrib_colors_t unAttribColor )
{
	Assert( unAttribColor >= 0 );
	Assert( unAttribColor < NUM_ATTRIB_COLORS );

	return g_AttribColorDefs[unAttribColor]
	     ? g_AttribColorDefs[unAttribColor]->GetHexColor()
		 : "#ebe2ca";
}

entityquality_t GetItemQualityFromString( const char *sQuality )
{
	for ( int i = 0; i < AE_MAX_TYPES; i++ )
	{
		if ( !Q_strnicmp( sQuality, g_szQualityStrings[i], 16 ) )
			return (entityquality_t)i;
	}

	return AE_NORMAL;
}

const char *g_szRecipeCategoryStrings[] =
{
	"crafting",		// RECIPE_CATEGORY_CRAFTINGITEMS = 0,
	"commonitem",	// RECIPE_CATEGORY_COMMONITEMS,
	"rareitem",		// RECIPE_CATEGORY_RAREITEMS,
	"special",		// RECIPE_CATEGORY_SPECIAL,
};

COMPILE_TIME_ASSERT( ARRAYSIZE( g_szRecipeCategoryStrings ) == NUM_RECIPE_CATEGORIES );

//-----------------------------------------------------------------------------
// Item acquisition.
//-----------------------------------------------------------------------------
// Strings shown to the local player in the pickup dialog
const char *g_pszItemPickupMethodStrings[] = 
{
	"#NewItemMethod_Dropped",			// UNACK_ITEM_DROPPED = 1,
	"#NewItemMethod_Crafted",			// UNACK_ITEM_CRAFTED,
	"#NewItemMethod_Traded",			// UNACK_ITEM_TRADED,
	"#NewItemMethod_Purchased",			// UNACK_ITEM_PURCHASED,
	"#NewItemMethod_FoundInCrate",		// UNACK_ITEM_FOUND_IN_CRATE,
	"#NewItemMethod_Gifted",			// UNACK_ITEM_GIFTED,
	"#NewItemMethod_Support",			// UNACK_ITEM_SUPPORT,
	"#NewItemMethod_Promotion",			// UNACK_ITEM_PROMOTION,
	"#NewItemMethod_Earned",			// UNACK_ITEM_EARNED,
	"#NewItemMethod_Refunded",			// UNACK_ITEM_REFUNDED,
	"#NewItemMethod_GiftWrapped",		// UNACK_ITEM_GIFT_WRAPPED,
	"#NewItemMethod_Foreign",			// UNACK_ITEM_FOREIGN,
	"#NewItemMethod_CollectionReward",	// UNACK_ITEM_COLLECTION_REWARD
	"#NewItemMethod_PreviewItem",		// UNACK_ITEM_PREVIEW_ITEM
	"#NewItemMethod_PreviewItemPurchased", // UNACK_ITEM_PREVIEW_ITEM_PURCHASED
	"#NewItemMethod_PeriodicScoreReward",// UNACK_ITEM_PERIODIC_SCORE_REWARD
	"#NewItemMethod_MvMBadgeCompletionReward",// UNACK_ITEM_MVM_MISSION_COMPLETION_REWARD
	"#NewItemMethod_MvMSquadSurplusReward",// UNACK_ITEM_MVM_SQUAD_SURPLUS_REWARD
	"#NewItemMethod_HolidayGift",		// UNACK_ITEM_FOUND_HOLIDAY_GIFT
	"#NewItemMethod_CommunityMarketPurchase",	// UNACK_ITEM_COMMUNITY_MARKET_PURCHASE
	"#NewItemMethod_RecipeOutput",	// UNACK_ITEM_RECIPE_OUTPUT
	NULL,							// UNACK_ITEM_HIDDEN_QUEST_ITEM
	"#NewItemMethod_QuestOutput",	// UNACK_ITEM_QUEST_OUTPUT
	"#NewItemMethod_QuestLoaner",	// UNACK_ITEM_QUEST_LOANER
	"#NewItemMethod_TradeUp",		// UNACK_ITEM_TRADE_UP
	"#NewItemMethod_QuestMerasmissionOutput", //UNACK_ITEM_QUEST_MERASMISSION_OUTPUT
	"#NewItemMethod_ViralCompetitiveBetaPassSpread", //UNACK_ITEM_VIRAL_COMPETITIVE_BETA_PASS_SPREAD
	"#NewItemMethod_BloodMoneyPurchase", //UNACK_ITEM_CYOA_BLOOD_MONEY_PURCHASE
	"#NewItemMethod_PaintKit", //UNACK_ITEM_PAINTKIT
#ifdef ENABLE_STORE_RENTAL_BACKEND
	"#NewItemMethod_RentalPurchase",	// UNACK_ITEM_RENTAL_PURCHASE
#endif
};

COMPILE_TIME_ASSERT( ARRAYSIZE( g_pszItemPickupMethodStrings ) == (UNACK_NUM_METHODS - 1) );		// -1 because UNACK_ITEM_DROPPED is index 1, not 0

const char *g_pszItemPickupMethodStringsUnloc[] = 
{
	"dropped",			// UNACK_ITEM_DROPPED = 1,
	"crafted",			// UNACK_ITEM_CRAFTED,
	"traded",			// UNACK_ITEM_TRADED,
	"purchased",		// UNACK_ITEM_PURCHASED,
	"found_in_crate",	// UNACK_ITEM_FOUND_IN_CRATE,
	"gifted",			// UNACK_ITEM_GIFTED,
	"support",			// UNACK_ITEM_SUPPORT,
	"promotion",		// UNACK_ITEM_PROMOTION,
	"earned",			// UNACK_ITEM_EARNED,
	"refunded",			// UNACK_ITEM_REFUNDED,
	"gift_wrapped",		// UNACK_ITEM_GIFT_WRAPPED
	"foreign",			// UNACK_ITEM_FOREIGN
	"collection_reward",// UNACK_ITEM_COLLECTION_REWARD
	"preview_item",		// UNACK_ITEM_PREVIEW_ITEM
	"preview_item_purchased", // UNACK_ITEM_PREVIEW_ITEM_PURCHASED
	"periodic_score_reward", // UNACK_ITEM_PERIODIC_SCORE_REWARD
	"mvm_badge_completion_reward", // UNACK_ITEM_MVM_MISSION_COMPLETION_REWARD
	"mvm_squad_surplus_reward", // UNACK_ITEM_MVM_SQUAD_SURPLUS_REWARD
	"holiday_gift",		// UNACK_ITEM_FOUND_HOLIDAY_GIFT
	"market_purchase",	// UNACK_ITEM_COMMUNITY_MARKET_PURCHASE
	"recipe_output",	// UNACK_ITEM_RECIPE_OUTPUT
	"hidden_quest",		// UNACK_ITEM_HIDDEN_QUEST_ITEM
	"quest_output",		// UNACK_ITEM_QUEST_OUTPUT
	"trade_up",			// UNACK_ITEM_TRADE_UP
	"quest_output",		// UNACK_ITEM_QUEST_MERASMISSION_OUTPUT
	"viral_competitive_beta_pass", //UNACK_ITEM_VIRAL_COMPETITIVE_BETA_PASS_SPREAD
	"cyoa_blood_money_purcahase", //UNACK_ITEM_CYOA_BLOOD_MONEY_PURCHASE
	"paintkit",			// UNACK_ITEM_PAINTKIT
#ifdef ENABLE_STORE_RENTAL_BACKEND
	"rental_purchase",	// UNACK_ITEM_RENTAL_PURCHASE
#endif
};

COMPILE_TIME_ASSERT( ARRAYSIZE( g_pszItemPickupMethodStringsUnloc ) == (UNACK_NUM_METHODS - 1) );

// Strings shown to other players in the chat dialog
const char *g_pszItemFoundMethodStrings[] = 
{
	"#Item_Found",				// UNACK_ITEM_DROPPED = 1,
	"#Item_Crafted",			// UNACK_ITEM_CRAFTED,
	"#Item_Traded",				// UNACK_ITEM_TRADED,
	NULL,						// UNACK_ITEM_PURCHASED,
	"#Item_FoundInCrate",		// UNACK_ITEM_FOUND_IN_CRATE,
	"#Item_Gifted",				// UNACK_ITEM_GIFTED,
	NULL,						// UNACK_ITEM_SUPPORT,
	NULL,						// UNACK_ITEM_PROMOTION
	"#Item_Earned",				// UNACK_ITEM_EARNED
	"#Item_Refunded",			// UNACK_ITEM_REFUNDED
	"#Item_GiftWrapped",		// UNACK_ITEM_GIFT_WRAPPED
	"#Item_Foreign",			// UNACK_ITEM_FOREIGN
	"#Item_CollectionReward",	// UNACK_ITEM_COLLECTION_REWARD
	"#Item_PreviewItem",		// UNACK_ITEM_PREVIEW_ITEM
	"#Item_PreviewItemPurchased",// UNACK_ITEM_PREVIEW_ITEM_PURCHASED
	"#Item_PeriodicScoreReward",// UNACK_ITEM_PERIODIC_SCORE_REWARD
	"#Item_MvMBadgeCompletionReward",// UNACK_ITEM_MVM_MISSION_COMPLETION_REWARD
	"#Item_MvMSquadSurplusReward",// UNACK_ITEM_MVM_SQUAD_SURPLUS_REWARD
	"#Item_HolidayGift",		// UNACK_ITEM_FOUND_HOLIDAY_GIFT
	NULL,						// UNACK_ITEM_COMMUNITY_MARKET_PURCHASE
	"#Item_RecipeOutput",		// UNACK_ITEM_RECIPE_OUTPUT
	NULL,						// UNACK_ITEM_HIDDEN_QUEST_ITEM
	"#Item_QuestOutput",		// UNACK_ITEM_QUEST_OUTPUT
	NULL,						// UNACK_ITEM_QUEST_LOANER
	"#Item_TradeUp",			// UNACK_ITEM_TRADE_UP
	"#Item_QuestMerasmissionOutput", // UNACK_ITEM_QUEST_MERASMISSION_OUTPUT
	"#Item_ViralCompetitiveBetaPassSpread", //UNACK_ITEM_VIRAL_COMPETITIVE_BETA_PASS_SPREAD
	"#Item_CYOABloodMoneyPurchase", // UNACK_ITEM_CYOA_BLOOD_MONEY_PURCHASE
	"#Item_Painkit", // UNACK_ITEM_PAINTKIT
#ifdef ENABLE_STORE_RENTAL_BACKEND
	NULL,						// UNACK_ITEM_RENTAL_PURCHASE
#endif
};

COMPILE_TIME_ASSERT( ARRAYSIZE( g_pszItemFoundMethodStrings ) == (UNACK_NUM_METHODS - 1) );


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
struct strange_attr_set_t
{
	strange_attr_set_t( const char *pScoreAttrName, const char *pTypeAttrName, const char *pRestrictionAttrName, const char *pRestrictionValueAttrName, bool bIsUserCustomizable )
		: m_attrScore( pScoreAttrName )
		, m_attrType( pTypeAttrName )
		, m_attrRestriction( pRestrictionAttrName )
		, m_attrRestrictionValue( pRestrictionValueAttrName )
		, m_bIsUserCustomizable( bIsUserCustomizable )
	{
		//
	}

	CSchemaAttributeDefHandle m_attrScore;
	CSchemaAttributeDefHandle m_attrType;
	CSchemaAttributeDefHandle m_attrRestriction;
	CSchemaAttributeDefHandle m_attrRestrictionValue;
	bool m_bIsUserCustomizable;
};

strange_attr_set_t g_KillEaterAttr[] =
{
	strange_attr_set_t( "kill eater",			"kill eater score type",		"strange restriction type 1",		"strange restriction value 1",		false ),
	strange_attr_set_t( "kill eater 2",			"kill eater score type 2",		"strange restriction type 2",		"strange restriction value 2",		false ),
	strange_attr_set_t( "kill eater 3",			"kill eater score type 3",		"strange restriction type 3",		"strange restriction value 3",		false ),

	// assumption: all of the user-customizable attributes will follow all of the schema-specified attributes
	strange_attr_set_t( "kill eater user 1",	"kill eater user score type 1",	"strange restriction user type 1",	"strange restriction user value 1",	true ),
	strange_attr_set_t( "kill eater user 2",	"kill eater user score type 2", "strange restriction user type 2",	"strange restriction user value 2",	true ),
	strange_attr_set_t( "kill eater user 3",	"kill eater user score type 3", "strange restriction user type 3",	"strange restriction user value 3",	true ),
};

int GetKillEaterAttrCount()
{
#ifdef DBGFLAG_ASSERT
	// Verify our commented assumption that all of the non-user-customizable attributes will be followed by
	// all of the user-customizable attributes.
	bool bInUserCustomizableBlock = false;

	for ( int i = 0; i < ARRAYSIZE( g_KillEaterAttr ); i++ )
	{
		if ( bInUserCustomizableBlock )
		{
			AssertMsg( g_KillEaterAttr[i].m_bIsUserCustomizable, "Ordering assumption for g_KillEaterAttr violated! User-customizable attributes should all be at the end of the list!" );
		}

		bInUserCustomizableBlock |= g_KillEaterAttr[i].m_bIsUserCustomizable;
	}
#endif

	return ARRAYSIZE( g_KillEaterAttr );
}

int GetKillEaterAttrCount_UserCustomizable()
{
	int iCount = 0;
	for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
	{
		if ( GetKillEaterAttr_IsUserCustomizable( i ) )
		{
			iCount++;
		}
	}

	return iCount;
}

const CEconItemAttributeDefinition *GetKillEaterAttr_Score( int i )
{
	Assert( i >= 0 );
	Assert( i < GetKillEaterAttrCount() );

	const CEconItemAttributeDefinition *pAttrRes = g_KillEaterAttr[i].m_attrScore;
	AssertMsg1( pAttrRes, "Missing Killeater attr score %s", g_KillEaterAttr[ i ].m_attrScore.GetName() );
	
	return pAttrRes;
}

const CEconItemAttributeDefinition *GetKillEaterAttr_Type( int i )
{
	Assert( i >= 0 );
	Assert( i < GetKillEaterAttrCount() );

	const CEconItemAttributeDefinition *pAttrRes = g_KillEaterAttr[i].m_attrType;
	AssertMsg1( pAttrRes, "Missing Killeater attr type %s", g_KillEaterAttr[ i ].m_attrType.GetName() );

	return pAttrRes;
}

const CEconItemAttributeDefinition *GetKillEaterAttr_Restriction( int i )
{
	Assert( i >= 0 );
	Assert( i < GetKillEaterAttrCount() );

	const CEconItemAttributeDefinition *pAttrRes = g_KillEaterAttr[i].m_attrRestriction;
	AssertMsg1( pAttrRes, "Missing Killeater attr restriction %s", g_KillEaterAttr[ i ].m_attrRestriction.GetName() );

	return pAttrRes;
}

const CEconItemAttributeDefinition *GetKillEaterAttr_RestrictionValue( int i )
{
	Assert( i >= 0 );
	Assert( i < GetKillEaterAttrCount() );

	const CEconItemAttributeDefinition *pAttrRes = g_KillEaterAttr[i].m_attrRestrictionValue;
	AssertMsg1( pAttrRes, "Missing Killeater attr restriction value %s", g_KillEaterAttr[ i ].m_attrRestrictionValue.GetName() );
	
	return pAttrRes;
}

bool GetKillEaterAttr_IsUserCustomizable( int i )
{
	Assert( i >= 0 );
	Assert( i < GetKillEaterAttrCount() );

	return g_KillEaterAttr[i].m_bIsUserCustomizable;
}


bool GetKilleaterValueByEvent( const IEconItemInterface* pItem, const kill_eater_event_t& EEventType, uint32& value )
{
	for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
	{
		const CEconItemAttributeDefinition *pAttribKillEater		  = GetKillEaterAttr_Score( i );
		const CEconItemAttributeDefinition *pAttribKillEaterScoreType = GetKillEaterAttr_Type( i );
		
		Assert( pAttribKillEater && pAttribKillEaterScoreType );
		if ( !pAttribKillEater || !pAttribKillEaterScoreType )
			return false;

		// make sure this item even has a kill count attribute we're looking for
		uint32 unKillEaterAttrValue;
		if ( !pItem->FindAttribute( pAttribKillEater, &unKillEaterAttrValue ) )
			continue;
		
		uint32 unKillEaterScoreTypeAttrValue = kKillEaterEvent_PlayerKill;

		float fKillEaterScoreTypeAttrValue;
		if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pItem, pAttribKillEaterScoreType, &fKillEaterScoreTypeAttrValue ) )
		{
			unKillEaterScoreTypeAttrValue = (uint32)fKillEaterScoreTypeAttrValue;
		}

		// this isn't the attribute we're trying to find
		if ( EEventType != (kill_eater_event_t)unKillEaterScoreTypeAttrValue )
			continue;

		value = unKillEaterAttrValue;
		return true;
	}

	return false;
}

// Does this thing have kill eater
bool BIsItemStrange( const IEconItemInterface *pItem )
{
	// Go over the attributes of the item, if it has any strange attributes the item is strange and don't apply
	uint32 unKillEaterAttr;
	for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
	{
		if ( pItem->FindAttribute( GetKillEaterAttr_Score( i ), &unKillEaterAttr ) )
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Get a localization token that describes why an item is not usable
//			in the trade-up crafting.  Returns NULL if no reason.  Can pass in
//			another item to compare against, which causes extra consistency checks
//-----------------------------------------------------------------------------
const char* GetCollectionCraftingInvalidReason( const IEconItemInterface *pTestItem, const IEconItemInterface *pSourceItem )
{
	if ( !pTestItem )
	{
		return "#TF_CollectionCrafting_NoItem";
	}

	uint32 nPaintkitDefindex = 0;
	bool bIsPaintkit = GetPaintKitDefIndex( pTestItem, &nPaintkitDefindex );

	// Needs to have a collection
	const CEconItemCollectionDefinition* pTestCollection = GetCollection( pTestItem );
	if ( !pTestCollection )
	{
		return "#TF_CollectionCrafting_NoCollection";
	}

	const CEconItemDefinition* pEffectiveItemDef = pTestItem->GetItemDefinition();

	// Make sure this item is a part of the collection it claims to be in
	{
		item_definition_index_t nThisDefIndex = pTestItem->GetItemDefIndex();
		bool bFound = false;
		for( int i=0; i < pTestCollection->m_iItemDefs.Count() && !bFound; ++i )
		{
			bFound |= pTestCollection->m_iItemDefs[i] == nThisDefIndex;

			// Paintkit items get extra checks.  We want to test if the item has a 
			// paintkit defindex that corresponds to the paintkit defindex of one
			// of the items in the collection.  That is, if you have a Pizza Minigun,
			// then it's technically within the collection that contains the
			// Pizza War Paint.
			if ( !bIsPaintkit )
				continue;
			
			uint32 nCollectionItemPaintkitDefindex = 0;
			const CEconItemDefinition* pCollectionItemDef = GetItemSchema()->GetItemDefinition( pTestCollection->m_iItemDefs[i] );
			if ( !GetPaintKitDefIndex( pCollectionItemDef, &nCollectionItemPaintkitDefindex ) )
				continue;

			if ( nCollectionItemPaintkitDefindex == nPaintkitDefindex )
			{
				// This is our source War Paint
				bFound = true;
				pEffectiveItemDef = pCollectionItemDef;			
			}
		}

		if ( !bFound )
		{
			return "#TF_CollectionCrafting_NoCollection";
		}
	}
	 
	// Needs rarity
	uint8 nRarity = pTestItem->GetRarity();
	if( nRarity == k_unItemRarity_Any )
	{
		return "#TF_CollectionCrafting_NoRarity";
	}

	// Can't use items with rarity at the "top" of a collection (what would they craft into?)
	if ( nRarity == pTestCollection->GetMaxRarity() )
	{
		return "#TF_CollectionCrafting_MaxRarity";
	}

	// No self mades or community items
	uint32 eQuality = pTestItem->GetQuality();
	if ( eQuality == AE_SELFMADE || eQuality == AE_COMMUNITY )
	{
		return "#TF_CollectionCrafting_NoUnusual";
	}

	// Don't let unusuals be crafted
	if ( pTestItem->BIsUnusual() )
	{
		return "#TF_CollectionCrafting_NoUnusual";
	}

	// Not allowed to be crafted?
	if ( !pTestItem->IsUsableInCrafting() )
	{
		return "#TF_CollectionCrafting_NotCraftable";
	}

	// If another item was passed in, we have a few consistency checks to make
	if ( pSourceItem )
	{
		// Need to have the same rarity
		if ( nRarity != pSourceItem->GetRarity() )
		{
			return "#TF_CollectionCrafting_MismatchRarity";
		}

		// Need to have the same strangeness
		if ( BIsItemStrange( pSourceItem ) != BIsItemStrange( pTestItem ) )
		{
			return "#TF_CollectionCrafting_MismatchStrange";
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get a localization token that describes why an item is not usable
//			in the Halloween Offering.  Returns NULL if no reason.  Can pass in
//			another item to compare against, which causes extra consistency checks
//-----------------------------------------------------------------------------
const char* GetHalloweenOfferingInvalidReason( const IEconItemInterface *pTestItem, const IEconItemInterface *pSourceItem )
{
	// Must either be a Cosmetic
	// Taunt
	// Allowable Tool (Strange part, Paint, name tag, killstreak).  Not crates, keys
	// Marketable Weapon ie Strange, Genuine, Vintage, paintkit

	// Cannot be Unusual

	if ( !pTestItem )
	{
		return "#TF_CollectionCrafting_NoItem";
	}

	// No self mades or community items
	uint32 eQuality = pTestItem->GetQuality();
	if ( eQuality == AE_SELFMADE || eQuality == AE_COMMUNITY )
	{
		return "#TF_CollectionCrafting_NoUnusual";
	}

	// This is how we test for unusuals.  Don't let unusuals be crafted
	static CSchemaAttributeDefHandle pAttrDef_ParticleEffect( "attach particle effect" );
	if ( pTestItem->FindAttribute( pAttrDef_ParticleEffect ) )
	{
		return "#TF_CollectionCrafting_NoUnusual";
	}

	static CSchemaAttributeDefHandle pAttrDef_TauntUnusualAttr( "taunt attach particle index" );
	if ( pTestItem->FindAttribute( pAttrDef_TauntUnusualAttr ) )
	{
		return "#TF_CollectionCrafting_NoUnusual";
	}

	// Invalid Items
	static CSchemaAttributeDefHandle pAttrDef_CannotTransmute( "cannot_transmute" );
	if ( pTestItem->FindAttribute( pAttrDef_CannotTransmute ) )
	{
		return "#TF_HalloweenOffering_Invalid";
	}

	static CSchemaAttributeDefHandle pAttrDef_CannotDelete( "cannot delete" );
	if ( pTestItem->FindAttribute( pAttrDef_CannotDelete ) )
	{
		return "#TF_HalloweenOffering_Invalid";
	}

	const CEconItemDefinition *pItemDef = pTestItem->GetItemDefinition();
	if ( pItemDef == NULL )
	{
		return "#TF_CollectionCrafting_NoItem";
	}

	if ( pTestItem->IsTemporaryItem() )
	{
		return "#TF_CollectionCrafting_NoItem";
	}

	// If you are a taunt or a cosmetic you are allowed
	if ( pTestItem->GetItemDefinition()->GetLoadoutSlot( 0 ) == LOADOUT_POSITION_MISC || pTestItem->GetItemDefinition()->GetLoadoutSlot( 0 ) == LOADOUT_POSITION_TAUNT )
	{
		// do not 'medal' equip region items
		if ( pTestItem->GetItemDefinition()->GetEquipRegionMask() & GetItemSchema()->GetEquipRegionBitMaskByName( "medal" ) )
		{
			return "#TF_HalloweenOffering_Invalid";
		}

		return NULL;
	}

	// Do not allow Crates
	if ( ( pItemDef->GetCapabilities() & ITEM_CAP_DECODABLE ) != 0 )
	{
		return "#TF_HalloweenOffering_Invalid";
	}

	// Cause of weird legacy items lets be explicit about what we allow
	if ( pItemDef->IsTool() )
	{
		// ignore everything that is not a paint can tool
		const IEconTool *pEconTool = pItemDef->GetEconTool();
		if ( !pEconTool )
			return "#TF_HalloweenOffering_Invalid";

		const char *pToolType = pEconTool->GetTypeName();

		if ( !V_strcmp( pToolType, "paint_can" ) ) 
			return NULL;
		else if ( !V_strcmp( pToolType, "strange_part" ) ) 
			return NULL;
		else if ( !V_strcmp( pToolType, "name" ) ) 
			return NULL;
		else if ( !V_strcmp( pToolType, "desc" ) ) 
			return NULL;
		else if ( !V_strcmp( pToolType, "killstreakifier" ) )
			return NULL;
		else if ( !V_strcmp( pToolType, "strangifier" ) )
			return NULL;

		// Not a tool we are allowing
		return "#TF_HalloweenOffering_Invalid";
	}

	// Otherwise you must be a weapon or we won't allow
	if ( pTestItem->GetItemDefinition()->GetLoadoutSlot( 0 ) == LOADOUT_POSITION_PRIMARY 
		|| pTestItem->GetItemDefinition()->GetLoadoutSlot( 0 ) == LOADOUT_POSITION_SECONDARY
		|| pTestItem->GetItemDefinition()->GetLoadoutSlot( 0 ) == LOADOUT_POSITION_MELEE
		|| pTestItem->GetItemDefinition()->GetLoadoutSlot( 0 ) == LOADOUT_POSITION_BUILDING
		|| pTestItem->GetItemDefinition()->GetLoadoutSlot( 0 ) == LOADOUT_POSITION_PDA
		|| pTestItem->GetItemDefinition()->GetLoadoutSlot( 0 ) == LOADOUT_POSITION_PDA2
	) {
		// Must be strange, genuine, vintage, haunted or paintkit (ie a marketable weapon)
		eQuality = pTestItem->GetQuality();
		if ( eQuality == AE_RARITY1 
			|| eQuality == AE_VINTAGE 
			|| eQuality == AE_HAUNTED
			|| eQuality == AE_COLLECTORS 
			|| eQuality == AE_PAINTKITWEAPON 
		) {
			return NULL;
		}

		// Weapons with rarity are allowed
		uint8 nRarity = pTestItem->GetItemDefinition()->GetRarity();
		if ( nRarity != k_unItemRarity_Any )
		{
			return NULL;
		}

		// Strange items.  Dont just check for strange quality, actually check for a strange attribute.
		// See if we've got any strange attributes.
		for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
		{
			if ( pTestItem->FindAttribute( GetKillEaterAttr_Score( i ) ) )
			{
				return NULL;
			}
		}
	}

	return "#TF_HalloweenOffering_Invalid";
}

const char* GetCraftCommonStatClockInvalidReason( const class IEconItemInterface *pTestItem, const class IEconItemInterface *pSourceItem )
{
	if ( !pTestItem )
	{
		return "#TF_CollectionCrafting_NoItem";
	}

	// Not allowed to be crafted?
	if ( !pTestItem->IsUsableInCrafting() )
	{
		return "#TF_CollectionCrafting_NotCraftable";
	}

	// No self mades or community items
	uint32 eQuality = pTestItem->GetQuality();
	if ( eQuality == AE_SELFMADE || eQuality == AE_COMMUNITY )
		return "#TF_CollectionCrafting_NoUnusual";

	// This is how we test for unusuals.  Don't let unusuals be crafted
	static CSchemaAttributeDefHandle pAttrDef_ParticleEffect( "attach particle effect" );
	if ( pTestItem->FindAttribute( pAttrDef_ParticleEffect ) )
		return "#TF_CollectionCrafting_NoUnusual";

	static CSchemaAttributeDefHandle pAttrDef_TauntUnusualAttr( "taunt attach particle index" );
	if ( pTestItem->FindAttribute( pAttrDef_TauntUnusualAttr ) )
		return "#TF_CollectionCrafting_NoUnusual";

	const CEconItemDefinition *pItemDef = pTestItem->GetItemDefinition();
	if ( pItemDef == NULL )
		return "#TF_CollectionCrafting_NoItem";

	if ( pTestItem->IsTemporaryItem() )
		return "#TF_CollectionCrafting_NoItem";

	// Strange items.  Dont just check for strange quality, actually check for a strange attribute.
	// See if we've got any strange attributes.
	for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
	{
		if ( pTestItem->FindAttribute( GetKillEaterAttr_Score( i ) ) )
		{
			return NULL;
		}
	}

	// Needs Rarity
	uint8 nRarity = pTestItem->GetRarity();
	if ( nRarity != k_unItemRarity_Any && nRarity > 1 ) // do not allow default nor common rarity
	{
		return NULL;
	}

	return "#TF_MannCoTrade_ItemInvalid";
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
enum { kMaxCardUpgradesPerItem = 2 };

int GetMaxCardUpgradesPerItem()
{
	return kMaxCardUpgradesPerItem;
}

const CEconItemAttributeDefinition *GetCardUpgradeForIndex( const IEconItemInterface *pItem, int i )
{
	Assert( pItem );
	Assert( i >= 0 );
	Assert( i < kMaxCardUpgradesPerItem );

	class CGetNthUserGeneratedAttributeIterator : public IEconItemUntypedAttributeIterator
	{
	public:
		CGetNthUserGeneratedAttributeIterator( int iTargetIndex )
			: m_iCount( iTargetIndex )
			, m_pAttrDef( NULL )
		{
		}

		virtual bool OnIterateAttributeValueUntyped( const CEconItemAttributeDefinition *pAttrDef ) OVERRIDE
		{
			if ( pAttrDef->GetUserGenerationType() != 0 && m_iCount-- == 0 )
			{
				m_pAttrDef = pAttrDef;
				return false;
			}

			return true;
		}

		const CEconItemAttributeDefinition *GetAttrDef() const { return m_pAttrDef; }

	private:
		int m_iCount;
		const CEconItemAttributeDefinition *m_pAttrDef;
	};

	CGetNthUserGeneratedAttributeIterator findNthAttrIterator( i );
	pItem->IterateAttributes( &findNthAttrIterator );

	return findNthAttrIterator.GetAttrDef();
}
