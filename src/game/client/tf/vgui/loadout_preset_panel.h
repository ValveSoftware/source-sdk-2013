//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef LOADOUT_PRESET_PANEL_H
#define LOADOUT_PRESET_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/PHandle.h"
#include "class_loadout_panel.h"

class CExButton;
class CSelectedItemPreset;

//-----------------------------------------------------------------------------
// A loadout preset panel, which allows combinations of items to be saved and
// restored via the GC.
//-----------------------------------------------------------------------------
class CLoadoutPresetPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CLoadoutPresetPanel, vgui::EditablePanel );
public:
	CLoadoutPresetPanel( vgui::Panel *pParent, const char *pName );	// name is ignored but needed for DECLARE_BUILD_FACTORY()

	void			SetClass( int iClass );
	void			EnableVerticalDisplay( bool bVertical );

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	ApplySettings( KeyValues *pInResourceData );
	virtual void	PerformLayout();
	virtual void	OnCommand( const char *command );
	virtual void	OnTick() OVERRIDE;

	bool			HandlePresetKeyPressed( vgui::KeyCode code );

	void			SetClassLoadoutPanel(CClassLoadoutPanel* pPanel) {
		m_pClassLoadoutPanel = pPanel;
	}

private:
	equipped_preset_t	GetSelectedPresetID() const;
	void				UpdatePresetButtonStates();
	void				LoadPreset( int iPresetIndex );

	enum PresetsConsts_t
	{
		MAX_PRESETS = 4,
	};

	int						m_iClass;
	KeyValues				*m_pPresetButtonKv;
	CExButton				*m_pPresetButtons[ MAX_PRESETS ];
	bool					m_bDisplayVertical;
	CClassLoadoutPanel*		m_pClassLoadoutPanel;

	enum PresetButtonColors_t
	{
		LOADED = 0, NOTLOADED,
		FG = 0, BG,
		DEFAULT = 0, ARMED, DEPRESSED
	};
	Color					m_aDefaultColors[2][2][3];	// [LOADED|NOTLOADED][FG|BG][DEFAULT|ARMED|DEPRESSED]
};


#endif // LOADOUT_PRESET_PANEL_H
