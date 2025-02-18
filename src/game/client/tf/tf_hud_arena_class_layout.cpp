//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "c_tf_player.h"
#include "c_tf_team.h"
#include "c_tf_playerresource.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include "tf_gamerules.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include "tf_hud_arena_class_layout.h"
#include "tf_hud_menu_spy_disguise.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

const char *g_sImagesBlue[] = {
	"",
	"class_sel_sm_scout_blu",
	"class_sel_sm_sniper_blu",
	"class_sel_sm_soldier_blu",
	"class_sel_sm_demo_blu",
	"class_sel_sm_medic_blu",
	"class_sel_sm_heavy_blu",
	"class_sel_sm_pyro_blu",
	"class_sel_sm_spy_blu",
	"class_sel_sm_engineer_blu",
	"",
};

const char *g_sImagesRed[] = {
	"",
	"class_sel_sm_scout_red",
	"class_sel_sm_sniper_red",
	"class_sel_sm_soldier_red",
	"class_sel_sm_demo_red",
	"class_sel_sm_medic_red",
	"class_sel_sm_heavy_red",
	"class_sel_sm_pyro_red",
	"class_sel_sm_spy_red",
	"class_sel_sm_engineer_red",
	"",
};


DECLARE_HUDELEMENT( CHudArenaClassLayout );

bool ArenaClassLayoutKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	CHudArenaClassLayout *pArenaClassLayoutPanel = ( CHudArenaClassLayout * )GET_HUDELEMENT( CHudArenaClassLayout );

	if ( pArenaClassLayoutPanel && pArenaClassLayoutPanel->ShouldDraw() )
	{
		return pArenaClassLayoutPanel->HandleKeyCodePressed( keynum );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudArenaClassLayout::CHudArenaClassLayout( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudArenaClassLayout" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	vgui::SETUP_PANEL( this );

	SetKeyBoardInputEnabled( true );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );

	m_pBackground = new CTFImagePanel( this, "background" );
	m_pLocalPlayerBG = new CTFImagePanel( this, "localPlayerBG" );
	m_pTitle = new CExLabel( this, "title", "" );
	m_pChangeLabel = new CExLabel( this, "changeLabel", "" );
	m_pChangeLabelShadow = new CExLabel( this, "changeLabelShadow", "" );

	char tempName[MAX_PATH];
	for ( int i = 0 ; i < MAX_CLASS_IMAGES ; ++i )
	{
		Q_snprintf( tempName, sizeof( tempName ), "classImage%d", i );
		m_ClassImages[i] = new CTFImagePanel( this, tempName );
	}

	SetVisible( false );

	RegisterForRenderGroup( "mid" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaClassLayout::Init( void )
{
	CHudElement::Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaClassLayout::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudArenaClassLayout.res" );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaClassLayout::PerformLayout( void )
{
	if ( !g_TF_PR )
		return;

	int i = 0;
	CUtlVector<int> teamPlayers;
	int nLocalPlayerTeam = GetLocalPlayerTeam();
	int iImageIndex = 0;
	CTFImagePanel *pImage = NULL;
	int nImageXPos = 0, nImageYPos = 0;

	if ( GetLocalPlayerTeam() <= LAST_SHARED_TEAM )
		return;

	int nClass = g_TF_PR->GetPlayerClass( GetLocalPlayerIndex() );
	if ( nClass == TF_CLASS_UNDEFINED )
		return;

	// count the number of players on our team that have chosen a playerclass
	for ( i = 1 ; i <= MAX_PLAYERS ; i++ )
	{
		if ( g_TF_PR->GetTeam( i ) == GetLocalPlayerTeam() )
		{
			if ( g_TF_PR->GetPlayerClass( i ) > TF_CLASS_UNDEFINED ) 
			{
				teamPlayers.AddToTail( i );
			}
		}

		if ( teamPlayers.Count() >= MAX_CLASS_IMAGES )
		{
			break;
		}
	}

	if ( teamPlayers.Count() > 0 )
	{
		int nBackgroundXPos, nBackgroundYPos, nBackgroundWide, nBackgroundTall;
		int nImageWide = m_ClassImages[iImageIndex]->GetWide();
		int nTotalWidth = ( teamPlayers.Count() * nImageWide ) + ( 2 * XRES( 10 ) ); // the XRES(10) is for the background to be scaled to cover the images on both ends
		int nXPos = ( GetWide() - nTotalWidth ) * 0.5;

		m_pBackground->GetBounds( nBackgroundXPos, nBackgroundYPos, nBackgroundWide, nBackgroundTall );
		m_pBackground->SetBounds( nXPos, nBackgroundYPos, nTotalWidth, nBackgroundTall );

		// this is where our first image will start
		nXPos += XRES( 10 );

		// the first image on the left is always the local player (and we'll have a special background behind it)
		pImage = m_ClassImages[iImageIndex];
		iImageIndex++;

		if ( pImage )
		{
			pImage->SetVisible( true );
			pImage->GetPos( nImageXPos, nImageYPos ); // only really care about the YPos here
			pImage->SetPos( nXPos, nImageYPos );
			pImage->SetImage( nLocalPlayerTeam == TF_TEAM_BLUE ? g_sImagesBlue[nClass] : g_sImagesRed[nClass] );

			if ( teamPlayers.Count() > 1 )
			{
				// local player background
				int nBGXPos, nBGYPos;
				m_pLocalPlayerBG->SetVisible( true );
				m_pLocalPlayerBG->GetPos( nBGXPos, nBGYPos );
				m_pLocalPlayerBG->SetPos( nXPos, nBGYPos );
			}
			else
			{
				m_pLocalPlayerBG->SetVisible( false );
			}

			nXPos += nImageWide;
		}

		for ( i = 0 ; i < teamPlayers.Count() ; i++ )
		{
			int iPlayerIndex = teamPlayers[i];

			if ( iPlayerIndex == GetLocalPlayerIndex() )
				continue;

			if ( iImageIndex >= MAX_CLASS_IMAGES )
				continue;

			if ( g_TF_PR->GetTeam( iPlayerIndex ) == GetLocalPlayerTeam() )
			{
				nClass = g_TF_PR->GetPlayerClass( iPlayerIndex );

				if ( nClass == TF_CLASS_UNDEFINED )
					continue;

				pImage = m_ClassImages[iImageIndex];

				if ( pImage )
				{
					pImage->SetVisible( true );
					pImage->SetPos( nXPos, nImageYPos );
					pImage->SetImage( nLocalPlayerTeam == TF_TEAM_BLUE ? g_sImagesBlue[nClass] : g_sImagesRed[nClass] );
					nXPos += nImageWide;
				}

				iImageIndex++;
			}
		}
	}

	// turn off any unused images
	while ( iImageIndex < MAX_CLASS_IMAGES )
	{
		pImage = m_ClassImages[iImageIndex];
		if ( pImage )
		{
			pImage->SetVisible( false );
		}

		iImageIndex++;
	}

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer && m_pChangeLabel && m_pChangeLabelShadow )
	{
		bool bShow = true;

		if ( ( pLocalPlayer->m_Shared.GetArenaNumChanges() >= tf_arena_change_limit.GetInt() ) ||
			 ( tf_arena_force_class.GetBool() == false ) )
		{
			bShow = false;
		}

		if ( m_pChangeLabel->IsVisible() != bShow )
		{
			m_pChangeLabel->SetVisible( bShow );
			m_pChangeLabelShadow->SetVisible( bShow );
		}
	}

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudArenaClassLayout::ShouldDraw( void )
{
	if ( TFGameRules() == NULL )
		return false;

	if ( CHudElement::ShouldDraw() == false )
		return false;

	if ( TFGameRules()->State_Get() != GR_STATE_PREROUND )
		return false;

	if ( TFGameRules()->IsInArenaMode() == false )
		return false;

	if ( GetLocalPlayerTeam() > LAST_SHARED_TEAM )
	{
		if ( ( tf_arena_force_class.GetBool() == true ) || ( GetGlobalTeam( GetLocalPlayerTeam() )->Get_Number_Players() > 1 ) )
		{
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pLocalPlayer && pLocalPlayer->IsAlive() )
			{
				C_TFPlayerClass *pClass = pLocalPlayer->GetPlayerClass();
				if ( pClass && pClass->GetClassIndex() != TF_CLASS_UNDEFINED )
				{
					return true;
				}					
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaClassLayout::SetVisible( bool state )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "show_class_layout" );
	if ( event )
	{
		event->SetBool( "show", state );
		gameeventmanager->FireEventClientSide( event );
	}

	BaseClass::SetVisible( state );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudArenaClassLayout::OnTick( void )
{
	bool bVisible = ShouldDraw();

	if ( bVisible != IsVisible() )
	{
		SetVisible( bVisible );
	}

	if ( !bVisible )
		return;
	
	InvalidateLayout( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudArenaClassLayout::HandleKeyCodePressed( vgui::KeyCode code )
{
	if ( code == KEY_F4 )
	{
		if ( ShouldDraw() && ( tf_arena_force_class.GetBool() == true ) )
		{
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pLocalPlayer && ( pLocalPlayer->m_Shared.GetArenaNumChanges() < tf_arena_change_limit.GetInt() ) )
			{
				engine->ClientCmd( "arena_changeclass" );
			}
		}
	}

	return false;
}

