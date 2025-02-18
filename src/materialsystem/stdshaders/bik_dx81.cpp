//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "cpp_shader_constant_register_map.h"

BEGIN_VS_SHADER( Bik_dx81, "Help for Bik_dx81" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( YTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "Y Bink Texture" )
//		SHADER_PARAM( ATEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "A Bink Texture" )
		SHADER_PARAM( CRTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "Cr Bink Texture" )
		SHADER_PARAM( CBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "Cb Bink Texture" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 81 )
		{	
			return "bik_dx80";
		}
		return 0;
	}

	SHADER_INIT
	{
		if ( params[YTEXTURE]->IsDefined() )
		{
			LoadTexture( YTEXTURE );
		}
//		if ( params[ATEXTURE]->IsDefined() )
//		{
//			LoadTexture( ATEXTURE );
//		}
		if ( params[CRTEXTURE]->IsDefined() )
		{
			LoadTexture( CRTEXTURE );
		}
		if ( params[CBTEXTURE]->IsDefined() )
		{
			LoadTexture( CBTEXTURE );
		}
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
			// we don't do alpha for these on dx8
			//				pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );

			unsigned int flags = VERTEX_POSITION;
			int numTexCoords = 1;
			pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, 0, 0 );

			pShaderShadow->SetVertexShader( "bik_vs11", 0 );
			pShaderShadow->SetPixelShader( "bik_ps14", 0 );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, YTEXTURE, FRAME );
			BindTexture( SHADER_SAMPLER1, CRTEXTURE, FRAME );
			BindTexture( SHADER_SAMPLER2, CBTEXTURE, FRAME );
			// we don't do alpha for these on dx8
			//				BindTexture( SHADER_SAMPLER3, ATEXTURE, FRAME );

			pShaderAPI->SetVertexShaderIndex( 0 );
			pShaderAPI->SetPixelShaderIndex( 0 );

			// We need the view matrix
			LoadViewMatrixIntoVertexShaderConstant( VERTEX_SHADER_VIEWMODEL );
		}
		Draw( );
	}
END_SHADER
