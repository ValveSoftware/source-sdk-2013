//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replaybrowseritemmanager.h"
#include "replaybrowserbasepage.h"
#include "replay/ireplaymoviemanager.h"
#include "replay/ireplaymanager.h"
#include "replay/ireplaymovie.h"

//-----------------------------------------------------------------------------

using namespace vgui;

//-----------------------------------------------------------------------------

extern IClientReplayContext *g_pClientReplayContext;
extern IReplayMovieManager *g_pReplayMovieManager;

//-----------------------------------------------------------------------------

class CReplayItemManager : public IReplayItemManager
{
public:
	virtual int GetItemCount()
	{
		return g_pReplayManager->GetReplayCount();
	}

	virtual void GetItems( CUtlLinkedList< IQueryableReplayItem *, int > &items )
	{
		g_pReplayManager->GetReplaysAsQueryableItems( items );
	}

	virtual IQueryableReplayItem *GetItem( ReplayItemHandle_t hItem )
	{
		return static_cast< CReplay * >( g_pReplayManager->GetReplay( (ReplayHandle_t)hItem ) );
	}

	virtual bool AreItemsMovies()
	{
		return false;
	}

	virtual void DeleteItem( Panel *pPage, ReplayItemHandle_t hItem, bool bNotifyUI )
	{
		g_pReplayManager->DeleteReplay( (ReplayHandle_t)hItem, bNotifyUI );
	}
};

//-----------------------------------------------------------------------------

class CMovieItemManager : public IReplayItemManager
{
public:
	virtual int GetItemCount()
	{
		return g_pReplayMovieManager->GetMovieCount();
	}

	virtual void GetItems( CUtlLinkedList< IQueryableReplayItem *, int > &items )
	{
		g_pReplayMovieManager->GetMoviesAsQueryableItems( items );
	}

	virtual IQueryableReplayItem *GetItem( ReplayItemHandle_t hItem )
	{
		return g_pReplayMovieManager->GetMovie( (ReplayHandle_t)hItem );
	}

	virtual bool AreItemsMovies()
	{
		return true;
	}

	virtual void DeleteItem( Panel *pPage, ReplayItemHandle_t hItem, bool bNotifyUI )
	{
		CReplayBrowserBasePage *pBasePage = static_cast< CReplayBrowserBasePage * >( pPage );

		// Free the lock so the file is deletable
		pBasePage->FreeDetailsPanelMovieLock();

		// Delete the entry & the file
		g_pReplayMovieManager->DeleteMovie( hItem );
	}
};

//-----------------------------------------------------------------------------

static CReplayItemManager s_ReplayItemManager;
static CMovieItemManager s_MovieItemManager;

//-----------------------------------------------------------------------------

IReplayItemManager *GetReplayItemManager()
{
	return &s_ReplayItemManager;
}

IReplayItemManager *GetReplayMovieItemManager()
{
	return &s_MovieItemManager;
}

IQueryableReplayItem *FindReplayItem( ReplayItemHandle_t hItem, IReplayItemManager **ppItemManager )
{
	static IReplayItemManager *s_pItemManagers[] = { &s_ReplayItemManager, &s_MovieItemManager };

	if ( ppItemManager )
	{
		*ppItemManager = NULL;
	}

	for ( int i = 0; i < 2; ++i )
	{
		IQueryableReplayItem *pItem = s_pItemManagers[ i ]->GetItem( hItem );
		if ( pItem )
		{
			if ( ppItemManager )
			{
				*ppItemManager = s_pItemManagers[ i ];
			}
			return pItem;
		}
	}
	return NULL;
}

#endif
