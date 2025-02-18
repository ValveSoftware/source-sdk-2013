//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_OBJECTIVESTATUS_H
#define TF_HUD_OBJECTIVESTATUS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_controls.h"
#include "tf_imagepanel.h"
#include "tf_hud_flagstatus.h"
#include "tf_hud_escort.h"
#include "tf_hud_training.h"
#include "hud_controlpointicons.h"
#include "GameEventListener.h"

#define MAX_BOSS_STUN_SKILL_SHOTS 3

//-----------------------------------------------------------------------------
// Purpose:  Parent panel for the various objective displays
//-----------------------------------------------------------------------------
class CTFHudPasstime;
class CTFHudObjectiveStatus : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudObjectiveStatus, vgui::EditablePanel );

public:
	CTFHudObjectiveStatus( const char *pElementName );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();
	virtual void Think();
	virtual bool ShouldDraw() OVERRIDE;

	virtual int GetRenderGroupPriority( void ) { return 60; }	// higher than build menus

	CControlPointProgressBar *GetControlPointProgressBar( void );

	//=============================================================================
	// HPE_BEGIN
	// [msmith] Functions for training stuff.
	//=============================================================================
	void SetTrainingText( char *msg);
	void SetTrainingObjective (char *obj);
	//=============================================================================
	// HPE_END
	//=============================================================================

private:

	void	SetVisiblePanels( void );

private:

	float					m_flNextThink;

	CTFHudFlagObjectives	*m_pFlagPanel;
	
	CHudControlPointIcons	*m_pControlPointIconsPanel;
	CControlPointProgressBar *m_pControlPointProgressBar;
	CTFHudEscort			*m_pEscortPanel;
	CTFHudMultipleEscort	*m_pMultipleEscortPanel;
	class CTFHUDRobotDestruction	*m_pRobotDestructionPanel;
	CTFHudPasstime			*m_pHudPasstime;
	
	//=============================================================================
	// HPE_BEGIN:
	// [msmith]	HUD for training stuff.
	//=============================================================================
	CTFHudTraining      *m_pTrainingPanel;
	//=============================================================================
	// HPE_END
	//=============================================================================
};

#endif	// TF_HUD_OBJECTIVESTATUS_H
