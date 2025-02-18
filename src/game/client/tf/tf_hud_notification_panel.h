//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_HUD_NOTIFICATION_PANEL_H
#define TF_HUD_NOTIFICATION_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include "IconPanel.h"
#include <vgui_controls/ImagePanel.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudNotificationPanel : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudNotificationPanel, EditablePanel );

public:

	typedef enum
	{
		kBackground_Blue,
		kBackground_Red,
		kBackground_Black,
	} BackgroundType_t;

	CHudNotificationPanel( const char *pElementName );

	virtual void	Init( void );
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );
	virtual void	PerformLayout( void );

	const char *GetNotificationByType( int iType, float& flDuration );

	void	MsgFunc_HudNotify( bf_read &msg );
	void	MsgFunc_HudNotifyCustom( bf_read &msg );

	void	SetupNotifyCustom( const char *pszText, const char *pszIcon, int iBackgroundTeam );
	void	SetupNotifyCustom( const wchar_t *pszText, const char *pszIcon, int iBackgroundTeam );
	void	SetupNotifyCustom( const wchar_t *pszText, HudNotification_t type, float overrideDuration = 0.0f );

	virtual void LevelInit( void ) { m_flFadeTime = 0; };

	bool		LoadManifest( void );

private:
	float m_flFadeTime;

	Label *m_pText;
	CIconPanel *m_pIcon;
	ImagePanel *m_pBackground;

	struct ShowCount_t
	{
		ShowCount_t() {}

		ShowCount_t( int nMaxShowCount, float flCooldown, ConVar* pConVar )
			: m_nMaxShowCount( nMaxShowCount )
			, m_flCooldown( flCooldown )
			, m_pConVar( pConVar )
			, m_flNextAllowedTime( 0.f )
		{}
		int m_nMaxShowCount;
		ConVar* m_pConVar;
		float m_flCooldown;
		float m_flNextAllowedTime;
	};
	CUtlMap< int, ShowCount_t > m_mapShowCounts;
};

#endif // TF_HUD_NOTIFICATION_PANEL_H
