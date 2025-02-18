//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <KeyValues.h>
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <filesystem.h>
#include <vgui_controls/AnimationController.h>
#include "iclientmode.h"
#include "clientmode_shared.h"
#include "shareddefs.h"
#include "tf_shareddefs.h"
#include "tf_controls.h"
#include "tf_gamerules.h"
#ifdef WIN32
#include "winerror.h"
#endif
#include "ixboxsystem.h"
#include "intromenu.h"
#include "tf_intromenu.h"
#include "inputsystem/iinputsystem.h"

// used to determine the action the intro menu should take when OnTick handles a think for us
enum
{
	INTRO_NONE,
	INTRO_STARTVIDEO,
	INTRO_BACK,
	INTRO_CONTINUE,
};

using namespace vgui;

// sort function for the list of captions that we're going to show
int CaptionsSort( CVideoCaption* const *p1, CVideoCaption* const *p2 )
{
	// check the start time
	if ( (*p2)->m_flStartTime < (*p1)->m_flStartTime )
	{
		return 1;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFIntroMenu::CTFIntroMenu( IViewPort *pViewPort ) : BaseClass( pViewPort )
{
	m_pVideo = new CTFVideoPanel( this, "VideoPanel" );
	m_pModel = new CModelPanel( this, "MenuBG" );
	m_pCaptionLabel = new CExLabel( this, "VideoCaption", "" );

#ifdef _X360
	m_pFooter = new CTFFooter( this, "Footer" );
#else
	m_pBack = new CExButton( this, "Back", "" );
	m_pOK = new CExButton( this, "Skip", "" );
	m_pReplayVideo = new CExButton( this, "ReplayVideo", "" );
	m_pContinue = new CExButton( this, "Continue", "" );
#endif

	m_iCurrentCaption = 0;
	m_flVideoStartTime = 0;

	m_flActionThink = -1;
	m_iAction = INTRO_NONE;

	//=============================================================================
	// HPE_BEGIN
	// [msmith] Flag for weather or not we're playing an in game video.
	//=============================================================================
	m_bPlayingInGameVideo = false;
	//=============================================================================
	// HPE_END
	//=============================================================================

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFIntroMenu::~CTFIntroMenu()
{
	m_Captions.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFIntroMenu::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( ::input->IsSteamControllerActive() )
	{
		LoadControlSettings( "Resource/UI/IntroMenu_SC.res" );
		SetMouseInputEnabled( false );
	}
	else
	{
		LoadControlSettings( "Resource/UI/IntroMenu.res" );
		SetMouseInputEnabled( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFIntroMenu::SetNextThink( float flActionThink, int iAction )
{
	m_flActionThink = flActionThink;
	m_iAction = iAction;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFIntroMenu::OnTick()
{
	// @note Tom Bui: (yuck)
	// in training, never show the back button
	// we do this late, because there's a race condition for when IsInTraining() will return true
	if ( m_pBack->IsVisible() && TFGameRules() && TFGameRules()->IsInTraining() )
	{
		m_pBack->SetVisible(false);
	}

	//=============================================================================
	// HPE_BEGIN
	// [msmith] Used to play a movie during a map.  For training videos.
	//=============================================================================
	if ( PendingInGameVideo() && !BaseClass::IsVisible() )
	{
		m_pViewPort->ShowPanel( this, true );
	}
	//=============================================================================
	// HPE_END
	//=============================================================================

	// do we have anything special to do?
	else if ( m_flActionThink > 0 && m_flActionThink < gpGlobals->curtime )
	{
		if ( m_iAction == INTRO_STARTVIDEO )
		{
				
			//=============================================================================
			// HPE_BEGIN
			// [msmith] Pulled start video into a separate function.
			//=============================================================================	
			StartVideo();
			//=============================================================================
			// HPE_END
			//=============================================================================
		}
		else if ( m_iAction == INTRO_BACK )
		{
			m_pViewPort->ShowPanel( this, false );
			m_pViewPort->ShowPanel( PANEL_MAPINFO, true );
		}
		else if ( m_iAction == INTRO_CONTINUE )
		{
			m_pViewPort->ShowPanel( this, false );

			//=============================================================================
			// HPE_BEGIN
			// [msmith] Used for the client to tell the server that we're watching a movie or not
			//=============================================================================
			tf_training_client_message.SetValue( "" );
			tf_training_client_message.SetValue( TRAINING_CLIENT_MESSAGE_NONE );
			//=============================================================================
			// HPE_END
			//=============================================================================

			if ( GetLocalPlayerTeam() == TEAM_UNASSIGNED )
			{
				if ( TFGameRules()->IsInArenaMode() == true && tf_arena_use_queue.GetBool() == true )
				{
					m_pViewPort->ShowPanel( PANEL_ARENA_TEAM, true );
				}
				else
				{
					engine->ClientCmd( "team_ui_setup" );
				}
			}
			else
			{
				C_TFPlayer *pPlayer =  C_TFPlayer::GetLocalTFPlayer();

				// only open the class menu if they're not on team Spectator and they haven't already picked a class
				if (  pPlayer && 
					( GetLocalPlayerTeam() != TEAM_SPECTATOR ) && 
					( pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED ) )
				{
					if ( tf_arena_force_class.GetBool() == false )
					{
						switch( GetLocalPlayerTeam() )
						{
						case TF_TEAM_RED:
							m_pViewPort->ShowPanel( PANEL_CLASS_RED, true );
							break;

						case TF_TEAM_BLUE:
							m_pViewPort->ShowPanel( PANEL_CLASS_BLUE, true );
							break;
						}
					}
				}
			}
		}

		// reset our think
		SetNextThink( -1, INTRO_NONE );
	}

	// check if we need to update our captions
	if ( m_pCaptionLabel && m_pCaptionLabel->IsVisible() )
	{
		UpdateCaptions();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFIntroMenu::OnThink()
{
	//Always hide the health... this needs to be done every frame because a message from the server keeps resetting this.
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer )
	{
		pLocalPlayer->m_Local.m_iHideHUD |= HIDEHUD_HEALTH;
	}

	BaseClass::OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFIntroMenu::LoadCaptions( void )
{
	bool bSuccess = false;

	// clear any current captions
	m_Captions.PurgeAndDeleteElements();
	m_iCurrentCaption = 0;

	if ( m_pCaptionLabel )
	{
		const char *szVideoFileName = GetVideoFileName( false );
		KeyValues *kvCaptions = NULL;
		char strFullpath[MAX_PATH];
		if ( szVideoFileName != NULL )
		{
			//=============================================================================
			// HPE_BEGIN
			// [msmith] The video may now be either a map video or an in game video.
			//			Made a function to decide which video name to give back.
			//=============================================================================
			Q_strncpy( strFullpath, szVideoFileName, MAX_PATH );	// Assume we must play out of the media directory
			//=============================================================================
			// HPE_END
			//=============================================================================		
			
			Q_strncat( strFullpath, ".res", MAX_PATH );					// Assume we're a .res extension type

			if ( g_pFullFileSystem->FileExists( strFullpath ) )
			{
				kvCaptions = new KeyValues( strFullpath );

				if ( kvCaptions )
				{
					if ( kvCaptions->LoadFromFile( g_pFullFileSystem, strFullpath ) )
					{
						for ( KeyValues *pData = kvCaptions->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey() )
						{
							CVideoCaption *pCaption = new CVideoCaption;
							if ( pCaption )
							{
								pCaption->m_pszString = ReadAndAllocStringValue( pData, "string" );
								pCaption->m_flStartTime = pData->GetFloat( "start", 0.0 );
								pCaption->m_flDisplayTime = pData->GetFloat( "length", 3.0 );

								m_Captions.AddToTail( pCaption );

								// we have at least one caption to show
								bSuccess = true;
							}
						}
					}

					kvCaptions->deleteThis();
				}
			}
		}
	}

	if ( bSuccess )
	{
		// sort the captions so we show them in the correct order (they're not necessarily in order in the .res file)
		m_Captions.Sort( CaptionsSort );
	}

	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFIntroMenu::UpdateCaptions( void )
{
	//=============================================================================
	// HPE_BEGIN
	// [msmith] Timing should be realtime when playing in game becase the curtime is paused.
	//=============================================================================
	float testTime = m_bPlayingInGameVideo ? gpGlobals->realtime : gpGlobals->curtime;
	//=============================================================================
	// HPE_END
	//=============================================================================		
	
	if ( m_pCaptionLabel && m_pCaptionLabel->IsVisible() && ( m_Captions.Count() > 0 ) )
	{
		CVideoCaption *pCaption = m_Captions[m_iCurrentCaption];

		if ( pCaption )
		{
			if ( ( pCaption->m_flCaptionStart >= 0 ) && ( pCaption->m_flCaptionStart + pCaption->m_flDisplayTime < testTime ) )
			{
				// fade out the caption
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "VideoCaptionFadeOut" );

				// move to the next caption
				m_iCurrentCaption++;

				if ( !m_Captions.IsValidIndex( m_iCurrentCaption ) )
				{
					// we're done showing captions
					m_pCaptionLabel->SetVisible( false );
				}
			}
			// is it time to show the caption?
			else if ( m_flVideoStartTime + pCaption->m_flStartTime < testTime )
			{
				// have we already started this video?
				if ( pCaption->m_flCaptionStart < 0 )
				{
					m_pCaptionLabel->SetText( pCaption->m_pszString );
					pCaption->m_flCaptionStart = testTime;

					// fade in the next caption
					g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "VideoCaptionFadeIn" );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFIntroMenu::ShowPanel( bool bShow )
{

	//=============================================================================
	// HPE_BEGIN:
	// [msmith]	Don't show the back button when in training.  You can only skip intro
	//			movies.
	//=============================================================================
	m_pBack->SetVisible(true);
	if ( TFGameRules() && TFGameRules()->IsInTraining() )
	{
		m_pBack->SetVisible( false );
		if ( PendingInGameVideo() == false )
		{			
			VideoSystem_t  playbackSystem = VideoSystem::NONE;
			char resolvedFile[MAX_PATH];
			if ( g_pVideo != NULL && g_pVideo->LocatePlayableVideoFile( GetVideoFileName(), "GAME", &playbackSystem, resolvedFile, sizeof(resolvedFile) ) != VideoResult::SUCCESS  )
			{
				//If we have no movie, no need to show the intro screen on a training mission.
				bShow = false;
			}
		}
	}
	//=============================================================================
	// HPE_END
	//=============================================================================


	if ( BaseClass::IsVisible() == bShow )
		return;

	// reset our think
	SetNextThink( -1, INTRO_NONE );

	if ( bShow )
	{
		InvalidateLayout( true, true );
		Activate();

		if ( m_pVideo )
		{
			//=============================================================================
			// HPE_BEGIN
			// [msmith] Pulled shutting down the video into a separate function.
			//			If we're showing an in game video, we need to enable pausing so that
			//			we can pause the game during the video.
			//			If we're showing an intro training movie, we also need to tell the server that
			//			we're whatching the intro movie so that the round does not start until it's over.
			//			If we are watching an in game video, we do NOT send a message for that because 
			//			tf_training_client_message will contain the name of the video we're watching.
			//=============================================================================			
			ShutdownVideo();
			SetNextThink( gpGlobals->curtime + m_pVideo->GetStartDelay(), INTRO_STARTVIDEO );
			
			if ( TFGameRules() && TFGameRules()->IsInTraining() )
			{
				if ( PendingInGameVideo() )
				{
					engine->ClientCmd( "sv_pausable 1" );
				}
				else
				{
					tf_training_client_message.SetValue( TRAINING_CLIENT_MESSAGE_WATCHING_INTRO_MOVIE );
				}
			}
			//=============================================================================
			// HPE_END
			//=============================================================================

		}

		if ( m_pModel )
		{
			m_pModel->SetPanelDirty();
		}
	}
	else
	{
		Shutdown();

		SetVisible( false );

		//=============================================================================
		// HPE_BEGIN
		// [msmith] We must disable the ability to pause.  If we don't, it looks like
		//			some other function in TF2 causes the entire game to pause if sv_pausable is enabled.
		//=============================================================================	
		if ( TFGameRules() && TFGameRules()->IsInTraining() )
		{
			engine->ClientCmd( "sv_pausable 0" );
		}
		//=============================================================================
		// HPE_END
		//=============================================================================
	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFIntroMenu::OnIntroFinished( void )
{
	// in training we want to give the user the ability to replay the movie
	if ( TFGameRules() && TFGameRules()->IsInTraining() )
	{
		m_pReplayVideo->SetVisible( true );
		m_pContinue->SetVisible( true );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "IntroMovieContinueBlink" );
		m_pOK->SetVisible( false );
	}
	else
	{
		float flTime = gpGlobals->curtime;

		if ( m_pModel && m_pModel->SetSequence( "UpSlow" ) )
		{
			// wait for the model sequence to finish before going to the next menu
			flTime = gpGlobals->curtime + m_pVideo->GetEndDelay();
		}

		Shutdown();

		SetNextThink( flTime, INTRO_CONTINUE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFIntroMenu::OnCommand( const char *command )
{
	if ( !Q_strcmp( command, "back" ) )
	{
		float flTime = gpGlobals->curtime;

		Shutdown();

		// try to play the screenup sequence
		if ( m_pModel && m_pModel->SetSequence( "Up" ) )
		{
			flTime = gpGlobals->curtime + 0.35f;
		}

		// wait for the model sequence to finish before going back to the mapinfo menu
		SetNextThink( flTime, INTRO_BACK );
	}
	else if ( !Q_strcmp( command, "skip" ) )
	{
		Shutdown();

		// continue right now
		SetNextThink( gpGlobals->curtime, INTRO_CONTINUE );
	}
	else if ( !Q_strcmp( command, "replayVideo" ) )
	{
		ShutdownVideo();
		SetNextThink( gpGlobals->curtime, INTRO_STARTVIDEO );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFIntroMenu::OnKeyCodePressed( KeyCode code )
{
	if ( code == KEY_XBUTTON_A || code == STEAMCONTROLLER_A )
	{
		OnCommand( "skip" );
	}
	else if ( code == KEY_XBUTTON_B || code == STEAMCONTROLLER_B )
	{
		OnCommand( "back" );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFIntroMenu::Shutdown( void )
{
	//=============================================================================
	// HPE_BEGIN
	// [msmith] Refactored the shutdown video logic into a containing function.
	//=============================================================================			
	ShutdownVideo();
	//=============================================================================
	// HPE_END
	//=============================================================================

	if ( m_pCaptionLabel && m_pCaptionLabel->IsVisible() )
	{
		m_pCaptionLabel->SetVisible( false );
	}

	m_iCurrentCaption = 0;
	m_flVideoStartTime = 0;

}




//=============================================================================
// HPE_BEGIN
// [msmith] New helper functions
//=============================================================================	
void CTFIntroMenu::ShutdownVideo()
{
	if ( m_pVideo )
	{
		m_pVideo->Shutdown(); // make sure we're not currently running
	}

	//Make sure we unpause the game if it was paused from an in game play of a video.
	if ( m_bPlayingInGameVideo )
	{
		UnpauseGame();
	}

	m_bPlayingInGameVideo = false;
}

bool CTFIntroMenu::PendingInGameVideo( void )
{
	if ( TFGameRules() && TFGameRules()->IsInTraining() )
	{
		//If the message is a string, it's a video name.
		return strlen( tf_training_client_message.GetString() ) > 3;
	}
	
	return false;
}

const char *CTFIntroMenu::GetVideoFileName( bool withExtension )
{
	if ( PendingInGameVideo() )
	{
		return TFGameRules()->FormatVideoName( tf_training_client_message.GetString(), withExtension );
	}

	if ( TFGameRules() && TFGameRules()->IsInTraining() )
	{
		ConVarRef training_map_video("training_map_video");
		if ( strlen( training_map_video.GetString() ) > 3 )
		{
			return TFGameRules()->FormatVideoName( training_map_video.GetString(), withExtension );
		}
	}

	return TFGameRules()->GetVideoFileForMap( withExtension );
}

void CTFIntroMenu::StartVideo()
{
	m_pOK->SetVisible( true );
	m_pReplayVideo->SetVisible( false );
	m_pContinue->SetVisible( false );
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "IntroMovieContinueBlinkStop" );
	if ( m_pVideo )
	{
		// turn on the captions if we have them
		if ( LoadCaptions() )
		{
			if ( m_pCaptionLabel && !m_pCaptionLabel->IsVisible() )
			{
				m_pCaptionLabel->SetText( " " );
				m_pCaptionLabel->SetVisible( true );
				//Make sure the label is fully faded in when starting to play.
				//It could have been faded out from a prior animation event form an animation effect in a previous video instance.
				m_pCaptionLabel->SetAlpha( 255 );
			}
		}
		else
		{
			if ( m_pCaptionLabel && m_pCaptionLabel->IsVisible() )
			{
				m_pCaptionLabel->SetVisible( false );
			}
		}

		m_pVideo->Activate();

		if ( PendingInGameVideo() )
		{
			m_pVideo->BeginPlayback( GetVideoFileName() );
			PauseGame();
			m_bPlayingInGameVideo = true;

			//Since we have started playing the video, we can reset the message string to empty.
			tf_training_client_message.SetValue( "" );
		}
		else
		{
			m_pVideo->BeginPlayback( GetVideoFileName() );
		}

		m_pVideo->MoveToFront();

		m_flVideoStartTime = m_bPlayingInGameVideo ? gpGlobals->realtime : gpGlobals->curtime;
	}
}

void CTFIntroMenu::UnpauseGame( void )
{
	if ( TFGameRules() && TFGameRules()->IsInTraining() )
	{
		engine->ClientCmd( "unpause" );
	}
}

void CTFIntroMenu::PauseGame( void )
{
	if ( TFGameRules() && TFGameRules()->IsInTraining() )
	{
		engine->ClientCmd( "pause" );
	}
}
//=============================================================================
// HPE_END
//=============================================================================
