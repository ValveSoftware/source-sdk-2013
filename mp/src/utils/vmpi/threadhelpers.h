//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef THREADHELPERS_H
#define THREADHELPERS_H
#ifdef _WIN32
#pragma once
#endif


#include "tier1/utllinkedlist.h"


#define SIZEOF_CS	24	// sizeof( CRITICAL_SECTION )


class CCriticalSection
{
public:
			CCriticalSection();
			~CCriticalSection();


protected:

	friend class CCriticalSectionLock;
	
	void	Lock();
	void	Unlock();


public:
	char	m_CS[SIZEOF_CS];

	// Used to protect against deadlock in debug mode.
//#if defined( _DEBUG )
	CUtlLinkedList<unsigned long,int>	m_Locks;
	char								m_DeadlockProtect[SIZEOF_CS];
//#endif
};


// Use this to lock a critical section.
class CCriticalSectionLock
{
public:
			CCriticalSectionLock( CCriticalSection *pCS );
			~CCriticalSectionLock();
	void	Lock();
	void	Unlock();

private:
	CCriticalSection	*m_pCS;
	bool				m_bLocked;
};


template< class T >
class CCriticalSectionData : private CCriticalSection
{
public:
	// You only have access to the data between Lock() and Unlock().
	T*		Lock()
	{
		CCriticalSection::Lock();
		return &m_Data;
	}
	
	void	Unlock()
	{
		CCriticalSection::Unlock();
	}

private:
	T m_Data;
};



// ------------------------------------------------------------------------------------------------ //
// CEvent.
// ------------------------------------------------------------------------------------------------ //
class CEvent
{
public:
	CEvent();
	~CEvent();

	bool Init( bool bManualReset, bool bInitialState );
	void Term();
	
	void* GetEventHandle() const;

	// Signal the event.
	bool SetEvent();

	// Unset the event's signalled status.
	bool ResetEvent();


private:
	void *m_hEvent;
};


#endif // THREADHELPERS_H
