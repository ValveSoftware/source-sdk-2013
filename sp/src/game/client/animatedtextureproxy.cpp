//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "baseanimatedtextureproxy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CAnimatedTextureProxy : public CBaseAnimatedTextureProxy
{
public:
	CAnimatedTextureProxy() {}
	virtual ~CAnimatedTextureProxy() {}
	virtual float GetAnimationStartTime( void* pBaseEntity );
};

EXPOSE_INTERFACE( CAnimatedTextureProxy, IMaterialProxy, "AnimatedTexture" IMATERIAL_PROXY_INTERFACE_VERSION );

#pragma warning (disable : 4100)

float CAnimatedTextureProxy::GetAnimationStartTime( void* pBaseEntity )
{
	return 0;
}

