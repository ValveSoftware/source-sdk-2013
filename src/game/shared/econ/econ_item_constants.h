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
// Purpose: Quality types of items
//-----------------------------------------------------------------------------
typedef int32 entityquality_t;
enum EEconItemQuality
{
	AE_UNDEFINED = -1,

	AE_NORMAL           = 0,
	AE_RARITY1          = 1,  // Genuine
	AE_RARITY2          = 2,  // Customized (unused)
	AE_VINTAGE          = 3,  // Vintage has to stay at 3 for backwards compatibility
	AE_RARITY3          = 4,  // Artisan
	AE_UNUSUAL          = 5,  // Unusual
	AE_UNIQUE           = 6,
	AE_COMMUNITY        = 7,
	AE_DEVELOPER        = 8,
	AE_SELFMADE         = 9,
	AE_CUSTOMIZED       = 10, // (unused)
	AE_STRANGE          = 11,
	AE_COMPLETED        = 12,
	AE_HAUNTED          = 13,
	AE_COLLECTORS       = 14,
	AE_PAINTKITWEAPON   = 15,

	AE_RARITY_DEFAULT   = 16,
	AE_RARITY_COMMON    = 17,
	AE_RARITY_UNCOMMON  = 18,
	AE_RARITY_RARE      = 19,
	AE_RARITY_MYTHICAL  = 20,
	AE_RARITY_LEGENDARY = 21,
	AE_RARITY_ANCIENT   = 22,

	AE_MAX_TYPES,
	AE_DEPRECATED_UNIQUE = 3,
};


//-----------------------------------------------------------------------------
// Purpose: colors used in the display of attributes
//-----------------------------------------------------------------------------
enum attrib_colors_t
{
	ATTRIB_COL_LEVEL = 0,
	ATTRIB_COL_NEUTRAL,
	ATTRIB_COL_POSITIVE,
	ATTRIB_COL_NEGATIVE,
	ATTRIB_COL_ITEMSET_NAME,
	ATTRIB_COL_ITEMSET_EQUIPPED,
	ATTRIB_COL_ITEMSET_MISSING,
	ATTRIB_COL_BUNDLE_ITEM,
	ATTRIB_COL_LIMITED_USE,
	ATTRIB_COL_component_flags,
	ATTRIB_COL_LIMITED_QUANTITY,

	ATTRIB_COL_RARITY_DEFAULT,
	ATTRIB_COL_RARITY_COMMON,
	ATTRIB_COL_RARITY_UNCOMMON,
	ATTRIB_COL_RARITY_RARE,
	ATTRIB_COL_RARITY_MYTHICAL,
	ATTRIB_COL_RARITY_LEGENDARY,
	ATTRIB_COL_RARITY_ANCIENT,
	ATTRIB_COL_RARITY_IMMORTAL,
	ATTRIB_COL_RARITY_ARCANA,

	ATTRIB_COL_STRANGE,
	ATTRIB_COL_UNUSUAL,

	NUM_ATTRIB_COLORS,
};


#define AE_USE_SCRIPT_VALUE			9999		// Can't be -1, due to unsigned ints used on the backend

const char *EconQuality_GetQualityString( EEconItemQuality eQuality );
const char *EconQuality_GetColorString( EEconItemQuality eQuality );
const char *EconQuality_GetLocalizationString( EEconItemQuality eQuality );
EEconItemQuality EconQuality_GetQualityFromString( const char* pszQuality );

// Sort order for rarities
int EconQuality_GetRarityScore( EEconItemQuality eQuality );

extern attrib_colors_t GetAttribColorIndexForName( const char* pszName );
extern const char *GetColorNameForAttribColor( attrib_colors_t unAttribColor );
extern const char *GetHexColorForAttribColor( attrib_colors_t unAttribColor );

// Utility function that'll get you an item quality from a string
entityquality_t GetItemQualityFromString( const char *sQuality );

enum recipecategories_t
{
	RECIPE_CATEGORY_CRAFTINGITEMS = 0,
	RECIPE_CATEGORY_COMMONITEMS,
	RECIPE_CATEGORY_RAREITEMS,
	RECIPE_CATEGORY_SPECIAL,

	NUM_RECIPE_CATEGORIES
};
extern const char *g_szRecipeCategoryStrings[NUM_RECIPE_CATEGORIES];

//-----------------------------------------------------------------------------
// Kill eater support.
// Strange counters and strange parts
//-----------------------------------------------------------------------------
#if defined( TF_DLL ) || defined( TF_GC_DLL ) || defined( TF_CLIENT_DLL )
enum kill_eater_event_t
{
	kKillEaterEvent_PlayerKill = 0,			// default; items with no event type specified use this
	kKillEaterEvent_UberActivated,
	kKillEaterEvent_PlayerKillAssist,
	kKillEaterEvent_PlayerKillsBySentry,	// your sentry you built with this item killed someone
	kKillEaterEvent_PeeVictims,				// this game is great
	kKillEaterEvent_BackstabAbsorbed,		// you're a sniper and you got a spy to stab your Razorback
	kKillEaterEvent_HeadsTaken,				// this also tracks kills but with different flavor text
	kKillEaterEvent_Humiliations,			// fish kills!
	kKillEaterEvent_GiftsGiven,				// number of gifts given
	kKillEaterEvent_DeathsFeigned,			// number of deaths successfully feigned with the Dead Ringer
	kKillEaterEvent_ScoutKill,		// = 10 // (part)
	kKillEaterEvent_SniperKill,				// (part)
	kKillEaterEvent_SoldierKill,			// (part)
	kKillEaterEvent_DemomanKill,			// (part)
	kKillEaterEvent_HeavyKill,				// (part)
	kKillEaterEvent_PyroKill,				// (part)
	kKillEaterEvent_SpyKill,				// (part)
	kKillEaterEvent_EngineerKill,			// (part)
	kKillEaterEvent_MedicKill,				// (part)
	kKillEaterEvent_BuildingDestroyed,		// (part)
	kKillEaterEvent_ProjectileReflect,	// = 20	// (part)
	kKillEaterEvent_HeadshotKill,			// (part)
	kKillEaterEvent_AirborneEnemyKill,		// (part) (enemy is in the air when they die)
	kKillEaterEvent_GibKill,				// (part)
	kKillEaterEvent_BuildingSapped,			// a sapper was doing damage to this building while it was destroyed
	kKillEaterEvent_PlayerTickle,			// we used our comedy holiday gloves to force someone else to laugh
	kKillEaterEvent_PlayerKillByBootStomp,	// we killed a player by transferring our falling damage onto them
	kKillEaterEvent_PlayerKillDuringFullMoon,				// (part) we killed a player during the full moon holiday event (GC-updated)
	kKillEaterEvent_PlayerKillStartDomination,				// (part) we killed a player and this kill was enough to start our domination of them
	kKillEaterEvent_PlayerKillAlreadyDominated,				// (part) we killed a player with this weapon that we were already dominating
	kKillEaterEvent_PlayerKillRevenge,			// = 30		// (part) we killed a player with this weapon when that player was dominating us
	kKillEaterEvent_PlayerKillPosthumous,					// (part) we killed a player after we were already dead (afterburn, stray rocket, etc.)
	kKillEaterEvent_BurningAllyExtinguished,				// (part) we used urine/milk/flamethrower/whatever to put out the fire on an ally that was burning
	kKillEaterEvent_PlayerKillCritical,						// (part) we killed a player with a shot that was a critical
	kKillEaterEvent_PlayerKillWhileExplosiveJumping,		// (part) we killed a player while we were rocket/sticky-jumping
	kKillEaterEvent_PlayerKillFriend,						// (part) we killed a player who is a Steam friend (GC-updated)
	kKillEaterEvent_SapperDestroyed,						// (part) we destroyed a sapper that was on a friendly building
	kKillEaterEvent_InvisibleSpiesKilled,					// (part) we killed an invisible spy
	kKillEaterEvent_MedicsWithFullUberKilled,				// (part) we killed a fully ubered medic
	kKillEaterEvent_RobotsDestroyed,						// (part) we killed a robot in MvM
	kKillEaterEvent_MinibossRobotsDestroyed,	// = 40		// (part) we killed a miniboss robot in MvM
	kKillEaterEvent_RobotsDestroyedAfterPenetration,		// (part) we killed a robot with a shot that had already penetrated another robot
	kKillEaterEvent_RobotHeadshotKills,						// (part) like kKillEaterEvent_HeadshotKill, but only for robots
	kKillEaterEvent_RobotsSlowed,							// (part) we hit some robots with Jarate and now they're slow
	kKillEaterEvent_KillWhileLowHealth,						// (part) we killed someone while we had <10% max health
	kKillEaterEvent_HalloweenKill,							// (part) we killed someone during the Halloween holiday
	kKillEaterEvent_HalloweenKillRobot,						// (part) we killed a robot in MvM during the Halloween holiday
	kKillEaterEvent_DefenderKill,							// (part) we killed someone carrying the intel, pushing the cart, or capping a point
	kKillEaterEvent_UnderwaterKill,							// (part) we killed someone who was completely submerged
	kKillEaterEvent_KillWhileUbercharged,					// (part) we killed someone while we were invulnerable
	kKillEaterEvent_FoodEaten,					// = 50		// We ate our food
	kKillEaterEvent_BannersDeployed,						// We deployed a banner buff
	kKillEaterEvent_NEGATIVE_SniperShotsMissed,				// (part) we shot our sniper rifle and didnt hit anything
	kKillEaterEvent_NEGATIVE_UbersDropped,					// (part) we died with a full ubercharge
	kKillEaterEvent_NEGATIVE_DeathsWhileCarryingBuilding,	// (part) we died while carrying a building
	kKillEaterEvent_NEGATIVE_DeathsFromCratering,			// (part) we died from cratering
	kKillEaterEvent_NEGATIVE_DeathsFromEnvironment,			// (part) we died from environmental damage
	kKillEaterEvent_NEGATIVE_Deaths,						// (part) we died :(
	kKillEaterEvent_TimeCloaked,							// Time we are cloaked
	kKillEaterEvent_HealingProvided,						// Health Provided to Allies
	kKillEaterEvent_TeleportsProvided,			// = 60		// Teleports Provided to Allies
	kKillEaterEvent_TanksDestroyed,							// (part) we dealt the killing blow to a tank in MvM
	kKillEaterEvent_LongDistanceKill,						// (part) we dealt the killing blow (while alive) from far away
	kKillEaterEvent_UniqueEvent__KilledAccountWithItem,					// (part) (unique event) how many individual accounts have we killed?
	kKillEaterEvent_PointsScored,						// How many score points we've accumulated
	kKillEaterEvent_DoubleDonks,						// Double-Donks scored with the loose cannon
	kKillEaterEvent_TeammatesWhipped,					// Whipped Teammates with the Disciplinary Action
	kKillEaterEvent_VictoryTimeKill,					// Kills while in Victory / Bonus Time
	kKillEaterEvent_RobotScoutKill,				// (part)
	kKillEaterEvent_RobotSniperKill,			// (part) Not yet shipped
	kKillEaterEvent_RobotSoldierKill,			// = 70 // (part) Not yet shipped
	kKillEaterEvent_RobotDemomanKill,			// (part) Not yet shipped
	kKillEaterEvent_RobotHeavyKill,				// (part) Not yet shipped
	kKillEaterEvent_RobotPyroKill,				// (part) Not yet shipped
	kKillEaterEvent_RobotSpyKill,				// (part)
	kKillEaterEvent_RobotEngineerKill,			// (part) Not yet shipped
	kKillEaterEvent_RobotMedicKill,				// (part) Not yet shipped
	kKillEaterEvent_TauntKill,					// Taunt Kills
	kKillEaterEvent_PlayersWearingUnusualKill,	// (part) we killed someone wearing an unusual hat (!)
	kKillEaterEvent_BurningEnemyKill,			// (part) we killed someone who was on fire up until they died
	kKillEaterEvent_KillstreaksEnded,			// = 80 // (part) we killed someone who was on a killstreak
	kKillEaterEvent_KillcamTaunts,				// (cosmetic part) we appeared wearing this item in the killcam taunting
	kKillEaterEvent_DamageDealt,				// (part) we have dealt this much damage to people
	kKillEaterEvent_FiresSurvived,				// (cosmetic part) we were lit on fire wearing this item and then the fire went out and we were still alive
	kKillEaterEvent_AllyHealingDone,			// (part) we have healed this much (directly, so doesn't count Mad Milk, etc. because we lose the item pointer at some point); also ignores self heal (ie., Concheror buff, MvM upgrades)
	kKillEaterEvent_PointBlankKills,			// (part) we killed someone while standing right next to them
	kKillEaterEvent_PlayerKillsByManualControlOfSentry,	// Kills from wrangled a sentry
	kKillEaterEvent_CosmeticKills,				// (cosmetic part) kills
	kKillEaterEvent_FullHealthKills,			// (part) Kills while at fullhealth
	kKillEaterEvent_TauntingPlayerKills,		// (part) Taunting Player Kills
	kKillEaterEvent_Halloween_OverworldKills, // = 90
	kKillEaterEvent_Halloween_UnderworldKills,
	kKillEaterEvent_Halloween_MinigamesWon,
	kKillEaterEvent_NonCritKills,				// part kills that are not crit or mini crit
	kKillEaterEvent_PlayersHit,					// part
	kKillEaterEvent_CosmeticAssists,			// Cosmetic part
	kKillEaterEvent_CosmeticOperationContractsCompleted, // Operation Stat Tracker
	kKillEaterEvent_CosmeticOperationKills, // Operation Stat Tracker
	kKillEaterEvent_CosmeticOperationContractsPoints,
	kKillEaterEvent_CosmeticOperationBonusPoints,
	kKillEaterEvent_TauntsPerformed, // = 100	// Strange Taunts
	kKillEaterEvent_InvasionKills,				// Kills During Invasion Event.  Locked after Operation
	kKillEaterEvent_InvasionKillsOnMap01,
	kKillEaterEvent_InvasionKillsOnMap02,
	kKillEaterEvent_InvasionKillsOnMap03,
	kKillEaterEvent_InvasionKillsOnMap04,
	kKillEaterEvent_HalloweenSouls,				// Halloween
	kKillEaterEvent_HalloweenContractsCompleted,
	kKillEaterEvent_HalloweenOfferings,
	kKillEaterEvent_PowerupBottlesUsed,
	kKillEaterEvent_ContractPointsEarned, // = 110
	kKillEaterEvent_ContractPointsContributedToFriends,

	// NEW ENTRIES MUST BE ADDED AT THE BOTTOM
};
#else
	// projects that actually want to implement kill-eater functionality will want to put their list somewhere around here,
	// but unfortunately the base code relies on this specific definition being entry 0
	static const uint32 kKillEaterEvent_PlayerKill = 0;
#endif // defined( TF_DLL ) || defined( TF_GC_DLL ) || defined( TF_CLIENT_DLL )

enum strange_event_restriction_t
{
	kStrangeEventRestriction_None = 0,				// default -- unassigned, all events pass
	kStrangeEventRestriction_VictimSteamAccount,	// the victim must have a specific Steam ID
#if defined( TF_DLL ) || defined( TF_GC_DLL ) || defined( TF_CLIENT_DLL )
	kStrangeEventRestriction_Map,					// must be playing on a certain map when the event takes place
	kStrangeEventRestriction_Competitive,			// must be playing in a competitive game
#endif // defined( TF_DLL ) || defined( TF_GC_DLL ) || defined( TF_CLIENT_DLL )
	kStrangeEventRestrictionCount
};

// Ugh -- these are shared between the GC and the client. Maybe #define is slightly better than
// magic string literals?
#define KILL_EATER_RANK_LEVEL_BLOCK_NAME		"KillEaterRank"

#ifdef TF_DLL
	class CTFWeaponBase *GetKilleaterWeaponFromDamageInfo( const class CTakeDamageInfo *pInfo );
	// A specific CEconEntity caused a kill eater event to happen. For example, a weapon might cause a
	// player kill event so we want to update the stats for that specific weapon.
	void EconEntity_OnOwnerKillEaterEvent( class CEconEntity *pEconEntity, class CTFPlayer *pOwner, class CTFPlayer *pVictim, kill_eater_event_t eEventType, int nIncrementValue = 1 );
	void EconItemInterface_OnOwnerKillEaterEvent( class IEconItemInterface *pEconEntity, class CTFPlayer *pOwner, class CTFPlayer *pVictim, kill_eater_event_t eEventType, int nIncrementValue = 1 );
	void EconEntity_OnOwnerKillEaterEventNoPartner( class CEconEntity *pEconEntity, class CTFPlayer *pOwner, kill_eater_event_t eEventType, int nIncrementValue = 1 );
	void EconItemInterface_OnOwnerKillEaterEventNoPartner( class IEconItemInterface *pEconEntity, class CTFPlayer *pOwner, kill_eater_event_t eEventType, int nIncrementValue = 1 );

	void HatAndMiscEconEntities_OnOwnerKillEaterEvent( class CTFPlayer *pOwner, class CTFPlayer *pVictim, kill_eater_event_t eEventType, int nIncrementValue = 1 );
	void HatAndMiscEconEntities_OnOwnerKillEaterEventNoParter( class CTFPlayer *pOwner, kill_eater_event_t eEventType, int nIncrementValue = 1 );

	void EconEntity_NonEquippedItemKillTracking_NoPartner( class CTFPlayer *pOwner, item_definition_index_t iDefIndex, kill_eater_event_t eEventType, int nIncrementValue = 1 );
	void EconEntity_NonEquippedItemKillTracking_NoPartnerBatched( class CTFPlayer *pOwner, item_definition_index_t iDefIndex, kill_eater_event_t eEventType, int nIncrementValue = 1 );
	// Batching system for frequent events (ie., damage dealing). The game server will flush all batches
	// at specific time intervals and send up one composite message to avoid flooding the GC. Batched
	// messages will only work correctly for types that support increment values. Because the game client
	// and game server don't know which event types support increment values we can't do any checking
	// before we send the message.
	void EconEntity_OnOwnerKillEaterEvent_Batched( class CEconEntity *pEconEntity, class CTFPlayer *pOwner, class CTFPlayer *pVictim, kill_eater_event_t eEventType, int nIncrementValue = 1 );
	void EconItemInterface_OnOwnerKillEaterEvent_Batched( class IEconItemInterface *pEconEntity, class CTFPlayer *pOwner, class CTFPlayer *pVictim, kill_eater_event_t eEventType, int nIncrementValue = 1 );
	void KillEaterEvents_FlushBatches();
#endif // TF_DLL

int GetKillEaterAttrCount();
int GetKillEaterAttrCount_UserCustomizable();
const class CEconItemAttributeDefinition *GetKillEaterAttr_Score( int i );
const class CEconItemAttributeDefinition *GetKillEaterAttr_Type( int i );
const class CEconItemAttributeDefinition *GetKillEaterAttr_Restriction( int i );
const class CEconItemAttributeDefinition *GetKillEaterAttr_RestrictionValue( int i );
bool GetKillEaterAttr_IsUserCustomizable( int i );
bool GetKilleaterValueByEvent( const class IEconItemInterface* pItem, const kill_eater_event_t& EEventType, uint32& value );
bool BIsItemStrange( const class IEconItemInterface *pItem );

const int COLLECTION_CRAFTING_ITEM_COUNT = 10;
const char* GetCollectionCraftingInvalidReason( const class IEconItemInterface *pTestItem, const class IEconItemInterface *pSourceItem );

const int HALLOWEEN_OFFERING_ITEM_COUNT = 3;
const char* GetHalloweenOfferingInvalidReason( const class IEconItemInterface *pTestItem, const class IEconItemInterface *pSourceItem );

const int CRAFT_COMMON_STATCLOCK_ITEM_COUNT = 5;
const char* GetCraftCommonStatClockInvalidReason( const class IEconItemInterface *pTestItem, const class IEconItemInterface *pSourceItem );

int GetMaxCardUpgradesPerItem();
const class CEconItemAttributeDefinition *GetCardUpgradeForIndex( const class IEconItemInterface *pItem, int i );

#define GUARANTEED_OUTPUT	(1<<0)
#define GUARANTEED_INPUT	(1<<1)

#define DYNAMIC_RECIPE_FLAG_IS_OUTPUT			(1<<0)
#define DYNAMIC_RECIPE_FLAG_IS_UNTRADABLE		(1<<1)
#define DYNAMIC_RECIPE_FLAG_PARAM_ITEM_DEF_SET	(1<<2)
#define DYNAMIC_RECIPE_FLAG_PARAM_QUALITY_SET	(1<<3)
#define DYNAMIC_RECIPE_FLAG_PARAM_ATTRIBUTE_SET_ALL	(1<<4)
#define DYNAMIC_RECIPE_FLAG_PARAM_ATTRIBUTE_SET_ANY	(1<<5)

#define k_ObjectiveTrackerFlag_OwnerClient	(1<<0)
#define k_ObjectiveTrackerFlag_Servers		(1<<1)
#define k_ObjectiveTrackerFlag_AllClients	(1<<2)

#define k_ObjectiveTrackerFlag_ClientAndServer ( k_ObjectiveTrackerFlag_OwnerClient | k_ObjectiveTrackerFlag_Servers )

const float k_MaxElapsedQuestReportTime = 10.f;

//===============================================================================================================
// POSITION HANDLING
//===============================================================================================================
// TF Inventory Position cracking

// REALLY OLD FORMAT (??):
//		We store a bag index in the highbits of the inventory position.
//		The lowbit stores the position of the item within the bag.
//
// LESS OLD FORMAT (up through July, 2011):
//		If Bit 31 is 0: 
//			Bits 1-16 are the backpack position.
//			Bits 17-26 are a bool for whether the item is equipped in the matching class.
//		Otherwise, if Bit 31 is 1:
//			Item hasn't been acknowledged by the player yet.
//			Bits 1-16 are the method by the player found the item (see unacknowledged_item_inventory_positions_t)
//		Bit 32 is 1, to note the new format.
//
// CURRENT FORMAT:
//		If Bit 31 is 0: 
//			Bits 1-16 are the backpack position.
//		Otherwise, if Bit 31 is 1:
//			Item hasn't been acknowledged by the player yet.
//			Bits 1-16 are the method by the player found the item (see unacknowledged_item_inventory_positions_t)
//		Equipped state is stored elsewhere.
//		This is the only format that should exist on clients.
// Note (1/15/2013) For backwards compatibility, if the value is 0 item is considered unacknowledged too


enum unacknowledged_item_inventory_positions_t
{
	UNACK_ITEM_UNKNOWN = 0,
	UNACK_ITEM_DROPPED = 1,
	UNACK_ITEM_CRAFTED,
	UNACK_ITEM_TRADED,
	UNACK_ITEM_PURCHASED,
	UNACK_ITEM_FOUND_IN_CRATE,
	UNACK_ITEM_GIFTED,
	UNACK_ITEM_SUPPORT,
	UNACK_ITEM_PROMOTION,
	UNACK_ITEM_EARNED,
	UNACK_ITEM_REFUNDED,
	UNACK_ITEM_GIFT_WRAPPED,
	UNACK_ITEM_FOREIGN,
	UNACK_ITEM_COLLECTION_REWARD,
	UNACK_ITEM_PREVIEW_ITEM,
	UNACK_ITEM_PREVIEW_ITEM_PURCHASED,
	UNACK_ITEM_PERIODIC_SCORE_REWARD,
	UNACK_ITEM_MVM_MISSION_COMPLETION_REWARD,
	UNACK_ITEM_MVM_SQUAD_SURPLUS_REWARD,
	UNACK_ITEM_FOUND_HOLIDAY_GIFT,
	UNACK_ITEM_COMMUNITY_MARKET_PURCHASE,
	UNACK_ITEM_RECIPE_OUTPUT,
	UNACK_ITEM_HIDDEN_QUEST_ITEM, // DEPRECATED.  Quests are no longer items
	UNACK_ITEM_QUEST_OUTPUT,
	UNACK_ITEM_QUEST_LOANER,
	UNACK_ITEM_TRADE_UP,
	UNACK_ITEM_QUEST_MERASMISSION_OUTPUT,
	UNACK_ITEM_VIRAL_COMPETITIVE_BETA_PASS_SPREAD,
	UNACK_ITEM_CYOA_BLOOD_MONEY_PURCHASE,
	UNACK_ITEM_PAINTKIT,
#ifdef ENABLE_STORE_RENTAL_BACKEND
	UNACK_ITEM_RENTAL_PURCHASE,
#endif

	UNACK_NUM_METHODS,
};

extern const char *g_pszItemPickupMethodStrings[UNACK_NUM_METHODS - 1];			// -1 because UNACK_ITEM_DROPPED is index 1, not 0
extern const char *g_pszItemPickupMethodStringsUnloc[UNACK_NUM_METHODS - 1];
extern const char *g_pszItemFoundMethodStrings[UNACK_NUM_METHODS - 1];

enum
{
	kGCItemSort_NoSort				= 0,			// this won't do anything, but can be used as a safe "header" value

	kGCItemSort_SortByName			= 1,
	kGCItemSort_SortByDefIndex		= 2,
	kGCItemSort_SortByRarity		= 3,
	kGCItemSort_SortByType			= 4,
	kGCItemSort_SortByDate			= 5,

	kGCItemSort_GameSpecificBase	= 100,
};

// FIXME: these should be moved... somewhere; where?
enum
{
	kTFGCItemSort_SortByClass		= kGCItemSort_GameSpecificBase + 1,
	kTFGCItemSort_SortBySlot		= kGCItemSort_GameSpecificBase + 2,
};

enum
{
	kBackendPosition_Unacked	= 1 << 30,
	kBackendPosition_NewFormat	= 1 << 31,

	kBackendPositionMask_Position		= 0x0000ffff,
	kBackendPositionMask_FormatFlags	= (kBackendPosition_Unacked | kBackendPosition_NewFormat),
};

inline void SetBackpackPosition( uint32 *pPosition, uint32 iPackPosition )
{
	(*pPosition) = iPackPosition;

	// Remove the unack'd flag
	(*pPosition) &= ~kBackendPosition_Unacked;	
}

inline bool IsNewPositionFormat( uint32 iBackendPosition )
{
	return ( iBackendPosition & kBackendPosition_NewFormat ) != 0;
}

inline bool IsUnacknowledged( uint32 iBackendPosition )
{
	// For backwards compatibility, we consider position 0 as unacknowledged too
	return (iBackendPosition == 0 || (iBackendPosition & kBackendPosition_Unacked) != 0);
}

inline int ExtractBackpackPositionFromBackend( uint32 iBackendPosition )
{
	if ( IsUnacknowledged( iBackendPosition) )
		return 0;

	return iBackendPosition & kBackendPositionMask_Position;
}

inline unacknowledged_item_inventory_positions_t GetUnacknowledgedReason( uint32 iBackendPosition )
{
	return (unacknowledged_item_inventory_positions_t)( iBackendPosition &= ~kBackendPositionMask_FormatFlags );
}

inline uint32 GetUnacknowledgedPositionFor( unacknowledged_item_inventory_positions_t iMethod )
{
	return (iMethod | kBackendPosition_Unacked | kBackendPosition_NewFormat);
}

//-----------------------------------------------------------------------------
// Item Preview event IDs for logging.
//-----------------------------------------------------------------------------
enum EEconItemPreviewEventIDs
{
	k_EEconItemPreview_Start				=1,
	k_EEconItemPreview_Expired				=2,
	k_EEconItemPreview_ItemPurchased		=3,
};

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

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
enum ECartItemType
{
	kCartItem_Purchase,				// a normal lifetime purchase (needs to stay as entry 0!)
	kCartItem_TryOutUpgrade,		// an upgrade from "try-it-out"
	kCartItem_Rental_1Day,
	kCartItem_Rental_3Day,
	kCartItem_Rental_7Day,
};

inline bool IsRentalCartItemType( ECartItemType eCartType )
{
	return eCartType == kCartItem_Rental_1Day
		|| eCartType == kCartItem_Rental_3Day
		|| eCartType == kCartItem_Rental_7Day;
}

const uint8	 k_unItemRarity_Any = 0xFF;
const uint8	 k_unItemQuality_Any = 0xFF;

typedef int		econ_tag_handle_t;

enum EItemUntradability
{
	k_Untradability_Temporary	= 1<<1,
	k_Untradability_Permanent	= 1<<2,
};

#define INVALID_ECON_TAG_HANDLE	((econ_tag_handle_t)-1)

#endif // ACTUAL_ECON_ITEM_CONSTANTS_H
