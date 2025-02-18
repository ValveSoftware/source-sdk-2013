//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CLIENT_THINKLIST_H
#define CLIENT_THINKLIST_H
#ifdef _WIN32
#pragma once
#endif


#include "igamesystem.h"
#include "utllinkedlist.h"
#include "cliententitylist.h"
#include "iclientthinkable.h"
#include "utlrbtree.h"


#define CLIENT_THINK_ALWAYS	-1293
#define CLIENT_THINK_NEVER	-1


#define INVALID_THINK_HANDLE ClientThinkList()->GetInvalidThinkHandle()


class CClientThinkList : public IGameSystemPerFrame
{
public:

							CClientThinkList();
	virtual					~CClientThinkList();
	
	virtual char const		*Name() { return "CClientThinkList"; }
	virtual bool			IsPerFrame() { return true; }

	// Set the next time at which you want to think. You can also use
	// one of the CLIENT_THINK_ defines.
	void					SetNextClientThink( ClientEntityHandle_t hEnt, float nextTime );
	
	// Remove an entity from the think list.
	void					RemoveThinkable( ClientEntityHandle_t hEnt );

	// Use to initialize your think handles in IClientThinkables.
	ClientThinkHandle_t		GetInvalidThinkHandle();

	// This is called after network updating and before rendering.
	void					PerformThinkFunctions();

	// Call this to destroy a thinkable object - deletes the object post think.
	void					AddToDeleteList( ClientEntityHandle_t hEnt );	
	void					RemoveFromDeleteList( ClientEntityHandle_t hEnt );

// IClientSystem implementation.
public:

	virtual bool Init();
	virtual void PostInit() {};
	virtual void Shutdown();
	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity() {}
	virtual void LevelShutdownPreEntity();
	virtual void LevelShutdownPostEntity();
	virtual void PreRender();
	virtual void PostRender() { }
	virtual void Update( float frametime );
	virtual void OnSave() {}
	virtual void OnRestore() {}
	virtual void SafeRemoveIfDesired() {}

private:
	struct ThinkEntry_t
	{
		ClientEntityHandle_t	m_hEnt;
		float					m_flNextClientThink;
		float					m_flLastClientThink;
		int						m_nIterEnum;
	};

	struct ThinkListChanges_t
	{
		ClientEntityHandle_t	m_hEnt;
		ClientThinkHandle_t		m_hThink;
		float					m_flNextTime;
	};

// Internal stuff.
private:
	void			SetNextClientThink( ClientThinkHandle_t hThink, float nextTime );
	void			RemoveThinkable( ClientThinkHandle_t hThink );
	void			PerformThinkFunction( ThinkEntry_t *pEntry, float curtime );
	ThinkEntry_t*	GetThinkEntry( ClientThinkHandle_t hThink );
	void			CleanUpDeleteList();

	// Add entity to frame think list
	void			AddEntityToFrameThinkList( ThinkEntry_t *pEntry, bool bAlwaysChain, int &nCount, ThinkEntry_t **ppFrameThinkList );

private:
	CUtlLinkedList<ThinkEntry_t, unsigned short>	m_ThinkEntries;

	CUtlVector<ClientEntityHandle_t>	m_aDeleteList;
	CUtlVector<ThinkListChanges_t>		m_aChangeList;

	// Makes sure the entries are thinked once per frame in the face of hierarchy
	int m_nIterEnum;
	bool m_bInThinkLoop;
};


// -------------------------------------------------------------------------------- //
// Inlines.
// -------------------------------------------------------------------------------- //

inline ClientThinkHandle_t CClientThinkList::GetInvalidThinkHandle()
{
	return (ClientThinkHandle_t)(uintp)m_ThinkEntries.InvalidIndex();
}


inline CClientThinkList::ThinkEntry_t* CClientThinkList::GetThinkEntry( ClientThinkHandle_t hThink )
{
	return &m_ThinkEntries[ (unsigned long)hThink ];
}


inline CClientThinkList* ClientThinkList()
{
	extern CClientThinkList g_ClientThinkList;
	return &g_ClientThinkList;
}


#endif // CLIENT_THINKLIST_H
