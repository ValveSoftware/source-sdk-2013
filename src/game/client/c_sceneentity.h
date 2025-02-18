//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_SCENEENTITY_H
#define C_SCENEENTITY_H
#ifdef _WIN32
#pragma once
#endif

#include "ichoreoeventcallback.h"
#include "choreoscene.h"

class C_SceneEntity : public C_BaseEntity, public IChoreoEventCallback
{
	friend class CChoreoEventCallback;

public:
	DECLARE_CLASS( C_SceneEntity, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_SceneEntity( void );
	~C_SceneEntity( void );

	// From IChoreoEventCallback
	virtual void			StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void			EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void			ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual bool			CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );


	virtual void PostDataUpdate( DataUpdateType_t updateType );
	virtual void PreDataUpdate( DataUpdateType_t updateType );

	virtual void StopClientOnlyScene();
	virtual void SetupClientOnlyScene( const char *pszFilename, C_BaseFlex *pOwner = NULL , bool bMultiplayer = false );

	virtual void ClientThink();

	void					OnResetClientTime();

	CHandle< C_BaseFlex >	GetActor( int i ){ return ( i < m_hActorList.Count() ) ? m_hActorList[i] : NULL; }

	virtual	void			DispatchStartSpeak( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event, soundlevel_t iSoundlevel );
	virtual void			DispatchEndSpeak( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event );

	bool IsClientOnly( void ){ return m_bClientOnly; }

private:

	void					ResetActorFlexesForScene();

	// Scene load/unload
	CChoreoScene			*LoadScene( const char *filename );
	void					LoadSceneFromFile( const char *filename );
	void					UnloadScene( void );
	void					PrefetchAnimBlocks( CChoreoScene *pScene );

	C_BaseFlex				*FindNamedActor( CChoreoActor *pChoreoActor );

	virtual void			DispatchStartFlexAnimation( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchEndFlexAnimation( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchStartExpression( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchEndExpression( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchStartGesture( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchProcessGesture( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchEndGesture( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchStartSequence( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchProcessSequence( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event );
	virtual void			DispatchEndSequence( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event );
	void					DispatchProcessLoop( CChoreoScene *scene, CChoreoEvent *event );

	char const				*GetSceneFileName();

	void					DoThink( float frametime );

	void					ClearSceneEvents( CChoreoScene *scene, bool canceled );
	void					SetCurrentTime( float t, bool forceClientSync );


	template <size_t maxLenInChars>
	bool					GetHWMorphSceneFileName( const char *pFilename, OUT_Z_ARRAY char (&hwmFilename)[maxLenInChars] ) const;

private:

	void					CheckQueuedEvents();
	void					WipeQueuedEvents();
	void					QueueStartEvent( float starttime, CChoreoScene *scene, CChoreoEvent *event );

	bool		m_bIsPlayingBack;
	bool		m_bPaused;
	bool		m_bMultiplayer;
	float		m_flCurrentTime;
	float		m_flForceClientTime;
	int			m_nSceneStringIndex;
	bool		m_bClientOnly;

	CHandle< C_BaseFlex >	m_hOwner; // if set, this overrides the m_hActorList in FindNamedActor()

	CUtlVector< CHandle< C_BaseFlex > > m_hActorList;		

private:
	bool		m_bWasPlaying;

	CChoreoScene *m_pScene;

	struct QueuedEvents_t
	{
		float			starttime;
		CChoreoScene	*scene;
		CChoreoEvent	*event;
	};

	CUtlVector< QueuedEvents_t > m_QueuedEvents;
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template <size_t maxLenInChars>
bool C_SceneEntity::GetHWMorphSceneFileName( const char *pFilename, OUT_Z_ARRAY char( &hwmFilename )[ maxLenInChars ] ) const
{
	extern bool UseHWMorphVCDs();

	// Make sure it's all zeros.
	V_memset( hwmFilename, 0, sizeof( hwmFilename ) );

	// Are we even using hardware morph?
	if ( !UseHWMorphVCDs() )
		return false;

	// Multi-player only!
	if ( !m_bMultiplayer )
		return false;

	// Do we have a valid filename?
	if ( !( pFilename && pFilename[ 0 ] ) )
		return false;

	// Check to see if we already have an player/hwm/* filename.
	if ( ( V_strstr( pFilename, "/high" ) != NULL ) || ( V_strstr( pFilename, "\\high" ) != NULL ) )
	{
		V_strcpy_safe( hwmFilename, pFilename );
		return true;
	}

	// Find the hardware morph scene name and pass that along as well.
	char szScene[ MAX_PATH ];
	V_strcpy_safe( szScene, pFilename );

	char szSceneHWM[ MAX_PATH ];
	szSceneHWM[ 0 ] = '\0';

	char *pszToken = strtok( szScene, "/\\" );
	while ( pszToken != NULL )
	{
		if ( !V_stricmp( pszToken, "low" ) )
		{
			V_strcat_safe( szSceneHWM, "high", sizeof( szSceneHWM ) );
		}
		else
		{
			V_strcat_safe( szSceneHWM, pszToken, sizeof( szSceneHWM ) );
		}

		pszToken = strtok( NULL, "/\\" );
		if ( pszToken != NULL )
		{
			V_strcat_safe( szSceneHWM, "\\", sizeof( szSceneHWM ) );
		}
	}

	V_strcpy_safe( hwmFilename, szSceneHWM );
	return true;
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

	bool GetString( short stringId, char *buff, int buffSize );
};

#endif // C_SCENEENTITY_H
