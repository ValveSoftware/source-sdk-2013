//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"

#include "debugmorphaccumulator_ps30.inc"
#include "debugmorphaccumulator_vs30.inc"

BEGIN_VS_SHADER_FLAGS( DebugMorphAccumulator, "Help for Debug Morph Accumulator", SHADER_NOT_EDITABLE )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthTest( false );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableCulling( false );
			pShaderShadow->FogMode( SHADER_FOGMODE_DISABLED );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableSRGBWrite( false );

			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, 0, 0 );

			DECLARE_STATIC_VERTEX_SHADER( debugmorphaccumulator_vs30 );
			SET_STATIC_VERTEX_SHADER( debugmorphaccumulator_vs30 );

			DECLARE_STATIC_PIXEL_SHADER( debugmorphaccumulator_ps30 );
			SET_STATIC_PIXEL_SHADER( debugmorphaccumulator_ps30 );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE );

			DECLARE_DYNAMIC_VERTEX_SHADER( debugmorphaccumulator_vs30 );
			SET_DYNAMIC_VERTEX_SHADER( debugmorphaccumulator_vs30 );

			DECLARE_DYNAMIC_PIXEL_SHADER( debugmorphaccumulator_ps30 );
			SET_DYNAMIC_PIXEL_SHADER( debugmorphaccumulator_ps30 );
		}
		Draw( );
	}
END_SHADER
