//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef MATERIALSYSTEM_CONFIG_H
#define MATERIALSYSTEM_CONFIG_H
#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/imaterialsystem.h"

#define MATERIALSYSTEM_CONFIG_VERSION "VMaterialSystemConfig002"

enum MaterialSystem_Config_Flags_t
{
	MATSYS_VIDCFG_FLAGS_WINDOWED					= ( 1 << 0 ),
	MATSYS_VIDCFG_FLAGS_RESIZING					= ( 1 << 1 ),
	MATSYS_VIDCFG_FLAGS_NO_WAIT_FOR_VSYNC			= ( 1 << 3 ),
	MATSYS_VIDCFG_FLAGS_STENCIL						= ( 1 << 4 ),
	MATSYS_VIDCFG_FLAGS_FORCE_TRILINEAR				= ( 1 << 5 ),
	MATSYS_VIDCFG_FLAGS_FORCE_HWSYNC				= ( 1 << 6 ),
	MATSYS_VIDCFG_FLAGS_DISABLE_SPECULAR			= ( 1 << 7 ),
	MATSYS_VIDCFG_FLAGS_DISABLE_BUMPMAP				= ( 1 << 8 ),
	MATSYS_VIDCFG_FLAGS_ENABLE_PARALLAX_MAPPING		= ( 1 << 9 ),
	MATSYS_VIDCFG_FLAGS_USE_Z_PREFILL				= ( 1 << 10 ),
	MATSYS_VIDCFG_FLAGS_REDUCE_FILLRATE				= ( 1 << 11 ),
	MATSYS_VIDCFG_FLAGS_ENABLE_HDR					= ( 1 << 12 ),
	MATSYS_VIDCFG_FLAGS_LIMIT_WINDOWED_SIZE			= ( 1 << 13 ),
	MATSYS_VIDCFG_FLAGS_SCALE_TO_OUTPUT_RESOLUTION  = ( 1 << 14 ),
	MATSYS_VIDCFG_FLAGS_USING_MULTIPLE_WINDOWS      = ( 1 << 15 ),
	MATSYS_VIDCFG_FLAGS_DISABLE_PHONG               = ( 1 << 16 ),
	MATSYS_VIDCFG_FLAGS_VR_MODE						= ( 1 << 17 ),
};

struct MaterialSystemHardwareIdentifier_t
{
	char *m_pCardName;
	unsigned int m_nVendorID;
	unsigned int m_nDeviceID;
};

struct MaterialSystem_Config_t
{
	bool Windowed() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_WINDOWED ) != 0; }
	bool Resizing() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_RESIZING ) != 0; }
#ifdef CSS_PERF_TEST
	bool WaitForVSync() const { return false; }//( m_Flags & MATSYS_VIDCFG_FLAGS_NO_WAIT_FOR_VSYNC ) == 0; }
#else
	bool WaitForVSync() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_NO_WAIT_FOR_VSYNC ) == 0; }
#endif
	bool Stencil() const { return (m_Flags & MATSYS_VIDCFG_FLAGS_STENCIL ) != 0; }
	bool ForceTrilinear() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_FORCE_TRILINEAR ) != 0; }
	bool ForceHWSync() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_FORCE_HWSYNC ) != 0; }
	bool UseSpecular() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_DISABLE_SPECULAR ) == 0; }
	bool UseBumpmapping() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_DISABLE_BUMPMAP ) == 0; }
	bool UseParallaxMapping() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_ENABLE_PARALLAX_MAPPING ) != 0; }
	bool UseZPrefill() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_USE_Z_PREFILL ) != 0; }
	bool ReduceFillrate() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_REDUCE_FILLRATE ) != 0; }
	bool HDREnabled() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_ENABLE_HDR ) != 0; }
	bool LimitWindowedSize() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_LIMIT_WINDOWED_SIZE ) != 0; }
	bool ScaleToOutputResolution() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_SCALE_TO_OUTPUT_RESOLUTION ) != 0; }
	bool UsingMultipleWindows() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_USING_MULTIPLE_WINDOWS ) != 0; }
	bool UsePhong() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_DISABLE_PHONG ) == 0; }
	bool VRMode() const { return ( m_Flags & MATSYS_VIDCFG_FLAGS_VR_MODE) != 0; }
	bool ShadowDepthTexture() const { return m_bShadowDepthTexture; }
	bool MotionBlur() const { return m_bMotionBlur; }
	bool SupportFlashlight() const { return m_bSupportFlashlight; }

	void SetFlag( unsigned int flag, bool val )
	{
		if( val )
		{
			m_Flags |= flag;	
		}
		else
		{
			m_Flags &= ~flag;	
		}
	}
	
	// control panel stuff
	MaterialVideoMode_t m_VideoMode;
	float m_fMonitorGamma;
	float m_fGammaTVRangeMin;
	float m_fGammaTVRangeMax;
	float m_fGammaTVExponent;
	bool m_bGammaTVEnabled;

	int m_nAASamples;
	int m_nForceAnisotropicLevel;
	int skipMipLevels;
	int dxSupportLevel;
	unsigned int m_Flags;
	bool bEditMode;				// true if in Hammer.
	unsigned char proxiesTestMode;	// 0 = normal, 1 = no proxies, 2 = alpha test all, 3 = color mod all
	bool bCompressedTextures;
	bool bFilterLightmaps;
	bool bFilterTextures;
	bool bReverseDepth;
	bool bBufferPrimitives;
	bool bDrawFlat;
	bool bMeasureFillRate;
	bool bVisualizeFillRate;
	bool bNoTransparency;
	bool bSoftwareLighting;
	bool bAllowCheats;
	char nShowMipLevels;
	bool bShowLowResImage;
	bool bShowNormalMap; 
	bool bMipMapTextures;
	unsigned char nFullbright;
	bool m_bFastNoBump;
	bool m_bSuppressRendering;

	// debug modes
	bool bShowSpecular; // This is the fast version that doesn't require reloading materials
	bool bShowDiffuse;  // This is the fast version that doesn't require reloading materials

	// misc
	int m_nReserved;	// Currently unused

	// No depth bias
	float m_SlopeScaleDepthBias_Normal;
	float m_DepthBias_Normal;

	// Depth bias for rendering decals closer to the camera
	float m_SlopeScaleDepthBias_Decal;
	float m_DepthBias_Decal;

	// Depth bias for biasing shadow depth map rendering away from the camera
	float m_SlopeScaleDepthBias_ShadowMap;
	float m_DepthBias_ShadowMap;

	uint m_WindowedSizeLimitWidth;
	uint m_WindowedSizeLimitHeight;
	int m_nAAQuality;
	bool m_bShadowDepthTexture;
	bool m_bMotionBlur;
	bool m_bSupportFlashlight;

	int m_nVRModeAdapter;

	MaterialSystem_Config_t()
	{
		memset( this, 0, sizeof( *this ) );

		// video config defaults
		SetFlag( MATSYS_VIDCFG_FLAGS_WINDOWED, false );
		SetFlag( MATSYS_VIDCFG_FLAGS_RESIZING, false );
		SetFlag( MATSYS_VIDCFG_FLAGS_NO_WAIT_FOR_VSYNC, true );
		SetFlag( MATSYS_VIDCFG_FLAGS_STENCIL, false );
		SetFlag( MATSYS_VIDCFG_FLAGS_FORCE_TRILINEAR, true );
		SetFlag( MATSYS_VIDCFG_FLAGS_FORCE_HWSYNC, true );
		SetFlag( MATSYS_VIDCFG_FLAGS_DISABLE_SPECULAR, false );
		SetFlag( MATSYS_VIDCFG_FLAGS_DISABLE_BUMPMAP, false );
		SetFlag( MATSYS_VIDCFG_FLAGS_ENABLE_PARALLAX_MAPPING, true );
		SetFlag( MATSYS_VIDCFG_FLAGS_USE_Z_PREFILL, false );
		SetFlag( MATSYS_VIDCFG_FLAGS_REDUCE_FILLRATE, false );
		SetFlag( MATSYS_VIDCFG_FLAGS_LIMIT_WINDOWED_SIZE, false );
		SetFlag( MATSYS_VIDCFG_FLAGS_SCALE_TO_OUTPUT_RESOLUTION, false );
		SetFlag( MATSYS_VIDCFG_FLAGS_USING_MULTIPLE_WINDOWS, false );
		SetFlag( MATSYS_VIDCFG_FLAGS_DISABLE_PHONG, false );
		SetFlag( MATSYS_VIDCFG_FLAGS_VR_MODE, false );

		m_VideoMode.m_Width = 640;
		m_VideoMode.m_Height = 480;
		m_VideoMode.m_RefreshRate = 60;
		dxSupportLevel = 0;
		bCompressedTextures = true;
		bFilterTextures = true;
		bFilterLightmaps = true;
		bMipMapTextures = true;
		bBufferPrimitives = true;

		m_fMonitorGamma = 2.2f;
		m_fGammaTVRangeMin = 16.0f;
		m_fGammaTVRangeMax = 255.0f;
		m_fGammaTVExponent = 2.5;
		m_bGammaTVEnabled = IsX360();

		m_nAASamples = 1;
		m_bShadowDepthTexture = false;
		m_bMotionBlur = false;
		m_bSupportFlashlight = true;

		m_nVRModeAdapter = -1;

		// misc defaults
		bAllowCheats = false;
		bCompressedTextures = true;
		bEditMode = false;

		// debug modes
		bShowSpecular = true;
		bShowDiffuse = true;
		nFullbright = 0;
		bShowNormalMap = false;
		bFilterLightmaps = true;
		bFilterTextures = true;
		bMipMapTextures = true;
		nShowMipLevels = 0;
		bShowLowResImage = false;
		bReverseDepth = false;
		bBufferPrimitives = true;
		bDrawFlat = false;
		bMeasureFillRate = false;
		bVisualizeFillRate = false;
		bSoftwareLighting = false;
		bNoTransparency = false;
		proxiesTestMode = 0;
		m_bFastNoBump = false;
		m_bSuppressRendering = false;
		m_SlopeScaleDepthBias_Decal = -0.5f;
		m_SlopeScaleDepthBias_Normal = 0.0f;
		m_SlopeScaleDepthBias_ShadowMap = 0.5f;
		m_DepthBias_Decal = -262144;
		m_DepthBias_Normal = 0.0f;
		m_DepthBias_ShadowMap = 262144;
		m_WindowedSizeLimitWidth = 1280;
		m_WindowedSizeLimitHeight = 1024;
	}
};


#endif // MATERIALSYSTEM_CONFIG_H
