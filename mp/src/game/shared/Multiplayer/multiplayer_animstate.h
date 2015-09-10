//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#ifndef MULTIPLAYERANIMSTATE_H
#define MULTIPLAYERANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"
#include "basecombatweapon_shared.h"
#include "iplayeranimstate.h"

#if defined( CLIENT_DLL )
class C_BasePlayer;
#define CPlayer C_BasePlayer
#else
class CBasePlayer;
#endif

enum PlayerAnimEvent_t
{
	PLAYERANIMEVENT_ATTACK_PRIMARY,
	PLAYERANIMEVENT_ATTACK_SECONDARY,
	PLAYERANIMEVENT_ATTACK_GRENADE,
	PLAYERANIMEVENT_RELOAD,
	PLAYERANIMEVENT_RELOAD_LOOP,
	PLAYERANIMEVENT_RELOAD_END,
	PLAYERANIMEVENT_JUMP,
	PLAYERANIMEVENT_SWIM,
	PLAYERANIMEVENT_DIE,
	PLAYERANIMEVENT_FLINCH_CHEST,
	PLAYERANIMEVENT_FLINCH_HEAD,
	PLAYERANIMEVENT_FLINCH_LEFTARM,
	PLAYERANIMEVENT_FLINCH_RIGHTARM,
	PLAYERANIMEVENT_FLINCH_LEFTLEG,
	PLAYERANIMEVENT_FLINCH_RIGHTLEG,
	PLAYERANIMEVENT_DOUBLEJUMP,

	// Cancel.
	PLAYERANIMEVENT_CANCEL,
	PLAYERANIMEVENT_SPAWN,

	// Snap to current yaw exactly
	PLAYERANIMEVENT_SNAP_YAW,

	PLAYERANIMEVENT_CUSTOM,				// Used to play specific activities
	PLAYERANIMEVENT_CUSTOM_GESTURE,
	PLAYERANIMEVENT_CUSTOM_SEQUENCE,	// Used to play specific sequences
	PLAYERANIMEVENT_CUSTOM_GESTURE_SEQUENCE,

	// TF Specific. Here until there's a derived game solution to this.
	PLAYERANIMEVENT_ATTACK_PRE,
	PLAYERANIMEVENT_ATTACK_POST,
	PLAYERANIMEVENT_GRENADE1_DRAW,
	PLAYERANIMEVENT_GRENADE2_DRAW,
	PLAYERANIMEVENT_GRENADE1_THROW,
	PLAYERANIMEVENT_GRENADE2_THROW,
	PLAYERANIMEVENT_VOICE_COMMAND_GESTURE,
	PLAYERANIMEVENT_DOUBLEJUMP_CROUCH,
	PLAYERANIMEVENT_STUN_BEGIN,
	PLAYERANIMEVENT_STUN_MIDDLE,
	PLAYERANIMEVENT_STUN_END,
	PLAYERANIMEVENT_PASSTIME_THROW_BEGIN,
	PLAYERANIMEVENT_PASSTIME_THROW_MIDDLE,
	PLAYERANIMEVENT_PASSTIME_THROW_END,
	PLAYERANIMEVENT_PASSTIME_THROW_CANCEL,

	PLAYERANIMEVENT_ATTACK_PRIMARY_SUPER,

	PLAYERANIMEVENT_COUNT
};

// Gesture Slots.
enum
{
	GESTURE_SLOT_ATTACK_AND_RELOAD,
	GESTURE_SLOT_GRENADE,
	GESTURE_SLOT_JUMP,
	GESTURE_SLOT_SWIM,
	GESTURE_SLOT_FLINCH,
	GESTURE_SLOT_VCD,
	GESTURE_SLOT_CUSTOM,

	GESTURE_SLOT_COUNT,
};

#define GESTURE_SLOT_INVALID	-1

struct GestureSlot_t
{
	int					m_iGestureSlot;
	Activity			m_iActivity;
	bool				m_bAutoKill;
	bool				m_bActive;
	CAnimationLayer		*m_pAnimLayer;
};

inline bool IsCustomPlayerAnimEvent( PlayerAnimEvent_t event )
{
	return ( event == PLAYERANIMEVENT_CUSTOM ) || ( event == PLAYERANIMEVENT_CUSTOM_GESTURE ) ||
		( event == PLAYERANIMEVENT_CUSTOM_SEQUENCE ) || ( event == PLAYERANIMEVENT_CUSTOM_GESTURE_SEQUENCE );
}

struct MultiPlayerPoseData_t
{
	int			m_iMoveX;
	int			m_iMoveY;
	int			m_iAimYaw;
	int			m_iAimPitch;
	int			m_iBodyHeight;
	int			m_iMoveYaw;
	int			m_iMoveScale;

	float		m_flEstimateYaw;
	float		m_flLastAimTurnTime;

	void Init()
	{
		m_iMoveX = 0;
		m_iMoveY = 0;
		m_iAimYaw = 0;
		m_iAimPitch = 0;
		m_iBodyHeight = 0;
		m_iMoveYaw = 0;
		m_iMoveScale = 0;
		m_flEstimateYaw = 0.0f;
		m_flLastAimTurnTime = 0.0f;
	}
};

struct DebugPlayerAnimData_t
{
	float		m_flSpeed;
	float		m_flAimPitch;
	float		m_flAimYaw;
	float		m_flBodyHeight;
	Vector2D	m_vecMoveYaw;

	void Init()
	{
		m_flSpeed = 0.0f;
		m_flAimPitch = 0.0f;
		m_flAimYaw = 0.0f;
		m_flBodyHeight = 0.0f;
		m_vecMoveYaw.Init();
	}
};

struct MultiPlayerMovementData_t
{
	// Set speeds to -1 if they are not used.
	float		m_flWalkSpeed;
	float		m_flRunSpeed;
	float		m_flSprintSpeed;	
	float		m_flBodyYawRate;
};


//=============================================================================
//
// Multi-Player Animation State
//
class CMultiPlayerAnimState
{
public:

	DECLARE_CLASS_NOBASE( CMultiPlayerAnimState );

	// Creation/Destruction
	CMultiPlayerAnimState() {}
	CMultiPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData );
	virtual ~CMultiPlayerAnimState();

	// This is called by both the client and the server in the same way to trigger events for
	// players firing, jumping, throwing grenades, etc.
	virtual void ClearAnimationState();
	virtual void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );
	virtual Activity CalcMainActivity();	
	virtual void Update( float eyeYaw, float eyePitch );
	virtual void Release( void );

	const QAngle &GetRenderAngles();

	virtual Activity TranslateActivity( Activity actDesired );

	virtual void SetRunSpeed( float flSpeed ) { m_MovementData.m_flRunSpeed = flSpeed; }
	virtual void SetWalkSpeed( float flSpeed ) { m_MovementData.m_flWalkSpeed = flSpeed; }
	virtual void SetSprintSpeed( float flSpeed ) { m_MovementData.m_flSprintSpeed = flSpeed; }

	// Debug
	virtual void ShowDebugInfo( void );
	virtual void DebugShowAnimState( int iStartLine );

	Activity GetCurrentMainActivity( void ) { return m_eCurrentMainSequenceActivity; }

	void OnNewModel( void );

	// Gestures.
	void	ResetGestureSlots( void );
	void	ResetGestureSlot( int iGestureSlot );
	void AddVCDSequenceToGestureSlot( int iGestureSlot, int iGestureSequence, float flCycle = 0.0f, bool bAutoKill = true );
	CAnimationLayer* GetGestureSlotLayer( int iGestureSlot );
	bool	IsGestureSlotActive( int iGestureSlot );
	bool	VerifyAnimLayerInSlot( int iGestureSlot );

	// Feet.
	// If you are forcing aim yaw, your code is almost definitely broken if you don't include a delay between 
	// teleporting and forcing yaw. This is due to an unfortunate interaction between the command lookback window,
	// and the fact that m_flEyeYaw is never propogated from the server to the client.
	// TODO: Fix this after Halloween 2014.
	bool	m_bForceAimYaw;

protected:

	virtual void Init( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData ); 
	CBasePlayer *GetBasePlayer( void )				{ return m_pPlayer; }

	// Allow inheriting classes to override SelectWeightedSequence
	virtual int SelectWeightedSequence( Activity activity ) { return GetBasePlayer()->SelectWeightedSequence( activity ); }
	virtual void RestartMainSequence();

	virtual void GetOuterAbsVelocity( Vector& vel );
	float GetOuterXYSpeed();

	virtual bool HandleJumping( Activity &idealActivity );
	virtual bool HandleDucking( Activity &idealActivity );
	virtual bool HandleMoving( Activity &idealActivity );
	virtual bool HandleSwimming( Activity &idealActivity );
	virtual bool HandleDying( Activity &idealActivity );

	// Gesture Slots
	CUtlVector<GestureSlot_t>		m_aGestureSlots;
	bool	InitGestureSlots( void );
	void	ShutdownGestureSlots( void );
	bool	IsGestureSlotPlaying( int iGestureSlot, Activity iGestureActivity );
	void	AddToGestureSlot( int iGestureSlot, Activity iGestureActivity, bool bAutoKill );
	virtual void RestartGesture( int iGestureSlot, Activity iGestureActivity, bool bAutoKill = true );
	void	ComputeGestureSequence( CStudioHdr *pStudioHdr );
	void	UpdateGestureLayer( CStudioHdr *pStudioHdr, GestureSlot_t *pGesture );
	void	DebugGestureInfo( void );
	virtual float	GetGesturePlaybackRate( void ) { return 1.0f; }

#ifdef CLIENT_DLL
	void	RunGestureSlotAnimEventsToCompletion( GestureSlot_t *pGesture );
#endif

	virtual void PlayFlinchGesture( Activity iActivity );

	virtual float CalcMovementSpeed( bool *bIsMoving );
	virtual float CalcMovementPlaybackRate( bool *bIsMoving );

	void DoMovementTest( CStudioHdr *pStudioHdr, float flX, float flY );
	void DoMovementTest( CStudioHdr *pStudioHdr );
	void GetMovementFlags( CStudioHdr *pStudioHdr );

	// Pose parameters.
	bool				SetupPoseParameters( CStudioHdr *pStudioHdr );
	virtual void		ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr );
	virtual void		ComputePoseParam_AimPitch( CStudioHdr *pStudioHdr );
	virtual void		ComputePoseParam_AimYaw( CStudioHdr *pStudioHdr );
	void				ComputePoseParam_BodyHeight( CStudioHdr *pStudioHdr );
	virtual void		EstimateYaw( void );
	void				ConvergeYawAngles( float flGoalYaw, float flYawRate, float flDeltaTime, float &flCurrentYaw );

	virtual float GetCurrentMaxGroundSpeed();
	virtual void ComputeSequences( CStudioHdr *pStudioHdr );
	void ComputeMainSequence();
	void UpdateInterpolators();
	void ResetGroundSpeed( void );
	float GetInterpolatedGroundSpeed( void );

	void ComputeFireSequence();
	void ComputeDeployedSequence();

	virtual bool ShouldUpdateAnimState();

	void				DebugShowAnimStateForPlayer( bool bIsServer );
	void				DebugShowEyeYaw( void );

// Client specific.
#ifdef CLIENT_DLL

	// Debug.
	void				DebugShowActivity( Activity activity );

#endif

protected:

	CBasePlayer	*m_pPlayer;

	QAngle				m_angRender;

	// Pose parameters.
	bool						m_bPoseParameterInit;
	MultiPlayerPoseData_t		m_PoseParameterData;
	DebugPlayerAnimData_t		m_DebugAnimData;

	bool						m_bCurrentFeetYawInitialized;
	float						m_flLastAnimationStateClearTime;
	
	float m_flEyeYaw;
	float m_flEyePitch;
	float m_flGoalFeetYaw;
	float m_flCurrentFeetYaw;
	float m_flLastAimTurnTime;

	MultiPlayerMovementData_t	m_MovementData;

	// Jumping.
	bool	m_bJumping;
	float	m_flJumpStartTime;	
	bool	m_bFirstJumpFrame;

	// Swimming.
	bool	m_bInSwim;
	bool	m_bFirstSwimFrame;

	// Dying
	bool	m_bDying;
	bool	m_bFirstDyingFrame;

	// Last activity we've used on the lower body. Used to determine if animations should restart.
	Activity m_eCurrentMainSequenceActivity;	

	// Specific full-body sequence to play
	int		m_nSpecificMainSequence;

	// Weapon data.
	CHandle<CBaseCombatWeapon>	m_hActiveWeapon;

	// Ground speed interpolators.
#ifdef CLIENT_DLL
	float m_flLastGroundSpeedUpdateTime;
	CInterpolatedVar<float> m_iv_flMaxGroundSpeed;
#endif
	float m_flMaxGroundSpeed;

	// movement playback options
	int m_nMovementSequence;
	LegAnimType_t m_LegAnimType;
};

// If this is set, then the game code needs to make sure to send player animation events
// to the local player if he's the one being watched.
extern ConVar cl_showanimstate;

#endif // DOD_PLAYERANIMSTATE_H