//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "ammodef.h"
#include "ai_memory.h"
#include "weapon_rpg.h"
#include "effect_color_tables.h"
#include "te_effect_dispatch.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern const char* g_pModelNameLaser;

// No model, impervious to damage.
#define SF_STARTDISABLED	(1 << 19)

#define CANNON_PAINT_ENEMY_TIME			1.0f
#define CANNON_SUBSEQUENT_PAINT_TIME	0.4f
#define	CANNON_PAINT_NPC_TIME_NOISE		1.0f

#define	NUM_ANCILLARY_BEAMS	4

int gHaloTexture = 0;

//-----------------------------------------------------------------------------
//
// Combine Cannon
//
//-----------------------------------------------------------------------------
class CNPC_Combine_Cannon : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_Combine_Cannon, CAI_BaseNPC );

public:
						CNPC_Combine_Cannon( void );
	virtual void		Precache( void );
	virtual void		Spawn( void );
	virtual Class_T		Classify( void );
	virtual float		MaxYawSpeed( void );
	virtual Vector		EyePosition( void );
	virtual void		UpdateOnRemove( void );
	virtual int			OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual bool		QuerySeeEntity( CBaseEntity *pEntity, bool bOnlyHateOrFearIfNPC = false );
	virtual void		StartTask( const Task_t *pTask );
	virtual void		RunTask( const Task_t *pTask );
	virtual int			RangeAttack1Conditions( float flDot, float flDist );
	virtual int			SelectSchedule( void );
	virtual int			TranslateSchedule( int scheduleType );
	virtual void		PrescheduleThink( void );
	virtual bool		FCanCheckAttacks ( void );
	virtual int			Restore( IRestore &restore );
	virtual void		OnScheduleChange( void );
	virtual bool		FVisible( CBaseEntity *pEntity, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL );
	
	virtual bool		WeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions) { return true; }

	virtual int			GetSoundInterests( void ) { return (SOUND_PLAYER|SOUND_COMBAT|SOUND_DANGER); }

	virtual bool		ShouldNotDistanceCull( void ) { return true; }

	virtual void		UpdateEfficiency( bool bInPVS )	{ SetEfficiency( ( GetSleepState() != AISS_AWAKE ) ? AIE_DORMANT : AIE_NORMAL ); SetMoveEfficiency( AIME_NORMAL ); }

	virtual const char *GetTracerType( void ) { return "HelicopterTracer"; }

private:

	void	ScopeGlint( void );
	void	AdjustShotPosition( CBaseEntity *pTarget, Vector *vecIn );

	float	GetRefireTime( void ) { return 0.1f; }

	bool IsLaserOn( void ) { return m_pBeam != NULL; }
	bool FireBullet( const Vector &vecTarget, bool bDirectShot );
	Vector  DesiredBodyTarget( CBaseEntity *pTarget );
	Vector	LeadTarget( CBaseEntity *pTarget );
	Vector GetBulletOrigin( void );

	static const char *pAttackSounds[];
	
	void ClearTargetGroup( void );

	float GetWaitTimePercentage( float flTime, bool fLinear );

	void GetPaintAim( const Vector &vecStart, const Vector &vecGoal, float flParameter, Vector *pProgress );

	bool VerifyShot( CBaseEntity *pTarget );

	void SetSweepTarget( const char *pszTarget );

	// Inputs
	void InputEnableSniper( inputdata_t &inputdata );
	void InputDisableSniper( inputdata_t &inputdata );

	void LaserOff( void );
	void LaserOn( const Vector &vecTarget, const Vector &vecDeviance );

	void PaintTarget( const Vector &vecTarget, float flPaintTime );

private:

	void	CreateLaser( void );
	void	CreateAncillaryBeams( void );
	void	UpdateAncillaryBeams( float flConvergencePerc, const Vector &vecOrigin, const Vector &vecBasis );

	int		m_iAmmoType;
	float	m_flBarrageDuration;
	Vector	m_vecPaintCursor;
	float	m_flPaintTime;

	CHandle<CBeam>				m_pBeam;
	CHandle<CBeam>				m_pAncillaryBeams[NUM_ANCILLARY_BEAMS];
	EHANDLE						m_hBarrageTarget;

	bool						m_fEnabled;
	Vector						m_vecPaintStart; // used to track where a sweep starts for the purpose of interpolating.
	float						m_flTimeLastAttackedPlayer;
	float						m_flTimeLastShotMissed;
	float						m_flSightDist;

	DEFINE_CUSTOM_AI;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( npc_combine_cannon, CNPC_Combine_Cannon );

//=========================================================
//=========================================================
BEGIN_DATADESC( CNPC_Combine_Cannon )

	DEFINE_FIELD( m_fEnabled, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vecPaintStart, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_flPaintTime, FIELD_TIME ),
	DEFINE_FIELD( m_vecPaintCursor, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_pBeam, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flTimeLastAttackedPlayer, FIELD_TIME ),
	DEFINE_FIELD( m_flTimeLastShotMissed, FIELD_TIME ),
	DEFINE_FIELD( m_iAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( m_flBarrageDuration, FIELD_TIME ),
	DEFINE_FIELD( m_hBarrageTarget, FIELD_EHANDLE ),

	DEFINE_ARRAY( m_pAncillaryBeams, FIELD_EHANDLE, NUM_ANCILLARY_BEAMS ),

	DEFINE_KEYFIELD( m_flSightDist, FIELD_FLOAT, "sightdist" ),
	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableSniper", InputEnableSniper ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableSniper", InputDisableSniper ),

END_DATADESC()


//=========================================================
// Private conditions
//=========================================================
enum Sniper_Conds
{
	COND_CANNON_ENABLED = LAST_SHARED_CONDITION,
	COND_CANNON_DISABLED,
	COND_CANNON_NO_SHOT,
};


//=========================================================
// schedules
//=========================================================
enum
{
	SCHED_CANNON_CAMP = LAST_SHARED_SCHEDULE,
	SCHED_CANNON_ATTACK,
	SCHED_CANNON_DISABLEDWAIT,
	SCHED_CANNON_SNAPATTACK,
};

//=========================================================
// tasks
//=========================================================
enum
{
	TASK_CANNON_PAINT_ENEMY = LAST_SHARED_TASK,
	TASK_CANNON_PAINT_DECOY,
	TASK_CANNON_ATTACK_CURSOR,
};

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CNPC_Combine_Cannon::CNPC_Combine_Cannon( void ) : 	
	m_pBeam( NULL ),
	m_hBarrageTarget( NULL )
{ 
#ifdef _DEBUG
	m_vecPaintCursor.Init();
	m_vecPaintStart.Init();
#endif
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Combine_Cannon::QuerySeeEntity( CBaseEntity *pEntity, bool bOnlyHateOrFearIfNPC )
{
	Disposition_t disp = IRelationType(pEntity);
	if ( disp != D_HT )
	{
		// Don't bother with anything I wouldn't shoot.
		return false;
	}

	if ( !FInViewCone(pEntity) )
	{
		// Yes, this does call FInViewCone twice a frame for all entities checked for 
		// visibility, but doing this allows us to cut out a bunch of traces that would
		// be done by VerifyShot for entities that aren't even in our viewcone.
		return false;
	}

	if ( VerifyShot( pEntity ) )
		return BaseClass::QuerySeeEntity( pEntity, bOnlyHateOrFearIfNPC );

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Hide the beams
//-----------------------------------------------------------------------------
void CNPC_Combine_Cannon::LaserOff( void )
{
	if ( m_pBeam != NULL )
	{
		m_pBeam->TurnOn();
	}

	for ( int i = 0; i < NUM_ANCILLARY_BEAMS; i++ )
	{
		if ( m_pAncillaryBeams[i] == NULL )
			continue;

		m_pAncillaryBeams[i]->TurnOn();
	}

	SetNextThink( gpGlobals->curtime + 0.1f );
}


//-----------------------------------------------------------------------------
// Purpose: Switch on the laser and point it at a direction
//-----------------------------------------------------------------------------
void CNPC_Combine_Cannon::LaserOn( const Vector &vecTarget, const Vector &vecDeviance )
{
	if ( m_pBeam != NULL )
	{
		m_pBeam->TurnOff();

		// Don't aim right at the guy right now.
		Vector vecInitialAim;

		if( vecDeviance == vec3_origin )
		{
			// Start the aim where it last left off!
			vecInitialAim = m_vecPaintCursor;
		}
		else
		{
			vecInitialAim = vecTarget;
		}

		vecInitialAim.x += random->RandomFloat( -vecDeviance.x, vecDeviance.x );
		vecInitialAim.y += random->RandomFloat( -vecDeviance.y, vecDeviance.y );
		vecInitialAim.z += random->RandomFloat( -vecDeviance.z, vecDeviance.z );

		m_pBeam->SetStartPos( GetBulletOrigin() );
		m_pBeam->SetEndPos( vecInitialAim );

		m_vecPaintStart = vecInitialAim;
	}

	for ( int i = 0; i < NUM_ANCILLARY_BEAMS; i++ )
	{
		if ( m_pAncillaryBeams[i] == NULL )
			continue;

		m_pAncillaryBeams[i]->TurnOff();
	}
}

//-----------------------------------------------------------------------------
// Crikey!
//-----------------------------------------------------------------------------
float CNPC_Combine_Cannon::GetWaitTimePercentage( float flTime, bool fLinear )
{
	float flElapsedTime;
	float flTimeParameter;

	flElapsedTime = flTime - (GetWaitFinishTime() - gpGlobals->curtime);

	flTimeParameter = ( flElapsedTime / flTime );

	if( fLinear )
	{
		return flTimeParameter;
	}
	else
	{
		return (1 + sin( (M_PI * flTimeParameter) - (M_PI / 2) ) ) / 2;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Combine_Cannon::GetPaintAim( const Vector &vecStart, const Vector &vecGoal, float flParameter, Vector *pProgress )
{
	// Quaternions
	Vector vecIdealDir;
	QAngle vecIdealAngles;
	QAngle vecCurrentAngles;
	Vector vecCurrentDir;
	Vector vecBulletOrigin = GetBulletOrigin();

	// vecIdealDir is where the gun should be aimed when the painting
	// time is up. This can be approximate. This is only for drawing the
	// laser, not actually aiming the weapon. A large discrepancy will look
	// bad, though.
	vecIdealDir = vecGoal - vecBulletOrigin;
	VectorNormalize(vecIdealDir);

	// Now turn vecIdealDir into angles!
	VectorAngles( vecIdealDir, vecIdealAngles );

	// This is the vector of the beam's current aim.
	vecCurrentDir = m_vecPaintStart - vecBulletOrigin;
	VectorNormalize(vecCurrentDir);

	// Turn this to angles, too.
	VectorAngles( vecCurrentDir, vecCurrentAngles );

	Quaternion idealQuat;
	Quaternion currentQuat;
	Quaternion aimQuat;

	AngleQuaternion( vecIdealAngles, idealQuat );
	AngleQuaternion( vecCurrentAngles, currentQuat );

	QuaternionSlerp( currentQuat, idealQuat, flParameter, aimQuat );

	QuaternionAngles( aimQuat, vecCurrentAngles );

	// Rebuild the current aim vector.
	AngleVectors( vecCurrentAngles, &vecCurrentDir );

	*pProgress = vecCurrentDir;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Combine_Cannon::CreateLaser( void )
{
	if ( m_pBeam != NULL )
		return;

	m_pBeam = CBeam::BeamCreate( g_pModelNameLaser, 2.0f );

	m_pBeam->SetColor( 0, 100, 255 );

	m_pBeam->PointsInit( vec3_origin, GetBulletOrigin() );
	m_pBeam->SetBrightness( 255 );
	m_pBeam->SetNoise( 0 );
	m_pBeam->SetWidth( 1.0f );
	m_pBeam->SetEndWidth( 0 );
	m_pBeam->SetScrollRate( 0 );
	m_pBeam->SetFadeLength( 0 );
	m_pBeam->SetHaloTexture( gHaloTexture );
	m_pBeam->SetHaloScale( 16.0f );

	// Think faster while painting
	SetNextThink( gpGlobals->curtime + 0.02f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Combine_Cannon::CreateAncillaryBeams( void )
{
	for ( int i = 0; i < NUM_ANCILLARY_BEAMS; i++ )
	{
		if ( m_pAncillaryBeams[i] != NULL )
			continue;

		m_pAncillaryBeams[i] = CBeam::BeamCreate( g_pModelNameLaser, 2.0f );
		m_pAncillaryBeams[i]->SetColor( 0, 100, 255 );

		m_pAncillaryBeams[i]->PointsInit( vec3_origin, GetBulletOrigin() );
		m_pAncillaryBeams[i]->SetBrightness( 255 );
		m_pAncillaryBeams[i]->SetNoise( 0 );
		m_pAncillaryBeams[i]->SetWidth( 1.0f );
		m_pAncillaryBeams[i]->SetEndWidth( 0 );
		m_pAncillaryBeams[i]->SetScrollRate( 0 );
		m_pAncillaryBeams[i]->SetFadeLength( 0 );
		m_pAncillaryBeams[i]->SetHaloTexture( gHaloTexture );
		m_pAncillaryBeams[i]->SetHaloScale( 16.0f );
		m_pAncillaryBeams[i]->TurnOff();
	}
}

#define LINE_LENGTH 1600.0f

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flConvergencePerc - 
//			vecBasis - 
//-----------------------------------------------------------------------------
void CNPC_Combine_Cannon::UpdateAncillaryBeams( float flConvergencePerc, const Vector &vecOrigin, const Vector &vecBasis )
{
	// Multiple beams deviate from the basis direction by a certain number of degrees and "converge" 
	// at the basis vector over a duration of time, the position in that duration expressed by 
	// flConvergencePerc.  The beams are most deviated at 0 and fully converged at 1.

	float flRotationOffset = (2*M_PI)/(float)NUM_ANCILLARY_BEAMS; // Degrees separating each beam, in radians
	float flDeviation = DEG2RAD(90) * ( 1.0f - flConvergencePerc );
	float flOffset;
	Vector vecFinal;
	Vector vecOffset;
	
	matrix3x4_t matRotate;
	QAngle vecAngles;
	VectorAngles( vecBasis, vecAngles );
	vecAngles[PITCH] += 90.0f;
	AngleMatrix( vecAngles, vecOrigin, matRotate );

	trace_t	tr;

	float flScale = LINE_LENGTH * flDeviation;

	// For each beam, find its offset and trace outwards to place its endpoint
	for ( int i = 0; i < NUM_ANCILLARY_BEAMS; i++ )
	{
		if ( flConvergencePerc >= 0.99f )
		{
			m_pAncillaryBeams[i]->TurnOn();
			continue;
		}

		m_pAncillaryBeams[i]->TurnOff();

		// Find the number of radians offset we are
		flOffset = (float) i * flRotationOffset + DEG2RAD( 30.0f );
		flOffset += (M_PI/8.0f) * sin( gpGlobals->curtime * 3.0f );
		
		// Construct a circle that's also offset by the line's length
		vecOffset.x = cos( flOffset ) * flScale;
		vecOffset.y = sin( flOffset ) * flScale;
		vecOffset.z = LINE_LENGTH;

		// Rotate this whole thing into the space of the basis vector
		VectorRotate( vecOffset, matRotate, vecFinal );
		VectorNormalize( vecFinal );

		// Trace a line down that vector to find where we'll eventually stop our line
		UTIL_TraceLine( vecOrigin, vecOrigin + ( vecFinal * LINE_LENGTH ), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

		// Move the beam to that position
		m_pAncillaryBeams[i]->SetBrightness( 255.0f * flConvergencePerc );
		m_pAncillaryBeams[i]->SetEndPos( tr.startpos );
		m_pAncillaryBeams[i]->SetStartPos( tr.endpos );
	}
}

//-----------------------------------------------------------------------------
// Sweep the laser sight towards the point where the gun should be aimed
//-----------------------------------------------------------------------------
void CNPC_Combine_Cannon::PaintTarget( const Vector &vecTarget, float flPaintTime )
{
	// vecStart is the barrel of the gun (or the laser sight)
	Vector vecStart = GetBulletOrigin();

	// keep painttime from hitting 0 exactly.
	flPaintTime = MAX( flPaintTime, 0.000001f );

	// Find out where we are in the arc of the paint duration
	float flPaintPerc = GetWaitTimePercentage( flPaintTime, false );

	ScopeGlint();

	// Find out where along our line we're painting
	Vector vecCurrentDir;
	float flInterp = RemapValClamped( flPaintPerc, 0.0f, 0.5f, 0.0f, 1.0f );
	flInterp = clamp( flInterp, 0.0f, 1.0f );
	GetPaintAim( m_vecPaintStart, vecTarget, flInterp, &vecCurrentDir );

#define THRESHOLD 0.9f
	float flNoiseScale;
	
	if ( flPaintPerc >= THRESHOLD )
	{
		flNoiseScale = 1 - (1 / (1 - THRESHOLD)) * ( flPaintPerc - THRESHOLD );
	}
	else if ( flPaintPerc <= 1 - THRESHOLD )
	{
		flNoiseScale = flPaintPerc / (1 - THRESHOLD);
	}
	else
	{
		flNoiseScale = 1;
	}

	// mult by P
	vecCurrentDir.x += flNoiseScale * ( sin( 3 * M_PI * gpGlobals->curtime ) * 0.0006 );
	vecCurrentDir.y += flNoiseScale * ( sin( 2 * M_PI * gpGlobals->curtime + 0.5 * M_PI ) * 0.0006 );
	vecCurrentDir.z += flNoiseScale * ( sin( 1.5 * M_PI * gpGlobals->curtime + M_PI ) * 0.0006 );

	// Find where our center is
	trace_t tr;
	UTIL_TraceLine( vecStart, vecStart + vecCurrentDir * 8192, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	m_vecPaintCursor = tr.endpos;

	// Update our beam position
	m_pBeam->SetEndPos( tr.startpos );
	m_pBeam->SetStartPos( tr.endpos );
	m_pBeam->SetBrightness( 255.0f * flPaintPerc );
	m_pBeam->RelinkBeam();

	// Find points around that center point and make our designators converge at that point over time
	UpdateAncillaryBeams( flPaintPerc, vecStart, vecCurrentDir );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Combine_Cannon::OnScheduleChange( void )
{
	LaserOff();
	
	m_hBarrageTarget = NULL;

	BaseClass::OnScheduleChange();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Combine_Cannon::Precache( void )
{
	PrecacheModel("models/combine_soldier.mdl");
	PrecacheModel("effects/bluelaser1.vmt");	
	
	gHaloTexture = PrecacheModel("sprites/light_glow03.vmt");

	PrecacheScriptSound( "NPC_Combine_Cannon.FireBullet" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Combine_Cannon::Spawn( void )
{
	Precache();

	/// HACK:
	SetModel( "models/combine_soldier.mdl" );

	// Setup our ancillary beams but keep them hidden for now
	CreateLaser();
	CreateAncillaryBeams();

	m_iAmmoType = GetAmmoDef()->Index( "CombineHeavyCannon" );

	SetHullType( HULL_HUMAN );
	SetHullSizeNormal();

	UTIL_SetSize( this, Vector( -16, -16 , 0 ), Vector( 16, 16, 64 ) );

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_FLY );
	m_bloodColor		= DONT_BLEED;
	m_iHealth			= 10;
	m_flFieldOfView		= DOT_45DEGREE;
	m_NPCState			= NPC_STATE_NONE;

	if( HasSpawnFlags( SF_STARTDISABLED ) )
	{
		m_fEnabled = false;
	}
	else
	{
		m_fEnabled = true;
	}

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_SIMPLE_RADIUS_DAMAGE );

	m_HackedGunPos = Vector ( 0, 0, 0 ); 

	AddSpawnFlags( SF_NPC_LONG_RANGE | SF_NPC_ALWAYSTHINK );

	NPCInit();

	// Limit our look distance
	SetDistLook( m_flSightDist );

	AddEffects( EF_NODRAW );
	AddSolidFlags( FSOLID_NOT_SOLID );

	// Point the cursor straight ahead so that the sniper's
	// first sweep of the laser doesn't look weird.
	Vector vecForward;
	AngleVectors( GetLocalAngles(), &vecForward );
	m_vecPaintCursor = GetBulletOrigin() + vecForward * 1024;

	// none!
	GetEnemies()->SetFreeKnowledgeDuration( 0.0f );
	GetEnemies()->SetEnemyDiscardTime( 2.0f );

	m_flTimeLastAttackedPlayer = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Class_T	CNPC_Combine_Cannon::Classify( void )
{
	if ( m_fEnabled )
		return	CLASS_COMBINE;

	return CLASS_NONE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CNPC_Combine_Cannon::GetBulletOrigin( void )
{
	return GetAbsOrigin();
}


//-----------------------------------------------------------------------------
// Purpose: Nothing kills the cannon but entity I/O
//-----------------------------------------------------------------------------
int CNPC_Combine_Cannon::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// We are invulnerable to normal attacks for the moment
	return 0;
}

//---------------------------------------------------------
// Purpose: 
//---------------------------------------------------------
void CNPC_Combine_Cannon::UpdateOnRemove( void )
{
	// Remove the main laser
	if ( m_pBeam != NULL )
	{
		UTIL_Remove( m_pBeam);
		m_pBeam = NULL;
	}

	// Remove our ancillary beams
	for ( int i = 0; i < NUM_ANCILLARY_BEAMS; i++ )
	{
		if ( m_pAncillaryBeams[i] == NULL )
			continue;

		UTIL_Remove( m_pAncillaryBeams[i] );
		m_pAncillaryBeams[i] = NULL;
	}

	BaseClass::UpdateOnRemove();
}

//---------------------------------------------------------
// Purpose: 
//---------------------------------------------------------
int CNPC_Combine_Cannon::SelectSchedule ( void )
{
	// Fire at our target
	if( GetEnemy() && HasCondition( COND_CAN_RANGE_ATTACK1 ) )
		return SCHED_RANGE_ATTACK1;

	// Wait for a target
	// TODO: Sweep like a sniper?
	return SCHED_COMBAT_STAND;
}

//---------------------------------------------------------
// Purpose: 
//---------------------------------------------------------
bool CNPC_Combine_Cannon::FCanCheckAttacks ( void )
{
	return true;
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Combine_Cannon::VerifyShot( CBaseEntity *pTarget )
{
	trace_t tr;

	Vector vecTarget = DesiredBodyTarget( pTarget );
	UTIL_TraceLine( GetBulletOrigin(), vecTarget, MASK_SHOT, pTarget, COLLISION_GROUP_NONE, &tr );

	if( tr.fraction != 1.0 )
	{
		if( pTarget->IsPlayer() )
		{
			// if the target is the player, do another trace to see if we can shoot his eyeposition. This should help 
			// improve sniper responsiveness in cases where the player is hiding his chest from the sniper with his 
			// head in full view.
			UTIL_TraceLine( GetBulletOrigin(), pTarget->EyePosition(), MASK_SHOT, pTarget, COLLISION_GROUP_NONE, &tr );

			if( tr.fraction == 1.0 )
			{
				return true;
			}
		}

		// Trace hit something.
		if( tr.m_pEnt )
		{
			if( tr.m_pEnt->m_takedamage == DAMAGE_YES )
			{
				// Just shoot it if I can hurt it. Probably a breakable or glass pane.
				return true;
			}
		}

		return false;
	}
	else
	{
		return true;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Combine_Cannon::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( GetNextAttack() > gpGlobals->curtime )
		return COND_NONE;

	if ( HasCondition( COND_SEE_ENEMY ) && !HasCondition( COND_ENEMY_OCCLUDED ) )
	{
		if ( VerifyShot( GetEnemy() ) )
		{
			// Can see the enemy, have a clear shot to his midsection
			ClearCondition( COND_CANNON_NO_SHOT );
			return COND_CAN_RANGE_ATTACK1;
		}
		else
		{
			// Can see the enemy, but can't take a shot at his midsection
			SetCondition( COND_CANNON_NO_SHOT );
			return COND_NONE;
		}
	}

	return COND_NONE;
}


//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Combine_Cannon::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
	case SCHED_RANGE_ATTACK1:
		return SCHED_CANNON_ATTACK;
		break;
	}
	return BaseClass::TranslateSchedule( scheduleType );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Combine_Cannon::ScopeGlint( void )
{
	CEffectData data;

	data.m_vOrigin = GetAbsOrigin();
	data.m_vNormal = vec3_origin;
	data.m_vAngles = vec3_angle;
	data.m_nColor = COMMAND_POINT_BLUE;

	DispatchEffect( "CommandPointer", data );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *vecIn - 
//-----------------------------------------------------------------------------
void CNPC_Combine_Cannon::AdjustShotPosition( CBaseEntity *pTarget, Vector *vecIn )
{
	if ( pTarget == NULL || vecIn == NULL )
		return;

	Vector low = pTarget->WorldSpaceCenter() - ( pTarget->WorldSpaceCenter() - pTarget->GetAbsOrigin() ) * .25;
	Vector high = pTarget->EyePosition();
	Vector delta = high - low;
	Vector result = low + delta * 0.5; 

	// Only take the height
	(*vecIn)[2] = result[2];
}

//---------------------------------------------------------
// This starts the bullet state machine.  The actual effects
// of the bullet will happen later.  This function schedules 
// those effects.
//
// fDirectShot indicates whether the bullet is a "direct shot"
// that is - fired with the intent that it will strike the
// enemy. Otherwise, the bullet is intended to strike a 
// decoy object or nothing at all in particular.
//---------------------------------------------------------
bool CNPC_Combine_Cannon::FireBullet( const Vector &vecTarget, bool bDirectShot )
{
	Vector vecBulletOrigin = GetBulletOrigin();
	Vector vecDir = ( vecTarget - vecBulletOrigin );
	VectorNormalize( vecDir );

	FireBulletsInfo_t info;
	info.m_iShots = 1;
	info.m_iTracerFreq = 1.0f;
	info.m_vecDirShooting = vecDir;
	info.m_vecSrc = vecBulletOrigin;
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_pAttacker = this;
	info.m_iAmmoType = m_iAmmoType;
	info.m_iPlayerDamage = 20.0f;
	info.m_vecSpread = Vector( 0.015f, 0.015f, 0.015f );  // medium cone

	FireBullets( info );

	EmitSound( "NPC_Combine_Cannon.FireBullet" );

	// Don't attack for a certain amount of time
	SetNextAttack( gpGlobals->curtime + GetRefireTime() );

	// Sniper had to be aiming here to fire here, so make it the cursor
	m_vecPaintCursor = vecTarget;

	LaserOff();

	return true;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Combine_Cannon::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_CANNON_ATTACK_CURSOR:
		break;

	case TASK_RANGE_ATTACK1:
		// Setup the information for this barrage
		m_flBarrageDuration = gpGlobals->curtime + random->RandomFloat( 0.25f, 0.5f );
		m_hBarrageTarget = GetEnemy();
		break;

	case TASK_CANNON_PAINT_ENEMY:
		{
			if ( GetEnemy()->IsPlayer() )
			{
				float delay = random->RandomFloat( 0.0f, 0.3f );
			
				if ( ( gpGlobals->curtime - m_flTimeLastAttackedPlayer ) < 1.0f )
				{
					SetWait( CANNON_SUBSEQUENT_PAINT_TIME );
					m_flPaintTime = CANNON_SUBSEQUENT_PAINT_TIME;
				}
				else
				{
					SetWait( CANNON_PAINT_ENEMY_TIME + delay );
					m_flPaintTime = CANNON_PAINT_ENEMY_TIME + delay;
				}
			}
			else
			{
				// Use a random time
				m_flPaintTime = CANNON_PAINT_ENEMY_TIME + random->RandomFloat( 0, CANNON_PAINT_NPC_TIME_NOISE );
				SetWait( m_flPaintTime );
			}

			// Try to start the laser where the player can't miss seeing it!
			Vector vecCursor;
			AngleVectors( GetEnemy()->GetLocalAngles(), &vecCursor );
			vecCursor *= 300;
			vecCursor += GetEnemy()->EyePosition();				
			LaserOn( vecCursor, Vector( 16, 16, 16 ) );
		}
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Combine_Cannon::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_CANNON_ATTACK_CURSOR:
		if( FireBullet( m_vecPaintCursor, true ) )
		{
			TaskComplete();
		}
		break;

	case TASK_RANGE_ATTACK1:
		{
			// Where we're focusing our fire
			Vector vecTarget = ( m_hBarrageTarget == NULL ) ? m_vecPaintCursor : LeadTarget( m_hBarrageTarget );

			// Fire at enemy
			if ( FireBullet( vecTarget, true ) )
			{
				bool bPlayerIsEnemy = ( m_hBarrageTarget && m_hBarrageTarget->IsPlayer() );
				bool bBarrageFinished = m_flBarrageDuration < gpGlobals->curtime;
				bool bNoShot = ( QuerySeeEntity( m_hBarrageTarget ) == false );	// FIXME: Store this info off better
				bool bSeePlayer = HasCondition( COND_SEE_PLAYER	);
				
				// Treat the player differently to normal NPCs
				if ( bPlayerIsEnemy )
				{
					// Store the last time we shot for doing an abbreviated attack telegraph
					m_flTimeLastAttackedPlayer = gpGlobals->curtime;

					// If we've got no shot and we're done with our current barrage
					if ( bNoShot && bBarrageFinished )
					{
						TaskComplete();
					}
				}
				else if ( bBarrageFinished || bSeePlayer )
				{
					// Done with the barrage or we saw the player as a better target
					TaskComplete();
				}
			}
		}
		break;

	case TASK_CANNON_PAINT_ENEMY:
		{
			// See if we're done painting our target
			if ( IsWaitFinished() )
			{			
				TaskComplete();
			}

			// Continue to paint the target
			PaintTarget( LeadTarget( GetEnemy() ), m_flPaintTime );
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
// The sniper throws away the circular list of old decoys when we restore.
//-----------------------------------------------------------------------------
int CNPC_Combine_Cannon::Restore( IRestore &restore )
{
	return BaseClass::Restore( restore );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
float CNPC_Combine_Cannon::MaxYawSpeed( void )
{
	return 60;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Combine_Cannon::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
	// NOTE: We'll deal with this on the client
	// Think faster if the beam is on, this gives the beam higher resolution.
	if( m_pBeam )
	{
		SetNextThink( gpGlobals->curtime + 0.03 );
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
	}

	// If the enemy has just stepped into view, or we've acquired a new enemy,
	// Record the last time we've seen the enemy as right now.
	//
	// If the enemy has been out of sight for a full second, mark him eluded.
	if( GetEnemy() != NULL )
	{
		if( gpGlobals->curtime - GetEnemies()->LastTimeSeen( GetEnemy() ) > 30 )
		{
			// Stop pestering enemies after 30 seconds of frustration.
			GetEnemies()->ClearMemory( GetEnemy() );
			SetEnemy(NULL);
		}
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
Vector CNPC_Combine_Cannon::EyePosition( void )
{
	return GetAbsOrigin();
}

//---------------------------------------------------------
//---------------------------------------------------------
Vector CNPC_Combine_Cannon::DesiredBodyTarget( CBaseEntity *pTarget )
{
	// By default, aim for the center
	Vector vecTarget = pTarget->WorldSpaceCenter();

	float flTimeSinceLastMiss = gpGlobals->curtime - m_flTimeLastShotMissed;

	if( pTarget->GetFlags() & FL_CLIENT )
	{
		if( !BaseClass::FVisible( vecTarget ) )
		{
			// go to the player's eyes if his center is concealed.
			// Bump up an inch so the player's not looking straight down a beam.
			vecTarget = pTarget->EyePosition() + Vector( 0, 0, 1 );
		}
	}
	else
	{
		if( pTarget->Classify() == CLASS_HEADCRAB )
		{
			// Headcrabs are tiny inside their boxes.
			vecTarget = pTarget->GetAbsOrigin();
			vecTarget.z += 4.0;
		}
		else if( pTarget->Classify() == CLASS_ZOMBIE )
		{
			if( flTimeSinceLastMiss > 0.0f && flTimeSinceLastMiss < 4.0f && hl2_episodic.GetBool() )
			{
				vecTarget = pTarget->BodyTarget( GetBulletOrigin(), false );
			}
			else
			{
				// Shoot zombies in the headcrab
				vecTarget = pTarget->HeadTarget( GetBulletOrigin() );
			}
		}
		else if( pTarget->Classify() == CLASS_ANTLION )
		{
			// Shoot about a few inches above the origin. This makes it easy to hit antlions
			// even if they are on their backs.
			vecTarget = pTarget->GetAbsOrigin();
			vecTarget.z += 18.0f;
		}
		else if( pTarget->Classify() == CLASS_EARTH_FAUNA )
		{
			// Shoot birds in the center
		}
		else
		{
			// Shoot NPCs in the chest
			vecTarget.z += 8.0f;
		}
	}

	return vecTarget;
}

//---------------------------------------------------------
//---------------------------------------------------------
Vector CNPC_Combine_Cannon::LeadTarget( CBaseEntity *pTarget )
{
	if ( pTarget != NULL )
	{
		Vector vecFuturePos;
		UTIL_PredictedPosition( pTarget, 0.05f, &vecFuturePos );
		AdjustShotPosition(	pTarget, &vecFuturePos );

		return vecFuturePos;
	}

	return vec3_origin;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Combine_Cannon::InputEnableSniper( inputdata_t &inputdata )
{
	ClearCondition( COND_CANNON_DISABLED );
	SetCondition( COND_CANNON_ENABLED );

	m_fEnabled = true;
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Combine_Cannon::InputDisableSniper( inputdata_t &inputdata )
{
	ClearCondition( COND_CANNON_ENABLED );
	SetCondition( COND_CANNON_DISABLED );

	m_fEnabled = false;
}

//---------------------------------------------------------
// See all NPC's easily.
//
// Only see the player if you can trace to both of his 
// eyeballs. That is, allow the player to peek around corners.
// This is a little more expensive than the base class' check!
//---------------------------------------------------------
#define CANNON_EYE_DIST 0.75
#define CANNON_TARGET_VERTICAL_OFFSET Vector( 0, 0, 5 );
bool CNPC_Combine_Cannon::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	// NPC
	if ( pEntity->IsPlayer() == false )
		return BaseClass::FVisible( pEntity, traceMask, ppBlocker );

	if ( pEntity->GetFlags() & FL_NOTARGET )
		return false;

	Vector	vecVerticalOffset;
	Vector	vecRight;
	Vector	vecEye;
	trace_t	tr;

	if( fabs( GetAbsOrigin().z - pEntity->WorldSpaceCenter().z ) <= 120.f )
	{
		// If the player is around the same elevation, look straight at his eyes. 
		// At the same elevation, the vertical peeking allowance makes it too easy
		// for a player to dispatch the sniper from cover.
		vecVerticalOffset = vec3_origin;
	}
	else
	{
		// Otherwise, look at a spot below his eyes. This allows the player to back away
		// from his cover a bit and have a peek at the sniper without being detected.
		vecVerticalOffset = CANNON_TARGET_VERTICAL_OFFSET;
	}

	AngleVectors( pEntity->GetLocalAngles(), NULL, &vecRight, NULL );

	vecEye = vecRight * CANNON_EYE_DIST - vecVerticalOffset;
	UTIL_TraceLine( EyePosition(), pEntity->EyePosition() + vecEye, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

#if 0
	NDebugOverlay::Line(EyePosition(), tr.endpos, 0,255,0, true, 0.1);
#endif

	bool fCheckFailed = false;

	if ( tr.fraction != 1.0 && tr.m_pEnt != pEntity )
	{
		fCheckFailed = true;
	}

	// Don't check the other eye if the first eye failed.
	if( !fCheckFailed )
	{
		vecEye = -vecRight * CANNON_EYE_DIST - vecVerticalOffset;
		UTIL_TraceLine( EyePosition(), pEntity->EyePosition() + vecEye, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

#if 0
		NDebugOverlay::Line(EyePosition(), tr.endpos, 0,255,0, true, 0.1);
#endif 

		if ( tr.fraction != 1.0 && tr.m_pEnt != pEntity )
		{
			fCheckFailed = true;
		}
	}

	if( !fCheckFailed )
	{
		// Can see the player.
		return true;
	}
	
	// Now, if the check failed, see if the player is ducking and has recently
	// fired a muzzleflash. If yes, see if you'd be able to see the player if 
	// they were standing in their current position instead of ducking. Since
	// the sniper doesn't have a clear shot in this situation, he will harrass
	// near the player.
	CBasePlayer *pPlayer;

	pPlayer = ToBasePlayer( pEntity );

	if( (pPlayer->GetFlags() & FL_DUCKING) && pPlayer->MuzzleFlashTime() > gpGlobals->curtime )
	{
		vecEye = pPlayer->EyePosition() + Vector( 0, 0, 32 );
		UTIL_TraceLine( EyePosition(), vecEye, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

		if( tr.fraction != 1.0 )
		{
			// Everything failed.
			if (ppBlocker)
			{
				*ppBlocker = tr.m_pEnt;
			}
			return false;
		}
		else
		{
			// Fake being able to see the player.
			return true;
		}
	}

	if (ppBlocker)
	{
		*ppBlocker = tr.m_pEnt;
	}

	return false;
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_combine_cannon, CNPC_Combine_Cannon )

	DECLARE_CONDITION( COND_CANNON_ENABLED );
	DECLARE_CONDITION( COND_CANNON_DISABLED );
	DECLARE_CONDITION( COND_CANNON_NO_SHOT );	

	DECLARE_TASK( TASK_CANNON_PAINT_ENEMY );
	DECLARE_TASK( TASK_CANNON_ATTACK_CURSOR );

	//=========================================================
	// CAMP
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CANNON_CAMP,

		"	Tasks"
		"		TASK_WAIT		1"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_HEAR_DANGER"
		"		COND_CANNON_DISABLED"
	)

	//=========================================================
	// ATTACK
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CANNON_ATTACK,

		"	Tasks"
		"		TASK_CANNON_PAINT_ENEMY		0"
		"		TASK_RANGE_ATTACK1			0"
		"	"
		"	Interrupts"
		"		COND_HEAR_DANGER"
		"		COND_CANNON_DISABLED"
	)
		
	//=========================================================
	// ATTACK
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CANNON_SNAPATTACK,

		"	Tasks"
		"		TASK_CANNON_ATTACK_CURSOR	0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_OCCLUDED"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_HEAR_DANGER"
		"		COND_CANNON_DISABLED"
	)

	//=========================================================
	// Sniper is allowed to process a couple conditions while
	// disabled, but mostly he waits until he's enabled.
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CANNON_DISABLEDWAIT,

		"	Tasks"
		"		TASK_WAIT			0.5"
		"	"
		"	Interrupts"
		"		COND_CANNON_ENABLED"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
	)

AI_END_CUSTOM_NPC()
