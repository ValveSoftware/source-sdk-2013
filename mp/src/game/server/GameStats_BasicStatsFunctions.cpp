//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "gamestats.h"

void BasicGameStatsRecord_t::Clear()
{
	m_nCount = 0;
	m_nSeconds = 0;
	m_nCommentary = 0;
	m_nHDR = 0;
	m_nCaptions = 0;
	m_bSteam = true;
	m_bCyberCafe = false;
	Q_memset( m_nSkill, 0, sizeof( m_nSkill ) );
	m_nDeaths = 0;
}

void BasicGameStatsRecord_t::SaveToBuffer( CUtlBuffer &buf )
{
	buf.PutInt( m_nCount );
	buf.PutInt( m_nSeconds );
	buf.PutInt( m_nCommentary );
	buf.PutInt( m_nHDR );
	buf.PutInt( m_nCaptions );
	for ( int i = 0; i < 3; ++i )
	{
		buf.PutInt( m_nSkill[ i ] );
	}

	buf.PutChar( m_bSteam ? 1 : 0 );
	buf.PutChar( m_bCyberCafe ? 1 : 0 );
	buf.PutInt( m_nDeaths );
}

bool BasicGameStatsRecord_t::ParseFromBuffer( CUtlBuffer &buf, int iBufferStatsVersion )
{
	bool bret = true;
	m_nCount = buf.GetInt();

	if ( m_nCount > 100000 || m_nCount < 0 )
	{
		bret = false;
	}

	m_nSeconds = buf.GetInt();
	// Note, don't put the buf.GetInt() in the macro since it'll get evaluated twice!!!
	m_nSeconds = MAX( m_nSeconds, 0 );

	m_nCommentary = buf.GetInt();
	if ( m_nCommentary < 0 || m_nCommentary > 100000 )
	{
		bret = false;
	}

	m_nHDR = buf.GetInt();
	if ( m_nHDR < 0 || m_nHDR > 100000 )
	{
		bret = false;
	}

	m_nCaptions = buf.GetInt();
	if ( m_nCaptions < 0 || m_nCaptions > 100000 )
	{
		bret = false;
	}

	for ( int i = 0; i < 3; ++i )
	{
		m_nSkill[ i ] = buf.GetInt();
		if ( m_nSkill[ i ] < 0 || m_nSkill[ i ]  > 100000 )
		{
			bret = false;
		}
	}

	if ( iBufferStatsVersion > GAMESTATS_FILE_VERSION_OLD )
	{
		m_bSteam = buf.GetChar() ? true : false;
	}
	if ( iBufferStatsVersion > GAMESTATS_FILE_VERSION_OLD2 )
	{
		m_bCyberCafe = buf.GetChar() ? true : false;
	}
	if ( iBufferStatsVersion > GAMESTATS_FILE_VERSION_OLD5 )
	{
		m_nDeaths = buf.GetInt();
	}

	return bret;
}

void BasicGameStats_t::Clear()
{
	m_nSecondsToCompleteGame = 0;
	m_Summary.Clear();
	m_MapTotals.Purge();
}

void BasicGameStats_t::SaveToBuffer( CUtlBuffer& buf )
{
	buf.PutInt( m_nSecondsToCompleteGame );

	m_Summary.SaveToBuffer( buf );

	int c = m_MapTotals.Count();
	buf.PutInt( c );
	for ( int i = m_MapTotals.First(); i != m_MapTotals.InvalidIndex(); i = m_MapTotals.Next( i ) )
	{
		char const *name = m_MapTotals.GetElementName( i );
		BasicGameStatsRecord_t &rec = m_MapTotals[ i ];

		buf.PutString( name );
		rec.SaveToBuffer( buf );
	}

	buf.PutChar( (char)m_nHL2ChaptureUnlocked );	
	buf.PutChar( m_bSteam ? 1 : 0 );
	buf.PutChar( m_bCyberCafe ? 1 : 0 );
	buf.PutShort( (short)m_nDXLevel );
}

BasicGameStatsRecord_t *BasicGameStats_t::FindOrAddRecordForMap( char const *mapname )
{
	int idx = m_MapTotals.Find( mapname );
	if ( idx == m_MapTotals.InvalidIndex() )
	{
		idx = m_MapTotals.Insert( mapname );
	}

	return &m_MapTotals[ idx ];
}

bool BasicGameStats_t::ParseFromBuffer( CUtlBuffer& buf, int iBufferStatsVersion )
{
	bool bret = true;

	m_nSecondsToCompleteGame = buf.GetInt();
	if ( m_nSecondsToCompleteGame < 0 || m_nSecondsToCompleteGame > 10000000 )
	{
		bret = false;
	}

	m_Summary.ParseFromBuffer( buf, iBufferStatsVersion );
	int c = buf.GetInt();
	if ( c > 1024 || c < 0 )
	{
		bret = false;
	}

	for ( int i = 0; i < c; ++i )
	{
		char mapname[ 256 ];
		buf.GetString( mapname );

		BasicGameStatsRecord_t *rec = FindOrAddRecordForMap( mapname );
		bool valid= rec->ParseFromBuffer( buf, iBufferStatsVersion );
		if ( !valid )
		{
			bret = false;
		}
	}

	if ( iBufferStatsVersion >= GAMESTATS_FILE_VERSION_OLD2 )
	{
		m_nHL2ChaptureUnlocked = (int)buf.GetChar();	
		m_bSteam = buf.GetChar() ? true : false;
	}
	if ( iBufferStatsVersion > GAMESTATS_FILE_VERSION_OLD2 )
	{
		m_bCyberCafe = buf.GetChar() ? true : false;
	}
	if ( iBufferStatsVersion > GAMESTATS_FILE_VERSION_OLD3 )
	{
		m_nDXLevel = (int)buf.GetShort();
	}
	return bret;
}

