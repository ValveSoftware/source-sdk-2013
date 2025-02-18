//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=============================================================================
#include "cbase.h"
#include "tf_team.h"
#include "entitylist.h"
#include "util.h"
#include "tf_obj.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Purpose: SendProxy that converts the UtlVector list of objects to entindexes, where it's reassembled on the client
//-----------------------------------------------------------------------------
void SendProxy_TeamObjectList( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CTFTeam *pTeam = (CTFTeam*)pStruct;

	Assert( iElement < pTeam->GetNumObjects() );

	CBaseObject *pObject = pTeam->GetObject(iElement);

	EHANDLE hObject;
	hObject = pObject;

	SendProxy_EHandleToInt( pProp, pStruct, &hObject, pOut, iElement, objectID );
}

int SendProxyArrayLength_TeamObjects( const void *pStruct, int objectID )
{
	CTFTeam *pTeam = (CTFTeam*)pStruct;
	int iObjects = pTeam->GetNumObjects();
	return iObjects;
}

//=============================================================================
//
// TF Team tables.
//
IMPLEMENT_SERVERCLASS_ST( CTFTeam, DT_TFTeam )

	SendPropInt( SENDINFO( m_nFlagCaptures ), 8 ),
	SendPropInt( SENDINFO( m_iRole ), 4, SPROP_UNSIGNED ),

	SendPropArray2( 
	SendProxyArrayLength_TeamObjects,
	SendPropInt( "team_object_array_element", 0, SIZEOF_IGNORE, NUM_NETWORKED_EHANDLE_BITS, SPROP_UNSIGNED, SendProxy_TeamObjectList ), 
	MAX_PLAYERS * MAX_OBJECTS_PER_PLAYER, 
	0, 
	"team_object_array"
	),

	SendPropEHandle( SENDINFO( m_hLeader ) ),

END_SEND_TABLE()


LINK_ENTITY_TO_CLASS( tf_team, CTFTeam );

//=============================================================================
//
// TF Team Manager Functions.
//
CTFTeamManager s_TFTeamManager;

CTFTeamManager *TFTeamMgr()
{
	return &s_TFTeamManager;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFTeamManager::CTFTeamManager()
{
	m_UndefinedTeamColor.r = 255;
	m_UndefinedTeamColor.g = 255;
	m_UndefinedTeamColor.b = 255;
	m_UndefinedTeamColor.a = 0;

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFTeamManager::Init( void )
{
	// Clear the list.
	Shutdown();

	// Create the team list.
	for ( int iTeam = 0; iTeam < TF_TEAM_COUNT; ++iTeam )
	{
		COMPILE_TIME_ASSERT( TF_TEAM_COUNT == ARRAYSIZE( g_aTeamNames ) );
		COMPILE_TIME_ASSERT( TF_TEAM_COUNT == ARRAYSIZE( g_aTeamColors ) );
		int index = Create( g_aTeamNames[iTeam], g_aTeamColors[iTeam] );
		Assert( index == iTeam );
		if ( index != iTeam )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFTeamManager::Shutdown( void )
{
	// Note, don't delete each team since they are in the gEntList and will 
	// automatically be deleted from there, instead.
	g_Teams.Purge();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFTeamManager::Create( const char *pName, color32 color )
{
	CTeam *pTeam = static_cast<CTeam*>( CreateEntityByName( "tf_team" ) );
	if ( pTeam )
	{
		// Add the team to the global list of teams.
		int iTeam = g_Teams.AddToTail( pTeam );

		// Initialize the team.
		pTeam->Init( pName, iTeam );
		pTeam->NetworkProp()->SetUpdateInterval( 0.75f );

		// Set the team color.
		CTFTeam *pTFTeam = static_cast<CTFTeam*>( pTeam );
		pTFTeam->SetColor( color );

		return iTeam;
	}

	// Error.
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFTeamManager::GetFlagCaptures( int iTeam )
{
	if ( !IsValidTeam( iTeam ) )
		return -1;

	CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
	if ( !pTeam )
		return -1;

	return pTeam->GetFlagCaptures();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFTeamManager::IncrementFlagCaptures( int iTeam )
{
	if ( !IsValidTeam( iTeam ) )
		return;

	CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
	if ( !pTeam )
		return;

	pTeam->IncrementFlagCaptures();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFTeamManager::AddTeamScore( int iTeam, int iScoreToAdd )
{
	if ( !IsValidTeam( iTeam ) )
		return;

	CTeam *pTeam = GetGlobalTeam( iTeam );
	if ( !pTeam )
		return;

	pTeam->AddScore( iScoreToAdd );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFTeamManager::IsValidTeam( int iTeam )
{
	if ( ( iTeam >= 0 ) && ( iTeam < g_Teams.Count() ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int	CTFTeamManager::GetTeamCount( void )
{
	return g_Teams.Count();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFTeam *CTFTeamManager::GetTeam( int iTeam )
{
	Assert( ( iTeam >= 0 ) && ( iTeam < g_Teams.Count() ) );
	if ( IsValidTeam( iTeam ) )
	{
		return static_cast<CTFTeam*>( g_Teams[iTeam] );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFTeam *CTFTeamManager::GetSpectatorTeam()
{
	return GetTeam( 0 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
color32 CTFTeamManager::GetUndefinedTeamColor( void )
{
	return m_UndefinedTeamColor;
}

//-----------------------------------------------------------------------------
// Purpose: Sends a message to the center of the player's screen.
//-----------------------------------------------------------------------------
void CTFTeamManager::PlayerCenterPrint( CBasePlayer *pPlayer, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
	ClientPrint( pPlayer, HUD_PRINTCENTER, msg_name, param1, param2, param3, param4 );
}

//-----------------------------------------------------------------------------
// Purpose: Sends a message to the center of the given teams screen.
//-----------------------------------------------------------------------------
void CTFTeamManager::TeamCenterPrint( int iTeam, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
	CTeamRecipientFilter filter( iTeam, true );
	UTIL_ClientPrintFilter( filter, HUD_PRINTCENTER, msg_name, param1, param2, param3, param4 );
}

//-----------------------------------------------------------------------------
// Purpose: Sends a message to the center of the player's teams screen (minus
//          the player).
//-----------------------------------------------------------------------------
void CTFTeamManager::PlayerTeamCenterPrint( CBasePlayer *pPlayer, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
	CTeamRecipientFilter filter( pPlayer->GetTeamNumber(), true );
	filter.RemoveRecipient( pPlayer );
	UTIL_ClientPrintFilter( filter, HUD_PRINTCENTER, msg_name, param1, param2, param3, param4 );
}

//=============================================================================
//
// TF Team Functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFTeam::CTFTeam()
{
	m_TeamColor.r = 0;
	m_TeamColor.g = 0;
	m_TeamColor.b = 0;
	m_TeamColor.a = 0;

	m_nFlagCaptures = 0;
	m_nTotalFlagCaptures = 0;
	m_flTotalSecondsKOTHPointOwned = 0.f;
	m_flTotalPLRTrackPercentTraveled = 0.f;

	m_hLeader = NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFTeam::SetColor( color32 color )
{
	m_TeamColor = color;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
color32 CTFTeam::GetColor( void )
{
	return m_TeamColor;
}

//-----------------------------------------------------------------------------
// Purpose:
//   Input: pPlayer - print to just that client, NULL = all clients
//-----------------------------------------------------------------------------
void CTFTeam::ShowScore( CBasePlayer *pPlayer )
{
	if ( pPlayer )
	{
		ClientPrint( pPlayer, HUD_PRINTNOTIFY,  UTIL_VarArgs( "Team %s: %d\n", GetName(), GetScore() ) );
	}
	else
	{
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "Team %s: %d\n", GetName(), GetScore() ) );
	}
}

//-----------------------------------------------------------------------------
// OBJECTS
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Add the specified object to this team.
//-----------------------------------------------------------------------------
void CTFTeam::AddObject( CBaseObject *pObject )
{
	TRACE_OBJECT( UTIL_VarArgs( "%0.2f CTFTeam::AddObject adding object %p:%s to team %s\n", gpGlobals->curtime, 
		pObject, pObject->GetClassname(), GetName() ) );

	bool alreadyInList = IsObjectOnTeam( pObject );
	Assert( !alreadyInList );
	if ( !alreadyInList )
	{
		m_aObjects.AddToTail( pObject );
	}

	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Returns true if the object is in the team's list of objects
//-----------------------------------------------------------------------------
bool CTFTeam::IsObjectOnTeam( CBaseObject *pObject ) const
{
	return ( m_aObjects.Find( pObject ) != -1 );
}

//-----------------------------------------------------------------------------
// Purpose: Remove this object from the team
//  Removes all references from all sublists as well
//-----------------------------------------------------------------------------
void CTFTeam::RemoveObject( CBaseObject *pObject )
{									   
	if ( m_aObjects.Count() <= 0 )
		return;

	if ( m_aObjects.Find( pObject ) != -1 )
	{
		TRACE_OBJECT( UTIL_VarArgs( "%0.2f CTFTeam::RemoveObject removing %p:%s from %s\n", gpGlobals->curtime, 
			pObject, pObject->GetClassname(), GetName() ) );

		m_aObjects.FindAndRemove( pObject );
	}
	else
	{
		TRACE_OBJECT( UTIL_VarArgs( "%0.2f CTFTeam::RemoveObject couldn't remove %p:%s from %s\n", gpGlobals->curtime, 
			pObject, pObject->GetClassname(), GetName() ) );
	}

	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFTeam::GetNumObjects( int iObjectType )
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
CBaseObject *CTFTeam::GetObject( int num )
{
	Assert( num >= 0 && num < m_aObjects.Count() );
	return m_aObjects[ num ];
}


//-----------------------------------------------------------------------------
// Purpose: Get a pointer to the specified TF team
//-----------------------------------------------------------------------------
CTFTeam *GetGlobalTFTeam( int iIndex )
{
	if ( iIndex < 0 || iIndex >= GetNumberOfTeams() )
		return NULL;

	return ( dynamic_cast< CTFTeam* >( g_Teams[iIndex] ) );
}

//-----------------------------------------------------------------------------
// Set the team leader
//-----------------------------------------------------------------------------
bool CTFTeam::SetTeamLeader( CBasePlayer *pPlayer )
{
	Assert ( pPlayer );

	// player must be on this team
	if ( m_aPlayers.Find(pPlayer) == m_aPlayers.InvalidIndex() )
	{
		Assert( !"can't set a player as leader of a team he's not on" );
		return false;
	}

	m_hLeader = pPlayer;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Get Leader
//-----------------------------------------------------------------------------
CBasePlayer *CTFTeam::GetTeamLeader( void )
{
	return m_hLeader.Get();
}

//-----------------------------------------------------------------------------
// Purpose: Add the specified player to this team. Remove them from their current team, if any.
//-----------------------------------------------------------------------------
void CTFTeam::AddPlayer( CBasePlayer *pPlayer )
{
	BaseClass::AddPlayer( pPlayer );

	if ( GetTeamLeader() == NULL )
	{
		SetTeamLeader( pPlayer );
	}

	TFGameRules()->TeamPlayerCountChanged( this );
}

//-----------------------------------------------------------------------------
// Purpose: Remove this player from the team
//-----------------------------------------------------------------------------
void CTFTeam::RemovePlayer( CBasePlayer *pPlayer )
{
	BaseClass::RemovePlayer( pPlayer );

	if ( pPlayer == m_hLeader.Get() )
	{
		m_hLeader = NULL;

		if ( m_aPlayers.Count() > 0 )
		{
			// pick a new leader randomly
			int iLeader = random->RandomInt( 0, m_aPlayers.Count()-1 );
			SetTeamLeader( m_aPlayers.Element(iLeader) );
		}
	}

	TFGameRules()->TeamPlayerCountChanged( this );
}