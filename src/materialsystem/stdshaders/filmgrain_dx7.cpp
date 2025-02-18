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

DEFINE_FALLBACK_SHADER( FilmGrain, FilmGrain_DX7 )

BEGIN_SHADER( FilmGrain_DX7, "Help for FilmGrain_DX7" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( GRAIN_TEXTURE,  SHADER_PARAM_TYPE_TEXTURE, "0", "Film grain texture" )
		SHADER_PARAM( NOISESCALE,     SHADER_PARAM_TYPE_VEC4, "", "Strength of film grain" )
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
		LoadTexture( GRAIN_TEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableConstantColor( true );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_ONE_MINUS_DST_COLOR, SHADER_BLEND_ONE_MINUS_SRC_COLOR );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );
		}
		DYNAMIC_STATE
		{
			float color[4];
			params[NOISESCALE]->GetVecValue( color, 4 );
			s_pShaderAPI->Color4fv( color );	
			BindTexture( SHADER_SAMPLER0, GRAIN_TEXTURE, -1 );
		}
		Draw();
	}
END_SHADER
