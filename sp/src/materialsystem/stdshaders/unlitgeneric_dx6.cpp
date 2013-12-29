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

DEFINE_FALLBACK_SHADER( UnlitGeneric, UnlitGeneric_DX6 )
DEFINE_FALLBACK_SHADER( MonitorScreen, UnlitGeneric_DX6 )
DEFINE_FALLBACK_SHADER( ParticleSphere, UnlitGeneric_DX6 )
DEFINE_FALLBACK_SHADER( Predator, Predator_DX60 )
DEFINE_FALLBACK_SHADER( Predator_DX60, UnlitGeneric_DX6 )
DEFINE_FALLBACK_SHADER( WindowImposter, WindowImposter_DX60 )
DEFINE_FALLBACK_SHADER( WindowImposter_DX60, UnlitGeneric_DX6 )

BEGIN_SHADER( UnlitGeneric_DX6,
			  "Help for UnlitGeneric_DX6" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_envmask", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( ENVMAPMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "envmap mask scale" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( ENVMAPOPTIONAL, SHADER_PARAM_TYPE_BOOL, "0", "Make the envmap only apply to dx9 and higher hardware" )
		SHADER_PARAM( ALPHATESTREFERENCE, SHADER_PARAM_TYPE_FLOAT, "0.7", "" )	
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		if( !params[ENVMAPTINT]->IsDefined() )
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		if( !params[ENVMAPMASKSCALE]->IsDefined() )
			params[ENVMAPMASKSCALE]->SetFloatValue( 1.0f );
		if( !params[DETAILSCALE]->IsDefined() )
			params[DETAILSCALE]->SetFloatValue( 4.0f );

		// No texture means no env mask in base alpha
		if ( !params[BASETEXTURE]->IsDefined() )
		{
			CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		}

		// If in decal mode, no debug override...
		if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
		{
			SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		}

		// Get rid of the envmap if it's optional for this dx level.
		if( params[ENVMAPOPTIONAL]->IsDefined() && params[ENVMAPOPTIONAL]->GetIntValue() )
		{
			params[ENVMAP]->SetUndefined();
		}

		// If mat_specular 0, then get rid of envmap
		if( !g_pConfig->UseSpecular() && params[ENVMAP]->IsDefined() && params[BASETEXTURE]->IsDefined() )
		{
			params[ENVMAP]->SetUndefined();
		}
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE );

			if (!params[BASETEXTURE]->GetTextureValue()->IsTranslucent())
			{
				if (IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK))
					CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
			}
		}

		// the second texture (if there is one)
		if (params[DETAIL]->IsDefined())
		{
			LoadTexture( DETAIL );
		}

		// Don't alpha test if the alpha channel is used for other purposes
		if (IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
			CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );

		if (params[ENVMAP]->IsDefined())
		{
			if( !IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) )
				LoadCubeMap( ENVMAP );
			else
				LoadTexture( ENVMAP );

			if( !g_pHardwareConfig->SupportsCubeMaps() )
				SET_FLAGS(MATERIAL_VAR_ENVMAPSPHERE);

			if (params[ENVMAPMASK]->IsDefined())
			{
				LoadTexture( ENVMAPMASK );
			}
		}
	}

	int GetDrawFlagsPass1(IMaterialVar** params, bool doDetail)
	{
		int flags = SHADER_DRAW_POSITION;
		if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
			flags |= SHADER_DRAW_COLOR;
		if (params[BASETEXTURE]->IsTexture())
			flags |= SHADER_DRAW_TEXCOORD0;
		if (doDetail)
			flags |= SHADER_DRAW_TEXCOORD1;
		return flags;
	}

	void SetDetailShadowState(IShaderShadow* pShaderShadow)
	{
		// Specifically choose overbright2, will cause mod2x here
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
		pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, 2.0f );
	}

	void SetDetailDymamicState(IShaderShadow* pShaderShadow)
	{
		BindTexture( SHADER_SAMPLER1, DETAIL, FRAME );
		SetFixedFunctionTextureScaledTransform( MATERIAL_TEXTURE1, BASETEXTURETRANSFORM, DETAILSCALE );
	}

	void DrawAdditiveNonTextured( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, bool doDetail )
	{
		SHADOW_STATE
		{
			SetModulationShadowState();
			SetAdditiveBlendingShadowState( );
			if (doDetail)
				SetDetailShadowState(pShaderShadow);
			pShaderShadow->DrawFlags( GetDrawFlagsPass1(params, doDetail) );
			FogToBlack();
		}
		DYNAMIC_STATE
		{
			SetModulationDynamicState();
			if (doDetail)
				SetDetailDymamicState(pShaderShadow);
		}
		Draw( );
	}

	void DrawAdditiveTextured( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, bool doDetail )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			SetModulationShadowState();
			SetAdditiveBlendingShadowState( BASETEXTURE, true );
			if (doDetail)
				SetDetailShadowState(pShaderShadow);
			pShaderShadow->DrawFlags( GetDrawFlagsPass1(params, doDetail) );
			FogToBlack();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE0, BASETEXTURETRANSFORM );
			if (doDetail)
				SetDetailDymamicState(pShaderShadow);
			SetModulationDynamicState();
		}
		Draw( );
	}

	void DrawNonTextured( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, bool doDetail )
	{
		SHADOW_STATE
		{
			SetModulationShadowState();
			SetNormalBlendingShadowState( );
			if (doDetail)
				SetDetailShadowState(pShaderShadow);
			pShaderShadow->DrawFlags( GetDrawFlagsPass1(params, doDetail) );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			SetModulationDynamicState();
			if (doDetail)
				SetDetailDymamicState(pShaderShadow);
		}
		Draw( );
	}

	void DrawTextured( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, bool doDetail )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			SetModulationShadowState();
			SetNormalBlendingShadowState( BASETEXTURE, true );
			if (doDetail)
				SetDetailShadowState(pShaderShadow);
			pShaderShadow->DrawFlags( GetDrawFlagsPass1(params, doDetail) );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE0, BASETEXTURETRANSFORM );
			if (doDetail)
				SetDetailDymamicState(pShaderShadow);
			SetModulationDynamicState();
		}
		Draw( );
	}

	SHADER_DRAW
	{
		bool isTextureDefined = params[BASETEXTURE]->IsTexture();
		bool hasVertexColor = IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR);
		bool doFirstPass = isTextureDefined || hasVertexColor || (!params[ENVMAP]->IsTexture());

		if (doFirstPass)
		{
			SHADOW_STATE
			{
 				pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );
				if( params[ALPHATESTREFERENCE]->IsDefined() && params[ALPHATESTREFERENCE]->GetFloatValue() > 0.0f )
				{
					pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GEQUAL, params[ALPHATESTREFERENCE]->GetFloatValue() );
				}
			}

			if (IS_FLAG_SET(MATERIAL_VAR_ADDITIVE))
			{
				if (!isTextureDefined)
				{
					bool hasDetailTexture = params[DETAIL]->IsTexture();
					DrawAdditiveNonTextured( params, pShaderAPI, pShaderShadow, hasDetailTexture );
				}
				else
				{
					// We can't do detail in a single pass if we're also
					// colormodulating and have vertex color
					bool hasDetailTexture = params[DETAIL]->IsTexture();
					bool isModulating = IsColorModulating() || IsAlphaModulating(); 
					bool onePassDetail = hasDetailTexture && (!hasVertexColor || !isModulating);
					DrawAdditiveTextured( params, pShaderAPI, pShaderShadow, onePassDetail );
					if (hasDetailTexture && !onePassDetail)
					{
						FixedFunctionMultiplyByDetailPass(
							BASETEXTURE, FRAME, BASETEXTURETRANSFORM, DETAIL, DETAILSCALE );
					}
				}
			}
			else
			{
				if (!isTextureDefined)
				{
					bool hasDetailTexture = params[DETAIL]->IsTexture();
					DrawNonTextured( params, pShaderAPI, pShaderShadow, hasDetailTexture );
				}
				else
				{
					// We can't do detail in a single pass if we're also
					// colormodulating and have vertex color
					bool hasDetailTexture = params[DETAIL]->IsTexture();
					bool isModulating = IsColorModulating() || IsAlphaModulating(); 
					bool onePassDetail = hasDetailTexture && (!hasVertexColor || !isModulating);
					DrawTextured( params, pShaderAPI, pShaderShadow, onePassDetail );
					if (hasDetailTexture && !onePassDetail)
					{
						FixedFunctionMultiplyByDetailPass(
							BASETEXTURE, FRAME, BASETEXTURETRANSFORM, DETAIL, DETAILSCALE );
					}
				}
			}
		}

		SHADOW_STATE
		{
			// Disable mod2x used by detail
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, 1.0f );
		}

		// Second pass...
		if (params[ENVMAP]->IsTexture() && 
			(!doFirstPass || IS_FLAG_SET(MATERIAL_VAR_MULTIPASS)) )
		{
			if (doFirstPass || IS_FLAG_SET(MATERIAL_VAR_ADDITIVE))
			{
				FixedFunctionAdditiveMaskedEnvmapPass( ENVMAP, ENVMAPMASK, BASETEXTURE,
					ENVMAPFRAME, ENVMAPMASKFRAME, FRAME, 
					BASETEXTURETRANSFORM, ENVMAPMASKSCALE, ENVMAPTINT );
			}
			else
			{
				FixedFunctionMaskedEnvmapPass( ENVMAP, ENVMAPMASK, BASETEXTURE,
					ENVMAPFRAME, ENVMAPMASKFRAME, FRAME, 
					BASETEXTURETRANSFORM, ENVMAPMASKSCALE, ENVMAPTINT );
			}
		}
	}
END_SHADER
