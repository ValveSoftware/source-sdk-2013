//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef TIMEDEVENTMGR_H
#define TIMEDEVENTMGR_H
#ifdef _WIN32
#pragma once
#endif


#include "utlpriorityqueue.h"


//
//
// These classes provide fast timed event callbacks. To use them, make a CTimedEventMgr
// and put CEventRegister objects in your objects that want the timed events.
//
//


class CTimedEventMgr;


abstract_class IEventRegisterCallback
{
public:
	virtual void FireEvent() = 0;
};


class CEventRegister
{
friend bool TimedEventMgr_LessFunc( CEventRegister* const &a, CEventRegister* const &b );
friend class CTimedEventMgr;

public:
	CEventRegister();
	~CEventRegister();

	// Call this before ever calling SetUpdateInterval().
	void Init( CTimedEventMgr *pMgr, IEventRegisterCallback *pCallback );
	
	// Use these to start and stop getting updates.
	void SetUpdateInterval( float interval );
	void StopUpdates();

	inline bool IsRegistered() const { return m_bRegistered; }

private:

	void Reregister();	// After having an event processed, this is called to have it register for the next one.
	void Term();


private:
	
	CTimedEventMgr *m_pEventMgr;
	float m_flNextEventTime;
	float m_flUpdateInterval;
	IEventRegisterCallback *m_pCallback;
	bool m_bRegistered;
};


class CTimedEventMgr
{
friend class CEventRegister;

public:
	CTimedEventMgr();

	// Call this each frame to fire events.
	void FireEvents();


private:

	// Things used by CEventRegister.
	void RegisterForNextEvent( CEventRegister *pEvent );
	void RemoveEvent( CEventRegister *pEvent );	
	
private:	
	
	// Events, sorted by the time at which they will fire.
	CUtlPriorityQueue<CEventRegister*> m_Events;
};


#endif // TIMEDEVENTMGR_H
