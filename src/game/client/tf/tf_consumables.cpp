//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"

// for messaging with the GC
#include "econ_gcmessages.h"
#include "econ_item_inventory.h"
#include "tf_gcmessages.h"
#include "tf_duel_summary.h"
#include "gc_clientsystem.h"

// other
#include "c_playerresource.h"
#include "c_tf_player.h"
#include "tf_item_wearable.h"
#include "econ_notifications.h"
#include "tf_hud_chat.h"
#include "c_tf_gamestats.h"
#include "tf_gamerules.h"
#include "tf_item_tools.h"
#include "c_tf_freeaccount.h"
#include "tf_item_powerup_bottle.h"
#include "tf_weapon_grapplinghook.h"

// for UI
#include "clientmode_tf.h"
#include "confirm_dialog.h"
#include "select_player_dialog.h"
#include "econ_notifications.h"
#include "vgui/ISurface.h"
#include "vgui/character_info_panel.h"
#include "tf_hud_mainmenuoverride.h"
#include "econ_ui.h"
#include "backpack_panel.h"
#include "store/v1/tf_store_page.h"
#include "econ_item_description.h"
#include "weapon_selection.h"
#include "collection_crafting_panel.h"
#include "clientmode_tf.h"
#include "vgui_controls/AnimationController.h"
#include "tf_matchmaking_dashboard_explanations.h"
#include "tf_matchmaking_dashboard_parent_manager.h"
#include "tf_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

ConVar tf_warpaint_explanation_viewed( "tf_warpaint_explanation_viewed", 0, FCVAR_ARCHIVE );

//-----------------------------------------------------------------------------
// Wrapped Gift Declarations

void UseGift( CEconItemView* pItem, CSteamID targetID );
extern void PerformToolAction_UnwrapGift( vgui::Panel* pParent, CEconItemView *pGiftItem );
extern void ShowWaitingDialog( CGenericWaitingDialog *pWaitingDialog, const char* pUpdateText, bool bAnimate, bool bShowCancel, float flMaxDuration );
class CDeliverGiftSelectDialog;
CDeliverGiftSelectDialog *OpenDeliverGiftDialog( vgui::Panel *pParent, CEconItemView *pItem );
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Duel Declarations
bool DuelMiniGame_IsDuelingLocalPlayer( C_TFPlayer *pPlayer );
bool DuelMiniGame_IsDueling();
//-----------------------------------------------------------------------------

class CWaitForPackageDialog : public CGenericWaitingDialog
{
public:
	CWaitForPackageDialog( vgui::Panel *pParent ) : CGenericWaitingDialog( pParent )
	{
	}

protected:
	virtual void OnTimeout()
	{
		// Play an exciting sound!
		vgui::surface()->PlaySound( "misc/achievement_earned.wav" );

		// Show them their loot!
		InventoryManager()->ShowItemsPickedUp( true );
	}
};

bool IgnoreRequestFromUser( const CSteamID &steamID )
{
	// ignore blocked players
	if ( steamapicontext && steamapicontext->SteamFriends() )
	{
		EFriendRelationship eRelationship = steamapicontext->SteamFriends()->GetFriendRelationship( steamID );
		switch ( eRelationship )
		{
			case k_EFriendRelationshipBlocked:
			{
				return true;
			}
		}
	}
	return false;
}

static void ShowSelectDuelTargetDialog( uint64 iItemID );

enum EServerPlayersGCSend
{
	kServerPlayers_DontSend,
	kServerPlayers_Send,
};

struct CUseItemConfirmContext
{
public:
	CUseItemConfirmContext( CEconItemView *pEconItemView, EServerPlayersGCSend eSendServerPlayers, const char* pszConfirmUseSound = NULL )
		: m_pEconItemView( pEconItemView )
		, m_bSendServerPlayers( eSendServerPlayers == kServerPlayers_Send )
		, m_pszConfirmUseSound( pszConfirmUseSound )
	{
		Assert( eSendServerPlayers == kServerPlayers_DontSend || eSendServerPlayers == kServerPlayers_Send );
	}

	void OnConfirmUse()
	{
		if ( m_pszConfirmUseSound && *m_pszConfirmUseSound )
		{
			vgui::surface()->PlaySound( m_pszConfirmUseSound );
		}
	}

	const char* m_pszConfirmUseSound;
	CEconItemView *m_pEconItemView;
	bool m_bSendServerPlayers;
};

static void UseItemConfirm( bool bConfirmed, void *pContext )
{
	static CSchemaAttributeDefHandle pAttrDef_UnlimitedUse( "unlimited quantity" );

	CUseItemConfirmContext *pConfirmContext = (CUseItemConfirmContext *)pContext;
	CEconItemView *pEconItemView = pConfirmContext->m_pEconItemView;

	// Chance restricted? Check if the item is and fail.
	const char *pUserTxnCC = GCClientSystem()->GetTxnCountryCode();
	bool bUserChanceRestricted = pUserTxnCC && !BEconCountryAllowDecodableContainers( pUserTxnCC );
	if ( bUserChanceRestricted )
	{
		// Restricted country?
		const GameItemDefinition_t *pItemDef = pEconItemView->GetItemDefinition();
		if ( pItemDef && pItemDef->IsChanceRestricted() )
		{
			ShowMessageBox( "#ToolContainerRestrictedTitle", "#ToolContainerRestricted", "#GameUI_OK" );
			return;
		}
	}

	if ( bConfirmed )
	{
		pConfirmContext->OnConfirmUse();

		if ( pEconItemView && !pEconItemView->FindAttribute( pAttrDef_UnlimitedUse ) )
		{
			GCSDK::CProtoBufMsg<CMsgUseItem> msg( k_EMsgGCUseItemRequest );
			msg.Body().set_item_id( pConfirmContext->m_pEconItemView->GetItemID() );

			if ( pConfirmContext->m_bSendServerPlayers )
			{
				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
					if ( pPlayer == NULL )
						continue;

					CSteamID steamIDPlayer;
					if ( !pPlayer->GetSteamID( &steamIDPlayer ) )
						continue;

					msg.Body().add_gift__potential_targets( steamIDPlayer.GetAccountID() );
				}
			}

			GCClientSystem()->BSendMessage( msg );
		}

		EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_CONSUMABLE, pEconItemView, NULL );

		// CBaseHudChat *pHUDChat = (CBaseHudChat *)GET_HUDELEMENT( CHudChat );
		// if ( pHUDChat )
		// {
		// 	char szAnsi[1024];
		// 	Q_snprintf( szAnsi, 1024, "Using item: %ull", pItem->GetItemID() );
		// 	pHUDChat->Printf( CHAT_FILTER_NONE, "%s", szAnsi );
		// }
	}
	delete pConfirmContext;
}

static void OpenPass( bool bConfirmed, void *pContext )
{
	if ( bConfirmed )
	{
		vgui::surface()->PlaySound( "ui/item_gift_wrap_unwrap.wav" );
		ShowWaitingDialog( new CWaitForPackageDialog( NULL ), "#ToolRedeemingPass", true, false, 5.0f );
	}
	UseItemConfirm( bConfirmed, pContext );
}

static void PrintTextToChat( const char *pText, KeyValues *pKeyValues )
{
	GetClientModeTFNormal()->PrintTextToChat( pText, pKeyValues );
}

void GetPlayerNameBySteamID( const CSteamID &steamID, OUT_Z_CAP(maxLenInChars) char *pDestBuffer, int maxLenInChars )
{
	// always attempt to precache this user's name -- we may need it later after they leave the server, for instance,
	// even if that's where they are now
	InventoryManager()->PersonaName_Precache( steamID.GetAccountID() );

	// first, look through players on our connected gameserver if available -- we already have this information and
	// if there's a disagreement between Steam and the gameserver the gameserver view is what the player is probably
	// expecting anyway
	if ( engine->IsInGame() )
	{
		for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
		{
			if ( g_PR->IsConnected( iPlayerIndex ) )
			{
				player_info_t pi;
				if ( !engine->GetPlayerInfo( iPlayerIndex, &pi ) )
					continue;
				if ( !pi.friendsID )
					continue;

				CSteamID steamIDTemp( pi.friendsID, 1, GetUniverse(), k_EAccountTypeIndividual );
				if ( steamIDTemp == steamID )
				{
					V_strncpy( pDestBuffer, UTIL_GetFilteredPlayerName( steamID, pi.name ), maxLenInChars );
					return;
				}
			}
		}
	}

	// try the persona name cache
	// this goes to steam if necessary
	const char *pszName = InventoryManager()->PersonaName_Get( steamID.GetAccountID() );
	if ( pszName != NULL )
	{
		V_strncpy( pDestBuffer, pszName, maxLenInChars );
	}
	else
	{
		// otherwise, return what we would normally return
		V_strncpy( pDestBuffer, steamapicontext->SteamFriends()->GetFriendPersonaName( steamID ), maxLenInChars );
	}
	UTIL_GetFilteredPlayerName( steamID, pDestBuffer );
}

static bool IsGCUseableItem( const GameItemDefinition_t *pItemDef )
{
	Assert( pItemDef );

	return (pItemDef->GetCapabilities() & ITEM_CAP_USABLE_GC) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: Implementation of the local response for someone who used a dueling minigame
//-----------------------------------------------------------------------------
void CEconTool_DuelingMinigame::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( pItem );

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	Assert( pLocalPlayer );

	if ( DuelMiniGame_IsDueling() )
	{
		// can't duel if already dueling
		ShowMessageBox( "#TF_Duel_Title",  "#TF_Duel_InADuel_Initiator", "#GameUI_OK" );
		return;
	}
	if ( pLocalPlayer->GetTeamNumber() != TF_TEAM_RED && pLocalPlayer->GetTeamNumber() != TF_TEAM_BLUE )
	{
		// can't duel from spectator mode, etc.
		ShowMessageBox( "#TF_UseFail_NotOnTeam_Title",  "#TF_UseFail_NotOnTeam", "#GameUI_OK" );
		return;
	}
	if ( TFGameRules() && !TFGameRules()->CanInitiateDuels() )
	{
		ShowMessageBox( "#TF_Duel_Title",  "#TF_Duel_CannotUse", "#GameUI_OK" );
		return;
	}

	ShowSelectDuelTargetDialog( pItem->GetItemID() );
}

//-----------------------------------------------------------------------------
// Purpose: Implementation of the local response for someone who used a noisemaker
//-----------------------------------------------------------------------------
void CEconTool_Noisemaker::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( pItem );

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	Assert( pLocalPlayer );

	if ( gpGlobals->curtime < pLocalPlayer->m_Shared.GetNextNoiseMakerTime() )
		return;

	if ( !pLocalPlayer->IsAlive() )
		return;

	if ( pLocalPlayer->GetTeamNumber() < FIRST_GAME_TEAM ) 
		return;

	// This may not be ideal. We're going to have the game server do the noise effect,
	// without checking the GC to see whether we have charges available. Querying the
	// GC would cause a significant delay before the item was used, so we simulate.

	// Tell the game server to play the sound.
	KeyValues *kv = new KeyValues( "use_action_slot_item_server" );
	engine->ServerCmdKeyValues( kv );

	// Tell the GC to consume a charge.
	CUseItemConfirmContext *context = new CUseItemConfirmContext( pItem, kServerPlayers_DontSend );
	UseItemConfirm( true, context );

	// Notify the player that they used their last charge.
	if ( pItem->GetItemQuantity() <= 1 )
	{
		CEconNotification *pNotification = new CEconNotification();
		pNotification->SetText( "#TF_NoiseMaker_Exhausted" );
		pNotification->SetLifetime( 7.0f );
		NotificationQueue_Add( pNotification );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Implementation of the local response for someone who used a gift-wrapped item
//-----------------------------------------------------------------------------
void UseUntargetedGiftConfirm( bool bConfirmed, void *pContext )
{
	if ( bConfirmed )
	{
		UseGift( static_cast<CEconItemView *>( pContext ), k_steamIDNil );
	}
}

void CEconTool_WrappedGift::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( pItem );

	if ( BIsDirectGift() )
	{
		OpenDeliverGiftDialog( pParent, pItem );
	}
	else
	{
		// ...otherwise, we should try and open the gift!
		PerformToolAction_UnwrapGift( pParent, pItem );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconTool_WeddingRing::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( pItem );

	// Don't do anything if we haven't been gifted already -- we don't expect to
	// ever get in here, really.
	static CSchemaAttributeDefHandle pAttrDef_GifterAccountID( "gifter account id" );

	uint32 unAccountID;
	if ( !pItem->FindAttribute( pAttrDef_GifterAccountID, &unAccountID ) )
		return;

	// We have been gifted, so pop up a dialog box to allow the user to accept/reject
	// the proposal.
	CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( "#TF_UseWeddingRing_Title", "#TF_UseWeddingRing_Text", 
														  "#TF_WeddingRing_AcceptProposal", "#TF_WeddingRing_RejectProposal", 
														  &UseItemConfirm );

	pDialog->AddStringToken( "item_name", pItem->GetItemName() );

	CUtlConstWideString sProposerPersonaName;
	GLocalizationProvider()->ConvertUTF8ToLocchar( TFInventoryManager()->PersonaName_Get( unAccountID ), &sProposerPersonaName );
	pDialog->AddStringToken( "proposer_name", sProposerPersonaName.Get() );

	pDialog->SetContext( new CUseItemConfirmContext( pItem, kServerPlayers_DontSend ) );
}

//-----------------------------------------------------------------------------
// Purpose: Implementation of the local response for someone who used a backpack expander
//-----------------------------------------------------------------------------
void CEconTool_BackpackExpander::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( pItem );

	// first validate that they aren't already at max inventory size and can use the item
	uint32 unExtraSlots =GetBackpackSlots();
	if ( unExtraSlots == 0 )
	{
		return;
	}
	uint32 unNewNumSlots = TFInventoryManager()->GetLocalTFInventory()->GetMaxItemCount() + unExtraSlots;
	if ( unNewNumSlots > MAX_NUM_BACKPACK_SLOTS )
	{
		ShowMessageBox( "#TF_UseBackpackExpanderFail_Title",  "#TF_UseBackpackExpanderFail_Text", "#GameUI_OK" );
		return;
	}
	// Free Trials can use expanders but max out at a smaller value since premium gains a bunch of free slots
	if ( IsFreeTrialAccount() && unNewNumSlots > MAX_NUM_BACKPACK_SLOTS - (DEFAULT_NUM_BACKPACK_SLOTS - DEFAULT_NUM_BACKPACK_SLOTS_FREE_TRIAL_ACCOUNT) )
	{
		ShowMessageBox( "#TF_UseBackpackExpanderFail_Title",  "#TF_UseBackpackExpanderFail_Text", "#GameUI_OK" );
		return;
	}
	
	CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( "#TF_UseBackpackExpander_Title", "#TF_UseBackpackExpander_Text", 
														  "#GameUI_OK", "#Cancel", 
														  &UseItemConfirm );
	pDialog->AddStringToken( "item_name", pItem->GetItemName() );
	wchar_t wszUsesLeft[32];
	_snwprintf( wszUsesLeft, ARRAYSIZE(wszUsesLeft), L"%d", pItem->GetItemQuantity() );
	pDialog->AddStringToken( "uses_left", wszUsesLeft );
	wchar_t wszNewSize[32];
	_snwprintf( wszNewSize, ARRAYSIZE(wszNewSize), L"%d", unNewNumSlots );
	pDialog->AddStringToken( "new_size", wszNewSize );
	pDialog->SetContext( new CUseItemConfirmContext( pItem, kServerPlayers_DontSend ) );
}

//-----------------------------------------------------------------------------
// Purpose: Implementation of the local response for someone who used an account upgrade
//-----------------------------------------------------------------------------
void CEconTool_AccountUpgradeToPremium::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( pItem );

	// if the account is already premium, abort here
	if ( !IsFreeTrialAccount() )
	{
		ShowMessageBox( "#TF_UseAccountUpgradeToPremiumFail_Title",  "#TF_UseAccountUpgradeToPremiumFail_Text", "#GameUI_OK" );
		return;
	}

	// show a confirmation dialog to make sure they want to consume the charge
	CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( "#TF_UseAccountUpgradeToPremium_Title", "#TF_UseAccountUpgradeToPremium_Text", 
														  "#GameUI_OK", "#Cancel", 
														  &UseItemConfirm );
	pDialog->AddStringToken( "item_name", pItem->GetItemName() );
	wchar_t wszUsesLeft[32];
	_snwprintf( wszUsesLeft, ARRAYSIZE(wszUsesLeft), L"%d", pItem->GetItemQuantity() );
	pDialog->AddStringToken( "uses_left", wszUsesLeft );
	pDialog->SetContext( new CUseItemConfirmContext( pItem, kServerPlayers_DontSend ) );
}

//-----------------------------------------------------------------------------
// Purpose: Implementation of the local response for someone who used a claim code item
//-----------------------------------------------------------------------------
void CEconTool_ClaimCode::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( pItem );

	CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( "#TF_UseClaimCode_Title", "#TF_UseClaimCode_Text", 
														  "#GameUI_OK", "#Cancel", 
														  &UseItemConfirm );
	pDialog->AddStringToken( "item_name", pItem->GetItemName() );

	const char *pszClaimValue = GetClaimType();
	if ( pszClaimValue )
	{
		wchar_t wszClaimType[128];
		KeyValuesAD pkvDummy( "dummy" );
		g_pVGuiLocalize->ConstructString_safe( wszClaimType, pszClaimValue, pkvDummy );
		pDialog->AddStringToken( "claim_type", wszClaimType );
	}
	pDialog->SetContext( new CUseItemConfirmContext( pItem, kServerPlayers_DontSend ) );
}

//-----------------------------------------------------------------------------
// Purpose: Implementation of the local response for someone who used an item that doesn't have
//			special-case handling (ie., paint); called into from other code
//-----------------------------------------------------------------------------
static bool s_bConsumableToolOpeningGift = false;
extern int s_iCrateType;
static void ClientConsumableTool_CustomCallback( CEconItemView *pItem, vgui::Panel *pParent, GenericConfirmDialogCallback callback )
{
	Assert( pItem );
	Assert( pItem->GetItemDefinition() );
	Assert( pItem->GetItemDefinition()->GetEconTool() );

	CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( "#TF_UseItem_Title", "#TF_UseItem_Text", 
														  "#GameUI_OK", "#Cancel", 
														  callback );
	pDialog->AddStringToken( "item_name", pItem->GetItemName() );
	wchar_t wszUsesLeft[32];
	_snwprintf( wszUsesLeft, ARRAYSIZE(wszUsesLeft), L"%d", pItem->GetItemQuantity() );
	pDialog->AddStringToken( "uses_left", wszUsesLeft );
	pDialog->SetContext(
		new CUseItemConfirmContext( pItem,
									pItem->GetItemDefinition()->GetTypedEconTool<CEconTool_Gift>()
										? kServerPlayers_Send
										: kServerPlayers_DontSend ) );

	// Minor Hack to get sound to play differently.  Add a look up table
	s_bConsumableToolOpeningGift = false;
	const CEconTool_Gift *pGiftTool = pItem->GetItemDefinition()->GetTypedEconTool<CEconTool_Gift>();
	if ( pGiftTool && pGiftTool->GetTargetRule() == kGiftTargetRule_OnlySelf)
	{
		s_bConsumableToolOpeningGift = true;
	}

	s_iCrateType = CRATETYPE_NORMAL;
	// Check Crate Type
	if ( pItem )
	{
		static CSchemaAttributeDefHandle pAttrib_IsWinterCase( "is winter case" );
		uint32 nIsWinterCase = 0;
		if ( pItem->FindAttribute( pAttrib_IsWinterCase, &nIsWinterCase ) && nIsWinterCase != 0 )
		{
			s_iCrateType = CRATETYPE_WINTER;
		}
	}
}

static void ClientConsumableTool_Generic( CEconItemView *pItem, vgui::Panel *pParent )
{
	ClientConsumableTool_CustomCallback( pItem, pParent, &UseItemConfirm );
}

//-----------------------------------------------------------------------------
// Purpose: Generic Response
//-----------------------------------------------------------------------------
void CEconTool_Xifier::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	ClientConsumableTool_Generic( pItem, pParent );
}

//-----------------------------------------------------------------------------
void CEconTool_ItemEaterRecharger::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	// Tell the GC to consume a charge.
	CUseItemConfirmContext *context = new CUseItemConfirmContext( pItem, kServerPlayers_DontSend );
	UseItemConfirm( true, context );
}

//-----------------------------------------------------------------------------
// Purpose: Implementation of the local response for someone who used (tried to redeem) a collection
//-----------------------------------------------------------------------------
void CEconTool_PaintCan::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	ClientConsumableTool_Generic( pItem, pParent );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconTool_Gift::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	ClientConsumableTool_Generic( pItem, pParent );
}

//-----------------------------------------------------------------------------
// Purpose: Implementation of the local response for someone who used (tried to redeem) a collection
//-----------------------------------------------------------------------------
void CEconTool_Default::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	ClientConsumableTool_Generic( pItem, pParent );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconTool_TFEventEnableHalloween::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( pItem );

	// Tell the GC we want to use this item.
	GCSDK::CProtoBufMsg<CMsgGC_Client_UseServerModificationItem> msg( k_EMsgGC_Client_UseServerModificationItem );
	msg.Body().set_item_id( pItem->GetItemID() );

	GCClientSystem()->BSendMessage( msg );
}

//-----------------------------------------------------------------------------
void CEconTool_DuckToken::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	ClientConsumableTool_Generic( pItem, pParent );
}
//-----------------------------------------------------------------------------
void CEconTool_GrantOperationPass::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( pItem );
	const CEconTool_GrantOperationPass *pEconToolOperationPass = pItem->GetItemDefinition()->GetTypedEconTool<CEconTool_GrantOperationPass>();
	if ( !pEconToolOperationPass )
	{
		ShowMessageBox( "#TF_UseOperationPassFail_Title", "#TF_UseOperationPassFail_Text", "#GameUI_OK" );
		return;
	}

	// Check that the player doesn't already have an active pass
	const char *szPassName = pEconToolOperationPass->m_pOperationPassName;
	CEconItemDefinition *pActivePassItemDef = GetItemSchema()->GetItemDefinitionByName( szPassName );
	CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
	if ( !pLocalInv || !pActivePassItemDef  )
	{
		ShowMessageBox( "#TF_UseOperationPassFail_Title", "#TF_UseOperationPassFail_Text", "#GameUI_OK" );
		return;
	}

	for ( int i = 0; i < pLocalInv->GetItemCount(); ++i )
	{
		CEconItemView *pItemLocal = pLocalInv->GetItem( i );
		Assert( pItemLocal );
		if ( pItemLocal->GetItemDefinition() == pActivePassItemDef )
		{
			ShowMessageBox( "#TF_UseOperationPassAlreadyActive_Title", "#TF_UseOperationPassAlreadyActive_Text", "#GameUI_OK" );	
			return;
		}
	}

	vgui::surface()->PlaySound( "ui/quest_operation_pass_buy.wav" );

	const char *pszTitle = "#TF_UseOperationPass_Title";
	const char *pszBody = "#TF_UseOperationPass_Text";

	static CSchemaItemDefHandle pItemDef_InvasionPass( "Unused Invasion Pass" ); 
	if ( pItem->GetItemDefinition() == pItemDef_InvasionPass )
	{
		pszBody = "#TF_UseInvasionPass_Text";
	}

	// show a confirmation dialog to make sure they want to consume the charge
	CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( pszTitle, pszBody,
		"#GameUI_OK", "#Cancel",
		&OpenPass );
	pDialog->AddStringToken( "item_name", pItem->GetItemName() );
	wchar_t wszUsesLeft[32];
	_snwprintf( wszUsesLeft, ARRAYSIZE( wszUsesLeft ), L"%d", pItem->GetItemQuantity() );
	pDialog->AddStringToken( "uses_left", wszUsesLeft );
	pDialog->SetContext( new CUseItemConfirmContext( pItem, kServerPlayers_DontSend, "ui/quest_operation_pass_use.wav" ) );
}
//-----------------------------------------------------------------------------
static void OpenKeylessCase( bool bConfirmed, void *pContext )
{
	const char *pUserTxnCC = GCClientSystem()->GetTxnCountryCode();
	bool bUserChanceRestricted = pUserTxnCC && !BEconCountryAllowDecodableContainers( pUserTxnCC );

	if ( bUserChanceRestricted )
	{
		ShowMessageBox( "#ToolContainerRestrictedTitle", "#ToolContainerRestricted", "#GameUI_OK" );
		return;
	}

	CUseItemConfirmContext *pConfirmContext = (CUseItemConfirmContext *)pContext;
	CEconItemView *pCaseEconItemView = pConfirmContext->m_pEconItemView;

	if ( bConfirmed )
	{
		static CSchemaAttributeDefHandle pAttrDef_SupplyCrateSeries( "set supply crate series" );

		// Tell the GC to unlock the subject item.
		GCSDK::CGCMsg< MsgGCUnlockCrate_t > msg( k_EMsgGCUnlockCrate );

		msg.Body().m_unToolItemID = INVALID_ITEM_ID;
		msg.Body().m_unSubjectItemID = pCaseEconItemView->GetItemID();

		int iSeries = 0;
		float fSeries;
		if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pCaseEconItemView, pAttrDef_SupplyCrateSeries, &fSeries ) )
		{
			iSeries = fSeries;
		}

		EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_CONSUMABLE, pCaseEconItemView, "unlocked_supply_crate", iSeries );

		GCClientSystem()->BSendMessage( msg );

		CCollectionCraftingPanel *pPanel = EconUI()->GetBackpackPanel()->GetCollectionCraftPanel();
		if ( pPanel )
		{
			pPanel->SetWaitingForItem( kEconItemOrigin_FoundInCrate );
		}
	}
	delete pConfirmContext;
}

void CEconTool_KeylessCase::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	ClientConsumableTool_CustomCallback( pItem, pParent, &OpenKeylessCase );
}


void PaintkitConfirmCallback( bool bConfirmed, void *pContext );

class CTFPainkitConsumeDialog : public EditablePanel
							  , public CLocalSteamSharedObjectListener
{
public:
	DECLARE_CLASS_SIMPLE( CTFPainkitConsumeDialog, EditablePanel );
	CTFPainkitConsumeDialog( C_EconItemView* pItem )
		: BaseClass( NULL, "PaintkitConsume" )
		, m_pItem( pItem )
		, m_nSourceItemID( pItem->GetID() )
	{
		TFModalStack()->PushModal( this );
		GetMMDashboardParentManager()->AddPanel( this );

		m_pIspectionPanel = new CTFItemInspectionPanel( this, "InspectionPanel" );
		m_pIspectionPanel->SetOptions( false, true, true );

		if ( !tf_warpaint_explanation_viewed.GetBool() )
		{
			tf_warpaint_explanation_viewed.SetValue( true );
			ShowDashboardExplanation( "WarPaintUse" );
		}
	}

	virtual ~CTFPainkitConsumeDialog()
	{
		TFModalStack()->PopModal( this );
		GetMMDashboardParentManager()->RemovePanel( this );
	}

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE
	{
		LoadControlSettings( "Resource/UI/econ/PaintkitConsumeDialog.res" );
		m_pWorkingLogoPanel = FindControl< CTFLogoPanel >( "WorkingLogo", true );
		m_pSuccessLogoPanel = FindControl< CTFLogoPanel >( "SuccessLogo", true );
		m_pIspectionPanel->InvalidateLayout( true, true );
		BaseClass::ApplySchemeSettings( pScheme );

		m_pIspectionPanel->SetItemCopy( m_pItem, true );
	}

	virtual void OnCommand( const char* pszCommand ) OVERRIDE
	{
		if ( FStrEq( pszCommand, "accept" ) ) 
		{
			CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( "#TF_UsePaintkit_Title", "#TF_UsePaintkit_Text", 
																  "#GameUI_OK", "#Cancel", 
																  PaintkitConfirmCallback );
			pDialog->AddStringToken( "item_name", m_pItem->GetItemName() );
			pDialog->SetContext( this );
			pDialog->MakeReadyForUse();
			
			int nX = 0;
			int nY = YRES( 250 );
			LocalToScreen( nX, nY );
			pDialog->SetPos( pDialog->GetXPos(), nY );
			pDialog->SetTall( YRES( 150 ) );
		}
		else if ( FStrEq( pszCommand, "cancel" ) ) 
		{
			MarkForDeletion();
			return;
		}
	}

	MESSAGE_FUNC( OnConfirmPopup, "ConfirmPopup" )
	{
		// Tell the GC to consume the paintkit and give a painted item
		GCSDK::CProtoBufMsg< CMsgConsumePaintkit > msg( k_EMsgGCConsumePaintKit );

		msg.Body().set_source_id( m_nSourceItemID );
		msg.Body().set_target_defindex( m_nSelectedItem );

		GCClientSystem()->BSendMessage( msg );

		PlaySoundEntry( "UI.WarPaintApplyStart" );
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pWorkingLogoPanel, "velocity", 200, 0.0f, 0.f, vgui::AnimationController::INTERPOLATOR_ACCEL, 0.75f, true, false );
		SetControlVisible( "RedeemingPanel", true );
		SetControlVisible( "Shade", true );
		PostMessage( this, new KeyValues( "NoResponse" ), 5.f );
		m_bSuccess = false;
	}

	MESSAGE_FUNC_PARAMS( OnItemSelected, "ItemSelected", pKVParams )
	{
		m_nSelectedItem = pKVParams->GetInt( "defindex" );
	}

	MESSAGE_FUNC( AckItems, "AckItems" )
	{
		MarkForDeletion();
		// Play an exciting sound!
		vgui::surface()->PlaySound( "misc/achievement_earned.wav" );
		InventoryManager()->ShowItemsPickedUp( true, false );
	}

	MESSAGE_FUNC( NoResponse, "NoResponse" )
	{
		// We succeeded, so don't do anything
		if ( m_bSuccess )
			return;

		SetControlVisible( "RedeemingPanel", false );
		SetControlVisible( "SuccessPanel", false );
		SetControlVisible( "FailurePanel", true );

		PostMessage( this, new KeyValues( "Reset" ), 3.f );
	}

	MESSAGE_FUNC( Reset, "Reset" )
	{
		SetControlVisible( "Shade", false );
		SetControlVisible( "RedeemingPanel", false );
		SetControlVisible( "SuccessPanel", false );
		SetControlVisible( "FailurePanel", false );
	}

	MESSAGE_FUNC( ShowSuccess, "ShowSuccess" )
	{
		SetControlVisible( "RedeemingPanel", false );
		SetControlVisible( "SuccessPanel", true );

		Color creditsGreen = GetColor( "CreditsGreen" );
		Color brightGreen = creditsGreen;
		BrigthenColor( brightGreen, 50 );
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pSuccessLogoPanel, "fgcolor", brightGreen, 0.f, 0.f, vgui::AnimationController::INTERPOLATOR_ACCEL, 0.75f, true, false );
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pSuccessLogoPanel, "fgcolor", creditsGreen, 0.f, 1.5f, vgui::AnimationController::INTERPOLATOR_DEACCEL, 0.75f, false, false );

		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pSuccessLogoPanel, "radius", 40, 0.f, 0.05f, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.75f, true, false );
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pSuccessLogoPanel, "radius", 30, 0.05f, 0.2f, vgui::AnimationController::INTERPOLATOR_DEACCEL, 0.75f, false, false );
	}

	virtual void SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE
	{
		if ( eEvent != eSOCacheEvent_Incremental)
			return;

		if ( pObject->GetTypeID() != CEconItem::k_nTypeID )
			return;

		const CEconItem* pItem = assert_cast< const CEconItem* >( pObject );
		unacknowledged_item_inventory_positions_t reason = GetUnacknowledgedReason( pItem->GetInventoryToken() );
		if ( reason != UNACK_ITEM_PAINTKIT )
			return;

		PlaySoundEntry( "UI.WarPaintApplyStop" );

		// This is what we were waiting for
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pWorkingLogoPanel, "velocity", 1000, 0.0f, 2.f, vgui::AnimationController::INTERPOLATOR_BIAS, 0.01f, true, false );
		PostMessage( this, new KeyValues( "ShowSuccess" ), 3.f );
		PostMessage( this, new KeyValues( "AckItems" ), 6.f );
		m_bSuccess = true;
	}


private:

	CTFItemInspectionPanel* m_pIspectionPanel;
	C_EconItemView* m_pItem;
	uint64 m_nSourceItemID;
	uint32 m_nSelectedItem;
	CTFLogoPanel* m_pWorkingLogoPanel;
	CTFLogoPanel* m_pSuccessLogoPanel;
	bool m_bSuccess = false;
};

void PaintkitConfirmCallback( bool bConfirmed, void *pContext )
{
	if ( bConfirmed )
	{
		CTFPainkitConsumeDialog* pPanel = (CTFPainkitConsumeDialog*)pContext;
		pPanel->PostMessage( pPanel, new KeyValues( "ConfirmPopup" ) );
	}
}

//-----------------------------------------------------------------------------
void CEconTool_PaintKit::OnClientUseConsumable( class C_EconItemView *pItem, vgui::Panel *pParent ) const
{
	new CTFPainkitConsumeDialog( pItem );
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CGCEventEnableResponse : public GCSDK::CGCClientJob
{
public:
	CGCEventEnableResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}
	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgGC_Client_UseServerModificationItem_Response> msg( pNetPacket );

		switch ( msg.Body().response_code() )
		{
		case CMsgGC_Client_UseServerModificationItem_Response::kServerModificationItemResponse_AlreadyInUse:
			ShowMessageBox( "#TF_ServerEnchantmentType", "#TF_Eternaween__AlreadyInUse", (KeyValues *)NULL );
			break;

		case CMsgGC_Client_UseServerModificationItem_Response::kServerModificationItemResponse_NotOnAuthenticatedServer:
			ShowMessageBox( "#TF_ServerEnchantmentType", "#TF_Eternaween__AuthenticatedServerRequired", (KeyValues *)NULL );
			break;

		case CMsgGC_Client_UseServerModificationItem_Response::kServerModificationItemResponse_ServerReject:
			ShowMessageBox( "#TF_ServerEnchantmentType", "#TF_Eternaween__ServerReject", (KeyValues *)NULL );
			break;

		case CMsgGC_Client_UseServerModificationItem_Response::kServerModificationItemResponse_InternalError:
			ShowMessageBox( "#TF_ServerEnchantmentType", "#TF_Eternaween__InternalError", (KeyValues *)NULL );
			break;

		case CMsgGC_Client_UseServerModificationItem_Response::kServerModificationItemResponse_EventAlreadyActive:
			ShowMessageBox( "#TF_ServerEnchantmentType", "#TF_Eternaween__EventAlreadyActive", (KeyValues *)NULL );
			break;
		}
		
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCEventEnableResponse, "CGCEventEnableResponse", k_EMsgGC_Client_UseServerModificationItem_Response, GCSDK::k_EServerTypeGCClient );

// invoked when the local player attempts to consume the given item
void UseConsumableItem( CEconItemView *pItem, vgui::Panel *pParent )
{
	Assert( pItem );

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	const GameItemDefinition_t *pItemDef = pItem->GetStaticData();
	Assert( pItemDef );

	bool bUsableOutOfGame = (pItemDef->GetCapabilities() & ITEM_CAP_USABLE_OUT_OF_GAME) != 0;

	// if we aren't useable outside of the game then make sure that we're in a game and that
	// we have a local player we can use
	if ( !bUsableOutOfGame )
	{
		if ( !engine->IsInGame() )
		{
			ShowMessageBox( "#TF_UseFail_NotInGame_Title",  "#TF_UseFail_NotInGame", "#GameUI_OK" );
			return;
		}

		if ( pLocalPlayer == NULL )
			return;
	}
	
	// make sure this item meets our baseline useable criteria
	if ( pItem->GetItemQuantity() <= 0 )
		return;

	if ( !IsGCUseableItem( pItemDef ) )
		return;

	const IEconTool *pEconTool = pItemDef->GetEconTool();
	if ( !pEconTool )
		return;

	// do whatever client work needs to be done, send a request to the GC to use the item, etc.
	pEconTool->OnClientUseConsumable( pItem, pParent );
}

// Called from the trade dialog when the player selects a target user ID.
void UseGift( CEconItemView* pItem, CSteamID targetID )
{
	// Validate pItem...
	if ( !pItem )
		return;

	const GameItemDefinition_t *pItemDef = pItem->GetItemDefinition();
	if ( !pItemDef )
		return;

	if ( !IsGCUseableItem( pItemDef ) )
		return;

	if ( !pItemDef->GetTypedEconTool<CEconTool_WrappedGift>() )
		return;

	GCSDK::CGCMsg< MsgGCDeliverGift_t > msg( k_EMsgGCDeliverGift );
	msg.Body().m_unGiftID = pItem->GetItemID();
	msg.Body().m_ulTargetSteamID = targetID.ConvertToUint64();
	GCClientSystem()->BSendMessage( msg );
																	  
	CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();

	C_CTF_GameStats.Event_Trading( IE_TRADING_ITEM_GIFTED, pItem, true, 
		steamID.ConvertToUint64(), targetID.ConvertToUint64() );
}

class CDeliverGiftSelectDialog : public CSelectPlayerDialog
{
public:
	CDeliverGiftSelectDialog( vgui::Panel *parent );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	void SetItem( CEconItemView* pItem ) { m_pItem = pItem; }
	virtual bool AllowOutOfGameFriends() { return true; }

	virtual void OnSelectPlayer( const CSteamID &steamID )
	{
		UseGift( m_pItem, steamID );
	}

private:
	CEconItemView* m_pItem;
};

CDeliverGiftSelectDialog::CDeliverGiftSelectDialog( vgui::Panel *parent ) 
: CSelectPlayerDialog( parent )
{
}

void CDeliverGiftSelectDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	CSelectPlayerDialog::ApplySchemeSettings( pScheme );
	SetDialogVariable( "title", g_pVGuiLocalize->Find( "TF_DeliverGiftDialog_Title" ) );
}

static vgui::DHANDLE<CDeliverGiftSelectDialog> g_hDeliverGiftDialog;

CDeliverGiftSelectDialog *OpenDeliverGiftDialog( vgui::Panel *pParent, CEconItemView *pItem )
{
	if (!g_hDeliverGiftDialog.Get())
	{
		g_hDeliverGiftDialog = vgui::SETUP_PANEL( new CDeliverGiftSelectDialog( pParent ) );
	}
	g_hDeliverGiftDialog->InvalidateLayout( false, true );
	g_hDeliverGiftDialog->Reset();
	g_hDeliverGiftDialog->SetVisible( true );
	g_hDeliverGiftDialog->MakePopup();
	g_hDeliverGiftDialog->MoveToFront();
	g_hDeliverGiftDialog->SetKeyBoardInputEnabled(true);
	g_hDeliverGiftDialog->SetMouseInputEnabled(true);
	g_hDeliverGiftDialog->SetItem( pItem );
	TFModalStack()->PushModal( g_hDeliverGiftDialog );

	return g_hDeliverGiftDialog;
}

// This is the command the user will execute.
// We want this to happen on the client, before forwarding to the game server, since we don't trust
// the game server.
static bool g_bUsedGCItem = false;
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

	// If we're in Mann Vs MAchine, and we're dead, we can use this to respawn instantly.
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && pLocalPlayer->IsObserver() )
	{
		float flNextRespawn = TFGameRules()->GetNextRespawnWave( pLocalPlayer->GetTeamNumber(), pLocalPlayer );
		if ( flNextRespawn )
		{
			int iRespawnWait = (flNextRespawn - gpGlobals->curtime);
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
		CTFGrapplingHook *pGrapplingHook = dynamic_cast< CTFGrapplingHook* >( pLocalPlayer->GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
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

	// send a request to the GC to use the item
	g_bUsedGCItem = false;
	CEconItemView *pItem = CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot( pLocalPlayer, LOADOUT_POSITION_ACTION );
	if( pItem )
	{
		const IEconTool *pEconTool = pItem->GetItemDefinition()->GetEconTool();
		bool bIsRecharger = ( pEconTool && FStrEq( pEconTool->GetTypeName(), "item_eater_recharger" ) );
		if ( IsGCUseableItem( pItem->GetItemDefinition() ) && pItem->GetItemQuantity() >= 1 && !bIsRecharger )
		{
			UseConsumableItem( pItem, NULL );
			g_bUsedGCItem = true;
		}
	}
	
	// otherwise, forward to game server
	if ( !g_bUsedGCItem )
	{
		KeyValues *kv = new KeyValues( "+use_action_slot_item_server" );
		engine->ServerCmdKeyValues( kv );
	}
}

static ConCommand start_use_action_slot_item( "+use_action_slot_item", StartUseActionSlotItem, "Use the item in the action slot." );

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

			C_BaseCombatWeapon* pLastWeapon = pLocalPlayer->GetLastWeapon();

			// switch away from the hook
			if ( pLastWeapon && pLocalPlayer->Weapon_CanSwitchTo( pLastWeapon ) )
			{
				pLocalPlayer->Weapon_Switch( pLastWeapon );
			}
			else
			{
				// in case we failed to switch back to last weapon for some reason, just find the next best
				pLocalPlayer->SwitchToNextBestWeapon( pLastWeapon );
			}

			return;
		}
	}

	// tell the game server we let go of the button if this wasn't a GC item
	if ( !g_bUsedGCItem )
	{
		KeyValues *kv = new KeyValues( "-use_action_slot_item_server" );
		engine->ServerCmdKeyValues( kv );
	}
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
			CTFPowerupBottle *pPowerupBottle = dynamic_cast< CTFPowerupBottle* >( pLocalPlayer->GetEquippedWearableForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
			if ( pPowerupBottle && pPowerupBottle->GetNumCharges() > 0 )
			{
				// They're in MvM and have a bottle with a charge, so do an action instead
				bDoTaunt = false;
			}

			if ( pLocalPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) && pLocalPlayer->GetActiveTFWeapon() && pLocalPlayer->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_MINIGUN )
			{
				int iRage = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pLocalPlayer, iRage, generate_rage_on_dmg );
				if ( iRage )
				{
					if ( pLocalPlayer->m_Shared.GetRageMeter() >= 100.f && !pLocalPlayer->m_Shared.IsRageDraining() )
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

//-----------------------------------------------------------------------------

class CTFGiftNotification : public CEconNotification
{
public:
	CTFGiftNotification( GCSDK::CProtoBufMsg<CMsgGCGiftedItems> &msg ) 
		: CEconNotification() 
	{
		const EUniverse eUniverse = GetUniverse();

		Assert( msg.Body().recipient_account_ids_size() > 0 );

		SetLifetime( 30.0f );

		m_bRandomPerson = msg.Body().has_was_random_person()
					   && msg.Body().was_random_person();

		const CSteamID gifterSteamID( msg.Body().gifter_steam_id(), eUniverse, k_EAccountTypeIndividual );
		SetSteamID( gifterSteamID );

		if ( m_bRandomPerson )
		{
			const CSteamID recipientSteamID( msg.Body().recipient_account_ids(0), eUniverse, k_EAccountTypeIndividual );
			if ( msg.Body().recipient_account_ids_size() > 0 )
			{
				// This might not really be a random gift but might instead be a gift they opened
				// themselves (ie., a shipment box).
				if ( recipientSteamID == gifterSteamID )
				{
					SetText( "#TF_GifterText_SelfOpen" );
				}
				else
				{
					SetText( "#TF_GifterText_Random" );

					char szRecipientName[ MAX_PLAYER_NAME_LENGTH ];
					GetPlayerNameBySteamID( recipientSteamID, szRecipientName, sizeof( szRecipientName ) );
					g_pVGuiLocalize->ConvertANSIToUnicode( szRecipientName, m_wszPlayerName, sizeof( m_wszPlayerName ) );
					AddStringToken( "recipient", m_wszPlayerName );
					m_vecSteamIDRecipients.AddToTail( recipientSteamID );
				}
			}
		}
		else
		{
			SetText( "#TF_GifterText_All" );

			for ( int i = 0; i < msg.Body().recipient_account_ids_size(); ++i )
			{
				const CSteamID recipientSteamID( msg.Body().recipient_account_ids(i), eUniverse, k_EAccountTypeIndividual );
				m_vecSteamIDRecipients.AddToTail( recipientSteamID );
			}
		}
		
		char szGifterName[ MAX_PLAYER_NAME_LENGTH ];
		GetPlayerNameBySteamID( m_steamID, szGifterName, sizeof( szGifterName ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( szGifterName, m_wszPlayerName, sizeof( m_wszPlayerName ) );
		AddStringToken( "giver", m_wszPlayerName );

		PrintToChatLog();
		
		SetSoundFilename( "misc/happy_birthday.wav" );
	}

	void PrintToChatLog()
	{
		CBaseHudChat *pHUDChat = (CBaseHudChat *)GET_HUDELEMENT( CHudChat );
		if ( pHUDChat )
		{
			wchar_t *pFormat = g_pVGuiLocalize->Find( "TF_GiftedItems" );
			if ( pFormat == NULL )
			{
				return;
			}
			FOR_EACH_VEC( m_vecSteamIDRecipients, i )
			{
				const CSteamID &steamIDRecipient = m_vecSteamIDRecipients[i];
				char szRecipientName[ MAX_PLAYER_NAME_LENGTH ];
				szRecipientName[0] = '\0';
				GetPlayerNameBySteamID( steamIDRecipient, szRecipientName, sizeof( szRecipientName ) );
				if ( szRecipientName[0] == '\0' )
				{
					continue;
				}

				wchar_t wszRecipientName[MAX_PLAYER_NAME_LENGTH];
				g_pVGuiLocalize->ConvertANSIToUnicode( szRecipientName, wszRecipientName, sizeof( wszRecipientName ) );

				wchar_t wszNotification[1024]=L"";
				g_pVGuiLocalize->ConstructString_safe( wszNotification, 
												  pFormat,
												  2, m_wszPlayerName, wszRecipientName );

				char szAnsi[1024];
				g_pVGuiLocalize->ConvertUnicodeToANSI( wszNotification, szAnsi, sizeof(szAnsi) );
			
				pHUDChat->Printf( CHAT_FILTER_NONE, "%s", szAnsi );
			}
		}
	}

	virtual EType NotificationType() { return eType_Basic; }

	wchar_t m_wszPlayerName[ MAX_PLAYER_NAME_LENGTH ];
	bool m_bRandomPerson;
	CUtlVector< CSteamID > m_vecSteamIDRecipients;
};

//#ifdef _DEBUG
//CON_COMMAND( cl_gifts_test, "tests the gift ui." )
//{
//	if ( !engine->IsInGame() )
//	{
//		return;
//	}
//
//	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
//	if ( pLocalPlayer == NULL )
//	{
//		return;
//	}
//
//	if ( steamapicontext == NULL || steamapicontext->SteamUser() == NULL )
//	{
//		return;
//	}
//
//	CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
//	GCSDK::CProtoBufMsg< CMsgGCGiftedItems > msg( k_EMsgGCGiftedItems );
//	msg.Body().m_ulGifterSteamID = steamID.ConvertToUint64();
//	msg.Body().m_bRandomPerson = ( args.ArgC() >= 2 );
//	msg.Body().m_unNumGiftRecipients = msg.Body().m_bRandomPerson ? 1 : 31;
//	for ( int i = 0; i < msg.Body().m_unNumGiftRecipients; ++i )
//	{
//		msg.AddUint64Data( steamID.ConvertToUint64() );
//	}
//	msg.ResetReadPtr();
//	NotificationQueue_Add( new CTFGiftNotification( msg ) );
//}
//#endif

//-----------------------------------------------------------------------------
// Purpose: Feedback to the local player who used an item
//-----------------------------------------------------------------------------
class CTFUseItemNotification : public CEconNotification
{
public:
	CTFUseItemNotification( EGCMsgUseItemResponse eResponse) 
		: CEconNotification() 
	{
		switch ( eResponse )
		{
		case k_EGCMsgUseItemResponse_ItemUsed: 		
			SetText( "#TF_UseItem_Success" );
			break;
		case k_EGCMsgUseItemResponse_GiftNoOtherPlayers:
			SetText( "#TF_UseItem_GiftNoPlayers" );
			break;
		case k_EGCMsgUseItemResponse_ServerError:
			SetText( "#TF_UseItem_Error" );
			break;
		case k_EGCMsgUseItemResponse_MiniGameAlreadyStarted:
			SetText( "#TF_UseItem_MiniGameAlreadyStarted" );
			break;
		case k_EGCMsgUseItemResponse_CannotBeUsedByAccount:
			SetText( "#TF_UseItem_CannotBeUsedByAccount" );
			break;
		case k_EGCMsgUseItemResponse_CannotUseWhileUntradable:
			SetText( "#TF_UseItem_CannotUseWhileUntradable" );
			break;
		case k_EGCMsgUseItemResponse_RecipientCannotRecieve:
			SetText( "#TF_UseItem_RecipientCannotRecieve" );
			break;
		default:
			Assert( !"Unknown response in CTFUseItemNotification!" );
		}
		SetLifetime( 20.0f );
	}

	virtual EType NotificationType() { return eType_Basic; }
};

//-----------------------------------------------------------------------------
// Purpose: Local player used an item and the GC responded with the status of that request
//-----------------------------------------------------------------------------
class CGCUseItemResponse : public GCSDK::CGCClientJob
{
public:
	CGCUseItemResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}
	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCUseItemResponse_t> msg( pNetPacket );
		EGCMsgUseItemResponse eResponse = (EGCMsgUseItemResponse)msg.Body().m_eResponse;
		if ( eResponse == k_EGCMsgUseItemResponse_ItemUsed_ItemsGranted )
		{
			if ( s_bConsumableToolOpeningGift )
			{
				vgui::surface()->PlaySound( "ui/item_gift_wrap_unwrap.wav" );
			}
			else
			{
				vgui::surface()->PlaySound( "ui/item_open_crate.wav" );
			}

			ShowWaitingDialog( new CWaitForPackageDialog( NULL ), "#ToolDecodeInProgress", true, false, 5.0f );
		}
		else if ( eResponse != k_EGCMsgUseItemResponse_ItemUsed )
		{
			NotificationQueue_Add( new CTFUseItemNotification( (EGCMsgUseItemResponse)msg.Body().m_eResponse ) );
		}
		else
		{
			// refresh the backpack
			if ( EconUI()->GetBackpackPanel() )
			{
				EconUI()->GetBackpackPanel()->UpdateModelPanels();
			}
		}
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCUseItemResponse, "CGCUseItemResponse", k_EMsgGCUseItemResponse, GCSDK::k_EServerTypeGCClient );

//-----------------------------------------------------------------------------
// Purpose: A player has gifted items
//-----------------------------------------------------------------------------
class CGCGiftedItems : public GCSDK::CGCClientJob
{
public:
	CGCGiftedItems( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}
	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgGCGiftedItems> msg( pNetPacket );

		if ( steamapicontext == NULL || steamapicontext->SteamFriends() == NULL )
		{
			return true;
		}

		char szGifterName[ MAX_PLAYER_NAME_LENGTH ];
		szGifterName[0] = '\0';
		GetPlayerNameBySteamID( CSteamID( msg.Body().gifter_steam_id(), GetUniverse(), k_EAccountTypeIndividual ), szGifterName, sizeof( szGifterName ) );
		if ( szGifterName[0] == '\0' )
		{
			return true;
		}
		
		// notify UI
		NotificationQueue_Add( new CTFGiftNotification( msg ) );
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCGiftedItems, "CGCGiftedItems", k_EMsgGCGiftedItems, GCSDK::k_EServerTypeGCClient );

//-----------------------------------------------------------------------------
// Purpose: A player has used a claim code item
//-----------------------------------------------------------------------------
class CGCUsedClaimCodeItem : public GCSDK::CGCClientJob
{
public:
	CGCUsedClaimCodeItem( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}
	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCUsedClaimCodeItem_t> msg( pNetPacket );

		if ( steamapicontext == NULL )
		{
			return true;
		}

		CUtlString url;
		if ( msg.BReadStr( &url ) )
		{
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( url.Get() );
			IViewPortPanel *pMMOverride = ( gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE ) );
			if ( pMMOverride )
			{
				((CHudMainMenuOverride*)pMMOverride)->UpdatePromotionalCodes();
			}
		}

		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCUsedClaimCodeItem, "CGCUsedClaimCodeItem", k_EMsgGCUsedClaimCodeItem, GCSDK::k_EServerTypeGCClient );


//-----------------------------------------------------------------------------
// Duel mini-game 
//-----------------------------------------------------------------------------

class CDuelMiniGameEventListener;

struct duel_minigame_local_data_t
{
	duel_minigame_local_data_t()
		: m_pEventListener( NULL )
		, m_steamIDOpponent()
		, m_unMyScore( 0 )
		, m_unOpponentScore( 0 )
		, m_iRequiredPlayerClass( TF_CLASS_UNDEFINED )
	{
	}
	uint32 m_unMyScore;
	uint32 m_unOpponentScore;
	int m_iRequiredPlayerClass;
	CDuelMiniGameEventListener *m_pEventListener;
	CSteamID m_steamIDOpponent;
};
static duel_minigame_local_data_t gDuelMiniGameLocalData;

bool DuelMiniGame_IsDueling()
{
	return gDuelMiniGameLocalData.m_steamIDOpponent != CSteamID();
}

int DuelMiniGame_GetRequiredPlayerClass()
{
	return gDuelMiniGameLocalData.m_steamIDOpponent != CSteamID() ? gDuelMiniGameLocalData.m_iRequiredPlayerClass : TF_CLASS_UNDEFINED;
}

static void DuelMiniGame_Reset();
static bool RemoveRelatedDuelNotifications( CEconNotification* pNotification );

/**
 * Duel info notification
 */
class CTFDuelInfoNotification : public CEconNotification
{
public:
	CTFDuelInfoNotification()
	{
	}

	static bool IsDuelInfoNotification( CEconNotification *pNotification )
	{
		return dynamic_cast< CTFDuelInfoNotification* >( pNotification ) != NULL;
	}
};

/**
 * Duel Notification
 */
class CTFDuelRequestNotification : public CEconNotification, public CGameEventListener
{
public:
	CTFDuelRequestNotification( const char *pInitiatorName, const CSteamID &steamIDInitiator, const CSteamID &steamIDTarget, const int iRequiredPlayerClass ) 
		: CEconNotification() 
		, m_steamIDInitiator( steamIDInitiator )
		, m_steamIDTarget( steamIDTarget )
		, m_iRequiredPlayerClass( iRequiredPlayerClass )
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( pInitiatorName, m_wszPlayerName, sizeof(m_wszPlayerName) );

		ListenForGameEvent( "teamplay_round_win" );
		ListenForGameEvent( "teamplay_round_stalemate" );
	}

	virtual EType NotificationType()
	{
		CSteamID localSteamID;
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer && pLocalPlayer->GetSteamID( &localSteamID ) && localSteamID == m_steamIDTarget )
		{
			return eType_AcceptDecline;
		}
		return eType_Basic;
	}

	// XXX(JohnS): Is there something that manually calls trigger here or is this dead code?
	virtual void Trigger()
	{
		CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( "#TF_Duel_Title",  "#TF_Duel_Request", "#GameUI_OK", "#TF_Duel_JoinCancel", &ConfirmDuel );
		pDialog->SetContext( this );
		pDialog->AddStringToken( "initiator", m_wszPlayerName );
		// so we aren't deleted
		SetIsInUse( true );
	}

	virtual void Accept()
	{
		ConfirmDuel( true, this );
	}

	virtual void Decline()
	{
		ConfirmDuel( false, this );
	}

	static bool IsDuelRequestNotification( CEconNotification *pNotification )
	{
		return dynamic_cast< CTFDuelRequestNotification* >( pNotification ) != NULL;
	}

	static void ConfirmDuel( bool bConfirmed, void *pContext )
	{
		CTFDuelRequestNotification *pNotification = (CTFDuelRequestNotification*)pContext;

		// if this is a class restricted duel, then make sure the local player is the same class before
		// they can accept
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( bConfirmed && pLocalPlayer && pNotification->m_iRequiredPlayerClass != TF_CLASS_UNDEFINED )
		{
			int iClass = pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex();
			if ( pNotification->m_iRequiredPlayerClass != iClass )
			{
				KeyValues *pKeyValues = new KeyValues( "DuelConfirm" );
				switch ( pNotification->m_iRequiredPlayerClass )
				{
				case TF_CLASS_SCOUT: 			pKeyValues->SetWString( "player_class", g_pVGuiLocalize->Find( "#TF_Class_Name_Scout" ) ); break;
				case TF_CLASS_SNIPER: 			pKeyValues->SetWString( "player_class", g_pVGuiLocalize->Find( "#TF_Class_Name_Sniper" ) ); break;
				case TF_CLASS_SOLDIER: 			pKeyValues->SetWString( "player_class", g_pVGuiLocalize->Find( "#TF_Class_Name_Soldier" ) ); break;
				case TF_CLASS_DEMOMAN: 			pKeyValues->SetWString( "player_class", g_pVGuiLocalize->Find( "#TF_Class_Name_Demoman" ) ); break;
				case TF_CLASS_MEDIC: 			pKeyValues->SetWString( "player_class", g_pVGuiLocalize->Find( "#TF_Class_Name_Medic" ) ); break;
				case TF_CLASS_HEAVYWEAPONS: 	pKeyValues->SetWString( "player_class", g_pVGuiLocalize->Find( "#TF_Class_Name_HWGuy" ) ); break;
				case TF_CLASS_PYRO: 			pKeyValues->SetWString( "player_class", g_pVGuiLocalize->Find( "#TF_Class_Name_Pyro" ) ); break;
				case TF_CLASS_SPY: 				pKeyValues->SetWString( "player_class", g_pVGuiLocalize->Find( "#TF_Class_Name_Spy" ) ); break;
				case TF_CLASS_ENGINEER: 		pKeyValues->SetWString( "player_class", g_pVGuiLocalize->Find( "#TF_Class_Name_Engineer" ) ); break;
				}
				ShowMessageBox( "#TF_Duel_Title",  "#TF_Duel_WrongClass", pKeyValues, "#GameUI_OK" );
				return;
			}
		}

		// notify GC of our choice
		GCSDK::CGCMsg< MsgGC_Duel_Response_t > msg( k_EMsgGC_Duel_Response );
		msg.Body().m_ulInitiatorSteamID = pNotification->m_steamIDInitiator.ConvertToUint64();
		msg.Body().m_ulTargetSteamID = pNotification->m_steamIDTarget.ConvertToUint64();
		msg.Body().m_bAccepted = bConfirmed;
		GCClientSystem()->BSendMessage( msg );
		pNotification->SetIsInUse( false );
		pNotification->MarkForDeletion();

		// remove all duel notifications if we've accepted
		if ( bConfirmed )
		{
			NotificationQueue_Remove( &RemoveRelatedDuelNotifications );
		}
	}

	void FireGameEvent( IGameEvent *event )
	{
		const char *pEventName = event->GetName();
		if ( FStrEq( "teamplay_round_win", pEventName ) || FStrEq( "teamplay_round_stalemate", pEventName ) )
		{
			Decline();
			return;
		}
	}

	CSteamID m_steamIDInitiator;
	CSteamID m_steamIDTarget;
	int m_iRequiredPlayerClass;
	wchar_t m_wszPlayerName[MAX_PLAYER_NAME_LENGTH];
};

/**
 * Listens for duel_status events and adds a notification.  Ideally adds to a scoreboard or something...
 */
class CDuelMiniGameEventListener : public CGameEventListener
{
public:
	CDuelMiniGameEventListener()
	{
		ListenForGameEvent( "duel_status" );
		ListenForGameEvent( "teamplay_round_win" );
		ListenForGameEvent( "teamplay_round_stalemate" );
	}

	virtual void FireGameEvent( IGameEvent *event )
	{
		const char *pEventName = event->GetName();

		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer == NULL )
		{
			return;
		}

		if ( Q_strcmp( "teamplay_round_win", pEventName ) == 0 || Q_strcmp( "teamplay_round_stalemate", pEventName ) == 0 )
		{
			DuelMiniGame_Reset();
			return;
		}
		else if ( Q_strcmp( "duel_status", pEventName ) == 0 )
		{
			int iKillerID = engine->GetPlayerForUserID( event->GetInt( "killer" ) );
			int iInitiatorID = engine->GetPlayerForUserID( event->GetInt( "initiator" ) );
			int iTargetID = engine->GetPlayerForUserID( event->GetInt( "target" ) );

			const char *pInitiatorName = ( iInitiatorID > 0 ? g_PR->GetPlayerName( iInitiatorID ) : "" );
			wchar_t wszInitiatorName[MAX_PLAYER_NAME_LENGTH] = L"";
			g_pVGuiLocalize->ConvertANSIToUnicode( pInitiatorName, wszInitiatorName, sizeof(wszInitiatorName) );

			const char *pTargetName = ( iTargetID > 0 ? g_PR->GetPlayerName( iTargetID ) : "" );
			wchar_t wszTargetName[MAX_PLAYER_NAME_LENGTH] = L"";
			g_pVGuiLocalize->ConvertANSIToUnicode( pTargetName, wszTargetName, sizeof(wszTargetName) );

			wchar_t wszInitiatorScore[16];
			_snwprintf( wszInitiatorScore, ARRAYSIZE( wszInitiatorScore ), L"%i", event->GetInt( "initiator_score", 0 ) );
			wchar_t wszTargetScore[16];
			_snwprintf( wszTargetScore, ARRAYSIZE( wszTargetScore ), L"%i", event->GetInt( "target_score", 0 ) );

			enum
			{
				kDuelScoreType_Kill,
				kDuelScoreType_Assist,
				kMaxDuelScoreTypes,
			};
			
			int iScoreType = event->GetInt( "score_type" );

			KeyValues *pKeyValues = new KeyValues( "DuelStatus" );
			pKeyValues->SetWString( "killer", iKillerID == iInitiatorID ? wszInitiatorName : wszTargetName );
			pKeyValues->SetWString( "initiator", wszInitiatorName );
			pKeyValues->SetWString( "target", wszTargetName );
			pKeyValues->SetWString( "initiator_score", wszInitiatorScore );
			pKeyValues->SetWString( "target_score", wszTargetScore );
			
			// if we aren't involved in the duel, don't show the notification
			if ( pLocalPlayer->entindex() == iInitiatorID || pLocalPlayer->entindex() == iTargetID )
			{
				// remove existing duel info notifications
				NotificationQueue_Remove( &RemoveRelatedDuelNotifications );
				// add new one
				CTFDuelInfoNotification *pNotification = new CTFDuelInfoNotification();
				pNotification->SetLifetime( 10.0f );
				pNotification->SetText( iScoreType == kDuelScoreType_Kill ? "TF_Duel_StatusKill" : "TF_Duel_StatusAssist" );
				pNotification->SetKeyValues( pKeyValues );
				pNotification->SetSoundFilename( "ui/duel_event.wav" );
				player_info_t pi;
				if ( engine->GetPlayerInfo( iKillerID, &pi ) && pi.friendsID != 0 )
				{
					CSteamID steamID( pi.friendsID, 1, GetUniverse(), k_EAccountTypeIndividual );
					pNotification->SetSteamID( steamID );
				}
				NotificationQueue_Add( pNotification );
				if ( pLocalPlayer->entindex() == iInitiatorID )
				{
					gDuelMiniGameLocalData.m_unMyScore = event->GetInt( "initiator_score" );
					gDuelMiniGameLocalData.m_unOpponentScore = event->GetInt( "target_score" );
				}
				else
				{
					gDuelMiniGameLocalData.m_unMyScore = event->GetInt( "target_score" );
					gDuelMiniGameLocalData.m_unOpponentScore = event->GetInt( "initiator_score" );
				}
			}

			// print to chat log
			PrintTextToChat( iScoreType == kDuelScoreType_Kill ? "TF_Duel_StatusForChat_Kill" : "TF_Duel_StatusForChat_Assist", pKeyValues );

			// cleanup
			pKeyValues->deleteThis();
		}
	}
};

/**
 * Duel request
 */
class CGC_Duel_Request : public GCSDK::CGCClientJob
{
public:
	CGC_Duel_Request( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}
	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGC_Duel_Request_t> msg( pNetPacket );
		CSteamID localSteamID;
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer == NULL || pLocalPlayer->GetSteamID( &localSteamID ) == false )
		{
			return true;
		}

		// get player names
		CSteamID steamIDInitiator( msg.Body().m_ulInitiatorSteamID );
		CSteamID steamIDTarget( msg.Body().m_ulTargetSteamID );

		// ignore blocked players (we don't want to print out to the console either)
		if ( IgnoreRequestFromUser( steamIDInitiator ) || IgnoreRequestFromUser( steamIDTarget ) )
		{
			GCSDK::CGCMsg< MsgGC_Duel_Response_t > msgGC( k_EMsgGC_Duel_Response );
			msgGC.Body().m_ulInitiatorSteamID = steamIDInitiator.ConvertToUint64();
			msgGC.Body().m_ulTargetSteamID = steamIDTarget.ConvertToUint64();
			msgGC.Body().m_bAccepted = false;
			GCClientSystem()->BSendMessage( msgGC );
			return true;
		}

		char szPlayerName_Initiator[ MAX_PLAYER_NAME_LENGTH ];
		GetPlayerNameBySteamID( steamIDInitiator, szPlayerName_Initiator, sizeof( szPlayerName_Initiator ) );
		char szPlayerName_Target[ MAX_PLAYER_NAME_LENGTH ];
		GetPlayerNameBySteamID( steamIDTarget, szPlayerName_Target, sizeof( szPlayerName_Target ) );
		wchar_t wszPlayerName_Initiator[MAX_PLAYER_NAME_LENGTH];
		wchar_t wszPlayerName_Target[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode( szPlayerName_Initiator, wszPlayerName_Initiator, sizeof(wszPlayerName_Initiator) );
		g_pVGuiLocalize->ConvertANSIToUnicode( szPlayerName_Target, wszPlayerName_Target, sizeof(wszPlayerName_Target) );

		// add notification
		KeyValues *pKeyValues = new KeyValues( "DuelText" );
		pKeyValues->SetWString( "initiator", wszPlayerName_Initiator );
		pKeyValues->SetWString( "target", wszPlayerName_Target );

		const char *pText = localSteamID == steamIDTarget ? "TF_Duel_Request" : "TF_Duel_Challenge";
		const char *pSoundFilename = "ui/duel_challenge.wav";
		if ( msg.Body().m_usAsPlayerClass >= TF_FIRST_NORMAL_CLASS && msg.Body().m_usAsPlayerClass < TF_LAST_NORMAL_CLASS )
		{
			pText = localSteamID == steamIDTarget ? "TF_Duel_Request_Class" : "TF_Duel_Challenge_Class";
			pSoundFilename = "ui/duel_challenge_with_restriction.wav";
			switch ( msg.Body().m_usAsPlayerClass )
			{
			case TF_CLASS_SCOUT: 			pKeyValues->SetWString( "player_class", g_pVGuiLocalize->Find( "#TF_Class_Name_Scout" ) ); break;
			case TF_CLASS_SNIPER: 			pKeyValues->SetWString( "player_class", g_pVGuiLocalize->Find( "#TF_Class_Name_Sniper" ) ); break;
			case TF_CLASS_SOLDIER: 			pKeyValues->SetWString( "player_class", g_pVGuiLocalize->Find( "#TF_Class_Name_Soldier" ) ); break;
			case TF_CLASS_DEMOMAN: 			pKeyValues->SetWString( "player_class", g_pVGuiLocalize->Find( "#TF_Class_Name_Demoman" ) ); break;
			case TF_CLASS_MEDIC: 			pKeyValues->SetWString( "player_class", g_pVGuiLocalize->Find( "#TF_Class_Name_Medic" ) ); break;
			case TF_CLASS_HEAVYWEAPONS: 	pKeyValues->SetWString( "player_class", g_pVGuiLocalize->Find( "#TF_Class_Name_HWGuy" ) ); break;
			case TF_CLASS_PYRO: 			pKeyValues->SetWString( "player_class", g_pVGuiLocalize->Find( "#TF_Class_Name_Pyro" ) ); break;
			case TF_CLASS_SPY: 				pKeyValues->SetWString( "player_class", g_pVGuiLocalize->Find( "#TF_Class_Name_Spy" ) ); break;
			case TF_CLASS_ENGINEER: 		pKeyValues->SetWString( "player_class", g_pVGuiLocalize->Find( "#TF_Class_Name_Engineer" ) ); break;
			}
		}
		if ( localSteamID == steamIDInitiator || localSteamID == steamIDTarget )
		{
			CTFDuelRequestNotification *pNotification = new CTFDuelRequestNotification( szPlayerName_Initiator, steamIDInitiator, steamIDTarget, msg.Body().m_usAsPlayerClass );
			pNotification->SetLifetime( localSteamID == steamIDTarget ? 30.0f : 7.0f );
			pNotification->SetText( pText );
			pNotification->SetKeyValues( pKeyValues );
			pNotification->SetSteamID( steamIDInitiator );
			pNotification->SetSoundFilename( pSoundFilename );
			NotificationQueue_Add( pNotification );
		}

		// print to chat log
		PrintTextToChat( pText, pKeyValues );

		pKeyValues->deleteThis();

		return true;
	}
protected:
};
GC_REG_JOB( GCSDK::CGCClient, CGC_Duel_Request, "CGC_Duel_Request", k_EMsgGC_Duel_Request, GCSDK::k_EServerTypeGCClient );

/**
 * Duel response
 */
class CGCClient_Duel_Response : public GCSDK::CGCClientJob
{
public:
	CGCClient_Duel_Response( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}
	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGC_Duel_Response_t> msg( pNetPacket );
		CSteamID localSteamID;
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer == NULL || pLocalPlayer->GetSteamID( &localSteamID ) == false )
			return true;

		// get player names
		CSteamID steamIDInitiator( msg.Body().m_ulInitiatorSteamID );
		CSteamID steamIDTarget( msg.Body().m_ulTargetSteamID );
		char szPlayerName_Initiator[ MAX_PLAYER_NAME_LENGTH ];
		GetPlayerNameBySteamID( steamIDInitiator, szPlayerName_Initiator, sizeof( szPlayerName_Initiator ) );
		char szPlayerName_Target[ MAX_PLAYER_NAME_LENGTH ];
		GetPlayerNameBySteamID( steamIDTarget, szPlayerName_Target, sizeof( szPlayerName_Target ) );
		wchar_t wszPlayerName_Initiator[MAX_PLAYER_NAME_LENGTH];
		wchar_t wszPlayerName_Target[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode( szPlayerName_Initiator, wszPlayerName_Initiator, sizeof(wszPlayerName_Initiator) );
		g_pVGuiLocalize->ConvertANSIToUnicode( szPlayerName_Target, wszPlayerName_Target, sizeof(wszPlayerName_Target) );

		KeyValues *pKeyValues = new KeyValues( "DuelText" );
		pKeyValues->SetWString( "initiator", wszPlayerName_Initiator );
		pKeyValues->SetWString( "target", wszPlayerName_Target );

		bool bIsClassDuel = msg.Body().m_usAsPlayerClass >= TF_FIRST_NORMAL_CLASS && msg.Body().m_usAsPlayerClass < TF_LAST_NORMAL_CLASS;

		// add notification
		const char *pText = "TF_Duel_Accept";
		const char *pSoundFilename = bIsClassDuel ? "ui/duel_challenge_accepted_with_restriction.wav" : "ui/duel_challenge_accepted.wav";
		if ( msg.Body().m_bAccepted == false )
		{
			const char *kDeclineStrings[] = {
				"TF_Duel_Decline",
				"TF_Duel_Decline2",
				"TF_Duel_Decline3"
			};
			pText = kDeclineStrings[ RandomInt( 0, ARRAYSIZE( kDeclineStrings ) - 1 ) ];
			pSoundFilename = bIsClassDuel ? "ui/duel_challenge_rejected_with_restriction.wav" : "ui/duel_challenge_rejected.wav";
		}

		if ( ( TFGameRules() && TFGameRules()->CanInitiateDuels() ) || msg.Body().m_bAccepted )
		{
			if ( localSteamID == steamIDInitiator || localSteamID == steamIDTarget )
			{
				// remove existing duel info notifications
				NotificationQueue_Remove( &RemoveRelatedDuelNotifications );
				// add new one
				CTFDuelInfoNotification *pNotification = new CTFDuelInfoNotification();
				pNotification->SetLifetime( 7.0f );
				pNotification->SetText( pText );
				pNotification->SetKeyValues( pKeyValues );
				pNotification->SetSteamID( steamIDInitiator );
				pNotification->SetSoundFilename( pSoundFilename );
				NotificationQueue_Add( pNotification );
			}

			// print to chat
			PrintTextToChat( pText, pKeyValues );
		}

		// store away opponent id and create event listener if applicable
		if ( msg.Body().m_bAccepted )
		{
			if ( localSteamID == steamIDInitiator )
			{
				gDuelMiniGameLocalData.m_steamIDOpponent = steamIDTarget;
			}
			else if ( localSteamID == steamIDTarget )
			{
				gDuelMiniGameLocalData.m_steamIDOpponent = steamIDInitiator;
			}
			if ( gDuelMiniGameLocalData.m_pEventListener == NULL )
			{
				gDuelMiniGameLocalData.m_pEventListener = new CDuelMiniGameEventListener();
			}
			gDuelMiniGameLocalData.m_iRequiredPlayerClass = msg.Body().m_usAsPlayerClass;
		}
		
		pKeyValues->deleteThis();

		return true;
	}
protected:
};
GC_REG_JOB( GCSDK::CGCClient, CGCClient_Duel_Response, "CGCClient_Duel_Response", k_EMsgGC_Duel_Response, GCSDK::k_EServerTypeGCClient );

/**
 * Duel status (whether the duel is in progress or cancelled)
 */
class CGC_Duel_Status : public GCSDK::CGCClientJob
{
public:
	CGC_Duel_Status( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}
	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGC_Duel_Status_t> msg( pNetPacket );
		// get player names
		CSteamID steamIDInitiator( msg.Body().m_ulInitiatorSteamID );
		CSteamID steamIDTarget( msg.Body().m_ulTargetSteamID );
		char szPlayerName_Initiator[ MAX_PLAYER_NAME_LENGTH ];
		GetPlayerNameBySteamID( steamIDInitiator, szPlayerName_Initiator, sizeof( szPlayerName_Initiator ) );
		char szPlayerName_Target[ MAX_PLAYER_NAME_LENGTH ];
		GetPlayerNameBySteamID( steamIDTarget, szPlayerName_Target, sizeof( szPlayerName_Target ) );
		wchar_t wszPlayerName_Initiator[MAX_PLAYER_NAME_LENGTH];
		wchar_t wszPlayerName_Target[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode( szPlayerName_Initiator, wszPlayerName_Initiator, sizeof(wszPlayerName_Initiator) );
		g_pVGuiLocalize->ConvertANSIToUnicode( szPlayerName_Target, wszPlayerName_Target, sizeof(wszPlayerName_Target) );

		// add notification
		const char *pText = "TF_Duel_InProgress";
		switch ( msg.Body().m_usStatus )
		{
		case kDuel_Status_AlreadyInDuel_Inititator: 
			pText = "TF_Duel_InADuel_Initiator"; 
			break;
		case kDuel_Status_AlreadyInDuel_Target: 
			pText = "TF_Duel_InADuel_Target"; 
			break;
		case kDuel_Status_DuelBanned_Initiator:
			pText = "TF_Duel_TempBanned_Initiator"; 
			break;
		case kDuel_Status_DuelBanned_Target:
			pText = "TF_Duel_TempBanned_Target"; 
			break;
		case kDuel_Status_Cancelled:
			pText = "TF_Duel_Cancelled";
			break;
		case kDuel_Status_MissingSession:
			pText = "TF_Duel_UserTemporarilyUnavailable";
			break;
		default:
			AssertMsg1( false, "Unknown duel status %i in CGC_Duel_Status!", msg.Body().m_usStatus );
		}
		// remove existing duel info notifications
		NotificationQueue_Remove( &RemoveRelatedDuelNotifications );
		// add new one
		CTFDuelInfoNotification *pNotification = new CTFDuelInfoNotification();
		pNotification->SetLifetime( 7.0f );
		pNotification->SetText( pText );
		pNotification->AddStringToken( "initiator", wszPlayerName_Initiator );
		pNotification->AddStringToken( "target", wszPlayerName_Target );
		pNotification->SetSteamID( steamIDInitiator );
		pNotification->SetSoundFilename( "ui/duel_event.wav" );
		NotificationQueue_Add( pNotification );
		return true;
	}
protected:
};
GC_REG_JOB( GCSDK::CGCClient, CGC_Duel_Status, "CGC_Duel_Status", k_EMsgGC_Duel_Status, GCSDK::k_EServerTypeGCClient );

/**
 * Duel Results--ideally this is a scoreboard as well.
 */
class CGC_Duel_Results : public GCSDK::CGCClientJob
{
public:
	CGC_Duel_Results( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}
	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGC_Duel_Results_t> msg( pNetPacket );

		if ( steamapicontext == NULL || steamapicontext->SteamUser() == NULL )
			return true;
		
		CSteamID localSteamID = steamapicontext->SteamUser()->GetSteamID();

		// get player names
		CSteamID steamIDWinner( msg.Body().m_ulWinnerSteamID );
		CSteamID steamIDInitiator( msg.Body().m_ulInitiatorSteamID );
		CSteamID steamIDTarget( msg.Body().m_ulTargetSteamID );
		char szPlayerName_Winner[ MAX_PLAYER_NAME_LENGTH ];
		GetPlayerNameBySteamID( steamIDWinner, szPlayerName_Winner, sizeof( szPlayerName_Winner ) );
		char szPlayerName_Initiator[ MAX_PLAYER_NAME_LENGTH ];
		GetPlayerNameBySteamID( steamIDInitiator, szPlayerName_Initiator, sizeof( szPlayerName_Initiator ) );
		char szPlayerName_Target[ MAX_PLAYER_NAME_LENGTH ];
		GetPlayerNameBySteamID( steamIDTarget, szPlayerName_Target, sizeof( szPlayerName_Target ) );
		wchar_t wszPlayerName_Winner[MAX_PLAYER_NAME_LENGTH];
		wchar_t wszPlayerName_Initiator[MAX_PLAYER_NAME_LENGTH];
		wchar_t wszPlayerName_Target[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode( szPlayerName_Winner, wszPlayerName_Winner, sizeof(wszPlayerName_Winner) );
		g_pVGuiLocalize->ConvertANSIToUnicode( szPlayerName_Initiator, wszPlayerName_Initiator, sizeof(wszPlayerName_Initiator) );
		g_pVGuiLocalize->ConvertANSIToUnicode( szPlayerName_Target, wszPlayerName_Target, sizeof(wszPlayerName_Target) );

		// build text
		bool bTie = msg.Body().m_usScoreInitiator == msg.Body().m_usScoreTarget;
		const char *pText = "TF_Duel_Win";
		switch ( msg.Body().m_usEndReason )
		{
		case kDuelEndReason_DuelOver:
			break;
		case kDuelEndReason_PlayerDisconnected:
			bTie = false;
			pText = "TF_Duel_Win_Disconnect";
			break;
		case kDuelEndReason_PlayerSwappedTeams:
			bTie = false;
			pText = "TF_Duel_Win_SwappedTeams";
			break;
		case kDuelEndReason_LevelShutdown:
			bTie = true;
			pText = "TF_Duel_Refund_LevelShutdown";
			break;
		case kDuelEndReason_ScoreTiedAtZero:
			bTie = true;
			pText = "TF_Duel_Refund_ScoreTiedAtZero";
			break;
		case kDuelEndReason_ScoreTied:
			bTie = true;
			pText = "TF_Duel_Tie";
			break;
		case kDuelEndReason_PlayerKicked:
			bTie = true;
			pText = "TF_Duel_Refund_Kicked";
			break;
		case kDuelEndReason_PlayerForceSwappedTeams:
			bTie = true;
			pText = "TF_Duel_Refund_ForceTeamSwap";
			break;
		case kDuelEndReason_Cancelled:
			bTie = true;
			pText = "TF_Duel_Cancelled";
			break;
		}

		KeyValues *pKeyValues = new KeyValues( "DuelResults" );
		if ( bTie == false )
		{
			pKeyValues->SetWString( "winner", wszPlayerName_Winner );
			pKeyValues->SetWString( "loser", steamIDWinner == steamIDInitiator ? wszPlayerName_Target : wszPlayerName_Initiator );
			wchar_t wszScoreInitiator[16];
			wchar_t wszScoreTarget[16];
			_snwprintf( wszScoreInitiator, ARRAYSIZE( wszScoreInitiator ), L"%u", (uint32)msg.Body().m_usScoreInitiator );
			_snwprintf( wszScoreTarget, ARRAYSIZE( wszScoreTarget ), L"%u", (uint32)msg.Body().m_usScoreTarget );
			pKeyValues->SetWString( "winner_score", steamIDWinner == steamIDInitiator ? wszScoreInitiator : wszScoreTarget );
			pKeyValues->SetWString( "loser_score", steamIDWinner == steamIDInitiator ? wszScoreTarget : wszScoreInitiator );
		}
		else
		{
			wchar_t wszScoreInitiator[16];
			_snwprintf( wszScoreInitiator, ARRAYSIZE( wszScoreInitiator ), L"%u", (uint32)msg.Body().m_usScoreInitiator );
			pKeyValues->SetWString( "score", wszScoreInitiator );
			pKeyValues->SetWString( "initiator", wszPlayerName_Initiator );
			pKeyValues->SetWString( "target", wszPlayerName_Target );
		}

		// add notification
		if ( localSteamID == steamIDInitiator || localSteamID == steamIDTarget )
		{
			// remove existing duel info notifications
			NotificationQueue_Remove( &RemoveRelatedDuelNotifications );
			// add new one
			CTFDuelInfoNotification *pNotification = new CTFDuelInfoNotification();
			pNotification->SetText( pText );
			pNotification->SetKeyValues( pKeyValues );
			pNotification->SetSteamID( bTie == false ? steamIDWinner : ( localSteamID == steamIDInitiator ? steamIDTarget : steamIDInitiator ) );
			pNotification->SetSoundFilename( "ui/duel_event.wav" );
			NotificationQueue_Add( pNotification );
			gDuelMiniGameLocalData.m_steamIDOpponent = CSteamID();
		}
		
		// print out to chat
		PrintTextToChat( pText, pKeyValues );

		// cleanup
		pKeyValues->deleteThis();

		// reset the dueling minigame
		DuelMiniGame_Reset();

		return true;
	}
protected:
};
GC_REG_JOB( GCSDK::CGCClient, CGC_Duel_Results, "CGC_Duel_Results", k_EMsgGC_Duel_Results, GCSDK::k_EServerTypeGCClient );

static bool RemoveRelatedDuelNotifications( CEconNotification *pNotification )
{
	return ( CTFDuelRequestNotification::IsDuelRequestNotification( pNotification ) || 
			 CTFDuelInfoNotification::IsDuelInfoNotification( pNotification ) );
}

static void DuelMiniGame_Reset()
{
	delete gDuelMiniGameLocalData.m_pEventListener;
	gDuelMiniGameLocalData.m_pEventListener = NULL;
	gDuelMiniGameLocalData.m_steamIDOpponent = CSteamID();
	gDuelMiniGameLocalData.m_unMyScore = 0;
	gDuelMiniGameLocalData.m_unOpponentScore = 0;
	gDuelMiniGameLocalData.m_iRequiredPlayerClass = TF_CLASS_UNDEFINED;
}

bool DuelMiniGame_IsDuelingLocalPlayer( C_TFPlayer *pPlayer )
{
	CSteamID steamID;
	if ( pPlayer->GetSteamID( &steamID ) )
	{
		return ( steamID == gDuelMiniGameLocalData.m_steamIDOpponent );
	}
	return false;
}

bool DuelMiniGame_GetStats( C_TFPlayer **ppPlayer, uint32 &unMyScore, uint32 &unOpponentScore )
{
	*ppPlayer = NULL;
	int iLocalPlayerIndex =  GetLocalPlayerIndex();
	for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
	{
		// find all players who are on the local player's team
		if( ( iPlayerIndex != iLocalPlayerIndex ) && ( g_PR->IsConnected( iPlayerIndex ) ) )
		{
			CSteamID steamID;
			C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( pPlayer && pPlayer->GetSteamID( &steamID ) && steamID == gDuelMiniGameLocalData.m_steamIDOpponent ) 
			{
				*ppPlayer = pPlayer;
				break;
			}
		}
	}
	static bool sbTesting = false;
	if ( sbTesting )
	{
		*ppPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	}
	if ( *ppPlayer != NULL )
	{
		CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
		unMyScore = gDuelMiniGameLocalData.m_unMyScore;
		unOpponentScore = gDuelMiniGameLocalData.m_unOpponentScore;
		return true;
	}
	return false;
}

/**
 * Select player for duel dialog.
 */
class CSelectPlayerForDuelDialog : public CSelectPlayerDialog, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CSelectPlayerForDuelDialog, CSelectPlayerDialog );
public:
	CSelectPlayerForDuelDialog( uint64 iItemID );
	virtual ~CSelectPlayerForDuelDialog();
	virtual void OnSelectPlayer( const CSteamID &steamID );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void FireGameEvent( IGameEvent *event );

	MESSAGE_FUNC_PARAMS( OnClassIconSelected, "ClassIconSelected", data );
	MESSAGE_FUNC_PARAMS( OnShowClassIconMouseover, "ShowClassIconMouseover", data );
	MESSAGE_FUNC( OnHideClassIconMouseover, "HideClassIconMouseover" );

protected:
	virtual const char *GetResFile() { return "resource/ui/SelectPlayerDialog_Duel.res"; } 
	void SetSelectedClass( int iClass );
	void SetupClassImage( const char *pImageControlName, int iClass );

	uint64 m_iItemID;
	uint8 m_iPlayerClass;
	vgui::Label *m_pClassIconMouseoverLabel;
};

static vgui::DHANDLE< CSelectPlayerForDuelDialog > g_pSelectPlayerForDuelingDialog;

CSelectPlayerForDuelDialog::CSelectPlayerForDuelDialog( uint64 iItemID ) 
	: CSelectPlayerDialog( NULL )
	, m_iItemID( iItemID )
	, m_iPlayerClass( TF_CLASS_UNDEFINED )
{
	g_pSelectPlayerForDuelingDialog = this;
	m_bAllowSameTeam = false;
	m_bAllowOutsideServer = false;
	m_pClassIconMouseoverLabel = new vgui::Label( this, "ClassUsageMouseoverLabel", "" );
	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "teamplay_round_stalemate" );
}

CSelectPlayerForDuelDialog::~CSelectPlayerForDuelDialog()
{
	g_pSelectPlayerForDuelingDialog = NULL;
}

void CSelectPlayerForDuelDialog::OnSelectPlayer( const CSteamID &steamID )
{
	GCSDK::CProtoBufMsg<CMsgUseItem> msg( k_EMsgGCUseItemRequest );
	msg.Body().set_item_id( m_iItemID );
	msg.Body().set_target_steam_id( steamID.ConvertToUint64() );
	msg.Body().set_duel__class_lock( m_iPlayerClass );
	GCClientSystem()->BSendMessage( msg );
}

void CSelectPlayerForDuelDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	CSelectPlayerDialog::ApplySchemeSettings( pScheme );
	SetDialogVariable( "title", g_pVGuiLocalize->Find( "TF_DuelDialog_Title" ) );

	SetupClassImage( "ClassUsageImage_Any", TF_CLASS_UNDEFINED );

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	SetupClassImage( "ClassUsageImage_Locked", pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex() );

	SetSelectedClass( TF_CLASS_UNDEFINED );
}

void CSelectPlayerForDuelDialog::SetupClassImage( const char *pImageControlName, int iClass )
{
	CStorePreviewClassIcon *pClassImage = dynamic_cast<CStorePreviewClassIcon*>( FindChildByName( pImageControlName ) );
	if ( pClassImage )
	{
		pClassImage->SetClass( iClass );
	}
}

void CSelectPlayerForDuelDialog::FireGameEvent( IGameEvent *event )
{
	const char *pEventName = event->GetName();

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer == NULL )
	{
		OnCommand( "cancel" );
		return;
	}

	if ( Q_strcmp( "teamplay_round_win", pEventName ) == 0 || Q_strcmp( "teamplay_round_stalemate", pEventName ) == 0 )
	{
		OnCommand( "cancel" );
		return;
	}
}

void CSelectPlayerForDuelDialog::OnClassIconSelected( KeyValues *data )
{
	int iClass = data->GetInt( "class", 0 );
	SetSelectedClass( iClass );
}

void CSelectPlayerForDuelDialog::OnHideClassIconMouseover( void )
{
	if ( m_pClassIconMouseoverLabel )
	{
		m_pClassIconMouseoverLabel->SetVisible( false );
	}
}

void CSelectPlayerForDuelDialog::OnShowClassIconMouseover( KeyValues *data )
{
	if ( m_pClassIconMouseoverLabel )
	{
		// Set the text to the correct string
		int iClass = data->GetInt( "class", 0 );
		if ( iClass >= TF_FIRST_NORMAL_CLASS && iClass < TF_LAST_NORMAL_CLASS )
		{
			wchar_t wzLocalized[256];
			const char *pszLocString = "#TF_SelectPlayer_Duel_PlayerClass";
			g_pVGuiLocalize->ConstructString_safe( wzLocalized, g_pVGuiLocalize->Find( pszLocString ), 1, g_pVGuiLocalize->Find( g_aPlayerClassNames[iClass] ) );
			m_pClassIconMouseoverLabel->SetText( wzLocalized );
		}
		else
		{
			const char *pszLocString = "#TF_SelectPlayer_Duel_AnyClass";
			m_pClassIconMouseoverLabel->SetText( pszLocString );
		}

		m_pClassIconMouseoverLabel->SetVisible( true );
	}
}

void CSelectPlayerForDuelDialog::SetSelectedClass( int iClass )
{
	m_iPlayerClass = iClass;

	const char* pClassName = "#TF_SelectPlayer_DuelClass_None";
	
	switch ( m_iPlayerClass )
	{
	case TF_CLASS_SCOUT: 			pClassName = "#TF_Class_Name_Scout"; break;
	case TF_CLASS_SNIPER: 			pClassName = "#TF_Class_Name_Sniper"; break;
	case TF_CLASS_SOLDIER: 			pClassName = "#TF_Class_Name_Soldier"; break;
	case TF_CLASS_DEMOMAN: 			pClassName = "#TF_Class_Name_Demoman"; break;
	case TF_CLASS_MEDIC: 			pClassName = "#TF_Class_Name_Medic"; break;
	case TF_CLASS_HEAVYWEAPONS: 	pClassName = "#TF_Class_Name_HWGuy"; break;
	case TF_CLASS_PYRO: 			pClassName = "#TF_Class_Name_Pyro"; break;
	case TF_CLASS_SPY: 				pClassName = "#TF_Class_Name_Spy"; break;
	case TF_CLASS_ENGINEER: 		pClassName = "#TF_Class_Name_Engineer"; break;
	}

	wchar_t wszText[1024]=L"";
	g_pVGuiLocalize->ConstructString_safe( wszText, 
									  g_pVGuiLocalize->Find( "#TF_SelectPlayer_DuelClass" ),
									  1, 
									  g_pVGuiLocalize->Find( pClassName ) );

	SetDialogVariable( "player_class", wszText );
}

static void ShowSelectDuelTargetDialog( uint64 iItemID )
{
	CSelectPlayerForDuelDialog *pDialog = vgui::SETUP_PANEL( new CSelectPlayerForDuelDialog( iItemID ) );
	pDialog->InvalidateLayout( false, true );

	pDialog->Reset();
	pDialog->SetVisible( true );
	pDialog->MakePopup();
	pDialog->MoveToFront();
	pDialog->SetKeyBoardInputEnabled(true);
	pDialog->SetMouseInputEnabled(true);
	TFModalStack()->PushModal( pDialog );
}

//-----------------------------------------------------------------------------
void CL_Consumables_LevelShutdown()
{
	DuelMiniGame_Reset();
	if ( g_pSelectPlayerForDuelingDialog )
	{
		g_pSelectPlayerForDuelingDialog->OnCommand( "cancel" );
	}
	NotificationQueue_Remove( &CTFDuelRequestNotification::IsDuelRequestNotification );
}

//
// Players see this whenever a person on their server uses a name tool
//
class CTFNameItemNotification : public GCSDK::CGCClientJob
{
public:
	CTFNameItemNotification( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}
	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgGCNameItemNotification> msg( pNetPacket );

		if ( steamapicontext == NULL || steamapicontext->SteamUser() == NULL )
			return true;
		
		if ( !engine->IsInGame() )
			return true;

		CBaseHudChat *pHUDChat = (CBaseHudChat *)GET_HUDELEMENT( CHudChat );
		if ( pHUDChat )
		{
			// Player
			wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
			char szPlayerName[ MAX_PLAYER_NAME_LENGTH ];

			GetPlayerNameBySteamID( msg.Body().player_steamid(), szPlayerName, sizeof( szPlayerName ) );
			g_pVGuiLocalize->ConvertANSIToUnicode( szPlayerName, wszPlayerName, sizeof( wszPlayerName ) );

			// Item
			item_definition_index_t nDefIndex = msg.Body().item_def_index();
			const GameItemDefinition_t *pItemDefinition = dynamic_cast<GameItemDefinition_t *>( GetItemSchema()->GetItemDefinition( nDefIndex ) );
			if ( !pItemDefinition )
				return true;
			entityquality_t iItemQuality = pItemDefinition->GetQuality();

			// Name
			wchar_t wszCustomName[MAX_ITEM_CUSTOM_NAME_LENGTH];
			g_pVGuiLocalize->ConvertANSIToUnicode( msg.Body().item_name_custom().c_str(), wszCustomName, sizeof( wszCustomName ) );

			wchar_t wszItemRenamed[256];
			_snwprintf( wszItemRenamed, ARRAYSIZE( wszItemRenamed ), L"%ls", g_pVGuiLocalize->Find( "#Item_Named" ) );

			const char *pszQualityColorString = EconQuality_GetColorString( (EEconItemQuality)iItemQuality );
			if ( pszQualityColorString )
			{
				pHUDChat->SetCustomColor( pszQualityColorString );
			}
	
			wchar_t wszLocalizedString[256];
			g_pVGuiLocalize->ConstructString_safe( wszLocalizedString, wszItemRenamed, 3, wszPlayerName, CEconItemLocalizedFullNameGenerator( GLocalizationProvider(), pItemDefinition, iItemQuality ).GetFullName(), wszCustomName );

			char szLocalized[256];
			g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalizedString, szLocalized, sizeof( szLocalized ) );

			pHUDChat->ChatPrintf( 0, CHAT_FILTER_SERVERMSG, "%s", szLocalized );
		}

		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CTFNameItemNotification, "CTFNameItemNotification", k_EMsgGCNameItemNotification, GCSDK::k_EServerTypeGCClient );

//
// A wrapper for a generic message sent down from the GC to clients. The clients do localization
// and layout.
//
class CClientDisplayNotification : public CEconNotification
{
public:
	CClientDisplayNotification( GCSDK::IMsgNetPacket *pNetPacket )
		: m_msg( pNetPacket )
	{
		SetText( m_msg.Body().notification_body_localization_key().c_str() );
		SetLifetime( 23.0f );

		if ( m_msg.Body().body_substring_keys_size() == m_msg.Body().body_substring_values_size() )
		{
			for ( int i = 0; i < m_msg.Body().body_substring_keys_size(); i++ )
			{
				const char *pszSubstringKey = m_msg.Body().body_substring_keys( i ).c_str();
				const char *pszSubstringValue = m_msg.Body().body_substring_values( i ).c_str();

				if ( pszSubstringValue[0] == '#' )
				{
					AddStringToken( pszSubstringKey, GLocalizationProvider()->Find( pszSubstringValue ) );
				}
				else
				{
					CUtlConstWideString wsSubstringValue;
					GLocalizationProvider()->ConvertUTF8ToLocchar( pszSubstringValue, &wsSubstringValue ) ;
					AddStringToken( pszSubstringKey, wsSubstringValue.Get() );
				}
			}
		}
	}

private:
	// We make a local copy of the full message because dynamic-length strings are sent down from the
	// GC and we need to make sure they stay in memory until the user looks at the notification.
	GCSDK::CProtoBufMsg<CMsgGCClientDisplayNotification> m_msg;
};

class CTFClientDisplayNotification : public GCSDK::CGCClientJob
{
public:
	CTFClientDisplayNotification( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}
	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgGCClientDisplayNotification> msg( pNetPacket );

		if ( steamapicontext == NULL || steamapicontext->SteamUser() == NULL )
			return true;

		NotificationQueue_Add( new CClientDisplayNotification( pNetPacket ) );

		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CTFClientDisplayNotification, "CTFClientDisplayNotification", k_EMsgGCClientDisplayNotification, GCSDK::k_EServerTypeGCClient );
