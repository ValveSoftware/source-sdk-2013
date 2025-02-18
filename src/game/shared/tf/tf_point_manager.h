//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#ifndef TF_POINT_MANAGER_H
#define TF_POINT_MANAGER_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#define CTFPointManager C_TFPointManager
#endif // CLIENT_DLL

#define MAX_POINT_MANAGER_POINTS	30

// basic information of points
// derived class should add extra data to its own struct
// see tf_flame_point_t for example
struct tf_point_t
{
	virtual ~tf_point_t() {}

	Vector	m_vecPosition = vec3_origin;
	Vector	m_vecVelocity = vec3_origin;
	float	m_flSpawnTime = 0.f;
	float	m_flLifeTime = 0.f;
	int		m_nPointIndex = 0;
	
	int		m_nHitWall = 0;
	Vector	m_vecPrevPosition = vec3_origin; // for collision
};
typedef CUtlVector< tf_point_t* > TFPointVec_t;

class CTFPointManager : public CBaseEntity
{
	DECLARE_CLASS( CTFPointManager, CBaseEntity );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	CTFPointManager();

	virtual void Spawn( void ) OVERRIDE;

	virtual void UpdateOnRemove( void ) OVERRIDE;

#ifdef GAME_DLL
	virtual void Touch( CBaseEntity *pOther ) OVERRIDE;

	virtual int UpdateTransmitState() OVERRIDE;
	virtual unsigned int PhysicsSolidMaskForEntity( void ) const OVERRIDE;

	virtual bool AddPoint( int iCurrentTick );
	void PointThink();
#else
	virtual void OnDataChanged( DataUpdateType_t updateType ) OVERRIDE;
	virtual void PostDataUpdate( DataUpdateType_t updateType ) OVERRIDE;
	virtual void ClientThink() OVERRIDE;
#endif // GAME_DLL

	void ClearPoints( void );

protected:
	virtual void InitializePoint( tf_point_t *pPoint, int nPointIndex );
	virtual Vector GetInitialPosition() const { return vec3_origin; }
	virtual Vector GetInitialVelocity() const { return vec3_origin; }

#ifdef GAME_DLL
	virtual bool ShouldCollide( CBaseEntity *pEnt ) const { return true; }
	virtual void OnCollide( CBaseEntity *pEnt, int nPointIndex ) {};
#endif // GAME_DLL

#ifdef CLIENT_DLL
	virtual void OnClientPointAdded( const tf_point_t *pNewestPoint ) {}
#endif // CLIENT_DLL

	virtual tf_point_t *AllocatePoint() { return new tf_point_t; }

	// update funcs
	virtual void Update();
	virtual bool UpdatePoint( tf_point_t *pPoint, int nIndex, float flDT, Vector *pVecNewPos = NULL, Vector *pVecMins = NULL, Vector *pVecMaxs = NULL );
	virtual bool OnPointHitWall( tf_point_t *pPoint, Vector &vecNewPos, Vector &vecNewVelocity, const trace_t& tr, float flDT ); // return true if point needs to be removed
	virtual void ModifyAdditionalMovementInfo( tf_point_t *pPoint, float flDT ) {}

	virtual float	GetInitialSpeed() const { return 0.f; }
	virtual float	GetLifeTime() const { return 0.f; }
	virtual float	GetGravity() const { return 0.f; }
	virtual float	GetDrag() const { return 0.f; }
	virtual Vector	GetAdditionalVelocity( const tf_point_t *pPoint ) const { return vec3_origin; }

	virtual float	GetRadius( const tf_point_t *pPoint ) const { return 0.f; }

	virtual int	GetMaxPoints() const { return 0; }

	virtual bool ShouldIgnoreStartSolid( void ) { return false; }

	const TFPointVec_t& GetPointVec() const { return m_vecPoints; }

	bool CanAddPoint() const { return m_vecPoints.Count() < GetMaxPoints(); }
	void RemovePoint( int nPointIndex );

	mutable CUniformRandomStream m_randomStream;

private:
	tf_point_t* AddPointInternal( int nPointIndex );

	CNetworkVar( int, m_nRandomSeed );
	CNetworkArray( int, m_nSpawnTime, MAX_POINT_MANAGER_POINTS );
	CNetworkVar( uint32, m_unNextPointIndex );
#ifdef CLIENT_DLL
	uint32 m_unClientNextPointIndex = 0;
#endif // CLIENT_DLL
	float m_flLastUpdateTime = 0.f;

	TFPointVec_t m_vecPoints;
};


#endif // TF_POINT_MANAGER_H
