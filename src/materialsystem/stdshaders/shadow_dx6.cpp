//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "shaderlib/cshader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( Shadow, Shadow_DX6 )

BEGIN_SHADER_FLAGS( Shadow_DX6, "Help for Shadow_DX6", SHADER_NOT_EDITABLE )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		// FIXME: Need fallback for dx5, don't fade out shadows, just pop them out

		/*
		The alpha blending state either must be:

	Src Color * Dst Color + Dst Color * 0	
		(src color = C*A + 1-A)

or

  // Can't be this, doesn't work with fog
	Src Color * Dst Color + Dst Color * (1-Src Alpha)	
		(src color = C * A, Src Alpha = A)

	*/
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
			LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

			// To accomplish shadow fading, subtract vertex alpha from texture alpha
			// color channel isn't used...
			pShaderShadow->EnableCustomPixelPipe( true );
			pShaderShadow->CustomTextureStages( 2 );
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_SELECTARG1, 
				SHADER_TEXARG_TEXTURE, SHADER_TEXARG_VERTEXCOLOR );
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SUBTRACT, 
				SHADER_TEXARG_TEXTURE, SHADER_TEXARG_VERTEXCOLOR );

			// Blend between white and the vertex color...
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
				SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_BLEND_PREVIOUSSTAGEALPHA, 
				SHADER_TEXARG_CONSTANTCOLOR, SHADER_TEXARG_TEXTURE );
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
				SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1, 
				SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_VERTEXCOLOR );

			EnableAlphaBlending( SHADER_BLEND_DST_COLOR, SHADER_BLEND_ZERO );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_COLOR | 
				SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_TEXCOORD1 );
			// We need to fog to *white* regardless of overbrighting...
			FogToWhite();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			BindTexture( SHADER_SAMPLER1, BASETEXTURE, FRAME );
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE0, BASETEXTURETRANSFORM );
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE1, BASETEXTURETRANSFORM );

			// The constant color is the shadow color...
			SetColorState( COLOR );
		}
		Draw( );
	}
END_SHADER
