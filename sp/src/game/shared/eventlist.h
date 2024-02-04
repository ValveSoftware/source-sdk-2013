//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef EVENTLIST_H
#define EVENTLIST_H
#ifdef _WIN32
#pragma once
#endif

#define AE_TYPE_SERVER			( 1 << 0 )
#define AE_TYPE_SCRIPTED		( 1 << 1 )		// see scriptevent.h
#define AE_TYPE_SHARED			( 1 << 2 )
#define AE_TYPE_WEAPON			( 1 << 3 )
#define AE_TYPE_CLIENT			( 1 << 4 )
#define AE_TYPE_FACEPOSER		( 1 << 5 )

#define AE_TYPE_NEWEVENTSYSTEM  ( 1 << 10 ) //Temporary flag.

#define AE_NOT_AVAILABLE		-1

typedef enum
{
	AE_INVALID = -1,			// So we have something more succint to check for than '-1'
	AE_EMPTY,
	AE_NPC_LEFTFOOT, // #define	NPC_EVENT_LEFTFOOT			2050
	AE_NPC_RIGHTFOOT, // #define NPC_EVENT_RIGHTFOOT			2051
	AE_NPC_BODYDROP_LIGHT, //#define NPC_EVENT_BODYDROP_LIGHT	2001
	AE_NPC_BODYDROP_HEAVY, //#define NPC_EVENT_BODYDROP_HEAVY	2002
	AE_NPC_SWISHSOUND, //#define NPC_EVENT_SWISHSOUND		2010
	AE_NPC_180TURN, //#define NPC_EVENT_180TURN			2020
	AE_NPC_ITEM_PICKUP, //#define NPC_EVENT_ITEM_PICKUP					2040
	AE_NPC_WEAPON_DROP, //#define NPC_EVENT_WEAPON_DROP					2041
	AE_NPC_WEAPON_SET_SEQUENCE_NAME, //#define NPC_EVENT_WEAPON_SET_SEQUENCE_NAME		2042
	AE_NPC_WEAPON_SET_SEQUENCE_NUMBER, //#define NPC_EVENT_WEAPON_SET_SEQUENCE_NUMBER	2043
	AE_NPC_WEAPON_SET_ACTIVITY, //#define NPC_EVENT_WEAPON_SET_ACTIVITY			2044
	AE_NPC_HOLSTER,
	AE_NPC_DRAW,
	AE_NPC_WEAPON_FIRE,

	AE_CL_PLAYSOUND, // #define CL_EVENT_SOUND				5004	// Emit a sound 
	AE_SV_PLAYSOUND,
	AE_CL_STOPSOUND,

	AE_START_SCRIPTED_EFFECT,
	AE_STOP_SCRIPTED_EFFECT,

	AE_CLIENT_EFFECT_ATTACH,
	
	AE_MUZZLEFLASH,			// Muzzle flash from weapons held by the player
	AE_NPC_MUZZLEFLASH,		// Muzzle flash from weapons held by NPCs
	
	AE_THUMPER_THUMP,		//Thumper Thump!
	AE_AMMOCRATE_PICKUP_AMMO,	//Ammo crate pick up ammo!

	AE_NPC_RAGDOLL,

	AE_NPC_ADDGESTURE,
	AE_NPC_RESTARTGESTURE,

	AE_NPC_ATTACK_BROADCAST,

	AE_NPC_HURT_INTERACTION_PARTNER,
	AE_NPC_SET_INTERACTION_CANTDIE,

	AE_SV_DUSTTRAIL,

	AE_CL_CREATE_PARTICLE_EFFECT,
#ifdef MAPBASE // From Alien Swarm SDK
	AE_CL_STOP_PARTICLE_EFFECT,
	AE_CL_ADD_PARTICLE_EFFECT_CP,
	//AE_CL_CREATE_PARTICLE_BRASS,
#endif

	AE_RAGDOLL,

	AE_CL_ENABLE_BODYGROUP,
	AE_CL_DISABLE_BODYGROUP,
	AE_CL_BODYGROUP_SET_VALUE,
	AE_CL_BODYGROUP_SET_VALUE_CMODEL_WPN,

	AE_WPN_PRIMARYATTACK,	// Used by weapons that want their primary attack to occur during an attack anim (i.e. grenade throwing)
	AE_WPN_INCREMENTAMMO,

	AE_WPN_HIDE,		// Used to hide player weapons
	AE_WPN_UNHIDE,		// Used to unhide player weapons

	AE_WPN_PLAYWPNSOUND,	// Play a weapon sound from the weapon script file

#ifdef MAPBASE
	AE_NPC_RESPONSE,	// Play a response system concept if we're not speaking
	AE_NPC_RESPONSE_FORCED,		// Always play a response system concept
	
	AE_VSCRIPT_RUN,			// Run vscript code (server + client)
	AE_VSCRIPT_RUN_FILE,	// Run vscript file (server + client)
#endif

	LAST_SHARED_ANIMEVENT,
} Animevent;


typedef struct evententry_s evententry_t;

//=========================================================
//=========================================================
extern void EventList_Init( void );
extern void EventList_Free( void );
extern bool EventList_RegisterSharedEvent( const char *pszEventName, int iEventIndex, int iType = 0 );
extern Animevent EventList_RegisterPrivateEvent( const char *pszEventName );
extern int EventList_IndexForName( const char *pszEventName );
extern const char *EventList_NameForIndex( int iEventIndex );
Animevent EventList_RegisterPrivateEvent( const char *pszEventName );

// This macro guarantees that the names of each event and the constant used to
// reference it in the code are identical.
#define REGISTER_SHARED_ANIMEVENT( _n, b ) EventList_RegisterSharedEvent(#_n, _n, b );
#define REGISTER_PRIVATE_ANIMEVENT( _n ) _n = EventList_RegisterPrivateEvent( #_n );

// Implemented in shared code
extern void EventList_RegisterSharedEvents( void );
extern int EventList_GetEventType( int eventIndex );



#endif // EVENTLIST_H
