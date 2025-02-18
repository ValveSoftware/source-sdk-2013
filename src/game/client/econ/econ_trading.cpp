//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"

#include "econ_trading.h"

// for messaging with the GC
#include "econ_gcmessages.h"
#include "econ_item_inventory.h"
#include "econ_gcmessages.h"
#include "econ_ui.h"

// other
#include "c_baseplayer.h"
#include "c_playerresource.h"

// UI
#include "confirm_dialog.h"
#include "econ_controls.h"
#include "vgui/ILocalize.h"
#include "econ_notifications.h"

#ifdef TF_CLIENT_DLL
#include "c_tf_gamestats.h"
#endif

#include "gc_clientsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------

const char *g_FriendRelationship[] =
{
	"none"
	"blocked",
	"request_recipient",
	"friend",
	"request_initiator",
	"ignored",
	"ignored_friend"
};

const char *g_RejectedReasons[] =
{
	"accepted",
	"declined",
	"vac_banned_initiator",
	"vac_banned_target",
	"target_already_trading",
	"trading_disabled",
	"user_not_logged_in"
};

const char *g_ClosedReasons[] =
{
	"traded",
	"canceled",
	"error",
	"does_not_own_items",
	"untradable_items",
	"no_items",
	"trading_disabled"
};

enum
{
	kShowTradeRequestsFrom_FriendsOnly			   = 1,
	kShowTradeRequestsFrom_FriendsAndCurrentServer = 2,
	kShowTradeRequestsFrom_Anyone				   = 3,
	kShowTradeRequestsFrom_NoOne				   = 4,
};

ConVar cl_trading_show_requests_from( "cl_trading_show_requests_from", "3", FCVAR_ARCHIVE, "View trade requests from a certain group only." );

static bool sbTestingSelfTrade = false;

static int iTradeRequests = 0;
static int iTradeAttempts = 0;
static int iTradeOffers = 0;
static int iGiftsGiven = 0;

const uint32 kTradeRequestLifetime = 30.0f;

// waiting dialog
class CTradingWaitDialog : public CGenericWaitingDialog
{
public:
	CTradingWaitDialog( const char *pText = "#TF_Trading_Timeout_Text", const wchar_t *pPlayerName = NULL ) 
		: CGenericWaitingDialog( NULL )
		, m_pText( pText )
		, m_pKeyValues( NULL )
	{
		if ( pPlayerName != NULL && wcsicmp( pPlayerName, L"" ) != 0 )
		{
			m_pKeyValues = new KeyValues( "CTradingWaitDialog" );
			m_pKeyValues->SetWString( "other_player", pPlayerName );
		}
	}

	virtual ~CTradingWaitDialog()
	{
		if ( m_pKeyValues != NULL )
		{
			m_pKeyValues->deleteThis();
		}
	}

protected:
	virtual void OnTimeout()
	{
		ShowMessageBox( "#TF_Trading_Timeout_Title", m_pText, m_pKeyValues, "#GameUI_OK" );
	}

	virtual void OnUserClose()
	{
		GCSDK::CGCMsg< MsgGCTrading_CancelSession_t > msg( k_EMsgGCTrading_CancelSession );
		GCClientSystem()->BSendMessage( msg );
		// @note not sure we need to wait for the GC here, but we will just in case...
		ShowWaitingDialog( new CTradingWaitDialog(), "#TF_Trading_WaitingForCancel", true, false, 20.0f );
	}

	KeyValues *m_pKeyValues;
	const char* m_pText;
};

// used by the waiting dialogs
static void TradeCompleteDialogClosed( bool bConfirmed, void *pContext )
{
	if ( bConfirmed )
	{
		InventoryManager()->ShowItemsPickedUp( true );
	}
}


//-----------------------------------------------------------------------------
// jobs
class CTFTradeRequestNotification : public CEconNotification
{
public:
	CTFTradeRequestNotification( uint64 ulInitiatorSteamID, uint32 unTradeRequestID, const char* pPlayerName ) 
		: CEconNotification() 
		, m_unTradeRequestID( unTradeRequestID )
	{
		SetSteamID( ulInitiatorSteamID );
		g_pVGuiLocalize->ConvertANSIToUnicode( pPlayerName, m_wszPlayerName, sizeof(m_wszPlayerName) );
		SetLifetime( kTradeRequestLifetime );
		SetText( "#TF_Trading_JoinText" );
		AddStringToken( "initiator", m_wszPlayerName );
	}

	virtual EType NotificationType() { return eType_AcceptDecline; }

	/// XXX(JohnS): Dead code? Was always accept/decline AFAICT
	virtual void Trigger()
	{
		CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( "#TF_Trading_JoinTitle",  "#TF_Trading_JoinText", "#GameUI_OK", "#TF_Trading_JoinCancel", &ConfirmJoinTradeSession );
		pDialog->SetContext( this );
		pDialog->AddStringToken( "initiator", m_wszPlayerName );
		// so we aren't deleted
		SetIsInUse( true );
	}

	virtual void Accept()
	{
		ConfirmJoinTradeSession( true, this );
	}
	virtual void Decline()
	{
		ConfirmJoinTradeSession( false, this );
	}
	static void ConfirmJoinTradeSession( bool bConfirmed, void *pContext )
	{
		CTFTradeRequestNotification *pNotification = (CTFTradeRequestNotification*)pContext;
		GCSDK::CGCMsg< MsgGCTrading_InitiateTradeResponse_t > msg( k_EMsgGCTrading_InitiateTradeResponse );
		msg.Body().m_eResponse = bConfirmed ? k_EGCMsgInitiateTradeResponse_Accepted : k_EGCMsgInitiateTradeResponse_Declined;
		msg.Body().m_unTradeRequestID = pNotification->m_unTradeRequestID;
		GCClientSystem()->BSendMessage( msg );
		if ( bConfirmed )
		{
			ShowWaitingDialog( new CTradingWaitDialog(), "#TF_Trading_WaitingForServer", true, false, kTradeRequestLifetime );
		}
		// now we can be deleted
		pNotification->SetIsInUse( false );
		pNotification->MarkForDeletion();
	}

	static bool IsTradingNotification( CEconNotification * pNotification )
	{
		return dynamic_cast< CTFTradeRequestNotification * >( pNotification ) != NULL;
	}

public:
	uint32 m_unTradeRequestID;
	wchar_t m_wszPlayerName[MAX_PLAYER_NAME_LENGTH];
};

// request from party A (through GC) to start trading
class CGCTrading_InitiateTradeRequest : public GCSDK::CGCClientJob
{
public:
	CGCTrading_InitiateTradeRequest( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	void SendDeclinedMessage( uint32 unTradeRequestID )
	{
		GCSDK::CGCMsg< MsgGCTrading_InitiateTradeResponse_t > msgResponse( k_EMsgGCTrading_InitiateTradeResponse );
		msgResponse.Body().m_eResponse = k_EGCMsgInitiateTradeResponse_Declined;
		msgResponse.Body().m_unTradeRequestID = unTradeRequestID;
		GCClientSystem()->BSendMessage( msgResponse );
	}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCTrading_InitiateTradeRequest_t> msg( pNetPacket );
		CUtlString playerName;
		msg.BReadStr( &playerName );

		if ( sbTestingSelfTrade )
		{
			CloseWaitingDialog();
		}
		else if ( msg.Body().m_ulOtherSteamID == Trading_GetLocalPlayerSteamID().ConvertToUint64() )
		{
			CloseWaitingDialog();
		}

		iTradeRequests++;
#ifdef TF_CLIENT_DLL
		C_CTF_GameStats.Event_Trading( IE_TRADING_REQUEST_RECEIVED, msg.Body().m_ulOtherSteamID, iTradeRequests );
#endif

		// auto-decline for ignored or blocked peoples
		if ( steamapicontext == NULL || steamapicontext->SteamFriends() == NULL )
		{
			return true;
		}
		CSteamID steamIDOther( msg.Body().m_ulOtherSteamID );
		EFriendRelationship eRelationship = steamapicontext->SteamFriends()->GetFriendRelationship( steamIDOther );
		switch ( eRelationship )
		{
			case k_EFriendRelationshipBlocked:
			case k_EFriendRelationshipIgnored:
			case k_EFriendRelationshipIgnoredFriend:
			{
				SendDeclinedMessage( msg.Body().m_unTradeRequestID );
				return true;
			}
			break;
		} // switch

		switch ( cl_trading_show_requests_from.GetInt() )
		{
			case kShowTradeRequestsFrom_FriendsOnly:
			{
				if ( eRelationship != k_EFriendRelationshipFriend )
				{
					SendDeclinedMessage( msg.Body().m_unTradeRequestID );
					return true;
				}
			}
			break;
			case kShowTradeRequestsFrom_FriendsAndCurrentServer:
			{
				if ( eRelationship == k_EFriendRelationshipFriend )
					break;
				bool bInCurrentGame = false;
				if ( engine->IsInGame() )
				{
					// otherwise, test if they are in the current game
					for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
					{
						if ( g_PR && g_PR->IsConnected( iPlayerIndex ) ) 
						{
							player_info_t pi;
							if ( !engine->GetPlayerInfo( iPlayerIndex, &pi ) )
								continue;
							if ( !pi.friendsID )
								continue;
							
							CSteamID steamID( pi.friendsID, 1, GetUniverse(), k_EAccountTypeIndividual );
							if ( steamID == steamIDOther )
							{
								bInCurrentGame = true;
								break;
							}
						}
					}
				}
				if ( bInCurrentGame == false )
				{
					SendDeclinedMessage( msg.Body().m_unTradeRequestID );
					return true;
				}
			}
			break;
			case kShowTradeRequestsFrom_Anyone:
			{
				// nothing to check
			}
			break;
			case kShowTradeRequestsFrom_NoOne:
			{
				SendDeclinedMessage( msg.Body().m_unTradeRequestID );
				return true;
			}
			break;
		}
		
		NotificationQueue_Add( new CTFTradeRequestNotification( msg.Body().m_ulOtherSteamID, msg.Body().m_unTradeRequestID, playerName.Get() ) );
		return true;
	}
protected:
};
GC_REG_JOB( GCSDK::CGCClient, CGCTrading_InitiateTradeRequest, "CGCTrading_InitiateTradeRequest", k_EMsgGCTrading_InitiateTradeRequest, GCSDK::k_EServerTypeGCClient );

#ifdef _DEBUG
#ifdef CLIENT_DLL
CON_COMMAND( cl_trading_test, "Tests the trade ui notification." )
{
	if ( steamapicontext == NULL || steamapicontext->SteamUser() == NULL )
		return;

	CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
	NotificationQueue_Add( new CTFTradeRequestNotification( steamID.ConvertToUint64(), steamID.ConvertToUint64(), "Biff" ) );
}
#endif
#endif // _DEBUG

/**
 * Remove notification that matches the trade request
 */
class CEconNotificationVisitor_RemoveTradeRequest	: public CEconNotificationVisitor
{
public: 
	CEconNotificationVisitor_RemoveTradeRequest( uint32 unTradeRequestID ) : m_unTradeRequestID( unTradeRequestID ) {}
	virtual void Visit( CEconNotification &notification )
	{
		if ( CTFTradeRequestNotification::IsTradingNotification( &notification ) )
		{
			CTFTradeRequestNotification &tradeNotification = dynamic_cast< CTFTradeRequestNotification& >( notification );
			if ( tradeNotification.m_unTradeRequestID == m_unTradeRequestID )
			{
				tradeNotification.MarkForDeletion();
			}
		}
	}
private:
	uint32 m_unTradeRequestID;
};

// GC notification of trade request status
static const char *g_pszTradeResponseDescLocKeys[] =
{
	"#TF_Trading_BusyText",					// k_EGCMsgInitiateTradeResponse_Accepted (should never be used!)
	"#TF_Trading_DeclinedText",				// k_EGCMsgInitiateTradeResponse_Declined
	"#TF_Trading_VACBannedText",			// k_EGCMsgInitiateTradeResponse_VAC_Banned_Initiator
	"#TF_Trading_VACBanned2Text",			// k_EGCMsgInitiateTradeResponse_VAC_Banned_Target
	"#TF_Trading_BusyText",					// k_EGCMsgInitiateTradeResponse_Target_Already_Trading
	"#TF_Trading_DisabledText",				// k_EGCMsgInitiateTradeResponse_Disabled
	"#TF_Trading_NotLoggedIn",				// k_EGCMsgInitiateTradeResponse_NotLoggedIn
	"#TF_Trading_BusyText",					// k_EGCMsgInitiateTradeResponse_Cancel (should never be used!)
	"#TF_Trading_TooSoon",					// k_EGCMsgInitiateTradeResponse_TooSoon
	"#TF_Trading_TooSoonPenalty",			// k_EGCMsgInitiateTradeResponse_TooSoonPenalty
	"#TF_Trading_TradeBannedText",			// (was k_EGCMsgInitiateTradeResponse_Free_Account_Initiator_DEPRECATED)
	"#TF_Trading_TradeBanned2Text",			// k_EGCMsgInitiateTradeResponse_Trade_Banned_Target
	"#TF_Trading_FreeAccountInitiate",		// k_EGCMsgInitiateTradeResponse_Free_Account_Initiator
	"#TF_Trading_SharedAccountInitiate",	// k_EGCMsgInitiateTradeResponse_Shared_Account_Initiator
	"#TF_Trading_Service_Unavailable",		// k_EGCMsgInitiateTradeResponse_Service_Unavailable
	"#TF_Trading_YouBlockedThem",			// k_EGCMsgInitiateTradeResponse_Target_Blocked
	"#TF_Trading_NeedVerifiedEmail",		// k_EGCMsgInitiateTradeResponse_NeedVerifiedEmail
	"#TF_Trading_NeedSteamGuard",			// k_EGCMsgInitiateTradeResponse_NeedSteamGuard
	"#TF_Trading_SteamGuardDuration",		// k_EGCMsgInitiateTradeResponse_SteamGuardDuration
	"#TF_Trading_TheyCannotTrade",			// k_EGCMsgInitiateTradeResponse_TheyCannotTrade
	"#TF_Trading_PasswordChanged",			// k_EGCMsgInitiateTradeResponse_Recent_Password_Reset = 20,
	"#TF_Trading_NewDevice",				// k_EGCMsgInitiateTradeResponse_Using_New_Device = 21,
	"#TF_Trading_InvalidCookie"				// k_EGCMsgInitiateTradeResponse_Sent_Invalid_Cookie = 22,
};

class CGCTrading_InitiateTradeResponse : public GCSDK::CGCClientJob
{
public:
	CGCTrading_InitiateTradeResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}
	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		// If this assertion fails it probably means you added a new value to EGCMsgInitiateTradeResponse
		// but didn't add a string to the array that tracks the user-facing response strings.
		Assert( ARRAYSIZE( g_pszTradeResponseDescLocKeys ) == k_EGCMsgInitiateTradeResponse_Count );

		CloseWaitingDialog();

		GCSDK::CGCMsg<MsgGCTrading_InitiateTradeResponse_t> msg( pNetPacket );
		const uint32 eResponse = msg.Body().m_eResponse;
		switch ( eResponse )
		{
			case k_EGCMsgInitiateTradeResponse_Accepted:
#ifdef TF_CLIENT_DLL
				C_CTF_GameStats.Event_Trading( IE_TRADING_REQUEST_ACCEPTED, iTradeRequests, g_RejectedReasons[msg.Body().m_eResponse]  );
#endif
				ShowWaitingDialog( new CTradingWaitDialog(), "#TF_Trading_WaitingForStart", true, false, 30.0f );
				return true; // !

			case k_EGCMsgInitiateTradeResponse_Cancel:
			{
				CEconNotificationVisitor_RemoveTradeRequest visitor( msg.Body().m_unTradeRequestID );
				NotificationQueue_Visit( visitor );
				break;
			}

			case k_EGCMsgInitiateTradeResponse_Declined:
			case k_EGCMsgInitiateTradeResponse_VAC_Banned_Initiator:
			case k_EGCMsgInitiateTradeResponse_VAC_Banned_Target:
			case k_EGCMsgInitiateTradeResponse_Target_Already_Trading:
			case k_EGCMsgInitiateTradeResponse_Disabled:
			case k_EGCMsgInitiateTradeResponse_NotLoggedIn:
			case k_EGCMsgInitiateTradeResponse_TooSoon:
			case k_EGCMsgInitiateTradeResponse_TooSoonPenalty:
			case k_EGCMsgInitiateTradeResponse_Trade_Banned_Initiator:
			case k_EGCMsgInitiateTradeResponse_Trade_Banned_Target:
			case k_EGCMsgInitiateTradeResponse_Shared_Account_Initiator:
			case k_EGCMsgInitiateTradeResponse_Service_Unavailable:
			case k_EGCMsgInitiateTradeResponse_Target_Blocked:
			case k_EGCMsgInitiateTradeResponse_NeedVerifiedEmail:
			case k_EGCMsgInitiateTradeResponse_NeedSteamGuard:
			case k_EGCMsgInitiateTradeResponse_TheyCannotTrade:
			case k_EGCMsgInitiateTradeResponse_Recent_Password_Reset:
			case k_EGCMsgInitiateTradeResponse_Using_New_Device:
			case k_EGCMsgInitiateTradeResponse_Sent_Invalid_Cookie:

				ShowMessageBox( "#TF_Trading_StatusTitle",  g_pszTradeResponseDescLocKeys[eResponse], "#GameUI_OK" );
				break;

			case k_EGCMsgInitiateTradeResponse_SteamGuardDuration:
			{

				KeyValuesAD kvTokens( "CTradingWaitDialog" );
				kvTokens->SetWString( "days", L"15" ); // Ideally this would come from the GC, which would get the value from Steam
				ShowMessageBox( "#TF_Trading_StatusTitle",  g_pszTradeResponseDescLocKeys[eResponse], kvTokens, "#GameUI_OK" );
				break;
			}

			default:
#ifdef TF_CLIENT_DLL
				C_CTF_GameStats.Event_Trading( IE_TRADING_REQUEST_REJECTED, iTradeRequests, "unknown" );
#endif
				ShowMessageBox( "#TF_Trading_StatusTitle",  "#TF_Trading_BusyText", "#GameUI_OK" );
				return true;
		} // switch

#ifdef TF_CLIENT_DLL
		C_CTF_GameStats.Event_Trading( IE_TRADING_REQUEST_REJECTED, iTradeRequests, g_RejectedReasons[msg.Body().m_eResponse] );
#endif

		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCTrading_InitiateTradeResponse, "CGCTrading_InitiateTradeResponse", k_EMsgGCTrading_InitiateTradeResponse, GCSDK::k_EServerTypeGCClient );

// start trading session
class CGCTrading_StartSession : public GCSDK::CGCClientJob
{
public:
	CGCTrading_StartSession( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}
	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCTrading_StartSession_t> msg( pNetPacket );

		steamapicontext->SteamFriends()->ActivateGameOverlayToUser( "jointrade", msg.Body().m_ulSteamIDPartyB );

		CloseWaitingDialog();

		// remove all trading notifications
		NotificationQueue_Remove( &CTFTradeRequestNotification::IsTradingNotification );

		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCTrading_StartSession, "CGCTrading_StartSession", k_EMsgGCTrading_StartSession, GCSDK::k_EServerTypeGCClient );


//-----------------------------------------------------------------------------
// External interface

CSteamID Trading_GetLocalPlayerSteamID()
{
	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		return steamapicontext->SteamUser()->GetSteamID();
	}
	return CSteamID();
}

void Trading_RequestTrade( int iPlayerIdx )
{
	CSteamID steamID;
	C_BasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex( iPlayerIdx ) );
	if ( pPlayer && pPlayer->GetSteamID( &steamID ) )
	{
		Trading_RequestTrade( steamID );
	}
}

void Trading_RequestTrade( const CSteamID &steamID )
{								
	sbTestingSelfTrade = false;
	GCSDK::CGCMsg< MsgGCTrading_InitiateTradeRequest_t > msg( k_EMsgGCTrading_InitiateTradeRequest );
	msg.Body().m_ulOtherSteamID = steamID.ConvertToUint64();
	bool bSent = GCClientSystem()->BSendMessage( msg );
	if ( bSent )
	{
		iTradeRequests++;
#ifdef TF_CLIENT_DLL
		C_CTF_GameStats.Event_Trading( IE_TRADING_REQUEST_SENT, msg.Body().m_ulOtherSteamID, iTradeRequests );
#endif

		const char* pPlayerName = InventoryManager()->PersonaName_Get( steamID.GetAccountID() );
		if ( pPlayerName != NULL && FStrEq( pPlayerName, "" ) == false )
		{
			wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
			g_pVGuiLocalize->ConvertANSIToUnicode( pPlayerName, wszPlayerName, sizeof(wszPlayerName) );
			CTradingWaitDialog *pDialog = new CTradingWaitDialog( "#TF_Trading_TimeoutPartyB_Named", wszPlayerName );
			ShowWaitingDialog( pDialog, "#TF_Trading_WaitingForPartyB", true, true, 30.0f );
			wchar_t wszConstructedString[1024];
			g_pVGuiLocalize->ConstructString_safe( wszConstructedString, g_pVGuiLocalize->Find( "#TF_Trading_WaitingForPartyB_Named" ), 1, wszPlayerName );
			pDialog->SetDialogVariable( "updatetext", wszConstructedString );
		}
		else
		{
			CTradingWaitDialog *pDialog = new CTradingWaitDialog( "#TF_Trading_TimeoutPartyB" );
			ShowWaitingDialog( pDialog, "#TF_Trading_WaitingForPartyB", true, true, 30.0f );
		}
	}
}

const char* UniverseToCommunityURL( EUniverse universe )
{
	switch( universe )
	{
	default: // return public if we don't have a better guess.
	case k_EUniversePublic: return "https://steamcommunity.com";
	case k_EUniverseBeta: return "https://beta.steamcommunity.com";
	case k_EUniverseDev: return "https://localhost/community";
	}

	// Should never get here.
	return UniverseToCommunityURL( k_EUniversePublic ); 
}

const char* GetCommunityURL()
{
	if ( GetUniverse() == k_EUniverseInvalid )
	{
		Assert( !"calling GetCommunityURL when not connected. This is allowed, but will return public universe." );
		return UniverseToCommunityURL( k_EUniversePublic );
	}

	return UniverseToCommunityURL( GetUniverse() );
}

void Trading_SendGift( const CSteamID& steamID, const CEconItemView& giftItem )
{
	if ( !steamapicontext || !steamapicontext->SteamFriends() )
	{
		// TODO: Error dialog.
		return;
	}

#ifdef TF_CLIENT_DLL
	C_CTF_GameStats.Event_Trading( IE_TRADING_ITEM_GIFTED, steamID.ConvertToUint64(), iGiftsGiven );
#endif

	// Build up the steam URL and send it over. 
	// Should look like this: https://steamcommunity.com/trade/1/sendgift/?appid=&contextid=&assetid=&steamid_target=

	steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( 
		CFmtStrMax( "%s/trade/1/sendgift/?appid=%d&contextid=%d&assetid=%llu&steamid_target=%llu",
					GetCommunityURL(), 
					engine->GetAppID(),
					2, // k_EEconContextBackpack
					giftItem.GetItemID(),
					steamID.ConvertToUint64()
		)
	);

}

CON_COMMAND( cl_trade, "Trade with a person by player name" )
{
	if ( args.ArgC() < 2 )
		return;

	if ( GetUniverse() == k_EUniverseInvalid )
		return;

	int iLocalPlayerIndex =  GetLocalPlayerIndex();
	for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
	{
		if( ( iPlayerIndex != iLocalPlayerIndex ) && ( g_PR->IsConnected( iPlayerIndex ) ) )
		{
			player_info_t pi;
			if ( !engine->GetPlayerInfo( iPlayerIndex, &pi ) )
				continue;
			if ( !pi.friendsID )
				continue;

			if ( FStrEq( pi.name, args[1] ) == false )
				continue;
			
			CSteamID steamID( pi.friendsID, 1, GetUniverse(), k_EAccountTypeIndividual );
			Trading_RequestTrade( steamID );
			return;
		}
	}
}

CON_COMMAND( cl_trade_steamid, "Trade with a person by steam id" )
{
	if ( args.ArgC() < 2 )
		return;

	if ( GetUniverse() == k_EUniverseInvalid )
		return;

	const char *pInput = args[1];
	if ( pInput[0] >= '0' && pInput[0] <= '9' )
	{
		CSteamID steamID;
		steamID.SetFromString( pInput, GetUniverse() );
		if ( steamID.IsValid() )
			Trading_RequestTrade( steamID );
	}
}

#ifdef _DEBUG
CON_COMMAND( cl_trading_test_self_trade, "Test self-trading" )
{
	Trading_RequestTrade( Trading_GetLocalPlayerSteamID() );
	sbTestingSelfTrade = true;
}
#endif
