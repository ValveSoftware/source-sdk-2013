//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_MANN_VS_MACHINE_VICTORY_H
#define TF_HUD_MANN_VS_MACHINE_VICTORY_H
#ifdef _WIN32
#pragma once
#endif


#include "hudelement.h"
#include "tf_controls.h"
#include "hud.h"
#include <vgui/IScheme.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/ScalableImagePanel.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui/ISurface.h>
#include <vgui_controls/ImageList.h>
#include <vgui/KeyCode.h>
#include "vgui_controls/SectionedListPanel.h"
#include "c_tf_objective_resource.h"
#include "vgui_avatarimage.h"
#include "item_model_panel.h"
#include "c_playerresource.h"
#include "tf_gcmessages.h"
#include "tf_mann_vs_machine_stats.h"
#include "tf_gamerules.h"
#include "tf_hud_mann_vs_machine_stats.h"
#include "tf_gc_client.h"

class CTFParticlePanel;

//=========================================================
class CVictoryPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CVictoryPanel, vgui::EditablePanel );
public:
	CVictoryPanel( Panel *parent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnTick( void );
	
	void ResetVictoryPanel();

	void SetMapAndPopFile ();
	
private:
	enum
	{
		INITIAL_VICTORY = 0,
		CREDITS_COLLECT,
		CREDITS_MISSED,
		CREDITS_BONUS,
		YOUR_UPGRADES,
		YOUR_BUYBACK,
		YOUR_BOTTLES,
		RATING_LABEL,
		RATING_SCORE,
		FINISHED,
	};

	void CaptureStats();

	bool StateUpdateValue( vgui::EditablePanel *parent, char* field, float targetTime, float currentTime, int nextState, int endValue );
	bool StateUpdateCreditText( vgui::EditablePanel *parent, char* field, float targetTime, float currentTime, int nextState, int useValue, int creditValue );
	bool CheckState( float targetTime, float currentTime, int nextState );

	void RatingLabelUpdate( void );
	void RatingScoreUpdate( void );

	float m_fPreviousTick;
	float m_fStateRunningTime;

	vgui::EditablePanel *m_pHeaderContainer;
	vgui::EditablePanel *m_pCreditContainerPanel;
	vgui::EditablePanel *m_pTeamStatsContainerPanel;
	vgui::EditablePanel *m_pYourStatsContainerPanel;
	vgui::EditablePanel *m_pRatingContainerPanel;
	CExImageButton		*m_pDoneButton;

	CCreditSpendPanel *m_pTotalGameCreditSpendPanel;

	int m_eState;

	int m_nCreditsCollected;
	int m_nCreditsMissed;
	int m_nCreditBonus;

	int m_nYourBuybacksCredits;
	int m_nYourBottlesCredits;
	int m_nYourUpgradeCredits;
};

#endif // TF_HUD_MANN_VS_MACHINE_VICTORY_H
