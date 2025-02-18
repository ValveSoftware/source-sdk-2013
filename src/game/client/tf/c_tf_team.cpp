//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side C_TFTeam class
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "engine/IEngineSound.h"
#include "hud.h"
#include "recvproxy.h"
#include "c_tf_team.h"
#include "c_tf_player.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#include "c_tf_playerresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: RecvProxy that converts the Player's object UtlVector to entindexes
//-----------------------------------------------------------------------------
void RecvProxy_TeamObjectList( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_TFTeam *pPlayer = (C_TFTeam*)pStruct;
	CBaseHandle *pHandle = (CBaseHandle*)(&(pPlayer->m_aObjects[pData->m_iElement])); 
	RecvProxy_IntToEHandle( pData, pStruct, pHandle );
}

void RecvProxyArrayLength_TeamObjects( void *pStruct, int objectID, int currentArrayLength )
{
	C_TFTeam *pPlayer = (C_TFTeam*)pStruct;

	if ( pPlayer->m_aObjects.Count() != currentArrayLength )
	{
		pPlayer->m_aObjects.SetSize( currentArrayLength );
	}
}

IMPLEMENT_CLIENTCLASS_DT( C_TFTeam, DT_TFTeam, CTFTeam )

	RecvPropInt( RECVINFO( m_nFlagCaptures ) ),
	RecvPropInt( RECVINFO( m_iRole ) ),

	RecvPropArray2( 
	RecvProxyArrayLength_TeamObjects,
	RecvPropInt( "team_object_array_element", 0, SIZEOF_IGNORE, 0, RecvProxy_TeamObjectList ), 
	MAX_PLAYERS * MAX_OBJECTS_PER_PLAYER, 
	0, 
	"team_object_array"	),

	RecvPropEHandle( RECVINFO( m_hLeader ) ),

END_RECV_TABLE()

#define TEAM_THINK_RATE 0.5f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFTeam::C_TFTeam()
{
	m_nFlagCaptures = 0;
	m_bUsingCustomTeamName = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFTeam::~C_TFTeam()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFTeam::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( gpGlobals->curtime + TEAM_THINK_RATE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char* C_TFTeam::Get_Name( void )
{
	// Use Get_Localized_Name() instead
	AssertMsg( false, "Use Get_Localized_Name() instead" );
	return "";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFTeam::ClientThink()
{
	BaseClass::ClientThink();

	UpdateTeamName();
	SetNextClientThink( gpGlobals->curtime + TEAM_THINK_RATE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFTeam::UpdateTeamName( void )
{
	m_bUsingCustomTeamName = false;

	const wchar_t *pwzName = NULL;
	if ( TFGameRules() && TFGameRules()->IsInTournamentMode() && ( ( m_iTeamNum == TF_TEAM_RED ) || ( m_iTeamNum == TF_TEAM_BLUE ) ) )
	{
		if ( TFGameRules()->IsCompetitiveMode() )
		{
			if ( g_TF_PR && ( g_TF_PR->HasPremadeParties() || g_TF_PR->GetEventTeamStatus() ) )
			{
				wchar_t wszTempName[MAX_TEAM_NAME_LENGTH];
				wchar_t *pFormat = g_pVGuiLocalize->Find( "#TF_Team_PartyLeader" );
				if ( !pFormat )
				{
					pFormat = L"%s";
				}

				if ( g_TF_PR->GetEventTeamStatus() )
				{
					//	GetEventTeamStatus() returns a value in the following range
					// 	enum WarMatch
					// 	{
					// 		NOPE = 0;
					// 		INVADERS_ARE_PYRO = 1;
					// 		INVADERS_ARE_HEAVY = 2;
					// 	};
					const char *pszTeamName = ( m_iTeamNum == TF_TEAM_BLUE ) ? 
											  ( g_TF_PR->GetEventTeamStatus() == 1 ? "#TF_Pyro" : "#TF_HWGuy" ) :
											  ( g_TF_PR->GetEventTeamStatus() == 1 ? "#TF_HWGuy" : "#TF_Pyro" );
					wchar_t *pwzWarTeam = g_pVGuiLocalize->Find( pszTeamName );
					V_swprintf_safe( m_wzTeamname, pFormat, pwzWarTeam );
					m_bUsingCustomTeamName = true;
					return;
				}
				else
				{
					int iPlayerIndex = ( m_iTeamNum == TF_TEAM_RED ) ? g_TF_PR->GetPartyLeaderRedTeamIndex() : g_TF_PR->GetPartyLeaderBlueTeamIndex();
					if ( g_TF_PR->IsConnected( iPlayerIndex ) )
					{
						g_pVGuiLocalize->ConvertANSIToUnicode( UTIL_SafeName( g_TF_PR->GetPlayerName( iPlayerIndex ) ), wszTempName, sizeof( wszTempName ) );
						V_swprintf_safe( m_wzTeamname, pFormat, wszTempName );
						m_bUsingCustomTeamName = true;
						return;
					}
				}
			}
		}
		else
		{
			const char *pTemp = ( m_iTeamNum == TF_TEAM_BLUE ) ? mp_tournament_blueteamname.GetString() : mp_tournament_redteamname.GetString();
			if ( pTemp && pTemp[0] )
			{
				g_pVGuiLocalize->ConvertANSIToUnicode( pTemp, m_wzTeamname, sizeof( m_wzTeamname ) );
				return;
			}
		}
	}

	if ( m_iTeamNum == TF_TEAM_BLUE )
	{
		pwzName = g_pVGuiLocalize->Find( "#TF_BlueTeam_Name" );
		if ( !pwzName )
		{
			pwzName = L"BLU";
		}
	}
	else if ( m_iTeamNum == TF_TEAM_RED )
	{
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			pwzName = g_pVGuiLocalize->Find( "#TF_Defenders" );
			if ( !pwzName )
			{
				pwzName = L"DEFENDERS";
			}
		}
		else
		{
			pwzName = g_pVGuiLocalize->Find( "#TF_RedTeam_Name" );
			if ( !pwzName )
			{
				pwzName = L"RED";
			}
		}
	}
	else if ( m_iTeamNum == TEAM_SPECTATOR )
	{
		pwzName = g_pVGuiLocalize->Find( "#TF_Spectators" );
		if ( !pwzName )
		{
			pwzName = L"SPECTATORS";
		}
	}

	V_wcscpy_safe( m_wzTeamname, pwzName ? pwzName : L"" );
}

//-----------------------------------------------------------------------------
// Purpose: Get the C_TFTeam for the specified team number
//-----------------------------------------------------------------------------
C_TFTeam *GetGlobalTFTeam( int iTeamNumber )
{
	for ( int i = 0; i < g_Teams.Count(); i++ )
	{
		if ( g_Teams[i]->GetTeamNumber() == iTeamNumber )
			return ( dynamic_cast< C_TFTeam* >( g_Teams[i] ) );
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFTeam::GetNumObjects( int iObjectType )
{
	// Asking for a count of a specific object type?
	if ( iObjectType > 0 )
	{
		int iCount = 0;
		for ( int i = 0; i < GetNumObjects(); i++ )
		{
			CBaseObject *pObject = GetObject(i);
			if ( pObject && pObject->GetType() == iObjectType )
			{
				iCount++;
			}
		}
		return iCount;
	}

	return m_aObjects.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseObject *C_TFTeam::GetObject( int num )
{
	Assert( num >= 0 && num < m_aObjects.Count() );
	return m_aObjects[ num ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BasePlayer *C_TFTeam::GetTeamLeader( void )
{
	return m_hLeader.Get();
}