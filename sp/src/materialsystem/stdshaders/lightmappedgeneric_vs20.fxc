//	STATIC: "ENVMAP_MASK"				"0..1"
//	STATIC: "TANGENTSPACE"				"0..1"
//  STATIC: "BUMPMAP"					"0..1"
//  STATIC: "DIFFUSEBUMPMAP"			"0..1"
//  STATIC: "VERTEXCOLOR"				"0..1"
//  STATIC: "VERTEXALPHATEXBLENDFACTOR"	"0..1"
//  STATIC: "RELIEF_MAPPING"            "0..0"
//  STATIC: "SEAMLESS"                  "0..1"
//  STATIC: "BUMPMASK"                  "0..1"
//	STATIC: "FLASHLIGHT"				"0..1"	[XBOX]

//  DYNAMIC: "FASTPATH"					"0..1"
//	DYNAMIC: "DOWATERFOG"				"0..1"
//  DYNAMIC: "LIGHTING_PREVIEW"			"0..1"	[PC]
//  DYNAMIC: "LIGHTING_PREVIEW"			"0..0"	[XBOX]

// This should not be a combo since I'm a moron with the tangent space and the flashlight.
//  SKIP: !$BUMPMAP && $DIFFUSEBUMPMAP
//  SKIP: $SEAMLESS && $RELIEF_MAPPING
//  SKIP: $BUMPMASK && $RELIEF_MAPPING
//  SKIP: $BUMPMASK && $SEAMLESS

#include "common_vs_fxc.h"

static const int g_FogType						= DOWATERFOG;
static const bool g_UseSeparateEnvmapMask		= ENVMAP_MASK;
static const bool g_bTangentSpace				= TANGENTSPACE;
static const bool g_bBumpmap					= BUMPMAP;
static const bool g_bBumpmapDiffuseLighting		= DIFFUSEBUMPMAP;
static const bool g_bVertexColor				= VERTEXCOLOR;
static const bool g_bVertexAlphaTexBlendFactor	= VERTEXALPHATEXBLENDFACTOR;
static const bool g_BumpMask					= BUMPMASK;

#if SEAMLESS
const float4 SeamlessScale : register( SHADER_SPECIFIC_CONST_0 );
#define SEAMLESS_SCALE (SeamlessScale.x)
#else
const float4 cBaseTexCoordTransform[2]			:  register( SHADER_SPECIFIC_CONST_0 );
const float4 cDetailOrBumpTexCoordTransform[2]	:  register( SHADER_SPECIFIC_CONST_2 );
#endif
// This should be identity if we are bump mapping, otherwise we'll screw up the lightmapTexCoordOffset.
const float4 cEnvmapMaskTexCoordTransform[2]	:  register( SHADER_SPECIFIC_CONST_4 );
const float4x4 g_FlashlightWorldToTexture		:  register( SHADER_SPECIFIC_CONST_6 );
const float4 cBlendMaskTexCoordTransform[2]     :  register( SHADER_SPECIFIC_CONST_10 );	// not contiguous with the rest!

struct VS_INPUT
{
	float3 vPos							: POSITION;
	float4 vNormal						: NORMAL;
	float2 vBaseTexCoord				: TEXCOORD0;
	float2 vLightmapTexCoord			: TEXCOORD1;
	float2 vLightmapTexCoordOffset		: TEXCOORD2;
	float3 vTangentS					: TANGENT;
	float3 vTangentT					: BINORMAL;
	float4 vColor						: COLOR0;
};

struct VS_OUTPUT
{
    float4 projPos					: POSITION;	
#if !defined( _X360 )    
	float  fog						: FOG;
#endif

#if SEAMLESS
	float3 SeamlessTexCoord         : TEXCOORD0;            // x y z 
	float4 detailOrBumpAndEnvmapMaskTexCoord : TEXCOORD1;   // envmap mask
#else
	float2 baseTexCoord				: TEXCOORD0;
	// detail textures and bumpmaps are mutually exclusive so that we have enough texcoords.
#if RELIEF_MAPPING
	float3 TangentSpaceViewRay		: TEXCOORD1;
#else
	float4 detailOrBumpAndEnvmapMaskTexCoord : TEXCOORD1;
#endif
#endif
	float4 lightmapTexCoord1And2	: TEXCOORD2;
	float4 lightmapTexCoord3		: TEXCOORD3;			// and basetexcoord*mask_scale
	float4 worldPos_projPosZ		: TEXCOORD4;

#if TANGENTSPACE || (LIGHTING_PREVIEW) || defined( _X360 )
	float3x3 tangentSpaceTranspose	: TEXCOORD5;	// and 6 and 7
#endif

	float4 vertexColor				: COLOR;				// in seamless, r g b = blend weights
	float4 vertexBlendX_fogFactorW	: COLOR1;

	// Extra iterators on 360, used in flashlight combo
#if defined( _X360 )
#if FLASHLIGHT
	float4 flashlightSpacePos		: TEXCOORD8;
	float4 vProjPos					: TEXCOORD9;
#endif
#endif

};

VS_OUTPUT main( const VS_INPUT v )
{
	VS_OUTPUT o = ( VS_OUTPUT )0;

	float3 vObjNormal;
	DecompressVertex_Normal( v.vNormal, vObjNormal );

	float3 worldPos = mul( float4( v.vPos, 1 ), cModel[0] );

	float4 vProjPos = mul( float4( v.vPos, 1 ), cModelViewProj );
	o.projPos = vProjPos;
	vProjPos.z = dot( float4( v.vPos, 1 ), cModelViewProjZ );
	
	o.worldPos_projPosZ = float4( worldPos, vProjPos.z );
	
	float3 worldNormal = mul( vObjNormal, ( float3x3 )cModel[0] );
	
#if TANGENTSPACE || (LIGHTING_PREVIEW) || defined( _X360 )
	float3 worldTangentS = mul( v.vTangentS, ( const float3x3 )cModel[0] );
	float3 worldTangentT = mul( v.vTangentT, ( const float3x3 )cModel[0] );

	#if SEAMLESS && BUMPMAP && defined( _X360 )
		float3 n = normalize( worldNormal );
		float3 n2 = n * n;           // sums to 1.

		o.tangentSpaceTranspose[0] = normalize( float3( n2.y + n2.z, 0.0f, n2.x ) );
		o.tangentSpaceTranspose[1] = normalize( float3( 0.0f, n2.x + n2.z, n2.y ) );
		o.tangentSpaceTranspose[2] = worldNormal;
	#else
		o.tangentSpaceTranspose[0] = worldTangentS;
		o.tangentSpaceTranspose[1] = worldTangentT;
		o.tangentSpaceTranspose[2] = worldNormal;
	#endif

#endif

	float3 worldVertToEyeVector = VSHADER_VECT_SCALE * (cEyePos - worldPos);

#if SEAMLESS
	{
		// we need to fill in the texture coordinate projections
		o.SeamlessTexCoord = SEAMLESS_SCALE*worldPos;
	}
#else
	{
		if (FASTPATH)
		{
			o.baseTexCoord.xy = v.vBaseTexCoord;
		}
		else
		{
			o.baseTexCoord.x = dot( v.vBaseTexCoord, cBaseTexCoordTransform[0] ) + cBaseTexCoordTransform[0].w;
			o.baseTexCoord.y = dot( v.vBaseTexCoord, cBaseTexCoordTransform[1] ) + cBaseTexCoordTransform[1].w;
		}
#if ( RELIEF_MAPPING == 0 )
		{
            // calculate detailorbumptexcoord
			if ( FASTPATH )
				o.detailOrBumpAndEnvmapMaskTexCoord.xy = v.vBaseTexCoord.xy;
			else
			{
				o.detailOrBumpAndEnvmapMaskTexCoord.x = dot( v.vBaseTexCoord, cDetailOrBumpTexCoordTransform[0] ) + cDetailOrBumpTexCoordTransform[0].w;
				o.detailOrBumpAndEnvmapMaskTexCoord.y = dot( v.vBaseTexCoord, cDetailOrBumpTexCoordTransform[1] ) + cDetailOrBumpTexCoordTransform[1].w;
			}
		}
#endif
	}
#endif
	if ( FASTPATH )
	{
		o.lightmapTexCoord3.zw = v.vBaseTexCoord;
	}
	else
	{
		o.lightmapTexCoord3.z = dot( v.vBaseTexCoord, cBlendMaskTexCoordTransform[0] ) + cBlendMaskTexCoordTransform[0].w;
		o.lightmapTexCoord3.w = dot( v.vBaseTexCoord, cBlendMaskTexCoordTransform[1] ) + cBlendMaskTexCoordTransform[1].w;
	}
	
	//  compute lightmap coordinates
	if( g_bBumpmap && g_bBumpmapDiffuseLighting )
	{
		o.lightmapTexCoord1And2.xy = v.vLightmapTexCoord + v.vLightmapTexCoordOffset;

		float2 lightmapTexCoord2 = o.lightmapTexCoord1And2.xy + v.vLightmapTexCoordOffset;
		float2 lightmapTexCoord3 = lightmapTexCoord2 + v.vLightmapTexCoordOffset;

		// reversed component order
		o.lightmapTexCoord1And2.w = lightmapTexCoord2.x;
		o.lightmapTexCoord1And2.z = lightmapTexCoord2.y;

		o.lightmapTexCoord3.xy = lightmapTexCoord3;
	}
	else
	{
		o.lightmapTexCoord1And2.xy = v.vLightmapTexCoord;
	}

#if ( RELIEF_MAPPING == 0)
	if( g_UseSeparateEnvmapMask || g_BumpMask )
	{
		// reversed component order
#	if FASTPATH
		o.detailOrBumpAndEnvmapMaskTexCoord.wz = v.vBaseTexCoord.xy;
#	else
		o.detailOrBumpAndEnvmapMaskTexCoord.w = dot( v.vBaseTexCoord, cEnvmapMaskTexCoordTransform[0] ) + cEnvmapMaskTexCoordTransform[0].w;
		o.detailOrBumpAndEnvmapMaskTexCoord.z = dot( v.vBaseTexCoord, cEnvmapMaskTexCoordTransform[1] ) + cEnvmapMaskTexCoordTransform[1].w;
#	endif
	}
#endif

	o.vertexBlendX_fogFactorW = CalcFog( worldPos, vProjPos, g_FogType );
#if !defined( _X360 )
	o.fog = o.vertexBlendX_fogFactorW;
#endif

	if (!g_bVertexColor)
	{
		o.vertexColor = float4( 1.0f, 1.0f, 1.0f, cModulationColor.a );
	}
	else
	{
#if FASTPATH
		o.vertexColor = v.vColor;
#else
		if ( g_bVertexAlphaTexBlendFactor )
		{
			o.vertexColor.rgb = v.vColor.rgb;
			o.vertexColor.a = cModulationColor.a;
		}
		else
		{
			o.vertexColor = v.vColor;
			o.vertexColor.a *= cModulationColor.a;
		}
#endif
	}
#if SEAMLESS
	// compute belnd weights in rgb
	float3 vNormal=normalize( worldNormal );
	o.vertexColor.xyz = vNormal * vNormal;           // sums to 1.
#endif

// On 360, we have extra iterators and can fold the flashlight into this shader
#if defined( _X360 )
	#if FLASHLIGHT
		o.flashlightSpacePos = mul( float4( worldPos, 1.0f ), g_FlashlightWorldToTexture );
		o.vProjPos = vProjPos;
	#endif
#endif

	if ( g_bVertexAlphaTexBlendFactor )
	{
		o.vertexBlendX_fogFactorW.r = v.vColor.a;
	}
	
	return o;
}
