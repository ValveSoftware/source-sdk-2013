//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef REPLAY_RENDEROVERLAY_H
#define REPLAY_RENDEROVERLAY_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------

#include "vgui_controls/Frame.h"
#include "vgui_controls/ProgressBar.h"
#include "replay/rendermovieparams.h"

//-----------------------------------------------------------------------------

class CExButton;
class CExLabel;
class IQuickTimeMovieMaker;
class CReplay;
class CReplayRenderer;

//-----------------------------------------------------------------------------

class CReplayRenderOverlay : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CReplayRenderOverlay, vgui::Frame );
public:
	CReplayRenderOverlay( Panel *pParent );
	~CReplayRenderOverlay();

	void Show();
	void Hide();

	CReplayRenderer		*m_pRenderer;

private:
	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	PerformLayout();
	virtual void	OnTick();
	virtual void	OnMousePressed( vgui::MouseCode nCode );
	virtual void	OnKeyCodeTyped( vgui::KeyCode nCode );
	virtual void	OnCommand( const char *pCommand );

private:
	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked", pPanel );

#if _DEBUG
	bool				m_bReloadScheme;
#endif

	int					m_unNumFrames;
	float				m_flStartTime;
	float				m_flPreviousTimeLeft;
	EditablePanel		*m_pBottom;
	vgui::ProgressBar	*m_pRenderProgress;
	vgui::CheckButton	*m_pPreviewCheckButton;
	CExButton			*m_pCancelButton;
	CExLabel			*m_pTitleLabel;
	CExLabel			*m_pFilenameLabel;
	CExLabel			*m_pProgressLabel;
};

//-----------------------------------------------------------------------------

void ReplayUI_OpenReplayRenderOverlay();
void ReplayUI_HideRenderOverlay();

//-----------------------------------------------------------------------------

#endif // REPLAY_RENDEROVERLAY_H
