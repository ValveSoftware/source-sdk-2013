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

public:

	// Input handlers
	void InputPauseBotSpawning(inputdata_t &inputdata);
	void InputUnpauseBotSpawning(inputdata_t &inputdata);
	void InputChangeBotAttributes(inputdata_t &inputdata);
	void InputChangeDefaultEventAttributes(inputdata_t &inputdata);
	
	DECLARE_DATADESC();
};

BEGIN_DATADESC( CPointPopulatorInterface )

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "PauseBotSpawning", InputPauseBotSpawning ),
	DEFINE_INPUTFUNC( FIELD_VOID, "UnpauseBotSpawning", InputUnpauseBotSpawning ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ChangeBotAttributes", InputChangeBotAttributes ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ChangeDefaultEventAttributes", InputChangeDefaultEventAttributes ),

END_DATADESC()

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

	if ( tf_populator_debug.GetBool() && g_pPopulationManager && !g_pPopulationManager->HasEventChangeAttributes( pszEventName ) )
	{
		Warning( "ChangeBotAttributes: Failed to find event [%s] in the pop file\n", pszEventName );
		return;
	}

	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		CUtlVector< CTFBot* > botVector;
		CollectPlayers( &botVector, TF_TEAM_PVE_INVADERS, COLLECT_ONLY_LIVING_PLAYERS );

		for ( int i=0; i<botVector.Count(); ++i )
		{
			const CTFBot::EventChangeAttributes_t* pEvent = botVector[i]->GetEventChangeAttributes( pszEventName );
			if ( pEvent )
			{
				botVector[i]->OnEventChangeAttributes( pEvent );
			}
		}
	}
}

void CPointPopulatorInterface::InputChangeDefaultEventAttributes(inputdata_t &inputdata)
{
	const char* pszEventName = inputdata.value.String();

	if ( tf_populator_debug.GetBool() && g_pPopulationManager && !g_pPopulationManager->HasEventChangeAttributes( pszEventName ) )
	{
		Warning( "ChangeBotAttributes: Failed to find event [%s] in the pop file\n", pszEventName );
		return;
	}

	if ( g_pPopulationManager )
	{
		g_pPopulationManager->SetDefaultEventChangeAttributesName( pszEventName );
	}	
}
