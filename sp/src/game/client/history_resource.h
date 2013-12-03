//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Item pickup history displayed onscreen when items are picked up.
//
// $NoKeywords: $
//=============================================================================//

#ifndef HISTORY_RESOURCE_H
#define HISTORY_RESOURCE_H
#pragma once

#include "hudelement.h"
#include "ehandle.h"

#include <vgui_controls/Panel.h>

enum 
{
	HISTSLOT_EMPTY,
	HISTSLOT_AMMO,
	HISTSLOT_WEAP,
	HISTSLOT_ITEM,
	HISTSLOT_AMMODENIED,
};

namespace vgui
{
	class IScheme;
}

class C_BaseCombatWeapon;

//-----------------------------------------------------------------------------
// Purpose: Used to draw the history of ammo / weapon / item pickups by the player
//-----------------------------------------------------------------------------
class CHudHistoryResource : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudHistoryResource, vgui::Panel );
private:
	struct HIST_ITEM 
	{
		HIST_ITEM() 
		{ 
			// init this here, because the code that overwrites previous history items will use this
			// to check to see if the item is empty
			DisplayTime = 0.0f; 
		}
		int type;
		float DisplayTime;  // the time at which this item should be removed from the history
		int iCount;
		int iId;
		CHandle< C_BaseCombatWeapon > m_hWeapon;

		CHudTexture *icon;
	};

	CUtlVector<HIST_ITEM> m_PickupHistory;

public:

	CHudHistoryResource( const char *pElementName );

	// CHudElement overrides
	virtual void Init( void );
	virtual void Reset( void );
	virtual bool ShouldDraw( void );
	virtual void Paint( void );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	void	AddToHistory( int iType, int iId, int iCount = 0 );
	void	AddToHistory( int iType, const char *szName, int iCount = 0 );
	void	AddToHistory( C_BaseCombatWeapon *weapon );
	void	MsgFunc_ItemPickup( bf_read &msg );
	void	MsgFunc_AmmoDenied( bf_read &msg );
	
	void	CheckClearHistory( void );
	void	SetHistoryGap( int iNewHistoryGap );
	void	AddIconToHistory( int iType, int iId, C_BaseCombatWeapon *weapon, int iCount, CHudTexture *icon );

private:
	// these vars are for hl1-port compatibility
	int		m_iHistoryGap;
	int		m_iCurrentHistorySlot;
	bool	m_bDoNotDraw;
	wchar_t m_wcsAmmoFullMsg[16];
	bool	m_bNeedsDraw;

	CPanelAnimationVarAliasType( float, m_flHistoryGap, "history_gap", "42", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flIconInset, "icon_inset", "28", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flTextInset, "text_inset", "26", "proportional_float" );
	CPanelAnimationVar( vgui::HFont, m_hNumberFont, "NumberFont", "HudNumbersSmall" );
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );
};

#endif // HISTORY_RESOURCE_H
