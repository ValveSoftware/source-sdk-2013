//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "functionproxy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CTimeMaterialProxy : public CResultProxy
{
public:
	virtual void OnBind( void *pC_BaseEntity );
};					    

void CTimeMaterialProxy::OnBind( void *pC_BaseEntity )
{
	SetFloatResult( gpGlobals->curtime );
}

EXPOSE_INTERFACE( CTimeMaterialProxy, IMaterialProxy, "CurrentTime" IMATERIAL_PROXY_INTERFACE_VERSION );
