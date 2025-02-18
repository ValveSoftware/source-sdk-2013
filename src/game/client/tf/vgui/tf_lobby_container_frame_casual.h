//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef TF_LOBBY_CONTAINER_FRAME_CASUAL_H
#define TF_LOBBY_CONTAINER_FRAME_CASUAL_H


#include "cbase.h"
//#include "tf_pvelobbypanel.h"
#include "game/client/iviewport.h"
#include "tf_shareddefs.h"
#include "econ/confirm_dialog.h"
#include "econ/econ_controls.h"
#include "ienginevgui.h"
#include "tf_gc_client.h"
#include "tf_party.h"
#include "tf_item_inventory.h"
#include "econ_ui.h"

#include "vgui_controls/Tooltip.h"
#include "vgui_controls/PropertyDialog.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/RadioButton.h"
#include "vgui_controls/SectionedListPanel.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "vgui_bitmapimage.h"
#include "vgui/IInput.h"
#include <vgui_controls/ImageList.h>
#include <vgui/IVGui.h>
#include "GameEventListener.h"
#include "vgui_avatarimage.h"
#include <vgui/ISurface.h>
#include <VGuiMatSurface/IMatSystemSurface.h>
#include "rtime.h"
#include "econ_game_account_client.h"
#include "tf_leaderboardpanel.h"
#include "tf_mapinfo.h"
#include "tf_ladder_data.h"
#include "tf_gamerules.h"
#include "confirm_dialog.h"
#include "tf_lobby_container_frame.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

class CBaseLobbyPanel;

// This is a big fat kludge so I can use the PropertyPage
class CLobbyContainerFrame_Casual : public CBaseLobbyContainerFrame
{
	DECLARE_CLASS_SIMPLE( CLobbyContainerFrame_Casual, CBaseLobbyContainerFrame );
public:
	CLobbyContainerFrame_Casual();
	~CLobbyContainerFrame_Casual();

	//
	// PropertyDialog overrides
	//
	virtual void ShowPanel( bool bShow ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;

	CMainMenuToolTip *GetTooltipPanel(){ return m_pToolTip; }

	static bool TypeCanHandleMatchGroup( ETFMatchGroup eMatchGroup );
	virtual bool CanHandleMatchGroup( ETFMatchGroup eMatchGroup ) const OVERRIDE
	{
		return TypeCanHandleMatchGroup( eMatchGroup );
	}

protected:

	virtual void WriteControls() OVERRIDE;

private:
	virtual const char* GetResFile() const OVERRIDE { return "Resource/UI/LobbyContainerFrame_Casual.res"; }
	virtual bool VerifyPartyAuthorization() const OVERRIDE;
	virtual void HandleBackPressed() OVERRIDE;


	CMainMenuToolTip *m_pToolTip;
};

#endif //TF_LOBBY_CONTAINER_FRAME_CASUAL_H
