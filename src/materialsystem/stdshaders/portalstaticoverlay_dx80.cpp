//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "convar.h"
#include "portalstaticoverlay_vs11.inc"
#include "portalstaticoverlay_ps11.inc"

BEGIN_VS_SHADER( PortalStaticOverlay, 
				"Help for PortalStaticOverlay shader" )

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
	if( !g_pHardwareConfig->SupportsVertexAndPixelShaders() )
		return "PortalStaticOverlay_DX60";

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
	bool bStaticBlendTexture = params[STATICBLENDTEXTURE]->IsTexture();
	bool bAlphaMaskTexture = params[ALPHAMASKTEXTURE]->IsTexture();

	bool bIsModel = IS_FLAG_SET( MATERIAL_VAR_MODEL );

	SHADOW_STATE
	{
		SetInitialShadowState();
		FogToFogColor();

		//pShaderShadow->EnablePolyOffset( SHADER_POLYOFFSET_DECAL ); //a portal is effectively a decal on top of a wall
		pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_NEAREROREQUAL );

		pShaderShadow->EnableDepthWrites( true );

		pShaderShadow->EnableBlending( true );
		pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );

		pShaderShadow->EnableAlphaTest( true );
		pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GREATER, 0.0f );

		pShaderShadow->EnableColorWrites( params[NOCOLORWRITE]->GetIntValue() == 0 );

		if( bStaticBlendTexture || bAlphaMaskTexture )
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		if( bStaticBlendTexture && bAlphaMaskTexture )
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

		int fmt = VERTEX_POSITION | VERTEX_NORMAL;
		int userDataSize = 0;
		if( bIsModel )
		{
			userDataSize = 4;
		}
		else
		{
			fmt |= VERTEX_TANGENT_S | VERTEX_TANGENT_T;
		}
		pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, userDataSize );

		DECLARE_STATIC_VERTEX_SHADER( portalstaticoverlay_vs11 );
		SET_STATIC_VERTEX_SHADER_COMBO( MODEL,  bIsModel );
		SET_STATIC_VERTEX_SHADER( portalstaticoverlay_vs11 );

		DECLARE_STATIC_PIXEL_SHADER( portalstaticoverlay_ps11 );
		SET_STATIC_PIXEL_SHADER_COMBO( HASALPHAMASK, bAlphaMaskTexture );
		SET_STATIC_PIXEL_SHADER_COMBO( HASSTATICTEXTURE, bStaticBlendTexture );
		SET_STATIC_PIXEL_SHADER( portalstaticoverlay_ps11 );
	}
	DYNAMIC_STATE
	{
		pShaderAPI->SetDefaultState();

		float fStaticAmount = params[STATICAMOUNT]->GetFloatValue();

		//x is static, y is inverse static
		float pc0[4] = { fStaticAmount, 1.0f - fStaticAmount, 0.0f, 0.0f };
		pShaderAPI->SetPixelShaderConstant( 0, pc0 );
		
		if( bStaticBlendTexture )
		{
			BindTexture( SHADER_SAMPLER0, STATICBLENDTEXTURE, STATICBLENDTEXTUREFRAME );
			if( bAlphaMaskTexture )
				BindTexture( SHADER_SAMPLER1, ALPHAMASKTEXTURE, ALPHAMASKTEXTUREFRAME );
		}
		else if( bAlphaMaskTexture )
		{
			BindTexture( SHADER_SAMPLER0, ALPHAMASKTEXTURE, ALPHAMASKTEXTUREFRAME );
		}

		DECLARE_DYNAMIC_VERTEX_SHADER( portalstaticoverlay_vs11 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
		SET_DYNAMIC_VERTEX_SHADER( portalstaticoverlay_vs11 );

		DECLARE_DYNAMIC_PIXEL_SHADER( portalstaticoverlay_ps11 );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( HDRENABLED, IsHDREnabled() );
		SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
		SET_DYNAMIC_PIXEL_SHADER( portalstaticoverlay_ps11 );
	}

	Draw();
}

END_SHADER


