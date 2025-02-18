//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "screenspaceeffect_vs20.inc"
#include "floattoscreen_vanilla_ps20.inc"
#include "floattoscreen_vanilla_ps20b.inc"
#include "floattoscreen_ps20.inc"
#include "floattoscreen_ps20b.inc"

BEGIN_VS_SHADER_FLAGS( floattoscreen_vanilla, "Help for floattoscreen_vanilla", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		if( params[FBTEXTURE]->IsDefined() )
		{
			LoadTexture( FBTEXTURE );
		}
	}
	
	SHADER_FALLBACK
	{
		// Requires DX9 + above
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			Assert( 0 );
			return "Wireframe";
		}
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthWrites( false );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			// convert from linear to gamma on write.
			pShaderShadow->EnableSRGBWrite( true );

			// Pre-cache shaders
			DECLARE_STATIC_VERTEX_SHADER( screenspaceeffect_vs20 );
			SET_STATIC_VERTEX_SHADER( screenspaceeffect_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( floattoscreen_ps20b );
				SET_STATIC_PIXEL_SHADER( floattoscreen_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( floattoscreen_ps20 );
				SET_STATIC_PIXEL_SHADER( floattoscreen_ps20 );
			}
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, FBTEXTURE, -1 );
			DECLARE_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( floattoscreen_vanilla_ps20b );
				SET_DYNAMIC_PIXEL_SHADER( floattoscreen_vanilla_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( floattoscreen_vanilla_ps20 );
				SET_DYNAMIC_PIXEL_SHADER( floattoscreen_vanilla_ps20 );
			}
		}
		Draw();
	}
END_SHADER
