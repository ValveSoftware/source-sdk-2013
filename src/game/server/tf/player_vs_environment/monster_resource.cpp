//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for non-player AI characters
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "monster_resource.h"
#include <coordsize.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST_NOBASE( CMonsterResource, DT_MonsterResource )

	SendPropInt( SENDINFO( m_iBossHealthPercentageByte ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iBossStunPercentageByte ), 8, SPROP_UNSIGNED ),

	SendPropInt( SENDINFO( m_iSkillShotCompleteCount ), 3, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_fSkillShotComboEndTime ) ),

	SendPropInt( SENDINFO( m_iBossState ) ),

END_SEND_TABLE()


BEGIN_DATADESC( CMonsterResource )

	DEFINE_FIELD( m_iBossHealthPercentageByte, FIELD_INTEGER ),
	DEFINE_FIELD( m_iBossStunPercentageByte, FIELD_INTEGER ),

	DEFINE_FIELD( m_iSkillShotCompleteCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_fSkillShotComboEndTime, FIELD_TIME ),

	// Function Pointers
	DEFINE_FUNCTION( Update ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( monster_resource, CMonsterResource );

CMonsterResource *g_pMonsterResource = NULL;


//-----------------------------------------------------------------------------
void CMonsterResource::Spawn( void )
{
	SetThink( &CMonsterResource::Update );
	SetNextThink( gpGlobals->curtime );

	m_iBossHealthPercentageByte = 0;
	m_iBossStunPercentageByte = 0;
	m_iSkillShotCompleteCount = 0;
	m_fSkillShotComboEndTime = 0;
	m_iBossState = 0;
}


//-----------------------------------------------------------------------------
//
// The Player resource is always transmitted to clients
//
int CMonsterResource::UpdateTransmitState( void )
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}


//-----------------------------------------------------------------------------
void CMonsterResource::Update( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );

}


//-----------------------------------------------------------------------------
void CMonsterResource::SetBossHealthPercentage( float percentFull )
{
	m_iBossHealthPercentageByte = 255.0f * percentFull;
}


//-----------------------------------------------------------------------------
void CMonsterResource::HideBossHealthMeter( void )
{
	m_iBossHealthPercentageByte = 0;
	m_iBossStunPercentageByte = 0;
}


//-----------------------------------------------------------------------------
void CMonsterResource::SetBossStunPercentage( float percentFull )
{
	m_iBossStunPercentageByte = 255.0f * percentFull;
}


//-----------------------------------------------------------------------------
void CMonsterResource::HideBossStunMeter( void )
{
	m_iBossStunPercentageByte = 0;
}


//-----------------------------------------------------------------------------
void CMonsterResource::StartSkillShotComboMeter( float comboMaxDuration )
{
	m_fSkillShotComboEndTime = gpGlobals->curtime + comboMaxDuration;
	m_iSkillShotCompleteCount = 1;
}


//-----------------------------------------------------------------------------
void CMonsterResource::IncrementSkillShotComboMeter( void )
{
	m_iSkillShotCompleteCount = m_iSkillShotCompleteCount + 1;
}


//-----------------------------------------------------------------------------
void CMonsterResource::HideSkillShotComboMeter( void )
{
	m_iSkillShotCompleteCount = 0;
}
