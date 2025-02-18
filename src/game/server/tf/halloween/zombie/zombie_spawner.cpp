//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "tf_shareddefs.h"
#include "zombie.h"
#include "zombie_spawner.h"

LINK_ENTITY_TO_CLASS( tf_zombie_spawner, CZombieSpawner );

BEGIN_DATADESC( CZombieSpawner )
	DEFINE_KEYFIELD( m_flZombieLifeTime, FIELD_FLOAT, "zombie_lifetime" ),
	DEFINE_KEYFIELD( m_nMaxActiveZombies, FIELD_INTEGER, "max_zombies" ),
	DEFINE_KEYFIELD( m_bInfiniteZombies, FIELD_BOOLEAN, "infinite_zombies" ),
	DEFINE_KEYFIELD( m_nSkeletonType, FIELD_INTEGER, "zombie_type" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMaxActiveZombies", InputSetMaxActiveZombies ),
END_DATADESC()

CZombieSpawner::CZombieSpawner()
{
	m_bEnabled = false;
	m_bInfiniteZombies = false;
	m_nMaxActiveZombies = 1;
	m_flZombieLifeTime = 0;
	m_nSkeletonType = 0;
	m_nSpawned = 0;
}


void CZombieSpawner::Spawn()
{
	BaseClass::Spawn();

	SetNextThink( gpGlobals->curtime );
}


void CZombieSpawner::Think()
{
	m_activeZombies.FindAndFastRemove( NULL );

	if ( m_bEnabled && ( ( m_bInfiniteZombies && m_activeZombies.Count() < m_nMaxActiveZombies ) || ( !m_bInfiniteZombies && m_nSpawned < m_nMaxActiveZombies ) ) )
	{
		int nZombieTeam = TF_TEAM_HALLOWEEN;
		int nSpawnerTeam = GetTeamNumber();
		if ( nSpawnerTeam == TF_TEAM_BLUE || nSpawnerTeam == TF_TEAM_RED )
		{
			nZombieTeam = nSpawnerTeam;
		}
		CZombie *pZombie = CZombie::SpawnAtPos( GetAbsOrigin(), m_flZombieLifeTime, nZombieTeam, NULL, (CZombie::SkeletonType_t)m_nSkeletonType );
		if ( pZombie )
		{
			m_nSpawned++;
			m_activeZombies.AddToTail( pZombie );
		}

		SetNextThink( gpGlobals->curtime + RandomFloat( 1.5f, 3.f ) );
		return;
	}

	SetNextThink( gpGlobals->curtime + 0.2f );
}


void CZombieSpawner::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;

	SetNextThink( gpGlobals->curtime );
}


void CZombieSpawner::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;
	m_nSpawned = 0;

	m_activeZombies.Purge();

	SetNextThink( gpGlobals->curtime );
}


void CZombieSpawner::InputSetMaxActiveZombies( inputdata_t &inputdata )
{
	m_nMaxActiveZombies = inputdata.value.Int();
}
