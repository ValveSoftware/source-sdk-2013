//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#ifndef TF_LOBBYPANEL_CASUAL_H
#define TF_LOBBYPANEL_CASUAL_H

#include "cbase.h"
#include "game/client/iviewport.h"
#include "tf_lobbypanel.h"
#include "tf_leaderboardpanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


class CBaseLobbyPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CLobbyPanel_Casual : public CBaseLobbyPanel
{
	DECLARE_CLASS_SIMPLE( CLobbyPanel_Casual, CBaseLobbyPanel );

public:
	CLobbyPanel_Casual( vgui::Panel *pParent, CBaseLobbyContainerFrame* pLobbyContainer );
	virtual ~CLobbyPanel_Casual();

	//
	// Panel overrides
	//
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout( void ) OVERRIDE;

	virtual ETFMatchGroup GetMatchGroup( void ) const OVERRIDE { return k_eMatchGroup_Casual_12v12; }

	virtual void OnThink() OVERRIDE;

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

private:

	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked", panel );

	CPanelAnimationVarAliasType( int, m_iCategorySpacer, "category_spacer", "4", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iCategoryNameWidth, "category_name_width", "190", "proportional_int" );

	virtual bool ShouldShowLateJoin() const OVERRIDE;
	virtual void ApplyChatUserSettings( const LobbyPlayerInfo &player,KeyValues *pKV ) const OVERRIDE;
	virtual const char* GetResFile() const OVERRIDE { return "Resource/UI/LobbyPanel_Casual.res"; }

	void WriteGameSettingsControls() OVERRIDE;

	void WriteCategories( void );

	CUtlVector<vgui::Label *> m_vecSearchCriteriaLabels;

	vgui::HFont m_fontCategoryListItem;
	vgui::HFont m_fontGroupHeader;

	float m_flCompetitiveRankProgress;
	float m_flCompetitiveRankPrevProgress;
	float m_flRefreshPlayerListTime;
	bool m_bCompetitiveRankChangePlayedSound;
	float m_flNextCasualStatsUpdateTime;

	bool m_bHasAMapSelected;

	CUtlMap< EMatchmakingGroupType, Panel* > m_mapGroupPanels;
	CUtlMap< EGameCategory, Panel* > m_mapCategoryPanels;

	bool					m_bCriteriaDirty;
};

#endif //TF_LOBBYPANEL_COMP_H
