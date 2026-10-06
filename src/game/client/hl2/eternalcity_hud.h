//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef ETERNALCITY_HUD_H
#define ETERNALCITY_HUD_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include "hud_element_helper.h"
#include <vgui_controls/Panel.h>
#include <vgui/VGUI.h>
#include <Color.h>

namespace vgui
{
	class IScheme;
}

class CEternalCityHud : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CEternalCityHud, vgui::Panel );

public:
	DECLARE_MULTIPLY_INHERITED();
	explicit CEternalCityHud( const char *pElementName );

	virtual void LevelShutdown();
	void ToggleCharacter();
	void ToggleEquipment();
	void ToggleInventory();
	bool IsModalOpen() const;
	void CloseModal();

protected:
	virtual void Paint();
	virtual bool ShouldDraw();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();
	virtual void OnMousePressed( vgui::MouseCode code );
	virtual void OnKeyCodePressed( vgui::KeyCode code );

private:
	enum PanelType_t
	{
		PANEL_NONE = 0,
		PANEL_CHARACTER,
		PANEL_EQUIPMENT,
		PANEL_INVENTORY
	};

	void TogglePanel( PanelType_t panel );
	void DrawText( int x, int y, const char *text, const Color &color );
	void DrawRect( int x, int y, int width, int height, const Color &color );
	void DrawModal();
	void DrawVitals();
	void DrawActionButton( int x, int y, int width, const char *label, bool enabled );

	PanelType_t m_PanelType;
	bool m_bPrototypeVestEquipped;
	int m_iMockMedkits;
	vgui::HFont m_hFont;
};

#endif
