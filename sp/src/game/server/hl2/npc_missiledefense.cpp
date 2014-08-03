//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include	"ai_basenpc.h"
#include	"ai_hull.h"
#include	"ai_senses.h"
#include	"ai_memory.h"
#include	"soundent.h"
#include	"smoke_trail.h"
#include	"weapon_rpg.h"
#include	"gib.h"
#include	"ndebugoverlay.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "ammodef.h"
#include "hl2_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MD_FULLAMMO	50


#define MD_BC_YAW		0
#define MD_BC_PITCH		1
#define MD_AP_LGUN		2
#define MD_AP_RGUN		1
#define MD_GIB_COUNT	4
#define MD_GIB_MODEL	"models/gibs/missile_defense_gibs.mdl"
#define MD_YAW_SPEED	24
#define MD_PITCH_SPEED  12

//=========================================================
//=========================================================
class CNPC_MissileDefense : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_MissileDefense, CAI_BaseNPC );
	DECLARE_DATADESC();

public:
	CNPC_MissileDefense( void ) { };
	void	Precache( void );
	void	Spawn( void );
	Class_T Classify( void ) { return CLASS_NONE; }
	int		GetSoundInterests( void ) { return SOUND_NONE; }
	float	MaxYawSpeed( void ) { return 90.f; }

	void	RunAI(void);
	void	FireCannons( void );
	void	AimGun( void );
	void	EnemyShootPosition(CBaseEntity* pEnemy, Vector *vPosition);

	void	Event_Killed( const CTakeDamageInfo &info );
	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void	Gib();
	void	GetGunAim( Vector *vecAim );	
	~CNPC_MissileDefense();

	Vector		m_vGunAng;
	int			m_iAmmoLoaded;
	float		m_flReloadedTime;
};

LINK_ENTITY_TO_CLASS( npc_missiledefense, CNPC_MissileDefense );

//=========================================================
//=========================================================
BEGIN_DATADESC( CNPC_MissileDefense )

	DEFINE_FIELD( m_iAmmoLoaded,		FIELD_INTEGER ),
	DEFINE_FIELD( m_flReloadedTime,	FIELD_TIME ),
	DEFINE_FIELD( m_vGunAng,			FIELD_VECTOR ),

END_DATADESC()

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_MissileDefense::Precache( void )
{
	PrecacheModel("models/missile_defense.mdl");
	PrecacheModel(MD_GIB_MODEL);

	PrecacheScriptSound( "NPC_MissileDefense.Attack" );
	PrecacheScriptSound( "NPC_MissileDefense.Reload" );
	PrecacheScriptSound( "NPC_MissileDefense.Turn" );

}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_MissileDefense::GetGunAim( Vector *vecAim )
{
	Vector vecPos;
	QAngle vecAng;

	GetAttachment( MD_AP_LGUN, vecPos, vecAng );

	vecAng.x = GetLocalAngles().x + GetBoneController( MD_BC_PITCH );
	vecAng.z = 0;
	vecAng.y = GetLocalAngles().y + GetBoneController( MD_BC_YAW );

	Vector vecForward;
	AngleVectors( vecAng, &vecForward );

	*vecAim = vecForward;
}


#define NOISE 0.035f
#define MD_ATTN_CANNON 0.4
//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_MissileDefense::FireCannons( void )
{
	// ----------------------------------------------
	//  Make sure I have an enemy
	// ----------------------------------------------
	if (GetEnemy() == NULL)
	{
		return;
	}

	// ----------------------------------------------
	//  Make sure I have ammo
	// ----------------------------------------------
	if( m_iAmmoLoaded < 1 )
	{
		return;
	}
	// ----------------------------------------------
	// Make sure gun it pointing in right direction	
	// ----------------------------------------------
	Vector vGunDir;
	GetGunAim( &vGunDir );
	Vector vTargetPos;
	EnemyShootPosition(GetEnemy(),&vTargetPos);

	Vector vTargetDir = vTargetPos - GetAbsOrigin();
	VectorNormalize( vTargetDir );

	float fDotPr = DotProduct( vGunDir, vTargetDir );
	if (fDotPr < 0.95)
	{
		return;
	}

	// ----------------------------------------------
	// Check line of sight
	// ----------------------------------------------
	trace_t tr;
	AI_TraceLine( GetEnemy()->EyePosition(), GetAbsOrigin(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr);
	if (tr.fraction < 1.0)
	{
		return;
	}

	Vector vecRight;
	Vector vecDir;
	Vector vecCenter;
	AngleVectors( GetLocalAngles(), NULL, &vecRight, NULL );

	vecCenter = WorldSpaceCenter();

	if( GetEnemy() == NULL )
	{
		return;
	}

	bool fSound = false;
	if( random->RandomInt( 0, 3 ) == 0 )
	{
		fSound = true;
	}


	EmitSound( "NPC_MissileDefense.Attack" );

	Vector vecGun;
	QAngle vecAng;
	
	GetAttachment( MD_AP_LGUN, vecGun, vecAng );

	Vector vecTarget;
	EnemyShootPosition(GetEnemy(),&vecTarget);

	vecDir = vecTarget - vecCenter;
	VectorNormalize(vecDir);
	vecDir.x += random->RandomFloat( -NOISE, NOISE );
	vecDir.y += random->RandomFloat( -NOISE, NOISE );

	Vector vecStart = vecGun + vecDir * 110;
	Vector vecEnd	= vecGun + vecDir * 4096;
	UTIL_Tracer( vecStart, vecEnd, 0, TRACER_DONT_USE_ATTACHMENT, 3000 + random->RandomFloat( 0, 2000 ), fSound );

	vecDir = vecTarget - vecCenter;
	VectorNormalize(vecDir);
	vecDir.x += random->RandomFloat( -NOISE, NOISE );
	vecDir.y += random->RandomFloat( -NOISE, NOISE );
	vecDir.z += random->RandomFloat( -NOISE, NOISE );

	GetAttachment( MD_AP_RGUN, vecGun, vecAng );
	vecStart = vecGun + vecDir * 110;
	vecEnd = vecGun + vecDir * 4096;
	UTIL_Tracer( vecStart, vecEnd, 0, TRACER_DONT_USE_ATTACHMENT, 3000 + random->RandomFloat( 0, 2000 ) );

	m_iAmmoLoaded -= 2;

	if( m_iAmmoLoaded < 1 )
	{
		// Incite a reload.
		EmitSound( "NPC_MissileDefense.Reload" );
		m_flReloadedTime = gpGlobals->curtime + 0.3;
		return;
	}

	// Do damage to the missile based on distance.
	// if < 1, make damage 0.

	float flDist = (GetEnemy()->GetLocalOrigin() - vecGun).Length();
	float flDamage;

	flDamage = 4000 - flDist;

	flDamage /= 1000.0;

	if( flDamage > 0 )
	{
		if( flDist <= 1500 )
		{
			flDamage *= 2;
		}

		CTakeDamageInfo info( this, this, flDamage, DMG_MISSILEDEFENSE );
		CalculateBulletDamageForce( &info, GetAmmoDef()->Index("SMG1"), vecDir, GetEnemy()->GetAbsOrigin() );
		GetEnemy()->TakeDamage( info );
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_MissileDefense::Spawn( void )
{
	Precache();

	SetModel( "models/missile_defense.mdl" );
	UTIL_SetSize( this, Vector( -36, -36 , 0 ), Vector( 36, 36, 64 ) );

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_NONE );
	m_takedamage		= DAMAGE_YES;
	SetBloodColor( DONT_BLEED );
	m_iHealth			= 10;
	m_flFieldOfView		= 0.1;
	m_NPCState			= NPC_STATE_NONE;
	CapabilitiesClear();
	CapabilitiesAdd ( bits_CAP_INNATE_RANGE_ATTACK1 );

	// Hate missiles	
	AddClassRelationship( CLASS_MISSILE, D_HT, 5 );

	m_spawnflags |= SF_NPC_LONG_RANGE;

	m_flReloadedTime = gpGlobals->curtime;

	InitBoneControllers();

	NPCInit();

	SetBoneController( MD_BC_YAW, 10 );
	SetBoneController( MD_BC_PITCH, 0 );

	SetBodygroup( 1, 1 );
	SetBodygroup( 2, 1 );
	SetBodygroup( 3, 1 );
	SetBodygroup( 4, 1 );

	m_NPCState = NPC_STATE_IDLE;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CNPC_MissileDefense::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// Only take blast damage
	if (info.GetDamageType() & DMG_BLAST )
	{
		return BaseClass::OnTakeDamage_Alive( info );
	}
	else
	{
		return 0;
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_MissileDefense::Event_Killed( const CTakeDamageInfo &info )
{
	StopSound( "NPC_MissileDefense.Turn" );
	Gib();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_MissileDefense::Gib(void)
{
	// Sparks
	for (int i = 0; i < 4; i++)
	{
		Vector sparkPos = GetAbsOrigin();
		sparkPos.x += random->RandomFloat(-12,12);
		sparkPos.y += random->RandomFloat(-12,12);
		sparkPos.z += random->RandomFloat(-12,12);
		g_pEffects->Sparks(sparkPos);
	}
	// Smoke
	UTIL_Smoke(GetAbsOrigin(), random->RandomInt(10, 15), 10);

	// Light
	CBroadcastRecipientFilter filter;

	te->DynamicLight( filter, 0.0,
			&GetAbsOrigin(), 255, 180, 100, 0, 100, 0.1, 0 );

	// Remove top parts
	SetBodygroup( 1, 0 );
	SetBodygroup( 2, 0 );
	SetBodygroup( 3, 0 );
	SetBodygroup( 4, 0 );
	m_takedamage = 0;
	SetThink(NULL);
	
	// Throw manhackgibs
	CGib::SpawnSpecificGibs( this, MD_GIB_COUNT, 300, 500, MD_GIB_MODEL);
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_MissileDefense::RunAI( void )
{
	// If my enemy is dead clear the memory and reset m_hEnemy
	if (GetEnemy() != NULL && 
		!GetEnemy()->IsAlive())
	{
		ClearEnemyMemory();
		SetEnemy( NULL );
	}

	if (GetEnemy() == NULL )
	{
		GetSenses()->Look( 4092 );
		SetEnemy( BestEnemy( ) );

		if (GetEnemy() != NULL)
		{
			m_iAmmoLoaded = MD_FULLAMMO;
			m_flReloadedTime = gpGlobals->curtime;
		}
	}

	if( m_iAmmoLoaded < 1 && gpGlobals->curtime > m_flReloadedTime )
	{
		m_iAmmoLoaded = MD_FULLAMMO;
	}

	AimGun();
	FireCannons();
	SetNextThink( gpGlobals->curtime + 0.05 );
}

//------------------------------------------------------------------------------
// Purpose : Add a little prediction into my enemy aim position
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_MissileDefense::EnemyShootPosition(CBaseEntity* pEnemy, Vector *vPosition)
{
	// This should never happen, but just in case
	if (!pEnemy)
	{
		return;
	}

	*vPosition = pEnemy->GetAbsOrigin();
	
	// Add prediction but prevents us from flipping around as enemy approaches us
	float	flDist		= (pEnemy->GetAbsOrigin() - GetAbsOrigin()).Length();
	Vector	vPredVel	= pEnemy->GetSmoothedVelocity() * 0.5;
	if ( flDist > vPredVel.Length())
	{
		*vPosition += vPredVel;
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_MissileDefense::AimGun( void )
{
	if (GetEnemy() == NULL)
	{
		StopSound( "NPC_MissileDefense.Turn" );
		return;
	}

	Vector forward, right, up;
	AngleVectors( GetLocalAngles(), &forward, &right, &up );
		
	// Get gun attachment points
	Vector vBasePos;
	QAngle vBaseAng;
	GetAttachment( MD_AP_LGUN, vBasePos, vBaseAng );

	Vector vTargetPos;
	EnemyShootPosition(GetEnemy(),&vTargetPos);

	Vector vTargetDir = vTargetPos - vBasePos;
	VectorNormalize( vTargetDir );

	Vector vecOut;
	vecOut.x = DotProduct( forward, vTargetDir );
	vecOut.y = -DotProduct( right, vTargetDir );
	vecOut.z = DotProduct( up, vTargetDir );

	QAngle angles;
	VectorAngles(vecOut, angles);

	if (angles.y > 180)
		angles.y = angles.y - 360;
	if (angles.y < -180)
		angles.y = angles.y + 360;
	if (angles.x > 180)
		angles.x = angles.x - 360;
	if (angles.x < -180)
		angles.x = angles.x + 360;

	float flOldX = m_vGunAng.x;
	float flOldY = m_vGunAng.y;

	if (angles.x > m_vGunAng.x)
		m_vGunAng.x = MIN( angles.x, m_vGunAng.x + MD_PITCH_SPEED );
	if (angles.x < m_vGunAng.x)
		m_vGunAng.x = MAX( angles.x, m_vGunAng.x - MD_PITCH_SPEED );
	if (angles.y > m_vGunAng.y)
		m_vGunAng.y = MIN( angles.y, m_vGunAng.y + MD_YAW_SPEED );
	if (angles.y < m_vGunAng.y)
		m_vGunAng.y = MAX( angles.y, m_vGunAng.y - MD_YAW_SPEED );

	m_vGunAng.y = SetBoneController( MD_BC_YAW,		m_vGunAng.y );
	m_vGunAng.x = SetBoneController( MD_BC_PITCH,	m_vGunAng.x );

	if (flOldX != m_vGunAng.x || flOldY != m_vGunAng.y)
	{
		EmitSound( "NPC_MissileDefense.Turn" );
	}
	else
	{
		StopSound( "NPC_MissileDefense.Turn" );
	}
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CNPC_MissileDefense::~CNPC_MissileDefense(void)
{
	StopSound( "NPC_MissileDefense.Turn" );
}
