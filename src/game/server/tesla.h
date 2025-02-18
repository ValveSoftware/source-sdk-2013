//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef TESLA_H
#define TESLA_H
#ifdef _WIN32
#pragma once
#endif


#include "baseentity.h"


class CTesla : public CBaseEntity
{
public:
	DECLARE_CLASS( CTesla, CBaseEntity );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CTesla();

	virtual void Spawn();
	virtual void Activate();
	virtual void Precache();

	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	void InputDoSpark( inputdata_t &inputdata );

	void DoSpark();
	void ShootArcThink();
	
	void SetupForNextArc();
	CBaseEntity* GetSourceEntity();


public:
	
	// Tesla parameters.
	string_t m_SourceEntityName;	// Which entity the arcs come from.
	CNetworkVar( string_t, m_SoundName );			// What sound to play when arcing.

	color32 m_Color;
	int m_NumBeams[2];		// Number of beams per spark.
	
	float m_flRadius;		// Radius it looks for surfaces to arc to.
	
	float m_flThickness[2];		// Beam thickness.
	float m_flTimeVisible[2];	// How long each beam stays around (min/max).
	float m_flArcInterval[2];	// Time between args (min/max).

	bool m_bOn;

	CNetworkVar( string_t, m_iszSpriteName );
};


#endif // TESLA_H
