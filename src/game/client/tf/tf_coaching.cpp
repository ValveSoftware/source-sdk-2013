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
#include "econ_game_account_client.h"
#include "tf_gcmessages.h"
#include "gc_clientsystem.h"

// ui related
#include "ienginevgui.h"
#include "confirm_dialog.h"
#include "tf_controls.h"
#include "econ_notifications.h"
#include "select_player_dialog.h"
#include "vgui_avatarimage.h"

// for hud element
#include "iclientmode.h"

// misc
#include "c_tf_freeaccount.h"
#include "c_tf_player.h"
#include "c_playerresource.h"
#include "tf_hud_statpanel.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------

static bool g_bHadCoach = false;
static bool g_bCanLikeCoach = false;

//-----------------------------------------------------------------------------

// used by the waiting dialogs
class CCoachingWaitDialog : public CGenericWaitingDialog
{
public:
	CCoachingWaitDialog() : CGenericWaitingDialog( NULL )
	{
	}

protected:
	virtual void OnTimeout()
	{
		ShowMessageBox( "#TF_Coach_Timeout_Title", "#TF_Coach_Timeout_Text", "#GameUI_OK" );
	}
};

//-----------------------------------------------------------------------------

static bool BInCoachesList()
{
	if ( InventoryManager() && TFInventoryManager()->GetLocalTFInventory() && TFInventoryManager()->GetLocalTFInventory()->GetSOC() )
	{
		CEconGameAccountClient *pGameAccountClient = TFInventoryManager()->GetLocalTFInventory()->GetSOC()->GetSingleton<CEconGameAccountClient>();
		if ( pGameAccountClient )
			return pGameAccountClient->Obj().in_coaches_list();
	}
	return false;
}

// send a message to the GC requesting to be added to the list of coaches
static void RequestAddToCoaches()
{
	GCSDK::CProtoBufMsg< CMsgTFCoaching_AddToCoaches > msg( k_EMsgGCCoaching_AddToCoaches );
	bool bSent = GCClientSystem()->BSendMessage( msg );
	if ( bSent )
	{
		ShowWaitingDialog( new CCoachingWaitDialog(), "#TF_Coach_WaitingForServer", true, false, 20.0f );
	}
}

// send a message to the GC requesting to be removed from the list of coaches
static void RequestRemoveFromCoaches()
{
	GCSDK::CProtoBufMsg< CMsgTFCoaching_RemoveFromCoaches > msg( k_EMsgGCCoaching_RemoveFromCoaches );
	GCClientSystem()->BSendMessage( msg );
	ShowWaitingDialog( new CCoachingWaitDialog(), "#TF_Coach_WaitingForServer", true, false, 20.0f );
}

// alternates between asking to be added/removed from the list of coaches
static void ToggleCoachingConfirm( bool bConfirmed, void *pContext )
{
	if ( bConfirmed )
	{
		if ( BInCoachesList() )
		{
			RequestRemoveFromCoaches();
		}
		else
		{
			RequestAddToCoaches();
		}
	}
}

static bool IsServerFull()
{
	int iNumPlayers = 0;
	
	for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
	{
		if ( g_PR->IsConnected( iPlayerIndex ) == false )
			continue;

		player_info_t pi;
		if ( !engine->GetPlayerInfo( iPlayerIndex, &pi ) )
			continue;

		++iNumPlayers;
	}
	return iNumPlayers >= gpGlobals->maxClients;
}

//-----------------------------------------------------------------------------

/**
 * Select player to ask to be a coach dialog.
 */
class CSelectPlayerForCoachDialog : public CSelectPlayerDialog
{
	DECLARE_CLASS_SIMPLE( CSelectPlayerForCoachDialog, CSelectPlayerDialog );
public:
	CSelectPlayerForCoachDialog();
	virtual ~CSelectPlayerForCoachDialog();
	virtual void OnSelectPlayer( const CSteamID &steamID );
	virtual void OnCommand( const char *command );
	virtual bool AllowOutOfGameFriends() { return false; }
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

protected:
	virtual const char *GetResFile() { return "resource/ui/SelectPlayerDialog_Coach.res"; } 
	bool CanAskPlayerToCoach( const CSteamID &steamID );
};

static vgui::DHANDLE< CSelectPlayerForCoachDialog > g_pSelectPlayerForCoachDialog;

CSelectPlayerForCoachDialog::CSelectPlayerForCoachDialog() 
	: CSelectPlayerDialog( NULL )
{
	g_pSelectPlayerForCoachDialog = this;
	m_bAllowSameTeam = true;
	m_bAllowOutsideServer = true;
}

CSelectPlayerForCoachDialog::~CSelectPlayerForCoachDialog()
{
	g_pSelectPlayerForCoachDialog = NULL;
}

void CSelectPlayerForCoachDialog::OnSelectPlayer( const CSteamID &steamID )
{
	if ( CanAskPlayerToCoach( steamID ) )
	{
		g_bCanLikeCoach = false;
		GCSDK::CProtoBufMsg< CMsgTFCoaching_FindCoach > msg( k_EMsgGCCoaching_FindCoach );
		msg.Body().set_account_id_friend_as_coach( steamID.GetAccountID() );
		GCClientSystem()->BSendMessage( msg );
		ShowWaitingDialog( new CCoachingWaitDialog(), "#TF_Coach_AskingFriend", true, false, 20.0f );
	}
}

void CSelectPlayerForCoachDialog::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "performmatchmaking" ) )
	{
		if ( CanAskPlayerToCoach( CSteamID() ) )
		{
			// close dialog
			OnCommand( "cancel" );
			// send message
			g_bCanLikeCoach = true;
			GCSDK::CProtoBufMsg< CMsgTFCoaching_FindCoach > msg( k_EMsgGCCoaching_FindCoach );
			GCClientSystem()->BSendMessage( msg );
			ShowWaitingDialog( new CCoachingWaitDialog(), "#TF_Coach_CoachSearching", true, false, 20.0f );
		}
		return;
	}
	BaseClass::OnCommand( command );
}

void CSelectPlayerForCoachDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	CSelectPlayerDialog::ApplySchemeSettings( pScheme );
	SetDialogVariable( "title", g_pVGuiLocalize->Find( "TF_FindCoachDialog_Title" ) );
}

bool CSelectPlayerForCoachDialog::CanAskPlayerToCoach( const CSteamID &steamID )
{
	bool bMatchesFriend = false;
	const int iMaxPlayers = gpGlobals->maxClients;
	int iNumPlayers = 0;
	for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
	{
		if ( g_PR->IsConnected( iPlayerIndex ) == false )
			continue;

		player_info_t pi;
		if ( !engine->GetPlayerInfo( iPlayerIndex, &pi ) )
			continue;

		// friend on this server?
		if ( pi.friendsID != 0 && pi.friendsID == steamID.GetAccountID() )
		{
			bMatchesFriend = true;
		}

		++iNumPlayers;
	}
	if ( iMaxPlayers <= iNumPlayers )
	{
		ShowMessageBox( "#TF_Coach_ServerFull_Title", "#TF_Coach_ServerFull_Text", "#GameUI_OK" );
		return false;
	}
	if ( bMatchesFriend )
	{
		return true;
	}
	return steamID.GetAccountID() == 0;
}

static void ShowFindCoachDialog()
{
	CSelectPlayerForCoachDialog *pDialog = vgui::SETUP_PANEL( new CSelectPlayerForCoachDialog() );
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

// sent from main menu
CON_COMMAND( cl_coach_toggle, "Toggle coach status" )
{
	if ( IsFreeTrialAccount() )
	{
		ShowMessageBox( "#TF_Coach_FreeAccount_Title", "#TF_Coach_FreeAccount_Text", "#GameUI_OK" );
		return;
	}
	if ( BInCoachesList() )
	{
		ShowConfirmDialog( "#TF_Coach_RemoveCoach_Title", "#TF_Coach_RemoveCoach_Text", 
						   "#TF_Coach_Yes", "#TF_Coach_No", 
						   &ToggleCoachingConfirm );
	}
	else
	{
		ShowConfirmDialog( "#TF_Coach_AddCoach_Title", "#TF_Coach_AddCoach_Text", 
						   "#TF_Coach_Yes", "#TF_Coach_No", 
						   &ToggleCoachingConfirm );
	}
}

//-----------------------------------------------------------------------------

class CGCCoaching_AddToCoachesResponse : public GCSDK::CGCClientJob
{
public:
	CGCCoaching_AddToCoachesResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCStandardResponse_t> msg( pNetPacket );
		
		CloseWaitingDialog();

		if ( msg.Body().m_eResponse == k_EGCMsgResponseOK )
		{
			ShowMessageBox( "#TF_Coach_AddedCoach_Title", "#TF_Coach_AddedCoach_Text", "#GameUI_OK" );
		}
		else if ( msg.Body().m_eResponse == k_EGCMsgResponseDenied )
		{
			ShowMessageBox( "#TF_Coach_Denied_Title", "#TF_Coach_Denied_Text", "#GameUI_OK" );
		}
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCCoaching_AddToCoachesResponse, "CGCCoaching_AddToCoachesResponse", k_EMsgGCCoaching_AddToCoachesResponse, GCSDK::k_EServerTypeGCClient );

//-----------------------------------------------------------------------------

class CGCCoaching_RemoveFromCoachesResponse : public GCSDK::CGCClientJob
{
public:
	CGCCoaching_RemoveFromCoachesResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCStandardResponse_t> msg( pNetPacket );
		
		CloseWaitingDialog();

		if ( msg.Body().m_eResponse == k_EGCMsgResponseOK )
		{
			ShowMessageBox( "#TF_Coach_RemovedCoach_Title", "#TF_Coach_RemovedCoach_Text", "#GameUI_OK" );
		}
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCCoaching_RemoveFromCoachesResponse, "CGCCoaching_RemoveFromCoachesResponse", k_EMsgGCCoaching_RemoveFromCoachesResponse, GCSDK::k_EServerTypeGCClient );

//-----------------------------------------------------------------------------

class CGCCoaching_RemovedAsCoach : public GCSDK::CGCClientJob
{
public:
	CGCCoaching_RemovedAsCoach( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		ShowMessageBox( "#TF_Coach_SessionEnded_Title", "#TF_Coach_SessionEnded_Text", "#GameUI_OK" );
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCCoaching_RemovedAsCoach, "CGCCoaching_RemovedAsCoach", k_EMsgGCCoaching_RemoveCurrentCoach, GCSDK::k_EServerTypeGCClient );

static void FindCoach( bool bConfirmed, void *pContext )
{
	if ( bConfirmed )
	{
		ShowFindCoachDialog();
	}
}

static void PromptFindCoach()
{
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalTFPlayer == NULL )
	{
		ShowMessageBox( "#TF_Coach_NotInGame_Title", "#TF_Coach_NotInGame_Text", "#GameUI_OK" );
	}
	else if ( pLocalTFPlayer->m_hCoach != NULL )
	{
		ShowMessageBox( "#TF_Coach_AlreadyBeingCoached_Title", "#TF_Coach_AlreadyBeingCoached_Text", "#GameUI_OK" );
	}
	else if ( pLocalTFPlayer->m_hStudent != NULL )
	{
		ShowMessageBox( "#TF_Coach_AlreadyCoaching_Title", "#TF_Coach_AlreadyCoaching_Text", "#GameUI_OK" );
	}
	else if ( TFGameRules() && TFGameRules()->IsInTraining() )
	{
		ShowMessageBox( "#TF_Coach_Training_Title", "#TF_Coach_Training_Text", "#GameUI_OK" );
	}
	else if ( IsServerFull() )
	{
		ShowMessageBox( "#TF_Coach_ServerFull_Title", "#TF_Coach_ServerFull_Text", "#GameUI_OK" );
	}
	else if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		ShowMessageBox( "#TF_Coach_MannVsMachine_Title", "#TF_Coach_MannVsMachine_Text", "#GameUI_OK" );
	}
	else
	{
		ShowConfirmDialog( "#TF_Coach_AskStudent_Title", "#TF_Coach_AskStudent_Text", 
						   "#TF_Coach_Yes", "#TF_Coach_No", 
						   &FindCoach );
	}
}

CON_COMMAND( cl_coach_find_coach, "Request a coach for the current game" )
{
	PromptFindCoach();
}

class CGCCoaching_FindCoachResponse : public GCSDK::CGCClientJob
{
public:
	CGCCoaching_FindCoachResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg< CMsgTFCoaching_FindCoachResponse > msg( pNetPacket );

		CloseWaitingDialog();

		if ( msg.Body().found_coach() )
		{
			const char* pText = "#TF_Coach_FoundCoach_Text";
			if ( msg.Body().num_likes() > 2 )
			{
				pText = "#TF_Coach_FoundCoachLike_Text";
			}
			CTFMessageBoxDialog *pDialog = ShowMessageBox( "#TF_Coach_FoundCoach_Title", pText, "#GameUI_OK" );
			if ( pDialog )
			{
				wchar_t szPlayerName[MAX_PLAYER_NAME_LENGTH];
				g_pVGuiLocalize->ConvertANSIToUnicode( msg.Body().coach_name().c_str(), szPlayerName, sizeof(szPlayerName) );
				pDialog->AddStringToken( "coachname", szPlayerName );

				wchar_t szNumLikes[32];
				V_snwprintf( szNumLikes, ARRAYSIZE(szNumLikes), L"%d", msg.Body().num_likes() );
				pDialog->AddStringToken( "numlikes", szNumLikes );
			}
		}
		else
		{
			// retry?
			ShowConfirmDialog( "#TF_Coach_StudentRetry_Title", "#TF_Coach_StudentRetry_Text", 
							   "#TF_Coach_Yes", "#TF_Coach_No", 
							   &FindCoach );
		}		
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCCoaching_FindCoachResponse, "CGCCoaching_FindCoachResponse", k_EMsgGCCoaching_FindCoachResponse, GCSDK::k_EServerTypeGCClient );

//-----------------------------------------------------------------------------

class CTFAskCoachNotification : public CEconNotification
{
public:
	CTFAskCoachNotification( bool bStudentIsFriend )
		: CEconNotification()
		, m_bStudentIsFriend( bStudentIsFriend )
	{
		SetLifetime( 20.0f );
		SetText( bStudentIsFriend ? "#TF_Coach_AskCoachForFriend_Text" : "#TF_Coach_AskCoach_Text" );
	}

	virtual EType NotificationType() { return eType_AcceptDecline; }

	// XXX(JohnS): Dead code? This notification type was accept/decline, so how was it being triggered?
	virtual void Trigger()
	{
		// prompt coach
		ShowConfirmDialog( "#TF_Coach_AskCoach_Title",
		                   m_bStudentIsFriend ? "#TF_Coach_AskCoachForFriend_Text" : "#TF_Coach_AskCoach_Text",
		                   "#TF_Coach_Yes", "#TF_Coach_No",
		                   &AskCoachCallback );
	}

	virtual void Accept()
	{
		AskCoachCallback( true, this );
	}
	virtual void Decline()
	{
		AskCoachCallback( false, this );
	}
	static void AskCoachCallback( bool bConfirmed, void *pContext )
	{
		CTFAskCoachNotification *pNotification = (CTFAskCoachNotification*)pContext;
		GCSDK::CProtoBufMsg< CMsgTFCoaching_AskCoachResponse > msg( k_EMsgGCCoaching_AskCoachResponse );
		msg.Body().set_accept_coaching_assignment( bConfirmed );
		GCClientSystem()->BSendMessage( msg );
		if ( bConfirmed )
		{
			ShowWaitingDialog( new CCoachingWaitDialog(), "#TF_Coach_JoiningStudent", true, false, 30.0f );
		}
		pNotification->MarkForDeletion();
	}

protected:
	bool m_bStudentIsFriend;
};

//-----------------------------------------------------------------------------

class CGCCoaching_AskCoach : public GCSDK::CGCClientJob
{
public:
	CGCCoaching_AskCoach( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}
	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg< CMsgTFCoaching_AskCoach > msg( pNetPacket );
		if ( steamapicontext && steamapicontext->SteamUtils() )
		{
			bool bStudentIsFriend = msg.Body().has_student_is_friend() && msg.Body().student_is_friend();
			CTFAskCoachNotification *pNotification = new CTFAskCoachNotification( bStudentIsFriend );
			if ( bStudentIsFriend )
			{
				wchar_t wszPlayerName[ MAX_PLAYER_NAME_LENGTH ];
				g_pVGuiLocalize->ConvertANSIToUnicode( InventoryManager()->PersonaName_Get( msg.Body().account_id_student() ), wszPlayerName, sizeof( wszPlayerName ) );
				pNotification->AddStringToken( "friend", wszPlayerName );
			}
			CSteamID steamID( msg.Body().account_id_student(), 1, GetUniverse(), k_EAccountTypeIndividual );
			pNotification->SetSteamID( steamID );
			NotificationQueue_Add( pNotification );
		}
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCCoaching_AskCoach, "CGCCoaching_AskCoach", k_EMsgGCCoaching_AskCoach, GCSDK::k_EServerTypeGCClient );

//-----------------------------------------------------------------------------

class CGCCoaching_CoachJoinGame : public GCSDK::CGCClientJob
{
public:
	CGCCoaching_CoachJoinGame( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg< CMsgTFCoaching_CoachJoinGame > msg( pNetPacket );

		CloseWaitingDialog();

		if ( msg.Body().join_game() )
		{
			if ( msg.Body().has_server_address() && msg.Body().has_server_port() )
			{
				// join the game
				uint32 iAddress = msg.Body().server_address();
				uint16 iPort = msg.Body().server_port();
				char command[256];
				Q_snprintf( command, sizeof(command), "connect %d.%d.%d.%d:%d coaching\n", (iAddress >> 24) & 0xff, (iAddress >> 16) & 0xff, (iAddress >> 8) & 0xff, iAddress & 0xff, iPort);
				engine->ClientCmd_Unrestricted( command );
			}
		}
		else
		{
			ShowMessageBox( "#TF_Coach_JoinFail_Title", "#TF_Coach_JoinFail_Text", "#GameUI_OK" );
		}
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCCoaching_CoachJoinGame, "CGCCoaching_CoachJoinGame", k_EMsgGCCoaching_CoachJoinGame, GCSDK::k_EServerTypeGCClient );

class CGCCoaching_AlreadyRatedCoach : public GCSDK::CGCClientJob
{
public:
	CGCCoaching_AlreadyRatedCoach( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		ShowMessageBox( "#TF_Coach_AlreadyRatedCoach_Title", "#TF_Coach_AlreadyRatedCoach_Text", "#GameUI_OK" );
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCCoaching_AlreadyRatedCoach, "CGCCoaching_AlreadyRatedCoach", k_EMsgGCCoaching_AlreadyRatedCoach, GCSDK::k_EServerTypeGCClient );


//-----------------------------------------------------------------------------

static void LikeCoachCallback( bool bConfirmed, void *pContext )
{
	GCSDK::CProtoBufMsg< CMsgTFCoaching_LikeCurrentCoach > msg( k_EMsgGCCoaching_LikeCurrentCoach );
	msg.Body().set_like_coach( bConfirmed );
	GCClientSystem()->BSendMessage( msg );
	g_bCanLikeCoach = false;
}

static void PromptIfLikeCoach()
{
	if ( g_bCanLikeCoach )
	{
		ShowConfirmDialog( "#TF_Coach_LikeCoach_Title", "#TF_Coach_LikeCoach_Text", 
						   "#TF_Coach_Yes", "#TF_Coach_No", 
						   &LikeCoachCallback );
	}
}

void CL_Coaching_LevelShutdown()
{
	if ( g_pSelectPlayerForCoachDialog )
	{
		g_pSelectPlayerForCoachDialog->OnCommand( "cancel" );
	}
	bool bHadCoach = g_bHadCoach;
	g_bHadCoach = false;
	if ( bHadCoach )
	{
		PromptIfLikeCoach();
	}
}

//-----------------------------------------------------------------------------

ConVar tf_coach_min_time_played( "tf_coach_min_time_played", "7200", FCVAR_CLIENTDLL | FCVAR_HIDDEN );
ConVar tf_coach_request_nevershowagain( "tf_coach_request_nevershowagain", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_HIDDEN );

// @return true if the current player has played less than a certain threshold of hours and 
// so is deemed eligible for coaching
static bool Coaching_ShouldRequestCoach()
{
	// cannot request a coach while in training
	if ( TFGameRules() && TFGameRules()->IsInTraining() )
	{
		return false;
	}
	// cannot request a coach if server is full
	if ( IsServerFull() )
	{
		return false;
	}

	return false;

	// Grab generic stats and add time played to total time played
	int totalTimePlayed = 0;
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
	{		
		ClassStats_t &classStats = CTFStatPanel::GetClassStats( iClass );
		totalTimePlayed += classStats.accumulated.m_iStat[TFSTAT_PLAYTIME] + classStats.accumulatedMVM.m_iStat[TFSTAT_PLAYTIME];
	}

	// Better to not risk losing slots needed for bot opponents to coaches, as this could
	// inadvertently reduce difficulty and/or induce undefined behaviors/conditions
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		return false;
	}

	// check how many hours this player has played
	const int minTimePlayedForCoachingEligibility = tf_coach_min_time_played.GetInt();	
	return ( totalTimePlayed < minTimePlayedForCoachingEligibility );
}

// If the player is eligible for coaching, then prompt them
void Coaching_CheckIfEligibleForCoaching()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer == NULL )
	{
		return;
	}

	// first time only...better way to do this?
	if ( pLocalPlayer->GetTeamNumber() != TEAM_UNASSIGNED )
	{
		return;
	}

	if ( !tf_coach_request_nevershowagain.GetBool() && Coaching_ShouldRequestCoach() )
	{
		ShowConfirmOptOutDialog( "#TF_Coach_AskStudent_Title", "#TF_Coach_AskStudent_Text", 
								 "#TF_Coach_Yes", "#TF_Coach_No", 
								 "#TF_Coach_AskStudent_DoNotShowAgain", "tf_coach_request_nevershowagain",
								 &FindCoach );
	}
}

//-----------------------------------------------------------------------------

class CCoachedByPanel : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CCoachedByPanel, vgui::EditablePanel );
public:
	CCoachedByPanel( const char *pElementName ) 
		: CHudElement( pElementName )
		, BaseClass( NULL, "CoachedByPanel" )
		, m_bCanLikeCoach( false )
	{
		vgui::Panel *pParent = g_pClientMode->GetViewport();
		SetParent( pParent );

		SetHiddenBits( HIDEHUD_MISCSTATUS );

		ListenForGameEvent( "localplayer_changeteam" );
		ListenForGameEvent( "player_changename" );
	}

	virtual ~CCoachedByPanel()
	{

	}

	virtual bool ShouldDraw( void )
	{
		if ( !CHudElement::ShouldDraw() )
		{
			return false;
		}

		C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalTFPlayer == NULL || pLocalTFPlayer->m_hCoach == NULL || pLocalTFPlayer->GetObserverMode() > OBS_MODE_NONE )
		{
			return false;
		}

		if ( !IsVisible() )
		{
			g_bHadCoach = true;
			m_bCanLikeCoach = g_bCanLikeCoach;
			UpdateUI();
			InvalidateLayout();
		}
		if ( m_bCanLikeCoach != g_bCanLikeCoach )
		{
			m_bCanLikeCoach = g_bCanLikeCoach;
			UpdateUI();
		}

		return true;
	}

	virtual void PerformLayout( void )
	{
		int iXIndent = XRES(5);
		int iXPostdent = XRES(10);
		int iWidth = iXIndent + iXPostdent;

		int iTextW, iTextH;

		C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( m_pCoachNameLabel && pLocalTFPlayer)
		{
			m_pCoachNameLabel->GetContentSize( iTextW, iTextH );
			iWidth += MAX( iTextW, m_minCoachNameLabelWidth );
			m_pCoachNameLabel->SetWide( iTextW );

			if ( m_pAvatar )
			{
				iWidth += m_pAvatar->GetWide();
			}

			SetSize( iWidth, GetTall() );

			if ( m_pBGPanel_Blue )
			{
				m_pBGPanel_Blue->SetSize( iWidth, GetTall() );
			}

			if ( m_pBGPanel_Red )
			{
				m_pBGPanel_Red->SetSize( iWidth, GetTall() );
			}

			if ( m_pBGPanel_Blue && m_pBGPanel_Red )
			{
				bool bRed = ( pLocalTFPlayer->GetTeamNumber() == TF_TEAM_RED );
				m_pBGPanel_Blue->SetVisible( !bRed );
				m_pBGPanel_Red->SetVisible( bRed );
			}
		}
	}

	virtual void ApplySchemeSettings( vgui::IScheme *scheme )
	{
		LoadControlSettings( "resource/UI/CoachedByPanel.res" );

		BaseClass::ApplySchemeSettings( scheme );

		m_pCoachNameLabel = dynamic_cast< vgui::Label* >( FindChildByName("CoachNameLabel") );

		m_minCoachNameLabelWidth = 0;
		if ( m_pCoachNameLabel )
		{
			m_minCoachNameLabelWidth = m_pCoachNameLabel->GetWide();
		}
		m_pBGPanel_Blue = FindChildByName("Background_Blue");
		m_pBGPanel_Red = FindChildByName("Background_Red");

		if ( m_pBGPanel_Blue )
		{
			m_pBGPanel_Blue->SetVisible( true );
		}

		m_pAvatar = dynamic_cast<CAvatarImagePanel *>( FindChildByName("AvatarImage") );

		UpdateUI();
	}

	virtual void FireGameEvent( IGameEvent *event )
	{
		C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalTFPlayer == NULL || pLocalTFPlayer->m_hCoach == NULL )
		{
			return;
		}
		const char *name = event->GetName();
		if ( FStrEq( name, "localplayer_changeteam" ) || 
			 ( FStrEq( name, "player_changename" ) && event->GetInt( "userid" ) == pLocalTFPlayer->m_hCoach->GetUserID() ) )
		{
			UpdateUI();
			InvalidateLayout();
		}
	}

	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
	{
		if ( !IsVisible() )
			return 1; // key not handled

		if ( !down )
			return 1; // key not handled

		C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalTFPlayer != NULL && pLocalTFPlayer->m_hCoach != NULL )
		{
			switch ( keynum )
			{
				case KEY_F7:
				{
					PromptIfLikeCoach();
				}
				break;
				case KEY_F8:
				{					
					GCSDK::CProtoBufMsg< CMsgTFCoaching_RemoveCurrentCoach > msg( k_EMsgGCCoaching_RemoveCurrentCoach );
					GCClientSystem()->BSendMessage( msg );
					return 0;
				}
				break;
			}
		}

		return 1; // key not handled
	}

	void UpdateUI()
	{
		C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalTFPlayer == NULL || m_pCoachNameLabel == NULL || pLocalTFPlayer->m_hCoach == NULL )
		{
			return;
		}
		
		C_TFPlayer *pCoachPlayer = pLocalTFPlayer->m_hCoach;
		const char* pCoachName = pCoachPlayer->GetPlayerName();

		wchar_t wszPlayerName[ MAX_PLAYER_NAME_LENGTH ];
		g_pVGuiLocalize->ConvertANSIToUnicode(pCoachName, wszPlayerName, sizeof( wszPlayerName ) );
		wchar_t wszText[ 256 ] = L"";
		g_pVGuiLocalize->ConstructString_safe( wszText, g_pVGuiLocalize->Find( "#TF_Coach_Coach_Prefix" ), 1, wszPlayerName );
		m_pCoachNameLabel->SetText( wszText );

		if ( m_pAvatar )
		{
			m_pAvatar->SetShouldDrawFriendIcon( false );

			if ( steamapicontext && steamapicontext->SteamUser() )
			{
				CSteamID coachSteamID;
				if ( pCoachPlayer->GetSteamID( &coachSteamID ) )
				{
					m_pAvatar->SetPlayer( coachSteamID, k_EAvatarSize64x64 );
				}
				else
				{
					m_pAvatar->ClearAvatar();
				}				
			}
		}

		SetChildPanelVisible( this, "LikeCoachLabel", m_bCanLikeCoach );
	}

protected:

	CAvatarImagePanel	*m_pAvatar;
	vgui::Label			*m_pCoachNameLabel;
	vgui::Panel			*m_pBGPanel_Blue;
	vgui::Panel			*m_pBGPanel_Red;
	int					m_minCoachNameLabelWidth;
	bool				m_bCanLikeCoach;
};

DECLARE_HUDELEMENT( CCoachedByPanel );

//-----------------------------------------------------------------------------

// @return true if the coaching panel handled the input
// false otherwise
bool CoachingHandlesKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	CCoachedByPanel *pCoachingPanel = ( CCoachedByPanel * )GET_HUDELEMENT( CCoachedByPanel );
	if ( pCoachingPanel )
	{
		return pCoachingPanel->HudElementKeyInput( down, keynum, pszCurrentBinding ) == 0;
	}
	return false;
}
