//========= Copyright Valve Corporation, All rights reserved. ============//
//
//----------------------------------------------------------------------------------------

#ifndef REPLAYMESSAGEPANEL_H
#define REPLAYMESSAGEPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/EditablePanel.h"

using namespace vgui;

//----------------------------------------------------------------------------------------
// Purpose:
//----------------------------------------------------------------------------------------
extern ConVar replay_msgduration_startrecord;
extern ConVar replay_msgduration_stoprecord;
extern ConVar replay_msgduration_replaysavailable;
extern ConVar replay_msgduration_error;
extern ConVar replay_msgduration_misc;
extern ConVar replay_msgduration_connectrecording;

//----------------------------------------------------------------------------------------
// Purpose: Forward declarations
//----------------------------------------------------------------------------------------
class CExLabel;
class CExButton;

class CReplayMessageDlg : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CReplayMessageDlg, EditablePanel );
public:
	CReplayMessageDlg( const char *pText );
	~CReplayMessageDlg();

	virtual void	ApplySchemeSettings( IScheme *pScheme );
	virtual void	PerformLayout();

	virtual void	OnKeyCodeTyped( KeyCode nCode );
	virtual void	OnCommand( const char *pCommand );

private:
	void			Close();

	Panel			*m_pDlg;
	CExLabel		*m_pMsgLabel;
	CExButton		*m_pOKButton;
};

//----------------------------------------------------------------------------------------
// Purpose: A panel for display messages from the replay system during gameplay
//----------------------------------------------------------------------------------------
class CReplayMessagePanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CReplayMessagePanel, EditablePanel );
public:
	CReplayMessagePanel( const char *pLocalizeName, float flDuration, bool bUrgent );
	virtual ~CReplayMessagePanel();

	void Show();
	virtual void OnTick();

	static int	InstanceCount();
	static void	RemoveAll();

private:
	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();


	CExLabel	*m_pMessageLabel;
	CExLabel	*m_pReplayLabel;
	ImagePanel	*m_pIcon;
	float		m_flShowStartTime;
	float		m_flShowDuration;
	bool		m_bUrgent;

#if defined( TF_CLIENT_DLL )
	char		m_szBorderName[ 64 ];
#endif
};

#endif // REPLAYMESSAGEPANEL_H