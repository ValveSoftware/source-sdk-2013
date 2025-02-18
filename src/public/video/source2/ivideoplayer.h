//========= Copyright (c) Valve LLC, All rights reserved. ============
//
// Purpose: Video player & management for all supported types of video files
//
//=============================================================================

#ifndef IVIDEOPLAYER_H
#define IVIDEOPLAYER_H
#pragma once


#ifdef LIBVIDEO_DLL_EXPORT
#define LIBVIDEO_INTERFACE	DLL_EXPORT
#else
#define LIBVIDEO_INTERFACE	DLL_IMPORT
#endif


//-----------------------------------------------------------------------------
// Purpose: Video rendering interface for video player
//-----------------------------------------------------------------------------
class IVideoPlayerVideoCallback
{
public:
	// For videos without an alpha plane, the pPlaneA pointer will be nullptr,
	// and the unStrideA value will be zero.
	virtual bool BPresentYUVA420Texture( uint nWidth, uint nHeight, void *pPlaneY, void *pPlaneU, void *pPlaneV, void *pPlaneA, uint unStrideY, uint unStrideU, uint unStrideV, uint unStrideA ) = 0;
};


//-----------------------------------------------------------------------------
// Purpose: Audio rendering interface for video player
//-----------------------------------------------------------------------------
class IVideoPlayerAudioCallback
{
public:
	virtual bool InitAudioOutput( int nSampleRate, int nChannels ) = 0;
	virtual void FreeAudioOutput() = 0;
	virtual bool IsReadyForAudioData() = 0;
	virtual void *GetAudioBuffer() = 0;
	virtual uint32 GetAudioBufferSize() = 0;
	virtual uint32 GetAudioBufferMinSize() = 0;
	virtual void CommitAudioBuffer( uint32 unBytes ) = 0;
	virtual uint32 GetRemainingCommittedAudio() = 0;
	virtual uint32 GetMixedMilliseconds() = 0;
	virtual uint32 GetPlaybackLatency() = 0;
	virtual void Pause() = 0;
	virtual void Resume() = 0;
};


//-----------------------------------------------------------------------------
// Purpose: Possible playback events
//-----------------------------------------------------------------------------
enum EVideoPlayerEvent
{
	k_EVideoPlayerEventInit,
	k_EVideoPlayerEventRepeat,
	k_EVideoPlayerEventEnd,
	k_EVideoPlayerEventPlaybackStateChange,
	k_EVideoPlayerEventChangedRepresentation,
};


//-----------------------------------------------------------------------------
// Purpose: Video player event interface
//-----------------------------------------------------------------------------
class IVideoPlayerEventCallback
{
public:
	virtual void VideoPlayerEvent( EVideoPlayerEvent eEvent ) = 0;
};


//-----------------------------------------------------------------------------
// Purpose: Possible playback states
//-----------------------------------------------------------------------------
enum EVideoPlayerPlaybackState
{
	k_EVideoPlayerPlaybackStatePause,
	k_EVideoPlayerPlaybackStatePlay,
	k_EVideoPlayerPlaybackStateStop,
	k_EVideoPlayerPlaybackStateAsyncSeek
};


//-----------------------------------------------------------------------------
// Purpose: Possible seek modes
//-----------------------------------------------------------------------------
enum EVideoPlayerSeekMode
{
	k_EVideoPlayerSeekModeKeyframe = 0,		// quick seek to nearby keyframe (default)
	k_EVideoPlayerSeekModePreciseAsync = 1,	// precise seek, state = AsyncSeek -> state = Pause
};


//-----------------------------------------------------------------------------
// Purpose: Possible playback errors
//			Numbered as they are used by web code
//-----------------------------------------------------------------------------
enum EVideoPlayerPlaybackError
{
	k_EVideoPlayerPlaybackErrorNone = 0,
	k_EVideoPlayerPlaybackErrorGeneric = 1,
	k_EVideoPlayerPlaybackErrorFailedDownload = 2,
};


//-----------------------------------------------------------------------------
// Purpose: Plays all supported types of video files
//-----------------------------------------------------------------------------
class IVideoPlayer
{
public:

	virtual ~IVideoPlayer() { }

	virtual bool BLoad( const char *pchURL ) = 0;
	virtual bool BLoad( const byte *pubData, uint cubData ) = 0;

	// playback control
	virtual void Play() = 0;
	virtual void Stop() = 0;
	virtual void Pause() = 0;
	virtual void SetPlaybackSpeed( float flPlaybackSpeed ) = 0;
	virtual void Seek( uint unSeekMS ) = 0;

	// settings
	virtual void SetRepeat( bool bRepeat ) = 0;
	virtual void SuggestMaxVerticalResolution( int nHeight ) = 0;

	// state. all returned times are in milliseconds
	virtual EVideoPlayerPlaybackState GetPlaybackState() = 0;
	virtual bool IsStoppedForBuffering() = 0;
	virtual float GetPlaybackSpeed() = 0;
	virtual uint32 GetDuration() = 0;
	virtual uint32 GetCurrentPlaybackTime() = 0;
	virtual EVideoPlayerPlaybackError GetPlaybackError() = 0;

	// debugging
	virtual void GetVideoResolution( int *pnWidth, int *pnHeight ) = 0;
	virtual int GetVideoDownloadRate() = 0;

	// video and audio can have multiple representations (1080p, 720p, etc.). Accessors:
	virtual int GetVideoRepresentationCount() = 0;
	virtual bool BGetVideoRepresentationInfo( int iRep, int *pnWidth, int *pnHeight ) = 0;
	virtual void ForceVideoRepresentation( int iRep ) = 0;
	virtual int GetCurrentVideoRepresentation() = 0;
	virtual void GetVideoSegmentInfo( int *pnCurrent, int *pnTotal ) = 0;

	virtual bool BHasAudioTrack() = 0;
	
	// control seek behavior/precision
	virtual void SetSeekMode( EVideoPlayerSeekMode eSeekMode ) = 0;
};


//-----------------------------------------------------------------------------
// Purpose: Methods
//-----------------------------------------------------------------------------
LIBVIDEO_INTERFACE void VideoPlaybackInitialize();
LIBVIDEO_INTERFACE void VideoPlaybackShutdown();
LIBVIDEO_INTERFACE void VideoPlaybackRunFrame();
LIBVIDEO_INTERFACE IVideoPlayer *CreateVideoPlayer( IVideoPlayerEventCallback *pEventCallback, IVideoPlayerVideoCallback *pVideoCallback, IVideoPlayerAudioCallback *pAudioCallback );
LIBVIDEO_INTERFACE void DeleteVideoPlayer( IVideoPlayer *pVideoPlayer );

#ifdef DBGFLAG_VALIDATE
LIBVIDEO_INTERFACE void VideoPauseForValidation();
LIBVIDEO_INTERFACE void VideoResumeFromValidation();
LIBVIDEO_INTERFACE void VideoValidate( CValidator &validator, const char *pchName );
LIBVIDEO_INTERFACE void VideoValidateStatics( CValidator &validator );
#endif


#endif // IVIDEOPLAYER_H
