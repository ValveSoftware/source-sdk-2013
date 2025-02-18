//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

// don't merge into main!!!!!!!!!!!!
#if 0
#include "BaseVSShader.h"
#include "vertextexturetest_vs30.inc"
#include "vertextexturetest_ps30.inc"

BEGIN_VS_SHADER_FLAGS( VertexTextureTest, "Help for VertexTextureTest", SHADER_NOT_EDITABLE )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BASETEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( FRAME2, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $basetexture2" )
		SHADER_PARAM( BASETEXTURE3, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( FRAME3, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $basetexture2" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 95 )
		{
			Assert( 0 );
			return "Wireframe";
		}

		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
		LoadTexture( BASETEXTURE2 );
		LoadTexture( BASETEXTURE3 );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE2, true );

			unsigned int flags = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( flags, 1, 0, 0, 0 );

	   		pShaderShadow->SetMorphFormat( MORPH_POSITION | MORPH_NORMAL | MORPH_WRINKLE );

			DECLARE_STATIC_VERTEX_SHADER( vertextexturetest_vs30 );
			SET_STATIC_VERTEX_SHADER( vertextexturetest_vs30 );

			DECLARE_STATIC_PIXEL_SHADER( vertextexturetest_ps30 );
			SET_STATIC_PIXEL_SHADER( vertextexturetest_ps30 );

			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			BindTexture( SHADER_TEXTURE_STAGE1, BASETEXTURE2, FRAME2 );
			BindTexture( SHADER_TEXTURE_STAGE2, BASETEXTURE3, FRAME3 );

			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );

			DECLARE_DYNAMIC_VERTEX_SHADER( vertextexturetest_vs30 );
			SET_DYNAMIC_VERTEX_SHADER( vertextexturetest_vs30 );

			DECLARE_DYNAMIC_PIXEL_SHADER( vertextexturetest_ps30 );
			SET_DYNAMIC_PIXEL_SHADER( vertextexturetest_ps30 );
		}
		Draw( );
	}
END_SHADER
#endif
