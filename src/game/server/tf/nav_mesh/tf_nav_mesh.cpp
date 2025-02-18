//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_nav_mesh.cpp
// TF specific nav mesh
// Michael Booth, February 2009

#include "cbase.h"
#include "tf_nav_mesh.h"
#include "bot/tf_bot.h"
#include "bot/tf_bot_manager.h"
#include "tf_obj.h"
#include "tf_obj_sentrygun.h"
#include "team_control_point_master.h"
#include "team_train_watcher.h"
#include "tf_gamerules.h"
#include "func_respawnroom.h"
#include "doors.h"
#include "props.h"
#include "filters.h"
#include "NextBotUtil.h"
#include "doors.h"
#include "props.h"
#include "BasePropDoor.h"

// NOTE: nav_debug_blocked ConVar is also use for debugging NAV_MESH_NAV_BLOCKER and TF_NAV_BLOCKED...

ConVar tf_show_in_combat_areas( "tf_show_in_combat_areas", "0", FCVAR_CHEAT );
ConVar tf_show_enemy_invasion_areas( "tf_show_enemy_invasion_areas", "0", FCVAR_CHEAT, "Highlight areas where the enemy team enters the visible environment of the local player" );
ConVar tf_show_blocked_areas( "tf_show_blocked_areas", "0", FCVAR_CHEAT, "Highlight areas that are considered blocked for TF-specific reasons" );
ConVar tf_show_incursion_flow( "tf_show_incursion_flow", "0", FCVAR_CHEAT );
ConVar tf_show_incursion_flow_range( "tf_show_incursion_flow_range", "150", FCVAR_CHEAT, "1 = red, 2 = blue" );
ConVar tf_show_incursion_flow_gradient( "tf_show_incursion_flow_gradient", "0", FCVAR_CHEAT, "1 = red, 2 = blue" );
ConVar tf_show_mesh_decoration( "tf_show_mesh_decoration", "0", FCVAR_CHEAT, "Highlight special areas" );
ConVar tf_show_mesh_decoration_manual( "tf_show_mesh_decoration_manual", "0", FCVAR_CHEAT, "Highlight special areas marked by hand" );
// Method 1 & 2 should be exactly the same for tf_show_sentry_danger.
ConVar tf_show_sentry_danger( "tf_show_sentry_danger", "0", FCVAR_CHEAT, "Show sentry danger areas. 1:Use m_sentryAreas. 2:Check all nav areas." );
ConVar tf_show_actor_potential_visibility( "tf_show_actor_potential_visibility", "0", FCVAR_CHEAT );
ConVar tf_show_control_points( "tf_show_control_points", "0", FCVAR_CHEAT );
ConVar tf_show_bomb_drop_areas( "tf_show_bomb_drop_areas", "0", FCVAR_CHEAT );

ConVar tf_bot_min_setup_gate_defend_range( "tf_bot_min_setup_gate_defend_range", "750", FCVAR_CHEAT, "How close from the setup gate(s) defending bots can take up positions. Areas closer than this will be in cover to ambush." );
ConVar tf_bot_max_setup_gate_defend_range( "tf_bot_max_setup_gate_defend_range", "2000", FCVAR_CHEAT, "How far from the setup gate(s) defending bots can take up positions" );
ConVar tf_bot_min_setup_gate_sniper_defend_range( "tf_bot_min_setup_gate_sniper_defend_range", "1500", FCVAR_CHEAT, "How far from the setup gate(s) a defending sniper will take up position" );
ConVar tf_show_gate_defense_areas( "tf_show_gate_defense_areas", "0", FCVAR_CHEAT );
ConVar tf_show_point_defense_areas( "tf_show_point_defense_areas", "0", FCVAR_CHEAT );


extern ConVar tf_bot_debug_select_defense_area;
extern ConVar tf_nav_in_combat_duration;
extern ConVar mp_teams_unbalance_limit;
extern ConVar mp_autoteambalance;
extern ConVar sv_alltalk;
extern ConVar mp_timelimit;


//--------------------------------------------------------------------------------------------------------------
ConVar tf_select_ambush_areas_radius( "tf_select_ambush_areas_radius", "750", FCVAR_CHEAT );
ConVar tf_select_ambush_areas_close_range( "tf_select_ambush_areas_close_range", "300", FCVAR_CHEAT );
ConVar tf_select_ambush_areas_max_enemy_exposure_area( "tf_select_ambush_areas_max_enemy_exposure_area", "500000", FCVAR_CHEAT );

class ScanSelectAmbushAreas
{
public:
	ScanSelectAmbushAreas( CUtlVector< CTFNavArea * > *ambushAreaVector, int teamToAmbush, float enemyIncursionLimit )
	{
		m_ambushAreaVector = ambushAreaVector;
		m_teamToAmbush = teamToAmbush;
		m_enemyIncursionLimit = enemyIncursionLimit;
	}

	bool operator() ( CNavArea *baseArea )
	{
		CTFNavArea *area = static_cast< CTFNavArea * >( baseArea );

		// no drop-downs or jumps
		if ( area->GetParent() && !area->GetParent()->IsContiguous( area ) )
			return false;

		float enemyIncursionDistanceAtArea = area->GetIncursionDistance( m_teamToAmbush );

		if ( enemyIncursionDistanceAtArea > m_enemyIncursionLimit )
			return false;

		int wallCount = 0;
		int dir;
		for( dir=0; dir<NUM_DIRECTIONS; ++dir )
		{
			if ( area->GetAdjacentCount( (NavDirType)dir ) == 0 )
			{
				// wall (or dropoff) on this side
				++wallCount;
			}
		}

		if ( wallCount >= 1 )
		{
			// good cover, are we also right next to enemy incursion areas?
			const CUtlVector< CTFNavArea * > &invasionVector = area->GetEnemyInvasionAreaVector( GetEnemyTeam( m_teamToAmbush ) );

			// don't use areas that are in plain sight of large amounts of incoming enemy space
			NavAreaCollector collector( true );
			area->ForAllPotentiallyVisibleAreas( collector );

			float totalVisibleThreatArea = 0.0f;
			FOR_EACH_VEC( collector.m_area, it )
			{
				CTFNavArea *visArea = static_cast< CTFNavArea * >( collector.m_area[ it ] );

				if ( visArea->GetIncursionDistance( m_teamToAmbush ) < enemyIncursionDistanceAtArea )
				{
					totalVisibleThreatArea += visArea->GetSizeX() * visArea->GetSizeY();
				}
			}

			if ( totalVisibleThreatArea > tf_select_ambush_areas_max_enemy_exposure_area.GetFloat() )
			{
				// too exposed
				return true;
			}

			float nearRangeSq = tf_select_ambush_areas_close_range.GetFloat();
			nearRangeSq *= nearRangeSq;

			FOR_EACH_VEC( invasionVector, it )
			{
				CTFNavArea *invasionArea = invasionVector[ it ];

				if ( invasionArea->GetIncursionDistance( m_teamToAmbush ) < enemyIncursionDistanceAtArea )
				{
					// the enemy will go through invasionArea before they reach the candidate area
					float rangeSq = ( invasionArea->GetCenter() - area->GetCenter() ).LengthSqr();
					if ( rangeSq < nearRangeSq )
					{
						// there is at least one nearby invasion area
						m_ambushAreaVector->AddToTail( area );
						break;
					}
				}
			}
		}

		return true;
	}

	int m_teamToAmbush;
	float m_enemyIncursionLimit;
	CUtlVector< CTFNavArea * > *m_ambushAreaVector;
};

void CMD_SelectAmbushAreas( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if ( player == NULL )
		return;

	CTFNavArea *searchSourceArea = static_cast< CTFNavArea * >( player->GetLastKnownArea() );

	int teamToAmbush = GetEnemyTeam( player->GetTeamNumber() );

	CUtlVector< CTFNavArea * > ambushAreaVector;
	ScanSelectAmbushAreas selector( &ambushAreaVector, teamToAmbush, searchSourceArea->GetIncursionDistance( teamToAmbush ) + 300.0f );
	SearchSurroundingAreas( searchSourceArea, searchSourceArea->GetCenter(), selector, tf_select_ambush_areas_radius.GetFloat() );

	FOR_EACH_VEC( ambushAreaVector, it )
	{
		TheNavMesh->AddToSelectedSet( ambushAreaVector[ it ] );
	}
}
static ConCommand tf_select_ambush_areas( "tf_select_ambush_areas", CMD_SelectAmbushAreas, "Add good ambush spots to the selected set. For debugging.", FCVAR_GAMEDLL | FCVAR_CHEAT );


#ifdef SKIPME
//-------------------------------------------------------------------------
void CMD_SelectIncursionZone( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if ( player == NULL )
		return;

	const CUtlVector< CTFNavArea * > *pointAreaVector = TheTFNavMesh()->GetControlPointAreas();
	if ( !pointAreaVector )
		return;

	int i;
	float incursionAtPoint = 0.0f;
	float maxInvaderTravelDistance = 2000.0f;

	for( i=0; i<pointAreaVector->Count(); ++i )
	{
		if ( pointAreaVector->Element(i)->GetIncursionDistance( TF_TEAM_BLUE ) > incursionAtPoint )
		{
			incursionAtPoint = pointAreaVector->Element(i)->GetIncursionDistance( TF_TEAM_BLUE );
		}
	}

	for( i=0; i<TheNavAreas.Count(); ++i )
	{
		CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ i ] );

		float inc = area->GetIncursionDistance( TF_TEAM_BLUE );
		if ( inc > 0.0f && inc < incursionAtPoint && inc > incursionAtPoint - maxInvaderTravelDistance )
		{
			NDebugOverlay::Cross3D( area->GetCenter(), 5.0f, 255, 255, 0, true, 99999.9f );
			//TheNavMesh->AddToSelectedSet( area );
		}
	}
}
static ConCommand tf_select_incursion_zone( "tf_select_incursion_zone", CMD_SelectIncursionZone, "Select areas where invading team approaches the objective. For debugging.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//-------------------------------------------------------------------------
void CMD_SelectControlPointIncursionAreas( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if ( player == NULL )
		return;

	const CUtlVector< CTFNavArea * > *pointAreaVector = TheTFNavMesh()->GetControlPointAreas();

	for( int i=0; i<pointAreaVector->Count(); ++i )
	{
		CTFNavArea *pointArea = (CTFNavArea *)pointAreaVector->Element(i);

		for( i=0; i<TheNavAreas.Count(); ++i )
		{
			CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ i ] );

			if ( area->GetIncursionDistance( TF_TEAM_BLUE ) > pointArea->GetIncursionDistance( TF_TEAM_BLUE ) )
				continue;

			if ( pointArea->IsPotentiallyVisible( area ) )
			{
				// the point is visible from this area

				// if no prior areas can see the point, we have a point incursion area
				CUtlVector< CTFNavArea * > priorVector;
				area->CollectPriorIncursionAreas( TF_TEAM_BLUE, &priorVector );

				int j;
				for( j=0; j<priorVector.Count(); ++j )
				{
					if ( pointArea->IsPotentiallyVisible( priorVector[j] ) )
					{
						break;
					}
				}

				if ( j == priorVector.Count() && j > 0 )
				{
					// no prior areas can see the point
					TheNavMesh->AddToSelectedSet( area );
				}
			}
		}
	}
}
static ConCommand tf_select_control_point_incursion_areas( "tf_select_control_point_incursion_areas", CMD_SelectControlPointIncursionAreas, "Select areas where invading team leaves cover near the objective. For debugging.", FCVAR_GAMEDLL | FCVAR_CHEAT );

//-------------------------------------------------------------------------
CON_COMMAND_F( tf_assign_territory, "Divvy up the mesh into red and blue territories. For debugging.", FCVAR_GAMEDLL )
{
	// Listenserver host or rcon access only!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	int i;

	// clear all territory markings
	for( i=0; i<TheNavAreas.Count(); ++i )
	{
		CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ i ] );
		area->ClearAttributeTF( TF_NAV_RED_TERRITORY | TF_NAV_BLUE_TERRITORY );
		area->SetParent( NULL );
	}

	const CUtlVector< CTFNavArea * > *pointAreaVector = TheTFNavMesh()->GetControlPointAreas();

	if ( !pointAreaVector || pointAreaVector->Count() <= 0 )
		return;

	// find centermost point area, and mark all contested point areas as owned by red
	Vector center = vec3_origin;
	for( i=0; i<pointAreaVector->Count(); ++i )
	{
		center += pointAreaVector->Element(i)->GetCenter();
		pointAreaVector->Element(i)->SetAttributeTF( TF_NAV_RED_TERRITORY );
	}
	center /= pointAreaVector->Count();

	CTFNavArea *pointArea = pointAreaVector->Element(0);
	for( i=0; i<pointAreaVector->Count(); ++i )
	{
		if ( pointAreaVector->Element(i)->IsOverlapping( center ) )
		{
			pointArea = pointAreaVector->Element(i);
			break;
		}
	}

	// spread red's territory to surround the contested area a bit
	const float surroundRange = 1000.0f;
	CUtlVector< CNavArea * > surroundingVector;
	CollectSurroundingAreas( &surroundingVector, pointArea, surroundRange );

	for( int t=0; t<surroundingVector.Count(); ++t )
	{
		CTFNavArea *area = (CTFNavArea *)surroundingVector[t];
		area->ClearAttributeTF( TF_NAV_BLUE_TERRITORY );
		area->SetAttributeTF( TF_NAV_RED_TERRITORY );
	}


	// do a breadth first search out from control point center
	// when a spawn room is reached, mark it and all its parent areas as belonging to the team of the spawn room
	CNavArea::ClearSearchLists();

	pointArea->AddToOpenList();
	pointArea->Mark();
	pointArea->SetParent( NULL );

	CUtlVectorFixedGrowable< const NavConnect *, 64 > adjAreaVector;

	while( !CNavArea::IsOpenListEmpty() )
	{
		// get next area to check
		CTFNavArea *area = static_cast< CTFNavArea * >( CNavArea::PopOpenList() );

		// ignore setup gates, since they will be open after the setup time
		if ( !area->HasAttributeTF( TF_NAV_BLUE_SETUP_GATE | TF_NAV_RED_SETUP_GATE ) && ( area->IsBlocked( TF_TEAM_RED ) || area->IsBlocked( TF_TEAM_BLUE ) ) )
		{
			// don't pass through blocked areas
			continue;
		}

		// explore adjacent floor areas
		adjAreaVector.RemoveAll();

		for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
		{
			// collect all OUTGOING links from this area to adjacent areas
			const NavConnectVector *adjVector = area->GetAdjacentAreas( (NavDirType)dir );
			FOR_EACH_VEC( (*adjVector), bit )
			{
				adjAreaVector.AddToTail( &(*adjVector)[ bit ] );
			}
		}

		FOR_EACH_VEC( adjAreaVector, vit )
		{
			const NavConnect *connect = adjAreaVector[ vit ];
			CTFNavArea *adjArea = static_cast< CTFNavArea * >( connect->area );

			if ( adjArea->ComputeAdjacentConnectionHeightChange( area ) > TF_PLAYER_JUMP_HEIGHT ||
				 area->ComputeAdjacentConnectionHeightChange( adjArea ) > TF_PLAYER_JUMP_HEIGHT )
			{
				// don't go up ledges too high to jump
				continue;
			}

			if ( !adjArea->IsMarked() )
			{
				adjArea->Mark();
				adjArea->SetParent( area );

				// if this area is in a spawn room, mark path we took to get here as the appropriate team's territory
				if ( adjArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_RED ) )
				{
					for( CTFNavArea *pathArea = adjArea; pathArea; pathArea = (CTFNavArea *)pathArea->GetParent() )
					{
						pathArea->SetAttributeTF( TF_NAV_RED_TERRITORY );
					}
				}
				else if ( adjArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE ) )
				{
					for( CTFNavArea *pathArea = adjArea; pathArea; pathArea = (CTFNavArea *)pathArea->GetParent() )
					{
						pathArea->SetAttributeTF( TF_NAV_BLUE_TERRITORY );
					}
				}

				adjArea->AddToOpenListTail();
			}
		}
	}

	if ( args.ArgC() == 1 )
	{
		return;
	}

	// iterate over all areas, spreading territory out from found routes into unclaimed areas
	CUtlVector< CTFNavArea * > spreadVector;

	while( true )
	{
		spreadVector.RemoveAll();

		for( int i=0; i<TheNavAreas.Count(); ++i )
		{
			CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ i ] );
			CTFNavArea *parent = (CTFNavArea *)area->GetParent();

			// if this area has no territory affiliation but its parent does, inherit it and iterate again
			if ( !area->HasAttributeTF( TF_NAV_RED_TERRITORY | TF_NAV_BLUE_TERRITORY ) && parent && parent->HasAttributeTF( TF_NAV_RED_TERRITORY | TF_NAV_BLUE_TERRITORY ) )
			{
				spreadVector.AddToTail( area );
			}
		}

		if ( spreadVector.Count() == 0 )
		{
			// finished spreading
			break;
		}

		// spread the territory influence one step out
		for( int j=0; j<spreadVector.Count(); ++j )
		{
			CTFNavArea *area = spreadVector[j];
			CTFNavArea *parent = (CTFNavArea *)area->GetParent();

			if ( parent->HasAttributeTF( TF_NAV_RED_TERRITORY ) )
			{
				area->SetAttributeTF( TF_NAV_RED_TERRITORY );
			}

			if ( parent->HasAttributeTF( TF_NAV_BLUE_TERRITORY ) )
			{
				area->SetAttributeTF( TF_NAV_BLUE_TERRITORY );
			}
		}
	}
}
#endif // SKIPME


//-------------------------------------------------------------------------
CTFNavMesh::CTFNavMesh( void )
{
	for( int j=0; j<MAX_CONTROL_POINTS; ++j )
	{
		m_controlPointAreaVector[j].RemoveAll();
		m_controlPointCenterAreaVector[j] = NULL;
	}

	ListenForGameEvent( "teamplay_setup_finished" );
	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( "teamplay_point_unlocked" );

	ListenForGameEvent( "player_builtobject" );
	ListenForGameEvent( "player_dropobject" );
	ListenForGameEvent( "player_carryobject" );
	ListenForGameEvent( "object_detonated" );
	ListenForGameEvent( "object_destroyed" );

	m_priorBotCount = 0;

	m_recomputeInternalDataTimer.Invalidate();
}


//-------------------------------------------------------------------------
CTFNavArea *CTFNavMesh::CreateArea( void ) const
{
	return new CTFNavArea;
}


//-------------------------------------------------------------------------
/**
 * Invoked on each game frame
 */
void CTFNavMesh::Update( void )
{
	CNavMesh::Update();

	if ( !TheNavAreas.Count() )
		return;

	UpdateDebugDisplay();

	if ( TheNextBots().GetNextBotCount() > 0 )
	{
		if ( m_priorBotCount == 0 )
		{
			// the first bot was just added
			ScheduleRecomputationOfInternalData( RESET );
		}

		// we use a timer here to give the map logic a few moments to settle out before inspecting it
		if ( m_recomputeInternalDataTimer.HasStarted() && m_recomputeInternalDataTimer.IsElapsed() )
		{
			m_recomputeInternalDataTimer.Invalidate();
			RecomputeInternalData();
		}

		if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT && m_watchCartTimer.IsElapsed() )
		{
			// the cart may have moved, recompute new sniper spots
			m_watchCartTimer.Start( 3.0f );
		}
	}

	m_priorBotCount = TheNextBots().GetNextBotCount();
}


//-------------------------------------------------------------------------
/**
 * (EXTEND) invoked when server loads a new map
 */
void CTFNavMesh::OnServerActivate( void )
{
	CNavMesh::OnServerActivate();

	m_sentryAreas.RemoveAll();

	ResetMeshAttributes( true );
	m_priorBotCount = 0;

	m_setupGateDefenseAreaVector.RemoveAll();

	m_redSpawnRoomAreaVector.RemoveAll();
	m_blueSpawnRoomAreaVector.RemoveAll();

	m_redSpawnRoomExitAreaVector.RemoveAll();
	m_blueSpawnRoomExitAreaVector.RemoveAll();
	for( int i=0; i<MAX_CONTROL_POINTS; ++i )
	{
		m_controlPointAreaVector[i].RemoveAll();
		m_controlPointCenterAreaVector[i] = NULL;
	}
}


//-------------------------------------------------------------------------
/**
 * Invoked when a game round restarts
 */
void CTFNavMesh::OnRoundRestart( void )
{
	CNavMesh::OnRoundRestart();

	ResetMeshAttributes( true );

	// nasty hack
	TheTFBots().OnRoundRestart();

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		RecomputeInternalData();
	}

	DevMsg( "CTFNavMesh: %d nav areas in mesh.\n", GetNavAreaCount() );
}

//--------------------------------------------------------------------------------------------------------
class DoorSetter
{
	CBaseEntity *m_door;
public:
	DoorSetter( CBaseEntity *door )
	{
		m_door = door;
		m_count = 0;
	}

	bool operator()( CNavArea *area )
	{
		CTFNavArea *doorArea = static_cast< CTFNavArea * >(area);
		doorArea->OnDoorCreated( m_door );
		++m_count;
		return true;
	}

	int m_count;
};


//--------------------------------------------------------------------------------------------------------
/**
 * Invoked when a door is created
 */
void CTFNavMesh::OnDoorCreated( CBaseEntity *door )
{
	Extent doorExtent;

	CBaseDoor *funcDoor = dynamic_cast< CBaseDoor * >(door);
	if ( funcDoor )
	{
		QAngle savedAngles = funcDoor->GetLocalAngles();
		Vector savedOrigin = funcDoor->GetLocalOrigin();
		TOGGLE_STATE savedToggleState = funcDoor->m_toggle_state;

		// Closed extent
		funcDoor->SetToggleState( TS_AT_BOTTOM );
		door->CollisionProp()->WorldSpaceAABB( &doorExtent.lo, &doorExtent.hi );

		funcDoor->SetToggleState( savedToggleState );
		funcDoor->SetLocalOrigin( savedOrigin );
		funcDoor->SetLocalAngles( savedAngles );
	}
	else
	{
		CBasePropDoor *propDoor = dynamic_cast< CBasePropDoor * >(door);
		if ( propDoor )
		{
			propDoor->ComputeDoorExtent( &doorExtent, CBasePropDoor::DOOR_EXTENT_CLOSED );
			doorExtent.Encompass( doorExtent );
		}
		else
		{
			Assert( false );
			return;
		}
	}

	doorExtent.hi.z -= StepHeight;
	doorExtent.lo.z -= StepHeight;	// in case the areas are sitting *just* below the door
	DoorSetter setter( door );
	ForAllAreasOverlappingExtent( setter, doorExtent );
}

//-------------------------------------------------------------------------
/**
 * One or more areas may have become blocked or are no longer blocked.
 * Recompute dependent mesh data.
 */
void CTFNavMesh::OnBlockedAreasChanged( void )
{
	VPROF_BUDGET( "CTFNavMesh::OnBlockedAreasChanged", "NextBot" );

	if ( TheNextBots().GetNextBotCount() == 0 )
		return;

	ScheduleRecomputationOfInternalData( BLOCKED_STATUS_CHANGED );
}


//-------------------------------------------------------------------------
void TestAndBlockOverlappingAreas( CBaseEntity *entity )
{
	Ray_t ray;
	trace_t trace;
	NextBotTraceFilterIgnoreActors filter( NULL, COLLISION_GROUP_NONE );

	const float crouchHeight = 30.0f;
	Vector hullMin, hullMax;
	Vector traceFrom, traceTo;

	Extent extent;
	extent.Init( entity );

	CUtlVector< CNavArea * > overlapVector;
	TheNavMesh->CollectAreasOverlappingExtent( extent, &overlapVector );

	for( int i=0; i<overlapVector.Count(); ++i )
	{
		CTFNavArea *area = (CTFNavArea *)overlapVector[i];

		const float tolerance = 1.0f;
		if ( fabs( area->GetCorner( NORTH_WEST ).z - area->GetCorner( NORTH_EAST ).z ) < tolerance )
		{
			// flat along X, potentially varies along Y
			hullMin.x = 0.0f;
			hullMin.y = 0.0f;
			hullMin.z = StepHeight;

			hullMax.x = area->GetSizeX();
			hullMax.y = 0.0f;
			hullMax.z = crouchHeight;

			traceFrom = area->GetCorner( NORTH_WEST );
			traceTo = area->GetCorner( SOUTH_WEST );
		}
		else if ( fabs( area->GetCorner( NORTH_WEST ).z - area->GetCorner( SOUTH_WEST ).z ) < tolerance )
		{
			// flat along Y, potentially varies along X
			hullMin.x = 0.0f;
			hullMin.y = 0.0f;
			hullMin.z = StepHeight;

			hullMax.x = 0.0f;
			hullMax.y = area->GetSizeY();
			hullMax.z = crouchHeight;

			traceFrom = area->GetCorner( NORTH_WEST );
			traceTo = area->GetCorner( NORTH_EAST );
		}
		else
		{
			// varies along both X and Y
			hullMin.x = 0.0f;
			hullMin.y = 0.0f;
			hullMin.z = StepHeight;

			hullMax.x = 1.0f;
			hullMax.y = 1.0f;
			hullMax.z = crouchHeight;

			traceFrom = area->GetCorner( NORTH_WEST );
			traceTo = area->GetCorner( SOUTH_EAST );
		}

		// need to trace from high to low to avoid interpenetration
		if ( traceFrom.z < traceTo.z )
		{
			Vector tmp = traceFrom;
			traceFrom = traceTo;
			traceTo = tmp;
		}

		ray.Init( traceFrom, traceTo, hullMin, hullMax );
		enginetrace->TraceRay( ray, MASK_PLAYERSOLID, &filter, &trace );

		// NDebugOverlay::SweptBox( traceFrom, traceTo, hullMin, hullMax, vec3_angle, 255, 255, 0, 255, 99999.9f );

		if ( trace.DidHit() )
		{
			if ( trace.m_pEnt && trace.m_pEnt->ShouldBlockNav() )
			{
				area->MarkAsBlocked( TEAM_ANY, entity );
			}
		}
	}
}


//-------------------------------------------------------------------------
void CTFNavMesh::ComputeBlockedAreas( void )
{
	// clear all blocked state
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ it ] );
		area->UnblockArea();
	}

#ifdef TF_CREEP_MODE
	if ( TFGameRules()->IsCreepWaveMode() )
	{
		// no blocking for creeps
		return;
	}
#endif

	// block mesh under solid brushes
	CFuncBrush *brush = NULL;
	while( ( brush = (CFuncBrush *)gEntList.FindEntityByClassname( brush, "func_brush" ) ) != NULL )
	{
		if ( brush->IsSolid() ) // && !brush->m_iDisabled )  // "disabled" seems to be overridden by solidity
		{
			// this brush is potentially blocking navigation
			TestAndBlockOverlappingAreas( brush );
		}
	}


	// Find all func_doors in the map.  If a func_door is surrounded by a trigger_multiple,
	// the trigger controls access to the door.  If the func_door is bare, the door itself
	// determines access.
	CBaseDoor *door = NULL;
	while( ( door = (CBaseDoor *)gEntList.FindEntityByClassname( door, "func_door*" ) ) != NULL )
	{
		// if a closed door is not controlled by a trigger assume it doesn't open at all until the scenario changes and map logic opens it
		bool isDoorClosed = ( door->m_toggle_state == TS_AT_BOTTOM || door->m_toggle_state == TS_GOING_DOWN );

		int doorOwnedByTeam = TEAM_UNASSIGNED;

		bool isDoorTriggerControlled = false;

		Extent triggerExtent, doorExtent;
		doorExtent.Init( door );

		CTriggerMultiple *trigger = NULL;
		while( ( trigger = (CTriggerMultiple *)gEntList.FindEntityByClassname( trigger, "trigger_multiple" ) ) != NULL )
		{
			triggerExtent.Init( trigger );

			// just check overlapping, not encompassing, since some door triggers only are player height tall (cp_gravelpit)
			if ( triggerExtent.IsOverlapping( doorExtent ) )
			{
				if ( !trigger->m_bDisabled )
				{
					// this trigger contains this door, and thus controls it
					isDoorTriggerControlled = true;

					// look for a filter attached to this trigger that limits access to one team
					if ( trigger->m_hFilter != NULL && FClassnameIs( trigger->m_hFilter, "filter_activator_tfteam" ) )
					{
						doorOwnedByTeam = trigger->m_hFilter->GetTeamNumber();
					}
				}
			}
		}

		// is this door acting like a wall?
		bool isDoorWall = isDoorTriggerControlled ? false : isDoorClosed;

		// set the blocked status of all areas overlapping this door
		NavAreaCollector doorAreas;
		TheNavMesh->ForAllAreasOverlappingExtent( doorAreas, doorExtent );

		int blockedTeam = ( doorOwnedByTeam == TEAM_UNASSIGNED ) ? TEAM_ANY : ( ( doorOwnedByTeam == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED );

		for( int i=0; i<doorAreas.m_area.Count(); ++i )
		{
			CTFNavArea *area = (CTFNavArea *)doorAreas.m_area[i];

			bool isDoorBlocking;
			if ( area->HasAttributeTF( TF_NAV_DOOR_ALWAYS_BLOCKS ) )
			{
				// closed doors always block
				isDoorBlocking = isDoorClosed;
			}
			else
			{
				// untriggered closed doors, or team-owned doors block
				isDoorBlocking = ( isDoorWall || doorOwnedByTeam != TEAM_UNASSIGNED );
			}

			if ( isDoorBlocking )
			{
				// this door is blocking navigation for at least one team
				if ( !area->HasAttributeTF( TF_NAV_DOOR_NEVER_BLOCKS ) )
				{
					area->MarkAsBlocked( blockedTeam, door );
				}
			}
			else
			{
				// we need to UN-block these areas to account for legacy func_brushes
				// used inside of cosmetic doors as a collision proxy that have marked
				// these areas as blocked
				area->UnblockArea( blockedTeam );
			}
		}
	}

#ifdef DONT_USE_BLOCKS_TOO_MUCH
	// Find all prop_dynamic entities in the map and block areas they overlap
	CDynamicProp *prop = NULL;
	while( ( prop = (CDynamicProp *)gEntList.FindEntityByClassname( prop, "prop_dynamic" ) ) != NULL )
	{
		if ( prop->IsSolid() )
		{
			// if this prop is parented to a door, ignore it - it has already been handled by the door code above
			CBaseDoor *parentDoor = dynamic_cast< CBaseDoor * >( prop->GetParent() );
			if ( !parentDoor )
			{
				// this prop is potentially blocking navigation
				TestAndBlockOverlappingAreas( prop );
			}
		}
	}
#endif // DONT_USE_BLOCKS_TOO_MUCH
}


//-------------------------------------------------------------------------
void CTFNavMesh::CollectControlPointAreas( void )
{
	for( int i=0; i<MAX_CONTROL_POINTS; ++i )
	{
		m_controlPointAreaVector[i].RemoveAll();
		m_controlPointCenterAreaVector[i] = NULL;
	}

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster )
	{
		CBaseEntity *trigger = NULL;
		while( ( trigger = gEntList.FindEntityByClassname( trigger, "trigger_capture_area*" ) ) != NULL )
		{
			CTeamControlPoint *point = ((CTriggerAreaCapture *)trigger)->GetControlPoint();

			if ( point )
			{
				Extent extent;
				extent.Init( trigger );

				// expand extent a bit to make sure it intersects ground below (koth_viaduct)
				extent.lo.z -= HalfHumanHeight;
				extent.hi.z += HalfHumanHeight;

				CUtlVector< CTFNavArea * > *pointAreaVector = &m_controlPointAreaVector[ point->GetPointIndex() ];
				TheNavMesh->CollectAreasOverlappingExtent< CTFNavArea >( extent, pointAreaVector );

				// find area closest to the control point's center
				m_controlPointCenterAreaVector[ point->GetPointIndex() ] = NULL;
				float closeRangeSq = FLT_MAX;

				for( int i=0; i<pointAreaVector->Count(); ++i )
				{
					CTFNavArea *area = pointAreaVector->Element(i);

					float rangeSq = ( area->GetCenter() - trigger->WorldSpaceCenter() ).Length2DSqr();
					if ( rangeSq < closeRangeSq )
					{
						m_controlPointCenterAreaVector[ point->GetPointIndex() ] = area;
						closeRangeSq = rangeSq;
					}
				}
			}
		}
	}
}


//-------------------------------------------------------------------------
// For MvM mode. Mark all nav areas where the bomb can drop and the invaders can reach it.
void CTFNavMesh::ComputeLegalBombDropAreas( void )
{
	if ( !TFGameRules()->IsMannVsMachineMode() )
	{
		return;
	}

	CTFNavArea *startArea = NULL;

	FOR_EACH_VEC( TheNavAreas, it )
	{
		CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ it ] );

		if ( area->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE ) )
		{
			startArea = area;
		}

		area->ClearAttributeTF( TF_NAV_BOMB_CAN_DROP_HERE );
	}

	if ( startArea == NULL )
	{
		Warning( "Can't find blue spawn room nav areas. No legal bomb drop areas are marked" );
		return;
	}

	CNavArea::ClearSearchLists();

	startArea->AddToOpenList();
	startArea->Mark();
	startArea->SetParent( NULL );

	CUtlVectorFixedGrowable< const NavConnect *, 64 > adjAreaVector;

	while( !CNavArea::IsOpenListEmpty() )
	{
		// get next area to check
		CTFNavArea *area = static_cast< CTFNavArea * >( CNavArea::PopOpenList() );

		// explore adjacent floor areas
		adjAreaVector.RemoveAll();

		for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
		{
			// collect all OUTGOING links from this area to adjacent areas
			const NavConnectVector *adjVector = area->GetAdjacentAreas( (NavDirType)dir );
			FOR_EACH_VEC( (*adjVector), bit )
			{
				adjAreaVector.AddToTail( &(*adjVector)[ bit ] );
			}
		}

		FOR_EACH_VEC( adjAreaVector, vit )
		{
			const NavConnect *connect = adjAreaVector[ vit ];
			CTFNavArea *adjArea = static_cast< CTFNavArea * >( connect->area );

			if ( adjArea->IsMarked() )
			{
				continue;
			}

			if ( area->ComputeAdjacentConnectionHeightChange( adjArea ) > StepHeight )
			{
				// don't go up ledges higher than a legal step
				continue;
			}

			if ( !adjArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE | TF_NAV_SPAWN_ROOM_RED ) )
			{
				// this area can be reached by walking from the spawn, so it's legal to drop the bomb here
				adjArea->SetAttributeTF( TF_NAV_BOMB_CAN_DROP_HERE );
			}

			adjArea->Mark();
			adjArea->SetParent( area );

			if ( !adjArea->IsOpen() )
			{
				// Since we're doing a breadth-first search, this area will end up at the end of the list.
				// Adding it to the tail explicitly saves us a bunch of list traversals.
				adjArea->AddToOpenListTail();
			}
		}
	}
}


//-------------------------------------------------------------------------
// For MvM mode. Mark all nav areas where the bomb can drop and the invaders can reach it.
void CTFNavMesh::ComputeBombTargetDistance()
{
	if ( !TFGameRules()->IsMannVsMachineMode() )
	{
		return;
	}

	CCaptureZone *zone = NULL;
	for( int i=0; i<ICaptureZoneAutoList::AutoList().Count(); ++i )
	{
		zone = static_cast< CCaptureZone* >( ICaptureZoneAutoList::AutoList()[i] );
		if ( zone->GetTeamNumber() == TF_TEAM_PVE_INVADERS )
		{
			break;
		}
	}

	if ( zone == NULL )
	{
		Warning( "Can't find bomb delivery zone." );
		return;
	}

	CTFNavArea *zoneArea = (CTFNavArea *)TheTFNavMesh()->GetNearestNavArea( zone->WorldSpaceCenter(), false, 500.0f, true );
	if ( !zoneArea )
	{
		Warning( "No nav area for bomb delivery zone." );
		return;
	}

	// invalidate all travel distances
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ it ] );
		area->m_distanceToBombTarget = -1.0f;
	}

	CNavArea::ClearSearchLists();

	zoneArea->AddToOpenList();
	zoneArea->Mark();
	zoneArea->SetParent( NULL );
	zoneArea->m_distanceToBombTarget = 0.0f;

	CUtlVectorFixedGrowable< const NavConnect *, 64 > adjAreaVector;

	while( !CNavArea::IsOpenListEmpty() )
	{
		// get next area to check
		CTFNavArea *area = static_cast< CTFNavArea * >( CNavArea::PopOpenList() );

		// explore adjacent floor areas
		adjAreaVector.RemoveAll();

		for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
		{
			// collect all OUTGOING links from this area to adjacent areas
			const NavConnectVector *adjVector = area->GetAdjacentAreas( (NavDirType)dir );
			FOR_EACH_VEC( (*adjVector), bit )
			{
				adjAreaVector.AddToTail( &(*adjVector)[ bit ] );
			}
		}

		FOR_EACH_VEC( adjAreaVector, vit )
		{
			const NavConnect *connect = adjAreaVector[ vit ];
			CTFNavArea *adjArea = static_cast< CTFNavArea * >( connect->area );

			if ( area->ComputeAdjacentConnectionHeightChange( adjArea ) > TF_PLAYER_JUMP_HEIGHT )
			{
				// don't go up ledges too high to jump
				continue;
			}

			// compute travel distance
			float newTravelDistance = 0.0f;

			float between = connect->length;
			newTravelDistance = area->m_distanceToBombTarget + between;

			float adjacentTravelDistance = adjArea->m_distanceToBombTarget;

			// Found a shortcut to our neighbor passing through this area?
			// Use a tolernace.  Without it, floating point math can make this loop go on forever,
			// because intermediate results are stored at a different precision
			float flTol = .001f;
			if ( adjacentTravelDistance < 0.0f || adjacentTravelDistance > newTravelDistance + flTol )
			{
				adjArea->m_distanceToBombTarget = newTravelDistance;
				adjArea->Mark();
				adjArea->SetParent( area );

				if ( !adjArea->IsOpen() )
				{
					// Since we're doing a breadth-first search, this area will end up at the end of the list.
					// Adding it to the tail explicitly saves us a bunch of list traversals.
					adjArea->AddToOpenListTail();
				}
			}
			else
			{
				// Found a shortcut this area that passes through the neighbor?
				float newTravelDistanceFromAdjacent = adjacentTravelDistance + between;
				if ( newTravelDistanceFromAdjacent + flTol < area->m_distanceToBombTarget )
				{
					// check if the reverse direction is cheaper (for the case of jumping off edges)
					area->m_distanceToBombTarget = newTravelDistanceFromAdjacent;
					area->Mark();
					area->SetParent( adjArea );
					if ( !area->IsOpen() )
					{
						// found a cheaper path, try to traverse backward
						area->AddToOpenListTail();
					}
				}
			}
		}
	}
}


//-------------------------------------------------------------------------
void CTFNavMesh::RecomputeInternalData( void )
{
	CollectControlPointAreas();
	RemoveAllMeshDecoration();
	DecorateMesh();
	ComputeBlockedAreas();			// relies on DecorateMesh() being complete
	ComputeIncursionDistances();
	ComputeInvasionAreas();
	ComputeLegalBombDropAreas();
	ComputeBombTargetDistance();	// for MvM

	if ( m_recomputeReason == RESET || m_recomputeReason == SETUP_FINISHED )
	{
		// update point-conditionally blocked areas
		FOR_EACH_VEC( TheNavAreas, it )
		{
			CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ it ] );

			if ( area->HasAttributeTF( TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE ) )
			{
				area->SetAttributeTF( TF_NAV_BLOCKED );
			}
		}
	}

	if ( m_recomputeReason == POINT_CAPTURED )
	{
		// update point-conditionally blocked areas
		FOR_EACH_VEC( TheNavAreas, it )
		{
			CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ it ] );

			if ( area->HasAttributeTF( TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE ) )
			{
				// which point unblocks us?

				// if no modifier given, unblock after first capture
				bool isUnblocked = true;

				if ( area->HasAttributeTF( TF_NAV_WITH_SECOND_POINT ) )
				{
					isUnblocked = (m_recomputeReasonWhichPoint >= 1);
				}
				else if ( area->HasAttributeTF( TF_NAV_WITH_THIRD_POINT ) )
				{
					isUnblocked = (m_recomputeReasonWhichPoint >= 2);
				}
				else if ( area->HasAttributeTF( TF_NAV_WITH_FOURTH_POINT ) )
				{
					isUnblocked = (m_recomputeReasonWhichPoint >= 3);
				}
				else if ( area->HasAttributeTF( TF_NAV_WITH_FIFTH_POINT ) )
				{
					isUnblocked = (m_recomputeReasonWhichPoint >= 4);
				}

				if ( isUnblocked )
				{
					area->ClearAttributeTF( TF_NAV_BLOCKED );
				}
			}
			else if ( area->HasAttributeTF( TF_NAV_BLOCKED_AFTER_POINT_CAPTURE ) )
			{
				// which point blocks us?

				// if no modifier given, block after first capture
				bool isBlocked = true;

				if ( area->HasAttributeTF( TF_NAV_WITH_SECOND_POINT ) )
				{
					isBlocked = ( m_recomputeReasonWhichPoint >= 1 );
				}
				else if ( area->HasAttributeTF( TF_NAV_WITH_THIRD_POINT ) )
				{
					isBlocked = ( m_recomputeReasonWhichPoint >= 2 );
				}
				else if ( area->HasAttributeTF( TF_NAV_WITH_FOURTH_POINT ) )
				{
					isBlocked = ( m_recomputeReasonWhichPoint >= 3 );
				}
				else if ( area->HasAttributeTF( TF_NAV_WITH_FIFTH_POINT ) )
				{
					isBlocked = ( m_recomputeReasonWhichPoint >= 4 );
				}

				if ( isBlocked )
				{
					area->SetAttributeTF( TF_NAV_BLOCKED );
				}
			}
		}
	}

	m_recomputeInternalDataTimer.Invalidate();
}

//-------------------------------------------------------------------------
// Re-calculate sentry danger attributes.
void CTFNavMesh::OnObjectChanged()
{
	// Clear all sentry danger attributes.
	ResetMeshAttributes( false );

	CUtlVector< CBaseObject * > ActiveSentries;
	ActiveSentries.EnsureCapacity( 16 );

	// Get a list of all sentries that aren't being carried or dying.
	for ( int oit = 0; oit < IBaseObjectAutoList::AutoList().Count(); ++oit )
	{
		CBaseObject* obj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[ oit ] );

		if ( obj->ObjectType() == OBJ_SENTRYGUN )
		{
			if ( !obj->IsDying() && !obj->IsCarried() )
				ActiveSentries.AddToTail( obj );
		}
	}

	// Only go through the NavAreas if we found some live sentries. Hopefully some of these
	//  sentries will be able to shoot some spies in the face.
	if ( ActiveSentries.Count() )
	{
		// We must iterate all of the nav areas because we're testing visibility
		//  and arbitrary switchback routes make the use of SearchSurroundingAreas
		//  not useful.
		FOR_EACH_VEC( TheNavAreas, it )
		{
			CTFNavArea *area = static_cast< CTFNavArea *>( TheNavAreas[ it ] );

			// Check all active sentries against this area.
			FOR_EACH_VEC( ActiveSentries, oit )
			{
				const CBaseObject* obj = ActiveSentries[ oit ];

				// If this area in range of this sentry?
				Vector close;
				area->GetClosestPointOnArea( obj->GetAbsOrigin(), &close );

				if ( ( obj->GetAbsOrigin() - close ).IsLengthLessThan( SENTRY_MAX_RANGE ) )
				{
					// Can this sentry reach this area?
					if ( area->IsPartiallyVisible( obj->GetAbsOrigin() + Vector( 0, 0, 30.0f ), obj ) )
					{
						// If this area wasn't already added to m_sentryAreas, do it now.
						if ( !area->HasAttributeTF( TF_NAV_BLUE_SENTRY_DANGER | TF_NAV_RED_SENTRY_DANGER ) )
							m_sentryAreas.AddToTail( area );

						// Mark this area as being potentially dangerous.
						area->SetAttributeTF( ( obj->GetTeamNumber() == TF_TEAM_RED ) ? TF_NAV_RED_SENTRY_DANGER : TF_NAV_BLUE_SENTRY_DANGER );
					}
				}
			}
		}
	}

	if ( tf_show_sentry_danger.GetBool() )
		DevMsg( "%s: sentries:%d areas count:%d\n", __FUNCTION__, ActiveSentries.Count(), m_sentryAreas.Count() );
}


//--------------------------------------------------------------------------------------------------------
/**
 * Return true if a Sentry Gun has been built in the given area
 */
bool CTFNavMesh::IsSentryGunHere( CTFNavArea *area ) const
{
	// Check to see if the area is on the highway to the danger zone.
	//  If it isn't then there shouldn't be a sentry gun here.
	if ( area->HasAttributeTF( TF_NAV_BLUE_SENTRY_DANGER | TF_NAV_RED_SENTRY_DANGER ) )
	{
		// Walk through all the objects built by players
		for ( int oit = 0; oit < IBaseObjectAutoList::AutoList().Count(); ++oit )
		{
			CBaseObject* obj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[ oit ] );

			if ( obj->ObjectType() == OBJ_SENTRYGUN )
			{
				// If this object is a sentry gun, and it's in this nav area, return true.
				if ( GetNearestNavArea( obj ) == area )
					return true;
			}
		}
	}

	return false;
}


//-------------------------------------------------------------------------
// Fill given vector will all objects on the given team
void CTFNavMesh::CollectBuiltObjects( CUtlVector< CBaseObject * > *collectionVector, int team )
{
	collectionVector->RemoveAll();

	// check all active sentries against this area
	for ( int oit = 0; oit < IBaseObjectAutoList::AutoList().Count(); ++oit )
	{
		CBaseObject* obj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[ oit ] );

		if ( team == TEAM_ANY || obj->GetTeamNumber() == team )
		{
			collectionVector->AddToTail( obj );
		}
	}
}


//-------------------------------------------------------------------------
void CTFNavMesh::FireGameEvent( IGameEvent *event )
{
	CNavMesh::FireGameEvent( event );

	const CUtlString eventName( event->GetName() );

	if ( eventName == "teamplay_point_captured" )
	{
		int whichPoint = event->GetInt( "cp" );

		ScheduleRecomputationOfInternalData( POINT_CAPTURED, whichPoint );
	}
	else if ( eventName == "teamplay_setup_finished" )
	{
		ScheduleRecomputationOfInternalData( SETUP_FINISHED );
	}
	else if ( eventName == "teamplay_point_unlocked" )
	{
		// recompute since doors may have opened/etc (koth_nucleus)
		int whichPoint = event->GetInt( "cp" );

		ScheduleRecomputationOfInternalData( POINT_UNLOCKED, whichPoint );
	}
	else if ( eventName == "player_builtobject" ||
			  eventName == "player_carryobject" ||
			  eventName == "object_detonated" ||
			  eventName == "object_destroyed" )
	{
		// We don't need "player_dropobject" as "player_builtobject" is sent right after.
		// Some message have "object", some have "objectid" - use the one that is set.
		int objecttype = !event->IsEmpty( "objecttype" ) ? event->GetInt( "objecttype" ) : event->GetInt( "object" );
		if ( objecttype == OBJ_SENTRYGUN )
		{
			if ( tf_show_sentry_danger.GetBool() )
				DevMsg( "%s: Got sentrygun %s event\n", __FUNCTION__, eventName.Get() );

			OnObjectChanged();
		}
	}
}


//-------------------------------------------------------------------------
void CTFNavMesh::BeginCustomAnalysis( bool bIncremental )
{

}


//-------------------------------------------------------------------------
// invoked when custom analysis step is complete
void CTFNavMesh::PostCustomAnalysis( void )
{

}


//-------------------------------------------------------------------------
void CTFNavMesh::EndCustomAnalysis()
{

}


//-------------------------------------------------------------------------
/**
 * Returns sub-version number of data format used by derived classes
 */
unsigned int CTFNavMesh::GetSubVersionNumber( void ) const
{
	// 1: initial implementation
	// 2: added TF-specific attribute flags
	return 2;
}


//-------------------------------------------------------------------------
/** 
 * Store custom mesh data for derived classes
 */
void CTFNavMesh::SaveCustomData( CUtlBuffer &fileBuffer ) const
{

}


//-------------------------------------------------------------------------
/**
 * Load custom mesh data for derived classes
 */
void CTFNavMesh::LoadCustomData( CUtlBuffer &fileBuffer, unsigned int subVersion )
{

}


//-------------------------------------------------------------------------
/**
 * Recompute travel distance from each team's spawn room for each nav area
 */
void CTFNavMesh::ComputeIncursionDistances( void )
{
	VPROF_BUDGET( "CTFNavMesh::ComputeIncursionDistances", "NextBot" );

	// invalidate all travel distances
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ it ] );

		for( int i=0; i<TF_TEAM_COUNT; ++i )
		{
			area->m_distanceFromSpawnRoom[i] = -1.0f;
		}
	}

	bool isRedComputed = false;
	bool isBlueComputed = false;
	for ( int i=0; i<IFuncRespawnRoomAutoList::AutoList().Count(); ++i )
	{
		CFuncRespawnRoom *spawnRoom = static_cast< CFuncRespawnRoom* >( IFuncRespawnRoomAutoList::AutoList()[i] );

		if ( !spawnRoom->GetActive() )
			continue;

		if ( spawnRoom->m_bDisabled )
			continue;

		// find a spawn point inside this room
		for ( int i=0; i<ITFTeamSpawnAutoList::AutoList().Count(); ++i )
		{
			CTFTeamSpawn *spawnSpot = static_cast< CTFTeamSpawn* >( ITFTeamSpawnAutoList::AutoList()[i] );

			if ( !spawnSpot->IsTriggered( NULL ) )
				continue;

			if ( spawnSpot->IsDisabled() )
				continue;

			if ( spawnSpot->GetTeamNumber() == TF_TEAM_RED && isRedComputed )
				continue;

			if ( spawnSpot->GetTeamNumber() == TF_TEAM_BLUE && isBlueComputed )
				continue;

			if ( spawnRoom->PointIsWithin( spawnSpot->GetAbsOrigin() ) )
			{
				// found a valid spawn spot in an active spawn room, compute travel distances throughout the nav mesh
				CTFNavArea *spawnArea = static_cast< CTFNavArea * >( TheTFNavMesh()->GetNearestNavArea( spawnSpot ) );
				if ( spawnArea )
				{
					ComputeIncursionDistances( spawnArea, spawnSpot->GetTeamNumber() );

					if ( spawnSpot->GetTeamNumber() == TF_TEAM_RED )
					{
						isRedComputed = true;
					}
					else
					{
						isBlueComputed = true;
					}

					break;
				}
			}
		}
	}

	if ( !isRedComputed )
	{
		Warning( "Can't compute incursion distances from the Red spawn room(s). Bots will perform poorly. This is caused by either a missing func_respawnroom, or missing info_player_teamspawn entities within the func_respawnroom.\n" );
	}

	if ( !isBlueComputed )
	{
		Warning( "Can't compute incursion distances from the Blue spawn room(s). Bots will perform poorly. This is caused by either a missing func_respawnroom, or missing info_player_teamspawn entities within the func_respawnroom.\n" );
	}

	if ( !TFGameRules()->IsMannVsMachineMode() )
	{
		// In Raid mode, the Red (bot) team has no spawn room.
		// So, we'll assume the Red incursion distance is the inverse of the Blue incursion distance for now.
		// @TODO: Use the Boss battle room as the anchor for computing Red incursion distances
		float maxBlueIncursionDistance = 0.0f;

		for( int i=0; i<TheNavAreas.Count(); ++i )
		{
			CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ i ] );

			if ( area->GetIncursionDistance( TF_TEAM_BLUE ) > maxBlueIncursionDistance )
			{
				maxBlueIncursionDistance = area->GetIncursionDistance( TF_TEAM_BLUE );
			}
		}

		for( int i=0; i<TheNavAreas.Count(); ++i )
		{
			CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ i ] );

			if ( area->GetIncursionDistance( TF_TEAM_BLUE ) >= 0.0f )
			{
				area->m_distanceFromSpawnRoom[ TF_TEAM_RED ] = maxBlueIncursionDistance - area->GetIncursionDistance( TF_TEAM_BLUE );
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------
/**
 * Flood-fill outwards, marking flow distance as we go.
 * When we reach an area, stop if it already has a lesser travel distance
 */
void CTFNavMesh::ComputeIncursionDistances( CTFNavArea *spawnArea, int team )
{
	if ( spawnArea == NULL || team < 0 || team >= TF_TEAM_COUNT )
	{
		return;
	}

	CNavArea::ClearSearchLists();

	spawnArea->m_distanceFromSpawnRoom[ team ] = 0.0f;
	spawnArea->AddToOpenList();
	spawnArea->Mark();
	spawnArea->SetParent( NULL );

	CUtlVectorFixedGrowable< const NavConnect *, 64 > adjAreaVector;
	//TFNavAttributeType teamSpawnRoom = ( team == TF_TEAM_RED ) ? TF_NAV_SPAWN_ROOM_RED : TF_NAV_SPAWN_ROOM_BLUE;

	while( !CNavArea::IsOpenListEmpty() )
	{
		// get next area to check
		CTFNavArea *area = static_cast< CTFNavArea * >( CNavArea::PopOpenList() );
		
		bool bIgnoreBlockedAreas = false;

#ifdef TF_RAID_MODE
		// TODO: Raid mode ignores blocked areas for now (cap gates break this)
		if ( TFGameRules()->IsRaidMode()  )
		{
			bIgnoreBlockedAreas = true;
		}
#endif // TF_RAID_MODE

		// TODO: Ditto for Mann Vs Machine mode
		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			bIgnoreBlockedAreas = true;
		}

		if ( !bIgnoreBlockedAreas )
		{
			// ignore spawn room exits, since they presumably will be open
			// ignore setup gates, since they will be open after the setup time
			if ( !area->HasAttributeTF( TF_NAV_SPAWN_ROOM_EXIT | TF_NAV_BLUE_SETUP_GATE | TF_NAV_RED_SETUP_GATE ) && area->IsBlocked( team ) )
			{
				// don't pass through blocked areas
				continue;
			}
		}

		// explore adjacent floor areas
		adjAreaVector.RemoveAll();

		for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
		{
			// collect all OUTGOING links from this area to adjacent areas
			const NavConnectVector *adjVector = area->GetAdjacentAreas( (NavDirType)dir );
			FOR_EACH_VEC( (*adjVector), bit )
			{
				adjAreaVector.AddToTail( &(*adjVector)[ bit ] );
			}
		}

		FOR_EACH_VEC( adjAreaVector, vit )
		{
			const NavConnect *connect = adjAreaVector[ vit ];
			CTFNavArea *adjArea = static_cast< CTFNavArea * >( connect->area );

			if ( area->ComputeAdjacentConnectionHeightChange( adjArea ) > TF_PLAYER_JUMP_HEIGHT )
			{
				// don't go up ledges too high to jump
				continue;
			}

			// compute travel distance
			float newTravelDistance = 0.0f;

			// travel distance is zero in all areas of our spawn room
			// if ( !adjArea->HasAttributeTF( teamSpawnRoom ) )
			{
				float between = connect->length;
				newTravelDistance = area->m_distanceFromSpawnRoom[ team ] + between;
			}

			float adjacentTravelDistance = adjArea->m_distanceFromSpawnRoom[ team ];

			if ( adjacentTravelDistance < 0.0f || adjacentTravelDistance > newTravelDistance )
			{
				adjArea->m_distanceFromSpawnRoom[ team ] = newTravelDistance;
				adjArea->Mark();
				adjArea->SetParent( area );

				if ( !adjArea->IsOpen() )
				{
					// Since we're doing a breadth-first search, this area will end up at the end of the list.
					// Adding it to the tail explicitly saves us a bunch of list traversals.
					adjArea->AddToOpenListTail();
				}
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------
void CTFNavMesh::ComputeInvasionAreas( void )
{
	VPROF_BUDGET( "CTFNavMesh::ComputeInvasionAreas", "NextBot" );

	FOR_EACH_VEC( TheNavAreas, it )
	{
		CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ it ] );
		
		area->ComputeInvasionAreaVectors();
	}
}


//--------------------------------------------------------------------------------------------------------
class CCollectAndLabelSpawnRoomAreas
{
public:
	CCollectAndLabelSpawnRoomAreas( void )
	{
		m_room = NULL;
	}

	void Init( CFuncRespawnRoom *room, int team, CUtlVector< CTFNavArea * > *areaVector )
	{
		m_room = room;
		m_team = team;
		m_areaVector = areaVector;
	}

	bool operator() ( CNavArea *baseArea )
	{
		static Vector stepHeight( 0.0f, 0.0f, 18.0f );

		if ( !m_room )
			return true;

		if ( m_room->PointIsWithin( baseArea->GetCenter() + stepHeight ) ||
			 m_room->PointIsWithin( baseArea->GetCorner( NORTH_WEST ) + stepHeight ) ||
			 m_room->PointIsWithin( baseArea->GetCorner( NORTH_EAST ) + stepHeight ) ||
			 m_room->PointIsWithin( baseArea->GetCorner( SOUTH_WEST ) + stepHeight ) ||
			 m_room->PointIsWithin( baseArea->GetCorner( SOUTH_EAST ) + stepHeight ) )
		{
			CTFNavArea *area = (CTFNavArea *)baseArea;

			area->SetAttributeTF( ( m_team == TF_TEAM_RED ) ? TF_NAV_SPAWN_ROOM_RED : TF_NAV_SPAWN_ROOM_BLUE );

			m_areaVector->AddToTail( area );
		}

		return true;
	}

	CFuncRespawnRoom *m_room;
	int m_team;
	CUtlVector< CTFNavArea * > *m_areaVector;
};


//--------------------------------------------------------------------------------------------------------
void CTFNavMesh::CollectAndMarkSpawnRoomExits( CTFNavArea *area, CUtlVector< CTFNavArea * > *exitAreaVector )
{
	for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
	{
		const NavConnectVector *connect = area->GetAdjacentAreas( (NavDirType)dir );
		if ( connect )
		{
			FOR_EACH_VEC( (*connect), cit )
			{
				CTFNavArea *adjArea = (CTFNavArea *)connect->Element(cit).area;

				if ( !adjArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE | TF_NAV_SPAWN_ROOM_RED ) )
				{
					// adjacent area leads out of spawn room - this is an exit
					area->SetAttributeTF( TF_NAV_SPAWN_ROOM_EXIT );
					exitAreaVector->AddToTail( area );
					return;
				}
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------
void CTFNavMesh::DecorateMesh( void )
{
	VPROF_BUDGET( "CTFNavMesh::DecorateMesh", "NextBot" );

	CBaseEntity *entity = NULL;
	CCollectAndLabelSpawnRoomAreas collectAndLabel;
	Extent extent;

	// mark spawn rooms
	m_redSpawnRoomAreaVector.RemoveAll();
	m_blueSpawnRoomAreaVector.RemoveAll();

	for ( int iFuncRespawnRoom=0; iFuncRespawnRoom<IFuncRespawnRoomAutoList::AutoList().Count(); ++iFuncRespawnRoom )
	{
		CFuncRespawnRoom *respawnRoom = static_cast< CFuncRespawnRoom* >( IFuncRespawnRoomAutoList::AutoList()[iFuncRespawnRoom] );

		if ( !respawnRoom->GetActive() )
			continue;

		if ( respawnRoom->m_bDisabled )
			continue;

		// func_respawn rooms only enforce spawn room rules. We need to search for enabled
		// info_player_teamspawn entities contained within an active func_respawnroom in
		// order to locate the current set of active spawn rooms
		// find a spawn point inside this room
		for ( int iTFTeamSpawn=0; iTFTeamSpawn<ITFTeamSpawnAutoList::AutoList().Count(); ++iTFTeamSpawn )
		{
			CTFTeamSpawn *spawnSpot = static_cast< CTFTeamSpawn* >( ITFTeamSpawnAutoList::AutoList()[iTFTeamSpawn] );

			if ( !spawnSpot->IsTriggered( NULL ) )
				continue;

			if ( spawnSpot->IsDisabled() )
				continue;

			if ( respawnRoom->PointIsWithin( spawnSpot->GetAbsOrigin() ) )
			{
				// found a valid spawn spot in an active spawn room
				collectAndLabel.Init( respawnRoom, spawnSpot->GetTeamNumber(), spawnSpot->GetTeamNumber() == TF_TEAM_RED ? &m_redSpawnRoomAreaVector : &m_blueSpawnRoomAreaVector );
				extent.Init( respawnRoom );

				TheNavMesh->ForAllAreasOverlappingExtent( collectAndLabel, extent );
			}
		}
	}

	// mark each spawn room area adjacent to a non-spawn room area as an exit
	m_redSpawnRoomExitAreaVector.RemoveAll();
	m_blueSpawnRoomExitAreaVector.RemoveAll();

	FOR_EACH_VEC( m_redSpawnRoomAreaVector, rit )
	{
		CollectAndMarkSpawnRoomExits( m_redSpawnRoomAreaVector[ rit ], &m_redSpawnRoomExitAreaVector );
	}

	FOR_EACH_VEC( m_blueSpawnRoomAreaVector, bit )
	{
		CollectAndMarkSpawnRoomExits( m_blueSpawnRoomAreaVector[ bit ], &m_blueSpawnRoomExitAreaVector );
	}


	// mark ammo areas
	entity = NULL;
	while( ( entity = gEntList.FindEntityByClassname( entity, "item_ammopack*" ) ) != NULL )
	{
		CTFNavArea *area = (CTFNavArea *)TheTFNavMesh()->GetNearestNavArea( entity->GetAbsOrigin() );
		if ( area )
		{
			area->SetAttributeTF( TF_NAV_HAS_AMMO );
		}
	}

	// mark health areas
	entity = NULL;
	while( ( entity = gEntList.FindEntityByClassname( entity, "item_healthkit*" ) ) != NULL )
	{
		CTFNavArea *area = (CTFNavArea *)TheTFNavMesh()->GetNearestNavArea( entity->GetAbsOrigin() );
		if ( area )
		{
			area->SetAttributeTF( TF_NAV_HAS_HEALTH );
		}
	}

	// mark control points
	for( int p=0; p<MAX_CONTROL_POINTS; ++p )
	{
		CUtlVector< CTFNavArea * > *pointAreaVector = &m_controlPointAreaVector[ p ];

		for( int i=0; i<pointAreaVector->Count(); ++i )
		{
			pointAreaVector->Element(i)->SetAttributeTF( TF_NAV_CONTROL_POINT );
		}
	}
}


//--------------------------------------------------------------------------------------------------------
void CTFNavMesh::RemoveAllMeshDecoration( void )
{
	FOR_EACH_VEC( TheNavAreas, it )
	{
		CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ it ] );

		// wipe all non-persistent attributes
		area->ClearAttributeTF( (TFNavAttributeType)( ~TF_NAV_PERSISTENT_ATTRIBUTES ) );
	}

	// We just cleared all our SENTRY_DANGER attributes. Wipe m_sentryAreas and recompute.
	m_sentryAreas.RemoveAll();
	OnObjectChanged();
}


//--------------------------------------------------------------------------------------------------------
void CTFNavMesh::ResetMeshAttributes( bool bScheduleRecomputation )
{
	// Clear all sentry danger attributes.
	FOR_EACH_VEC( m_sentryAreas, nit )
	{
		// One of the sentry danger attributes should be set.
		Assert( bScheduleRecomputation || m_sentryAreas[ nit ]->HasAttributeTF(  TF_NAV_BLUE_SENTRY_DANGER | TF_NAV_RED_SENTRY_DANGER ) );
		m_sentryAreas[ nit ]->ClearAttributeTF( TF_NAV_BLUE_SENTRY_DANGER | TF_NAV_RED_SENTRY_DANGER );
	}
	m_sentryAreas.RemoveAll();

#ifdef DBGFLAG_ASSERT
	FOR_EACH_VEC( TheNavAreas, it )
	{
		// Sentry danger attributes should not be set anywhere.
		CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ it ] );
		Assert( !area->HasAttributeTF( TF_NAV_BLUE_SENTRY_DANGER | TF_NAV_RED_SENTRY_DANGER ) );
	}
#endif

	if ( bScheduleRecomputation )
	{
		ScheduleRecomputationOfInternalData( RESET );
	}
}


//--------------------------------------------------------------------------------------------------------
class DrawIncursionFlow
{
public:
	bool operator() ( CNavArea *baseArea )
	{
		CTFNavArea *area = static_cast< CTFNavArea * >( baseArea );

		int team = ( tf_show_incursion_flow.GetInt() == 1 ) ? TF_TEAM_RED : TF_TEAM_BLUE;

		const float cycleRange = 2500.0f;
		const float cycleRate = 0.333f;		// cycles/sec

		float baseFlow = area->GetIncursionDistance( team );

		for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
		{
			const NavConnectVector *adjVector = area->GetAdjacentAreas( (NavDirType)dir );
			FOR_EACH_VEC( (*adjVector), bit )
			{
				CTFNavArea *adjArea = static_cast< CTFNavArea * >( (*adjVector)[ bit ].area );

				if ( area->ComputeAdjacentConnectionHeightChange( adjArea ) > TF_PLAYER_JUMP_HEIGHT )
				{
					// don't go up ledges too high to jump
					continue;
				}

				float adjFlow = adjArea->GetIncursionDistance( team );

				if ( adjFlow > baseFlow )
				{
					float cycle = fmod( adjFlow - ( gpGlobals->curtime * cycleRate * cycleRange ), cycleRange );
					float t = 2.0f * cycle / cycleRange;
					if ( t > 1.0f )
					{
						t = 2.0f - t;
					}
					
					int r, g, b;
					if ( team == TF_TEAM_RED )
					{
						r = 255 * t;
						g = 0;
						b = 0;
					}
					else
					{
						r = 0;
						g = 0;
						b = 255 * t;
					}

					NDebugOverlay::HorzArrow( area->GetCenter(), adjArea->GetCenter(), 5.0f, r, g, b, 255, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
				}
			}
		}

		return true;
	}
};


void CTFNavMesh::UpdateDebugDisplay( void ) const
{
	// avoid Warning() spam from UTIL_GetListenServerHost when on a dedicated server
	if ( engine->IsDedicatedServer() )
		return;

	CBasePlayer *player = UTIL_GetListenServerHost();
	if ( player == NULL )
		return;


	if ( tf_show_in_combat_areas.GetBool() )
	{
		FOR_EACH_VEC( TheNavAreas, it )
		{
			CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ it ] );
			
			if ( area->IsInCombat() )
			{
				float t = area->GetCombatIntensity();
				area->DrawFilled( t * 255, 0, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
			}
		}
	}

	if ( tf_show_enemy_invasion_areas.GetBool() )
	{
		CTFNavArea *myArea = static_cast< CTFNavArea * >( player->GetLastKnownArea() );

		if ( myArea )
		{
			const CUtlVector< CTFNavArea * > &invasionAreaVector = myArea->GetEnemyInvasionAreaVector( player->GetTeamNumber() );

			FOR_EACH_VEC( invasionAreaVector, it )
			{
				CTFNavArea *area = static_cast< CTFNavArea * >( invasionAreaVector[ it ] );

				area->DrawFilled( 255, 0, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
			}
		}
	}

	if ( tf_show_bomb_drop_areas.GetBool() )
	{
		FOR_EACH_VEC( TheNavAreas, it )
		{
			CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ it ] );

			if ( area->HasAttributeTF( TF_NAV_BOMB_CAN_DROP_HERE ) )
			{
				area->DrawFilled( 0, 255, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
			}
		}
	}

	if ( tf_show_blocked_areas.GetBool() )
	{
		FOR_EACH_VEC( TheNavAreas, it )
		{
			CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ it ] );
			const char *describe = "";

			if ( area->HasAttributeTF( TF_NAV_BLOCKED ) )
			{
				area->DrawFilled( 255, 0, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true, 0.0f );
			}

			if ( area->IsBlocked( TF_TEAM_RED ) )
			{
				if ( area->IsBlocked( TF_TEAM_BLUE ) )
				{
					area->DrawFilled( 100, 0, 100, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
					describe = "Blocked for All";
				}
				else
				{
					area->DrawFilled( 100, 0, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
					describe = "Blocked for Red";
				}
			}
			else if ( area->IsBlocked( TF_TEAM_BLUE ) )
			{
				area->DrawFilled( 0, 0, 100, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				describe = "Blocked for Blue";
			}

			if ( describe && TheNavMesh->GetSelectedArea() == area )
			{
				NDebugOverlay::Text( area->GetCenter(), describe, false, NDEBUG_PERSIST_TILL_NEXT_SERVER );
			}
		}
	}

	if ( tf_show_incursion_flow.GetInt() > 0 || tf_show_incursion_flow_gradient.GetInt() > 0 )
	{
		Vector forward;
		AngleVectors( player->EyeAngles() + player->GetPunchAngle(), &forward );

		float maxRange = 2000.0f;
		Vector to = player->EyePosition() + maxRange * forward;

		trace_t result;
		CTraceFilterWalkableEntities filter( NULL, COLLISION_GROUP_NONE, WALK_THRU_EVERYTHING );
		UTIL_TraceLine( player->EyePosition(), to, MASK_NPCSOLID, &filter, &result );

		CTFNavArea *selectedArea = static_cast< CTFNavArea * >( TheNavMesh->GetNearestNavArea( result.endpos, false, 500.0f ) );

		if ( selectedArea )
		{
			if ( tf_show_incursion_flow.GetInt() > 0 )
			{
				DrawIncursionFlow draw;
				SearchSurroundingAreas( selectedArea, selectedArea->GetCenter(), draw, tf_show_incursion_flow_range.GetFloat() );
			}
			else if ( tf_show_incursion_flow_gradient.GetInt() > 0 )
			{
				int myTeam;
				int r,g,b;
				if ( tf_show_incursion_flow_gradient.GetInt() == 1 )
				{
					myTeam = TF_TEAM_RED;
					r = 255;
					g = 0;
					b = 0;
				}
				else
				{
					myTeam = TF_TEAM_BLUE;
					r = 0;
					g = 0;
					b = 255;
				}

				selectedArea->DrawFilled( r, g, b, 255 );

				CUtlVector< CTFNavArea * > areaVector;
				selectedArea->CollectPriorIncursionAreas( myTeam, &areaVector );
				FOR_EACH_VEC( areaVector, p )
				{
					areaVector[p]->DrawFilled( r/2, g/2, b/2, 255 );
				}

				selectedArea->CollectNextIncursionAreas( myTeam, &areaVector );
				FOR_EACH_VEC( areaVector, n )
				{
					areaVector[n]->DrawFilled( MIN( r+100, 255 ), MIN( g+100, 255 ), MIN( b+100, 255 ), 255 );
				}
			}
		}
	}

	if ( tf_show_mesh_decoration.GetBool() && !tf_show_mesh_decoration_manual.GetBool() )
	{
		// render these from cached vectors to verify their data
		int i;
		const CUtlVector< CTFNavArea * > *areaVector;

		areaVector = GetSpawnRoomAreas( TF_TEAM_BLUE );
		if ( areaVector )
		{
			for( i=0; i<areaVector->Count(); ++i )
			{
				CTFNavArea *area = areaVector->Element(i);

				if ( !area->HasAttributeTF( TF_NAV_SPAWN_ROOM_EXIT ) )
				{
					area->DrawFilled( 0, 0, 255, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );

					if ( TheNavMesh->GetSelectedArea() == area )
					{
						NDebugOverlay::Text( area->GetCenter(), "Blue Spawn Room", false, NDEBUG_PERSIST_TILL_NEXT_SERVER );
					}
				}
			}
		}

		areaVector = GetSpawnRoomExitAreas( TF_TEAM_BLUE );
		if ( areaVector )
		{
			for( i=0; i<areaVector->Count(); ++i )
			{
				CTFNavArea *area = areaVector->Element(i);

				area->DrawFilled( 150, 150, 255, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );

				if ( TheNavMesh->GetSelectedArea() == area )
				{
					NDebugOverlay::Text( area->GetCenter(), "Blue Spawn Exit", false, NDEBUG_PERSIST_TILL_NEXT_SERVER );
				}
			}
		}

		areaVector = GetSpawnRoomAreas( TF_TEAM_RED );
		if ( areaVector )
		{
			for( i=0; i<areaVector->Count(); ++i )
			{
				CTFNavArea *area = areaVector->Element(i);

				if ( !area->HasAttributeTF( TF_NAV_SPAWN_ROOM_EXIT ) )
				{
					area->DrawFilled( 255, 0, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );

					if ( TheNavMesh->GetSelectedArea() == area )
					{
						NDebugOverlay::Text( area->GetCenter(), "Red Spawn Room", false, NDEBUG_PERSIST_TILL_NEXT_SERVER );
					}
				}
			}
		}

		areaVector = GetSpawnRoomExitAreas( TF_TEAM_RED );
		if ( areaVector )
		{
			for( i=0; i<areaVector->Count(); ++i )
			{
				CTFNavArea *area = areaVector->Element(i);

				area->DrawFilled( 255, 150, 150, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );

				if ( TheNavMesh->GetSelectedArea() == area )
				{
					NDebugOverlay::Text( area->GetCenter(), "Red Spawn Exit", false, NDEBUG_PERSIST_TILL_NEXT_SERVER );
				}
			}
		}
	}


	if ( tf_show_mesh_decoration.GetBool() || tf_show_mesh_decoration_manual.GetBool() )
	{
		FOR_EACH_VEC( TheNavAreas, it )
		{
			CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ it ] );
			const char *describe = "";

			if ( !tf_show_mesh_decoration_manual.GetBool() )
			{
				if ( area->HasAttributeTF( TF_NAV_HAS_AMMO ) && area->HasAttributeTF( TF_NAV_HAS_HEALTH ) )
				{
					area->DrawFilled( 255, 0, 255, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
					describe = "Health & Ammo";
				}
				else
				{
					if ( area->HasAttributeTF( TF_NAV_HAS_AMMO ) )
					{
						area->DrawFilled( 100, 100, 100, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
						describe = "Ammo";
					}
					else if ( area->HasAttributeTF( TF_NAV_HAS_HEALTH ) )
					{
						area->DrawFilled( 255, 150, 150, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
						describe = "Health";
					}
				}

				if ( area->HasAttributeTF( TF_NAV_CONTROL_POINT ) )
				{
					area->DrawFilled( 0, 255, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
					describe = "Control Point";
				}

				if ( area->HasAttributeTF( TF_NAV_BLUE_ONE_WAY_DOOR ) )
				{
					area->DrawFilled( 100, 100, 255, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				}

				if ( area->HasAttributeTF( TF_NAV_RED_ONE_WAY_DOOR ) )
				{
					area->DrawFilled( 255, 100, 100, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				}
			}

			if ( area->HasAttributeTF( TF_NAV_SNIPER_SPOT ) )
			{
				area->DrawFilled( 255, 255, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				describe = "Sniper Spot";
			}

			if ( area->HasAttributeTF( TF_NAV_SENTRY_SPOT ) )
			{
				area->DrawFilled( 255, 100, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				describe = "Sentry Spot";
			}

			if ( area->HasAttributeTF( TF_NAV_NO_SPAWNING ) )
			{
				area->DrawFilled( 100, 100, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				describe = "No Spawning";
			}

			if ( area->HasAttributeTF( TF_NAV_RESCUE_CLOSET ) )
			{
				area->DrawFilled( 0, 255, 255, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				describe = "Rescue Closet";
			}

			if ( area->HasAttributeTF( TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE ) )
			{
				area->DrawFilled( 0, 255, 255, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );

				if ( area->HasAttributeTF( TF_NAV_WITH_SECOND_POINT ) )
				{
					describe = "Blocked Until Second Point Captured";
				}
				else if ( area->HasAttributeTF( TF_NAV_WITH_THIRD_POINT ) )
				{
					describe = "Blocked Until Third Point Captured";
				}
				else if ( area->HasAttributeTF( TF_NAV_WITH_FOURTH_POINT ) )
				{
					describe = "Blocked Until Fourth Point Captured";
				}
				else if ( area->HasAttributeTF( TF_NAV_WITH_FIFTH_POINT ) )
				{
					describe = "Blocked Until Fifth Point Captured";
				}
				else
				{
					describe = "Blocked Until First Point Captured";
				}
			}
			
			if ( area->HasAttributeTF( TF_NAV_BLOCKED_AFTER_POINT_CAPTURE ) )
			{
				area->DrawFilled( 255, 255, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );

				if ( area->HasAttributeTF( TF_NAV_WITH_SECOND_POINT ) )
				{
					describe = "Blocked After Second Point Captured";
				}
				else if ( area->HasAttributeTF( TF_NAV_WITH_THIRD_POINT ) )
				{
					describe = "Blocked After Third Point Captured";
				}
				else if ( area->HasAttributeTF( TF_NAV_WITH_FOURTH_POINT ) )
				{
					describe = "Blocked After Fourth Point Captured";
				}
				else if ( area->HasAttributeTF( TF_NAV_WITH_FIFTH_POINT ) )
				{
					describe = "Blocked After Fifth Point Captured";
				}
				else
				{
					describe = "Blocked After First Point Captured";
				}
			}

			if ( area->HasAttributeTF( TF_NAV_BLUE_SETUP_GATE ) )
			{
				area->DrawFilled( 0, 0, 100, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				describe = "Blue Setup Gate";
			}
			
			if ( area->HasAttributeTF( TF_NAV_RED_SETUP_GATE ) )
			{
				area->DrawFilled( 100, 0, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				describe = "Red Setup Gate";
			}

			if ( area->HasAttributeTF( TF_NAV_DOOR_ALWAYS_BLOCKS ) )
			{
				area->DrawFilled( 100, 0, 100, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				describe = "Door Always Blocks";
			}

			if ( area->HasAttributeTF( TF_NAV_DOOR_NEVER_BLOCKS ) )
			{
				area->DrawFilled( 0, 100, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				describe = "Door Never Blocks";
			}

			if ( area->HasAttributeTF( TF_NAV_UNBLOCKABLE ) )
			{
				area->DrawFilled( 0, 200, 100, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				describe = "Unblockable";
			}

			if ( describe && TheNavMesh->GetSelectedArea() == area )
			{
				NDebugOverlay::Text( area->GetCenter(), describe, false, NDEBUG_PERSIST_TILL_NEXT_SERVER );
			}
		}
	}

	if ( tf_show_sentry_danger.GetBool() )
	{
		if ( tf_show_sentry_danger.GetInt() == 2 )
		{
			// Walk all TheNavAreas entries. Left this code in to help debug in case
			//	TheNavAreas is never not _exactly_ the same as m_sentryAreas.
			FOR_EACH_VEC( TheNavAreas, it )
			{
				const CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ it ] );
				int r = area->HasAttributeTF( TF_NAV_RED_SENTRY_DANGER ) * 255;
				int b = area->HasAttributeTF( TF_NAV_BLUE_SENTRY_DANGER ) * 255;

				if ( r || b )
				{
					area->DrawFilled( r, 0, b, 80, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				}
			}
		}
		else
		{
			// Only go through the m_SentryAreas entries. Should be the same as walking the
			// 	entire TheNavAreas, but a lot faster.
			FOR_EACH_VEC( m_sentryAreas, nit )
			{
				const CTFNavArea *area = m_sentryAreas[ nit ];
				int r = area->HasAttributeTF( TF_NAV_RED_SENTRY_DANGER ) * 255;
				int b = area->HasAttributeTF( TF_NAV_BLUE_SENTRY_DANGER ) * 255;

				if ( r || b )
				{
					area->DrawFilled( r, 0, b, 80, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				}
			}
		}
	}

	if ( tf_show_actor_potential_visibility.GetBool() )
	{
		FOR_EACH_VEC( TheNavAreas, it )
		{
			CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ it ] );

			if ( area->IsPotentiallyVisibleToTeam( TF_TEAM_BLUE ) )
			{
				if ( area->IsPotentiallyVisibleToTeam( TF_TEAM_RED ) )
				{
					area->DrawFilled( 255, 0, 255, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				}
				else
				{
					area->DrawFilled( 0, 0, 255, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				}
			}
			else if ( area->IsPotentiallyVisibleToTeam( TF_TEAM_RED ) )
			{
				area->DrawFilled( 255, 0, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
			}
		}		
	}

/*
	if ( tf_show_gate_defense_areas.GetBool() )
	{
		FOR_EACH_VEC( TheNavAreas, it )
		{
			CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ it ] );

			if ( area->HasAttributeTF( TF_NAV_DEFEND_SETUP_GATES ) )
			{
				if ( area->HasAttributeTF( TF_NAV_DEFEND_VIA_SNIPING ) )
					area->DrawFilled( 0, 255, 255, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				else if ( area->HasAttributeTF( TF_NAV_DEFEND_VIA_AMBUSH ) )
					area->DrawFilled( 255, 0, 255, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				else
					area->DrawFilled( 0, 0, 255, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
			}
		}
	}

	if ( tf_show_point_defense_areas.GetBool() )
	{
		FOR_EACH_VEC( TheNavAreas, it )
		{
			CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ it ] );

			if ( area->HasAttributeTF( TF_NAV_DEFEND_POINT ) )
			{
				if ( area->HasAttributeTF( TF_NAV_DEFEND_VIA_SNIPING ) )
					area->DrawFilled( 0, 255, 100, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				else if ( area->HasAttributeTF( TF_NAV_DEFEND_VIA_AMBUSH ) )
					area->DrawFilled( 255, 150, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
				else
					area->DrawFilled( 0, 150, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER, true );
			}
		}
	}
*/

	if ( tf_show_control_points.GetBool() )
	{
		for( int which=0; which<MAX_CONTROL_POINTS; ++which )
		{
			for( int i=0; i<m_controlPointAreaVector[ which ].Count(); ++i )
			{
				CTFNavArea *area = m_controlPointAreaVector[ which ][ i ];

				if ( m_controlPointCenterAreaVector[ which ] == area )
				{
					area->DrawFilled( 255, 255, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER );
				}
				else
				{
					area->DrawFilled( 255, 150, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER );
				}
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------
/**
 * Populate the given "ambushVector" with good areas to lurk in ambush for the invading enemy team
 */
void CTFNavMesh::CollectAmbushAreas( CUtlVector< CTFNavArea * > *ambushVector, CTFNavArea *startArea, int teamToAmbush, float searchRadius, float incursionTolerance ) const
{
	ScanSelectAmbushAreas selector( ambushVector, teamToAmbush, startArea->GetIncursionDistance( teamToAmbush ) + incursionTolerance );
	SearchSurroundingAreas( startArea, startArea->GetCenter(), selector, searchRadius );
}


//--------------------------------------------------------------------------------------------------------
/**
 * Populate the given vector with areas that are just outside of the given team's spawn room(s)
 */
void CTFNavMesh::CollectSpawnRoomThresholdAreas( CUtlVector< CTFNavArea * > *spawnExitAreaVector, int team ) const
{
	const CUtlVector< CTFNavArea * > *exitAreaVector = GetSpawnRoomExitAreas( team );

	if ( !exitAreaVector )
		return;

	for( int i=0; i<exitAreaVector->Count(); ++i )
	{
		CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ i ] );

		// find largest non-spawn-room area connected to this exit
		CTFNavArea *exitArea = NULL;
		float exitAreaSize = 0.0f;

		for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
		{
			const NavConnectVector *adjConnect = area->GetAdjacentAreas( (NavDirType)dir );

			for( int j=0; j<adjConnect->Count(); ++j )
			{
				CTFNavArea *adjArea = (CTFNavArea *)adjConnect->Element(j).area;

				if ( !adjArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_RED | TF_NAV_SPAWN_ROOM_BLUE | TF_NAV_SPAWN_ROOM_EXIT ) )
				{
					// this area is outside of the spawn room
					float size = adjArea->GetSizeX() * adjArea->GetSizeY();
					if ( size > exitAreaSize )
					{
						exitArea = adjArea;
						exitAreaSize = size;
					}
				}
			}
		}

		if ( exitArea )
		{
			spawnExitAreaVector->AddToTail( exitArea );
		}
	}
}


//--------------------------------------------------------------------------------------------------------
// Populate the given vector with areas that have a bomb travel distance within the given range
void CTFNavMesh::CollectAreaWithinBombTravelRange( CUtlVector< CTFNavArea * > *spawnExitAreaVector, float minTravel, float maxTravel ) const
{
	for( int i=0; i<TheNavAreas.Count(); ++i )
	{
		CTFNavArea *area = static_cast< CTFNavArea * >( TheNavAreas[ i ] );

		float travelDistance = area->GetTravelDistanceToBombTarget();

		if ( travelDistance >= minTravel && travelDistance <= maxTravel )
		{
			spawnExitAreaVector->AddToTail( area );
		}
	}
}
