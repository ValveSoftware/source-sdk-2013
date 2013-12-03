//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef NPC_SCANNER_H
#define NPC_SCANNER_H
#ifdef _WIN32
#pragma once
#endif

#include "npc_basescanner.h"

//------------------------------------
// Spawnflags
//------------------------------------
#define SF_CSCANNER_NO_DYNAMIC_LIGHT	(1 << 16)
#define SF_CSCANNER_STRIDER_SCOUT		(1 << 17)

class CBeam;
class CSprite;
class SmokeTrail;
class CSpotlightEnd;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_CScanner : public CNPC_BaseScanner
{
	DECLARE_CLASS( CNPC_CScanner, CNPC_BaseScanner );

public:
	CNPC_CScanner();

	int				GetSoundInterests( void ) { return (SOUND_WORLD|SOUND_COMBAT|SOUND_PLAYER|SOUND_DANGER); }
	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );

	bool			FValidateHintType(CAI_Hint *pHint);

	virtual int		TranslateSchedule( int scheduleType );
	Disposition_t	IRelationType(CBaseEntity *pTarget);

	void			NPCThink( void );

	void			GatherConditions( void );
	void			PrescheduleThink( void );
	void			Precache(void);
	void			RunTask( const Task_t *pTask );
	int				SelectSchedule(void);
	virtual char	*GetScannerSoundPrefix( void );
	void			Spawn(void);
	void			Activate();
	void			StartTask( const Task_t *pTask );
	void			UpdateOnRemove( void );
	void			DeployMine();
	float			GetMaxSpeed();
	virtual void	Gib( void );

	void			HandleAnimEvent( animevent_t *pEvent );
	Activity		NPC_TranslateActivity( Activity eNewActivity );

	void			InputDisableSpotlight( inputdata_t &inputdata );
	void			InputSetFollowTarget( inputdata_t &inputdata );
	void			InputClearFollowTarget( inputdata_t &inputdata );
	void			InputInspectTargetPhoto( inputdata_t &inputdata );
	void			InputInspectTargetSpotlight( inputdata_t &inputdata );
	void			InputDeployMine( inputdata_t &inputdata );
	void			InputEquipMine( inputdata_t &inputdata );
	void			InputShouldInspect( inputdata_t &inputdata );

	void			InspectTarget( inputdata_t &inputdata, ScannerFlyMode_t eFlyMode );

	void			Event_Killed( const CTakeDamageInfo &info );

	char			*GetEngineSound( void );

	virtual float	MinGroundDist(void);
	virtual void	AdjustScannerVelocity( void );

	virtual float	GetHeadTurnRate( void );

public:
	bool			HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);

	// ------------------------------
	//	Inspecting
	// ------------------------------
	Vector			m_vInspectPos;
	float			m_fInspectEndTime;
	float			m_fCheckCitizenTime;	// Time to look for citizens to harass
	float			m_fCheckHintTime;		// Time to look for hints to inspect
	bool			m_bShouldInspect;
	bool			m_bOnlyInspectPlayers;
	bool			m_bNeverInspectPlayers;

	void			SetInspectTargetToEnt(CBaseEntity *pEntity, float fInspectDuration);
	void			SetInspectTargetToPos(const Vector &vInspectPos, float fInspectDuration);
	void			SetInspectTargetToHint(CAI_Hint *pHint, float fInspectDuration);
	void			ClearInspectTarget(void);
	bool			HaveInspectTarget(void);
	Vector			InspectTargetPosition(void);
	bool			IsValidInspectTarget(CBaseEntity *pEntity);
	CBaseEntity*	BestInspectTarget(void);
	void			RequestInspectSupport(void);

	bool			IsStriderScout() { return HasSpawnFlags( SF_CSCANNER_STRIDER_SCOUT ); }

	// ------------------------
	//  Photographing
	// ------------------------
	float			m_fNextPhotographTime;
	CSprite*		m_pEyeFlash;

	void			TakePhoto( void );
	void			BlindFlashTarget( CBaseEntity *pTarget );

	// ------------------------------
	//	Spotlight
	// ------------------------------
	Vector			m_vSpotlightTargetPos;
	Vector			m_vSpotlightCurrentPos;
	CHandle<CBeam>	m_hSpotlight;
	CHandle<CSpotlightEnd> m_hSpotlightTarget;
	Vector			m_vSpotlightDir;
	Vector			m_vSpotlightAngVelocity;
	float			m_flSpotlightCurLength;
	float			m_flSpotlightMaxLength;
	float			m_flSpotlightGoalWidth;
	float			m_fNextSpotlightTime;
	int				m_nHaloSprite;

	void			SpotlightUpdate(void);
	Vector			SpotlightTargetPos(void);
	Vector			SpotlightCurrentPos(void);
	void			SpotlightCreate(void);
	void			SpotlightDestroy(void);

protected:
	void			BecomeClawScanner( void ) { m_bIsClawScanner = true; }

private:
	bool			MovingToInspectTarget( void );
	virtual float	GetGoalDistance( void );

	bool m_bIsClawScanner;	// Formerly the shield scanner.
	bool m_bIsOpen;			// Only for claw scanner

	COutputEvent		m_OnPhotographPlayer;
	COutputEvent		m_OnPhotographNPC;

	bool				OverrideMove(float flInterval);
	void				MoveToTarget(float flInterval, const Vector &MoveTarget);
	void				MoveToSpotlight(float flInterval);
	void				MoveToPhotograph(float flInterval);

	// Attacks
	bool				m_bNoLight;
	bool				m_bPhotoTaken;

	void				AttackPreFlash(void);
	void				AttackFlash(void);
	void				AttackFlashBlind(void);

	virtual void		AttackDivebomb(void);

	DEFINE_CUSTOM_AI;

	// Custom interrupt conditions
	enum
	{
		COND_CSCANNER_HAVE_INSPECT_TARGET = BaseClass::NEXT_CONDITION,
		COND_CSCANNER_INSPECT_DONE,							
		COND_CSCANNER_CAN_PHOTOGRAPH,
		COND_CSCANNER_SPOT_ON_TARGET,

		NEXT_CONDITION,
	};

	// Custom schedules
	enum
	{
		SCHED_CSCANNER_SPOTLIGHT_HOVER = BaseClass::NEXT_SCHEDULE,
		SCHED_CSCANNER_SPOTLIGHT_INSPECT_POS,
		SCHED_CSCANNER_SPOTLIGHT_INSPECT_CIT,
		SCHED_CSCANNER_PHOTOGRAPH_HOVER,
		SCHED_CSCANNER_PHOTOGRAPH,
		SCHED_CSCANNER_ATTACK_FLASH,
		SCHED_CSCANNER_MOVE_TO_INSPECT,
		SCHED_CSCANNER_PATROL,

		NEXT_SCHEDULE,
	};

	// Custom tasks
	enum
	{
		TASK_CSCANNER_SET_FLY_PHOTO = BaseClass::NEXT_TASK,
		TASK_CSCANNER_SET_FLY_SPOT,
		TASK_CSCANNER_PHOTOGRAPH,
		TASK_CSCANNER_ATTACK_PRE_FLASH,
		TASK_CSCANNER_ATTACK_FLASH,
		TASK_CSCANNER_SPOT_INSPECT_ON,
		TASK_CSCANNER_SPOT_INSPECT_WAIT,
		TASK_CSCANNER_SPOT_INSPECT_OFF,
		TASK_CSCANNER_CLEAR_INSPECT_TARGET,
		TASK_CSCANNER_GET_PATH_TO_INSPECT_TARGET,

		NEXT_TASK,
	};

	DECLARE_DATADESC();
};

#endif // NPC_SCANNER_H
