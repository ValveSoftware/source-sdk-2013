//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#ifndef TF_MVM_CRITERIA_H
#define TF_MVM_CRITERIA_H


#include "cbase.h"
#include "game/client/iviewport.h"
#include "vgui_bitmapimage.h"
#include "vgui_controls//SectionedListPanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

class CMvMPlayerTicketStatusPanel;

class CMVMCriteriaPanel : public vgui::EditablePanel
						, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CMVMCriteriaPanel, vgui::EditablePanel );

public:
	CMVMCriteriaPanel( vgui::Panel *pParent, const char* pszName );
	virtual ~CMVMCriteriaPanel();

	//
	// Panel overrides
	//
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;

	//
	// CGameEventListener overrides
	//
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

	virtual void OnCommand( const char *command ) OVERRIDE;

	void SetMannUpTicketCount( int nCount );
	void SetSquadSurplusCount( int nCount );

	void ToggleSquadSurplusCheckButton( void )
	{
		if ( !GTFGCClientSystem()->BLocalPlayerInventoryHasSquadSurplusVoucher() )
			return;

		m_pSquadSurplusCheckButton->SetSelected( !m_pSquadSurplusCheckButton->IsSelected() );
	}


	void WriteControls();

private:
	int m_iImageIsBanned = -1;
	int m_iImageNew = -1;
	int m_iImageNo = -1;
	int m_iImageRadioButtonYes = -1;
	int m_iImageRadioButtonNo = -1;
	int m_iImageCheckBoxDisabled = -1;
	int m_iImageCheckBoxYes = -1;
	int m_iImageCheckBoxNo = -1;
	int m_iImageCheckBoxMixed = -1;

	CPanelAnimationVarAliasType( int, m_iNewWidth, "challenge_new_width", "19", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iChallengeCheckBoxWidth, "challenge_check_box_width", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iAvatarWidth, "avatar_width", "16", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iPlayerNameWidth, "player_name_width", "110", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBannedWidth, "squad_surplus_width", "12", "proportional_int" );

	CPanelAnimationVarAliasType( int, m_iChallengeSpacer, "challenge_spacer", "4", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iChallengeNameWidth, "challenge_name_width", "190", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iChallengeSkillWidth, "challenge_skill_width", "110", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iChallengeCompletedSize, "challenge_completed_size", "15", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iMapImageWidth, "challenge_map_width", "60", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iMapImageHeight, "challenge_map_height", "40", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTourMapWidth, "squad_surplus_width", "20", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBadgeLevelWidth, "badge_level_width", "20", "proportional_int" );

	CPanelAnimationVarAliasType( int, m_iTourNameWidth, "tour_name_width", "160", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTourSkillWidth, "tour_skill_width", "90", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTourProgressWidth, "tour_progress_width", "70", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTourNumberWidth, "tour_number_width", "40", "proportional_int" );

	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked", panel );

	MESSAGE_FUNC_PTR( OnItemLeftClick, "ItemLeftClick", panel );

#ifdef USE_MVM_TOUR
	void OnClickedOnTour();
#endif // USE_MVM_TOUR
	void OnClickedOnChallenge();

	class ChallengeList : public vgui::SectionedListPanel
	{
	public:
		ChallengeList( CMVMCriteriaPanel *pCriteriaPanel, vgui::Panel *parent, const char *name )
		: vgui::SectionedListPanel( parent, name )
		, m_pCriteriaPanel( pCriteriaPanel )
		{
			m_imageChallengeCompleted.SetImageFile( "vgui/pve/mvm_challenge_completed" );
		}

		virtual void OnMouseDoublePressed(vgui::MouseCode code) OVERRIDE { /* Just eat it */ }
		virtual void Paint() OVERRIDE;

		CMVMCriteriaPanel *m_pCriteriaPanel;

		BitmapImage m_imageChallengeCompleted;
		CUtlVector<BitmapImage> m_vecMapImages;
	};

	CUtlVector<vgui::Label *> m_vecSearchCriteriaLabels;

	vgui::EditablePanel *m_pMvMTourOfDutyGroupPanel;
		vgui::EditablePanel *m_pMvMTourOfDutyListGroupBox;
		vgui::SectionedListPanel *m_pTourList;

	vgui::EditablePanel *m_MvMEconItemsGroupBox;
		vgui::CheckButton *m_pSquadSurplusCheckButton;
		vgui::Button *m_pMannUpNowButton;
		CMvMPlayerTicketStatusPanel* m_arPlayerTicketStatusPanels[ MAX_PARTY_SIZE ];

	vgui::EditablePanel *m_pMannUpTourLootDescriptionBox;
		vgui::ImagePanel *m_pMannUpTourLootImage;
		vgui::Label *m_pTourDifficultyWarning;
		//vgui::Label *m_pMannUpTourLootDetailLabel;

	vgui::EditablePanel *m_MvMPracticeGroupPanel;

	vgui::EditablePanel *m_pMvMSelectChallengeGroupPanel;
		vgui::EditablePanel *m_pMVMChallengeListGroupBox;
		ChallengeList *m_pChallengeList;


	vgui::HFont m_fontChallengeListHeader;
	vgui::HFont m_fontChallengeListItem;


	int m_iWritingPanel = 0;
	vgui::ImageList				*m_pImageList = NULL;

#ifdef USE_MVM_TOUR
	void WriteTourList();
#endif // USE_MVM_TOUR
	void WriteChallengeList();
};

#endif // TF_MVM_CRITERIA_H
