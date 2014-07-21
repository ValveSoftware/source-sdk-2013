//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef ISHADERSHADOW_H
#define ISHADERSHADOW_H

#ifdef _WIN32
#pragma once
#endif

#include "shaderapi/shareddefs.h"
#include <materialsystem/imaterial.h>


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class CMeshBuilder;
class IMaterialVar;
struct LightDesc_t; 


//-----------------------------------------------------------------------------
// important enumerations
//-----------------------------------------------------------------------------
enum ShaderDepthFunc_t 
{ 
	SHADER_DEPTHFUNC_NEVER,
	SHADER_DEPTHFUNC_NEARER,
	SHADER_DEPTHFUNC_EQUAL,
	SHADER_DEPTHFUNC_NEAREROREQUAL,
	SHADER_DEPTHFUNC_FARTHER,
	SHADER_DEPTHFUNC_NOTEQUAL,
	SHADER_DEPTHFUNC_FARTHEROREQUAL,
	SHADER_DEPTHFUNC_ALWAYS
};

enum ShaderBlendFactor_t
{
	SHADER_BLEND_ZERO,
	SHADER_BLEND_ONE,
	SHADER_BLEND_DST_COLOR,
	SHADER_BLEND_ONE_MINUS_DST_COLOR,
	SHADER_BLEND_SRC_ALPHA,
	SHADER_BLEND_ONE_MINUS_SRC_ALPHA,
	SHADER_BLEND_DST_ALPHA,
	SHADER_BLEND_ONE_MINUS_DST_ALPHA,
	SHADER_BLEND_SRC_ALPHA_SATURATE,
	SHADER_BLEND_SRC_COLOR,
	SHADER_BLEND_ONE_MINUS_SRC_COLOR
};

enum ShaderBlendOp_t
{
	SHADER_BLEND_OP_ADD,
	SHADER_BLEND_OP_SUBTRACT,
	SHADER_BLEND_OP_REVSUBTRACT,
	SHADER_BLEND_OP_MIN,
	SHADER_BLEND_OP_MAX
};

enum ShaderAlphaFunc_t
{
	SHADER_ALPHAFUNC_NEVER,
	SHADER_ALPHAFUNC_LESS,
	SHADER_ALPHAFUNC_EQUAL,
	SHADER_ALPHAFUNC_LEQUAL,
	SHADER_ALPHAFUNC_GREATER,
	SHADER_ALPHAFUNC_NOTEQUAL,
	SHADER_ALPHAFUNC_GEQUAL,
	SHADER_ALPHAFUNC_ALWAYS
};

enum ShaderStencilFunc_t 
{ 
	SHADER_STENCILFUNC_NEVER = 0,
	SHADER_STENCILFUNC_LESS,
	SHADER_STENCILFUNC_EQUAL,
	SHADER_STENCILFUNC_LEQUAL,
	SHADER_STENCILFUNC_GREATER,
	SHADER_STENCILFUNC_NOTEQUAL,
	SHADER_STENCILFUNC_GEQUAL,
	SHADER_STENCILFUNC_ALWAYS
};

enum ShaderStencilOp_t 
{ 
	SHADER_STENCILOP_KEEP = 0,
	SHADER_STENCILOP_ZERO,
	SHADER_STENCILOP_SET_TO_REFERENCE,
	SHADER_STENCILOP_INCREMENT_CLAMP,
	SHADER_STENCILOP_DECREMENT_CLAMP,
	SHADER_STENCILOP_INVERT,
	SHADER_STENCILOP_INCREMENT_WRAP,
	SHADER_STENCILOP_DECREMENT_WRAP,
};

enum ShaderTexChannel_t
{
	SHADER_TEXCHANNEL_COLOR = 0,
	SHADER_TEXCHANNEL_ALPHA
};

enum ShaderPolyModeFace_t
{
	SHADER_POLYMODEFACE_FRONT,
	SHADER_POLYMODEFACE_BACK,
	SHADER_POLYMODEFACE_FRONT_AND_BACK,
};

enum ShaderPolyMode_t
{
	SHADER_POLYMODE_POINT,
	SHADER_POLYMODE_LINE,
	SHADER_POLYMODE_FILL
};

enum ShaderTexArg_t
{
	SHADER_TEXARG_TEXTURE = 0,
	SHADER_TEXARG_VERTEXCOLOR,
	SHADER_TEXARG_SPECULARCOLOR,
	SHADER_TEXARG_CONSTANTCOLOR,
	SHADER_TEXARG_PREVIOUSSTAGE,
	SHADER_TEXARG_NONE,
	SHADER_TEXARG_ZERO,
	SHADER_TEXARG_TEXTUREALPHA,
	SHADER_TEXARG_INVTEXTUREALPHA,
	SHADER_TEXARG_ONE,
};

enum ShaderTexOp_t
{
	// DX5 shaders support these
	SHADER_TEXOP_MODULATE = 0,
	SHADER_TEXOP_MODULATE2X,
	SHADER_TEXOP_MODULATE4X,
	SHADER_TEXOP_SELECTARG1,
	SHADER_TEXOP_SELECTARG2,
	SHADER_TEXOP_DISABLE,

	// DX6 shaders support these
	SHADER_TEXOP_ADD,
	SHADER_TEXOP_SUBTRACT,
	SHADER_TEXOP_ADDSIGNED2X,
	SHADER_TEXOP_BLEND_CONSTANTALPHA,
	SHADER_TEXOP_BLEND_TEXTUREALPHA,
	SHADER_TEXOP_BLEND_PREVIOUSSTAGEALPHA,
	SHADER_TEXOP_MODULATECOLOR_ADDALPHA,
	SHADER_TEXOP_MODULATEINVCOLOR_ADDALPHA,

	// DX7
	SHADER_TEXOP_DOTPRODUCT3
};

enum ShaderTexGenParam_t
{
	SHADER_TEXGENPARAM_OBJECT_LINEAR,
	SHADER_TEXGENPARAM_EYE_LINEAR,
	SHADER_TEXGENPARAM_SPHERE_MAP,
	SHADER_TEXGENPARAM_CAMERASPACEREFLECTIONVECTOR,
	SHADER_TEXGENPARAM_CAMERASPACENORMAL
};

enum ShaderDrawBitField_t
{
	SHADER_DRAW_POSITION			= 0x0001,
	SHADER_DRAW_NORMAL				= 0x0002,
	SHADER_DRAW_COLOR				= 0x0004,
	SHADER_DRAW_SPECULAR			= 0x0008,

	SHADER_DRAW_TEXCOORD0			= 0x0010,
	SHADER_DRAW_TEXCOORD1			= 0x0020,
	SHADER_DRAW_TEXCOORD2			= 0x0040,
	SHADER_DRAW_TEXCOORD3			= 0x0080,

	SHADER_DRAW_LIGHTMAP_TEXCOORD0	= 0x0100,
	SHADER_DRAW_LIGHTMAP_TEXCOORD1	= 0x0200,
	SHADER_DRAW_LIGHTMAP_TEXCOORD2	= 0x0400,
	SHADER_DRAW_LIGHTMAP_TEXCOORD3	= 0x0800,

	SHADER_DRAW_SECONDARY_TEXCOORD0	= 0x1000,
	SHADER_DRAW_SECONDARY_TEXCOORD1	= 0x2000,
	SHADER_DRAW_SECONDARY_TEXCOORD2	= 0x4000,
	SHADER_DRAW_SECONDARY_TEXCOORD3	= 0x8000,

	SHADER_TEXCOORD_MASK = SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_TEXCOORD1 | 
							SHADER_DRAW_TEXCOORD2 | SHADER_DRAW_TEXCOORD3,

	SHADER_LIGHTMAP_TEXCOORD_MASK = SHADER_DRAW_LIGHTMAP_TEXCOORD0 | 
									SHADER_DRAW_LIGHTMAP_TEXCOORD1 | 
									SHADER_DRAW_LIGHTMAP_TEXCOORD2 | 
									SHADER_DRAW_LIGHTMAP_TEXCOORD3,

	SHADER_SECONDARY_TEXCOORD_MASK = SHADER_DRAW_SECONDARY_TEXCOORD0 | 
									SHADER_DRAW_SECONDARY_TEXCOORD1 | 
									SHADER_DRAW_SECONDARY_TEXCOORD2 | 
									SHADER_DRAW_SECONDARY_TEXCOORD3,
};


enum ShaderFogMode_t
{
	SHADER_FOGMODE_DISABLED = 0,
	SHADER_FOGMODE_OO_OVERBRIGHT,
	SHADER_FOGMODE_BLACK,
	SHADER_FOGMODE_GREY,
	SHADER_FOGMODE_FOGCOLOR,
	SHADER_FOGMODE_WHITE,
	SHADER_FOGMODE_NUMFOGMODES
};

enum ShaderMaterialSource_t
{
	SHADER_MATERIALSOURCE_MATERIAL = 0,
	SHADER_MATERIALSOURCE_COLOR1,
	SHADER_MATERIALSOURCE_COLOR2,
};


// m_ZBias has only two bits in ShadowState_t, so be careful extending this enum
enum PolygonOffsetMode_t
{
	SHADER_POLYOFFSET_DISABLE		= 0x0,
	SHADER_POLYOFFSET_DECAL		= 0x1,
	SHADER_POLYOFFSET_SHADOW_BIAS	= 0x2,
	SHADER_POLYOFFSET_RESERVED		= 0x3	// Reserved for future use
};


//-----------------------------------------------------------------------------
// The Shader interface versions
//-----------------------------------------------------------------------------
#define SHADERSHADOW_INTERFACE_VERSION	"ShaderShadow010"


//-----------------------------------------------------------------------------
// the shader API interface (methods called from shaders)
//-----------------------------------------------------------------------------
abstract_class IShaderShadow
{
public:
	// Sets the default *shadow* state
	virtual void SetDefaultState() = 0;

	// Methods related to depth buffering
	virtual void DepthFunc( ShaderDepthFunc_t depthFunc ) = 0;
	virtual void EnableDepthWrites( bool bEnable ) = 0;
	virtual void EnableDepthTest( bool bEnable ) = 0;
	virtual void EnablePolyOffset( PolygonOffsetMode_t nOffsetMode ) = 0;

	// These methods for controlling stencil are obsolete and stubbed to do nothing.  Stencil
	// control is via the shaderapi/material system now, not part of the shadow state.
	// Methods related to stencil
	virtual void EnableStencil( bool bEnable ) = 0;
	virtual void StencilFunc( ShaderStencilFunc_t stencilFunc ) = 0;
	virtual void StencilPassOp( ShaderStencilOp_t stencilOp ) = 0;
	virtual void StencilFailOp( ShaderStencilOp_t stencilOp ) = 0;
	virtual void StencilDepthFailOp( ShaderStencilOp_t stencilOp ) = 0;
	virtual void StencilReference( int nReference ) = 0;
	virtual void StencilMask( int nMask ) = 0;
	virtual void StencilWriteMask( int nMask ) = 0;

	// Suppresses/activates color writing 
	virtual void EnableColorWrites( bool bEnable ) = 0;
	virtual void EnableAlphaWrites( bool bEnable ) = 0;

	// Methods related to alpha blending
	virtual void EnableBlending( bool bEnable ) = 0;
	virtual void BlendFunc( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor ) = 0;
	// More below...

	// Alpha testing
	virtual void EnableAlphaTest( bool bEnable ) = 0;
	virtual void AlphaFunc( ShaderAlphaFunc_t alphaFunc, float alphaRef /* [0-1] */ ) = 0;

	// Wireframe/filled polygons
	virtual void PolyMode( ShaderPolyModeFace_t face, ShaderPolyMode_t polyMode ) = 0;

	// Back face culling
	virtual void EnableCulling( bool bEnable ) = 0;

	// constant color + transparency
	virtual void EnableConstantColor( bool bEnable ) = 0;

	// Indicates the vertex format for use with a vertex shader
	// The flags to pass in here come from the VertexFormatFlags_t enum
	// If pTexCoordDimensions is *not* specified, we assume all coordinates
	// are 2-dimensional
	virtual void VertexShaderVertexFormat( unsigned int nFlags, 
			int nTexCoordCount, int* pTexCoordDimensions, int nUserDataSize ) = 0;

	// Pixel and vertex shader methods
	virtual void SetVertexShader( const char* pFileName, int nStaticVshIndex ) = 0;
	virtual	void SetPixelShader( const char* pFileName, int nStaticPshIndex = 0 ) = 0;

	// Indicates we're going to light the model
	virtual void EnableLighting( bool bEnable ) = 0;

	// Enables specular lighting (lighting has also got to be enabled)
	virtual void EnableSpecular( bool bEnable ) = 0;

	// Convert from linear to gamma color space on writes to frame buffer.
	virtual void EnableSRGBWrite( bool bEnable ) = 0;

	// Convert from gamma to linear on texture fetch.
	virtual void EnableSRGBRead( Sampler_t sampler, bool bEnable ) = 0;

	// Activate/deactivate skinning. Indexed blending is automatically
	// enabled if it's available for this hardware. When blending is enabled,
	// we allocate enough room for 3 weights (max allowed)
	virtual void EnableVertexBlend( bool bEnable ) = 0;

	// per texture unit stuff
	virtual void OverbrightValue( TextureStage_t stage, float value ) = 0;
	virtual void EnableTexture( Sampler_t sampler, bool bEnable ) = 0;
	virtual void EnableTexGen( TextureStage_t stage, bool bEnable ) = 0;
	virtual void TexGen( TextureStage_t stage, ShaderTexGenParam_t param ) = 0;

	// alternate method of specifying per-texture unit stuff, more flexible and more complicated
	// Can be used to specify different operation per channel (alpha/color)...
	virtual void EnableCustomPixelPipe( bool bEnable ) = 0;
	virtual void CustomTextureStages( int stageCount ) = 0;
	virtual void CustomTextureOperation( TextureStage_t stage, ShaderTexChannel_t channel, 
		ShaderTexOp_t op, ShaderTexArg_t arg1, ShaderTexArg_t arg2 ) = 0;

	// indicates what per-vertex data we're providing
	virtual void DrawFlags( unsigned int drawFlags ) = 0;

	// A simpler method of dealing with alpha modulation
	virtual void EnableAlphaPipe( bool bEnable ) = 0;
	virtual void EnableConstantAlpha( bool bEnable ) = 0;
	virtual void EnableVertexAlpha( bool bEnable ) = 0;
	virtual void EnableTextureAlpha( TextureStage_t stage, bool bEnable ) = 0;

	// GR - Separate alpha blending
	virtual void EnableBlendingSeparateAlpha( bool bEnable ) = 0;
	virtual void BlendFuncSeparateAlpha( ShaderBlendFactor_t srcFactor, ShaderBlendFactor_t dstFactor ) = 0;
	virtual void FogMode( ShaderFogMode_t fogMode ) = 0;

	virtual void SetDiffuseMaterialSource( ShaderMaterialSource_t materialSource ) = 0;

	// Indicates the morph format for use with a vertex shader
	// The flags to pass in here come from the MorphFormatFlags_t enum
	virtual void SetMorphFormat( MorphFormat_t flags ) = 0;

	virtual void DisableFogGammaCorrection( bool bDisable ) = 0; //some blending modes won't work properly with corrected fog

	// Alpha to coverage
	virtual void EnableAlphaToCoverage( bool bEnable ) = 0;

	// Shadow map filtering
	virtual void SetShadowDepthFiltering( Sampler_t stage ) = 0;

	// More alpha blending state
	virtual void BlendOp( ShaderBlendOp_t blendOp ) = 0;
	virtual void BlendOpSeparateAlpha( ShaderBlendOp_t blendOp ) = 0;
};
// end class IShaderShadow



#endif // ISHADERSHADOW_H
