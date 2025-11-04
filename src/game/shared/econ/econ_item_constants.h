//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef ACTUAL_ECON_ITEM_CONSTANTS_H		// ECON_ITEM_CONSTANTS_H is used by src/common/econ_item_view.h
#define ACTUAL_ECON_ITEM_CONSTANTS_H
#ifdef _WIN32
#pragma once
#endif

//=============================================================================
// To avoid #include dependency chains, this file should
// contain only constants that do not depend on other 
// header files.
// This file is #included in cbase.h to allow schema compiles
// to use these constants to ensure correlation between 
// code data structures and database entries
//=============================================================================

typedef	uint32		item_price_t;						// this is the type that is used to hold currency values for transactions! don't change this without changing the relevant code/databases/etc.
typedef uint8		item_transaction_quantity_t;

class CLocalizationProvider;

enum { kLocalizedPriceSizeInChararacters = 64 };

//-----------------------------------------------------------------------------
// Econ Item testing
//-----------------------------------------------------------------------------
enum testitem_itemtypes_t
{
	TI_TYPE_UNKNOWN = -1,

	TI_TYPE_WEAPON = 0,
	TI_TYPE_HEADGEAR,
	TI_TYPE_MISC1,
	TI_TYPE_MISC2,

	TI_TYPE_COUNT,
};
#define TESTITEM_DEFINITIONS_BEGIN_AT		40000

//-----------------------------------------------------------------------------
// Type IDs for economy classes. These are part of the client-GC protocol and
// should not change if it can be helped
//-----------------------------------------------------------------------------
enum EEconTypeID
{
	k_EEconTypeItem							=1,
	k_EEconTypePlayerInfo					=2,
	k_EEconTypeClaimCode					=3,
	k_EEconTypeRecipe						=5,
	k_EEconTypeGameAccountClient			=7,
	k_EEconTypeGameAccount					=8,
	k_EEconTypeDuelSummary					=19,
	k_EEconTypeExperiment					=20,
	k_EEconTypeMapContribution				=28,
	k_EEconTypeGameServerAccount			=29,
	k_EEconTypeCoachRating					=30,
//	k_EEconTypeEquipInstance				=31,		// DEPRECATED
	k_EEconTypeSelectedItemPreset			=35,
	k_EEconTypeItemPresetInstance			=36,
	k_EEconTypeGameAccountForGameServers	=37,
	k_EEConTypeWarData						=38,
	k_EEConTypeLadderData					=39,
	k_EEConTypeMatchResultPlayerInfo		=40,
	k_EEconTypeXPSource						=41,
	k_EEconTypeNotification					=42,
	k_EEconTypeQuestMap						=43,
	k_EEconTypeQuestMapNode					=44,
	k_EEConTypeQuest						=45,
	k_EEconTypeQuestMapRewardPurchase		=46,
};

//-----------------------------------------------------------------------------
// Actions for the ItemAudit table
//-----------------------------------------------------------------------------
// WARNING!!! Values stored in DB. Do not renumber!
enum EItemAction
{
	k_EItemActionInvalid					 = -1,
	k_EItemActionGSCreate					 = 0,
	k_EItemActionUnpurchase					 = 1,
	k_EItemActionDelete						 = 2,
	k_EItemActionAwardAchievement			 = 3,
	k_EItemActionBanned						 = 4,
	k_EItemActionQuantityChanged			 = 5,
	k_EItemActionRestored					 = 6,
	k_EItemActionAwardTime					 = 7,
	k_EItemActionManualCreate				 = 8,
	k_EItemActionDrop						 = 9,
	k_EItemActionPickUp						 = 10,
	k_EItemActionCraftDestroy				 = 11,
	k_EItemActionCraftCreate				 = 12,
	k_EItemActionLimitExceeded				 = 13,
	k_EItemActionPurchase					 = 14,
	k_EItemActionNameChanged_Add			 = 15,
	k_EItemActionUnlockCrate_Add			 = 16,
	k_EItemActionPaintItem_Add				 = 17,
	k_EItemActionAutoGrantItem				 = 18,
	k_EItemActionCrossGameAchievement		 = 19,
	k_EItemActionAddItemToSocket_Add		 = 20,
	k_EItemActionAddSocketToItem_Add		 = 21,
	k_EItemActionRemoveSocketItem_Add		 = 22,
	k_EItemActionCustomizeItemTexture_Add	 = 23,
	k_EItemActionItemTraded_Add				 = 24,
	k_EItemActionUseItem					 = 25,
	k_EItemActionAwardGift_Receiver			 = 26,
	k_EItemActionNameChanged_Remove			 = 27,
	k_EItemActionUnlockCrate_Remove			 = 28,
	k_EItemActionPaintItem_Remove			 = 29,
	k_EItemActionAddItemToSocket_Remove		 = 30,
	k_EItemActionAddSocketToItem_Remove		 = 31,
	k_EItemActionRemoveSocketItem_Remove	 = 32,
	k_EItemActionCustomizeItemTexture_Remove = 33,
	k_EItemActionItemTraded_Remove			 = 34,
	k_EItemActionUnpackItemBundle			 = 35,
	k_EItemActionCreateItemFromBundle		 = 36,
	k_EItemActionAwardStorePromotionItem	 = 37,
	k_EItemActionConvertItem				 = 38,
	k_EItemActionEarnedItem					 = 39,
	k_EItemActionAwardGift_Giver			 = 40,
	k_EItemActionRefundedItem				 = 41,
	k_EItemActionAwardThirdPartyPromo		 = 42,
	k_EItemActionRemoveItemName_Remove		 = 43,
	k_EItemActionRemoveItemName_Add			 = 44,
	k_EItemActionRemoveItemPaint_Remove		 = 45,
	k_EItemActionRemoveItemPaint_Add		 = 46,
	k_EItemActionHalloweenDrop				 = 47,
	k_EItemActionSteamWorkshopContributor	 = 48,
	k_EItemActionManualOwnershipChange		 = 49,			// when we have bad bugs that corrupt item data and have to fix up rows in the DB by hand
	k_EItemActionSupportDelete				 = 50,
	k_EItemActionSupportCreatedByUndo		 = 51,
	k_EItemActionSupportDeletedByUndo		 = 52,
	k_EItemActionSupportQuantityChangedByUndo = 53,
	k_EItemActionSupportRename_Add			 = 54,
	k_EItemActionSupportRename_Remove		 = 55,
	k_EItemActionSupportDescribe_Add		 = 56,
	k_EItemActionSupportDescribe_Remove		 = 57,

	k_EItemActionStrangePartApply_Add		 = 58,
	k_EItemActionStrangePartApply_Remove	 = 59,
	k_EItemActionStrangeScoreReset_Add		 = 60,
	k_EItemActionStrangeScoreReset_Remove	 = 61,
	k_EItemActionStrangePartRemove_Add		 = 62,
	k_EItemActionStrangePartRemove_Remove	 = 63,

	k_EItemActionSupportStrangify_Add		 = 64,
	k_EItemActionSupportStrangify_Remove	 = 65,

	k_EItemActionUpgradeCardApply_Add		 = 66,
	k_EItemActionUpgradeCardApply_Remove	 = 67,
	k_EItemActionUpgradeCardRemove_Add		 = 68,
	k_EItemActionUpgradeCardRemove_Remove	 = 69,

	k_EItemActionStrangeRestrictionApply_Add	= 70,
	k_EItemActionStrangeRestrictionApply_Remove	= 71,
	k_EItemActionTransmogrify_Add				= 72,
	k_EItemActionTransmogrify_Remove			= 73,
	k_EItemActionHalloweenSpellPageAdd_Add		= 74,
	k_EItemActionHalloweenSpellPageAdd_Remove	= 75,

	k_EItemActionDev_ClientLootListRoll		 = 90,

	k_EItemActionGiftWrap_Add				 = 100,
	k_EItemActionGiftWrap_Remove			 = 101,
	k_EItemActionGiftDelivery_Add			 = 102,
	k_EItemActionGiftDelivery_Remove		 = 103,
	k_EItemActionGiftUnwrap_Add				 = 104,
	k_EItemActionGiftUnwrap_Remove			 = 105,
	k_EItemActionPackageItem				 = 106,
	k_EItemActionPackageItem_Revoked		 = 107,
	k_EItemActionHandleMapToken			 	 = 108,
	k_EItemActionCafeOrSchoolItem_Remove	 = 109,
	k_EItemActionVACBanned_Remove			 = 110,
	k_EItemActionUpgradeThirdPartyPromo		 = 111,
	k_EItemActionExpired					 = 112,
	k_EItemActionTradeRollback_Add			 = 113,
	k_EItemActionTradeRollback_Remove		 = 114,
	k_EItemActionCDKeyGrant					 = 115,
	k_EItemActionCDKeyRevoke				 = 116,
	k_EItemActionWeddingRing_Add			 = 117,
	k_EItemActionWeddingRing_Remove			 = 118,
	k_EItemActionWeddingRing_AddPartner		 = 119,
	k_EItemActionEconSetUnowned				 = 120,
	k_EItemActionEconSetOwned				 = 121,
	k_EItemActionStrangifyItem_Add			 = 122,
	k_EItemActionStrangifyItem_Remove		 = 123,
	k_EItemActionConsumeItem_Consume_ToolRemove		= 124,
	k_EItemActionConsumeItem_Consume_ToolAdd		= 125,
	k_EItemActionConsumeItem_Consume_InputRemove	= 126,
	k_EItemActionConsumeItem_Complete_OutputAdd		= 127,	
	k_EItemActionConsumeItem_Complete_ToolRemove	= 128,
	k_EItemActionItemEaterRecharge_Add			 = 129,
	k_EItemActionItemEaterRecharge_Remove		 = 130,

	k_EItemActionRemoveItemCraftIndex_Remove = 150,
	k_EItemActionRemoveItemCraftIndex_Add	 = 151,
	k_EItemActionRemoveItemMakersMark_Remove = 152,				// early versions of this will be in the database as 150
	k_EItemActionRemoveItemMakersMark_Add	 = 153,				// early versions of this will be in the database as 151 because I am a terrible person

	// Never used on public in TF
	k_EItemActionCollectItem_CollectedItem			 = 154,
	k_EItemActionCollectItem_UpdateCollection		 = 155,
	k_EItemActionCollectItem_RemoveCollection		 = 156,
	k_EItemActionCollectItem_RedeemCollectionReward	 = 157,

	k_EItemActionPreviewItem_BeginPreviewPeriod  = 158,
	k_EItemActionPreviewItem_EndPreviewPeriodExpired	= 159,
	k_EItemActionPreviewItem_EndPreviewPeriodItemBought	= 160,

	k_EItemActionPeriodicScoreReward_Add	 = 170,
	k_EItemActionPeriodicScoreReward_Remove	 = 171,

	k_EItemActionMvM_ChallengeCompleted_RemoveTicket				= 180,			// we completed a challenge and consumed this ticket as the cost
	k_EItemActionMvM_ChallengeCompleted_GrantBadge					= 181,			// we completed a challenge and granted the player a badge because they didn't have one
	k_EItemActionMvM_ChallengeCompleted_UpdateBadgeStamps_Remove	= 182,			// we completed a challenge and we're crossing an entry off our badge checklist (this may also reset the badge back down to empty if this was the last line item)
	k_EItemActionMvM_ChallengeCompleted_UpdateBadgeStamps_Add		= 183,			// (other half of the above)
	k_EItemActionMvM_ChallengeCompleted_GrantMissionCompletionLoot	= 184,			// we completed a mission in MvM
	k_EItemActionMvM_RemoveSquadSurplusVoucher						= 185,
	k_EItemActionMvM_AwardSquadSurplus_Receiver						= 186,
	k_EItemActionMvM_AwardSquadSurplus_Giver						= 187,
	k_EItemActionMvM_ChallengeCompleted_GrantTourCompletionLoot		= 188,			// we completed a full tour in MvM
	k_EItemActionMvM_AwardHelpANoobBonus_Helper						= 189,

	k_EItemActionHalloween_UpdateMerasmusLootLevel_Add				= 200,			// set the level of the merasmus loot
	k_EItemActionHalloween_UpdateMerasmusLootLevel_Remove			= 201,			

	k_EItemActionRemoveItemKillStreak_Remove = 202,
	k_EItemActionRemoveItemKillStreak_Add	 = 203,

	k_EItemActionSupportAddOrModifyAttribute_Remove = 204,
	k_EItemActionSupportAddOrModifyAttribute_Add	= 205,

	k_EItemActionSpyVsEngyWar_JoinedWar	= 206,	// Never used on public

	k_EItemAction_UpdateDuckBadgeLevel_Add			= 207,
	k_EItemAction_UpdateDuckBadgeLevel_Remove		= 208,

	k_EItemAction_QuestDrop							= 209,

	k_EItemAction_OperationPass_Add					= 210, // Never used on public in TF

	k_EItemActionMarket_Add							= 211,
	k_EItemActionMarket_Remove						= 212,

	k_EItemAction_QuestComplete_Reward				= 213,
	k_EItemAction_QuestComplete_Remove				= 214,

	k_EItemAction_QuestLoaner_Add					= 215,
	k_EItemActionStrangeCountTransfer_Add			= 216,
	k_EItemActionStrangeCountTransfer_Remove		= 217,

	k_EItemActionCraftCollectionUpgrade_Add			= 218,
	k_EItemActionCraftCollectionUpgrade_Remove		= 219,

	k_EItemActionCraftHalloweenOffering_Add			= 220,
	k_EItemActionCraftHalloweenOffering_Remove		= 221,

	k_EItemActionRemoveItemGiftedBy_Remove			= 222,
	k_EItemActionRemoveItemGiftedBy_Add				= 223,

	k_EItemActionAddParticleVerticalAttr_Remove		= 224,
	k_EItemActionAddParticleVerticalAttr_Add		= 225,

	k_EItemActionAddParticleUseHeadOriginAttr_Remove = 226,
	k_EItemActionAddParticleUseHeadOriginAttr_Add	= 227,

	k_EItemActionRemoveItemDynamicAttr_Add			= 228,
	k_EItemActionRemoveItemDynamicAttr_Remove		= 229,

	k_EItemActionCraftStatClockTradeUp_Add			= 230,
	k_EItemActionCraftStatClockTradeUp_Remove		= 231,

	k_EItemActionViralCompetitiveBetaPass_Drop		= 232,

	k_EItemActionSupportDeleteAttribute_Remove		= 233,
	k_EItemActionSupportDeleteAttribute_Add			= 234,

	k_EItemActionCYOABloodMoneyPurchase				= 235,

	k_EItemActionPaintKitConsume_Remove				= 236,
	k_EItemActionPaintKitConsume_Add				= 237,
	k_EItemActionDeletedAccountTerminated			= 238,

	// The "OneOffRefund" asset api, for setting up specific/manual refund offers that may also restore related items to
	// the user's inventory (e.g. the key/crate used to obtain something)
	k_EItemActionOneOffRefund_Add					= 239,
	k_EItemActionOneOffRefund_Remove				= 240,

	// Adding a new action?
	// Be sure to:
	//  tf_english.txt:
	//   - Add ItemHistory_Action strings (shown to users)
	//  econ_item.cpp:
	//   - Add an ITEM_ACTION to ENUMSTRINGS_START( EItemAction )
	//   - Update PchFriendlyNameFromEItemAction (shown to support)
	//  econ_assetapi_context.cpp:
	//   - Update CEconContextBackpack::YldUserHistory_GetFriendlyUserHistory, if necessary
	//   - Update BIsActionConnected
	//   - Update BIsActionCreative
	//   - Update BIsActionDestructive
	//   - Update BIsActionQuantityChange
	//   - Update BIsActionDestructiveEscrow

	// Let's be consistent with the underscores please.
	// k_EItemActionYourNewAction, not k_EItemAction_YourNewAction
	// Yes, it matters. See PchLocalizedNameFromEItemAction for why.

	k_EItemActionMax
};

// These are stored in the database as uint8
COMPILE_TIME_ASSERT( k_EItemActionMax < 256 );

extern const char		*PchNameFromEItemAction( EItemAction eAction );
extern const char		*PchNameFromEItemActionUnsafe( EItemAction eAction );

extern bool BIsActionCreative( EItemAction );
extern bool BIsActionDestructive( EItemAction );
extern bool BIsActionDestructiveEscrow( EItemAction eAction );

enum EItemActionMissingBehavior { kEItemAction_FriendlyNameLookup_ReturnNULLIfMissing, kEItemAction_FriendlyNameLookup_ReturnDummyStringIfMissing };
extern const char		*PchFriendlyNameFromEItemAction( EItemAction eAction, EItemActionMissingBehavior eMissingBehavior );
extern const char		*PchLocalizedNameFromEItemAction( EItemAction eAction, CLocalizationProvider &localizationProvider );

//-----------------------------------------------------------------------------
// Purpose: Used to pass audit actions to asset servers for SetUnowned and 
//			SetOwned methods. 
//-----------------------------------------------------------------------------
enum EEconOwnershipAction
{
	k_EEconOwnershipAction_Invalid = 0,

	k_EEconOwnershipAction_TradeBase = 100,
	k_EEconOwnershipAction_TradeCommit = 101,		// precommit and docommit step of a trade. Reference is trade ID
	k_EEconOwnershipAction_TradeRollback = 102,		// cancelcommit and rollbackcommit step of a trade. Reference is trade ID
};

// old
enum eEconItemFlags_Deprecated
{
	kDeprecated_EconItemFlag_AchievementGrantedItem	= 1 << 0,
	kDeprecated_EconItemFlag_CannotTrade				= 1 << 1,
	kDeprecated_EconItemFlag_Purchased				= 1 << 2,
	kDeprecated_EconItemFlag_CannotBeUsedInCrafting	= 1 << 3,
	kDeprecated_EconItemFlag_Promotion				= 1 << 4,
};

//-----------------------------------------------------------------------------
// Periodic score events
//-----------------------------------------------------------------------------
enum eEconPeriodicScoreEvents
{
	kPeriodicScoreEvent_GiftsDistributed	= 0,
	kPeriodicScoreEvent_DuelsWon			= 1,
	kPeriodicScoreEvent_MapStampsPurchased	= 2,
};

//-----------------------------------------------------------------------------
// Flags for CEconItem
//-----------------------------------------------------------------------------
// WARNING!!! Values stored in DB.  DO NOT CHANGE EXISTING VALUES.  Add values to the end.
enum eEconItemFlags
{
	kEconItemFlag_CannotTrade									= 1 << 0,
	kEconItemFlag_CannotBeUsedInCrafting						= 1 << 1,
	kEconItemFlag_CanBeTradedByFreeAccounts						= 1 << 2,
	kEconItemFlag_NonEconomy									= 1 << 3,		// used for items that are meant to not interact in the economy -- these can't be traded, gift-wrapped, crafted, etc.
	kEconItemFlag_PurchasedAfterStoreCraftabilityChanges2012	= 1 << 4,		// cosmetic items coming from the store are now usable in crafting; this flag is set on all items purchased from the store after this change was made

#ifdef CLIENT_DLL
#ifdef TF_CLIENT_DLL
	kEconItemFlagClient_ForceBlueTeam							= 1 << 5,
#endif // TF_CLIENT_DLL
	kEconItemFlagClient_StoreItem								= 1 << 6,
	kEconItemFlagClient_Preview									= 1 << 7,		// only set on the client; means "this item is being previewed"
#endif // CLIENT_DLL

	// combination of the above flags used in code
	kEconItemFlags_CheckFlags_AllGCFlags						= kEconItemFlag_CannotTrade | kEconItemFlag_CannotBeUsedInCrafting | kEconItemFlag_CanBeTradedByFreeAccounts | kEconItemFlag_NonEconomy | kEconItemFlag_PurchasedAfterStoreCraftabilityChanges2012,
};

//-----------------------------------------------------------------------------
// Origin for an item for CEconItem
//-----------------------------------------------------------------------------
// WARNING!!! Values stored in DB.  DO NOT CHANGE EXISTING VALUES.  Add values to the end.
enum eEconItemOrigin
{
	kEconItemOrigin_Invalid = -1,				// should never be stored in the DB! used to indicate "invalid" for in-memory objects only

	kEconItemOrigin_Drop = 0,
	kEconItemOrigin_Achievement,
	kEconItemOrigin_Purchased,
	kEconItemOrigin_Traded,
	kEconItemOrigin_Crafted,
	kEconItemOrigin_StorePromotion,
	kEconItemOrigin_Gifted,
	kEconItemOrigin_SupportGranted,
	kEconItemOrigin_FoundInCrate,
	kEconItemOrigin_Earned,
	kEconItemOrigin_ThirdPartyPromotion,
	kEconItemOrigin_GiftWrapped,
	kEconItemOrigin_HalloweenDrop,
	kEconItemOrigin_PackageItem,
	kEconItemOrigin_Foreign,
	kEconItemOrigin_CDKey,
	kEconItemOrigin_CollectionReward,
	kEconItemOrigin_PreviewItem,
	kEconItemOrigin_SteamWorkshopContribution,
	kEconItemOrigin_PeriodicScoreReward,
	kEconItemOrigin_MvMMissionCompletionReward,			// includes loot from both "mission completed" and "tour completed" events
	kEconItemOrigin_MvMSquadSurplusReward,
	kEconItemOrigin_RecipeOutput,
	kEconItemOrigin_QuestDrop,
	kEconItemOrigin_QuestLoanerItem,
	kEconItemOrigin_TradeUp,
	kEconItemOrigin_ViralCompetitiveBetaPassSpread,
	kEconItemOrigin_CYOABloodMoneyPurchase,
	kEconItemOrigin_Paintkit,
	kEconItemOrigin_UntradableFreeContractReward,

	kEconItemOrigin_Max,
};
extern const char		*PchNameFromeEconItemOrigin( eEconItemOrigin eOrigin );

// The Steam backend representation of a unique item index
typedef uint64	itemid_t;
typedef uint16	item_definition_index_t;
typedef uint16	attrib_definition_index_t;
typedef uint32	attrib_value_t;
typedef uint32	operation_definition_index_t;
typedef uint8	war_definition_index_t;
typedef uint8	war_side_t;
typedef uint32	ObjectiveConditionDefIndex_t;

// Misc typedefs for clarity.
typedef uint32	equip_region_mask_t;
typedef uint8	style_index_t;

const uint64 INVALID_ITEM_ID							= (itemid_t)-1;
const item_definition_index_t INVALID_ITEM_DEF_INDEX	= ((item_definition_index_t)-1);
const attrib_definition_index_t INVALID_ATTRIB_DEF_INDEX= ((attrib_definition_index_t)-1);

const war_definition_index_t INVALID_WAR_DEF_INDEX		= ((war_definition_index_t)-1);
const war_side_t INVALID_WAR_SIDE						= ((war_side_t)-1);
// Hard code the pyro/heavy stuff. Must be in sync with the schema.
const war_definition_index_t PYRO_VS_HEAVY_WAR_DEF_INDEX= ((war_definition_index_t)0);
const war_side_t PYRO_VS_HEAVY_WAR_SIDE_HEAVY = ((war_side_t)0);
const war_side_t PYRO_VS_HEAVY_WAR_SIDE_PYRO = ((war_side_t)1);

const ObjectiveConditionDefIndex_t INVALID_QUEST_OBJECTIVE_CONDITIONS_INDEX = ObjectiveConditionDefIndex_t(-1);

typedef CUtlMap< uint32, const class CQuestDefinition* > QuestDefMap_t;

//-----------------------------------------------------------------------------

// Standard/default backpack size
#define DEFAULT_NUM_BACKPACK_SLOTS						300
#define DEFAULT_NUM_BACKPACK_SLOTS_FREE_TRIAL_ACCOUNT	50
#define MAX_NUM_BACKPACK_SLOTS							4000

// Current item level range
#define MIN_ITEM_LEVEL					0
#define MAX_ITEM_LEVEL					100

// Maximum number of attributes allowed on a single item
#define MAX_ATTRIBUTES_PER_ITEM					20
// The maximum length of a single attribute's description
//	divide by locchar_t, so we can ensure 192 bytes, whether that's 128 wchars on client or 256 utf-8 bytes on gc
#define MAX_ATTRIBUTE_DESCRIPTION_LENGTH		( 256 / sizeof( locchar_t ) )

// The maximum length of an item's name
#define MAX_ITEM_NAME_LENGTH					128
#define MAX_ITEM_DESC_LENGTH					256
// The maximum length of an item description. (Extra +1 line is for the base item type line)
#define MAX_ITEM_DESCRIPTION_LENGTH				((MAX_ATTRIBUTES_PER_ITEM+1) * MAX_ATTRIBUTE_DESCRIPTION_LENGTH)

// For custom user-naming of econ items.
#define MAX_ITEM_CUSTOM_NAME_LENGTH				40
#define MAX_ITEM_CUSTOM_NAME_DATABASE_SIZE		((4 * MAX_ITEM_CUSTOM_NAME_LENGTH) + 1)	// Ensures we can store MAX_ITEM_CUSTOM_NAME_LENGTH
																						// characters worth of obscure unicode characters in UTF8
#define MAX_ITEM_CUSTOM_DESC_LENGTH				80
#define MAX_ITEM_CUSTOM_DESC_DATABASE_SIZE		((4 * MAX_ITEM_CUSTOM_DESC_LENGTH) + 1)

#define MAX_KILLCAM_MESSAGE_LENGTH				40
#define MAX_KILLCAM_MESSAGE_DATABASE_SIZE		((4 * MAX_KILLCAM_MESSAGE_LENGTH) + 1)

// max length in the DB for claim codes
#define MAX_CLAIM_CODE_LENGTH					128	

// The item definition index reserved for the preview item
#define PREVIEW_ITEM_DEFINITION_INDEX			(item_definition_index_t)-1

// The number of items to work on in a job before checking if a yield is necessary
#define MAX_ITEMS_BEFORE_YIELD					50

// TF team-color paints (moved from econ_item_view.h)
#define RGB_INT_RED  12073019
#define RGB_INT_BLUE 5801378

// Custom textures
const int k_nCustomImageSize = 128;
const int k_nMaxCustomImageFileSize = k_nCustomImageSize*k_nCustomImageSize*4 + 4*1024; // Is this about right?

//-----------------------------------------------------------------------------
// Purpose: colors used in the display of attributes
//-----------------------------------------------------------------------------
enum attrib_colors_t
{
	ATTRIB_COL_LEVEL = 0,
	ATTRIB_COL_NEUTRAL,
	ATTRIB_COL_POSITIVE,
	ATTRIB_COL_NEGATIVE,

	NUM_ATTRIB_COLORS,
};


#define AE_USE_SCRIPT_VALUE			9999		// Can't be -1, due to unsigned ints used on the backend

extern attrib_colors_t GetAttribColorIndexForName( const char* pszName );
extern const char *GetColorNameForAttribColor( attrib_colors_t unAttribColor );
extern const char *GetHexColorForAttribColor( attrib_colors_t unAttribColor );

enum recipecategories_t
{
	RECIPE_CATEGORY_CRAFTINGITEMS = 0,
	RECIPE_CATEGORY_COMMONITEMS,
	RECIPE_CATEGORY_RAREITEMS,
	RECIPE_CATEGORY_SPECIAL,

	NUM_RECIPE_CATEGORIES
};
extern const char *g_szRecipeCategoryStrings[NUM_RECIPE_CATEGORIES];

int GetMaxCardUpgradesPerItem();
const class CEconItemAttributeDefinition *GetCardUpgradeForIndex( const class IEconItemInterface *pItem, int i );

#define GUARANTEED_OUTPUT	(1<<0)
#define GUARANTEED_INPUT	(1<<1)

//-----------------------------------------------------------------------------
// List of holidays. These are sorted by priority. Needs to match static IIsHolidayActive *s_HolidayChecks
//-----------------------------------------------------------------------------
enum EHoliday
{
	kHoliday_None							= 0,		// must stay at zero for backwards compatibility
	kHoliday_TFBirthday,
	kHoliday_Halloween,
	kHoliday_Christmas,
	kHoliday_CommunityUpdate,
	kHoliday_EOTL,
	kHoliday_Valentines,
	kHoliday_MeetThePyro,
	kHoliday_FullMoon,
	kHoliday_HalloweenOrFullMoon,
	kHoliday_HalloweenOrFullMoonOrValentines,
	kHoliday_AprilFools,
	kHoliday_Soldier,
	kHoliday_Summer,
	kHolidayCount,
};

typedef int		econ_tag_handle_t;

#define INVALID_ECON_TAG_HANDLE	((econ_tag_handle_t)-1)

#endif // ACTUAL_ECON_ITEM_CONSTANTS_H
