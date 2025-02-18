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

DEFINE_FALLBACK_SHADER( FilmDust, FilmDust_DX7 )

BEGIN_SHADER( FilmDust_DX7, "Help for FilmDust_DX7" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DUST_TEXTURE,   SHADER_PARAM_TYPE_TEXTURE, "0", "Film grain texture" )
		SHADER_PARAM( CHANNEL_SELECT, SHADER_PARAM_TYPE_VEC4,    "",  "Select which color channel to use" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		// Requires DX9 + above
		if ( g_pHardwareConfig->GetDXSupportLevel() < 70 )
		{
			Assert( 0 );
			return "Wireframe";
		}
		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( DUST_TEXTURE );								 
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableConstantColor( true );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_ZERO, SHADER_BLEND_ONE_MINUS_SRC_COLOR );

			pShaderShadow->EnableCustomPixelPipe( true );
			pShaderShadow->CustomTextureStages( 1 );
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_DOTPRODUCT3, SHADER_TEXARG_CONSTANTCOLOR, SHADER_TEXARG_TEXTURE );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		}
		DYNAMIC_STATE
		{
			float color[4];
			params[CHANNEL_SELECT]->GetVecValue( color, 4 );
			s_pShaderAPI->Color4fv( color );	

			BindTexture( SHADER_SAMPLER0, DUST_TEXTURE, -1 );
		}
		Draw();
	}
END_SHADER
