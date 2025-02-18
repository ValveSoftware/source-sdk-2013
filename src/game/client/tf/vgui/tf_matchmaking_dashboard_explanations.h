//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_MATCHMAKING_DASHBOARD_EXPLANATIONS_H
#define TF_MATCHMAKING_DASHBOARD_EXPLANATIONS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>

class CExplanationPopup;

using namespace vgui;

CExplanationPopup* ShowDashboardExplanation( const char* pszExplanation );

class CExplanationManager : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CExplanationManager, EditablePanel );
public:
	CExplanationManager();
	virtual ~CExplanationManager();

	virtual void SetParent( Panel *newParent ) OVERRIDE;
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void OnChildAdded( VPANEL child ) OVERRIDE;
	virtual void OnTick() OVERRIDE;

	CExplanationPopup* ShowExplanation( const char* pszExplanationName );

private:
	CUtlVector < CExplanationPopup* > m_vecPopups;
	CUtlVector < CExplanationPopup* > m_vecQueuedPopups;
};

#endif // TF_MATCHMAKING_DASHBOARD_EXPLANATIONS_H
