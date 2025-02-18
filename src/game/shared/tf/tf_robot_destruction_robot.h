//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The robots for use in the Robot Destruction TF2 game mode.
//
//=========================================================================//
#ifndef ROBOT_DESTRUCTION_ROBOT_H
#define ROBOT_DESTRUCTION_ROBOT_H
#pragma once

#include "cbase.h"

#ifdef GAME_DLL
	#include "tf_shareddefs.h"
	#include "pathtrack.h"
	#include "NextBotGroundLocomotion.h"
	#include "NextBotIntentionInterface.h"
	#include "NextBotBehavior.h"
	#include "NextBot.h"
	#include "../server/NextBot/Path/NextBotPathFollow.h"
	#include "../server/NextBot/Path/NextBotPath.h"
	#include "../server/tf/halloween/headless_hatman_body.h"
	#include "tf_obj_dispenser.h"
#else
	#include "c_obj_dispenser.h"
#endif

#ifdef CLIENT_DLL
	#define CTFRobotDestruction_Robot C_TFRobotDestruction_Robot
	#define CRobotDispenser C_RobotDispenser
#endif

#include "props_shared.h"

enum eRobotType
{
	ROBOT_TYPE_FRUSTUM = 0,
	ROBOT_TYPE_SPHERE,
	ROBOT_TYPE_KING,

	NUM_ROBOT_TYPES
};

enum eRobotUIState
{
	ROBOT_STATE_INACIVE = 0,
	ROBOT_STATE_ACTIVE,
	ROBOT_STATE_DEAD,
	ROBOT_STATE_SHIELDED,

	NUM_ROBOT_STATES
};

struct RobotData_t
{
public:
	enum EStringDataKey_t
	{
		MODEL_KEY = 0,
		DAMAGED_MODEL_KEY,
		HURT_SOUND_KEY,
		DEATH_SOUND_KEY,
		COLLIDE_SOUND_KEY,
		IDLE_SOUND_KEY
	};

	enum EFloatDataKey_t
	{
		HEALTH_BAR_OFFSET_KEY
	};

	RobotData_t( const char* pszModelName
			   , const char* pszDamagedModelName
			   , const char *pszHurtSound
			   , const char *pszDeathSound
			   , const char *pszCollideSound
			   , const char *pszIdleSound
			   , float flHealthBarOffset )
	{
		m_stringMap.SetLessFunc( DefLessFunc(int) );
		m_floatMap.SetLessFunc( DefLessFunc(int) );
		m_stringMap.Insert( MODEL_KEY, pszModelName );
		m_stringMap.Insert( DAMAGED_MODEL_KEY, pszDamagedModelName );
		m_stringMap.Insert( HURT_SOUND_KEY, pszHurtSound );
		m_stringMap.Insert( DEATH_SOUND_KEY, pszDeathSound );
		m_stringMap.Insert( COLLIDE_SOUND_KEY, pszCollideSound );
		m_stringMap.Insert( IDLE_SOUND_KEY, pszIdleSound );
		m_floatMap.Insert( HEALTH_BAR_OFFSET_KEY, flHealthBarOffset );
	}

	const char* GetStringData( EStringDataKey_t key ) const
	{
		return GetData< const char * >( m_stringMap, (int)key );
	}

	float GetFloatData( EFloatDataKey_t key ) const
	{
		return GetData< float >( m_floatMap, (int)key );
	}

	void Precache();

private:

	template< typename T >
	T GetData( const CUtlMap< int, T >& map, int nKey ) const
	{
		int nIndex = map.Find( nKey );
		if ( nIndex != map.InvalidIndex() )
		{
			return map.Element( nIndex );
		}

		AssertMsg1( 0, "No entry for key %d", nKey );
		return T(0);
	}

	CUtlMap< int, const char * > m_stringMap;
	CUtlMap< int, float > m_floatMap;
};

class CTFRobotDestruction_Robot;

#ifdef GAME_DLL

struct RobotSpawnData_t
{
	RobotSpawnData_t()
		: m_eType( ROBOT_TYPE_FRUSTUM )
		, m_nRobotHealth( 100 )
		, m_nPoints( 0 )
		, m_nNumGibs( 0 )
		, m_pszPathName( NULL )
		, m_pszGroupName( NULL )
	{}

	RobotSpawnData_t &operator=( const RobotSpawnData_t& rhs )
	{
		m_eType = rhs.m_eType;
		m_nRobotHealth = rhs.m_nRobotHealth;
		m_nPoints = rhs.m_nPoints;
		m_nNumGibs = rhs.m_nNumGibs;
		m_pszPathName = rhs.m_pszPathName;
		m_pszGroupName = rhs.m_pszGroupName;

		return *this;
	}

	eRobotType m_eType;
	int m_nRobotHealth;
	int m_nPoints;
	int m_nNumGibs;
	const char *m_pszPathName;
	const char *m_pszGroupName;
};

//----------------------------------------------------------------------------
class CRobotLocomotion : public NextBotGroundLocomotion
{
public:
	CRobotLocomotion( INextBot *bot ) : NextBotGroundLocomotion( bot ) { }
	virtual ~CRobotLocomotion() { }

	virtual float GetGroundSpeed() const OVERRIDE;
	virtual float GetRunSpeed( void ) const OVERRIDE;			// get maximum running speed
	virtual float GetStepHeight( void ) const OVERRIDE;				// if delta Z is greater than this, we have to jump to get up
	virtual float GetMaxJumpHeight( void ) const OVERRIDE;			// return maximum height of a jump

	virtual bool ShouldCollideWith( const CBaseEntity *object ) const OVERRIDE;

private:
	virtual float GetMaxYawRate( void ) const OVERRIDE;				// return max rate of yaw rotation
};


//----------------------------------------------------------------------------
class CRobotIntention : public IIntention
{
public:
	CRobotIntention( class CTFRobotDestruction_Robot *me );
	virtual ~CRobotIntention();

	virtual void Reset( void ) OVERRIDE;
	virtual void Update( void ) OVERRIDE;

	virtual QueryResultType			IsPositionAllowed( const INextBot *me, const Vector &pos ) const OVERRIDE;	// is the a place we can be?

	virtual INextBotEventResponder *FirstContainedResponder( void ) const OVERRIDE  { return m_behavior; }
	virtual INextBotEventResponder *NextContainedResponder( INextBotEventResponder *current ) const OVERRIDE { return NULL; }

private:
	Behavior< CTFRobotDestruction_Robot > *m_behavior;
};

//---------------------------------------------------------------------------------------------
class CRobotBehavior : public Action< CTFRobotDestruction_Robot >
{
public:
	virtual Action< CTFRobotDestruction_Robot > *InitialContainedAction( CTFRobotDestruction_Robot *me ) OVERRIDE;
	virtual ActionResult< CTFRobotDestruction_Robot >	OnStart( CTFRobotDestruction_Robot *me, Action< CTFRobotDestruction_Robot > *priorAction ) OVERRIDE;
	virtual ActionResult< CTFRobotDestruction_Robot >	Update( CTFRobotDestruction_Robot *me, float interval ) OVERRIDE;
	virtual EventDesiredResult< CTFRobotDestruction_Robot > OnInjured( CTFRobotDestruction_Robot *me, const CTakeDamageInfo &info );
	EventDesiredResult< CTFRobotDestruction_Robot > OnContact( CTFRobotDestruction_Robot *me, CBaseEntity *pOther, CGameTrace *result = NULL );
	virtual const char *GetName( void ) const	{ return "RobotBehavior"; }		// return name of this action

private:
	CountdownTimer m_SpeakTimer;
	CountdownTimer m_IdleSpeakTimer;
};
#endif

class CRobotDispenser : 
#ifdef GAME_DLL
	public CObjectDispenser
#else
	public C_ObjectDispenser
#endif
{
#ifdef GAME_DLL
	DECLARE_CLASS( CRobotDispenser, CObjectDispenser )
#else
	DECLARE_CLASS( CRobotDispenser, C_ObjectDispenser )
#endif
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();
public:
#ifdef GAME_DLL
	CRobotDispenser();

	virtual void Spawn( void ) OVERRIDE;
	virtual void OnGoActive( void ) OVERRIDE;
	virtual void GetControlPanelInfo( int nPanelIndex, const char *&pPanelName ) OVERRIDE;
	virtual void SetModel( const char *pModel ) OVERRIDE;
	virtual float GetDispenserRadius( void ) OVERRIDE { return 128; }
	virtual float GetHealRate() const OVERRIDE { return 5.f; }

	virtual int DispenseMetal( CTFPlayer * ) OVERRIDE { return 0; }
	virtual bool DispenseAmmo( CTFPlayer * ) OVERRIDE { return false; }

private:

	virtual void PlayActiveSound() OVERRIDE { /*DO NOTHING*/ }
#endif
};

class CTFRobotDestruction_RobotAnimController
{
public:
	CTFRobotDestruction_RobotAnimController( CTFRobotDestruction_Robot *pOuter );
	void Update();
	void Impulse( const Vector& vecImpulse );
private:
	void Approach( Vector& vecIn, const Vector& vecTarget, float flRate );
	void GetPoseParams();

	Vector m_vecOldOrigin;
	Vector m_vecLean;
	Vector m_vecImpulse;
	CTFRobotDestruction_Robot *m_pOuter;
	
	struct PoseParams_t
	{
		int m_nMoveX;
		int m_nMoveY;
	} m_poseParams;
};

#ifdef GAME_DLL
	typedef NextBotCombatCharacter RobotBaseClass;
#else
	typedef CBaseCombatCharacter RobotBaseClass;
#endif

class CTFRobotDestruction_Robot : public RobotBaseClass
#ifdef CLIENT_DLL
	, public CGameEventListener
#endif
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CTFRobotDestruction_Robot, RobotBaseClass )
	DECLARE_NETWORKCLASS();
public:

	CTFRobotDestruction_Robot( void );
	virtual ~CTFRobotDestruction_Robot( void );
	static void StaticPrecache( void );
	virtual void Precache( void ) OVERRIDE;

	virtual void Spawn( void ) OVERRIDE;
	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const OVERRIDE;
#ifdef CLIENT_DLL
	virtual float GetHealthBarHeightOffset( void ) const OVERRIDE;
	virtual void OnDataChanged( DataUpdateType_t type ) OVERRIDE;
	virtual int	GetHealth( void ) const OVERRIDE { return m_iHealth; }
	virtual int	GetMaxHealth( void ) const OVERRIDE { return m_iMaxHealth; }
	virtual bool IsHealthBarVisible( void ) const OVERRIDE { return true; }
	virtual void UpdateClientSideAnimation( void ) OVERRIDE;
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;
	virtual CStudioHdr* OnNewModel() OVERRIDE;
	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options ) OVERRIDE;
	void UpdateDamagedEffects( void );
#else
	virtual void HandleAnimEvent( animevent_t *pEvent ) OVERRIDE;
	virtual bool IsRemovedOnReset( void ) const { return false; }
	virtual void UpdateOnRemove( void ) OVERRIDE;
	virtual void Event_Killed( const CTakeDamageInfo &info ) OVERRIDE;
	virtual int OnTakeDamage( const CTakeDamageInfo &info ) OVERRIDE;
	virtual void TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator ) OVERRIDE;
	virtual void UpdateAnimsThink( void );
	virtual bool IsProjectileCollisionTarget( void ) const OVERRIDE { return true; }
	
	void RepairSelfThink( void );
	bool GetShieldedState( void ) const { return m_bShielded; }
	CPathTrack *GetNextPath( void ) const { return m_hNextPath; }
	void ArriveAtPath( void );
	void SetRobotSpawnData( const RobotSpawnData_t& data ) { m_spawnData = data; m_eType = data.m_eType; }
	const RobotSpawnData_t &GetRobotSpawnData() const { return m_spawnData; }
	void SetGroup( class CTFRobotDestruction_RobotGroup* pGroup ) { m_hGroup.Set( pGroup ); }
	void SetSpawn( class CTFRobotDestruction_RobotSpawn* pSpawn ) { m_hSpawn.Set( pSpawn ); }
	void EnableUber( void );
	void DisableUber( void );

	// INextBot
	virtual CRobotIntention	*GetIntentionInterface( void ) const		{ return m_intention; }
	virtual CRobotLocomotion	*GetLocomotionInterface( void ) const	{ return m_locomotor; }
	virtual CHeadlessHatmanBody	*GetBodyInterface( void ) const			{ return m_body; }
	void SetNewActivity( Activity activity );
	void SetIsPanicked( bool bPanicked ) { m_bIsPanicked = bPanicked; }
	bool GetIsPanicked( void ) const { return m_bIsPanicked; }

	//Inputs
	void InputStopAndUseComputer( inputdata_t &inputdata );
private:

	void PlayDeathEffects( void );
	void ModifyDamage( CTakeDamageInfo *info ) const;
	void SpewBars( int nNumToSpew );
	void SpewBarsThink( void );
	void SelfDestructThink( void );
	void SpewGibs( void );
#endif
private:

	int							m_iHealth;
	int							m_iMaxHealth;
	CUtlVector<breakmodel_t>	m_aGibs;
	CUtlVector<breakmodel_t>	m_aSpawnProps;
	CTFRobotDestruction_RobotAnimController m_animController;
	CNetworkVar( bool, m_bShielded );
	CNetworkVar( eRobotType, m_eType );
#ifdef CLIENT_DLL
	HPARTICLEFFECT	m_hDamagedParticleEffect;
#else
	CRobotDispenser				*m_pDispenser;
	RobotSpawnData_t			m_spawnData;
	CHandle< CPathTrack >		m_hNextPath;
	int							m_nPointsSpewed;
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iHealth );
	CHandle< CTFRobotDestruction_RobotGroup > m_hGroup;
	CHandle< CTFRobotDestruction_RobotSpawn > m_hSpawn;

	CRobotIntention *m_intention;
	CRobotLocomotion *m_locomotor;
	CHeadlessHatmanBody *m_body;
	bool m_bIsPanicked;
#endif
};

#ifdef GAME_DLL
//--------------------------------------------------------------------------------------------------------------
class CRobotPathCost : public IPathCost
{
public:
	CRobotPathCost( CTFRobotDestruction_Robot *me )
	{
		m_me = me;
	}

	// return the cost (weighted distance between) of moving from "fromArea" to "area", or -1 if the move is not allowed
	virtual float operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const
	{
		if ( fromArea == NULL )
		{
			// first area in path, no cost
			return 0.0f;
		}
		else
		{
			if ( !m_me->GetLocomotionInterface()->IsAreaTraversable( area ) )
			{
				// our locomotor says we can't move here
				return -1.0f;
			}

			// compute distance traveled along path so far
			float dist;

			if ( ladder )
			{
				dist = ladder->m_length;
			}
			else if ( length > 0.0 )
			{
				// optimization to avoid recomputing length
				dist = length;
			}
			else
			{
				dist = ( area->GetCenter() - fromArea->GetCenter() ).Length();
			}

			// check height change
			float deltaZ = fromArea->ComputeAdjacentConnectionHeightChange( area );
			if ( deltaZ >= m_me->GetLocomotionInterface()->GetStepHeight() )
			{
				if ( deltaZ >= m_me->GetLocomotionInterface()->GetMaxJumpHeight() )
				{
					// too high to reach
					return -1.0f;
				}

				// jumping is slower than flat ground
				const float jumpPenalty = 5.0f;
				dist += jumpPenalty * dist;
			}
			else if ( deltaZ < -m_me->GetLocomotionInterface()->GetDeathDropHeight() )
			{
				// too far to drop
				return -1.0f;
			}

			return dist + fromArea->GetCostSoFar();
		}
	}

	CTFRobotDestruction_Robot *m_me;
};
#endif // GAME_DLL
#endif // ROBOT_DESTRUCTION_ROBOT_H
