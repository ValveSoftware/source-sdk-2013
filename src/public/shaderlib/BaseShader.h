//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
// This is what all shaders inherit from.
//===========================================================================//

#ifndef BASESHADER_H
#define BASESHADER_H

#ifdef _WIN32		   
#pragma once
#endif

#include "materialsystem/IShader.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/ishaderapi.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "shaderlib/BaseShader.h"
#include "convar.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IMaterialVar;

//-----------------------------------------------------------------------------
// Standard material vars
//-----------------------------------------------------------------------------
// Note: if you add to these, add to s_StandardParams in CBaseShader.cpp
enum ShaderMaterialVars_t
{
	FLAGS = 0,
	FLAGS_DEFINED,	// mask indicating if the flag was specified
	FLAGS2,
	FLAGS_DEFINED2,
	COLOR,
	ALPHA,
	BASETEXTURE,
	FRAME,
	BASETEXTURETRANSFORM,
	FLASHLIGHTTEXTURE,
	FLASHLIGHTTEXTUREFRAME,
	COLOR2,
	SRGBTINT,

	NUM_SHADER_MATERIAL_VARS
};


// Alpha belnd mode enums. Moved from basevsshader
enum BlendType_t
{
	// no alpha blending
	BT_NONE = 0,



	// src * srcAlpha + dst * (1-srcAlpha)
	// two passes for HDR:
	//		pass 1:
	//			color: src * srcAlpha + dst * (1-srcAlpha)
	//			alpha: srcAlpha * zero + dstAlpha * (1-srcAlpha)
	//		pass 2:
	//			color: none
	//			alpha: srcAlpha * one + dstAlpha * one
	//
	BT_BLEND,


	
	// src * one + dst * one
	// one pass for HDR
	BT_ADD,


	
	// Why do we ever use this instead of using premultiplied alpha?
	// src * srcAlpha + dst * one
	// two passes for HDR
	//		pass 1:
	//			color: src * srcAlpha + dst * one
	//			alpha: srcAlpha * one + dstAlpha * one
	//		pass 2:
	//			color: none
	//			alpha: srcAlpha * one + dstAlpha * one
	BT_BLENDADD
};


//-----------------------------------------------------------------------------
// Base class for shaders, contains helper methods.
//-----------------------------------------------------------------------------
class CBaseShader : public IShader
{
public:
	// constructor
	CBaseShader();

	// Methods inherited from IShader
	virtual char const* GetFallbackShader( IMaterialVar** params ) const { return 0; }
	virtual int GetNumParams( ) const;
	virtual char const* GetParamName( int paramIndex ) const;
	virtual char const* GetParamHelp( int paramIndex ) const;
	virtual ShaderParamType_t GetParamType( int paramIndex ) const;
	virtual char const* GetParamDefault( int paramIndex ) const;
	virtual int GetParamFlags( int nParamIndex ) const;

	virtual void InitShaderParams( IMaterialVar** ppParams, const char *pMaterialName );
	virtual void InitShaderInstance( IMaterialVar** ppParams, IShaderInit *pShaderInit, const char *pMaterialName, const char *pTextureGroupName );
	virtual void DrawElements( IMaterialVar **params, int nModulationFlags, IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI,
								VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContext );

	virtual	const SoftwareVertexShader_t GetSoftwareVertexShader() const { return m_SoftwareVertexShader; }

	virtual int ComputeModulationFlags( IMaterialVar** params, IShaderDynamicAPI* pShaderAPI );
	virtual bool NeedsPowerOfTwoFrameBufferTexture( IMaterialVar **params, bool bCheckSpecificToThisFrame = true ) const;
	virtual bool NeedsFullFrameBufferTexture( IMaterialVar **params, bool bCheckSpecificToThisFrame = true ) const;
	virtual bool IsTranslucent( IMaterialVar **params ) const;

public:
	// These functions must be implemented by the shader
	virtual void OnInitShaderParams( IMaterialVar** ppParams, const char *pMaterialName ) {}
	virtual void OnInitShaderInstance( IMaterialVar** ppParams, IShaderInit *pShaderInit, const char *pMaterialName ) = 0;
	virtual void OnDrawElements( IMaterialVar **params, IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr ) = 0;

	// Sets the default shadow state
	void SetInitialShadowState( );
 
	// Draws a snapshot
	void Draw( bool bMakeActualDrawCall = true );

	// Are we currently taking a snapshot?
	bool IsSnapshotting() const;

	// Gets at the current materialvar flags
	int CurrentMaterialVarFlags() const;

	// Finds a particular parameter	(works because the lowest parameters match the shader)
	int FindParamIndex( const char *pName ) const;

	// Are we using graphics?
	bool IsUsingGraphics();

	// Are we using editor materials?
	bool CanUseEditorMaterials();

	// Gets the builder...
	CMeshBuilder* MeshBuilder();

	// Loads a texture
	void LoadTexture( int nTextureVar, int nAdditionalCreationFlags = 0 );

	// Loads a bumpmap
	void LoadBumpMap( int nTextureVar );

	// Loads a cubemap
	void LoadCubeMap( int nTextureVar, int nAdditionalCreationFlags = 0  );

	// get the shaderapi handle for a texture. BE CAREFUL WITH THIS. 
	ShaderAPITextureHandle_t GetShaderAPITextureBindHandle( int nTextureVar, int nFrameVar, int nTextureChannel = 0 );


	// Binds a texture
	void BindTexture( Sampler_t sampler1, Sampler_t sampler2, int nTextureVar, int nFrameVar = -1 );
	void BindTexture( Sampler_t sampler1, int nTextureVar, int nFrameVar = -1 );
	void BindTexture( Sampler_t sampler1, ITexture *pTexture, int nFrame = 0 );
	void BindTexture( Sampler_t sampler1, Sampler_t sampler2, ITexture *pTexture, int nFrame = 0 );

	void GetTextureDimensions( float* pOutWidth, float* pOutHeight, int nTextureVar );

	// Is the texture translucent?
	bool TextureIsTranslucent( int textureVar, bool isBaseTexture );

	// Returns the translucency...
	float GetAlpha( IMaterialVar** params = NULL );

	// Is the color var white?
	bool IsWhite( int colorVar );

	// Helper methods for fog
	void FogToOOOverbright( void );
	void FogToWhite( void );
	void FogToBlack( void );
	void FogToGrey( void );
	void FogToFogColor( void );
	void DisableFog( void );
	void DefaultFog( void );
	
	// Helpers for alpha blending
	void EnableAlphaBlending( ShaderBlendFactor_t src, ShaderBlendFactor_t dst );
	void DisableAlphaBlending();

	void SetBlendingShadowState( BlendType_t nMode );

	void SetNormalBlendingShadowState( int textureVar = -1, bool isBaseTexture = true );
	void SetAdditiveBlendingShadowState( int textureVar = -1, bool isBaseTexture = true );
	void SetDefaultBlendingShadowState( int textureVar = -1, bool isBaseTexture = true );
	void SingleTextureLightmapBlendMode( );

	// Helpers for color modulation
	void SetColorState( int colorVar, bool setAlpha = false );
	bool IsAlphaModulating();
	bool IsColorModulating();
	void ComputeModulationColor( float* color );
	void SetModulationShadowState( int tintVar = -1 );
	void SetModulationDynamicState( int tintVar = -1 );

	// Helpers for HDR
	bool IsHDREnabled( void );

	// Loads the identity matrix into the texture
	void LoadIdentity( MaterialMatrixMode_t matrixMode );

	// Loads the camera to world transform
	void LoadCameraToWorldTransform( MaterialMatrixMode_t matrixMode );
	void LoadCameraSpaceSphereMapTransform( MaterialMatrixMode_t matrixMode );

	// Sets a texture translation transform in fixed function
	void SetFixedFunctionTextureTranslation( MaterialMatrixMode_t mode, int translationVar );
	void SetFixedFunctionTextureScale( MaterialMatrixMode_t mode, int scaleVar );
	void SetFixedFunctionTextureScaledTransform( MaterialMatrixMode_t textureTransform, int transformVar, int scaleVar );
	void SetFixedFunctionTextureTransform( MaterialMatrixMode_t textureTransform, int transformVar );

	void CleanupDynamicStateFixedFunction( );

	// Fixed function Base * detail pass
	void FixedFunctionBaseTimesDetailPass( int baseTextureVar, int frameVar, 
		int baseTextureTransformVar, int detailVar, int detailScaleVar );

	// Fixed function Self illumination pass
	void FixedFunctionSelfIlluminationPass( Sampler_t sampler, 
		int baseTextureVar, int frameVar, int baseTextureTransformVar, int selfIllumTintVar );

	// Masked environment map
	void FixedFunctionMaskedEnvmapPass( int envMapVar, int envMapMaskVar, 
		int baseTextureVar, int envMapFrameVar, int envMapMaskFrameVar, 
		int frameVar, int maskOffsetVar, int maskScaleVar, int tintVar = -1 );

	// Additive masked environment map
	void FixedFunctionAdditiveMaskedEnvmapPass( int envMapVar, int envMapMaskVar, 
		int baseTextureVar, int envMapFrameVar, int envMapMaskFrameVar, 
		int frameVar, int maskOffsetVar, int maskScaleVar, int tintVar = -1 );

	// Modulate by detail texture pass
	void FixedFunctionMultiplyByDetailPass( int baseTextureVar, int frameVar, 
		int textureOffsetVar, int detailVar, int detailScaleVar );

	// Multiply by lightmap pass
	void FixedFunctionMultiplyByLightmapPass( int baseTextureVar, int frameVar, 
		int baseTextureTransformVar, float alphaOverride = -1 );

 	// Helper methods for environment mapping
	int SetShadowEnvMappingState( int envMapMaskVar, int tintVar = -1 );
	void SetDynamicEnvMappingState( int envMapVar, int envMapMaskVar, 
		int baseTextureVar, int envMapFrameVar, int envMapMaskFrameVar, 
		int frameVar, int maskOffsetVar, int maskScaleVar, int tintVar = -1 );

	bool UsingFlashlight( IMaterialVar **params ) const;
	bool UsingEditor( IMaterialVar **params ) const;

	void DrawFlashlight_dx70( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, 
							  IShaderShadow* pShaderShadow, 
							  int flashlightTextureVar, int flashlightTextureFrameVar, 
							  bool suppress_lighting = false );

	void SetFlashlightFixedFunctionTextureTransform( MaterialMatrixMode_t matrix );

	void GetColorParameter( IMaterialVar** params, float *pColorOut ) const; // return tint color (color*color2)
	void ApplyColor2Factor( float *pColorOut ) const;		// (*pColorOut) *= COLOR2

	static IMaterialVar **s_ppParams;

protected:
	SoftwareVertexShader_t m_SoftwareVertexShader;

	static const char *s_pTextureGroupName; // Current material's texture group name.
	static IShaderShadow *s_pShaderShadow;
	static IShaderDynamicAPI *s_pShaderAPI;
	static IShaderInit *s_pShaderInit;

private:
	static int s_nModulationFlags;
	static CMeshBuilder *s_pMeshBuilder;
};


//-----------------------------------------------------------------------------
// Gets at the current materialvar flags
//-----------------------------------------------------------------------------
inline int CBaseShader::CurrentMaterialVarFlags() const
{
	return s_ppParams[FLAGS]->GetIntValue();
}

//-----------------------------------------------------------------------------
// Are we currently taking a snapshot?
//-----------------------------------------------------------------------------
inline bool CBaseShader::IsSnapshotting() const
{
	return (s_pShaderShadow != NULL);
}

//-----------------------------------------------------------------------------
// Is the color var white?
//-----------------------------------------------------------------------------
inline bool CBaseShader::IsWhite( int colorVar )
{
	if (colorVar < 0)
		return true;

	if (!s_ppParams[colorVar]->IsDefined())
		return true;

	float color[3];
	s_ppParams[colorVar]->GetVecValue( color, 3 );
	return (color[0] >= 1.0f) && (color[1] >= 1.0f) && (color[2] >= 1.0f);
}


class CBasePerMaterialContextData								// shaders can keep per material data in classes descended from this
{
 public:
	uint32 m_nVarChangeID;
	bool m_bMaterialVarsChanged;							// set by mat system when material vars change. shader should rehtink and then clear the var

	FORCEINLINE CBasePerMaterialContextData( void )
	{
		m_bMaterialVarsChanged = true;
		m_nVarChangeID = 0xffffffff;
	}

	// virtual destructor so that derived classes can have their own data to be cleaned up on
	// delete of material
	virtual ~CBasePerMaterialContextData( void )
	{
	}
};

#endif // BASESHADER_H
