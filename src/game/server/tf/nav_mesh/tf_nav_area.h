//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_nav_area.h
// TF specific nav area
// Michael Booth, February 2009

#ifndef TF_NAV_AREA_H
#define TF_NAV_AREA_H

#include "nav_area.h"
#include "tf_shareddefs.h"

enum TFNavAttributeType
{
	TF_NAV_INVALID						= 0x00000000,

	// Also look for NAV_MESH_NAV_BLOCKER (w/ nav_debug_blocked ConVar).
	TF_NAV_BLOCKED						= 0x00000001,			// blocked for some TF-specific reason
	TF_NAV_SPAWN_ROOM_RED				= 0x00000002,
	TF_NAV_SPAWN_ROOM_BLUE				= 0x00000004,
	TF_NAV_SPAWN_ROOM_EXIT				= 0x00000008,
	TF_NAV_HAS_AMMO						= 0x00000010,
	TF_NAV_HAS_HEALTH					= 0x00000020,
	TF_NAV_CONTROL_POINT				= 0x00000040,

	TF_NAV_BLUE_SENTRY_DANGER			= 0x00000080,			// sentry can potentially fire upon enemies in this area
	TF_NAV_RED_SENTRY_DANGER			= 0x00000100,

	TF_NAV_BLUE_SETUP_GATE				= 0x00000800,			// this area is blocked until the setup period is over
	TF_NAV_RED_SETUP_GATE				= 0x00001000,			// this area is blocked until the setup period is over
	TF_NAV_BLOCKED_AFTER_POINT_CAPTURE	= 0x00002000,			// this area becomes blocked after the first point is capped
	TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE  = 0x00004000,			// this area is blocked until the first point is capped, then is unblocked
	TF_NAV_BLUE_ONE_WAY_DOOR			= 0x00008000,
	TF_NAV_RED_ONE_WAY_DOOR				= 0x00010000,

 	TF_NAV_WITH_SECOND_POINT			= 0x00020000,			// modifier for BLOCKED_*_POINT_CAPTURE
 	TF_NAV_WITH_THIRD_POINT				= 0x00040000,			// modifier for BLOCKED_*_POINT_CAPTURE
  	TF_NAV_WITH_FOURTH_POINT			= 0x00080000,			// modifier for BLOCKED_*_POINT_CAPTURE
 	TF_NAV_WITH_FIFTH_POINT				= 0x00100000,			// modifier for BLOCKED_*_POINT_CAPTURE

	TF_NAV_SNIPER_SPOT					= 0x00200000,			// this is a good place for a sniper to lurk
	TF_NAV_SENTRY_SPOT					= 0x00400000,			// this is a good place to build a sentry

	TF_NAV_ESCAPE_ROUTE					= 0x00800000,			// for Raid mode
	TF_NAV_ESCAPE_ROUTE_VISIBLE			= 0x01000000,			// all areas that have visibility to the escape route

	TF_NAV_NO_SPAWNING					= 0x02000000,			// don't spawn bots in this area

 	TF_NAV_RESCUE_CLOSET				= 0x04000000,			// for respawning friends in Raid mode

 	TF_NAV_BOMB_CAN_DROP_HERE			= 0x08000000,			// the bomb can be dropped here and reached by the invaders in MvM

	TF_NAV_DOOR_NEVER_BLOCKS			= 0x10000000,
	TF_NAV_DOOR_ALWAYS_BLOCKS			= 0x20000000,

	TF_NAV_UNBLOCKABLE					= 0x40000000,			// this area cannot be blocked

	// save/load these manually set flags, and don't clear them between rounds
	TF_NAV_PERSISTENT_ATTRIBUTES		= TF_NAV_SNIPER_SPOT | TF_NAV_SENTRY_SPOT | TF_NAV_NO_SPAWNING | TF_NAV_BLUE_SETUP_GATE | TF_NAV_RED_SETUP_GATE | TF_NAV_BLOCKED_AFTER_POINT_CAPTURE | TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE | TF_NAV_BLUE_ONE_WAY_DOOR | TF_NAV_RED_ONE_WAY_DOOR | TF_NAV_DOOR_NEVER_BLOCKS | TF_NAV_DOOR_ALWAYS_BLOCKS | TF_NAV_UNBLOCKABLE | TF_NAV_WITH_SECOND_POINT | TF_NAV_WITH_THIRD_POINT | TF_NAV_WITH_FOURTH_POINT | TF_NAV_WITH_FIFTH_POINT | TF_NAV_RESCUE_CLOSET
};

class CTFNavArea : public CNavArea
{
public:
	DECLARE_CLASS( CTFNavArea, CNavArea );

	CTFNavArea( void );
	~CTFNavArea( void );

	virtual void OnServerActivate( void );						// (EXTEND) invoked when map is initially loaded
	virtual void OnRoundRestart( void );						// (EXTEND) invoked for each area when the round restarts

	virtual void CustomAnalysis( bool isIncremental = false );	// for game-specific analysis
	virtual void Draw( void ) const;							// draw area for debugging & editing

	virtual void UpdateBlocked( bool force = false, int teamID = TEAM_ANY )		{ }		// we'll handle managing blocked status directly
	virtual bool IsBlocked( int teamID, bool ignoreNavBlockers = false ) const;

	virtual void Save( CUtlBuffer &fileBuffer, unsigned int version ) const;								// (EXTEND)
	virtual NavErrorType Load( CUtlBuffer &fileBuffer, unsigned int version, unsigned int subVersion );		// (EXTEND)

	float GetIncursionDistance( int team ) const;				// return travel distance from the team's active spawn room to this area, -1 for invalid
	CTFNavArea *GetNextIncursionArea( int team ) const;			// return adjacent area with largest increase in incursion distance
	bool IsReachableByTeam( int team ) const;					// return true if the given team can reach this area
	void CollectPriorIncursionAreas( int team, CUtlVector< CTFNavArea * > *priorVector );	// populate 'priorVector' with a collection of adjacent areas that have a lower incursion distance that this area
	void CollectNextIncursionAreas( int team, CUtlVector< CTFNavArea * > *priorVector );	// populate 'priorVector' with a collection of adjacent areas that have a higher incursion distance that this area

	const CUtlVector< CTFNavArea * > &GetEnemyInvasionAreaVector( int myTeam ) const;	// given OUR team index, return list of areas the enemy is invading from
	bool IsAwayFromInvasionAreas( int myTeam, float safetyRange = 1000.0f ) const;		// return true if this area is at least safetyRange units away from all invasion areas

	void ComputeInvasionAreaVectors( void );
	void SetInvasionSearchMarker( unsigned int marker );
	bool IsInvasionSearchMarked( unsigned int marker ) const;

	void SetAttributeTF( int flags );
	void ClearAttributeTF( int flags );
	bool HasAttributeTF( int flags ) const;

	void AddPotentiallyVisibleActor( CBaseCombatCharacter *who );
	void RemovePotentiallyVisibleActor( CBaseCombatCharacter *who );
	void ClearAllPotentiallyVisibleActors( void );
	bool IsPotentiallyVisibleToActor( CBaseCombatCharacter *who ) const;	// return true if the given actor has potential visibility to this area

	virtual bool IsPotentiallyVisibleToTeam( int team ) const;				// return true if any portion of this area is visible to anyone on the given team (very fast)

	class IForEachPotentiallyVisibleActor
	{
	public:
		virtual bool Inspect( CBaseCombatCharacter *who ) = 0;
	};
	bool ForEachPotentiallyVisibleActor( IForEachPotentiallyVisibleActor &func, int team = TEAM_ANY );

	void OnCombat( void );										// invoked when combat happens in/near this area
	bool IsInCombat( void ) const;								// return true if this area has seen combat recently
	float GetCombatIntensity( void ) const;						// 1 = in active combat, 0 = quiet

	static void MakeNewTFMarker( void );
	static void ResetTFMarker( void );
	bool IsTFMarked( void ) const;
	void TFMark( void );

	// Raid mode -------------------------------------------------
	void AddToWanderCount( int count );
	void SetWanderCount( int count );
	int GetWanderCount( void ) const;

	bool IsValidForWanderingPopulation( void ) const;
	// Raid mode -------------------------------------------------

	// Distance for MvM bomb delivery
	float GetTravelDistanceToBombTarget( void ) const;

	//- Script access to nav functions ------------------------------------------------------------------
	DECLARE_ENT_SCRIPTDESC();
	HSCRIPT GetScriptInstance();
	bool IsBottleneck( void ) const;
	Vector FindRandomSpot( void ) const;						// return a random spot in this area
	void OnDoorCreated( CBaseEntity *door );					// invoked when a door is created
	CBaseEntity *GetDoor( void ) const;							// return a door contained in this area
	
	int ScriptGetID( void ) const { return (int)GetID(); }
	void ScriptGetAdjacentAreas( int dir, HSCRIPT hTable );
	HSCRIPT ScriptGetAdjacentArea( int dir, int i );
	HSCRIPT ScriptGetRandomAdjacentArea( int dir );
	void ScriptGetIncomingConnections( int dir, HSCRIPT hTable );
	void ScriptAddIncomingConnection( HSCRIPT hSource, int incomingEdgeDir );
	void ScriptConnectToArea( HSCRIPT hArea, int dir );
	void ScriptDisconnectArea( HSCRIPT hArea );
	bool ScriptIsConnectedArea( HSCRIPT hArea, int dir );
	Vector ScriptGetCorner( int corner ) const { return GetCorner( (NavCornerType)corner ); }
	void ScriptMarkAsBlocked( int teamID );
	int ScriptGetAdjacentCount( int dir ) const	{ return GetAdjacentCount( (NavDirType)dir ); }
	const char* ScriptGetPlaceName();
	void ScriptSetPlaceName( const char* pszName );
	int ScriptComputeDirection( const Vector &point ) const;
	int ScriptGetPlayerCount( int teamID ) const { return GetPlayerCount( teamID ); }
	bool ScriptIsOverlapping( HSCRIPT hArea ) const;
	bool ScriptIsOverlappingOrigin( const Vector &pos, float tolerance ) const { return IsOverlapping( pos, tolerance ); }
	bool ScriptIsEdge( int dir ) const { return IsEdge( (NavDirType) dir ); }
	bool ScriptContains( HSCRIPT hArea ) const;
	bool ScriptContainsOrigin( const Vector &pos ) const { return Contains( pos ); }
	float ScriptComputeGroundHeightChange( HSCRIPT hArea );
	HSCRIPT ScriptGetParent( void );
	int ScriptGetParentHow( void ) const { return GetParentHow(); }
	void ScriptUnblockArea( void );
	bool ScriptIsVisible( const Vector &eye ) const	{ return IsVisible( eye ); }
	float ScriptGetZ( const Vector &pos ) const	{ return GetZ( pos ); }
	bool ScriptIsCoplanar( HSCRIPT hArea ) const;
	bool ScriptIsContiguous( HSCRIPT hArea ) const;
	float ScriptComputeAdjacentConnectionHeightChange( HSCRIPT hArea ) const;
	void ScriptRemoveOrthogonalConnections( int dir );
	HSCRIPT ScriptGetElevator( void ) { return ToHScript( (CBaseEntity*)GetElevator() ); }
	void ScriptGetElevatorAreas( HSCRIPT hTable );
	HSCRIPT ScriptGetDoor( void ) { return ToHScript( GetDoor() ); }
	Vector ScriptComputeClosestPointInPortal( HSCRIPT to, int dir, const Vector &fromPos ) const;

private:
	friend class CTFNavMesh;

	float m_distanceFromSpawnRoom[ TF_TEAM_COUNT ];
	CUtlVector< CTFNavArea * > m_invasionAreaVector[ TF_TEAM_COUNT ];	// use our team as index to get list of areas the enemy is invading from
	unsigned int m_invasionSearchMarker;

	unsigned int m_attributeFlags;

	CUtlVector< CHandle< CBaseCombatCharacter > > m_potentiallyVisibleActor[ TF_TEAM_COUNT ];

	float m_combatIntensity;
	IntervalTimer m_combatTimer;

	static unsigned int m_masterTFMark;
	unsigned int m_TFMark;					// this area's mark

	// Raid mode -------------------------------------------------
	int m_wanderCount;						// how many wandering defenders to populate here
	// Raid mode -------------------------------------------------

	float m_distanceToBombTarget;

	EHANDLE m_hDoor;

	HSCRIPT	m_hScriptInstance;
};

inline HSCRIPT ToHScript( CNavArea *pArea )
{
	CTFNavArea* pTerrorArea = ( CTFNavArea* )pArea;
	return ( pTerrorArea ) ? pTerrorArea->GetScriptInstance() : NULL;
}

inline HSCRIPT ToHScript( CTFNavArea *pArea )
{
	return ( pArea ) ? pArea->GetScriptInstance() : NULL;
}

template <> ScriptClassDesc_t *GetScriptDesc<CTFNavArea>( CTFNavArea * );
inline CTFNavArea *ToNavArea( HSCRIPT hScript )
{
	return ( IsValid( hScript ) ) ? (CTFNavArea *)g_pScriptVM->GetInstanceValue( hScript, GetScriptDescForClass(CTFNavArea) ) : NULL;
}

inline float CTFNavArea::GetTravelDistanceToBombTarget( void ) const
{
	return m_distanceToBombTarget;
}

inline void CTFNavArea::AddToWanderCount( int count )
{
	m_wanderCount += count;
}

inline void CTFNavArea::SetWanderCount( int count )
{
	m_wanderCount = count;
}

inline int CTFNavArea::GetWanderCount( void ) const
{
	return m_wanderCount;
}



inline bool CTFNavArea::IsPotentiallyVisibleToActor( CBaseCombatCharacter *who ) const
{
	if ( who == NULL )
		return false;

	int team = who->GetTeamNumber();
	if ( team < 0 || team >= TF_TEAM_COUNT )
		return false;

	return m_potentiallyVisibleActor[ team ].Find( who ) != m_potentiallyVisibleActor[ team ].InvalidIndex();
}


inline bool CTFNavArea::IsPotentiallyVisibleToTeam( int team ) const
{
	return team >= 0 && team < TF_TEAM_COUNT && m_potentiallyVisibleActor[ team ].Count() > 0;
}


inline bool CTFNavArea::ForEachPotentiallyVisibleActor( CTFNavArea::IForEachPotentiallyVisibleActor &func, int team )
{
	if ( team == TEAM_ANY )
	{
		for( int t=0; t<TF_TEAM_COUNT; ++t )
		{
			for( int i=0; i<m_potentiallyVisibleActor[ t ].Count(); ++i )
			{
				CBaseCombatCharacter *who = m_potentiallyVisibleActor[ t ][ i ];

				if ( who && func.Inspect( who ) == false )
				{
					return false;
				}
			}
		}
	}
	else if ( team >= 0 && team < TF_TEAM_COUNT )
	{
		for( int i=0; i<m_potentiallyVisibleActor[ team ].Count(); ++i )
		{
			CBaseCombatCharacter *who = m_potentiallyVisibleActor[ team ][ i ];

			if ( who && func.Inspect( who ) == false )
			{
				return false;
			}
		}
	}

	return true;
}

inline void CTFNavArea::RemovePotentiallyVisibleActor( CBaseCombatCharacter *who )
{
	for( int i=0; i<TF_TEAM_COUNT; ++i )
		m_potentiallyVisibleActor[i].FindAndFastRemove( who );
}

inline void CTFNavArea::ClearAllPotentiallyVisibleActors( void )
{
	for( int i=0; i<TF_TEAM_COUNT; ++i )
		m_potentiallyVisibleActor[i].RemoveAll();
}

inline float CTFNavArea::GetIncursionDistance( int team ) const
{
	if ( team < 0 || team >= TF_TEAM_COUNT )
	{
		return -1.0f;
	}

	return m_distanceFromSpawnRoom[ team ];
}

inline bool CTFNavArea::IsReachableByTeam( int team ) const
{
	if ( team < 0 || team >= TF_TEAM_COUNT )
	{
		return false;
	}

	return m_distanceFromSpawnRoom[ team ] >= 0.0f;
}

inline const CUtlVector< CTFNavArea * > &CTFNavArea::GetEnemyInvasionAreaVector( int myTeam ) const
{
	if ( myTeam < 0 || myTeam >= TF_TEAM_COUNT )
	{
		myTeam = 0.0f;
	}

	return m_invasionAreaVector[ myTeam ];
}

inline void CTFNavArea::SetInvasionSearchMarker( unsigned int marker )
{
	m_invasionSearchMarker = marker;
}

inline bool CTFNavArea::IsInvasionSearchMarked( unsigned int marker ) const
{
	return marker == m_invasionSearchMarker;
}

inline void CTFNavArea::SetAttributeTF( int flags )
{
	m_attributeFlags |= flags;
}

inline void CTFNavArea::ClearAttributeTF( int flags )
{
	m_attributeFlags &= ~flags;
}

inline bool CTFNavArea::HasAttributeTF( int flags ) const
{
	return ( m_attributeFlags & flags ) ? true : false;
}

#endif // TF_NAV_AREA_H
