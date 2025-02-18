//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ITEM_RENTAL_UI_H
#define ITEM_RENTAL_UI_H
#ifdef _WIN32
#pragma once
#endif

#include "econ_ui.h"
#include "vgui/ISurface.h"
#include "econ_controls.h"
#include "gc_clientsystem.h"
#include "tool_items/tool_items.h"
#include "store/store_panel.h"
#include "vgui_controls/PropertySheet.h"
#include "confirm_dialog.h"
#include "econ_notifications.h"

class CEconPreviewNotification : public CEconNotification
{
public:
	CEconPreviewNotification( uint64 ulSteamID, uint32 iItemDef );

	virtual EType NotificationType() { return eType_Trigger; }

	virtual void Trigger() {}

	int GetItemDefIndex()
	{
		return m_pItemDef->GetDefinitionIndex();
	}

public:
	const CEconItemDefinition *m_pItemDef;
};

class CEconPreviewExpiredNotification : public CEconPreviewNotification
{
public:
	CEconPreviewExpiredNotification( uint64 ulSteamID, uint32 iItemDef ) : CEconPreviewNotification( ulSteamID, iItemDef ) {}

	virtual EType NotificationType() { return eType_Trigger; }

	virtual void Trigger();
};

class CEconPreviewItemBoughtNotification : public CEconPreviewNotification
{
public:
	CEconPreviewItemBoughtNotification( uint64 ulSteamID, uint32 iItemDef ) : CEconPreviewNotification( ulSteamID, iItemDef ) {}

	virtual EType NotificationType() { return eType_Trigger; }

	virtual void Trigger()
	{
		EconUI()->OpenEconUI( ECONUI_BACKPACK );
		MarkForDeletion();
	}
};

#endif // ITEM_RENTAL_UI_H
