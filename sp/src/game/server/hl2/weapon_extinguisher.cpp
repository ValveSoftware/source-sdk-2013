//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "gamerules.h"
#include "game.h"
#include "in_buttons.h"
#include "extinguisherjet.h"
#include "entitylist.h"
#include "fire.h"
#include "ar2_explosion.h"
#include "ndebugoverlay.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	fire_extinguisher_debug( "fire_extinguisher_debug", "0" );
ConVar	fire_extinguisher_radius( "fire_extinguisher_radius", "45" );
ConVar	fire_extinguisher_distance( "fire_extinguisher_distance", "200" );
ConVar	fire_extinguisher_strength( "fire_extinguisher_strength", "0.97" );	//TODO: Stub for real numbers
ConVar	fire_extinguisher_explode_radius( "fire_extinguisher_explode_radius", "256" );
ConVar	fire_extinguisher_explode_strength( "fire_extinguisher_explode_strength", "0.3" );	//TODO: Stub for real numbers

#define	EXTINGUISHER_AMMO_RATE	0.2

extern short	g_sModelIndexFireball;	// (in combatweapon.cpp) holds the index for the smoke cloud

class CWeaponExtinguisher: public CHLSelectFireMachineGun
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponExtinguisher, CHLSelectFireMachineGun );

	DECLARE_SERVERCLASS();

	CWeaponExtinguisher();

	void	Spawn( void );
	void	Precache( void );

	void	ItemPostFrame( void );
	void	Event_Killed( const CTakeDamageInfo &info );
	void	Equip( CBaseCombatCharacter *pOwner );

protected:
	
	void	StartJet( void );
	void	StopJet( void );

	CExtinguisherJet	*m_pJet;
};

IMPLEMENT_SERVERCLASS_ST(CWeaponExtinguisher, DT_WeaponExtinguisher)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_extinguisher, CWeaponExtinguisher );
PRECACHE_WEAPON_REGISTER( weapon_extinguisher );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CWeaponExtinguisher )

	DEFINE_FIELD( m_pJet,	FIELD_CLASSPTR ),

END_DATADESC()


CWeaponExtinguisher::CWeaponExtinguisher( void )
{
	m_pJet		= NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponExtinguisher::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "ExtinguisherCharger.Use" );

	UTIL_PrecacheOther( "env_extinguisherjet" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponExtinguisher::Spawn( void )
{
	BaseClass::Spawn();

	m_takedamage	= DAMAGE_YES;
	m_iHealth		= 25;//FIXME: Define
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void CWeaponExtinguisher::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );

	m_takedamage	= DAMAGE_NO;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInflictor - 
//			*pAttacker - 
//			flDamage - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
void CWeaponExtinguisher::Event_Killed( const CTakeDamageInfo &info )
{
	//TODO: Use a real effect
	if ( AR2Explosion *pExplosion = AR2Explosion::CreateAR2Explosion( GetAbsOrigin() ) )
	{
		pExplosion->SetLifetime( 10 );
	}		

	//TODO: Use a real effect
	CPASFilter filter( GetAbsOrigin() );

	te->Explosion( filter, 0.0,
		&GetAbsOrigin(), 
		g_sModelIndexFireball,
		2.0, 
		15,
		TE_EXPLFLAG_NONE,
		250,
		100 );

	//Put out fire in a radius
	FireSystem_ExtinguishInRadius( GetAbsOrigin(), fire_extinguisher_explode_radius.GetInt(), fire_extinguisher_explode_strength.GetFloat() );

	SetThink( SUB_Remove );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: Turn the jet effect and noise on
//-----------------------------------------------------------------------------
void CWeaponExtinguisher::StartJet( void )
{
	//See if the jet needs to be created
	if ( m_pJet == NULL )
	{
		m_pJet = (CExtinguisherJet *) CreateEntityByName( "env_extinguisherjet" );

		if ( m_pJet == NULL )
		{
			Msg( "Unable to create jet for weapon_extinguisher!\n" );
			return;
		}

		//Setup the jet
		m_pJet->m_bEmit	= false;
		UTIL_SetOrigin( m_pJet, GetAbsOrigin() );
		m_pJet->SetParent( this );

		m_pJet->m_bUseMuzzlePoint = true;
		m_pJet->m_bAutoExtinguish = false;
		m_pJet->m_nLength = fire_extinguisher_distance.GetInt();
	}

	//Turn the jet on
	if ( m_pJet != NULL )
	{
		m_pJet->TurnOn();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Turn the jet effect and noise off
//-----------------------------------------------------------------------------
void CWeaponExtinguisher::StopJet( void )
{
	//Turn the jet off
	if ( m_pJet != NULL )
	{
		m_pJet->TurnOff();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponExtinguisher::ItemPostFrame( void )
{	
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	//Only shoot if we have ammo
	if ( pOwner->GetAmmoCount(m_iSecondaryAmmoType) <= 0 )
	{
		StopJet();
		return;
	}
	
	//See if we should try and extinguish fires
	if ( pOwner->m_nButtons & IN_ATTACK )
	{
		//Drain ammo
		if ( m_flNextPrimaryAttack < gpGlobals->curtime  )
		{
			pOwner->RemoveAmmo( 1, m_iSecondaryAmmoType );
			m_flNextPrimaryAttack = gpGlobals->curtime + EXTINGUISHER_AMMO_RATE;
		}

		//If we're just run out...
		if ( pOwner->GetAmmoCount(m_iSecondaryAmmoType) <= 0 )
		{
			StopJet();
			return;
		}

		//Turn the jet on
		StartJet();

		Vector	vTestPos, vMuzzlePos;
		Vector	vForward, vRight, vUp;

		pOwner->EyeVectors( &vForward, &vRight, &vUp );
		
		vMuzzlePos	= pOwner->Weapon_ShootPosition( );
		
		//FIXME: Need to get the exact same muzzle point!

		//FIXME: This needs to be adjusted so the server collision matches the visuals on the client
		vMuzzlePos	+= vForward * 15.0f;
		vMuzzlePos	+= vRight * 6.0f;
		vMuzzlePos	+= vUp * -4.0f;

		QAngle aTmp;
		VectorAngles( vForward, aTmp );
		aTmp[PITCH] += 10;
		AngleVectors( aTmp, &vForward );

		vTestPos	= vMuzzlePos + ( vForward * fire_extinguisher_distance.GetInt() );

		trace_t	tr;
		UTIL_TraceLine( vMuzzlePos, vTestPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

		//Extinguish the fire where we hit
		FireSystem_ExtinguishInRadius( tr.endpos, fire_extinguisher_radius.GetInt(), fire_extinguisher_strength.GetFloat() );

		//Debug visualization
		if ( fire_extinguisher_debug.GetInt() )
		{
			int	radius = fire_extinguisher_radius.GetInt();

			NDebugOverlay::Line( vMuzzlePos, tr.endpos, 0, 0, 128, false, 0.0f );
			
			NDebugOverlay::Box( vMuzzlePos, Vector(-1, -1, -1), Vector(1, 1, 1), 0, 0, 128, false, 0.0f );
			NDebugOverlay::Box( tr.endpos, Vector(-2, -2, -2), Vector(2, 2, 2), 0, 0, 128, false, 0.0f );
			NDebugOverlay::Box( tr.endpos, Vector(-radius, -radius, -radius), Vector(radius, radius, radius), 0, 0, 255, false, 0.0f );
		}
	}
	else
	{
		StopJet();
	}
}

class CExtinguisherCharger : public CBaseToggle
{
public:
	DECLARE_CLASS( CExtinguisherCharger, CBaseToggle );

	void Spawn( void );
	bool CreateVPhysics();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int	ObjectCaps( void ) { return (BaseClass::ObjectCaps() | FCAP_CONTINUOUS_USE) & ~FCAP_ACROSS_TRANSITION; }

protected:
	float	m_flNextCharge; 
	bool	m_bSoundOn;

	void	TurnOff( void );

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( func_extinguishercharger, CExtinguisherCharger );

BEGIN_DATADESC( CExtinguisherCharger )

	DEFINE_FIELD( m_flNextCharge, FIELD_TIME),
	DEFINE_FIELD( m_bSoundOn, FIELD_BOOLEAN ),

	DEFINE_FUNCTION( TurnOff ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Spawn the object
//-----------------------------------------------------------------------------
void CExtinguisherCharger::Spawn( void )
{
	Precache();

	SetSolid( SOLID_BSP );
	SetMoveType( MOVETYPE_PUSH );

	SetModel( STRING( GetModelName() ) );

	m_bSoundOn = false;

	CreateVPhysics();
}

//-----------------------------------------------------------------------------

bool CExtinguisherCharger::CreateVPhysics()
{
	VPhysicsInitStatic();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CExtinguisherCharger::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Make sure that we have a caller
	if ( pActivator == NULL )
		return;
	
	// If it's not a player, ignore
	if ( pActivator->IsPlayer() == false )
		return;

	// Turn our sound on, if it's not already
	if ( m_bSoundOn == false )
	{
		EmitSound( "ExtinguisherCharger.Use" );
		m_bSoundOn = true;
	}

	SetNextThink( gpGlobals->curtime + 0.25 );
	
	SetThink( TurnOff );

	CBasePlayer	*pPlayer = ToBasePlayer( pActivator );

	if ( pPlayer )
	{
		//FIXME: Need a way to do this silently
		pPlayer->GiveAmmo( 1, "extinguisher" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExtinguisherCharger::TurnOff( void )
{
	//Turn the sound off
	if ( m_bSoundOn )
	{
		StopSound( "ExtinguisherCharger.Use" );
		m_bSoundOn = false;
	}
}
