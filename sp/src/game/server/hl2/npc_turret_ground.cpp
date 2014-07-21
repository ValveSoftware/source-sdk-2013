//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Combine gun turret that emerges from a trapdoor in the ground.
//
//=============================================================================//
#include "cbase.h"
#include "npc_turret_ground.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_senses.h"
#include "ai_memory.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "npcevent.h"
#include "IEffects.h"
#include "ammodef.h"
#include "beam_shared.h"
#include "explode.h"
#include "te_effect_dispatch.h"

#define GROUNDTURRET_BEAM_SPRITE "materials/effects/bluelaser2.vmt"

#define GROUNDTURRET_VIEWCONE		60.0f // (degrees)
#define GROUNDTURRET_RETIRE_TIME	7.0f

ConVar ai_newgroundturret ( "ai_newgroundturret", "0" );


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( npc_turret_ground, CNPC_GroundTurret );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_GroundTurret )
	DEFINE_FIELD( m_iAmmoType,		FIELD_INTEGER ),
	DEFINE_FIELD( m_pSmoke,			FIELD_CLASSPTR ),
	DEFINE_FIELD( m_vecSpread,		FIELD_VECTOR ),
	DEFINE_FIELD( m_bEnabled,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flTimeNextShoot, FIELD_TIME ),
	DEFINE_FIELD( m_flTimeLastSawEnemy, FIELD_TIME ),
	DEFINE_FIELD( m_iDeathSparks,	FIELD_INTEGER ),
	DEFINE_FIELD( m_bHasExploded,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flSensingDist,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flTimeNextPing, FIELD_TIME ),
	DEFINE_FIELD( m_bSeeEnemy,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vecClosedPos,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecLightOffset,	FIELD_POSITION_VECTOR ),

	DEFINE_THINKFUNC( DeathEffects ),

	DEFINE_OUTPUT( m_OnAreaClear, "OnAreaClear" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	// DEFINE_FIELD( m_ShotSounds, FIELD_SHORT ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_GroundTurret::Precache( void )
{
	PrecacheModel( GROUNDTURRET_BEAM_SPRITE );
	PrecacheModel( "models/combine_turrets/ground_turret.mdl" );

	PrecacheScriptSound( "NPC_CeilingTurret.Deploy" );
	m_ShotSounds = PrecacheScriptSound( "NPC_FloorTurret.ShotSounds" );
	PrecacheScriptSound( "NPC_FloorTurret.Die" );
	PrecacheScriptSound( "NPC_FloorTurret.Ping" );
	PrecacheScriptSound( "DoSpark" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_GroundTurret::Spawn( void )
{
	Precache();

	UTIL_SetModel( this, "models/combine_turrets/ground_turret.mdl" );

	SetNavType( NAV_FLY );
	SetSolid( SOLID_VPHYSICS );

	SetBloodColor( DONT_BLEED );
	m_iHealth			= 125;
	m_flFieldOfView		= cos( ((GROUNDTURRET_VIEWCONE / 2.0f) * M_PI / 180.0f) );
	m_NPCState			= NPC_STATE_NONE;

	m_vecSpread.x = 0.5;
	m_vecSpread.y = 0.5;
	m_vecSpread.z = 0.5;

	CapabilitiesClear();

	AddEFlags( EFL_NO_DISSOLVE );

	NPCInit();

	CapabilitiesAdd( bits_CAP_SIMPLE_RADIUS_DAMAGE );

	m_iAmmoType = GetAmmoDef()->Index( "PISTOL" );

	m_pSmoke = NULL;

	m_bHasExploded = false;
	m_bEnabled = false;

	if( ai_newgroundturret.GetBool() )
	{
		m_flSensingDist = 384;
		SetDistLook( m_flSensingDist );
	}
	else
	{
		m_flSensingDist = 2048;
	}

	if( !GetParent() )
	{
		DevMsg("ERROR! npc_ground_turret with no parent!\n");
		UTIL_Remove(this);
		return;
	}

	m_flTimeNextShoot = gpGlobals->curtime;
	m_flTimeNextPing = gpGlobals->curtime;

	m_vecClosedPos = GetAbsOrigin();

	StudioFrameAdvance();

	Vector vecPos;

	GetAttachment( "eyes", vecPos );
	SetViewOffset( vecPos - GetAbsOrigin() );

	GetAttachment( "light", vecPos );
	m_vecLightOffset = vecPos - GetAbsOrigin();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_GroundTurret::CreateVPhysics( void )
{
	//Spawn our physics hull
	if ( !VPhysicsInitStatic() )
	{
		DevMsg( "npc_turret_ground unable to spawn physics object!\n" );
	}

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_GroundTurret::PrescheduleThink()
{
	if( UTIL_FindClientInPVS(edict()) )
	{
		SetNextThink( gpGlobals->curtime + 0.03f );
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_GroundTurret::Classify( void )
{
	if( !IsOpen() )
	{
		// NPC's should disregard me if I'm closed.
		return CLASS_NONE;
	}
	else
	{
		return CLASS_COMBINE;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_GroundTurret::PostNPCInit()
{
	BaseClass::PostNPCInit();
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_GroundTurret::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if( !info.GetInflictor() )
	{
		return 0;
	}

	// Only take damage from self (kill input from my bullseye) or missiles.
	if( info.GetInflictor() != this && info.GetInflictor()->Classify() != CLASS_MISSILE )
	{
		return 0;
	}

	CTakeDamageInfo infoCopy = info;

	if( info.GetInflictor() == this )
	{
		// Taking damage from myself, make sure it's fatal.
		infoCopy.SetDamage( GetHealth() );
		infoCopy.SetDamageType( DMG_REMOVENORAGDOLL | DMG_GENERIC );
	}

	return BaseClass::OnTakeDamage_Alive( infoCopy );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_GroundTurret::Event_Killed( const CTakeDamageInfo &info )
{
	BaseClass::Event_Killed( info );

	if ( m_pSmoke != NULL )
		return;

	m_pSmoke = SmokeTrail::CreateSmokeTrail();
	
	if ( m_pSmoke )
	{
		m_pSmoke->m_SpawnRate			= 18;
		m_pSmoke->m_ParticleLifetime	= 3.0;
		m_pSmoke->m_StartSize			= 8;
		m_pSmoke->m_EndSize				= 32;
		m_pSmoke->m_SpawnRadius			= 16;
		m_pSmoke->m_MinSpeed			= 8;
		m_pSmoke->m_MaxSpeed			= 32;
		m_pSmoke->m_Opacity 			= 0.6;
		
		m_pSmoke->m_StartColor.Init( 0.25f, 0.25f, 0.25f );
		m_pSmoke->m_EndColor.Init( 0, 0, 0 );
		m_pSmoke->SetLifetime( 30.0f );
		m_pSmoke->FollowEntity( this );
	}

	m_iDeathSparks = random->RandomInt( 6, 12 );

	SetThink( &CNPC_GroundTurret::DeathEffects );
	SetNextThink( gpGlobals->curtime + 1.5f );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_GroundTurret::DeathEffects()
{
	if( !m_bHasExploded )
	{
		//ExplosionCreate( GetAbsOrigin(), QAngle( 0, 0, 1 ), this, 150, 150, false );
		CTakeDamageInfo info;
		DeathSound( info );
		m_bHasExploded = true;
		SetNextThink( gpGlobals->curtime + 0.5 );
	}
	else
	{
		// Sparks
		EmitSound( "DoSpark" );
		m_iDeathSparks--;

		if( m_iDeathSparks == 0 )
		{
			SetThink(NULL);
			return;
		}

		SetNextThink( gpGlobals->curtime + random->RandomFloat( 0.5, 2.5 ) );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_GroundTurret::DeathSound( const CTakeDamageInfo &info )
{
	EmitSound("NPC_FloorTurret.Die");
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_GroundTurret::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
#if 1
	//BaseClass::MakeTracer( vecTracerSrc, tr, iTracerType );
	UTIL_Tracer( vecTracerSrc, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, 5000, true, "AR2Tracer" );
#else
	CBeam *pBeam;
	int	width = 2;

	pBeam = CBeam::BeamCreate( GROUNDTURRET_BEAM_SPRITE, width );
	if ( !pBeam )
		return;
	
	pBeam->SetStartPos( vecTracerSrc );
	pBeam->SetEndPos( tr.endpos );
	pBeam->SetWidth( width );
	pBeam->SetEndWidth( width / 4.0f );

	pBeam->SetBrightness( 100 );
	pBeam->SetColor( 0, 145+random->RandomInt( -16, 16 ), 255 );
	pBeam->RelinkBeam();
	pBeam->LiveForTime( random->RandomFloat( 0.2f, 0.5f ) );
#endif
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_GroundTurret::GatherConditions()
{
	if( !IsEnabled() )
	{
		return;
	}

	if( !IsOpen() && !UTIL_FindClientInPVS( edict() ) )
	{
		return;
	}

	// Throw away old enemies so the turret can retire
	AIEnemiesIter_t iter;

	for( AI_EnemyInfo_t *pEMemory = GetEnemies()->GetFirst(&iter); pEMemory != NULL; pEMemory = GetEnemies()->GetNext(&iter) )
	{
		if( pEMemory->timeLastSeen < gpGlobals->curtime - GROUNDTURRET_RETIRE_TIME )
		{
			pEMemory->hEnemy = NULL;
		}
	}

	BaseClass::GatherConditions();

	if( GetEnemy() && HasCondition(COND_SEE_ENEMY) )
	{
		m_flTimeLastSawEnemy = gpGlobals->curtime;
	}
	else
	{
		if( gpGlobals->curtime - m_flTimeLastSawEnemy >= GROUNDTURRET_RETIRE_TIME )
		{
			m_OnAreaClear.FireOutput(this, this);
			m_flTimeLastSawEnemy = FLT_MAX;
			return;
		}
	}

	if( HasCondition( COND_SEE_ENEMY ) )
	{
		m_bSeeEnemy = true;
	}
	else
	{
		m_bSeeEnemy = false;
	}

	if( GetEnemy() && m_bSeeEnemy && IsEnabled() )
	{
		if( m_flTimeNextShoot < gpGlobals->curtime )
		{
			Shoot();
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
Vector CNPC_GroundTurret::EyePosition()
{
	if( ai_newgroundturret.GetBool() )
	{
		return GetAbsOrigin() + Vector( 0, 0, 6 );
	}

	return GetAbsOrigin() + GetViewOffset();
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_GroundTurret::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	if ( BaseClass::FVisible( pEntity, traceMask, ppBlocker ) )
		return true;
	if ( ( pEntity->GetAbsOrigin().AsVector2D() - GetAbsOrigin().AsVector2D() ).LengthSqr() < Square(10*12) &&
		 FInViewCone( pEntity->GetAbsOrigin() ) &&
		 BaseClass::FVisible( pEntity->GetAbsOrigin() + Vector( 0, 0, 1 ), traceMask, ppBlocker ) )
		return true;
	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_GroundTurret::QuerySeeEntity( CBaseEntity *pEntity, bool bOnlyHateOrFearIfNPC)
{
	float flDist;

	flDist = (pEntity->GetAbsOrigin() - EyePosition()).Length2DSqr();

	if( flDist <= m_flSensingDist * m_flSensingDist )
	{
		return BaseClass::QuerySeeEntity(pEntity, bOnlyHateOrFearIfNPC);
	}

	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_GroundTurret::IsEnabled()
{
	if( ai_newgroundturret.GetBool() )
	{
		return true;
	}

	return m_bEnabled;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_GroundTurret::IsOpen()
{
	// The method is hacky but in the end, this does actually give
	// us a pretty good idea if the turret is open or closed.
	return( fabs(GetAbsOrigin().z - m_vecClosedPos.z ) > 1.0f );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_GroundTurret::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_GROUNDTURRET_SCAN:
		Scan();
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_GroundTurret::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_GROUNDTURRET_SCAN:
		Scan();
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_GroundTurret::SelectSchedule( void )
{
	return SCHED_GROUND_TURRET_IDLE;
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_GroundTurret::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
	case SCHED_IDLE_STAND:
		return SCHED_GROUND_TURRET_IDLE;
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
// Purpose: Override base class activiites
// Input  :
// Output :
//-----------------------------------------------------------------------------
Activity CNPC_GroundTurret::NPC_TranslateActivity( Activity activity )
{
	return ACT_IDLE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_GroundTurret::Shoot()
{
	FireBulletsInfo_t info;

	Vector vecSrc = EyePosition();
	Vector vecDir;

	GetVectors( &vecDir, NULL, NULL );

	for( int i = 0 ; i < 1 ; i++ )
	{
		info.m_vecSrc = vecSrc;
		
		if( i > 0 || !GetEnemy()->IsPlayer() )
		{
			// Subsequent shots or shots at non-players random
			GetVectors( &info.m_vecDirShooting, NULL, NULL );
			info.m_vecSpread = m_vecSpread;
		}
		else
		{
			// First shot is at the enemy.
			info.m_vecDirShooting = GetActualShootTrajectory( vecSrc );
			info.m_vecSpread = VECTOR_CONE_PRECALCULATED;
		}
		
		info.m_iTracerFreq = 1;
		info.m_iShots = 1;
		info.m_pAttacker = this;
		info.m_flDistance = MAX_COORD_RANGE;
		info.m_iAmmoType = m_iAmmoType;

		FireBullets( info );
	}

	// Do the AR2 muzzle flash
	CEffectData data;
	data.m_nEntIndex = entindex();
	data.m_nAttachmentIndex = LookupAttachment( "eyes" );
	data.m_flScale = 1.0f;
	data.m_fFlags = MUZZLEFLASH_COMBINE;
	DispatchEffect( "MuzzleFlash", data );

	EmitSound( "NPC_FloorTurret.ShotSounds", m_ShotSounds );

	if( IsX360() )
	{
		m_flTimeNextShoot = gpGlobals->curtime + 0.2;
	}
	else
	{
		m_flTimeNextShoot = gpGlobals->curtime + 0.09;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_GroundTurret::ProjectBeam( const Vector &vecStart, const Vector &vecDir, int width, int brightness, float duration )
{
	CBeam *pBeam;
	pBeam = CBeam::BeamCreate( GROUNDTURRET_BEAM_SPRITE, width );
	if ( !pBeam )
		return;

	trace_t tr;
	AI_TraceLine( vecStart, vecStart + vecDir * m_flSensingDist, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	
	pBeam->SetStartPos( tr.endpos );
	pBeam->SetEndPos( tr.startpos );
	pBeam->SetWidth( width );
	pBeam->SetEndWidth( 0.1 );
	pBeam->SetFadeLength( 16 );

	pBeam->SetBrightness( brightness );
	pBeam->SetColor( 0, 145+random->RandomInt( -16, 16 ), 255 );
	pBeam->RelinkBeam();
	pBeam->LiveForTime( duration );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_GroundTurret::Scan()
{
	if( m_bSeeEnemy )
	{
		// Using a bool for this check because the condition gets wiped out by changing schedules.
		return;
	}

	if( IsOpeningOrClosing() )
	{
		// Moving.
		return;
	}

	if( !IsOpen() )
	{
		// Closed
		return;
	}

	if( !UTIL_FindClientInPVS(edict()) )
	{
		return;
	}

	if( gpGlobals->curtime >= m_flTimeNextPing )
	{
		EmitSound( "NPC_FloorTurret.Ping" );
		m_flTimeNextPing = gpGlobals->curtime + 1.0f;
	}

	QAngle	scanAngle;
	Vector	forward;
	Vector	vecEye = GetAbsOrigin() + m_vecLightOffset;

	// Draw the outer extents
	scanAngle = GetAbsAngles();
	scanAngle.y += (GROUNDTURRET_VIEWCONE / 2.0f);
	AngleVectors( scanAngle, &forward, NULL, NULL );
	ProjectBeam( vecEye, forward, 1, 30, 0.1 );

	scanAngle = GetAbsAngles();
	scanAngle.y -= (GROUNDTURRET_VIEWCONE / 2.0f);
	AngleVectors( scanAngle, &forward, NULL, NULL );
	ProjectBeam( vecEye, forward, 1, 30, 0.1 );

	// Draw a sweeping beam
	scanAngle = GetAbsAngles();
	scanAngle.y += (GROUNDTURRET_VIEWCONE / 2.0f) * sin( gpGlobals->curtime * 3.0f );
	
	AngleVectors( scanAngle, &forward, NULL, NULL );
	ProjectBeam( vecEye, forward, 1, 30, 0.3 );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_GroundTurret::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;

	// Because the turret might not ever ACQUIRE an enemy, we need to arrange to 
	// retire after a few seconds.
	m_flTimeLastSawEnemy = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_GroundTurret::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_groundturret, CNPC_GroundTurret )

	DECLARE_TASK( TASK_GROUNDTURRET_SCAN );

	DEFINE_SCHEDULE
	(
		SCHED_GROUND_TURRET_IDLE,

		"	Tasks "
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
		"		TASK_GROUNDTURRET_SCAN	0"
		""
		"	Interrupts "
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_LOST_ENEMY"
	)

	DEFINE_SCHEDULE
	(
		SCHED_GROUND_TURRET_ATTACK,

		"	Tasks "
		"		TASK_WAIT_INDEFINITE	0"
		""
		"	Interrupts "
		"		COND_NEW_ENEMY"
		"		COND_LOST_ENEMY"
		"		COND_SEE_ENEMY"
	)

AI_END_CUSTOM_NPC()

