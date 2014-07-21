//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "hud.h"
#include "kbutton.h"
#include "input.h"

#include <vgui/IInput.h>
#include "vgui_controls/Controls.h"
#include "tier0/vprof.h"
#include "debugoverlay_shared.h"
#include "cam_thirdperson.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-------------------------------------------------- Global Variables

static ConVar cam_command( "cam_command", "0", FCVAR_CHEAT | FCVAR_CHEAT);	 // tells camera to go to thirdperson
static ConVar cam_snapto( "cam_snapto", "0", FCVAR_ARCHIVE | FCVAR_CHEAT);	 // snap to thirdperson view
static ConVar cam_ideallag( "cam_ideallag", "4.0", FCVAR_ARCHIVE| FCVAR_CHEAT, "Amount of lag used when matching offset to ideal angles in thirdperson view" );
static ConVar cam_idealdelta( "cam_idealdelta", "4.0", FCVAR_ARCHIVE| FCVAR_CHEAT, "Controls the speed when matching offset to ideal angles in thirdperson view" );
ConVar cam_idealyaw( "cam_idealyaw", "0", FCVAR_ARCHIVE| FCVAR_CHEAT );	 // thirdperson yaw
ConVar cam_idealpitch( "cam_idealpitch", "0", FCVAR_ARCHIVE | FCVAR_CHEAT  );	 // thirperson pitch
ConVar cam_idealdist( "cam_idealdist", "150", FCVAR_ARCHIVE | FCVAR_CHEAT );	 // thirdperson distance
ConVar cam_idealdistright( "cam_idealdistright", "0", FCVAR_ARCHIVE | FCVAR_CHEAT );	 // thirdperson distance
ConVar cam_idealdistup( "cam_idealdistup", "0", FCVAR_ARCHIVE | FCVAR_CHEAT );	 // thirdperson distance
static ConVar cam_collision( "cam_collision", "1", FCVAR_ARCHIVE | FCVAR_CHEAT, "When in thirdperson and cam_collision is set to 1, an attempt is made to keep the camera from passing though walls." );
static ConVar cam_showangles( "cam_showangles", "0", FCVAR_CHEAT, "When in thirdperson, print viewangles/idealangles/cameraoffsets to the console." );
static ConVar c_maxpitch( "c_maxpitch", "90", FCVAR_ARCHIVE| FCVAR_CHEAT );
static ConVar c_minpitch( "c_minpitch", "0", FCVAR_ARCHIVE| FCVAR_CHEAT );
static ConVar c_maxyaw( "c_maxyaw",   "135", FCVAR_ARCHIVE | FCVAR_CHEAT);
static ConVar c_minyaw( "c_minyaw",   "-135", FCVAR_ARCHIVE| FCVAR_CHEAT );
static ConVar c_maxdistance( "c_maxdistance",   "200", FCVAR_ARCHIVE| FCVAR_CHEAT );
static ConVar c_mindistance( "c_mindistance",   "30", FCVAR_ARCHIVE| FCVAR_CHEAT );
static ConVar c_orthowidth( "c_orthowidth",   "100", FCVAR_ARCHIVE| FCVAR_CHEAT );
static ConVar c_orthoheight( "c_orthoheight",   "100", FCVAR_ARCHIVE | FCVAR_CHEAT );

static kbutton_t cam_pitchup, cam_pitchdown, cam_yawleft, cam_yawright;
static kbutton_t cam_in, cam_out; // -- "cam_move" is unused

extern ConVar cl_thirdperson;


// API Wrappers

/*
==============================
CAM_ToThirdPerson

==============================
*/
void CAM_ToThirdPerson(void)
{
	if ( cl_thirdperson.GetBool() == false )
	{
		g_ThirdPersonManager.SetOverridingThirdPerson( true );
	}

	input->CAM_ToThirdPerson();

	// Let the local player know
	C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();
	if ( localPlayer )
	{
		localPlayer->ThirdPersonSwitch( true );
	}
}

/*
==============================
CAM_ToThirdPerson_MayaMode

==============================
*/
static bool & Is_CAM_ThirdPerson_MayaMode(void)
{
	static bool s_b_CAM_ThirdPerson_MayaMode = false;
	return s_b_CAM_ThirdPerson_MayaMode;
}
void CAM_ToThirdPerson_MayaMode(void)
{
	bool &rb = Is_CAM_ThirdPerson_MayaMode();
	rb = !rb;
}

/*
==============================
CAM_ToFirstPerson

==============================
*/
void CAM_ToFirstPerson(void) 
{ 
	C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();
	if ( localPlayer && !localPlayer->CanUseFirstPersonCommand() )
		return;

	if ( cl_thirdperson.GetBool() == false )
	{
		g_ThirdPersonManager.SetOverridingThirdPerson( false );
	}

	input->CAM_ToFirstPerson();

	// Let the local player know
	if ( localPlayer )
	{
		localPlayer->ThirdPersonSwitch( false );
	}
}

/*
==============================
CAM_ToOrthographic

==============================
*/
void CAM_ToOrthographic(void) 
{ 
	input->CAM_ToOrthographic();
}

/*
==============================
CAM_StartMouseMove

==============================
*/
void CAM_StartMouseMove( void )
{
	input->CAM_StartMouseMove();
}

/*
==============================
CAM_EndMouseMove

==============================
*/
void CAM_EndMouseMove( void )
{
	input->CAM_EndMouseMove();
}

/*
==============================
CAM_StartDistance

==============================
*/
void CAM_StartDistance( void )
{
	input->CAM_StartDistance();
}

/*
==============================
CAM_EndDistance

==============================
*/
void CAM_EndDistance( void )
{
	input->CAM_EndDistance();
}

/*
==============================
CAM_ToggleSnapto

==============================
*/
void CAM_ToggleSnapto( void )
{ 
	cam_snapto.SetValue( !cam_snapto.GetInt() );
}


/*
==============================
MoveToward

==============================
*/
float MoveToward( float cur, float goal, float lag )
{
	if( cur != goal )
	{
		if( abs( cur - goal ) > 180.0 )
		{
			if( cur < goal )
				cur += 360.0;
			else
				cur -= 360.0;
		}

		if( cur < goal )
		{
			if( cur < goal - 1.0 )
				cur += ( goal - cur ) / lag;
			else
				cur = goal;
		}
		else
		{
			if( cur > goal + 1.0 )
				cur -= ( cur - goal ) / lag;
			else
				cur = goal;
		}
	}


	// bring cur back into range
	if( cur < 0 )
		cur += 360.0;
	else if( cur >= 360 )
		cur -= 360;

	return cur;
}

/*
==============================
CAM_Think

==============================
*/
void CInput::CAM_Think( void )
{
	VPROF("CAM_Think");
	//
	if ( m_pCameraThirdData )
	{
		return CAM_CameraThirdThink();
	}

	Vector idealAngles;
	Vector camOffset;
	float flSensitivity;
	QAngle viewangles;
	
	switch( cam_command.GetInt() )
	{
	case CAM_COMMAND_TOTHIRDPERSON:
		CAM_ToThirdPerson();
		break;
		
	case CAM_COMMAND_TOFIRSTPERSON:
		CAM_ToFirstPerson();
		break;
		
	case CAM_COMMAND_NONE:
	default:
		break;
	}

	g_ThirdPersonManager.Update();

	if( !m_fCameraInThirdPerson )
		return;

	// In Maya-mode
	if ( Is_CAM_ThirdPerson_MayaMode() )
	{
		// Unless explicitly moving the camera, don't move it
		m_fCameraInterceptingMouse = m_fCameraMovingWithMouse =
			vgui::input()->IsKeyDown( KEY_LALT ) || vgui::input()->IsKeyDown( KEY_RALT );
		if ( !m_fCameraMovingWithMouse )
			return;

		// Zero-out camera-control kbutton_t structures
		memset( &cam_pitchup, 0, sizeof( cam_pitchup ) );
		memset( &cam_pitchdown, 0, sizeof( cam_pitchdown ) );
		memset( &cam_yawleft, 0, sizeof( cam_yawleft ) );
		memset( &cam_yawright, 0, sizeof( cam_yawright ) );
		memset( &cam_in, 0, sizeof( cam_in ) );
		memset( &cam_out, 0, sizeof( cam_out ) );

		// Unless left or right mouse button is down, don't do anything
#ifndef _XBOX
		if ( /* Left+Middle Button Down */ vgui::input()->IsMouseDown( MOUSE_LEFT ) && vgui::input()->IsMouseDown( MOUSE_MIDDLE ) )
		{
			// Do only zoom in/out camera adjustment
			m_fCameraDistanceMove = true;
		}
		else if ( /* Left Button Down */ vgui::input()->IsMouseDown( MOUSE_LEFT ) )
		{
			// Do only rotational camera movement
			m_fCameraDistanceMove = false;
		}
		else if ( /* Right Button Down */ vgui::input()->IsMouseDown( MOUSE_RIGHT ) )
		{
			// Do only zoom in/out camera adjustment
			m_fCameraDistanceMove = true;
		}
		else
		{
			// Neither left or right buttons down, don't do anything
			ResetMouse();
			return;
		}
#endif
	}
	
	idealAngles[ PITCH ] = cam_idealpitch.GetFloat();
	idealAngles[ YAW ]   = cam_idealyaw.GetFloat();
	idealAngles[ DIST ]  = cam_idealdist.GetFloat();

	//
	//movement of the camera with the mouse
	//
	if ( m_fCameraMovingWithMouse )
	{
		int cpx, cpy;
#ifndef _XBOX		
		//get windows cursor position
		GetMousePos (cpx, cpy);
#else
		//xboxfixme
		cpx = cpy = 0;
#endif
		
		m_nCameraX = cpx;
		m_nCameraY = cpy;
		
		//check for X delta values and adjust accordingly
		//eventually adjust YAW based on amount of movement
		//don't do any movement of the cam using YAW/PITCH if we are zooming in/out the camera	
		if (!m_fCameraDistanceMove)
		{
			int x, y;
			GetWindowCenter( x,  y );
			
			//keep the camera within certain limits around the player (ie avoid certain bad viewing angles)  
			if (m_nCameraX>x)
			{
				//if ((idealAngles[YAW]>=225.0)||(idealAngles[YAW]<135.0))
				if (idealAngles[YAW]<c_maxyaw.GetFloat())
				{
					idealAngles[ YAW ] += (CAM_ANGLE_MOVE)*((m_nCameraX-x)/2);
				}
				if (idealAngles[YAW]>c_maxyaw.GetFloat())
				{
					
					idealAngles[YAW]=c_maxyaw.GetFloat();
				}
			}
			else if (m_nCameraX<x)
			{
				//if ((idealAngles[YAW]<=135.0)||(idealAngles[YAW]>225.0))
				if (idealAngles[YAW]>c_minyaw.GetFloat())
				{
					idealAngles[ YAW ] -= (CAM_ANGLE_MOVE)* ((x-m_nCameraX)/2);
					
				}
				if (idealAngles[YAW]<c_minyaw.GetFloat())
				{
					idealAngles[YAW]=c_minyaw.GetFloat();
					
				}
			}
			
			//check for y delta values and adjust accordingly
			//eventually adjust PITCH based on amount of movement
			//also make sure camera is within bounds
			if (m_nCameraY > y)
			{
				if(idealAngles[PITCH]<c_maxpitch.GetFloat())
				{
					idealAngles[PITCH] +=(CAM_ANGLE_MOVE)* ((m_nCameraY-y)/2);
				}
				if (idealAngles[PITCH]>c_maxpitch.GetFloat())
				{
					idealAngles[PITCH]=c_maxpitch.GetFloat();
				}
			}
			else if (m_nCameraY<y)
			{
				if (idealAngles[PITCH]>c_minpitch.GetFloat())
				{
					idealAngles[PITCH] -= (CAM_ANGLE_MOVE)*((y-m_nCameraY)/2);
				}
				if (idealAngles[PITCH]<c_minpitch.GetFloat())
				{
					idealAngles[PITCH]=c_minpitch.GetFloat();
				}
			}
			
			//set old mouse coordinates to current mouse coordinates
			//since we are done with the mouse
			
			if ( ( flSensitivity = gHUD.GetSensitivity() ) != 0 )
			{
				m_nCameraOldX=m_nCameraX*flSensitivity;
				m_nCameraOldY=m_nCameraY*flSensitivity;
			}
			else
			{
				m_nCameraOldX=m_nCameraX;
				m_nCameraOldY=m_nCameraY;
			}
#ifndef _XBOX
			ResetMouse();
#endif
		}
	}
	
	//Nathan code here
	if( input->KeyState( &cam_pitchup ) )
		idealAngles[ PITCH ] += cam_idealdelta.GetFloat();
	else if( input->KeyState( &cam_pitchdown ) )
		idealAngles[ PITCH ] -= cam_idealdelta.GetFloat();
	
	if( input->KeyState( &cam_yawleft ) )
		idealAngles[ YAW ] -= cam_idealdelta.GetFloat();
	else if( input->KeyState( &cam_yawright ) )
		idealAngles[ YAW ] += cam_idealdelta.GetFloat();
	
	if( input->KeyState( &cam_in ) )
	{
		idealAngles[ DIST ] -= 2*cam_idealdelta.GetFloat();
		if( idealAngles[ DIST ] < CAM_MIN_DIST )
		{
			// If we go back into first person, reset the angle
			idealAngles[ PITCH ] = 0;
			idealAngles[ YAW ] = 0;
			idealAngles[ DIST ] = CAM_MIN_DIST;
		}
		
	}
	else if( input->KeyState( &cam_out ) )
		idealAngles[ DIST ] += 2*cam_idealdelta.GetFloat();
	
	if (m_fCameraDistanceMove)
	{
		int x, y;
		GetWindowCenter( x, y );

		if (m_nCameraY>y)
		{
			if(idealAngles[ DIST ]<c_maxdistance.GetFloat())
			{
				idealAngles[ DIST ] +=cam_idealdelta.GetFloat() * ((m_nCameraY-y)/2);
			}
			if (idealAngles[ DIST ]>c_maxdistance.GetFloat())
			{
				idealAngles[ DIST ]=c_maxdistance.GetFloat();
			}
		}
		else if (m_nCameraY<y)
		{
			if (idealAngles[ DIST ]>c_mindistance.GetFloat())
			{
				idealAngles[ DIST ] -= (cam_idealdelta.GetFloat())*((y-m_nCameraY)/2);
			}
			if (idealAngles[ DIST ]<c_mindistance.GetFloat())
			{
				idealAngles[ DIST ]=c_mindistance.GetFloat();
			}
		}
		//set old mouse coordinates to current mouse coordinates
		//since we are done with the mouse
		m_nCameraOldX=m_nCameraX*gHUD.GetSensitivity();
		m_nCameraOldY=m_nCameraY*gHUD.GetSensitivity();
#ifndef _XBOX
		ResetMouse();
#endif
	}

	// Obtain engine view angles and if they popped while the camera was static,
	// fix the camera angles as well
	engine->GetViewAngles( viewangles );
	static QAngle s_oldAngles = viewangles;
	if ( Is_CAM_ThirdPerson_MayaMode() && ( s_oldAngles != viewangles ) )
	{
		idealAngles[ PITCH ] += s_oldAngles[ PITCH ] - viewangles[ PITCH ];
		idealAngles[  YAW  ] += s_oldAngles[  YAW  ] - viewangles[  YAW  ];
		s_oldAngles = viewangles;
	}

	// bring the pitch values back into a range that MoveToward can handle
	if ( idealAngles[ PITCH ] > 180 )
		idealAngles[ PITCH ] -= 360;
	else if ( idealAngles[ PITCH ] < -180 )
		idealAngles[ PITCH ] += 360;

	// bring the yaw values back into a range that MoveToward can handle
	// --
	// Vitaliy: going with >= 180 and <= -180.
	// This introduces a potential discontinuity when looking directly at model face
	// as camera yaw will be jumping from +180 to -180 and back, but when working with
	// the camera allows smooth rotational transitions from left to right and back.
	// Otherwise one of the transitions that has ">"-comparison will be locked.
	// --
	if ( idealAngles[ YAW ] >= 180 )
		idealAngles[ YAW ] -= 360;
	else if ( idealAngles[ YAW ] <= -180 )
		idealAngles[ YAW ] += 360;

	// clamp pitch, yaw and dist...
	idealAngles[ PITCH ] = clamp( idealAngles[ PITCH ], c_minpitch.GetFloat(), c_maxpitch.GetFloat() );
	idealAngles[ YAW ]   = clamp( idealAngles[ YAW ], c_minyaw.GetFloat(), c_maxyaw.GetFloat() );
	idealAngles[ DIST ]  = clamp( idealAngles[ DIST ], c_mindistance.GetFloat(), c_maxdistance.GetFloat() );

	// update ideal angles
	cam_idealpitch.SetValue( idealAngles[ PITCH ] );
	cam_idealyaw.SetValue( idealAngles[ YAW ] );
	cam_idealdist.SetValue( idealAngles[ DIST ] );
	
	// Move the CameraOffset "towards" the idealAngles
	// Note: CameraOffset = viewangle + idealAngle
	VectorCopy( g_ThirdPersonManager.GetCameraOffsetAngles(), camOffset );
	
	if( cam_snapto.GetInt() )
	{
		camOffset[ YAW ] = cam_idealyaw.GetFloat() + viewangles[ YAW ];
		camOffset[ PITCH ] = cam_idealpitch.GetFloat() + viewangles[ PITCH ];
		camOffset[ DIST ] = cam_idealdist.GetFloat();
	}
	else
	{
		float lag = MAX( 1, 1 + cam_ideallag.GetFloat() );

		if( camOffset[ YAW ] - viewangles[ YAW ] != cam_idealyaw.GetFloat() )
			camOffset[ YAW ] = MoveToward( camOffset[ YAW ], cam_idealyaw.GetFloat() + viewangles[ YAW ], lag );
		
		if( camOffset[ PITCH ] - viewangles[ PITCH ] != cam_idealpitch.GetFloat() )
			camOffset[ PITCH ] = MoveToward( camOffset[ PITCH ], cam_idealpitch.GetFloat() + viewangles[ PITCH ], lag );
		
		if( abs( camOffset[ DIST ] - cam_idealdist.GetFloat() ) < 2.0 )
			camOffset[ DIST ] = cam_idealdist.GetFloat();
		else
			camOffset[ DIST ] += ( cam_idealdist.GetFloat() - camOffset[ DIST ] ) / lag;
	}

	// move the camera closer to the player if it hit something
	if ( cam_collision.GetInt() )
	{
		QAngle desiredCamAngles = QAngle( camOffset[ PITCH ], camOffset[ YAW ], camOffset[ DIST ] );

		if ( g_ThirdPersonManager.IsOverridingThirdPerson() == false )
		{
			desiredCamAngles = viewangles;
		}

		g_ThirdPersonManager.PositionCamera( C_BasePlayer::GetLocalPlayer(), desiredCamAngles );
    }

	if ( cam_showangles.GetInt() )
	{
		engine->Con_NPrintf( 4, "Pitch: %6.1f   Yaw: %6.1f %38s", viewangles[ PITCH ], viewangles[ YAW ], "view angles" );
		engine->Con_NPrintf( 6, "Pitch: %6.1f   Yaw: %6.1f   Dist: %6.1f %19s", cam_idealpitch.GetFloat(), cam_idealyaw.GetFloat(), cam_idealdist.GetFloat(), "ideal angles" );
		engine->Con_NPrintf( 8, "Pitch: %6.1f   Yaw: %6.1f   Dist: %6.1f %16s", g_ThirdPersonManager.GetCameraOffsetAngles()[ PITCH ], g_ThirdPersonManager.GetCameraOffsetAngles()[ YAW ], g_ThirdPersonManager.GetCameraOffsetAngles()[ DIST ], "camera offset" );
	}

	g_ThirdPersonManager.SetCameraOffsetAngles( camOffset );
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void ClampRange180( float &value )
{
	if ( value >= 180.0f )
	{
		value -= 360.0f;
	}
	else if ( value <= -180.0f )
	{
		value += 360.0f;
	}
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CInput::CAM_SetCameraThirdData( CameraThirdData_t *pCameraData, const QAngle &vecCameraOffset )
{
	m_pCameraThirdData = pCameraData;

	Vector vTempOffset;

	vTempOffset[PITCH] = vecCameraOffset[PITCH];
	vTempOffset[YAW] = vecCameraOffset[YAW];
	vTempOffset[DIST] = vecCameraOffset[DIST];

	g_ThirdPersonManager.SetCameraOffsetAngles( vTempOffset );

}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CInput::CAM_CameraThirdThink( void )
{
	// Verify data.
	if ( !m_pCameraThirdData )
		return;

	// Verify that we are in third person mode.
	if( !m_fCameraInThirdPerson )
		return;

	// Obtain engine view angles and if they popped while the camera was static, fix the camera angles as well.
	QAngle angView;
	engine->GetViewAngles( angView );

	// Move the CameraOffset "towards" the idealAngles, Note: CameraOffset = viewangle + idealAngle
	Vector vecCamOffset;
	VectorCopy( g_ThirdPersonManager.GetCameraOffsetAngles(), vecCamOffset );

	// Move the camera.
	float flLag = MAX( 1, 1 + m_pCameraThirdData->m_flLag );
	if( vecCamOffset[PITCH] - angView[PITCH] != m_pCameraThirdData->m_flPitch )
	{
		vecCamOffset[PITCH] = MoveToward( vecCamOffset[PITCH], ( m_pCameraThirdData->m_flPitch + angView[PITCH] ), flLag );
	}
	if( vecCamOffset[YAW] - angView[YAW] != m_pCameraThirdData->m_flYaw )
	{
		vecCamOffset[YAW] = MoveToward( vecCamOffset[YAW], ( m_pCameraThirdData->m_flYaw + angView[YAW] ), flLag );
	}
	if( abs( vecCamOffset[DIST] - m_pCameraThirdData->m_flDist ) < 2.0 )
	{
		vecCamOffset[DIST] = m_pCameraThirdData->m_flDist;
	}
	else
	{
		vecCamOffset[DIST] += ( m_pCameraThirdData->m_flDist - vecCamOffset[DIST] ) / flLag;
	}

	C_BasePlayer* pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	if ( pLocalPlayer )
	{
		QAngle desiredCamAngles = QAngle( vecCamOffset[ PITCH ], vecCamOffset[ YAW ], vecCamOffset[DIST] );
	
		g_ThirdPersonManager.PositionCamera( C_BasePlayer::GetLocalPlayer(), desiredCamAngles );
		
	//	vecCamOffset = g_ThirdPersonManager.GetCameraOffsetAngles();
	}

	ClampRange180( vecCamOffset[PITCH] );
	ClampRange180( vecCamOffset[YAW] );

	g_ThirdPersonManager.SetCameraOffsetAngles( vecCamOffset );
}

void CAM_PitchUpDown( const CCommand &args ) { KeyDown( &cam_pitchup, args[1] ); }
void CAM_PitchUpUp( const CCommand &args ) { KeyUp( &cam_pitchup, args[1] ); }
void CAM_PitchDownDown( const CCommand &args ) { KeyDown( &cam_pitchdown, args[1] ); }
void CAM_PitchDownUp( const CCommand &args ) { KeyUp( &cam_pitchdown, args[1] ); }
void CAM_YawLeftDown( const CCommand &args ) { KeyDown( &cam_yawleft, args[1] ); }
void CAM_YawLeftUp( const CCommand &args ) { KeyUp( &cam_yawleft, args[1] ); }
void CAM_YawRightDown( const CCommand &args ) { KeyDown( &cam_yawright, args[1] ); }
void CAM_YawRightUp( const CCommand &args ) { KeyUp( &cam_yawright, args[1] ); }
void CAM_InDown( const CCommand &args ) { KeyDown( &cam_in, args[1] ); }
void CAM_InUp( const CCommand &args ) { KeyUp( &cam_in, args[1] ); }
void CAM_OutDown( const CCommand &args ) { KeyDown( &cam_out, args[1] ); }
void CAM_OutUp( const CCommand &args ) { KeyUp( &cam_out, args[1] ); }

/*
==============================
CAM_ToThirdPerson

==============================
*/
void CInput::CAM_ToThirdPerson(void)
{ 
	QAngle viewangles;

	engine->GetViewAngles( viewangles );

	if( !m_fCameraInThirdPerson )
	{
		m_fCameraInThirdPerson = true; 
	
		g_ThirdPersonManager.SetCameraOffsetAngles( Vector( viewangles[ YAW ], viewangles[ PITCH ], CAM_MIN_DIST ) );
	}

	cam_command.SetValue( 0 );
}

/*
==============================
CAM_ToFirstPerson

==============================
*/
void CInput::CAM_ToFirstPerson(void)
{
	g_ThirdPersonManager.SetDesiredCameraOffset( vec3_origin );

	m_fCameraInThirdPerson = false;
	cam_command.SetValue( 0 );

	// Let the local player know
	C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();
	if ( localPlayer )
	{
		localPlayer->ThirdPersonSwitch( false );
	}
}

/*
==============================
CAM_ToFirstPerson

==============================
*/
bool CInput::CAM_IsOrthographic(void) const
{
	return m_CameraIsOrthographic;
}


/*
==============================
CAM_ToFirstPerson

==============================
*/
void CInput::CAM_OrthographicSize(float& w, float& h) const
{
	w = c_orthowidth.GetFloat(); h = c_orthoheight.GetFloat();
}


/*
==============================
CAM_ToFirstPerson

==============================
*/
void CInput::CAM_ToOrthographic(void)
{
	m_fCameraInThirdPerson = false;
	m_CameraIsOrthographic = true;
	cam_command.SetValue( 0 );
}

/*
==============================
CAM_StartMouseMove

==============================
*/
void CInput::CAM_StartMouseMove(void)
{
	float flSensitivity;
		
	//only move the cam with mouse if we are in third person.
	if ( m_fCameraInThirdPerson )
	{
		//set appropriate flags and initialize the old mouse position
		//variables for mouse camera movement
		if (!m_fCameraMovingWithMouse)
		{
			int cpx, cpy;

			m_fCameraMovingWithMouse=true;
			m_fCameraInterceptingMouse=true;
#ifndef _XBOX			
			GetMousePos(cpx, cpy);
#else
			// xboxfixme
			cpx = cpy = 0;
#endif
			m_nCameraX = cpx;
			m_nCameraY = cpy;

			if ( ( flSensitivity = gHUD.GetSensitivity() ) != 0 )
			{
				m_nCameraOldX=m_nCameraX*flSensitivity;
				m_nCameraOldY=m_nCameraY*flSensitivity;
			}
			else
			{
				m_nCameraOldX=m_nCameraX;
				m_nCameraOldY=m_nCameraY;
			}
		}
	}
	//we are not in 3rd person view..therefore do not allow camera movement
	else
	{   
		m_fCameraMovingWithMouse=false;
		m_fCameraInterceptingMouse=false;
	}
}

/*
==============================
CAM_EndMouseMove

the key has been released for camera movement
tell the engine that mouse camera movement is off
==============================
*/
void CInput::CAM_EndMouseMove(void)
{
   m_fCameraMovingWithMouse=false;
   m_fCameraInterceptingMouse=false;
}

/*
==============================
CAM_StartDistance

routines to start the process of moving the cam in or out 
using the mouse
==============================
*/
void CInput::CAM_StartDistance(void)
{
	//only move the cam with mouse if we are in third person.
	if ( m_fCameraInThirdPerson )
	{
	  //set appropriate flags and initialize the old mouse position
	  //variables for mouse camera movement
	  if (!m_fCameraDistanceMove)
	  {
		  int cpx, cpy;

		  m_fCameraDistanceMove=true;
		  m_fCameraMovingWithMouse=true;
		  m_fCameraInterceptingMouse=true;
#ifndef _XBOX
		  GetMousePos(cpx, cpy);
#else
		  // xboxfixme
		  cpx = cpy = 0;
#endif

		  m_nCameraX = cpx;
		  m_nCameraY = cpy;

		  m_nCameraOldX=m_nCameraX*gHUD.GetSensitivity();
		  m_nCameraOldY=m_nCameraY*gHUD.GetSensitivity();
	  }
	}
	//we are not in 3rd person view..therefore do not allow camera movement
	else
	{   
		m_fCameraDistanceMove=false;
		m_fCameraMovingWithMouse=false;
		m_fCameraInterceptingMouse=false;
	}
}

/*
==============================
CAM_EndDistance

the key has been released for camera movement
tell the engine that mouse camera movement is off
==============================
*/
void CInput::CAM_EndDistance(void)
{
   m_fCameraDistanceMove=false;
   m_fCameraMovingWithMouse=false;
   m_fCameraInterceptingMouse=false;
}

/*
==============================
CAM_IsThirdPerson

==============================
*/
int CInput::CAM_IsThirdPerson( void )
{
	return m_fCameraInThirdPerson;
}

/*
==============================
CAM_InterceptingMouse

==============================
*/
int CInput::CAM_InterceptingMouse( void )
{
	return m_fCameraInterceptingMouse;
}

static ConCommand startpitchup( "+campitchup", CAM_PitchUpDown );
static ConCommand endpitcup( "-campitchup", CAM_PitchUpUp );
static ConCommand startpitchdown( "+campitchdown", CAM_PitchDownDown );
static ConCommand endpitchdown( "-campitchdown", CAM_PitchDownUp );
static ConCommand startcamyawleft( "+camyawleft", CAM_YawLeftDown );
static ConCommand endcamyawleft( "-camyawleft", CAM_YawLeftUp );
static ConCommand startcamyawright( "+camyawright", CAM_YawRightDown );
static ConCommand endcamyawright( "-camyawright", CAM_YawRightUp );
static ConCommand startcamin( "+camin", CAM_InDown );
static ConCommand endcamin( "-camin", CAM_InUp );
static ConCommand startcamout( "+camout", CAM_OutDown );
static ConCommand camout( "-camout", CAM_OutUp );
static ConCommand thirdperson_mayamode( "thirdperson_mayamode", ::CAM_ToThirdPerson_MayaMode, "Switch to thirdperson Maya-like camera controls.", FCVAR_CHEAT );

// TF allows servers to push people into first/thirdperson, for mods
#ifdef TF_CLIENT_DLL
static ConCommand thirdperson( "thirdperson", ::CAM_ToThirdPerson, "Switch to thirdperson camera.", FCVAR_CHEAT | FCVAR_SERVER_CAN_EXECUTE );
static ConCommand firstperson( "firstperson", ::CAM_ToFirstPerson, "Switch to firstperson camera.", FCVAR_SERVER_CAN_EXECUTE );
#else
static ConCommand thirdperson( "thirdperson", ::CAM_ToThirdPerson, "Switch to thirdperson camera.", FCVAR_CHEAT );
static ConCommand firstperson( "firstperson", ::CAM_ToFirstPerson, "Switch to firstperson camera." );
#endif
static ConCommand camortho( "camortho", ::CAM_ToOrthographic, "Switch to orthographic camera.", FCVAR_CHEAT );
static ConCommand startcammousemove( "+cammousemove",::CAM_StartMouseMove);
static ConCommand endcammousemove( "-cammousemove",::CAM_EndMouseMove);
static ConCommand startcamdistance( "+camdistance", ::CAM_StartDistance );
static ConCommand endcamdistance( "-camdistance", ::CAM_EndDistance );
static ConCommand snapto( "snapto", CAM_ToggleSnapto );
/*
==============================
Init_Camera

==============================
*/
void CInput::Init_Camera( void )
{
	m_CameraIsOrthographic = false;
}