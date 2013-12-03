//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IREPLAYMOVIERENDERER_H
#define IREPLAYMOVIERENDERER_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

class IReplayMovie;
struct RenderMovieParams_t;

//----------------------------------------------------------------------------------------

abstract_class IReplayMovieRenderer : public IBaseInterface
{
public:
	virtual bool			SetupRenderer( RenderMovieParams_t &params, IReplayMovie *pMovie ) = 0;
	virtual void			ShutdownRenderer() = 0;
	virtual void			RenderVideo() = 0;
	virtual void			RenderAudio( unsigned char *pBuffer, int nSize, int nNumSamples ) = 0;
	
	virtual void			SetAudioSyncFrame( bool isSync = false ) = 0;
	virtual bool			IsAudioSyncFrame() = 0;
	virtual float			GetRecordingFrameDuration() = 0;
};

//----------------------------------------------------------------------------------------

#endif // IREPLAYMOVIERENDERER_H
