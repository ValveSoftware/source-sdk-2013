//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "c_obj_sapper.h"
#include "c_tf_player.h"
#include <igameevents.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Start thinking
//-----------------------------------------------------------------------------
void C_ObjectSapper::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create the sparking effect if we're built and ready
//-----------------------------------------------------------------------------
void C_ObjectSapper::ClientThink( void )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "building_info_changed" );
	if ( event )
	{
		event->SetInt( "building_type", OBJ_ATTACHMENT_SAPPER );
		event->SetInt( "object_mode", GetObjectMode() );
		gameeventmanager->FireEventClientSide( event );
	}
}

float C_ObjectSapper::GetReversesBuildingConstructionSpeed( void )
{
	float flReverseSpeed = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBuilder(), flReverseSpeed, sapper_degenerates_buildings );

	return flReverseSpeed;
}

IMPLEMENT_CLIENTCLASS_DT(C_ObjectSapper, DT_ObjectSapper, CObjectSapper)
END_RECV_TABLE()