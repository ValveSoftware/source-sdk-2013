//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef FUNC_BULLETSHIELD_H
#define FUNC_BULLETSHIELD_H
#ifdef _WIN32
#pragma once
#endif

//!! replace this with generic start enabled/disabled
#define SF_WALL_START_OFF		0x0001
#define SF_IGNORE_PLAYERUSE		0x0002

#include "modelentities.h"

//-----------------------------------------------------------------------------
// Purpose: shield that stops bullets, but not other objects
// enabled state:	brush is visible
// disabled staute:	brush not visible
//-----------------------------------------------------------------------------
class CFuncBulletShield : public CFuncBrush
{
public:
	DECLARE_CLASS( CFuncBulletShield, CFuncBrush );
	DECLARE_DATADESC();

	virtual void Spawn( void );
	
	bool TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );
	/*
	bool CreateVPhysics( void );

	virtual int	ObjectCaps( void ) { return HasSpawnFlags(SF_IGNORE_PLAYERUSE) ? BaseClass::ObjectCaps() : BaseClass::ObjectCaps() | FCAP_IMPULSE_USE; }

	virtual int DrawDebugTextOverlays( void );

	void TurnOff( void );
	void TurnOn( void );

	// Input handlers
	void InputTurnOff( inputdata_t &inputdata );
	void InputTurnOn( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	enum BrushSolidities_e {
		BRUSHSOLID_TOGGLE = 0,
		BRUSHSOLID_NEVER  = 1,
		BRUSHSOLID_ALWAYS = 2,
	};

	BrushSolidities_e m_iSolidity;
	int m_iDisabled;
	bool m_bSolidBsp;
	string_t m_iszExcludedClass;
	bool m_bInvertExclusion;

	DECLARE_DATADESC();

	virtual bool IsOn( void );
	*/
};


#endif // MODELENTITIES_H
