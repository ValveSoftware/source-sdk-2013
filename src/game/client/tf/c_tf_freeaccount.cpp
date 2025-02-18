//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "c_tf_freeaccount.h"

#include "gcsdk/sharedobjectcache.h"
#include "tf_gcmessages.h"
#include "econ_game_account_client.h"
#include "tf_item_inventory.h"
#include "tf_player_info.h"

#include <vgui/ILocalize.h>
#include "confirm_dialog.h"
#include "econ/econ_notifications.h"
#include "select_player_dialog.h"

#include "gc_clientsystem.h"

//-----------------------------------------------------------------------------

class CSelectMostHelpfulFriendDialog : public CSelectPlayerDialog
{
public:
	CSelectMostHelpfulFriendDialog( vgui::Panel *parent ) 
		: CSelectPlayerDialog( parent )
		, m_iNumFriends( 0 )
		, m_bRefreshing( false )
	{
	}

	virtual void UpdatePlayerList()
	{
		CSelectPlayerDialog::UpdatePlayerList();

		vgui::Label *pLabelEmpty = dynamic_cast<vgui::Label*>( m_pStatePanels[m_iCurrentState]->FindChildByName("EmptyPlayerListLabel") );
		vgui::Label *pLabelQuery = dynamic_cast<vgui::Label*>( m_pStatePanels[m_iCurrentState]->FindChildByName("QueryLabel") );
		vgui::Label *pLabelRetrieving = dynamic_cast<vgui::Label*>( m_pStatePanels[m_iCurrentState]->FindChildByName("RetrievingPlayerListLabel") );

		if ( pLabelEmpty )
		{
			pLabelEmpty->SetVisible( m_bRefreshing == false && pLabelEmpty->IsVisible() );
		}
		if ( pLabelQuery )
		{
			pLabelQuery->SetVisible( m_bRefreshing == false && pLabelQuery->IsVisible() );
		}
		if ( pLabelRetrieving )
		{
			pLabelRetrieving->SetVisible( m_bRefreshing );
		}	
	}

	virtual void Reset()
	{
		CSelectPlayerDialog::Reset();
		m_iNumFriends = 0;
		if ( m_bRefreshing == false )
		{
			RequestFriendsFromGC();
		}
	}

	virtual void SetupSelectFriends()
	{
		m_PlayerInfoList.Purge();

		if ( steamapicontext && steamapicontext->SteamFriends() )
		{
			// Get our game info so we can use that to test if our friends are connected to the same game as us
			FriendGameInfo_t myGameInfo;
			CSteamID mySteamID = steamapicontext->SteamUser()->GetSteamID();
			steamapicontext->SteamFriends()->GetFriendGamePlayed( mySteamID, &myGameInfo );

			int iFriends = steamapicontext->SteamFriends()->GetFriendCount( k_EFriendFlagImmediate );
			if ( m_iNumFriends != iFriends )
			{
				RequestFriendsFromGC();
			}
			else
			{
				m_PlayerInfoList = m_FriendsWhoOwnTF2;
			}
		}

		UpdatePlayerList();
	}

	virtual void OnSelectPlayer( const CSteamID &steamID )
	{
		GCSDK::CProtoBufMsg<CMsgTFFreeTrialChooseMostHelpfulFriend> msg( k_EMsgGCFreeTrial_ChooseMostHelpfulFriend );
		msg.Body().set_account_id_friend( steamID.GetAccountID() );
		GCClientSystem()->BSendMessage( msg );
	}

	void OnTF2FriendsReceived( GCSDK::CProtoBufMsg<CMsgTFRequestTF2FriendsResponse> &msg )
	{
		// populate the list of friends who own TF2
		m_bRefreshing = false;
		m_FriendsWhoOwnTF2.Purge();
		for ( int i = 0; i < msg.Body().account_ids_size(); ++i )
		{
			uint32 unAccountID = msg.Body().account_ids( i );
			FOR_EACH_VEC( m_EntireFriendsList, j )
			{
				partner_info_t &info = m_EntireFriendsList[j];
				if ( info.m_steamID.GetAccountID() == unAccountID )
				{
					int idx = m_FriendsWhoOwnTF2.AddToTail();
					partner_info_t &infoCopy = m_FriendsWhoOwnTF2[idx];
					infoCopy = info;
					break;
				}
			}
		}

		// update UI
		if ( m_iCurrentState == SPDS_SELECTING_FROM_FRIENDS )
		{
			m_PlayerInfoList = m_FriendsWhoOwnTF2;
			UpdatePlayerList();
		}
	}

protected:
	virtual const char *GetResFile() { return "resource/ui/SelectMostHelpfulFriendDialog.res"; }

	void RequestFriendsFromGC()
	{
		// send a message to the GC requesting that it validate which friends own TF2
		m_bRefreshing = true;
		m_EntireFriendsList.Purge();
		m_FriendsWhoOwnTF2.Purge();

		if ( steamapicontext && steamapicontext->SteamFriends() )
		{
			// Get our game info so we can use that to test if our friends are connected to the same game as us
			FriendGameInfo_t myGameInfo;
			CSteamID mySteamID = steamapicontext->SteamUser()->GetSteamID();
			steamapicontext->SteamFriends()->GetFriendGamePlayed( mySteamID, &myGameInfo );

			m_iNumFriends = steamapicontext->SteamFriends()->GetFriendCount( k_EFriendFlagImmediate );
			if ( m_iNumFriends > 0 )
			{
				GCSDK::CProtoBufMsg<CMsgTFRequestTF2Friends> msg( k_EMsgGCRequestTF2Friends );
				for ( int i = 0; i < m_iNumFriends; i++ )
				{
					CSteamID friendSteamID = steamapicontext->SteamFriends()->GetFriendByIndex( i, k_EFriendFlagImmediate );
					
					const char *pszName = steamapicontext->SteamFriends()->GetFriendPersonaName( friendSteamID );
					int idx = m_EntireFriendsList.AddToTail();
					partner_info_t &info = m_EntireFriendsList[idx];
					info.m_steamID = friendSteamID;
					info.m_name = pszName;

					msg.Body().add_account_ids( friendSteamID.GetAccountID() );
				}
				GCClientSystem()->BSendMessage( msg );
			}
			else
			{
				m_bRefreshing = false;
			}
		}
	}

	// data
	bool m_bRefreshing;
	int m_iNumFriends;
	CUtlVector<partner_info_t>	m_EntireFriendsList;
	CUtlVector<partner_info_t>	m_FriendsWhoOwnTF2;
};

static vgui::DHANDLE<CSelectMostHelpfulFriendDialog> g_hSelectMostHelpfulFriendDialog;

/**
 *   Notification that the player should choose their most helpful friend.
 */
class CSelectHelpfulFriendNotification : public CEconNotification
{
public:
	CSelectHelpfulFriendNotification() {}

	virtual void Accept()
	{
		OpenSelectMostHelpfulFriendDialog( NULL );
		MarkForDeletion();
	}

	virtual void Decline()
	{
		MarkForDeletion();
	}

	virtual EType NotificationType() { return eType_AcceptDecline; }

	void Trigger()
	{
		OpenSelectMostHelpfulFriendDialog( NULL );
		MarkForDeletion();
	}

	static bool RemoveOtherNotifications( CEconNotification *pNotification )
	{
		return dynamic_cast< CSelectHelpfulFriendNotification* >( pNotification ) != NULL;
	}

};

class CWasThankedBySomeoneNotification : public CEconNotification
{
public:
	CWasThankedBySomeoneNotification( const CSteamID& steamID )
	{
		SetText( "#TF_Trial_Alert_ThankedBySomeone" );
		SetLifetime( 30.0f );

		{
			extern void GetPlayerNameBySteamID( const CSteamID &steamID, OUT_Z_CAP(maxLenInChars) char *pDestBuffer, int maxLenInChars );

			wchar_t wszPlayerName[ MAX_PLAYER_NAME_LENGTH ];
			char szPlayerName[ MAX_PLAYER_NAME_LENGTH ];
			GetPlayerNameBySteamID( steamID, szPlayerName, sizeof( szPlayerName ) );
			g_pVGuiLocalize->ConvertANSIToUnicode( szPlayerName, wszPlayerName, sizeof( wszPlayerName ) );
			AddStringToken( "thanker", wszPlayerName );
		}
	}

	static bool RemoveOtherNotifications( CEconNotification *pNotification )
	{
		return dynamic_cast< CWasThankedBySomeoneNotification* >( pNotification ) != NULL;
	}
};

#ifdef _DEBUG
CON_COMMAND( cl_free_trial_select_friend, "Bring up dialog to select most helpful friend" )
{
	OpenSelectMostHelpfulFriendDialog( NULL );
}

CON_COMMAND( cl_thanks_test, "Tests the thanked ui notification." )
{
	if ( steamapicontext == NULL || steamapicontext->SteamUser() == NULL )
		return;

	CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
	NotificationQueue_Add( new CWasThankedBySomeoneNotification( steamID ) );
}
#endif

class CGCRequestTF2FriendsResponse : public GCSDK::CGCClientJob
{
public:
	CGCRequestTF2FriendsResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgTFRequestTF2FriendsResponse> msg( pNetPacket );
		if ( g_hSelectMostHelpfulFriendDialog.Get() )
		{
			g_hSelectMostHelpfulFriendDialog->OnTF2FriendsReceived( msg );
		}
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCRequestTF2FriendsResponse, "CGCRequestTF2FriendsResponse", k_EMsgGCRequestTF2FriendsResponse, GCSDK::k_EServerTypeGCClient );

class CGCThankedBySomeone : public GCSDK::CGCClientJob
{
public:
	CGCThankedBySomeone( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgTFThankedBySomeone> msg( pNetPacket );
		NotificationQueue_Add( new CWasThankedBySomeoneNotification( CSteamID( msg.Body().thanker_steam_id() ) ) );
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCThankedBySomeone, "CGCThankedBySomeone", k_EMsgGCFreeTrial_ThankedBySomeone, GCSDK::k_EServerTypeGCClient );

class CGCThankedSomeone : public GCSDK::CGCClientJob
{
public:
	CGCThankedSomeone( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		ShowMessageBox( "#TF_Trial_ThankSuccess_Title",  "#TF_Trial_ThankSuccess_Text", "#GameUI_OK" );
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCThankedSomeone, "CGCThankedSomeone", k_EMsgGCFreeTrial_ThankedSomeone, GCSDK::k_EServerTypeGCClient );

class CGCFreeTrialConvertedToPremium : public GCSDK::CGCClientJob
{
public:
	CGCFreeTrialConvertedToPremium( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		ShowMessageBox( "#TF_Trial_Converted_Title",  "#TF_Trial_Converted_Text", "#GameUI_OK" );
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCFreeTrialConvertedToPremium, "CGCFreeTrialConvertedToPremium", k_EMsgGCFreeTrial_ConvertedToPremium, GCSDK::k_EServerTypeGCClient );


//-----------------------------------------------------------------------------
// External API

#if _DEBUG
ConVar tf_forcetrialaccount( "tf_forcetrialaccount", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
#endif


bool IsFreeTrialAccount()
{
#if _DEBUG
	if ( tf_forcetrialaccount.GetBool() )
		return true;
#endif

	if ( InventoryManager() && TFInventoryManager()->GetLocalTFInventory() && TFInventoryManager()->GetLocalTFInventory()->GetSOC() )
	{
		CEconGameAccountClient *pGameAccountClient = TFInventoryManager()->GetLocalTFInventory()->GetSOC()->GetSingleton<CEconGameAccountClient>();
		if ( pGameAccountClient )
			return pGameAccountClient->Obj().trial_account();
	}
	return false;
}

bool NeedsToChooseMostHelpfulFriend()
{
	{
		if ( InventoryManager() && TFInventoryManager()->GetLocalTFInventory() && TFInventoryManager()->GetLocalTFInventory()->GetSOC() )
		{
			CEconGameAccountClient *pGameAccountClient = TFInventoryManager()->GetLocalTFInventory()->GetSOC()->GetSingleton<CEconGameAccountClient>();
			if ( pGameAccountClient )
			{
				return !pGameAccountClient->Obj().trial_account()
					&& pGameAccountClient->Obj().need_to_choose_most_helpful_friend();
			}
		}
	}
	return false;
}

void NotifyNeedsToChooseMostHelpfulFriend()
{
	// remove duplicates
	NotificationQueue_Remove( &CSelectHelpfulFriendNotification::RemoveOtherNotifications );
	// add new notification
	CSelectHelpfulFriendNotification *pNotification = new CSelectHelpfulFriendNotification();
	pNotification->SetText( "#TF_Trial_Alert_SelectFriend" );
	pNotification->SetLifetime( 120.0f );
	NotificationQueue_Add( pNotification );
}

CSelectPlayerDialog *OpenSelectMostHelpfulFriendDialog( vgui::Panel *pParent )
{
	if (!g_hSelectMostHelpfulFriendDialog.Get())
	{
		g_hSelectMostHelpfulFriendDialog = vgui::SETUP_PANEL( new CSelectMostHelpfulFriendDialog( pParent ) );
	}
	g_hSelectMostHelpfulFriendDialog->InvalidateLayout( false, true );
	g_hSelectMostHelpfulFriendDialog->Reset();
	g_hSelectMostHelpfulFriendDialog->SetVisible( true );
	g_hSelectMostHelpfulFriendDialog->MakePopup();
	g_hSelectMostHelpfulFriendDialog->MoveToFront();
	g_hSelectMostHelpfulFriendDialog->SetKeyBoardInputEnabled(true);
	g_hSelectMostHelpfulFriendDialog->SetMouseInputEnabled(true);
	TFModalStack()->PushModal( g_hSelectMostHelpfulFriendDialog );

	return g_hSelectMostHelpfulFriendDialog;
}

