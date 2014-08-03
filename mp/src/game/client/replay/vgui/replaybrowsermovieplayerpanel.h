//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef REPLAYBROWSERMOVIEPLAYERPANEL_H
#define REPLAYBROWSERMOVIEPLAYERPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "replaybrowserbasepanel.h"
#include "video/ivideoservices.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: A panel that plays AVI's
//-----------------------------------------------------------------------------
class CMoviePlayerPanel : public CReplayBasePanel
{
	DECLARE_CLASS_SIMPLE( CMoviePlayerPanel, CReplayBasePanel );
public:
	CMoviePlayerPanel( Panel *pParent, const char *pName, const char *pMovieFilename );
	~CMoviePlayerPanel();

	virtual void	Paint();

	void			Play();
	void			SetLooping( bool bLooping )		{ m_bLooping = bLooping; }

	bool			IsPlaying()						{ return m_bPlaying; }
	void			SetScrubOnMouseOverMode( bool bOn );
	void			FreeMaterial();
	void			ToggleFullscreen();

private:
	virtual void	PerformLayout();
	virtual void	OnMousePressed( MouseCode code );
	virtual void	OnTick();

	IVideoMaterial *m_pVideoMaterial;

	IMaterial		*m_pMaterial;
	float			m_flCurFrame;
	int				m_nNumFrames;
	bool			m_bPlaying;
	bool			m_bLooping;
	float			m_flLastTime;
	int				m_nGlobalPos[2];
	int				m_nLastMouseXPos;
	bool			m_bFullscreen;
	Panel			*m_pOldParent;
	int				m_aOldBounds[4];
	bool			m_bMouseOverScrub;		// In this mode, we don't playback, only scrub on mouse over
};

#endif // REPLAYBROWSERMOVIEPLAYERPANEL_H
