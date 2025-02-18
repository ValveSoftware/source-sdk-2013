//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "BaseVSShader.h"

#include "teeth.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( Teeth, Teeth_DX8 )

BEGIN_VS_SHADER( Teeth_DX8, "Help for Teeth_DX8" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( ILLUMFACTOR, SHADER_PARAM_TYPE_FLOAT, "1", "Amount to darken or brighten the teeth" )
		SHADER_PARAM( FORWARD, SHADER_PARAM_TYPE_VEC3, "[1 0 0]", "Forward direction vector for teeth lighting" )
		SHADER_PARAM( INTRO, SHADER_PARAM_TYPE_BOOL, "0", "is teeth in the ep1 intro" )
 	    SHADER_PARAM( ENTITYORIGIN, SHADER_PARAM_TYPE_VEC3,"0.0","center if the model in world space" )
 	    SHADER_PARAM( WARPPARAM, SHADER_PARAM_TYPE_FLOAT,"0.0","animation param between 0 and 1" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		// FLASHLIGHTFIXME
		params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight001" );

		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );

		if( !params[INTRO]->IsDefined() )
		{
			params[INTRO]->SetIntValue( 0 );
		}
	}

	SHADER_FALLBACK
	{
		if ( IsPC() && ( g_pHardwareConfig->GetDXSupportLevel() < 80 || g_pConfig->bSoftwareLighting ) )
		{
			return "Teeth_dx6";
		}
		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( FLASHLIGHTTEXTURE );
		LoadTexture( BASETEXTURE );
	}

	void DrawUsingVertexShader( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->VertexShaderVertexFormat( 
				VERTEX_POSITION | VERTEX_NORMAL, 1, 0, 0 );
			teeth_Static_Index vshIndex;
			vshIndex.SetHALF_LAMBERT( IS_FLAG_SET( MATERIAL_VAR_HALFLAMBERT ) );
			vshIndex.SetINTRO( params[INTRO]->GetIntValue() != 0 );
			pShaderShadow->SetVertexShader( "Teeth", vshIndex.GetIndex() );
			pShaderShadow->SetPixelShader( "VertexLitTexture_Overbright2" );

			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			SetAmbientCubeDynamicStateVertexShader();

			Vector4D lighting;
			params[FORWARD]->GetVecValue( lighting.Base(), 3 );
			lighting[3] = params[ILLUMFACTOR]->GetFloatValue();
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, lighting.Base() );

			teeth_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			vshIndex.SetSKINNING( pShaderAPI->GetCurrentNumBones() > 0 );
			vshIndex.SetLIGHT_COMBO( pShaderAPI->GetCurrentLightCombo() );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );

			if( params[INTRO]->GetIntValue() )
			{
				float curTime = params[WARPPARAM]->GetFloatValue();
				float timeVec[4] = { 0.0f, 0.0f, 0.0f, curTime };
				Assert( params[ENTITYORIGIN]->IsDefined() );
				params[ENTITYORIGIN]->GetVecValue( timeVec, 3 );
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, timeVec, 1 );
			}
		}
		Draw();
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );
		}
		bool hasFlashlight = UsingFlashlight( params );
		if( hasFlashlight )
		{
			DrawFlashlight_dx80( params, pShaderAPI, pShaderShadow, false, -1, -1, -1,
								 FLASHLIGHTTEXTURE, FLASHLIGHTTEXTUREFRAME, false, false, 0, -1, -1,
								 // Optional parameters, specific to teeth:
								 true, FORWARD, ILLUMFACTOR );
		}
		else
		{
			DrawUsingVertexShader( params, pShaderAPI, pShaderShadow );
		}
	}
END_SHADER
