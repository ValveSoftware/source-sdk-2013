//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A point entity that periodically emits sparks and "bzzt" sounds.
//
// $NoKeywords: $
//=============================================================================//

#ifndef ENVSPARK_H
#define ENVSPARK_H
#ifdef _WIN32
#pragma once
#endif

class CEnvSpark : public CPointEntity
{
	DECLARE_CLASS( CEnvSpark, CPointEntity );

public:
	CEnvSpark( void );

	void	Spawn( void );
	void	Precache( void );
	void	SparkThink( void );

	void	StartSpark( void );
	void	StopSpark( void );

	// Input handlers
	void InputStartSpark( inputdata_t &inputdata );
	void InputStopSpark( inputdata_t &inputdata );
	void InputToggleSpark( inputdata_t &inputdata );
	void InputSparkOnce( inputdata_t &inputdata );

	bool IsSparking( void ){ return ( GetNextThink() != TICK_NEVER_THINK ); }
	
	DECLARE_DATADESC();

	float			m_flDelay;
	int				m_nGlowSpriteIndex;
	int				m_nMagnitude;
	int				m_nTrailLength;

	COutputEvent	m_OnSpark;
};

#endif // ENVSPARK_H
