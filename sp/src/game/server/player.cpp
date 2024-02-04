//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Functions dealing with the player.
//
//===========================================================================//

#include "cbase.h"
#include "const.h"
#include "baseplayer_shared.h"
#include "trains.h"
#include "soundent.h"
#include "gib.h"
#include "shake.h"
#include "decals.h"
#include "gamerules.h"
#include "game.h"
#include "entityapi.h"
#include "entitylist.h"
#include "eventqueue.h"
#include "worldsize.h"
#include "isaverestore.h"
#include "globalstate.h"
#include "basecombatweapon.h"
#include "ai_basenpc.h"
#include "ai_network.h"
#include "ai_node.h"
#include "ai_networkmanager.h"
#include "ammodef.h"
#include "mathlib/mathlib.h"
#include "ndebugoverlay.h"
#include "baseviewmodel.h"
#include "in_buttons.h"
#include "client.h"
#include "team.h"
#include "particle_smokegrenade.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "movehelper_server.h"
#include "igamemovement.h"
#include "saverestoretypes.h"
#include "iservervehicle.h"
#include "movevars_shared.h"
#include "vcollide_parse.h"
#include "player_command.h"
#include "vehicle_base.h"
#include "AI_Criteria.h"
#include "globals.h"
#include "usermessages.h"
#include "gamevars_shared.h"
#include "world.h"
#include "physobj.h"
#include "KeyValues.h"
#include "coordsize.h"
#include "vphysics/player_controller.h"
#include "saverestore_utlvector.h"
#include "hltvdirector.h"
#include "nav_mesh.h"
#include "env_zoom.h"
#include "rumble_shared.h"
#include "gamestats.h"
#ifdef MAPBASE // From Alien Swarm SDK
#include "env_tonemap_controller.h"
#include "fogvolume.h"
#endif
#include "npcevent.h"
#include "datacache/imdlcache.h"
#include "hintsystem.h"
#include "env_debughistory.h"
#include "fogcontroller.h"
#include "gameinterface.h"
#include "hl2orange.spa.h"
#include "dt_utlvector_send.h"
#include "vote_controller.h"
#include "ai_speech.h"

#if defined USES_ECON_ITEMS
#include "econ_wearable.h"
#endif

// NVNT haptic utils
#include "haptics/haptic_utils.h"

#ifdef HL2_DLL
#include "combine_mine.h"
#include "weapon_physcannon.h"
#ifdef MAPBASE
#include "mapbase/GlobalStrings.h"
#include "mapbase/matchers.h"
#endif
#endif

#ifdef MAPBASE_VSCRIPT
#include "mapbase/vscript_funcs_shared.h"
#endif

ConVar autoaim_max_dist( "autoaim_max_dist", "2160" ); // 2160 = 180 feet
ConVar autoaim_max_deflect( "autoaim_max_deflect", "0.99" );

#ifdef CSTRIKE_DLL
ConVar	spec_freeze_time( "spec_freeze_time", "5.0", FCVAR_CHEAT | FCVAR_REPLICATED, "Time spend frozen in observer freeze cam." );
ConVar	spec_freeze_traveltime( "spec_freeze_traveltime", "0.7", FCVAR_CHEAT | FCVAR_REPLICATED, "Time taken to zoom in to frame a target in observer freeze cam.", true, 0.01, false, 0 );
#else
ConVar	spec_freeze_time( "spec_freeze_time", "4.0", FCVAR_CHEAT | FCVAR_REPLICATED, "Time spend frozen in observer freeze cam." );
ConVar	spec_freeze_traveltime( "spec_freeze_traveltime", "0.4", FCVAR_CHEAT | FCVAR_REPLICATED, "Time taken to zoom in to frame a target in observer freeze cam.", true, 0.01, false, 0 );
#endif

ConVar sv_bonus_challenge( "sv_bonus_challenge", "0", FCVAR_REPLICATED, "Set to values other than 0 to select a bonus map challenge type." );

static ConVar sv_maxusrcmdprocessticks( "sv_maxusrcmdprocessticks", "24", FCVAR_NOTIFY, "Maximum number of client-issued usrcmd ticks that can be replayed in packet loss conditions, 0 to allow no restrictions" );

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar old_armor( "player_old_armor", "0" );

static ConVar physicsshadowupdate_render( "physicsshadowupdate_render", "0" );
bool IsInCommentaryMode( void );
bool IsListeningToCommentary( void );

#if !defined( CSTRIKE_DLL )
ConVar cl_sidespeed( "cl_sidespeed", "450", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar cl_upspeed( "cl_upspeed", "320", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar cl_forwardspeed( "cl_forwardspeed", "450", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar cl_backspeed( "cl_backspeed", "450", FCVAR_REPLICATED | FCVAR_CHEAT );
#endif // CSTRIKE_DLL

// This is declared in the engine, too
ConVar	sv_noclipduringpause( "sv_noclipduringpause", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "If cheats are enabled, then you can noclip with the game paused (for doing screenshots, etc.)." );

extern ConVar sv_maxunlag;
extern ConVar sv_turbophysics;
extern ConVar *sv_maxreplay;

extern CServerGameDLL g_ServerGameDLL;

// TIME BASED DAMAGE AMOUNT
// tweak these values based on gameplay feedback:
#define PARALYZE_DURATION	2		// number of 2 second intervals to take damage
#define PARALYZE_DAMAGE		1.0		// damage to take each 2 second interval

#define NERVEGAS_DURATION	2
#define NERVEGAS_DAMAGE		5.0

#define POISON_DURATION		5
#define POISON_DAMAGE		2.0

#define RADIATION_DURATION	2
#define RADIATION_DAMAGE	1.0

#define ACID_DURATION		2
#define ACID_DAMAGE			5.0

#define SLOWBURN_DURATION	2
#define SLOWBURN_DAMAGE		1.0

#define SLOWFREEZE_DURATION	2
#define SLOWFREEZE_DAMAGE	1.0

//----------------------------------------------------
// Player Physics Shadow
//----------------------------------------------------
#define VPHYS_MAX_DISTANCE		2.0
#define VPHYS_MAX_VEL			10
#define VPHYS_MAX_DISTSQR		(VPHYS_MAX_DISTANCE*VPHYS_MAX_DISTANCE)
#define VPHYS_MAX_VELSQR		(VPHYS_MAX_VEL*VPHYS_MAX_VEL)


extern bool		g_fDrawLines;
int				gEvilImpulse101;

bool gInitHUD = true;

extern void respawn(CBaseEntity *pEdict, bool fCopyCorpse);
int MapTextureTypeStepType(char chTextureType);
extern void	SpawnBlood(Vector vecSpot, const Vector &vecDir, int bloodColor, float flDamage);
extern void AddMultiDamage( const CTakeDamageInfo &info, CBaseEntity *pEntity );


#define CMD_MOSTRECENT 0

//#define	FLASH_DRAIN_TIME	 1.2 //100 units/3 minutes
//#define	FLASH_CHARGE_TIME	 0.2 // 100 units/20 seconds  (seconds per unit)


//#define PLAYER_MAX_SAFE_FALL_DIST	20// falling any farther than this many feet will inflict damage
//#define	PLAYER_FATAL_FALL_DIST		60// 100% damage inflicted if player falls this many feet
//#define	DAMAGE_PER_UNIT_FALLEN		(float)( 100 ) / ( ( PLAYER_FATAL_FALL_DIST - PLAYER_MAX_SAFE_FALL_DIST ) * 12 )
//#define MAX_SAFE_FALL_UNITS			( PLAYER_MAX_SAFE_FALL_DIST * 12 )

// player damage adjusters
ConVar	sk_player_head( "sk_player_head","2" );
ConVar	sk_player_chest( "sk_player_chest","1" );
ConVar	sk_player_stomach( "sk_player_stomach","1" );
ConVar	sk_player_arm( "sk_player_arm","1" );
ConVar	sk_player_leg( "sk_player_leg","1" );

//ConVar	player_usercommand_timeout( "player_usercommand_timeout", "10", 0, "After this many seconds without a usercommand from a player, the client is kicked." );
#ifdef _DEBUG
ConVar  sv_player_net_suppress_usercommands( "sv_player_net_suppress_usercommands", "0", FCVAR_CHEAT, "For testing usercommand hacking sideeffects. DO NOT SHIP" );
#endif // _DEBUG
ConVar  sv_player_display_usercommand_errors( "sv_player_display_usercommand_errors", "0", FCVAR_CHEAT, "1 = Display warning when command values are out-of-range. 2 = Spew invalid ranges." );

ConVar  player_debug_print_damage( "player_debug_print_damage", "0", FCVAR_CHEAT, "When true, print amount and type of all damage received by player to console." );

#ifdef MAPBASE
ConVar	player_use_visibility_cache( "player_use_visibility_cache", "0", FCVAR_NONE, "Allows the player to use the visibility cache." );
#endif


void CC_GiveCurrentAmmo( void )
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);

	if( pPlayer )
	{
		CBaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();

		if( pWeapon )
		{
			if( pWeapon->UsesPrimaryAmmo() )
			{
				int ammoIndex = pWeapon->GetPrimaryAmmoType();

				if( ammoIndex != -1 )
				{
					int giveAmount;
					giveAmount = GetAmmoDef()->MaxCarry(ammoIndex);
					pPlayer->GiveAmmo( giveAmount, GetAmmoDef()->GetAmmoOfIndex(ammoIndex)->pName );
				}
			}
			if( pWeapon->UsesSecondaryAmmo() && pWeapon->HasSecondaryAmmo() )
			{
				// Give secondary ammo out, as long as the player already has some
				// from a presumeably natural source. This prevents players on XBox
				// having Combine Balls and so forth in areas of the game that
				// were not tested with these items.
				int ammoIndex = pWeapon->GetSecondaryAmmoType();

				if( ammoIndex != -1 )
				{
					int giveAmount;
					giveAmount = GetAmmoDef()->MaxCarry(ammoIndex);
					pPlayer->GiveAmmo( giveAmount, GetAmmoDef()->GetAmmoOfIndex(ammoIndex)->pName );
				}
			}
		}
	}
}
static ConCommand givecurrentammo("givecurrentammo", CC_GiveCurrentAmmo, "Give a supply of ammo for current weapon..\n", FCVAR_CHEAT );


// pl
BEGIN_SIMPLE_DATADESC( CPlayerState )
	// DEFINE_FIELD( netname, FIELD_STRING ),  // Don't stomp player name with what's in save/restore
	DEFINE_FIELD( v_angle, FIELD_VECTOR ),
	DEFINE_FIELD( deadflag, FIELD_BOOLEAN ),

	// this is always set to true on restore, don't bother saving it.
	// DEFINE_FIELD( fixangle, FIELD_INTEGER ),
	// DEFINE_FIELD( anglechange, FIELD_FLOAT ),
	// DEFINE_FIELD( hltv, FIELD_BOOLEAN ),
	// DEFINE_FIELD( replay, FIELD_BOOLEAN ),
	// DEFINE_FIELD( frags, FIELD_INTEGER ),
	// DEFINE_FIELD( deaths, FIELD_INTEGER ),
END_DATADESC()

// Global Savedata for player
BEGIN_DATADESC( CBasePlayer )

	DEFINE_EMBEDDED( m_Local ),
#if defined USES_ECON_ITEMS
	DEFINE_EMBEDDED( m_AttributeList ),
#endif
	DEFINE_UTLVECTOR( m_hTriggerSoundscapeList, FIELD_EHANDLE ),
	DEFINE_EMBEDDED( pl ),

	DEFINE_FIELD( m_StuckLast, FIELD_INTEGER ),

	DEFINE_FIELD( m_nButtons, FIELD_INTEGER ),
	DEFINE_FIELD( m_afButtonLast, FIELD_INTEGER ),
	DEFINE_FIELD( m_afButtonPressed, FIELD_INTEGER ),
	DEFINE_FIELD( m_afButtonReleased, FIELD_INTEGER ),
	DEFINE_FIELD( m_afButtonDisabled, FIELD_INTEGER ),
	DEFINE_FIELD( m_afButtonForced,	FIELD_INTEGER ),

	DEFINE_FIELD( m_iFOV,		FIELD_INTEGER ),
	DEFINE_FIELD( m_iFOVStart,	FIELD_INTEGER ),
	DEFINE_FIELD( m_flFOVTime,	FIELD_TIME ),
	DEFINE_FIELD( m_iDefaultFOV,FIELD_INTEGER ),
	DEFINE_FIELD( m_flVehicleViewFOV, FIELD_FLOAT ),

	//DEFINE_FIELD( m_fOnTarget, FIELD_BOOLEAN ), // Don't need to restore
	DEFINE_FIELD( m_iObserverMode, FIELD_INTEGER ),
	DEFINE_FIELD( m_iObserverLastMode, FIELD_INTEGER ),
	DEFINE_FIELD( m_hObserverTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bForcedObserverMode, FIELD_BOOLEAN ),
	DEFINE_AUTO_ARRAY( m_szAnimExtension, FIELD_CHARACTER ),
//	DEFINE_CUSTOM_FIELD( m_Activity, ActivityDataOps() ),

	DEFINE_FIELD( m_nUpdateRate, FIELD_INTEGER ),
	DEFINE_FIELD( m_fLerpTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_bLagCompensation, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bPredictWeapons, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_vecAdditionalPVSOrigin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecCameraPVSOrigin, FIELD_POSITION_VECTOR ),

	DEFINE_FIELD( m_hUseEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_iTrain, FIELD_INTEGER ),
	DEFINE_FIELD( m_iRespawnFrames, FIELD_FLOAT ),
	DEFINE_FIELD( m_afPhysicsFlags, FIELD_INTEGER ),
	DEFINE_FIELD( m_hVehicle, FIELD_EHANDLE ),

	// recreate, don't restore
	// DEFINE_FIELD( m_CommandContext, CUtlVector < CCommandContext > ),
	//DEFINE_FIELD( m_pPhysicsController, FIELD_POINTER ),
	//DEFINE_FIELD( m_pShadowStand, FIELD_POINTER ),
	//DEFINE_FIELD( m_pShadowCrouch, FIELD_POINTER ),
	//DEFINE_FIELD( m_vphysicsCollisionState, FIELD_INTEGER ),
	DEFINE_ARRAY( m_szNetworkIDString, FIELD_CHARACTER, MAX_NETWORKID_LENGTH ),	
	DEFINE_FIELD( m_oldOrigin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecSmoothedVelocity, FIELD_VECTOR ),
	//DEFINE_FIELD( m_touchedPhysObject, FIELD_BOOLEAN ),
	//DEFINE_FIELD( m_bPhysicsWasFrozen, FIELD_BOOLEAN ),
	//DEFINE_FIELD( m_iPlayerSound, FIELD_INTEGER ),	// Don't restore, set in Precache()
	DEFINE_FIELD( m_iTargetVolume, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_rgItems, FIELD_INTEGER ),
	//DEFINE_FIELD( m_fNextSuicideTime, FIELD_TIME ),
	// DEFINE_FIELD( m_PlayerInfo, CPlayerInfo ),

	DEFINE_FIELD( m_flSwimTime, FIELD_TIME ),
	DEFINE_FIELD( m_flDuckTime, FIELD_TIME ),
	DEFINE_FIELD( m_flDuckJumpTime, FIELD_TIME ),

	DEFINE_FIELD( m_flSuitUpdate, FIELD_TIME ),
	DEFINE_AUTO_ARRAY( m_rgSuitPlayList, FIELD_INTEGER ),
	DEFINE_FIELD( m_iSuitPlayNext, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_rgiSuitNoRepeat, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_rgflSuitNoRepeatTime, FIELD_TIME ),
	DEFINE_FIELD( m_bPauseBonusProgress, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iBonusProgress, FIELD_INTEGER ),
	DEFINE_FIELD( m_iBonusChallenge, FIELD_INTEGER ),
	DEFINE_FIELD( m_lastDamageAmount, FIELD_INTEGER ),
	DEFINE_FIELD( m_tbdPrev, FIELD_TIME ),
	DEFINE_FIELD( m_flStepSoundTime, FIELD_FLOAT ),
	DEFINE_ARRAY( m_szNetname, FIELD_CHARACTER, MAX_PLAYER_NAME_LENGTH ),

	//DEFINE_FIELD( m_flgeigerRange, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( m_flgeigerDelay, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( m_igeigerRangePrev, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( m_iStepLeft, FIELD_INTEGER ), // Don't need to restore
	//DEFINE_FIELD( m_chTextureType, FIELD_CHARACTER ), // Don't need to restore
	//DEFINE_FIELD( m_surfaceProps, FIELD_INTEGER ),	// don't need to restore, reset by gamemovement
	// DEFINE_FIELD( m_pSurfaceData, surfacedata_t* ),
	//DEFINE_FIELD( m_surfaceFriction, FIELD_FLOAT ),
	//DEFINE_FIELD( m_chPreviousTextureType, FIELD_CHARACTER ),

	DEFINE_FIELD( m_idrowndmg, FIELD_INTEGER ),
	DEFINE_FIELD( m_idrownrestored, FIELD_INTEGER ),

	DEFINE_FIELD( m_nPoisonDmg, FIELD_INTEGER ),
	DEFINE_FIELD( m_nPoisonRestored, FIELD_INTEGER ),

	DEFINE_FIELD( m_bitsHUDDamage, FIELD_INTEGER ),
	DEFINE_FIELD( m_fInitHUD, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flDeathTime, FIELD_TIME ),
	DEFINE_FIELD( m_flDeathAnimTime, FIELD_TIME ),

	//DEFINE_FIELD( m_fGameHUDInitialized, FIELD_BOOLEAN ), // only used in multiplayer games
	//DEFINE_FIELD( m_fWeapon, FIELD_BOOLEAN ),  // Don't restore, client needs reset
	//DEFINE_FIELD( m_iUpdateTime, FIELD_INTEGER ), // Don't need to restore
	//DEFINE_FIELD( m_iClientBattery, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( m_iClientHideHUD, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( m_vecAutoAim, FIELD_VECTOR ), // Don't save/restore - this is recomputed
	//DEFINE_FIELD( m_lastx, FIELD_INTEGER ),
	//DEFINE_FIELD( m_lasty, FIELD_INTEGER ),

	DEFINE_FIELD( m_iFrags, FIELD_INTEGER ),
	DEFINE_FIELD( m_iDeaths, FIELD_INTEGER ),
	DEFINE_FIELD( m_bAllowInstantSpawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextDecalTime, FIELD_TIME ),
	//DEFINE_AUTO_ARRAY( m_szTeamName, FIELD_STRING ), // mp

	//DEFINE_FIELD( m_iConnected, FIELD_INTEGER ),
	// from edict_t
	DEFINE_FIELD( m_ArmorValue, FIELD_INTEGER ),
	DEFINE_FIELD( m_DmgOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( m_DmgTake, FIELD_FLOAT ),
	DEFINE_FIELD( m_DmgSave, FIELD_FLOAT ),
	DEFINE_FIELD( m_AirFinished, FIELD_TIME ),
	DEFINE_FIELD( m_PainFinished, FIELD_TIME ),
	
	DEFINE_FIELD( m_iPlayerLocked, FIELD_INTEGER ),

	DEFINE_AUTO_ARRAY( m_hViewModel, FIELD_EHANDLE ),
	
	DEFINE_FIELD( m_flMaxspeed, FIELD_FLOAT ),
	DEFINE_FIELD( m_flWaterJumpTime, FIELD_TIME ),
	DEFINE_FIELD( m_vecWaterJumpVel, FIELD_VECTOR ),
	DEFINE_FIELD( m_nImpulse, FIELD_INTEGER ),
	DEFINE_FIELD( m_flSwimSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_vecLadderNormal, FIELD_VECTOR ),

	DEFINE_FIELD( m_flFlashTime, FIELD_TIME ),
	DEFINE_FIELD( m_nDrownDmgRate, FIELD_INTEGER ),
	DEFINE_FIELD( m_iSuicideCustomKillFlags, FIELD_INTEGER ),

	// NOT SAVED
	//DEFINE_FIELD( m_vForcedOrigin, FIELD_VECTOR ),
	//DEFINE_FIELD( m_bForceOrigin, FIELD_BOOLEAN ),
	//DEFINE_FIELD( m_nTickBase, FIELD_INTEGER ),
	//DEFINE_FIELD( m_LastCmd, FIELD_ ),
	// DEFINE_FIELD( m_pCurrentCommand, CUserCmd ),
	//DEFINE_FIELD( m_bGamePaused, FIELD_BOOLEAN ),
	//	DEFINE_FIELD( m_iVehicleAnalogBias, FIELD_INTEGER ),

	// m_flVehicleViewFOV
	// m_vecVehicleViewOrigin
	// m_vecVehicleViewAngles
	// m_nVehicleViewSavedFrame

	DEFINE_FIELD( m_bitsDamageType, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_rgbTimeBasedDamage, FIELD_CHARACTER ),
	DEFINE_FIELD( m_fLastPlayerTalkTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_hLastWeapon, FIELD_EHANDLE ),

#if !defined( NO_ENTITY_PREDICTION )
	// DEFINE_FIELD( m_SimulatedByThisPlayer, CUtlVector < CHandle < CBaseEntity > > ),
#endif

	DEFINE_FIELD( m_flOldPlayerZ, FIELD_FLOAT ),
	DEFINE_FIELD( m_flOldPlayerViewOffsetZ, FIELD_FLOAT ),
	DEFINE_FIELD( m_bPlayerUnderwater, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hViewEntity, FIELD_EHANDLE ),

	DEFINE_FIELD( m_hConstraintEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_vecConstraintCenter, FIELD_VECTOR ),
	DEFINE_FIELD( m_flConstraintRadius, FIELD_FLOAT ),
	DEFINE_FIELD( m_flConstraintWidth, FIELD_FLOAT ),
	DEFINE_FIELD( m_flConstraintSpeedFactor, FIELD_FLOAT ),
	DEFINE_FIELD( m_hZoomOwner, FIELD_EHANDLE ),
	
	DEFINE_FIELD( m_flLaggedMovementValue, FIELD_FLOAT ),

	DEFINE_FIELD( m_vNewVPhysicsPosition, FIELD_VECTOR ),
	DEFINE_FIELD( m_vNewVPhysicsVelocity, FIELD_VECTOR ),

	DEFINE_FIELD( m_bSinglePlayerGameEnding, FIELD_BOOLEAN ),
	DEFINE_ARRAY( m_szLastPlaceName, FIELD_CHARACTER, MAX_PLACE_NAME_LENGTH ),

	DEFINE_FIELD( m_autoKickDisabled, FIELD_BOOLEAN ),

#ifdef MAPBASE
	DEFINE_FIELD( m_bInTriggerFall, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_bDrawPlayerModelExternally, FIELD_BOOLEAN ),
#endif

	// Function Pointers
	DEFINE_FUNCTION( PlayerDeathThink ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetHealth", InputSetHealth ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetHUDVisibility", InputSetHUDVisibility ),
#ifdef MAPBASE // From Alien Swarm SDK (kind of)
	DEFINE_INPUTFUNC( FIELD_INPUT, "SetFogController", InputSetFogController ),
	DEFINE_INPUTFUNC( FIELD_INPUT, "SetPostProcessController", InputSetPostProcessController ),
	DEFINE_INPUTFUNC( FIELD_INPUT, "SetColorCorrectionController", InputSetColorCorrectionController ),
#else
	DEFINE_INPUTFUNC( FIELD_STRING, "SetFogController", InputSetFogController ),
#endif
	DEFINE_INPUTFUNC( FIELD_STRING, "HandleMapEvent", InputHandleMapEvent ),
#ifdef MAPBASE
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetSuppressAttacks", InputSetSuppressAttacks ),
#endif

	DEFINE_FIELD( m_nNumCrouches, FIELD_INTEGER ),
	DEFINE_FIELD( m_bDuckToggled, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flForwardMove, FIELD_FLOAT ),
	DEFINE_FIELD( m_flSideMove, FIELD_FLOAT ),
	DEFINE_FIELD( m_vecPreviouslyPredictedOrigin, FIELD_POSITION_VECTOR ), 

	DEFINE_FIELD( m_nNumCrateHudHints, FIELD_INTEGER ),

#ifdef MAPBASE // From Alien Swarm SDK
	DEFINE_FIELD( m_hPostProcessCtrl, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hColorCorrectionCtrl, FIELD_EHANDLE ),
#endif

	// DEFINE_FIELD( m_nBodyPitchPoseParam, FIELD_INTEGER ),
	// DEFINE_ARRAY( m_StepSoundCache, StepSoundCache_t,  2  ),

	// DEFINE_UTLVECTOR( m_vecPlayerCmdInfo ),
	// DEFINE_UTLVECTOR( m_vecPlayerSimInfo ),
END_DATADESC()

#ifdef MAPBASE_VSCRIPT
// TODO: Better placement?
ScriptHook_t	g_Hook_PlayerRunCommand;

BEGIN_ENT_SCRIPTDESC( CBasePlayer, CBaseCombatCharacter, "The player entity." )

	DEFINE_SCRIPTFUNC_NAMED( ScriptIsPlayerNoclipping, "IsNoclipping", "Returns true if the player is in noclip mode." ) 

	DEFINE_SCRIPTFUNC_NAMED( VScriptGetExpresser, "GetExpresser", "Gets a handle for this player's expresser." )

	DEFINE_SCRIPTFUNC( GetPlayerName, "Gets the player's name." )
	DEFINE_SCRIPTFUNC( GetUserID, "Gets the player's user ID." )
	DEFINE_SCRIPTFUNC_NAMED( GetUserID, "GetPlayerUserId", SCRIPT_HIDE )
	DEFINE_SCRIPTFUNC( GetNetworkIDString, "Gets the player's network (i.e. Steam) ID." )

	DEFINE_SCRIPTFUNC( FragCount, "Gets the number of frags (kills) this player has in a multiplayer game." )
	DEFINE_SCRIPTFUNC( DeathCount, "Gets the number of deaths this player has had in a multiplayer game." )
	DEFINE_SCRIPTFUNC( IsConnected, "Returns true if this player is connected." )
	DEFINE_SCRIPTFUNC( IsDisconnecting, "Returns true if this player is disconnecting." )
	DEFINE_SCRIPTFUNC( IsSuitEquipped, "Returns true if this player had the HEV suit equipped." )

	DEFINE_SCRIPTFUNC_NAMED( ArmorValue, "GetArmor", "Gets the player's armor." )
	DEFINE_SCRIPTFUNC_NAMED( SetArmorValue, "SetArmor", "Sets the player's armor." )

	DEFINE_SCRIPTFUNC( FlashlightIsOn, "Returns true if the flashlight is on." )
	DEFINE_SCRIPTFUNC( FlashlightTurnOn, "Turns on the flashlight." )
	DEFINE_SCRIPTFUNC( FlashlightTurnOff, "Turns off the flashlight." )

	DEFINE_SCRIPTFUNC( DisableButtons, "Disables the specified button mask." )
	DEFINE_SCRIPTFUNC( EnableButtons, "Enables the specified button mask if it was disabled before." )
	DEFINE_SCRIPTFUNC( ForceButtons, "Forces the specified button mask." )
	DEFINE_SCRIPTFUNC( UnforceButtons, "Unforces the specified button mask if it was forced before." )

	DEFINE_SCRIPTFUNC( GetButtons, "Gets the player's active buttons." )
	DEFINE_SCRIPTFUNC( GetButtonPressed, "Gets the player's currently pressed buttons." )
	DEFINE_SCRIPTFUNC( GetButtonReleased, "Gets the player's just-released buttons." )
	DEFINE_SCRIPTFUNC( GetButtonLast, "Gets the player's previously active buttons." )
	DEFINE_SCRIPTFUNC( GetButtonDisabled, "Gets the player's currently unusable buttons." )
	DEFINE_SCRIPTFUNC( GetButtonForced, "Gets the player's currently forced buttons." )

	DEFINE_SCRIPTFUNC( GetFOV, "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetFOVOwner, "GetFOVOwner", "Gets current view owner." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetFOV, "SetFOV", "Sets player FOV regardless of view owner." )

	DEFINE_SCRIPTFUNC( ViewPunch, "Punches the player's view with the specified vector." )
	DEFINE_SCRIPTFUNC( SetMuzzleFlashTime, "Sets the player's muzzle flash time for AI." )
	DEFINE_SCRIPTFUNC( SetSuitUpdate, "Sets an update for the player's HEV suit." )

	DEFINE_SCRIPTFUNC_NAMED( ScriptGetAutoaimVector, "GetAutoaimVector", "Gets the player's autoaim shooting direction with the specified scale." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetAutoaimVectorCustomMaxDist, "GetAutoaimVectorCustomMaxDist", "Gets the player's autoaim shooting direction with the specified scale and a custom max distance." )
	DEFINE_SCRIPTFUNC( ShouldAutoaim, "Returns true if the player should be autoaiming." )

	DEFINE_SCRIPTFUNC_NAMED( ScriptGetEyeForward, "GetEyeForward", "Gets the player's forward eye vector." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetEyeRight, "GetEyeRight", "Gets the player's right eye vector." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetEyeUp, "GetEyeUp", "Gets the player's up eye vector." )

	DEFINE_SCRIPTFUNC_NAMED( ScriptGetViewModel, "GetViewModel", "Returns the viewmodel of the specified index." )

	// 
	// Hooks
	// 
	BEGIN_SCRIPTHOOK( g_Hook_PlayerRunCommand, "PlayerRunCommand", FIELD_VOID, "Called when running a player command on the server." )
		DEFINE_SCRIPTHOOK_PARAM( "command", FIELD_HSCRIPT )
	END_SCRIPTHOOK()

END_SCRIPTDESC();
#else
BEGIN_ENT_SCRIPTDESC( CBasePlayer, CBaseAnimating, "The player entity." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsPlayerNoclipping, "IsNoclipping", "Returns true if the player is in noclip mode." ) 
END_SCRIPTDESC();
#endif

int giPrecacheGrunt = 0;

edict_t *CBasePlayer::s_PlayerEdict = NULL;


inline bool ShouldRunCommandsInContext( const CCommandContext *ctx )
{
	// TODO: This should be enabled at some point. If usercmds can run while paused, then
	// they can create entities which will never die and it will fill up the entity list.
#ifdef NO_USERCMDS_DURING_PAUSE
	return !ctx->paused || sv_noclipduringpause.GetInt();
#else
	return true;
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseViewModel
//-----------------------------------------------------------------------------
CBaseViewModel *CBasePlayer::GetViewModel( int index /*= 0*/, bool bObserverOK )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );
	return m_hViewModel[ index ].Get();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::CreateViewModel( int index /*=0*/ )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );

	if ( GetViewModel( index ) )
		return;

	CBaseViewModel *vm = ( CBaseViewModel * )CreateEntityByName( "viewmodel" );
	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( index );
		DispatchSpawn( vm );
		vm->FollowEntity( this );
		m_hViewModel.Set( index, vm );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::DestroyViewModels( void )
{
	int i;
	for ( i = MAX_VIEWMODELS - 1; i >= 0; i-- )
	{
		CBaseViewModel *vm = GetViewModel( i );
		if ( !vm )
			continue;

		UTIL_Remove( vm );
		m_hViewModel.Set( i, NULL );
	}
}

#ifdef MAPBASE
extern char g_szDefaultHandsModel[MAX_PATH];
extern int g_iDefaultHandsSkin;
extern int g_iDefaultHandsBody;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::CreateHandModel(int index, int iOtherVm)
{
	Assert(index >= 0 && index < MAX_VIEWMODELS && iOtherVm >= 0 && iOtherVm < MAX_VIEWMODELS );

	if (GetViewModel( index ))
	{
		// This can happen if the player respawns
		// Don't draw unless we're already using a hands weapon
		if ( !GetActiveWeapon() || !GetActiveWeapon()->UsesHands() )
			GetViewModel( index )->AddEffects( EF_NODRAW );
		return;
	}

	CBaseViewModel *vm = (CBaseViewModel *)CreateEntityByName("hand_viewmodel");
	if (vm)
	{
		vm->SetAbsOrigin(GetAbsOrigin());
		vm->SetOwner(this);
		vm->SetIndex(index);

		vm->SetModel( g_szDefaultHandsModel );
		vm->m_nSkin = g_iDefaultHandsSkin;
		vm->m_nBody = g_iDefaultHandsBody;

		DispatchSpawn(vm);
		vm->FollowEntity(GetViewModel(iOtherVm), true);
		m_hViewModel.Set(index, vm);
		vm->AddEffects( EF_NODRAW );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Static member function to create a player of the specified class
// Input  : *className - 
//			*ed - 
// Output : CBasePlayer
//-----------------------------------------------------------------------------
CBasePlayer *CBasePlayer::CreatePlayer( const char *className, edict_t *ed )
{
	CBasePlayer *player;
	CBasePlayer::s_PlayerEdict = ed;
	player = ( CBasePlayer * )CreateEntityByName( className );
	return player;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
CBasePlayer::CBasePlayer( )
{
	AddEFlags( EFL_NO_AUTO_EDICT_ATTACH );

#ifdef _DEBUG
	m_vecAutoAim.Init();
	m_vecAdditionalPVSOrigin.Init();
	m_vecCameraPVSOrigin.Init();
	m_DmgOrigin.Init();
	m_vecLadderNormal.Init();

	m_oldOrigin.Init();
	m_vecSmoothedVelocity.Init();
#endif

	if ( s_PlayerEdict )
	{
		// take the assigned edict_t and attach it
		Assert( s_PlayerEdict != NULL );
		NetworkProp()->AttachEdict( s_PlayerEdict );
		s_PlayerEdict = NULL;
	}

	m_flFlashTime = -1;
	pl.fixangle = FIXANGLE_ABSOLUTE;
	pl.hltv = false;
	pl.replay = false;
	pl.frags = 0;
	pl.deaths = 0;

	m_szNetname[0] = '\0';

	m_iHealth = 0;
	Weapon_SetLast( NULL );
	m_bitsDamageType = 0;

	m_bForceOrigin = false;
	m_hVehicle = NULL;
	m_pCurrentCommand = NULL;
	
	// Setup our default FOV
	m_iDefaultFOV = g_pGameRules->DefaultFOV();

	m_hZoomOwner = NULL;

	m_nUpdateRate = 20;  // cl_updaterate defualt
	m_fLerpTime = 0.1f; // cl_interp default
	m_bPredictWeapons = true;
	m_bLagCompensation = false;
	m_flLaggedMovementValue = 1.0f;
	m_StuckLast = 0;
	m_impactEnergyScale = 1.0f;
	m_fLastPlayerTalkTime = 0.0f;
	m_PlayerInfo.SetParent( this );

	ResetObserverMode();

	m_surfaceProps = 0;
	m_pSurfaceData = NULL;
	m_surfaceFriction = 1.0f;
	m_chTextureType = 0;
	m_chPreviousTextureType = 0;

	m_iSuicideCustomKillFlags = 0;
	m_fDelay = 0.0f;
	m_fReplayEnd = -1;
	m_iReplayEntity = 0;

	m_autoKickDisabled = false;

	m_nNumCrouches = 0;
	m_bDuckToggled = false;
	m_bPhysicsWasFrozen = false;

	// Used to mask off buttons
	m_afButtonDisabled = 0;
	m_afButtonForced = 0;

	m_nBodyPitchPoseParam = -1;
	m_flForwardMove = 0;
	m_flSideMove = 0;

	// NVNT default to no haptics
	m_bhasHaptics = false;

	m_vecConstraintCenter = vec3_origin;

	m_flLastUserCommandTime = 0.f;
	m_flMovementTimeForUserCmdProcessingRemaining = 0.0f;

	m_hPostProcessCtrl.Set( NULL );
}

CBasePlayer::~CBasePlayer( )
{
	VPhysicsDestroyObject();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
void CBasePlayer::UpdateOnRemove( void )
{
	if ( !g_pGameRules->IsMultiplayer() && g_pScriptVM )
	{
		g_pScriptVM->SetValue( "player", SCRIPT_VARIANT_NULL );
	}

	VPhysicsDestroyObject();

	// Remove him from his current team
	if ( GetTeam() )
	{
		GetTeam()->RemovePlayer( this );
	}

	// Chain at end to mimic destructor unwind order
	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : **pvs - 
//			**pas - 
//-----------------------------------------------------------------------------
void CBasePlayer::SetupVisibility( CBaseEntity *pViewEntity, unsigned char *pvs, int pvssize )
{
	// If we have a viewentity, we don't add the player's origin.
	if ( pViewEntity )
		return;

	Vector org;
	org = EyePosition();

	engine->AddOriginToPVS( org );
}

int	CBasePlayer::UpdateTransmitState()
{
	// always call ShouldTransmit() for players
	return SetTransmitState( FL_EDICT_FULLCHECK );
}

int CBasePlayer::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	// Allow me to introduce myself to, err, myself.
	// I.e., always update the recipient player data even if it's nodraw (first person mode)
	if ( pInfo->m_pClientEnt == edict() )
	{
		return FL_EDICT_ALWAYS;
	}

	// when HLTV/Replay is connected and spectators press +USE, they
	// signal that they are recording a interesting scene
	// so transmit these 'cameramans' to the HLTV or Replay client
	if ( HLTVDirector()->GetCameraMan() == entindex() )
	{
		CBaseEntity *pRecipientEntity = CBaseEntity::Instance( pInfo->m_pClientEnt );
		
		Assert( pRecipientEntity->IsPlayer() );
		
		CBasePlayer *pRecipientPlayer = static_cast<CBasePlayer*>( pRecipientEntity );
		if ( pRecipientPlayer->IsHLTV() ||
			 pRecipientPlayer->IsReplay() )
		{
			// HACK force calling RecomputePVSInformation to update PVS data
			NetworkProp()->AreaNum();
			return FL_EDICT_ALWAYS;
		}
	}

	// Transmit for a short time after death and our death anim finishes so ragdolls can access reliable player data.
	// Note that if m_flDeathAnimTime is never set, as long as m_lifeState is set to LIFE_DEAD after dying, this
	// test will act as if the death anim is finished.
	if ( IsEffectActive( EF_NODRAW ) || ( IsObserver() && ( gpGlobals->curtime - m_flDeathTime > 0.5 ) && 
		( m_lifeState == LIFE_DEAD ) && ( gpGlobals->curtime - m_flDeathAnimTime > 0.5 ) ) )
	{
		return FL_EDICT_DONTSEND;
	}

	return BaseClass::ShouldTransmit( pInfo );
}


bool CBasePlayer::WantsLagCompensationOnEntity( const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const
{
	//Tony; only check teams in teamplay
	if ( gpGlobals->teamplay )
	{
		// Team members shouldn't be adjusted unless friendly fire is on.
		if ( !friendlyfire.GetInt() && pPlayer->GetTeamNumber() == GetTeamNumber() )
			return false;
	}

	// If this entity hasn't been transmitted to us and acked, then don't bother lag compensating it.
	if ( pEntityTransmitBits && !pEntityTransmitBits->Get( pPlayer->entindex() ) )
		return false;

	const Vector &vMyOrigin = GetAbsOrigin();
	const Vector &vHisOrigin = pPlayer->GetAbsOrigin();

	// get max distance player could have moved within max lag compensation time, 
	// multiply by 1.5 to to avoid "dead zones"  (sqrt(2) would be the exact value)
	float maxDistance = 1.5 * pPlayer->MaxSpeed() * sv_maxunlag.GetFloat();

	// If the player is within this distance, lag compensate them in case they're running past us.
	if ( vHisOrigin.DistTo( vMyOrigin ) < maxDistance )
		return true;

	// If their origin is not within a 45 degree cone in front of us, no need to lag compensate.
	Vector vForward;
	AngleVectors( pCmd->viewangles, &vForward );
	
	Vector vDiff = vHisOrigin - vMyOrigin;
	VectorNormalize( vDiff );

	float flCosAngle = 0.707107f;	// 45 degree angle
	if ( vForward.Dot( vDiff ) < flCosAngle )
		return false;

	return true;
}

void CBasePlayer::PauseBonusProgress( bool bPause )
{
	m_bPauseBonusProgress = bPause;
}

void CBasePlayer::SetBonusProgress( int iBonusProgress )
{
	if ( !m_bPauseBonusProgress )
		m_iBonusProgress = iBonusProgress;
}

void CBasePlayer::SetBonusChallenge( int iBonusChallenge )
{
	m_iBonusChallenge = iBonusChallenge;
}


//-----------------------------------------------------------------------------
// Sets the view angles
//-----------------------------------------------------------------------------
void CBasePlayer::SnapEyeAngles( const QAngle &viewAngles )
{
	pl.v_angle = viewAngles;
	pl.fixangle = FIXANGLE_ABSOLUTE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iSpeed - 
//			iMax - 
// Output : int
//-----------------------------------------------------------------------------
int TrainSpeed(int iSpeed, int iMax)
{
	float fSpeed, fMax;
	int iRet = 0;

	fMax = (float)iMax;
	fSpeed = iSpeed;

	fSpeed = fSpeed/fMax;

	if (iSpeed < 0)
		iRet = TRAIN_BACK;
	else if (iSpeed == 0)
		iRet = TRAIN_NEUTRAL;
	else if (fSpeed < 0.33)
		iRet = TRAIN_SLOW;
	else if (fSpeed < 0.66)
		iRet = TRAIN_MEDIUM;
	else
		iRet = TRAIN_FAST;

	return iRet;
}

void CBasePlayer::DeathSound( const CTakeDamageInfo &info )
{
	// temporarily using pain sounds for death sounds

	// Did we die from falling?
	if ( m_bitsDamageType & DMG_FALL )
	{
		// They died in the fall. Play a splat sound.
		EmitSound( "Player.FallGib" );
	}
	else
	{
		EmitSound( "Player.Death" );
	}

	// play one of the suit death alarms
	if ( IsSuitEquipped() )
	{
		UTIL_EmitGroupnameSuit(edict(), "HEV_DEAD");
	}
}

// override takehealth
// bitsDamageType indicates type of damage healed. 

int CBasePlayer::TakeHealth( float flHealth, int bitsDamageType )
{
	// clear out any damage types we healed.
	// UNDONE: generic health should not heal any
	// UNDONE: time-based damage
	if (m_takedamage)
	{
		int bitsDmgTimeBased = g_pGameRules->Damage_GetTimeBased();
		m_bitsDamageType &= ~( bitsDamageType & ~bitsDmgTimeBased );
	}

	// I disabled reporting history into the dbghist because it was super spammy.
	// But, if you need to reenable it, the code is below in the "else" clause.
#if 1 // #ifdef DISABLE_DEBUG_HISTORY
	return BaseClass::TakeHealth (flHealth, bitsDamageType);
#else
	const int healingTaken = BaseClass::TakeHealth(flHealth,bitsDamageType);
	char buf[256];
	Q_snprintf(buf, 256, "[%f] Player %s healed for %d with damagetype %X\n", gpGlobals->curtime, GetDebugName(), healingTaken, bitsDamageType);
	ADD_DEBUG_HISTORY( HISTORY_PLAYER_DAMAGE, buf );

	return healingTaken;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Draw all overlays (should be implemented in cascade by subclass to add
//			any additional non-text overlays)
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
void CBasePlayer::DrawDebugGeometryOverlays(void) 
{
	// --------------------------------------------------------
	// If in buddha mode and dead draw lines to indicate death
	// --------------------------------------------------------
	if ((m_debugOverlays & OVERLAY_BUDDHA_MODE) && m_iHealth == 1)
	{
		Vector vBodyDir = BodyDirection2D( );
		Vector eyePos	= EyePosition() + vBodyDir*10.0;
		Vector vUp		= Vector(0,0,8);
		Vector vSide;
		CrossProduct( vBodyDir, vUp, vSide);
		NDebugOverlay::Line(eyePos+vSide+vUp, eyePos-vSide-vUp, 255,0,0, false, 0);
		NDebugOverlay::Line(eyePos+vSide-vUp, eyePos-vSide+vUp, 255,0,0, false, 0);
	}
	BaseClass::DrawDebugGeometryOverlays();
}

//=========================================================
// TraceAttack
//=========================================================
void CBasePlayer::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	if ( m_takedamage )
	{
		CTakeDamageInfo info = inputInfo;

		if ( info.GetAttacker() )
		{
			// --------------------------------------------------
			//  If an NPC check if friendly fire is disallowed
			// --------------------------------------------------
			CAI_BaseNPC *pNPC = info.GetAttacker()->MyNPCPointer();
#ifdef MAPBASE
			if ( pNPC && (pNPC->CapabilitiesGet() & bits_CAP_NO_HIT_PLAYER) && pNPC->IRelationType( this ) > D_FR )
#else
			if ( pNPC && (pNPC->CapabilitiesGet() & bits_CAP_NO_HIT_PLAYER) && pNPC->IRelationType( this ) != D_HT )
#endif
				return;

			// Prevent team damage here so blood doesn't appear
			if ( info.GetAttacker()->IsPlayer() )
			{
				if ( !g_pGameRules->FPlayerCanTakeDamage( this, info.GetAttacker(), info ) )
					return;
			}
		}

		SetLastHitGroup( ptr->hitgroup );

		
		switch ( ptr->hitgroup )
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			info.ScaleDamage( sk_player_head.GetFloat() );
			break;
		case HITGROUP_CHEST:
			info.ScaleDamage( sk_player_chest.GetFloat() );
			break;
		case HITGROUP_STOMACH:
			info.ScaleDamage( sk_player_stomach.GetFloat() );
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			info.ScaleDamage( sk_player_arm.GetFloat() );
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			info.ScaleDamage( sk_player_leg.GetFloat() );
			break;
		default:
			break;
		}

#ifdef MAPBASE

		// Damage filter bleed control needs to exist on all DLLs
		bool bShouldBleed = 
#ifdef HL2_EPISODIC
			!g_pGameRules->Damage_ShouldNotBleed( info.GetDamageType() ) &&
#endif
			DamageFilterAllowsBlood( info );

		if ( bShouldBleed )
#else
#ifdef HL2_EPISODIC
		// If this damage type makes us bleed, then do so
		bool bShouldBleed = !g_pGameRules->Damage_ShouldNotBleed( info.GetDamageType() );
		if ( bShouldBleed )
#endif
#endif
		{
			SpawnBlood(ptr->endpos, vecDir, BloodColor(), info.GetDamage());// a little surface blood.
			TraceBleed( info.GetDamage(), vecDir, ptr, info.GetDamageType() );
		}

		AddMultiDamage( info, this );
	}
}

//------------------------------------------------------------------------------
// Purpose : Do some kind of damage effect for the type of damage
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBasePlayer::DamageEffect(float flDamage, int fDamageType)
{
	if (fDamageType & DMG_CRUSH)
	{
		//Red damage indicator
		color32 red = {128,0,0,128};
		UTIL_ScreenFade( this, red, 1.0f, 0.1f, FFADE_IN );
	}
	else if (fDamageType & DMG_DROWN)
	{
		//Red damage indicator
		color32 blue = {0,0,128,128};
		UTIL_ScreenFade( this, blue, 1.0f, 0.1f, FFADE_IN );
	}
	else if (fDamageType & DMG_SLASH)
	{
		// If slash damage shoot some blood
		SpawnBlood(EyePosition(), g_vecAttackDir, BloodColor(), flDamage);
	}
	else if (fDamageType & DMG_PLASMA)
	{
		// Blue screen fade
		color32 blue = {0,0,255,100};
		UTIL_ScreenFade( this, blue, 0.2, 0.4, FFADE_MODULATE );

		// Very small screen shake
		// Both -0.1 and 0.1 map to 0 when converted to integer, so all of these RandomInt
		// calls are just expensive ways of returning zero. This code has always been this
		// way and has never had any value. clang complains about the conversion from a
		// literal floating-point number to an integer.
		//ViewPunch(QAngle(random->RandomInt(-0.1,0.1), random->RandomInt(-0.1,0.1), random->RandomInt(-0.1,0.1)));

		// Burn sound 
		EmitSound( "Player.PlasmaDamage" );
	}
	else if (fDamageType & DMG_SONIC)
	{
		// Sonic damage sound 
		EmitSound( "Player.SonicDamage" );
	}
	else if ( fDamageType & DMG_BULLET )
	{
		EmitSound( "Flesh.BulletImpact" );
	}
}

/*
	Take some damage.  
	NOTE: each call to OnTakeDamage with bitsDamageType set to a time-based damage
	type will cause the damage time countdown to be reset.  Thus the ongoing effects of poison, radiation
	etc are implemented with subsequent calls to OnTakeDamage using DMG_GENERIC.
*/

// Old values
#define OLD_ARMOR_RATIO	 0.2	// Armor Takes 80% of the damage
#define OLD_ARMOR_BONUS  0.5	// Each Point of Armor is work 1/x points of health

// New values
#define ARMOR_RATIO	0.2
#define ARMOR_BONUS	1.0

//---------------------------------------------------------
//---------------------------------------------------------
bool CBasePlayer::ShouldTakeDamageInCommentaryMode( const CTakeDamageInfo &inputInfo )
{
	// Only ignore damage when we're listening to a commentary node
	if ( !IsListeningToCommentary() )
		return true;

	// Allow SetHealth inputs to kill player.
	if ( inputInfo.GetInflictor() == this && inputInfo.GetAttacker() == this )
		return true;

#ifdef PORTAL
	if ( inputInfo.GetDamageType() & DMG_ACID )
		return true;
#endif

	// In commentary, ignore all damage except for falling and leeches
	if ( !(inputInfo.GetDamageType() & (DMG_BURN | DMG_PLASMA | DMG_FALL | DMG_CRUSH)) && inputInfo.GetDamageType() != DMG_GENERIC )
		return false;

	// We let DMG_CRUSH pass the check above so that we can check here for stress damage. Deny the CRUSH damage if there is no attacker,
	// or if the attacker isn't a BSP model. Therefore, we're allowing any CRUSH damage done by a BSP model.
	if ( (inputInfo.GetDamageType() & DMG_CRUSH) && ( inputInfo.GetAttacker() == NULL || !inputInfo.GetAttacker()->IsBSPModel() ) )
		return false;

	return true;
}

int CBasePlayer::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	// have suit diagnose the problem - ie: report damage type
	int bitsDamage = inputInfo.GetDamageType();
	int ffound = true;
	int fmajor;
	int fcritical;
	int fTookDamage;
	int ftrivial;
	float flRatio;
	float flBonus;
	float flHealthPrev = m_iHealth;

	CTakeDamageInfo info = inputInfo;

	IServerVehicle *pVehicle = GetVehicle();
	if ( pVehicle )
	{
		// Let the vehicle decide if we should take this damage or not
		if ( pVehicle->PassengerShouldReceiveDamage( info ) == false )
			return 0;
	}

 	if ( IsInCommentaryMode() )
	{
		if( !ShouldTakeDamageInCommentaryMode( info ) )
			return 0;
	}

	if ( GetFlags() & FL_GODMODE )
		return 0;

	if ( m_debugOverlays & OVERLAY_BUDDHA_MODE ) 
	{
		if ((m_iHealth - info.GetDamage()) <= 0)
		{
			m_iHealth = 1;
			return 0;
		}
	}

	// Early out if there's no damage
	if ( !info.GetDamage() )
		return 0;

	if( old_armor.GetBool() )
	{
		flBonus = OLD_ARMOR_BONUS;
		flRatio = OLD_ARMOR_RATIO;
	}
	else
	{
		flBonus = ARMOR_BONUS;
		flRatio = ARMOR_RATIO;
	}

	if ( ( info.GetDamageType() & DMG_BLAST ) && g_pGameRules->IsMultiplayer() )
	{
		// blasts damage armor more.
		flBonus *= 2;
	}

	// Already dead
	if ( !IsAlive() )
		return 0;
	// go take the damage first

	
	if ( !g_pGameRules->FPlayerCanTakeDamage( this, info.GetAttacker(), inputInfo ) )
	{
		// Refuse the damage
		return 0;
	}

	// print to console if the appropriate cvar is set
#ifdef DISABLE_DEBUG_HISTORY
	if (player_debug_print_damage.GetBool() && info.GetDamage() > 0)
#endif
	{
		char dmgtype[64];
		CTakeDamageInfo::DebugGetDamageTypeString( info.GetDamageType(), dmgtype, 512 );
		char outputString[256];
		Q_snprintf( outputString, 256, "%f: Player %s at [%0.2f %0.2f %0.2f] took %f damage from %s, type %s\n", gpGlobals->curtime, GetDebugName(),
			GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z, info.GetDamage(), info.GetInflictor()->GetDebugName(), dmgtype );

		//Msg( "%f: Player %s at [%0.2f %0.2f %0.2f] took %f damage from %s, type %s\n", gpGlobals->curtime, GetDebugName(),
		//	GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z, info.GetDamage(), info.GetInflictor()->GetDebugName(), dmgtype );

		ADD_DEBUG_HISTORY( HISTORY_PLAYER_DAMAGE, outputString );
#ifndef DISABLE_DEBUG_HISTORY
		if ( player_debug_print_damage.GetBool() ) // if we're not in here just for the debug history
#endif
		{
			Msg( "%s", outputString);
		}
	}

	// keep track of amount of damage last sustained
	m_lastDamageAmount = info.GetDamage();

	// Armor. 
	if (m_ArmorValue && !(info.GetDamageType() & (DMG_FALL | DMG_DROWN | DMG_POISON | DMG_RADIATION)) )// armor doesn't protect against fall or drown damage!
	{
		float flNew = info.GetDamage() * flRatio;

		float flArmor;

		flArmor = (info.GetDamage() - flNew) * flBonus;

		if( !old_armor.GetBool() )
		{
			if( flArmor < 1.0 )
			{
				flArmor = 1.0;
			}
		}

		// Does this use more armor than we have?
		if (flArmor > m_ArmorValue)
		{
			flArmor = m_ArmorValue;
			flArmor *= (1/flBonus);
			flNew = info.GetDamage() - flArmor;
			m_DmgSave = m_ArmorValue;
			m_ArmorValue = 0;
		}
		else
		{
			m_DmgSave = flArmor;
			m_ArmorValue -= flArmor;
		}
		
		info.SetDamage( flNew );
	}


#if defined( WIN32 ) && !defined( _X360 )
	// NVNT if player's client has a haptic device send them a user message with the damage.
	if(HasHaptics())
		HapticsDamage(this,info);
#endif

	// this cast to INT is critical!!! If a player ends up with 0.5 health, the engine will get that
	// as an int (zero) and think the player is dead! (this will incite a clientside screentilt, etc)
	
	// NOTENOTE: jdw - We are now capable of retaining the mantissa of this damage value and deferring its application
	
	// info.SetDamage( (int)info.GetDamage() );

	// Call up to the base class
	fTookDamage = BaseClass::OnTakeDamage( info );

	// Early out if the base class took no damage
	if ( !fTookDamage )
		return 0;

	// add to the damage total for clients, which will be sent as a single
	// message at the end of the frame
	// todo: remove after combining shotgun blasts?
	if ( info.GetInflictor() && info.GetInflictor()->edict() )
		m_DmgOrigin = info.GetInflictor()->GetAbsOrigin();

	m_DmgTake += (int)info.GetDamage();
	
	// Reset damage time countdown for each type of time based damage player just sustained
	for (int i = 0; i < CDMG_TIMEBASED; i++)
	{
		// Make sure the damage type is really time-based.
		// This is kind of hacky but necessary until we setup DamageType as an enum.
		int iDamage = ( DMG_PARALYZE << i );
		if ( ( info.GetDamageType() & iDamage ) && g_pGameRules->Damage_IsTimeBased( iDamage ) )
		{
			m_rgbTimeBasedDamage[i] = 0;
		}
	}

	// Display any effect associate with this damage type
	DamageEffect(info.GetDamage(),bitsDamage);

	// how bad is it, doc?
	ftrivial = (m_iHealth > 75 || m_lastDamageAmount < 5);
	fmajor = (m_lastDamageAmount > 25);
	fcritical = (m_iHealth < 30);

	// handle all bits set in this damage message,
	// let the suit give player the diagnosis

	// UNDONE: add sounds for types of damage sustained (ie: burn, shock, slash )

	// UNDONE: still need to record damage and heal messages for the following types

		// DMG_BURN	
		// DMG_FREEZE
		// DMG_BLAST
		// DMG_SHOCK

	m_bitsDamageType |= bitsDamage; // Save this so we can report it to the client
	m_bitsHUDDamage = -1;  // make sure the damage bits get resent

	while (fTookDamage && (!ftrivial || g_pGameRules->Damage_IsTimeBased( bitsDamage ) ) && ffound && bitsDamage)
	{
		ffound = false;

		if (bitsDamage & DMG_CLUB)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG4", false, SUIT_NEXT_IN_30SEC);	// minor fracture
			bitsDamage &= ~DMG_CLUB;
			ffound = true;
		}
		if (bitsDamage & (DMG_FALL | DMG_CRUSH))
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG5", false, SUIT_NEXT_IN_30SEC);	// major fracture
			else
				SetSuitUpdate("!HEV_DMG4", false, SUIT_NEXT_IN_30SEC);	// minor fracture
	
			bitsDamage &= ~(DMG_FALL | DMG_CRUSH);
			ffound = true;
		}
		
		if (bitsDamage & DMG_BULLET)
		{
			if (m_lastDamageAmount > 5)
				SetSuitUpdate("!HEV_DMG6", false, SUIT_NEXT_IN_30SEC);	// blood loss detected
			//else
			//	SetSuitUpdate("!HEV_DMG0", false, SUIT_NEXT_IN_30SEC);	// minor laceration
			
			bitsDamage &= ~DMG_BULLET;
			ffound = true;
		}

		if (bitsDamage & DMG_SLASH)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG1", false, SUIT_NEXT_IN_30SEC);	// major laceration
			else
				SetSuitUpdate("!HEV_DMG0", false, SUIT_NEXT_IN_30SEC);	// minor laceration

			bitsDamage &= ~DMG_SLASH;
			ffound = true;
		}
		
		if (bitsDamage & DMG_SONIC)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG2", false, SUIT_NEXT_IN_1MIN);	// internal bleeding
			bitsDamage &= ~DMG_SONIC;
			ffound = true;
		}

		if (bitsDamage & (DMG_POISON | DMG_PARALYZE))
		{
			if (bitsDamage & DMG_POISON)
			{
				m_nPoisonDmg += info.GetDamage();
				m_tbdPrev = gpGlobals->curtime;
				m_rgbTimeBasedDamage[itbd_PoisonRecover] = 0;
			}

			SetSuitUpdate("!HEV_DMG3", false, SUIT_NEXT_IN_1MIN);	// blood toxins detected
			bitsDamage &= ~( DMG_POISON | DMG_PARALYZE );
			ffound = true;
		}

		if (bitsDamage & DMG_ACID)
		{
			SetSuitUpdate("!HEV_DET1", false, SUIT_NEXT_IN_1MIN);	// hazardous chemicals detected
			bitsDamage &= ~DMG_ACID;
			ffound = true;
		}

		if (bitsDamage & DMG_NERVEGAS)
		{
			SetSuitUpdate("!HEV_DET0", false, SUIT_NEXT_IN_1MIN);	// biohazard detected
			bitsDamage &= ~DMG_NERVEGAS;
			ffound = true;
		}

		if (bitsDamage & DMG_RADIATION)
		{
			SetSuitUpdate("!HEV_DET2", false, SUIT_NEXT_IN_1MIN);	// radiation detected
			bitsDamage &= ~DMG_RADIATION;
			ffound = true;
		}
		if (bitsDamage & DMG_SHOCK)
		{
			bitsDamage &= ~DMG_SHOCK;
			ffound = true;
		}
	}

	float flPunch = -2;

	if( hl2_episodic.GetBool() && info.GetAttacker() && !FInViewCone( info.GetAttacker() ) )
	{
		if( info.GetDamage() > 10.0f )
			flPunch = -10;
		else
			flPunch = RandomFloat( -5, -7 );
	}

	m_Local.m_vecPunchAngle.SetX( flPunch );

	if (fTookDamage && !ftrivial && fmajor && flHealthPrev >= 75) 
	{
		// first time we take major damage...
		// turn automedic on if not on
		SetSuitUpdate("!HEV_MED1", false, SUIT_NEXT_IN_30MIN);	// automedic on

		// give morphine shot if not given recently
		SetSuitUpdate("!HEV_HEAL7", false, SUIT_NEXT_IN_30MIN);	// morphine shot
	}
	
	if (fTookDamage && !ftrivial && fcritical && flHealthPrev < 75)
	{

		// already took major damage, now it's critical...
		if (m_iHealth < 6)
			SetSuitUpdate("!HEV_HLTH3", false, SUIT_NEXT_IN_10MIN);	// near death
		else if (m_iHealth < 20)
			SetSuitUpdate("!HEV_HLTH2", false, SUIT_NEXT_IN_10MIN);	// health critical
	
		// give critical health warnings
		if (!random->RandomInt(0,3) && flHealthPrev < 50)
			SetSuitUpdate("!HEV_DMG7", false, SUIT_NEXT_IN_5MIN); //seek medical attention
	}

	// if we're taking time based damage, warn about its continuing effects
	if (fTookDamage && g_pGameRules->Damage_IsTimeBased( info.GetDamageType() ) && flHealthPrev < 75)
		{
			if (flHealthPrev < 50)
			{
				if (!random->RandomInt(0,3))
					SetSuitUpdate("!HEV_DMG7", false, SUIT_NEXT_IN_5MIN); //seek medical attention
			}
			else
				SetSuitUpdate("!HEV_HLTH1", false, SUIT_NEXT_IN_10MIN);	// health dropping
		}

	// Do special explosion damage effect
	if ( bitsDamage & DMG_BLAST )
	{
		OnDamagedByExplosion( info );
	}

	return fTookDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//			damageAmount - 
//-----------------------------------------------------------------------------
#define MIN_SHOCK_AND_CONFUSION_DAMAGE	30.0f
#define MIN_EAR_RINGING_DISTANCE		240.0f  // 20 feet

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
void CBasePlayer::OnDamagedByExplosion( const CTakeDamageInfo &info )
{
	float lastDamage = info.GetDamage();

	float distanceFromPlayer = 9999.0f;

	CBaseEntity *inflictor = info.GetInflictor();
	if ( inflictor )
	{
		Vector delta = GetAbsOrigin() - inflictor->GetAbsOrigin();
		distanceFromPlayer = delta.Length();
	}

	bool ear_ringing = distanceFromPlayer < MIN_EAR_RINGING_DISTANCE ? true : false;
	bool shock = lastDamage >= MIN_SHOCK_AND_CONFUSION_DAMAGE;

	if ( !shock && !ear_ringing )
		return;

	int effect = shock ? 
		random->RandomInt( 35, 37 ) : 
		random->RandomInt( 32, 34 );

	CSingleUserRecipientFilter user( this );
	enginesound->SetPlayerDSP( user, effect, false );
}

//=========================================================
// PackDeadPlayerItems - call this when a player dies to
// pack up the appropriate weapons and ammo items, and to
// destroy anything that shouldn't be packed.
//
// This is pretty brute force :(
//=========================================================
void CBasePlayer::PackDeadPlayerItems( void )
{
	int iWeaponRules;
	int iAmmoRules;
	int i;
	CBaseCombatWeapon *rgpPackWeapons[ 20 ];// 20 hardcoded for now. How to determine exactly how many weapons we have?
	int iPackAmmo[ MAX_AMMO_SLOTS + 1];
	int iPW = 0;// index into packweapons array
	int iPA = 0;// index into packammo array

	memset(rgpPackWeapons, NULL, sizeof(rgpPackWeapons) );
	memset(iPackAmmo, -1, sizeof(iPackAmmo) );

	// get the game rules 
	iWeaponRules = g_pGameRules->DeadPlayerWeapons( this );
 	iAmmoRules = g_pGameRules->DeadPlayerAmmo( this );

	if ( iWeaponRules == GR_PLR_DROP_GUN_NO && iAmmoRules == GR_PLR_DROP_AMMO_NO )
	{
		// nothing to pack. Remove the weapons and return. Don't call create on the box!
		RemoveAllItems( true );
		return;
	}

// go through all of the weapons and make a list of the ones to pack
	for ( i = 0 ; i < WeaponCount() ; i++ )
	{
		// there's a weapon here. Should I pack it?
		CBaseCombatWeapon *pPlayerItem = GetWeapon( i );
		if ( pPlayerItem )
		{
			switch( iWeaponRules )
			{
			case GR_PLR_DROP_GUN_ACTIVE:
				if ( GetActiveWeapon() && pPlayerItem == GetActiveWeapon() )
				{
					// this is the active item. Pack it.
					rgpPackWeapons[ iPW++ ] = pPlayerItem;
				}
				break;

			case GR_PLR_DROP_GUN_ALL:
				rgpPackWeapons[ iPW++ ] = pPlayerItem;
				break;

			default:
				break;
			}
		}
	}

// now go through ammo and make a list of which types to pack.
	if ( iAmmoRules != GR_PLR_DROP_AMMO_NO )
	{
		for ( i = 0 ; i < MAX_AMMO_SLOTS ; i++ )
		{
			if ( GetAmmoCount( i ) > 0 )
			{
				// player has some ammo of this type.
				switch ( iAmmoRules )
				{
				case GR_PLR_DROP_AMMO_ALL:
					iPackAmmo[ iPA++ ] = i;
					break;

				case GR_PLR_DROP_AMMO_ACTIVE:
					// WEAPONTODO: Make this work
					/*
					if ( GetActiveWeapon() && i == GetActiveWeapon()->m_iPrimaryAmmoType ) 
					{
						// this is the primary ammo type for the active weapon
						iPackAmmo[ iPA++ ] = i;
					}
					else if ( GetActiveWeapon() && i == GetActiveWeapon()->m_iSecondaryAmmoType ) 
					{
						// this is the secondary ammo type for the active weapon
						iPackAmmo[ iPA++ ] = i;
					}
					*/
					break;

				default:
					break;
				}
			}
		}
	}

	RemoveAllItems( true );// now strip off everything that wasn't handled by the code above.
}

void CBasePlayer::RemoveAllItems( bool removeSuit )
{
	if (GetActiveWeapon())
	{
		ResetAutoaim( );
		GetActiveWeapon()->Holster( );
	}

	Weapon_SetLast( NULL );
	RemoveAllWeapons();
 	RemoveAllAmmo();

#ifdef MAPBASE
	// Hide hand viewmodel
	CBaseViewModel *vm = GetViewModel( 1 );
	if ( vm )
	{
		vm->AddEffects( EF_NODRAW );
	}
#endif

	if ( removeSuit )
	{
		RemoveSuit();
	}

	UpdateClientData();
}

//Tony; correct this for base code so that IsDead will be correct accross all games.
bool CBasePlayer::IsDead() const
{
	return m_lifeState != LIFE_ALIVE;
}

static float DamageForce( const Vector &size, float damage )
{ 
	float force = damage * ((32 * 32 * 72.0) / (size.x * size.y * size.z)) * 5;
	
	if ( force > 1000.0) 
	{
		force = 1000.0;
	}

	return force;
}


const impactdamagetable_t &CBasePlayer::GetPhysicsImpactDamageTable()
{
	return gDefaultPlayerImpactDamageTable;
}


int CBasePlayer::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// set damage type sustained
	m_bitsDamageType |= info.GetDamageType();

	if ( !BaseClass::OnTakeDamage_Alive( info ) )
		return 0;

	CBaseEntity * attacker = info.GetAttacker();

	if ( !attacker )
		return 0;

	Vector vecDir = vec3_origin;
	if ( info.GetInflictor() )
	{
		vecDir = info.GetInflictor()->WorldSpaceCenter() - Vector ( 0, 0, 10 ) - WorldSpaceCenter();
		VectorNormalize( vecDir );
	}

	if ( info.GetInflictor() && (GetMoveType() == MOVETYPE_WALK) && 
		( !attacker->IsSolidFlagSet(FSOLID_TRIGGER)) )
	{
		Vector force = vecDir * -DamageForce( WorldAlignSize(), info.GetBaseDamage() );
		if ( force.z > 250.0f )
		{
			force.z = 250.0f;
		}
		ApplyAbsVelocityImpulse( force );
	}

	// fire global game event

	IGameEvent * event = gameeventmanager->CreateEvent( "player_hurt" );
	if ( event )
	{
		event->SetInt("userid", GetUserID() );
		event->SetInt("health", MAX(0, m_iHealth) );
		event->SetInt("priority", 5 );	// HLTV event priority, not transmitted

		if ( attacker->IsPlayer() )
		{
			CBasePlayer *player = ToBasePlayer( attacker );
			event->SetInt("attacker", player->GetUserID() ); // hurt by other player
		}
		else
		{
			event->SetInt("attacker", 0 ); // hurt by "world"
		}

        gameeventmanager->FireEvent( event );
	}
	
	// Insert a combat sound so that nearby NPCs hear battle
	if ( attacker->IsNPC() )
	{
		CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 512, 0.5, this );//<<TODO>>//magic number
	}

	return 1;
}


void CBasePlayer::Event_Killed( const CTakeDamageInfo &info )
{
	CSound *pSound;

	if ( Hints() )
	{
		Hints()->ResetHintTimers();
	}

	g_pGameRules->PlayerKilled( this, info );

	gamestats->Event_PlayerKilled( this, info );

	RumbleEffect( RUMBLE_STOP_ALL, 0, RUMBLE_FLAGS_NONE );

#if defined( WIN32 ) && !defined( _X360 )
	// NVNT set the drag to zero in the case of underwater death.
	HapticSetDrag(this,0);
#endif
	ClearUseEntity();
	
	// this client isn't going to be thinking for a while, so reset the sound until they respawn
	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( edict() ) );
	{
		if ( pSound )
		{
			pSound->Reset();
		}
	}

	// don't let the status bar glitch for players with <0 health.
	if (m_iHealth < -99)
	{
		m_iHealth = 0;
	}

	// holster the current weapon
	if ( GetActiveWeapon() )
	{
		GetActiveWeapon()->Holster();
	}

	SetAnimation( PLAYER_DIE );

	if ( !IsObserver() )
	{
		SetViewOffset( VEC_DEAD_VIEWHEIGHT_SCALED( this ) );
	}
	m_lifeState		= LIFE_DYING;

	pl.deadflag = true;
	AddSolidFlags( FSOLID_NOT_SOLID );
	// force contact points to get flushed if no longer valid
	// UNDONE: Always do this on RecheckCollisionFilter() ?
	IPhysicsObject *pObject = VPhysicsGetObject();
	if ( pObject )
	{
		pObject->RecheckContactPoints();
	}

	SetMoveType( MOVETYPE_FLYGRAVITY );
	SetGroundEntity( NULL );

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(NULL, false, 0);

	// reset FOV
	SetFOV( this, 0 );
	
	if ( FlashlightIsOn() )
	{
		 FlashlightTurnOff();
	}

	m_flDeathTime = gpGlobals->curtime;

	ClearLastKnownArea();

	BaseClass::Event_Killed( info );
}

void CBasePlayer::Event_Dying( const CTakeDamageInfo& info )
{
	// NOT GIBBED, RUN THIS CODE

	DeathSound( info );

	// The dead body rolls out of the vehicle.
	if ( IsInAVehicle() )
	{
		LeaveVehicle();
	}

	QAngle angles = GetLocalAngles();

	angles.x = 0;
	angles.z = 0;
	
	SetLocalAngles( angles );

	SetThink(&CBasePlayer::PlayerDeathThink);
	SetNextThink( gpGlobals->curtime + 0.1f );
	BaseClass::Event_Dying( info );
}


// Set the activity based on an event or current state
void CBasePlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	int animDesired;
	char szAnim[64];

	float speed;

	speed = GetAbsVelocity().Length2D();

	if (GetFlags() & (FL_FROZEN|FL_ATCONTROLS))
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	Activity idealActivity = ACT_WALK;// TEMP!!!!!

	// This could stand to be redone. Why is playerAnim abstracted from activity? (sjb)
	if (playerAnim == PLAYER_JUMP)
	{
		idealActivity = ACT_HOP;
	}
	else if (playerAnim == PLAYER_SUPERJUMP)
	{
		idealActivity = ACT_LEAP;
	}
	else if (playerAnim == PLAYER_DIE)
	{
		if ( m_lifeState == LIFE_ALIVE )
		{
			idealActivity = GetDeathActivity();
		}
	}
	else if (playerAnim == PLAYER_ATTACK1)
	{
		if ( m_Activity == ACT_HOVER	|| 
			 m_Activity == ACT_SWIM		||
			 m_Activity == ACT_HOP		||
			 m_Activity == ACT_LEAP		||
			 m_Activity == ACT_DIESIMPLE )
		{
			idealActivity = m_Activity;
		}
		else
		{
			idealActivity = ACT_RANGE_ATTACK1;
		}
	}
	else if (playerAnim == PLAYER_IDLE || playerAnim == PLAYER_WALK)
	{
		if ( !( GetFlags() & FL_ONGROUND ) && (m_Activity == ACT_HOP || m_Activity == ACT_LEAP) )	// Still jumping
		{
			idealActivity = m_Activity;
		}
		else if ( GetWaterLevel() > 1 )
		{
			if ( speed == 0 )
				idealActivity = ACT_HOVER;
			else
				idealActivity = ACT_SWIM;
		}
		else
		{
			idealActivity = ACT_WALK;
		}
	}

	
	if (idealActivity == ACT_RANGE_ATTACK1)
	{
		if ( GetFlags() & FL_DUCKING )	// crouching
		{
			Q_strncpy( szAnim, "crouch_shoot_" ,sizeof(szAnim));
		}
		else
		{
			Q_strncpy( szAnim, "ref_shoot_" ,sizeof(szAnim));
		}
		Q_strncat( szAnim, m_szAnimExtension ,sizeof(szAnim), COPY_ALL_CHARACTERS );
		animDesired = LookupSequence( szAnim );
		if (animDesired == -1)
			animDesired = 0;

		if ( GetSequence() != animDesired || !SequenceLoops() )
		{
			SetCycle( 0 );
		}

		// Tracker 24588:  In single player when firing own weapon this causes eye and punchangle to jitter
		//if (!SequenceLoops())
		//{
		//	IncrementInterpolationFrame();
		//}

		SetActivity( idealActivity );
		ResetSequence( animDesired );
	}
	else if (idealActivity == ACT_WALK)
	{
		if (GetActivity() != ACT_RANGE_ATTACK1 || IsActivityFinished())
		{
			if ( GetFlags() & FL_DUCKING )	// crouching
			{
				Q_strncpy( szAnim, "crouch_aim_" ,sizeof(szAnim));
			}
			else
			{
				Q_strncpy( szAnim, "ref_aim_" ,sizeof(szAnim));
			}
			Q_strncat( szAnim, m_szAnimExtension,sizeof(szAnim), COPY_ALL_CHARACTERS );
			animDesired = LookupSequence( szAnim );
			if (animDesired == -1)
				animDesired = 0;
			SetActivity( ACT_WALK );
		}
		else
		{
			animDesired = GetSequence();
		}
	}
	else
	{
		if ( GetActivity() == idealActivity)
			return;
	
		SetActivity( idealActivity );

		animDesired = SelectWeightedSequence( m_Activity );

		// Already using the desired animation?
		if (GetSequence() == animDesired)
			return;

		ResetSequence( animDesired );
		SetCycle( 0 );
		return;
	}

	// Already using the desired animation?
	if (GetSequence() == animDesired)
		return;

	//Msg( "Set animation to %d\n", animDesired );
	// Reset to first frame of desired animation
	ResetSequence( animDesired );
	SetCycle( 0 );
}

/*
===========
WaterMove
============
*/
#ifdef HL2_DLL

// test for HL2 drowning damage increase (aux power used instead)
#define AIRTIME						7		// lung full of air lasts this many seconds
#define DROWNING_DAMAGE_INITIAL		10
#define DROWNING_DAMAGE_MAX			10

#else

#define AIRTIME						12		// lung full of air lasts this many seconds
#define DROWNING_DAMAGE_INITIAL		2
#define DROWNING_DAMAGE_MAX			5

#endif

void CBasePlayer::WaterMove()
{
	if ( ( GetMoveType() == MOVETYPE_NOCLIP ) && !GetMoveParent() )
	{
		m_AirFinished = gpGlobals->curtime + AIRTIME;
		return;
	}

	if ( m_iHealth < 0 || !IsAlive() )
	{
		UpdateUnderwaterState();
		return;
	}

	// waterlevel 0 - not in water (WL_NotInWater)
	// waterlevel 1 - feet in water (WL_Feet)
	// waterlevel 2 - waist in water (WL_Waist)
	// waterlevel 3 - head in water (WL_Eyes)

	if (GetWaterLevel() != WL_Eyes || CanBreatheUnderwater()) 
	{
		// not underwater
		
		// play 'up for air' sound
		
		if (m_AirFinished < gpGlobals->curtime)
		{
			EmitSound( "Player.DrownStart" );
		}

		m_AirFinished = gpGlobals->curtime + AIRTIME;
		m_nDrownDmgRate = DROWNING_DAMAGE_INITIAL;

		// if we took drowning damage, give it back slowly
		if (m_idrowndmg > m_idrownrestored)
		{
			// set drowning damage bit.  hack - dmg_drownrecover actually
			// makes the time based damage code 'give back' health over time.
			// make sure counter is cleared so we start count correctly.
			
			// NOTE: this actually causes the count to continue restarting
			// until all drowning damage is healed.

			m_bitsDamageType |= DMG_DROWNRECOVER;
			m_bitsDamageType &= ~DMG_DROWN;
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		}

	}
	else
	{	// fully under water
		// stop restoring damage while underwater
		m_bitsDamageType &= ~DMG_DROWNRECOVER;
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;

		if (m_AirFinished < gpGlobals->curtime && !(GetFlags() & FL_GODMODE) )		// drown!
		{
			if (m_PainFinished < gpGlobals->curtime)
			{
				// take drowning damage
				m_nDrownDmgRate += 1;
				if (m_nDrownDmgRate > DROWNING_DAMAGE_MAX)
				{
					m_nDrownDmgRate = DROWNING_DAMAGE_MAX;
				}

				OnTakeDamage( CTakeDamageInfo( GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), m_nDrownDmgRate, DMG_DROWN ) );
				m_PainFinished = gpGlobals->curtime + 1;
				
				// track drowning damage, give it back when
				// player finally takes a breath
				m_idrowndmg += m_nDrownDmgRate;
			} 
		}
		else
		{
			m_bitsDamageType &= ~DMG_DROWN;
		}
	}

	UpdateUnderwaterState();
}


// true if the player is attached to a ladder
bool CBasePlayer::IsOnLadder( void )
{ 
	return (GetMoveType() == MOVETYPE_LADDER);
}


float CBasePlayer::GetWaterJumpTime() const
{
	return m_flWaterJumpTime;
}

void CBasePlayer::SetWaterJumpTime( float flWaterJumpTime )
{
	m_flWaterJumpTime = flWaterJumpTime;
}

float CBasePlayer::GetSwimSoundTime( void ) const
{
	return m_flSwimSoundTime;
}

void CBasePlayer::SetSwimSoundTime( float flSwimSoundTime )
{
	m_flSwimSoundTime = flSwimSoundTime;
}

void CBasePlayer::ShowViewPortPanel( const char * name, bool bShow, KeyValues *data )
{
	CSingleUserRecipientFilter filter( this );
	filter.MakeReliable();

	int count = 0;
	KeyValues *subkey = NULL;

	if ( data )
	{
		subkey = data->GetFirstSubKey();
		while ( subkey )
		{
			count++; subkey = subkey->GetNextKey();
		}

		subkey = data->GetFirstSubKey(); // reset 
	}

	UserMessageBegin( filter, "VGUIMenu" );
		WRITE_STRING( name ); // menu name
		WRITE_BYTE( bShow?1:0 );
		WRITE_BYTE( count );
		
		// write additional data (be careful not more than 192 bytes!)
		while ( subkey )
		{
			WRITE_STRING( subkey->GetName() );
			WRITE_STRING( subkey->GetString() );
			subkey = subkey->GetNextKey();
		}
	MessageEnd();
}


void CBasePlayer::PlayerDeathThink(void)
{
	float flForward;

	SetNextThink( gpGlobals->curtime + 0.1f );

	if (GetFlags() & FL_ONGROUND)
	{
		flForward = GetAbsVelocity().Length() - 20;
		if (flForward <= 0)
		{
			SetAbsVelocity( vec3_origin );
		}
		else
		{
			Vector vecNewVelocity = GetAbsVelocity();
			VectorNormalize( vecNewVelocity );
			vecNewVelocity *= flForward;
			SetAbsVelocity( vecNewVelocity );
		}
	}

	if ( HasWeapons() )
	{
		// we drop the guns here because weapons that have an area effect and can kill their user
		// will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
		// player class sometimes is freed. It's safer to manipulate the weapons once we know
		// we aren't calling into any of their code anymore through the player pointer.
		PackDeadPlayerItems();
	}

	if (GetModelIndex() && (!IsSequenceFinished()) && (m_lifeState == LIFE_DYING))
	{
		StudioFrameAdvance( );

		m_iRespawnFrames++;
		if ( m_iRespawnFrames < 60 )  // animations should be no longer than this
			return;
	}

	if (m_lifeState == LIFE_DYING)
	{
		m_lifeState = LIFE_DEAD;
		m_flDeathAnimTime = gpGlobals->curtime;
	}
	
	StopAnimation();

	IncrementInterpolationFrame();
	m_flPlaybackRate = 0.0;
	
	int fAnyButtonDown = (m_nButtons & ~IN_SCORE);
	
	// Strip out the duck key from this check if it's toggled
	if ( (fAnyButtonDown & IN_DUCK) && GetToggledDuckState())
	{
		fAnyButtonDown &= ~IN_DUCK;
	}

	// wait for all buttons released
	if (m_lifeState == LIFE_DEAD)
	{
		if (fAnyButtonDown)
			return;

		if ( g_pGameRules->FPlayerCanRespawn( this ) )
		{
			m_lifeState = LIFE_RESPAWNABLE;
		}
		
		return;
	}

// if the player has been dead for one second longer than allowed by forcerespawn, 
// forcerespawn isn't on. Send the player off to an intermission camera until they 
// choose to respawn.
	if ( g_pGameRules->IsMultiplayer() && ( gpGlobals->curtime > (m_flDeathTime + DEATH_ANIMATION_TIME) ) && !IsObserver() )
	{
		// go to dead camera. 
		StartObserverMode( m_iObserverLastMode );
	}
	
// wait for any button down,  or mp_forcerespawn is set and the respawn time is up
	if (!fAnyButtonDown 
		&& !( g_pGameRules->IsMultiplayer() && forcerespawn.GetInt() > 0 && (gpGlobals->curtime > (m_flDeathTime + 5))) )
		return;

	m_nButtons = 0;
	m_iRespawnFrames = 0;

	//Msg( "Respawn\n");

	respawn( this, !IsObserver() );// don't copy a corpse if we're in deathcam.
	SetNextThink( TICK_NEVER_THINK );
}

/*

//=========================================================
// StartDeathCam - find an intermission spot and send the
// player off into observer mode
//=========================================================
void CBasePlayer::StartDeathCam( void )
{
	CBaseEntity *pSpot, *pNewSpot;
	int iRand;

	if ( GetViewOffset() == vec3_origin )
	{
		// don't accept subsequent attempts to StartDeathCam()
		return;
	}

	pSpot = gEntList.FindEntityByClassname( NULL, "info_intermission");	

	if ( pSpot )
	{
		// at least one intermission spot in the world.
		iRand = random->RandomInt( 0, 3 );

		while ( iRand > 0 )
		{
			pNewSpot = gEntList.FindEntityByClassname( pSpot, "info_intermission");
			
			if ( pNewSpot )
			{
				pSpot = pNewSpot;
			}

			iRand--;
		}

		CreateCorpse();
		StartObserverMode( pSpot->GetAbsOrigin(), pSpot->GetAbsAngles() );
	}
	else
	{
		// no intermission spot. Push them up in the air, looking down at their corpse
		trace_t tr;

		CreateCorpse();

		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector( 0, 0, 128 ), 
			MASK_PLAYERSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
		QAngle angles;
		VectorAngles( GetAbsOrigin() - tr.endpos, angles );
		StartObserverMode( tr.endpos, angles );
		return;
	}
} */

void CBasePlayer::StopObserverMode()
{
	m_bForcedObserverMode = false;
	m_afPhysicsFlags &= ~PFLAG_OBSERVER;

	if ( m_iObserverMode == OBS_MODE_NONE )
		return;

	if ( m_iObserverMode  > OBS_MODE_DEATHCAM )
	{
		m_iObserverLastMode = m_iObserverMode;
	}

	m_iObserverMode.Set( OBS_MODE_NONE );

	ShowViewPortPanel( "specmenu", false );
	ShowViewPortPanel( "specgui", false );
	ShowViewPortPanel( "overview", false );
}

bool CBasePlayer::StartObserverMode(int mode)
{
	if ( !IsObserver() )
	{
		// set position to last view offset
		SetAbsOrigin( GetAbsOrigin() + GetViewOffset() );
		SetViewOffset( vec3_origin );
	}

	Assert( mode > OBS_MODE_NONE );
	
	m_afPhysicsFlags |= PFLAG_OBSERVER;

	// Holster weapon immediately, to allow it to cleanup
    if ( GetActiveWeapon() )
		GetActiveWeapon()->Holster();

	// clear out the suit message cache so we don't keep chattering
    SetSuitUpdate(NULL, FALSE, 0);

	SetGroundEntity( (CBaseEntity *)NULL );
	
	RemoveFlag( FL_DUCKING );
	
    AddSolidFlags( FSOLID_NOT_SOLID );

	SetObserverMode( mode );

	if ( gpGlobals->eLoadType != MapLoad_Background )
	{
		ShowViewPortPanel( "specgui" , ModeWantsSpectatorGUI(mode) );
	}
	
	// Setup flags
    m_Local.m_iHideHUD = HIDEHUD_HEALTH;
	m_takedamage = DAMAGE_NO;		

	// Become invisible
	AddEffects( EF_NODRAW );		

	m_iHealth = 1;
	m_lifeState = LIFE_DEAD; // Can't be dead, otherwise movement doesn't work right.
	m_flDeathAnimTime = gpGlobals->curtime;
	pl.deadflag = true;

	return true;
}

bool CBasePlayer::SetObserverMode(int mode )
{
	if ( mode < OBS_MODE_NONE || mode >= NUM_OBSERVER_MODES )
		return false;


	// check mp_forcecamera settings for dead players
	if ( mode > OBS_MODE_FIXED && GetTeamNumber() > TEAM_SPECTATOR )
	{
		switch ( mp_forcecamera.GetInt() )
		{
			case OBS_ALLOW_ALL	:	break;	// no restrictions
			case OBS_ALLOW_TEAM :	mode = OBS_MODE_IN_EYE;	break;
			case OBS_ALLOW_NONE :	mode = OBS_MODE_FIXED; break;	// don't allow anything
		}
	}

	if ( m_iObserverMode > OBS_MODE_DEATHCAM )
	{
		// remember mode if we were really spectating before
		m_iObserverLastMode = m_iObserverMode;
	}

	m_iObserverMode = mode;
	
	switch ( mode )
	{
		case OBS_MODE_NONE:
		case OBS_MODE_FIXED :
		case OBS_MODE_DEATHCAM :
			SetFOV( this, 0 );	// Reset FOV
			SetViewOffset( vec3_origin );
			SetMoveType( MOVETYPE_NONE );
			break;

		case OBS_MODE_CHASE :
		case OBS_MODE_IN_EYE :	
			// udpate FOV and viewmodels
			SetObserverTarget( m_hObserverTarget );	
			SetMoveType( MOVETYPE_OBSERVER );
			break;

		//=============================================================================
		// HPE_BEGIN:
		// [menglish] Added freeze cam to the setter.  Uses same setup as the roaming mode
		//=============================================================================

		case OBS_MODE_ROAMING :
		case OBS_MODE_FREEZECAM :
			SetFOV( this, 0 );	// Reset FOV
			SetObserverTarget( m_hObserverTarget );
			SetViewOffset( vec3_origin );
			SetMoveType( MOVETYPE_OBSERVER );
			break;

		//=============================================================================
		// HPE_END
		//=============================================================================
	}

	CheckObserverSettings();

	return true;	
}

int CBasePlayer::GetObserverMode()
{
	return m_iObserverMode;
}

void CBasePlayer::ForceObserverMode(int mode)
{
	int tempMode = OBS_MODE_ROAMING;

	if ( m_iObserverMode == mode )
		return;

	// don't change last mode if already in forced mode

	if ( m_bForcedObserverMode )
	{
		tempMode = m_iObserverLastMode;
	}
	
	SetObserverMode( mode );

	if ( m_bForcedObserverMode )
	{
		m_iObserverLastMode = tempMode;
	}

	m_bForcedObserverMode = true;
}

void CBasePlayer::CheckObserverSettings()
{
	// check if we are in forced mode and may go back to old mode
	if ( m_bForcedObserverMode )
	{
		CBaseEntity * target = m_hObserverTarget;

		if ( !IsValidObserverTarget(target) )
		{
			// if old target is still invalid, try to find valid one
			target = FindNextObserverTarget( false );
		}

		if ( target )
		{
				// we found a valid target
				m_bForcedObserverMode = false;	// disable force mode
				SetObserverMode( m_iObserverLastMode ); // switch to last mode
				SetObserverTarget( target ); // goto target
				
				// TODO check for HUD icons
				return;
		}
		else
		{
			// else stay in forced mode, no changes
			return;
		}
	}

	// make sure our last mode is valid
	if ( m_iObserverLastMode < OBS_MODE_FIXED )
	{
		m_iObserverLastMode = OBS_MODE_ROAMING;
	}

	// check if our spectating target is still a valid one
	
	if (  m_iObserverMode == OBS_MODE_IN_EYE || m_iObserverMode == OBS_MODE_CHASE || m_iObserverMode == OBS_MODE_FIXED )
	{
		ValidateCurrentObserverTarget();
				
		CBasePlayer *target = ToBasePlayer( m_hObserverTarget.Get() );

		// for ineye mode we have to copy several data to see exactly the same 

		if ( target && m_iObserverMode == OBS_MODE_IN_EYE )
		{
			int flagMask =	FL_ONGROUND | FL_DUCKING ;

			int flags = target->GetFlags() & flagMask;

			if ( (GetFlags() & flagMask) != flags )
			{
				flags |= GetFlags() & (~flagMask); // keep other flags
				ClearFlags();
				AddFlag( flags );
			}

			if ( target->GetViewOffset() != GetViewOffset()	)
			{
				SetViewOffset( target->GetViewOffset() );
			}
		}

		// Update the fog.
		if ( target )
		{
			if ( target->m_Local.m_PlayerFog.m_hCtrl.Get() != m_Local.m_PlayerFog.m_hCtrl.Get() )
			{
				m_Local.m_PlayerFog.m_hCtrl.Set( target->m_Local.m_PlayerFog.m_hCtrl.Get() );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::ValidateCurrentObserverTarget( void )
{
	if ( !IsValidObserverTarget( m_hObserverTarget.Get() ) )
	{
		// our target is not valid, try to find new target
		CBaseEntity * target = FindNextObserverTarget( false );
		if ( target )
		{
			// switch to new valid target
			SetObserverTarget( target );	
		}
		else
		{
			// couldn't find new target, switch to temporary mode
			if ( mp_forcecamera.GetInt() == OBS_ALLOW_ALL )
			{
				// let player roam around
				ForceObserverMode( OBS_MODE_ROAMING );
			}
			else
			{
				// fix player view right where it is
				ForceObserverMode( OBS_MODE_FIXED );
				m_hObserverTarget.Set( NULL ); // no traget to follow
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::AttemptToExitFreezeCam( void )
{
	StartObserverMode( OBS_MODE_DEATHCAM );
}

bool CBasePlayer::StartReplayMode( float fDelay, float fDuration, int iEntity )
{
	if ( ( sv_maxreplay == NULL ) || ( sv_maxreplay->GetFloat() <= 0 ) )
		return false;

	m_fDelay = fDelay;
	m_fReplayEnd = gpGlobals->curtime + fDuration;
	m_iReplayEntity = iEntity;

	return true;
}

void CBasePlayer::StopReplayMode()
{
	m_fDelay = 0.0f;
	m_fReplayEnd = -1;
	m_iReplayEntity = 0;
}

int	CBasePlayer::GetDelayTicks()
{
	if ( m_fReplayEnd > gpGlobals->curtime )
	{
		return TIME_TO_TICKS( m_fDelay );
	}
	else
	{
		if ( m_fDelay > 0.0f )
			StopReplayMode();

		return 0;
	}
}

int CBasePlayer::GetReplayEntity()
{
	return m_iReplayEntity;
}

CBaseEntity * CBasePlayer::GetObserverTarget()
{
	return m_hObserverTarget.Get();
}

void CBasePlayer::ObserverUse( bool bIsPressed )
{
#ifndef _XBOX
	if ( !HLTVDirector()->IsActive() )
		return;

	if ( GetTeamNumber() != TEAM_SPECTATOR )
		return;	// only pure spectators can play cameraman

	if ( !bIsPressed )
		return;

	bool bIsHLTV = HLTVDirector()->IsActive();

	if ( bIsHLTV )
	{
		int iCameraManIndex = HLTVDirector()->GetCameraMan();

		if ( iCameraManIndex == 0 )
		{
			// turn camera on
			HLTVDirector()->SetCameraMan( entindex() );
		}
		else if ( iCameraManIndex == entindex() )
		{
			// turn camera off
			HLTVDirector()->SetCameraMan( 0 );
		}
		else
		{
			ClientPrint( this, HUD_PRINTTALK, "Camera in use by other player." );	
		}
	}

	/* UTIL_SayText( "Spectator can not USE anything", this );

	Vector dir,end;
	Vector start = GetAbsOrigin();

	AngleVectors( GetAbsAngles(), &dir );
	VectorNormalize( dir );

	VectorMA( start, 32.0f, dir, end );

	trace_t	tr;
	UTIL_TraceLine( start, end, MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );

	if ( tr.fraction == 1.0f )
		return;	// no obstacles in spectators way

	VectorMA( start, 128.0f, dir, end );

	Ray_t ray;
	ray.Init( end, start, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );

	UTIL_TraceRay( ray, MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );

	if ( tr.startsolid || tr.allsolid )
		return;

	SetAbsOrigin( tr.endpos ); */
#endif
}

void CBasePlayer::JumptoPosition(const Vector &origin, const QAngle &angles)
{
	SetAbsOrigin( origin );
	SetAbsVelocity( vec3_origin );	// stop movement
	SetLocalAngles( angles );
	SnapEyeAngles( angles );
}

bool CBasePlayer::SetObserverTarget(CBaseEntity *target)
{
	if ( !IsValidObserverTarget( target ) )
		return false;
	
	// set new target
	m_hObserverTarget.Set( target ); 

	// reset fov to default
	SetFOV( this, 0 );	
	
	if ( m_iObserverMode == OBS_MODE_ROAMING )
	{
		Vector	dir, end;
		Vector	start = target->EyePosition();
		
		AngleVectors( target->EyeAngles(), &dir );
		VectorNormalize( dir );
		VectorMA( start, -64.0f, dir, end );

		Ray_t ray;
		ray.Init( start, end, VEC_DUCK_HULL_MIN	, VEC_DUCK_HULL_MAX );

		trace_t	tr;
		UTIL_TraceRay( ray, MASK_PLAYERSOLID, target, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );

		JumptoPosition( tr.endpos, target->EyeAngles() );
	}
	
	return true;
}

bool CBasePlayer::IsValidObserverTarget(CBaseEntity * target)
{
	if ( target == NULL )
		return false;

	// MOD AUTHORS: Add checks on target here or in derived method

	if ( !target->IsPlayer() )	// only track players
		return false;

	CBasePlayer * player = ToBasePlayer( target );

	/* Don't spec observers or players who haven't picked a class yet
 	if ( player->IsObserver() )
		return false;	*/

	if( player == this )
		return false; // We can't observe ourselves.

	if ( player->IsEffectActive( EF_NODRAW ) ) // don't watch invisible players
		return false;

	if ( player->m_lifeState == LIFE_RESPAWNABLE ) // target is dead, waiting for respawn
		return false;

	if ( player->m_lifeState == LIFE_DEAD || player->m_lifeState == LIFE_DYING )
	{
		if ( (player->m_flDeathTime + DEATH_ANIMATION_TIME ) < gpGlobals->curtime )
		{
			return false;	// allow watching until 3 seconds after death to see death animation
		}
	}
		
	// check forcecamera settings for active players
	if ( GetTeamNumber() != TEAM_SPECTATOR )
	{
		switch ( mp_forcecamera.GetInt() )	
		{
			case OBS_ALLOW_ALL	:	break;
			case OBS_ALLOW_TEAM :	if ( GetTeamNumber() != target->GetTeamNumber() )
										 return false;
									break;
			case OBS_ALLOW_NONE :	return false;
		}
	}
	
	return true;	// passed all test
}

int CBasePlayer::GetNextObserverSearchStartPoint( bool bReverse )
{
	int iDir = bReverse ? -1 : 1; 

	int startIndex;

	if ( m_hObserverTarget )
	{
		// start using last followed player
		startIndex = m_hObserverTarget->entindex();	
	}
	else
	{
		// start using own player index
		startIndex = this->entindex();
	}

	startIndex += iDir;
	if (startIndex > gpGlobals->maxClients)
		startIndex = 1;
	else if (startIndex < 1)
		startIndex = gpGlobals->maxClients;

	return startIndex;
}

CBaseEntity * CBasePlayer::FindNextObserverTarget(bool bReverse)
{
	// MOD AUTHORS: Modify the logic of this function if you want to restrict the observer to watching
	//				only a subset of the players. e.g. Make it check the target's team.

/*	if ( m_flNextFollowTime && m_flNextFollowTime > gpGlobals->time )
	{
		return;
	} 

	m_flNextFollowTime = gpGlobals->time + 0.25;
	*/	// TODO move outside this function

	int startIndex = GetNextObserverSearchStartPoint( bReverse );
	
	int	currentIndex = startIndex;
	int iDir = bReverse ? -1 : 1; 
	
	do
	{
		CBaseEntity * nextTarget = UTIL_PlayerByIndex( currentIndex );

		if ( IsValidObserverTarget( nextTarget ) )
		{
			return nextTarget;	// found next valid player
		}

		currentIndex += iDir;

		// Loop through the clients
  		if (currentIndex > gpGlobals->maxClients)
  			currentIndex = 1;
		else if (currentIndex < 1)
  			currentIndex = gpGlobals->maxClients;

	} while ( currentIndex != startIndex );
		
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this object can be +used by the player
//-----------------------------------------------------------------------------
bool CBasePlayer::IsUseableEntity( CBaseEntity *pEntity, unsigned int requiredCaps )
{
	if ( pEntity )
	{
		int caps = pEntity->ObjectCaps();
		if ( caps & (FCAP_IMPULSE_USE|FCAP_CONTINUOUS_USE|FCAP_ONOFF_USE|FCAP_DIRECTIONAL_USE) )
		{
			if ( (caps & requiredCaps) == requiredCaps )
			{
				return true;
			}
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBasePlayer::CanPickupObject( CBaseEntity *pObject, float massLimit, float sizeLimit )
{
	// UNDONE: Make this virtual and move to HL2 player
#ifdef HL2_DLL
	//Must be valid
	if ( pObject == NULL )
		return false;

	//Must move with physics
	if ( pObject->GetMoveType() != MOVETYPE_VPHYSICS )
		return false;

	IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	int count = pObject->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );

	//Must have a physics object
	if (!count)
		return false;

	float objectMass = 0;
	bool checkEnable = false;
	for ( int i = 0; i < count; i++ )
	{
		objectMass += pList[i]->GetMass();
		if ( !pList[i]->IsMoveable() )
		{
			checkEnable = true;
		}
		if ( pList[i]->GetGameFlags() & FVPHYSICS_NO_PLAYER_PICKUP )
			return false;
		if ( pList[i]->IsHinged() )
			return false;
	}


	//Msg( "Target mass: %f\n", pPhys->GetMass() );

	//Must be under our threshold weight
	if ( massLimit > 0 && objectMass > massLimit )
		return false;

	if ( checkEnable )
	{
		// Allowing picking up of bouncebombs.
		CBounceBomb *pBomb = dynamic_cast<CBounceBomb*>(pObject);
		if( pBomb )
			return true;

		// Allow pickup of phys props that are motion enabled on player pickup
		CPhysicsProp *pProp = dynamic_cast<CPhysicsProp*>(pObject);
		CPhysBox *pBox = dynamic_cast<CPhysBox*>(pObject);
		if ( !pProp && !pBox )
			return false;

		if ( pProp && !(pProp->HasSpawnFlags( SF_PHYSPROP_ENABLE_ON_PHYSCANNON )) )
			return false;

		if ( pBox && !(pBox->HasSpawnFlags( SF_PHYSBOX_ENABLE_ON_PHYSCANNON )) )
			return false;
	}

	if ( sizeLimit > 0 )
	{
		const Vector &size = pObject->CollisionProp()->OBBSize();
		if ( size.x > sizeLimit || size.y > sizeLimit || size.z > sizeLimit )
			return false;
	}

	return true;
#else
	return false;
#endif
}

float CBasePlayer::GetHeldObjectMass( IPhysicsObject *pHeldObject )
{
	return 0;
}

CBaseEntity	*CBasePlayer::GetHeldObject( void )
{
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:	Server side of jumping rules.  Most jumping logic is already
//			handled in shared gamemovement code.  Put stuff here that should
//			only be done server side.
//-----------------------------------------------------------------------------
void CBasePlayer::Jump()
{
}

void CBasePlayer::Duck( )
{
	if (m_nButtons & IN_DUCK) 
	{
		if ( m_Activity != ACT_LEAP )
		{
			SetAnimation( PLAYER_WALK );
		}
	}
}

//
// ID's player as such.
//
Class_T  CBasePlayer::Classify ( void )
{
	return CLASS_PLAYER;
}


void CBasePlayer::ResetFragCount()
{
	m_iFrags = 0;
	pl.frags = m_iFrags;
}

void CBasePlayer::IncrementFragCount( int nCount )
{
	m_iFrags += nCount;
	pl.frags = m_iFrags;
}

void CBasePlayer::ResetDeathCount()
{
	m_iDeaths = 0;
	pl.deaths = m_iDeaths;
}

void CBasePlayer::IncrementDeathCount( int nCount )
{
	m_iDeaths += nCount;
	pl.deaths = m_iDeaths;
}

void CBasePlayer::AddPoints( int score, bool bAllowNegativeScore )
{
	// Positive score always adds
	if ( score < 0 )
	{
		if ( !bAllowNegativeScore )
		{
			if ( m_iFrags < 0 )		// Can't go more negative
				return;
			
			if ( -score > m_iFrags )	// Will this go negative?
			{
				score = -m_iFrags;		// Sum will be 0
			}
		}
	}

	m_iFrags += score;
	pl.frags = m_iFrags;
}

void CBasePlayer::AddPointsToTeam( int score, bool bAllowNegativeScore )
{
	if ( GetTeam() )
	{
		GetTeam()->AddScore( score );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CBasePlayer::GetCommandContextCount( void ) const
{
	return m_CommandContext.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : CCommandContext
//-----------------------------------------------------------------------------
CCommandContext *CBasePlayer::GetCommandContext( int index )
{
	if ( index < 0 || index >= m_CommandContext.Count() )
		return NULL;

	return &m_CommandContext[ index ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCommandContext	*CBasePlayer::AllocCommandContext( void )
{
	int idx = m_CommandContext.AddToTail();
	if ( m_CommandContext.Count() > 1000 )
	{
		Assert( 0 );
	}
	return &m_CommandContext[ idx ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//-----------------------------------------------------------------------------
void CBasePlayer::RemoveCommandContext( int index )
{
	m_CommandContext.Remove( index );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::RemoveAllCommandContexts()
{
	m_CommandContext.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Removes all existing contexts, but leaves the last one around ( or creates it if it doesn't exist -- which would be a bug )
//-----------------------------------------------------------------------------
CCommandContext *CBasePlayer::RemoveAllCommandContextsExceptNewest( void )
{
	int count = m_CommandContext.Count();
	int toRemove = count - 1;
	if ( toRemove > 0 )
	{
		m_CommandContext.RemoveMultiple( 0, toRemove );
	}

	if ( !m_CommandContext.Count() )
	{
		Assert( 0 );
		CCommandContext *ctx = AllocCommandContext();
		Q_memset( ctx, 0, sizeof( *ctx ) );
	}

	return &m_CommandContext[ 0 ];
}

//-----------------------------------------------------------------------------
// Purpose: Replaces the first nCommands CUserCmds in the context with the ones passed in -- this is used to help meter out CUserCmds over the number of simulation ticks on the server
//-----------------------------------------------------------------------------
void CBasePlayer::ReplaceContextCommands( CCommandContext *ctx, CUserCmd *pCommands, int nCommands )
{
	// Blow away all of the commands
	ctx->cmds.RemoveAll();

	ctx->numcmds			= nCommands;
	ctx->totalcmds			= nCommands;
	ctx->dropped_packets	= 0; // meaningless in this context

	// Add them in so the most recent is at slot 0
	for ( int i = nCommands - 1; i >= 0; --i )
	{
		ctx->cmds.AddToTail( pCommands[ i ] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Determine how much time we will be running this frame
// Output : float
//-----------------------------------------------------------------------------
int CBasePlayer::DetermineSimulationTicks( void )
{
	int command_context_count = GetCommandContextCount();

	int context_number;

	int simulation_ticks = 0;

	// Determine how much time we will be running this frame and fixup player clock as needed
	for ( context_number = 0; context_number < command_context_count; context_number++ )
	{
		CCommandContext const *ctx = GetCommandContext( context_number );
		Assert( ctx );
		Assert( ctx->numcmds > 0 );
		Assert( ctx->dropped_packets >= 0 );

		// Determine how long it will take to run those packets
		simulation_ticks += ctx->numcmds + ctx->dropped_packets;
	}

	return simulation_ticks;
}

// 2 ticks ahead or behind current clock means we need to fix clock on client
static ConVar sv_clockcorrection_msecs( "sv_clockcorrection_msecs", "60", 0, "The server tries to keep each player's m_nTickBase withing this many msecs of the server absolute tickcount" );
static ConVar sv_playerperfhistorycount( "sv_playerperfhistorycount", "60", 0, "Number of samples to maintain in player perf history", true, 1.0f, true, 128.0 );

//-----------------------------------------------------------------------------
// Purpose: Based upon amount of time in simulation time, adjust m_nTickBase so that
//  we just end at the end of the current frame (so the player is basically on clock
//  with the server)
// Input  : simulation_ticks - 
//-----------------------------------------------------------------------------
void CBasePlayer::AdjustPlayerTimeBase( int simulation_ticks )
{
	Assert( simulation_ticks >= 0 );
	if ( simulation_ticks < 0 )
		return;

	CPlayerSimInfo *pi = NULL;
	if ( sv_playerperfhistorycount.GetInt() > 0 )
	{
		while ( m_vecPlayerSimInfo.Count() > sv_playerperfhistorycount.GetInt() )
		{
			m_vecPlayerSimInfo.Remove( m_vecPlayerSimInfo.Head() );
		}

		pi = &m_vecPlayerSimInfo[ m_vecPlayerSimInfo.AddToTail() ];
	}

	// Start in the past so that we get to the sv.time that we'll hit at the end of the
	//  frame, just as we process the final command
	
	if ( gpGlobals->maxClients == 1 )
	{
		// set TickBase so that player simulation tick matches gpGlobals->tickcount after
		// all commands have been executed
		m_nTickBase = gpGlobals->tickcount - simulation_ticks + gpGlobals->simTicksThisFrame;
	}
	else // multiplayer
	{
		float flCorrectionSeconds = clamp( sv_clockcorrection_msecs.GetFloat() / 1000.0f, 0.0f, 1.0f );
		int nCorrectionTicks = TIME_TO_TICKS( flCorrectionSeconds );

		// Set the target tick flCorrectionSeconds (rounded to ticks) ahead in the future. this way the client can
		//  alternate around this target tick without getting smaller than gpGlobals->tickcount.
		// After running the commands simulation time should be equal or after current gpGlobals->tickcount, 
		//  otherwise the simulation time drops out of the client side interpolated var history window.

		int	nIdealFinalTick = gpGlobals->tickcount + nCorrectionTicks;

		int nEstimatedFinalTick = m_nTickBase + simulation_ticks;
		
		// If client gets ahead of this, we'll need to correct
		int	 too_fast_limit = nIdealFinalTick + nCorrectionTicks;
		// If client falls behind this, we'll also need to correct
		int	 too_slow_limit = nIdealFinalTick - nCorrectionTicks;
			
		// See if we are too fast
		if ( nEstimatedFinalTick > too_fast_limit ||
			 nEstimatedFinalTick < too_slow_limit )
		{
			int nCorrectedTick = nIdealFinalTick - simulation_ticks + gpGlobals->simTicksThisFrame;

			if ( pi )
			{
				pi->m_nTicksCorrected = nCorrectionTicks;
			}

			m_nTickBase = nCorrectedTick;
		}
	}

	if ( pi )
	{
		pi->m_flFinalSimulationTime = TICKS_TO_TIME( m_nTickBase + simulation_ticks + gpGlobals->simTicksThisFrame );
	}
}

void CBasePlayer::RunNullCommand( void )
{
	CUserCmd cmd;	// NULL command

	// Store off the globals.. they're gonna get whacked
	float flOldFrametime = gpGlobals->frametime;
	float flOldCurtime = gpGlobals->curtime;

	pl.fixangle = FIXANGLE_NONE;

	if ( IsReplay() )
	{
		cmd.viewangles = QAngle( 0, 0, 0 );
	}
	else
	{
		cmd.viewangles = EyeAngles();
	}

	float flTimeBase = gpGlobals->curtime;
	SetTimeBase( flTimeBase );

	MoveHelperServer()->SetHost( this );
	PlayerRunCommand( &cmd, MoveHelperServer() );

	// save off the last good usercmd
	SetLastUserCommand( cmd );

	// Restore the globals..
	gpGlobals->frametime = flOldFrametime;
	gpGlobals->curtime = flOldCurtime;

	MoveHelperServer()->SetHost( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Note, don't chain to BaseClass::PhysicsSimulate
//-----------------------------------------------------------------------------
void CBasePlayer::PhysicsSimulate( void )
{
	VPROF_BUDGET( "CBasePlayer::PhysicsSimulate", VPROF_BUDGETGROUP_PLAYER );

	// If we've got a moveparent, we must simulate that first.
	CBaseEntity *pMoveParent = GetMoveParent();
	if (pMoveParent)
	{
		pMoveParent->PhysicsSimulate();
	}

	// Make sure not to simulate this guy twice per frame
	if ( m_nSimulationTick == gpGlobals->tickcount )
	{
		return;
	}
	
	m_nSimulationTick = gpGlobals->tickcount;

	// See how many CUserCmds are queued up for running
	int simulation_ticks = DetermineSimulationTicks();

	// If some time will elapse, make sure our clock (m_nTickBase) starts at the correct time
	if ( simulation_ticks > 0 )
	{
		AdjustPlayerTimeBase( simulation_ticks );
	}

	if ( IsHLTV() || IsReplay() )
	{
		// just run a single, empty command to make sure 
		// all PreThink/PostThink functions are called as usual
		Assert ( GetCommandContextCount() == 0 );
		RunNullCommand();
		RemoveAllCommandContexts();
		return;
	}

	// Store off true server timestamps
	float savetime		= gpGlobals->curtime;
	float saveframetime = gpGlobals->frametime;

	int command_context_count = GetCommandContextCount();
	

	// Build a list of all available commands
	CUtlVector< CUserCmd >	vecAvailCommands;

	// Contexts go from oldest to newest
	for ( int context_number = 0; context_number < command_context_count; context_number++ )
	{
		// Get oldest ( newer are added to tail )
		CCommandContext *ctx = GetCommandContext( context_number );
		if ( !ShouldRunCommandsInContext( ctx ) )
			continue;

		if ( !ctx->cmds.Count() )
			continue;

		int numbackup = ctx->totalcmds - ctx->numcmds;

		// If we haven't dropped too many packets, then run some commands
		if ( ctx->dropped_packets < 24 )                
		{
			int droppedcmds = ctx->dropped_packets;

			// run the last known cmd for each dropped cmd we don't have a backup for
			while ( droppedcmds > numbackup )
			{
				m_LastCmd.tick_count++;
				vecAvailCommands.AddToTail( m_LastCmd );
				droppedcmds--;
			}

			// Now run the "history" commands if we still have dropped packets
			while ( droppedcmds > 0 )
			{
				int cmdnum = ctx->numcmds + droppedcmds - 1;
				vecAvailCommands.AddToTail( ctx->cmds[cmdnum] );
				droppedcmds--;
			}
		}

		// Now run any new command(s).  Go backward because the most recent command is at index 0.
		for ( int i = ctx->numcmds - 1; i >= 0; i-- )
		{
			vecAvailCommands.AddToTail( ctx->cmds[i] );
		}

		// Save off the last good command in case we drop > numbackup packets and need to rerun them
		//  we'll use this to "guess" at what was in the missing packets
		m_LastCmd = ctx->cmds[ CMD_MOSTRECENT ];
	}

	// gpGlobals->simTicksThisFrame == number of ticks remaining to be run, so we should take the last N CUserCmds and postpone them until the next frame

	// If we're running multiple ticks this frame, don't peel off all of the commands, spread them out over
	// the server ticks.  Use blocks of two in alternate ticks
	int commandLimit = CBaseEntity::IsSimulatingOnAlternateTicks() ? 2 : 1;
	int commandsToRun = vecAvailCommands.Count();
	if ( gpGlobals->simTicksThisFrame >= commandLimit && vecAvailCommands.Count() > commandLimit )
	{
		int commandsToRollOver = MIN( vecAvailCommands.Count(), ( gpGlobals->simTicksThisFrame - 1 ) );
		commandsToRun = vecAvailCommands.Count() - commandsToRollOver;
		Assert( commandsToRun >= 0 );
		// Clear all contexts except the last one
		if ( commandsToRollOver > 0 )
		{
			CCommandContext *ctx = RemoveAllCommandContextsExceptNewest();
			ReplaceContextCommands( ctx, &vecAvailCommands[ commandsToRun ], commandsToRollOver );
		}
		else
		{
			// Clear all contexts
			RemoveAllCommandContexts();
		}
	}
	else
	{
		// Clear all contexts
		RemoveAllCommandContexts();
	}

	float vphysicsArrivalTime = TICK_INTERVAL;

#ifdef _DEBUG
	if ( sv_player_net_suppress_usercommands.GetBool() )
	{
		commandsToRun = 0;
	}
#endif // _DEBUG

	int numUsrCmdProcessTicksMax = sv_maxusrcmdprocessticks.GetInt();
	if ( gpGlobals->maxClients != 1 && numUsrCmdProcessTicksMax ) // don't apply this filter in SP games
	{
		// Grant the client some time buffer to execute user commands
		m_flMovementTimeForUserCmdProcessingRemaining += TICK_INTERVAL;

		// but never accumulate more than N ticks
		if ( m_flMovementTimeForUserCmdProcessingRemaining > numUsrCmdProcessTicksMax * TICK_INTERVAL )
			m_flMovementTimeForUserCmdProcessingRemaining = numUsrCmdProcessTicksMax * TICK_INTERVAL;
	}
	else
	{
		// Otherwise we don't care to track time
		m_flMovementTimeForUserCmdProcessingRemaining = FLT_MAX;
	}

	// Now run the commands
	if ( commandsToRun > 0 )
	{
		m_flLastUserCommandTime = savetime;

		MoveHelperServer()->SetHost( this );

		// Suppress predicted events, etc.
		if ( IsPredictingWeapons() )
		{
			IPredictionSystem::SuppressHostEvents( this );
		}

		for ( int i = 0; i < commandsToRun; ++i )
		{
			PlayerRunCommand( &vecAvailCommands[ i ], MoveHelperServer() );

			// Update our vphysics object.
			if ( m_pPhysicsController )
			{
				VPROF( "CBasePlayer::PhysicsSimulate-UpdateVPhysicsPosition" );
				// If simulating at 2 * TICK_INTERVAL, add an extra TICK_INTERVAL to position arrival computation
				UpdateVPhysicsPosition( m_vNewVPhysicsPosition, m_vNewVPhysicsVelocity, vphysicsArrivalTime );
				vphysicsArrivalTime += TICK_INTERVAL;
			}
		}

		// Always reset after running commands
		IPredictionSystem::SuppressHostEvents( NULL );

		MoveHelperServer()->SetHost( NULL );

		// Copy in final origin from simulation
		CPlayerSimInfo *pi = NULL;
		if ( m_vecPlayerSimInfo.Count() > 0 )
		{
			pi = &m_vecPlayerSimInfo[ m_vecPlayerSimInfo.Tail() ];
			pi->m_flTime = Plat_FloatTime();
			pi->m_vecAbsOrigin = GetAbsOrigin();
			pi->m_flGameSimulationTime = gpGlobals->curtime;
			pi->m_nNumCmds = commandsToRun;
		}
	}

	// Restore the true server clock
	// FIXME:  Should this occur after simulation of children so
	//  that they are in the timespace of the player?
	gpGlobals->curtime		= savetime;
	gpGlobals->frametime	= saveframetime;	

// 	// Kick the player if they haven't sent a user command in awhile in order to prevent clients
// 	// from using packet-level manipulation to mess with gamestate.  Not sending usercommands seems
// 	// to have all kinds of bad effects, such as stalling a bunch of Think()'s and gamestate handling.
// 	// An example from TF: A medic stops sending commands after deploying an uber on another player.
// 	// As a result, invuln is permanently on the heal target because the maintenance code is stalled.
// 	if ( GetTimeSinceLastUserCommand() > player_usercommand_timeout.GetFloat() )
// 	{
// 		// If they have an active netchan, they're almost certainly messing with usercommands?
// 		INetChannelInfo *pNetChanInfo = engine->GetPlayerNetInfo( entindex() );
// 		if ( pNetChanInfo && pNetChanInfo->GetTimeSinceLastReceived() < 5.f )
// 		{
// 			engine->ServerCommand( UTIL_VarArgs( "kickid %d %s\n", GetUserID(), "UserCommand Timeout" ) );
// 		}
// 	}
}

unsigned int CBasePlayer::PhysicsSolidMaskForEntity() const
{
	return MASK_PLAYERSOLID;
}

//-----------------------------------------------------------------------------
// Purpose: This will force usercmd processing to actually consume commands even if the global tick counter isn't incrementing
//-----------------------------------------------------------------------------
void CBasePlayer::ForceSimulation()
{
	m_nSimulationTick = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *buf - 
//			totalcmds - 
//			dropped_packets - 
//			ignore - 
//			paused - 
// Output : float -- Time in seconds of last movement command
//-----------------------------------------------------------------------------
void CBasePlayer::ProcessUsercmds( CUserCmd *cmds, int numcmds, int totalcmds,
	int dropped_packets, bool paused )
{
	CCommandContext *ctx = AllocCommandContext();
	Assert( ctx );

	int i;
	for ( i = totalcmds - 1; i >= 0; i-- )
	{
		CUserCmd *pCmd = &cmds[totalcmds - 1 - i];

		// Validate values
		if ( !IsUserCmdDataValid( pCmd ) )
		{
			pCmd->MakeInert();
		}

		ctx->cmds.AddToTail( *pCmd );
	}
	ctx->numcmds			= numcmds;
	ctx->totalcmds			= totalcmds,
	ctx->dropped_packets	= dropped_packets;
	ctx->paused				= paused;
		
	// If the server is paused, zero out motion,buttons,view changes
	if ( ctx->paused )
	{
		bool clear_angles = true;

		// If no clipping and cheats enabled and sv_noclipduringpause enabled, then don't zero out movement part of CUserCmd
		if ( GetMoveType() == MOVETYPE_NOCLIP &&
			sv_cheats->GetBool() && 
			sv_noclipduringpause.GetBool() )
		{
			clear_angles = false;
		}

		for ( i = 0; i < ctx->numcmds; i++ )
		{
			ctx->cmds[ i ].buttons = 0;
			if ( clear_angles )
			{
				ctx->cmds[ i ].forwardmove = 0;
				ctx->cmds[ i ].sidemove = 0;
				ctx->cmds[ i ].upmove = 0;
				VectorCopy ( pl.v_angle, ctx->cmds[ i ].viewangles );
			}
		}

		ctx->dropped_packets = 0;
	}

	// Set global pause state for this player
	m_bGamePaused = paused;

	if ( paused )
	{
		ForceSimulation();
		// Just run the commands right away if paused
		PhysicsSimulate();
	}

	if ( sv_playerperfhistorycount.GetInt() > 0 )
	{
		CPlayerCmdInfo pi;
		pi.m_flTime = Plat_FloatTime();
		pi.m_nDroppedPackets = dropped_packets;
		pi.m_nNumCmds = numcmds;
	
		while ( m_vecPlayerCmdInfo.Count() >= sv_playerperfhistorycount.GetInt() )
		{
			m_vecPlayerCmdInfo.Remove( m_vecPlayerCmdInfo.Head() );
		}

		m_vecPlayerCmdInfo.AddToTail( pi );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check that command values are reasonable
//-----------------------------------------------------------------------------
bool CBasePlayer::IsUserCmdDataValid( CUserCmd *pCmd )
{
	if ( IsBot() || IsFakeClient() )
		return true;

	// Maximum difference between client's and server's tick_count
	const int nCmdMaxTickDelta = ( 1.f / gpGlobals->interval_per_tick ) * 2.5f;
	const int nMinDelta = Max( 0, gpGlobals->tickcount - nCmdMaxTickDelta );
	const int nMaxDelta = gpGlobals->tickcount + nCmdMaxTickDelta;

	bool bValid = ( pCmd->tick_count >= nMinDelta && pCmd->tick_count < nMaxDelta ) &&
				  // Prevent clients from sending invalid view angles to try to get leaf server code to crash
				  ( pCmd->viewangles.IsValid() && IsEntityQAngleReasonable( pCmd->viewangles ) ) &&
				  // Movement ranges
				  ( IsFinite( pCmd->forwardmove ) && IsEntityCoordinateReasonable( pCmd->forwardmove ) ) &&
				  ( IsFinite( pCmd->sidemove ) && IsEntityCoordinateReasonable( pCmd->sidemove ) ) &&
				  ( IsFinite( pCmd->upmove ) && IsEntityCoordinateReasonable( pCmd->upmove ) );

	int nWarningLevel = sv_player_display_usercommand_errors.GetInt();
	if ( !bValid && nWarningLevel > 0 )
	{
		DevMsg( "UserCommand out-of-range for userid %i\n", GetUserID() );

		if ( nWarningLevel == 2 )
		{
			DevMsg( " tick_count: %i\n viewangles: %5.2f %5.2f %5.2f \n forward: %5.2f \n side: \t%5.2f \n up: \t%5.2f\n",
					pCmd->tick_count, 
					pCmd->viewangles.x,
					pCmd->viewangles.y,
					pCmd->viewangles.x,
					pCmd->forwardmove,
					pCmd->sidemove,
					pCmd->upmove );
		}
	}

	return bValid;
}

void CBasePlayer::DumpPerfToRecipient( CBasePlayer *pRecipient, int nMaxRecords )
{
	if ( !pRecipient )
		return;

	char buf[ 256 ] = { 0 };
	int curpos = 0;

	int nDumped = 0;
	Vector prevo( 0, 0, 0 );
	float prevt = 0.0f;

	for ( int i = m_vecPlayerSimInfo.Tail(); i != m_vecPlayerSimInfo.InvalidIndex() ; i = m_vecPlayerSimInfo.Previous( i ) )
	{
		const CPlayerSimInfo *pi = &m_vecPlayerSimInfo[ i ];

		float vel = 0.0f;

		// Note we're walking from newest backward
		float dt = prevt - pi->m_flFinalSimulationTime;
		if ( nDumped > 0 && dt > 0.0f )
		{
			Vector d = pi->m_vecAbsOrigin - prevo;
			vel = d.Length() / dt;
		}

		char line[ 128 ];
		int len = Q_snprintf( line, sizeof( line ), "%.3f %d %d %.3f %.3f vel %.2f\n",
			pi->m_flTime,
			pi->m_nNumCmds,
			pi->m_nTicksCorrected,
			pi->m_flFinalSimulationTime,
			pi->m_flGameSimulationTime,
			vel );

		if ( curpos + len > 200 )
		{
			ClientPrint( pRecipient, HUD_PRINTCONSOLE, (char const *)buf );
			buf[ 0 ] = 0;
			curpos = 0;
		}

		Q_strncpy( &buf[ curpos ], line, sizeof( buf ) - curpos );
		curpos += len;

		++nDumped;
		if ( nMaxRecords != -1 && nDumped >= nMaxRecords )
			break;

		prevo = pi->m_vecAbsOrigin;
		prevt = pi->m_flFinalSimulationTime;
	}

	if ( curpos > 0 )
	{
		ClientPrint( pRecipient, HUD_PRINTCONSOLE, buf );
	}

	nDumped = 0;
	curpos = 0;

	for ( int i = m_vecPlayerCmdInfo.Tail(); i != m_vecPlayerCmdInfo.InvalidIndex() ; i = m_vecPlayerCmdInfo.Previous( i ) )
	{
		const CPlayerCmdInfo *pi = &m_vecPlayerCmdInfo[ i ];

		char line[ 128 ];
		int len = Q_snprintf( line, sizeof( line ), "%.3f %d %d\n",
			pi->m_flTime,
			pi->m_nNumCmds,
			pi->m_nDroppedPackets );

		if ( curpos + len > 200 )
		{
			ClientPrint( pRecipient, HUD_PRINTCONSOLE, (char const *)buf );
			buf[ 0 ] = 0;
			curpos = 0;
		}

		Q_strncpy( &buf[ curpos ], line, sizeof( buf ) - curpos );
		curpos += len;

		++nDumped;
		if ( nMaxRecords != -1 && nDumped >= nMaxRecords )
			break;
	}

	if ( curpos > 0 )
	{
		ClientPrint( pRecipient, HUD_PRINTCONSOLE, buf );
	}
}

// Duck debouncing code to stop menu changes from disallowing crouch/uncrouch
ConVar xc_crouch_debounce( "xc_crouch_debounce", "0", FCVAR_NONE );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *ucmd - 
//			*moveHelper - 
//-----------------------------------------------------------------------------
void CBasePlayer::PlayerRunCommand(CUserCmd *ucmd, IMoveHelper *moveHelper)
{
	m_touchedPhysObject = false;

	if ( pl.fixangle == FIXANGLE_NONE)
	{
		VectorCopy ( ucmd->viewangles, pl.v_angle );
	}

	// Handle FL_FROZEN.
	// Prevent player moving for some seconds after New Game, so that they pick up everything
	if( GetFlags() & FL_FROZEN || 
		(developer.GetInt() == 0 && gpGlobals->eLoadType == MapLoad_NewGame && gpGlobals->curtime < 3.0 ) )
	{
		ucmd->forwardmove = 0;
		ucmd->sidemove = 0;
		ucmd->upmove = 0;
		ucmd->buttons = 0;
		ucmd->impulse = 0;
		VectorCopy ( pl.v_angle, ucmd->viewangles );
	}
	else
	{
		// Force a duck if we're toggled
		if ( GetToggledDuckState() )
		{
			// If this is set, we've altered our menu options and need to debounce the duck
			if ( xc_crouch_debounce.GetBool() )
			{
				ToggleDuck();
				
				// Mark it as handled
				xc_crouch_debounce.SetValue( 0 );
			}
			else
			{
				ucmd->buttons |= IN_DUCK;
			}
		}
	}

#ifdef MAPBASE_VSCRIPT
	// Movement hook for VScript
	if (m_ScriptScope.IsInitialized() && g_Hook_PlayerRunCommand.CanRunInScope(m_ScriptScope))
	{
		HSCRIPT hCmd = g_pScriptVM->RegisterInstance( reinterpret_cast<CScriptUserCmd*>(ucmd) );

		// command
		ScriptVariant_t args[] = { hCmd };
		g_Hook_PlayerRunCommand.Call( m_ScriptScope, NULL, args );

		g_pScriptVM->RemoveInstance( hCmd );
	}
#endif
	
	PlayerMove()->RunCommand(this, ucmd, moveHelper);
}

//-----------------------------------------------------------------------------
// Purpose: Strips off IN_xxx flags from the player's input
//-----------------------------------------------------------------------------
void CBasePlayer::DisableButtons( int nButtons )
{
	m_afButtonDisabled |= nButtons;
}

//-----------------------------------------------------------------------------
// Purpose: Re-enables stripped IN_xxx flags to the player's input
//-----------------------------------------------------------------------------
void CBasePlayer::EnableButtons( int nButtons )
{
	m_afButtonDisabled &= ~nButtons;
}

//-----------------------------------------------------------------------------
// Purpose: Strips off IN_xxx flags from the player's input
//-----------------------------------------------------------------------------
void CBasePlayer::ForceButtons( int nButtons )
{
	m_afButtonForced |= nButtons;
}

//-----------------------------------------------------------------------------
// Purpose: Re-enables stripped IN_xxx flags to the player's input
//-----------------------------------------------------------------------------
void CBasePlayer::UnforceButtons( int nButtons )
{
	m_afButtonForced &= ~nButtons;
}

void CBasePlayer::HandleFuncTrain(void)
{
	if ( m_afPhysicsFlags & PFLAG_DIROVERRIDE )
		AddFlag( FL_ONTRAIN );
	else 
		RemoveFlag( FL_ONTRAIN );

	// Train speed control
	if (( m_afPhysicsFlags & PFLAG_DIROVERRIDE ) == 0)
	{
		if (m_iTrain & TRAIN_ACTIVE)
		{
			m_iTrain = TRAIN_NEW; // turn off train
		}
		return;
	}

	CBaseEntity *pTrain = GetGroundEntity();
	float vel;

	if ( pTrain )
	{
		if ( !(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) )
			pTrain = NULL;
	}
	
	if ( !pTrain )
	{
		if ( GetActiveWeapon()->ObjectCaps() & FCAP_DIRECTIONAL_USE )
		{
			m_iTrain = TRAIN_ACTIVE | TRAIN_NEW;

			if ( m_nButtons & IN_FORWARD )
			{
				m_iTrain |= TRAIN_FAST;
			}
			else if ( m_nButtons & IN_BACK )
			{
				m_iTrain |= TRAIN_BACK;
			}
			else
			{
				m_iTrain |= TRAIN_NEUTRAL;
			}
			return;
		}
		else
		{
			trace_t trainTrace;
			// Maybe this is on the other side of a level transition
			UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector(0,0,-38), 
				MASK_PLAYERSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trainTrace );

			if ( trainTrace.fraction != 1.0 && trainTrace.m_pEnt )
				pTrain = trainTrace.m_pEnt;


			if ( !pTrain || !(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) || !pTrain->OnControls(this) )
			{
				m_afPhysicsFlags &= ~PFLAG_DIROVERRIDE;
				m_iTrain = TRAIN_NEW|TRAIN_OFF;
				return;
			}
		}
	}
	else if ( !( GetFlags() & FL_ONGROUND ) || pTrain->HasSpawnFlags( SF_TRACKTRAIN_NOCONTROL ) || (m_nButtons & (IN_MOVELEFT|IN_MOVERIGHT) ) )
	{
		// Turn off the train if you jump, strafe, or the train controls go dead
		m_afPhysicsFlags &= ~PFLAG_DIROVERRIDE;
		m_iTrain = TRAIN_NEW|TRAIN_OFF;
		return;
	}

	SetAbsVelocity( vec3_origin );
	vel = 0;
	if ( m_afButtonPressed & IN_FORWARD )
	{
		vel = 1;
		pTrain->Use( this, this, USE_SET, (float)vel );
	}
	else if ( m_afButtonPressed & IN_BACK )
	{
		vel = -1;
		pTrain->Use( this, this, USE_SET, (float)vel );
	}

	if (vel)
	{
		m_iTrain = TrainSpeed(pTrain->m_flSpeed, ((CFuncTrackTrain*)pTrain)->GetMaxSpeed());
		m_iTrain |= TRAIN_ACTIVE|TRAIN_NEW;
	}
}


void CBasePlayer::PreThink(void)
{						
	if ( g_fGameOver || m_iPlayerLocked )
		return;         // intermission or finale

	if ( Hints() )
	{
		Hints()->Update();
	}

	ItemPreFrame( );
	WaterMove();

	if ( g_pGameRules && g_pGameRules->FAllowFlashlight() )
		m_Local.m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	else
		m_Local.m_iHideHUD |= HIDEHUD_FLASHLIGHT;

	// checks if new client data (for HUD and view control) needs to be sent to the client
	UpdateClientData();
	
	CheckTimeBasedDamage();

	CheckSuitUpdate();

	if ( GetObserverMode() > OBS_MODE_FREEZECAM )
	{
		CheckObserverSettings();	// do this each frame
	}

	if ( m_lifeState >= LIFE_DYING )
	{
		// track where we are in the nav mesh even when dead
		UpdateLastKnownArea();
		return;
	}

	HandleFuncTrain();

	if (m_nButtons & IN_JUMP)
	{
		// If on a ladder, jump off the ladder
		// else Jump
		Jump();
	}

	// If trying to duck, already ducked, or in the process of ducking
	if ((m_nButtons & IN_DUCK) || (GetFlags() & FL_DUCKING) || (m_afPhysicsFlags & PFLAG_DUCKING) )
		Duck();

	//
	// If we're not on the ground, we're falling. Update our falling velocity.
	//
	if ( !( GetFlags() & FL_ONGROUND ) )
	{
		m_Local.m_flFallVelocity = -GetAbsVelocity().z;
	}

	// track where we are in the nav mesh
	UpdateLastKnownArea();


	// StudioFrameAdvance( );//!!!HACKHACK!!! Can't be hit by traceline when not animating?
}


/* Time based Damage works as follows: 
	1) There are several types of timebased damage:

		#define DMG_PARALYZE		(1 << 14)	// slows affected creature down
		#define DMG_NERVEGAS		(1 << 15)	// nerve toxins, very bad
		#define DMG_POISON			(1 << 16)	// blood poisioning
		#define DMG_RADIATION		(1 << 17)	// radiation exposure
		#define DMG_DROWNRECOVER	(1 << 18)	// drown recovery
		#define DMG_ACID			(1 << 19)	// toxic chemicals or acid burns
		#define DMG_SLOWBURN		(1 << 20)	// in an oven

	2) A new hit inflicting tbd restarts the tbd counter - each NPC has an 8bit counter,
		per damage type. The counter is decremented every second, so the maximum time 
		an effect will last is 255/60 = 4.25 minutes.  Of course, staying within the radius
		of a damaging effect like fire, nervegas, radiation will continually reset the counter to max.

	3) Every second that a tbd counter is running, the player takes damage.  The damage
		is determined by the type of tdb.  
			Paralyze		- 1/2 movement rate, 30 second duration.
			Nervegas		- 5 points per second, 16 second duration = 80 points max dose.
			Poison			- 2 points per second, 25 second duration = 50 points max dose.
			Radiation		- 1 point per second, 50 second duration = 50 points max dose.
			Drown			- 5 points per second, 2 second duration.
			Acid/Chemical	- 5 points per second, 10 second duration = 50 points max.
			Burn			- 10 points per second, 2 second duration.
			Freeze			- 3 points per second, 10 second duration = 30 points max.

	4) Certain actions or countermeasures counteract the damaging effects of tbds:

		Armor/Heater/Cooler - Chemical(acid),burn, freeze all do damage to armor power, then to body
							- recharged by suit recharger
		Air In Lungs		- drowning damage is done to air in lungs first, then to body
							- recharged by poking head out of water
							- 10 seconds if swiming fast
		Air In SCUBA		- drowning damage is done to air in tanks first, then to body
							- 2 minutes in tanks. Need new tank once empty.
		Radiation Syringe	- Each syringe full provides protection vs one radiation dosage
		Antitoxin Syringe	- Each syringe full provides protection vs one poisoning (nervegas or poison).
		Health kit			- Immediate stop to acid/chemical, fire or freeze damage.
		Radiation Shower	- Immediate stop to radiation damage, acid/chemical or fire damage.
		
	
*/

// If player is taking time based damage, continue doing damage to player -
// this simulates the effect of being poisoned, gassed, dosed with radiation etc -
// anything that continues to do damage even after the initial contact stops.
// Update all time based damage counters, and shut off any that are done.

// The m_bitsDamageType bit MUST be set if any damage is to be taken.
// This routine will detect the initial on value of the m_bitsDamageType
// and init the appropriate counter.  Only processes damage every second.

//#define PARALYZE_DURATION	30		// number of 2 second intervals to take damage
//#define PARALYZE_DAMAGE		0.0		// damage to take each 2 second interval

//#define NERVEGAS_DURATION	16
//#define NERVEGAS_DAMAGE		5.0

//#define POISON_DURATION		25
//#define POISON_DAMAGE		2.0

//#define RADIATION_DURATION	50
//#define RADIATION_DAMAGE	1.0

//#define ACID_DURATION		10
//#define ACID_DAMAGE			5.0

//#define SLOWBURN_DURATION	2
//#define SLOWBURN_DAMAGE		1.0

//#define SLOWFREEZE_DURATION	1.0
//#define SLOWFREEZE_DAMAGE	3.0

/* */


void CBasePlayer::CheckTimeBasedDamage() 
{
	int i;
	byte bDuration = 0;

	static float gtbdPrev = 0.0;

	// If we don't have any time based damage return.
	if ( !g_pGameRules->Damage_IsTimeBased( m_bitsDamageType ) )
		return;

	// only check for time based damage approx. every 2 seconds
	if ( abs( gpGlobals->curtime - m_tbdPrev ) < 2.0 )
		return;
	
	m_tbdPrev = gpGlobals->curtime;

	for (i = 0; i < CDMG_TIMEBASED; i++)
	{
		// Make sure the damage type is really time-based.
		// This is kind of hacky but necessary until we setup DamageType as an enum.
		int iDamage = ( DMG_PARALYZE << i );
		if ( !g_pGameRules->Damage_IsTimeBased( iDamage ) )
			continue;


		// make sure bit is set for damage type
		if ( m_bitsDamageType & iDamage )
		{
			switch (i)
			{
			case itbd_Paralyze:
				// UNDONE - flag movement as half-speed
				bDuration = PARALYZE_DURATION;
				break;
			case itbd_NerveGas:
//				OnTakeDamage(pev, pev, NERVEGAS_DAMAGE, DMG_GENERIC);	
				bDuration = NERVEGAS_DURATION;
				break;
//			case itbd_Poison:
//				OnTakeDamage( CTakeDamageInfo( this, this, POISON_DAMAGE, DMG_GENERIC ) );
//				bDuration = POISON_DURATION;
//				break;
			case itbd_Radiation:
//				OnTakeDamage(pev, pev, RADIATION_DAMAGE, DMG_GENERIC);
				bDuration = RADIATION_DURATION;
				break;
			case itbd_DrownRecover:
				// NOTE: this hack is actually used to RESTORE health
				// after the player has been drowning and finally takes a breath
				if (m_idrowndmg > m_idrownrestored)
				{
					int idif = MIN(m_idrowndmg - m_idrownrestored, 10);

					TakeHealth(idif, DMG_GENERIC);
					m_idrownrestored += idif;
				}
				bDuration = 4;	// get up to 5*10 = 50 points back
				break;

			case itbd_PoisonRecover:
			{
				// NOTE: this hack is actually used to RESTORE health
				// after the player has been poisoned.
				if (m_nPoisonDmg > m_nPoisonRestored)
				{
					int nDif = MIN(m_nPoisonDmg - m_nPoisonRestored, 10);
					TakeHealth(nDif, DMG_GENERIC);
					m_nPoisonRestored += nDif;
				}
				bDuration = 9;	// get up to 10*10 = 100 points back
				break;
			}

			case itbd_Acid:
//				OnTakeDamage(pev, pev, ACID_DAMAGE, DMG_GENERIC);
				bDuration = ACID_DURATION;
#ifdef MAPBASE
				// Prevents ant workers from inducing the Flash Plague, flashing the player's screen every time they take damage henceforth.
				// I think people came up with a different name, but I can't bother to look for it right now.
				// This fix might prevent other acid damage stuff as well, so it's not episodic-exclusive.
				m_bitsDamageType &= ~(DMG_ACID);
#endif
				break;
			case itbd_SlowBurn:
//				OnTakeDamage(pev, pev, SLOWBURN_DAMAGE, DMG_GENERIC);
				bDuration = SLOWBURN_DURATION;
				break;
			case itbd_SlowFreeze:
//				OnTakeDamage(pev, pev, SLOWFREEZE_DAMAGE, DMG_GENERIC);
				bDuration = SLOWFREEZE_DURATION;
				break;
			default:
				bDuration = 0;
			}

			if (m_rgbTimeBasedDamage[i])
			{
				// decrement damage duration, detect when done.
				if (!m_rgbTimeBasedDamage[i] || --m_rgbTimeBasedDamage[i] == 0)
				{
					m_rgbTimeBasedDamage[i] = 0;
					// if we're done, clear damage bits
					m_bitsDamageType &= ~(DMG_PARALYZE << i);	
				}
			}
			else
				// first time taking this damage type - init damage duration
				m_rgbTimeBasedDamage[i] = bDuration;
		}
	}
}

/*
THE POWER SUIT

The Suit provides 3 main functions: Protection, Notification and Augmentation. 
Some functions are automatic, some require power. 
The player gets the suit shortly after getting off the train in C1A0 and it stays
with him for the entire game.

Protection

	Heat/Cold
		When the player enters a hot/cold area, the heating/cooling indicator on the suit 
		will come on and the battery will drain while the player stays in the area. 
		After the battery is dead, the player starts to take damage. 
		This feature is built into the suit and is automatically engaged.
	Radiation Syringe
		This will cause the player to be immune from the effects of radiation for N seconds. Single use item.
	Anti-Toxin Syringe
		This will cure the player from being poisoned. Single use item.
	Health
		Small (1st aid kits, food, etc.)
		Large (boxes on walls)
	Armor
		The armor works using energy to create a protective field that deflects a
		percentage of damage projectile and explosive attacks. After the armor has been deployed,
		it will attempt to recharge itself to full capacity with the energy reserves from the battery.
		It takes the armor N seconds to fully charge. 

Notification (via the HUD)

x	Health
x	Ammo  
x	Automatic Health Care
		Notifies the player when automatic healing has been engaged. 
x	Geiger counter
		Classic Geiger counter sound and status bar at top of HUD 
		alerts player to dangerous levels of radiation. This is not visible when radiation levels are normal.
x	Poison
	Armor
		Displays the current level of armor. 

Augmentation 

	Reanimation (w/adrenaline)
		Causes the player to come back to life after he has been dead for 3 seconds. 
		Will not work if player was gibbed. Single use.
	Long Jump
		Used by hitting the ??? key(s). Caused the player to further than normal.
	SCUBA	
		Used automatically after picked up and after player enters the water. 
		Works for N seconds. Single use.	
	
Things powered by the battery

	Armor		
		Uses N watts for every M units of damage.
	Heat/Cool	
		Uses N watts for every second in hot/cold area.
	Long Jump	
		Uses N watts for every jump.
	Alien Cloak	
		Uses N watts for each use. Each use lasts M seconds.
	Alien Shield	
		Augments armor. Reduces Armor drain by one half
 
*/

// if in range of radiation source, ping geiger counter

#define GEIGERDELAY 0.25

void CBasePlayer::UpdateGeigerCounter( void )
{
	byte range;

	// delay per update ie: don't flood net with these msgs
	if (gpGlobals->curtime < m_flgeigerDelay)
		return;

	m_flgeigerDelay = gpGlobals->curtime + GEIGERDELAY;
		
	// send range to radition source to client
	range = (byte) clamp(Floor2Int(m_flgeigerRange / 4), 0, 255);

	// This is to make sure you aren't driven crazy by geiger while in the airboat
	if ( IsInAVehicle() )
	{
		range = clamp( (int)range * 4, 0, 255 );
	}

#ifdef MAPBASE
	// If the geiger is disabled, just use 255
	if (HasSpawnFlags(SF_PLAYER_NO_GEIGER))
		range = 255;
#endif

	if (range != m_igeigerRangePrev)
	{
		m_igeigerRangePrev = range;

		CSingleUserRecipientFilter user( this );
		user.MakeReliable();
		UserMessageBegin( user, "Geiger" );
			WRITE_BYTE( range );
		MessageEnd();
	}

	// reset counter and semaphore
	if (!random->RandomInt(0,3))
	{
		m_flgeigerRange = 1000;
	}
}

/*
================
CheckSuitUpdate

Play suit update if it's time
================
*/

#define SUITUPDATETIME	3.5
#define SUITFIRSTUPDATETIME 0.1

void CBasePlayer::CheckSuitUpdate()
{
	int i;
	int isentence = 0;
	int isearch = m_iSuitPlayNext;
	
	// Ignore suit updates if no suit
	if ( !IsSuitEquipped() )
		return;

	// if in range of radiation source, ping geiger counter
	UpdateGeigerCounter();

	if ( g_pGameRules->IsMultiplayer() )
	{
		// don't bother updating HEV voice in multiplayer.
		return;
	}

	if ( gpGlobals->curtime >= m_flSuitUpdate && m_flSuitUpdate > 0)
	{
		// play a sentence off of the end of the queue
		for (i = 0; i < CSUITPLAYLIST; i++)
			{
			if ((isentence = m_rgSuitPlayList[isearch]) != 0)
				break;
			
			if (++isearch == CSUITPLAYLIST)
				isearch = 0;
			}

		if (isentence)
		{
			m_rgSuitPlayList[isearch] = 0;
			if (isentence > 0)
			{
				// play sentence number

				char sentence[512];
				Q_snprintf( sentence, sizeof( sentence ), "!%s", engine->SentenceNameFromIndex( isentence ) );
				UTIL_EmitSoundSuit( edict(), sentence );
			}
			else
			{
				// play sentence group
				UTIL_EmitGroupIDSuit(edict(), -isentence);
			}
		m_flSuitUpdate = gpGlobals->curtime + SUITUPDATETIME;
		}
		else
			// queue is empty, don't check 
			m_flSuitUpdate = 0;
	}
}
 
// add sentence to suit playlist queue. if fgroup is true, then
// name is a sentence group (HEV_AA), otherwise name is a specific
// sentence name ie: !HEV_AA0.  If iNoRepeat is specified in
// seconds, then we won't repeat playback of this word or sentence
// for at least that number of seconds.

void CBasePlayer::SetSuitUpdate(const char *name, int fgroup, int iNoRepeatTime)
{
	int i;
	int isentence;
	int iempty = -1;
	
	
	// Ignore suit updates if no suit
	if ( !IsSuitEquipped() )
		return;

	if ( g_pGameRules->IsMultiplayer() )
	{
		// due to static channel design, etc. We don't play HEV sounds in multiplayer right now.
		return;
	}

	// if name == NULL, then clear out the queue

	if (!name)
	{
		for (i = 0; i < CSUITPLAYLIST; i++)
			m_rgSuitPlayList[i] = 0;
		return;
	}
	// get sentence or group number
	if (!fgroup)
	{
		isentence = SENTENCEG_Lookup(name);	// Lookup sentence index (not group) by name
		if (isentence < 0)
			return;
	}
	else
		// mark group number as negative
		isentence = -SENTENCEG_GetIndex(name);		// Lookup group index by name

	// check norepeat list - this list lets us cancel
	// the playback of words or sentences that have already
	// been played within a certain time.

	for (i = 0; i < CSUITNOREPEAT; i++)
	{
		if (isentence == m_rgiSuitNoRepeat[i])
			{
			// this sentence or group is already in 
			// the norepeat list

			if (m_rgflSuitNoRepeatTime[i] < gpGlobals->curtime)
				{
				// norepeat time has expired, clear it out
				m_rgiSuitNoRepeat[i] = 0;
				m_rgflSuitNoRepeatTime[i] = 0.0;
				iempty = i;
				break;
				}
			else
				{
				// don't play, still marked as norepeat
				return;
				}
			}
		// keep track of empty slot
		if (!m_rgiSuitNoRepeat[i])
			iempty = i;
	}

	// sentence is not in norepeat list, save if norepeat time was given

	if (iNoRepeatTime)
	{
		if (iempty < 0)
			iempty = random->RandomInt(0, CSUITNOREPEAT-1); // pick random slot to take over
		m_rgiSuitNoRepeat[iempty] = isentence;
		m_rgflSuitNoRepeatTime[iempty] = iNoRepeatTime + gpGlobals->curtime;
	}

	// find empty spot in queue, or overwrite last spot
	
	m_rgSuitPlayList[m_iSuitPlayNext++] = isentence;
	if (m_iSuitPlayNext == CSUITPLAYLIST)
		m_iSuitPlayNext = 0;

	if (m_flSuitUpdate <= gpGlobals->curtime)
	{
		if (m_flSuitUpdate == 0)
			// play queue is empty, don't delay too long before playback
			m_flSuitUpdate = gpGlobals->curtime + SUITFIRSTUPDATETIME;
		else 
			m_flSuitUpdate = gpGlobals->curtime + SUITUPDATETIME; 
	}

}

//=========================================================
// UpdatePlayerSound - updates the position of the player's
// reserved sound slot in the sound list.
//=========================================================
void CBasePlayer::UpdatePlayerSound ( void )
{
	int iBodyVolume;
	int iVolume;
	CSound *pSound;

	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( edict() ) );

	if ( !pSound )
	{
		Msg( "Client lost reserved sound!\n" );
		return;
	}

	if (GetFlags() & FL_NOTARGET)
	{
		pSound->m_iVolume = 0;
		return;
	}

	// now figure out how loud the player's movement is.
	if ( GetFlags() & FL_ONGROUND )
	{	
		iBodyVolume = GetAbsVelocity().Length(); 

		// clamp the noise that can be made by the body, in case a push trigger,
		// weapon recoil, or anything shoves the player abnormally fast. 
		// NOTE: 512 units is a pretty large radius for a sound made by the player's body.
		// then again, I think some materials are pretty loud.
		if ( iBodyVolume > 512 )
		{
			iBodyVolume = 512;
		}
	}
	else
	{
		iBodyVolume = 0;
	}

	if ( m_nButtons & IN_JUMP )
	{
		// Jumping is a little louder.
		iBodyVolume += 100;
	}

	m_iTargetVolume = iBodyVolume;

	// if target volume is greater than the player sound's current volume, we paste the new volume in 
	// immediately. If target is less than the current volume, current volume is not set immediately to the
	// lower volume, rather works itself towards target volume over time. This gives NPCs a much better chance
	// to hear a sound, especially if they don't listen every frame.
	iVolume = pSound->Volume();

	if ( m_iTargetVolume > iVolume )
	{
		iVolume = m_iTargetVolume;
	}
	else if ( iVolume > m_iTargetVolume )
	{
		iVolume -= 250 * gpGlobals->frametime;

		if ( iVolume < m_iTargetVolume )
		{
			iVolume = 0;
		}
	}

	if ( pSound )
	{
		pSound->SetSoundOrigin( GetAbsOrigin() );
		pSound->m_iType = SOUND_PLAYER;
		pSound->m_iVolume = iVolume;
	}

	// Below are a couple of useful little bits that make it easier to visualize just how much noise the 
	// player is making. 
	//Vector forward = UTIL_YawToVector( pl.v_angle.y );
	//UTIL_Sparks( GetAbsOrigin() + forward * iVolume );
	//Msg( "%d/%d\n", iVolume, m_iTargetVolume );
}

// This is a glorious hack to find free space when you've crouched into some solid space
// Our crouching collisions do not work correctly for some reason and this is easier
// than fixing the problem :(
void FixPlayerCrouchStuck( CBasePlayer *pPlayer )
{
	trace_t trace;

	// Move up as many as 18 pixels if the player is stuck.
	int i;
	Vector org = pPlayer->GetAbsOrigin();;
	for ( i = 0; i < 18; i++ )
	{
		UTIL_TraceHull( pPlayer->GetAbsOrigin(), pPlayer->GetAbsOrigin(), 
			VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
		if ( trace.startsolid )
		{
			Vector origin = pPlayer->GetAbsOrigin();
			origin.z += 1.0f;
			pPlayer->SetLocalOrigin( origin );
		}
		else
			return;
	}

	pPlayer->SetAbsOrigin( org );

	for ( i = 0; i < 18; i++ )
	{
		UTIL_TraceHull( pPlayer->GetAbsOrigin(), pPlayer->GetAbsOrigin(), 
			VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
		if ( trace.startsolid )
		{
			Vector origin = pPlayer->GetAbsOrigin();
			origin.z -= 1.0f;
			pPlayer->SetLocalOrigin( origin );
		}
		else
			return;
	}
}
#define SMOOTHING_FACTOR 0.9
extern CMoveData *g_pMoveData;

// UNDONE: Look and see if the ground entity is in hierarchy with a MOVETYPE_VPHYSICS?
// Behavior in that case is not as good currently when the parent is rideable
bool CBasePlayer::IsRideablePhysics( IPhysicsObject *pPhysics )
{
	if ( pPhysics )
	{
		if ( pPhysics->GetMass() > (VPhysicsGetObject()->GetMass()*2) )
			return true;
	}

	return false;
}

IPhysicsObject *CBasePlayer::GetGroundVPhysics()
{
	CBaseEntity *pGroundEntity = GetGroundEntity();
	if ( pGroundEntity && pGroundEntity->GetMoveType() == MOVETYPE_VPHYSICS )
	{
		IPhysicsObject *pPhysGround = pGroundEntity->VPhysicsGetObject();
		if ( pPhysGround && pPhysGround->IsMoveable() )
			return pPhysGround;
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// For debugging...
//-----------------------------------------------------------------------------
void CBasePlayer::ForceOrigin( const Vector &vecOrigin )
{
	m_bForceOrigin = true;
	m_vForcedOrigin = vecOrigin;
}

#ifdef MAPBASE // From Alien Swarm SDK
//--------------------------------------------------------------------------------------------------------
void CBasePlayer::OnTonemapTriggerStartTouch( CTonemapTrigger *pTonemapTrigger )
{
	m_hTriggerTonemapList.FindAndRemove( pTonemapTrigger );
	m_hTriggerTonemapList.AddToTail( pTonemapTrigger );
}


//--------------------------------------------------------------------------------------------------------
void CBasePlayer::OnTonemapTriggerEndTouch( CTonemapTrigger *pTonemapTrigger )
{
	m_hTriggerTonemapList.FindAndRemove( pTonemapTrigger );
}


//--------------------------------------------------------------------------------------------------------
void CBasePlayer::UpdateTonemapController( void )
{
	// For now, Mapbase uses Tony Sergi's Source 2007 tonemap fixes.
	// Alien Swarm SDK tonemap controller code copies the parameters instead.

	CEnvTonemapController *pController = NULL;

	if (m_hTriggerTonemapList.Count() > 0)
	{
		pController = static_cast<CEnvTonemapController*>(m_hTriggerTonemapList.Tail()->GetTonemapController());
	}
	else if (TheTonemapSystem()->GetMasterTonemapController())
	{
		pController = static_cast<CEnvTonemapController*>(TheTonemapSystem()->GetMasterTonemapController());
	}

	if (pController)
	{
		//m_hTonemapController = TheTonemapSystem()->GetMasterTonemapController();

		if (pController->m_bUseCustomAutoExposureMax)
			m_Local.m_TonemapParams.m_flAutoExposureMax = pController->m_flCustomAutoExposureMax;

		if (pController->m_bUseCustomAutoExposureMin)
			m_Local.m_TonemapParams.m_flAutoExposureMin = pController->m_flCustomAutoExposureMin;

		if (pController->m_bUseCustomBloomScale)
			m_Local.m_TonemapParams.m_flBloomScale = pController->m_flCustomBloomScale;
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::PostThink()
{
	m_vecSmoothedVelocity = m_vecSmoothedVelocity * SMOOTHING_FACTOR + GetAbsVelocity() * ( 1 - SMOOTHING_FACTOR );

#ifdef MAPBASE // From Alien Swarm SDK
	UpdateTonemapController();
	UpdateFXVolume();
#endif

	if ( !g_fGameOver && !m_iPlayerLocked )
	{
		if ( IsAlive() )
		{
			// set correct collision bounds (may have changed in player movement code)
			VPROF_SCOPE_BEGIN( "CBasePlayer::PostThink-Bounds" );
			if ( GetFlags() & FL_DUCKING )
			{
				SetCollisionBounds( VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );
			}
			else
			{
				SetCollisionBounds( VEC_HULL_MIN, VEC_HULL_MAX );
			}
			VPROF_SCOPE_END();

			VPROF_SCOPE_BEGIN( "CBasePlayer::PostThink-Use" );
			// Handle controlling an entity
			if ( m_hUseEntity != NULL )
			{ 
				// if they've moved too far from the gun, or deployed another weapon, unuse the gun
				if ( m_hUseEntity->OnControls( this ) && 
					( !GetActiveWeapon() || GetActiveWeapon()->IsEffectActive( EF_NODRAW ) ||
					( GetActiveWeapon()->GetActivity() == ACT_VM_HOLSTER ) 
	#ifdef PORTAL // Portalgun view model stays up when holding an object -Jeep
					|| FClassnameIs( GetActiveWeapon(), "weapon_portalgun" ) 
	#endif //#ifdef PORTAL			
					) )
				{  
					m_hUseEntity->Use( this, this, USE_SET, 2 );	// try fire the gun
				}
				else
				{
					// they've moved off the controls
					ClearUseEntity();
				}
			}
			VPROF_SCOPE_END();

			// do weapon stuff
			VPROF_SCOPE_BEGIN( "CBasePlayer::PostThink-ItemPostFrame" );
			ItemPostFrame();
			VPROF_SCOPE_END();

			if ( GetFlags() & FL_ONGROUND )
			{		
				if (m_Local.m_flFallVelocity > 64 && !g_pGameRules->IsMultiplayer())
				{
					CSoundEnt::InsertSound ( SOUND_PLAYER, GetAbsOrigin(), m_Local.m_flFallVelocity, 0.2, this );
					// Msg( "fall %f\n", m_Local.m_flFallVelocity );
				}
				m_Local.m_flFallVelocity = 0;
			}

			// select the proper animation for the player character	
			VPROF( "CBasePlayer::PostThink-Animation" );
			// If he's in a vehicle, sit down
			if ( IsInAVehicle() )
				SetAnimation( PLAYER_IN_VEHICLE );
			else if (!GetAbsVelocity().x && !GetAbsVelocity().y)
				SetAnimation( PLAYER_IDLE );
			else if ((GetAbsVelocity().x || GetAbsVelocity().y) && ( GetFlags() & FL_ONGROUND ))
				SetAnimation( PLAYER_WALK );
			else if (GetWaterLevel() > 1)
				SetAnimation( PLAYER_WALK );
		}

		// Don't allow bogus sequence on player
		if ( GetSequence() == -1 )
		{
			SetSequence( 0 );
		}

		VPROF_SCOPE_BEGIN( "CBasePlayer::PostThink-StudioFrameAdvance" );
		StudioFrameAdvance();
		VPROF_SCOPE_END();

		VPROF_SCOPE_BEGIN( "CBasePlayer::PostThink-DispatchAnimEvents" );
		DispatchAnimEvents( this );
		VPROF_SCOPE_END();

		SetSimulationTime( gpGlobals->curtime );

		//Let the weapon update as well
		VPROF_SCOPE_BEGIN( "CBasePlayer::PostThink-Weapon_FrameUpdate" );
		Weapon_FrameUpdate();
		VPROF_SCOPE_END();

		VPROF_SCOPE_BEGIN( "CBasePlayer::PostThink-UpdatePlayerSound" );
		UpdatePlayerSound();
		VPROF_SCOPE_END();

		if ( m_bForceOrigin )
		{
			SetLocalOrigin( m_vForcedOrigin );
			SetLocalAngles( m_Local.m_vecPunchAngle );
			m_Local.m_vecPunchAngle = RandomAngle( -25, 25 );
			m_Local.m_vecPunchAngleVel.Init();
		}

		VPROF_SCOPE_BEGIN( "CBasePlayer::PostThink-PostThinkVPhysics" );
		PostThinkVPhysics();
		VPROF_SCOPE_END();
	}

#if !defined( NO_ENTITY_PREDICTION )
	// Even if dead simulate entities
	SimulatePlayerSimulatedEntities();
#endif

}

// handles touching physics objects
void CBasePlayer::Touch( CBaseEntity *pOther )
{
	if ( pOther == GetGroundEntity() )
		return;

	if ( pOther->GetMoveType() != MOVETYPE_VPHYSICS || pOther->GetSolid() != SOLID_VPHYSICS || (pOther->GetSolidFlags() & FSOLID_TRIGGER) )
		return;

	IPhysicsObject *pPhys = pOther->VPhysicsGetObject();
	if ( !pPhys || !pPhys->IsMoveable() )
		return;

	SetTouchedPhysics( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBasePlayer::PostThinkVPhysics( void )
{
	// Check to see if things are initialized!
	if ( !m_pPhysicsController )
		return;

	Vector newPosition = GetAbsOrigin();
	float frametime = gpGlobals->frametime;
	if ( frametime <= 0 || frametime > 0.1f )
		frametime = 0.1f;

	IPhysicsObject *pPhysGround = GetGroundVPhysics();

	if ( !pPhysGround && m_touchedPhysObject && g_pMoveData->m_outStepHeight <= 0.f && (GetFlags() & FL_ONGROUND) )
	{
		newPosition = m_oldOrigin + frametime * g_pMoveData->m_outWishVel;
		newPosition = (GetAbsOrigin() * 0.5f) + (newPosition * 0.5f);
	}

	int collisionState = VPHYS_WALK;
	if ( GetMoveType() == MOVETYPE_NOCLIP || GetMoveType() == MOVETYPE_OBSERVER )
	{
		collisionState = VPHYS_NOCLIP;
	}
	else if ( GetFlags() & FL_DUCKING )
	{
		collisionState = VPHYS_CROUCH;
	}

	if ( collisionState != m_vphysicsCollisionState )
	{
		SetVCollisionState( GetAbsOrigin(), GetAbsVelocity(), collisionState );
	}

	if ( !(TouchedPhysics() || pPhysGround) )
	{
		float maxSpeed = m_flMaxspeed > 0.0f ? m_flMaxspeed : sv_maxspeed.GetFloat();
		g_pMoveData->m_outWishVel.Init( maxSpeed, maxSpeed, maxSpeed );
	}

	// teleport the physics object up by stepheight (game code does this - reflect in the physics)
	if ( g_pMoveData->m_outStepHeight > 0.1f )
	{
		if ( g_pMoveData->m_outStepHeight > 4.0f )
		{
			VPhysicsGetObject()->SetPosition( GetAbsOrigin(), vec3_angle, true );
		}
		else
		{
			// don't ever teleport into solid
			Vector position, end;
			VPhysicsGetObject()->GetPosition( &position, NULL );
			end = position;
			end.z += g_pMoveData->m_outStepHeight;
			trace_t trace;
			UTIL_TraceEntity( this, position, end, MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
			if ( trace.DidHit() )
			{
				g_pMoveData->m_outStepHeight = trace.endpos.z - position.z;
			}
			m_pPhysicsController->StepUp( g_pMoveData->m_outStepHeight );
		}
		m_pPhysicsController->Jump();
	}
	g_pMoveData->m_outStepHeight = 0.0f;
	
	// Store these off because after running the usercmds, it'll pass them
	// to UpdateVPhysicsPosition.	
	m_vNewVPhysicsPosition = newPosition;
	m_vNewVPhysicsVelocity = g_pMoveData->m_outWishVel;

	m_oldOrigin = GetAbsOrigin();
}

void CBasePlayer::UpdateVPhysicsPosition( const Vector &position, const Vector &velocity, float secondsToArrival )
{
	bool onground = (GetFlags() & FL_ONGROUND) ? true : false;
	IPhysicsObject *pPhysGround = GetGroundVPhysics();
	
	// if the object is much heavier than the player, treat it as a local coordinate system
	// the player controller will solve movement differently in this case.
	if ( !IsRideablePhysics(pPhysGround) )
	{
		pPhysGround = NULL;
	}

	m_pPhysicsController->Update( position, velocity, secondsToArrival, onground, pPhysGround );
}

void CBasePlayer::UpdatePhysicsShadowToCurrentPosition()
{
	UpdateVPhysicsPosition( GetAbsOrigin(), vec3_origin, gpGlobals->frametime );
}

void CBasePlayer::UpdatePhysicsShadowToPosition( const Vector &vecAbsOrigin )
{
	UpdateVPhysicsPosition( vecAbsOrigin, vec3_origin, gpGlobals->frametime );
}

Vector CBasePlayer::GetSmoothedVelocity( void )
{ 
	if ( IsInAVehicle() )
	{
		return GetVehicle()->GetVehicleEnt()->GetSmoothedVelocity();
	}
	return m_vecSmoothedVelocity;
}


CBaseEntity	*g_pLastSpawn = NULL;


//-----------------------------------------------------------------------------
// Purpose: Finds a player start entity of the given classname. If any entity of
//			of the given classname has the SF_PLAYER_START_MASTER flag set, that
//			is the entity that will be returned. Otherwise, the first entity of
//			the given classname is returned.
// Input  : pszClassName - should be "info_player_start", "info_player_coop", or
//			"info_player_deathmatch"
//-----------------------------------------------------------------------------
CBaseEntity *FindPlayerStart(const char *pszClassName)
{
	#define SF_PLAYER_START_MASTER	1
	
	CBaseEntity *pStart = gEntList.FindEntityByClassname(NULL, pszClassName);
	CBaseEntity *pStartFirst = pStart;
	while (pStart != NULL)
	{
		if (pStart->HasSpawnFlags(SF_PLAYER_START_MASTER))
		{
			return pStart;
		}

		pStart = gEntList.FindEntityByClassname(pStart, pszClassName);
	}

	return pStartFirst;
}

/*
============
EntSelectSpawnPoint

Returns the entity to spawn at

USES AND SETS GLOBAL g_pLastSpawn
============
*/
CBaseEntity *CBasePlayer::EntSelectSpawnPoint()
{
	CBaseEntity *pSpot;
	edict_t		*player;

	player = edict();

// choose a info_player_deathmatch point
	if (g_pGameRules->IsCoOp())
	{
		pSpot = gEntList.FindEntityByClassname( g_pLastSpawn, "info_player_coop");
		if ( pSpot )
			goto ReturnSpot;
		pSpot = gEntList.FindEntityByClassname( g_pLastSpawn, "info_player_start");
		if ( pSpot ) 
			goto ReturnSpot;
	}
	else if ( g_pGameRules->IsDeathmatch() )
	{
		pSpot = g_pLastSpawn;
		// Randomize the start spot
		for ( int i = random->RandomInt(1,5); i > 0; i-- )
			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );
		if ( !pSpot )  // skip over the null point
			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );

		CBaseEntity *pFirstSpot = pSpot;

		do 
		{
			if ( pSpot )
			{
				// check if pSpot is valid
				if ( g_pGameRules->IsSpawnPointValid( pSpot, this ) )
				{
					if ( pSpot->GetLocalOrigin() == vec3_origin )
					{
						pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );
						continue;
					}

					// if so, go to pSpot
					goto ReturnSpot;
				}
			}
			// increment pSpot
			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_deathmatch" );
		} while ( pSpot != pFirstSpot ); // loop if we're not back to the start

		// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
		if ( pSpot )
		{
			CBaseEntity *ent = NULL;
			for ( CEntitySphereQuery sphere( pSpot->GetAbsOrigin(), 128 ); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
			{
				// if ent is a client, kill em (unless they are ourselves)
				if ( ent->IsPlayer() && !(ent->edict() == player) )
					ent->TakeDamage( CTakeDamageInfo( GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), 300, DMG_GENERIC ) );
			}
			goto ReturnSpot;
		}
	}

	// If startspot is set, (re)spawn there.
	if ( !gpGlobals->startspot || !strlen(STRING(gpGlobals->startspot)))
	{
		pSpot = FindPlayerStart( "info_player_start" );
		if ( pSpot )
			goto ReturnSpot;
	}
	else
	{
		pSpot = gEntList.FindEntityByName( NULL, gpGlobals->startspot );
		if ( pSpot )
			goto ReturnSpot;
	}

ReturnSpot:
	if ( !pSpot  )
	{
		Warning( "PutClientInServer: no info_player_start on level\n");
		return CBaseEntity::Instance( INDEXENT( 0 ) );
	}

	g_pLastSpawn = pSpot;
	return pSpot;
}

//-----------------------------------------------------------------------------
// Purpose: Called the first time the player's created
//-----------------------------------------------------------------------------
void CBasePlayer::InitialSpawn( void )
{
	m_iConnected = PlayerConnected;
	gamestats->Event_PlayerConnected( this );
}

//-----------------------------------------------------------------------------
// Purpose: clear our m_Local.m_TonemapParams to -1.
//-----------------------------------------------------------------------------
void CBasePlayer::ClearTonemapParams( void )
{
	//Tony; clear all the variables to -1.0
	m_Local.m_TonemapParams.m_flAutoExposureMin = -1.0f;
	m_Local.m_TonemapParams.m_flAutoExposureMax = -1.0f;
	m_Local.m_TonemapParams.m_flTonemapScale = -1.0f;
	m_Local.m_TonemapParams.m_flBloomScale = -1.0f;
	m_Local.m_TonemapParams.m_flTonemapRate = -1.0f;
}
void CBasePlayer::InputSetTonemapScale( inputdata_t &inputdata )
{
	m_Local.m_TonemapParams.m_flTonemapScale = inputdata.value.Float();
}

void CBasePlayer::InputSetTonemapRate( inputdata_t &inputdata )
{
	m_Local.m_TonemapParams.m_flTonemapRate = inputdata.value.Float();
}
void CBasePlayer::InputSetAutoExposureMin( inputdata_t &inputdata )
{
	m_Local.m_TonemapParams.m_flAutoExposureMin = inputdata.value.Float();
}

void CBasePlayer::InputSetAutoExposureMax( inputdata_t &inputdata )
{
	m_Local.m_TonemapParams.m_flAutoExposureMax = inputdata.value.Float();
}

void CBasePlayer::InputSetBloomScale( inputdata_t &inputdata )
{
	m_Local.m_TonemapParams.m_flBloomScale = inputdata.value.Float();
}

//Tony; restore defaults (set min/max to -1.0 so nothing gets overridden)
void CBasePlayer::InputUseDefaultAutoExposure( inputdata_t &inputdata )
{
	m_Local.m_TonemapParams.m_flAutoExposureMin = -1.0f;
	m_Local.m_TonemapParams.m_flAutoExposureMax = -1.0f;
	m_Local.m_TonemapParams.m_flTonemapRate = -1.0f;
}
void CBasePlayer::InputUseDefaultBloomScale( inputdata_t &inputdata )
{
	m_Local.m_TonemapParams.m_flBloomScale = -1.0f;
}
//	void	InputSetBloomScaleRange( inputdata_t &inputdata );

//-----------------------------------------------------------------------------
// Purpose: Called everytime the player respawns
//-----------------------------------------------------------------------------
void CBasePlayer::Spawn( void )
{
	// Needs to be done before weapons are given
	if ( Hints() )
	{
		Hints()->ResetHints();
	}

	//Tony; make sure tonemap params is cleared.
	ClearTonemapParams();

	SetClassname( "player" );

	// Shared spawning code..
	SharedSpawn();
	
	SetSimulatedEveryTick( true );
	SetAnimatedEveryTick( true );

	m_ArmorValue		= SpawnArmorValue();
	SetBlocksLOS( false );
	m_iMaxHealth		= m_iHealth;

	// Clear all flags except for FL_FULLEDICT
	if ( GetFlags() & FL_FAKECLIENT )
	{
		ClearFlags();
		AddFlag( FL_CLIENT | FL_FAKECLIENT );
	}
	else
	{
		ClearFlags();
		AddFlag( FL_CLIENT );
	}

	AddFlag( FL_AIMTARGET );

	m_AirFinished	= gpGlobals->curtime + AIRTIME;
	m_nDrownDmgRate	= DROWNING_DAMAGE_INITIAL;
	
 // only preserve the shadow flag
	int effects = GetEffects() & EF_NOSHADOW;
	SetEffects( effects );

	IncrementInterpolationFrame();

	// Initialize the fog and postprocess controllers.
	InitFogController();
#ifdef MAPBASE // From Alien Swarm SDK
	InitPostProcessController();
#endif

	m_DmgTake		= 0;
	m_DmgSave		= 0;
	m_bitsHUDDamage		= -1;
	m_bitsDamageType	= 0;
	m_afPhysicsFlags	= 0;

	m_idrownrestored = m_idrowndmg;

	SetFOV( this, 0 );

	m_flNextDecalTime	= 0;// let this player decal as soon as he spawns.

	m_flgeigerDelay = gpGlobals->curtime + 2.0;	// wait a few seconds until user-defined message registrations
												// are recieved by all clients
	
	m_flFieldOfView		= 0.766;// some NPCs use this to determine whether or not the player is looking at them.

	m_vecAdditionalPVSOrigin = vec3_origin;
	m_vecCameraPVSOrigin = vec3_origin;

	if ( !m_fGameHUDInitialized )
		g_pGameRules->SetDefaultPlayerTeam( this );

#ifdef MAPBASE
	CBaseEntity *pSpawnPoint = g_pGameRules->GetPlayerSpawnSpot( this );
	SpawnedAtPoint( pSpawnPoint );
#else
	g_pGameRules->GetPlayerSpawnSpot( this );
#endif

	m_Local.m_bDucked = false;// This will persist over round restart if you hold duck otherwise. 
	m_Local.m_bDucking = false;
    SetViewOffset( VEC_VIEW_SCALED( this ) );
	Precache();
	
	m_bitsDamageType = 0;
	m_bitsHUDDamage = -1;
	SetPlayerUnderwater( false );

	m_iTrain = TRAIN_NEW;
	
	m_HackedGunPos		= Vector( 0, 32, 0 );

	m_iBonusChallenge = sv_bonus_challenge.GetInt();
	sv_bonus_challenge.SetValue( 0 );

	if ( m_iPlayerSound == SOUNDLIST_EMPTY )
	{
		Msg( "Couldn't alloc player sound slot!\n" );
	}

	SetThink(NULL);
	m_fInitHUD = true;
	m_fWeapon = false;
	m_iClientBattery = -1;

	m_lastx = m_lasty = 0;

	Q_strncpy( m_szLastPlaceName.GetForModify(), "", MAX_PLACE_NAME_LENGTH );
	
	CSingleUserRecipientFilter user( this );
	enginesound->SetPlayerDSP( user, 0, false );

	CreateViewModel();
#ifdef MAPBASE
	CreateHandModel();
#endif

	SetCollisionGroup( COLLISION_GROUP_PLAYER );

	// if the player is locked, make sure he stays locked
	if ( m_iPlayerLocked )
	{
		m_iPlayerLocked = false;
		LockPlayerInPlace();
	}

	if ( GetTeamNumber() != TEAM_SPECTATOR )
	{
		StopObserverMode();
	}
	else
	{
		StartObserverMode( m_iObserverLastMode );
	}

	StopReplayMode();

	// Clear any screenfade
	color32 nothing = {0,0,0,255};
	UTIL_ScreenFade( this, nothing, 0, 0, FFADE_IN | FFADE_PURGE );

	g_pGameRules->PlayerSpawn( this );

	m_flLaggedMovementValue = 1.0f;
	m_vecSmoothedVelocity = vec3_origin;
	InitVCollision( GetAbsOrigin(), GetAbsVelocity() );

	if ( !g_pGameRules->IsMultiplayer() && g_pScriptVM )
	{
		g_pScriptVM->SetValue( "player", GetScriptInstance() );
	}

#if !defined( TF_DLL )
	IGameEvent *event = gameeventmanager->CreateEvent( "player_spawn" );
	
	if ( event )
	{
		event->SetInt("userid", GetUserID() );
		gameeventmanager->FireEvent( event );
	}
#endif

	RumbleEffect( RUMBLE_STOP_ALL, 0, RUMBLE_FLAGS_NONE );

	// Calculate this immediately
	m_nVehicleViewSavedFrame = 0;

	// track where we are in the nav mesh
	UpdateLastKnownArea();

	BaseClass::Spawn();

	// track where we are in the nav mesh
	UpdateLastKnownArea();

	m_weaponFiredTimer.Invalidate();
}

void CBasePlayer::Activate( void )
{
	BaseClass::Activate();

	AimTarget_ForceRepopulateList();

	RumbleEffect( RUMBLE_STOP_ALL, 0, RUMBLE_FLAGS_NONE );

	// Reset the analog bias. If the player is in a vehicle when the game
	// reloads, it will autosense and apply the correct bias.
	m_iVehicleAnalogBias = VEHICLE_ANALOG_BIAS_NONE;
}

void CBasePlayer::Precache( void )
{
	BaseClass::Precache();


	PrecacheScriptSound( "Player.FallGib" );
	PrecacheScriptSound( "Player.Death" );
	PrecacheScriptSound( "Player.PlasmaDamage" );
	PrecacheScriptSound( "Player.SonicDamage" );
	PrecacheScriptSound( "Player.DrownStart" );
	PrecacheScriptSound( "Player.DrownContinue" );
	PrecacheScriptSound( "Player.Wade" );
	PrecacheScriptSound( "Player.AmbientUnderWater" );
	enginesound->PrecacheSentenceGroup( "HEV" );

	// These are always needed
#ifndef TF_DLL
	PrecacheParticleSystem( "slime_splash_01" );
	PrecacheParticleSystem( "slime_splash_02" );
	PrecacheParticleSystem( "slime_splash_03" );
#endif

	// in the event that the player JUST spawned, and the level node graph
	// was loaded, fix all of the node graph pointers before the game starts.
	
	// !!!BUGBUG - now that we have multiplayer, this needs to be moved!
	/* todo - put in better spot and use new ainetowrk stuff
	if ( WorldGraph.m_fGraphPresent && !WorldGraph.m_fGraphPointersSet )
	{
		if ( !WorldGraph.FSetGraphPointers() )
		{
			Msg( "**Graph pointers were not set!\n");
		}
		else
		{
			Msg( "**Graph Pointers Set!\n" );
		} 
	}
	*/

	// SOUNDS / MODELS ARE PRECACHED in ClientPrecache() (game specific)
	// because they need to precache before any clients have connected

	// init geiger counter vars during spawn and each time
	// we cross a level transition
	m_flgeigerRange = 1000;
	m_igeigerRangePrev = 1000;

#if 0
	// @Note (toml 04-19-04): These are saved, used to be slammed here
	m_bitsDamageType = 0;
	m_bitsHUDDamage = -1;
	SetPlayerUnderwter( false );

	m_iTrain = TRAIN_NEW;
#endif

#ifdef MAPBASE
	PrecacheModel( g_szDefaultHandsModel );
#endif

	m_iClientBattery = -1;

	m_iUpdateTime = 5;  // won't update for 1/2 a second

	if ( gInitHUD )
		m_fInitHUD = true;

}

//-----------------------------------------------------------------------------
// Purpose: Force this player to immediately respawn
//-----------------------------------------------------------------------------
void CBasePlayer::ForceRespawn( void )
{
	RemoveAllItems( true );

	// Reset ground state for airwalk animations
	SetGroundEntity( NULL );

	// Stop any firing that was taking place before respawn.
	m_nButtons = 0;

	Spawn();
}

int CBasePlayer::Save( ISave &save )
{
	if ( !BaseClass::Save(save) )
		return 0;

	return 1;
}


// Friend class of CBaseEntity to access private member data.
class CPlayerRestoreHelper
{
public:

	const Vector &GetAbsOrigin( CBaseEntity *pent )
	{
		return pent->m_vecAbsOrigin;
	}

	const Vector &GetAbsVelocity( CBaseEntity *pent )
	{
		return pent->m_vecAbsVelocity;
	}
};


int CBasePlayer::Restore( IRestore &restore )
{
	int status = BaseClass::Restore(restore);
	if ( !status )
		return 0;

	CSaveRestoreData *pSaveData = gpGlobals->pSaveData;
	// landmark isn't present.
	if ( !pSaveData->levelInfo.fUseLandmark )
	{
		Msg( "No Landmark:%s\n", pSaveData->levelInfo.szLandmarkName );

		// default to normal spawn
		CBaseEntity *pSpawnSpot = EntSelectSpawnPoint();
		SetLocalOrigin( pSpawnSpot->GetLocalOrigin() + Vector(0,0,1) );
		SetLocalAngles( pSpawnSpot->GetLocalAngles() );
	}

	QAngle newViewAngles = pl.v_angle;
	newViewAngles.z = 0;	// Clear out roll
	SetLocalAngles( newViewAngles );
	SnapEyeAngles( newViewAngles );

	// Copied from spawn() for now
	SetBloodColor( BLOOD_COLOR_RED );
	
	// clear this - it will get reset by touching the trigger again
	m_afPhysicsFlags &= ~PFLAG_VPHYSICS_MOTIONCONTROLLER;

	if ( GetFlags() & FL_DUCKING ) 
	{
		// Use the crouch HACK
		FixPlayerCrouchStuck( this );
		UTIL_SetSize(this, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
		m_Local.m_bDucked = true;
	}
	else
	{
		m_Local.m_bDucked = false;
		UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);
	}

	// We need to get at m_vecAbsOrigin as it was restored but can't let it be
	// recalculated by a call to GetAbsOrigin because hierarchy isn't fully restored yet,
	// so we use this backdoor to get at the private data in CBaseEntity.
	CPlayerRestoreHelper helper;
	InitVCollision( helper.GetAbsOrigin( this ), helper.GetAbsVelocity( this ) );

	// success
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::OnRestore( void )
{
	BaseClass::OnRestore();


	SetViewEntity( m_hViewEntity );
	SetDefaultFOV(m_iDefaultFOV);		// force this to reset if zero

	// Calculate this immediately
	m_nVehicleViewSavedFrame = 0;

	m_nBodyPitchPoseParam = LookupPoseParameter( "body_pitch" );

	// HACK: (03/25/09) Then the player goes across a transition it doesn't spawn and register
	// it's instance. We're hacking around this for now, but this will go away when we get around to 
	// having entities cross transitions and keep their script state.
	if ( !g_pGameRules->IsMultiplayer() && g_pScriptVM && (gpGlobals->eLoadType == MapLoad_Transition) )
	{
		g_pScriptVM->SetValue( "player", GetScriptInstance() );
	}
}

/* void CBasePlayer::SetTeamName( const char *pTeamName )
{
	Q_strncpy( m_szTeamName, pTeamName, TEAM_NAME_LENGTH );
} */

void CBasePlayer::SetArmorValue( int value )
{
	m_ArmorValue = value;
}

void CBasePlayer::IncrementArmorValue( int nCount, int nMaxValue )
{ 
	m_ArmorValue += nCount;
	if (nMaxValue > 0)
	{
		if (m_ArmorValue > nMaxValue)
			m_ArmorValue = nMaxValue;
	}
}

// used by the physics gun and game physics... is there a better interface?
void CBasePlayer::SetPhysicsFlag( int nFlag, bool bSet )
{
	if (bSet)
		m_afPhysicsFlags |= nFlag;
	else
		m_afPhysicsFlags &= ~nFlag;
}


void CBasePlayer::NotifyNearbyRadiationSource( float flRange )
{
	// if player's current geiger counter range is larger
	// than range to this trigger hurt, reset player's
	// geiger counter range 

	if (m_flgeigerRange >= flRange)
		m_flgeigerRange = flRange;
}

void CBasePlayer::AllowImmediateDecalPainting()
{
	m_flNextDecalTime = gpGlobals->curtime;
}

// Suicide...
void CBasePlayer::CommitSuicide( bool bExplode /*= false*/, bool bForce /*= false*/ )
{
	MDLCACHE_CRITICAL_SECTION();

	if( !IsAlive() )
		return;
		
	// prevent suiciding too often
	if ( m_fNextSuicideTime > gpGlobals->curtime && !bForce )
		return;

	// don't let them suicide for 5 seconds after suiciding
	m_fNextSuicideTime = gpGlobals->curtime + 5;

	int fDamage = DMG_PREVENT_PHYSICS_FORCE | ( bExplode ? ( DMG_BLAST | DMG_ALWAYSGIB ) : DMG_NEVERGIB );

	// have the player kill themself
	m_iHealth = 0;
	CTakeDamageInfo info( this, this, 0, fDamage, m_iSuicideCustomKillFlags );
	Event_Killed( info );
	Event_Dying( info );
	m_iSuicideCustomKillFlags = 0;
}

// Suicide with style...
void CBasePlayer::CommitSuicide( const Vector &vecForce, bool bExplode /*= false*/, bool bForce /*= false*/ )
{
	MDLCACHE_CRITICAL_SECTION();

	// Already dead.
	if( !IsAlive() )
		return;

	// Prevent suicides for a time.
	if ( m_fNextSuicideTime > gpGlobals->curtime && !bForce )
		return;

	m_fNextSuicideTime = gpGlobals->curtime + 5;  

	// Apply the force.
	int nHealth = GetHealth();

	// Kill the player.
	CTakeDamageInfo info;
	info.SetDamage( nHealth + 10 );
	info.SetAttacker( this );
	info.SetDamageType( bExplode ? DMG_ALWAYSGIB : DMG_GENERIC );
	info.SetDamageForce( vecForce );
	info.SetDamagePosition( WorldSpaceCenter() );
	TakeDamage( info );
}

//==============================================
// HasWeapons - do I have any weapons at all?
//==============================================
bool CBasePlayer::HasWeapons( void )
{
	int i;

	for ( i = 0 ; i < WeaponCount() ; i++ )
	{
		if ( GetWeapon(i) )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecForce - 
//-----------------------------------------------------------------------------
void CBasePlayer::VelocityPunch( const Vector &vecForce )
{
	// Clear onground and add velocity.
	SetGroundEntity( NULL );
	ApplyAbsVelocityImpulse(vecForce );
}


//--------------------------------------------------------------------------------------------------------------
// VEHICLES
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Whether or not the player is currently able to enter the vehicle
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::CanEnterVehicle( IServerVehicle *pVehicle, int nRole )
{
	// Must not have a passenger there already
	if ( pVehicle->GetPassenger( nRole ) )
		return false;

	// Must be able to holster our current weapon (ie. grav gun!)
	if ( pVehicle->IsPassengerUsingStandardWeapons( nRole ) == false )
	{
		//Must be able to stow our weapon
		CBaseCombatWeapon *pWeapon = GetActiveWeapon();
		if ( ( pWeapon != NULL ) && ( pWeapon->CanHolster() == false ) )
			return false;
	}

	// Must be alive
	if ( IsAlive() == false )
		return false;

	// Can't be pulled by a barnacle
	if ( IsEFlagSet( EFL_IS_BEING_LIFTED_BY_BARNACLE ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Put this player in a vehicle 
//-----------------------------------------------------------------------------
bool CBasePlayer::GetInVehicle( IServerVehicle *pVehicle, int nRole )
{
	Assert( NULL == m_hVehicle.Get() );
	Assert( nRole >= 0 );
	
	// Make sure we can enter the vehicle
	if ( CanEnterVehicle( pVehicle, nRole ) == false )
		return false;

	CBaseEntity *pEnt = pVehicle->GetVehicleEnt();
	Assert( pEnt );

	// Try to stow weapons
	if ( pVehicle->IsPassengerUsingStandardWeapons( nRole ) == false )
	{
		CBaseCombatWeapon *pWeapon = GetActiveWeapon();
		if ( pWeapon != NULL )
		{
			pWeapon->Holster( NULL );
		}

#ifndef HL2_DLL
		m_Local.m_iHideHUD |= HIDEHUD_WEAPONSELECTION;
#endif
		m_Local.m_iHideHUD |= HIDEHUD_INVEHICLE;
	}

	if ( !pVehicle->IsPassengerVisible( nRole ) )
	{
		AddEffects( EF_NODRAW );
	}

	// Put us in the vehicle
	pVehicle->SetPassenger( nRole, this );

	ViewPunchReset();

	// Setting the velocity to 0 will cause the IDLE animation to play
	SetAbsVelocity( vec3_origin );
	SetMoveType( MOVETYPE_NOCLIP );

	// This is a hack to fixup the player's stats since they really didn't "cheat" and enter noclip from the console
	gamestats->Event_DecrementPlayerEnteredNoClip( this );

	// Get the seat position we'll be at in this vehicle
	Vector vSeatOrigin;
	QAngle qSeatAngles;
	pVehicle->GetPassengerSeatPoint( nRole, &vSeatOrigin, &qSeatAngles );
	
	// Set us to that position
	SetAbsOrigin( vSeatOrigin );
	SetAbsAngles( qSeatAngles );
	
	// Parent to the vehicle
	SetParent( pEnt );

	SetCollisionGroup( COLLISION_GROUP_IN_VEHICLE );
	
	// We cannot be ducking -- do all this before SetPassenger because it
	// saves our view offset for restoration when we exit the vehicle.
	RemoveFlag( FL_DUCKING );
	SetViewOffset( VEC_VIEW_SCALED( this ) );
	m_Local.m_bDucked = false;
	m_Local.m_bDucking  = false;
	m_Local.m_flDucktime = 0.0f;
	m_Local.m_flDuckJumpTime = 0.0f;
	m_Local.m_flJumpTime = 0.0f;

	// Turn our toggled duck off
	if ( GetToggledDuckState() )
	{
		ToggleDuck();
	}

	m_hVehicle = pEnt;

	// Throw an event indicating that the player entered the vehicle.
	g_pNotify->ReportNamedEvent( this, "PlayerEnteredVehicle" );

	m_iVehicleAnalogBias = VEHICLE_ANALOG_BIAS_NONE;

	OnVehicleStart();

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Remove this player from a vehicle
//-----------------------------------------------------------------------------
void CBasePlayer::LeaveVehicle( const Vector &vecExitPoint, const QAngle &vecExitAngles )
{
	if ( NULL == m_hVehicle.Get() )
		return;

	IServerVehicle *pVehicle = GetVehicle();
	Assert( pVehicle );

	int nRole = pVehicle->GetPassengerRole( this );
	Assert( nRole >= 0 );

	SetParent( NULL );

	// Find the first non-blocked exit point:
	Vector vNewPos = GetAbsOrigin();
	QAngle qAngles = GetAbsAngles();
	if ( vecExitPoint == vec3_origin )
	{
		// FIXME: this might fail to find a safe exit point!!
		pVehicle->GetPassengerExitPoint( nRole, &vNewPos, &qAngles );
	}
	else
	{
		vNewPos = vecExitPoint;
		qAngles = vecExitAngles;
	}
	OnVehicleEnd( vNewPos );
	SetAbsOrigin( vNewPos );
	SetAbsAngles( qAngles );
	// Clear out any leftover velocity
	SetAbsVelocity( vec3_origin );

	qAngles[ROLL] = 0;
	SnapEyeAngles( qAngles );

#ifndef HL2_DLL
	m_Local.m_iHideHUD &= ~HIDEHUD_WEAPONSELECTION;
#endif

	m_Local.m_iHideHUD &= ~HIDEHUD_INVEHICLE;

	RemoveEffects( EF_NODRAW );

	SetMoveType( MOVETYPE_WALK );
	SetCollisionGroup( COLLISION_GROUP_PLAYER );

	if ( VPhysicsGetObject() )
	{
		VPhysicsGetObject()->SetPosition( vNewPos, vec3_angle, true );
	}

	m_hVehicle = NULL;
	pVehicle->SetPassenger(nRole, NULL);

	// Re-deploy our weapon
	if ( IsAlive() )
	{
		if ( GetActiveWeapon() && GetActiveWeapon()->IsWeaponVisible() == false )
		{
			GetActiveWeapon()->Deploy();
			ShowCrosshair( true );
		}
	}

	// Just cut all of the rumble effects. 
	RumbleEffect( RUMBLE_STOP_ALL, 0, RUMBLE_FLAGS_NONE );
}


//==============================================
// !!!UNDONE:ultra temporary SprayCan entity to apply
// decal frame at a time. For PreAlpha CD
//==============================================
class CSprayCan : public CPointEntity
{
public:
	DECLARE_CLASS( CSprayCan, CPointEntity );

	void	Spawn ( CBasePlayer *pOwner );
	void	Think( void );

	virtual void Precache();

	virtual int	ObjectCaps( void ) { return FCAP_DONT_SAVE; }
};

LINK_ENTITY_TO_CLASS( spraycan, CSprayCan );
PRECACHE_REGISTER( spraycan );

void CSprayCan::Spawn ( CBasePlayer *pOwner )
{
	SetLocalOrigin( pOwner->WorldSpaceCenter() + Vector ( 0 , 0 , 32 ) );
	SetLocalAngles( pOwner->EyeAngles() );
	SetOwnerEntity( pOwner );
	SetNextThink( gpGlobals->curtime );
	EmitSound( "SprayCan.Paint" );
}

void CSprayCan::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "SprayCan.Paint" );
}

void CSprayCan::Think( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwnerEntity() );
	if ( pPlayer )
	{
       	int playernum = pPlayer->entindex();
		
		Vector forward;
		trace_t	tr;	

		AngleVectors( GetAbsAngles(), &forward );
		UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + forward * 128, 
			MASK_SOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, & tr);

		UTIL_PlayerDecalTrace( &tr, playernum );
	}
	
	// Just painted last custom frame.
	UTIL_Remove( this );
}

class	CBloodSplat : public CPointEntity
{
public:
	DECLARE_CLASS( CBloodSplat, CPointEntity );

	void	Spawn ( CBaseEntity *pOwner );
	void	Think ( void );
};

void CBloodSplat::Spawn ( CBaseEntity *pOwner )
{
	SetLocalOrigin( pOwner->WorldSpaceCenter() + Vector ( 0 , 0 , 32 ) );
	SetLocalAngles( pOwner->GetLocalAngles() );
	SetOwnerEntity( pOwner );

	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CBloodSplat::Think( void )
{
	trace_t	tr;	
	
	if ( g_Language.GetInt() != LANGUAGE_GERMAN )
	{
		CBasePlayer *pPlayer;
		pPlayer = ToBasePlayer( GetOwnerEntity() );

		Vector forward;
		AngleVectors( GetAbsAngles(), &forward );
		UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + forward * 128, 
			MASK_SOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, & tr);

		UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_RED );
	}
	UTIL_Remove( this );
}

//==============================================

//-----------------------------------------------------------------------------
// Purpose: Create and give the named item to the player. Then return it.
//-----------------------------------------------------------------------------
CBaseEntity	*CBasePlayer::GiveNamedItem( const char *pszName, int iSubType )
{
	// If I already own this type don't create one
	if ( Weapon_OwnsThisType(pszName, iSubType) )
		return NULL;

	// Msg( "giving %s\n", pszName );

	EHANDLE pent;

	pent = CreateEntityByName(pszName);
	if ( pent == NULL )
	{
		Msg( "NULL Ent in GiveNamedItem!\n" );
		return NULL;
	}

	pent->SetLocalOrigin( GetLocalOrigin() );
	pent->AddSpawnFlags( SF_NORESPAWN );

	CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon*>( (CBaseEntity*)pent );
	if ( pWeapon )
	{
		pWeapon->SetSubType( iSubType );
	}

	DispatchSpawn( pent );

#ifdef MAPBASE
	if ( pWeapon )
	{
		for (int i=0;i<MAX_WEAPONS;i++) 
		{
			if ( m_hMyWeapons[i].Get() && m_hMyWeapons[i]->GetSlot() == pWeapon->GetSlot() && m_hMyWeapons[i]->GetPosition() == pWeapon->GetPosition() )
			{
				// Make sure it matches the subtype
				if ( m_hMyWeapons[i]->GetSubType() == iSubType )
				{
					// Don't use this weapon if the slot is already occupied
					UTIL_Remove( pWeapon );
					return NULL;
				}
			}
		}
	}
#endif

	if ( pent != NULL && !(pent->IsMarkedForDeletion()) ) 
	{
		pent->Touch( this );
	}

	return pent;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the nearest COLLIBALE entity in front of the player
//			that has a clear line of sight with the given classname
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity *FindEntityClassForward( CBasePlayer *pMe, char *classname )
{
	trace_t tr;

	Vector forward;
	pMe->EyeVectors( &forward );
	UTIL_TraceLine(pMe->EyePosition(),
		pMe->EyePosition() + forward * MAX_COORD_RANGE,
		MASK_SOLID, pMe, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0 && tr.DidHitNonWorldEntity() )
	{
		CBaseEntity *pHit = tr.m_pEnt;
		if (FClassnameIs( pHit,classname ) )
		{
			return pHit;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the nearest COLLIBALE entity in front of the player
//			that has a clear line of sight. If HULL is true, the trace will
//			hit the collision hull of entities. Otherwise, the trace will hit
//			hitboxes.
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity *FindEntityForward( CBasePlayer *pMe, bool fHull )
{
	if ( pMe )
	{
		trace_t tr;
		Vector forward;
		int mask;

		if( fHull )
		{
			mask = MASK_SOLID;
		}
		else
		{
			mask = MASK_SHOT;
		}

		pMe->EyeVectors( &forward );
		UTIL_TraceLine(pMe->EyePosition(),
			pMe->EyePosition() + forward * MAX_COORD_RANGE,
			mask, pMe, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0 && tr.DidHitNonWorldEntity() )
		{
			return tr.m_pEnt;
		}
	}
	return NULL;

}

//-----------------------------------------------------------------------------
// Purpose: Finds the nearest entity in front of the player of the given
//			classname, preferring collidable entities, but allows selection of 
//			enities that are on the other side of walls or objects
//
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity *FindPickerEntityClass( CBasePlayer *pPlayer, char *classname )
{
	// First try to trace a hull to an entity
	CBaseEntity *pEntity = FindEntityClassForward( pPlayer, classname );

	// If that fails just look for the nearest facing entity
	if (!pEntity) 
	{
		Vector forward;
		Vector origin;
		pPlayer->EyeVectors( &forward );
		origin = pPlayer->WorldSpaceCenter();		
		pEntity = gEntList.FindEntityClassNearestFacing( origin, forward,0.95,classname);
	}
	return pEntity;
}

//-----------------------------------------------------------------------------
// Purpose: Finds the nearest entity in front of the player, preferring
//			collidable entities, but allows selection of enities that are
//			on the other side of walls or objects
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer )
{
	MDLCACHE_CRITICAL_SECTION();

	// First try to trace a hull to an entity
	CBaseEntity *pEntity = FindEntityForward( pPlayer, true );

	// If that fails just look for the nearest facing entity
	if (!pEntity) 
	{
		Vector forward;
		Vector origin;
		pPlayer->EyeVectors( &forward );
		origin = pPlayer->WorldSpaceCenter();		
		pEntity = gEntList.FindEntityNearestFacing( origin, forward,0.95);
	}
	return pEntity;
}

//-----------------------------------------------------------------------------
// Purpose: Finds the nearest node in front of the player
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_Node *FindPickerAINode( CBasePlayer *pPlayer, NodeType_e nNodeType )
{
	Vector forward;
	Vector origin;

	pPlayer->EyeVectors( &forward );
	origin = pPlayer->EyePosition();	
	return g_pAINetworkManager->GetEditOps()->FindAINodeNearestFacing( origin, forward,0.90, nNodeType);
}

//-----------------------------------------------------------------------------
// Purpose: Finds the nearest link in front of the player
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_Link *FindPickerAILink( CBasePlayer* pPlayer )
{
	Vector forward;
	Vector origin;

	pPlayer->EyeVectors( &forward );
	origin = pPlayer->EyePosition();	
	return g_pAINetworkManager->GetEditOps()->FindAILinkNearestFacing( origin, forward,0.90);
}

/*
===============
ForceClientDllUpdate

When recording a demo, we need to have the server tell us the entire client state
so that the client side .dll can behave correctly.
Reset stuff so that the state is transmitted.
===============
*/
void CBasePlayer::ForceClientDllUpdate( void )
{
	m_iClientBattery = -1;
	m_iTrain |= TRAIN_NEW;  // Force new train message.
	m_fWeapon = false;          // Force weapon send

	// Force all HUD data to be resent to client
	m_fInitHUD = true;

	// Now force all the necessary messages
	//  to be sent.
	UpdateClientData();

	UTIL_RestartAmbientSounds(); // MOTODO that updates the sounds for everybody
}

/*
============
ImpulseCommands
============
*/

void CBasePlayer::ImpulseCommands( )
{
	trace_t	tr;
		
	int iImpulse = (int)m_nImpulse;
	switch (iImpulse)
	{
	case 100:
        // temporary flashlight for level designers
        if ( FlashlightIsOn() )
		{
			FlashlightTurnOff();
		}
        else 
		{
			FlashlightTurnOn();
		}
		break;

	case 200:
		if ( sv_cheats->GetBool() )
		{
			CBaseCombatWeapon *pWeapon;

			pWeapon = GetActiveWeapon();
#ifdef MAPBASE
			if (!pWeapon)
				return;
#endif
			
			if( pWeapon->IsEffectActive( EF_NODRAW ) )
			{
				pWeapon->Deploy();
			}
			else
			{
				pWeapon->Holster();
			}
		}
		break;

	case	201:// paint decal
		
		if ( gpGlobals->curtime < m_flNextDecalTime )
		{
			// too early!
			break;
		}

		{
			Vector forward;
			EyeVectors( &forward );
			UTIL_TraceLine ( EyePosition(), 
				EyePosition() + forward * 128, 
				MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, & tr);
		}

		if ( tr.fraction != 1.0 )
		{// line hit something, so paint a decal
			m_flNextDecalTime = gpGlobals->curtime + decalfrequency.GetFloat();
			CSprayCan *pCan = CREATE_UNSAVED_ENTITY( CSprayCan, "spraycan" );
			pCan->Spawn( this );

#ifdef CSTRIKE_DLL
			//=============================================================================
			// HPE_BEGIN:
			// [pfreese] Fire off a game event - the Counter-Strike stats manager listens
			// to these achievements for one of the CS achievements.
			//=============================================================================
			
			IGameEvent * event = gameeventmanager->CreateEvent( "player_decal" );
			if ( event )
			{
				event->SetInt("userid", GetUserID() );
				gameeventmanager->FireEvent( event );
			}

			//=============================================================================
			// HPE_END
			//=============================================================================
#endif			
		}

		break;

	case	202:// player jungle sound 
		if ( gpGlobals->curtime < m_flNextDecalTime )
		{
			// too early!
			break;

		}
		
		EntityMessageBegin( this );
			WRITE_BYTE( PLAY_PLAYER_JINGLE );
		MessageEnd();

		m_flNextDecalTime = gpGlobals->curtime + decalfrequency.GetFloat();
		break;

	default:
		// check all of the cheat impulse commands now
		CheatImpulseCommands( iImpulse );
		break;
	}
	
	m_nImpulse = 0;
}

#ifdef HL2_EPISODIC

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void CreateJalopy( CBasePlayer *pPlayer )
{
	// Cheat to create a jeep in front of the player
	Vector vecForward;
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	CBaseEntity *pJeep = (CBaseEntity *)CreateEntityByName( "prop_vehicle_jeep" );
	if ( pJeep )
	{
		Vector vecOrigin = pPlayer->GetAbsOrigin() + vecForward * 256 + Vector(0,0,64);
		QAngle vecAngles( 0, pPlayer->GetAbsAngles().y - 90, 0 );
		pJeep->SetAbsOrigin( vecOrigin );
		pJeep->SetAbsAngles( vecAngles );
		pJeep->KeyValue( "model", "models/vehicle.mdl" );
		pJeep->KeyValue( "solid", "6" );
		pJeep->KeyValue( "targetname", "jeep" );
		pJeep->KeyValue( "vehiclescript", "scripts/vehicles/jalopy.txt" );
		DispatchSpawn( pJeep );
		pJeep->Activate();
		pJeep->Teleport( &vecOrigin, &vecAngles, NULL );
	}
}

void CC_CH_CreateJalopy( void )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;
	CreateJalopy( pPlayer );
}

static ConCommand ch_createjalopy("ch_createjalopy", CC_CH_CreateJalopy, "Spawn jalopy in front of the player.", FCVAR_CHEAT);

#endif // HL2_EPISODIC

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void CreateJeep( CBasePlayer *pPlayer )
{
	// Cheat to create a jeep in front of the player
	Vector vecForward;
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	//Tony; in sp sdk, we have prop_vehicle_hl2buggy; because episode 2 modified the jeep code to turn it into the jalopy instead of the regular buggy
#if defined ( HL2_EPISODIC )
	CBaseEntity *pJeep = (CBaseEntity *)CreateEntityByName( "prop_vehicle_hl2buggy" );
#else
	CBaseEntity *pJeep = (CBaseEntity *)CreateEntityByName( "prop_vehicle_jeep" );
#endif
	if ( pJeep )
	{
		Vector vecOrigin = pPlayer->GetAbsOrigin() + vecForward * 256 + Vector(0,0,64);
		QAngle vecAngles( 0, pPlayer->GetAbsAngles().y - 90, 0 );
		pJeep->SetAbsOrigin( vecOrigin );
		pJeep->SetAbsAngles( vecAngles );
		pJeep->KeyValue( "model", "models/buggy.mdl" );
		pJeep->KeyValue( "solid", "6" );
#if defined ( HL2_EPISODIC )
		pJeep->KeyValue( "targetname", "hl2buggy" );
#else
		pJeep->KeyValue( "targetname", "jeep" );
#endif
		pJeep->KeyValue( "vehiclescript", "scripts/vehicles/jeep_test.txt" );
		DispatchSpawn( pJeep );
		pJeep->Activate();
		pJeep->Teleport( &vecOrigin, &vecAngles, NULL );
	}
}


void CC_CH_CreateJeep( void )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;
	CreateJeep( pPlayer );
}

static ConCommand ch_createjeep("ch_createjeep", CC_CH_CreateJeep, "Spawn jeep in front of the player.", FCVAR_CHEAT);


//-----------------------------------------------------------------------------
// Create an airboat in front of the specified player
//-----------------------------------------------------------------------------
static void CreateAirboat( CBasePlayer *pPlayer )
{
	// Cheat to create a jeep in front of the player
	Vector vecForward;
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	CBaseEntity *pJeep = ( CBaseEntity* )CreateEntityByName( "prop_vehicle_airboat" );
	if ( pJeep )
	{
		Vector vecOrigin = pPlayer->GetAbsOrigin() + vecForward * 256 + Vector( 0,0,64 );
		QAngle vecAngles( 0, pPlayer->GetAbsAngles().y - 90, 0 );
		pJeep->SetAbsOrigin( vecOrigin );
		pJeep->SetAbsAngles( vecAngles );
		pJeep->KeyValue( "model", "models/airboat.mdl" );
		pJeep->KeyValue( "solid", "6" );
		pJeep->KeyValue( "targetname", "airboat" );
		pJeep->KeyValue( "vehiclescript", "scripts/vehicles/airboat.txt" );
		DispatchSpawn( pJeep );
		pJeep->Activate();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CC_CH_CreateAirboat( void )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;

	CreateAirboat( pPlayer );

}

static ConCommand ch_createairboat( "ch_createairboat", CC_CH_CreateAirboat, "Spawn airboat in front of the player.", FCVAR_CHEAT );


//=========================================================
//=========================================================
void CBasePlayer::CheatImpulseCommands( int iImpulse )
{
#if !defined( HLDEMO_BUILD )
	if ( !sv_cheats->GetBool() )
	{
		return;
	}

	CBaseEntity *pEntity;
	trace_t tr;

	switch ( iImpulse )
	{
	case 76:
		{
			if (!giPrecacheGrunt)
			{
				giPrecacheGrunt = 1;
				Msg( "You must now restart to use Grunt-o-matic.\n");
			}
			else
			{
				Vector forward = UTIL_YawToVector( EyeAngles().y );
				Create("NPC_human_grunt", GetLocalOrigin() + forward * 128, GetLocalAngles());
			}
			break;
		}

	case 81:

		GiveNamedItem( "weapon_cubemap" );
		break;

	case 82:
		// Cheat to create a jeep in front of the player
		CreateJeep( this );
		break;

	case 83:
		// Cheat to create a airboat in front of the player
		CreateAirboat( this );
		break;

	case 101:
		gEvilImpulse101 = true;

		EquipSuit();

		// Give the player everything!
		GiveAmmo( 255,	"Pistol");
		GiveAmmo( 255,	"AR2");
		GiveAmmo( 5,	"AR2AltFire");
		GiveAmmo( 255,	"SMG1");
		GiveAmmo( 255,	"Buckshot");
		GiveAmmo( 3,	"smg1_grenade");
		GiveAmmo( 3,	"rpg_round");
		GiveAmmo( 5,	"grenade");
		GiveAmmo( 32,	"357" );
		GiveAmmo( 16,	"XBowBolt" );
#ifdef HL2_EPISODIC
		GiveAmmo( 5,	"Hopwire" );
#endif		
		GiveNamedItem( "weapon_smg1" );
		GiveNamedItem( "weapon_frag" );
		GiveNamedItem( "weapon_crowbar" );
		GiveNamedItem( "weapon_pistol" );
		GiveNamedItem( "weapon_ar2" );
		GiveNamedItem( "weapon_shotgun" );
		GiveNamedItem( "weapon_physcannon" );
		GiveNamedItem( "weapon_bugbait" );
		GiveNamedItem( "weapon_rpg" );
		GiveNamedItem( "weapon_357" );
		GiveNamedItem( "weapon_crossbow" );
#ifdef HL2_EPISODIC
		// GiveNamedItem( "weapon_magnade" );
#endif
		if ( GetHealth() < 100 )
		{
			TakeHealth( 25, DMG_GENERIC );
		}
		
		gEvilImpulse101		= false;

		break;

	case 102:
		// Gibbage!!!
		CGib::SpawnRandomGibs( this, 1, GIB_HUMAN );
		break;

	case 103:
		// What the hell are you doing?
		pEntity = FindEntityForward( this, true );
		if ( pEntity )
		{
			CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
			if ( pNPC )
				pNPC->ReportAIState();
		}
		break;

	case 106:
		// Give me the classname and targetname of this entity.
		pEntity = FindEntityForward( this, true );
		if ( pEntity )
		{
			Msg( "Classname: %s", pEntity->GetClassname() );
			
			if ( pEntity->GetEntityName() != NULL_STRING )
			{
				Msg( " - Name: %s\n", STRING( pEntity->GetEntityName() ) );
			}
			else
			{
				Msg( " - Name: No Targetname\n" );
			}

			if ( pEntity->m_iParent != NULL_STRING )
				Msg( "Parent: %s\n", STRING(pEntity->m_iParent) );

			Msg( "Model: %s\n", STRING( pEntity->GetModelName() ) );
			if ( pEntity->m_iGlobalname != NULL_STRING )
				Msg( "Globalname: %s\n", STRING(pEntity->m_iGlobalname) );
		}
		break;

	case 107:
		{
			trace_t tr;

			edict_t		*pWorld = engine->PEntityOfEntIndex( 0 );

			Vector start = EyePosition();
			Vector forward;
			EyeVectors( &forward );
			Vector end = start + forward * 1024;
			UTIL_TraceLine( start, end, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
			if ( tr.m_pEnt )
				pWorld = tr.m_pEnt->edict();

			const char *pTextureName = tr.surface.name;

			if ( pTextureName )
				Msg( "Texture: %s\n", pTextureName );
		}
		break;

	//
	// Sets the debug NPC to be the NPC under the crosshair.
	//
	case 108:
	{
		pEntity = FindEntityForward( this, true );
		if ( pEntity )
		{
			CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
			if ( pNPC != NULL )
			{
				Msg( "Debugging %s (0x%p)\n", pNPC->GetClassname(), pNPC );
				CAI_BaseNPC::SetDebugNPC( pNPC );
			}
		}
		break;
	}

	case	195:// show shortest paths for entire level to nearest node
		{
			Create("node_viewer_fly", GetLocalOrigin(), GetLocalAngles());
		}
		break;
	case	196:// show shortest paths for entire level to nearest node
		{
			Create("node_viewer_large", GetLocalOrigin(), GetLocalAngles());
		}
		break;
	case	197:// show shortest paths for entire level to nearest node
		{
			Create("node_viewer_human", GetLocalOrigin(), GetLocalAngles());
		}
		break;
	case	202:// Random blood splatter
		{
			Vector forward;
			EyeVectors( &forward );
			UTIL_TraceLine ( EyePosition(), 
				EyePosition() + forward * 128, 
				MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, & tr);

			if ( tr.fraction != 1.0 )
			{// line hit something, so paint a decal
				CBloodSplat *pBlood = CREATE_UNSAVED_ENTITY( CBloodSplat, "bloodsplat" );
				pBlood->Spawn( this );
			}
		}
		break;
	case	203:// remove creature.
		pEntity = FindEntityForward( this, true );
		if ( pEntity )
		{
			UTIL_Remove( pEntity );
//			if ( pEntity->m_takedamage )
//				pEntity->SetThink(SUB_Remove);
		}
		break;
	}
#endif	// HLDEMO_BUILD
}


bool CBasePlayer::ClientCommand( const CCommand &args )
{
	const char *cmd = args[0];
#ifdef _DEBUG
	if( stricmp( cmd, "test_SmokeGrenade" ) == 0 )
	{
		if ( sv_cheats && sv_cheats->GetBool() )
		{
			ParticleSmokeGrenade *pSmoke = dynamic_cast<ParticleSmokeGrenade*>( CreateEntityByName(PARTICLESMOKEGRENADE_ENTITYNAME) );
			if ( pSmoke )
			{
				Vector vForward;
				AngleVectors( GetLocalAngles(), &vForward );
				vForward.z = 0;
				VectorNormalize( vForward );

				pSmoke->SetLocalOrigin( GetLocalOrigin() + vForward * 100 );
				pSmoke->SetFadeTime(25, 30);	// Fade out between 25 seconds and 30 seconds.
				pSmoke->Activate();
				pSmoke->SetLifetime(30);
				pSmoke->FillVolume();

				return true;
			}
		}
	}
	else
#endif // _DEBUG
	if( stricmp( cmd, "vehicleRole" ) == 0 )
	{
		// Get the vehicle role value.
		if ( args.ArgC() == 2 )
		{
			// Check to see if a player is in a vehicle.
			if ( IsInAVehicle() )
			{
				int nRole = atoi( args[1] );
				IServerVehicle *pVehicle = GetVehicle();
				if ( pVehicle )
				{
					// Only switch roles if role is empty!
					if ( !pVehicle->GetPassenger( nRole ) )
					{
						LeaveVehicle();
						GetInVehicle( pVehicle, nRole );
					}
				}			
			}

			return true;
		}
	}
	else if ( HandleVoteCommands( args ) )
	{
		return true;
	}
	else if ( stricmp( cmd, "spectate" ) == 0 ) // join spectator team & start observer mode
	{
		if ( GetTeamNumber() == TEAM_SPECTATOR )
			return true;

		ConVarRef mp_allowspectators( "mp_allowspectators" );
		if ( mp_allowspectators.IsValid() )
		{
			if ( ( mp_allowspectators.GetBool() == false ) && !IsHLTV() && !IsReplay() )
			{
				ClientPrint( this, HUD_PRINTCENTER, "#Cannot_Be_Spectator" );
				return true;
			}
		}

		if ( !IsDead() )
		{
			CommitSuicide();	// kill player
		}

		RemoveAllItems( true );

		ChangeTeam( TEAM_SPECTATOR );

		StartObserverMode( OBS_MODE_ROAMING );
		return true;
	}
	else if ( stricmp( cmd, "spec_mode" ) == 0 ) // new observer mode
	{
		int mode;

		if ( GetObserverMode() == OBS_MODE_FREEZECAM )
		{
			AttemptToExitFreezeCam();
			return true;
		}

		// not allowed to change spectator modes when mp_fadetoblack is being used
		if ( mp_fadetoblack.GetBool() )
		{
			if ( GetTeamNumber() > TEAM_SPECTATOR )
				return true;
		}

		// check for parameters.
		if ( args.ArgC() >= 2 )
		{
			mode = atoi( args[1] );

			if ( mode < OBS_MODE_IN_EYE || mode > LAST_PLAYER_OBSERVERMODE )
				mode = OBS_MODE_IN_EYE;
		}
		else
		{
			// switch to next spec mode if no parameter given
 			mode = GetObserverMode() + 1;
			
			if ( mode > LAST_PLAYER_OBSERVERMODE )
			{
				mode = OBS_MODE_IN_EYE;
			}
			else if ( mode < OBS_MODE_IN_EYE )
			{
				mode = OBS_MODE_ROAMING;
			}

		}
	
		// don't allow input while player or death cam animation
		if ( GetObserverMode() > OBS_MODE_DEATHCAM )
		{
			// set new spectator mode, don't allow OBS_MODE_NONE
			if ( !SetObserverMode( mode ) )
				ClientPrint( this, HUD_PRINTCONSOLE, "#Spectator_Mode_Unkown");
			else
				engine->ClientCommand( edict(), "cl_spec_mode %d", mode );
		}
		else
		{
			// remember spectator mode for later use
			m_iObserverLastMode = mode;
			engine->ClientCommand( edict(), "cl_spec_mode %d", mode );
		}

		return true;
	}
	else if ( stricmp( cmd, "spec_next" ) == 0 ) // chase next player
	{
		if ( GetObserverMode() > OBS_MODE_FIXED )
		{
			// set new spectator mode
			CBaseEntity * target = FindNextObserverTarget( false );
			if ( target )
			{
				SetObserverTarget( target );
			}
		}
		else if ( GetObserverMode() == OBS_MODE_FREEZECAM )
		{
			AttemptToExitFreezeCam();
		}
		
		return true;
	}
	else if ( stricmp( cmd, "spec_prev" ) == 0 ) // chase prevoius player
	{
		if ( GetObserverMode() > OBS_MODE_FIXED )
		{
			// set new spectator mode
			CBaseEntity * target = FindNextObserverTarget( true );
			if ( target )
			{
				SetObserverTarget( target );
			}
		}
		else if ( GetObserverMode() == OBS_MODE_FREEZECAM )
		{
			AttemptToExitFreezeCam();
		}
		
		return true;
	}
	
	else if ( stricmp( cmd, "spec_player" ) == 0 ) // chase next player
	{
		if ( GetObserverMode() > OBS_MODE_FIXED && args.ArgC() == 2 )
		{
			int index = atoi( args[1] );

			CBasePlayer * target;

			if ( index == 0 )
			{
				target = UTIL_PlayerByName( args[1] );
			}
			else
			{
				target = UTIL_PlayerByIndex( index );
			}

			if ( IsValidObserverTarget( target ) )
			{
				SetObserverTarget( target );
			}
		}
		
		return true;
	}

	else if ( stricmp( cmd, "spec_goto" ) == 0 ) // chase next player
	{
		if ( ( GetObserverMode() == OBS_MODE_FIXED ||
			   GetObserverMode() == OBS_MODE_ROAMING ) &&
			 args.ArgC() == 6 )
		{
			Vector origin;
			origin.x = atof( args[1] );
			origin.y = atof( args[2] );
			origin.z = atof( args[3] );

			QAngle angle;
			angle.x = atof( args[4] );
			angle.y = atof( args[5] );
			angle.z = 0.0f;

#ifdef MAPBASE
			#define SPECGOTO_MAX_VALUE 0xFFFF/2.0f

            // This could crash the game somehow if not checked.. Thanks to Nairda.
            if (abs(angle.x) <= 360.0f && abs(angle.y) <= 360.0f && abs(origin.x) < SPECGOTO_MAX_VALUE &&
                abs(origin.y) < SPECGOTO_MAX_VALUE && abs(origin.z) < SPECGOTO_MAX_VALUE)
            {
                JumptoPosition(origin, angle);
            }
            else
            {
                engine->ClientPrintf(edict(), "spec_goto: Out-of-bounds");
            }
#else
			JumptoPosition( origin, angle );
#endif
		}
		
		return true;
	}
	else if ( stricmp( cmd, "playerperf" ) == 0 )
	{
		int nRecip = entindex();
		if ( args.ArgC() >= 2 )
		{
			nRecip = clamp( Q_atoi( args.Arg( 1 ) ), 1, gpGlobals->maxClients );
		}
		int nRecords = -1; // all
		if ( args.ArgC() >= 3 )
		{
			nRecords = MAX( Q_atoi( args.Arg( 2 ) ), 1 );
		}

		CBasePlayer *pl = UTIL_PlayerByIndex( nRecip );
		if ( pl )
		{
			pl->DumpPerfToRecipient( this, nRecords );
		}
		return true;
	}

	return false;
}

extern bool UTIL_ItemCanBeTouchedByPlayer( CBaseEntity *pItem, CBasePlayer *pPlayer );

//-----------------------------------------------------------------------------
// Purpose: Player reacts to bumping a weapon. 
// Input  : pWeapon - the weapon that the player bumped into.
// Output : Returns true if player picked up the weapon
//-----------------------------------------------------------------------------
bool CBasePlayer::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
	CBaseCombatCharacter *pOwner = pWeapon->GetOwner();

	// Can I have this weapon type?
	if ( !IsAllowedToPickupWeapons() )
		return false;

	if ( pOwner || !Weapon_CanUse( pWeapon ) || !g_pGameRules->CanHavePlayerItem( this, pWeapon ) )
	{
		if ( gEvilImpulse101 )
		{
			UTIL_Remove( pWeapon );
		}
		return false;
	}

	// Act differently in the episodes
	if ( hl2_episodic.GetBool() )
	{
		// Don't let the player touch the item unless unobstructed
		if ( !UTIL_ItemCanBeTouchedByPlayer( pWeapon, this ) && !gEvilImpulse101 )
			return false;
	}
	else
	{
		// Don't let the player fetch weapons through walls (use MASK_SOLID so that you can't pickup through windows)
#ifdef MAPBASE
		if( (pWeapon->FVisible( this, MASK_SOLID ) == false && !(GetFlags() & FL_NOTARGET)) && !HasSpawnFlags(SF_WEAPON_ALWAYS_TOUCHABLE) )
#else
		if( pWeapon->FVisible( this, MASK_SOLID ) == false && !(GetFlags() & FL_NOTARGET) )
#endif
			return false;
	}
	
	// ----------------------------------------
	// If I already have it just take the ammo
	// ----------------------------------------
	if (Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType())) 
	{
		if( Weapon_EquipAmmoOnly( pWeapon ) )
		{
			// Only remove me if I have no ammo left
#ifdef MAPBASE
			if ( pWeapon->HasPrimaryAmmo() || pWeapon->HasSecondaryAmmo() )
#else
			if ( pWeapon->HasPrimaryAmmo() )
#endif
				return false;

			UTIL_Remove( pWeapon );
			return true;
		}
		else
		{
			return false;
		}
	}
#ifdef MAPBASE
	// --------------------------------------------------------------------------------
	// If we own a weapon in the same position take the ammo but leave the weapon behind
	// --------------------------------------------------------------------------------
	if (!pWeapon->HasSpawnFlags(SF_WEAPON_USED)) // Make sure we're being used and not being bumped
	{
		for (int i=0;i<MAX_WEAPONS;i++) 
		{
			if (m_hMyWeapons[i] &&
				pWeapon->GetSlot() == m_hMyWeapons[i]->GetSlot() &&
				pWeapon->GetPosition() == m_hMyWeapons[i]->GetPosition())
			{
				//Weapon_EquipAmmoOnly( pWeapon );

				// Weapon_EquipAmmoOnly checks the array again, which isn't necessary here
				int	primaryGiven	= (pWeapon->UsesClipsForAmmo1()) ? pWeapon->m_iClip1 : pWeapon->GetPrimaryAmmoCount();
				int secondaryGiven	= (pWeapon->UsesClipsForAmmo2()) ? pWeapon->m_iClip2 : pWeapon->GetSecondaryAmmoCount();

				int takenPrimary   = GiveAmmo( primaryGiven, pWeapon->m_iPrimaryAmmoType); 
				int takenSecondary = GiveAmmo( secondaryGiven, pWeapon->m_iSecondaryAmmoType); 
				
				if( pWeapon->UsesClipsForAmmo1() )
				{
					pWeapon->m_iClip1 -= takenPrimary;
				}
				else
				{
					pWeapon->SetPrimaryAmmoCount( pWeapon->GetPrimaryAmmoCount() - takenPrimary );
				}

				if( pWeapon->UsesClipsForAmmo2() )
				{
					pWeapon->m_iClip2 -= takenSecondary;
				}
				else
				{
					pWeapon->SetSecondaryAmmoCount( pWeapon->GetSecondaryAmmoCount() - takenSecondary );
				}

				return false;
			}
		}
	}
#endif
	// -------------------------
	// Otherwise take the weapon
	// -------------------------
#ifndef MAPBASE
	else 
#endif
	{
		pWeapon->CheckRespawn();

		pWeapon->AddSolidFlags( FSOLID_NOT_SOLID );
		pWeapon->AddEffects( EF_NODRAW );

		Weapon_Equip( pWeapon );
		if ( IsInAVehicle() )
		{
			pWeapon->Holster();
		}
		else
		{
#ifdef HL2_DLL

			if ( IsX360() )
			{
				CFmtStr hint;
				hint.sprintf( "#valve_hint_select_%s", pWeapon->GetClassname() );
				UTIL_HudHintText( this, hint.Access() );
			}

#ifndef MAPBASE // See CBasePlayer::Weapon_Equip.
			// Always switch to a newly-picked up weapon
			if ( !PlayerHasMegaPhysCannon() )
			{
				// If it uses clips, load it full. (this is the first time you've picked up this type of weapon)
				if ( pWeapon->UsesClipsForAmmo1() )
				{
					pWeapon->m_iClip1 = pWeapon->GetMaxClip1();
				}

				Weapon_Switch( pWeapon );
			}
#endif
#endif
		}
		return true;
	}
}


bool CBasePlayer::RemovePlayerItem( CBaseCombatWeapon *pItem )
{
	if (GetActiveWeapon() == pItem)
	{
		ResetAutoaim( );
		pItem->Holster( );
		pItem->SetNextThink( TICK_NEVER_THINK );; // crowbar may be trying to swing again, etc
		pItem->SetThink( NULL );
	}

	if ( m_hLastWeapon.Get() == pItem )
	{
		Weapon_SetLast( NULL );
	}

	return Weapon_Detach( pItem );
}


//-----------------------------------------------------------------------------
// Purpose: Hides or shows the player's view model. The "r_drawviewmodel" cvar
//			can still hide the viewmodel even if this is set to true.
// Input  : bShow - true to show, false to hide the view model.
//-----------------------------------------------------------------------------
void CBasePlayer::ShowViewModel(bool bShow)
{
	m_Local.m_bDrawViewmodel = bShow;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bDraw - 
//-----------------------------------------------------------------------------
void CBasePlayer::ShowCrosshair( bool bShow )
{
	if ( bShow )
	{
		m_Local.m_iHideHUD &= ~HIDEHUD_CROSSHAIR;
	}
	else
	{
		m_Local.m_iHideHUD |= HIDEHUD_CROSSHAIR;
	}
}

//-----------------------------------------------------------------------------
// Used by vscript to determine if the player is noclipping
//-----------------------------------------------------------------------------
bool CBasePlayer::ScriptIsPlayerNoclipping(void)
{
	return (GetMoveType() == MOVETYPE_NOCLIP);
}

#ifdef MAPBASE_VSCRIPT
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
HSCRIPT CBasePlayer::VScriptGetExpresser()
{
	HSCRIPT hScript = NULL;
	CAI_Expresser *pExpresser = GetExpresser();
	if (pExpresser)
	{
		hScript = g_pScriptVM->RegisterInstance( pExpresser );
	}

	return hScript;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
HSCRIPT CBasePlayer::ScriptGetViewModel( int viewmodelindex )
{
	if (viewmodelindex < 0 || viewmodelindex >= MAX_VIEWMODELS)
	{
		Warning( "GetViewModel: Invalid index '%i'\n", viewmodelindex );
		return NULL;
	}

	return ToHScript( GetViewModel( viewmodelindex ) );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
QAngle CBasePlayer::BodyAngles()
{
	return EyeAngles();
}

//------------------------------------------------------------------------------
// Purpose : Add noise to BodyTarget() to give enemy a better chance of
//			 getting a clear shot when the player is peeking above a hole
//			 or behind a ladder (eventually the randomly-picked point 
//			 along the spine will be one that is exposed above the hole or 
//			 between rungs of a ladder.)
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CBasePlayer::BodyTarget( const Vector &posSrc, bool bNoisy ) 
{ 
	if ( IsInAVehicle() )
	{
		return GetVehicle()->GetVehicleEnt()->BodyTarget( posSrc, bNoisy );
	}
	if (bNoisy)
	{
		return GetAbsOrigin() + (GetViewOffset() * random->RandomFloat( 0.7, 1.0 )); 
	}
	else
	{
		return EyePosition(); 
	}
};		

/*
=========================================================
	UpdateClientData

resends any changed player HUD info to the client.
Called every frame by PlayerPreThink
Also called at start of demo recording and playback by
ForceClientDllUpdate to ensure the demo gets messages
reflecting all of the HUD state info.
=========================================================
*/
void CBasePlayer::UpdateClientData( void )
{
	CSingleUserRecipientFilter user( this );
	user.MakeReliable();

	if (m_fInitHUD)
	{
		m_fInitHUD = false;
		gInitHUD = false;

		UserMessageBegin( user, "ResetHUD" );
			WRITE_BYTE( 0 );
		MessageEnd();

		if ( !m_fGameHUDInitialized )
		{
			g_pGameRules->InitHUD( this );
			InitHUD();
			m_fGameHUDInitialized = true;
			if ( g_pGameRules->IsMultiplayer() )
			{
				variant_t value;
				g_EventQueue.AddEvent( "game_player_manager", "OnPlayerJoin", value, 0, this, this );
			}
		}

		variant_t value;
		g_EventQueue.AddEvent( "game_player_manager", "OnPlayerSpawn", value, 0, this, this );
	}

	// HACKHACK -- send the message to display the game title
	CWorld *world = GetWorldEntity();
	if ( world && world->GetDisplayTitle() )
	{
		UserMessageBegin( user, "GameTitle" );
		MessageEnd();
		world->SetDisplayTitle( false );
	}

	if (m_ArmorValue != m_iClientBattery)
	{
		m_iClientBattery = m_ArmorValue;

		// send "battery" update message
		if ( usermessages->LookupUserMessage( "Battery" ) != -1 )
		{
			UserMessageBegin( user, "Battery" );
				WRITE_SHORT( (int)m_ArmorValue);
			MessageEnd();
		}
	}

#if 0 // BYE BYE!!
	// Update Flashlight
	if ((m_flFlashLightTime) && (m_flFlashLightTime <= gpGlobals->curtime))
	{
		if (FlashlightIsOn())
		{
			if (m_iFlashBattery)
			{
				m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->curtime;
				m_iFlashBattery--;
				
				if (!m_iFlashBattery)
					FlashlightTurnOff();
			}
		}
		else
		{
			if (m_iFlashBattery < 100)
			{
				m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->curtime;
				m_iFlashBattery++;
			}
			else
				m_flFlashLightTime = 0;
		}
	}
#endif 

	CheckTrainUpdate();

	// Update all the items
	for ( int i = 0; i < WeaponCount(); i++ )
	{
		if ( GetWeapon(i) )  // each item updates it's successors
			GetWeapon(i)->UpdateClientData( this );
	}

	// update the client with our poison state
	m_Local.m_bPoisoned = ( m_bitsDamageType & DMG_POISON ) 
						&& ( m_nPoisonDmg > m_nPoisonRestored ) 
						&& ( m_iHealth < 100 );

	// Check if the bonus progress HUD element should be displayed
	if ( m_iBonusChallenge == 0 && m_iBonusProgress == 0 && !( m_Local.m_iHideHUD & HIDEHUD_BONUS_PROGRESS ) )
		m_Local.m_iHideHUD |= HIDEHUD_BONUS_PROGRESS;
	if ( ( m_iBonusChallenge != 0 )&& ( m_Local.m_iHideHUD & HIDEHUD_BONUS_PROGRESS ) )
		m_Local.m_iHideHUD &= ~HIDEHUD_BONUS_PROGRESS;

	// Let any global rules update the HUD, too
	g_pGameRules->UpdateClientData( this );
}

void CBasePlayer::RumbleEffect( unsigned char index, unsigned char rumbleData, unsigned char rumbleFlags )
{
	if( !IsAlive() )
		return;

	CSingleUserRecipientFilter filter( this );
	filter.MakeReliable();

	UserMessageBegin( filter, "Rumble" );
	WRITE_BYTE( index );
	WRITE_BYTE( rumbleData );
	WRITE_BYTE( rumbleFlags	);
	MessageEnd();
}

void CBasePlayer::EnableControl(bool fControl)
{
	if (!fControl)
		AddFlag( FL_FROZEN );
	else
		RemoveFlag( FL_FROZEN );

}

void CBasePlayer::CheckTrainUpdate( void )
{
	if ( ( m_iTrain & TRAIN_NEW ) )
	{
		CSingleUserRecipientFilter user( this );
		user.MakeReliable();

		// send "Train" update message
		UserMessageBegin( user, "Train" );
			WRITE_BYTE(m_iTrain & 0xF);
		MessageEnd();

		m_iTrain &= ~TRAIN_NEW;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether the player should autoaim or not
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::ShouldAutoaim( void )
{
	// cannot be in multiplayer
	if ( gpGlobals->maxClients > 1 )
		return false;

	// autoaiming is only for easy and medium skill
	return ( IsX360() || !g_pGameRules->IsSkillLevel(SKILL_HARD) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CBasePlayer::GetAutoaimVector( float flScale )
{
	autoaim_params_t params;

	params.m_fScale = flScale;
	params.m_fMaxDist = autoaim_max_dist.GetFloat();

	GetAutoaimVector( params );
	return params.m_vecAutoAimDir;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CBasePlayer::GetAutoaimVector( float flScale, float flMaxDist )
{
	autoaim_params_t params;

	params.m_fScale = flScale;
	params.m_fMaxDist = flMaxDist;

	GetAutoaimVector( params );
	return params.m_vecAutoAimDir;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBasePlayer::GetAutoaimVector( autoaim_params_t &params )
{
	// Assume autoaim will not be assisting.
	params.m_bAutoAimAssisting = false;

	if ( ( ShouldAutoaim() == false ) || ( params.m_fScale == AUTOAIM_SCALE_DIRECT_ONLY ) )
	{
		Vector	forward;
		AngleVectors( EyeAngles() + m_Local.m_vecPunchAngle, &forward );

		params.m_vecAutoAimDir = forward;
		params.m_hAutoAimEntity.Set(NULL);
		params.m_vecAutoAimPoint = vec3_invalid;
		params.m_bAutoAimAssisting = false;
		return;
	}

	Vector vecSrc	= Weapon_ShootPosition( );

	m_vecAutoAim.Init( 0.0f, 0.0f, 0.0f );

	QAngle angles = AutoaimDeflection( vecSrc, params );

	// update ontarget if changed
	if ( !g_pGameRules->AllowAutoTargetCrosshair() )
		m_fOnTarget = false;

	if (angles.x > 180)
		angles.x -= 360;
	if (angles.x < -180)
		angles.x += 360;
	if (angles.y > 180)
		angles.y -= 360;
	if (angles.y < -180)
		angles.y += 360;

	if (angles.x > 25)
		angles.x = 25;
	if (angles.x < -25)
		angles.x = -25;
	if (angles.y > 12)
		angles.y = 12;
	if (angles.y < -12)
		angles.y = -12;

	Vector	forward;

	if( IsInAVehicle() && g_pGameRules->GetAutoAimMode() == AUTOAIM_ON_CONSOLE )
	{
		m_vecAutoAim = angles;
		AngleVectors( EyeAngles() + m_vecAutoAim, &forward );
	}
	else
	{
		// always use non-sticky autoaim
		m_vecAutoAim = angles * 0.9f;
		AngleVectors( EyeAngles() + m_Local.m_vecPunchAngle + m_vecAutoAim, &forward );
	}

	params.m_vecAutoAimDir = forward;
}

//-----------------------------------------------------------------------------
// Targets represent themselves to autoaim as a viewplane-parallel disc with
// a radius specified by the target. The player then modifies this radius
// to achieve more or less aggressive aiming assistance
//-----------------------------------------------------------------------------
float CBasePlayer::GetAutoaimScore( const Vector &eyePosition, const Vector &viewDir, const Vector &vecTarget, CBaseEntity *pTarget, float fScale, CBaseCombatWeapon *pActiveWeapon )
{
	float radiusSqr;
	float targetRadius = pTarget->GetAutoAimRadius() * fScale;

	if( pActiveWeapon != NULL )
		targetRadius *= pActiveWeapon->WeaponAutoAimScale();

	float targetRadiusSqr = Square( targetRadius );

	Vector vecNearestPoint = PointOnLineNearestPoint( eyePosition, eyePosition + viewDir * 8192, vecTarget );
	Vector vecDiff = vecTarget - vecNearestPoint;

	radiusSqr = vecDiff.LengthSqr();

	if( radiusSqr <= targetRadiusSqr )
	{
		float score;

		score = 1.0f - (radiusSqr / targetRadiusSqr);

		Assert( score >= 0.0f && score <= 1.0f );
		return score;
	}
	
	// 0 means no score- doesn't qualify for autoaim.
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecSrc - 
//			flDist - 
//			flDelta - 
// Output : Vector
//-----------------------------------------------------------------------------
QAngle CBasePlayer::AutoaimDeflection( Vector &vecSrc, autoaim_params_t &params )
{
	float		bestscore;
	float		score;
	QAngle		eyeAngles;
	Vector		bestdir;
	CBaseEntity	*bestent;
	trace_t		tr;
	Vector		v_forward, v_right, v_up;

	if ( ShouldAutoaim() == false )
	{
		m_fOnTarget = false;
		return vec3_angle;
	}

	eyeAngles = EyeAngles();
	AngleVectors( eyeAngles + m_Local.m_vecPunchAngle + m_vecAutoAim, &v_forward, &v_right, &v_up );

	// try all possible entities
	bestdir = v_forward;
	bestscore = 0.0f;
	bestent = NULL;

	//Reset this data
	m_fOnTarget					= false;
	params.m_bOnTargetNatural	= false;
	
	CBaseEntity *pIgnore = NULL;

	if( IsInAVehicle() )
	{
		pIgnore = GetVehicleEntity();
	}

	CTraceFilterSkipTwoEntities traceFilter( this, pIgnore, COLLISION_GROUP_NONE );

	UTIL_TraceLine( vecSrc, vecSrc + bestdir * MAX_COORD_FLOAT, MASK_SHOT, &traceFilter, &tr );

	CBaseEntity *pEntHit = tr.m_pEnt;

	if ( pEntHit && pEntHit->m_takedamage != DAMAGE_NO && pEntHit->GetHealth() > 0 )
	{
		// don't look through water
		if (!((GetWaterLevel() != 3 && pEntHit->GetWaterLevel() == 3) || (GetWaterLevel() == 3 && pEntHit->GetWaterLevel() == 0)))
		{
			if( pEntHit->ShouldAttractAutoAim(this) )
			{
				bool bAimAtThis = true;

				if( pEntHit->IsNPC() && g_pGameRules->GetAutoAimMode() > AUTOAIM_NONE )
				{
					int iRelationType = GetDefaultRelationshipDisposition( pEntHit->Classify() );

					if( iRelationType != D_HT )
					{
						bAimAtThis = false;
					}
				}

				if( bAimAtThis )
				{
					if ( pEntHit->GetFlags() & FL_AIMTARGET )
					{
						m_fOnTarget = true;
					}

					// Player is already on target naturally, don't autoaim.
					// Fill out the autoaim_params_t struct, though.
					params.m_hAutoAimEntity.Set(pEntHit);
					params.m_vecAutoAimDir = bestdir;
					params.m_vecAutoAimPoint = tr.endpos;
					params.m_bAutoAimAssisting = false;
					params.m_bOnTargetNatural = true;
					return vec3_angle;
				}
			}

			//Fall through and look for an autoaim ent.
		}
	}

	int count = AimTarget_ListCount();
	if ( count )
	{
		CBaseEntity **pList = (CBaseEntity **)stackalloc( sizeof(CBaseEntity *) * count );
		AimTarget_ListCopy( pList, count );

		for ( int i = 0; i < count; i++ )
		{
			Vector center;
			Vector dir;
			CBaseEntity *pEntity = pList[i];

			// Don't autoaim at anything that doesn't want to be.
			if( !pEntity->ShouldAttractAutoAim(this) )
				continue;

			// Don't shoot yourself
			if ( pEntity == this )
				continue;

			if ( (pEntity->IsNPC() && !pEntity->IsAlive()) || !pEntity->edict() )
				continue;

			if ( !g_pGameRules->ShouldAutoAim( this, pEntity->edict() ) )
				continue;

			// don't look through water
			if ((GetWaterLevel() != 3 && pEntity->GetWaterLevel() == 3) || (GetWaterLevel() == 3 && pEntity->GetWaterLevel() == 0))
				continue;

			if( pEntity->MyNPCPointer() )
			{
				// If this entity is an NPC, only aim if it is an enemy.
				if ( IRelationType( pEntity ) != D_HT )
				{
					if ( !pEntity->IsPlayer() && !g_pGameRules->IsDeathmatch())
						// Msg( "friend\n");
						continue;
				}
			}

			// Don't autoaim at the noisy bodytarget, this makes the autoaim crosshair wobble.
			//center = pEntity->BodyTarget( vecSrc, false );
			center = pEntity->WorldSpaceCenter();

			dir = (center - vecSrc);
			
			float dist = dir.Length2D();
			VectorNormalize( dir );

			// Skip if out of range.
			if( dist > params.m_fMaxDist )
				continue;

			float dot = DotProduct (dir, v_forward );

			// make sure it's in front of the player
			if( dot < 0 )
				continue;

			if( !(pEntity->GetFlags() & FL_FLY) )
			{
				// Refuse to take wild shots at targets far from reticle.
				if( GetActiveWeapon() != NULL && dot < GetActiveWeapon()->GetMaxAutoAimDeflection() )
				{
					// Be lenient if the player is looking down, though. 30 degrees through 90 degrees of pitch.
					// (90 degrees is looking down at player's own 'feet'. Looking straight ahead is 0 degrees pitch.
					// This was done for XBox to make it easier to fight headcrabs around the player's feet.
					if( eyeAngles.x < 30.0f || eyeAngles.x > 90.0f || g_pGameRules->GetAutoAimMode() != AUTOAIM_ON_CONSOLE )
					{
						continue;
					}
				}
			}

			score = GetAutoaimScore(vecSrc, v_forward, pEntity->GetAutoAimCenter(), pEntity, params.m_fScale, GetActiveWeapon() );

			if( score <= bestscore )
			{
				continue;
			}

			UTIL_TraceLine( vecSrc, center, MASK_SHOT, &traceFilter, &tr );

			if (tr.fraction != 1.0 && tr.m_pEnt != pEntity )
			{
				// Msg( "hit %s, can't see %s\n", STRING( tr.u.ent->classname ), STRING( pEdict->classname ) );
				continue;
			}

			// This is the best candidate so far.
			bestscore = score;
			bestent = pEntity;
			bestdir = dir;
		}
		if ( bestent )
		{
			QAngle bestang;

			VectorAngles( bestdir, bestang );

			if( IsInAVehicle() )
			{
				bestang -= EyeAngles();
			}
			else
			{
				bestang -= EyeAngles() - m_Local.m_vecPunchAngle;
			}

			m_fOnTarget = true;

			// Autoaim detected a target for us. Aim automatically at its bodytarget.
			params.m_hAutoAimEntity.Set(bestent);
			params.m_vecAutoAimDir = bestdir;
			params.m_vecAutoAimPoint = bestent->BodyTarget( vecSrc, false );
			params.m_bAutoAimAssisting = true;

			return bestang;
		}
	}

	return vec3_angle;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::ResetAutoaim( void )
{
	if (m_vecAutoAim.x != 0 || m_vecAutoAim.y != 0)
	{
		m_vecAutoAim = QAngle( 0, 0, 0 );
		engine->CrosshairAngle( edict(), 0, 0 );
	}
	m_fOnTarget = false;
}

#ifdef MAPBASE
ConVar  player_debug_probable_aim_target( "player_debug_probable_aim_target", "0", FCVAR_CHEAT, "" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CBasePlayer::GetProbableAimTarget( const Vector &vecSrc, const Vector &vecDir )
{
	trace_t tr;
	CBaseEntity *pIgnore = NULL;
	if (IsInAVehicle())
		pIgnore = GetVehicleEntity();

	CTraceFilterSkipTwoEntities traceFilter( this, pIgnore, COLLISION_GROUP_NONE );

	// Based on dot product and distance
	// If we aim directly at something, only return it if there's not a larger entity slightly off-center
	// Should be weighted based on whether an entity is a NPC, etc.
	CBaseEntity *pBestEnt = NULL;
	float flBestWeight = 0.0f;
	for (CBaseEntity *pEntity = UTIL_EntitiesInPVS( this, NULL ); pEntity; pEntity = UTIL_EntitiesInPVS( this, pEntity ))
	{
		// Combat characters can be unviewable if they just died
		if (!pEntity->IsViewable() && !pEntity->IsCombatCharacter())
			continue;

		if (pEntity == this || pEntity->GetMoveParent() == this || pEntity == GetVehicleEntity())
			continue;

		Vector vecEntDir = (pEntity->EyePosition() - vecSrc);
		float flDot = DotProduct( vecEntDir.Normalized(), vecDir);

		if (flDot < m_flFieldOfView)
			continue;

		// Make sure we can see it
		UTIL_TraceLine( vecSrc, pEntity->EyePosition(), MASK_SHOT, &traceFilter, &tr );
		if (tr.m_pEnt != pEntity)
		{
			if (pEntity->IsCombatCharacter())
			{
				// Trace between centers as well just in case our eyes are blocked
				UTIL_TraceLine( WorldSpaceCenter(), pEntity->WorldSpaceCenter(), MASK_SHOT, &traceFilter, &tr );
				if (tr.m_pEnt != pEntity)
					continue;
			}
			else
				continue;
		}

		float flWeight = flDot - (vecEntDir.LengthSqr() / Square( 2048.0f ));

		if (pEntity->IsCombatCharacter())
		{
			// Hostile NPCs are more likely targets
			if (IRelationType( pEntity ) <= D_FR)
				flWeight += 0.5f;
		}
		else if (pEntity->GetFlags() & FL_AIMTARGET)
		{
			// FL_AIMTARGET is often used for props like explosive barrels
			flWeight += 0.25f;
		}

		if (player_debug_probable_aim_target.GetBool())
		{
			float flWeightClamped = 1.0f - RemapValClamped( flWeight, -2.0f, 2.0f, 0.0f, 1.0f );
			pEntity->EntityText( 0, UTIL_VarArgs( "%f", flWeight ), 2.0f, flWeightClamped * 255.0f, 255.0f, flWeightClamped * 255.0f, 255 );
		}

		if (flWeight > flBestWeight)
		{
			pBestEnt = pEntity;
			flBestWeight = flWeight;
		}
	}
	
	if (player_debug_probable_aim_target.GetBool())
	{
		Msg( "Best probable aim target is %s\n", pBestEnt->GetDebugName() );
		NDebugOverlay::EntityBounds( pBestEnt, 255, 100, 0, 0, 2.0f );
	}

	return pBestEnt;
}
#endif

// ==========================================================================
//	> Weapon stuff
// ==========================================================================

//-----------------------------------------------------------------------------
// Purpose: Override base class, player can always use weapon
// Input  : A weapon
// Output :	true or false
//-----------------------------------------------------------------------------
bool CBasePlayer::Weapon_CanUse( CBaseCombatWeapon *pWeapon )
{
	return true;
}



//-----------------------------------------------------------------------------
// Purpose: Override to clear dropped weapon from the hud
//-----------------------------------------------------------------------------
void CBasePlayer::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget /* = NULL */, const Vector *pVelocity /* = NULL */ )
{
	bool bWasActiveWeapon = false;
	if ( pWeapon == GetActiveWeapon() )
	{
		bWasActiveWeapon = true;
	}

	if ( pWeapon )
	{
		if ( bWasActiveWeapon )
		{
			pWeapon->SendWeaponAnim( ACT_VM_IDLE );
		}
	}

	BaseClass::Weapon_Drop( pWeapon, pvecTarget, pVelocity );

	if ( bWasActiveWeapon )
	{
		if (!SwitchToNextBestWeapon( NULL ))
		{
			CBaseViewModel *vm = GetViewModel();
			if ( vm )
			{
				vm->AddEffects( EF_NODRAW );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : weaponSlot - 
//-----------------------------------------------------------------------------
void CBasePlayer::Weapon_DropSlot( int weaponSlot )
{
	CBaseCombatWeapon *pWeapon;

	// Check for that slot being occupied already
	for ( int i=0; i < MAX_WEAPONS; i++ )
	{
		pWeapon = GetWeapon( i );
		
		if ( pWeapon != NULL )
		{
			// If the slots match, it's already occupied
			if ( pWeapon->GetSlot() == weaponSlot )
			{
				Weapon_Drop( pWeapon, NULL, NULL );
			}
		}
	}
}

#ifdef MAPBASE
ConVar player_autoswitch_on_first_pickup("player_autoswitch_on_pickup", "1", FCVAR_NONE, "Determines how the player should autoswitch when picking up a new weapon. 0 = no autoswitch, 1 = always (default), 2 = use unused weighting system");
#endif

//-----------------------------------------------------------------------------
// Purpose: Override to add weapon to the hud
//-----------------------------------------------------------------------------
void CBasePlayer::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	BaseClass::Weapon_Equip( pWeapon );

#ifdef MAPBASE
	// BumpWeapon's code appeared to be deprecated; The same operation is already handled here, but with much more code involved.
	// There's also an unused weighting system which was overridden by that deprecated code. The unused weighting code can be enabled
	// via player_autoswitch_on_first_pickup.
	bool bShouldSwitch = false;
	switch (player_autoswitch_on_first_pickup.GetInt())
	{
		// Unused Weighting
		case 2:
			bShouldSwitch = g_pGameRules->FShouldSwitchWeapon( this, pWeapon );
			break;

		// Always (old behavior)
		case 1:
			bShouldSwitch = true;
			break;
	}
#else
	bool bShouldSwitch = g_pGameRules->FShouldSwitchWeapon( this, pWeapon );
#endif

#ifdef HL2_DLL
	if ( bShouldSwitch == false && PhysCannonGetHeldEntity( GetActiveWeapon() ) == pWeapon && 
		 Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType()) )
	{
		bShouldSwitch = true;
	}
#endif//HL2_DLL

	// should we switch to this item?
	if ( bShouldSwitch )
	{
		Weapon_Switch( pWeapon );
	}
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Activity CBasePlayer::Weapon_TranslateActivity( Activity baseAct, bool *pRequired )
{
	Activity weaponTranslation = BaseClass::Weapon_TranslateActivity( baseAct, pRequired );
	
	if ( GetActiveWeapon() && GetActiveWeapon()->IsEffectActive(EF_NODRAW) && baseAct != ACT_ARM )
	{
		// Our weapon is holstered. Use the base activity.
		return baseAct;
	}
	if ( GetModelPtr() && (!GetModelPtr()->HaveSequenceForActivity(weaponTranslation) || baseAct == weaponTranslation) )
	{
		// This is used so players can fall back to backup activities in the same way NPCs in Mapbase can
		Activity backupActivity = Weapon_BackupActivity(baseAct, pRequired ? *pRequired : false);
		if ( baseAct != backupActivity && GetModelPtr()->HaveSequenceForActivity(backupActivity) )
			return backupActivity;

		return baseAct;
	}

	return weaponTranslation;
}
#endif


//=========================================================
// HasNamedPlayerItem Does the player already have this item?
//=========================================================
CBaseEntity *CBasePlayer::HasNamedPlayerItem( const char *pszItemName )
{
	for ( int i = 0 ; i < WeaponCount() ; i++ )
	{
		if ( !GetWeapon(i) )
			continue;

		if ( FStrEq( pszItemName, GetWeapon(i)->GetClassname() ) )
		{
			return GetWeapon(i);
		}
	}

	return NULL;
}

#if defined USES_ECON_ITEMS
//-----------------------------------------------------------------------------
// Purpose: Add this wearable to the players' equipment list.
//-----------------------------------------------------------------------------
void CBasePlayer::EquipWearable( CEconWearable *pItem )
{
	Assert( pItem );

	if ( pItem )
	{
		m_hMyWearables.AddToHead( pItem );
		pItem->Equip( this );
	}

#ifdef DEBUG
	// Double check list integrity.
	for ( int i = m_hMyWearables.Count()-1; i >= 0; --i )
	{
		Assert( m_hMyWearables[i] != NULL );
	}
	// Networked Vector has a max size of MAX_WEARABLES_SENT_FROM_SERVER, should never have more then 7 wearables
	// in public
	// Search for : RecvPropUtlVector( RECVINFO_UTLVECTOR( m_hMyWearables ), MAX_WEARABLES_SENT_FROM_SERVER,	RecvPropEHandle(NULL, 0, 0) ),
	Assert( m_hMyWearables.Count() <= MAX_WEARABLES_SENT_FROM_SERVER );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Remove this wearable from the player's equipment list.
//-----------------------------------------------------------------------------
void CBasePlayer::RemoveWearable( CEconWearable *pItem )
{
	Assert( pItem );

	for ( int i = m_hMyWearables.Count()-1; i >= 0; --i )
	{
		CEconWearable *pWearable = m_hMyWearables[i];
		if ( pWearable == pItem )
		{
			pItem->UnEquip( this );
			UTIL_Remove( pWearable );
			m_hMyWearables.Remove( i );
			break;
		}

		// Integrety is failing, remove NULLs
		if ( !pWearable )
		{
			m_hMyWearables.Remove( i );
			break;
		}
	}

#ifdef DEBUG
	// Double check list integrity.
	for ( int i = m_hMyWearables.Count()-1; i >= 0; --i )
	{
		Assert( m_hMyWearables[i] != NULL );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::PlayWearableAnimsForPlaybackEvent( wearableanimplayback_t iPlayback )
{
	// Tell all our wearables to play their animations
	FOR_EACH_VEC( m_hMyWearables, i )
	{
		if ( m_hMyWearables[i] )
		{
			m_hMyWearables[i]->PlayAnimForPlaybackEvent( iPlayback );
		}
	}
}
#endif // USES_ECON_ITEMS

#ifdef MAPBASE
bool CBasePlayer::ShouldUseVisibilityCache( CBaseEntity *pEntity )
{
	// In CBaseEntity::FVisible(), players are allowed to see through CONTENTS_BLOCKLOS, which is used for
	// nodraw, block LOS brushes, etc. This is so some code doesn't erronesouly assume the player can't see
	// an entity (when the player can, in fact, see it) and therefore do something the player is not supposed to see.
	// 
	// However, to reduce the number of traces FVisible() runs, CBaseCombatCharacter uses a "visibility cache" shared
	// by all entities derived from it. The player is normally a part of this visibility cache, so when it runs a trace
	// through a CONTENTS_BLOCKLOS surface, the visibility cache assumes entities can now see through it and therefore
	// NPCs to see through the brush which should normally block their LOS.
	// 
	// This solution stops the player from using the visibility cache altogether, toggled by a convar.
	return player_use_visibility_cache.GetBool();
}
#endif

//================================================================================
// TEAM HANDLING
//================================================================================
//-----------------------------------------------------------------------------
// Purpose: Put the player in the specified team
//-----------------------------------------------------------------------------

void CBasePlayer::ChangeTeam( int iTeamNum, bool bAutoTeam, bool bSilent)
{
	if ( !GetGlobalTeam( iTeamNum ) )
	{
		Warning( "CBasePlayer::ChangeTeam( %d ) - invalid team index.\n", iTeamNum );
		return;
	}

	// if this is our current team, just abort
	if ( iTeamNum == GetTeamNumber() )
	{
		return;
	}

	// Immediately tell all clients that he's changing team. This has to be done
	// first, so that all user messages that follow as a result of the team change
	// come after this one, allowing the client to be prepared for them.
	IGameEvent * event = gameeventmanager->CreateEvent( "player_team" );
	if ( event )
	{
		event->SetInt("userid", GetUserID() );
		event->SetInt("team", iTeamNum );
		event->SetInt("oldteam", GetTeamNumber() );
		event->SetInt("disconnect", IsDisconnecting());
		event->SetInt("autoteam", bAutoTeam );
		event->SetInt("silent", bSilent );
		event->SetString("name", GetPlayerName() );

		gameeventmanager->FireEvent( event );
	}

	// Remove him from his current team
	if ( GetTeam() )
	{
		GetTeam()->RemovePlayer( this );
	}

	// Are we being added to a team?
	if ( iTeamNum )
	{
		GetGlobalTeam( iTeamNum )->AddPlayer( this );
	}

	BaseClass::ChangeTeam( iTeamNum );
}



//-----------------------------------------------------------------------------
// Purpose: Locks a player to the spot; they can't move, shoot, or be hurt
//-----------------------------------------------------------------------------
void CBasePlayer::LockPlayerInPlace( void )
{
	if ( m_iPlayerLocked )
		return;

	AddFlag( FL_GODMODE | FL_FROZEN );
	SetMoveType( MOVETYPE_NONE );
	m_iPlayerLocked = true;

	// force a client data update, so that anything that has been done to
	// this player previously this frame won't get delayed in being sent
	UpdateClientData();
}

//-----------------------------------------------------------------------------
// Purpose: Unlocks a previously locked player
//-----------------------------------------------------------------------------
void CBasePlayer::UnlockPlayer( void )
{
	if ( !m_iPlayerLocked )
		return;

	RemoveFlag( FL_GODMODE | FL_FROZEN );
	SetMoveType( MOVETYPE_WALK );
	m_iPlayerLocked = false;
}

bool CBasePlayer::ClearUseEntity()
{
	if ( m_hUseEntity != NULL )
	{
		// Stop controlling the train/object
		// TODO: Send HUD Update
		m_hUseEntity->Use( this, this, USE_OFF, 0 );
		m_hUseEntity = NULL;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::HideViewModels( void )
{
	for ( int i = 0 ; i < MAX_VIEWMODELS; i++ )
	{
		CBaseViewModel *vm = GetViewModel( i );
		if ( !vm )
			continue;

		vm->SetWeaponModel( NULL, NULL );
	}
}

class CStripWeapons : public CPointEntity
{
	DECLARE_CLASS( CStripWeapons, CPointEntity );
public:
	void InputStripWeapons(inputdata_t &data);
	void InputStripWeaponsAndSuit(inputdata_t &data);

	void StripWeapons(inputdata_t &data, bool stripSuit);
	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( player_weaponstrip, CStripWeapons );

BEGIN_DATADESC( CStripWeapons )
	DEFINE_INPUTFUNC( FIELD_VOID, "Strip", InputStripWeapons ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StripWeaponsAndSuit", InputStripWeaponsAndSuit ),
END_DATADESC()
	

void CStripWeapons::InputStripWeapons(inputdata_t &data)
{
	StripWeapons(data, false);
}

void CStripWeapons::InputStripWeaponsAndSuit(inputdata_t &data)
{
	StripWeapons(data, true);
}

void CStripWeapons::StripWeapons(inputdata_t &data, bool stripSuit)
{
	CBasePlayer *pPlayer = NULL;

	if ( data.pActivator && data.pActivator->IsPlayer() )
	{
		pPlayer = (CBasePlayer *)data.pActivator;
	}
	else if ( !g_pGameRules->IsDeathmatch() )
	{
		pPlayer = UTIL_GetLocalPlayer();
	}

	if ( pPlayer )
	{
		pPlayer->RemoveAllItems( stripSuit );
	}
}


class CRevertSaved : public CPointEntity
{
	DECLARE_CLASS( CRevertSaved, CPointEntity );
public:
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	LoadThink( void );

	DECLARE_DATADESC();

	inline	float	Duration( void ) { return m_Duration; }
	inline	float	HoldTime( void ) { return m_HoldTime; }
	inline	float	LoadTime( void ) { return m_loadTime; }

	inline	void	SetDuration( float duration ) { m_Duration = duration; }
	inline	void	SetHoldTime( float hold ) { m_HoldTime = hold; }
	inline	void	SetLoadTime( float time ) { m_loadTime = time; }

	//Inputs
	void InputReload(inputdata_t &data);

#ifdef HL1_DLL
	void	MessageThink( void );
	inline	float	MessageTime( void ) { return m_messageTime; }
	inline	void	SetMessageTime( float time ) { m_messageTime = time; }
#endif

private:

	float	m_loadTime;
	float	m_Duration;
	float	m_HoldTime;

#ifdef HL1_DLL
	string_t m_iszMessage;
	float	m_messageTime;
#endif
};

LINK_ENTITY_TO_CLASS( player_loadsaved, CRevertSaved );

BEGIN_DATADESC( CRevertSaved )

#ifdef HL1_DLL
	DEFINE_KEYFIELD( m_iszMessage, FIELD_STRING, "message" ),
	DEFINE_KEYFIELD( m_messageTime, FIELD_FLOAT, "messagetime" ),	// These are not actual times, but durations, so save as floats

	DEFINE_FUNCTION( MessageThink ),
#endif

	DEFINE_KEYFIELD( m_loadTime, FIELD_FLOAT, "loadtime" ),
	DEFINE_KEYFIELD( m_Duration, FIELD_FLOAT, "duration" ),
	DEFINE_KEYFIELD( m_HoldTime, FIELD_FLOAT, "holdtime" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Reload", InputReload ),


	// Function Pointers
	DEFINE_FUNCTION( LoadThink ),

END_DATADESC()

CBaseEntity *CreatePlayerLoadSave( Vector vOrigin, float flDuration, float flHoldTime, float flLoadTime )
{
	CRevertSaved *pRevertSaved = (CRevertSaved *) CreateEntityByName( "player_loadsaved" );

	if ( pRevertSaved == NULL )
		return NULL;

	UTIL_SetOrigin( pRevertSaved, vOrigin );

	pRevertSaved->Spawn();
	pRevertSaved->SetDuration( flDuration );
	pRevertSaved->SetHoldTime( flHoldTime );
	pRevertSaved->SetLoadTime( flLoadTime );

	return pRevertSaved;
}



void CRevertSaved::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	UTIL_ScreenFadeAll( m_clrRender, Duration(), HoldTime(), FFADE_OUT );
	SetNextThink( gpGlobals->curtime + LoadTime() );
	SetThink( &CRevertSaved::LoadThink );

	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	if ( pPlayer )
	{
		//Adrian: Setting this flag so we can't move or save a game.
		pPlayer->pl.deadflag = true;
		pPlayer->AddFlag( (FL_NOTARGET|FL_FROZEN) );

		// clear any pending autosavedangerous
		g_ServerGameDLL.m_fAutoSaveDangerousTime = 0.0f;
		g_ServerGameDLL.m_fAutoSaveDangerousMinHealthToCommit = 0.0f;
	}
}

void CRevertSaved::InputReload( inputdata_t &inputdata )
{
	UTIL_ScreenFadeAll( m_clrRender, Duration(), HoldTime(), FFADE_OUT );

#ifdef HL1_DLL
	SetNextThink( gpGlobals->curtime + MessageTime() );
	SetThink( &CRevertSaved::MessageThink );
#else
	SetNextThink( gpGlobals->curtime + LoadTime() );
	SetThink( &CRevertSaved::LoadThink );
#endif

	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	if ( pPlayer )
	{
		//Adrian: Setting this flag so we can't move or save a game.
		pPlayer->pl.deadflag = true;
		pPlayer->AddFlag( (FL_NOTARGET|FL_FROZEN) );

		// clear any pending autosavedangerous
		g_ServerGameDLL.m_fAutoSaveDangerousTime = 0.0f;
		g_ServerGameDLL.m_fAutoSaveDangerousMinHealthToCommit = 0.0f;
	}
}

#ifdef HL1_DLL
void CRevertSaved::MessageThink( void )
{
	UTIL_ShowMessageAll( STRING( m_iszMessage ) );
	float nextThink = LoadTime() - MessageTime();
	if ( nextThink > 0 ) 
	{
		SetNextThink( gpGlobals->curtime + nextThink );
		SetThink( &CRevertSaved::LoadThink );
	}
	else
		LoadThink();
}
#endif


void CRevertSaved::LoadThink( void )
{
	if ( !gpGlobals->deathmatch )
	{
		engine->ServerCommand("reload\n");
	}
}

#define SF_SPEED_MOD_SUPPRESS_WEAPONS	(1<<0)	// Take away weapons
#define SF_SPEED_MOD_SUPPRESS_HUD		(1<<1)	// Take away the HUD
#define SF_SPEED_MOD_SUPPRESS_JUMP		(1<<2)
#define SF_SPEED_MOD_SUPPRESS_DUCK		(1<<3)
#define SF_SPEED_MOD_SUPPRESS_USE		(1<<4)
#define SF_SPEED_MOD_SUPPRESS_SPEED		(1<<5)
#define SF_SPEED_MOD_SUPPRESS_ATTACK	(1<<6)
#define SF_SPEED_MOD_SUPPRESS_ZOOM		(1<<7)
#ifdef MAPBASE
// Needs to be inverse because suppressing the flashlight is already default behavior
// and we don't want to break compatibility for existing speedmods
#define SF_SPEED_MOD_DONT_SUPPRESS_FLASHLIGHT	(1<<8)
#endif

class CMovementSpeedMod : public CPointEntity
{
	DECLARE_CLASS( CMovementSpeedMod, CPointEntity );
public:
	void InputSpeedMod(inputdata_t &data);

#ifdef MAPBASE
	void InputEnable(inputdata_t &data);
	void InputDisable(inputdata_t &data);

	void InputSetAdditionalButtons(inputdata_t &data);
#endif

private:
	int GetDisabledButtonMask( void );

#ifdef MAPBASE
	int m_iAdditionalButtons;
#endif

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( player_speedmod, CMovementSpeedMod );

BEGIN_DATADESC( CMovementSpeedMod )
	DEFINE_INPUTFUNC( FIELD_FLOAT, "ModifySpeed", InputSpeedMod ),
#ifdef MAPBASE
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_KEYFIELD( m_iAdditionalButtons, FIELD_INTEGER, "AdditionalButtons" ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetAdditionalButtons", InputSetAdditionalButtons ),
#endif
END_DATADESC()
	
int CMovementSpeedMod::GetDisabledButtonMask( void )
{
	int nMask = 0;

	if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_JUMP ) )
	{
		nMask |= IN_JUMP;
	}
	
	if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_DUCK ) )
	{
		nMask |= IN_DUCK;
	}

	if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_USE ) )
	{
		nMask |= IN_USE;
	}
	
	if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_SPEED ) )
	{
		nMask |= IN_SPEED;
	}
	
	if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_ATTACK ) )
	{
		nMask |= (IN_ATTACK|IN_ATTACK2);
	}

	if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_ZOOM ) )
	{
		nMask |= IN_ZOOM;
	}

#ifdef MAPBASE
	if ( m_iAdditionalButtons != 0 )
	{
		nMask |= m_iAdditionalButtons;
	}
#endif

	return nMask;
}

void CMovementSpeedMod::InputSpeedMod(inputdata_t &data)
{
	CBasePlayer *pPlayer = NULL;

	if ( data.pActivator && data.pActivator->IsPlayer() )
	{
		pPlayer = (CBasePlayer *)data.pActivator;
	}
	else if ( !g_pGameRules->IsDeathmatch() )
	{
		pPlayer = UTIL_GetLocalPlayer();
	}

	if ( pPlayer )
	{
		if ( data.value.Float() != 1.0f )
		{
			// Holster weapon immediately, to allow it to cleanup
			if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_WEAPONS ) )
			{
				if ( pPlayer->GetActiveWeapon() )
				{
					pPlayer->Weapon_SetLast( pPlayer->GetActiveWeapon() );
					pPlayer->GetActiveWeapon()->Holster();
					pPlayer->ClearActiveWeapon();
				}
				
				pPlayer->HideViewModels();
			}

#ifdef MAPBASE
			if ( !HasSpawnFlags( SF_SPEED_MOD_DONT_SUPPRESS_FLASHLIGHT ) )
			{
#endif
			// Turn off the flashlight
			if ( pPlayer->FlashlightIsOn() )
			{
				pPlayer->FlashlightTurnOff();
			}
			
			// Disable the flashlight's further use
			pPlayer->SetFlashlightEnabled( false );
#ifdef MAPBASE
			}
#endif
			pPlayer->DisableButtons( GetDisabledButtonMask() );

			// Hide the HUD
			if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_HUD ) )
			{
				pPlayer->m_Local.m_iHideHUD |= HIDEHUD_ALL;
			}
		}
		else
		{
			// Bring the weapon back
			if  ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_WEAPONS ) && pPlayer->GetActiveWeapon() == NULL )
			{
				pPlayer->SetActiveWeapon( pPlayer->Weapon_GetLast() );
				if ( pPlayer->GetActiveWeapon() )
				{
					pPlayer->GetActiveWeapon()->Deploy();
				}
			}

#ifdef MAPBASE
			if ( !HasSpawnFlags( SF_SPEED_MOD_DONT_SUPPRESS_FLASHLIGHT ) )
			{
#endif
			// Allow the flashlight again
			pPlayer->SetFlashlightEnabled( true );
#ifdef MAPBASE
			}
#endif
			pPlayer->EnableButtons( GetDisabledButtonMask() );

			// Restore the HUD
			if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_HUD ) )
			{
				pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_ALL;
			}
		}

		pPlayer->SetLaggedMovementValue( data.value.Float() );
	}
}

#ifdef MAPBASE
void CMovementSpeedMod::InputEnable(inputdata_t &data)
{
	CBasePlayer *pPlayer = NULL;

	if ( data.pActivator && data.pActivator->IsPlayer() )
	{
		pPlayer = (CBasePlayer *)data.pActivator;
	}
	else if ( !g_pGameRules->IsDeathmatch() )
	{
		pPlayer = UTIL_GetLocalPlayer();
	}

	if ( pPlayer )
	{
		// Holster weapon immediately, to allow it to cleanup
		if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_WEAPONS ) )
		{
			if ( pPlayer->GetActiveWeapon() )
			{
				pPlayer->Weapon_SetLast( pPlayer->GetActiveWeapon() );
				pPlayer->GetActiveWeapon()->Holster();
				pPlayer->ClearActiveWeapon();
			}
			
			pPlayer->HideViewModels();
		}

		// Turn off the flashlight
		if ( pPlayer->FlashlightIsOn() )
		{
			pPlayer->FlashlightTurnOff();
		}
		
		// Disable the flashlight's further use
		pPlayer->SetFlashlightEnabled( false );
		pPlayer->DisableButtons( GetDisabledButtonMask() );

		// Hide the HUD
		if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_HUD ) )
		{
			pPlayer->m_Local.m_iHideHUD |= HIDEHUD_ALL;
		}
	}
}

void CMovementSpeedMod::InputDisable(inputdata_t &data)
{
	CBasePlayer *pPlayer = NULL;

	if ( data.pActivator && data.pActivator->IsPlayer() )
	{
		pPlayer = (CBasePlayer *)data.pActivator;
	}
	else if ( !g_pGameRules->IsDeathmatch() )
	{
		pPlayer = UTIL_GetLocalPlayer();
	}

	if ( pPlayer )
	{
		// Bring the weapon back
		if  ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_WEAPONS ) && pPlayer->GetActiveWeapon() == NULL )
		{
			pPlayer->SetActiveWeapon( pPlayer->Weapon_GetLast() );
			if ( pPlayer->GetActiveWeapon() )
			{
				pPlayer->GetActiveWeapon()->Deploy();
			}
		}

		// Allow the flashlight again
		pPlayer->SetFlashlightEnabled( true );
		pPlayer->EnableButtons( GetDisabledButtonMask() );

		// Restore the HUD
		if ( HasSpawnFlags( SF_SPEED_MOD_SUPPRESS_HUD ) )
		{
			pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_ALL;
		}
	}
}

void CMovementSpeedMod::InputSetAdditionalButtons(inputdata_t &data)
{
	CBasePlayer *pPlayer = NULL;

	if ( data.pActivator && data.pActivator->IsPlayer() )
	{
		pPlayer = (CBasePlayer *)data.pActivator;
	}
	else if ( !g_pGameRules->IsDeathmatch() )
	{
		pPlayer = UTIL_GetLocalPlayer();
	}

	bool bAlreadyDisabled = false;
	if ( pPlayer )
	{
		bAlreadyDisabled = (pPlayer->m_afButtonDisabled & GetDisabledButtonMask()) != 0;
	}

	m_iAdditionalButtons = data.value.Int();

	// If we were already disabling buttons, re-disable them
	if ( bAlreadyDisabled )
	{
		// We should probably do something better than this.
		pPlayer->m_afButtonForced = GetDisabledButtonMask();
	}
}
#endif

#ifdef MAPBASE
class CLogicPlayerInfo : public CPointEntity
{
	DECLARE_CLASS( CLogicPlayerInfo, CPointEntity );
public:
	void InputGetPlayerInfo( inputdata_t &inputdata );
	void InputGetPlayerByID( inputdata_t &inputdata );
	void InputGetPlayerByName( inputdata_t &inputdata );

	void GetPlayerInfo( CBasePlayer *pPlayer );

	COutputInt m_OutUserID;
	COutputString m_OutPlayerName;
	COutputEHANDLE m_OutPlayerEntity;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( logic_playerinfo, CLogicPlayerInfo );

BEGIN_DATADESC( CLogicPlayerInfo )
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "GetPlayerInfo", InputGetPlayerInfo ),
	DEFINE_INPUTFUNC( FIELD_STRING, "GetPlayerByID", InputGetPlayerByID ),
	DEFINE_INPUTFUNC( FIELD_STRING, "GetPlayerByName", InputGetPlayerByName ),

	DEFINE_OUTPUT( m_OutUserID, "OutUserID" ),
	DEFINE_OUTPUT( m_OutPlayerName, "OutPlayerName" ),
	DEFINE_OUTPUT( m_OutPlayerEntity, "OutPlayerEntity" ),
END_DATADESC()
	

void CLogicPlayerInfo::InputGetPlayerInfo( inputdata_t &inputdata )
{
	CBasePlayer *pPlayer = ToBasePlayer(inputdata.value.Entity());

	// If there was no entity to begin with, try the local player
	if (!pPlayer && !inputdata.value.Entity())
		pPlayer = UTIL_GetLocalPlayer();

	if (pPlayer)
		GetPlayerInfo( pPlayer );
}

void CLogicPlayerInfo::InputGetPlayerByID( inputdata_t &inputdata )
{
	for (int i = 1; i < gpGlobals->maxClients; i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if (pPlayer)
		{
			if (Matcher_NamesMatch( inputdata.value.String(), UTIL_VarArgs("%i", pPlayer->GetUserID()) ))
			{
				GetPlayerInfo( pPlayer );
				return;
			}
		}
	}
}

void CLogicPlayerInfo::InputGetPlayerByName( inputdata_t &inputdata )
{
	for (int i = 1; i < gpGlobals->maxClients; i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if (pPlayer)
		{
			if (Matcher_NamesMatch( inputdata.value.String(), pPlayer->GetPlayerName() ))
			{
				GetPlayerInfo( pPlayer );
				return;
			}
		}
	}
}

void CLogicPlayerInfo::GetPlayerInfo( CBasePlayer *pPlayer )
{
	m_OutUserID.Set( pPlayer->GetUserID(), pPlayer, this );

	m_OutPlayerName.Set( AllocPooledString(pPlayer->GetPlayerName()), pPlayer, this );

	m_OutPlayerEntity.Set( pPlayer, pPlayer, this );
}
#endif


void SendProxy_CropFlagsToPlayerFlagBitsLength( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID)
{
	int mask = (1<<PLAYER_FLAG_BITS) - 1;
	int data = *(int *)pVarData;

	pOut->m_Int = ( data & mask );
}

#ifdef MAPBASE
// Needs to shift bits since network table only sends the player ones
void SendProxy_ShiftPlayerSpawnflags( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID )
{
	int *pInt = (int *)pVarData;

	pOut->m_Int = (*pInt) >> 16;
}
#endif

// -------------------------------------------------------------------------------- //
// SendTable for CPlayerState.
// -------------------------------------------------------------------------------- //

	BEGIN_SEND_TABLE_NOBASE(CPlayerState, DT_PlayerState)
		SendPropInt		(SENDINFO(deadflag),	1, SPROP_UNSIGNED ),
	END_SEND_TABLE()

// -------------------------------------------------------------------------------- //
// This data only gets sent to clients that ARE this player entity.
// -------------------------------------------------------------------------------- //

	BEGIN_SEND_TABLE_NOBASE( CBasePlayer, DT_LocalPlayerExclusive )

		SendPropDataTable	( SENDINFO_DT(m_Local), &REFERENCE_SEND_TABLE(DT_Local) ),
		
// If HL2_DLL is defined, then baseflex.cpp already sends these.
#ifndef HL2_DLL
		SendPropFloat		( SENDINFO_VECTORELEM(m_vecViewOffset, 0), 8, SPROP_ROUNDDOWN, -32.0, 32.0f),
		SendPropFloat		( SENDINFO_VECTORELEM(m_vecViewOffset, 1), 8, SPROP_ROUNDDOWN, -32.0, 32.0f),
		SendPropFloat		( SENDINFO_VECTORELEM(m_vecViewOffset, 2), 20, SPROP_CHANGES_OFTEN,	0.0f, 256.0f),
#endif

		SendPropFloat		( SENDINFO(m_flFriction),		8,	SPROP_ROUNDDOWN,	0.0f,	4.0f),

		SendPropArray3		( SENDINFO_ARRAY3(m_iAmmo), SendPropInt( SENDINFO_ARRAY(m_iAmmo), -1, SPROP_VARINT | SPROP_UNSIGNED ) ),
			
		SendPropInt			( SENDINFO( m_fOnTarget ), 2, SPROP_UNSIGNED ),

		SendPropInt			( SENDINFO( m_nTickBase ), -1, SPROP_CHANGES_OFTEN ),
		SendPropInt			( SENDINFO( m_nNextThinkTick ) ),

		SendPropEHandle		( SENDINFO( m_hLastWeapon ) ),
		SendPropEHandle		( SENDINFO( m_hGroundEntity ), SPROP_CHANGES_OFTEN ),

		SendPropFloat		( SENDINFO_VECTORELEM(m_vecVelocity, 0), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),
		SendPropFloat		( SENDINFO_VECTORELEM(m_vecVelocity, 1), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),
		SendPropFloat		( SENDINFO_VECTORELEM(m_vecVelocity, 2), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),

#if PREDICTION_ERROR_CHECK_LEVEL > 1 
		SendPropVector		( SENDINFO( m_vecBaseVelocity ), -1, SPROP_COORD ),
#else
		SendPropVector		( SENDINFO( m_vecBaseVelocity ), 20, 0, -1000, 1000 ),
#endif

		SendPropEHandle		( SENDINFO( m_hConstraintEntity)),
		SendPropVector		( SENDINFO( m_vecConstraintCenter), 0, SPROP_NOSCALE ),
		SendPropFloat		( SENDINFO( m_flConstraintRadius ), 0, SPROP_NOSCALE ),
		SendPropFloat		( SENDINFO( m_flConstraintWidth ), 0, SPROP_NOSCALE ),
		SendPropFloat		( SENDINFO( m_flConstraintSpeedFactor ), 0, SPROP_NOSCALE ),

		SendPropFloat		( SENDINFO( m_flDeathTime ), 0, SPROP_NOSCALE ),

		SendPropInt			( SENDINFO( m_nWaterLevel ), 2, SPROP_UNSIGNED ),
		SendPropFloat		( SENDINFO( m_flLaggedMovementValue ), 0, SPROP_NOSCALE ),

#ifdef MAPBASE
		// Transmitted from the server for internal player spawnflags.
		// See baseplayer_shared.h for more details.
		SendPropInt			( SENDINFO( m_spawnflags ), 3, SPROP_UNSIGNED, SendProxy_ShiftPlayerSpawnflags ),

		SendPropBool		( SENDINFO( m_bDrawPlayerModelExternally ) ),
		SendPropBool		( SENDINFO( m_bInTriggerFall ) ),
#endif

	END_SEND_TABLE()


// -------------------------------------------------------------------------------- //
// DT_BasePlayer sendtable.
// -------------------------------------------------------------------------------- //
	
#if defined USES_ECON_ITEMS
	EXTERN_SEND_TABLE(DT_AttributeList);
#endif

	IMPLEMENT_SERVERCLASS_ST( CBasePlayer, DT_BasePlayer )

#if defined USES_ECON_ITEMS
		SendPropDataTable(SENDINFO_DT(m_AttributeList), &REFERENCE_SEND_TABLE(DT_AttributeList)),
#endif

		SendPropDataTable(SENDINFO_DT(pl), &REFERENCE_SEND_TABLE(DT_PlayerState), SendProxy_DataTableToDataTable),

		SendPropEHandle(SENDINFO(m_hVehicle)),
		SendPropEHandle(SENDINFO(m_hUseEntity)),
		SendPropInt		(SENDINFO(m_iHealth), -1, SPROP_VARINT | SPROP_CHANGES_OFTEN ),
		SendPropInt		(SENDINFO(m_lifeState), 3, SPROP_UNSIGNED ),
		SendPropInt		(SENDINFO(m_iBonusProgress), 15 ),
		SendPropInt		(SENDINFO(m_iBonusChallenge), 4 ),
		SendPropFloat	(SENDINFO(m_flMaxspeed), 12, SPROP_ROUNDDOWN, 0.0f, 2048.0f ),  // CL
		SendPropInt		(SENDINFO(m_fFlags), PLAYER_FLAG_BITS, SPROP_UNSIGNED|SPROP_CHANGES_OFTEN, SendProxy_CropFlagsToPlayerFlagBitsLength ),
		SendPropInt		(SENDINFO(m_iObserverMode), 3, SPROP_UNSIGNED ),
		SendPropEHandle	(SENDINFO(m_hObserverTarget) ),
		SendPropInt		(SENDINFO(m_iFOV), 8, SPROP_UNSIGNED ),
		SendPropInt		(SENDINFO(m_iFOVStart), 8, SPROP_UNSIGNED ),
		SendPropFloat	(SENDINFO(m_flFOVTime) ),
		SendPropInt		(SENDINFO(m_iDefaultFOV), 8, SPROP_UNSIGNED ),
		SendPropEHandle	(SENDINFO(m_hZoomOwner) ),
		SendPropArray	( SendPropEHandle( SENDINFO_ARRAY( m_hViewModel ) ), m_hViewModel ),
		SendPropString	(SENDINFO(m_szLastPlaceName) ),

#ifdef MAPBASE // From Alien Swarm SDK
		// Postprocess data
		SendPropEHandle		( SENDINFO(m_hPostProcessCtrl) ),
		SendPropEHandle		( SENDINFO(m_hColorCorrectionCtrl) ),
#endif

#if defined USES_ECON_ITEMS
		SendPropUtlVector( SENDINFO_UTLVECTOR( m_hMyWearables ), MAX_WEARABLES_SENT_FROM_SERVER, SendPropEHandle( NULL, 0 ) ),
#endif // USES_ECON_ITEMS

		// Data that only gets sent to the local player.
		SendPropDataTable( "localdata", 0, &REFERENCE_SEND_TABLE(DT_LocalPlayerExclusive), SendProxy_SendLocalDataTable ),

	END_SEND_TABLE()

//=============================================================================
//
// Player Physics Shadow Code
//

void CBasePlayer::SetupVPhysicsShadow( const Vector &vecAbsOrigin, const Vector &vecAbsVelocity, CPhysCollide *pStandModel, const char *pStandHullName, CPhysCollide *pCrouchModel, const char *pCrouchHullName )
{
	solid_t solid;
	Q_strncpy( solid.surfaceprop, "player", sizeof(solid.surfaceprop) );
	solid.params = g_PhysDefaultObjectParams;
	solid.params.mass = 85.0f;
	solid.params.inertia = 1e24f;
	solid.params.enableCollisions = false;
	//disable drag
	solid.params.dragCoefficient = 0;
	// create standing hull
	m_pShadowStand = PhysModelCreateCustom( this, pStandModel, GetLocalOrigin(), GetLocalAngles(), pStandHullName, false, &solid );
	m_pShadowStand->SetCallbackFlags( CALLBACK_GLOBAL_COLLISION | CALLBACK_SHADOW_COLLISION );

	// create crouchig hull
	m_pShadowCrouch = PhysModelCreateCustom( this, pCrouchModel, GetLocalOrigin(), GetLocalAngles(), pCrouchHullName, false, &solid );
	m_pShadowCrouch->SetCallbackFlags( CALLBACK_GLOBAL_COLLISION | CALLBACK_SHADOW_COLLISION );

	// default to stand
	VPhysicsSetObject( m_pShadowStand );

	// tell physics lists I'm a shadow controller object
	PhysAddShadow( this );	
	m_pPhysicsController = physenv->CreatePlayerController( m_pShadowStand );
	m_pPhysicsController->SetPushMassLimit( 350.0f );
	m_pPhysicsController->SetPushSpeedLimit( 50.0f );
	
	// Give the controller a valid position so it doesn't do anything rash.
	UpdatePhysicsShadowToPosition( vecAbsOrigin );

	// init state
	if ( GetFlags() & FL_DUCKING )
	{
		SetVCollisionState( vecAbsOrigin, vecAbsVelocity, VPHYS_CROUCH );
	}
	else
	{
		SetVCollisionState( vecAbsOrigin, vecAbsVelocity, VPHYS_WALK );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Empty, just want to keep the baseentity version from being called
//          current so we don't kick up dust, etc.
//-----------------------------------------------------------------------------
void CBasePlayer::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBasePlayer::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	float savedImpact = m_impactEnergyScale;
	
	// HACKHACK: Reduce player's stress by 1/8th
	m_impactEnergyScale *= 0.125f;
	ApplyStressDamage( pPhysics, true );
	m_impactEnergyScale = savedImpact;
}

//-----------------------------------------------------------------------------
// Purpose: Allow bots etc to use slightly different solid masks
//-----------------------------------------------------------------------------
unsigned int CBasePlayer::PlayerSolidMask( bool brushOnly ) const
{
	if ( brushOnly )
	{
		return MASK_PLAYERSOLID_BRUSHONLY;
	}

	return MASK_PLAYERSOLID;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBasePlayer::VPhysicsShadowUpdate( IPhysicsObject *pPhysics )
{
	if ( sv_turbophysics.GetBool() )
		return;

	Vector newPosition;

	bool physicsUpdated = m_pPhysicsController->GetShadowPosition( &newPosition, NULL ) > 0 ? true : false;

	// UNDONE: If the player is penetrating, but the player's game collisions are not stuck, teleport the physics shadow to the game position
	if ( pPhysics->GetGameFlags() & FVPHYSICS_PENETRATING )
	{
		CUtlVector<CBaseEntity *> list;
		PhysGetListOfPenetratingEntities( this, list );
		for ( int i = list.Count()-1; i >= 0; --i )
		{
			// filter out anything that isn't simulated by vphysics
			// UNDONE: Filter out motion disabled objects?
			if ( list[i]->GetMoveType() == MOVETYPE_VPHYSICS )
			{
				// I'm currently stuck inside a moving object, so allow vphysics to 
				// apply velocity to the player in order to separate these objects
				m_touchedPhysObject = true;
			}

			// if it's an NPC, tell them that the player is intersecting them
			CAI_BaseNPC *pNPC = list[i]->MyNPCPointer();
			if ( pNPC )
			{
				pNPC->PlayerPenetratingVPhysics();
			}
		}
	}

	bool bCheckStuck = false;
	if ( m_afPhysicsFlags & PFLAG_GAMEPHYSICS_ROTPUSH )
	{
		bCheckStuck = true;
		m_afPhysicsFlags &= ~PFLAG_GAMEPHYSICS_ROTPUSH;
	}
	if ( m_pPhysicsController->IsInContact() || (m_afPhysicsFlags & PFLAG_VPHYSICS_MOTIONCONTROLLER) )
	{
		m_touchedPhysObject = true;
	}

	if ( IsFollowingPhysics() )
	{
		m_touchedPhysObject = true;
	}

	if ( GetMoveType() == MOVETYPE_NOCLIP || pl.deadflag )
	{
		m_oldOrigin = GetAbsOrigin();
		return;
	}

	if ( phys_timescale.GetFloat() == 0.0f )
	{
		physicsUpdated = false;
	}

	if ( !physicsUpdated )
		return;

	IPhysicsObject *pPhysGround = GetGroundVPhysics();

	Vector newVelocity;
	pPhysics->GetPosition( &newPosition, 0 );
	m_pPhysicsController->GetShadowVelocity( &newVelocity );
	// assume vphysics gave us back a position without penetration
	Vector lastValidPosition = newPosition;

	if ( physicsshadowupdate_render.GetBool() )
	{
		NDebugOverlay::Box( GetAbsOrigin(), WorldAlignMins(), WorldAlignMaxs(), 255, 0, 0, 24, 15.0f );
		NDebugOverlay::Box( newPosition, WorldAlignMins(), WorldAlignMaxs(), 0,0,255, 24, 15.0f);
		//	NDebugOverlay::Box( newPosition, WorldAlignMins(), WorldAlignMaxs(), 0,0,255, 24, .01f);
	}

	Vector tmp = GetAbsOrigin() - newPosition;
	if ( !m_touchedPhysObject && !(GetFlags() & FL_ONGROUND) )
	{
		tmp.z *= 0.5f;	// don't care about z delta as much
	}

	float dist = tmp.LengthSqr();
	float deltaV = (newVelocity - GetAbsVelocity()).LengthSqr();

	float maxDistErrorSqr = VPHYS_MAX_DISTSQR;
	float maxVelErrorSqr = VPHYS_MAX_VELSQR;
	if ( IsRideablePhysics(pPhysGround) )
	{
		maxDistErrorSqr *= 0.25;
		maxVelErrorSqr *= 0.25;
	}

	// player's physics was frozen, try moving to the game's simulated position if possible
	if ( m_pPhysicsController->WasFrozen() )
	{
		m_bPhysicsWasFrozen = true;
		// check my position (physics object could have simulated into my position
		// physics is not very far away, check my position
		trace_t trace;
		UTIL_TraceEntity( this, GetAbsOrigin(), GetAbsOrigin(), MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
		if ( !trace.startsolid )
			return;

		// The physics shadow position is probably not in solid, try to move from there to the desired position
		UTIL_TraceEntity( this, newPosition, GetAbsOrigin(), MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
		if ( !trace.startsolid )
		{
			// found a valid position between the two?  take it.
			SetAbsOrigin( trace.endpos );
			UpdateVPhysicsPosition(trace.endpos, vec3_origin, 0);
			return;
		}

	}
	if ( dist >= maxDistErrorSqr || deltaV >= maxVelErrorSqr || (pPhysGround && !m_touchedPhysObject) )
	{
		if ( m_touchedPhysObject || pPhysGround )
		{
			// BUGBUG: Rewrite this code using fixed timestep
			if ( deltaV >= maxVelErrorSqr && !m_bPhysicsWasFrozen )
			{
				Vector dir = GetAbsVelocity();
				float len = VectorNormalize(dir);
				float dot = DotProduct( newVelocity, dir );
				if ( dot > len )
				{
					dot = len;
				}
				else if ( dot < -len )
				{
					dot = -len;
				}
				
				VectorMA( newVelocity, -dot, dir, newVelocity );
				
				if ( m_afPhysicsFlags & PFLAG_VPHYSICS_MOTIONCONTROLLER )
				{
					float val = Lerp( 0.1f, len, dot );
					VectorMA( newVelocity, val - len, dir, newVelocity );
				}

				if ( !IsRideablePhysics(pPhysGround) )
				{
					if ( !(m_afPhysicsFlags & PFLAG_VPHYSICS_MOTIONCONTROLLER ) && IsSimulatingOnAlternateTicks() )
					{
						newVelocity *= 0.5f;
					}
					ApplyAbsVelocityImpulse( newVelocity );
				}
			}
			
			trace_t trace;
			UTIL_TraceEntity( this, newPosition, newPosition, MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
			if ( !trace.allsolid && !trace.startsolid )
			{
				SetAbsOrigin( newPosition );
			}
		}
		else
		{
			bCheckStuck = true;
		}
	}
	else
	{
		if ( m_touchedPhysObject )
		{
			// check my position (physics object could have simulated into my position
			// physics is not very far away, check my position
			trace_t trace;
			UTIL_TraceEntity( this, GetAbsOrigin(), GetAbsOrigin(),
				MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
			
			// is current position ok?
			if ( trace.allsolid || trace.startsolid )
			{
				// no use the final stuck check to move back to old if this stuck fix didn't work
				bCheckStuck = true;
				lastValidPosition = m_oldOrigin;
				SetAbsOrigin( newPosition );
			}
		}
	}

	if ( bCheckStuck )
	{
		trace_t trace;
		UTIL_TraceEntity( this, GetAbsOrigin(), GetAbsOrigin(), MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );

		// current position is not ok, fixup
		if ( trace.allsolid || trace.startsolid )
		{
			// STUCK!?!?!
			//Warning( "Checkstuck failed.  Stuck on %s!!\n", trace.m_pEnt->GetClassname() );
			SetAbsOrigin( lastValidPosition );
		}
	}
	m_oldOrigin = GetAbsOrigin();
	m_bPhysicsWasFrozen = false;
}

// recreate physics on save/load, don't try to save the state!
bool CBasePlayer::ShouldSavePhysics()
{
	return false;
}

void CBasePlayer::RefreshCollisionBounds( void )
{
	BaseClass::RefreshCollisionBounds();

	InitVCollision( GetAbsOrigin(), GetAbsVelocity() );
	SetViewOffset( VEC_VIEW_SCALED( this ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBasePlayer::InitVCollision( const Vector &vecAbsOrigin, const Vector &vecAbsVelocity )
{
	// Cleanup any old vphysics stuff.
	VPhysicsDestroyObject();

	// in turbo physics players dont have a physics shadow
	if ( sv_turbophysics.GetBool() )
		return;
	
	CPhysCollide *pModel = PhysCreateBbox( VEC_HULL_MIN_SCALED( this ), VEC_HULL_MAX_SCALED( this ) );
	CPhysCollide *pCrouchModel = PhysCreateBbox( VEC_DUCK_HULL_MIN_SCALED( this ), VEC_DUCK_HULL_MAX_SCALED( this ) );

	SetupVPhysicsShadow( vecAbsOrigin, vecAbsVelocity, pModel, "player_stand", pCrouchModel, "player_crouch" );
}


void CBasePlayer::VPhysicsDestroyObject()
{
	// Since CBasePlayer aliases its pointer to the physics object, tell CBaseEntity to 
	// clear out its physics object pointer so we don't wind up deleting one of
	// the aliased objects twice.
	VPhysicsSetObject( NULL );

	PhysRemoveShadow( this );
	
	if ( m_pPhysicsController )
	{
		physenv->DestroyPlayerController( m_pPhysicsController );
		m_pPhysicsController = NULL;
	}

	if ( m_pShadowStand )
	{
		m_pShadowStand->EnableCollisions( false );
		PhysDestroyObject( m_pShadowStand );
		m_pShadowStand = NULL;
	}
	if ( m_pShadowCrouch )
	{
		m_pShadowCrouch->EnableCollisions( false );
		PhysDestroyObject( m_pShadowCrouch );
		m_pShadowCrouch = NULL;
	}

	BaseClass::VPhysicsDestroyObject();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBasePlayer::SetVCollisionState( const Vector &vecAbsOrigin, const Vector &vecAbsVelocity, int collisionState )
{
	m_vphysicsCollisionState = collisionState;
	switch( collisionState )
	{
	case VPHYS_WALK:
 		m_pShadowStand->SetPosition( vecAbsOrigin, vec3_angle, true );
		m_pShadowStand->SetVelocity( &vecAbsVelocity, NULL );
		m_pShadowCrouch->EnableCollisions( false );
		m_pPhysicsController->SetObject( m_pShadowStand );
		VPhysicsSwapObject( m_pShadowStand );
		m_pShadowStand->EnableCollisions( true );
		break;

	case VPHYS_CROUCH:
		m_pShadowCrouch->SetPosition( vecAbsOrigin, vec3_angle, true );
		m_pShadowCrouch->SetVelocity( &vecAbsVelocity, NULL );
		m_pShadowStand->EnableCollisions( false );
		m_pPhysicsController->SetObject( m_pShadowCrouch );
		VPhysicsSwapObject( m_pShadowCrouch );
		m_pShadowCrouch->EnableCollisions( true );
		break;
	
	case VPHYS_NOCLIP:
		m_pShadowCrouch->EnableCollisions( false );
		m_pShadowStand->EnableCollisions( false );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBasePlayer::GetFOV( void )
{
	int nDefaultFOV;

	// The vehicle's FOV wins if we're asking for a default value
	if ( GetVehicle() )
	{
		CacheVehicleView();
		nDefaultFOV = ( m_flVehicleViewFOV == 0 ) ? GetDefaultFOV() : (int) m_flVehicleViewFOV;
	}
	else
	{
		nDefaultFOV = GetDefaultFOV();
	}
	
	int fFOV = ( m_iFOV == 0 ) ? nDefaultFOV : m_iFOV;

	// If it's immediate, just do it
	if ( m_Local.m_flFOVRate == 0.0f )
		return fFOV;

	float deltaTime = (float)( gpGlobals->curtime - m_flFOVTime ) / m_Local.m_flFOVRate;

	if ( deltaTime >= 1.0f )
	{
		//If we're past the zoom time, just take the new value and stop lerping
		m_iFOVStart = fFOV;
	}
	else
	{
		fFOV = SimpleSplineRemapValClamped( deltaTime, 0.0f, 1.0f, m_iFOVStart, fFOV );
	}

	return fFOV;
}


//-----------------------------------------------------------------------------
// Get the current FOV used for network computations
// Choose the smallest FOV, as it will open the largest number of portals
//-----------------------------------------------------------------------------
int CBasePlayer::GetFOVForNetworking( void )
{
	int nDefaultFOV;

	// The vehicle's FOV wins if we're asking for a default value
	if ( GetVehicle() )
	{
		CacheVehicleView();
		nDefaultFOV = ( m_flVehicleViewFOV == 0 ) ? GetDefaultFOV() : (int) m_flVehicleViewFOV;
	}
	else
	{
		nDefaultFOV = GetDefaultFOV();
	}

	int fFOV = ( m_iFOV == 0 ) ? nDefaultFOV : m_iFOV;

	// If it's immediate, just do it
	if ( m_Local.m_flFOVRate == 0.0f )
		return fFOV;

	if ( gpGlobals->curtime - m_flFOVTime < m_Local.m_flFOVRate )
	{
		fFOV = MIN( fFOV, m_iFOVStart );
	}
	return fFOV;
}


float CBasePlayer::GetFOVDistanceAdjustFactorForNetworking()
{
	float defaultFOV	= (float)GetDefaultFOV();
	float localFOV		= (float)GetFOVForNetworking();

	if ( localFOV == defaultFOV || defaultFOV < 0.001f )
		return 1.0f;

	// If FOV is lower, then we're "zoomed" in and this will give a factor < 1 so apparent LOD distances can be
	//  shorted accordingly
	return localFOV / defaultFOV;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the default FOV for the player if nothing else is going on
// Input  : FOV - the new base FOV for this player
//-----------------------------------------------------------------------------
void CBasePlayer::SetDefaultFOV( int FOV )
{
	m_iDefaultFOV = ( FOV == 0 ) ? g_pGameRules->DefaultFOV() : FOV;
}

#ifdef MAPBASE_VSCRIPT
void CBasePlayer::ScriptSetFOV(int iFOV, float flRate)
{
	m_iFOVStart = GetFOV();

	m_flFOVTime = gpGlobals->curtime;
	m_iFOV = iFOV;

	m_Local.m_flFOVRate = flRate;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: // static func
// Input  : set - 
//-----------------------------------------------------------------------------
void CBasePlayer::ModifyOrAppendPlayerCriteria( AI_CriteriaSet& set )
{
	// Append our health
	set.AppendCriteria( "playerhealth", UTIL_VarArgs( "%i", GetHealth() ) );
	float healthfrac = 0.0f;
	if ( GetMaxHealth() > 0 )
	{
		healthfrac = (float)GetHealth() / (float)GetMaxHealth();
	}

	set.AppendCriteria( "playerhealthfrac", UTIL_VarArgs( "%.3f", healthfrac ) );

	CBaseCombatWeapon *weapon = GetActiveWeapon();
	if ( weapon )
	{
		set.AppendCriteria( "playerweapon", weapon->GetClassname() );
	}
	else
	{
		set.AppendCriteria( "playerweapon", "none" );
	}

	// Append current activity name
	set.AppendCriteria( "playeractivity", CAI_BaseNPC::GetActivityName( GetActivity() ) );

	set.AppendCriteria( "playerspeed", UTIL_VarArgs( "%.3f", GetAbsVelocity().Length() ) );

	AppendContextToCriteria( set, "player" );
}


const QAngle& CBasePlayer::GetPunchAngle()
{
	return m_Local.m_vecPunchAngle.Get();
}


void CBasePlayer::SetPunchAngle( const QAngle &punchAngle )
{
	m_Local.m_vecPunchAngle = punchAngle;

	if ( IsAlive() )
	{
		int index = entindex();

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer && i != index && pPlayer->GetObserverTarget() == this && pPlayer->GetObserverMode() == OBS_MODE_IN_EYE )
			{
				pPlayer->SetPunchAngle( punchAngle );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Apply a movement constraint to the player
//-----------------------------------------------------------------------------
void CBasePlayer::ActivateMovementConstraint( CBaseEntity *pEntity, const Vector &vecCenter, float flRadius, float flConstraintWidth, float flSpeedFactor )
{
	m_hConstraintEntity = pEntity;
	m_vecConstraintCenter = vecCenter;
	m_flConstraintRadius = flRadius;
	m_flConstraintWidth = flConstraintWidth;
	m_flConstraintSpeedFactor = flSpeedFactor;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::DeactivateMovementConstraint( )
{
	m_hConstraintEntity = NULL;
	m_flConstraintRadius = 0.0f;
	m_vecConstraintCenter = vec3_origin;
}

//-----------------------------------------------------------------------------
// Perhaps a poorly-named function. This function traces against the supplied
// NPC's hitboxes (instead of hull). If the trace hits a different NPC, the 
// new NPC is selected. Otherwise, the supplied NPC is determined to be the 
// one the citizen wants. This function allows the selection of a citizen over
// another citizen's shoulder, which is impossible without tracing against
// hitboxes instead of the hull (sjb)
//-----------------------------------------------------------------------------
CBaseEntity *CBasePlayer::DoubleCheckUseNPC( CBaseEntity *pNPC, const Vector &vecSrc, const Vector &vecDir )
{
	trace_t tr;

	UTIL_TraceLine( vecSrc, vecSrc + vecDir * 1024, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	if( tr.m_pEnt != NULL && tr.m_pEnt->MyNPCPointer() && tr.m_pEnt != pNPC )
	{
		// Player is selecting a different NPC through some negative space
		// in the first NPC's hitboxes (between legs, over shoulder, etc).
		return tr.m_pEnt;
	}

	return pNPC;
}


bool CBasePlayer::IsBot() const
{
	return (GetFlags() & FL_FAKECLIENT) != 0;
}

bool CBasePlayer::IsFakeClient() const
{
	return (GetFlags() & FL_FAKECLIENT) != 0;
}

void CBasePlayer::EquipSuit( bool bPlayEffects )
{ 
	m_Local.m_bWearingSuit = true; 
}

void CBasePlayer::RemoveSuit( void )
{
	m_Local.m_bWearingSuit = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &tr - 
//			nDamageType - 
//-----------------------------------------------------------------------------
void CBasePlayer::DoImpactEffect( trace_t &tr, int nDamageType )
{
	if ( GetActiveWeapon() )
	{
		GetActiveWeapon()->DoImpactEffect( tr, nDamageType );
		return;
	}

	BaseClass::DoImpactEffect( tr, nDamageType );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBasePlayer::InputSetHealth( inputdata_t &inputdata )
{
	int iNewHealth = inputdata.value.Int();
	int iDelta = abs(GetHealth() - iNewHealth);
	if ( iNewHealth > GetHealth() )
	{
		TakeHealth( iDelta, DMG_GENERIC );
	}
	else if ( iNewHealth < GetHealth() )
	{
		// Strip off and restore armor so that it doesn't absorb any of this damage.
		int armor = m_ArmorValue;
		m_ArmorValue = 0;
		TakeDamage( CTakeDamageInfo( this, this, iDelta, DMG_GENERIC ) );
		m_ArmorValue = armor;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBasePlayer::InputHandleMapEvent( inputdata_t &inputdata )
{
	Internal_HandleMapEvent( inputdata );
}

//-----------------------------------------------------------------------------
// Purpose: Hides or displays the HUD
// Input  : &inputdata -
//-----------------------------------------------------------------------------
void CBasePlayer::InputSetHUDVisibility( inputdata_t &inputdata )
{
	bool bEnable = inputdata.value.Bool();

	if ( bEnable )
	{
		m_Local.m_iHideHUD &= ~HIDEHUD_ALL;
	}
	else
	{
		m_Local.m_iHideHUD |= HIDEHUD_ALL;
	}
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata -
//-----------------------------------------------------------------------------
void CBasePlayer::InputSetSuppressAttacks( inputdata_t &inputdata )
{
	inputdata.value.Bool() ?
		AddSpawnFlags( SF_PLAYER_SUPPRESS_FIRING ) :
		RemoveSpawnFlags( SF_PLAYER_SUPPRESS_FIRING );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Set the fog controller data per player.
// Input  : &inputdata -
//-----------------------------------------------------------------------------
void CBasePlayer::InputSetFogController( inputdata_t &inputdata )
{
	// Find the fog controller with the given name.
#ifdef MAPBASE // From Alien Swarm SDK
	CFogController *pFogController = NULL;
	if ( inputdata.value.FieldType() == FIELD_EHANDLE )
	{
		pFogController = dynamic_cast<CFogController*>( inputdata.value.Entity().Get() );
	}
	else
	{
		pFogController = dynamic_cast<CFogController*>( gEntList.FindEntityByName( NULL, inputdata.value.String() ) );
	}
#else
	CFogController *pFogController = dynamic_cast<CFogController*>( gEntList.FindEntityByName( NULL, inputdata.value.String() ) );
#endif
	if ( pFogController )
	{
		m_Local.m_PlayerFog.m_hCtrl.Set( pFogController );
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CBasePlayer::InitFogController( void )
{
	// Setup with the default master controller.
	m_Local.m_PlayerFog.m_hCtrl = FogSystem()->GetMasterFogController();
}

#ifdef MAPBASE // From Alien Swarm SDK
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CBasePlayer::InitPostProcessController( void )
{
	// Setup with the default master controller.
	m_hPostProcessCtrl = PostProcessSystem()->GetMasterPostProcessController();
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CBasePlayer::InitColorCorrectionController( void )
{
	m_hColorCorrectionCtrl = ColorCorrectionSystem()->GetMasterColorCorrection();
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CBasePlayer::InputSetPostProcessController( inputdata_t &inputdata )
{
	// Find the fog controller with the given name.
	CPostProcessController *pController = NULL;
	if ( inputdata.value.FieldType() == FIELD_EHANDLE )
	{
		pController = dynamic_cast<CPostProcessController*>( inputdata.value.Entity().Get() );
	}
	else
	{
		pController = dynamic_cast<CPostProcessController*>( gEntList.FindEntityByName( NULL, inputdata.value.String() ) );
	}

	if ( pController )
	{
		m_hPostProcessCtrl.Set( pController );
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CBasePlayer::InputSetColorCorrectionController( inputdata_t &inputdata )
{
	// Find the fog controller with the given name.
	CColorCorrection *pController = NULL;
	if ( inputdata.value.FieldType() == FIELD_EHANDLE )
	{
		pController = dynamic_cast<CColorCorrection*>( inputdata.value.Entity().Get() );
	}
	else
	{
		pController = dynamic_cast<CColorCorrection*>( gEntList.FindEntityByName( NULL, inputdata.value.String() ) );
	}

	if ( pController )
	{
		m_hColorCorrectionCtrl.Set( pController );
	}

}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//-----------------------------------------------------------------------------
void CBasePlayer::SetViewEntity( CBaseEntity *pEntity ) 
{ 
	m_hViewEntity = pEntity; 

	if ( m_hViewEntity )
	{
		engine->SetView( edict(), m_hViewEntity->edict() );
	}
	else
	{
		engine->SetView( edict(), edict() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Looks at the player's reserve ammo and also all his weapons for any ammo
//			of the specified type
// Input  : nAmmoIndex - ammo to look for
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBasePlayer::HasAnyAmmoOfType( int nAmmoIndex )
{
	// Must be a valid index
	if ( nAmmoIndex < 0 )
		return false;

	// If we have some in reserve, we're already done
	if ( GetAmmoCount( nAmmoIndex ) )
		return true;

	CBaseCombatWeapon *pWeapon;

	// Check all held weapons
	for ( int i=0; i < MAX_WEAPONS; i++ )
	{
		pWeapon = GetWeapon( i );

		if ( !pWeapon )
			continue;

		// We must use clips and use this sort of ammo
		if ( pWeapon->UsesClipsForAmmo1() && pWeapon->GetPrimaryAmmoType() == nAmmoIndex )
		{
			// If we have any ammo, we're done
			if ( pWeapon->HasPrimaryAmmo() )
				return true;
		}
		
		// We'll check both clips for the same ammo type, just in case
		if ( pWeapon->UsesClipsForAmmo2() && pWeapon->GetSecondaryAmmoType() == nAmmoIndex )
		{
			if ( pWeapon->HasSecondaryAmmo() )
				return true;
		}
	}	

	// We're completely without this type of ammo
	return false;
}

bool CBasePlayer::HandleVoteCommands( const CCommand &args )
{
	if( g_voteController == NULL )
		return false;

	if(  FStrEq( args[0], "Vote" ) )
	{
		if( args.ArgC() < 2 )
			return true;

		const char *arg2 = args[1];
		char szResultString[MAX_COMMAND_LENGTH];

		CVoteController::TryCastVoteResult nTryResult = g_voteController->TryCastVote( entindex(), arg2 );
		switch( nTryResult )
		{
		case CVoteController::CAST_OK:
			{
				Q_snprintf( szResultString, MAX_COMMAND_LENGTH, "Voting %s.\n", arg2 );
				break;
			}
		case CVoteController::CAST_FAIL_SERVER_DISABLE:
			{
				Q_snprintf( szResultString, MAX_COMMAND_LENGTH, "Vote failed: server disabled.\n" );
				break;
			}
		case CVoteController::CAST_FAIL_NO_ACTIVE_ISSUE:
			{
				Q_snprintf( szResultString, MAX_COMMAND_LENGTH, "A vote has not been called.\n" );
				break;
			}
		case CVoteController::CAST_FAIL_TEAM_RESTRICTED:
			{
				Q_snprintf( szResultString, MAX_COMMAND_LENGTH, "Vote failed: team restricted.\n" );
				break;
			}
		case CVoteController::CAST_FAIL_NO_CHANGES:
			{
				Q_snprintf( szResultString, MAX_COMMAND_LENGTH, "Vote failed: no changing vote.\n" );
				break;
			}
		case CVoteController::CAST_FAIL_DUPLICATE:
			{
				Q_snprintf( szResultString, MAX_COMMAND_LENGTH, "Vote failed: already voting %s.\n", arg2 );
				break;
			}
		case CVoteController::CAST_FAIL_VOTE_CLOSED:
			{
				Q_snprintf( szResultString, MAX_COMMAND_LENGTH, "Vote failed: voting closed.\n" );
				break;
			}
		case CVoteController::CAST_FAIL_SYSTEM_ERROR:
		default:
			{
				Q_snprintf( szResultString, MAX_COMMAND_LENGTH, "Vote failed: system error.\n" );
				break;
			}
		}

		DevMsg( "%s", szResultString );		

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//  return a string version of the players network (i.e steam) ID.
//
//-----------------------------------------------------------------------------
const char *CBasePlayer::GetNetworkIDString()
{
	//Tony; bots don't have network id's, and this can potentially crash, especially with plugins creating them.
	if (IsBot())
		return "__BOT__";

	//Tony; if networkidstring is null for any reason, the strncpy will crash!
	if (!m_szNetworkIDString)
		return "NULLID";

	const char *pStr = engine->GetPlayerNetworkIDString( edict() );
	Q_strncpy( m_szNetworkIDString, pStr ? pStr : "", sizeof(m_szNetworkIDString) );
	return m_szNetworkIDString; 
}

//-----------------------------------------------------------------------------
//  Assign the player a name
//-----------------------------------------------------------------------------
void CBasePlayer::SetPlayerName( const char *name )
{
	Assert( name );

	if ( name )
	{
		Assert( strlen(name) > 0 );

		Q_strncpy( m_szNetname, name, sizeof(m_szNetname) );
	}
}

//-----------------------------------------------------------------------------
// sets the "don't autokick me" flag on a player
//-----------------------------------------------------------------------------
class DisableAutokick
{
public:
	DisableAutokick( int userID )
	{
		m_userID = userID;
	}

	bool operator()( CBasePlayer *player )
	{
		if ( player->GetUserID() == m_userID )
		{
			Msg( "autokick is disabled for %s\n", player->GetPlayerName() );
			player->DisableAutoKick( true );
			return false; // don't need to check other players
		}

		return true; // keep looking at other players
	}

private:
	int m_userID;
};

//-----------------------------------------------------------------------------
// sets the "don't autokick me" flag on a player
//-----------------------------------------------------------------------------
CON_COMMAND( mp_disable_autokick, "Prevents a userid from being auto-kicked" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( args.ArgC() != 2 )
	{
		Msg( "Usage: mp_disable_autokick <userid>\n" );
		return;
	}

	int userID = atoi( args[1] );
	DisableAutokick disable( userID );
	ForEachPlayer( disable );
}

//-----------------------------------------------------------------------------
// Purpose: Toggle between the duck being on and off
//-----------------------------------------------------------------------------
void CBasePlayer::ToggleDuck( void )
{
	// Toggle the state
	m_bDuckToggled = !m_bDuckToggled;
}

//-----------------------------------------------------------------------------
// Just tells us how far the stick is from the center. No directional info
//-----------------------------------------------------------------------------
float CBasePlayer::GetStickDist()
{
	Vector2D controlStick;

	controlStick.x = m_flForwardMove;
	controlStick.y = m_flSideMove;

	return controlStick.Length();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayer::HandleAnimEvent( animevent_t *pEvent )
{
	if ((pEvent->type & AE_TYPE_NEWEVENTSYSTEM) && (pEvent->type & AE_TYPE_SERVER))
	{
		if ( pEvent->event == AE_RAGDOLL )
		{
			// Convert to ragdoll immediately
			CreateRagdollEntity();
			BecomeRagdollOnClient( vec3_origin );
 
			// Force the player to start death thinking
			SetThink(&CBasePlayer::PlayerDeathThink);
			SetNextThink( gpGlobals->curtime + 0.1f );
			return;
		}
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
//  CPlayerInfo functions (simple passthroughts to get around the CBasePlayer multiple inheritence limitation)
//-----------------------------------------------------------------------------
const char *CPlayerInfo::GetName()
{ 
	Assert( m_pParent );
	return m_pParent->GetPlayerName(); 
}

int	CPlayerInfo::GetUserID() 
{ 
	Assert( m_pParent );
	return engine->GetPlayerUserId( m_pParent->edict() ); 
}

const char *CPlayerInfo::GetNetworkIDString() 
{ 
	Assert( m_pParent );
	return m_pParent->GetNetworkIDString(); 
}

int	CPlayerInfo::GetTeamIndex() 
{ 
	Assert( m_pParent );
	return m_pParent->GetTeamNumber(); 
}  

void CPlayerInfo::ChangeTeam( int iTeamNum ) 
{ 
	Assert( m_pParent );
	m_pParent->ChangeTeam(iTeamNum); 
}

int	CPlayerInfo::GetFragCount() 
{ 
	Assert( m_pParent );
	return m_pParent->FragCount(); 
}

int	CPlayerInfo::GetDeathCount() 
{ 
	Assert( m_pParent );
	return m_pParent->DeathCount(); 
}

bool CPlayerInfo::IsConnected() 
{ 
	Assert( m_pParent );
	return m_pParent->IsConnected(); 
}

int	CPlayerInfo::GetArmorValue() 
{ 
	Assert( m_pParent );
	return m_pParent->ArmorValue(); 
}

bool CPlayerInfo::IsHLTV() 
{ 
	Assert( m_pParent );
	return m_pParent->IsHLTV(); 
}

bool CPlayerInfo::IsReplay()
{
#ifdef TF_DLL // FIXME: Need run-time check for whether replay is enabled
	Assert( m_pParent );
	return m_pParent->IsReplay();
#else
	return false;
#endif
}

bool CPlayerInfo::IsPlayer() 
{ 
	Assert( m_pParent );
	return m_pParent->IsPlayer(); 
}

bool CPlayerInfo::IsFakeClient() 
{ 
	Assert( m_pParent );
	return m_pParent->IsFakeClient(); 
}

bool CPlayerInfo::IsDead() 
{ 
	Assert( m_pParent );
	return m_pParent->IsDead(); 
}

bool CPlayerInfo::IsInAVehicle() 
{ 
	Assert( m_pParent );
	return m_pParent->IsInAVehicle(); 
}

bool CPlayerInfo::IsObserver() 
{ 
	Assert( m_pParent );
	return m_pParent->IsObserver(); 
}

const Vector CPlayerInfo::GetAbsOrigin() 
{ 
	Assert( m_pParent );
	return m_pParent->GetAbsOrigin(); 
}

const QAngle CPlayerInfo::GetAbsAngles() 
{ 
	Assert( m_pParent );
	return m_pParent->GetAbsAngles(); 
}

const Vector CPlayerInfo::GetPlayerMins() 
{ 
	Assert( m_pParent );
	return m_pParent->GetPlayerMins(); 
}

const Vector CPlayerInfo::GetPlayerMaxs() 
{ 
	Assert( m_pParent );
	return m_pParent->GetPlayerMaxs(); 
}

const char *CPlayerInfo::GetWeaponName() 
{ 
	Assert( m_pParent );
	CBaseCombatWeapon *weap = m_pParent->GetActiveWeapon();
	if ( !weap )
	{
		return NULL;
	}
	return weap->GetName();
}

const char *CPlayerInfo::GetModelName() 
{ 
	Assert( m_pParent );
	return m_pParent->GetModelName().ToCStr(); 
}

const int CPlayerInfo::GetHealth() 
{ 
	Assert( m_pParent );
	return m_pParent->GetHealth(); 
}

const int CPlayerInfo::GetMaxHealth() 
{ 
	Assert( m_pParent );
	return m_pParent->GetMaxHealth(); 
}





void CPlayerInfo::SetAbsOrigin( Vector & vec ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		m_pParent->SetAbsOrigin(vec); 
	}
}

void CPlayerInfo::SetAbsAngles( QAngle & ang ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		m_pParent->SetAbsAngles(ang); 
	}
}

void CPlayerInfo::RemoveAllItems( bool removeSuit ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		m_pParent->RemoveAllItems(removeSuit); 
	}
}

void CPlayerInfo::SetActiveWeapon( const char *WeaponName ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		CBaseCombatWeapon *weap = m_pParent->Weapon_Create( WeaponName );
		if ( weap )
		{
			m_pParent->Weapon_Equip(weap); 
			m_pParent->Weapon_Switch(weap); 
		}
	}
}

void CPlayerInfo::SetLocalOrigin( const Vector& origin ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		m_pParent->SetLocalOrigin(origin); 
	}
}

const Vector CPlayerInfo::GetLocalOrigin( void ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		Vector origin = m_pParent->GetLocalOrigin();
		return origin; 
	}
	else
	{
		return Vector( 0, 0, 0 );
	}
}

void CPlayerInfo::SetLocalAngles( const QAngle& angles ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		m_pParent->SetLocalAngles( angles ); 
	}
}

const QAngle CPlayerInfo::GetLocalAngles( void ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		return m_pParent->GetLocalAngles(); 
	}
	else
	{
		return QAngle();
	}
}

bool CPlayerInfo::IsEFlagSet( int nEFlagMask ) 
{ 
	Assert( m_pParent );
	if ( m_pParent->IsBot() )
	{
		return m_pParent->IsEFlagSet(nEFlagMask); 
	}
	return false;
}

void CPlayerInfo::RunPlayerMove( CBotCmd *ucmd ) 
{ 
	if ( m_pParent->IsBot() )
	{
		Assert( m_pParent );
		CUserCmd cmd;
		cmd.buttons = ucmd->buttons;
		cmd.command_number = ucmd->command_number;
		cmd.forwardmove = ucmd->forwardmove;
		cmd.hasbeenpredicted = ucmd->hasbeenpredicted;
		cmd.impulse = ucmd->impulse;
		cmd.mousedx = ucmd->mousedx;
		cmd.mousedy = ucmd->mousedy;
		cmd.random_seed = ucmd->random_seed;
		cmd.sidemove = ucmd->sidemove;
		cmd.tick_count = ucmd->tick_count;
		cmd.upmove = ucmd->upmove;
		cmd.viewangles = ucmd->viewangles;
		cmd.weaponselect = ucmd->weaponselect;
		cmd.weaponsubtype = ucmd->weaponsubtype;

		// Store off the globals.. they're gonna get whacked
		float flOldFrametime = gpGlobals->frametime;
		float flOldCurtime = gpGlobals->curtime;

		m_pParent->SetTimeBase( gpGlobals->curtime );

		MoveHelperServer()->SetHost( m_pParent );
		m_pParent->PlayerRunCommand( &cmd, MoveHelperServer() );

		// save off the last good usercmd
		m_pParent->SetLastUserCommand( cmd );

		// Clear out any fixangle that has been set
		m_pParent->pl.fixangle = FIXANGLE_NONE;

		// Restore the globals..
		gpGlobals->frametime = flOldFrametime;
		gpGlobals->curtime = flOldCurtime;
		MoveHelperServer()->SetHost( NULL );
	}
}

void CPlayerInfo::SetLastUserCommand( const CBotCmd &ucmd ) 
{ 
	if ( m_pParent->IsBot() )
	{
		Assert( m_pParent );
		CUserCmd cmd;
		cmd.buttons = ucmd.buttons;
		cmd.command_number = ucmd.command_number;
		cmd.forwardmove = ucmd.forwardmove;
		cmd.hasbeenpredicted = ucmd.hasbeenpredicted;
		cmd.impulse = ucmd.impulse;
		cmd.mousedx = ucmd.mousedx;
		cmd.mousedy = ucmd.mousedy;
		cmd.random_seed = ucmd.random_seed;
		cmd.sidemove = ucmd.sidemove;
		cmd.tick_count = ucmd.tick_count;
		cmd.upmove = ucmd.upmove;
		cmd.viewangles = ucmd.viewangles;
		cmd.weaponselect = ucmd.weaponselect;
		cmd.weaponsubtype = ucmd.weaponsubtype;

		m_pParent->SetLastUserCommand(cmd); 
	}
}


CBotCmd CPlayerInfo::GetLastUserCommand()
{
	CBotCmd cmd;
	const CUserCmd *ucmd = m_pParent->GetLastUserCommand();
	if ( ucmd )
	{
		cmd.buttons = ucmd->buttons;
		cmd.command_number = ucmd->command_number;
		cmd.forwardmove = ucmd->forwardmove;
		cmd.hasbeenpredicted = ucmd->hasbeenpredicted;
		cmd.impulse = ucmd->impulse;
		cmd.mousedx = ucmd->mousedx;
		cmd.mousedy = ucmd->mousedy;
		cmd.random_seed = ucmd->random_seed;
		cmd.sidemove = ucmd->sidemove;
		cmd.tick_count = ucmd->tick_count;
		cmd.upmove = ucmd->upmove;
		cmd.viewangles = ucmd->viewangles;
		cmd.weaponselect = ucmd->weaponselect;
		cmd.weaponsubtype = ucmd->weaponsubtype;
	}
	return cmd;
}

// Notify that I've killed some other entity. (called from Victim's Event_Killed).
void CBasePlayer::Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	BaseClass::Event_KilledOther( pVictim, info );
	if ( pVictim != this )
	{
		gamestats->Event_PlayerKilledOther( this, pVictim, info );
	}
	else
	{
		gamestats->Event_PlayerSuicide( this );
	}
}

void CBasePlayer::SetModel( const char *szModelName )
{
	BaseClass::SetModel( szModelName );
	m_nBodyPitchPoseParam = LookupPoseParameter( "body_pitch" );
}

void CBasePlayer::SetBodyPitch( float flPitch )
{
	if ( m_nBodyPitchPoseParam >= 0 )
	{
		SetPoseParameter( m_nBodyPitchPoseParam, flPitch );
	}
}

void CBasePlayer::AdjustDrownDmg( int nAmount )
{
	m_idrowndmg += nAmount;
	if ( m_idrowndmg < m_idrownrestored )
	{
		m_idrowndmg = m_idrownrestored;
	}
}



#if !defined(NO_STEAM)
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBasePlayer::GetSteamID( CSteamID *pID )
{
	const CSteamID *pClientID = engine->GetClientSteamID( edict() );
	if ( pClientID )
	{
		*pID = *pClientID;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
uint64 CBasePlayer::GetSteamIDAsUInt64( void )
{
	CSteamID steamIDForPlayer;
	if ( GetSteamID( &steamIDForPlayer ) )
		return steamIDForPlayer.ConvertToUint64();
	return 0;
}
#endif // NO_STEAM

#ifdef MAPBASE // From Alien Swarm SDK
//--------------------------------------------------------------------------------------------------------
void CBasePlayer::UpdateFXVolume( void )
{
	CFogController *pFogController = NULL;
	CPostProcessController *pPostProcessController = NULL;
	CColorCorrection* pColorCorrectionEnt = NULL;

	Vector eyePos;
	CBaseEntity *pViewEntity = GetViewEntity();
	if ( pViewEntity )
	{
		eyePos = pViewEntity->GetAbsOrigin();
	}
	else
	{
		eyePos = EyePosition();
	}

	CFogVolume *pFogVolume = CFogVolume::FindFogVolumeForPosition( eyePos );
	if ( pFogVolume )
	{
		pFogController = pFogVolume->GetFogController();
		pPostProcessController = pFogVolume->GetPostProcessController();
		pColorCorrectionEnt = pFogVolume->GetColorCorrectionController();

		if ( !pFogController )
		{
			pFogController = FogSystem()->GetMasterFogController();
		}

		if ( !pPostProcessController )
		{
			pPostProcessController = PostProcessSystem()->GetMasterPostProcessController();
		}

		if ( !pColorCorrectionEnt )
		{
			pColorCorrectionEnt = ColorCorrectionSystem()->GetMasterColorCorrection();
		}
	}
	else if ( TheFogVolumes.Count() > 0 )
	{
		// If we're not in a fog volume, clear our fog volume, if the map has any.
		// This will get us back to using the master fog controller.
		pFogController = FogSystem()->GetMasterFogController();
		pPostProcessController = PostProcessSystem()->GetMasterPostProcessController();
		pColorCorrectionEnt = ColorCorrectionSystem()->GetMasterColorCorrection();
	}

	if ( pFogController && m_Local.m_PlayerFog.m_hCtrl.Get() != pFogController )
	{
		m_Local.m_PlayerFog.m_hCtrl.Set( pFogController );
	}

	if ( pPostProcessController )
	{
		m_hPostProcessCtrl.Set( pPostProcessController );
	}

	if ( pColorCorrectionEnt )
	{
		m_hColorCorrectionCtrl.Set( pColorCorrectionEnt );
	}
}
#endif
