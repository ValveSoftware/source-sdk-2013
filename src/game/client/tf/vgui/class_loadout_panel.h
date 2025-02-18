//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CLASS_LOADOUT_PANEL_H
#define CLASS_LOADOUT_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "base_loadout_panel.h"
#include "tf_playermodelpanel.h"
#include "item_selection_panel.h"
#include <../common/GameUI/cvarslider.h>
#include <vgui/VGUI.h>
#include "vgui_controls/CheckButton.h"

#define NUM_ITEM_PANELS_IN_LOADOUT		CLASS_LOADOUT_POSITION_COUNT

class CLoadoutPresetPanel;

class CLoadoutItemOptionsPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CLoadoutItemOptionsPanel, vgui::EditablePanel );
public:
	CLoadoutItemOptionsPanel( Panel *parent, const char *pName );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );

	virtual void OnCommand( const char *command );
	virtual void OnMessage( const KeyValues* pParams, vgui::VPANEL hFromPanel );

	void SetItemSlot( loadout_positions_t eItemSlot, int iClassIndex );
	loadout_positions_t GetItemSlot() const { return m_eItemSlot; }
	void UpdateItemOptionsUI();

private:

	void AddControlsParticleEffect( void ) const;
	void AddControlsSetStyle( void ) const;
	CEconItemView* GetItem( void ) const;

	class vgui::PanelListPanel	*m_pListPanel;
	CCvarSlider					*m_pHatParticleSlider;
	CExButton					*m_pSetStyleButton;
	vgui::CheckButton			*m_pHatParticleUseHeadButton;

	int						m_iCurrentClassIndex;
	loadout_positions_t		m_eItemSlot;
};


//-----------------------------------------------------------------------------
// A loadout screen that handles modifying the loadout of a specific class
//-----------------------------------------------------------------------------
class CClassLoadoutPanel : public CBaseLoadoutPanel
{
	DECLARE_CLASS_SIMPLE( CClassLoadoutPanel, CBaseLoadoutPanel );
public:
	CClassLoadoutPanel( vgui::Panel *parent );
	~CClassLoadoutPanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void PerformLayout( void );
	virtual void FireGameEvent( IGameEvent *event );

	virtual void AddNewItemPanel( int iPanelIndex ) OVERRIDE;
	virtual void UpdateModelPanels( void );
	virtual int	 GetNumItemPanels( void ) { return NUM_ITEM_PANELS_IN_LOADOUT; };
	virtual void OnShowPanel( bool bVisible, bool bReturningFromArmory );
	virtual void PostShowPanel( bool bVisible );
	virtual void OnKeyCodePressed( vgui::KeyCode code ) OVERRIDE;
	virtual void OnNavigateTo( const char* panelName ) OVERRIDE;
	virtual void OnNavigateFrom( const char* panelName ) OVERRIDE;

	void		 SetClass( int iClass );
	void		 SetTeam( int iTeam );

	int			 GetNumRelevantSlots() const;
	CEconItemView	*GetItemInSlot( int iSlot );

	MESSAGE_FUNC_PTR( OnItemPanelMouseReleased, "ItemPanelMouseReleased", panel );
	MESSAGE_FUNC_PARAMS( OnSelectionReturned, "SelectionReturned", data );
	MESSAGE_FUNC( OnCancelSelection, "CancelSelection" );
	MESSAGE_FUNC( OnClosing, "Closing" );
	virtual void OnCommand( const char *command );
	virtual void OnMessage( const KeyValues* pParams, vgui::VPANEL hFromPanel );

	void		SetSelectionPanel( CEquipSlotItemSelectionPanel *pPanel ) { m_pSelectionPanel = pPanel; }
	void		UpdatePassiveAttributes( void );

	bool		IsInSelectionPanel() const	{ return m_pSelectionPanel != NULL; }
	CEquipSlotItemSelectionPanel	*GetItemSelectionPanel() { return m_pSelectionPanel; }

	bool		IsEditingTauntSlots() const { return m_bInTauntLoadoutMode; }

	enum classloadoutpage_t
	{
		CHARACTER_LOADOUT_PAGE,
		TAUNT_LOADOUT_PAGE
	};
	void			SetLoadoutPage( classloadoutpage_t loadoutPage );

protected:
	virtual void	SetBorderForItem( CItemModelPanel *pItemPanel, bool bMouseOver );
	void			AddAttribPassiveText( const class CEconAttributeDescription& AttrDesc, INOUT_Z_CAP(iNumPassiveChars) wchar_t *out_wszPassiveDesc, int iNumPassiveChars );
	void			RespawnPlayer();
	virtual void	ApplyKVsToItemPanels( void ) OVERRIDE;
	void			ClearItemOptionsMenu( void );
	void			SetOptionsButtonText( int nIndex, const char* pszText );
	static bool		AnyOptionsAvailableForItem( const CEconItemView *pItem );

	int						m_iCurrentClassIndex;
	int						m_iCurrentTeamIndex;
	int						m_iCurrentSlotIndex;
	bool					m_bLoadoutHasChanged;
	bool					m_bInTauntLoadoutMode;
	CTFPlayerModelPanel		*m_pPlayerModelPanel;
	CEquipSlotItemSelectionPanel	*m_pSelectionPanel;
	vgui::Label				*m_pTauntHintLabel;
	CExLabel				*m_pTauntLabel;
	CExLabel				*m_pTauntCaratLabel;
	CExLabel				*m_pPassiveAttribsLabel;
	Panel					*m_pTopLinePanel;

	CExButton				*m_pBuildablesButton;
	CExImageButton			*m_pCharacterLoadoutButton;
	CExImageButton			*m_pTauntLoadoutButton;

	CLoadoutPresetPanel		*m_pLoadoutPresetPanel;

	CExplanationPopup		*m_pPresetsExplanationPopup;
	CExplanationPopup		*m_pTauntsExplanationPopup;

	KeyValues				*m_pItemOptionPanelKVs;
	CUtlVector< CExButton * > m_vecItemOptionButtons;
	CLoadoutItemOptionsPanel *m_pItemOptionPanel;

private:
	void UpdatePageButtonColor( CExImageButton *pPageButton, bool bIsActive );

	enum PageButtonColors_t
	{
		LOADED = 0, NOTLOADED,
		FG = 0, BG,
		DEFAULT = 0, ARMED, DEPRESSED
	};
	Color					m_aDefaultColors[2][2][3];	// [LOADED|NOTLOADED][FG|BG][DEFAULT|ARMED|DEPRESSED]
};

extern CClassLoadoutPanel *g_pClassLoadoutPanel;

#endif // CLASS_LOADOUT_PANEL_H
