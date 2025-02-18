//========= Copyright Valve Corporation, All rights reserved. ============//
//
// A message forwarder. Fires an OnTrigger output when triggered, and can be
// disabled to prevent forwarding outputs.
//
// Useful as an intermediary between one entity and another for turning on or
// off an I/O connection, or as a container for holding a set of outputs that
// can be triggered from multiple places.
//
//=============================================================================

#include "cbase.h"
#include "entityinput.h"
#include "entityoutput.h"
#include "eventqueue.h"
#include "soundent.h"
#include "logicrelay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const int SF_REMOVE_ON_FIRE				= 0x001;	// Relay will remove itself after being triggered.
const int SF_ALLOW_FAST_RETRIGGER		= 0x002;	// Unless set, relay will disable itself until the last output is sent.

LINK_ENTITY_TO_CLASS(logic_relay, CLogicRelay);


BEGIN_DATADESC( CLogicRelay )

	DEFINE_FIELD(m_bWaitForRefire, FIELD_BOOLEAN),
	DEFINE_KEYFIELD(m_bDisabled, FIELD_BOOLEAN, "StartDisabled"),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
	DEFINE_INPUTFUNC(FIELD_VOID, "EnableRefire", InputEnableRefire),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),
	DEFINE_INPUTFUNC(FIELD_VOID, "Trigger", InputTrigger),
	DEFINE_INPUTFUNC(FIELD_VOID, "CancelPending", InputCancelPending),

	// Outputs
	DEFINE_OUTPUT(m_OnTrigger, "OnTrigger"),
	DEFINE_OUTPUT(m_OnSpawn, "OnSpawn"),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CLogicRelay::CLogicRelay(void)
{
}


//------------------------------------------------------------------------------
// Kickstarts a think if we have OnSpawn connections.
//------------------------------------------------------------------------------
void CLogicRelay::Activate()
{
	BaseClass::Activate();
	
	if ( m_OnSpawn.NumberOfElements() > 0)
	{
		SetNextThink( gpGlobals->curtime + 0.01 );
	}
}


//-----------------------------------------------------------------------------
// If we have OnSpawn connections, this is called shortly after spawning to
// fire the OnSpawn output.
//-----------------------------------------------------------------------------
void CLogicRelay::Think()
{
	// Fire an output when we spawn. This is used for self-starting an entity
	// template -- since the logic_relay is inside the template, it gets all the
	// name and I/O connection fixup, so can target other entities in the template.
	m_OnSpawn.FireOutput( this, this );

	// We only get here if we had OnSpawn connections, so this is safe.
	if ( m_spawnflags & SF_REMOVE_ON_FIRE )
	{
		UTIL_Remove(this);
	}
}


//------------------------------------------------------------------------------
// Purpose: Turns on the relay, allowing it to fire outputs.
//------------------------------------------------------------------------------
void CLogicRelay::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}

//------------------------------------------------------------------------------
// Purpose: Enables us to fire again. This input is only posted from our Trigger
//			function to prevent rapid refire.
//------------------------------------------------------------------------------
void CLogicRelay::InputEnableRefire( inputdata_t &inputdata )
{ 
	m_bWaitForRefire = false;
}


//------------------------------------------------------------------------------
// Purpose: Cancels any I/O events in the queue that were fired by us.
//------------------------------------------------------------------------------
void CLogicRelay::InputCancelPending( inputdata_t &inputdata )
{ 
	g_EventQueue.CancelEvents( this );

	// Stop waiting; allow another Trigger.
	m_bWaitForRefire = false;
}


//------------------------------------------------------------------------------
// Purpose: Turns off the relay, preventing it from firing outputs.
//------------------------------------------------------------------------------
void CLogicRelay::InputDisable( inputdata_t &inputdata )
{ 
	m_bDisabled = true;
}


//------------------------------------------------------------------------------
// Purpose: Toggles the enabled/disabled state of the relay.
//------------------------------------------------------------------------------
void CLogicRelay::InputToggle( inputdata_t &inputdata )
{ 
	m_bDisabled = !m_bDisabled;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that triggers the relay.
//-----------------------------------------------------------------------------
void CLogicRelay::InputTrigger( inputdata_t &inputdata )
{
	if ((!m_bDisabled) && (!m_bWaitForRefire))
	{
		m_OnTrigger.FireOutput( inputdata.pActivator, this );
		
		if (m_spawnflags & SF_REMOVE_ON_FIRE)
		{
			UTIL_Remove(this);
		}
		else if (!(m_spawnflags & SF_ALLOW_FAST_RETRIGGER))
		{
			//
			// Disable the relay so that it cannot be refired until after the last output
			// has been fired and post an input to re-enable ourselves.
			//
			m_bWaitForRefire = true;
			g_EventQueue.AddEvent(this, "EnableRefire", m_OnTrigger.GetMaxDelay() + 0.001, this, this);
		}
	}
}

