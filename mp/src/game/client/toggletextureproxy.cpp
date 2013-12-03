//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "toggletextureproxy.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include <KeyValues.h>
#include "functionproxy.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// forward declarations
void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );

EXPOSE_INTERFACE( CBaseToggleTextureProxy, IMaterialProxy, "ToggleTexture" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Constructor, destructor: 
//-----------------------------------------------------------------------------

CBaseToggleTextureProxy::CBaseToggleTextureProxy()
{
	Cleanup();
}

CBaseToggleTextureProxy::~CBaseToggleTextureProxy()
{
	Cleanup();
}

C_BaseEntity *CBaseToggleTextureProxy::BindArgToEntity( void *pArg )
{
	IClientRenderable *pRend = (IClientRenderable *)pArg;
	return pRend->GetIClientUnknown()->GetBaseEntity();
}

//-----------------------------------------------------------------------------
// Initialization, shutdown
//-----------------------------------------------------------------------------
bool CBaseToggleTextureProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	char const* pTextureVarName = pKeyValues->GetString( "toggleTextureVar" );
	if( !pTextureVarName )
		return false;

	bool foundVar;
	m_TextureVar = pMaterial->FindVar( pTextureVarName, &foundVar, false );
	if( !foundVar )
		return false;

	char const* pTextureFrameNumVarName = pKeyValues->GetString( "toggleTextureFrameNumVar" );
	if( !pTextureFrameNumVarName )
		return false;

	m_TextureFrameNumVar = pMaterial->FindVar( pTextureFrameNumVarName, &foundVar, false );
	if( !foundVar )
		return false;
	
	m_WrapAnimation = !!pKeyValues->GetInt( "toggleShouldWrap", 1 );
	return true;
}

void CBaseToggleTextureProxy::Cleanup()
{
	m_TextureVar = NULL;
	m_TextureFrameNumVar = NULL;
}


//-----------------------------------------------------------------------------
// Does the dirty deed
//-----------------------------------------------------------------------------
void CBaseToggleTextureProxy::OnBind( void *pC_BaseEntity )
{
	assert ( m_TextureVar );

	if (!pC_BaseEntity)
		return;

	if( m_TextureVar->GetType() != MATERIAL_VAR_TYPE_TEXTURE )
	{
		return;
	}

	ITexture *pTexture = NULL;

	pTexture = m_TextureVar->GetTextureValue();

	if ( pTexture == NULL )
		 return;

	C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );

	if ( pEntity == NULL )
		 return;
	
	int numFrames = pTexture->GetNumAnimationFrames();
	int frame = pEntity->GetTextureFrameIndex();

	int intFrame = ((int)frame) % numFrames; 

	if ( m_WrapAnimation == false )
	{
		if ( frame > numFrames )
			 intFrame = numFrames;
	}
		
	m_TextureFrameNumVar->SetIntValue( intFrame );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

IMaterial *CBaseToggleTextureProxy::GetMaterial()
{
	return m_TextureFrameNumVar->GetOwningMaterial();
}
