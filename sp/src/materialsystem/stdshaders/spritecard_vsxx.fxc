//  STATIC: "ZOOM_ANIMATE_SEQ2"		"0..1" [vs20]
//  STATIC: "DUALSEQUENCE"			"0..1" [vs20]
//	STATIC: "EXTRACTGREENALPHA"		"0..1" [vs20]

//  STATIC: "ZOOM_ANIMATE_SEQ2"		"0..0" [vs11]
//  STATIC: "DUALSEQUENCE"			"0..0" [vs11]
//	STATIC: "EXTRACTGREENALPHA"		"0..0" [vs11]

//	STATIC: "USE_INSTANCING"		"0..1" [vs20]
//	DYNAMIC: "ORIENTATION"			"0..2"


#include "common_vs_fxc.h"

const float4x3 cModelView			: register(SHADER_SPECIFIC_CONST_0);
const float4x4 cProj				: register(SHADER_SPECIFIC_CONST_3);

#if ZOOM_ANIMATE_SEQ2
const float4 ScaleParms             : register(SHADER_SPECIFIC_CONST_7);
#define OLDFRM_SCALE_START (ScaleParms.x)
#define OLDFRM_SCALE_END (ScaleParms.y)
#endif

const float4 SizeParms              : register(SHADER_SPECIFIC_CONST_8);
const float4 SizeParms2             : register(SHADER_SPECIFIC_CONST_9);
const float4 ViewportTransformScaled : register(SHADER_SPECIFIC_CONST_10);

#define MINIMUM_SIZE_FACTOR (SizeParms.x)
#define MAXIMUM_SIZE_FACTOR (SizeParms.y)

#define START_FADE_SIZE_FACTOR (SizeParms.z)
#define END_FADE_SIZE_FACTOR (SizeParms.w)

// alpha fade w/ distance
#define START_FAR_FADE  ( SizeParms2.x )
#define FAR_FADE_FACTOR ( SizeParms2.y )	   // alpha = 1-min(1,max(0, (dist-start_fade)*factor))

// Define stuff for instancing on 360
#if ( defined( _X360 ) && defined( SHADER_MODEL_VS_2_0 ) )
#define CONST_PC 
#define VERTEX_INDEX_PARAM_360 ,int Index:INDEX
#define DO_INSTANCING 1
#else
#define CONST_PC const
#define VERTEX_INDEX_PARAM_360
#endif


struct VS_INPUT
{
	// This is all of the stuff that we ever use.
	float4 vTint            : COLOR;
	float4 vPos				: POSITION;
	float4 vTexCoord0		: TEXCOORD0;
	float4 vTexCoord1		: TEXCOORD1;
	float4 vParms           : TEXCOORD2;   // frame blend, rot, radius, yaw
	// FIXME: remove this vertex element for (USE_INSTANCING == 1), need to shuffle the following elements down
	float2 vCornerID        : TEXCOORD3;   // 0,0 1,0 1,1 0,1
	float4 vTexCoord2		: TEXCOORD4;
#if DUALSEQUENCE
	float4 vSeq2TexCoord0   : TEXCOORD5;
	float4 vSeq2TexCoord1   : TEXCOORD6; 
	float4 vParms1          : TEXCOORD7;  // second frame blend, ?,?,?
#endif
};

struct VS_OUTPUT
{
    float4 projPos			: POSITION;	

	float2 texCoord0		: TEXCOORD0;
	float2 texCoord1		: TEXCOORD1;
	float4 argbcolor		: COLOR;
	float4 blendfactor0		: TEXCOORD2;
	float2 texCoord2		: TEXCOORD3;
#if !defined( SHADER_MODEL_VS_1_1 ) 
	float4 blendfactor1		: TEXCOORD4;	// for extracting green/alpha
#endif
#if DUALSEQUENCE
	float2 vSeq2TexCoord0   : TEXCOORD5;
	float2 vSeq2TexCoord1   : TEXCOORD6;
#endif

#if defined( _X360 )
	float4 vScreenPos_ReverseZ		: TEXCOORD7;
#else
	float4 vScreenPos		: TEXCOORD7;
#endif
};

#define BLENDFACTOR v.vParms.x
#define ROTATION v.vParms.y
#define RADIUS v.vParms.z
#define YAW (v.vParms.w)

#if ( ZOOM_ANIMATE_SEQ2 ) 
float getlerpscaled( float l_in, float s0, float s1, float ts )
{
	l_in = 2.0*(l_in-.5);
	l_in *= lerp(s0,s1,ts);
	return 0.5+0.5*l_in;
}

float getlerpscale_for_old_frame( float l_in, float ts )
{
	return getlerpscaled( l_in, OLDFRM_SCALE_START, OLDFRM_SCALE_END, ts);
}

float getlerpscale_for_new_frame( float l_in, float ts )
{
	return getlerpscaled( l_in, 1.0, OLDFRM_SCALE_START, ts );
}
#endif // ZOOM_ANIMATE_SEQ2

#ifdef DO_INSTANCING
void InstancedVertexRead( inout VS_INPUT v, int index )
{
	// Duplicate each VB vertex 4 times (and generate vCornerID - the only thing that varies per-corner)
	float4 vTint;
	float4 vPos;
	float4 vTexCoord0;
	float4 vTexCoord1;
	float4 vParms;
	float4 vTexCoord2;
	float4 vSeq_TexCoord0; // NOTE: April XDK compiler barfs on var names which have a number in the middle! (i.e. vSeq2TexCoord0)
	float4 vSeq_TexCoord1;
	float4 vParms1;

	int spriteIndex = index / 4;
	int cornerIndex = index - 4*spriteIndex;
	asm
	{
		vfetch vTint,			spriteIndex, color0;
		vfetch vPos,			spriteIndex, position0;
		vfetch vTexCoord0,		spriteIndex, texcoord0;
		vfetch vTexCoord1,		spriteIndex, texcoord1;
		vfetch vParms,			spriteIndex, texcoord2;
		vfetch vTexCoord2,		spriteIndex, texcoord4;
#if DUALSEQUENCE
		vfetch vSeq_TexCoord0,	spriteIndex, texcoord5;
		vfetch vSeq_TexCoord1,	spriteIndex, texcoord6;
		vfetch vParms1,			spriteIndex, texcoord7;
#endif
	};

	v.vTint				= vTint;
	v.vPos				= vPos;
	v.vTexCoord0		= vTexCoord0;
	v.vTexCoord1		= vTexCoord1;
	v.vParms			= vParms;
	v.vTexCoord2		= vTexCoord2;
#if DUALSEQUENCE
	v.vSeq2TexCoord0	= vSeq_TexCoord0;
	v.vSeq2TexCoord1	= vSeq_TexCoord1;
	v.vParms1			= vParms1;
#endif

	// Compute vCornerID - order is: (0,0) (1,0) (1,1) (0,1)
	//   float2 IDs[4] = { {0,0}, {1,0}, {1,1}, {0,1} };
	//   v.vCornerID.xy = IDs[ cornerIndex ];
	// This compiles to 2 ops on 360 (MADDs with abs/sat register read/write modifiers):
	v.vCornerID.xy = float2( 1.5f, 0.0f ) + cornerIndex*float2( -1.0f, 1.0f );
	v.vCornerID.xy = saturate( float2(1.5f, -3.0f) + float2( -1.0f, 2.0f )*abs( v.vCornerID.xy ) );
}
#endif

VS_OUTPUT main( CONST_PC VS_INPUT v
			    VERTEX_INDEX_PARAM_360 )
{
	VS_OUTPUT o;

#ifdef DO_INSTANCING
	if ( USE_INSTANCING )
	{
		InstancedVertexRead( v, Index );
	}
#endif

#if SHADER_MODEL_VS_1_1
	float4 tint = v.vTint;
#else
	float4 tint = GammaToLinear( v.vTint );
#endif

	float2 sc_yaw;
	sincos( YAW, sc_yaw.y, sc_yaw.x );

	float2 sc;
	sincos( ROTATION, sc.y, sc.x );

	float2 ix=2*v.vCornerID.xy-1;
	float x1=dot(ix,sc);
	float y1=sc.x*ix.y-sc.y*ix.x;

	float4 projPos;
	float3 worldPos;
	worldPos = mul4x3( v.vPos, cModel[0] );

	float rad = RADIUS;
	float3 v2p = ( worldPos - cEyePos );
	float l = length(v2p);
	rad=max(rad, MINIMUM_SIZE_FACTOR * l);
	// now, perform fade out
#ifndef SHADER_MODEL_VS_1_1
	if ( rad > START_FADE_SIZE_FACTOR * l )
	{
		if ( rad > END_FADE_SIZE_FACTOR *l )
		{
			tint = 0;
			rad = 0;											// cull so we emit 0-sized sprite
		}
		else
		{
			tint *= 1-(rad-START_FADE_SIZE_FACTOR*l)/(END_FADE_SIZE_FACTOR*l-START_FADE_SIZE_FACTOR*l);
		}
	}
#endif


#ifndef SHADER_MODEL_VS_1_1
	// perform far fade
	float tscale =  1-min(1, max(0, (l-START_FAR_FADE)*FAR_FADE_FACTOR) );
	tint *= tscale;

	if ( tscale <= 0)
		rad = 0;											// cull so we emit 0-sized sprite
#endif

	rad=min(rad, MAXIMUM_SIZE_FACTOR * l);

#if ORIENTATION == 0
	// Screen-aligned case
	float3 viewPos;
	viewPos = mul4x3( v.vPos, cModelView );

	float3 disp=float3( -x1,y1,0);
 	float tmpx=disp.x*sc_yaw.x+disp.z*sc_yaw.y;
 	disp.z = disp.z*sc_yaw.x-disp.x*sc_yaw.y;
 	disp.x=tmpx;

	viewPos.xyz += disp * rad;

	projPos  = mul( float4(viewPos, 1.0f), cProj );
#endif
	
#if ORIENTATION == 1
	// Z-aligned case
	if (l > rad/2)
	{
		float3 up = float3(0,0,1);
		float3 right = normalize(cross(up, v2p));
		float tmpx=right.x*sc_yaw.x+right.y*sc_yaw.y;
		right.y = right.y*sc_yaw.x-right.x*sc_yaw.y;
		right.x=tmpx;
		
		worldPos += (x1*rad)*right;
		worldPos.z += (y1*rad)*up.z;
		
#ifndef SHADER_MODEL_VS_1_1
		if (l < rad*2 )
		{
			tint *= smoothstep(rad/2,rad,l);
		}
#endif

	}
	projPos  = mul( float4(worldPos, 1.0f), cViewProj );
#endif

#if ORIENTATION == 2
	// aligned with z plane case - easy
	float3 wpos=v.vPos+RADIUS*float3( y1,x1,0);
	projPos  = mul( float4(wpos, 1.0f), cModelViewProj );
#endif

	o.blendfactor0 = float4( v.vParms.x, 0, 0, 0 );
	o.projPos		= projPos;
	o.texCoord0.x	= lerp( v.vTexCoord0.z, v.vTexCoord0.x, v.vCornerID.x );
	o.texCoord0.y	= lerp( v.vTexCoord0.w, v.vTexCoord0.y, v.vCornerID.y );
	o.texCoord1.x	= lerp( v.vTexCoord1.z, v.vTexCoord1.x, v.vCornerID.x );
	o.texCoord1.y	= lerp( v.vTexCoord1.w, v.vTexCoord1.y, v.vCornerID.y );
	o.texCoord2.x	= lerp( v.vTexCoord2.z, v.vTexCoord2.x, v.vCornerID.x );
	o.texCoord2.y	= lerp( v.vTexCoord2.w, v.vTexCoord2.y, v.vCornerID.y );

#if ( DUALSEQUENCE )
	float2 lerpold = v.vCornerID.xy;
	float2 lerpnew = v.vCornerID.xy;

#if ( ZOOM_ANIMATE_SEQ2 )
		lerpold.x = getlerpscale_for_old_frame( v.vCornerID.x, v.vParms1.x );
		lerpold.y = getlerpscale_for_old_frame( v.vCornerID.y, v.vParms1.x );
		lerpnew.x = getlerpscale_for_new_frame( v.vCornerID.x, v.vParms1.x );
		lerpnew.y = getlerpscale_for_new_frame( v.vCornerID.y, v.vParms1.x );
#endif

	o.vSeq2TexCoord0.xy	= lerp( v.vSeq2TexCoord0.zw, v.vSeq2TexCoord0.xy, lerpold.xy );
	o.vSeq2TexCoord1.xy	= lerp( v.vSeq2TexCoord1.zw, v.vSeq2TexCoord1.xy, lerpnew.xy );

	o.blendfactor0.z = v.vParms1.x;
#endif


#if !defined( SHADER_MODEL_VS_1_1 ) 

	o.blendfactor1 = float4( 0.0f, 0.0f, 0.0f, 0.0f );

#if ( EXTRACTGREENALPHA )
														// Input range	  Output range
		if ( v.vParms.x < 0.25f )						// 0.0 .. 0.25
		{
			o.blendfactor0.a = v.vParms.x * 2 + 0.5f;					// 0.5 .. 1.0
			o.blendfactor0.g = 1 - o.blendfactor0.a;					// 0.5 .. 0.0
		}
		else if ( v.vParms.x < 0.75f )					// 0.25 .. 0.75
		{
			o.blendfactor1.g = v.vParms.x * 2 - 0.5f;					// 0.0 .. 1.0
			o.blendfactor0.a = 1 - o.blendfactor1.g;					// 1.0 .. 0.0
		}
		else											// 0.75 .. 1.0	
		{
			o.blendfactor1.a = v.vParms.x * 2 - 1.5f;					// 0.0 .. 0.5
			o.blendfactor1.g = 1 - o.blendfactor1.a;					// 1.0 .. 0.5
		}
#endif

#endif

	// Map projected position to the refraction texture
	float2 vScreenPos;
	vScreenPos.x = projPos.x;
	vScreenPos.y = -projPos.y; // invert Y
	vScreenPos = (vScreenPos + projPos.w) * 0.5f;

	// Need to also account for the viewport transform, which matters when rendering with mat_viewportscale != 1.0
	vScreenPos = (vScreenPos * ViewportTransformScaled.xy) + (projPos.w * ViewportTransformScaled.zw);
	
#if defined( _X360 )
	o.vScreenPos_ReverseZ = float4(vScreenPos.x, vScreenPos.y, projPos.w - projPos.z, projPos.w );
#else
	o.vScreenPos = float4(vScreenPos.x, vScreenPos.y, projPos.z, projPos.w );
#endif

	o.argbcolor = tint;
	return o;
}


