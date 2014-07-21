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

			// Look for array specification...
			char pTemp[256];
			if (strchr(pVarName, '['))
			{		 
				// strip off the array...
				Q_strncpy( pTemp, pVarName, 256 );
				char *pArray = strchr( pTemp, '[' );
				*pArray++ = 0;

				char* pIEnd;
				m_FloatVecComp = strtol( pArray, &pIEnd, 10 );

				// Use the version without the array...
				pVarName = pTemp;
			}
			else
			{
				m_FloatVecComp = -1;
			}

			bool bFoundVar;
			m_pFloatVar = pMaterial->FindVar( pVarName, &bFoundVar, true );
			if (!bFoundVar)
				return false;
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
	char const* pResult = pKeyValues->GetString( "resultVar" );
	if( !pResult )
		return false;

	// Look for array specification...
	char pTemp[256];
	if (strchr(pResult, '['))
	{		 
		// strip off the array...
		Q_strncpy( pTemp, pResult, 256 );
		char *pArray = strchr( pTemp, '[' );
		*pArray++ = 0;

		char* pIEnd;
		m_ResultVecComp = strtol( pArray, &pIEnd, 10 );

		// Use the version without the array...
		pResult = pTemp;
	}
	else
	{
		m_ResultVecComp = -1;
	}

	bool foundVar;
	m_pResult = pMaterial->FindVar( pResult, &foundVar, true );
	if( !foundVar )
		return false;

	return true;
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

	bool foundVar;
	m_pSrc1 = pMaterial->FindVar( pSrcVar1, &foundVar, true );
	if( !foundVar )
		return false;

	// Source 2 is optional, some math ops may be single-input
	char const* pSrcVar2 = pKeyValues->GetString( "srcVar2" );
	if( pSrcVar2 && (*pSrcVar2) )
	{
		m_pSrc2 = pMaterial->FindVar( pSrcVar2, &foundVar, true );
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

