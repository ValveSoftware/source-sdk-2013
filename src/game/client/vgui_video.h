//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: VGUI panel which can play back video, in-engine
//
//=============================================================================

#ifndef VGUI_VIDEO_H
#define VGUI_VIDEO_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui_controls/EditablePanel.h>

//#define QUICKTIME_VIDEO
//#define BINK_VIDEO

#include "video/ivideoservices.h"


class VideoPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( VideoPanel, vgui::EditablePanel );
public:

	VideoPanel( unsigned int nXPos, unsigned int nYPos, unsigned int nHeight, unsigned int nWidth, bool allowAlternateMedia = true );

	virtual ~VideoPanel( void );

	virtual void Activate( void );
	virtual void Paint( void );
	virtual void DoModal( void );
	virtual void OnKeyCodeTyped( vgui::KeyCode code );
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void OnClose( void );
	virtual void GetPanelPos( int &xpos, int &ypos );

	void SetExitCommand( const char *pExitCommand )
	{
		if ( pExitCommand && pExitCommand[0] )
		{
			Q_strncpy( m_szExitCommand, pExitCommand, MAX_PATH );
		}
	}

	virtual bool BeginPlayback( const char *pFilename );

	void SetBlackBackground( bool bBlack ){ m_bBlackBackground = bBlack; }

protected:

	virtual void OnTick( void ) { BaseClass::OnTick(); }
	virtual void OnCommand( const char *pcCommand ) { BaseClass::OnCommand( pcCommand ); }
	virtual void OnVideoOver(){}

protected:
	IVideoMaterial *m_VideoMaterial;
	
	IMaterial		*m_pMaterial;
	int				m_nPlaybackHeight;			// Calculated to address ratio changes
	int				m_nPlaybackWidth;
	char			m_szExitCommand[MAX_PATH];	// This call is fired at the engine when the video finishes or is interrupted

	float			m_flU;	// U,V ranges for video on its sheet
	float			m_flV;

	bool			m_bBlackBackground;
	bool			m_bAllowAlternateMedia;
};


// Creates a VGUI panel which plays a video and executes a client command at its finish (if specified)
extern bool VideoPanel_Create( unsigned int nXPos, unsigned int nYPos, 
							   unsigned int nWidth, unsigned int nHeight, 
							   const char *pVideoFilename, 
							   const char *pExitCommand = NULL );

#endif // VGUI_VIDEO_H
