//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_raid_logic.cpp
// Raid game mode singleton manager
// Michael Booth, November 2009

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "team.h"
#include "nav_pathfind.h"
#include "tf_gamerules.h"
#include "team_control_point_master.h"
#include "bot/tf_bot.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "raid/tf_raid_logic.h"
#include "bot_npc/bot_npc_minion.h"
#include "tf_obj_sentrygun.h"
#include "filesystem.h"

#include "func_respawnroom.h"
#include "pathtrack.h"

extern ConVar mp_teams_unbalance_limit;
extern ConVar mp_autoteambalance;
extern ConVar sv_alltalk;
extern ConVar mp_timelimit;

CRaidLogic *g_pRaidLogic = NULL;

ConVar tf_debug_sniper_spots( "tf_debug_sniper_spots", "0"/*, FCVAR_CHEAT*/ );


ConVar tf_raid_max_wanderers( "tf_raid_max_wanderers", "10"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_max_defense_engineers( "tf_raid_max_defense_engineers", "6"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_max_defense_demomen( "tf_raid_max_defense_demomen", "1"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_max_defense_heavies( "tf_raid_max_defense_heavies", "1"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_max_defense_soldiers( "tf_raid_max_defense_soldiers", "1"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_max_defense_pyros( "tf_raid_max_defense_pyros", "1"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_max_defense_spies( "tf_raid_max_defense_spies", "1"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_max_defense_snipers( "tf_raid_max_defense_snipers", "3"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_max_defense_squads( "tf_raid_max_defense_squads", "1"/*, FCVAR_CHEAT*/ );


ConVar tf_raid_wandering_density( "tf_raid_wandering_density", "0.00001", FCVAR_CHEAT, "Wandering defenders per unit area" );
ConVar tf_raid_spawn_wanderers( "tf_raid_spawn_wanderers", "1"/*, FCVAR_CHEAT*/ );

ConVar tf_raid_defender_density( "tf_raid_defender_density", "0.000001", 0/*FCVAR_CHEAT*/, "Wandering defenders per unit area" );
ConVar tf_raid_max_defender_count( "tf_raid_max_defender_count", "18", 0/*FCVAR_CHEAT*/ );
ConVar tf_raid_spawn_defenders( "tf_raid_spawn_defenders", "1"/*, FCVAR_CHEAT*/ );


ConVar tf_raid_sentry_density( "tf_raid_sentry_density", "0.0000005", 0/*FCVAR_CHEAT*/, "Sentry guns per unit area" );
ConVar tf_raid_sentry_spacing( "tf_raid_sentry_spacing", "750", 0/*FCVAR_CHEAT*/, "Minimum travel distance between sentry gun spots" );
ConVar tf_raid_debug_sentry_placement( "tf_raid_debug_sentry_placement", "0"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_spawn_sentries( "tf_raid_spawn_sentries", "1"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_spawn_engineers( "tf_raid_spawn_engineers", "0"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_engineer_spawn_interval( "tf_raid_engineer_spawn_interval", "20"/*, FCVAR_CHEAT*/ );

ConVar tf_raid_mob_spawn_min_interval( "tf_raid_mob_spawn_min_interval", "60"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_mob_spawn_max_interval( "tf_raid_mob_spawn_max_interval", "90"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_mob_spawn_count( "tf_raid_mob_spawn_count", "15"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_mob_spawn_below_tolerance( "tf_raid_mob_spawn_below_tolerance", "150"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_mob_spawn_min_range( "tf_raid_mob_spawn_min_range", "1000"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_spawn_mobs( "tf_raid_spawn_mobs", "1"/*, FCVAR_CHEAT*/ );

ConVar tf_raid_spawn_mob_as_squad_chance_start( "tf_raid_spawn_mob_as_squad_chance_start", "100" ); // /*, FCVAR_CHEAT*/ );
ConVar tf_raid_spawn_mob_as_squad_chance_halfway( "tf_raid_spawn_mob_as_squad_chance_halfway", "100" ); // /*, FCVAR_CHEAT*/ );
ConVar tf_raid_spawn_mob_as_squad_chance_final( "tf_raid_spawn_mob_as_squad_chance_final", "100" ); // /*, FCVAR_CHEAT*/ );
ConVar tf_raid_squad_medic_intro_percent( "tf_raid_squad_medic_intro_percent", "0.5" ); // /*, FCVAR_CHEAT*/ );


ConVar tf_raid_capture_mob_interval( "tf_raid_capture_mob_interval", "20"/*, FCVAR_CHEAT*/ );

ConVar tf_raid_special_spawn_min_interval( "tf_raid_special_spawn_min_interval", "20"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_special_spawn_max_interval( "tf_raid_special_spawn_max_interval", "30"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_spawn_specials( "tf_raid_spawn_specials", "0"/*, FCVAR_CHEAT*/ );

ConVar tf_raid_sniper_spawn_ahead_incursion( "tf_raid_sniper_spawn_ahead_incursion", "6000"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_sniper_spawn_behind_incursion( "tf_raid_sniper_spawn_behind_incursion", "6000"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_sniper_spawn_max_range( "tf_raid_sniper_spawn_max_range", "6000"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_sniper_spawn_min_range( "tf_raid_sniper_spawn_min_range", "1000"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_show_escape_route( "tf_raid_show_escape_route", "0"/*, FCVAR_CHEAT*/ );

ConVar tf_raid_sentry_build_ahead_incursion( "tf_raid_sentry_build_ahead_incursion", "5000"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_sentry_build_behind_incursion( "tf_raid_sentry_build_behind_incursion", "-1000"/*, FCVAR_CHEAT*/ );

ConVar tf_raid_debug( "tf_raid_debug", "0"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_debug_escape_route( "tf_raid_debug_escape_route", "0"/*, FCVAR_CHEAT*/ );
ConVar tf_raid_debug_director( "tf_raid_debug_director", "0"/*, FCVAR_CHEAT*/ );

ConVar tf_raid_spawn_enable( "tf_raid_spawn_enable", "1"/*, FCVAR_CHEAT*/ );

extern ConVar tf_populator_active_buffer_range;


extern bool IsSpaceToSpawnHere( const Vector &where );


//--------------------------------------------------------------------------------------------------------
// Check actual line of sight to team
bool IsPlayerVisibleToTeam( CTFPlayer *subject, int teamIndex )
{
	CTeam *team = GetGlobalTeam( teamIndex );
	for( int t=0; t<team->GetNumPlayers(); ++t )
	{
		CTFPlayer *teamMember = (CTFPlayer *)team->GetPlayer(t);

		if ( !teamMember->IsAlive() )
			continue;

		CTFBot *bot = ToTFBot( teamMember );
		if ( bot && bot->HasAttribute( CTFBot::IS_NPC ) )
			continue;

		if ( teamMember->IsInFieldOfView( subject->EyePosition() ) )
		{
			if ( teamMember->IsLineOfSightClear( subject, CBaseCombatCharacter::IGNORE_ACTORS ) )
			{
				// visible to team
				return true;
			}
		}
	}

	return false;
}



//--------------------------------------------------------------------------------------------------------------
int GetAvailableRedSpawnSlots( void )
{
	int available = 0;

	// count dead bots we can re-use
	CTeam *deadTeam = GetGlobalTeam( TEAM_SPECTATOR );
	for( int i=0; i<deadTeam->GetNumPlayers(); ++i )
	{
		if ( !deadTeam->GetPlayer(i)->IsBot() )
			continue;

		// reuse this guy
		++available;
	}

	// count unused player slots
	int totalPlayerCount = 0;
	totalPlayerCount += GetGlobalTeam( TEAM_SPECTATOR )->GetNumPlayers();
	totalPlayerCount += 4; // always leave room for 4 blue players
	totalPlayerCount += GetGlobalTeam( TF_TEAM_RED )->GetNumPlayers();

	available += gpGlobals->maxClients - totalPlayerCount;

	return available;
}


//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
BEGIN_DATADESC( CRaidLogic )
	DEFINE_THINKFUNC( Update ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_logic_raid, CRaidLogic );


//-------------------------------------------------------------------------
CRaidLogic::CRaidLogic()
{
	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "teamplay_round_start" );
	m_didFailLastTime = false;
}


//-------------------------------------------------------------------------
CRaidLogic::~CRaidLogic()
{
	g_pRaidLogic = NULL;
}


//-------------------------------------------------------------------------
void CRaidLogic::Spawn( void )
{
	BaseClass::Spawn();

	Reset();

	SetThink( &CRaidLogic::Update );
	SetNextThink( gpGlobals->curtime );

	m_didFailLastTime = false;

	g_pRaidLogic = this;

	m_miniBossIndex = 0;
}


//-------------------------------------------------------------------------
void CRaidLogic::Reset( void )
{
	m_isWaitingForRaidersToLeaveSpawnRoom = true;
	m_wasCapturingPoint = false;
	m_mobSpawnTimer.Invalidate();
	m_mobLifetimeTimer.Invalidate();
	m_specialSpawnTimer.Invalidate();
	m_mobCountRemaining = 0;
	m_mobArea = NULL;
	m_mobClass = TF_CLASS_SCOUT;
	m_priorRaiderAliveCount = -1;
	m_farthestAlongRaider = NULL;
	m_farthestAlongEscapeRouteArea = NULL;
	m_incursionDistanceAtEnd = -1.0f;

	m_wandererCount = 0;
	m_engineerCount = 0;
	m_demomanCount = 0;
	m_heavyCount = 0;
	m_soldierCount = 0;
	m_pyroCount = 0;
	m_spyCount = 0;
	m_sniperCount = 0;
	m_squadCount = 0;

	m_miniBossIndex = 0;
}


#if 0
//--------------------------------------------------------------------------------------------------------
bool SpawnWanderer( const Vector &spot )
{
	if ( !tf_raid_spawn_wanderers.GetBool() )
		return false;

	return SpawnRedTFBot( TF_CLASS_SCOUT, spot ) ? true : false;

/*
	CBaseCombatCharacter *minion = static_cast< CBaseCombatCharacter * >( CreateEntityByName( "bot_npc_minion" ) );
	if ( minion )
	{
		minion->SetAbsOrigin( spot );
		minion->SetOwnerEntity( NULL );

		DispatchSpawn( minion );

		return true;
	}

	return false;
*/
}
#endif // 0


//-------------------------------------------------------------------------
void CRaidLogic::OnRoundStart( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsRaidMode() )
		return;

	Reset();

	// unspawn entire red team
	CTeam *defendingTeam = GetGlobalTeam( TF_TEAM_RED );
	int i;
	for( i=0; i<defendingTeam->GetNumPlayers(); ++i )
	{
		engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", defendingTeam->GetPlayer(i)->GetUserID() ) );
	}

	// remove all minions
	CBaseEntity *minion = NULL;
	while( ( minion = gEntList.FindEntityByClassname( minion, "bot_npc_minion" ) ) != NULL )
	{
		UTIL_Remove( minion );
	}

	// kick last round's NPCs
	CTeam *raidingTeam = GetGlobalTeam( TF_TEAM_BLUE );
	for( i=0; i<raidingTeam->GetNumPlayers(); ++i )
	{
		CTFBot *bot = ToTFBot( raidingTeam->GetPlayer(i) );
		if ( bot && bot->HasAttribute( CTFBot::IS_NPC ) )
		{
			engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", raidingTeam->GetPlayer(i)->GetUserID() ) );
		}
	}


	BuildEscapeRoute();

	// collect special areas
	m_sniperSpotVector.RemoveAll();
	m_sentrySpotVector.RemoveAll();
	m_rescueClosetVector.RemoveAll();

	m_miniBossHomeVector.RemoveAll();
	m_miniBossIndex = 0;

	float availableSentrySpotArea = 0.0f;
	for( i=0; i<TheNavAreas.Count(); ++i )
	{
		CTFNavArea *area = (CTFNavArea *)TheNavAreas[i];

		if ( area->HasAttributeTF( TF_NAV_SNIPER_SPOT ) )
			m_sniperSpotVector.AddToTail( area );

		if ( area->HasAttributeTF( TF_NAV_SENTRY_SPOT ) )
		{
			m_sentrySpotVector.AddToTail( area );
			availableSentrySpotArea += area->GetSizeX() * area->GetSizeY();
		}

		if ( area->HasAttributeTF( TF_NAV_RESCUE_CLOSET ) )
			m_rescueClosetVector.AddToTail( area );

		if ( area->HasAttributeTF( TF_NAV_RED_SETUP_GATE ) )
			m_miniBossHomeVector.AddToTail( area );
	}

	// compute total geometric area of entire nav mesh, and clear all wander counts
	float totalSpace = 0.0f;
	for( i=0; i<TheNavAreas.Count(); ++i )
	{
		CTFNavArea *area = (CTFNavArea *)TheNavAreas[ i ];

		totalSpace += area->GetSizeX() * area->GetSizeY();

		area->SetWanderCount( 0 );
	}

#if 0
	//----------------------------------------------
	// fill the world with wandering defenders
	int totalPopulation = (int)( tf_raid_wandering_density.GetFloat() * totalSpace + 0.5f );

	CUtlVector< CNavArea * > minionAreaVector;
	SelectSeparatedShuffleSet< CNavArea >( totalPopulation, tf_raid_sentry_spacing.GetFloat(), TheNavAreas, &minionAreaVector );

	for( int i=0; i<minionAreaVector.Count(); ++i )
	{
		static_cast< CTFNavArea * >( minionAreaVector[i] )->AddToWanderCount( 1 );
//		SpawnWanderer( minionAreaVector[i]->GetRandomPoint() );
	}

	DevMsg( "RAID: Total minion population = %d\n", minionAreaVector.Count() );


	//----------------------------------------------
	// determine where sentry guns will be

	// the total sentry population is based on the total actual space, not just sentry areas
	totalPopulation = (int)( tf_raid_sentry_density.GetFloat() * totalSpace + 0.5f );

	SelectSeparatedShuffleSet< CTFNavArea >( totalPopulation, tf_raid_sentry_spacing.GetFloat(), m_sentrySpotVector, &m_actualSentrySpotVector );

	for( int i=0; i<m_actualSentrySpotVector.Count(); ++i )
	{
		SpawnSentry( m_actualSentrySpotVector[i]->GetCenter() );
	}

	DevMsg( "RAID: Total sentry population = %d\n", m_actualSentrySpotVector.Count() );


	//----------------------------------------------
	// fill the world with defending bots
	if ( tf_raid_spawn_defenders.GetBool() )
	{
		const int classRosterCount = 5;
		int classRoster[ classRosterCount ] = { TF_CLASS_SNIPER, TF_CLASS_DEMOMAN, TF_CLASS_SNIPER, TF_CLASS_DEMOMAN, TF_CLASS_PYRO };

		CUtlVector< CTFNavArea * > validSpawnAreaVector;
		for( int i=0; i<TheNavAreas.Count(); ++i )
		{
			CTFNavArea *area = (CTFNavArea *)TheNavAreas[i];
			if ( area->IsValidForWanderingPopulation() && IsSpaceToSpawnHere( area->GetCenter() ) )
			{
				validSpawnAreaVector.AddToTail( area );
			}
		}

		totalPopulation = (int)( tf_raid_defender_density.GetFloat() * totalSpace + 0.5f );
		totalPopulation = clamp( totalPopulation, 0, tf_raid_max_defender_count.GetInt() );

		CUtlVector< CTFNavArea * > defenderAreaVector;
		SelectSeparatedShuffleSet< CTFNavArea >( totalPopulation, tf_raid_sentry_spacing.GetFloat(), validSpawnAreaVector, &defenderAreaVector );

		for( int i=0; i<defenderAreaVector.Count(); ++i )
		{
			CTFNavArea *homeArea = defenderAreaVector[i];

			CTFBot *bot = SpawnRedTFBot( classRoster[ i % classRosterCount ], homeArea->GetCenter() + Vector( 0, 0, 10.0f ) );
			if ( bot )
			{
				bot->SetHomeArea( homeArea );
			}
			else
			{
				DevMsg( "RAID: Failed to spawn defender!\n" );
			}
		}

		DevMsg( "RAID: Total defender population = %d\n", defenderAreaVector.Count() );
	}
#endif


	// collect all capture point gates
	m_gateVector.RemoveAll();
	CBaseEntity *entity = NULL;
	while( ( entity = gEntList.FindEntityByClassname( entity, "func_door*" ) ) != NULL )
	{
		CBaseDoor *door = (CBaseDoor *)entity;

		if ( door->GetEntityName() != NULL_STRING && V_stristr( STRING( door->GetEntityName() ), "raid" ) )
		{
			m_gateVector.AddToTail( door );
		}
	}
}


//--------------------------------------------------------------------------------------------------------
void CRaidLogic::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

	if ( !Q_strcmp( eventName, "teamplay_point_captured" ) )
	{
		// they just capped - give them a break and reset the mob spawner
		StartMobTimer( RandomFloat( tf_raid_mob_spawn_min_interval.GetFloat(), tf_raid_mob_spawn_max_interval.GetFloat() ) );
		DevMsg( "RAID: %3.2f: Reset Mob timer after successful point capture\n", gpGlobals->curtime );
	}
	else if ( !Q_strcmp( eventName, "teamplay_round_win" ) )
	{
		if ( event->GetInt( "team" ) == TF_TEAM_RED )
		{
			// the raiders didn't make it
			m_didFailLastTime = true;
		}
	}
	else if ( !Q_strcmp( eventName, "teamplay_round_start" ) )
	{
		OnRoundStart();
	}
}


//--------------------------------------------------------------------------------------------------------
int CompareIncursionDistances( CTFNavArea * const *area1, CTFNavArea * const *area2 )
{
	float d1 = (*area1)->GetIncursionDistance( TF_TEAM_BLUE );
	float d2 = (*area2)->GetIncursionDistance( TF_TEAM_BLUE );

	if ( d1 < d2 )
		return -1;

	if ( d1 > d2 )
		return 1;

	return 0;
}


#if 0
//--------------------------------------------------------------------------------------------------------
class CPopulator : public ISearchSurroundingAreasFunctor
{
public:
	CPopulator( float leaderIncursionRange, int maxWanderersToSpawn )
	{
		m_leaderIncursionRange = leaderIncursionRange;
		m_spaceLeft = maxWanderersToSpawn;

		m_floor = FLT_MAX;

		CTeam *invaderTeam = GetGlobalTeam( TF_TEAM_BLUE );
		for( int i=0; i<invaderTeam->GetNumPlayers(); ++i )
		{
			if ( !invaderTeam->GetPlayer(i)->IsAlive() )
				continue;

			if ( invaderTeam->GetPlayer(i)->GetAbsOrigin().z < m_floor )
			{
				m_floor = invaderTeam->GetPlayer(i)->GetAbsOrigin().z;
			}
		}

		m_floor -= tf_raid_mob_spawn_below_tolerance.GetFloat();
	}

	virtual bool operator() ( CNavArea *baseArea, CNavArea *priorArea, float travelDistanceSoFar )
	{
		CTFNavArea *area = (CTFNavArea *)baseArea;

		if ( area->IsBlocked( TF_TEAM_RED ) )
			return true;

		if ( area->HasAttributeTF( TF_NAV_NO_SPAWNING | TF_NAV_SPAWN_ROOM_BLUE ) )
			return true;

		if ( tf_raid_debug.GetInt() > 1 )
		{
			if ( area->IsPotentiallyVisibleToTeam( TF_TEAM_BLUE ) )
				area->DrawFilled( 255, 100, 0, 100, 1.0f );
			else
				area->DrawFilled( 0, 100, 255, 100, 1.0f );
		}

		// require minimum size
		if ( area->GetSizeX() < 45.0f && area->GetSizeY() < 45.0f )
			return true;

		// don't use areas far below team
// 		if ( area->GetCenter().z < m_floor )
// 			return true;

		if ( area->IsPotentiallyVisibleToTeam( TF_TEAM_BLUE ) )
		{
			// don't spawn wanderers in view of raiders
			// clear any unspawned wanderers here
			area->SetWanderCount( 0 );
			return true;
		}

		// collect out-of-sight areas
		m_hiddenAreaVector.AddToTail( area );

		// collect out-of-sight areas ahead of the team for special spawns
		const float aheadBuffer = 500.0f;
		if ( area->GetIncursionDistance( TF_TEAM_BLUE ) > m_leaderIncursionRange + aheadBuffer )
		{
			m_hiddenAreaAheadVector.AddToTail( area );
		}

		if ( m_spaceLeft <= 0 )
			return true;

		if ( !tf_raid_spawn_wanderers.GetBool() )
		{
			return true;
		}

		if ( TFGameRules()->GetRaidLogic()->IsMobSpawning() )
		{
			// don't spawn wanderers if a mob is spawning to keep slots free
			return true;
		}

		if ( !area->IsValidForWanderingPopulation() )
		{
			return true;
		}

		int maxSpawnCount = 5;
		while( area->GetWanderCount() > 0 && --maxSpawnCount && m_spaceLeft )
		{
			// attempt to spawn a wanderer here
			if ( SpawnWanderer( area->GetRandomPoint() + Vector( 0, 0, StepHeight ) ) )
			{
				area->SetWanderCount( area->GetWanderCount() - 1 );
				--m_spaceLeft;
			}
		}

		return true;
	}

	// return true if 'adjArea' should be included in the ongoing search
	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar ) 
	{
		CTFNavArea *area = (CTFNavArea *)adjArea;
		float incursionDistance = area->GetIncursionDistance( TF_TEAM_BLUE );

		return incursionDistance > m_leaderIncursionRange - tf_populator_active_buffer_range.GetFloat() &&
			   incursionDistance < m_leaderIncursionRange + tf_populator_active_buffer_range.GetFloat() &&
			   !adjArea->IsBlocked( TEAM_ANY );
	}

	virtual void PostSearch( void )
	{
		// collect the highest hidden & ahead areas
		float minZ = 999999.9f, maxZ = -999999.9f;

		int i;
		for( i=0; i<m_hiddenAreaAheadVector.Count(); ++i )
		{
			CTFNavArea *area = m_hiddenAreaAheadVector[i];
			float areaZ = area->GetCenter().z;

			if ( areaZ < minZ )
				minZ = areaZ;
			if ( areaZ > maxZ )
				maxZ = areaZ;
		}

		float floorZ = minZ + 0.7f * ( maxZ - minZ );

		for( i=0; i<m_hiddenAreaAheadVector.Count(); ++i )
		{
			CTFNavArea *area = m_hiddenAreaAheadVector[i];
			float areaZ = area->GetCenter().z;

			if ( areaZ > floorZ )
			{
				m_hiddenAreaAheadHighVector.AddToTail( area );
			}
		}

		// sort hidden areas by incursion distance
		m_hiddenAreaVector.Sort( CompareIncursionDistances );
	}


	float m_leaderIncursionRange;
	float m_floor;
	int m_spaceLeft;

	CUtlVector< CTFNavArea * > m_hiddenAreaVector;
	CUtlVector< CTFNavArea * > m_hiddenAreaAheadVector;
	CUtlVector< CTFNavArea * > m_hiddenAreaAheadHighVector;
};
#endif // 0


//--------------------------------------------------------------------------------------------------------
bool CRaidLogic::Unspawn( CTFPlayer *who )
{
	if ( who->IsAlive() && who->GetTeamNumber() == TF_TEAM_RED )
	{
		// only cull Engineers who are far behind the team
		if ( who->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			CTFNavArea *area = (CTFNavArea *)who->GetLastKnownArea();
			if ( area && area->GetIncursionDistance( TF_TEAM_BLUE ) > GetMaximumRaiderIncursionDistance() - tf_populator_active_buffer_range.GetFloat() )
				return false;
		}
		else if ( !who->IsPlayerClass( TF_CLASS_SCOUT ) )
 		{
 			// do not unspawn these classes - they lurk at far distances
 			return false;
		}
 	}

	// check actual line of sight to team
	if ( IsPlayerVisibleToTeam( who, TF_TEAM_BLUE ) )
		return false;

	who->ChangeTeam( TEAM_SPECTATOR, false, true );

	// destroy all buildings (for relocated engineers)
	who->RemoveAllObjects();

	return true;
}


//--------------------------------------------------------------------------------------------------------
CTFNavArea *CRaidLogic::FindSpawnAreaAhead( void )
{
	CTFPlayer *leader = GetFarthestAlongRaider();

	if ( leader == NULL || m_escapeRouteVector.Count() == 0 )
		return NULL;

	const float minTravel = 1000.0f;
	float minIncursion = GetMaximumRaiderIncursionDistance() + minTravel;

	// find first non-visible area ahead of leader along escape path beyond a minimum distance
	for( int i=0; i<m_escapeRouteVector.Count(); ++i )
	{
		CTFNavArea *area = m_escapeRouteVector[i];

		if ( area->IsBlocked( TF_TEAM_RED ) )
			return NULL;

		if ( area->HasAttributeTF( TF_NAV_NO_SPAWNING ) )
			continue;

		if ( area->GetIncursionDistance( TF_TEAM_BLUE ) < minIncursion )
			continue;

		if ( area->IsPotentiallyVisibleToTeam( TF_TEAM_BLUE ) )
			continue;

		if ( IsSpaceToSpawnHere( area->GetCenter() ) )
		{
			// found valid squad spawn
			return area;
		}
	}

	return NULL;
}


//--------------------------------------------------------------------------------------------------------
CTFNavArea *CRaidLogic::FindSpawnAreaBehind( void )
{
	CTFPlayer *leader = GetFarthestAlongRaider();

	if ( leader == NULL || m_escapeRouteVector.Count() == 0 )
		return NULL;

	const float minTravel = 1000.0f;
	float maxIncursion = GetMaximumRaiderIncursionDistance() - minTravel;

	// find first non-visible area behind leader along escape path beyond a minimum distance
	for( int i=m_escapeRouteVector.Count()-1; i >= 0; --i )
	{
		CTFNavArea *area = m_escapeRouteVector[i];

		if ( area->HasAttributeTF( TF_NAV_NO_SPAWNING ) )
			continue;

		if ( area->GetIncursionDistance( TF_TEAM_BLUE ) > maxIncursion )
			continue;

		if ( area->IsPotentiallyVisibleToTeam( TF_TEAM_BLUE ) )
			continue;

		if ( IsSpaceToSpawnHere( area->GetCenter() ) )
		{
			// found valid squad spawn
			return area;
		}
	}

	return NULL;
}


#if 0
//--------------------------------------------------------------------------------------------------------
bool CRaidLogic::SpawnSquad( CTFNavArea *spawnArea )
{
	if ( spawnArea == NULL )
		return false;

	int squadSize = 4;

	int freeSlots = GetAvailableRedSpawnSlots();
	if ( freeSlots < squadSize )
	{
		DevMsg( "RAID: %3.2f: *** Not enough free slots to spawn a squad\n", gpGlobals->curtime );
		return false;
	}

	const int squadClassMaxCount = 4;
	int squadClasses[ squadClassMaxCount ];
	int squadClassCount = 0;

	squadClasses[ squadClassCount++ ] = TF_CLASS_HEAVYWEAPONS;
	squadClasses[ squadClassCount++ ] = TF_CLASS_SOLDIER;
	squadClasses[ squadClassCount++ ] = TF_CLASS_PYRO;
	squadClasses[ squadClassCount++ ] = TF_CLASS_DEMOMAN;

	// randomly shuffle the class order
	int n = squadClassCount;
	while( n > 1 )
	{
		int k = RandomInt( 0, n-1 );
		n--;
		
		int tmp = squadClasses[n];
		squadClasses[n] = squadClasses[k];
		squadClasses[k] = tmp;
	}

	if ( GetMaximumRaiderIncursionDistance() > tf_raid_squad_medic_intro_percent.GetFloat() * GetIncursionDistanceAtEnd() )
	{
		// Medics join squads farther into the raid
		squadClasses[0] = TF_CLASS_MEDIC;
	}


	CTFBotSquad *squad = new CTFBotSquad;
	CTFBot *bot;

	DevMsg( "RAID: %3.2f: <<<< Spawning Squad >>>>\n", gpGlobals->curtime );

	for( int i=0; i<squadSize; ++i )
	{
		int which = squadClasses[ i ];

		bot = SpawnRedTFBot( which, spawnArea->GetCenter() );
		if ( !bot )
			return false;

		bot->JoinSquad( squad );

		DevMsg( "RAID: %3.2f: Squad member %s(%d)\n", gpGlobals->curtime, bot->GetPlayerName(), bot->entindex() );
	}

	IGameEvent* event = gameeventmanager->CreateEvent( "raid_spawn_squad" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	return true;
}
#endif // 0


//--------------------------------------------------------------------------------------------------------
void CRaidLogic::StartMobTimer( float duration )
{
	if ( IsMobSpawning() )
	{
		DevMsg( "RAID: %3.2f: Skipping mob spawn because an existing mob is still in progress\n", gpGlobals->curtime );
		return;
	}

	m_mobSpawnTimer.Start( duration );
	m_mobLifetimeTimer.Invalidate();
	m_mobCountRemaining = 0;
	m_mobArea = NULL;
}


#if 0
//--------------------------------------------------------------------------------------------------------
CTFNavArea *CRaidLogic::SelectMobSpawn( CUtlVector< CTFNavArea * > *spawnAreaVector, RelativePositionType where )
{
	if ( spawnAreaVector->Count() == 0 )
	{
		// use high-reliability locations
		CTFNavArea *spawnArea = FindSpawnAreaBehind();
		if ( spawnArea )
			return spawnArea;

		return NULL; // FindSpawnAreaAhead();
	}

	const int maxRetries = 5;
	CTFNavArea *spawnArea = NULL;
	CTFPlayer *farRaider = GetFarthestAlongRaider();

	if ( !farRaider )
		return NULL;

	for( int r=0; r<maxRetries; ++r )
	{
		int which = 0;
		switch( where )
		{
		case AHEAD:
			// areas are sorted from behind to ahead - weight the selection to choose ahead
			which = SkewedRandomValue() * spawnAreaVector->Count();
			break;

		case BEHIND:
			// areas are sorted from behind to ahead - weight the selection to choose behind
			which = ( 1.0f - SkewedRandomValue() ) * spawnAreaVector->Count();
			break;

		case ANYWHERE:
			// areas are sorted from behind to ahead - weight the selection to choose ahead
			which = RandomFloat( 0.0f, 1.0f ) * spawnAreaVector->Count();
			break;
		}

		if ( which == spawnAreaVector->Count() )
			--which;

		spawnArea = spawnAreaVector->Element( which );

		if ( ( farRaider->GetAbsOrigin() - spawnArea->GetCenter() ).IsLengthGreaterThan( tf_raid_mob_spawn_min_range.GetFloat() ) )
		{
			// well behaved spawn area
			return spawnArea;
		}
	}

	// return whatever we've found so far
	return spawnArea;
}


//--------------------------------------------------------------------------------------------------------
void CRaidLogic::SpawnMobs( CUtlVector< CTFNavArea * > *spawnAreaVector )
{
	if ( !tf_raid_spawn_mobs.GetBool() )
		return;

	if ( m_mobLifetimeTimer.HasStarted() )
	{
		if ( m_mobLifetimeTimer.IsElapsed() )
		{
			// time is up for this mob to spawn, clear the rest
			DevMsg( "RAID: %3.2f: Mob spawn lifetime is up. Mob count remaining unspawned: %d\n", gpGlobals->curtime, m_mobCountRemaining );
			m_mobLifetimeTimer.Invalidate();
			m_mobCountRemaining = 0;
			m_mobArea = NULL;

			// start next mob
			m_mobSpawnTimer.Start( RandomFloat( tf_raid_mob_spawn_min_interval.GetFloat(), tf_raid_mob_spawn_max_interval.GetFloat() ) );
		}

		// can't create another mob until this mob has expired
		return;
	}

	if ( m_mobSpawnTimer.HasStarted() && m_mobSpawnTimer.IsElapsed() && spawnAreaVector && spawnAreaVector->Count() > 0 )
	{
		// chance of mob changes as we progress through the map
		int mobChance;
		float progressRatio = GetMaximumRaiderIncursionDistance() / GetIncursionDistanceAtEnd();
		if ( progressRatio < 0.5f )
		{
			mobChance = tf_raid_spawn_mob_as_squad_chance_start.GetInt() + 2.0f * progressRatio * ( tf_raid_spawn_mob_as_squad_chance_halfway.GetInt() - tf_raid_spawn_mob_as_squad_chance_start.GetInt() );
		}
		else
		{
			mobChance = tf_raid_spawn_mob_as_squad_chance_halfway.GetInt() + 2.0f * ( progressRatio - 0.5f ) * ( tf_raid_spawn_mob_as_squad_chance_final.GetInt() - tf_raid_spawn_mob_as_squad_chance_halfway.GetInt() );
		}

		if ( RandomInt( 1, 100 ) <= mobChance )
		{
			// this mob is a squad
			CTFNavArea *spawnArea = SelectMobSpawn( spawnAreaVector, BEHIND ); // m_wasCapturingPoint ? ANYWHERE : BEHIND );

			if ( SpawnSquad( spawnArea ) )
			{
				StartMobTimer( RandomFloat( tf_raid_mob_spawn_min_interval.GetFloat(), tf_raid_mob_spawn_max_interval.GetFloat() ) );
			}
			else
			{
				// couldn't spawn - try again soon
				StartMobTimer( 1.0f );
				DevMsg( "RAID: %3.2f: No place to spawn Squad!\n", gpGlobals->curtime );
			}
			return;
		}

		// time to throw in a mob rush
		m_mobArea = SelectMobSpawn( spawnAreaVector, m_wasCapturingPoint ? ANYWHERE : BEHIND );
		if ( m_mobArea )
		{
			const int mobClassCount = 4;
			static int mobClassList[ mobClassCount ] = 
			{
				TF_CLASS_SCOUT,
				TF_CLASS_HEAVYWEAPONS,
				TF_CLASS_PYRO,
				TF_CLASS_SPY,
			};

			m_mobLifetimeTimer.Start( 7.0f );
			m_mobCountRemaining = tf_raid_mob_spawn_count.GetInt();
			m_mobClass = mobClassList[ RandomInt( 0, mobClassCount-1 ) ];
			DevMsg( "RAID: %3.2f: <<<< Creating mob! >>>>\n", gpGlobals->curtime );

			IGameEvent* event = gameeventmanager->CreateEvent( "raid_spawn_mob" );
			if ( event )
			{
				gameeventmanager->FireEvent( event );
			}
		}
		else
		{
			// couldn't spawn - try again soon
			StartMobTimer( 1.0f );
			DevMsg( "RAID: %3.2f: No place to spawn Mob!\n", gpGlobals->curtime );
		}
	}
}
#endif // 0


//--------------------------------------------------------------------------------------------------------
class CNearbyHiddenScan : public ISearchSurroundingAreasFunctor
{
public:
	CNearbyHiddenScan( void )
	{
		m_hiddenArea = NULL;
	}

	virtual bool operator() ( CNavArea *baseArea, CNavArea *priorArea, float travelDistanceSoFar )
	{
		CTFNavArea *area = (CTFNavArea *)baseArea;

		if ( area->GetIncursionDistance( TF_TEAM_BLUE ) < TFGameRules()->GetRaidLogic()->GetMaximumRaiderIncursionDistance() )
		{
			// defenders can't spawn in region under raider's control
			return true;
		}

		if ( !area->IsPotentiallyVisibleToTeam( TF_TEAM_BLUE ) && area->IsValidForWanderingPopulation() )
		{
			// found a hidden spot
			m_hiddenArea = area;
			return false;
		}

		return true;
	}

	CTFNavArea *m_hiddenArea;
};


//--------------------------------------------------------------------------------------------------------
//
// Choose unpopulated sentry area nearest the invaders
//
CTFNavArea *CRaidLogic::SelectRaidSentryArea( void ) const
{
	CTFNavArea *nearestSentryArea = NULL;
	float nearestSentryAreaIncDist = FLT_MAX;

	float invaderIncDist = GetMaximumRaiderIncursionDistance();
	float aheadLimit = invaderIncDist + tf_raid_sentry_build_ahead_incursion.GetFloat();
	float behindLimit = invaderIncDist - tf_raid_sentry_build_behind_incursion.GetFloat();

	// collect a vector of alive enemies
	CUtlVector< CTFPlayer * > enemyVector;
	CollectPlayers( &enemyVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );

	// check for unpopulated sentry areas in the active area set
	for( int i=0; i<m_actualSentrySpotVector.Count(); ++i )
	{
		CTFNavArea *sentryArea = m_actualSentrySpotVector[i];

		// is this area in play?
		if ( sentryArea->GetIncursionDistance( TF_TEAM_BLUE ) > aheadLimit )
			continue;

		if ( sentryArea->GetIncursionDistance( TF_TEAM_BLUE ) < behindLimit )
			continue;

		// is this area 'owned' by another, active, engineer?
		int e;
		for( e=0; e<enemyVector.Count(); ++e )
		{
			if ( !enemyVector[e]->IsBot() )
				continue;

			if ( !enemyVector[e]->IsPlayerClass( TF_CLASS_ENGINEER ) )
				continue;

			CTFBot *engineer = (CTFBot *)enemyVector[e];
			if ( engineer->GetHomeArea() && engineer->GetHomeArea()->GetID() == sentryArea->GetID() )
			{
				break;
			}
		}

		if ( e < enemyVector.Count() )
		{
			// another engineer is using this area
			continue;
		}

		// this is an unreserved sentry area in the active area set, keep the nearest one that is ahead of the team
		float incDist = sentryArea->GetIncursionDistance( TF_TEAM_BLUE );
		if ( incDist < nearestSentryAreaIncDist && incDist >= behindLimit )
		{
			nearestSentryArea = sentryArea;
			nearestSentryAreaIncDist = incDist;
		}
	}

	if ( nearestSentryArea )
	{
		// find a nearby non-visible spawn spot for the engineer to enter from
		CNearbyHiddenScan hide;
		const float hideRange = 1000.0f;
		SearchSurroundingAreas( nearestSentryArea, hide, hideRange );

		if ( !hide.m_hiddenArea )
		{
			DevMsg( "RAID: %3.2f: Can't find hidden area to spawn in engineer", gpGlobals->curtime );
			return NULL;
		}

		// actual spawn-in, hidden area is this area's parent
		nearestSentryArea->SetParent( hide.m_hiddenArea );

		return nearestSentryArea;
	}

	return NULL;
}


#if 0
//--------------------------------------------------------------------------------------------------------
void CRaidLogic::SpawnEngineers( void )
{
	if ( !tf_raid_spawn_engineers.GetBool() || !m_engineerSpawnTimer.IsElapsed() )
		return;

	m_engineerSpawnTimer.Start( tf_raid_engineer_spawn_interval.GetFloat() );

	int engineerCount = m_engineerCount;

	while( engineerCount < tf_raid_max_defense_engineers.GetInt() )
	{
		// parent of sentry area is hidden area where engineer spawns
		CTFNavArea *sentryArea = SelectRaidSentryArea();

		if ( !sentryArea )
		{
			// no available areas
			break;
		}

		const int maxTries = 10;
		int tryCount;
		for( tryCount=0; tryCount<maxTries; ++tryCount )
		{
			CTFBot *bot = SpawnRedTFBot( TF_CLASS_ENGINEER, sentryArea->GetParent()->GetCenter() + Vector( 0, 0, RandomFloat( 0.0f, 30.0f ) ) );
			if ( bot )
			{
				// engineer bot will move to the sentry area and build
				bot->SetHomeArea( sentryArea );
				++engineerCount;
				DevMsg( "RAID: %3.2f: Spawned engineer", gpGlobals->curtime );
				break;
			}
		}

		if ( tryCount == maxTries )
		{
			DevMsg( "RAID: %3.2f: Can't spawn engineer", gpGlobals->curtime );
			break;
		}
	}
}


//--------------------------------------------------------------------------------------------------------
void CRaidLogic::SpawnSpecials( CUtlVector< CTFNavArea * > *spawnAheadVector, CUtlVector< CTFNavArea * > *spawnAnywhereVector )
{
	// spawn specials
	if ( tf_raid_spawn_specials.GetBool() && m_specialSpawnTimer.HasStarted() && m_specialSpawnTimer.IsElapsed() )
	{
		// time to add in a "special"
		m_specialSpawnTimer.Start( RandomFloat( tf_raid_special_spawn_min_interval.GetFloat(), tf_raid_special_spawn_max_interval.GetFloat() ) );

		DevMsg( "RAID: %3.2f: <<<< Spawning Special >>>>\n", gpGlobals->curtime );

		const int specialClassCount = 8;
		int availableSpecialClassList[ specialClassCount ];
		int availableCount = 0;

		if ( m_sniperCount < tf_raid_max_defense_snipers.GetInt() )
		{
			// increased chance of a sniper
			availableSpecialClassList[ availableCount++ ] = TF_CLASS_SNIPER;
			availableSpecialClassList[ availableCount++ ] = TF_CLASS_SNIPER;
		}

		if ( m_demomanCount < tf_raid_max_defense_demomen.GetInt() )
		{
			availableSpecialClassList[ availableCount++ ] = TF_CLASS_DEMOMAN;
		}

		if ( m_heavyCount < tf_raid_max_defense_heavies.GetInt() )
		{
			availableSpecialClassList[ availableCount++ ] = TF_CLASS_HEAVYWEAPONS;
		}

		if ( m_soldierCount < tf_raid_max_defense_soldiers.GetInt() )
		{
			availableSpecialClassList[ availableCount++ ] = TF_CLASS_SOLDIER;
		}

		if ( m_pyroCount < tf_raid_max_defense_pyros.GetInt() )
		{
			availableSpecialClassList[ availableCount++ ] = TF_CLASS_PYRO;
		}

		if ( m_spyCount < tf_raid_max_defense_spies.GetInt() )
		{
			availableSpecialClassList[ availableCount++ ] = TF_CLASS_SPY;
		}

		if ( availableCount == 0 )
		{
			// nothing to spawn
			DevMsg( "RAID: %3.2f: All specials in play already.\n", gpGlobals->curtime );
			return;
		}

		int whichClass = availableSpecialClassList[ RandomInt( 0, availableCount-1 ) ];

		if ( whichClass == TF_CLASS_SNIPER )
		{
			CTFNavArea *homeArea = FindSniperSpawn();
			if ( homeArea )
			{
				// actual spawn-in, hidden area is this area's parent
				for( int tryCount=0; tryCount<10; ++tryCount )
				{
					CTFBot *bot = SpawnRedTFBot( whichClass, homeArea->GetParent()->GetCenter() + Vector( 0, 0, RandomFloat( 0.0f, 30.0f ) ) );
					if ( bot )
					{
						// Bot will move to his home area to do his business
						bot->SetHomeArea( homeArea );
						return;
					}					
				}
			}
		}
		else if ( spawnAheadVector && spawnAheadVector->Count() > 0 )
		{
			CTFNavArea *where = spawnAheadVector->Element( RandomInt( 0, spawnAheadVector->Count()-1 ) );

			CTFBot *bot = SpawnRedTFBot( whichClass, where->GetCenter() + Vector( 0, 0, StepHeight ) );
			if ( bot )
			{
				bot->SetHomeArea( where );
				return;
			}
		}

		// failed to create special - try again soon
		m_specialSpawnTimer.Start( RandomFloat( 1.0f, 2.0f ) );
		DevMsg( "RAID: %3.2f: Failed to spawn Special.\n", gpGlobals->curtime );
	}
}
#endif // 0


//--------------------------------------------------------------------------------------------------------
void CRaidLogic::CullObsoleteEnemies( float minIncursion, float maxIncursion )
{
return;

	// cull wanderers outside of the active area set - use slightly larger range to avoid thrashing
	CTeam *defenseTeam = GetGlobalTeam( TF_TEAM_RED );

	float cullMinIncursionDistance = minIncursion - 1.1f * tf_populator_active_buffer_range.GetFloat();
	float cullMaxIncursionDistance = maxIncursion + 1.1f * tf_populator_active_buffer_range.GetFloat();

	for( int i=0; i<defenseTeam->GetNumPlayers(); ++i )
	{
		if ( !defenseTeam->GetPlayer(i)->IsBot() )
			continue;

		CTFBot *defender = (CTFBot *)defenseTeam->GetPlayer(i);

		if ( !defender->IsAlive() )
			continue;

		CTFNavArea *defenderArea = (CTFNavArea *)defender->GetLastKnownArea();
		if ( defenderArea == NULL )
		{
			// bad placement, underground most likely
			Unspawn( defender );
		}
		else if ( !defender->IsMoving() || defender->GetLocomotionInterface()->IsStuck() )
		{
			if ( defenderArea->GetIncursionDistance( TF_TEAM_BLUE ) < cullMinIncursionDistance ||
				 defenderArea->GetIncursionDistance( TF_TEAM_BLUE ) > cullMaxIncursionDistance )
			{
				if ( Unspawn( defender ) )
				{
					if ( !defender->HasAttribute( CTFBot::AGGRESSIVE ) )
					{
						defenderArea->AddToWanderCount( 1 );
					}
				}
			}
		}
	}

	// aggressively cull wanderers if we need to spawn a mob and have no room
	if ( IsMobSpawning() && GetAvailableRedSpawnSlots() <= 0 )
	{
		// try to make room by removing an unseen wanderer
		for( int i=0; i<defenseTeam->GetNumPlayers(); ++i )
		{
			if ( !defenseTeam->GetPlayer(i)->IsBot() )
				continue;

			CTFBot *defender = (CTFBot *)defenseTeam->GetPlayer(i);

			if ( !defender->IsAlive() )
				continue;

			// only cull Scouts
			if ( !defender->IsPlayerClass( TF_CLASS_SCOUT ) )
				continue;

			// don't cull mob rushers
			if ( defender->HasAttribute( CTFBot::AGGRESSIVE ) )
				continue;

			// try to open up a slot
			if ( Unspawn( defender ) )
			{
				if ( defender->GetLastKnownArea() )
				{
					defender->GetLastKnownArea()->AddToWanderCount( 1 );
				}
				break;
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------
class CShowEscapeRoute : public ISearchSurroundingAreasFunctor
{
public:
	virtual bool operator() ( CNavArea *baseArea, CNavArea *priorArea, float travelDistanceSoFar )
	{
		CTFNavArea *area = (CTFNavArea *)baseArea;

		if ( area->HasAttributeTF( TF_NAV_ESCAPE_ROUTE ) )
		{
			area->DrawFilled( 100, 255, 255, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, false );
		}
		else if ( area->HasAttributeTF( TF_NAV_ESCAPE_ROUTE_VISIBLE ) )
		{
			area->DrawFilled( 100, 200, 100, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, false );
		}

		return true;
	}

	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar ) 
	{
		return travelDistanceSoFar < 3000.0f;
	}
};


//--------------------------------------------------------------------------------------------------------
void CRaidLogic::DrawDebugDisplay( float deltaT )
{
	// avoid Warning() spam from UTIL_GetListenServerHost when on a dedicated server
	if ( engine->IsDedicatedServer() )
		return;

	CBasePlayer *player = UTIL_GetListenServerHost();
	if ( player == NULL )
		return;

	if ( tf_raid_debug_director.GetBool() )
	{
		NDebugOverlay::ScreenText( 0.01f, 0.5f, CFmtStr( "Mob timer: %3.2f", m_mobSpawnTimer.GetRemainingTime() ), 255, 255, 0, 255, 0.33f );
		NDebugOverlay::ScreenText( 0.01f, 0.51f, CFmtStr( "Wanderers: %d", m_wandererCount ), 255, 255, 0, 255, 0.33f );
		NDebugOverlay::ScreenText( 0.01f, 0.52f, CFmtStr( "Engineers: %d", m_engineerCount ), 255, 255, 0, 255, 0.33f );
		NDebugOverlay::ScreenText( 0.01f, 0.53f, CFmtStr( "Demomen: %d", m_demomanCount ), 255, 255, 0, 255, 0.33f );
		NDebugOverlay::ScreenText( 0.01f, 0.54f, CFmtStr( "Heavies: %d", m_heavyCount ), 255, 255, 0, 255, 0.33f );
		NDebugOverlay::ScreenText( 0.01f, 0.55f, CFmtStr( "Soldiers: %d", m_soldierCount ), 255, 255, 0, 255, 0.33f );
		NDebugOverlay::ScreenText( 0.01f, 0.56f, CFmtStr( "Pyros: %d", m_pyroCount ), 255, 255, 0, 255, 0.33f );
		NDebugOverlay::ScreenText( 0.01f, 0.57f, CFmtStr( "Spies: %d", m_spyCount ), 255, 255, 0, 255, 0.33f );
		NDebugOverlay::ScreenText( 0.01f, 0.58f, CFmtStr( "Snipers: %d", m_sniperCount ), 255, 255, 0, 255, 0.33f );
		NDebugOverlay::ScreenText( 0.01f, 0.59f, CFmtStr( "Squads: %d", m_squadCount ), 255, 255, 0, 255, 0.33f );
		NDebugOverlay::ScreenText( 0.01f, 0.60f, CFmtStr( "Raider max inc range: %3.2f", GetMaximumRaiderIncursionDistance() ), 255, 255, 0, 255, 0.33f );
	}

	if ( tf_raid_show_escape_route.GetInt() == 2 )
	{
		if ( m_escapeRouteVector.Count() >= 2 )
		{
			int anchor=0;
			float closeRangeSq = FLT_MAX;
			for( int i=0; i<m_escapeRouteVector.Count(); ++i )
			{
				float rangeSq = ( m_escapeRouteVector[i]->GetCenter() - player->GetAbsOrigin() ).LengthSqr();
				if ( rangeSq < closeRangeSq )
				{
					closeRangeSq = rangeSq;
					anchor = i;
				}
			}

			int lo = MAX( 0, anchor-20 );
			int hi = MIN( m_escapeRouteVector.Count(), anchor+20 );

			for( int i=lo; i<hi-1; ++i )
			{
				NDebugOverlay::HorzArrow( m_escapeRouteVector[i]->GetCenter(), m_escapeRouteVector[i+1]->GetCenter(), 5.0f, 255, 0, 0, 255, true, deltaT );
				NDebugOverlay::Text( m_escapeRouteVector[i]->GetCenter(), CFmtStr( "%d", i ), true, deltaT );
			}
		}
	}
	else if ( tf_raid_show_escape_route.GetInt() == 1 )
	{
		CShowEscapeRoute show;
		SearchSurroundingAreas( player->GetLastKnownArea(), show );
	}

	if ( tf_raid_debug_sentry_placement.GetBool() )
	{
		for( int i=0; i<m_actualSentrySpotVector.Count(); ++i )
		{
			m_actualSentrySpotVector[i]->DrawFilled( 255, 155, 0, 255, deltaT, true );		
		}
	}
}


//--------------------------------------------------------------------------------------------------------
void CRaidLogic::Update( void )
{
	VPROF_BUDGET( "CRaidLogic::Update", "Game" );

	const float deltaT = 0.33f;

	SetNextThink( gpGlobals->curtime + deltaT );

	if ( !TFGameRules()->IsRaidMode() )
		return;

	DrawDebugDisplay( deltaT );

	CTeam *raidingTeam = GetGlobalTeam( TF_TEAM_BLUE );

	// total hack for testing mini-bosses
	if ( m_miniBossIndex == 0 && m_miniBossHomeVector.Count() > 0 )
	{
		CTFNavArea *bossHomeArea = m_miniBossHomeVector[0];

		CBaseEntity *miniBoss = CreateEntityByName( "bot_boss_mini_rockets" );
		if ( miniBoss )
		{
			miniBoss->SetAbsOrigin( bossHomeArea->GetCenter() );
			miniBoss->SetOwnerEntity( NULL );

			DispatchSpawn( miniBoss );
		}

		++m_miniBossIndex;
	}
	else if ( m_miniBossIndex == 1 && m_miniBossHomeVector.Count() > 1 )
	{
		if ( gEntList.FindEntityByClassname( NULL, "bot_boss_mini_rockets" ) == NULL )
		{
			CTFNavArea *bossHomeArea = m_miniBossHomeVector[1];

			CBaseEntity *miniBoss = CreateEntityByName( "bot_boss_mini_nuker" );
			if ( miniBoss )
			{
				miniBoss->SetAbsOrigin( bossHomeArea->GetCenter() );
				miniBoss->SetOwnerEntity( NULL );

				DispatchSpawn( miniBoss );
			}

			++m_miniBossIndex;
		}
	}


	if ( IsWaitingForRaidersToLeaveSafeRoom() )
	{
		// has anyone left?
		for( int i=0; i<raidingTeam->GetNumPlayers(); ++i )
		{
			CTFPlayer *player = (CTFPlayer *)raidingTeam->GetPlayer(i);

			if ( !player->IsAlive() || !player->GetLastKnownArea() )
				continue;

			// don't start until a HUMAN leaves the safe room
			if ( player->IsBot() )
				continue;

			CTFNavArea *area = (CTFNavArea *)player->GetLastKnownArea();

			if ( !area->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE ) )
			{
				// this Raider has left the spawn room - game on!
				m_isWaitingForRaidersToLeaveSpawnRoom = false;

				StartMobTimer( RandomFloat( 0.5f * tf_raid_mob_spawn_min_interval.GetFloat(), tf_raid_mob_spawn_max_interval.GetFloat() ) );
				m_specialSpawnTimer.Start( RandomFloat( 0.0f, tf_raid_special_spawn_min_interval.GetFloat() ) );

				DevMsg( "RAID: %3.2f: Raiders left the spawn room!\n", gpGlobals->curtime );
			}
		}
	}
	else
	{
		int aliveCount = 0;
		for( int i=0; i<raidingTeam->GetNumPlayers(); ++i )
		{
			CTFPlayer *player = ToTFPlayer( raidingTeam->GetPlayer(i) );

			CTFBot *bot = ToTFBot( player );
			if ( bot && bot->HasAttribute( CTFBot::IS_NPC ) )
				continue;

			if ( player->IsAlive() )
			{
				CTFNavArea *area = (CTFNavArea *)player->GetLastKnownArea();
				if ( !area || !area->HasAttributeTF( TF_NAV_RESCUE_CLOSET ) )
				{
					// only count raiders not in a rescue closet
					++aliveCount;
				}
			}
		}

		if ( m_priorRaiderAliveCount < 0 )
		{
			// just left the safe room
			if ( m_didFailLastTime )
			{
				TFGameRules()->BroadcastSound( 255, "Announcer.DontFailAgain" );
			}
			else
			{
				TFGameRules()->BroadcastSound( 255, "Announcer.AM_GameStarting04" );		// "Let the games begin!"
			}
		}
		else
		{
			if ( m_priorRaiderAliveCount > aliveCount )
			{
				// somebody died, warn the team
				switch( aliveCount )
				{
				case 3:	TFGameRules()->BroadcastSound( 255, "Announcer.RoundEnds3seconds" );	break;
				case 2: TFGameRules()->BroadcastSound( 255, "Announcer.RoundEnds2seconds" );	break;
				case 1:	TFGameRules()->BroadcastSound( 255, "Announcer.AM_LastManAlive01" );	break;
				}
			}
			else if ( m_priorRaiderAliveCount < aliveCount )
			{
				// someone has joined/respawned, let the team know
				switch( aliveCount )
				{
				case 4:	TFGameRules()->BroadcastSound( 255, "Announcer.RoundEnds4seconds" );	break;
				case 3:	TFGameRules()->BroadcastSound( 255, "Announcer.RoundEnds3seconds" );	break;
				case 2: TFGameRules()->BroadcastSound( 255, "Announcer.RoundEnds2seconds" );	break;
				}
			}
		}

		m_priorRaiderAliveCount = aliveCount;


		if ( TFGameRules() && !TFGameRules()->IsInWaitingForPlayers() )
		{
			// if all of the raiders die, they lose
			if ( aliveCount == 0 )
			{
				CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
				if ( pRules )
				{
					pRules->SetWinningTeam( TF_TEAM_RED, WINREASON_OPPONENTS_DEAD );
				}
			}
		}
	}


	// have the raiders begun capturing the next point?
	CTeamControlPoint *ctrlPoint = GetContestedPoint();
	bool isCapturingPoint = ( ctrlPoint && ctrlPoint->GetTeamCapPercentage( TF_TEAM_BLUE ) );

	// find maximum incursion distance
	if ( m_incursionDistanceAtEnd < 0.0f ) 
	{
		for( int i=0; i<TheNavAreas.Count(); ++i )
		{
			CTFNavArea *area = (CTFNavArea *)TheNavAreas[ i ];

			if ( area->GetIncursionDistance( TF_TEAM_BLUE ) > m_incursionDistanceAtEnd )
			{
				m_incursionDistanceAtEnd = area->GetIncursionDistance( TF_TEAM_BLUE );
			}
		}
	}

	// find incursion range that surrounds raider team
	float minIncursion = FLT_MAX, maxIncursion = -FLT_MAX;

	m_farthestAlongRaider = NULL;
	CTFPlayer *capturer = NULL;
	int i;

	for( i=0; i<raidingTeam->GetNumPlayers(); ++i )
	{
		CTFPlayer *player = (CTFPlayer *)raidingTeam->GetPlayer(i);

		if ( !player->IsAlive() || !player->GetLastKnownArea() )
			continue;

		CTFBot *bot = ToTFBot( player );
		if ( bot && bot->HasAttribute( CTFBot::IS_NPC ) )
			continue;

		if ( player->IsCapturingPoint() )
		{
			capturer = player;
		}

		CTFNavArea *area = (CTFNavArea *)player->GetLastKnownArea();

		float myIncursion = area->GetIncursionDistance( TF_TEAM_BLUE );

		if ( maxIncursion < myIncursion )
		{
			maxIncursion = myIncursion;
			m_farthestAlongRaider = player;
		}
		
		if ( minIncursion > myIncursion )
		{
			minIncursion = myIncursion;
		}
	}

	m_farthestAlongEscapeRouteArea = NULL;

	if ( !m_farthestAlongRaider )
		return;

	// watch for point capture events
	if ( !m_wasCapturingPoint && isCapturingPoint )
	{
		DevMsg( "RAID: %3.2f: Point capture STARTED!\n", gpGlobals->curtime );

		// UTIL_CenterPrintAll( CFmtStr( "*** %s started capturing the point! ***", capturer->GetPlayerName() ) );

		// spawn a mob immediately
		StartMobTimer( 0.1f );
	}
	else if ( m_wasCapturingPoint && !isCapturingPoint )
	{
		DevMsg( "RAID: %3.2f: Point capture STOPPED!\n", gpGlobals->curtime );
	}
	m_wasCapturingPoint = isCapturingPoint;

	// track escape route area nearest leader
	for( i=0; i<m_escapeRouteVector.Count(); ++i )
	{
		CTFNavArea *area = m_escapeRouteVector[i];

		if ( area->GetIncursionDistance( TF_TEAM_BLUE ) <= GetMaximumRaiderIncursionDistance() )
		{
			if ( m_farthestAlongEscapeRouteArea )
			{
				if ( m_farthestAlongEscapeRouteArea->GetIncursionDistance( TF_TEAM_BLUE ) < area->GetIncursionDistance( TF_TEAM_BLUE ) )
				{
					m_farthestAlongEscapeRouteArea = area;
				}
			}
			else
			{
				m_farthestAlongEscapeRouteArea = area;
			}
		}
	}

	if ( tf_raid_debug_escape_route.GetBool() && m_farthestAlongEscapeRouteArea )
	{
		m_farthestAlongEscapeRouteArea->DrawFilled( 255, 100, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER );
	}

	CullObsoleteEnemies( minIncursion, maxIncursion );

	// count defensive types
	CTeam *defenseTeam = GetGlobalTeam( TF_TEAM_RED );
	m_wandererCount = 0;
	m_engineerCount = 0;
	m_demomanCount = 0;
	m_heavyCount = 0;
	m_soldierCount = 0;
	m_pyroCount = 0;
	m_spyCount = 0;
	m_sniperCount = 0;
	m_squadCount = 0;

	CUtlVector< CTFBotSquad * > m_squadVector;

	for( i=0; i<defenseTeam->GetNumPlayers(); ++i )
	{
		if ( !defenseTeam->GetPlayer(i)->IsBot() )
			continue;

		CTFBot *defender = (CTFBot *)defenseTeam->GetPlayer(i);

		if ( !defender->IsAlive() )
			continue;

		if ( defender->GetSquad() )
		{
			if ( m_squadVector.Find( defender->GetSquad() ) == m_squadVector.InvalidIndex() )
			{
				m_squadVector.AddToTail( defender->GetSquad() );
				++m_squadCount;
			}
			continue;
		}

		if ( defender->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			++m_engineerCount;
		}
		else if ( defender->IsPlayerClass( TF_CLASS_DEMOMAN ) )
		{
			++m_demomanCount;
		}
		else if ( defender->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
		{
			++m_heavyCount;
		}
		else if ( defender->IsPlayerClass( TF_CLASS_SOLDIER ) )
		{
			++m_soldierCount;
		}
		else if ( defender->IsPlayerClass( TF_CLASS_PYRO ) )
		{
			++m_pyroCount;
		}
		else if ( defender->IsPlayerClass( TF_CLASS_SPY ) )
		{
			++m_spyCount;
		}
		else if ( defender->IsPlayerClass( TF_CLASS_SNIPER ) )
		{
			++m_sniperCount;
		}
		else if ( defender->IsPlayerClass( TF_CLASS_SCOUT ) )
		{
			if ( defender->HasAttribute( CTFBot::AGGRESSIVE ) )
				continue;

			++m_wandererCount;
		}
	}


	if ( tf_raid_spawn_enable.GetBool() == false )
		return;

#if 0
	// populate wanderers
	CPopulator populator( maxIncursion, tf_raid_max_wanderers.GetInt() - m_wandererCount );
	SearchSurroundingAreas( m_farthestAlongRaider->GetLastKnownArea(), populator );

	// if raiders are capturing a point, spawn mobs at a faster rate
	if ( isCapturingPoint && m_mobSpawnTimer.GetRemainingTime() > tf_raid_capture_mob_interval.GetFloat() )
	{
		StartMobTimer( tf_raid_capture_mob_interval.GetFloat() - 0.1f );
	}

	SpawnMobs( &populator.m_hiddenAreaVector );
	SpawnSpecials( &populator.m_hiddenAreaAheadVector, &populator.m_hiddenAreaVector );
	SpawnEngineers();


	// emit mob
	if ( IsMobSpawning() && m_mobCountRemaining && m_mobArea && tf_raid_spawn_mobs.GetBool() )
	{
		if ( m_mobArea->HasAttributeTF( TF_NAV_NO_SPAWNING ) )
		{
			// the mob is trying to spawn from an area that doesn't have room to spawn in bots, pick another
			DevMsg( "RAID: %3.2f: Mob attempting to spawn where there isn't room - retrying\n", gpGlobals->curtime );
			m_mobArea = SelectMobSpawn( &populator.m_hiddenAreaVector, BEHIND );
			if ( !m_mobArea )
			{
				DevMsg( "RAID: %3.2f: Can't find a mob spawn area!\n", gpGlobals->curtime );

				// try again soon
				StartMobTimer( 2.0f );
			}
		}

		if ( m_mobArea )
		{
			if ( SpawnRedTFBot( m_mobClass, m_mobArea->GetCenter() + Vector( 0, 0, StepHeight ), IS_MOB_RUSHER ) )
			{
				--m_mobCountRemaining;
				DevMsg( "RAID: %3.2f: Spawned mob member, %d to go\n", gpGlobals->curtime, m_mobCountRemaining );
			}
		}
	}
#endif // 0

	// block/unblock capture point gate doors
	// TODO: Do this more efficiently 
	for( i=0; i<m_gateVector.Count(); ++i )
	{
		CBaseDoor *door = m_gateVector[i];

		Extent doorExtent;
		doorExtent.Init( door );

		NavAreaCollector overlapAreas;
		TheNavMesh->ForAllAreasOverlappingExtent( overlapAreas, doorExtent );

		for( int b=0; b<overlapAreas.m_area.Count(); ++b )
		{
			CTFNavArea *area = (CTFNavArea *)overlapAreas.m_area[b];
			if ( door->m_toggle_state == TS_AT_TOP )
			{
				// open, not blocked
				area->ClearAttributeTF( TF_NAV_BLOCKED );
			}
			else
			{
				// not open, blocked
				area->SetAttributeTF( TF_NAV_BLOCKED );
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------
float CRaidLogic::GetMaximumRaiderIncursionDistance( void ) const
{
	if ( m_farthestAlongRaider == NULL )
		return 0.0f;

	CTFNavArea *area = (CTFNavArea *)m_farthestAlongRaider->GetLastKnownArea();

	return area ? area->GetIncursionDistance( TF_TEAM_BLUE ) : 0.0f;
}


//--------------------------------------------------------------------------------------------------------
/**
 * Given a viewing area, return the earliest escape route area near the team that is visible
 */
CTFNavArea *CRaidLogic::FindEarliestVisibleEscapeRouteAreaNearTeam( CTFNavArea *viewArea ) const
{
	if ( viewArea == NULL || m_farthestAlongEscapeRouteArea == NULL )
		return NULL;

	const float nearIncursionRange = 1000.0f;
	const float minIncursion = GetMaximumRaiderIncursionDistance() - nearIncursionRange;
	const float maxIncursion = GetMaximumRaiderIncursionDistance() + nearIncursionRange;

	NavAreaCollector collector;
	viewArea->ForAllCompletelyVisibleAreas( collector );

	CTFNavArea *firstArea = NULL;
	float firstAreaIncursion = FLT_MAX;

	for( int i=0; i<collector.m_area.Count(); ++i )
	{
		CTFNavArea *area = (CTFNavArea *)collector.m_area[i];
		float areaIncursion = area->GetIncursionDistance( TF_TEAM_BLUE );

		if ( area->HasAttributeTF( TF_NAV_ESCAPE_ROUTE ) && areaIncursion > minIncursion && areaIncursion < maxIncursion )
		{
			if ( tf_debug_sniper_spots.GetBool() )
			{
				area->DrawFilled( 255, 0, 255, 255, 120.0f );
			}

/*
			float rangeSq = ( viewArea->GetCenter() - area->GetCenter() ).LengthSqr();

			if ( firstAreaIncursion > rangeSq )
			{
				// closest
				firstAreaIncursion = rangeSq;
				firstArea = area;
			}
*/

			if ( firstAreaIncursion > areaIncursion )
			{
				firstArea = area;
				firstAreaIncursion = areaIncursion;
			}
		}
	}

	return firstArea;
}


//--------------------------------------------------------------------------------------------------------
/**
 * Return area where sniper should take up a vantage point. That area's parent will be
 * the actual spawn-in area not currently visible to the team.
 */
CTFNavArea *CRaidLogic::FindSniperSpawn( void )
{
	CUtlVector< CTFNavArea * > validAreas;

	float aheadLimit = GetMaximumRaiderIncursionDistance() + tf_raid_sniper_spawn_ahead_incursion.GetFloat();
	float behindLimit = GetMaximumRaiderIncursionDistance() - tf_raid_sniper_spawn_behind_incursion.GetFloat();

	float tooCloseLimitSq = tf_raid_sniper_spawn_min_range.GetFloat();
	tooCloseLimitSq *= tooCloseLimitSq;

	for( int i=0; i<m_sniperSpotVector.Count(); ++i )
	{
		CTFNavArea *sniperArea = m_sniperSpotVector[i];

		if ( sniperArea->GetIncursionDistance( TF_TEAM_BLUE ) < 0.0f )
			continue;

		if ( sniperArea->GetIncursionDistance( TF_TEAM_BLUE ) > aheadLimit )
			continue;

		if ( sniperArea->GetIncursionDistance( TF_TEAM_BLUE ) < behindLimit )
			continue;

		// make sure no Raider is too close
		CClosestTFPlayer close( sniperArea->GetCenter(), TF_TEAM_BLUE );
		ForEachPlayer( close );

		if ( close.m_closePlayer && close.m_closeRangeSq < tooCloseLimitSq )
			continue;

		// this is often too restrictive - we really want "potentially visible to region near team"
		if ( !sniperArea->IsPotentiallyVisibleToTeam( TF_TEAM_BLUE ) )
		{
			if ( tf_debug_sniper_spots.GetBool() )
				sniperArea->DrawFilled( 100, 100, 100, 255, 99999.9f );

			continue;
		}

		validAreas.AddToTail( sniperArea );
	}

	if ( validAreas.Count() )
	{
		// choose a specific sniper spot
		CTFNavArea *sniperArea = validAreas[ RandomInt( 0, validAreas.Count()-1 ) ];

		// find a nearby non-visible spawn spot
		CNearbyHiddenScan hide;
		const float hideRange = 1000.0f;
		SearchSurroundingAreas( sniperArea, hide, hideRange );

		if ( hide.m_hiddenArea )
		{
			sniperArea->SetParent( hide.m_hiddenArea );

			if ( tf_debug_sniper_spots.GetBool() )
			{
				const float deltaT = 999999.9f;
				hide.m_hiddenArea->DrawFilled( 0, 0, 255, 255, deltaT );
				NDebugOverlay::HorzArrow( hide.m_hiddenArea->GetCenter() + Vector( 0, 0, 10.0f ), sniperArea->GetCenter() + Vector( 0, 0, 10.0f ), 5.0f, 255, 255, 0, 255, true, deltaT );
				sniperArea->DrawFilled( 255, 0, 0, 255, deltaT );

				for( int i=0; i<validAreas.Count(); ++i )
				{
					if ( validAreas[i] != sniperArea && validAreas[i] != hide.m_hiddenArea )
						validAreas[i]->DrawFilled( 0, 255, 0, 255, 99999.9f );
				}
			}

			return sniperArea;
		}
	}

	return NULL;
}


//--------------------------------------------------------------------------------------------------------
/**
 * Return a good area for a Sentry Gun, and return a hidden area for the engineer
 * to spawn as the parent of that area.
 */
CTFNavArea *CRaidLogic::FindSentryArea( void )
{
	CUtlVector< CTFNavArea * > validAreas;

	float aheadLimit = GetMaximumRaiderIncursionDistance() + tf_raid_sentry_build_ahead_incursion.GetFloat();
	float behindLimit = GetMaximumRaiderIncursionDistance() - tf_raid_sentry_build_behind_incursion.GetFloat();

	for( int i=0; i<m_sentrySpotVector.Count(); ++i )
	{
		CTFNavArea *sentryArea = m_sentrySpotVector[i];

		if ( sentryArea->GetIncursionDistance( TF_TEAM_BLUE ) < 0.0f )
			continue;

		if ( sentryArea->GetIncursionDistance( TF_TEAM_BLUE ) > aheadLimit )
			continue;

		if ( sentryArea->GetIncursionDistance( TF_TEAM_BLUE ) < behindLimit )
			continue;

		// don't use this area if it already has a sentry in it
		if ( TheTFNavMesh()->IsSentryGunHere( sentryArea ) )
			continue;

		validAreas.AddToTail( sentryArea );
	}

	if ( validAreas.Count() )
	{
		// choose a specific sentry spot
		CTFNavArea *sentryArea = validAreas[ RandomInt( 0, validAreas.Count()-1 ) ];

		// find a nearby non-visible spawn spot for the engineer
		CNearbyHiddenScan hide;
		const float hideRange = 1000.0f;
		SearchSurroundingAreas( sentryArea, hide, hideRange );

		if ( hide.m_hiddenArea )
		{
			sentryArea->SetParent( hide.m_hiddenArea );
			return sentryArea;
		}
	}

	return NULL;
}


//---------------------------------------------------------------------------------------------
// Pick a member of the raiding (blue) team for a red defender to attack
CTFPlayer *CRaidLogic::SelectRaiderToAttack( void )
{
	CTeam *invaderTeam = GetGlobalTeam( TF_TEAM_BLUE );

	// attack point cappers first
	CUtlVector< CTFPlayer * > victimVector;
	int i;
	for( i=0; i<invaderTeam->GetNumPlayers(); ++i )
	{
		CTFPlayer *player = (CTFPlayer *)invaderTeam->GetPlayer(i);

		if ( player->IsAlive() && player->IsCapturingPoint() )
			victimVector.AddToTail( player );
	}

	if ( victimVector.Count() == 0 )
	{
		// pick a random living Raider
		for( i=0; i<invaderTeam->GetNumPlayers(); ++i )
		{
			CTFPlayer *player = (CTFPlayer *)invaderTeam->GetPlayer(i);

			CTFBot *bot = ToTFBot( player );
			if ( bot && bot->HasAttribute( CTFBot::IS_NPC ) )
				continue;

			if ( player->IsAlive() )
				victimVector.AddToTail( player );
		}
	}

	if ( victimVector.Count() )
	{
		return victimVector[ RandomInt( 0, victimVector.Count()-1 ) ];
	}

	return NULL;
}


//--------------------------------------------------------------------------------------------------------
// Return entity positioned within next valid rescue closet area for to respawn players in
CBaseEntity *CRaidLogic::GetRescueRespawn( void ) const
{
	if ( g_internalSpawnPoint == NULL )
	{
		g_internalSpawnPoint = (CPopulatorInternalSpawnPoint *)CreateEntityByName( "populator_internal_spawn_point" );
		g_internalSpawnPoint->Spawn();
	}

	float limit = GetMaximumRaiderIncursionDistance() - 500.0f;

	CTFNavArea *rescueArea = NULL;
	float rescueFlow = FLT_MAX;

	for( int i=0; i<m_rescueClosetVector.Count(); ++i )
	{
		float flow = m_rescueClosetVector[i]->GetIncursionDistance( TF_TEAM_BLUE );
		if ( flow > limit && flow < rescueFlow )
		{
			rescueArea = m_rescueClosetVector[i];
			rescueFlow = flow;
		}
	}

	if ( rescueArea )
	{
		g_internalSpawnPoint->SetAbsOrigin( rescueArea->GetCenter() );
		g_internalSpawnPoint->SetLocalAngles( vec3_angle );

		return g_internalSpawnPoint;
	}

	return NULL;
}


//--------------------------------------------------------------------------------------------------------
class CMarkEscapeRoute
{
public:
	CMarkEscapeRoute( CUtlVector< CTFNavArea * > *escapeRouteVector )
	{
		m_escapeRouteVector = escapeRouteVector;
	}

	void operator() ( CNavArea *baseArea )
	{
		CTFNavArea *area = (CTFNavArea *)baseArea;

		if ( !m_escapeRouteVector->HasElement( area ) )
		{
			area->SetAttributeTF( TF_NAV_ESCAPE_ROUTE );
			m_escapeRouteVector->AddToTail( area );
		}
	}

	CUtlVector< CTFNavArea * > *m_escapeRouteVector;
};


//--------------------------------------------------------------------------------------------------------
class CMarkEscapeRouteVisible
{
public:
	bool operator() ( CNavArea *baseArea )
	{
		CTFNavArea *area = (CTFNavArea *)baseArea;

		area->SetAttributeTF( TF_NAV_ESCAPE_ROUTE_VISIBLE );

		return true;
	}
};


//---------------------------------------------------------------------------------------------
// Locate and store escape route
void CRaidLogic::BuildEscapeRoute( void )
{
	// find blue spawn room
	CBaseEntity *entity = NULL;
	for ( int i=0; i<IFuncRespawnRoomAutoList::AutoList().Count(); ++i )
	{
		CFuncRespawnRoom *respawnRoom = static_cast< CFuncRespawnRoom* >( IFuncRespawnRoomAutoList::AutoList()[i] );
		entity = respawnRoom;
		if ( respawnRoom->GetActive() && respawnRoom->GetTeamNumber() == TF_TEAM_BLUE )
		{
			break;
		}
	}

	if ( entity ) 
	{
		// respawn room absorigin is 0,0,0 - have to use extent
		Extent extent;
		extent.Init( entity );
		Vector safeRoomPos = ( extent.lo + extent.hi ) / 2;

		DevMsg( "RAID: Blue spawn room at (%g, %g, %g)\n", safeRoomPos.x, safeRoomPos.y, safeRoomPos.z );


		// assume path_track nearest the blue spawn is the start of the escape route
		// according to the mappers, previous pointers do not contain useful data
		CPathTrack *pathNode = NULL;
		CPathTrack *firstPathNode = NULL;
		float closeRangeSq = FLT_MAX;

		for( pathNode = dynamic_cast< CPathTrack * >( gEntList.FindEntityByClassname( pathNode, "path_track" ) );
			 pathNode;
			 pathNode = dynamic_cast< CPathTrack * >( gEntList.FindEntityByClassname( pathNode, "path_track" ) ) )
		{
			float rangeSq = ( pathNode->GetAbsOrigin() - safeRoomPos ).LengthSqr();

			if ( rangeSq < closeRangeSq )
			{
				closeRangeSq = rangeSq;
				firstPathNode = pathNode;
			}
		}

		if ( firstPathNode )
		{
			CUtlVector< CTFNavArea * > pathTrackAreaVector;

			for( pathNode = firstPathNode; pathNode; pathNode = pathNode->GetNext() )
			{
				CTFNavArea *pathArea = (CTFNavArea *)TheNavMesh->GetNavArea( pathNode->GetAbsOrigin() );
				if ( pathArea )
				{
					if ( !pathTrackAreaVector.HasElement( pathArea ) )
					{
						pathTrackAreaVector.AddToTail( pathArea );
					}
				}
			}

			if ( pathTrackAreaVector.Count() > 1 )
			{
				// build contiguous escape route area vector
				m_escapeRouteVector.RemoveAll();
				CMarkEscapeRoute markAsEscapeRoute( &m_escapeRouteVector );
				for( int i=1; i<pathTrackAreaVector.Count(); ++i )
				{
					// mark all areas between as on the escape route
					TheNavMesh->ForAllAreasAlongLine( markAsEscapeRoute, pathTrackAreaVector[i-1], pathTrackAreaVector[i] );
				}

				// flag all areas that can see the escape route as escape route 'visible'
				for( int i=0; i<m_escapeRouteVector.Count(); ++i )
				{
					CTFNavArea *escapeArea = m_escapeRouteVector[i];

					CMarkEscapeRouteVisible markAsEscapeRouteVisible;
					escapeArea->ForAllCompletelyVisibleAreas( markAsEscapeRouteVisible );
				}
			}
		}
	}
}


//---------------------------------------------------------------------------------------------
//
// Return the next control point that can be captured
//
CTeamControlPoint *CRaidLogic::GetContestedPoint( void ) const
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster )
	{
		for( int i=0; i<pMaster->GetNumPoints(); ++i )
		{
			CTeamControlPoint *point = pMaster->GetControlPoint( i );
			if ( point && pMaster->IsInRound( point ) )
			{
				if ( ObjectiveResource()->GetOwningTeam( point->GetPointIndex() ) == TF_TEAM_BLUE )
					continue;

				// blue are the invaders
				if ( !TeamplayGameRules()->TeamMayCapturePoint( TF_TEAM_BLUE, point->GetPointIndex() ) )
					continue;

				return point;
			}
		}
	}

	return NULL;
}


#endif // TF_RAID_MODE
