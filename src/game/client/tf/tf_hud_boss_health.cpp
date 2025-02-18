//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/ProgressBar.h>
#include "c_monster_resource.h"
#include "tf_hud_boss_health.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_HUDELEMENT( CHudBossHealthMeter );


ConVar cl_boss_show_stun( "cl_boss_show_stun", "0", FCVAR_DEVELOPMENTONLY );


//-----------------------------------------------------------------------------
CHudBossHealthMeter::CHudBossHealthMeter( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudBossHealth" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pStunMeter = NULL;

	m_pHealthBarPanel = new vgui::EditablePanel( this, "HealthBarPanel" );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_inactiveColor = Color( 0, 255, 0, 255 );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}


//-----------------------------------------------------------------------------
void CHudBossHealthMeter::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudBossHealth.res" );

	m_pStunMeter = dynamic_cast< ContinuousProgressBar * >( FindChildByName( "StunMeter" ) );

	//BarImage
	m_pBarImagePanel = dynamic_cast<ImagePanel*>( m_pHealthBarPanel->FindChildByName( "BarImage" ) );
	m_bossActiveBarColor = m_pBarImagePanel ? m_pBarImagePanel->GetDrawColor() : m_inactiveColor;
		

	// BorderImage
	m_pBorderImagePanel = dynamic_cast< ImagePanel* >( FindChildByName( "BorderImage" ) );
	m_bossActiveBorderColor = m_pBorderImagePanel ? m_pBorderImagePanel->GetDrawColor() : m_inactiveColor;

	BaseClass::ApplySchemeSettings( pScheme );
}


//-----------------------------------------------------------------------------
void CHudBossHealthMeter::Init( void )
{
}


//-----------------------------------------------------------------------------
bool CHudBossHealthMeter::ShouldDraw( void )
{
	CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	if ( !pPlayer || pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM )
	{
		return false;
	}

	if ( CHudElement::ShouldDraw() && g_pMonsterResource )
	{
		return g_pMonsterResource->GetBossHealthPercentage() > 0.0f ? true : false;
	}

	if ( pPlayer->GetTeamNumber() <= TEAM_SPECTATOR )
	{
		return false;
	}

	return false;
}

void CHudBossHealthMeter::OnTick( void )
{
	int nXPos, nYPos;
	GetPos( nXPos, nYPos );
	nYPos = m_nHealthDeadPosY;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer && !pPlayer->IsObserver() )
	{
		nYPos = m_nHealthAlivePosY;
	}

	SetPos( nXPos, nYPos );
}


//-----------------------------------------------------------------------------
// Update HUD due to data changes
void CHudBossHealthMeter::Update( void )
{
	if ( g_pMonsterResource )
	{
		m_pHealthBarPanel->SetWide( m_nHealthBarWide * g_pMonsterResource->GetBossHealthPercentage() );

		if ( m_pStunMeter )
		{
			float stun = g_pMonsterResource->GetBossStunPercentage();
			if ( stun > 0.0f && cl_boss_show_stun.GetBool() )
			{
				if ( !m_pStunMeter->IsVisible() )
				{
					m_pStunMeter->SetVisible( true );
				}

				m_pStunMeter->SetProgress( g_pMonsterResource->GetBossStunPercentage() );
			}
			else if ( m_pStunMeter->IsVisible() )
			{
				m_pStunMeter->SetVisible( false );
			}
		}

		int iState = g_pMonsterResource->GetBossState();
	
		if ( m_pBarImagePanel )
		{
			Color barColor = ( iState == 0 ) ? m_bossActiveBarColor : m_inactiveColor;
			m_pBarImagePanel->SetDrawColor( barColor );
		}

		if ( m_pBorderImagePanel )
		{
			Color borderColor = ( iState == 0 ) ? m_bossActiveBorderColor : m_inactiveColor;
			m_pBorderImagePanel->SetDrawColor( borderColor );
		}
	}
}