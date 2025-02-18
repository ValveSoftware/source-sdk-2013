//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates objective data
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "clientmode_tf.h"
#include "c_tf_objective_resource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_TFObjectiveResource, DT_TFObjectiveResource, CTFObjectiveResource)
	RecvPropInt( RECVINFO(m_nMannVsMachineMaxWaveCount) ),
	RecvPropInt( RECVINFO(m_nMannVsMachineWaveCount) ),
	RecvPropInt( RECVINFO(m_nMannVsMachineWaveEnemyCount) ),
	RecvPropInt( RECVINFO(m_nMvMWorldMoney) ),
	RecvPropFloat( RECVINFO( m_flMannVsMachineNextWaveTime ) ),
	RecvPropBool( RECVINFO( m_bMannVsMachineBetweenWaves ) ),
	RecvPropInt( RECVINFO(m_nFlagCarrierUpgradeLevel) ),
	RecvPropFloat( RECVINFO( m_flMvMBaseBombUpgradeTime ) ),
	RecvPropFloat( RECVINFO( m_flMvMNextBombUpgradeTime ) ),
	RecvPropString( RECVINFO( m_iszMvMPopfileName ) ),
	RecvPropInt( RECVINFO(m_iChallengeIndex) ),
	RecvPropInt( RECVINFO(m_nMvMEventPopfileType) ),
	
	RecvPropArray3( RECVINFO_ARRAY( m_nMannVsMachineWaveClassCounts ), RecvPropInt( RECVINFO( m_nMannVsMachineWaveClassCounts[0] ) ) ),
	RecvPropArray( RecvPropString( RECVINFO( m_iszMannVsMachineWaveClassNames[0]) ), m_iszMannVsMachineWaveClassNames ),
	RecvPropArray3( RECVINFO_ARRAY( m_nMannVsMachineWaveClassFlags ), RecvPropInt( RECVINFO( m_nMannVsMachineWaveClassFlags[0] ) ) ),

	RecvPropArray3( RECVINFO_ARRAY( m_nMannVsMachineWaveClassCounts2 ), RecvPropInt( RECVINFO( m_nMannVsMachineWaveClassCounts2[0] ) ) ),
	RecvPropArray( RecvPropString( RECVINFO( m_iszMannVsMachineWaveClassNames2[0]) ), m_iszMannVsMachineWaveClassNames2 ),
	RecvPropArray3( RECVINFO_ARRAY( m_nMannVsMachineWaveClassFlags2 ), RecvPropInt( RECVINFO( m_nMannVsMachineWaveClassFlags2[0] ) ) ),

	RecvPropArray3( RECVINFO_ARRAY( m_bMannVsMachineWaveClassActive ), RecvPropBool( RECVINFO( m_bMannVsMachineWaveClassActive[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_bMannVsMachineWaveClassActive2 ), RecvPropBool( RECVINFO( m_bMannVsMachineWaveClassActive2[0] ) ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFObjectiveResource::C_TFObjectiveResource()
{
	PrecacheMaterial( "sprites/obj_icons/icon_obj_cap_blu" );
	PrecacheMaterial( "sprites/obj_icons/icon_obj_cap_blu_up" );
	PrecacheMaterial( "sprites/obj_icons/icon_obj_cap_red" );
	PrecacheMaterial( "sprites/obj_icons/icon_obj_cap_red_up" );
	PrecacheMaterial( "VGUI/flagtime_empty" );
	PrecacheMaterial( "VGUI/flagtime_full" );

	m_nMannVsMachineMaxWaveCount = 0;
	m_nMannVsMachineWaveCount = 0;
	m_nMannVsMachineWaveEnemyCount = 0;
	m_nMvMWorldMoney = 0;
	m_flMannVsMachineNextWaveTime = 0;
	m_bMannVsMachineBetweenWaves = false;
	m_nFlagCarrierUpgradeLevel = 0;
	m_iChallengeIndex = -1;
	m_nMvMEventPopfileType = MVM_EVENT_POPFILE_NONE;

	memset( m_nMannVsMachineWaveClassCounts, 0, sizeof( m_nMannVsMachineWaveClassCounts ) );
	memset( m_nMannVsMachineWaveClassCounts2, 0, sizeof( m_nMannVsMachineWaveClassCounts2 ) );
	memset( m_nMannVsMachineWaveClassFlags, MVM_CLASS_FLAG_NONE, sizeof( m_nMannVsMachineWaveClassFlags ) );
	memset( m_nMannVsMachineWaveClassFlags2, MVM_CLASS_FLAG_NONE, sizeof( m_nMannVsMachineWaveClassFlags2 ) );
	memset( m_bMannVsMachineWaveClassActive, 0, sizeof( m_bMannVsMachineWaveClassActive ) );
	memset( m_bMannVsMachineWaveClassActive2, 0, sizeof( m_bMannVsMachineWaveClassActive2 ) );

	int i = 0;
	for ( i = 0 ; i < ARRAYSIZE( m_iszMannVsMachineWaveClassNames ) ; ++i )
	{
		m_iszMannVsMachineWaveClassNames[ i ][ 0 ] = '\0';
	}

	for ( i = 0 ; i < ARRAYSIZE( m_iszMannVsMachineWaveClassNames2 ) ; ++i )
	{
		m_iszMannVsMachineWaveClassNames2[ i ][ 0 ] = '\0';
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFObjectiveResource::~C_TFObjectiveResource()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_TFObjectiveResource::GetGameSpecificCPCappingSwipe( int index_, int iCappingTeam )
{
	Assert( index_ < m_iNumControlPoints );
	Assert( iCappingTeam != TEAM_UNASSIGNED );

	if ( iCappingTeam == TF_TEAM_RED )
		return "sprites/obj_icons/icon_obj_cap_red";

	return "sprites/obj_icons/icon_obj_cap_blu";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_TFObjectiveResource::GetGameSpecificCPBarFG( int index_, int iOwningTeam )
{
	Assert( index_ < m_iNumControlPoints );

	if ( iOwningTeam == TF_TEAM_RED )
		return "progress_bar_red";

	if ( iOwningTeam == TF_TEAM_BLUE )
		return "progress_bar_blu";

	return "progress_bar";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_TFObjectiveResource::GetGameSpecificCPBarBG( int index_, int iCappingTeam )
{
	Assert( index_ < m_iNumControlPoints );
	Assert( iCappingTeam != TEAM_UNASSIGNED );

	if ( iCappingTeam == TF_TEAM_RED )
		return "progress_bar_red";

	return "progress_bar_blu";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFObjectiveResource::SetCappingTeam( int index_, int team )
{
	//Display warning that someone is capping our point.
	//Only do this at the start of a cap and if WE own the point.
	//Also don't warn on a point that will do a "Last Point cap" warning.
	if ( GetNumControlPoints() > 0 && GetCapWarningLevel( index_ ) == CP_WARN_NORMAL && GetCPCapPercentage( index_ ) == 0.0f && team != TEAM_UNASSIGNED )
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer )
		{
			if ( pLocalPlayer->GetTeamNumber() != team )
			{
				CLocalPlayerFilter filter;
				if ( GetOwningTeam( index_ ) != TEAM_UNASSIGNED )
				{
					C_BaseEntity::EmitSound( filter, -1, "Announcer.ControlPointContested" );
				}
				else
				{
					C_BaseEntity::EmitSound( filter, -1, "Announcer.ControlPointContested_Neutral" );
				}
			}
		}
	}

	BaseClass::SetCappingTeam( index_, team );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFObjectiveResource::GetMannVsMachineWaveClassCount( int nIndex )
{
	if ( nIndex < ARRAYSIZE( m_nMannVsMachineWaveClassCounts ) )
	{
		return m_nMannVsMachineWaveClassCounts[ nIndex ]; 
	}
	nIndex -= ARRAYSIZE( m_nMannVsMachineWaveClassCounts );

	if ( nIndex < ARRAYSIZE( m_nMannVsMachineWaveClassCounts2 ) )
	{
		return m_nMannVsMachineWaveClassCounts2[ nIndex ]; 
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_TFObjectiveResource::GetMannVsMachineWaveClassName( int nIndex )
{ 
	if ( nIndex < ARRAYSIZE( m_iszMannVsMachineWaveClassNames ) )
	{
		return m_iszMannVsMachineWaveClassNames[ nIndex ]; 
	}
	nIndex -= ARRAYSIZE( m_iszMannVsMachineWaveClassNames );

	if ( nIndex < ARRAYSIZE( m_iszMannVsMachineWaveClassNames2 ) )
	{
		return m_iszMannVsMachineWaveClassNames2[ nIndex ]; 
	}

	return "";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
unsigned int C_TFObjectiveResource::GetMannVsMachineWaveClassFlags( int nIndex ) 
{ 
	if ( nIndex < ARRAYSIZE( m_nMannVsMachineWaveClassFlags ) )
	{
		return m_nMannVsMachineWaveClassFlags[ nIndex ]; 
	}
	nIndex -= ARRAYSIZE( m_nMannVsMachineWaveClassFlags );

	if ( nIndex < ARRAYSIZE( m_nMannVsMachineWaveClassFlags2 ) )
	{
		return m_nMannVsMachineWaveClassFlags2[ nIndex ]; 
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFObjectiveResource::GetMannVsMachineWaveClassActive( int nIndex )
{
	if ( nIndex < ARRAYSIZE( m_bMannVsMachineWaveClassActive ) )
	{
		return m_bMannVsMachineWaveClassActive[ nIndex ]; 
	}
	nIndex -= ARRAYSIZE( m_bMannVsMachineWaveClassActive );

	if ( nIndex < ARRAYSIZE( m_bMannVsMachineWaveClassActive2 ) )
	{
		return m_bMannVsMachineWaveClassActive2[ nIndex ]; 
	}

	return 0;
}
