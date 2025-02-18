//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"
#include "c_monster_resource.h"
#include "tf_hud_boss_health.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


void RecvProxy_UpdateBossHud( const CRecvProxyData *pData, void *pStruct, void *pOut );


IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_MonsterResource, DT_MonsterResource, CMonsterResource )

	RecvPropInt( RECVINFO( m_iBossHealthPercentageByte ), 0, RecvProxy_UpdateBossHud ),
	RecvPropInt( RECVINFO( m_iBossStunPercentageByte ), 0, RecvProxy_UpdateBossHud ),

	RecvPropInt( RECVINFO( m_iSkillShotCompleteCount ) ),
	RecvPropTime( RECVINFO( m_fSkillShotComboEndTime ) ),

	RecvPropInt( RECVINFO( m_iBossState ), 0, RecvProxy_UpdateBossHud ),

END_RECV_TABLE()

C_MonsterResource *g_pMonsterResource = NULL;


//-----------------------------------------------------------------------------
// Update the HUD meter when the Boss' data changes
void RecvProxy_UpdateBossHud( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	int *out = (int *)pOut;

	*out = pData->m_Value.m_Int;

	CHudBossHealthMeter *meter = GET_HUDELEMENT( CHudBossHealthMeter );
	if ( meter )
	{
		meter->Update();
	}
}



//-----------------------------------------------------------------------------
C_MonsterResource::C_MonsterResource()
{
	m_iBossHealthPercentageByte = 0;
	m_iBossStunPercentageByte = 0;

	m_iSkillShotCompleteCount = 0;
	m_fSkillShotComboEndTime = 0;

	m_iBossState = 0;

	// do this here because entity is created via network messages from the server entity's creation
	Assert( g_pMonsterResource == NULL );
	g_pMonsterResource = this;
}


//-----------------------------------------------------------------------------
C_MonsterResource::~C_MonsterResource()
{
	Assert( g_pMonsterResource == this );
	g_pMonsterResource = NULL;
}


//-----------------------------------------------------------------------------
float C_MonsterResource::GetBossHealthPercentage( void )
{
	return (float)m_iBossHealthPercentageByte / 255.0f;
}


//-----------------------------------------------------------------------------
float C_MonsterResource::GetBossStunPercentage( void )
{
	return (float)m_iBossStunPercentageByte / 255.0f;
}
