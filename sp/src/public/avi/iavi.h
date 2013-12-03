//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
//=============================================================================

#ifndef IAVI_H
#define IAVI_H

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
// Parameters for creating a new AVI
//-----------------------------------------------------------------------------
struct AVIParams_t
{
	AVIParams_t() :
		m_nFrameRate( 0 ), m_nFrameScale( 1 ), m_nWidth( 0 ), m_nHeight( 0 ),
		m_nSampleRate( 0 ), m_nSampleBits( 0 ), m_nNumChannels( 0 ), m_bGetCodecFromUser( true )
	{
		m_pFileName[ 0 ] = 0;
	}

	char		m_pFileName[ 256 ];
	char		m_pPathID[ 256 ];

	// fps = m_nFrameRate / m_nFrameScale
	// for integer framerates, set framerate to the fps, and framescale to 1
	// for ntsc-style framerates like 29.97 (or 23.976 or 59.94),
	// set framerate to 30,000 (or 24,000 or 60,000) and framescale to 1001
	// yes, framescale is an odd naming choice, but it matching MS's AVI api
	int			m_nFrameRate;
	int			m_nFrameScale;

	int			m_nWidth;
	int			m_nHeight;

	// Sound/.wav info
	int			m_nSampleRate;
	int			m_nSampleBits;
	int			m_nNumChannels;

	// The user will be asked to select a compressor if true, otherwise the
	// previous or default will be used.
	bool		m_bGetCodecFromUser;
};


//-----------------------------------------------------------------------------
// Handle to an AVI
//-----------------------------------------------------------------------------
typedef unsigned short AVIHandle_t;
enum
{
	AVIHANDLE_INVALID = (AVIHandle_t)~0
};


//-----------------------------------------------------------------------------
// Handle to an AVI material
//-----------------------------------------------------------------------------
typedef unsigned short AVIMaterial_t;
enum
{
	AVIMATERIAL_INVALID = (AVIMaterial_t)~0
};


//-----------------------------------------------------------------------------
// Main AVI interface
//-----------------------------------------------------------------------------
#define AVI_INTERFACE_VERSION "VAvi001"

class IAvi : public IAppSystem
{
public:
	// Necessary to call this before any other AVI interface methods 
	virtual void	SetMainWindow( void* hWnd ) = 0;

	// Start/stop recording an AVI
	virtual AVIHandle_t	StartAVI( const AVIParams_t& params ) = 0;
	virtual void	FinishAVI( AVIHandle_t handle ) = 0;

	// Add frames to an AVI
	virtual void	AppendMovieSound( AVIHandle_t h, short *buf, size_t bufsize ) = 0;
	virtual void	AppendMovieFrame( AVIHandle_t h, const BGR888_t *pRGBData ) = 0;

	// Create/destroy an AVI material (a materialsystem IMaterial)
	virtual AVIMaterial_t CreateAVIMaterial( const char *pMaterialName, const char *pFileName, const char *pPathID ) = 0;
	virtual void DestroyAVIMaterial( AVIMaterial_t hMaterial ) = 0;
	
	// Sets the time for an AVI material
	virtual void SetTime( AVIMaterial_t hMaterial, float flTime ) = 0;

	// Gets the IMaterial associated with an AVI material
	virtual IMaterial* GetMaterial( AVIMaterial_t hMaterial ) = 0;

	// Returns the max texture coordinate of the AVI
	virtual void GetTexCoordRange( AVIMaterial_t hMaterial, float *pMaxU, float *pMaxV ) = 0;

	// Returns the frame size of the AVI (stored in a subrect of the material itself)
	virtual void GetFrameSize( AVIMaterial_t hMaterial, int *pWidth, int *pHeight ) = 0;

	// Returns the frame rate of the AVI
	virtual int GetFrameRate( AVIMaterial_t hMaterial ) = 0;

	// Returns the total frame count of the AVI
	virtual int GetFrameCount( AVIMaterial_t hMaterial ) = 0;

	// Sets the frame for an AVI material (use instead of SetTime)
	virtual void SetFrame( AVIMaterial_t hMaterial, float flFrame ) = 0;

	// Plays a given AVI/WMV file until it completes or the user presses ESC, SPACE, or ENTER
	virtual void PlayWindowsMediaVideo( const char *filename, void *mainWindow, int width, int height, float forcedMinTime ) = 0;

};



#endif // IAVI_H
