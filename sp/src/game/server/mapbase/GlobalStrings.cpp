//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ==================
//
// Purpose: See GlobalStrings.h for more information.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "GlobalStrings.h"


// Global strings must be initially declared here.
// Be sure to sync them with the externs in GlobalStrings.h.

// -------------------------------------------------------------
// 
// Classnames
// 
// -------------------------------------------------------------

#ifdef HL2_DLL
string_t gm_isz_class_Shotgun;
string_t gm_isz_class_SMG1;
string_t gm_isz_class_AR2;
string_t gm_isz_class_Pistol;
string_t gm_isz_class_Stunstick;
string_t gm_isz_class_Crowbar;
string_t gm_isz_class_RPG;
string_t gm_isz_class_357;
string_t gm_isz_class_Grenade;
string_t gm_isz_class_Physcannon;
string_t gm_isz_class_Crossbow;

string_t gm_isz_class_Strider;
string_t gm_isz_class_Gunship;
string_t gm_isz_class_Dropship;
string_t gm_isz_class_FloorTurret;
string_t gm_isz_class_CScanner;
string_t gm_isz_class_ClawScanner;
string_t gm_isz_class_Rollermine;
#endif

string_t gm_isz_class_Bullseye;

string_t gm_isz_class_PropPhysics;
string_t gm_isz_class_PropPhysicsOverride;
string_t gm_isz_class_FuncPhysbox;
string_t gm_isz_class_EnvFire;

// -------------------------------------------------------------

string_t gm_isz_name_player;
string_t gm_isz_name_activator;

// -------------------------------------------------------------

// -------------------------------------------------------------

// We know it hasn't been allocated yet
#define INITIALIZE_GLOBAL_STRING(string, text) string = AllocPooledString(text) //SetGlobalString(string, text)

void InitGlobalStrings()
{
#ifdef HL2_DLL
	INITIALIZE_GLOBAL_STRING(gm_isz_class_Shotgun, "weapon_shotgun");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_SMG1, "weapon_smg1");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_AR2, "weapon_ar2");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_Pistol, "weapon_pistol");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_Stunstick, "weapon_stunstick");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_Crowbar, "weapon_crowbar");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_RPG, "weapon_rpg");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_357, "weapon_357");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_Grenade, "weapon_frag");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_Physcannon, "weapon_physcannon");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_Crossbow, "weapon_crossbow");

	INITIALIZE_GLOBAL_STRING(gm_isz_class_Strider, "npc_strider");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_Gunship, "npc_combinegunship");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_Dropship, "npc_combinedropship");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_FloorTurret, "npc_turret_floor");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_CScanner, "npc_cscanner");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_ClawScanner, "npc_clawscanner");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_Rollermine, "npc_rollermine");
#endif

	INITIALIZE_GLOBAL_STRING(gm_isz_class_Bullseye, "npc_bullseye");

	INITIALIZE_GLOBAL_STRING(gm_isz_class_PropPhysics, "prop_physics");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_PropPhysicsOverride, "prop_physics_override");
	INITIALIZE_GLOBAL_STRING(gm_isz_class_FuncPhysbox, "func_physbox"); 
	INITIALIZE_GLOBAL_STRING(gm_isz_class_EnvFire, "env_fire");

	INITIALIZE_GLOBAL_STRING(gm_isz_name_player, "!player");
	INITIALIZE_GLOBAL_STRING(gm_isz_name_activator, "!activator");
}

// -------------------------------------------------------------



