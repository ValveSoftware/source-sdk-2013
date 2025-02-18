//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_MATCHMAKING_DASHBOARD_POPUP_H
#define TF_MATCHMAKING_DASHBOARD_POPUP_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "tf_controls.h"
#include <vgui_controls/PHandle.h>
#include "local_steam_shared_object_listener.h"

//-----------------------------------------------------------------------------
// Purpose: Popup that goes underneath the dashboard and displays anything
//			important the user needs to know about
//-----------------------------------------------------------------------------
class CTFMatchmakingPopup : public CExpandablePanel
						  , public CGameEventListener
{
	friend class CTFMatchmakingDashboard;
	DECLARE_CLASS_SIMPLE( CTFMatchmakingPopup, CExpandablePanel );
public:

	CTFMatchmakingPopup( const char* pszName );
	virtual ~CTFMatchmakingPopup();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void OnThink() OVERRIDE;
	virtual void OnTick() OVERRIDE;

	virtual void OnEnter();
	virtual void OnUpdate();
	virtual void OnExit();
	virtual void Update();

	virtual void FireGameEvent( IGameEvent *pEvent ) OVERRIDE;

private:
	virtual bool ShouldBeActve() const = 0;
	void UpdateRematchtime();
	void UpdateAutoJoinTime();

	bool m_bActive;
};


#endif // TF_MATCHMAKING_DASHBOARD_POPUP_H
