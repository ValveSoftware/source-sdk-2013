//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#if !defined( INPUT_H )
#define INPUT_H
#ifdef _WIN32
#pragma once
#endif

#include "iinput.h"
#include "mathlib/vector.h"
#include "kbutton.h"
#include "ehandle.h"
#include "inputsystem/AnalogCode.h"

typedef unsigned int CRC32_t;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CKeyboardKey
{
public:
	// Name for key
	char				name[ 32 ];
	// Pointer to the underlying structure
	kbutton_t			*pkey;
	// Next key in key list.
	CKeyboardKey		*next;
};

class ConVar;

class CInput : public IInput
{
// Interface
public:
							CInput( void );
							~CInput( void );

	virtual		void		Init_All( void );
	virtual		void		Shutdown_All( void );
	virtual		int			GetButtonBits( int );
	virtual		void		CreateMove ( int sequence_number, float input_sample_frametime, bool active );
	virtual		void		ExtraMouseSample( float frametime, bool active );
	virtual		bool		WriteUsercmdDeltaToBuffer( bf_write *buf, int from, int to, bool isnewcommand );
	virtual		void		EncodeUserCmdToBuffer( bf_write& buf, int slot );
	virtual		void		DecodeUserCmdFromBuffer( bf_read& buf, int slot );

	virtual		CUserCmd	*GetUserCmd( int sequence_number );

	virtual		void		MakeWeaponSelection( C_BaseCombatWeapon *weapon );

	virtual		float		KeyState( kbutton_t *key );
	virtual		int			KeyEvent( int down, ButtonCode_t keynum, const char *pszCurrentBinding );
	virtual		kbutton_t	*FindKey( const char *name );

	virtual		void		ControllerCommands( void );
	virtual		void		Joystick_Advanced( void );
	virtual		void		Joystick_SetSampleTime(float frametime);
	virtual		void		IN_SetSampleTime( float frametime );

	virtual		void		AccumulateMouse( void );
	virtual		void		ActivateMouse( void );
	virtual		void		DeactivateMouse( void );

	virtual		void		ClearStates( void );
	virtual		float		GetLookSpring( void );

	virtual		void		GetFullscreenMousePos( int *mx, int *my, int *unclampedx = NULL, int *unclampedy = NULL );
	virtual		void		SetFullscreenMousePos( int mx, int my );
	virtual		void		ResetMouse( void );

//	virtual		bool		IsNoClipping( void );
	virtual		float		GetLastForwardMove( void );
	virtual		float		Joystick_GetForward( void );
	virtual		float		Joystick_GetSide( void );
	virtual		float		Joystick_GetPitch( void );
	virtual		float		Joystick_GetYaw( void );
	virtual		void		ClearInputButton( int bits );

	virtual		void		CAM_Think( void );
	virtual		int			CAM_IsThirdPerson( void );
	virtual		void		CAM_ToThirdPerson(void);
	virtual		void		CAM_ToFirstPerson(void);
	virtual		void		CAM_StartMouseMove(void);
	virtual		void		CAM_EndMouseMove(void);
	virtual		void		CAM_StartDistance(void);
	virtual		void		CAM_EndDistance(void);
	virtual		int			CAM_InterceptingMouse( void );

	// orthographic camera info
	virtual		void		CAM_ToOrthographic();
	virtual		bool		CAM_IsOrthographic() const;
	virtual		void		CAM_OrthographicSize( float& w, float& h ) const;

	virtual		float		CAM_CapYaw( float fVal ) const { return fVal; }
	virtual		float		CAM_CapPitch( float fVal ) const { return fVal; }
	
#if defined( HL2_CLIENT_DLL )
	// IK back channel info
	virtual		void		AddIKGroundContactInfo( int entindex, float minheight, float maxheight );
#endif
	virtual		void		LevelInit( void );

	virtual		void		CAM_SetCameraThirdData( CameraThirdData_t *pCameraData, const QAngle &vecCameraOffset );
	virtual		void		CAM_CameraThirdThink( void );	

	virtual	bool		EnableJoystickMode();

// Private Implementation
protected:
	// Implementation specific initialization
	void		Init_Camera( void );
	void		Init_Keyboard( void );
	void		Init_Mouse( void );
	void		Shutdown_Keyboard( void );
	// Add a named key to the list queryable by the engine
	void		AddKeyButton( const char *name, kbutton_t *pkb );
	// Mouse/keyboard movement input helpers
	void		ScaleMovements( CUserCmd *cmd );
	void		ComputeForwardMove( CUserCmd *cmd );
	void		ComputeUpwardMove( CUserCmd *cmd );
	void		ComputeSideMove( CUserCmd *cmd );
	void		AdjustAngles ( float frametime );
	void		ClampAngles( QAngle& viewangles );
	void		AdjustPitch( float speed, QAngle& viewangles );
	virtual void AdjustYaw( float speed, QAngle& viewangles );
	float		DetermineKeySpeed( float frametime );
	void		GetAccumulatedMouseDeltasAndResetAccumulators( float *mx, float *my );
	void		GetMouseDelta( float inmousex, float inmousey, float *pOutMouseX, float *pOutMouseY );
	void		ScaleMouse( float *x, float *y );
	virtual void ApplyMouse( QAngle& viewangles, CUserCmd *cmd, float mouse_x, float mouse_y );
	virtual void MouseMove ( CUserCmd *cmd );

	// Joystick  movement input helpers
	void		ControllerMove ( float frametime, CUserCmd *cmd );
	void		JoyStickMove ( float frametime, CUserCmd *cmd );
	float		ScaleAxisValue( const float axisValue, const float axisThreshold );
	virtual float JoyStickAdjustYaw( float flSpeed ) { return flSpeed; }

	// Call this to get the cursor position. The call will be logged in the VCR file if there is one.
	void		GetMousePos(int &x, int &y);
	void		SetMousePos(int x, int y);
	void		GetWindowCenter( int&x, int& y );
	// Called once per frame to allow convar overrides to acceleration settings when mouse is active
	void		CheckMouseAcclerationVars();

	void		ValidateUserCmd( CUserCmd *usercmd, int sequence_number );

// Private Data
private:
	typedef struct
	{
		unsigned int AxisFlags;
		unsigned int AxisMap;
		unsigned int ControlMap;
	} joy_axis_t;

	void		DescribeJoystickAxis( char const *axis, joy_axis_t *mapping );
	char const	*DescribeAxis( int index );

	enum
	{
		GAME_AXIS_NONE = 0,
		GAME_AXIS_FORWARD,
		GAME_AXIS_PITCH,
		GAME_AXIS_SIDE,
		GAME_AXIS_YAW,
		MAX_GAME_AXES
	};

	enum
	{
		CAM_COMMAND_NONE = 0,
		CAM_COMMAND_TOTHIRDPERSON = 1,
		CAM_COMMAND_TOFIRSTPERSON = 2
	};

	enum
	{
		MOUSE_ACCEL_THRESHHOLD1 = 0,	// if mouse moves > this many mickey's double it
		MOUSE_ACCEL_THRESHHOLD2,		// if mouse moves > this many mickey's double it a second time
		MOUSE_SPEED_FACTOR,				// 0 = disabled, 1 = threshold 1 enabled, 2 = threshold 2 enabled

		NUM_MOUSE_PARAMS,
	};

	// Has the mouse been initialized?
	bool		m_fMouseInitialized;
	// Is the mosue active?
	bool		m_fMouseActive;
	// Has the joystick advanced initialization been run?
	bool		m_fJoystickAdvancedInit;
	// Used to support hotplugging by reinitializing the advanced joystick system when we toggle between some/none joysticks.
	bool		m_fHadJoysticks;

	// Accumulated mouse deltas
	float		m_flAccumulatedMouseXMovement;
	float		m_flAccumulatedMouseYMovement;
	float		m_flPreviousMouseXPosition;
	float		m_flPreviousMouseYPosition;
	float		m_flRemainingJoystickSampleTime;
	float		m_flKeyboardSampleTime;

	// Flag to restore systemparameters when exiting
	bool		m_fRestoreSPI;
	// Original mouse parameters
	int			m_rgOrigMouseParms[ NUM_MOUSE_PARAMS ];
	// Current mouse parameters.
	int			m_rgNewMouseParms[ NUM_MOUSE_PARAMS ];
	bool		m_rgCheckMouseParam[ NUM_MOUSE_PARAMS ];
	// Are the parameters valid
	bool		m_fMouseParmsValid;
	// Joystick Axis data
	joy_axis_t m_rgAxes[ MAX_JOYSTICK_AXES ];
	// List of queryable keys
	CKeyboardKey *m_pKeys;
	
	// Is the 3rd person camera using the mouse?
	bool		m_fCameraInterceptingMouse;
	// Are we in 3rd person view?
	bool		m_fCameraInThirdPerson;
	// Should we move view along with mouse?
	bool		m_fCameraMovingWithMouse;

	
	// Is the camera in distance moving mode?
	bool		m_fCameraDistanceMove;
	// Old and current mouse position readings.
	int			m_nCameraOldX;
	int			m_nCameraOldY;
	int			m_nCameraX;
	int			m_nCameraY;

	// orthographic camera settings
	bool		m_CameraIsOrthographic;

	QAngle		m_angPreviousViewAngles;

	float		m_flLastForwardMove;

	float m_flPreviousJoystickForward;
	float m_flPreviousJoystickSide;
	float m_flPreviousJoystickPitch;
	float m_flPreviousJoystickYaw;

	class CVerifiedUserCmd
	{
	public:
		CUserCmd	m_cmd;
		CRC32_t		m_crc;
	};
				
	CUserCmd	*m_pCommands;
	CVerifiedUserCmd *m_pVerifiedCommands;

	CameraThirdData_t	*m_pCameraThirdData;

	// Set until polled by CreateMove and cleared
	CHandle< C_BaseCombatWeapon > m_hSelectedWeapon;

#if defined( HL2_CLIENT_DLL )
	CUtlVector< CEntityGroundContact > m_EntityGroundContact;
#endif
};

extern kbutton_t in_strafe;
extern kbutton_t in_speed;
extern kbutton_t in_jlook;
extern kbutton_t in_graph;  
extern kbutton_t in_moveleft;
extern kbutton_t in_moveright;
extern kbutton_t in_forward;
extern kbutton_t in_back;
extern kbutton_t in_joyspeed;

extern class ConVar in_joystick;
extern class ConVar joy_autosprint;

extern void KeyDown( kbutton_t *b, const char *c );
extern void KeyUp( kbutton_t *b, const char *c );


#endif // INPUT_H
	