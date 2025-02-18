//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_nav_mesh.h
// TF specific nav mesh
// Michael Booth, February 2009

#ifndef TF_NAV_MESH_H
#define TF_NAV_MESH_H

#include "nav_mesh.h"
#include "tf_nav_area.h"
#include "tf_obj_teleporter.h"

#define TF_PLAYER_JUMP_HEIGHT	45.0f			// non crouch-jumping

class CBaseObject;
class CObjectTeleporter;
class CTFPlayer;

//-------------------------------------------------------------------------
// General purpose collector class for ForAllArea-style functor methods
class CTFAreaCollector
{
public:
	bool operator() ( CNavArea *area )
	{
		m_vector.AddToTail( (CTFNavArea *)area );
		return true;
	}

	CUtlVector< CTFNavArea * > m_vector;
};


//-------------------------------------------------------------------------
class CTFNavMesh : public CNavMesh
{
public:
	CTFNavMesh( void );

	virtual CTFNavArea *CreateArea( void ) const;						// CNavArea factory

	virtual void Update( void );										// invoked on each game frame

	virtual unsigned int GetSubVersionNumber( void ) const;									// returns sub-version number of data format used by derived classes
	virtual void SaveCustomData( CUtlBuffer &fileBuffer ) const;							// store custom mesh data for derived classes
	virtual void LoadCustomData( CUtlBuffer &fileBuffer, unsigned int subVersion );			// load custom mesh data for derived classes

	virtual void OnServerActivate( void );								// (EXTEND) invoked when server loads a new map
	virtual void OnRoundRestart( void );								// invoked when a game round restarts

	virtual void FireGameEvent( IGameEvent *event );

	/**
	 * Return true if nav mesh can be trusted for all climbing/jumping decisions because game environment is fairly simple.
	 * Authoritative meshes mean path followers can skip CPU intesive realtime scanning of unpredictable geometry.
	 */
	virtual bool IsAuthoritative( void ) const { return true; }			// TF2 has nice clean environments

	virtual unsigned int GetGenerationTraceMask( void ) const;			// return the mask used by traces when generating the mesh

	void OnObjectChanged();
	bool IsSentryGunHere( CTFNavArea *area ) const;						// return true if a Sentry Gun has been built in the given area

	void CollectBuiltObjects( CUtlVector< CBaseObject * > *collectionVector, int team = TEAM_ANY );	// fill given vector will all objects on the given team

	struct BallisticLaunchInfo
	{
		Vector m_launchSpot;						// where to stand
		float m_aimYaw;								// how to aim
		float m_aimPitch;							// how to aim
		float m_chargeTime;							// how long to charge weapon
	};

	// populate the given vector with ways to launch grenades to hit the given building
	void CollectBallisticAttackInfo( CBaseObject *building, CUtlVector< BallisticLaunchInfo > *infoVector ) const;

	void ResetMeshAttributes( bool bScheduleRecomputation );

	void CollectControlPointAreas( void );

	void DecorateMesh( void );
	void DecorateMeshTacticalHints( void );
	void RemoveAllMeshDecoration( void );

	// populate the given "ambushVector" with good areas to lurk in ambush for the invading enemy team
	void CollectAmbushAreas( CUtlVector< CTFNavArea * > *ambushVector, CTFNavArea *startArea, int teamToAmbush, float searchRadius, float incursionTolerance = 300.0f ) const;

	// populate the given vector with areas that are just outside of the given team's spawn room(s)
	void CollectSpawnRoomThresholdAreas( CUtlVector< CTFNavArea * > *spawnExitAreaVector, int team ) const;

	// populate the given vector with areas that have a bomb travel distance within the given range
	void CollectAreaWithinBombTravelRange( CUtlVector< CTFNavArea * > *spawnExitAreaVector, float minTravel, float maxTravel ) const;

	const CUtlVector< CTFNavArea * > *GetSetupGateDefenseAreas( void ) const;	// return vector of areas that are good for defending enemies coming out of the blue setup gates
	const CUtlVector< CTFNavArea * > *GetControlPointAreas( int pointIndex ) const;		// return vector of areas overlapping the given control point
	CTFNavArea *GetControlPointCenterArea( int pointIndex ) const;				// return area overlapping the center of the given control point
	const CUtlVector< CTFNavArea * > *GetSpawnRoomAreas( int team ) const;		// return vector of areas within the given team spawn room(s)
	const CUtlVector< CTFNavArea * > *GetSpawnRoomExitAreas( int team ) const;	// return vector of areas where the given team exits their spawn room(s)

	enum RecomputeReasonType
	{
		RESET,
		SETUP_FINISHED,
		POINT_CAPTURED,
		POINT_UNLOCKED,
		BLOCKED_STATUS_CHANGED,
		MAP_LOGIC
	};
	void ScheduleRecomputationOfInternalData( RecomputeReasonType reason, int whichPoint );

	virtual void OnDoorCreated( CBaseEntity *door );					// invoked when a door is created

protected:
	virtual void BeginCustomAnalysis( bool bIncremental );
	virtual void PostCustomAnalysis( void );							// invoked when custom analysis step is complete
	virtual void EndCustomAnalysis();

private:
	void ComputeIncursionDistances( void );					// recompute travel distance from each team's spawn room for each nav area
	void ComputeIncursionDistances( CTFNavArea *spawnArea, int team );
	void ComputeInvasionAreas( void );
	void ComputeLegalBombDropAreas( void );
	void ComputeBombTargetDistance();

	void UpdateDebugDisplay( void ) const;

	void OnBlockedAreasChanged( void );

	void ComputeBlockedAreas( void );

	CountdownTimer m_recomputeInternalDataTimer;			// if started, when counts down recompute internal data to give various map logic time to complete
	RecomputeReasonType m_recomputeReason;
	int m_recomputeReasonWhichPoint;
	void RecomputeInternalData( void );

	// Array of areas with sentry danger attributes set.
	CUtlVector< CTFNavArea * > m_sentryAreas;

	CUtlVector< CTFNavArea * > m_setupGateDefenseAreaVector;

	CUtlVector< CTFNavArea * > m_controlPointAreaVector[ MAX_CONTROL_POINTS ];
	CTFNavArea *m_controlPointCenterAreaVector[ MAX_CONTROL_POINTS ];

	CUtlVector< CTFNavArea * > m_redSpawnRoomAreaVector;
	CUtlVector< CTFNavArea * > m_blueSpawnRoomAreaVector;

	CUtlVector< CTFNavArea * > m_redSpawnRoomExitAreaVector;
	CUtlVector< CTFNavArea * > m_blueSpawnRoomExitAreaVector;
	void CollectAndMarkSpawnRoomExits( CTFNavArea *area, CUtlVector< CTFNavArea * > *exitAreaVector );

	CountdownTimer m_watchCartTimer;

	int m_priorBotCount;
};


inline void CTFNavMesh::ScheduleRecomputationOfInternalData( CTFNavMesh::RecomputeReasonType reason, int whichPoint = 0 )
{
	m_recomputeInternalDataTimer.Start( 2.0f );
	m_recomputeReason = reason;
	m_recomputeReasonWhichPoint = whichPoint;
}


inline const CUtlVector< CTFNavArea * > *CTFNavMesh::GetSpawnRoomAreas( int team ) const
{
	if ( team == TF_TEAM_RED )
	{
		return &m_redSpawnRoomAreaVector;
	}

	if ( team == TF_TEAM_BLUE )
	{
		return &m_blueSpawnRoomAreaVector;
	}

	return NULL;
}

inline const CUtlVector< CTFNavArea * > *CTFNavMesh::GetSpawnRoomExitAreas( int team ) const
{
	if ( team == TF_TEAM_RED )
	{
		return &m_redSpawnRoomExitAreaVector;
	}

	if ( team == TF_TEAM_BLUE )
	{
		return &m_blueSpawnRoomExitAreaVector;
	}

	return NULL;
}

inline const CUtlVector< CTFNavArea * > *CTFNavMesh::GetControlPointAreas( int pointIndex ) const
{
	if ( pointIndex < 0 || pointIndex >= MAX_CONTROL_POINTS )
	{
		return NULL;
	}

	return &m_controlPointAreaVector[ pointIndex ];
}

inline CTFNavArea *CTFNavMesh::GetControlPointCenterArea( int pointIndex ) const
{
	if ( pointIndex < 0 || pointIndex >= MAX_CONTROL_POINTS )
	{
		return NULL;
	}

	return m_controlPointCenterAreaVector[ pointIndex ];
}

inline const CUtlVector< CTFNavArea * > *CTFNavMesh::GetSetupGateDefenseAreas( void ) const
{
	return &m_setupGateDefenseAreaVector;
}


inline unsigned int CTFNavMesh::GetGenerationTraceMask( void ) const
{
	return MASK_PLAYERSOLID_BRUSHONLY;
}


inline CTFNavMesh *TheTFNavMesh( void )
{
	return reinterpret_cast< CTFNavMesh * >( TheNavMesh );
}


extern TFNavAttributeType NameToTFAttribute( const char *name );
extern const char *TFAttributeToName( TFNavAttributeType attribute );

#endif // TF_NAV_MESH_H
