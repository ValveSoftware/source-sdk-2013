//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef TF_TANK_BOSS_H
#define TF_TANK_BOSS_H

#include "tf_population_manager.h"
#include "tf_base_boss.h"
#include "tf_tank_boss_body.h"
#include "tf_achievementdata.h"

//----------------------------------------------------------------------------
class CTFTankBoss : public CTFBaseBoss
{
public:
	DECLARE_CLASS( CTFTankBoss, CTFBaseBoss );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CTFTankBoss();
	virtual ~CTFTankBoss();

	virtual void Precache( void );
	virtual void Spawn( void );
	virtual void SetSkin( int nSkin ) { if ( m_body ) m_body->SetSkin( nSkin ); }

	virtual void UpdateOnRemove( void );

	virtual void UpdateCollisionBounds( void );

	virtual CTFTankBossBody *GetBodyInterface( void ) const { return m_body; }

	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &rawInfo );

	virtual void Event_Killed( const CTakeDamageInfo &info );

	void TankBossThink( void );

	void SetStartingPathTrackNode( char *name );

	void DefineOnKilledOutput( EventInfo *eventInfo );
	void DefineOnBombDroppedOutput( EventInfo *eventInfo );

	void SetWaveSpawnPopulator( CWaveSpawnPopulator *pWave ){ m_pWaveSpawnPopulator = pWave; }

	virtual int GetCurrencyValue( void );

	// Input handlers
	void InputDestroyIfAtCapturePoint( inputdata_t &inputdata );
	void InputAddCaptureDestroyPostfix( inputdata_t &inputdata );

	void UpdatePingSound( void );

protected:
	virtual void ModifyDamage( CTakeDamageInfo *info ) const;

private:

	void Explode();

private:
	CTFTankBossBody *m_body;

	CHandle< CPathTrack > m_startNode;
	CHandle< CPathTrack > m_endNode;
	CHandle< CPathTrack > m_goalNode;
	CUtlVector< float > m_CumulativeDistances;
	float m_fTotalDistance;
	int m_nNodeNumber;

	float m_flSpawnTime;

	bool m_isDroppingBomb;
	float m_flDroppingStart;

	int m_exhaustAttachment;
	bool m_isSmoking;

	bool m_bIsPlayerKilled;
	bool m_bPlayedHalfwayAlert;
	bool m_bPlayedNearAlert;

	int m_lastHealth;
	int m_damageModelIndex;
	int m_nDeathAnimPick;
	char m_szDeathPostfix[ 8 ];

	Vector m_lastRightTrackPos;
	Vector m_lastLeftTrackPos;

	CountdownTimer m_rumbleTimer;

	EventInfo m_onKilledEventInfo;
	EventInfo m_onBombDroppedEventInfo;
	void FirePopFileEvent( EventInfo *eventInfo );

	CHandle< CBaseAnimating > m_bomb;
	CHandle< CBaseAnimating > m_leftTracks;
	CHandle< CBaseAnimating > m_rightTracks;

	CountdownTimer m_crushTimer;
	CWaveSpawnPopulator *m_pWaveSpawnPopulator;

	Vector m_vCollisionMins;
	Vector m_vCollisionMaxs;

	float m_flLastPingTime;

	static float m_flLastTankAlert;

	CHistoryVector< EntityHistory_t, CEntityHistoryLess, 12 > m_vecDamagers;
};


inline void CTFTankBoss::DefineOnKilledOutput( EventInfo *eventInfo )
{
	if ( eventInfo )
	{
		m_onKilledEventInfo.m_action = eventInfo->m_action;
		m_onKilledEventInfo.m_target = eventInfo->m_target;
	}
}

inline void CTFTankBoss::DefineOnBombDroppedOutput( EventInfo *eventInfo )
{
	if ( eventInfo )
	{
		m_onBombDroppedEventInfo.m_action = eventInfo->m_action;
		m_onBombDroppedEventInfo.m_target = eventInfo->m_target;
	}
}

#endif // TF_TANK_BOSS_H
