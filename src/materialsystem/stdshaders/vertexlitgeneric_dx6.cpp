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

DEFINE_FALLBACK_SHADER( VertexLitGeneric, VertexLitGeneric_DX6 )

BEGIN_SHADER( VertexLitGeneric_DX6, 
			  "Help for VertexLitGeneric_DX6" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_envmask", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "envmap mask scale" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( ENVMAPOPTIONAL, SHADER_PARAM_TYPE_BOOL, "0", "Make the envmap only apply to dx9 and higher hardware" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );

		if( !params[ENVMAPMASKSCALE]->IsDefined() )
			params[ENVMAPMASKSCALE]->SetFloatValue( 1.0f );

		if( !params[ENVMAPTINT]->IsDefined() )
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );

		if( !params[SELFILLUMTINT]->IsDefined() )
			params[SELFILLUMTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );

		if( !params[DETAILSCALE]->IsDefined() )
			params[DETAILSCALE]->SetFloatValue( 4.0f );

		// No envmap uses mode 0, it's one less pass
		// Also, if multipass = 0, then go to mode 0 also
		if ( ( !params[ENVMAP]->IsDefined() ) ||
			 ( !IS_FLAG_SET(MATERIAL_VAR_MULTIPASS) ) )
		{
			CLEAR_FLAGS( MATERIAL_VAR_ENVMAPMODE );
		}

		// Vertex color requires mode 1
		if ( IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR) )
		{
			SET_FLAGS( MATERIAL_VAR_ENVMAPMODE );
		}

		// No texture means no self-illum or env mask in base alpha
		if ( !params[BASETEXTURE]->IsDefined() )
		{
			CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
			CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		}

		// If in decal mode, no debug override...
		if ( IS_FLAG_SET(MATERIAL_VAR_DECAL) )
		{
			SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		}

		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_SOFTWARE_LIGHTING );

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

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE );

			if (!params[BASETEXTURE]->GetTextureValue()->IsTranslucent())
			{
				CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
				CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
			}
		}

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
			{
				SET_FLAGS( MATERIAL_VAR_ENVMAPSPHERE );
			}

			if (params[ENVMAPMASK]->IsDefined())
				LoadTexture( ENVMAPMASK );
		}
	}

	int GetDrawFlagsPass1(IMaterialVar** params)
	{
		int flags = SHADER_DRAW_POSITION | SHADER_DRAW_COLOR;
		if (params[BASETEXTURE]->IsTexture())
			flags |= SHADER_DRAW_TEXCOORD0;
		return flags;
	}

	void DrawVertexLightingOnly( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, false );

			SetModulationShadowState();
			SetDefaultBlendingShadowState( );
			pShaderShadow->DrawFlags( GetDrawFlagsPass1( params ) );
			DefaultFog();
		}
		DYNAMIC_STATE
		{
			SetModulationDynamicState();
		}
		Draw();
	}

	void MultiplyByVertexLighting( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			// FIXME: How to deal with texture alpha??

			pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE0, false );
			pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE1, false );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, false );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, false );

			// NOTE: We're not doing lightmapping here, but we want to use the
			// same blend mode as we used for lightmapping
			pShaderShadow->EnableBlending( true );
			SingleTextureLightmapBlendMode();
			
			pShaderShadow->EnableCustomPixelPipe( true );
			pShaderShadow->CustomTextureStages( 1 );

			// This here will perform color = vertex light * (cc alpha) + 1 * (1 - cc alpha)
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_BLEND_CONSTANTALPHA, 
				SHADER_TEXARG_VERTEXCOLOR, SHADER_TEXARG_CONSTANTCOLOR );

			// Alpha isn't used, it doesn't matter what we set it to.
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1, 
				SHADER_TEXARG_NONE, SHADER_TEXARG_NONE );

			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_COLOR );
			FogToOOOverbright();
		}
		DYNAMIC_STATE
		{
			// Put the alpha in the color channel to modulate the color down....
			float alpha = GetAlpha();
			pShaderAPI->Color4f( OO_OVERBRIGHT, OO_OVERBRIGHT, OO_OVERBRIGHT, alpha );
		}
		Draw();

		SHADOW_STATE
		{
			pShaderShadow->EnableCustomPixelPipe( false );
		}
	}


	//-----------------------------------------------------------------------------
	// Used by mode 1
	//-----------------------------------------------------------------------------

	void DrawBaseTimesVertexLighting( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		// Base times vertex lighting, no vertex color
		SHADOW_STATE
		{
			// alpha test
 			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			// base
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE0, OVERBRIGHT );

			// Independenly configure alpha and color

			// Color = Color mod * Vertex Light * Tex (x2)
			// Alpha = Constant Alpha * Tex Alpha (no tex alpha if self illum == 1)
			// Can't have color modulation here
			pShaderShadow->EnableConstantColor( IsColorModulating() );

			// Independenly configure alpha and color
			pShaderShadow->EnableAlphaPipe( true );
			pShaderShadow->EnableConstantAlpha( IsAlphaModulating() );
			pShaderShadow->EnableVertexAlpha( IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA) );

			if (!IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) && !IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK))
				pShaderShadow->EnableTextureAlpha( SHADER_TEXTURE_STAGE0, true );

			SetDefaultBlendingShadowState( BASETEXTURE, true );
			pShaderShadow->DrawFlags( GetDrawFlagsPass1( params ) );
			DefaultFog();
		}
		DYNAMIC_STATE
		{
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE0, BASETEXTURETRANSFORM );
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			SetModulationDynamicState();
		}
		Draw();

		SHADOW_STATE
		{
			pShaderShadow->EnableAlphaPipe( false );
		}
	}

	//-----------------------------------------------------------------------------
	// Envmap times vertex lighting, no vertex color
	//-----------------------------------------------------------------------------

	void DrawEnvmapTimesVertexLighting( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			int materialVarFlags = params[FLAGS]->GetIntValue();

			// alpha test
 			pShaderShadow->EnableAlphaTest( false );

			int flags = SetShadowEnvMappingState( ENVMAPMASK ) | SHADER_DRAW_COLOR;
			bool hasEnvMapMask = params[ENVMAPMASK]->IsTexture();

			pShaderShadow->OverbrightValue( hasEnvMapMask ? 
				SHADER_TEXTURE_STAGE1 : SHADER_TEXTURE_STAGE0, OVERBRIGHT );

			// Independenly configure alpha and color

			// Color = Env map * Vertex Light * Envmapmask (x2)
			// Alpha = Constant Alpha * Vertex light alpha * Env Map mask Alpha
			pShaderShadow->EnableConstantColor( IsColorModulating() );

			pShaderShadow->EnableAlphaPipe( true );
			pShaderShadow->EnableConstantAlpha( IsAlphaModulating() );
			pShaderShadow->EnableVertexAlpha( (materialVarFlags & MATERIAL_VAR_VERTEXALPHA) != 0 );
			if (hasEnvMapMask)
				pShaderShadow->EnableTextureAlpha( SHADER_TEXTURE_STAGE1, true );

			SetDefaultBlendingShadowState( BASETEXTURE, true );

			pShaderShadow->DrawFlags( flags );
			DefaultFog();
		}
		DYNAMIC_STATE
		{
			SetDynamicEnvMappingState( ENVMAP, ENVMAPMASK, BASETEXTURE,
				ENVMAPFRAME, ENVMAPMASKFRAME, FRAME, 
				BASETEXTURETRANSFORM, ENVMAPMASKSCALE );
		}
		Draw();

		SHADOW_STATE
		{
			pShaderShadow->EnableCustomPixelPipe( false );
			pShaderShadow->EnableAlphaPipe( false );
		}
	}

	void DrawMode1( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		bool texDefined = params[BASETEXTURE]->IsTexture();
		bool envDefined = params[ENVMAP]->IsTexture();
//		bool maskDefined = params[ENVMAPMASK]->IsTexture();

		// Pass 1 : Base + env

		// FIXME: Could make it 1 pass for base + env, if it wasn't
		// for the envmap tint. So this is 3 passes for now....

		// If it's base + mask * env, gotta do that in 2 passes
		// Gotta do funky stuff to fade out self-illuminated stuff
		bool hasEnvMapTint = !IsWhite(ENVMAPTINT);
		
		// Special case, can do in one pass
		if (!hasEnvMapTint && !texDefined && !IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR) &&
			!IsColorModulating() )
		{
			DrawEnvmapTimesVertexLighting( params, pShaderAPI, pShaderShadow );
			return;
		}

		if (texDefined)
		{
			FixedFunctionBaseTimesDetailPass( 
				BASETEXTURE, FRAME, BASETEXTURETRANSFORM, DETAIL, DETAILSCALE ); 
		}
		else
		{
			FixedFunctionMaskedEnvmapPass( 
				ENVMAP, ENVMAPMASK, BASETEXTURE,
				ENVMAPFRAME, ENVMAPMASKFRAME, FRAME, 
				BASETEXTURETRANSFORM, ENVMAPMASKSCALE, ENVMAPTINT );
		}

		// We can get here if multipass isn't set if we specify a vertex color
		if ( IS_FLAG_SET(MATERIAL_VAR_MULTIPASS) )
		{
			if ( texDefined && envDefined )
			{
				FixedFunctionAdditiveMaskedEnvmapPass( 
					ENVMAP, ENVMAPMASK, BASETEXTURE,
					ENVMAPFRAME, ENVMAPMASKFRAME, FRAME, 
					BASETEXTURETRANSFORM, ENVMAPMASKSCALE, ENVMAPTINT );
			}
		}

		// Pass 2 : * vertex lighting
		MultiplyByVertexLighting( params, pShaderAPI, pShaderShadow );

		// FIXME: We could add it to the lightmap 
		// Draw the selfillum pass (blows away envmap at self-illum points)
		if ( IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) )
		{
			FixedFunctionSelfIlluminationPass( 
				SHADER_SAMPLER0, BASETEXTURE, FRAME, BASETEXTURETRANSFORM, SELFILLUMTINT );
		}
	}

	void DrawMode0( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		// Pass 1 : Base * lightmap or just lightmap
		if ( params[BASETEXTURE]->IsTexture() )
		{
			DrawBaseTimesVertexLighting( params, pShaderAPI, pShaderShadow );

			// Detail map
			FixedFunctionMultiplyByDetailPass(
				BASETEXTURE, FRAME, BASETEXTURETRANSFORM, DETAIL, DETAILSCALE );

			// Draw the selfillum pass
			if ( IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) )
			{
				FixedFunctionSelfIlluminationPass( 
					SHADER_SAMPLER0, BASETEXTURE, FRAME, BASETEXTURETRANSFORM, SELFILLUMTINT );
			}
		}
		else
		{
			DrawVertexLightingOnly( params, pShaderAPI, pShaderShadow );

			// Detail map
			FixedFunctionMultiplyByDetailPass(
				BASETEXTURE, FRAME, BASETEXTURETRANSFORM, DETAIL, DETAILSCALE );
		}
		 
		// Pass 2 : Masked environment map
		if ( params[ENVMAP]->IsTexture() && 
			 (IS_FLAG_SET(MATERIAL_VAR_MULTIPASS)) )
		{
			FixedFunctionAdditiveMaskedEnvmapPass( 
				ENVMAP, ENVMAPMASK, BASETEXTURE,
				ENVMAPFRAME, ENVMAPMASKFRAME, FRAME, 
				BASETEXTURETRANSFORM, ENVMAPMASKSCALE, ENVMAPTINT );
		}
	}

	SHADER_DRAW
	{
		bool useMode1 = IS_FLAG_SET(MATERIAL_VAR_ENVMAPMODE);
		if (!useMode1)
		{
			// Base * Vertex Lighting + env
			DrawMode0( params, pShaderAPI, pShaderShadow );
		}
		else
		{
			// ( Base + env ) * Vertex Lighting
			DrawMode1( params, pShaderAPI, pShaderShadow );
		}
	}
END_SHADER

