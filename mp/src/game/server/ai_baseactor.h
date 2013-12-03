//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Hooks and classes for the support of humanoid NPCs with 
//			groovy facial animation capabilities, aka, "Actors"
//
//=============================================================================//

#ifndef AI_BASEACTOR_H
#define AI_BASEACTOR_H

#include "ai_basehumanoid.h"
#include "ai_speech.h"
#include "AI_Interest_Target.h"
#include <limits.h>


#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------
// CAI_BaseActor
//
// Purpose: The base class for all head/body/eye expressive NPCS.
//
//-----------------------------------------------------------------------------
enum PoseParameter_t { POSE_END=INT_MAX };
enum FlexWeight_t { FLEX_END=INT_MAX };

struct AILookTargetArgs_t
{
	EHANDLE 			hTarget;
	Vector				vTarget;
	float				flDuration;
	float				flInfluence;
	float				flRamp;
	bool 				bExcludePlayers;
	CAI_InterestTarget *pQueue;
};

class CAI_BaseActor : public CAI_ExpresserHost<CAI_BaseHumanoid>
{
	DECLARE_CLASS( CAI_BaseActor, CAI_ExpresserHost<CAI_BaseHumanoid> );

	//friend CPoseParameter;
	//friend CFlexWeight;

public:

	// FIXME: this method is lame, isn't there some sort of template thing that would get rid of the Outer pointer?

	void	Init( PoseParameter_t &index, const char *szName ) { index = (PoseParameter_t)LookupPoseParameter( szName ); };
	void	Set( PoseParameter_t index, float flValue ) { SetPoseParameter( (int)index, flValue ); }
	float	Get( PoseParameter_t index ) { return GetPoseParameter( (int)index ); }

	float	ClampWithBias( PoseParameter_t index, float value, float base );

	// Note, you must add all names to this static function in order for Init to work
	static bool	IsServerSideFlexController( char const *szName );

	void	Init( FlexWeight_t &index, const char *szName ) 
	{ 
		// Make this fatal!!!
		if ( !IsServerSideFlexController( szName ) )
		{
			Error( "You forgot to add flex controller %s to list in CAI_BaseActor::IsServerSideFlexController().", szName );
		}

		index = (FlexWeight_t)FindFlexController( szName ); 
	}
	void	Set( FlexWeight_t index, float flValue ) { SetFlexWeight( (LocalFlexController_t)index, flValue ); }
	float	Get( FlexWeight_t index ) { return GetFlexWeight( (LocalFlexController_t)index ); }


public:
	CAI_BaseActor()
	 :	m_fLatchedPositions( 0 ),
		m_latchedEyeOrigin( vec3_origin ),
		m_latchedEyeDirection( vec3_origin ),
		m_latchedHeadDirection( vec3_origin ),
		m_flBlinktime( 0 ),
		m_hLookTarget( NULL ),
		m_iszExpressionScene( NULL_STRING ),
		m_iszIdleExpression( NULL_STRING ),
		m_iszAlertExpression( NULL_STRING ),
		m_iszCombatExpression( NULL_STRING ),
		m_iszDeathExpression( NULL_STRING ),
		m_iszExpressionOverride( NULL_STRING )
	{
		memset( m_flextarget, 0, 64 * sizeof( m_flextarget[0] ) );
	}

	~CAI_BaseActor()
	{
		delete m_pExpresser;
	}

	virtual void			StudioFrameAdvance();

	virtual void			Precache();

	virtual void			SetModel( const char *szModelName );

	virtual	bool			StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget );
	virtual bool			ProcessSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );
	virtual	bool			ClearSceneEvent( CSceneEventInfo *info, bool fastKill, bool canceled );
	virtual bool			CheckSceneEventCompletion( CSceneEventInfo *info, float currenttime, CChoreoScene *scene, CChoreoEvent *event );

	Vector					EyePosition( );
	virtual Vector			HeadDirection2D( void );
	virtual Vector			HeadDirection3D( void );
	virtual Vector			EyeDirection2D( void );
	virtual Vector			EyeDirection3D( void );

	CBaseEntity				*GetLooktarget() { return m_hLookTarget.Get(); }
	virtual void			OnNewLookTarget() {};

	// CBaseFlex
	virtual	void			SetViewtarget( const Vector &viewtarget );
	
	// CAI_BaseNPC
	virtual float			PickLookTarget( bool bExcludePlayers = false, float minTime = 1.5, float maxTime = 2.5 );
	virtual float			PickLookTarget( CAI_InterestTarget &queue, bool bExcludePlayers = false, float minTime = 1.5, float maxTime = 2.5 );
	virtual bool 			PickTacticalLookTarget( AILookTargetArgs_t *pArgs );
	virtual bool 			PickRandomLookTarget( AILookTargetArgs_t *pArgs );
	virtual void			MakeRandomLookTarget( AILookTargetArgs_t *pArgs, float minTime, float maxTime );
	virtual bool			HasActiveLookTargets( void );
	virtual void 			OnSelectedLookTarget( AILookTargetArgs_t *pArgs ) { return; }
	virtual void 			ClearLookTarget( CBaseEntity *pTarget );
	virtual void			ExpireCurrentRandomLookTarget() { m_flNextRandomLookTime = gpGlobals->curtime - 0.1f; }

	virtual void			StartTaskRangeAttack1( const Task_t *pTask );

	virtual void			AddLookTarget( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp = 0.0 );
	virtual void			AddLookTarget( const Vector &vecPosition, float flImportance, float flDuration, float flRamp = 0.0 );

	virtual void			SetHeadDirection( const Vector &vTargetPos, float flInterval );

	void					UpdateBodyControl( void );
	void					UpdateHeadControl( const Vector &vHeadTarget, float flHeadInfluence );
	virtual	float			GetHeadDebounce( void ) { return 0.3; } // how much of previous head turn to use

	virtual void			MaintainLookTargets( float flInterval );
	virtual bool			ValidEyeTarget(const Vector &lookTargetPos);
	virtual bool			ValidHeadTarget(const Vector &lookTargetPos);
	virtual float			HeadTargetValidity(const Vector &lookTargetPos);

	virtual bool			ShouldBruteForceFailedNav()	{ return true; }

	void					AccumulateIdealYaw( float flYaw, float flIntensity );
	bool					SetAccumulatedYawAndUpdate( void );

	float					m_flAccumYawDelta;
	float					m_flAccumYawScale;

	//---------------------------------

	virtual	void 			OnStateChange( NPC_STATE OldState, NPC_STATE NewState );

	//---------------------------------

	virtual void			PlayExpressionForState( NPC_STATE state );
	virtual const char		*SelectRandomExpressionForState( NPC_STATE state );

	float					SetExpression( const char * );
	void					ClearExpression();
	const char *			GetExpression();

	enum
	{
		SCENE_AI_BLINK = 1,
		SCENE_AI_HOLSTER,
		SCENE_AI_UNHOLSTER,
		SCENE_AI_AIM,
		SCENE_AI_RANDOMLOOK,
		SCENE_AI_RANDOMFACEFLEX,
		SCENE_AI_RANDOMHEADFLEX,
		SCENE_AI_IGNORECOLLISION,
		SCENE_AI_DISABLEAI
	};


	DECLARE_DATADESC();
private:
	enum
	{
		HUMANOID_LATCHED_EYE	= 0x0001,
		HUMANOID_LATCHED_HEAD	= 0x0002,
		HUMANOID_LATCHED_ALL	= 0x0003,
	};

	//---------------------------------

	void					UpdateLatchedValues( void );

	// Input handlers.
	void InputSetExpressionOverride( inputdata_t &inputdata );

	//---------------------------------

	int						m_fLatchedPositions;
	Vector					m_latchedEyeOrigin;
	Vector 					m_latchedEyeDirection;		// direction eyes are looking
	Vector 					m_latchedHeadDirection;		// direction head is aiming

	void					ClearHeadAdjustment( void );
	Vector					m_goalHeadDirection;
	float					m_goalHeadInfluence;

	//---------------------------------

	float					m_goalSpineYaw;
	float					m_goalBodyYaw;
	Vector					m_goalHeadCorrection;

	//---------------------------------

	float					m_flBlinktime;
	EHANDLE					m_hLookTarget;
	CAI_InterestTarget		m_lookQueue;
	CAI_InterestTarget		m_syntheticLookQueue;

	CAI_InterestTarget		m_randomLookQueue;
	float					m_flNextRandomLookTime;	// FIXME: move to scene

	//---------------------------------

	string_t				m_iszExpressionScene;
	EHANDLE					m_hExpressionSceneEnt;
	float					m_flNextRandomExpressionTime;

	string_t				m_iszExpressionOverride;

protected:
	string_t				m_iszIdleExpression;
	string_t				m_iszAlertExpression;
	string_t				m_iszCombatExpression;
	string_t				m_iszDeathExpression;

private:
	//---------------------------------

	//PoseParameter_t			m_ParameterBodyTransY;		// "body_trans_Y"
	//PoseParameter_t			m_ParameterBodyTransX;		// "body_trans_X"
	//PoseParameter_t			m_ParameterBodyLift;		// "body_lift"
	PoseParameter_t			m_ParameterBodyYaw;			// "body_yaw"
	//PoseParameter_t			m_ParameterBodyPitch;		// "body_pitch"
	//PoseParameter_t			m_ParameterBodyRoll;		// "body_roll"
	PoseParameter_t			m_ParameterSpineYaw;		// "spine_yaw"
	//PoseParameter_t			m_ParameterSpinePitch;		// "spine_pitch"
	//PoseParameter_t			m_ParameterSpineRoll;		// "spine_roll"
	PoseParameter_t			m_ParameterNeckTrans;		// "neck_trans"
	PoseParameter_t			m_ParameterHeadYaw;			// "head_yaw"
	PoseParameter_t			m_ParameterHeadPitch;		// "head_pitch"
	PoseParameter_t			m_ParameterHeadRoll;		// "head_roll"

	//FlexWeight_t			m_FlexweightMoveRightLeft;	// "move_rightleft"
	//FlexWeight_t			m_FlexweightMoveForwardBack;// "move_forwardback"
	//FlexWeight_t			m_FlexweightMoveUpDown;		// "move_updown"
	FlexWeight_t			m_FlexweightBodyRightLeft;	// "body_rightleft"
	//FlexWeight_t			m_FlexweightBodyUpDown;		// "body_updown"
	//FlexWeight_t			m_FlexweightBodyTilt;		// "body_tilt"
	FlexWeight_t			m_FlexweightChestRightLeft;	// "chest_rightleft"
	//FlexWeight_t			m_FlexweightChestUpDown;	// "chest_updown"
	//FlexWeight_t			m_FlexweightChestTilt;		// "chest_tilt"
	FlexWeight_t			m_FlexweightHeadForwardBack;// "head_forwardback"
	FlexWeight_t			m_FlexweightHeadRightLeft;	// "head_rightleft"
	FlexWeight_t			m_FlexweightHeadUpDown;		// "head_updown"
	FlexWeight_t			m_FlexweightHeadTilt;		// "head_tilt"

	PoseParameter_t			m_ParameterGestureHeight;		// "gesture_height"
	PoseParameter_t			m_ParameterGestureWidth;		// "gesture_width"
	FlexWeight_t			m_FlexweightGestureUpDown;		// "gesture_updown"
	FlexWeight_t			m_FlexweightGestureRightLeft;	// "gesture_rightleft"

private:
	//---------------------------------
	bool RandomFaceFlex( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );
	bool RandomHeadFlex( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );
	float m_flextarget[64];

public:
	virtual bool UseSemaphore( void );

protected:
	bool	m_bDontUseSemaphore;

public:
	//---------------------------------
	//
	// Speech support
	//
	virtual CAI_Expresser *GetExpresser();

protected:
	bool CreateComponents();
	virtual CAI_Expresser *CreateExpresser();
private:
	//---------------------------------
	CAI_Expresser *m_pExpresser;
};

//-----------------------------------------------------------------------------
#endif // AI_BASEACTOR_H
