//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Entity Spawner
//
//=============================================================================
#ifndef TF_ENTITY_SPAWNER_H
#define TF_ENTITY_SPAWNER_H
#ifdef _WIN32
#pragma once
#endif

class CEntitySpawnPoint;
class CEntitySpawnManager;

class CEntitySpawnPoint : public CServerOnlyPointEntity, public IEntityListener
{
public:
	DECLARE_CLASS( CEntitySpawnPoint, CServerOnlyPointEntity );
	DECLARE_DATADESC();

	CEntitySpawnPoint() {}

	virtual void	Spawn( void );
	virtual void	UpdateOnRemove( void );
	bool			IsUsed( void ) { return (m_hMyEntity.Get() != NULL); }
	void			SetEntity( CBaseEntity* pEnt ) { m_hMyEntity = pEnt; }
	void			RespawnNotifyThink( void );

	virtual void	OnEntityDeleted( CBaseEntity* pEntity );

private:
	string_t		m_iszSpawnManagerName;
	float			m_flNodeFree;

	CHandle< CEntitySpawnManager >	m_hSpawnManager;
	CHandle< CBaseEntity > m_hMyEntity;
};

class CEntitySpawnManager : public CLogicalEntity
{
public:
	DECLARE_CLASS( CEntitySpawnManager, CLogicalEntity );
	DECLARE_DATADESC();

	CEntitySpawnManager() {}

	virtual void	Spawn( void );
	void			RegisterSpawnPoint( CEntitySpawnPoint* pNewPoint );
	virtual void	Activate( void );
	void			SpawnAllEntities( void );
	bool			SpawnEntity( void );
	int				GetRespawnTime( void ) { return m_iRespawnTime; }

private:
	int				GetRandomUnusedIndex( void );
	bool			SpawnEntityAt( int iIndex );

private:
	string_t		m_iszEntityName;
	int				m_iEntityCount;
	int				m_iRespawnTime;
	bool			m_bDropToGround;
	bool			m_bRandomRotation;

	int				m_iMaxSpawnedEntities;
	CUtlVector< CHandle< CEntitySpawnPoint > >	m_SpawnPoints;
};

#endif	//TF_ENTITY_SPAWNER_H
