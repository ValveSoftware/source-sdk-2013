//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//
#include "cbase.h"
#include "items.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "tf_player.h"
#include "tf_team.h"
#include "engine/IEngineSound.h"
#include "filesystem.h"
#include "tf_powerup.h"
#include "bot/tf_bot.h"

//=============================================================================
float PackRatios[POWERUP_SIZES] =
{
	0.2,	// SMALL
	0.5,	// MEDIUM
	1.0,	// FULL
};

//=============================================================================
//
// CTF Powerup tables.
//

BEGIN_DATADESC( CTFPowerup )

	// Keyfields.
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "powerup_model" ),
	DEFINE_KEYFIELD( m_bAutoMaterialize, FIELD_BOOLEAN, "AutoMaterialize" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

	// Outputs.

END_DATADESC();

//=============================================================================
//
// CTF Powerup functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPowerup::CTFPowerup()
{
	m_bDisabled = false;
	m_bRespawning = false;
	m_bAutoMaterialize = true;

	m_iszModel = NULL_STRING;

	m_flThrowerTouchTime = -1;

	UseClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPowerup::Spawn( void )
{
	Precache();
	SetModel( GetPowerupModel() );

	BaseClass::Spawn();

	BaseClass::SetOriginalSpawnOrigin( GetAbsOrigin() );
	BaseClass::SetOriginalSpawnAngles( GetAbsAngles() );

	VPhysicsDestroyObject();
	SetMoveType( MOVETYPE_NONE );
	SetSolidFlags( FSOLID_NOT_SOLID | FSOLID_TRIGGER );

	if ( m_bDisabled )
	{
		SetDisabled( true );
	}

	m_bRespawning = false;

	ResetSequence( LookupSequence("idle") );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPowerup::Precache()
{
	PrecacheModel( GetPowerupModel() );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity* CTFPowerup::Respawn( void )
{
	m_bRespawning = true;
	CBaseEntity *pReturn = BaseClass::Respawn();

	// Override the respawn time
	SetNextThink( gpGlobals->curtime + GetRespawnDelay() );

	return pReturn;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPowerup::Materialize( void )
{
	if ( !m_bAutoMaterialize )
	{
		return;
	}

	Materialize_Internal();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPowerup::Materialize_Internal( void )
{
	if ( !m_bDisabled && IsEffectActive( EF_NODRAW ) )
	{
		// changing from invisible state to visible.
		EmitSound( "Item.Materialize" );
		RemoveEffects( EF_NODRAW );
	}

	m_bRespawning = false;
	SetTouch( &CItem::ItemTouch );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPowerup::ValidTouch( CBasePlayer *pPlayer )
{
	// Is the item enabled?
	if ( IsDisabled() )
	{
		return false;
	}

	// Only touch a live player.
	if ( !pPlayer || !pPlayer->IsPlayer() || !pPlayer->IsAlive() )
	{
		return false;
	}

	// Team number and does it match?
	int iTeam = GetTeamNumber();
	if ( iTeam && ( pPlayer->GetTeamNumber() != iTeam ) )
	{
		return false;
	}

	// enemies in mann vs machine can't pick up any powerups
	if ( TFGameRules()->IsMannVsMachineMode() && pPlayer->GetTeamNumber() == TF_TEAM_PVE_INVADERS )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPowerup::MyTouch( CBasePlayer *pPlayer )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerup::DropSingleInstance( Vector &vecLaunchVel, CBaseCombatCharacter *pThrower, float flThrowerTouchDelay, float flResetTime /*= 0.1f*/ )
{
//	SetSize( Vector(-8,-8,-8), Vector(8,8,8) );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetAbsVelocity( vecLaunchVel );	
	SetSolid( SOLID_BBOX );
	if ( flResetTime )
	{
		ActivateWhenAtRest( flResetTime );
	}

	m_bThrownSingleInstance = true;
	AddSpawnFlags( SF_NORESPAWN );

	SetOwnerEntity( pThrower );
	m_flThrowerTouchTime = gpGlobals->curtime + flThrowerTouchDelay;

	// Remove ourselves after some time
	SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + GetLifeTime(), "PowerupRemoveThink" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerup::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerup::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPowerup::IsDisabled( void )
{
	return m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerup::InputToggle( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		SetDisabled( false );
	}
	else
	{
		SetDisabled( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerup::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;

	if ( bDisabled )
	{
		AddEffects( EF_NODRAW );
	}
	else
	{
		// only turn it back on if we're not in the middle of respawning
		if ( !m_bRespawning )
		{
            RemoveEffects( EF_NODRAW );
		}
		else if ( !m_bAutoMaterialize )
		{
			// We wait for a set-enabled to re-materialize if we were 
			// set to not auto-materialize
			Materialize_Internal();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CTFPowerup::GetPowerupModel( void )
{
	if ( m_iszModel != NULL_STRING )
	{
		if ( g_pFullFileSystem->FileExists( STRING( m_iszModel ), "GAME" ) )
		{
			return ( STRING( m_iszModel ) );
		}
	}

	return GetDefaultPowerupModel();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPowerup::ItemCanBeTouchedByPlayer( CBasePlayer *pPlayer )
{
	if ( pPlayer == GetOwnerEntity() )
	{
		if ( ( m_flThrowerTouchTime > 0 ) && ( gpGlobals->curtime < m_flThrowerTouchTime ) )
		{
			return false;
		}
	}

	return BaseClass::ItemCanBeTouchedByPlayer( pPlayer );
}
