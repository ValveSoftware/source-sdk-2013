//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "rtime.h"
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/TextEntry.h"
#include "vgui/IInput.h"
#include "econ_item_system.h"
#include "econ_item_constants.h"
#include "econ_gcmessages.h"
#include "econ_item_inventory.h"
#include "item_rental_ui.h"

#ifdef TF_CLIENT_DLL
#include "c_tf_gamestats.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

CEconPreviewNotification::CEconPreviewNotification( uint64 ulSteamID, uint32 iItemDef ) 
	: CEconNotification() 
{
	SetSteamID( ulSteamID );
	SetLifetime( 20.0f );

	m_pItemDef = GetItemSchema()->GetItemDefinition( iItemDef );
	if ( !m_pItemDef )
		return;

	AddStringToken( "item_name",  g_pVGuiLocalize->Find(m_pItemDef->GetItemBaseName()) );
}

void CEconPreviewExpiredNotification::Trigger()
{
}