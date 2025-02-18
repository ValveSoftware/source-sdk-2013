//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef ZOMBIE_SPAWN_H
#define ZOMBIE_SPAWN_H

class CZombieSpawner : public CPointEntity
{
	DECLARE_CLASS( CZombieSpawner, CPointEntity );
	DECLARE_DATADESC();
public:
	CZombieSpawner();

	virtual void Spawn();
	virtual void Think();

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputSetMaxActiveZombies( inputdata_t &inputdata );

private:
	bool m_bEnabled;
	bool m_bInfiniteZombies;
	int m_nMaxActiveZombies;
	float m_flZombieLifeTime;
	int m_nSkeletonType;

	int m_nSpawned;

	CUtlVector< CHandle< CZombie > > m_activeZombies;
};

#endif // ZOMBIE_SPAWN_H
