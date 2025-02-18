//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
//=============================================================================

#ifndef IQUICKTIME_H
#define IQUICKTIME_H

#ifdef _WIN32
#pragma once
#endif

  
#include "appframework/IAppSystem.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
struct BGR888_t;
class IMaterial;


//-----------------------------------------------------------------------------
// Handle to a QUICKTIME
//-----------------------------------------------------------------------------
typedef unsigned short QUICKTIMEHandle_t;
enum
{
	QUICKTIMEHANDLE_INVALID = (QUICKTIMEHandle_t)~0
};


//-----------------------------------------------------------------------------
// Handle to an QUICKTIME material
//-----------------------------------------------------------------------------
typedef unsigned short QUICKTIMEMaterial_t;
enum
{
	QUICKTIMEMATERIAL_INVALID = (QUICKTIMEMaterial_t)~0
};

typedef unsigned int MovieHandle_t;
const MovieHandle_t	cInvalidMovieHandle = (MovieHandle_t) ~0;

enum eVideoSystemStatus
{
	cVideoSystem_OK = 0,
	cVideoSystem_NotInstalled,
	cVideoSystem_NotCurrentVersion,
	cVideoSystem_InitializationError,
	
	cVideoSystem_ForceInt32 = 0x7FFFFFFF	// Make sure eNum is (at least) an int32
};


enum eVideoSystemFeatures
{
	cVideoSystem_NoFeatures						= 0x00000000,
	cVideoSystem_PlayMoviesFromFile				= 0x00000001,
	cVideoSystem_RenderVideoFrameToMaterial		= 0x00000002,
	cVideoSystem_EncodeVideoToFile				= 0x00000010,
	cVideoSystem_EncodeAudioToFile				= 0x00000020,
	
	cVideoSystem_ForceInt32a					= 0x7FFFFFFF
};

DEFINE_ENUM_BITWISE_OPERATORS( eVideoSystemFeatures );

enum eVideoEncodeQuality
{
	cVEQuality_Min = 0,
	cVEQuality_Low = 25,
	cVEQuality_Normal = 50,
	cVEQuality_High = 75,
	cVEQuality_Max = 100
};

// -----------------------------------------------------------------------
// eVideoFrameFormat_t - bit format for quicktime video frames
// -----------------------------------------------------------------------
enum eVideoFrameFormat_t
{
	cVFF_Undefined = 0,
	cVFF_R8G8B8A8_32Bit,
	cVFF_R8G8B8_24Bit,
	
	cVFF_Count,						// Auto list counter
	cVFF_ForceInt32	= 0x7FFFFFFF	// Make sure eNum is (at least) an int32
};

// -----------------------------------------------------------------------
// eAudioSourceFormat_t - Audio encoding source options
// -----------------------------------------------------------------------
enum eAudioSourceFormat_t
{
	cASF_Undefined = 0,
	cASF_None,
	cASF_16BitPCMStereo,
	
	cASF_Count,						// Auto list counter
	cASF_ForceInt32	= 0x7FFFFFFF	// Make sure eNum is (at least) an int32
};



//-----------------------------------------------------------------------------
// IQuickTimeMovieMaker interface
//-----------------------------------------------------------------------------
class IQuickTimeMovieMaker : public IBaseInterface
{
public:
	virtual bool	CreateNewMovieFile(  MovieHandle_t &theMovie, const char *pFilename, int nWidth, int nHeight, int nFps, eVideoEncodeQuality quality, eAudioSourceFormat_t srcAudioFormat = cASF_None, int audioSampleRate = 0  ) = 0;
	virtual bool	AppendVideoFrame( MovieHandle_t theMovie, unsigned char *pFrame ) = 0;
	virtual bool	AppendAudioSamples( MovieHandle_t theMovie, void *sampleBuffer, size_t sampleSize ) = 0;
	virtual bool	FinishMovie( MovieHandle_t theMovie, bool success = true ) = 0;
};

//-----------------------------------------------------------------------------
// Main QUICKTIME interface
//-----------------------------------------------------------------------------
#define QUICKTIME_INTERFACE_VERSION "IQuickTime002"

class IQuickTime : public IAppSystem
{
public:
	virtual bool					IsVideoSystemAvailable() = 0;
	virtual eVideoSystemStatus		GetVideoSystemStatus() = 0;
	virtual eVideoSystemFeatures	GetVideoSystemFeatures() = 0;

	// Create/destroy a QUICKTIME material (a materialsystem IMaterial)
	virtual QUICKTIMEMaterial_t CreateMaterial( const char *pMaterialName, const char *pFileName, const char *pPathID ) = 0;
	virtual void DestroyMaterial( QUICKTIMEMaterial_t hMaterial ) = 0;

	// Create/destroy a quicktime movie maker, which will encode audio/video
	virtual IQuickTimeMovieMaker *CreateMovieMaker() = 0;
	virtual void DestroyMovieMaker( IQuickTimeMovieMaker *&pMovieMaker ) = 0;
	
	// Update the frame (if necessary)
	virtual bool Update( QUICKTIMEMaterial_t hMaterial ) = 0;

	// Gets the IMaterial associated with an BINK material
	virtual IMaterial* GetMaterial( QUICKTIMEMaterial_t hMaterial ) = 0;

	// Returns the max texture coordinate of the BINK
	virtual void GetTexCoordRange( QUICKTIMEMaterial_t hMaterial, float *pMaxU, float *pMaxV ) = 0;

	// Returns the frame size of the QUICKTIME Image Frame (stored in a subrect of the material itself)
	virtual void GetFrameSize( QUICKTIMEMaterial_t hMaterial, int *pWidth, int *pHeight ) = 0;

	// Returns the frame rate of the QUICKTIME
	virtual int GetFrameRate( QUICKTIMEMaterial_t hMaterial ) = 0;

	// Sets the frame for an BINK material (use instead of SetTime)
	virtual void SetFrame( QUICKTIMEMaterial_t hMaterial, float flFrame ) = 0;

	// Returns the total frame count of the BINK
	virtual int GetFrameCount( QUICKTIMEMaterial_t hMaterial ) = 0;

	virtual bool SetSoundDevice( void *pDevice ) = 0;

	// Plays a given MOV file until it completes or the user presses ESC, SPACE, or ENTER
	virtual void PlayQuicktimeVideo( const char *filename, void *mainWindow, int windowWidth, int windowHeight, int desktopWidth, int desktopHeight, bool windowed, float forcedMinTime ) = 0;
	
	// Estimates the size of a recorded movie
    virtual bool EstimateMovieSize( unsigned long &EstSize, int nWidth, int nHeight, int nFps,  float duration, eVideoEncodeQuality quality, eAudioSourceFormat_t srcAudioFormat = cASF_None, int audioSampleRate = 0 ) = 0;

};


#endif // IQUICKTIME_H
