//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_BASE_BUILD_MENU
#define TF_HUD_BASE_BUILD_MENU
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "hudelement.h"
#include "c_tf_player.h"
#include "tf_weaponbase.h"

using namespace vgui;


class CHudBaseBuildMenu : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudBaseBuildMenu, EditablePanel );

public:

	CHudBaseBuildMenu( const char *pElementName, const char *pMenuName )
		: CHudElement( pElementName )
		, BaseClass( NULL, pMenuName )
	{
		m_bBuilderEquipped = false;
	}

	virtual bool ShouldDraw( void ) OVERRIDE
	{
		CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pPlayer )
			return false;

		CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();

		if ( !pWpn )
			return false;

		// Don't show the menu for first person spectator
		if ( pPlayer != pWpn->GetOwner() )
			return false;

		if ( !CHudElement::ShouldDraw() )
			return false;

		return m_bBuilderEquipped;
	}

	void SetBuilderEquipped( bool bEquipped )
	{
		m_bBuilderEquipped = bEquipped;
	}

private:

	bool m_bBuilderEquipped;
};


#endif	// TF_HUD_BASE_BUILD_MENU
