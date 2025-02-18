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
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include "tf_spectatorgui.h"
#include "hud_macros.h"
#include "c_playerresource.h"
#include "view.h"
#include "player_vs_environment/c_tf_upgrades.h"
#include "tf_hud_inspectpanel.h"
#include "clientmode_tf.h"
#include "vguicenterprint.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


DECLARE_HUDELEMENT( CHudInspectPanel );

static float s_flLastInspectDownTime = 0.f;

void InspectDown()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer == NULL )
	{
		return;
	}

	s_flLastInspectDownTime = gpGlobals->curtime;

	KeyValues *kv = new KeyValues( "+inspect_server" );
	engine->ServerCmdKeyValues( kv );

	pLocalPlayer->SetInspectTime( gpGlobals->curtime );
}
static ConCommand s_inspect_down_cmd( "+inspect", InspectDown, "", FCVAR_SERVER_CAN_EXECUTE );

void InspectUp()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer == NULL )
	{
		return;
	}

	// quick tap on the inspect button, try to inspect players
	if ( gpGlobals->curtime - s_flLastInspectDownTime <= 0.2f )
	{
		CHudElement *pElement = gHUD.FindElement( "CHudInspectPanel" );
		if ( pElement )
		{
			((CHudInspectPanel *)pElement)->UserCmd_InspectTarget();
		}
	}

	KeyValues *kv = new KeyValues( "-inspect_server" );
	engine->ServerCmdKeyValues( kv );

	pLocalPlayer->SetInspectTime( 0.f );
}
static ConCommand s_inspect_up_cmd( "-inspect", InspectUp, "", FCVAR_SERVER_CAN_EXECUTE );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudInspectPanel::CHudInspectPanel( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudInspectPanel" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_pItemPanel = new CItemModelPanel( this, "itempanel" ) ;
	m_iTargetItemIterator = 0;

	RegisterForRenderGroup( "mid" );
	RegisterForRenderGroup( "commentary" );
	RegisterForRenderGroup( "arena_target_id" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudInspectPanel::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudInspectPanel.res" );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudInspectPanel::LockInspectRenderGroup( bool bLock )
{
	int iIndex = gHUD.LookupRenderGroupIndexByName( "inspect_panel" );
	if ( bLock )
	{
		gHUD.LockRenderGroup( iIndex );
	}
	else
	{
		gHUD.UnlockRenderGroup( iIndex );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudInspectPanel::ShouldDraw( void )
{
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !CHudElement::ShouldDraw() || !pLocalTFPlayer || !pLocalTFPlayer->IsAlive() )
	{
		m_hTarget = NULL;
		SetPanelVisible( false );
		return false;
	}

	return ( m_hTarget != NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudInspectPanel::UserCmd_InspectTarget( void )
{
	// If we're in observer mode, we cycle items on the current observer target
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return;

	C_TFPlayer *pTargetPlayer = GetInspectTarget( pLocalTFPlayer );

	bool bVisible = false;
	if ( pLocalTFPlayer->IsObserver() )
	{
		CTFSpectatorGUI *pPanel = (CTFSpectatorGUI*)gViewPortInterface->FindPanelByName( PANEL_SPECGUI );
		if ( pPanel )
		{
			pPanel->ForceItemPanelCycle();
		}
	}
	// In MvM, display player upgrades
	else if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		CHudUpgradePanel *pUpgradePanel = GET_HUDELEMENT( CHudUpgradePanel );
		if ( pUpgradePanel )
		{
			// Close if open
			if ( pUpgradePanel->IsVisible() )
			{
				pUpgradePanel->OnCommand( "cancel" );
			}
			// Inspect a player
			else if ( pTargetPlayer && ( pTargetPlayer->GetTeamNumber() != TF_TEAM_PVE_INVADERS ) )
			{
				if ( !GetClientModeTFNormal()->BIsFriendOrPartyMember( pTargetPlayer ) )
				{
					internalCenterPrint->Print( "#TF_Invalid_Inspect_Target" );
					return;
				}
				
				pUpgradePanel->InspectUpgradesForPlayer( pTargetPlayer );
			}
			// Inspect self
			else if ( !pTargetPlayer && pLocalTFPlayer  )
			{
				pUpgradePanel->InspectUpgradesForPlayer( pLocalTFPlayer );
			}
		}
	}
	else
	{
		if ( pTargetPlayer && !pTargetPlayer->IsEnemyPlayer() )
		{
			m_hTarget = pTargetPlayer;

			CEconItemView *pItem = m_hTarget->GetInspectItem( &m_iTargetItemIterator );
			if ( pItem && pItem->IsValid() )
			{
				m_pItemPanel->SetDialogVariable( "killername", g_PR->GetPlayerName( m_hTarget->entindex() ) );
				m_pItemPanel->SetItem( pItem );

				// force update description to get the correct panel size
				m_pItemPanel->UpdateDescription();
				bVisible = true;
				int x,y;
				GetPos( x, y );
				SetPos( x, ScreenHeight() - YRES( 12 ) - m_pItemPanel->GetTall() );
			}
		}
		else
		{
			m_iTargetItemIterator = 0;
		}
	}

	SetPanelVisible( bVisible );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFPlayer *CHudInspectPanel::GetInspectTarget( C_TFPlayer *pLocalTFPlayer )
{
	if ( !pLocalTFPlayer )
		return NULL;

	C_TFPlayer *pTargetPlayer = NULL;

	trace_t tr;
	Vector vecStart, vecEnd;
	VectorMA( MainViewOrigin(), MAX_TRACE_LENGTH, MainViewForward(), vecEnd );
	VectorMA( MainViewOrigin(), 10,   MainViewForward(), vecStart );
	UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, pLocalTFPlayer, COLLISION_GROUP_NONE, &tr );

	if ( !tr.startsolid && tr.DidHitNonWorldEntity() )
	{
		C_BaseEntity *pEntity = tr.m_pEnt;
		if ( pEntity && ( pEntity != pLocalTFPlayer ) && pEntity->IsPlayer() )
		{
			// Get the player under our crosshair
			pTargetPlayer = ToTFPlayer( pEntity );

			// Fix up if it's a spy disguised as my team
			if ( pLocalTFPlayer->m_Shared.IsSpyDisguisedAsMyTeam( pTargetPlayer ) )
			{
				// Get the player that the spy is disguised as
				C_TFPlayer *pDisguiseTarget = pTargetPlayer->m_Shared.GetDisguiseTarget();
				if ( pDisguiseTarget && pDisguiseTarget->GetPlayerClass()->GetClassIndex() == pTargetPlayer->m_Shared.GetDisguiseClass() )
				{
					// The spy is disguised as the same class as the target, so inspect the disguise target instead
					pTargetPlayer = pDisguiseTarget;
				}
			}
		}
	}

	return pTargetPlayer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudInspectPanel::SetPanelVisible( bool bVisible )
{
	if ( m_pItemPanel->IsVisible() != bVisible )
	{
		LockInspectRenderGroup( bVisible );
		m_pItemPanel->SetVisible( bVisible );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CHudInspectPanel::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( IsVisible() && pszCurrentBinding && pszCurrentBinding[0] )
	{
		if ( FStrEq( pszCurrentBinding, "+attack" ) )
		{
			CBasePlayer *pLocalPlayer = CBasePlayer::GetLocalPlayer();
			if ( pLocalPlayer && ( pLocalPlayer->GetTeamNumber() >= FIRST_GAME_TEAM ) && pLocalPlayer->IsAlive() )
			{
				SetPanelVisible( false );
			}
		}
	}

	return 1; // intentionally not handling the key
}
