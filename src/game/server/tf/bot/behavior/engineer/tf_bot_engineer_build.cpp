//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_engineer_build.cpp
// Engineer building his buildings
// Michael Booth, February 2009

#include "cbase.h"
#include "nav_mesh.h"
#include "tf_player.h"
#include "tf_obj.h"
#include "tf_obj_sentrygun.h"
#include "tf_obj_dispenser.h"
#include "tf_gamerules.h"
#include "tf_weapon_builder.h"
#include "bot/tf_bot.h"
#include "bot/behavior/engineer/tf_bot_engineer_build.h"
#include "bot/behavior/engineer/tf_bot_engineer_build_teleport_entrance.h"
#include "bot/behavior/engineer/tf_bot_engineer_move_to_build.h"


#include "raid/tf_raid_logic.h"

// this was useful when engineers build at their normal (slow) rate to make sure initial sentries get built in time
ConVar tf_raid_engineer_infinte_metal( "tf_raid_engineer_infinte_metal", "1", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );


//---------------------------------------------------------------------------------------------
Action< CTFBot > *CTFBotEngineerBuild::InitialContainedAction( CTFBot *me )
{
	if ( TFGameRules()->IsPVEModeActive() )
	{
		return new CTFBotEngineerMoveToBuild;
	}

	return new CTFBotEngineerBuildTeleportEntrance;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEngineerBuild::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEngineerBuild::Update( CTFBot *me, float interval )
{
	if ( TFGameRules()->IsPVEModeActive() && tf_raid_engineer_infinte_metal.GetBool() )
	{
		// infinite ammo
		me->GiveAmmo( 1000, TF_AMMO_METAL, true );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotEngineerBuild::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotEngineerBuild::OnTerritoryLost( CTFBot *me, int territoryID )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
// Hack to disable ammo/health gathering elsewhere
QueryResultType CTFBotEngineerBuild::ShouldHurry( const INextBot *meBot ) const
{
	CTFBot *me = (CTFBot *)meBot->GetEntity();

	CObjectSentrygun *mySentry = (CObjectSentrygun *)me->GetObjectOfType( OBJ_SENTRYGUN );
	CObjectDispenser *myDispenser = (CObjectDispenser *)me->GetObjectOfType( OBJ_DISPENSER );

	if ( mySentry && myDispenser && !mySentry->IsBuilding() && !myDispenser->IsBuilding() && me->GetActiveTFWeapon() && me->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_WRENCH )
	{
		if ( me->IsAmmoLow() && myDispenser->GetAvailableMetal() <= 0 )
		{
			// we're totally out of metal - collect some nearby
			return ANSWER_NO;
		}

		// by being in a "hurry" we wont collect health and ammo
		return ANSWER_YES;
	}

	return ANSWER_UNDEFINED;
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotEngineerBuild::ShouldAttack( const INextBot *meBot, const CKnownEntity *them ) const
{
	CTFBot *me = (CTFBot *)meBot->GetEntity();
	CObjectSentrygun *mySentry = (CObjectSentrygun *)me->GetObjectOfType( OBJ_SENTRYGUN );

	CTFPlayer *themPlayer = ToTFPlayer( them->GetEntity() );

	if ( themPlayer && themPlayer->IsPlayerClass( TF_CLASS_SPY ) )
	{
		// Engineers hate Spies
		return ANSWER_YES;
	}

	if ( mySentry && me->IsRangeLessThan( mySentry, 100.0f ) )
	{
		// focus on keeping our sentry alive
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}
