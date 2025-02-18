//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#ifndef TF_CASUAL_CRITERIA_H
#define TF_CASUAL_CRITERIA_H

#include "cbase.h"
#include "game/client/iviewport.h"
#include "vgui_controls/EditablePanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCasualCriteriaPanel	: public vgui::EditablePanel
							, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CCasualCriteriaPanel, vgui::EditablePanel );

public:
	CCasualCriteriaPanel( vgui::Panel *pParent, const char* pszName );
	virtual ~CCasualCriteriaPanel();

	//
	// Panel overrides
	//
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout( void ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void OnThink() OVERRIDE;

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

private:

	void WriteCategories( void );

	MESSAGE_FUNC_PARAMS( OnCategoryExpanded, "CategoryExpanded", params );
	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked", panel );

	CPanelAnimationVarAliasType( int, m_iCategorySpacer, "category_spacer", "4", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iCategoryNameWidth, "category_name_width", "190", "proportional_int" );

	CUtlVector<vgui::Label *> m_vecSearchCriteriaLabels;

	vgui::HFont m_fontCategoryListItem;
	vgui::HFont m_fontGroupHeader;

	float m_flCompetitiveRankProgress;
	float m_flCompetitiveRankPrevProgress;
	float m_flRefreshPlayerListTime;
	bool m_bCompetitiveRankChangePlayedSound;

	bool m_bHasAMapSelected;

	CUtlMap< EMatchmakingGroupType, Panel* > m_mapGroupPanels;
	CUtlMap< EGameCategory, Panel* > m_mapCategoryPanels;

	bool					m_bCriteriaDirty;
};

#endif //TF_CASUAL_CRITERIA_H
