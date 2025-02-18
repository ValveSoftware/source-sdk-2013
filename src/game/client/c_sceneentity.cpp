//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "networkstringtable_clientdll.h"
#include "dt_utlvector_recv.h"
#include "choreoevent.h"
#include "choreoactor.h"
#include "choreochannel.h"
#include "filesystem.h"
#include "ichoreoeventcallback.h"
#include "scenefilecache/ISceneFileCache.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "tier2/tier2.h"
#include "hud_closecaption.h"
#include "tier0/icommandline.h"

#include "c_sceneentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Decodes animtime and notes when it changes
// Input  : *pStruct - ( C_BaseEntity * ) used to flag animtime is changine
//			*pVarData - 
//			*pIn - 
//			objectID - 
//-----------------------------------------------------------------------------
void RecvProxy_ForcedClientTime( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_SceneEntity *pScene = reinterpret_cast< C_SceneEntity * >( pStruct );
	*(float *)pOut = pData->m_Value.m_Float;
	pScene->OnResetClientTime();
}

#if defined( CSceneEntity )
#undef CSceneEntity
#endif

IMPLEMENT_CLIENTCLASS_DT(C_SceneEntity, DT_SceneEntity, CSceneEntity)
	RecvPropInt(RECVINFO(m_nSceneStringIndex)),
	RecvPropBool(RECVINFO(m_bIsPlayingBack)),
	RecvPropBool(RECVINFO(m_bPaused)),
	RecvPropBool(RECVINFO(m_bMultiplayer)),
	RecvPropFloat(RECVINFO(m_flForceClientTime), 0, RecvProxy_ForcedClientTime ),
	RecvPropUtlVector( 
		RECVINFO_UTLVECTOR( m_hActorList ), 
		MAX_ACTORS_IN_SCENE,
		RecvPropEHandle(NULL, 0, 0)),
END_RECV_TABLE()

C_SceneEntity::C_SceneEntity( void )
{
	m_pScene = NULL;
	m_bMultiplayer = false;

	m_hOwner = NULL;
	m_bClientOnly = false;
}

C_SceneEntity::~C_SceneEntity( void )
{
	UnloadScene();
}

void C_SceneEntity::OnResetClientTime()
{
	// In TF2 we ignore this as the scene is played entirely client-side.
#ifndef TF_CLIENT_DLL
	m_flCurrentTime = m_flForceClientTime;
#endif
}

char const *C_SceneEntity::GetSceneFileName()
{
	return g_pStringTableClientSideChoreoScenes->GetString( m_nSceneStringIndex );
}

ConVar mp_usehwmvcds( "mp_usehwmvcds", "0", NULL, "Enable the use of the hw morph vcd(s). (-1 = never, 1 = always, 0 = based upon GPU)" ); // -1 = never, 0 = if hasfastvertextextures, 1 = always
bool UseHWMorphVCDs()
{
// 	if ( mp_usehwmvcds.GetInt() == 0 )
// 		return g_pMaterialSystemHardwareConfig->HasFastVertexTextures();
// 	return mp_usehwmvcds.GetInt() > 0;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_SceneEntity::ResetActorFlexesForScene()
{
	int nActorCount = m_pScene->GetNumActors();
	for( int iActor = 0; iActor < nActorCount; ++iActor )
	{
		CChoreoActor *pChoreoActor = m_pScene->GetActor( iActor );
		if ( !pChoreoActor )
			continue;

		C_BaseFlex *pFlexActor = FindNamedActor( pChoreoActor );
		if ( !pFlexActor )
			continue;

		CStudioHdr *pStudioHdr = pFlexActor->GetModelPtr();
		if ( !pStudioHdr )
			continue;

		if ( pStudioHdr->numflexdesc() == 0 )
			continue;

		// Reset the flex weights to their starting position.
		LocalFlexController_t iController;
		for ( iController = LocalFlexController_t(0); iController < pStudioHdr->numflexcontrollers(); ++iController )
		{
			pFlexActor->SetFlexWeight( iController, 0.0f );
		}

		// Reset the prediction interpolation values.
		pFlexActor->m_iv_flexWeight.Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_SceneEntity::StopClientOnlyScene()
{
	if ( m_pScene )
	{
		m_pScene->ResetSimulation();

		if ( m_hOwner.Get() )
		{
			m_hOwner->RemoveChoreoScene( m_pScene );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_SceneEntity::SetupClientOnlyScene( const char *pszFilename, C_BaseFlex *pOwner /* = NULL */, bool bMultiplayer /* = false */ )
{
	m_bIsPlayingBack = true;
	m_bMultiplayer = bMultiplayer;
	m_hOwner = pOwner;
	m_bClientOnly = true;

	char szFilename[MAX_PATH];
	Assert( V_strlen( pszFilename ) < MAX_PATH );
	V_strcpy_safe( szFilename, pszFilename );

	char szSceneHWM[ MAX_PATH ];
	if ( GetHWMorphSceneFileName( szFilename, szSceneHWM ) )
	{
		V_strcpy_safe( szFilename, szSceneHWM );
	}

	Assert(  szFilename[ 0 ] );
	if ( szFilename[ 0 ] )
	{
		LoadSceneFromFile( szFilename );

		if ( !HushAsserts() )
		{
			Assert( m_pScene );
		}

		// Should handle gestures and sequences client side.
		if ( m_bMultiplayer )
		{
			if ( m_pScene )
			{
				int types[6];
				types[0] = CChoreoEvent::FLEXANIMATION;
				types[1] = CChoreoEvent::EXPRESSION;
				types[2] = CChoreoEvent::GESTURE;
				types[3] = CChoreoEvent::SEQUENCE;
				types[4] = CChoreoEvent::SPEAK;
				types[5] = CChoreoEvent::LOOP;
				m_pScene->RemoveEventsExceptTypes( types, 6 );
			}

			PrefetchAnimBlocks( m_pScene );
		}
		else
		{
			if ( m_pScene )
			{
				int types[ 2 ];
				types[ 0 ] =  CChoreoEvent::FLEXANIMATION;
				types[ 1 ] =  CChoreoEvent::EXPRESSION;
				m_pScene->RemoveEventsExceptTypes( types, 2 );
			}
		}

		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	if ( m_hOwner.Get() )
	{
		if ( !HushAsserts() )
		{
			Assert( m_pScene );
		}

		if ( m_pScene )
		{
			ClearSceneEvents( m_pScene, false );

			if ( m_bIsPlayingBack )
			{
				m_pScene->ResetSimulation();
				m_hOwner->StartChoreoScene( m_pScene );
			}
			else
			{
				m_pScene->ResetSimulation();
				m_hOwner->RemoveChoreoScene( m_pScene );
			}

			// Reset the flex weights when we start a new scene.  This is normally done on the player model, but since
			// we don't have a player here yet - we need to do this!
			ResetActorFlexesForScene();
		}
	}
	else
	{
		for( int i = 0; i < m_hActorList.Count() ; ++i )
		{
			C_BaseFlex *actor = m_hActorList[ i ].Get();
			if ( !actor )
				continue;

			Assert( m_pScene );

			if ( m_pScene )
			{
				ClearSceneEvents( m_pScene, false );

				if ( m_bIsPlayingBack )
				{
					m_pScene->ResetSimulation();
					actor->StartChoreoScene( m_pScene );
				}
				else
				{
					m_pScene->ResetSimulation();
					actor->RemoveChoreoScene( m_pScene );
				}
			}
		}
	}
}

void C_SceneEntity::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	char const *str = GetSceneFileName();
	char szFilename[MAX_PATH];
	if ( str )
	{
		Assert( V_strlen( str ) < MAX_PATH );
		V_strcpy_safe( szFilename, str );
	}
	else
	{
		szFilename[0] = '\0';
	}

	char szSceneHWM[MAX_PATH];
	if ( GetHWMorphSceneFileName( szFilename, szSceneHWM ) )
	{
		V_strcpy_safe( szFilename, szSceneHWM );
	}

	if ( updateType == DATA_UPDATE_CREATED )
	{
		Assert( szFilename[ 0 ] );
		if ( szFilename[ 0 ] )
		{
			LoadSceneFromFile( szFilename );

			// Kill everything except flex events
			Assert( m_pScene );

			// Should handle gestures and sequences clientside.
			if ( m_bMultiplayer )
			{
				if ( m_pScene )
				{
					int types[6];
					types[0] = CChoreoEvent::FLEXANIMATION;
					types[1] = CChoreoEvent::EXPRESSION;
					types[2] = CChoreoEvent::GESTURE;
					types[3] = CChoreoEvent::SEQUENCE;				
					types[4] = CChoreoEvent::SPEAK;
					types[5] = CChoreoEvent::LOOP;
					m_pScene->RemoveEventsExceptTypes( types, 6 );
				}

				PrefetchAnimBlocks( m_pScene );
			}
			else
			{
				if ( m_pScene )
				{
					int types[ 2 ];
					types[ 0 ] =  CChoreoEvent::FLEXANIMATION;
					types[ 1 ] =  CChoreoEvent::EXPRESSION;
					m_pScene->RemoveEventsExceptTypes( types, 2 );
				}
			}

			SetNextClientThink( CLIENT_THINK_ALWAYS );
		}

		m_bWasPlaying = !m_bIsPlayingBack; // force it to be "changed"
	}

	// Playback state changed...
	if ( m_bWasPlaying != m_bIsPlayingBack )
	{
		for(int i = 0; i < m_hActorList.Count() ; ++i )
		{
			C_BaseFlex *actor = m_hActorList[ i ].Get();
			if ( !actor )
				continue;

			Assert( m_pScene );

			if ( m_pScene )
			{
				ClearSceneEvents( m_pScene, false );

				if ( m_bIsPlayingBack )
				{
					m_pScene->ResetSimulation();
					actor->StartChoreoScene( m_pScene );
				}
				else
				{
					m_pScene->ResetSimulation();
					actor->RemoveChoreoScene( m_pScene );
				}
			}
		}
	}
}

void C_SceneEntity::PreDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PreDataUpdate( updateType );

	m_bWasPlaying = m_bIsPlayingBack;
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame that an event is active (Start/EndEvent as also
//  called)
// Input  : *event - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void C_SceneEntity::ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	// For now we only need to process events if we go back in time.
	if ( currenttime < event->m_flPrevTime )
	{
		//if ( !V_strstr( scene->GetFilename(), "idleloop" ) )
		//{
		//	Msg( "ProcessEvent( %6.4f, %32s %6.4f )    %6.4f\n", currenttime, event->GetName(), event->m_flPrevTime, m_flCurrentTime );
		//}

		C_BaseFlex *pActor = NULL;
		CChoreoActor *actor = event->GetActor();
		if ( actor )
		{
			pActor = FindNamedActor( actor );
			if ( NULL == pActor )
			{
				// TODO: QueueProcessEvent
				// This can occur if we haven't been networked an actor yet... we need to queue it so that we can 
				//  fire off the process event as soon as we have the actor resident on the client.
				return;
			}
		}

		switch ( event->GetType() )
		{
		case CChoreoEvent::GESTURE:
			{
				// Verify data.
				Assert( m_bMultiplayer );
				Assert( scene != NULL );
				Assert( event != NULL );

				if ( pActor )
				{
					DispatchProcessGesture( scene, pActor, event );
				}
			}
			break;
		case CChoreoEvent::SEQUENCE:
			{
				// Verify data.
				Assert( m_bMultiplayer );
				Assert( scene != NULL );
				Assert( event != NULL );

				if ( pActor )
				{
					DispatchProcessSequence( scene, pActor, event );
				}
			}
			break;
		}
	}

	event->m_flPrevTime = currenttime;
}

//-----------------------------------------------------------------------------
// Purpose: Called for events that are part of a pause condition
// Input  : *event - 
// Output : Returns true on event completed, false on non-completion.
//-----------------------------------------------------------------------------
bool C_SceneEntity::CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	return true;
}

C_BaseFlex *C_SceneEntity::FindNamedActor( CChoreoActor *pChoreoActor )
{
	if ( !m_pScene )
		return NULL;

	if ( m_hOwner.Get() != NULL )
	{
		return m_hOwner.Get();
	}

	int idx = m_pScene->FindActorIndex( pChoreoActor );
	if ( idx < 0 || idx >= m_hActorList.Count() )
		return NULL;

	return m_hActorList[ idx ].Get();
}

//-----------------------------------------------------------------------------
// Purpose: All events are leading edge triggered
// Input  : currenttime - 
//			*event - 
//-----------------------------------------------------------------------------
void C_SceneEntity::StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event );

	if ( !Q_stricmp( event->GetName(), "NULL" ) )
 	{
 		Scene_Printf( "%s : %8.2f:  ignored %s\n", GetSceneFileName(), currenttime, event->GetDescription() );
 		return;
 	}
 

	C_BaseFlex *pActor = NULL;
	CChoreoActor *actor = event->GetActor();
	if ( actor )
	{
		pActor = FindNamedActor( actor );
		if ( NULL == pActor )
		{
			// This can occur if we haven't been networked an actor yet... we need to queue it so that we can 
			//  fire off the start event as soon as we have the actor resident on the client.
			QueueStartEvent( currenttime, scene, event );
			return;
		}
	}

	Scene_Printf( "%s : %8.2f:  start %s\n", GetSceneFileName(), currenttime, event->GetDescription() );

	switch ( event->GetType() )
	{
	case CChoreoEvent::FLEXANIMATION:
		{
			if ( pActor )
			{
				DispatchStartFlexAnimation( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::EXPRESSION:
		{
			if ( pActor )
			{
				DispatchStartExpression( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::GESTURE:
		{
			// Verify data.
			Assert( m_bMultiplayer );
			Assert( scene != NULL );
			Assert( event != NULL );

			if ( pActor )
			{
				DispatchStartGesture( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::SEQUENCE:
		{
			// Verify data.
			Assert( m_bMultiplayer );
			Assert( scene != NULL );
			Assert( event != NULL );

			if ( pActor )
			{
				DispatchStartSequence( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::LOOP:
		{
			// Verify data.
			Assert( m_bMultiplayer );
			Assert( scene != NULL );
			Assert( event != NULL );

			DispatchProcessLoop( scene, event );
		}
	case CChoreoEvent::SPEAK:
		{
			if ( IsClientOnly() && pActor )
			{
				// FIXME: dB hack.  soundlevel needs to be moved into inside of wav?
				soundlevel_t iSoundlevel = SNDLVL_TALKING;
				if ( event->GetParameters2() )
				{
					iSoundlevel = (soundlevel_t)atoi( event->GetParameters2() );
					if ( iSoundlevel == SNDLVL_NONE )
					{
						iSoundlevel = SNDLVL_TALKING;
					}
				}

				DispatchStartSpeak( scene, pActor, event, iSoundlevel );
			}
		}
		break;
	default:
		break;
	}

	event->m_flPrevTime = currenttime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//			*event - 
//-----------------------------------------------------------------------------
void C_SceneEntity::DispatchProcessLoop( CChoreoScene *scene, CChoreoEvent *event )
{
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
// Purpose: Playback sound file that contains phonemes
// Input  : *actor - 
//			*parameters - 
//-----------------------------------------------------------------------------
void C_SceneEntity::DispatchStartSpeak( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event, soundlevel_t iSoundlevel )
{
	// Emit sound
	if ( IsClientOnly() && actor )
	{
		CSingleUserRecipientFilter filter( C_BasePlayer::GetLocalPlayer() );

		float time_in_past = m_flCurrentTime - event->GetStartTime() ;
		float soundtime = gpGlobals->curtime - time_in_past;

		EmitSound_t es;
		es.m_nChannel = CHAN_VOICE;
		es.m_flVolume = 1;
		es.m_SoundLevel = iSoundlevel;
		es.m_flSoundTime = soundtime;

		// No CC since we do it manually
		// FIXME:  This will  change
		es.m_bEmitCloseCaption = false;
		es.m_pSoundName = event->GetParameters();

		EmitSound( filter, actor->entindex(), es );
		actor->AddSceneEvent( scene, event, NULL, IsClientOnly() );

		// Close captioning only on master token no matter what...
		if ( event->GetCloseCaptionType() == CChoreoEvent::CC_MASTER )
		{
			char tok[ CChoreoEvent::MAX_CCTOKEN_STRING ];
			bool validtoken = event->GetPlaybackCloseCaptionToken( tok, sizeof( tok ) );
			if ( validtoken )
			{
				CRC32_t tokenCRC;
				CRC32_Init( &tokenCRC );

				char lowercase[ 256 ];
				Q_strncpy( lowercase, tok, sizeof( lowercase ) );
				Q_strlower( lowercase );

				CRC32_ProcessBuffer( &tokenCRC, lowercase, Q_strlen( lowercase ) );
				CRC32_Final( &tokenCRC );

				float endtime = event->GetLastSlaveEndTime();
				float durationShort = event->GetDuration();
				float durationLong = endtime - event->GetStartTime();
				float duration = MAX( durationShort, durationLong );

				CHudCloseCaption *hudCloseCaption = GET_HUDELEMENT( CHudCloseCaption );
				if ( hudCloseCaption )
				{
					hudCloseCaption->ProcessCaption( lowercase, duration );
				}
			}

		}
	}
}

void C_SceneEntity::DispatchEndSpeak( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event )
{
	if ( IsClientOnly() )
	{
		actor->RemoveSceneEvent( scene, event, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : currenttime - 
//			*event - 
//-----------------------------------------------------------------------------
void C_SceneEntity::EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event );

	if ( !Q_stricmp( event->GetName(), "NULL" ) )
 	{
 		return;
 	}

	C_BaseFlex *pActor = NULL;
	CChoreoActor *actor = event->GetActor();
	if ( actor )
	{
		pActor = FindNamedActor( actor );
	}

	Scene_Printf( "%s : %8.2f:  finish %s\n", GetSceneFileName(), currenttime, event->GetDescription() );

	switch ( event->GetType() )
	{
	case CChoreoEvent::FLEXANIMATION:
		{
			if ( pActor )
			{
				DispatchEndFlexAnimation( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::EXPRESSION:
		{
			if ( pActor )
			{
				DispatchEndExpression( scene, pActor, event );
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
	case CChoreoEvent::SEQUENCE:
		{
			if ( pActor )
			{
				DispatchEndSequence( scene, pActor, event );
			}
		}
		break;
	case CChoreoEvent::SPEAK:
		{
			if ( IsClientOnly() && pActor )
			{
				DispatchEndSpeak( scene, pActor, event );
			}
		}
		break;
	default:
		break;
	}
}

bool CChoreoStringPool::GetString( short stringId, char *buff, int buffSize )
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

CChoreoStringPool g_ChoreoStringPool;

CChoreoScene *C_SceneEntity::LoadScene( const char *filename )
{
	char loadfile[ 512 ];
	Q_strncpy( loadfile, filename, sizeof( loadfile ) );
	Q_SetExtension( loadfile, ".vcd", sizeof( loadfile ) );
	Q_FixSlashes( loadfile );

	char *pBuffer = NULL;
	size_t bufsize = scenefilecache->GetSceneBufferSize( loadfile );
	if ( bufsize <= 0 )
		return NULL;

	pBuffer = new char[ bufsize ];
	if ( !scenefilecache->GetSceneData( filename, (byte *)pBuffer, bufsize ) )
	{
		delete[] pBuffer;
		return NULL;
	}

	CChoreoScene *pScene;
	if ( IsBufferBinaryVCD( pBuffer, bufsize ) )
	{
		pScene = new CChoreoScene( this );
		CUtlBuffer buf( pBuffer, bufsize, CUtlBuffer::READ_ONLY );
		if ( !pScene->RestoreFromBinaryBuffer( buf, loadfile, &g_ChoreoStringPool ) )
		{
			Warning( "Unable to restore binary scene '%s'\n", loadfile );
			delete pScene;
			pScene = NULL;
		}
		else
		{
			pScene->SetPrintFunc( Scene_Printf );
			pScene->SetEventCallbackInterface( this );
		}
	}
	else
	{
		g_TokenProcessor.SetBuffer( pBuffer );
		pScene = ChoreoLoadScene( loadfile, this, &g_TokenProcessor, Scene_Printf );
	}

	delete[] pBuffer;
	return pScene;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//-----------------------------------------------------------------------------
void C_SceneEntity::LoadSceneFromFile( const char *filename )
{
	UnloadScene();
	m_pScene = LoadScene( filename );
}

void C_SceneEntity::ClearSceneEvents( CChoreoScene *scene, bool canceled )
{
	if ( !m_pScene )
		return;

	Scene_Printf( "%s : %8.2f:  clearing events\n", GetSceneFileName(), m_flCurrentTime );

	int i;
	for ( i = 0 ; i < m_pScene->GetNumActors(); i++ )
	{
		C_BaseFlex *pActor = FindNamedActor( m_pScene->GetActor( i ) );
		if ( !pActor )
			continue;

		// Clear any existing expressions
		pActor->ClearSceneEvents( scene, canceled );
	}

	WipeQueuedEvents();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_SceneEntity::UnloadScene( void )
{
	WipeQueuedEvents();

	if ( m_pScene )
	{
		ClearSceneEvents( m_pScene, false );
		for ( int i = 0 ; i < m_pScene->GetNumActors(); i++ )
		{
			C_BaseFlex *pTestActor = FindNamedActor( m_pScene->GetActor( i ) );

			if ( !pTestActor )
				continue;
		
			pTestActor->RemoveChoreoScene( m_pScene );
		}
	}
	delete m_pScene;
	m_pScene = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*event - 
//-----------------------------------------------------------------------------
void C_SceneEntity::DispatchStartFlexAnimation( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event )
{
	actor->AddSceneEvent( scene, event, NULL, IsClientOnly() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*event - 
//-----------------------------------------------------------------------------
void C_SceneEntity::DispatchEndFlexAnimation( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event )
{
	actor->RemoveSceneEvent( scene, event, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*event - 
//-----------------------------------------------------------------------------
void C_SceneEntity::DispatchStartExpression( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event )
{
	actor->AddSceneEvent( scene, event, NULL, IsClientOnly() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*event - 
//-----------------------------------------------------------------------------
void C_SceneEntity::DispatchEndExpression( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event )
{
	actor->RemoveSceneEvent( scene, event, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*parameters - 
//-----------------------------------------------------------------------------
void C_SceneEntity::DispatchStartGesture( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event )
{
	// Ingore null gestures
	if ( !Q_stricmp( event->GetName(), "NULL" ) )
		return;

	actor->AddSceneEvent( scene, event, NULL, IsClientOnly() ); 
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*parameters - 
//-----------------------------------------------------------------------------
void C_SceneEntity::DispatchProcessGesture( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event )
{
	// Ingore null gestures
	if ( !Q_stricmp( event->GetName(), "NULL" ) )
		return;

	actor->RemoveSceneEvent( scene, event, false );
	actor->AddSceneEvent( scene, event, NULL, IsClientOnly() ); 
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//			*parameters - 
//-----------------------------------------------------------------------------
void C_SceneEntity::DispatchEndGesture( CChoreoScene *scene, C_BaseFlex *actor, CChoreoEvent *event )
{
	// Ingore null gestures
	if ( !Q_stricmp( event->GetName(), "NULL" ) )
		return;

	actor->RemoveSceneEvent( scene, event, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void C_SceneEntity::DispatchStartSequence( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->AddSceneEvent( scene, event, NULL, IsClientOnly() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void C_SceneEntity::DispatchProcessSequence( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->RemoveSceneEvent( scene, event, false );
	actor->AddSceneEvent( scene, event, NULL, IsClientOnly() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *actor - 
//-----------------------------------------------------------------------------
void C_SceneEntity::DispatchEndSequence( CChoreoScene *scene, CBaseFlex *actor, CChoreoEvent *event )
{
	actor->RemoveSceneEvent( scene, event, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_SceneEntity::DoThink( float frametime )
{
	if ( !m_pScene )
		return;

	if ( !m_bIsPlayingBack )
	{
		WipeQueuedEvents();
		return;
	}

	CheckQueuedEvents();

	if ( m_bPaused )
	{
		return;
	}

	// Msg( "CL:  %d, %f for %s\n", gpGlobals->tickcount, m_flCurrentTime, m_pScene->GetFilename() );

	// Tell scene to go
	m_pScene->Think( m_flCurrentTime );
	// Drive simulation time for scene
	m_flCurrentTime += gpGlobals->frametime;
}

void C_SceneEntity::ClientThink()
{
	DoThink( gpGlobals->frametime );
}

void C_SceneEntity::CheckQueuedEvents()
{
// Check for duplicates
	CUtlVector< QueuedEvents_t > events;
	events = m_QueuedEvents;
	m_QueuedEvents.RemoveAll();

	int c = events.Count();
	for ( int i = 0; i < c; ++i )
	{
		const QueuedEvents_t& check = events[ i ];
		
		// Retry starting this event
		StartEvent( check.starttime, check.scene, check.event );
	}
}

void C_SceneEntity::WipeQueuedEvents()
{
	m_QueuedEvents.Purge();
}

void C_SceneEntity::QueueStartEvent( float starttime, CChoreoScene *scene, CChoreoEvent *event )
{
	// Check for duplicates
	int c = m_QueuedEvents.Count();
	for ( int i = 0; i < c; ++i )
	{
		const QueuedEvents_t& check = m_QueuedEvents[ i ];
		if ( check.scene == scene && 
			 check.event == event )
			return;
	}

	QueuedEvents_t qe;
	qe.scene = scene;
	qe.event = event;
	qe.starttime = starttime;
	m_QueuedEvents.AddToTail( qe );
}

//-----------------------------------------------------------------------------
// Purpose: Resets time such that the client version of the .vcd is also updated, if appropriate
// Input  : t - 
//			forceClientSync - unused for now, we may want to reenable this at some point
//-----------------------------------------------------------------------------
void C_SceneEntity::SetCurrentTime( float t, bool forceClientSync )
{
	m_flCurrentTime = t;
	m_flForceClientTime = t;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_SceneEntity::PrefetchAnimBlocks( CChoreoScene *pScene )
{
	if ( !HushAsserts() )
	{
		Assert( pScene && m_bMultiplayer );
	}
	if ( !pScene || !m_bMultiplayer )
		return;

	// Build a fast lookup, too
	CUtlMap<CChoreoActor*,CBaseFlex*> actorMap( 0, 0, DefLessFunc( CChoreoActor* ) );

	int nSpew = 0;
	int nResident = 0;
	int nChecked = 0;

	// Iterate events and precache necessary resources
	for ( int i = 0; i < pScene->GetNumEvents(); i++ )
	{
		CChoreoEvent *pEvent = pScene->GetEvent( i );
		if ( !pEvent )
			continue;

		// load any necessary data
		switch ( pEvent->GetType() )
		{
		default:
			break;
		case CChoreoEvent::SEQUENCE:
		case CChoreoEvent::GESTURE:
			{
				CChoreoActor *pActor = pEvent->GetActor();
				if ( pActor )
				{
					CBaseFlex *pFlex = NULL;
					int idx = actorMap.Find( pActor );
					if ( idx == actorMap.InvalidIndex() )
					{
						pFlex = FindNamedActor( pActor );
						idx = actorMap.Insert( pActor, pFlex );
					}
					else
					{
						pFlex = actorMap[ idx ];
					}

					if ( pFlex )
					{
						int iSequence = pFlex->LookupSequence( pEvent->GetParameters() );
						if ( iSequence >= 0 )
						{
							CStudioHdr *pStudioHdr = pFlex->GetModelPtr();
							if ( pStudioHdr )
							{
								// Now look up the animblock
								mstudioseqdesc_t &seqdesc = pStudioHdr->pSeqdesc( iSequence );
								for ( int iGroup = 0 ; iGroup < seqdesc.groupsize[ 0 ] ; ++iGroup )
								{
									for ( int j = 0; j < seqdesc.groupsize[ 1 ]; ++j )
									{
										int iAnimation = seqdesc.anim( iGroup, j );
										int iBaseAnimation = pStudioHdr->iRelativeAnim( iSequence, iAnimation );
										mstudioanimdesc_t &animdesc = pStudioHdr->pAnimdesc( iBaseAnimation );

										++nChecked;

										if ( nSpew != 0 )
										{
											Msg( "%s checking block %d\n", pStudioHdr->pszName(), animdesc.animblock );
										}

										// Async load the animation
										int iFrame = 0;
										const mstudioanim_t *panim = animdesc.pAnim( &iFrame );
										if ( panim )
										{
											++nResident;
											if ( nSpew > 1 )
											{
												Msg( "%s:%s[%i:%i] was resident\n", pStudioHdr->pszName(), animdesc.pszName(), iGroup, j );
											}
										}
										else
										{
											if ( nSpew != 0 )
											{
												Msg( "%s:%s[%i:%i] async load\n", pStudioHdr->pszName(), animdesc.pszName(), iGroup, j );
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
	}

	if ( !nSpew || nChecked <= 0 )
		return;

	Msg( "%d of %d animations resident\n", nResident, nChecked );
}