//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Single Player animation state 'handler'. This utility is used
//            to evaluate the pose parameter value based on the direction
//            and speed of the player.
// 
// ------------------------------------------------------------------------------
//
// This was originally based on the following VDC article:
// https://developer.valvesoftware.com/wiki/Fixing_the_player_animation_state_(Single_Player)
//
// It has been modified by Blixibon to derive from CBasePlayerAnimState instead and support 9-way blends.
// Much of the work done to make this derive from CBasePlayerAnimState utilized code from the Alien Swarm SDK.
// 
//=============================================================================//

#ifndef SINGLEPLAYER_ANIMSTATE_H
#define SINGLEPLAYER_ANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "base_playeranimstate.h"

#ifdef CLIENT_DLL
#include "c_baseplayer.h"
#else
#include "player.h"
#endif

#ifdef MAPBASE
// Special definition for differentiating between SP and HL2:DM anim states
#define SP_ANIM_STATE 1
#endif

class CSinglePlayerAnimState : public CBasePlayerAnimState
{
public:
    CSinglePlayerAnimState( CBasePlayer *pPlayer );
    
	Activity CalcMainActivity();
	int CalcAimLayerSequence( float *flCycle, float *flAimSequenceWeight, bool bForceIdle );
    float GetCurrentMaxGroundSpeed();

    void SetPlayerAnimation( PLAYER_ANIM playerAnim );
    Activity TranslateActivity( Activity actDesired );

	void ComputeSequences( CStudioHdr *pStudioHdr );

	void AddMiscSequence( int iSequence, float flBlendIn = 0.0f, float flBlendOut = 0.0f, float flPlaybackRate = 1.0f, bool bHoldAtEnd = false, bool bOnlyWhenStill = false );

    void ClearAnimationState();
    void ClearAnimationLayers();

private:

    bool HandleJumping();

	void ComputeFireSequence();
	void ComputeReloadSequence();
	void ComputeWeaponSwitchSequence();
	void ComputeMiscSequence();

	void UpdateLayerSequenceGeneric( int iLayer, bool &bEnabled, float &flCurCycle,
									int &iSequence, bool bWaitAtEnd,
									float fBlendIn=0.15f, float fBlendOut=0.15f, bool bMoveBlend = false, 
									float fPlaybackRate=1.0f, bool bUpdateCycle = true );

    void                ComputePoseParam_BodyYaw( void );
    void                ComputePoseParam_BodyPitch( CStudioHdr *pStudioHdr );
    void                ComputePoseParam_BodyLookYaw( void );
    void                ComputePoseParam_HeadPitch( CStudioHdr *pStudioHdr );

    CBasePlayer* m_pPlayer;

    // Current state variables.
    bool m_bJumping;			// Set on a jump event.
    float m_flJumpStartTime;
    bool m_bFirstJumpFrame;

	// Aim sequence plays reload while this is on.
	bool m_bReloading;
	float m_flReloadCycle;
	int m_iReloadSequence;
	float m_flReloadBlendOut, m_flReloadBlendIn;
	float m_fReloadPlaybackRate;

	bool m_bWeaponSwitching;
	float m_flWeaponSwitchCycle;
	int m_iWeaponSwitchSequence;

	bool m_bPlayingMisc;
	float m_flMiscCycle, m_flMiscBlendOut, m_flMiscBlendIn;
	int m_iMiscSequence;
	bool m_bMiscOnlyWhenStill, m_bMiscHoldAtEnd;
	bool m_bMiscNoOverride;
	float m_fMiscPlaybackRate;
	bool m_bMiscCycleRewound;
	float m_flMiscRewindCycle;
	// This is set to true if ANY animation is being played in the fire layer.
	bool m_bFiring;						// If this is on, then it'll continue the fire animation in the fire layer
										// until it completes.
	int m_iFireSequence;				// (For any sequences in the fire layer, including grenade throw).
	float m_flFireCycle;
};

CSinglePlayerAnimState *CreatePlayerAnimationState( CBasePlayer *pPlayer );

#endif // SINGLEPLAYER_ANIMSTATE_H
