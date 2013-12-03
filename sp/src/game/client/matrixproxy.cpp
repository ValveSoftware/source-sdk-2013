//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "mathlib/vmatrix.h"
#include "functionproxy.h"
#include "materialsystem/imaterialvar.h"
#include <KeyValues.h>
#include "materialsystem/imaterial.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// forward declarations
void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );

class C_BaseEntity;

//-----------------------------------------------------------------------------
// Texture transform proxy
//-----------------------------------------------------------------------------
class CTextureTransformProxy : public CResultProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
	IMaterialVar *m_pCenterVar;
	IMaterialVar *m_pScaleVar;
	IMaterialVar *m_pRotateVar;
	IMaterialVar *m_pTranslateVar;
};

bool CTextureTransformProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	// All are optional...
	m_pCenterVar = NULL;
	m_pScaleVar = NULL;
	m_pRotateVar = NULL;
	m_pTranslateVar = NULL;

	bool bFoundVar;
	char const* pVarName = pKeyValues->GetString( "centerVar" );
	if( pVarName && pVarName[0] )
	{
		m_pCenterVar = pMaterial->FindVar( pVarName, &bFoundVar, false );
	}

	pVarName = pKeyValues->GetString( "scaleVar" );
	if( pVarName && pVarName[0] )
	{
		m_pScaleVar = pMaterial->FindVar( pVarName, &bFoundVar, false );
	}

	pVarName = pKeyValues->GetString( "rotateVar" );
	if( pVarName && pVarName[0] )
	{
		m_pRotateVar = pMaterial->FindVar( pVarName, &bFoundVar, false );
	}

	pVarName = pKeyValues->GetString( "translateVar" );
	if( pVarName && pVarName[0] )
	{
		m_pTranslateVar = pMaterial->FindVar( pVarName, &bFoundVar, false );
	}

	return CResultProxy::Init( pMaterial, pKeyValues );
}

void CTextureTransformProxy::OnBind( void *pC_BaseEntity )
{
	Vector2D center( 0.5, 0.5 );
	Vector2D translation( 0, 0 );

	VMatrix mat, temp;

	if (m_pCenterVar)
	{
		m_pCenterVar->GetVecValue( center.Base(), 2 );
	}
	MatrixBuildTranslation( mat, -center.x, -center.y, 0.0f );

	if (m_pScaleVar)
	{
		Vector2D scale;
		m_pScaleVar->GetVecValue( scale.Base(), 2 );
		MatrixBuildScale( temp, scale.x, scale.y, 1.0f );
		MatrixMultiply( temp, mat, mat );
	}

	if (m_pRotateVar)
	{
		float angle = m_pRotateVar->GetFloatValue( );
		MatrixBuildRotateZ( temp, angle );
		MatrixMultiply( temp, mat, mat );
	}
	MatrixBuildTranslation( temp, center.x, center.y, 0.0f );
	MatrixMultiply( temp, mat, mat );

	if (m_pTranslateVar)
	{
		m_pTranslateVar->GetVecValue( translation.Base(), 2 );
		MatrixBuildTranslation( temp, translation.x, translation.y, 0.0f );
		MatrixMultiply( temp, mat, mat );
	}

	m_pResult->SetMatrixValue( mat );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}



EXPOSE_INTERFACE( CTextureTransformProxy, IMaterialProxy, "TextureTransform" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Rotation proxy
//-----------------------------------------------------------------------------
class CMatrixRotateProxy : public CResultProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
	CFloatInput	m_Angle;
	IMaterialVar *m_pAxisVar;
};

bool CMatrixRotateProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	// All are optional...
	m_pAxisVar = NULL;

	bool bFoundVar;
	char const* pVarName = pKeyValues->GetString( "axisVar" );
	if( pVarName && pVarName[0] )
	{
		m_pAxisVar = pMaterial->FindVar( pVarName, &bFoundVar, false );
	}

	if (!m_Angle.Init( pMaterial, pKeyValues, "angle", 0 ))
		return false;

	return CResultProxy::Init( pMaterial, pKeyValues );
}

void CMatrixRotateProxy::OnBind( void *pC_BaseEntity )
{
	VMatrix mat;
	Vector axis( 0, 0, 1 );
	if (m_pAxisVar)
	{
		m_pAxisVar->GetVecValue( axis.Base(), 3 );
		if (VectorNormalize( axis ) < 1e-3)
		{
			axis.Init( 0, 0, 1 );
		}
	}

	MatrixBuildRotationAboutAxis( mat, axis, m_Angle.GetFloat() );
	m_pResult->SetMatrixValue( mat );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}



EXPOSE_INTERFACE( CMatrixRotateProxy, IMaterialProxy, "MatrixRotate" IMATERIAL_PROXY_INTERFACE_VERSION );

