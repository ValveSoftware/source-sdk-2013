//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_BLENDED_MOVEMENT_H
#define AI_BLENDED_MOVEMENT_H

#include "ai_basenpc.h"
#include "ai_motor.h"
#include "ai_navigator.h"

struct AI_Waypoint_t;

//-----------------------------------------------------------------------------
// CLASS: CAI_BlendedMotor
//
// Purpose: Home of fancy human animation transition code
//
//-----------------------------------------------------------------------------

class CAI_BlendedMotor : public CAI_Motor
{
	typedef CAI_Motor BaseClass;
public:
	CAI_BlendedMotor( CAI_BaseNPC *pOuter )
	 :	BaseClass( pOuter )
	{
		m_iPrimaryLayer = -1;
		m_nPrimarySequence = ACT_INVALID;

		m_iSecondaryLayer = -1;
		m_nSecondarySequence =  ACT_INVALID;
		m_flSecondaryWeight = 0.0f;

		m_nSavedGoalActivity = ACT_INVALID;
		m_nSavedTranslatedGoalActivity = ACT_INVALID;
		m_nGoalSequence = ACT_INVALID;

		m_nPrevMovementSequence = ACT_INVALID;
		m_nInteriorSequence = ACT_INVALID;

		m_bDeceleratingToGoal = false;

		m_flStartCycle = 0.0f;

		m_flPredictiveSpeedAdjust = 1.0f;
		m_flReactiveSpeedAdjust = 1.0f;
		m_vecPrevOrigin1.Init();
		m_vecPrevOrigin2.Init();

		m_prevYaw = 0.0f;
		m_doTurn = 0.0f;
		m_doLeft = 0.0f;
		m_doRight = 0.0f;
		m_flNextTurnAct = 0.0f;
	}

	void 	MoveClimbStart( const Vector &climbDest, const Vector &climbDir, float climbDist, float yaw );
	void 	MoveJumpStart( const Vector &velocity );

	void	ResetMoveCalculations();
	void	MoveStart();
	void	ResetGoalSequence();
	void	MoveStop();
	void	MovePaused();
	void	MoveContinue();

	float	OverrideMaxYawSpeed( Activity activity );
	void	UpdateYaw( int speed );
	void	RecalculateYawSpeed(); 

	bool	IsDeceleratingToGoal() const	{ return m_bDeceleratingToGoal; }
	float	GetMoveScriptTotalTime();

	void	MaintainTurnActivity( void );
	bool	AddTurnGesture( float flYD );


private:
	AIMotorMoveResult_t MoveGroundExecute( const AILocalMoveGoal_t &move, AIMoveTrace_t *pTraceResult );
	AIMotorMoveResult_t MoveFlyExecute( const AILocalMoveGoal_t &move, AIMoveTrace_t *pTraceResult );


	// --------------------------------

	void	BuildMoveScript(  const AILocalMoveGoal_t &move, AIMoveTrace_t *pTraceResult );

	void	BuildVelocityScript( const AILocalMoveGoal_t &move );
	void	InsertSlowdown( float distToObstruction, float idealAccel, bool bAlwaysSlowdown );

	int		BuildTurnScript( int i, int j );
	void	BuildTurnScript( const AILocalMoveGoal_t &move );
	int 	BuildInsertNode( int i, float flTime );

	Activity GetTransitionActivity( void );
	
	// --------------------------------

	// helpers to simplify code
	float	GetCycle()														{ return GetOuter()->GetCycle();								}
	int		AddLayeredSequence( int sequence, int iPriority )				{ return GetOuter()->AddLayeredSequence( sequence, iPriority ); }
	void	SetLayerWeight( int iLayer, float flWeight )					{ GetOuter()->SetLayerWeight( iLayer, flWeight );				}
	void	SetLayerPlaybackRate( int iLayer, float flPlaybackRate )		{ GetOuter()->SetLayerPlaybackRate( iLayer, flPlaybackRate );	}
	void	SetLayerNoRestore( int iLayer, bool bNoRestore )				{ GetOuter()->SetLayerNoRestore( iLayer, bNoRestore );			}
	void	SetLayerCycle( int iLayer, float flCycle )						{ GetOuter()->SetLayerCycle( iLayer, flCycle );					}
	void	SetLayerCycle( int iLayer, float flCycle, float flPrevCycle )	{ GetOuter()->SetLayerCycle( iLayer, flCycle, flPrevCycle );	}
	void	RemoveLayer( int iLayer, float flKillRate, float flKillDelay )	{ GetOuter()->RemoveLayer( iLayer, flKillRate, flKillDelay );	}

	// --------------------------------

	struct AI_Movementscript_t
	{
	public:
		AI_Movementscript_t( )
		{
			Init( );
		};

		void Init( void )
		{
			memset( this, 0, sizeof(*this) );
		};

		float	flTime;			// time till next entry
		float	flElapsedTime;	// time since first entry

		float	flDist;			// distance to next entry

		float	flMaxVelocity;

		// float	flVelocity;

		float	flYaw;
		float	flAngularVelocity;

		bool	bLooping;
		int		nFlags;

		AI_Waypoint_t *pWaypoint;

	public:
		AI_Movementscript_t *pNext;
		AI_Movementscript_t *pPrev;

		Vector	vecLocation;

	};
	
	//---------------------------------

	CUtlVector<AI_Movementscript_t>	m_scriptMove;
	CUtlVector<AI_Movementscript_t>	m_scriptTurn;

	//---------------------------------

	bool			m_bDeceleratingToGoal;

	int				m_iPrimaryLayer;
	int				m_iSecondaryLayer;

	int				m_nPrimarySequence;
	int				m_nSecondarySequence;
	float			m_flSecondaryWeight;

	Activity		m_nSavedGoalActivity;
	Activity		m_nSavedTranslatedGoalActivity;
	int				m_nGoalSequence;

	int				m_nPrevMovementSequence;
	int				m_nInteriorSequence;

	float			m_flStartCycle;

	float			m_flCurrRate;

	float			m_flPredictiveSpeedAdjust;		// predictive speed adjust from probing slope 
	float			m_flReactiveSpeedAdjust;		// reactive speed adjust when slope movement detected
	Vector			m_vecPrevOrigin1;
	Vector			m_vecPrevOrigin2;

	//---------------------------------

	float			m_flNextTurnGesture;	// next time for large turn gesture

	//---------------------------------
	float			m_prevYaw;
	float			m_doTurn;
	float			m_doLeft;
	float			m_doRight;
	float			m_flNextTurnAct;		// next time for small turn gesture

	
	float	GetMoveScriptDist( float &flNewSpeed );
	float	GetMoveScriptYaw( void );
	void	SetMoveScriptAnim( float flNewSpeed );

	int		GetInteriorSequence( int fromSequence );

	DECLARE_SIMPLE_DATADESC();
};

//-----------------------------------------------------------------------------
// CLASS: CAI_BlendingHost
//
// Purpose: Bridge to the home of fancy human animation transition code
//
//-----------------------------------------------------------------------------

template <class BASE_NPC>
class CAI_BlendingHost : public BASE_NPC
{
	DECLARE_CLASS_NOFRIEND( CAI_BlendingHost, BASE_NPC );
public:
	const CAI_BlendedMotor *GetBlendedMotor() const { return assert_cast<const CAI_BlendedMotor *>(this->GetMotor()); }
	CAI_BlendedMotor *		GetBlendedMotor()		{ return assert_cast<CAI_BlendedMotor *>(this->GetMotor()); }

	CAI_Motor *CreateMotor()
	{
		MEM_ALLOC_CREDIT();
		return new CAI_BlendedMotor( this );
	}

	CAI_Navigator *CreateNavigator()
	{
		CAI_Navigator *pNavigator = BaseClass::CreateNavigator();
		pNavigator->SetValidateActivitySpeed( false );
		return pNavigator;
	}

	float MaxYawSpeed( void )
	{
		float override = GetBlendedMotor()->OverrideMaxYawSpeed( this->GetActivity() );
		if ( override != -1 )
			return override;
		return BaseClass::MaxYawSpeed();
	}

	float GetTimeToNavGoal()
	{
		float result = GetBlendedMotor()->GetMoveScriptTotalTime();
		if ( result != -1 )
			return result;
		return BaseClass::GetTimeToNavGoal();
	}

};

//-------------------------------------
// to simplify basic usage:
class CAI_BlendedNPC : public CAI_BlendingHost<CAI_BaseNPC>
{
	DECLARE_CLASS( CAI_BlendedNPC, CAI_BlendingHost<CAI_BaseNPC> );
};

//-----------------------------------------------------------------------------	

#endif
