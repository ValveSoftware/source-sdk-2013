//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: AI system that makes NPCs verbally respond to game events
//
//=============================================================================

#include "cbase.h"
#include "ai_eventresponse.h"
#include "ai_basenpc.h"

ConVar	ai_debug_eventresponses( "ai_debug_eventresponses", "0", FCVAR_NONE, "Set to 1 to see all NPC response events trigger, and which NPCs choose to respond to them." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNPCEventResponseSystem g_NPCEventResponseSystem( "CNPCEventResponseSystem" );

CNPCEventResponseSystem *NPCEventResponse()
{
	return &g_NPCEventResponseSystem;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCEventResponseSystem::LevelInitPreEntity( void )
{
	m_ActiveEvents.Purge();
	m_flNextEventPoll = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCEventResponseSystem::TriggerEvent( const char *pResponse, bool bForce, bool bCancelScript )
{
	m_flNextEventPoll = gpGlobals->curtime;

	// Find the event by name
	int iIndex = m_ActiveEvents.Find( pResponse );
	if ( iIndex == m_ActiveEvents.InvalidIndex() )
	{
		storedevent_t newEvent;
		newEvent.flEventTime = gpGlobals->curtime;
		newEvent.flNextResponseTime = 0;
		newEvent.bForce = bForce;
		newEvent.bCancelScript = bCancelScript;
		newEvent.bPreventExpiration = false;
		m_ActiveEvents.Insert( pResponse, newEvent );

		if ( ai_debug_eventresponses.GetBool() ) 
		{
			Msg( "NPCEVENTRESPONSE: (%.2f) Trigger fired for event named: %s\n", gpGlobals->curtime, pResponse );
		}
	}
	else 
	{
		// Update the trigger time
		m_ActiveEvents[iIndex].flEventTime = gpGlobals->curtime;
		m_ActiveEvents[iIndex].bForce = bForce;
		m_ActiveEvents[iIndex].bCancelScript = bCancelScript;

		if ( ai_debug_eventresponses.GetBool() ) 
		{
			Msg( "NPCEVENTRESPONSE: (%.2f) Trigger resetting already-active event firing named: %s\n", gpGlobals->curtime, pResponse );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCEventResponseSystem::FrameUpdatePreEntityThink()
{
 	if ( !m_ActiveEvents.Count() || !AI_IsSinglePlayer() || !UTIL_GetLocalPlayer() )
		return;

	if ( m_flNextEventPoll > gpGlobals->curtime )
		return;
	m_flNextEventPoll = gpGlobals->curtime + 0.2;

	// Move through all events, removing expired ones and finding NPCs for active ones.
	for ( int i = m_ActiveEvents.First(); i != m_ActiveEvents.InvalidIndex(); )
	{	
 		float flTime = m_ActiveEvents[i].flEventTime;
		const char *pResponse = m_ActiveEvents.GetElementName(i);

		// Save off the next index so we can safely remove this one
		int iNext = m_ActiveEvents.Next(i); 

		// Should it have expired by now?
		if ( !m_ActiveEvents[i].bPreventExpiration && (flTime + NPCEVENTRESPONSE_GIVEUP_TIME) < gpGlobals->curtime )
		{
			if ( ai_debug_eventresponses.GetBool() ) 
			{
				Msg( "NPCEVENTRESPONSE: (%.2f) Removing expired event named: %s\n", gpGlobals->curtime, pResponse );
			}

			m_ActiveEvents.RemoveAt(i);
		}
		else if ( m_ActiveEvents[i].flNextResponseTime < gpGlobals->curtime )
		{
			// If we've fired once, and our current event should expire now, then expire.
			if ( m_ActiveEvents[i].bPreventExpiration && (flTime + NPCEVENTRESPONSE_GIVEUP_TIME) < gpGlobals->curtime )
			{
				if ( ai_debug_eventresponses.GetBool() ) 
				{
					Msg( "NPCEVENTRESPONSE: (%.2f) Removing expired fired event named: %s\n", gpGlobals->curtime, pResponse );
				}

				m_ActiveEvents.RemoveAt(i);
			}
			else
			{
				float flNearestDist = NPCEVENTRESPONSE_DISTANCE_SQR;
				CAI_BaseNPC *pNearestNPC = NULL;
				Vector vecPlayerCenter = UTIL_GetLocalPlayer()->WorldSpaceCenter();

				// Try and find the nearest NPC to the player
				CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
				for ( int j = 0; j < g_AI_Manager.NumAIs(); j++ )
				{
					if ( ppAIs[j]->CanRespondToEvent( pResponse ))
					{
						float flDistToPlayer = ( vecPlayerCenter - ppAIs[j]->WorldSpaceCenter()).LengthSqr();
						if ( flDistToPlayer < flNearestDist )
						{
							flNearestDist = flDistToPlayer;
							pNearestNPC = ppAIs[j];
						}
					}
				}

				// Found one? 
				if ( pNearestNPC )
				{
					if ( pNearestNPC->RespondedTo( pResponse, m_ActiveEvents[i].bForce, m_ActiveEvents[i].bCancelScript ) )
					{
						// Don't remove the response yet. Leave it around until the refire time has expired.
						// This stops repeated firings of the same concept from spamming the NPCs.
						m_ActiveEvents[i].bPreventExpiration = true;
						m_ActiveEvents[i].flNextResponseTime = gpGlobals->curtime + NPCEVENTRESPONSE_REFIRE_TIME;

						if ( ai_debug_eventresponses.GetBool() ) 
						{
							Msg( "NPCEVENTRESPONSE: (%.2f) Event '%s' responded to by NPC '%s'. Refire available at: %.2f\n", gpGlobals->curtime, pResponse, pNearestNPC->GetDebugName(), m_ActiveEvents[i].flNextResponseTime );
						}

						// Don't issue multiple responses at once
						return;
					}
				}
			}
		}

		i = iNext;
	}
}

//---------------------------------------------------------------------------------------------
// Entity version for mapmaker to hook into the system
//---------------------------------------------------------------------------------------------
class CNPCEventResponseSystemEntity : public CBaseEntity
{
	DECLARE_CLASS( CNPCEventResponseSystemEntity, CBaseEntity );
public:
	DECLARE_DATADESC();

	void	Spawn();
	void 	InputTriggerResponseEvent( inputdata_t &inputdata );
	void 	InputForceTriggerResponseEvent( inputdata_t &inputdata );
	void 	InputForceTriggerResponseEventNoCancel( inputdata_t &inputdata );
};

LINK_ENTITY_TO_CLASS( ai_npc_eventresponsesystem, CNPCEventResponseSystemEntity );

BEGIN_DATADESC( CNPCEventResponseSystemEntity )
	DEFINE_INPUTFUNC( FIELD_STRING,	"TriggerResponseEvent",	InputTriggerResponseEvent ),
	DEFINE_INPUTFUNC( FIELD_STRING,	"ForceTriggerResponseEvent", InputForceTriggerResponseEvent ),
	DEFINE_INPUTFUNC( FIELD_STRING,	"ForceTriggerResponseEventNoCancel", InputForceTriggerResponseEventNoCancel ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCEventResponseSystemEntity::Spawn( void )
{
	// Invisible, non solid.
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCEventResponseSystemEntity::InputTriggerResponseEvent( inputdata_t &inputdata )
{
	NPCEventResponse()->TriggerEvent( inputdata.value.String(), false, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCEventResponseSystemEntity::InputForceTriggerResponseEvent( inputdata_t &inputdata )
{
	NPCEventResponse()->TriggerEvent( inputdata.value.String(), true, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCEventResponseSystemEntity::InputForceTriggerResponseEventNoCancel( inputdata_t &inputdata )
{
	NPCEventResponse()->TriggerEvent( inputdata.value.String(), true, false );
}
