//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_hud_minigame.h"
#include "iclientmode.h"
#include "tf_gamerules.h"
#include <filesystem.h>
#include <vgui/IVGui.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT( CHudMiniGame );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudMiniGame::CHudMiniGame( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HudMiniGame" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pActiveMinigame = NULL;
	V_strcpy_safe( m_szResFilename, "resource/UI/HudMiniGame_Base.res" );
	
	SetHiddenBits( 0 );
	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMiniGame::ApplySchemeSettings( vgui::IScheme *scheme )
{
	// load control settings...
	LoadControlSettings( m_szResFilename );

	BaseClass::ApplySchemeSettings( scheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudMiniGame::ShouldDraw( void )
{
	if ( !CHudElement::ShouldDraw() )
		return false;

	if ( !CTFMinigameLogic::GetMinigameLogic() || !CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame() )
		return false;

	if ( TFGameRules() && ( TFGameRules()->State_Get() != GR_STATE_RND_RUNNING ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMiniGame::OnTick()
{
	if ( CTFMinigameLogic::GetMinigameLogic() )
	{
		const char *pszResFilename = NULL;
		CTFMiniGame *pActiveMinigame = CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame();

		if ( pActiveMinigame )
		{
			pszResFilename = pActiveMinigame->GetResFile();
		}

		if ( pActiveMinigame != m_pActiveMinigame )
		{
			m_pActiveMinigame = pActiveMinigame;
		}
		
		if ( pszResFilename && pszResFilename[0] && m_szResFilename && m_szResFilename[0] )	
		{
			if ( !FStrEq( pszResFilename, m_szResFilename + sizeof( "resource/UI/" ) - 1 ) )
			{
				V_sprintf_safe( m_szResFilename, "resource/UI/%s", pszResFilename );
				InvalidateLayout( false, true );
			}
		}
			
		if ( m_pActiveMinigame )
		{
			SetDialogVariable( "redscore", m_pActiveMinigame->GetScoreForTeam( TF_TEAM_RED ) );
			SetDialogVariable( "bluescore", m_pActiveMinigame->GetScoreForTeam( TF_TEAM_BLUE ) );
			SetDialogVariable( "rounds", m_pActiveMinigame->GetMaxScore() );
		}
	}
}