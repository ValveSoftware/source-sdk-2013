//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

//Note: Not upgraded to vs/ps 2.0 fxc's because this shader is unused and there are no test cases to verify against.
#include "BaseVSShader.h"

#if !defined( _X360 )
#include "shadowmodel_ps20.inc"
#include "shadowmodel_vs20.inc"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( ShadowModel, ShadowModel_DX9 )


#if !defined( _X360 ) //not used for anything at time of 360 ship, and we want to avoid storing/loading assembly shaders

//PC version
BEGIN_VS_SHADER_FLAGS( ShadowModel_DX9, "Help for ShadowModel", SHADER_NOT_EDITABLE )

BEGIN_SHADER_PARAMS
SHADER_PARAM( BASETEXTUREOFFSET, SHADER_PARAM_TYPE_VEC2, "[0 0]", "$baseTexture texcoord offset" )
SHADER_PARAM( BASETEXTURESCALE, SHADER_PARAM_TYPE_VEC2, "[1 1]", "$baseTexture texcoord scale" )
SHADER_PARAM( FALLOFFOFFSET, SHADER_PARAM_TYPE_FLOAT, "0", "Distance at which shadow starts to fade" )
SHADER_PARAM( FALLOFFDISTANCE, SHADER_PARAM_TYPE_FLOAT, "100", "Max shadow distance" )
SHADER_PARAM( FALLOFFAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0.9", "Amount to brighten the shadow at max dist" )
END_SHADER_PARAMS

SHADER_INIT_PARAMS()
{
	if (!params[BASETEXTURESCALE]->IsDefined())
	{
		Vector2D scale(1, 1);
		params[BASETEXTURESCALE]->SetVecValue( scale.Base(), 2 );
	}

	if (!params[FALLOFFDISTANCE]->IsDefined())
		params[FALLOFFDISTANCE]->SetFloatValue( 100.0f );

	if (!params[FALLOFFAMOUNT]->IsDefined())
		params[FALLOFFAMOUNT]->SetFloatValue( 0.9f );
}

SHADER_FALLBACK
{
	if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		return "ShadowModel_DX8";

	return 0;
}

SHADER_INIT
{
	if (params[BASETEXTURE]->IsDefined())
	{
		LoadTexture( BASETEXTURE );
	}
}

SHADER_DRAW
{
	SHADOW_STATE
	{
		// Base texture on stage 0
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

		// Multiplicative blending state...
		EnableAlphaBlending( SHADER_BLEND_DST_COLOR, SHADER_BLEND_ZERO );

		int fmt = VERTEX_POSITION | VERTEX_NORMAL;
		pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

		DECLARE_STATIC_VERTEX_SHADER( shadowmodel_vs20 );
		SET_STATIC_VERTEX_SHADER( shadowmodel_vs20 );

		DECLARE_STATIC_PIXEL_SHADER( shadowmodel_ps20 );
		SET_STATIC_PIXEL_SHADER( shadowmodel_ps20 );

		// We need to fog to *white* regardless of overbrighting...
		FogToWhite();
	}
	DYNAMIC_STATE
	{
		BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
		SetVertexShaderMatrix3x4( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );

		SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_3, BASETEXTUREOFFSET );
		SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, BASETEXTURESCALE );

		Vector4D shadow;
		shadow[0] = params[FALLOFFOFFSET]->GetFloatValue();
		shadow[1] = params[FALLOFFDISTANCE]->GetFloatValue() + shadow[0];
		if (shadow[1] != 0.0f)
			shadow[1] = 1.0f / shadow[1];
		shadow[2] = params[FALLOFFAMOUNT]->GetFloatValue();
		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_5, shadow.Base(), 1 );

		// The constant color is the shadow color...
		SetModulationVertexShaderDynamicState();

		DECLARE_DYNAMIC_VERTEX_SHADER( shadowmodel_vs20 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
		SET_DYNAMIC_VERTEX_SHADER( shadowmodel_vs20 );

		DECLARE_DYNAMIC_PIXEL_SHADER( shadowmodel_ps20 );
		SET_DYNAMIC_PIXEL_SHADER( shadowmodel_ps20 );
	}
	Draw( );
}
END_SHADER


#else

//360 version

BEGIN_VS_SHADER_FLAGS( ShadowModel_DX9, "Help for ShadowModel", SHADER_NOT_EDITABLE )

BEGIN_SHADER_PARAMS
SHADER_PARAM( BASETEXTUREOFFSET, SHADER_PARAM_TYPE_VEC2, "[0 0]", "$baseTexture texcoord offset" )
SHADER_PARAM( BASETEXTURESCALE, SHADER_PARAM_TYPE_VEC2, "[1 1]", "$baseTexture texcoord scale" )
SHADER_PARAM( FALLOFFOFFSET, SHADER_PARAM_TYPE_FLOAT, "0", "Distance at which shadow starts to fade" )
SHADER_PARAM( FALLOFFDISTANCE, SHADER_PARAM_TYPE_FLOAT, "100", "Max shadow distance" )
SHADER_PARAM( FALLOFFAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0.9", "Amount to brighten the shadow at max dist" )
END_SHADER_PARAMS

SHADER_INIT_PARAMS()
{
}

SHADER_FALLBACK
{
	return 0;
}

SHADER_INIT
{
}

SHADER_DRAW
{
	Draw( false );
}
END_SHADER

#endif