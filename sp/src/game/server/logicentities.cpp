//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements many of the entities that control logic flow within a map.
//
//=============================================================================

#include "cbase.h"
#include "entityinput.h"
#include "entityoutput.h"
#include "eventqueue.h"
#include "mathlib/mathlib.h"
#include "globalstate.h"
#include "ndebugoverlay.h"
#include "saverestore_utlvector.h"
#include "vstdlib/random.h"
#include "gameinterface.h"
#ifdef MAPBASE
#include "mapbase/variant_tools.h"
#include "mapbase/matchers.h"
#include "mapbase/datadesc_mod.h"
#include "activitylist.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern CServerGameDLL g_ServerGameDLL;

//-----------------------------------------------------------------------------
// Purpose: An entity that acts as a container for game scripts.
//-----------------------------------------------------------------------------

#define MAX_SCRIPT_GROUP 16

class CLogicScript : public CPointEntity
{
public:
	DECLARE_CLASS( CLogicScript, CPointEntity );
	DECLARE_DATADESC();

	void RunVScripts()
	{
		/*
			EntityGroup <- [];
			function __AppendToScriptGroup( name ) 
			{
				if ( name.len() == 0 ) 
				{ 
					EntityGroup.append( null ); 
				} 
				else
				{ 
					local ent = Entities.FindByName( null, name );
					EntityGroup.append( ent );
					if ( ent != null )
					{
						ent.ValidateScriptScope();
						ent.GetScriptScope().EntityGroup <- EntityGroup;
					}
				}
			}
		*/

 		static const char szAddCode[] =
		{
			0x45,0x6e,0x74,0x69,0x74,0x79,0x47,0x72,0x6f,0x75,0x70,0x20,0x3c,0x2d,0x20,0x5b,0x5d,0x3b,0x0d,0x0a,
			0x66,0x75,0x6e,0x63,0x74,0x69,0x6f,0x6e,0x20,0x5f,0x5f,0x41,0x70,0x70,0x65,0x6e,0x64,0x54,0x6f,0x53,
			0x63,0x72,0x69,0x70,0x74,0x47,0x72,0x6f,0x75,0x70,0x28,0x20,0x6e,0x61,0x6d,0x65,0x20,0x29,0x20,0x0d,
			0x0a,0x7b,0x0d,0x0a,0x09,0x69,0x66,0x20,0x28,0x20,0x6e,0x61,0x6d,0x65,0x2e,0x6c,0x65,0x6e,0x28,0x29,
			0x20,0x3d,0x3d,0x20,0x30,0x20,0x29,0x20,0x0d,0x0a,0x09,0x7b,0x20,0x0d,0x0a,0x09,0x09,0x45,0x6e,0x74,
			0x69,0x74,0x79,0x47,0x72,0x6f,0x75,0x70,0x2e,0x61,0x70,0x70,0x65,0x6e,0x64,0x28,0x20,0x6e,0x75,0x6c,
			0x6c,0x20,0x29,0x3b,0x20,0x0d,0x0a,0x09,0x7d,0x20,0x0d,0x0a,0x09,0x65,0x6c,0x73,0x65,0x0d,0x0a,0x09,
			0x7b,0x20,0x0d,0x0a,0x09,0x09,0x6c,0x6f,0x63,0x61,0x6c,0x20,0x65,0x6e,0x74,0x20,0x3d,0x20,0x45,0x6e,
			0x74,0x69,0x74,0x69,0x65,0x73,0x2e,0x46,0x69,0x6e,0x64,0x42,0x79,0x4e,0x61,0x6d,0x65,0x28,0x20,0x6e,
			0x75,0x6c,0x6c,0x2c,0x20,0x6e,0x61,0x6d,0x65,0x20,0x29,0x3b,0x0d,0x0a,0x09,0x09,0x45,0x6e,0x74,0x69,
			0x74,0x79,0x47,0x72,0x6f,0x75,0x70,0x2e,0x61,0x70,0x70,0x65,0x6e,0x64,0x28,0x20,0x65,0x6e,0x74,0x20,
			0x29,0x3b,0x0d,0x0a,0x09,0x09,0x69,0x66,0x20,0x28,0x20,0x65,0x6e,0x74,0x20,0x21,0x3d,0x20,0x6e,0x75,
			0x6c,0x6c,0x20,0x29,0x0d,0x0a,0x09,0x09,0x7b,0x0d,0x0a,0x09,0x09,0x09,0x65,0x6e,0x74,0x2e,0x56,0x61,
			0x6c,0x69,0x64,0x61,0x74,0x65,0x53,0x63,0x72,0x69,0x70,0x74,0x53,0x63,0x6f,0x70,0x65,0x28,0x29,0x3b,
			0x0d,0x0a,0x09,0x09,0x09,0x65,0x6e,0x74,0x2e,0x47,0x65,0x74,0x53,0x63,0x72,0x69,0x70,0x74,0x53,0x63,
			0x6f,0x70,0x65,0x28,0x29,0x2e,0x45,0x6e,0x74,0x69,0x74,0x79,0x47,0x72,0x6f,0x75,0x70,0x20,0x3c,0x2d,
			0x20,0x45,0x6e,0x74,0x69,0x74,0x79,0x47,0x72,0x6f,0x75,0x70,0x3b,0x0d,0x0a,0x09,0x09,0x7d,0x0d,0x0a,
			0x09,0x7d,0x0d,0x0a,0x7d,0x0d,0x0a,0x00
		};

		int iLastMember;
		for ( iLastMember = MAX_SCRIPT_GROUP - 1; iLastMember >= 0; iLastMember-- )
		{
			if ( m_iszGroupMembers[iLastMember] != NULL_STRING )
			{
				break;
			}
		}

		if ( iLastMember >= 0 )
		{
			HSCRIPT hAddScript = g_pScriptVM->CompileScript( szAddCode );
			if ( hAddScript )
			{
				ValidateScriptScope();
				m_ScriptScope.Run( hAddScript );
				HSCRIPT hAddFunc = m_ScriptScope.LookupFunction( "__AppendToScriptGroup" );
				if ( hAddFunc )
				{
					for ( int i = 0; i <= iLastMember; i++ )
					{
						m_ScriptScope.Call( hAddFunc, NULL, STRING(m_iszGroupMembers[i]) );
					}
					g_pScriptVM->ReleaseFunction( hAddFunc );
					m_ScriptScope.ClearValue( "__AppendToScriptGroup" );
				}

				g_pScriptVM->ReleaseScript( hAddScript );
			}
		}
		BaseClass::RunVScripts();
	}

	string_t m_iszGroupMembers[MAX_SCRIPT_GROUP];

};

LINK_ENTITY_TO_CLASS( logic_script, CLogicScript );

BEGIN_DATADESC( CLogicScript )
	// Silence, Classcheck!
	// DEFINE_ARRAY( m_iszGroupMembers, FIELD_STRING, MAX_NUM_TEMPLATES ),

	DEFINE_KEYFIELD( m_iszGroupMembers[0], FIELD_STRING, "Group00"),
	DEFINE_KEYFIELD( m_iszGroupMembers[1], FIELD_STRING, "Group01"),
	DEFINE_KEYFIELD( m_iszGroupMembers[2], FIELD_STRING, "Group02"),
	DEFINE_KEYFIELD( m_iszGroupMembers[3], FIELD_STRING, "Group03"),
	DEFINE_KEYFIELD( m_iszGroupMembers[4], FIELD_STRING, "Group04"),
	DEFINE_KEYFIELD( m_iszGroupMembers[5], FIELD_STRING, "Group05"),
	DEFINE_KEYFIELD( m_iszGroupMembers[6], FIELD_STRING, "Group06"),
	DEFINE_KEYFIELD( m_iszGroupMembers[7], FIELD_STRING, "Group07"),
	DEFINE_KEYFIELD( m_iszGroupMembers[8], FIELD_STRING, "Group08"),
	DEFINE_KEYFIELD( m_iszGroupMembers[9], FIELD_STRING, "Group09"),
	DEFINE_KEYFIELD( m_iszGroupMembers[10], FIELD_STRING, "Group10"),
	DEFINE_KEYFIELD( m_iszGroupMembers[11], FIELD_STRING, "Group11"),
	DEFINE_KEYFIELD( m_iszGroupMembers[12], FIELD_STRING, "Group12"),
	DEFINE_KEYFIELD( m_iszGroupMembers[13], FIELD_STRING, "Group13"),
	DEFINE_KEYFIELD( m_iszGroupMembers[14], FIELD_STRING, "Group14"),
	DEFINE_KEYFIELD( m_iszGroupMembers[15], FIELD_STRING, "Group15"),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: Compares a set of integer inputs to the one main input
//			Outputs true if they are all equivalant, false otherwise
//-----------------------------------------------------------------------------
class CLogicCompareInteger : public CLogicalEntity
{
public:
	DECLARE_CLASS( CLogicCompareInteger, CLogicalEntity );

	// outputs
#ifdef MAPBASE
	COutputVariant m_OnEqual;
	COutputVariant m_OnNotEqual;
#else
	COutputEvent m_OnEqual;
	COutputEvent m_OnNotEqual;
#endif

	// data
#ifdef MAPBASE
	variant_t m_iValue;
	bool m_iShouldCompareToValue;
	bool m_bStrLenAllowed = true;
	int DrawDebugTextOverlays(void);
#else
	int m_iIntegerValue;
	int m_iShouldCompareToValue;
#endif

	DECLARE_DATADESC();

	CMultiInputVar m_AllIntCompares;

	// Input handlers
	void InputValue( inputdata_t &inputdata );
#ifdef MAPBASE
	void InputValueNoFire( inputdata_t &inputdata );
	void InputSetIntegerValue( inputdata_t &inputdata );
#endif
	void InputCompareValues( inputdata_t &inputdata );
};


LINK_ENTITY_TO_CLASS( logic_multicompare, CLogicCompareInteger );


BEGIN_DATADESC( CLogicCompareInteger )

	DEFINE_OUTPUT( m_OnEqual, "OnEqual" ),
	DEFINE_OUTPUT( m_OnNotEqual, "OnNotEqual" ),

#ifdef MAPBASE
	DEFINE_KEYVARIANT( m_iValue, "IntegerValue" ),
	DEFINE_KEYFIELD( m_iShouldCompareToValue, FIELD_BOOLEAN, "ShouldComparetoValue" ),
	DEFINE_KEYFIELD( m_bStrLenAllowed, FIELD_BOOLEAN, "StrLenAllowed" ),
#else
	DEFINE_KEYFIELD( m_iIntegerValue, FIELD_INTEGER, "IntegerValue" ),
	DEFINE_KEYFIELD( m_iShouldCompareToValue, FIELD_INTEGER, "ShouldComparetoValue" ),
#endif

	DEFINE_FIELD( m_AllIntCompares, FIELD_INPUT ),

	DEFINE_INPUTFUNC( FIELD_INPUT, "InputValue", InputValue ),
#ifdef MAPBASE
	DEFINE_INPUTFUNC( FIELD_INPUT, "InputValueNoFire", InputValueNoFire ),
	DEFINE_INPUTFUNC( FIELD_INPUT, "SetReferenceValue", InputSetIntegerValue ),
#endif
	DEFINE_INPUTFUNC( FIELD_INPUT, "CompareValues", InputCompareValues ),

END_DATADESC()




//-----------------------------------------------------------------------------
// Purpose: Adds to the list of compared values
//-----------------------------------------------------------------------------
void CLogicCompareInteger::InputValue( inputdata_t &inputdata )
{
#ifdef MAPBASE
	// Parse the input value, regardless of field type
	inputdata.value = Variant_ParseInput(inputdata);
#else
	// make sure it's an int, if it can't be converted just throw it away
	if ( !inputdata.value.Convert(FIELD_INTEGER) )
		return;
#endif

	// update the value list with the new value
	m_AllIntCompares.AddValue( inputdata.value, inputdata.nOutputID );

	// if we haven't already this frame, send a message to ourself to update and fire
	if ( !m_AllIntCompares.m_bUpdatedThisFrame )
	{
#ifdef MAPBASE
		// Need to wait for all inputs to arrive
		g_EventQueue.AddEvent( this, "CompareValues", 0.01, inputdata.pActivator, this, inputdata.nOutputID );
#else
		// TODO: need to add this event with a lower priority, so it gets called after all inputs have arrived
		g_EventQueue.AddEvent( this, "CompareValues", 0, inputdata.pActivator, this, inputdata.nOutputID );
#endif
		m_AllIntCompares.m_bUpdatedThisFrame = TRUE;
	}
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: Adds to the list of compared values without firing
//-----------------------------------------------------------------------------
void CLogicCompareInteger::InputValueNoFire( inputdata_t &inputdata )
{
	// Parse the input value, regardless of field type
	inputdata.value = Variant_ParseInput(inputdata);

	// update the value list with the new value
	m_AllIntCompares.AddValue( inputdata.value, inputdata.nOutputID );
}

//-----------------------------------------------------------------------------
// Purpose: Sets our reference value
//-----------------------------------------------------------------------------
void CLogicCompareInteger::InputSetIntegerValue( inputdata_t &inputdata )
{
	m_iValue = Variant_ParseInput(inputdata);
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Forces a recompare
//-----------------------------------------------------------------------------
void CLogicCompareInteger::InputCompareValues( inputdata_t &inputdata )
{
	m_AllIntCompares.m_bUpdatedThisFrame = FALSE;

	// loop through all the values comparing them
#ifdef MAPBASE
	variant_t value = m_iValue;
	CMultiInputVar::inputitem_t *input = m_AllIntCompares.m_InputList;

	if ( !m_iShouldCompareToValue && input )
	{
		value = input->value;
	}

	while ( input )
	{
		if ( !Variant_Equal(value, input->value, m_bStrLenAllowed) )
		{
			// false
			m_OnNotEqual.Set( input->value, inputdata.pActivator, this );
			return;
		}

		input = input->next;
	}

	// true! all values equal
	m_OnEqual.Set( value, inputdata.pActivator, this );
#else
	int value = m_iIntegerValue;
	CMultiInputVar::inputitem_t *input = m_AllIntCompares.m_InputList;

	if ( !m_iShouldCompareToValue && input )
	{
		value = input->value.Int();
	}

	while ( input )
	{
		if ( input->value.Int() != value )
		{
			// false
			m_OnNotEqual.FireOutput( inputdata.pActivator, this );
			return;
		}

		input = input->next;
	}

	// true! all values equal
	m_OnEqual.FireOutput( inputdata.pActivator, this );
#endif
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CLogicCompareInteger::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		Q_snprintf(tempstr, sizeof(tempstr), "    Reference Value: %s", m_iValue.GetDebug());
		EntityText(text_offset, tempstr, 0);
		text_offset++;

		int count = 1;
		CMultiInputVar::inputitem_t *input = m_AllIntCompares.m_InputList;
		while ( input )
		{
			Q_snprintf(tempstr, sizeof(tempstr), "    Value %i: %s", count, input->value.GetDebug());
			EntityText(text_offset, tempstr, 0);
			text_offset++;

			count++;
			input = input->next;
		}
	}
	return text_offset;
}
#endif


//-----------------------------------------------------------------------------
// Purpose: Timer entity. Fires an output at regular or random intervals.
//-----------------------------------------------------------------------------
//
// Spawnflags and others constants.
//
const int SF_TIMER_UPDOWN = 1;
const float LOGIC_TIMER_MIN_INTERVAL = 0.01;


class CTimerEntity : public CLogicalEntity
{
public:
	DECLARE_CLASS( CTimerEntity, CLogicalEntity );

	void Spawn( void );
	void Think( void );

	void Toggle( void );
	void Enable( void );
	void Disable( void );
	void FireTimer( void );

	int DrawDebugTextOverlays(void);

	// outputs
	COutputEvent m_OnTimer;
	COutputEvent m_OnTimerHigh;
	COutputEvent m_OnTimerLow;

	// inputs
	void InputToggle( inputdata_t &inputdata );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputFireTimer( inputdata_t &inputdata );
	void InputRefireTime( inputdata_t &inputdata );
	void InputResetTimer( inputdata_t &inputdata );
	void InputAddToTimer( inputdata_t &inputdata );
	void InputSubtractFromTimer( inputdata_t &inputdata );

	int m_iDisabled;
	float m_flRefireTime;
	bool m_bUpDownState;
	int m_iUseRandomTime;
	float m_flLowerRandomBound;
	float m_flUpperRandomBound;
#ifdef MAPBASE
	bool m_bUseBoundsForTimerInputs;
#endif

	// methods
	void ResetTimer( void );
	
	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( logic_timer, CTimerEntity );


BEGIN_DATADESC( CTimerEntity )

	// Keys
	DEFINE_KEYFIELD( m_iDisabled, FIELD_INTEGER, "StartDisabled" ),
	DEFINE_KEYFIELD( m_flRefireTime, FIELD_FLOAT, "RefireTime" ),

	DEFINE_FIELD( m_bUpDownState, FIELD_BOOLEAN ),

#ifdef MAPBASE
	DEFINE_KEYFIELD( m_bUseBoundsForTimerInputs, FIELD_BOOLEAN, "UseBoundsForTimerInputs" ),
#endif

	// Inputs
	DEFINE_INPUTFUNC( FIELD_FLOAT, "RefireTime", InputRefireTime ),
	DEFINE_INPUTFUNC( FIELD_VOID, "FireTimer", InputFireTimer ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddToTimer", InputAddToTimer ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ResetTimer", InputResetTimer ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SubtractFromTimer", InputSubtractFromTimer ),

	DEFINE_INPUT( m_iUseRandomTime, FIELD_INTEGER, "UseRandomTime" ),
	DEFINE_INPUT( m_flLowerRandomBound, FIELD_FLOAT, "LowerRandomBound" ),
	DEFINE_INPUT( m_flUpperRandomBound, FIELD_FLOAT, "UpperRandomBound" ),


	// Outputs
	DEFINE_OUTPUT( m_OnTimer, "OnTimer" ),
	DEFINE_OUTPUT( m_OnTimerHigh, "OnTimerHigh" ),
	DEFINE_OUTPUT( m_OnTimerLow, "OnTimerLow" ),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTimerEntity::Spawn( void )
{
	if (!m_iUseRandomTime && (m_flRefireTime < LOGIC_TIMER_MIN_INTERVAL))
	{
		m_flRefireTime = LOGIC_TIMER_MIN_INTERVAL;
	}

	if ( !m_iDisabled && (m_flRefireTime > 0 || m_iUseRandomTime) )
	{
		Enable();
	}
	else
	{
		Disable();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTimerEntity::Think( void )
{
	FireTimer();
}


//-----------------------------------------------------------------------------
// Purpose: Sets the time the timerentity will next fire
//-----------------------------------------------------------------------------
void CTimerEntity::ResetTimer( void )
{
	if ( m_iDisabled )
		return;

	if ( m_iUseRandomTime )
	{
		m_flRefireTime = random->RandomFloat( m_flLowerRandomBound, m_flUpperRandomBound );
	}

	SetNextThink( gpGlobals->curtime + m_flRefireTime );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTimerEntity::Enable( void )
{
	m_iDisabled = FALSE;
	ResetTimer();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTimerEntity::Disable( void )
{
	m_iDisabled = TRUE;
	SetNextThink( TICK_NEVER_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTimerEntity::Toggle( void )
{
	if ( m_iDisabled )
	{
		Enable();
	}
	else
	{
		Disable();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTimerEntity::FireTimer( void )
{
	if ( !m_iDisabled )
	{
		//
		// Up/down timers alternate between two outputs.
		//
		if (m_spawnflags & SF_TIMER_UPDOWN)
		{
			if (m_bUpDownState)
			{
				m_OnTimerHigh.FireOutput( this, this );
			}
			else
			{
				m_OnTimerLow.FireOutput( this, this );
			}

			m_bUpDownState = !m_bUpDownState;
		}
		//
		// Regular timers only fire a single output.
		//
		else
		{
			m_OnTimer.FireOutput( this, this );
		}

		ResetTimer();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTimerEntity::InputEnable( inputdata_t &inputdata )
{
	Enable();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTimerEntity::InputDisable( inputdata_t &inputdata )
{
	Disable();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTimerEntity::InputToggle( inputdata_t &inputdata )
{
	Toggle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTimerEntity::InputFireTimer( inputdata_t &inputdata )
{
	FireTimer();
}


//-----------------------------------------------------------------------------
// Purpose: Changes the time interval between timer fires
//			Resets the next firing to be time + newRefireTime
// Input  : Float refire frequency in seconds.
//-----------------------------------------------------------------------------
void CTimerEntity::InputRefireTime( inputdata_t &inputdata )
{
	float flRefireInterval = inputdata.value.Float();

	if ( flRefireInterval < LOGIC_TIMER_MIN_INTERVAL)
	{
		flRefireInterval = LOGIC_TIMER_MIN_INTERVAL;
	}

	if (m_flRefireTime != flRefireInterval )
	{
		m_flRefireTime = flRefireInterval;
		ResetTimer();
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CTimerEntity::InputResetTimer( inputdata_t &inputdata )
{
	// don't reset the timer if it isn't enabled
	if ( m_iDisabled )
		return;

	ResetTimer();
}


//-----------------------------------------------------------------------------
// Purpose: Adds to the time interval if the timer is enabled
// Input  : Float time to add in seconds
//-----------------------------------------------------------------------------
void CTimerEntity::InputAddToTimer( inputdata_t &inputdata )
{
	// don't add time if the timer isn't enabled
	if ( m_iDisabled )
		return;
	
	// Add time to timer
 	float flNextThink = GetNextThink();	
#ifdef MAPBASE
	if (m_bUseBoundsForTimerInputs)
	{
		// Make sure it's not above our min or max bounds
		if (flNextThink - gpGlobals->curtime > m_flUpperRandomBound)
			return;

		flNextThink += inputdata.value.Float();
		flNextThink = clamp( flNextThink - gpGlobals->curtime, m_flLowerRandomBound, m_flUpperRandomBound );
		SetNextThink( gpGlobals->curtime + flNextThink );
	}
	else
	{
		SetNextThink( flNextThink + inputdata.value.Float() );
	}
#else
	SetNextThink( flNextThink += inputdata.value.Float() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Subtract from the time interval if the timer is enabled
// Input  : Float time to subtract in seconds
//-----------------------------------------------------------------------------
void CTimerEntity::InputSubtractFromTimer( inputdata_t &inputdata )
{
	// don't add time if the timer isn't enabled
	if ( m_iDisabled )
		return;

	// Subtract time from the timer but don't let the timer go negative
	float flNextThink = GetNextThink();
#ifdef MAPBASE
	if (m_bUseBoundsForTimerInputs)
	{
		// Make sure it's not above our min or max bounds
		if (flNextThink - gpGlobals->curtime < m_flLowerRandomBound)
			return;

		flNextThink -= inputdata.value.Float();
		flNextThink = clamp( flNextThink - gpGlobals->curtime, m_flLowerRandomBound, m_flUpperRandomBound );
		SetNextThink( gpGlobals->curtime + flNextThink );
	}
	else
	{
		flNextThink -= inputdata.value.Float();
		SetNextThink( (flNextThink <= gpGlobals->curtime) ? gpGlobals->curtime : flNextThink );
	}
#else
	if ( ( flNextThink - gpGlobals->curtime ) <= inputdata.value.Float() )
	{
		SetNextThink( gpGlobals->curtime );
	}
	else
	{
		SetNextThink( flNextThink -= inputdata.value.Float() );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CTimerEntity::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		// print refire time
		Q_snprintf(tempstr,sizeof(tempstr),"refire interval: %.2f sec", m_flRefireTime);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		// print seconds to next fire
		if ( !m_iDisabled )
		{
			float flNextThink = GetNextThink();
			Q_snprintf( tempstr, sizeof( tempstr ), "      firing in: %.2f sec", flNextThink - gpGlobals->curtime );
			EntityText( text_offset, tempstr, 0);
			text_offset++;
		}
	}
	return text_offset;
}


//-----------------------------------------------------------------------------
// Purpose: Computes a line between two entities
//-----------------------------------------------------------------------------
class CLogicLineToEntity : public CLogicalEntity
{
public:
	DECLARE_CLASS( CLogicLineToEntity, CLogicalEntity );

	void Activate(void);
	void Spawn( void );
	void Think( void );

	// outputs
	COutputVector m_Line;

	DECLARE_DATADESC();

private:
	string_t m_SourceName;
	EHANDLE	m_StartEntity;
	EHANDLE m_EndEntity;
};

LINK_ENTITY_TO_CLASS( logic_lineto, CLogicLineToEntity );


BEGIN_DATADESC( CLogicLineToEntity )

	// Keys
	// target is handled in the base class, stored in field m_target
	DEFINE_KEYFIELD( m_SourceName, FIELD_STRING, "source" ),
 	DEFINE_FIELD( m_StartEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_EndEntity, FIELD_EHANDLE ),

	// Outputs
	DEFINE_OUTPUT( m_Line, "Line" ),

END_DATADESC()



//-----------------------------------------------------------------------------
// Find the entities
//-----------------------------------------------------------------------------
void CLogicLineToEntity::Activate(void)
{
	BaseClass::Activate();

	if (m_target != NULL_STRING)
	{
		m_EndEntity = gEntList.FindEntityByName( NULL, m_target );

		//
		// If we were given a bad measure target, just measure sound where we are.
		//
		if ((m_EndEntity == NULL) || (m_EndEntity->edict() == NULL))
		{
			Warning( "logic_lineto - Target not found or target with no origin!\n");
			m_EndEntity = this;
		}
	}
	else
	{
		m_EndEntity = this;
	}

	if (m_SourceName != NULL_STRING)
	{
		m_StartEntity = gEntList.FindEntityByName( NULL, m_SourceName );

		//
		// If we were given a bad measure target, just measure sound where we are.
		//
		if ((m_StartEntity == NULL) || (m_StartEntity->edict() == NULL))
		{
			Warning( "logic_lineto - Source not found or source with no origin!\n");
			m_StartEntity = this;
		}
	}
	else
	{
		m_StartEntity = this;
	}
}


//-----------------------------------------------------------------------------
// Find the entities
//-----------------------------------------------------------------------------
void CLogicLineToEntity::Spawn(void)
{
	SetNextThink( gpGlobals->curtime + 0.01f );
}


//-----------------------------------------------------------------------------
// Find the entities
//-----------------------------------------------------------------------------
void CLogicLineToEntity::Think(void)
{
	CBaseEntity* pDest = m_EndEntity.Get();
	CBaseEntity* pSrc = m_StartEntity.Get();
	if (!pDest || !pSrc || !pDest->edict() || !pSrc->edict())
	{
		// Can sleep for a long time, no more lines.
		m_Line.Set( vec3_origin, this, this );
		SetNextThink( gpGlobals->curtime + 10 );
		return;
	}

	Vector delta;
	VectorSubtract( pDest->GetAbsOrigin(), pSrc->GetAbsOrigin(), delta ); 
	m_Line.Set(delta, this, this);

	SetNextThink( gpGlobals->curtime + 0.01f );
}


//-----------------------------------------------------------------------------
// Purpose: Remaps a given input range to an output range.
//-----------------------------------------------------------------------------
const int SF_MATH_REMAP_IGNORE_OUT_OF_RANGE = 1;
const int SF_MATH_REMAP_CLAMP_OUTPUT_TO_RANGE = 2;

class CMathRemap : public CLogicalEntity
{
public:

	DECLARE_CLASS( CMathRemap, CLogicalEntity );

	void Spawn(void);

	// Keys
	float m_flInMin;
	float m_flInMax;
	float m_flOut1;		// Output value when input is m_fInMin
	float m_flOut2;		// Output value when input is m_fInMax

	bool  m_bEnabled;

	// Inputs
	void InputValue( inputdata_t &inputdata );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	// Outputs
	COutputFloat m_OutValue;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(math_remap, CMathRemap);


BEGIN_DATADESC( CMathRemap )

	DEFINE_INPUTFUNC(FIELD_FLOAT, "InValue", InputValue ),

	DEFINE_OUTPUT(m_OutValue, "OutValue"),

	DEFINE_KEYFIELD(m_flInMin, FIELD_FLOAT, "in1"),
	DEFINE_KEYFIELD(m_flInMax, FIELD_FLOAT, "in2"),
	DEFINE_KEYFIELD(m_flOut1, FIELD_FLOAT, "out1"),
	DEFINE_KEYFIELD(m_flOut2, FIELD_FLOAT, "out2"),

	DEFINE_FIELD( m_bEnabled, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

END_DATADESC()




//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMathRemap::Spawn(void)
{
	//
	// Avoid a divide by zero in ValueChanged.
	//
	if (m_flInMin == m_flInMax)
	{
		m_flInMin = 0;
		m_flInMax = 1;
	}

	//
	// Make sure min and max are set properly relative to one another.
	//
	if (m_flInMin > m_flInMax)
	{
		float flTemp = m_flInMin;
		m_flInMin = m_flInMax;
		m_flInMax = flTemp;
	}

	m_bEnabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMathRemap::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMathRemap::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: Input handler that is called when the input value changes.
//-----------------------------------------------------------------------------
void CMathRemap::InputValue( inputdata_t &inputdata )
{
	float flValue = inputdata.value.Float();

	//
	// Disallow out-of-range input values to avoid out-of-range output values.
	//
	float flClampValue = clamp(flValue, m_flInMin, m_flInMax);

	if ((flClampValue == flValue) || !FBitSet(m_spawnflags, SF_MATH_REMAP_IGNORE_OUT_OF_RANGE))
	{
		//
		// Remap the input value to the desired output range and update the output.
		//
		float flRemappedValue = m_flOut1 + (((flValue - m_flInMin) * (m_flOut2 - m_flOut1)) / (m_flInMax - m_flInMin));

		if ( FBitSet( m_spawnflags, SF_MATH_REMAP_CLAMP_OUTPUT_TO_RANGE ) )
		{
			flRemappedValue = clamp( flRemappedValue, m_flOut1, m_flOut2 );
		}

		if ( m_bEnabled == true )
		{
			m_OutValue.Set(flRemappedValue, inputdata.pActivator, this);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Remaps a given input range to an output range.
//-----------------------------------------------------------------------------
const int SF_COLOR_BLEND_IGNORE_OUT_OF_RANGE = 1;

class CMathColorBlend : public CLogicalEntity
{
public:

	DECLARE_CLASS( CMathColorBlend, CLogicalEntity );

	void Spawn(void);

	// Keys
	float m_flInMin;
	float m_flInMax;
	color32 m_OutColor1;		// Output color when input is m_fInMin
	color32 m_OutColor2;		// Output color when input is m_fInMax

	// Inputs
	void InputValue( inputdata_t &inputdata );

	// Outputs
	COutputColor32 m_OutValue;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(math_colorblend, CMathColorBlend);


BEGIN_DATADESC( CMathColorBlend )

	DEFINE_INPUTFUNC(FIELD_FLOAT, "InValue", InputValue ),

	DEFINE_OUTPUT(m_OutValue, "OutColor"),

	DEFINE_KEYFIELD(m_flInMin, FIELD_FLOAT, "inmin"),
	DEFINE_KEYFIELD(m_flInMax, FIELD_FLOAT, "inmax"),
	DEFINE_KEYFIELD(m_OutColor1, FIELD_COLOR32, "colormin"),
	DEFINE_KEYFIELD(m_OutColor2, FIELD_COLOR32, "colormax"),

END_DATADESC()




//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMathColorBlend::Spawn(void)
{
	//
	// Avoid a divide by zero in ValueChanged.
	//
	if (m_flInMin == m_flInMax)
	{
		m_flInMin = 0;
		m_flInMax = 1;
	}

	//
	// Make sure min and max are set properly relative to one another.
	//
	if (m_flInMin > m_flInMax)
	{
		float flTemp = m_flInMin;
		m_flInMin = m_flInMax;
		m_flInMax = flTemp;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that is called when the input value changes.
//-----------------------------------------------------------------------------
void CMathColorBlend::InputValue( inputdata_t &inputdata )
{
	float flValue = inputdata.value.Float();

	//
	// Disallow out-of-range input values to avoid out-of-range output values.
	//
	float flClampValue = clamp(flValue, m_flInMin, m_flInMax);
	if ((flClampValue == flValue) || !FBitSet(m_spawnflags, SF_COLOR_BLEND_IGNORE_OUT_OF_RANGE))
	{
		//
		// Remap the input value to the desired output color and update the output.
		//
		color32 Color;
		Color.r = m_OutColor1.r + (((flClampValue - m_flInMin) * (m_OutColor2.r - m_OutColor1.r)) / (m_flInMax - m_flInMin));
		Color.g = m_OutColor1.g + (((flClampValue - m_flInMin) * (m_OutColor2.g - m_OutColor1.g)) / (m_flInMax - m_flInMin));
		Color.b = m_OutColor1.b + (((flClampValue - m_flInMin) * (m_OutColor2.b - m_OutColor1.b)) / (m_flInMax - m_flInMin));
		Color.a = m_OutColor1.a + (((flClampValue - m_flInMin) * (m_OutColor2.a - m_OutColor1.a)) / (m_flInMax - m_flInMin));

		m_OutValue.Set(Color, inputdata.pActivator, this);
	}
}


//-----------------------------------------------------------------------------
// Console command to set the state of a global
//-----------------------------------------------------------------------------
void CC_Global_Set( const CCommand &args )
{
	const char *szGlobal = args[1];
	const char *szState = args[2];

	if ( szGlobal == NULL || szState == NULL )
	{
		Msg( "Usage: global_set <globalname> <state>: Sets the state of the given env_global (0 = OFF, 1 = ON, 2 = DEAD).\n" );
		return;
	}

	int nState = atoi( szState );

	int nIndex = GlobalEntity_GetIndex( szGlobal );

	if ( nIndex >= 0 )
	{
		GlobalEntity_SetState( nIndex, ( GLOBALESTATE )nState );
	}
	else
	{
		GlobalEntity_Add( szGlobal, STRING( gpGlobals->mapname ), ( GLOBALESTATE )nState );
	}
}

static ConCommand global_set( "global_set", CC_Global_Set, "global_set <globalname> <state>: Sets the state of the given env_global (0 = OFF, 1 = ON, 2 = DEAD).", FCVAR_CHEAT );

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Console command to set the counter of a global
//-----------------------------------------------------------------------------
void CC_Global_Counter( const CCommand &args )
{
	const char *szGlobal = args[1];
	const char *szCounter = args[2];

	if ( szGlobal == NULL || szCounter == NULL )
	{
		Msg( "Usage: global_counter <globalname> <counter>: Sets the counter of the given env_global.\n" );
		return;
	}

	int nCounter = atoi( szCounter );

	int nIndex = GlobalEntity_GetIndex( szGlobal );

	if ( nIndex >= 0 )
	{
		GlobalEntity_SetCounter( nIndex, nCounter );
	}
	else
	{
		nIndex = GlobalEntity_Add( szGlobal, STRING( gpGlobals->mapname ), GLOBAL_ON );
		GlobalEntity_SetCounter( nIndex, nCounter );
	}
}

static ConCommand global_counter( "global_counter", CC_Global_Counter, "global_counter <globalname> <counter>: Sets the counter of the given env_global.", FCVAR_CHEAT );
#endif


//-----------------------------------------------------------------------------
// Purpose: Holds a global state that can be queried by other entities to change
//			their behavior, such as "predistaster".
//-----------------------------------------------------------------------------
const int SF_GLOBAL_SET = 1;	// Set global state to initial state on spawn

class CEnvGlobal : public CLogicalEntity
{
public:
	DECLARE_CLASS( CEnvGlobal, CLogicalEntity );

	void Spawn( void );

#ifdef MAPBASE
	bool KeyValue( const char *szKeyName, const char *szValue );
#endif

	// Input handlers
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	void InputRemove( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );
	void InputSetCounter( inputdata_t &inputdata );
	void InputAddToCounter( inputdata_t &inputdata );
	void InputGetCounter( inputdata_t &inputdata );

	int DrawDebugTextOverlays(void);

	DECLARE_DATADESC();

	COutputInt m_outCounter;
		
	string_t	m_globalstate;
	int			m_triggermode;
	int			m_initialstate;
	int			m_counter;			// A counter value associated with this global.
};


BEGIN_DATADESC( CEnvGlobal )

	DEFINE_KEYFIELD( m_globalstate, FIELD_STRING, "globalstate" ),
	DEFINE_FIELD( m_triggermode, FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_initialstate, FIELD_INTEGER, "initialstate" ),
	DEFINE_KEYFIELD( m_counter, FIELD_INTEGER, "counter" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn",	InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Remove",	InputRemove ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle",	InputToggle ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetCounter",	InputSetCounter ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddToCounter",	InputAddToCounter ),
	DEFINE_INPUTFUNC( FIELD_VOID, "GetCounter",	InputGetCounter ),
	
#ifdef MAPBASE
	DEFINE_OUTPUT( m_outCounter, "OutCounter" ),
#else
	DEFINE_OUTPUT( m_outCounter, "Counter" ),
#endif

END_DATADESC()


LINK_ENTITY_TO_CLASS( env_global, CEnvGlobal );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvGlobal::Spawn( void )
{
	if ( !m_globalstate )
	{
		UTIL_Remove( this );
		return;
	}

#ifdef HL2_EPISODIC
	// if we modify the state of the physics cannon, make sure we precache the ragdoll boogie zap sound
	if ( ( m_globalstate != NULL_STRING ) && ( stricmp( STRING( m_globalstate ), "super_phys_gun" ) == 0 ) )
	{
		PrecacheScriptSound( "RagdollBoogie.Zap" );
	}
#endif

	if ( FBitSet( m_spawnflags, SF_GLOBAL_SET ) )
	{
		if ( !GlobalEntity_IsInTable( m_globalstate ) )
		{
			GlobalEntity_Add( m_globalstate, gpGlobals->mapname, (GLOBALESTATE)m_initialstate );
		}
		
		if ( m_counter != 0 )
		{
			GlobalEntity_SetCounter( m_globalstate, m_counter );
		}
	}
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: Cache user entity field values until spawn is called.
// Input  : szKeyName - Key to handle.
//			szValue - Value for key.
// Output : Returns true if the key was handled, false if not.
//-----------------------------------------------------------------------------
bool CEnvGlobal::KeyValue( const char *szKeyName, const char *szValue )
{
	// Any "Counter" outputs are changed to "OutCounter" before spawning.
	if (FStrEq(szKeyName, "Counter") && strchr(szValue, ','))
	{
		return BaseClass::KeyValue( "OutCounter", szValue );
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}
#endif

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CEnvGlobal::InputTurnOn( inputdata_t &inputdata )
{
	if ( GlobalEntity_IsInTable( m_globalstate ) )
	{
		GlobalEntity_SetState( m_globalstate, GLOBAL_ON );
	}
	else
	{
		GlobalEntity_Add( m_globalstate, gpGlobals->mapname, GLOBAL_ON );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CEnvGlobal::InputTurnOff( inputdata_t &inputdata )
{
	if ( GlobalEntity_IsInTable( m_globalstate ) )
	{
		GlobalEntity_SetState( m_globalstate, GLOBAL_OFF );
	}
	else
	{
		GlobalEntity_Add( m_globalstate, gpGlobals->mapname, GLOBAL_OFF );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CEnvGlobal::InputRemove( inputdata_t &inputdata )
{
	if ( GlobalEntity_IsInTable( m_globalstate ) )
	{
		GlobalEntity_SetState( m_globalstate, GLOBAL_DEAD );
	}
	else
	{
		GlobalEntity_Add( m_globalstate, gpGlobals->mapname, GLOBAL_DEAD );
	}
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CEnvGlobal::InputSetCounter( inputdata_t &inputdata )
{
	if ( !GlobalEntity_IsInTable( m_globalstate ) )
	{
		GlobalEntity_Add( m_globalstate, gpGlobals->mapname, GLOBAL_ON );
	}

	GlobalEntity_SetCounter( m_globalstate, inputdata.value.Int() );
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CEnvGlobal::InputAddToCounter( inputdata_t &inputdata )
{
	if ( !GlobalEntity_IsInTable( m_globalstate ) )
	{
		GlobalEntity_Add( m_globalstate, gpGlobals->mapname, GLOBAL_ON );
	}

	GlobalEntity_AddToCounter( m_globalstate, inputdata.value.Int() );
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CEnvGlobal::InputGetCounter( inputdata_t &inputdata )
{
	if ( !GlobalEntity_IsInTable( m_globalstate ) )
	{
		GlobalEntity_Add( m_globalstate, gpGlobals->mapname, GLOBAL_ON );
	}

	m_outCounter.Set( GlobalEntity_GetCounter( m_globalstate ), inputdata.pActivator, this );
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CEnvGlobal::InputToggle( inputdata_t &inputdata )
{
	GLOBALESTATE oldState = GlobalEntity_GetState( m_globalstate );
	GLOBALESTATE newState;

	if ( oldState == GLOBAL_ON )
	{
		newState = GLOBAL_OFF;
	}
	else if ( oldState == GLOBAL_OFF )
	{
		newState = GLOBAL_ON;
	}
	else
	{
		return;
	}

	if ( GlobalEntity_IsInTable( m_globalstate ) )
	{
		GlobalEntity_SetState( m_globalstate, newState );
	}
	else
	{
		GlobalEntity_Add( m_globalstate, gpGlobals->mapname, newState );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CEnvGlobal::DrawDebugTextOverlays(void) 
{
	// Skip AIClass debug overlays
	int text_offset = CBaseEntity::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf(tempstr,sizeof(tempstr),"State: %s",STRING(m_globalstate));
		EntityText(text_offset,tempstr,0);
		text_offset++;

		GLOBALESTATE nState = GlobalEntity_GetState( m_globalstate );

		switch( nState )
		{
		case GLOBAL_OFF:
			Q_strncpy(tempstr,"Value: OFF",sizeof(tempstr));
			break;

		case GLOBAL_ON:
			Q_strncpy(tempstr,"Value: ON",sizeof(tempstr));
			break;

		case GLOBAL_DEAD:
			Q_strncpy(tempstr,"Value: DEAD",sizeof(tempstr));
			break;
		}
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define MS_MAX_TARGETS 32

const int SF_MULTI_INIT	= 1;

class CMultiSource : public CLogicalEntity
{
public:
	DECLARE_CLASS( CMultiSource, CLogicalEntity );

	void Spawn( );
	bool KeyValue( const char *szKeyName, const char *szValue );
	void Use( ::CBaseEntity *pActivator, ::CBaseEntity *pCaller, USE_TYPE useType, float value );
	int	ObjectCaps( void ) { return(BaseClass::ObjectCaps() | FCAP_MASTER); }
	bool IsTriggered( ::CBaseEntity *pActivator );
	void Register( void );

	DECLARE_DATADESC();

	EHANDLE		m_rgEntities[MS_MAX_TARGETS];
	int			m_rgTriggered[MS_MAX_TARGETS];

	COutputEvent m_OnTrigger;		// Fired when all connections are triggered.

	int			m_iTotal;
	string_t	m_globalstate;
};

BEGIN_DATADESC( CMultiSource )

	//!!!BUGBUG FIX
	DEFINE_ARRAY( m_rgEntities, FIELD_EHANDLE, MS_MAX_TARGETS ),
	DEFINE_ARRAY( m_rgTriggered, FIELD_INTEGER, MS_MAX_TARGETS ),
	DEFINE_FIELD( m_iTotal, FIELD_INTEGER ),

	DEFINE_KEYFIELD( m_globalstate, FIELD_STRING, "globalstate" ),

	// Function pointers
	DEFINE_FUNCTION( Register ),

	// Outputs
	DEFINE_OUTPUT(m_OnTrigger, "OnTrigger"),

END_DATADESC()


LINK_ENTITY_TO_CLASS( multisource, CMultiSource );


//-----------------------------------------------------------------------------
// Purpose: Cache user entity field values until spawn is called.
// Input  : szKeyName - Key to handle.
//			szValue - Value for key.
// Output : Returns true if the key was handled, false if not.
//-----------------------------------------------------------------------------
bool CMultiSource::KeyValue( const char *szKeyName, const char *szValue )
{
	if (	FStrEq(szKeyName, "style") ||
				FStrEq(szKeyName, "height") ||
				FStrEq(szKeyName, "killtarget") ||
				FStrEq(szKeyName, "value1") ||
				FStrEq(szKeyName, "value2") ||
				FStrEq(szKeyName, "value3"))
	{
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiSource::Spawn()
{ 
	SetNextThink( gpGlobals->curtime + 0.1f );
	m_spawnflags |= SF_MULTI_INIT;	// Until it's initialized
	SetThink(&CMultiSource::Register);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pActivator - 
//			pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CMultiSource::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	int i = 0;

	// Find the entity in our list
	while (i < m_iTotal)
		if ( m_rgEntities[i++] == pCaller )
			break;

	// if we didn't find it, report error and leave
	if (i > m_iTotal)
	{
		Warning("MultiSrc: Used by non member %s.\n", pCaller->edict() ? pCaller->GetClassname() : "<logical entity>");
		return;	
	}

	// CONSIDER: a Use input to the multisource always toggles.  Could check useType for ON/OFF/TOGGLE

	m_rgTriggered[i-1] ^= 1;

	// 
	if ( IsTriggered( pActivator ) )
	{
		DevMsg( 2, "Multisource %s enabled (%d inputs)\n", GetDebugName(), m_iTotal );
		USE_TYPE useType = USE_TOGGLE;
		if ( m_globalstate != NULL_STRING )
			useType = USE_ON;

		m_OnTrigger.FireOutput(pActivator, this);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMultiSource::IsTriggered( CBaseEntity * )
{
	// Is everything triggered?
	int i = 0;

	// Still initializing?
	if ( m_spawnflags & SF_MULTI_INIT )
		return 0;

	while (i < m_iTotal)
	{
		if (m_rgTriggered[i] == 0)
			break;
		i++;
	}

	if (i == m_iTotal)
	{
		if ( !m_globalstate || GlobalEntity_GetState( m_globalstate ) == GLOBAL_ON )
			return 1;
	}
	
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiSource::Register(void)
{ 
	CBaseEntity *pTarget = NULL;

	m_iTotal = 0;
	memset( m_rgEntities, 0, MS_MAX_TARGETS * sizeof(EHANDLE) );

	SetThink(&CMultiSource::SUB_DoNothing);

	// search for all entities which target this multisource (m_iName)
	// dvsents2: port multisource to entity I/O!

	pTarget = gEntList.FindEntityByTarget( NULL, STRING(GetEntityName()) );

	while ( pTarget && (m_iTotal < MS_MAX_TARGETS) )
	{
		if ( pTarget )
			m_rgEntities[m_iTotal++] = pTarget;

		pTarget = gEntList.FindEntityByTarget( pTarget, STRING(GetEntityName()) );
	}

	pTarget = gEntList.FindEntityByClassname( NULL, "multi_manager" );
	while (pTarget && (m_iTotal < MS_MAX_TARGETS))
	{
		if ( pTarget && pTarget->HasTarget(GetEntityName()) )
			m_rgEntities[m_iTotal++] = pTarget;

		pTarget = gEntList.FindEntityByClassname( pTarget, "multi_manager" );
	}

	m_spawnflags &= ~SF_MULTI_INIT;
}


//-----------------------------------------------------------------------------
// Purpose: Holds a value that can be added to and subtracted from.
//-----------------------------------------------------------------------------
class CMathCounter : public CLogicalEntity
{
	DECLARE_CLASS( CMathCounter, CLogicalEntity );
#ifdef MAPBASE
protected:
#else
private:
#endif
	float m_flMin;		// Minimum clamp value. If min and max are BOTH zero, no clamping is done.
	float m_flMax;		// Maximum clamp value.
	bool m_bHitMin;		// Set when we reach or go below our minimum value, cleared if we go above it again.
	bool m_bHitMax;		// Set when we reach or exceed our maximum value, cleared if we fall below it again.

	bool m_bDisabled;

	bool KeyValue(const char *szKeyName, const char *szValue);
	void Spawn(void);

	int DrawDebugTextOverlays(void);

#ifdef MAPBASE
	virtual
#endif
	void UpdateOutValue(CBaseEntity *pActivator, float fNewValue);

	// Inputs
	void InputAdd( inputdata_t &inputdata );
	void InputDivide( inputdata_t &inputdata );
	void InputMultiply( inputdata_t &inputdata );
	void InputSetValue( inputdata_t &inputdata );
	void InputSetValueNoFire( inputdata_t &inputdata );
	void InputSubtract( inputdata_t &inputdata );
	void InputSetHitMax( inputdata_t &inputdata );
	void InputSetHitMin( inputdata_t &inputdata );
	void InputGetValue( inputdata_t &inputdata );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
#ifdef MAPBASE
	void InputSetMinValueNoFire( inputdata_t &inputdata );
	void InputSetMaxValueNoFire( inputdata_t &inputdata );
#endif

	// Outputs
	COutputFloat m_OutValue;
	COutputFloat m_OnGetValue;	// Used for polling the counter value.
	COutputEvent m_OnHitMin;
	COutputEvent m_OnHitMax;
#ifdef MAPBASE
	COutputEvent m_OnChangedFromMin;
	COutputEvent m_OnChangedFromMax;
#endif

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(math_counter, CMathCounter);


BEGIN_DATADESC( CMathCounter )

	DEFINE_FIELD(m_bHitMax, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bHitMin, FIELD_BOOLEAN),

	// Keys
	DEFINE_KEYFIELD(m_flMin, FIELD_FLOAT, "min"),
	DEFINE_KEYFIELD(m_flMax, FIELD_FLOAT, "max"),

	DEFINE_KEYFIELD(m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_FLOAT, "Add", InputAdd),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "Divide", InputDivide),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "Multiply", InputMultiply),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetValue", InputSetValue),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetValueNoFire", InputSetValueNoFire),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "Subtract", InputSubtract),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetHitMax", InputSetHitMax),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetHitMin", InputSetHitMin),
	DEFINE_INPUTFUNC(FIELD_VOID, "GetValue", InputGetValue),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
#ifdef MAPBASE
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetMaxValueNoFire", InputSetMaxValueNoFire ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetMinValueNoFire", InputSetMinValueNoFire ),
#endif

	// Outputs
	DEFINE_OUTPUT(m_OutValue, "OutValue"),
	DEFINE_OUTPUT(m_OnHitMin, "OnHitMin"),
	DEFINE_OUTPUT(m_OnHitMax, "OnHitMax"),
	DEFINE_OUTPUT(m_OnGetValue, "OnGetValue"),
#ifdef MAPBASE
	DEFINE_OUTPUT( m_OnChangedFromMin, "OnChangedFromMin" ),
	DEFINE_OUTPUT( m_OnChangedFromMax, "OnChangedFromMax" ),
#endif

END_DATADESC()




//-----------------------------------------------------------------------------
// Purpose: Handles key values from the BSP before spawn is called.
//-----------------------------------------------------------------------------
bool CMathCounter::KeyValue(const char *szKeyName, const char *szValue)
{
	//
	// Set the initial value of the counter.
	//
	if (!stricmp(szKeyName, "startvalue"))
	{
#ifdef MAPBASE
		m_OutValue.Init(atof(szValue));
#else
		m_OutValue.Init(atoi(szValue));
#endif
		return(true);
	}

	return(BaseClass::KeyValue(szKeyName, szValue));
}


//-----------------------------------------------------------------------------
// Purpose: Called before spawning, after key values have been set.
//-----------------------------------------------------------------------------
void CMathCounter::Spawn( void )
{
	//
	// Make sure max and min are ordered properly or clamp won't work.
	//
	if (m_flMin > m_flMax)
	{
		float flTemp = m_flMax;
		m_flMax = m_flMin;
		m_flMin = flTemp;
	}

	//
	// Clamp initial value to within the valid range.
	//
	if ((m_flMin != 0) || (m_flMax != 0))
	{
		float flStartValue = clamp(m_OutValue.Get(), m_flMin, m_flMax);
		m_OutValue.Init(flStartValue);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CMathCounter::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		Q_snprintf(tempstr,sizeof(tempstr),"    min value: %f", m_flMin);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"    max value: %f", m_flMax);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"current value: %f", m_OutValue.Get());
		EntityText(text_offset,tempstr,0);
		text_offset++;

		if( m_bDisabled )
		{	
			Q_snprintf(tempstr,sizeof(tempstr),"*DISABLED*");		
		}
		else
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Enabled.");
		}
		EntityText(text_offset,tempstr,0);
		text_offset++;

	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Change min/max
//-----------------------------------------------------------------------------
void CMathCounter::InputSetHitMax( inputdata_t &inputdata )
{
	m_flMax = inputdata.value.Float();
	if ( m_flMax < m_flMin )
	{
		m_flMin = m_flMax;
	}
	UpdateOutValue( inputdata.pActivator, m_OutValue.Get() );
}

void CMathCounter::InputSetHitMin( inputdata_t &inputdata )
{
	m_flMin = inputdata.value.Float();
	if ( m_flMax < m_flMin )
	{
		m_flMax = m_flMin;
	}
	UpdateOutValue( inputdata.pActivator, m_OutValue.Get() );
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Change min/max
//-----------------------------------------------------------------------------
void CMathCounter::InputSetMaxValueNoFire( inputdata_t &inputdata )
{
	m_flMax = inputdata.value.Float();
	if ( m_flMax < m_flMin )
	{
		m_flMin = m_flMax;
	}
}

void CMathCounter::InputSetMinValueNoFire( inputdata_t &inputdata )
{
	m_flMin = inputdata.value.Float();
	if ( m_flMax < m_flMin )
	{
		m_flMax = m_flMin;
	}
}
#endif

	
//-----------------------------------------------------------------------------
// Purpose: Input handler for adding to the accumulator value.
// Input  : Float value to add.
//-----------------------------------------------------------------------------
void CMathCounter::InputAdd( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring ADD because it is disabled\n", GetDebugName() );
		return;
	}

	float fNewValue = m_OutValue.Get() + inputdata.value.Float();
	UpdateOutValue( inputdata.pActivator, fNewValue );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for multiplying the current value.
// Input  : Float value to multiply the value by.
//-----------------------------------------------------------------------------
void CMathCounter::InputDivide( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring DIVIDE because it is disabled\n", GetDebugName() );
		return;
	}

	if (inputdata.value.Float() != 0)
	{
		float fNewValue = m_OutValue.Get() / inputdata.value.Float();
		UpdateOutValue( inputdata.pActivator, fNewValue );
	}
	else
	{
		DevMsg( 1, "LEVEL DESIGN ERROR: Divide by zero in math_value\n" );
		UpdateOutValue( inputdata.pActivator, m_OutValue.Get() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for multiplying the current value.
// Input  : Float value to multiply the value by.
//-----------------------------------------------------------------------------
void CMathCounter::InputMultiply( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring MULTIPLY because it is disabled\n", GetDebugName() );
		return;
	}

	float fNewValue = m_OutValue.Get() * inputdata.value.Float();
	UpdateOutValue( inputdata.pActivator, fNewValue );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for updating the value.
// Input  : Float value to set.
//-----------------------------------------------------------------------------
void CMathCounter::InputSetValue( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring SETVALUE because it is disabled\n", GetDebugName() );
		return;
	}

	UpdateOutValue( inputdata.pActivator, inputdata.value.Float() );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for updating the value.
// Input  : Float value to set.
//-----------------------------------------------------------------------------
void CMathCounter::InputSetValueNoFire( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring SETVALUENOFIRE because it is disabled\n", GetDebugName() );
		return;
	}

	float flNewValue = inputdata.value.Float();
	if (( m_flMin != 0 ) || (m_flMax != 0 ))
	{
		flNewValue = clamp(flNewValue, m_flMin, m_flMax);
	}

	m_OutValue.Init( flNewValue );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for subtracting from the current value.
// Input  : Float value to subtract.
//-----------------------------------------------------------------------------
void CMathCounter::InputSubtract( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring SUBTRACT because it is disabled\n", GetDebugName() );
		return;
	}

	float fNewValue = m_OutValue.Get() - inputdata.value.Float();
	UpdateOutValue( inputdata.pActivator, fNewValue );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathCounter::InputGetValue( inputdata_t &inputdata )
{
	float flOutValue = m_OutValue.Get();
	m_OnGetValue.Set( flOutValue, inputdata.pActivator, inputdata.pCaller );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathCounter::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathCounter::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the value to the new value, clamping and firing the output value.
// Input  : fNewValue - Value to set.
//-----------------------------------------------------------------------------
void CMathCounter::UpdateOutValue(CBaseEntity *pActivator, float fNewValue)
{
	if ((m_flMin != 0) || (m_flMax != 0))
	{
		//
		// Fire an output any time we reach or exceed our maximum value.
		//
		if ( fNewValue >= m_flMax )
		{
			if ( !m_bHitMax )
			{
				m_bHitMax = true;
				m_OnHitMax.FireOutput( pActivator, this );
			}
		}
		else
		{
#ifdef MAPBASE
			// Fire an output if we just changed from the maximum value
			if ( m_OutValue.Get() == m_flMax )
			{
				m_OnChangedFromMax.FireOutput( pActivator, this );
			}
#endif

			m_bHitMax = false;
		}

		//
		// Fire an output any time we reach or go below our minimum value.
		//
		if ( fNewValue <= m_flMin )
		{
			if ( !m_bHitMin )
			{
				m_bHitMin = true;
				m_OnHitMin.FireOutput( pActivator, this );
			}
		}
		else
		{
#ifdef MAPBASE
			// Fire an output if we just changed from the maximum value
			if ( m_OutValue.Get() == m_flMin )
			{
				m_OnChangedFromMin.FireOutput( pActivator, this );
			}
#endif
			m_bHitMin = false;
		}

		fNewValue = clamp(fNewValue, m_flMin, m_flMax);
	}

	m_OutValue.Set(fNewValue, pActivator, this);
}


#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: Advanced math_counter with advanced calculation capabilities.
//-----------------------------------------------------------------------------
class CMathCounterAdvanced : public CMathCounter
{
	DECLARE_CLASS( CMathCounterAdvanced, CMathCounter );
private:

	bool m_bPreserveValue;
	bool m_bAlwaysOutputAsInt;
	float m_flLerpPercent;

	void UpdateOutValue(CBaseEntity *pActivator, float fNewValue);

	void InputSetValueToPi( inputdata_t &inputdata );

	void InputPower( inputdata_t &inputdata );
	void InputSquareRoot( inputdata_t &inputdata );

	void InputRound( inputdata_t &inputdata );
	void InputFloor( inputdata_t &inputdata );
	void InputCeiling( inputdata_t &inputdata );
	void InputTrunc( inputdata_t &inputdata );

	void InputSine( inputdata_t &inputdata );
	void InputCosine( inputdata_t &inputdata );
	void InputTangent( inputdata_t &inputdata );

	void InputRandomInt( inputdata_t &inputdata );
	void InputRandomFloat( inputdata_t &inputdata );

	void InputLerpTo( inputdata_t &inputdata );

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(math_counter_advanced, CMathCounterAdvanced);


BEGIN_DATADESC( CMathCounterAdvanced )

	// Keys
	DEFINE_INPUT(m_bPreserveValue, FIELD_BOOLEAN, "PreserveValue"),
	DEFINE_INPUT(m_bAlwaysOutputAsInt, FIELD_BOOLEAN, "AlwaysOutputAsInt"),
	DEFINE_INPUT(m_flLerpPercent, FIELD_FLOAT, "SetLerpPercent"),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "SetValueToPi", InputSetValueToPi),

	DEFINE_INPUTFUNC(FIELD_VOID, "SquareRoot", InputSquareRoot),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "Power", InputPower),

	DEFINE_INPUTFUNC(FIELD_INTEGER, "Round", InputRound),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "Floor", InputFloor),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "Ceil", InputCeiling),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "Trunc", InputTrunc),

	DEFINE_INPUTFUNC(FIELD_VOID, "Sin", InputSine),
	DEFINE_INPUTFUNC(FIELD_VOID, "Cos", InputCosine),
	DEFINE_INPUTFUNC(FIELD_VOID, "Tan", InputTangent),

	DEFINE_INPUTFUNC(FIELD_STRING, "RandomInt", InputRandomInt),
	DEFINE_INPUTFUNC(FIELD_STRING, "RandomFloat", InputRandomFloat),

	DEFINE_INPUTFUNC(FIELD_FLOAT, "LerpTo", InputLerpTo),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Input handler for setting the current value to pi.
//-----------------------------------------------------------------------------
void CMathCounterAdvanced::InputSetValueToPi( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring SET TO PI because it is disabled\n", GetDebugName() );
		return;
	}

	float fNewValue = M_PI;
	UpdateOutValue( inputdata.pActivator, fNewValue );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for calculating the square root of the current value.
//-----------------------------------------------------------------------------
void CMathCounterAdvanced::InputSquareRoot( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring SQUARE ROOT because it is disabled\n", GetDebugName() );
		return;
	}

	float fNewValue = sqrt(m_OutValue.Get());
	UpdateOutValue( inputdata.pActivator, fNewValue );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for exponentiation of the current value. Use 2 to square it.
// Input  : Integer value to raise the current value's power.
//-----------------------------------------------------------------------------
void CMathCounterAdvanced::InputPower( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring POWER!!! because it is disabled\n", GetDebugName() );
		return;
	}

	float fNewValue = pow(m_OutValue.Get(), inputdata.value.Int());
	UpdateOutValue( inputdata.pActivator, fNewValue );
}

// 
// For some reason, I had trouble finding the original math functions at first.
// Then I just randomly stumbled upon them, bright as day.
// Oh well. These might be faster anyway.
// 
FORCEINLINE int RoundToNumber(int input, int number)
{
	(input < 0 && number > 0) ? number *= -1 : 0;
	int result = (input + (number / 2));
	result -= (result % number);
	return result;
}

// Warning: Negative numbers should be ceiled
FORCEINLINE int FloorToNumber(int input, int number)
{
	return (input - (input % number));
}

FORCEINLINE int CeilToNumber(int input, int number)
{
	(input < 0 && number > 0) ? number *= -1 : 0;
	int result = (input - (input % number));
	return result != input ? result + number : result;
}

FORCEINLINE int TruncToNumber(int input, int number)
{
	//(input < 0 && number > 0) ? number *= -1 : 0;
	return (input - (input % number));
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for rounding an integer to the specified number. (e.g. 126 rounding to 10 = 130, 1523 rounding to 5 = 1525)
// Input  : Integer value to round the current value to.
//-----------------------------------------------------------------------------
void CMathCounterAdvanced::InputRound( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring ROUND because it is disabled\n", GetDebugName() );
		return;
	}

	int iMultiple = inputdata.value.Int();
	int iNewValue;
	if (iMultiple != 0)
	{
		// Round to the nearest input number.
		iNewValue = RoundToNumber(m_OutValue.Get(), iMultiple);
	}
	else
	{
		// 0 just rounds floats.
		iNewValue = static_cast<int>(m_OutValue.Get() + 0.5f);
	}

	UpdateOutValue( inputdata.pActivator, iNewValue );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for flooring an integer to the specified number. (e.g. 126 flooring to 10 = 120, 1528 flooring to 5 = 1525)
// Input  : Integer value to floor the current value to.
//-----------------------------------------------------------------------------
void CMathCounterAdvanced::InputFloor( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring FLOOR because it is disabled\n", GetDebugName() );
		return;
	}

	int iMultiple = inputdata.value.Int();
	int iNewValue;
	if (iMultiple != 0)
	{
		iNewValue = m_OutValue.Get();
		if (iNewValue >= 0)
		{
			// Floor to the nearest input number.
			iNewValue = FloorToNumber(m_OutValue.Get(), iMultiple);
		}
		else
		{
			// We have to do it differently for negatives.
			iNewValue = CeilToNumber(m_OutValue.Get(), iMultiple);
		}
	}
	else
	{
		// 0 just floors floats.
		iNewValue = static_cast<int>(m_OutValue.Get());
	}

	UpdateOutValue( inputdata.pActivator, iNewValue );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for ceiling an integer to the specified number. (e.g. 126 ceiling to 10 = 130, 1523 ceiling to 50 = 1550)
// Input  : Integer value to ceil the current value to.
//-----------------------------------------------------------------------------
void CMathCounterAdvanced::InputCeiling( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring CEIL because it is disabled\n", GetDebugName() );
		return;
	}

	int iMultiple = inputdata.value.Int();
	int iNewValue;
	if (iMultiple != 0)
	{
		// Ceil to the nearest input number.
		iNewValue = CeilToNumber(m_OutValue.Get(), iMultiple);
	}
	else
	{
		// 0 just ceils floats.
		iNewValue = static_cast<int>(m_OutValue.Get()) + (m_OutValue.Get() != 0 ? 1 : 0);
	}

	UpdateOutValue( inputdata.pActivator, iNewValue );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for truncating an integer to the specified number. (e.g. 126 rounding to 10 = 120, -1523 rounding to 5 = 1520)
// Input  : Integer value to truncate the current value to.
//-----------------------------------------------------------------------------
void CMathCounterAdvanced::InputTrunc( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring TRUNC because it is disabled\n", GetDebugName() );
		return;
	}

	int iMultiple = inputdata.value.Int();
	int iNewValue;
	if (iMultiple != 0)
	{
		// Floor always truncates negative numbers if we don't tell it not to
		iNewValue = FloorToNumber(m_OutValue.Get(), iMultiple);
	}
	else
	{
		// 0 just ceils floats.
		iNewValue = static_cast<int>(m_OutValue.Get());
		if (iNewValue < 0)
			iNewValue += 1;
	}

	UpdateOutValue( inputdata.pActivator, iNewValue );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for applying sine to the current value.
//-----------------------------------------------------------------------------
void CMathCounterAdvanced::InputSine( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring SINE because it is disabled\n", GetDebugName() );
		return;
	}

	float fNewValue = sin(m_OutValue.Get());
	UpdateOutValue( inputdata.pActivator, fNewValue );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for applying cosine to the current value.
//-----------------------------------------------------------------------------
void CMathCounterAdvanced::InputCosine( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring SINE because it is disabled\n", GetDebugName() );
		return;
	}

	float fNewValue = cos(m_OutValue.Get());
	UpdateOutValue( inputdata.pActivator, fNewValue );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for applying tangent to the current value.
//-----------------------------------------------------------------------------
void CMathCounterAdvanced::InputTangent( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring SINE because it is disabled\n", GetDebugName() );
		return;
	}

	float fNewValue = tan(m_OutValue.Get());
	UpdateOutValue( inputdata.pActivator, fNewValue );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for random int generation.
//-----------------------------------------------------------------------------
void CMathCounterAdvanced::InputRandomInt( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring RANDOMINT because it is disabled\n", GetDebugName() );
		return;
	}

	int i1 = 0;
	int i2 = 0;

	char szInput[128];
	Q_strncpy( szInput, inputdata.value.String(), sizeof(szInput) );
	char *sSpace = strchr( szInput, ' ' );
	if ( sSpace )
	{
		i1 = atoi(szInput);
		i2 = atoi(sSpace+1);
	}
	else
	{
		// No space, assume anything from 0 to X
		i2 = atoi(szInput);
	}

	float fNewValue = RandomInt(i1, i2);
	UpdateOutValue( inputdata.pActivator, fNewValue );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for random float generation.
//-----------------------------------------------------------------------------
void CMathCounterAdvanced::InputRandomFloat( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring RANDOMFLOAT because it is disabled\n", GetDebugName() );
		return;
	}

	float f1 = 0;
	float f2 = 0;

	char szInput[128];
	Q_strncpy( szInput, inputdata.value.String(), sizeof(szInput) );
	char *sSpace = strchr( szInput, ' ' );
	if ( sSpace )
	{
		f1 = atof(szInput);
		f2 = atof(sSpace+1);
	}
	else
	{
		// No space, assume anything from 0 to X
		f2 = atof(szInput);
	}

	float fNewValue = RandomFloat(f1, f2);
	UpdateOutValue( inputdata.pActivator, fNewValue );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for random float generation.
//-----------------------------------------------------------------------------
void CMathCounterAdvanced::InputLerpTo( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Counter %s ignoring LERPTO because it is disabled\n", GetDebugName() );
		return;
	}

	float fNewValue = m_OutValue.Get() + (inputdata.value.Float() - m_OutValue.Get()) * m_flLerpPercent;
	UpdateOutValue( inputdata.pActivator, fNewValue );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the value to the new value, clamping and firing the output value.
// Input  : fNewValue - Value to set.
//-----------------------------------------------------------------------------
void CMathCounterAdvanced::UpdateOutValue(CBaseEntity *pActivator, float fNewValue)
{
	if (m_bAlwaysOutputAsInt)
		fNewValue = roundf(fNewValue);

	if (m_bPreserveValue)
	{
		//float fOriginal = m_OutValue.Get();
		//DevMsg("Preserve Before: %f\n", fOriginal);
		//BaseClass::UpdateOutValue(pActivator, fNewValue);
		//DevMsg("Preserve After: %f\n", fOriginal);
		//m_OutValue.Init(fOriginal);

		variant_t var;
		var.SetFloat(fNewValue);
		m_OutValue.FireOutput( var, pActivator, this );
	}
	else
	{
		BaseClass::UpdateOutValue(pActivator, fNewValue);
	}
}
#endif



//-----------------------------------------------------------------------------
// Purpose: Compares a single string input to up to 16 case values, firing an
//			output corresponding to the case value that matched, or a default
//			output if the input value didn't match any of the case values.
//
//			This can also be used to fire a random output from a set of outputs.
//-----------------------------------------------------------------------------
#define MAX_LOGIC_CASES 16

class CLogicCase : public CLogicalEntity
{
	DECLARE_CLASS( CLogicCase, CLogicalEntity );
private:
	string_t m_nCase[MAX_LOGIC_CASES];

#ifdef MAPBASE
	bool m_bMultipleCasesAllowed;
#endif

	int m_nShuffleCases;
	int m_nLastShuffleCase;
	unsigned char m_uchShuffleCaseMap[MAX_LOGIC_CASES];

	void Spawn(void);

	int BuildCaseMap(unsigned char *puchMap);

	// Inputs
	void InputValue( inputdata_t &inputdata );
	void InputPickRandom( inputdata_t &inputdata );
	void InputPickRandomShuffle( inputdata_t &inputdata );

	// Outputs
	COutputEvent m_OnCase[MAX_LOGIC_CASES];		// Fired when the input value matches one of the case values.
	COutputVariant m_OnDefault;					// Fired when no match was found.
#ifdef MAPBASE
	COutputVariant m_OnUsed;					// Fired when this entity receives any input at all.
#endif

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(logic_case, CLogicCase);


BEGIN_DATADESC( CLogicCase )

// Silence, Classcheck!
//	DEFINE_ARRAY( m_nCase, FIELD_STRING, MAX_LOGIC_CASES ),

	// Keys
	DEFINE_KEYFIELD(m_nCase[0], FIELD_STRING, "Case01"),
	DEFINE_KEYFIELD(m_nCase[1], FIELD_STRING, "Case02"),
	DEFINE_KEYFIELD(m_nCase[2], FIELD_STRING, "Case03"),
	DEFINE_KEYFIELD(m_nCase[3], FIELD_STRING, "Case04"),
	DEFINE_KEYFIELD(m_nCase[4], FIELD_STRING, "Case05"),
	DEFINE_KEYFIELD(m_nCase[5], FIELD_STRING, "Case06"),
	DEFINE_KEYFIELD(m_nCase[6], FIELD_STRING, "Case07"),
	DEFINE_KEYFIELD(m_nCase[7], FIELD_STRING, "Case08"),
	DEFINE_KEYFIELD(m_nCase[8], FIELD_STRING, "Case09"),
	DEFINE_KEYFIELD(m_nCase[9], FIELD_STRING, "Case10"),
	DEFINE_KEYFIELD(m_nCase[10], FIELD_STRING, "Case11"),
	DEFINE_KEYFIELD(m_nCase[11], FIELD_STRING, "Case12"),
	DEFINE_KEYFIELD(m_nCase[12], FIELD_STRING, "Case13"),
	DEFINE_KEYFIELD(m_nCase[13], FIELD_STRING, "Case14"),
	DEFINE_KEYFIELD(m_nCase[14], FIELD_STRING, "Case15"),
	DEFINE_KEYFIELD(m_nCase[15], FIELD_STRING, "Case16"),

#ifdef MAPBASE
	DEFINE_KEYFIELD(m_bMultipleCasesAllowed, FIELD_BOOLEAN, "MultipleCasesAllowed"),
#endif
	
	DEFINE_FIELD( m_nShuffleCases, FIELD_INTEGER ),
	DEFINE_FIELD( m_nLastShuffleCase, FIELD_INTEGER ),
	DEFINE_ARRAY( m_uchShuffleCaseMap, FIELD_CHARACTER, MAX_LOGIC_CASES ),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_INPUT, "InValue", InputValue),
	DEFINE_INPUTFUNC(FIELD_VOID, "PickRandom", InputPickRandom),
	DEFINE_INPUTFUNC(FIELD_VOID, "PickRandomShuffle", InputPickRandomShuffle),

	// Outputs
	DEFINE_OUTPUT(m_OnCase[0], "OnCase01"),
	DEFINE_OUTPUT(m_OnCase[1], "OnCase02"),
	DEFINE_OUTPUT(m_OnCase[2], "OnCase03"),
	DEFINE_OUTPUT(m_OnCase[3], "OnCase04"),
	DEFINE_OUTPUT(m_OnCase[4], "OnCase05"),
	DEFINE_OUTPUT(m_OnCase[5], "OnCase06"),
	DEFINE_OUTPUT(m_OnCase[6], "OnCase07"),
	DEFINE_OUTPUT(m_OnCase[7], "OnCase08"),
	DEFINE_OUTPUT(m_OnCase[8], "OnCase09"),
	DEFINE_OUTPUT(m_OnCase[9], "OnCase10"),
	DEFINE_OUTPUT(m_OnCase[10], "OnCase11"),
	DEFINE_OUTPUT(m_OnCase[11], "OnCase12"),
	DEFINE_OUTPUT(m_OnCase[12], "OnCase13"),
	DEFINE_OUTPUT(m_OnCase[13], "OnCase14"),
	DEFINE_OUTPUT(m_OnCase[14], "OnCase15"),
	DEFINE_OUTPUT(m_OnCase[15], "OnCase16"),

	DEFINE_OUTPUT(m_OnDefault, "OnDefault"),
#ifdef MAPBASE
	DEFINE_OUTPUT(m_OnUsed, "OnUsed"),
#endif

END_DATADESC()




//-----------------------------------------------------------------------------
// Purpose: Called before spawning, after key values have been set.
//-----------------------------------------------------------------------------
void CLogicCase::Spawn( void )
{
	m_nLastShuffleCase = -1;
}


//-----------------------------------------------------------------------------
// Purpose: Evaluates the new input value, firing the appropriate OnCaseX output
//			if the input value matches one of the "CaseX" keys.
// Input  : Value - Variant value to compare against the values of the case fields.
//				We use a variant so that we can convert any input type to a string.
//-----------------------------------------------------------------------------
void CLogicCase::InputValue( inputdata_t &inputdata )
{
#ifdef MAPBASE
	m_OnUsed.Set(inputdata.value, inputdata.pActivator, this);
	bool bFoundCase = false;
#endif
	const char *pszValue = inputdata.value.String();
	for (int i = 0; i < MAX_LOGIC_CASES; i++)
	{
#ifdef MAPBASE
		if ((m_nCase[i] != NULL_STRING) && Matcher_Match(STRING(m_nCase[i]), pszValue))
		{
			m_OnCase[i].FireOutput( inputdata.pActivator, this );

			if (!m_bMultipleCasesAllowed)
				return;
			else if (!bFoundCase)
				bFoundCase = true;
		}
#else
		if ((m_nCase[i] != NULL_STRING) && !stricmp(STRING(m_nCase[i]), pszValue))
		{
			m_OnCase[i].FireOutput( inputdata.pActivator, this );
			return;
		}
#endif
	}
	
#ifdef MAPBASE
	if (!bFoundCase)
#endif
	m_OnDefault.Set( inputdata.value, inputdata.pActivator, this );
}


//-----------------------------------------------------------------------------
// Count the number of valid cases, building a packed array
// that maps 0..NumCases to the actual CaseX values.
//
// This allows our zany mappers to set up cases sparsely if they desire.
// NOTE: assumes pnMap points to an array of MAX_LOGIC_CASES
//-----------------------------------------------------------------------------
int CLogicCase::BuildCaseMap(unsigned char *puchCaseMap)
{
	memset(puchCaseMap, 0, sizeof(unsigned char) * MAX_LOGIC_CASES);

	int nNumCases = 0;
	for (int i = 0; i < MAX_LOGIC_CASES; i++)
	{
		if (m_OnCase[i].NumberOfElements() > 0)
		{
			puchCaseMap[nNumCases] = (unsigned char)i;
			nNumCases++;
		}
	}
	
	return nNumCases;
}


//-----------------------------------------------------------------------------
// Purpose: Makes the case statement choose a case at random.
//-----------------------------------------------------------------------------
void CLogicCase::InputPickRandom( inputdata_t &inputdata )
{
	unsigned char uchCaseMap[MAX_LOGIC_CASES];
	int nNumCases = BuildCaseMap( uchCaseMap );

	//
	// Choose a random case from the ones that were set up by the level designer.
	//
	if ( nNumCases > 0 )
	{
		int nRandom = random->RandomInt(0, nNumCases - 1);
		int nCase = (unsigned char)uchCaseMap[nRandom];

		Assert(nCase < MAX_LOGIC_CASES);

		if (nCase < MAX_LOGIC_CASES)
		{
			m_OnCase[nCase].FireOutput( inputdata.pActivator, this );
		}
	}
	else
	{
		DevMsg( 1, "Firing PickRandom input on logic_case %s with no cases set up\n", GetDebugName() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Makes the case statement choose a case at random.
//-----------------------------------------------------------------------------
void CLogicCase::InputPickRandomShuffle( inputdata_t &inputdata )
{
	int nAvoidCase = -1;
	int nCaseCount = m_nShuffleCases;
	
	if ( nCaseCount == 0 )
	{
		// Starting a new shuffle batch.
		nCaseCount = m_nShuffleCases = BuildCaseMap( m_uchShuffleCaseMap );
		
		if ( ( m_nShuffleCases > 1 ) && ( m_nLastShuffleCase != -1 ) )
		{
			// Remove the previously picked case from the case map for this pick only.
			// This avoids repeats across shuffle batch boundaries.		
			nAvoidCase = m_nLastShuffleCase;
			
			for (int i = 0; i < m_nShuffleCases; i++ )
			{
				if ( m_uchShuffleCaseMap[i] == nAvoidCase )
				{
					unsigned char uchSwap = m_uchShuffleCaseMap[i];
					m_uchShuffleCaseMap[i] = m_uchShuffleCaseMap[nCaseCount - 1];
					m_uchShuffleCaseMap[nCaseCount - 1] = uchSwap;
					nCaseCount--;
					break;
				}
			}
		}
	}
	
	//
	// Choose a random case from the ones that were set up by the level designer.
	// Never repeat a case within a shuffle batch, nor consecutively across batches.
	//
	if ( nCaseCount > 0 )
	{
		int nRandom = random->RandomInt( 0, nCaseCount - 1 );

		int nCase = m_uchShuffleCaseMap[nRandom];
		Assert(nCase < MAX_LOGIC_CASES);

		if (nCase < MAX_LOGIC_CASES)
		{
			m_OnCase[nCase].FireOutput( inputdata.pActivator, this );
		}
		
		m_uchShuffleCaseMap[nRandom] = m_uchShuffleCaseMap[m_nShuffleCases - 1];
		m_nShuffleCases--;

		m_nLastShuffleCase = nCase;
	}
	else
	{
		DevMsg( 1, "Firing PickRandom input on logic_case %s with no cases set up\n", GetDebugName() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Compares a floating point input to a predefined value, firing an
//			output to indicate the result of the comparison.
//-----------------------------------------------------------------------------
class CLogicCompare : public CLogicalEntity
{
	DECLARE_CLASS( CLogicCompare, CLogicalEntity );

public:
	int DrawDebugTextOverlays(void);

#ifdef MAPBASE
	void Spawn();
#endif

private:
	// Inputs
	void InputSetValue( inputdata_t &inputdata );
	void InputSetValueCompare( inputdata_t &inputdata );
	void InputSetCompareValue( inputdata_t &inputdata );
#ifdef MAPBASE
	void InputSetCompareValueCompare( inputdata_t &inputdata );
#endif
	void InputCompare( inputdata_t &inputdata );

#ifdef MAPBASE
	void DoCompare(CBaseEntity *pActivator, variant_t value);
#else
	void DoCompare(CBaseEntity *pActivator, float flInValue);
#endif

#ifdef MAPBASE
	bool m_bStrLenAllowed = true;
	bool m_bGreaterThanOrEqual;
	variant_t m_InValue;					// Place to hold the last input value for a recomparison.
	variant_t m_CompareValue;				// The value to compare the input value against.
#else
	float m_flInValue;					// Place to hold the last input value for a recomparison.
	float m_flCompareValue;				// The value to compare the input value against.
#endif

	// Outputs
#ifdef MAPBASE
	COutputVariant m_OnLessThan;			// Fired when the input value is less than the compare value.
	COutputVariant m_OnEqualTo;			// Fired when the input value is equal to the compare value.
	COutputVariant m_OnNotEqualTo;		// Fired when the input value is not equal to the compare value.
	COutputVariant m_OnGreaterThan;		// Fired when the input value is greater than the compare value.
	COutputVariant m_OnLessThanOrEqualTo;			// Fired when the input value is less than or equal to the compare value.
	COutputVariant m_OnGreaterThanOrEqualTo;		// Fired when the input value is greater than or equal to the compare value.
#else
	COutputFloat m_OnLessThan;			// Fired when the input value is less than the compare value.
	COutputFloat m_OnEqualTo;			// Fired when the input value is equal to the compare value.
	COutputFloat m_OnNotEqualTo;		// Fired when the input value is not equal to the compare value.
	COutputFloat m_OnGreaterThan;		// Fired when the input value is greater than the compare value.
#endif

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(logic_compare, CLogicCompare);


BEGIN_DATADESC( CLogicCompare )

	// Keys
#ifdef MAPBASE
	DEFINE_KEYFIELD( m_bStrLenAllowed, FIELD_BOOLEAN, "StrLenAllowed" ),
	DEFINE_KEYVARIANT(m_CompareValue, "CompareValue"),
	DEFINE_KEYVARIANT(m_InValue, "InitialValue"),
#else
	DEFINE_KEYFIELD(m_flCompareValue, FIELD_FLOAT, "CompareValue"),
	DEFINE_KEYFIELD(m_flInValue, FIELD_FLOAT, "InitialValue"),
#endif

	// Inputs
#ifdef MAPBASE
	DEFINE_INPUTFUNC(FIELD_INPUT, "SetValue", InputSetValue),
	DEFINE_INPUTFUNC(FIELD_INPUT, "SetValueCompare", InputSetValueCompare),
	DEFINE_INPUTFUNC(FIELD_INPUT, "SetCompareValue", InputSetCompareValue),
	DEFINE_INPUTFUNC(FIELD_INPUT, "SetCompareValueCompare", InputSetCompareValueCompare),
#else
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetValue", InputSetValue),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetValueCompare", InputSetValueCompare),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetCompareValue", InputSetCompareValue),
#endif
	DEFINE_INPUTFUNC(FIELD_VOID, "Compare", InputCompare),

	// Outputs
	DEFINE_OUTPUT(m_OnEqualTo, "OnEqualTo"),
	DEFINE_OUTPUT(m_OnNotEqualTo, "OnNotEqualTo"),
	DEFINE_OUTPUT(m_OnGreaterThan, "OnGreaterThan"),
	DEFINE_OUTPUT(m_OnLessThan, "OnLessThan"),
#ifdef MAPBASE
	DEFINE_OUTPUT(m_OnGreaterThanOrEqualTo, "OnGreaterThanOrEqualTo"),
	DEFINE_OUTPUT(m_OnLessThanOrEqualTo, "OnLessThanOrEqualTo"),
#endif

END_DATADESC()




#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: Input handler for a new input value without performing a comparison.
//-----------------------------------------------------------------------------
void CLogicCompare::Spawn()
{
	// Empty initial values are equivalent to 0
	if (m_InValue.FieldType() == FIELD_STRING && m_InValue.String()[0] == '\0')
		m_InValue.SetInt( 0 );

	BaseClass::Spawn();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Input handler for a new input value without performing a comparison.
//-----------------------------------------------------------------------------
void CLogicCompare::InputSetValue( inputdata_t &inputdata )
{
#ifdef MAPBASE
	m_InValue = Variant_ParseInput(inputdata);
#else
	m_flInValue = inputdata.value.Float();
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for a setting a new value and doing the comparison.
//-----------------------------------------------------------------------------
void CLogicCompare::InputSetValueCompare( inputdata_t &inputdata )
{
#ifdef MAPBASE
	m_InValue = Variant_ParseInput(inputdata);
	DoCompare( inputdata.pActivator, m_InValue );
#else
	m_flInValue = inputdata.value.Float();
	DoCompare( inputdata.pActivator, m_flInValue );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for a new input value without performing a comparison.
//-----------------------------------------------------------------------------
void CLogicCompare::InputSetCompareValue( inputdata_t &inputdata )
{
#ifdef MAPBASE
	m_CompareValue = Variant_ParseInput(inputdata);
#else
	m_flCompareValue = inputdata.value.Float();
#endif
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: Input handler for a new input value and doing the comparison.
//-----------------------------------------------------------------------------
void CLogicCompare::InputSetCompareValueCompare( inputdata_t &inputdata )
{
	m_CompareValue = Variant_ParseInput(inputdata);
	DoCompare( inputdata.pActivator, m_InValue );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Input handler for forcing a recompare of the last input value.
//-----------------------------------------------------------------------------
void CLogicCompare::InputCompare( inputdata_t &inputdata )
{
#ifdef MAPBASE
	DoCompare( inputdata.pActivator, m_InValue );
#else
	DoCompare( inputdata.pActivator, m_flInValue );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Compares the input value to the compare value, firing the appropriate
//			output(s) based on the comparison result.
// Input  : flInValue - Value to compare against the comparison value.
//-----------------------------------------------------------------------------
#ifdef MAPBASE
void CLogicCompare::DoCompare(CBaseEntity *pActivator, variant_t value)
{
	if (Variant_Equal(value, m_CompareValue, m_bStrLenAllowed))
	{
		m_OnEqualTo.Set(value, pActivator, this);

		m_OnLessThanOrEqualTo.Set(value, pActivator, this);
		m_OnGreaterThanOrEqualTo.Set(value, pActivator, this);
	}
	else
	{
		m_OnNotEqualTo.Set(value, pActivator, this);

		if (Variant_Greater(m_InValue, m_CompareValue, m_bStrLenAllowed))
		{
			m_OnGreaterThan.Set(value, pActivator, this);
			m_OnGreaterThanOrEqualTo.Set(value, pActivator, this);
		}
		else
		{
			m_OnLessThan.Set(value, pActivator, this);
			m_OnLessThanOrEqualTo.Set(value, pActivator, this);
		}
	}
}
#else
void CLogicCompare::DoCompare(CBaseEntity *pActivator, float flInValue)
{
	if (flInValue == m_flCompareValue)
	{
		m_OnEqualTo.Set(flInValue, pActivator, this);
	}
	else
	{
		m_OnNotEqualTo.Set(flInValue, pActivator, this);

		if (flInValue > m_flCompareValue)
		{
			m_OnGreaterThan.Set(flInValue, pActivator, this);
		}
		else
		{
			m_OnLessThan.Set(flInValue, pActivator, this);
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CLogicCompare::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		// print duration
#ifdef MAPBASE
		Q_snprintf(tempstr,sizeof(tempstr),"    Initial Value: %s", m_InValue.GetDebug());
#else
		Q_snprintf(tempstr,sizeof(tempstr),"    Initial Value: %f", m_flInValue);
#endif
		EntityText(text_offset,tempstr,0);
		text_offset++;

		// print hold time
#ifdef MAPBASE
		Q_snprintf(tempstr,sizeof(tempstr),"    Compare Value: %s", m_CompareValue.GetDebug());
#else
		Q_snprintf(tempstr,sizeof(tempstr),"    Compare Value: %f", m_flCompareValue);
#endif
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Tests a boolean value, firing an output to indicate whether the
//			value was true or false.
//-----------------------------------------------------------------------------
class CLogicBranch : public CLogicalEntity
{
	DECLARE_CLASS( CLogicBranch, CLogicalEntity );
	
public:

	void UpdateOnRemove();

	void AddLogicBranchListener( CBaseEntity *pEntity );
	inline bool GetLogicBranchState();
	virtual int DrawDebugTextOverlays( void );

private:

	enum LogicBranchFire_t
	{
		LOGIC_BRANCH_FIRE,
		LOGIC_BRANCH_NO_FIRE,
	};

	// Inputs
	void InputSetValue( inputdata_t &inputdata );
	void InputSetValueTest( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );
	void InputToggleTest( inputdata_t &inputdata );
	void InputTest( inputdata_t &inputdata );

	void UpdateValue(bool bNewValue, CBaseEntity *pActivator, LogicBranchFire_t eFire);

	bool m_bInValue;					// Place to hold the last input value for a future test.
	
	CUtlVector<EHANDLE> m_Listeners;	// A list of logic_branch_listeners that are monitoring us.

	// Outputs
	COutputEvent m_OnTrue;				// Fired when the value is true.
	COutputEvent m_OnFalse;				// Fired when the value is false.

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(logic_branch, CLogicBranch);


BEGIN_DATADESC( CLogicBranch )

	// Keys
	DEFINE_KEYFIELD(m_bInValue, FIELD_BOOLEAN, "InitialValue"),

	DEFINE_UTLVECTOR( m_Listeners, FIELD_EHANDLE ),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_BOOLEAN, "SetValue", InputSetValue),
	DEFINE_INPUTFUNC(FIELD_BOOLEAN, "SetValueTest", InputSetValueTest),
	DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),
	DEFINE_INPUTFUNC(FIELD_VOID, "ToggleTest", InputToggleTest),
	DEFINE_INPUTFUNC(FIELD_VOID, "Test", InputTest),

	// Outputs
	DEFINE_OUTPUT(m_OnTrue, "OnTrue"),
	DEFINE_OUTPUT(m_OnFalse, "OnFalse"),

END_DATADESC()


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLogicBranch::UpdateOnRemove()
{
	for ( int i = 0; i < m_Listeners.Count(); i++ )
	{
		CBaseEntity *pEntity = m_Listeners.Element( i ).Get();
		if ( pEntity )
		{
			g_EventQueue.AddEvent( pEntity, "_OnLogicBranchRemoved", 0, this, this );
		}
	}
	
	BaseClass::UpdateOnRemove();
}
	

//-----------------------------------------------------------------------------
// Purpose: Input handler to set a new input value without firing outputs.
// Input  : Boolean value to set.
//-----------------------------------------------------------------------------
void CLogicBranch::InputSetValue( inputdata_t &inputdata )
{
	UpdateValue( inputdata.value.Bool(), inputdata.pActivator, LOGIC_BRANCH_NO_FIRE );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler to set a new input value and fire appropriate outputs.
// Input  : Boolean value to set.
//-----------------------------------------------------------------------------
void CLogicBranch::InputSetValueTest( inputdata_t &inputdata )
{
	UpdateValue( inputdata.value.Bool(), inputdata.pActivator, LOGIC_BRANCH_FIRE );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for toggling the boolean value without firing outputs.
//-----------------------------------------------------------------------------
void CLogicBranch::InputToggle( inputdata_t &inputdata )
{
	UpdateValue( !m_bInValue, inputdata.pActivator, LOGIC_BRANCH_NO_FIRE );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for toggling the boolean value and then firing the
//			appropriate output based on the new value.
//-----------------------------------------------------------------------------
void CLogicBranch::InputToggleTest( inputdata_t &inputdata )
{
	UpdateValue( !m_bInValue, inputdata.pActivator, LOGIC_BRANCH_FIRE );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for forcing a test of the last input value.
//-----------------------------------------------------------------------------
void CLogicBranch::InputTest( inputdata_t &inputdata )
{
	UpdateValue( m_bInValue, inputdata.pActivator, LOGIC_BRANCH_FIRE );
}


//-----------------------------------------------------------------------------
// Purpose: Tests the last input value, firing the appropriate output based on
//			the test result.
// Input  : bInValue - 
//-----------------------------------------------------------------------------
void CLogicBranch::UpdateValue( bool bNewValue, CBaseEntity *pActivator, LogicBranchFire_t eFire )
{
	if ( m_bInValue != bNewValue )
	{
		m_bInValue = bNewValue;

		for ( int i = 0; i < m_Listeners.Count(); i++ )
		{
			CBaseEntity *pEntity = m_Listeners.Element( i ).Get();
			if ( pEntity )
			{
				g_EventQueue.AddEvent( pEntity, "_OnLogicBranchChanged", 0, this, this );
			}
		}
	}

	if ( eFire == LOGIC_BRANCH_FIRE )
	{
		if ( m_bInValue )
		{
			m_OnTrue.FireOutput( pActivator, this );
		}
		else
		{
			m_OnFalse.FireOutput( pActivator, this );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Accessor for logic_branchlist to test the value of the branch on demand.
//-----------------------------------------------------------------------------
bool CLogicBranch::GetLogicBranchState()
{
	return m_bInValue;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLogicBranch::AddLogicBranchListener( CBaseEntity *pEntity )
{
	if ( m_Listeners.Find( pEntity ) == -1 )
	{
		m_Listeners.AddToTail( pEntity );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CLogicBranch::DrawDebugTextOverlays( void )
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		// print refire time
		Q_snprintf( tempstr, sizeof(tempstr), "Branch value: %s", (m_bInValue) ? "TRUE" : "FALSE" );
		EntityText( text_offset, tempstr, 0 );
		text_offset++;
	}

	return text_offset;
}

#ifdef MAPBASE
extern void MapbaseGameLog_Record( const char *szContext );
extern ConVar mapbase_game_log_on_autosave;
#endif

//-----------------------------------------------------------------------------
// Purpose: Autosaves when triggered
//-----------------------------------------------------------------------------
class CLogicAutosave : public CLogicalEntity
{
	DECLARE_CLASS( CLogicAutosave, CLogicalEntity );

protected:
	// Inputs
	void InputSave( inputdata_t &inputdata );
	void InputSaveDangerous( inputdata_t &inputdata );
	void InputSetMinHitpointsThreshold( inputdata_t &inputdata );

	DECLARE_DATADESC();
	bool m_bForceNewLevelUnit;
	int m_minHitPoints;
	int m_minHitPointsToCommit;
};

LINK_ENTITY_TO_CLASS(logic_autosave, CLogicAutosave);

BEGIN_DATADESC( CLogicAutosave )
	DEFINE_KEYFIELD( m_bForceNewLevelUnit, FIELD_BOOLEAN, "NewLevelUnit" ),
	DEFINE_KEYFIELD( m_minHitPoints, FIELD_INTEGER, "MinimumHitPoints" ),
	DEFINE_KEYFIELD( m_minHitPointsToCommit, FIELD_INTEGER, "MinHitPointsToCommit" ),
	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Save", InputSave ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SaveDangerous", InputSaveDangerous ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMinHitpointsThreshold", InputSetMinHitpointsThreshold ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Save!
//-----------------------------------------------------------------------------
void CLogicAutosave::InputSave( inputdata_t &inputdata )
{
#ifdef MAPBASE
	if (mapbase_game_log_on_autosave.GetBool())
	{
		MapbaseGameLog_Record( "autosave" );
	}
#endif

	if ( m_bForceNewLevelUnit )
	{
		engine->ClearSaveDir();
	}

	engine->ServerCommand( "autosave\n" );
}

//-----------------------------------------------------------------------------
// Purpose: Save safely!
//-----------------------------------------------------------------------------
void CLogicAutosave::InputSaveDangerous( inputdata_t &inputdata )
{
#ifdef MAPBASE
	if (mapbase_game_log_on_autosave.GetBool())
	{
		MapbaseGameLog_Record( "autosave_dangerous" );
	}
#endif

	CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );

	if ( g_ServerGameDLL.m_fAutoSaveDangerousTime != 0.0f && g_ServerGameDLL.m_fAutoSaveDangerousTime >= gpGlobals->curtime )
	{
		// A previous dangerous auto save was waiting to become safe

		if ( pPlayer->GetDeathTime() == 0.0f || pPlayer->GetDeathTime() > gpGlobals->curtime )
		{
			// The player isn't dead, so make the dangerous auto save safe
			engine->ServerCommand( "autosavedangerousissafe\n" );
		}
	}

	if ( m_bForceNewLevelUnit )
	{
		engine->ClearSaveDir();
	}

	if ( pPlayer->GetHealth() >= m_minHitPoints )
	{
		engine->ServerCommand( "autosavedangerous\n" );
		g_ServerGameDLL.m_fAutoSaveDangerousTime = gpGlobals->curtime + inputdata.value.Float();

		// Player must have this much health when we go to commit, or we don't commit.
		g_ServerGameDLL.m_fAutoSaveDangerousMinHealthToCommit = m_minHitPointsToCommit;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Autosaves when triggered
//-----------------------------------------------------------------------------
class CLogicActiveAutosave : public CLogicAutosave
{
	DECLARE_CLASS( CLogicActiveAutosave, CLogicAutosave );

	void InputEnable( inputdata_t &inputdata )
	{
		m_flStartTime = -1;
		SetThink( &CLogicActiveAutosave::SaveThink );
		SetNextThink( gpGlobals->curtime );
	}

	void InputDisable( inputdata_t &inputdata )
	{
		SetThink( NULL );
	}

	void SaveThink()
	{
		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
		if ( pPlayer )
		{
			if ( m_flStartTime < 0 )
			{
				if ( pPlayer->GetHealth() <= m_minHitPoints )
				{
					m_flStartTime = gpGlobals->curtime;
				}
			}
			else
			{
				if ( pPlayer->GetHealth() >= m_TriggerHitPoints )
				{
					inputdata_t inputdata;
					DevMsg( 2, "logic_active_autosave (%s, %d) triggered\n", STRING( GetEntityName() ), entindex() );
					if ( !m_flDangerousTime )
					{
						InputSave( inputdata );
					}
					else
					{
						inputdata.value.SetFloat( m_flDangerousTime );
						InputSaveDangerous( inputdata );
					}
					m_flStartTime = -1;
				}
				else if ( m_flTimeToTrigger > 0 && gpGlobals->curtime - m_flStartTime > m_flTimeToTrigger )
				{
					m_flStartTime = -1;
				}
			}
		}

		float thinkInterval = ( m_flStartTime < 0 ) ? 1.0 : 0.5;
		SetNextThink( gpGlobals->curtime + thinkInterval );
	}

	DECLARE_DATADESC();

	int m_TriggerHitPoints;
	float m_flTimeToTrigger;
	float m_flStartTime;
	float m_flDangerousTime;
};

LINK_ENTITY_TO_CLASS(logic_active_autosave, CLogicActiveAutosave);

BEGIN_DATADESC( CLogicActiveAutosave )
	DEFINE_KEYFIELD( m_TriggerHitPoints, FIELD_INTEGER, "TriggerHitPoints" ),
	DEFINE_KEYFIELD( m_flTimeToTrigger, FIELD_FLOAT, "TimeToTrigger" ),
	DEFINE_KEYFIELD( m_flDangerousTime, FIELD_FLOAT, "DangerousTime" ),
	DEFINE_FIELD( m_flStartTime, FIELD_TIME ),
	DEFINE_THINKFUNC( SaveThink ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Keyfield set func
//-----------------------------------------------------------------------------
void CLogicAutosave::InputSetMinHitpointsThreshold( inputdata_t &inputdata )
{
	int setTo = inputdata.value.Int();
	AssertMsg1(setTo >= 0 && setTo <= 100, "Tried to set autosave MinHitpointsThreshold to %d!\n", setTo);
	m_minHitPoints = setTo;
}

// Finds the named physics object.  If no name, returns the world
// If a name is specified and an object not found - errors are reported
IPhysicsObject *FindPhysicsObjectByNameOrWorld( string_t name, CBaseEntity *pErrorEntity )
{
	if ( !name )
		return g_PhysWorldObject;

	IPhysicsObject *pPhysics = FindPhysicsObjectByName( name.ToCStr(), pErrorEntity );
	if ( !pPhysics )
	{
		DevWarning("%s: can't find %s\n", pErrorEntity->GetClassname(), name.ToCStr());
	}
	return pPhysics;
}

class CLogicCollisionPair : public CLogicalEntity
{
	DECLARE_CLASS( CLogicCollisionPair, CLogicalEntity );
public:

#ifdef MAPBASE
	// !activator, !caller, etc. support
	void EnableCollisions( bool bEnable, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL )
	{
		IPhysicsObject *pPhysics0 = NULL;
		IPhysicsObject *pPhysics1 = NULL;

		CBaseEntity *pEntity0 = gEntList.FindEntityByName( NULL, m_nameAttach1, this, pActivator, pCaller );
		if (pEntity0)
			pPhysics0 = pEntity0->VPhysicsGetObject();

		CBaseEntity *pEntity1 = gEntList.FindEntityByName( NULL, m_nameAttach2, this, pActivator, pCaller );
		if (pEntity1)
			pPhysics1 = pEntity1->VPhysicsGetObject();
#else
	void EnableCollisions( bool bEnable )
	{
		IPhysicsObject *pPhysics0 = FindPhysicsObjectByNameOrWorld( m_nameAttach1, this );
		IPhysicsObject *pPhysics1 = FindPhysicsObjectByNameOrWorld( m_nameAttach2, this );
#endif

		// need two different objects to do anything
		if ( pPhysics0 && pPhysics1 && pPhysics0 != pPhysics1 )
		{
			m_disabled = !bEnable;
			m_succeeded = true;
			if ( bEnable )
			{
				PhysEnableEntityCollisions( pPhysics0, pPhysics1 );
			}
			else
			{
				PhysDisableEntityCollisions( pPhysics0, pPhysics1 );
			}
		}
		else
		{
			m_succeeded = false;
		}
	}

	void Activate( void )
	{
		if ( m_disabled )
		{
			EnableCollisions( false );
		}
		BaseClass::Activate();
	}

	void InputDisableCollisions( inputdata_t &inputdata )
	{
		if ( m_succeeded && m_disabled )
			return;
#ifdef MAPBASE
		EnableCollisions( false, inputdata.pActivator, inputdata.pCaller );
#else
		EnableCollisions( false );
#endif
	}

	void InputEnableCollisions( inputdata_t &inputdata )
	{
		if ( m_succeeded && !m_disabled )
			return;
#ifdef MAPBASE
		EnableCollisions( true, inputdata.pActivator, inputdata.pCaller );
#else
		EnableCollisions( true );
#endif
	}
	// If Activate() becomes PostSpawn()
	//void OnRestore() { Activate(); }

	DECLARE_DATADESC();

private:
	string_t		m_nameAttach1;
	string_t		m_nameAttach2;
	bool			m_disabled;
	bool			m_succeeded;
};

BEGIN_DATADESC( CLogicCollisionPair )
	DEFINE_KEYFIELD( m_nameAttach1, FIELD_STRING, "attach1" ),
	DEFINE_KEYFIELD( m_nameAttach2, FIELD_STRING, "attach2" ),
	DEFINE_KEYFIELD( m_disabled, FIELD_BOOLEAN, "startdisabled" ),
	DEFINE_FIELD( m_succeeded, FIELD_BOOLEAN ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableCollisions", InputDisableCollisions ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableCollisions", InputEnableCollisions ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( logic_collision_pair, CLogicCollisionPair );


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define MAX_LOGIC_BRANCH_NAMES 16

class CLogicBranchList : public CLogicalEntity
{
	DECLARE_CLASS( CLogicBranchList, CLogicalEntity );

	virtual void Spawn();
	virtual void Activate();
	virtual int DrawDebugTextOverlays( void );

private:

	enum LogicBranchListenerLastState_t
	{
		LOGIC_BRANCH_LISTENER_NOT_INIT = 0,
		LOGIC_BRANCH_LISTENER_ALL_TRUE,
		LOGIC_BRANCH_LISTENER_ALL_FALSE,
		LOGIC_BRANCH_LISTENER_MIXED,
	};

	void DoTest( CBaseEntity *pActivator );

	string_t m_nLogicBranchNames[MAX_LOGIC_BRANCH_NAMES];
	CUtlVector<EHANDLE> m_LogicBranchList;
	LogicBranchListenerLastState_t m_eLastState;

	// Inputs
	void Input_OnLogicBranchRemoved( inputdata_t &inputdata );
	void Input_OnLogicBranchChanged( inputdata_t &inputdata );
	void InputTest( inputdata_t &inputdata );

	// Outputs
	COutputEvent m_OnAllTrue;			// Fired when all the registered logic_branches become true.
	COutputEvent m_OnAllFalse;			// Fired when all the registered logic_branches become false.
	COutputEvent m_OnMixed;				// Fired when one of the registered logic branches changes, but not all are true or false.

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(logic_branch_listener, CLogicBranchList);


BEGIN_DATADESC( CLogicBranchList )

	// Silence, classcheck!
	//DEFINE_ARRAY( m_nLogicBranchNames, FIELD_STRING, MAX_LOGIC_BRANCH_NAMES ),

	// Keys
	DEFINE_KEYFIELD( m_nLogicBranchNames[0], FIELD_STRING, "Branch01" ),
	DEFINE_KEYFIELD( m_nLogicBranchNames[1], FIELD_STRING, "Branch02" ),
	DEFINE_KEYFIELD( m_nLogicBranchNames[2], FIELD_STRING, "Branch03" ),
	DEFINE_KEYFIELD( m_nLogicBranchNames[3], FIELD_STRING, "Branch04" ),
	DEFINE_KEYFIELD( m_nLogicBranchNames[4], FIELD_STRING, "Branch05" ),
	DEFINE_KEYFIELD( m_nLogicBranchNames[5], FIELD_STRING, "Branch06" ),
	DEFINE_KEYFIELD( m_nLogicBranchNames[6], FIELD_STRING, "Branch07" ),
	DEFINE_KEYFIELD( m_nLogicBranchNames[7], FIELD_STRING, "Branch08" ),
	DEFINE_KEYFIELD( m_nLogicBranchNames[8], FIELD_STRING, "Branch09" ),
	DEFINE_KEYFIELD( m_nLogicBranchNames[9], FIELD_STRING, "Branch10" ),
	DEFINE_KEYFIELD( m_nLogicBranchNames[10], FIELD_STRING, "Branch11" ),
	DEFINE_KEYFIELD( m_nLogicBranchNames[11], FIELD_STRING, "Branch12" ),
	DEFINE_KEYFIELD( m_nLogicBranchNames[12], FIELD_STRING, "Branch13" ),
	DEFINE_KEYFIELD( m_nLogicBranchNames[13], FIELD_STRING, "Branch14" ),
	DEFINE_KEYFIELD( m_nLogicBranchNames[14], FIELD_STRING, "Branch15" ),
	DEFINE_KEYFIELD( m_nLogicBranchNames[15], FIELD_STRING, "Branch16" ),
	
	DEFINE_UTLVECTOR( m_LogicBranchList, FIELD_EHANDLE ),
	
	DEFINE_FIELD( m_eLastState, FIELD_INTEGER ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_INPUT, "Test", InputTest ),
	DEFINE_INPUTFUNC( FIELD_INPUT, "_OnLogicBranchChanged", Input_OnLogicBranchChanged ),
	DEFINE_INPUTFUNC( FIELD_INPUT, "_OnLogicBranchRemoved", Input_OnLogicBranchRemoved ),

	// Outputs
	DEFINE_OUTPUT( m_OnAllTrue, "OnAllTrue" ),
	DEFINE_OUTPUT( m_OnAllFalse, "OnAllFalse" ),
	DEFINE_OUTPUT( m_OnMixed, "OnMixed" ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Called before spawning, after key values have been set.
//-----------------------------------------------------------------------------
void CLogicBranchList::Spawn( void )
{
}


//-----------------------------------------------------------------------------
// Finds all the logic_branches that we are monitoring and register ourselves with them.
//-----------------------------------------------------------------------------
void CLogicBranchList::Activate( void )
{
	for ( int i = 0; i < MAX_LOGIC_BRANCH_NAMES; i++ )
	{
		CBaseEntity *pEntity = NULL;
		while ( ( pEntity = gEntList.FindEntityGeneric( pEntity, STRING( m_nLogicBranchNames[i] ), this ) ) != NULL )
		{
			if ( FClassnameIs( pEntity, "logic_branch" ) )
			{
				CLogicBranch *pBranch = (CLogicBranch *)pEntity;
				pBranch->AddLogicBranchListener( this );
				m_LogicBranchList.AddToTail( pBranch );
			}
			else
			{
				DevWarning( "logic_branchlist %s refers to entity %s, which is not a logic_branch\n", GetDebugName(), pEntity->GetDebugName() );
			}
		}
	}
	
	BaseClass::Activate();
}


//-----------------------------------------------------------------------------
// Called when a monitored logic branch is deleted from the world, since that
// might affect our final result.
//-----------------------------------------------------------------------------
void CLogicBranchList::Input_OnLogicBranchRemoved( inputdata_t &inputdata )
{
	int nIndex = m_LogicBranchList.Find( inputdata.pActivator );
	if ( nIndex != -1 )
	{
		m_LogicBranchList.FastRemove( nIndex );
	}

	// See if this logic_branch's deletion affects the final result.
	DoTest( inputdata.pActivator );
}


//-----------------------------------------------------------------------------
// Called when the value of a monitored logic branch changes.
//-----------------------------------------------------------------------------
void CLogicBranchList::Input_OnLogicBranchChanged( inputdata_t &inputdata )
{
	DoTest( inputdata.pActivator );
}


//-----------------------------------------------------------------------------
// Input handler to manually test the monitored logic branches and fire the
// appropriate output.
//-----------------------------------------------------------------------------
void CLogicBranchList::InputTest( inputdata_t &inputdata )
{
	// Force an output.
	m_eLastState = LOGIC_BRANCH_LISTENER_NOT_INIT;

	DoTest( inputdata.pActivator );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLogicBranchList::DoTest( CBaseEntity *pActivator )
{
	bool bOneTrue = false;
	bool bOneFalse = false;
	
	for ( int i = 0; i < m_LogicBranchList.Count(); i++ )
	{
		CLogicBranch *pBranch = (CLogicBranch *)m_LogicBranchList.Element( i ).Get();
		if ( pBranch && pBranch->GetLogicBranchState() )
		{
			bOneTrue = true;
		}
		else
		{
			bOneFalse = true;
		}
	}

	// Only fire the output if the new result differs from the last result.
	if ( bOneTrue && !bOneFalse )
	{
		if ( m_eLastState != LOGIC_BRANCH_LISTENER_ALL_TRUE )
		{
			m_OnAllTrue.FireOutput( pActivator, this );
			m_eLastState = LOGIC_BRANCH_LISTENER_ALL_TRUE;
		}
	}
	else if ( bOneFalse && !bOneTrue )
	{
		if ( m_eLastState != LOGIC_BRANCH_LISTENER_ALL_FALSE )
		{
			m_OnAllFalse.FireOutput( pActivator, this );
			m_eLastState = LOGIC_BRANCH_LISTENER_ALL_FALSE;
		}
	}
	else
	{
		if ( m_eLastState != LOGIC_BRANCH_LISTENER_MIXED )
		{
			m_OnMixed.FireOutput( pActivator, this );
			m_eLastState = LOGIC_BRANCH_LISTENER_MIXED;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CLogicBranchList::DrawDebugTextOverlays( void )
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		for ( int i = 0; i < m_LogicBranchList.Count(); i++ )
		{
			CLogicBranch *pBranch = (CLogicBranch *)m_LogicBranchList.Element( i ).Get();
			if ( pBranch )
			{
				Q_snprintf( tempstr, sizeof(tempstr), "Branch (%s): %s", STRING(pBranch->GetEntityName()), (pBranch->GetLogicBranchState()) ? "TRUE" : "FALSE" );
				EntityText( text_offset, tempstr, 0 );
				text_offset++;
			}
		}
	}

	return text_offset;
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: Prints messages to the console.
//-----------------------------------------------------------------------------
class CLogicConsole : public CLogicalEntity
{
public:

	DECLARE_CLASS( CLogicConsole, CLogicalEntity );

	// Keys
	int m_iDevLevel = 1;
	Color m_MsgColor = Color(210, 250, 255, 255);
	Color m_WarningColor = Color(255, 210, 210, 255);
	bool m_bNewLineNotAuto = false;

	// TODO: Replace "append" with variable arguments?
	inline void LCMsg(const char *msg, const char *append = NULL) { ConColorMsg(m_MsgColor, msg, append); }
	inline void LCDevMsg(int lvl, const char *msg, const char *append = NULL) { developer.GetInt() >= lvl ? ConColorMsg(m_MsgColor, msg, append) : (void)0; }
	inline void LCWarning(const char *msg, const char *append = NULL) { ConColorMsg(m_WarningColor, msg, append); }
	inline void LCDevWarning(int lvl, const char *msg, const char *append = NULL) { developer.GetInt() >= lvl ? ConColorMsg(m_WarningColor, msg, append) : (void)0; }

	//inline void LCMsg(const char *msg, const char *append = NULL) { ColorSpewMessage(SPEW_MESSAGE, &m_MsgColor, msg, append); }
	//inline void LCDevMsg(int lvl, const char *msg, const char *append = NULL) { developer.GetInt() >= lvl ? ColorSpewMessage(SPEW_MESSAGE, &m_MsgColor, msg, append) : (void)0; }
	//inline void LCWarning(const char *msg, const char *append = NULL) { ColorSpewMessage(SPEW_MESSAGE, &m_WarningColor, msg, append); }
	//inline void LCDevWarning(int lvl, const char *msg, const char *append = NULL) { developer.GetInt() >= lvl ? ColorSpewMessage(SPEW_MESSAGE, &m_WarningColor, msg, append) : (void)0; }

	// Inputs
	void InputSendMsg( inputdata_t &inputdata ) { !m_bNewLineNotAuto ? LCMsg("%s\n", inputdata.value.String()) : LCMsg("%s", inputdata.value.String()); }
	void InputSendWarning( inputdata_t &inputdata ) { !m_bNewLineNotAuto ? LCWarning("%s\n", inputdata.value.String()) : LCWarning("%s", inputdata.value.String()); }
	void InputSendDevMsg( inputdata_t &inputdata ) { !m_bNewLineNotAuto ? LCDevMsg(m_iDevLevel, "%s\n", inputdata.value.String()) : LCDevMsg(m_iDevLevel, "%s", inputdata.value.String()); }
	void InputSendDevWarning( inputdata_t &inputdata ) { !m_bNewLineNotAuto ? LCDevWarning(m_iDevLevel, "%s\n", inputdata.value.String()) : LCDevWarning(m_iDevLevel, "%s", inputdata.value.String()); }

	void InputNewLine( inputdata_t &inputdata ) { LCMsg("\n"); }
	void InputDevNewLine( inputdata_t &inputdata ) { LCDevMsg(m_iDevLevel, "\n"); }

	// MAPBASE MP TODO: "ClearConsoleOnTarget"
	// (and make this input broadcast to all players)
	void InputClearConsole( inputdata_t &inputdata ) { UTIL_GetLocalPlayer() ? engine->ClientCommand(UTIL_GetLocalPlayer()->edict(), "clear") : (void)0; }

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(logic_console, CLogicConsole);


BEGIN_DATADESC( CLogicConsole )

	DEFINE_INPUT( m_iDevLevel, FIELD_INTEGER, "SetDevLvl" ),
	DEFINE_INPUT( m_MsgColor, FIELD_COLOR32, "SetMsgColor" ),
	DEFINE_INPUT( m_WarningColor, FIELD_COLOR32, "SetWarningColor" ),
	DEFINE_INPUT( m_bNewLineNotAuto, FIELD_BOOLEAN, "SetNewLineNotAuto" ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SendMsg", InputSendMsg ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SendWarning", InputSendWarning ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SendDevMsg", InputSendDevMsg ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SendDevWarning", InputSendDevWarning ),

	DEFINE_INPUTFUNC( FIELD_VOID, "NewLine", InputNewLine ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DevNewLine", InputDevNewLine ),

	DEFINE_INPUTFUNC( FIELD_VOID, "ClearConsole", InputClearConsole ),

END_DATADESC()

ConVar sv_allow_logic_convar( "sv_allow_logic_convar", "1", FCVAR_NOT_CONNECTED );

//-----------------------------------------------------------------------------
// Purpose: Gets console variables for the evil mapper.
//-----------------------------------------------------------------------------
class CLogicConvar : public CLogicalEntity
{
public:

	DECLARE_CLASS( CLogicConvar, CLogicalEntity );

	// Keys
	string_t m_iszConVar;
	string_t m_iszCompareValue;

	const char *GetConVarString( inputdata_t &inputdata );

	// Inputs
	void InputGetValue( inputdata_t &inputdata );
	void InputTest( inputdata_t &inputdata );

	// Outputs
	COutputEvent m_OnTrue;
	COutputEvent m_OnFalse;
	COutputString m_OutValue;
	COutputEvent m_OnDenied;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(logic_convar, CLogicConvar);


BEGIN_DATADESC( CLogicConvar )

	DEFINE_INPUT( m_iszConVar, FIELD_STRING, "SetConVar" ),
	DEFINE_INPUT( m_iszCompareValue, FIELD_STRING, "SetTestValue" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "GetValue", InputGetValue ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Test", InputTest ),

	DEFINE_OUTPUT(m_OnTrue, "OnTrue"),
	DEFINE_OUTPUT(m_OnFalse, "OnFalse"),
	DEFINE_OUTPUT(m_OutValue, "OutValue"),
	DEFINE_OUTPUT(m_OnDenied, "OnDenied"),

END_DATADESC()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const char *CLogicConvar::GetConVarString( inputdata_t &inputdata )
{
	if (!sv_allow_logic_convar.GetBool())
	{
		m_OnDenied.FireOutput(this, this);

		//return ConVarRef("<null>", true);
		return NULL;
	}

	ConVarRef pCVar = ConVarRef(STRING(m_iszConVar), true);
	if (!pCVar.IsValid())
	{
		const char *pszCVar = STRING( m_iszConVar );
		CBasePlayer *pPlayer = ToBasePlayer( inputdata.pActivator );
		if (!pPlayer && AI_IsSinglePlayer())
			pPlayer = UTIL_PlayerByIndex( 1 );

		if (pPlayer)
		{
			// Check if it's a common cheat command a player might be using
			if (FStrEq( pszCVar, "god" ))
				return (pPlayer->GetFlags() & FL_GODMODE) ? "1" : "0";
			if (FStrEq( pszCVar, "notarget" ))
				return (pPlayer->GetFlags() & FL_NOTARGET) ? "1" : "0";
			if (FStrEq( pszCVar, "noclip" ))
				return (pPlayer->IsEFlagSet(EFL_NOCLIP_ACTIVE)) ? "1" : "0";

			// It might be a client convar
			// This function returns a blank string if the convar doesn't exist, so we have to put this at the end
			const char *pszClientValue = engine->GetClientConVarValue( pPlayer->GetClientIndex(), pszCVar );
			if (pszClientValue)
			{
				return pszClientValue;
			}
		}

		//Warning("Warning: %s has invalid convar \"%s\"\n", GetDebugName(), STRING(m_iszConVar));
	}

	return pCVar.GetString();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLogicConvar::InputGetValue( inputdata_t &inputdata )
{
	const char *pCVarString = GetConVarString(inputdata);
	if (pCVarString != NULL)
		m_OutValue.Set( AllocPooledString( pCVarString ), inputdata.pActivator, this );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLogicConvar::InputTest( inputdata_t &inputdata )
{
	const char *pCVarString = GetConVarString(inputdata);
	if (pCVarString)
	{
		if (Matcher_Match( STRING( m_iszCompareValue ), pCVarString ))
		{
			m_OnTrue.FireOutput(inputdata.pActivator, this);
		}
		else
		{
			m_OnFalse.FireOutput(inputdata.pActivator, this);
		}
	}
}

#define MAX_LOGIC_FORMAT_PARAMETERS 8
//-----------------------------------------------------------------------------
// Purpose: Takes a string and a bunch of parameters and spits out a formatted string.
//-----------------------------------------------------------------------------
class CLogicFormat : public CLogicalEntity
{
public:

	DECLARE_CLASS( CLogicFormat, CLogicalEntity );

	// Keys
	string_t m_iszInput;
	string_t m_iszParameter[MAX_LOGIC_FORMAT_PARAMETERS];
	string_t m_iszBackupParameter;

	void FormatString(const char *szStringToFormat, char *szOutput, int outputlen);

	// Inputs
	void InputGetFormattedString( inputdata_t &inputdata );

	// Outputs
	COutputString m_OutFormattedString;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(logic_format, CLogicFormat);


BEGIN_DATADESC( CLogicFormat )

	DEFINE_INPUT( m_iszInput, FIELD_STRING, "SetInputValue" ),
	DEFINE_INPUT( m_iszParameter[0], FIELD_STRING, "SetParameter0" ),
	DEFINE_INPUT( m_iszParameter[1], FIELD_STRING, "SetParameter1" ),
	DEFINE_INPUT( m_iszParameter[2], FIELD_STRING, "SetParameter2" ),
	DEFINE_INPUT( m_iszParameter[3], FIELD_STRING, "SetParameter3" ),
	DEFINE_INPUT( m_iszParameter[4], FIELD_STRING, "SetParameter4" ),
	DEFINE_INPUT( m_iszParameter[5], FIELD_STRING, "SetParameter5" ),
	DEFINE_INPUT( m_iszParameter[6], FIELD_STRING, "SetParameter6" ),
	DEFINE_INPUT( m_iszParameter[7], FIELD_STRING, "SetParameter7" ),
	DEFINE_INPUT( m_iszBackupParameter, FIELD_STRING, "SetBackupParameter" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "GetFormattedValue", InputGetFormattedString ),

	DEFINE_OUTPUT(m_OutFormattedString, "OutFormattedValue"),

END_DATADESC()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLogicFormat::InputGetFormattedString( inputdata_t &inputdata )
{
	char szFormatted[256];
	if (m_iszInput != NULL_STRING)
	{
		FormatString(STRING(m_iszInput), szFormatted, sizeof(szFormatted));
		m_OutFormattedString.Set(AllocPooledString(szFormatted), inputdata.pActivator, this);
	}
}

//-----------------------------------------------------------------------------
// I'm bad at coding.
//-----------------------------------------------------------------------------
void CLogicFormat::FormatString(const char *szStringToFormat, char *szOutput, int outputlen)
{
	const char *szParameters[MAX_LOGIC_FORMAT_PARAMETERS];
	for (int i = 0; i < MAX_LOGIC_FORMAT_PARAMETERS; i++)
	{
		if (m_iszParameter[i] != NULL_STRING)
		{
			szParameters[i] = STRING(m_iszParameter[i]);
		}
		else if (m_iszBackupParameter != NULL_STRING)
		{
			szParameters[i] = STRING(m_iszBackupParameter);
		}
		else
		{
			szParameters[i] = "<null>";
		}
	}

	char szFormatted[256] = { 0 }; // Needed so garbage isn't spewed at the beginning
	//Q_snprintf(szFormatted, sizeof(szFormatted), szInput, szParameters);

	char szInput[256];
	Q_strncpy(szInput, szStringToFormat, sizeof(szInput));

	bool inparam = (szInput[0] == '{');
	int curparam = 0;
	char *szToken = strtok(szInput, "{");
	while (szToken != NULL)
	{
		if (inparam)
		{
			curparam = atoi(szToken);
			if (curparam < MAX_LOGIC_FORMAT_PARAMETERS /*&& szParameters[curparam] != NULL*/) //if (curparam < MAX_FORMAT_PARAMETERS)
			{
				Q_strncat(szFormatted, szParameters[curparam], sizeof(szFormatted));
			}
			else
			{
				Warning("Warning: Parameter %i out of bounds in \"%s\"\n", curparam, szStringToFormat);

				// This might not be the best way to do this, but
				// reaching it is supposed to be the result of a mistake anyway.
				m_iszBackupParameter != NULL_STRING ?
					Q_strncat( szFormatted, STRING(m_iszBackupParameter), sizeof( szFormatted ) ) :
					Q_strncat( szFormatted, "<null>", sizeof( szFormatted ) );
			}

			inparam = false;
			szToken = strtok(NULL, "{");
		}
		else
		{
			Q_strncat( szFormatted, szToken, sizeof( szFormatted ) );

			inparam = true;
			szToken = strtok(NULL, "}");
		}
	}

	Q_strncpy(szOutput, szFormatted, outputlen);
}


//-----------------------------------------------------------------------------
// Purpose: Accesses a keyvalue from a specific entity
// Mostly ported from Half-Laugh.
//-----------------------------------------------------------------------------
class CLogicKeyfieldAccessor : public CLogicalEntity
{
	DECLARE_CLASS(CLogicKeyfieldAccessor, CLogicalEntity);

protected:
	CBaseEntity *GetTarget(CBaseEntity *pCaller, CBaseEntity *pActivator);

	virtual bool TestKey(CBaseEntity *pTarget, const char *szKeyName);
	virtual bool SetKeyValue(CBaseEntity *pTarget, const char *szKeyName, const char *szValue);
	virtual bool SetKeyValueBits(CBaseEntity *pTarget, const char *szKeyName, int iValue, bool bRemove = false);

	// Inputs
	void InputTest(inputdata_t &inputdata);
	void InputTestKey(inputdata_t &inputdata);
	void InputTestTarget(inputdata_t &inputdata);

	void InputSetKey(inputdata_t &inputdata);

	void InputSetValue(inputdata_t &inputdata);
	void InputAddBits(inputdata_t &inputdata);
	void InputRemoveBits(inputdata_t &inputdata);

	//bool ReadUnregisteredKeyfields(CBaseEntity *pTarget, const char *szKeyName, variant_t *variant);

	COutputVariant m_OutValue;
	COutputEvent m_OnFailed;

	string_t m_iszKey;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(logic_keyfield, CLogicKeyfieldAccessor);


BEGIN_DATADESC(CLogicKeyfieldAccessor)

DEFINE_KEYFIELD( m_iszKey, FIELD_STRING, "keyname" ),

// Inputs
DEFINE_INPUTFUNC(FIELD_VOID, "Test", InputTest),
DEFINE_INPUTFUNC(FIELD_STRING, "TestKey", InputTestKey),
DEFINE_INPUTFUNC(FIELD_STRING, "TestTarget", InputTestTarget),
DEFINE_INPUTFUNC(FIELD_STRING, "SetKey", InputSetKey),

DEFINE_INPUTFUNC(FIELD_STRING, "SetValue", InputSetValue),
DEFINE_INPUTFUNC(FIELD_INTEGER, "AddBits", InputAddBits),
DEFINE_INPUTFUNC(FIELD_INTEGER, "RemoveBits", InputRemoveBits),

DEFINE_OUTPUT( m_OutValue, "OutValue" ),
DEFINE_OUTPUT( m_OnFailed, "OnFailed" ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CBaseEntity *CLogicKeyfieldAccessor::GetTarget(CBaseEntity *pCaller, CBaseEntity *pActivator)
{
	return gEntList.FindEntityByName(NULL, m_target, this, pActivator, pCaller);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CLogicKeyfieldAccessor::TestKey(CBaseEntity *pTarget, const char *szKeyName)
{
	variant_t variant;
	if (pTarget->ReadKeyField(szKeyName, &variant) || ReadUnregisteredKeyfields(pTarget, szKeyName, &variant))
	{
		m_OutValue.Set(variant, pTarget, this);
		return true;
	}
	else
	{
		m_OnFailed.FireOutput(pTarget, this);
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CLogicKeyfieldAccessor::SetKeyValue(CBaseEntity *pTarget, const char *szKeyName, const char *szValue)
{
	if (pTarget->KeyValue(szKeyName, szValue))
	{
		// We'll still fire OutValue
		variant_t variant;
		if (!pTarget->ReadKeyField(szKeyName, &variant))
			ReadUnregisteredKeyfields(pTarget, szKeyName, &variant);

		m_OutValue.Set(variant, pTarget, this);
		return true;
	}
	else
	{
		m_OnFailed.FireOutput(pTarget, this);
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CLogicKeyfieldAccessor::SetKeyValueBits(CBaseEntity *pTarget, const char *szKeyName, int iValue, bool bRemove)
{
	variant_t variant;
	if ((pTarget->ReadKeyField(szKeyName, &variant) || ReadUnregisteredKeyfields(pTarget, szKeyName, &variant)) && variant.FieldType() == FIELD_INTEGER)
	{
		if (bRemove)
			variant.SetInt(variant.Int() & ~iValue);
		else
			variant.SetInt(variant.Int() | iValue);

		pTarget->KeyValue(szKeyName, UTIL_VarArgs("%i", variant.Int()));

		m_OutValue.Set(variant, pTarget, this);
		return true;
	}
	else
	{
		m_OnFailed.FireOutput(pTarget, this);
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicKeyfieldAccessor::InputTest(inputdata_t &inputdata)
{
	CBaseEntity *pTarget = GetTarget(inputdata.pCaller, inputdata.pActivator);
	if (pTarget && m_iszKey != NULL_STRING)
	{
		TestKey(pTarget, STRING(m_iszKey));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicKeyfieldAccessor::InputTestKey(inputdata_t &inputdata)
{
	const char *input = inputdata.value.String();
	CBaseEntity *pTarget = GetTarget(inputdata.pCaller, inputdata.pActivator);
	if (input && pTarget)
	{
		TestKey(pTarget, input);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicKeyfieldAccessor::InputTestTarget(inputdata_t &inputdata)
{
	m_target = inputdata.value.StringID();
	CBaseEntity *pTarget = gEntList.FindEntityByName(NULL, inputdata.value.StringID(), this, inputdata.pCaller, inputdata.pActivator);
	if (pTarget && m_iszKey != NULL_STRING)
	{
		TestKey(pTarget, STRING(m_iszKey));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicKeyfieldAccessor::InputSetKey(inputdata_t &inputdata)
{
	m_iszKey = inputdata.value.StringID();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicKeyfieldAccessor::InputSetValue(inputdata_t &inputdata)
{
	const char *input = inputdata.value.String();
	CBaseEntity *pTarget = GetTarget(inputdata.pCaller, inputdata.pActivator);
	if (input && pTarget)
	{
		SetKeyValue(pTarget, STRING(m_iszKey), input);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicKeyfieldAccessor::InputAddBits(inputdata_t &inputdata)
{
	int input = inputdata.value.Int();
	CBaseEntity *pTarget = GetTarget(inputdata.pCaller, inputdata.pActivator);
	if (input && pTarget)
	{
		SetKeyValueBits(pTarget, STRING(m_iszKey), input, false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicKeyfieldAccessor::InputRemoveBits(inputdata_t &inputdata)
{
	int input = inputdata.value.Int();
	CBaseEntity *pTarget = GetTarget(inputdata.pCaller, inputdata.pActivator);
	if (input && pTarget)
	{
		SetKeyValueBits(pTarget, STRING(m_iszKey), input, true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Clamps the input value between two values
//-----------------------------------------------------------------------------
class CMathClamp : public CLogicalEntity
{
public:

	DECLARE_CLASS( CMathClamp, CLogicalEntity );

	// Keys
	variant_t m_Max;
	variant_t m_Min;

	// Inputs
	void InputClampValue( inputdata_t &inputdata );

	float ClampValue(float input, float min, float max, int *bounds);
	void ClampValue(variant_t var, inputdata_t *inputdata);

	// Outputs
	COutputVariant m_OutValue;
	COutputVariant m_OnBeforeMin;
	COutputVariant m_OnBeyondMax;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(math_clamp, CMathClamp);


BEGIN_DATADESC( CMathClamp )

	DEFINE_INPUT( m_Max, FIELD_INPUT, "SetMax" ),
	DEFINE_INPUT( m_Min, FIELD_INPUT, "SetMin" ),

	DEFINE_INPUTFUNC( FIELD_INPUT, "ClampValue", InputClampValue ),

	DEFINE_OUTPUT(m_OutValue, "OutValue"),
	DEFINE_OUTPUT(m_OnBeforeMin, "OnBeforeMin"),
	DEFINE_OUTPUT(m_OnBeyondMax, "OnBeyondMax"),

END_DATADESC()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathClamp::InputClampValue( inputdata_t &inputdata )
{
	ClampValue(inputdata.value, &inputdata);
}

//-----------------------------------------------------------------------------
// "bounds" returns 1 if the number was less than min, 2 if more than max. Must not be NULL
//-----------------------------------------------------------------------------
inline float CMathClamp::ClampValue(float input, float min, float max, int *bounds)
{
	if ( max < min )
	{
		Warning("WARNING: Max value (%f) less than min value (%f) in %s!\n", max, min, GetDebugName());
		return max;
	}
	else if( input < min )
	{
		*bounds = 1;
		return min;
	}
	else if( input > max )
	{
		*bounds = 2;
		return max;
	}
	else
		return input;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathClamp::ClampValue(variant_t var, inputdata_t *inputdata)
{
	// Don't convert up here in case of invalid type

	int nBounds = 0;

	switch (var.FieldType())
	{
		case FIELD_FLOAT:
		{
			m_Max.Convert(var.FieldType());
			m_Min.Convert(var.FieldType());

			var.SetFloat(ClampValue(var.Float(), m_Max.Float(), m_Min.Float(), &nBounds));
		} break;
		case FIELD_INTEGER:
		{
			m_Max.Convert(var.FieldType());
			m_Min.Convert(var.FieldType());

			var.SetInt(ClampValue(var.Int(), m_Max.Int(), m_Min.Int(), &nBounds));
		} break;
		case FIELD_VECTOR:
		{
			m_Max.Convert(var.FieldType());
			m_Min.Convert(var.FieldType());

			Vector min;
			Vector max;
			m_Min.Vector3D(min);
			m_Max.Vector3D(max);

			Vector vec;
			var.Vector3D(vec);

			vec.x = ClampValue(vec.x, min.x, max.x, &nBounds);
			vec.y = ClampValue(vec.y, min.y, max.y, &nBounds);
			vec.z = ClampValue(vec.z, min.z, max.z, &nBounds);

			var.SetVector3D(vec);
		} break;
		default:
		{
			Warning("Error: Unsupported value %s in math_clamp %s\n", var.GetDebug(), STRING(GetEntityName()));
			return;
		}
	}

	if (inputdata)
	{
		m_OutValue.Set(var, inputdata->pActivator, this);
		if (nBounds == 1)
			m_OnBeforeMin.Set(var, inputdata->pActivator, this);
		else if (nBounds == 2)
			m_OnBeyondMax.Set(var, inputdata->pActivator, this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Bits calculations.
//-----------------------------------------------------------------------------
class CMathBits : public CLogicalEntity
{
	DECLARE_CLASS( CMathBits, CLogicalEntity );
private:

	bool m_bDisabled;

	bool KeyValue(const char *szKeyName, const char *szValue);

	void UpdateOutValue(CBaseEntity *pActivator, int iNewValue);

	int DrawDebugTextOverlays(void);

	// Inputs
	void InputAdd( inputdata_t &inputdata );
	void InputSubtract( inputdata_t &inputdata );
	void InputShiftLeft( inputdata_t &inputdata );
	void InputShiftRight( inputdata_t &inputdata );
	void InputApplyAnd( inputdata_t &inputdata );
	void InputApplyOr( inputdata_t &inputdata );
	void InputSetValue( inputdata_t &inputdata );
	void InputSetValueNoFire( inputdata_t &inputdata );
	void InputGetValue( inputdata_t &inputdata );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputContainsBits( inputdata_t &inputdata );
	void InputContainsAllBits( inputdata_t &inputdata );

	// Outputs
	COutputInt m_OutValue;
	COutputInt m_OnGetValue;
	COutputEvent m_OnTrue;
	COutputEvent m_OnFalse;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(math_bits, CMathBits);


BEGIN_DATADESC( CMathBits )

	// Keys
	DEFINE_KEYFIELD(m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_INTEGER, "Add", InputAdd),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "Subtract", InputSubtract),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "ShiftLeft", InputShiftLeft),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "ShiftRight", InputShiftRight),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "ApplyAnd", InputApplyAnd),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "ApplyOr", InputApplyOr),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetValue", InputSetValue),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetValueNoFire", InputSetValueNoFire),
	DEFINE_INPUTFUNC(FIELD_VOID, "GetValue", InputGetValue),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "ContainsBits", InputContainsBits),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "ContainsAllBits", InputContainsAllBits),

	// Outputs
	DEFINE_OUTPUT(m_OutValue, "OutValue"),
	DEFINE_OUTPUT(m_OnGetValue, "OnGetValue"),
	DEFINE_OUTPUT(m_OnTrue, "OnTrue"),
	DEFINE_OUTPUT(m_OnFalse, "OnFalse"),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Handles key values from the BSP before spawn is called.
//-----------------------------------------------------------------------------
bool CMathBits::KeyValue(const char *szKeyName, const char *szValue)
{
	//
	// Set the initial value of the counter.
	//
	if (!stricmp(szKeyName, "startvalue"))
	{
		m_OutValue.Init(atoi(szValue));
		return(true);
	}

	return(BaseClass::KeyValue(szKeyName, szValue));
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for adding to the accumulator value.
// Input  : Bit value to add.
//-----------------------------------------------------------------------------
void CMathBits::InputAdd( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Bits %s ignoring ADD because it is disabled\n", GetDebugName() );
		return;
	}

	int iNewValue = m_OutValue.Get() | inputdata.value.Int();
	UpdateOutValue( inputdata.pActivator, iNewValue );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for subtracting from the current value.
// Input  : Bit value to subtract.
//-----------------------------------------------------------------------------
void CMathBits::InputSubtract( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Bits %s ignoring SUBTRACT because it is disabled\n", GetDebugName() );
		return;
	}

	int iNewValue = m_OutValue.Get() & ~inputdata.value.Int();
	UpdateOutValue( inputdata.pActivator, iNewValue );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for shifting from the current value.
// Input  : Bit value to shift by.
//-----------------------------------------------------------------------------
void CMathBits::InputShiftLeft( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Bits %s ignoring SHIFTLEFT because it is disabled\n", GetDebugName() );
		return;
	}

	int iNewValue = m_OutValue.Get() << inputdata.value.Int();
	UpdateOutValue( inputdata.pActivator, iNewValue );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for shifting from the current value.
// Input  : Bit value to shift by.
//-----------------------------------------------------------------------------
void CMathBits::InputShiftRight( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Bits %s ignoring SHIFTRIGHT because it is disabled\n", GetDebugName() );
		return;
	}

	int iNewValue = m_OutValue.Get() >> inputdata.value.Int();
	UpdateOutValue( inputdata.pActivator, iNewValue );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for applying & to the current value.
// Input  : Bit value to shift by.
//-----------------------------------------------------------------------------
void CMathBits::InputApplyAnd( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Bits %s ignoring APPLYAND because it is disabled\n", GetDebugName() );
		return;
	}

	int iNewValue = m_OutValue.Get() & inputdata.value.Int();
	UpdateOutValue( inputdata.pActivator, iNewValue );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for applying | to the current value.
// Input  : Bit value to shift by.
//-----------------------------------------------------------------------------
void CMathBits::InputApplyOr( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Bits %s ignoring APPLYOR because it is disabled\n", GetDebugName() );
		return;
	}

	int iNewValue = m_OutValue.Get() | inputdata.value.Int();
	UpdateOutValue( inputdata.pActivator, iNewValue );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for updating the value.
// Input  : Bit value to set.
//-----------------------------------------------------------------------------
void CMathBits::InputSetValue( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Bits %s ignoring SETVALUE because it is disabled\n", GetDebugName() );
		return;
	}

	UpdateOutValue( inputdata.pActivator, inputdata.value.Int() );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for updating the value.
// Input  : Bit value to set.
//-----------------------------------------------------------------------------
void CMathBits::InputSetValueNoFire( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Bits %s ignoring SETVALUENOFIRE because it is disabled\n", GetDebugName() );
		return;
	}

	m_OutValue.Init( inputdata.value.Int() );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathBits::InputGetValue( inputdata_t &inputdata )
{
	int iOutValue = m_OutValue.Get();
	m_OnGetValue.Set( iOutValue, inputdata.pActivator, inputdata.pCaller );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for checking whether a bit is stored.
// Input  : Bit value to check.
//-----------------------------------------------------------------------------
void CMathBits::InputContainsBits( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Bits %s ignoring CONTAINS BITS because it is disabled\n", GetDebugName() );
		return;
	}

	if (m_OutValue.Get() & inputdata.value.Int())
		m_OnTrue.FireOutput(inputdata.pActivator, this);
	else
		m_OnFalse.FireOutput(inputdata.pActivator, this);
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for checking whether all of the specified bits are stored.
// Input  : Bit value to check.
//-----------------------------------------------------------------------------
void CMathBits::InputContainsAllBits( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Bits %s ignoring CONTAINS ALL BITS because it is disabled\n", GetDebugName() );
		return;
	}

	bool bResult = false;
	int iInput = inputdata.value.Int();
	int iValue = m_OutValue.Get();

	for (int i = 1, n = 0; n < 32; (i <<= 1), n++)
	{
		DevMsg("%i\n", i);
		if (iInput & i)
		{
			if (!(iValue & i))
			{
				DevMsg("%i does not go into %i\n", i, iValue);
				bResult = false;
				break;
			}
			else if (!bResult)
			{
				bResult = true;
			}
		}
	}

	if (bResult)
		m_OnTrue.FireOutput(inputdata.pActivator, this);
	else
		m_OnFalse.FireOutput(inputdata.pActivator, this);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathBits::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathBits::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the value to the new value, firing the output value.
// Input  : iNewValue - Value to set.
//-----------------------------------------------------------------------------
void CMathBits::UpdateOutValue(CBaseEntity *pActivator, int iNewValue)
{
	m_OutValue.Set(iNewValue, pActivator, this);
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CMathBits::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		Q_snprintf(tempstr,sizeof(tempstr),"current value: %i", m_OutValue.Get());
		EntityText(text_offset,tempstr,0);
		text_offset++;

		if( m_bDisabled )
		{	
			Q_snprintf(tempstr,sizeof(tempstr),"*DISABLED*");		
		}
		else
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Enabled.");
		}
		EntityText(text_offset,tempstr,0);
		text_offset++;

	}
	return text_offset;
}

// These spawnflags control math_vector dimensions.
#define SF_MATH_VECTOR_DISABLE_X ( 1 << 0 )
#define SF_MATH_VECTOR_DISABLE_Y ( 1 << 1 )
#define SF_MATH_VECTOR_DISABLE_Z ( 1 << 2 )

//-----------------------------------------------------------------------------
// Purpose: Vector calculations.
//-----------------------------------------------------------------------------
class CMathVector : public CLogicalEntity
{
	DECLARE_CLASS( CMathVector, CLogicalEntity );
private:

	bool m_bDisabled;

	bool KeyValue(const char *szKeyName, const char *szValue);
	bool KeyValue( const char *szKeyName, const Vector &vecValue );

	void UpdateOutValue(CBaseEntity *pActivator, Vector vecNewValue);

	int DrawDebugTextOverlays(void);

	// Inputs
	void InputAdd( inputdata_t &inputdata );
	void InputSubtract( inputdata_t &inputdata );
	void InputDivide( inputdata_t &inputdata );
	void InputMultiply( inputdata_t &inputdata );
	void InputSetValue( inputdata_t &inputdata );
	void InputSetValueNoFire( inputdata_t &inputdata );
	void InputGetValue( inputdata_t &inputdata );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	void PointAt( Vector &origin, Vector &target, Vector &out );
	void InputPointAtLocation( inputdata_t &inputdata );
	void InputPointAtEntity( inputdata_t &inputdata );

	void InputNormalize( inputdata_t &inputdata );
	void InputNormalizeAngles( inputdata_t &inputdata );
	void InputVectorAngles( inputdata_t &inputdata );
	void InputAngleVectorForward( inputdata_t &inputdata );
	void InputAngleVectorRight( inputdata_t &inputdata );
	void InputAngleVectorUp( inputdata_t &inputdata );

	void SetCoordinate(float value, char coord, CBaseEntity *pActivator);
	void GetCoordinate(char coord, CBaseEntity *pActivator);
	void AddCoordinate(float value, char coord, CBaseEntity *pActivator);
	void SubtractCoordinate(float value, char coord, CBaseEntity *pActivator);

	void InputSetX( inputdata_t &inputdata ) { SetCoordinate(inputdata.value.Float(), 'X', inputdata.pActivator); }
	void InputSetY( inputdata_t &inputdata ) { SetCoordinate(inputdata.value.Float(), 'Y', inputdata.pActivator); }
	void InputSetZ( inputdata_t &inputdata ) { SetCoordinate(inputdata.value.Float(), 'Z', inputdata.pActivator); }
	void InputGetX( inputdata_t &inputdata ) { GetCoordinate('X', inputdata.pActivator); }
	void InputGetY( inputdata_t &inputdata ) { GetCoordinate('Y', inputdata.pActivator); }
	void InputGetZ( inputdata_t &inputdata ) { GetCoordinate('Z', inputdata.pActivator); }
	void InputAddX( inputdata_t &inputdata ) { AddCoordinate(inputdata.value.Float(), 'X', inputdata.pActivator); }
	void InputAddY( inputdata_t &inputdata ) { AddCoordinate(inputdata.value.Float(), 'Y', inputdata.pActivator); }
	void InputAddZ( inputdata_t &inputdata ) { AddCoordinate(inputdata.value.Float(), 'Z', inputdata.pActivator); }
	void InputSubtractX( inputdata_t &inputdata ) { SubtractCoordinate(inputdata.value.Float(), 'X', inputdata.pActivator); }
	void InputSubtractY( inputdata_t &inputdata ) { SubtractCoordinate(inputdata.value.Float(), 'Y', inputdata.pActivator); }
	void InputSubtractZ( inputdata_t &inputdata ) { SubtractCoordinate(inputdata.value.Float(), 'Z', inputdata.pActivator); }

	// Outputs
	COutputVector m_OutValue;
	COutputFloat m_OutX;
	COutputFloat m_OutY;
	COutputFloat m_OutZ;

	COutputVector m_OnGetValue;
	COutputFloat m_OnGetX;
	COutputFloat m_OnGetY;
	COutputFloat m_OnGetZ;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(math_vector, CMathVector);


BEGIN_DATADESC( CMathVector )

	// Keys
	DEFINE_KEYFIELD(m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VECTOR, "Add", InputAdd),
	DEFINE_INPUTFUNC(FIELD_VECTOR, "Subtract", InputSubtract),
	DEFINE_INPUTFUNC(FIELD_VECTOR, "Divide", InputDivide),
	DEFINE_INPUTFUNC(FIELD_VECTOR, "Multiply", InputMultiply),
	DEFINE_INPUTFUNC(FIELD_VECTOR, "SetValue", InputSetValue),
	DEFINE_INPUTFUNC(FIELD_VECTOR, "SetValueNoFire", InputSetValueNoFire),
	DEFINE_INPUTFUNC(FIELD_VOID, "GetValue", InputGetValue),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_INPUTFUNC( FIELD_VECTOR, "PointAtLocation", InputPointAtLocation ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "PointAtEntity", InputPointAtEntity ),

	DEFINE_INPUTFUNC(FIELD_VOID, "Normalize", InputNormalize),
	DEFINE_INPUTFUNC(FIELD_VOID, "NormalizeAngles", InputNormalizeAngles),
	DEFINE_INPUTFUNC(FIELD_VOID, "VectorAngles", InputVectorAngles),
	DEFINE_INPUTFUNC(FIELD_VOID, "AngleVectorForward", InputAngleVectorForward),
	DEFINE_INPUTFUNC(FIELD_VOID, "AngleVectorRight", InputAngleVectorRight),
	DEFINE_INPUTFUNC(FIELD_VOID, "AngleVectorUp", InputAngleVectorUp),

	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetX", InputSetX),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetY", InputSetY),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetZ", InputSetZ),
	DEFINE_INPUTFUNC(FIELD_VOID, "GetX", InputGetX),
	DEFINE_INPUTFUNC(FIELD_VOID, "GetY", InputGetY),
	DEFINE_INPUTFUNC(FIELD_VOID, "GetZ", InputGetZ),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "AddX", InputAddX),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "AddY", InputAddY),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "AddZ", InputAddZ),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SubtractX", InputSubtractX),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SubtractY", InputSubtractY),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SubtractZ", InputSubtractZ),

	// Outputs
	DEFINE_OUTPUT(m_OutValue, "OutValue"),
	DEFINE_OUTPUT(m_OutX, "OutX"),
	DEFINE_OUTPUT(m_OutY, "OutY"),
	DEFINE_OUTPUT(m_OutZ, "OutZ"),

	DEFINE_OUTPUT(m_OnGetValue, "OnGetValue"),
	DEFINE_OUTPUT(m_OnGetX, "OnGetX"),
	DEFINE_OUTPUT(m_OnGetY, "OnGetY"),
	DEFINE_OUTPUT(m_OnGetZ, "OnGetZ"),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Handles key values from the BSP before spawn is called.
//-----------------------------------------------------------------------------
bool CMathVector::KeyValue(const char *szKeyName, const char *szValue)
{
	//
	// Set the initial value of the counter.
	//
	if (!stricmp(szKeyName, "startvalue"))
	{
		Vector vec;
		UTIL_StringToVector( vec.Base(), szValue );
		m_OutValue.Init(vec);
		return(true);
	}

	return(BaseClass::KeyValue(szKeyName, szValue));
}

//-----------------------------------------------------------------------------
// Purpose: Handles key values from the BSP before spawn is called.
//-----------------------------------------------------------------------------
bool CMathVector::KeyValue( const char *szKeyName, const Vector &vecValue ) 
{
	//
	// Set the initial value of the counter.
	//
	if (!stricmp(szKeyName, "startvalue"))
	{
		m_OutValue.Init(vecValue);
		return true;
	}

	// So, CLogicalEntity descends from CBaseEntity...
	// Yup.
	// ...and CBaseEntity has a version of KeyValue that takes vectors.
	// Yup.
	// Since it's virtual, I could easily override it just like I could with a KeyValue that takes strings, right?
	// Sounds right to me.
	// So let me override it.
	// *No suitable function exists*
	return CBaseEntity::KeyValue(szKeyName, vecValue);
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for adding to the accumulator value.
// Input  : Bit value to add.
//-----------------------------------------------------------------------------
void CMathVector::InputAdd( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Vector %s ignoring ADD because it is disabled\n", GetDebugName() );
		return;
	}

	Vector vec;
	inputdata.value.Vector3D(vec);
	Vector cur;
	m_OutValue.Get(cur);
	UpdateOutValue( inputdata.pActivator, cur + vec );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for subtracting from the current value.
// Input  : Bit value to subtract.
//-----------------------------------------------------------------------------
void CMathVector::InputSubtract( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Vector %s ignoring SUBTRACT because it is disabled\n", GetDebugName() );
		return;
	}

	Vector vec;
	inputdata.value.Vector3D(vec);
	Vector cur;
	m_OutValue.Get(cur);
	UpdateOutValue( inputdata.pActivator, cur - vec );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for multiplying the current value.
// Input  : Float value to multiply the value by.
//-----------------------------------------------------------------------------
void CMathVector::InputDivide( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Vector %s ignoring DIVIDE because it is disabled\n", GetDebugName() );
		return;
	}

	Vector vec;
	inputdata.value.Vector3D(vec);
	Vector cur;
	m_OutValue.Get(cur);

	if (vec.x != 0)
		cur.x /= vec.x;
	if (vec.y != 0)
		cur.y /= vec.y;
	if (vec.z != 0)
		cur.z /= vec.z;

	UpdateOutValue( inputdata.pActivator, cur );

	//if (vec.x != 0 && vec.y != 0 && vec.z != 0)
	//{
	//	UpdateOutValue( inputdata.pActivator, cur / vec );
	//}
	//else
	//{
	//	DevMsg( 1, "LEVEL DESIGN ERROR: Divide by zero in math_vector\n" );
	//	UpdateOutValue( inputdata.pActivator, cur );
	//}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for multiplying the current value.
// Input  : Float value to multiply the value by.
//-----------------------------------------------------------------------------
void CMathVector::InputMultiply( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Vector %s ignoring MULTIPLY because it is disabled\n", GetDebugName() );
		return;
	}

	Vector vec;
	inputdata.value.Vector3D(vec);
	Vector cur;
	m_OutValue.Get(cur);
	UpdateOutValue( inputdata.pActivator, cur * vec );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for updating the value.
// Input  : Bit value to set.
//-----------------------------------------------------------------------------
void CMathVector::InputSetValue( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Vector %s ignoring SETVALUE because it is disabled\n", GetDebugName() );
		return;
	}

	Vector vec;
	inputdata.value.Vector3D(vec);
	UpdateOutValue( inputdata.pActivator, vec );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for updating the value.
// Input  : Bit value to set.
//-----------------------------------------------------------------------------
void CMathVector::InputSetValueNoFire( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Vector %s ignoring SETVALUENOFIRE because it is disabled\n", GetDebugName() );
		return;
	}

	Vector vec;
	inputdata.value.Vector3D(vec);
	m_OutValue.Init( vec );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathVector::InputGetValue( inputdata_t &inputdata )
{
	Vector cur;
	m_OutValue.Get(cur);
	m_OnGetValue.Set( cur, inputdata.pActivator, inputdata.pCaller );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathVector::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathVector::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathVector::PointAt( Vector &origin, Vector &target, Vector &out )
{
	out = origin - target;
	VectorNormalize( out );

	QAngle ang;
	VectorAngles( out, ang );

	out[0] = ang[0];
	out[1] = ang[1];
	out[2] = ang[2];
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathVector::InputPointAtLocation( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Vector %s ignoring POINTATLOCATION because it is disabled\n", GetDebugName() );
		return;
	}

	Vector cur;
	m_OutValue.Get(cur);

	Vector location;
	inputdata.value.Vector3D( location );

	PointAt( cur, location, cur );

	UpdateOutValue( inputdata.pActivator, cur );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathVector::InputPointAtEntity( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Vector %s ignoring POINTATENTITY because it is disabled\n", GetDebugName() );
		return;
	}

	if (!inputdata.value.Entity())
	{
		Warning("%s received no entity to point at\n", GetDebugName());
		return;
	}

	Vector cur;
	m_OutValue.Get(cur);

	Vector location = inputdata.value.Entity()->GetAbsOrigin();

	PointAt( cur, location, cur );

	UpdateOutValue( inputdata.pActivator, cur );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathVector::InputNormalize( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Vector %s ignoring NORMALIZE because it is disabled\n", GetDebugName() );
		return;
	}

	Vector cur;
	m_OutValue.Get(cur);
	VectorNormalize(cur);
	UpdateOutValue( inputdata.pActivator, cur );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathVector::InputNormalizeAngles( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Vector %s ignoring NORMALIZEANGLES because it is disabled\n", GetDebugName() );
		return;
	}

	Vector cur;
	m_OutValue.Get(cur);
	cur.x = AngleNormalize(cur.x);
	cur.y = AngleNormalize(cur.y);
	cur.z = AngleNormalize(cur.z);
	UpdateOutValue( inputdata.pActivator, cur );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathVector::InputVectorAngles( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Vector %s ignoring VECTORANGLES because it is disabled\n", GetDebugName() );
		return;
	}

	Vector cur;
	QAngle ang;
	m_OutValue.Get(cur);
	VectorAngles(cur, ang);
	UpdateOutValue( inputdata.pActivator, Vector(ang.x, ang.y, ang.z) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathVector::InputAngleVectorForward( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Vector %s ignoring ANGLEVECTORFORWARD because it is disabled\n", GetDebugName() );
		return;
	}

	Vector cur;
	m_OutValue.Get(cur);
	AngleVectors(QAngle(cur.x, cur.y, cur.z), &cur);
	UpdateOutValue( inputdata.pActivator, cur );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathVector::InputAngleVectorRight( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Vector %s ignoring ANGLEVECTORRIGHT because it is disabled\n", GetDebugName() );
		return;
	}

	Vector cur;
	m_OutValue.Get(cur);
	AngleVectors(QAngle(cur.x, cur.y, cur.z), NULL, &cur, NULL);
	UpdateOutValue( inputdata.pActivator, cur );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathVector::InputAngleVectorUp( inputdata_t &inputdata )
{
	if( m_bDisabled )
	{
		DevMsg("Math Vector %s ignoring ANGLEVECTORUP because it is disabled\n", GetDebugName() );
		return;
	}

	Vector cur;
	m_OutValue.Get(cur);
	AngleVectors(QAngle(cur.x, cur.y, cur.z), NULL, NULL, &cur);
	UpdateOutValue( inputdata.pActivator, cur );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathVector::SetCoordinate(float value, char coord, CBaseEntity *pActivator)
{
	if( m_bDisabled )
	{
		DevMsg("Math Vector %s ignoring SET%c because it is disabled\n", GetDebugName(), coord );
		return;
	}

	Vector vec;
	m_OutValue.Get(vec);
	switch (coord)
	{
		case 'X':	vec.x = value; break;
		case 'Y':	vec.y = value; break;
		case 'Z':	vec.z = value; break;
	}
	UpdateOutValue( pActivator, vec );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathVector::GetCoordinate(char coord, CBaseEntity *pActivator)
{
	if( m_bDisabled )
	{
		DevMsg("Math Vector %s ignoring SET%c because it is disabled\n", GetDebugName(), coord );
		return;
	}

	Vector vec;
	m_OutValue.Get(vec);
	switch (coord)
	{
		case 'X':	m_OnGetX.Set(vec.x, pActivator, this); break;
		case 'Y':	m_OnGetY.Set(vec.y, pActivator, this); break;
		case 'Z':	m_OnGetZ.Set(vec.z, pActivator, this); break;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathVector::AddCoordinate(float value, char coord, CBaseEntity *pActivator)
{
	if( m_bDisabled )
	{
		DevMsg("Math Vector %s ignoring ADD%c because it is disabled\n", GetDebugName(), coord );
		return;
	}

	Vector vec;
	m_OutValue.Get(vec);
	switch (coord)
	{
		case 'X':	vec.x += value; break;
		case 'Y':	vec.y += value; break;
		case 'Z':	vec.z += value; break;
	}
	UpdateOutValue( pActivator, vec );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathVector::SubtractCoordinate(float value, char coord, CBaseEntity *pActivator)
{
	if( m_bDisabled )
	{
		DevMsg("Math Vector %s ignoring SUBTRACT%c because it is disabled\n", GetDebugName(), coord );
		return;
	}

	Vector vec;
	m_OutValue.Get(vec);
	switch (coord)
	{
		case 'X':	vec.x += value; break;
		case 'Y':	vec.y += value; break;
		case 'Z':	vec.z += value; break;
	}
	UpdateOutValue( pActivator, vec );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the value to the new value, firing the output value.
// Input  : vecNewValue - Value to set.
//-----------------------------------------------------------------------------
void CMathVector::UpdateOutValue(CBaseEntity *pActivator, Vector vecNewValue)
{
	if (HasSpawnFlags( SF_MATH_VECTOR_DISABLE_X ))
		vecNewValue.x = 0;
	if (HasSpawnFlags( SF_MATH_VECTOR_DISABLE_Y ))
		vecNewValue.y = 0;
	if (HasSpawnFlags( SF_MATH_VECTOR_DISABLE_Z ))
		vecNewValue.z = 0;

	m_OutValue.Set(vecNewValue, pActivator, this);

	m_OutX.Set(vecNewValue.x, pActivator, this);
	m_OutY.Set(vecNewValue.y, pActivator, this);
	m_OutZ.Set(vecNewValue.z, pActivator, this);
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CMathVector::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Vector cur;
		m_OutValue.Get(cur);

		Q_snprintf(tempstr, sizeof(tempstr), "current value: [%g %g %g]", (double)cur[0], (double)cur[1], (double)cur[2]);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		if( m_bDisabled )
		{	
			Q_snprintf(tempstr,sizeof(tempstr),"*DISABLED*");		
		}
		else
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Enabled.");
		}
		EntityText(text_offset,tempstr,0);
		text_offset++;

	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Accesses/modifies any field in a datadesc based on its internal name.
//			Oh boy.
//-----------------------------------------------------------------------------
class CLogicFieldAccessor : public CLogicKeyfieldAccessor
{
	DECLARE_CLASS(CLogicFieldAccessor, CLogicKeyfieldAccessor);

private:
	bool TestKey(CBaseEntity *pTarget, const char *szKeyName);
	bool SetKeyValue(CBaseEntity *pTarget, const char *szKeyName, const char *szValue);
	bool SetKeyValueBits(CBaseEntity *pTarget, const char *szKeyName, int iValue, bool bRemove = false);

	//DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(logic_datadesc_accessor, CLogicFieldAccessor);


//BEGIN_DATADESC(CLogicFieldAccessor)

//END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CLogicFieldAccessor::TestKey(CBaseEntity *pTarget, const char *szKeyName)
{
	variant_t var;
	for ( datamap_t *dmap = pTarget->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
	{
		// search through all the readable fields in the data description, looking for a match
		for ( int i = 0; i < dmap->dataNumFields; i++ )
		{
			if ( dmap->dataDesc[i].flags & (FTYPEDESC_SAVE | FTYPEDESC_KEY) )
			{
				DevMsg("Field Name: %s,\n", dmap->dataDesc[i].fieldName);
				if ( Matcher_NamesMatch(szKeyName, dmap->dataDesc[i].fieldName) )
				{
					fieldtype_t fieldtype = dmap->dataDesc[i].fieldType;
					switch (fieldtype)
					{
						case FIELD_TIME:		fieldtype = FIELD_FLOAT; break;
						case FIELD_MODELNAME:	fieldtype = FIELD_STRING; break;
						case FIELD_SOUNDNAME:	fieldtype = FIELD_STRING; break;
						// There's definitely more of them. Add when demand becomes prevalent
					}

					var.Set( fieldtype, ((char*)pTarget) + dmap->dataDesc[i].fieldOffset[ TD_OFFSET_NORMAL ] );
					DevMsg("FIELD TYPE: %i\n", fieldtype);
					m_OutValue.Set(var, pTarget, this);
					return true;
				}
			}
		}
	}

	m_OnFailed.FireOutput(pTarget, this);
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CLogicFieldAccessor::SetKeyValue(CBaseEntity *pTarget, const char *szKeyName, const char *szValue)
{
	for ( datamap_t *dmap = pTarget->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
	{
		// search through all the readable fields in the data description, looking for a match
		for ( int i = 0; i < dmap->dataNumFields; i++ )
		{
			if ( dmap->dataDesc[i].flags & (FTYPEDESC_SAVE | FTYPEDESC_KEY) )
			{
				DevMsg("Field Name: %s,\n", dmap->dataDesc[i].fieldName);
				if ( Matcher_NamesMatch(szKeyName, dmap->dataDesc[i].fieldName) )
				{
					// Copied from ::ParseKeyvalue...
					fieldtype_t fieldtype = FIELD_VOID;
					typedescription_t *pField = &dmap->dataDesc[i];
					char *data = Datadesc_SetFieldString( szValue, pTarget, pField, &fieldtype );

					if (!data)
					{
						Warning( "%s cannot set field of type %i.\n", GetDebugName(), dmap->dataDesc[i].fieldType );
					}
					else if (fieldtype != FIELD_VOID)
					{
						variant_t var;
						var.Set(fieldtype, data);
						m_OutValue.Set(var, pTarget, this);
						return true;
					}
				}
			}
		}
	}

	m_OnFailed.FireOutput(pTarget, this);
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CLogicFieldAccessor::SetKeyValueBits(CBaseEntity *pTarget, const char *szKeyName, int iValue, bool bRemove)
{
	variant_t var;
	for ( datamap_t *dmap = pTarget->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
	{
		// search through all the readable fields in the data description, looking for a match
		for ( int i = 0; i < dmap->dataNumFields; i++ )
		{
			if ( dmap->dataDesc[i].flags & (FTYPEDESC_SAVE | FTYPEDESC_KEY) )
			{
				DevMsg("Field Name: %s,\n", dmap->dataDesc[i].fieldName);
				if ( Matcher_NamesMatch(szKeyName, dmap->dataDesc[i].fieldName) )
				{
					fieldtype_t fieldtype = dmap->dataDesc[i].fieldType;
					if (fieldtype != FIELD_INTEGER)
						break;

					var.Set( fieldtype, ((char*)pTarget) + dmap->dataDesc[i].fieldOffset[ TD_OFFSET_NORMAL ] );

					if (bRemove)
						var.SetInt(var.Int() & ~iValue);
					else
						var.SetInt(var.Int() | iValue);

					DevMsg("FIELD TYPE: %i\n", fieldtype);
					m_OutValue.Set(var, pTarget, this);
					return true;
				}
			}
		}
	}

	m_OnFailed.FireOutput(pTarget, this);
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Passes global variables, like curtime.
//-----------------------------------------------------------------------------
class CGameGlobalVars : public CLogicalEntity
{
	DECLARE_CLASS( CGameGlobalVars, CLogicalEntity );
private:

	// Inputs
	void InputGetCurtime( inputdata_t &inputdata ) { m_OutCurtime.Set(gpGlobals->curtime, inputdata.pActivator, this); }
	void InputGetFrameCount( inputdata_t &inputdata ) { m_OutFrameCount.Set(gpGlobals->framecount, inputdata.pActivator, this); }
	void InputGetFrametime( inputdata_t &inputdata ) { m_OutFrametime.Set(gpGlobals->frametime, inputdata.pActivator, this); }
	void InputGetTickCount( inputdata_t &inputdata ) { m_OutTickCount.Set(gpGlobals->tickcount, inputdata.pActivator, this); }
	void InputGetIntervalPerTick( inputdata_t &inputdata ) { m_OutIntervalPerTick.Set(gpGlobals->interval_per_tick, inputdata.pActivator, this); }

	// Outputs
	COutputFloat m_OutCurtime;
	COutputInt m_OutFrameCount;
	COutputFloat m_OutFrametime;
	COutputInt m_OutTickCount;
	COutputInt m_OutIntervalPerTick;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(game_globalvars, CGameGlobalVars);


BEGIN_DATADESC( CGameGlobalVars )

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "GetCurtime", InputGetCurtime ),
	DEFINE_INPUTFUNC( FIELD_VOID, "GetFrameCount", InputGetFrameCount ),
	DEFINE_INPUTFUNC( FIELD_VOID, "GetFrametime", InputGetFrametime ),
	DEFINE_INPUTFUNC( FIELD_VOID, "GetTickCount", InputGetTickCount ),
	DEFINE_INPUTFUNC( FIELD_VOID, "GetIntervalPerTick", InputGetIntervalPerTick ),

	// Outputs
	DEFINE_OUTPUT(m_OutCurtime, "OutCurtime"),
	DEFINE_OUTPUT(m_OutFrameCount, "OutFrameCount"),
	DEFINE_OUTPUT(m_OutFrametime, "OutFrametime"),
	DEFINE_OUTPUT(m_OutTickCount, "OutTickCount"),
	DEFINE_OUTPUT(m_OutIntervalPerTick, "OutIntervalPerTick"),

END_DATADESC()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//void CGameGlobalVars::InputGetCurtime( inputdata_t &inputdata )
//{
//	m_OutCurtime.Set(gpGlobals->curtime, inputdata.pActivator, this);
//}


#define MathModCalc(val1, val2, op) \
	switch (op) \
	{ \
		case '+':	val1 += val2; break; \
		case '-':	val1 -= val2; break; \
		case '*':	val1 *= val2; break; \
		case '/':	val1 /= val2; break; \
	} \

//-----------------------------------------------------------------------------
// Purpose: Modifies values on the fly.
//-----------------------------------------------------------------------------
class CMathMod : public CLogicalEntity
{
	DECLARE_CLASS( CMathMod, CLogicalEntity );
private:

	bool KeyValue(const char *szKeyName, const char *szValue);

	// Inputs
	void InputSetMod( inputdata_t &inputdata );
	void InputSetOperator( inputdata_t &inputdata );

	void InputModInt( inputdata_t &inputdata );
	void InputModFloat( inputdata_t &inputdata );
	void InputModVector( inputdata_t &inputdata );

	// Outputs
	COutputInt m_OutInt;
	COutputFloat m_OutFloat;
	COutputVector m_OutVector;

	int m_Operator;

	variant_t m_Mod;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(math_mod, CMathMod);


BEGIN_DATADESC( CMathMod )

	DEFINE_KEYFIELD( m_Operator, FIELD_INTEGER, "SetOperator" ),

	DEFINE_VARIANT( m_Mod ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_INPUT, "SetMod", InputSetMod ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetOperator", InputSetOperator ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "ModInt", InputModInt ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "ModFloat", InputModFloat ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "ModVector", InputModVector ),

	// Outputs
	DEFINE_OUTPUT(m_OutInt, "OutInt"),
	DEFINE_OUTPUT(m_OutFloat, "OutFloat"),
	DEFINE_OUTPUT(m_OutVector, "OutVector"),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Handles key values from the BSP before spawn is called.
//-----------------------------------------------------------------------------
bool CMathMod::KeyValue(const char *szKeyName, const char *szValue)
{
	if (!stricmp(szKeyName, "startvalue"))
	{
		// It converts later anyway
		m_Mod.SetString(AllocPooledString(szValue));
		return true;
	}

	return BaseClass::KeyValue(szKeyName, szValue);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathMod::InputSetMod( inputdata_t &inputdata )
{
	m_Mod = inputdata.value;
	//if (inputdata.value.FieldType() == FIELD_STRING)
	//	m_Mod = Variant_Parse(inputdata.value.String());
	//else
	//	m_Mod = inputdata.value;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathMod::InputSetOperator( inputdata_t &inputdata )
{
	m_Operator = inputdata.value.String()[0];
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathMod::InputModInt( inputdata_t &inputdata )
{
	m_Mod.Convert(FIELD_INTEGER);

	DevMsg("Operator is %c you see\n", m_Operator);

	int out = inputdata.value.Int();
	MathModCalc(out, m_Mod.Int(), m_Operator);

	m_OutInt.Set( out, inputdata.pActivator, this );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathMod::InputModFloat( inputdata_t &inputdata )
{
	m_Mod.Convert(FIELD_FLOAT);

	float out = inputdata.value.Float();
	MathModCalc(out, m_Mod.Float(), m_Operator);

	m_OutFloat.Set( out, inputdata.pActivator, this );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathMod::InputModVector( inputdata_t &inputdata )
{
	m_Mod.Convert(FIELD_VECTOR);

	Vector out;
	inputdata.value.Vector3D(out);
	Vector mod;
	m_Mod.Vector3D(mod);
	MathModCalc(out, mod, m_Operator);

	m_OutVector.Set( out, inputdata.pActivator, this );
}

//-----------------------------------------------------------------------------
// Purpose: Gets an entity's model information.
//-----------------------------------------------------------------------------
class CLogicModelInfo : public CLogicalEntity
{
	DECLARE_CLASS( CLogicModelInfo, CLogicalEntity );
private:

	CBaseAnimating *GetTarget(inputdata_t &inputdata);
	int GetPoseParameterIndex(CBaseAnimating *pTarget);

	// Inputs
	//void InputSetTarget( inputdata_t &inputdata ) { BaseClass::InputSetTarget(inputdata); m_hTarget = NULL; }
	void InputGetNumSkins( inputdata_t &inputdata );
	void InputLookupSequence( inputdata_t &inputdata );
	void InputLookupActivity( inputdata_t &inputdata );

	void InputSetPoseParameterName( inputdata_t &inputdata );
	void InputSetPoseParameterValue( inputdata_t &inputdata );
	void InputGetPoseParameter( inputdata_t &inputdata );

	// Outputs
	COutputInt m_OutNumSkins;
	COutputInt m_OnHasSequence;
	COutputEvent m_OnLacksSequence;

	COutputFloat m_OutPoseParameterValue;

	// KeyValues

	string_t m_iszPoseParameterName;
	int m_iPoseParameterIndex = -1;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(logic_modelinfo, CLogicModelInfo);


BEGIN_DATADESC( CLogicModelInfo )

	DEFINE_KEYFIELD( m_iszPoseParameterName, FIELD_STRING, "PoseParameterName" ),
	DEFINE_FIELD( m_iPoseParameterIndex, FIELD_INTEGER ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "GetNumSkins", InputGetNumSkins ),
	DEFINE_INPUTFUNC( FIELD_STRING, "LookupSequence", InputLookupSequence ),
	DEFINE_INPUTFUNC( FIELD_STRING, "LookupActivity", InputLookupActivity ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetPoseParameterName", InputSetPoseParameterName ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetPoseParameterValue", InputSetPoseParameterValue ),
	DEFINE_INPUTFUNC( FIELD_VOID, "GetPoseParameter", InputGetPoseParameter ),

	// Outputs
	DEFINE_OUTPUT(m_OutNumSkins, "OutNumSkins"),
	DEFINE_OUTPUT(m_OnHasSequence, "OnHasSequence"),
	DEFINE_OUTPUT(m_OnLacksSequence, "OnLacksSequence"),
	DEFINE_OUTPUT(m_OutPoseParameterValue, "OutPoseParameterValue"),

END_DATADESC()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline CBaseAnimating *CLogicModelInfo::GetTarget(inputdata_t &inputdata)
{
	CBaseEntity *pEntity = gEntList.FindEntityByName(NULL, STRING(m_target), this, inputdata.pActivator, inputdata.pCaller);
	if (!pEntity || !pEntity->GetBaseAnimating())
		return NULL;
	return pEntity->GetBaseAnimating();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int CLogicModelInfo::GetPoseParameterIndex(CBaseAnimating *pTarget)
{
	if (m_iPoseParameterIndex == -1)
		m_iPoseParameterIndex = pTarget->LookupPoseParameter(STRING(m_iszPoseParameterName));
	return m_iPoseParameterIndex;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLogicModelInfo::InputGetNumSkins( inputdata_t &inputdata )
{
	CBaseAnimating *pAnimating = GetTarget(inputdata);
	if (pAnimating && pAnimating->GetModelPtr())
	{
		m_OutNumSkins.Set(pAnimating->GetModelPtr()->numskinfamilies(), pAnimating, this);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLogicModelInfo::InputLookupSequence( inputdata_t &inputdata )
{
	CBaseAnimating *pAnimating = GetTarget(inputdata);
	if (pAnimating && pAnimating->GetModelPtr())
	{
		int index = pAnimating->LookupSequence(inputdata.value.String());

		if (index != ACT_INVALID)
			m_OnHasSequence.Set(index, pAnimating, this);
		else
			m_OnLacksSequence.FireOutput(pAnimating, this);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLogicModelInfo::InputLookupActivity( inputdata_t &inputdata )
{
	CBaseAnimating *pAnimating = GetTarget(inputdata);
	if (pAnimating && pAnimating->GetModelPtr())
	{
		int iActivity = ActivityList_IndexForName(inputdata.value.String());
		if (iActivity == -1)
		{
			// Check if it's a raw activity ID
			iActivity = atoi(inputdata.value.String());
			if (!ActivityList_NameForIndex(iActivity))
			{
				Msg("%s received invalid LookupActivity %s\n", GetDebugName(), inputdata.value.String());
				return;
			}
		}

		int index = pAnimating->SelectWeightedSequence((Activity)iActivity);

		if (index != ACT_INVALID)
			m_OnHasSequence.Set(index, pAnimating, this);
		else
			m_OnLacksSequence.FireOutput(pAnimating, this);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLogicModelInfo::InputSetPoseParameterName( inputdata_t &inputdata )
{
	m_iszPoseParameterName = inputdata.value.StringID();
	m_iPoseParameterIndex = -1;

	CBaseAnimating *pAnimating = GetTarget(inputdata);
	if (pAnimating && pAnimating->GetModelPtr())
	{
		if (GetPoseParameterIndex(pAnimating) == -1)
			Warning("%s: Pose parameter \"%s\" does not exist on %s\n", GetDebugName(), inputdata.value.String(), STRING(pAnimating->GetModelName()));
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLogicModelInfo::InputSetPoseParameterValue( inputdata_t &inputdata )
{
	CBaseAnimating *pAnimating = GetTarget(inputdata);
	if (pAnimating && pAnimating->GetModelPtr())
	{
		int index = GetPoseParameterIndex(pAnimating);
		if (index != -1)
		{
			pAnimating->SetPoseParameter( index, inputdata.value.Float() );
		}
		else
			Warning("%s: Pose parameter \"%s\" does not exist on %s\n", GetDebugName(), STRING(m_iszPoseParameterName), STRING(pAnimating->GetModelName()));
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLogicModelInfo::InputGetPoseParameter( inputdata_t &inputdata )
{
	CBaseAnimating *pAnimating = GetTarget(inputdata);
	if (pAnimating && pAnimating->GetModelPtr())
	{
		int index = GetPoseParameterIndex(pAnimating);
		if (index != -1)
		{
			m_OutPoseParameterValue.Set( pAnimating->GetPoseParameter( index ), pAnimating, this );
		}
		else
			Warning("%s: Pose parameter \"%s\" does not exist on %s\n", GetDebugName(), inputdata.value.String(), STRING(pAnimating->GetModelName()));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks and calculates an entity's position.
//-----------------------------------------------------------------------------
class CLogicEntityPosition : public CLogicalEntity
{
	DECLARE_CLASS( CLogicEntityPosition, CLogicalEntity );
private:
	EHANDLE m_hTarget;

	int m_iPositionType;
	enum
	{
		POSITION_ORIGIN = 0,
		POSITION_LOCAL,
		POSITION_BBOX,
		POSITION_EYES,
		POSITION_EARS,
		POSITION_ATTACHMENT,
	};

	// Something that accompanies the position type, like an attachment name.
	string_t m_iszPositionParameter;

	CBaseEntity *GetTarget(CBaseEntity *pActivator, CBaseEntity *pCaller);

	Vector GetPosition(CBaseEntity *pEntity);
	QAngle GetAngles(CBaseEntity *pEntity);

	// Inputs
	void InputGetPosition( inputdata_t &inputdata );
	void InputSetPosition( inputdata_t &inputdata );
	void InputPredictPosition( inputdata_t &inputdata );

	void InputSetTarget( inputdata_t &inputdata ) { BaseClass::InputSetTarget(inputdata); m_hTarget = NULL; }

	// Outputs
	COutputVector m_OutPosition;
	COutputVector m_OutAngles;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(logic_entity_position, CLogicEntityPosition);

BEGIN_DATADESC( CLogicEntityPosition )

	// Keys
	DEFINE_FIELD( m_hTarget, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_iPositionType, FIELD_INTEGER, "PositionType" ),
	DEFINE_KEYFIELD( m_iszPositionParameter, FIELD_STRING, "PositionParameter" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "GetPosition", InputGetPosition ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetPosition", InputSetPosition ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "PredictPosition", InputPredictPosition ),

	// Outputs
	DEFINE_OUTPUT(m_OutPosition, "OutPosition"),
	DEFINE_OUTPUT(m_OutAngles, "OutAngles"),

END_DATADESC()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline CBaseEntity *CLogicEntityPosition::GetTarget(CBaseEntity *pActivator, CBaseEntity *pCaller)
{
	// Always reset with procedurals
	if (!m_hTarget || STRING(m_target)[0] == '!')
		m_hTarget = gEntList.FindEntityByName(NULL, STRING(m_target), this, pActivator, pCaller);
	return m_hTarget.Get();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CLogicEntityPosition::GetPosition(CBaseEntity *pEntity)
{
	switch (m_iPositionType)
	{
		case POSITION_ORIGIN:			return pEntity->GetAbsOrigin();
		case POSITION_LOCAL:			return pEntity->GetLocalOrigin();
		case POSITION_BBOX:				return pEntity->WorldSpaceCenter();
		case POSITION_EYES:				return pEntity->EyePosition();
		case POSITION_EARS:				return pEntity->EarPosition();
		case POSITION_ATTACHMENT:
		{
			CBaseAnimating *pAnimating = pEntity->GetBaseAnimating();
			if (!pAnimating)
			{
				Warning("%s wants to measure one of %s's attachments, but %s doesn't support them!\n", GetDebugName(), pEntity->GetDebugName(), pEntity->GetDebugName());
				break;
			}

			Vector vecPosition;
			pAnimating->GetAttachment(STRING(m_iszPositionParameter), vecPosition);
			return vecPosition;
		}
	}

	return vec3_origin;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
QAngle CLogicEntityPosition::GetAngles(CBaseEntity *pEntity)
{
	switch (m_iPositionType)
	{
	case POSITION_BBOX:
		case POSITION_EARS:
		case POSITION_ORIGIN:			return pEntity->GetAbsAngles(); break;
		case POSITION_LOCAL:			return pEntity->GetLocalAngles(); break;
		case POSITION_EYES:				return pEntity->EyeAngles(); break;
		case POSITION_ATTACHMENT:
		{
			CBaseAnimating *pAnimating = pEntity->GetBaseAnimating();
			if (!pAnimating)
			{
				Warning("%s wants to measure one of %s's attachments, but %s doesn't support them!\n", GetDebugName(), pEntity->GetDebugName(), pEntity->GetDebugName());
				break;
			}

			QAngle AttachmentAngles;
			matrix3x4_t attachmentToWorld;
			pAnimating->GetAttachment( pAnimating->LookupAttachment( STRING( m_iszPositionParameter ) ), attachmentToWorld );
			MatrixAngles( attachmentToWorld, AttachmentAngles );
			return AttachmentAngles;
		} break;
	}

	return vec3_angle;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLogicEntityPosition::InputGetPosition( inputdata_t &inputdata )
{
	CBaseEntity *pEntity = GetTarget(inputdata.pActivator, inputdata.pCaller);
	if (!pEntity)
	{
		m_OutPosition.Set( vec3_origin, NULL, this );
		m_OutAngles.Set( vec3_angle, NULL, this );
		return;
	}

	m_OutPosition.Set( GetPosition(pEntity), pEntity, this );
	m_OutAngles.Set( GetAngles(pEntity), pEntity, this );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLogicEntityPosition::InputSetPosition( inputdata_t &inputdata )
{
	CBaseEntity *pEntity = GetTarget(inputdata.pActivator, inputdata.pCaller);
	if (!pEntity)
	{
		Warning("%s can't find entity %s for SetPosition!\n", GetDebugName(), STRING(m_target));
		return;
	}

	Vector vec;
	inputdata.value.Vector3D(vec);

	// If the position is local, they might want to move local origin instead
	if (m_iPositionType == POSITION_LOCAL)
		pEntity->SetLocalOrigin(vec);
	else
		pEntity->SetAbsOrigin(vec);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLogicEntityPosition::InputPredictPosition( inputdata_t &inputdata )
{
	CBaseEntity *pEntity = GetTarget(inputdata.pActivator, inputdata.pCaller);
	if (!pEntity)
	{
		m_OutPosition.Set( vec3_origin, NULL, this );
		m_OutAngles.Set( vec3_angle, NULL, this );
		return;
	}

	Vector vecPosition;
	UTIL_PredictedPosition(pEntity, GetPosition(pEntity), inputdata.value.Float(), &vecPosition);

	QAngle angAngles;
	UTIL_PredictedAngles(pEntity, GetAngles(pEntity), inputdata.value.Float(), &angAngles);

	m_OutPosition.Set( vecPosition, pEntity, this );
	m_OutAngles.Set( angAngles, pEntity, this );
}

//-----------------------------------------------------------------------------
// Purpose: Accesses context values
//-----------------------------------------------------------------------------
class CLogicContextAccessor : public CLogicalEntity
{
	DECLARE_CLASS(CLogicContextAccessor, CLogicalEntity);

public:
	CBaseEntity *GetTarget(CBaseEntity *pCaller, CBaseEntity *pActivator);

	bool TestContext(CBaseEntity *pTarget, const char *szKeyName);
	void SetContext(CBaseEntity *pTarget, const char *szKeyName, string_t szValue);

	// Inputs
	void InputTest(inputdata_t &inputdata);
	void InputTestContext(inputdata_t &inputdata);
	void InputTestTarget(inputdata_t &inputdata);

	void InputSetContext(inputdata_t &inputdata);

	void InputSetValue(inputdata_t &inputdata);

	//bool ReadUnregisteredKeyfields(CBaseEntity *pTarget, const char *szKeyName, variant_t *variant);

	COutputString m_OutValue;
	COutputEvent m_OnFailed;

	string_t m_iszContext;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(logic_context_accessor, CLogicContextAccessor);


BEGIN_DATADESC(CLogicContextAccessor)

DEFINE_KEYFIELD( m_iszContext, FIELD_STRING, "context" ),

// Inputs
DEFINE_INPUTFUNC(FIELD_VOID, "Test", InputTest),
DEFINE_INPUTFUNC(FIELD_STRING, "TestContext", InputTestContext),
DEFINE_INPUTFUNC(FIELD_STRING, "TestTarget", InputTestTarget),
DEFINE_INPUTFUNC(FIELD_STRING, "SetContext", InputSetContext),
DEFINE_INPUTFUNC(FIELD_STRING, "SetValue", InputSetValue),

DEFINE_OUTPUT( m_OutValue, "OutValue" ),
DEFINE_OUTPUT( m_OnFailed, "OnFailed" ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CBaseEntity *CLogicContextAccessor::GetTarget(CBaseEntity *pCaller, CBaseEntity *pActivator)
{
	return gEntList.FindEntityByName(NULL, m_target, this, pActivator, pCaller);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CLogicContextAccessor::TestContext(CBaseEntity *pTarget, const char *szKeyName)
{
	int idx = pTarget->FindContextByName( szKeyName );
	if ( idx != -1 )
	{
		m_OutValue.Set(FindPooledString(pTarget->GetContextValue(idx)), pTarget, this);
		return true;
	}
	else
	{
		m_OnFailed.FireOutput(pTarget, this);
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicContextAccessor::SetContext(CBaseEntity *pTarget, const char *szKeyName, string_t szValue)
{
	pTarget->AddContext(szKeyName, STRING(szValue));

	m_OutValue.Set(szValue, pTarget, this);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicContextAccessor::InputTest(inputdata_t &inputdata)
{
	CBaseEntity *pTarget = GetTarget(inputdata.pCaller, inputdata.pActivator);
	if (pTarget && m_iszContext != NULL_STRING)
	{
		TestContext(pTarget, STRING(m_iszContext));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicContextAccessor::InputTestContext(inputdata_t &inputdata)
{
	const char *input = inputdata.value.String();
	CBaseEntity *pTarget = GetTarget(inputdata.pCaller, inputdata.pActivator);
	if (input && pTarget)
	{
		TestContext(pTarget, input);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicContextAccessor::InputTestTarget(inputdata_t &inputdata)
{
	m_target = inputdata.value.StringID();
	CBaseEntity *pTarget = gEntList.FindEntityByName(NULL, inputdata.value.StringID(), this, inputdata.pCaller, inputdata.pActivator);
	if (pTarget && m_iszContext != NULL_STRING)
	{
		TestContext(pTarget, STRING(m_iszContext));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicContextAccessor::InputSetContext(inputdata_t &inputdata)
{
	m_iszContext = inputdata.value.StringID();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicContextAccessor::InputSetValue(inputdata_t &inputdata)
{
	CBaseEntity *pTarget = GetTarget(inputdata.pCaller, inputdata.pActivator);
	if (pTarget)
	{
		SetContext(pTarget, STRING(m_iszContext), inputdata.value.StringID());
	}
}

//-----------------------------------------------------------------------------
// Purpose: Replicates light pattern functionality.
//-----------------------------------------------------------------------------
class CMathLightPattern : public CLogicalEntity
{
	DECLARE_CLASS( CMathLightPattern, CLogicalEntity );
private:


	string_t m_iszPattern;

	bool m_bDisabled;

	void Spawn();
	bool KeyValue( const char *szKeyName, const char *szValue );

	void OutputCurPattern();

	void StartPatternThink();
	void PatternThink();
	unsigned char m_NextLetter = 0;

	// How fast the pattern should be
	float m_flPatternSpeed = 0.1f;

	inline bool VerifyPatternValid() { return (m_iszPattern != NULL_STRING && STRING( m_iszPattern )[0] != '\0'); }

	// Inputs
	void InputSetStyle( inputdata_t &inputdata );
	void InputSetPattern( inputdata_t &inputdata );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	// Outputs
	COutputFloat m_OutValue;
	COutputString m_OutLetter;
	COutputEvent m_OnLightOn;
	COutputEvent m_OnLightOff;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( math_lightpattern, CMathLightPattern );

BEGIN_DATADESC( CMathLightPattern )

	// Keys
	DEFINE_KEYFIELD(m_iszPattern, FIELD_STRING, "pattern"),
	DEFINE_KEYFIELD(m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_KEYFIELD(m_flPatternSpeed, FIELD_FLOAT, "PatternSpeed"),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetStyle", InputSetStyle ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetPattern", InputSetPattern ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

	// Outputs
	DEFINE_OUTPUT(m_OutValue, "OutValue"),
	DEFINE_OUTPUT(m_OutLetter, "OutLetter"),
	DEFINE_OUTPUT(m_OnLightOn, "OnLightOn"),
	DEFINE_OUTPUT(m_OnLightOff, "OnLightOff"),

	DEFINE_THINKFUNC( PatternThink ),
	DEFINE_FIELD( m_NextLetter, FIELD_CHARACTER ),

END_DATADESC()

extern const char *GetDefaultLightstyleString( int styleIndex );

static const char *s_pLightPatternContext = "PatternContext";

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMathLightPattern::Spawn()
{
	BaseClass::Spawn();

	if (!m_bDisabled && VerifyPatternValid())
		StartPatternThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMathLightPattern::OutputCurPattern()
{
	// This code looks messy, but it does what it's supposed to and is safe enough.
	// First, we get the next letter in the pattern sequence.
	// Next, we calculate its integral proximity to the character 'a' (fully dark)
	// and calculate its approximate brightness by dividing it by the number of letters in the alphabet other than a.
	// We output that brightness value for things like projected textures and other custom intensity values
	// so they could replicate the patterns of their corresponding vrad lights.
	char cLetter = STRING(m_iszPattern)[m_NextLetter];
	int iValue = (cLetter - 'a');
	float flResult = iValue != 0 ? ((float)iValue / 25.0f) : 0.0f;
	m_OutValue.Set(flResult, this, this);

	// User-friendly "Light on, light off" outputs
	if (flResult > 0)
		m_OnLightOn.FireOutput(this, this);
	else
		m_OnLightOff.FireOutput(this, this);

	// Create a string with cLetter and a null terminator.
	char szLetter[2] = { cLetter, '\0' };
	m_OutLetter.Set( AllocPooledString(szLetter), this, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMathLightPattern::StartPatternThink()
{
	// Output our current/next one immediately.
	OutputCurPattern();

	// Start thinking now.
	SetContextThink( &CMathLightPattern::PatternThink, gpGlobals->curtime, s_pLightPatternContext );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMathLightPattern::PatternThink()
{
	// Output our current/next one
	OutputCurPattern();

	// Increment
	m_NextLetter++;
	if (STRING(m_iszPattern)[m_NextLetter] == '\0')
		m_NextLetter = 0;

	//m_OutLetter.Set(AllocPooledString(UTIL_VarArgs("%c", m_NextLetter)), this, this);

	SetNextThink( gpGlobals->curtime + m_flPatternSpeed, s_pLightPatternContext );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMathLightPattern::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "style" ) )
	{
		m_iszPattern = AllocPooledString(GetDefaultLightstyleString(atoi(szValue)));
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMathLightPattern::InputSetStyle( inputdata_t &inputdata )
{
	m_iszPattern = AllocPooledString(GetDefaultLightstyleString(inputdata.value.Int()));
	m_NextLetter = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMathLightPattern::InputSetPattern( inputdata_t &inputdata )
{
	m_iszPattern = inputdata.value.StringID();
	m_NextLetter = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMathLightPattern::InputEnable( inputdata_t &inputdata )
{
	if (VerifyPatternValid())
		StartPatternThink();
	else
		Warning("%s tried to enable without valid pattern\n", GetDebugName());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMathLightPattern::InputDisable( inputdata_t &inputdata )
{
	SetContextThink( NULL, TICK_NEVER_THINK, s_pLightPatternContext );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMathLightPattern::InputToggle( inputdata_t &inputdata )
{
	if (GetNextThink(s_pLightPatternContext) != TICK_NEVER_THINK)
		InputDisable(inputdata);
	else
		InputEnable(inputdata);
}

//-----------------------------------------------------------------------------
// Purpose: Sequences for keypads, etc.
//-----------------------------------------------------------------------------
#define MAX_SEQUENCE_CASES 16

class CLogicSequence : public CLogicalEntity
{
	DECLARE_CLASS( CLogicSequence, CLogicalEntity );
public:
	CLogicSequence();

	void Activate();

	bool KeyValue( const char *szKeyName, const char *szValue );

	void TestCase( int iCase, string_t iszValue, CBaseEntity *pActivator );
	void SequenceComplete( string_t iszValue, CBaseEntity *pActivator );

private:
	string_t	m_iszCase[MAX_SEQUENCE_CASES];
	int			m_iNumCases;

	bool m_bDisabled;

	bool m_bDontIncrementOnPass;

	// Inputs
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );
	void InputInValue( inputdata_t &inputdata );
	void InputSetCurrentCase( inputdata_t &inputdata );
	void InputSetCurrentCaseNoFire( inputdata_t &inputdata );
	void InputIncrementSequence( inputdata_t &inputdata );
	void InputResetSequence( inputdata_t &inputdata );

	// Outputs
	COutputInt		m_CurCase;
	COutputString	m_OnCasePass;
	COutputString	m_OnCaseFail;
	COutputString	m_OnSequenceComplete;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( logic_sequence, CLogicSequence );


BEGIN_DATADESC( CLogicSequence )

	// Keys
	DEFINE_KEYFIELD( m_iszCase[0], FIELD_STRING, "Case01" ),
	DEFINE_KEYFIELD( m_iszCase[1], FIELD_STRING, "Case02" ),
	DEFINE_KEYFIELD( m_iszCase[2], FIELD_STRING, "Case03" ),
	DEFINE_KEYFIELD( m_iszCase[3], FIELD_STRING, "Case04" ),
	DEFINE_KEYFIELD( m_iszCase[4], FIELD_STRING, "Case05" ),
	DEFINE_KEYFIELD( m_iszCase[5], FIELD_STRING, "Case06" ),
	DEFINE_KEYFIELD( m_iszCase[6], FIELD_STRING, "Case07" ),
	DEFINE_KEYFIELD( m_iszCase[7], FIELD_STRING, "Case08" ),
	DEFINE_KEYFIELD( m_iszCase[8], FIELD_STRING, "Case09" ),
	DEFINE_KEYFIELD( m_iszCase[9], FIELD_STRING, "Case10" ),
	DEFINE_KEYFIELD( m_iszCase[10], FIELD_STRING, "Case11" ),
	DEFINE_KEYFIELD( m_iszCase[11], FIELD_STRING, "Case12" ),
	DEFINE_KEYFIELD( m_iszCase[12], FIELD_STRING, "Case13" ),
	DEFINE_KEYFIELD( m_iszCase[13], FIELD_STRING, "Case14" ),
	DEFINE_KEYFIELD( m_iszCase[14], FIELD_STRING, "Case15" ),
	DEFINE_KEYFIELD( m_iszCase[15], FIELD_STRING, "Case16" ),

	// This doesn't need to be saved, it can be assigned every Activate()
	//DEFINE_FIELD( m_iNumCases, FIELD_INTEGER ),

	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),

	DEFINE_KEYFIELD( m_bDontIncrementOnPass, FIELD_BOOLEAN, "DontIncrementOnPass" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_STRING, "InValue", InputInValue ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetCurrentCase", InputSetCurrentCase ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetCurrentCaseNoFire", InputSetCurrentCaseNoFire ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "IncrementSequence", InputIncrementSequence ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ResetSequence", InputResetSequence ),

	// Outputs
	DEFINE_OUTPUT( m_CurCase, "OutCurCase" ),
	DEFINE_OUTPUT( m_OnCasePass, "OnCasePass" ),
	DEFINE_OUTPUT( m_OnCaseFail, "OnCaseFail" ),
	DEFINE_OUTPUT( m_OnSequenceComplete, "OnSequenceComplete" ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CLogicSequence::CLogicSequence()
{
	m_CurCase.Init( 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicSequence::Activate( void )
{
	BaseClass::Activate();

	// Count number of cases
	for (m_iNumCases = 0; m_iNumCases < MAX_SEQUENCE_CASES; m_iNumCases++)
	{
		if (m_iszCase[m_iNumCases] == NULL_STRING)
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Cache user entity field values until spawn is called.
// Input  : szKeyName - Key to handle.
//			szValue - Value for key.
// Output : Returns true if the key was handled, false if not.
//-----------------------------------------------------------------------------
bool CLogicSequence::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq( szKeyName, "StartCase" ))
	{
		m_CurCase.Init( atoi(szValue) );
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicSequence::TestCase( int iCase, string_t iszValue, CBaseEntity *pActivator )
{
	if (m_bDisabled)
	{
		DevMsg("%s ignoring case test because it is disabled\n", GetDebugName());
		return;
	}

	// Arrays are 0-based, so the index is (iCase - 1)
	int iIndex = iCase - 1;
	if (iIndex >= m_iNumCases)
	{
		DevMsg("%s ignoring case test because the current case %i is greater than or equal to the number of cases %i\n", GetDebugName(), iCase, m_iNumCases);
		return;
	}

	if (Matcher_Match( STRING( m_iszCase[iIndex] ), STRING(iszValue) ))
	{
		m_OnCasePass.Set( iszValue, pActivator, this );

		if (!m_bDontIncrementOnPass)
		{
			m_CurCase.Set(iCase + 1, pActivator, this);

			if (m_CurCase.Get() > m_iNumCases)
			{
				// Sequence complete!
				SequenceComplete(iszValue, pActivator);
			}
		}
		else
		{
			m_CurCase.Set(iCase, pActivator, this);
		}
	}
	else
	{
		m_OnCaseFail.Set( iszValue, pActivator, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicSequence::SequenceComplete( string_t iszValue, CBaseEntity *pActivator )
{
	m_OnSequenceComplete.Set( iszValue, pActivator, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicSequence::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicSequence::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicSequence::InputToggle( inputdata_t &inputdata )
{
	m_bDisabled = (m_bDisabled == false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicSequence::InputInValue( inputdata_t &inputdata )
{
	TestCase( m_CurCase.Get(), inputdata.value.StringID(), inputdata.pActivator );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicSequence::InputSetCurrentCase( inputdata_t &inputdata )
{
	m_CurCase.Set( inputdata.value.Int(), inputdata.pActivator, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicSequence::InputSetCurrentCaseNoFire( inputdata_t &inputdata )
{
	m_CurCase.Init( inputdata.value.Int() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicSequence::InputIncrementSequence( inputdata_t &inputdata )
{
	int iInc = inputdata.value.Int();
	m_CurCase.Set( m_CurCase.Get() + (iInc != 0 ? iInc : 1), inputdata.pActivator, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicSequence::InputResetSequence( inputdata_t &inputdata )
{
	m_CurCase.Set( 1, inputdata.pActivator, this );
}

//-----------------------------------------------------------------------------
// Purpose: Generates various types of numbers based on existing material proxies
//-----------------------------------------------------------------------------
class CMathGenerate : public CLogicalEntity
{
public:
	DECLARE_CLASS( CMathGenerate, CLogicalEntity );
	CMathGenerate();

	enum GenerateType_t
	{
		GENERATE_SINE_WAVE,
		GENERATE_LINEAR_RAMP,
		GENERATE_UNIFORM_NOISE,
		GENERATE_GAUSSIAN_NOISE,
		GENERATE_EXPONENTIAL,
	};

	// Keys
	float m_flMax;
	float m_flMin;

	float m_flParam1;
	float m_flParam2;

	bool m_bDisabled;

	GenerateType_t m_iGenerateType;

	// Inputs
	void InputSetValue( inputdata_t &inputdata );
	void InputSetValueNoFire( inputdata_t &inputdata );
	void InputGetValue( inputdata_t &inputdata );
	void InputSetGenerateType( inputdata_t &inputdata );

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	void UpdateOutValue( float fNewValue, CBaseEntity *pActivator = NULL );
	void UpdateOutValueSine( float fNewValue, CBaseEntity *pActivator = NULL );

	// Basic functions
	void Spawn();
	bool KeyValue( const char *szKeyName, const char *szValue );

	void StartGenerating();
	void StopGenerating();

	// Number generation functions
	void GenerateSineWave();
	void GenerateLinearRamp();
	void GenerateUniformNoise();
	void GenerateGaussianNoise();
	void GenerateExponential();

	// The gaussian stream normally only exists on the client, so we use our own.
	static CGaussianRandomStream m_GaussianStream;

	bool m_bHitMin;		// Set when we reach or go below our minimum value, cleared if we go above it again.
	bool m_bHitMax;		// Set when we reach or exceed our maximum value, cleared if we fall below it again.

	// Outputs
	COutputFloat m_OutValue;
	COutputFloat m_OnGetValue;	// Used for polling the counter value.
	COutputEvent m_OnHitMin;
	COutputEvent m_OnHitMax;
	COutputEvent m_OnChangedFromMin;
	COutputEvent m_OnChangedFromMax;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( math_generate, CMathGenerate );


BEGIN_DATADESC( CMathGenerate )

	DEFINE_INPUT( m_flMax, FIELD_FLOAT, "SetHitMax" ),
	DEFINE_INPUT( m_flMin, FIELD_FLOAT, "SetHitMin" ),
	DEFINE_INPUT( m_flParam1, FIELD_FLOAT, "SetParam1" ),
	DEFINE_INPUT( m_flParam2, FIELD_FLOAT, "SetParam2" ),
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),

	DEFINE_KEYFIELD( m_iGenerateType, FIELD_INTEGER, "GenerateType" ),

	DEFINE_FIELD( m_bHitMax, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bHitMin, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetValue", InputSetValue ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetValueNoFire", InputSetValueNoFire ),
	DEFINE_INPUTFUNC( FIELD_VOID, "GetValue", InputGetValue ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetGenerateType", InputSetGenerateType ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

	DEFINE_OUTPUT( m_OutValue, "OutValue" ),
	DEFINE_OUTPUT( m_OnHitMin, "OnHitMin" ),
	DEFINE_OUTPUT( m_OnHitMax, "OnHitMax" ),
	DEFINE_OUTPUT( m_OnGetValue, "OnGetValue" ),
	DEFINE_OUTPUT( m_OnChangedFromMin, "OnChangedFromMin" ),
	DEFINE_OUTPUT( m_OnChangedFromMax, "OnChangedFromMax" ),

	DEFINE_THINKFUNC( GenerateSineWave ),
	DEFINE_THINKFUNC( GenerateLinearRamp ),
	DEFINE_THINKFUNC( GenerateUniformNoise ),
	DEFINE_THINKFUNC( GenerateGaussianNoise ),
	DEFINE_THINKFUNC( GenerateExponential ),

END_DATADESC()

CGaussianRandomStream CMathGenerate::m_GaussianStream;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMathGenerate::CMathGenerate()
{
	m_GaussianStream.AttachToStream( random );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMathGenerate::Spawn()
{
	BaseClass::Spawn();

	if (!m_bDisabled)
		StartGenerating();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMathGenerate::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq( szKeyName, "InitialValue" ))
	{
		m_OutValue.Init( atof(szValue) );
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathGenerate::InputSetValue( inputdata_t &inputdata )
{
	UpdateOutValue(inputdata.value.Float(), inputdata.pActivator);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathGenerate::InputSetValueNoFire( inputdata_t &inputdata )
{
	m_OutValue.Init(inputdata.value.Float());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathGenerate::InputGetValue( inputdata_t &inputdata )
{
	float flOutValue = m_OutValue.Get();
	m_OnGetValue.Set( flOutValue, inputdata.pActivator, inputdata.pCaller );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathGenerate::InputSetGenerateType( inputdata_t &inputdata )
{
	m_iGenerateType = (GenerateType_t)inputdata.value.Int();

	if (GetNextThink() != TICK_NEVER_THINK)
	{
		// Change our generation function if we're already generating.
		// StartGenerating() should set to the new function.
		StartGenerating();
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathGenerate::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
	StartGenerating();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathGenerate::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
	StopGenerating();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathGenerate::InputToggle( inputdata_t &inputdata )
{
	m_bDisabled ? InputEnable(inputdata) : InputDisable(inputdata);
}

//-----------------------------------------------------------------------------
// Purpose: Sets the value to the new value, clamping and firing the output value.
// Input  : fNewValue - Value to set.
//-----------------------------------------------------------------------------
void CMathGenerate::UpdateOutValue( float fNewValue, CBaseEntity *pActivator )
{
	if ((m_flMin != 0) || (m_flMax != 0))
	{
		//
		// Fire an output any time we reach or exceed our maximum value.
		//
		if ( fNewValue >= m_flMax || (m_iGenerateType == GENERATE_SINE_WAVE && fNewValue >= (m_flMax * 0.995f)) )
		{
			if ( !m_bHitMax )
			{
				m_bHitMax = true;
				m_OnHitMax.FireOutput( pActivator, this );
			}
		}
		else
		{
			// Fire an output if we just changed from the maximum value
			if ( m_OutValue.Get() == m_flMax )
			{
				m_OnChangedFromMax.FireOutput( pActivator, this );
			}

			m_bHitMax = false;
		}

		//
		// Fire an output any time we reach or go below our minimum value.
		//
		if ( fNewValue <= m_flMin )
		{
			if ( !m_bHitMin )
			{
				m_bHitMin = true;
				m_OnHitMin.FireOutput( pActivator, this );
			}
		}
		else
		{
			// Fire an output if we just changed from the maximum value
			if ( m_OutValue.Get() == m_flMin )
			{
				m_OnChangedFromMin.FireOutput( pActivator, this );
			}

			m_bHitMin = false;
		}

		fNewValue = clamp(fNewValue, m_flMin, m_flMax);
	}

	m_OutValue.Set(fNewValue, pActivator, this);
}

//-----------------------------------------------------------------------------
// Purpose: Sets the value to the new value, clamping and firing the output value.
//			Sine generation needs to use a different function to account for skips and imprecision.
// Input  : fNewValue - Value to set.
//-----------------------------------------------------------------------------
void CMathGenerate::UpdateOutValueSine( float fNewValue, CBaseEntity *pActivator )
{
	if ((m_flMin != 0) || (m_flMax != 0))
	{
		//
		// Fire an output any time we reach or exceed our maximum value.
		//
		if ( fNewValue >= (m_flMax * 0.995f) )
		{
			if ( !m_bHitMax )
			{
				m_bHitMax = true;
				m_OnHitMax.FireOutput( pActivator, this );
			}
		}
		else
		{
			// Fire an output if we just changed from the maximum value
			if ( m_bHitMax )
			{
				m_OnChangedFromMax.FireOutput( pActivator, this );
			}

			m_bHitMax = false;
		}

		//
		// Fire an output any time we reach or go below our minimum value.
		//
		if ( fNewValue <= (m_flMin * 1.005f) )
		{
			if ( !m_bHitMin )
			{
				m_bHitMin = true;
				m_OnHitMin.FireOutput( pActivator, this );
			}
		}
		else
		{
			// Fire an output if we just changed from the maximum value
			if ( m_bHitMin )
			{
				m_OnChangedFromMin.FireOutput( pActivator, this );
			}

			m_bHitMin = false;
		}

		//fNewValue = clamp(fNewValue, m_flMin, m_flMax);
	}

	m_OutValue.Set(fNewValue, pActivator, this);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathGenerate::StartGenerating()
{
	// Correct any min/max quirks here
	if (m_flMin > m_flMax)
	{
		float flTemp = m_flMin;
		m_flMin = m_flMax;
		m_flMax = flTemp;
	}

	switch (m_iGenerateType)
	{
		case GENERATE_SINE_WAVE:
			SetThink( &CMathGenerate::GenerateSineWave );
			break;
		case GENERATE_LINEAR_RAMP:
			SetThink( &CMathGenerate::GenerateLinearRamp );
			break;
		case GENERATE_UNIFORM_NOISE:
			SetThink( &CMathGenerate::GenerateUniformNoise );
			break;
		case GENERATE_GAUSSIAN_NOISE:
			SetThink( &CMathGenerate::GenerateGaussianNoise );
			break;
		case GENERATE_EXPONENTIAL:
			SetThink( &CMathGenerate::GenerateExponential );
			break;

		default:
			Warning("%s is set to invalid generation type %i! It won't do anything now.\n", GetDebugName(), m_iGenerateType);
			StopGenerating();
			return;
	}

	// All valid types should fall through to this
	SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathGenerate::StopGenerating()
{
	SetThink(NULL);
	SetNextThink( TICK_NEVER_THINK );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathGenerate::GenerateSineWave()
{
	// CSineProxy in mathproxy.cpp
	float flSineTimeOffset = m_flParam2;
	float flSinePeriod = m_flParam1;
	float flValue;

	if (flSinePeriod == 0)
		flSinePeriod = 1;

	// get a value in [0,1]
	flValue = ( sin( 2.0f * M_PI * (gpGlobals->curtime - flSineTimeOffset) / flSinePeriod ) * 0.5f ) + 0.5f;
	// get a value in [min,max]	
	flValue = ( m_flMax - m_flMin ) * flValue + m_flMin;
	
	UpdateOutValueSine( flValue );

	SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathGenerate::GenerateLinearRamp()
{
	// CLinearRampProxy in mathproxy.cpp

	// Param1 = rate
	float flVal = m_flParam1 * gpGlobals->curtime + m_OutValue.Get();

	// clamp
	if (flVal < m_flMin)
		flVal = m_flMin;
	else if (flVal > m_flMax)
		flVal = m_flMax;

	UpdateOutValue( flVal );

	SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathGenerate::GenerateUniformNoise()
{
	// CUniformNoiseProxy in mathproxy.cpp

	UpdateOutValue( random->RandomFloat( m_flMin, m_flMax ) );

	SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathGenerate::GenerateGaussianNoise()
{
	// CGaussianNoiseProxy in mathproxy.cpp

	float flMean = m_flParam1;
	float flStdDev = m_flParam2;
	float flVal = m_GaussianStream.RandomFloat( flMean, flStdDev );

	// clamp
	if (flVal < m_flMin)
		flVal = m_flMin;
	else if (flVal > m_flMax)
		flVal = m_flMax;

	UpdateOutValue( flVal );

	SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMathGenerate::GenerateExponential()
{
	// CExponentialProxy in mathproxy.cpp

	// Param1 = scale
	// Param2 = offset
	float flVal = m_flParam1 * exp( m_OutValue.Get() + m_flParam2 );

	// clamp
	if (flVal < m_flMin)
		flVal = m_flMin;
	else if (flVal > m_flMax)
		flVal = m_flMax;

	UpdateOutValue( flVal );

	SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
}
#endif
