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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudTeamGoal : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudTeamGoal, EditablePanel );

public:
	CHudTeamGoal( const char *pElementName );

	virtual void	LevelInit( void );
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );

	void			SetupGoalPanel( const char *pszGoal );

private:
	Label			*m_pSwitchLabel;
	Label			*m_pGoalLabel;
	CTFImagePanel	*m_pGoalImage;
	float			m_flHideAt;
	int				m_iGoalLabelOrgY;
};

DECLARE_HUDELEMENT( CHudTeamGoal );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTeamGoal::CHudTeamGoal( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudTeamGoal" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_flHideAt = 0;
	m_iGoalLabelOrgY = 0;

	RegisterForRenderGroup( "commentary" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamGoal::LevelInit( void )
{
	m_flHideAt = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamGoal::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudTeamGoal.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pSwitchLabel = dynamic_cast<Label *>( FindChildByName("SwitchLabel") );
	m_pGoalLabel = dynamic_cast<Label *>( FindChildByName("GoalLabel") );
	m_pGoalImage = dynamic_cast<CTFImagePanel *>( FindChildByName("GoalImage") );

	if ( m_pGoalLabel )
	{
		int iIgnored;
		m_pGoalLabel->GetPos( iIgnored, m_iGoalLabelOrgY );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudTeamGoal::ShouldDraw( void )
{
	if ( !TFGameRules() )
		return false;

	bool bCouldSee = TFGameRules()->ShouldShowTeamGoal();

	if ( TFGameRules()->IsInTournamentMode() )
	{
		bCouldSee = false;
	}

	if ( m_flHideAt && m_flHideAt < gpGlobals->curtime )
	{
		if ( !bCouldSee )
		{
			m_flHideAt = 0;
		}

		return false;
	}

	if ( bCouldSee )
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() >= FIRST_GAME_TEAM )
		{
			const char *pszGoal = TFGameRules()->GetTeamGoalString( pPlayer->GetTeamNumber() );
			if ( pszGoal && pszGoal[0] && CHudElement::ShouldDraw() )
			{
				if ( !IsVisible() )
				{
					// Once we've played a map 15 times, don't show team goals anymore.
					if ( UTIL_GetMapKeyCount( "viewed" ) > 15 )
					{
						m_flHideAt = -1;	// To prevent it rechecking until next level load
						return false;
					}

					SetupGoalPanel( pszGoal );

					// Show for 15 seconds
					m_flHideAt = gpGlobals->curtime + 15.0;
				}

				// Don't appear if the team switch alert is there
				CHudElement *pHudSwitch = gHUD.FindElement( "CHudTeamSwitch" );
				if ( pHudSwitch && pHudSwitch->ShouldDraw() )
					return false;

				return true;
			}
		}
	}

	return false;
}

const char *pszTeamRoleIcons[NUM_TEAM_ROLES] =
{
	"../hud/hud_icon_capture",		// TEAM_ROLE_NONE = 0,
	"../hud/hud_icon_defend",		// TEAM_ROLE_DEFENDERS,
	"../hud/hud_icon_attack",		// TEAM_ROLE_ATTACKERS,
};

const char *pszTeamRoleSwitch[NUM_TEAM_ROLES] =
{
	" ",							// TEAM_ROLE_NONE = 0,
	"#TF_teamswitch_defenders",		// TEAM_ROLE_DEFENDERS,
	"#TF_teamswitch_attackers",		// TEAM_ROLE_ATTACKERS,
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamGoal::SetupGoalPanel( const char *pszGoal )
{
	if ( m_pGoalLabel )
	{
		wchar_t *pszLocalizedGoal = g_pVGuiLocalize->Find( pszGoal );
		if ( pszLocalizedGoal )
		{
			m_pGoalLabel->SetText( pszLocalizedGoal );
		}
		else
		{
			m_pGoalLabel->SetText( pszGoal );
		}
	}

	if ( m_pSwitchLabel )
	{
		m_pSwitchLabel->SetVisible( false );
	}

	C_TFTeam *pLocalTeam = GetGlobalTFTeam( GetLocalPlayerTeam() );
	if ( pLocalTeam )
	{

//=============================================================================
// HPE_BEGIN:
// [msmith]	If we're in training, we want to use a different icon here.
//=============================================================================
		if ( TFGameRules()->IsInTraining() )
		{
			m_pGoalImage->SetImage( "../hud/hud_icon_training" );
		}
//=============================================================================
// HPE_END
//=============================================================================
		else
		{
			int iRole = pLocalTeam->GetRole();
			if ( iRole >= 0 && iRole < NUM_TEAM_ROLES )
			{
				m_pGoalImage->SetImage( pszTeamRoleIcons[iRole] );

				if ( m_pSwitchLabel )
				{
					if ( TFGameRules() && TFGameRules()->SwitchedTeamsThisRound() )
					{
						m_pSwitchLabel->SetText( g_pVGuiLocalize->Find( pszTeamRoleSwitch[iRole] ) );
						m_pSwitchLabel->SetVisible( true );
					}
				}
			}
		}
	}

	if ( m_pGoalLabel && m_pSwitchLabel )
	{
		// If the switch label is invisible, move the goal label up to where it is.
		int iX, iY, iSwitchY, iIgnored;
		m_pGoalLabel->GetPos( iX, iY );
		m_pSwitchLabel->GetPos( iIgnored, iSwitchY );
		if ( m_pSwitchLabel->IsVisible() )
		{
			m_pGoalLabel->SetPos( iX, m_iGoalLabelOrgY );
		}
		else
		{
			m_pGoalLabel->SetPos( iX, iSwitchY );
		}
	}
}
