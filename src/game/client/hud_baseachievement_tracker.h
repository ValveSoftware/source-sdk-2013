//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef HUD_ACHIEVEMENT_TRACKER_H
#define HUD_ACHIEVEMENT_TRACKER_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>

class IAchievement;
namespace vgui
{
	class ImagePanel;
};

//-----------------------------------------------------------------------------
// Purpose:  Draws information about one achievement (name, description, progress)
//-----------------------------------------------------------------------------
class CAchievementTrackerItem : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CAchievementTrackerItem, vgui::EditablePanel );
public:
	CAchievementTrackerItem( vgui::Panel* pParent, const char *pElementName );
	virtual ~CAchievementTrackerItem();

	virtual void ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void SetAchievement( IAchievement* pAchievement );
	virtual void OnThink();
	virtual void SetSlot( int i ) { m_iSlot = i; }
	virtual void PerformLayout();
	virtual void AchievementIncremented( int iCount );
	virtual int  GetAchievementID() { return m_iAchievementID; }
	virtual float GetGlow() { return m_flGlow; }
	virtual void ShowAccumulatedIncrements();
	virtual int GetLastCount() { return m_iLastCount; }
	virtual void UpdateAchievementDisplay();

protected:	
	vgui::Label *m_pAchievementName;
	vgui::Label *m_pAchievementNameGlow;
	vgui::Label *m_pAchievementDesc;
	vgui::ImagePanel		*m_pProgressBarBackground;
	vgui::ImagePanel		*m_pProgressBar;

	int m_iAchievementID;
	int m_iLastPaintedAchievementID;
	int m_iLastProgressBarGoal, m_iLastProgressBarCount, m_iLastCount;
	int m_iSlot;
	float m_flGlowTime;
	float m_flGlow;
	float m_flShowIncrementsTime;
	int m_iAccumulatedIncrement;

	CPanelAnimationVarAliasType( int, m_iPadding, "Padding", "1", "proportional_int" );		// space between description and bar
};

//---------------------------------------------------------------------------------------------
// Purpose:  Scrolls a floating number up the screen to indicate achievement progression
//---------------------------------------------------------------------------------------------

enum floating_number_directions
{
	FN_DIR_UP,
	FN_DIR_DOWN,
	FN_DIR_LEFT,
	FN_DIR_RIGHT,
};

class CFloatingAchievementNumber : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CFloatingAchievementNumber, vgui::EditablePanel );
public:
	CFloatingAchievementNumber( int iProgress, int x, int y, floating_number_directions iDir, vgui::Panel* pParent );
	virtual ~CFloatingAchievementNumber();
	
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void OnThink();

protected:
	vgui::Label *m_pNumberLabel;
	int m_iStartX;
	int m_iStartY;
	int m_iProgress;
	float m_fStartTime;
	floating_number_directions m_iDirection;

	CPanelAnimationVarAliasType( int, m_iScrollDistance, "ScrollDistance", "40", "proportional_int" );		// how far the floating number will scroll up before disappearing completely
};

//-----------------------------------------------------------------------------
// Purpose:  Creates panels to show achievements on the HUD
//-----------------------------------------------------------------------------
class CHudBaseAchievementTracker : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudBaseAchievementTracker, vgui::EditablePanel );
public:
	CHudBaseAchievementTracker( const char *pElementName );

	virtual void Reset();
	virtual void LevelInit();
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void OnThink();
	virtual bool ShouldDraw();
	virtual void PerformLayout();
	virtual bool ShouldShowAchievement( IAchievement *pAchievement );
	virtual void UpdateAchievementItems();
	virtual int  GetMaxAchievementsShown();
	virtual CAchievementTrackerItem* GetAchievementPanel( int i );
	virtual CAchievementTrackerItem* CreateAchievementPanel();

protected:
	float m_flNextThink;
	CUtlVector<CAchievementTrackerItem*> m_AchievementItem;

	CPanelAnimationVarAliasType( int, m_iItemPadding, "ItemPadding", "8", "proportional_int" );		// space between each achievement item
};

#endif	// HUD_ACHIEVEMENT_TRACKER_H
