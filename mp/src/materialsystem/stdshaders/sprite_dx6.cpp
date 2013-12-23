//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// Implementation of the sprite shader
//=============================================================================//

#include "shaderlib/cshader.h"
#include <string.h>
#include "const.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// WARNING!  Change these in engine/SpriteGn.h if you change them here!
#define SPR_VP_PARALLEL_UPRIGHT		0
#define SPR_FACING_UPRIGHT			1
#define SPR_VP_PARALLEL				2
#define SPR_ORIENTED				3
#define SPR_VP_PARALLEL_ORIENTED	4


DEFINE_FALLBACK_SHADER( Sprite, Sprite_DX6 )

BEGIN_SHADER( Sprite_DX6, 
			  "Help for Sprite_DX6" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( SPRITEORIGIN, SHADER_PARAM_TYPE_VEC3, "[0 0 0]", "sprite origin" )
		SHADER_PARAM( SPRITEORIENTATION, SHADER_PARAM_TYPE_INTEGER, "0", "sprite orientation" )
		SHADER_PARAM( SPRITERENDERMODE, SHADER_PARAM_TYPE_INTEGER, "0", "sprite rendermode" )
		SHADER_PARAM( IGNOREVERTEXCOLORS, SHADER_PARAM_TYPE_BOOL, "1", "ignore vertex colors" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		// FIXME: This can share code with sprite.cpp
		// FIXME: Not sure if this is the best solution, but it's a very]
		// easy one. When graphics aren't enabled, we oftentimes need to get
		// at the parameters of a shader. Therefore, we must set the default
		// values in a separate phase from when we load resources.

		if (!params[ALPHA]->IsDefined())
			params[ ALPHA ]->SetFloatValue( 1.0f );

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
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableCulling( false );
		}

		switch( params[SPRITERENDERMODE]->GetIntValue() )
		{
		case kRenderNormal:
			SHADOW_STATE
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );
				FogToFogColor();
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			}
			Draw();
			break;
		case kRenderTransColor:
			SHADOW_STATE
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_COLOR );
				FogToFogColor();
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			}
			Draw();
			break;
		case kRenderTransTexture:
			SHADOW_STATE
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_COLOR );
				FogToFogColor();
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
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
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_COLOR );
				FogToBlack();
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			}
			Draw();
			break;
		case kRenderTransAlpha:
			SHADOW_STATE
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_COLOR );
				FogToFogColor();
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			}
			Draw();
			break;
		case kRenderTransAlphaAdd:
			SHADOW_STATE
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_COLOR );
				FogToFogColor();
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			}
			Draw();

			SHADOW_STATE
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_ONE_MINUS_SRC_ALPHA, SHADER_BLEND_ONE );
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_COLOR );
				FogToBlack();
			}
			DYNAMIC_STATE
			{
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			}
			Draw();
			break;

		case kRenderTransAdd:
			SHADOW_STATE
			{
				if( params[ IGNOREVERTEXCOLORS ]->GetIntValue() )
				{
					pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );
				}
				else
				{
					pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_COLOR );
				}
				pShaderShadow->EnableConstantColor( true );
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				FogToBlack();
			}
			DYNAMIC_STATE
			{
				SetColorState( COLOR, ALPHA );
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			}
			Draw();
			break;
		case kRenderTransAddFrameBlend:
			{
				float flFrame = params[FRAME]->GetFloatValue();
				float flFade = params[ALPHA]->GetFloatValue();
				SHADOW_STATE
				{
					if( params[ IGNOREVERTEXCOLORS ]->GetIntValue() )
					{
						pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );
					}
					else
					{
						pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_COLOR );
					}
					pShaderShadow->EnableConstantColor( true );
					pShaderShadow->EnableDepthWrites( false );
					pShaderShadow->EnableBlending( true );
					pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
					pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
					FogToBlack();
				}
				DYNAMIC_STATE
				{
					float frameBlendAlpha = 1.0f - ( flFrame - ( int )flFrame );
					pShaderAPI->Color3f( flFade * frameBlendAlpha, flFade * frameBlendAlpha, flFade * frameBlendAlpha );
					ITexture *pTexture = params[BASETEXTURE]->GetTextureValue();
					BindTexture( SHADER_SAMPLER0, pTexture, ( int )flFrame );
				}
				Draw();
				SHADOW_STATE
				{
					FogToBlack();
				}
				DYNAMIC_STATE
				{
					float frameBlendAlpha = ( flFrame - ( int )flFrame );
					pShaderAPI->Color3f( flFade * frameBlendAlpha, flFade * frameBlendAlpha, flFade * frameBlendAlpha );
					ITexture *pTexture = params[BASETEXTURE]->GetTextureValue();
					int numAnimationFrames = pTexture->GetNumAnimationFrames();
					BindTexture( SHADER_SAMPLER0, pTexture, ( ( int )flFrame + 1 ) % numAnimationFrames );
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
