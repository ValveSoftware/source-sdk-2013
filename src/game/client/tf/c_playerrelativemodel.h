//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef C_PLAYERRELATIVEMODEL_H
#define C_PLAYERRELATIVEMODEL_H
#ifdef _WIN32
#pragma once
#endif

#include "c_tf_player.h"

#define PRM_PERMANENT			-1

//
// Flags
#define PRM_SPIN_Z				(1<<0)

//-----------------------------------------------------------------------------
// Purpose: A clientside, visual only model that's positioned relative to players
//-----------------------------------------------------------------------------
class C_PlayerRelativeModel : public C_BaseAnimating
{
	DECLARE_CLASS( C_PlayerRelativeModel, C_BaseAnimating );
public:
	static C_PlayerRelativeModel *Create( const char *pszModelName, C_BaseEntity *pParent, Vector vecOffset, QAngle angleOffset, float flAnimSpeed, float flLifetime = 0.2, int iFlags = 0 );

	bool	Initialize( const char *pszModelName, C_BaseEntity *pParent, Vector vecOffset, QAngle angleOffset, float flAnimSpeed, float flLifetime, int iFlags );
	void	SetLifetime( float flLifetime );
	void	ClientThink( void );
	
protected:
	float	m_flExpiresAt;
	int		m_iFlags;
private:
	float	m_flRotateAt;
	float	m_flAnimateAt;
	float	m_flScale;

	Vector m_vecOffsetPos;
	QAngle m_angleOffset;

	QAngle m_qOffsetRotation;
	float  m_flAnimSpeed;
};

//-----------------------------------------------------------------------------
// Purpose: A clientside, visual only model that's positioned relative to players
//-----------------------------------------------------------------------------
class C_MerasmusBombEffect : public C_PlayerRelativeModel
{
	DECLARE_CLASS( C_MerasmusBombEffect, C_PlayerRelativeModel );
public:
	static C_MerasmusBombEffect *Create( const char *pszModelName, C_TFPlayer *pParent, Vector vecOffset, QAngle angleOffset, float flAnimSpeed, float flLifetime = 0.2, int iFlags = 0 );

	bool	Initialize( const char *pszModelName, C_TFPlayer *pParent, Vector vecOffset,  QAngle angleOffset, float flAnimSpeed, float flLifetime, int iFlags );
	void	ClientThink( void );
private:
	CNewParticleEffect			   *m_pBombonomiconBeam;
	CNewParticleEffect			   *m_pBombonomiconEffect;
};
#endif // C_PLAYERRELATIVEMODEL_H
