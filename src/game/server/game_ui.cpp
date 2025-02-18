//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entities that capture the player's UI and move it into game design
//			as outputs.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "entitylist.h"
#include "util.h"
#include "physics.h"
#include "entityoutput.h"
#include "player.h"
#include "in_buttons.h"
#include "basecombatweapon.h"
#include "baseviewmodel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//----------------------------------------------------------------
// Spawn flags
//----------------------------------------------------------------
#define  SF_GAMEUI_FREEZE_PLAYER		32	
#define  SF_GAMEUI_HIDE_WEAPON			64	
#define  SF_GAMEUI_USE_DEACTIVATES		128
#define  SF_GAMEUI_JUMP_DEACTIVATES		256


class CGameUI : public CBaseEntity
{
public:
	DECLARE_CLASS( CGameUI, CBaseEntity );

	DECLARE_DATADESC();

	// Input handlers
	void InputDeactivate( inputdata_t &inputdata );
	void InputActivate( inputdata_t &inputdata );

	void Think( void );
	void Deactivate( CBaseEntity *pActivator );

	float				m_flFieldOfView;
	CHandle<CBaseCombatWeapon>	m_hSaveWeapon;

	COutputEvent		m_playerOn;
	COutputEvent		m_playerOff;

	COutputEvent		m_pressedMoveLeft;
	COutputEvent		m_pressedMoveRight;
	COutputEvent		m_pressedForward;
	COutputEvent		m_pressedBack;
	COutputEvent		m_pressedAttack;
	COutputEvent		m_pressedAttack2;
	
	COutputEvent		m_unpressedMoveLeft;
	COutputEvent		m_unpressedMoveRight;
	COutputEvent		m_unpressedForward;
	COutputEvent		m_unpressedBack;
	COutputEvent		m_unpressedAttack;
	COutputEvent		m_unpressedAttack2;

	COutputFloat		m_xaxis;
	COutputFloat		m_yaxis;
	COutputFloat		m_attackaxis;
	COutputFloat		m_attack2axis;

	bool				m_bForceUpdate;
	int					m_nLastButtonState;

	CHandle<CBasePlayer>	m_player;
};


BEGIN_DATADESC( CGameUI )

	DEFINE_KEYFIELD( m_flFieldOfView, FIELD_FLOAT, "FieldOfView" ),
	DEFINE_FIELD( m_hSaveWeapon, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bForceUpdate, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_player, FIELD_EHANDLE ),
	DEFINE_FIELD( m_nLastButtonState, FIELD_INTEGER ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Deactivate", InputDeactivate ),
	DEFINE_INPUTFUNC( FIELD_STRING, "Activate", InputActivate ),

	DEFINE_OUTPUT( m_playerOn, "PlayerOn" ),
	DEFINE_OUTPUT( m_playerOff, "PlayerOff" ),

	DEFINE_OUTPUT( m_pressedMoveLeft, "PressedMoveLeft" ),
	DEFINE_OUTPUT( m_pressedMoveRight, "PressedMoveRight" ),
	DEFINE_OUTPUT( m_pressedForward, "PressedForward" ),
	DEFINE_OUTPUT( m_pressedBack, "PressedBack" ),
	DEFINE_OUTPUT( m_pressedAttack, "PressedAttack" ),
	DEFINE_OUTPUT( m_pressedAttack2, "PressedAttack2" ),

	DEFINE_OUTPUT( m_unpressedMoveLeft, "UnpressedMoveLeft" ),
	DEFINE_OUTPUT( m_unpressedMoveRight, "UnpressedMoveRight" ),
	DEFINE_OUTPUT( m_unpressedForward, "UnpressedForward" ),
	DEFINE_OUTPUT( m_unpressedBack, "UnpressedBack" ),
	DEFINE_OUTPUT( m_unpressedAttack, "UnpressedAttack" ),
	DEFINE_OUTPUT( m_unpressedAttack2, "UnpressedAttack2" ),

	DEFINE_OUTPUT( m_xaxis, "XAxis" ),
	DEFINE_OUTPUT( m_yaxis, "YAxis" ),
	DEFINE_OUTPUT( m_attackaxis, "AttackAxis" ),
	DEFINE_OUTPUT( m_attack2axis, "Attack2Axis" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( game_ui, CGameUI );
	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameUI::InputDeactivate( inputdata_t &inputdata )
{
	Deactivate( inputdata.pActivator );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameUI::Deactivate( CBaseEntity *pActivator )
{
	CBasePlayer *pPlayer = m_player;

	AssertMsg(pPlayer, "CGameUI deactivated without a player!");

	if (pPlayer)
	{
		// Re-enable player motion
		if ( FBitSet( m_spawnflags, SF_GAMEUI_FREEZE_PLAYER ) )
		{
			m_player->RemoveFlag( FL_ATCONTROLS );
		}

		// Restore weapons
		if ( FBitSet( m_spawnflags, SF_GAMEUI_HIDE_WEAPON ) )
		{
			// Turn the hud back on
			pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_WEAPONSELECTION;

			if ( m_hSaveWeapon.Get() )
			{
				m_player->Weapon_Switch( m_hSaveWeapon.Get() );
				m_hSaveWeapon = NULL;
			}

			if ( pPlayer->GetActiveWeapon() )
			{
				pPlayer->GetActiveWeapon()->Deploy();
			}
		}

		// Announce that the player is no longer controlling through us
		m_playerOff.FireOutput( pPlayer, this, 0 );

		// Clear out the axis controls
		m_xaxis.Set( 0, pPlayer, this );
		m_yaxis.Set( 0, pPlayer, this );
		m_attackaxis.Set( 0, pPlayer, this );
		m_attack2axis.Set( 0, pPlayer, this );
		m_nLastButtonState = 0;
		m_player = NULL;
	}
	else
	{
		Warning("%s Deactivate(): I have no player when called by %s!\n", GetEntityName().ToCStr(), pActivator ? pActivator->GetEntityName().ToCStr() : NULL);
	}
	
	// Stop thinking
	SetNextThink( TICK_NEVER_THINK );
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CGameUI::InputActivate( inputdata_t &inputdata )
{
	CBasePlayer *pPlayer;

	// Determine if we're specifying this as an override parameter
	if ( inputdata.value.StringID() != NULL_STRING )
	{
		CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, inputdata.value.String(), this, inputdata.pActivator, inputdata.pCaller );
		if ( pEntity == NULL || pEntity->IsPlayer() == false )
		{
			Warning( "%s InputActivate: entity %s not found or is not a player!\n", GetEntityName().ToCStr(), inputdata.value.String() );
			return;
		}

		pPlayer = ToBasePlayer( pEntity );
	}
	else
	{
		// Otherwise try to use the activator
		if ( inputdata.pActivator == NULL || inputdata.pActivator->IsPlayer() == false )
		{
			Warning( "%s InputActivate: invalid or missing !activator!\n", GetEntityName().ToCStr() );
			return;
		}

		pPlayer = ToBasePlayer( inputdata.pActivator );
	}

	// If another player is already using these controls3, ignore this activation
	if ( m_player.Get() != NULL && pPlayer != m_player.Get() )
	{
		// TODO: We could allow this by calling Deactivate() at this point and continuing on -- jdw
		return;
	}

	// Setup our internal data
	m_player = pPlayer;
	m_playerOn.FireOutput( pPlayer, this, 0 );

	// Turn the hud off
	SetNextThink( gpGlobals->curtime );

	// Disable player's motion
	if ( FBitSet( m_spawnflags, SF_GAMEUI_FREEZE_PLAYER ) )
	{
		m_player->AddFlag( FL_ATCONTROLS );
	}

	// Store off and hide the currently held weapon
	if ( FBitSet( m_spawnflags, SF_GAMEUI_HIDE_WEAPON ) )
	{
		m_player->m_Local.m_iHideHUD |= HIDEHUD_WEAPONSELECTION;

		if ( m_player->GetActiveWeapon() )
		{
			m_hSaveWeapon = m_player->GetActiveWeapon();

			m_player->GetActiveWeapon()->Holster();
			m_player->ClearActiveWeapon();
			m_player->HideViewModels();
		}
	}

	// We must update our state
	m_bForceUpdate = true;
}


//------------------------------------------------------------------------------
// Purpose: Samples the player's inputs and fires outputs based on what buttons
//			are currently held down.
//------------------------------------------------------------------------------
void CGameUI::Think( void )
{
	CBasePlayer *pPlayer = m_player;

	// If player is gone, stop thinking
	if (pPlayer == NULL)
	{
		SetNextThink( TICK_NEVER_THINK );
		return;
	}

	// If we're forcing an update, state with a clean button state
	if ( m_bForceUpdate )
	{
		m_nLastButtonState = pPlayer->m_nButtons;
	}

	// ------------------------------------------------
	// Check that toucher is facing the UI within
	// the field of view tolerance.  If not disconnect
	// ------------------------------------------------
	if (m_flFieldOfView > -1)
	{
		Vector vPlayerFacing;
		pPlayer->EyeVectors( &vPlayerFacing );
		Vector vPlayerToUI =  GetAbsOrigin() - pPlayer->WorldSpaceCenter();
		VectorNormalize(vPlayerToUI);

		float flDotPr = DotProduct(vPlayerFacing,vPlayerToUI);
		if (flDotPr < m_flFieldOfView)
		{
			Deactivate( pPlayer );
			return;
		}
	}

	pPlayer->AddFlag( FL_ONTRAIN );
	SetNextThink( gpGlobals->curtime );

	// Deactivate if they jump or press +use.
	// FIXME: prevent the use from going through in player.cpp
	if ((( pPlayer->m_afButtonPressed & IN_USE ) && ( m_spawnflags & SF_GAMEUI_USE_DEACTIVATES )) ||
		(( pPlayer->m_afButtonPressed & IN_JUMP ) && ( m_spawnflags & SF_GAMEUI_JUMP_DEACTIVATES )))
	{
		Deactivate( pPlayer );
		return;
	}

	// Determine what's different
	int nButtonsChanged = ( pPlayer->m_nButtons ^ m_nLastButtonState );

	//
	// Handle all our possible input triggers
	//

	if ( nButtonsChanged & IN_MOVERIGHT )
	{
		if ( m_nLastButtonState & IN_MOVERIGHT )
		{
			m_unpressedMoveRight.FireOutput( pPlayer, this, 0 );
		}
		else
		{
			m_pressedMoveRight.FireOutput( pPlayer, this, 0 );
		}
	}

	if ( nButtonsChanged & IN_MOVELEFT )
	{
		if ( m_nLastButtonState & IN_MOVELEFT )
		{
			m_unpressedMoveLeft.FireOutput( pPlayer, this, 0 );
		}
		else
		{
			m_pressedMoveLeft.FireOutput( pPlayer, this, 0 );
		}
	}

	if ( nButtonsChanged & IN_FORWARD )
	{
		if ( m_nLastButtonState & IN_FORWARD )
		{
			m_unpressedForward.FireOutput( pPlayer, this, 0 );
		}
		else
		{
			m_pressedForward.FireOutput( pPlayer, this, 0 );
		}
	}

	if ( nButtonsChanged & IN_BACK )
	{
		if ( m_nLastButtonState & IN_BACK )
		{
			m_unpressedBack.FireOutput( pPlayer, this, 0 );
		}
		else
		{
			m_pressedBack.FireOutput( pPlayer, this, 0 );
		}
	}

	if ( nButtonsChanged & IN_ATTACK )
	{
		if ( m_nLastButtonState & IN_ATTACK )
		{
			m_unpressedAttack.FireOutput( pPlayer, this, 0 );
		}
		else
		{
			m_pressedAttack.FireOutput( pPlayer, this, 0 );
		}
	}

	if ( nButtonsChanged & IN_ATTACK2 )
	{
		if ( m_nLastButtonState & IN_ATTACK2 )
		{
			m_unpressedAttack2.FireOutput( pPlayer, this, 0 );
		}
		else
		{
			m_pressedAttack2.FireOutput( pPlayer, this, 0 );
		}
	}

	// Setup for the next frame
	m_nLastButtonState = pPlayer->m_nButtons;

	float x = 0, y = 0, attack = 0, attack2 = 0;
	if ( pPlayer->m_nButtons & IN_MOVERIGHT )
	{
		x = 1;
	}
	else if ( pPlayer->m_nButtons & IN_MOVELEFT )
	{
		x = -1;
	}

	if ( pPlayer->m_nButtons & IN_FORWARD )
	{
		y = 1;
	}
	else if ( pPlayer->m_nButtons & IN_BACK )
	{
		y = -1;
	}

	if ( pPlayer->m_nButtons & IN_ATTACK )
	{
		attack = 1;
	}

	if ( pPlayer->m_nButtons & IN_ATTACK2 )
	{
		attack2 = 1;
	}

	//
	// Fire the analog outputs if they changed.
	//
	if ( m_bForceUpdate || ( m_xaxis.Get() != x ) )
	{
		m_xaxis.Set( x, pPlayer, this );
	}

	if ( m_bForceUpdate || ( m_yaxis.Get() != y ) )
	{
		m_yaxis.Set( y, pPlayer, this );
	}

	if ( m_bForceUpdate || ( m_attackaxis.Get() != attack ) )
	{
		m_attackaxis.Set( attack, pPlayer, this );
	}

	if ( m_bForceUpdate || ( m_attack2axis.Get() != attack2 ) )
	{
		m_attack2axis.Set( attack2, pPlayer, this );
	}

	m_bForceUpdate = false;
}
