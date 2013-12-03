//========= Copyright Valve Corporation, All rights reserved. ============//
//
// VGUI_VIDEO_PLAYER
//
//  Creates a VGUI Panel that can play a video in engine with "media player like"
//     functionality.  vgui_video.h has a basic player panel, but this allows for
//     much greater control over playback, etc 
//
//=============================================================================

#ifndef VGUI_VIDEO_PLAYER
#define VGUI_VIDEO_PLAYER

#ifdef _WIN32
	#pragma once
#endif

#include <vgui_controls/Panel.h>
#include "video/ivideoservices.h"


class VideoPlayerPanel : public vgui::Panel		// Should this be EditablePanel ?
{
	public:

		DECLARE_CLASS_SIMPLE( VideoPlayerPanel, vgui::Panel );
	
							VideoPlayerPanel( vgui::Panel *parent, const char *panelName, int nXpos, int nYpos, int nWidth, int nHeight, const char *pVideoFile = NULL );
	
		virtual			   ~VideoPlayerPanel();

		virtual void 		Activate( void );
		virtual void 		Paint( void );
		virtual void 		OnClose( void );
		
		bool				SetVideo( const char *pVideoFile );
		void				ClearVideo();
		bool				IsReady()			{ return m_VideoLoaded; }
		
		bool				StartVideo();
		bool				StopVideo();
		bool				PauseVideo();
		bool				UnpauseVideo();
		bool				IsPlaying()			{ return m_VideoPlaying; }
		bool				IsPaused()			{ return m_VideoPaused; }
		
		float				GetCurrentPlaybackTime();
		bool				SetCurrentPlaybackTime( float newTime );

		float				GetVideoDuration()	{ return m_VideoDuration; }
		bool				HasAudio();
		
		bool				IsMuted();
		bool				SetMute( bool muted );
		
		float				GetVolume();
		bool				SetVolume( float newVolume );
	

	protected:

		virtual void		OnTick( void ) { BaseClass::OnTick(); }
		virtual void		OnCommand( const char *pcCommand ) { BaseClass::OnCommand( pcCommand ); }

	private:

		IVideoMaterial		*m_VideoMaterial;
		IMaterial			*m_pMaterial;
	
		bool				m_VideoLoaded;
		bool				m_VideoPlaying;
		bool				m_VideoPaused;
	
		int					m_nPlaybackHeight;			// Calculated to address ratio changes
		int					m_nPlaybackWidth;
		int					m_letterBox;
		
		float				m_flU, m_flV;

		char			   *m_VideoFileName;
		float				m_VideoDuration;
		
	

};

#endif

