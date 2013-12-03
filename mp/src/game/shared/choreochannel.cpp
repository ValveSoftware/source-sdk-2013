//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "choreochannel.h"
#include "choreoevent.h"
#include "choreoscene.h"
#include "utlrbtree.h"
#include "tier1/utlbuffer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CChoreoChannel::CChoreoChannel( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CChoreoChannel::CChoreoChannel(const char *name )
{
	Init();
	SetName( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Assignment
// Input  : src - 
//-----------------------------------------------------------------------------
CChoreoChannel&	CChoreoChannel::operator=( const CChoreoChannel& src )
{
	m_bActive = src.m_bActive;
	Q_strncpy( m_szName, src.m_szName, sizeof( m_szName ) );
	for ( int i = 0; i < src.m_Events.Size(); i++ )
	{
		CChoreoEvent *e = src.m_Events[ i ];
		CChoreoEvent *newEvent = new CChoreoEvent( e->GetScene() );
		*newEvent = *e;
		AddEvent( newEvent );
		newEvent->SetChannel( this );
		newEvent->SetActor( m_pActor );
	}

	return *this;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
void CChoreoChannel::SetName( const char *name )
{
	assert( Q_strlen( name ) < MAX_CHANNEL_NAME );
	Q_strncpy( m_szName, name, sizeof( m_szName ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CChoreoChannel::GetName( void )
{
	return m_szName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoChannel::GetNumEvents( void )
{
	return m_Events.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : event - 
// Output : CChoreoEvent
//-----------------------------------------------------------------------------
CChoreoEvent *CChoreoChannel::GetEvent( int event )
{
	if ( event < 0 || event >= m_Events.Size() )
	{
		return NULL;
	}

	return m_Events[ event ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CChoreoChannel::AddEvent( CChoreoEvent *event )
{
	m_Events.AddToTail( event );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CChoreoChannel::RemoveEvent( CChoreoEvent *event )
{
	int idx = FindEventIndex( event );
	if ( idx == -1 )
		return;

	m_Events.Remove( idx );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoChannel::RemoveAllEvents()
{
	m_Events.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoChannel::FindEventIndex( CChoreoEvent *event )
{
	for ( int i = 0; i < m_Events.Size(); i++ )
	{
		if ( event == m_Events[ i ] )
		{
			return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoChannel::Init( void )
{
	m_szName[ 0 ] = 0;
	SetActor( NULL );
	m_bActive = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoActor
//-----------------------------------------------------------------------------
CChoreoActor *CChoreoChannel::GetActor( void )
{
	return m_pActor;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void CChoreoChannel::SetActor( CChoreoActor *actor )
{
	m_pActor = actor;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : active - 
//-----------------------------------------------------------------------------
void CChoreoChannel::SetActive( bool active )
{
	m_bActive = active;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoChannel::GetActive( void ) const
{
	return m_bActive;
}

static bool ChoreEventStartTimeLessFunc( CChoreoEvent * const &p1, CChoreoEvent * const &p2 )
{
	CChoreoEvent *e1;
	CChoreoEvent *e2;

	e1 = const_cast< CChoreoEvent * >( p1 );
	e2 = const_cast< CChoreoEvent * >( p2 );

	return e1->GetStartTime() < e2->GetStartTime();
}

void CChoreoChannel::ReconcileGestureTimes()
{
	// Sort gesture events within channel by starting time
	CUtlRBTree< CChoreoEvent * >  sortedGestures( 0, 0, ChoreEventStartTimeLessFunc );
	int i;
	// Sort items
	int c = GetNumEvents();
	for ( i = 0; i < c; i++ )
	{
		CChoreoEvent *e = GetEvent( i );
		Assert( e );
		if ( e->GetType() != CChoreoEvent::GESTURE )
			continue;

		sortedGestures.Insert( e );
	}

	// Now walk list of gestures
	if ( !sortedGestures.Count() )
		return;

	CChoreoEvent *previous = NULL;

	for ( i = sortedGestures.FirstInorder(); i != sortedGestures.InvalidIndex(); i = sortedGestures.NextInorder( i ) )
	{
		CChoreoEvent *event = sortedGestures[ i ];

		if ( !previous )
		{
			// event->SetStartTime( 0.0f );
		}
		else if ( previous->GetSyncToFollowingGesture() )
		{
			// TODO: ask the sequence for what tags to match

			CEventAbsoluteTag *pEntryTag = event->FindEntryTag( CChoreoEvent::PLAYBACK );
			CEventAbsoluteTag *pExitTag = previous->FindExitTag( CChoreoEvent::PLAYBACK );

			if (pEntryTag && pExitTag)
			{
				float entryTime = pEntryTag->GetAbsoluteTime( );

				// get current decay rate of previous gesture
				float duration = previous->GetDuration();
				float decayTime = (1.0 - pExitTag->GetPercentage()) * duration;

				// adjust the previous gestures end time to current apex + existing decay rate
				previous->RescaleGestureTimes( previous->GetStartTime(), entryTime + decayTime, true );
				previous->SetEndTime( entryTime + decayTime );

				// set the previous gestures end tag to the current apex
				pExitTag->SetAbsoluteTime( entryTime );

				event->PreventTagOverlap( );
				previous->PreventTagOverlap( );
			}
			// BUG: Tracker 3298:  ywb 1/31/04
			// I think this fixes the issue with abutting past NULL gestures on paste:
			// Here's the bug report:
			// -------------------------
			// When copying and pasteing posture and gesture clips in face poser the beginings of the clips stretch 
			//  to the begining of the scene even if there is a null gesture in place at the begining.
			// -------------------------
			/*
			else if ( pEntryTag && !Q_stricmp( previous->GetName(), "NULL" ) )
			{
				// If the previous was a null event, then do a bit of fixup
				event->SetStartTime( previous->GetEndTime() );

				event->PreventTagOverlap( );
			}
			*/

			// The previous event decays from it's end dispaly end time to the current event's display start time
			// The next event starts just after the display end time of the previous event
		}

		previous = event;
	}

	if ( previous )
	{
		CChoreoScene *scene = previous->GetScene();
		if ( scene )
		{
			// HACK:  Could probably do better by allowing user to drag the blue "end time" bar
			//float finish = scene->FindStopTime();
			//previous->RescaleGestureTimes( previous->GetStartTime(), finish );
			//previous->SetEndTime( finish );
		}
	}

	/*
	c = 0;
	for ( i = sortedGestures.FirstInorder(); i != sortedGestures.InvalidIndex(); i = sortedGestures.NextInorder( i ) )
	{
		CChoreoEvent *event = sortedGestures[ i ];

		Msg( "event %i start %f disp %f dispend %f end %f\n",
			c + 1,
			event->GetStartTime( CChoreoEvent::SIMULATION ),
			event->GetStartTime( CChoreoEvent::DISPLAY ),
			event->GetEndTime( CChoreoEvent::DISPLAY ),
			event->GetEndTime( CChoreoEvent::SIMULATION )
		);
		c++;
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoChannel::MarkForSaveAll( bool mark )
{
	SetMarkedForSave( mark );

	int c = GetNumEvents();
	for ( int i = 0; i < c; i++ )
	{
		CChoreoEvent *e = GetEvent( i );
		e->SetMarkedForSave( mark );
	}
}


struct EventGroup
{
	EventGroup() :
		timeSortedEvents( 0, 0, ChoreEventStartTimeLessFunc )
	{
	}

	EventGroup( const EventGroup& src )
		:
		timeSortedEvents( 0, 0, ChoreEventStartTimeLessFunc )
	{
		timeSortedEvents.RemoveAll();
		int i = src.timeSortedEvents.FirstInorder();
		while ( i != src.timeSortedEvents.InvalidIndex() )
		{
			timeSortedEvents.Insert( src.timeSortedEvents[ i ] );

			i = src.timeSortedEvents.NextInorder( i );
		}
	}

	EventGroup & operator=( const EventGroup& src )
	{
		if ( this == &src )
			return *this;

		timeSortedEvents.RemoveAll();
		int i = src.timeSortedEvents.FirstInorder();
		while ( i != src.timeSortedEvents.InvalidIndex() )
		{
			timeSortedEvents.Insert( src.timeSortedEvents[ i ] );

			i = src.timeSortedEvents.NextInorder( i );
		}
		return *this;
	}

	CUtlRBTree< CChoreoEvent * > timeSortedEvents;
};

// Compute master/slave, count, endtime info for close captioning data
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoChannel::ReconcileCloseCaption()
{
	// Create a dictionary based on the combined token name
	CUtlDict< EventGroup, int > validSpeakEventsGroupedByName;

	int i;
	// Sort items
	int c = GetNumEvents();
	for ( i = 0; i < c; i++ )
	{
		CChoreoEvent *e = GetEvent( i );
		Assert( e );
		if ( e->GetType() != CChoreoEvent::SPEAK )
			continue;

		CChoreoEvent::CLOSECAPTION type;

		type = e->GetCloseCaptionType();
		if ( type == CChoreoEvent::CC_DISABLED )
		{
			e->SetUsingCombinedFile( false );
			e->SetRequiredCombinedChecksum( 0 );
			e->SetNumSlaves( 0 );
			e->SetLastSlaveEndTime( 0.0f );
			continue;
		}

		char const *name = e->GetCloseCaptionToken();
		if ( !name || !name[0] )
		{
			// Fixup invalid slave tag
			if ( type == CChoreoEvent::CC_SLAVE )
			{
				e->SetCloseCaptionType( CChoreoEvent::CC_MASTER );
				e->SetUsingCombinedFile( false );
				e->SetRequiredCombinedChecksum( 0 );
				e->SetNumSlaves( 0 );
				e->SetLastSlaveEndTime( 0.0f );
			}
			continue;
		}
	
		int idx = validSpeakEventsGroupedByName.Find( name );
		if ( idx == validSpeakEventsGroupedByName.InvalidIndex() )
		{
			EventGroup eg;
			eg.timeSortedEvents.Insert( e );
			validSpeakEventsGroupedByName.Insert( name, eg );
		}
		else
		{
			EventGroup & eg = validSpeakEventsGroupedByName[ idx ];
			eg.timeSortedEvents.Insert( e );
		}
	}

	c = validSpeakEventsGroupedByName.Count();
	// Now walk list of events by group
	if ( !c )
	{
		return;
	}

	for ( i = 0; i < c; ++i )
	{
		EventGroup & eg = validSpeakEventsGroupedByName[ i ];
		int sortedEventInGroup = eg.timeSortedEvents.Count();
		// If there's only one, just mark it valid
		if ( sortedEventInGroup <= 1 )
		{
			CChoreoEvent *e = eg.timeSortedEvents[ 0 ];
			Assert( e );
			// Make sure it's the master
			e->SetCloseCaptionType( CChoreoEvent::CC_MASTER );
			// Since it's by itself, can't be using "combined" file
			e->SetUsingCombinedFile( false );
			e->SetRequiredCombinedChecksum( 0 );
			e->SetNumSlaves( 0 );
			e->SetLastSlaveEndTime( 0.0f );
			continue;
		}

		// Okay, read them back in of start time
		int j = eg.timeSortedEvents.FirstInorder();
		CChoreoEvent *master = NULL;
		while ( j != eg.timeSortedEvents.InvalidIndex() )
		{
			CChoreoEvent *e = eg.timeSortedEvents[ j ];
			if ( !master )
			{
				master = e;
				e->SetCloseCaptionType( CChoreoEvent::CC_MASTER );
				//e->SetUsingCombinedFile( true );
				e->SetRequiredCombinedChecksum( 0 );
				e->SetNumSlaves( sortedEventInGroup - 1 );
				e->SetLastSlaveEndTime( e->GetEndTime() );
			}
			else
			{
				// Keep bumping out the end time
				master->SetLastSlaveEndTime( e->GetEndTime() );
				e->SetCloseCaptionType( CChoreoEvent::CC_SLAVE );
				e->SetUsingCombinedFile( master->IsUsingCombinedFile() );
				e->SetRequiredCombinedChecksum( 0 );
				e->SetLastSlaveEndTime( 0.0f );
			}

			j = eg.timeSortedEvents.NextInorder( j );
		}
	}
}

bool CChoreoChannel::GetSortedCombinedEventList( char const *cctoken, CUtlRBTree< CChoreoEvent * >& events )
{
	events.RemoveAll();

	int i;
	// Sort items
	int c = GetNumEvents();
	for ( i = 0; i < c; i++ )
	{
		CChoreoEvent *e = GetEvent( i );
		Assert( e );
		if ( e->GetType() != CChoreoEvent::SPEAK )
			continue;

		if ( e->GetCloseCaptionType() == CChoreoEvent::CC_DISABLED )
			continue;

		// A master with no slaves is not a combined event
		if ( e->GetCloseCaptionType() == CChoreoEvent::CC_MASTER &&
			 e->GetNumSlaves() == 0 )
			 continue;

		char const *token = e->GetCloseCaptionToken();
		if ( Q_stricmp( token, cctoken ) )
			continue;

		events.Insert( e );
	}

	return ( events.Count() > 0 ) ? true : false;
}

void CChoreoChannel::SaveToBuffer( CUtlBuffer& buf, CChoreoScene *pScene, IChoreoStringPool *pStringPool )
{
	buf.PutShort( pStringPool->FindOrAddString( GetName() ) );

	int c = GetNumEvents();
	Assert( c <= 255 );
	buf.PutUnsignedChar( c );

	for ( int i = 0; i < c; i++ )
	{
		CChoreoEvent *e = GetEvent( i );
		Assert( e );
		e->SaveToBuffer( buf, pScene, pStringPool );
	}

	buf.PutChar( GetActive() ? 1 : 0 );
}

bool CChoreoChannel::RestoreFromBuffer( CUtlBuffer& buf, CChoreoScene *pScene, CChoreoActor *pActor, IChoreoStringPool *pStringPool )
{
	char sz[ 256 ];
	pStringPool->GetString( buf.GetShort(), sz, sizeof( sz ) );
	SetName( sz );

	int numEvents = (int)buf.GetUnsignedChar();
	for ( int i = 0 ; i < numEvents; ++i )
	{
		CChoreoEvent *e = pScene->AllocEvent();
		if ( e->RestoreFromBuffer( buf, pScene, pStringPool ) )
		{
			AddEvent( e );
			e->SetChannel( this );
			e->SetActor( pActor );
			continue;
		}
		return false;
	}

	SetActive( buf.GetChar() == 1 ? true : false );

	return true;
}
