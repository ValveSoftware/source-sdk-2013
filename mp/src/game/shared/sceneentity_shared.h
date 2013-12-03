//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef SCENEENTITY_SHARED_H
#define SCENEENTITY_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#if defined( CLIENT_DLL )
#define CBaseFlex C_BaseFlex
#define CSceneEntity C_SceneEntity
#endif

#include "iscenetokenprocessor.h"

class CBaseFlex;

class CChoreoEvent;
class CChoreoScene;
class CChoreoActor;
class CSceneEntity;

//-----------------------------------------------------------------------------
// Purpose: One of a number of currently playing scene events for this actor
//-----------------------------------------------------------------------------
// FIXME: move this, it's only used in in baseflex and baseactor
class CSceneEventInfo
{
public:
	CSceneEventInfo()
		:
	m_pEvent( 0 ),
	m_pScene( 0 ),
	m_pActor( 0 ),
	m_bStarted( false ),
	m_iLayer( -1 ),
	m_iPriority( 0 ),
	m_nSequence( 0 ),
	m_bIsGesture( false ),
	m_flWeight( 0.0f ),
	m_hTarget(),
	m_bIsMoving( false ),
	m_bHasArrived( false ),
	m_flInitialYaw( 0.0f ),
	m_flTargetYaw( 0.0f ),
	m_flFacingYaw( 0.0f ),
	m_nType( 0 ),
	m_flNext( 0.0f ),
	m_bClientSide( false )
	{
	}

	// The event handle of the current scene event
	CChoreoEvent	*m_pEvent;

	// Current Scene
	CChoreoScene	*m_pScene;

	// Current actor
	CChoreoActor	*m_pActor;

	// Set after the first time the event has been configured ( allows
	//  bumping markov index only at start of event playback, not every frame )
	bool			m_bStarted;

public:
	//	EVENT local data...
	// FIXME: Evil, make accessors or figure out better place
	// FIXME: This won't work, scenes don't save and restore...
	int						m_iLayer;
	int						m_iPriority;
	int						m_nSequence;
	bool					m_bIsGesture;
	float					m_flWeight; // used for suppressions of posture while moving

	// movement, faceto targets?
	EHANDLE					m_hTarget;
	bool					m_bIsMoving;
	bool					m_bHasArrived;
	float					m_flInitialYaw;
	float					m_flTargetYaw;
	float					m_flFacingYaw;

	// generic AI events
	int						m_nType;
	float					m_flNext;

	// is this event only client side?
	bool					m_bClientSide; 

	void					InitWeight( CBaseFlex *pActor );
	float					UpdateWeight( CBaseFlex *pActor );
};

//-----------------------------------------------------------------------------
// Purpose: Helper for parsing scene data file
//-----------------------------------------------------------------------------
class CSceneTokenProcessor : public ISceneTokenProcessor
{
public:
	const char	*CurrentToken( void );
	bool		GetToken( bool crossline );
	bool		TokenAvailable( void );
	void		Error( PRINTF_FORMAT_STRING const char *fmt, ... );
	void		SetBuffer( char *buffer );
private:
	const char	*m_pBuffer;
	char		m_szToken[ 1024 ];
};

extern CSceneTokenProcessor g_TokenProcessor;

void Scene_Printf( PRINTF_FORMAT_STRING const char *pFormat, ... );
extern ConVar scene_clientflex;

#endif // SCENEENTITY_SHARED_H
