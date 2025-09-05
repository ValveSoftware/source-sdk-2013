//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_engineer_building.cpp
// At building location, constructing buildings
// Michael Booth, May 2010

#include "cbase.h"
#include "nav_mesh.h"
#include "tf_player.h"
#include "tf_obj.h"
#include "tf_obj_sentrygun.h"
#include "tf_obj_dispenser.h"
#include "tf_gamerules.h"
#include "tf_weapon_builder.h"
#include "team_train_watcher.h"
#include "bot/tf_bot.h"
#include "bot/behavior/engineer/tf_bot_engineer_building.h"
#include "bot/behavior/engineer/tf_bot_engineer_move_to_build.h"
#include "bot/behavior/engineer/tf_bot_engineer_build_teleport_exit.h"
#include "bot/behavior/engineer/tf_bot_engineer_build_sentrygun.h"
#include "bot/behavior/engineer/tf_bot_engineer_build_dispenser.h"
#include "bot/behavior/tf_bot_attack.h"
#include "bot/behavior/tf_bot_get_ammo.h"
#include "bot/map_entities/tf_bot_hint_teleporter_exit.h"
#include "bot/map_entities/tf_bot_hint_sentrygun.h"
#include "NextBotUtil.h"

// Guarded Engineer behavior seams (header-only; default off)
#include "bot/behavior/engineer/tf_bot_engineer_seams.h"

#if TF_BOT_ENGINEER_SEAMS
extern ConVar tf_bot_engineer_seams_debug;

// Minimum useful travel distance between teleporter ends (fallback heuristic)
ConVar tf_bot_engineer_seams_min_teleport_travel( "tf_bot_engineer_seams_min_teleport_travel", "3000", FCVAR_CHEAT, "Minimum useful teleporter travel distance" );

// Guarded controls for teleporter redeploy behavior
ConVar tf_bot_engineer_seams_teleport_redeploy( "tf_bot_engineer_seams_teleport_redeploy", "0", FCVAR_CHEAT, "Enable guarded teleporter redeploy when invalid/suboptimal" );
ConVar tf_bot_engineer_seams_teleport_redeploy_cooldown( "tf_bot_engineer_seams_teleport_redeploy_cooldown", "15", FCVAR_CHEAT, "Cooldown before attempting another teleporter redeploy (seconds)" );

// Guarded control for repair-first policy under health fraction threshold
ConVar tf_bot_engineer_seams_repair_health_frac( "tf_bot_engineer_seams_repair_health_frac", "0", FCVAR_CHEAT, "Health fraction below which repair takes priority (0=disabled)" );

// Guarded link-check rate limit to reduce noise/work
ConVar tf_bot_engineer_seams_teleport_link_check_cooldown( "tf_bot_engineer_seams_teleport_link_check_cooldown", "2", FCVAR_CHEAT, "Cooldown between teleporter link checks (seconds)" );

// Guarded controls for maintenance metal reserve and upgrade cap
ConVar tf_bot_engineer_seams_metal_reserve( "tf_bot_engineer_seams_metal_reserve", "0", FCVAR_CHEAT, "Minimum metal to keep in reserve (0=disabled)" );
ConVar tf_bot_engineer_seams_upgrade_cap( "tf_bot_engineer_seams_upgrade_cap", "0", FCVAR_CHEAT, "Max upgrade level to target (0=disabled, 1..3)" );

// Guarded controls for threat-aware repair/upgrade interrupt
ConVar tf_bot_engineer_seams_repair_interrupt( "tf_bot_engineer_seams_repair_interrupt", "0", FCVAR_CHEAT, "Enable repair/upgrade interrupt under nearby threat" );
ConVar tf_bot_engineer_seams_repair_interrupt_range( "tf_bot_engineer_seams_repair_interrupt_range", "600", FCVAR_CHEAT, "Threat radius for repair interrupt" );
ConVar tf_bot_engineer_seams_repair_interrupt_window( "tf_bot_engineer_seams_repair_interrupt_window", "1.5", FCVAR_CHEAT, "Recent injury window (sec) to consider interrupting" );
ConVar tf_bot_engineer_seams_repair_interrupt_attack( "tf_bot_engineer_seams_repair_interrupt_attack", "0", FCVAR_CHEAT, "If 1, briefly switch to attack behavior; else just pause wrenching" );
ConVar tf_bot_engineer_seams_repair_interrupt_cooldown( "tf_bot_engineer_seams_repair_interrupt_cooldown", "2.0", FCVAR_CHEAT, "Cooldown between interrupts (sec)" );

#endif // TF_BOT_ENGINEER_SEAMS


ConVar tf_bot_engineer_retaliate_range( "tf_bot_engineer_retaliate_range", "750", FCVAR_CHEAT, "If attacker who destroyed sentry is closer than this, attack. Otherwise, retreat" );
ConVar tf_bot_engineer_exit_near_sentry_range( "tf_bot_engineer_exit_near_sentry_range", "2500", FCVAR_CHEAT, "Maximum travel distance between a bot's Sentry gun and its Teleporter Exit" );
ConVar tf_bot_engineer_max_sentry_travel_distance_to_point( "tf_bot_engineer_max_sentry_travel_distance_to_point", "2500", FCVAR_CHEAT, "Maximum travel distance between a bot's Sentry gun and the currently contested point" );

extern ConVar tf_bot_path_lookahead_range;

const int MaxPlacementAttempts = 5;


//---------------------------------------------------------------------------------------------
CTFBotEngineerBuilding::CTFBotEngineerBuilding( void )
{
	m_sentryBuildHint = NULL;
}


//---------------------------------------------------------------------------------------------
CTFBotEngineerBuilding::CTFBotEngineerBuilding( CTFBotHintSentrygun *sentryBuildHint )
{
	m_sentryBuildHint = sentryBuildHint;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEngineerBuilding::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_sentryTriesLeft = MaxPlacementAttempts;

	m_territoryRangeTimer.Invalidate();

    // Rate-limit seam maintain-eval debug (guarded prints)
    m_debugMaintainEvalTimer.Invalidate();
    // Rate-limit teleporter link check under guard
    m_teleportLinkCheckCooldown.Invalidate();
    // Guarded: allow immediate evaluation of repair interrupt
    m_repairInterruptCooldown.Invalidate();

	m_hasBuiltSentry = false;
	m_isSentryOutOfPosition = false;
	m_nearbyMetalStatus = NEARBY_METAL_UNKNOWN;

	return Continue();
}


//---------------------------------------------------------------------------------------------
// Everything is built, upgrade/maintain it
// TODO: Upgrade/maintain nearby friendly buildings, too.
void CTFBotEngineerBuilding::UpgradeAndMaintainBuildings( CTFBot *me )
{
	CObjectSentrygun *mySentry = (CObjectSentrygun *)me->GetObjectOfType( OBJ_SENTRYGUN );
	CObjectDispenser *myDispenser = (CObjectDispenser *)me->GetObjectOfType( OBJ_DISPENSER );

	if ( !mySentry )
	{
		return;
	}

	CBaseCombatWeapon *wrench = me->Weapon_GetSlot( TF_WPN_TYPE_MELEE );
	if ( wrench )
	{
		me->Weapon_Switch( wrench );
	}

	const float tooFarRange = 75.0f;

	if ( !myDispenser )
	{
		// just work on our sentry
		float rangeToSentry = me->GetDistanceBetween( mySentry );

		if ( rangeToSentry < 1.2f * tooFarRange )
		{
			// crouch both for cover behind our buildings, but also to slow us down so we hit our move goal more accurately
			me->PressCrouchButton();
		}

		if ( rangeToSentry > tooFarRange )
		{
			if ( m_repathTimer.IsElapsed() )
			{
				m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

				CTFBotPathCost cost( me, FASTEST_ROUTE );
				m_path.Compute( me, mySentry->GetAbsOrigin(), cost );
			}

			m_path.Update( me );
		}
		else
		{
			// we are in position - work on our buildings
			me->StopLookingAroundForEnemies();
			me->GetBodyInterface()->AimHeadTowards( mySentry->WorldSpaceCenter(), IBody::CRITICAL, 1.0f, NULL, "Work on my Sentry" );
			me->PressFireButton();
		}

		return;
	}

	// sit near both buildings
	Vector betweenMyBuildings = ( mySentry->GetAbsOrigin() + myDispenser->GetAbsOrigin() ) / 2.0f;

	// try to equalize distance between both
	float rangeToSentry = me->GetDistanceBetween( mySentry );
	float rangeToDispenser = me->GetDistanceBetween( myDispenser );

	const float equalTolerance = 25.0f;

	if ( rangeToSentry < 1.2f * tooFarRange && rangeToDispenser < 1.2f * tooFarRange )
	{
		// crouch both for cover behind our buildings, but also to slow us down so we hit our move goal more accurately
		me->PressCrouchButton();
	}

	if ( fabs( rangeToDispenser - rangeToSentry ) > equalTolerance || rangeToSentry > tooFarRange || rangeToDispenser > tooFarRange )
	{
		if ( m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

			CTFBotPathCost cost( me, FASTEST_ROUTE );
			m_path.Compute( me, betweenMyBuildings, cost );
		}

		m_path.Update( me );
	}

	if ( rangeToSentry < tooFarRange || rangeToDispenser < tooFarRange )
	{
		// we are (nearly) in position - work on our buildings
		m_searchTimer.Invalidate();

		CBaseObject *workTarget = mySentry;
		bool selectedByUpgrade = false; // true if baseline chose via upgrade preference rather than repair/urgent

		if ( mySentry->HasSapper() || mySentry->IsPlasmaDisabled() )
		{
			workTarget = mySentry;
		}
		else if ( myDispenser->HasSapper() || myDispenser->IsPlasmaDisabled() )
		{
			workTarget = myDispenser;
		}
		else if ( mySentry->GetTimeSinceLastInjury() < 1.0f || mySentry->GetHealth() < mySentry->GetMaxHealth() )
		{
			workTarget = mySentry;
		}
		else if ( mySentry->IsBuilding() )
		{
			workTarget = mySentry;
		}
		else if ( myDispenser->IsBuilding() )
		{
			workTarget = myDispenser;
		}
		else if ( mySentry->GetUpgradeLevel() < 3 )
		{
			workTarget = mySentry;
			selectedByUpgrade = true;
		}
		else if ( myDispenser->GetHealth() < myDispenser->GetMaxHealth() )
		{
			workTarget = myDispenser;
		}
		else if ( myDispenser->GetUpgradeLevel() < mySentry->GetUpgradeLevel() )
		{
			workTarget = myDispenser;
			selectedByUpgrade = true;
		}

#if TF_BOT_ENGINEER_SEAMS
		// Guarded: prefer repairing damaged buildings before upgrading, under threshold or seam policy
		if ( selectedByUpgrade )
		{
			const int engMetal = me->GetAmmoCount( TF_AMMO_METAL );
			bool seamRepair = Seam_ShouldRepairFirst( workTarget, engMetal );
			float cvar = tf_bot_engineer_seams_repair_health_frac.GetFloat();
			float sFrac = 1.0f;
			float dFrac = 1.0f;
			if ( mySentry->GetMaxHealth() > 0 )
			{
				sFrac = (float)mySentry->GetHealth() / (float)mySentry->GetMaxHealth();
			}
			if ( myDispenser->GetMaxHealth() > 0 )
			{
				dFrac = (float)myDispenser->GetHealth() / (float)myDispenser->GetMaxHealth();
			}

			// Optional low-noise trace to confirm maintain evaluation is running under guard
			if ( developer.GetBool() && tf_bot_engineer_seams_debug.GetBool() && cvar > 0.0f )
			{
				if ( m_debugMaintainEvalTimer.IsElapsed() )
				{
					DevMsg( "[TF-ENG seam] Maintain eval: selUp=%d sentry=%.2f disp=%.2f cvar=%.2f seam=%d\n",
						(int)selectedByUpgrade, sFrac, dFrac, cvar, (int)seamRepair );
					m_debugMaintainEvalTimer.Start( 1.0f );
				}
			}

			bool belowThresh = ( cvar > 0.0f ) && ( ( sFrac < cvar ) || ( dFrac < cvar ) );
			if ( seamRepair || belowThresh )
			{
				// Choose which to repair: prefer the one under threshold; if both, prefer sentry
				CBaseObject *chosen = workTarget;
				const char *chosenStr = "baseline";
				if ( belowThresh )
				{
					if ( sFrac < cvar && dFrac < cvar )
					{
						chosen = mySentry;
						chosenStr = "sentry";
					}
					else if ( sFrac < cvar )
					{
						chosen = mySentry;
						chosenStr = "sentry";
					}
					else if ( dFrac < cvar )
					{
						chosen = myDispenser;
						chosenStr = "dispenser";
					}
				}
				else
				{
					// seamRepair requested but no threshold: repair whichever is more damaged; prefer sentry on tie
					if ( sFrac <= dFrac )
					{
						chosen = mySentry;
						chosenStr = "sentry";
					}
					else
					{
						chosen = myDispenser;
						chosenStr = "dispenser";
					}
				}

				workTarget = chosen;

				if ( developer.GetBool() && tf_bot_engineer_seams_debug.GetBool() )
				{
					DevMsg( "[TF-ENG seam] Repair priority: sentry=%.2f disp=%.2f chosen=%s cvar=%.2f seam=%d\n",
						sFrac, dFrac, chosenStr, cvar, (int)seamRepair );
				}
			}
		}

		// Guarded: enforce metal reserve and upgrade cap when the baseline chose an upgrade target
		if ( selectedByUpgrade )
		{
			// Compute effective reserve and caps
			const int engMetal2 = me->GetAmmoCount( TF_AMMO_METAL );
			int seamReserve = Seam_MetalReserve( me );
			int capSentry = Seam_TargetUpgradeLevel( mySentry );
			int capDisp = Seam_TargetUpgradeLevel( myDispenser );
			int cvarReserve = tf_bot_engineer_seams_metal_reserve.GetInt();
			int cvarCap = tf_bot_engineer_seams_upgrade_cap.GetInt();

			int effectiveReserve = ( seamReserve > 0 ) ? seamReserve : cvarReserve;
			// Determine cap for current work target
			int effectiveCap = 0;
			int targetLevel = 0;
			const char *targetName = "unknown";
			if ( workTarget == mySentry )
			{
				effectiveCap = ( capSentry > 0 ) ? capSentry : cvarCap;
				targetLevel = mySentry->GetUpgradeLevel();
				targetName = "sentry";
			}
			else if ( workTarget == myDispenser )
			{
				effectiveCap = ( capDisp > 0 ) ? capDisp : cvarCap;
				targetLevel = myDispenser->GetUpgradeLevel();
				targetName = "dispenser";
			}

			bool blockUpgrade = false;
			// Reserve rule: block upgrades when at or below reserve
			if ( effectiveReserve > 0 && engMetal2 <= effectiveReserve )
			{
				blockUpgrade = true;
			}
			// Cap rule: block if target is already at/above cap (1..3)
			if ( !blockUpgrade && effectiveCap > 0 && effectiveCap <= 3 )
			{
				if ( targetLevel >= effectiveCap )
				{
					blockUpgrade = true;
				}
			}

			// Trace-only: confirm evaluation is running even if not blocking
			if ( developer.GetBool() && tf_bot_engineer_seams_debug.GetBool() )
			{
				if ( m_debugMaintainEvalTimer.IsElapsed() )
				{
					DevMsg( "[TF-ENG seam] Maintain eval (T5.2): selUp=%d metal=%d reserve=%d cap=%d target=%s lvl=%d block=%d\n",
						(int)selectedByUpgrade, engMetal2, effectiveReserve, effectiveCap, targetName, targetLevel, (int)blockUpgrade );
					m_debugMaintainEvalTimer.Start( 1.0f );
				}
			}

			bool actionBlockedNoRepair = false;
			if ( blockUpgrade )
			{
				// Prefer repair if anything is damaged; choose more damaged (prefer sentry on tie)
				bool sentryDamaged = ( mySentry->GetHealth() < mySentry->GetMaxHealth() );
				bool dispDamaged = ( myDispenser->GetHealth() < myDispenser->GetMaxHealth() );
				if ( sentryDamaged || dispDamaged )
				{
					float sFrac2 = 1.0f;
					float dFrac2 = 1.0f;
					if ( mySentry->GetMaxHealth() > 0 )
					{
						sFrac2 = (float)mySentry->GetHealth() / (float)mySentry->GetMaxHealth();
					}
					if ( myDispenser->GetMaxHealth() > 0 )
					{
						dFrac2 = (float)myDispenser->GetHealth() / (float)myDispenser->GetMaxHealth();
					}

					if ( sentryDamaged && ( !dispDamaged || sFrac2 <= dFrac2 ) )
					{
						workTarget = mySentry;
						targetName = "sentry";
					}
					else if ( dispDamaged )
					{
						workTarget = myDispenser;
						targetName = "dispenser";
					}
				}
				else
				{
					// Nothing to repair; block the upgrade by skipping action this frame
					actionBlockedNoRepair = true;
				}

				if ( developer.GetBool() && tf_bot_engineer_seams_debug.GetBool() )
				{
					int dbgCap = effectiveCap;
					DevMsg( "[TF-ENG seam] Upgrade blocked: reserve=%d metal=%d cap=%d target=%s lvl=%d selectedByUpgrade=%d\n",
						effectiveReserve, engMetal2, dbgCap, targetName, targetLevel, (int)selectedByUpgrade );
				}
			}

			// If the upgrade is blocked and nothing to repair, we skip firing below
			if ( actionBlockedNoRepair )
			{
				// Aim but don't hit the wrench to avoid upgrading
				me->StopLookingAroundForEnemies();
				me->GetBodyInterface()->AimHeadTowards( workTarget->WorldSpaceCenter(), IBody::CRITICAL, 1.0f, NULL, "Hold upgrade due to reserve/cap" );
				return; // no-op this cycle
			}
		}
#endif

		me->StopLookingAroundForEnemies();
		me->GetBodyInterface()->AimHeadTowards( workTarget->WorldSpaceCenter(), IBody::CRITICAL, 1.0f, NULL, "Work on my buildings" );
		me->PressFireButton();
	}
}


//---------------------------------------------------------------------------------------------
bool CTFBotEngineerBuilding::IsMetalSourceNearby( CTFBot *me ) const
{
	CUtlVector< CNavArea * > nearbyVector;
	CollectSurroundingAreas( &nearbyVector, me->GetLastKnownArea(), 2000.0f, me->GetLocomotionInterface()->GetStepHeight(), me->GetLocomotionInterface()->GetStepHeight() );

	for( int i=0; i<nearbyVector.Count(); ++i )
	{
		CTFNavArea *area = (CTFNavArea *)nearbyVector[i];
		if ( area->HasAttributeTF( TF_NAV_HAS_AMMO ) )
		{
			return true;
		}

		// this assumes all spawn rooms have resupply cabinets
		if ( me->GetTeamNumber() == TF_TEAM_RED && area->HasAttributeTF( TF_NAV_SPAWN_ROOM_RED ) )
		{
			return true;
		}

		if ( me->GetTeamNumber() == TF_TEAM_BLUE && area->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE ) )
		{
			return true;
		}
	}

	return false;
}


//---------------------------------------------------------------------------------------------
bool CTFBotEngineerBuilding::CheckIfSentryIsOutOfPosition( CTFBot *me ) const
{
	// Re-evaluate if MvM ever needs something more dynamic
	if ( TFGameRules()->IsPVEModeActive() )
		return false;

	CObjectSentrygun *mySentry = (CObjectSentrygun *)me->GetObjectOfType( OBJ_SENTRYGUN );

	if ( !mySentry )
	{
		return false;
	}

	// payload
	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		CTeamTrainWatcher *trainWatcher;

		if ( me->GetTeamNumber() == TF_TEAM_BLUE )
		{
			trainWatcher = TFGameRules()->GetPayloadToPush( me->GetTeamNumber() );
		}
		else
		{
			trainWatcher = TFGameRules()->GetPayloadToBlock( me->GetTeamNumber() );
		}

		if ( trainWatcher )
		{
			float sentryDistanceAlongPath;
			trainWatcher->ProjectPointOntoPath( mySentry->GetAbsOrigin(), NULL, &sentryDistanceAlongPath );

			const float behindTrainTolerance = SENTRY_MAX_RANGE;
			return ( trainWatcher->GetTrainDistanceAlongTrack() > sentryDistanceAlongPath + behindTrainTolerance );
		}
	}

	// control points
	mySentry->UpdateLastKnownArea();
	CNavArea *sentryArea = mySentry->GetLastKnownArea();

	CTeamControlPoint *point = me->GetMyControlPoint();
	if ( point )
	{
		CTFNavArea *pointArea = TheTFNavMesh()->GetControlPointCenterArea( point->GetPointIndex() );

		if ( sentryArea && pointArea )
		{
			CTFBotPathCost cost( me, FASTEST_ROUTE );
			if ( NavAreaTravelDistance( sentryArea, pointArea, cost, tf_bot_engineer_max_sentry_travel_distance_to_point.GetFloat() ) < 0 &&
				 NavAreaTravelDistance( pointArea, sentryArea, cost, tf_bot_engineer_max_sentry_travel_distance_to_point.GetFloat() ) < 0 )
			{
				return true;
			}
		}
	}

	return false;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEngineerBuilding::Update( CTFBot *me, float interval )
{
	CObjectSentrygun *mySentry = (CObjectSentrygun *)me->GetObjectOfType( OBJ_SENTRYGUN );
	CObjectDispenser *myDispenser = (CObjectDispenser *)me->GetObjectOfType( OBJ_DISPENSER );
	CObjectTeleporter *myTeleportEntrance = (CObjectTeleporter *)me->GetObjectOfType( OBJ_TELEPORTER, MODE_TELEPORTER_ENTRANCE );
	CObjectTeleporter *myTeleportExit = (CObjectTeleporter *)me->GetObjectOfType( OBJ_TELEPORTER, MODE_TELEPORTER_EXIT );

#if TF_BOT_ENGINEER_SEAMS
	// Guarded: Validate teleporter link and estimate travel distance. Only log/set local flags; no behavior change here.
	bool seam_TeleporterLinkInvalid = false;
	bool seam_TeleporterShouldRedeploy = false;
	float seam_TeleporterTravelDelta = 0.0f;
	if ( myTeleportEntrance && myTeleportExit )
	{
		myTeleportEntrance->UpdateLastKnownArea();
		myTeleportExit->UpdateLastKnownArea();
		CTFNavArea *entrArea = (CTFNavArea *)myTeleportEntrance->GetLastKnownArea();
		CTFNavArea *exitArea = (CTFNavArea *)myTeleportExit->GetLastKnownArea();
		if ( entrArea && exitArea )
		{
			float incA = entrArea->GetIncursionDistance( me->GetTeamNumber() );
			float incB = exitArea->GetIncursionDistance( me->GetTeamNumber() );
			seam_TeleporterTravelDelta = fabsf( incA - incB );
		}
		else
		{
			seam_TeleporterTravelDelta = ( myTeleportEntrance->GetAbsOrigin() - myTeleportExit->GetAbsOrigin() ).Length();
		}

		bool isValid = Seam_TeleporterIsValid( myTeleportEntrance, myTeleportExit );
		bool shouldRedeploy = Seam_TeleporterShouldRedeploy( myTeleportEntrance, myTeleportExit, seam_TeleporterTravelDelta );

		// Interpret seam returns; apply simple fallback using cvar if no redeploy guidance
		seam_TeleporterLinkInvalid = !isValid;
		seam_TeleporterShouldRedeploy = shouldRedeploy;
		if ( !seam_TeleporterShouldRedeploy )
		{
			if ( seam_TeleporterTravelDelta < tf_bot_engineer_seams_min_teleport_travel.GetFloat() )
			{
				seam_TeleporterShouldRedeploy = true;
			}
		}

		if ( developer.GetBool() && tf_bot_engineer_seams_debug.GetBool() )
		{
			DevMsg( "[TF-ENG seam] Teleporter link check (Building): valid=%d redeploy=%d travelDelta=%.1f\n",
				(int)!seam_TeleporterLinkInvalid, (int)seam_TeleporterShouldRedeploy, seam_TeleporterTravelDelta );
		}

		// Guarded exit redeploy when link is invalid or travel is suboptimal
		if ( tf_bot_engineer_seams_teleport_redeploy.GetBool() && myTeleportEntrance && myTeleportExit && m_teleportRedeployCooldown.IsElapsed() )
		{
			// Avoid redeploy if we are under immediate attack (match baseline criteria)
			bool seam_isUnderAttack = ( me->GetTimeSinceLastInjury() < 1.0f );
			seam_isUnderAttack |= ( mySentry && ( mySentry->HasSapper() || mySentry->IsPlasmaDisabled() ) );
			seam_isUnderAttack |= ( myDispenser && ( myDispenser->HasSapper() || myDispenser->IsPlasmaDisabled() ) );

			if ( !seam_isUnderAttack && ( seam_TeleporterLinkInvalid || seam_TeleporterShouldRedeploy ) )
			{
				// Detonate only the exit; entrance remains
				myTeleportExit->DetonateObject();
				// Start cooldown to prevent oscillation; exit retry timer behavior remains as-is
				m_teleportRedeployCooldown.Start( tf_bot_engineer_seams_teleport_redeploy_cooldown.GetFloat() );

				if ( developer.GetBool() && tf_bot_engineer_seams_debug.GetBool() )
				{
					DevMsg( "[TF-ENG seam] Teleporter redeploy: detonate exit (invalid=%d redeploy=%d travelDelta=%.1f)\n",
						(int)seam_TeleporterLinkInvalid, (int)seam_TeleporterShouldRedeploy, seam_TeleporterTravelDelta );
				}
			}
		}
	}
#endif

	bool isUnderAttack = ( me->GetTimeSinceLastInjury() < 1.0f );
	isUnderAttack |= ( mySentry && ( mySentry->HasSapper() || mySentry->IsPlasmaDisabled() ) );
	isUnderAttack |= ( myDispenser && ( myDispenser->HasSapper() || myDispenser->IsPlasmaDisabled() ) );

	me->StartLookingAroundForEnemies();

	// try to build a Sentry
	if ( !mySentry )
	{
		m_nearbyMetalStatus = NEARBY_METAL_UNKNOWN;

		// react to nearby threats if our sentry is down
		const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
		if ( threat && threat->IsVisibleRecently() )
		{
			me->EquipBestWeaponForThreat( threat );
		}

		if ( !m_hasBuiltSentry && m_sentryTriesLeft > 0 )
		{
			--m_sentryTriesLeft;

			if ( m_sentryBuildHint )
			{
				return SuspendFor( new CTFBotEngineerBuildSentryGun( m_sentryBuildHint ), "Building a Sentry at a hint location" );
			}

			return SuspendFor( new CTFBotEngineerBuildSentryGun, "Building a Sentry" );
		}
		else
		{
			// can't build a Sentry here - pick a new place
			return ChangeTo( new CTFBotEngineerMoveToBuild, "Couldn't find a place to build" );
		}
	}

	// I have a Sentry
	m_hasBuiltSentry = true;

	if ( m_sentryBuildHint != NULL && !m_sentryBuildHint->IsEnabled() )
	{
		// our hint has been disabled and no longer has influence on our behavior
		m_sentryBuildHint = NULL;
	}

	// periodically check that our Sentry is still near the contested point
	if ( m_sentryBuildHint == NULL || !m_sentryBuildHint->IsSticky() )
	{
		if ( !m_isSentryOutOfPosition && m_territoryRangeTimer.IsElapsed() )
		{
			m_territoryRangeTimer.Start( RandomFloat( 3.0f, 5.0f ) );

			m_isSentryOutOfPosition = CheckIfSentryIsOutOfPosition( me );
		}

		if ( m_isSentryOutOfPosition )
		{
			// the point has moved, only keep sentry as long as it keeps attacking
			if ( mySentry->GetTimeSinceLastFired() > 10.0f )
			{
				mySentry->DetonateObject();

				// if we built here because of a hint, disable that hint so we don't use it and rebuild here again
				if ( m_sentryBuildHint != NULL )
				{
					inputdata_t dummy;
					m_sentryBuildHint->InputDisable( dummy );

					m_sentryBuildHint = NULL;
				}

				if ( myDispenser )
				{
					myDispenser->DetonateObject();
				}

				if ( myTeleportExit )
				{
					myTeleportExit->DetonateObject();
				}

				me->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_MOVEUP );

				return ChangeTo( new CTFBotEngineerMoveToBuild, "Need to move my gear closer to the point!" );
			}
		}
	}

	// if my dispenser is too far away from my sentry, destroy and rebuild it next update
	// @TODO: Flag hint-built entities for a larger range
	const float maxSeparation = 500.0f;
	if ( myDispenser )
	{
		if ( ( mySentry->GetAbsOrigin() - myDispenser->GetAbsOrigin() ).IsLengthGreaterThan( maxSeparation ) )
		{
			myDispenser->DestroyObject();
			myDispenser = NULL;
		}
	}

	// build up the sentry all the way if there is a metal source nearby
	if ( mySentry->GetUpgradeLevel() < 3 )
	{
		if ( m_nearbyMetalStatus == NEARBY_METAL_UNKNOWN )
		{
			m_nearbyMetalStatus = IsMetalSourceNearby( me ) ? NEARBY_METAL_EXISTS : NEARBY_METAL_NONE;
		}

		if ( m_nearbyMetalStatus == NEARBY_METAL_EXISTS )
		{
#if TF_BOT_ENGINEER_SEAMS
			// Guarded: before maintaining/upgrading, optionally interrupt to defend if recently injured
			if ( tf_bot_engineer_seams_repair_interrupt.GetBool() && m_repairInterruptCooldown.IsElapsed() )
			{
				bool recentInjury = ( me->GetTimeSinceLastInjury() < tf_bot_engineer_seams_repair_interrupt_window.GetFloat() );
				if ( recentInjury )
				{
					m_repairInterruptCooldown.Start( tf_bot_engineer_seams_repair_interrupt_cooldown.GetFloat() );
					if ( developer.GetBool() && tf_bot_engineer_seams_debug.GetBool() )
					{
						DevMsg( "[TF-ENG seam] Repair interrupt: injury=%d range=%.0f attack=%d\n",
							(int)recentInjury,
							tf_bot_engineer_seams_repair_interrupt_range.GetFloat(),
							(int)tf_bot_engineer_seams_repair_interrupt_attack.GetBool() );
					}

					if ( tf_bot_engineer_seams_repair_interrupt_attack.GetBool() )
					{
						return SuspendFor( new CTFBotAttack, "Defend nest (threat interrupt)" );
					}

					// Otherwise, pause wrenching this frame to let attack layers act
					return Continue();
				}
			}
#endif
			UpgradeAndMaintainBuildings( me );
			return Continue();
		}
	}

/*
	if ( myTeleportExit )
	{
		// if my teleporter exit is too far away from my sentry, destroy and rebuild it next update
		if ( ( mySentry->GetAbsOrigin() - myTeleportExit->GetAbsOrigin() ).IsLengthGreaterThan( maxSeparation ) )
		{
			myTeleportExit->DestroyObject();
			myTeleportExit = NULL;
		}
	}
*/

	// try to build a Dispenser (build after tele exit in training)
	if ( !TFGameRules()->IsInTraining() || myTeleportExit )
	{
		const float dispenserRebuildInterval = 10.0f;
		if ( myDispenser )
		{
			// don't rebuild immediately after building is destroyed
			m_dispenserRetryTimer.Start( dispenserRebuildInterval );
		}
		else if ( m_dispenserRetryTimer.IsElapsed() && !isUnderAttack )
		{
			m_dispenserRetryTimer.Start( dispenserRebuildInterval );

			return SuspendFor( new CTFBotEngineerBuildDispenser, "Building a Dispenser" );
		}
	}

	// try to build a Teleporter Exit
	const float exitRebuildInterval = TFGameRules()->IsInTraining() ? 5.0f : 30.0f;
	if ( myTeleportExit )
	{
		// don't rebuild immediately after building is destroyed
		m_teleportExitRetryTimer.Start( exitRebuildInterval );
	}
	else if ( m_teleportExitRetryTimer.IsElapsed() && myTeleportEntrance && !isUnderAttack )
	{
		m_teleportExitRetryTimer.Start( exitRebuildInterval );

		// we need to build a teleporter exit yet
		if ( m_sentryBuildHint != NULL )
		{
			// if there are teleporter exit hints, find the closest one to our sentry and use it
			CUtlVector< CBaseEntity * > hintVector;
			CTFBotHintTeleporterExit *hint = NULL;
			while( ( hint = (CTFBotHintTeleporterExit *)gEntList.FindEntityByClassname( hint, "bot_hint_teleporter_exit" ) ) != NULL )
			{
				if ( hint->IsEnabled() && hint->InSameTeam( me ) )
				{
					hintVector.AddToTail( hint );
				}
			}

			if ( hintVector.Count() > 0 )
			{
				mySentry->UpdateLastKnownArea();
				CBaseEntity *closeHint = SelectClosestEntityByTravelDistance( me, hintVector, mySentry->GetLastKnownArea(), tf_bot_engineer_exit_near_sentry_range.GetFloat() );

				if ( closeHint )
				{
					return SuspendFor( new CTFBotEngineerBuildTeleportExit( closeHint->GetAbsOrigin(), closeHint->GetAbsAngles().y ), "Building teleporter exit at nearby hint" );
				}
			}
		}
		else if ( me->IsRangeLessThan( mySentry, 300.0f ) )
		{
			// drop a teleporter exit near our sentry
			return SuspendFor( new CTFBotEngineerBuildTeleportExit(), "Building teleporter exit" );
		}
	}

	// everything is built - maintain them
#if TF_BOT_ENGINEER_SEAMS
	// Guarded: before maintaining/upgrading, optionally interrupt to defend if recently injured
	if ( tf_bot_engineer_seams_repair_interrupt.GetBool() && m_repairInterruptCooldown.IsElapsed() )
	{
		bool recentInjury = ( me->GetTimeSinceLastInjury() < tf_bot_engineer_seams_repair_interrupt_window.GetFloat() );
		if ( recentInjury )
		{
			m_repairInterruptCooldown.Start( tf_bot_engineer_seams_repair_interrupt_cooldown.GetFloat() );
			if ( developer.GetBool() && tf_bot_engineer_seams_debug.GetBool() )
			{
				DevMsg( "[TF-ENG seam] Repair interrupt: injury=%d range=%.0f attack=%d\n",
					(int)recentInjury,
					tf_bot_engineer_seams_repair_interrupt_range.GetFloat(),
					(int)tf_bot_engineer_seams_repair_interrupt_attack.GetBool() );
			}

			if ( tf_bot_engineer_seams_repair_interrupt_attack.GetBool() )
			{
				return SuspendFor( new CTFBotAttack, "Defend nest (threat interrupt)" );
			}

			// Otherwise, pause wrenching this frame to let attack layers act
			return Continue();
		}
	}
#endif
	UpgradeAndMaintainBuildings( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CTFBotEngineerBuilding::OnEnd( CTFBot *me, Action< CTFBot > *nextAction )
{
	me->StartLookingAroundForEnemies();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotEngineerBuilding::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotEngineerBuilding::OnTerritoryLost( CTFBot *me, int territoryID )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotEngineerBuilding::OnTerritoryCaptured( CTFBot *me, int territoryID )
{
	return TryContinue();
}
