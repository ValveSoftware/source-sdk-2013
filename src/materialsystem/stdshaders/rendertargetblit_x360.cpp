//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Replaces 360 missing StretchRect() functionality.
//
//=============================================================================

#include "BaseVSShader.h"
#include "rendertargetblit_vs20.inc"
#include "rendertargetblit_ps20.inc"
#include "rendertargetblit_ps20b.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER_FLAGS( RenderTargetBlit_X360, "", SHADER_NOT_EDITABLE )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		if ( !params[BASETEXTURE]->IsDefined() )
		{
			params[BASETEXTURE]->SetStringValue( "_rt_FullFrameFB" );
		}
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, false );
			pShaderShadow->EnableSRGBWrite( false );

			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_ALWAYS );
			pShaderShadow->EnableCulling( false );

			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, 0, 0 );

			DECLARE_STATIC_VERTEX_SHADER( rendertargetblit_vs20 );
			SET_STATIC_VERTEX_SHADER( rendertargetblit_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( rendertargetblit_ps20b );
				SET_STATIC_PIXEL_SHADER( rendertargetblit_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( rendertargetblit_ps20 );
				SET_STATIC_PIXEL_SHADER( rendertargetblit_ps20 );
			}
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

			DECLARE_DYNAMIC_VERTEX_SHADER( rendertargetblit_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( rendertargetblit_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( rendertargetblit_ps20b );
				SET_DYNAMIC_PIXEL_SHADER( rendertargetblit_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( rendertargetblit_ps20 );
				SET_DYNAMIC_PIXEL_SHADER( rendertargetblit_ps20 );
			}
		}

		Draw();		
	}
END_SHADER
