//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//


#include "BaseVSShader.h"

#include "waterdudv_vs11.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER( Water_DuDv, "Help for Water_DuDv" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "", "dudv bump map" )
		SHADER_PARAM( BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )
		SHADER_PARAM( REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "0", "" )
		SHADER_PARAM( REFRACTTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "refraction tint" )
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
		if ( params[BUMPMAP]->IsDefined() )
		{
			LoadTexture( BUMPMAP );
		}
		if( !params[REFRACTTINT]->IsDefined() )
		{
			params[REFRACTTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableAlphaWrites( true );
			pShaderShadow->EnableColorWrites( true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			int fmt = VERTEX_POSITION | VERTEX_NORMAL;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0, 0 );

			pShaderShadow->SetVertexShader( "WaterDuDv_vs11", 0 );
			pShaderShadow->SetPixelShader( "WaterDuDv_ps11", 0 );
			DisableFog();
		}
		DYNAMIC_STATE
		{
			waterdudv_vs11_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			vshIndex.SetDOFOG( pShaderAPI->GetSceneFogMode() != MATERIAL_FOG_NONE );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );

			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, BUMPTRANSFORM );

			Vector4D vec;
			const float *pTint = params[REFRACTTINT]->GetVecValue();
			float flAverage = ( pTint[0] + pTint[1] + pTint[2] ) / 3.0f;
			vec.Init( flAverage, flAverage, flAverage, 1.0f );
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_3, vec.Base() );
	
			// Amount to refract
			SetPixelShaderConstant( 0, REFRACTAMOUNT );

			// Used to renormalize
			vec.Init( 1.0f, 1.0f, 1.0f, 1.0f );
			pShaderAPI->SetPixelShaderConstant( 1, vec.Base() );	

			// Used to deal with the red channel
			vec.Init( 0.0f, 1.0f, 1.0f, 1.0f );
			pShaderAPI->SetPixelShaderConstant( 2, vec.Base() );	

			vec.Init( 1.0f, 0.0f, 0.0f, 0.0f );
			pShaderAPI->SetPixelShaderConstant( 3, vec.Base() );	

			BindTexture( SHADER_TEXTURE_STAGE0, BUMPMAP, BUMPFRAME );
		}
		Draw();
	}
END_SHADER

