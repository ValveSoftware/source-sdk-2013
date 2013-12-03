//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef NPC_BARNACLE_H
#define NPC_BARNACLE_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "studio.h"
#include "physics_prop_ragdoll.h"

class CNPC_Barnacle;


#define BARNACLE_PULL_SPEED			80
#define BARNACLE_KILL_VICTIM_DELAY	5 // how many seconds after pulling prey in to gib them. 

// Tongue
#define BARNACLE_TONGUE_POINTS	8

#define BARNACLE_MIN_PULL_TIME	3.0f

#define NUM_BARNACLE_GIBS	4

#define	SF_BARNACLE_CHEAP_DEATH	(1<<16)	// Don't spawn as many gibs
#define	SF_BARNACLE_AMBUSH	(1<<17)	// Start with tongue retracted and wait for input.

// when true, causes the barnacle's visible tongue to offset
// from the physical one when pulling the player.
#define BARNACLE_USE_TONGUE_OFFSET 1


//-----------------------------------------------------------------------------
// Purpose: This is the entity we place at the top & bottom of the tongue, to create a vphysics spring
//-----------------------------------------------------------------------------
class CBarnacleTongueTip : public CBaseAnimating
{
	DECLARE_CLASS( CBarnacleTongueTip, CBaseAnimating );

public:
	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void UpdateOnRemove( );
	virtual void VPhysicsUpdate( IPhysicsObject *pPhysics );

	virtual int	UpdateTransmitState( void );
	bool						CreateSpring( CBaseAnimating *pTongueRoot );
	static CBarnacleTongueTip	*CreateTongueTip( CNPC_Barnacle *pBarnacle, CBaseAnimating *pTongueRoot, const Vector &vecOrigin, const QAngle &vecAngles );
	static CBarnacleTongueTip	*CreateTongueRoot( const Vector &vecOrigin, const QAngle &vecAngles );

	IPhysicsSpring			*m_pSpring;

private:
	CHandle<CNPC_Barnacle>	m_hBarnacle;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_Barnacle : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_Barnacle, CAI_BaseNPC );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CNPC_Barnacle();
	~CNPC_Barnacle();

	void			Spawn( void );
	virtual void	Activate( void );
	void			Precache( void );
	Class_T			Classify ( void );
	virtual void	ComputeWorldSpaceSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs );
	virtual void	HandleAnimEvent( animevent_t *pEvent );
	void			Event_Killed( const CTakeDamageInfo &info );
	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void			PlayerHasIlluminatedNPC( CBasePlayer *pPlayer, float flDot );

	// The tongue's vphysics updated
	void OnTongueTipUpdated();

private:
	void SetAltitude( float flAltitude );
	void SpawnDeathGibs( void );

	void InitTonguePosition( void );
	CBaseEntity* TongueTouchEnt ( float *pflLength );
	void BarnacleThink ( void );
	void SwallowPrey( void );
	void WaitTillDead ( void );
 	void AttachTongueToTarget( CBaseEntity *pTouchEnt, Vector vecGrabPos );
	CRagdollProp *AttachRagdollToTongue( CBaseAnimating *pAnimating );
	void RemoveRagdoll( bool bDestroyRagdoll );
	void LostPrey( bool bRemoveRagdoll );
	void BitePrey( void );

	// Updates the tongue length
	void UpdateTongue( void );

	// Spit out the prey; add physics force!
	void SpitPrey();

	void SprayBlood();

	// What type of enemy do we have?
	bool IsEnemyAPlayer();
	bool IsEnemyARagdoll();
	bool IsEnemyAPhysicsObject();
	bool IsEnemyAnNPC();

	bool CanPickup( CBaseCombatCharacter *pBCC );

	// Allows the ragdoll to settle before biting it
	bool WaitForRagdollToSettle( float flBiteZOffset );

	// Allows the physics prop to settle before biting it
	bool WaitForPhysicsObjectToSettle( float flBiteZOffset );

	// Play a scream right before biting
	void PlayLiftingScream( float flBiteZOffset );

	// Pulls the prey upward toward the mouth
	void PullEnemyTorwardsMouth( bool bAdjustEnemyOrigin );

	// Lift the prey stuck to our tongue up towards our mouth
	void LiftPrey( void );
	void LiftPlayer( float flBiteZOffset );
	void LiftRagdoll( float flBiteZOffset );
	void LiftPhysicsObject( float flBiteZOffset );
	void LiftNPC( float flBiteZOffset );

	void UpdatePlayerConstraint( void );

	void InputDropTongue( inputdata_t &inputdata );
	void InputSetDropTongueSpeed( inputdata_t &inputdata );
	void DropTongue( void );



#if HL2_EPISODIC
	/// Decides whether something should poison the barnacle upon eating
	static bool IsPoisonous( CBaseEntity *pVictim );

	void InputLetGo( inputdata_t &inputdata );
	COutputEHANDLE m_OnGrab, m_OnRelease;

	const impactdamagetable_t &GetPhysicsImpactDamageTable( void );
#endif

	CNetworkVar( float, m_flAltitude );
	int				m_cGibs;				// barnacle loads up on gibs each time it kills something.
	bool			m_bLiftingPrey;			// true when the prey's on the tongue and being lifted to the mouth
	bool			m_bSwallowingPrey;		// if it's a human, true while the barnacle chews it and swallows it whole. 
	float			m_flDigestFinish;		// time at which we've finished digesting something we chewed
	float			m_flVictimHeight;
	int				m_iGrabbedBoneIndex;
	bool			m_bPlayedPullSound;
	bool			m_bPlayerWasStanding;
	
	static const char	*m_szGibNames[NUM_BARNACLE_GIBS];

	// Tongue spline points
	CNetworkVar( Vector, m_vecRoot );
	CNetworkVar( Vector, m_vecTip );
	CNetworkVar( Vector, m_vecTipDrawOffset );

	// Tongue tip & root
	CHandle<CBarnacleTongueTip>	m_hTongueRoot;
	CHandle<CBarnacleTongueTip>	m_hTongueTip;
	CHandle<CRagdollProp>		m_hRagdoll;
	matrix3x4_t					m_pRagdollBones[MAXSTUDIOBONES];
	IPhysicsConstraint			*m_pConstraint;
	float						m_flRestUnitsAboveGround;
	int							m_nSpitAttachment;
	EHANDLE						m_hLastSpitEnemy;
	int							m_nShakeCount;

	float						m_flNextBloodTime;
#ifndef _XBOX
	int							m_nBloodColor;
#endif
	Vector						m_vecBloodPos;

	float						m_flBarnaclePullSpeed;
	float						m_flLocalTimer;

	Vector						m_vLastEnemyPos;
	float						m_flLastPull;
	CSimpleSimTimer				m_StuckTimer;
	bool						m_bSwallowingBomb;
#ifdef HL2_EPISODIC
	bool						m_bSwallowingPoison;
#endif
	
#if BARNACLE_USE_TONGUE_OFFSET
	// Static because only one barnacle can be holding the player
	// at a time, and because it's not really a big deal if it
	// resets to zero after reload.
	const static Vector				m_svPlayerHeldTipOffset;
#endif

	DEFINE_CUSTOM_AI;
};


//-----------------------------------------------------------------------------
// What type of enemy do we have?
//-----------------------------------------------------------------------------
inline bool CNPC_Barnacle::IsEnemyAPlayer()
{
	return GetEnemy() && GetEnemy()->IsPlayer();
}

inline bool CNPC_Barnacle::IsEnemyARagdoll()
{
	return m_hRagdoll != NULL;
}

inline bool CNPC_Barnacle::IsEnemyAPhysicsObject()
{
	return !m_hRagdoll && GetEnemy() && !GetEnemy()->IsPlayer() && 
		!GetEnemy()->MyNPCPointer() && (GetEnemy()->GetMoveType() == MOVETYPE_VPHYSICS);
}

inline bool CNPC_Barnacle::IsEnemyAnNPC()
{
	return !IsEnemyARagdoll() && (GetEnemy()->MyNPCPointer() != NULL);
}


#endif // NPC_BARNACLE_H
