//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_NAMESPACES_H
#define AI_NAMESPACES_H

class CStringRegistry;

#if defined( _WIN32 )
#pragma once
#endif

#define MAX_STRING_INDEX 9999
const int GLOBAL_IDS_BASE = 1000000000; // decimal for debugging readability

//-----------------------------------------------------------------------------

inline bool AI_IdIsGlobal( int id )			{ return ( id >= GLOBAL_IDS_BASE || id == -1 ); }
inline bool AI_IdIsLocal( int id )			{ return ( id < GLOBAL_IDS_BASE || id == -1 );  }
inline int  AI_RemapToGlobal( int id )		{ return ( id != -1 ) ? id + GLOBAL_IDS_BASE : -1; }
inline int  AI_RemapFromGlobal( int id )	{ return ( id != -1 ) ? id - GLOBAL_IDS_BASE : -1; }

inline int	AI_MakeGlobal( int id )			{ return AI_IdIsLocal( id ) ? AI_RemapToGlobal( id ) : id; }

//-----------------------------------------------------------------------------
// CAI_GlobalNamespace
//
// Purpose: Symbol table for all symbols across a given namespace, a
//			bi-directional mapping of "text" to global ID
//

class CAI_GlobalNamespace
{
public:
	CAI_GlobalNamespace();
	~CAI_GlobalNamespace();

	void Clear();

	void AddSymbol( const char *pszSymbol, int symbolID );
	int NextGlobalBase() const;

	const char *IdToSymbol( int symbolID ) const;
	int SymbolToId( const char *pszSymbol ) const;

private:
	CStringRegistry * 	m_pSymbols;
	int					m_NextGlobalBase;
};

//-----------------------------------------------------------------------------
// CAI_LocalIdSpace
//
// Purpose: Maps per class IDs to global IDs, so that various classes can use
//			the same integer in local space to represent different globally
//			unique integers. Used for schedules, tasks, conditions and squads
//

class CAI_LocalIdSpace
{
public:
	CAI_LocalIdSpace( bool fIsRoot = false );

	bool Init( CAI_GlobalNamespace *pGlobalNamespace, CAI_LocalIdSpace *pParentIDSpace = NULL );
	bool IsGlobalBaseSet() const { return ( m_globalBase != -1 ); }

	bool AddSymbol( const char *pszSymbol, int localId, const char *pszDebugSymbolType = "", const char *pszDebugOwner = "" );

	int GlobalToLocal( int globalID ) const;
	int LocalToGlobal( int localID ) const;

	CAI_GlobalNamespace *GetGlobalNamespace() { return m_pGlobalNamespace; }
	const CAI_GlobalNamespace *GetGlobalNamespace() const { return m_pGlobalNamespace; }

private:
	bool IsLocalBaseSet() const	{ return ( m_localBase != MAX_STRING_INDEX );	}
	int GetLocalBase() const	{ return m_localBase;  }
	int GetGlobalBase() const	{ return m_globalBase; }
	int GetLocalTop() const		{ return m_localTop;  }
	int GetGlobalTop() const	{ return m_globalTop; }

	bool SetLocalBase( int newBase );

	// --------------------------------

	int 					m_globalBase;
	int 					m_localBase;
	int 					m_localTop;
	int 					m_globalTop;

	CAI_LocalIdSpace *		m_pParentIDSpace;
	CAI_GlobalNamespace *	m_pGlobalNamespace;
};

//-----------------------------------------------------------------------------
//
// Namespaces used by CAI_BaseNPC
//
//-----------------------------------------------------------------------------

class CAI_GlobalScheduleNamespace
{
public:
	void Clear()
	{
		m_ScheduleNamespace.Clear();
		m_TaskNamespace.Clear();
		m_ConditionNamespace.Clear();
	}

	void 		AddSchedule( const char *pszSchedule, int scheduleID );
	const char *ScheduleIdToSymbol( int scheduleID ) const;
	int 		ScheduleSymbolToId( const char *pszSchedule ) const;

	void 		AddTask( const char *pszTask, int taskID );
	const char *TaskIdToSymbol( int taskID ) const;
	int 		TaskSymbolToId( const char *pszTask ) const;

	void 		AddCondition( const char *pszCondition, int conditionID );
	const char *ConditionIdToSymbol( int conditionID ) const;
	int 		ConditionSymbolToId( const char *pszCondition ) const;
	int			NumConditions() const;

private:
	friend class CAI_ClassScheduleIdSpace;

	CAI_GlobalNamespace m_ScheduleNamespace;
	CAI_GlobalNamespace m_TaskNamespace;
	CAI_GlobalNamespace m_ConditionNamespace;
};

//-------------------------------------

class CAI_ClassScheduleIdSpace
{
public:
	CAI_ClassScheduleIdSpace( bool fIsRoot = false )
	 :	m_ScheduleIds( fIsRoot ),
	 	m_TaskIds( fIsRoot ),
	 	m_ConditionIds( fIsRoot )
	{
	}

	bool Init( const char *pszClassName, CAI_GlobalScheduleNamespace *pGlobalNamespace, CAI_ClassScheduleIdSpace *pParentIDSpace = NULL );

	const char *GetClassName() const { return m_pszClassName; }

	bool IsGlobalBaseSet() const;

	bool AddSchedule( const char *pszSymbol, int localId, const char *pszDebugOwner = "" );
	int ScheduleGlobalToLocal( int globalID ) const;
	int ScheduleLocalToGlobal( int localID ) const;

	bool AddTask( const char *pszSymbol, int localId, const char *pszDebugOwner = "" );
	int TaskGlobalToLocal( int globalID ) const;
	int TaskLocalToGlobal( int localID ) const;

	bool AddCondition( const char *pszSymbol, int localId, const char *pszDebugOwner = "" );
	int ConditionGlobalToLocal( int globalID ) const;
	int ConditionLocalToGlobal( int localID ) const;

private:
	const char *	 m_pszClassName;
	CAI_LocalIdSpace m_ScheduleIds;
	CAI_LocalIdSpace m_TaskIds;
	CAI_LocalIdSpace m_ConditionIds;
};

//-----------------------------------------------------------------------------

inline void CAI_GlobalScheduleNamespace::AddSchedule( const char *pszSchedule, int scheduleID )
{
	m_ScheduleNamespace.AddSymbol( pszSchedule, scheduleID);
}

inline const char *CAI_GlobalScheduleNamespace::ScheduleIdToSymbol( int scheduleID ) const
{
	return m_ScheduleNamespace.IdToSymbol( scheduleID );
}

inline int CAI_GlobalScheduleNamespace::ScheduleSymbolToId( const char *pszSchedule ) const
{
	return m_ScheduleNamespace.SymbolToId( pszSchedule );
}

inline void CAI_GlobalScheduleNamespace::AddTask( const char *pszTask, int taskID )
{
	m_TaskNamespace.AddSymbol( pszTask, taskID);
}

inline const char *CAI_GlobalScheduleNamespace::TaskIdToSymbol( int taskID ) const
{
	return m_TaskNamespace.IdToSymbol( taskID );
}

inline int CAI_GlobalScheduleNamespace::TaskSymbolToId( const char *pszTask ) const
{
	return m_TaskNamespace.SymbolToId( pszTask );
}

inline void CAI_GlobalScheduleNamespace::AddCondition( const char *pszCondition, int conditionID )
{
	m_ConditionNamespace.AddSymbol( pszCondition, conditionID);
}

inline const char *CAI_GlobalScheduleNamespace::ConditionIdToSymbol( int conditionID ) const
{
	return m_ConditionNamespace.IdToSymbol( conditionID );
}

inline int CAI_GlobalScheduleNamespace::ConditionSymbolToId( const char *pszCondition ) const
{
	return m_ConditionNamespace.SymbolToId( pszCondition );
}

inline int CAI_GlobalScheduleNamespace::NumConditions() const
{ 
	return m_ConditionNamespace.NextGlobalBase() - GLOBAL_IDS_BASE; 
}

inline bool CAI_ClassScheduleIdSpace::Init( const char *pszClassName, CAI_GlobalScheduleNamespace *pGlobalNamespace, CAI_ClassScheduleIdSpace *pParentIDSpace )
{
	m_pszClassName = pszClassName;
	return ( m_ScheduleIds.Init( &pGlobalNamespace->m_ScheduleNamespace, ( pParentIDSpace ) ? &pParentIDSpace->m_ScheduleIds : NULL ) &&
			 m_TaskIds.Init( &pGlobalNamespace->m_TaskNamespace, ( pParentIDSpace ) ? &pParentIDSpace->m_TaskIds : NULL ) &&
			 m_ConditionIds.Init( &pGlobalNamespace->m_ConditionNamespace, ( pParentIDSpace ) ? &pParentIDSpace->m_ConditionIds : NULL ) );
}

//-----------------------------------------------------------------------------

inline bool CAI_ClassScheduleIdSpace::IsGlobalBaseSet() const
{
	return m_ScheduleIds.IsGlobalBaseSet();
}

inline bool CAI_ClassScheduleIdSpace::AddSchedule( const char *pszSymbol, int localId, const char *pszDebugOwner )
{
	return m_ScheduleIds.AddSymbol( pszSymbol, localId, "schedule", pszDebugOwner );
}

inline int CAI_ClassScheduleIdSpace::ScheduleGlobalToLocal( int globalID ) const
{
	return m_ScheduleIds.GlobalToLocal( globalID );
}

inline int CAI_ClassScheduleIdSpace::ScheduleLocalToGlobal( int localID ) const
{
	return m_ScheduleIds.LocalToGlobal( localID );
}

inline bool CAI_ClassScheduleIdSpace::AddTask( const char *pszSymbol, int localId, const char *pszDebugOwner )
{
	return m_TaskIds.AddSymbol( pszSymbol, localId, "task", pszDebugOwner );
}

inline int CAI_ClassScheduleIdSpace::TaskGlobalToLocal( int globalID ) const
{
	return m_TaskIds.GlobalToLocal( globalID );
}

inline int CAI_ClassScheduleIdSpace::TaskLocalToGlobal( int localID ) const
{
	return m_TaskIds.LocalToGlobal( localID );
}

inline bool CAI_ClassScheduleIdSpace::AddCondition( const char *pszSymbol, int localId, const char *pszDebugOwner )
{
	return m_ConditionIds.AddSymbol( pszSymbol, localId, "condition", pszDebugOwner );
}

inline int CAI_ClassScheduleIdSpace::ConditionGlobalToLocal( int globalID ) const
{
	return m_ConditionIds.GlobalToLocal( globalID );
}

inline int CAI_ClassScheduleIdSpace::ConditionLocalToGlobal( int localID ) const
{
	return m_ConditionIds.LocalToGlobal( localID );
}

//=============================================================================

#endif // AI_NAMESPACES_H
