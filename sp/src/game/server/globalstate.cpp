//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "basetypes.h"
#include "saverestore.h"
#include "saverestore_utlvector.h"
#include "saverestore_utlsymbol.h"
#include "globalstate.h"
#include "igamesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

struct globalentity_t
{
	DECLARE_SIMPLE_DATADESC();

	CUtlSymbol	name;
	CUtlSymbol	levelName;
	GLOBALESTATE	state;
	int				counter;
};


class CGlobalState : public CAutoGameSystem
{
public:
	CGlobalState( char const *name ) : CAutoGameSystem( name ), m_disableStateUpdates(false)
	{
	}

	// IGameSystem
	virtual void LevelShutdownPreEntity() 
	{
		// don't allow state updates during shutdowns
		Assert( !m_disableStateUpdates );
		m_disableStateUpdates = true;
	}
	
	virtual void LevelShutdownPostEntity() 
	{
		Assert( m_disableStateUpdates );
		m_disableStateUpdates = false;
	}

	void EnableStateUpdates( bool bEnable )
	{
		m_disableStateUpdates = !bEnable;
	}

	void SetState( int globalIndex, GLOBALESTATE state )
	{
		if ( m_disableStateUpdates || !m_list.IsValidIndex(globalIndex) )
			return;
		m_list[globalIndex].state = state;
	}

	GLOBALESTATE GetState( int globalIndex )
	{
		if ( !m_list.IsValidIndex(globalIndex) )
			return GLOBAL_OFF;
		return m_list[globalIndex].state;
	}

	void SetCounter( int globalIndex, int counter )
	{
		if ( m_disableStateUpdates || !m_list.IsValidIndex(globalIndex) )
			return;
		m_list[globalIndex].counter = counter;
	}

	int AddToCounter( int globalIndex, int delta )
	{
		if ( m_disableStateUpdates || !m_list.IsValidIndex(globalIndex) )
			return 0;
		return ( m_list[globalIndex].counter += delta );
	}

	int GetCounter( int globalIndex )
	{
		if ( !m_list.IsValidIndex(globalIndex) )
			return 0;
		return m_list[globalIndex].counter;
	}

	void SetMap( int globalIndex, string_t mapname )
	{
		if ( !m_list.IsValidIndex(globalIndex) )
			return;
		m_list[globalIndex].levelName = m_nameList.AddString( STRING(mapname) );
	}

	const char *GetMap( int globalIndex )
	{
		if ( !m_list.IsValidIndex(globalIndex) )
			return NULL;
		return m_nameList.String( m_list[globalIndex].levelName );
	}

	const char *GetName( int globalIndex )
	{
		if ( !m_list.IsValidIndex(globalIndex) )
			return NULL;
		return m_nameList.String( m_list[globalIndex].name );
	}

	int GetIndex( const char *pGlobalname )
	{
		CUtlSymbol symName = m_nameList.Find( pGlobalname );

		if ( symName.IsValid() )
		{
			for ( int i = m_list.Count() - 1; i >= 0; --i )
			{
				if ( m_list[i].name == symName )
					return i;
			}
		}

		return -1;
	}

	int AddEntity( const char *pGlobalname, const char *pMapName, GLOBALESTATE state )
	{
		globalentity_t entity;
		entity.name = m_nameList.AddString( pGlobalname );
		entity.levelName = m_nameList.AddString( pMapName );
		entity.state = state;

		int index = GetIndex( m_nameList.String( entity.name ) );
		if ( index >= 0 )
			return index;
		return m_list.AddToTail( entity );
	}

	int GetNumGlobals( void )
	{
		return m_list.Count();
	}

	void			Reset( void );
	int				Save( ISave &save );
	int				Restore( IRestore &restore );
	DECLARE_SIMPLE_DATADESC();

//#ifdef _DEBUG
	void			DumpGlobals( void );
//#endif

public:
	CUtlSymbolTable	m_nameList;
private:
	bool			m_disableStateUpdates;
	CUtlVector<globalentity_t> m_list;
};

static CGlobalState gGlobalState( "CGlobalState" );

static CUtlSymbolDataOps g_GlobalSymbolDataOps( gGlobalState.m_nameList );


void GlobalEntity_SetState( int globalIndex, GLOBALESTATE state )
{
	gGlobalState.SetState( globalIndex, state );
}

void GlobalEntity_SetCounter( int globalIndex, int counter )
{
	gGlobalState.SetCounter( globalIndex, counter );
}

int GlobalEntity_AddToCounter( int globalIndex, int delta )
{
	return gGlobalState.AddToCounter( globalIndex, delta );
}

void GlobalEntity_EnableStateUpdates( bool bEnable )
{
	gGlobalState.EnableStateUpdates( bEnable );
}


void GlobalEntity_SetMap( int globalIndex, string_t mapname )
{
	gGlobalState.SetMap( globalIndex, mapname );
}

int GlobalEntity_Add( const char *pGlobalname, const char *pMapName, GLOBALESTATE state )
{
	return gGlobalState.AddEntity( pGlobalname, pMapName, state );
}

int GlobalEntity_GetIndex( const char *pGlobalname )
{
	return gGlobalState.GetIndex( pGlobalname );
}

GLOBALESTATE GlobalEntity_GetState( int globalIndex )
{
	return gGlobalState.GetState( globalIndex );
}

int GlobalEntity_GetCounter( int globalIndex )
{
	return gGlobalState.GetCounter( globalIndex );
}

const char *GlobalEntity_GetMap( int globalIndex )
{
	return gGlobalState.GetMap( globalIndex );
}

const char *GlobalEntity_GetName( int globalIndex )
{
	return gGlobalState.GetName( globalIndex );
}

int GlobalEntity_GetNumGlobals( void )
{
	return gGlobalState.GetNumGlobals();
}

CON_COMMAND(dump_globals, "Dump all global entities/states")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	gGlobalState.DumpGlobals();
}

// This is available all the time now on impulse 104, remove later
//#ifdef _DEBUG
void CGlobalState::DumpGlobals( void )
{
	static const char *estates[] = { "Off", "On", "Dead" };

	Msg( "-- Globals --\n" );
	for ( int i = 0; i < m_list.Count(); i++ )
	{
		Msg( "%s: %s (%s) = %d\n", m_nameList.String( m_list[i].name ), m_nameList.String( m_list[i].levelName ), estates[m_list[i].state], m_list[i].counter );
	}
}
//#endif


// Global state Savedata 
BEGIN_SIMPLE_DATADESC( CGlobalState )
	DEFINE_UTLVECTOR( m_list, FIELD_EMBEDDED ),
	// DEFINE_FIELD( m_nameList, CUtlSymbolTable ),
	// DEFINE_FIELD( m_disableStateUpdates, FIELD_BOOLEAN ),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( globalentity_t )
	DEFINE_CUSTOM_FIELD( name, &g_GlobalSymbolDataOps ),
	DEFINE_CUSTOM_FIELD( levelName, &g_GlobalSymbolDataOps ),
	DEFINE_FIELD( state, FIELD_INTEGER ),
	DEFINE_FIELD( counter, FIELD_INTEGER ),
END_DATADESC()


int CGlobalState::Save( ISave &save )
{
	if ( !save.WriteFields( "GLOBAL", this, NULL, m_DataMap.dataDesc, m_DataMap.dataNumFields ) )
		return 0;
	
	return 1;
}

int CGlobalState::Restore( IRestore &restore )
{
	Reset();
	if ( !restore.ReadFields( "GLOBAL", this, NULL, m_DataMap.dataDesc, m_DataMap.dataNumFields ) )
		return 0;
	
	return 1;
}

void CGlobalState::Reset( void )
{
	m_list.Purge();
	m_nameList.RemoveAll();
}


void SaveGlobalState( CSaveRestoreData *pSaveData )
{
	CSave saveHelper( pSaveData );
	gGlobalState.Save( saveHelper );
}


void RestoreGlobalState( CSaveRestoreData *pSaveData )
{
	CRestore restoreHelper( pSaveData );
	gGlobalState.Restore( restoreHelper );
}


//-----------------------------------------------------------------------------
// Purpose: This gets called when a level is shut down
//-----------------------------------------------------------------------------

void ResetGlobalState( void )
{
	gGlobalState.Reset();
}


void ShowServerGameTime()
{
	Msg( "Server game time: %f\n", gpGlobals->curtime );
}

CON_COMMAND(server_game_time, "Gives the game time in seconds (server's curtime)")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	ShowServerGameTime();
}
