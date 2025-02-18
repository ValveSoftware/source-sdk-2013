//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "convar.h"
#include "d3dx.h"

DEFINE_FALLBACK_SHADER( PortalStaticOverlay, PortalStaticOverlay_DX60 );

BEGIN_VS_SHADER( PortalStaticOverlay_DX60, 
				"Help for PortalStaticOverlay_DX60 shader" )

				BEGIN_SHADER_PARAMS
				SHADER_PARAM_OVERRIDE( COLOR, SHADER_PARAM_TYPE_COLOR, "{255 255 255}", "unused", SHADER_PARAM_NOT_EDITABLE )
				SHADER_PARAM_OVERRIDE( ALPHA, SHADER_PARAM_TYPE_FLOAT, "1.0", "unused", SHADER_PARAM_NOT_EDITABLE )
				SHADER_PARAM( STATICAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0.0", "Amount of the static blend texture to blend into the base texture" )
				SHADER_PARAM( STATICBLENDTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "When adding static, this is the texture that gets blended in" )
				SHADER_PARAM( STATICBLENDTEXTUREFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
				SHADER_PARAM( ALPHAMASKTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "An alpha mask for odd shaped portals" )
				SHADER_PARAM( ALPHAMASKTEXTUREFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
				SHADER_PARAM( NOCOLORWRITE, SHADER_PARAM_TYPE_INTEGER, "0", "" )
				END_SHADER_PARAMS


SHADER_INIT_PARAMS()
{
	SET_FLAGS( MATERIAL_VAR_TRANSLUCENT );
}

SHADER_FALLBACK
{
	return 0;
}


SHADER_INIT
{
	if( params[STATICBLENDTEXTURE]->IsDefined() )
		LoadTexture( STATICBLENDTEXTURE );
	if( params[ALPHAMASKTEXTURE]->IsDefined() )
		LoadTexture( ALPHAMASKTEXTURE );

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

	if( !params[NOCOLORWRITE]->IsDefined() )
		params[NOCOLORWRITE]->SetIntValue( 0 );
}

SHADER_DRAW
{
	SHADOW_STATE
	{
		SetInitialShadowState();
		FogToFogColor();

		//pShaderShadow->EnablePolyOffset( SHADER_POLYOFFSET_DECAL ); //a portal is effectively a decal on top of a wall
		pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_NEAREROREQUAL );

		pShaderShadow->EnableDepthWrites( true );

		pShaderShadow->EnableAlphaTest( true );
		pShaderShadow->EnableColorWrites( params[NOCOLORWRITE]->GetIntValue() == 0 );
	}
	DYNAMIC_STATE
	{
		pShaderAPI->SetDefaultState();
	}
	
	if( params[ALPHAMASKTEXTURE]->IsTexture() )
		StaticPass_WithAlphaMask( pShaderShadow, pShaderAPI, params ); //portal static texture blending, with an alpha mask
	else
		StaticPass_NoAlphaMask( pShaderShadow, pShaderAPI, params ); //portal static texture blending
}

void StaticPass_NoAlphaMask( IShaderShadow *pShaderShadow, IShaderDynamicAPI *pShaderAPI, IMaterialVar **params )
{
	SHADOW_STATE
	{
		pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 );

		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

		pShaderShadow->EnableBlending( true );
		pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );

		pShaderShadow->EnableCustomPixelPipe( true );
		pShaderShadow->CustomTextureStages( 1 );

		pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
			SHADER_TEXCHANNEL_COLOR, 
			SHADER_TEXOP_SELECTARG1,
			SHADER_TEXARG_TEXTURE, SHADER_TEXARG_TEXTURE );

		pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
			SHADER_TEXCHANNEL_ALPHA, 
			SHADER_TEXOP_BLEND_CONSTANTALPHA,
			SHADER_TEXARG_ZERO, SHADER_TEXARG_ONE );
	}
	DYNAMIC_STATE
	{
		BindTexture( SHADER_SAMPLER0, STATICBLENDTEXTURE, STATICBLENDTEXTUREFRAME );
		pShaderAPI->Color4f( 0.0f, 0.0f, 0.0f, params[STATICAMOUNT]->GetFloatValue() );
	}
	Draw();
	SHADOW_STATE
	{
		pShaderShadow->EnableCustomPixelPipe( false );
	}
}


void StaticPass_WithAlphaMask( IShaderShadow *pShaderShadow, IShaderDynamicAPI *pShaderAPI, IMaterialVar **params )
{
	SHADOW_STATE
	{
		pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_TEXCOORD1 );

		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

		pShaderShadow->EnableBlending( true );
		pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );

		pShaderShadow->EnableCustomPixelPipe( true );
		pShaderShadow->CustomTextureStages( 2 );

		//portal static
		pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
			SHADER_TEXCHANNEL_COLOR, 
			SHADER_TEXOP_SELECTARG1,
			SHADER_TEXARG_TEXTURE, SHADER_TEXARG_TEXTURE );

		pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
			SHADER_TEXCHANNEL_ALPHA, 
			SHADER_TEXOP_BLEND_CONSTANTALPHA,
			SHADER_TEXARG_ZERO, SHADER_TEXARG_ONE );


		//alpha mask
		pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
			SHADER_TEXCHANNEL_COLOR, 
			SHADER_TEXOP_SELECTARG1,
			SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_PREVIOUSSTAGE );

		pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
			SHADER_TEXCHANNEL_ALPHA, 
			SHADER_TEXOP_MODULATE,
			SHADER_TEXARG_TEXTUREALPHA, SHADER_TEXARG_PREVIOUSSTAGE );
	}
	DYNAMIC_STATE
	{		
		BindTexture( SHADER_SAMPLER0, STATICBLENDTEXTURE, STATICBLENDTEXTUREFRAME );
		BindTexture( SHADER_SAMPLER1, ALPHAMASKTEXTURE, ALPHAMASKTEXTUREFRAME );

		pShaderAPI->Color4f( 0.0f, 0.0f, 0.0f, params[STATICAMOUNT]->GetFloatValue() );
	}
	Draw();
	SHADOW_STATE
	{
		pShaderShadow->EnableCustomPixelPipe( false );
	}
}

END_SHADER


