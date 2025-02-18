//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_SCRIPTCONDITIONS_H
#define AI_SCRIPTCONDITIONS_H

#include "baseentity.h"
#include "entityoutput.h"
#include "simtimer.h"
#include "ai_npcstate.h"

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------

class CAI_ProxTester
{
public:
	CAI_ProxTester()
		: m_distSq( 0 ),
		m_fInside( false )
	{
	}

	void Init( float dist )
	{
		m_fInside = ( dist > 0 );
		m_distSq = dist * dist;
	}

	bool Check( CBaseEntity *pEntity1, CBaseEntity *pEntity2 )
	{
		if ( m_distSq != 0 )
		{
			float distSq = ( pEntity1->GetAbsOrigin() - pEntity2->GetAbsOrigin() ).LengthSqr();
			bool fInside = ( distSq < m_distSq );

			return ( m_fInside == fInside );
		}
		return true;
	}

	DECLARE_SIMPLE_DATADESC();

private:

	float m_distSq;
	bool  m_fInside;
};

//-----------------------------------------------------------------------------
class CAI_ScriptConditionsElement
{
public:

	DECLARE_SIMPLE_DATADESC();

	void			SetActor( CBaseEntity *pEntity ) { m_hActor = pEntity; }
	CBaseEntity		*GetActor( void ){ return m_hActor.Get(); }

	void			SetTimer( CSimTimer timer ) { m_Timer = timer;	}
	CSimTimer		*GetTimer( void ) { return &m_Timer;	}
	
	void			SetTimeOut( CSimTimer timeout) { m_Timeout = timeout;	}
	CSimTimer		*GetTimeOut( void ) { return &m_Timeout;	}

private:
	EHANDLE			m_hActor;
	CSimTimer		m_Timer;
	CSimTimer		m_Timeout;
};

//-----------------------------------------------------------------------------
// class CAI_ScriptConditions
//
// Purpose: Watches a set of conditions relative to a given NPC, and when they
//			are all satisfied, fires the relevant output
//-----------------------------------------------------------------------------

class CAI_ScriptConditions : public CBaseEntity, public IEntityListener
{
	DECLARE_CLASS( CAI_ScriptConditions, CBaseEntity );

public:
	CAI_ScriptConditions()
		:	m_fDisabled( true ),
		m_flRequiredTime( 0 ),
		m_fMinState( NPC_STATE_IDLE ),
		m_fMaxState( NPC_STATE_IDLE ),
		m_fScriptStatus( TRS_NONE ),
		m_fActorSeePlayer( TRS_NONE ),
		m_flPlayerActorProximity( 0 ),
		m_flPlayerActorFOV( -1 ),
		m_fPlayerActorLOS( TRS_NONE ),
		m_fActorSeeTarget( TRS_NONE ),
		m_flActorTargetProximity( 0 ),
		m_flPlayerTargetProximity( 0 ),
		m_flPlayerTargetFOV( 0 ),
		m_fPlayerTargetLOS( TRS_NONE ),
		m_fPlayerBlockingActor( TRS_NONE ),
		m_flMinTimeout( 0 ),
		m_flMaxTimeout( 0 ),
		m_fActorInPVS( TRS_NONE ),
		m_fActorInVehicle( TRS_NONE ),
		m_fPlayerInVehicle( TRS_NONE )
	{
#ifndef HL2_EPISODIC
		m_hActor = NULL;
#endif
	}

private:
	void Spawn();
	void Activate();

	void EvaluationThink();

	void Enable();
	void Disable();

	void SetThinkTime()			{ SetNextThink( gpGlobals->curtime + 0.250 ); }

	// Evaluators
	struct EvalArgs_t
	{
		CBaseEntity *pActor; 
		CBasePlayer *pPlayer; 
		CBaseEntity *pTarget;
	};

	bool EvalState( const EvalArgs_t &args );
	bool EvalActorSeePlayer( const EvalArgs_t &args );
	bool EvalPlayerActorLook( const EvalArgs_t &args );
	bool EvalPlayerTargetLook( const EvalArgs_t &args );
	bool EvalPlayerActorProximity( const EvalArgs_t &args );
	bool EvalPlayerTargetProximity( const EvalArgs_t &args );
	bool EvalActorTargetProximity( const EvalArgs_t &args );
	bool EvalActorSeeTarget( const EvalArgs_t &args );
	bool EvalPlayerActorLOS( const EvalArgs_t &args );
	bool EvalPlayerTargetLOS( const EvalArgs_t &args );
	bool EvalPlayerBlockingActor( const EvalArgs_t &args );
	bool EvalActorInPVS( const EvalArgs_t &args );
	bool EvalPlayerInVehicle( const EvalArgs_t &args );
	bool EvalActorInVehicle( const EvalArgs_t &args );

	void OnEntitySpawned( CBaseEntity *pEntity );

	int AddNewElement( CBaseEntity *pActor );

	bool ActorInList( CBaseEntity *pActor );
	void UpdateOnRemove( void );

	// Input handlers
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	// Output handlers
	COutputEvent	m_OnConditionsSatisfied;
	COutputEvent	m_OnConditionsTimeout;
	COutputEvent	m_NoValidActors;

	//---------------------------------

#ifndef HL2_EPISODIC
	CBaseEntity *GetActor()		{ return m_hActor.Get();			}
#endif
	CBasePlayer *GetPlayer()	{ return UTIL_GetLocalPlayer();	}

	//---------------------------------

	// @Note (toml 07-17-02): At some point, it may be desireable to switch to using function objects instead of functions. Probably
	// if support for NPCs addiing custom conditions becomes necessary
	typedef bool (CAI_ScriptConditions::*EvaluationFunc_t)( const EvalArgs_t &args );

	struct EvaluatorInfo_t
	{
		EvaluationFunc_t	pfnEvaluator;
		const char			*pszName;
	};

	static EvaluatorInfo_t gm_Evaluators[];

	//---------------------------------
	// Evaluation helpers

	static bool IsInFOV( CBaseEntity *pViewer, CBaseEntity *pViewed, float fov, bool bTrueCone );
	static bool PlayerHasLineOfSight( CBaseEntity *pViewer, CBaseEntity *pViewed, bool fNot );
	static bool ActorInPlayersPVS( CBaseEntity *pActor, bool bNot );

	virtual void OnRestore( void );

	//---------------------------------
	// General conditions info

	bool			m_fDisabled;
	bool			m_bLeaveAsleep;
	EHANDLE			m_hTarget;

	float			m_flRequiredTime;	// How long should the conditions me true

#ifndef HL2_EPISODIC
	EHANDLE 		m_hActor;
	CSimTimer		m_Timer; 			// @TODO (toml 07-16-02): save/load of timer once Jay has save/load of contained objects
	CSimTimer		m_Timeout;
#endif

	//---------------------------------
	// Specific conditions data
	NPC_STATE		m_fMinState;
	NPC_STATE		m_fMaxState;
	ThreeState_t 	m_fScriptStatus;
	ThreeState_t 	m_fActorSeePlayer;
	string_t		m_Actor;

	float 			m_flPlayerActorProximity;
	CAI_ProxTester	m_PlayerActorProxTester;

	float			m_flPlayerActorFOV;
	bool			m_bPlayerActorFOVTrueCone;
	ThreeState_t	m_fPlayerActorLOS;
	ThreeState_t 	m_fActorSeeTarget;

	float 			m_flActorTargetProximity;
	CAI_ProxTester	m_ActorTargetProxTester;

	float 			m_flPlayerTargetProximity;
	CAI_ProxTester	m_PlayerTargetProxTester;

	float 			m_flPlayerTargetFOV;
	bool			m_bPlayerTargetFOVTrueCone;
	ThreeState_t	m_fPlayerTargetLOS;
	ThreeState_t	m_fPlayerBlockingActor;
	ThreeState_t	m_fActorInPVS;

	float			m_flMinTimeout;
	float			m_flMaxTimeout;

	ThreeState_t	m_fActorInVehicle;
	ThreeState_t	m_fPlayerInVehicle;

	CUtlVector< CAI_ScriptConditionsElement > m_ElementList;

	//---------------------------------

	DECLARE_DATADESC();
};

//=============================================================================

#endif // AI_SCRIPTCONDITIONS_H
