//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include <KeyValues.h>
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/itexture.h"
#include "proxyentity.h"
#include "functionproxy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DEFAULT_OPEN_PUPIL_RATE 0.03
#define DEFAULT_CLOSE_PUPIL_RATE 0.1

//-----------------------------------------------------------------------------
// Returns the proximity of the player to the entity
//-----------------------------------------------------------------------------
class CPupilProxy : public CEntityMaterialProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( C_BaseEntity *pBaseEntity );
	virtual IMaterial *GetMaterial();

private:
	IMaterialVar *m_pAnimatedTextureVar;
	IMaterialVar *m_pAnimatedTextureFrameNumVar;
	IMaterialVar *m_pLightingVar;

	CFloatInput m_flPupilCloseRate;
	CFloatInput m_flPupilOpenRate;
};

bool CPupilProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	char const* pAnimatedTextureVarName = pKeyValues->GetString( "TextureVar" );
	if( !pAnimatedTextureVarName )
		return false;

	bool foundVar;
	m_pAnimatedTextureVar = pMaterial->FindVar( pAnimatedTextureVarName, &foundVar, false );
	if( !foundVar )
		return false;

	char const* pAnimatedTextureFrameNumVarName = pKeyValues->GetString( "TextureFrameNumVar" );
	if( !pAnimatedTextureFrameNumVarName )
		return false;

	m_pAnimatedTextureFrameNumVar = pMaterial->FindVar( pAnimatedTextureFrameNumVarName, &foundVar, false );
	if( !foundVar )
		return false;

	m_pLightingVar = pMaterial->FindVar( "$lighting", &foundVar, false );
	if( !foundVar )
	{
		Warning("Materials using the pupil proxy must have a field called $lighting which has a value of 0.5!\n" );
		return false;
	}

	m_flPupilCloseRate.Init( pMaterial, pKeyValues, "PupilCloseRate", DEFAULT_CLOSE_PUPIL_RATE );
	m_flPupilOpenRate.Init( pMaterial, pKeyValues, "PupilOpenRate", DEFAULT_OPEN_PUPIL_RATE );

	return true;
}

void CPupilProxy::OnBind( C_BaseEntity *pBaseEntity )
{
	if (!pBaseEntity || !m_pAnimatedTextureVar )
		return;

	if( m_pAnimatedTextureVar->GetType() != MATERIAL_VAR_TYPE_TEXTURE )
		return;

	ITexture *pTexture = m_pAnimatedTextureVar->GetTextureValue();
	int nFrameCount = pTexture->GetNumAnimationFrames();

	// Compute the lighting at the eye position of the entity; use it to dialate the pupil
	Vector forward;
	pBaseEntity->GetVectors( &forward, NULL, NULL );

	Vector eyePt = pBaseEntity->EyePosition();
	Vector color;
	engine->ComputeLighting( eyePt, &forward, false, color );

	// Compute the intensity...
	float flIntensity = ( 0.299f * color[0] + 0.587f * color[1] + 0.114f * color[2] ) * 0.5;
	flIntensity = clamp( flIntensity, 0.f, 1.f );
	float flLastIntensity = m_pLightingVar->GetFloatValue( );
	if ( flIntensity > flLastIntensity )
	{
		float flMaxChange = m_flPupilCloseRate.GetFloat() * gpGlobals->frametime;
		if ( flIntensity > (flMaxChange + flLastIntensity) )
		{
			flIntensity = flLastIntensity + flMaxChange;
		}
	}
	else
	{
		float flMaxChange = m_flPupilOpenRate.GetFloat() * gpGlobals->frametime;
		if ( flIntensity < (flLastIntensity - flMaxChange) )
		{
			flIntensity = flLastIntensity - flMaxChange;
		}
	}

	int nFrame = nFrameCount * flIntensity;
	nFrame = clamp( nFrame, 0, nFrameCount - 1 );

	m_pAnimatedTextureFrameNumVar->SetIntValue( nFrame );
	m_pLightingVar->SetFloatValue( flIntensity );
}

IMaterial *CPupilProxy::GetMaterial()
{
	if ( !m_pAnimatedTextureVar )
		return NULL;

	return m_pAnimatedTextureVar->GetOwningMaterial();
}

EXPOSE_INTERFACE( CPupilProxy, IMaterialProxy, "Pupil" IMATERIAL_PROXY_INTERFACE_VERSION );
