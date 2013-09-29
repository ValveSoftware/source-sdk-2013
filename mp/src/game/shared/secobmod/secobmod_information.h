//====================================================//
//  Source Engine: CoOperative Base Mod Information.  //
//====================================================//
//SecobMod__IFDEF_Fixes  These are mainly kept to show where if defined lines have had to be changed to ifdef for code to compile.
//SecobMod__IFDEF_Info   These are like the above but have information describing why the fix is in place.
//SecobMod__MiscFixes    These are small fixes to the code to allow things to run correctly.
//SecobMod__MiscFixes	   These are like the above but have information describing why the fix was required.
//SecobMod__Information  These are the main comment lines found in the source code explaining many things in detail.
//SecobMod__ChangeME!    These are lines of code that require changing/you may wish to change for the game to run correctly once you start modifying the base.
//SecobMod__FixMe		   These are bugs in the code which require a fix before they'll work.

//A list of known bugs can be found on the Valve wiki page, these tend to be somewhat minor and beyond my fixing abilities as well (which isn't saying much admittedly).


//================================================//
//  Co-Op Singleplayer Maps Information.          //
//================================================//
// To those wishing to make a co-op version of the Half-Life 2/Ep1/2 maps, Mulekick on the steam forums gave this advice for older maps:
// - This does mean that you will need to start using the Everything solution file, which means more work settings things up again -
//Basically brushes that block LOS from AI's won't work with older maps.
//To fix this:
//In public/bspflags.h, change the line starting with
//#define MASK_BLOCKLOS (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_BLOCKLOS)
//to:
//#define MASK_BLOCKLOS (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_BLOCKLOS|CONTENTS_OPAQUE)

//=========================//
//  GameInfo.txt Backup.   //
//=========================//
/*
"GameInfo"
{
	game	"Source Engine Co-Operative Base Modification (2013)"
	title	""
	title2	""
	type multiplayer_only
	nomodels 0
	nohimodel 0
	nocrosshair 0
	hidden_maps
	{
		"test_speakers"		1
		"test_hardware"		1
	}


	FileSystem
	{
		SteamAppId				243750
		
		SearchPaths
		{		
			//=======================================
			//Secob Game Search Paths
			//=======================================
			//SecobMod__ Allow custom content by user .vpk packaged mods.
			game+mod			hl2mp/custom/*
			game+mod			hl2/custom/*

			//SecobMod__This is where we load in the Source SDK's hl2 vpk files.
			//If we load the half-life2's files then we crash the game on startup, and these work just fine.
			game				|all_source_engine_paths|hl2/hl2_english.vpk
			game				|all_source_engine_paths|hl2/hl2_pak.vpk
			game				|all_source_engine_paths|hl2/hl2_textures.vpk
			game				|all_source_engine_paths|hl2/hl2_sound_vo_english.vpk
			game				|all_source_engine_paths|hl2/hl2_sound_misc.vpk
			game				|all_source_engine_paths|hl2/hl2_misc.vpk
			platform			|all_source_engine_paths|platform/platform_misc.vpk

			//SecobMod__ Set the gameinfo.txt's location to that of the mod search path here and add the bin folder.
			game+mod+mod_write+default_write_path		|gameinfo_path|.
			gamebin				|gameinfo_path|bin
			
			//SecobMod__ Set the Source SDK 2013 Multiplayer (hl2mp) folder as the game search and write path.
			game+game_write		hl2mp
			gamebin				hl2mp/bin
			game+mod			|all_source_engine_paths|hl2mp
			game+mod			hl2mp/hl2mp_pak.vpk
			Game				|gameinfo_path|.

			//SecobMod__Finally we find any other missing content.
			game				|all_source_engine_paths|hl2mp
			game				|all_source_engine_paths|hl2
			platform			|all_source_engine_paths|platform

			
			
			//=======================================
			//Valve Game Search Paths
			//=======================================
			//SecobMod__Mount our additional games here
			//This allows you to use other content in your games and load some maps from other games (but some maps crash to desktop, and portal2 maps can't be loaded due to version mismatch).
			
		//Episode 2	
		game "|gameinfo_path|..\..\common\Half-Life 2\ep2\ep2_sound_vo_english.vpk"	
		game "|gameinfo_path|..\..\common\Half-Life 2\ep2\ep2_pak.vpk"

		//Half-Life Source
		game "|gameinfo_path|..\..\common\Half-Life 2\hl1_hd\hl1_hd_pak.vpk"
		game "|gameinfo_path|..\..\common\Half-Life 2\hl1\hl1_sound_vo_english.vpk"
		game "|gameinfo_path|..\..\common\Half-Life 2\hl1\hl1_pak.vpk"
		
		//Counter Strike Source
		game "|gameinfo_path|..\..\common\Counter-Strike Source\cstrike"
		game "|gameinfo_path|..\..\common\Counter-Strike Source\cstrike\cstrike_english.vpk"
		game "|gameinfo_path|..\..\common\Counter-Strike Source\cstrike\cstrike_pak.vpk"
		
		//Portal1
		game "|gameinfo_path|..\..\common\Portal\portal"
		game "|gameinfo_path|..\..\common\Portal\portal\portal_pak.vpk"
		
		//Portal2
		game "|gameinfo_path|..\..\common\Portal 2\portal2"
		game "|gameinfo_path|..\..\common\Portal 2\portal2\pak01.vpk"

		//Episode1
		game "|gameinfo_path|..\..\common\Half-Life 2\episodic\ep1_pak.vpk"
		game "|gameinfo_path|..\..\common\Half-Life 2\episodic\ep1_sound_vo_english.vpk"
		
		//Team Fortress 2
		game "|gameinfo_path|..\..\common\Team Fortress 2\tf"
		game "|gameinfo_path|..\..\common\Team Fortress 2\tf\tf2_textures.vpk"
		game "|gameinfo_path|..\..\common\Team Fortress 2\tf\tf2_sound_vo_english.vpk"
		game "|gameinfo_path|..\..\common\Team Fortress 2\tf\tf2_sound_misc.vpk"
		game "|gameinfo_path|..\..\common\Team Fortress 2\tf\tf2_misc.vpk"
	
			
		//SecobMod__If we want to load any official maps from any Valve .vpk enabled game we add the folder paths to them here.
		game "|gameinfo_path|..\..\common\Half-Life 2\ep2"	
		game "|gameinfo_path|..\..\common\Half-Life 2\episodic"
		game "|gameinfo_path|..\..\common\Half-Life 2\hl2"
		game "|gameinfo_path|..\..\common\Half-Life 2\hl1"
		game "|gameinfo_path|..\..\common\portal\portal"
		game "|gameinfo_path|..\..\common\Portal 2\portal2"
		game "|gameinfo_path|..\..\common\Team Fortress 2\tf"
		game "|gameinfo_path|..\..\common\Counter-Strike Source\cstrike"
		}
	}
}
*/

//====================================================//
//  Complete list of modified files. 				  //
//====================================================//
/*
achievements_hlx.cpp 
activitylist.cpp 
ai_activity.cpp 
ai_activity.h 
ai_allymanager.cpp 
ai_basenpc.cpp 
ai_basenpc.h 
ai_basenpc_schedule.cpp 
ai_behavior_actbusy.cpp 
ai_behavior_fear.cpp 
ai_behavior_follow.cpp 
ai_behavior_lead.cpp 
ai_behavior_passenger.cpp 
ai_behavior_passenger.h 
ai_behavior_standoff.cpp 
ai_hint.cpp 
ai_moveprobe.cpp 
ai_planesolver.cpp 
ai_playerally.cpp 
ai_relationship.cpp 
ai_scriptconditions.cpp 
ai_scriptconditions.h 
ai_speech.cpp 
ai_utils.h 
antlion_maker.cpp 
baseanimating.h 
basecombatcharacter.cpp 
basecombatweapon_shared.cpp 
baseentity.cpp 
baseentity_shared.cpp 
baseflex.cpp 
basegrenade_shared.cpp 
baseplayer_shared.cpp 
baseviewmodel_shared.cpp 
baseviewmodel_shared.h 
baseviewport.cpp 
c_baseentity.cpp 
c_baseplayer.cpp 
c_baseplayer.h 
c_colorcorrection.cpp 
c_dynamiclight.cpp 
c_hl2mp_player.cpp 
c_hl2mp_player.h 
c_npc_advisor.cpp 
c_npc_antlionguard.cpp 
c_prop_scalable.cpp 
c_vehicle_airboat.cpp 
c_vehicle_jeep.cpp 
c_weapon_stunstick.cpp 
cdll_client_int.cpp 
classmenu.cpp 
client.cpp 
clientmode_shared.cpp 
colorcorrectionvolume.cpp 
combine_mine.cpp 
CommentarySystem.cpp 
effects.cpp 
EntityDissolve.cpp 
entitylist.cpp 
env_headcrabcanister.cpp 
env_zoom.cpp 
EnvHudHint.cpp 
EnvMessage.cpp 
ep2_gamestats.cpp 
func_break.cpp 
func_recharge.cpp 
func_tank.cpp 
func_tank.h 
fx_impact.cpp 
game.cpp 
gameinterface.cpp 
gameinterface.h 
gamemovement.cpp 
gamemovement.h 
gamerules.cpp 
gameweaponmanager.cpp 
geiger.cpp 
genericactor.cpp 
gib.cpp 
globals.cpp 
globals.h 
grenade_frag.cpp 
hl_gamemovement.cpp 
hl_gamemovement.h 
hl2_gamerules.cpp 
hl2_gamerules.h 
hl2_player.cpp 
hl2_usermessages.cpp 
hl2mp_client.cpp 
hl2mp_gamerules.cpp 
hl2mp_gamerules.h 
hl2mp_player.cpp 
hl2mp_player.h 
hl2mp_weapon_parse.cpp 
hl2mp_weapon_parse.h 
hud_ammo.cpp 
hud_basechat.cpp 
hud_battery.cpp 
hud_damageindicator.cpp 
hud_flashlight.cpp 
hud_health.cpp 
hud_locator.cpp 
hud_poisondamageindicator.cpp 
hud_posture.cpp 
hud_squadstatus.cpp 
hud_suitpower.cpp 
hud_zoom.cpp 
ilagcompensationmanager.h 
in_main.cpp 
item_battery.cpp 
item_dynamic_resupply.cpp 
item_suit.cpp 
iviewrender.h 
logicentities.cpp 
maprules.cpp 
message_entity.cpp 
monstermaker.cpp 
movevars_shared.cpp 
multiplay_gamerules.cpp 
multiplayer_animstate.cpp 
multiplayer_animstate.h 
npc_advisor.cpp 
npc_alyx_episodic.cpp 
npc_antlion.cpp 
npc_antlion.h 
npc_antlionguard.cpp 
npc_attackchopper.cpp 
npc_barnacle.cpp 
npc_barnacle.h 
npc_basescanner.cpp 
npc_BaseZombie.cpp 
npc_citizen17.cpp 
npc_citizen17.h 
npc_combine.cpp 
npc_combinedropship.cpp 
npc_combinegunship.cpp 
npc_combines.cpp 
npc_combines.h 
npc_dog.cpp 
npc_enemyfinder.cpp 
npc_headcrab.cpp 
npc_headcrab.h 
npc_metropolice.cpp 
npc_playercompanion.cpp 
npc_playercompanion.h 
npc_rollermine.cpp 
npc_scanner.cpp 
npc_strider.cpp 
npc_talker.cpp 
npc_turret.cpp 
npc_turret_floor.cpp 
npc_turret_floor.h 
npc_vortigaunt_episodic.cpp 
npc_zombine.cpp 
particlesystemquery.cpp 
physconstraint.cpp 
physics.cpp 
physics.h 
physics_npc_solver.cpp 
player.cpp 
player.h 
player_command.cpp 
player_lagcompensation.cpp 
player_pickup.cpp 
prop_combine_ball.cpp 
prop_thumper.cpp 
proto_sniper.cpp 
ragdoll_shared.cpp 
recipientfilter.cpp 
sceneentity.cpp 
script_intro.cpp 
shareddefs.h 
soundscape.cpp 
subs.cpp 
teamplayroundbased_gamerules.cpp 
test_stressentities.cpp 
trigger_portal.cpp 
triggers.cpp 
usercmd.cpp 
usercmd.h 
util.cpp 
util.h 
util_shared.cpp 
vehicle_airboat.cpp 
vehicle_crane.cpp 
vehicle_jeep.cpp 
vehicle_jeep.h 
vehicle_jeep_episodic.cpp 
vehicle_prisoner_pod.cpp 
vehicle_viewcontroller.cpp 
vguitextwindow.cpp 
view.cpp 
viewrender.h 
weapon_ar2.cpp 
weapon_ar2.h 
weapon_crossbow.cpp 
weapon_crowbar.cpp 
weapon_crowbar.h 
weapon_frag.cpp 
weapon_hl2mpbase.cpp 
weapon_hl2mpbase.h 
weapon_hl2mpbase_machinegun.h 
weapon_hl2mpbasebasebludgeon.h 
weapon_physcannon.cpp 
weapon_physcannon.h 
weapon_pistol.cpp 
weapon_rpg.cpp 
weapon_rpg.h 
weapon_selection.cpp 
weapon_shotgun.cpp 
weapon_slam.cpp 
weapon_smg1.cpp 
weapon_striderbuster.cpp 
weapon_stunstick.cpp*/