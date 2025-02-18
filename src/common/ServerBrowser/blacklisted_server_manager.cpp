//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

//#include "..\..\serverbrowser\pch_serverbrowser.h"

#undef _snprintf	// needed since matchmakingtypes.h inlines a bare _snprintf

#include "convar.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "steam/steamclientpublic.h"
#include "steam/matchmakingtypes.h"
#include <time.h>
#include "blacklisted_server_manager.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBlacklistedServerManager::CBlacklistedServerManager()
{
	m_iNextServerID = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Reset the list of blacklisted servers to empty.
//-----------------------------------------------------------------------------
void CBlacklistedServerManager::Reset( void )
{
	m_Blacklist.RemoveAll();
	m_iNextServerID = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Appends all the servers inside the specified file to the blacklist.
// Returns count of appended servers, zero for failure.
//-----------------------------------------------------------------------------
int CBlacklistedServerManager::LoadServersFromFile( const char *pszFilename, bool bResetTimes )
{
	KeyValues *pKV = new KeyValues( "serverblacklist" );
	if ( !pKV->LoadFromFile( g_pFullFileSystem, pszFilename, "MOD" ) )
		return 0;

	int count = 0;

	for ( KeyValues *pData = pKV->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey() )
	{
		const char *pszName = pData->GetString( "name" );

		uint32 ulDate = pData->GetInt( "date" );
		if ( bResetTimes )
		{
			time_t today;
			time( &today );
			ulDate = today;
		}

		const char *pszNetAddr = pData->GetString( "addr" );
		if ( pszNetAddr && pszNetAddr[0] && pszName && pszName[0] )
		{
			int iIdx = m_Blacklist.AddToTail();

			m_Blacklist[iIdx].m_nServerID = m_iNextServerID++;
			V_strncpy( m_Blacklist[iIdx].m_szServerName, pszName, sizeof( m_Blacklist[iIdx].m_szServerName ) );
			m_Blacklist[iIdx].m_ulTimeBlacklistedAt = ulDate;
			m_Blacklist[iIdx].m_NetAdr.SetFromString( pszNetAddr );

			++count;
		}
	}

	pKV->deleteThis();

	return count;
}


//-----------------------------------------------------------------------------
// Purpose: save blacklist to disk
//-----------------------------------------------------------------------------
void CBlacklistedServerManager::SaveToFile( const char *pszFilename )
{
	KeyValues *pKV = new KeyValues( "serverblacklist" );

	for ( int i = 0; i < m_Blacklist.Count(); i++ )
	{
		KeyValues *pSubKey = new KeyValues( "server" );
		pSubKey->SetString( "name", m_Blacklist[i].m_szServerName );
		pSubKey->SetInt( "date", m_Blacklist[i].m_ulTimeBlacklistedAt );
		pSubKey->SetString( "addr", m_Blacklist[i].m_NetAdr.ToString() );
		pKV->AddSubKey( pSubKey );
	}

	pKV->SaveToFile( g_pFullFileSystem, pszFilename, "MOD" );

	pKV->deleteThis();
}


//-----------------------------------------------------------------------------
// Purpose: Add the given server to the blacklist. Return added server.
//-----------------------------------------------------------------------------
blacklisted_server_t *CBlacklistedServerManager::AddServer( gameserveritem_t &server )
{
	// Make sure we don't already have this IP in the list somewhere
	netadr_t netAdr( server.m_NetAdr.GetIP(), server.m_NetAdr.GetConnectionPort() );

	// Don't let them add reserved addresses to their blacklists
 	if ( netAdr.IsReservedAdr() )
 		return NULL;

	int iIdx = m_Blacklist.AddToTail();
	V_strncpy( m_Blacklist[iIdx].m_szServerName, server.GetName(), sizeof( m_Blacklist[iIdx].m_szServerName ) );

	time_t today;
	time( &today );
	m_Blacklist[iIdx].m_ulTimeBlacklistedAt = today;
	m_Blacklist[iIdx].m_NetAdr = netAdr;
	m_Blacklist[iIdx].m_nServerID = m_iNextServerID++;

	return &m_Blacklist[iIdx];
}


//-----------------------------------------------------------------------------
// Purpose: Add the given server to the blacklist. Return added server.
//-----------------------------------------------------------------------------
blacklisted_server_t *CBlacklistedServerManager::AddServer( const char *serverName, uint32 serverIP, int serverPort )
{
	netadr_t netAdr( serverIP, serverPort );

	// Don't let them add reserved addresses to their blacklists
 	if ( netAdr.IsReservedAdr() )
 		return NULL;

	int iIdx = m_Blacklist.AddToTail();

	V_strncpy( m_Blacklist[iIdx].m_szServerName, serverName, sizeof( m_Blacklist[iIdx].m_szServerName ) );

	time_t today;
	time( &today );
	m_Blacklist[iIdx].m_ulTimeBlacklistedAt = today;

	m_Blacklist[iIdx].m_NetAdr = netAdr;
	m_Blacklist[iIdx].m_nServerID = m_iNextServerID++;

	return &m_Blacklist[iIdx];
}


//-----------------------------------------------------------------------------
// Purpose: Add the given server to the blacklist. Return added server.
//-----------------------------------------------------------------------------
blacklisted_server_t *CBlacklistedServerManager::AddServer( const char *serverName, const char *netAddressString, uint32 timestamp )
{
	netadr_t netAdr( netAddressString );

	// Don't let them add reserved addresses to their blacklists
	if ( netAdr.IsReservedAdr() )
		return NULL;

	int iIdx = m_Blacklist.AddToTail();

	V_strncpy( m_Blacklist[iIdx].m_szServerName, serverName, sizeof( m_Blacklist[iIdx].m_szServerName ) );
	m_Blacklist[iIdx].m_ulTimeBlacklistedAt = timestamp;
	m_Blacklist[iIdx].m_NetAdr = netAdr;
	m_Blacklist[iIdx].m_nServerID = m_iNextServerID++;

	return &m_Blacklist[iIdx];
}


//-----------------------------------------------------------------------------
// Purpose: Remove server with matching 'server id' from list
//-----------------------------------------------------------------------------
void CBlacklistedServerManager::RemoveServer( int iServerID )
{
	for ( int i = 0; i < m_Blacklist.Count(); i++ )
	{
		if ( m_Blacklist[i].m_nServerID == iServerID )
		{
			m_Blacklist.Remove(i);
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Given a serverID, return its blacklist entry
//-----------------------------------------------------------------------------
blacklisted_server_t *CBlacklistedServerManager::GetServer( int iServerID )
{
	for ( int i = 0; i < m_Blacklist.Count(); i++ )
	{
		if ( m_Blacklist[i].m_nServerID == iServerID )
			return &m_Blacklist[i];
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if given server is blacklisted
//-----------------------------------------------------------------------------
bool CBlacklistedServerManager::IsServerBlacklisted( const gameserveritem_t &server ) const
{
	return IsServerBlacklisted( server.m_NetAdr.GetIP(), server.m_NetAdr.GetConnectionPort(), server.GetName() );
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if given server is blacklisted
//-----------------------------------------------------------------------------
bool CBlacklistedServerManager::IsServerBlacklisted( uint32 serverIP, int serverPort, const char *serverName ) const
{
	netadr_t netAdr( serverIP, serverPort );

	ConVarRef sb_showblacklists( "sb_showblacklists" );

	for ( int i = 0; i < m_Blacklist.Count(); i++ )
	{
		if ( m_Blacklist[i].m_NetAdr.ip[3] == 0 )
		{
			if ( m_Blacklist[i].m_NetAdr.CompareClassCAdr( netAdr ) )
			{
				if ( sb_showblacklists.IsValid() && sb_showblacklists.GetBool() )
				{
					Msg( "Blacklisted '%s' (%s), due to rule '%s' (Class C).\n", serverName, netAdr.ToString(), m_Blacklist[i].m_NetAdr.ToString() );
				}
				return true;
			}
		}
		else
		{
			if ( m_Blacklist[i].m_NetAdr.CompareAdr( netAdr, (m_Blacklist[i].m_NetAdr.GetPort() == 0) ) )
			{
				if ( sb_showblacklists.IsValid() && sb_showblacklists.GetBool() )
				{
					Msg( "Blacklisted '%s' (%s), due to rule '%s'.\n", serverName, netAdr.ToString(), m_Blacklist[i].m_NetAdr.ToString() );
				}
				return true;
			}
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the given server is allowed to be blacklisted at all
//-----------------------------------------------------------------------------
bool CBlacklistedServerManager::CanServerBeBlacklisted( gameserveritem_t &server ) const
{
	return CanServerBeBlacklisted( server.m_NetAdr.GetIP(), server.m_NetAdr.GetConnectionPort(), server.GetName() );
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the given server is allowed to be blacklisted at all
//-----------------------------------------------------------------------------
bool CBlacklistedServerManager::CanServerBeBlacklisted( uint32 serverIP, int serverPort, const char *serverName ) const
{
	netadr_t netAdr( serverIP, serverPort );

	if ( !netAdr.IsValid() )
		return false;

	// Don't let them add reserved addresses to their blacklists
	if ( netAdr.IsReservedAdr() )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Returns vector of blacklisted servers
//-----------------------------------------------------------------------------
const CUtlVector< blacklisted_server_t > &CBlacklistedServerManager::GetServerVector( void ) const
{
	return m_Blacklist;
}
