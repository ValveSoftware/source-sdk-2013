//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Things thrown from the hand
//
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "ammodef.h"
#include "gamerules.h"
#include "grenade_brickbat.h"
#include "weapon_brickbat.h"
#include "soundent.h"
#include "decals.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Global Savedata for changelevel trigger
BEGIN_DATADESC( CGrenade_Brickbat )

	DEFINE_FIELD( m_nType, FIELD_INTEGER ),
	DEFINE_FIELD( m_bExplodes, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bBounceToFlat, FIELD_BOOLEAN ),

	// Function Pointers
	DEFINE_FUNCTION( BrickbatTouch ),
	DEFINE_FUNCTION( BrickbatThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( brickbat, CGrenade_Brickbat );

void CGrenade_Brickbat::Spawn( void )
{
	SetCollisionGroup( COLLISION_GROUP_PROJECTILE );
	SetTouch( BrickbatTouch );
	SetThink( BrickbatThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	m_takedamage = DAMAGE_YES;
	m_iHealth = 1;

	SetGravity( 1.0 );
	SetSequence( 1 );

	CreateVPhysics();
}

bool CGrenade_Brickbat::CreateVPhysics()
{
	VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
	IPhysicsObject *pPhysics = VPhysicsGetObject();
	if ( pPhysics )
	{
		// we want world touches
		unsigned int flags = pPhysics->GetCallbackFlags();
		pPhysics->SetCallbackFlags( flags | CALLBACK_GLOBAL_TOUCH_STATIC );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CGrenade_Brickbat::BrickbatTouch( CBaseEntity *pOther )
{
	// -----------------------------------------------------------
	// Might be physically simulated so get my velocity manually
	// -----------------------------------------------------------
	Vector vVelocity;
	GetVelocity(&vVelocity,NULL);

	// -----------------------------------
	// Do damage if we moving fairly fast
	// -----------------------------------
	if (vVelocity.Length() > 100)
	{
		if (GetThrower())
		{
			trace_t tr;
			tr = CBaseEntity::GetTouchTrace( );
			ClearMultiDamage( );
			Vector forward;
			AngleVectors( GetLocalAngles(), &forward );

			CTakeDamageInfo info( this, GetThrower(), m_flDamage, DMG_CRUSH );
			CalculateMeleeDamageForce( &info, forward, tr.endpos );
			pOther->DispatchTraceAttack( info, forward, &tr );
			ApplyMultiDamage();
		}
		// If this thrown item explodes, blow it up
		if (m_bExplodes) 
		{
			Detonate();
			return;
		}
	}
	else if (pOther->GetFlags() & FL_CLIENT)
	{
		SpawnBrickbatWeapon();
		return;
	}
}

//------------------------------------------------------------------------------
// Purpose : Brickbat grenade turns back into a brickbat weapon
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenade_Brickbat::SpawnBrickbatWeapon( void )
{
	CWeaponBrickbat *pBrickbat = (CWeaponBrickbat*)CBaseEntity::CreateNoSpawn( 
		"weapon_brickbat", GetLocalOrigin(), GetLocalAngles(), NULL );
	// Spawn after we set the ammo type so the correct model is used
	if (pBrickbat)
	{
		pBrickbat->m_iCurrentAmmoType = m_nType;
		pBrickbat->Spawn();
		VPhysicsDestroyObject();
		SetThink(NULL);
		UTIL_Remove(this);
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenade_Brickbat::BrickbatThink( void )
{
	// -----------------------------------------------------------
	// Might be physically simulated so get my velocity manually
	// -----------------------------------------------------------
	Vector vVelocity;
	AngularImpulse vAngVel;
	GetVelocity(&vVelocity,&vAngVel);

	// See if I can lose my owner (has dropper moved out of way?)
	// Want do this so owner can throw the brickbat
	if (GetOwnerEntity())
	{
		trace_t tr;
		Vector	vUpABit = GetAbsOrigin();
		vUpABit.z += 5.0;

		CBaseEntity* saveOwner = GetOwnerEntity();
		SetOwnerEntity( NULL );
		UTIL_TraceEntity( this, GetAbsOrigin(), vUpABit, MASK_SOLID, &tr );
		if ( tr.startsolid || tr.fraction != 1.0 )
		{
			SetOwnerEntity( saveOwner );
		}
	}

	// ---------------------------------------------------------------
	//	Make sure we're not resting on a living thing's bounding box
	// ---------------------------------------------------------------
	if (vVelocity.Length() < 0.01)
	{
		trace_t tr;
		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector(0,0,10), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction < 1.0 && tr.m_pEnt)
		{
			CBaseEntity *pEntity = tr.m_pEnt;
			if (pEntity->GetFlags() & (FL_CLIENT | FL_NPC))
			{
				// --------------------
				// Bounce me off 
				// --------------------
				Vector vNewVel;
				vNewVel.y = 100;
				vNewVel.x = random->RandomInt(-100,100);
				vNewVel.z = random->RandomInt(-100,100);

				// If physically simulated
				IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
				if ( pPhysicsObject )
				{
					pPhysicsObject->AddVelocity( &vNewVel, &vAngVel );
				}
				// Otherwise
				else
				{
					SetAbsVelocity( vNewVel );
				}
			}
		}
	}

	if (vVelocity.Length() < 0.01)
	{
		SpawnBrickbatWeapon();
	}
	SetNextThink( gpGlobals->curtime + 0.1f );
}

//=====================================================================
//	> Rock
//=====================================================================
class CGrenadeRockBB : public CGrenade_Brickbat
{
public:
	DECLARE_CLASS( CGrenadeRockBB, CGrenade_Brickbat );

	void Spawn(void)
	{
		m_nType = BRICKBAT_ROCK;
		SetModel( "models/props_junk/Rock001a.mdl" );
		BaseClass::Spawn();
	}
	void Precache( void )
	{
		PrecacheModel("models/props_junk/Rock001a.mdl");
		BaseClass::Precache();
	}
};
LINK_ENTITY_TO_CLASS( grenade_rockbb, CGrenadeRockBB );
PRECACHE_REGISTER(grenade_rockbb);


//=====================================================================
//	> BeerBottle
//=====================================================================
class CGrenadeBottle : public CGrenade_Brickbat
{
public:
	DECLARE_CLASS( CGrenadeBottle, CGrenade_Brickbat );

	void Spawn(void)
	{
		m_nType	= BRICKBAT_BOTTLE;
		m_bExplodes	= true;
		SetModel( "models/weapons/w_bb_bottle.mdl" );
		BaseClass::Spawn();
	}
	void Precache( void );
	void Detonate( void );
};

void CGrenadeBottle::Precache( void )
{
	PrecacheModel("models/weapons/w_bb_bottle.mdl");

	PrecacheScriptSound( "GrenadeBottle.Detonate" );

	BaseClass::Precache();
}

void CGrenadeBottle::Detonate( void )
{	
	trace_t trace;

	UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + GetAbsVelocity(), MASK_SOLID, this, COLLISION_GROUP_NONE, &trace);
	UTIL_DecalTrace( &trace, "BeerSplash" );

	EmitSound( "GrenadeBottle.Detonate" );

	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), 400, 0.5);

	UTIL_Remove( this );
}

LINK_ENTITY_TO_CLASS( grenade_beerbottle, CGrenadeBottle );
PRECACHE_REGISTER(grenade_beerbottle);


