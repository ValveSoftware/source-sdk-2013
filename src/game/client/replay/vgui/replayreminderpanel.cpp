//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replayreminderpanel.h"
#include "replay/ireplaysystem.h"
#include "replay/replay.h"
#include "replay/ireplayscreenshotmanager.h"
#include "replay/ireplaymanager.h"
#include "replay/screenshot.h"
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"

//-----------------------------------------------------------------------------

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------

DECLARE_HUDELEMENT( CReplayReminderPanel );

//-----------------------------------------------------------------------------

CReplayReminderPanel::CReplayReminderPanel( const char *pElementName )
:	EditablePanel( g_pClientMode->GetViewport(), "ReplayReminder" ),
	CHudElement( pElementName )
{
	SetScheme( "ClientScheme" );

	m_flShowTime = 0;
	m_bShouldDraw = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayReminderPanel::SetupText()
{
	// Get current key binding, if any.
	const char *pBoundKey = engine->Key_LookupBinding( "save_replay" );
	if ( !pBoundKey || FStrEq( pBoundKey, "(null)" ) )
	{
		pBoundKey = " ";
	}

	char szKey[16];
	Q_snprintf( szKey, sizeof(szKey), "%s", pBoundKey );

	wchar_t wKey[16];
	wchar_t wLabel[256];

	g_pVGuiLocalize->ConvertANSIToUnicode( szKey, wKey, sizeof( wKey ) );
	g_pVGuiLocalize->ConstructString( wLabel, sizeof( wLabel ), g_pVGuiLocalize->Find("#Replay_freezecam_replay" ), 1, wKey );

	// Set the text
	SetDialogVariable( "text", wLabel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayReminderPanel::ApplySchemeSettings( IScheme *pScheme )
{
	LoadControlSettings("Resource/UI/ReplayReminder.res", "GAME");

	BaseClass::ApplySchemeSettings( pScheme );

	SetupText();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayReminderPanel::Show()
{
	m_flShowTime = gpGlobals->curtime;
	SetVisible( true );
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( GetParent(), "HudReplayReminderIn" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayReminderPanel::Hide()
{
	SetVisible( false );
	m_flShowTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CReplayReminderPanel::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( ShouldDraw() && pszCurrentBinding )
	{
		if ( FStrEq (pszCurrentBinding, "save_replay" ) )
		{
			SetVisible( false );
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayReminderPanel::OnThink()
{
	BaseClass::OnThink();

	if ( !IsVisible() )
		return;
	
	// If we're displaying the element for some specific duration...
	if ( m_flShowTime )
	{
		// Get maximum duration
		ConVarRef replay_postwinreminderduration( "replay_postwinreminderduration" );
		float flShowLength = replay_postwinreminderduration.IsValid() ? replay_postwinreminderduration.GetFloat() : 5.0f;

		if ( gpGlobals->curtime >= m_flShowTime + flShowLength )
		{
			m_flShowTime = 0;
			SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CReplayReminderPanel::SetVisible( bool bState )
{
	if ( bState )
	{
		SetupText();
	}

	// Store this state for ShouldDraw()
	m_bShouldDraw = bState;

	BaseClass::SetVisible( bState );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CReplayReminderPanel::ShouldDraw()
{
	return m_bShouldDraw;
}

#endif // #if defined( REPLAY_ENABLED )