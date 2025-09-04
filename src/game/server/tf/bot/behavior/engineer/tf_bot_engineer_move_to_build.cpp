//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_engineer_move_to_build.cpp
// Engineer moving into position to build
// Michael Booth, February 2009

#include "cbase.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_obj_sentrygun.h"
#include "tf_weapon_builder.h"
#include "team_train_watcher.h"
#include "bot/tf_bot.h"
#include "bot/behavior/engineer/tf_bot_engineer_build.h"
#include "bot/behavior/engineer/tf_bot_engineer_move_to_build.h"
#include "bot/behavior/engineer/tf_bot_engineer_building.h"
#include "bot/map_entities/tf_bot_hint_sentrygun.h"
#include "bot/behavior/tf_bot_get_ammo.h"
#include "bot/behavior/tf_bot_retreat_to_cover.h"
#include "bot/behavior/engineer/tf_bot_engineer_build_teleport_exit.h"
#include "trigger_area_capture.h"

#include "raid/tf_raid_logic.h"

// Guarded Engineer behavior seams (header-only; default off)
#include "bot/behavior/engineer/tf_bot_engineer_seams.h"


extern ConVar tf_bot_path_lookahead_range;

ConVar tf_bot_debug_sentry_placement( "tf_bot_debug_sentry_placement", "0", FCVAR_CHEAT );
ConVar tf_bot_max_teleport_exit_travel_to_point( "tf_bot_max_teleport_exit_travel_to_point", "2500", FCVAR_CHEAT, "In an offensive engineer bot's tele exit is farther from the point than this, destroy it" );
ConVar tf_bot_min_teleport_travel( "tf_bot_min_teleport_travel", "3000", FCVAR_CHEAT, "Minimum travel distance between teleporter entrance and exit before engineer bot will build one" );
#if TF_BOT_ENGINEER_SEAMS
// Lightweight guardable traces for seam heuristics
ConVar tf_bot_engineer_seams_debug( "tf_bot_engineer_seams_debug", "0", FCVAR_CHEAT, "Debug Engineer seams heuristics" );
// Tunable cvars for guarded placement heuristics (default-off behavior parity)
ConVar tf_bot_engineer_seams_desired_range( "tf_bot_engineer_seams_desired_range", "0", FCVAR_CHEAT, "Desired max sentry placement range (0=default)" );
ConVar tf_bot_engineer_seams_nest_spacing_min( "tf_bot_engineer_seams_nest_spacing_min", "0", FCVAR_CHEAT, "Minimum distance between friendly sentry nests (0=default)" );
ConVar tf_bot_engineer_seams_height_bias_weight( "tf_bot_engineer_seams_height_bias_weight", "0", FCVAR_CHEAT, "Weight to prefer areas above the objective (0=disabled)" );
ConVar tf_bot_engineer_seams_hint_radius( "tf_bot_engineer_seams_hint_radius", "900", FCVAR_CHEAT, "Proximity radius for hint-aware bias" );
ConVar tf_bot_engineer_seams_hint_bias_weight( "tf_bot_engineer_seams_hint_bias_weight", "0", FCVAR_CHEAT, "Additional bias weight for hint proximity (0=none)" );
#endif

//--------------------------------------------------------------------------------------------------------
static Vector s_pointCentroid;

int CompareRangeToPoint( CTFNavArea * const *area1, CTFNavArea * const *area2 )
{
	float d1 = ( (*area1)->GetCenter() - s_pointCentroid ).LengthSqr();
	float d2 = ( (*area2)->GetCenter() - s_pointCentroid ).LengthSqr();

	// reversed so farthest is sorted first in the vector
	if ( d1 < d2 )
		return 1;

	if ( d1 > d2 )
		return -1;

	return 0;
}


//---------------------------------------------------------------------------------------------
void CTFBotEngineerMoveToBuild::CollectBuildAreas( CTFBot *me )
{
	// if we have a predesignated build area, we're done
	if ( me->GetHomeArea() )
		return;

	m_sentryAreaVector.RemoveAll();

	CUtlVector< CTFNavArea * > pointAreaVector;
	Vector pointCentroid = vec3_origin;
	float pointEnemyIncursion = 0.0f;
	int i;

	int myTeam = me->GetTeamNumber();
	int enemyTeam = ( myTeam == TF_TEAM_BLUE ) ? TF_TEAM_RED : TF_TEAM_BLUE;

	CCaptureZone *zone = me->GetFlagCaptureZone();
	if ( zone )
	{
		// NOTE: Not strictly the right thing - should defend location of our team's flag
		CTFNavArea *zoneArea = (CTFNavArea *)TheTFNavMesh()->GetNearestNavArea( zone->WorldSpaceCenter(), false, 500.0f, true );
		if ( zoneArea )
		{
			pointAreaVector.AddToTail( zoneArea );
			pointCentroid += zoneArea->GetCenter();
			pointEnemyIncursion += zoneArea->GetIncursionDistance( enemyTeam );
		}
	}
	else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		CTeamTrainWatcher *trainWatcher;

		if ( myTeam == TF_TEAM_BLUE )
		{
			trainWatcher = TFGameRules()->GetPayloadToPush( me->GetTeamNumber() );
		}
		else
		{
			trainWatcher = TFGameRules()->GetPayloadToBlock( me->GetTeamNumber() );
		}

		if ( trainWatcher )
		{
			Vector checkpointPos = trainWatcher->GetNextCheckpointPosition();

			CTFNavArea *checkpointArea = (CTFNavArea *)TheTFNavMesh()->GetNearestNavArea( checkpointPos, false, 500.0f, true );
			if ( checkpointArea )
			{
				pointAreaVector.AddToTail( checkpointArea );
				pointCentroid += checkpointArea->GetCenter();
				pointEnemyIncursion += checkpointArea->GetIncursionDistance( enemyTeam );
			}
		}
	}
	else
	{
		// collect all areas overlapping the point
		CTeamControlPoint *ctrlPoint = me->GetMyControlPoint();
		if ( !ctrlPoint )
			return;

		const CUtlVector< CTFNavArea * > *ctrlPointAreaVector = TheTFNavMesh()->GetControlPointAreas( ctrlPoint->GetPointIndex() );

		if ( ctrlPointAreaVector )
		{
			for( i=0; i<ctrlPointAreaVector->Count(); ++i )
			{
				CTFNavArea *area = ctrlPointAreaVector->Element(i);

				pointAreaVector.AddToTail( area );
				pointCentroid += area->GetCenter();
				pointEnemyIncursion += area->GetIncursionDistance( enemyTeam );
			}
		}
	}

	if ( pointAreaVector.Count() == 0 )
		return;

	pointCentroid /= pointAreaVector.Count();
	pointEnemyIncursion /= pointAreaVector.Count();


	// collect all areas that can see the point
	CUtlVector< CTFNavArea * > exposedAreaVector;

#if TF_BOT_ENGINEER_SEAMS
	int seam_range_rejects = 0;
	int seam_spacing_rejects = 0;
	const int kSeamMaxSamples = 3;
	int seam_range_samples = 0;
	int seam_spacing_samples = 0;
#endif
	for( i=0; i<pointAreaVector.Count(); ++i )
	{
		CTFAreaCollector collect;
		pointAreaVector[i]->ForAllPotentiallyVisibleAreas( collect );

		for( int j=0; j<collect.m_vector.Count(); ++j )
		{
			CTFNavArea *visibleArea = collect.m_vector[j];


			if ( visibleArea->GetIncursionDistance( myTeam ) < 0 || visibleArea->GetIncursionDistance( enemyTeam ) < 0 )
				continue;

			if ( TFGameRules()->IsInKothMode() )
			{
				// ignore areas the enemy can reach first
				if ( visibleArea->GetIncursionDistance( myTeam ) >= visibleArea->GetIncursionDistance( enemyTeam ) )
					continue;
			}

// incursion flow is badly behaved at cap #1, stage #2 in dustbowl
// 			else
// 			{
// 				if ( pointEnemyIncursion > visibleArea->GetIncursionDistance( enemyTeam ) )
// 					continue;
// 			}

			if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP )
			{
				// don't build directly on the point
				if ( visibleArea->HasAttributeTF( TF_NAV_CONTROL_POINT ) )
					continue;

				// ignore areas below the point
				const float tooFarBelow = 150.0f;
				if ( visibleArea->GetCenter().z < pointCentroid.z - tooFarBelow )
					continue;

				// ignore areas too far from the point for the sentry gun to reach
				const float tolerance = 1.1f;
#if TF_BOT_ENGINEER_SEAMS
				float desiredMaxRange = SENTRY_MAX_RANGE * tolerance;
				// Allow future tuning of desired placement range; if none provided, slightly prefer mid-range
				{
					float seamRange = Seam_DesiredPlacementRange( me, NULL );
					if ( seamRange > 0.0f )
					{
						desiredMaxRange = seamRange * tolerance;
					}
					else
					{
						// If seam didn't provide, check guarded cvar for desired range
						float cvarRange = tf_bot_engineer_seams_desired_range.GetFloat();
						if ( cvarRange > 0.0f )
						{
							desiredMaxRange = cvarRange * tolerance;
						}
						else
						{
							// Nudge down the max range a bit to bias toward mid-range
							desiredMaxRange *= 0.9f;
						}
					}
				}
				{
					Vector toPoint = visibleArea->GetCenter() - pointCentroid;
					if ( toPoint.IsLengthGreaterThan( desiredMaxRange ) )
					{
						seam_range_rejects++;
						if ( seam_range_samples < kSeamMaxSamples )
						{
							seam_range_samples++;
							DevMsg( "[TF-ENG seam] Range reject: len=%.1f > max=%.1f at (%.0f,%.0f,%.0f)\n",
								toPoint.Length(), desiredMaxRange,
								visibleArea->GetCenter().x, visibleArea->GetCenter().y, visibleArea->GetCenter().z );
						}
						continue;
					}
				}
#else
				if ( ( visibleArea->GetCenter() - pointCentroid ).IsLengthGreaterThan( SENTRY_MAX_RANGE * tolerance ) )
					continue;
#endif
			}

			// ignore areas that don't have clear line of FIRE (not sight)
			const float sentryEyeHeight = 60.0f;
			const float pointFlagHeight = 70.0f; // 100.0f;
			if ( !me->IsLineOfFireClear( visibleArea->GetCenter() + Vector( 0, 0, sentryEyeHeight ), pointCentroid + Vector( 0, 0, pointFlagHeight ) ) )
				continue;

				// Optional nest spacing filter (guarded)

#if TF_BOT_ENGINEER_SEAMS
				{
					float nestMin = Seam_NestSpacingMin( me );
					if ( nestMin <= 0.0f )
					{
						// If seam didn't provide, check guarded cvar; fall back to 400
						nestMin = tf_bot_engineer_seams_nest_spacing_min.GetFloat();
						if ( nestMin <= 0.0f )
						{
							nestMin = 400.0f;
						}
					}
					if ( nestMin > 0.0f )
					{
						bool tooCloseToNest = false;
						float nestDist = 0.0f;
						// Scan existing friendly sentry guns and avoid clustering
						for ( CBaseEntity *ent = gEntList.FindEntityByClassname( NULL, "obj_sentrygun" ); ent; ent = gEntList.FindEntityByClassname( ent, "obj_sentrygun" ) )
						{
							CObjectSentrygun *pSentry = dynamic_cast< CObjectSentrygun * >( ent );
							if ( pSentry && pSentry->GetTeamNumber() == me->GetTeamNumber() )
							{
								Vector d = pSentry->GetAbsOrigin() - visibleArea->GetCenter();
								if ( d.IsLengthLessThan( nestMin ) )
								{
									nestDist = d.Length();
									tooCloseToNest = true;
									break;
								}
							}
						}
						if ( tooCloseToNest )
						{
							seam_spacing_rejects++;
							if ( seam_spacing_samples < kSeamMaxSamples )
							{
								seam_spacing_samples++;
								DevMsg( "[TF-ENG seam] Spacing reject: dist=%.1f < min=%.1f at (%.0f,%.0f,%.0f)\n",
									nestDist, nestMin,
									visibleArea->GetCenter().x, visibleArea->GetCenter().y, visibleArea->GetCenter().z );
							}
							continue;
						}
					}
				}
#endif

				if ( !exposedAreaVector.HasElement( visibleArea ) )
					exposedAreaVector.AddToTail( visibleArea );
		}
	}

	// keep the farthest away areas
	const float keepRatio = 1.0f; // 0.5f;
	s_pointCentroid = pointCentroid;
	exposedAreaVector.Sort( CompareRangeToPoint );

#if TF_BOT_ENGINEER_SEAMS
	// Light height bias: prefer areas with positive bias
	if ( exposedAreaVector.Count() > 1 )
	{
		CUtlVector< CTFNavArea * > preferred;
		CUtlVector< CTFNavArea * > others;
		FOR_EACH_VEC( exposedAreaVector, itBias )
		{
			CTFNavArea *area = exposedAreaVector[ itBias ];
			float bias = Seam_HeightBias( me, area->GetCenter() );
			if ( bias == 0.0f )
			{
				// If seam didn't provide, use optional weighted bias; else parity 0/1
				float weight = tf_bot_engineer_seams_height_bias_weight.GetFloat();
				if ( weight > 0.0f )
				{
					bias = ( area->GetCenter().z > pointCentroid.z ) ? weight : 0.0f;
				}
				else
				{
					// Fallback: prefer areas slightly above the point centroid
					bias = ( area->GetCenter().z > pointCentroid.z ) ? 1.0f : 0.0f;
				}
			}
			if ( bias > 0.0f )
				preferred.AddToTail( area );
			else
				others.AddToTail( area );
		}
		exposedAreaVector.RemoveAll();
		FOR_EACH_VEC( preferred, ip )
		{
			exposedAreaVector.AddToTail( preferred[ ip ] );
		}
		FOR_EACH_VEC( others, io )
		{
			exposedAreaVector.AddToTail( others[ io ] );
		}

		DevMsg( "[TF-ENG seam] Height bias: preferred=%d, others=%d (after range/spacing rejects: range=%d, spacing=%d)\n",
			preferred.Count(), others.Count(), seam_range_rejects, seam_spacing_rejects );
	}
#endif

#if TF_BOT_ENGINEER_SEAMS
	// Hint-aware bias: prefer areas near enabled, team-matching sentry hints
	if ( exposedAreaVector.Count() > 1 )
	{
		CUtlVector< CTFBotHintSentrygun * > teamHints;
		CTFBotHintSentrygun *sentryHint;
		for ( sentryHint = static_cast< CTFBotHintSentrygun * >( gEntList.FindEntityByClassname( NULL, "bot_hint_sentrygun" ) );
			  sentryHint;
			  sentryHint = static_cast< CTFBotHintSentrygun * >( gEntList.FindEntityByClassname( sentryHint, "bot_hint_sentrygun" ) ) )
		{
			if ( sentryHint->IsAvailableForSelection( me ) )
			{
				teamHints.AddToTail( sentryHint );
			}
		}

		if ( teamHints.Count() > 0 )
		{
			CUtlVector< CTFNavArea * > preferredByHint;
			CUtlVector< CTFNavArea * > othersByHint;

			// Proximity radius from guarded cvar with sane fallback
			float hintRadius = tf_bot_engineer_seams_hint_radius.GetFloat();
			if ( hintRadius <= 0.0f )
			{
				hintRadius = 900.0f; // default radius
			}
			FOR_EACH_VEC( exposedAreaVector, itHint )
			{
				CTFNavArea *area = exposedAreaVector[ itHint ];
				Vector pos = area->GetCenter();

				// Find nearest hint
				CTFBotHintSentrygun *nearest = NULL;
				float bestDistSqr = FLT_MAX;
				FOR_EACH_VEC( teamHints, ih )
				{
					CTFBotHintSentrygun *h = teamHints[ ih ];
					float d2 = ( h->GetAbsOrigin() - pos ).LengthSqr();
					if ( d2 < bestDistSqr )
					{
						bestDistSqr = d2;
						nearest = h;
					}
				}

				float hintBias = 0.0f;
				if ( nearest )
				{
					float seamBias = Seam_HintBias( me, nearest );
					if ( seamBias != 0.0f )
					{
						// Use provided seam bias directly when present
						hintBias = seamBias;
					}
					else
					{
						// Proximity fallback: positive within radius, stronger when closer
						float bestDist = FastSqrt( bestDistSqr );
						if ( bestDist < hintRadius )
						{
							hintBias = ( hintRadius - bestDist ) / hintRadius;
						}
						// Add simple global weight for hint proximity bias
						hintBias += tf_bot_engineer_seams_hint_bias_weight.GetFloat();
					}
				}

				if ( hintBias > 0.0f )
				{
					preferredByHint.AddToTail( area );
				}
				else
				{
					othersByHint.AddToTail( area );
				}
			}

			exposedAreaVector.RemoveAll();
			FOR_EACH_VEC( preferredByHint, ipb )
			{
				exposedAreaVector.AddToTail( preferredByHint[ ipb ] );
			}
			FOR_EACH_VEC( othersByHint, iob )
			{
				exposedAreaVector.AddToTail( othersByHint[ iob ] );
			}

			if ( tf_bot_engineer_seams_debug.GetBool() && developer.GetBool() )
			{
				DevMsg( "[TF-ENG seam] Hint bias: preferred=%d, others=%d, hints=%d\n",
					preferredByHint.Count(), othersByHint.Count(), teamHints.Count() );
			}
		}
	}
#endif

	for( i=0; i<exposedAreaVector.Count() * keepRatio; ++i )
	{
		CTFNavArea *usableArea = exposedAreaVector[i];

		m_sentryAreaVector.AddToTail( usableArea );
	}

	// calculate total surface area
	m_totalSurfaceArea = 0.0f;
	FOR_EACH_VEC( m_sentryAreaVector, it )
	{
		CTFNavArea *area = m_sentryAreaVector[ it ];

		m_totalSurfaceArea += area->GetSizeX() * area->GetSizeY();

		if ( tf_bot_debug_sentry_placement.GetBool() )
		{
			TheNavMesh->AddToSelectedSet( area );
		}
	}

#if TF_BOT_ENGINEER_SEAMS
	DevMsg( "[TF-ENG seam] CollectBuildAreas: kept_areas=%d total_surface=%.0f\n",
		m_sentryAreaVector.Count(), m_totalSurfaceArea );
#endif
}


//---------------------------------------------------------------------------------------------
/**
 * Doesn't recompute the potential areas, just reselected from the list
 */
void CTFBotEngineerMoveToBuild::SelectBuildLocation( CTFBot *me )
{
	m_path.Invalidate();

	m_sentryBuildHint = NULL;
	m_sentryBuildLocation = vec3_origin;


	// if we have a build spot, use it
	if ( me->GetHomeArea() )
	{
		m_sentryBuildLocation = me->GetHomeArea()->GetCenter();
#if TF_BOT_ENGINEER_SEAMS
		DevMsg( "[TF-ENG seam] SelectBuildLocation: using home area at (%.0f,%.0f,%.0f)\n",
			m_sentryBuildLocation.x, m_sentryBuildLocation.y, m_sentryBuildLocation.z );
#endif
		return;
	}

	// if we have a set of specific build locations, pick one of them
	CUtlVector< CTFBotHintSentrygun * > sentryHintVector;

	CTFBotHintSentrygun *sentryHint;
	for( sentryHint = static_cast< CTFBotHintSentrygun * >( gEntList.FindEntityByClassname( NULL, "bot_hint_sentrygun" ) );
		 sentryHint;
		 sentryHint = static_cast< CTFBotHintSentrygun * >( gEntList.FindEntityByClassname( sentryHint, "bot_hint_sentrygun" ) ) )
	{
		// clear the previous owner if it is us
		if ( sentryHint->GetPlayerOwner() == me )
		{
			sentryHint->SetPlayerOwner( NULL );
		}
		if ( sentryHint->IsAvailableForSelection( me ) )
		{
			sentryHintVector.AddToTail( sentryHint );
		}
	}

	if ( sentryHintVector.Count() > 0 )
	{
		int which = RandomInt( 0, sentryHintVector.Count()-1 );

		m_sentryBuildHint = sentryHintVector[ which ];
		m_sentryBuildHint->SetPlayerOwner( me );
		m_sentryBuildLocation = m_sentryBuildHint->GetAbsOrigin();

#if TF_BOT_ENGINEER_SEAMS
		DevMsg( "[TF-ENG seam] SelectBuildLocation: using sentry hint at (%.0f,%.0f,%.0f)\n",
			m_sentryBuildLocation.x, m_sentryBuildLocation.y, m_sentryBuildLocation.z );
#endif

		return;
	}


	// collect nav area candidates
	CollectBuildAreas( me );

	// choose based on surface area to avoid biasing finely subdivided areas of the mesh
	float which = RandomFloat( 0.0f, m_totalSurfaceArea - 1.0f );
	float soFar = 0.0f;
	FOR_EACH_VEC( m_sentryAreaVector, sit )
	{
		CTFNavArea *area = m_sentryAreaVector[ sit ];

		soFar += area->GetSizeX() * area->GetSizeY();

			if ( which < soFar )
			{
				m_sentryBuildLocation = area->GetRandomPoint();
#if TF_BOT_ENGINEER_SEAMS
				DevMsg( "[TF-ENG seam] SelectBuildLocation: selected area center=(%.0f,%.0f,%.0f) random_point=(%.0f,%.0f,%.0f)\n",
					area->GetCenter().x, area->GetCenter().y, area->GetCenter().z,
					m_sentryBuildLocation.x, m_sentryBuildLocation.y, m_sentryBuildLocation.z );
				// Optional coarse trace: nearest hint to chosen area
				if ( tf_bot_engineer_seams_debug.GetBool() && developer.GetBool() )
				{
					CTFBotHintSentrygun *sentryHint;
					CTFBotHintSentrygun *nearest = NULL;
					float bestDistSqr = FLT_MAX;
					for ( sentryHint = static_cast< CTFBotHintSentrygun * >( gEntList.FindEntityByClassname( NULL, "bot_hint_sentrygun" ) );
						  sentryHint;
						  sentryHint = static_cast< CTFBotHintSentrygun * >( gEntList.FindEntityByClassname( sentryHint, "bot_hint_sentrygun" ) ) )
					{
						if ( sentryHint->IsAvailableForSelection( me ) )
						{
							float d2 = ( sentryHint->GetAbsOrigin() - area->GetCenter() ).LengthSqr();
							if ( d2 < bestDistSqr )
							{
								bestDistSqr = d2;
								nearest = sentryHint;
							}
						}
					}
					if ( nearest )
					{
						Vector hc = nearest->GetAbsOrigin();
						Vector ac = area->GetCenter();
						DevMsg( "[TF-ENG seam] Hint trace: area_center=(%.0f,%.0f,%.0f) nearest_hint=(%.0f,%.0f,%.0f) dist=%.0f\n",
							ac.x, ac.y, ac.z, hc.x, hc.y, hc.z, FastSqrt( bestDistSqr ) );
					}
				}
#endif
				return;
			}
	}

	if ( !HushAsserts() )
	{
		Assert( !"Failed to find a build location" );
	}
	m_sentryBuildLocation = me->GetAbsOrigin();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEngineerMoveToBuild::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

#ifdef TF_RAID_MODE
	if ( TFGameRules()->IsRaidMode() )
	{
		if ( me->GetHomeArea() && TFGameRules()->GetRaidLogic() )
		{
			// try to pick a new area
			CTFNavArea *sentryArea = TFGameRules()->GetRaidLogic()->SelectRaidSentryArea();
			if ( sentryArea )
			{
				me->SetHomeArea( sentryArea );
			}
		}
	}
#endif // TF_RAID_MODE

	SelectBuildLocation( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEngineerMoveToBuild::Update( CTFBot *me, float interval )
{
	if ( m_fallBackTimer.HasStarted() )
	{
		if ( m_fallBackTimer.IsElapsed() )
		{
			SelectBuildLocation( me );
			m_fallBackTimer.Invalidate();
		}
		else
		{
			// wait a moment while we decide where to build near fallback point
			return Continue();
		}
	}

	CBaseObject	*mySentry = me->GetObjectOfType( OBJ_SENTRYGUN );
	if ( mySentry )
	{
		// we already have a sentry from a previous life - continue what we were doing

		// if we used a sentry hint last time, reuse it
		CTFBotHintSentrygun *sentryHint;
		for( sentryHint = static_cast< CTFBotHintSentrygun * >( gEntList.FindEntityByClassname( NULL, "bot_hint_sentrygun" ) );
			 sentryHint;
			 sentryHint = static_cast< CTFBotHintSentrygun * >( gEntList.FindEntityByClassname( sentryHint, "bot_hint_sentrygun" ) ) )
		{
			if ( sentryHint->GetPlayerOwner() == me )
			{
				return ChangeTo( new CTFBotEngineerBuilding( sentryHint ), "Going back to my existing sentry nest and reusing a sentry hint" );
			}
		}

		return ChangeTo( new CTFBotEngineerBuilding, "Going back to my existing sentry nest" );
	}

	// offensive engineers need to place a forward teleporter
	if ( ( TFGameRules()->IsAttackDefenseMode() && me->GetTeamNumber() == TF_TEAM_BLUE ) ||
		 ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP && !TFGameRules()->IsAttackDefenseMode() && !TFGameRules()->IsInKothMode() ) )
	{
		CObjectTeleporter *myTeleportExit = (CObjectTeleporter *)me->GetObjectOfType( OBJ_TELEPORTER, MODE_TELEPORTER_EXIT );
		int myTeam = me->GetTeamNumber();

		if ( myTeleportExit )
		{
			// if exit is too far from the point, destroy it and try again
			CTeamControlPoint *point = me->GetMyControlPoint();
			if ( point )
			{
				CTFNavArea *pointArea = TheTFNavMesh()->GetControlPointCenterArea( point->GetPointIndex() );

				myTeleportExit->UpdateLastKnownArea();
				CTFNavArea *exitArea = (CTFNavArea *)myTeleportExit->GetLastKnownArea();

				if ( pointArea && exitArea )
				{
					float travelToPoint = fabs( exitArea->GetIncursionDistance( myTeam ) - pointArea->GetIncursionDistance( myTeam ) );

					if ( travelToPoint > tf_bot_max_teleport_exit_travel_to_point.GetFloat() )
					{
						// too far, destroy it
						myTeleportExit->DestroyObject();
						myTeleportExit = NULL;
					}
				}
			}
		}
		else
		{
			CObjectTeleporter *myTeleportEntrance = (CObjectTeleporter *)me->GetObjectOfType( OBJ_TELEPORTER, MODE_TELEPORTER_ENTRANCE );
			CTFNavArea *myArea = me->GetLastKnownArea();

			bool shouldBuildExit = true;

			// if we have a teleporter entrance, don't place the exit too close to it
			if ( myTeleportEntrance && myArea )
			{
				myTeleportEntrance->UpdateLastKnownArea();
				CTFNavArea *enterArea = (CTFNavArea *)myTeleportEntrance->GetLastKnownArea();

				if ( enterArea )
				{
					float travelBetween = fabs( enterArea->GetIncursionDistance( myTeam ) - myArea->GetIncursionDistance( myTeam ) );

					if ( travelBetween < tf_bot_min_teleport_travel.GetFloat() )
					{
						shouldBuildExit = false;
					}
				}
			}

			if ( shouldBuildExit )
			{
				// no exit yet - need to place one
				// when we see the enemy, retreat to cover and build the exit there
				if ( me->GetVisionInterface()->GetPrimaryKnownThreat( true ) )
				{
					if ( !me->m_Shared.InCond( TF_COND_INVULNERABLE ) && ShouldRetreat( me ) != ANSWER_NO )
					{
						Action< CTFBot > *nextActionWhenInCover = new CTFBotEngineerBuildTeleportExit;
						return SuspendFor( new CTFBotRetreatToCover( nextActionWhenInCover ), "Retreating to a safe place to build my teleporter exit" );
					}
				}
			}
		}
	}

	// move to build position
	if ( m_repathTimer.IsElapsed() )
	{
		m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

		CTFBotPathCost cost( me, SAFEST_ROUTE );
		m_path.Compute( me, m_sentryBuildLocation, cost );
	}

	Vector forward;
	me->EyeVectors( &forward );
	forward.z = 0.0f;
	forward.NormalizeInPlace();

	Vector myBlueprintPosition = me->GetAbsOrigin() + 50.0f * forward;

	const float closeToHome = 25.0f;
	Vector toBuild = m_sentryBuildLocation - myBlueprintPosition;
	Vector toMe = m_sentryBuildLocation - me->GetAbsOrigin();

	if ( me->GetLocomotionInterface()->IsOnGround() )
	{
		// we need to wait until we're on the ground since the Build action assumes our position OnStart is where we are going to build
		if ( toMe.AsVector2D().IsLengthLessThan( closeToHome ) || toBuild.AsVector2D().IsLengthLessThan( closeToHome ) )
		{
			if ( m_sentryBuildHint != NULL )
			{
				return ChangeTo( new CTFBotEngineerBuilding( m_sentryBuildHint ), "Reached my precise build location" );
			}

			return ChangeTo( new CTFBotEngineerBuilding, "Reached my build location" );
		}

		m_path.Update( me );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotEngineerMoveToBuild::OnStuck( CTFBot *me )
{
//	SelectBuildLocation( me );
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotEngineerMoveToBuild::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotEngineerMoveToBuild::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	SelectBuildLocation( me );

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotEngineerMoveToBuild::OnTerritoryLost( CTFBot *me, int territoryID )
{
	// we have to wait a moment until contested point changes to select a new build spot
	m_fallBackTimer.Start( 0.2f );

	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotEngineerMoveToBuild::OnTerritoryCaptured( CTFBot *me, int territoryID )
{
	// we have to wait a moment until contested point changes to select a new build spot
	m_fallBackTimer.Start( 0.2f );

	return TryContinue();
}
