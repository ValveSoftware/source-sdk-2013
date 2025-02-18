//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "player.h"
#include "soundent.h"
#include "te_particlesystem.h"
#include "ndebugoverlay.h"
#include "in_buttons.h"
#include "ai_basenpc.h"
#include "ai_memory.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MAX_BURN_RADIUS		256
#define RADIUS_GROW_RATE	50.0	// units/sec 

#define IMMOLATOR_TARGET_INVALID Vector( FLT_MAX, FLT_MAX, FLT_MAX )

class CWeaponImmolator : public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponImmolator, CBaseHLCombatWeapon );

	DECLARE_SERVERCLASS();

	CWeaponImmolator( void );
	
	void Precache( void );
	void PrimaryAttack( void );
	void ItemPostFrame( void );

	int CapabilitiesGet( void ) {	return bits_CAP_WEAPON_RANGE_ATTACK1;	}

	void ImmolationDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore );
	virtual bool WeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions );	
	virtual int	WeaponRangeAttack1Condition( float flDot, float flDist );

	void Update();
	void UpdateThink();

	void StartImmolating();
	void StopImmolating();
	bool IsImmolating() { return m_flBurnRadius != 0.0; }

	DECLARE_ACTTABLE();
	DECLARE_DATADESC();

	int	m_beamIndex;

	float m_flBurnRadius;
	float m_flTimeLastUpdatedRadius;

	Vector  m_vecImmolatorTarget;
};

IMPLEMENT_SERVERCLASS_ST(CWeaponImmolator, DT_WeaponImmolator)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( info_target_immolator, CPointEntity );
LINK_ENTITY_TO_CLASS( weapon_immolator, CWeaponImmolator );
PRECACHE_WEAPON_REGISTER( weapon_immolator );

BEGIN_DATADESC( CWeaponImmolator )

	DEFINE_FIELD( m_beamIndex, FIELD_INTEGER ),
	DEFINE_FIELD( m_flBurnRadius, FIELD_FLOAT ),
	DEFINE_FIELD( m_flTimeLastUpdatedRadius, FIELD_TIME ),
	DEFINE_FIELD( m_vecImmolatorTarget, FIELD_VECTOR ),

	DEFINE_ENTITYFUNC( UpdateThink ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Maps base activities to weapons-specific ones so our characters do the right things.
//-----------------------------------------------------------------------------
acttable_t CWeaponImmolator::m_acttable[] = 
{
	{	ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SNIPER_RIFLE, true }
};

IMPLEMENT_ACTTABLE( CWeaponImmolator );

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponImmolator::CWeaponImmolator( void )
{
	m_fMaxRange1 = 4096;
	StopImmolating();
}

void CWeaponImmolator::StartImmolating()
{
	// Start the radius really tiny because we use radius == 0.0 to 
	// determine whether the immolator is operating or not.
	m_flBurnRadius = 0.1;
	m_flTimeLastUpdatedRadius = gpGlobals->curtime;
	SetThink( UpdateThink );
	SetNextThink( gpGlobals->curtime );

	CSoundEnt::InsertSound( SOUND_DANGER, m_vecImmolatorTarget, 256, 5.0, GetOwner() );
}

void CWeaponImmolator::StopImmolating()
{
	m_flBurnRadius = 0.0;
	SetThink( NULL );
	m_vecImmolatorTarget= IMMOLATOR_TARGET_INVALID;
	m_flNextPrimaryAttack = gpGlobals->curtime + 5.0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponImmolator::Precache( void )
{
	m_beamIndex = PrecacheModel( "sprites/bluelaser1.vmt" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponImmolator::PrimaryAttack( void )
{
	WeaponSound( SINGLE );

	if( !IsImmolating() )
	{
		StartImmolating();
	} 
}

//-----------------------------------------------------------------------------
// This weapon is said to have Line of Sight when it CAN'T see the target, but
// can see a place near the target than can.
//-----------------------------------------------------------------------------
bool CWeaponImmolator::WeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions )
{
	CAI_BaseNPC* npcOwner = GetOwner()->MyNPCPointer();

	if( !npcOwner )
	{
		return false;
	}

	if( IsImmolating() )
	{
		// Don't update while Immolating. This is a committed attack.
		return false;
	}

	// Assume we won't find a target.
	m_vecImmolatorTarget = targetPos;
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Weapon firing conditions
//-----------------------------------------------------------------------------
int CWeaponImmolator::WeaponRangeAttack1Condition( float flDot, float flDist )
{
	if( m_flNextPrimaryAttack > gpGlobals->curtime )
	{
		// Too soon to attack!
		return COND_NONE;
	}

	if( IsImmolating() )
	{
		// Once is enough!
		return COND_NONE;
	}

	if(	m_vecImmolatorTarget == IMMOLATOR_TARGET_INVALID )
	{
		// No target!
		return COND_NONE;
	}

	if ( flDist > m_fMaxRange1 )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if ( flDot < 0.5f )	// UNDONE: Why check this here? Isn't the AI checking this already?
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_RANGE_ATTACK1;
}

void CWeaponImmolator::UpdateThink( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();

	if( pOwner && !pOwner->IsAlive() )
	{
		StopImmolating();
		return;
	}

	Update();
	SetNextThink( gpGlobals->curtime + 0.05 );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponImmolator::Update()
{
	float flDuration = gpGlobals->curtime - m_flTimeLastUpdatedRadius;
	if( flDuration != 0.0 )
	{
		m_flBurnRadius += RADIUS_GROW_RATE * flDuration;
	}

	// Clamp
	m_flBurnRadius = MIN( m_flBurnRadius, MAX_BURN_RADIUS );

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	Vector vecSrc;
	Vector vecAiming;

	if( pOwner )
	{
		vecSrc	 = pOwner->Weapon_ShootPosition( );
		vecAiming = pOwner->GetAutoaimVector(AUTOAIM_2DEGREES);
	}
	else
	{
		CBaseCombatCharacter *pOwner = GetOwner();

		vecSrc = pOwner->Weapon_ShootPosition( );
		vecAiming = m_vecImmolatorTarget - vecSrc;
		VectorNormalize( vecAiming );
	}

	trace_t	tr;
	UTIL_TraceLine( vecSrc, vecSrc + vecAiming * MAX_TRACE_LENGTH, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr );

	int brightness;
	brightness = 255 * (m_flBurnRadius/MAX_BURN_RADIUS);
	UTIL_Beam(  vecSrc,
				tr.endpos,
				m_beamIndex,
				0,		//halo index
				0,		//frame start
				2.0f,	//framerate
				0.1f,	//life
				20,		// width
				1,		// endwidth
				0,		// fadelength,
				1,		// noise

				0,		// red
				255,	// green
				0,		// blue,

				brightness, // bright
				100  // speed
				);


	if( tr.DidHitWorld() )
	{
		int beams;

		for( beams = 0 ; beams < 5 ; beams++ )
		{
			Vector vecDest;

			// Random unit vector
			vecDest.x = random->RandomFloat( -1, 1 );
			vecDest.y = random->RandomFloat( -1, 1 );
			vecDest.z = random->RandomFloat( 0, 1 );

			// Push out to radius dist.
			vecDest = tr.endpos + vecDest * m_flBurnRadius;

			UTIL_Beam(  tr.endpos,
						vecDest,
						m_beamIndex,
						0,		//halo index
						0,		//frame start
						2.0f,	//framerate
						0.15f,	//life
						20,		// width
						1.75,	// endwidth
						0.75,	// fadelength,
						15,		// noise

						0,		// red
						255,	// green
						0,		// blue,

						128, // bright
						100  // speed
						);
		}

		// Immolator starts to hurt a few seconds after the effect is seen
		if( m_flBurnRadius > 64.0 )
		{
			ImmolationDamage( CTakeDamageInfo( this, this, 1, DMG_BURN ), tr.endpos, m_flBurnRadius, CLASS_NONE );
		}
	}
	else
	{
		// The attack beam struck some kind of entity directly.
	}

	m_flTimeLastUpdatedRadius = gpGlobals->curtime;

	if( m_flBurnRadius >= MAX_BURN_RADIUS )
	{
		StopImmolating();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponImmolator::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();
}



void CWeaponImmolator::ImmolationDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore )
{
	CBaseEntity *pEntity = NULL;
	trace_t		tr;
	Vector		vecSpot;

	Vector vecSrc = vecSrcIn;

	// iterate on all entities in the vicinity.
	for ( CEntitySphereQuery sphere( vecSrc, flRadius ); pEntity = sphere.GetCurrentEntity(); sphere.NextEntity() )
	{
		CBaseCombatCharacter *pBCC;

		pBCC = pEntity->MyCombatCharacterPointer();

		if ( pBCC && !pBCC->IsOnFire() )
		{
			// UNDONE: this should check a damage mask, not an ignore
			if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
			{
				continue;
			}

			if( pEntity == GetOwner() )
			{
				continue;
			}

			pBCC->Ignite( random->RandomFloat( 15, 20 ) );
		}
	}
}
