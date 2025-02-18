//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "shaderlib/cshader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_SHADER_FLAGS( Decal, "Help for Decal", SHADER_NOT_EDITABLE )
			  
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS
	
	SHADER_INIT_PARAMS()
	{
		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		// vertex alpha 0 = mod2x, vertex alpha 1 = src_alpha,1-src_alpha
		// need to set constant color to grey
		SHADOW_STATE
		{
			// common stuff
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnablePolyOffset( SHADER_POLYOFFSET_DECAL );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->EnableBlending( true );
		}
		// MOD2X pass		
		SHADOW_STATE
		{
			pShaderShadow->EnableCustomPixelPipe( true );
			pShaderShadow->CustomTextureStages( 2 );

			// color = texture
			// alpha = vertexalpha			
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
					SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_SELECTARG1, 
					SHADER_TEXARG_TEXTURE, SHADER_TEXARG_NONE );
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
					SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1, 
					SHADER_TEXARG_SPECULARCOLOR, SHADER_TEXARG_NONE );

			// color = texture
			// alpha = blend
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
				SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_BLEND_PREVIOUSSTAGEALPHA, 
				SHADER_TEXARG_CONSTANTCOLOR, SHADER_TEXARG_PREVIOUSSTAGE );

			pShaderShadow->BlendFunc( SHADER_BLEND_DST_COLOR, SHADER_BLEND_SRC_COLOR );
			FogToGrey();

			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_SPECULAR
				| SHADER_DRAW_LIGHTMAP_TEXCOORD1 );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->Color3f( 0.5f, 0.5f, 0.5f );
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_LIGHTMAP );
		}
		Draw();

		// srcalpha,1-srcalpha pass		
		SHADOW_STATE
		{
			pShaderShadow->EnableCustomPixelPipe( true );
			pShaderShadow->CustomTextureStages( 2 );

			// color = texture
			// alpha = texture
			if (IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ))
			{
				pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
						SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_MODULATE, 
						SHADER_TEXARG_TEXTURE, SHADER_TEXARG_VERTEXCOLOR );
			}
			else
			{
				pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
						SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_SELECTARG1, 
						SHADER_TEXARG_TEXTURE, SHADER_TEXARG_NONE );
			}

			if( IS_FLAG_SET( MATERIAL_VAR_VERTEXALPHA ) )
			{
				pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
						SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_MODULATE, 
						SHADER_TEXARG_TEXTURE, SHADER_TEXARG_VERTEXCOLOR );
			}
			else
			{
				pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
						SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1, 
						SHADER_TEXARG_TEXTURE, SHADER_TEXARG_NONE );
			}

			// color = texture [* vertex color] * lightmap color
			// alpha = texture alpha [* vertex alpha] * specular alpha
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
					SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_MODULATE2X, 
					SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_TEXTURE );
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
				SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_MODULATE, 
				SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_SPECULARCOLOR );

			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			SetColorState( COLOR );
		}
		Draw();
	}
END_SHADER
