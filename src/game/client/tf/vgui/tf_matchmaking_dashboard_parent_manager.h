//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_MATCHMAKING_DASHBOARD_PARENT_MANAGER_H
#define TF_MATCHMAKING_DASHBOARD_PARENT_MANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PHandle.h>
#include <vgui_controls/Panel.h>

namespace vgui
{
	class Panel;
	class EditablePanel;
}

class CMMDashboardParentManager* GetMMDashboardParentManager();

//-----------------------------------------------------------------------------
// CMMDashboardParentManager
// Purpose: This guy keeps the MM dashboard as the top-most panel but does so
//			*without making it a popup*.  This is important because popups look
//			awful whenever they overlap and transparency is involved.  This class
//			does its dirty work by keeping track of the top-most fullscreen popup
//			and setting that panel as the MM dashboard's parent.  When that popup
//			goes away, we set the parent to the next popup on the stack, or to
//			the GameUI if none are active.  If we're in-game, then we parent to
//			the our special popup container.  Why not always just parent to that
//			single popup container?  Because we want the MINIMUM mouse focus area
//			possible because the dashboard is not a rectangle (it grows/shrinks).
//			
//			
//			If anything draws on top of the MM dashboard and you dont want it to
//			have that panel add itself to this class using PushModalFullscreenPopup
//			when it goes visible and PopModalFullscreenPopup when it hides itself
//-----------------------------------------------------------------------------
class CMMDashboardParentManager : public CGameEventListener
{
public:
	CMMDashboardParentManager();

	void UpdateParenting();

	void AddPanel( vgui::Panel* pChild );
	void RemovePanel( vgui::Panel* pChild );

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;
private:

	void AttachToGameUI();
	void AttachToTopMostPopup();

	bool m_bAttachedToGameUI;

	class CUtlSortVectorPanelZPos
	{
	public:
		bool Less( const vgui::Panel* lhs, const vgui::Panel* rhs, void * )
		{
			return lhs->GetZPos() < rhs->GetZPos();
		}
	};

	CUtlSortVector< vgui::Panel*, CUtlSortVectorPanelZPos > m_vecPanels;
	vgui::PHandle m_pHUDPopup;
};

#endif // TF_MATCHMAKING_DASHBOARD_PARENT_MANAGER_H
