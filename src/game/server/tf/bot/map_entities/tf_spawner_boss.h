//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_spawner_boss.h
// Entity to spawn a Boss
// Michael Booth, February 2011

#ifndef TF_SPAWNER_BOSS_H
#define TF_SPAWNER_BOSS_H

#ifdef OBSOLETE_USE_BOSS_ALPHA

#ifdef TF_RAID_MODE

class CBotNPC;

class CTFSpawnerBoss : public CPointEntity
{
public:
	DECLARE_CLASS( CTFSpawnerBoss, CPointEntity );
	DECLARE_DATADESC();

	CTFSpawnerBoss( void );
	virtual ~CTFSpawnerBoss() { }

	void SpawnerThink( void );

	// Input.
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	// Output
	void OnBotKilled( CBotNPC *pBot );
	void OnBotStunned( CBotNPC *pBot );

private:
	bool m_isExpended;
	int m_spawnCount;
	int m_spawnCountRemaining;
	int m_maxActiveCount;
	float m_spawnInterval;
	string_t m_teamName;

	COutputEvent m_onSpawned;
	COutputEvent m_onExpended;
	COutputEvent m_onBotKilled;
	COutputEvent m_onBotStunned;

	CUtlVector< CHandle< CBotNPC > > m_spawnedBotVector;
};

#endif // TF_RAID_MODE

#endif // OBSOLETE_USE_BOSS_ALPHA

#endif // TF_SPAWNER_BOSS_H
