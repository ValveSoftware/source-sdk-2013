//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ENVLASER_H
#define ENVLASER_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "beam_shared.h"
#include "entityoutput.h"


class CSprite;


class CEnvLaser : public CBeam
{
	DECLARE_CLASS( CEnvLaser, CBeam );
public:
	void	Spawn( void );
	void	Precache( void );
	bool	KeyValue( const char *szKeyName, const char *szValue );

	void	TurnOn( void );
	void	TurnOff( void );
	int		IsOn( void );

	void	FireAtPoint( trace_t &point );
	void	StrikeThink( void );

	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	DECLARE_DATADESC();

	string_t m_iszLaserTarget;	// Name of entity or entities to strike at, randomly picked if more than one match.
	CSprite	*m_pSprite;
	string_t m_iszSpriteName;
	Vector  m_firePosition;

	float	m_flStartFrame;
};

#endif // ENVLASER_H
