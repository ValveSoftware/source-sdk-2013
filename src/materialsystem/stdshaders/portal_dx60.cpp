//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "convar.h"
#include "d3dx.h"

DEFINE_FALLBACK_SHADER( Portal, Portal_DX60 )

BEGIN_VS_SHADER( Portal_DX60, 
				"Help for Portal_DX60 shader" )

				BEGIN_SHADER_PARAMS
				SHADER_PARAM_OVERRIDE( COLOR, SHADER_PARAM_TYPE_COLOR, "{255 255 255}", "unused", SHADER_PARAM_NOT_EDITABLE )
				SHADER_PARAM_OVERRIDE( ALPHA, SHADER_PARAM_TYPE_FLOAT, "1.0", "unused", SHADER_PARAM_NOT_EDITABLE )
				SHADER_PARAM( STATICAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0.0", "Amount of the static blend texture to blend into the base texture" )
				SHADER_PARAM( STATICBLENDTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "When adding static, this is the texture that gets blended in" )
				SHADER_PARAM( STATICBLENDTEXTUREFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
				SHADER_PARAM( ALPHAMASKTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "An alpha mask for odd shaped portals" )
				SHADER_PARAM( ALPHAMASKTEXTUREFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
				SHADER_PARAM( RENDERFIXZ, SHADER_PARAM_TYPE_INTEGER, "0", "Special depth handling, intended for rendering bug workarounds for extremely close polygons" )
				END_SHADER_PARAMS

SHADER_INIT_PARAMS()
{
	SET_FLAGS( MATERIAL_VAR_TRANSLUCENT );
}

SHADER_FALLBACK
{
	return 0; //it doesn't get any simpler than this
}

SHADER_INIT
{
	if( params[STATICBLENDTEXTURE]->IsDefined() )
		LoadTexture( STATICBLENDTEXTURE );
	if( params[ALPHAMASKTEXTURE]->IsDefined() )
		LoadTexture( ALPHAMASKTEXTURE );

	if( !params[STATICAMOUNT]->IsDefined() )
		params[STATICAMOUNT]->SetFloatValue( 0.0f );

	if( !params[STATICAMOUNT]->IsDefined() )
		params[STATICAMOUNT]->SetFloatValue( 0.0f );

	if( !params[STATICBLENDTEXTURE]->IsDefined() )
		params[STATICBLENDTEXTURE]->SetIntValue( 0 );
	if( !params[STATICBLENDTEXTUREFRAME]->IsDefined() )
		params[STATICBLENDTEXTUREFRAME]->SetIntValue( 0 );

	if( !params[ALPHAMASKTEXTURE]->IsDefined() )
		params[ALPHAMASKTEXTURE]->SetIntValue( 0 );
	if( !params[ALPHAMASKTEXTUREFRAME]->IsDefined() )
		params[ALPHAMASKTEXTUREFRAME]->SetIntValue( 0 );

	if( !params[RENDERFIXZ]->IsDefined() )
		params[RENDERFIXZ]->SetIntValue( 0 );
}

SHADER_DRAW
{
	bool bAlphaMaskTexture = params[ALPHAMASKTEXTURE]->IsTexture();

	SHADOW_STATE
	{
		SetInitialShadowState();
		
		bool bIsModel = IS_FLAG_SET( MATERIAL_VAR_MODEL );

		FogToFogColor();

		if( params[RENDERFIXZ]->GetIntValue() == 0 )
		{
			//pShaderShadow->EnablePolyOffset( SHADER_POLYOFFSET_DECAL ); //a portal is effectively a decal on top of a wall
			pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_NEAREROREQUAL );
		}
		else
		{
			pShaderShadow->EnablePolyOffset( SHADER_POLYOFFSET_DISABLE );
			pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_ALWAYS );
			pShaderShadow->EnableDepthTest( false );
			pShaderShadow->EnableDepthWrites( false );
		}

		pShaderShadow->EnableDepthWrites( true );

		if( bAlphaMaskTexture )
		{
			pShaderShadow->EnableAlphaTest( true );
			if( bIsModel )
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_TEXCOORD1 );
			else
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_LIGHTMAP_TEXCOORD0 | SHADER_DRAW_LIGHTMAP_TEXCOORD1 );
		}
		else
		{
			if( bIsModel )
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );
			else
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_LIGHTMAP_TEXCOORD0 );
		}
	}
	DYNAMIC_STATE
	{
		pShaderAPI->SetDefaultState();
	}

	if( bAlphaMaskTexture )
		StaticPass_WithAlphaMask( pShaderShadow, pShaderAPI, params ); //portal static texture blending
	else
		StaticPass_NoAlphaMask( pShaderShadow, pShaderAPI, params ); //portal static texture blending
}


void StaticPass_NoAlphaMask( IShaderShadow *pShaderShadow, IShaderDynamicAPI *pShaderAPI, IMaterialVar **params )
{
	bool bStaticBlendTexture = params[STATICBLENDTEXTURE]->IsTexture();

	SHADOW_STATE
	{
		if( bStaticBlendTexture )
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
	}
	DYNAMIC_STATE
	{
		if( bStaticBlendTexture )
			BindTexture( SHADER_SAMPLER0, STATICBLENDTEXTURE, STATICBLENDTEXTUREFRAME );
		else
			pShaderAPI->Color4f( 0.5f, 0.5f, 0.5f, 0.0f );
	}
	Draw();
}


void StaticPass_WithAlphaMask( IShaderShadow *pShaderShadow, IShaderDynamicAPI *pShaderAPI, IMaterialVar **params )
{
	bool bStaticBlendTexture = params[STATICBLENDTEXTURE]->IsTexture();

	SHADOW_STATE
	{
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
		pShaderShadow->EnableBlending( true );
		pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );

		pShaderShadow->EnableCustomPixelPipe( true );
		pShaderShadow->CustomTextureStages( 2 );

		//portal static
		if( bStaticBlendTexture )
		{
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_COLOR, 
				SHADER_TEXOP_SELECTARG1,
				SHADER_TEXARG_TEXTURE, SHADER_TEXARG_TEXTURE );
		}
		else
		{
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_COLOR, 
				SHADER_TEXOP_SELECTARG1,
				SHADER_TEXARG_CONSTANTCOLOR, SHADER_TEXARG_CONSTANTCOLOR );
		}

		pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
			SHADER_TEXCHANNEL_ALPHA, 
			SHADER_TEXOP_SELECTARG1,
			SHADER_TEXARG_ZERO, SHADER_TEXARG_ZERO );


		//alpha mask
		pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
			SHADER_TEXCHANNEL_COLOR, 
			SHADER_TEXOP_SELECTARG1,
			SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_PREVIOUSSTAGE );

		pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
			SHADER_TEXCHANNEL_ALPHA, 
			SHADER_TEXOP_SELECTARG1,
			SHADER_TEXARG_TEXTUREALPHA, SHADER_TEXARG_TEXTUREALPHA );
	}
	DYNAMIC_STATE
	{
		if( bStaticBlendTexture )
			BindTexture( SHADER_SAMPLER0, STATICBLENDTEXTURE, STATICBLENDTEXTUREFRAME );
		else
			pShaderAPI->Color4f( 0.5f, 0.5f, 0.5f, 0.0f );
		
		BindTexture( SHADER_SAMPLER1, ALPHAMASKTEXTURE, ALPHAMASKTEXTUREFRAME );
	}
	Draw();
	SHADOW_STATE
	{
		pShaderShadow->EnableCustomPixelPipe( false );
	}
}

END_SHADER


