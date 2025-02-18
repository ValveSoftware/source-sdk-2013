//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "quest_notification_panel.h"
#include "vgui/ISurface.h"
#include "ienginevgui.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "basemodel_panel.h"
#include "tf_item_inventory.h"
#include "quest_log_panel.h"
#include "econ_controls.h"
#include "c_tf_player.h"
#include <vgui_controls/AnimationController.h>
#include "engine/IEngineSound.h"
#include "econ_item_system.h"
#include "tf_hud_item_progress_tracker.h"
#include "tf_spectatorgui.h"
#include "econ_quests.h"
#include "tf_quest_map_node.h"
#include "tf_quest_map_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

ConVar tf_quest_notification_line_delay( "tf_quest_notification_line_delay", "1.2", FCVAR_ARCHIVE );

extern ISoundEmitterSystemBase *soundemitterbase;
CQuestNotificationPanel *g_pQuestNotificationPanel = NULL;

DECLARE_HUDELEMENT( CQuestNotificationPanel );

CQuestNotification::CQuestNotification( const CQuestThemeDefinition* pTheme )
	: m_defID( pTheme->GetID() )
{}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CQuestNotification::Present( CQuestNotificationPanel* pNotificationPanel )
{
	m_timerDialog.Start( tf_quest_notification_line_delay.GetFloat() );

	return 0.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CQuestNotification_Speaking::CQuestNotification_Speaking( const CQuestThemeDefinition* pTheme )
	: CQuestNotification( pTheme )
{
	m_pszSoundToSpeak = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CQuestNotification_Speaking::Present( CQuestNotificationPanel* pNotificationPanel )
{
	CQuestNotification::Present( pNotificationPanel );

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return 0.f;

	CTFPlayer* pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return 0.f;

	const CQuestThemeDefinition* pTheme = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestThemeDefinition >( m_defID );
	Assert( pTheme );

	if ( pTheme )
	{
		// Get the sound we need to speak
		m_pszSoundToSpeak = GetSoundEntry( pTheme, pTFPlayer->GetPlayerClass()->GetClassIndex() );
		float flPresentTime = 0.f;
		if ( m_pszSoundToSpeak )
		{
			flPresentTime = enginesound->GetSoundDuration( m_pszSoundToSpeak ) + m_timerDialog.GetCountdownDuration() + 1.f;
			m_timerShow.Start( enginesound->GetSoundDuration( m_pszSoundToSpeak ) + m_timerDialog.GetCountdownDuration() + 1.f );
		}

		return flPresentTime;
	}

	return 0.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestNotification_Speaking::Update()
{
	if ( m_timerDialog.IsElapsed() && m_timerDialog.HasStarted() )
	{
		m_timerDialog.Invalidate();

		// Play it!
		if ( m_pszSoundToSpeak )
		{
			vgui::surface()->PlaySound( m_pszSoundToSpeak );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CQuestNotification_Speaking::IsDone() const
{
	return m_timerShow.IsElapsed() && m_timerShow.HasStarted();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CQuestNotification_NewQuest::GetSoundEntry( const CQuestThemeDefinition* pTheme, int nClassIndex )
{
	return pTheme->GetGiveSoundForClass( nClassIndex );
}

bool CQuestNotification_NewQuest::ShouldPresent() const
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	CTFPlayer* pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return false;

	IViewPortPanel* pSpecGuiPanel = gViewPortInterface->FindPanelByName( PANEL_SPECGUI );
	if ( !pTFPlayer->IsAlive() )
	{
		if ( !pSpecGuiPanel || !pSpecGuiPanel->IsVisible() )
			return false;
	}
	else
	{
		// Local player is in a spawn room
		if ( pTFPlayer->m_Shared.GetRespawnTouchCount() <= 0 )
			return false;
	}

	return true;
}

CQuestNotification_CompletedQuest::CQuestNotification_CompletedQuest( const CQuestThemeDefinition* pTheme )
	: CQuestNotification_Speaking( pTheme )
{
	const char *pszSoundName = UTIL_GetRandomSoundFromEntry( "Quest.StatusTickComplete" );
	m_PresentTimer.Start( enginesound->GetSoundDuration( pszSoundName ) - 2.f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CQuestNotification_CompletedQuest::GetSoundEntry( const CQuestThemeDefinition* pTheme, int nClassIndex )
{
	return pTheme->GetCompleteSoundForClass( nClassIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CQuestNotification_CompletedQuest::ShouldPresent() const
{
	return m_PresentTimer.IsElapsed() && m_PresentTimer.HasStarted();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CQuestNotification_FullyCompletedQuest::GetSoundEntry( const CQuestThemeDefinition* pTheme, int nClassIndex )
{
	return pTheme->GetFullyCompleteSoundForClass( nClassIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CQuestNotificationPanel::CQuestNotificationPanel( const char *pszElementName )
	: CHudElement( pszElementName )
	, EditablePanel( NULL, "QuestNotificationPanel" )
	, m_flTimeSinceLastShown( 0.f )
	, m_bIsPresenting( false )
	, m_bInitialized( false )
	, m_pMainContainer( NULL )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	g_pQuestNotificationPanel = this;

	ListenForGameEvent( "player_spawn" );

	memset( &m_flLastNotifiedTime, 0.f, sizeof( m_flLastNotifiedTime ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CQuestNotificationPanel::~CQuestNotificationPanel()
{}




void CQuestNotificationPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// Default, load pauling
	LoadControlSettings( "Resource/UI/econ/QuestNotificationPanel_Pauling_standard.res" );

	m_pMainContainer = FindControl< EditablePanel >( "MainContainer", true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestNotificationPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	CExLabel* pNewQuestLabel = FindControl< CExLabel >( "NewQuestText", true );
	if ( pNewQuestLabel )
	{
		const wchar_t *pszText = NULL;
		const char *pszTextKey = "#QuestNotification_Accept";
		if ( pszTextKey )
		{
			pszText = g_pVGuiLocalize->Find( pszTextKey );
		}
		if ( pszText )
		{					
			wchar_t wzFinal[512] = L"";
			UTIL_ReplaceKeyBindings( pszText, 0, wzFinal, sizeof( wzFinal ) );
			pNewQuestLabel->SetText( wzFinal );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestNotificationPanel::FireGameEvent( IGameEvent * event )
{
	if ( FStrEq( event->GetName(), "player_spawn" ) )
	{
		const int iUserID = event->GetInt( "userid" );
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pLocalPlayer )
			return;

		if ( iUserID != pLocalPlayer->GetUserID() )
			return;

		CheckForAvailableNodeNotification();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestNotificationPanel::Reset()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestNotificationPanel::CheckForAvailableNodeNotification()
{
	if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() )
		return;

	// If this node is locked, requirements are met then
	// we should remind them to go unlock a node, so long as there isn't already
	// a quest active.
	if ( GetQuestMapHelper().GetActiveQuest() == NULL &&
		 GetQuestMapHelper().GetNumCurrentlyUnlockableNodes() > 0 )
	{
		AddNotification( new CQuestNotification_NewQuest( GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestThemeDefinition >( 1 ) ) ); // Super hack for now
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestNotificationPanel::AddNotification( CQuestNotification* pNotification )
{
	// Check if there's already a notification of this type
	FOR_EACH_VEC_BACK( m_vecNotifications, i )
	{
		// There's already a quest of this type in queue, no need to add another
		if ( m_vecNotifications[i]->GetType() == pNotification->GetType() )
		{
			delete pNotification;
			return;
		}
	}

	// Check if we've already done a notification for this type recently
	if ( Plat_FloatTime() < m_flLastNotifiedTime[ pNotification->GetType() ])
	{
		delete pNotification;
		return;
	}

	// Add notification
	m_vecNotifications.AddToTail( pNotification );
	// Mark that we've created a notification of this type for this item
	m_flLastNotifiedTime[ pNotification->GetType() ] = Plat_FloatTime() + pNotification->GetReplayTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CQuestNotificationPanel::ShouldDraw()
{
	return false;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	CTFPlayer* pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return false;

	// Not selected a class, so they haven't joined in
	if ( pTFPlayer->IsPlayerClass( 0 ) )
		return false;

	if ( !CHudElement::ShouldDraw() )
		return false;

	if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestNotificationPanel::OnThink()
{
	if ( !ShouldDraw() )
		return;

	bool bHasStarted = m_animTimer.HasStarted();
	float flShowProgress = bHasStarted ? 1.f : 0.f;
	const float flTransitionTime = 0.5f;

	Update();
	
	if ( bHasStarted )
	{
		// Transitions
		if ( m_animTimer.GetElapsedTime() < flTransitionTime )
		{
			flShowProgress = Bias( m_animTimer.GetElapsedTime() / flTransitionTime, 0.75f );
		}
		else if ( ( m_animTimer.GetRemainingTime() + 1.f ) < flTransitionTime )
		{
			flShowProgress = Bias( Max( 0.0f, m_animTimer.GetRemainingTime() + 1.f ) / flTransitionTime, 0.25f );
		}
	}
	
	// Move the main container around
	if ( m_pMainContainer )
	{
		int nY = g_pSpectatorGUI && g_pSpectatorGUI->IsVisible() ? g_pSpectatorGUI->GetTopBarHeight() : 0;

		float flXPos = RemapValClamped( flShowProgress, 0.f, 1.f, 0.f, m_pMainContainer->GetWide() + XRES( 4 ) );
		m_pMainContainer->SetPos( GetWide() - (int)flXPos, nY );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CQuestNotificationPanel::ShouldPresent()
{
	// Suppress making new notifications while in competitive play
	if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() )
		return false;

	if ( !m_timerNotificationCooldown.IsElapsed() )
		return false;

	// We need notifications!
	if ( m_vecNotifications.IsEmpty() )
		return false;

	// It's been a few seconds since we were last shown
	if ( ( Plat_FloatTime() - m_flTimeSinceLastShown ) < 1.5f )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CQuestNotificationPanel::Update()
{
	bool bAllowedToShow = ShouldPresent();

	if ( bAllowedToShow && !m_bIsPresenting )
	{
		if ( m_vecNotifications.Head()->ShouldPresent() )
		{
			float flPresentTime = m_vecNotifications.Head()->Present( this );
			m_animTimer.Start( flPresentTime );

			m_timerHoldUp.Start( 3.f );

			// Notification sound
			vgui::surface()->PlaySound( "ui/quest_alert.wav" );
			m_bIsPresenting = true;
		}
	}
	else if ( !bAllowedToShow && m_bIsPresenting && m_timerHoldUp.IsElapsed() )
	{
		m_flTimeSinceLastShown = Plat_FloatTime();
		// Play the slide-out animation
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "QuestNotification_Hide" );
		m_bIsPresenting = false;
	}
	else if ( m_bIsPresenting ) // We are presenting a notification
	{
		if ( m_vecNotifications.Count() )
		{
			m_vecNotifications.Head()->Update();
			// Check if the notification is done
			if ( m_vecNotifications.Head()->IsDone() )
			{
				// Start our cooldown
				m_timerNotificationCooldown.Start( 1.f );
				// We're done with this notification
				delete m_vecNotifications.Head();
				m_vecNotifications.Remove( 0 );
			}
		}
	}
}
