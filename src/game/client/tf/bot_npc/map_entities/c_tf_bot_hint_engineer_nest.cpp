//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"
#include "c_tf_bot_hint_engineer_nest.h"

IMPLEMENT_CLIENTCLASS_DT(C_TFBotHintEngineerNest, DT_TFBotHintEngineerNest, CTFBotHintEngineerNest)
	RecvPropBool( RECVINFO(m_bHasActiveTeleporter) ),
END_RECV_TABLE()

//------------------------------------------------------------------------------
C_TFBotHintEngineerNest::C_TFBotHintEngineerNest( void )
{
	m_bHasActiveTeleporter = false;
	m_bHadActiveTeleporter = false;
	m_pMvMActiveTeleporter = NULL;
}


C_TFBotHintEngineerNest::~C_TFBotHintEngineerNest()
{

}


void C_TFBotHintEngineerNest::UpdateOnRemove()
{
	StopEffect();
	BaseClass::UpdateOnRemove();
}


void C_TFBotHintEngineerNest::OnPreDataChanged( DataUpdateType_t type )
{
	BaseClass::OnPreDataChanged( type );

	m_bHadActiveTeleporter = m_bHasActiveTeleporter;
}


void C_TFBotHintEngineerNest::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( m_bHadActiveTeleporter != m_bHasActiveTeleporter )
	{
		if ( m_bHasActiveTeleporter )
		{
			StartEffect();
		}
		else
		{
			StopEffect();
		}
	}
}


void C_TFBotHintEngineerNest::StartEffect()
{
	if ( !m_pMvMActiveTeleporter )
	{
		m_pMvMActiveTeleporter = ParticleProp()->Create( "teleporter_mvm_bot_persist", PATTACH_ABSORIGIN );
	}
}


void C_TFBotHintEngineerNest::StopEffect()
{
	if ( m_pMvMActiveTeleporter )
	{
		ParticleProp()->StopEmission( m_pMvMActiveTeleporter );
		m_pMvMActiveTeleporter = NULL;
	}
}
