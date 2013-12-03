//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef REPLAYBROWSER_ITEMMANAGER_H
#define REPLAYBROWSER_ITEMMANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "utllinkedlist.h"
#include <vgui_controls/Panel.h>

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
typedef int ReplayItemHandle_t;

//-----------------------------------------------------------------------------
// Purpose: Layer of abstraction between UI and replay demos or rendered movies
//-----------------------------------------------------------------------------
class IQueryableReplayItem;

abstract_class IReplayItemManager : public IBaseInterface
{
public:
	virtual int						GetItemCount() = 0;
	virtual void					GetItems( CUtlLinkedList< IQueryableReplayItem *, int > &items ) = 0;
	virtual IQueryableReplayItem	*GetItem( ReplayItemHandle_t hItem ) = 0;
	virtual bool					AreItemsMovies() = 0;
	virtual void					DeleteItem( vgui::Panel *pPage, ReplayItemHandle_t hItem, bool bNotifyUI ) = 0;
};

IReplayItemManager *GetReplayItemManager();
IReplayItemManager *GetReplayMovieItemManager();

// Find an item and put the item manager in ppItemManager
IQueryableReplayItem *FindReplayItem( ReplayItemHandle_t hItem, IReplayItemManager **ppItemManager );

#endif // REPLAYBROWSER_ITEMMANAGER_H
