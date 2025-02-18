//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_objective_resource.h"
#include "shareddefs.h"
#include "player_vs_environment/tf_population_manager.h"
#include <coordsize.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern void SendProxy_StringT_To_String( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

// Datatable
IMPLEMENT_SERVERCLASS_ST( CTFObjectiveResource, DT_TFObjectiveResource )
	SendPropInt( SENDINFO(m_nMannVsMachineMaxWaveCount), 9, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nMannVsMachineWaveCount), 9, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nMannVsMachineWaveEnemyCount), 16, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nMvMWorldMoney), 16, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flMannVsMachineNextWaveTime ) ),
	SendPropBool( SENDINFO( m_bMannVsMachineBetweenWaves ) ),
	SendPropInt( SENDINFO(m_nFlagCarrierUpgradeLevel), 4, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flMvMBaseBombUpgradeTime ) ),
	SendPropFloat( SENDINFO( m_flMvMNextBombUpgradeTime ) ),
	SendPropStringT ( SENDINFO( m_iszMvMPopfileName ) ),
	SendPropInt( SENDINFO(m_iChallengeIndex), 16 ),
	SendPropInt( SENDINFO(m_nMvMEventPopfileType), 4, SPROP_UNSIGNED ),

	SendPropArray3( SENDINFO_ARRAY3( m_nMannVsMachineWaveClassCounts ), SendPropInt( SENDINFO_ARRAY( m_nMannVsMachineWaveClassCounts ), 16 ) ),
	SendPropArray( SendPropString( SENDINFO_ARRAY( m_iszMannVsMachineWaveClassNames ), 0, SendProxy_StringT_To_String ), m_iszMannVsMachineWaveClassNames ),
	SendPropArray3( SENDINFO_ARRAY3( m_nMannVsMachineWaveClassFlags ), SendPropInt( SENDINFO_ARRAY( m_nMannVsMachineWaveClassFlags ), 10, SPROP_UNSIGNED ) ),

	SendPropArray3( SENDINFO_ARRAY3( m_nMannVsMachineWaveClassCounts2 ), SendPropInt( SENDINFO_ARRAY( m_nMannVsMachineWaveClassCounts2 ), 16 ) ),
	SendPropArray( SendPropString( SENDINFO_ARRAY( m_iszMannVsMachineWaveClassNames2 ), 0, SendProxy_StringT_To_String ), m_iszMannVsMachineWaveClassNames2 ),
	SendPropArray3( SENDINFO_ARRAY3( m_nMannVsMachineWaveClassFlags2 ), SendPropInt( SENDINFO_ARRAY( m_nMannVsMachineWaveClassFlags2 ), 10, SPROP_UNSIGNED ) ),

	SendPropArray3( SENDINFO_ARRAY3( m_bMannVsMachineWaveClassActive ), SendPropBool( SENDINFO_ARRAY( m_bMannVsMachineWaveClassActive ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bMannVsMachineWaveClassActive2 ), SendPropBool( SENDINFO_ARRAY( m_bMannVsMachineWaveClassActive2 ) ) ),
END_SEND_TABLE()


BEGIN_DATADESC( CTFObjectiveResource )
	DEFINE_FIELD( m_nMannVsMachineMaxWaveCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_nMannVsMachineWaveCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_nMannVsMachineWaveEnemyCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_nMvMWorldMoney, FIELD_INTEGER ),
	DEFINE_FIELD( m_flMannVsMachineNextWaveTime, FIELD_TIME ),
	DEFINE_FIELD( m_bMannVsMachineBetweenWaves, FIELD_BOOLEAN ),

	DEFINE_AUTO_ARRAY( m_nMannVsMachineWaveClassCounts, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_iszMannVsMachineWaveClassNames, FIELD_STRING ),
	DEFINE_AUTO_ARRAY( m_nMannVsMachineWaveClassFlags, FIELD_INTEGER ),

	DEFINE_AUTO_ARRAY( m_nMannVsMachineWaveClassCounts2, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_iszMannVsMachineWaveClassNames2, FIELD_STRING ),
	DEFINE_AUTO_ARRAY( m_nMannVsMachineWaveClassFlags2, FIELD_INTEGER ),

	DEFINE_AUTO_ARRAY( m_bMannVsMachineWaveClassActive, FIELD_BOOLEAN ),
	DEFINE_AUTO_ARRAY( m_bMannVsMachineWaveClassActive2, FIELD_BOOLEAN ),
END_DATADESC()


LINK_ENTITY_TO_CLASS( tf_objective_resource, CTFObjectiveResource );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFObjectiveResource::CTFObjectiveResource()
{
	m_nMannVsMachineMaxWaveCount = 0;
	m_nMannVsMachineWaveCount = 0;
	m_nMannVsMachineWaveEnemyCount = 0;
	m_nMvMWorldMoney = 0;
	m_flMannVsMachineNextWaveTime = 0;
	m_bMannVsMachineBetweenWaves = false;
	m_nFlagCarrierUpgradeLevel = 0;
	m_flMvMBaseBombUpgradeTime = 0;
	m_flMvMNextBombUpgradeTime = 0;
	m_iChallengeIndex = -1;
	SetMvMPopfileName( MAKE_STRING( "" ) );
	m_nMvMEventPopfileType.Set( MVM_EVENT_POPFILE_NONE );

	int i = 0;
	for ( i = 0 ; i < m_nMannVsMachineWaveClassCounts.Count() ; ++i )
	{
		m_nMannVsMachineWaveClassCounts.GetForModify( i ) = 0;
	}
	
	for ( i = 0 ; i < m_nMannVsMachineWaveClassCounts2.Count() ; ++i )
	{
		m_nMannVsMachineWaveClassCounts2.GetForModify( i ) = 0;
	}

	for ( i = 0 ; i < m_nMannVsMachineWaveClassFlags.Count() ; ++i )
	{
		m_nMannVsMachineWaveClassFlags.GetForModify( i ) = MVM_CLASS_FLAG_NONE;
	}

	for ( i = 0 ; i < m_nMannVsMachineWaveClassFlags2.Count() ; ++i )
	{
		m_nMannVsMachineWaveClassFlags2.GetForModify( i ) = MVM_CLASS_FLAG_NONE;
	}

	for ( i = 0 ; i < m_iszMannVsMachineWaveClassNames.Count() ; ++i )
	{
		m_iszMannVsMachineWaveClassNames.GetForModify( i ) = NULL_STRING;
	}

	for ( i = 0 ; i < m_iszMannVsMachineWaveClassNames2.Count() ; ++i )
	{
		m_iszMannVsMachineWaveClassNames2.GetForModify( i ) = NULL_STRING;
	}

	for ( i = 0 ; i < m_bMannVsMachineWaveClassActive.Count() ; ++i )
	{
		m_bMannVsMachineWaveClassActive.GetForModify( i ) = false;
	}

	for ( i = 0 ; i < m_bMannVsMachineWaveClassActive2.Count() ; ++i )
	{
		m_bMannVsMachineWaveClassActive2.GetForModify( i ) = false;
	}

	m_teleporterString = AllocPooledString( "teleporter" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFObjectiveResource::~CTFObjectiveResource()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFObjectiveResource::Spawn( void )
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFObjectiveResource::ClearMannVsMachineWaveClassFlags( void )
{
	int i = 0;
	for ( i = 0 ; i < m_nMannVsMachineWaveClassFlags.Count() ; ++i )
	{
		m_nMannVsMachineWaveClassFlags.GetForModify( i ) = MVM_CLASS_FLAG_NONE;
	}

	for ( i = 0 ; i < m_nMannVsMachineWaveClassFlags2.Count() ; ++i )
	{
		m_nMannVsMachineWaveClassFlags2.GetForModify( i ) = MVM_CLASS_FLAG_NONE;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFObjectiveResource::AddMannVsMachineWaveClassFlags( int nIndex, unsigned int iFlags )
{
	if ( nIndex < m_nMannVsMachineWaveClassFlags.Count() )
	{
		m_nMannVsMachineWaveClassFlags.GetForModify( nIndex ) |= iFlags;
		return;
	}
	nIndex -= m_nMannVsMachineWaveClassFlags.Count(); 

	if ( nIndex < m_nMannVsMachineWaveClassFlags2.Count() )
	{
		m_nMannVsMachineWaveClassFlags2.GetForModify( nIndex ) |= iFlags;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFObjectiveResource::GetMannVsMachineWaveClassCount( int nIndex )
{ 
	if ( nIndex < m_nMannVsMachineWaveClassCounts.Count() )
	{
		return m_nMannVsMachineWaveClassCounts[ nIndex ];
	}
	nIndex -= m_nMannVsMachineWaveClassCounts.Count();

	if ( nIndex  < m_nMannVsMachineWaveClassCounts2.Count() )
	{
		return m_nMannVsMachineWaveClassCounts2[ nIndex ];
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFObjectiveResource::SetMannVsMachineWaveClassActive( string_t iszClassIconName, bool bActive /*= true*/ )
{
	for ( int i = 0 ; i < m_iszMannVsMachineWaveClassNames.Count() ; ++i )
	{
		if ( ( m_iszMannVsMachineWaveClassNames[ i ] == iszClassIconName ) )
		{
			m_bMannVsMachineWaveClassActive.GetForModify( i ) = bActive;
			return;
		}
	}

	for ( int i = 0 ; i < m_iszMannVsMachineWaveClassNames2.Count() ; ++i )
	{
		if ( ( m_iszMannVsMachineWaveClassNames2[ i ] == iszClassIconName ) )
		{
			m_bMannVsMachineWaveClassActive2.GetForModify( i ) = bActive;
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFObjectiveResource::SetMannVsMachineWaveClassCount( int nIndex, int nCount )
{
	if ( nIndex < m_nMannVsMachineWaveClassCounts.Count() )
	{
		m_nMannVsMachineWaveClassCounts.GetForModify( nIndex ) = nCount;
		return;
	}
	nIndex -= m_nMannVsMachineWaveClassCounts.Count();

	if ( nIndex < m_nMannVsMachineWaveClassCounts2.Count() )
	{
		m_nMannVsMachineWaveClassCounts2.GetForModify( nIndex ) = nCount;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFObjectiveResource::SetMannVsMachineWaveClassName( int nIndex, string_t iszClassIconName )
{
	if ( nIndex < m_iszMannVsMachineWaveClassNames.Count() )
	{
		m_iszMannVsMachineWaveClassNames.GetForModify( nIndex ) = iszClassIconName;
		return;
	}
	nIndex -= m_iszMannVsMachineWaveClassNames.Count();

	if ( nIndex < m_iszMannVsMachineWaveClassNames2.Count() )
	{
		m_iszMannVsMachineWaveClassNames2.GetForModify( nIndex ) = iszClassIconName;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFObjectiveResource::IncrementMannVsMachineWaveClassCount( string_t iszClassIconName, unsigned int iFlags )
{
	int i = 0;
	for ( i = 0 ; i < m_iszMannVsMachineWaveClassNames.Count() ; ++i )
	{
		if ( ( m_iszMannVsMachineWaveClassNames[ i ] == iszClassIconName ) && ( m_nMannVsMachineWaveClassFlags[ i ] & iFlags ) )
		{
			m_nMannVsMachineWaveClassCounts.GetForModify( i ) += 1;

			if ( m_nMannVsMachineWaveClassCounts[ i ] <= 0 )
			{
				m_nMannVsMachineWaveClassCounts.GetForModify( i ) = 1;
			}

			return;
		}
	}

	for ( i = 0 ; i < m_iszMannVsMachineWaveClassNames2.Count() ; ++i )
	{
		if ( ( m_iszMannVsMachineWaveClassNames2[ i ] == iszClassIconName ) && ( m_nMannVsMachineWaveClassFlags2[ i ] & iFlags ) )
		{
			m_nMannVsMachineWaveClassCounts2.GetForModify( i ) += 1;

			if ( m_nMannVsMachineWaveClassCounts2[ i ] <= 0 )
			{
				m_nMannVsMachineWaveClassCounts2.GetForModify( i ) = 1;
			}

			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFObjectiveResource::DecrementMannVsMachineWaveClassCount( string_t iszClassIconName, unsigned int iFlags )
{
	int i = 0;
	for ( i = 0 ; i < m_iszMannVsMachineWaveClassNames.Count() && i < m_nMannVsMachineWaveClassFlags.Count() && i < m_nMannVsMachineWaveClassCounts.Count() ; ++i )
	{
		if ( ( m_iszMannVsMachineWaveClassNames[ i ] == iszClassIconName ) && ( m_nMannVsMachineWaveClassFlags[ i ] & iFlags ) )
		{
			m_nMannVsMachineWaveClassCounts.GetForModify( i ) -= 1;

			if ( m_nMannVsMachineWaveClassCounts[ i ] < 0 )
			{
				m_nMannVsMachineWaveClassCounts.GetForModify( i ) = 0;
			}

			if ( !m_nMannVsMachineWaveClassCounts[ i ] )
			{
				SetMannVsMachineWaveClassActive( iszClassIconName, false );
			}

			return;
		}
	}

	for ( i = 0 ; i < m_iszMannVsMachineWaveClassNames2.Count() && i < m_nMannVsMachineWaveClassFlags2.Count() && i < m_nMannVsMachineWaveClassCounts2.Count() ; ++i )
	{
		if ( ( m_iszMannVsMachineWaveClassNames2[ i ] == iszClassIconName ) && ( m_nMannVsMachineWaveClassFlags2[ i ] & iFlags ) )
		{
			m_nMannVsMachineWaveClassCounts2.GetForModify( i ) -= 1;

			if ( m_nMannVsMachineWaveClassCounts2[ i ] < 0 )
			{
				m_nMannVsMachineWaveClassCounts2.GetForModify( i ) = 0;
			}

			if ( !m_nMannVsMachineWaveClassCounts2[ i ] )
			{
				SetMannVsMachineWaveClassActive( iszClassIconName, false );
			}

			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFObjectiveResource::IncrementTeleporterCount()
{
	IncrementMannVsMachineWaveClassCount( m_teleporterString, MVM_CLASS_FLAG_MISSION );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFObjectiveResource::DecrementTeleporterCount()
{
	DecrementMannVsMachineWaveClassCount( m_teleporterString, MVM_CLASS_FLAG_MISSION );
}
