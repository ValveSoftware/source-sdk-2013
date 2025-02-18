//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/EditablePanel.h>
#include "tf_imagepanel.h"
#include "tf_gamerules.h"
#include "c_tf_team.h"
#include "tf_hud_freezepanel.h"
#include "tf_hud_teamswitch.h"
#include "hud_chat.h"
#include "usermessages.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

extern ConVar tf_arena_preround_time;

#define TF_ARENA_NOTIFICATION_HIDETIME tf_arena_preround_time.GetFloat();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudArenaNotification : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudArenaNotification, EditablePanel );

public:
	CHudArenaNotification( const char *pElementName );

	virtual void	Init( void );
	virtual void	OnTick( void );
	virtual void	LevelInit( void );
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	SetVisible( bool state );

	virtual void	FireGameEvent( IGameEvent * event );
	void			SetupSwitchPanel( int iNotification );

private:
	Label			*m_pBalanceLabel;
	float			m_flHideAt;
	CHudTeamSwitch		*m_pTeamSwitchPanel;
};

DECLARE_HUDELEMENT( CHudArenaNotification );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudArenaNotification::CHudArenaNotification( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudArenaNotification" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_flHideAt = 0;
	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
	m_pTeamSwitchPanel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaNotification::Init( void )
{
	// listen for events
	ListenForGameEvent( "arena_player_notification" );
	ListenForGameEvent( "arena_match_maxstreak" );
	ListenForGameEvent( "arena_round_start" );

	SetVisible( false );
	CHudElement::Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaNotification::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "arena_player_notification", pEventName ) == 0 )
	{
		int iPlayer = event->GetInt( "player" );
		int iNotification = event->GetInt( "message" );
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

		if ( pPlayer && iPlayer == pPlayer->entindex() )
		{
			SetupSwitchPanel( iNotification );
			m_flHideAt = gpGlobals->curtime + TF_ARENA_NOTIFICATION_HIDETIME;
			SetVisible( true );

			if ( m_pTeamSwitchPanel == NULL )
			{
				m_pTeamSwitchPanel = GET_HUDELEMENT( CHudTeamSwitch );
			}
		}
	}
	else if ( Q_strcmp( "arena_match_maxstreak", pEventName ) == 0 )
	{
		int iTeam = event->GetInt( "team" );
		int iStreak = event->GetInt( "streak" );

		C_TFTeam *pTeam = GetGlobalTFTeam( iTeam );

		if ( !pTeam )
			return;

		CBaseHudChat *hudChat = (CBaseHudChat *)GET_HUDELEMENT( CHudChat );

		wchar_t wStreak[6];
		_snwprintf( wStreak, ARRAYSIZE( wStreak ), L"%i", iStreak );

		wchar_t wszLocalized[100];
		g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#TF_Arena_MaxStreak" ), 2, pTeam->Get_Localized_Name(), wStreak );
		

		char szLocalized[100];
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof(szLocalized) );

		hudChat->ChatPrintf( 0, CHAT_FILTER_NONE, "%c%s", COLOR_NORMAL, szLocalized );
	}
	else if ( Q_strcmp( "arena_round_start", pEventName ) == 0 )
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

		if ( pPlayer )
		{
			pPlayer->EmitSound( "Ambient.Siren" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CHudArenaNotification::OnTick( void )
{
	if ( IsVisible() == true )
	{
		if ( m_flHideAt == 9999  )
		{
			C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

			if ( pPlayer && pPlayer->GetTeamNumber() > LAST_SHARED_TEAM )
			{
				if ( tf_arena_use_queue.GetBool() || TFGameRules()->State_Get() != GR_STATE_PREGAME )
				{
					m_flHideAt = 0.1f;
				}
			}
		}

		if ( ( m_flHideAt && m_flHideAt < gpGlobals->curtime ) || ( m_pTeamSwitchPanel && m_pTeamSwitchPanel->ShouldDraw() == true ) || TFGameRules()->InStalemate() == true || TFGameRules()->IsInArenaMode() == false )
		{
			SetVisible( false );
			m_flHideAt = 0;
		}
	}
	else
	{
		if ( TFGameRules() && TFGameRules()->IsInArenaMode() == true )
		{
			if ( TFGameRules()->State_Get() == GR_STATE_PREGAME )
			{
				SetupSwitchPanel( TF_ARENA_NOTIFICATION_NOPLAYERS );
				m_flHideAt = 9999;
				SetVisible( true );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaNotification::SetVisible( bool state )
{
	int iRenderGroup = gHUD.LookupRenderGroupIndexByName( "arena_target_id" );

	if ( state == true )
	{
		gHUD.LockRenderGroup( iRenderGroup );
	}
	else
	{
		gHUD.UnlockRenderGroup( iRenderGroup );
	}

	BaseClass::SetVisible( state );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaNotification::LevelInit( void )
{
	m_flHideAt = 0;
	SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudArenaNotification::ShouldDraw( void )
{
	return ( IsVisible() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaNotification::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudArenaNotification.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pBalanceLabel = dynamic_cast<Label *>( FindChildByName("BalanceLabel") );

	SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaNotification::SetupSwitchPanel( int iNotification )
{
	if ( m_pBalanceLabel )
	{
		SetDialogVariable( "notificationtip", g_pVGuiLocalize->Find( "#TF_Arena_ProTip" ) );

		if ( iNotification == TF_ARENA_NOTIFICATION_CAREFUL )
		{
			m_pBalanceLabel->SetText( g_pVGuiLocalize->Find( "#TF_Arena_Careful" ) );
		}
		else if ( iNotification == TF_ARENA_NOTIFICATION_SITOUT )
		{
			m_pBalanceLabel->SetText( g_pVGuiLocalize->Find( "#TF_Arena_SitOut" ) );
		}
		else if ( iNotification == TF_ARENA_NOTIFICATION_NOPLAYERS )
		{
			m_pBalanceLabel->SetText( g_pVGuiLocalize->Find( "#TF_Arena_Welcome" ) );
			SetDialogVariable( "notificationtip", g_pVGuiLocalize->Find( "#TF_Arena_NoPlayers" ) );
		}
	}
}

//----------------------------------------------------------------------------------------------------------------
// Receive the HudArenaNotify user message and send out a clientside event for achievements to hook.
USER_MESSAGE( HudArenaNotify )
{
	int iPlayerIndex = (int) msg.ReadByte();
	int iNotification = (int) msg.ReadByte();

	IGameEvent *event = gameeventmanager->CreateEvent( "arena_player_notification" );

	if ( event )
	{
		event->SetInt( "player", iPlayerIndex );
		event->SetInt( "message", iNotification );
		gameeventmanager->FireEventClientSide( event );
	}
}