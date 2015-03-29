//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "functionproxy.h"
#include <KeyValues.h>
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterial.h"
#include "iclientrenderable.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Get the named material variable and the vector component, if applicable.
static bool GetMaterialVariable(IMaterial* pMaterial, const char* pVarName,
		IMaterialVar*& pMaterialVar, int& vecComp)
{
	// Look for array specification...
	char pTemp[256];
	if (strchr(pVarName, '['))
	{
		// strip off the array...
		Q_strncpy( pTemp, pVarName, 256 );
		char *pArray = strchr( pTemp, '[' );
		*pArray++ = 0;

		char* pIEnd;
		vecComp = strtol( pArray, &pIEnd, 10 );

		// Use the version without the array...
		pVarName = pTemp;
	}
	else
	{
		vecComp = -1;
	}

	bool foundVar;
	pMaterialVar = pMaterial->FindVar( pVarName, &foundVar, true );
	return foundVar;
}

// Return the material's float value if vecComp is < 0, otherwise
// return the given component of the material's vector.
static float GetMaterialFloat(const IMaterialVar& material, int vecComp)
{
	if( vecComp < 0 )
		return material.GetFloatValue();

	int iVecSize = material.VectorSize();
	if ( vecComp >= iVecSize )
		return 0;

	float v[4];
	material.GetVecValue( v, iVecSize );
	return v[vecComp];
}

//-----------------------------------------------------------------------------
// Helper class to deal with floating point inputs
//-----------------------------------------------------------------------------
bool CFloatInput::Init( IMaterial *pMaterial, KeyValues *pKeyValues, const char *pKeyName, float flDefault )
{
	m_pFloatVar = NULL;
	KeyValues *pSection = pKeyValues->FindKey( pKeyName );
	if (pSection)
	{
		if (pSection->GetDataType() == KeyValues::TYPE_STRING)
		{
			const char *pVarName = pSection->GetString();

			// Look for numbers...
			float flValue;
			int nCount = sscanf( pVarName, "%f", &flValue );
			if (nCount == 1)
			{
				m_flValue = flValue;
				return true;
			}

			return GetMaterialVariable(pMaterial, pVarName, m_pFloatVar, m_FloatVecComp);
		}
		else
		{
			m_flValue = pSection->GetFloat();
		}
	}
	else
	{
		m_flValue = flDefault;
	}
	return true;
}

float CFloatInput::GetFloat() const
{
	if (!m_pFloatVar)
		return m_flValue;
	
	if( m_FloatVecComp < 0 )
		return m_pFloatVar->GetFloatValue();

	int iVecSize = m_pFloatVar->VectorSize();
	if ( m_FloatVecComp >= iVecSize )
		return 0;
	
	float v[4];
	m_pFloatVar->GetVecValue( v, iVecSize );
	return v[m_FloatVecComp];
}



//-----------------------------------------------------------------------------
//
// Result proxy; a result (with vector friendliness)
//
//-----------------------------------------------------------------------------

CResultProxy::CResultProxy() : m_pResult(0)
{
}

CResultProxy::~CResultProxy()
{
}


bool CResultProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	char const* pVarName = pKeyValues->GetString( "resultVar" );
	if( !pVarName )
		return false;

	return GetMaterialVariable(pMaterial, pVarName, m_pResult, m_ResultVecComp);
}


//-----------------------------------------------------------------------------
// A little code to allow us to set single components of vectors
//-----------------------------------------------------------------------------
void CResultProxy::SetFloatResult( float result )
{
	if (m_pResult->GetType() == MATERIAL_VAR_TYPE_VECTOR)
	{		
		if ( m_ResultVecComp >= 0 )
		{
			m_pResult->SetVecComponentValue( result, m_ResultVecComp );
		}
		else
		{
			float v[4];
			int vecSize = m_pResult->VectorSize();

			for (int i = 0; i < vecSize; ++i)
				v[i] = result;

			m_pResult->SetVecValue( v, vecSize );
		}		
	}
	else
	{
		m_pResult->SetFloatValue( result );
	}
}

C_BaseEntity *CResultProxy::BindArgToEntity( void *pArg )
{
	IClientRenderable *pRend = (IClientRenderable *)pArg;
	return pRend ? pRend->GetIClientUnknown()->GetBaseEntity() : NULL;
}

IMaterial *CResultProxy::GetMaterial()
{
	return m_pResult->GetOwningMaterial();
}


//-----------------------------------------------------------------------------
//
// Base functional proxy; two sources (one is optional) and a result
//
//-----------------------------------------------------------------------------

CFunctionProxy::CFunctionProxy() : m_pSrc1(0), m_pSrc2(0)
{
}

CFunctionProxy::~CFunctionProxy()
{
}


bool CFunctionProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	char const* pSrcVar1 = pKeyValues->GetString( "srcVar1" );
	if( !pSrcVar1 )
		return false;

	bool foundVar = GetMaterialVariable(pMaterial, pSrcVar1, m_pSrc1, m_Src1VecComp);
	if( !foundVar )
		return false;

	// Source 2 is optional, some math ops may be single-input
	char const* pSrcVar2 = pKeyValues->GetString( "srcVar2" );
	if( pSrcVar2 && (*pSrcVar2) )
	{
		foundVar = GetMaterialVariable(pMaterial, pSrcVar2, m_pSrc2, m_Src2VecComp);
		if( !foundVar )
			return false;
	}
	else
	{
		m_pSrc2 = 0;
	}

	return true;
}


void CFunctionProxy::ComputeResultType( MaterialVarType_t& resultType, int& vecSize )
{
	// Feh, this is ugly. Basically, don't change the result type
	// unless it's undefined.
	resultType = m_pResult->GetType();
	if (resultType == MATERIAL_VAR_TYPE_VECTOR)
	{
		if (m_ResultVecComp >= 0)
			resultType = MATERIAL_VAR_TYPE_FLOAT;
		vecSize = m_pResult->VectorSize();
	}
	else if (resultType == MATERIAL_VAR_TYPE_UNDEFINED)
	{
		resultType = m_pSrc1->GetType();
		if (resultType == MATERIAL_VAR_TYPE_VECTOR)
		{
			vecSize = m_pSrc1->VectorSize();
		}
		else if ((resultType == MATERIAL_VAR_TYPE_UNDEFINED) && m_pSrc2)
		{
			resultType = m_pSrc2->GetType();
			if (resultType == MATERIAL_VAR_TYPE_VECTOR)
			{
				vecSize = m_pSrc2->VectorSize();
			}
		}
	}
}

float CFunctionProxy::GetSrc1Float() const
{
	return GetMaterialFloat(*m_pSrc1, m_Src1VecComp);
}

float CFunctionProxy::GetSrc2Float() const
{
	return GetMaterialFloat(*m_pSrc2, m_Src2VecComp);
}
