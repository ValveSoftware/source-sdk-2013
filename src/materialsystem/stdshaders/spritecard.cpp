//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: shader for drawing sprites as cards, with animation frame lerping
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"
#include "convar.h"

// STDSHADER_DX9_DLL_EXPORT
#include "spritecard_ps20.inc"
#include "spritecard_ps20b.inc"
#include "spritecard_vs20.inc"
#include "splinecard_vs20.inc"

#if SUPPORT_DX8
// STDSHADER_DX8_DLL_EXPORT
#include "spritecard_vs11.inc"
#include "spritecard_ps11.inc"
#include "splinecard_vs11.inc"
#endif

#include "tier0/icommandline.h" //command line

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DEFAULT_PARTICLE_FEATHERING_ENABLED 1

#ifdef STDSHADER_DX8_DLL_EXPORT
DEFINE_FALLBACK_SHADER( Spritecard, Spritecard_DX8 )
#endif

int GetDefaultDepthFeatheringValue( void ) //Allow the command-line to go against the default soft-particle value
{
	static int iRetVal = -1;

	if( iRetVal == -1 )
	{
#		if( DEFAULT_PARTICLE_FEATHERING_ENABLED == 1 )
		{
			if( CommandLine()->CheckParm( "-softparticlesdefaultoff" ) )
				iRetVal = 0;
			else
				iRetVal = 1;
		}
#		else
		{
			if( CommandLine()->CheckParm( "-softparticlesdefaulton" ) )
				iRetVal = 1;
			else
				iRetVal = 0;
		}
#		endif
	}

	// On low end parts on the Mac, we reduce particles and shut off depth blending here
	static ConVarRef mat_reduceparticles( "mat_reduceparticles" );
	if ( mat_reduceparticles.GetBool() )
	{
		iRetVal = 0;
	}

	return iRetVal;
}


#ifdef STDSHADER_DX9_DLL_EXPORT
BEGIN_VS_SHADER_FLAGS( Spritecard, "Help for Spritecard", SHADER_NOT_EDITABLE )
#else
BEGIN_VS_SHADER_FLAGS( Spritecard_DX8, "Help for Spritecard_DX8", SHADER_NOT_EDITABLE )
#endif

BEGIN_SHADER_PARAMS
SHADER_PARAM( DEPTHBLEND, SHADER_PARAM_TYPE_INTEGER, "0", "fade at intersection boundaries" )
SHADER_PARAM( DEPTHBLENDSCALE, SHADER_PARAM_TYPE_FLOAT, "50.0", "Amplify or reduce DEPTHBLEND fading. Lower values make harder edges." )
SHADER_PARAM( ORIENTATION, SHADER_PARAM_TYPE_INTEGER, "0", "0 = always face camera, 1 = rotate around z, 2= parallel to ground" )
SHADER_PARAM( ADDBASETEXTURE2, SHADER_PARAM_TYPE_FLOAT, "0.0", "amount to blend second texture into frame by" )
SHADER_PARAM( OVERBRIGHTFACTOR, SHADER_PARAM_TYPE_FLOAT, "1.0", "overbright factor for texture. For HDR effects.")
SHADER_PARAM( DUALSEQUENCE, SHADER_PARAM_TYPE_INTEGER, "0", "blend two separate animated sequences.")
SHADER_PARAM( SEQUENCE_BLEND_MODE, SHADER_PARAM_TYPE_INTEGER, "0", "defines the blend mode between the images un dual sequence particles. 0 = avg, 1=alpha from first, rgb from 2nd, 2= first over second" )
SHADER_PARAM( MAXLUMFRAMEBLEND1, SHADER_PARAM_TYPE_INTEGER, "0", "instead of blending between animation frames for the first sequence, select pixels based upon max luminance" )
SHADER_PARAM( MAXLUMFRAMEBLEND2, SHADER_PARAM_TYPE_INTEGER, "0", "instead of blending between animation frames for the 2nd sequence, select pixels based upon max luminance" )
SHADER_PARAM( RAMPTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "if specified, then the red value of the image is used to index this ramp to produce the output color" )
SHADER_PARAM( ZOOMANIMATESEQ2, SHADER_PARAM_TYPE_FLOAT, "1.0", "amount to gradually zoom between frames on the second sequence. 2.0 will double the size of a frame over its lifetime.")
SHADER_PARAM( EXTRACTGREENALPHA, SHADER_PARAM_TYPE_INTEGER, "0", "grayscale data sitting in green/alpha channels")
SHADER_PARAM( ADDOVERBLEND, SHADER_PARAM_TYPE_INTEGER, "0", "use ONE:INVSRCALPHA blending")
SHADER_PARAM( ADDSELF, SHADER_PARAM_TYPE_FLOAT, "0.0", "amount of base texture to additively blend in" )
SHADER_PARAM( BLENDFRAMES, SHADER_PARAM_TYPE_BOOL, "1", "whether or not to smoothly blend between animated frames" )
SHADER_PARAM( MINSIZE, SHADER_PARAM_TYPE_FLOAT, "0.0", "minimum screen fractional size of particle")
SHADER_PARAM( STARTFADESIZE, SHADER_PARAM_TYPE_FLOAT, "10.0", "screen fractional size to start fading particle out")
SHADER_PARAM( ENDFADESIZE, SHADER_PARAM_TYPE_FLOAT, "20.0", "screen fractional size to finish fading particle out")
SHADER_PARAM( MAXSIZE, SHADER_PARAM_TYPE_FLOAT, "20.0", "maximum screen fractional size of particle")
SHADER_PARAM( USEINSTANCING, SHADER_PARAM_TYPE_BOOL, "1", "whether to use GPU vertex instancing (submit 1 vert per particle quad)")
SHADER_PARAM( SPLINETYPE, SHADER_PARAM_TYPE_INTEGER, "0", "spline type 0 = none,  1=ctamull rom")
SHADER_PARAM( MAXDISTANCE, SHADER_PARAM_TYPE_FLOAT, "100000.0", "maximum distance to draw particles at")
SHADER_PARAM( FARFADEINTERVAL, SHADER_PARAM_TYPE_FLOAT, "400.0", "interval over which to fade out far away particles")
END_SHADER_PARAMS

SHADER_INIT_PARAMS()
{
	INIT_FLOAT_PARM( MAXDISTANCE, 100000.0);
	INIT_FLOAT_PARM( FARFADEINTERVAL, 400.0);
	INIT_FLOAT_PARM( MAXSIZE, 20.0 );
	INIT_FLOAT_PARM( ENDFADESIZE, 20.0 );
	INIT_FLOAT_PARM( STARTFADESIZE, 10.0 );
	INIT_FLOAT_PARM( DEPTHBLENDSCALE, 50.0 );
	INIT_FLOAT_PARM( OVERBRIGHTFACTOR, 1.0 );
	INIT_FLOAT_PARM( ADDBASETEXTURE2, 0.0 );
	INIT_FLOAT_PARM( ADDSELF, 0.0 );
	INIT_FLOAT_PARM( ZOOMANIMATESEQ2, 0.0 );

	if ( !params[DEPTHBLEND]->IsDefined() )
	{
		params[ DEPTHBLEND ]->SetIntValue( GetDefaultDepthFeatheringValue() );
	}
	if ( !g_pHardwareConfig->SupportsPixelShaders_2_b() )
	{
		params[ DEPTHBLEND ]->SetIntValue( 0 );
	}
	if ( !params[DUALSEQUENCE]->IsDefined() )
	{
		params[DUALSEQUENCE]->SetIntValue( 0 );
	}
	if ( !params[MAXLUMFRAMEBLEND1]->IsDefined() )
	{
		params[MAXLUMFRAMEBLEND1]->SetIntValue( 0 );
	}
	if ( !params[MAXLUMFRAMEBLEND2]->IsDefined() )
	{
		params[MAXLUMFRAMEBLEND2]->SetIntValue( 0 );
	}
	if ( !params[EXTRACTGREENALPHA]->IsDefined() )
	{
		params[EXTRACTGREENALPHA]->SetIntValue( 0 );
	}
	if ( !params[ADDOVERBLEND]->IsDefined() )
	{
		params[ADDOVERBLEND]->SetIntValue( 0 );
	}
	if ( !params[BLENDFRAMES]->IsDefined() )
	{
		params[ BLENDFRAMES ]->SetIntValue( 1 );
	}
	if ( !params[USEINSTANCING]->IsDefined() )
	{
		params[ USEINSTANCING ]->SetIntValue( IsX360() ? 1 : 0 );
	}
	SET_FLAGS2( MATERIAL_VAR2_IS_SPRITECARD );
}

SHADER_FALLBACK
{
#ifdef STDSHADER_DX9_DLL_EXPORT
	if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		return "SpriteCard_DX8";
#endif
#ifdef STDSHADER_DX8_DLL_EXPORT
	// STDSHADER_DX8_DLL_EXPORT
	if ( g_pHardwareConfig->GetDXSupportLevel() < 80 )
		return "Wireframe";
#endif
	return 0;
}

SHADER_INIT
{
#ifdef STDSHADER_DX9_DLL_EXPORT
	const bool bDX8 = false;
#endif
#ifdef STDSHADER_DX8_DLL_EXPORT
	const bool bDX8 = true;
#endif

	SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );

	if ( params[BASETEXTURE]->IsDefined() )
	{
		bool bExtractGreenAlpha = false;
		if ( params[EXTRACTGREENALPHA]->IsDefined() )
		{
			bExtractGreenAlpha = params[EXTRACTGREENALPHA]->GetIntValue() != 0;
		}

		LoadTexture( BASETEXTURE, !bExtractGreenAlpha && !bDX8 ? TEXTUREFLAGS_SRGB : 0 );
	}
	if ( params[RAMPTEXTURE]->IsDefined() )
	{
		LoadTexture( RAMPTEXTURE, TEXTUREFLAGS_SRGB );
	}
}

SHADER_DRAW
{
#ifdef STDSHADER_DX9_DLL_EXPORT
	const bool bDX8 = false;
#endif
#ifdef STDSHADER_DX8_DLL_EXPORT
	const bool bDX8 = true;
#endif
	bool bUseRampTexture = (! bDX8 ) && ( params[RAMPTEXTURE]->IsDefined() );
	bool bZoomSeq2 = (! bDX8 ) && ( ( params[ZOOMANIMATESEQ2]->GetFloatValue()) > 1.0 );
	bool bDepthBlend = (! bDX8 ) && ( params[DEPTHBLEND]->GetIntValue() != 0 );
	bool bAdditive2ndTexture = params[ADDBASETEXTURE2]->GetFloatValue() != 0.0;
	int nSplineType = params[SPLINETYPE]->GetIntValue();

	SHADOW_STATE
	{
		bool bSecondSequence = params[DUALSEQUENCE]->GetIntValue() != 0;
		bool bAddOverBlend = params[ADDOVERBLEND]->GetIntValue() != 0;
		bool bExtractGreenAlpha = (! bDX8 ) && ( params[EXTRACTGREENALPHA]->GetIntValue() != 0 );
		bool bBlendFrames = (! bDX8 ) && ( params[BLENDFRAMES]->GetIntValue() != 0 );
		if ( nSplineType )
		{
			bBlendFrames = false;
		}
		bool bAddSelf = params[ADDSELF]->GetFloatValue() != 0.0;
		bool bUseInstancing = IsX360() ? ( params[ USEINSTANCING ]->GetIntValue() != 0 ) : false;
		if ( nSplineType )
			bUseInstancing = false;

		// draw back-facing because of yaw spin
		pShaderShadow->EnableCulling( false );

		// Be sure not to write to dest alpha
		pShaderShadow->EnableAlphaWrites( false );

		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		if ( bDX8 )
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

		if ( bAdditive2ndTexture && bDX8 )
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );

		if ( bUseRampTexture )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, true );
		}

		if ( bDepthBlend )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
		}

		if ( bAdditive2ndTexture || bAddSelf )
			pShaderShadow->EnableAlphaTest( false );
		else
			pShaderShadow->EnableAlphaTest( true );

		pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GREATER, 0.01f );

		if ( bAdditive2ndTexture || bAddOverBlend || bAddSelf )
		{
			EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
		}
		else
		{
			if ( IS_FLAG_SET(MATERIAL_VAR_ADDITIVE) )
			{
				EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
			}
			else
			{
				EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}
		}

		unsigned int flags = VERTEX_POSITION | VERTEX_COLOR;
		static int s_TexCoordSize[8]={4,				// 0 = sheet bounding uvs, frame0
			4,				// 1 = sheet bounding uvs, frame 1
			4,				// 2 = frame blend, rot, radius, ???
			2,				// 3 = corner identifier ( 0/0,1/0,1/1, 1/0 )
			4,				// 4 = texture 2 bounding uvs
			4,				// 5 = second sequence bounding uvs, frame0
			4,				// 6 = second sequence bounding uvs, frame1
			4,				// 7 = second sequence frame blend, ?,?,?
		};
		static int s_TexCoordSizeSpline[]={4,				// 0 = sheet bounding uvs, frame0
			4,				// 1 = sheet bounding uvs, frame 1
			4,				// 2 = frame blend, rot, radius, ???
			4,				// 3 = corner identifier ( 0/0,1/0,1/1, 1/0 )
			4,				// 4 = texture 2 bounding uvs
			4,				// 5 = second sequence bounding uvs, frame0
			4,				// 6 = second sequence bounding uvs, frame1
			4,				// 7 = second sequence frame blend, ?,?,?
		};

		int numTexCoords = 4;
		if ( true /* bAdditive2ndTexture */ ) // there is no branch for 2nd texture in the VS! -henryg
		{
			numTexCoords = 5;
		}
		if ( bSecondSequence )
		{
			// the whole shebang - 2 sequences, with a possible multi-image sequence first
			numTexCoords = 8;
		}
		pShaderShadow->VertexShaderVertexFormat( flags,
			numTexCoords, 
			nSplineType? s_TexCoordSizeSpline : s_TexCoordSize, 0 );

		if ( bDX8 )
		{
#if SUPPORT_DX8
			if ( nSplineType )
			{
				DECLARE_STATIC_VERTEX_SHADER( splinecard_vs11 );
				SET_STATIC_VERTEX_SHADER( splinecard_vs11 );
			}
			else
			{
				DECLARE_STATIC_VERTEX_SHADER( spritecard_vs11 );
				if ( bSecondSequence )
					bAdditive2ndTexture = false;
				SET_STATIC_VERTEX_SHADER_COMBO( DUALSEQUENCE, false );
				SET_STATIC_VERTEX_SHADER_COMBO( ZOOM_ANIMATE_SEQ2, false );
				SET_STATIC_VERTEX_SHADER_COMBO( EXTRACTGREENALPHA, bExtractGreenAlpha );
				SET_STATIC_VERTEX_SHADER( spritecard_vs11 );
			}

			DECLARE_STATIC_PIXEL_SHADER( spritecard_ps11 );
			SET_STATIC_PIXEL_SHADER_COMBO( ADDBASETEXTURE2, bAdditive2ndTexture );
			SET_STATIC_PIXEL_SHADER_COMBO( ADDSELF, bAddSelf );
			SET_STATIC_PIXEL_SHADER_COMBO( USEALPHAASRGB, bSecondSequence );
			SET_STATIC_PIXEL_SHADER( spritecard_ps11 );
#endif
		}
		else
		{
			if ( nSplineType )
			{
				DECLARE_STATIC_VERTEX_SHADER( splinecard_vs20 );
				SET_STATIC_VERTEX_SHADER( splinecard_vs20 );
			}
			else
			{
				DECLARE_STATIC_VERTEX_SHADER( spritecard_vs20 );
				SET_STATIC_VERTEX_SHADER_COMBO( DUALSEQUENCE, bSecondSequence );
				SET_STATIC_VERTEX_SHADER_COMBO( ZOOM_ANIMATE_SEQ2, bZoomSeq2 );
				SET_STATIC_VERTEX_SHADER_COMBO( EXTRACTGREENALPHA, bExtractGreenAlpha );
				SET_STATIC_VERTEX_SHADER_COMBO( USE_INSTANCING, bUseInstancing );
				SET_STATIC_VERTEX_SHADER( spritecard_vs20 );
			}

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( spritecard_ps20b );
				SET_STATIC_PIXEL_SHADER_COMBO( ADDBASETEXTURE2, bAdditive2ndTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( ADDSELF, bAddSelf );
				SET_STATIC_PIXEL_SHADER_COMBO( ANIMBLEND, bBlendFrames );
				SET_STATIC_PIXEL_SHADER_COMBO( DUALSEQUENCE, bSecondSequence );
				SET_STATIC_PIXEL_SHADER_COMBO( SEQUENCE_BLEND_MODE, bSecondSequence ? params[SEQUENCE_BLEND_MODE]->GetIntValue() : 0 );
				SET_STATIC_PIXEL_SHADER_COMBO( MAXLUMFRAMEBLEND1, params[MAXLUMFRAMEBLEND1]->GetIntValue() );
				SET_STATIC_PIXEL_SHADER_COMBO( MAXLUMFRAMEBLEND2, bSecondSequence? params[MAXLUMFRAMEBLEND1]->GetIntValue() : 0 );
				SET_STATIC_PIXEL_SHADER_COMBO( COLORRAMP, bUseRampTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( EXTRACTGREENALPHA, bExtractGreenAlpha );
				SET_STATIC_PIXEL_SHADER_COMBO( DEPTHBLEND, bDepthBlend );
				SET_STATIC_PIXEL_SHADER( spritecard_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( spritecard_ps20 );
				SET_STATIC_PIXEL_SHADER_COMBO( ADDBASETEXTURE2, bAdditive2ndTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( DUALSEQUENCE, bSecondSequence );
				SET_STATIC_PIXEL_SHADER_COMBO( ADDSELF, bAddSelf );
				SET_STATIC_PIXEL_SHADER_COMBO( ANIMBLEND, bBlendFrames );
				SET_STATIC_PIXEL_SHADER_COMBO( SEQUENCE_BLEND_MODE, bSecondSequence ? params[SEQUENCE_BLEND_MODE]->GetIntValue() : 0 );
				SET_STATIC_PIXEL_SHADER_COMBO( MAXLUMFRAMEBLEND1, params[MAXLUMFRAMEBLEND1]->GetIntValue() );
				SET_STATIC_PIXEL_SHADER_COMBO( MAXLUMFRAMEBLEND2, bSecondSequence? params[MAXLUMFRAMEBLEND1]->GetIntValue() : 0 );
				SET_STATIC_PIXEL_SHADER_COMBO( COLORRAMP, bUseRampTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( EXTRACTGREENALPHA, bExtractGreenAlpha );
				SET_STATIC_PIXEL_SHADER( spritecard_ps20 );
			}

			if ( !bDX8 )
				pShaderShadow->EnableSRGBWrite( true );

			if( !bExtractGreenAlpha && !bDX8 )
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );
		}
	}
	DYNAMIC_STATE
	{
		BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

		if ( bDX8 )										// bind on 2nd sampelr so we can lerp
			BindTexture( SHADER_SAMPLER1, BASETEXTURE, FRAME );

		if ( bDX8 && bAdditive2ndTexture )
			BindTexture( SHADER_SAMPLER3, BASETEXTURE, FRAME );

		if ( bUseRampTexture && ( !bDX8 ) )
		{
			BindTexture( SHADER_SAMPLER1, RAMPTEXTURE, FRAME );
		}

		if ( bDepthBlend )
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER2, TEXTURE_FRAME_BUFFER_FULL_DEPTH );
		}

		LoadViewportTransformScaledIntoVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_10 );

		int nOrientation = params[ORIENTATION]->GetIntValue();
		nOrientation = clamp( nOrientation, 0, 2 );

		// We need these only when screen-orienting
		if ( nOrientation == 0 )
		{
			LoadModelViewMatrixIntoVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0 );
			LoadProjectionMatrixIntoVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_3 );
		}

		if ( bZoomSeq2 )
		{
			float flZScale=1.0/(params[ZOOMANIMATESEQ2]->GetFloatValue());
			float C0[4]={ 0.5*(1.0+flZScale), flZScale, 0, 0 };
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_7, C0,
				ARRAYSIZE(C0)/4 );
		}

		// set fade constants in vsconsts 8 and 9
		float flMaxDistance = params[MAXDISTANCE]->GetFloatValue();
		float flStartFade = max( 1.0, flMaxDistance - params[FARFADEINTERVAL]->GetFloatValue() );

		float VC0[8]={ params[MINSIZE]->GetFloatValue(), params[MAXSIZE]->GetFloatValue(),
			params[STARTFADESIZE]->GetFloatValue(), params[ENDFADESIZE]->GetFloatValue(),
			flStartFade, 1.0/(flMaxDistance-flStartFade),
			0,0 };

		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_8, VC0, ARRAYSIZE(VC0)/4 );

		pShaderAPI->SetDepthFeatheringPixelShaderConstant( 2, params[DEPTHBLENDSCALE]->GetFloatValue() );

		float C0[4]={ params[ADDBASETEXTURE2]->GetFloatValue(),
			params[OVERBRIGHTFACTOR]->GetFloatValue(),
			params[ADDSELF]->GetFloatValue(),
			0.0f };

		if ( bDX8 && ( !bAdditive2ndTexture ) )	// deal with 0..1 limit for pix shader constants
		{
			C0[2] *= 0.25;
			C0[1] *= 0.25;
		}

		pShaderAPI->SetPixelShaderConstant( 0, C0, ARRAYSIZE(C0)/4 );

		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
#if SUPPORT_DX8
			if ( nSplineType )
			{
				DECLARE_DYNAMIC_VERTEX_SHADER( splinecard_vs11 );
				SET_DYNAMIC_VERTEX_SHADER( splinecard_vs11 );
			}
			else
			{
				DECLARE_DYNAMIC_VERTEX_SHADER( spritecard_vs11 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( ORIENTATION, nOrientation );
				SET_DYNAMIC_VERTEX_SHADER( spritecard_vs11 );
			}
#endif
		}
		else
		{
			if ( nSplineType )
			{
				DECLARE_DYNAMIC_VERTEX_SHADER( splinecard_vs20 );
				SET_DYNAMIC_VERTEX_SHADER( splinecard_vs20 );
			}
			else
			{
				DECLARE_DYNAMIC_VERTEX_SHADER( spritecard_vs20 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( ORIENTATION, nOrientation );
				SET_DYNAMIC_VERTEX_SHADER( spritecard_vs20 );
			}
		}
	}
	Draw( );
}
END_SHADER
