//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SCRIPTEDTARGET_H
#define SCRIPTEDTARGET_H
#ifdef _WIN32
#pragma once
#endif

#ifndef SCRIPTEVENT_H
#include "scriptevent.h"
#endif

#include "ai_basenpc.h"

class CScriptedTarget : public CAI_BaseNPC
{
	DECLARE_CLASS( CScriptedTarget, CAI_BaseNPC );
public:
	DECLARE_DATADESC();

	void				Spawn( void );
	virtual int			ObjectCaps( void ) { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }


	void				ScriptThink( void );
	CBaseEntity*		FindEntity( void );

	void TurnOn(void);
	void TurnOff(void);

	// Input handlers
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	CScriptedTarget*	NextScriptedTarget(void);
	float				MoveSpeed(void)			{ return m_nMoveSpeed; };
	float				EffectDuration(void)	{ return m_flEffectDuration; };

	int					DrawDebugTextOverlays(void);
	void				DrawDebugGeometryOverlays(void);
	float				PercentComplete(void);

	Vector				m_vLastPosition;	// Last position that's been reached
	
private:
	int					m_iDisabled;		// Initial state
	string_t			m_iszEntity;		// entity that is wanted for this script
	float				m_flRadius;			// range to search

	int					m_nMoveSpeed;		// How fast do I burn from target to target
	float				m_flPauseDuration;	// How long to pause at this target
	float				m_flPauseDoneTime;	// When is pause over
	float				m_flEffectDuration;	// How long should any associated effect last?

	COutputEvent		m_AtTarget;			// Fired when scripted target has been reached
	COutputEvent		m_LeaveTarget;		// Fired when scripted target is left
};

#endif // SCRIPTEDTARGET_H
