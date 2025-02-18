//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ====
//
// Purpose:
//
//=============================================================================

#ifndef LOGIC_EVENTLISTENER_H
#define LOGIC_EVENTLISTENER_H
#pragma once

#include "GameEventListener.h"

//-----------------------------------------------------------------------------
// Purpose: Used to relay outputs/inputs from the events to the world and vice versa
//-----------------------------------------------------------------------------
class CLogicEventListener : public CLogicalEntity, public CGameEventListener
{
	DECLARE_CLASS( CLogicEventListener, CLogicalEntity );
	DECLARE_DATADESC();

public:
	// FIXME: Subclass
#ifdef PORTAL2
#endif // PORTAL2

	virtual void Spawn( void );
	virtual void FireGameEvent( IGameEvent *event );
	
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );

private:

	string_t	m_iszEventName;
	bool		m_bIsEnabled;
	int			m_nTeam;
	bool		m_bFetchEventData;

	COutputEvent m_OnEventFired;

};

//-----------------------------------------------------------------------------
// Purpose: Used to relay outputs/inputs from the events to the world and vice versa
//-----------------------------------------------------------------------------
class CLogicEventListenerItemEquip : public CLogicEventListener
{
	DECLARE_CLASS( CLogicEventListenerItemEquip, CLogicalEntity );
	DECLARE_DATADESC();

public:
	virtual void Spawn( void );
	virtual void FireGameEvent( IGameEvent *event );

	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );

private:
	bool		m_bIsEnabled;
	int			m_nTeam;
	string_t	m_szWeaponClassname;
	int			m_nWeaponType;

	COutputEvent m_OnEventFired;
};
#endif	// LOGIC_EVENTLISTENER_H
