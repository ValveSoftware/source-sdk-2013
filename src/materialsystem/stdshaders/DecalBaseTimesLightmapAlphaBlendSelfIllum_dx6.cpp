//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "shaderlib/cshader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( DecalBaseTimesLightmapAlphaBlendSelfIllum, DecalBaseTimesLightmapAlphaBlendSelfIllum_DX6 )

BEGIN_SHADER( DecalBaseTimesLightmapAlphaBlendSelfIllum_DX6, 
			  "Help for DecalBaseTimesLightmapAlphaBlendSelfIllum_DX6" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM_OVERRIDE( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "decals/decalporthole001b", "decal base texture", 0 )
		SHADER_PARAM( SELFILLUMTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "decals/decalporthole001b_mask", "self-illum texture" )
		SHADER_PARAM( SELFILLUMTEXTUREFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "self-illum texture frame" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
		LoadTexture( SELFILLUMTEXTURE );
	}

	SHADER_DRAW
	{
		if( g_pHardwareConfig->GetSamplerCount() < 2 )
		{
			ShaderWarning( "DecalBaseTimesLightmapAlphaBlendSelfIllum: not implemented for single-texturing hardware\n" );
			return;
		}


		SHADOW_STATE
		{
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnablePolyOffset( SHADER_POLYOFFSET_DECAL );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, OVERBRIGHT );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | 
						SHADER_DRAW_TEXCOORD1 | SHADER_DRAW_LIGHTMAP_TEXCOORD0 );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_LIGHTMAP );
			BindTexture( SHADER_SAMPLER1, BASETEXTURE, FRAME );
		}
		Draw();

		SHADOW_STATE
		{
			SetInitialShadowState( );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnablePolyOffset( SHADER_POLYOFFSET_DECAL );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );
			FogToBlack();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, SELFILLUMTEXTURE, SELFILLUMTEXTUREFRAME );
		}
		Draw();
	}
END_SHADER
