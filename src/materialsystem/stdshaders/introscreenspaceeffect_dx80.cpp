//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "ScreenSpaceEffect_vs11.inc"
#include "IntroScreenSpaceEffect_ps11.inc"

DEFINE_FALLBACK_SHADER( IntroScreenSpaceEffect, IntroScreenSpaceEffect_dx80 )

BEGIN_VS_SHADER_FLAGS( IntroScreenSpaceEffect_dx80, "Help for IntroScreenSpaceEffect_dx80", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( MODE, SHADER_PARAM_TYPE_INTEGER, "0", "" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_FULL_FRAME_BUFFER_TEXTURE );
	}

	SHADER_INIT
	{	
	}
	
	SHADER_FALLBACK
	{
		// Requires DX9 + above
		if ( IsPC() && g_pHardwareConfig->GetDXSupportLevel() < 80 )
			return "introscreenspaceeffect_dx60";
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );
			DECLARE_STATIC_VERTEX_SHADER( screenspaceeffect_vs11 );
			SET_STATIC_VERTEX_SHADER( screenspaceeffect_vs11 );

			DECLARE_STATIC_PIXEL_SHADER( introscreenspaceeffect_ps11 );
			SET_STATIC_PIXEL_SHADER( introscreenspaceeffect_ps11 );

			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0 );
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_1 );
			DECLARE_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs11 );
			SET_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs11 );

			DECLARE_DYNAMIC_PIXEL_SHADER( introscreenspaceeffect_ps11 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( MODE,  params[MODE]->GetIntValue() );
			SET_DYNAMIC_PIXEL_SHADER( introscreenspaceeffect_ps11 );

			SetPixelShaderConstant( 0, ALPHA );
		}
		Draw();
	}
END_SHADER
