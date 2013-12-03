//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "materialsystem/imaterialproxy.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "c_world.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// forward declarations
void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );

class CWorldDimsProxy : public IMaterialProxy
{
public:
	CWorldDimsProxy();
	virtual ~CWorldDimsProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );
	virtual void Release( void ) { delete this; }
	virtual IMaterial *GetMaterial();


public:
	IMaterialVar *m_pMinsVar;
	IMaterialVar *m_pMaxsVar;
};


CWorldDimsProxy::CWorldDimsProxy()
{
	m_pMinsVar = m_pMaxsVar = NULL;
}

CWorldDimsProxy::~CWorldDimsProxy()
{
}

bool CWorldDimsProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	m_pMinsVar = pMaterial->FindVar( "$world_mins", NULL, false );
	m_pMaxsVar = pMaterial->FindVar( "$world_maxs", NULL, false );
	return true;
}

void CWorldDimsProxy::OnBind( void *pC_BaseEntity )
{
	if ( m_pMinsVar && m_pMaxsVar )
	{
		C_World *pWorld = GetClientWorldEntity();
		if ( pWorld )
		{
			m_pMinsVar->SetVecValue( (const float*)&pWorld->m_WorldMins, 3 );
			m_pMaxsVar->SetVecValue( (const float*)&pWorld->m_WorldMaxs, 3 );
		}
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

IMaterial *CWorldDimsProxy::GetMaterial()
{
	return m_pMinsVar->GetOwningMaterial();
}

EXPOSE_INTERFACE( CWorldDimsProxy, IMaterialProxy, "WorldDims" IMATERIAL_PROXY_INTERFACE_VERSION );


