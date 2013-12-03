//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is the base version of the vortigaunt
//
//=============================================================================//

#ifndef NPC_VORTIGAUNT_H
#define NPC_VORTIGAUNT_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "ai_behavior.h"
#include "ai_behavior_lead.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_assault.h"
#include "npc_playercompanion.h"

#define		VORTIGAUNT_MAX_BEAMS				8

#define VORTIGAUNT_BEAM_ALL		-1
#define	VORTIGAUNT_BEAM_ZAP		0
#define	VORTIGAUNT_BEAM_HEAL	1
#define VORTIGAUNT_BEAM_DISPEL	2

class CBeam;
class CSprite;
class CVortigauntChargeToken;
class CVortigauntEffectDispel;

extern ConVar sk_vortigaunt_zap_range;

enum VortigauntHealState_t
{
	HEAL_STATE_NONE,		// Not trying to heal
	HEAL_STATE_WARMUP,		// In the "warm-up" phase of healing
	HEAL_STATE_HEALING,		// In the process of healing
	HEAL_STATE_COOLDOWN,	// in the "cooldown" phase of healing
};

//=========================================================
//	>> CNPC_Vortigaunt
//=========================================================
class CNPC_Vortigaunt : public CNPC_PlayerCompanion
{
	DECLARE_CLASS( CNPC_Vortigaunt, CNPC_PlayerCompanion );

public:
					CNPC_Vortigaunt( void );

	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual float	MaxYawSpeed( void );

	virtual	Vector  FacingPosition( void );
	virtual Vector	BodyTarget( const Vector &posSrc, bool bNoisy = true );

	virtual void	PrescheduleThink( void );
	virtual void	BuildScheduleTestBits( void );
	virtual void	OnScheduleChange( void );

	virtual int		RangeAttack1Conditions( float flDot, float flDist );	// Primary zap
	virtual int		RangeAttack2Conditions( float flDot, float flDist );	// Concussive zap (larger)
	virtual bool	InnateWeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions );
	virtual int		MeleeAttack1Conditions( float flDot, float flDist );	// Dispel
	virtual float	InnateRange1MinRange( void ) { return 0.0f; }
	virtual float	InnateRange1MaxRange( void ) { return sk_vortigaunt_zap_range.GetFloat()*12; }
	virtual int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual bool	FInViewCone( CBaseEntity *pEntity );
	virtual bool	ShouldMoveAndShoot( void );

	// vorts have a very long head/neck swing, so debounce heavily
	virtual	float	GetHeadDebounce( void ) { return 0.7; } // how much of previous head turn to use

	virtual void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void		AlertSound( void );
	virtual Class_T		Classify ( void ) { return IsGameEndAlly() ? CLASS_PLAYER_ALLY_VITAL : CLASS_VORTIGAUNT; }
	virtual void		HandleAnimEvent( animevent_t *pEvent );
	virtual Activity	NPC_TranslateActivity( Activity eNewActivity );

	virtual void	UpdateOnRemove( void );
	virtual void	Event_Killed( const CTakeDamageInfo &info );
	virtual	void	GatherConditions( void );
	virtual void	RunTask( const Task_t *pTask );
	virtual void	StartTask( const Task_t *pTask );
	virtual void	ClearSchedule( const char *szReason );

	virtual void	DeclineFollowing( void );
	virtual bool	CanBeUsedAsAFriend( void );
	virtual bool	IsPlayerAlly( void ) { return true; }

	// Override these to set behavior
	virtual int		TranslateSchedule( int scheduleType );
	virtual int		SelectSchedule( void );
	virtual int		SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	virtual bool	IsValidEnemy( CBaseEntity *pEnemy );
	bool			IsLeading( void ) { return ( GetRunningBehavior() == &m_LeadBehavior && m_LeadBehavior.HasGoal() ); }

	void			DeathSound( const CTakeDamageInfo &info );
	void			PainSound( const CTakeDamageInfo &info );
	
	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	virtual void	SpeakSentence( int sentType );

	virtual int		IRelationPriority( CBaseEntity *pTarget );
	virtual Disposition_t 	IRelationType( CBaseEntity *pTarget );
	virtual bool	IsReadinessCapable( void ) { return true; }
	virtual float	GetReadinessDecay() { return 30.0f; }
	virtual bool	ShouldRegenerateHealth( void ) { return m_bRegenerateHealth; }
	virtual bool	CanRunAScriptedNPCInteraction( bool bForced = false );
	virtual void	AimGun( void );
	virtual void	OnUpdateShotRegulator( void );

	void	InputEnableArmorRecharge( inputdata_t &data );
	void	InputDisableArmorRecharge( inputdata_t &data );
	void	InputExtractBugbait( inputdata_t &data );
	void	InputChargeTarget( inputdata_t &data );
	void	InputDispel( inputdata_t &data );
	void	InputBeginCarryNPC( inputdata_t &indputdata );
	void	InputEndCarryNPC( inputdata_t &indputdata );

	// Health regeneration
	void	InputEnableHealthRegeneration( inputdata_t &data );
	void	InputDisableHealthRegeneration( inputdata_t &data );

	// color
	void	InputTurnBlue( inputdata_t &data );
	void	InputTurnBlack( inputdata_t &data );

	virtual void	SetScriptedScheduleIgnoreConditions( Interruptability_t interrupt );
	virtual void    OnRestore( void );
	virtual bool	OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );
	virtual void	OnStartScene( void );
	virtual bool	IsInterruptable( void );
	virtual bool	CanFlinch( void );

	// used so a grub can notify me that I stepped on it. Says a line.
	void	OnSquishedGrub( const CBaseEntity *pGrub );

private:

	int		NumAntlionsInRadius( float flRadius );
	void	DispelAntlions( const Vector &vecOrigin, float flRadius, bool bDispel = true );
	bool	HealGestureHasLOS( void );
	bool	PlayerBelowHealthPercentage( CBasePlayer *pPlayer, float flPerc );
	void	StartHealing( void );
	void	StopHealing( bool bInterrupt = false );
	void	MaintainHealSchedule( void );
	bool	ShouldHealTarget( CBaseEntity *pTarget );
	int		SelectHealSchedule( void );

	void	CreateBeamBlast( const Vector &vecOrigin );

private:
	//=========================================================
	// Vortigaunt schedules
	//=========================================================
	enum
	{
		SCHED_VORTIGAUNT_STAND = BaseClass::NEXT_SCHEDULE,
		SCHED_VORTIGAUNT_RANGE_ATTACK,
		SCHED_VORTIGAUNT_HEAL,
		SCHED_VORTIGAUNT_EXTRACT_BUGBAIT,
		SCHED_VORTIGAUNT_FACE_PLAYER,
		SCHED_VORTIGAUNT_RUN_TO_PLAYER,
		SCHED_VORTIGAUNT_DISPEL_ANTLIONS,
		SCHED_VORT_FLEE_FROM_BEST_SOUND,
		SCHED_VORT_ALERT_FACE_BESTSOUND,
	};

	//=========================================================
	// Vortigaunt Tasks 
	//=========================================================
	enum 
	{
		TASK_VORTIGAUNT_HEAL_WARMUP = BaseClass::NEXT_TASK,
		TASK_VORTIGAUNT_HEAL,
		TASK_VORTIGAUNT_EXTRACT_WARMUP,
		TASK_VORTIGAUNT_EXTRACT,
		TASK_VORTIGAUNT_EXTRACT_COOLDOWN,
		TASK_VORTIGAUNT_FIRE_EXTRACT_OUTPUT,
		TASK_VORTIGAUNT_WAIT_FOR_PLAYER,
		TASK_VORTIGAUNT_GET_HEAL_TARGET,
		TASK_VORTIGAUNT_DISPEL_ANTLIONS
	};

	//=========================================================
	// Vortigaunt Conditions
	//=========================================================
	enum
	{
		COND_VORTIGAUNT_CAN_HEAL = BaseClass::NEXT_CONDITION,
		COND_VORTIGAUNT_HEAL_TARGET_TOO_FAR,	// Outside or heal range
		COND_VORTIGAUNT_HEAL_TARGET_BLOCKED,	// Blocked by an obstruction
		COND_VORTIGAUNT_HEAL_TARGET_BEHIND_US,	// Not within our "forward" range
		COND_VORTIGAUNT_HEAL_VALID,				// All conditions satisfied	
		COND_VORTIGAUNT_DISPEL_ANTLIONS,		// Repulse all antlions around us
	};

	// ------------
	// Beams
	// ------------
	inline bool		InAttackSequence( void );
	void			ClearBeams( void );
	void			ArmBeam( int beamType, int nHand );
	void			ZapBeam( int nHand );
	int				m_nLightningSprite;

	// ---------------
	//  Glow
	// ----------------
	void			ClearHandGlow( void );
	float			m_fGlowAge;
	float			m_fGlowChangeTime;
	bool			m_bGlowTurningOn;
	int				m_nCurGlowIndex;
	
	CHandle<CVortigauntEffectDispel>	m_hHandEffect[2];
	
	void			StartHandGlow( int beamType, int nHand );
	void			EndHandGlow( int beamType = VORTIGAUNT_BEAM_ALL );
	void			MaintainGlows( void );

	// ----------------
	//  Healing
	// ----------------
	bool				m_bRegenerateHealth;
	float				m_flNextHealTime;		// Next time allowed to heal player
	EHANDLE				m_hHealTarget;			// The person that I'm going to heal.
	bool				m_bPlayerRequestedHeal;	// This adds some priority to our heal (allows it to happen in combat, etc)
	float				m_flNextHealTokenTime;
	
	VortigauntHealState_t	m_eHealState;
	
	CBaseEntity		*FindHealTarget( void );
	bool			HealBehaviorAvailable( void );
	void			SetHealTarget( CBaseEntity *pTarget, bool bPlayerRequested );
	void			GatherHealConditions( void );

	int				m_nNumTokensToSpawn;
	float			m_flHealHinderedTime;
	float			m_flPainTime;
	float			m_nextLineFireTime;

	bool			m_bArmorRechargeEnabled;
	bool			m_bForceArmorRecharge;
	float			m_flDispelTestTime;
	
	bool			m_bExtractingBugbait;
	
	bool			IsCarryingNPC( void ) const { return m_bCarryingNPC; }
	bool			m_bCarryingNPC;

	COutputEvent	m_OnFinishedExtractingBugbait;
	COutputEvent	m_OnFinishedChargingTarget;
	COutputEvent	m_OnPlayerUse;
	
	//Adrian: Let's do it the right way!
	int				m_iLeftHandAttachment;
	int				m_iRightHandAttachment;
	bool			m_bStopLoopingSounds;
	float			m_flAimDelay;			// Amount of time to suppress aiming

	// used for fading from green vort to blue vort
	CNetworkVar( bool,  m_bIsBlue );
	CNetworkVar( float, m_flBlueEndFadeTime );

	// used for fading to black
	CNetworkVar( bool, m_bIsBlack );

public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
};

//=============================================================================
// 
//  Charge Token 
//	
//=============================================================================

class CVortigauntChargeToken : public CBaseEntity
{
	DECLARE_CLASS( CVortigauntChargeToken, CBaseEntity );

public:

	static CVortigauntChargeToken *CreateChargeToken( const Vector &vecOrigin, CBaseEntity *pOwner, CBaseEntity *pTarget );

	CVortigauntChargeToken( void );	

	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;

	void	FadeAndDie( void );
	void	SeekThink( void );
	void	SeekTouch( CBaseEntity	*pOther );
	void	SetTargetEntity( CBaseEntity *pTarget ) { m_hTarget = pTarget; }

private:

	Vector	GetSteerVector( const Vector &vecForward );

	float				m_flLifetime;
	EHANDLE				m_hTarget;

	CNetworkVar( bool, m_bFadeOut );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
};

//=============================================================================
// 
//  Dispel Effect
//	
//=============================================================================

class CVortigauntEffectDispel : public CBaseEntity
{
	DECLARE_CLASS( CVortigauntEffectDispel, CBaseEntity );

public:

	static CVortigauntEffectDispel *CreateEffectDispel( const Vector &vecOrigin, CBaseEntity *pOwner, CBaseEntity *pTarget );

	CVortigauntEffectDispel( void );	

	virtual void	Spawn( void );

	void	FadeAndDie( void );

private:

	CNetworkVar( bool, m_bFadeOut );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
};

#endif // NPC_VORTIGAUNT_H
