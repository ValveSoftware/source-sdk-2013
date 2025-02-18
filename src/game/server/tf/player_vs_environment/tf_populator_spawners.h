//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: tf_populator_spawners
// Implementations of NPC Spawning Code for PvE related game modes (MvM)
//=============================================================================//
#ifndef TF_POPULATOR_SPAWNERS_H
#define TF_POPULATOR_SPAWNERS_H


#include "bot/tf_bot.h"
#include "tf_mann_vs_machine_stats.h"

class CMannVsMachineStats;
class KeyValues;
class IPopulator;
class CPopulationManager;
class CWave;

enum RelativePositionType
{
	UNDEFINED = 0,
	AHEAD,
	BEHIND,
	ANYWHERE
};

struct EventInfo
{
	EventInfo() : m_delay(0.0f), m_param() {}
	
	CFmtStr m_target;
	CFmtStr m_action;
	variant_t m_param;
	float m_delay;
};

enum SpawnLocationResult
{
	SPAWN_LOCATION_NOT_FOUND = 0,
	SPAWN_LOCATION_NAV,
	SPAWN_LOCATION_TELEPORTER
};

typedef CUtlVector< CHandle< CBaseEntity > > EntityHandleVector_t;
typedef CUtlVector< CHandle< CTFTeamSpawn > > TFTeamSpawnVector_t;

//--------------------------------------------------------------------------------------------------------
//
// Return a random value with a distribution like so:
// 1|  /
//  | /
//  |/
//  /----
//  0   1
inline float SkewedRandomValue( void )
{
	float x = RandomFloat( 0, 1.0f );
	float y = RandomFloat( 0, 1.0f );
	return x < y ? y : x;	
}

//-----------------------------------------------------------------------
class CMvMBotUpgrade
{
public:
	char	szAttrib[ MAX_ATTRIBUTE_DESCRIPTION_LENGTH ];	// Debug
	int		iAttribIndex;	
	float	flValue;
	float	flMax;
	int		nCost;
	bool    bIsBotAttr;
	bool	bIsSkillAttr;		// Probably want to make these an enum or flag later
};

//-----------------------------------------------------------------------
class CTFNavAreaIncursionLess
{
public:
	bool Less( const CTFNavArea *a, const CTFNavArea *b, void *pCtx )
	{
		return a->GetIncursionDistance( TF_TEAM_BLUE ) < b->GetIncursionDistance( TF_TEAM_BLUE );
	}
};

//-----------------------------------------------------------------------
// A Spawner is responsible for actually spawning a particular
// instance of an entity into the environment.
// If 'result' is non-NULL, spawned entities are added to this vector.
class IPopulationSpawner
{
public:
	// We need a virtual destructor or else the derived-class destructors won't be called,
	// leading to memory leaks. Found via clang warning.
	virtual ~IPopulationSpawner()
	{
	}

	IPopulationSpawner( IPopulator *populator )
	{
		m_populator = populator;
	}

	IPopulator *GetPopulator( void ) const
	{
		return m_populator;
	}

	virtual bool Parse( KeyValues *data ) = 0;
	virtual bool Spawn( const Vector &here, EntityHandleVector_t *result = NULL ) = 0;
	virtual bool IsWhereRequired( void ) const		// does this spawner need a valid Where parameter?
	{
		return true;
	}
	
	virtual bool IsVarious( void ) { return false; }
	virtual int GetClass( int nSpawnNum = -1 ) { return TF_CLASS_UNDEFINED; }
	virtual string_t GetClassIcon( int nSpawnNum = -1 ) { return NULL_STRING; }
	virtual int GetHealth( int nSpawnNum = -1  ){ return 0; }
	virtual bool IsMiniBoss( int nSpawnNum = -1 ) { return false; }
	virtual bool HasAttribute( CTFBot::AttributeType type, int nSpawnNum = -1 ) { return false; }
	virtual bool HasEventChangeAttributes( const char* pszEventName ) const = 0;

	static IPopulationSpawner *ParseSpawner( IPopulator *populator, KeyValues *data );

protected:
	IPopulator *m_populator;
};


//-----------------------------------------------------------------------
// A RandomChoice spawner picks one of the Spawners in its
// vector at random and invokes it to spawn entities.
class CRandomChoiceSpawner : public IPopulationSpawner
{
public:
	CRandomChoiceSpawner( IPopulator *populator );
	virtual ~CRandomChoiceSpawner();

	virtual bool Parse( KeyValues *data );
	virtual bool Spawn( const Vector &here, CUtlVector< CHandle< CBaseEntity > > *result = NULL );

	virtual bool IsVarious( void ) { return true; }
	virtual int GetClass( int nSpawnNum = -1 );
	virtual string_t GetClassIcon( int nSpawnNum = -1 );
	virtual int GetHealth( int nSpawnNum = -1  );
	virtual bool IsMiniBoss( int nSpawnNum = -1 ) OVERRIDE;
	virtual bool HasAttribute( CTFBot::AttributeType type, int nSpawnNum = -1 );

	virtual bool HasEventChangeAttributes( const char* pszEventName ) const OVERRIDE;

	CUtlVector< IPopulationSpawner * > m_spawnerVector;
	CUtlVector< int > m_nRandomPickDecision;

	int m_nNumSpawned;
};


//-----------------------------------------------------------------------
class CTFBotSpawner : public IPopulationSpawner
{
public:
	CTFBotSpawner( IPopulator *populator );
	virtual ~CTFBotSpawner() { }

	virtual bool Parse( KeyValues *data );
	bool ParseEventChangeAttributes( KeyValues *data );
	virtual bool Spawn( const Vector &here, EntityHandleVector_t *result = NULL );

	virtual int GetClass( int nSpawnNum = -1 );
	virtual string_t GetClassIcon( int nSpawnNum = -1 );
	virtual int GetHealth( int nSpawnNum = -1  );
	virtual bool IsMiniBoss( int nSpawnNum = -1 ) OVERRIDE;
	virtual bool HasAttribute( CTFBot::AttributeType type, int nSpawnNum = -1 );

	virtual bool HasEventChangeAttributes( const char* pszEventName ) const OVERRIDE;

	int m_class;
	string_t m_iszClassIcon;

	int m_health;
	float m_scale;
	float m_flAutoJumpMin;
	float m_flAutoJumpMax;

	CUtlString m_name;
	CUtlStringList m_teleportWhereName;

	CTFBot::EventChangeAttributes_t m_defaultAttributes;
	CUtlVector< CTFBot::EventChangeAttributes_t > m_eventChangeAttributes;
};

//-----------------------------------------------------------------------
class CTankSpawner : public IPopulationSpawner
{
public:
	CTankSpawner( IPopulator *populator );

	virtual string_t GetClassIcon( int nSpawnNum = -1 ) { return MAKE_STRING( "tank" ); }
	virtual int GetHealth( int nSpawnNum = -1  ){ return m_health; }

	virtual bool Parse( KeyValues *data );
	virtual bool Spawn( const Vector &here, EntityHandleVector_t *result = NULL );

	virtual bool IsWhereRequired( void ) const		// does this spawner need a valid Where parameter?
	{
		// the Tank spawns at a given path node
		return false;
	}

	virtual bool IsMiniBoss( int nSpawnNum = -1 ) OVERRIDE { return true; }

	virtual bool HasEventChangeAttributes( const char* pszEventName ) const OVERRIDE { return false; }

	int m_health;
	float m_speed;
	CUtlString m_name;
	CUtlString m_startingPathTrackNodeName;		// which path_track we start at
	int m_skin;
	EventInfo *m_onKilledOutput;
	EventInfo *m_onBombDroppedOutput;
};

//-----------------------------------------------------------------------
class CSentryGunSpawner : public IPopulationSpawner
{
public:
	CSentryGunSpawner( IPopulator *populator );

	virtual bool Parse( KeyValues *data );
	virtual bool Spawn( const Vector &here, EntityHandleVector_t *result = NULL );

	virtual bool HasEventChangeAttributes( const char* pszEventName ) const OVERRIDE { return false; }

	int m_level;
};


//-----------------------------------------------------------------------
class CSquadSpawner : public IPopulationSpawner
{
public:
	CSquadSpawner( IPopulator *populator );
	virtual ~CSquadSpawner();

	virtual bool Parse( KeyValues *data );
	virtual bool Spawn( const Vector &here, EntityHandleVector_t *result = NULL );

	virtual bool IsVarious( void ) { return true; }
	virtual int GetClass( int nSpawnNum = -1 );
	virtual string_t GetClassIcon( int nSpawnNum = -1 );
	virtual int GetHealth( int nSpawnNum = -1  );
	virtual bool IsMiniBoss( int nSpawnNum = -1 ) OVERRIDE;
	virtual bool HasAttribute( CTFBot::AttributeType type, int nSpawnNum = -1 );

	virtual bool HasEventChangeAttributes( const char* pszEventName ) const OVERRIDE;

	CUtlVector< IPopulationSpawner * > m_memberSpawnerVector;	// all of these are invoked to instantiate the squad

	float m_formationSize;
	bool m_bShouldPreserveSquad;
};


//-----------------------------------------------------------------------
class CMobSpawner : public IPopulationSpawner
{
public:
	CMobSpawner( IPopulator *populator );
	virtual ~CMobSpawner();

	virtual bool Parse( KeyValues *data );
	virtual bool Spawn( const Vector &here, EntityHandleVector_t *result = NULL );
	virtual bool HasEventChangeAttributes( const char* pszEventName ) const OVERRIDE;

	int m_count;
	IPopulationSpawner *m_spawner;

private:
	// TODO: Rethink this, since spawners are one-shot, and mobs need to spawn over time...
	CountdownTimer m_mobSpawnTimer;
	CountdownTimer m_mobLifetimeTimer;
	CTFNavArea *m_mobArea;
	int m_mobCountRemaining;
};


#endif // TF_POPULATOR_SPAWNERS_H
