//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "sharedInterface.h"
#include "materialsystem/imaterial.h"
#include <KeyValues.h>
#include "materialsystem/imaterialvar.h"
#include "functionproxy.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// forward declarations
void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );

class C_BaseEntity;

//-----------------------------------------------------------------------------
// Adds two vars...
//-----------------------------------------------------------------------------

class CAddProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );
};

bool CAddProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	// Requires 2 args..
	bool ok = CFunctionProxy::Init( pMaterial, pKeyValues );
	ok = ok && m_pSrc2;
	return ok;
}

void CAddProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pSrc2 && m_pResult );

	MaterialVarType_t resultType;
	int vecSize;
	ComputeResultType( resultType, vecSize );

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector a, b, c;
			m_pSrc1->GetVecValue( a.Base(), vecSize ); 
			m_pSrc2->GetVecValue( b.Base(), vecSize ); 
			VectorAdd( a, b, c );
			m_pResult->SetVecValue( c.Base(), vecSize );
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		SetFloatResult( GetSrc1Float() + GetSrc2Float() );
		break;

	case MATERIAL_VAR_TYPE_INT:
		m_pResult->SetFloatValue( m_pSrc1->GetIntValue() + m_pSrc2->GetIntValue() );
		break;
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CAddProxy, IMaterialProxy, "Add" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Subtracts two vars...
//-----------------------------------------------------------------------------

class CSubtractProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );
};

bool CSubtractProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	// Requires 2 args..
	bool ok = CFunctionProxy::Init( pMaterial, pKeyValues );
	ok = ok && m_pSrc2;
	return ok;
}

void CSubtractProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pSrc2 && m_pResult );

	MaterialVarType_t resultType;
	int vecSize;
	ComputeResultType( resultType, vecSize );

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector a, b, c;
			m_pSrc1->GetVecValue( a.Base(), vecSize ); 
			m_pSrc2->GetVecValue( b.Base(), vecSize ); 
			VectorSubtract( a, b, c );
			m_pResult->SetVecValue( c.Base(), vecSize );
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		SetFloatResult( GetSrc1Float() - GetSrc2Float() );
		break;

	case MATERIAL_VAR_TYPE_INT:
		m_pResult->SetFloatValue( m_pSrc1->GetIntValue() - m_pSrc2->GetIntValue() );
		break;
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CSubtractProxy, IMaterialProxy, "Subtract" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Multiplies two vars...
//-----------------------------------------------------------------------------

class CMultiplyProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );
};

bool CMultiplyProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	// Requires 2 args..
	bool ok = CFunctionProxy::Init( pMaterial, pKeyValues );
	ok = ok && m_pSrc2;
	return ok;
}

void CMultiplyProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pSrc2 && m_pResult );

	MaterialVarType_t resultType;
	int vecSize;
	ComputeResultType( resultType, vecSize );

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector a, b, c;
			m_pSrc1->GetVecValue( a.Base(), vecSize ); 
			m_pSrc2->GetVecValue( b.Base(), vecSize ); 
			VectorMultiply( a, b, c );
			m_pResult->SetVecValue( c.Base(), vecSize );
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		SetFloatResult( GetSrc1Float() * GetSrc2Float() );
		break;

	case MATERIAL_VAR_TYPE_INT:
		m_pResult->SetFloatValue( m_pSrc1->GetIntValue() * m_pSrc2->GetIntValue() );
		break;
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}


EXPOSE_INTERFACE( CMultiplyProxy, IMaterialProxy, "Multiply" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// divides two vars...
//-----------------------------------------------------------------------------

class CDivideProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );
};

bool CDivideProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	// Requires 2 args..
	bool ok = CFunctionProxy::Init( pMaterial, pKeyValues );
	ok = ok && m_pSrc2;
	return ok;
}

void CDivideProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pSrc2 && m_pResult );

	MaterialVarType_t resultType;
	int vecSize;
	ComputeResultType( resultType, vecSize );

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector a, b, c;
			m_pSrc1->GetVecValue( a.Base(), vecSize ); 
			m_pSrc2->GetVecValue( b.Base(), vecSize ); 
			VectorDivide( a, b, c );
			m_pResult->SetVecValue( c.Base(), vecSize );
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		if (GetSrc2Float() != 0)
		{
			SetFloatResult( GetSrc1Float() / GetSrc2Float() );
		}
		else
		{
			SetFloatResult( GetSrc1Float() );
		}
		break;

	case MATERIAL_VAR_TYPE_INT:
		if (m_pSrc2->GetIntValue() != 0)
		{
			m_pResult->SetFloatValue( m_pSrc1->GetIntValue() / m_pSrc2->GetIntValue() );
		}
		else
		{
			m_pResult->SetFloatValue( m_pSrc1->GetIntValue() );
		}
		break;
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CDivideProxy, IMaterialProxy, "Divide" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// clamps a var...
//-----------------------------------------------------------------------------

class CClampProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
	CFloatInput m_Min;
	CFloatInput m_Max;
};

bool CClampProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CFunctionProxy::Init( pMaterial, pKeyValues ))
		return false;

	if (!m_Min.Init( pMaterial, pKeyValues, "min", 0 ))
		return false;

	if (!m_Max.Init( pMaterial, pKeyValues, "max", 1 ))
		return false;

	return true;
}

void CClampProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pResult );

	MaterialVarType_t resultType;
	int vecSize;
	ComputeResultType( resultType, vecSize );

	float flMin = m_Min.GetFloat();
	float flMax = m_Max.GetFloat();

	if (flMin > flMax)
	{
		float flTemp = flMin;
		flMin = flMax;
		flMax = flTemp;
	}

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector a;
			m_pSrc1->GetVecValue( a.Base(), vecSize );
			for (int i = 0; i < vecSize; ++i)
			{
				if (a[i] < flMin)
					a[i] = flMin;
				else if (a[i] > flMax)
					a[i] = flMax;
			}
			m_pResult->SetVecValue( a.Base(), vecSize );
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		{
			float src = GetSrc1Float();
			if (src < flMin)
				src = flMin;
			else if (src > flMax)
				src = flMax;
			SetFloatResult( src );
		}
		break;

	case MATERIAL_VAR_TYPE_INT:
		{
			int src = m_pSrc1->GetIntValue();
			if (src < flMin)
				src = flMin;
			else if (src > flMax)
				src = flMax;
			m_pResult->SetIntValue( src );
		}
		break;
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}


EXPOSE_INTERFACE( CClampProxy, IMaterialProxy, "Clamp" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Creates a sinusoid
//-----------------------------------------------------------------------------

// sinePeriod: time that it takes to go through whole sine wave in seconds (default: 1.0f)
// sineMax : the max value for the sine wave (default: 1.0f )
// sineMin: the min value for the sine wave  (default: 0.0f )
class CSineProxy : public CResultProxy
{
public:
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );

private:
	CFloatInput m_SinePeriod;
	CFloatInput m_SineMax;
	CFloatInput m_SineMin;
	CFloatInput m_SineTimeOffset;
};


bool CSineProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	if (!m_SinePeriod.Init( pMaterial, pKeyValues, "sinePeriod", 1.0f ))
		return false;
	if (!m_SineMax.Init( pMaterial, pKeyValues, "sineMax", 1.0f ))
		return false;
	if (!m_SineMin.Init( pMaterial, pKeyValues, "sineMin", 0.0f ))
		return false;
	if (!m_SineTimeOffset.Init( pMaterial, pKeyValues, "timeOffset", 0.0f ))
		return false;

	return true;
}

void CSineProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pResult );

	float flValue;
	float flSineTimeOffset = m_SineTimeOffset.GetFloat();
	float flSineMax = m_SineMax.GetFloat();
	float flSineMin = m_SineMin.GetFloat();
	float flSinePeriod = m_SinePeriod.GetFloat();
	if (flSinePeriod == 0)
		flSinePeriod = 1;

	// get a value in [0,1]
	flValue = ( sin( 2.0f * M_PI * (gpGlobals->curtime - flSineTimeOffset) / flSinePeriod ) * 0.5f ) + 0.5f;
	// get a value in [min,max]	
	flValue = ( flSineMax - flSineMin ) * flValue + flSineMin;
	
	SetFloatResult( flValue );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CSineProxy, IMaterialProxy, "Sine" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// copies a var...
//-----------------------------------------------------------------------------

class CEqualsProxy : public CFunctionProxy
{
public:
	void OnBind( void *pC_BaseEntity );
};


void CEqualsProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pResult );

	MaterialVarType_t resultType;
	int vecSize;
	ComputeResultType( resultType, vecSize );

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector a;
			m_pSrc1->GetVecValue( a.Base(), vecSize );
			m_pResult->SetVecValue( a.Base(), vecSize );
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		SetFloatResult( GetSrc1Float() );
		break;

	case MATERIAL_VAR_TYPE_INT:
		m_pResult->SetIntValue( m_pSrc1->GetIntValue() );
		break;
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}


EXPOSE_INTERFACE( CEqualsProxy, IMaterialProxy, "Equals" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Get the fractional part of a var
//-----------------------------------------------------------------------------

class CFracProxy : public CFunctionProxy
{
public:
	void OnBind( void *pC_BaseEntity );
};


void CFracProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pResult );

	MaterialVarType_t resultType;
	int vecSize;
	ComputeResultType( resultType, vecSize );

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector a;
			m_pSrc1->GetVecValue( a.Base(), vecSize );
			a[0] -= ( float )( int )a[0];
			a[1] -= ( float )( int )a[1];
			a[2] -= ( float )( int )a[2];
			m_pResult->SetVecValue( a.Base(), vecSize );
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		{
			float a = GetSrc1Float();
			a -= ( int )a;
			SetFloatResult( a );
		}
		break;

	case MATERIAL_VAR_TYPE_INT:
		// don't do anything besides assignment!
		m_pResult->SetIntValue( m_pSrc1->GetIntValue() );
		break;
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}


EXPOSE_INTERFACE( CFracProxy, IMaterialProxy, "Frac" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Get the Integer part of a var
//-----------------------------------------------------------------------------

class CIntProxy : public CFunctionProxy
{
public:
	void OnBind( void *pC_BaseEntity );
};

void CIntProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pResult );

	MaterialVarType_t resultType;
	int vecSize;
	ComputeResultType( resultType, vecSize );

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector a;
			m_pSrc1->GetVecValue( a.Base(), vecSize );
			a[0] = ( float )( int )a[0];
			a[1] = ( float )( int )a[1];
			a[2] = ( float )( int )a[2];
			m_pResult->SetVecValue( a.Base(), vecSize );
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		{
			float a = GetSrc1Float();
			a = ( float )( int )a;
			SetFloatResult( a );
		}
		break;

	case MATERIAL_VAR_TYPE_INT:
		// don't do anything besides assignment!
		m_pResult->SetIntValue( m_pSrc1->GetIntValue() );
		break;
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CIntProxy, IMaterialProxy, "Int" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Linear ramp proxy
//-----------------------------------------------------------------------------
class CLinearRampProxy : public CResultProxy
{
public:
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );

private:
	CFloatInput m_Rate;
	CFloatInput m_InitialValue;
};


bool CLinearRampProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	if (!m_Rate.Init( pMaterial, pKeyValues, "rate", 1 ))
		return false;

	if (!m_InitialValue.Init( pMaterial, pKeyValues, "initialValue", 0 ))
		return false;

	return true;
}

void CLinearRampProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pResult );

	float flValue;
	
	// get a value in [0,1]
	flValue = m_Rate.GetFloat() * gpGlobals->curtime + m_InitialValue.GetFloat();	
	SetFloatResult( flValue );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}



EXPOSE_INTERFACE( CLinearRampProxy, IMaterialProxy, "LinearRamp" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Uniform noise proxy
//-----------------------------------------------------------------------------
class CUniformNoiseProxy : public CResultProxy
{
public:
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );

private:
	CFloatInput	m_flMinVal;
	CFloatInput	m_flMaxVal;
};


bool CUniformNoiseProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	if (!m_flMinVal.Init( pMaterial, pKeyValues, "minVal", 0 ))
		return false;

	if (!m_flMaxVal.Init( pMaterial, pKeyValues, "maxVal", 1 ))
		return false;

	return true;
}

void CUniformNoiseProxy::OnBind( void *pC_BaseEntity )
{
	SetFloatResult( random->RandomFloat( m_flMinVal.GetFloat(), m_flMaxVal.GetFloat() ) );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}


EXPOSE_INTERFACE( CUniformNoiseProxy, IMaterialProxy, "UniformNoise" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Gaussian noise proxy
//-----------------------------------------------------------------------------
class CGaussianNoiseProxy : public CResultProxy
{
public:
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );

private:
	CFloatInput m_Mean;
	CFloatInput m_StdDev;
	CFloatInput	m_flMinVal;
	CFloatInput	m_flMaxVal;
};


bool CGaussianNoiseProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CResultProxy::Init( pMaterial, pKeyValues ))
		return false;

	if (!m_Mean.Init( pMaterial, pKeyValues, "mean", 0.0f ))
		return false;

	if (!m_StdDev.Init( pMaterial, pKeyValues, "halfwidth", 1.0f ))
		return false;

	if (!m_flMinVal.Init( pMaterial, pKeyValues, "minVal", -FLT_MAX ))
		return false;

	if (!m_flMaxVal.Init( pMaterial, pKeyValues, "maxVal", FLT_MAX ))
		return false;

	return true;
}

void CGaussianNoiseProxy::OnBind( void *pC_BaseEntity )
{
	float flMean = m_Mean.GetFloat();
	float flStdDev = m_StdDev.GetFloat();
	float flVal = randomgaussian->RandomFloat( flMean, flStdDev );
	float flMaxVal = m_flMaxVal.GetFloat();
	float flMinVal = m_flMinVal.GetFloat();

	if (flMinVal > flMaxVal)
	{
		float flTemp = flMinVal;
		flMinVal = flMaxVal;
		flMaxVal = flTemp;
	}

	// clamp
	if (flVal < flMinVal)
		flVal = flMinVal;
	else if ( flVal > flMaxVal )
		flVal = flMaxVal;

	SetFloatResult( flVal );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}


EXPOSE_INTERFACE( CGaussianNoiseProxy, IMaterialProxy, "GaussianNoise" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Exponential proxy
//-----------------------------------------------------------------------------
class CExponentialProxy : public CFunctionProxy
{
public:
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );

private:
	CFloatInput	m_Scale;
	CFloatInput	m_Offset;
	CFloatInput	m_flMinVal;
	CFloatInput	m_flMaxVal;
};


bool CExponentialProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CFunctionProxy::Init( pMaterial, pKeyValues ))
		return false;

	if (!m_Scale.Init( pMaterial, pKeyValues, "scale", 1.0f ))
		return false;

	if (!m_Offset.Init( pMaterial, pKeyValues, "offset", 0.0f ))
		return false;

	if (!m_flMinVal.Init( pMaterial, pKeyValues, "minVal", -FLT_MAX ))
		return false;

	if (!m_flMaxVal.Init( pMaterial, pKeyValues, "maxVal", FLT_MAX ))
		return false;

	return true;
}

void CExponentialProxy::OnBind( void *pC_BaseEntity )
{	
	float flVal = m_Scale.GetFloat() * exp(m_pSrc1->GetFloatValue( ) + m_Offset.GetFloat());

	float flMaxVal = m_flMaxVal.GetFloat();
	float flMinVal = m_flMinVal.GetFloat();

	if (flMinVal > flMaxVal)
	{
		float flTemp = flMinVal;
		flMinVal = flMaxVal;
		flMaxVal = flTemp;
	}

	// clamp
	if (flVal < flMinVal)
		flVal = flMinVal;
	else if ( flVal > flMaxVal )
		flVal = flMaxVal;

	SetFloatResult( flVal );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}


EXPOSE_INTERFACE( CExponentialProxy, IMaterialProxy, "Exponential" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Absolute value proxy
//-----------------------------------------------------------------------------
class CAbsProxy : public CFunctionProxy
{
public:
	virtual void OnBind( void *pC_BaseEntity );
};


void CAbsProxy::OnBind( void *pC_BaseEntity )
{	
	SetFloatResult( fabs(m_pSrc1->GetFloatValue( )) );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}


EXPOSE_INTERFACE( CAbsProxy, IMaterialProxy, "Abs" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Empty proxy-- used to comment out large proxy blocks
//-----------------------------------------------------------------------------
class CEmptyProxy : public IMaterialProxy
{
public:
	CEmptyProxy() {}
	virtual ~CEmptyProxy() {}
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues ) { return true; }
	virtual void OnBind( void *pC_BaseEntity ) {}
	virtual void Release( void ) { delete this; }
	virtual IMaterial *GetMaterial() { return NULL; }
};


EXPOSE_INTERFACE( CEmptyProxy, IMaterialProxy, "Empty" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Comparison proxy
//-----------------------------------------------------------------------------
class CLessOrEqualProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );

private:
	IMaterialVar *m_pLessVar;
	IMaterialVar *m_pGreaterVar;
};

bool CLessOrEqualProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	char const* pLessEqualVar = pKeyValues->GetString( "lessEqualVar" );
	if( !pLessEqualVar )
		return false;

	bool foundVar;
	m_pLessVar = pMaterial->FindVar( pLessEqualVar, &foundVar, true );
	if( !foundVar )
		return false;

	char const* pGreaterVar = pKeyValues->GetString( "greaterVar" );
	if( !pGreaterVar )
		return false;

	foundVar;
	m_pGreaterVar = pMaterial->FindVar( pGreaterVar, &foundVar, true );
	if( !foundVar )
		return false;

	// Compare 2 args..
	bool ok = CFunctionProxy::Init( pMaterial, pKeyValues );
	ok = ok && m_pSrc2;
	return ok;
}

void CLessOrEqualProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pSrc2 && m_pLessVar && m_pGreaterVar && m_pResult );

	IMaterialVar *pSourceVar;
	if (GetSrc1Float() <= GetSrc2Float())
	{
		pSourceVar = m_pLessVar;
	}
	else
	{
		pSourceVar = m_pGreaterVar;
	}

	int vecSize = 0;
	MaterialVarType_t resultType = m_pResult->GetType();
	if (resultType == MATERIAL_VAR_TYPE_VECTOR)
	{
		if (m_ResultVecComp >= 0)
			resultType = MATERIAL_VAR_TYPE_FLOAT;
		vecSize = m_pResult->VectorSize();
	}
	else if (resultType == MATERIAL_VAR_TYPE_UNDEFINED)
	{
		resultType = pSourceVar->GetType();
		if (resultType == MATERIAL_VAR_TYPE_VECTOR)
		{
			vecSize = pSourceVar->VectorSize();
		}
	}

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector src;
			pSourceVar->GetVecValue( src.Base(), vecSize ); 
			m_pResult->SetVecValue( src.Base(), vecSize );
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		SetFloatResult( pSourceVar->GetFloatValue() );
		break;

	case MATERIAL_VAR_TYPE_INT:
		m_pResult->SetFloatValue( pSourceVar->GetIntValue() );
		break;
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CLessOrEqualProxy, IMaterialProxy, "LessOrEqual" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// WrapMinMax proxy
//-----------------------------------------------------------------------------
class CWrapMinMaxProxy : public CFunctionProxy
{
public:
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );

private:
	CFloatInput	m_flMinVal;
	CFloatInput	m_flMaxVal;
};

bool CWrapMinMaxProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	if (!CFunctionProxy::Init( pMaterial, pKeyValues ))
		return false;

	if (!m_flMinVal.Init( pMaterial, pKeyValues, "minVal", 0 ))
		return false;

	if (!m_flMaxVal.Init( pMaterial, pKeyValues, "maxVal", 1 ))
		return false;

	return true;
}

void CWrapMinMaxProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pResult );

	if ( m_flMaxVal.GetFloat() <= m_flMinVal.GetFloat() ) // Bad input, just return the min
	{
		SetFloatResult( m_flMinVal.GetFloat() );
	}
	else
	{
		float flResult = ( GetSrc1Float() - m_flMinVal.GetFloat() ) / ( m_flMaxVal.GetFloat() - m_flMinVal.GetFloat() );

		if ( flResult >= 0.0f )
		{
			flResult -= ( float )( int )flResult;
		}
		else // Negative
		{
			flResult -= ( float )( ( ( int )flResult ) - 1 );
		}

		flResult *= ( m_flMaxVal.GetFloat() - m_flMinVal.GetFloat() );
		flResult += m_flMinVal.GetFloat();

		SetFloatResult( flResult );
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CWrapMinMaxProxy, IMaterialProxy, "WrapMinMax" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Selects the first var value if it's non-zero, otherwise goes with the second
//-----------------------------------------------------------------------------

class CSelectFirstIfNonZeroProxy : public CFunctionProxy
{
public:
	bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	void OnBind( void *pC_BaseEntity );
};

bool CSelectFirstIfNonZeroProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	// Requires 2 args..
	bool ok = CFunctionProxy::Init( pMaterial, pKeyValues );
	ok = ok && m_pSrc2;
	return ok;
}

void CSelectFirstIfNonZeroProxy::OnBind( void *pC_BaseEntity )
{
	Assert( m_pSrc1 && m_pSrc2 && m_pResult );

	MaterialVarType_t resultType;
	int vecSize;
	ComputeResultType( resultType, vecSize );

	switch( resultType )
	{
	case MATERIAL_VAR_TYPE_VECTOR:
		{
			Vector a, b;
			m_pSrc1->GetVecValue( a.Base(), vecSize ); 
			m_pSrc2->GetVecValue( b.Base(), vecSize ); 

			if ( !a.IsZero() )
			{
				m_pResult->SetVecValue( a.Base(), vecSize );
			}
			else
			{
				m_pResult->SetVecValue( b.Base(), vecSize );
			}
		}
		break;

	case MATERIAL_VAR_TYPE_FLOAT:
		if ( GetSrc1Float() )
		{
			SetFloatResult( GetSrc1Float() );
		}
		else
		{
			SetFloatResult( GetSrc2Float() );
		}
		break;

	case MATERIAL_VAR_TYPE_INT:
		if ( m_pSrc1->GetIntValue() )
		{
			m_pResult->SetFloatValue( m_pSrc1->GetIntValue() );
		}
		else
		{
			m_pResult->SetFloatValue( m_pSrc2->GetIntValue() );
		}
		break;
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CSelectFirstIfNonZeroProxy, IMaterialProxy, "SelectFirstIfNonZero" IMATERIAL_PROXY_INTERFACE_VERSION );