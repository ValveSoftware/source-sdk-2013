//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This module implements all the proxies used by the particle systems.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "particlemgr.h"
#include "materialsystem/imaterialproxy.h"
#include "materialsystem/imaterialvar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// ------------------------------------------------------------------------ //
// ParticleSphereProxy
// ------------------------------------------------------------------------ //

class ParticleSphereProxy : public IMaterialProxy
{
// IMaterialProxy overrides.
public:
	ParticleSphereProxy()
	{
	}

	virtual		~ParticleSphereProxy() 
	{
	}
	
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
	{
		m_pLightPosition = pMaterial->FindVar( "$light_position", NULL, false );
		m_pLightColor = pMaterial->FindVar( "$light_color", NULL, false );
		return true;
	}

	virtual void OnBind( void *pvParticleMgr )
	{
		if( !pvParticleMgr )
			return;

		CParticleMgr *pMgr = (CParticleMgr*)pvParticleMgr;
		CParticleLightInfo info;
		pMgr->GetDirectionalLightInfo( info );

		// Transform the light into camera space.
		Vector vTransformedPos = pMgr->GetModelView() * info.m_vPos;
		if ( m_pLightPosition )
			m_pLightPosition->SetVecValue( vTransformedPos.Base(), 3 );

		if ( m_pLightColor )
		{
			Vector vTotalColor = info.m_vColor * info.m_flIntensity;
			m_pLightColor->SetVecValue( vTotalColor.Base(), 3 );
		}
	}

	virtual void	Release( void ) { delete this; }

	virtual IMaterial *GetMaterial()
	{
		IMaterialVar *pVar = m_pLightPosition ? m_pLightPosition : m_pLightColor;
		if ( !pVar )
			return NULL;
		return pVar->GetOwningMaterial();
	}

private:

	IMaterialVar	*m_pLightPosition;
	IMaterialVar	*m_pLightColor;
};

EXPOSE_INTERFACE( ParticleSphereProxy, IMaterialProxy, "ParticleSphereProxy" IMATERIAL_PROXY_INTERFACE_VERSION );

