//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IREPLAYMOVIE_H
#define IREPLAYMOVIE_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "iqueryablereplayitem.h"
#include "replay/rendermovieparams.h"

//----------------------------------------------------------------------------------------

abstract_class IReplayMovie : public IQueryableReplayItem
{
public:
	virtual ReplayHandle_t		GetMovieHandle()  const = 0;
	virtual ReplayHandle_t		GetReplayHandle() const = 0;
	virtual const ReplayRenderSettings_t &GetRenderSettings() = 0;
	virtual void				GetFrameDimensions( int &nWidth, int &nHeight ) = 0;
	virtual void				SetIsRendered( bool bIsRendered ) = 0;
	virtual void				SetMovieFilename( const char *pFilename ) = 0;
	virtual const char			*GetMovieFilename() const = 0;
	virtual void				SetMovieTitle( const wchar_t *pTitle ) = 0;
	virtual void				SetRenderTime( float flRenderTime ) = 0;
	virtual float				GetRenderTime() const = 0;
	virtual void				CaptureRecordTime() = 0;
	virtual void				SetLength( float flLength ) = 0;

	virtual bool				IsUploaded() const = 0;
	virtual void				SetUploaded( bool bUploaded ) = 0;
	virtual void				SetUploadURL( const char *pURL ) = 0;
	virtual const char			*GetUploadURL() const = 0;
};

//----------------------------------------------------------------------------------------

#endif // IREPLAYMOVIE_H