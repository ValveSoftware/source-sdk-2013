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
#include "tf_gamerules.h"
#include "tf_logic_halloween_2014.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/Label.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudDemomanPipes : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudDemomanPipes, EditablePanel );

public:
	CHudDemomanPipes( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );

private:
	vgui::EditablePanel *m_pPipesPresent;
	vgui::EditablePanel *m_pNoPipesPresent;

	vgui::Label *m_pChargeLabel;
	vgui::ContinuousProgressBar *m_pChargeMeter;

	bool m_bChargeMode;
	float m_flOldProgress;
	int m_iLastPipes;
};

DECLARE_HUDELEMENT( CHudDemomanPipes );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudDemomanPipes::CHudDemomanPipes( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudDemomanPipes" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pPipesPresent = new EditablePanel( this, "PipesPresentPanel" );
	m_pNoPipesPresent = new EditablePanel( this, "NoPipesPresentPanel" );

	SetHiddenBits( HIDEHUD_MISCSTATUS | HIDEHUD_PIPES_AND_CHARGE );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );

	m_bChargeMode = false;
	m_flOldProgress = 1.f;
	m_iLastPipes = -1;

	if ( !m_pChargeMeter )
	{
		m_pChargeMeter = new ContinuousProgressBar( this, "ChargeMeter" );
	}

	if ( !m_pChargeLabel )
	{
		m_pChargeLabel = new Label( this, "ChargeLabel", "" );
	}

	RegisterForRenderGroup( "inspect_panel" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDemomanPipes::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudDemomanPipes.res" );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudDemomanPipes::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer || !pPlayer->IsPlayerClass( TF_CLASS_DEMOMAN ) )
		return false;

	if ( !pPlayer->IsAlive() )
		return false;

	if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
		return false;

	if ( CTFMinigameLogic::GetMinigameLogic() && CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame() )
		return false;

	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDemomanPipes::OnTick( void )
{
	if ( !IsVisible() )
		return;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;

	int iPipes = pPlayer->GetNumActivePipebombs();
	if ( iPipes != m_iLastPipes )
	{
		// SetDialogVariable is expensive as it does lots of localization work, so only call it if we need to
		m_pPipesPresent->SetDialogVariable( "activepipes", iPipes );
		m_pNoPipesPresent->SetDialogVariable( "activepipes", iPipes );
		m_pPipesPresent->SetVisible( iPipes > 0 );
		m_pNoPipesPresent->SetVisible( iPipes <= 0 );
		m_iLastPipes = iPipes;
	}
	m_pChargeMeter->SetVisible( false );
	m_pChargeLabel->SetVisible( false );

	if ( !m_bChargeMode )
	{
		if ( pPlayer->m_Shared.IsShieldEquipped() )
		{
			m_bChargeMode = true;
		}
	}
	else
	{
		if ( !pPlayer->m_Shared.IsShieldEquipped() )
		{
			m_bChargeMode = false;
		}
		else
		{
			m_pChargeMeter->SetVisible( true );
			m_pChargeLabel->SetVisible( true );
			m_pPipesPresent->SetVisible( false );
			m_pNoPipesPresent->SetVisible( false );

			float flProgress = pPlayer->m_Shared.GetDemomanChargeMeter() / 100.f;
			m_pChargeMeter->SetProgress( flProgress );
			if ( pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
			{
				if ( flProgress <= 0.33f )
				{
					m_pChargeMeter->SetFgColor( Color( 255, 0, 0, 255 ) );
				}
				else if ( flProgress <= 0.75f )
				{
					m_pChargeMeter->SetFgColor( Color( 255, 178, 0, 255 ) );
				}
				else
				{
					m_pChargeMeter->SetFgColor( Color( 153, 255, 153, 255 ) );
				}
			}
			else
			{
				m_pChargeMeter->SetFgColor( Color( 255, 255, 255, 255 ) );

				// Play a sound if we are newly ready.
				if ( C_TFPlayer::GetLocalTFPlayer() && flProgress >= 1.f && m_flOldProgress < 1.f )
				{
					m_flOldProgress = flProgress;
					if ( C_TFPlayer::GetLocalTFPlayer()->IsAlive() )
					{
						C_TFPlayer::GetLocalTFPlayer()->EmitSound( "TFPlayer.ReCharged" );
					}
				}
				else
				{
					m_flOldProgress = flProgress;
				}
			}
		}
	}
}