//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_BUILDING_STATUS_H
#define TF_HUD_BUILDING_STATUS_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include "tf_controls.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui/IScheme.h>
#include <vgui_controls/ProgressBar.h>
#include "utlpriorityqueue.h"

class C_BaseObject;
class CIconPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBuildingHealthBar : public vgui::ProgressBar
{
	DECLARE_CLASS_SIMPLE( CBuildingHealthBar, vgui::ProgressBar );

public:
	CBuildingHealthBar(Panel *parent, const char *panelName);

	virtual void Paint();
	virtual void PaintBackground();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

private:
	Color m_cHealthColor;
	Color m_cLowHealthColor;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBuildingStatusAlertTray : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CBuildingStatusAlertTray, vgui::Panel );

public:
	CBuildingStatusAlertTray(Panel *parent, const char *panelName);

	void ApplySettings( KeyValues *inResourceData );

	virtual void Paint( void );
	virtual void PaintBackground( void );

	void LevelInit( void );

	void ShowTray( void );
	void HideTray( void );

	bool IsTrayOut( void ) { return m_bIsTrayOut; }

	void SetAlertType( BuildingHudAlert_t alertLevel );

	float GetPercentDeployed( void ) { return m_flAlertDeployedPercent; }
	BuildingHudAlert_t GetAlertType( void ) { return m_lastAlertType; }

private:
	bool m_bIsTrayOut;
	bool m_bUseTallImage;

	CHudTexture *m_pAlertPanelHudTexture;
	IMaterial *m_pAlertPanelMaterial;

	BuildingHudAlert_t m_lastAlertType;

	CPanelAnimationVar( float, m_flAlertDeployedPercent, "deployed", "0.0" );

};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBuildingStatusItem : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CBuildingStatusItem, vgui::EditablePanel );

public:

	// actual panel constructor
	CBuildingStatusItem( Panel *parent, const char *szLayout, int iObjectType, int iObjectMode );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Paint( void );
	virtual void PaintBackground( void );
	virtual void OnTick( void );

	virtual void PerformLayout( void );

	virtual void LevelInit( void );

	bool HasBeenPositioned() const { return bPositioned; }
	void SetPositioned(bool val) { bPositioned = val; }

	int GetRepresentativeObjectType();
	int GetRepresentativeObjectMode();
	C_BaseObject *GetRepresentativeObject();

	virtual int GetObjectPriority();

	virtual const char *GetBackgroundImage( void );
	virtual const char *GetInactiveBackgroundImage( void );

	vgui::EditablePanel *GetBuiltPanel() { return m_pBuiltPanel; }
	vgui::EditablePanel *GetNotBuiltPanel() { return m_pNotBuiltPanel; }

	vgui::EditablePanel *GetBuildingPanel() { return m_pBuildingPanel; }
	vgui::EditablePanel *GetRunningPanel() { return m_pRunningPanel; }

	virtual bool IsRealObject( void ) { return true; }

	void SetObject( C_BaseObject *pObj );

	bool IsActive( void ) { return m_bActive; }

private:

	bool bPositioned;		// false if we have not yet faded in and been positioned

	char m_szLayout[128];

	int m_iObjectType;
	int m_iObjectMode;
	bool m_bActive;

	// Two main subpanels
	vgui::EditablePanel *m_pNotBuiltPanel;
	vgui::EditablePanel *m_pBuiltPanel;

	// Subpanels of the m_pBuiltPanel
	vgui::EditablePanel *m_pBuildingPanel;		// subpanel shown while building
	vgui::EditablePanel *m_pRunningPanel;		// subpanel shown while built and running
	vgui::ProgressBar *m_pHealthBar;			// health bar element

	CHandle<C_BaseObject> m_pObject;			// pointer to the object we represent

	// Alert side panel
	CBuildingStatusAlertTray *m_pAlertTray;
	CIconPanel *m_pWrenchIcon;
	CIconPanel *m_pSapperIcon;

	CIconPanel *m_pUpgradeIcons[3];

	int m_iUpgradeLevel;

	// children of buildingPanel
	vgui::ContinuousProgressBar *m_pBuildingProgress;

	// elements that are always on

	// background
	CIconPanel *m_pBackground;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBuildingStatusItem_SentryGun : public CBuildingStatusItem
{
	DECLARE_CLASS_SIMPLE( CBuildingStatusItem_SentryGun, CBuildingStatusItem );

public:
	CBuildingStatusItem_SentryGun( Panel *parent );

	virtual void OnTick( void );
	virtual void PerformLayout( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

	virtual const char *GetBackgroundImage( void );
	virtual const char *GetInactiveBackgroundImage( void );

private:

	CIconPanel *m_pSentryIcons[3];

	vgui::ImagePanel *m_pRocketIcon;
	CIconPanel *m_pUpgradeIcon;

	vgui::ContinuousProgressBar *m_pShellsProgress;
	vgui::ContinuousProgressBar *m_pRocketsProgress;
	vgui::ContinuousProgressBar *m_pUpgradeProgress;

	int m_iUpgradeLevel;

	// Ammo
	Color m_cLowAmmoColor;
	Color m_cNormalAmmoColor;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBuildingStatusItem_SentryGun_Disposable : public CBuildingStatusItem
{
	DECLARE_CLASS_SIMPLE( CBuildingStatusItem_SentryGun_Disposable, CBuildingStatusItem );

public:
	CBuildingStatusItem_SentryGun_Disposable( Panel *parent );

	virtual void OnTick( void );
	virtual void PerformLayout( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

	virtual const char *GetBackgroundImage( void );
	virtual const char *GetInactiveBackgroundImage( void );

private:

	CIconPanel *m_pSentryIcons[3];
	CIconPanel *m_pUpgradeIcon;

	vgui::ContinuousProgressBar *m_pShellsProgress;

	int m_iUpgradeLevel;

	// Ammo
	Color m_cLowAmmoColor;
	Color m_cNormalAmmoColor;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBuildingStatusItem_Dispenser : public CBuildingStatusItem
{
	DECLARE_CLASS_SIMPLE( CBuildingStatusItem_Dispenser, CBuildingStatusItem );

public:
	CBuildingStatusItem_Dispenser( Panel *parent );

	virtual void PerformLayout( void );

private:

	CIconPanel *m_pUpgradeIcon;

	vgui::ContinuousProgressBar *m_pAmmoProgress;
	vgui::ContinuousProgressBar *m_pUpgradeProgress;

};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBuildingStatusItem_TeleporterEntrance : public CBuildingStatusItem
{
	DECLARE_CLASS_SIMPLE( CBuildingStatusItem_TeleporterEntrance, CBuildingStatusItem );

public:
	CBuildingStatusItem_TeleporterEntrance( Panel *parent );
	virtual void OnTick( void );
	virtual void PerformLayout( void );

private:

	// 2 subpanels
	vgui::EditablePanel *m_pChargingPanel;
	vgui::EditablePanel *m_pFullyChargedPanel;

	// children of m_pChargingPanel
	vgui::ContinuousProgressBar *m_pRechargeTimer;

	// local state
	int m_iTeleporterState;
	int m_iTimesUsed;

	CIconPanel *m_pUpgradeIcon;

	vgui::ContinuousProgressBar *m_pUpgradeProgress;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBuildingStatusItem_TeleporterExit : public CBuildingStatusItem
{
	DECLARE_CLASS_SIMPLE( CBuildingStatusItem_TeleporterExit, CBuildingStatusItem );

public:
	CBuildingStatusItem_TeleporterExit( Panel *parent );
	virtual void PerformLayout( void );

private:

	CIconPanel *m_pUpgradeIcon;
	vgui::ContinuousProgressBar *m_pUpgradeProgress;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBuildingStatusItem_Sapper : public CBuildingStatusItem
{
	DECLARE_CLASS_SIMPLE( CBuildingStatusItem_Sapper, CBuildingStatusItem );

public:
	CBuildingStatusItem_Sapper( Panel *parent );

	virtual void PerformLayout( void );

private:
	// Health of target building
	vgui::ContinuousProgressBar *m_pTargetHealthBar;

	// image of target building 
	CIconPanel *m_pTargetIcon;

	int m_iTargetType;
};

//-----------------------------------------------------------------------------
// Purpose: Container panel for object status panels
//-----------------------------------------------------------------------------
class CHudBuildingStatusContainer : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudBuildingStatusContainer, vgui::Panel );

public:
	CHudBuildingStatusContainer( const char *pElementName );

	virtual bool ShouldDraw( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void OnTick( void );

	virtual void LevelInit( void );

	void AddBuildingPanel( int iBuildingType, int iBuildingMode=0 );
	CBuildingStatusItem *CreateItemPanel( int iObjectType, int iObjectMode );

	void UpdateAllBuildings( void );
	void OnBuildingChanged( int iBuildingType, int iBuildingMode, bool bBuildingIsDead );

	void RepositionObjectPanels();

	void FireGameEvent( IGameEvent *event );

	void RecalculateAlertState( void );

protected:

	// a list of CBuildingStatusItems that we're showing
	CUtlPriorityQueue< CBuildingStatusItem * > m_BuildingPanels;

private:

	BuildingHudAlert_t m_AlertLevel;
	float m_flNextBeep;
	int m_iNumBeepsToBeep;
};

//-----------------------------------------------------------------------------
// Purpose: Separate panels for spy
//-----------------------------------------------------------------------------
class CHudBuildingStatusContainer_Spy : public CHudBuildingStatusContainer
{
	DECLARE_CLASS_SIMPLE( CHudBuildingStatusContainer_Spy, CHudBuildingStatusContainer );

public:
	CHudBuildingStatusContainer_Spy( const char *pElementName );

	virtual bool ShouldDraw( void );
};

//-----------------------------------------------------------------------------
// Purpose: Separate panels for engineer
//-----------------------------------------------------------------------------
class CHudBuildingStatusContainer_Engineer : public CHudBuildingStatusContainer
{
	DECLARE_CLASS_SIMPLE( CHudBuildingStatusContainer_Engineer, CHudBuildingStatusContainer );

public:
	CHudBuildingStatusContainer_Engineer( const char *pElementName );

	virtual bool ShouldDraw( void );

	virtual void OnTick( void );
};

#endif //TF_HUD_BUILDING_STATUS_H
