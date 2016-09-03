//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: This is where all common code for vertex shaders go.
//
// $NoKeywords: $
//
//===========================================================================//



#ifndef COMMON_VS_FXC_H_
#define COMMON_VS_FXC_H_

#include "common_fxc.h"

// Put global skip commands here. . make sure and check that the appropriate vars are defined
// so these aren't used on the wrong shaders!
// --------------------------------------------------------------------------------
// Ditch all fastpath attemps if we are doing LIGHTING_PREVIEW.
//	SKIP: defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// --------------------------------------------------------------------------------


#ifndef COMPRESSED_VERTS
// Default to no vertex compression
#define COMPRESSED_VERTS 0
#endif

#if ( !defined( SHADER_MODEL_VS_2_0 ) && !defined( SHADER_MODEL_VS_3_0 ) )
#if COMPRESSED_VERTS == 1
#error "Vertex compression is only for DX9 and up!"
#endif
#endif

// We're testing 2 normal compression methods
// One compressed normals+tangents into a SHORT2 each (8 bytes total)
// The other compresses them together, into a single UBYTE4 (4 bytes total)
// FIXME: pick one or the other, compare lighting quality in important cases
#define COMPRESSED_NORMALS_SEPARATETANGENTS_SHORT2	0
#define COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4	1
//#define COMPRESSED_NORMALS_TYPE						COMPRESSED_NORMALS_SEPARATETANGENTS_SHORT2
#define COMPRESSED_NORMALS_TYPE					COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4


#define FOGTYPE_RANGE				0
#define FOGTYPE_HEIGHT				1

#define COMPILE_ERROR ( 1/0; )

// -------------------------
// CONSTANTS
// -------------------------

#pragma def ( vs, c0, 0.0f, 1.0f, 2.0f, 0.5f )

const float4 cConstants1				: register(c1);
#define cOOGamma			cConstants1.x
#define cOverbright			2.0f
#define cOneThird			cConstants1.z
#define cOOOverbright		( 1.0f / 2.0f )


// The g_bLightEnabled registers and g_nLightCountRegister hold the same information regarding
// enabling lights, but callers internal to this file tend to use the loops, while external
// callers will end up using the booleans
const bool g_bLightEnabled[4]			: register(b0);
										// through b3

const int g_nLightCountRegister			: register(i0);


#define g_nLightCount					g_nLightCountRegister.x

const float4 cEyePosWaterZ				: register(c2);
#define cEyePos			cEyePosWaterZ.xyz

// This is still used by asm stuff.
const float4 cObsoleteLightIndex		: register(c3);

const float4x4 cModelViewProj			: register(c4);
const float4x4 cViewProj				: register(c8);

// Only cFlexScale.x is used
// It is a binary value used to switch on/off the addition of the flex delta stream
const float4 cFlexScale					: register(c13);

const float4 cFogParams					: register(c16);
#define cFogEndOverFogRange cFogParams.x
#define cFogOne cFogParams.y
#define cFogMaxDensity cFogParams.z
#define cOOFogRange cFogParams.w

const float4x4 cViewModel				: register(c17);

const float3 cAmbientCubeX [ 2 ] : register ( c21 ) ;
const float3 cAmbientCubeY [ 2 ] : register ( c23 ) ;
const float3 cAmbientCubeZ [ 2 ] : register ( c25 ) ;

#ifdef SHADER_MODEL_VS_3_0
const float4 cFlexWeights [ 512 ] : register ( c1024 ) ;
#endif

struct LightInfo
{
	float4 color;						// {xyz} is color	w is light type code (see comment below)
	float4 dir;							// {xyz} is dir		w is light type code
	float4 pos;
	float4 spotParams;
	float4 atten;
};

// w components of color and dir indicate light type:
// 1x - directional
// 01 - spot
// 00 - point

// Four lights x 5 constants each = 20 constants
LightInfo cLightInfo[4]					: register(c27);
#define LIGHT_0_POSITION_REG					   c29

#ifdef SHADER_MODEL_VS_1_1

const float4 cModulationColor			: register(c37);

#define SHADER_SPECIFIC_CONST_0 c38
#define SHADER_SPECIFIC_CONST_1 c39
#define SHADER_SPECIFIC_CONST_2 c40
#define SHADER_SPECIFIC_CONST_3 c41
#define SHADER_SPECIFIC_CONST_4 c42
#define SHADER_SPECIFIC_CONST_5 c43
#define SHADER_SPECIFIC_CONST_6 c44
#define SHADER_SPECIFIC_CONST_7 c45
#define SHADER_SPECIFIC_CONST_8 c46
#define SHADER_SPECIFIC_CONST_9 c47
#define SHADER_SPECIFIC_CONST_10 c14
#define SHADER_SPECIFIC_CONST_11 c15

static const int cModel0Index = 48;
const float4x3 cModel[16]					: register(c48);
// last cmodel is c105 for dx80, c214 for dx90

#else // DX9 shaders (vs20 and beyond)

const float4 cModulationColor			: register( c47 );

#define SHADER_SPECIFIC_CONST_0 c48
#define SHADER_SPECIFIC_CONST_1 c49
#define SHADER_SPECIFIC_CONST_2 c50
#define SHADER_SPECIFIC_CONST_3 c51
#define SHADER_SPECIFIC_CONST_4 c52
#define SHADER_SPECIFIC_CONST_5 c53
#define SHADER_SPECIFIC_CONST_6 c54
#define SHADER_SPECIFIC_CONST_7 c55
#define SHADER_SPECIFIC_CONST_8 c56
#define SHADER_SPECIFIC_CONST_9 c57
#define SHADER_SPECIFIC_CONST_10 c14
#define SHADER_SPECIFIC_CONST_11 c15

static const int cModel0Index = 58;
const float4x3 cModel[53]					: register( c58 );
// last cmodel is c105 for dx80, c214 for dx90

#define SHADER_SPECIFIC_BOOL_CONST_0 b4
#define SHADER_SPECIFIC_BOOL_CONST_1 b5
#define SHADER_SPECIFIC_BOOL_CONST_2 b6
#define SHADER_SPECIFIC_BOOL_CONST_3 b7
#define SHADER_SPECIFIC_BOOL_CONST_4 b8
#define SHADER_SPECIFIC_BOOL_CONST_5 b9
#define SHADER_SPECIFIC_BOOL_CONST_6 b10
#define SHADER_SPECIFIC_BOOL_CONST_7 b11
#endif // vertex shader model constant packing changes


//=======================================================================================
// Methods to decompress vertex normals
//=======================================================================================

//-----------------------------------------------------------------------------------
// Decompress a normal from two-component compressed format
// We expect this data to come from a signed SHORT2 stream in the range of -32768..32767
//
// -32678 and 0 are invalid encodings
// w contains the sign to use in the cross product when generating a binormal
void _DecompressShort2Tangent( float2 inputTangent, out float4 outputTangent )
{
	float2 ztSigns		= sign( inputTangent );				// sign bits for z and tangent (+1 or -1)
	float2 xyAbs		= abs(  inputTangent );				// 1..32767
	outputTangent.xy	= (xyAbs - 16384.0f) / 16384.0f;	// x and y
	outputTangent.z		= ztSigns.x * sqrt( saturate( 1.0f - dot( outputTangent.xy, outputTangent.xy ) ) );
	outputTangent.w		= ztSigns.y;
}

//-----------------------------------------------------------------------------------
// Same code as _DecompressShort2Tangent, just one returns a float4, one a float3
void _DecompressShort2Normal( float2 inputNormal, out float3 outputNormal )
{
	float4 result;
	_DecompressShort2Tangent( inputNormal, result );
	outputNormal = result.xyz;
}

//-----------------------------------------------------------------------------------
// Decompress normal+tangent together
void _DecompressShort2NormalTangent( float2 inputNormal, float2 inputTangent, out float3 outputNormal, out float4 outputTangent )
{
	// FIXME: if we end up sticking with the SHORT2 format, pack the normal and tangent into a single SHORT4 element
	//        (that would make unpacking normal+tangent here together much cheaper than the sum of their parts)
	_DecompressShort2Normal(  inputNormal,  outputNormal  );
	_DecompressShort2Tangent( inputTangent, outputTangent );
}

//=======================================================================================
// Decompress a normal and tangent from four-component compressed format
// We expect this data to come from an unsigned UBYTE4 stream in the range of 0..255
// The final vTangent.w contains the sign to use in the cross product when generating a binormal
void _DecompressUByte4NormalTangent( float4 inputNormal,
									out float3 outputNormal,   // {nX, nY, nZ}
									out float4 outputTangent )   // {tX, tY, tZ, sign of binormal}
{
	float fOne   = 1.0f;

	float4 ztztSignBits	= ( inputNormal - 128.0f ) < 0;						// sign bits for zs and binormal (1 or 0)  set-less-than (slt) asm instruction
	float4 xyxyAbs		= abs( inputNormal - 128.0f ) - ztztSignBits;		// 0..127
	float4 xyxySignBits	= ( xyxyAbs - 64.0f ) < 0;							// sign bits for xs and ys (1 or 0)
	float4 normTan		= (abs( xyxyAbs - 64.0f ) - xyxySignBits) / 63.0f;	// abs({nX, nY, tX, tY})
	outputNormal.xy		= normTan.xy;										// abs({nX, nY, __, __})
	outputTangent.xy	= normTan.zw;										// abs({tX, tY, __, __})

	float4 xyxySigns	= 1 - 2*xyxySignBits;								// Convert sign bits to signs
	float4 ztztSigns	= 1 - 2*ztztSignBits;								// ( [1,0] -> [-1,+1] )

	outputNormal.z		= 1.0f - outputNormal.x - outputNormal.y;			// Project onto x+y+z=1
	outputNormal.xyz	= normalize( outputNormal.xyz );					// Normalize onto unit sphere
	outputNormal.xy	   *= xyxySigns.xy;										// Restore x and y signs
	outputNormal.z	   *= ztztSigns.x;										// Restore z sign

	outputTangent.z		= 1.0f - outputTangent.x - outputTangent.y;			// Project onto x+y+z=1
	outputTangent.xyz	= normalize( outputTangent.xyz );					// Normalize onto unit sphere
	outputTangent.xy   *= xyxySigns.zw;										// Restore x and y signs
	outputTangent.z	   *= ztztSigns.z;										// Restore z sign
	outputTangent.w		= ztztSigns.w;										// Binormal sign
}


//-----------------------------------------------------------------------------------
// Decompress just a normal from four-component compressed format (same as above)
// We expect this data to come from an unsigned UBYTE4 stream in the range of 0..255
// [ When compiled, this works out to approximately 17 asm instructions ]
void _DecompressUByte4Normal( float4 inputNormal,
							out float3 outputNormal)					// {nX, nY, nZ}
{
	float fOne			= 1.0f;

	float2 ztSigns		= ( inputNormal.xy - 128.0f ) < 0;				// sign bits for zs and binormal (1 or 0)  set-less-than (slt) asm instruction
	float2 xyAbs		= abs( inputNormal.xy - 128.0f ) - ztSigns;		// 0..127
	float2 xySigns		= ( xyAbs -  64.0f ) < 0;						// sign bits for xs and ys (1 or 0)
	outputNormal.xy		= ( abs( xyAbs - 64.0f ) - xySigns ) / 63.0f;	// abs({nX, nY})

	outputNormal.z		= 1.0f - outputNormal.x - outputNormal.y;		// Project onto x+y+z=1
	outputNormal.xyz	= normalize( outputNormal.xyz );				// Normalize onto unit sphere

	outputNormal.xy	   *= lerp( fOne.xx, -fOne.xx, xySigns   );			// Restore x and y signs
	outputNormal.z	   *= lerp( fOne.x,  -fOne.x,  ztSigns.x );			// Restore z sign
}


void DecompressVertex_Normal( float4 inputNormal, out float3 outputNormal )
{
	if ( COMPRESSED_VERTS == 1 )
	{
		if ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_SEPARATETANGENTS_SHORT2 )
		{
			_DecompressShort2Normal( inputNormal.xy, outputNormal );
		}
		else // ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4 )
		{
			_DecompressUByte4Normal( inputNormal, outputNormal );
		}
	}
	else
	{
		outputNormal = inputNormal.xyz;
	}
}

void DecompressVertex_NormalTangent( float4 inputNormal,  float4 inputTangent, out float3 outputNormal, out float4 outputTangent )
{
	if ( COMPRESSED_VERTS == 1 )
	{
		if ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_SEPARATETANGENTS_SHORT2 )
		{
			_DecompressShort2NormalTangent( inputNormal.xy, inputTangent.xy, outputNormal, outputTangent );
		}
		else // ( COMPRESSED_NORMALS_TYPE == COMPRESSED_NORMALS_COMBINEDTANGENTS_UBYTE4 )
		{
			_DecompressUByte4NormalTangent( inputNormal, outputNormal, outputTangent );
		}
	}
	else
	{
		outputNormal  = inputNormal.xyz;
		outputTangent = inputTangent;
	}
}


#ifdef SHADER_MODEL_VS_3_0

//-----------------------------------------------------------------------------
// Methods to sample morph data from a vertex texture
// NOTE: vMorphTargetTextureDim.x = width, cVertexTextureDim.y = height, cVertexTextureDim.z = # of float4 fields per vertex
// For position + normal morph for example, there will be 2 fields.
//-----------------------------------------------------------------------------
float4 SampleMorphDelta( sampler2D vt, const float3 vMorphTargetTextureDim, const float4 vMorphSubrect, const float flVertexID, const float flField )
{
	float flColumn = floor( flVertexID / vMorphSubrect.w );

	float4 t;
	t.x = vMorphSubrect.x + vMorphTargetTextureDim.z * flColumn + flField + 0.5f;
	t.y = vMorphSubrect.y + flVertexID - flColumn * vMorphSubrect.w + 0.5f;
	t.xy /= vMorphTargetTextureDim.xy;	
	t.z = t.w = 0.f;

	return tex2Dlod( vt, t );
}

// Optimized version which reads 2 deltas
void SampleMorphDelta2( sampler2D vt, const float3 vMorphTargetTextureDim, const float4 vMorphSubrect, const float flVertexID, out float4 delta1, out float4 delta2 )
{
	float flColumn = floor( flVertexID / vMorphSubrect.w );

	float4 t;
	t.x = vMorphSubrect.x + vMorphTargetTextureDim.z * flColumn + 0.5f;
	t.y = vMorphSubrect.y + flVertexID - flColumn * vMorphSubrect.w + 0.5f;
	t.xy /= vMorphTargetTextureDim.xy;	
	t.z = t.w = 0.f;

	delta1 = tex2Dlod( vt, t );
	t.x += 1.0f / vMorphTargetTextureDim.x;
	delta2 = tex2Dlod( vt, t );
}

#endif // SHADER_MODEL_VS_3_0

//-----------------------------------------------------------------------------
// Method to apply morphs
//-----------------------------------------------------------------------------
bool ApplyMorph( float3 vPosFlex, in float3 vPosition, out float3 vPosition_O )
{
	// Flexes coming in from a separate stream
	float3 vPosDelta = vPosFlex.xyz * cFlexScale.x;
	vPosition_O.xyz = vPosition + vPosDelta;
	return true;
}

bool ApplyMorph( float3 vPosFlex, float3 vNormalFlex,
	in float3 vPosition, out float3 vPosition_O,
	in float3 vNormal, out float3 vNormal_O )
{
	// Flexes coming in from a separate stream
	float3 vPosDelta = vPosFlex.xyz * cFlexScale.x;
	float3 vNormalDelta = vNormalFlex.xyz * cFlexScale.x;
	vPosition_O.xyz	= vPosition + vPosDelta;
	vNormal_O.xyz	= vNormal + vNormalDelta;
	return true;
}

bool ApplyMorph( float3 vPosFlex, float3 vNormalFlex,
	in float3 vPosition, out float3 vPosition_O,
	in float3 vNormal, out float3 vNormal_O,
	in float3 vTangent, out float3 vTangent_O )
{
	// Flexes coming in from a separate stream
	float3 vPosDelta = vPosFlex.xyz * cFlexScale.x;
	float3 vNormalDelta = vNormalFlex.xyz * cFlexScale.x;
	vPosition_O.xyz	= vPosition + vPosDelta;
	vNormal_O.xyz	= vNormal + vNormalDelta;
	vTangent_O.xyz	= vTangent + vNormalDelta;
	return true;
}

bool ApplyMorph( float4 vPosFlex, float3 vNormalFlex, 
	inout float3 vPosition, inout float3 vNormal, inout float3 vTangent, out float flWrinkle )
{
	// Flexes coming in from a separate stream
	float3 vPosDelta = vPosFlex.xyz * cFlexScale.x;
	float3 vNormalDelta = vNormalFlex.xyz * cFlexScale.x;
	flWrinkle = vPosFlex.w * cFlexScale.y;
	vPosition.xyz += vPosDelta;
	vNormal       += vNormalDelta;
	vTangent.xyz  += vNormalDelta;
	return true;
}

#ifdef SHADER_MODEL_VS_3_0

bool ApplyMorph( sampler2D morphSampler, const float3 vMorphTargetTextureDim, const float4 vMorphSubrect, 
				const float flVertexID, const float3 vMorphTexCoord,
				in float3 vPosition, out float3 vPosition_O )
{
#if MORPHING

#if 1
	// Flexes coming in from a separate stream
	float4 vPosDelta = SampleMorphDelta( morphSampler, vMorphTargetTextureDim, vMorphSubrect, flVertexID, 0 );
	vPosition_O		= vPosition + vPosDelta.xyz;
#else
	float4 t = float4( vMorphTexCoord.x, vMorphTexCoord.y, 0.0f, 0.0f );
	float3 vPosDelta = tex2Dlod( morphSampler, t );
	vPosition	+= vPosDelta.xyz * vMorphTexCoord.z;
#endif // DECAL

	return true;

#else // !MORPHING
	vPosition_O = vPosition;
	return false;
#endif
}
 
bool ApplyMorph( sampler2D morphSampler, const float3 vMorphTargetTextureDim, const float4 vMorphSubrect, 
				const float flVertexID, const float3 vMorphTexCoord, 
				in float3 vPosition, out float3 vPosition_O,
				in float3 vNormal, out float3 vNormal_O )
{
#if MORPHING

#if 1
	float4 vPosDelta, vNormalDelta;
	SampleMorphDelta2( morphSampler, vMorphTargetTextureDim, vMorphSubrect, flVertexID, vPosDelta, vNormalDelta );
	vPosition_O		= vPosition + vPosDelta.xyz;
	vNormal_O		= vNormal + vNormalDelta.xyz;
#else
	float4 t = float4( vMorphTexCoord.x, vMorphTexCoord.y, 0.0f, 0.0f );
	float3 vPosDelta = tex2Dlod( morphSampler, t );
	t.x += 1.0f / vMorphTargetTextureDim.x;
	float3 vNormalDelta = tex2Dlod( morphSampler, t );
	vPosition	+= vPosDelta.xyz * vMorphTexCoord.z;
	vNormal		+= vNormalDelta.xyz * vMorphTexCoord.z;
#endif // DECAL

	return true;

#else // !MORPHING
	vPosition_O = vPosition;
	vNormal_O = vNormal;
	return false;
#endif
}

bool ApplyMorph( sampler2D morphSampler, const float3 vMorphTargetTextureDim, const float4 vMorphSubrect, 
				const float flVertexID, const float3 vMorphTexCoord, 
				in float3 vPosition, out float3 vPosition_O,
				in float3 vNormal, out float3 vNormal_O,
				in float3 vTangent, out float3 vTangent_O )
{
#if MORPHING

#if 1
	float4 vPosDelta, vNormalDelta;
	SampleMorphDelta2( morphSampler, vMorphTargetTextureDim, vMorphSubrect, flVertexID, vPosDelta, vNormalDelta );
	vPosition_O		= vPosition + vPosDelta.xyz;
	vNormal_O		= vNormal + vNormalDelta.xyz;
	vTangent_O		= vTangent + vNormalDelta.xyz;
#else
	float4 t = float4( vMorphTexCoord.x, vMorphTexCoord.y, 0.0f, 0.0f );
	float3 vPosDelta = tex2Dlod( morphSampler, t );
	t.x += 1.0f / vMorphTargetTextureDim.x;
	float3 vNormalDelta = tex2Dlod( morphSampler, t );
	vPosition	+= vPosDelta.xyz * vMorphTexCoord.z;
	vNormal		+= vNormalDelta.xyz * vMorphTexCoord.z;
	vTangent	+= vNormalDelta.xyz * vMorphTexCoord.z;
#endif // DECAL

	return true;

#else // MORPHING
	vPosition_O = vPosition;
	vNormal_O = vNormal;
	vTangent_O = vTangent;
	return false;
#endif
}

bool ApplyMorph( sampler2D morphSampler, const float3 vMorphTargetTextureDim, const float4 vMorphSubrect,
	const float flVertexID, const float3 vMorphTexCoord,
	inout float3 vPosition, inout float3 vNormal, inout float3 vTangent, out float flWrinkle )
{
#if MORPHING

#if !DECAL
	float4 vPosDelta, vNormalDelta;
	SampleMorphDelta2( morphSampler, vMorphTargetTextureDim, vMorphSubrect, flVertexID, vPosDelta, vNormalDelta );
	vPosition	+= vPosDelta.xyz;
	vNormal		+= vNormalDelta.xyz;
	vTangent	+= vNormalDelta.xyz;
	flWrinkle = vPosDelta.w;
#else
	float4 t = float4( vMorphTexCoord.x, vMorphTexCoord.y, 0.0f, 0.0f );
	float4 vPosDelta = tex2Dlod( morphSampler, t );
	t.x += 1.0f / vMorphTargetTextureDim.x;
	float3 vNormalDelta = tex2Dlod( morphSampler, t );

	vPosition	+= vPosDelta.xyz * vMorphTexCoord.z;
	vNormal		+= vNormalDelta.xyz * vMorphTexCoord.z;
	vTangent	+= vNormalDelta.xyz * vMorphTexCoord.z;
	flWrinkle	= vPosDelta.w * vMorphTexCoord.z;
#endif // DECAL

	return true;

#else // MORPHING

	flWrinkle = 0.0f;
	return false;

#endif
}

#endif   // SHADER_MODEL_VS_3_0


float RangeFog( const float3 projPos )
{
	return max( cFogMaxDensity, ( -projPos.z * cOOFogRange + cFogEndOverFogRange ) );
}

float WaterFog( const float3 worldPos, const float3 projPos )
{
	float4 tmp;
	
	tmp.xy = cEyePosWaterZ.wz - worldPos.z;

	// tmp.x is the distance from the water surface to the vert
	// tmp.y is the distance from the eye position to the vert

	// if $tmp.x < 0, then set it to 0
	// This is the equivalent of moving the vert to the water surface if it's above the water surface
	
	tmp.x = max( 0.0f, tmp.x );

	// $tmp.w = $tmp.x / $tmp.y
	tmp.w = tmp.x / tmp.y;

	tmp.w *= projPos.z;

	// $tmp.w is now the distance that we see through water.

	return max( cFogMaxDensity, ( -tmp.w * cOOFogRange + cFogOne ) );
}

float CalcFog( const float3 worldPos, const float3 projPos, const int fogType )
{
#if defined( _X360 )
	// 360 only does pixel fog
	return 1.0f;
#endif

	if( fogType == FOGTYPE_RANGE )
	{
		return RangeFog( projPos );
	}
	else
	{
#if SHADERMODEL_VS_2_0 == 1
		// We do this work in the pixel shader in dx9, so don't do any fog here.
		return 1.0f;
#else
		return WaterFog( worldPos, projPos );
#endif
	}
}

float CalcFog( const float3 worldPos, const float3 projPos, const bool bWaterFog )
{
#if defined( _X360 )
	// 360 only does pixel fog
	return 1.0f;
#endif

	float flFog;
	if( !bWaterFog )
	{
		flFog = RangeFog( projPos );
	}
	else
	{
#if SHADERMODEL_VS_2_0 == 1
		// We do this work in the pixel shader in dx9, so don't do any fog here.
		flFog = 1.0f;
#else
		flFog = WaterFog( worldPos, projPos );
#endif
	}

	return flFog;
}

float4 DecompressBoneWeights( const float4 weights )
{
	float4 result = weights;

	if ( COMPRESSED_VERTS )
	{
		// Decompress from SHORT2 to float. In our case, [-1, +32767] -> [0, +1]
		// NOTE: we add 1 here so we can divide by 32768 - which is exact (divide by 32767 is not).
		//       This avoids cracking between meshes with different numbers of bone weights.
		//       We use SHORT2 instead of SHORT2N for a similar reason - the GPU's conversion
		//       from [-32768,+32767] to [-1,+1] is imprecise in the same way.
		result += 1;
		result /= 32768;
	}

	return result;
}

void SkinPosition( bool bSkinning, const float4 modelPos, 
                   const float4 boneWeights, float4 fBoneIndices,
				   out float3 worldPos )
{
#if !defined( _X360 )
	int3 boneIndices = D3DCOLORtoUBYTE4( fBoneIndices );
#else
	int3 boneIndices = fBoneIndices;
#endif

	// Needed for invariance issues caused by multipass rendering
#if defined( _X360 )
	[isolate] 
#endif
	{ 
		if ( !bSkinning )
		{
			worldPos = mul4x3( modelPos, cModel[0] );
		}
		else // skinning - always three bones
		{
			float4x3 mat1 = cModel[boneIndices[0]];
			float4x3 mat2 = cModel[boneIndices[1]];
			float4x3 mat3 = cModel[boneIndices[2]];

			float3 weights = DecompressBoneWeights( boneWeights ).xyz;
			weights[2] = 1 - (weights[0] + weights[1]);

			float4x3 blendMatrix = mat1 * weights[0] + mat2 * weights[1] + mat3 * weights[2];
			worldPos = mul4x3( modelPos, blendMatrix );
		}
	}
}

void SkinPositionAndNormal( bool bSkinning, const float4 modelPos, const float3 modelNormal,
                            const float4 boneWeights, float4 fBoneIndices,
						    out float3 worldPos, out float3 worldNormal )
{
	// Needed for invariance issues caused by multipass rendering
#if defined( _X360 )
	[isolate] 
#endif
	{ 

#if !defined( _X360 )
		int3 boneIndices = D3DCOLORtoUBYTE4( fBoneIndices );
#else
		int3 boneIndices = fBoneIndices;
#endif

		if ( !bSkinning )
		{
			worldPos = mul4x3( modelPos, cModel[0] );
			worldNormal = mul3x3( modelNormal, ( const float3x3 )cModel[0] );
		}
		else // skinning - always three bones
		{
			float4x3 mat1 = cModel[boneIndices[0]];
			float4x3 mat2 = cModel[boneIndices[1]];
			float4x3 mat3 = cModel[boneIndices[2]];

			float3 weights = DecompressBoneWeights( boneWeights ).xyz;
			weights[2] = 1 - (weights[0] + weights[1]);

			float4x3 blendMatrix = mat1 * weights[0] + mat2 * weights[1] + mat3 * weights[2];
			worldPos = mul4x3( modelPos, blendMatrix );
			worldNormal = mul3x3( modelNormal, ( float3x3 )blendMatrix );
		}

	} // end [isolate]
}

// Is it worth keeping SkinPosition and SkinPositionAndNormal around since the optimizer
// gets rid of anything that isn't used?
void SkinPositionNormalAndTangentSpace( 
							bool bSkinning,
						    const float4 modelPos, const float3 modelNormal, 
							const float4 modelTangentS,
                            const float4 boneWeights, float4 fBoneIndices,
						    out float3 worldPos, out float3 worldNormal, 
							out float3 worldTangentS, out float3 worldTangentT )
{
#if !defined( _X360 )
	int3 boneIndices = D3DCOLORtoUBYTE4( fBoneIndices );
#else
	int3 boneIndices = fBoneIndices;
#endif

	// Needed for invariance issues caused by multipass rendering
#if defined( _X360 )
	[isolate] 
#endif
	{ 
		if ( !bSkinning )
		{
			worldPos = mul4x3( modelPos, cModel[0] );
			worldNormal = mul3x3( modelNormal, ( const float3x3 )cModel[0] );
			worldTangentS = mul3x3( ( float3 )modelTangentS, ( const float3x3 )cModel[0] );
		}
		else // skinning - always three bones
		{
			float4x3 mat1 = cModel[boneIndices[0]];
			float4x3 mat2 = cModel[boneIndices[1]];
			float4x3 mat3 = cModel[boneIndices[2]];

			float3 weights = DecompressBoneWeights( boneWeights ).xyz;
			weights[2] = 1 - (weights[0] + weights[1]);

			float4x3 blendMatrix = mat1 * weights[0] + mat2 * weights[1] + mat3 * weights[2];
			worldPos = mul4x3( modelPos, blendMatrix );
			worldNormal = mul3x3( modelNormal, ( const float3x3 )blendMatrix );
			worldTangentS = mul3x3( ( float3 )modelTangentS, ( const float3x3 )blendMatrix );
		}
		worldTangentT = cross( worldNormal, worldTangentS ) * -modelTangentS.w;
	}
}


//-----------------------------------------------------------------------------
// Lighting helper functions
//-----------------------------------------------------------------------------

float3 AmbientLight( const float3 worldNormal )
{
	float3 nSquared = worldNormal * worldNormal;
	int3 isNegative = ( worldNormal < 0.0 );
	float3 linearColor;
	linearColor = nSquared.x * cAmbientCubeX[isNegative.x] +
	              nSquared.y * cAmbientCubeY[isNegative.y] +
	              nSquared.z * cAmbientCubeZ[isNegative.z];
	return linearColor;
}

// The following "internal" routines are called "privately" by other routines in this file which
// handle the particular flavor of vs20 control flow appropriate to the original caller
float VertexAttenInternal( const float3 worldPos, int lightNum )
{
	float result = 0.0f;

	// Get light direction
	float3 lightDir = cLightInfo[lightNum].pos - worldPos;

	// Get light distance squared.
	float lightDistSquared = dot( lightDir, lightDir );

	// Get 1/lightDistance
	float ooLightDist = rsqrt( lightDistSquared );

	// Normalize light direction
	lightDir *= ooLightDist;

	float3 vDist;
#	if defined( _X360 )
	{
		//X360 dynamic compile hits an internal compiler error using dst(), this is the breakdown of how dst() works from the 360 docs.
		vDist.x = 1;
		vDist.y = lightDistSquared * ooLightDist;
		vDist.z = lightDistSquared;
		//flDist.w = ooLightDist;
	}
#	else
	{
		vDist = dst( lightDistSquared, ooLightDist );
	}
#	endif

	float flDistanceAtten = 1.0f / dot( cLightInfo[lightNum].atten.xyz, vDist );

	// Spot attenuation
	float flCosTheta = dot( cLightInfo[lightNum].dir.xyz, -lightDir );
	float flSpotAtten = (flCosTheta - cLightInfo[lightNum].spotParams.z) * cLightInfo[lightNum].spotParams.w;
	flSpotAtten = max( 0.0001f, flSpotAtten );
	flSpotAtten = pow( flSpotAtten, cLightInfo[lightNum].spotParams.x );
	flSpotAtten = saturate( flSpotAtten );

	// Select between point and spot
	float flAtten = lerp( flDistanceAtten, flDistanceAtten * flSpotAtten, cLightInfo[lightNum].dir.w );

	// Select between above and directional (no attenuation)
	result = lerp( flAtten, 1.0f, cLightInfo[lightNum].color.w );

	return result;
}

float CosineTermInternal( const float3 worldPos, const float3 worldNormal, int lightNum, bool bHalfLambert )
{
	// Calculate light direction assuming this is a point or spot
	float3 lightDir = normalize( cLightInfo[lightNum].pos - worldPos );

	// Select the above direction or the one in the structure, based upon light type
	lightDir = lerp( lightDir, -cLightInfo[lightNum].dir, cLightInfo[lightNum].color.w );

	// compute N dot L
	float NDotL = dot( worldNormal, lightDir );

	if ( !bHalfLambert )
	{
		NDotL = max( 0.0f, NDotL );
	}
	else	// Half-Lambert
	{
		NDotL = NDotL * 0.5 + 0.5;
		NDotL = NDotL * NDotL;
	}
	return NDotL;
}

// This routine uses booleans to do early-outs and is meant to be called by routines OUTSIDE of this file
float GetVertexAttenForLight( const float3 worldPos, int lightNum )
{
	float result = 0.0f;
	if ( g_bLightEnabled[lightNum] )
	{
		result = VertexAttenInternal( worldPos, lightNum );
	}

	return result;
}

// This routine uses booleans to do early-outs and is meant to be called by routines OUTSIDE of this file
float CosineTerm( const float3 worldPos, const float3 worldNormal, int lightNum, bool bHalfLambert )
{
	float flResult = 0.0f;
	if ( g_bLightEnabled[lightNum] )
	{
		flResult = CosineTermInternal( worldPos, worldNormal, lightNum, bHalfLambert );
	}

	return flResult;
}

float3 DoLightInternal( const float3 worldPos, const float3 worldNormal, int lightNum, bool bHalfLambert )
{
	return cLightInfo[lightNum].color *
		CosineTermInternal( worldPos, worldNormal, lightNum, bHalfLambert ) *
		VertexAttenInternal( worldPos, lightNum );
}

// This routine
float3 DoLighting( const float3 worldPos, const float3 worldNormal,
				   const float3 staticLightingColor, const bool bStaticLight,
				   const bool bDynamicLight, bool bHalfLambert )
{
	float3 linearColor = float3( 0.0f, 0.0f, 0.0f );

	if( bStaticLight )			// Static light
	{
		float3 col = staticLightingColor * cOverbright;
#if defined ( _X360 )
		linearColor += col * col;
#else
		linearColor += GammaToLinear( col );
#endif
	}

	if( bDynamicLight )			// Dynamic light
	{
		for (int i = 0; i < g_nLightCount; i++)
		{
			linearColor += DoLightInternal( worldPos, worldNormal, i, bHalfLambert );
		}		
	}

	if( bDynamicLight )
	{
		linearColor += AmbientLight( worldNormal ); //ambient light is already remapped
	}

	return linearColor;
}

float3 DoLightingUnrolled( const float3 worldPos, const float3 worldNormal,
				  const float3 staticLightingColor, const bool bStaticLight,
				  const bool bDynamicLight, bool bHalfLambert, const int nNumLights )
{
	float3 linearColor = float3( 0.0f, 0.0f, 0.0f );

	if( bStaticLight )			// Static light
	{
		linearColor += GammaToLinear( staticLightingColor * cOverbright );
	}

	if( bDynamicLight )			// Ambient light
	{
		if ( nNumLights >= 1 )
			linearColor += DoLightInternal( worldPos, worldNormal, 0, bHalfLambert );
		if ( nNumLights >= 2 )
			linearColor += DoLightInternal( worldPos, worldNormal, 1, bHalfLambert );
		if ( nNumLights >= 3 )
			linearColor += DoLightInternal( worldPos, worldNormal, 2, bHalfLambert );
		if ( nNumLights >= 4 )
			linearColor += DoLightInternal( worldPos, worldNormal, 3, bHalfLambert );
	}

	if( bDynamicLight )
	{
		linearColor += AmbientLight( worldNormal ); //ambient light is already remapped
	}

	return linearColor;
}

int4 FloatToInt( in float4 floats )
{
	return D3DCOLORtoUBYTE4( floats.zyxw / 255.001953125 );
}

float2 ComputeSphereMapTexCoords( in float3 reflectionVector )
{
	// transform reflection vector into view space
	reflectionVector = mul( reflectionVector, ( float3x3 )cViewModel );

	// generate <rx ry rz+1>
	float3 tmp = float3( reflectionVector.x, reflectionVector.y, reflectionVector.z + 1.0f );

	// find 1 / len
	float ooLen = dot( tmp, tmp );
	ooLen = 1.0f / sqrt( ooLen );

	// tmp = tmp/|tmp| + 1
	tmp.xy = ooLen * tmp.xy + 1.0f;

	return tmp.xy * 0.5f;
}


#define DEFORMATION_CLAMP_TO_BOX_IN_WORLDSPACE 1
							// minxyz.minsoftness / maxxyz.maxsoftness
float3 ApplyDeformation( float3 worldpos, int deftype, float4 defparms0, float4 defparms1,
						 float4 defparms2, float4 defparms3 )
{
	float3 ret = worldpos;
	if ( deftype == DEFORMATION_CLAMP_TO_BOX_IN_WORLDSPACE )
	{
		ret=max( ret, defparms2.xyz );
		ret=min( ret, defparms3.xyz );
	}

	return ret;
}


#endif //#ifndef COMMON_VS_FXC_H_
