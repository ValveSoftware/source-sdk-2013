//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "BaseVSShader.h"
#include "common_hlsl_cpp_consts.h"
#include "convar.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( Downsample_nohdr, Downsample_nohdr_DX80 )

BEGIN_VS_SHADER_FLAGS( Downsample_nohdr_DX80, "Help for Downsample_nohdr_DX80", SHADER_NOT_EDITABLE )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
	}

	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 80 )
		{
			return "Wireframe";
		}
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableAlphaWrites( true );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );

			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, 0, 0 );

			pShaderShadow->SetVertexShader( "Downsample_vs11", 0 );
			pShaderShadow->SetPixelShader( "Downsample_nohdr_ps11" );
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, -1 );
			BindTexture( SHADER_SAMPLER1, BASETEXTURE, -1 );
			BindTexture( SHADER_SAMPLER2, BASETEXTURE, -1 );
			BindTexture( SHADER_SAMPLER3, BASETEXTURE, -1 );

			int width, height;
			pShaderAPI->GetBackBufferDimensions( width, height );

			float v[4][4];
			float dX = 1.0f/width;
			float dY = 1.0f/height;

			v[0][0] = .5*dX;
			v[0][1] = .5*dY;
			v[1][0] = 2.5*dX;
			v[1][1] = .5*dY;
			v[2][0] = .5*dX;
			v[2][1] = 2.5*dY;
			v[3][0] = 2.5*dX;
			v[3][1] = 2.5*dY;
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, &v[0][0], 4 );

			pShaderAPI->SetVertexShaderIndex( 0 );
			pShaderAPI->SetPixelShaderIndex( 0 );
		}
		Draw();
	}
END_SHADER
