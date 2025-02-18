//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IQUERYABLEREPLAYITEM_H
#define IQUERYABLEREPLAYITEM_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "interface.h"
#include "iqueryablereplayitem.h"
#include "replay/replayhandle.h"
#include "replay/replaytime.h"

//----------------------------------------------------------------------------------------

class CReplay;

//----------------------------------------------------------------------------------------

typedef int QueryableReplayItemHandle_t;

//----------------------------------------------------------------------------------------

abstract_class IQueryableReplayItem : public IBaseInterface
{
public:
	virtual const CReplayTime	&GetItemDate() const = 0;
	virtual bool				IsItemRendered() const = 0;
	virtual CReplay				*GetItemReplay() = 0;
	virtual ReplayHandle_t		GetItemReplayHandle() const = 0;
	virtual QueryableReplayItemHandle_t	GetItemHandle() const = 0;	// Get the handle of this item
	virtual const wchar_t		*GetItemTitle() const = 0;
	virtual void				SetItemTitle( const wchar_t *pTitle ) = 0;
	virtual float				GetItemLength() const = 0;
	virtual void				*GetUserData() = 0;
	virtual void				SetUserData( void *pUserData ) = 0;
	virtual bool				IsItemAMovie() const = 0;
};

//----------------------------------------------------------------------------------------

#endif // IQUERYABLEREPLAYITEM_H
