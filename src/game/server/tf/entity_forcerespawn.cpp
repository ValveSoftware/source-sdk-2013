//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf/tf_shareddefs.h"
#include "entity_forcerespawn.h"
#include "tf_player.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// CTFReset tables.
//
BEGIN_DATADESC( CTFForceRespawn )

// Inputs.
DEFINE_INPUTFUNC( FIELD_VOID, "ForceRespawn", InputForceRespawn ),
DEFINE_INPUTFUNC( FIELD_VOID, "ForceRespawnSwitchTeams", InputForceRespawnSwitchTeams ),
DEFINE_INPUTFUNC( FIELD_INTEGER, "ForceTeamRespawn", InputForceTeamRespawn ),

// Outputs.
DEFINE_OUTPUT( m_outputOnForceRespawn, "OnForceRespawn" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( game_forcerespawn, CTFForceRespawn );

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFForceRespawn::CTFForceRespawn()
{

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFForceRespawn::ForceRespawn( bool bSwitchTeams, int nTeam /* = TEAM_UNASSIGNED */, bool bRemoveEverything /* = true */ )
{
	int i = 0;

	if ( bRemoveEverything && TFGameRules() )
	{
		TFGameRules()->RemoveAllProjectilesAndBuildings();
	}

	// respawn the players
	for ( i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			// Ignore players who aren't on an active team
			if ( pPlayer->GetTeamNumber() != TF_TEAM_RED && pPlayer->GetTeamNumber() != TF_TEAM_BLUE )
			{
				// Let the player spawn immediately when they do pick a class
				pPlayer->AllowInstantSpawn();
				continue;
			}

			if ( bSwitchTeams )
			{
				if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
				{
					pPlayer->ForceChangeTeam( TF_TEAM_BLUE, true );
				}
				else if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
				{
					pPlayer->ForceChangeTeam( TF_TEAM_RED, true );
				}
			}

			// Ignore players who haven't picked a class yet
			if ( !pPlayer->GetPlayerClass() || pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED )
			{
				// Allow them to spawn instantly when they do choose
				pPlayer->AllowInstantSpawn();
				continue;
			}

			if ( nTeam != TEAM_UNASSIGNED )
			{
				// Ignore players who aren't on the team we're trying to respawn
				if ( pPlayer->GetTeamNumber() != nTeam )
				{
					continue;
				}
				else
				{
					// Ignore players on the team that aren't dead
					if ( pPlayer->IsAlive() )
						continue;
				}
			}
			
			pPlayer->ForceRespawn();
		}
	}

	// remove any dropped weapons/ammo packs	
	CBaseEntity *pEnt = NULL;
	while ( (pEnt = gEntList.FindEntityByClassname( pEnt, "tf_ammo_pack" )) != NULL )
	{
		UTIL_Remove( pEnt );
	}

	// Output.
	m_outputOnForceRespawn.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFForceRespawn::InputForceRespawn( inputdata_t &inputdata )
{
	ForceRespawn( false );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFForceRespawn::InputForceRespawnSwitchTeams( inputdata_t &inputdata )
{
	ForceRespawn( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFForceRespawn::InputForceTeamRespawn( inputdata_t &inputdata )
{
	int nTeam = inputdata.value.Int();
	ForceRespawn( false, nTeam, false );
}

