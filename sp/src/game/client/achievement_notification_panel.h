//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ACHIEVEMENT_NOTIFICATION_PANEL_H
#define ACHIEVEMENT_NOTIFICATION_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "hudelement.h"

using namespace vgui;

class CAchievementNotificationPanel : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CAchievementNotificationPanel, EditablePanel );

public:
	CAchievementNotificationPanel( const char *pElementName );

	virtual void	Init();
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	PerformLayout( void );
	virtual void	LevelInit( void ) { m_flHideTime = 0; }
	virtual void	FireGameEvent( IGameEvent * event );
	virtual void	OnTick( void );

	void AddNotification( const char *szIconBaseName, const wchar_t *pHeading, const wchar_t *pTitle );

private:
	void ShowNextNotification();
	void SetXAndWide( Panel *pPanel, int x, int wide );

	float m_flHideTime;

	Label *m_pLabelHeading;
	Label *m_pLabelTitle;
	EditablePanel *m_pPanelBackground;
	ImagePanel *m_pIcon;

	struct Notification_t
	{
		char szIconBaseName[255];
		wchar_t szHeading[255];
		wchar_t szTitle[255];
	};

	CUtlLinkedList<Notification_t> m_queueNotification;
};

#endif	// ACHIEVEMENT_NOTIFICATION_PANEL_H