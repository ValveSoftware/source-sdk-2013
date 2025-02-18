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

DEFINE_FALLBACK_SHADER( Vortwarp, VortWarp_DX7 )

float QuadraticBezier( float t, float A, float B, float C )
{
	return Lerp( t, Lerp( t, A, B ), Lerp( t, B, C ) );
}

float CubicBezier( float t, float A, float B, float C, float D )
{
	return QuadraticBezier( t, Lerp( t, A, B ), Lerp( t, B, C ), Lerp( t, C, D ) );
}

BEGIN_SHADER( VortWarp_DX7, 
			  "Help for VortWarp_DX7" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
 	    SHADER_PARAM( ENTITYORIGIN, SHADER_PARAM_TYPE_VEC3,"0.0","center if the model in world space" )
 	    SHADER_PARAM( WARPPARAM, SHADER_PARAM_TYPE_FLOAT,"0.0","animation param between 0 and 1" )

		SHADER_PARAM( SELFILLUMMAP, SHADER_PARAM_TYPE_TEXTURE, "", "self-illumination map" )
		SHADER_PARAM( UNLIT, SHADER_PARAM_TYPE_BOOL, "", "" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );

		if( !params[SELFILLUMTINT]->IsDefined() )
			params[SELFILLUMTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );

		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );

		if( !params[UNLIT]->IsDefined() )
		{
			params[UNLIT]->SetIntValue( 0 );
		}
	}

	SHADER_FALLBACK
	{
		if (g_pHardwareConfig->GetDXSupportLevel() < 70)
			return "VertexLitGeneric_DX6";
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
		if( params[SELFILLUMMAP]->IsDefined() )
		{
			LoadTexture( SELFILLUMMAP );
		}

		// Don't alpha test if the alpha channel is used for other purposes
		if (IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
			CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );
	}

	void DrawBaseTimesVertexColor( bool bUnlit, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			// alpha test
 			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			pShaderShadow->EnableCustomPixelPipe( true );
			
			pShaderShadow->CustomTextureStages( 1 );

			if( bUnlit )
			{
				pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
					SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_SELECTARG1, 
					SHADER_TEXARG_TEXTURE, SHADER_TEXARG_NONE );
			}
			else
			{
				pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
					SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_MODULATE2X, 
					SHADER_TEXARG_TEXTURE, SHADER_TEXARG_VERTEXCOLOR );
			}
		
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_SELECTARG1, 
				SHADER_TEXARG_TEXTURE, SHADER_TEXARG_NONE );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			int flags = SHADER_DRAW_POSITION | SHADER_DRAW_NORMAL | SHADER_DRAW_TEXCOORD0;
			pShaderShadow->DrawFlags( flags );
			DefaultFog();

			pShaderShadow->CustomTextureStages( 2 );

			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
				SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_SELECTARG1,
				SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_NONE );

			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
				SHADER_TEXCHANNEL_ALPHA, SHADER_TEXOP_ADD,
				SHADER_TEXARG_PREVIOUSSTAGE, SHADER_TEXARG_CONSTANTCOLOR );

			// for warping in
			pShaderShadow->EnableAlphaTest( true );

			pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GEQUAL, 1.0f );

//			SetDefaultBlendingShadowState( BASETEXTURE, true );
		}
		DYNAMIC_STATE
		{
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE0, BASETEXTURETRANSFORM );
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			float warp = params[WARPPARAM]->GetFloatValue();
			float t = warp;
			warp = CubicBezier( warp, 0.0f, 1.0f, 0.0f, 0.0f );
			warp = Lerp( t, warp, 1.0f );
			pShaderAPI->Color4f( 1.0f, 1.0f, 1.0f, warp );
		}
		Draw();
	}

	int GetDrawFlagsPass1(IMaterialVar** params)
	{
		int flags = SHADER_DRAW_POSITION;
		if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
			flags |= SHADER_DRAW_COLOR;
		if (params[BASETEXTURE]->IsTexture())
			flags |= SHADER_DRAW_TEXCOORD0;
		return flags;
	}

	void DrawUnlit( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			SetModulationShadowState();
			SetNormalBlendingShadowState( BASETEXTURE, true );
			pShaderShadow->DrawFlags( GetDrawFlagsPass1(params) );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE0, BASETEXTURETRANSFORM );
			SetModulationDynamicState();
		}
		Draw( );
	}

	//-----------------------------------------------------------------------------
	// Fixed function Self illumination pass
	//-----------------------------------------------------------------------------
	void ScrollySelfIllumPass( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			SetInitialShadowState();

			// A little setup for self illum here...
			SetModulationShadowState( SELFILLUMTINT );

			s_pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			s_pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

			// Don't bother with z writes here...
 			s_pShaderShadow->EnableDepthWrites( false );

			pShaderShadow->EnableCustomPixelPipe( true );
			
			pShaderShadow->CustomTextureStages( 2 );

			// basetexture * selfillumtint
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_MODULATE, 
				SHADER_TEXARG_TEXTURE, SHADER_TEXARG_CONSTANTCOLOR );
			// previous * selfillummap
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
				SHADER_TEXCHANNEL_COLOR, SHADER_TEXOP_MODULATE, 
				SHADER_TEXARG_TEXTURE, SHADER_TEXARG_PREVIOUSSTAGE );

			// We're always blending
			EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE );

			s_pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_TEXCOORD1 );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			s_pShaderAPI->SetDefaultState();

			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			BindTexture( SHADER_SAMPLER1, SELFILLUMMAP, -1 );

			// -----------------------------------------------------------
			// Set up the self illum tint
			// -----------------------------------------------------------
			float selfIllumTint[3];
			params[SELFILLUMTINT]->GetVecValue( selfIllumTint, 3 );
			selfIllumTint[0] = clamp( selfIllumTint[0], 0.0f, 1.0f );
			selfIllumTint[1] = clamp( selfIllumTint[1], 0.0f, 1.0f );
			selfIllumTint[2] = clamp( selfIllumTint[2], 0.0f, 1.0f );
//			Warning( "selfillumtint: %f %f %f\n", selfIllumTint[0], selfIllumTint[1], selfIllumTint[2] );

			pShaderAPI->Color4f( selfIllumTint[0], selfIllumTint[1], selfIllumTint[2], 1.0f );

			// -----------------------------------------------------------
			// Set up the self illum scrolling
			// -----------------------------------------------------------
			float curTime = pShaderAPI->CurrentTime();
			pShaderAPI->MatrixMode( MATERIAL_TEXTURE0 );

			// only do the upper 3x3 since this is a 2D matrix
			float mat[16];
			mat[0] = 1.0f;		mat[1] = 0.0f;		mat[2] = 0.0f;
			mat[4] = 0.0f;		mat[5] = 1.0f;		mat[6] = 0.0f;
			mat[8] = .11f * curTime; mat[9] = .124 * curTime; mat[10] = 1.0f;

			// Better set the stuff we don't set with some sort of value!
			mat[3] = mat[7] = mat[11] = 0;
			mat[12] = mat[13] = mat[14] = 0;
			mat[15] = 1;

			s_pShaderAPI->LoadMatrix( mat );
		}
		Draw();
	}

	SHADER_DRAW
	{
		bool hasFlashlight = UsingFlashlight( params );

		if( hasFlashlight )
		{
//			DrawFlashlight_dx70( params, pShaderAPI, pShaderShadow, FLASHLIGHTTEXTURE, FLASHLIGHTTEXTUREFRAME );
			return;
		}

		// Draw base times lighting.
		// Lighting is either sent down per vertex from the app, or it's in the second
		// stream as color values.
		DrawBaseTimesVertexColor( params[UNLIT]->GetIntValue() != 0, params, pShaderAPI, pShaderShadow );
//		ScrollySelfIllumPass( params, pShaderAPI, pShaderShadow );
	}
END_SHADER
