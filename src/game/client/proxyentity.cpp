//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "proxyentity.h"
#include "iclientrenderable.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Cleanup
//-----------------------------------------------------------------------------
void CEntityMaterialProxy::Release( void )
{ 
	delete this; 
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CEntityMaterialProxy::OnBind( void *pRenderable )
{
	if( !pRenderable )
	{
		OnBindNotEntity( pRenderable );
		return;
	}

	IClientRenderable *pRend = ( IClientRenderable* )pRenderable;
	C_BaseEntity *pEnt = pRend->GetIClientUnknown()->GetBaseEntity();
	if ( pEnt )
	{
		OnBind( pEnt );
		if ( ToolsEnabled() )
		{
			ToolFramework_RecordMaterialParams( GetMaterial() );
		}
	}
	else
	{
		OnBindNotEntity( pRenderable );
	}
}
