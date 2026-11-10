//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements populator interface entity.  Used for map
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_population_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf_populator_debug;


class CPointPopulatorInterface : public CPointEntity
{
	DECLARE_CLASS( CPointPopulatorInterface, CPointEntity );
	DECLARE_ENT_SCRIPTDESC();

public:

	// Input handlers
	void InputPauseBotSpawning(inputdata_t &inputdata);
	void InputUnpauseBotSpawning(inputdata_t &inputdata);
	void InputChangeBotAttributes(inputdata_t &inputdata);
	void InputChangeDefaultEventAttributes(inputdata_t &inputdata);

	// Vscript handlers
	int ScriptGetWaveNumber(void) { return g_pPopulationManager->GetWaveNumber(); }
	int ScriptGetWaveTotal(void) { return g_pPopulationManager->GetTotalWaveCount(); }
	void ScriptStartWave(void) { g_pPopulationManager->StartCurrentWave(); }
	void ScriptJumpToWave(int waveNumber, float fCleanMoneyPercent = -1.0f) { g_pPopulationManager->JumpToWave((uint32)waveNumber, fCleanMoneyPercent); }
	void ScriptForceEndWave(bool bSuccess) { g_pPopulationManager->ScriptForceWaveEnd(bSuccess); }
	void PauseBotSpawn(void) { g_pPopulationManager->PauseSpawning(); }
	void UnPauseBotSpawn(void) { g_pPopulationManager->UnpauseSpawning(); }
	bool BotSpawnState(void) { return g_pPopulationManager->IsSpawningPaused(); }
	void ChangeBotAttributes(const char* pszEventName);
	void ChangeDefaultEventAttributes(const char* pszEventName);
	
	DECLARE_DATADESC();
};

BEGIN_DATADESC( CPointPopulatorInterface )

// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "PauseBotSpawning", InputPauseBotSpawning ),
	DEFINE_INPUTFUNC( FIELD_VOID, "UnpauseBotSpawning", InputUnpauseBotSpawning ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ChangeBotAttributes", InputChangeBotAttributes ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ChangeDefaultEventAttributes", InputChangeDefaultEventAttributes ),

END_DATADESC()

	BEGIN_ENT_SCRIPTDESC_ROOT( CPointPopulatorInterface, "Interface for MvM populator" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetWaveNumber, "GetWaveNumber", "Get Current Wave number" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetWaveTotal, "GetTotalWaveCount", "Get Total Wave in current mission" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptStartWave, "ForceWaveStart", "Forcibly Start current wave" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptJumpToWave, "JumpToWave", "Arugments - (waveNum, CurrencyPercent); Set the current Wave" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptForceEndWave, "ForceWaveEnd", "Arugments - (victory); Force finishes the current Wave" )
	DEFINE_SCRIPTFUNC_NAMED( PauseBotSpawn, "PauseSpawn", "Pause Bot spawnwaves" )
	DEFINE_SCRIPTFUNC_NAMED( UnPauseBotSpawn, "UnPauseSpawn", "Resume Bot spawnwaves" )
	DEFINE_SCRIPTFUNC_NAMED( BotSpawnState, "IsSpawnPause", "Are spawnwaves paused" )
	DEFINE_SCRIPTFUNC( ChangeBotAttributes, "Arugments - (attribute); ChangeBotAttributes input as a vscript function" )
	DEFINE_SCRIPTFUNC( ChangeDefaultEventAttributes, "Arugments - (attribute); ChangeDefaultEventAttributes input as a vscript function" )
END_SCRIPTDESC();

LINK_ENTITY_TO_CLASS( point_populator_interface, CPointPopulatorInterface );


void CPointPopulatorInterface::InputPauseBotSpawning( inputdata_t &inputdata )
{
	Assert( g_pPopulationManager );
	if( g_pPopulationManager )
	{
		g_pPopulationManager->PauseSpawning();
	}
}

void CPointPopulatorInterface::InputUnpauseBotSpawning( inputdata_t &inputdata )
{
	Assert( g_pPopulationManager );
	if( g_pPopulationManager )
	{
		g_pPopulationManager->UnpauseSpawning();
	}
}

void CPointPopulatorInterface::InputChangeBotAttributes( inputdata_t &inputdata )
{
	const char* pszEventName = inputdata.value.String();

	ChangeBotAttributes(pszEventName);
}

void CPointPopulatorInterface::InputChangeDefaultEventAttributes(inputdata_t &inputdata)
{
	const char* pszEventName = inputdata.value.String();

	ChangeDefaultEventAttributes(pszEventName);
}

//The following are the above but replaced the input text for direct string
void CPointPopulatorInterface::ChangeBotAttributes(const char* pszEventName)
{
	if ( tf_populator_debug.GetBool() && g_pPopulationManager && !g_pPopulationManager->HasEventChangeAttributes(pszEventName) )
	{
		Warning("ChangeBotAttributes: Failed to find event [%s] in the pop file\n", pszEventName);
		return;
	}

	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		CUtlVector< CTFBot* > botVector;
		CollectPlayers(&botVector, TF_TEAM_PVE_INVADERS, COLLECT_ONLY_LIVING_PLAYERS);

		for ( int i = 0; i < botVector.Count(); ++i )
		{
			const CTFBot::EventChangeAttributes_t* pEvent = botVector[i]->GetEventChangeAttributes(pszEventName);
			if ( pEvent )
			{
				botVector[i]->OnEventChangeAttributes(pEvent);
			}
		}
	}
}

void CPointPopulatorInterface::ChangeDefaultEventAttributes(const char* pszEventName)
{
	if ( tf_populator_debug.GetBool() && g_pPopulationManager && !g_pPopulationManager->HasEventChangeAttributes(pszEventName) )
	{
		Warning("ChangeBotAttributes: Failed to find event [%s] in the pop file\n", pszEventName);
		return;
	}

	if ( g_pPopulationManager )
	{
		g_pPopulationManager->SetDefaultEventChangeAttributesName(pszEventName);
	}
}