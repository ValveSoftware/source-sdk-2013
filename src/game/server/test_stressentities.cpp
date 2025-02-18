//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "test_stressentities.h"
#include "vstdlib/random.h"
#include "world.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CStressEntityReg	*CStressEntityReg::s_pHead = NULL;


// CStressEntityReg::s_pHead in array form for convenient access.
CUtlVector<CStressEntityReg*> g_StressEntityRegs;

CUtlVector<EHANDLE> g_StressEntities;


CBaseEntity* MoveToRandomSpot( CBaseEntity *pEnt )
{
	if ( pEnt )
	{
		CBasePlayer *pLocalPlayer = UTIL_GetLocalPlayer();
		if ( pLocalPlayer )
		{			
			Vector vForward;
			pLocalPlayer->EyeVectors(&vForward );

			UTIL_SetOrigin( pEnt, GetRandomSpot() );
		}
	}

	return pEnt;
}


Vector GetRandomSpot()
{
	CWorld *pEnt = GetWorldEntity();
	if ( pEnt )
	{
		Vector vMin, vMax;
		pEnt->GetWorldBounds( vMin, vMax );
		return Vector(
			RandomFloat( vMin.x, vMax.x ),
			RandomFloat( vMin.y, vMax.y ),
			RandomFloat( vMin.z, vMax.z ) );
	}
	else
	{
		return Vector( 0, 0, 0 );
	}
}


void Test_InitRandomEntitySpawner( const CCommand &args )
{
	// Put the list of registered functions into array form for convenience.
	g_StressEntityRegs.Purge();
	for ( CStressEntityReg *pCur=CStressEntityReg::GetListHead(); pCur; pCur=pCur->GetNext() )
		g_StressEntityRegs.AddToTail( pCur );

	// Create slots for all the entities..
	int nSlots = 100;
	if ( args.ArgC() >= 2 )
		nSlots = atoi( args[ 1 ] );

	g_StressEntities.Purge();
	g_StressEntities.SetSize( nSlots );

	Msg( "Test_InitRandomEntitySpawner: created %d slots.\n", nSlots );
}


void Test_SpawnRandomEntities( const CCommand &args )
{
	if ( args.ArgC() < 3 )
	{
		Error( "Test_SpawnRandomEntities <min # entities> <max # entities> missing arguments." );
	}

	if ( g_StressEntities.Count() == 0 )
	{
		Error( "Test_SpawnRandomEntities: not initialized (call Test_InitRandomEntitySpawner frst)." );
	} 

	int nMin = atoi( args[ 1 ] );
	int nMax = atoi( args[ 2 ] );
	int count = RandomInt( nMin, nMax );

	for ( int i=0; i < count; i++ )
	{
		int iSlot = RandomInt( 0, g_StressEntities.Count() - 1 );

		// Remove any old entity in this slot.
		if ( g_StressEntities[iSlot].Get() )
			UTIL_RemoveImmediate( g_StressEntities[iSlot] );

		// Create a new one in this slot.
		int iType = RandomInt( 0, g_StressEntityRegs.Count() - 1 );
		g_StressEntities[iSlot] = g_StressEntityRegs[iType]->GetFn()();
	}
}


void Test_RandomizeInPVS( const CCommand &args )
{
	if ( args.ArgC() < 2 )
	{
		Error( "Test_RandomizeInPVS <percentage chance to change>" );
	}

	int percent = atoi( args[ 1 ] );
	for ( int i=0; i < g_StressEntities.Count(); i++ )
	{
		CBaseEntity *pEnt = g_StressEntities[i];

		if ( pEnt )
		{
			if ( RandomInt( 0, 100 ) < percent )
			{
				if ( pEnt->IsEffectActive( EF_NODRAW ) )
					pEnt->RemoveEffects( EF_NODRAW );
				else
					pEnt->AddEffects( EF_NODRAW );
			}
		}
	}
}


void Test_RemoveAllRandomEntities()
{
	for ( int i=0; i < g_StressEntities.Count(); i++ )
	{
		if ( g_StressEntities[i].Get() )
			UTIL_Remove( g_StressEntities[i] );
	}
}


ConCommand cc_Test_InitRandomEntitySpawner( "Test_InitRandomEntitySpawner", Test_InitRandomEntitySpawner, 0, FCVAR_CHEAT );
ConCommand cc_Test_SpawnRandomEntities( "Test_SpawnRandomEntities", Test_SpawnRandomEntities, 0, FCVAR_CHEAT );
ConCommand cc_Test_RandomizeInPVS( "Test_RandomizeInPVS", Test_RandomizeInPVS, 0, FCVAR_CHEAT );
ConCommand cc_Test_RemoveAllRandomEntities( "Test_RemoveAllRandomEntities", Test_RemoveAllRandomEntities, 0, FCVAR_CHEAT );

