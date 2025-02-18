//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_mapitems.h"
#include "tf_shareddefs.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_timer.h"
#include "tf_team.h"


#if 0

bool ActivateDoResults(CTFGoal *Goal, CTFPlayer *AP, CTFGoal *ActivatingGoal);
bool ActivationSucceeded(CTFGoal *Goal, CTFPlayer *AP, CTFGoal *ActivatingGoal);
void DoResults(CTFGoal *Goal, CTFPlayer *AP, BOOL bAddBonuses);


// ---------------------------------------------------------------------------------------- //
// Global helpers.
// ---------------------------------------------------------------------------------------- //

const char* GetTeamName( int iTeam )
{
//	if ( iTeam == 0 )
//	{
//		return "SPECTATOR";
//	}
//	else
	{
		CTeam *pTeam = GetGlobalTeam( iTeam );
		if ( pTeam )
		{
			return pTeam->GetName();
		}
		else
		{
			return "UNKNOWN TEAM";
		}
	}
}


//===========================================
int GetTeamCheckTeam( const char *pTargetName )
{
	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, pTargetName );
	if ( pEntity )
	{
		if ( !strcmp( pEntity->GetClassname(), "info_tf_teamcheck" ) )
			return pEntity->GetTeamNumber();
	}

	return 0;
}


//=========================================================================
// Displays the state of a GoalItem
void DisplayItemStatus(CTFGoal *Goal, CTFPlayer *Player, CTFGoalItem *Item)
{
	MDEBUG( Msg( "Displaying Item Status\nItem goal_no : %d\n", Item->goal_no) );

	// If we have a teamcheck entity, use it instead
	if ( Item->owned_by_teamcheck != NULL_STRING )
		Item->owned_by = GetTeamCheckTeam( STRING(Item->owned_by_teamcheck) );

	if (Item->goal_state == TFGS_ACTIVE)
	{
		MDEBUG( Msg( "   Item is ACTIVE\n") );

		if ( (Goal->team_str_carried != NULL_STRING) || (Goal->non_team_str_carried != NULL_STRING) )
		{
			CBaseEntity *pOwner = Item->GetOwnerEntity();

			if (Player->GetTeamNumber() == Item->owned_by)
			{
				if (Player == Item->GetOwnerEntity())
					ClientPrint( Player, HUD_PRINTTALK, STRING(Goal->team_str_carried), "you" );
				else
					ClientPrint( Player, HUD_PRINTTALK, STRING(Goal->team_str_carried), STRING(pOwner->GetEntityName()) );
			}
			else
			{
				if (Player == Item->GetOwnerEntity())
					ClientPrint( Player, HUD_PRINTTALK, STRING(Goal->non_team_str_carried), "you" );
				else
					ClientPrint( Player, HUD_PRINTTALK, STRING(Goal->non_team_str_carried), STRING(pOwner->GetEntityName()) );
			}
		}
	}
	else if (Item->GetAbsOrigin() != Item->oldorigin)
	{
		MDEBUG( Msg( "   Item has MOVED\n") );

		if ( (Goal->team_str_moved != NULL_STRING) || (Goal->non_team_str_moved != NULL_STRING) )
		{
			if (Player->GetTeamNumber() == Item->owned_by)
				ClientPrint( Player, HUD_PRINTTALK, STRING(Goal->team_str_moved) );
			else
				ClientPrint( Player, HUD_PRINTTALK, STRING(Goal->non_team_str_moved) );
		}
	}
	else
	{
		MDEBUG( Msg( "   Item is AT HOME\n") );

		if ( Goal->team_str_home != NULL_STRING || Goal->non_team_str_home != NULL_STRING )
		{
			if (Player->GetTeamNumber() == Item->owned_by)
				ClientPrint( Player, HUD_PRINTTALK, STRING(Goal->team_str_home) );
			else
				ClientPrint( Player, HUD_PRINTTALK, STRING(Goal->non_team_str_home) );
		}
	}
}

#if 0
//=========================================================================
// Inactivates a Teamspawn point
void InactivateSpawn(CTFSpawn *Spawn)
{
	Spawn->goal_state = TFGS_REMOVED;
}

//=========================================================================
// Activates a Teamspawn point
void ActivateSpawn(CTFSpawn *Spawn)
{
	Spawn->goal_state = TFGS_INACTIVE;
}
#endif

//=========================================================================
// Increase the score of a team
void TeamFortress_TeamIncreaseScore(int tno, int scoretoadd)
{
	if ( tno == 0 )
		return;
	
	CTeam *pTeam = GetGlobalTeam( tno );
	if ( !pTeam )
		return;

	pTeam->AddScore( scoretoadd );
}

// Returns true if the AP's carrying at least 1 of the items in the group
bool HasItemFromGroup( CBaseEntity *AP, int iGroupNo )
{
	// Find all items in the group
	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "item_tfgoal" );
	while ( pEnt )
	{
		CTFGoalItem *pGoal = dynamic_cast<CTFGoalItem*>( pEnt );
		if ( (pGoal->group_no == iGroupNo) && (pGoal->GetOwnerEntity() == AP) )
			return true;
		
		pEnt = gEntList.FindEntityByClassname( pEnt, "item_tfgoal" );
	}

	return false;
}

//=========================================================================
// Returns true if all the goals in the specified group are in the specified state
bool AllGoalsInState( int iGroupNo, int iState )
{
	// Find all goals in the group
	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "info_tfgoal" );
	while ( pEnt )
	{
		CTFGoal *pGoal = dynamic_cast< CTFGoal* >( pEnt );
		if ( pGoal )
		{
			if (pGoal->group_no == iGroupNo)
			{
				// All Goals in the group must be in the specified state
				if (pGoal->goal_state != iState)
					return false;
			}
		}		
	
		pEnt = gEntList.FindEntityByClassname( pEnt, "info_tfgoal" );
	}

	return true;
}


// Return the item with a goal_no equal to ino
CTFGoalItem* Finditem(int ino)
{
	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "item_tfgoal" );
	while ( pEnt )
	{
		CTFGoalItem *pGoal = dynamic_cast<CTFGoalItem*>( pEnt );
		if (pGoal && pGoal->goal_no == ino)
			return pGoal;

		pEnt = gEntList.FindEntityByClassname( pEnt, "item_tfgoal" );
	}

	// Goal does not exist
	Warning("Could not find an item with a goal_no of %d.\n", ino);
	return NULL;
}


//=========================================================================
// Return the TeamSpawn with a goal_no equal to gno
CTFSpawn* Findteamspawn(int gno)
{
	// Search by netname
	//TFTODO: I think FindEntityByClassname will do the same thing.
	//CBaseEntity *pEnt = UTIL_FindEntityByString( NULL, "netname", "info_player_teamspawn" );
	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
	while ( pEnt )
	{
		CTFSpawn *pSpawn = dynamic_cast<CTFSpawn*>( pEnt );
		if ( pSpawn )
		{
			if (pSpawn->goal_no == gno)
				return pSpawn;
		}

		pEnt = gEntList.FindEntityByClassname( pEnt, "info_player_teamspawn" );
	}

	// Goal does not exist
	Warning("Could not find a Teamspawn with a goal_no of %d.\n", gno);
	return NULL;
}


//=========================================================================
// Return the goal with a goal_no equal to gno
CTFGoal* Findgoal(int gno)
{
	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "item_tfgoal" );
	while ( pEnt )
	{
		CTFGoal *pGoal = dynamic_cast<CTFGoal*>( pEnt );
		if (pGoal && pGoal->goal_no == gno)
			return pGoal;

		pEnt = gEntList.FindEntityByClassname( pEnt, "item_tfgoal" );
	}

	// Goal does not exist
	Warning("Could not find a goal with a goal_no of %d.\n", gno);
	return NULL;
}


//=========================================================================
// Remove a Timer/Goal
void RemoveGoal(CTFGoal *pGoal)
{
	pGoal->AddSolidFlags( FSOLID_NOT_SOLID );
	pGoal->goal_state = TFGS_REMOVED;
	pGoal->AddEffects( EF_NODRAW );
}


//=========================================================================
// Return true if the player meets the AP criteria
bool APMeetsCriteria(CTFGoal *Goal, CTFPlayer *AP)
{
	MDEBUG(Warning("==========================\n"));
	MDEBUG(Warning("AP Criteria Checking\n"));
	MDEBUG(Warning(UTIL_VarArgs("Goal: %s", STRING(Goal->edict()->netname))));

	CTFGoal *pGoal;
	CTFGoalItem *pItem;

	if (AP != NULL && AP->Classify() == CLASS_PLAYER)
	{
		MDEBUG(Warning(UTIL_VarArgs("\nAP  : %s\n", AP->GetPlayerName())));

		// If a player of a specific team can only activate this
		if (Goal->GetTeamNumber())
		{
			MDEBUG(Warning("   Checking team."));
			if (Goal->GetTeamNumber() != AP->GetTeamNumber())
				return false;
			if ( !AP->IsAlive() ) // don't want dead or dying players activating this
				return false;
			MDEBUG(Warning("passed.\n"));
		}

		// If a player in a team specified by a teamcheck entity can activate this
		if (Goal->teamcheck != NULL_STRING)
		{
			MDEBUG(Warning("   Checking teamcheck entity."));

			if ( AP->GetTeamNumber() != GetTeamCheckTeam( STRING(Goal->teamcheck) ) )
				return false;

			MDEBUG(Warning("   passed.\n"));
		}

		// If a player of a specific class can only activate this
		if (Goal->playerclass)
		{
			MDEBUG(Warning("   Checking class."));
			CTFPlayerClass *pPlayerClass = AP->GetPlayerClass();
			if ( pPlayerClass && !pPlayerClass->IsClass( Goal->playerclass ) )
				return false;
			MDEBUG(Warning("passed.\n"));
		}

		// If this activation needs a GoalItem, make sure the player has it
		if (Goal->items_allowed)
		{
			MDEBUG(Warning("   Checking items."));
			pItem = Finditem(Goal->items_allowed);
			if (!pItem)
				return false;
			if (pItem->GetOwnerEntity() != AP)
				return false;
			MDEBUG(Warning("passed.\n"));
		}
	}

	// Check Goal states
	if (Goal->if_goal_is_active)
	{
		MDEBUG(Warning("   Checking if_goal_is_active."));
		pGoal = Findgoal(Goal->if_goal_is_active);
		if (!pGoal)
			return false;
		if (pGoal->goal_state != TFGS_ACTIVE)
			return false; 
		MDEBUG(Warning("passed.\n"));
	}

	if (Goal->if_goal_is_inactive)
	{
		MDEBUG(Warning("   Checking if_goal_is_inactive."));
		pGoal = Findgoal(Goal->if_goal_is_inactive);
		if (!pGoal)
			return false;
		if (pGoal->goal_state != TFGS_INACTIVE)
			return false;
		MDEBUG(Warning("passed.\n"));
	}

	if (Goal->if_goal_is_removed)
	{
		MDEBUG(Warning("   Checking if_goal_is_removed."));
		pGoal = Findgoal(Goal->if_goal_is_removed);
		if (!pGoal)
			return false;
		if (pGoal->goal_state != TFGS_REMOVED)
			return false;
		MDEBUG(Warning("passed.\n"));
	}

	// Check Group States
	if (Goal->if_group_is_active)
	{
		MDEBUG(Warning("   Checking if_group_is_active."));
		if ( !AllGoalsInState(Goal->if_group_is_active, TFGS_ACTIVE) )
			return false;
		MDEBUG(Warning("passed.\n"));
	}

	if (Goal->if_group_is_inactive)
	{
		MDEBUG(Warning("   Checking if_group_is_inactive."));
		if ( !AllGoalsInState(Goal->if_group_is_inactive, TFGS_INACTIVE) )
			return false;
		MDEBUG(Warning("passed.\n"));
	}

	if (Goal->if_group_is_removed)
	{
		MDEBUG(Warning("   Checking if_group_is_removed."));
		if ( !AllGoalsInState(Goal->if_group_is_removed, TFGS_REMOVED) )
			return false;
		MDEBUG(Warning("passed.\n"));
	}

	// Check Item States
	if (Goal->if_item_has_moved)
	{
		MDEBUG(Warning("   Checking if_item_has_moved."));
		// Find the item
		pItem = Finditem(Goal->if_item_has_moved);
		if (!pItem)
			return false;
		if (pItem->goal_state != TFGS_ACTIVE && pItem->GetAbsOrigin() == pItem->oldorigin)
			return false;
		MDEBUG(Warning("passed.\n"));
	}

	if (Goal->if_item_hasnt_moved)
	{
		MDEBUG(Warning("   Checking if_item_hasnt_moved."));
		// Find the item
		pItem = Finditem(Goal->if_item_hasnt_moved);
		if (!pItem)
			return false;
		if (pItem->goal_state == TFGS_ACTIVE || pItem->GetAbsOrigin() != pItem->oldorigin )
			return false;
		MDEBUG(Warning("passed.\n"));
	}

	// Check Items being carried
	if (AP != NULL  && AP->Classify() == CLASS_PLAYER)
	{
		if (Goal->has_item_from_group)
		{
			MDEBUG(Warning("   Checking has_item_from_group."));
			if ( !HasItemFromGroup(AP, Goal->has_item_from_group) )
				return false;
			MDEBUG(Warning("passed.\n"));
		}

		if (Goal->hasnt_item_from_group)
		{
			MDEBUG(Warning("   Checking hasnt_item_from_group."));
			if ( HasItemFromGroup(AP, Goal->hasnt_item_from_group) )
				return false;
			MDEBUG(Warning("passed.\n"));
		}
	}

	MDEBUG(Warning("Criteria passed.\n"));
	return true;
}


//=========================================================================
// Return true if the Entity should activate
bool ShouldActivate(CTFGoal *Goal, CTFPlayer *AP)
{
#ifdef MAP_DEBUG
	Warning(UTIL_VarArgs("\nDoIActivate: ", Goal->edict()->netname ? STRING(Goal->edict()->netname) : STRING(Goal->edict()->classname)));
	if (AP)
		Warning(UTIL_VarArgs(", AP: %s\n", AP->GetPlayerName()));
#endif

	// Abort if it's already active
	if (Goal->goal_state == TFGS_ACTIVE)
	{
		MDEBUG(Warning("-- Goal already active --\n"));
		return false;
	}
	// Abort if it's been removed
	if (Goal->goal_state == TFGS_REMOVED)
	{
		MDEBUG(Warning("-- Goal is in Removed state --\n"));
		return false;
	}
	// Abort if it's been activated already and its activation's being delayed
	if (Goal->goal_state == TFGS_DELAYED)
	{
		MDEBUG(Warning("-- Goal is being Delayed --\n"));
		return false;
	}

	// See if the AP matches the criteria
	bool bAPMet = APMeetsCriteria(Goal, AP);
	bool bAct = false;
	bool bRevAct;
	if ( FClassnameIs(Goal,"item_tfgoal") )
		bRevAct = (Goal->goal_activation & TFGI_REVERSE_AP) != 0;
	else
		bRevAct = (Goal->goal_activation & TFGA_REVERSE_AP) != 0;

	// Does the AP match the AP Criteria?
	if (bAPMet)
	{
		MDEBUG(Warning("-- Criteria met --\n"));
		if (!bRevAct)
			bAct = true;
	}
	else
	{
		MDEBUG(Warning("-- Criteria not met --\n"));
		if (bRevAct)
		{
			MDEBUG(Warning("Reverse Activation\n"));
			bAct = true;
		}
	}

#ifdef MAP_DEBUG
	if (bAct)
		Warning("Activation.\n");
	else
		Warning("NO Activation.\n");
#endif

	return bAct;
};


//=========================================================================
// Return TRUE if the player is affected by the goal
BOOL IsAffectedBy(CTFGoal *Goal, CTFPlayer *Player, CTFPlayer *AP)
{
	// Don't affect anyone who isn't alive or is in Observer mode
	CTFPlayerClass *pPlayerClass = AP->GetPlayerClass();
	if ( pPlayerClass && pPlayerClass->IsClass( TF_CLASS_UNDEFINED ) )
		return FALSE;

	// Same Environment Check
	if (Goal->goal_effects & TFGE_SAME_ENVIRONMENT)
	{
		int iEnviron = UTIL_PointContents( Goal->GetAbsOrigin() );
		if ( UTIL_PointContents( Player->GetAbsOrigin() ) != iEnviron )
			return FALSE;
	}

	if (Goal->t_length != 0)
	{
		// Within radius?
		if ((Goal->GetAbsOrigin() - Player->GetAbsOrigin()).Length() <= Goal->t_length)
		{
			// Obstructed by walls?
			if (Goal->goal_effects & TFGE_WALL)
			{
				trace_t tr;
				UTIL_TraceLine ( Goal->GetAbsOrigin(), Player->WorldSpaceCenter(), MASK_SOLID, Goal, COLLISION_GROUP_NONE, &tr );
				if ( tr.fraction == 1.0 )
					return TRUE;
			}
			else
			{
				return TRUE;
			}
		}
	}	

	if ( Goal->Classify() != CLASS_TFGOAL_TIMER && AP != NULL )
	{
		// Spawnpoints always affect the player who spawns on them
		if ((Goal->Classify() == CLASS_TFSPAWN) && (Player == AP))
			return TRUE;

		if ((Goal->goal_effects & TFGE_AP) && (Player == AP))
			return TRUE;
	
		if ((Goal->goal_effects & TFGE_AP_TEAM) && (AP->GetTeamNumber() == Player->GetTeamNumber()))
			return TRUE;
	}

	if (Goal->goal_effects & TFGE_NOT_AP_TEAM)
	{
		if (AP == NULL || AP->GetTeamNumber() != Player->GetTeamNumber())
			return TRUE;
	}

	if ((Goal->goal_effects & TFGE_NOT_AP) && (Player != AP))
		return TRUE;

	if ((Goal->maxammo_shells != 0) && (Player->GetTeamNumber() == Goal->maxammo_shells))
		return TRUE;

	if ((Goal->maxammo_nails != 0) && (Player->GetTeamNumber() != Goal->maxammo_nails))
		return TRUE;

	return FALSE;
}


//=========================================================================
// Do all the checking of Item Groups
void DoItemGroupWork(CTFGoalItem *Item, CTFPlayer *AP)
{
	if (Item->distance != 0)
	{
		if (Item->pain_finished == 0)
		{
			// No goal specified in .pain_finished. Print error.
			Warning( "GoalItem %d has .distance specified, but no .pain_finished\n", Item->goal_no );
		}

		BOOL bAllCarried = TRUE;
		// Find all items
		CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "item_tfgoal" );
		while ( pEnt && bAllCarried )
		{
			CTFGoalItem *pItem = dynamic_cast<CTFGoalItem*>( pEnt );
			if ( pItem )
			{
				if (pItem->group_no == Item->distance && pItem->goal_state != TFGS_ACTIVE)
					bAllCarried = FALSE;
			}

			pEnt = gEntList.FindEntityByClassname( pEnt, "item_tfgoal" );
		}

		if (bAllCarried)
		{
			CTFGoal *pGoal = Findgoal(Item->pain_finished);
			if (pGoal)
				DoResults(pGoal, AP, (Item->goal_result & TFGR_ADD_BONUSES));
		}
	}

	if (Item->speed != 0)
	{
		if (Item->attack_finished == 0)
		{
			// No goal specified in .attack_finished. Print error.
			Warning( "GoalItem %d has .speed specified, but no .attack_finished\n", Item->goal_no );
		}

		BOOL bAllCarried = TRUE;
		CBaseEntity *pCarrier = NULL;
		// Find all goals
		CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "item_tfgoal" );
		while ( pEnt && bAllCarried )
		{
			CTFGoalItem *pItem = dynamic_cast<CTFGoalItem*>( pEnt );
			if ( pItem )
			{
				if (pItem->group_no == Item->speed)
				{
					if (pItem->goal_state != TFGS_ACTIVE)
						bAllCarried = FALSE;
					else if (!pCarrier)	// Store Player
						pCarrier = pItem->GetOwnerEntity();
					else if (pCarrier != pItem->GetOwnerEntity()) // Need to all be carried by the same player
						bAllCarried = FALSE;
				}
			}

			pEnt = gEntList.FindEntityByClassname( pEnt, "item_tfgoal" );
		}

		if (bAllCarried)
		{
			CTFGoal *pGoal = Findgoal(Item->attack_finished);
			if (pGoal)
				DoResults(pGoal, AP, (Item->goal_result & TFGR_ADD_BONUSES));
		}
	}
}


//=========================================================================
// Remove any results applied to this player by the Goal
// Used when a GoalItem is dropped/removed
void RemoveResults(CTFGoal *Goal, CTFPlayer *pPlayer) 
{
	// Only remove the stats if the player has been affected
	// by this item. This is needed because the player may have
	// died since being affected
	if ( FClassnameIs( Goal, "item_tfgoal" ) )
	{
		if (!(pPlayer->item_list & Goal->item_list))
			return;

		if (Goal->goal_activation & TFGI_DONTREMOVERES)
			return;

		// Remove the affected flag
		pPlayer->item_list &= ~(Goal->item_list);
	}

	if (Goal->GetHealth() > 0)
		pPlayer->TakeDamage( CTakeDamageInfo( Goal, Goal, Goal->GetHealth(), DMG_IGNOREARMOR ) );
	if (Goal->GetHealth() < 0)
		pPlayer->TakeHealth( (0 - Goal->GetHealth()), 0 );
	pPlayer->lives -= Goal->lives;
//	pPlayer->armortype -= Goal->armortype;
	pPlayer->SetArmorValue( pPlayer->ArmorValue() - Goal->armorvalue );
//	pPlayer->armorclass &= ~(Goal->armorclass);
	
	if (Goal->frags)
	{
		pPlayer->TF_AddFrags(Goal->frags);
	}

	pPlayer->RemoveAmmo( Goal->ammo_shells, TF_AMMO_SHELLS );
	pPlayer->RemoveAmmo( Goal->ammo_nails, TF_AMMO_NAILS );
	pPlayer->RemoveAmmo( Goal->ammo_rockets, TF_AMMO_ROCKETS );
	pPlayer->RemoveAmmo( Goal->ammo_cells, TF_AMMO_CELLS );
	pPlayer->RemoveAmmo( Goal->ammo_medikit, TF_AMMO_MEDIKIT );
	pPlayer->RemoveAmmo( Goal->ammo_detpack, TF_AMMO_DETPACK );
	
	// Detpacks
//TFTODO: this should be handled in the GiveAmmo functions..
//	if (pPlayer->ammo_detpack > pPlayer->maxammo_detpack)
//		pPlayer->ammo_detpack = pPlayer->maxammo_detpack;
	
	// Grenades
	pPlayer->RemoveAmmo( Goal->no_grenades_1, TF_AMMO_GRENADES1 );
	pPlayer->RemoveAmmo( Goal->no_grenades_2, TF_AMMO_GRENADES2 );
	
	// If they had a primed grenade, and they don't have any more of
	// that type of grenade, unprime it and remove it.
	if (pPlayer->m_Shared.GetStateFlags() & TFSTATE_GRENPRIMED)
	{
		if (pPlayer->GetAmmoCount( TF_AMMO_GRENADES2 ) <= 0 || pPlayer->GetAmmoCount( TF_AMMO_GRENADES1 ) <= 0)
		{
			pPlayer->m_Shared.RemoveStateFlags( TFSTATE_GRENPRIMED );
			pPlayer->m_Shared.RemoveStateFlags( TFSTATE_GRENTHROWING );
			pPlayer->bRemoveGrenade = TRUE;
		}
	}

	BOOL puinvin = FALSE;
	BOOL puinvis = FALSE;
	BOOL puquad = FALSE;
	BOOL purad = FALSE;
	// Make sure we don't remove an effect another Goal is also supplying
	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "item_tfgoal" );
	while ( pEnt )
	{
		CTFGoalItem *pItem = dynamic_cast<CTFGoalItem*>( pEnt );
		if ( pItem )
		{
			if ( (pItem->GetOwnerEntity() == pPlayer) && (pEnt != Goal) )
			{
				if (pItem->invincible_finished > 0)
					puinvin = TRUE;
				if (pItem->invisible_finished > 0)
					puinvis = TRUE;
				if (pItem->super_damage_finished > 0)
					puquad = TRUE;
				if (pItem->radsuit_finished > 0)
					purad = TRUE;
			}
		}
		pEnt = gEntList.FindEntityByClassname( pEnt, "item_tfgoal" );
	}

	// Remove all powerups
	if ((Goal->invincible_finished > 0) && (!puinvin))
	{
		// if its a GoalItem, powerup was permanent, so we remove TFSTATE flag
		pPlayer->m_Shared.RemoveStateFlags( TFSTATE_INVINCIBLE );
		pPlayer->m_Shared.AddItemFlags( IT_INVULNERABILITY );
		pPlayer->invincible_finished = gpGlobals->curtime + Goal->invincible_finished;
	}
	if ((Goal->invisible_finished > 0) && (!puinvis))
	{
		// if its a GoalItem, powerup was permanent, so we remove TFSTATE flag
		pPlayer->m_Shared.RemoveStateFlags( TFSTATE_INVISIBLE );
		pPlayer->m_Shared.AddItemFlags( IT_INVISIBILITY );
		pPlayer->invisible_finished = gpGlobals->curtime + Goal->invisible_finished;
	}
	if ((Goal->super_damage_finished > 0) && (!puquad))
	{
		// if its a GoalItem, powerup was permanent, so we remove TFSTATE flag
		pPlayer->m_Shared.RemoveStateFlags( TFSTATE_QUAD );
		pPlayer->m_Shared.AddItemFlags( IT_QUAD );
		pPlayer->super_damage_finished = gpGlobals->curtime + Goal->super_damage_finished;
	}
	if ((Goal->radsuit_finished > 0) && (!purad))
	{
		// if its a GoalItem, powerup was permanent, so we remove TFSTATE flag
		pPlayer->m_Shared.RemoveStateFlags( TFSTATE_RADSUIT );
		pPlayer->m_Shared.AddItemFlags( IT_SUIT );
		pPlayer->radsuit_finished = gpGlobals->curtime + Goal->radsuit_finished;
	}

	// Now apply the pev->playerclass limitations & Redisplay Ammo counts
	//W_SetCurrentAmmo ();
}


//=========================================================================
// Give the GoalItem to a Player. 
void tfgoalitem_GiveToPlayer(CTFGoalItem *Item, CTFPlayer *AP, CTFGoal *Goal)
{
	MDEBUG(Warning( "Giving %s to %s\n", Item->GetEntityName().ToCStr(), AP->GetPlayerName()));

	// Don't let it re-drop
	if (Item->redrop_count)
		Item->SetThink( NULL );

	Item->SetOwnerEntity( AP );
	// Remove it from the map
	Item->FollowEntity( AP );
	// Play carry animations
	if (Item->GetModelName() != NULL_STRING)
	{
		Item->RemoveEffects( EF_NODRAW );

		Item->SetSequence( Item->LookupSequence( "carried" ) );
		if (Item->GetSequence() != -1)
		{
			Item->ResetSequenceInfo();
			Item->SetCycle( 0.0f );
		}
	}

	Item->AddSolidFlags( FSOLID_NOT_SOLID );

	// Do the deeds on the player
	if (Item->goal_activation & TFGI_GLOW)
		AP->AddEffects( EF_BRIGHTLIGHT );		//TFTODO: this used to be EF_BRIGHTFIELD.. make sure it's the same
	if (Item->goal_activation & TFGI_SLOW)
		AP->TeamFortress_SetSpeed();
	if (Item->speed_reduction)
		AP->TeamFortress_SetSpeed();

 	if (Item->goal_activation & TFGI_ITEMGLOWS)
	{
		Item->m_nRenderFX = kRenderFxNone;
		Item->SetRenderColor( 0, 0, 0, 0 );
	}

	// Light up console icons
	if (Item->items & IT_KEY1)
		AP->m_Shared.AddItemFlags( IT_KEY1 );
	if (Item->items & IT_KEY2)
		AP->m_Shared.AddItemFlags( IT_KEY2 );
	if (Item->items & IT_KEY3)
		AP->m_Shared.AddItemFlags( IT_KEY3 );
	if (Item->items & IT_KEY4)
		AP->m_Shared.AddItemFlags( IT_KEY4 );

	// Only do the results if we're allowed to
	if (Goal != Item)
	{
		if (Goal->goal_result & TFGR_NO_ITEM_RESULTS)
		{
			Item->goal_state = TFGS_ACTIVE;
			return;
		}
	}

	MDEBUG(Warning("Doing item results...\n"));

	// Prevent the Player from disguising themself if applicable
	if (Item->goal_result & TFGR_REMOVE_DISGUISE)
		AP->is_unableto_spy_or_teleport = 1;

	// Do the Results, adding the bonuses
	DoResults(Item, AP, TRUE);

	// Check the Item Group Stuff
	DoItemGroupWork(Item, AP);
}


//=========================================================================
// Drop the item 
void tfgoalitem_drop(CTFGoalItem *Item, BOOL PAlive, CTFPlayer *P)
{
	CBaseEntity *pOwner = Item->GetOwnerEntity();

	// Backup origin for retry at the drop
	if ( FBitSet( pOwner->GetFlags(), FL_DUCKING ) )
		Item->redrop_origin = pOwner->GetAbsOrigin() + Vector(0, 0, 26);
	else
		Item->redrop_origin = pOwner->GetAbsOrigin() + Vector(0, 0, 8);
	Item->redrop_count = 0;

	Item->SetTouch( &CTFGoalItem::item_tfgoal_touch );
	Item->DoDrop( Item->redrop_origin );

	Item->SetOwnerEntity( P );
	if (PAlive)
	{
		Vector vForward, vUp;
		AngleVectors( P->EyeAngles(), &vForward, NULL, &vUp );
		Item->SetAbsVelocity( (vForward * 400) + (vUp * 200) );

		Item->SetTouch( NULL );
		Item->SetThink( &CTFGoalItem::tfgoalitem_droptouch ); 	    // give it 0.75 seconds
		Item->SetNextThink( gpGlobals->curtime + 0.75 );		// and then set it's touch func

		// Prevent the dropping player from picking it up for longer
		Item->enemy = P;
		Item->m_flDroppedAt = gpGlobals->curtime;
	}
}


//=========================================================================
// Remove the GoalItem from a Player. 
void tfgoalitem_RemoveFromPlayer(CTFGoalItem *Item, CTFPlayer *AP, int iMethod) 
{
	MDEBUG(Warning("Removing %s from %s\n", STRING(Item->pev->netname), STRING(AP->pev->netname)));

	// If we have a teamcheck entity, use it instead
	if ( Item->owned_by_teamcheck != NULL_STRING )
		Item->owned_by = GetTeamCheckTeam( STRING(Item->owned_by_teamcheck) );

	BOOL lighton = FALSE;
	BOOL slowon = FALSE;
	BOOL key1on = FALSE;
	BOOL key2on = FALSE;
	BOOL key3on = FALSE;
	BOOL key4on = FALSE;
	BOOL spyoff = FALSE;
	// Remove the effects from the player
	// Make sure we don't remove an effect another Goal is also supplying
	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "item_tfgoal" );
	while ( pEnt )
	{
		CTFGoalItem *pItem = dynamic_cast<CTFGoalItem*>( pEnt );
		if ( pItem )
		{
			if ( (pItem->GetOwnerEntity() == AP) && (pEnt != Item) )
			{
				if (pItem->goal_activation & TFGI_GLOW)
					lighton = TRUE;
				if (pItem->goal_activation & TFGI_SLOW)
					slowon = TRUE;

				if (pItem->items & IT_KEY1)
					key1on = TRUE;
				if (pItem->items & IT_KEY2)
					key2on = TRUE;
				if (pItem->items & IT_KEY3)
					key3on = TRUE;
				if (pItem->items & IT_KEY4)
					key4on = TRUE;

				if (pItem->goal_result & TFGR_REMOVE_DISGUISE)
					spyoff = TRUE;
			}
		}
		pEnt = gEntList.FindEntityByClassname( pEnt, "item_tfgoal" );
	}

	// Check Powerups too
	if (!lighton)
	{
		if (AP->invincible_finished > gpGlobals->curtime + 3)
			lighton = TRUE;
		else if (AP->super_damage_finished > gpGlobals->curtime + 3)
			lighton = TRUE;
	}
	if (!lighton)
	{
		//TFTODO: Add support for EF_BRIGHTFIELD if necessary.
		//AP->RemoveEffects( EF_BRIGHTFIELD );
		AP->RemoveEffects( EF_BRIGHTLIGHT );
	}
 	if (Item->goal_activation & TFGI_ITEMGLOWS)
	{
		Item->m_nRenderFX = kRenderFxGlowShell;

//		if (Item->owned_by > 0 && Item->owned_by <= 4)
//			Item->m_clrRender = Vector255ToRGBColor( rgbcolors[Item->owned_by] );
//		else
//			Item->m_clrRender = Vector255ToRGBColor( rgbcolors[0] );

		if ( TFTeamMgr()->IsValidTeam( Item->owned_by ) )
		{
			CTFTeam *pTeam = TFTeamMgr()->GetTeam( Item->owned_by );
			Item->m_clrRender = pTeam->GetColor();
		}
		else
		{
			Item->m_clrRender = TFTeamMgr()->GetUndefinedTeamColor();
		}

		Item->SetRenderColorA( 100 );	// Shell size
	}

	// Remove the Spy prevention
	if (!spyoff)
		AP->is_unableto_spy_or_teleport = FALSE;
	// Remove the lit console key icons
	if (!key1on)
		AP->m_Shared.RemoveItemFlags( IT_KEY1 );
	if (!key2on)
		AP->m_Shared.RemoveItemFlags( IT_KEY2 );
	if (!key3on)
		AP->m_Shared.RemoveItemFlags( IT_KEY3 );
	if (!key4on)
		AP->m_Shared.RemoveItemFlags( IT_KEY4 );

	// Remove AP Modifications
	// Go through all the players and do any results
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && IsAffectedBy(Item, pPlayer, AP) )
			RemoveResults(Item, pPlayer);
	}

	// Setup animations
	if (Item->GetModelName() != NULL_STRING)
	{
		Item->SetSequence( Item->LookupSequence( "not_carried" ) );
		if (Item->GetSequence() != -1)
		{
			Item->ResetSequenceInfo();
			Item->SetCycle( 0.0f );
		}
	}

	// Return it to the starting point if the flag is set
	if (iMethod == GI_DROP_PLAYERDEATH || iMethod == GI_DROP_PLAYERDROP)
	{
		// Do messages
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( !pPlayer )
				continue;

			if (pPlayer->GetTeamNumber() == Item->owned_by)
			{
				if (Item->team_drop != NULL_STRING)
					UTIL_ShowMessage( STRING(Item->team_drop), pPlayer );
				if (Item->netname_team_drop != NULL_STRING)
					ClientPrint( pPlayer, HUD_PRINTNOTIFY, STRING(Item->netname_team_drop), AP->GetPlayerName() );
				// Old printing
				if (Item->org_team_drop != NULL_STRING)
					ClientPrint( pPlayer, HUD_PRINTCENTER, STRING(Item->org_team_drop) );
			}
			else // (pPlayer->GetTeamNumber() != Item->owned_by)
			{
				if (Item->non_team_drop != NULL_STRING)
					UTIL_ShowMessage( STRING(Item->non_team_drop), pPlayer );
				if (Item->netname_non_team_drop != NULL_STRING)
					ClientPrint( pPlayer, HUD_PRINTNOTIFY, STRING(Item->netname_non_team_drop), AP->GetPlayerName() );
				// Old printing
				if (Item->org_non_team_drop != NULL_STRING)
					ClientPrint( pPlayer, HUD_PRINTCENTER, STRING(Item->org_non_team_drop) );
			}
		}

		// Drop it if the flag is set
		if (Item->goal_activation & TFGI_RETURN_DROP)
		{
			CTimer *pTimer = Timer_CreateTimer( Item, TF_TIMER_RETURNITEM );
			pTimer->m_flNextThink = gpGlobals->curtime + 0.5f;
			if (iMethod == GI_DROP_PLAYERDEATH)
				pTimer->weapon = GI_RET_DROP_DEAD;
			else 
				pTimer->weapon = GI_RET_DROP_LIVING;
		}
		else if (Item->goal_activation & TFGI_DROP)
		{
			if ( (iMethod == GI_DROP_PLAYERDROP) && (Item->goal_activation & TFGI_CANBEDROPPED) )
				tfgoalitem_drop(Item, TRUE, AP);
			else
				tfgoalitem_drop(Item, FALSE, AP);
		}
		else
		{
			// Remove the Item
			Item->SetOwnerEntity( NULL );
			Item->SetNextThink( gpGlobals->curtime );
			Item->SetThink( &CBaseEntity::SUB_Remove );
			AP->TeamFortress_SetSpeed();
			return;
		}

		Item->SetOwnerEntity( NULL );
		Item->RemoveFlag( FL_ONGROUND );
		UTIL_SetSize(Item, Item->goal_min, Item->goal_max);

		AP->TeamFortress_SetSpeed();
	}
	else if (iMethod == GI_DROP_REMOVEGOAL)		
	{
		Item->SetOwnerEntity( NULL );

		if (Item->goal_activation & TFGI_RETURN_GOAL)
		{
			CTimer *pTimer = Timer_CreateTimer( Item, TF_TIMER_RETURNITEM );
			pTimer->m_flNextThink = gpGlobals->curtime + 0.5;
			pTimer->weapon = GI_RET_GOAL;
			AP->TeamFortress_SetSpeed();
			return;
		}

		// Don't remove it, since it may be given away again later
		Item->AddSolidFlags( FSOLID_NOT_SOLID );
		Item->AddEffects( EF_NODRAW );
		Item->StopFollowingEntity();
		AP->TeamFortress_SetSpeed();
	}
}


//=========================================================================
// Apply modifications to the Player passed in
void Apply_Results(CTFGoal *Goal, CTFPlayer *Player, CTFPlayer *AP, BOOL bAddBonuses)
{
	MDEBUG( Warning("Applying Results from %s to %s\n", STRING(Goal->pev->netname), STRING(Player->pev->netname)) );

#if 0
	// If this is a goalitem, record the fact that this player
	// has been affected by it.
	if ( FClassnameIs(Goal, "item_tfgoal") )
		Player->item_list |= Goal->item_list;

	if (Player == AP)
	{
		// Alter the team score
		if (Goal->count != 0 && Player->GetTeamNumber() > 0)
		{
			TeamFortress_TeamIncreaseScore(Player->GetTeamNumber(), Goal->count);
			// Display short team scores
			//TeamFortress_TeamShowScores(FALSE, NULL);
		}
	}

	// Apply Stats, only if told to
	if (bAddBonuses)
	{
		MDEBUG( Warning("Adding bonuses.\n") );
		// Some results are not applied to dead players
		if ( Player->IsAlive() )
		{
			if (Goal->GetHealth() > 0)
				Player->TakeHealth(Goal->GetHealth(), 0);
			if (Goal->GetHealth() < 0)
			{
				// Make sure we don't gib them, because it creates too many entities if 
				// a lot of players are affected by this Goal.
				Player->TakeDamage( CTakeDamageInfo( Goal, Goal, (0 - Goal->GetHealth()), DMG_IGNOREARMOR | DMG_NEVERGIB ) );
			}
		}

		// The player may be dead now, so check again
		if ( Player->IsAlive() )
		{
//			if (Goal->armortype > 0)
//				Player->armortype = Goal->armortype;
//			if (Goal->armorvalue > 0)
//				Player->armortype = Player->armor_allowed;
			Player->IncrementArmorValue( Goal->armorvalue );
//			if (Goal->armorclass > 0)
//				Player->armorclass = Goal->armorclass;

			Player->GiveAmmo( Goal->ammo_shells, TF_AMMO_SHELLS );
			Player->GiveAmmo( Goal->ammo_nails, TF_AMMO_NAILS );
			Player->GiveAmmo( Goal->ammo_rockets, TF_AMMO_ROCKETS );
			Player->GiveAmmo( Goal->ammo_cells, TF_AMMO_CELLS );
			Player->GiveAmmo( Goal->ammo_medikit, TF_AMMO_MEDIKIT );
			Player->GiveAmmo( Goal->ammo_detpack, TF_AMMO_DETPACK );

#ifdef TFTODO // do this when grenades are implemented.
			// Grenades
			if ( Player->tp_grenades_1 != GR_TYPE_NONE )
				Player->no_grenades_1 += Goal->no_grenades_1;
			if ( Player->tp_grenades_2 != GR_TYPE_NONE )
				Player->no_grenades_2 += Goal->no_grenades_2;

			// If they had a primed grenade, and they don't have any more of
			// that type of grenade, unprime it and remove it.
			if (Player->tfstate & TFSTATE_GRENPRIMED)
			{
				if ( (Player->m_iPrimedGrenType == 1 && Player->no_grenades_1 <= 0 && Goal->no_grenades_1 < 0) || 
					 (Player->m_iPrimedGrenType == 2 && Player->no_grenades_2 <= 0 && Goal->no_grenades_2 < 0) )
				{
					Player->tfstate &= ~TFSTATE_GRENPRIMED;
					Player->tfstate &= ~TFSTATE_GRENTHROWING;
					Player->bRemoveGrenade = TRUE;
				}
			}
#endif
		
			// Apply any powerups
			if (Goal->invincible_finished > 0)
			{
				Player->m_Shared.AddItemFlags( IT_INVULNERABILITY );
				Player->invincible_finished = gpGlobals->curtime + Goal->invincible_finished;
				// if its a GoalItem, powerup is permanent, so we use TFSTATE flags
				if ( FClassnameIs(Goal, "item_tfgoal") )
				{
					Player->m_Shared.AddStateFlags( TFSTATE_INVINCIBLE );
					Player->invincible_finished = gpGlobals->curtime + 666;
				}

				// Force it to recalculate shell color
				Player->m_nRenderFX = kRenderFxNone;
			}
			if (Goal->invisible_finished > 0)
			{
				Player->m_Shared.AddItemFlags( IT_INVISIBILITY );
				Player->invisible_finished = gpGlobals->curtime + Goal->invisible_finished;
				// if its a GoalItem, powerup is permanent, so we use TFSTATE flags
				if ( FClassnameIs(Goal, "item_tfgoal") )
				{
					Player->m_Shared.AddStateFlags( TFSTATE_INVISIBLE );
					Player->invisible_finished = gpGlobals->curtime + 666;
				}

				// Force it to recalculate shell color
				Player->m_nRenderFX = kRenderFxNone;
			}
			if (Goal->super_damage_finished > 0)
			{
				Player->m_Shared.AddItemFlags( IT_QUAD );
				Player->super_damage_finished = gpGlobals->curtime + Goal->super_damage_finished;
				// if its a GoalItem, powerup is permanent, so we use TFSTATE flags
				if ( FClassnameIs(Goal, "item_tfgoal") )
				{
					Player->m_Shared.AddStateFlags( TFSTATE_QUAD );
					Player->super_damage_finished = gpGlobals->curtime + 666;
				}

				// Force it to recalculate shell color
				Player->m_nRenderFX = kRenderFxNone;
			}
			if (Goal->radsuit_finished > 0)
			{
				Player->m_Shared.AddItemFlags( IT_SUIT );
				Player->radsuit_finished = gpGlobals->curtime + Goal->radsuit_finished;
				// if its a GoalItem, powerup is permanent, so we use TFSTATE flags
				if ( FClassnameIs(Goal, "item_tfgoal") )
				{
					Player->m_Shared.AddStateFlags( TFSTATE_RADSUIT );
					Player->radsuit_finished = gpGlobals->curtime + 666;
				}
			}
		}

		// These results are applied to dead and living players
		Player->lives += Goal->lives;

		if ( Goal->frags != 0 )
			Player->TF_AddFrags(Goal->frags);
	}
#ifdef MAP_DEBUG
	else
		ALERT( at_console, "NOT Adding bonuses.\n" );
#endif

	// If the Goal resets Spy skin/color then do it
	CTFPlayerClass *pPlayerClass = Player->GetPlayerClass();	
	if ( pPlayerClass && pPlayerClass->IsClass( TF_CLASS_SPY ) && Goal->goal_result & TFGR_REMOVE_DISGUISE )
	{
		pPlayerClass->Spy_RemoveDisguise();
	}

	// If there's a GoalItem for this goal, give it to the player
	// GoalItems use "items" for the console lights... so don't do it for items.
	if ( Goal->items != 0 && !FClassnameIs(Goal,"item_tfgoal") )
	{
		// Find the item
		CTFGoalItem *pItem = Finditem(Goal->items);
		// Don't give them the item if it's the item that just affected them
		if (pItem != NULL && pItem != Goal)
			tfgoalitem_GiveToPlayer(pItem, Player, Goal);
	}

	// If this goal removes an item from the player, remove it
	if (Goal->axhitme != 0)
	{
		CTFGoalItem *pItem = Finditem(Goal->axhitme);
		if (pItem->GetOwnerEntity() == Player)
			tfgoalitem_RemoveFromPlayer(pItem, Player, GI_DROP_REMOVEGOAL);
	}

	// if this goal removes a group of items from the player, remove them
	if (Goal->remove_item_group != 0)
	{
		// Find all items in the group
		CTFGoalItem *pItemToRemove = NULL;
		CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "item_tfgoal" );
		while ( pEnt )
		{
			CTFGoalItem *pItem = dynamic_cast<CTFGoalItem*>( pEnt );
			if ( pItem )
			{
				if ( (pItem->group_no == Goal->remove_item_group) && (pItem->GetOwnerEntity() == Player) )
					pItemToRemove = pItem;
				
				// need to cycle before removing it from the player, because it may be destroyed
				pEnt = gEntList.FindEntityByClassname( pEnt, "item_tfgoal" );
				if (pItemToRemove)
				{
					tfgoalitem_RemoveFromPlayer(pItemToRemove, Player, GI_DROP_REMOVEGOAL);
					pItemToRemove = NULL;
				}
			}
		}
	}

	// Display all the item statuses
	Player->DisplayLocalItemStatus(Goal);

	// Destroy buildings
	if (Goal->goal_result & TFGR_DESTROY_BUILDINGS)
	{
		Player->no_sentry_message = TRUE;
		Player->no_dispenser_message = TRUE;
		Player->no_entry_teleporter_message = TRUE;
		Player->no_exit_teleporter_message = TRUE;
		Player->Engineer_RemoveBuildings();
		Player->TeamFortress_RemoveLiveGrenades();
		Player->TeamFortress_RemoveRockets();
		Player->RemovePipebombs();

		// is the player setting a detpack?
		if ( Player->is_detpacking )
		{
			Player->TeamFortress_DetpackStop();
		}
		else
		{
			// does the player have a detpack in the world?
			if ( Player->TeamFortress_RemoveDetpacks() )
			{
				Player->GiveAmmo( 1, TF_AMMO_DETPACK );
			}
		}
	}

	// Force respawns
	if (Goal->goal_result & TFGR_FORCE_RESPAWN)
	{
		// Only if they're alive
		if ( Player->IsAlive() )
			Player->ForceRespawn();
	}

#endif
}


//=========================================================================
// Use (Triggered) function for Goals
void EndRound( CTFGoal *pGoal )
{
	// fade everyones screen
	color32 clr;
	memset( &clr, 0, sizeof( clr ) );
	UTIL_ScreenFadeAll( clr, 0.3, pGoal->m_flEndRoundTime, FFADE_MODULATE | FFADE_OUT );

	// Display Long TeamScores to everyone
//	TeamFortress_TeamShowScores(TRUE, NULL);
	TFTeamMgr()->ShowScores();

	int highestScore = -99990;
	int winningTeam = 1;
	const char *winnerMsg = "";
	// Only do team score check if the win one is set
	if ( pGoal->m_iszEndRoundMsg_Team1_Win != NULL_STRING )
	{
		// work out which team won
		int nTeamCount = TFTeamMgr()->GetTeamCount();
		for ( int iTeam = 1; iTeam < nTeamCount; ++iTeam )
		{
			//			int teamScore = TeamFortress_TeamGetScoreFrags( i );
			CTFTeam *pTeam = TFTeamMgr()->GetTeam( iTeam );
			if ( pTeam )
			{
				int teamScore = pTeam->GetScore();
				if ( teamScore > highestScore )
				{
					winningTeam = iTeam;
					highestScore = teamScore;
				}
			}
		}

		// work out the winning msg
		switch ( winningTeam )
		{
		case 1:		winnerMsg = STRING(pGoal->m_iszEndRoundMsg_Team1_Win);		break;
		case 2:		winnerMsg = STRING(pGoal->m_iszEndRoundMsg_Team2_Win);		break;
		case 3:		winnerMsg = STRING(pGoal->m_iszEndRoundMsg_Team3_Win);		break;
		case 4:		winnerMsg = STRING(pGoal->m_iszEndRoundMsg_Team4_Win);		break;
		};
	}

	// Prevent players from moving and shooting
	no_cease_fire_text = TRUE;
	cease_fire = TRUE;

	// Send out the messages
	CTFPlayer *client = NULL;
	while ( ((client = (CTFPlayer*)gEntList.FindEntityByClassname( client, "player" )) != NULL) && (!FNullEnt(client->edict())) ) 
	{
		if ( !client )
			continue;

		// Freeze all the players
		if ( client->IsObserver() == FALSE )
		{
			//TFTODO implement something for this?
			// iuser4 stops firing on the clients
			//client->pev->iuser4 = TRUE;

			//TFTODO: implement HIDEHUD_WEAPONS, or is it the same as HIDEHUD_WEAPONSELECTION?
			//client->m_Local.m_iHideHUD |= (HIDEHUD_HEALTH | HIDEHUD_WEAPONS);
			client->m_Local.m_iHideHUD |= (HIDEHUD_HEALTH | HIDEHUD_WEAPONSELECTION);
			
			client->m_Shared.AddStateFlags( TFSTATE_CANT_MOVE );
		}
		client->TeamFortress_SetSpeed();

		// Owned by and Non owned by take precedence
		if ( pGoal->m_iszEndRoundMsg_OwnedBy != NULL_STRING && ( client->GetTeamNumber() == pGoal->owned_by ) )
		{
			UTIL_ShowMessage( STRING( pGoal->m_iszEndRoundMsg_OwnedBy ), client );
		}
		else if ( pGoal->m_iszEndRoundMsg_NonOwnedBy != NULL_STRING && ( client->GetTeamNumber() != pGoal->owned_by ) )
		{
			UTIL_ShowMessage( STRING( pGoal->m_iszEndRoundMsg_NonOwnedBy ), client );
		}
		else if ( pGoal->m_iszEndRoundMsg_Team1_Win != NULL_STRING && client->GetTeamNumber() == winningTeam )
		{
			UTIL_ShowMessage( winnerMsg, client );
		}
		else
		{
			const char *loserMsg = "";
			// work out the loser message and send it to them
			if ( pGoal->m_iszEndRoundMsg_Team1_Win != NULL_STRING )
			{
				switch ( client->GetTeamNumber() )
				{
				case 1:		loserMsg = STRING(pGoal->m_iszEndRoundMsg_Team1_Lose);		break;
				case 2:		loserMsg = STRING(pGoal->m_iszEndRoundMsg_Team2_Lose);		break;
				case 3:		loserMsg = STRING(pGoal->m_iszEndRoundMsg_Team3_Lose);		break;
				case 4:		loserMsg = STRING(pGoal->m_iszEndRoundMsg_Team4_Lose);		break;
				};
			}
			else
			{
				switch ( client->GetTeamNumber() )
				{
				case 1:		loserMsg = STRING(pGoal->m_iszEndRoundMsg_Team1);		break;
				case 2:		loserMsg = STRING(pGoal->m_iszEndRoundMsg_Team2);		break;
				case 3:		loserMsg = STRING(pGoal->m_iszEndRoundMsg_Team3);		break;
				case 4:		loserMsg = STRING(pGoal->m_iszEndRoundMsg_Team4);		break;
				};
			}

			UTIL_ShowMessage( loserMsg, client );
		}
	}

	// Create a timer to remove the EndRound in the specified time
	CTimer *pTimer = Timer_CreateTimer( pGoal, TF_TIMER_ENDROUND );
	//pTimer->SetThink( &CBaseEntity::EndRoundEnd );
	pTimer->m_flNextThink = gpGlobals->curtime + pGoal->m_flEndRoundTime;
}


//=========================================================================
// Inactivate a Timer/Goal
void InactivateGoal(CTFGoal *Goal)
{
	MDEBUG( Warning("Inactivating %s", STRING(Goal->pev->netname)) );

	if (Goal->goal_state == TFGS_ACTIVE)
	{
		MDEBUG( Warning("... succeeded.\n") );
		// Not a timer goal
		if (Goal->Classify() != CLASS_TFGOAL_TIMER)
		{
			if ( Goal->goal_activation & TFGI_SOLID && (Goal->Classify() == CLASS_TFGOAL || Goal->Classify() == CLASS_TFGOAL_ITEM) )
				Goal->SetSolid( SOLID_BBOX );
			else
				Goal->AddSolidFlags( FSOLID_TRIGGER );
		}

		Goal->goal_state = TFGS_INACTIVE;
		const char *pModel = STRING( Goal->GetModelName() );
		if (pModel && pModel[0] != '*')
			Goal->RemoveEffects( EF_NODRAW );
	}
#ifdef MAP_DEBUG
	else
		Warning("... failed. Goal is %s\n", g_szStates[Goal->goal_state]);
#endif
}


//=========================================================================
// Restores a Timer/Goal
void RestoreGoal(CTFGoal *Goal)
{
	MDEBUG( Warning("Attempting to Restore %s", STRING(Goal->pev->netname)) );

	if (Goal->goal_state == TFGS_REMOVED)
	{
		MDEBUG( Warning("... succeeded.\n") );

		// Not a timer goal
		if (Goal->search_time == 0)
		{
			if (Goal->goal_activation & TFGI_SOLID && FClassnameIs(Goal, "item_tfgoal") )
				Goal->SetSolid( SOLID_BBOX );
			else
				Goal->AddSolidFlags( FSOLID_TRIGGER );
		}
		else
			Goal->SetNextThink( gpGlobals->curtime + Goal->search_time );

		Goal->goal_state = TFGS_INACTIVE;
		const char *pModel = STRING(Goal->GetModelName());
		if (pModel[0] != '*')
			Goal->RemoveEffects( EF_NODRAW );
	}
#ifdef MAP_DEBUG
	else
		Warning("... failed. Goal is %s\n", g_szStates[Goal->goal_state]);
#endif
}


//=========================================================================
// Do all the activation/inactivation/etc of Goal Groups
void DoGroupWork(CTFGoal *Goal, CTFPlayer *AP)
{
#ifdef MAP_DEBUG
	if (Goal->all_active || Goal->activate_group_no || Goal->inactivate_group_no || Goal->restore_group_no || Goal->remove_group_no)
		Warning("Doing Groupwork...\n");
#endif

	// Check all goals activated flag
	if (Goal->all_active != 0)
	{
		if (Goal->last_impulse == 0)
		{
			// No goal specified in .last_impulse. Print error.
			Warning("Goal %d has .all_active specified, but no .last_impulse\n", Goal->goal_no);
		}
		else
		{
			MDEBUG( Warning("All Active Group Check.\n") );

			BOOL bAllSet = TRUE;
			// Find all goals
			CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "info_tfgoal" );
			while ( pEnt && bAllSet)
			{
				CTFGoal *pGoal = dynamic_cast< CTFGoal* >( pEnt );
				if ( pGoal )
				{
					if (pGoal->group_no == Goal->all_active && pGoal->goal_state != TFGS_ACTIVE)
						bAllSet = FALSE;
				}

				pEnt = gEntList.FindEntityByClassname( pEnt, "info_tfgoal" );
			}

			// If all goals in this group are activated, do it
			if (bAllSet)
			{
				MDEBUG( Warning("All Active, Activating last_impulse.\n") );

				CTFGoal *pGoal = Findgoal(Goal->last_impulse);
				if (pGoal)
					DoResults(pGoal, AP, (Goal->goal_result & TFGR_ADD_BONUSES));
			}
		#ifdef MAP_DEBUG
			else
			{
				Warning("Not all Active.\n");
			}
		#endif
		}
	}

	// Check Activate all in the group flag
	if (Goal->activate_group_no != 0)
	{
		// Find all goals
		CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "info_tfgoal" );
		while ( pEnt )
		{
			CTFGoal *pGoal = dynamic_cast< CTFGoal* >( pEnt );
			if ( pGoal )
			{
				if (pGoal->group_no == Goal->activate_group_no)
					ActivateDoResults(pGoal, AP, Goal);
			}

			pEnt = gEntList.FindEntityByClassname( pEnt, "info_tfgoal" );
		}
	}

	// Check Inactivate all in the group flag
	if (Goal->inactivate_group_no != 0)
	{
		// Find all goals
		CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "info_tfgoal" );
		while ( pEnt )
		{
			CTFGoal *pGoal = dynamic_cast< CTFGoal* >( pEnt );
			if ( pGoal )
			{
				if (pGoal->group_no == Goal->inactivate_group_no)
					InactivateGoal(pGoal);
			}

			pEnt = gEntList.FindEntityByClassname( pEnt, "info_tfgoal" );
		}
	}

	// Check Remove all in the group flag
	if (Goal->remove_group_no != 0)
	{
		// Find all goals
		CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "info_tfgoal" );
		while ( pEnt )
		{
			CTFGoal *pGoal = dynamic_cast< CTFGoal* >( pEnt );
			if ( pGoal )
			{
				if (pGoal->group_no == Goal->remove_group_no)
					RemoveGoal(pGoal);
			}

			pEnt = gEntList.FindEntityByClassname( pEnt, "info_tfgoal" );
		}
	}
	
	// Check Restore all in the group flag
	if (Goal->restore_group_no != 0)
	{
		// Find all goals
		CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "info_tfgoal" );
		while ( pEnt )
		{
			CTFGoal *pGoal = dynamic_cast< CTFGoal* >( pEnt );
			if ( pGoal )
			{
				if (pGoal->group_no == Goal->restore_group_no)
					RestoreGoal(pGoal);
			}

			pEnt = gEntList.FindEntityByClassname( pEnt, "info_tfgoal" );
		}
	}

#ifdef MAP_DEBUG
	if (Goal->remove_spawngroup || Goal->restore_spawngroup)
		Warning("Doing SpawnGroupwork...\n");
#endif
}



//=========================================================================
// Do all the activation/inactivation/etc of individual Goals
void DoGoalWork(CTFGoal *Goal, CTFPlayer *AP)
{
#ifdef MAP_DEBUG
	if (Goal->activate_goal_no || Goal->inactivate_goal_no || Goal->restore_goal_no || Goal->remove_goal_no || Goal->return_item_no)
		Warning("Doing Goalwork...\n");
#endif

	// If another goal should be activated, activate it
	if (Goal->activate_goal_no != 0)
	{
		CTFGoal *pFoundGoal = Findgoal(Goal->activate_goal_no);
		if (pFoundGoal)
			ActivateDoResults(pFoundGoal, AP, Goal);
	}

	// If another goal should be inactivated, inactivate it
	if (Goal->inactivate_goal_no != 0)
	{
		CTFGoal *pFoundGoal = Findgoal(Goal->inactivate_goal_no);
		if (pFoundGoal)
			InactivateGoal(pFoundGoal);
	}

	// If another goal should be restored, restore it
	if (Goal->restore_goal_no != 0)
	{
		CTFGoal *pFoundGoal = Findgoal(Goal->restore_goal_no);
		if (pFoundGoal)
			RestoreGoal(pFoundGoal);
	}

	// If another goal should be removed, remove it
	if (Goal->remove_goal_no != 0)
	{
		CTFGoal *pFoundGoal = Findgoal(Goal->remove_goal_no);
		if (pFoundGoal)
			RemoveGoal(pFoundGoal);
	}

	// If a GoalItem should be returned, return it
	if (Goal->return_item_no != 0)
	{
		CTFGoalItem *pFoundGoal = Finditem(Goal->return_item_no);
		if (pFoundGoal)
		{
			CBaseEntity *pOwner = pFoundGoal->GetOwnerEntity();
			Assert( dynamic_cast<CTFPlayer*>( pOwner ) );
			if (pFoundGoal->goal_state == TFGS_ACTIVE)
				tfgoalitem_RemoveFromPlayer(pFoundGoal, (CTFPlayer*)pOwner, GI_DROP_REMOVEGOAL);

			// Setup a ReturnItem timer
			CTimer *pTimer = Timer_CreateTimer( pFoundGoal, TF_TIMER_RETURNITEM );
			pTimer->weapon = GI_RET_TIME;
			pTimer->m_flNextThink = gpGlobals->curtime + 0.1;

			pFoundGoal->AddSolidFlags( FSOLID_NOT_SOLID );
		}
	}

#ifdef MAP_DEBUG
	if (Goal->remove_spawnpoint || Goal->restore_spawnpoint)
		Warning("Doing Spawnwork...\n");
#endif

	// Spawnpoint behaviour
	if (Goal->remove_spawnpoint != 0)
	{
		CTFSpawn *pFoundGoal = Findteamspawn(Goal->remove_spawnpoint);
		if (pFoundGoal)
			InactivateSpawn(pFoundGoal);
	}

	if (Goal->restore_spawnpoint != 0)
	{
		CTFSpawn *pFoundGoal = Findteamspawn(Goal->restore_spawnpoint);
		if (pFoundGoal)
		{
			if (pFoundGoal->goal_state == TFGS_REMOVED)
				ActivateSpawn(pFoundGoal);
		}
	}
}


//=========================================================================
// Do all the activation/removal of Quake Triggers
void DoTriggerWork(CTFGoal *Goal, CTFPlayer *AP)
{
	// remove killtargets
	if (Goal->killtarget != NULL_STRING)
	{
		MDEBUG( Warning("Doing Triggerwork...\n") );
		MDEBUG( Warning("Killing Target(s): %s\n", STRING(Goal->killtarget)) );

		CBaseEntity *pentKillTarget = gEntList.FindEntityByName( NULL, STRING(Goal->killtarget) );
		while ( pentKillTarget )
		{
			UTIL_Remove( pentKillTarget );
			pentKillTarget = gEntList.FindEntityByName( pentKillTarget, STRING(Goal->killtarget) );
		}
	}

	// fire targets
	if (Goal->target != NULL_STRING)
	{
		MDEBUG( Warning("Doing Triggerwork...\n") );
		MDEBUG( ALERT( at_console, "Activating Target(s): %s\n", STRING(Goal->pev->target) ) );

		CBaseEntity *pentTarget = gEntList.FindEntityByName( NULL, STRING(Goal->target) );
		while ( pentTarget )
		{
			CBaseEntity *pTarget = pentTarget;
			if ( !(pTarget->GetFlags() & FL_KILLME) ) 
				pTarget->Use( AP, Goal, USE_TOGGLE, 0 );
			pentTarget = gEntList.FindEntityByName( pentTarget, STRING(Goal->target) );
		}
	}
}


//=========================================================================
// Setup the way this Timer/Goal/Item will respawn
void SetupRespawn(CTFGoal *pGoal)
{
	MDEBUG( Warning("Setting up Respawn...\n") );

	pGoal->m_bAddBonuses = FALSE;

	// Check status of respawn for this goal
	// Single Activation, do nothing
	if (pGoal->goal_result & TFGR_SINGLE)
	{
		RemoveGoal(pGoal);
		return;
	}

	// Timer Goal?
	if (pGoal->Classify() == CLASS_TFGOAL_TIMER)
	{
		InactivateGoal(pGoal);
		pGoal->SetThink( &CTFGoal::tfgoal_timer_tick );
		pGoal->SetNextThink( gpGlobals->curtime + pGoal->search_time );
		return;
	}

	// Respawn Activation, set up respawn
	if (pGoal->wait > 0)
	{
		pGoal->SetThink(&CTFGoal::DoRespawn);
		pGoal->SetNextThink( gpGlobals->curtime + pGoal->wait );
		return;
	}
	// Permanently active goal?
	else if (pGoal->wait == -1)
		return;

	// Otherwise, it's a Multiple Goal
	InactivateGoal(pGoal);
}


//=========================================================================
// Do the results for the Timer/Goal/Item
void DoResults(CTFGoal *Goal, CTFPlayer *AP, BOOL bAddBonuses) 
{
	// Can't activate during PreMatch time
	if ( (TFGameRules()->IsInPreMatch()) && (Goal->Classify() != CLASS_TFGOAL_TIMER) )
		return;

	// Is the goal already activated?
	// This check is needed for goals which are being activated by other goals
	if (Goal->goal_state == TFGS_ACTIVE)
		return;

	// Delayed Activation?
	if (Goal->delay_time > 0 && Goal->goal_state != TFGS_DELAYED)
	{
		MDEBUG( Warning("Delaying Results of %s\n", STRING(Goal->edict()->netname)) );

		Goal->goal_state = TFGS_DELAYED;
		Timer_CreateTimer( Goal, TF_TIMER_DELAYEDGOAL );
		Goal->enemy = AP;
		Goal->SetThink( &CTFGoal::DelayedResult );
		Goal->SetNextThink( gpGlobals->curtime + Goal->delay_time );
		Goal->weapon = bAddBonuses;
		return;
	}

	// If we have a teamcheck entity, use it instead
	if ( Goal->owned_by_teamcheck != NULL_STRING )
		Goal->owned_by = GetTeamCheckTeam( STRING(Goal->owned_by_teamcheck) );

	Goal->goal_state = TFGS_INACTIVE;
	

	// if it's a TF goal, removes it's model
	if ( Goal->Classify() == CLASS_TFGOAL || Goal->Classify() == CLASS_TFGOAL_TIMER )
		Goal->AddEffects( EF_NODRAW );

#ifdef MAP_DEBUG
	Warning("---= Activation =---\n");
	if (AP)
		Warning("Goal: %s\nAP  : %s\n", STRING(Goal->edict()->netname), AP->GetPlayerName());
	else
		Warning("Goal: %s\nAP  : NONE\n", STRING(Goal->edict()->netname));
	if (bAddBonuses)
		Warning("  adding bonuses\n-=================-\n");
	else
		Warning("NOT adding bonuses\n-=================-\n");
#endif

	// Make the sound
	if (Goal->noise != NULL_STRING)
	{
		Goal->EmitSound( STRING( Goal->noise ) );
	}

	// Increase scores
	BOOL bDumpScores = FALSE;
	int i;
	for ( i = 0; i <= 3; i++)
	{
		if (Goal->increase_team[i] != 0)
		{
			TeamFortress_TeamIncreaseScore(i + 1, Goal->increase_team[i]);
			bDumpScores = TRUE;
		}
	}

	// Increase the score of the team that owns this entity
	if ( ( Goal->increase_team_owned_by != 0 ) && ( Goal->owned_by != 0 ) )
	{
		TeamFortress_TeamIncreaseScore( Goal->owned_by, Goal->increase_team_owned_by );
		bDumpScores = TRUE;
	}

	// CTF Map support
	if (TFGameRules()->CTF_Map == TRUE && AP != NULL)
	{
		if (Goal->goal_no == CTF_FLAG1 || Goal->goal_no == CTF_FLAG1 || Goal->goal_no == CTF_DROPOFF1 || Goal->goal_no == CTF_DROPOFF1)
		{
			// Do Messaging
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
				if ( !pPlayer )
					continue;

				if ( (pPlayer->GetTeamNumber() == 2 && Goal->goal_no == CTF_FLAG1) || (pPlayer->GetTeamNumber() == 1 && Goal->goal_no == CTF_FLAG2) )
				{
					if (pPlayer == AP)
						ClientPrint( pPlayer, HUD_PRINTCENTER, "You got the enemy flag!\n\nReturn to base!");
					else
						ClientPrint( pPlayer, HUD_PRINTCENTER, "Your team GOT the ENEMY flag!!");
				}
				else if (Goal->goal_no == CTF_FLAG1 || Goal->goal_no == CTF_FLAG2) 
				{
					ClientPrint( pPlayer, HUD_PRINTCENTER, "Your flag has been TAKEN!!");
				}
				else if ( (pPlayer->GetTeamNumber() == 2 && Goal->goal_no == CTF_DROPOFF1) || (pPlayer->GetTeamNumber() == 1 && Goal->goal_no == CTF_DROPOFF2) )
				{
					if (pPlayer == AP)
						ClientPrint( pPlayer, HUD_PRINTCENTER, "You CAPTURED the FLAG!!");
					else
						ClientPrint( pPlayer, HUD_PRINTCENTER, "Your flag was CAPTURED!!");
				}
				else if (Goal->goal_no == CTF_DROPOFF1 || Goal->goal_no == CTF_DROPOFF2) 
				{
					ClientPrint( pPlayer, HUD_PRINTCENTER, "Your team CAPTURED the flag!!");
				}
			}

			const char *pTeamName = "Spectator";
			if ( AP->GetTeamNumber() != 0 )
			{
				CTeam *pTeam = GetGlobalTeam( AP->GetTeamNumber() );
				if ( pTeam )
					pTeamName = pTeam->GetName();
			}

			// Console Prints
			switch(Goal->goal_no)
			{
			case CTF_FLAG1:
				UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs("%s GOT the BLUE flag!", AP->GetPlayerName()) );

				UTIL_LogPrintf("\"%s<%i><%s>\" triggered \"Stole_Blue_Flag\"\n", 
					AP->GetPlayerName(),
					AP->GetUserID(),
					pTeamName );

				AP->m_Shared.AddItemFlags( IT_KEY1 );
				break;
			case CTF_FLAG2:
				UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs("%s GOT the RED flag!", AP->GetPlayerName()) );

				UTIL_LogPrintf("\"%s<%i><%s>\" triggered \"Stole_Red_Flag\"\n", 
					AP->GetPlayerName(),
					AP->GetUserID(),
					pTeamName );

				AP->m_Shared.AddItemFlags( IT_KEY2 );
				break;
			case CTF_DROPOFF1:
				UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs("%s CAPTURED the RED flag!", AP->GetPlayerName()) );

				UTIL_LogPrintf("\"%s<%i><%s>\" triggered \"Captured_Red_Flag\"\n", 
					AP->GetPlayerName(),
					AP->GetUserID(),
					pTeamName );

				AP->m_Shared.RemoveItemFlags( IT_KEY2 );
				break;
			case CTF_DROPOFF2:
				UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs("%s CAPTURED the BLUE flag!", AP->GetPlayerName()) );

				UTIL_LogPrintf("\"%s<%i><%s>\" triggered \"Captured_Blue_Flag\"\n", 
					AP->GetPlayerName(),
					AP->GetUserID(),
					pTeamName );

				AP->m_Shared.RemoveItemFlags( IT_KEY1 );
				break;
			default:
				break;
			}
		}
	}

	// Do Spawnpoint work before cycling players, so Forced respawn players work correctly.
	if (Goal->remove_spawngroup != 0)
	{
		// Find all goals
		//TFTODO: I think FindEntityByClassname will do the same thing.
		//CBaseEntity *pEnt = UTIL_FindEntityByString( NULL, "netname", "info_player_teamspawn" );
		CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
		while ( pEnt )
		{
			CTFSpawn *pSpawn = dynamic_cast<CTFSpawn*>( pEnt );
			if ( pSpawn )
			{
				if ( pSpawn->group_no == Goal->remove_spawngroup)
					InactivateSpawn(pSpawn);
			}

			pEnt = gEntList.FindEntityByClassname( pEnt, "info_player_teamspawn" );
		}
	}

	if (Goal->restore_spawngroup != 0)
	{
		// Find all goals
		//TFTODO: I think FindEntityByClassname will do the same thing.
		//CBaseEntity *pEnt = UTIL_FindEntityByString( NULL, "netname", "info_player_teamspawn" );
		CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
		while ( pEnt )
		{
			CTFSpawn *pSpawn = dynamic_cast<CTFSpawn*>( pEnt );
			if ( pSpawn )
			{
				if (pSpawn->group_no == Goal->restore_spawngroup)
					ActivateSpawn(pSpawn);
			}

			pEnt = gEntList.FindEntityByClassname( pEnt, "info_player_teamspawn" );
		}
	}

	// Go through all the players and do any results
	if ( Goal->broadcast != NULL_STRING && TFGameRules()->CTF_Map == FALSE )
	{
		UTIL_LogPrintf("World triggered \"%s\"\n", STRING(Goal->broadcast) );
	}
	if ( Goal->netname_broadcast != NULL_STRING && TFGameRules()->CTF_Map == FALSE && AP != NULL )
	{
		UTIL_LogPrintf("\"%s<%i><%s>\" triggered \"%s\"\n", 
				AP->GetPlayerName(),
				AP->GetUserID(),
				( AP->GetTeamNumber() != 0 ) ? GetTeamName( AP->GetTeamNumber() ) : "Spectator",
				STRING(Goal->netname_broadcast) );
	}
	
	BOOL bGotOne = FALSE;
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer )
			continue;

		// Centerprinting
		if (Goal->broadcast != NULL_STRING && TFGameRules()->CTF_Map == FALSE)
			UTIL_ShowMessage( STRING(Goal->broadcast), pPlayer );
		if (Goal->netname_broadcast != NULL_STRING && TFGameRules()->CTF_Map == FALSE && AP != NULL)
			ClientPrint( pPlayer, HUD_PRINTNOTIFY, STRING(Goal->netname_broadcast), AP->GetPlayerName() );
		// Old printing
		if (Goal->org_broadcast != NULL_STRING && TFGameRules()->CTF_Map == FALSE)
			ClientPrint( pPlayer, HUD_PRINTCENTER, STRING(Goal->org_broadcast) );

		// VOX
		if (Goal->speak != NULL_STRING)
			pPlayer->ClientHearVox( STRING(Goal->speak) );

		if (AP == pPlayer)
		{
			// Spawnpoints handle their own printing elsewhere
			if (Goal->message != NULL_STRING && Goal->Classify() != CLASS_TFSPAWN)
				UTIL_ShowMessage( STRING(Goal->message), pPlayer );
			if (Goal->org_message != NULL_STRING && Goal->Classify() != CLASS_TFSPAWN)
				ClientPrint( pPlayer, HUD_PRINTCENTER, STRING(Goal->org_message) );

			// VOX
			if (Goal->AP_speak != NULL_STRING)
				pPlayer->ClientHearVox( STRING(Goal->AP_speak) );
		}
		else if ( (AP != NULL) && (AP->GetTeamNumber() == pPlayer->GetTeamNumber()) )
		{
			// Text Printing
			if (Goal->owners_team_broadcast != NULL_STRING && pPlayer->GetTeamNumber() == Goal->owned_by)
				UTIL_ShowMessage( STRING(Goal->owners_team_broadcast), pPlayer );
			else if (Goal->non_owners_team_broadcast != NULL_STRING && pPlayer->GetTeamNumber() != Goal->owned_by)
				UTIL_ShowMessage( STRING(Goal->non_owners_team_broadcast), pPlayer );
			else if (Goal->team_broadcast!= NULL_STRING )
				UTIL_ShowMessage( STRING(Goal->team_broadcast), pPlayer );
			// Old Text Printing
			if (Goal->org_owners_team_broadcast != NULL_STRING && pPlayer->GetTeamNumber() == Goal->owned_by)
				ClientPrint( pPlayer, HUD_PRINTCENTER, STRING(Goal->org_owners_team_broadcast) );
			else if (Goal->org_non_owners_team_broadcast != NULL_STRING && pPlayer->GetTeamNumber() != Goal->owned_by)
				ClientPrint( pPlayer, HUD_PRINTCENTER, STRING(Goal->org_non_owners_team_broadcast) );
			else if (Goal->org_team_broadcast!= NULL_STRING )
				ClientPrint( pPlayer, HUD_PRINTCENTER, STRING(Goal->org_team_broadcast) );
				

			// VOX
			if (Goal->owners_team_speak != NULL_STRING && pPlayer->GetTeamNumber() == Goal->owned_by)
				pPlayer->ClientHearVox( STRING(Goal->owners_team_speak) );
			else if (Goal->non_owners_team_speak != NULL_STRING && pPlayer->GetTeamNumber() != Goal->owned_by)
				pPlayer->ClientHearVox( STRING(Goal->non_owners_team_speak) );
			else if (Goal->team_speak!= NULL_STRING )
				pPlayer->ClientHearVox( STRING(Goal->team_speak) );

			if (Goal->netname_owners_team_broadcast != NULL_STRING && pPlayer->GetTeamNumber() == Goal->owned_by)
				ClientPrint( pPlayer, HUD_PRINTNOTIFY, STRING(Goal->netname_owners_team_broadcast), AP->GetPlayerName() );
			else if (Goal->netname_team_broadcast!= NULL_STRING )
				ClientPrint( pPlayer, HUD_PRINTNOTIFY, STRING(Goal->netname_team_broadcast), AP->GetPlayerName() );
		}
		else
		{
			// Text Printing
			if (Goal->owners_team_broadcast != NULL_STRING && pPlayer->GetTeamNumber() == Goal->owned_by)
				UTIL_ShowMessage( STRING(Goal->owners_team_broadcast), pPlayer );
			else if (Goal->non_owners_team_broadcast != NULL_STRING && pPlayer->GetTeamNumber() != Goal->owned_by)
				UTIL_ShowMessage( STRING(Goal->non_owners_team_broadcast), pPlayer );
			else if (Goal->non_team_broadcast != NULL_STRING )
				UTIL_ShowMessage( STRING(Goal->non_team_broadcast), pPlayer );
			// Old Text Printing
			if (Goal->org_owners_team_broadcast != NULL_STRING && pPlayer->GetTeamNumber() == Goal->owned_by)
				ClientPrint( pPlayer, HUD_PRINTCENTER, STRING(Goal->org_owners_team_broadcast) );
			else if (Goal->org_non_owners_team_broadcast != NULL_STRING && pPlayer->GetTeamNumber() != Goal->owned_by)
				ClientPrint( pPlayer, HUD_PRINTCENTER, STRING(Goal->org_non_owners_team_broadcast) );
			else if (Goal->org_non_team_broadcast!= NULL_STRING )
				ClientPrint( pPlayer, HUD_PRINTCENTER, STRING(Goal->org_non_team_broadcast) );

			// VOX
			if (Goal->owners_team_speak != NULL_STRING && pPlayer->GetTeamNumber() == Goal->owned_by)
				pPlayer->ClientHearVox( STRING(Goal->owners_team_speak) );
			else if (Goal->non_owners_team_speak != NULL_STRING && pPlayer->GetTeamNumber() != Goal->owned_by)
				pPlayer->ClientHearVox( STRING(Goal->non_owners_team_speak) );
			else if (Goal->non_team_speak != NULL_STRING )
				pPlayer->ClientHearVox( STRING(Goal->non_team_speak) );

			if (Goal->netname_owners_team_broadcast != NULL_STRING && pPlayer->GetTeamNumber() == Goal->owned_by && AP != NULL)
				ClientPrint( pPlayer, HUD_PRINTNOTIFY, STRING(Goal->netname_owners_team_broadcast), AP->GetPlayerName() );
			else if (Goal->netname_non_team_broadcast != NULL_STRING && AP != NULL)
				ClientPrint( pPlayer, HUD_PRINTNOTIFY, STRING(Goal->netname_non_team_broadcast), AP->GetPlayerName() );
		}

		if (IsAffectedBy(Goal, pPlayer, AP))
		{
			// If its a Timer Goal, see if it needs to check Criteria again
			if (Goal->search_time != 0 && Goal->goal_effects & TFGE_TIMER_CHECK_AP)
			{
				if (APMeetsCriteria(Goal, pPlayer))
				{
					Apply_Results(Goal, (CTFPlayer*)pPlayer, AP, bAddBonuses);
					bGotOne = TRUE;
				}
			}
			else
			{
				Apply_Results(Goal, (CTFPlayer*)pPlayer, AP, bAddBonuses);
				bGotOne = TRUE;
			}
		}
	}

#ifdef MAP_DEBUG
	if (bGotOne == FALSE)
		Warning("NO PLAYERS AFFECTED\n");
#endif

	// Goal is now active
	// Items are not set to active. They handle their modes.
	if ( Goal->Classify() == CLASS_TFGOAL_TIMER || Goal->Classify() == CLASS_TFGOAL )
		Goal->goal_state = TFGS_ACTIVE;

	// EndGame checking 
	if (Goal->goal_result & TFGR_ENDGAME)
	{
		// Display Long TeamScores to everyone
//		TeamFortress_TeamShowScores(TRUE, NULL);
		TFTeamMgr()->ShowScores();

		if ( g_pGameRules->IsMultiplayer() )
			TFGameRules()->TFGoToIntermission();
		return;
	}

	// EndRound checking
	if (Goal->m_flEndRoundTime)
		EndRound( Goal );

	// Do Goal Group checking
	DoGroupWork(Goal, AP);

	// Do Goal checking
	DoGoalWork(Goal, AP);

	// Do Quake Trigger actions (Standard entities use SUB_UseTargets())
	if ( Goal->Classify() == CLASS_TFGOAL_TIMER || Goal->Classify() == CLASS_TFGOAL_ITEM || Goal->Classify() == CLASS_TFGOAL || Goal->Classify() == CLASS_TFSPAWN || Goal->do_triggerwork )
		DoTriggerWork(Goal, AP);

	// Setup for Respawn
	// Items, Triggers, and Spawnpoints do their own respawn work
	if ( Goal->Classify() == CLASS_TFGOAL || Goal->Classify() == CLASS_TFGOAL_TIMER )
		SetupRespawn(Goal);
}


//=========================================================================
// Check to see if the Goal should Activate. Handle Else Goals if not.
// If it does activate, Do the Results. Return true if the Goal activated.
bool ActivateDoResults(CTFGoal *Goal, CTFPlayer *AP, CTFGoal *ActivatingGoal)
{
	// Check Goal activation. This func handles Else Goals.
	if ( !ActivationSucceeded(Goal, AP, ActivatingGoal) )
		return false;

	// Do the Results.
	if (ActivatingGoal == Goal || Goal->m_bAddBonuses == true)
		DoResults(Goal, AP, true);
	else if (ActivatingGoal != NULL)
		DoResults(Goal, AP, (ActivatingGoal->goal_result & TFGR_ADD_BONUSES));
	else
		DoResults(Goal, AP, 0);

	return true;
}


//=========================================================================
// Return true if the Goal should activate, and handle Else Goals
bool ActivationSucceeded(CTFGoal *Goal, CTFPlayer *AP, CTFGoal *ActivatingGoal)
{
	// Can't activate during PreMatch time, except for timers
	if ( (TFGameRules()->IsInPreMatch()) && (Goal->Classify() != CLASS_TFGOAL_TIMER) )
		return false;

	// If activation fails, try and activate the Else Goal
	if ( !ShouldActivate(Goal, AP) )
	{
		// If an else goal should be activated, activate it
		if (Goal->else_goal != 0)
		{
			MDEBUG( Warning("   Else Goal.\n") );

			CTFGoal *pElseGoal = Findgoal(Goal->else_goal);
			if (pElseGoal)
				ActivateDoResults(pElseGoal, AP, Goal);
		}

		return false;
	}

	return true;
}


// ---------------------------------------------------------------------------------------- //
// CTFBaseItem existence.
// ---------------------------------------------------------------------------------------- //

//===========================================
// Check whether this entity should exist at this skill
bool CTFBaseItem::CheckExistence()
{
	if (ex_skill_min == -1 && g_iSkillLevel < 0)
		return FALSE;
	else if (ex_skill_max == -1 && g_iSkillLevel > 0)
		return FALSE;
	
	if ( (ex_skill_min != 0) && (ex_skill_min != -1) && (g_iSkillLevel < ex_skill_min) )
		return FALSE;
	else if ( (ex_skill_max != 0) && (ex_skill_max != -1) && (g_iSkillLevel > ex_skill_max) )
		return FALSE;

	return TRUE;
}


// ---------------------------------------------------------------------------------------- //
// CTFGoal implementation.
// ---------------------------------------------------------------------------------------- //

LINK_ENTITY_TO_CLASS(info_tfgoal, CTFGoal);

BEGIN_DATADESC( CTFGoal )
	DEFINE_FUNCTION( PlaceGoal ),
	DEFINE_FUNCTION( DelayedResult )
END_DATADESC()



//===========================================
// TF Goal spawn
void CTFGoal::Spawn( void )
{
	if (CheckExistence() == false)
	{
		UTIL_Remove(this);
		return;
	}

	// Graphic
	string_t modelName = GetModelName();
	if ( modelName != NULL_STRING )
	{
		// Brush Models need to be invisible
		const char *pModel = STRING( modelName );
		if (pModel[0] == '*')
			AddEffects( EF_NODRAW );
	}

#ifdef TFTODO
	// Activation sound
	if (pev->noise)
		PRECACHE_SOUND( (char*)STRING(pev->noise) );

	// For the powerups
	PRECACHE_SOUND("items/protect.wav");
	PRECACHE_SOUND("items/protect2.wav");
	PRECACHE_SOUND("items/protect3.wav");
	PRECACHE_SOUND("FVox/HEV_logon.wav");
	PRECACHE_SOUND("FVox/hev_shutdown.wav");
	PRECACHE_SOUND("items/inv1.wav");
	PRECACHE_SOUND("items/inv2.wav");
	PRECACHE_SOUND("items/inv3.wav");
	PRECACHE_SOUND("items/damage.wav");
	PRECACHE_SOUND("items/damage2.wav");
	PRECACHE_SOUND("items/damage3.wav");
#endif

	// Set initial states
	AddSolidFlags( FSOLID_TRIGGER );
	if (goal_state == 0)
		goal_state = TFGS_INACTIVE;

	// Set Size
	if (goal_min != vec3_origin && goal_max != vec3_origin)
		UTIL_SetSize( this, goal_min, goal_max );

	StartGoal();
}


//=========================================================================
// Respawn the goal
void CTFGoal::DoRespawn()
{
	RestoreGoal(this);
	InactivateGoal(this);
}


//=========================================================================
// Timer goal tick
void CTFGoal::tfgoal_timer_tick()
{
	// Check criteria
	if (goal_state != TFGS_REMOVED)
	{
	#ifdef MAP_DEBUG
		Warning("==========================\n");
		Warning("Timer Tick for: %s\nChecking Criteria...", GetEntityName().ToCStr());
	#endif

		// Timers don't fire during prematch.
		// Instead, they setup to fire the correct amount of time past the prematch
		if ( TFGameRules()->IsInPreMatch() )
		{
			MDEBUG( Warning("\n PREMATCH IS ON. DELAYING UNTIL AFTER PREMATCH.\n") );

			SetThink( &CTFGoal::tfgoal_timer_tick );
			SetNextThink( TFGameRules()->GetPreMatchEndTime() + search_time );
			return;
		}

		if (APMeetsCriteria(this, NULL))
		{
			DoResults(this, NULL, TRUE);
		}
		else
		{
			MDEBUG( Warning("\n") );
			SetThink( &CTFGoal::tfgoal_timer_tick );
			SetNextThink( gpGlobals->curtime + search_time );
		}
	}
}


//=========================================================================
// Use (Triggered) function for Goals
void CTFGoal::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// If the Activator isn't a player, pretend there isn't an activator
	if (pActivator && !pActivator->IsPlayer())
	{
		pActivator = NULL;
	}

	CTFGoal *pCallerGoal = dynamic_cast<CTFGoal*>( pCaller );
	if ( !pCallerGoal )
	{
		// Need to rethink some stuff if this can be called by entities that aren't CTFGoals.
		Assert( false );
		return;
	}

	// Goals are only activatable by players
	if (!pActivator || pActivator->IsPlayer())
	{
		// Force it to add bonuses
		m_bAddBonuses = true;
		ActivateDoResults(this, (CTFPlayer*)pActivator, pCallerGoal);
	}
}


//===========================================
// Make Goal's more easy to touch
void CTFGoal::SetObjectCollisionBox( void )
{
	const char *pModel = STRING( GetModelName() );
	if (pModel[0] != '*')
	{
		Vector vMins = WorldAlignMins() + Vector(-24, -24, 0);
		Vector vMaxs = WorldAlignMaxs() + Vector(24, 24, 16);
		SetCollisionBounds( vMins, vMaxs );
	}
	else
	{
// Do we even need to do this? The bmodel should be setup correctly at this point anyway.
#ifdef TFTODO		
		// Ripped from ::SetObjectCollisionBox
		float		max, v;
		int			i;

		max = 0;
		for (i=0 ; i<3 ; i++)
		{
			v = fabs( (( float * )pev->mins )[i]);
			if (v > max)
				max = v;
			v = fabs( (( float * )pev->maxs )[i]);
			if (v > max)
				max = v;
		}
		for (i=0 ; i<3 ; i++)
		{
			((float *)pev->absmin)[i] = (( float * )GetAbsOrigin())[i] - max;
			((float *)pev->absmax)[i] = (( float * )GetAbsOrigin())[i] + max;
		}

		pev->absmin.x -= 1;
		pev->absmin.y -= 1;
		pev->absmin.z -= 1;
		pev->absmax.x += 1;
		pev->absmax.y += 1;
		pev->absmax.z += 1;
#endif
	}
}

//===========================================
// Start the Goal 
void CTFGoal::StartGoal( void )
{
	m_bAddBonuses = false;
	SetThink( &CTFGoal::PlaceGoal );
	SetNextThink( gpGlobals->curtime + 0.2 );	// goals start after other solids

	if (goal_state == TFGS_REMOVED)
		RemoveGoal(this);
};

//===========================================
// Sets up the Goal's first thoughts
void CTFGoal::PlaceGoal( void )
{
	if ( FClassnameIs(this, "info_tfgoal_timer") )
	{
		// Set up the next Timer Tick
		SetThink( &CTFGoal::tfgoal_timer_tick );
		SetNextThink( gpGlobals->curtime + search_time );
	}
	else
	{
		// Only give touch functions to goals that can be activated by touch
		if (goal_activation & TFGA_TOUCH)
			SetTouch( &CTFGoal::tfgoal_touch );
	}

	// So searches for this goal work later on
	Assert( stricmp( GetClassname(), "info_tfgoal" ) == 0 );

	// Drop to ground
 	if (goal_activation & TFGA_DROPTOGROUND)
	{
// Is this right?
#ifdef TFTODO
		SetMoveType( MOVETYPE_TOSS );
#else		
		SetMoveType( MOVETYPE_FLYGRAVITY );
#endif
		SetAbsOrigin( GetAbsOrigin() + Vector( 0, 0, 6 ) );

		if ( UTIL_DropToFloor(this, MASK_SOLID) == 0)
		{
			Error("TF Goal %s fell out of level at %f,%f,%f", GetEntityName(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z);
			UTIL_Remove( this );
			return;
		}
	}

	SetMoveType( MOVETYPE_NONE );
	SetAbsVelocity( Vector( 0, 0, 0 ) );
	oldorigin = GetAbsOrigin(); 		// So we can return it later
}


// Touch function for Goals
void CTFGoal::tfgoal_touch( CBaseEntity *pOther )
{
	// Can't touch during PreMatch time
	if ( TFGameRules()->IsInPreMatch() )
		return;
	// If it is not activated in by the player's touch, return
	if (!(goal_activation & TFGA_TOUCH))
		return;
	// Only activatable by a player
	if (!pOther->IsPlayer())
		return;
	if ( pOther->IsAlive() == FALSE )
		return;

	// If it's already active, don't bother
	if (goal_state == TFGS_ACTIVE)
	{
		MDEBUG( Warning("Goal already active. aborting touch.\n") );
		return;
	}

	// CTF Hack to make sure the key is in place. Like the rest of the CTF_Map stuff,
	// it's not needed... the base scripting could handle it all.
	if (TFGameRules()->CTF_Map)
	{
		if ((goal_no == CTF_DROPOFF1) && (pOther->GetTeamNumber() == 1))
		{
			CTFGoalItem *pFlag = Finditem(CTF_FLAG1);
			if ((pFlag->goal_state == TFGS_ACTIVE) || (pFlag->GetAbsOrigin() != pFlag->oldorigin))
				return;
		}
		if ((goal_no == CTF_DROPOFF2) && (pOther->GetTeamNumber() == 2))
		{
			CTFGoalItem *pFlag = Finditem(CTF_FLAG2);
			if ((pFlag->goal_state == TFGS_ACTIVE) || (pFlag->GetAbsOrigin() != pFlag->oldorigin))
				return;
		}
	}

	ActivateDoResults(this, ToTFPlayer( pOther ), this);
}

//=========================================================================
// Handles Delayed Activation of Goals
void CTFGoal::DelayedResult()
{
	CBaseEntity *pEnemy = enemy;
	if (goal_state == TFGS_DELAYED)
		DoResults(this, ToTFPlayer( pEnemy ), weapon);
}


// ---------------------------------------------------------------------------------------- //
// CTFGoalItem implementation.
// ---------------------------------------------------------------------------------------- //

LINK_ENTITY_TO_CLASS(item_tfgoal, CTFGoalItem);


//=========================================================================
// Spawn a goalitem entity
void CTFGoalItem::Spawn( void )
{
	if (CheckExistence() == false)
	{
		UTIL_Remove(this);
		return;
	}

	// Set this in case they used abbreviations
	Assert( stricmp( GetClassname(), "item_tfgoal" ) == 0 );

	// Graphic
	if ( GetModelName() != NULL_STRING )
	{
		// Setup animations
		SetSequence( LookupSequence( "not_carried" ) );
		if ( GetSequence() != -1 )
		{
			ResetSequenceInfo();
			SetCycle( 0.0f );
		}
	}

#ifdef TFTODO
	// Respawn sound
	PRECACHE_SOUND("items/itembk2.wav");

	// Activation sound
	if (pev->noise)
		PRECACHE_SOUND( (char*)STRING(pev->noise) );

	if (!(pev->netname))
		pev->netname = MAKE_STRING("goalitem");
#endif

	if (goal_state == 0)
		goal_state = TFGS_INACTIVE;

	// Set initial solidity
	if (goal_activation & TFGI_SOLID)
	{
		SetSolid( SOLID_BBOX );
		// Solid goalitems need a bbox 
		if (goal_min == vec3_origin)
			goal_min = Vector(-16, -16, -24);
		if (goal_max == vec3_origin)
			goal_max = Vector(16, 16, 32);
	}
	else
	{
		SetSolidFlags( FSOLID_TRIGGER );
	}

	if (drop_time <= 0)
		drop_time = 60;

	// Set Size
	UTIL_SetSize(this, goal_min, goal_max);

	SetTouch( &CTFGoalItem::item_tfgoal_touch );
	StartItem();
};

//=========================================================================
// Start the Goal Item
void CTFGoalItem::StartItem( void )
{
	SetThink( &CTFGoalItem::PlaceItem );
	SetNextThink( gpGlobals->curtime + 0.2 );	// items start after other solids

	if (goal_state == TFGS_REMOVED)
		RemoveGoal(this);
}

//===========================================
// Place the Goal Item
void CTFGoalItem::PlaceItem( void )
{
	static int item_list_bit = 1;	// used to determine what the bit of each new GoalItem will be.

	SetAbsVelocity( vec3_origin );

	// Drop to ground
 	if (goal_activation & TFGA_DROPTOGROUND)
	{
#ifdef TFTODO
		pev->movetype = MOVETYPE_TOSS;
#else
		SetMoveType( MOVETYPE_FLYGRAVITY );
#endif
		SetAbsOrigin( GetAbsOrigin() + Vector( 0, 0, 6 ) );

		if ( UTIL_DropToFloor( this, MASK_SOLID ) == 0)
		{
			Error("TF GoalItem %s fell out of level at %f,%f,%f", STRING( GetEntityName() ), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z);
			UTIL_Remove( this );
			return;
		}
	}

	SetMoveType( MOVETYPE_NONE );
	oldorigin = GetAbsOrigin(); 		// So we can return it later

 	if (goal_activation & TFGI_ITEMGLOWS)
	{
		m_nRenderFX = kRenderFxGlowShell;

		// If we have a teamcheck entity, use it instead
		if ( owned_by_teamcheck != NULL_STRING )
			owned_by = GetTeamCheckTeam( STRING(owned_by_teamcheck) );

//		if (owned_by > 0 && owned_by <= 4)
//			m_clrRender = Vector255ToRGBColor( rgbcolors[owned_by] );
//		else
//			m_clrRender = Vector255ToRGBColor( rgbcolors[0] );

		if ( TFTeamMgr()->IsValidTeam( owned_by ) )
		{
			CTFTeam *pTeam = TFTeamMgr()->GetTeam( owned_by );
			m_clrRender = pTeam->GetColor();
		}
		else
		{
			m_clrRender = TFTeamMgr()->GetUndefinedTeamColor();
		}

		SetRenderColorA( 100 );	// Shell size
	}

	// Set the item bit
	item_list = item_list_bit;
	item_list_bit *= 2;
}


// Touch function for the goalitem entity
void CTFGoalItem::item_tfgoal_touch( CBaseEntity *pOther )
{
	if (!pOther->IsPlayer())
		return;
	if ( pOther->IsAlive() == FALSE )
		return;
	// Can't touch during PreMatch time
	if ( TFGameRules()->IsInPreMatch() )
		return;

	// Hack to prevent feigning spies from repicking up flags
	CTFPlayer *pPlayer = dynamic_cast<CTFPlayer*>( pOther );
	CTFPlayerClass *pPlayerClass = pPlayer->GetPlayerClass();
	if ( pPlayerClass && pPlayerClass->IsClass( TF_CLASS_SPY ) && pPlayerClass->Spy_IsFeigning() )
		return;

	// Prevent the dropping player from picking it up for longer
	if (enemy.Get() && m_flDroppedAt != 0)
	{
		if ( (enemy == pOther) && m_flDroppedAt + 5 > gpGlobals->curtime )
			return;
	}
	m_flDroppedAt = 0;

	ASSERT( pOther != GetOwnerEntity() );	// There is no way this should ever happen, and yet it still does.

	// Prevent picking up flags through thin walls
	trace_t tr;
	UTIL_TraceLine ( WorldSpaceCenter(), pOther->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0 && tr.m_pEnt != pOther )
		return;

	// CTF Hack to return your key.
	// Should use the builtin support now.
	if (TFGameRules()->CTF_Map == TRUE)
	{
		// Flag not at home?
		if (GetAbsOrigin() != oldorigin)
		{
			if (GetTeamNumber() == 1)
			{
				if (goal_no == CTF_FLAG1)
				{
					UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs("%s RETURNED the BLUE flag!", pPlayer->GetPlayerName()) );
					UTIL_LogPrintf("\"%s<%i><%s>\" triggered \"Returned_Blue_Flag\"\n", 
						pPlayer->GetPlayerName(),
						pPlayer->GetUserID(),
						GetTeamName( pOther->GetTeamNumber() ) );
				}
				else
				{
					UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs("%s RETURNED the RED flag!", pPlayer->GetPlayerName()) );
					UTIL_LogPrintf("\"%s<%i><%s>\" triggered \"Returned_Red_Flag\"\n", 
						pPlayer->GetPlayerName(),
						pPlayer->GetUserID(),
						GetTeamName( pOther->GetTeamNumber() ) );
				}

				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
					if ( !pPlayer )
						continue;

					if (pPlayer->GetTeamNumber() == 1 && goal_no == CTF_FLAG1)
						ClientPrint( pPlayer, HUD_PRINTCENTER, "Your flag was RETURNED!!\n");
					else if (pPlayer->GetTeamNumber() != 1 && goal_no == CTF_FLAG1)
						ClientPrint( pPlayer, HUD_PRINTCENTER, "The ENEMY flag was RETURNED!!\n");
					else if (pPlayer->GetTeamNumber() == 2 && goal_no == CTF_FLAG2)
						ClientPrint( pPlayer, HUD_PRINTCENTER, "Your flag was RETURNED!!\n");
					else if (pPlayer->GetTeamNumber() == 2 && goal_no == CTF_FLAG1)
						ClientPrint( pPlayer, HUD_PRINTCENTER, "The ENEMY flag was RETURNED!!\n");
				}

				goal_state = TFGS_INACTIVE;
				SetSolidFlags( FSOLID_TRIGGER );
				SetTouch( &CTFGoalItem::item_tfgoal_touch );
				SetAbsOrigin( oldorigin );
				
				EmitSound( "GoalItem.Touch" );
				//EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "items/itembk2.wav", 1, ATTN_NORM, 0, 150 );
				return;
			}
		}
		else
		{
			// Ignore touches to our own flag when it's at home
			if (pOther->GetTeamNumber() == 1 && goal_no == CTF_FLAG1)
				return;
			if (pOther->GetTeamNumber() == 2 && goal_no == CTF_FLAG2)
				return;
		}
	}

	// Activate. Handles Else Goals if it fails.
	if ( ActivationSucceeded( this, ToTFPlayer( pOther ), NULL) )
	{
		// Give it to the player
		tfgoalitem_GiveToPlayer(this, ToTFPlayer( pOther ), this);
		// It may have killed the player, so check:
		if (pOther->GetHealth() > 0)
			goal_state = TFGS_ACTIVE;
	}
}


//=========================================================================
// Throw the item into the air at the specified position
void CTFGoalItem::DoDrop( Vector vecOrigin )
{
	SetAbsOrigin( vecOrigin );

	StopFollowingEntity();
	SetMoveType( MOVETYPE_FLYGRAVITY );

	// Just drop it vertically first time to prevent it falling through walls too often
	Vector vVel = GetAbsVelocity();
	vVel.z = 400;
	if (redrop_count > 1)
	{
		// Second and third drops try pushing it in other directions
		vVel.x = RandomFloat(-50, 50);
		vVel.y = RandomFloat(-50, 50);
	}
	SetAbsVelocity( vVel );
	
	goal_state = TFGS_INACTIVE;
	SetAbsAngles( QAngle( 0, 0, 0 ) );
	if (goal_activation & TFGI_SOLID)
	{
		SetSolid( SOLID_BBOX );
	}
	else
	{
		SetSolid( SOLID_BBOX );
		SetSolidFlags( FSOLID_TRIGGER );
	}

	RemoveEffects( EF_NODRAW );

	UTIL_SetSize(this, goal_min, goal_max);

	redrop_count++;

	SetThink( &CTFGoalItem::tfgoalitem_dropthink ); 	    // give it five seconds
	SetNextThink( gpGlobals->curtime + 5.0 );	// and then find where it ended up
}


//=========================================================================
// Set the GoalItems touch func
void CTFGoalItem::tfgoalitem_droptouch()
{
	SetTouch( &CTFGoalItem::item_tfgoal_touch );
	SetThink( &CTFGoalItem::tfgoalitem_dropthink ); 	     // give it five seconds since it was dropped
	SetNextThink( gpGlobals->curtime + 4.25 ); // and then find where it ended up
}


//=========================================================================
// A quick check to make sure the items is not in a wall
void CTFGoalItem::tfgoalitem_dropthink()
{
	MDEBUG( Msg( "DropThink for %s\n", STRING(pev->netname)) );

	StopFollowingEntity();
	SetMoveType( MOVETYPE_FLYGRAVITY );
	
	if (drop_time != 0)
	{
		int iEnviron = UTIL_PointContents( GetAbsOrigin() );
	
		if (iEnviron == CONTENTS_SLIME)
		{
			SetNextThink( gpGlobals->curtime + (drop_time / 4) );
		}
#ifdef TFTODO // CONTENTS_LAVA and CONTENTS_SKY don't exist in src.
		else if (iEnviron == CONTENTS_LAVA)
		{
			SetNextThink( gpGlobals->curtime + 5 );
		}
		else if (iEnviron == CONTENTS_SOLID || iEnviron == CONTENTS_SKY)
#else
		else if (iEnviron == CONTENTS_SOLID)
#endif
		{
			// Its out of the world
			// Retry a drop from the original position 3 times
			if (redrop_count < 3)
			{
				// Retry the Drop
				DoDrop( redrop_origin );
				return;
			}
			else
			{
				// Fourth time round, just return it
				SetNextThink( gpGlobals->curtime + 2 );
			}
		}
		else
		{
			SetNextThink( gpGlobals->curtime + drop_time );
		}
		
		SetThink( &CTFGoalItem::tfgoalitem_remove );
	}
}


//=========================================================================
// Remove the item, or Return it if needed
void CTFGoalItem::tfgoalitem_remove()
{
	MDEBUG( Msg( "RemoveItem for %s...", STRING(pev->netname)) );

	// Has someone picked it up?
	if (goal_state == TFGS_ACTIVE)
	{
		MDEBUG( Msg( "Item picked up, exiting.\n") );
		return;
	}

	// Should it be returned?
	if (goal_activation & TFGI_RETURN_REMOVE)
	{
		MDEBUG( Msg( "Returned.\n") );

		CTimer *pTimer = Timer_CreateTimer( this, TF_TIMER_RETURNITEM );
		pTimer->weapon = GI_RET_TIME;
		pTimer->m_flNextThink = gpGlobals->curtime + 0.1;
		//pTimer->SetThink( &CBaseEntity::ReturnItem ); this is done by CreateTimer code now.
		return;
	}

	MDEBUG( Msg( "Removed.\n") );
	UTIL_Remove(this);
}

#if 0
// ---------------------------------------------------------------------------------------------------- //
// CTFSpawn implementation.
// ---------------------------------------------------------------------------------------------------- //

LINK_ENTITY_TO_CLASS(info_player_teamspawn_old, CTFSpawn);

//===========================================
void CTFSpawn::Spawn( void )
{
	if (CheckExistence() == FALSE)
	{
		UTIL_Remove(this);
		return;
	}

	// Team spawnpoints must have a team associated with them
	if ( (GetTeamNumber() <= 0 || GetTeamNumber() >= 5) && teamcheck == NULL_STRING)
	{
		Warning("Teamspawnpoint with an invalid GetTeamNumber() of %d\n", GetTeamNumber());
		return;
	}

	// Save out the info_player_teamspawn
	Assert( stricmp( GetClassname(), "info_player_teamspawn" ) == 0 );
#ifdef TFTODO
	pev->netname = pev->classname = MAKE_STRING(  );
#endif
}

void CTFSpawn::Activate( void )
{
	m_pTeamCheck = NULL;

	// Find the team check entity
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, STRING(teamcheck) );

	if ( pTarget )
	{
		if ( !strcmp( pTarget->GetClassname(), "info_tf_teamcheck" ) )
			m_pTeamCheck = pTarget;
	}		
}

BOOL CTFSpawn::CheckTeam( int iTeamNo )
{
	// First check team number
	if ( GetTeamNumber() )
	{
		return ( iTeamNo == GetTeamNumber() );
	}

	// Then check the teamcheck
	if ( m_pTeamCheck )
	{
		CTeamCheck *pTeamCheck = dynamic_cast<CTeamCheck*>( m_pTeamCheck.Get() );
		Assert( pTeamCheck );
		if ( pTeamCheck->TeamMatches( iTeamNo ) == FALSE )
			return FALSE;

		return TRUE;
	}

	return FALSE;
}
#endif

// ---------------------------------------------------------------------------------------------------- //
// CBaseDelay implementation.
// ---------------------------------------------------------------------------------------------------- //

LINK_ENTITY_TO_CLASS( DelayedUse, CBaseDelay );

void CBaseDelay::SUB_UseTargets( CBaseEntity *pActivator, USE_TYPE useType, float value )
{
	// TeamFortress Goal Checking
	if (!pActivator || pActivator->IsPlayer() )
		DoResults(this, ToTFPlayer( pActivator ), TRUE);

	//
	// exit immediatly if we don't have a target or kill target
	//
	if (target == NULL_STRING && !m_iszKillTarget)
		return;

	//
	// check for a delay
	//
	if (m_flDelay != 0)
	{
		// create a temp object to fire at a later time
		CBaseDelay *pTemp = (CBaseDelay*)CreateEntityByName( "DelayedUse" );
		Assert( stricmp( pTemp->GetClassname(), "DelayedUse" ) == 0 );

		pTemp->SetNextThink( gpGlobals->curtime + m_flDelay );
		pTemp->SetThink( &CBaseDelay::DelayThink );
		
		// Save the useType
		pTemp->button = (int)useType;
		pTemp->m_iszKillTarget = m_iszKillTarget;
		pTemp->m_flDelay = 0; // prevent "recursion"
		pTemp->target = target;
		return;
	}

	//
	// kill the killtargets
	//

	if ( m_iszKillTarget != NULL_STRING )
	{
		CBaseEntity *pentKillTarget = NULL;

		Msg( "KillTarget: %s\n", STRING(m_iszKillTarget) );
		pentKillTarget = gEntList.FindEntityByName( NULL, STRING(m_iszKillTarget) );
		while ( pentKillTarget )
		{
			Msg( "killing %s\n", pentKillTarget->GetClassname() );
			
			CBaseEntity *pNext = gEntList.FindEntityByName( pentKillTarget, STRING(m_iszKillTarget) );
			UTIL_Remove( pentKillTarget );
			pentKillTarget = pNext;
		}
	}
	
	//
	// fire targets
	//
	if ( target != NULL_STRING )
	{
		FireTargets( STRING(target), pActivator, this, useType, value );
	}
}


bool CBaseDelay::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "delay"))
	{
		m_flDelay = atof( szValue );
		return true;
	}
	else if (FStrEq(szKeyName, "killtarget"))
	{
		m_iszKillTarget = MAKE_STRING(szValue);
		return true;
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}
}

void CBaseDelay::DelayThink( void )
{
	// The use type is cached (and stashed) in pev->button
	SUB_UseTargets( NULL, (USE_TYPE)button, 0 );
	UTIL_Remove( this );
}


// ---------------------------------------------------------------------------------------------------- //
// CTeamCheck implementation.
// ---------------------------------------------------------------------------------------------------- //

LINK_ENTITY_TO_CLASS(info_tf_teamcheck, CTeamCheck );


void CTeamCheck::Spawn( void )
{
}

void CTeamCheck::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Either Toggle or get set to a specific team number
	if ( useType == USE_TOGGLE )
	{
		if ( GetTeamNumber() == 1 )
			ChangeTeam( 2 );
		else 
			ChangeTeam( 1 );
	}
	else if ( useType == USE_SET )
	{
		if ( value >= 1 && value <= 4 )
			ChangeTeam( value );
	}
}

BOOL CTeamCheck::TeamMatches( int iTeam )
{
	return ( iTeam == GetTeamNumber() );
}

#endif
