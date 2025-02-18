//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>

#include "tf_imagepanel.h"
#include "c_tf_player.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( CTFImagePanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFImagePanel::CTFImagePanel( Panel *parent, const char *name ) : ScalableImagePanel( parent, name )
{
	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		m_szTeamBG[i][0] = '\0';
	}

	C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	m_iBGTeam = pPlayer ? pPlayer->GetTeamNumber() : TEAM_UNASSIGNED;

	ListenForGameEvent( "localplayer_changeteam" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImagePanel::ApplySettings( KeyValues *inResourceData )
{
	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		Q_strncpy( m_szTeamBG[i], inResourceData->GetString( VarArgs("teambg_%d", i), "" ), sizeof( m_szTeamBG[i] ) );

		if ( m_szTeamBG[i] && m_szTeamBG[i][0] )
		{
			PrecacheMaterial( VarArgs( "vgui/%s", m_szTeamBG[i] ) );
		}
	}

	BaseClass::ApplySettings( inResourceData );

	UpdateBGImage();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImagePanel::UpdateBGImage( void )
{
	if ( m_iBGTeam >= 0 && m_iBGTeam < TF_TEAM_COUNT )
	{
		if ( m_szTeamBG[m_iBGTeam] && m_szTeamBG[m_iBGTeam][0] )
		{
			SetImage( m_szTeamBG[m_iBGTeam] );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImagePanel::FireGameEvent( IGameEvent * event )
{
	if ( FStrEq( "localplayer_changeteam", event->GetName() ) )
	{
		C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		m_iBGTeam = pPlayer ? pPlayer->GetTeamNumber() : TEAM_UNASSIGNED;
		UpdateBGImage();
	}
}