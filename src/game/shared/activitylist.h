//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ACTIVITYLIST_H
#define ACTIVITYLIST_H
#ifdef _WIN32
#pragma once
#endif

#include <KeyValues.h>

typedef struct activityentry_s activityentry_t;

class CActivityRemap
{
public:

	CActivityRemap()
	{
		pExtraBlock = NULL;
	}

	void SetExtraKeyValueBlock ( KeyValues *pKVBlock )
	{
		pExtraBlock = pKVBlock;
	}

	KeyValues *GetExtraKeyValueBlock ( void ) { return pExtraBlock; }

	Activity 		activity;
	Activity		mappedActivity;

private:

	KeyValues		*pExtraBlock;
};


class CActivityRemapCache
{
public:

	CActivityRemapCache()
	{
	}

	CActivityRemapCache( const CActivityRemapCache& src )
	{
		int c = src.m_cachedActivityRemaps.Count();
		for ( int i = 0; i < c; i++ )
		{
			m_cachedActivityRemaps.AddToTail( src.m_cachedActivityRemaps[ i ] );
		}
	}

	CActivityRemapCache& operator = ( const CActivityRemapCache& src )
	{
		if ( this == &src )
			return *this;

		int c = src.m_cachedActivityRemaps.Count();
		for ( int i = 0; i < c; i++ )
		{
			m_cachedActivityRemaps.AddToTail( src.m_cachedActivityRemaps[ i ] );
		}

		return *this;
	}

	CUtlVector< CActivityRemap > m_cachedActivityRemaps;
};

void UTIL_LoadActivityRemapFile( const char *filename, const char *section, CUtlVector <CActivityRemap> &entries );

//=========================================================
//=========================================================
extern void ActivityList_Init( void );
extern void ActivityList_Free( void );
extern bool ActivityList_RegisterSharedActivity( const char *pszActivityName, int iActivityIndex );
extern Activity ActivityList_RegisterPrivateActivity( const char *pszActivityName );
extern int ActivityList_IndexForName( const char *pszActivityName );
extern const char *ActivityList_NameForIndex( int iActivityIndex );
extern int ActivityList_HighestIndex();

// This macro guarantees that the names of each activity and the constant used to
// reference it in the code are identical.
#define REGISTER_SHARED_ACTIVITY( _n ) ActivityList_RegisterSharedActivity(#_n, _n);
#define REGISTER_PRIVATE_ACTIVITY( _n ) _n = ActivityList_RegisterPrivateActivity( #_n );

// Implemented in shared code
extern void ActivityList_RegisterSharedActivities( void );

class ISaveRestoreOps;
extern ISaveRestoreOps* ActivityDataOps();

#endif // ACTIVITYLIST_H
