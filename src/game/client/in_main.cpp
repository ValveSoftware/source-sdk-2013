//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: builds an intended movement command to send to the server
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "kbutton.h"
#include "usercmd.h"
#include "in_buttons.h"
#include "input.h"
#include "iviewrender.h"
#include "iclientmode.h"
#include "prediction.h"
#include "bitbuf.h"
#include "checksum_md5.h"
#include "hltvcamera.h"
#if defined( REPLAY_ENABLED )
#include "replay/replaycamera.h"
#endif
#include <ctype.h> // isalnum()
#include <voice_status.h>
#include "cam_thirdperson.h"

#ifdef SIXENSE
#include "sixense/in_sixense.h"
#endif

#include "client_virtualreality.h"
#include "sourcevr/isourcevirtualreality.h"

// NVNT Include
#include "haptics/haptic_utils.h"
#include <vgui/ISurface.h>

extern ConVar in_joystick;
extern ConVar cam_idealpitch;
extern ConVar cam_idealyaw;

// For showing/hiding the scoreboard
#include <game/client/iviewport.h>

// Need this for steam controller
#include "clientsteamcontext.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// FIXME, tie to entity state parsing for player!!!
int g_iAlive = 1;

static int s_ClearInputState = 0;

// Defined in pm_math.c
float anglemod( float a );

// FIXME void V_Init( void );
static int in_impulse = 0;
static int in_cancel = 0;

ConVar cl_anglespeedkey( "cl_anglespeedkey", "0.67", 0 );
ConVar cl_yawspeed( "cl_yawspeed", "210", FCVAR_NONE, "Client yaw speed.", true, -100000, true, 100000 );
ConVar cl_pitchspeed( "cl_pitchspeed", "225", FCVAR_NONE, "Client pitch speed.", true, -100000, true, 100000 );
ConVar cl_pitchdown( "cl_pitchdown", "89", FCVAR_CHEAT );
ConVar cl_pitchup( "cl_pitchup", "89", FCVAR_CHEAT );
#if defined( CSTRIKE_DLL )
ConVar cl_sidespeed( "cl_sidespeed", "400", FCVAR_CHEAT );
ConVar cl_upspeed( "cl_upspeed", "320", FCVAR_ARCHIVE|FCVAR_CHEAT );
ConVar cl_forwardspeed( "cl_forwardspeed", "400", FCVAR_ARCHIVE|FCVAR_CHEAT );
ConVar cl_backspeed( "cl_backspeed", "400", FCVAR_ARCHIVE|FCVAR_CHEAT );
#else
ConVar cl_sidespeed( "cl_sidespeed", "450", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar cl_upspeed( "cl_upspeed", "320", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar cl_forwardspeed( "cl_forwardspeed", "450", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar cl_backspeed( "cl_backspeed", "450", FCVAR_REPLICATED | FCVAR_CHEAT );
#endif // CSTRIKE_DLL
ConVar lookspring( "lookspring", "0", FCVAR_ARCHIVE );
ConVar lookstrafe( "lookstrafe", "0", FCVAR_ARCHIVE );
ConVar in_joystick( "joystick","0", FCVAR_ARCHIVE );

ConVar thirdperson_platformer( "thirdperson_platformer", "0", 0, "Player will aim in the direction they are moving." );
ConVar thirdperson_screenspace( "thirdperson_screenspace", "0", 0, "Movement will be relative to the camera, eg: left means screen-left" );

ConVar sv_noclipduringpause( "sv_noclipduringpause", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "If cheats are enabled, then you can noclip with the game paused (for doing screenshots, etc.)." );

extern ConVar cl_mouselook;

#define UsingMouselook() cl_mouselook.GetBool()

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition

===============================================================================
*/

kbutton_t	in_speed;
kbutton_t	in_walk;
kbutton_t	in_jlook;
kbutton_t	in_strafe;
kbutton_t	in_commandermousemove;
kbutton_t	in_forward;
kbutton_t	in_back;
kbutton_t	in_moveleft;
kbutton_t	in_moveright;
// Display the netgraph
kbutton_t	in_graph;  
kbutton_t	in_joyspeed;		// auto-speed key from the joystick (only works for player movement, not vehicles)

static	kbutton_t	in_klook;
kbutton_t	in_left;
kbutton_t	in_right;
static	kbutton_t	in_lookup;
static	kbutton_t	in_lookdown;
static	kbutton_t	in_use;
static	kbutton_t	in_jump;
static	kbutton_t	in_attack;
static	kbutton_t	in_attack2;
static	kbutton_t	in_up;
static	kbutton_t	in_down;
static	kbutton_t	in_duck;
static	kbutton_t	in_reload;
static	kbutton_t	in_alt1;
static	kbutton_t	in_alt2;
static	kbutton_t	in_score;
static	kbutton_t	in_break;
static	kbutton_t	in_zoom;
static  kbutton_t   in_grenade1;
static  kbutton_t   in_grenade2;
static	kbutton_t	in_attack3;
kbutton_t	in_ducktoggle;

/*
===========
IN_CenterView_f
===========
*/
void IN_CenterView_f (void)
{
	QAngle viewangles;

	if ( UsingMouselook() == false )
	{
		if ( !::input->CAM_InterceptingMouse() )
		{
			engine->GetViewAngles( viewangles );
			viewangles[PITCH] = 0;
			engine->SetViewAngles( viewangles );
		}
	}
}

/*
===========
IN_Joystick_Advanced_f
===========
*/
void IN_Joystick_Advanced_f (void)
{
	::input->Joystick_Advanced();
}

/*
============
KB_ConvertString

Removes references to +use and replaces them with the keyname in the output string.  If
 a binding is unfound, then the original text is retained.
NOTE:  Only works for text with +word in it.
============
*/
int KB_ConvertString( char *in, char **ppout )
{
	char sz[ 4096 ];
	char binding[ 64 ];
	char *p;
	char *pOut;
	char *pEnd;
	const char *pBinding;

	if ( !ppout )
		return 0;

	*ppout = NULL;
	p = in;
	pOut = sz;
	while ( *p )
	{
		if ( *p == '+' )
		{
			pEnd = binding;
			while ( *p && ( V_isalnum( *p ) || ( pEnd == binding ) ) && ( ( pEnd - binding ) < 63 ) )
			{
				*pEnd++ = *p++;
			}

			*pEnd =  '\0';

			pBinding = NULL;
			if ( strlen( binding + 1 ) > 0 )
			{
				// See if there is a binding for binding?
				pBinding = engine->Key_LookupBinding( binding + 1 );
			}

			if ( pBinding )
			{
				*pOut++ = '[';
				pEnd = (char *)pBinding;
			}
			else
			{
				pEnd = binding;
			}

			while ( *pEnd )
			{
				*pOut++ = *pEnd++;
			}

			if ( pBinding )
			{
				*pOut++ = ']';
			}
		}
		else
		{
			*pOut++ = *p++;
		}
	}

	*pOut = '\0';

	int maxlen = strlen( sz ) + 1;
	pOut = ( char * )malloc( maxlen );
	Q_strncpy( pOut, sz, maxlen );
	*ppout = pOut;

	return 1;
}

/*
==============================
FindKey

Allows the engine to request a kbutton handler by name, if the key exists.
==============================
*/
kbutton_t *CInput::FindKey( const char *name )
{
	CKeyboardKey *p;
	p = m_pKeys;
	while ( p )
	{
		if ( !Q_stricmp( name, p->name ) )
		{
			return p->pkey;
		}

		p = p->next;
	}
	return NULL;
}

/*
============
AddKeyButton

Add a kbutton_t * to the list of pointers the engine can retrieve via KB_Find
============
*/
void CInput::AddKeyButton( const char *name, kbutton_t *pkb )
{
	CKeyboardKey *p;	
	kbutton_t *kb;

	kb = FindKey( name );
	
	if ( kb )
		return;

	p = new CKeyboardKey;

	Q_strncpy( p->name, name, sizeof( p->name ) );
	p->pkey = pkb;

	p->next = m_pKeys;
	m_pKeys = p;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CInput::CInput( void )
{
	m_pCommands = NULL;
	m_pCameraThirdData = NULL;
	m_pVerifiedCommands = NULL;
	m_PreferredGameActionSet = GAME_ACTION_SET_MENUCONTROLS;
	m_bSteamControllerGameActionsInitialized = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CInput::~CInput( void )
{
}

/*
============
Init_Keyboard

Add kbutton_t definitions that the engine can query if needed
============
*/
void CInput::Init_Keyboard( void )
{
	m_pKeys = NULL;

	AddKeyButton( "in_graph", &in_graph );
	AddKeyButton( "in_jlook", &in_jlook );
}

/*
============
Shutdown_Keyboard

Clear kblist
============
*/
void CInput::Shutdown_Keyboard( void )
{
	CKeyboardKey *p, *n;
	p = m_pKeys;
	while ( p )
	{
		n = p->next;
		delete p;
		p = n;
	}
	m_pKeys = NULL;
}

/*
============
KeyDown
============
*/
void KeyDown( kbutton_t *b, const char *c )
{
	int		k = -1;
	if ( c && c[0] )
	{
		k = atoi(c);
	}

	if (k == b->down[0] || k == b->down[1])
		return;		// repeating key
	
	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else
	{
		if ( c[0] )
		{
			DevMsg( 1,"Three keys down for a button '%c' '%c' '%c'!\n", b->down[0], b->down[1], c[0]);
		}
		return;
	}
	
	if (b->state & 1)
		return;		// still down
	b->state |= 1 + 2;	// down + impulse down
}

/*
============
KeyUp
============
*/
void KeyUp( kbutton_t *b, const char *c )
{	
	if ( !c || !c[0] )
	{
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

	int k = atoi(c);

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// key up without coresponding down (menu pass through)

	if (b->down[0] || b->down[1])
	{
		//Msg ("Keys down for button: '%c' '%c' '%c' (%d,%d,%d)!\n", b->down[0], b->down[1], c, b->down[0], b->down[1], c);
		return;		// some other key is still holding it down
	}

	if (!(b->state & 1))
		return;		// still up (this should not happen)

	b->state &= ~1;		// now up
	b->state |= 4; 		// impulse up
}

void IN_CommanderMouseMoveDown( const CCommand &args ) {KeyDown(&in_commandermousemove, args[1] );}
void IN_CommanderMouseMoveUp( const CCommand &args ) {KeyUp(&in_commandermousemove, args[1] );}
void IN_BreakDown( const CCommand &args ) { KeyDown( &in_break , args[1] );}
void IN_BreakUp( const CCommand &args )
{ 
	KeyUp( &in_break, args[1] ); 
#if defined( _DEBUG )
	DebuggerBreak();
#endif
};
void IN_KLookDown ( const CCommand &args ) {KeyDown(&in_klook, args[1] );}
void IN_KLookUp ( const CCommand &args ) {KeyUp(&in_klook, args[1] );}
void IN_JLookDown ( const CCommand &args ) {KeyDown(&in_jlook, args[1] );}
void IN_JLookUp ( const CCommand &args ) {KeyUp(&in_jlook, args[1] );}
void IN_UpDown( const CCommand &args ) {KeyDown(&in_up, args[1] );}
void IN_UpUp( const CCommand &args ) {KeyUp(&in_up, args[1] );}
void IN_DownDown( const CCommand &args ) {KeyDown(&in_down, args[1] );}
void IN_DownUp( const CCommand &args ) {KeyUp(&in_down, args[1] );}
void IN_LeftDown( const CCommand &args ) {KeyDown(&in_left, args[1] );}
void IN_LeftUp( const CCommand &args ) {KeyUp(&in_left, args[1] );}
void IN_RightDown( const CCommand &args ) {KeyDown(&in_right, args[1] );}
void IN_RightUp( const CCommand &args ) {KeyUp(&in_right, args[1] );}
void IN_ForwardDown( const CCommand &args ) {KeyDown(&in_forward, args[1] );}
void IN_ForwardUp( const CCommand &args ) {KeyUp(&in_forward, args[1] );}
void IN_BackDown( const CCommand &args ) {KeyDown(&in_back, args[1] );}
void IN_BackUp( const CCommand &args ) {KeyUp(&in_back, args[1] );}
void IN_LookupDown( const CCommand &args ) {KeyDown(&in_lookup, args[1] );}
void IN_LookupUp( const CCommand &args ) {KeyUp(&in_lookup, args[1] );}
void IN_LookdownDown( const CCommand &args ) {KeyDown(&in_lookdown, args[1] );}
void IN_LookdownUp( const CCommand &args ) {KeyUp(&in_lookdown, args[1] );}
void IN_MoveleftDown( const CCommand &args ) {KeyDown(&in_moveleft, args[1] );}
void IN_MoveleftUp( const CCommand &args ) {KeyUp(&in_moveleft, args[1] );}
void IN_MoverightDown( const CCommand &args ) {KeyDown(&in_moveright, args[1] );}
void IN_MoverightUp( const CCommand &args ) {KeyUp(&in_moveright, args[1] );}
void IN_WalkDown( const CCommand &args ) {KeyDown(&in_walk, args[1] );}
void IN_WalkUp( const CCommand &args ) {KeyUp(&in_walk, args[1] );}
void IN_SpeedDown( const CCommand &args ) {KeyDown(&in_speed, args[1] );}
void IN_SpeedUp( const CCommand &args ) {KeyUp(&in_speed, args[1] );}
void IN_StrafeDown( const CCommand &args ) {KeyDown(&in_strafe, args[1] );}
void IN_StrafeUp( const CCommand &args ) {KeyUp(&in_strafe, args[1] );}
void IN_Attack2Down( const CCommand &args ) { KeyDown(&in_attack2, args[1] );}
void IN_Attack2Up( const CCommand &args ) {KeyUp(&in_attack2, args[1] );}
void IN_UseDown ( const CCommand &args ) {KeyDown(&in_use, args[1] );}
void IN_UseUp ( const CCommand &args ) {KeyUp(&in_use, args[1] );}
void IN_JumpDown ( const CCommand &args ) {KeyDown(&in_jump, args[1] );}
void IN_JumpUp ( const CCommand &args ) {KeyUp(&in_jump, args[1] );}
void IN_DuckDown( const CCommand &args ) {KeyDown(&in_duck, args[1] );}
void IN_DuckUp( const CCommand &args ) {KeyUp(&in_duck, args[1] );}
void IN_ReloadDown( const CCommand &args ) {KeyDown(&in_reload, args[1] );}
void IN_ReloadUp( const CCommand &args ) {KeyUp(&in_reload, args[1] );}
void IN_Alt1Down( const CCommand &args ) {KeyDown(&in_alt1, args[1] );}
void IN_Alt1Up( const CCommand &args ) {KeyUp(&in_alt1, args[1] );}
void IN_Alt2Down( const CCommand &args ) {KeyDown(&in_alt2, args[1] );}
void IN_Alt2Up( const CCommand &args ) {KeyUp(&in_alt2, args[1] );}
void IN_GraphDown( const CCommand &args ) {KeyDown(&in_graph, args[1] );}
void IN_GraphUp( const CCommand &args ) {KeyUp(&in_graph, args[1] );}
void IN_ZoomDown( const CCommand &args ) {KeyDown(&in_zoom, args[1] );}
void IN_ZoomUp( const CCommand &args ) {KeyUp(&in_zoom, args[1] );}
void IN_Grenade1Up( const CCommand &args ) { KeyUp( &in_grenade1, args[1] ); }
void IN_Grenade1Down( const CCommand &args ) { KeyDown( &in_grenade1, args[1] ); }
void IN_Grenade2Up( const CCommand &args ) { KeyUp( &in_grenade2, args[1] ); }
void IN_Grenade2Down( const CCommand &args ) { KeyDown( &in_grenade2, args[1] ); }
void IN_XboxStub( const CCommand &args ) { /*do nothing*/ }
void IN_Attack3Down( const CCommand &args ) { KeyDown(&in_attack3, args[1] );}
void IN_Attack3Up( const CCommand &args ) { KeyUp(&in_attack3, args[1] );}

void IN_DuckToggle( const CCommand &args ) 
{ 
	if ( ::input->KeyState(&in_ducktoggle) )
	{
		KeyUp( &in_ducktoggle, args[1] ); 
	}
	else
	{
		KeyDown( &in_ducktoggle, args[1] ); 
	}
}

void IN_AttackDown( const CCommand &args )
{
	KeyDown( &in_attack, args[1] );
}

void IN_AttackUp( const CCommand &args )
{
	KeyUp( &in_attack, args[1] );
	in_cancel = 0;
}

// Special handling
void IN_Cancel( const CCommand &args )
{
	in_cancel = 1;
}

void IN_Impulse( const CCommand &args )
{
	in_impulse = atoi( args[1] );
}

void IN_ScoreDown( const CCommand &args )
{
	KeyDown( &in_score, args[1] );
	if ( gViewPortInterface )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, true );
	}
}

void IN_ScoreUp( const CCommand &args )
{
	KeyUp( &in_score, args[1] );
	if ( gViewPortInterface )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, false );
		GetClientVoiceMgr()->StopSquelchMode();
	}
}


/*
============
KeyEvent

Return 1 to allow engine to process the key, otherwise, act on it as needed
============
*/
int CInput::KeyEvent( int down, ButtonCode_t code, const char *pszCurrentBinding )
{
	// Deal with camera intercepting the mouse
	if ( ( code == MOUSE_LEFT ) || ( code == MOUSE_RIGHT ) )
	{
		if ( m_fCameraInterceptingMouse )
			return 0;
	}

	if ( g_pClientMode )
		return g_pClientMode->KeyInput(down, code, pszCurrentBinding);

	return 1;
}



/*
===============
KeyState

Returns 0.25 if a key was pressed and released during the frame,
0.5 if it was pressed and held
0 if held then released, and
1.0 if held for the entire time
===============
*/
float CInput::KeyState ( kbutton_t *key )
{
	float		val = 0.0;
	int			impulsedown, impulseup, down;
	
	impulsedown = key->state & 2;
	impulseup	= key->state & 4;
	down		= key->state & 1;
	
	if ( impulsedown && !impulseup )
	{
		// pressed and held this frame?
		val = down ? 0.5 : 0.0;
	}

	if ( impulseup && !impulsedown )
	{
		// released this frame?
		val = down ? 0.0 : 0.0;
	}

	if ( !impulsedown && !impulseup )
	{
		// held the entire frame?
		val = down ? 1.0 : 0.0;
	}

	if ( impulsedown && impulseup )
	{
		if ( down )
		{
			// released and re-pressed this frame
			val = 0.75;	
		}
		else
		{
			// pressed and released this frame
			val = 0.25;	
		}
	}

	// clear impulses
	key->state &= 1;		
	return val;
}

void CInput::IN_SetSampleTime( float frametime )
{
	m_flKeyboardSampleTime = frametime;
}

/*
==============================
DetermineKeySpeed

==============================
*/
static ConVar in_usekeyboardsampletime( "in_usekeyboardsampletime", "1", 0, "Use keyboard sample time smoothing." );

float CInput::DetermineKeySpeed( float frametime )
{

	if ( in_usekeyboardsampletime.GetBool() )
	{
		if ( m_flKeyboardSampleTime <= 0 )
			return 0.0f;
	
		frametime = MIN( m_flKeyboardSampleTime, frametime );
		m_flKeyboardSampleTime -= frametime;
	}
	
	float speed;

	speed = frametime;

	if ( in_speed.state & 1 )
	{
		speed *= cl_anglespeedkey.GetFloat();
	}

	return speed;
}

/*
==============================
AdjustYaw

==============================
*/
void CInput::AdjustYaw( float speed, QAngle& viewangles )
{
	if ( !(in_strafe.state & 1) )
	{
		viewangles[YAW] -= speed*cl_yawspeed.GetFloat() * KeyState (&in_right);
		viewangles[YAW] += speed*cl_yawspeed.GetFloat() * KeyState (&in_left);
	}

	// thirdperson platformer mode
	// use movement keys to aim the player relative to the thirdperson camera
	if ( CAM_IsThirdPerson() && thirdperson_platformer.GetInt() )
	{
		float side = KeyState(&in_moveleft) - KeyState(&in_moveright);
		float forward = KeyState(&in_forward) - KeyState(&in_back);

		if ( side || forward )
		{
			viewangles[YAW] = RAD2DEG(atan2(side, forward)) + g_ThirdPersonManager.GetCameraOffsetAngles()[ YAW ];
		}
		if ( side || forward || KeyState (&in_right) || KeyState (&in_left) )
		{
			cam_idealyaw.SetValue( g_ThirdPersonManager.GetCameraOffsetAngles()[ YAW ] - viewangles[ YAW ] );
		}
	}
}

/*
==============================
AdjustPitch

==============================
*/
void CInput::AdjustPitch( float speed, QAngle& viewangles )
{
	// only allow keyboard looking if mouse look is disabled
	if ( UsingMouselook() == false )
	{
		float	up, down;

		if ( in_klook.state & 1 )
		{
			view->StopPitchDrift ();
			viewangles[PITCH] -= speed*cl_pitchspeed.GetFloat() * KeyState (&in_forward);
			viewangles[PITCH] += speed*cl_pitchspeed.GetFloat() * KeyState (&in_back);
		}

		up		= KeyState ( &in_lookup );
		down	= KeyState ( &in_lookdown );
		
		viewangles[PITCH] -= speed*cl_pitchspeed.GetFloat() * up;
		viewangles[PITCH] += speed*cl_pitchspeed.GetFloat() * down;

		if ( up || down )
		{
			view->StopPitchDrift ();
		}
	}	
}

/*
==============================
ClampAngles

==============================
*/
void CInput::ClampAngles( QAngle& viewangles )
{
	if ( viewangles[PITCH] > cl_pitchdown.GetFloat() )
	{
		viewangles[PITCH] = cl_pitchdown.GetFloat();
	}
	if ( viewangles[PITCH] < -cl_pitchup.GetFloat() )
	{
		viewangles[PITCH] = -cl_pitchup.GetFloat();
	}

#ifndef PORTAL	// Don't constrain Roll in Portal because the player can be upside down! -Jeep
	if ( viewangles[ROLL] > 50 )
	{
		viewangles[ROLL] = 50;
	}
	if ( viewangles[ROLL] < -50 )
	{
		viewangles[ROLL] = -50;
	}
#endif
}

/*
================
AdjustAngles

Moves the local angle positions
================
*/
void CInput::AdjustAngles ( float frametime )
{
	float	speed;
	QAngle viewangles;
	
	// Determine control scaling factor ( multiplies time )
	speed = DetermineKeySpeed( frametime );
	if ( speed <= 0.0f )
	{
		return;
	}

	// Retrieve latest view direction from engine
	engine->GetViewAngles( viewangles );

	// Adjust YAW
	AdjustYaw( speed, viewangles );

	// Adjust PITCH if keyboard looking
	AdjustPitch( speed, viewangles );
	
	// Make sure values are legitimate
	ClampAngles( viewangles );

	// Store new view angles into engine view direction
	engine->SetViewAngles( viewangles );
}

/*
==============================
ComputeSideMove

==============================
*/
void CInput::ComputeSideMove( CUserCmd *cmd )
{
	// thirdperson platformer movement
	if ( CAM_IsThirdPerson() && thirdperson_platformer.GetInt() )
	{
		// no sideways movement in this mode
		return;
	}

	// thirdperson screenspace movement
	if ( CAM_IsThirdPerson() && thirdperson_screenspace.GetInt() )
	{
		float ideal_yaw = cam_idealyaw.GetFloat();
		float ideal_sin = sin(DEG2RAD(ideal_yaw));
		float ideal_cos = cos(DEG2RAD(ideal_yaw));
		
		float movement = ideal_cos*KeyState(&in_moveright)
			+  ideal_sin*KeyState(&in_back)
			+ -ideal_cos*KeyState(&in_moveleft)
			+ -ideal_sin*KeyState(&in_forward);

		cmd->sidemove += cl_sidespeed.GetFloat() * movement;

		return;
	}

	// If strafing, check left and right keys and act like moveleft and moveright keys
	if ( in_strafe.state & 1 )
	{
		cmd->sidemove += cl_sidespeed.GetFloat() * KeyState (&in_right);
		cmd->sidemove -= cl_sidespeed.GetFloat() * KeyState (&in_left);
	}

	// Otherwise, check strafe keys
	cmd->sidemove += cl_sidespeed.GetFloat() * KeyState (&in_moveright);
	cmd->sidemove -= cl_sidespeed.GetFloat() * KeyState (&in_moveleft);
}

/*
==============================
ComputeUpwardMove

==============================
*/
void CInput::ComputeUpwardMove( CUserCmd *cmd )
{
	cmd->upmove += cl_upspeed.GetFloat() * KeyState (&in_up);
	cmd->upmove -= cl_upspeed.GetFloat() * KeyState (&in_down);
}

/*
==============================
ComputeForwardMove

==============================
*/
void CInput::ComputeForwardMove( CUserCmd *cmd )
{
	// thirdperson platformer movement
	if ( CAM_IsThirdPerson() && thirdperson_platformer.GetInt() )
	{
		// movement is always forward in this mode
		float movement = KeyState(&in_forward)
			|| KeyState(&in_moveright)
			|| KeyState(&in_back)
			|| KeyState(&in_moveleft);

		cmd->forwardmove += cl_forwardspeed.GetFloat() * movement;

		return;
	}

	// thirdperson screenspace movement
	if ( CAM_IsThirdPerson() && thirdperson_screenspace.GetInt() )
	{
		float ideal_yaw = cam_idealyaw.GetFloat();
		float ideal_sin = sin(DEG2RAD(ideal_yaw));
		float ideal_cos = cos(DEG2RAD(ideal_yaw));
		
		float movement = ideal_cos*KeyState(&in_forward)
			+  ideal_sin*KeyState(&in_moveright)
			+ -ideal_cos*KeyState(&in_back)
			+ -ideal_sin*KeyState(&in_moveleft);

		cmd->forwardmove += cl_forwardspeed.GetFloat() * movement;

		return;
	}

	if ( !(in_klook.state & 1 ) )
	{	
		cmd->forwardmove += cl_forwardspeed.GetFloat() * KeyState (&in_forward);
		cmd->forwardmove -= cl_backspeed.GetFloat() * KeyState (&in_back);
	}	
}

/*
==============================
ScaleMovements

==============================
*/
void CInput::ScaleMovements( CUserCmd *cmd )
{
	// float spd;

	// clip to maxspeed
	// FIXME FIXME:  This doesn't work
	return;

	/*
	spd = engine->GetClientMaxspeed();
	if ( spd == 0.0 )
		return;

	// Scale the speed so that the total velocity is not > spd
	float fmov = sqrt( (cmd->forwardmove*cmd->forwardmove) + (cmd->sidemove*cmd->sidemove) + (cmd->upmove*cmd->upmove) );

	if ( fmov > spd && fmov > 0.0 )
	{
		float fratio = spd / fmov;

		if ( !IsNoClipping() ) 
		{
			cmd->forwardmove	*= fratio;
			cmd->sidemove		*= fratio;
			cmd->upmove			*= fratio;
		}
	}
	*/
}


/*
===========
ControllerMove
===========
*/
void CInput::ControllerMove( float frametime, CUserCmd *cmd )
{
	if ( IsPC() )
	{
		if ( !m_fCameraInterceptingMouse && m_fMouseActive )
		{
			MouseMove( cmd);
		}
	}

	SteamControllerMove( frametime, cmd );
	JoyStickMove( frametime, cmd );

	// NVNT if we have a haptic device..
	if(haptics && haptics->HasDevice())
	{
		if(engine->IsPaused() || engine->IsLevelMainMenuBackground() || vgui::surface()->IsCursorVisible() || !engine->IsInGame())
		{
			// NVNT send a menu process to the haptics system.
			haptics->MenuProcess();
			return;
		}
#ifdef CSTRIKE_DLL
		// NVNT cstrike fov grabing.
		C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
		if(player){
			haptics->UpdatePlayerFOV(player->GetFOV());
		}
#endif
		// NVNT calculate move with the navigation on the haptics system.
		haptics->CalculateMove(cmd->forwardmove, cmd->sidemove, frametime);
		// NVNT send a game process to the haptics system.
		haptics->GameProcess();
#if defined( WIN32 ) && !defined( _X360 )
		// NVNT update our avatar effect.
		UpdateAvatarEffect();
#endif
	}


}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *weapon - 
//-----------------------------------------------------------------------------
void CInput::MakeWeaponSelection( C_BaseCombatWeapon *weapon )
{
	m_hSelectedWeapon = weapon;
}

/*
================
CreateMove

Send the intended movement message to the server
if active == 1 then we are 1) not playing back demos ( where our commands are ignored ) and
2 ) we have finished signing on to server
================
*/

void CInput::ExtraMouseSample( float frametime, bool active )
{
	CUserCmd dummy;
	CUserCmd *cmd = &dummy;

	cmd->Reset();


	QAngle viewangles;
	engine->GetViewAngles( viewangles );
	QAngle originalViewangles = viewangles;

	if ( active )
	{
		// Determine view angles
		AdjustAngles ( frametime );

		// Determine sideways movement
		ComputeSideMove( cmd );

		// Determine vertical movement
		ComputeUpwardMove( cmd );

		// Determine forward movement
		ComputeForwardMove( cmd );

		// Scale based on holding speed key or having too fast of a velocity based on client maximum
		//  speed.
		ScaleMovements( cmd );

		// Allow mice and other controllers to add their inputs
		ControllerMove( frametime, cmd );
#ifdef SIXENSE
		g_pSixenseInput->SixenseFrame( frametime, cmd ); 

		if( g_pSixenseInput->IsEnabled() )
		{
			g_pSixenseInput->SetView( frametime, cmd );
		}
#endif
	}

	// Retreive view angles from engine ( could have been set in IN_AdjustAngles above )
	engine->GetViewAngles( viewangles );

	// Set button and flag bits, don't blow away state
#ifdef SIXENSE
	if( g_pSixenseInput->IsEnabled() )
	{
		// Some buttons were set in SixenseUpdateKeys, so or in any real keypresses
		cmd->buttons |= GetButtonBits( 0 );
	}
	else
	{
		cmd->buttons = GetButtonBits( 0 );
	}
#else
	cmd->buttons = GetButtonBits( 0 );
#endif

	// Use new view angles if alive, otherwise user last angles we stored off.
	if ( g_iAlive )
	{
		VectorCopy( viewangles, cmd->viewangles );
		VectorCopy( viewangles, m_angPreviousViewAngles );
	}
	else
	{
		VectorCopy( m_angPreviousViewAngles, cmd->viewangles );
	}

	// Let the move manager override anything it wants to.
	if ( g_pClientMode->CreateMove( frametime, cmd ) )
	{
		// Get current view angles after the client mode tweaks with it
		engine->SetViewAngles( cmd->viewangles );
		prediction->SetLocalViewAngles( cmd->viewangles );
	}

	// Let the headtracker override the view at the very end of the process so
	// that vehicles and other stuff in g_pClientMode->CreateMove can override 
	// first
	if ( active && UseVR() )
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if( pPlayer && !pPlayer->GetVehicle() )
		{
			QAngle curViewangles, newViewangles;
			Vector curMotion, newMotion;
			engine->GetViewAngles( curViewangles );
			curMotion.Init ( 
				cmd->forwardmove,
				cmd->sidemove,
				cmd->upmove );
			g_ClientVirtualReality.OverridePlayerMotion ( frametime, originalViewangles, curViewangles, curMotion, &newViewangles, &newMotion );
			engine->SetViewAngles( newViewangles );
			cmd->forwardmove = newMotion[0];
			cmd->sidemove = newMotion[1];
			cmd->upmove = newMotion[2];

			cmd->viewangles = newViewangles;
			prediction->SetLocalViewAngles( cmd->viewangles );
		}
	}

}

void CInput::CreateMove ( int sequence_number, float input_sample_frametime, bool active )
{	
	CUserCmd *cmd = &m_pCommands[ sequence_number % MULTIPLAYER_BACKUP ];
	CVerifiedUserCmd *pVerified = &m_pVerifiedCommands[ sequence_number % MULTIPLAYER_BACKUP ];

	cmd->Reset();

	cmd->command_number = sequence_number;
	cmd->tick_count = gpGlobals->tickcount;

	QAngle viewangles;
	engine->GetViewAngles( viewangles );
	QAngle originalViewangles = viewangles;

	if ( active || sv_noclipduringpause.GetInt() )
	{
		// Determine view angles
		AdjustAngles ( input_sample_frametime );

		// Determine sideways movement
		ComputeSideMove( cmd );

		// Determine vertical movement
		ComputeUpwardMove( cmd );

		// Determine forward movement
		ComputeForwardMove( cmd );

		// Scale based on holding speed key or having too fast of a velocity based on client maximum
		//  speed.
		ScaleMovements( cmd );

		// Allow mice and other controllers to add their inputs
		ControllerMove( input_sample_frametime, cmd );
#ifdef SIXENSE
		g_pSixenseInput->SixenseFrame( input_sample_frametime, cmd ); 

		if( g_pSixenseInput->IsEnabled() )
		{
			g_pSixenseInput->SetView( input_sample_frametime, cmd );
		}
#endif
	}
	else
	{
		// need to run and reset mouse input so that there is no view pop when unpausing
		if ( !m_fCameraInterceptingMouse && m_fMouseActive )
		{
			float mx, my;
			GetAccumulatedMouseDeltasAndResetAccumulators( &mx, &my );
			ResetMouse();
		}
	}
	// Retreive view angles from engine ( could have been set in IN_AdjustAngles above )
	engine->GetViewAngles( viewangles );

	// Latch and clear impulse
	cmd->impulse = in_impulse;
	in_impulse = 0;

	// Latch and clear weapon selection
	if ( m_hSelectedWeapon != NULL )
	{
		C_BaseCombatWeapon *weapon = m_hSelectedWeapon;

		cmd->weaponselect = weapon->entindex();
		cmd->weaponsubtype = weapon->GetSubType();

		// Always clear weapon selection
		m_hSelectedWeapon = NULL;
	}

	// Set button and flag bits
#ifdef SIXENSE
	if( g_pSixenseInput->IsEnabled() )
	{
		// Some buttons were set in SixenseUpdateKeys, so or in any real keypresses
		cmd->buttons |= GetButtonBits( 1 );
	}
	else
	{
		cmd->buttons = GetButtonBits( 1 );
	}
#else
	// Set button and flag bits
	cmd->buttons = GetButtonBits( 1 );
#endif

	// Using joystick?
#ifdef SIXENSE
	if ( in_joystick.GetInt() || g_pSixenseInput->IsEnabled() )
#else
	if ( in_joystick.GetInt() )
#endif
	{
		if ( cmd->forwardmove > 0 )
		{
			cmd->buttons |= IN_FORWARD;
		}
		else if ( cmd->forwardmove < 0 )
		{
			cmd->buttons |= IN_BACK;
		}
	}

	// Use new view angles if alive, otherwise user last angles we stored off.
	if ( g_iAlive )
	{
		VectorCopy( viewangles, cmd->viewangles );
		VectorCopy( viewangles, m_angPreviousViewAngles );
	}
	else
	{
		VectorCopy( m_angPreviousViewAngles, cmd->viewangles );
	}

	// Let the move manager override anything it wants to.
	if ( g_pClientMode->CreateMove( input_sample_frametime, cmd ) )
	{
		// Get current view angles after the client mode tweaks with it
#ifdef SIXENSE
		// Only set the engine angles if sixense is not enabled. It is done in SixenseInput::SetView otherwise.
		if( !g_pSixenseInput->IsEnabled() )
		{
			engine->SetViewAngles( cmd->viewangles );
		}
#else
		engine->SetViewAngles( cmd->viewangles );

#endif

		if ( UseVR() )
		{
			C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
			if( pPlayer && !pPlayer->GetVehicle() )
			{
				QAngle curViewangles, newViewangles;
				Vector curMotion, newMotion;
				engine->GetViewAngles( curViewangles );
				curMotion.Init ( 
					cmd->forwardmove,
					cmd->sidemove,
					cmd->upmove );
				g_ClientVirtualReality.OverridePlayerMotion ( input_sample_frametime, originalViewangles, curViewangles, curMotion, &newViewangles, &newMotion );
				engine->SetViewAngles( newViewangles );
				cmd->forwardmove = newMotion[0];
				cmd->sidemove = newMotion[1];
				cmd->upmove = newMotion[2];
				cmd->viewangles = newViewangles;
			}
			else
			{
				Vector vPos;
				g_ClientVirtualReality.GetTorsoRelativeAim( &vPos, &cmd->viewangles );
				engine->SetViewAngles( cmd->viewangles );
			}
		}
	}

	m_flLastForwardMove = cmd->forwardmove;

	cmd->random_seed = MD5_PseudoRandom( sequence_number ) & 0x7fffffff;

	HLTVCamera()->CreateMove( cmd );
#if defined( REPLAY_ENABLED )
	ReplayCamera()->CreateMove( cmd );
#endif

#if defined( HL2_CLIENT_DLL )
	// copy backchannel data
	int i;
	for (i = 0; i < m_EntityGroundContact.Count(); i++)
	{
		cmd->entitygroundcontact.AddToTail( m_EntityGroundContact[i] );
	}
	m_EntityGroundContact.RemoveAll();
#endif

	pVerified->m_cmd = *cmd;
	pVerified->m_crc = cmd->GetChecksum();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//			buffersize - 
//			slot - 
//-----------------------------------------------------------------------------
void CInput::EncodeUserCmdToBuffer( bf_write& buf, int sequence_number )
{
	CUserCmd nullcmd;
	CUserCmd *cmd = GetUserCmd( sequence_number);

	WriteUsercmd( &buf, cmd, &nullcmd );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//			buffersize - 
//			slot - 
//-----------------------------------------------------------------------------
void CInput::DecodeUserCmdFromBuffer( bf_read& buf, int sequence_number )
{
	CUserCmd nullcmd;
	CUserCmd *cmd = &m_pCommands[ sequence_number % MULTIPLAYER_BACKUP];

	ReadUsercmd( &buf, cmd, &nullcmd );
}

void CInput::ValidateUserCmd( CUserCmd *usercmd, int sequence_number )
{
	// Validate that the usercmd hasn't been changed
	CRC32_t crc = usercmd->GetChecksum();
	if ( crc != m_pVerifiedCommands[ sequence_number % MULTIPLAYER_BACKUP ].m_crc )
	{
		*usercmd = m_pVerifiedCommands[ sequence_number % MULTIPLAYER_BACKUP ].m_cmd;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *buf - 
//			from - 
//			to - 
//-----------------------------------------------------------------------------
bool CInput::WriteUsercmdDeltaToBuffer( bf_write *buf, int from, int to, bool isnewcommand )
{
	Assert( m_pCommands );

	CUserCmd nullcmd;

	CUserCmd *f, *t;

	int startbit = buf->GetNumBitsWritten();

	if ( from == -1 )
	{
		f = &nullcmd;
	}
	else
	{
		f = GetUserCmd( from );

		if ( !f )
		{
			// DevMsg( "WARNING! User command delta too old (from %i, to %i)\n", from, to );
			f = &nullcmd;
		}
		else
		{
			ValidateUserCmd( f, from );
		}
	}

	t = GetUserCmd( to );

	if ( !t )
	{
		// DevMsg( "WARNING! User command too old (from %i, to %i)\n", from, to );
		t = &nullcmd;
	}
	else
	{
		ValidateUserCmd( t, to );
	}

	// Write it into the buffer
	WriteUsercmd( buf, t, f );

	if ( buf->IsOverflowed() )
	{
		int endbit = buf->GetNumBitsWritten();

		Msg( "WARNING! User command buffer overflow(%i %i), last cmd was %i bits long\n",
			from, to,  endbit - startbit );

		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : slot - 
// Output : CUserCmd
//-----------------------------------------------------------------------------
CUserCmd *CInput::GetUserCmd( int sequence_number )
{
	Assert( m_pCommands );

	CUserCmd *usercmd = &m_pCommands[ sequence_number % MULTIPLAYER_BACKUP ];

	if ( usercmd->command_number != sequence_number )
	{
		return NULL;	// usercmd was overwritten by newer command
	}

	return usercmd;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bits - 
//			in_button - 
//			in_ignore - 
//			*button - 
//			reset - 
// Output : static void
//-----------------------------------------------------------------------------
static void CalcButtonBits( int& bits, int in_button, int in_ignore, kbutton_t *button, bool reset )
{
	// Down or still down?
	if ( button->state & 3 )
	{
		bits |= in_button;
	}

	int clearmask = ~2;
	if ( in_ignore & in_button )
	{
		// This gets taken care of below in the GetButtonBits code
		//bits &= ~in_button;
		// Remove "still down" as well as "just down"
		clearmask = ~3;
	}

	if ( reset )
	{
		button->state &= clearmask;
	}
}

/*
============
GetButtonBits

Returns appropriate button info for keyboard and mouse state
Set bResetState to 1 to clear old state info
============
*/
int CInput::GetButtonBits( int bResetState )
{
	int bits = 0;

	CalcButtonBits( bits, IN_SPEED, s_ClearInputState, &in_speed, bResetState );
	CalcButtonBits( bits, IN_WALK, s_ClearInputState, &in_walk, bResetState );
	CalcButtonBits( bits, IN_ATTACK, s_ClearInputState, &in_attack, bResetState );
	CalcButtonBits( bits, IN_DUCK, s_ClearInputState, &in_duck, bResetState );
	CalcButtonBits( bits, IN_JUMP, s_ClearInputState, &in_jump, bResetState );
	CalcButtonBits( bits, IN_FORWARD, s_ClearInputState, &in_forward, bResetState );
	CalcButtonBits( bits, IN_BACK, s_ClearInputState, &in_back, bResetState );
	CalcButtonBits( bits, IN_USE, s_ClearInputState, &in_use, bResetState );
	CalcButtonBits( bits, IN_LEFT, s_ClearInputState, &in_left, bResetState );
	CalcButtonBits( bits, IN_RIGHT, s_ClearInputState, &in_right, bResetState );
	CalcButtonBits( bits, IN_MOVELEFT, s_ClearInputState, &in_moveleft, bResetState );
	CalcButtonBits( bits, IN_MOVERIGHT, s_ClearInputState, &in_moveright, bResetState );
	CalcButtonBits( bits, IN_ATTACK2, s_ClearInputState, &in_attack2, bResetState );
	CalcButtonBits( bits, IN_RELOAD, s_ClearInputState, &in_reload, bResetState );
	CalcButtonBits( bits, IN_ALT1, s_ClearInputState, &in_alt1, bResetState );
	CalcButtonBits( bits, IN_ALT2, s_ClearInputState, &in_alt2, bResetState );
	CalcButtonBits( bits, IN_SCORE, s_ClearInputState, &in_score, bResetState );
	CalcButtonBits( bits, IN_ZOOM, s_ClearInputState, &in_zoom, bResetState );
	CalcButtonBits( bits, IN_GRENADE1, s_ClearInputState, &in_grenade1, bResetState );
	CalcButtonBits( bits, IN_GRENADE2, s_ClearInputState, &in_grenade2, bResetState );
	CalcButtonBits( bits, IN_ATTACK3, s_ClearInputState, &in_attack3, bResetState );

	if ( KeyState(&in_ducktoggle) )
	{
		bits |= IN_DUCK;
	}

	// Cancel is a special flag
	if (in_cancel)
	{
		bits |= IN_CANCEL;
	}

	if ( gHUD.m_iKeyBits & IN_WEAPON1 )
	{
		bits |= IN_WEAPON1;
	}

	if ( gHUD.m_iKeyBits & IN_WEAPON2 )
	{
		bits |= IN_WEAPON2;
	}

	// Clear out any residual
	bits &= ~s_ClearInputState;

	if ( bResetState )
	{
		s_ClearInputState = 0;
	}

	return bits;
}


//-----------------------------------------------------------------------------
// Causes an input to have to be re-pressed to become active
//-----------------------------------------------------------------------------
void CInput::ClearInputButton( int bits )
{
	s_ClearInputState |= bits;
}


/*
==============================
GetLookSpring

==============================
*/
float CInput::GetLookSpring( void )
{
	return lookspring.GetInt();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CInput::GetLastForwardMove( void )
{
	return m_flLastForwardMove;
}


#if defined( HL2_CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: back channel contact info for ground contact
// Output :
//-----------------------------------------------------------------------------

void CInput::AddIKGroundContactInfo( int entindex, float minheight, float maxheight )
{
	CEntityGroundContact data;
	data.entindex = entindex;
	data.minheight = minheight;
	data.maxheight = maxheight;

	if (m_EntityGroundContact.Count() >= MAX_EDICTS)
	{
		// some overflow here, probably bogus anyway
		Assert(0);
		m_EntityGroundContact.RemoveAll();
		return;
	}

	m_EntityGroundContact.AddToTail( data );
}
#endif


static ConCommand startcommandermousemove("+commandermousemove", IN_CommanderMouseMoveDown);
static ConCommand endcommandermousemove("-commandermousemove", IN_CommanderMouseMoveUp);
static ConCommand startmoveup("+moveup",IN_UpDown);
static ConCommand endmoveup("-moveup",IN_UpUp);
static ConCommand startmovedown("+movedown",IN_DownDown);
static ConCommand endmovedown("-movedown",IN_DownUp);
static ConCommand startleft("+left",IN_LeftDown);
static ConCommand endleft("-left",IN_LeftUp);
static ConCommand startright("+right",IN_RightDown);
static ConCommand endright("-right",IN_RightUp);
static ConCommand startforward("+forward",IN_ForwardDown);
static ConCommand endforward("-forward",IN_ForwardUp);
static ConCommand startback("+back",IN_BackDown);
static ConCommand endback("-back",IN_BackUp);
static ConCommand startlookup("+lookup", IN_LookupDown);
static ConCommand endlookup("-lookup", IN_LookupUp);
static ConCommand startlookdown("+lookdown", IN_LookdownDown);
static ConCommand lookdown("-lookdown", IN_LookdownUp);
static ConCommand startstrafe("+strafe", IN_StrafeDown);
static ConCommand endstrafe("-strafe", IN_StrafeUp);
static ConCommand startmoveleft("+moveleft", IN_MoveleftDown);
static ConCommand endmoveleft("-moveleft", IN_MoveleftUp);
static ConCommand startmoveright("+moveright", IN_MoverightDown);
static ConCommand endmoveright("-moveright", IN_MoverightUp);
static ConCommand startspeed("+speed", IN_SpeedDown);
static ConCommand endspeed("-speed", IN_SpeedUp);
static ConCommand startwalk("+walk", IN_WalkDown);
static ConCommand endwalk("-walk", IN_WalkUp);
static ConCommand startattack("+attack", IN_AttackDown);
static ConCommand endattack("-attack", IN_AttackUp);
static ConCommand startattack2("+attack2", IN_Attack2Down);
static ConCommand endattack2("-attack2", IN_Attack2Up);
static ConCommand startuse("+use", IN_UseDown);
static ConCommand enduse("-use", IN_UseUp);
static ConCommand startjump("+jump", IN_JumpDown);
static ConCommand endjump("-jump", IN_JumpUp);
static ConCommand impulse("impulse", IN_Impulse);
static ConCommand startklook("+klook", IN_KLookDown);
static ConCommand endklook("-klook", IN_KLookUp);
static ConCommand startjlook("+jlook", IN_JLookDown);
static ConCommand endjlook("-jlook", IN_JLookUp);
static ConCommand startduck("+duck", IN_DuckDown);
static ConCommand endduck("-duck", IN_DuckUp);
static ConCommand startreload("+reload", IN_ReloadDown);
static ConCommand endreload("-reload", IN_ReloadUp);
static ConCommand startalt1("+alt1", IN_Alt1Down);
static ConCommand endalt1("-alt1", IN_Alt1Up);
static ConCommand startalt2("+alt2", IN_Alt2Down);
static ConCommand endalt2("-alt2", IN_Alt2Up);
static ConCommand startscore("+score", IN_ScoreDown);
static ConCommand endscore("-score", IN_ScoreUp);
static ConCommand startshowscores("+showscores", IN_ScoreDown);
static ConCommand endshowscores("-showscores", IN_ScoreUp);
static ConCommand startgraph("+graph", IN_GraphDown);
static ConCommand endgraph("-graph", IN_GraphUp);
static ConCommand startbreak("+break",IN_BreakDown);
static ConCommand endbreak("-break",IN_BreakUp);
static ConCommand force_centerview("force_centerview", IN_CenterView_f);
static ConCommand joyadvancedupdate("joyadvancedupdate", IN_Joystick_Advanced_f, "", FCVAR_CLIENTCMD_CAN_EXECUTE);
static ConCommand startzoom("+zoom", IN_ZoomDown);
static ConCommand endzoom("-zoom", IN_ZoomUp);
static ConCommand endgrenade1( "-grenade1", IN_Grenade1Up );
static ConCommand startgrenade1( "+grenade1", IN_Grenade1Down );
static ConCommand endgrenade2( "-grenade2", IN_Grenade2Up );
static ConCommand startgrenade2( "+grenade2", IN_Grenade2Down );
static ConCommand startattack3("+attack3", IN_Attack3Down);
static ConCommand endattack3("-attack3", IN_Attack3Up);

#ifdef TF_CLIENT_DLL
static ConCommand toggle_duck( "toggle_duck", IN_DuckToggle );
#endif

// Xbox 360 stub commands
static ConCommand xboxmove("xmove", IN_XboxStub);
static ConCommand xboxlook("xlook", IN_XboxStub);

/*
============
Init_All
============
*/
void CInput::Init_All (void)
{
	Assert( !m_pCommands );
	m_pCommands = new CUserCmd[ MULTIPLAYER_BACKUP ];
	m_pVerifiedCommands = new CVerifiedUserCmd[ MULTIPLAYER_BACKUP ];

	m_fMouseInitialized	= false;
	m_fRestoreSPI		= false;
	m_fMouseActive		= false;
	Q_memset( m_rgOrigMouseParms, 0, sizeof( m_rgOrigMouseParms ) );
	Q_memset( m_rgNewMouseParms, 0, sizeof( m_rgNewMouseParms ) );
	Q_memset( m_rgCheckMouseParam, 0, sizeof( m_rgCheckMouseParam ) );

	m_rgNewMouseParms[ MOUSE_ACCEL_THRESHHOLD1 ] = 0;	// no 2x
	m_rgNewMouseParms[ MOUSE_ACCEL_THRESHHOLD2 ] = 0;	// no 4x
	m_rgNewMouseParms[ MOUSE_SPEED_FACTOR ] = 1;		// 0 = disabled, 1 = threshold 1 enabled, 2 = threshold 2 enabled

	m_fMouseParmsValid	= false;
	m_fJoystickAdvancedInit = false;
	m_fHadJoysticks = false;
	m_flLastForwardMove = 0.0;

	// Make sure this is activated now so steam controller stuff works
	ClientSteamContext().Activate();

	// Initialize inputs
	if ( IsPC() )
	{
		Init_Mouse ();
		Init_Keyboard();
	}
		
	// Initialize third person camera controls.
	Init_Camera();

	// Initialize steam controller action sets
	m_bSteamControllerGameActionsInitialized = InitializeSteamControllerGameActionSets();
}

/*
============
Shutdown_All
============
*/
void CInput::Shutdown_All(void)
{
	DeactivateMouse();
	Shutdown_Keyboard();

	delete[] m_pCommands;
	m_pCommands = NULL;

	delete[] m_pVerifiedCommands;
	m_pVerifiedCommands = NULL;
}

void CInput::LevelInit( void )
{
#if defined( HL2_CLIENT_DLL )
	// Remove any IK information
	m_EntityGroundContact.RemoveAll();
#endif
}

