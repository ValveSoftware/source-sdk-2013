//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "clientsideeffects.h"

#if !defined( FXSTATICLINE_H )
#define FXSTATICLINE_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"

class IMaterial;

#define FXSTATICLINE_FLIP_HORIZONTAL	0x00000001	//Swaps the TC's so the texture is flipped horizontally
#define FXSTATICLINE_FLIP_VERTICAL		0x00000002	//Swaps the TC's so the texture is flipped vertically

class CFXStaticLine : public CClientSideEffect
{
public:

	CFXStaticLine( const char *name, const Vector& start, const Vector& end, float scale, float life, const char *shader, unsigned int flags );
	~CFXStaticLine( void );

	virtual void	Draw( double frametime );
	virtual bool	IsActive( void );
	virtual void	Destroy( void );
	virtual	void	Update( double frametime );

protected:

	IMaterial		*m_pMaterial;
	Vector			m_vecStart, m_vecEnd;
	unsigned int	m_uiFlags;
	float			m_fLife;
	float			m_fScale;
};

#endif
