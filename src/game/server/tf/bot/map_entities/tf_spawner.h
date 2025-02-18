//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_spawner.h
// Entity to spawn one or more templatized entities
// Michael Booth, April 2011

#ifndef TF_SPAWNER_H
#define TF_SPAWNER_H

//--------------------------------------------------------
/**
 * Each particular type of entity the tf_spawner can create
 * has an associated template (derived from this class)
 * which defines its spawning location and initial properties.
 */
class CTFSpawnTemplate : public CPointEntity
{
public:
	DECLARE_CLASS( CTFSpawnTemplate, CPointEntity );

	virtual ~CTFSpawnTemplate() { }

	virtual CBaseEntity *Instantiate( void ) const = 0;			// spawn an instance of this template
};


//--------------------------------------------------------
class CTFSpawner : public CPointEntity
{
public:
	DECLARE_CLASS( CTFSpawner, CPointEntity );
	DECLARE_DATADESC();

	CTFSpawner( void );
	virtual ~CTFSpawner() { }

	void SpawnerThink( void );

	// Input.
	void InputReset( inputdata_t &inputdata );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	// Output
	void OnKilled( CBaseEntity *dead );

private:
	void Reset( void );

	bool m_bExpended;
	int m_spawnCount;
	int m_spawnCountRemaining;
	int m_maxActiveCount;
	float m_spawnInterval;

	string_t m_templateName;
	CHandle< CTFSpawnTemplate > m_template;

	COutputEvent m_onSpawned;
	COutputEvent m_onExpended;
	COutputEvent m_onKilled;

	CUtlVector< CHandle< CBaseEntity > > m_spawnedVector;
};


#endif // TF_SPAWNER_H
