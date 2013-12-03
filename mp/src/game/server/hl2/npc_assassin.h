//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef NPC_ASSASSIN_H
#define NPC_ASSASSIN_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "soundent.h"

//Eye states
enum eyeState_t
{
	ASSASSIN_EYE_SEE_TARGET = 0,		//Sees the target, bright and big
	ASSASSIN_EYE_SEEKING_TARGET,	//Looking for a target, blinking (bright)
	ASSASSIN_EYE_ACTIVE,			//Actively looking
	ASSASSIN_EYE_DORMANT,			//Not active
	ASSASSIN_EYE_DEAD,				//Completely invisible
};

//=========================================================
//=========================================================
class CNPC_Assassin : public CAI_BaseNPC
{
public:
	DECLARE_CLASS( CNPC_Assassin, CAI_BaseNPC );
	// DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CNPC_Assassin( void );
	
	Class_T		Classify( void )			{ return CLASS_COMBINE;	}
	int			GetSoundInterests ( void )	{ return (SOUND_WORLD|SOUND_COMBAT|SOUND_PLAYER);	}

	int			SelectSchedule ( void );
	int			MeleeAttack1Conditions ( float flDot, float flDist );
	int			RangeAttack1Conditions ( float flDot, float flDist );
	int			RangeAttack2Conditions ( float flDot, float flDist );

	void		Precache( void );
	void		Spawn( void );
	void		PrescheduleThink( void );
	void		HandleAnimEvent( animevent_t *pEvent );
	void		StartTask( const Task_t *pTask );
	void		RunTask( const Task_t *pTask );
	void		OnScheduleChange( void );
	void		GatherEnemyConditions( CBaseEntity *pEnemy );
	void		BuildScheduleTestBits( void );
	void		Event_Killed( const CTakeDamageInfo &info );

	bool		FValidateHintType ( CAI_Hint *pHint );
	bool		IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const;
	bool		MovementCost( int moveType, const Vector &vecStart, const Vector &vecEnd, float *pCost );

	float		MaxYawSpeed( void );

	const Vector &GetViewOffset( void );

private:

	void		SetEyeState( eyeState_t state );
	void		FirePistol( int hand );
	bool		CanFlip( int flipType, Activity &activity, const Vector *avoidPosition );

	int			m_nNumFlips;
	int			m_nLastFlipType;
	float		m_flNextFlipTime;	//Next earliest time the assassin can flip again
	float		m_flNextLungeTime;
	float		m_flNextShotTime;

	bool		m_bEvade;
	bool		m_bAggressive;		// Sets certain state, including whether or not her eye is visible
	bool		m_bBlinkState;

	CSprite				*m_pEyeSprite;
	CSpriteTrail		*m_pEyeTrail;

	DEFINE_CUSTOM_AI;
};


#endif // NPC_ASSASSIN_H
