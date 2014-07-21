//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "usercmd.h"
#include "igamemovement.h"
#include "mathlib/mathlib.h"
#include "client.h"
#include "player_command.h"
#include "movehelper_server.h"
#include "iservervehicle.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IGameMovement *g_pGameMovement;
extern CMoveData *g_pMoveData;	// This is a global because it is subclassed by each game.
extern ConVar sv_noclipduringpause;

ConVar sv_maxusrcmdprocessticks_warning( "sv_maxusrcmdprocessticks_warning", "-1", FCVAR_NONE, "Print a warning when user commands get dropped due to insufficient usrcmd ticks allocated, number of seconds to throttle, negative disabled" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPlayerMove::CPlayerMove( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: We're about to run this usercmd for the specified player.  We can set up groupinfo and masking here, etc.
//  This is the time to examine the usercmd for anything extra.  This call happens even if think does not.
// Input  : *player - 
//			*cmd - 
//-----------------------------------------------------------------------------
void CPlayerMove::StartCommand( CBasePlayer *player, CUserCmd *cmd )
{
	VPROF( "CPlayerMove::StartCommand" );

#if !defined( NO_ENTITY_PREDICTION )
	CPredictableId::ResetInstanceCounters();
#endif

	player->m_pCurrentCommand = cmd;
	CBaseEntity::SetPredictionRandomSeed( cmd );
	CBaseEntity::SetPredictionPlayer( player );
	
#if defined (HL2_DLL)
	// pull out backchannel data and move this out

	int i;
	for (i = 0; i < cmd->entitygroundcontact.Count(); i++)
	{
		int entindex =  cmd->entitygroundcontact[i].entindex;
		CBaseEntity *pEntity = CBaseEntity::Instance( engine->PEntityOfEntIndex( entindex) );
		if (pEntity)
		{
			CBaseAnimating *pAnimating = pEntity->GetBaseAnimating();
			if (pAnimating)
			{
				pAnimating->SetIKGroundContactInfo( cmd->entitygroundcontact[i].minheight, cmd->entitygroundcontact[i].maxheight );
			}
		}
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose: We've finished running a user's command
// Input  : *player - 
//-----------------------------------------------------------------------------
void CPlayerMove::FinishCommand( CBasePlayer *player )
{
	VPROF( "CPlayerMove::FinishCommand" );

	player->m_pCurrentCommand = NULL;
	CBaseEntity::SetPredictionRandomSeed( NULL );
	CBaseEntity::SetPredictionPlayer( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Checks if the player is standing on a moving entity and adjusts velocity and 
//  basevelocity appropriately
// Input  : *player - 
//			frametime - 
//-----------------------------------------------------------------------------
void CPlayerMove::CheckMovingGround( CBasePlayer *player, double frametime )
{
	VPROF( "CPlayerMove::CheckMovingGround()" );

	CBaseEntity	    *groundentity;

	if ( player->GetFlags() & FL_ONGROUND )
	{
		groundentity = player->GetGroundEntity();
		if ( groundentity && ( groundentity->GetFlags() & FL_CONVEYOR) )
		{
			Vector vecNewVelocity;
			groundentity->GetGroundVelocityToApply( vecNewVelocity );
			if ( player->GetFlags() & FL_BASEVELOCITY )
			{
				vecNewVelocity += player->GetBaseVelocity();
			}
			player->SetBaseVelocity( vecNewVelocity );
			player->AddFlag( FL_BASEVELOCITY );
		}
	}

	if ( !( player->GetFlags() & FL_BASEVELOCITY ) )
	{
		// Apply momentum (add in half of the previous frame of velocity first)
		player->ApplyAbsVelocityImpulse( (1.0 + ( frametime * 0.5 )) * player->GetBaseVelocity() );
		player->SetBaseVelocity( vec3_origin );
	}

	player->RemoveFlag( FL_BASEVELOCITY );
}

//-----------------------------------------------------------------------------
// Purpose: Prepares for running movement
// Input  : *player - 
//			*ucmd - 
//			*pHelper - 
//			*move - 
//			time - 
//-----------------------------------------------------------------------------
void CPlayerMove::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	VPROF( "CPlayerMove::SetupMove" );

	// Allow sound, etc. to be created by movement code
	move->m_bFirstRunOfFunctions = true;
	move->m_bGameCodeMovedPlayer = false;
	if ( player->GetPreviouslyPredictedOrigin() != player->GetAbsOrigin() )
	{
		move->m_bGameCodeMovedPlayer = true;
	}

	// Prepare the usercmd fields
	move->m_nImpulseCommand		= ucmd->impulse;	
	move->m_vecViewAngles		= ucmd->viewangles;

	CBaseEntity *pMoveParent = player->GetMoveParent();
	if (!pMoveParent)
	{
		move->m_vecAbsViewAngles = move->m_vecViewAngles;
	}
	else
	{
		matrix3x4_t viewToParent, viewToWorld;
		AngleMatrix( move->m_vecViewAngles, viewToParent );
		ConcatTransforms( pMoveParent->EntityToWorldTransform(), viewToParent, viewToWorld );
		MatrixAngles( viewToWorld, move->m_vecAbsViewAngles );
	}

	move->m_nButtons			= ucmd->buttons;

	// Ingore buttons for movement if at controls
	if ( player->GetFlags() & FL_ATCONTROLS )
	{
		move->m_flForwardMove		= 0;
		move->m_flSideMove			= 0;
		move->m_flUpMove				= 0;
	}
	else
	{
		move->m_flForwardMove		= ucmd->forwardmove;
		move->m_flSideMove			= ucmd->sidemove;
		move->m_flUpMove				= ucmd->upmove;
	}

	// Prepare remaining fields
	move->m_flClientMaxSpeed		= player->m_flMaxspeed;
	move->m_nOldButtons			= player->m_Local.m_nOldButtons;
	move->m_vecAngles			= player->pl.v_angle;

	move->m_vecVelocity			= player->GetAbsVelocity();

	move->m_nPlayerHandle		= player;

	move->SetAbsOrigin( player->GetAbsOrigin() );

	// Copy constraint information
	if ( player->m_hConstraintEntity.Get() )
		move->m_vecConstraintCenter = player->m_hConstraintEntity.Get()->GetAbsOrigin();
	else
		move->m_vecConstraintCenter = player->m_vecConstraintCenter;
	move->m_flConstraintRadius = player->m_flConstraintRadius;
	move->m_flConstraintWidth = player->m_flConstraintWidth;
	move->m_flConstraintSpeedFactor = player->m_flConstraintSpeedFactor;
}

//-----------------------------------------------------------------------------
// Purpose: Finishes running movement
// Input  : *player - 
//			*move - 
//			*ucmd - 
//			time - 
//-----------------------------------------------------------------------------
void CPlayerMove::FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move )
{
	VPROF( "CPlayerMove::FinishMove" );

	// NOTE: Don't copy this.  the movement code modifies its local copy but is not expecting to be authoritative
	//player->m_flMaxspeed			= move->m_flClientMaxSpeed;
	player->SetAbsOrigin( move->GetAbsOrigin() );
	player->SetAbsVelocity( move->m_vecVelocity );
	player->SetPreviouslyPredictedOrigin( move->GetAbsOrigin() );

	player->m_Local.m_nOldButtons			= move->m_nButtons;

	// Convert final pitch to body pitch
	float pitch = move->m_vecAngles[ PITCH ];
	if ( pitch > 180.0f )
	{
		pitch -= 360.0f;
	}
	pitch = clamp( pitch, -90.f, 90.f );

	move->m_vecAngles[ PITCH ] = pitch;

	player->SetBodyPitch( pitch );

	player->SetLocalAngles( move->m_vecAngles );

	// The class had better not have changed during the move!!
	if ( player->m_hConstraintEntity )
		Assert( move->m_vecConstraintCenter == player->m_hConstraintEntity.Get()->GetAbsOrigin() );
	else
		Assert( move->m_vecConstraintCenter == player->m_vecConstraintCenter );
	Assert( move->m_flConstraintRadius == player->m_flConstraintRadius );
	Assert( move->m_flConstraintWidth == player->m_flConstraintWidth );
	Assert( move->m_flConstraintSpeedFactor == player->m_flConstraintSpeedFactor );
}

//-----------------------------------------------------------------------------
// Purpose: Called before player thinks
// Input  : *player - 
//			thinktime - 
//-----------------------------------------------------------------------------
void CPlayerMove::RunPreThink( CBasePlayer *player )
{
	VPROF( "CPlayerMove::RunPreThink" );

	// Run think functions on the player
	VPROF_SCOPE_BEGIN( "player->PhysicsRunThink()" );
	if ( !player->PhysicsRunThink() )
		return;
	VPROF_SCOPE_END();

	VPROF_SCOPE_BEGIN( "g_pGameRules->PlayerThink( player )" );
	// Called every frame to let game rules do any specific think logic for the player
	g_pGameRules->PlayerThink( player );
	VPROF_SCOPE_END();

	VPROF_SCOPE_BEGIN( "player->PreThink()" );
	player->PreThink();
	VPROF_SCOPE_END();
}

//-----------------------------------------------------------------------------
// Purpose: Runs the PLAYER's thinking code if time.  There is some play in the exact time the think
//  function will be called, because it is called before any movement is done
//  in a frame.  Not used for pushmove objects, because they must be exact.
//  Returns false if the entity removed itself.
// Input  : *ent - 
//			frametime - 
//			clienttimebase - 
// Output : void CPlayerMove::RunThink
//-----------------------------------------------------------------------------
void CPlayerMove::RunThink (CBasePlayer *player, double frametime )
{
	VPROF( "CPlayerMove::RunThink" );
	int thinktick = player->GetNextThinkTick();

	if ( thinktick <= 0 || thinktick > player->m_nTickBase )
		return;
		
	//gpGlobals->curtime = thinktime;
	player->SetNextThink( TICK_NEVER_THINK );

	// Think
	player->Think();
}

//-----------------------------------------------------------------------------
// Purpose: Called after player movement
// Input  : *player - 
//			thinktime - 
//			frametime - 
//-----------------------------------------------------------------------------
void CPlayerMove::RunPostThink( CBasePlayer *player )
{
	VPROF( "CPlayerMove::RunPostThink" );

	// Run post-think
	player->PostThink();
}

void CommentarySystem_PePlayerRunCommand( CBasePlayer *player, CUserCmd *ucmd );

//-----------------------------------------------------------------------------
// Purpose: Runs movement commands for the player
// Input  : *player - 
//			*ucmd - 
//			*moveHelper - 
// Output : void CPlayerMove::RunCommand
//-----------------------------------------------------------------------------
void CPlayerMove::RunCommand ( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *moveHelper )
{
	const float playerCurTime = player->m_nTickBase * TICK_INTERVAL; 
	const float playerFrameTime = player->m_bGamePaused ? 0 : TICK_INTERVAL;
	const float flTimeAllowedForProcessing = player->ConsumeMovementTimeForUserCmdProcessing( playerFrameTime );
	if ( !player->IsBot() && ( flTimeAllowedForProcessing < playerFrameTime ) )
	{
		// Make sure that the activity in command is erased because player cheated or dropped too many packets
		double dblWarningFrequencyThrottle = sv_maxusrcmdprocessticks_warning.GetFloat();
		if ( dblWarningFrequencyThrottle >= 0 )
		{
			static double s_dblLastWarningTime = 0;
			double dblTimeNow = Plat_FloatTime();
			if ( !s_dblLastWarningTime || ( dblTimeNow - s_dblLastWarningTime >= dblWarningFrequencyThrottle ) )
			{
				s_dblLastWarningTime = dblTimeNow;
				Warning( "sv_maxusrcmdprocessticks_warning at server tick %u: Ignored client %s usrcmd (%.6f < %.6f)!\n", gpGlobals->tickcount, player->GetPlayerName(), flTimeAllowedForProcessing, playerFrameTime );
			}
		}
		return; // Don't process this command
	}

	StartCommand( player, ucmd );

	// Set globals appropriately
	gpGlobals->curtime		=  playerCurTime;
	gpGlobals->frametime	=  playerFrameTime;

	// Prevent hacked clients from sending us invalid view angles to try to get leaf server code to crash
	if ( !ucmd->viewangles.IsValid() || !IsEntityQAngleReasonable(ucmd->viewangles) )
	{
		ucmd->viewangles = vec3_angle;
	}

	// Add and subtract buttons we're forcing on the player
	ucmd->buttons |= player->m_afButtonForced;
	ucmd->buttons &= ~player->m_afButtonDisabled;

	if ( player->m_bGamePaused )
	{
		// If no clipping and cheats enabled and noclipduring game enabled, then leave
		//  forwardmove and angles stuff in usercmd
		if ( player->GetMoveType() == MOVETYPE_NOCLIP &&
			 sv_cheats->GetBool() && 
			 sv_noclipduringpause.GetBool() )
		{
			gpGlobals->frametime = TICK_INTERVAL;
		}
	}

	/*
	// TODO:  We can check whether the player is sending more commands than elapsed real time
	cmdtimeremaining -= ucmd->msec;
	if ( cmdtimeremaining < 0 )
	{
	//	return;
	}
	*/

	g_pGameMovement->StartTrackPredictionErrors( player );

	CommentarySystem_PePlayerRunCommand( player, ucmd );

	// Do weapon selection
	if ( ucmd->weaponselect != 0 )
	{
		CBaseCombatWeapon *weapon = dynamic_cast< CBaseCombatWeapon * >( CBaseEntity::Instance( ucmd->weaponselect ) );
		if ( weapon )
		{
			VPROF( "player->SelectItem()" );
			player->SelectItem( weapon->GetName(), ucmd->weaponsubtype );
		}
	}

	IServerVehicle *pVehicle = player->GetVehicle();

	// Latch in impulse.
	if ( ucmd->impulse )
	{
		// Discard impulse commands unless the vehicle allows them.
		// FIXME: UsingStandardWeapons seems like a bad filter for this. The flashlight is an impulse command, for example.
		if ( !pVehicle || player->UsingStandardWeaponsInVehicle() )
		{
			player->m_nImpulse = ucmd->impulse;
		}
	}

	// Update player input button states
	VPROF_SCOPE_BEGIN( "player->UpdateButtonState" );
	player->UpdateButtonState( ucmd->buttons );
	VPROF_SCOPE_END();

	CheckMovingGround( player, TICK_INTERVAL );

	g_pMoveData->m_vecOldAngles = player->pl.v_angle;

	// Copy from command to player unless game .dll has set angle using fixangle
	if ( player->pl.fixangle == FIXANGLE_NONE )
	{
		player->pl.v_angle = ucmd->viewangles;
	}
	else if( player->pl.fixangle == FIXANGLE_RELATIVE )
	{
		player->pl.v_angle = ucmd->viewangles + player->pl.anglechange;
	}

	// Call standard client pre-think
	RunPreThink( player );

	// Call Think if one is set
	RunThink( player, TICK_INTERVAL );

	// Setup input.
	SetupMove( player, ucmd, moveHelper, g_pMoveData );

	// Let the game do the movement.
	if ( !pVehicle )
	{
		VPROF( "g_pGameMovement->ProcessMovement()" );
		Assert( g_pGameMovement );
		g_pGameMovement->ProcessMovement( player, g_pMoveData );
	}
	else
	{
		VPROF( "pVehicle->ProcessMovement()" );
		pVehicle->ProcessMovement( player, g_pMoveData );
	}
			
	// Copy output
	FinishMove( player, ucmd, g_pMoveData );

	// Let server invoke any needed impact functions
	VPROF_SCOPE_BEGIN( "moveHelper->ProcessImpacts" );
	moveHelper->ProcessImpacts();
	VPROF_SCOPE_END();

	RunPostThink( player );

	g_pGameMovement->FinishTrackPredictionErrors( player );

	FinishCommand( player );

	// Let time pass
	if ( gpGlobals->frametime > 0 )
	{
		player->m_nTickBase++;
	}
}
