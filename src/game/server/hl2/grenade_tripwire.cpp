//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the tripmine grenade.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "util.h"
#include "shake.h"
#include "grenade_tripwire.h"
#include "grenade_homer.h"
#include "rope.h"
#include "rope_shared.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar    sk_dmg_tripwire		( "sk_dmg_tripwire","0");
ConVar    sk_tripwire_radius	( "sk_tripwire_radius","0"); 

#define GRENADETRIPWIRE_MISSILEMDL	"models/Weapons/ar2_grenade.mdl"

#define TGRENADE_LAUNCH_VEL		1200
#define TGRENADE_SPIN_MAG		50
#define TGRENADE_SPIN_SPEED		100
#define TGRENADE_MISSILE_OFFSET 50
#define TGRENADE_MAX_ROPE_LEN	1500

LINK_ENTITY_TO_CLASS( npc_tripwire, CTripwireGrenade );

BEGIN_DATADESC( CTripwireGrenade )

	DEFINE_FIELD( m_flPowerUp,		FIELD_TIME ),
	DEFINE_FIELD( m_nMissileCount,	FIELD_INTEGER ),
	DEFINE_FIELD( m_vecDir,			FIELD_VECTOR ),
	DEFINE_FIELD( m_vTargetPos,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vTargetOffset,	FIELD_VECTOR ),
	DEFINE_FIELD( m_pRope,			FIELD_CLASSPTR ),
	DEFINE_FIELD( m_pHook,			FIELD_CLASSPTR ),

	// Function Pointers
	DEFINE_FUNCTION( WarningThink ),
	DEFINE_FUNCTION( PowerupThink ),
	DEFINE_FUNCTION( RopeBreakThink ),
	DEFINE_FUNCTION( FireThink ),

END_DATADESC()

CTripwireGrenade::CTripwireGrenade()
{
	m_vecDir.Init();
}

void CTripwireGrenade::Spawn( void )
{
	Precache( );

	SetMoveType( MOVETYPE_FLY );
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_SOLID );

	SetModel( "models/Weapons/w_slam.mdl" );

	m_nMissileCount	= 0;
	
	UTIL_SetSize(this, Vector( -4, -4, -2), Vector(4, 4, 2));

	m_flPowerUp = gpGlobals->curtime + 1.2;//<<CHECK>>get rid of this
	
	SetThink( WarningThink );
	SetNextThink( gpGlobals->curtime + 1.0f );

	m_takedamage		= DAMAGE_YES;

	m_iHealth = 1;

	m_pRope = NULL;
	m_pHook = NULL;

	// Tripwire grenade sits at 90 on wall so rotate back to get m_vecDir
	QAngle angles = GetLocalAngles();
	angles.x -= 90;

	AngleVectors( angles, &m_vecDir );
}


void CTripwireGrenade::Precache( void )
{
	PrecacheModel("models/Weapons/w_slam.mdl"); 

	PrecacheModel(GRENADETRIPWIRE_MISSILEMDL);
}


void CTripwireGrenade::WarningThink( void  )
{
	// play activate sound
	EmitSound( "TripwireGrenade.Activate" );

	// set to power up
	SetThink( PowerupThink );
	SetNextThink( gpGlobals->curtime + 1.0f );
}


void CTripwireGrenade::PowerupThink( void  )
{
	if (gpGlobals->curtime > m_flPowerUp)
	{
		MakeRope( );
		RemoveSolidFlags( FSOLID_NOT_SOLID );
		m_bIsLive			= true;
	}
	SetNextThink( gpGlobals->curtime + 0.1f );
}


void CTripwireGrenade::BreakRope( void )
{
	if (m_pRope)
	{
		m_pRope->m_RopeFlags |= ROPE_COLLIDE;
		m_pRope->DetachPoint(0);

		Vector vVelocity;
		m_pHook->GetVelocity( &vVelocity, NULL );
		if (vVelocity.Length() > 1)
		{
			m_pRope->DetachPoint(1);
		}
	}
}


void CTripwireGrenade::MakeRope( void )
{
	SetThink( RopeBreakThink );

	// Delay first think slightly so rope has time
	// to appear if person right in front of it
	SetNextThink( gpGlobals->curtime + 1.0f );

	// Create hook for end of tripwire
	m_pHook = (CTripwireHook*)CBaseEntity::Create( "tripwire_hook", GetLocalOrigin(), GetLocalAngles() );
	if (m_pHook)
	{
		Vector vShootVel = 800*(m_vecDir + Vector(0,0,0.3)+RandomVector(-0.01,0.01));
		m_pHook->SetVelocity( vShootVel, vec3_origin);
		m_pHook->SetOwnerEntity( this );
		m_pHook->m_hGrenade		= this;

		m_pRope = CRopeKeyframe::Create(this,m_pHook,0,0);
		if (m_pRope)
		{
			m_pRope->m_Width		= 1;
			m_pRope->m_RopeLength	= 3;
			m_pRope->m_Slack		= 100;

			CPASAttenuationFilter filter( this,"TripwireGrenade.ShootRope" );
			EmitSound( filter, entindex(),"TripwireGrenade.ShootRope" );
		}
	}
}

void CTripwireGrenade::Attach( void )
{
	StopSound( "TripwireGrenade.ShootRope" );
}

void CTripwireGrenade::RopeBreakThink( void  )
{
	// See if I can go solid yet (has dropper moved out of way?)
	if (IsSolidFlagSet(FSOLID_NOT_SOLID))
	{
		trace_t tr;
		Vector	vUpBit = GetAbsOrigin();
		vUpBit.z += 5.0;

		UTIL_TraceEntity( this, GetAbsOrigin(), vUpBit, MASK_SHOT, &tr );
		if ( !tr.startsolid && (tr.fraction == 1.0) )
		{
			RemoveSolidFlags( FSOLID_NOT_SOLID );
		}
	}

	// Check if rope had gotten beyond it's max length
	float flRopeLength = (GetAbsOrigin()-m_pHook->GetAbsOrigin()).Length();
	if (flRopeLength > TGRENADE_MAX_ROPE_LEN)
	{
		// Shoot missiles at hook
		m_iHealth = 0;
		BreakRope();
		m_vTargetPos = m_pHook->GetAbsOrigin();
		CrossProduct ( m_vecDir, Vector(0,0,1), m_vTargetOffset );
		m_vTargetOffset *=TGRENADE_MISSILE_OFFSET; 
		SetThink(FireThink);
		FireThink();
	}

	// Check to see if can see hook
	// NOT MASK_SHOT because we want only simple hit boxes
	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), m_pHook->GetAbsOrigin(), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	// If can't see hook
	CBaseEntity *pEntity = tr.m_pEnt;
	if (tr.fraction != 1.0 && pEntity != m_pHook)
	{
		// Shoot missiles at place where rope was intersected
		m_iHealth = 0;
		BreakRope();
		m_vTargetPos = tr.endpos;
		CrossProduct ( m_vecDir, Vector(0,0,1), m_vTargetOffset );
		m_vTargetOffset *=TGRENADE_MISSILE_OFFSET; 
		SetThink(FireThink);
		FireThink();
		return;
	}

	SetNextThink( gpGlobals->curtime + 0.1f );
}

//------------------------------------------------------------------------------
// Purpose : Die if I take any damage
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CTripwireGrenade::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// Killed upon any damage
	Event_Killed( info );
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: If someone damaged, me shoot of my missiles and die
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CTripwireGrenade::Event_Killed( const CTakeDamageInfo &info )
{
	if (m_iHealth > 0)
	{
		// Fire missiles and blow up
		for (int i=0;i<6;i++)
		{
			Vector vTargetPos = GetAbsOrigin() + RandomVector(-600,600);
			FireMissile(vTargetPos);
		}
		BreakRope();
		UTIL_Remove(this);
	}
}

//------------------------------------------------------------------------------
// Purpose : Fire a missile at the target position
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CTripwireGrenade::FireMissile(const Vector &vTargetPos)
{
	Vector vTargetDir		= (vTargetPos - GetAbsOrigin());
	VectorNormalize(vTargetDir);

	float flGravity			= 0.0001;	// No gravity on the missiles
	bool  bSmokeTrail		= true;
	float flHomingSpeed		= 0;
	Vector vLaunchVelocity	= TGRENADE_LAUNCH_VEL*vTargetDir;
	float flSpinMagnitude	= TGRENADE_SPIN_MAG;
	float flSpinSpeed		= TGRENADE_SPIN_SPEED;

		//<<CHECK>> hold in string_t
	CGrenadeHomer *pGrenade = CGrenadeHomer::CreateGrenadeHomer( MAKE_STRING(GRENADETRIPWIRE_MISSILEMDL), MAKE_STRING("TripwireGrenade.FlySound"),  GetAbsOrigin(), vec3_angle, edict() );

	pGrenade->Spawn( );
	pGrenade->SetSpin(flSpinMagnitude,flSpinSpeed);
	pGrenade->SetHoming(0,0,0,0,0);
	pGrenade->SetDamage(sk_dmg_tripwire.GetFloat());
	pGrenade->SetDamageRadius(sk_tripwire_radius.GetFloat());
	pGrenade->Launch(this,NULL,vLaunchVelocity,flHomingSpeed,flGravity,bSmokeTrail);

	// Calculate travel time
	float flTargetDist	= (GetAbsOrigin() - vTargetPos).Length();

	pGrenade->m_flDetonateTime = gpGlobals->curtime + flTargetDist/TGRENADE_LAUNCH_VEL;

}

//------------------------------------------------------------------------------
// Purpose : Shoot off a series of missiles over time, then go intert
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CTripwireGrenade::FireThink()
{
	SetNextThink( gpGlobals->curtime + 0.16f );

	Vector vTargetPos		= m_vTargetPos + (m_vTargetOffset * m_nMissileCount);
	FireMissile(vTargetPos);

	vTargetPos		= m_vTargetPos - (m_vTargetOffset * m_nMissileCount);
	FireMissile(vTargetPos);


	m_nMissileCount++;
	if (m_nMissileCount > 4)
	{
		m_iHealth = -1;
		SetThink( NULL );
	}
}

// ####################################################################
//   CTripwireHook
//
//		This is what the tripwire shoots out at the end of the rope
// ####################################################################
LINK_ENTITY_TO_CLASS( tripwire_hook, CTripwireHook );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CTripwireHook )

	DEFINE_FIELD( m_hGrenade, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bAttached, FIELD_BOOLEAN ),

END_DATADESC()


void CTripwireHook::Spawn( void )
{

	Precache( );
	SetModel( "models/Weapons/w_grenade.mdl" );//<<CHECK>>

	UTIL_SetSize(this, Vector( -1, -1, -1), Vector(1,1, 1));

	m_takedamage		= DAMAGE_NO;
	m_bAttached			= false;

	CreateVPhysics();
}

bool CTripwireHook::CreateVPhysics()
{
	// Create the object in the physics system
	IPhysicsObject *pPhysicsObject = VPhysicsInitNormal( SOLID_BBOX, 0, false );
	
	// Make sure I get touch called for static geometry
	if ( pPhysicsObject )
	{
		int flags = pPhysicsObject->GetCallbackFlags();
		flags |= CALLBACK_GLOBAL_TOUCH_STATIC;
		pPhysicsObject->SetCallbackFlags(flags);
	}
	return true;
}


void CTripwireHook::Precache( void )
{
	PrecacheModel("models/Weapons/w_grenade.mdl"); //<<CHECK>>
}

void CTripwireHook::EndTouch( CBaseEntity *pOther )
{
	//<<CHECK>>do instead by clearing touch function
	if (!m_bAttached)
	{
		m_bAttached = true;

		SetVelocity(vec3_origin, vec3_origin);
		SetMoveType( MOVETYPE_NONE );

		EmitSound( "TripwireGrenade.Hook" );

		// Let the tripwire grenade know that I've attached
		CTripwireGrenade* pGrenade = dynamic_cast<CTripwireGrenade*>((CBaseEntity*)m_hGrenade);
		if (pGrenade)
		{
			pGrenade->Attach();
		}
	}
}

void CTripwireHook::SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->AddVelocity( &velocity, &angVelocity );
	}
}