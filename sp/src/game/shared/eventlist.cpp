//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "eventlist.h"
#include "stringregistry.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// NOTE: If CStringRegistry allowed storing arbitrary data, we could just use that.
// in this case we have the "isPrivate" member and the replacement rules 
// (eventIndex can be reused by private activities), so a custom table is necessary
struct eventlist_t
{
	int					eventIndex;
	int					iType;
	unsigned short		stringKey;
	short				isPrivate;
};

CUtlVector<eventlist_t> g_EventList;

// This stores the actual event names.  Also, the string ID in the registry is simply an index 
// into the g_EventList array.
CStringRegistry	g_EventStrings;

// this is just here to accelerate adds
static int g_HighestEvent = 0;

int g_nEventListVersion = 1;


void EventList_Init( void )
{
	g_HighestEvent = 0;
}

void EventList_Free( void )
{
	g_EventStrings.ClearStrings();
	g_EventList.Purge();

	// So studiohdrs can reindex event indices
	++g_nEventListVersion;
}

// add a new event to the database
eventlist_t *EventList_AddEventEntry( const char *pName, int iEventIndex, bool isPrivate, int iType )
{
	MEM_ALLOC_CREDIT();
	int index = g_EventList.AddToTail();
	eventlist_t *pList = &g_EventList[index];
	pList->eventIndex = iEventIndex;
	pList->stringKey = g_EventStrings.AddString( pName, index );
	pList->isPrivate = isPrivate;
	pList->iType = iType;	

	// UNDONE: This implies that ALL shared activities are added before ANY custom activities
	// UNDONE: Segment these instead?  It's a 32-bit int, how many activities do we need?
	if ( iEventIndex > g_HighestEvent )
	{
		g_HighestEvent = iEventIndex;
	}

	return pList;
}

// get the database entry from a string
static eventlist_t *ListFromString( const char *pString )
{
	// just use the string registry to do this search/map
	int stringID = g_EventStrings.GetStringID( pString );
	if ( stringID < 0 )
		return NULL;

	return &g_EventList[stringID];
}

// Get the database entry for an index
static eventlist_t *ListFromEvent( int eventIndex )
{
	// ugly linear search
	for ( int i = 0; i < g_EventList.Size(); i++ )
	{
		if ( g_EventList[i].eventIndex == eventIndex )
		{
			return &g_EventList[i];
		}
	}

	return NULL;
}

int EventList_GetEventType( int eventIndex )
{
	eventlist_t *pEvent = ListFromEvent( eventIndex );

	if ( pEvent )
	{
		return pEvent->iType;
	}

	return -1;
}


bool EventList_RegisterSharedEvent( const char *pszEventName, int iEventIndex, int iType )
{
	// UNDONE: Do we want to do these checks when not in developer mode? or maybe DEBUG only?
	// They really only matter when you change the list of code controlled activities.  IDs
	// for content controlled activities never collide because they are generated.

	// first, check to make sure the slot we're asking for is free. It must be for 
	// a shared event.
	eventlist_t *pList = ListFromString( pszEventName );
	if ( !pList )
	{
		pList = ListFromEvent( iEventIndex );
	}

	//Already in list.
	if ( pList )
	{
		return false;
	}
	// ----------------------------------------------------------------

	EventList_AddEventEntry( pszEventName, iEventIndex, false, iType );
	return true;
}

Animevent EventList_RegisterPrivateEvent( const char *pszEventName )
{
	eventlist_t *pList = ListFromString( pszEventName );
	if ( pList )
	{
		// this activity is already in the list. If the activity we collided with is also private, 
		// then the collision is OK. Otherwise, it's a bug.
		if ( pList->isPrivate )
		{
			return (Animevent)pList->eventIndex;
		}
		else
		{
			// this private activity collides with a shared activity. That is not allowed.
			Warning( "***\nShared<->Private Event collision!\n***\n" );
			Assert(0);
			return AE_INVALID;
		}
	}

	pList = EventList_AddEventEntry( pszEventName, g_HighestEvent+1, true, AE_TYPE_SERVER );
	return (Animevent)pList->eventIndex;
}

// Get the index for a given Event name
// Done at load time for all models
int EventList_IndexForName( const char *pszEventName )
{
	// this is a fast O(lgn) search (actually does 2 O(lgn) searches)
	eventlist_t *pList = ListFromString( pszEventName );

	if ( pList )
	{
		return pList->eventIndex;
	}

	return -1;
}

// Get the name for a given index
// This should only be used in debug code, it does a linear search
// But at least it only compares integers
const char *EventList_NameForIndex( int eventIndex )
{
	eventlist_t *pList = ListFromEvent( eventIndex );
	if ( pList )
	{
		return g_EventStrings.GetStringForKey( pList->stringKey );
	}
	return NULL;
}

void EventList_RegisterSharedEvents( void )
{
	REGISTER_SHARED_ANIMEVENT( AE_EMPTY, AE_TYPE_SERVER );
	
	REGISTER_SHARED_ANIMEVENT( AE_NPC_LEFTFOOT, AE_TYPE_SERVER );
	REGISTER_SHARED_ANIMEVENT( AE_NPC_RIGHTFOOT, AE_TYPE_SERVER  );
	REGISTER_SHARED_ANIMEVENT( AE_NPC_BODYDROP_LIGHT, AE_TYPE_SERVER  );
	REGISTER_SHARED_ANIMEVENT( AE_NPC_BODYDROP_HEAVY, AE_TYPE_SERVER  );
	REGISTER_SHARED_ANIMEVENT( AE_NPC_SWISHSOUND, AE_TYPE_SERVER  );
	REGISTER_SHARED_ANIMEVENT( AE_NPC_180TURN, AE_TYPE_SERVER  );
	REGISTER_SHARED_ANIMEVENT( AE_NPC_ITEM_PICKUP, AE_TYPE_SERVER  );
	REGISTER_SHARED_ANIMEVENT( AE_NPC_WEAPON_DROP, AE_TYPE_SERVER  );
	REGISTER_SHARED_ANIMEVENT( AE_NPC_WEAPON_SET_SEQUENCE_NAME, AE_TYPE_SERVER  );
	REGISTER_SHARED_ANIMEVENT( AE_NPC_WEAPON_SET_SEQUENCE_NUMBER, AE_TYPE_SERVER  );
	REGISTER_SHARED_ANIMEVENT( AE_NPC_WEAPON_SET_ACTIVITY, AE_TYPE_SERVER  );
	REGISTER_SHARED_ANIMEVENT( AE_NPC_HOLSTER, AE_TYPE_SERVER  );
	REGISTER_SHARED_ANIMEVENT( AE_NPC_DRAW, AE_TYPE_SERVER  );
	REGISTER_SHARED_ANIMEVENT( AE_NPC_WEAPON_FIRE, AE_TYPE_SERVER | AE_TYPE_WEAPON );

	REGISTER_SHARED_ANIMEVENT( AE_CL_PLAYSOUND, AE_TYPE_CLIENT );
	REGISTER_SHARED_ANIMEVENT( AE_SV_PLAYSOUND, AE_TYPE_SERVER );
	REGISTER_SHARED_ANIMEVENT( AE_CL_STOPSOUND, AE_TYPE_CLIENT );

	REGISTER_SHARED_ANIMEVENT( AE_START_SCRIPTED_EFFECT, AE_TYPE_SERVER );
	REGISTER_SHARED_ANIMEVENT( AE_STOP_SCRIPTED_EFFECT, AE_TYPE_SERVER );

	REGISTER_SHARED_ANIMEVENT( AE_CLIENT_EFFECT_ATTACH, AE_TYPE_CLIENT );

	REGISTER_SHARED_ANIMEVENT( AE_MUZZLEFLASH, AE_TYPE_CLIENT );
	REGISTER_SHARED_ANIMEVENT( AE_NPC_MUZZLEFLASH, AE_TYPE_CLIENT );

	REGISTER_SHARED_ANIMEVENT( AE_THUMPER_THUMP, AE_TYPE_SERVER );
	REGISTER_SHARED_ANIMEVENT( AE_AMMOCRATE_PICKUP_AMMO, AE_TYPE_SERVER );

	REGISTER_SHARED_ANIMEVENT( AE_NPC_RAGDOLL, AE_TYPE_SERVER );

	REGISTER_SHARED_ANIMEVENT( AE_NPC_ADDGESTURE, AE_TYPE_SERVER );
	REGISTER_SHARED_ANIMEVENT( AE_NPC_RESTARTGESTURE, AE_TYPE_SERVER );

	REGISTER_SHARED_ANIMEVENT( AE_NPC_ATTACK_BROADCAST, AE_TYPE_SERVER );

	REGISTER_SHARED_ANIMEVENT( AE_NPC_HURT_INTERACTION_PARTNER, AE_TYPE_SERVER );
	REGISTER_SHARED_ANIMEVENT( AE_NPC_SET_INTERACTION_CANTDIE, AE_TYPE_SERVER );

	REGISTER_SHARED_ANIMEVENT( AE_SV_DUSTTRAIL, AE_TYPE_SERVER );

	REGISTER_SHARED_ANIMEVENT( AE_CL_CREATE_PARTICLE_EFFECT, AE_TYPE_CLIENT );

	REGISTER_SHARED_ANIMEVENT( AE_RAGDOLL, AE_TYPE_SERVER );

	REGISTER_SHARED_ANIMEVENT( AE_CL_ENABLE_BODYGROUP, AE_TYPE_CLIENT );
	REGISTER_SHARED_ANIMEVENT( AE_CL_DISABLE_BODYGROUP, AE_TYPE_CLIENT );
	REGISTER_SHARED_ANIMEVENT( AE_CL_BODYGROUP_SET_VALUE, AE_TYPE_CLIENT );
	REGISTER_SHARED_ANIMEVENT( AE_CL_BODYGROUP_SET_VALUE_CMODEL_WPN, AE_TYPE_CLIENT );

	REGISTER_SHARED_ANIMEVENT( AE_WPN_PRIMARYATTACK, AE_TYPE_CLIENT | AE_TYPE_SERVER );
	REGISTER_SHARED_ANIMEVENT( AE_WPN_INCREMENTAMMO, AE_TYPE_CLIENT | AE_TYPE_SERVER );

	REGISTER_SHARED_ANIMEVENT( AE_WPN_HIDE, AE_TYPE_CLIENT | AE_TYPE_SERVER );
	REGISTER_SHARED_ANIMEVENT( AE_WPN_UNHIDE, AE_TYPE_CLIENT | AE_TYPE_SERVER );

	REGISTER_SHARED_ANIMEVENT( AE_WPN_PLAYWPNSOUND, AE_TYPE_CLIENT | AE_TYPE_SERVER );
}