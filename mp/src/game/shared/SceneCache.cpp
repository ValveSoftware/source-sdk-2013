//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "SceneCache.h"
#include "choreoscene.h"
#include "choreoevent.h"

extern ISoundEmitterSystemBase *soundemitterbase;
CChoreoScene *BlockingLoadScene( const char *filename );

CSceneCache::CSceneCache()
{
	msecs = 0;
}

CSceneCache::CSceneCache( const CSceneCache& src )
{
	msecs  = src.msecs;
	sounds = src.sounds;
}

int	CSceneCache::GetSoundCount() const
{
	return sounds.Count();
}

char const *CSceneCache::GetSoundName( int index )
{
	return soundemitterbase->GetSoundName( sounds[ index ] );
}

void CSceneCache::Save( CUtlBuffer& buf  )
{
	buf.PutUnsignedInt( msecs );

	unsigned short c = GetSoundCount();
	buf.PutShort( c );
	
	Assert( sounds.Count() <= 65536 );

	for ( int i = 0; i < c; ++i )
	{
		buf.PutString( GetSoundName( i ) );
	}
}

void CSceneCache::Restore( CUtlBuffer& buf  )
{
	MEM_ALLOC_CREDIT();

	msecs = buf.GetUnsignedInt();

	unsigned short c = (unsigned short)buf.GetShort();

	for ( int i = 0; i < c; ++i )
	{
		char soundname[ 512 ];
		buf.GetString( soundname );

		int idx = soundemitterbase->GetSoundIndex( soundname );
		if ( idx != -1 )
		{
			Assert( idx <= 65535 );
			if ( sounds.Find( idx ) == sounds.InvalidIndex() )
			{
				sounds.AddToTail( (unsigned short)idx );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Static method
// Input  : *event - 
//			soundlist - 
//-----------------------------------------------------------------------------
void CSceneCache::PrecacheSceneEvent( CChoreoEvent *event, CUtlVector< unsigned short >& soundlist )
{
	if ( !event || event->GetType() != CChoreoEvent::SPEAK )
		return;

	int idx = soundemitterbase->GetSoundIndex( event->GetParameters() );
	if ( idx != -1 )
	{
		MEM_ALLOC_CREDIT();
		Assert( idx <= 65535 );
		soundlist.AddToTail( (unsigned short)idx );
	}

	if ( event->GetCloseCaptionType() == CChoreoEvent::CC_MASTER )
	{
		char tok[ CChoreoEvent::MAX_CCTOKEN_STRING ];
		if ( event->GetPlaybackCloseCaptionToken( tok, sizeof( tok ) ) )
		{
			int idx = soundemitterbase->GetSoundIndex( tok );
			if ( idx != -1 && soundlist.Find( idx ) == soundlist.InvalidIndex() )
			{
				MEM_ALLOC_CREDIT();
				Assert( idx <= 65535 );
				soundlist.AddToTail( (unsigned short)idx );
			}
		}
	}
}

void CSceneCache::Rebuild( char const *filename )
{
	msecs = 0;
	sounds.RemoveAll();

	CChoreoScene *scene = BlockingLoadScene( filename );
	if ( scene )
	{
		// Walk all events looking for SPEAK events
		CChoreoEvent *event;
		int c = scene->GetNumEvents();
		for ( int i = 0; i < c; ++i )
		{
			event = scene->GetEvent( i );
			PrecacheSceneEvent( event, sounds );
		}

		// Update scene duration, too
		msecs = (int)( scene->FindStopTime() * 1000.0f + 0.5f );

		delete scene;
	}
}
