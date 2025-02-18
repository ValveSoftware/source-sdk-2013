//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#if !defined( FXLINE_H )
#define FXLINE_H
#ifdef _WIN32
#pragma once
#endif

struct FXLineData_t
{
	Vector		m_vecStart;
	Vector		m_vecEnd;
	Vector		m_vecStartVelocity;
	Vector		m_vecEndVelocity;
	float		m_flStartAlpha;
	float		m_flEndAlpha;
	float		m_flStartScale;
	float		m_flEndScale;
	float		m_flDieTime;
	float		m_flLifeTime;
	
	IMaterial	*m_pMaterial;
};

#include "fx_staticline.h"

class CFXLine : public CClientSideEffect
{
public:

	CFXLine( const char *name, const FXLineData_t &data );
	
	~CFXLine( void );

	virtual void	Draw( double frametime );
	virtual bool	IsActive( void );
	virtual void	Destroy( void );
	virtual	void	Update( double frametime );

protected:

	FXLineData_t	m_FXData;
};

void FX_DrawLine( const Vector &start, const Vector &end, float scale, IMaterial *pMaterial, const color32 &color );
void FX_DrawLineFade( const Vector &start, const Vector &end, float scale, IMaterial *pMaterial, const color32 &color, float fadeDist );

#endif	//FXLINE_H
