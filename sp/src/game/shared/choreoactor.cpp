//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "choreoactor.h"
#include "choreochannel.h"
#include "choreoscene.h"
#include "tier1/utlbuffer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CChoreoActor::CChoreoActor( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CChoreoActor::CChoreoActor( const char *name )
{
	Init();
	SetName( name );
}

//-----------------------------------------------------------------------------
// Purpose: // Assignment
// Input  : src - 
// Output : CChoreoActor&
//-----------------------------------------------------------------------------
CChoreoActor& CChoreoActor::operator=( const CChoreoActor& src )
{
	m_bActive = src.m_bActive;

	Q_strncpy( m_szName, src.m_szName, sizeof( m_szName ) );
	Q_strncpy( m_szFacePoserModelName, src.m_szFacePoserModelName, sizeof( m_szFacePoserModelName ) );

	for ( int i = 0; i < src.m_Channels.Size(); i++ )
	{
		CChoreoChannel *c = src.m_Channels[ i ];
		CChoreoChannel *newChannel = new CChoreoChannel();
		newChannel->SetActor( this );
		*newChannel = *c;
		AddChannel( newChannel );
	}

	return *this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoActor::Init( void )
{
	m_szName[ 0 ] = 0;
	m_szFacePoserModelName[ 0 ] = 0;
	m_bActive = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
void CChoreoActor::SetName( const char *name )
{
	assert( strlen( name ) < MAX_ACTOR_NAME );
	Q_strncpy( m_szName, name, sizeof( m_szName ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoActor::GetName( void )
{
	return m_szName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoActor::GetNumChannels( void )
{
	return m_Channels.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : channel - 
// Output : CChoreoChannel
//-----------------------------------------------------------------------------
CChoreoChannel *CChoreoActor::GetChannel( int channel )
{
	if ( channel < 0 || channel >= m_Channels.Size() )
	{
		return NULL;
	}

	return m_Channels[ channel ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *channel - 
//-----------------------------------------------------------------------------
void CChoreoActor::AddChannel( CChoreoChannel *channel )
{
	m_Channels.AddToTail( channel );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *channel - 
//-----------------------------------------------------------------------------
void CChoreoActor::RemoveChannel( CChoreoChannel *channel )
{
	int idx = FindChannelIndex( channel );
	if ( idx == -1 )
		return;

	m_Channels.Remove( idx );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoActor::RemoveAllChannels()
{
	m_Channels.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : c1 - 
//			c2 - 
//-----------------------------------------------------------------------------
void CChoreoActor::SwapChannels( int c1, int c2 )
{
	CChoreoChannel *temp;

	temp = m_Channels[ c1 ];
	m_Channels[ c1 ] = m_Channels[ c2 ];
	m_Channels[ c2 ] = temp;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *channel - 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoActor::FindChannelIndex( CChoreoChannel *channel )
{
	for ( int i = 0; i < m_Channels.Size(); i++ )
	{
		if ( channel == m_Channels[ i ] )
		{
			return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
void CChoreoActor::SetFacePoserModelName( const char *name )
{
	Q_strncpy( m_szFacePoserModelName, name, sizeof( m_szFacePoserModelName ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *CChoreoActor::GetFacePoserModelName( void ) const
{
	return m_szFacePoserModelName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : active - 
//-----------------------------------------------------------------------------
void CChoreoActor::SetActive( bool active )
{
	m_bActive = active;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoActor::GetActive( void ) const
{
	return m_bActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoActor::MarkForSaveAll( bool mark )
{
	SetMarkedForSave( mark );

	int c = GetNumChannels();
	for ( int i = 0; i < c; i++ )
	{
		CChoreoChannel *channel = GetChannel( i );
		channel->MarkForSaveAll( mark );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : CChoreoChannel
//-----------------------------------------------------------------------------
CChoreoChannel *CChoreoActor::FindChannel( const char *name )
{
	int c = GetNumChannels();
	for ( int i = 0; i < c; i++ )
	{
		CChoreoChannel *channel = GetChannel( i );
		if ( !Q_stricmp( channel->GetName(), name ) )
			return channel;
	}

	return NULL;
}

void CChoreoActor::SaveToBuffer( CUtlBuffer& buf, CChoreoScene *pScene, IChoreoStringPool *pStringPool )
{
	buf.PutShort( pStringPool->FindOrAddString( GetName() ) );

	int c = GetNumChannels();
	Assert( c <= 255 );
	buf.PutUnsignedChar( c );

	for ( int i = 0; i < c; i++ )
	{
		CChoreoChannel *channel = GetChannel( i );
		Assert( channel );
		channel->SaveToBuffer( buf, pScene, pStringPool );
	}

	/*
	if ( Q_strlen( a->GetFacePoserModelName() ) > 0 )
	{
		FilePrintf( buf, level + 1, "faceposermodel \"%s\"\n", a->GetFacePoserModelName() );
	}
	*/
	buf.PutChar( GetActive() ? 1 : 0 );
}

bool CChoreoActor::RestoreFromBuffer( CUtlBuffer& buf, CChoreoScene *pScene, IChoreoStringPool *pStringPool )
{
	char sz[ 256 ];
	pStringPool->GetString( buf.GetShort(), sz, sizeof( sz ) );

	SetName( sz );

	int i;
	int c = buf.GetUnsignedChar();
	for ( i = 0; i < c; i++ )
	{
		CChoreoChannel *channel = pScene->AllocChannel();
		Assert( channel );
		if ( channel->RestoreFromBuffer( buf, pScene, this, pStringPool ) )
		{
			AddChannel( channel );
			channel->SetActor( this );
			continue;
		}

		return false;
	}

	SetActive( buf.GetChar() == 1 ? true : false );

	return true;
}

