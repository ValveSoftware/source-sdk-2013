//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IREPLAYMOVIEMANAGER_H
#define IREPLAYMOVIEMANAGER_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "interface.h"
#include "utlstring.h"
#include "utllinkedlist.h"
#include "replay/replayhandle.h"

//----------------------------------------------------------------------------------------

class IReplayMovie;
class CReplay;
struct RenderMovieParams_t;
class IQueryableReplayItem;

//----------------------------------------------------------------------------------------

abstract_class IReplayMovieManager : public IBaseInterface
{
public:
	virtual int				GetMovieCount() = 0;
	virtual void			GetMovieList( CUtlLinkedList< IReplayMovie * > &list ) = 0;	// Fills the list with all movies
	virtual IReplayMovie	*GetMovie( ReplayHandle_t hMovie ) = 0;
	virtual IReplayMovie	*CreateAndAddMovie( ReplayHandle_t hReplay ) = 0;	// Creates and adds a movie based on the given replay
	virtual void			DeleteMovie( ReplayHandle_t hMovie ) = 0;	// Delete a movie
	virtual int				GetNumMoviesDependentOnReplay( const CReplay *pReplay ) = 0;	// Get the number of movies that depend on the given replay
	virtual void			GetCachedMovieTitleAndClear( wchar_t *pOut, int nOutBufLength ) = 0;	// TODO: This is a hack - fix this
	virtual void			SetPendingMovie( IReplayMovie *pMovie ) = 0;
	virtual IReplayMovie	*GetPendingMovie() = 0;
	virtual void			FlagMovieForFlush( IReplayMovie *pMovie, bool bImmediate ) = 0;
	virtual void			GetMoviesAsQueryableItems( CUtlLinkedList< IQueryableReplayItem *, int > &lstMovies ) = 0;
	virtual const char		*GetRenderDir() const = 0;
	virtual const char		*GetRawExportDir() const = 0;

	// TODO: Is this the best place for code that actually manages rendering?
	virtual bool			IsRendering() const = 0;
	virtual bool			RenderingCancelled() const = 0;
	virtual void			RenderMovie( RenderMovieParams_t const& params ) = 0;	// Renders the given replay - or if params.hReplay is -1, render all unrendered replays
	virtual void			RenderNextMovie() = 0;
	virtual void			CompleteRender( bool bSuccess, bool bShowBrowser ) = 0;
	virtual void			CancelRender() = 0;
	virtual void			ClearRenderCancelledFlag() = 0;
};

//----------------------------------------------------------------------------------------

#endif // IREPLAYMOVIEMANAGER_H