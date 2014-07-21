//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Fires an output when the map spawns (or respawns if not set to 
//			only fire once). It can be set to check a global state before firing.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "entityinput.h"
#include "entityoutput.h"
#include "eventqueue.h"
#include "mathlib/mathlib.h"
#include "globalstate.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const int SF_AUTO_FIREONCE		= 0x01;
const int SF_AUTO_FIREONRELOAD	= 0x02;


class CLogicAuto : public CBaseEntity
{
public:
	DECLARE_CLASS( CLogicAuto, CBaseEntity );

	void Activate(void);
	void Think(void);

	int ObjectCaps(void) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	DECLARE_DATADESC();

private:

	// fired no matter why the map loaded
	COutputEvent m_OnMapSpawn;

	// fired for specified types of map loads
	COutputEvent m_OnNewGame;
	COutputEvent m_OnLoadGame;
	COutputEvent m_OnMapTransition;
	COutputEvent m_OnBackgroundMap;
	COutputEvent m_OnMultiNewMap;
	COutputEvent m_OnMultiNewRound;

	string_t m_globalstate;
};

LINK_ENTITY_TO_CLASS(logic_auto, CLogicAuto);


BEGIN_DATADESC( CLogicAuto )

	DEFINE_KEYFIELD(m_globalstate, FIELD_STRING, "globalstate"),

	// Outputs
	DEFINE_OUTPUT(m_OnMapSpawn, "OnMapSpawn"),
	DEFINE_OUTPUT(m_OnNewGame, "OnNewGame"),
	DEFINE_OUTPUT(m_OnLoadGame, "OnLoadGame"),
	DEFINE_OUTPUT(m_OnMapTransition, "OnMapTransition"),
	DEFINE_OUTPUT(m_OnBackgroundMap, "OnBackgroundMap"),
	DEFINE_OUTPUT(m_OnMultiNewMap, "OnMultiNewMap" ),
	DEFINE_OUTPUT(m_OnMultiNewRound, "OnMultiNewRound" ),

END_DATADESC()


//------------------------------------------------------------------------------
// Purpose : Fire my outputs here if I fire on map reload
//------------------------------------------------------------------------------
void CLogicAuto::Activate(void)
{
	BaseClass::Activate();
	SetNextThink( gpGlobals->curtime + 0.2 );
}


//-----------------------------------------------------------------------------
// Purpose: Called shortly after level spawn. Checks the global state and fires
//			targets if the global state is set or if there is not global state
//			to check.
//-----------------------------------------------------------------------------
void CLogicAuto::Think(void)
{
	if (!m_globalstate || GlobalEntity_GetState(m_globalstate) == GLOBAL_ON)
	{
		if (gpGlobals->eLoadType == MapLoad_Transition)
		{
			m_OnMapTransition.FireOutput(NULL, this);
		}
		else if (gpGlobals->eLoadType == MapLoad_NewGame)
		{
			m_OnNewGame.FireOutput(NULL, this);
		}
		else if (gpGlobals->eLoadType == MapLoad_LoadGame)
		{
			m_OnLoadGame.FireOutput(NULL, this);
		}
		else if (gpGlobals->eLoadType == MapLoad_Background)
		{
			m_OnBackgroundMap.FireOutput(NULL, this);
		}

		m_OnMapSpawn.FireOutput(NULL, this);

		if ( g_pGameRules->IsMultiplayer() )
		{
			// In multiplayer, fire the new map / round events.
			if ( g_pGameRules->InRoundRestart() )
			{
				m_OnMultiNewRound.FireOutput(NULL, this);
			}
			else
			{
				m_OnMultiNewMap.FireOutput(NULL, this);
			}
		}

		if (m_spawnflags & SF_AUTO_FIREONCE)
		{
			UTIL_Remove(this);
		}
	}
}

