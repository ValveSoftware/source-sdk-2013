//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Simple, small, free-standing tools for building AIs
//
//=============================================================================//

#ifndef AI_UTILS_H
#define AI_UTILS_H

#include "simtimer.h"
#include "ai_component.h"

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------
//
// Function to get the local player. AI does not want asserts or warnings,
// just NULL result
//
//-----------------------------------------------------------------------------

inline CBasePlayer *AI_GetSinglePlayer()
{
	if ( gpGlobals->maxClients > 1 )
	{
		return NULL;
	}
	
	return UTIL_GetLocalPlayer();
}

inline bool AI_IsSinglePlayer()
{
	return ( gpGlobals->maxClients == 1 );
}


//-----------------------------------------------------------------------------
//
// CAI_MoveMonitor
//
// Purpose: Watch an entity, trigger if moved more than a tolerance
//
//-----------------------------------------------------------------------------

class CAI_MoveMonitor
{
public:
	CAI_MoveMonitor()
	 : m_vMark( 0, 0, 0 ),
	   m_flMarkTolerance( NO_MARK )
	{
	}
	
	void SetMark( CBaseEntity *pEntity, float tolerance )
	{
		if ( pEntity )
		{
			m_vMark = pEntity->GetAbsOrigin();
			m_flMarkTolerance = tolerance;
		}
	}
	
	void ClearMark()
	{
	   m_flMarkTolerance = NO_MARK;
	}

	bool IsMarkSet()
	{
		return ( m_flMarkTolerance != NO_MARK );
	}

	bool TargetMoved( CBaseEntity *pEntity )
	{
		if ( IsMarkSet() && pEntity != NULL )
		{
			float distance = ( m_vMark - pEntity->GetAbsOrigin() ).Length();
			if ( distance > m_flMarkTolerance )
				return true;
		}
		return false;
	}

	bool TargetMoved2D( CBaseEntity *pEntity )
	{
		if ( IsMarkSet() && pEntity != NULL )
		{
			float distance = ( m_vMark.AsVector2D() - pEntity->GetAbsOrigin().AsVector2D() ).Length();
			if ( distance > m_flMarkTolerance )
				return true;
		}
		return false;
	}

	Vector GetMarkPos() { return m_vMark; }
	
private:
	enum
	{
		NO_MARK = -1
	};
	
	Vector			   m_vMark;
	float			   m_flMarkTolerance;

	DECLARE_SIMPLE_DATADESC();
};


//-----------------------------------------------------------------------------
//
// CAI_ShotRegulator
//
// Purpose: Assists in creating non-constant bursty shooting style
//
//-----------------------------------------------------------------------------
class CAI_ShotRegulator
{
public:
	CAI_ShotRegulator();

	// Sets the various parameters for burst (this one's for backwards compatibility)
	// NOTE: This will modify the next shot time
	void SetParameters( int minShotsPerBurst, int maxShotsPerBurst, float minRestTime, float maxRestTime = 0.0 );

	// NOTE: The next 3 methods will *not* modify the next shot time
	// Sets the number of shots to shoot in a single burst
	void SetBurstShotCountRange( int minShotsPerBurst, int maxShotsPerBurst );

	// How much time should I rest between bursts?
	void SetRestInterval( float flMinRestInterval, float flMaxRestInterval );

	// How much time should I wait in between shots in a single burst?
	void SetBurstInterval( float flMinBurstInterval, float flMaxBurstInterval );
	
	// Poll the current parameters
	void GetBurstShotCountRange( int *pMinShotsPerBurst, int *pMaxShotsPerBurst ) const;
	void GetRestInterval( float *pMinRestInterval, float *pMaxRestInterval ) const;
	void GetBurstInterval( float *pMinBurstInterval, float *pMaxBurstInterval ) const;

	// Reset the state. If true, the next burst time is set to now,
	// otherwise it'll wait one rest interval before shooting 
	void Reset( bool bStartShooting = true );

	// Should we shoot?
	bool ShouldShoot() const;

	// When will I shoot next?
	float NextShotTime() const;

	// Am I in the middle of a rest period?
	bool IsInRestInterval() const;

	// NOTE: These will not modify the next shot time
	int GetBurstShotsRemaining() const;
	void SetBurstShotsRemaining( int shots );

	// Call this when the NPC fired the weapon;
	void OnFiredWeapon();

	// Causes us to potentially delay our shooting time
	void FireNoEarlierThan( float flTime );

	// Prevent/Allow shooting
	void EnableShooting( void );
	void DisableShooting( void );
	
private:
	float	m_flNextShotTime;
	bool	m_bInRestInterval;
	unsigned short	m_nBurstShotsRemaining;
	unsigned short	m_nMinBurstShots, m_nMaxBurstShots;
	float	m_flMinRestInterval, m_flMaxRestInterval;
	float	m_flMinBurstInterval, m_flMaxBurstInterval;
	bool	m_bDisabled;

	DECLARE_SIMPLE_DATADESC();
};


//-----------------------------------------------------------------------------
//
// CAI_AccelDecay
//
// Purpose: Maintain a smooth acceleration, deceleration curve
//
//-----------------------------------------------------------------------------

class CAI_AccelDecay
{
public:
	CAI_AccelDecay();

	void	SetParameters( float minVelocity, float maxVelocity, float accelPercent, float decelPercent );
	float	Update( float flCurrent, float flTarget, float flInterval );
	void	ResetVelocity( float flVelocity = 0.0f );
	void	SetMaxVelocity( float maxVelocity );

private:
	float	m_velocity;

	float	m_maxVelocity; // = 300;
	float	m_minVelocity; // = 10;

	float	m_invDecay; //	0.8	// maintain X percent of velocity when slowing down
	float	m_decayTime;//	0.4161	// Sum( 1..cycle, HEIGHTINVDECAY^cycle ) 
	float	m_accel;	//	0.5		// accel toward maxVelocity by X percent each cycle

	DECLARE_SIMPLE_DATADESC();
};



//-----------------------------------------------------------------------------
//
// Purpose: Utility to allow place grace in cover
//
//-----------------------------------------------------------------------------

struct AI_FreePassParams_t
{
	float timeToTrigger;		// How long after not detected to issue pass
	float duration;				// How long in the open pass before revoked
	float moveTolerance;		// How far in open needed to move to revoke pass
	float refillRate;			// After hiding again during pass, how quickly to reinstitute pass(seconds per second)
	float coverDist;			// When hiding, how far from an obstructing object needed to be considered in cover
	
	float peekTime;				// How long allowed to peek
	float peekTimeAfterDamage;	// How long allowed to peek after damaged by
	float peekEyeDist;			// how far spaced out the eyes are
	float peekEyeDistZ;			// how far below eye position to test eyes (handles peek up)

	DECLARE_SIMPLE_DATADESC();
};

//-------------------------------------

class CAI_FreePass : public CAI_Component
{
public:
	CAI_FreePass()
	 : m_FreePassTimeRemaining(0)
	{
	}

	void			Reset( float passTime = -1, float moveTolerance = -1 );

	void			SetPassTarget( CBaseEntity *pTarget )		{ m_hTarget = pTarget; m_FreePassTimeRemaining = 0; }
	CBaseEntity *	GetPassTarget()								{ return m_hTarget; }
	
	void			SetParams( const AI_FreePassParams_t &params )	{ m_Params = params; }
	const AI_FreePassParams_t &GetParams() const					{ return m_Params; }
	
	//---------------------------------
	//	Free pass
	//---------------------------------
	void			Update();
	
	bool			HasPass();
	void 			Revoke( bool bUpdateMemory = false );
	
	float			GetTimeRemaining()					{ return m_FreePassTimeRemaining; }
	void			SetTimeRemaining( float passTime )	{ m_FreePassTimeRemaining = passTime; }

	bool			ShouldAllowFVisible( bool bBaseResult );

private:
	EHANDLE			m_hTarget;

	float			m_FreePassTimeRemaining;
	CAI_MoveMonitor m_FreePassMoveMonitor;
	
	AI_FreePassParams_t m_Params;

	DECLARE_SIMPLE_DATADESC();
};

//-----------------------------------------------------------------------------

class CTraceFilterNav : public CTraceFilterSimple
{
public:
	CTraceFilterNav( CAI_BaseNPC *pProber, bool bIgnoreTransientEntities, const IServerEntity *passedict, int collisionGroup, bool m_bAllowPlayerAvoid = true );
	bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask );

private:
	CAI_BaseNPC *m_pProber;
	bool m_bIgnoreTransientEntities;
	bool m_bCheckCollisionTable;
	bool m_bAllowPlayerAvoid;
};

extern string_t g_iszFuncBrushClassname;

#endif // AI_UTILS_H
