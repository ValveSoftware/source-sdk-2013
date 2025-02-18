//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "local_steam_shared_object_listener.h"
#include "tf_gc_client.h"


CLocalSteamSharedObjectListener::CLocalSteamSharedObjectListener()
{
	// Delayed add in case something derives from us and isn't fully constructed yet
	// so their SOCreated functions are setup in the VTable yet.
	GTFGCClientSystem()->AddLocalPlayerSOListener( this, false );
}


CLocalSteamSharedObjectListener::~CLocalSteamSharedObjectListener()
{
	GTFGCClientSystem()->RemoveLocalPlayerSOListener( this );
}
