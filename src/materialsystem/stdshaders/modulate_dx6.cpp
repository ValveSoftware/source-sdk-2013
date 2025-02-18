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

// FIXME!!  This version doesn't support vertexalpha to make it blend to white!!!
DEFINE_FALLBACK_SHADER( Modulate, Modulate_DX6 )

BEGIN_SHADER( Modulate_DX6,
			  "Help for Modulate_DX6" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( WRITEZ, SHADER_PARAM_TYPE_BOOL, "0", "Forces z to be written if set" )
		SHADER_PARAM( MOD2X, SHADER_PARAM_TYPE_BOOL, "0", "forces a 2x modulate so that you can brighten and darken things" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
			LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		bool bMod2X = params[MOD2X]->IsDefined() && params[MOD2X]->GetIntValue();
		SHADOW_STATE
		{
			if( bMod2X )
			{
				EnableAlphaBlending( SHADER_BLEND_DST_COLOR, SHADER_BLEND_SRC_COLOR );
			}
			else
			{
				EnableAlphaBlending( SHADER_BLEND_DST_COLOR, SHADER_BLEND_ZERO );
			}

			if (params[WRITEZ]->GetIntValue() != 0)
			{
				// This overrides the disabling of depth writes performed in
				// EnableAlphaBlending
				pShaderShadow->EnableDepthWrites(true);
			}

			pShaderShadow->EnableConstantColor( true );

			int drawFlags = SHADER_DRAW_POSITION;

			if (params[BASETEXTURE]->IsTexture())
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				drawFlags |= SHADER_DRAW_TEXCOORD0;
			}

			if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
			{
				drawFlags |= SHADER_DRAW_COLOR;
			}

			pShaderShadow->DrawFlags( drawFlags );
			// We need to fog to *white* regardless of overbrighting...
			if( bMod2X )
			{
				FogToGrey();
			}
			else
			{
				FogToOOOverbright();
			}
		}
		DYNAMIC_STATE
		{
			if (params[BASETEXTURE]->IsTexture())
			{
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
				SetFixedFunctionTextureTransform( MATERIAL_TEXTURE0, BASETEXTURETRANSFORM );
			}

			// The constant color is the modulation color...
			SetColorState( COLOR );

		}
		Draw( );
	}
END_SHADER
