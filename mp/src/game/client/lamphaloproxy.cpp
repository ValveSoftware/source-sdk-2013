//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "proxyentity.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterial.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Used for halos on lamps, this material fades the sprite IN
// as the viewer nears.
//-----------------------------------------------------------------------------
class CLampHaloProxy : public CEntityMaterialProxy
{
public:
						CLampHaloProxy( void );
	virtual				~CLampHaloProxy( void );
	virtual bool		Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void		OnBind( C_BaseEntity *pC_BaseEntity );

	virtual IMaterial *	GetMaterial();

private:
	IMaterialVar		*m_pFadeValue;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CLampHaloProxy::CLampHaloProxy( void )
{
	m_pFadeValue = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CLampHaloProxy::~CLampHaloProxy( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get pointer to the color value
// Input  : *pMaterial - 
//-----------------------------------------------------------------------------
bool CLampHaloProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	assert( pMaterial );

	// Get pointers to material vars.

	// Need to get the color variable.
	bool found;
	m_pFadeValue = pMaterial->FindVar( "$alpha", &found );
	return found;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pC_BaseEntity - 
//-----------------------------------------------------------------------------
#define FADE_DIST	150

void CLampHaloProxy::OnBind( C_BaseEntity *pEnt )
{
	if ( !m_pFadeValue )
		return;
	
	Vector vecLocal = pEnt->GetAbsOrigin() - CurrentViewOrigin();
	VectorNormalize( vecLocal );

	float fade = fabs( vecLocal.z );

	// I hate these magic numbers here, will have to revise
	// (sjb)
	if( fade < 0.25 )
	{
		fade = 0.0;
	}
	else
	{
		fade = MIN( (fade - 0.25) * 1.35, 1.0f );
	}

	m_pFadeValue->SetFloatValue( fade );
}

IMaterial *CLampHaloProxy::GetMaterial()
{
	if ( !m_pFadeValue )
		return NULL;

	return m_pFadeValue->GetOwningMaterial();
}

EXPOSE_INTERFACE( CLampHaloProxy, IMaterialProxy, "lamphalo" IMATERIAL_PROXY_INTERFACE_VERSION );
