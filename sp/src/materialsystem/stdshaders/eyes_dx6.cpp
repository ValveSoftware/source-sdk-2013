//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Teeth renderer
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "mathlib/vmatrix.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( Eyes, Eyes_dx6 )

BEGIN_VS_SHADER( Eyes_dx6, 
			  "Help for Eyes" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( IRIS, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "iris texture" )
		SHADER_PARAM( IRISFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame for the iris texture" )
		SHADER_PARAM( GLINT, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "glint texture" )
		SHADER_PARAM( EYEORIGIN, SHADER_PARAM_TYPE_VEC3, "[0 0 0]", "origin for the eyes" )
		SHADER_PARAM( EYEUP, SHADER_PARAM_TYPE_VEC3, "[0 0 1]", "up vector for the eyes" )
		SHADER_PARAM( IRISU, SHADER_PARAM_TYPE_VEC4, "[0 1 0 0 ]", "U projection vector for the iris" )
		SHADER_PARAM( IRISV, SHADER_PARAM_TYPE_VEC4, "[0 0 1 0]", "V projection vector for the iris" )
		SHADER_PARAM( GLINTU, SHADER_PARAM_TYPE_VEC4, "[0 1 0 0]", "U projection vector for the glint" )
		SHADER_PARAM( GLINTV, SHADER_PARAM_TYPE_VEC4, "[0 0 1 0]", "V projection vector for the glint" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		// FLASHLIGHTFIXME
		params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight001" );

		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
	}

	SHADER_INIT
	{
		LoadTexture( FLASHLIGHTTEXTURE );
		LoadTexture( BASETEXTURE );
		LoadTexture( IRIS );
	}

	void SetTextureTransform( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
					MaterialMatrixMode_t textureTransform, int uparam, int vparam )
	{
		Vector4D u, v;
		params[uparam]->GetVecValue( u.Base(), 4 );
		params[vparam]->GetVecValue( v.Base(), 4 );

		// Need to transform these puppies into camera space
		// they are defined in world space
		VMatrix mat, invTrans;
		pShaderAPI->GetMatrix( MATERIAL_VIEW, mat.m[0] );
		mat = mat.Transpose();

		// Compute the inverse transpose of the matrix
		// NOTE: I only have to invert it here because VMatrix is transposed
		// with respect to what gets returned from GetMatrix.
		mat.InverseGeneral( invTrans );
		invTrans = invTrans.Transpose();

		// Transform the u and v planes into view space
		Vector4D uview, vview;
		uview.AsVector3D() = invTrans.VMul3x3( u.AsVector3D() );
		vview.AsVector3D() = invTrans.VMul3x3( v.AsVector3D() );
		uview[3] = u[3] - DotProduct( mat.GetTranslation(), uview.AsVector3D() );
		vview[3] = v[3] - DotProduct( mat.GetTranslation(), vview.AsVector3D() );

		float m[16];
		m[0] = uview[0];	m[1] = vview[0];	m[2] = 0.0f;	m[3] = 0.0f;
		m[4] = uview[1];	m[5] = vview[1];	m[6] = 0.0f;	m[7] = 0.0f;
		m[8] = uview[2];	m[9] = vview[2];	m[10] = 1.0f;	m[11] = 0.0f;
		m[12] = uview[3];	m[13] = vview[3];	m[14] = 0.0f;	m[15] = 1.0f;

		pShaderAPI->MatrixMode( textureTransform );
		pShaderAPI->LoadMatrix( m );
	}

	void DrawFlashlight_Iris( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			SET_FLAGS2( MATERIAL_VAR2_NEEDS_FIXED_FUNCTION_FLASHLIGHT );

			// Alpha blend
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableAlphaWrites( false );

			int flags = SHADER_DRAW_POSITION | SHADER_DRAW_COLOR | SHADER_DRAW_NORMAL;
			pShaderShadow->DrawFlags( flags );
			FogToBlack();
			
			pShaderShadow->EnableLighting( true );
			
			pShaderShadow->EnableCustomPixelPipe( true );
			pShaderShadow->CustomTextureStages( 2 );
			
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_COLOR, 
				SHADER_TEXOP_MODULATE,
				SHADER_TEXARG_TEXTURE, 
				SHADER_TEXARG_VERTEXCOLOR );
			
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
				SHADER_TEXCHANNEL_COLOR, 
				SHADER_TEXOP_MODULATE,
				SHADER_TEXARG_TEXTURE, SHADER_TEXARG_PREVIOUSSTAGE );
			
			// alpha stage 0
			// get alpha from constant alpha
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE0, 
				SHADER_TEXCHANNEL_ALPHA, 
				SHADER_TEXOP_SELECTARG1,
				SHADER_TEXARG_CONSTANTCOLOR, SHADER_TEXARG_NONE );
			
			// alpha stage 1
			// get alpha from $basetexture
			pShaderShadow->CustomTextureOperation( SHADER_TEXTURE_STAGE1, 
				SHADER_TEXCHANNEL_ALPHA, 
				SHADER_TEXOP_MODULATE,
				SHADER_TEXARG_TEXTURE, SHADER_TEXARG_PREVIOUSSTAGE );
			
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			
			// Shove the view position into texcoord 0 before the texture matrix.
			pShaderShadow->TexGen( SHADER_TEXTURE_STAGE0, SHADER_TEXGENPARAM_EYE_LINEAR );
			pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE0, true );

			// iris transform
			pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE1, true );
			pShaderShadow->TexGen( SHADER_TEXTURE_STAGE1, SHADER_TEXGENPARAM_EYE_LINEAR );
		
		}
		DYNAMIC_STATE
		{
			SetFlashlightFixedFunctionTextureTransform( MATERIAL_TEXTURE0 );

			// NOTE: This has to come after the loadmatrix since the loadmatrix screws with the
			// transform flags!!!!!!
			// Specify that we have XYZ texcoords that need to be divided by W before the pixel shader.
			// NOTE Tried to divide XY by Z, but doesn't work.
			pShaderAPI->SetTextureTransformDimension( SHADER_TEXTURE_STAGE0, 3, true );
			
			BindTexture( SHADER_SAMPLER0, FLASHLIGHTTEXTURE, FLASHLIGHTTEXTUREFRAME );

			BindTexture( SHADER_SAMPLER1, IRIS, IRISFRAME );
			SetTextureTransform( params, pShaderAPI, MATERIAL_TEXTURE1, IRISU, IRISV );
		}
		Draw();
	}
	
	void DrawFlashlight( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		// whites
		DrawFlashlight_dx70( params, pShaderAPI, pShaderShadow, 
							 FLASHLIGHTTEXTURE, FLASHLIGHTTEXTUREFRAME, true );

		// iris
		DrawFlashlight_Iris( params, pShaderAPI, pShaderShadow );
	}
	
	void DrawUsingSoftwareLighting( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		// whites
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE0, OVERBRIGHT );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_COLOR | SHADER_DRAW_TEXCOORD0 );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
		}
		Draw();

		// iris
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE0, OVERBRIGHT );
			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_COLOR );
			pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->TexGen( SHADER_TEXTURE_STAGE0, SHADER_TEXGENPARAM_EYE_LINEAR );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, IRIS, IRISFRAME );
			SetTextureTransform( params, pShaderAPI, MATERIAL_TEXTURE0, IRISU, IRISV );
		}
		Draw();

		// Glint
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, false );
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE0, 1.0f );
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, 1.0f );

			pShaderShadow->EnableConstantColor( true );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );

			pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->TexGen( SHADER_TEXTURE_STAGE0, SHADER_TEXGENPARAM_EYE_LINEAR );

			pShaderShadow->DrawFlags( SHADER_DRAW_POSITION );
			FogToBlack();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, GLINT );
			SetTextureTransform( params, pShaderAPI, MATERIAL_TEXTURE0, GLINTU, GLINTV );
		}
		Draw( );
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
			DrawFlashlight( params, pShaderAPI, pShaderShadow );
		}
		else
		{
			DrawUsingSoftwareLighting( params, pShaderAPI, pShaderShadow );
		}

	}
END_SHADER

