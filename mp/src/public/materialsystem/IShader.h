//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef ISHADER_H
#define ISHADER_H

#ifdef _WIN32
#pragma once
#endif

//==================================================================================================
// **this goes into both platforms which run the translator, either the real Mac client or
// the Windows client running with r_emulategl mode **
//
// size of the VS register bank in ARB / GLSL we expose
// it's not 256, because you can't use all 256 slots in 10.5.x.
// use this constant everywhere you might normally use "256" in reference to a parameter array size.
// The highest shader constant is c218, plus we allocate c219 and c220 for two clip planes
#define	DXABSTRACT_VS_PARAM_SLOTS	219
#define DXABSTRACT_VS_FIRST_BONE_SLOT VERTEX_SHADER_MODEL

// user clip plane 0 goes in DXABSTRACT_VS_CLIP_PLANE_BASE... plane 1 goes in the slot after that
// dxabstract uses these constants to check plane index limit and to deliver planes to shader for DP4 -> oCLP[n]
#define	DXABSTRACT_VS_CLIP_PLANE_BASE (DXABSTRACT_VS_PARAM_SLOTS-2)

//==================================================================================================


#include "materialsystem/imaterialsystem.h"
#include "materialsystem/ishaderapi.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class IMaterialVar;
class IShaderShadow;
class IShaderDynamicAPI;
class IShaderInit;
class CBasePerMaterialContextData;

//-----------------------------------------------------------------------------
// Shader flags
//-----------------------------------------------------------------------------
enum ShaderFlags_t
{
	SHADER_NOT_EDITABLE = 0x1
};


//-----------------------------------------------------------------------------
// Shader parameter flags
//-----------------------------------------------------------------------------
enum ShaderParamFlags_t
{
	SHADER_PARAM_NOT_EDITABLE = 0x1
};


//-----------------------------------------------------------------------------
// Information about each shader parameter
//-----------------------------------------------------------------------------
struct ShaderParamInfo_t
{
	const char *m_pName;
	const char *m_pHelp;
	ShaderParamType_t m_Type;
	const char *m_pDefaultValue;
	int m_nFlags;
};


//-----------------------------------------------------------------------------
// Standard vertex shader constants
//-----------------------------------------------------------------------------
enum
{
	// Standard vertex shader constants
	VERTEX_SHADER_MATH_CONSTANTS0 = 0,
	VERTEX_SHADER_MATH_CONSTANTS1 = 1,
	VERTEX_SHADER_CAMERA_POS = 2,
	VERTEX_SHADER_FLEXSCALE = 3,		// used by DX9 only!
	VERTEX_SHADER_LIGHT_INDEX = 3,		// used by DX8 only!
	VERTEX_SHADER_MODELVIEWPROJ = 4,
	VERTEX_SHADER_VIEWPROJ = 8,
	VERTEX_SHADER_MODELVIEWPROJ_THIRD_ROW = 12,
	VERTEX_SHADER_VIEWPROJ_THIRD_ROW = 13,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_10 = 14,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_11 = 15,
	VERTEX_SHADER_FOG_PARAMS = 16,
	VERTEX_SHADER_VIEWMODEL = 17,
	VERTEX_SHADER_AMBIENT_LIGHT = 21,
	VERTEX_SHADER_LIGHTS = 27,
	VERTEX_SHADER_LIGHT0_POSITION = 29,
	VERTEX_SHADER_MODULATION_COLOR = 47,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_0 = 48,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_1 = 49,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_2 = 50,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_3 = 51,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_4 = 52,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_5 = 53,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_6 = 54,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_7 = 55,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_8 = 56,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_9 = 57,
	VERTEX_SHADER_MODEL = 58,

	//
	// We reserve up through 216 for the 53 bones
	//

	// 219		ClipPlane0				|------ OpenGL will jam clip planes into these two
	// 220		ClipPlane1				|	

	VERTEX_SHADER_FLEX_WEIGHTS = 1024,
	VERTEX_SHADER_MAX_FLEX_WEIGHT_COUNT = 512,
};

#define VERTEX_SHADER_BONE_TRANSFORM( k )	( VERTEX_SHADER_MODEL + 3 * (k) )

//-----------------------------------------------------------------------------
// Standard vertex shader constants
//-----------------------------------------------------------------------------
enum
{
	// Standard vertex shader constants
	VERTEX_SHADER_LIGHT_ENABLE_BOOL_CONST = 0,
	VERTEX_SHADER_LIGHT_ENABLE_BOOL_CONST_COUNT = 4,

	VERTEX_SHADER_SHADER_SPECIFIC_BOOL_CONST_0 = 4,
	VERTEX_SHADER_SHADER_SPECIFIC_BOOL_CONST_1 = 5,
	VERTEX_SHADER_SHADER_SPECIFIC_BOOL_CONST_2 = 6,
	VERTEX_SHADER_SHADER_SPECIFIC_BOOL_CONST_3 = 7,
	VERTEX_SHADER_SHADER_SPECIFIC_BOOL_CONST_4 = 8,
	VERTEX_SHADER_SHADER_SPECIFIC_BOOL_CONST_5 = 9,
	VERTEX_SHADER_SHADER_SPECIFIC_BOOL_CONST_6 = 10,
	VERTEX_SHADER_SHADER_SPECIFIC_BOOL_CONST_7 = 11,
};
// The public methods exposed by each shader
//-----------------------------------------------------------------------------
abstract_class IShader
{
public:
	// Returns the shader name
	virtual char const* GetName( ) const = 0;

	// returns the shader fallbacks
	virtual char const* GetFallbackShader( IMaterialVar** params ) const = 0;

	// Shader parameters
	virtual int GetNumParams( ) const = 0;

	// These functions must be implemented by the shader
	virtual void InitShaderParams( IMaterialVar** ppParams, const char *pMaterialName ) = 0;
	virtual void InitShaderInstance( IMaterialVar** ppParams, IShaderInit *pShaderInit, const char *pMaterialName, const char *pTextureGroupName ) = 0;
	virtual void DrawElements( IMaterialVar **params, int nModulationFlags,
		IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr ) = 0;

	virtual char const* GetParamName( int paramIndex ) const = 0;
	virtual char const* GetParamHelp( int paramIndex ) const = 0;
	virtual ShaderParamType_t GetParamType( int paramIndex ) const = 0;
	virtual char const* GetParamDefault( int paramIndex ) const = 0;

	// FIXME: Figure out a better way to do this?
	virtual int ComputeModulationFlags( IMaterialVar** params, IShaderDynamicAPI* pShaderAPI ) = 0;
	virtual bool NeedsPowerOfTwoFrameBufferTexture( IMaterialVar **params, bool bCheckSpecificToThisFrame = true ) const = 0;
	virtual bool NeedsFullFrameBufferTexture( IMaterialVar **params, bool bCheckSpecificToThisFrame ) const = 0;
	virtual bool IsTranslucent( IMaterialVar **params ) const = 0;

	virtual int GetParamFlags( int paramIndex ) const = 0;

	virtual int GetFlags() const = 0;

	// FIXME: Remove GetParamName, etc. above
//	virtual const ShaderParamInfo_t& GetParamInfo( int paramIndex ) const = 0;
};


//-----------------------------------------------------------------------------
// Shader dictionaries defined in DLLs
//-----------------------------------------------------------------------------
enum PrecompiledShaderType_t
{
	PRECOMPILED_VERTEX_SHADER = 0,
	PRECOMPILED_PIXEL_SHADER,

	PRECOMPILED_SHADER_TYPE_COUNT,
};


//-----------------------------------------------------------------------------
// Flags field of PrecompiledShader_t
//-----------------------------------------------------------------------------
enum
{
	// runtime flags
	SHADER_DYNAMIC_COMPILE_IS_HLSL = 0x1,
	SHADER_FAILED_LOAD = 0x2,
};

#endif // ISHADER_H
