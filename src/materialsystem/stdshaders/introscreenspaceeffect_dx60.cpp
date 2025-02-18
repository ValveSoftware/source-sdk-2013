//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

DEFINE_FALLBACK_SHADER( IntroScreenSpaceEffect, IntroScreenSpaceEffect_dx60 )

BEGIN_SHADER_FLAGS( IntroScreenSpaceEffect_dx60, "Help for IntroScreenSpaceEffect_dx60", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( MODE, SHADER_PARAM_TYPE_INTEGER, "0", "" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_FULL_FRAME_BUFFER_TEXTURE );
	}

	SHADER_INIT
	{
	}
	
	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableCulling( false );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableDepthTest( false );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_TEXCOORD1 );
			DisableFog();

			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
			pShaderShadow->EnableCustomPixelPipe( true );
			pShaderShadow->CustomTextureStages( 2 );
			pShaderShadow->EnableConstantAlpha( true );
			pShaderShadow->EnableBlending( true );

			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_SELECTARG1, 
				SHADER_TEXARG_TEXTURE, SHADER_TEXARG_NONE );
		
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
				SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_ADD, 
				SHADER_TEXARG_TEXTURE, SHADER_TEXARG_PREVIOUSSTAGE );

			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
				SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1, 
				SHADER_TEXARG_CONSTANTCOLOR, SHADER_TEXARG_NONE );
		}
		DYNAMIC_STATE
		{
			switch( params[MODE]->GetIntValue() )
			{
			case 7:
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0 );
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_BLACK );
				break;
			case 8:
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_1 );
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_BLACK );
				break;
			default:
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0 );
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_1 );
				break;
			}
			float color[4] = { 1.0, 1.0, 1.0, 1.0 };
			color[3] = params[ALPHA]->GetFloatValue();
			s_pShaderAPI->Color4fv( color );	
		}
		Draw();
	}
END_SHADER
