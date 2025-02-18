//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CHOREOSCENE_H
#define CHOREOSCENE_H
#ifdef _WIN32
#pragma once
#endif

class CChoreoEvent;
class CChoreoChannel;
class CChoreoActor;
class IChoreoEventCallback;
class CEventRelativeTag;
class CUtlBuffer;
class CFlexAnimationTrack;
class ISceneTokenProcessor;
class IChoreoStringPool;

#include "tier1/utlvector.h"
#include "tier1/utldict.h"
#include "bitvec.h"
#include "expressionsample.h"
#include "choreoevent.h"

#define DEFAULT_SCENE_FPS	60
#define MIN_SCENE_FPS		10
#define MAX_SCENE_FPS		240

#define SCENE_BINARY_TAG		MAKEID( 'b', 'v', 'c', 'd' )
#define SCENE_BINARY_VERSION	0x04

//-----------------------------------------------------------------------------
// Purpose: Container for choreographed scene of events for actors
//-----------------------------------------------------------------------------
class CChoreoScene : public ICurveDataAccessor
{
	typedef enum 
	{
		PROCESSING_TYPE_IGNORE = 0,
		PROCESSING_TYPE_START,
		PROCESSING_TYPE_START_RESUMECONDITION,
		PROCESSING_TYPE_CONTINUE,
		PROCESSING_TYPE_STOP,
	} PROCESSING_TYPE;

	struct ActiveList
	{
		PROCESSING_TYPE		pt;
		CChoreoEvent		*e;
	};

public:
	// Construction
					CChoreoScene( IChoreoEventCallback *callback );
					~CChoreoScene( void );

	// Assignment
	CChoreoScene&	operator=(const CChoreoScene& src );

	// ICurveDataAccessor methods
	virtual float	GetDuration() { return FindStopTime(); };
	virtual bool	CurveHasEndTime();
	virtual int		GetDefaultCurveType();

	// Binary serialization
	bool			SaveBinary( char const *pszBinaryFileName, char const *pPathID, unsigned int nTextVersionCRC, IChoreoStringPool *pStringPool );
	void			SaveToBinaryBuffer( CUtlBuffer& buf, unsigned int nTextVersionCRC, IChoreoStringPool *pStringPool );
	bool			RestoreFromBinaryBuffer( CUtlBuffer& buf, char const *filename, IChoreoStringPool *pStringPool );
	static bool		GetCRCFromBinaryBuffer( CUtlBuffer& buf, unsigned int& crc );

	// We do some things differently while restoring from a save.
	inline void		SetRestoring( bool bRestoring );
	inline bool		IsRestoring();

	enum
	{
		MAX_SCENE_FILENAME = 128,
	};

	// Event callback handler
	void			SetEventCallbackInterface( IChoreoEventCallback *callback );

	// Loading
	bool			ParseFromBuffer( char const *pFilename, ISceneTokenProcessor *tokenizer );
	void			SetPrintFunc( void ( *pfn )( PRINTF_FORMAT_STRING const char *fmt, ... ) );

	// Saving
	bool			SaveToFile( const char *filename );
	bool			ExportMarkedToFile( const char *filename );
	void			MarkForSaveAll( bool mark );

	// Merges two .vcd's together, returns true if any data was merged
	bool			Merge( CChoreoScene *other );

	static void		FileSaveFlexAnimationTrack( CUtlBuffer& buf, int level, CFlexAnimationTrack *track, int nDefaultCurveType );
	static void		FileSaveFlexAnimations( CUtlBuffer& buf, int level, CChoreoEvent *e );
	static void		FileSaveRamp( CUtlBuffer& buf, int level, CChoreoEvent *e );
	void			FileSaveSceneRamp( CUtlBuffer& buf, int level );
	static void		FileSaveScaleSettings( CUtlBuffer& buf, int level, CChoreoScene *scene );

	static void		ParseFlexAnimations( ISceneTokenProcessor *tokenizer, CChoreoEvent *e, bool removeold = true );
	static void		ParseRamp( ISceneTokenProcessor *tokenizer, CChoreoEvent *e );
	static void		ParseSceneRamp( ISceneTokenProcessor *tokenizer, CChoreoScene *scene );
	static void		ParseScaleSettings( ISceneTokenProcessor *tokenizer, CChoreoScene *scene );
	static void		ParseEdgeInfo( ISceneTokenProcessor *tokenizer, EdgeInfo_t *edgeinfo );

	// Debugging
	void			SceneMsg( PRINTF_FORMAT_STRING const char *pFormat, ... );
	void			Print( void );

	// Sound system needs to have sounds pre-queued by this much time
	void			SetSoundFileStartupLatency( float time );

	// Simulation
	void			Think( float curtime );
	float			LoopThink( float curtime );
	void			ProcessActiveListEntry( ActiveList *entry );
	// Retrieves time in simulation
	float			GetTime( void );
	// Retrieves start/stop time for looped/debug scene
	void			GetSceneTimes( float& start, float& end );

	void			SetTime( float t );
	void			LoopToTime( float t );

	// Has simulation finished
	bool			SimulationFinished( void );
	// Reset simulation
	void			ResetSimulation( bool forward = true, float starttime = 0.0f, float endtime = 0.0f );
	// Find time at which last simulation event is triggered
	float			FindStopTime( void );

	void			ResumeSimulation( void );

	// Have all the pause events happened
	bool			CheckEventCompletion( void );

	// Find named actor in scene data
	CChoreoActor	*FindActor( const char *name );
	// Remove actor from scene
	void			RemoveActor( CChoreoActor *actor );
	// Find index for actor
	int				FindActorIndex( CChoreoActor *actor );

	// Swap actors in the data
	void			SwapActors( int a1, int a2 );

	// General data access
	int				GetNumEvents( void );
	CChoreoEvent	*GetEvent( int event );

	int				GetNumActors( void );
	CChoreoActor	*GetActor( int actor );
	
	int				GetNumChannels( void );
	CChoreoChannel	*GetChannel( int channel );

	// Object allocation/destruction
	void			DeleteReferencedObjects( CChoreoActor *actor );
	void			DeleteReferencedObjects( CChoreoChannel *channel );
	void			DeleteReferencedObjects( CChoreoEvent *event );

	CChoreoActor	*AllocActor( void );
	CChoreoChannel	*AllocChannel( void );
	CChoreoEvent	*AllocEvent( void );

	void			AddEventToScene( CChoreoEvent *event );
	void			AddActorToScene( CChoreoActor *actor );
	void			AddChannelToScene( CChoreoChannel *channel );

	// Fixup simulation times for channel gestures
	void			ReconcileGestureTimes( void );

	// Go through all elements and update relative tags, removing any orphaned
	// tags and updating the timestamp of normal tags
	void			ReconcileTags( void );
	CEventRelativeTag	*FindTagByName( const char *wavname, const char *name );
	CChoreoEvent	*FindTargetingEvent( const char *wavname, const char *name );

	// Used by UI to provide target actor names
	char const		*GetMapname( void );
	void			SetMapname( const char *name );

	void			ExportEvents( const char *filename, CUtlVector< CChoreoEvent * >& events );
	void			ImportEvents( ISceneTokenProcessor *tokenizer, CChoreoActor *actor, CChoreoChannel *channel );

	// Subscene support
	void			SetSubScene( bool sub );
	bool			IsSubScene( void ) const;

	int				GetSceneFPS( void ) const;
	void			SetSceneFPS( int fps );
	bool			IsUsingFrameSnap( void ) const;
	void			SetUsingFrameSnap( bool snap );

	float			SnapTime( float t );

	int				GetSceneRampCount( void ) { return m_SceneRamp.GetCount(); };
	CExpressionSample *GetSceneRamp( int index ) { return m_SceneRamp.Get( index ); };
	CExpressionSample *AddSceneRamp( float time, float value, bool selected ) { return m_SceneRamp.Add( time, value, selected ); };
	void			DeleteSceneRamp( int index ) { m_SceneRamp.Delete( index ); };
	void			ClearSceneRamp( void ) { m_SceneRamp.Clear(); };
	void			ResortSceneRamp( void ) { m_SceneRamp.Resort( this ); };

	CCurveData		*GetSceneRamp( void ) { return &m_SceneRamp; };


	// Global intensity for scene
	float			GetSceneRampIntensity( float time ) { return m_SceneRamp.GetIntensity( this, time ); }

	int				GetTimeZoom( char const *tool );
	void			SetTimeZoom( char const *tool, int tz );
	int				TimeZoomFirst();
	int				TimeZoomNext( int i );
	int				TimeZoomInvalid() const;
	char const		*TimeZoomName( int i );

	void			ReconcileCloseCaption();

	char const		*GetFilename() const;
	void			SetFileName( char const *fn );

	bool			GetPlayingSoundName( char *pchBuff, int iBuffLength );
	bool			HasUnplayedSpeech();
	bool			HasFlexAnimation();
	void			SetBackground( bool bIsBackground );
	bool			IsBackground( void );

	void			ClearPauseEventDependencies();

	bool			HasEventsOfType( CChoreoEvent::EVENTTYPE type ) const;
	void			RemoveEventsExceptTypes( int* typeList, int count );

	void IgnorePhonemes( bool bIgnore );
	bool ShouldIgnorePhonemes() const;

	// This is set by the engine to signify that we're not modifying the data and 
	//  therefore we can precompute the end time
	static	bool	s_bEditingDisabled; 

private:

	// Simulation stuff
	enum
	{
		IN_RANGE = 0,
		BEFORE_RANGE,
		AFTER_RANGE
	};

	int				IsTimeInRange( float t, float starttime, float endtime );

	static bool EventLess( const CChoreoScene::ActiveList &al0, const CChoreoScene::ActiveList &al1 );

	int				EventThink( CChoreoEvent *e, 
						float frame_start_time, 
						float frame_end_time,
						bool playing_forward, PROCESSING_TYPE& disposition );

	// Prints to debug console, etc
	void			choreoprintf( int level, PRINTF_FORMAT_STRING const char *fmt, ... );

	// Initialize scene
	void			Init( IChoreoEventCallback *callback );

	float			FindAdjustedStartTime( void );
	float			FindAdjustedEndTime( void );

	CChoreoEvent	*FindPauseBetweenTimes( float starttime, float endtime );

	// Parse scenes from token buffer
	CChoreoEvent	*ParseEvent( CChoreoActor *actor, CChoreoChannel *channel );
	CChoreoChannel	*ParseChannel( CChoreoActor *actor );
	CChoreoActor	*ParseActor( void );
	   
	void			ParseFPS( void );
	void			ParseSnap( void );
	void			ParseIgnorePhonemes( void );

	// Map file for retrieving named objects
	void			ParseMapname( void );
	// When previewing actor in hlfaceposer, this is the model to associate
	void			ParseFacePoserModel( CChoreoActor *actor );

	// Print to printfunc
	void			PrintEvent( int level, CChoreoEvent *e );
	void			PrintChannel( int level, CChoreoChannel *c );
	void			PrintActor( int level, CChoreoActor *a );

	// File I/O
public:
	static void		FilePrintf( CUtlBuffer& buf, int level, PRINTF_FORMAT_STRING const char *fmt, ... );
private:
	void			FileSaveEvent( CUtlBuffer& buf, int level, CChoreoEvent *e );
	void			FileSaveChannel( CUtlBuffer& buf, int level, CChoreoChannel *c );
	void			FileSaveActor( CUtlBuffer& buf, int level, CChoreoActor *a );
	void			FileSaveHeader( CUtlBuffer& buf );

	// Object destruction
	void			DestroyActor( CChoreoActor *actor );
	void			DestroyChannel( CChoreoChannel *channel );
	void			DestroyEvent( CChoreoEvent *event );


	void			AddPauseEventDependency( CChoreoEvent *pauseEvent, CChoreoEvent *suppressed );

	void			InternalDetermineEventTypes();

	// Global object storage
	CUtlVector < CChoreoEvent * >	m_Events;
	CUtlVector < CChoreoActor * >	m_Actors;
	CUtlVector < CChoreoChannel	* >	m_Channels;

	// These are just pointers, the actual objects are in m_Events
	CUtlVector < CChoreoEvent * >	m_ResumeConditions;
	// These are just pointers, the actual objects are in m_Events
	CUtlVector < CChoreoEvent * >	m_ActiveResumeConditions;
	// These are just pointers, the actual objects are in m_Events
	CUtlVector < CChoreoEvent * >	m_PauseEvents;

	// Current simulation time
	float			m_flCurrentTime;

	float			m_flStartLoopTime;

	float			m_flStartTime;
	float			m_flEndTime;

	float			m_flEarliestTime;
	float			m_flLatestTime;
	int				m_nActiveEvents;

	// Wave file playback needs to issue play commands a bit ahead of time
	//  in order to hit exact marks
	float			m_flSoundSystemLatency;

	// Scene's linger a bit after finishing to let blends reset themselves
	float			m_flLastActiveTime;

	// Print callback function
	void ( *m_pfnPrint )( PRINTF_FORMAT_STRING const char *fmt, ... );

	IChoreoEventCallback			*m_pIChoreoEventCallback;

	ISceneTokenProcessor			*m_pTokenizer;

	enum
	{
		MAX_MAPNAME = 128
	};

	char			m_szMapname[ MAX_MAPNAME ];

	int				m_nSceneFPS;

	CCurveData		m_SceneRamp;

	CUtlDict< int, int >	m_TimeZoomLookup;
	char			m_szFileName[ MAX_SCENE_FILENAME ];

	CBitVec< CChoreoEvent::NUM_TYPES > m_bitvecHasEventOfType;

	// tag to suppress vcd when others are playing
	bool			m_bIsBackground : 1;
	bool			m_bIgnorePhonemes : 1;
	bool			m_bSubScene : 1;
	bool			m_bUseFrameSnap : 1;
	bool			m_bRestoring : 1;

	int				m_nLastPauseEvent;
	// This only gets updated if it's loaded from a buffer which means we're not in an editor
	float			m_flPrecomputedStopTime;
};


bool CChoreoScene::IsRestoring()
{
	return m_bRestoring;
}


void CChoreoScene::SetRestoring( bool bRestoring )
{
	m_bRestoring = bRestoring;
}


abstract_class IChoreoStringPool
{
public:
	virtual short	FindOrAddString( const char *pString ) = 0;
	virtual bool	GetString( short stringId, char *buff, int buffSize ) = 0; 	
};

CChoreoScene *ChoreoLoadScene( 
	char const *filename,
	IChoreoEventCallback *callback, 
	ISceneTokenProcessor *tokenizer,
	void ( *pfn ) ( PRINTF_FORMAT_STRING const char *fmt, ... ) );

bool IsBufferBinaryVCD( char *pBuffer, int bufferSize );

#endif // CHOREOSCENE_H
