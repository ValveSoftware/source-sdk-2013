//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This file defines all of our over-the-wire net protocols for the
//			Game Coordinator for the item system.  Note that we never use types
//			with undefined length (like int).  Always use an explicit type 
//			(like int32).
//
//=============================================================================

#ifndef ITEM_GCMESSAGES_H
#define ITEM_GCMESSAGES_H
#ifdef _WIN32
#pragma once
#endif

#include "econ_gcmessages.pb.h"

#pragma pack( push, 1 )


// generic zero-length message struct
struct MsgGCEmpty_t
{

};

// k_EMsgGCSetItemPosition 
struct MsgGCSetItemPosition_t
{
	uint64 m_unItemID;
	uint32 m_unNewPosition;
};

// k_EMsgGCCraft
struct MsgGCCraft_t
{
	int16	m_nRecipeDefIndex;
	uint16	m_nItemCount;
	// list of m_nItemCount uint64 item IDs
};

// k_EMsgGCDelete
struct MsgGCDelete_t
{
	uint64 m_unItemID;
};

// k_EMsgGCCraftResponse
struct MsgGCStandardResponse_t
{
	int16			m_nResponseIndex;
	uint32	m_eResponse;
};

// k_EMsgGCVerifyCacheSubscription
struct MsgGCVerifyCacheSubscription_t
{
	uint64 m_ulSteamID;
};

// k_EMsgGCNameItem
struct MsgGCNameItem_t
{
	uint64 m_unToolItemID;		// the Nametag item
	uint64 m_unSubjectItemID;	// the item to be renamed
	bool m_bDescription;
	// Varchar: Item name
};

// k_EMsgGCNameBaseItem
struct MsgGCNameBaseItem_t
{
	uint64 m_unToolItemID;				// the Nametag item
	uint32 m_unBaseItemDefinitionID;	// the base item definition to be renamed
	bool m_bDescription;
	// Varchar: Item name
};

// k_EMsgGCUnlockCrate
struct MsgGCUnlockCrate_t
{
	uint64 m_unToolItemID;		// the crate key
	uint64 m_unSubjectItemID;	// the crate to be decoded
};

// k_EMsgGCPaintItem
struct MsgGCPaintItem_t
{
	uint64 m_unToolItemID;		// the Paint Can item
	uint64 m_unSubjectItemID;	// the item to be painted
};

// k_EMsgGCGiftWrapItem
struct MsgGCGiftWrapItem_t
{
	uint64 m_unToolItemID;		// the Gift Wrap item
	uint64 m_unSubjectItemID;	// the item to be wrapped
};

// k_EMsgGCDeliverGift
struct MsgGCDeliverGift_t
{
	uint64 m_unGiftID;
	uint64 m_ulGiverSteamID;
	uint64 m_ulTargetSteamID;
};

// k_EMsgGCUnwrapGiftRequest
struct MsgGCUnwrapGiftRequest_t
{
	uint64 m_unItemID;
};

// k_EMsgGCMOTDRequest
struct MsgGCMOTDRequest_t
{
	RTime32		m_nLastMOTDRequest;	// Time at which the client last asked for MOTDs. GC will send back all MOTDs posted since.
	int16	m_eLanguage;
};

// k_EMsgGCMOTDRequestResponse
struct MsgGCMOTDRequestResponse_t
{
	int16		m_nEntries;
};

// k_EMsgGCCustomizeItemTexture
struct MsgGCCustomizeItemTexture_t
{
	uint64 m_unToolItemID;		// the tool
	uint64 m_unSubjectItemID;	// the item wants the texture
	uint64 m_unImageUGCHandle;	// cloud ID of image file (UGCHandle_t)
};

// k_EMsgGCSetItemStyle
struct MsgGCSetItemStyle_t
{
	uint64 m_unItemID;
	uint8 m_iStyle;
};

// k_EMsgGCItemPreviewCheckStatus
struct MsgGCCheckItemPreviewStatus_t
{
	uint32 m_unItemDefIndex;
};

// k_EMsgGCItemPreviewCheckStatusResponse
struct MsgGCItemPreviewCheckStatusResponse_t
{
	uint32 m_unItemDefIndex;
	uint32 m_eResponse;
	RTime32 m_timePreviewTime;
};

// k_EMsgGCItemPreviewRequest
struct MsgGCItemPreviewRequest_t
{
	uint32 m_unItemDefIndex;
};

// k_EMsgGCItemPreviewRequestResponse
struct MsgGCItemPreviewRequestResponse_t
{
	uint32 m_unItemDefIndex;
	uint32	m_eResponse;
};

// k_EMsgGCItemPreviewExpire
struct MsgGCItemPreviewExpire_t
{

};

// k_EMsgGCItemPreviewExpireNotification
struct MsgGCItemPreviewExpireNotification_t
{
	uint32 m_unItemDefIndex;
};

//-----------------------------------------------------------------------------

// k_EMsgGCUseItemResponse
enum EGCMsgUseItemResponse
{
	k_EGCMsgUseItemResponse_ItemUsed					= 0,
	k_EGCMsgUseItemResponse_GiftNoOtherPlayers			= 1,
	k_EGCMsgUseItemResponse_ServerError					= 2,
	k_EGCMsgUseItemResponse_MiniGameAlreadyStarted		= 3,
	k_EGCMsgUseItemResponse_ItemUsed_ItemsGranted		= 4,
	k_EGCMsgUseItemResponse_CannotBeUsedByAccount		= 5,
	k_EGCMsgUseItemResponse_CannotUseWhileUntradable	= 6,
	k_EGCMsgUseItemResponse_RecipientCannotRecieve		= 7,
	k_EGCMsgUseItemResponse_ForceSizeInt				= 0x7FFFFFFF
};

// k_EMsgGCUseItemResponse
struct MsgGCUseItemResponse_t
{
	uint32 m_eResponse;
};

// k_EMsgGCSpawnItem
struct MsgGCSpawnItem_t
{
	uint64 m_ulInitiatorSteamID;
	uint32 m_unItemDefinitionID;
	// other data dynamically added:
	// string of initiator name
};

// k_EMsgGCRespawnPostLoadoutChange
struct MsgGCRespawnPostLoadoutChange_t
{
	uint64 m_ulInitiatorSteamID;
};

// k_EMsgGCRemoveItemName
struct MsgGCRemoveItemName_t
{
	uint64 m_unItemID;
	bool m_bDescription;
};

//-----------------------------------------------------------------------------
// Trading

// k_EMsgGCTrading_InitiateTradeRequest
struct MsgGCTrading_InitiateTradeRequest_t
{
	uint32 m_unTradeRequestID;
	uint64 m_ulOtherSteamID;
	// @note player A's name as string when sent to party B
};

enum EGCMsgInitiateTradeResponse
{
	k_EGCMsgInitiateTradeResponse_Accepted				  = 0,
	k_EGCMsgInitiateTradeResponse_Declined				  = 1,
	k_EGCMsgInitiateTradeResponse_VAC_Banned_Initiator	  = 2,
	k_EGCMsgInitiateTradeResponse_VAC_Banned_Target		  = 3,
	k_EGCMsgInitiateTradeResponse_Target_Already_Trading  = 4,
	k_EGCMsgInitiateTradeResponse_Disabled				  = 5,
	k_EGCMsgInitiateTradeResponse_NotLoggedIn			  = 6,
	k_EGCMsgInitiateTradeResponse_Cancel				  = 7,
	k_EGCMsgInitiateTradeResponse_TooSoon				  = 8,
	k_EGCMsgInitiateTradeResponse_TooSoonPenalty		  = 9,
	k_EGCMsgInitiateTradeResponse_Trade_Banned_Initiator  = 10,
	k_EGCMsgInitiateTradeResponse_Trade_Banned_Target	  = 11,
	k_EGCMsgInitiateTradeResponse_Free_Account_Initiator_DEPRECATED  = 12,			// free accounts can initiate trades now
	k_EGCMsgInitiateTradeResponse_Shared_Account_Initiator= 13,
	k_EGCMsgInitiateTradeResponse_Service_Unavailable	  = 14,
	k_EGCMsgInitiateTradeResponse_Target_Blocked		  = 15,
	k_EGCMsgInitiateTradeResponse_NeedVerifiedEmail		  = 16,
	k_EGCMsgInitiateTradeResponse_NeedSteamGuard		  = 17,
	k_EGCMsgInitiateTradeResponse_SteamGuardDuration	  = 18,
	k_EGCMsgInitiateTradeResponse_TheyCannotTrade		  = 19,
	k_EGCMsgInitiateTradeResponse_Recent_Password_Reset	  = 20,
	k_EGCMsgInitiateTradeResponse_Using_New_Device		  = 21,
	k_EGCMsgInitiateTradeResponse_Sent_Invalid_Cookie	  = 22,

	k_EGCMsgInitiateTradeResponse_Count,
	k_EGCMsgInitiateTradeResponse_ForceSizeInt			  = 0x7FFFFFFF
};

// k_EMsgGCTrading_InitiateTradeResponse
struct MsgGCTrading_InitiateTradeResponse_t
{
	uint32	m_eResponse;
	uint32 m_unTradeRequestID;
};

// k_EMsgGCTrading_StartSession
struct MsgGCTrading_StartSession_t
{
	uint32 m_unSessionVersion;
	uint64 m_ulSteamIDPartyA;
	uint64 m_ulSteamIDPartyB;
	// @note strings from player names will be added to the message
};

// k_EMsgGCTrading_CancelSession
struct MsgGCTrading_CancelSession_t
{
};

// k_EMsgGCUsedClaimCodeItem
struct MsgGCUsedClaimCodeItem_t
{
	// string of URL
};

//-----------------------------------------------------------------------------
// ServerBrowser messages

enum EGCMsgServerBrowser
{
	k_EGCMsgServerBrowser_FromServerBrowser = 0,
	k_EGCMsgServerBrowser_FromAutoAskDialog = 1,
};

// k_EMsgGCServerBrowser_FavoriteServer
// k_EMsgGCServerBrowser_BlacklistServer
struct MsgGCServerBrowser_Server_t
{
	uint32 m_unIP;
	int m_usPort;
	uint8 m_ubSource;		// 0=serverbrowser, 1=auto-ask dialog
};

//-----------------------------------------------------------------------------
// Public facing loot lists.

// k_EMsgGC_RevolvingLootList
struct MsgGC_RevolvingLootList_t
{
	uint8		m_usListID; // Id of this list.
	// Var Data:
	// Serialized Lootlist KV
};


// k_EMsgGCLookupAccount 
struct MsgGCLookupAccount_t
{
	uint16	m_uiFindType;

	// Var Data
	// string containing Persona / URL / etc
};

// k_EMsgGCLookupAccountName 
struct MsgGCLookupAccountName_t
{
	uint32 m_unAccountID;
};

// k_EMsgGCLookupAccountNameResponse
struct MsgGCLookupAccountNameResponse_t
{
	uint32 m_unAccountID;
	// string containing persona name
};

#pragma pack( pop )

#endif
