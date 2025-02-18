//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include <stdarg.h>
#include "baseflex.h"
#include "entitylist.h"
#include "choreoevent.h"
#include "choreoactor.h"
#include "choreochannel.h"
#include "choreoscene.h"
#include "studio.h"
#include "networkstringtable_gamedll.h"
#include "ai_basenpc.h"
#include "engine/IEngineSound.h"
#include "ai_navigator.h"
#include "saverestore_utlvector.h"
#include "ai_baseactor.h"
#include "AI_Criteria.h"
#include "tier1/strtools.h"
#include "checksum_crc.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "utlbuffer.h"
#include "tier0/icommandline.h"
#include "sceneentity.h"
#include "datacache/idatacache.h"
#include "dt_utlvector_send.h"
#include "ichoreoeventcallback.h"
#include "scenefilecache/ISceneFileCache.h"
#include "SceneCache.h"
#include "scripted.h"
#include "env_debughistory.h"
#include "team.h"

#ifdef HL2_EPISODIC
#include "npc_alyx_episodic.h"
#endif // HL2_EPISODIC

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ISoundEmitterSystemBase *soundemitterbase;
extern ISceneFileCache *scenefilecache;

class CSceneEntity;
class CBaseFlex;

// VCDS are loaded from their compiled/binary format (much faster)
// Requies vcds be saved as compiled assets
//#define COMPILED_VCDS 1

static ConVar scene_forcecombined( "scene_forcecombined", "0", 0, "When playing back, force use of combined .wav files even in english." );
static ConVar scene_maxcaptionradius( "scene_maxcaptionradius", "1200", 0, "Only show closed captions if recipient is within this many units of speaking actor (0==disabled)." );

// Assume sound system is 100 msec lagged (only used if we can't find snd_mixahead cvar!)
#define SOUND_SYSTEM_LATENCY_DEFAULT ( 0.1f )

// Think every 50 msec (FIXME: Try 10hz?)
#define SCENE_THINK_INTERVAL 0.001 // FIXME: make scene's think in concert with their npc's	

#define FINDNAMEDENTITY_MAX_ENTITIES	32		// max number of entities to be considered for random entity selection in FindNamedEntity

// List of the last 5 lines of speech from NPCs for bug reports
static recentNPCSpeech_t speechListSounds[ SPEECH_LIST_MAX_SOUNDS ] = { { 0, "", "" }, { 0, "", "" }, { 0, "", "" }, { 0, "", "" }, { 0, "", "" } };
static int  speechListIndex = 0;

// Only allow scenes to change their pitch within a range of values
#define SCENE_MIN_PITCH	0.25f
#define SCENE_MAX_PITCH 2.5f

//===========================================================================================================
// SCENE LIST MANAGER
//===========================================================================================================
#define SCENE_LIST_MANAGER_MAX_SCENES	16

//-----------------------------------------------------------------------------
// Purpose: Entity that manages a list of scenes
//-----------------------------------------------------------------------------
class CSceneListManager : public CLogicalEntity
{
	DECLARE_CLASS( CSceneListManager, CLogicalEntity );
public:
	DECLARE_DATADESC();

	virtual void Activate( void );

	void		 ShutdownList( void );
	void		 SceneStarted( CBaseEntity *pSceneOrManager );
	void		 AddListManager( CSceneListManager *pManager );
	void		 RemoveScene( int iIndex );

	// Inputs
	void	InputShutdown( inputdata_t &inputdata );

private:
	CUtlVector< CHandle< CSceneListManager > >	m_hListManagers;
	string_t				m_iszScenes[SCENE_LIST_MANAGER_MAX_SCENES];
	EHANDLE					m_hScenes[SCENE_LIST_MANAGER_MAX_SCENES];
};

//-----------------------------------------------------------------------------
// Purpose: This class exists solely to call think on all scene entities in a deterministic order
//-----------------------------------------------------------------------------
class CSceneManager : public CBaseEntity
{
	DECLARE_CLASS( CSceneManager, CBaseEntity );
	DECLARE_DATADESC();

public:
	virtual void			Spawn()
	{
		BaseClass::Spawn();
		SetNextThink( gpGlobals->curtime );
	}

	virtual int				ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_DONT_SAVE; }

	virtual void			Think();

			void			ClearAllScenes();

			void			AddSceneEntity( CSceneEntity *scene );
			void			RemoveSceneEntity( CSceneEntity *scene );

			void			QueueRestoredSound( CBaseFlex *actor, char const *soundname, soundlevel_t soundlevel, float time_in_past );

			void			OnClientActive( CBasePlayer *player );
			
			void			RemoveActorFromScenes( CBaseFlex *pActor, bool bInstancedOnly, bool bNonIdleOnly, const char *pszThisSceneOnly );
			void			RemoveScenesInvolvingActor( CBaseFlex *pActor );
			void			PauseActorsScenes( CBaseFlex *pActor, bool bInstancedOnly  );
			bool			IsInInterruptableScenes( CBaseFlex *pActor );
			void			ResumeActorsScenes( CBaseFlex *pActor, bool bInstancedOnly  );
			void			QueueActorsScenesToResume( CBaseFlex *pActor, bool bInstancedOnly  );
			bool			IsRunningScriptedScene( CBaseFlex *pActor, bool bIgnoreInstancedScenes );
			bool			IsRunningScriptedSceneAndNotPaused( CBaseFlex *pActor, bool bIgnoreInstancedScenes );
			bool			IsRunningScriptedSceneWithSpeech( CBaseFlex *pActor, bool bIgnoreInstancedScenes );
			bool			IsRunningScriptedSceneWithSpeechAndNotPaused( CBaseFlex *pActor, bool bIgnoreInstancedScenes );


private:

	struct CRestoreSceneSound
	{
		CRestoreSceneSound()
		{
			actor = NULL;
			soundname[ 0 ] = NULL;
			soundlevel = SNDLVL_NORM;
			time_in_past = 0.0f;
		}

		CHandle< CBaseFlex >	actor;
		char					soundname[ 128 ];
		soundlevel_t			soundlevel;
		float					time_in_past;
	};

	CUtlVector< CHandle< CSceneEntity > >	m_ActiveScenes;

	CUtlVector< CRestoreSceneSound >		m_QueuedSceneSounds;
};

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CSceneManager )

	DEFINE_UTLVECTOR( m_ActiveScenes,	FIELD_EHANDLE ),
	// DEFINE_FIELD( m_QueuedSceneSounds, CUtlVector < CRestoreSceneSound > ),  // Don't save/restore this, it's created and used by OnRestore only

END_DATADESC()

#ifdef DISABLE_DEBUG_HISTORY
#define LocalScene_Printf Scene_Printf
#else
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFormat - 
//			... - 
// Output : static void
//-----------------------------------------------------------------------------
void LocalScene_Printf( const char *pFormat, ... )
{
	va_list marker;
	char msg[8192];

	va_start(marker, pFormat);
	Q_vsnprintf(msg, sizeof(msg), pFormat, marker);
	va_end(marker);	

	Scene_Printf( "%s", msg );
	ADD_DEBUG_HISTORY( HISTORY_SCENE_PRINT, UTIL_VarArgs( "(%0.2f) %s", gpGlobals->curtime, msg ) );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//			**buffer - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CopySceneFileIntoMemory( char const *pFilename, void **pBuffer, int *pSize )
{
	size_t bufSize = scenefilecache->GetSceneBufferSize( pFilename );
	if ( bufSize > 0 )
	{
		*pBuffer = new byte[bufSize];
		*pSize = bufSize;
		return scenefilecache->GetSceneData( pFilename, (byte *)(*pBuffer), bufSize );
	}

	*pBuffer = 0;
	*pSize = 0;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FreeSceneFileMemory( void *buffer )
{
	delete[] (byte*) buffer;
}

//-----------------------------------------------------------------------------
// Binary compiled VCDs get their strings from a pool
//-----------------------------------------------------------------------------
class CChoreoStringPool : public IChoreoStringPool
{
public:
	short FindOrAddString( const char *pString )
	{
		// huh?, no compilation at run time, only fetches
		Assert( 0 );
		return -1;
	}

	bool GetString( short stringId, char *buff, int buffSize )
	{
		// fetch from compiled pool
		const char *pString = scenefilecache->GetSceneString( stringId );
		if ( !pString )
		{
			V_strncpy( buff, "", buffSize );
			return false;
		}
		V_strncpy( buff, pString, buffSize );
		return true;
	} 	
};
CChoreoStringPool g_ChoreoStringPool;

//-----------------------------------------------------------------------------
// Purpose: Singleton scene manager.  Created by first placed scene or recreated it it's deleted for some unknown reason
// Output : CSceneManager
//-----------------------------------------------------------------------------
CSceneManager *GetSceneManager()
{
	// Create it if it doesn't exist
	static CHandle< CSceneManager >	s_SceneManager;
	if ( s_SceneManager == NULL )
	{
		s_SceneManager = ( CSceneManager * )CreateEntityByName( "scene_manager" );
		Assert( s_SceneManager );
		if ( s_SceneManager )
		{
			s_SceneManager->Spawn();
		}
	}

	Assert( s_SceneManager );
	return s_SceneManager;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
//-----------------------------------------------------------------------------
void SceneManager_ClientActive( CBasePlayer *player )
{
	Assert( GetSceneManager() );

	if ( GetSceneManager() )
	{
		GetSceneManager()->OnClientActive( player );
	}
}

//-----------------------------------------------------------------------------
// Purpose: FIXME, need to deal with save/restore
//-----------------------------------------------------------------------------
class CSceneEntity : public CPointEntity, public IChoreoEventCallback
{
	friend class CInstancedSceneEntity;
public:

	enum
	{
		SCENE_ACTION_UNKNOWN = 0,
		SCENE_ACTION_CANCEL,
		SCENE_ACTION_RESUME,
	};

	enum
	{
		SCENE_BUSYACTOR_DEFAULT = 0,
		SCENE_BUSYACTOR_WAIT,
		SCENE_BUSYACTOR_INTERRUPT,
		SCENE_BUSYACTOR_INTERRUPT_CANCEL,
	};




	DECLARE_CLASS( CSceneEntity, CPointEntity );
	DECLARE_SERVERCLASS();
	DECLARE_ENT_SCRIPTDESC();

							CSceneEntity( void );
							~CSceneEntity( void );
				
	// From IChoreoEventCallback
	virtual void			StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void			EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void			ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual bool			CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );


	virtual int				UpdateTransmitState();
	virtual int				ShouldTransmit( const CCheckTransmitInfo *pInfo );

	void					SetRecipientFilter( IRecipientFilter *filter );

	virtual void			Activate();

	virtual	void			Precache( void );
	virtual void			Spawn( void );
	virtual void			UpdateOnRemove( void );

	virtual void			OnRestore();
	virtual void			OnLoaded();

	DECLARE_DATADESC();

	virtual void			OnSceneFinished( bool canceled, bool fireoutput );

	virtual void			DoThink( float frametime );
	virtual void			PauseThink( void );

	bool					IsPlayingBack() const			{ return m_bIsPlayingBack; }
	bool					IsPaused() const				{ return m_bPaused; }
	bool					IsMultiplayer() const			{ return m_bMultiplayer; }

	bool					IsInterruptable();
	virtual void			ClearInterrupt();
	virtual void			CheckInterruptCompletion();

	virtual bool			InterruptThisScene( CSceneEntity *otherScene );
	void					RequestCompletionNotification( CSceneEntity *otherScene );

	virtual void			NotifyOfCompletion( CSceneEntity *interruptor );

	void					AddListManager( CSceneListManager *pManager );

	void					ClearActivatorTargets( void );

	void					SetBreakOnNonIdle( bool bBreakOnNonIdle ) { m_bBreakOnNonIdle = bBreakOnNonIdle; }
	bool					ShouldBreakOnNonIdle( void ) { return m_bBreakOnNonIdle; }

	// Inputs
	void InputStartPlayback( inputdata_t &inputdata );
	void InputPausePlayback( inputdata_t &inputdata );
	void InputResumePlayback( inputdata_t &inputdata );
	void InputCancelPlayback( inputdata_t &inputdata );
	void InputCancelAtNextInterrupt( inputdata_t &inputdata );
	void InputPitchShiftPlayback( inputdata_t &inputdata );
	void InputTriggerEvent( inputdata_t &inputdata );

	// If the scene is playing, finds an actor in the scene who can respond to the specified concept token
	void InputInterjectResponse( inputdata_t &inputdata );

	// If this scene is waiting on an actor, give up and quit trying.
	void InputStopWaitingForActor( inputdata_t &inputdata );

	virtual void StartPlayback( void );
	virtual void PausePlayback( void );
	virtual void ResumePlayback( void );
	virtual void CancelPlayback( void );
	virtual void PitchShiftPlayback( float fPitch );
	virtual void QueueResumePlayback( void );

	bool		 ValidScene() const;

	// Scene load/unload
	static CChoreoScene			*LoadScene( const char *filename, IChoreoEventCallback *pCallback );

	void					UnloadScene( void );

	struct SpeakEventSound_t
	{
		CUtlSymbol	m_Symbol;
		float		m_flStartTime;
	};

	static bool SpeakEventSoundLessFunc( const SpeakEventSound_t& lhs, const SpeakEventSound_t& rhs );

	bool					GetSoundNameForPlayer( CChoreoEvent *event, CBasePlayer *player, char *buf, size_t buflen, CBaseEntity *pActor );

	void					BuildSortedSpeakEventSoundsPrefetchList( 
								CChoreoScene *scene, 
								CUtlSymbolTable& table, 
								CUtlRBTree< SpeakEventSound_t >& soundnames, 
								float timeOffset );
	void					PrefetchSpeakEventSounds( CUtlSymbolTable& table, CUtlRBTree< SpeakEventSound_t >& soundnames );

	// Event handlers
	virtual void			DispatchStartExpression( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchEndExpression( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchStartFlexAnimation( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchEndFlexAnimation( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchStartGesture( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchEndGesture( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchStartLookAt( CChoreoScene *scene, CBaseFlex *actor, CBaseEntity *actor2, CChoreoEvent *event );
	virtual void			DispatchEndLookAt( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchStartMoveTo( CChoreoScene *scene, CBaseFlex *actor, CBaseEntity *actor2, CChoreoEvent *event );
	virtual void			DispatchEndMoveTo( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual	void			DispatchStartSpeak( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event, soundlevel_t iSoundlevel );
	virtual void			DispatchEndSpeak( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchStartFace( CChoreoScene *scene, CBaseFlex *actor, CBaseEntity *actor2, CChoreoEvent *event );
	virtual void			DispatchEndFace( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchStartSequence( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchEndSequence( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchStartSubScene( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchStartInterrupt( CChoreoScene *scene, CChoreoEvent *event );
	virtual void			DispatchEndInterrupt( CChoreoScene *scene, CChoreoEvent *event );
	virtual void			DispatchStartGeneric( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchEndGeneric( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );

	// NPC can play interstitial vcds (such as responding to the player doing something during a scene)
	virtual void			DispatchStartPermitResponses( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchEndPermitResponses( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event );


	// Global events
	virtual void			DispatchProcessLoop( CChoreoScene *scene, CChoreoEvent *event );
	virtual void			DispatchPauseScene( CChoreoScene *scene, const char *parameters );
	virtual void			DispatchStopPoint( CChoreoScene *scene, const char *parameters );

	virtual float			EstimateLength( void );
	
	void					CancelIfSceneInvolvesActor( CBaseEntity *pActor );
	bool					InvolvesActor( CBaseEntity *pActor );		// NOTE: returns false if scene hasn't loaded yet

	void					GenerateSoundScene( CBaseFlex *pActor, const char *soundname );

	virtual float			GetPostSpeakDelay()	{ return 1.0; }

	bool					HasUnplayedSpeech( void );
	bool					HasFlexAnimation( void );

	void					SetCurrentTime( float t, bool forceClientSync );

	void					InputScriptPlayerDeath( inputdata_t &inputdata );

	void					InputSetTarget1( inputdata_t &inputdata );
	void					InputSetTarget2( inputdata_t &inputdata );
	void					InputSetTarget3( inputdata_t &inputdata );
	void					InputSetTarget4( inputdata_t &inputdata );
	void					InputSetTarget5( inputdata_t &inputdata );
	void					InputSetTarget6( inputdata_t &inputdata );
	void					InputSetTarget7( inputdata_t &inputdata );
	void					InputSetTarget8( inputdata_t &inputdata );

	void					AddBroadcastTeamTarget( int nTeamIndex );
	void					RemoveBroadcastTeamTarget( int nTeamIndex );

// Data
public:
	string_t				m_iszSceneFile;

	string_t				m_iszResumeSceneFile;
	EHANDLE					m_hWaitingForThisResumeScene;
	bool					m_bWaitingForResumeScene;

	string_t				m_iszTarget1;
	string_t				m_iszTarget2;
	string_t				m_iszTarget3;
	string_t				m_iszTarget4;
	string_t				m_iszTarget5;
	string_t				m_iszTarget6;
	string_t				m_iszTarget7;
	string_t				m_iszTarget8;

	EHANDLE					m_hTarget1;
	EHANDLE					m_hTarget2;
	EHANDLE					m_hTarget3;
	EHANDLE					m_hTarget4;
	EHANDLE					m_hTarget5;
	EHANDLE					m_hTarget6;
	EHANDLE					m_hTarget7;
	EHANDLE					m_hTarget8;

	CNetworkVar( bool, m_bIsPlayingBack );
	CNetworkVar( bool, m_bPaused );
	CNetworkVar( bool, m_bMultiplayer );
	CNetworkVar( float, m_flForceClientTime );

	float					m_flCurrentTime;
	float					m_flFrameTime;
	bool					m_bCancelAtNextInterrupt;

	float					m_fPitch;

	bool					m_bAutomated;
	int						m_nAutomatedAction;
	float					m_flAutomationDelay;
	float					m_flAutomationTime;

	// A pause from an input requires another input to unpause (it's a hard pause)
	bool					m_bPausedViaInput;

	// Waiting for the actor to be able to speak.
	bool					m_bWaitingForActor;

	// Waiting for a point at which we can interrupt our actors
	bool					m_bWaitingForInterrupt;
	bool					m_bInterruptedActorsScenes;

	bool					m_bBreakOnNonIdle;

public:
	virtual CBaseFlex		*FindNamedActor( int index );
	virtual CBaseFlex		*FindNamedActor( CChoreoActor *pChoreoActor );
	virtual CBaseFlex		*FindNamedActor( const char *name );
	virtual CBaseEntity		*FindNamedEntity( const char *name, CBaseEntity *pActor = NULL, bool bBaseFlexOnly = false, bool bUseClear = false );
	CBaseEntity				*FindNamedTarget( string_t iszTarget, bool bBaseFlexOnly = false );
	virtual CBaseEntity		*FindNamedEntityClosest( const char *name, CBaseEntity *pActor = NULL, bool bBaseFlexOnly = false, bool bUseClear = false, const char *pszSecondary = NULL );

	HSCRIPT					ScriptFindNamedEntity( const char *name );
	bool					ScriptLoadSceneFromString( const char * pszFilename, const char *pszData );
private:

	CUtlVector< CHandle< CBaseFlex > >		m_hActorList;
	CUtlVector< CHandle< CBaseEntity > >	m_hRemoveActorList;

private:

	inline void				SetRestoring( bool bRestoring );

	// Prevent derived classed from using this!
	virtual void			Think( void ) {};


	void					ClearSceneEvents( CChoreoScene *scene, bool canceled );
	void					ClearSchedules( CChoreoScene *scene );

	float					GetSoundSystemLatency( void );
	void					PrecacheScene( CChoreoScene *scene );

	CChoreoScene			*GenerateSceneForSound( CBaseFlex *pFlexActor, const char *soundname );

	bool					CheckActors();

	void					PrefetchAnimBlocks( CChoreoScene *scene );

	bool					ShouldNetwork() const;
	// Set if we tried to async the scene but the FS returned that the data was not loadable
	bool					m_bSceneMissing;

	CChoreoScene			*m_pScene;
	CNetworkVar( int, m_nSceneStringIndex );

	static const ConVar		*m_pcvSndMixahead;

	COutputEvent			m_OnStart;
	COutputEvent			m_OnCompletion;
	COutputEvent			m_OnCanceled;
	COutputEvent			m_OnTrigger1;
	COutputEvent			m_OnTrigger2;
	COutputEvent			m_OnTrigger3;
	COutputEvent			m_OnTrigger4;
	COutputEvent			m_OnTrigger5;
	COutputEvent			m_OnTrigger6;
	COutputEvent			m_OnTrigger7;
	COutputEvent			m_OnTrigger8;
	COutputEvent			m_OnTrigger9;
	COutputEvent			m_OnTrigger10;
	COutputEvent			m_OnTrigger11;
	COutputEvent			m_OnTrigger12;
	COutputEvent			m_OnTrigger13;
	COutputEvent			m_OnTrigger14;
	COutputEvent			m_OnTrigger15;
	COutputEvent			m_OnTrigger16;

	int						m_nInterruptCount;
	bool					m_bInterrupted;
	CHandle< CSceneEntity >	m_hInterruptScene;

	bool					m_bCompletedEarly;

	bool					m_bInterruptSceneFinished;
	CUtlVector< CHandle< CSceneEntity > >	m_hNotifySceneCompletion;
	CUtlVector< CHandle< CSceneListManager > >	m_hListManagers;

	bool					m_bRestoring;

	bool					m_bGenerated;
	string_t				m_iszSoundName;
	CHandle< CBaseFlex >	m_hActor;

	EHANDLE					m_hActivator;

	int						m_BusyActor;

	int						m_iPlayerDeathBehavior;

	CRecipientFilter		*m_pRecipientFilter;

public:
	void					SetBackground( bool bIsBackground );
	bool					IsBackground( void );
};

LINK_ENTITY_TO_CLASS( logic_choreographed_scene, CSceneEntity );
LINK_ENTITY_TO_CLASS( scripted_scene, CSceneEntity );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CSceneEntity, DT_SceneEntity )
	SendPropInt(SENDINFO(m_nSceneStringIndex),MAX_CHOREO_SCENES_STRING_BITS,SPROP_UNSIGNED),
	SendPropBool(SENDINFO(m_bIsPlayingBack)),
	SendPropBool(SENDINFO(m_bPaused)),
	SendPropBool(SENDINFO(m_bMultiplayer)),
	SendPropFloat(SENDINFO(m_flForceClientTime)),
	SendPropUtlVector(
		SENDINFO_UTLVECTOR( m_hActorList ),
		MAX_ACTORS_IN_SCENE, // max elements
		SendPropEHandle( NULL, 0 ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CSceneEntity )

	// Keys
	DEFINE_KEYFIELD( m_iszSceneFile, FIELD_STRING, "SceneFile" ),
	DEFINE_KEYFIELD( m_iszResumeSceneFile, FIELD_STRING, "ResumeSceneFile" ),
	DEFINE_FIELD( m_hWaitingForThisResumeScene, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bWaitingForResumeScene, FIELD_BOOLEAN ),

	DEFINE_KEYFIELD( m_iszTarget1, FIELD_STRING, "target1" ),
	DEFINE_KEYFIELD( m_iszTarget2, FIELD_STRING, "target2" ),
	DEFINE_KEYFIELD( m_iszTarget3, FIELD_STRING, "target3" ),
	DEFINE_KEYFIELD( m_iszTarget4, FIELD_STRING, "target4" ),
	DEFINE_KEYFIELD( m_iszTarget5, FIELD_STRING, "target5" ),
	DEFINE_KEYFIELD( m_iszTarget6, FIELD_STRING, "target6" ),
	DEFINE_KEYFIELD( m_iszTarget7, FIELD_STRING, "target7" ),
	DEFINE_KEYFIELD( m_iszTarget8, FIELD_STRING, "target8" ),

	DEFINE_KEYFIELD( m_BusyActor, FIELD_INTEGER, "busyactor" ),

	DEFINE_FIELD( m_hTarget1, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hTarget2, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hTarget3, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hTarget4, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hTarget5, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hTarget6, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hTarget7, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hTarget8, FIELD_EHANDLE ),

	DEFINE_FIELD( m_bIsPlayingBack, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bPaused, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flCurrentTime, FIELD_FLOAT ),  // relative, not absolute time
	DEFINE_FIELD( m_flForceClientTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_flFrameTime, FIELD_FLOAT ),  // last frametime
	DEFINE_FIELD( m_bCancelAtNextInterrupt, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_fPitch, FIELD_FLOAT ),
	DEFINE_FIELD( m_bAutomated, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nAutomatedAction, FIELD_INTEGER ),
	DEFINE_FIELD( m_flAutomationDelay, FIELD_FLOAT ),
	DEFINE_FIELD( m_flAutomationTime, FIELD_FLOAT ),  // relative, not absolute time

	DEFINE_FIELD( m_bPausedViaInput, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bWaitingForActor, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bWaitingForInterrupt, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bInterruptedActorsScenes, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bBreakOnNonIdle, FIELD_BOOLEAN ),

	DEFINE_UTLVECTOR( m_hActorList, FIELD_EHANDLE ),
	DEFINE_UTLVECTOR( m_hRemoveActorList, FIELD_EHANDLE ),
	
	// DEFINE_FIELD( m_pScene, FIELD_XXXX ) // Special processing used for this

	// These are set up in the constructor
	// DEFINE_FIELD( m_pcvSndMixahead, FIELD_XXXXX ),
	// DEFINE_FIELD( m_bRestoring, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_nInterruptCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_bInterrupted, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hInterruptScene, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bCompletedEarly, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bInterruptSceneFinished, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_bGenerated, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iszSoundName, FIELD_STRING ),
	DEFINE_FIELD( m_hActor, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hActivator, FIELD_EHANDLE ),

	// DEFINE_FIELD( m_bSceneMissing, FIELD_BOOLEAN ),
	DEFINE_UTLVECTOR( m_hNotifySceneCompletion, FIELD_EHANDLE ),
	DEFINE_UTLVECTOR( m_hListManagers, FIELD_EHANDLE ),

	DEFINE_FIELD( m_bMultiplayer, FIELD_BOOLEAN ),
//	DEFINE_FIELD( m_nSceneStringIndex, FIELD_INTEGER ),

	// DEFINE_FIELD( m_pRecipientFilter, IRecipientFilter* ),	// Multiplayer only

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Start", InputStartPlayback ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Pause", InputPausePlayback ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Resume", InputResumePlayback ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Cancel", InputCancelPlayback ),
	DEFINE_INPUTFUNC( FIELD_VOID, "CancelAtNextInterrupt", InputCancelAtNextInterrupt ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "PitchShift", InputPitchShiftPlayback ),
	DEFINE_INPUTFUNC( FIELD_STRING, "InterjectResponse", 	InputInterjectResponse ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StopWaitingForActor", 	InputStopWaitingForActor ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "Trigger", InputTriggerEvent ),

	DEFINE_KEYFIELD( m_iPlayerDeathBehavior, FIELD_INTEGER, "onplayerdeath" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ScriptPlayerDeath", InputScriptPlayerDeath ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetTarget1", InputSetTarget1 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTarget2", InputSetTarget2 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTarget3", InputSetTarget3 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTarget4", InputSetTarget4 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTarget5", InputSetTarget5 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTarget6", InputSetTarget6 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTarget7", InputSetTarget7 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTarget8", InputSetTarget8 ),

	// Outputs
	DEFINE_OUTPUT( m_OnStart, "OnStart"),
	DEFINE_OUTPUT( m_OnCompletion, "OnCompletion"),
	DEFINE_OUTPUT( m_OnCanceled, "OnCanceled"),
	DEFINE_OUTPUT( m_OnTrigger1, "OnTrigger1"),
	DEFINE_OUTPUT( m_OnTrigger2, "OnTrigger2"),
	DEFINE_OUTPUT( m_OnTrigger3, "OnTrigger3"),
	DEFINE_OUTPUT( m_OnTrigger4, "OnTrigger4"),
	DEFINE_OUTPUT( m_OnTrigger5, "OnTrigger5"),
	DEFINE_OUTPUT( m_OnTrigger6, "OnTrigger6"),
	DEFINE_OUTPUT( m_OnTrigger7, "OnTrigger7"),
	DEFINE_OUTPUT( m_OnTrigger8, "OnTrigger8"),
	DEFINE_OUTPUT( m_OnTrigger9, "OnTrigger9"),
	DEFINE_OUTPUT( m_OnTrigger10, "OnTrigger10"),
	DEFINE_OUTPUT( m_OnTrigger11, "OnTrigger11"),
	DEFINE_OUTPUT( m_OnTrigger12, "OnTrigger12"),
	DEFINE_OUTPUT( m_OnTrigger13, "OnTrigger13"),
	DEFINE_OUTPUT( m_OnTrigger14, "OnTrigger14"),
	DEFINE_OUTPUT( m_OnTrigger15, "OnTrigger15"),
	DEFINE_OUTPUT( m_OnTrigger16, "OnTrigger16"),
END_DATADESC()

BEGIN_ENT_SCRIPTDESC( CSceneEntity, CBaseEntity, "Choreographed scene which controls animation and/or dialog on one or more actors." )
	DEFINE_SCRIPTFUNC( EstimateLength, "Returns length of this scene in seconds." )
	DEFINE_SCRIPTFUNC( IsPlayingBack, "If this scene is currently playing." )
	DEFINE_SCRIPTFUNC( IsPaused, "If this scene is currently paused." )
	DEFINE_SCRIPTFUNC( AddBroadcastTeamTarget, "Adds a team (by index) to the broadcast list" )
	DEFINE_SCRIPTFUNC( RemoveBroadcastTeamTarget, "Removes a team (by index) from the broadcast list" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptFindNamedEntity, "FindNamedEntity", "given an entity reference, such as !target, get actual entity from scene object" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptLoadSceneFromString, "LoadSceneFromString", "given a dummy scene name and a vcd string, load the scene" )
END_SCRIPTDESC();

const ConVar	*CSceneEntity::m_pcvSndMixahead = NULL;


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSceneEntity::CSceneEntity( void )
{
	m_bWaitingForActor	= false;
	m_bWaitingForInterrupt = false;
	m_bInterruptedActorsScenes = false;
	m_bIsPlayingBack	= false;
	m_bPaused			= false;
	m_bMultiplayer = false;
	m_fPitch = 1.0f;
	m_iszSceneFile		= NULL_STRING;
	m_iszResumeSceneFile = NULL_STRING;
	m_hWaitingForThisResumeScene = NULL;
	m_bWaitingForResumeScene = false;
	SetCurrentTime( 0.0f, false );
	m_bCancelAtNextInterrupt = false;

	m_bAutomated		= false;
	m_nAutomatedAction	= SCENE_ACTION_UNKNOWN;
	m_flAutomationDelay = 0.0f;
	m_flAutomationTime = 0.0f;

	m_bPausedViaInput	= false;
	ClearInterrupt();

	m_pScene			= NULL;

	m_bCompletedEarly	= false;

	if ( !m_pcvSndMixahead )
		m_pcvSndMixahead	= cvar->FindVar( "snd_mixahead" );

	m_BusyActor			= SCENE_BUSYACTOR_DEFAULT;

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSceneEntity::~CSceneEntity( void )
{
	delete m_pRecipientFilter;
	m_pRecipientFilter = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Resets time such that the client version of the .vcd is also updated, if appropriate
// Input  : t - 
//			forceClientSync - forces new timestamp down to client .dll via networking
//-----------------------------------------------------------------------------
void CSceneEntity::SetCurrentTime( float t, bool bForceClientSync )
{
	m_flCurrentTime = t;
	if ( gpGlobals->maxClients == 1 || bForceClientSync )
	{
		m_flForceClientTime = t;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::UpdateOnRemove( void )
{
	UnloadScene();
	BaseClass::UpdateOnRemove();

	if ( GetSceneManager() )
	{
		GetSceneManager()->RemoveSceneEntity( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*soundname - 
// Output : CChoreoScene
//-----------------------------------------------------------------------------
CChoreoScene *CSceneEntity::GenerateSceneForSound( CBaseFlex *pFlexActor, const char *soundname )
{
	float duration = CBaseEntity::GetSoundDuration( soundname, pFlexActor ? STRING( pFlexActor->GetModelName() ) : NULL );
	if( duration <= 0.0f )
	{
		Warning( "CSceneEntity::GenerateSceneForSound:  Couldn't determine duration of %s\n", soundname );
		return NULL;
	}

	CChoreoScene *scene = new CChoreoScene( this );
	if ( !scene )
	{
		Warning( "CSceneEntity::GenerateSceneForSound:  Failed to allocated new scene!!!\n" );
	}
	else
	{
		scene->SetPrintFunc( LocalScene_Printf );


		CChoreoActor *actor = scene->AllocActor();
		CChoreoChannel *channel = scene->AllocChannel();
		CChoreoEvent *event = scene->AllocEvent();

		Assert( actor );
		Assert( channel );
		Assert( event );

		if ( !actor || !channel || !event )
		{
			Warning( "CSceneEntity::GenerateSceneForSound:  Alloc of actor, channel, or event failed!!!\n" );
			delete scene;
			return NULL;
		}

		// Set us up the actorz
		actor->SetName( "!self" );  // Could be pFlexActor->GetName()?
		actor->SetActive( true );

		// Set us up the channelz
		channel->SetName( STRING( m_iszSceneFile ) );
		channel->SetActor( actor );

		// Add to actor
		actor->AddChannel( channel );
	
		// Set us up the eventz
		event->SetType( CChoreoEvent::SPEAK );
		event->SetName( soundname );
		event->SetParameters( soundname );
		event->SetStartTime( 0.0f );
		event->SetUsingRelativeTag( false );
		event->SetEndTime( duration );
		event->SnapTimes();

		// Add to channel
		channel->AddEvent( event );

		// Point back to our owners
		event->SetChannel( channel );
		event->SetActor( actor );

	}

	return scene;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::Activate()
{
	if ( m_bGenerated && !m_pScene )
	{
		m_pScene = GenerateSceneForSound( m_hActor, STRING( m_iszSoundName ) );
	}

	BaseClass::Activate();

	if ( GetSceneManager() )
	{
		GetSceneManager()->AddSceneEntity( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CSceneEntity::GetSoundSystemLatency( void )
{
	if ( m_pcvSndMixahead )
	{
		return m_pcvSndMixahead->GetFloat();
	}
	
	// Assume 100 msec sound system latency
	return SOUND_SYSTEM_LATENCY_DEFAULT;
}
		
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//-----------------------------------------------------------------------------
void CSceneEntity::PrecacheScene( CChoreoScene *scene )
{
	Assert( scene );

	// Iterate events and precache necessary resources
	for ( int i = 0; i < scene->GetNumEvents(); i++ )
	{
		CChoreoEvent *event = scene->GetEvent( i );
		if ( !event )
			continue;

		// load any necessary data
		switch (event->GetType() )
		{
		default:
			break;
		case CChoreoEvent::SPEAK:
			{
				// Defined in SoundEmitterSystem.cpp
				// NOTE:  The script entries associated with .vcds are forced to preload to avoid
				//  loading hitches during triggering
				PrecacheScriptSound( event->GetParameters() );

				if ( event->GetCloseCaptionType() == CChoreoEvent::CC_MASTER && 
					 event->GetNumSlaves() > 0 )
				{
					char tok[ CChoreoEvent::MAX_CCTOKEN_STRING ];
					if ( event->GetPlaybackCloseCaptionToken( tok, sizeof( tok ) ) )
					{
						PrecacheScriptSound( tok );
					}
				}
			}
			break;
		case CChoreoEvent::SUBSCENE:
			{
				// Only allow a single level of subscenes for now
				if ( !scene->IsSubScene() )
				{
					CChoreoScene *subscene = event->GetSubScene();
					if ( !subscene )
					{
						subscene = LoadScene( event->GetParameters(), this );
						subscene->SetSubScene( true );
						event->SetSubScene( subscene );

						// Now precache it's resources, if any
						PrecacheScene( subscene );
					}
				}
			}
			break;
		}
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::Precache( void )
{
	if ( m_bGenerated )
		return;

	if ( m_iszSceneFile == NULL_STRING )
		return;

	if ( m_iszResumeSceneFile != NULL_STRING )
	{
		PrecacheInstancedScene( STRING( m_iszResumeSceneFile ) );
	}

	PrecacheInstancedScene( STRING( m_iszSceneFile ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActor - 
//			*soundname - 
//-----------------------------------------------------------------------------
void CSceneEntity::GenerateSoundScene( CBaseFlex *pActor, const char *soundname )
{
	m_bGenerated	= true;
	m_iszSoundName	= MAKE_STRING( soundname );
	m_hActor		= pActor;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSceneEntity::HasUnplayedSpeech( void )
{
	if ( m_pScene )
		return m_pScene->HasUnplayedSpeech();

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSceneEntity::HasFlexAnimation( void )
{
	if ( m_pScene )
		return m_pScene->HasFlexAnimation();

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------

void CSceneEntity::SetBackground( bool bIsBackground )
{
	if ( m_pScene )
	{
		m_pScene->SetBackground( bIsBackground );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------

bool CSceneEntity::IsBackground( void )
{
	if ( m_pScene )
		return m_pScene->IsBackground( );

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::OnRestore()
{
	BaseClass::OnRestore();

	// Fix saved games that have their pitch set to zero
	if ( m_fPitch < SCENE_MIN_PITCH || m_fPitch > SCENE_MAX_PITCH )
		m_fPitch = 1.0f;

	if ( !m_bIsPlayingBack )
		return;

	if ( !m_pScene )
	{
		m_pScene = LoadScene( STRING( m_iszSceneFile ), this );
		if ( !m_pScene )
		{
			m_bSceneMissing = true;
			return;
		}

		OnLoaded();
		
		if ( ShouldNetwork() )
		{
			m_nSceneStringIndex = g_pStringTableClientSideChoreoScenes->AddString( CBaseEntity::IsServer(), STRING( m_iszSceneFile ) );
		}

		UpdateTransmitState();
	}

	m_bSceneMissing = false;

	int i;
	for ( i = 0 ; i < m_pScene->GetNumActors(); i++ )
	{
		CBaseFlex *pTestActor = FindNamedActor( i );
		if ( !pTestActor )
			continue;

		if ( !pTestActor->MyCombatCharacterPointer() )
			continue;

		// Needed?
		//if ( !pTestActor->MyCombatCharacterPointer()->IsAlive() )
		//	return;

		pTestActor->StartChoreoScene( m_pScene );
	}

	float dt = SCENE_THINK_INTERVAL;

	bool paused = m_bPaused;

	m_bPaused = false;

	// roll back slightly so that pause events still trigger
	m_pScene->ResetSimulation( true, m_flCurrentTime - SCENE_THINK_INTERVAL, m_flCurrentTime );
	m_pScene->SetTime( m_flCurrentTime - SCENE_THINK_INTERVAL );

	SetCurrentTime( m_flCurrentTime, true );

	// Robin: This causes a miscount of any interrupt events in the scene.
	// All the variables are saved/restored properly, so we can safely leave them alone.
	//ClearInterrupt();

	SetRestoring( true );

	DoThink( dt );

	SetRestoring( false );

	if ( paused )
	{
		PausePlayback();
	}

	NetworkProp()->NetworkStateForceUpdate();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CSceneEntity::SetRestoring( bool bRestoring )
{
	m_bRestoring = bRestoring;
	if ( m_pScene )
	{
		m_pScene->SetRestoring( bRestoring );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::Spawn( void )
{
	Precache();
}

void CSceneEntity::PauseThink( void )
{
	if ( !m_pScene )
		return;

	// Stay paused if pause occurred from interrupt
	if ( m_bInterrupted )
		return;

	// If entity I/O paused the scene, then it'll have to resume/cancel the scene...
	if ( m_bPausedViaInput )
	{
		// If we're waiting for a resume scene to finish, continue when it's done
		if ( m_bWaitingForResumeScene && !m_hWaitingForThisResumeScene )
		{
			// Resume scene has finished, stop waiting for it
			m_bWaitingForResumeScene = false;
		}
		else
		{
			return;
		}
	}

	if ( !m_bAutomated )
	{
		// FIXME:  Game code should check for AI waiting conditions being met, etc.
		//
		//
		//
		bool bAllFinished = m_pScene->CheckEventCompletion( );

		if ( bAllFinished )
		{
			// Perform action
			switch ( m_nAutomatedAction )
			{
			case SCENE_ACTION_RESUME:
				ResumePlayback();
				break;
			case SCENE_ACTION_CANCEL:
				LocalScene_Printf( "%s : PauseThink canceling playback\n", STRING( m_iszSceneFile ) );
				CancelPlayback();
				break;
			default:
				ResumePlayback();
				break;
			}

			// Reset
			m_bAutomated = false;
			m_nAutomatedAction = SCENE_ACTION_UNKNOWN;
			m_flAutomationTime = 0.0f;
			m_flAutomationDelay = 0.0f;
			m_bPausedViaInput = false;
		}
		return;
	}

	// Otherwise, see if resume/cancel is automatic and act accordingly if enough time
	//  has passed
	m_flAutomationTime += (gpGlobals->frametime);

	if ( m_flAutomationDelay > 0.0f &&
		m_flAutomationTime < m_flAutomationDelay )
		return;

	// Perform action
	switch ( m_nAutomatedAction )
	{
	case SCENE_ACTION_RESUME:
		LocalScene_Printf( "%s : Automatically resuming playback\n", STRING( m_iszSceneFile ) );
		ResumePlayback();
		break;
	case SCENE_ACTION_CANCEL:
		LocalScene_Printf( "%s : Automatically canceling playback\n", STRING( m_iszSceneFile ) );
		CancelPlayback();
		break;
	default:
		LocalScene_Printf( "%s : Unknown action %i, automatically resuming playback\n", STRING( m_iszSceneFile ), m_nAutomatedAction );
		ResumePlayback();
		break;
	}

	// Reset
	m_bAutomated = false;
	m_nAutomatedAction = SCENE_ACTION_UNKNOWN;
	m_flAutomationTime = 0.0f;
	m_flAutomationDelay = 0.0f;
	m_bPausedViaInput = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchPauseScene( CChoreoScene *scene, const char *parameters )
{
	// Don't pause during restore, since we'll be restoring the pause state already
	if ( m_bRestoring )
		return;

	// FIXME:  Hook this up to AI, etc. somehow, perhaps poll each actor for conditions using
	//  scene resume condition iterator
	PausePlayback();

	char token[1024];

	m_bPausedViaInput = false;
	m_bAutomated		= false;
	m_nAutomatedAction	= SCENE_ACTION_UNKNOWN;
	m_flAutomationDelay = 0.0f;
	m_flAutomationTime = 0.0f;

	// Check for auto resume/cancel
	const char *buffer = parameters;
	buffer = engine->ParseFile( buffer, token, sizeof( token ) );
	if ( !stricmp( token, "automate" ) )
	{
		buffer = engine->ParseFile( buffer, token, sizeof( token ) );
		if ( !stricmp( token, "Cancel" ) )
		{
			m_nAutomatedAction = SCENE_ACTION_CANCEL;
		}
		else if ( !stricmp( token, "Resume" ) )
		{
			m_nAutomatedAction = SCENE_ACTION_RESUME;
		}

		if ( m_nAutomatedAction != SCENE_ACTION_UNKNOWN )
		{
			buffer = engine->ParseFile( buffer, token, sizeof( token ) );
			m_flAutomationDelay = (float)atof( token );

			if ( m_flAutomationDelay > 0.0f )
			{
				// Success
				m_bAutomated = true;
				m_flAutomationTime = 0.0f;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchProcessLoop( CChoreoScene *scene, CChoreoEvent *event )
{
	// Don't restore this event since it's implied in the current "state" of the scene timer, etc.
	if ( m_bRestoring )
		return;

	Assert( scene );
	Assert( event->GetType() == CChoreoEvent::LOOP );

	float backtime = (float)atof( event->GetParameters() );

	bool process = true;
	int counter = event->GetLoopCount();
	if ( counter != -1 )
	{
		int remaining = event->GetNumLoopsRemaining();
		if ( remaining <= 0 )
		{
			process = false;
		}
		else
		{
			event->SetNumLoopsRemaining( --remaining );
		}
	}

	if ( !process )
		return;

	scene->LoopToTime( backtime );
	SetCurrentTime( backtime, true );
}

//-----------------------------------------------------------------------------
// Purpose: Flag the scene as already "completed"
// Input  : *scene - 
//			*parameters - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStopPoint( CChoreoScene *scene, const char *parameters )
{
	if ( m_bCompletedEarly )
	{
		Assert( 0 );
		Warning( "Scene '%s' with two stop point events!\n", STRING( m_iszSceneFile ) );
		return;
	}
	// Fire completion trigger early
	m_bCompletedEarly = true;
	m_OnCompletion.FireOutput( this, this, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSceneEntity::IsInterruptable()
{
	return ( m_nInterruptCount > 0 ) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//			*actor - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartInterrupt( CChoreoScene *scene, CChoreoEvent *event )
{
	// Don't re-interrupt during restore
	if ( m_bRestoring )
		return;

	// If we're supposed to cancel at our next interrupt point, cancel now
	if ( m_bCancelAtNextInterrupt )
	{
		m_bCancelAtNextInterrupt = false;
		LocalScene_Printf( "%s : cancelled via interrupt\n", STRING( m_iszSceneFile ) );
		CancelPlayback();
		return;
	}

	++m_nInterruptCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//			*actor - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchEndInterrupt( CChoreoScene *scene, CChoreoEvent *event )
{
	// Don't re-interrupt during restore
	if ( m_bRestoring )
		return;

	--m_nInterruptCount;

	if ( m_nInterruptCount < 0 )
	{
		m_nInterruptCount = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartExpression( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->AddSceneEvent( scene, event );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchEndExpression( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->RemoveSceneEvent( scene, event, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartFlexAnimation( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->AddSceneEvent( scene, event );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchEndFlexAnimation( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->RemoveSceneEvent( scene, event, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*parameters - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartGesture( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	// Ingore null gestures
	if ( !Q_stricmp( event->GetName(), "NULL" ) )
		return;

	actor->AddSceneEvent( scene, event); 
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*parameters - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchEndGesture( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	// Ingore null gestures
	if ( !Q_stricmp( event->GetName(), "NULL" ) )
		return;

	actor->RemoveSceneEvent( scene, event, m_bRestoring );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*parameters - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartGeneric( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	CBaseEntity *pTarget = FindNamedEntity( event->GetParameters2( ) );
	actor->AddSceneEvent( scene, event, pTarget );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*parameters - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchEndGeneric( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->RemoveSceneEvent( scene, event, m_bRestoring );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*actor2 - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartLookAt( CChoreoScene *scene, CBaseFlex *actor, CBaseEntity *actor2, CChoreoEvent *event )
{
	actor->AddSceneEvent( scene, event, actor2 );
}


void CSceneEntity::DispatchEndLookAt( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->RemoveSceneEvent( scene, event, m_bRestoring );
}


//-----------------------------------------------------------------------------
// Purpose: Move to spot/actor
// FIXME:  Need to allow this to take arbitrary amount of time and pause playback
//  while waiting for actor to move into position
// Input  : *actor - 
//			*parameters - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartMoveTo( CChoreoScene *scene, CBaseFlex *actor, CBaseEntity *actor2, CChoreoEvent *event )
{
	actor->AddSceneEvent( scene, event, actor2 );
}


void CSceneEntity::DispatchEndMoveTo( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->RemoveSceneEvent( scene, event, m_bRestoring );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *token - 
//			listener - 
//			soundorigins - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool AttenuateCaption( const char *token, const Vector& listener, CUtlVector< Vector >& soundorigins )
{
	if ( scene_maxcaptionradius.GetFloat() <= 0.0f )
	{
		return false;
	}

	int c = soundorigins.Count();

	if ( c <= 0 )
	{
		return false;
	}

	float maxdistSqr = scene_maxcaptionradius.GetFloat() * scene_maxcaptionradius.GetFloat();

	for ( int i = 0; i  < c; ++i )
	{
		const Vector& org = soundorigins[ i ];

		float distSqr = ( org - listener ).LengthSqr();
		if ( distSqr <= maxdistSqr )
		{
			return false;
		}
	}

	// All sound sources too far, don't show caption...
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *event - which event 
//			player - which recipient 
//			buf, buflen:  where to put the data 
// Output : Returns true if the sound should be played/prefetched
//-----------------------------------------------------------------------------
bool CSceneEntity::GetSoundNameForPlayer( CChoreoEvent *event, CBasePlayer *player, char *buf, size_t buflen, CBaseEntity *pActor )
{
	Assert( event );
	Assert( player );
	Assert( buf );
	Assert( buflen > 0 );

	bool ismasterevent = true;
	char tok[ CChoreoEvent::MAX_CCTOKEN_STRING ];
	bool validtoken = false;

	tok[ 0 ] = 0;

	if ( event->GetCloseCaptionType() == CChoreoEvent::CC_SLAVE ||
		event->GetCloseCaptionType() == CChoreoEvent::CC_DISABLED )
	{
		ismasterevent = false;
	}
	else
	{
		validtoken = event->GetPlaybackCloseCaptionToken( tok, sizeof( tok ) );
	}

	const char* pchToken = "";

	if ( pActor && pActor->IsPlayer() )
	{
		pchToken = dynamic_cast< CBasePlayer* >( pActor )->GetSceneSoundToken();
	}

	// Copy the sound name
	CopySoundNameWithModifierToken( buf, event->GetParameters(), buflen, pchToken );

	// If there was a modifier token, don't change the sound based on CC
	if ( pchToken[0] != 0 )
		return true;

	bool usingEnglish = true;
	if ( !IsXbox() )
	{
		char const *cvarvalue = engine->GetClientConVarValue( player->entindex(), "english" );
		if ( cvarvalue && *cvarvalue && Q_atoi( cvarvalue ) != 1 )
		{
			usingEnglish = false;
		}

	}

	// This makes it like they are running in another language
	if ( scene_forcecombined.GetBool() )
	{
		usingEnglish = false;
	}

	if ( usingEnglish )
	{
		// English sounds always play
		return true;
	}
	
	if ( ismasterevent )
	{
		// Master event sounds always play too (master will be the combined .wav)
		if ( validtoken )
		{
			Q_strncpy( buf, tok, buflen );
		}
		return true;
	}

	// Slave events don't play any sound...
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Playback sound file that contains phonemes
// Input  : *actor - 
//			*parameters - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartSpeak( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event, soundlevel_t iSoundlevel )
{
	// Emit sound
	if ( actor )
	{
		CPASAttenuationFilter filter( actor );

		if ( m_pRecipientFilter )
		{
			int filterCount = filter.GetRecipientCount();
			int recipientPlayerCount = m_pRecipientFilter->GetRecipientCount();
			for ( int i = filterCount-1; i >= 0; --i )
			{
				int playerindex = filter.GetRecipientIndex( i );

				bool bFound = false;

				for ( int j = 0; j < recipientPlayerCount; ++j )
				{
					if ( m_pRecipientFilter->GetRecipientIndex(j) == playerindex )
					{
						bFound = true;
						break;
					}
				}

				if ( !bFound )
				{
					filter.RemoveRecipientByPlayerIndex( playerindex );
				}
			}			
		}

		float time_in_past = m_flCurrentTime - event->GetStartTime() ;

		float soundtime = gpGlobals->curtime - time_in_past;

		if ( m_bRestoring )
		{
			// Need to queue sounds on restore because the player has not yet connected
			GetSceneManager()->QueueRestoredSound( actor, event->GetParameters(), iSoundlevel, time_in_past );

			return;
		}

		// Add padding to prevent any other talker talking right after I'm done, because I might 
		// be continuing speaking with another scene.
		float flDuration = event->GetDuration() - time_in_past;

		CAI_BaseActor *pBaseActor = dynamic_cast<CAI_BaseActor*>(actor);
		if ( pBaseActor )
		{
			pBaseActor->NoteSpeaking( flDuration, GetPostSpeakDelay() );
		}
		else if ( actor->IsNPC() )
		{
			GetSpeechSemaphore( actor->MyNPCPointer() )->Acquire( flDuration + GetPostSpeakDelay(), actor );
		}

		EmitSound_t es;
		es.m_nChannel = CHAN_VOICE;
		es.m_flVolume = 1;
		es.m_SoundLevel = iSoundlevel;
		// Only specify exact delay in single player
		es.m_flSoundTime = ( gpGlobals->maxClients == 1 ) ? soundtime : 0.0f;
		if ( scene->ShouldIgnorePhonemes() )
		{
			es.m_nFlags |= SND_IGNORE_PHONEMES;
		}

		if ( actor->GetSpecialDSP() != 0 )
		{
			es.m_nSpecialDSP = actor->GetSpecialDSP();
		}

		// No CC since we do it manually
		// FIXME:  This will  change
		es.m_bEmitCloseCaption = false;

		int c = filter.GetRecipientCount();
		for ( int i = 0; i < c; ++i )
		{
			int playerindex = filter.GetRecipientIndex( i );
			CBasePlayer *player = UTIL_PlayerByIndex( playerindex );
			if ( !player )
				continue;

			CSingleUserRecipientFilter filter2( player );

			char soundname[ 512 ];
			if ( !GetSoundNameForPlayer( event, player, soundname, sizeof( soundname ), actor ) )
			{
				continue;
			}

			es.m_pSoundName = soundname;

			// keep track of the last few sounds played for bug reports
			speechListSounds[ speechListIndex ].time = gpGlobals->curtime;
			Q_strncpy( speechListSounds[ speechListIndex ].name, soundname, sizeof( speechListSounds[ 0 ].name ) );
			Q_strncpy( speechListSounds[ speechListIndex ].sceneName, ( scene ) ? scene->GetFilename() : "", sizeof( speechListSounds[ 0 ].sceneName ) );
			
			speechListIndex++;
			if ( speechListIndex >= SPEECH_LIST_MAX_SOUNDS )
			{
				speechListIndex = 0;
			}

			// Warning( "Speak %s\n", soundname );

			if ( m_fPitch != 1.0f )
			{
				if ( es.m_nPitch )
					es.m_nPitch = static_cast<float>( es.m_nPitch ) * m_fPitch;
				else
					es.m_nPitch = 100.0f * m_fPitch;

				es.m_nFlags |= SND_CHANGE_PITCH;
			}

			EmitSound( filter2, actor->entindex(), es );
			actor->AddSceneEvent( scene, event );
		}
	
		// Close captioning only on master token no matter what...
		if ( event->GetCloseCaptionType() == CChoreoEvent::CC_MASTER )
		{
			char tok[ CChoreoEvent::MAX_CCTOKEN_STRING ];
			bool validtoken = event->GetPlaybackCloseCaptionToken( tok, sizeof( tok ) );
			if ( validtoken )
			{
				char lowercase[ 256 ];
				Q_strncpy( lowercase, tok, sizeof( lowercase ) );
				Q_strlower( lowercase );

				// Remove any players who don't want close captions
				CBaseEntity::RemoveRecipientsIfNotCloseCaptioning( filter );

				// Certain events are marked "don't attenuate", (breencast), skip those here
				if ( !event->IsSuppressingCaptionAttenuation() && 
					( filter.GetRecipientCount() > 0 ) )
				{
					int c = filter.GetRecipientCount();
					for ( int i = c - 1 ; i >= 0; --i )
					{
						CBasePlayer *player = UTIL_PlayerByIndex( filter.GetRecipientIndex( i ) );
						if ( !player )
							continue;

						Vector playerOrigin = player->GetAbsOrigin();

						if ( AttenuateCaption( lowercase, playerOrigin, es.m_UtlVecSoundOrigin ) )
						{
							// If the player has a view entity, measure the distance to that
							if ( !player->GetViewEntity() || AttenuateCaption( lowercase, player->GetViewEntity()->GetAbsOrigin(), es.m_UtlVecSoundOrigin ) )
							{
								filter.RemoveRecipient( player );
							}
						}
					}
				}

				// Anyone left?
				if ( filter.GetRecipientCount() > 0 )
				{
					float endtime = event->GetLastSlaveEndTime();
					float durationShort = event->GetDuration();
					float durationLong = endtime - event->GetStartTime();

					float duration = MAX( durationShort, durationLong );


					byte byteflags = CLOSE_CAPTION_WARNIFMISSING; // warnifmissing
					/*
					// Never for .vcds...
					if ( fromplayer )
					{
						byteflags |= CLOSE_CAPTION_FROMPLAYER;
					}
					*/
					char const *pszActorModel = STRING( actor->GetModelName() );
					gender_t gender = soundemitterbase->GetActorGender( pszActorModel );

					if ( gender == GENDER_MALE )
					{
						byteflags |= CLOSE_CAPTION_GENDER_MALE;
					}
					else if ( gender == GENDER_FEMALE )
					{ 
						byteflags |= CLOSE_CAPTION_GENDER_FEMALE;
					}

					// Send caption and duration hint down to client
					UserMessageBegin( filter, "CloseCaption" );
						WRITE_STRING( lowercase );
						WRITE_SHORT( MIN( 255, (int)( duration * 10.0f ) ) );
						WRITE_BYTE( byteflags ); // warn on missing
					MessageEnd();
				}
			}
		}
	}
}

void CSceneEntity::DispatchEndSpeak( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->RemoveSceneEvent( scene, event, m_bRestoring );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*actor2 - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartFace( CChoreoScene *scene, CBaseFlex *actor, CBaseEntity *actor2, CChoreoEvent *event )
{
	actor->AddSceneEvent( scene, event, actor2 );
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*actor2 - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchEndFace( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->RemoveSceneEvent( scene, event, m_bRestoring );
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartSequence( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->AddSceneEvent( scene, event );
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchEndSequence( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->RemoveSceneEvent( scene, event, m_bRestoring );
}

//-----------------------------------------------------------------------------
// Purpose: NPC can play interstitial vcds (such as responding to the player doing something during a scene)
// Input  : *scene - 
//			*actor - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartPermitResponses( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->SetPermitResponse( gpGlobals->curtime + event->GetDuration() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//			*actor - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchEndPermitResponses( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->SetPermitResponse( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CSceneEntity::EstimateLength( void )
{
	if ( !m_pScene )
	{
		return GetSceneDuration( STRING( m_iszSceneFile ) );
	}
	return m_pScene->FindStopTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
// NOTE: returns false if scene hasn't loaded yet
//-----------------------------------------------------------------------------
void CSceneEntity::CancelIfSceneInvolvesActor( CBaseEntity *pActor )
{
	if ( InvolvesActor( pActor ) )
	{
		LocalScene_Printf( "%s : cancelled for '%s'\n", STRING( m_iszSceneFile ), pActor->GetDebugName() );
		CancelPlayback();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// NOTE: returns false if scene hasn't loaded yet
//-----------------------------------------------------------------------------
bool CSceneEntity::InvolvesActor( CBaseEntity *pActor )
{
 	if ( !m_pScene )
		return false;	

	int i;
	for ( i = 0 ; i < m_pScene->GetNumActors(); i++ )
	{
		CBaseFlex *pTestActor = FindNamedActor( i );
		if ( !pTestActor )
			continue;

		if ( pTestActor == pActor )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::DoThink( float frametime )
{
	CheckInterruptCompletion();

	if ( m_bWaitingForActor || m_bWaitingForInterrupt )
	{
		// Try to start playback.
		StartPlayback();
	}

	if ( !m_pScene )
		return;

	if ( !m_bIsPlayingBack )
		return;

	// catch bad pitch shifting from old save games
	Assert( m_fPitch >= SCENE_MIN_PITCH && m_fPitch <= SCENE_MAX_PITCH );
	m_fPitch = clamp( m_fPitch, SCENE_MIN_PITCH, SCENE_MAX_PITCH );

	if ( m_bPaused )
	{
		PauseThink();
		return;
	}

	// Msg("%.2f %s\n", gpGlobals->curtime, STRING( m_iszSceneFile ) );

	//Msg( "SV:  %d, %f for %s\n", gpGlobals->tickcount, m_flCurrentTime, m_pScene->GetFilename() );

	m_flFrameTime = frametime;

	m_pScene->SetSoundFileStartupLatency( GetSoundSystemLatency() );

	// Tell scene to go
	m_pScene->Think( m_flCurrentTime );

	// Did we get to the end
	if ( !m_bPaused )
	{
		// Drive simulation time for scene
		SetCurrentTime( m_flCurrentTime + m_flFrameTime * m_fPitch, false );

		if ( m_pScene->SimulationFinished() )
		{
			OnSceneFinished( false, true );
	
			// Stop them from doing anything special
			ClearSchedules( m_pScene );
		}
	}
	else 
	{
		// Drive simulation time for scene
		SetCurrentTime( m_pScene->GetTime(), true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input handlers
//-----------------------------------------------------------------------------
void CSceneEntity::InputStartPlayback( inputdata_t &inputdata )
{
	// Already playing, ignore
	if ( m_bIsPlayingBack )
		return;

	// Already waiting on someone.
	if ( m_bWaitingForActor || m_bWaitingForInterrupt )
		return;

	ClearActivatorTargets();
	m_hActivator = inputdata.pActivator;
	StartPlayback();
}

void CSceneEntity::InputPausePlayback( inputdata_t &inputdata )
{
	PausePlayback();
	m_bPausedViaInput = true;
}

void CSceneEntity::InputResumePlayback( inputdata_t &inputdata )
{
	ResumePlayback();
}

void CSceneEntity::InputCancelPlayback( inputdata_t &inputdata )
{
	LocalScene_Printf( "%s : cancelled via input\n", STRING( m_iszSceneFile ) );
	CancelPlayback();
}

void CSceneEntity::InputScriptPlayerDeath( inputdata_t &inputdata )
{
	if ( m_iPlayerDeathBehavior == SCRIPT_CANCEL )
	{
		LocalScene_Printf( "%s : cancelled via player death\n", STRING( m_iszSceneFile ) );
		CancelPlayback();
	}
}

void CSceneEntity::InputSetTarget1( inputdata_t &inputdata )
{
	m_iszTarget1 = MAKE_STRING( inputdata.value.String() );
	m_hActorList.Purge();
	NetworkProp()->NetworkStateForceUpdate();
	m_hTarget1 = FindNamedTarget( m_iszTarget1, false );
}

void CSceneEntity::InputSetTarget2( inputdata_t &inputdata )
{
	m_iszTarget2 = MAKE_STRING( inputdata.value.String() );
	m_hActorList.Purge();
	NetworkProp()->NetworkStateForceUpdate();
	m_hTarget2 = FindNamedTarget( m_iszTarget2, false );
}

void CSceneEntity::InputSetTarget3( inputdata_t &inputdata )
{
	m_iszTarget3 = MAKE_STRING( inputdata.value.String() );
	m_hActorList.Purge();
	NetworkProp()->NetworkStateForceUpdate();
	m_hTarget3 = FindNamedTarget( m_iszTarget3, false );
}

void CSceneEntity::InputSetTarget4( inputdata_t &inputdata )
{
	m_iszTarget4 = MAKE_STRING( inputdata.value.String() );
	m_hActorList.Purge();
	NetworkProp()->NetworkStateForceUpdate();
	m_hTarget4 = FindNamedTarget( m_iszTarget4, false );
}

void CSceneEntity::InputSetTarget5( inputdata_t &inputdata )
{
	m_iszTarget5 = MAKE_STRING( inputdata.value.String() );
	m_hActorList.Purge();
	NetworkProp()->NetworkStateForceUpdate();
	m_hTarget5 = FindNamedTarget( m_iszTarget5, false );
}

void CSceneEntity::InputSetTarget6( inputdata_t &inputdata )
{
	m_iszTarget6 = MAKE_STRING( inputdata.value.String() );
	m_hActorList.Purge();
	NetworkProp()->NetworkStateForceUpdate();
	m_hTarget6 = FindNamedTarget( m_iszTarget6, false );
}

void CSceneEntity::InputSetTarget7( inputdata_t &inputdata )
{
	m_iszTarget7 = MAKE_STRING( inputdata.value.String() );
	m_hActorList.Purge();
	NetworkProp()->NetworkStateForceUpdate();
	m_hTarget7 = FindNamedTarget( m_iszTarget7, false );
}

void CSceneEntity::InputSetTarget8( inputdata_t &inputdata )
{
	m_iszTarget8 = MAKE_STRING( inputdata.value.String() );
	m_hActorList.Purge();
	NetworkProp()->NetworkStateForceUpdate();
	m_hTarget8 = FindNamedTarget( m_iszTarget8, false );
}


void CSceneEntity::InputCancelAtNextInterrupt( inputdata_t &inputdata )
{
	// If we're currently in an interruptable point, interrupt immediately
	if ( IsInterruptable() )
	{
		LocalScene_Printf( "%s : cancelled via input at interrupt point\n", STRING( m_iszSceneFile ) );
		CancelPlayback();
		return;
	}

	// Otherwise, cancel when we next hit an interrupt point
	m_bCancelAtNextInterrupt = true;
}

void CSceneEntity::InputPitchShiftPlayback( inputdata_t &inputdata )
{
	PitchShiftPlayback( inputdata.value.Float() );
}

void CSceneEntity::InputTriggerEvent( inputdata_t &inputdata )
{
	CBaseEntity *pActivator = this; // at some point, find this from the inputdata
	switch ( inputdata.value.Int() )
	{
	case 1:
		m_OnTrigger1.FireOutput( pActivator, this, 0 );
		break;
	case 2:
		m_OnTrigger2.FireOutput( pActivator, this, 0 );
		break;
	case 3:
		m_OnTrigger3.FireOutput( pActivator, this, 0 );
		break;
	case 4:
		m_OnTrigger4.FireOutput( pActivator, this, 0 );
		break;
	case 5:
		m_OnTrigger5.FireOutput( pActivator, this, 0 );
		break;
	case 6:
		m_OnTrigger6.FireOutput( pActivator, this, 0 );
		break;
	case 7:
		m_OnTrigger7.FireOutput( pActivator, this, 0 );
		break;
	case 8:
		m_OnTrigger8.FireOutput( pActivator, this, 0 );
		break;
	case 9:
		m_OnTrigger9.FireOutput( pActivator, this, 0 );
		break;
	case 10:
		m_OnTrigger10.FireOutput( pActivator, this, 0 );
		break;
	case 11:
		m_OnTrigger11.FireOutput( pActivator, this, 0 );
		break;
	case 12:
		m_OnTrigger12.FireOutput( pActivator, this, 0 );
		break;
	case 13:
		m_OnTrigger13.FireOutput( pActivator, this, 0 );
		break;
	case 14:
		m_OnTrigger14.FireOutput( pActivator, this, 0 );
		break;
	case 15:
		m_OnTrigger15.FireOutput( pActivator, this, 0 );
		break;
	case 16:
		m_OnTrigger16.FireOutput( pActivator, this, 0 );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CSceneEntity::InputInterjectResponse( inputdata_t &inputdata )
{
	// Not currently playing a scene
	if ( !m_pScene )
		return;

	CUtlVector<CAI_BaseActor *> candidates;

	for ( int i = 0 ; i < m_pScene->GetNumActors(); i++ )
	{
		CBaseFlex *pTestActor = FindNamedActor( i );
		if ( !pTestActor )
			continue;

		CAI_BaseActor *pBaseActor = dynamic_cast<CAI_BaseActor *>(pTestActor);
		if ( !pBaseActor || !pBaseActor->IsAlive() )
			continue;

		candidates.AddToTail( pBaseActor );
	}

	int c = candidates.Count();
	if ( !c )
		return;

	if ( !m_bIsPlayingBack )
	{
		// Use any actor if not playing a scene
		// int useIndex = RandomInt( 0, c - 1 );
		Assert( !"m_bIsPlayBack is false and this code does nothing. Should it?");
	}
	else
	{
		CUtlString modifiers("scene:");
		modifiers += STRING( GetEntityName() );

		while (candidates.Count() > 0)
		{
			// Pick a random slot in the candidates array.
			int slot = RandomInt( 0, candidates.Count() - 1 );

			CAI_BaseActor *npc = candidates[ slot ];

			// Try to find the response for this slot.
			AI_Response response;
			bool result = npc->SpeakFindResponse( response, inputdata.value.String(), modifiers.Get() );
			if ( result )
			{
				float duration = npc->GetResponseDuration( response );

				if ( ( duration > 0.0f ) && npc->PermitResponse( duration ) )
				{
					// If we could look it up, dispatch it and bail.
					npc->SpeakDispatchResponse( inputdata.value.String(), response );
					return;
				}
			}

			// Remove this entry and look for another one.
			candidates.FastRemove(slot);
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CSceneEntity::InputStopWaitingForActor( inputdata_t &inputdata )
{
	if( m_bIsPlayingBack )
	{
		// Already started.
		return;
	}

	m_bWaitingForActor = false;
}

bool CSceneEntity::CheckActors()
{
	Assert( m_pScene );
	if ( !m_pScene )
		return false;

	int i;
	for ( i = 0 ; i < m_pScene->GetNumActors(); i++ )
	{
		CBaseFlex *pTestActor = FindNamedActor( i );
		if ( !pTestActor )
			continue;

		if ( !pTestActor->MyCombatCharacterPointer() )
			continue;

		if ( !pTestActor->MyCombatCharacterPointer()->IsAlive() )
			return false;

		if ( m_BusyActor == SCENE_BUSYACTOR_WAIT )
		{
			CAI_BaseNPC *pActor = pTestActor->MyNPCPointer();

			if ( pActor )
			{
				bool bShouldWait = false;
				if ( hl2_episodic.GetBool() )
				{
					// Episodic waits until the NPC is fully finished with any .vcd with speech in it
					if ( IsRunningScriptedSceneWithSpeech( pActor ) )
					{
						bShouldWait = true;
					}
					
#ifdef HL2_EPISODIC
					// HACK: Alyx cannot play scenes when she's in the middle of transitioning					
					if ( pActor->IsInAVehicle() )
					{
						CNPC_Alyx *pAlyx = dynamic_cast<CNPC_Alyx *>(pActor);
						if ( pAlyx != NULL && ( pAlyx->GetPassengerState() == PASSENGER_STATE_ENTERING || pAlyx->GetPassengerState() == PASSENGER_STATE_EXITING ) )
						{
							bShouldWait = true;
						}
					}
#endif // HL2_EPISODIC
				}

				if ( pActor->GetExpresser() && pActor->GetExpresser()->IsSpeaking() )
				{
					bShouldWait = true;
				}

				if ( bShouldWait )
				{
					// One of the actors for this scene is talking already.
					// Try again next think.
					m_bWaitingForActor = true;
					return false;
				}
			}
		}
		else if ( m_BusyActor == SCENE_BUSYACTOR_INTERRUPT || m_BusyActor == SCENE_BUSYACTOR_INTERRUPT_CANCEL )
		{
			CBaseCombatCharacter *pActor = pTestActor->MyCombatCharacterPointer();
			if ( pActor && !IsInInterruptableScenes( pActor ) )
			{
				// One of the actors is in a scene that's not at an interrupt point.
				// Wait until the scene finishes or an interrupt point is reached.
				m_bWaitingForInterrupt = true;
				return false;
			}

			if ( m_BusyActor == SCENE_BUSYACTOR_INTERRUPT_CANCEL )
			{
				// Cancel existing scenes
				RemoveActorFromScriptedScenes( pActor, false );
			}
			else
			{
				// Pause existing scenes
				PauseActorsScriptedScenes( pActor, false );
				m_bInterruptedActorsScenes = true;
			}
		}

		pTestActor->StartChoreoScene( m_pScene );
	}

	return true;
}

#if !defined( _RETAIL )
static ConVar scene_async_prefetch_spew( "scene_async_prefetch_spew", "0", 0, "Display async .ani file loading info." );
#endif

void CSceneEntity::PrefetchAnimBlocks( CChoreoScene *scene )
{
	Assert( scene );

	// Build a fast lookup, too
	CUtlMap< CChoreoActor *, CBaseFlex *> actorMap( 0, 0, DefLessFunc( CChoreoActor * ) );
	
	int spew = 
#if !defined( _RETAIL )
		scene_async_prefetch_spew.GetInt();
#else 
		0;
#endif

	int resident = 0;
	int checked = 0;

	// Iterate events and precache necessary resources
	for ( int i = 0; i < scene->GetNumEvents(); i++ )
	{
		CChoreoEvent *event = scene->GetEvent( i );
		if ( !event )
			continue;

		// load any necessary data
		switch ( event->GetType() )
		{
		default:
			break;
		case CChoreoEvent::SEQUENCE:
		case CChoreoEvent::GESTURE:
			{
				CChoreoActor *actor = event->GetActor();
				if ( actor )
				{
					CBaseFlex *pActor = NULL;
					int idx = actorMap.Find( actor );
					if ( idx == actorMap.InvalidIndex() )
					{
						pActor = FindNamedActor( actor );
						idx = actorMap.Insert( actor, pActor );
					}
					else
					{
						pActor = actorMap[ idx ];
					}

					if ( pActor )
					{
						int seq = pActor->LookupSequence( event->GetParameters() );
						if ( seq >= 0 )
						{
							CStudioHdr *pStudioHdr = pActor->GetModelPtr();
							if ( pStudioHdr )
							{
								// Now look up the animblock
								mstudioseqdesc_t &seqdesc = pStudioHdr->pSeqdesc( seq );
								for ( int i = 0 ; i < seqdesc.groupsize[ 0 ] ; ++i )
								{
									for ( int j = 0; j < seqdesc.groupsize[ 1 ]; ++j )
									{
										int animation = seqdesc.anim( i, j );
										int baseanimation = pStudioHdr->iRelativeAnim( seq, animation );
										mstudioanimdesc_t &animdesc = pStudioHdr->pAnimdesc( baseanimation );

										++checked;

										if ( spew != 0 )
										{
											Msg( "%s checking block %d\n", pStudioHdr->pszName(), animdesc.animblock );
										}

										// Async load the animation
										int iFrame = 0;
										const mstudioanim_t *panim = animdesc.pAnim( &iFrame );
										if ( panim )
										{
											++resident;
											if ( spew > 1 )
											{
												Msg( "%s:%s[%i:%i] was resident\n", pStudioHdr->pszName(), animdesc.pszName(), i, j );
											}
										}
										else
										{
											if ( spew != 0 )
											{
												Msg( "%s:%s[%i:%i] async load\n", pStudioHdr->pszName(), animdesc.pszName(), i, j );
											}
										}
									}
								}
							}
						}
					}
				}
			}
			break;
		}
	}

	if ( !spew || checked <= 0 )
		return;

	Msg( "%d of %d animations resident\n", resident, checked );
}

void CSceneEntity::OnLoaded()
{
	// Nothing
}

//-----------------------------------------------------------------------------
// Purpose: Initiate scene playback
//-----------------------------------------------------------------------------
void CSceneEntity::StartPlayback( void )
{
	if ( !m_pScene )
	{
		if ( m_bSceneMissing )
			return;

		m_pScene = LoadScene( STRING( m_iszSceneFile ), this );
		if ( !m_pScene )
		{
			DevMsg( "%s missing from scenes.image\n", STRING( m_iszSceneFile ) );
			m_bSceneMissing = true;
			return;
		}

		OnLoaded();

		if ( ShouldNetwork() )
		{
			m_nSceneStringIndex = g_pStringTableClientSideChoreoScenes->AddString( CBaseEntity::IsServer(), STRING( m_iszSceneFile ) );
		}

		UpdateTransmitState();
	}

	if ( m_bIsPlayingBack )
		return;

	// Make sure actors are alive and able to handle this scene now, otherwise
	//  we'll wait for them to show up
	if ( !CheckActors() )
	{
		return;
	}

	m_bCompletedEarly = false;
	m_bWaitingForActor	= false;
	m_bWaitingForInterrupt = false;
	m_bIsPlayingBack	= true;
	NetworkProp()->NetworkStateForceUpdate();
	m_bPaused			= false;
	SetCurrentTime( 0.0f, true );
	m_pScene->ResetSimulation();
	ClearInterrupt();

	// Put face back in neutral pose
	ClearSceneEvents( m_pScene, false );

	m_OnStart.FireOutput( this, this, 0 );

	// Aysnchronously load speak sounds
	CUtlSymbolTable prefetchSoundSymbolTable;
	CUtlRBTree< SpeakEventSound_t > soundnames( 0, 0, SpeakEventSoundLessFunc );

	BuildSortedSpeakEventSoundsPrefetchList( m_pScene, prefetchSoundSymbolTable, soundnames, 0.0f );
	PrefetchSpeakEventSounds( prefetchSoundSymbolTable, soundnames );

	// Tell any managers we're within that we've started
	int c = m_hListManagers.Count();
	for ( int i = 0; i < c; i++ )
	{
		if ( m_hListManagers[i] )
		{
			m_hListManagers[i]->SceneStarted( this );
		}
	}

	PrefetchAnimBlocks( m_pScene );
}

//-----------------------------------------------------------------------------
// Purpose: Static method used to sort by event start time
//-----------------------------------------------------------------------------
bool CSceneEntity::SpeakEventSoundLessFunc( const SpeakEventSound_t& lhs, const SpeakEventSound_t& rhs )
{
	return lhs.m_flStartTime < rhs.m_flStartTime;
}

//-----------------------------------------------------------------------------
// Purpose: Prefetches the list of sounds build by BuildSortedSpeakEventSoundsPrefetchList
//-----------------------------------------------------------------------------
void CSceneEntity::PrefetchSpeakEventSounds( CUtlSymbolTable& table, CUtlRBTree< SpeakEventSound_t >& soundnames )
{
	for ( int i = soundnames.FirstInorder(); i != soundnames.InvalidIndex() ; i = soundnames.NextInorder( i ) )
	{
		SpeakEventSound_t& sound = soundnames[ i ];
		// Look it up in the string table
		char const *soundname = table.String( sound.m_Symbol );

		// Warning( "Prefetch %s\n", soundname );

		PrefetchScriptSound( soundname );  
	}
}

//-----------------------------------------------------------------------------
// Purpose:  Builds list of sounds sorted by start time for prefetching 
//-----------------------------------------------------------------------------
void CSceneEntity::BuildSortedSpeakEventSoundsPrefetchList( 
	CChoreoScene *scene, 
	CUtlSymbolTable& table, 
	CUtlRBTree< SpeakEventSound_t >& soundnames,
	float timeOffset )
{
	Assert( scene );

	// Iterate events and precache necessary resources
	for ( int i = 0; i < scene->GetNumEvents(); i++ )
	{
		CChoreoEvent *event = scene->GetEvent( i );
		if ( !event )
			continue;

		// load any necessary data
		switch (event->GetType() )
		{
		default:
			break;
		case CChoreoEvent::SPEAK:
			{
				
				// NOTE:  The script entries associated with .vcds are forced to preload to avoid
				//  loading hitches during triggering
				char soundname[ CChoreoEvent::MAX_CCTOKEN_STRING ];
				Q_strncpy( soundname, event->GetParameters(), sizeof( soundname ) );
				
				if ( event->GetCloseCaptionType() == CChoreoEvent::CC_MASTER )
				{
					event->GetPlaybackCloseCaptionToken( soundname, sizeof( soundname ) );
				}
				
				// In single player, try to use the combined or regular .wav files as needed
				if ( gpGlobals->maxClients == 1 )
				{
					CBasePlayer *player = UTIL_GetLocalPlayer();
					if ( player && !GetSoundNameForPlayer( event, player, soundname, sizeof( soundname ), player ) )
					{
						// Skip to next event
						continue;
					}
				}
				/*
				else
				{
					// UNDONE:  Probably need some other solution in multiplayer... (not sure how to "prefetch" on certain players
					// with one sound, but not prefetch the same sound for others...)
				}
				*/

				SpeakEventSound_t ses;
				ses.m_Symbol = table.AddString( soundname );
				ses.m_flStartTime = timeOffset + event->GetStartTime();

				soundnames.Insert( ses );
			}
			break;
		case CChoreoEvent::SUBSCENE:
			{
				// Only allow a single level of subscenes for now
				if ( !scene->IsSubScene() )
				{
					CChoreoScene *subscene = event->GetSubScene();
					if ( !subscene )
					{
						subscene = LoadScene( event->GetParameters(), this );
						subscene->SetSubScene( true );
						event->SetSubScene( subscene );

						// Now precache it's resources, if any
						BuildSortedSpeakEventSoundsPrefetchList( subscene, table, soundnames, event->GetStartTime() );
					}
				}
			}
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::PausePlayback( void )
{
	if ( !m_bIsPlayingBack )
		return;

	if ( m_bPaused )
		return;

	m_bPaused = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::ResumePlayback( void )
{
	if ( !m_bIsPlayingBack )
		return;

	if ( !m_bPaused )
		return;

	Assert( m_pScene );
	if ( !m_pScene )
	{
		// This should never happen!!!!
		return;
	}

	// FIXME:  Iterate using m_pScene->IterateResumeConditionEvents and 
	//  only resume if the event conditions have all been satisfied

	// FIXME:  Just resume for now
	m_pScene->ResumeSimulation();

	m_bPaused = false;
	m_bPausedViaInput = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::CancelPlayback( void )
{
	if ( !m_bIsPlayingBack )
		return;

    m_bIsPlayingBack		= false;
	m_bPaused				= false;

	m_OnCanceled.FireOutput( this, this, 0 );

	LocalScene_Printf( "%s : %8.2f:  canceled\n", STRING( m_iszSceneFile ), m_flCurrentTime );

	OnSceneFinished( true, false );
}

void CSceneEntity::PitchShiftPlayback( float fPitch )
{
	fPitch = clamp( fPitch, SCENE_MIN_PITCH, SCENE_MAX_PITCH );

	m_fPitch = fPitch;

	if ( !m_pScene )
		return;

	for ( int iActor = 0 ; iActor < m_pScene->GetNumActors(); ++iActor )
	{
		CBaseFlex *pTestActor = FindNamedActor( iActor );

		if ( !pTestActor )
			continue;

		char szBuff[ 256 ];
		
		if ( m_pScene->GetPlayingSoundName( szBuff, sizeof( szBuff ) ) )
		{
			CPASAttenuationFilter filter( pTestActor );
			EmitSound_t params;
			params.m_pSoundName = szBuff;
			params.m_nPitch = 100.0f * fPitch;
			params.m_nFlags = SND_CHANGE_PITCH;
			pTestActor->EmitSound( filter, pTestActor->entindex(), params );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Start a resume scene, if we have one, and resume playing when it finishes
//-----------------------------------------------------------------------------
void CSceneEntity::QueueResumePlayback( void )
{
	// Do we have a resume scene?
	if ( m_iszResumeSceneFile != NULL_STRING )
	{
		bool bStartedScene = false;

		// If it has ".vcd" somewhere in the string, try using it as a scene file first
		if ( Q_stristr( STRING(m_iszResumeSceneFile), ".vcd" ) ) 
		{
			bStartedScene = InstancedScriptedScene( NULL, STRING(m_iszResumeSceneFile), &m_hWaitingForThisResumeScene, 0, false ) != 0;
		}

		// HACKHACK: For now, get the first target, and see if we can find a response for him
		if ( !bStartedScene )
		{
			CBaseFlex *pActor = FindNamedActor( 0 );
			if ( pActor )
			{
				CAI_BaseActor *pBaseActor = dynamic_cast<CAI_BaseActor*>(pActor);
				if ( pBaseActor )
				{
					AI_Response response;
					bool result = pBaseActor->SpeakFindResponse( response, STRING(m_iszResumeSceneFile), NULL );
					if ( result )
					{
						const char *szResponse = response.GetResponsePtr();
						bStartedScene = InstancedScriptedScene( NULL, szResponse, &m_hWaitingForThisResumeScene, 0, false ) != 0;
					}
				}
			}
		}

		// If we started a scene/response, wait for it to finish
		if ( bStartedScene )
		{
			m_bWaitingForResumeScene = true;
		}
		else
		{
			// Failed to create the scene. Resume immediately.
			ResumePlayback();
		}
	}
	else
	{
		// No resume scene, so just resume immediately
		ResumePlayback();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Query whether the scene actually loaded. Only meaninful after Spawn()
//-----------------------------------------------------------------------------
bool CSceneEntity::ValidScene() const
{
	return ( m_pScene != NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActor - 
//			*scene - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::DispatchStartSubScene( CChoreoScene *scene, CBaseFlex *pActor, CChoreoEvent *event)
{
	if ( !scene->IsSubScene() )
	{
		CChoreoScene *subscene = event->GetSubScene();
		if ( !subscene )
		{
			Assert( 0 );
			/*
			subscene = LoadScene( event->GetParameters() );
			subscene->SetSubScene( true );
			event->SetSubScene( subscene );
			*/
		}

		if ( subscene )
		{
			subscene->ResetSimulation();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: All events are leading edge triggered
// Input  : currenttime - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event );

	if ( !Q_stricmp( event->GetName(), "NULL" ) )
 	{
 		LocalScene_Printf( "%s : %8.2f:  ignored %s\n", STRING( m_iszSceneFile ), currenttime, event->GetDescription() );
 		return;
 	}
 

	CBaseFlex *pActor = NULL;
	CChoreoActor *actor = event->GetActor();
	if ( actor )
	{
		pActor = FindNamedActor( actor );
		if (pActor == NULL)
		{
			Warning( "CSceneEntity %s unable to find actor named \"%s\"\n", STRING(GetEntityName()), actor->GetName() );
			return;
		}
	}

	LocalScene_Printf( "%s : %8.2f:  start %s\n", STRING( m_iszSceneFile ), currenttime, event->GetDescription() );

	switch ( event->GetType() )
	{
	case CChoreoEvent::SUBSCENE:
		{
			if ( pActor && !IsMultiplayer() )
			{
				DispatchStartSubScene( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::EXPRESSION:
		{
			if ( pActor && !IsMultiplayer() )
			{
				DispatchStartExpression( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::FLEXANIMATION:
		{
			if ( pActor && !IsMultiplayer() )
			{
				DispatchStartFlexAnimation( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::LOOKAT:
		{
			if ( pActor && !IsMultiplayer() )
			{
				CBaseEntity *pActor2 = FindNamedEntity( event->GetParameters( ), pActor );
				if ( pActor2 )
				{
					// Huh?
					DispatchStartLookAt( scene, pActor, pActor2, event );
				}
				else
				{
					Warning( "CSceneEntity %s unable to find actor named \"%s\"\n", STRING(GetEntityName()), event->GetParameters() );
				}
			}
		}
		break;
	case CChoreoEvent::SPEAK:
		{
			if ( pActor )
			{
				// Speaking is edge triggered

				// FIXME: dB hack.  soundlevel needs to be moved into inside of wav?
				soundlevel_t iSoundlevel = SNDLVL_TALKING;
				if (event->GetParameters2())
				{
					iSoundlevel = (soundlevel_t)atoi( event->GetParameters2() );
					if (iSoundlevel == SNDLVL_NONE)
						iSoundlevel = SNDLVL_TALKING;
				}

				DispatchStartSpeak( scene, pActor, event, iSoundlevel );
			}
		}
		break;
	case CChoreoEvent::MOVETO:
		{
			// FIXME: make sure moveto's aren't edge triggered
			if ( !event->HasEndTime() )
			{
				event->SetEndTime( event->GetStartTime() + 1.0 );
			}

			if ( pActor && !IsMultiplayer() )
			{
				CBaseEntity *pActor2 = NULL;
				if ( event->GetParameters3( ) && strlen( event->GetParameters3( ) ) > 0 )
				{
					pActor2 = FindNamedEntityClosest( event->GetParameters( ), pActor, false, true, event->GetParameters3( ) );
				}
				else
				{
					pActor2 = FindNamedEntity( event->GetParameters( ), pActor, false, true );
				}
				if ( pActor2 )
				{

					DispatchStartMoveTo( scene, pActor, pActor2, event );
				}
				else
				{
					Warning( "CSceneEntity %s unable to find actor named \"%s\"\n", STRING(GetEntityName()), event->GetParameters() );
				}
			}
		}
		break;
	case CChoreoEvent::FACE:
		{
			if ( pActor && !IsMultiplayer() )
			{
				CBaseEntity *pActor2 = FindNamedEntity( event->GetParameters( ), pActor );
				if ( pActor2 )
				{
					DispatchStartFace( scene, pActor, pActor2, event );
				}
				else
				{
					Warning( "CSceneEntity %s unable to find actor named \"%s\"\n", STRING(GetEntityName()), event->GetParameters() );
				}
			}
		}
		break;
	case CChoreoEvent::GESTURE:
		{
			if ( pActor )
			{
				DispatchStartGesture( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::GENERIC:
		{
			// If the first token in the parameters is "debugtext", print the rest of the text
			if ( event->GetParameters() && !Q_strncmp( event->GetParameters(), "debugtext", 9 ) )
			{
				const char *pszText = event->GetParameters() + 10;

				hudtextparms_s tTextParam;
				tTextParam.x			= -1;
				tTextParam.y			= 0.65;
				tTextParam.effect		= 0;
				tTextParam.r1			= 255;
				tTextParam.g1			= 170;
				tTextParam.b1			= 0;
				tTextParam.a1			= 255;
				tTextParam.r2			= 255;
				tTextParam.g2			= 170;
				tTextParam.b2			= 0;
				tTextParam.a2			= 255;
				tTextParam.fadeinTime	= 0;
				tTextParam.fadeoutTime	= 0;
				tTextParam.holdTime		= 3.1;
				tTextParam.fxTime		= 0;
				tTextParam.channel		= 1;
				UTIL_HudMessageAll( tTextParam, pszText );
				break;
			}

			if ( pActor )
			{
				DispatchStartGeneric( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::FIRETRIGGER:
		{
			if ( IsMultiplayer() )
				break;

			// Don't re-fire triggers during restore, the entities should already reflect all such state...
			if ( m_bRestoring )
			{
				break;
			}

			CBaseEntity *pActivator = pActor;
			if (!pActivator)
			{
				pActivator = this;
			}

			// FIXME:  how do I decide who fired it??
			switch( atoi( event->GetParameters() ) )
			{
			case 1:
				m_OnTrigger1.FireOutput( pActivator, this, 0 );
				break;
			case 2:
				m_OnTrigger2.FireOutput( pActivator, this, 0 );
				break;
			case 3:
				m_OnTrigger3.FireOutput( pActivator, this, 0 );
				break;
			case 4:
				m_OnTrigger4.FireOutput( pActivator, this, 0 );
				break;
			case 5:
				m_OnTrigger5.FireOutput( pActivator, this, 0 );
				break;
			case 6:
				m_OnTrigger6.FireOutput( pActivator, this, 0 );
				break;
			case 7:
				m_OnTrigger7.FireOutput( pActivator, this, 0 );
				break;
			case 8:
				m_OnTrigger8.FireOutput( pActivator, this, 0 );
				break;
			case 9:
				m_OnTrigger9.FireOutput( pActivator, this, 0 );
				break;
			case 10:
				m_OnTrigger10.FireOutput( pActivator, this, 0 );
				break;
			case 11:
				m_OnTrigger11.FireOutput( pActivator, this, 0 );
				break;
			case 12:
				m_OnTrigger12.FireOutput( pActivator, this, 0 );
				break;
			case 13:
				m_OnTrigger13.FireOutput( pActivator, this, 0 );
				break;
			case 14:
				m_OnTrigger14.FireOutput( pActivator, this, 0 );
				break;
			case 15:
				m_OnTrigger15.FireOutput( pActivator, this, 0 );
				break;
			case 16:
				m_OnTrigger16.FireOutput( pActivator, this, 0 );
				break;
			}
		}
		break;
	case CChoreoEvent::SEQUENCE:
		{
			if ( pActor )
			{
				DispatchStartSequence( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::SECTION:
		{
			if ( IsMultiplayer() )
				break;

			// Pauses scene playback
			DispatchPauseScene( scene, event->GetParameters() );
		}
		break;
	case CChoreoEvent::LOOP:
		{
			DispatchProcessLoop( scene, event );
		}
		break;
	case CChoreoEvent::INTERRUPT:
		{
			if ( IsMultiplayer() )
				break;

			DispatchStartInterrupt( scene, event );
		}
		break;

	case CChoreoEvent::STOPPOINT:
		{
			if ( IsMultiplayer() )
				break;

			DispatchStopPoint( scene, event->GetParameters() );
		}
		break;

	case CChoreoEvent::PERMIT_RESPONSES:
		{
			if ( IsMultiplayer() )
				break;

			DispatchStartPermitResponses( scene, pActor, event );
		}
		break;
	default:
		{
			// FIXME: Unhandeled event
			// Assert(0);
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : currenttime - 
//			*event - 
//-----------------------------------------------------------------------------
void CSceneEntity::EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event );

	if ( !Q_stricmp( event->GetName(), "NULL" ) )
 	{
 		return;
 	}

	CBaseFlex *pActor = NULL;
	CChoreoActor *actor = event->GetActor();
	if ( actor )
	{
		pActor = FindNamedActor( actor );
	}

	LocalScene_Printf( "%s : %8.2f:  finish %s\n", STRING( m_iszSceneFile ), currenttime, event->GetDescription() );

	switch ( event->GetType() )
	{
	case CChoreoEvent::EXPRESSION:
		{
			if ( pActor && !IsMultiplayer() )
			{
				DispatchEndExpression( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::SPEAK:
		{
			if ( pActor )
			{
				DispatchEndSpeak( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::FLEXANIMATION:
		{
			if ( pActor && !IsMultiplayer() )
			{
				DispatchEndFlexAnimation( scene, pActor, event );
			}
		}
		break;

	case CChoreoEvent::LOOKAT:
		{
			if ( pActor && !IsMultiplayer() )
			{
				DispatchEndLookAt( scene, pActor, event );
			}
		}
		break;


	case CChoreoEvent::GESTURE:
		{
			if ( pActor )
			{
				DispatchEndGesture( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::GENERIC:
		{
			// If the first token in the parameters is "debugtext", we printed it and we're done
			if ( event->GetParameters() && !Q_strncmp( event->GetParameters(), "debugtext", 9 ) )
				break;

			if ( pActor )
			{
				DispatchEndGeneric( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::SEQUENCE:
		{
			if ( pActor )
			{
				DispatchEndSequence( scene, pActor, event );
			}
		}
		break;

	case CChoreoEvent::FACE:
		{
			if ( pActor && !IsMultiplayer() )
			{
				DispatchEndFace( scene, pActor, event );
			}
		}
		break;

	case CChoreoEvent::MOVETO:
		{
			if ( pActor && !IsMultiplayer() )
			{
				DispatchEndMoveTo( scene, pActor, event );
			}
		}
		break;

	case CChoreoEvent::SUBSCENE:
		{
			if ( IsMultiplayer() )
				break;

			CChoreoScene *subscene = event->GetSubScene();
			if ( subscene )
			{
				subscene->ResetSimulation();
			}
		}
		break;
	case CChoreoEvent::INTERRUPT:
		{
			if ( IsMultiplayer() )
				break;

			DispatchEndInterrupt( scene, event );
		}
		break;

	case CChoreoEvent::PERMIT_RESPONSES:
		{
			if ( IsMultiplayer() )
				break;

			DispatchEndPermitResponses( scene, pActor, event );
		}
		break;
	default:
		break;
	}
}



//-----------------------------------------------------------------------------
// Purpose: Only spew one time per missing scene!!!
// Input  : *scenename - 
//-----------------------------------------------------------------------------
void MissingSceneWarning( char const *scenename )
{
	static CUtlSymbolTable missing;

	// Make sure we only show the message once
	if ( UTL_INVAL_SYMBOL == missing.Find( scenename ) )
	{
		missing.AddString( scenename );

		Warning( "Scene '%s' missing!\n", scenename );
	}
}

bool CSceneEntity::ShouldNetwork() const
{
	if ( m_bMultiplayer )
	{
		if ( m_pScene && 
			( m_pScene->HasEventsOfType( CChoreoEvent::FLEXANIMATION ) || 
			  m_pScene->HasEventsOfType( CChoreoEvent::EXPRESSION )||
			  m_pScene->HasEventsOfType( CChoreoEvent::GESTURE ) ||
			  m_pScene->HasEventsOfType( CChoreoEvent::SEQUENCE ) ) )
		{
			return true;
		}
	}
	else
	{
		if ( m_pScene && 
			( m_pScene->HasEventsOfType( CChoreoEvent::FLEXANIMATION ) || 
			m_pScene->HasEventsOfType( CChoreoEvent::EXPRESSION ) ) )
		{
			return true;
		}
	}

	return false;
}

CChoreoScene *CSceneEntity::LoadScene( const char *filename, IChoreoEventCallback *pCallback )
{
	DevMsg( 2, "Blocking load of scene from '%s'\n", filename );

	char loadfile[MAX_PATH];
	Q_strncpy( loadfile, filename, sizeof( loadfile ) );
	Q_SetExtension( loadfile, ".vcd", sizeof( loadfile ) );
	Q_FixSlashes( loadfile );

	// binary compiled vcd
	void *pBuffer;
	int fileSize;
	if ( !CopySceneFileIntoMemory( loadfile, &pBuffer, &fileSize ) )
	{
		MissingSceneWarning( loadfile );
		return NULL;
	}

	CChoreoScene *pScene = new CChoreoScene( NULL );
	CUtlBuffer buf( pBuffer, fileSize, CUtlBuffer::READ_ONLY );
	if ( !pScene->RestoreFromBinaryBuffer( buf, loadfile, &g_ChoreoStringPool ) )
	{
		Warning( "CSceneEntity::LoadScene: Unable to load binary scene '%s'\n", loadfile );
		delete pScene;
		pScene = NULL;
	}
	else
	{
		pScene->SetPrintFunc( LocalScene_Printf );
		pScene->SetEventCallbackInterface( pCallback );
	}

	FreeSceneFileMemory( pBuffer );
	return pScene;
}

CChoreoScene *BlockingLoadScene( const char *filename )
{
	return CSceneEntity::LoadScene( filename, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::UnloadScene( void )
{
	if ( m_pScene )
	{
		ClearSceneEvents( m_pScene, false );

		for ( int i = 0 ; i < m_pScene->GetNumActors(); i++ )
		{
			CBaseFlex *pTestActor = FindNamedActor( i );

			if ( !pTestActor )
				continue;
		
			pTestActor->RemoveChoreoScene( m_pScene );
		}
	}
	delete m_pScene;
	m_pScene = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame that an event is active (Start/EndEvent as also
//  called)
// Input  : *event - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CSceneEntity::ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	switch ( event->GetType() )
	{
	case CChoreoEvent::SUBSCENE:
		{
			Assert( event->GetType() == CChoreoEvent::SUBSCENE );

			CChoreoScene *subscene = event->GetSubScene();
			if ( !subscene )
				return;

			if ( subscene->SimulationFinished() )
				return;
			
			// Have subscenes think for appropriate time
			subscene->Think( m_flFrameTime );
		}
		break;

	default:
		break;
	}
	
	return;
}



//-----------------------------------------------------------------------------
// Purpose: Called for events that are part of a pause condition
// Input  : *event - 
// Output : Returns true on event completed, false on non-completion.
//-----------------------------------------------------------------------------
bool CSceneEntity::CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	switch ( event->GetType() )
	{
	case CChoreoEvent::SUBSCENE:
		{
		}
		break;
	default:
		{
			CBaseFlex *pActor = NULL;
			CChoreoActor *actor = event->GetActor();
			if ( actor )
			{
				pActor = FindNamedActor( actor );
				if (pActor == NULL)
				{
					Warning( "CSceneEntity %s unable to find actor \"%s\"\n", STRING(GetEntityName()), actor->GetName() );
					return true;
				}
			}
			if (pActor)
			{
				return pActor->CheckSceneEvent( currenttime, scene, event );
			}
		}
		break;
	}
	
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Get a sticky version of a named actor
// Input  : CChoreoActor 
// Output : CBaseFlex
//-----------------------------------------------------------------------------

CBaseFlex *CSceneEntity::FindNamedActor( int index )
{
	if (m_hActorList.Count() == 0)
	{
		m_hActorList.SetCount( m_pScene->GetNumActors() );
		NetworkProp()->NetworkStateForceUpdate();
	}

	if ( !m_hActorList.IsValidIndex( index ) )
	{
		DevWarning( "Scene %s has %d actors, but scene entity only has %d actors\n", m_pScene->GetFilename(), m_pScene->GetNumActors(), m_hActorList.Size() );
		return NULL;
	}

	CBaseFlex *pActor = m_hActorList[ index ];

	if (pActor == NULL || !pActor->IsAlive() )
	{
		CChoreoActor *pChoreoActor = m_pScene->GetActor( index );
		if ( !pChoreoActor )
			return NULL;
		
		pActor = FindNamedActor( pChoreoActor->GetName() );

		if (pActor)
		{
			// save who we found so we'll use them again
			m_hActorList[ index ] = pActor;
			NetworkProp()->NetworkStateForceUpdate();
		}
	}

	return pActor;
}

//-----------------------------------------------------------------------------
// Purpose: Get a sticky version of a named actor
// Input  : CChoreoActor 
// Output : CBaseFlex
//-----------------------------------------------------------------------------

CBaseFlex *CSceneEntity::FindNamedActor( CChoreoActor *pChoreoActor )
{
	int index = m_pScene->FindActorIndex( pChoreoActor );

	if (index >= 0)
	{
		return FindNamedActor( index );
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Search for an actor by name, make sure it can do face poses
// Input  : *name - 
// Output : CBaseFlex
//-----------------------------------------------------------------------------
CBaseFlex *CSceneEntity::FindNamedActor( const char *name )
{
	CBaseEntity *entity = FindNamedEntity( name, NULL, true );

	if ( !entity )
	{
		// Couldn't find actor!
		return NULL;
	}

	// Make sure it can actually do facial animation, etc.
	CBaseFlex *flexEntity = dynamic_cast< CBaseFlex * >( entity );
	if ( !flexEntity )
	{
		// That actor was not a CBaseFlex!
		return NULL;
	}

	return flexEntity;
}

//-----------------------------------------------------------------------------
// Purpose: Find an entity specified by a target name
// Input  : *name - 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CSceneEntity::FindNamedTarget( string_t iszTarget, bool bBaseFlexOnly )
{
	if ( !stricmp( STRING(iszTarget), "!activator" ) )
		return m_hActivator;

	// If we don't have a wildcard in the target, just return the first entity found
	if ( !strchr( STRING(iszTarget), '*' ) )
		return gEntList.FindEntityByName( NULL, iszTarget );

	CBaseEntity *pTarget = NULL;
	while ( (pTarget = gEntList.FindEntityByName( pTarget, iszTarget )) != NULL )
	{
		if ( bBaseFlexOnly )
		{
			// Make sure it can actually do facial animation, etc.
			if ( dynamic_cast< CBaseFlex * >( pTarget ) )
				return pTarget;
		}
		else
		{
			return pTarget;
		}
	}

	// Failed to find one
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Filters entities only if they're clear
//-----------------------------------------------------------------------------
class CSceneFindMarkFilter : public IEntityFindFilter
{
public:
	void SetActor( CBaseEntity *pActor )
	{
		m_hActor = pActor;
	}

	bool ShouldFindEntity( CBaseEntity *pEntity )
	{
		if ( !m_hActor )
			return true;

		// If we find no truly valid marks, we'll just use the first.
		if ( !m_hEntityFound.Get() )
		{
			m_hEntityFound = pEntity;
		}

		// We only want marks that are clear
		trace_t tr;
		Vector vecOrigin = pEntity->GetAbsOrigin();
		AI_TraceHull( vecOrigin, vecOrigin, m_hActor->WorldAlignMins(), m_hActor->WorldAlignMaxs(), MASK_SOLID, m_hActor, COLLISION_GROUP_NONE, &tr );
		if ( tr.startsolid )
		{
			return false;
		}
		m_hEntityFound = pEntity;
		return true;
	}

	CBaseEntity *GetFilterResult( void )
	{
		return m_hEntityFound;
	}

private:
	EHANDLE		m_hActor;

	// To maintain backwards compatability, store off the first mark
	// we find. If we find no truly valid marks, we'll just use the first.
	EHANDLE		m_hEntityFound;
};

HSCRIPT CSceneEntity::ScriptFindNamedEntity( const char *name )
{
	return ToHScript(FindNamedEntity( name, NULL, false, false ));
}

//-----------------------------------------------------------------------------
// Purpose: vscript - create a scene directly from a buffer containing
// a vcd description, and load it into the scene entity.
//-----------------------------------------------------------------------------

bool CSceneEntity::ScriptLoadSceneFromString( const char * pszFilename, const char *pszData )
{
	CChoreoScene *pScene = new CChoreoScene( NULL ); 
	
	// CSceneTokenProcessor SceneTokenProcessor;
	// SceneTokenProcessor.SetBuffer( pszData );
	g_TokenProcessor.SetBuffer( (char *)pszData );

	if ( !pScene->ParseFromBuffer( pszFilename, &g_TokenProcessor ) ) //&SceneTokenProcessor ) )
		{
			Warning( "CSceneEntity::LoadSceneFromString: Unable to parse scene data '%s'\n", pszFilename );
			delete pScene;
			pScene = NULL;
		}
		else
		{
			pScene->SetPrintFunc( LocalScene_Printf );
			pScene->SetEventCallbackInterface( this );

			// precache all sounds for the newly constructed scene
			PrecacheScene( pScene );
		}

	if ( pScene != NULL )
	{
		// release prior scene if present
		UnloadScene();
		m_pScene = pScene;
		return true;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Finds the entity nearest to both entities, and is clear
//-----------------------------------------------------------------------------
class CSceneFindNearestMarkFilter : public IEntityFindFilter
{
public:

	CSceneFindNearestMarkFilter( const CBaseEntity *pActor, const Vector &vecPos2, float flMaxRadius = MAX_TRACE_LENGTH )
	{
		m_vecPos2 = vecPos2;

		m_flMaxSegmentDistance = flMaxRadius;

		m_flNearestToTarget = flMaxRadius;
		m_pNearestToTarget = NULL;
		m_flNearestToActor = flMaxRadius;
		m_pNearestToActor = NULL;

		m_hActor = pActor;
		if (pActor)
		{
			m_vecPos1 = pActor->GetAbsOrigin();
			m_flMaxSegmentDistance = MIN( flMaxRadius, (m_vecPos1 - m_vecPos2).Length() + 1.0 );
			if (m_flMaxSegmentDistance <= 1.0)
			{
				// must be closest to self
				m_flMaxSegmentDistance = MIN( flMaxRadius, MAX_TRACE_LENGTH );
			}
		}
	}

	bool ShouldFindEntity( CBaseEntity *pEntity )
	{
		if ( !m_hActor )
			return true;

		// If we find no truly valid marks, we'll just use the first.
		if ( m_pNearestToActor == NULL )
		{
			m_pNearestToActor = pEntity;
		}

		// We only want marks that are clear
		trace_t tr;
		Vector vecOrigin = pEntity->GetAbsOrigin();
		AI_TraceHull( vecOrigin, vecOrigin, m_hActor->WorldAlignMins(), m_hActor->WorldAlignMaxs(), MASK_SOLID, m_hActor, COLLISION_GROUP_NONE, &tr );
		if ( !tr.startsolid || tr.m_pEnt == m_hActor)
		{
			float dist1 = (m_vecPos1 - pEntity->GetAbsOrigin()).Length();
			float dist2 = (m_vecPos2 - pEntity->GetAbsOrigin()).Length();
			/*
			char text[256];
			Q_snprintf( text, sizeof( text ), "%.0f : %.0f", dist1, dist2 );
			NDebugOverlay::Text( pEntity->GetAbsOrigin() + Vector( 0, 0, 8 ), text, false, 5.0f );
			*/
			// find the point closest to the actor
			if (dist1 <= m_flNearestToActor)
			{
				m_pNearestToActor = pEntity;
				m_flNearestToActor = dist2;
			}
			// find that node that's closest to both, but the distance to it from the actor isn't farther than 
			// the distance to the second node.  This should keep the actor from walking past their point of interest
			if (dist1 <= m_flMaxSegmentDistance && dist2 <= m_flMaxSegmentDistance && dist2 < m_flNearestToTarget)
			{
				m_pNearestToTarget = pEntity;
				m_flNearestToTarget = dist2;
			}
		}

		return false;
	}

	CBaseEntity *GetFilterResult( void ) 
	{ 
		if (m_pNearestToTarget)
			return m_pNearestToTarget;
		return m_pNearestToActor;
	}

private:
	EHANDLE		m_hActor;
	Vector		m_vecPos1;
	Vector		m_vecPos2;
	float		m_flMaxSegmentDistance;
	float		m_flNearestToTarget;
	CBaseEntity *m_pNearestToTarget;
	float		m_flNearestToActor;
	CBaseEntity *m_pNearestToActor;
};

//-----------------------------------------------------------------------------
// Purpose: Search for an actor by name, make sure it can do face poses
// Input  : *name - 
// Output : CBaseFlex
//-----------------------------------------------------------------------------
CBaseEntity *CSceneEntity::FindNamedEntity( const char *name, CBaseEntity *pActor, bool bBaseFlexOnly, bool bUseClear )
{
	CBaseEntity *entity = NULL;

	if ( !stricmp( name, "Player" ) || !stricmp( name, "!player" ))
	{
		entity = ( gpGlobals->maxClients == 1 ) ? ( CBaseEntity * )UTIL_GetLocalPlayer() : NULL;
	}
	else if ( !stricmp( name, "!target1" ) )
	{
		if (m_hTarget1 == NULL)
		{
			m_hTarget1 = FindNamedTarget( m_iszTarget1, bBaseFlexOnly );
		}
		return m_hTarget1;
	}
	else if ( !stricmp( name, "!target2" ) )
	{
		if (m_hTarget2 == NULL)
		{
			m_hTarget2 = FindNamedTarget( m_iszTarget2, bBaseFlexOnly );
		}
		return m_hTarget2;
	}
	else if ( !stricmp( name, "!target3" ) )
	{
		if (m_hTarget3 == NULL)
		{
			m_hTarget3 = FindNamedTarget( m_iszTarget3, bBaseFlexOnly );
		}
		return m_hTarget3;
	}
	else if ( !stricmp( name, "!target4" ) )
	{
		if (m_hTarget4 == NULL)
		{
			m_hTarget4 = FindNamedTarget( m_iszTarget4, bBaseFlexOnly );
		}
		return m_hTarget4;
	}
	else if ( !stricmp( name, "!target5" ) )
	{
		if (m_hTarget5 == NULL)
		{
			m_hTarget5 = FindNamedTarget( m_iszTarget5, bBaseFlexOnly );
		}
		return m_hTarget5;
	}
	else if ( !stricmp( name, "!target6" ) )
	{
		if (m_hTarget6 == NULL)
		{
			m_hTarget6 = FindNamedTarget( m_iszTarget6, bBaseFlexOnly );
		}
		return m_hTarget6;
	}
	else if ( !stricmp( name, "!target7" ) )
	{
		if (m_hTarget7 == NULL)
		{
			m_hTarget7 = FindNamedTarget( m_iszTarget7, bBaseFlexOnly );
		}
		return m_hTarget7;
	}
	else if ( !stricmp( name, "!target8" ) )
	{
		if (m_hTarget8 == NULL)
		{
			m_hTarget8 = FindNamedTarget( m_iszTarget8, bBaseFlexOnly );
		}
		return m_hTarget8;
	}
	else if (pActor && pActor->MyNPCPointer())
	{
		CSceneFindMarkFilter *pFilter = NULL;
		if ( bUseClear )
		{
			pFilter = new CSceneFindMarkFilter();
			pFilter->SetActor( pActor );
		}

		entity = pActor->MyNPCPointer()->FindNamedEntity( name, pFilter );
		if ( !entity && pFilter )
		{
			entity = pFilter->GetFilterResult();
		}
	}	
	else
	{
		// search for up to 32 entities with the same name and choose one randomly
		CBaseEntity *entityList[ FINDNAMEDENTITY_MAX_ENTITIES ];
		int	iCount;

		entity = NULL;
		for( iCount = 0; iCount < FINDNAMEDENTITY_MAX_ENTITIES; iCount++ )
		{
			entity = gEntList.FindEntityByName( entity, name, NULL, pActor );
			if ( !entity )
			{
				break;
			}
			entityList[ iCount ] = entity;
		}

		if ( iCount > 0 )
		{
			entity = entityList[ RandomInt( 0, iCount - 1 ) ];
		}
		else
		{
			entity = NULL;
		}
	}

	return entity;
}


//-----------------------------------------------------------------------------
// Purpose: Search for an actor by name, make sure it can do face poses
// Input  : *name - 
// Output : CBaseFlex
//-----------------------------------------------------------------------------
CBaseEntity *CSceneEntity::FindNamedEntityClosest( const char *name, CBaseEntity *pActor, bool bBaseFlexOnly, bool bUseClear, const char *pszSecondary )
{
	CBaseEntity *entity = NULL;

	if ( !stricmp( name, "!activator" ) )
	{
		return m_hActivator;
	} 
	else if ( !stricmp( name, "Player" ) || !stricmp( name, "!player" ))
	{
		entity = ( gpGlobals->maxClients == 1 ) ? ( CBaseEntity * )UTIL_GetLocalPlayer() : NULL;
		return entity;
	}
	else if ( !stricmp( name, "!target1" ) )
	{
		name = STRING( m_iszTarget1 );
	}
	else if ( !stricmp( name, "!target2" ) )
	{
		name = STRING( m_iszTarget2 );
	}
	else if ( !stricmp( name, "!target3" ) )
	{
		name = STRING( m_iszTarget3 );
	}
	else if ( !stricmp( name, "!target4" ) )
	{
		name = STRING( m_iszTarget4 );
	}
	else if ( !stricmp( name, "!target5" ) )
	{
		name = STRING( m_iszTarget5 );
	}
	else if ( !stricmp( name, "!target6" ) )
	{
		name = STRING( m_iszTarget6 );
	}
	else if ( !stricmp( name, "!target7" ) )
	{
		name = STRING( m_iszTarget7 );
	}

	if (pActor && pActor->MyNPCPointer())
	{
		if (pszSecondary && strlen( pszSecondary ) > 0)
		{
			CBaseEntity *pActor2 = FindNamedEntityClosest( pszSecondary, pActor, false, false, NULL );

			if (pActor2)
			{
				CSceneFindNearestMarkFilter *pFilter = new CSceneFindNearestMarkFilter( pActor, pActor2->GetAbsOrigin() );

				entity = pActor->MyNPCPointer()->FindNamedEntity( name, pFilter );
				if (!entity && pFilter)
				{
					entity = pFilter->GetFilterResult();
				}
			}
		}
		if (!entity)
		{
			CSceneFindMarkFilter *pFilter = NULL;
			if ( bUseClear )
			{
				pFilter = new CSceneFindMarkFilter();
				pFilter->SetActor( pActor );
			}

			entity = pActor->MyNPCPointer()->FindNamedEntity( name, pFilter );
			if (!entity && pFilter)
			{
				entity = pFilter->GetFilterResult();
			}
		}
	}	
	else
	{
		// search for up to 32 entities with the same name and choose one randomly
		int	iCount;
		entity = NULL;
		CBaseEntity *current = NULL;
		for( iCount = 0; iCount < FINDNAMEDENTITY_MAX_ENTITIES; iCount++ )
		{
			current = gEntList.FindEntityByName( current, name, NULL, pActor );
			if ( current )
			{
				if (RandomInt( 0, iCount ) == 0)
					entity = current;
			}
		}

		entity = NULL;
	}

	return entity;
}


//-----------------------------------------------------------------------------
// Purpose: Remove all "scene" expressions from all actors in this scene
//-----------------------------------------------------------------------------
void CSceneEntity::ClearSceneEvents( CChoreoScene *scene, bool canceled )
{
	if ( !m_pScene )
		return;

	LocalScene_Printf( "%s : %8.2f:  clearing events\n", STRING( m_iszSceneFile ), m_flCurrentTime );

	int i;
	for ( i = 0 ; i < m_pScene->GetNumActors(); i++ )
	{
		CBaseFlex *pActor = FindNamedActor( i );
		if ( !pActor )
			continue;

		// Clear any existing expressions
		pActor->ClearSceneEvents( scene, canceled );
	}

	// Iterate events and precache necessary resources
	for ( i = 0; i < scene->GetNumEvents(); i++ )
	{
		CChoreoEvent *event = scene->GetEvent( i );
		if ( !event )
			continue;

		// load any necessary data
		switch (event->GetType() )
		{
		default:
			break;
		case CChoreoEvent::SUBSCENE:
			{
				// Only allow a single level of subscenes for now
				if ( !scene->IsSubScene() )
				{
					CChoreoScene *subscene = event->GetSubScene();
					if ( subscene )
					{
						ClearSceneEvents( subscene, canceled );
					}
				}
			}
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Remove all imposed schedules from all actors in this scene
//-----------------------------------------------------------------------------
void CSceneEntity::ClearSchedules( CChoreoScene *scene )
{
	if ( !m_pScene )
		return;

	int i;
	for ( i = 0 ; i < m_pScene->GetNumActors(); i++ )
	{
		CBaseFlex *pActor = FindNamedActor( i );
		if ( !pActor )
			continue;

		CAI_BaseNPC *pNPC = pActor->MyNPCPointer();

		if ( pNPC )
		{
			/*
			if ( pNPC->IsCurSchedule( SCHED_SCENE_GENERIC ) )
				pNPC->ClearSchedule( "Scene entity clearing all actor schedules" );
			*/
		}
		else
		{
			pActor->ResetSequence( pActor->SelectWeightedSequence( ACT_IDLE ) );
			pActor->SetCycle( 0 );
		}
		// Clear any existing expressions
	}

	// Iterate events and precache necessary resources
	for ( i = 0; i < scene->GetNumEvents(); i++ )
	{
		CChoreoEvent *event = scene->GetEvent( i );
		if ( !event )
			continue;

		// load any necessary data
		switch (event->GetType() )
		{
		default:
			break;
		case CChoreoEvent::SUBSCENE:
			{
				// Only allow a single level of subscenes for now
				if ( !scene->IsSubScene() )
				{
					CChoreoScene *subscene = event->GetSubScene();
					if ( subscene )
					{
						ClearSchedules( subscene );
					}
				}
			}
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: If we are currently interruptable, pause this scene and wait for the other
//  scene to finish
// Input  : *otherScene - 
//-----------------------------------------------------------------------------
bool CSceneEntity::InterruptThisScene( CSceneEntity *otherScene )
{
	Assert( otherScene );

	if ( !IsInterruptable() )
	{
		return false;
	}

	// Already interrupted
	if ( m_bInterrupted )
	{
		return false;
	}

	m_bInterrupted		= true;
	m_hInterruptScene	= otherScene;

	// Ask other scene to tell us when it's finished or canceled
	otherScene->RequestCompletionNotification( this );

	PausePlayback();
	return true;
}

/*
void scene_interrupt( const CCommand &args )
{
	if ( args.ArgC() != 3 )
		return;

	const char *scene1 = args[1];
	const char *scene2 = args[2];

	CSceneEntity *s1 = dynamic_cast< CSceneEntity * >( gEntList.FindEntityByName( NULL, scene1 ) );
	CSceneEntity *s2 = dynamic_cast< CSceneEntity * >( gEntList.FindEntityByName( NULL, scene2 ) );

	if ( !s1 || !s2 )
		return;

	if ( s1->InterruptThisScene( s2 ) )
	{
		s2->StartPlayback();
	}
}

static ConCommand interruptscene( "int", scene_interrupt, "interrupt scene 1 with scene 2.", FCVAR_CHEAT );
*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::CheckInterruptCompletion()
{
	if ( !m_bInterrupted )
		return;

	// If the interruptor goes away it's the same as having that scene finish up...
	if ( m_hInterruptScene != NULL && 
		!m_bInterruptSceneFinished )
	{
		return;
	}

	m_bInterrupted = false;
	m_hInterruptScene = NULL;

	ResumePlayback();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::ClearInterrupt()
{
	m_nInterruptCount = 0;
	m_bInterrupted = false;
	m_hInterruptScene = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Another scene is asking us to notify upon completion
// Input  : *notify - 
//-----------------------------------------------------------------------------
void CSceneEntity::RequestCompletionNotification( CSceneEntity *notify )
{
	CHandle< CSceneEntity > h;
	h = notify;
	// Only add it once
	if ( m_hNotifySceneCompletion.Find( h ) == m_hNotifySceneCompletion.InvalidIndex() )
	{
		m_hNotifySceneCompletion.AddToTail( h );
	}
}

//-----------------------------------------------------------------------------
// Purpose: An interrupt scene has finished or been canceled, we can resume once we pick up this state in CheckInterruptCompletion
// Input  : *interruptor - 
//-----------------------------------------------------------------------------
void CSceneEntity::NotifyOfCompletion( CSceneEntity *interruptor )
{
	Assert( m_bInterrupted );
	Assert( m_hInterruptScene == interruptor );
	m_bInterruptSceneFinished = true;

	CheckInterruptCompletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneEntity::AddListManager( CSceneListManager *pManager )
{
	CHandle< CSceneListManager > h;
	h = pManager;
	// Only add it once
	if ( m_hListManagers.Find( h ) == m_hListManagers.InvalidIndex() )
	{
		m_hListManagers.AddToTail( h );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Clear any targets that a referencing !activator
//-----------------------------------------------------------------------------
void CSceneEntity::ClearActivatorTargets( void )
{
	if ( !stricmp( STRING(m_iszTarget1), "!activator" ) )
	{
		// We need to clear out actors so they're re-evaluated
		m_hActorList.Purge();
		NetworkProp()->NetworkStateForceUpdate();
		m_hTarget1 = NULL;
	}
	if ( !stricmp( STRING(m_iszTarget2), "!activator" ) )
	{
		// We need to clear out actors so they're re-evaluated
		m_hActorList.Purge();
		NetworkProp()->NetworkStateForceUpdate();
		m_hTarget2 = NULL;
	}
	if ( !stricmp( STRING(m_iszTarget3), "!activator" ) )
	{
		// We need to clear out actors so they're re-evaluated
		m_hActorList.Purge();
		NetworkProp()->NetworkStateForceUpdate();
		m_hTarget3 = NULL;
	}
	if ( !stricmp( STRING(m_iszTarget4), "!activator" ) )
	{
		// We need to clear out actors so they're re-evaluated
		m_hActorList.Purge();
		NetworkProp()->NetworkStateForceUpdate();
		m_hTarget4 = NULL;
	}
	if ( !stricmp( STRING(m_iszTarget5), "!activator" ) )
	{
		// We need to clear out actors so they're re-evaluated
		m_hActorList.Purge();
		NetworkProp()->NetworkStateForceUpdate();
		m_hTarget5 = NULL;
	}
	if ( !stricmp( STRING(m_iszTarget6), "!activator" ) )
	{
		// We need to clear out actors so they're re-evaluated
		m_hActorList.Purge();
		NetworkProp()->NetworkStateForceUpdate();
		m_hTarget6 = NULL;
	}
	if ( !stricmp( STRING(m_iszTarget7), "!activator" ) )
	{
		// We need to clear out actors so they're re-evaluated
		m_hActorList.Purge();
		NetworkProp()->NetworkStateForceUpdate();
		m_hTarget7 = NULL;
	}
	if ( !stricmp( STRING(m_iszTarget8), "!activator" ) )
	{
		// We need to clear out actors so they're re-evaluated
		m_hActorList.Purge();
		NetworkProp()->NetworkStateForceUpdate();
		m_hTarget8 = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when a scene is completed or canceled
//-----------------------------------------------------------------------------
void CSceneEntity::OnSceneFinished( bool canceled, bool fireoutput )
{
	if ( !m_pScene )
		return;

	LocalScene_Printf( "%s : %8.2f:  finished\n", STRING( m_iszSceneFile ), m_flCurrentTime );

	// Notify any listeners
	int c = m_hNotifySceneCompletion.Count();
	int i;
	for ( i = 0; i < c; i++ )
	{
		CSceneEntity *ent = m_hNotifySceneCompletion[ i ].Get();
		if ( !ent )
			continue;

		ent->NotifyOfCompletion( this );
	}
	m_hNotifySceneCompletion.RemoveAll();

	// Clear simulation
	m_pScene->ResetSimulation();
	m_bIsPlayingBack = false;
	m_bPaused = false;
	SetCurrentTime( 0.0f, false );
	
	// Clear interrupt state if we were interrupted for some reason
	ClearInterrupt();

	if ( fireoutput && !m_bCompletedEarly)
	{
		m_OnCompletion.FireOutput( this, this, 0 );
	}

	// Put face back in neutral pose
	ClearSceneEvents( m_pScene, canceled );

	for ( i = 0 ; i < m_pScene->GetNumActors(); i++ )
	{
		CBaseFlex *pTestActor = FindNamedActor( i );

		if ( !pTestActor )
			continue;

		pTestActor->RemoveChoreoScene( m_pScene, canceled );

		// If we interrupted the actor's previous scenes, resume them
		if ( m_bInterruptedActorsScenes )
		{
			QueueActorsScriptedScenesToResume( pTestActor, false );
		}
	}
}

//-----------------------------------------------------------------------------
// Should we transmit it to the client?
//-----------------------------------------------------------------------------
int CSceneEntity::UpdateTransmitState()
{
	if ( !ShouldNetwork() )
	{
		return SetTransmitState( FL_EDICT_DONTSEND );
	}

	if ( m_pRecipientFilter )
	{
		return SetTransmitState( FL_EDICT_FULLCHECK );
	}

	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: Which clients should we be transmitting to?
//-----------------------------------------------------------------------------
int CSceneEntity::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	int result = BaseClass::ShouldTransmit( pInfo );

	// if we have excluded them via our recipient filter, don't send
	if ( m_pRecipientFilter && result != FL_EDICT_DONTSEND )
	{
		bool bFound = false;

		// If we can't find them in the recipient list, exclude
		int i;
		for ( i=0; i<m_pRecipientFilter->GetRecipientCount();i++ )
		{
			int iRecipient = m_pRecipientFilter->GetRecipientIndex(i);

			CBasePlayer *player = static_cast< CBasePlayer * >( CBaseEntity::Instance( iRecipient ) );

			if ( player && player->edict() == pInfo->m_pClientEnt )
			{
				bFound = true;
				break;
			}
		}

		if ( !bFound )
		{
			result = FL_EDICT_DONTSEND;
		}
	}

	return result;
}

void CSceneEntity::SetRecipientFilter( IRecipientFilter *filter )
{
	// create a copy of this filter
	if ( filter )
	{
		m_pRecipientFilter = new CRecipientFilter();
		m_pRecipientFilter->CopyFrom( (CRecipientFilter &)( *filter ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds a player (by index) to the recipient filter
//-----------------------------------------------------------------------------
void CSceneEntity::AddBroadcastTeamTarget( int nTeamIndex )
{
	if ( m_pRecipientFilter == NULL )
	{
		CRecipientFilter filter;
		SetRecipientFilter( &filter );
	}

	CTeam *pTeam = GetGlobalTeam( nTeamIndex );
	Assert( pTeam );
	if ( pTeam == NULL )
		return;

	m_pRecipientFilter->AddRecipientsByTeam( pTeam );
}

//-----------------------------------------------------------------------------
// Purpose: Removes a player (by index) from the recipient filter
//-----------------------------------------------------------------------------
void CSceneEntity::RemoveBroadcastTeamTarget( int nTeamIndex )
{
	if ( m_pRecipientFilter == NULL )
	{
		CRecipientFilter filter;
		SetRecipientFilter( &filter );
	}

	CTeam *pTeam = GetGlobalTeam( nTeamIndex );
	Assert( pTeam );
	if ( pTeam == NULL )
		return;

	m_pRecipientFilter->RemoveRecipientsByTeam( pTeam );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output :
//-----------------------------------------------------------------------------
class CInstancedSceneEntity : public CSceneEntity
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CInstancedSceneEntity, CSceneEntity ); 
public:
	EHANDLE					m_hOwner;
	bool					m_bHadOwner;
	float					m_flPostSpeakDelay;
	float					m_flPreDelay;
	char					m_szInstanceFilename[ CChoreoScene::MAX_SCENE_FILENAME ];
	bool					m_bIsBackground;

	virtual void			StartPlayback( void );
	virtual void			DoThink( float frametime );
	virtual CBaseFlex		*FindNamedActor( const char *name );
	virtual CBaseEntity		*FindNamedEntity( const char *name );
	virtual float			GetPostSpeakDelay()	{ return m_flPostSpeakDelay; }
	virtual void			SetPostSpeakDelay( float flDelay ) { m_flPostSpeakDelay = flDelay; }
	virtual float			GetPreDelay()	{ return m_flPreDelay; }
	virtual void			SetPreDelay( float flDelay ) { m_flPreDelay = flDelay; }

	virtual void			OnLoaded();

	virtual void			DispatchStartMoveTo( CChoreoScene *scene, CBaseFlex *actor, CBaseEntity *actor2, CChoreoEvent *event )
							{ 
								if (PassThrough( actor )) BaseClass::DispatchStartMoveTo( scene, actor, actor2, event ); 
							};

	virtual void			DispatchEndMoveTo( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event ) 
							{ 
								if (PassThrough( actor )) BaseClass::DispatchEndMoveTo( scene, actor, event ); 
							};

	virtual void			DispatchStartFace( CChoreoScene *scene, CBaseFlex *actor, CBaseEntity *actor2, CChoreoEvent *event ) 
							{ 
								if (PassThrough( actor )) BaseClass::DispatchStartFace( scene, actor, actor2, event ); 
							};

	virtual void			DispatchEndFace( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event ) 
							{ 
								if (PassThrough( actor )) BaseClass::DispatchEndFace( scene, actor, event ); 
							};

	virtual void			DispatchStartSequence( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )  
							{ 
								if ( IsMultiplayer() )
								{
									BaseClass::DispatchStartSequence( scene, actor, event );
								}
							};
	virtual void			DispatchEndSequence( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
							{						
								if ( IsMultiplayer() )
								{
									BaseClass::DispatchEndSequence( scene, actor, event );
								}
							};
	virtual void			DispatchPauseScene( CChoreoScene *scene, const char *parameters ) { /* suppress */ };

	void OnRestore();

	virtual float			EstimateLength( void );

private:
	bool					PassThrough( CBaseFlex *actor );
};

LINK_ENTITY_TO_CLASS( instanced_scripted_scene, CInstancedSceneEntity );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CInstancedSceneEntity )

	DEFINE_FIELD( m_hOwner,				FIELD_EHANDLE ),
	DEFINE_FIELD( m_bHadOwner,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flPostSpeakDelay,	FIELD_FLOAT ),
	DEFINE_FIELD( m_flPreDelay,	FIELD_FLOAT ),
	DEFINE_AUTO_ARRAY( m_szInstanceFilename, FIELD_CHARACTER ),
	DEFINE_FIELD( m_bIsBackground,		FIELD_BOOLEAN ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: create a one-shot scene, no movement, sequences, etc.
// Input  :
// Output :
//-----------------------------------------------------------------------------
float InstancedScriptedScene( CBaseFlex *pActor, const char *pszScene, EHANDLE *phSceneEnt,
							 float flPostDelay, bool bIsBackground, AI_Response *response,
							 bool bMultiplayer, IRecipientFilter *filter /* = NULL */ )
{
	VPROF( "InstancedScriptedScene" );

	CInstancedSceneEntity *pScene = (CInstancedSceneEntity *)CBaseEntity::CreateNoSpawn( "instanced_scripted_scene", vec3_origin, vec3_angle );

	// This code expands any $gender tags into male or female tags based on the gender of the actor (based on his/her .mdl)
	if ( pActor )
	{
		pActor->GenderExpandString( pszScene, pScene->m_szInstanceFilename, sizeof( pScene->m_szInstanceFilename ) );
	}
	else
	{
		Q_strncpy( pScene->m_szInstanceFilename, pszScene, sizeof( pScene->m_szInstanceFilename ) );
	}
	pScene->m_iszSceneFile = MAKE_STRING( pScene->m_szInstanceFilename );

	// FIXME: I should set my output to fire something that kills me....

	// FIXME: add a proper initialization function
	pScene->m_hOwner = pActor;
	pScene->m_bHadOwner = pActor != NULL;
	pScene->m_bMultiplayer = bMultiplayer;
	pScene->SetPostSpeakDelay( flPostDelay );
	DispatchSpawn( pScene );
	pScene->Activate();
	pScene->m_bIsBackground = bIsBackground;

	pScene->SetBackground( bIsBackground );
	pScene->SetRecipientFilter( filter );
	
	if ( response )
	{
		float flPreDelay = response->GetPreDelay();
		if ( flPreDelay )
		{
			pScene->SetPreDelay( flPreDelay );
		}
	}

	pScene->StartPlayback();

	if ( response )
	{
		// If the response wants us to abort on NPC state switch, remember that
		pScene->SetBreakOnNonIdle( response->ShouldBreakOnNonIdle() );
	}

	if ( phSceneEnt )
	{
		*phSceneEnt = pScene;
	}

	return pScene->EstimateLength();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActor - 
//			*soundnmame - 
//			*phSceneEnt - 
// Output : float
//-----------------------------------------------------------------------------
float InstancedAutoGeneratedSoundScene( CBaseFlex *pActor, char const *soundname, EHANDLE *phSceneEnt /*= NULL*/ )
{
	if ( !pActor )
	{
		Warning( "InstancedAutoGeneratedSoundScene:  Expecting non-NULL pActor for sound %s\n", soundname );
		return 0;
	}

	CInstancedSceneEntity *pScene = (CInstancedSceneEntity *)CBaseEntity::CreateNoSpawn( "instanced_scripted_scene", vec3_origin, vec3_angle );

	Q_strncpy( pScene->m_szInstanceFilename, UTIL_VarArgs( "AutoGenerated(%s)", soundname ), sizeof( pScene->m_szInstanceFilename ) );
	pScene->m_iszSceneFile = MAKE_STRING( pScene->m_szInstanceFilename );

	pScene->m_hOwner = pActor;
	pScene->m_bHadOwner = pActor != NULL;

	pScene->GenerateSoundScene( pActor, soundname );

	pScene->Spawn();
	pScene->Activate();
	pScene->StartPlayback();

	if ( phSceneEnt )
	{
		*phSceneEnt = pScene;
	}

	return pScene->EstimateLength();
}

//-----------------------------------------------------------------------------

void StopScriptedScene( CBaseFlex *pActor, EHANDLE hSceneEnt )
{
	CBaseEntity *pEntity = hSceneEnt;
	CSceneEntity *pScene = dynamic_cast<CSceneEntity *>(pEntity);

	if ( pScene )
	{
		LocalScene_Printf( "%s : stop scripted scene\n", STRING( pScene->m_iszSceneFile ) );
		pScene->CancelPlayback();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszScene - 
// Output : float
//-----------------------------------------------------------------------------
float GetSceneDuration( char const *pszScene )
{
	unsigned int msecs = 0;

	SceneCachedData_t cachedData;
	if ( scenefilecache->GetSceneCachedData( pszScene, &cachedData ) )
	{
		msecs = cachedData.msecs;
	}

	return (float)msecs * 0.001f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszScene - 
// Output : int
//-----------------------------------------------------------------------------
int GetSceneSpeechCount( char const *pszScene )
{
	SceneCachedData_t cachedData;
	if ( scenefilecache->GetSceneCachedData( pszScene, &cachedData ) )
	{
		return cachedData.numSounds;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Used for precaching instanced scenes
// Input  : *pszScene - 
//-----------------------------------------------------------------------------
void PrecacheInstancedScene( char const *pszScene )
{
	static int nMakingReslists = -1;
	
	if ( nMakingReslists == -1 )
	{
		nMakingReslists = CommandLine()->FindParm( "-makereslists" ) > 0 ? 1 : 0;
	}

	if ( nMakingReslists == 1 )
	{
		// Just stat the file to add to reslist
		g_pFullFileSystem->Size( pszScene );
	}

	// verify existence, cache is pre-populated, should be there
	SceneCachedData_t sceneData;
	if ( !scenefilecache->GetSceneCachedData( pszScene, &sceneData ) )
	{
		// Scenes are sloppy and don't always exist.
		// A scene that is not in the pre-built cache image, but on disk, is a true error.
		if ( developer.GetInt() && ( IsX360() && ( g_pFullFileSystem->GetDVDMode() != DVDMODE_STRICT ) && g_pFullFileSystem->FileExists( pszScene, "GAME" ) ) )
		{
			Warning( "PrecacheInstancedScene: Missing scene '%s' from scene image cache.\nRebuild scene image cache!\n", pszScene );
		}
	}
	else
	{
		for ( int i = 0; i < sceneData.numSounds; ++i )
		{
			short stringId = scenefilecache->GetSceneCachedSound( sceneData.sceneId, i );
			CBaseEntity::PrecacheScriptSound( scenefilecache->GetSceneString( stringId ) );
		}
	}

	g_pStringTableClientSideChoreoScenes->AddString( CBaseEntity::IsServer(), pszScene );
}

HSCRIPT ScriptCreateSceneEntity( const char* pszScene )
{
	if ( IsEntityCreationAllowedInScripts() == false )
	{
		Warning( "VScript error: A script attempted to create a scene entity mid-game. Entity creation from scripts is only allowed during map init.\n" );
		return NULL;
	}

	g_pScriptVM->RegisterClass( GetScriptDescForClass( CSceneEntity ) );
	CSceneEntity *pScene = (CSceneEntity *)CBaseEntity::CreateNoSpawn( "logic_choreographed_scene", vec3_origin, vec3_angle );

	if ( pScene )
	{
		pScene->m_iszSceneFile = AllocPooledString( pszScene );
		DispatchSpawn( pScene );
	}

	return ToHScript( pScene );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInstancedSceneEntity::StartPlayback( void )
{
	// Wait until our pre delay is over
	if ( GetPreDelay() )
		return;

	BaseClass::StartPlayback();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CInstancedSceneEntity::DoThink( float frametime )
{
	CheckInterruptCompletion();

	if ( m_flPreDelay > 0 )
	{
		m_flPreDelay = MAX( 0, m_flPreDelay - frametime );
		StartPlayback();
		if ( !m_bIsPlayingBack )
			return;
	}

	if ( !m_pScene || !m_bIsPlayingBack || ( m_bHadOwner && m_hOwner == NULL ) )
	{
		UTIL_Remove( this );
		return;
	}

	// catch bad pitch shifting from old save games
	Assert( m_fPitch >= SCENE_MIN_PITCH && m_fPitch <= SCENE_MAX_PITCH );
	m_fPitch = clamp( m_fPitch, SCENE_MIN_PITCH, SCENE_MAX_PITCH );

	if ( m_bPaused )
	{
		PauseThink();
		return;
	}

	float dt = frametime;

	m_pScene->SetSoundFileStartupLatency( GetSoundSystemLatency() );

	// Tell scene to go
	m_pScene->Think( m_flCurrentTime );
	// Drive simulation time for scene
	SetCurrentTime( m_flCurrentTime + dt * m_fPitch, false );

	// Did we get to the end
	if ( m_pScene->SimulationFinished() )
	{
		OnSceneFinished( false, false );

		UTIL_Remove( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Search for an actor by name, make sure it can do face poses
// Input  : *name - 
// Output : CBaseFlex
//-----------------------------------------------------------------------------
CBaseFlex *CInstancedSceneEntity::FindNamedActor( const char *name )
{
	if ( m_pScene->GetNumActors() == 1 || stricmp( name, "!self" ) == 0 )
	{
		if ( m_hOwner != NULL )
		{
			CBaseCombatCharacter *pCharacter = m_hOwner->MyCombatCharacterPointer();
			if ( pCharacter )
			{
				return pCharacter;
			}
		}
	}
	return BaseClass::FindNamedActor( name );
}


//-----------------------------------------------------------------------------
// Purpose: Search for an actor by name, make sure it can do face poses
// Input  : *name - 
// Output : CBaseFlex
//-----------------------------------------------------------------------------
CBaseEntity *CInstancedSceneEntity::FindNamedEntity( const char *name )
{
	CBaseEntity *pOther = NULL;

	if (m_hOwner != NULL)
	{
		CAI_BaseNPC	*npc = m_hOwner->MyNPCPointer();

		if (npc)
		{
			pOther = npc->FindNamedEntity( name );
		}
		else if ( m_hOwner->MyCombatCharacterPointer() )
		{
			pOther = m_hOwner;
		}
	}

	if (!pOther)
	{
		pOther = BaseClass::FindNamedEntity( name );
	}
	return pOther;
}


//-----------------------------------------------------------------------------
// Purpose: Suppress certain events when it's instanced since they're can cause odd problems
// Input  : actor
// Output : true - the event should happen, false - it shouldn't
//-----------------------------------------------------------------------------

bool CInstancedSceneEntity::PassThrough( CBaseFlex *actor )
{
	if (!actor)
		return false;

	CAI_BaseNPC *myNpc = actor->MyNPCPointer( );

	if (!myNpc)
		return false;

	if (myNpc->IsCurSchedule( SCHED_SCENE_GENERIC ))
	{
		return true;
	}

	if (myNpc->GetCurSchedule())
	{
		CAI_ScheduleBits testBits;
		myNpc->GetCurSchedule()->GetInterruptMask( &testBits );

		if (testBits.IsBitSet( COND_IDLE_INTERRUPT )) 
		{
			return true;
		}
	}

	LocalScene_Printf( "%s : event suppressed\n", STRING( m_iszSceneFile ) );

	return false;
}


//-----------------------------------------------------------------------------
void CInstancedSceneEntity::OnRestore()
{
	if ( m_bHadOwner && !m_hOwner )
	{
		// probably just came back from a level transition
		UTIL_Remove( this );
		return;
	}
	// reset background state
	if ( m_pScene )
	{
		m_pScene->SetBackground( m_bIsBackground );
	}
	BaseClass::OnRestore();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CInstancedSceneEntity::EstimateLength( void )
{
	return (BaseClass::EstimateLength() + GetPreDelay());
}


void CInstancedSceneEntity::OnLoaded()
{
	BaseClass::OnLoaded();
	SetBackground( m_bIsBackground );
}

bool g_bClientFlex = true;

LINK_ENTITY_TO_CLASS( scene_manager, CSceneManager );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneManager::Think()
{
	// Latch this only once per frame...
	g_bClientFlex = scene_clientflex.GetBool();

	// The manager is always thinking at 20 hz
	SetNextThink( gpGlobals->curtime + SCENE_THINK_INTERVAL );
	float frameTime = ( gpGlobals->curtime - GetLastThink() );
	frameTime = MIN( 0.1, frameTime );

	// stop if AI is diabled
	if (CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI)
		return;

	bool needCleanupPass = false;
	int c = m_ActiveScenes.Count();
	for ( int i = 0; i < c; i++ )
	{
		CSceneEntity *scene = m_ActiveScenes[ i ].Get();
		if ( !scene )
		{
			needCleanupPass = true;
			continue;
		}

		scene->DoThink( frameTime );

		if ( m_ActiveScenes.Count() < c )
		{
			// Scene removed self while thinking. Adjust iteration.
			c = m_ActiveScenes.Count();
			i--;
		}
	}

	// Now delete any invalid ones
	if ( needCleanupPass )
	{
		for ( int i = c - 1; i >= 0; i-- )
		{
			CSceneEntity *scene = m_ActiveScenes[ i ].Get();
			if ( scene )
				continue;

			m_ActiveScenes.Remove( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneManager::ClearAllScenes()
{
	m_ActiveScenes.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//-----------------------------------------------------------------------------
void CSceneManager::AddSceneEntity( CSceneEntity *scene )
{
	CHandle< CSceneEntity > h;
	
	h = scene;

	// Already added/activated
	if ( m_ActiveScenes.Find( h ) != m_ActiveScenes.InvalidIndex() )
	{
		return;
	}

	m_ActiveScenes.AddToTail( h );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//-----------------------------------------------------------------------------
void CSceneManager::RemoveSceneEntity( CSceneEntity *scene )
{
	CHandle< CSceneEntity > h;
	
	h = scene;

	m_ActiveScenes.FindAndRemove( h );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
//-----------------------------------------------------------------------------
void CSceneManager::OnClientActive( CBasePlayer *player )
{
	int c = m_QueuedSceneSounds.Count();
	for ( int i = 0; i < c; i++ )
	{
		CRestoreSceneSound *sound = &m_QueuedSceneSounds[ i ];

		if ( sound->actor == NULL )
			continue;

		// Blow off sounds too far in past to encode over networking layer
		if ( fabs( 1000.0f * sound->time_in_past ) > MAX_SOUND_DELAY_MSEC )
			continue;

		CPASAttenuationFilter filter( sound->actor );
		
		EmitSound_t es;
		es.m_nChannel = CHAN_VOICE;
		es.m_flVolume = 1;
		es.m_pSoundName = sound->soundname;
		es.m_SoundLevel = sound->soundlevel;
		es.m_flSoundTime = gpGlobals->curtime - sound->time_in_past;

		EmitSound( filter, sound->actor->entindex(), es );
	}

	m_QueuedSceneSounds.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Deletes scenes involving the specified actor
//-----------------------------------------------------------------------------
void CSceneManager::RemoveScenesInvolvingActor( CBaseFlex *pActor )
{
	if ( !pActor )
		return;

	// This loop can remove items from m_ActiveScenes array, so loop through backwards.
	int c = m_ActiveScenes.Count();
	for ( int i = c - 1 ; i >= 0; --i )
	{
		CSceneEntity *pScene = m_ActiveScenes[ i ].Get();
		if ( !pScene )
		{
			continue;
		}

		if ( pScene->InvolvesActor( pActor ) ) // NOTE: returns false if scene hasn't loaded yet
		{
			LocalScene_Printf( "%s : removed for '%s'\n", STRING( pScene->m_iszSceneFile ), pActor ? pActor->GetDebugName() : "NULL" );
			pScene->CancelPlayback();
		}
		else
		{
			CInstancedSceneEntity *pInstancedScene = dynamic_cast< CInstancedSceneEntity * >( pScene );
			if ( pInstancedScene && pInstancedScene->m_hOwner )
			{
				if ( pInstancedScene->m_hOwner == pActor )
				{
					if ( pInstancedScene->m_bIsPlayingBack )
					{
						pInstancedScene->OnSceneFinished( true, false );
					}

					LocalScene_Printf( "%s : removed for '%s'\n", STRING( pInstancedScene->m_iszSceneFile ), pActor ? pActor->GetDebugName() : "NULL" );
					UTIL_Remove( pInstancedScene );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stops scenes involving the specified actor
//-----------------------------------------------------------------------------
void CSceneManager::RemoveActorFromScenes( CBaseFlex *pActor, bool bInstancedOnly, bool bNonIdleOnly, const char *pszThisSceneOnly )
{
	int c = m_ActiveScenes.Count();
	for ( int i = 0; i < c; i++ )
	{
		CSceneEntity *pScene = m_ActiveScenes[ i ].Get();
		if ( !pScene )
		{
			continue;
		}
		
		// If only stopping instanced scenes, then skip it if it can't cast to an instanced scene
		if ( bInstancedOnly && 
			( dynamic_cast< CInstancedSceneEntity * >( pScene ) == NULL ) )
		{
			continue;
		}

		if ( bNonIdleOnly && !pScene->ShouldBreakOnNonIdle() )
			continue;

		if ( pScene->InvolvesActor( pActor ) )
		{
			if ( pszThisSceneOnly && pszThisSceneOnly[0] )
			{
				if ( Q_strcmp( pszThisSceneOnly, STRING(pScene->m_iszSceneFile) ) )
					continue;
			}

			LocalScene_Printf( "%s : removed for '%s'\n", STRING( pScene->m_iszSceneFile ), pActor ? pActor->GetDebugName() : "NULL" );
			pScene->CancelPlayback();
		}

	}
}

//-----------------------------------------------------------------------------
// Purpose: Pause scenes involving the specified actor
//-----------------------------------------------------------------------------
void CSceneManager::PauseActorsScenes( CBaseFlex *pActor, bool bInstancedOnly  )
{
	int c = m_ActiveScenes.Count();
	for ( int i = 0; i < c; i++ )
	{
		CSceneEntity *pScene = m_ActiveScenes[ i ].Get();
		if ( !pScene )
		{
			continue;
		}

		// If only stopping instanced scenes, then skip it if it can't cast to an instanced scene
		if ( bInstancedOnly && 
			( dynamic_cast< CInstancedSceneEntity * >( pScene ) == NULL ) )
		{
			continue;
		}

		if ( pScene->InvolvesActor( pActor ) && pScene->IsPlayingBack() )
		{
			LocalScene_Printf( "Pausing actor %s scripted scene: %s\n", pActor->GetDebugName(), STRING(pScene->m_iszSceneFile) );

			variant_t emptyVariant;
			pScene->AcceptInput( "Pause", pScene, pScene, emptyVariant, 0 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this Actor is only in scenes that are interruptable right now
//-----------------------------------------------------------------------------
bool CSceneManager::IsInInterruptableScenes( CBaseFlex *pActor )
{
	int c = m_ActiveScenes.Count();
	for ( int i = 0; i < c; i++ )
	{
		CSceneEntity *pScene = m_ActiveScenes[ i ].Get();
		if ( !pScene )
			continue;

		//Ignore background scenes since they're harmless.
		if ( pScene->IsBackground() == true )
			continue;

		if ( pScene->InvolvesActor( pActor ) && pScene->IsPlayingBack() )
		{
			if ( pScene->IsInterruptable() == false )
				return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Resume any paused scenes involving the specified actor
//-----------------------------------------------------------------------------
void CSceneManager::ResumeActorsScenes( CBaseFlex *pActor, bool bInstancedOnly  )
{
	int c = m_ActiveScenes.Count();
	for ( int i = 0; i < c; i++ )
	{
		CSceneEntity *pScene = m_ActiveScenes[ i ].Get();
		if ( !pScene )
		{
			continue;
		}

		// If only stopping instanced scenes, then skip it if it can't cast to an instanced scene
		if ( bInstancedOnly && 
			( dynamic_cast< CInstancedSceneEntity * >( pScene ) == NULL ) )
		{
			continue;
		}

		if ( pScene->InvolvesActor( pActor ) && pScene->IsPlayingBack() )
		{
			LocalScene_Printf( "Resuming actor %s scripted scene: %s\n", pActor->GetDebugName(), STRING(pScene->m_iszSceneFile) );

			variant_t emptyVariant;
			pScene->AcceptInput( "Resume", pScene, pScene, emptyVariant, 0 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set all paused, in-playback scenes to resume when the actor is ready
//-----------------------------------------------------------------------------
void CSceneManager::QueueActorsScenesToResume( CBaseFlex *pActor, bool bInstancedOnly  )
{
	int c = m_ActiveScenes.Count();
	for ( int i = 0; i < c; i++ )
	{
		CSceneEntity *pScene = m_ActiveScenes[ i ].Get();
		if ( !pScene )
		{
			continue;
		}

		// If only stopping instanced scenes, then skip it if it can't cast to an instanced scene
		if ( bInstancedOnly && 
			( dynamic_cast< CInstancedSceneEntity * >( pScene ) == NULL ) )
		{
			continue;
		}

		if ( pScene->InvolvesActor( pActor ) && pScene->IsPlayingBack() && pScene->IsPaused() )
		{
			pScene->QueueResumePlayback();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns if there are scenes involving the specified actor
//-----------------------------------------------------------------------------
bool CSceneManager::IsRunningScriptedScene( CBaseFlex *pActor, bool bIgnoreInstancedScenes )
{
	int c = m_ActiveScenes.Count();
	for ( int i = 0; i < c; i++ )
	{
		CSceneEntity *pScene = m_ActiveScenes[ i ].Get();
		if ( !pScene ||
			 !pScene->IsPlayingBack() ||
			 ( bIgnoreInstancedScenes && dynamic_cast<CInstancedSceneEntity *>(pScene) != NULL )
			)
		{
			continue;
		}
		
		if ( pScene->InvolvesActor( pActor ) )
		{
			return true;
		}
	}
	return false;
}

bool CSceneManager::IsRunningScriptedSceneAndNotPaused( CBaseFlex *pActor, bool bIgnoreInstancedScenes )
{
	int c = m_ActiveScenes.Count();
	for ( int i = 0; i < c; i++ )
	{
		CSceneEntity *pScene = m_ActiveScenes[ i ].Get();
		if ( !pScene ||
			 !pScene->IsPlayingBack() ||
			 pScene->IsPaused() ||
			 ( bIgnoreInstancedScenes && dynamic_cast<CInstancedSceneEntity *>(pScene) != NULL )
			)
		{
			continue;
		}
		
		if ( pScene->InvolvesActor( pActor ) )
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActor - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSceneManager::IsRunningScriptedSceneWithSpeech( CBaseFlex *pActor, bool bIgnoreInstancedScenes )
{
	int c = m_ActiveScenes.Count();
	for ( int i = 0; i < c; i++ )
	{
		CSceneEntity *pScene = m_ActiveScenes[ i ].Get();
		if ( !pScene ||
			 !pScene->IsPlayingBack() ||
			 ( bIgnoreInstancedScenes && dynamic_cast<CInstancedSceneEntity *>(pScene) != NULL )
			)
		{
			continue;
		}
		
		if ( pScene->InvolvesActor( pActor ) )
		{
			if ( pScene->HasUnplayedSpeech() )
				return true;
		}
	}
	return false;
}


bool CSceneManager::IsRunningScriptedSceneWithSpeechAndNotPaused( CBaseFlex *pActor, bool bIgnoreInstancedScenes )
{
	int c = m_ActiveScenes.Count();
	for ( int i = 0; i < c; i++ )
	{
		CSceneEntity *pScene = m_ActiveScenes[ i ].Get();
		if ( !pScene ||
			 !pScene->IsPlayingBack() ||
			 pScene->IsPaused() ||
			 ( bIgnoreInstancedScenes && dynamic_cast<CInstancedSceneEntity *>(pScene) != NULL )
			)
		{
			continue;
		}
		
		if ( pScene->InvolvesActor( pActor ) )
		{
			if ( pScene->HasUnplayedSpeech() )
				return true;
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*soundname - 
//			soundlevel - 
//			soundtime - 
//-----------------------------------------------------------------------------
void CSceneManager::QueueRestoredSound( CBaseFlex *actor, char const *soundname, soundlevel_t soundlevel, float time_in_past )
{
	CRestoreSceneSound e;
	e.actor = actor;
	Q_strncpy( e.soundname, soundname, sizeof( e.soundname ) );
	e.soundlevel = soundlevel;
	e.time_in_past = time_in_past;

	m_QueuedSceneSounds.AddToTail( e );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void RemoveActorFromScriptedScenes( CBaseFlex *pActor, bool instancedscenesonly, bool nonidlescenesonly, const char *pszThisSceneOnly )
{
	GetSceneManager()->RemoveActorFromScenes( pActor, instancedscenesonly, nonidlescenesonly, pszThisSceneOnly );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void RemoveAllScenesInvolvingActor( CBaseFlex *pActor )
{
	GetSceneManager()->RemoveScenesInvolvingActor( pActor );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void PauseActorsScriptedScenes( CBaseFlex *pActor, bool instancedscenesonly )
{
	GetSceneManager()->PauseActorsScenes( pActor, instancedscenesonly );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool IsInInterruptableScenes( CBaseFlex *pActor )
{
	return GetSceneManager()->IsInInterruptableScenes( pActor );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void ResumeActorsScriptedScenes( CBaseFlex *pActor, bool instancedscenesonly )
{
	GetSceneManager()->ResumeActorsScenes( pActor, instancedscenesonly );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void QueueActorsScriptedScenesToResume( CBaseFlex *pActor, bool instancedscenesonly )
{
	GetSceneManager()->QueueActorsScenesToResume( pActor, instancedscenesonly );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool IsRunningScriptedScene( CBaseFlex *pActor, bool bIgnoreInstancedScenes )
{
	return GetSceneManager()->IsRunningScriptedScene( pActor, bIgnoreInstancedScenes );
}

bool IsRunningScriptedSceneAndNotPaused( CBaseFlex *pActor, bool bIgnoreInstancedScenes )
{
	return GetSceneManager()->IsRunningScriptedSceneAndNotPaused( pActor, bIgnoreInstancedScenes );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool IsRunningScriptedSceneWithSpeech( CBaseFlex *pActor, bool bIgnoreInstancedScenes )
{
	return GetSceneManager()->IsRunningScriptedSceneWithSpeech( pActor, bIgnoreInstancedScenes );
}

bool IsRunningScriptedSceneWithSpeechAndNotPaused( CBaseFlex *pActor, bool bIgnoreInstancedScenes )
{
	return GetSceneManager()->IsRunningScriptedSceneWithSpeechAndNotPaused( pActor, bIgnoreInstancedScenes );
}


//===========================================================================================================
// SCENE LIST MANAGER
//===========================================================================================================
LINK_ENTITY_TO_CLASS( logic_scene_list_manager, CSceneListManager );

BEGIN_DATADESC( CSceneListManager )
	DEFINE_UTLVECTOR( m_hListManagers, FIELD_EHANDLE ),

	// Keys
	DEFINE_KEYFIELD( m_iszScenes[0], FIELD_STRING, "scene0" ),
	DEFINE_KEYFIELD( m_iszScenes[1], FIELD_STRING, "scene1" ),
	DEFINE_KEYFIELD( m_iszScenes[2], FIELD_STRING, "scene2" ),
	DEFINE_KEYFIELD( m_iszScenes[3], FIELD_STRING, "scene3" ),
	DEFINE_KEYFIELD( m_iszScenes[4], FIELD_STRING, "scene4" ),
	DEFINE_KEYFIELD( m_iszScenes[5], FIELD_STRING, "scene5" ),
	DEFINE_KEYFIELD( m_iszScenes[6], FIELD_STRING, "scene6" ),
	DEFINE_KEYFIELD( m_iszScenes[7], FIELD_STRING, "scene7" ),
	DEFINE_KEYFIELD( m_iszScenes[8], FIELD_STRING, "scene8" ),
	DEFINE_KEYFIELD( m_iszScenes[9], FIELD_STRING, "scene9" ),
	DEFINE_KEYFIELD( m_iszScenes[10], FIELD_STRING, "scene10" ),
	DEFINE_KEYFIELD( m_iszScenes[11], FIELD_STRING, "scene11" ),
	DEFINE_KEYFIELD( m_iszScenes[12], FIELD_STRING, "scene12" ),
	DEFINE_KEYFIELD( m_iszScenes[13], FIELD_STRING, "scene13" ),
	DEFINE_KEYFIELD( m_iszScenes[14], FIELD_STRING, "scene14" ),
	DEFINE_KEYFIELD( m_iszScenes[15], FIELD_STRING, "scene15" ),

	DEFINE_FIELD( m_hScenes[0], FIELD_EHANDLE ),
	DEFINE_FIELD( m_hScenes[1], FIELD_EHANDLE ),
	DEFINE_FIELD( m_hScenes[2], FIELD_EHANDLE ),
	DEFINE_FIELD( m_hScenes[3], FIELD_EHANDLE ),
	DEFINE_FIELD( m_hScenes[4], FIELD_EHANDLE ),
	DEFINE_FIELD( m_hScenes[5], FIELD_EHANDLE ),
	DEFINE_FIELD( m_hScenes[6], FIELD_EHANDLE ),
	DEFINE_FIELD( m_hScenes[7], FIELD_EHANDLE ),
	DEFINE_FIELD( m_hScenes[8], FIELD_EHANDLE ),
	DEFINE_FIELD( m_hScenes[9], FIELD_EHANDLE ),
	DEFINE_FIELD( m_hScenes[10], FIELD_EHANDLE ),
	DEFINE_FIELD( m_hScenes[11], FIELD_EHANDLE ),
	DEFINE_FIELD( m_hScenes[12], FIELD_EHANDLE ),
	DEFINE_FIELD( m_hScenes[13], FIELD_EHANDLE ),
	DEFINE_FIELD( m_hScenes[14], FIELD_EHANDLE ),
	DEFINE_FIELD( m_hScenes[15], FIELD_EHANDLE ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Shutdown", InputShutdown ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneListManager::Activate( void )
{
	BaseClass::Activate();

	// Hook up scenes, but not after loading a game because they're saved.
	if ( gpGlobals->eLoadType != MapLoad_LoadGame )
	{
		for ( int i = 0; i < SCENE_LIST_MANAGER_MAX_SCENES; i++ )
		{
			if ( m_iszScenes[i] != NULL_STRING )
			{
				m_hScenes[i] = gEntList.FindEntityByName( NULL, STRING(m_iszScenes[i]) );
				if ( m_hScenes[i] )
				{
					CSceneEntity *pScene = dynamic_cast<CSceneEntity*>(m_hScenes[i].Get());
					if ( pScene )
					{
						pScene->AddListManager( this );
					}
					else 
					{
						CSceneListManager *pList = dynamic_cast<CSceneListManager*>(m_hScenes[i].Get());
						if ( pList )
						{
							pList->AddListManager( this );
						}
						else
						{
							Warning( "%s(%s) found an entity that wasn't a logic_choreographed_scene or logic_scene_list_manager in slot %d, named %s\n", GetDebugName(), GetClassname(), i, STRING(m_iszScenes[i]) );
							m_hScenes[i] = NULL;
						}
					}
				}
				else
				{
					Warning( "%s(%s) could not find scene %d, named %s\n", GetDebugName(), GetClassname(), i, STRING(m_iszScenes[i]) );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: A scene or manager in our list has started playing.
//			Remove all scenes earlier in the list. 
//-----------------------------------------------------------------------------
void CSceneListManager::SceneStarted( CBaseEntity *pSceneOrManager )
{
	// Move backwards and call remove on all scenes / managers earlier in the list to the fired one
 	bool bFoundStart = false;
	for ( int i = SCENE_LIST_MANAGER_MAX_SCENES-1; i >= 0; i-- )
	{
		if ( !m_hScenes[i] )
			continue;

		if ( bFoundStart )
		{
			RemoveScene( i );
		}
		else if ( m_hScenes[i] == pSceneOrManager )
		{
			bFoundStart = true;
		}
	}

	// Tell any managers we're within that we've started a scene
	if ( bFoundStart )
	{
		int c = m_hListManagers.Count();
		for ( int i = 0; i < c; i++ )
		{
			if ( m_hListManagers[i] )
			{
				m_hListManagers[i]->SceneStarted( this );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneListManager::AddListManager( CSceneListManager *pManager )
{
	CHandle< CSceneListManager > h;
	h = pManager;
	// Only add it once
	if ( m_hListManagers.Find( h ) == m_hListManagers.InvalidIndex() )
	{
		m_hListManagers.AddToTail( h );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Shut down all scenes, and then remove this entity
//-----------------------------------------------------------------------------
void CSceneListManager::InputShutdown( inputdata_t &inputdata )
{
	ShutdownList();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneListManager::ShutdownList( void )
{
	for ( int i = 0; i < SCENE_LIST_MANAGER_MAX_SCENES; i++ )
	{
		if ( m_hScenes[i] )
		{
			RemoveScene(i);
		}
	}

	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSceneListManager::RemoveScene( int iIndex )
{
	CSceneEntity *pScene = dynamic_cast<CSceneEntity*>(m_hScenes[iIndex].Get());
	if ( pScene )
	{
		// Remove the scene
		UTIL_Remove( pScene );
		return;
	}

	// Tell the list manager to shut down all scenes
	CSceneListManager *pList = dynamic_cast<CSceneListManager*>(m_hScenes[iIndex].Get());
	if ( pList )
	{
		pList->ShutdownList();
	}
}

void ReloadSceneFromDisk( CBaseEntity *ent )
{
	CSceneEntity *scene = dynamic_cast< CSceneEntity * >( ent );
	if ( !scene )
		return;

	Assert( 0 );
}

// Purpose: 
// Input  : *ent - 
// Output : char const
//-----------------------------------------------------------------------------
char const *GetSceneFilename( CBaseEntity *ent )
{
	CSceneEntity *scene = dynamic_cast< CSceneEntity * >( ent );
	if ( !scene )
		return "";

	return STRING( scene->m_iszSceneFile );
}

//-----------------------------------------------------------------------------
// Purpose: Return a list of the last 5 lines of speech from NPCs for bug reports
// Input  :
// Output : speech - last 5 sound files played as speech
//			returns the number of sounds in the returned list
//-----------------------------------------------------------------------------

int GetRecentNPCSpeech( recentNPCSpeech_t speech[ SPEECH_LIST_MAX_SOUNDS ] )
{
	int i;
	int num;
	int index;

	// clear out the output list
	for( i = 0; i < SPEECH_LIST_MAX_SOUNDS; i++ )
	{
		speech[ i ].time = 0.0f;
		speech[ i ].name[ 0 ] = 0;
		speech[ i ].sceneName[ 0 ] = 0;
	}

	// copy the sound names into the list in order they were played
	num = 0;
	index = speechListIndex;
	for( i = 0; i < SPEECH_LIST_MAX_SOUNDS; i++ ) 
	{
		if ( speechListSounds[ index ].name[ 0 ] ) 
		{
			// only copy names that are not zero length
			speech[ num ] = speechListSounds[ index ];
			num++;
		}

		index++;
		if ( index >= SPEECH_LIST_MAX_SOUNDS ) 
		{
			index = 0;
		}
	}

	return num;
}

//-----------------------------------------------------------------------------
// Purpose: Displays a list of the last 5 lines of speech from NPCs
// Input  :
// Output : 
//-----------------------------------------------------------------------------

static void ListRecentNPCSpeech( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	recentNPCSpeech_t speech[ SPEECH_LIST_MAX_SOUNDS ];
	int  num;
	int  i;

	// get any sounds that were spoken by NPCs recently
	num = GetRecentNPCSpeech( speech );
	Msg( "Recent NPC speech:\n" );
	for( i = 0; i < num; i++ )
	{
		Msg( "   time: %6.3f   sound name: %s   scene: %s\n", speech[ i ].time, speech[ i ].name, speech[ i ].sceneName );
	}
	Msg( "Current time: %6.3f\n", gpGlobals->curtime );
}

static ConCommand ListRecentNPCSpeechCmd( "listRecentNPCSpeech", ListRecentNPCSpeech, "Displays a list of the last 5 lines of speech from NPCs.", FCVAR_DONTRECORD|FCVAR_GAMEDLL );

CON_COMMAND( scene_flush, "Flush all .vcds from the cache and reload from disk." )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	Msg( "Reloading\n" );
	scenefilecache->Reload();
	Msg( "   done\n" );
}
