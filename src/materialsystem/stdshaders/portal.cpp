//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "portal_vs20.inc"
#include "portal_ps20.inc"
#include "portal_ps20b.inc"
#include "convar.h"
#include "cpp_shader_constant_register_map.h"


DEFINE_FALLBACK_SHADER( Portal, Portal_DX90 )

BEGIN_VS_SHADER( Portal_DX90, 
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
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
			return "Portal_DX80";

		return 0;
	}

	SHADER_INIT
	{
		if ( params[BASETEXTURE]->IsDefined() )
		{
			if ( IsX360() )
			{
				// prevent unused rt access
				IMaterialVar* pNameVar = params[BASETEXTURE];
				const char *pStringValue = pNameVar->GetStringValue();
				if ( !V_stricmp( pStringValue, "_rt_portal1" ) || !V_stricmp( pStringValue, "_rt_portal2" ) )
				{
					pNameVar->SetStringValue( "white" );
				}
			}
			LoadTexture( BASETEXTURE, TEXTUREFLAGS_SRGB );
		}

		if ( params[STATICBLENDTEXTURE]->IsDefined() )
			LoadTexture( STATICBLENDTEXTURE );	
		if ( params[ALPHAMASKTEXTURE]->IsDefined() )
			LoadTexture( ALPHAMASKTEXTURE );

		if ( !params[STATICAMOUNT]->IsDefined() )
			params[STATICAMOUNT]->SetFloatValue( 0.0f );

		if ( !params[STATICAMOUNT]->IsDefined() )
			params[STATICAMOUNT]->SetFloatValue( 0.0f );

		if ( !params[STATICBLENDTEXTURE]->IsDefined() )
			params[STATICBLENDTEXTURE]->SetIntValue( 0 );
		if ( !params[STATICBLENDTEXTUREFRAME]->IsDefined() )
			params[STATICBLENDTEXTUREFRAME]->SetIntValue( 0 );

		if ( !params[ALPHAMASKTEXTURE]->IsDefined() )
			params[ALPHAMASKTEXTURE]->SetIntValue( 0 );
		if ( !params[ALPHAMASKTEXTUREFRAME]->IsDefined() )
			params[ALPHAMASKTEXTUREFRAME]->SetIntValue( 0 );

		if ( !params[RENDERFIXZ]->IsDefined() )
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
		bool bAlphaMaskTexture = ( params[ALPHAMASKTEXTURE]->IsTexture()? 1 : 0 );
		
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

			if( bAlphaMaskTexture )
			{
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}
			else
			{
				pShaderShadow->EnableBlending( false );
			}

			pShaderShadow->EnableSRGBWrite( true );

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

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );

			if( bStaticBlendTexture || bAlphaMaskTexture )
				pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

			if( bStaticBlendTexture && bAlphaMaskTexture )
				pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
			
			DECLARE_STATIC_VERTEX_SHADER( portal_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( HASALPHAMASK, bAlphaMaskTexture );
			SET_STATIC_VERTEX_SHADER_COMBO( HASSTATICTEXTURE, bStaticBlendTexture );
			SET_STATIC_VERTEX_SHADER_COMBO( USEALTERNATEVIEW, (params[USEALTERNATEVIEWMATRIX]->GetIntValue() != 0) );
			SET_STATIC_VERTEX_SHADER( portal_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( portal_ps20b );
				SET_STATIC_PIXEL_SHADER_COMBO( HASALPHAMASK, bAlphaMaskTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( HASSTATICTEXTURE, bStaticBlendTexture );
				SET_STATIC_PIXEL_SHADER( portal_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( portal_ps20 );
				SET_STATIC_PIXEL_SHADER_COMBO( HASALPHAMASK, bAlphaMaskTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( HASSTATICTEXTURE, bStaticBlendTexture );
				SET_STATIC_PIXEL_SHADER( portal_ps20 );
			}

		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();

			//x is static, y is inverse static
			float pc0[4] = { fStaticAmount, 1.0f - fStaticAmount, 0.0f, 0.0f };
			pShaderAPI->SetPixelShaderConstant( 0, pc0 );

			pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

			float vEyePos_SpecExponent[4];
			pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );
			vEyePos_SpecExponent[3] = 0.0f;
			pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );

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

			DECLARE_DYNAMIC_VERTEX_SHADER( portal_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( ADDSTATIC, bHasStatic );
			SET_DYNAMIC_VERTEX_SHADER( portal_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( portal_ps20b );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( ADDSTATIC, bHasStatic );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( HDRENABLED, IsHDREnabled() );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER( portal_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( portal_ps20 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( ADDSTATIC, bHasStatic );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( HDRENABLED, IsHDREnabled() );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER( portal_ps20 );
			}
		}

		Draw();		
	}

END_SHADER


