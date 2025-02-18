//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: tf_populator_spawners
// Implementations of NPC Spawning Code for PvE related game modes (MvM)
//=============================================================================//

#include "cbase.h"

#include "tf_populators.h"
#include "tf_populator_spawners.h"
#include "tf_team.h"
#include "tf_obj_sentrygun.h"
#include "tf_objective_resource.h"
#include "eventqueue.h"
#include "tf_tank_boss.h"
#include "tf_gc_server.h"
#include "tf_gamerules.h"
#include "etwprof.h"
#include "team_control_point_master.h"

extern ConVar tf_populator_debug;
extern ConVar tf_populator_active_buffer_range;

ConVar tf_mvm_engineer_teleporter_uber_duration( "tf_mvm_engineer_teleporter_uber_duration", "5.f", FCVAR_CHEAT );
ConVar tf_mvm_currency_bonus_ratio_min( "tf_mvm_currency_bonus_ratio_min", "0.95f", FCVAR_HIDDEN, "The minimum percentage of wave money players must collect in order to qualify for min bonus - 0.1 to 1.0.  Half the bonus amount will be awarded when reaching min ratio, and half when reaching max.", true, 0.1, true, 1.0 );
ConVar tf_mvm_currency_bonus_ratio_max( "tf_mvm_currency_bonus_ratio_max", "1.f", FCVAR_HIDDEN, "The highest percentage of wave money players must collect in order to qualify for max bonus - 0.1 to 1.0.  Half the bonus amount will be awarded when reaching min ratio, and half when reaching max.", true, 0.1, true, 1.0 );

//-----------------------------------------------------------------------
static void FireEvent( EventInfo *eventInfo, const char *eventName )
{
	if ( eventInfo )
	{
		CBaseEntity *targetEntity = gEntList.FindEntityByName( NULL, eventInfo->m_target );
		if ( !targetEntity )
		{
			Warning( "WaveSpawnPopulator: Can't find target entity '%s' for %s\n", eventInfo->m_target.Access(), eventName );
		}
		else
		{
			g_EventQueue.AddEvent( targetEntity, eventInfo->m_action, eventInfo->m_param, eventInfo->m_delay, NULL, NULL );
		}
	}
}

//-----------------------------------------------------------------------
static EventInfo *ParseEvent( KeyValues *values )
{
	EventInfo *eventInfo = new EventInfo;

	for ( KeyValues *data = values->GetFirstSubKey(); data != NULL; data = data->GetNextKey() )
	{
		const char *name = data->GetName();

		if ( Q_strlen( name ) <= 0 )
		{
			continue;
		}

		if ( !Q_stricmp( name, "Target" ) )
		{
			eventInfo->m_target.sprintf( "%s", data->GetString() );
		}
		else if ( !Q_stricmp( name, "Action" ) )
		{
			eventInfo->m_action.sprintf( "%s", data->GetString() );
		}
		else if ( !Q_stricmp( name, "Param" ) )
        {
            eventInfo->m_param.SetString( AllocPooledString( data->GetString() ) );
        }
        else if ( !Q_stricmp( name, "Delay" ) )
        {
            eventInfo->m_delay = data->GetFloat();
        }
		else
		{
			Warning( "Unknown field '%s' in WaveSpawn event definition.\n", data->GetString() );
			delete eventInfo;
			return NULL;
		}
	}

	return eventInfo;
}

static CHandle<CBaseEntity> s_lastTeleporter = NULL;
static float s_flLastTeleportTime = -1;

//-----------------------------------------------------------------------
// Given a named entity, select a random invader teleporter with the same name and
// return it's WorldSpaceCenter.
SpawnLocationResult DoTeleporterOverride( CBaseEntity *spawnEnt, Vector& vSpawnPosition, bool bClosestPointOnNav )
{
	CUtlVector< CBaseEntity * > teleporterVector;

	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
		if ( pObj->GetType() != OBJ_TELEPORTER )
			continue;

		if ( pObj->GetTeamNumber() != TF_TEAM_PVE_INVADERS )
			continue;

		if ( pObj->IsBuilding() )
			continue;

		if ( pObj->HasSapper() )
			continue;

		if ( pObj->IsPlasmaDisabled() )
			continue;

		CObjectTeleporter *teleporter = assert_cast< CObjectTeleporter* >( pObj );
		const CUtlStringList& teleportWhereNames = teleporter->GetTeleportWhere();

		const char* pszSpawnPointName = STRING( spawnEnt->GetEntityName() );
		for ( int iTelePoints =0; iTelePoints<teleportWhereNames.Count(); ++iTelePoints )
		{
			// check if this teleporter can replace the original spawn point
			if ( FStrEq( teleportWhereNames[iTelePoints], pszSpawnPointName ) )
			{
				teleporterVector.AddToTail( teleporter );
				break;
			}
		}
	}

	if ( teleporterVector.Count() > 0 )
	{
		int which = RandomInt( 0, teleporterVector.Count()-1 );
		vSpawnPosition = teleporterVector[ which ]->WorldSpaceCenter();
		s_lastTeleporter = teleporterVector[ which ];
		return SPAWN_LOCATION_TELEPORTER;
	}

	CTFNavArea *pNav = (CTFNavArea *)TheNavMesh->GetNearestNavArea( spawnEnt->WorldSpaceCenter() );
	if ( !pNav )
		return SPAWN_LOCATION_NOT_FOUND;

	if ( bClosestPointOnNav )
	{
		pNav->GetClosestPointOnArea( spawnEnt->WorldSpaceCenter(), &vSpawnPosition );
	}
	else
	{
		vSpawnPosition = pNav->GetCenter();
	}

	return SPAWN_LOCATION_NAV;
}

//-----------------------------------------------------------------------
void OnBotTeleported( CTFBot* bot )
{
	const Vector& origin = s_lastTeleporter->GetAbsOrigin();

	// don't too many sound and effect when lots of bots teleporting in short time.
	if ( gpGlobals->curtime - s_flLastTeleportTime > 0.1f  )
	{
		CPVSFilter filter( origin );
#if 0
		// These are pretty, but they're chewing into our particle budget (1000 particles each!)
		// They're also basically invisible because bots spawn in ubered.

		TE_TFParticleEffect( filter, 0.0, "teleported_blue", origin, vec3_angle );
		TE_TFParticleEffect( filter, 0.0, "player_sparkles_blue", origin, vec3_angle );
#endif
		s_lastTeleporter->EmitSound( "MVM.Robot_Teleporter_Deliver" );

		s_flLastTeleportTime = gpGlobals->curtime;
	}

	// force bot to face in the direction specified by the teleporter
	Vector vForward;
	AngleVectors( s_lastTeleporter->GetAbsAngles(), &vForward, NULL, NULL );
	bot->GetLocomotionInterface()->FaceTowards( bot->GetAbsOrigin() + 50 * vForward );

	// spy shouldn't get any effect from the teleporter
	if ( !bot->IsPlayerClass( TF_CLASS_SPY ) )
	{
		bot->TeleportEffect();

		// invading bots get uber while they leave their spawn so they don't drop their cash where players can't pick it up
		float flUberTime = tf_mvm_engineer_teleporter_uber_duration.GetFloat();
		bot->m_Shared.AddCond( TF_COND_INVULNERABLE, flUberTime );
		bot->m_Shared.AddCond( TF_COND_INVULNERABLE_WEARINGOFF, flUberTime );
	}
}

//-----------------------------------------------------------------------
// CSpawnLocation
//-----------------------------------------------------------------------
CSpawnLocation::CSpawnLocation()
{
	m_relative = UNDEFINED;
	m_teamSpawnVector.RemoveAll();
	m_nSpawnCount = 0;
	m_nRandomSeed = RandomInt( 0, 9999 );
	m_bClosestPointOnNav = false;
}

//-----------------------------------------------------------------------
// Return true if we successfully parse a "Where" clause
bool CSpawnLocation::Parse( KeyValues *data )
{
	const char *name = data->GetName();
	const char *value = data->GetString();

	if ( Q_strlen( name ) <= 0 )
	{
		return false;
	}

	if ( FStrEq( name, "Where" ) || FStrEq( name, "ClosestPoint" ) )
	{
		if ( FStrEq( value, "Ahead" ) )
		{
			m_relative = AHEAD;
		}
		else if ( FStrEq( value, "Behind" ) )
		{
			m_relative = BEHIND;
		}
		else if ( FStrEq( value, "Anywhere" ) )
		{
			m_relative = ANYWHERE;
		}
		else
		{
			m_bClosestPointOnNav = FStrEq( name, "ClosestPoint" );

			// collect entities with given name
			bool bFound = false;
			for ( int i=0; i<ITFTeamSpawnAutoList::AutoList().Count(); ++i )
			{
				CTFTeamSpawn* pTeamSpawn = static_cast< CTFTeamSpawn* >( ITFTeamSpawnAutoList::AutoList()[i] );
				if ( FStrEq( STRING( pTeamSpawn->GetEntityName() ), value ) )
				{
					m_teamSpawnVector.AddToTail( pTeamSpawn );
					bFound = true;
				}
			}

			if ( !bFound )
			{
				Warning( "Invalid Where argument '%s'\n", value );
				return false;
			}
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------
SpawnLocationResult CSpawnLocation::FindSpawnLocation( Vector& vSpawnPosition )
{
	TFTeamSpawnVector_t activeSpawn;
	for ( int i=0; i<m_teamSpawnVector.Count(); ++i )
	{
		if ( m_teamSpawnVector[i]->IsDisabled() )
			continue;

		activeSpawn.AddToTail( m_teamSpawnVector[i] );
	}

	// treat spawn points as deck of cards. shuffle it when we run out
	if ( m_nSpawnCount >= activeSpawn.Count() )
	{
		m_nRandomSeed = RandomInt( 0, 9999 );
		m_nSpawnCount = 0;
	}
	CUniformRandomStream randomSpawn;
	randomSpawn.SetSeed( m_nRandomSeed );
	activeSpawn.Shuffle( &randomSpawn );

	if ( activeSpawn.Count() > 0 )
	{
		// if any invading teleporters exist with this name, use them instead
		SpawnLocationResult result = DoTeleporterOverride( activeSpawn[ m_nSpawnCount ], vSpawnPosition, m_bClosestPointOnNav );
		if ( result != SPAWN_LOCATION_NOT_FOUND )
		{
			m_nSpawnCount++;
			return result;
		}
	}

	CTFNavArea *spawnArea = SelectSpawnArea();
	if ( spawnArea )
	{
		vSpawnPosition = spawnArea->GetCenter();
		return SPAWN_LOCATION_NAV;
	}

	return SPAWN_LOCATION_NOT_FOUND;
}

//-----------------------------------------------------------------------
CTFNavArea *CSpawnLocation::SelectSpawnArea( void ) const
{
	VPROF_BUDGET( "CSpawnLocation::SelectSpawnArea", "NextBot" );

	if ( m_relative == UNDEFINED )
	{
		return NULL;
	}

#ifdef TF_RAID_MODE
	CTFPlayer *farRaider = g_pRaidLogic->GetFarthestAlongRaider();

	if ( !farRaider )
	{
		return NULL;
	}
#endif // TF_RAID_MODE

	//
	// Collect all areas surrounding the invading team and
	// build a vector sorted by increasing incursion distance
	//
	CUtlSortVector< CTFNavArea *, CTFNavAreaIncursionLess > theaterAreaVector;

	CTFNavArea::MakeNewTFMarker();

	CTeam *team = GetGlobalTeam( TF_TEAM_BLUE );
	for( int t=0; t<team->GetNumPlayers(); ++t )
	{
		CTFPlayer *teamMember = (CTFPlayer *)team->GetPlayer(t);

		if ( !teamMember->IsAlive() )
			continue;

		CTFBot *bot = ToTFBot( teamMember );
		if ( bot && bot->HasAttribute( CTFBot::IS_NPC ) )
			continue;

		if ( teamMember->GetLastKnownArea() == NULL )
			continue;

		// collect areas surrounding this invader
		CUtlVector< CNavArea * > nearbyAreaVector;
		CollectSurroundingAreas( &nearbyAreaVector, teamMember->GetLastKnownArea(), tf_populator_active_buffer_range.GetFloat() );

		for( int i=0; i<nearbyAreaVector.Count(); ++i )
		{
			CTFNavArea *area = (CTFNavArea *)nearbyAreaVector[i];

			if ( !area->IsTFMarked() )
			{		
				area->TFMark();

				if ( area->IsPotentiallyVisibleToTeam( TF_TEAM_BLUE ) )
					continue;

				if ( !area->IsValidForWanderingPopulation() )
					continue;

				theaterAreaVector.Insert( area );

				if ( tf_populator_debug.GetBool() )
				{
					TheTFNavMesh()->AddToSelectedSet( area );
				}
			}
		}
	}

	if ( theaterAreaVector.Count() == 0 )
	{
		if ( tf_populator_debug.GetBool() ) 
		{
			DevMsg( "%3.2f: SelectSpawnArea: Empty theater!\n", gpGlobals->curtime );
		}
		return NULL;
	}

	const int maxRetries = 5;
	CTFNavArea *spawnArea = NULL;

	for( int r=0; r<maxRetries; ++r )
	{
		int which = 0;

		switch( m_relative )
		{
		case AHEAD:
			// areas are sorted from behind to ahead - weight the selection to choose ahead
			which = SkewedRandomValue() * theaterAreaVector.Count();
			break;

		case BEHIND:
			// areas are sorted from behind to ahead - weight the selection to choose behind
			which = ( 1.0f - SkewedRandomValue() ) * theaterAreaVector.Count();
			break;

		case ANYWHERE:
			// choose any valid area at random
			which = RandomFloat( 0.0f, 1.0f ) * theaterAreaVector.Count();
			break;
		}

		if ( which >= theaterAreaVector.Count() )
			which = theaterAreaVector.Count()-1;

		spawnArea = theaterAreaVector[ which ];

		// well behaved spawn area
		return spawnArea;
		
	}

	return NULL;
}

//-----------------------------------------------------------------------
// CMissionPopulator
//-----------------------------------------------------------------------
CMissionPopulator::CMissionPopulator( CPopulationManager *manager ) : IPopulator( manager )
{
	m_mission = CTFBot::NO_MISSION;
	m_initialCooldown = 0.0f;
	m_cooldownDuration = 0.0f;
	m_desiredCount = 0;
	m_beginAtWaveIndex = 0;
	m_stopAtWaveIndex = 99999;
	m_state = NOT_STARTED;
}


//-----------------------------------------------------------------------
bool CMissionPopulator::Parse( KeyValues *values )
{
	int waveDuration = 99999;

	for ( KeyValues *data = values->GetFirstSubKey(); data != NULL; data = data->GetNextKey() )
	{
		const char *name = data->GetName();

		if ( Q_strlen( name ) <= 0 )
		{
			continue;
		}

		if ( m_where.Parse( data ) )
		{
			continue;
		}

		if ( !Q_stricmp( name, "Objective" ) )
		{
			if ( !Q_stricmp( data->GetString(), "DestroySentries" ) )
			{
				m_mission = CTFBot::MISSION_DESTROY_SENTRIES;
			}
			else if ( !Q_stricmp( data->GetString(), "Sniper" ) )
			{
				m_mission = CTFBot::MISSION_SNIPER;
			}
			else if ( !Q_stricmp( data->GetString(), "Spy" ) )
			{
				m_mission = CTFBot::MISSION_SPY;
			}
			else if ( !Q_stricmp( data->GetString(), "Engineer" ) )
			{
				m_mission = CTFBot::MISSION_ENGINEER;
			}
			else if ( !Q_stricmp( data->GetString(), "SeekAndDestroy" ) )
			{
				m_mission = CTFBot::MISSION_DESTROY_SENTRIES;
			}
			else
			{
				Warning( "Invalid mission '%s'\n", data->GetString() );
				return false;
			}
		}
		else if ( !Q_stricmp( name, "InitialCooldown" ) )
		{
			m_initialCooldown = data->GetFloat();
		}
		else if ( !Q_stricmp( name, "CooldownTime" ) )
		{
			m_cooldownDuration = data->GetFloat();
		}
		else if ( !Q_stricmp( name, "BeginAtWave" ) )
		{
			m_beginAtWaveIndex = data->GetInt() - 1;		// internally counts from 0
		}
		else if ( !Q_stricmp( name, "RunForThisManyWaves" ) )
		{
			waveDuration = data->GetInt();
		}
		else if ( !Q_stricmp( name, "DesiredCount" ) )
		{
			m_desiredCount = data->GetInt();
		}
		else
		{
			m_spawner = IPopulationSpawner::ParseSpawner( this, data );

			if ( m_spawner == NULL )
			{
				Warning( "Unknown attribute '%s' in Mission definition.\n", name );
			}
		}
	}

	m_stopAtWaveIndex = m_beginAtWaveIndex + waveDuration;

	return true;
}


//--------------------------------------------------------------------------------------------------------
// Dispatch sentry killer squads
bool CMissionPopulator::UpdateMissionDestroySentries( void )
{
	VPROF_BUDGET( "CMissionPopulator::UpdateMissionDestroySentries", "NextBot" );

	if ( !m_cooldownTimer.IsElapsed() )
	{
		return false;
	}

	if ( !m_checkForDangerousSentriesTimer.IsElapsed() )
	{
		return false;
	}

	if( g_pPopulationManager->IsSpawningPaused() )
	{
		return false;
	}

	m_checkForDangerousSentriesTimer.Start( RandomFloat( 5.0f, 10.0f ) );

	// collect all of the dangerous sentries
	CUtlVector< CObjectSentrygun * > dangerousSentryVector;

	int nDmgLimit = 0;	
	int nKillLimit = 0;
	GetManager()->GetSentryBusterDamageAndKillThreshold( nDmgLimit, nKillLimit );

	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject* pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
		if ( pObj->ObjectType() == OBJ_SENTRYGUN )
		{
			// Disposable sentries are not valid targets
			if ( pObj->IsDisposableBuilding() )
				continue;

			if ( pObj->GetTeamNumber() == TF_TEAM_PVE_DEFENDERS )
			{
				CTFPlayer *sentryOwner = pObj->GetOwner();
				if ( sentryOwner )
				{
					int nDmgDone = sentryOwner->GetAccumulatedSentryGunDamageDealt();
					int nKillsMade = sentryOwner->GetAccumulatedSentryGunKillCount();

					if ( nDmgDone >= nDmgLimit || nKillsMade >= nKillLimit )
					{
						dangerousSentryVector.AddToTail( static_cast< CObjectSentrygun* >( pObj ) );
					}
				}
			}
		}
	}

	CUtlVector< CTFPlayer * > livePlayerVector;
	CollectPlayers( &livePlayerVector, TF_TEAM_PVE_INVADERS, COLLECT_ONLY_LIVING_PLAYERS );

	// dispatch a sentry busting squad for each dangerous sentry
	bool didSpawn = false;

	for( int i=0; i<dangerousSentryVector.Count(); ++i )
	{
		CObjectSentrygun *targetSentry = dangerousSentryVector[i];

		// if there is already a squad out there destroying this sentry, don't spawn another one
		int j;
		for( j=0; j<livePlayerVector.Count(); ++j )
		{
			CTFBot *bot = dynamic_cast<CTFBot *>( livePlayerVector[j] );
			if ( bot && bot->HasMission( CTFBot::MISSION_DESTROY_SENTRIES ) && bot->GetMissionTarget() == targetSentry )
			{
				// there is already a sentry busting squad active for this sentry
				break;
			}
		}

		if ( j < livePlayerVector.Count() )
		{
			continue;
		}

		// spawn a sentry buster squad to destroy this sentry
		Vector vSpawnPosition;
		SpawnLocationResult spawnLocationResult = m_where.FindSpawnLocation( vSpawnPosition );
		if ( spawnLocationResult != SPAWN_LOCATION_NOT_FOUND )
		{
			EntityHandleVector_t spawnVector;

			if ( m_spawner && m_spawner->Spawn( vSpawnPosition, &spawnVector ) )
			{
				// success
				if ( tf_populator_debug.GetBool() )
				{
					DevMsg( "MANN VS MACHINE: %3.2f: <<<< Spawning Sentry Busting Mission >>>>\n", gpGlobals->curtime );
				}

				for( int k=0; k<spawnVector.Count(); ++k )
				{
					CTFBot *bot = ToTFBot( spawnVector[k] );
					if ( bot )
					{
						bot->SetFlagTarget( NULL );
						bot->SetMission( CTFBot::MISSION_DESTROY_SENTRIES );
						bot->SetMissionTarget( targetSentry );

						// force an update to start the behavior so we can set the sentry
						bot->Update();

						bot->MarkAsMissionEnemy();

						didSpawn = true;

						bot->GetPlayerClass()->SetCustomModel( g_szBotBossSentryBusterModel, USE_CLASS_ANIMATIONS );
						bot->UpdateModel();
						bot->SetBloodColor( DONT_BLEED );

						if ( TFObjectiveResource() )
						{
							unsigned int iFlags = MVM_CLASS_FLAG_MISSION;
							if ( bot->IsMiniBoss() )
							{
								iFlags |= MVM_CLASS_FLAG_MINIBOSS;
							}
							if ( bot->HasAttribute( CTFBot::ALWAYS_CRIT ) )
							{
								iFlags |= MVM_CLASS_FLAG_ALWAYSCRIT;
							}
							TFObjectiveResource()->IncrementMannVsMachineWaveClassCount( m_spawner->GetClassIcon( k ), iFlags );
						}

						if ( TFGameRules() )
						{
							TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_SENTRY_BUSTER, TF_TEAM_PVE_DEFENDERS );
						}

						// what bot should do after spawning at teleporter exit
						if ( spawnLocationResult == SPAWN_LOCATION_TELEPORTER )
						{
							OnBotTeleported( bot );
						}
					}
				}
			}
		}
		else if ( tf_populator_debug.GetBool() )
		{
			Warning( "MissionPopulator: %3.2f: Can't find a place to spawn a sentry destroying squad\n", gpGlobals->curtime );
		}
	}

	if ( didSpawn )
	{
		float flCoolDown = m_cooldownDuration;

		CWave *pWave = GetManager()->GetCurrentWave();
		if ( pWave )
		{
			pWave->IncrementSentryBustersSpawned();
			
			if ( TFGameRules() )
			{
				if ( pWave->NumSentryBustersSpawned() > 1 )
				{
					TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Sentry_Buster_Alert_Another" );
				}
				else
				{
					TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Sentry_Buster_Alert" );
				}
			}

			flCoolDown = m_cooldownDuration + pWave->NumSentryBustersKilled() * m_cooldownDuration;

			pWave->ResetSentryBustersKilled();
		}

		m_cooldownTimer.Start( flCoolDown );
	}

	return didSpawn;
}


//-----------------------------------------------------------------------
bool CMissionPopulator::UpdateMission( CTFBot::MissionType mission )
{
	VPROF_BUDGET( "CMissionPopulator::UpdateMission", "NextBot" );

	int activeMissionMembers = 0;

	CUtlVector< CTFPlayer * > livePlayerVector;
	CollectPlayers( &livePlayerVector, TF_TEAM_PVE_INVADERS, COLLECT_ONLY_LIVING_PLAYERS );

	for( int i=0; i<livePlayerVector.Count(); ++i )
	{
		CTFBot *pBot = dynamic_cast<CTFBot *>( livePlayerVector[i] );
		if ( pBot && pBot ->HasMission( mission ) )
		{
			++activeMissionMembers;
		}
	}

	if( g_pPopulationManager->IsSpawningPaused() )
	{
		return false;
	}

	if ( activeMissionMembers > 0 )
	{
		// wait until prior mission is dead

		// cooldown is time after death of last mission member
		m_cooldownTimer.Start( m_cooldownDuration );

		return false;
	}

	if ( !m_cooldownTimer.IsElapsed() )
	{
		return false;
	}

	// are there enough free slots?
	int currentEnemyCount = GetGlobalTeam( TF_TEAM_PVE_INVADERS )->GetNumPlayers();

	if ( currentEnemyCount + m_desiredCount > CPopulationManager::MVM_INVADERS_TEAM_SIZE )
	{
		// not enough slots yet
		if ( tf_populator_debug.GetBool() ) 
		{
			DevMsg( "MANN VS MACHINE: %3.2f: Waiting for slots to spawn mission.\n", gpGlobals->curtime );
		}

		return false;
	}

	if ( tf_populator_debug.GetBool() ) 
	{
		DevMsg( "MANN VS MACHINE: %3.2f: <<<< Spawning Mission >>>>\n", gpGlobals->curtime );
	}

	int nSniperCount = 0;
	FOR_EACH_VEC( livePlayerVector, iLiveBot )
	{
		CTFBot *pBot = dynamic_cast<CTFBot *>( livePlayerVector[iLiveBot] );
		if ( pBot && pBot->IsPlayerClass( TF_CLASS_SNIPER ) )
		{
			nSniperCount++;
		}
	}

	// dispatch mission members
	for( int iDesiredCount=0; iDesiredCount<m_desiredCount; ++iDesiredCount )
	{
		Vector vSpawnPosition;
		SpawnLocationResult spawnLocationResult = m_where.FindSpawnLocation( vSpawnPosition );
		if ( spawnLocationResult != SPAWN_LOCATION_NOT_FOUND )
		{
			EntityHandleVector_t spawnedVector;
			if ( m_spawner && m_spawner->Spawn( vSpawnPosition, &spawnedVector ) )
			{
				// success
				for( int iSpawn=0; iSpawn<spawnedVector.Count(); ++iSpawn )
				{
					CTFBot *bot = ToTFBot( spawnedVector[iSpawn] );
					if ( bot )
					{
						bot->SetFlagTarget( NULL );
						bot->SetMission( mission );
						//bot->SetMissionString( "" );
						bot->MarkAsMissionEnemy();

						if ( TFObjectiveResource() )
						{
							unsigned int iFlags = MVM_CLASS_FLAG_MISSION;
							if ( bot->IsMiniBoss() )
							{
								iFlags |= MVM_CLASS_FLAG_MINIBOSS;
							}
							if ( bot->HasAttribute( CTFBot::ALWAYS_CRIT ) )
							{
								iFlags |= MVM_CLASS_FLAG_ALWAYSCRIT;
							}
							TFObjectiveResource()->IncrementMannVsMachineWaveClassCount( bot->GetPlayerClass()->GetClassIconName(), iFlags );
						}

						// Response rules stuff for MvM
						if ( TFGameRules()->IsMannVsMachineMode() )
						{
							// Only have defenders announce the arrival of the first enemy Sniper
							if ( bot->HasMission( CTFBot::MISSION_SNIPER ) )
							{
								nSniperCount++;

								if ( nSniperCount == 1 )
								{
									TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_SNIPER_CALLOUT, TF_TEAM_PVE_DEFENDERS );
								}
							}
						}

						// what bot should do after spawning at teleporter exit
						if ( spawnLocationResult == SPAWN_LOCATION_TELEPORTER )
						{
							OnBotTeleported( bot );
						}
					}
				}
			}
		}
		else
		{
			if ( tf_populator_debug.GetBool() ) 
			{
				Warning( "MissionPopulator: %3.2f: Skipped a member - can't find a place to spawn\n", gpGlobals->curtime );
			}
		}
	}

	m_cooldownTimer.Start( m_cooldownDuration );

	return true;
}


//-----------------------------------------------------------------------
void CMissionPopulator::Update( void )
{
	VPROF_BUDGET( "CMissionPopulator::Update", "NextBot" );

	if ( TFGameRules()->InSetup() ||
		 GetManager()->GetWaveNumber() < m_beginAtWaveIndex ||
		 GetManager()->GetWaveNumber() >= m_stopAtWaveIndex ||
		 TFObjectiveResource()->GetMannVsMachineIsBetweenWaves() )
	{
		m_state = NOT_STARTED;
		return;
	}

	if ( m_state == NOT_STARTED )
	{
		if ( m_initialCooldown > 0.0f )
		{
			m_state = INITIAL_COOLDOWN;
			m_cooldownTimer.Start( m_initialCooldown );
			return;
		}

		m_state = RUNNING;
		m_cooldownTimer.Invalidate();
	}
	else if ( m_state == INITIAL_COOLDOWN )
	{
		if ( !m_cooldownTimer.IsElapsed() )
		{
			return;
		}

		m_state = RUNNING;
		m_cooldownTimer.Invalidate();
	}

	switch( m_mission )
	{
	case CTFBot::MISSION_SEEK_AND_DESTROY:
		break;

	case CTFBot::MISSION_DESTROY_SENTRIES:
		UpdateMissionDestroySentries();
		break;

	case CTFBot::MISSION_SNIPER:
	case CTFBot::MISSION_SPY:
	case CTFBot::MISSION_ENGINEER:
		UpdateMission( m_mission );
		break;
	}
}


void CMissionPopulator::UnpauseSpawning( void )
{
	m_cooldownTimer.Start( m_cooldownDuration );
	m_checkForDangerousSentriesTimer.Start( RandomFloat( 5.0f, 10.0f ) );
}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
CRandomPlacementPopulator::CRandomPlacementPopulator( CPopulationManager *manager ) : IPopulator( manager )
{
	m_count = 0;
	m_minSeparation = 0.0f;
	m_navAreaFilter = 0xFFFFFFFF;
}


//-----------------------------------------------------------------------
bool CRandomPlacementPopulator::Parse( KeyValues *values )
{
	for ( KeyValues *data = values->GetFirstSubKey(); data != NULL; data = data->GetNextKey() )
	{
		const char *name = data->GetName();

		if ( Q_strlen( name ) <= 0 )
		{
			continue;
		}

		if ( !Q_stricmp( name, "Count" ) )
		{
			m_count = data->GetInt();
		}
		else if ( !Q_stricmp( name, "MinimumSeparation" ) )
		{
			m_minSeparation = data->GetFloat();
		}
		else if ( !Q_stricmp( name, "NavAreaFilter" ) )
		{
			if ( !Q_stricmp( data->GetString(), "SENTRY_SPOT" ) )
			{
				m_navAreaFilter = TF_NAV_SENTRY_SPOT;
			}
			else if ( !Q_stricmp( data->GetString(), "SNIPER_SPOT" ) )
			{
				m_navAreaFilter = TF_NAV_SNIPER_SPOT;
			}
			else
			{
				Warning( "Unknown NavAreaFilter value '%s'\n", data->GetString() );
			}
		}
		else
		{
			m_spawner = IPopulationSpawner::ParseSpawner( this, data );

			if ( m_spawner == NULL )
			{
				Warning( "Unknown attribute '%s' in RandomPlacement definition.\n", name );
			}
		}
	}

	return true;
}


//-----------------------------------------------------------------------
// Create initial population at start of scenario
void CRandomPlacementPopulator::PostInitialize( void )
{
	int i;
	CUtlVector< CTFNavArea * > candidateAreaVector;

	for( i=0; i<TheNavAreas.Count(); ++i )
	{
		CTFNavArea *area = (CTFNavArea *)TheNavAreas[i];

		if ( area->HasAttributeTF( m_navAreaFilter ) )
		{
			candidateAreaVector.AddToTail( area );
		}
	}

	CUtlVector< CTFNavArea * > selectedAreaVector;
	SelectSeparatedShuffleSet< CTFNavArea >( m_count, m_minSeparation, candidateAreaVector, &selectedAreaVector );

	if ( m_spawner )
	{
		for( i=0; i<selectedAreaVector.Count(); ++i )
		{
			m_spawner->Spawn( selectedAreaVector[i]->GetCenter() );
		}
	}
}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
CPeriodicSpawnPopulator::CPeriodicSpawnPopulator( CPopulationManager *manager ) : IPopulator( manager )
{
	m_minInterval = 30.0f;
	m_maxInterval = 30.0f;
}


//-----------------------------------------------------------------------
bool CPeriodicSpawnPopulator::Parse( KeyValues *values )
{
	for ( KeyValues *data = values->GetFirstSubKey(); data != NULL; data = data->GetNextKey() )
	{
		const char *name = data->GetName();

		if ( Q_strlen( name ) <= 0 )
		{
			continue;
		}

		if ( m_where.Parse( data ) )
		{
			continue;
		}

		if ( !Q_stricmp( name, "When" ) )
		{
			if ( data->GetFirstSubKey() )
			{
				for ( KeyValues *whenData = data->GetFirstSubKey(); whenData != NULL; whenData = whenData->GetNextKey() )
				{
					if ( !Q_stricmp( whenData->GetName(), "MinInterval" ) )
					{
						m_minInterval = whenData->GetFloat();
					}
					else if ( !Q_stricmp( whenData->GetName(), "MaxInterval" ) )
					{
						m_maxInterval = whenData->GetFloat();
					}
					else
					{
						Warning( "Invalid field '%s' encountered in When\n", whenData->GetName() );
						return false;
					}
				}
			}
			else
			{
				// single constant value
				m_minInterval = data->GetFloat();
				m_maxInterval = m_minInterval;
			}
		}
		else
		{
			m_spawner = IPopulationSpawner::ParseSpawner( this, data );

			if ( m_spawner == NULL )
			{
				Warning( "Unknown attribute '%s' in PeriodicSpawn definition.\n", name );
			}
		}
	}

	return true;
}


//-----------------------------------------------------------------------
// Create initial population at start of scenario
void CPeriodicSpawnPopulator::PostInitialize( void )
{
	m_timer.Start( RandomFloat( m_minInterval, m_maxInterval ) );
}



//-----------------------------------------------------------------------
// Continuously invoked to modify population over time
void CPeriodicSpawnPopulator::Update( void )
{
	if ( m_timer.IsElapsed() && !g_pPopulationManager->IsSpawningPaused() )
	{
		m_timer.Start( RandomFloat( m_minInterval, m_maxInterval ) );

		Vector vSpawnPosition;
		SpawnLocationResult spawnLocationResult = m_where.FindSpawnLocation( vSpawnPosition );
		if ( spawnLocationResult != SPAWN_LOCATION_NOT_FOUND )
		{
			EntityHandleVector_t spawnedVector;
			if ( m_spawner && m_spawner->Spawn( vSpawnPosition, &spawnedVector ) )
			{
				// success
				for( int i=0; i<spawnedVector.Count(); ++i )
				{
					CTFBot *bot = ToTFBot( spawnedVector[i] );
					if ( bot )
					{
						// what bot should do after spawning at teleporter exit
						if ( spawnLocationResult == SPAWN_LOCATION_TELEPORTER )
						{
							OnBotTeleported( bot );
						}
					}
				}

				return;
			}
		}

		// retry soon, in the hopes constraints have changed
		m_timer.Start( 2.0f );
	}
}

void CPeriodicSpawnPopulator::UnpauseSpawning( void )
{
	m_timer.Start( RandomFloat( m_minInterval, m_maxInterval ) );
}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

int CWaveSpawnPopulator::m_reservedPlayerSlotCount = 0;

CWaveSpawnPopulator::CWaveSpawnPopulator( CPopulationManager *manager ) : IPopulator( manager )
{
	m_totalCount = 0;
	m_maxActive = 999;
	m_spawnCount = 1;
	m_waitBeforeStarting = 0.0f;
	m_waitBetweenSpawns = 0.0f;
	m_bWaitBetweenSpawnAfterDeath = false;
	m_totalCurrency = -1;
	SetState( PENDING );

	m_startWaveOutput = NULL;
	m_firstSpawnOutput = NULL;
	m_lastSpawnOutput = NULL;
	m_doneOutput = NULL;

	m_bSupportWave = false;
	m_bLimitedSupport = false;
	m_pParent = NULL;

	m_bRandomSpawn = false;
	m_spawnLocationResult = SPAWN_LOCATION_NOT_FOUND;
}

//-----------------------------------------------------------------------
CWaveSpawnPopulator::~CWaveSpawnPopulator()
{
	delete m_startWaveOutput;
	delete m_firstSpawnOutput;
	delete m_lastSpawnOutput;
	delete m_doneOutput;
}

//-----------------------------------------------------------------------
void CWaveSpawnPopulator::ForceFinish()
{
	if ( m_state < WAIT_FOR_ALL_DEAD )
	{
		SetState( WAIT_FOR_ALL_DEAD );
	}
	else if ( m_state != WAIT_FOR_ALL_DEAD )
	{
		SetState( DONE );
	}

	FOR_EACH_VEC( m_activeVector, i )
	{
		// Move bots over to spectator
		CTFBot *pBot = ToTFBot( m_activeVector[i] );
		if ( pBot )
		{
			pBot->ChangeTeam( TEAM_SPECTATOR, false, true );
		}
		else // Other things just get removed.  (ie. Tanks)
		{
			m_activeVector[i]->Remove();
		}
	}

	m_activeVector.Purge();
}


//-----------------------------------------------------------------------
bool CWaveSpawnPopulator::Parse( KeyValues *values )
{
	// First, see if we have any Template keys
	KeyValues *pTemplate = values->FindKey( "Template" );
	if ( pTemplate )
	{
		KeyValues *pTemplateKV = GetManager()->GetTemplate( pTemplate->GetString() );
		if ( pTemplateKV )
		{
			// Pump all the keys into ourself now
			if ( Parse( pTemplateKV ) == false )
			{
				return false;
			}
		}
		else
		{
			Warning( "Unknown Template '%s' in WaveSpawn definition\n", pTemplate->GetString() );
		}
	}

	for ( KeyValues *data = values->GetFirstSubKey(); data != NULL; data = data->GetNextKey() )
	{
		const char *name = data->GetName();

		if ( Q_strlen( name ) <= 0 )
		{
			continue;
		}

		if ( m_where.Parse( data ) )
		{
			continue;
		}

		// Skip templates when looping through the rest of the keys
		if ( !Q_stricmp( name, "Template" ) )
			continue;

		if ( !Q_stricmp( name, "TotalCount" ) )
		{
			m_totalCount = data->GetInt();
		}
		else if ( !Q_stricmp( name, "MaxActive" ) )
		{
			m_maxActive = data->GetInt();
		}
		else if ( !Q_stricmp( name, "SpawnCount" ) )
		{
			m_spawnCount = data->GetInt();
		}
		else if ( !Q_stricmp( name, "WaitBeforeStarting" ) )
		{
			m_waitBeforeStarting = data->GetFloat();
		}
		else if ( !Q_stricmp( name, "WaitBetweenSpawns" ) )
		{
			if ( m_waitBetweenSpawns != 0.f && m_bWaitBetweenSpawnAfterDeath )
			{
				Warning( "Already specified WaitBetweenSpawnsAfterDeath time, WaitBetweenSpawns won't be used\n" );
				continue;
			}

			m_waitBetweenSpawns = data->GetFloat();
		}
		else if ( !Q_stricmp( name, "WaitBetweenSpawnsAfterDeath" ) )
		{
			if ( m_waitBetweenSpawns != 0.f )
			{
				Warning( "Already specified WaitBetweenSpawns time, WaitBetweenSpawnsAfterDeath won't be used\n" );
				continue;
			}

			m_bWaitBetweenSpawnAfterDeath = true;
			m_waitBetweenSpawns = data->GetFloat();
		}
		else if ( !Q_stricmp( name, "StartWaveWarningSound" ) )
		{
			m_startWaveWarningSound.sprintf( "%s", data->GetString() );
		}
		else if ( !Q_stricmp( name, "StartWaveOutput" ) )
		{
			m_startWaveOutput = ParseEvent( data );
		}
		else if ( !Q_stricmp( name, "FirstSpawnWarningSound" ) )
		{
			m_firstSpawnWarningSound.sprintf( "%s", data->GetString() );
		}
		else if ( !Q_stricmp( name, "FirstSpawnOutput" ) )
		{
			m_firstSpawnOutput = ParseEvent( data );
		}
		else if ( !Q_stricmp( name, "LastSpawnWarningSound" ) )
		{
			m_lastSpawnWarningSound.sprintf( "%s", data->GetString() );
		}
		else if ( !Q_stricmp( name, "LastSpawnOutput" ) )
		{
			m_lastSpawnOutput = ParseEvent( data );
		}
		else if ( !Q_stricmp( name, "DoneWarningSound" ) )
		{
			m_doneWarningSound.sprintf( "%s", data->GetString() );
		}
		else if ( !Q_stricmp( name, "DoneOutput" ) )
		{
			m_doneOutput = ParseEvent( data );
		}
		else if ( !Q_stricmp( name, "TotalCurrency" ) )
		{
			m_totalCurrency = data->GetInt();
		}
		else if ( !Q_stricmp( name, "Name" ) )
		{
			m_name = data->GetString();
		}
		else if ( !Q_stricmp( name, "WaitForAllSpawned" ) )
		{
			m_waitForAllSpawned = data->GetString();
		}
		else if ( !Q_stricmp( name, "WaitForAllDead" ) )
		{
			m_waitForAllDead = data->GetString();
		}
		else if ( !Q_stricmp( name, "Support" ) )
		{
			m_bLimitedSupport = !Q_stricmp( data->GetString(), "Limited" );
			m_bSupportWave = true;
		}
		else if ( !Q_stricmp( name, "RandomSpawn" ) )
		{
			m_bRandomSpawn = data->GetBool();
		}
		else
		{
			m_spawner = IPopulationSpawner::ParseSpawner( this, data );

			if ( m_spawner == NULL )
			{
				Warning( "Unknown attribute '%s' in WaveSpawn definition.\n", name );
			}
		}

		// These allow us to avoid rounding errors later when divvying money to bots
		m_unallocatedCurrency = m_totalCurrency;
		m_remainingCount = m_totalCount;
	}

	return true;
}


//-----------------------------------------------------------------------
void CWaveSpawnPopulator::OnPlayerKilled( CTFPlayer *corpse )
{
	m_activeVector.FindAndFastRemove( corpse );
}


//-----------------------------------------------------------------------
bool CWaveSpawnPopulator::IsFinishedSpawning( void )
{
	if ( m_bSupportWave && !m_bLimitedSupport )
	{
		// support waves are never done spawning until
		// we get OnNonSupportWavesDone called
		return false;
	}

	return ( m_countSpawnedSoFar >= m_totalCount );
}


//-----------------------------------------------------------------------
void CWaveSpawnPopulator::SetState( InternalStateType eState )
{
	m_state = eState;

	switch( m_state )
	{
	case PENDING:
	case PRE_SPAWN_DELAY:
	case SPAWNING:
		break;
	case WAIT_FOR_ALL_DEAD:
		// last spawn has occurred
		if ( m_lastSpawnWarningSound.Length() > 0 )
		{
			TFGameRules()->BroadcastSound( 255, m_lastSpawnWarningSound );
		}

		FireEvent( m_lastSpawnOutput, "LastSpawnOutput" );

		if ( tf_populator_debug.GetBool() )
		{
			DevMsg( "%3.2f: WaveSpawn(%s) started WAIT_FOR_ALL_DEAD\n", gpGlobals->curtime, m_name.IsEmpty() ? "" : m_name.Get() );
		}
		break;
	case DONE:
		if ( m_doneWarningSound.Length() > 0 )
		{
			TFGameRules()->BroadcastSound( 255, m_doneWarningSound );
		}

		FireEvent( m_doneOutput, "DoneOutput" );

		if ( tf_populator_debug.GetBool() )
		{
			DevMsg( "%3.2f: WaveSpawn(%s) DONE\n", gpGlobals->curtime, m_name.IsEmpty() ? "" : m_name.Get() );
		}
		break;
	}
}


//-----------------------------------------------------------------------
void CWaveSpawnPopulator::OnNonSupportWavesDone( void )
{
	if ( m_bSupportWave )
	{
		switch( m_state )
		{
		case PENDING:
		case PRE_SPAWN_DELAY:
			SetState( DONE );
			break;
		case SPAWNING:
		case WAIT_FOR_ALL_DEAD:
			if ( TFGameRules() && ( m_unallocatedCurrency > 0 ) )
			{
				TFGameRules()->DistributeCurrencyAmount( m_unallocatedCurrency, NULL, true, true );
				m_unallocatedCurrency = 0;
			}
			SetState( WAIT_FOR_ALL_DEAD );
 		case DONE:
			break;
		}
	}
}


//-----------------------------------------------------------------------
int CWaveSpawnPopulator::GetCurrencyAmountPerDeath( void )
{
	int nCurrency = 0;
	
	if ( m_bSupportWave )
	{
		if ( m_state == WAIT_FOR_ALL_DEAD )
		{
			// we're still in the m_ActiveVector at this point so the number of players
			// in the vector is the division of the money since we're done spawning
			m_remainingCount = m_activeVector.Count();
		}
	}

	if ( m_unallocatedCurrency > 0 )
	{
		// We shouldn't be back in here if our remaining count is 0
		Assert ( m_remainingCount > 0 );

		// Band-aid for playtest
		m_remainingCount = m_remainingCount <= 0 ? 1 : m_remainingCount;

		nCurrency = m_unallocatedCurrency / m_remainingCount;
		m_unallocatedCurrency -= nCurrency;
		m_remainingCount--;
	}

	return nCurrency;
}

	
//-----------------------------------------------------------------------
// Continuously invoked to modify population over time
void CWaveSpawnPopulator::Update( void )
{
	VPROF_BUDGET( "CWaveSpawnPopulator::Update", "NextBot" );

	switch( m_state )
	{
	case DONE:
		return;

	case PENDING:
		m_timer.Start( m_waitBeforeStarting );
		SetState( PRE_SPAWN_DELAY );

		// zero this here to ensure it is cleared between Waves
		// since all WaveSpawns start at the same time at the beginning of a Wave
		m_reservedPlayerSlotCount = 0;

		if ( m_startWaveWarningSound.Length() > 0 )
		{
			TFGameRules()->BroadcastSound( 255, m_startWaveWarningSound );
		}

		FireEvent( m_startWaveOutput, "StartWaveOutput" );

		if ( tf_populator_debug.GetBool() )
		{
			DevMsg( "%3.2f: WaveSpawn(%s) started PRE_SPAWN_DELAY\n", gpGlobals->curtime, m_name.IsEmpty() ? "" : m_name.Get() );
		}
		break;

	case PRE_SPAWN_DELAY:
		if ( m_timer.IsElapsed() )
		{
			m_countSpawnedSoFar = 0;
			m_myReservedSlotCount = 0;
			SetState( SPAWNING );

			if ( m_firstSpawnWarningSound.Length() > 0 )
			{
				TFGameRules()->BroadcastSound( 255, m_firstSpawnWarningSound );
			}

			FireEvent( m_firstSpawnOutput, "FirstSpawnOutput" );

			if ( tf_populator_debug.GetBool() )
			{
				DevMsg( "%3.2f: WaveSpawn(%s) started SPAWNING\n", gpGlobals->curtime, m_name.IsEmpty() ? "" : m_name.Get() );
			}
		}
		break;

	case SPAWNING:
		if ( m_timer.IsElapsed() )
		{
			if( g_pPopulationManager->IsSpawningPaused() )
			{
				return;
			}

			if ( !m_spawner )
			{
				Warning( "Invalid spawner\n" );
				SetState( DONE );
				return;
			}

			// count up how many entities we've spawned are still active
			int currentActive = 0;
			for( int i=0; i<m_activeVector.Count(); ++i )
			{
				if ( m_activeVector[i] != NULL && m_activeVector[i]->IsAlive() )
				{
					++currentActive;
				}
			}

			if ( m_bWaitBetweenSpawnAfterDeath )
			{
				if ( currentActive == 0 )
				{
					if ( m_spawnLocationResult != SPAWN_LOCATION_NOT_FOUND )
					{
						// free up the current spawn area so we select a new one for the next group
						m_spawnLocationResult = SPAWN_LOCATION_NOT_FOUND;

						if ( m_waitBetweenSpawns > 0.0f )
						{
							// start delay
							m_timer.Start( m_waitBetweenSpawns );
						}

						// wait for the timer
						return;
					}
				}
				else
				{
					// wait until all current actives are dead
					return;
				}
			}

			if ( currentActive >= m_maxActive )
			{
				// we've reached our allowed cap
				return;
			}

			if ( m_myReservedSlotCount <= 0 )
			{
				// are there enough free slots?
				if ( ( m_maxActive - currentActive ) < m_spawnCount )
				{
					// not enough room to spawn a group so wait
					return;
				}

				int currentEnemyCount = GetGlobalTeam( TF_TEAM_PVE_INVADERS )->GetNumPlayers();

				if ( currentEnemyCount + m_spawnCount + m_reservedPlayerSlotCount > CPopulationManager::MVM_INVADERS_TEAM_SIZE )
				{
					// no space right now
					return;
				}

				// there is room - reserve our slots to ensure another concurrent WaveSpawn doesn't consume them
				m_reservedPlayerSlotCount += m_spawnCount;
				m_myReservedSlotCount = m_spawnCount;
			}

			bool bTeleported = ( m_spawnLocationResult == SPAWN_LOCATION_TELEPORTER );

			Vector vSpawnPosition = vec3_origin;
			if ( m_spawner && m_spawner->IsWhereRequired() )
			{
				// try to look for a spawn point or a new teleport location
				if ( m_spawnLocationResult == SPAWN_LOCATION_NOT_FOUND || m_spawnLocationResult == SPAWN_LOCATION_TELEPORTER )
				{
					m_spawnLocationResult = m_where.FindSpawnLocation( m_vSpawnPosition );
					if ( m_spawnLocationResult == SPAWN_LOCATION_NOT_FOUND )
					{
						// try again
						return;
					}
				}

				vSpawnPosition = m_vSpawnPosition;
				bTeleported = ( m_spawnLocationResult == SPAWN_LOCATION_TELEPORTER );

				// reset m_pCurrentSpawnArea if we want to pick a new spawn area for the next bot to spawn
				if ( m_bRandomSpawn )
				{
					m_spawnLocationResult = SPAWN_LOCATION_NOT_FOUND;
				}
			}

			EntityHandleVector_t m_justSpawnedVector;
			if ( m_spawner && m_spawner->Spawn( vSpawnPosition, &m_justSpawnedVector ) )
			{
				// successfully spawned

				FOR_EACH_VEC( m_justSpawnedVector, i )
				{
					if ( m_justSpawnedVector[i].Get() == NULL )
						continue;

					CTFBot *bot = ToTFBot( m_justSpawnedVector[i] );
					if ( bot )
					{
						bot->SetCustomCurrencyWorth( 0 );
						bot->SetWaveSpawnPopulator( this );

						// Allows client UI to know if a specific spawner is active
						TFObjectiveResource()->SetMannVsMachineWaveClassActive( bot->GetPlayerClass()->GetClassIconName() );

						if ( IsLimitedSupportWave() )
						{
							bot->MarkAsLimitedSupportEnemy();
						}

						// what bot should do after spawning at teleporter exit
						if ( bTeleported )
						{
							OnBotTeleported( bot );
						}
					}
					else
					{
						CTFTankBoss *tank = dynamic_cast< CTFTankBoss * >( m_justSpawnedVector[i].Get() );
						if ( tank )
						{
							tank->SetCurrencyValue( 0 );
							tank->SetWaveSpawnPopulator( this );

							m_pParent->IncrementTanksSpawned();
						}
					}
				}

				int justSpawnedCount = m_justSpawnedVector.Count();

				m_countSpawnedSoFar += justSpawnedCount;

				// release our reserved slots
				int slotsToReleaseCount = ( justSpawnedCount <= m_myReservedSlotCount ) ? justSpawnedCount : m_myReservedSlotCount;
				m_myReservedSlotCount -= slotsToReleaseCount;
				m_reservedPlayerSlotCount -= slotsToReleaseCount;

				// somehow, duplicate entries can end up in m_activeVector if we just AddVectorToTail() - look into this
				for( int i = 0 ; i < m_justSpawnedVector.Count() ; ++i )
				{
					CBaseEntity *newEntity = m_justSpawnedVector[i];

					for( int j = 0 ; j < m_activeVector.Count() ; ++j )
					{
						if ( m_activeVector[j] == NULL )
							continue;

						if ( m_activeVector[j]->entindex() == newEntity->entindex() )
						{
							Warning( "WaveSpawn duplicate entry in active vector\n" );
							continue;
						}
					}

					m_activeVector.AddToTail( newEntity );
				}

				if ( IsFinishedSpawning() )
				{
					SetState( WAIT_FOR_ALL_DEAD );
					return;
				}

				// successfully spawned a group of SpawnCount entities
				if ( m_myReservedSlotCount <= 0 && !m_bWaitBetweenSpawnAfterDeath )
				{
					// free up the current spawn area so we select a new one for the next group
					m_spawnLocationResult = SPAWN_LOCATION_NOT_FOUND;
					
					if ( m_waitBetweenSpawns > 0.0f )
					{
						// start delay
						m_timer.Start( m_waitBetweenSpawns );
					}
				}

				// not done yet
				return;
			}

			// couldn't spawn - retry soon
			m_timer.Start( 1.0f );
		}
		break;

	case WAIT_FOR_ALL_DEAD:
		FOR_EACH_VEC( m_activeVector, i )
		{
			if ( m_activeVector[i] != NULL && m_activeVector[i]->IsAlive() )
			{
				// not done yet
				return;
			}
		}

		// everyone we spawned is dead
		SetState( DONE );
		break;

	} // switch

	// not done yet
}


//-------------------------------------------------------------------------
// End CWaveSpawnPopulator
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
// CWave
//-------------------------------------------------------------------------
CWave::CWave( CPopulationManager *manager ) : IPopulator( manager )
{
	m_iEnemyCount = 0;
	m_nTanksSpawned = 0;
	m_nSentryBustersSpawned = 0;
	m_nNumEngineersTeleportSpawned = 0;
	m_nNumSentryBustersKilled = 0;
	m_totalCurrency = 0;
	m_waitWhenDone = 0.0f;
	m_isStarted = false;
	m_bFiredInitWaveOutput = false;
	m_startOutput = NULL;
	m_doneOutput = NULL;
	m_initOutput = NULL;
	m_bCheckBonusCreditsMin = true;
	m_bCheckBonusCreditsMax = true;
	m_bPlayedUpgradeAlert = false;
	m_flBonusCreditsTime = 0;
	m_isEveryContainedWaveSpawnDone = false;
	m_flStartTime = 0;

	m_doneTimer.Invalidate();
}

//-------------------------------------------------------------------------
CWave::~CWave()
{
	delete m_startOutput;
	delete m_doneOutput;
	delete m_initOutput;
	m_waveSpawnVector.PurgeAndDeleteElements();
}

//-------------------------------------------------------------------------
bool CWave::Parse( KeyValues *data )
{
	m_iEnemyCount = 0;
	m_nWaveClassCounts.RemoveAll();
	m_totalCurrency = 0;

	FOR_EACH_SUBKEY( data, kvWave )
	{
		if ( !Q_stricmp( kvWave->GetName(), "WaveSpawn" ) )
		{
			CWaveSpawnPopulator *wavePopulator = new CWaveSpawnPopulator( GetManager() );

			if ( wavePopulator->Parse( kvWave ) == false )
			{
				Warning( "Error reading WaveSpawn definition\n" );
				return false;
			}

			m_waveSpawnVector.AddToTail( wavePopulator );

			if ( !wavePopulator->IsSupportWave() )
			{
				// this is a total of all enemies we have to fight that are NOT support enemies
				m_iEnemyCount += wavePopulator->m_totalCount;
			}
			m_totalCurrency += wavePopulator->m_totalCurrency;

			wavePopulator->SetParent( this );

			if ( wavePopulator->m_spawner )
			{
				if ( wavePopulator->m_spawner->IsVarious() )
				{
					for ( int i = 0; i < wavePopulator->m_totalCount; ++i )
					{
						unsigned int iFlags = wavePopulator->IsSupportWave() ? MVM_CLASS_FLAG_SUPPORT : MVM_CLASS_FLAG_NORMAL;
						if ( wavePopulator->m_spawner->IsMiniBoss( i ) )
						{
							iFlags |= MVM_CLASS_FLAG_MINIBOSS;
						}
						if ( wavePopulator->m_spawner->HasAttribute( CTFBot::ALWAYS_CRIT, i ) )
						{
							iFlags |= MVM_CLASS_FLAG_ALWAYSCRIT;
						}
						if ( wavePopulator->IsLimitedSupportWave() )
						{
							iFlags |= MVM_CLASS_FLAG_SUPPORT_LIMITED;
						}
						AddClassType( wavePopulator->m_spawner->GetClassIcon( i ), 1, iFlags );
					}
				}
				else
				{
					unsigned int iFlags = wavePopulator->IsSupportWave() ? MVM_CLASS_FLAG_SUPPORT : MVM_CLASS_FLAG_NORMAL;
					if ( wavePopulator->m_spawner->IsMiniBoss() )
					{
						iFlags |= MVM_CLASS_FLAG_MINIBOSS;
					}
					if ( wavePopulator->m_spawner->HasAttribute( CTFBot::ALWAYS_CRIT ) )
					{
						iFlags |= MVM_CLASS_FLAG_ALWAYSCRIT;
					}
					if ( wavePopulator->IsLimitedSupportWave() )
					{
						iFlags |= MVM_CLASS_FLAG_SUPPORT_LIMITED;
					}
					AddClassType( wavePopulator->m_spawner->GetClassIcon(), wavePopulator->m_totalCount, iFlags );
				}
			}
		}				
		else if ( !Q_stricmp( kvWave->GetName(), "Sound" ) )
		{
			m_soundName.sprintf( "%s", kvWave->GetString() );
		}
		else if ( !Q_stricmp( kvWave->GetName(), "Description" ) )
		{
			m_description.sprintf( "%s", kvWave->GetString() );
		}
		else if ( !Q_stricmp( kvWave->GetName(), "WaitWhenDone" ) )
		{
			m_waitWhenDone = kvWave->GetFloat();
		}
		else if ( !Q_stricmp( kvWave->GetName(), "Checkpoint" ) )
		{
			//m_isCheckpoint = true;
		}
		else if ( !Q_stricmp( kvWave->GetName(), "StartWaveOutput" ) )
		{
			m_startOutput = ParseEvent( kvWave );
		}
		else if ( !Q_stricmp( kvWave->GetName(), "DoneOutput" ) )
		{
			m_doneOutput = ParseEvent( kvWave );
		}
		else if ( !Q_stricmp( kvWave->GetName(), "InitWaveOutput" ) )
		{
			m_initOutput = ParseEvent( kvWave );
		}
		else
		{
			Warning( "Unknown attribute '%s' in Wave definition.\n", kvWave->GetName() );
		}
	}

	return true;
}
//-------------------------------------------------------------------------
// If we are the currently active wave, update all contained WaveSpawns. 
void CWave::Update( void )
{
	VPROF_BUDGET( "CWave::Update", "NextBot" );

	if ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
	{
		ActiveWaveUpdate();
	}
	else if ( TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS || TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		WaveIntermissionUpdate();
	}

	// is the wave done?
	if ( m_isEveryContainedWaveSpawnDone && TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
	{
		if ( GetManager()->IsBonusRound() && GetManager()->GetBonusBoss() && GetManager()->GetBonusBoss()->IsAlive() )
		{
			return;
		}
		WaveCompleteUpdate();
	}	
}


//-------------------------------------------------------------------------
void CWave::OnPlayerKilled( CTFPlayer *corpse )
{
	for( int i=0; i<m_waveSpawnVector.Count(); ++i )
	{
		m_waveSpawnVector[i]->OnPlayerKilled( corpse );
	}
}


//-------------------------------------------------------------------------
bool CWave::HasEventChangeAttributes( const char* pszEventName ) const
{
	for ( int i=0; i<m_waveSpawnVector.Count(); ++i )
	{
		if ( m_waveSpawnVector[i]->HasEventChangeAttributes( pszEventName ) )
		{
			return true;
		}
	}

	return false;
}


//-------------------------------------------------------------------------
void CWave::ForceFinish()
{
	FOR_EACH_VEC( m_waveSpawnVector, i )
	{
		m_waveSpawnVector[i]->ForceFinish();
	}
}


//-------------------------------------------------------------------------
void CWave::ForceReset()
{
	m_isStarted = false;
	m_bFiredInitWaveOutput = false;
	m_flBonusCreditsTime = 0;
	m_isEveryContainedWaveSpawnDone = false;
	m_flStartTime = 0;

	m_doneTimer.Invalidate();

	FOR_EACH_VEC( m_waveSpawnVector, i )
	{
		m_waveSpawnVector[i]->ForceReset();
	}
}

//-------------------------------------------------------------------------
CWaveSpawnPopulator *CWave::FindWaveSpawnPopulator( const char *name )
{
	FOR_EACH_VEC( m_waveSpawnVector, i )
	{
		CWaveSpawnPopulator *waveSpawnPopulator = m_waveSpawnVector[i];
		if ( !Q_stricmp( waveSpawnPopulator->m_name.Get(), name ) )
		{
			return waveSpawnPopulator;
		}
	}

	return NULL;
}

//-------------------------------------------------------------------------
void CWave::AddClassType( string_t iszClassIconName, int nCount, unsigned int iFlags )
{
	int nIndex = -1;
	for ( int nClass = 0; nClass < m_nWaveClassCounts.Count(); ++nClass )
	{
		if ( ( m_nWaveClassCounts[ nClass ].iszClassIconName == iszClassIconName ) && ( m_nWaveClassCounts[ nClass ].iFlags & iFlags ) )
		{
			nIndex = nClass;
			break;
		}
	}

	if ( nIndex == -1 )
	{
		nIndex = m_nWaveClassCounts.AddToTail();
		m_nWaveClassCounts[ nIndex ].iszClassIconName = iszClassIconName;
		m_nWaveClassCounts[ nIndex ].nClassCount = 0;
		m_nWaveClassCounts[ nIndex ].iFlags = MVM_CLASS_FLAG_NONE;
	}

	m_nWaveClassCounts[ nIndex ].nClassCount += nCount;
	m_nWaveClassCounts[ nIndex ].iFlags |= iFlags;
}

//-------------------------------------------------------------------------
// Private
//-------------------------------------------------------------------------
bool CWave::IsDoneWithNonSupportWaves( void )
{
	FOR_EACH_VEC( m_waveSpawnVector, i )
	{
		CWaveSpawnPopulator *waveSpawnPopulator = m_waveSpawnVector[i];
		if ( waveSpawnPopulator )
		{
			if ( !waveSpawnPopulator->IsSupportWave() && !waveSpawnPopulator->IsDone() )
				return false;
		}
	}

	return true;
}

//-------------------------------------------------------------------------
void CWave::ActiveWaveUpdate( void )
{
	VPROF_BUDGET( "CWave::ActiveWaveUpdate", "NextBot" );

	if ( !m_isStarted )
	{
		// Delay the start of the next wave
		if ( GetManager()->IsInEndlessWaves() && m_flStartTime > gpGlobals->curtime )
			return;

		// wave just started
		m_isStarted = true;

		FireEvent( m_startOutput, "StartWaveOutput" );

		if ( m_soundName.Length() > 0 )
		{
			TFGameRules()->BroadcastSound( 255, m_soundName );
		}

		GetManager()->AdjustMinPlayerSpawnTime();
	}

	m_isEveryContainedWaveSpawnDone = true;

	if ( GetManager()->IsBonusRound() )
	{
		return;
	}

	// update each contained WaveSpawn
	FOR_EACH_VEC( m_waveSpawnVector, i )
	{
		CWaveSpawnPopulator *waveSpawnPopulator = m_waveSpawnVector[i];
		bool bWaiting = false;

		// check if this WaveSpawn is waiting for another WaveSpawn to be done spawning players
		if ( !waveSpawnPopulator->m_waitForAllSpawned.IsEmpty() )
		{
			char *name = waveSpawnPopulator->m_waitForAllSpawned.GetForModify();
			FOR_EACH_VEC( m_waveSpawnVector, j )
			{
				CWaveSpawnPopulator *predecessor = m_waveSpawnVector[j];
				if ( predecessor && !Q_stricmp( predecessor->m_name.Get(), name ) )
				{
					if ( !predecessor->IsDoneSpawningBots() )
					{
						bWaiting = true;
						break;
					}
				}
			}
		}

		if ( !bWaiting )
		{
			// check if this WaveSpawn is waiting for another WaveSpawn's players to all be dead
			if ( !waveSpawnPopulator->m_waitForAllDead.IsEmpty() )
			{
				const char *name = waveSpawnPopulator->m_waitForAllDead.Get();
				FOR_EACH_VEC( m_waveSpawnVector, j )
				{
					CWaveSpawnPopulator *predecessor = m_waveSpawnVector[j];
					if ( predecessor && !Q_stricmp( predecessor->m_name.Get(), name ) )
					{
						if ( !predecessor->IsDone() )
						{
							bWaiting = true;
							break;
						}
					}
				}
			}
		}

		if ( bWaiting )
		{
			continue;
		}

		waveSpawnPopulator->Update();

		m_isEveryContainedWaveSpawnDone &= waveSpawnPopulator->IsDone();
	}

	if ( IsDoneWithNonSupportWaves() )
	{
		// Loop through and tell all the WaveSpawns
		FOR_EACH_VEC( m_waveSpawnVector, i )
		{
			CWaveSpawnPopulator *waveSpawnPopulator = m_waveSpawnVector[i];
			waveSpawnPopulator->OnNonSupportWavesDone();
		}

		for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
		{
			// Now let's kill everyone left on the attacking team
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( pPlayer && pPlayer->IsAlive() && 
				 ( ( pPlayer->GetTeamNumber() == TF_TEAM_PVE_INVADERS ) || pPlayer->m_Shared.InCond( TF_COND_REPROGRAMMED ) ) )
			{
				pPlayer->CommitSuicide( true, false );
			}
		}
	}
}

//-------------------------------------------------------------------------
void CWave::WaveCompleteUpdate( void )
{
	bool bHasTank = NumTanksSpawned() >= 1;

	FireEvent( m_doneOutput, "DoneOutput" );

	bool bLastWave = ( GetManager()->GetWaveNumber() + 1 ) >= GetManager()->GetTotalWaveCount();
	bool bMidWave = ( GetManager()->GetWaveNumber() + 1 ) >= ( GetManager()->GetTotalWaveCount() / 2 );
	bool bAdvancedPopfile = ( g_pPopulationManager ? g_pPopulationManager->IsAdvancedPopFile() : false );

	IGameEvent *event = gameeventmanager->CreateEvent( "mvm_wave_complete" );
	if ( event )
	{
		event->SetBool( "advanced", bAdvancedPopfile );
		gameeventmanager->FireEvent( event );
	}

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		if ( bAdvancedPopfile )
		{
			CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
			if ( pMaster && ( pMaster->GetNumPoints() > 0 ) )
			{
				if ( pMaster->GetNumPointsOwnedByTeam( TF_TEAM_PVE_DEFENDERS ) == pMaster->GetNumPoints() )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "mvm_adv_wave_complete_no_gates" );
					if ( event )
					{
						event->SetInt( "index", GetManager()->GetWaveNumber() );
						gameeventmanager->FireEvent( event );
					}
				}
			}
		}
	}

	if ( bLastWave && !GetManager()->IsInEndlessWaves() )
	{
		GetManager()->MvMVictory();

		if ( TFGameRules() )
		{
			if ( GTFGCClientSystem()->GetMatch() && GTFGCClientSystem()->GetMatch()->m_eMatchGroup == k_eTFMatchGroup_MvM_MannUp )
			{
				TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Manned_Up_Wave_End" );
			}
			else
			{
				TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Final_Wave_End" );
			}

			TFGameRules()->BroadcastSound( 255, "music.mvm_end_last_wave" );
		}

		event = gameeventmanager->CreateEvent( "mvm_mission_complete" );
		if ( event )
		{
			event->SetString( "mission", GetManager()->GetPopulationFilename() );
			gameeventmanager->FireEvent( event );
		}
	}
	else
	{
		if ( TFGameRules() )
		{
			TFGameRules()->BroadcastSound( 255,"Announcer.MVM_Wave_End" );

			if( bHasTank )
			{
				TFGameRules()->BroadcastSound( 255, "music.mvm_end_tank_wave" );
			}
			else if( bMidWave )
			{
				TFGameRules()->BroadcastSound( 255, "music.mvm_end_mid_wave" );
			}
			else
			{
				TFGameRules()->BroadcastSound( 255, "music.mvm_end_wave" );
			}
		}
	}

	CBroadcastRecipientFilter filter;
	filter.MakeReliable();
	UserMessageBegin( filter, "MVMAnnouncement" );
		WRITE_CHAR( TF_MVM_ANNOUNCEMENT_WAVE_COMPLETE );
		WRITE_CHAR( GetManager()->GetWaveNumber() );
	MessageEnd();

	if ( TFObjectiveResource() )
	{
		// if we're using a timer between waves...
		if ( !g_pPopulationManager->GetWavesUseReadyBetween() )
		{
			if ( !m_doneTimer.HasStarted() )
			{
				m_doneTimer.Start( m_waitWhenDone );
			}

			TFObjectiveResource()->SetMannVsMachineNextWaveTime( gpGlobals->curtime + m_waitWhenDone );
		}

		// Force respawn dead defenders
		CUtlVector< CTFPlayer * > playerVector;
		CollectPlayers( &playerVector, TF_TEAM_PVE_DEFENDERS );
		FOR_EACH_VEC( playerVector, i )
		{
			if ( !playerVector[i]->IsAlive() )
			{
				playerVector[i]->ForceRespawn();
			}

			// clear player's accumulated sentry damage
			playerVector[i]->ResetAccumulatedSentryGunDamageDealt();
			playerVector[i]->ResetAccumulatedSentryGunKillCount();
		}		
	}

	GetManager()->WaveEnd( true );
}

//-------------------------------------------------------------------------
void CWave::WaveIntermissionUpdate ( void )
{
	if ( !m_bFiredInitWaveOutput )
	{
		FireEvent( m_initOutput, "InitWaveOutput" );

		m_bFiredInitWaveOutput = true;
	}

	if ( m_GetUpgradesAlertTimer.HasStarted() && m_GetUpgradesAlertTimer.IsElapsed() )
	{
		// Monitor for full wave currency collection bonus
		if ( ( m_bCheckBonusCreditsMin || m_bCheckBonusCreditsMax ) && gpGlobals->curtime > m_flBonusCreditsTime )
		{
			int nWaveNum = GetManager()->GetWaveNumber() - 1;
			int nDropped = MannVsMachineStats_GetDroppedCredits( nWaveNum );
			int nAcquired = MannVsMachineStats_GetAcquiredCredits( nWaveNum, false );
			float flRatioCollected = clamp( ( (float)nAcquired / (float)nDropped ), 0.1f, 1.f );

			float flMinBonus = tf_mvm_currency_bonus_ratio_min.GetFloat();
			float flMaxBonus = tf_mvm_currency_bonus_ratio_max.GetFloat();

			Assert( flMinBonus <= flMaxBonus );
			if ( flMinBonus > flMaxBonus )
				flMinBonus = flMaxBonus;

			// Max bonus
			if ( m_bCheckBonusCreditsMax && nDropped > 0 && flRatioCollected >= flMaxBonus )
			{
				int nAmount = (float)TFGameRules()->CalculateCurrencyAmount_ByType( TF_CURRENCY_WAVE_COLLECTION_BONUS ) * 0.5f;
				TFGameRules()->DistributeCurrencyAmount( nAmount, NULL, true, false, true );

				TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Bonus" );
				IGameEvent *event = gameeventmanager->CreateEvent( "mvm_creditbonus_wave" );
				if ( event )
				{
					gameeventmanager->FireEvent( event );
				}

				m_bCheckBonusCreditsMax = false;
				m_GetUpgradesAlertTimer.Reset();
			}
			// Min bonus
			if ( m_bCheckBonusCreditsMin && nDropped > 0 && flRatioCollected >= flMinBonus )
			{
				int nAmount = (float)TFGameRules()->CalculateCurrencyAmount_ByType( TF_CURRENCY_WAVE_COLLECTION_BONUS ) * 0.5f;
				TFGameRules()->DistributeCurrencyAmount( nAmount, NULL, true, false, true );

				TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Bonus" );
				IGameEvent *event = gameeventmanager->CreateEvent( "mvm_creditbonus_wave" );
				if ( event )
				{
					gameeventmanager->FireEvent( event );
				}

				m_bCheckBonusCreditsMin = false;
			}
			else if ( !m_bPlayedUpgradeAlert )
			{
				TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Get_To_Upgrade" );

				m_bPlayedUpgradeAlert = true;
				m_GetUpgradesAlertTimer.Reset();
			}

			m_flBonusCreditsTime = gpGlobals->curtime + 0.25f;
		}
	}

	// When we use a timer between waves, start it here
	if ( m_doneTimer.HasStarted() && m_doneTimer.IsElapsed() )
	{
		m_doneTimer.Invalidate();
		GetManager()->StartCurrentWave();
	}
}
//-------------------------------------------------------------------------
// End CWave
//-------------------------------------------------------------------------
