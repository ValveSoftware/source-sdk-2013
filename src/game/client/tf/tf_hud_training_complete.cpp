//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include <vgui/IScheme.h>
#include "hud.h"
#include "hudelement.h"
#include "tf_hud_statpanel.h"
#include "tf_spectatorgui.h"
#include "vgui_controls/AnimationController.h"
#include "iclientmode.h"
#include "engine/IEngineSound.h"
#include "c_tf_playerresource.h"
#include "c_team.h"
#include "tf_clientscoreboard.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "vgui/IInput.h"
#include "vgui_avatarimage.h"
#include "fmtstr.h"
#include "teamplayroundbased_gamerules.h"
#include "tf_gamerules.h"
#include "tf_hud_training.h"
#include "tf_hud_mainmenuoverride.h"
#include "achievementmgr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

static const int TEMP_STRING_SIZE = 256;
static const float UPDATE_DELAY = 0.1f;
static const float DELAY_TO_SHOW_BUTTONS = 5.0f;

extern CAchievementMgr g_AchievementMgrTF;	// global achievement mgr for TF

class CTFTrainingComplete : public EditablePanel, public CHudElement
{
private:
	DECLARE_CLASS_SIMPLE( CTFTrainingComplete, EditablePanel );

public:
	CTFTrainingComplete( const char *pElementName );

	virtual void Reset();
	virtual void Init();
	virtual void PerformLayout();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void FireGameEvent( IGameEvent * event );
	virtual void OnThink();
	virtual bool ShouldDraw( void );
	virtual void SetVisible( bool value );

	virtual int GetRenderGroupPriority() { return 70; }
protected:
	
	// vgui overrides
	virtual void OnCommand( const char *command );

private:
	void SetUpResults( IGameEvent * event );

	CExButton		*m_pReplay;
	CExButton		*m_pNext;
	CExButton		*m_pQuit;
	ImagePanel		*m_pTopBar;
	ImagePanel		*m_pBottomBar;
	EditablePanel	*m_ResultsPanel;

	int m_iReplayY;
	int m_iNextY;
	int m_iBottomBarY;
	int m_iTopBarY;

	bool	m_bShouldBeVisible;
	float	m_showButtonsTime;
};


DECLARE_HUDELEMENT_DEPTH( CTFTrainingComplete, 1 );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFTrainingComplete::CTFTrainingComplete( const char *pElementName ) : EditablePanel( NULL, "TrainingComplete" ), CHudElement( pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	m_bShouldBeVisible = false;
	m_showButtonsTime = 0.0f;

	m_ResultsPanel = NULL;
	m_pReplay = NULL;
	m_pNext = NULL;
	m_pQuit = NULL;
	m_pTopBar = NULL;
	m_pBottomBar = NULL;
	m_iReplayY = 0;
	m_iNextY = 0;
	m_iBottomBarY = 0;
	m_iTopBarY = 0;

	SetScheme( "ClientScheme" );

	MakePopup();

	RegisterForRenderGroup( "mid" );

}

void CTFTrainingComplete::SetUpResults( IGameEvent *event )
{
	m_ResultsPanel = dynamic_cast<EditablePanel *>( FindChildByName( "Results" ) );

	const char *map = event->GetString( "map" );
	const char *nextMap = event->GetString( "next_map" );
	const char *endText = event->GetString( "text" );
	bool bHasNextMap = Q_stricmp( nextMap, "" ) != 0;

	// title
	{
		wchar_t outputText[MAX_TRAINING_MSG_LENGTH];
		CTFHudTraining::FormatTrainingText(bHasNextMap ? "#TF_Training_Success" : "#TF_Training_Completed" , outputText);
		m_ResultsPanel->SetDialogVariable( "wintext", outputText);
	}

	// record that the player has completed training with the current class
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer && !V_stricmp(map, "tr_target" ) )
	{
		Training_MarkClassComplete( pLocalPlayer->GetPlayerClass()->GetClassIndex(), 1 );
	}
	else if ( !V_stricmp(map, "tr_dustbowl" ) )
	{
		Training_MarkClassComplete( TF_CLASS_SOLDIER, 2 );
	}
	else
	{
		Warning( "Completed a training that we don't recognize!?\n" );
		Assert( false );
	}

	// Set the text to show to the user
	CExRichText *pRichText = dynamic_cast<CExRichText *>(FindChildByName( "ResultsText", true ) );
	if ( pRichText )
	{
		wchar_t wsText_LastMap[MAX_TRAINING_MSG_LENGTH];
		wchar_t wsText_NextMap[MAX_TRAINING_MSG_LENGTH];
#ifdef WIN32
		V_swprintf_safe( wsText_LastMap, L"%S", GetMapDisplayName( map ) );
		V_swprintf_safe( wsText_NextMap, L"%S", GetMapDisplayName( nextMap ) );
#else
		// GetMapDisplayName returns char * which is %s, NOT %S, on Posix
		V_swprintf_safe( wsText_LastMap, L"%s", GetMapDisplayName( map ) );
		V_swprintf_safe( wsText_NextMap, L"%s", GetMapDisplayName( nextMap ) );		
#endif
		wchar_t wsResult[MAX_TRAINING_MSG_LENGTH];
		g_pVGuiLocalize->ConstructString_safe( wsResult, g_pVGuiLocalize->Find( endText ), 2, wsText_LastMap, wsText_NextMap );
		pRichText->SetText( wsResult );

		bHasNextMap = Q_stricmp( nextMap, "" ) != 0;
		m_pNext->SetVisible( bHasNextMap );
		m_pQuit->SetVisible( !bHasNextMap );
	}

	extern int Training_GetProgressCount();
	TFGameRules()->SetAllowTrainingAchievements( true );
	g_AchievementMgrTF.UpdateAchievement( ACHIEVEMENT_TF_COMPLETE_TRAINING, Training_GetProgressCount() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTrainingComplete::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTrainingComplete::Reset()
{
	m_bShouldBeVisible = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTrainingComplete::Init()
{
	// listen for events
	ListenForGameEvent( "training_complete" );
	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "teamplay_game_over" );
	ListenForGameEvent( "tf_game_over" );

	m_bShouldBeVisible = false;

	CHudElement::Init();
}

void CTFTrainingComplete::SetVisible( bool value )
{
	if ( value == IsVisible() )
		return;

	if ( value )
	{
		RequestFocus();
		SetKeyBoardInputEnabled( true );
		SetMouseInputEnabled( true );

		HideLowerPriorityHudElementsInGroup( "mid" );
	}
	else
	{
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( false );

		UnhideLowerPriorityHudElementsInGroup( "mid" );
	}

	BaseClass::SetVisible( value );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTrainingComplete::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "teamplay_round_start", pEventName ) == 0 )
	{
		m_bShouldBeVisible = false;
	}
	else if ( Q_strcmp( "teamplay_game_over", pEventName ) == 0 )
	{
		m_bShouldBeVisible = false;
	}
	else if ( Q_strcmp( "tf_game_over", pEventName ) == 0 )
	{
		m_bShouldBeVisible = false;
	}
	else if ( Q_strcmp( "training_complete", pEventName ) == 0 )
	{
		if ( !g_PR )
			return;

		InvalidateLayout( false, true );

		// Prevent the game from continuing until they press a button.
		tf_training_client_message.SetValue( (int)TRAINING_CLIENT_MESSAGE_IN_SUMMARY_SCREEN );

		m_showButtonsTime = gpGlobals->curtime + DELAY_TO_SHOW_BUTTONS;

		m_bShouldBeVisible = true;
									  
		SetUpResults( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Performs layout
//-----------------------------------------------------------------------------
void CTFTrainingComplete::PerformLayout()
{
	if ( m_pTopBar == NULL || m_pBottomBar == NULL || m_pReplay == NULL || m_pNext == NULL )
	{
		return;
	}
	//Get the offsets
	int dummy;
	int offset = m_pBottomBar->GetTall();
	m_pReplay->GetPos( dummy, m_iReplayY );
	m_pReplay->SetPos( dummy, m_iReplayY + offset );
	m_pNext->GetPos( dummy, m_iNextY );
	m_pNext->SetPos( dummy, m_iNextY + offset );
	m_pQuit->GetPos( dummy, m_iNextY );
	m_pQuit->SetPos( dummy, m_iNextY + offset );
	m_pBottomBar->GetPos( dummy, m_iBottomBarY );
	m_pBottomBar->SetPos( dummy, m_iBottomBarY + offset );
	m_pTopBar->GetPos( dummy, m_iTopBarY );
	m_pTopBar->SetPos( dummy, m_iTopBarY - offset );
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFTrainingComplete::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	LoadControlSettings( "resource/UI/TrainingComplete.res" );

	m_pReplay = dynamic_cast<CExButton *>( FindChildByName( "Replay" ) );
	m_pNext = dynamic_cast<CExButton *>( FindChildByName( "Next" ) );
	m_pQuit = dynamic_cast<CExButton *>( FindChildByName( "Quit" ) );
	m_pTopBar = dynamic_cast<ImagePanel *>( FindChildByName( "TopBar" ) );
	m_pBottomBar = dynamic_cast<ImagePanel *>( FindChildByName( "BottomBar" ) );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: returns whether panel should be drawn
//-----------------------------------------------------------------------------
bool CTFTrainingComplete::ShouldDraw()
{
	if ( !m_bShouldBeVisible )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: panel think method
//-----------------------------------------------------------------------------
void CTFTrainingComplete::OnThink()
{
	if ( 0.0f != m_showButtonsTime )
	{
		if( gpGlobals->curtime > m_showButtonsTime)
		{
			//After a certain amount of time, show the menu nav buttons and hide the HUD.
			m_showButtonsTime = 0.0f;

		}
	}
	else
	{
		//The menu buttons are showing.

		//Always hide the health... this needs to be done every frame because a message from the server keeps resetting this.
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer )
		{
			pLocalPlayer->m_Local.m_iHideHUD |= HIDEHUD_HEALTH;
		}

		if ( 0 != m_iBottomBarY )
		{
			static const float BLEND_AMOUNT = 0.15f;
			static const float BLEND_CONST = BLEND_AMOUNT - 1.0f;
			int x, y;
			
			//Get the dy for the bottom bar.  We'll use that for all the widgets since they're all traveling the same distance.
			m_pBottomBar->GetPos( x, y );
			int dy = m_iBottomBarY - y;
			// Note:  y = m_iBottomBarY - dy.  We use this to great advantage later to help keep the code cleaner.

			// if we have less than a pixel step, clamp us to the end.
			if ( abs(dy) < (int)( 1.0f / BLEND_AMOUNT ) )
			{
				dy = 0;
			}
			m_pBottomBar->SetPos( x, m_iBottomBarY + (int)( BLEND_CONST * (float)dy ) );

			m_pNext->GetPos( x, y );
			m_pNext->SetPos( x, m_iNextY + (int)( BLEND_CONST * (float)dy ) );

			m_pQuit->GetPos( x, y );
			m_pQuit->SetPos( x, m_iNextY + (int)( BLEND_CONST * (float)dy ) );

			m_pReplay->GetPos( x, y );
			m_pReplay->SetPos( x, m_iReplayY + (int)( BLEND_CONST * (float)dy ) );

			m_pTopBar->GetPos( x, y );
			m_pTopBar->SetPos( x, m_iTopBarY - (int)( BLEND_CONST * (float)dy ) );

			if ( dy == 0 )
			{
				//Stop condition.
				m_iBottomBarY = 0;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTrainingComplete::OnCommand( const char *command )
{
	if ( !Q_strcmp( command, "next" ) )
	{
		tf_training_client_message.SetValue( (int)TRAINING_CLIENT_MESSAGE_NEXT_MAP );
	}
	else if ( !Q_strcmp( command, "replay" ) )
	{
		tf_training_client_message.SetValue( (int)TRAINING_CLIENT_MESSAGE_REPLAY );
	}
	else if ( !Q_strcmp( command, "quit" ) )
	{
		engine->ExecuteClientCmd( "disconnect\n" );
		IViewPortPanel *pMMOverride = ( gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE ) );
		if ( pMMOverride )
		{
			((CHudMainMenuOverride*)pMMOverride)->ScheduleTrainingCheck( true );	
		}
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

#if _DEBUG
//@note Tom Bui: For testing...
CON_COMMAND( training_complete, "Test")
{
	IGameEvent *winEvent = gameeventmanager->CreateEvent( "training_complete" );
	if ( winEvent )
	{
		static bool sbTarget = true;
		winEvent->SetString( "map", "blah" );
		winEvent->SetString( "next_map", sbTarget ? "tr_dustbowl" : "" );
		winEvent->SetString( "text", sbTarget ? "#TR_Target_EndDialog" : "#TR_Dustbowl_EndDialog" );

		gameeventmanager->FireEvent( winEvent );
	}
}
#endif
