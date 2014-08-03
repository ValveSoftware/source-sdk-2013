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

class CAnimatedEntityTextureProxy : public CBaseAnimatedTextureProxy
{
public:
	CAnimatedEntityTextureProxy() {}
	virtual ~CAnimatedEntityTextureProxy() {}

	virtual float GetAnimationStartTime( void* pBaseEntity );
	virtual void AnimationWrapped( void* pC_BaseEntity );

};

EXPOSE_INTERFACE( CAnimatedEntityTextureProxy, IMaterialProxy, "AnimatedEntityTexture" IMATERIAL_PROXY_INTERFACE_VERSION );

float CAnimatedEntityTextureProxy::GetAnimationStartTime( void* pArg )
{
	IClientRenderable *pRend = (IClientRenderable *)pArg;
	if (!pRend)
		return 0.0f;

	C_BaseEntity* pEntity = pRend->GetIClientUnknown()->GetBaseEntity();
	if (pEntity)
	{
		return pEntity->GetTextureAnimationStartTime();
	}
	return 0.0f;
}

void CAnimatedEntityTextureProxy::AnimationWrapped( void* pArg )
{
	IClientRenderable *pRend = (IClientRenderable *)pArg;
	if (!pRend)
		return;

	C_BaseEntity* pEntity = pRend->GetIClientUnknown()->GetBaseEntity();
	if (pEntity)
	{
		pEntity->TextureAnimationWrapped();
	}
}
