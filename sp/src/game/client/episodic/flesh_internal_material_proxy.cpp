//========= Copyright Valve Corporation, All rights reserved. ============//
// Purpose: 
//
// $NoKeywords: $
//=====================================================================================//
#include "cbase.h"
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "debugoverlay_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_FleshEffectTarget;
void AddFleshProxyTarget( C_FleshEffectTarget *pTarget );
void RemoveFleshProxy( C_FleshEffectTarget *pTarget );

//=============================================================================
// 
//  Flesh effect target (used for orchestrating the "Invisible Alyx" moment
//	
//=============================================================================

class C_FleshEffectTarget : public C_BaseEntity
{
	DECLARE_CLASS( C_FleshEffectTarget, C_BaseEntity );

public:
	float GetRadius( void ) 
	{
		if ( m_flScaleTime <= 0.0f )
			return m_flRadius;

		float dt = ( gpGlobals->curtime - m_flScaleStartTime );
		if ( dt >= m_flScaleTime )
			return m_flRadius;

		return SimpleSplineRemapVal( ( dt / m_flScaleTime ), 0.0f, 1.0f, m_flStartRadius, m_flRadius );
	}

	virtual void Release( void )
	{
		// Remove us from the list of targets
		RemoveFleshProxy( this );
	}

	virtual void OnDataChanged( DataUpdateType_t updateType )
	{
		BaseClass::OnDataChanged( updateType );

		if ( updateType == DATA_UPDATE_CREATED )
		{
			// Add us to the list of flesh proxy targets
			AddFleshProxyTarget( this );
		}
	}

	float	m_flRadius;
	float	m_flStartRadius;
	float	m_flScaleStartTime;
	float	m_flScaleTime;

	DECLARE_CLIENTCLASS();
};

void RecvProxy_FleshEffect_Radius( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_FleshEffectTarget	*pTarget = (C_FleshEffectTarget	*) pStruct;
	float flRadius	= pData->m_Value.m_Float;

	//If changed, update our internal information
	if ( pTarget->m_flRadius != flRadius )
	{
		pTarget->m_flStartRadius	= pTarget->m_flRadius;
		pTarget->m_flScaleStartTime = gpGlobals->curtime;
	}
	
	pTarget->m_flRadius = flRadius;
}

IMPLEMENT_CLIENTCLASS_DT( C_FleshEffectTarget, DT_FleshEffectTarget, CFleshEffectTarget )
	RecvPropFloat( RECVINFO(m_flRadius), 0, RecvProxy_FleshEffect_Radius ),
	RecvPropFloat( RECVINFO(m_flScaleTime) ),
END_RECV_TABLE()

CUtlVector< C_FleshEffectTarget * >	g_FleshProxyTargets;

void AddFleshProxyTarget( C_FleshEffectTarget *pTarget )
{
	// Take it!
	g_FleshProxyTargets.AddToTail( pTarget );
}

void RemoveFleshProxy( C_FleshEffectTarget *pTarget )
{
	int nIndex = g_FleshProxyTargets.Find( pTarget );
	if ( nIndex != g_FleshProxyTargets.InvalidIndex() )
	{
		g_FleshProxyTargets.Remove( nIndex );
	}
}

// $sineVar : name of variable that controls the FleshInterior level (float)
class CFleshInteriorMaterialProxy : public CEntityMaterialProxy
{
public:
	CFleshInteriorMaterialProxy();
	virtual ~CFleshInteriorMaterialProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( C_BaseEntity *pEntity );
	virtual IMaterial *GetMaterial();

private:
	IMaterialVar *m_pMaterialParamFleshEffectCenterRadius1;
	IMaterialVar *m_pMaterialParamFleshEffectCenterRadius2;
	IMaterialVar *m_pMaterialParamFleshEffectCenterRadius3;
	IMaterialVar *m_pMaterialParamFleshEffectCenterRadius4;
	IMaterialVar *m_pMaterialParamFleshGlobalOpacity;
	IMaterialVar *m_pMaterialParamFleshSubsurfaceTint;
};

CFleshInteriorMaterialProxy::CFleshInteriorMaterialProxy()
{
	m_pMaterialParamFleshEffectCenterRadius1 = NULL;
	m_pMaterialParamFleshEffectCenterRadius2 = NULL;
	m_pMaterialParamFleshEffectCenterRadius3 = NULL;
	m_pMaterialParamFleshEffectCenterRadius4 = NULL;
	m_pMaterialParamFleshGlobalOpacity = NULL;
	m_pMaterialParamFleshSubsurfaceTint = NULL;
}

CFleshInteriorMaterialProxy::~CFleshInteriorMaterialProxy()
{
	// Do nothing
}

bool CFleshInteriorMaterialProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	bool bFoundVar = false;

	m_pMaterialParamFleshEffectCenterRadius1 = pMaterial->FindVar( "$FleshEffectCenterRadius1", &bFoundVar, false );
	if ( bFoundVar == false)
		return false;

	m_pMaterialParamFleshEffectCenterRadius2 = pMaterial->FindVar( "$FleshEffectCenterRadius2", &bFoundVar, false );
	if ( bFoundVar == false)
		return false;

	m_pMaterialParamFleshEffectCenterRadius3 = pMaterial->FindVar( "$FleshEffectCenterRadius3", &bFoundVar, false );
	if ( bFoundVar == false)
		return false;

	m_pMaterialParamFleshEffectCenterRadius4 = pMaterial->FindVar( "$FleshEffectCenterRadius4", &bFoundVar, false );
	if ( bFoundVar == false)
		return false;

	m_pMaterialParamFleshGlobalOpacity = pMaterial->FindVar( "$FleshGlobalOpacity", &bFoundVar, false );
	if ( bFoundVar == false)
		return false;

	m_pMaterialParamFleshSubsurfaceTint = pMaterial->FindVar( "$FleshSubsurfaceTint", &bFoundVar, false );
	if ( bFoundVar == false)
		return false;

	return true;
}

void CFleshInteriorMaterialProxy::OnBind( C_BaseEntity *pEnt )
{
	IMaterialVar *pParams[] =
	{
		m_pMaterialParamFleshEffectCenterRadius1,
		m_pMaterialParamFleshEffectCenterRadius2,
		m_pMaterialParamFleshEffectCenterRadius3,
		m_pMaterialParamFleshEffectCenterRadius4
	};

	float vEffectCenterRadius[4];
	for ( int i = 0; i < ARRAYSIZE( pParams ); i++ )
	{
		if ( i < g_FleshProxyTargets.Count() )
		{
			// Setup the target
			if ( g_FleshProxyTargets[i]->IsAbsQueriesValid() == false )
				continue;

			Vector vecAbsOrigin = g_FleshProxyTargets[i]->GetAbsOrigin();
			vEffectCenterRadius[0] = vecAbsOrigin.x;
			vEffectCenterRadius[1] = vecAbsOrigin.y;
			vEffectCenterRadius[2] = vecAbsOrigin.z;
			vEffectCenterRadius[3] = g_FleshProxyTargets[i]->GetRadius();
		}
		else
		{
			// Clear the target
			vEffectCenterRadius[0] = vEffectCenterRadius[1] = vEffectCenterRadius[2] = vEffectCenterRadius[3] = 0.0f;
		}

		// Set the value either way
		pParams[i]->SetVecValue( vEffectCenterRadius, 4 );
	}

	// Subsurface texture. NOTE: This texture bleeds through the color of the flesh texture so expect
	//   to have to set this brighter than white to really see the subsurface texture glow through.
	if ( m_pMaterialParamFleshSubsurfaceTint != NULL )
	{
		float vSubsurfaceTintColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		// !!! Test code. REPLACE ME!
		// vSubsurfaceTintColor[0] = vSubsurfaceTintColor[1] = vSubsurfaceTintColor[2] = sinf( gpGlobals->curtime * 3.0f ) + 1.0f;  // * 0.5f + 0.5f;

		m_pMaterialParamFleshSubsurfaceTint->SetVecValue( vSubsurfaceTintColor, 4 );
	}
}

IMaterial *CFleshInteriorMaterialProxy::GetMaterial()
{
	if ( m_pMaterialParamFleshEffectCenterRadius1 == NULL)
		return NULL;

	return m_pMaterialParamFleshEffectCenterRadius1->GetOwningMaterial();
}

EXPOSE_INTERFACE( CFleshInteriorMaterialProxy, IMaterialProxy, "FleshInterior" IMATERIAL_PROXY_INTERFACE_VERSION );
