//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef IN_SIXENSE_H
#define IN_SIXENSE_H

#include "mathlib/vector.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/Label.h"
#include "vgui_video.h"
#include "convar.h"
#include "usercmd.h"
#include "tier1/utlstring.h"
#include "GameEventListener.h"
#include "in_sixense_gesture_bindings.h"


#if defined( CLIENT_DLL )
class C_Portal_Player;
#define CPortal_Player C_Portal_Player
#else
class CPortal_Player;
#endif

// forward declarations
namespace sixenseUtils {
	class IFPSViewAngles;
	class IFPSPlayerMovement;
	class IFPSEvents;
	class IFPSMeleeWeapon;

	class IDerivatives;
	class IButtonStates;
	class ILaserPointer;

	class IControllerManager;
};

class SixenseInput : public CGameEventListener
{

public:
	bool LoadModules();
	bool UnloadModules();

	SixenseInput();
	~SixenseInput();


	void Init();
	void PostInit();
	void Shutdown();
#ifdef PORTAL2
	bool IsBaseWarningUp();
	void PlayerPortalled( const VMatrix &PortalMatrix );
	void SetOneToOneMode( bool bOnOrOff );

	inline QAngle GetAnglesToRightHand() { return m_AnglesToRightHand; }
	inline QAngle GetAnglesToLeftHand() { return m_AnglesToLeftHand; }

	// If the right trigger is held and a melee weapon is selected, go to 1-to-1 melee mode
	bool IsInOneToOneMode();
	bool IsInAlwaysOneToOneMode();

	void FixPortalView();
	void SetPortalTweakingParameters( bool bIsTweaking );

	bool IsHoldingObject();

	C_BaseEntity *GetHeldObject();
#endif

	bool IsEnabled();
	bool IsLeftHanded();

	// SixenseFrame computes the view parameters from the controllers. Should be called once per frame
	bool SixenseFrame( float flFrametime, CUserCmd *pCmd );
	void SixenseUpdateKeys( float flFrametime, CUserCmd *pCmd );

	bool SendKeyToActiveWindow(ButtonCode_t key);
	void SixenseUpdateMouseCursor();
	void SixenseUpdateControllerManager();
	void controllerManagerCallback( int );
	inline void ResetFrameTime( float flTime ) { m_fRemainingFrameTime = flTime; }

	// Set the engine's view angles
	void SetView( float flInputSampleFrametime, CUserCmd *pCmd );
	void SetMode( int nMode );
	void ResetView( QAngle SpawnAngles );
	void SetEnabled( bool bEnabled );
	void LoadDefaultSettings( int nLevel );
	bool InMenuMode();


	QAngle GetViewAngles();
	QAngle GetViewAngleOffset();

	void ForceViewAngles( QAngle angles );
	bool IsSixenseMap();
	void CreateGUI( vgui::VPANEL parent );
	void SwitchViewModes( CUserCmd *pCmd );

	// playerIndex = (0 to 3), handIndex = (left = 0 or right = 1)
	void Rumble( unsigned char nIndex, unsigned char nRumbleData, unsigned char nRumbleFlags );
	void Rumble( unsigned char nPlayerIndex, unsigned char nHandIndex, unsigned char nRumbleData, unsigned char nRumbleFlags );

	void SetFilter( float f );
	void GetFOV( float *pHfov, float *pVfov );

#ifdef SIXENSE_PLAYER_DATA
	void SetPlayerHandPositions( CUserCmd *pCmd, float flFrametime );
#endif

	void SetBaseOffset();
	void SetFilterLevel( float flNearRange, float flNearVal, float flFarRange, float flFarVal );

	static class SixenseGUIFrame *m_SixenseFrame;

	bool IsAimingForwards();



	virtual void FireGameEvent( IGameEvent *pEvent );

	void BlendView();

	void DisableFreeAimSpin( int nDisable );
	void DisableGestures( int nDisable );
	void PlayerSpawn();
	bool AreBindingsDisabled();

	void LeftPointGesture( bool start );
	void RightPointGesture( bool start );

	void StartRatchet();
	void StopRatchet();

	void CheckWeaponForScope();

	SixenseGestureBindings *GetGestureBindings();

	void InstallConvarCallbacks();
	void UpdateValuesFromConvars();
	void ConvarChanged();

private:

	bool m_bIsEnabled; // sixense.dll loaded
	bool m_bIsActive; // controllers not docked

	bool m_bModulesLoaded;

	bool m_bWasInMenuMode;

#ifdef PORTAL2
	bool m_bJustPortalled;

	bool m_bIsLeftTriggerDown;
	bool m_bIsRightTriggerDown;

	bool m_bIsIn1to1Mode;
	bool m_bIs1to1ModeLocked;
	bool m_bIs1to1ModeScaling;
	bool m_bIs1to1ModeRatcheting;

	bool m_bExitOneWhenAimingForwards;
	bool m_bScalingLockedOneToOne;

	bool m_bIsTweaking;

	float m_fDisableJumpUntil;

	int m_nGlowIndex;

	float m_fLastHorizSpeedMult;
	float m_fLastVertSpeedMult;

	QAngle m_AnglesToRightHand, m_AnglesToLeftHand;

	float m_fTweakSixenseAimFreeaimAccelBandExponent;
	float m_fTweakSixenseAimFreeaimAutoLevelRate;
	float m_fTweakSixenseAimFreeaimAccelBandSize;
	float m_fTweakSixenseAimFreeaimMaxSpeed;
	float m_fTweakSixenseAimFreeaimDeadZoneRadius;
	float m_fTweakSixenseAimFreeaimHeadingMultiplier;
	float m_fTweakSixenseAimFreeaimPitchMultiplier;
	float m_fTweakSixenseAim1to1HeadingMultiplier;
	float m_fTweakSixenseAim1to1PitchMultiplier;

	Vector3 m_GrabPos;
#endif

	bool m_bConvarChanged;

	bool m_bPlayerValid;
	
	float m_fRemainingFrameTime;

	bool m_bScopeSwitchedMode;
	sixenseUtils::IFPSViewAngles::fps_mode m_nScopeSwitchedPrevMode;
	int m_nScopeSwitchedPrevSpringViewEnabled;

	float m_fTeleportWaitToBlendTime;

	class ISixenseAPI *m_pSixenseAPI;

	struct _sixenseAllControllerData *m_pACD;

	class sixenseUtils::IFPSViewAngles *m_pFPSViewAngles;
	class sixenseUtils::IFPSPlayerMovement *m_pFPSPlayerMovement;
	class sixenseUtils::IFPSEvents *m_pFPSEvents;

	class sixenseUtils::IDerivatives *m_pLeftDeriv, *m_pRightDeriv;
	class sixenseUtils::IButtonStates *m_pLeftButtonStates, *m_pRightButtonStates;
	class sixenseUtils::ILaserPointer *m_pLaserPointer;

	class sixenseUtils::IControllerManager *m_pControllerManager;

	int m_LastViewMode;
	int m_nLeftIndex, m_nRightIndex;

	void PlayerDroppedEntity( int entityID );
	void PlayerUsedEntity( int entityID );

	bool m_bMoveMouseToCenter;
	int m_nFilterLevel;
	unsigned char m_nLastLeftSequence, m_nLastRightSequence;
	bool m_bShouldSetBaseOffset;
	bool m_bJustSpawned;

#ifdef WATERMARK
	class SixenseWatermarkFrame *m_WatermarkFrame;
#endif

	int m_nFreeaimSpinDisabled;
	int m_nGesturesDisabled;

	bool m_nShouldUnduck;

	SixenseGestureBindings *m_pGestureBindings;
};

extern SixenseInput *g_pSixenseInput;


class SixenseGUIFrame : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( SixenseGUIFrame, vgui::Frame );

public:

	// Construction
	SixenseGUIFrame( vgui::VPANEL parent, char const *pPanelName );
	virtual ~SixenseGUIFrame();

	void setImage( CUtlString img_name );

	virtual void SetVisible( bool bState );

private:

	vgui::ImagePanel *m_ImagePanel;

};

#ifdef PORTAL2
class SixenseBaseWarning : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( SixenseBaseWarning, vgui::Frame );
public:
	SixenseBaseWarning( vgui::Panel *parent, char const *name );
	//virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
protected:
	//virtual void PaintBackground();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	vgui::Label *_label;
};
#endif

#endif
