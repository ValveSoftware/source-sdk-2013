//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: These are a couple of base proxy classes to help us with
// getting/setting source/result material vars
//
// $NoKeywords: $
//=============================================================================//

#ifndef FUNCTIONPROXY_H
#define FUNCTIONPROXY_H

#include "materialsystem/imaterialproxy.h"
#include "materialsystem/imaterialvar.h"

class IMaterialVar;
class C_BaseEntity;


//-----------------------------------------------------------------------------
// Helper class to deal with floating point inputs
//-----------------------------------------------------------------------------
class CFloatInput
{
public:
	bool  Init( IMaterial *pMaterial, KeyValues *pKeyValues, const char *pKeyName, float flDefault = 0.0f );
	float GetFloat() const;

private:
	float m_flValue;
	IMaterialVar *m_pFloatVar;
	int	m_FloatVecComp;
};


//-----------------------------------------------------------------------------
// Result proxy; a result (with vector friendliness)
//-----------------------------------------------------------------------------
class CResultProxy : public IMaterialProxy
{
public:
	CResultProxy();
	virtual ~CResultProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void Release( void ) { delete this; }
	virtual IMaterial *GetMaterial();

protected:
	C_BaseEntity *BindArgToEntity( void *pArg );
	void SetFloatResult( float result );

	IMaterialVar* m_pResult;
	int m_ResultVecComp;
};


//-----------------------------------------------------------------------------
// Base functional proxy; two sources (one is optional) and a result
//-----------------------------------------------------------------------------
class CFunctionProxy : public CResultProxy
{
public:
	CFunctionProxy();
	virtual ~CFunctionProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );

protected:
	void ComputeResultType( MaterialVarType_t& resultType, int& vecSize );

	IMaterialVar* m_pSrc1;
	IMaterialVar* m_pSrc2;
};

#endif // FUNCTIONPROXY_H

