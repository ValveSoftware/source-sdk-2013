//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A base class for all material proxies in the tf client dll
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_proxyentity.h"
#include "materialsystem/imaterialvar.h"


CBaseInvisMaterialProxy::CBaseInvisMaterialProxy()
{
	m_pPercentInvisible = NULL;
}


bool CBaseInvisMaterialProxy::Init( IMaterial *pMaterial, KeyValues* pKeyValues )
{
	bool bFound;
	m_pPercentInvisible = pMaterial->FindVar( "$cloakfactor", &bFound );

	return bFound;
}

void CBaseInvisMaterialProxy::Release()
{
	delete this;
}

IMaterial *CBaseInvisMaterialProxy::GetMaterial()
{
	if ( !m_pPercentInvisible )
		return NULL;

	return m_pPercentInvisible->GetOwningMaterial();
}

void CBaseInvisMaterialProxy::OnBindNotEntity( void *pRenderable )
{
	if ( m_pPercentInvisible )
	{
		m_pPercentInvisible->SetFloatValue( 0.0f );
	}
}
