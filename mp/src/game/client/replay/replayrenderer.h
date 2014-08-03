//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef REPLAYRENDERER_H
#define REPLAYRENDERER_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------

#include "replay/ireplaymovierenderer.h"
#include "replay/rendermovieparams.h"
#include "movieobjects/timeutils.h"
#include "fmtstr.h"

//-----------------------------------------------------------------------------

class CReplay;
class CReplayPerformance;
class IQuickTimeMovieMaker;
class CReplayRenderOverlay;
class IVideoRecorder;

//-----------------------------------------------------------------------------

class CReplayRenderer : public IReplayMovieRenderer
{
public:
	CReplayRenderer( CReplayRenderOverlay *pOverlay );
	~CReplayRenderer();

	const CReplayPerformance *GetPerformance() const;
	const char		*GetMovieFilename() const;
	static int		GetNumMotionBlurTimeSteps( int nQuality );

	//
	// IReplayMovieRenderer interface
	//
	virtual bool	SetupRenderer( RenderMovieParams_t &params, IReplayMovie *pMovie );
	virtual void	ShutdownRenderer();
	virtual void	RenderVideo();
	virtual void	RenderAudio( unsigned char *pBuffer, int nSize, int nNumSamples );

	virtual void	SetAudioSyncFrame( bool isSync = false );
	virtual bool	IsAudioSyncFrame();
	virtual float	GetRecordingFrameDuration();


private:
	bool			IsDepthOfFieldEnabled() const;
	bool			IsAntialiasingEnabled() const;
	bool			IsHDR() const;
	bool			IsMotionBlurEnabled() const;
	int				GetMotionBlurQuality() const;
	int				GetDepthOfFieldQuality() const;
	int				NumMotionBlurTimeSteps() const;
	float			GetFramerate() const;
	void			ComputeSampleCounts( int *pNSamplesPerTimeStep, int *pNTotalSamples ) const;
	float			GetViewModelFOVOffset();
	void			SetupDOFMatrixSkewView( const Vector &pos, const QAngle &angles, int nSample, CViewSetup& viewSetup );
	void			BeginRenderingSample( int nSample, int x, int y, int nWidth, int nHeight, float fTonemapScale );
	void			EndRendering();
	void			SetupSampleView( int x, int y, int w, int h, int nSample, CViewSetup& viewSetup );
	void			InitBuffers( const RenderMovieParams_t &params );
	void			DrawResolvingQuad( int nWidth, int nHeight );
	float			GetRenderLength( const CReplay *pReplay );
	void			CompositeAndLayoffFrame( int nFrame );
	void			RenderLayoffFrame( DmeTime_t time, int nCurSample, int nNumTotalSamples );
	void			ResolveSamples( int nSample, DmeTime_t frametime, int x, int y, int nWidth, int nHeight, bool bShowUser, float flBloomScale );
	void			LayoffFrame( int nFrame );
	double			GetShutterSpeed() const;
	void			ClearToBlack( CTextureReference &buf, int x, int y, int nWidth, int nHeight );
	bool			SetupJitterTable();
	void			GetViewSetup( CViewSetup &viewsetup );

	bool				m_bIsAudioSyncFrame;

	const Vector2D		*m_pJitterTable;
	int					m_nNumJitterSamples;
	IVideoRecorder		*m_pMovieMaker;
	bool				m_bCacheFullSceneState;
	BGRA8888_t			*m_pLayoffBuf;
	IReplayMovie		*m_pMovie;
	RenderMovieParams_t	m_RenderParams;
	CTextureReference	m_AccumBuffSample;
	CTextureReference	m_LayoffResult;	
	CTextureReference	m_AccumBuffPingPong[2];				// Buffers and materials for ping-pong accumulation buffer
	CMaterialReference	m_FourSampleResolveMatRef;
	bool				m_bForceCheapDoF;
	bool				m_bShutterClosed;
	int					m_nCurrentPingPong;
	int					m_nFrame;

	ConVar				*m_pViewmodelFov;
	ConVar				*m_pDefaultFov;

	int					m_nTimeStep;
	int					m_nCurSample;
	DmeTime_t			m_curSampleTime;

	int					m_iTgaFrame;
	CFmtStr				m_fmtTgaRenderDirName;

	CReplayRenderOverlay	*m_pRenderOverlay;
};

//-----------------------------------------------------------------------------

#endif // REPLAYRENDERER_H
