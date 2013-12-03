//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Acts exactly like "AnimatedTexture", but ONLY if the texture 
//			it's working on matches the desired texture to work on.
//
//			This assumes that some other proxy will be switching out the textures.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "materialsystem/imaterialproxy.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/itexture.h"
#include "baseanimatedtextureproxy.h"
#include "utlstring.h"
#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CAnimateSpecificTexture : public CBaseAnimatedTextureProxy
{
private:
	CUtlString m_OnlyAnimateOnTexture;
public:
	virtual float GetAnimationStartTime( void* pBaseEntity ) { return 0; }
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );
	virtual void Release( void ) { delete this; }
};

bool CAnimateSpecificTexture::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	char const* pszAnimateOnTexture = pKeyValues->GetString( "onlyAnimateOnTexture" );
	if( !pszAnimateOnTexture )
		return false;

	m_OnlyAnimateOnTexture.Set( pszAnimateOnTexture );

	return CBaseAnimatedTextureProxy::Init( pMaterial, pKeyValues );
}

void CAnimateSpecificTexture::OnBind( void *pC_BaseEntity )
{
	if( FStrEq( m_AnimatedTextureVar->GetTextureValue()->GetName(), m_OnlyAnimateOnTexture ) )
	{
		CBaseAnimatedTextureProxy::OnBind( pC_BaseEntity );
	}
	//else do nothing
}

EXPOSE_INTERFACE( CAnimateSpecificTexture, IMaterialProxy, "AnimateSpecificTexture" IMATERIAL_PROXY_INTERFACE_VERSION );