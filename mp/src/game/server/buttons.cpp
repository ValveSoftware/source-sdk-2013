//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements buttons.
//
//=============================================================================

#include "cbase.h"
#include "doors.h"
#include "ndebugoverlay.h"
#include "spark.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "tier1/strtools.h"
#include "buttons.h"
#include "eventqueue.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void PlayLockSounds( CBaseEntity *pEdict, locksound_t *pls, int flocked, int fbutton );
string_t MakeButtonSound( int sound );				// get string of button sound number


#define SF_BUTTON_DONTMOVE				1
#define SF_ROTBUTTON_NOTSOLID			1
#define	SF_BUTTON_TOGGLE				32		// button stays pushed until reactivated
#define SF_BUTTON_TOUCH_ACTIVATES		256		// Button fires when touched.
#define SF_BUTTON_DAMAGE_ACTIVATES		512		// Button fires when damaged.
#define SF_BUTTON_USE_ACTIVATES			1024	// Button fires when used.
#define SF_BUTTON_LOCKED				2048	// Whether the button is initially locked.
#define	SF_BUTTON_SPARK_IF_OFF			4096	// button sparks in OFF state
#define	SF_BUTTON_JIGGLE_ON_USE_LOCKED	8192	// whether to jiggle if someone uses us when we're locked

BEGIN_DATADESC( CBaseButton )

	DEFINE_KEYFIELD( m_vecMoveDir, FIELD_VECTOR, "movedir" ),
	DEFINE_FIELD( m_fStayPushed, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_fRotating, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_bLockedSound, FIELD_CHARACTER ),
	DEFINE_FIELD( m_bLockedSentence, FIELD_CHARACTER ),
	DEFINE_FIELD( m_bUnlockedSound, FIELD_CHARACTER ),	
	DEFINE_FIELD( m_bUnlockedSentence, FIELD_CHARACTER ),
	DEFINE_FIELD( m_bLocked, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_sNoise, FIELD_SOUNDNAME ),
	DEFINE_FIELD( m_flUseLockedTime, FIELD_TIME ),
	DEFINE_FIELD( m_bSolidBsp, FIELD_BOOLEAN ),
	
	DEFINE_KEYFIELD( m_sounds, FIELD_INTEGER, "sounds" ),
	
//	DEFINE_FIELD( m_ls, FIELD_SOUNDNAME ),   // This is restored in Precache()
//  DEFINE_FIELD( m_nState, FIELD_INTEGER ),

	// Function Pointers
	DEFINE_FUNCTION( ButtonTouch ),
	DEFINE_FUNCTION( ButtonSpark ),
	DEFINE_FUNCTION( TriggerAndWait ),
	DEFINE_FUNCTION( ButtonReturn ),
	DEFINE_FUNCTION( ButtonBackHome ),
	DEFINE_FUNCTION( ButtonUse ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Lock", InputLock ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Unlock", InputUnlock ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Press", InputPress ),
	DEFINE_INPUTFUNC( FIELD_VOID, "PressIn", InputPressIn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "PressOut", InputPressOut ),

	// Outputs
	DEFINE_OUTPUT( m_OnDamaged, "OnDamaged" ),
	DEFINE_OUTPUT( m_OnPressed, "OnPressed" ),
	DEFINE_OUTPUT( m_OnUseLocked, "OnUseLocked" ),
	DEFINE_OUTPUT( m_OnIn, "OnIn" ),
	DEFINE_OUTPUT( m_OnOut, "OnOut" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( func_button, CBaseButton );



void CBaseButton::Precache( void )
{
	// get door button sounds, for doors which require buttons to open
	if (m_bLockedSound)
	{
		m_ls.sLockedSound = MakeButtonSound( (int)m_bLockedSound );
		PrecacheScriptSound(m_ls.sLockedSound.ToCStr());
	}

	if (m_bUnlockedSound)
	{
		m_ls.sUnlockedSound = MakeButtonSound( (int)m_bUnlockedSound );
		PrecacheScriptSound(m_ls.sUnlockedSound.ToCStr());
	}

	// get sentence group names, for doors which are directly 'touched' to open

	switch (m_bLockedSentence)
	{
		case 1: m_ls.sLockedSentence = MAKE_STRING("NA"); break; // access denied
		case 2: m_ls.sLockedSentence = MAKE_STRING("ND"); break; // security lockout
		case 3: m_ls.sLockedSentence = MAKE_STRING("NF"); break; // blast door
		case 4: m_ls.sLockedSentence = MAKE_STRING("NFIRE"); break; // fire door
		case 5: m_ls.sLockedSentence = MAKE_STRING("NCHEM"); break; // chemical door
		case 6: m_ls.sLockedSentence = MAKE_STRING("NRAD"); break; // radiation door
		case 7: m_ls.sLockedSentence = MAKE_STRING("NCON"); break; // gen containment
		case 8: m_ls.sLockedSentence = MAKE_STRING("NH"); break; // maintenance door
		case 9: m_ls.sLockedSentence = MAKE_STRING("NG"); break; // broken door
		
		default: m_ls.sLockedSentence = NULL_STRING; break;
	}

	switch (m_bUnlockedSentence)
	{
		case 1: m_ls.sUnlockedSentence = MAKE_STRING("EA"); break; // access granted
		case 2: m_ls.sUnlockedSentence = MAKE_STRING("ED"); break; // security door
		case 3: m_ls.sUnlockedSentence = MAKE_STRING("EF"); break; // blast door
		case 4: m_ls.sUnlockedSentence = MAKE_STRING("EFIRE"); break; // fire door
		case 5: m_ls.sUnlockedSentence = MAKE_STRING("ECHEM"); break; // chemical door
		case 6: m_ls.sUnlockedSentence = MAKE_STRING("ERAD"); break; // radiation door
		case 7: m_ls.sUnlockedSentence = MAKE_STRING("ECON"); break; // gen containment
		case 8: m_ls.sUnlockedSentence = MAKE_STRING("EH"); break; // maintenance door
	
		default: m_ls.sUnlockedSentence = NULL_STRING; break;
	}

	if ( m_sNoise != NULL_STRING )
	{
		PrecacheScriptSound( STRING( m_sNoise ) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Cache user-entity-field values until spawn is called.
// Input  : szKeyName - 
//			szValue - 
// Output : Returns true if handled, false if not.
//-----------------------------------------------------------------------------
bool CBaseButton::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "locked_sound"))
	{
		m_bLockedSound = atof(szValue);
	}
	else if (FStrEq(szKeyName, "locked_sentence"))
	{
		m_bLockedSentence = atof(szValue);
	}
	else if (FStrEq(szKeyName, "unlocked_sound"))
	{
		m_bUnlockedSound = atof(szValue);
	}
	else if (FStrEq(szKeyName, "unlocked_sentence"))
	{
		m_bUnlockedSentence = atof(szValue);
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Locks the button. If locked, the button will play the locked sound
//			when the player tries to use it.
//-----------------------------------------------------------------------------
void CBaseButton::Lock()
{
	m_bLocked = true;
}


//-----------------------------------------------------------------------------
// Purpose: Unlocks the button, making it able to be pressed again.
//-----------------------------------------------------------------------------
void CBaseButton::Unlock()
{
	m_bLocked = false;
}


//-----------------------------------------------------------------------------
// Purpose: Locks the button. If locked, the button will play the locked sound
//			when the player tries to use it.
//-----------------------------------------------------------------------------
void CBaseButton::InputLock( inputdata_t &inputdata )
{
	Lock();
}


//-----------------------------------------------------------------------------
// Purpose: Unlocks the button, making it able to be pressed again.
//-----------------------------------------------------------------------------
void CBaseButton::InputUnlock( inputdata_t &inputdata )
{
	Unlock();
}


//-----------------------------------------------------------------------------
// Presses or unpresses the button.
//-----------------------------------------------------------------------------
void CBaseButton::Press( CBaseEntity *pActivator, BUTTON_CODE eCode )
{
	if ( ( eCode == BUTTON_PRESS ) && ( m_toggle_state == TS_GOING_UP || m_toggle_state == TS_GOING_DOWN ) )
	{
		return;
	}

	if ( ( eCode == BUTTON_ACTIVATE ) && ( m_toggle_state == TS_GOING_UP || m_toggle_state == TS_AT_TOP ) )
	{
		return;
	}

	if ( ( eCode == BUTTON_RETURN ) && ( m_toggle_state == TS_GOING_DOWN || m_toggle_state == TS_AT_BOTTOM ) )
	{
		return;
	}

	// FIXME: consolidate all the button press code into one place!
	if (m_bLocked)
	{
		// play button locked sound
		PlayLockSounds(this, &m_ls, TRUE, TRUE);
		return;
	}

	// Temporarily disable the touch function, until movement is finished.
	SetTouch( NULL );

	if ( ( ( eCode == BUTTON_PRESS ) && ( m_toggle_state == TS_AT_TOP ) ) ||
		 ( ( eCode == BUTTON_RETURN ) && ( m_toggle_state == TS_AT_TOP || m_toggle_state == TS_GOING_UP ) ) )
	{
		if ( m_sNoise != NULL_STRING )
		{
			CPASAttenuationFilter filter( this );

			EmitSound_t ep;
			ep.m_nChannel = CHAN_VOICE;
			ep.m_pSoundName = (char*)STRING(m_sNoise);
			ep.m_flVolume = 1;
			ep.m_SoundLevel = SNDLVL_NORM;

			EmitSound( filter, entindex(), ep );
		}

		m_OnPressed.FireOutput(pActivator, this);
		ButtonReturn();
	}
	else if ( ( eCode == BUTTON_PRESS ) ||
			  ( ( eCode == BUTTON_ACTIVATE ) && ( m_toggle_state == TS_AT_BOTTOM || m_toggle_state == TS_GOING_DOWN ) ) )
	{
		m_OnPressed.FireOutput(pActivator, this);
		ButtonActivate();
	}
}


//-----------------------------------------------------------------------------
// Presses the button.
//-----------------------------------------------------------------------------
void CBaseButton::InputPress( inputdata_t &inputdata )
{
	Press( inputdata.pActivator, BUTTON_PRESS );
}


//-----------------------------------------------------------------------------
// Presses the button, sending it to the top/pressed position.
//-----------------------------------------------------------------------------
void CBaseButton::InputPressIn( inputdata_t &inputdata )
{
	Press( inputdata.pActivator, BUTTON_ACTIVATE );
}


//-----------------------------------------------------------------------------
// Unpresses the button, sending it to the unpressed/bottom position.
//-----------------------------------------------------------------------------
void CBaseButton::InputPressOut( inputdata_t &inputdata )
{
	Press( inputdata.pActivator, BUTTON_RETURN );
}


//-----------------------------------------------------------------------------
// Purpose: We have been damaged. Possibly activate, depending on our flags.
// Input  : pInflictor - 
//			pAttacker - 
//			flDamage - 
//			bitsDamageType - 
// Output : 
//-----------------------------------------------------------------------------
int CBaseButton::OnTakeDamage( const CTakeDamageInfo &info )
{
	m_OnDamaged.FireOutput(m_hActivator, this);

	// dvsents2: remove obselete health keyvalue from func_button
	if (!HasSpawnFlags(SF_BUTTON_DAMAGE_ACTIVATES) && (m_iHealth == 0))
	{
		return(0);
	}

	BUTTON_CODE code = ButtonResponseToTouch();

	if ( code == BUTTON_NOTHING )
		return 0;

	m_hActivator = info.GetAttacker();

	// dvsents2: why would activator be NULL here?
	if ( m_hActivator == NULL )
		return 0;

	if (m_bLocked)
	{
		return(0);
	}

	// Temporarily disable the touch function, until movement is finished.
	SetTouch( NULL );

	if ( code == BUTTON_RETURN )
	{
		if ( m_sNoise != NULL_STRING )
		{
			CPASAttenuationFilter filter( this );

			EmitSound_t ep;
			ep.m_nChannel = CHAN_VOICE;
			ep.m_pSoundName = (char*)STRING(m_sNoise);
			ep.m_flVolume = 1;
			ep.m_SoundLevel = SNDLVL_NORM;

			EmitSound( filter, entindex(), ep );
		}

		m_OnPressed.FireOutput(m_hActivator, this);
		ButtonReturn();
	}
	else
	{
		// code == BUTTON_ACTIVATE
		m_OnPressed.FireOutput(m_hActivator, this);
		ButtonActivate( );
	}

	return 0;
}


void CBaseButton::Spawn( )
{ 
	//----------------------------------------------------
	//determine sounds for buttons
	//a sound of 0 should not make a sound
	//----------------------------------------------------
	if ( m_sounds )
	{
		m_sNoise = MakeButtonSound( m_sounds );
		PrecacheScriptSound(m_sNoise.ToCStr());
	}
	else
	{
		m_sNoise = NULL_STRING;
	}

	Precache();

	if ( HasSpawnFlags( SF_BUTTON_SPARK_IF_OFF ) )// this button should spark in OFF state
	{
		SetThink ( &CBaseButton::ButtonSpark );
		SetNextThink( gpGlobals->curtime + 0.5f );// no hurry, make sure everything else spawns
	}

	// Convert movedir from angles to a vector
	QAngle angMoveDir = QAngle( m_vecMoveDir.x, m_vecMoveDir.y, m_vecMoveDir.z );
	AngleVectors( angMoveDir, &m_vecMoveDir );

	SetMoveType( MOVETYPE_PUSH );
	SetSolid( SOLID_BSP );
	SetModel( STRING( GetModelName() ) );
	
	if (m_flSpeed == 0)
	{
		m_flSpeed = 40;
	}

	m_takedamage = DAMAGE_YES;

	if (m_flWait == 0)
	{
		m_flWait = 1;
	}

	if (m_flLip == 0)
	{
		m_flLip = 4;
	}

	m_toggle_state = TS_AT_BOTTOM;
	m_vecPosition1 = GetLocalOrigin();

	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	Vector vecButtonOBB = CollisionProp()->OBBSize();
	vecButtonOBB -= Vector( 2, 2, 2 );
	m_vecPosition2	= m_vecPosition1 + (m_vecMoveDir * (DotProductAbs( m_vecMoveDir, vecButtonOBB ) - m_flLip));

	// Is this a non-moving button?
	if ( ((m_vecPosition2 - m_vecPosition1).Length() < 1) || HasSpawnFlags(SF_BUTTON_DONTMOVE) )
	{
		m_vecPosition2 = m_vecPosition1;
	}

	m_fStayPushed = (m_flWait == -1 ? TRUE : FALSE);
	m_fRotating = FALSE;

	if (HasSpawnFlags(SF_BUTTON_LOCKED))
	{
		m_bLocked = true;
	}

	//
	// If using activates the button, set its use function.
	//
	if (HasSpawnFlags(SF_BUTTON_USE_ACTIVATES))
	{
		SetUse(&CBaseButton::ButtonUse);
	}
	else
	{
		SetUse(NULL);
	}

	//
	// If touching activates the button, set its touch function.
	//
	if (HasSpawnFlags(SF_BUTTON_TOUCH_ACTIVATES))
	{
		SetTouch( &CBaseButton::ButtonTouch );
	}
	else 
	{
		SetTouch ( NULL );
	}

	CreateVPhysics();
}

//-----------------------------------------------------------------------------

bool CBaseButton::CreateVPhysics()
{
	VPhysicsInitShadow( false, false );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Button sound table.
//			Also used by CBaseDoor to get 'touched' door lock/unlock sounds
// Input  : sound - index of sound to look up.
// Output : Returns a pointer to the corresponding sound file.
//-----------------------------------------------------------------------------
string_t MakeButtonSound( int sound )
{ 
	char tmp[1024];
	Q_snprintf( tmp, sizeof(tmp), "Buttons.snd%d", sound );
	return AllocPooledString(tmp);
}


//-----------------------------------------------------------------------------
// Purpose: Think function that emits sparks at random intervals.
//-----------------------------------------------------------------------------
void CBaseButton::ButtonSpark ( void )
{
	SetThink ( &CBaseButton::ButtonSpark );
	SetNextThink( gpGlobals->curtime + 0.1 + random->RandomFloat ( 0, 1.5 ) );// spark again at random interval

	DoSpark( this, WorldSpaceCenter(), 1, 1, true, vec3_origin );
}


//-----------------------------------------------------------------------------
// Purpose: Called when someone uses us whilst we are locked.
//-----------------------------------------------------------------------------
bool CBaseButton::OnUseLocked( CBaseEntity *pActivator )
{
	PlayLockSounds(this, &m_ls, TRUE, TRUE);

	if ( gpGlobals->curtime > m_flUseLockedTime )
	{
		m_OnUseLocked.FireOutput( pActivator, this );
		m_flUseLockedTime = gpGlobals->curtime + 0.5;
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Use function that starts the button moving.
// Input  : pActivator - 
//			pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CBaseButton::ButtonUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Ignore touches if button is moving, or pushed-in and waiting to auto-come-out.
	// UNDONE: Should this use ButtonResponseToTouch() too?
	if (m_toggle_state == TS_GOING_UP || m_toggle_state == TS_GOING_DOWN )
		return;		

	if (m_bLocked)
	{
		OnUseLocked( pActivator );
		return;
	}

	m_hActivator = pActivator;

	if ( m_toggle_state == TS_AT_TOP)
	{
		//
		// If it's a toggle button it can return now. Otherwise, it will either
		// return on its own or will stay pressed indefinitely.
		//
		if ( HasSpawnFlags(SF_BUTTON_TOGGLE))
		{
			if ( m_sNoise != NULL_STRING )
			{
				CPASAttenuationFilter filter( this );

				EmitSound_t ep;
				ep.m_nChannel = CHAN_VOICE;
				ep.m_pSoundName = (char*)STRING(m_sNoise);
				ep.m_flVolume = 1;
				ep.m_SoundLevel = SNDLVL_NORM;

				EmitSound( filter, entindex(), ep );
			}

			m_OnPressed.FireOutput(m_hActivator, this);
			ButtonReturn();
		}
	}
	else
	{
		m_OnPressed.FireOutput(m_hActivator, this);
		ButtonActivate( );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns a code indicating how the button should respond to being touched.
// Output : Returns one of the following:
//				BUTTON_NOTHING - do nothing
//				BUTTON_RETURN - 
//				BUTTON_ACTIVATE - act as if pressed
//-----------------------------------------------------------------------------
CBaseButton::BUTTON_CODE CBaseButton::ButtonResponseToTouch( void )
{
	// Ignore touches if button is moving, or pushed-in and waiting to auto-come-out.
	if (m_toggle_state == TS_GOING_UP ||
		m_toggle_state == TS_GOING_DOWN ||
		(m_toggle_state == TS_AT_TOP && !m_fStayPushed && !HasSpawnFlags(SF_BUTTON_TOGGLE) ) )
		return BUTTON_NOTHING;

	if (m_toggle_state == TS_AT_TOP)
	{
		if ( HasSpawnFlags(SF_BUTTON_TOGGLE) && !m_fStayPushed)
		{
			return BUTTON_RETURN;
		}
	}
	else
		return BUTTON_ACTIVATE;

	return BUTTON_NOTHING;
}


//-----------------------------------------------------------------------------
// Purpose: Touch function that activates the button if it responds to touch.
// Input  : pOther - The entity that touched us.
//-----------------------------------------------------------------------------
void CBaseButton::ButtonTouch( CBaseEntity *pOther )
{
	// Ignore touches by anything but players
	if ( !pOther->IsPlayer() )
		return;

	m_hActivator = pOther;

	BUTTON_CODE code = ButtonResponseToTouch();

	if ( code == BUTTON_NOTHING )
		return;

	if (!UTIL_IsMasterTriggered(m_sMaster, pOther) || m_bLocked)
	{
		// play button locked sound
		PlayLockSounds(this, &m_ls, TRUE, TRUE);
		return;
	}

	// Temporarily disable the touch function, until movement is finished.
	SetTouch( NULL );

	if ( code == BUTTON_RETURN )
	{
		if ( m_sNoise != NULL_STRING )
		{
			CPASAttenuationFilter filter( this );

			EmitSound_t ep;
			ep.m_nChannel = CHAN_VOICE;
			ep.m_pSoundName = (char*)STRING(m_sNoise);
			ep.m_flVolume = 1;
			ep.m_SoundLevel = SNDLVL_NORM;

			EmitSound( filter, entindex(), ep );
		}

		m_OnPressed.FireOutput(m_hActivator, this);
		ButtonReturn();
	}
	else
	{
		// code == BUTTON_ACTIVATE
		m_OnPressed.FireOutput(m_hActivator, this);
		ButtonActivate( );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Starts the button moving "in/up".
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CBaseButton::ButtonActivate( void )
{
	if ( m_sNoise != NULL_STRING )
	{
		CPASAttenuationFilter filter( this );

		EmitSound_t ep;
		ep.m_nChannel = CHAN_VOICE;
		ep.m_pSoundName = (char*)STRING(m_sNoise);
		ep.m_flVolume = 1;
		ep.m_SoundLevel = SNDLVL_NORM;

		EmitSound( filter, entindex(), ep );
	}
	
	if (!UTIL_IsMasterTriggered(m_sMaster, m_hActivator) || m_bLocked)
	{
		// button is locked, play locked sound
		PlayLockSounds(this, &m_ls, TRUE, TRUE);
		return;
	}
	else
	{
		// button is unlocked, play unlocked sound
		PlayLockSounds(this, &m_ls, FALSE, TRUE);
	}

	ASSERT(m_toggle_state == TS_AT_BOTTOM);
	m_toggle_state = TS_GOING_UP;
	
	SetMoveDone( &CBaseButton::TriggerAndWait );
	if (!m_fRotating)
		LinearMove( m_vecPosition2, m_flSpeed);
	else
		AngularMove( m_vecAngle2, m_flSpeed);
}


//-----------------------------------------------------------------------------
// Purpose: Enables or disables the use capability based on our spawnflags.
//-----------------------------------------------------------------------------
int	CBaseButton::ObjectCaps(void)
{
	return((BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) |
			(HasSpawnFlags(SF_BUTTON_USE_ACTIVATES) ? (FCAP_IMPULSE_USE | FCAP_USE_IN_RADIUS) : 0));
}


//-----------------------------------------------------------------------------
// Purpose: Button has reached the "pressed/top" position. Fire its OnIn output,
//			and pause before returning to "unpressed/bottom".
//-----------------------------------------------------------------------------
void CBaseButton::TriggerAndWait( void )
{
	ASSERT(m_toggle_state == TS_GOING_UP);

	if (!UTIL_IsMasterTriggered(m_sMaster, m_hActivator) || m_bLocked)
	{
		return;
	}

	m_toggle_state = TS_AT_TOP;
	
	//
	// Re-instate touches if the button is of the toggle variety.
	//
	if (m_fStayPushed || HasSpawnFlags(SF_BUTTON_TOGGLE ) )
	{
		if (HasSpawnFlags(SF_BUTTON_TOUCH_ACTIVATES))
		{
			SetTouch(&CBaseButton::ButtonTouch);
		}
		else
		{
			// BUGBUG: ALL buttons no longer respond to touch
			SetTouch (NULL);
		}
	}

	//
	// If button automatically comes back out, start it moving out.
	//
	else
	{
		SetNextThink( gpGlobals->curtime + m_flWait );
		SetThink( &CBaseButton::ButtonReturn );
	}
	
	m_nState = 1;			// use alternate textures

	m_OnIn.FireOutput(m_hActivator, this);
}


//-----------------------------------------------------------------------------
// Purpose: Starts the button moving "out/down".
//-----------------------------------------------------------------------------
void CBaseButton::ButtonReturn( void )
{
	ASSERT(m_toggle_state == TS_AT_TOP);
	m_toggle_state = TS_GOING_DOWN;
	
	SetMoveDone( &CBaseButton::ButtonBackHome );
	if (!m_fRotating)
		LinearMove( m_vecPosition1, m_flSpeed);
	else
		AngularMove( m_vecAngle1, m_flSpeed);

	m_nState = 0;			// use normal textures
}


//-----------------------------------------------------------------------------
// Purpose: Button has returned to the "unpressed/bottom" position. Fire its
//			OnOut output and stop moving.
//-----------------------------------------------------------------------------
void CBaseButton::ButtonBackHome( void )
{
	ASSERT(m_toggle_state == TS_GOING_DOWN);
	m_toggle_state = TS_AT_BOTTOM;

	m_OnOut.FireOutput(m_hActivator, this);

	//
	// Re-instate touch method, movement cycle is complete.
	//
	if (HasSpawnFlags(SF_BUTTON_TOUCH_ACTIVATES))
	{
		SetTouch( &CBaseButton::ButtonTouch );
	}
	else
	{
		// BUGBUG: ALL buttons no longer respond to touch
		SetTouch ( NULL );
	}

	// reset think for a sparking button
	if (HasSpawnFlags( SF_BUTTON_SPARK_IF_OFF ) )
	{
		SetThink ( &CBaseButton::ButtonSpark );
		SetNextThink( gpGlobals->curtime + 0.5f );// no hurry
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CBaseButton::DrawDebugTextOverlays()
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		static const char *pszStates[] =
		{
			"Pressed",
			"Unpressed",
			"Pressing...",
			"Unpressing...",
			"<UNKNOWN STATE>",
		};
	
		char tempstr[255];

		int nState = m_toggle_state;
		if ( ( nState < 0 ) || ( nState > 3 ) )
		{
			nState = 4;
		}

		Q_snprintf( tempstr, sizeof(tempstr), "State: %s", pszStates[nState] );
		EntityText( text_offset, tempstr, 0 );
		text_offset++;

		Q_snprintf( tempstr, sizeof(tempstr), "%s", m_bLocked ? "Locked" : "Unlocked" );
		EntityText( text_offset, tempstr, 0 );
		text_offset++;
	}
	return text_offset;
}


//
// Rotating button (aka "lever")
//
LINK_ENTITY_TO_CLASS( func_rot_button, CRotButton );


void CRotButton::Spawn( void )
{
	//----------------------------------------------------
	//determine sounds for buttons
	//a sound of 0 should not make a sound
	//----------------------------------------------------
	if ( m_sounds )
	{
		m_sNoise = MakeButtonSound( m_sounds );
		PrecacheScriptSound(m_sNoise.ToCStr());
	}
	else
	{
		m_sNoise = NULL_STRING;
	}

	// set the axis of rotation
	CBaseToggle::AxisDir();

	// check for clockwise rotation
	if ( HasSpawnFlags( SF_DOOR_ROTATE_BACKWARDS) )
	{
		m_vecMoveAng = m_vecMoveAng * -1;
	}

	SetMoveType( MOVETYPE_PUSH );
	
#ifdef HL1_DLL
	SetSolid( SOLID_BSP );
#else
	SetSolid( SOLID_VPHYSICS );
#endif
	if ( HasSpawnFlags( SF_ROTBUTTON_NOTSOLID ) )
	{
		AddEFlags( EFL_USE_PARTITION_WHEN_NOT_SOLID );
		AddSolidFlags( FSOLID_NOT_SOLID );
	}

	SetModel( STRING( GetModelName() ) );
	
	if (m_flSpeed == 0)
		m_flSpeed = 40;

	if (m_flWait == 0)
		m_flWait = 1;

	if (m_iHealth > 0)
	{
		m_takedamage = DAMAGE_YES;
	}

	m_toggle_state = TS_AT_BOTTOM;
	m_vecAngle1	= GetLocalAngles();
	m_vecAngle2	= GetLocalAngles() + m_vecMoveAng * m_flMoveDistance;
	ASSERTSZ(m_vecAngle1 != m_vecAngle2, "rotating button start/end positions are equal\n");

	m_fStayPushed = (m_flWait == -1 ? TRUE : FALSE);
	m_fRotating = TRUE;

	SetUse(&CRotButton::ButtonUse);

	//
	// If touching activates the button, set its touch function.
	//
	if (!HasSpawnFlags(SF_BUTTON_TOUCH_ACTIVATES))
	{
		SetTouch ( NULL );
	}
	else
	{
		SetTouch( &CRotButton::ButtonTouch );
	}

	CreateVPhysics();
}


bool CRotButton::CreateVPhysics( void )
{
	VPhysicsInitShadow( false, false );
	return true;
}


//-----------------------------------------------------------------------------
// CMomentaryRotButton spawnflags
//-----------------------------------------------------------------------------
#define SF_MOMENTARY_DOOR			1
#define SF_MOMENTARY_NOT_USABLE		2
#define SF_MOMENTARY_AUTO_RETURN	16


BEGIN_DATADESC( CMomentaryRotButton )

	DEFINE_FIELD( m_lastUsed, FIELD_INTEGER ),
	DEFINE_FIELD( m_start, FIELD_VECTOR ),
	DEFINE_FIELD( m_end, FIELD_VECTOR ),
	DEFINE_FIELD( m_IdealYaw, FIELD_FLOAT ),
	DEFINE_FIELD( m_sNoise, FIELD_SOUNDNAME ),
	DEFINE_FIELD( m_bUpdateTarget, FIELD_BOOLEAN ),

	DEFINE_KEYFIELD( m_direction, FIELD_INTEGER, "StartDirection" ),
	DEFINE_KEYFIELD( m_returnSpeed, FIELD_FLOAT, "returnspeed" ),
	DEFINE_KEYFIELD( m_flStartPosition, FIELD_FLOAT, "StartPosition"),
	DEFINE_KEYFIELD( m_bSolidBsp, FIELD_BOOLEAN, "solidbsp" ),

	// Function Pointers
	DEFINE_FUNCTION( UseMoveDone ),
	DEFINE_FUNCTION( ReturnMoveDone ),
	DEFINE_FUNCTION( SetPositionMoveDone ),
	DEFINE_FUNCTION( UpdateThink ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetPosition", InputSetPosition ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetPositionImmediately", InputSetPositionImmediately ),
	DEFINE_INPUTFUNC( FIELD_VOID, "_DisableUpdateTarget", InputDisableUpdateTarget ),
	DEFINE_INPUTFUNC( FIELD_VOID, "_EnableUpdateTarget", InputEnableUpdateTarget ),

	// Outputs
	DEFINE_OUTPUT( m_Position, "Position" ),
	DEFINE_OUTPUT( m_OnUnpressed, "OnUnpressed" ),
	DEFINE_OUTPUT( m_OnFullyClosed, "OnFullyClosed" ),
	DEFINE_OUTPUT( m_OnFullyOpen, "OnFullyOpen" ),
	DEFINE_OUTPUT( m_OnReachedPosition, "OnReachedPosition" ),

	DEFINE_INPUTFUNC( FIELD_VOID,	"Enable",	InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Disable",	InputDisable ),
	DEFINE_FIELD( m_bDisabled, FIELD_BOOLEAN )

END_DATADESC()


LINK_ENTITY_TO_CLASS( momentary_rot_button, CMomentaryRotButton );


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CMomentaryRotButton::Spawn( void )
{
	CBaseToggle::AxisDir();

	m_bUpdateTarget = true;

	if ( m_flSpeed == 0 )
	{
		m_flSpeed = 100;
	}

	// Clamp start position and issue bounds warning
	if (m_flStartPosition < 0.0f || m_flStartPosition > 1.0f)
	{
		Warning("WARNING: Momentary door (%s) start position not between 0 and 1.  Clamping.\n",GetDebugName());
		m_flStartPosition = clamp(m_IdealYaw, 0.f, 1.f);
	}

	// Check direction fields (for backward compatibility)
	if (m_direction != 1 && m_direction != -1)
	{
		m_direction = 1;
	}

	if (m_flMoveDistance < 0)
	{
		m_vecMoveAng = m_vecMoveAng * -1;
		m_flMoveDistance = -m_flMoveDistance;
	}

	m_start = GetLocalAngles() - m_vecMoveAng * m_flMoveDistance * m_flStartPosition;
	m_end	= GetLocalAngles() + m_vecMoveAng * m_flMoveDistance * (1-m_flStartPosition);

	m_IdealYaw			= m_flStartPosition;

	// Force start direction at end points
	if (m_flStartPosition == 0.0)
	{
		m_direction = -1;
	}
	else if (m_flStartPosition == 1.0)
	{
		m_direction = 1;
	}

	if (HasSpawnFlags(SF_BUTTON_LOCKED))
	{
		m_bLocked = true;
	}

	if ( HasSpawnFlags( SF_BUTTON_USE_ACTIVATES ) )
	{
		if ( m_sounds )
		{
			m_sNoise = MakeButtonSound( m_sounds );
			PrecacheScriptSound(m_sNoise.ToCStr());
		}
		else
		{
			m_sNoise = NULL_STRING;
		}

		m_lastUsed	= 0;
		UpdateTarget(0,this);
	}

#ifdef HL1_DLL
	SetSolid( SOLID_BSP );
#else
	SetSolid( SOLID_VPHYSICS );
#endif
	if (HasSpawnFlags(SF_ROTBUTTON_NOTSOLID))
	{
		AddEFlags( EFL_USE_PARTITION_WHEN_NOT_SOLID );
		AddSolidFlags( FSOLID_NOT_SOLID );
	}

	SetMoveType( MOVETYPE_PUSH );
	SetModel( STRING( GetModelName() ) );
	
	CreateVPhysics();

	// Slam the object back to solid - if we really want it to be solid.
	if ( m_bSolidBsp )
	{
		SetSolid( SOLID_BSP );
	}

	m_bDisabled = false;
}

int	CMomentaryRotButton::ObjectCaps( void ) 
{ 
	int flags = BaseClass::ObjectCaps();
	if (!HasSpawnFlags(SF_BUTTON_USE_ACTIVATES))
	{
		return flags;
	}
	else
	{	
		return (flags | FCAP_CONTINUOUS_USE | FCAP_USE_IN_RADIUS);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMomentaryRotButton::CreateVPhysics( void )
{
	VPhysicsInitShadow( false, false );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMomentaryRotButton::PlaySound( void )
{
	if ( m_sNoise == NULL_STRING )
		return;

	CPASAttenuationFilter filter( this );

	EmitSound_t ep;
	ep.m_nChannel = CHAN_VOICE;
	ep.m_pSoundName = (char*)STRING(m_sNoise);
	ep.m_flVolume = 1;
	ep.m_SoundLevel = SNDLVL_NORM;

	EmitSound( filter, entindex(), ep );
}


//-----------------------------------------------------------------------------
// Purpose: Returns a given angular position as a value along our motion from 0 to 1.
// Input  : vecAngles - 
//-----------------------------------------------------------------------------
float CMomentaryRotButton::GetPos( const QAngle &vecAngles )
{
	float flScale = 1;
	if (( m_vecMoveAng[0] < 0 ) || ( m_vecMoveAng[1] < 0 ) || ( m_vecMoveAng[2] < 0 ))
	{
		flScale = -1;
	}

	float flPos = flScale * CBaseToggle::AxisDelta( m_spawnflags, vecAngles, m_start ) / m_flMoveDistance;
	return( clamp( flPos, 0.f, 1.f ));
}


//------------------------------------------------------------------------------
// Purpose :
// Input   : flPosition 
//------------------------------------------------------------------------------
void CMomentaryRotButton::InputSetPosition( inputdata_t &inputdata )
{
	m_IdealYaw = clamp( inputdata.value.Float(), 0.f, 1.f );

	float flCurPos = GetPos( GetLocalAngles() );
	if ( flCurPos < m_IdealYaw )
	{
		// Moving forward (from start to end).
		SetLocalAngularVelocity( m_flSpeed * m_vecMoveAng );
		m_direction = 1;
	}
	else if ( flCurPos > m_IdealYaw )
	{
		// Moving backward (from end to start).
		SetLocalAngularVelocity( -m_flSpeed * m_vecMoveAng );
		m_direction = -1;
	}
	else
	{
		// We're there already; nothing to do.
		SetLocalAngularVelocity( vec3_angle );
		return;
	}

	SetMoveDone( &CMomentaryRotButton::SetPositionMoveDone );

	SetThink( &CMomentaryRotButton::UpdateThink );
	SetNextThink( gpGlobals->curtime );

	//
	// Think again in 0.1 seconds or the time that it will take us to reach our movement goal,
	// whichever is the shorter interval. This prevents us from overshooting and stuttering when we
	// are told to change position in very small increments.
	//
	QAngle vecNewAngles = m_start + m_vecMoveAng * ( m_IdealYaw * m_flMoveDistance );
	float flAngleDelta = fabs( AxisDelta( m_spawnflags, vecNewAngles, GetLocalAngles() ));
	float dt = flAngleDelta / m_flSpeed;
	if ( dt < TICK_INTERVAL )
	{
		dt = TICK_INTERVAL;
		float speed = flAngleDelta / TICK_INTERVAL;
		SetLocalAngularVelocity( speed * m_vecMoveAng * m_direction );
	}
	dt = clamp( dt, TICK_INTERVAL, TICK_INTERVAL * 6);

	SetMoveDoneTime( dt );
}


//------------------------------------------------------------------------------
// Purpose :
// Input   : flPosition 
//------------------------------------------------------------------------------
void CMomentaryRotButton::InputSetPositionImmediately( inputdata_t &inputdata )
{
	m_IdealYaw = clamp( inputdata.value.Float(), 0.f, 1.f );
	SetLocalAngles( m_start + m_vecMoveAng * ( m_IdealYaw * m_flMoveDistance ) );
}


//------------------------------------------------------------------------------
// Purpose: Turns off target updates so that we can change the wheel's position
//			without changing the target's position. Used for jiggling when locked.
//------------------------------------------------------------------------------
void CMomentaryRotButton::InputDisableUpdateTarget( inputdata_t &inputdata )
{
	m_bUpdateTarget = false;
}


//------------------------------------------------------------------------------
// Purpose: Turns target updates back on (after jiggling).
//------------------------------------------------------------------------------
void CMomentaryRotButton::InputEnableUpdateTarget( inputdata_t &inputdata )
{
	m_bUpdateTarget = true;
}


//-----------------------------------------------------------------------------
// Purpose: Locks the button. If locked, the button will play the locked sound
//			when the player tries to use it.
//-----------------------------------------------------------------------------
void CMomentaryRotButton::Lock()
{
	BaseClass::Lock();

	SetLocalAngularVelocity( vec3_angle );
	SetMoveDoneTime( -1 );
	SetMoveDone( NULL );

	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
}


//-----------------------------------------------------------------------------
// Purpose: Unlocks the button, making it able to be pressed again.
//-----------------------------------------------------------------------------
void CMomentaryRotButton::Unlock()
{
	BaseClass::Unlock();

	SetMoveDone( &CMomentaryRotButton::ReturnMoveDone );

	// Delay before autoreturn.
	SetMoveDoneTime( 0.1f );
}


//-----------------------------------------------------------------------------
// Purpose: Fires the appropriate outputs at the extremes of motion.
//-----------------------------------------------------------------------------
void CMomentaryRotButton::OutputMovementComplete( void )
{
	if (m_IdealYaw == 1.0)
	{
		m_OnFullyClosed.FireOutput(this, this);
	}
	else if (m_IdealYaw == 0.0)
	{
		m_OnFullyOpen.FireOutput(this, this);
	}

	m_OnReachedPosition.FireOutput( this, this );
}


//------------------------------------------------------------------------------
// Purpose: MoveDone function for the SetPosition input handler. Tracks our
//			progress toward a movement goal and updates our outputs.
//------------------------------------------------------------------------------
void CMomentaryRotButton::SetPositionMoveDone(void)
{
	float flCurPos = GetPos( GetLocalAngles() );

	if ((( flCurPos >= m_IdealYaw ) && ( m_direction == 1 )) ||
		(( flCurPos <= m_IdealYaw ) && ( m_direction == -1 )))
	{
		//
		// We reached or surpassed our movement goal.
		//
		SetLocalAngularVelocity( vec3_angle );
		// BUGBUG: Won't this get the player stuck?
		SetLocalAngles( m_start + m_vecMoveAng * ( m_IdealYaw * m_flMoveDistance ) );
		SetNextThink( TICK_NEVER_THINK );
		SetMoveDoneTime( -1 );
		UpdateTarget( m_IdealYaw, this );
		OutputMovementComplete();
		return;
	}

	// TODO: change this to use a Think function like ReturnThink.
	QAngle vecNewAngles = m_start + m_vecMoveAng * ( m_IdealYaw * m_flMoveDistance );
	float flAngleDelta = fabs( AxisDelta( m_spawnflags, vecNewAngles, GetLocalAngles() ));
	float dt = flAngleDelta / m_flSpeed;
	if ( dt < TICK_INTERVAL )
	{
		dt = TICK_INTERVAL;
		float speed = flAngleDelta / TICK_INTERVAL;
		SetLocalAngularVelocity( speed * m_vecMoveAng * m_direction );
	}
	dt = clamp( dt, TICK_INTERVAL, TICK_INTERVAL * 6);

	SetMoveDoneTime( dt );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pActivator - 
//			pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CMomentaryRotButton::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( m_bDisabled == true )
		return;

	if (m_bLocked)
	{
		if ( OnUseLocked( pActivator ) && HasSpawnFlags( SF_BUTTON_JIGGLE_ON_USE_LOCKED ) )
		{
			// Jiggle two degrees.
			float flDist = 2.0 / m_flMoveDistance;

			// Must be first!
			g_EventQueue.AddEvent( this, "_DisableUpdateTarget", 0, this, this );

			variant_t value;
			value.SetFloat( flDist );
			g_EventQueue.AddEvent( this, "SetPosition", value, 0.01, this, this );

			value.SetFloat( 0.0 );
			g_EventQueue.AddEvent( this, "SetPosition", value, 0.1, this, this );

			value.SetFloat( 0.5 * flDist );
			g_EventQueue.AddEvent( this, "SetPosition", value, 0.2, this, this );

			value.SetFloat( 0.0 );
			g_EventQueue.AddEvent( this, "SetPosition", value, 0.3, this, this );

			// Must be last! And must be late enough to cover the settling time.
			g_EventQueue.AddEvent( this, "_EnableUpdateTarget", 0.5, this, this );
		}

		return;
	}

	//
	// Reverse our direction and play movement sound every time the player
	// pauses between uses.
	//
	bool bPlaySound = false;
	
	if ( !m_lastUsed )
	{
		bPlaySound = true;
		m_direction = -m_direction;

		//Alert that we've been pressed
		m_OnPressed.FireOutput( m_hActivator, this );
	}

	m_lastUsed = 1;

	float flPos = GetPos( GetLocalAngles() );
	UpdateSelf( flPos, bPlaySound );

	//
	// Think every frame while we are moving.
	// HACK: Don't reset the think time if we already have a pending think.
	// This works around an issue with host_thread_mode > 0 when the player's
	// clock runs ahead of the server.
	//
	if ( !m_pfnThink )
	{
		SetThink( &CMomentaryRotButton::UpdateThink );
		SetNextThink( gpGlobals->curtime );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles changing direction at the extremes of our range of motion
//			and updating our avelocity while being used by the player.
// Input  : value - Number from 0 to 1 indicating our desired position within
//				our range of motion, 0 = start, 1 = end.
//-----------------------------------------------------------------------------
void CMomentaryRotButton::UpdateSelf( float value, bool bPlaySound )
{
	//
	// Set our move clock to 0.1 seconds in the future so we stop spinning unless we are
	// used again before then.
	//
	SetMoveDoneTime( 0.1 );

	//
	// If we hit the end, zero our avelocity and snap to the end angles.
	//
	if ( m_direction > 0 && value >= 1.0 )
	{
		SetLocalAngularVelocity( vec3_angle );
		SetLocalAngles( m_end );

		m_OnFullyClosed.FireOutput(this, this);
		return;
	}
	//
	// If we returned to the start, zero our avelocity and snap to the start angles.
	//
	else if ( m_direction < 0 && value <= 0 )
	{
		SetLocalAngularVelocity( vec3_angle );
		SetLocalAngles( m_start );

		m_OnFullyOpen.FireOutput(this, this);
		return;
	}
	
	if ( bPlaySound )
	{
		PlaySound();
	}

	SetLocalAngularVelocity( ( m_direction * m_flSpeed ) * m_vecMoveAng );
	SetMoveDone( &CMomentaryRotButton::UseMoveDone );
}


//-----------------------------------------------------------------------------
// Purpose: Updates the value of our position, firing any targets.
// Input  : value - New position, from 0 - 1.
//-----------------------------------------------------------------------------
void CMomentaryRotButton::UpdateTarget( float value, CBaseEntity *pActivator )
{
	if ( !m_bUpdateTarget )
		return;

	if (m_Position.Get() != value)
	{
		m_Position.Set(value, pActivator, this);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handles the end of motion caused by player use.
//-----------------------------------------------------------------------------
void CMomentaryRotButton::UseMoveDone( void )
{
	SetLocalAngularVelocity( vec3_angle );

	// Make sure our targets stop where we stopped.
	float flPos = GetPos( GetLocalAngles() );
	UpdateTarget( flPos, this );

	// Alert that we've been unpressed
	m_OnUnpressed.FireOutput( m_hActivator, this );

	m_lastUsed = 0;

	if ( !HasSpawnFlags( SF_BUTTON_TOGGLE ) && m_returnSpeed > 0 )
	{
		SetMoveDone( &CMomentaryRotButton::ReturnMoveDone );
		m_direction = -1;

		// Delay before autoreturn.
		SetMoveDoneTime( 0.1f );
	}
	else
	{
		SetThink( NULL );
		SetMoveDone( NULL );
	}
}


//-----------------------------------------------------------------------------
// Purpose: MoveDone function for rotating back to the start position.
//-----------------------------------------------------------------------------
void CMomentaryRotButton::ReturnMoveDone( void )
{
	float value = GetPos( GetLocalAngles() );
	if ( value <= 0 )
	{
		//
		// Got back to the start, stop spinning.
		//
		SetLocalAngularVelocity( vec3_angle );
		SetLocalAngles( m_start );

		UpdateTarget( 0, NULL );

		SetMoveDoneTime( -1 );
		SetMoveDone( NULL );

		SetNextThink( TICK_NEVER_THINK );
		SetThink( NULL );
	}
	else
	{
		SetLocalAngularVelocity( -m_returnSpeed * m_vecMoveAng );
		SetMoveDoneTime( 0.1f );

		SetThink( &CMomentaryRotButton::UpdateThink );
		SetNextThink( gpGlobals->curtime + 0.01f );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Think function for updating target as we move.
//-----------------------------------------------------------------------------
void CMomentaryRotButton::UpdateThink( void )
{
	float value = GetPos( GetLocalAngles() );
	UpdateTarget( value, NULL );
	SetNextThink( gpGlobals->curtime );
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CMomentaryRotButton::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[255];
		
		Q_snprintf(tempstr,sizeof(tempstr),"QAngle: %.2f %.2f %.2f", GetLocalAngles()[0], GetLocalAngles()[1], GetLocalAngles()[2]);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"AVelocity: %.2f %.2f %.2f", GetLocalAngularVelocity()[0], GetLocalAngularVelocity()[1], GetLocalAngularVelocity()[2]);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Target Pos:   %3.3f",m_IdealYaw);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		float flCurPos = GetPos(GetLocalAngles());
		Q_snprintf(tempstr,sizeof(tempstr),"Current Pos:   %3.3f",flCurPos);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Direction: %s",(m_direction == 1) ? "Forward" : "Backward");
		EntityText(text_offset,tempstr,0);
		text_offset++;

	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Input hander that starts the spawner
//-----------------------------------------------------------------------------
void CMomentaryRotButton::InputEnable( inputdata_t &inputdata )
{
	Enable();
}


//-----------------------------------------------------------------------------
// Purpose: Input hander that stops the spawner
//-----------------------------------------------------------------------------
void CMomentaryRotButton::InputDisable( inputdata_t &inputdata )
{
	Disable();
}

//-----------------------------------------------------------------------------
// Purpose: Start the spawner
//-----------------------------------------------------------------------------
void CMomentaryRotButton::Enable( void )
{
	m_bDisabled = false;
}


//-----------------------------------------------------------------------------
// Purpose: Stop the spawner
//-----------------------------------------------------------------------------
void CMomentaryRotButton::Disable( void )
{
	m_bDisabled = true;
}