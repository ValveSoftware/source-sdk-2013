//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "shaderlib/cshader.h"
#include "worldvertextransition_dx6_helper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


void InitParamsWorldVertexTransition_DX6( IMaterialVar** params, WorldVertexTransition_DX6_Vars_t &info )
{
	SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
	// FLASHLIGHTFIXME
	params[info.m_nFlashlightTextureVar]->SetStringValue( "effects/flashlight001" );
}

void InitWorldVertexTransition_DX6( CBaseShader *pShader, IMaterialVar** params, WorldVertexTransition_DX6_Vars_t &info )
{
	// FLASHLIGHTFIXME
	if ( params[info.m_nFlashlightTextureVar]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nFlashlightTextureVar );
	}

	if ( params[info.m_nBaseTextureVar]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nBaseTextureVar );
	}

	if ( params[info.m_nBaseTexture2Var]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nBaseTexture2Var );
	}
}

static void DrawFlashlightPass( CBaseShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, int nPass, WorldVertexTransition_DX6_Vars_t &info )
{
	SHADOW_STATE
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_FIXED_FUNCTION_FLASHLIGHT );
		pShaderShadow->EnableDepthWrites( false );
		pShaderShadow->EnableAlphaWrites( false );

		pShaderShadow->SetDiffuseMaterialSource( SHADER_MATERIALSOURCE_COLOR1 );
		if( nPass == 0 )
		{
			pShader->EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
		}
		else
		{
			pShader->EnableAlphaBlending( SHADER_BLEND_ONE_MINUS_SRC_ALPHA, SHADER_BLEND_ONE );
		}
		
		int flags = SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD1 | SHADER_DRAW_COLOR | SHADER_DRAW_NORMAL;
		pShaderShadow->DrawFlags( flags );
		pShader->FogToBlack();

		pShaderShadow->EnableLighting( true );

		pShaderShadow->EnableCustomPixelPipe( true );
		pShaderShadow->CustomTextureStages( 2 );
		
		// color stage 0
		// projected texture * vertex color (lighting)
		pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
			SHADER_TEXCHANNEL_COLOR, 
			SHADER_TEXOP_MODULATE,
			SHADER_TEXARG_TEXTURE, 
			SHADER_TEXARG_VERTEXCOLOR );
		
		// color stage 1
		// * base texture
		pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
			SHADER_TEXCHANNEL_COLOR, 
			SHADER_TEXOP_MODULATE,
			SHADER_TEXARG_TEXTURE, SHADER_TEXARG_PREVIOUSSTAGE );
		
		// alpha stage 0
		// get alpha from constant alpha * vertex color
		pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
			SHADER_TEXCHANNEL_ALPHA, 
			SHADER_TEXOP_MODULATE,
			SHADER_TEXARG_CONSTANTCOLOR, SHADER_TEXARG_VERTEXCOLOR );
		
		// alpha stage 1
		// get alpha from $basetexture
		pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
			SHADER_TEXCHANNEL_ALPHA, 
			SHADER_TEXOP_MODULATE,
			SHADER_TEXARG_TEXTURE, SHADER_TEXARG_PREVIOUSSTAGE );
		
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

		// Shove the view position into texcoord 0 before the texture matrix.
		pShaderShadow->TexGen( SHADER_TEXTURE_STAGE0, SHADER_TEXGENPARAM_EYE_LINEAR );
		pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE0, true );
	}
	DYNAMIC_STATE
	{
		pShader->SetFlashlightFixedFunctionTextureTransform( MATERIAL_TEXTURE0 );

		// NOTE: This has to come after the loadmatrix since the loadmatrix screws with the
		// transform flags!!!!!!
		// Specify that we have XYZ texcoords that need to be divided by W before the pixel shader.
		// NOTE Tried to divide XY by Z, but doesn't work.
		pShaderAPI->SetTextureTransformDimension( SHADER_TEXTURE_STAGE0, 3, true );
		
		pShader->BindTexture( SHADER_SAMPLER0, FLASHLIGHTTEXTURE, FLASHLIGHTTEXTUREFRAME );
		if( nPass == 0 )
		{
			pShader->BindTexture( SHADER_SAMPLER1, info.m_nBaseTexture2Var, -1 );
		}
		else
		{
			pShader->BindTexture( SHADER_SAMPLER1, info.m_nBaseTextureVar, info.m_nBaseTextureFrameVar );
		}
	}
	pShader->Draw();
}


void DrawWorldVertexTransition_DX6( CBaseShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, WorldVertexTransition_DX6_Vars_t &info )
{
	bool bHasFlashlight = pShader->UsingFlashlight( params );
	if( bHasFlashlight )
	{
		DrawFlashlightPass( pShader, params, pShaderAPI, pShaderShadow, 0, info );
		DrawFlashlightPass( pShader, params, pShaderAPI, pShaderShadow, 1, info );
		return;
	}

	SHADOW_STATE
	{
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
		pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, OVERBRIGHT );

		pShaderShadow->DrawFlags( SHADER_DRAW_COLOR | SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD1 | SHADER_DRAW_LIGHTMAP_TEXCOORD0 );
		pShader->FogToFogColor();
	}
	DYNAMIC_STATE
	{
		//pShaderAPI->TexMinFilter( SHADER_TEXFILTERMODE_NEAREST );
		//pShaderAPI->TexMagFilter( SHADER_TEXFILTERMODE_NEAREST );
		pShader->BindTexture( SHADER_SAMPLER1, info.m_nBaseTextureVar );
		pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_LIGHTMAP );
	}
	pShader->Draw();

	SHADOW_STATE
	{
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
		pShaderShadow->EnableBlending( true );

		pShaderShadow->EnableCustomPixelPipe( true );
		pShaderShadow->CustomTextureStages( 2 );
		
		// alpha and color stage 0
		pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
			SHADER_TEXCHANNEL_ALPHA, 
			SHADER_TEXOP_SELECTARG1,
			SHADER_TEXARG_TEXTURE, 
			SHADER_TEXARG_TEXTURE );

		pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
			SHADER_TEXCHANNEL_COLOR, 
			SHADER_TEXOP_SELECTARG1,
			SHADER_TEXARG_TEXTURE, 
			SHADER_TEXARG_TEXTURE );
		
		// alpha and color stage 1
		pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
			SHADER_TEXCHANNEL_ALPHA, 
			SHADER_TEXOP_MODULATE,
			SHADER_TEXARG_TEXTURE, 
			SHADER_TEXARG_VERTEXCOLOR );

		pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
			SHADER_TEXCHANNEL_COLOR, 
			SHADER_TEXOP_MODULATE2X,
			SHADER_TEXARG_PREVIOUSSTAGE, 
			SHADER_TEXARG_TEXTURE );

		// Looks backwards, but this is done so that lightmap alpha = 1 when only
		// using 1 texture (needed for translucent displacements).
		pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );

		pShaderShadow->DrawFlags( SHADER_DRAW_COLOR | SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD1 | SHADER_DRAW_LIGHTMAP_TEXCOORD0 );
		pShader->FogToFogColor();
	}
	DYNAMIC_STATE
	{
		pShader->BindTexture( SHADER_SAMPLER1, info.m_nBaseTexture2Var );
		pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_LIGHTMAP );
	}
	pShader->Draw();
}
