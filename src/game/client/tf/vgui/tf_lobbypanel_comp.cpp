//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"

#include "tf_party.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/ScrollableEditablePanel.h"

#include "tf_lobbypanel_comp.h"
#include "tf_lobby_container_frame_comp.h"
#include "tf_comp_stats.h"

#include "vgui/ISystem.h"

#include "tf_streams.h"
#include "tf_badge_panel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

class CCompStatsPanel;

#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>

CLobbyPanel_Comp::CLobbyPanel_Comp( vgui::Panel *pParent, CBaseLobbyContainerFrame* pLobbyContainer )
	: BaseClass( pParent, pLobbyContainer )
{
	m_pCompStats = new CCompStatsPanel( this, "stats" );
}

CLobbyPanel_Comp::~CLobbyPanel_Comp()
{
	m_pCompStats->MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLobbyPanel_Comp::ApplyChatUserSettings( const CBaseLobbyPanel::LobbyPlayerInfo &player, KeyValues *pKV ) const
{
	pKV->SetInt( "has_ticket", 0 );
	pKV->SetInt( "squad_surplus", 0 );
}


void CLobbyPanel_Comp::WriteGameSettingsControls()
{
	m_pCompStats->WriteControls();
}
