//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Depth of field material
//
//===========================================================================//

#include "BaseVSShader.h"
#include "depth_of_field_vs20.inc"
#include "depth_of_field_ps20b.inc"
#include "convar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar mat_dof_max_blur_radius( "mat_dof_max_blur_radius", "50" );
ConVar mat_dof_quality( "mat_dof_quality", "3" );
ConVar mat_dof_constant( "mat_dof_constant", "512" );

// 8 samples
static const float s_flPoissonConstsQuality0[16] = {
	0.0, 0.0,
	0.527837, -0.085868,
	-0.040088, 0.536087,
	-0.670445, -0.179949,
	-0.419418, -0.616039,
	0.440453, -0.639399,
	-0.757088, 0.349334,
	0.574619, 0.685879
};

// 16 samples
static const float s_flPoissonConstsQuality1[32] = {
	0.0747,		-0.8341,
	-0.9138,	0.3251,
	0.8667,		-0.3029,
	-0.4642,	0.2187,
	-0.1505,	0.7320,
	0.7310,		-0.6786,
	0.2859,		-0.3254,
	-0.1311,	-0.2292,
	0.3518,		0.6470,
	-0.7485,	-0.6307,
	0.1687,		0.1873,
	-0.3604,	-0.7483,
	-0.5658,	-0.1521,
	0.7102,		0.0536,
	-0.6056,	0.7747,
	0.7793,		0.6194
};

// 32 samples
static const float s_flPoissonConstsQuality2[64] = {
	0.0854f, -0.0644f,
	0.8744f, 0.1665f,
	0.2329f, 0.3995f,
	-0.7804f, 0.5482f,
	-0.4577f, 0.7647f,
	-0.1936f, 0.5564f,
	0.4205f, -0.5768f,
	-0.0304f, -0.9050f,
	-0.5215f, 0.1854f,
	0.3161f, -0.2954f,
	0.0666f, -0.5564f,
	-0.2137f, -0.0072f,
	-0.4112f, -0.3311f,
	0.6438f, -0.2484f,
	-0.9055f, -0.0360f,
	0.8323f, 0.5268f,
	0.5592f, 0.3459f,
	-0.6797f, -0.5201f,
	-0.4325f, -0.8857f,
	0.8768f, -0.4197f,
	0.3090f, -0.8646f,
	0.5034f, 0.8603f,
	0.3752f, 0.0627f,
	-0.0161f, 0.2627f,
	0.0969f, 0.7054f,
	-0.2291f, -0.6595f,
	-0.5887f, -0.1100f,
	0.7048f, -0.6528f,
	-0.8438f, 0.2706f,
	-0.5061f, 0.4653f,
	-0.1245f, -0.3302f,
	-0.1801f, 0.8486f
};

DEFINE_FALLBACK_SHADER( DepthOfField, DepthOfField_dx9 )
BEGIN_VS_SHADER_FLAGS( DepthOfField_dx9, "Depth of Field", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( SMALLFB, SHADER_PARAM_TYPE_TEXTURE, "_rt_SmallFB1", "Downsampled backbuffer" )
		SHADER_PARAM( NEARPLANE, SHADER_PARAM_TYPE_FLOAT, "0", "Near plane depth" )
		SHADER_PARAM( FARPLANE,  SHADER_PARAM_TYPE_FLOAT, "0", "Far plane depth" )
		SHADER_PARAM( NEARBLURDEPTH,  SHADER_PARAM_TYPE_FLOAT, "0", "Near blur plane depth" )
		SHADER_PARAM( NEARFOCUSDEPTH,  SHADER_PARAM_TYPE_FLOAT, "0", "Near focus plane depth" )
		SHADER_PARAM( FARFOCUSDEPTH,  SHADER_PARAM_TYPE_FLOAT, "0", "Far focus plane depth" )
		SHADER_PARAM( FARBLURDEPTH,  SHADER_PARAM_TYPE_FLOAT, "0", "Far blur plane depth" )
		SHADER_PARAM( NEARBLURRADIUS,  SHADER_PARAM_TYPE_FLOAT, "0", "Max near blur radius" )
		SHADER_PARAM( FARBLURRADIUS,  SHADER_PARAM_TYPE_FLOAT, "0", "Max far blur radius" )
		SHADER_PARAM( QUALITY, SHADER_PARAM_TYPE_INTEGER, "0", "Quality level. Selects different algorithms." )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_PARAM_STRING_IF_NOT_DEFINED( SMALLFB, "_rt_SmallFB1" );
		SET_PARAM_FLOAT_IF_NOT_DEFINED( NEARPLANE, 0.0f );
		SET_PARAM_FLOAT_IF_NOT_DEFINED( FARPLANE, 0.0f );
		SET_PARAM_FLOAT_IF_NOT_DEFINED( NEARBLURDEPTH, 0.0f );
		SET_PARAM_FLOAT_IF_NOT_DEFINED( NEARFOCUSDEPTH, 0.0f );
		SET_PARAM_FLOAT_IF_NOT_DEFINED( FARFOCUSDEPTH, 0.0f );
		SET_PARAM_FLOAT_IF_NOT_DEFINED( FARBLURDEPTH, 0.0f );
		SET_PARAM_FLOAT_IF_NOT_DEFINED( NEARBLURRADIUS, 0.0f );
		SET_PARAM_FLOAT_IF_NOT_DEFINED( FARBLURRADIUS, 0.0f );
		SET_PARAM_INT_IF_NOT_DEFINED( QUALITY, 0 );
	}

	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 92 )
		{
			return "Wireframe";
		}

		return 0;
	}

	SHADER_INIT
	{
		if ( params[BASETEXTURE]->IsDefined() )
		{
			LoadTexture( BASETEXTURE );
		}
		if ( params[SMALLFB]->IsDefined() )
		{
			LoadTexture( SMALLFB );
		}
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, 0, 0 );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, false );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, false );
			pShaderShadow->EnableSRGBWrite( false );

			DECLARE_STATIC_VERTEX_SHADER( depth_of_field_vs20 );
			SET_STATIC_VERTEX_SHADER( depth_of_field_vs20 );

			if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( depth_of_field_ps20b );
				SET_STATIC_PIXEL_SHADER( depth_of_field_ps20b );
			}
			else
			{
				Assert( !"No ps_2_b. This shouldn't be happening" );
			}

			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableAlphaWrites( false );
		}

		DYNAMIC_STATE
		{
			DECLARE_DYNAMIC_VERTEX_SHADER( depth_of_field_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( depth_of_field_vs20 );

			// Bind textures
			BindTexture( SHADER_SAMPLER0, BASETEXTURE );
			BindTexture( SHADER_SAMPLER1, SMALLFB );

			// near blur = blur of stuff in front of focus range
			// far blur = blur of stuff behind focus range

			// C0: set near/far blur and focus distances
			// x = near blur distance
			// y = near focus distance
			// z = far focus distance
			// w = far blur distance
			// C1:
			// x = blur radius for near blur (in pixels)
			// y = blur radius for far blur (in pixels)
			// TODO: Specifying this stuff in pixels makes blurs look smaller on high backbuffer resolutions.
			// This might be a problem for tweaking these values.
			float vConst[16] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

			vConst[0] = params[NEARBLURDEPTH]->GetFloatValue();
			vConst[1] = params[NEARFOCUSDEPTH]->GetFloatValue();
			vConst[2] = params[FARFOCUSDEPTH]->GetFloatValue();
			vConst[3] = params[FARBLURDEPTH]->GetFloatValue();;
			// max blur radius will need to be set based on quality level and screen res
			vConst[4] = mat_dof_max_blur_radius.GetFloat();
			vConst[5] = MIN( params[NEARBLURRADIUS]->GetFloatValue(), vConst[4] ) / vConst[4];	// near and far blur radius as fraction of max radius
			vConst[6] = MIN( params[FARBLURRADIUS]->GetFloatValue(), vConst[4] ) / vConst[4];

			vConst[8] = params[NEARPLANE]->GetFloatValue();
			vConst[9] = params[FARPLANE]->GetFloatValue();
			
			vConst[10] = mat_dof_constant.GetFloat() * ( vConst[9] - vConst[8] ) / vConst[9]; 

			vConst[12] = vConst[10] / ( vConst[0] - vConst[1] );
			vConst[13] = ( vConst[8] - vConst[1] ) / ( vConst[0] - vConst[1] );
			vConst[14] = vConst[10] / ( vConst[3] - vConst[2] );
			vConst[15] = ( vConst[8] - vConst[2] ) / ( vConst[3] - vConst[2] );

			pShaderAPI->SetPixelShaderConstant( 0, vConst, 4 );

			// set up poisson sample location constants pre-divided by screen res
			int nNumPoissonSamples = 0;
			const float *pPoissonSrc = NULL;
			switch ( params[QUALITY]->GetIntValue() )
			{
			case 0:
				// NOTE: These must match the shader
				nNumPoissonSamples = 8;
				pPoissonSrc = s_flPoissonConstsQuality0;
				break;

			case 1:
			case 2:
				nNumPoissonSamples = 16;
				pPoissonSrc = s_flPoissonConstsQuality1;
				break;

			case 3:
				nNumPoissonSamples = 32;
				pPoissonSrc = s_flPoissonConstsQuality2;
				break;

			default:
				Warning( "Invalid mat_dof_quality value. Resetting to 0.\n" );
				mat_dof_quality.SetValue( 0 );
				nNumPoissonSamples = 8;
				pPoissonSrc = s_flPoissonConstsQuality0;
				break;
			}

			float vPoissonConst[64];	// temp table

			// Get texture dimensions
			ITexture *pTex = params[BASETEXTURE]->GetTextureValue();
			Assert( pTex );
			float flInvTexWidth = 1.0f / static_cast<float>( pTex->GetActualWidth() );
			float flInvTexHeight = 1.0f / static_cast<float>( pTex->GetActualHeight() );

			for ( int i = 0; i < nNumPoissonSamples; i++ )
			{
				vPoissonConst[ 2*i ] = pPoissonSrc[ 2*i ] * flInvTexWidth;
				vPoissonConst[ 2*i+1 ] = pPoissonSrc[ 2*i+1 ] * flInvTexHeight;
			}

			// swizzle every other 2-tuple so that I can use the free .wz swizzle in the shader
			for ( int i = 1; i < nNumPoissonSamples; i += 2)
			{
				float t = vPoissonConst[ 2*i ];
				vPoissonConst[ 2*i ] = vPoissonConst[ 2*i+1 ];
				vPoissonConst[ 2*i+1 ] = t;
			}

			pShaderAPI->SetPixelShaderConstant( 4, vPoissonConst, nNumPoissonSamples / 2 );

			if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( depth_of_field_ps20b );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( QUALITY, params[QUALITY]->GetIntValue() );
				SET_DYNAMIC_PIXEL_SHADER( depth_of_field_ps20b );
			}
			else
			{
				Assert( !"No ps_2_b. This shouldn't be happening" );
			}
		}

		Draw();
	}
END_SHADER
