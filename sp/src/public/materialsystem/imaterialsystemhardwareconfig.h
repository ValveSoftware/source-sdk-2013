//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#ifndef IMATERIALSYSTEMHARDWARECONFIG_H
#define IMATERIALSYSTEMHARDWARECONFIG_H

#ifdef _WIN32
#pragma once
#endif


#include "tier1/interface.h"

//-----------------------------------------------------------------------------
// GL helpers
//-----------------------------------------------------------------------------
FORCEINLINE bool IsEmulatingGL()
{
	static bool bIsEmulatingGL = ( Plat_GetCommandLineA() ) ? ( strstr( Plat_GetCommandLineA(), "-r_emulate_gl" ) != NULL ) : false;
	return bIsEmulatingGL;
}

FORCEINLINE bool IsOpenGL( void )
{
	return IsPlatformOpenGL() || IsEmulatingGL();
}

//-----------------------------------------------------------------------------
// Material system interface version
//-----------------------------------------------------------------------------
#define MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION		"MaterialSystemHardwareConfig012"

// HDRFIXME NOTE: must match common_ps_fxc.h
enum HDRType_t
{
	HDR_TYPE_NONE,
	HDR_TYPE_INTEGER,
	HDR_TYPE_FLOAT,
};

// For now, vertex compression is simply "on or off" (for the sake of simplicity
// and MeshBuilder perf.), but later we may support multiple flavours.
enum VertexCompressionType_t
{
	// This indicates an uninitialized VertexCompressionType_t value
	VERTEX_COMPRESSION_INVALID = 0xFFFFFFFF,

	// 'VERTEX_COMPRESSION_NONE' means that no elements of a vertex are compressed
	VERTEX_COMPRESSION_NONE = 0,

	// Currently (more stuff may be added as needed), 'VERTEX_COMPRESSION_ON' means:
	//  - if a vertex contains VERTEX_ELEMENT_NORMAL, this is compressed
	//    (see CVertexBuilder::CompressedNormal3f)
	//  - if a vertex contains VERTEX_ELEMENT_USERDATA4 (and a normal - together defining a tangent
	//    frame, with the binormal reconstructed in the vertex shader), this is compressed
	//    (see CVertexBuilder::CompressedUserData)
	//  - if a vertex contains VERTEX_ELEMENT_BONEWEIGHTSx, this is compressed
	//    (see CVertexBuilder::CompressedBoneWeight3fv)
	VERTEX_COMPRESSION_ON = 1
};


// use DEFCONFIGMETHOD to define time-critical methods that we want to make just return constants
// on the 360, so that the checks will happen at compile time. Not all methods are defined this way
// - just the ones that I perceive as being called often in the frame interval.
#ifdef _X360
#define DEFCONFIGMETHOD( ret_type, method, xbox_return_value )		\
FORCEINLINE ret_type method const 									\
{																	\
	return xbox_return_value;										\
}


#else
#define DEFCONFIGMETHOD( ret_type, method, xbox_return_value )	\
virtual ret_type method const = 0;
#endif



//-----------------------------------------------------------------------------
// Material system configuration
//-----------------------------------------------------------------------------
class IMaterialSystemHardwareConfig
{
public:
	// on xbox, some methods are inlined to return constants

	DEFCONFIGMETHOD( bool, HasDestAlphaBuffer(), true );
	DEFCONFIGMETHOD( bool, HasStencilBuffer(), true );
	virtual int	 GetFrameBufferColorDepth() const = 0;
	virtual int  GetSamplerCount() const = 0;
	virtual bool HasSetDeviceGammaRamp() const = 0;
	DEFCONFIGMETHOD( bool, SupportsCompressedTextures(), true );
	virtual VertexCompressionType_t SupportsCompressedVertices() const = 0;
	DEFCONFIGMETHOD( bool, SupportsNormalMapCompression(), true );
	DEFCONFIGMETHOD( bool, SupportsVertexAndPixelShaders(), true );
	DEFCONFIGMETHOD( bool, SupportsPixelShaders_1_4(), true );
	DEFCONFIGMETHOD( bool, SupportsStaticControlFlow(), true );
	DEFCONFIGMETHOD( bool, SupportsPixelShaders_2_0(), true );
	DEFCONFIGMETHOD( bool,  SupportsVertexShaders_2_0(), true );
	virtual int  MaximumAnisotropicLevel() const = 0;	// 0 means no anisotropic filtering
	virtual int  MaxTextureWidth() const = 0;
	virtual int  MaxTextureHeight() const = 0;
	virtual int	 TextureMemorySize() const = 0;
	virtual bool SupportsOverbright() const = 0;
	virtual bool SupportsCubeMaps() const = 0;
	virtual bool SupportsMipmappedCubemaps() const = 0;
	virtual bool SupportsNonPow2Textures() const = 0;

	// The number of texture stages represents the number of computations
	// we can do in the fixed-function pipeline, it is *not* related to the
	// simultaneous number of textures we can use
	virtual int  GetTextureStageCount() const = 0;
	virtual int	 NumVertexShaderConstants() const = 0;
	virtual int	 NumPixelShaderConstants() const = 0;
	virtual int	 MaxNumLights() const = 0;
	virtual bool SupportsHardwareLighting() const = 0;
	virtual int	 MaxBlendMatrices() const = 0;
	virtual int	 MaxBlendMatrixIndices() const = 0;
	virtual int	 MaxTextureAspectRatio() const = 0;
	virtual int	 MaxVertexShaderBlendMatrices() const = 0;
	virtual int	 MaxUserClipPlanes() const = 0;
	virtual bool UseFastClipping() const = 0;

	// This here should be the major item looked at when checking for compat
	// from anywhere other than the material system	shaders
	DEFCONFIGMETHOD( int, GetDXSupportLevel(), 98 );
	virtual const char *GetShaderDLLName() const = 0;

	virtual bool ReadPixelsFromFrontBuffer() const = 0;

	// Are dx dynamic textures preferred?
	virtual bool PreferDynamicTextures() const = 0;

	DEFCONFIGMETHOD( bool, SupportsHDR(), true );

	virtual bool HasProjectedBumpEnv() const = 0;
	virtual bool SupportsSpheremapping() const = 0;
	virtual bool NeedsAAClamp() const = 0;
	virtual bool NeedsATICentroidHack() const = 0;

	virtual bool SupportsColorOnSecondStream() const = 0;
	virtual bool SupportsStaticPlusDynamicLighting() const = 0;

	// Does our card have a hard time with fillrate 
	// relative to other cards w/ the same dx level?
	virtual bool PreferReducedFillrate() const = 0;

	// This is the max dx support level supported by the card
	virtual int	 GetMaxDXSupportLevel() const = 0;

	// Does the card specify fog color in linear space when sRGBWrites are enabled?
	virtual bool SpecifiesFogColorInLinearSpace() const = 0;

	// Does the card support sRGB reads/writes?
	DEFCONFIGMETHOD( bool, SupportsSRGB(), true );
	DEFCONFIGMETHOD( bool, FakeSRGBWrite(), false );
	DEFCONFIGMETHOD( bool, CanDoSRGBReadFromRTs(), true );

	virtual bool SupportsGLMixedSizeTargets() const = 0;

	virtual bool IsAAEnabled() const = 0;	// Is antialiasing being used?

	// NOTE: Anything after this was added after shipping HL2.
	virtual int GetVertexTextureCount() const = 0;
	virtual int GetMaxVertexTextureDimension() const = 0;

	virtual int  MaxTextureDepth() const = 0;

	virtual HDRType_t GetHDRType() const = 0;
	virtual HDRType_t GetHardwareHDRType() const = 0;

	DEFCONFIGMETHOD( bool, SupportsPixelShaders_2_b(), true );
	virtual bool SupportsStreamOffset() const = 0;

	virtual int StencilBufferBits() const = 0;
	virtual int MaxViewports() const = 0;

	virtual void OverrideStreamOffsetSupport( bool bOverrideEnabled, bool bEnableSupport ) = 0;

	virtual int GetShadowFilterMode() const = 0;

	virtual int NeedsShaderSRGBConversion() const = 0;

	DEFCONFIGMETHOD( bool, UsesSRGBCorrectBlending(), true );

	virtual bool SupportsShaderModel_3_0() const = 0;
	virtual bool HasFastVertexTextures() const = 0;
	virtual int MaxHWMorphBatchCount() const = 0;

	// Does the board actually support this?
	DEFCONFIGMETHOD( bool, ActuallySupportsPixelShaders_2_b(), true );

	virtual bool SupportsHDRMode( HDRType_t nHDRMode ) const = 0;

	virtual bool GetHDREnabled( void ) const = 0;
	virtual void SetHDREnabled( bool bEnable ) = 0;

	virtual bool SupportsBorderColor( void ) const = 0;
	virtual bool SupportsFetch4( void ) const = 0;

	inline bool ShouldAlwaysUseShaderModel2bShaders() const { return IsOpenGL(); }
	inline bool PlatformRequiresNonNullPixelShaders() const { return IsOpenGL(); }
};

#endif // IMATERIALSYSTEMHARDWARECONFIG_H
