//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#ifndef TF_LOBBYPANEL_COMP_H
#define TF_LOBBYPANEL_COMP_H

#include "cbase.h"
#include "game/client/iviewport.h"
#include "tf_lobbypanel.h"
#include "tf_leaderboardpanel.h"
#include "local_steam_shared_object_listener.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace GCSDK;

class CBaseLobbyPanel;

namespace vgui
{
	class ScrollableEditablePanel;
};


class CLobbyPanel_Comp : public CBaseLobbyPanel, public CLocalSteamSharedObjectListener
{
	DECLARE_CLASS_SIMPLE( CLobbyPanel_Comp, CBaseLobbyPanel );
public:

	CLobbyPanel_Comp( vgui::Panel *pParent, CBaseLobbyContainerFrame* pLobbyContainer );
	virtual ~CLobbyPanel_Comp();

	virtual void ApplyChatUserSettings( const LobbyPlayerInfo &player,KeyValues *pKV ) const OVERRIDE;
	virtual const char* GetResFile() const OVERRIDE { return "Resource/UI/LobbyPanel_Comp.res"; }
	virtual EMatchGroup GetMatchGroup( void ) const OVERRIDE { return k_nMatchGroup_Ladder_6v6; }
	virtual bool ShouldShowLateJoin() const OVERRIDE { return false; }
	void WriteGameSettingsControls() OVERRIDE;

private:
	class CCompStatsPanel* m_pCompStats;
};

#endif //TF_LOBBYPANEL_COMP_H
