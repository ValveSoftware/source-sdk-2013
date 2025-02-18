//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "serverbenchmark_tf.h"
#include "tf_shareddefs.h"
#include "tf_bot_temp.h"
#include "entity_tfstart.h"
#include "tf_player.h"


static ConVar sv_benchmark_freeroam( "sv_benchmark_freeroam", "0", 0, "Allow the local player to move freely in the benchmark. Only used for debugging. Don't use for real benchmarks because it will make the timing inconsistent." );


class CTFServerBenchmark : public CServerBenchmarkHook
{
public:
	virtual void StartBenchmark()
	{
		ConVarRef cvBotFlipout( "bot_flipout" );
		cvBotFlipout.SetValue( 3 );

		extern ConVar bot_forceattack_down;
		bot_forceattack_down.SetValue( true );

		extern ConVar mp_teams_unbalance_limit;
		mp_teams_unbalance_limit.SetValue( (int)0 );

		m_nBotsCreated = 0;
		m_bSetupLocalPlayer = false;
	}

	virtual void GetPhysicsModelNames( CUtlVector<char*> &modelNames )
	{
		modelNames.AddToTail( "models/props_farm/wooden_barrel.mdl" );
		modelNames.AddToTail( "models/props_gameplay/orange_cone001.mdl" );
	}

	virtual CBaseEntity* GetBlueSpawnPoint( bool bBest )
	{
		CUtlVector<CTFTeamSpawn*> spawns;
		for ( int i=0; i<ITFTeamSpawnAutoList::AutoList().Count(); ++i )
		{
			CTFTeamSpawn *pTFSpawn = static_cast< CTFTeamSpawn* >( ITFTeamSpawnAutoList::AutoList()[i] );
			if ( !pTFSpawn->IsDisabled() && pTFSpawn->GetTeamNumber() == TF_TEAM_BLUE )
				spawns.AddToTail( pTFSpawn );
		}

		// If we're not looking for the BEST one, return a random one.
		if ( !bBest )
		{
			if ( spawns.Count() == 0 )
				return NULL;

			return spawns[ g_pServerBenchmark->RandomInt( 0, spawns.Count()-1 ) ];
		}

		float flBestSpawn = 0;
		CTFTeamSpawn *pBestSpawn = NULL;
		for ( int i=0; i < spawns.Count(); i++ )
		{
			Vector vForward;
			AngleVectors( spawns[i]->GetLocalAngles(), &vForward );

			float flCurSpawn = 0;
			for ( int j=0; j < spawns.Count(); j++ )
			{
				if ( j == i )
					continue;
			
				Vector vTo = spawns[j]->GetAbsOrigin() - spawns[i]->GetAbsOrigin();
				VectorNormalize( vTo );

				flCurSpawn += vForward.Dot( vTo );
			}

			if ( flCurSpawn > flBestSpawn )
			{
				flBestSpawn = flCurSpawn;
				pBestSpawn = spawns[i];
			}
		}

		return pBestSpawn;
	}

	virtual void UpdateBenchmark()
	{
		if ( m_nBotsCreated == 0 )
		{
			return;
		}

		RandomSeed( 0 );
	
		// Put the player at a blue spawn point.
		if ( !engine->IsDedicatedServer() )
		{
			CTFPlayer *pLocalPlayer = dynamic_cast< CTFPlayer* >( UTIL_GetListenServerHost() );
			if ( pLocalPlayer )
			{
				if ( !m_bSetupLocalPlayer )
				{
					m_bSetupLocalPlayer = true;

					pLocalPlayer->ChangeTeam( TEAM_SPECTATOR );
					pLocalPlayer->SetDesiredPlayerClassIndex( TF_CLASS_SNIPER );
					pLocalPlayer->ForceRespawn();

					// Now pick whatever blue spawn point has the most other spawn points in front of it.
					CBaseEntity *pBestSpawn = GetBlueSpawnPoint( true );
					if ( !pBestSpawn )
						Error( "Can't find spawn position for local player." );
				
					m_vLocalPlayerOrigin = pBestSpawn->GetLocalOrigin() + Vector(0,0,80);
					m_vLocalPlayerEyeAngles = pBestSpawn->GetLocalAngles();
				}

				((CBasePlayer*)pLocalPlayer)->SetObserverMode( OBS_MODE_ROAMING );
				
				// Lock the player in place for a little bit, then let them go free when we have some bots.
				if ( !sv_benchmark_freeroam.GetBool() || m_nBotsCreated < 2 )
				{
					pLocalPlayer->Teleport( &m_vLocalPlayerOrigin, &m_vLocalPlayerEyeAngles, NULL );
					pLocalPlayer->SetObserverTarget( NULL );
					
					if ( !sv_benchmark_freeroam.GetBool() )
						pLocalPlayer->AddFlag( FL_FROZEN );
				}
			}
		}

		RespawnDeadPlayers();
		MoveRedPlayersToBlueArea();
		AddSentries();
	}

	void RespawnDeadPlayers()
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( pPlayer && pPlayer->IsDead() && !g_pServerBenchmark->IsLocalBenchmarkPlayer( pPlayer ) )
			{
				pPlayer->ForceRespawn();
			}
		}
	}

	void AddSentries()
	{
		const char *pSentryClassName = "obj_sentrygun";
		
		for ( int iTeamIteration=0; iTeamIteration < 2; iTeamIteration++ )
		{
			int iTeam = (iTeamIteration == 0) ? TF_TEAM_BLUE : TF_TEAM_RED;

			// Get the current # of sentries.
			int nSentries = 0;
			CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, pSentryClassName );
			while( pSpot )
			{
				if ( pSpot->GetTeamNumber() == iTeam )
					++nSentries;

				pSpot = gEntList.FindEntityByClassname( pSpot, pSentryClassName );
			}

			// Make new ones if necessary.
			if ( nSentries < 2 )
			{
				// Find an engineer..
				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CTFPlayer *pPlayer = dynamic_cast< CTFPlayer* >( UTIL_PlayerByIndex( i ) );
					if ( pPlayer && pPlayer->GetTeamNumber() == iTeam && pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_ENGINEER )
					{
						pPlayer->StartBuildingObjectOfType( OBJ_SENTRYGUN );
						if ( pPlayer->GetActiveWeapon() )
							pPlayer->GetActiveWeapon()->PrimaryAttack();

						break;
					}
				}
			}
		}
	}

	void MoveRedPlayersToBlueArea()
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( pPlayer && pPlayer->GetTeamNumber() == TF_TEAM_RED )
			{
				// If too far from the spawns, then teleport into the blue area.
				CBaseEntity *pSpawn = GetBlueSpawnPoint( false );
				if ( pPlayer->GetAbsOrigin().DistTo( pSpawn->GetAbsOrigin() ) > 2000 )
				{
					pPlayer->Teleport( &pSpawn->GetAbsOrigin(), NULL, NULL );
				}
			}
		}
	}

	virtual CBasePlayer* CreateBot()
	{
		int iTeam = (g_pServerBenchmark->RandomInt( 0, 1 ) == 1) ? TF_TEAM_BLUE : TF_TEAM_RED;
		if ( m_nBotsCreated == 0 )
			iTeam = TF_TEAM_BLUE;

		int iClass = g_pServerBenchmark->RandomInt( TF_FIRST_NORMAL_CLASS, ( TF_LAST_NORMAL_CLASS - 1 ) ); //( TF_LAST_NORMAL_CLASS - 1 ) to exclude the new civilian class
		if ( m_nBotsCreated < 4 )
			iClass = TF_CLASS_ENGINEER; // Make engineers first so they'll build sentries.

		CBasePlayer *pPlayer = BotPutInServer( false, false, iTeam, iClass, NULL );
		if ( !pPlayer )
			Error( "Server benchmark: Can't create bot." );
 		
		++m_nBotsCreated;
		return pPlayer;
	}

private:
	int m_nBotsCreated;
	bool m_bSetupLocalPlayer;
	
	Vector m_vLocalPlayerOrigin;
	QAngle m_vLocalPlayerEyeAngles;
};

static CTFServerBenchmark g_TFServerBenchmark;
