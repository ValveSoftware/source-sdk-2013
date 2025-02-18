//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
//=============================================================================

#ifndef IVIDEOSERVICES_H
#define IVIDEOSERVICES_H

#if defined ( WIN32 )
    #pragma once
#endif

#include <math.h>
#include "appframework/IAppSystem.h"
#include "tier0/platform.h"

#include <stdint.h>
#ifndef _STDINT_H
#define _STDINT_H
#endif
#ifndef _STDINT
#define _STDINT
#endif

#ifndef INT32_MAX
#define INT32_MAX    (0x7FFFFFFF)
#endif
#ifndef UINT32_MAX
#define UINT32_MAX   (0xFFFFFFFFu)
#endif

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IMaterial;


//-----------------------------------------------------------------------------
// Types used when dealing with video services
//-----------------------------------------------------------------------------

#define FILE_EXTENSION_ANY_MATCHING_VIDEO		".vid"

//#define ENABLE_EXTERNAL_ENCODER_LOGGING


//-----------------------------------------------------------------------------
// enums used when dealing with video services
//-----------------------------------------------------------------------------

	// ==============================================
	// various general video system enumerations

	
namespace VideoResult
{
	enum EVideoResult_t
	{
		SUCCESS				= 0,
		
		SYSTEM_NOT_AVAILABLE,
		CODEC_NOT_AVAILABLE,
		FEATURE_NOT_AVAILABLE,
		
		UNKNOWN_OPERATION,
		ILLEGAL_OPERATION,
		OPERATION_NOT_SUPPORTED,
		
		BAD_INPUT_PARAMETERS,
		OPERATION_ALREADY_PERFORMED,
		OPERATION_OUT_OF_SEQUENCE,
		
		VIDEO_ERROR_OCCURED,
		FILE_ERROR_OCCURED,
		AUDIO_ERROR_OCCURED,
		SYSTEM_ERROR_OCCURED,
		INITIALIZATION_ERROR_OCCURED,
		SHUTDOWN_ERROR_OCCURED,
		
		MATERIAL_NOT_FOUND,
		RECORDER_NOT_FOUND,
		VIDEO_FILE_NOT_FOUND,
		VIDEO_SYSTEM_NOT_FOUND,
	};
};
typedef VideoResult::EVideoResult_t VideoResult_t;

namespace VideoSystem
{
	enum EVideoSystem_t
	{
		ALL_VIDEO_SYSTEMS = -2,
		DETERMINE_FROM_FILE_EXTENSION = -1,
		NONE = 0,
		
		BINK,
		AVI,
		WMV,
		QUICKTIME,
		WEBM,
		SOURCE2_WRAPPER,
		
		VIDEO_SYSTEM_COUNT,
		VIDEO_SYSTEM_FIRST = 1,
	};
};
typedef VideoSystem::EVideoSystem_t VideoSystem_t;


namespace VideoSystemStatus
{
	enum EVideoSystemStatus_t
	{
		OK = 0,
		NOT_INSTALLED,
		NOT_CURRENT_VERSION,
		NOT_INITIALIZED,
		INITIALIZATION_ERROR,
	};
};
typedef VideoSystemStatus::EVideoSystemStatus_t VideoSystemStatus_t;


namespace VideoSystemFeature
{
	enum EVideoSystemFeature_t
	{
		NO_FEATURES					= 0x00000000,
		PLAY_VIDEO_FILE_FULL_SCREEN = 0x00000001,
		PLAY_VIDEO_FILE_IN_MATERIAL = 0x00000002,
		ENCODE_VIDEO_TO_FILE		= 0x00000004,
		ENCODE_AUDIO_TO_FILE		= 0x00000008,
		
		
		FULL_PLAYBACK				= PLAY_VIDEO_FILE_FULL_SCREEN | PLAY_VIDEO_FILE_IN_MATERIAL,
		FULL_ENCODE					= ENCODE_VIDEO_TO_FILE | ENCODE_AUDIO_TO_FILE,
		ALL_VALID_FEATURES			= FULL_PLAYBACK | FULL_ENCODE,
		
		ESF_FORCE_UINT32			= UINT32_MAX,
	};

	DEFINE_ENUM_BITWISE_OPERATORS( EVideoSystemFeature_t );
};
typedef VideoSystemFeature::EVideoSystemFeature_t VideoSystemFeature_t;
	
	
namespace VideoSoundDeviceOperation
{
	enum EVideoSoundDeviceOperation_t
	{
		SET_DIRECT_SOUND_DEVICE = 0,			// Windows option
		SET_MILES_SOUND_DEVICE,					// Supported by RAD
		HOOK_X_AUDIO,							// Xbox Option
		SET_SOUND_MANAGER_DEVICE,				// OSX Option
		SET_LIB_AUDIO_DEVICE,					// PS3 Option
		SET_SDL_SOUND_DEVICE,					// SDL Audio
		SET_SDL_PARAMS,							// SDL Audio params
		SDLMIXER_CALLBACK,						// SDLMixer callback
				
		OPERATION_COUNT
	};
};
typedef VideoSoundDeviceOperation::EVideoSoundDeviceOperation_t VideoSoundDeviceOperation_t;


	// ==============================================
	// Video Encoding related settings

namespace VideoEncodeCodec
{
	//
	// NOTE: NEW CODECS SHOULD BE ADDED TO THE END OF THIS LIST.
	//
	enum EVideoEncodeCodec_t
	{
		MPEG2_CODEC,
		MPEG4_CODEC,
		H261_CODEC,
		H263_CODEC,
		H264_CODEC,
		MJPEG_A_CODEC,
		MJPEG_B_CODEC,
		SORENSON3_CODEC,
		CINEPACK_CODEC,
		WEBM_CODEC,

		//
		// NOTE: ADD NEW CODECS HERE.
		//

		CODEC_COUNT,

		DEFAULT_CODEC = H264_CODEC,
	};
};
typedef VideoEncodeCodec::EVideoEncodeCodec_t VideoEncodeCodec_t;


namespace VideoEncodeQuality
{
	enum EVideoEncodeQuality_t
	{
		MIN_QUALITY = 0,
		MAX_QUALITY = 100
	};
};
typedef VideoEncodeQuality::EVideoEncodeQuality_t VideoEncodeQuality_t;


namespace VideoEncodeSourceFormat
{
	enum EVideoEncodeSourceFormat_t				// Image source format for frames to encoded
	{
		BGRA_32BIT = 0,
		BGR_24BIT,
		RGB_24BIT,
		RGBA_32BIT,
		
		VIDEO_FORMAT_COUNT,
		VIDEO_FORMAT_FIRST = 0
	};
};
typedef VideoEncodeSourceFormat::EVideoEncodeSourceFormat_t VideoEncodeSourceFormat_t;


namespace VideoEncodeGamma
{
	enum EVideoEncodeGamma_t
	{
		NO_GAMMA_ADJUST = 0,
		PLATFORM_STANDARD_GAMMA,
		GAMMA_1_8,
		GAMMA_2_2,
		GAMMA_2_5,
		
		GAMMA_COUNT
	};
};
typedef VideoEncodeGamma::EVideoEncodeGamma_t VideoEncodeGamma_t;


namespace VideoPlaybackGamma
{
	enum EVideoPlaybackGamma_t
	{
		USE_GAMMA_CONVAR = -1,
		NO_GAMMA_ADJUST = 0,
		PLATFORM_DEFAULT_GAMMMA,
		GAMMA_1_8,
		GAMMA_2_2,
		GAMMA_2_5,
		
		GAMMA_COUNT
	};
};
typedef VideoPlaybackGamma::EVideoPlaybackGamma_t VideoPlaybackGamma_t;


	// ==============================================
	// Video Playback related settings

namespace VideoPlaybackFlags
{
	enum EVideoPlaybackFlags_t						// Options when playing a video file
	{
		NO_PLAYBACK_OPTIONS			= 0x00000000,
		
		// Full Screen Playback Options
		FILL_WINDOW					= 0x00000001,		// force video fill entire window
		LOCK_ASPECT_RATIO			= 0x00000002,		// preserve aspect ratio when scaling
		INTEGRAL_SCALE				= 0x00000004,		// scale video only by integral amounts
		CENTER_VIDEO_IN_WINDOW		= 0x00000008,		// center output video in window

		FORCE_MIN_PLAY_TIME			= 0x00000010,		// play for a minimum amount of time before allowing skip or abort
		
		ABORT_ON_SPACE				= 0x00000100,		// Keys to abort fullscreen playback on
		ABORT_ON_ESC				= 0x00000200,
		ABORT_ON_RETURN				= 0x00000400,
		ABORT_ON_ANY_KEY			= 0x00000700,
		
		PAUSE_ON_SPACE				= 0x00001000,		// Keys to pause fullscreen playback on
		PAUSE_ON_ESC				= 0x00002000,
		PAUSE_ON_RETURN				= 0x00004000,
		PAUSE_ON_ANY_KEY			= 0x00007000,
		
		LOOP_VIDEO					= 0x00010000,		// Full Screen and Video material
		NO_AUDIO					= 0x00020000,
		PRELOAD_VIDEO				= 0x00040000,
		
		DONT_AUTO_START_VIDEO		= 0x00100000,		// Don't begin playing until told to do so.
		TEXTURES_ACTUAL_SIZE		= 0x00200000,		// Try and use textures the same size as the video frame
		
		VALID_FULLSCREEN_FLAGS		= 0x0007771F,		// Playback Flags that are valid for playing videos fullscreen
		VALID_MATERIAL_FLAGS		= 0x00370000,		// Playback Flags that are valid for playing videos in a material

		DEFAULT_MATERIAL_OPTIONS	= NO_PLAYBACK_OPTIONS,
		DEFAULT_FULLSCREEN_OPTIONS	= CENTER_VIDEO_IN_WINDOW | LOCK_ASPECT_RATIO | ABORT_ON_ANY_KEY,
		
		EVPF_FORCE_UINT32			= UINT32_MAX,
	};
	
	DEFINE_ENUM_BITWISE_OPERATORS( EVideoPlaybackFlags_t );
}
typedef VideoPlaybackFlags::EVideoPlaybackFlags_t VideoPlaybackFlags_t;


namespace AudioEncodeSourceFormat
{
	enum EAudioEncodeSourceFormat_t				// Audio source format to encode
	{
		AUDIO_NONE = 0,
		AUDIO_16BIT_PCMStereo,
		
		AUDIO_FORMAT_COUNT
	};
};
typedef AudioEncodeSourceFormat::EAudioEncodeSourceFormat_t AudioEncodeSourceFormat_t;


namespace AudioEncodeOptions
{
	enum EAudioEncodeOptions_t						// Options to control audio encoding
	{
		NO_AUDIO_OPTIONS					= 0x00000000,
		
		USE_AUDIO_ENCODE_GROUP_SIZE			= 0x00000001,		// When adding to the video media, use fixed size sample groups
		GROUP_SIZE_IS_VIDEO_FRAME			= 0x00000002,		// use a group size equal to one video frame in duration
		LIMIT_AUDIO_TRACK_TO_VIDEO_DURATION = 0x00000004,		// Don't let the Audio Track exceed the video track in duration
		PAD_AUDIO_WITH_SILENCE				= 0x00000008,		// If Audio track duration is less than video track's, pad with silence
		
		AEO_FORCE_UINT32					= UINT32_MAX,
	};
	DEFINE_ENUM_BITWISE_OPERATORS( EAudioEncodeOptions_t );
}
typedef AudioEncodeOptions::EAudioEncodeOptions_t AudioEncodeOptions_t;


//-----------------------------------------------------------------------------
// Frame Rate Class
//-----------------------------------------------------------------------------
class VideoFrameRate_t
{
	public:
		inline						VideoFrameRate_t() : m_TimeUnitsPerSecond( 0 ), m_TimeUnitsPerFrame( 1000 ) {};
		
		inline						VideoFrameRate_t( int FPS, bool NTSC )		{ SetFPS( FPS, NTSC); } 
		inline	explicit			VideoFrameRate_t( float FPS )				{ SetFPS( FPS); };

		inline VideoFrameRate_t&	operator=( const VideoFrameRate_t& rhs ) { m_TimeUnitsPerSecond = rhs.m_TimeUnitsPerSecond; m_TimeUnitsPerFrame = rhs.m_TimeUnitsPerFrame; return *this; }
		inline						VideoFrameRate_t( const VideoFrameRate_t &rhs ) { *this = rhs; };

		inline void					SetRaw( int timeUnitsPerSecond, int TimeUnitsPerFrame )		{ m_TimeUnitsPerSecond = timeUnitsPerSecond; m_TimeUnitsPerFrame = TimeUnitsPerFrame; }
		
		inline float				GetFPS() const					{ return (float) m_TimeUnitsPerSecond / (float) m_TimeUnitsPerFrame; }
		inline int					GetIntFPS() const				{ return (int) ( (float) m_TimeUnitsPerSecond / (float) m_TimeUnitsPerFrame + 0.5f ); }
		inline bool					IsNTSCRate() const				{ return ( m_TimeUnitsPerFrame == 1001 ); }
		inline int					GetUnitsPerSecond() const		{ return m_TimeUnitsPerSecond; }
		inline int					GetUnitsPerFrame() const		{ return m_TimeUnitsPerFrame; }

		inline void					SetFPS( int FPS, bool NTSC )	{ m_TimeUnitsPerSecond = FPS * 1000; m_TimeUnitsPerFrame = 1000 + (uint) NTSC; }
		inline void					SetFPS( float FPS )				{ m_TimeUnitsPerSecond = (uint) ( FPS * 1000.0f ); m_TimeUnitsPerFrame = 1000; }
		
		static inline bool			IsNTSC( float FPS )				{ float diff = ceil(FPS) - FPS; return ( diff > 0.02f && diff < 0.05f); }

		inline void					Clear()							{ m_TimeUnitsPerSecond = 0; m_TimeUnitsPerFrame = 1000; }
		inline bool					IsValid()						{ return ( m_TimeUnitsPerSecond != 0); }

	private:
		uint32						m_TimeUnitsPerSecond;
		uint32						m_TimeUnitsPerFrame;
};



//-----------------------------------------------------------------------------
// specific interfaces returned and managed by video services
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Video Material interface - manages the playing back of a video to a 
//     a material / texture combo
//-----------------------------------------------------------------------------
class IVideoMaterial : public IBaseInterface
{
	public:
		// Video information functions		
		virtual const char		   *GetVideoFileName() = 0;								// Gets the file name of the video this material is playing
		virtual VideoResult_t		GetLastResult() = 0;								// Gets detailed info on the last operation
		
		virtual VideoFrameRate_t	&GetVideoFrameRate() = 0;							// Returns the frame rate of the associated video in FPS

		// Audio Functions
		virtual bool				HasAudio() = 0;										// Query if the video has an audio track
		
		virtual bool				SetVolume( float fVolume ) = 0;						// Adjust the playback volume
		virtual float				GetVolume() = 0;									// Query the current volume
		
		virtual void				SetMuted( bool bMuteState ) = 0;					// Mute/UnMutes the audio playback
		virtual bool				IsMuted() = 0;										// Query muted status

		virtual VideoResult_t		SoundDeviceCommand( VideoSoundDeviceOperation_t operation, void *pDevice = nullptr, void *pData = nullptr ) = 0;		// Assign Sound Device for this Video Material
		
		// Video playback state functions
		virtual bool				IsVideoReadyToPlay() = 0;							// Queries if the video material was initialized successfully and is ready for playback, but not playing or finished
		virtual bool				IsVideoPlaying() = 0;								// Is the video currently playing (and needs update calls, etc), or paused while playing?
		virtual bool				IsNewFrameReady() = 0;								// Do we have a new frame to get & display?
		virtual bool				IsFinishedPlaying() = 0;							// Have we reached the end of the movie
		
		virtual bool				StartVideo() = 0;									// Starts the video playing
		virtual bool				StopVideo() = 0;									// Terminates the video playing
		
		virtual void				SetLooping( bool bLoopVideo ) = 0;					// Sets the video to loop (or not)
		virtual bool				IsLooping() = 0;									// Queries if the video is looping
		
		virtual void				SetPaused( bool bPauseState ) = 0;					// Pauses or Unpauses video playback
		virtual bool				IsPaused() = 0;										// Queries if the video is paused

		// Position in playback functions
		virtual float				GetVideoDuration() = 0;								// Returns the duration of the associated video in seconds
		virtual int					GetFrameCount() = 0;								// Returns the total number of (unique) frames in the video
		
		virtual bool				SetFrame( int FrameNum ) = 0;						// Sets the current frame # in the video to play next 
		virtual int					GetCurrentFrame() = 0;								// Gets the current frame # for the video playback, 0 based
		
		virtual bool				SetTime( float flTime ) = 0;						// Sets the video playback to specified time (in seconds)
		virtual float				GetCurrentVideoTime() = 0;							// Gets the current time in the video playback

		// Update function
		virtual bool				Update() = 0;										// Updates the video frame to reflect the time passed, true = new frame available

		// Material / Texture Info functions
		virtual IMaterial		   *GetMaterial() = 0;									// Gets the IMaterial associated with an video material

		virtual void				GetVideoTexCoordRange( float *pMaxU, float *pMaxV ) = 0;		// Returns the max texture coordinate of the video portion of the material surface ( 0.0, 0.0 to U, V )
		virtual void				GetVideoImageSize( int *pWidth, int *pHeight ) = 0;				// Returns the frame size of the Video Image Frame in pixels ( the stored in a subrect of the material itself)
		
};


//-----------------------------------------------------------------------------
// Video Recorder interface - manages the creation of a new video file
//-----------------------------------------------------------------------------
class IVideoRecorder : public IBaseInterface
{
	public:
		virtual bool				EstimateMovieFileSize( size_t *pEstSize, int movieWidth, int movieHeight, VideoFrameRate_t movieFps, float movieDuration, VideoEncodeCodec_t theCodec, int videoQuality,  AudioEncodeSourceFormat_t srcAudioFormat = AudioEncodeSourceFormat::AUDIO_NONE, int audioSampleRate = 0 ) = 0;
		
		virtual bool				CreateNewMovieFile( const char *pFilename, bool hasAudioTrack = false ) = 0;
		
		virtual bool				SetMovieVideoParameters( VideoEncodeCodec_t theCodec, int videoQuality, int movieFrameWidth, int movieFrameHeight, VideoFrameRate_t movieFPS, VideoEncodeGamma_t gamma = VideoEncodeGamma::NO_GAMMA_ADJUST ) = 0;
		virtual bool				SetMovieSourceImageParameters( VideoEncodeSourceFormat_t srcImageFormat, int imgWidth, int imgHeight ) = 0;
		virtual bool				SetMovieSourceAudioParameters( AudioEncodeSourceFormat_t srcAudioFormat = AudioEncodeSourceFormat::AUDIO_NONE, int audioSampleRate = 0, AudioEncodeOptions_t audioOptions = AudioEncodeOptions::NO_AUDIO_OPTIONS, int audioSampleGroupSize = 0) = 0;
		
		virtual bool				IsReadyToRecord() = 0;
		virtual VideoResult_t		GetLastResult() = 0;
		
		virtual bool				AppendVideoFrame( void *pFrameBuffer, int nStrideAdjustBytes = 0 ) = 0;
		virtual bool				AppendAudioSamples( void *pSampleBuffer, size_t sampleSize ) = 0;
		
		virtual int					GetFrameCount() = 0;
		virtual int					GetSampleCount() = 0;
		
		virtual VideoFrameRate_t	GetFPS() = 0;
		virtual int					GetSampleRate() = 0;
		
		virtual bool				AbortMovie() = 0;
		virtual bool				FinishMovie( bool SaveMovieToDisk = true ) = 0;
		
#ifdef ENABLE_EXTERNAL_ENCODER_LOGGING
 		virtual bool                LogMessage(  const char *msg ) = 0;
#endif
		
};




//-----------------------------------------------------------------------------
// Main VIDEO_SERVICES interface
//-----------------------------------------------------------------------------
#define VIDEO_SERVICES_INTERFACE_VERSION	"IVideoServices002"


class IVideoServices : public IAppSystem
{
	public:
		// Query the available video systems
		virtual int						GetAvailableVideoSystemCount() = 0;
		virtual VideoSystem_t			GetAvailableVideoSystem( int n ) = 0;
		
		virtual bool					IsVideoSystemAvailable( VideoSystem_t videoSystem ) = 0;
		virtual VideoSystemStatus_t		GetVideoSystemStatus( VideoSystem_t videoSystem ) = 0;
		virtual VideoSystemFeature_t	GetVideoSystemFeatures( VideoSystem_t videoSystem ) = 0;
		virtual const char			   *GetVideoSystemName( VideoSystem_t videoSystem ) = 0;

		virtual VideoSystem_t			FindNextSystemWithFeature( VideoSystemFeature_t features, VideoSystem_t startAfter = VideoSystem::NONE ) = 0;

		virtual VideoResult_t			GetLastResult() = 0;
		
		// deal with video file extensions and video system mappings
		virtual	int						GetSupportedFileExtensionCount( VideoSystem_t videoSystem ) = 0;		
		virtual const char             *GetSupportedFileExtension( VideoSystem_t videoSystem, int extNum = 0 ) = 0;
		virtual VideoSystemFeature_t    GetSupportedFileExtensionFeatures( VideoSystem_t videoSystem, int extNum = 0 ) = 0;
		
		virtual	VideoSystem_t			LocateVideoSystemForPlayingFile( const char *pFileName, VideoSystemFeature_t playMode = VideoSystemFeature::PLAY_VIDEO_FILE_IN_MATERIAL ) = 0;
		virtual VideoResult_t			LocatePlayableVideoFile( const char *pSearchFileName, const char *pPathID, VideoSystem_t *pPlaybackSystem, char *pPlaybackFileName, int fileNameMaxLen, VideoSystemFeature_t playMode = VideoSystemFeature::FULL_PLAYBACK ) = 0;

		// Create/destroy a video material
		virtual IVideoMaterial		   *CreateVideoMaterial( const char *pMaterialName, const char *pVideoFileName, const char *pPathID = nullptr, 
															 VideoPlaybackFlags_t playbackFlags = VideoPlaybackFlags::DEFAULT_MATERIAL_OPTIONS,
															 VideoSystem_t videoSystem = VideoSystem::DETERMINE_FROM_FILE_EXTENSION, bool PlayAlternateIfNotAvailable = true ) = 0;
															 
		virtual VideoResult_t			DestroyVideoMaterial( IVideoMaterial* pVideoMaterial ) = 0;
		virtual int						GetUniqueMaterialID() = 0;

		// Create/destroy a video encoder		
		virtual VideoResult_t			IsRecordCodecAvailable( VideoSystem_t videoSystem, VideoEncodeCodec_t codec ) = 0;
		
		virtual IVideoRecorder		   *CreateVideoRecorder( VideoSystem_t videoSystem ) = 0;
		virtual VideoResult_t			DestroyVideoRecorder( IVideoRecorder *pVideoRecorder ) = 0;

		// Plays a given video file until it completes or the user presses ESC, SPACE, or ENTER
		virtual VideoResult_t			PlayVideoFileFullScreen( const char *pFileName, const char *pPathID, void *mainWindow, int windowWidth, int windowHeight, int desktopWidth, int desktopHeight, bool windowed, float forcedMinTime, 
																 VideoPlaybackFlags_t playbackFlags = VideoPlaybackFlags::DEFAULT_FULLSCREEN_OPTIONS, 
																 VideoSystem_t videoSystem = VideoSystem::DETERMINE_FROM_FILE_EXTENSION, bool PlayAlternateIfNotAvailable = true ) = 0;

		// Sets the sound devices that the video will decode to
		virtual VideoResult_t			SoundDeviceCommand( VideoSoundDeviceOperation_t operation, void *pDevice = nullptr, void *pData = nullptr, VideoSystem_t videoSystem = VideoSystem::ALL_VIDEO_SYSTEMS ) = 0;

		// Get the (localized) name of a codec as a string
		virtual const wchar_t			*GetCodecName( VideoEncodeCodec_t nCodec ) = 0;
			
};








#endif // IVIDEOSERVICES_H
