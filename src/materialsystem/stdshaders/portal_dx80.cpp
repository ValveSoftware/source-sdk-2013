//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "portal_vs11.inc"
#include "portal_ps11.inc"
#include "convar.h"


DEFINE_FALLBACK_SHADER( Portal, Portal_DX80 )

BEGIN_VS_SHADER( Portal_DX80, 
				"Help for Portal shader" )

				BEGIN_SHADER_PARAMS
				SHADER_PARAM_OVERRIDE( COLOR, SHADER_PARAM_TYPE_COLOR, "{255 255 255}", "unused", SHADER_PARAM_NOT_EDITABLE )
				SHADER_PARAM_OVERRIDE( ALPHA, SHADER_PARAM_TYPE_FLOAT, "1.0", "unused", SHADER_PARAM_NOT_EDITABLE )
				SHADER_PARAM( STATICAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0.0", "Amount of the static blend texture to blend into the base texture" )
				SHADER_PARAM( STATICBLENDTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "When adding static, this is the texture that gets blended in" )
				SHADER_PARAM( STATICBLENDTEXTUREFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
				SHADER_PARAM( ALPHAMASKTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "An alpha mask for odd shaped portals" )
				SHADER_PARAM( ALPHAMASKTEXTUREFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
				SHADER_PARAM( RENDERFIXZ, SHADER_PARAM_TYPE_INTEGER, "0", "Special depth handling, intended for rendering bug workarounds for extremely close polygons" )
				SHADER_PARAM( USEALTERNATEVIEWMATRIX, SHADER_PARAM_TYPE_INTEGER, "1", "Use the alternate view matrix instead of the current view matrix" )
				SHADER_PARAM( ALTERNATEVIEWMATRIX, SHADER_PARAM_TYPE_MATRIX, "0", "The alternate view matrix to use when $usealternateviewmatrix is enabled" )
				END_SHADER_PARAMS

SHADER_INIT_PARAMS()
{
	SET_FLAGS( MATERIAL_VAR_TRANSLUCENT );
	if( !params[BASETEXTURE]->IsDefined() )
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_POWER_OF_TWO_FRAME_BUFFER_TEXTURE );
	}
}

SHADER_FALLBACK
{
	if( !g_pHardwareConfig->SupportsVertexAndPixelShaders() )
		return "Portal_DX60";

	return 0;
}

int iMaxTextureStages;

SHADER_INIT
{
	iMaxTextureStages = min( 3, g_pHardwareConfig->GetSamplerCount() );

	if (params[BASETEXTURE]->IsDefined() )
		LoadTexture( BASETEXTURE );
	if( params[STATICBLENDTEXTURE]->IsDefined() )
		LoadTexture( STATICBLENDTEXTURE );	
	if( (iMaxTextureStages > 1) && params[ALPHAMASKTEXTURE]->IsDefined() )
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

	if ( !params[USEALTERNATEVIEWMATRIX]->IsDefined() )
		params[USEALTERNATEVIEWMATRIX]->SetIntValue( 0 );

	if ( !params[ALTERNATEVIEWMATRIX]->IsDefined() )
	{
		VMatrix matIdentity;
		matIdentity.Identity();
		params[ALTERNATEVIEWMATRIX]->SetMatrixValue( matIdentity );
	}
}

SHADER_DRAW
{
	bool bStaticBlendTexture = params[STATICBLENDTEXTURE]->IsTexture();
	bool bAlphaMaskTexture = ((iMaxTextureStages > 1) && (params[ALPHAMASKTEXTURE]->IsTexture()))?(1):(0); //must support at least 2 texture stages to use any kind of mask
	
	float fStaticAmount = params[STATICAMOUNT]->GetFloatValue();
	
	SHADOW_STATE
	{
		SetInitialShadowState();
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

		pShaderShadow->EnableAlphaTest( true );

		if( ((iMaxTextureStages < 3) && bStaticBlendTexture) || bAlphaMaskTexture ) //in multipass modes, we need alpha to mix portal static, other
		{
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
		}
		else
		{
			pShaderShadow->EnableBlending( false );
		}

		int fmt = VERTEX_POSITION | VERTEX_NORMAL;
		int userDataSize = 0;
		int	iTexCoords = 1;
		if( IS_FLAG_SET( MATERIAL_VAR_MODEL ) )
		{
			userDataSize = 4;		
		}
		else
		{
			fmt |= VERTEX_TANGENT_S | VERTEX_TANGENT_T;
		}
		pShaderShadow->VertexShaderVertexFormat( fmt, iTexCoords, NULL, userDataSize );

		DECLARE_STATIC_VERTEX_SHADER( portal_vs11 );
		SET_STATIC_VERTEX_SHADER_COMBO( MAXTEXTURESTAGES, (iMaxTextureStages - 1) );
		SET_STATIC_VERTEX_SHADER_COMBO( HASALPHAMASK, bAlphaMaskTexture );
		SET_STATIC_VERTEX_SHADER_COMBO( HASSTATICTEXTURE, bStaticBlendTexture );
		SET_STATIC_VERTEX_SHADER_COMBO( USEALTERNATEVIEW, (params[USEALTERNATEVIEWMATRIX]->GetIntValue() != 0) );
		SET_STATIC_VERTEX_SHADER( portal_vs11 );

		DECLARE_STATIC_PIXEL_SHADER( portal_ps11 );
		SET_STATIC_PIXEL_SHADER_COMBO( MAXTEXTURESTAGES, (iMaxTextureStages - 1) );
		SET_STATIC_PIXEL_SHADER_COMBO( HASALPHAMASK, bAlphaMaskTexture );
		SET_STATIC_PIXEL_SHADER_COMBO( HASSTATICTEXTURE, bStaticBlendTexture );
		SET_STATIC_PIXEL_SHADER( portal_ps11 );

	}
	DYNAMIC_STATE
	{
		pShaderAPI->SetDefaultState();

		//x is static, y is inverse static
		float pc0[4] = { fStaticAmount, 1.0f - fStaticAmount, 0.0f, 0.0f };
		pShaderAPI->SetPixelShaderConstant( 0, pc0 );

		if ( params[USEALTERNATEVIEWMATRIX]->GetIntValue() != 0 )
		{
			const VMatrix &matCustomView = params[ALTERNATEVIEWMATRIX]->GetMatrixValue();

			VMatrix matProj;
			pShaderAPI->GetMatrix( MATERIAL_PROJECTION, matProj.Base() );
			MatrixTranspose( matProj, matProj );

			VMatrix matFinal;
			MatrixMultiply( matProj, matCustomView, matFinal );
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, matFinal.Base(), 4 );
		}
	}


	if( iMaxTextureStages == 3 )
	{
		SinglePass( pShaderShadow, pShaderAPI, params, fStaticAmount, bStaticBlendTexture, bAlphaMaskTexture );
	}
	else
	{
		MultiPass_CutoutPass( pShaderShadow, pShaderAPI, params, bAlphaMaskTexture );
		if( bStaticBlendTexture ) //if we don't have a static texture, the cutout pass will mix in gray for any static amount
			MultiPass_StaticPass( pShaderShadow, pShaderAPI, params, bAlphaMaskTexture );
	}
}

void SinglePass( IShaderShadow *pShaderShadow, IShaderDynamicAPI *pShaderAPI, IMaterialVar **params, float fStaticAmount, bool bStaticBlendTexture, bool bAlphaMaskTexture )
{
	SHADOW_STATE
	{		
		// source render target that contains the image that we are warping.
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		
		if( bStaticBlendTexture || bAlphaMaskTexture )
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

		if( bStaticBlendTexture && bAlphaMaskTexture )
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );

	}
	DYNAMIC_STATE
	{
		if ( params[BASETEXTURE]->IsTexture() )
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
		else
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0 );

		bool bHasStatic = (fStaticAmount > 0.0f);
		bool bUsingStaticTexture = (bStaticBlendTexture && bHasStatic);

		if ( bAlphaMaskTexture )
		{
			BindTexture( SHADER_SAMPLER1, ALPHAMASKTEXTURE, ALPHAMASKTEXTUREFRAME );
			if ( bUsingStaticTexture )
				BindTexture( SHADER_SAMPLER2, STATICBLENDTEXTURE, STATICBLENDTEXTUREFRAME );
		}
		else
		{
			if ( bUsingStaticTexture )
				BindTexture( SHADER_SAMPLER1, STATICBLENDTEXTURE, STATICBLENDTEXTUREFRAME );
		}

		int iHasStatic = bHasStatic?1:0;

		DECLARE_DYNAMIC_VERTEX_SHADER( portal_vs11 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( ADDSTATIC, iHasStatic );
		SET_DYNAMIC_VERTEX_SHADER( portal_vs11 );

		DECLARE_DYNAMIC_PIXEL_SHADER( portal_ps11 );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( ADDSTATIC, iHasStatic );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
		SET_DYNAMIC_PIXEL_SHADER( portal_ps11 );
	}
	Draw();
}


void MultiPass_CutoutPass( IShaderShadow *pShaderShadow, IShaderDynamicAPI *pShaderAPI, IMaterialVar **params, bool bAlphaMaskTexture )
{
	//the basic portal effect
	SHADOW_STATE
	{
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

		if( bAlphaMaskTexture )
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
	}
	DYNAMIC_STATE
	{
		if ( params[BASETEXTURE]->IsTexture() )
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
		else
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0 );

		if( bAlphaMaskTexture )
			BindTexture( SHADER_SAMPLER1, ALPHAMASKTEXTURE, ALPHAMASKTEXTUREFRAME );

		DECLARE_DYNAMIC_VERTEX_SHADER( portal_vs11 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( ADDSTATIC, 0 );
		SET_DYNAMIC_VERTEX_SHADER( portal_vs11 );

		DECLARE_DYNAMIC_PIXEL_SHADER( portal_ps11 );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( ADDSTATIC, 0 );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
		SET_DYNAMIC_PIXEL_SHADER( portal_ps11 );
	}
	Draw();
}


void MultiPass_StaticPass( IShaderShadow *pShaderShadow, IShaderDynamicAPI *pShaderAPI, IMaterialVar **params, bool bAlphaMaskTexture )
{
	SHADOW_STATE
	{
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		if( bAlphaMaskTexture )
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

	}
	DYNAMIC_STATE
	{
		BindTexture( SHADER_SAMPLER0, STATICBLENDTEXTURE, STATICBLENDTEXTUREFRAME );
		if( bAlphaMaskTexture )
			BindTexture( SHADER_SAMPLER1, ALPHAMASKTEXTURE, ALPHAMASKTEXTUREFRAME );

		DECLARE_DYNAMIC_VERTEX_SHADER( portal_vs11 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( ADDSTATIC, 1 );
		SET_DYNAMIC_VERTEX_SHADER( portal_vs11 );

		DECLARE_DYNAMIC_PIXEL_SHADER( portal_ps11 );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( ADDSTATIC, 1 );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
		SET_DYNAMIC_PIXEL_SHADER( portal_ps11 );
	}
	Draw();
}

END_SHADER


