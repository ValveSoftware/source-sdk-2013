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


DEFINE_FALLBACK_SHADER( WorldTwoTextureBlend, WorldTwoTextureBlend_DX6 )


BEGIN_SHADER( WorldTwoTextureBlend_DX6, 
			  "Help for WorldTwoTextureBlend" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM_OVERRIDE( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldTwoTextureBlend", "iris texture", 0 )
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/WorldTwoTextureBlend_detail", "detail texture" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "1.0", "scale of the detail texture" )
		SHADER_PARAM( DETAIL_ALPHA_MASK_BASE_TEXTURE, SHADER_PARAM_TYPE_BOOL, "0", 
			"If this is 1, then when detail alpha=0, no base texture is blended and when "
			"detail alpha=1, you get detail*base*lightmap" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		LoadTexture( FLASHLIGHTTEXTURE );
		LoadTexture( BASETEXTURE );
		LoadTexture( DETAIL );
	}

	SHADER_INIT_PARAMS()
	{
		// FLASHLIGHTFIXME
		params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight001" );

		if( !params[DETAIL_ALPHA_MASK_BASE_TEXTURE]->IsDefined() )
			params[DETAIL_ALPHA_MASK_BASE_TEXTURE]->SetIntValue( 0 );
	
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
	}

	SHADER_DRAW
	{
		float detailScale = params[DETAILSCALE]->GetFloatValue();

		bool hasFlashlight = UsingFlashlight( params );

		if( hasFlashlight )
		{
			DrawFlashlight_dx70( params, pShaderAPI, pShaderShadow, FLASHLIGHTTEXTURE, FLASHLIGHTTEXTUREFRAME );
			return;
		}

		// DX6 fallback mode.
		if ( params[DETAIL_ALPHA_MASK_BASE_TEXTURE]->GetIntValue() )
		{
			DetailAlphaMaskPass1( pShaderShadow, pShaderAPI, params, detailScale );
			DetailAlphaMaskPass2( pShaderShadow, pShaderAPI, detailScale );
		}
		else
		{
			// FIXME: add multitexture support!
			NormalModePass1( pShaderShadow, pShaderAPI );
			NormalModePass2( pShaderShadow, pShaderAPI, params, detailScale );
			NormalModePass3( pShaderShadow, pShaderAPI, params, detailScale );
		}
	}


	// ------------------------------------------------------------------------------ //
	// "Normal" mode - doesn't use the detail texture's alpha mask.
	// ------------------------------------------------------------------------------ //
	
	void NormalModePass1( 
		IShaderShadow *pShaderShadow, 
		IShaderDynamicAPI *pShaderAPI )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
		}
		Draw();
	}

	void NormalModePass2(
		IShaderShadow *pShaderShadow, 
		IShaderDynamicAPI *pShaderAPI,
		IMaterialVar **params,
		float detailScale )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );
			FogToFogColor();
		}
		
		DYNAMIC_STATE
		{
			if ( detailScale != 1.0f )
			{
				pShaderAPI->MatrixMode( MATERIAL_TEXTURE0 );
				pShaderAPI->LoadIdentity();
				pShaderAPI->ScaleXY( detailScale, detailScale );
			}
			BindTexture( SHADER_SAMPLER0, DETAIL );
		}
		Draw();
	}

	void NormalModePass3(
		IShaderShadow *pShaderShadow, 
		IShaderDynamicAPI *pShaderAPI,
		IMaterialVar **params,
		float detailScale )
	{
		SHADOW_STATE
		{
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
			SingleTextureLightmapBlendMode();
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_LIGHTMAP_TEXCOORD0 );
			FogToOOOverbright();
		}
		DYNAMIC_STATE
		{
			if ( detailScale != 1.0f )
				pShaderAPI->LoadIdentity( );
			
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_LIGHTMAP );
		}
		Draw();
	}


	// ------------------------------------------------------------------------------ //
	// "Detail alpha mask mode".
	// ------------------------------------------------------------------------------ //

	void DetailAlphaMaskPass1( 
		IShaderShadow *pShaderShadow, 
		IShaderDynamicAPI *pShaderAPI, 
		IMaterialVar **params,
		float detailScale )
	{
		// The equation is [B*Da + (1-Da)] * [D * L]
		SHADOW_STATE
		{
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );

			pShaderShadow->EnableCustomPixelPipe( true );
			pShaderShadow->CustomTextureStages( 2 );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
 		
			// Stage 0
			// Color = B*2
			// Note the 2x here.. we do 4x total in this shader and 
			// the first 2x is here. The second is in SingleTextureLightmapBlendMode in the 2nd pass.
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_MODULATE2X,	
				SHADER_TEXARG_TEXTURE, SHADER_TEXARG_CONSTANTCOLOR );

			// Stage 1 [where P = prev stage]
			// Color = B*Da + (1-Da)
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
				SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_MODULATEINVCOLOR_ADDALPHA, 
				SHADER_TEXARG_INVTEXTUREALPHA, SHADER_TEXARG_PREVIOUSSTAGE );

			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_TEXCOORD1 );
			FogToFogColor();
		}									  
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			BindTexture( SHADER_SAMPLER1, DETAIL );

			pShaderAPI->Color4f( 1, 1, 1, 1 );

			if ( detailScale != 1.0f )
			{
				pShaderAPI->MatrixMode( MATERIAL_TEXTURE1 );
				pShaderAPI->LoadIdentity();
				pShaderAPI->ScaleXY( detailScale, detailScale );
			}
			
		}
		Draw();
	}

	void DetailAlphaMaskPass2( IShaderShadow *pShaderShadow, IShaderDynamicAPI *pShaderAPI, float detailScale )
	{
		SHADOW_STATE
		{
			s_pShaderShadow->EnableCustomPixelPipe( true );
			s_pShaderShadow->CustomTextureStages( 2 );

			s_pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			s_pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			
			s_pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE0, true);

			// Make sure the texgen transform is applied to the texture coordinates and not to an auto-generated reflection vector or whatever.
			s_pShaderShadow->TexGen( SHADER_TEXTURE_STAGE0, SHADER_TEXGENPARAM_OBJECT_LINEAR ); 

			// This turns on blending and does overbrighting if it's enabled.
			SingleTextureLightmapBlendMode();

			// Stage 0, color = D
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_SELECTARG1, 
				SHADER_TEXARG_TEXTURE, SHADER_TEXARG_CONSTANTCOLOR );

			// Stage 1, color = D*L
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
				SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_MODULATE, 
				SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_TEXTURE );
		
			// Use the lightmap coordinates in both stages.
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_LIGHTMAP_TEXCOORD1 );
			FogToFogColor();
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, DETAIL);
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_LIGHTMAP );

			if ( detailScale != 1.0f )			
			{
				pShaderAPI->MatrixMode( MATERIAL_TEXTURE1 );
				pShaderAPI->LoadIdentity();
				
				pShaderAPI->MatrixMode( MATERIAL_TEXTURE0 );
				pShaderAPI->LoadIdentity();
				pShaderAPI->ScaleXY( detailScale, detailScale );
			}
		}
		
		Draw();
	}

END_SHADER

