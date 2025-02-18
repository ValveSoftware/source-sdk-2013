//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Load item upgrade data from KeyValues
//
// $NoKeywords: $
//=============================================================================

#ifndef C_TF_UPGRADES_H
#define C_TF_UPGRADES_H


#include "c_baseentity.h"
#include "networkvar.h"
#include "econ_item_constants.h"
#include "tf_shareddefs.h"
#include "c_tf_player.h"
#include "hudelement.h"
#include "vgui_controls/EditablePanel.h"
#include "tf_controls.h"

#define MAX_ITEM_SLOT_BUY_PANELS 6

class CItemModelPanel;
class CImageButton;

namespace vgui
{
	class ImagePanel;
	class Button;
}

enum costlabel_chache_t
{
	CLCACHE_DIRTY,
	CLCACHE_NOT_AFFORDABLE_1,
	CLCACHE_NOT_AFFORDABLE_2,
	CLCACHE_NOT_AFFORDABLE_3,
	CLCACHE_NOT_AFFORDABLE_4,
	CLCACHE_NOT_AFFORDABLE_5,
	CLCACHE_NOT_AFFORDABLE_6,
	CLCACHE_NOT_AFFORDABLE_7,
	CLCACHE_NOT_AFFORDABLE_8,
	CLCACHE_NOT_AFFORDABLE_9,
	CLCACHE_AFFORDABLE,
};


//-----------------------------------------------------------------------------
// Purpose: HUD Element that provides the interface to the upgrade options
//-----------------------------------------------------------------------------
class CUpgradeBuyPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CUpgradeBuyPanel, vgui::EditablePanel );

public:

	enum ColorSet
	{
		COLOR_SET_DEFAULT,
		COLOR_SET_OWNED,
		COLOR_SET_PURCHASED,
		COLOR_SET_DISABLED,
	};

public:
	CUpgradeBuyPanel( Panel *parent, const char *panelName );
	virtual ~CUpgradeBuyPanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout( void );
	virtual void OnCommand( const char *command );

	bool ValidateUpgradeStepData( void );

	void SetNumLevelImages( int nValues );
	void SetSkillTreeButtonColors( int nButton, ColorSet nColorSet );
	void SetInspectMode( bool bValue ) { m_bInspectMode = bValue; }
	void SetPlayer( C_TFPlayer *pPlayer ) { m_hPlayer = pPlayer; }

	void UpdateImages( int nCurrentMoney );

public:

	KeyValues *m_pSkillTreeButtonKVs;	

	vgui::ImagePanel *m_pIcon;
	vgui::Label	*m_pPriceLabel;
	vgui::Label	*m_pShortDescriptionLabel;
	CImageButton *m_pIncrementButton;
	CImageButton *m_pDecrementButton;
	CUtlVector< vgui::ImagePanel* > m_SkillTreeImages;

	int m_nWeaponSlot;
	int m_nUpgradeIndex;
	int m_nPrice;

	int m_nGridPositionX;
	int m_nGridPositionY;

	int m_nCurrentStep;
	int m_nPurchases;

	bool m_bOverCap;
	char m_szAttribName[MAX_ATTRIBUTE_DESCRIPTION_LENGTH];

	bool m_bInspectMode;

	CPanelAnimationVarAliasType( int, m_iUpgradeButtonXPos, "upgradebutton_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iUpgradeButtonYPos, "upgradebutton_ypos", "0", "proportional_int" );

	static Color m_rgbaDefaultFG;
	static Color m_rgbaDefaultBG;
	static Color m_rgbaArmedFG;
	static Color m_rgbaArmedBG;
	static Color m_rgbaDepressedFG;
	static Color m_rgbaDepressedBG;
	static Color m_rgbaSelectedFG;
	static Color m_rgbaSelectedBG;
	static Color m_rgbaDisabledFG;
	static Color m_rgbaDisabledBG;

private:
	CHandle< C_TFPlayer > m_hPlayer;
};


struct ItemSlotBuyPanels
{
	static const int CHARACTER_UPGRADE = -1;
	static const int INVALID_SLOT = -2;

	typedef CUpgradeBuyPanel *UPGRADEPTR;
	class CUpgradeBuyPanelLess
	{
	public:
		bool Less( const UPGRADEPTR &src1, const UPGRADEPTR &src2, void *pCtx )
		{
			if ( src1->m_nPrice > src2->m_nPrice )
				return true;

			if ( src1->m_nPrice == src2->m_nPrice && src1->m_nUpgradeIndex < src2->m_nUpgradeIndex )
				return true;

			return false;
		}
	};

	ItemSlotBuyPanels()
	{
		nSlot = INVALID_SLOT;
		m_iItemID = INVALID_ITEM_ID;
	}

	void SetItemID(itemid_t iIndex ) { m_iItemID = iIndex; }
	itemid_t GetItemID( void ) { return m_iItemID; }

	int nSlot;
	CUtlSortVector< CUpgradeBuyPanel*, CUpgradeBuyPanelLess >	upgradeBuyPanels;
	itemid_t m_iItemID;
};


//-----------------------------------------------------------------------------
// Purpose: HUD Element that provides the interface to the upgrade options
//-----------------------------------------------------------------------------
class CHudUpgradePanel : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudUpgradePanel, vgui::EditablePanel );

public:
	CHudUpgradePanel( const char *pElementName );
	virtual ~CHudUpgradePanel();

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	ApplySettings( KeyValues *inResourceData );
	virtual void	PerformLayout( void );
	virtual bool	ShouldDraw( void );
	virtual void	SetVisible( bool bVisible );
	virtual void	SetActive( bool bActive );
	virtual int		GetRenderGroupPriority( void ) { return 35; }	// less than statpanel
	virtual void	OnCommand( const char *command );
	virtual void	OnTick( void );
	virtual void	FireGameEvent( IGameEvent *event );
	void			InspectUpgradesForPlayer( C_TFPlayer *pPlayer ) { m_hPlayer = pPlayer; m_bInspectMode = true; m_bShowUpgradeMenu = true; }
	C_TFPlayer		*GetPlayer( void ) { return m_hPlayer; }
	void			PlayerInventoryChanged( C_TFPlayer *pPlayer );

	MESSAGE_FUNC_PTR( OnItemPanelEntered, "ItemPanelEntered", panel );
	MESSAGE_FUNC_PTR( OnItemPanelExited, "ItemPanelExited", panel );
	MESSAGE_FUNC_PTR( OnItemPanelMousePressed, "ItemPanelMousePressed", panel );

	virtual GameActionSet_t GetPreferredActionSet() { return IsActive() ? GAME_ACTION_SET_MENUCONTROLS : GAME_ACTION_SET_NONE; }

protected:
	void			CreateItemModelPanel( int iLoadoutSlot );
	void			UpdateModelPanels( void );
	virtual void	SetBorderForItem( CItemModelPanel *pItemPanel, bool bMouseOver );
	void			UpgradeItemInSlot( int iSlot );
	void			UpdateUpgradeButtons( void );
	void			UpdateButtonStates( int nCurrentCurrency, int nUpgrade = 0, int nNumPurchased = 0 );
	void			UpdateJoystickControls( void );
	void			UpdateHighlights( void );
	void			UpdateMouseOverHighlight( void );

	void			UpdateItemStatsLabel( void );
	void			CancelUpgrades( void );
	void			AddItemStatText( const locchar_t *loc_AttrDescText, attrib_colors_t eColor, wchar_t *out_wszAttribDesc, int iAttribDescSize );
	CEconItemView*	GetLocalPlayerBottleFromInventory( void );
	bool			QuickEquipBottle( void );

protected:
	vgui::EditablePanel *m_pTipPanel;
	vgui::EditablePanel	*m_pSelectWeaponPanel;
	CExLabel *m_pUpgradeItemStatsLabel;

	vgui::Panel *m_pPlayerUpgradeButton;
	vgui::Panel *m_pActiveTabPanel;
	vgui::Panel *m_pMouseOverTabPanel;
	vgui::Panel *m_pMouseOverUpgradePanel;
	CUpgradeBuyPanel *m_pActiveUpgradeBuyPanel;
	vgui::Panel *m_pPlayerRespecButton;

	CUtlVector< CItemModelPanel* > m_pItemPanels;
	KeyValues			*m_pItemModelPanelKVs;	
	int					m_iVisibleItemPanels;

	int		m_iWeaponSlotBeingUpgraded;
	bool	m_bShowUpgradeMenu;
	bool	m_bCancelUpgrades;
	bool	m_bOpenLoadout;
	bool	m_bWasInZone;
	bool	m_bHighlightedTab;
	bool	m_bInspectMode;

	int		m_nCurrency;
	int		m_nUpgradeActivity;

	bool	m_bAwardMaxSlotAchievement;
	bool	m_bAwardMaxResistAchievement;

	ItemSlotBuyPanels	m_ItemSlotBuyPanels[ MAX_ITEM_SLOT_BUY_PANELS ];

	CPanelAnimationVarAliasType( int, m_iItemPanelXPos, "itempanel_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iItemPanelYPos, "itempanel_ypos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iItemPanelXDelta, "itempanel_xdelta", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iItemPanelYDelta, "itempanel_ydelta", "0", "proportional_int" );

	CPanelAnimationVarAliasType( int, m_iUpgradeBuyPanelXPos, "upgradebuypanel_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iUpgradeBuyPanelYPos, "upgradebuypanel_ypos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iUpgradeBuyPanelDelta, "upgradebuypanel_delta", "0", "proportional_int" );

	bool m_bNavUpDownPressed;
	bool m_bNavLeftRightPressed;
	bool m_bNavButtonPressed;
	bool m_bUsingController;

private:
	void UpdateTip();
	CHandle< C_TFPlayer > m_hPlayer;
};


extern bool MannVsMachine_GetUpgradeInfo( int iAttribute, int iQuality, float &flValue );


#endif // C_TF_UPGRADES_H
