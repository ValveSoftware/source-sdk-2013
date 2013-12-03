//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef NPC_TURRET_GROUND_H
#define NPC_TURRET_GROUND_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "smoke_trail.h"

//=========================================================
//=========================================================
class CNPC_GroundTurret : public CAI_BaseNPC
{
public:
	DECLARE_CLASS( CNPC_GroundTurret, CAI_BaseNPC );
	DECLARE_DATADESC();

	void			Precache( void );
	virtual void	Spawn( void );
	bool			CreateVPhysics( void );
	void			PrescheduleThink();
	Class_T			Classify( void );

	void PostNPCInit();

	// Damage & Death
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void Event_Killed( const CTakeDamageInfo &info );
	void DeathEffects();
	bool CanBecomeRagdoll( void ) { return false; }
	void DeathSound( const CTakeDamageInfo &info );

	// Combat
	void MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
	Vector GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget )
	{
		return VECTOR_CONE_5DEGREES;
	}


	// Sensing 
	void GatherConditions();
	Vector EyePosition();
	bool FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker );
	bool QuerySeeEntity( CBaseEntity *pEntity, bool bOnlyHateOrFearIfNPC = false );


	bool IsOpeningOrClosing() { return (GetAbsVelocity().z != 0.0f); }
	bool IsEnabled();
	bool IsOpen();

	// Tasks & Schedules
	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );
	virtual int SelectSchedule( void );
	virtual int TranslateSchedule( int scheduleType );

	// Activities & Animation
	Activity			NPC_TranslateActivity( Activity eNewActivity );
	virtual void		Shoot();
	virtual void		Scan();
	void				ProjectBeam( const Vector &vecStart, const Vector &vecDir, int width, int brightness, float duration );

	// Local
	void SetActive( bool bActive ) {}

	// Inputs
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	// Outputs
	COutputEvent	m_OnAreaClear;

	DEFINE_CUSTOM_AI;

protected:
	//-----------------------------------------------------
	// Conditions, Schedules, Tasks
	//-----------------------------------------------------
	enum 
	{
		SCHED_GROUND_TURRET_IDLE = BaseClass::NEXT_SCHEDULE,
		SCHED_GROUND_TURRET_ATTACK,

		TASK_GROUNDTURRET_SCAN = BaseClass::NEXT_TASK,
	};

	int			m_iAmmoType;
	SmokeTrail	*m_pSmoke;

	bool		m_bEnabled;

	float		m_flTimeNextShoot;
	float		m_flTimeLastSawEnemy;
	Vector		m_vecSpread;
	bool		m_bHasExploded;
	int			m_iDeathSparks;
	float		m_flSensingDist;
	bool		m_bSeeEnemy;
	float		m_flTimeNextPing;

	Vector		m_vecClosedPos;

	Vector		m_vecLightOffset;

	HSOUNDSCRIPTHANDLE 	m_ShotSounds;
};

#endif //#ifndef NPC_TURRET_GROUND_H
