//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// Implementation of the sprite shader
//=============================================================================//

#include "BaseVSShader.h"
#include <string.h>
#include "const.h"
#include "sprite_vs11.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// WARNING!  Change these in engine/SpriteGn.h if you change them here!
#define SPR_VP_PARALLEL_UPRIGHT		0
#define SPR_FACING_UPRIGHT			1
#define SPR_VP_PARALLEL				2
#define SPR_ORIENTED				3
#define SPR_VP_PARALLEL_ORIENTED	4


DEFINE_FALLBACK_SHADER( Sprite, Sprite_DX8 )

BEGIN_VS_SHADER( Sprite_DX8, 
			  "Help for Sprite_DX8" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( SPRITEORIGIN, SHADER_PARAM_TYPE_VEC3, "[0 0 0]", "sprite origin" )
		SHADER_PARAM( SPRITEORIENTATION, SHADER_PARAM_TYPE_INTEGER, "0", "sprite orientation" )
		SHADER_PARAM( SPRITERENDERMODE, SHADER_PARAM_TYPE_INTEGER, "0", "sprite rendermode" )
		SHADER_PARAM( IGNOREVERTEXCOLORS, SHADER_PARAM_TYPE_BOOL, "1", "ignore vertex colors" )
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if ( IsPC() && g_pHardwareConfig->GetDXSupportLevel() < 80 )
			return "Sprite_DX6";
		return 0;
	}

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

#define SHADER_USE_VERTEX_COLOR		1
#define SHADER_USE_CONSTANT_COLOR	2

	void SetSpriteCommonShadowState( unsigned int shaderFlags )
	{
		s_pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

		unsigned int flags = VERTEX_POSITION;
		if( shaderFlags & SHADER_USE_VERTEX_COLOR )
		{
			flags |= VERTEX_COLOR;
		}
		s_pShaderShadow->VertexShaderVertexFormat( flags, 1, 0, 0 );

		sprite_vs11_Static_Index vshIndex;
		bool vertexColor = ( shaderFlags & SHADER_USE_VERTEX_COLOR ) ? true : false;
		vshIndex.SetVERTEXCOLOR( vertexColor );
		s_pShaderShadow->SetVertexShader( "sprite_vs11", vshIndex.GetIndex() );

		// "VERTEXCOLOR" "0..1"
		// "CONSTANTCOLOR" "0..1"
		int pshIndex = 0;
		if ( shaderFlags & SHADER_USE_VERTEX_COLOR ) pshIndex |= 0x1;
		if ( shaderFlags & SHADER_USE_CONSTANT_COLOR ) pshIndex |= 0x2;
		s_pShaderShadow->SetPixelShader( "sprite_ps11", pshIndex );
	}

	void SetSpriteCommonDynamicState( unsigned int shaderFlags )
	{
		BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

		MaterialFogMode_t fogType = s_pShaderAPI->GetSceneFogMode();
		int fogIndex = ( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) ? 1 : 0;
		sprite_vs11_Dynamic_Index vshIndex;
		vshIndex.SetSKINNING( 0 );
		vshIndex.SetDOWATERFOG( fogIndex );
		s_pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );

		s_pShaderAPI->SetPixelShaderIndex( 0 );
		if ( shaderFlags & SHADER_USE_CONSTANT_COLOR )
		{
			SetPixelShaderConstant( 0, COLOR, ALPHA );
		}

	 	float color[4] = { 1.0, 1.0, 1.0, 1.0 };
		s_pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_MODULATION_COLOR, color );

		// identity base texture transorm
		float ident[2][4] = { 
			{ 1.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 1.0f, 0.0f, 0.0f } 
		};
		s_pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, &ident[0][0], 2 ); 
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
				pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_ALWAYS );
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
					sprite_vs11_Dynamic_Index vshIndex;
					vshIndex.SetSKINNING( 0 );
					vshIndex.SetDOWATERFOG( fogIndex );
					s_pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );

					float color[4] = { 1.0, 1.0, 1.0, 1.0 };
					s_pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_MODULATION_COLOR, color );

					s_pShaderAPI->SetPixelShaderIndex( 0 );
	
					color[0] = color[1] = color[2] = flFade * frameBlendAlpha;
					color[3] = 1.0f;
					s_pShaderAPI->SetPixelShaderConstant( 0, color );

	
					// identity base texture transorm
					float ident[2][4] = { 
						{ 1.0f, 0.0f, 0.0f, 0.0f },
						{ 0.0f, 1.0f, 0.0f, 0.0f } 
					};
					s_pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, &ident[0][0], 2 ); 
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
					sprite_vs11_Dynamic_Index vshIndex;
					vshIndex.SetSKINNING( 0 );
					vshIndex.SetDOWATERFOG( fogIndex );
					s_pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );

					float color[4] = { 1.0, 1.0, 1.0, 1.0 };
					s_pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_MODULATION_COLOR, color );

					s_pShaderAPI->SetPixelShaderIndex( 0 );

					color[0] = color[1] = color[2] = flFade * frameBlendAlpha;
					color[3] = 1.0f;
					s_pShaderAPI->SetPixelShaderConstant( 0, color );

					// identity base texture transorm
					float ident[2][4] = { 
						{ 1.0f, 0.0f, 0.0f, 0.0f },
						{ 0.0f, 1.0f, 0.0f, 0.0f } 
					};
					s_pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, &ident[0][0], 2 ); 
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
