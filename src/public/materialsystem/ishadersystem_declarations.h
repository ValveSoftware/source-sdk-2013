//===== Copyright © Valve Corporation, All rights reserved. ======//
#ifndef ISHADER_DECLARATIONS_HDR
#define ISHADER_DECLARATIONS_HDR

//-----------------------------------------------------------------------------
// Standard vertex shader constants
//-----------------------------------------------------------------------------
enum
{
	// Standard vertex shader constants
	VERTEX_SHADER_MATH_CONSTANTS0 = 0,
	VERTEX_SHADER_MATH_CONSTANTS1 = 1,
	VERTEX_SHADER_CAMERA_POS = 2,
	VERTEX_SHADER_FLEXSCALE = 3,   // DX9 only
	VERTEX_SHADER_LIGHT_INDEX = 3, // DX8 only
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
	// We reserve up through 216 for the 53 bones supported on DX9
	//
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_13 = 217,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_14 = 218,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_15 = 219,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_16 = 220,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_17 = 221,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_18 = 222,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_19 = 223,
	VERTEX_SHADER_SHADER_SPECIFIC_CONST_12 = 224,

	// 226		ClipPlane0				|------ OpenGL will jam clip planes into these two
	// 227		ClipPlane1				|


	VERTEX_SHADER_FLEX_WEIGHTS = 1024,
	VERTEX_SHADER_MAX_FLEX_WEIGHT_COUNT = 512,
};

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
	SHADER_IS_ASM = 0x1,
	SHADER_FAILED_LOAD = 0x2,
};


#endif
