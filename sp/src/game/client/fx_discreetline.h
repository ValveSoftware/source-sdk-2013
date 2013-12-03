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

#if !defined( FXDISCREETLINE_H )
#define FXDISCREETLINE_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"
#include "clientsideeffects.h"

class IMaterial;

class CFXDiscreetLine : public CClientSideEffect
{
public:

	CFXDiscreetLine ( const char *name, const Vector& start, const Vector& direction, float velocity, 
		float length, float clipLength, float scale, float life, const char *shader );
	~CFXDiscreetLine ( void );

	virtual void	Draw( double frametime );
	virtual bool	IsActive( void );
	virtual void	Destroy( void );
	virtual	void	Update( double frametime );

protected:

	IMaterial		*m_pMaterial;
	float			m_fLife;
	Vector			m_vecOrigin, m_vecDirection;
	float			m_fVelocity;
	float			m_fStartTime;
	float			m_fClipLength;
	float			m_fScale;
	float			m_fLength;
};

#endif	//FXDISCREETLINE_H