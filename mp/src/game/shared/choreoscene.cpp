//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//


#if defined(_WIN32) && !defined(_X360)
#include <windows.h>
#endif

#include "basetypes.h"
#include <stdio.h>
#include "choreoscene.h"
#include "choreoevent.h"
#include "choreochannel.h"
#include "choreoactor.h"
#include "ichoreoeventcallback.h"
#include "iscenetokenprocessor.h"
#include "utlbuffer.h"
#include "filesystem.h"
#include "utlrbtree.h"
#include "mathlib/mathlib.h"
#include "tier1/strtools.h"
#include "tier2/tier2.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#pragma warning( disable : 4127 )

// Let scene linger for 1/4 second so blends can finish
#define SCENE_LINGER_TIME 0.25f

// The engine turns this to true in dlls/sceneentity.cpp at bool SceneCacheInit()!!!
bool	CChoreoScene::s_bEditingDisabled = false;

//-----------------------------------------------------------------------------
// Purpose: Creates scene from a file
// Input  : *filename - 
//			*pfn - 
// Output : CChoreoScene
//-----------------------------------------------------------------------------
CChoreoScene *ChoreoLoadScene( 	
	char const *filename,
	IChoreoEventCallback *callback, 
	ISceneTokenProcessor *tokenizer,
	void ( *pfn ) ( const char *fmt, ... ) )
{
	MEM_ALLOC_CREDIT();
	CChoreoScene *scene = new CChoreoScene( callback );
	Assert( scene );
	scene->ParseFromBuffer( filename, tokenizer );
	scene->SetPrintFunc( pfn );
	return scene;
}

bool IsBufferBinaryVCD( char *pBuffer, int bufferSize )
{	
	if ( bufferSize > 4 && *(int *)pBuffer == SCENE_BINARY_TAG )
	{
		return true;	
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Debug printout
// Input  : level - 
//			*fmt - 
//			... - 
//-----------------------------------------------------------------------------
void CChoreoScene::choreoprintf( int level, const char *fmt, ... )
{
	char string[ 2048 ];
	va_list argptr;
	va_start( argptr, fmt );
	Q_vsnprintf( string, sizeof(string), fmt, argptr );
	va_end( argptr );

	while ( level-- > 0 )
	{
		if (m_pfnPrint )
		{ 
			 (*m_pfnPrint)( "  " );
		} 
		else
		{
			 printf( "  " );
		}
		Msg( "  " );
	}

	if ( m_pfnPrint )
	{
		(*m_pfnPrint)( string );
	}
	else
	{
		printf( "%s", string );
	}
	
	Msg( "%s", string );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CChoreoScene::CChoreoScene( IChoreoEventCallback *callback )
{
	Init( callback );
}


//-----------------------------------------------------------------------------
// Purpose: // Assignment
// Input  : src - 
// Output : CChoreoScene&
//-----------------------------------------------------------------------------
CChoreoScene& CChoreoScene::operator=( const CChoreoScene& src )
{
	Init( src.m_pIChoreoEventCallback );

	// Delete existing
	int i;
	for ( i = 0; i < m_Actors.Size(); i++ )
	{
		CChoreoActor *a = m_Actors[ i ];
		Assert( a );
		delete a;
	}

	m_Actors.RemoveAll();

	for ( i = 0; i < m_Events.Size(); i++ )
	{
		CChoreoEvent *e = m_Events[ i ];
		Assert( e );
		delete e;
	}

	m_Events.RemoveAll();

	for ( i = 0 ; i < m_Channels.Size(); i++ )
	{
		CChoreoChannel *c = m_Channels[ i ];
		Assert( c );
		delete c;
	}

	m_Channels.RemoveAll();

	m_pTokenizer = src.m_pTokenizer;
	
	m_flCurrentTime = src.m_flCurrentTime;
	m_flStartLoopTime = src.m_flStartLoopTime;
	m_flStartTime = src.m_flStartTime;
	m_flEndTime	= src.m_flEndTime;
	m_flSoundSystemLatency = src.m_flSoundSystemLatency;
	m_pfnPrint = src.m_pfnPrint;
	m_flLastActiveTime = src.m_flLastActiveTime;
	m_pTokenizer = src.m_pTokenizer;
	m_bSubScene = src.m_bSubScene;
	m_nSceneFPS = src.m_nSceneFPS;
	m_bUseFrameSnap = src.m_bUseFrameSnap;
	m_bIgnorePhonemes = src.m_bIgnorePhonemes;

	// Now copy the object tree
	// First copy the global events

	for ( i = 0; i < src.m_Events.Size(); i++ )
	{
		CChoreoEvent *event = src.m_Events[ i ];
		if ( event->GetActor() == NULL )
		{
			MEM_ALLOC_CREDIT();

			// Copy it
			CChoreoEvent *newEvent = AllocEvent();
			*newEvent = *event;
		}
	}

	// Finally, push actors, channels, events onto global stacks
	for ( i = 0; i < src.m_Actors.Size(); i++ )
	{
		CChoreoActor *actor = src.m_Actors[ i ];
		CChoreoActor *newActor = AllocActor();
		*newActor = *actor;

		for ( int j = 0; j < newActor->GetNumChannels() ; j++ )
		{
			CChoreoChannel *ch = newActor->GetChannel( j ); 
			m_Channels.AddToTail( ch );

			for ( int k = 0; k < ch->GetNumEvents(); k++ )
			{
				CChoreoEvent *ev = ch->GetEvent( k );
				m_Events.AddToTail( ev );
				ev->SetScene( this );
			}
		}
	}

	Q_strncpy( m_szMapname, src.m_szMapname, sizeof( m_szMapname ) );

	m_SceneRamp = src.m_SceneRamp;

	m_TimeZoomLookup.RemoveAll();
	for ( i = 0; i < (int)src.m_TimeZoomLookup.Count(); i++ )
	{
		m_TimeZoomLookup.Insert( src.m_TimeZoomLookup.GetElementName( i ), src.m_TimeZoomLookup[ i ] );
	}

	Q_strncpy( m_szFileName, src.m_szFileName, sizeof( m_szFileName ) );

	m_nLastPauseEvent = src.m_nLastPauseEvent;
	m_flPrecomputedStopTime = src.m_flPrecomputedStopTime;

	m_bitvecHasEventOfType = src.m_bitvecHasEventOfType;

	return *this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoScene::Init( IChoreoEventCallback *callback )
{
	m_flPrecomputedStopTime = 0.0f;
	m_pTokenizer			= NULL;
	m_szMapname[ 0 ] = 0;

	m_flCurrentTime = 0.0f;
	m_flStartLoopTime = -1.f;
	m_flStartTime = 0.0f;
	m_flEndTime	= 0.0f;
	m_flSoundSystemLatency = 0.0f;
	m_pfnPrint = NULL;
	m_flLastActiveTime = 0.0f;
	m_flEarliestTime	= 0.0f;
	m_flLatestTime		= 0.0f;
	m_nActiveEvents = 0;

	m_pIChoreoEventCallback = callback;

	m_bSubScene = false;
	m_nSceneFPS		= DEFAULT_SCENE_FPS;
	m_bUseFrameSnap = false;
	m_szFileName[0] = 0;

	m_bIsBackground = false;
	m_bitvecHasEventOfType.ClearAll();
	m_nLastPauseEvent = -1;
	m_bIgnorePhonemes = false;
}

//-----------------------------------------------------------------------------
// Purpose: Destroy objects and queues
//-----------------------------------------------------------------------------
CChoreoScene::~CChoreoScene( void )
{
	int i;
	for ( i = 0; i < m_Actors.Size(); i++ )
	{
		CChoreoActor *a = m_Actors[ i ];
		Assert( a );
		delete a;
	}

	m_Actors.RemoveAll();

	for ( i = 0; i < m_Events.Size(); i++ )
	{
		CChoreoEvent *e = m_Events[ i ];
		Assert( e );
		delete e;
	}

	m_Events.RemoveAll();

	for ( i = 0 ; i < m_Channels.Size(); i++ )
	{
		CChoreoChannel *c = m_Channels[ i ];
		Assert( c );
		delete c;
	}

	m_Channels.RemoveAll();
}


//-----------------------------------------------------------------------------
// Ignore phonemes
//-----------------------------------------------------------------------------
void CChoreoScene::IgnorePhonemes( bool bIgnore )
{
	m_bIgnorePhonemes = bIgnore;
}

bool CChoreoScene::ShouldIgnorePhonemes() const
{
	return m_bIgnorePhonemes;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *callback - 
//-----------------------------------------------------------------------------
void CChoreoScene::SetEventCallbackInterface( IChoreoEventCallback *callback )
{
	m_pIChoreoEventCallback = callback;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : level - 
//			*e - 
//-----------------------------------------------------------------------------
void CChoreoScene::PrintEvent( int level, CChoreoEvent *e )
{
	choreoprintf( level, "event %s \"%s\"\n", CChoreoEvent::NameForType( e->GetType() ), e->GetName() );
	choreoprintf( level, "{\n" );
	choreoprintf( level + 1, "time %f %f\n", e->GetStartTime(), e->GetEndTime() );
	choreoprintf( level + 1, "param \"%s\"\n", e->GetParameters() );
	if ( strlen( e->GetParameters2() ) > 0 )
	{
		choreoprintf( level + 1, "param2 \"%s\"\n", e->GetParameters2() );
	}
	if ( strlen( e->GetParameters3() ) > 0 )
	{
		choreoprintf( level + 1, "param3 \"%s\"\n", e->GetParameters3() );
	}
	choreoprintf( level, "}\n" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : level - 
//			*c - 
//-----------------------------------------------------------------------------
void CChoreoScene::PrintChannel( int level, CChoreoChannel *c )
{
	choreoprintf( level, "channel \"%s\"\n", c->GetName() );
	choreoprintf( level, "{\n" );
	
	for ( int i = 0; i < c->GetNumEvents(); i++ )
	{
		CChoreoEvent *e = c->GetEvent( i );
		if ( e )
		{
			PrintEvent( level + 1, e );
		}
	}

	choreoprintf( level, "}\n" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : level - 
//			*a - 
//-----------------------------------------------------------------------------
void CChoreoScene::PrintActor( int level, CChoreoActor *a )
{
	choreoprintf( level, "actor \"%s\"\n", a->GetName() );
	choreoprintf( level, "{\n" );
	
	for ( int i = 0; i < a->GetNumChannels(); i++ )
	{
		CChoreoChannel *c = a->GetChannel( i );
		if ( c )
		{
			PrintChannel( level + 1, c );
		}
	}

	choreoprintf( level, "}\n\n" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoScene::Print( void )
{
	// Look for events that don't have actor/channel set
	int i;

	for ( i = 0 ; i < m_Events.Size(); i++ )
	{
		CChoreoEvent *e = m_Events[ i ];
		if ( e->GetActor() )
			continue;

		PrintEvent( 0, e );
	}

	for ( i = 0 ; i < m_Actors.Size(); i++ )
	{
		CChoreoActor *a = m_Actors[ i ];
		if ( !a )
			continue;

		PrintActor( 0, a );
	}
}

//-----------------------------------------------------------------------------
// Purpose: prints if m_pfnPrint is active
// Output : 
//-----------------------------------------------------------------------------

void CChoreoScene::SceneMsg( const char *pFormat, ... )
{
	char string[ 2048 ];
	va_list argptr;
	va_start( argptr, pFormat );
	Q_vsnprintf( string, sizeof(string), pFormat, argptr );
	va_end( argptr );

	if ( m_pfnPrint )
	{
		(*m_pfnPrint)( string );
	}
	else
	{
		Msg( "%s", string );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoEvent
//-----------------------------------------------------------------------------
CChoreoEvent *CChoreoScene::AllocEvent( void )
{
	MEM_ALLOC_CREDIT_CLASS();
	CChoreoEvent *e = new CChoreoEvent( this );
	Assert( e );
	m_Events.AddToTail( e );
	return e;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoChannel
//-----------------------------------------------------------------------------
CChoreoChannel *CChoreoScene::AllocChannel( void )
{
	MEM_ALLOC_CREDIT_CLASS();
	CChoreoChannel *c = new CChoreoChannel();
	Assert( c );
	m_Channels.AddToTail( c );
	return c;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoActor
//-----------------------------------------------------------------------------
CChoreoActor *CChoreoScene::AllocActor( void )
{
	MEM_ALLOC_CREDIT_CLASS();
	CChoreoActor *a = new CChoreoActor;
	Assert( a );
	m_Actors.AddToTail( a );
	return a;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : CChoreoActor
//-----------------------------------------------------------------------------
CChoreoActor *CChoreoScene::FindActor( const char *name )
{
	for ( int i = 0; i < m_Actors.Size(); i++ )
	{
		CChoreoActor *a = m_Actors[ i ];
		if ( !a )
			continue;

		if ( !Q_stricmp( a->GetName(), name ) )
			return a;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoScene::GetNumEvents( void )
{
	return m_Events.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : event - 
// Output : CChoreoEvent
//-----------------------------------------------------------------------------
CChoreoEvent *CChoreoScene::GetEvent( int event )
{
	if ( event < 0 || event >= m_Events.Size() )
		return NULL;

	return m_Events[ event ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoScene::GetNumActors( void )
{
	return m_Actors.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : actor - 
// Output : CChoreoActor
//-----------------------------------------------------------------------------
CChoreoActor *CChoreoScene::GetActor( int actor )
{
	if ( actor < 0 || actor >= GetNumActors() )
		return NULL;
	return m_Actors[ actor ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoScene::GetNumChannels( void )
{
	return m_Channels.Size();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : channel - 
// Output : CChoreoChannel
//-----------------------------------------------------------------------------
CChoreoChannel *CChoreoScene::GetChannel( int channel )
{
	if ( channel < 0 || channel >= GetNumChannels() )
		return NULL;
	return m_Channels[ channel ];
}

void CChoreoScene::ParseRamp( ISceneTokenProcessor *tokenizer, CChoreoEvent *e )
{
	e->GetRamp()->Parse( tokenizer, e );
}

void CChoreoScene::ParseSceneRamp( ISceneTokenProcessor *tokenizer, CChoreoScene *scene )
{
	scene->m_SceneRamp.Parse( tokenizer, scene );
}

void CCurveData::Parse( ISceneTokenProcessor *tokenizer, ICurveDataAccessor *data )
{
	Clear();

	tokenizer->GetToken( true );

	if ( !Q_stricmp( tokenizer->CurrentToken(), "leftedge" ) )
	{
		CChoreoScene::ParseEdgeInfo( tokenizer, &m_RampEdgeInfo[ 0 ] );
	}

	if ( !Q_stricmp( tokenizer->CurrentToken(), "rightedge" ) )
	{
		CChoreoScene::ParseEdgeInfo( tokenizer, &m_RampEdgeInfo[ 1 ] );
	}

	if ( stricmp( tokenizer->CurrentToken(), "{" ) )
		tokenizer->Error( "expecting {\n" );
	
	while ( 1 )
	{
		// Parse until }
		tokenizer->GetToken( true );
		
		if ( strlen( tokenizer->CurrentToken() ) <= 0 )
		{
			tokenizer->Error( "expecting ramp data\n" );
			break;
		}
		
		if ( !Q_stricmp( tokenizer->CurrentToken(), "}" ) )
			break;
		
		CUtlVector< CExpressionSample > samples;
		
		float time = (float)atof( tokenizer->CurrentToken() );
		tokenizer->GetToken( false );
		float value = (float)atof( tokenizer->CurrentToken() );
		
		// Add to counter
		int idx = samples.AddToTail();
		CExpressionSample *s = &samples[ idx ];
			
		s->time			= time;
		s->value		= value;
		
		// If there are more tokens on this line, then it's a new format curve name
		if ( tokenizer->TokenAvailable() )
		{
			tokenizer->GetToken( false );
			int curveType = Interpolator_CurveTypeForName( tokenizer->CurrentToken() );
			s->SetCurveType( curveType );
		}

		if ( samples.Size() >= 1 )
		{
			for ( int i = 0; i < samples.Size(); i++ )
			{
				CExpressionSample sample = samples[ i ];

				CExpressionSample *newSample = Add( sample.time, sample.value, false );
				newSample->SetCurveType( sample.GetCurveType() );
			}
		}
	}

	Resort( data );
}

//-----------------------------------------------------------------------------
// Purpose: Helper for restoring edge info
// Input  : *edgeinfo - 
//-----------------------------------------------------------------------------
void CChoreoScene::ParseEdgeInfo( ISceneTokenProcessor *tokenizer, EdgeInfo_t *edgeinfo )
{
	Assert( edgeinfo );
	Assert( tokenizer );

	tokenizer->GetToken( false );
	edgeinfo->m_bActive = true;
	edgeinfo->m_CurveType = Interpolator_CurveTypeForName( tokenizer->CurrentToken() );
	tokenizer->GetToken( false );
	edgeinfo->m_flZeroPos = atof( tokenizer->CurrentToken() );
	tokenizer->GetToken( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tokenizer - 
//			*e - 
//-----------------------------------------------------------------------------
void CChoreoScene::ParseFlexAnimations( ISceneTokenProcessor *tokenizer, CChoreoEvent *e, bool removeold /*= true*/ )
{
	Assert( e );

	if ( removeold )
	{
		// Make sure there's nothing already there...
		e->RemoveAllTracks();
		// Make it re-index
		e->SetTrackLookupSet( false );
	}

	// BACKWARD COMPATABILITY
	// in the old system the samples were 0.0 to 1.0 mapped to endtime - starttime
	// if samples_use_time is true, then samples are actually offsets of time from starttime
	bool samples_use_realtime = false;
	// Parse tags between { }
	//
	tokenizer->GetToken( true );

	Assert( e->HasEndTime() );

	float endtime		= e->GetEndTime();
	float starttime		= e->GetStartTime();
	float event_time	= endtime - starttime;
	int nDefaultCurveType = CURVE_DEFAULT;

	// Is it the new file format?
	if ( !Q_stricmp( tokenizer->CurrentToken(), "samples_use_time" ) )
	{
		samples_use_realtime = true;
		tokenizer->GetToken( true );
	}

	// Check for default curve type
	if ( !Q_strnicmp( tokenizer->CurrentToken(), "defaultcurvetype", 16 ) )
	{
		const char *pTest = tokenizer->CurrentToken() + 16;
		if ( *pTest == 0 )
		{
			tokenizer->GetToken( true );
			pTest = tokenizer->CurrentToken();
		}

		if ( *pTest != '=' )
		{
			tokenizer->Error( "expecting =\n" );
		}
		++pTest;
		if ( *pTest == 0 )
		{
			tokenizer->GetToken( true );
			pTest = tokenizer->CurrentToken();
		}

		nDefaultCurveType = Interpolator_CurveTypeForName( pTest );
		tokenizer->GetToken( true );
		e->SetDefaultCurveType( nDefaultCurveType );
	}

	if ( stricmp( tokenizer->CurrentToken(), "{" ) )
	{
		tokenizer->Error( "expecting {\n" );
	}
	
	while ( 1 )
	{
		// Parse until }
		tokenizer->GetToken( true );
		
		if ( strlen( tokenizer->CurrentToken() ) <= 0 )
		{
			tokenizer->Error( "expecting flex animation data\n" );
			break;
		}
		
		if ( !Q_stricmp( tokenizer->CurrentToken(), "}" ) )
			break;
		
		char flexcontroller[ CFlexAnimationTrack::MAX_CONTROLLER_NAME ];
		Q_strncpy( flexcontroller, tokenizer->CurrentToken(), sizeof( flexcontroller ) );
		
		// Animations default to active
		bool active = true;
		bool combo = false;
		float range_min = 0.0f;
		float range_max = 1.0f;
		tokenizer->GetToken( true );

		EdgeInfo_t edgeinfo[ 2 ];
		
		if ( !Q_stricmp( tokenizer->CurrentToken(), "disabled" ) )
		{
			active = false;
			tokenizer->GetToken( true );
		}
		
		if ( !Q_stricmp( tokenizer->CurrentToken(), "combo" ) )
		{
			combo = true;
			tokenizer->GetToken( true );
		}
		
		if ( !Q_stricmp( tokenizer->CurrentToken(), "range" ) )
		{
			tokenizer->GetToken( false );
			range_min = atof( tokenizer->CurrentToken() );
			tokenizer->GetToken( false );
			range_max = atof( tokenizer->CurrentToken() );
			tokenizer->GetToken( true );
		}
		
		if ( !Q_stricmp( tokenizer->CurrentToken(), "leftedge" ) )
		{
			ParseEdgeInfo( tokenizer, &edgeinfo[ 0 ] );
		}

		if ( !Q_stricmp( tokenizer->CurrentToken(), "rightedge" ) )
		{
			ParseEdgeInfo( tokenizer, &edgeinfo[ 1 ] );
		}

		CUtlVector< CExpressionSample > samples[2];
		
		for ( int samplecount = 0; samplecount < ( combo ? 2 : 1 ); samplecount++ )
		{
			if ( stricmp( tokenizer->CurrentToken(), "{" ) )
			{
				tokenizer->Error( "expecting {\n" );
			}
			
			while ( 1 )
			{
				tokenizer->GetToken( true );
				
				if ( strlen( tokenizer->CurrentToken() ) <= 0 )
				{
					tokenizer->Error( "expecting flex animation data\n" );
					break;
				}
				
				if ( !Q_stricmp( tokenizer->CurrentToken(), "}" ) )
					break;
				
				float time = (float)atof( tokenizer->CurrentToken() );
				tokenizer->GetToken( false );
				float value = (float)atof( tokenizer->CurrentToken() );
				
				// Add to counter
				int idx = samples[ samplecount ].AddToTail();
				
				CExpressionSample *s = &samples[ samplecount ][ idx ];
				
				if ( samples_use_realtime )
				{
					s->time		= time;
				}
				else
				{
					// Time is an old style fraction (0 to 1) map into real time
					s->time		= time * event_time;
				}

				s->value		= value;

				// If there are more tokens on this line, then it's a new format curve name
				if ( tokenizer->TokenAvailable() )
				{
					tokenizer->GetToken( false );
					int curveType = Interpolator_CurveTypeForName( tokenizer->CurrentToken() );
					s->SetCurveType( curveType );
				}
				else
				{
					s->SetCurveType( nDefaultCurveType );
				}
			}
			
			if ( combo && samplecount == 0 )
			{
				tokenizer->GetToken( true );
			}
		}
		
		if ( active || samples[ 0 ].Size() >= 1 )
		{
			// Add it in
			CFlexAnimationTrack *track = e->AddTrack( flexcontroller );
			Assert( track );
			track->SetTrackActive( active );
			track->SetComboType( combo );
			
			track->SetMin( range_min );
			track->SetMax( range_max );
			
			for ( int t = 0; t < ( combo ? 2 : 1 ); t++ )
			{
				for ( int i = 0; i < samples[ t ].Size(); i++ )
				{
					CExpressionSample *sample = &samples[ t ][ i ];
					
					CExpressionSample *added = track->AddSample( sample->time, sample->value, t );
					Assert( added );
					added->SetCurveType( sample->GetCurveType() );
				}
			}

			for ( int edge = 0; edge < 2; ++edge )
			{
				if ( !edgeinfo[ edge ].m_bActive )
					continue;

				track->SetEdgeActive( edge == 0 ? true : false, true );
				track->SetEdgeInfo( edge == 0 ? true : false, edgeinfo[ edge ].m_CurveType, edgeinfo[ edge ].m_flZeroPos );
			}

			track->Resort( 0 );
			track->Resort( 1 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*channel - 
// Output : CChoreoEvent
//-----------------------------------------------------------------------------
CChoreoEvent *CChoreoScene::ParseEvent( CChoreoActor *actor, CChoreoChannel *channel )
{
	// For conversion of old style attack/sustain/decay ramps
	bool hadramp = false;
	float attack = 1.0f, sustain = 1.0f, decay = 1.0f;

	CChoreoEvent *e;
	{
	MEM_ALLOC_CREDIT();
	e = AllocEvent();
	}

	MEM_ALLOC_CREDIT();

	Assert( e );

	// read event type
	m_pTokenizer->GetToken( false );

	e->SetType( CChoreoEvent::TypeForName( m_pTokenizer->CurrentToken() ) );

	m_pTokenizer->GetToken( false );
	e->SetName( m_pTokenizer->CurrentToken() );

	m_pTokenizer->GetToken( true );
	if ( stricmp( m_pTokenizer->CurrentToken(), "{" ) )
		m_pTokenizer->Error( "expecting {\n" );

	while ( 1 )
	{
		m_pTokenizer->GetToken( true );
		if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "}" ) )
			break;

		if ( strlen( m_pTokenizer->CurrentToken() ) <= 0 )
		{
			m_pTokenizer->Error( "expecting more tokens!" );
			break;
		}

		if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "time" ) )
		{
			float start, end = 1.0f;

			m_pTokenizer->GetToken( false );
			start = (float)atof( m_pTokenizer->CurrentToken() );
			if ( m_pTokenizer->TokenAvailable() )
			{
				m_pTokenizer->GetToken( false );
				end = (float)atof( m_pTokenizer->CurrentToken() );
			}

			e->SetStartTime( start );
			e->SetEndTime( end );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "ramp" ) )
		{
			hadramp = true;

			m_pTokenizer->GetToken( false );
			attack = (float)atof( m_pTokenizer->CurrentToken() );
			if ( m_pTokenizer->TokenAvailable() )
			{
				m_pTokenizer->GetToken( false );
				sustain = (float)atof( m_pTokenizer->CurrentToken() );
			}
			if ( m_pTokenizer->TokenAvailable() )
			{
				m_pTokenizer->GetToken( false );
				decay = (float)atof( m_pTokenizer->CurrentToken() );
			}
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "param" ) )
		{
			m_pTokenizer->GetToken( false );

			e->SetParameters( m_pTokenizer->CurrentToken() );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "param2" ) )
		{
			m_pTokenizer->GetToken( false );

			e->SetParameters2( m_pTokenizer->CurrentToken() );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "param3" ) )
		{
			m_pTokenizer->GetToken( false );

			e->SetParameters3( m_pTokenizer->CurrentToken() );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "pitch" ) )
		{
			m_pTokenizer->GetToken( false );
			e->SetPitch( atoi( m_pTokenizer->CurrentToken() )  );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "yaw" ) )
		{
			m_pTokenizer->GetToken( false );
			e->SetYaw( atoi( m_pTokenizer->CurrentToken() )  );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "loopcount" ) )
		{
			m_pTokenizer->GetToken( false );
			e->SetLoopCount( atoi( m_pTokenizer->CurrentToken() ) );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "resumecondition" ) )
		{
			e->SetResumeCondition( true );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "fixedlength" ) )
		{
			e->SetFixedLength( true );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "lockbodyfacing" ) )
		{
			e->SetLockBodyFacing( true );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "distancetotarget" ) )
		{
			m_pTokenizer->GetToken( false );
			e->SetDistanceToTarget( atof( m_pTokenizer->CurrentToken() ) );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "forceshortmovement" ) )
		{
			e->SetForceShortMovement( true );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "synctofollowinggesture" ) )
		{
			e->SetSyncToFollowingGesture( true );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "active" ) )
		{
			m_pTokenizer->GetToken( false );
			e->SetActive( atoi( m_pTokenizer->CurrentToken() ) ? true : false );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "playoverscript" ) )
		{
			e->SetPlayOverScript( true );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "tags" ) )
		{
			// Parse tags between { }
			//
			m_pTokenizer->GetToken( true );
			if ( stricmp( m_pTokenizer->CurrentToken(), "{" ) )
				m_pTokenizer->Error( "expecting {\n" );

			while ( 1 )
			{
				// Parse until }
				m_pTokenizer->GetToken( true );

				if ( strlen( m_pTokenizer->CurrentToken() ) <= 0 )
				{
					m_pTokenizer->Error( "expecting relative tag\n" );
					break;
				}

				if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "}" ) )
					break;

				char tagname[ CEventRelativeTag::MAX_EVENTTAG_LENGTH ];
				float percentage;

				Q_strncpy( tagname, m_pTokenizer->CurrentToken(), sizeof( tagname ) );
				m_pTokenizer->GetToken( false );
				percentage = (float)atof( m_pTokenizer->CurrentToken() );

				e->AddRelativeTag( tagname, percentage );
			}
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "sequenceduration" ) )
		{
			float duration = 0.0f;

			m_pTokenizer->GetToken( false );
			duration = (float)atof( m_pTokenizer->CurrentToken() );

			e->SetGestureSequenceDuration( duration );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "absolutetags" ) )
		{
			m_pTokenizer->GetToken( true );
			CChoreoEvent::AbsTagType tagtype;
			
			tagtype = CChoreoEvent::TypeForAbsoluteTagName( m_pTokenizer->CurrentToken() );

			if ( tagtype == (CChoreoEvent::AbsTagType) -1 )
			{
				m_pTokenizer->Error( "expecting valid tag type!!!" );
			}

			// Parse tags between { }
			//
			m_pTokenizer->GetToken( true );
			if ( stricmp( m_pTokenizer->CurrentToken(), "{" ) )
				m_pTokenizer->Error( "expecting {\n" );

			while ( 1 )
			{
				// Parse until }
				m_pTokenizer->GetToken( true );

				if ( strlen( m_pTokenizer->CurrentToken() ) <= 0 )
				{
					m_pTokenizer->Error( "expecting relative tag\n" );
					break;
				}

				if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "}" ) )
					break;

				char tagname[ CFlexTimingTag::MAX_EVENTTAG_LENGTH ];
				float t;

				Q_strncpy( tagname, m_pTokenizer->CurrentToken(), sizeof( tagname ) );
				m_pTokenizer->GetToken( false );
				t = (float)atof( m_pTokenizer->CurrentToken() );

				e->AddAbsoluteTag( tagtype, tagname, t );
			}
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "flextimingtags" ) )
		{
			// Parse tags between { }
			//
			m_pTokenizer->GetToken( true );
			if ( stricmp( m_pTokenizer->CurrentToken(), "{" ) )
				m_pTokenizer->Error( "expecting {\n" );

			while ( 1 )
			{
				// Parse until }
				m_pTokenizer->GetToken( true );

				if ( strlen( m_pTokenizer->CurrentToken() ) <= 0 )
				{
					m_pTokenizer->Error( "expecting relative tag\n" );
					break;
				}

				if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "}" ) )
					break;

				char tagname[ CFlexTimingTag::MAX_EVENTTAG_LENGTH ];
				float percentage;
				bool locked;

				Q_strncpy( tagname, m_pTokenizer->CurrentToken(), sizeof( tagname ) );
				m_pTokenizer->GetToken( false );
				percentage = (float)atof( m_pTokenizer->CurrentToken() );

				m_pTokenizer->GetToken( false );
				locked = atoi( m_pTokenizer->CurrentToken() ) ? true : false;

				e->AddTimingTag( tagname, percentage, locked );
			}
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "relativetag" ) )
		{
			char tagname[ CChoreoEvent::MAX_TAGNAME_STRING ];
			char wavname[ CChoreoEvent::MAX_TAGNAME_STRING ];

			m_pTokenizer->GetToken( false );
			Q_strncpy( tagname, m_pTokenizer->CurrentToken(), sizeof( tagname ) );
			m_pTokenizer->GetToken( false );
			Q_strncpy( wavname, m_pTokenizer->CurrentToken(), sizeof( wavname ) );

			e->SetUsingRelativeTag( true, tagname, wavname );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "flexanimations" ) )
		{
			ParseFlexAnimations( m_pTokenizer, e );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "event_ramp" ) )
		{
			ParseRamp( m_pTokenizer, e );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "cctype" ) )
		{
			m_pTokenizer->GetToken( false );
			e->SetCloseCaptionType( CChoreoEvent::CCTypeForName( m_pTokenizer->CurrentToken() ) );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "cctoken" ) )
		{
			m_pTokenizer->GetToken( false );
			e->SetCloseCaptionToken( m_pTokenizer->CurrentToken() );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "cc_usingcombinedfile" ) )
		{
			e->SetUsingCombinedFile( true );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "cc_combinedusesgender" ) )
		{
			e->SetCombinedUsingGenderToken( true );
		}
		else if( !Q_stricmp( m_pTokenizer->CurrentToken(), "cc_noattenuate" ) )
		{
			e->SetSuppressingCaptionAttenuation( true );
		}
	}

	if ( channel )
	{
		channel->AddEvent( e );
	}

	e->SetActor( actor );
	e->SetChannel( channel );

	// It had old sytle ramp and none of the new style stuff
	//  Convert it
	if ( hadramp && !e->GetRampCount() )
	{
		// Only retrofit if something was changed by user
		if ( attack != 1.0f ||
			 sustain != 1.0f ||
			 decay != 1.0f )
		{
			float attacktime = ( 1.0f - attack ) * e->GetDuration();
			float decaytime = decay * e->GetDuration();
			float midpoint = ( attacktime + decaytime ) * 0.5f;

			e->AddRamp( attacktime, sustain, false );
			e->AddRamp( midpoint, sustain, false );
			e->AddRamp( decaytime, sustain, false );
			e->ResortRamp();
		}
	}

	return e;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CChoreoActor
//-----------------------------------------------------------------------------
CChoreoActor *CChoreoScene::ParseActor( void )
{
	CChoreoActor *a = AllocActor();
	Assert( a );

	m_pTokenizer->GetToken( false );
	a->SetName( m_pTokenizer->CurrentToken() );

	m_pTokenizer->GetToken( true );
	if ( stricmp( m_pTokenizer->CurrentToken(), "{" ) )
		m_pTokenizer->Error( "expecting {" );

	// Parse channels
	while ( 1 )
	{
		m_pTokenizer->GetToken( true );
		if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "}" ) )
			break;

		if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "channel" ) )
		{
			ParseChannel( a );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "faceposermodel" ) )
		{
			ParseFacePoserModel( a );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "active" ) )
		{
			m_pTokenizer->GetToken( true );
			a->SetActive( atoi( m_pTokenizer->CurrentToken() ) ? true : false );
		}
		else
		{
			m_pTokenizer->Error( "expecting channel got %s\n", m_pTokenizer->CurrentToken() );
		}
	}

	return a;
}

//-----------------------------------------------------------------------------
// Output : char const
//-----------------------------------------------------------------------------
const char *CChoreoScene::GetMapname( void )
{
	return m_szMapname;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
void CChoreoScene::SetMapname( const char *name )
{
	Q_strncpy( m_szMapname, name, sizeof( m_szMapname ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoScene::ParseMapname( void )
{
	m_szMapname[ 0 ] = 0;

	m_pTokenizer->GetToken( true );
	Q_strncpy( m_szMapname, m_pTokenizer->CurrentToken(), sizeof( m_szMapname ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoScene::ParseFPS( void )
{
	m_pTokenizer->GetToken( true );
	m_nSceneFPS = atoi( m_pTokenizer->CurrentToken() );
	// Clamp to valid range
	m_nSceneFPS = clamp( m_nSceneFPS, MIN_SCENE_FPS, MAX_SCENE_FPS);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoScene::ParseSnap( void )
{
	m_pTokenizer->GetToken( true );
	m_bUseFrameSnap = !Q_stricmp( m_pTokenizer->CurrentToken(), "on" ) ? true : false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoScene::ParseIgnorePhonemes( void )
{
	m_pTokenizer->GetToken( true );
	m_bIgnorePhonemes = !Q_stricmp( m_pTokenizer->CurrentToken(), "on" ) ? true : false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void CChoreoScene::ParseFacePoserModel( CChoreoActor *actor )
{
	m_pTokenizer->GetToken( true );
	actor->SetFacePoserModelName( m_pTokenizer->CurrentToken() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
// Output : CChoreoChannel
//-----------------------------------------------------------------------------
CChoreoChannel *CChoreoScene::ParseChannel( CChoreoActor *actor )
{
	CChoreoChannel *c = AllocChannel();
	Assert( c );

	m_pTokenizer->GetToken( false );
	c->SetName( m_pTokenizer->CurrentToken() );

	m_pTokenizer->GetToken( true );
	if ( stricmp( m_pTokenizer->CurrentToken(), "{" ) )
		m_pTokenizer->Error( "expecting {" );

	// Parse channels
	while ( 1 )
	{
		m_pTokenizer->GetToken( true );
		if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "}" ) )
			break;

		if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "event" ) )
		{
			ParseEvent( actor, c );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "active" ) )
		{
			m_pTokenizer->GetToken( true );
			c->SetActive( atoi( m_pTokenizer->CurrentToken() ) ? true : false );
		}
		else
		{
			m_pTokenizer->Error( "expecting event got %s\n", m_pTokenizer->CurrentToken() );
		}
	}

	Assert( actor );
	if ( actor )
	{
		actor->AddChannel( c );
		c->SetActor( actor );
	}

	return c;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoScene::ParseFromBuffer( const char *pFilename, ISceneTokenProcessor *tokenizer )
{
	Q_strncpy( m_szFileName, pFilename, sizeof(m_szFileName) );
	m_pTokenizer = tokenizer;

	while ( 1 )
	{
		if ( !m_pTokenizer->GetToken( true ) )
		{
			break;
		}

		if ( strlen( m_pTokenizer->CurrentToken() ) <= 0 )
			break;

		if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "event" ) )
		{
			ParseEvent( NULL, NULL );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "actor" ) )
		{
			ParseActor();
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "mapname" ) )
		{
			ParseMapname();
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "fps" ) )
		{
			ParseFPS();
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "snap" ) )
		{
			ParseSnap();
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "ignorePhonemes" ) )
		{
			ParseIgnorePhonemes();
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "scene_ramp" ) )
		{
			ParseSceneRamp( m_pTokenizer, this );
		}
		else if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "scalesettings" ) )
		{
			ParseScaleSettings( m_pTokenizer, this );
		}
		else
		{
			m_pTokenizer->Error( "%s: unexpected token %s\n", m_szFileName, m_pTokenizer->CurrentToken() );
			break;
		}
	}

	// Fixup time tags
	ReconcileTags();

	ReconcileGestureTimes();

	ReconcileCloseCaption();

	InternalDetermineEventTypes(); 

	if ( CChoreoScene::s_bEditingDisabled )
	{
		m_flPrecomputedStopTime = FindStopTime();
	}

	return true;
}

void CChoreoScene::RemoveEventsExceptTypes( int* typeList, int count )
{
	int i;
	for ( i = 0 ; i < m_Actors.Count(); i++ )
	{
		CChoreoActor *a = m_Actors[ i ];
		if ( !a )
			continue;

		for ( int j = 0; j < a->GetNumChannels(); j++ )
		{
			CChoreoChannel *c = a->GetChannel( j );
			if ( !c )
				continue;
			
			int num = c->GetNumEvents();
			for ( int k = num - 1 ; k >= 0; --k )
			{
				CChoreoEvent *e = c->GetEvent( k );
				if ( !e )
					continue;

				bool found = false;
				for ( int idx = 0; idx < count; ++idx )
				{
					if ( e->GetType() == ( CChoreoEvent::EVENTTYPE )typeList[ idx ] )
					{
						found = true;
						break;
					}
				}

				if ( !found )
				{
					c->RemoveEvent( e );
					DeleteReferencedObjects( e );
				}
			}
		}
	}

	// Remvoe non-matching global events, too
	for ( i = m_Events.Count() - 1 ; i >= 0; --i )
	{
		CChoreoEvent *e = m_Events[ i ];

		// This was already dealt with above...
		if ( e->GetActor() )
			continue;

		bool found = false;
		for ( int idx = 0; idx < count; ++idx )
		{
			if ( e->GetType() == ( CChoreoEvent::EVENTTYPE )typeList[ idx ] )
			{
				found = true;
				break;
			}
		}

		if ( !found )
		{
			DeleteReferencedObjects( e );
		}
	}
}

void CChoreoScene::InternalDetermineEventTypes()
{
	m_bitvecHasEventOfType.ClearAll();

	for ( int i = 0 ; i < m_Actors.Size(); i++ )
	{
		CChoreoActor *a = m_Actors[ i ];
		if ( !a )
			continue;

		for ( int j = 0; j < a->GetNumChannels(); j++ )
		{
			CChoreoChannel *c = a->GetChannel( j );
			if ( !c )
				continue;
			
			for ( int k = 0 ; k < c->GetNumEvents(); k++ )
			{
				CChoreoEvent *e = c->GetEvent( k );
				if ( !e )
					continue;

				m_bitvecHasEventOfType.Set( e->GetType(), true );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoScene::FindStopTime( void )
{
	if ( m_flPrecomputedStopTime != 0.0f )
	{
		return m_flPrecomputedStopTime;
	}

	float lasttime = 0.0f;

	int c = m_Events.Count();
	for ( int i = 0; i < c ; i++ )
	{
		CChoreoEvent *e = m_Events[ i ];
		Assert( e );

		float checktime = e->HasEndTime() ? e->GetEndTime() : e->GetStartTime();
		if ( checktime > lasttime )
		{
			lasttime = checktime;
		}
	}

	return lasttime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fp - 
//			level - 
//			*fmt - 
//			... - 
//-----------------------------------------------------------------------------
void CChoreoScene::FilePrintf( CUtlBuffer& buf, int level, const char *fmt, ... )
{
	va_list argptr;
	va_start( argptr, fmt );

	while ( level-- > 0 )
	{
		buf.Printf( "  " );
	}

	buf.VaPrintf( fmt, argptr );
	va_end( argptr );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fp - 
//-----------------------------------------------------------------------------
void CChoreoScene::FileSaveHeader( CUtlBuffer& buf )
{
	FilePrintf( buf, 0, "// Choreo version 1\n" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mark - 
//-----------------------------------------------------------------------------
void CChoreoScene::MarkForSaveAll( bool mark )
{
	int i;

	// Mark global events
	for ( i = 0 ; i < m_Events.Size(); i++ )
	{
		CChoreoEvent *e = m_Events[ i ];
		if ( e->GetActor() )
			continue;

		e->SetMarkedForSave( mark );
	}

	// Recursively mark everything else
	for ( i = 0 ; i < m_Actors.Size(); i++ )
	{
		CChoreoActor *a = m_Actors[ i ];
		if ( !a )
			continue;

		a->MarkForSaveAll( mark );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoScene::ExportMarkedToFile( const char *filename )
{
	// Create a serialization buffer
	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	FileSaveHeader( buf );

	// Look for events that don't have actor/channel set
	int i;
	for ( i = 0 ; i < m_Events.Size(); i++ )
	{
		CChoreoEvent *e = m_Events[ i ];
		if ( e->GetActor() )
			continue;

		FileSaveEvent( buf, 0, e );
	}

	for ( i = 0 ; i < m_Actors.Size(); i++ )
	{
		CChoreoActor *a = m_Actors[ i ];
		if ( !a )
			continue;

		FileSaveActor( buf, 0, a );
	}

	// Write it out baby
	FileHandle_t fh = g_pFullFileSystem->Open( filename, "wt" );
	if (fh)
	{
		g_pFullFileSystem->Write( buf.Base(), buf.TellPut(), fh );
		g_pFullFileSystem->Close(fh);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//-----------------------------------------------------------------------------
bool CChoreoScene::SaveToFile( const char *filename )
{
	// Create a serialization buffer
	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	FileSaveHeader( buf );

	MarkForSaveAll( true );

	// Look for events that don't have actor/channel set
	int i;
	for ( i = 0 ; i < m_Events.Size(); i++ )
	{
		CChoreoEvent *e = m_Events[ i ];
		if ( e->GetActor() )
			continue;

		FileSaveEvent( buf, 0, e );
	}

	for ( i = 0 ; i < m_Actors.Size(); i++ )
	{
		CChoreoActor *a = m_Actors[ i ];
		if ( !a )
			continue;

		FileSaveActor( buf, 0, a );
	}

	if ( m_szMapname[ 0 ] )
	{
		FilePrintf( buf, 0, "mapname \"%s\"\n", m_szMapname );
	}

	FileSaveSceneRamp( buf, 0 );
	FileSaveScaleSettings( buf, 0, this );

	FilePrintf( buf, 0, "fps %i\n", m_nSceneFPS );
	FilePrintf( buf, 0, "snap %s\n", m_bUseFrameSnap ? "on" : "off" );
	FilePrintf( buf, 0, "ignorePhonemes %s\n", m_bIgnorePhonemes ? "on" : "off" );

	// Write it out baby
	FileHandle_t fh = g_pFullFileSystem->Open( filename, "wt" );
	if (fh)
	{
		g_pFullFileSystem->Write( buf.Base(), buf.TellPut(), fh );
		g_pFullFileSystem->Close(fh);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//			level - 
//			*e - 
//-----------------------------------------------------------------------------
void CChoreoScene::FileSaveRamp( CUtlBuffer& buf, int level, CChoreoEvent *e )
{
	e->GetRamp()->FileSave( buf, level, "event_ramp" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//			level - 
//			*e - 
//-----------------------------------------------------------------------------
void CChoreoScene::FileSaveSceneRamp( CUtlBuffer& buf, int level )
{
	m_SceneRamp.FileSave( buf, level, "scene_ramp" );
}


void CCurveData::FileSave( CUtlBuffer& buf, int level, const char *name )
{
	// Nothing to save?
	int c = GetCount();
	if ( c <= 0 && 
		!IsEdgeActive( true ) && 
		!IsEdgeActive( false ) )
		return;

	char line[ 1024 ];
	Q_strncpy( line, name, sizeof( line ) );

	if ( IsEdgeActive( true ) || IsEdgeActive( false ) )
	{
		if ( IsEdgeActive( true ) )
		{
			char sz[ 256 ];
			Q_snprintf( sz, sizeof( sz ), " leftedge %s %.3f", Interpolator_NameForCurveType( GetEdgeCurveType( true ), false ), GetEdgeZeroValue( true ) );
			Q_strncat( line, sz, sizeof( line ), COPY_ALL_CHARACTERS );
		}
		if ( IsEdgeActive( false ) )
		{
			char sz[ 256 ];
			Q_snprintf( sz, sizeof( sz )," rightedge %s %.3f", Interpolator_NameForCurveType( GetEdgeCurveType( false ), false ), GetEdgeZeroValue( false ) );
			Q_strncat( line, sz, sizeof( line ), COPY_ALL_CHARACTERS );
		}
	}

	CChoreoScene::FilePrintf( buf, level, "%s\n", line );
	CChoreoScene::FilePrintf( buf, level, "{\n" );

	for ( int i = 0; i < c; i++ )
	{
		CExpressionSample *sample = Get( i );
		if ( sample->GetCurveType() != CURVE_DEFAULT )
		{
			CChoreoScene::FilePrintf( buf, level + 1, "%.4f %.4f \"%s\"\n",
				sample->time,
				sample->value,
				Interpolator_NameForCurveType( sample->GetCurveType(), false ) );	
		}
		else
		{
			CChoreoScene::FilePrintf( buf, level + 1, "%.4f %.4f\n",
				sample->time,
				sample->value );	
		}
	}

	CChoreoScene::FilePrintf( buf, level, "}\n" );
}

void CChoreoScene::FileSaveScaleSettings( CUtlBuffer& buf, int level, CChoreoScene *scene )
{
	// Nothing to save?
	int c = scene->m_TimeZoomLookup.Count();
	if ( c <= 0 )
		return;

	FilePrintf( buf, level, "scalesettings\n" );
	FilePrintf( buf, level, "{\n" );

	for ( int i = 0; i < c; i++ )
	{
		int value = scene->m_TimeZoomLookup[ i ];

		FilePrintf( buf, level + 1, "\"%s\" \"%i\"\n",
			scene->m_TimeZoomLookup.GetElementName( i ),
			value );	
	}

	FilePrintf( buf, level, "}\n" );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//			level - 
//			*track - 
//-----------------------------------------------------------------------------
void CChoreoScene::FileSaveFlexAnimationTrack( CUtlBuffer& buf, int level, CFlexAnimationTrack *track, int nDefaultCurveType )
{
	if ( !track )
		return;

	if ( !track->IsTrackActive() && track->GetNumSamples() <= 0 )
		return;

	char line[ 1024 ];
	Q_snprintf( line, sizeof( line ), "\"%s\" ", track->GetFlexControllerName() );
	if ( !track->IsTrackActive() )
	{
		char sz[ 256 ];
		Q_snprintf( sz, sizeof( sz ), "disabled " );
		Q_strncat( line, sz, sizeof( line ), COPY_ALL_CHARACTERS );
	}
	if ( track->IsComboType() )
	{
		char sz[ 256 ];
		Q_snprintf( sz, sizeof( sz ), "combo " );
		Q_strncat( line, sz, sizeof( line ), COPY_ALL_CHARACTERS );
	}
	if ( track->GetMin() != 0.0f || track->GetMax() != 1.0f)
	{
		char sz[ 256 ];
		Q_snprintf( sz, sizeof( sz ), "range %.1f %.1f ", track->GetMin(), track->GetMax() );
		Q_strncat( line, sz, sizeof( line ), COPY_ALL_CHARACTERS );
	}
	if ( track->IsEdgeActive( true ) || track->IsEdgeActive( false ) )
	{
		char edgestr[ 512 ];
		edgestr[ 0 ] = 0;

		if ( track->IsEdgeActive( true ) )
		{
			char sz[ 256 ];
			Q_snprintf( sz, sizeof( sz ), "leftedge %s %.3f ", Interpolator_NameForCurveType( track->GetEdgeCurveType( true ), false ), track->GetEdgeZeroValue( true ) );
			Q_strncat( edgestr, sz, sizeof( edgestr ), COPY_ALL_CHARACTERS );
		}
		if ( track->IsEdgeActive( false ) )
		{
			char sz[ 256 ];
			Q_snprintf( sz, sizeof( sz ), "rightedge %s %.3f ", Interpolator_NameForCurveType( track->GetEdgeCurveType( false ), false ), track->GetEdgeZeroValue( false ) );
			Q_strncat( edgestr, sz, sizeof( edgestr ), COPY_ALL_CHARACTERS );
		}

		Q_strncat( line, edgestr, sizeof( line ), COPY_ALL_CHARACTERS );
	}


	FilePrintf( buf, level + 2, "%s\n", line );
	
	// Write out samples
	FilePrintf( buf, level + 2, "{\n" );

	for ( int j = 0 ; j < track->GetNumSamples( 0 ) ; j++ )
	{
		CExpressionSample *s = track->GetSample( j, 0 );
		if ( !s )
			continue;

		if ( s->GetCurveType() != nDefaultCurveType && s->GetCurveType() != CURVE_DEFAULT )
		{
			FilePrintf( buf, level + 3, "%.4f %.4f \"%s\"\n",
				s->time,
				s->value,
				Interpolator_NameForCurveType( s->GetCurveType(), false ) );	
		}
		else
		{
			FilePrintf( buf, level + 3, "%.4f %.4f\n",
				s->time,
				s->value );	
		}
	}

	FilePrintf( buf, level + 2, "}\n" );

	// Write out combo samples
	if ( track->IsComboType() )
	{
		FilePrintf( buf, level + 2, "{\n" );
	
		for ( int j = 0 ; j < track->GetNumSamples( 1) ; j++ )
		{
			CExpressionSample *s = track->GetSample( j, 1 );
			if ( !s )
				continue;

			if ( s->GetCurveType() != nDefaultCurveType && s->GetCurveType() != CURVE_DEFAULT )
			{
				FilePrintf( buf, level + 3, "%.4f %.4f \"%s\"\n",
					s->time,
					s->value,
					Interpolator_NameForCurveType( s->GetCurveType(), false ) );	
			}
			else
			{
				FilePrintf( buf, level + 3, "%.4f %.4f\n",
					s->time,
					s->value );	
			}
		}

		FilePrintf( buf, level + 2, "}\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//			level - 
//			*e - 
//-----------------------------------------------------------------------------
void CChoreoScene::FileSaveFlexAnimations( CUtlBuffer& buf, int level, CChoreoEvent *e )
{
	// Nothing to save
	if ( e->GetNumFlexAnimationTracks() <= 0 )
		return;

	if ( e->GetDefaultCurveType() != CURVE_DEFAULT )
	{
		FilePrintf( buf, level + 1, "flexanimations samples_use_time defaultcurvetype=%s\n",
			Interpolator_NameForCurveType( e->GetDefaultCurveType(), false ) );
	}
	else
	{
		FilePrintf( buf, level + 1, "flexanimations samples_use_time\n" );
	}
	FilePrintf( buf, level + 1, "{\n" );

	for ( int i = 0; i < e->GetNumFlexAnimationTracks(); i++ )
	{
		CFlexAnimationTrack *track = e->GetFlexAnimationTrack( i );
		FileSaveFlexAnimationTrack( buf, level, track, e->GetDefaultCurveType() );
	}

	FilePrintf( buf, level + 1, "}\n" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fp - 
//			level - 
//			*e - 
//-----------------------------------------------------------------------------
void CChoreoScene::FileSaveEvent( CUtlBuffer& buf, int level, CChoreoEvent *e )
{
	if ( !e->IsMarkedForSave() )
		return;

	FilePrintf( buf, level, "event %s \"%s\"\n", CChoreoEvent::NameForType( e->GetType() ), e->GetName() );
	FilePrintf( buf, level, "{\n" );

	float st, et;
	st = e->GetStartTime();
	et = e->GetEndTime();

	FilePrintf( buf, level + 1, "time %f %f\n", st, et );
	FilePrintf( buf, level + 1, "param \"%s\"\n", e->GetParameters() );
	if ( strlen( e->GetParameters2() ) > 0 )
	{
		FilePrintf( buf, level + 1, "param2 \"%s\"\n", e->GetParameters2() );
	}
	if ( strlen( e->GetParameters3() ) > 0 )
	{
		FilePrintf( buf, level + 1, "param3 \"%s\"\n", e->GetParameters3() );
	}
	if ( e->GetRampCount() > 0 )
	{
		FileSaveRamp( buf, level + 1, e );
	}
	if ( e->GetPitch() != 0 )
	{
		FilePrintf( buf, level + 1, "pitch \"%i\"\n", e->GetPitch() );
	}
	if ( e->GetYaw() != 0 )
	{
		FilePrintf( buf, level + 1, "yaw \"%i\"\n", e->GetYaw() );
	}
	if ( e->IsResumeCondition() )
	{
		FilePrintf( buf, level + 1, "resumecondition\n" );
	}
	if ( e->IsLockBodyFacing() )
	{
		FilePrintf( buf, level + 1, "lockbodyfacing\n" );
	}
	if ( e->GetDistanceToTarget() > 0.0f )
	{
		FilePrintf( buf, level + 1, "distancetotarget %.2f\n", e->GetDistanceToTarget() );
	}
	if ( e->GetForceShortMovement() )
	{
		FilePrintf( buf, level + 1, "forceshortmovement\n" );
	}
	if ( e->GetSyncToFollowingGesture() )
	{
		FilePrintf( buf, level + 1, "synctofollowinggesture\n" );
	}
	if ( !e->GetActive() )
	{
		FilePrintf( buf, level + 1, "active 0\n" );
	}
	if ( e->GetPlayOverScript() )
	{
		FilePrintf( buf, level + 1, "playoverscript\n" );
	}
	if ( e->IsFixedLength() )
	{
		FilePrintf( buf, level + 1, "fixedlength\n" );
	}
	if ( e->GetNumRelativeTags() > 0 )
	{
		FilePrintf( buf, level + 1, "tags\n" );
		FilePrintf( buf, level + 1, "{\n" );
		for ( int t = 0; t < e->GetNumRelativeTags(); t++ )
		{
			CEventRelativeTag *rt = e->GetRelativeTag( t );
			Assert( rt );
			FilePrintf( buf, level + 2, "\"%s\" %f\n", rt->GetName(), rt->GetPercentage() );
		}
		FilePrintf( buf, level + 1, "}\n" );
	}
	if ( e->GetNumTimingTags() > 0 )
	{
		FilePrintf( buf, level + 1, "flextimingtags\n" );
		FilePrintf( buf, level + 1, "{\n" );
		for ( int t = 0; t < e->GetNumTimingTags(); t++ )
		{
			CFlexTimingTag *tt = e->GetTimingTag( t );
			Assert( tt );
			FilePrintf( buf, level + 2, "\"%s\" %f %i\n", tt->GetName(), tt->GetPercentage(), tt->GetLocked() ? 1 : 0 );
		}
		FilePrintf( buf, level + 1, "}\n" );
	}
	int tagtype;
	for ( tagtype = 0; tagtype < CChoreoEvent::NUM_ABS_TAG_TYPES; tagtype++ )
	{
		if ( e->GetNumAbsoluteTags( (CChoreoEvent::AbsTagType)tagtype ) > 0 )
		{
			FilePrintf( buf, level + 1, "absolutetags %s\n", CChoreoEvent::NameForAbsoluteTagType( (CChoreoEvent::AbsTagType)tagtype ) );
			FilePrintf( buf, level + 1, "{\n" );
			for ( int t = 0; t < e->GetNumAbsoluteTags( (CChoreoEvent::AbsTagType)tagtype ); t++ )
			{
				CEventAbsoluteTag *abstag = e->GetAbsoluteTag( (CChoreoEvent::AbsTagType)tagtype, t );
				Assert( abstag );
				FilePrintf( buf, level + 2, "\"%s\" %f\n", abstag->GetName(), abstag->GetPercentage() );
			}
			FilePrintf( buf, level + 1, "}\n" );
		}
	}

	if ( e->GetType() == CChoreoEvent::GESTURE )
	{
		float duration;
		if ( e->GetGestureSequenceDuration( duration ) )
		{
			FilePrintf( buf, level + 1, "sequenceduration %f\n", duration );
		}
	}

	if ( e->IsUsingRelativeTag() )
	{
		FilePrintf( buf, level + 1, "relativetag \"%s\" \"%s\"\n",
			e->GetRelativeTagName(), e->GetRelativeWavName() );
	}
	
	if ( e->GetNumFlexAnimationTracks() > 0 )
	{
		FileSaveFlexAnimations( buf, level, e );
	}

	if ( e->GetType() == CChoreoEvent::LOOP )
	{
		FilePrintf( buf, level + 1, "loopcount \"%i\"\n", e->GetLoopCount() );
	}

	if ( e->GetType() == CChoreoEvent::SPEAK )
	{
		FilePrintf( buf, level + 1, "cctype \"%s\"\n", CChoreoEvent::NameForCCType( e->GetCloseCaptionType() ) );
		FilePrintf( buf, level + 1, "cctoken \"%s\"\n", e->GetCloseCaptionToken() );
		if ( e->GetCloseCaptionType() != CChoreoEvent::CC_DISABLED &&
			 e->IsUsingCombinedFile() )
		{
			FilePrintf( buf, level + 1, "cc_usingcombinedfile\n" );
		}
		if ( e->IsCombinedUsingGenderToken() )
		{
			FilePrintf( buf, level + 1, "cc_combinedusesgender\n" );
		}
		if ( e->IsSuppressingCaptionAttenuation() )
		{
			FilePrintf( buf, level + 1, "cc_noattenuate\n" );
		}
	}

	FilePrintf( buf, level, "}\n" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fp - 
//			level - 
//			*c - 
//-----------------------------------------------------------------------------
void CChoreoScene::FileSaveChannel( CUtlBuffer& buf, int level, CChoreoChannel *c )
{
	if ( !c->IsMarkedForSave() )
		return;

	FilePrintf( buf, level, "channel \"%s\"\n", c->GetName() );
	FilePrintf( buf, level, "{\n" );
	
	for ( int i = 0; i < c->GetNumEvents(); i++ )
	{
		CChoreoEvent *e = c->GetEvent( i );
		if ( e )
		{
			FileSaveEvent( buf, level + 1, e );
		}
	}

	if ( !c->GetActive() )
	{
		// Only write out inactive
		FilePrintf( buf, level + 1, "active \"0\"\n" );
	}

	FilePrintf( buf, level, "}\n" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fp - 
//			level - 
//			*a - 
//-----------------------------------------------------------------------------
void CChoreoScene::FileSaveActor( CUtlBuffer& buf, int level, CChoreoActor *a )
{
	if ( !a->IsMarkedForSave() )
		return;

	FilePrintf( buf, level, "actor \"%s\"\n", a->GetName() );
	FilePrintf( buf, level, "{\n" );
	
	for ( int i = 0; i < a->GetNumChannels(); i++ )
	{
		CChoreoChannel *c = a->GetChannel( i );
		if ( c )
		{
			FileSaveChannel( buf, level + 1, c );
		}
	}

	if ( Q_strlen( a->GetFacePoserModelName() ) > 0 )
	{
		FilePrintf( buf, level + 1, "faceposermodel \"%s\"\n", a->GetFacePoserModelName() );
	}

	if ( !a->GetActive() )
	{
		// Only write out inactive
		FilePrintf( buf, level + 1, "active \"0\"\n" );
	}

	FilePrintf( buf, level, "}\n\n" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoScene::FindAdjustedStartTime( void )
{
	float earliest_time = 0.0f;

	CChoreoEvent *e;

	for ( int i = 0; i < m_Events.Size(); i++ )
	{
		e = m_Events[ i ];

		float starttime = e->GetStartTime();

		// If it's a wav file, pre-queue the starting time by the sound system's
		//  current latency
		if ( e->GetType() == CChoreoEvent::SPEAK )
		{
			starttime -= m_flSoundSystemLatency;
		}

		if ( starttime < earliest_time )
		{
			earliest_time = starttime;
		}
	}

	return earliest_time;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoScene::FindAdjustedEndTime( void )
{
	float latest_time = 0.0f;

	CChoreoEvent *e;

	for ( int i = 0; i < m_Events.Size(); i++ )
	{
		e = m_Events[ i ];

		float endtime = e->GetStartTime();
		if ( e->HasEndTime() )
		{
			endtime = e->GetEndTime();
		}

		// If it's a wav file, pre-queue the starting time by the sound system's
		//  current latency
		if ( e->GetType() == CChoreoEvent::SPEAK )
		{
			endtime += m_flSoundSystemLatency;
		}

		if ( endtime > latest_time )
		{
			latest_time = endtime;
		}
	}

	return latest_time;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoScene::ResetSimulation( bool forward /*= true*/, float starttime /*= 0.0f*/, float endtime /*= 0.0f*/ )
{
	CChoreoEvent *e;

	m_ActiveResumeConditions.RemoveAll();
	m_ResumeConditions.RemoveAll();
	m_PauseEvents.RemoveAll();

	// Put all items into the pending queue
	for ( int i = 0; i < m_Events.Size(); i++ )
	{
		e = m_Events[ i ];
		e->ResetProcessing();

		if ( e->GetType() == CChoreoEvent::SECTION )
		{
			m_PauseEvents.AddToTail( e );
			continue;
		}

		if ( e->IsResumeCondition() )
		{
			m_ResumeConditions.AddToTail( e );
			continue;
		}
	}

	// Find earliest adjusted start time
	m_flEarliestTime	= FindAdjustedStartTime();
	m_flLatestTime		= FindAdjustedEndTime();

	m_flCurrentTime = forward ? m_flEarliestTime : m_flLatestTime;

	m_flStartLoopTime = -1.f;

	// choreoprintf( 0, "Start time %f\n", m_flCurrentTime );

	m_flLastActiveTime = 0.0f;
	m_nActiveEvents = m_Events.Size();

	m_flStartTime = starttime;
	m_flEndTime = endtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CChoreoScene::CheckEventCompletion( void )
{
	CChoreoEvent *e;

	bool bAllCompleted = true;
	// check all items in the active pending queue
	for ( int i = 0; i < m_ActiveResumeConditions.Size(); i++ )
	{
		e = m_ActiveResumeConditions[ i ];

		bAllCompleted = bAllCompleted && e->CheckProcessing( m_pIChoreoEventCallback, this, m_flCurrentTime );
	}
	return bAllCompleted;
}




//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoScene::SimulationFinished( void )
{
	// Scene's linger for a little bit to allow things to settle
	// check for events that are still active...

	if ( m_flCurrentTime > m_flLatestTime )
	{
		if ( m_nActiveEvents != 0 )
		{
			return false;
		}

		return true;
	}
	if ( m_flCurrentTime < m_flEarliestTime )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CChoreoEvent *CChoreoScene::FindPauseBetweenTimes( float starttime, float endtime )
{
	CChoreoEvent *e;

	// Iterate through all events in the scene
	for ( int i = 0; i < m_PauseEvents.Size(); i++ )
	{
		e = m_PauseEvents[ i ];
		if ( !e )
			continue;

		Assert( e->GetType() == CChoreoEvent::SECTION );

		int time_is = IsTimeInRange( e->GetStartTime(), starttime, endtime );
		if ( IN_RANGE != time_is )
			continue;

		// Found a pause in between start and end time
		return e;
	}

	// No pause inside the specified time span
	return NULL;
}

int CChoreoScene::IsTimeInRange( float t, float starttime, float endtime )
{
	if ( t > endtime )
	{
		return AFTER_RANGE;
	}
	else if ( t < starttime )
	{
		return BEFORE_RANGE;
	}

	return IN_RANGE;
}

int CChoreoScene::EventThink( CChoreoEvent *e, float frame_start_time, float frame_end_time, bool playing_forward, PROCESSING_TYPE& disposition )
{
	disposition = PROCESSING_TYPE_IGNORE;
	int iret = 0;

	bool hasend = e->HasEndTime();
	float starttime, endtime;

	starttime = e->GetStartTime();
	endtime = hasend ? e->GetEndTime() : e->GetStartTime();

	if ( !playing_forward )
	{
		// Swap intervals
		float temp = frame_start_time;
		frame_start_time = frame_end_time;
		frame_end_time = temp;
	}

	bool suppressed = false;

	// Special processing
	switch ( e->GetType() )
	{
	default:
		break;
	case CChoreoEvent::SPEAK:
		// If it's a wav file, pre-queue the starting/endtime time by the sound system's
		//  current latency
		{
			if ( playing_forward )
			{
				starttime -= m_flSoundSystemLatency;


				// Search for pause condition in between the original time and the
				//  adjusted start time, but make sure that the pause event hasn't already triggered...
				CChoreoEvent *pauseEvent = FindPauseBetweenTimes( starttime, starttime + m_flSoundSystemLatency );
				if ( pauseEvent && 
					( frame_start_time <= pauseEvent->GetStartTime() ) )
				{
					pauseEvent->AddEventDependency( e );

					suppressed = true;
				}
			}
			/*
			else
			// Don't bother if playing backward!!!
			{
				endtime += m_flSoundSystemLatency;

				// Search for pause condition in between the original time and the
				//  adjusted start time
				CChoreoEvent *pauseEvent = FindPauseBetweenTimes( endtime - m_flSoundSystemLatency, endtime );
				if ( pauseEvent )
				{
					pauseEvent->AddEventDependency( e );

					suppressed = true;
				}
			}
			*/

			if ( !suppressed )
			{
				// if this SPEAK event starts before the beginning of the current loop, don't play the SPEAK event again in the loop
				if ( m_flStartLoopTime >= 0.f && starttime < m_flStartLoopTime )
				{
					return iret;
				}
			}
		}
		break;
	case CChoreoEvent::SUBSCENE:
		{
			if ( IsSubScene() )
			{
				suppressed = true;
			}
		}
		break;
	}

	if ( suppressed )
	{
		if ( e->IsProcessing() )
		{
			disposition = PROCESSING_TYPE_STOP;
		}
		return iret;
	}

	int where_is_event;

	if ( e->IsProcessing() )
	{
		where_is_event = IsTimeInRange( frame_start_time, starttime, endtime );
		if ( IN_RANGE == where_is_event )
		{
			disposition = PROCESSING_TYPE_CONTINUE;
			iret = 1;
		}
		else
		{
			disposition = PROCESSING_TYPE_STOP;
		}
	}
	else
	{

		// Is the event supposed to be active at this time
		where_is_event = IsTimeInRange( frame_start_time, starttime, endtime );

		if ( IN_RANGE == where_is_event )
		{
			if ( e->IsResumeCondition() )
			{
				disposition = PROCESSING_TYPE_START_RESUMECONDITION;
			}
			else
			{
				disposition = PROCESSING_TYPE_START;
			}
			iret = 1;
		}
		// See if it's a single fire event which should occur during this frame
		else if ( !hasend )
		{
			where_is_event = IsTimeInRange( starttime, frame_start_time, frame_end_time );
			if ( IN_RANGE == where_is_event )
			{
				disposition = PROCESSING_TYPE_START;
				iret = 1;
			}

		}
	}

	return iret;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &e0 - 
//			&e1 - 
// Output : static bool
//-----------------------------------------------------------------------------
bool CChoreoScene::EventLess( const CChoreoScene::ActiveList &al0, const CChoreoScene::ActiveList &al1 )
{
	CChoreoEvent *event0, *event1;
	event0 = const_cast< CChoreoEvent * >( al0.e );
	event1 = const_cast< CChoreoEvent * >( al1.e );

	if ( event0->GetStartTime() < event1->GetStartTime() )
	{
		return true;
	}

	if ( event0->GetStartTime() > event1->GetStartTime() )
	{
		return false;
	}

	// Check for end time overlap
	if ( event0->HasEndTime() && event1->HasEndTime() )
	{
		if ( event0->GetEndTime() > event1->GetEndTime() )
			return true;
		else if ( event0->GetEndTime() < event1->GetEndTime() )
			return false;
	}

	CChoreoActor *a0, *a1;
	a0 = event0->GetActor();
	a1 = event1->GetActor();

	// Start time equal, go to order in channel
	if ( !a0 || !a1 || a0 != a1 )
	{
		return strcmp( event0->GetName(), event1->GetName() ) == -1;
	}

	CChoreoChannel *c0 = event0->GetChannel();
	CChoreoChannel *c1 = event1->GetChannel();

	if ( !c0 || !c1 || c0 != c1 )
	{
		return strcmp( event0->GetName(), event1->GetName() ) == -1;
	}

	// Go by slot within channel
	int index0 = a0->FindChannelIndex( c0 );
	int index1 = a1->FindChannelIndex( c1 );

	return ( index0 < index1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoScene::ClearPauseEventDependencies()
{
	int c = m_PauseEvents.Count();
	for ( int i = 0 ; i < c; ++i )
	{
		CChoreoEvent *pause = m_PauseEvents[ i ];
		Assert( pause );
		pause->ClearEventDependencies();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pauseEvent - 
//			*suppressed - 
//-----------------------------------------------------------------------------
void CChoreoScene::AddPauseEventDependency( CChoreoEvent *pauseEvent, CChoreoEvent *suppressed )
{
	Assert( pauseEvent );
	Assert( pauseEvent != suppressed );
	pauseEvent->AddEventDependency( suppressed );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : curtime - 
//-----------------------------------------------------------------------------
void CChoreoScene::Think( float curtime )
{
	CChoreoEvent *e;

	float oldt = m_flCurrentTime;
	float dt;

	m_nActiveEvents = 0;

	ClearPauseEventDependencies();

	CUtlRBTree< ActiveList, int > pending(0,0,EventLess);

	// Handle loop events first:
	//float flLoopPoint = LoopThink( curtime );
	LoopThink( curtime );
	if ( m_flCurrentTime != oldt )
	{
		// We hit a loop, we need to adjust the times.
		//curtime = m_flCurrentTime + ( oldt - flLoopPoint ); // if we overshot, skip by how much we overshot
		curtime = m_flCurrentTime;
		Assert( curtime > 0.0f );
	}

	dt = curtime - oldt;
	oldt = m_flCurrentTime;

	bool playing_forward = ( dt >= 0.0f ) ? true : false;
	//if ( !playing_forward )
	//{
	//	Msg( "-----dt was negative. %f   oldt: %f   t: %f\n", dt, oldt, curtime );
	//}
	//else
	//{
	//	Msg( "+++++dt was positive. %f   oldt: %f   t: %f\n", dt, oldt, curtime );
	//}


	// Iterate through all events in the scene
	int i;
	for ( i = 0; i < m_Events.Size(); i++ )
	{
		e = m_Events[ i ];
		if ( !e )
			continue;

		PROCESSING_TYPE disposition;
		m_nActiveEvents += EventThink( e, m_flCurrentTime, curtime, playing_forward, disposition );

		if ( disposition != PROCESSING_TYPE_IGNORE )
		{
			ActiveList entry;

			entry.e		= e;
			entry.pt	= disposition;

			pending.Insert( entry );
		}
	}

	// Events are sorted start time and then by channel and actor slot or by name if those aren't equal
	i = pending.FirstInorder();
	while ( i != pending.InvalidIndex() )
	{
		ActiveList *entry = &pending[ i ];

		Assert( entry->e );

		ProcessActiveListEntry( entry );

		i = pending.NextInorder( i );
	}

	// If a Process call slams this time, don't override it!!!
	if ( oldt == m_flCurrentTime )
	{
		m_flCurrentTime = curtime;
	}

	// Still processing?
	if ( m_nActiveEvents )
	{
		m_flLastActiveTime = m_flCurrentTime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Loop points are handled prior to other events
// Input  : curtime - 
//-----------------------------------------------------------------------------
float CChoreoScene::LoopThink( float curtime )
{
	float oldt = m_flCurrentTime;
	float dt = curtime - oldt;

	bool playing_forward = ( dt >= 0.0f ) ? true : false;

	// Iterate through all events in the scene
	CChoreoEvent *e;
	int i;
	for ( i = 0; i < m_Events.Size(); i++ )
	{
		e = m_Events[ i ];
		if ( !e || e->GetType() != CChoreoEvent::LOOP )
			continue;

		PROCESSING_TYPE disposition;
		m_nActiveEvents += EventThink( e, m_flCurrentTime, curtime, playing_forward, disposition );

		if ( disposition != PROCESSING_TYPE_IGNORE )
		{
			ActiveList entry;

			entry.e		= e;
			entry.pt	= disposition;

			//float ret = (float)atof( e->GetParameters() );
			float ret = e->GetStartTime();
			ProcessActiveListEntry( &entry );

			return ret;
		}
	}

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : entry - 
//-----------------------------------------------------------------------------
void CChoreoScene::ProcessActiveListEntry( ActiveList *entry )
{
	const bool dump = false;
	if ( dump )
	{
		Msg( "%f == %s starting at %f (actor %p channel %p)\n",
			m_flCurrentTime, entry->e->GetName(), entry->e->GetStartTime(),
			entry->e->GetActor(), entry->e->GetChannel() );
	}

	switch ( entry->pt )
	{
	default:
	case PROCESSING_TYPE_IGNORE:
		{
			Assert( 0 );
		}
		break;
	case PROCESSING_TYPE_START:
	case PROCESSING_TYPE_START_RESUMECONDITION:
		{
			entry->e->StartProcessing( m_pIChoreoEventCallback, this, m_flCurrentTime );

			if ( entry->pt == PROCESSING_TYPE_START_RESUMECONDITION )
			{
				Assert( entry->e->IsResumeCondition() );
				m_ActiveResumeConditions.AddToTail( entry->e );
			}

			// This event can "pause" the scene, so we need to remember who "paused" the scene so that
			//  when we resume we can resume any suppressed events dependent on this pauser...
			if ( entry->e->GetType() == CChoreoEvent::SECTION )
			{
				// So this event should be in the pauseevents list, otherwise this'll be -1
				m_nLastPauseEvent = m_PauseEvents.Find( entry->e );
			}
		}
		break;
	case PROCESSING_TYPE_CONTINUE:
		{
			entry->e->ContinueProcessing( m_pIChoreoEventCallback, this, m_flCurrentTime );
		}
		break;
	case PROCESSING_TYPE_STOP:
		{
			entry->e->StopProcessing( m_pIChoreoEventCallback, this, m_flCurrentTime );
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoScene::GetTime( void )
{
	return m_flCurrentTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
//-----------------------------------------------------------------------------
void CChoreoScene::SetTime( float t )
{
	m_flCurrentTime = t;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
//-----------------------------------------------------------------------------
void CChoreoScene::LoopToTime( float t )
{
	m_flCurrentTime = t;

	m_flStartLoopTime = t;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pfn - 
//-----------------------------------------------------------------------------
void CChoreoScene::SetPrintFunc( void ( *pfn ) ( const char *fmt, ... ) )
{
	m_pfnPrint = pfn;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void CChoreoScene::RemoveActor( CChoreoActor *actor )
{
	int idx = FindActorIndex( actor );
	if ( idx == -1 )
		return;

	m_Actors.Remove( idx );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoScene::FindActorIndex( CChoreoActor *actor )
{
	for ( int i = 0; i < m_Actors.Size(); i++ )
	{
		if ( actor == m_Actors[ i ] )
		{
			return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : a1 - 
//			a2 - 
//-----------------------------------------------------------------------------
void CChoreoScene::SwapActors( int a1, int a2 )
{
	CChoreoActor *temp;

	temp = m_Actors[ a1 ];
	m_Actors[ a1 ] = m_Actors[ a2 ];
	m_Actors[ a2 ] = temp;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void CChoreoScene::DeleteReferencedObjects( CChoreoActor *actor )
{
	for ( int i = 0; i < actor->GetNumChannels(); i++ )
	{
		CChoreoChannel *channel = actor->GetChannel( i );
		actor->RemoveChannel( channel );
		
		DeleteReferencedObjects( channel );
	}

	DestroyActor( actor );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *channel - 
//-----------------------------------------------------------------------------
void CChoreoScene::DeleteReferencedObjects( CChoreoChannel *channel )
{
	for ( int i = 0; i < channel->GetNumEvents(); i++ )
	{
		CChoreoEvent *event = channel->GetEvent( i );
		channel->RemoveEvent( event );
		
		DeleteReferencedObjects( event );
	}

	DestroyChannel( channel );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CChoreoScene::DeleteReferencedObjects( CChoreoEvent *event )
{
	int idx = m_PauseEvents.Find( event );
	if ( idx != m_PauseEvents.InvalidIndex() )
	{
		m_PauseEvents.Remove( idx );
	}
	// Events don't reference anything lower
	DestroyEvent( event );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void CChoreoScene::DestroyActor( CChoreoActor *actor )
{
	int size = m_Actors.Size();
	for ( int i = size - 1; i >= 0; i-- )
	{
		CChoreoActor *a = m_Actors[ i ];
		if ( a == actor )
		{
			m_Actors.Remove( i );
		}
	}

	delete actor;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *channel - 
//-----------------------------------------------------------------------------
void CChoreoScene::DestroyChannel( CChoreoChannel *channel )
{
	int size = m_Channels.Size();
	for ( int i = size - 1; i >= 0; i-- )
	{
		CChoreoChannel *c = m_Channels[ i ];
		if ( c == channel )
		{
			m_Channels.Remove( i );
		}
	}

	delete channel;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - 
//-----------------------------------------------------------------------------
void CChoreoScene::DestroyEvent( CChoreoEvent *event )
{
	int size = m_Events.Size();
	for ( int i = size - 1; i >= 0; i-- )
	{
		CChoreoEvent *e = m_Events[ i ];
		if ( e == event )
		{
			m_Events.Remove( i );
		}
	}

	delete event;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoScene::ResumeSimulation( void )
{
	// If the thing that paused us was a SECTION pause event, then this will be set
	if ( m_nLastPauseEvent >= 0 && 
		 m_nLastPauseEvent < m_PauseEvents.Count() )
	{
		// Start any suppressed dependencies immediately, should only be .wav files!!!
		// These are .wav files which are placed at or just after the SECTION pause event
		//  in the .vcd, but due to the user's sound system latency, they would have triggered before the
		//  pause (we pre-queue sounds).  Since we suppressed that, we need to unsupress / start these sounds 
		//  now that the SECTION pause is being resumed from
		CUtlVector< CChoreoEvent * > deps;
		CChoreoEvent *pauseEvent = m_PauseEvents[ m_nLastPauseEvent ];
		Assert( pauseEvent );
		
		// Sanity check ( this should be about 1 tick usually  15 msec)
		float timeSincePaused = m_flCurrentTime - pauseEvent->GetStartTime();
		if ( fabs( timeSincePaused ) > 1.0f )
		{
			Assert( !"Resume simulation with unexpected pause event" );
		}
		
		// Snag any sounds which were suppressed by this issue
		pauseEvent->GetEventDependencies( deps );
		for ( int j = 0; j < deps.Count(); ++j )
		{
			CChoreoEvent *startEvent = deps[ j ];
			Assert( startEvent );
			// Start them now.  Yes, they won't pre-queue, but it's better than totally skipping the sound!!!
			startEvent->StartProcessing( m_pIChoreoEventCallback, this, m_flCurrentTime );
		}
	}

	// Reset section pause signal
	m_nLastPauseEvent = -1;

	m_ActiveResumeConditions.RemoveAll();
}

// Sound system needs to have sounds pre-queued by this much time
void CChoreoScene::SetSoundFileStartupLatency( float time )
{
	Assert( time >= 0 );
	m_flSoundSystemLatency = time;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : start - 
//			end - 
//-----------------------------------------------------------------------------
void CChoreoScene::GetSceneTimes( float& start, float& end )
{
	start	= m_flStartTime;
	end		= m_flEndTime;
}

//-----------------------------------------------------------------------------
// Purpose: Do housekeeping on times that are relative to tags
//-----------------------------------------------------------------------------
void CChoreoScene::ReconcileTags( void )
{
	for ( int i = 0 ; i < m_Actors.Size(); i++ )
	{
		CChoreoActor *a = m_Actors[ i ];
		if ( !a )
			continue;

		for ( int j = 0; j < a->GetNumChannels(); j++ )
		{
			CChoreoChannel *c = a->GetChannel( j );
			if ( !c )
				continue;
			
			for ( int k = 0 ; k < c->GetNumEvents(); k++ )
			{
				CChoreoEvent *e = c->GetEvent( k );
				if ( !e )
					continue;

				if ( !e->IsUsingRelativeTag() )
					continue;

				CEventRelativeTag *tag = FindTagByName( 
					e->GetRelativeWavName(),
					e->GetRelativeTagName() );

				if ( tag )
				{
					// Determine correct starting time based on tag
					float starttime = tag->GetStartTime();

					// Figure out delta
					float dt = starttime - e->GetStartTime();

					// Fix up start and possible end time
					e->OffsetTime( dt );
				}
				else
				{
					// The tag was missing!!! unflag it
					choreoprintf( 0, "Event %s was missing tag %s for wav %s\n",
						e->GetName(), e->GetRelativeWavName(), e->GetRelativeTagName() );

					e->SetUsingRelativeTag( false, "", "" );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *wavname - 
//			*name - 
// Output : CChoreoEvent
//-----------------------------------------------------------------------------
CChoreoEvent *CChoreoScene::FindTargetingEvent( const char *wavname, const char *name )
{
	for ( int i = 0 ; i < m_Actors.Size(); i++ )
	{
		CChoreoActor *a = m_Actors[ i ];
		if ( !a )
			continue;

		for ( int j = 0; j < a->GetNumChannels(); j++ )
		{
			CChoreoChannel *c = a->GetChannel( j );
			if ( !c )
				continue;
			
			for ( int k = 0 ; k < c->GetNumEvents(); k++ )
			{
				CChoreoEvent *e = c->GetEvent( k );
				if ( !e )
					continue;

				if ( !e->IsUsingRelativeTag() )
					continue;

				if ( stricmp( wavname, e->GetRelativeWavName() ) )
					continue;

				if ( stricmp( name, e->GetRelativeTagName() ) )
					continue;

				return e;
			}
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *wavname - 
//			*name - 
// Output : CEventRelativeTag
//-----------------------------------------------------------------------------
CEventRelativeTag *CChoreoScene::FindTagByName( const char *wavname, const char *name )
{
	for ( int i = 0 ; i < m_Actors.Size(); i++ )
	{
		CChoreoActor *a = m_Actors[ i ];
		if ( !a )
			continue;

		for ( int j = 0; j < a->GetNumChannels(); j++ )
		{
			CChoreoChannel *c = a->GetChannel( j );
			if ( !c )
				continue;
			
			for ( int k = 0 ; k < c->GetNumEvents(); k++ )
			{
				CChoreoEvent *e = c->GetEvent( k );
				if ( !e )
					continue;

				if ( e->GetType() != CChoreoEvent::SPEAK )
					continue;

				// Search for tag by name
				if ( !strstr( e->GetParameters(), wavname ) )
					continue;

				CEventRelativeTag *tag = e->FindRelativeTag( name );
				if ( !tag )
					continue;

				return tag;
			}
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//-----------------------------------------------------------------------------
void CChoreoScene::ExportEvents( const char *filename, CUtlVector< CChoreoEvent * >& events )
{
	if ( events.Size() <= 0 )
		return;

	// Create a serialization buffer
	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	FilePrintf( buf, 0, "// Choreo version 1:  <%i> Exported Events\n", events.Size() );

	// Save out the selected events.
	int i;
	for ( i = 0 ; i < events.Size(); i++ )
	{
		CChoreoEvent *e = events[ i ];
		if ( !e->GetActor() )
			continue;

		FileSaveEvent( buf, 0, e );
	}

	// Write it out baby
	FileHandle_t fh = g_pFullFileSystem->Open( filename, "wt" );
	if (fh)
	{
		g_pFullFileSystem->Write( buf.Base(), buf.TellPut(), fh );
		g_pFullFileSystem->Close(fh);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*channel - 
//			starttime - 
//-----------------------------------------------------------------------------
void CChoreoScene::ImportEvents( ISceneTokenProcessor *tokenizer, CChoreoActor *actor, CChoreoChannel *channel )
{
	m_pTokenizer = tokenizer;

	while ( 1 )
	{
		if ( !m_pTokenizer->GetToken( true ) )
		{
			break;
		}

		if ( strlen( m_pTokenizer->CurrentToken() ) <= 0 )
			break;

		if ( !Q_stricmp( m_pTokenizer->CurrentToken(), "event" ) )
		{
			ParseEvent( actor, channel );
		}
		else
		{
			m_pTokenizer->Error( "unexpected token %s\n", m_pTokenizer->CurrentToken() );
			break;
		}
	}

	// Fixup time tags
	ReconcileTags();
}

void CChoreoScene::SetSubScene( bool sub )
{
	m_bSubScene = sub;
}

bool CChoreoScene::IsSubScene( void ) const
{
	return m_bSubScene;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoScene::GetSceneFPS( void ) const
{
	return m_nSceneFPS;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fps - 
//-----------------------------------------------------------------------------
void CChoreoScene::SetSceneFPS( int fps )
{
	m_nSceneFPS = fps;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoScene::IsUsingFrameSnap( void ) const
{
	return m_bUseFrameSnap;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : snap - 
//-----------------------------------------------------------------------------
void CChoreoScene::SetUsingFrameSnap( bool snap )
{
	m_bUseFrameSnap = snap;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : t - 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoScene::SnapTime( float t )
{
	if ( !IsUsingFrameSnap() )
		return t;

	float fps = (float)GetSceneFPS();
	Assert( fps > 0 );

	int itime = (int)( t * fps + 0.5f );
	
	t = (float)itime / fps;

	// FIXME:  If FPS is set and "using grid", snap to proper fractional time value
	return t;
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChoreoScene::ReconcileGestureTimes()
{
	for ( int i = 0 ; i < m_Actors.Size(); i++ )
	{
		CChoreoActor *a = m_Actors[ i ];
		if ( !a )
			continue;

		for ( int j = 0; j < a->GetNumChannels(); j++ )
		{
			CChoreoChannel *c = a->GetChannel( j );
			if ( !c )
				continue;

			c->ReconcileGestureTimes();
		}
	}
}

int CChoreoScene::TimeZoomFirst()
{
	return m_TimeZoomLookup.First();
}

int CChoreoScene::TimeZoomNext( int i )
{
	return m_TimeZoomLookup.Next( i );
}
int CChoreoScene::TimeZoomInvalid() const
{
	return m_TimeZoomLookup.InvalidIndex();
}
char const *CChoreoScene::TimeZoomName( int i )
{
	return m_TimeZoomLookup.GetElementName( i );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tool - 
// Output : int
//-----------------------------------------------------------------------------
int CChoreoScene::GetTimeZoom( char const *tool )
{
	// If not present add it
	int idx = m_TimeZoomLookup.Find( tool );
	if ( idx == m_TimeZoomLookup.InvalidIndex() )
	{
		idx = m_TimeZoomLookup.Insert( tool, 100 );
	}

	return m_TimeZoomLookup[ idx ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tool - 
//			tz - 
//-----------------------------------------------------------------------------
void CChoreoScene::SetTimeZoom( char const *tool, int tz )
{
	// If not present add it
	int idx = m_TimeZoomLookup.Find( tool );
	if ( idx == m_TimeZoomLookup.InvalidIndex() )
	{
		idx = m_TimeZoomLookup.Insert( tool, 100 );
	}

	m_TimeZoomLookup[ idx ] = tz;
}

void CChoreoScene::ParseScaleSettings( ISceneTokenProcessor *tokenizer, CChoreoScene *scene )
{
	tokenizer->GetToken( true );

	if ( stricmp( tokenizer->CurrentToken(), "{" ) )
		tokenizer->Error( "expecting {\n" );
	
	while ( 1 )
	{
		// Parse until }
		tokenizer->GetToken( true );
		
		if ( strlen( tokenizer->CurrentToken() ) <= 0 )
		{
			tokenizer->Error( "expecting scalesettings data\n" );
			break;
		}
		
		if ( !Q_stricmp( tokenizer->CurrentToken(), "}" ) )
			break;

		char tool[ 256 ];
		Q_strncpy( tool, tokenizer->CurrentToken(), sizeof( tool ) );

		tokenizer->GetToken( false );

		int tz = Q_atoi( tokenizer->CurrentToken() );
		if ( tz <= 0 )
			tz = 100;

		scene->SetTimeZoom( tool, tz );
	}
}

// Merges two .vcd's together
bool CChoreoScene::Merge( CChoreoScene *other )
{
	int acount = 0;
	int ccount = 0;
	int ecount = 0;

	// Look for events that don't have actor/channel set
	int i;
	for ( i = 0 ; i < other->m_Events.Size(); i++ )
	{
		CChoreoEvent *e = other->m_Events[ i ];
		if ( e->GetActor() )
			continue;

		MEM_ALLOC_CREDIT();
		// Make a copy of the other event and add it to this scene
		CChoreoEvent *newEvent = AllocEvent();
		*newEvent = *e;
		newEvent->SetScene( this );
		ecount++;
	}

	for ( i = 0 ; i < other->m_Actors.Size(); i++ )
	{
		CChoreoActor *a = other->m_Actors[ i ];

		// See if that actor already exists
		bool newActor = false;
		CChoreoActor *destActor = FindActor( a->GetName() );
		if ( !destActor )
		{
			newActor = true;
			destActor = AllocActor();
			*destActor = *a;
			destActor->RemoveAllChannels();
			acount++;
		}

		// Now we have a destination actor, work on channels
		for ( int j = 0; j < a->GetNumChannels(); j++ )
		{
			CChoreoChannel *ch = a->GetChannel( j );

			bool newChannel = false;
			CChoreoChannel *destChannel = NULL;
			destChannel = destActor->FindChannel( ch->GetName() );
			if ( !destChannel )
			{
				destChannel = AllocChannel();
				*destChannel = *ch;
				destChannel->RemoveAllEvents();
				newChannel = true;
				ccount++;
			}

			if ( newChannel )
			{
				destActor->AddChannel( destChannel );
				destChannel->SetActor( destActor );
			}

			// Now we have a destination channel, work on events themselves
			for ( int k = 0 ; k < ch->GetNumEvents(); k++ )
			{
				CChoreoEvent *e = ch->GetEvent( k );

				// Just import them wholesale, no checking
				MEM_ALLOC_CREDIT();
				CChoreoEvent *newEvent = AllocEvent();
				*newEvent = *e;
				newEvent->SetScene( this );

				destChannel->AddEvent( newEvent );

				newEvent->SetChannel( destChannel );
				newEvent->SetActor( destActor );
				
				ecount++;
			}
		}
	}

	Msg( "Merged in (%i) actors, (%i) channels, and (%i) events\n",
		acount, ccount, ecount );

	return ( ecount || acount || ccount );
}

//-----------------------------------------------------------------------------
// Purpose: Updates master/slave status info per channel
//-----------------------------------------------------------------------------
void CChoreoScene::ReconcileCloseCaption()
{
	for ( int i = 0 ; i < m_Actors.Size(); i++ )
	{
		CChoreoActor *a = m_Actors[ i ];
		if ( !a )
			continue;

		for ( int j = 0; j < a->GetNumChannels(); j++ )
		{
			CChoreoChannel *c = a->GetChannel( j );
			if ( !c )
				continue;

			c->ReconcileCloseCaption();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
char const *CChoreoScene::GetFilename() const
{
	return m_szFileName;
}


void CChoreoScene::SetFileName( char const *fn )
{
	Q_strncpy( m_szFileName, fn, sizeof( m_szFileName ) );
}


bool CChoreoScene::GetPlayingSoundName( char *pchBuff, int iBuffLength )
{
	for ( int i = 0; i < m_Events.Size(); i++ )
	{
		CChoreoEvent *e = m_Events[ i ];
		if ( e->GetType() == CChoreoEvent::SPEAK && e->IsProcessing() )
		{
			Q_strncpy( pchBuff, e->GetParameters(), iBuffLength );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this scene has speech events that haven't played yet
//-----------------------------------------------------------------------------
bool CChoreoScene::HasUnplayedSpeech()
{
	for ( int i = 0; i < m_Events.Size(); i++ )
	{
		CChoreoEvent *e = m_Events[ i ];
		if ( e->GetType() == CChoreoEvent::SPEAK )
		{
			// Have we played it yet?
			if ( m_flCurrentTime < e->GetStartTime() )
				return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this scene has flex animation events that are playing
//-----------------------------------------------------------------------------
bool CChoreoScene::HasFlexAnimation()
{
	for ( int i = 0; i < m_Events.Size(); i++ )
	{
		CChoreoEvent *e = m_Events[ i ];
		if ( e->GetType() == CChoreoEvent::FLEXANIMATION )
		{
			// Have we played it yet?
			if ( m_flCurrentTime >= e->GetStartTime() && m_flCurrentTime <= e->GetEndTime() )
				return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CChoreoScene::SetBackground( bool bIsBackground )
{
	m_bIsBackground = bIsBackground;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CChoreoScene::IsBackground( )
{
	return m_bIsBackground;
}

bool CChoreoScene::HasEventsOfType( CChoreoEvent::EVENTTYPE type ) const
{
	return m_bitvecHasEventOfType.IsBitSet( type );
}

// ICurveDataAccessor method
bool CChoreoScene::CurveHasEndTime()
{
	return true;
}

int CChoreoScene::GetDefaultCurveType()
{
	return CURVE_CATMULL_ROM_TO_CATMULL_ROM;
}

bool CChoreoScene::SaveBinary( char const *pszBinaryFileName, char const *pPathID, unsigned int nTextVersionCRC, IChoreoStringPool *pStringPool )
{
	bool bret = false;

	CUtlBuffer buf;
	SaveToBinaryBuffer( buf, nTextVersionCRC, pStringPool );

	if ( g_pFullFileSystem->FileExists( pszBinaryFileName, pPathID ) && 
		 !g_pFullFileSystem->IsFileWritable( pszBinaryFileName, pPathID ) )
	{
		Warning( "Forcing '%s' to be writable!!!\n", pszBinaryFileName );
		g_pFullFileSystem->SetFileWritable( pszBinaryFileName, true, pPathID );
	}

	FileHandle_t fh = g_pFullFileSystem->Open( pszBinaryFileName, "wb", pPathID );
	if ( FILESYSTEM_INVALID_HANDLE != fh )
	{
		g_pFullFileSystem->Write( buf.Base(), buf.TellPut(), fh );
		g_pFullFileSystem->Close( fh );

		// Success
		bret = true;
	}
	else
	{
		Warning( "Unable to open '%s' for writing!!!\n", pszBinaryFileName );
	}

	return bret;
}

void CChoreoScene::SaveToBinaryBuffer( CUtlBuffer& buf, unsigned int nTextVersionCRC, IChoreoStringPool *pStringPool )
{
	buf.PutInt( SCENE_BINARY_TAG );
	buf.PutChar( SCENE_BINARY_VERSION );
	buf.PutInt( nTextVersionCRC );

	// Look for events that don't have actor/channel set
	CUtlVector< CChoreoEvent * > eventList;
	int i;
	for ( i = 0 ; i < m_Events.Size(); i++ )
	{
		CChoreoEvent *e = m_Events[ i ];
		if ( e->GetActor() )
			continue;

		eventList.AddToTail( e );
	}

	int c = eventList.Count();
	Assert( c <= 255 );
	buf.PutUnsignedChar( c );
	for ( i = 0; i < c; ++i )
	{
		CChoreoEvent *e = eventList[ i ];
		e->SaveToBuffer( buf, this, pStringPool );
	}

	// Now serialize the actors themselves
	CUtlVector< CChoreoActor * >	actorList;
	for ( i = 0 ; i < m_Actors.Size(); i++ )
	{
		CChoreoActor *a = m_Actors[ i ];
		if ( !a )
			continue;

		actorList.AddToTail( a );
	}

	c = actorList.Count();
	Assert( c <= 255 );
	buf.PutUnsignedChar( c );
	for ( i = 0; i < c; ++i )
	{
		CChoreoActor *a = actorList[ i ];
		a->SaveToBuffer( buf, this, pStringPool );
	}

	/*
	// compiled version strips out map name, only used by editor
	if ( m_szMapname[ 0 ] )
	{
		FilePrintf( buf, 0, "mapname \"%s\"\n", m_szMapname );
	}
	*/

	m_SceneRamp.SaveToBuffer( buf, pStringPool );

	/*
	// compiled version strips out scale settings fps and snap, only used by editor
	FileSaveScaleSettings( buf, 0, this );
	FilePrintf( buf, 0, "fps %i\n", m_nSceneFPS );
	FilePrintf( buf, 0, "snap %s\n", m_bUseFrameSnap ? "on" : "off" );
	*/
	buf.PutUnsignedChar( m_bIgnorePhonemes );
}

//-----------------------------------------------------------------------------
// Purpose: Static method to extract just the CRC from a binary .xcd file
// Input  : buf - 
//			crc - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoScene::GetCRCFromBinaryBuffer( CUtlBuffer& buf, unsigned int& crc )
{
	bool bret = false;

	int pos = buf.TellGet();

	int tag = buf.GetInt();
	if ( tag == SCENE_BINARY_TAG )
	{
		byte ver = buf.GetChar();
		if ( ver == SCENE_BINARY_VERSION )
		{
			bret = true;
			crc = (unsigned int)buf.GetInt();
		}
	}

	buf.SeekGet( CUtlBuffer::SEEK_HEAD, pos );

	return bret;
}

bool CChoreoScene::RestoreFromBinaryBuffer( CUtlBuffer& buf, char const *filename, IChoreoStringPool *pStringPool )
{
	Q_strncpy( m_szFileName, filename, sizeof( m_szFileName ) );

	int tag = buf.GetInt();
	if ( tag != SCENE_BINARY_TAG )
		return false;

	byte ver = buf.GetChar();
	if ( ver != SCENE_BINARY_VERSION )
		return false;

	// Skip the CRC
	buf.GetInt();

	int i;
	int eventCount = buf.GetUnsignedChar();
	for ( i = 0; i < eventCount; ++i )
	{
		MEM_ALLOC_CREDIT();
		CChoreoEvent *e = AllocEvent();
		Assert( e );
		
		if ( e->RestoreFromBuffer( buf, this, pStringPool ) )
		{
			continue;
		}

		return false;
	}

	int actorCount = buf.GetUnsignedChar();
	for ( i = 0; i < actorCount; ++i )
	{
		CChoreoActor *a = AllocActor();
		Assert( a );
		if ( a->RestoreFromBuffer( buf, this, pStringPool ) )
		{
			continue;
		}

		return false;
	}

	if ( !m_SceneRamp.RestoreFromBuffer( buf, pStringPool ) )
	{
		return false;
	}

	m_bIgnorePhonemes = ( buf.GetUnsignedChar( ) != 0 );

// FIXME:  Are these ever needed on restore?
//	ReconcileTags();
//	ReconcileGestureTimes();

	ReconcileCloseCaption();

	InternalDetermineEventTypes();

	if ( CChoreoScene::s_bEditingDisabled )
	{
		m_flPrecomputedStopTime = FindStopTime();
	}

	return true;
}
