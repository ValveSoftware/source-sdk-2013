//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterialsystem.h"
#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// $sineVar : name of variable that controls the alpha level (float)
class CShieldProxy : public CEntityMaterialProxy
{
public:
	CShieldProxy();
	virtual ~CShieldProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( C_BaseEntity *pC_BaseEntity );
	virtual IMaterial *GetMaterial();

private:
	IMaterialVar *m_AlphaVar;
	IMaterialVar *m_pTextureScrollVar;
	float m_ScrollRate;
	float m_ScrollAngle;
};

CShieldProxy::CShieldProxy()
{
	m_AlphaVar				= NULL;
	m_pTextureScrollVar		= NULL;
	m_ScrollRate = 0;
	m_ScrollAngle = 0;
}

CShieldProxy::~CShieldProxy()
{
}


bool CShieldProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	bool foundVar;

	m_AlphaVar = pMaterial->FindVar( "$translucency", &foundVar, false );
	if( !foundVar )
		return false;

	char const* pScrollVarName = pKeyValues->GetString( "textureScrollVar" );
	if (!pScrollVarName)
		return false;

	m_pTextureScrollVar = pMaterial->FindVar( pScrollVarName, &foundVar, false );
	if( !foundVar )
		return false;

	m_ScrollRate = pKeyValues->GetFloat( "textureScrollRate", 1 );
	m_ScrollAngle = pKeyValues->GetFloat( "textureScrollAngle", 0 );
	return true;
}

void CShieldProxy::OnBind( C_BaseEntity *pEnt )
{
	if (m_AlphaVar)
	{
		m_AlphaVar->SetFloatValue( pEnt->m_clrRender->a );
	}

	if( !m_pTextureScrollVar )
	{
		return;
	}
	
	float sOffset, tOffset;
	
	sOffset = gpGlobals->curtime * sin( m_ScrollAngle * ( M_PI / 180.0f ) ) * m_ScrollRate;
	tOffset = gpGlobals->curtime * cos( m_ScrollAngle * ( M_PI / 180.0f ) ) * m_ScrollRate;

	// make sure that we are positive
	if( sOffset < 0.0f )
	{
		sOffset += 1.0f + -( int )sOffset;
	}
	if( tOffset < 0.0f )
	{
		tOffset += 1.0f + -( int )tOffset;
	}
	
	// make sure that we are in a [0,1] range
	sOffset = sOffset - ( int )sOffset;
	tOffset = tOffset - ( int )tOffset;
	
	m_pTextureScrollVar->SetVecValue( sOffset, tOffset, 0.0f );
}

IMaterial *CShieldProxy::GetMaterial()
{
	if ( !m_AlphaVar )
		return NULL;

	return m_AlphaVar->GetOwningMaterial();
}

EXPOSE_INTERFACE( CShieldProxy, IMaterialProxy, "Shield" IMATERIAL_PROXY_INTERFACE_VERSION );
