//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_basehelicopter.h"
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "KeyValues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_BaseHelicopter, DT_BaseHelicopter, CBaseHelicopter )
	RecvPropTime( RECVINFO( m_flStartupTime ) ),
END_RECV_TABLE()


C_BaseHelicopter::C_BaseHelicopter()
{
}


//-----------------------------------------------------------------------------
// Chopper blade fade-in time
//-----------------------------------------------------------------------------
#define FADE_IN_TIME	2.0f


//-----------------------------------------------------------------------------
// Sets the fade of the blades when the chopper starts up
//-----------------------------------------------------------------------------
class CHeliBladeMaterialProxy : public CEntityMaterialProxy
{
public:
	CHeliBladeMaterialProxy() { m_AlphaVar = NULL; }
	virtual ~CHeliBladeMaterialProxy() {}
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( C_BaseEntity *pEntity );
	virtual IMaterial *GetMaterial();

private:
	IMaterialVar *m_AlphaVar;
	bool m_bFadeOut;
};

bool CHeliBladeMaterialProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	bool foundVar;
	m_AlphaVar = pMaterial->FindVar( "$alpha", &foundVar, false );
	m_bFadeOut = pKeyValues->GetInt( "$fadeout", 0 ) != 0;
	return foundVar;
}

void CHeliBladeMaterialProxy::OnBind( C_BaseEntity *pEnt )
{
	if (!m_AlphaVar)
		return;

	C_BaseHelicopter *pHeli = dynamic_cast<C_BaseHelicopter*>( pEnt );
	if ( pHeli )
	{
		float dt = gpGlobals->curtime  - pHeli->StartupTime();
		dt /= FADE_IN_TIME;
		dt = clamp( dt, 0.0f, 1.0f );
		if ( m_bFadeOut ) 
		{
			dt = 1.0f - dt;
		}

		m_AlphaVar->SetFloatValue( dt );
	}
	else
	{
		m_AlphaVar->SetFloatValue( 1.0f );
	}
}

IMaterial *CHeliBladeMaterialProxy::GetMaterial()
{
	if ( !m_AlphaVar )
		return NULL;

	return m_AlphaVar->GetOwningMaterial();
}

EXPOSE_INTERFACE( CHeliBladeMaterialProxy, IMaterialProxy, "HeliBlade" IMATERIAL_PROXY_INTERFACE_VERSION );

