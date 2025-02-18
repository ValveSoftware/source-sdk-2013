//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "BaseVSShader.h"
#include <string.h>
#include "const.h"

#include "cpp_shader_constant_register_map.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#include "sprite_vs20.inc"
#include "sprite_ps20.inc"
#include "sprite_ps20b.inc"

// WARNING!  Change these in engine/SpriteGn.h if you change them here!
#define SPR_VP_PARALLEL_UPRIGHT		0
#define SPR_FACING_UPRIGHT			1
#define SPR_VP_PARALLEL				2
#define SPR_ORIENTED				3
#define SPR_VP_PARALLEL_ORIENTED	4


DEFINE_FALLBACK_SHADER( Sprite, Sprite_DX9 )

BEGIN_VS_SHADER( Sprite_DX9, 
			  "Help for Sprite_DX9" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( SPRITEORIGIN, SHADER_PARAM_TYPE_VEC3, "[0 0 0]", "sprite origin" )
		SHADER_PARAM( SPRITEORIENTATION, SHADER_PARAM_TYPE_INTEGER, "0", "sprite orientation" )
		SHADER_PARAM( SPRITERENDERMODE, SHADER_PARAM_TYPE_INTEGER, "0", "sprite rendermode" )
		SHADER_PARAM( IGNOREVERTEXCOLORS, SHADER_PARAM_TYPE_BOOL, "1", "ignore vertex colors" )
		SHADER_PARAM( NOSRGB, SHADER_PARAM_TYPE_BOOL, "0", "do not operate in srgb space" )
		SHADER_PARAM( HDRCOLORSCALE, SHADER_PARAM_TYPE_FLOAT, "1.0", "hdr color scale" )
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if (g_pHardwareConfig->GetDXSupportLevel() < 90)
			return "Sprite_DX8";
		return 0;
	}
	SHADER_INIT_PARAMS()
	{
		// FIXME: This can share code with sprite.cpp
		if (!params[ALPHA]->IsDefined())
		{
			params[ALPHA]->SetFloatValue( 1.0f );
		}

		if (!params[HDRCOLORSCALE]->IsDefined())
		{
			params[HDRCOLORSCALE]->SetFloatValue( 1.0f );
		}

		if ( !params[NOSRGB]->IsDefined() )
		{
			// Disable sRGB reads and writes by default
			params[NOSRGB]->SetIntValue( 1 );
		}

		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		SET_FLAGS( MATERIAL_VAR_VERTEXCOLOR );
		SET_FLAGS( MATERIAL_VAR_VERTEXALPHA );

		// translate from a string orientation to an enumeration
		if (params[SPRITEORIENTATION]->IsDefined())
		{
			const char *orientationString = params[SPRITEORIENTATION]->GetStringValue();
			if( stricmp( orientationString, "parallel_upright" ) == 0 )
			{
				params[SPRITEORIENTATION]->SetIntValue( SPR_VP_PARALLEL_UPRIGHT );
			}
			else if( stricmp( orientationString, "facing_upright" ) == 0 )
			{
				params[SPRITEORIENTATION]->SetIntValue( SPR_FACING_UPRIGHT );
			}
			else if( stricmp( orientationString, "vp_parallel" ) == 0 )
			{
				params[SPRITEORIENTATION]->SetIntValue( SPR_VP_PARALLEL );
			}
			else if( stricmp( orientationString, "oriented" ) == 0 )
			{
				params[SPRITEORIENTATION]->SetIntValue( SPR_ORIENTED );
			}
			else if( stricmp( orientationString, "vp_parallel_oriented" ) == 0 )
			{
				params[SPRITEORIENTATION]->SetIntValue( SPR_VP_PARALLEL_ORIENTED );
			}
			else
			{
				Warning( "error with $spriteOrientation\n" );
				params[SPRITEORIENTATION]->SetIntValue( SPR_VP_PARALLEL_UPRIGHT );
			}
		}
		else
		{
			// default case
			params[SPRITEORIENTATION]->SetIntValue( SPR_VP_PARALLEL_UPRIGHT );
		}
	}

	SHADER_INIT
	{
		bool bSRGB = s_ppParams[NOSRGB]->GetIntValue() == 0;
		LoadTexture( BASETEXTURE, bSRGB ? TEXTUREFLAGS_SRGB : 0 );
	}

#define SHADER_USE_VERTEX_COLOR		1
#define SHADER_USE_CONSTANT_COLOR	2

	void SetSpriteCommonShadowState( unsigned int shaderFlags )
	{
		IShaderShadow *pShaderShadow = s_pShaderShadow;
		s_pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		bool bSRGB = s_ppParams[NOSRGB]->GetIntValue() == 0;
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, bSRGB );

		// Only enabling this on OSX() - it causes GL mode's light glow sprites to be much darker vs. D3D9 under Linux/Win GL.
		bool bSRGBOutputAdapter = ( IsOSX() && !g_pHardwareConfig->FakeSRGBWrite() ) && !bSRGB;

		unsigned int flags = VERTEX_POSITION;
		if( shaderFlags & SHADER_USE_VERTEX_COLOR )
		{
			flags |= VERTEX_COLOR;
		}
		int numTexCoords = 1;
		s_pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, 0, 0 );

		DECLARE_STATIC_VERTEX_SHADER( sprite_vs20 );
		SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR,  ( shaderFlags & SHADER_USE_VERTEX_COLOR ) ? true : false );
		SET_STATIC_VERTEX_SHADER_COMBO( SRGB,  bSRGB );
		SET_STATIC_VERTEX_SHADER( sprite_vs20 );

		if( g_pHardwareConfig->SupportsPixelShaders_2_b() || g_pHardwareConfig->ShouldAlwaysUseShaderModel2bShaders() ) // Always send GL down this path
		{
			DECLARE_STATIC_PIXEL_SHADER( sprite_ps20b );
			SET_STATIC_PIXEL_SHADER_COMBO( VERTEXCOLOR,  ( shaderFlags &  SHADER_USE_VERTEX_COLOR ) ? true : false );
			SET_STATIC_PIXEL_SHADER_COMBO( CONSTANTCOLOR,  ( shaderFlags & SHADER_USE_CONSTANT_COLOR ) ? true : false );
			SET_STATIC_PIXEL_SHADER_COMBO( HDRTYPE,  g_pHardwareConfig->GetHDRType() );
			SET_STATIC_PIXEL_SHADER_COMBO( SRGB, bSRGB );
			SET_STATIC_PIXEL_SHADER_COMBO( SRGB_OUTPUT_ADAPTER, bSRGBOutputAdapter );
			SET_STATIC_PIXEL_SHADER( sprite_ps20b );
		}
		else
		{
			DECLARE_STATIC_PIXEL_SHADER( sprite_ps20 );
			SET_STATIC_PIXEL_SHADER_COMBO( VERTEXCOLOR,  ( shaderFlags &  SHADER_USE_VERTEX_COLOR ) ? true : false );
			SET_STATIC_PIXEL_SHADER_COMBO( CONSTANTCOLOR,  ( shaderFlags & SHADER_USE_CONSTANT_COLOR ) ? true : false );
			SET_STATIC_PIXEL_SHADER_COMBO( HDRTYPE,  g_pHardwareConfig->GetHDRType() );
			SET_STATIC_PIXEL_SHADER_COMBO( SRGB, bSRGB );
			SET_STATIC_PIXEL_SHADER( sprite_ps20 );
		}

		// OSX always has to sRGB write (don't do this on Linux/Win GL - it causes glow sprites to be way too dark)
		s_pShaderShadow->EnableSRGBWrite( bSRGB || ( IsOSX() && !g_pHardwareConfig->FakeSRGBWrite() ) );
	}

	void SetSpriteCommonDynamicState( unsigned int shaderFlags )
	{
		IShaderDynamicAPI *pShaderAPI = s_pShaderAPI;
		bool bSRGB = s_ppParams[NOSRGB]->GetIntValue() == 0;

		BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

		MaterialFogMode_t fogType = s_pShaderAPI->GetSceneFogMode();
		int fogIndex = ( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) ? 1 : 0;
		DECLARE_DYNAMIC_VERTEX_SHADER( sprite_vs20 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG,  fogIndex );
		SET_DYNAMIC_VERTEX_SHADER( sprite_vs20 );

		if( g_pHardwareConfig->SupportsPixelShaders_2_b() || g_pHardwareConfig->ShouldAlwaysUseShaderModel2bShaders() ) // Always send GL down this path
		{
			DECLARE_DYNAMIC_PIXEL_SHADER( sprite_ps20b );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( HDRENABLED, IsHDREnabled() );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
			SET_DYNAMIC_PIXEL_SHADER( sprite_ps20b );
		}
		else
		{
			DECLARE_DYNAMIC_PIXEL_SHADER( sprite_ps20 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( HDRENABLED, IsHDREnabled() );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
			SET_DYNAMIC_PIXEL_SHADER( sprite_ps20 );
		}

		pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

		float vEyePos_SpecExponent[4];
		pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );
		vEyePos_SpecExponent[3] = 0.0f;
		pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );

		if( shaderFlags & SHADER_USE_CONSTANT_COLOR )
		{
			if ( bSRGB )
				SetPixelShaderConstantGammaToLinear( 0, COLOR, ALPHA );
			else
				SetPixelShaderConstant( 0, COLOR, ALPHA );
		}

		if( IsHDREnabled() )
		{
			if ( bSRGB )
				SetPixelShaderConstantGammaToLinear( 1, HDRCOLORSCALE );
			else
				SetPixelShaderConstant( 1, HDRCOLORSCALE );
		}
	}

	SHADER_DRAW
	{
		bool bSRGB = params[NOSRGB]->GetIntValue() == 0;

		SHADOW_STATE
		{
			pShaderShadow->EnableCulling( false );
		}

		switch( params[SPRITERENDERMODE]->GetIntValue() )
		{
		case kRenderNormal:
			SHADOW_STATE
			{
				FogToFogColor();

				SetSpriteCommonShadowState( 0 );
			}
			DYNAMIC_STATE
			{
				SetSpriteCommonDynamicState( 0 );
			}
			Draw();
			break;
		case kRenderTransColor:
		case kRenderTransTexture:
			SHADOW_STATE
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				
				FogToFogColor();

				SetSpriteCommonShadowState( SHADER_USE_VERTEX_COLOR );
			}
			DYNAMIC_STATE
			{
				SetSpriteCommonDynamicState( SHADER_USE_VERTEX_COLOR );
			}
			Draw();
			break;
		case kRenderGlow:
		case kRenderWorldGlow:
			SHADOW_STATE
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableDepthTest( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
				
				FogToBlack();

				SetSpriteCommonShadowState( SHADER_USE_VERTEX_COLOR );
			}
			DYNAMIC_STATE
			{
				SetSpriteCommonDynamicState( SHADER_USE_VERTEX_COLOR );
			}
			Draw();
			break;
		case kRenderTransAlpha:
			// untested cut and past from kRenderTransAlphaAdd  . . same as first pass of that.
			SHADOW_STATE
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				
				FogToFogColor();

				SetSpriteCommonShadowState( SHADER_USE_VERTEX_COLOR );
			}
			DYNAMIC_STATE
			{
				SetSpriteCommonDynamicState( SHADER_USE_VERTEX_COLOR );
			}
			Draw();
			break;
		case kRenderTransAlphaAdd:
			SHADOW_STATE
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				
				FogToFogColor();

				SetSpriteCommonShadowState( SHADER_USE_VERTEX_COLOR );
			}
			DYNAMIC_STATE
			{
				SetSpriteCommonDynamicState( SHADER_USE_VERTEX_COLOR );
			}
			Draw();

			SHADOW_STATE
			{
				SetInitialShadowState();
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_ONE_MINUS_SRC_ALPHA, SHADER_BLEND_ONE );
				
				FogToBlack();

				SetSpriteCommonShadowState( SHADER_USE_VERTEX_COLOR );
			}
			DYNAMIC_STATE
			{
				SetSpriteCommonDynamicState( SHADER_USE_VERTEX_COLOR );
			}
			Draw();
			break;

		case kRenderTransAdd:
			{
				unsigned int flags = SHADER_USE_CONSTANT_COLOR;
				if( !params[ IGNOREVERTEXCOLORS ]->GetIntValue() )
				{
					flags |= SHADER_USE_VERTEX_COLOR;
				}
				SHADOW_STATE
				{
					pShaderShadow->EnableDepthWrites( false );
					pShaderShadow->EnableBlending( true );
					pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
					
					FogToBlack();

					SetSpriteCommonShadowState( flags );
				}
				DYNAMIC_STATE
				{
					SetSpriteCommonDynamicState( flags );
				}
			}
			Draw();
			break;
		case kRenderTransAddFrameBlend:
			{
				float flFrame = params[FRAME]->GetFloatValue();
				float flFade = params[ALPHA]->GetFloatValue();
				unsigned int flags = SHADER_USE_CONSTANT_COLOR;
				if( !params[ IGNOREVERTEXCOLORS ]->GetIntValue() )
				{
					flags |= SHADER_USE_VERTEX_COLOR;
				}
				SHADOW_STATE
				{
					pShaderShadow->EnableDepthWrites( false );
					pShaderShadow->EnableBlending( true );
					pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
					
					FogToBlack();

					SetSpriteCommonShadowState( flags );
				}
				DYNAMIC_STATE
				{
					float frameBlendAlpha = 1.0f - ( flFrame - ( int )flFrame );
					ITexture *pTexture = params[BASETEXTURE]->GetTextureValue();
					BindTexture( SHADER_SAMPLER0, pTexture, ( int )flFrame );

					MaterialFogMode_t fogType = s_pShaderAPI->GetSceneFogMode();
					int fogIndex = ( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) ? 1 : 0;
					DECLARE_DYNAMIC_VERTEX_SHADER( sprite_vs20 );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG,  fogIndex );
					SET_DYNAMIC_VERTEX_SHADER( sprite_vs20 );

					if( g_pHardwareConfig->SupportsPixelShaders_2_b() || g_pHardwareConfig->ShouldAlwaysUseShaderModel2bShaders() ) // Always send GL down this path
					{
						DECLARE_DYNAMIC_PIXEL_SHADER( sprite_ps20b );
						SET_DYNAMIC_PIXEL_SHADER_COMBO( HDRENABLED,  IsHDREnabled() );
						SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
						SET_DYNAMIC_PIXEL_SHADER( sprite_ps20b );
					}
					else
					{
						DECLARE_DYNAMIC_PIXEL_SHADER( sprite_ps20 );
						SET_DYNAMIC_PIXEL_SHADER_COMBO( HDRENABLED,  IsHDREnabled() );
						SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
						SET_DYNAMIC_PIXEL_SHADER( sprite_ps20 );
					}

					pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

					float vEyePos_SpecExponent[4];
					pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );
					vEyePos_SpecExponent[3] = 0.0f;
					pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );

					float color[4];
					if ( bSRGB )
						color[0] = color[1] = color[2] = GammaToLinear( flFade * frameBlendAlpha );
					else
						color[0] = color[1] = color[2] = flFade * frameBlendAlpha;
					color[3] = 1.0f;
					s_pShaderAPI->SetPixelShaderConstant( 0, color );
					if( IsHDREnabled() )
					{
						if ( bSRGB )
							SetPixelShaderConstantGammaToLinear( 1, HDRCOLORSCALE );
						else
							SetPixelShaderConstant( 1, HDRCOLORSCALE );
					}
				}
				Draw();
				SHADOW_STATE
				{
					FogToBlack();

					SetSpriteCommonShadowState( flags );
				}
				DYNAMIC_STATE
				{
					float frameBlendAlpha = ( flFrame - ( int )flFrame );
					ITexture *pTexture = params[BASETEXTURE]->GetTextureValue();
					int numAnimationFrames = pTexture->GetNumAnimationFrames();
					BindTexture( SHADER_SAMPLER0, pTexture, ( ( int )flFrame + 1 ) % numAnimationFrames );

					MaterialFogMode_t fogType = s_pShaderAPI->GetSceneFogMode();
					int fogIndex = ( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) ? 1 : 0;
					DECLARE_DYNAMIC_VERTEX_SHADER( sprite_vs20 );
					SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG,  fogIndex );
					SET_DYNAMIC_VERTEX_SHADER( sprite_vs20 );

					if( g_pHardwareConfig->SupportsPixelShaders_2_b() || g_pHardwareConfig->ShouldAlwaysUseShaderModel2bShaders() ) // Always send GL down this path
					{
						DECLARE_DYNAMIC_PIXEL_SHADER( sprite_ps20b );
						SET_DYNAMIC_PIXEL_SHADER_COMBO( HDRENABLED,  IsHDREnabled() );
						SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
						SET_DYNAMIC_PIXEL_SHADER( sprite_ps20b );
					}
					else
					{
						DECLARE_DYNAMIC_PIXEL_SHADER( sprite_ps20 );
						SET_DYNAMIC_PIXEL_SHADER_COMBO( HDRENABLED,  IsHDREnabled() );
						SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
						SET_DYNAMIC_PIXEL_SHADER( sprite_ps20 );
					}

					pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

					float vEyePos_SpecExponent[4];
					pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );
					vEyePos_SpecExponent[3] = 0.0f;
					pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );

					float color[4];
					if ( bSRGB )
						color[0] = color[1] = color[2] = GammaToLinear( flFade * frameBlendAlpha );
					else
						color[0] = color[1] = color[2] = flFade * frameBlendAlpha;
					color[3] = 1.0f;
					s_pShaderAPI->SetPixelShaderConstant( 0, color );
					if( IsHDREnabled() )
					{
						if ( bSRGB )
							SetPixelShaderConstantGammaToLinear( 1, HDRCOLORSCALE );
						else
							SetPixelShaderConstant( 1, HDRCOLORSCALE );
					}
				}
				Draw();
			}

			break;
		default:
			ShaderWarning( "shader Sprite: Unknown sprite render mode\n" );
			break;
		}
	}
END_SHADER
