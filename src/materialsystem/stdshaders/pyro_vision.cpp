//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"
#include "commandbuilder.h"

#include "pyro_vision_ps20.inc"
#include "pyro_vision_ps20b.inc"
#include "pyro_vision_vs20.inc"
#include "pyro_vision_ps30.inc"
#include "pyro_vision_vs30.inc"

#include "../materialsystem_global.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


static ConVar mat_visualize_dof( "mat_visualize_dof", "0", FCVAR_CHEAT );


class CPyroVision_DX9_Context : public CBasePerMaterialContextData
{
public:
	uint8 *m_pStaticCmds;
	CCommandBufferBuilder< CFixedCommandStorageBuffer< 1000 > > m_SemiStaticCmdsOut;

	void ResetStaticCmds( void )
	{
		if ( m_pStaticCmds )
		{
			delete[] m_pStaticCmds;
			m_pStaticCmds = NULL;
		}
	}

	CPyroVision_DX9_Context( void )
	{
		m_pStaticCmds = NULL;
	}

	~CPyroVision_DX9_Context( void )
	{
		ResetStaticCmds();
	}

};


static const float kDefaultStripeScale[ 3 ] = { 0.002f, 0.002f, 0.2f };
static const float kDefaultStripeFadeNormal1[ 3 ] = { 0.0f, 0.0f, 1.0f };
static const float kDefaultStripeFadeNormal2[ 3 ] = { 0.0f, 0.0f, 0.0f };
static const float kDefaultStripeColor[ 3 ] = { 0.0f, 1.0f, 1.0f };
static const float kDefaultSelfIllumTint[ 3 ] = { 1.0f, 1.0f, 1.0f };
static const float kDefaultGrayStep[ 2 ] = { 0.0f, 1.0f };


BEGIN_VS_SHADER( pyro_vision, "Help for pyro vision" )
	BEGIN_SHADER_PARAMS
	
		SHADER_PARAM( EFFECT, SHADER_PARAM_TYPE_INTEGER, "0.0", "" )

		SHADER_PARAM( VERTEX_LIT, SHADER_PARAM_TYPE_INTEGER, "0.0", "" )
		SHADER_PARAM( FULLBRIGHT, SHADER_PARAM_TYPE_INTEGER, "0.0", "" )
		SHADER_PARAM( ALPHATESTREFERENCE, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )	
		
		SHADER_PARAM( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( BASETEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( BLENDMODULATETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "texture to use r/g channels for blend range for" )
		SHADER_PARAM( BLENDMASKTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$blendmodulatetexture texcoord transform" )

		SHADER_PARAM( STRIPETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( STRIPE_SCALE, SHADER_PARAM_TYPE_VEC3, "[ 0.002 0.002 0.2 ]", "" )
		SHADER_PARAM( STRIPE_FADE_NORMAL1, SHADER_PARAM_TYPE_VEC3, "[ 0.0 0.0 1.0 ]", "" )
		SHADER_PARAM( STRIPE_FADE_NORMAL2, SHADER_PARAM_TYPE_VEC3, "[ 0.0 0.0 0.0 ]", "" )
		SHADER_PARAM( STRIPE_COLOR, SHADER_PARAM_TYPE_VEC3, "[ 0.0 0.0 0.0 ]", "" )
		SHADER_PARAM( STRIPE_LM_SCALE, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )

		// EFFECT 0
		SHADER_PARAM( BASE_STEP_RANGE, SHADER_PARAM_TYPE_VEC2, "[ 0.0 1.0 ]", "" )
		SHADER_PARAM( LIGHTMAP_STEP_RANGE, SHADER_PARAM_TYPE_VEC2, "[ 0.0 1.0 ]", "" )
		SHADER_PARAM( CANVAS, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( CANVAS_SCALE, SHADER_PARAM_TYPE_VEC3, "[ 0.002 0.002 0.2 ]", "" )
		SHADER_PARAM( CANVAS_STEP_RANGE, SHADER_PARAM_TYPE_VEC2, "[ 0.0 1.0 ]", "" )
		SHADER_PARAM( CANVAS_COLOR_START, SHADER_PARAM_TYPE_VEC3, "[ 0.0 0.0 0.0 ]", "" )
		SHADER_PARAM( CANVAS_COLOR_END, SHADER_PARAM_TYPE_VEC3, "[ 0.0 0.0 0.0 ]", "" )

		// EFFECT 1
		SHADER_PARAM( DIFFUSE_BASE, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )
		SHADER_PARAM( DIFFUSE_WHITE, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )
		SHADER_PARAM( GRAY_POWER, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )
		SHADER_PARAM( GRAY_STEP, SHADER_PARAM_TYPE_VEC2, "[ 0.0 1.0 ]", "" )
		SHADER_PARAM( LIGHTMAP_GRADIENTS, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )
		SHADER_PARAM( COLORBAR, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[ 1.0 1.0 1.0 ]", "Self-illumination tint" )
		
		// EFFECT 2 Depth of Field
		SHADER_PARAM( DOF_START_DISTANCE, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )
		SHADER_PARAM( DOF_POWER, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )
		SHADER_PARAM( DOF_MAX, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )

		// EFFECT 3 Pyro Post
		SHADER_PARAM( WARPTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( NOISETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( VIGNETTE_TEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( VIGNETTE_TILE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( NOISE_SCALE, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )
		SHADER_PARAM( TIME_SCALE, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )
		SHADER_PARAM( HEAT_HAZE_SCALE, SHADER_PARAM_TYPE_FLOAT, "0.0", "" )
	
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_PARAM_INT_IF_NOT_DEFINED( EFFECT, 0 );
		SET_PARAM_INT_IF_NOT_DEFINED( VERTEX_LIT, 0 );
		SET_PARAM_INT_IF_NOT_DEFINED( FULLBRIGHT, 0 );
		SET_PARAM_FLOAT_IF_NOT_DEFINED( ALPHATESTREFERENCE, 0.0f );

		SET_PARAM_STRING_IF_NOT_DEFINED( BLENDMASKTRANSFORM, "center .5 .5 scale 1 1 rotate 0 translate 0 0" );

		SET_PARAM_VEC_IF_NOT_DEFINED( STRIPE_SCALE, kDefaultStripeScale, 3 );
		SET_PARAM_VEC_IF_NOT_DEFINED( STRIPE_FADE_NORMAL1, kDefaultStripeFadeNormal1, 3 );
		SET_PARAM_VEC_IF_NOT_DEFINED( STRIPE_FADE_NORMAL2, kDefaultStripeFadeNormal2, 3 );
		SET_PARAM_VEC_IF_NOT_DEFINED( STRIPE_COLOR, kDefaultStripeColor, 3 );
		SET_PARAM_FLOAT_IF_NOT_DEFINED( STRIPE_LM_SCALE, 0.0f );

		SET_PARAM_FLOAT_IF_NOT_DEFINED( DIFFUSE_BASE, 0.0f );
		SET_PARAM_FLOAT_IF_NOT_DEFINED( DIFFUSE_WHITE, 0.0f );
		SET_PARAM_FLOAT_IF_NOT_DEFINED( GRAY_POWER, 1.0f );
		SET_PARAM_VEC_IF_NOT_DEFINED( GRAY_STEP, kDefaultGrayStep, 2 );
		SET_PARAM_FLOAT_IF_NOT_DEFINED( LIGHTMAP_GRADIENTS, 255.0f );
		
		
		SET_PARAM_VEC_IF_NOT_DEFINED( SELFILLUMTINT, kDefaultSelfIllumTint, 3 );

		SET_PARAM_FLOAT_IF_NOT_DEFINED( DOF_START_DISTANCE, 0.1f );
		SET_PARAM_FLOAT_IF_NOT_DEFINED( DOF_POWER, 0.4f );
		SET_PARAM_FLOAT_IF_NOT_DEFINED( DOF_MAX, 0.7f );

		SET_PARAM_FLOAT_IF_NOT_DEFINED( NOISE_SCALE, 0.2f );
		SET_PARAM_FLOAT_IF_NOT_DEFINED( TIME_SCALE, 0.02f );
		SET_PARAM_FLOAT_IF_NOT_DEFINED( HEAT_HAZE_SCALE, 0.2f );

		bool	bVertexLit = params[ VERTEX_LIT ]->GetIntValue() != 0;

		if ( bVertexLit )
		{
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );
		}
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE, TEXTUREFLAGS_SRGB );

		if ( params[ CANVAS ]->IsDefined() )
		{
			LoadTexture( CANVAS, TEXTUREFLAGS_SRGB );
		}

		if ( params[ COLORBAR ]->IsDefined() )
		{
			LoadTexture( COLORBAR, TEXTUREFLAGS_SRGB );
		}

		if ( params[ BASETEXTURE2 ]->IsDefined() )
		{
			LoadTexture( BASETEXTURE2, TEXTUREFLAGS_SRGB );
		}

		if ( params[ BLENDMODULATETEXTURE ]->IsDefined() )
		{
			LoadTexture( BLENDMODULATETEXTURE );
		}

		if ( params[ STRIPETEXTURE ]->IsDefined() )
		{
			LoadTexture( STRIPETEXTURE, TEXTUREFLAGS_SRGB );
		}

		if ( params[ WARPTEXTURE ]->IsDefined() )
		{
			LoadTexture( WARPTEXTURE );
		}

		if ( params[ NOISETEXTURE ]->IsDefined() )
		{
			LoadTexture( NOISETEXTURE );
		}

		if ( params[ VIGNETTE_TEXTURE ]->IsDefined() )
		{
			LoadTexture( VIGNETTE_TEXTURE );
		}

		if ( params[ VIGNETTE_TILE ]->IsDefined() )
		{
			LoadTexture( VIGNETTE_TILE, TEXTUREFLAGS_SRGB );
		}
	}

	SHADER_DRAW
	{
		CPyroVision_DX9_Context		*pContextData = reinterpret_cast< CPyroVision_DX9_Context *> ( *pContextDataPtr );
		bool						bNeedRegenStaticCmds = ( !pContextData ) || pShaderShadow;
		bool						bVertexLit = params[ VERTEX_LIT ]->GetIntValue() != 0;
		bool						bFullBright = params[ FULLBRIGHT ]->GetIntValue() != 0;
		bool						bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();
		bool						bHasVertexColor = IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR );
		bool						bHasBaseTexture2 = ( params[ BASETEXTURE2 ]->IsDefined() ) && ( params[ BASETEXTURE2 ]->IsTexture() );
		bool						bHasBlendModulateTexture = bHasBaseTexture2 && ( params[ BLENDMODULATETEXTURE ]->IsDefined() ) && ( params[ BLENDMODULATETEXTURE ]->IsTexture() );
		bool						bHasHeatHaze = ( params[ WARPTEXTURE ]->IsDefined() ) && ( params[ WARPTEXTURE ]->IsTexture() );
		int							nEffect = params[ EFFECT ]->GetIntValue();
		bool						bSelfIllum = IS_FLAG_SET( MATERIAL_VAR_SELFILLUM );
		bool						bVisualizeDoF = mat_visualize_dof.GetBool();
		bool						bIsAlphaTested = IS_FLAG_SET( MATERIAL_VAR_ALPHATEST ) != 0;
		bool						bFullyOpaque = !bIsAlphaTested;
		bool						bHasColorbar = ( params[ COLORBAR ]->IsDefined() ) && ( params[ COLORBAR ]->IsTexture() );
		bool						bHasStripes = ( params[ STRIPETEXTURE ]->IsDefined() ) && ( params[ STRIPETEXTURE ]->IsTexture() );
		bool						bHasStripesNormal2 = false;
		if ( bHasStripes )
		{
			Vector vNormal2;

			params[ STRIPE_FADE_NORMAL2 ]->GetVecValue( vNormal2.Base(), 3 );

			if ( vNormal2.LengthSqr() > 0.5f )
			{
				bHasStripesNormal2 = true;
			}
		}

		if ( !pContextData )								// make sure allocated
		{
			pContextData = new CPyroVision_DX9_Context;
			*pContextDataPtr = pContextData;
		}

		if ( nEffect == 3 )
		{
			static ConVarRef  pyro_vignette_distortion( "pyro_vignette_distortion" );

			bHasHeatHaze &= pyro_vignette_distortion.GetBool();
		}


		if ( pShaderShadow || bNeedRegenStaticCmds )
		{
			pContextData->ResetStaticCmds();
			CCommandBufferBuilder< CFixedCommandStorageBuffer< 5000 > > staticCmdsBuf;

			Vector4D vParms(0, 0, 0, 0);

			params[ STRIPE_SCALE ]->GetVecValue( &vParms.x, 3 );
			staticCmdsBuf.SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, vParms.Base() );

			staticCmdsBuf.BindTexture( this, SHADER_SAMPLER0, BASETEXTURE, -1 );
			staticCmdsBuf.BindStandardTexture( SHADER_SAMPLER1, TEXTURE_LIGHTMAP );
			if ( bHasBlendModulateTexture )
			{
				staticCmdsBuf.BindTexture( this, SHADER_SAMPLER3, BLENDMODULATETEXTURE, -1 );
				staticCmdsBuf.BindTexture( this, SHADER_SAMPLER4, BASETEXTURE2, -1 );
			}

			if ( bHasStripes )
			{
				staticCmdsBuf.BindTexture( this, SHADER_SAMPLER5, STRIPETEXTURE, -1 );
				
				params[ STRIPE_COLOR ]->GetVecValue( &vParms.x, 3 );
				vParms.w = params[ STRIPE_LM_SCALE ]->GetFloatValue();
				staticCmdsBuf.SetPixelShaderConstant( 5, vParms.Base() );

				params[ STRIPE_FADE_NORMAL1 ]->GetVecValue( &vParms.x, 3 );
				staticCmdsBuf.SetPixelShaderConstant( 6, vParms.Base() );

				if ( bHasStripesNormal2 )
				{
					params[ STRIPE_FADE_NORMAL2 ]->GetVecValue( &vParms.x, 3 );
					staticCmdsBuf.SetPixelShaderConstant( 7, vParms.Base() );
				}
			}

			switch( nEffect )
			{
				case 0:
					{
						params[ CANVAS_SCALE ]->GetVecValue( &vParms.x, 3 );
						staticCmdsBuf.SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, vParms.Base() );

						staticCmdsBuf.BindTexture( this, SHADER_SAMPLER2, CANVAS, -1 );

						params[ BASE_STEP_RANGE ]->GetVecValue( &vParms.x, 2 );
						params[ LIGHTMAP_STEP_RANGE ]->GetVecValue( &vParms.z, 2 );
						staticCmdsBuf.SetPixelShaderConstant( 0, vParms.Base() );

						params[ CANVAS_STEP_RANGE ]->GetVecValue( &vParms.x, 2 );
						staticCmdsBuf.SetPixelShaderConstant( 2, vParms.Base() );

						params[ CANVAS_COLOR_START ]->GetVecValue( &vParms.x, 3 );
						staticCmdsBuf.SetPixelShaderConstant( 3, vParms.Base() );

						params[ CANVAS_COLOR_END ]->GetVecValue( &vParms.x, 3 );
						staticCmdsBuf.SetPixelShaderConstant( 4, vParms.Base() );
					}
					break;

				case 1:
					{
						if ( bHasColorbar )
						{
							staticCmdsBuf.BindTexture( this, SHADER_SAMPLER2, COLORBAR, -1 );
						}

						vParms.x = params[ GRAY_POWER ]->GetFloatValue();
						params[ GRAY_STEP ]->GetVecValue( &vParms.y, 2 );
						vParms.w = params[ LIGHTMAP_GRADIENTS ]->GetFloatValue();
						staticCmdsBuf.SetPixelShaderConstant( 0, vParms.Base() );

						vParms[ 0 ] = params[ DIFFUSE_BASE ]->GetFloatValue();
						params[ SELFILLUMTINT ]->GetVecValue( &vParms[ 1 ], 3 );
						staticCmdsBuf.SetPixelShaderConstant( 2, vParms.Base() );
					}
					break;

				case 2:
					{
						staticCmdsBuf.BindTexture( this, SHADER_SAMPLER4, BASETEXTURE2, -1 );

						vParms.x = params[ DOF_START_DISTANCE ]->GetFloatValue();
						vParms.y = params[ DOF_POWER ]->GetFloatValue();
						vParms.z = params[ DOF_MAX ]->GetFloatValue();
						vParms.w = 0.0f;
						staticCmdsBuf.SetPixelShaderConstant( 0, vParms.Base() );
					}
					break;

				case 3:
					{
						staticCmdsBuf.BindTexture( this, SHADER_SAMPLER3, NOISETEXTURE, -1 );
						staticCmdsBuf.BindTexture( this, SHADER_SAMPLER4, WARPTEXTURE, -1 );
						staticCmdsBuf.BindTexture( this, SHADER_SAMPLER5, VIGNETTE_TEXTURE, -1 );
						staticCmdsBuf.BindTexture( this, SHADER_SAMPLER6, VIGNETTE_TILE, -1 );

						vParms.x = params[ NOISE_SCALE ]->GetFloatValue();
						vParms.y = params[ TIME_SCALE ]->GetFloatValue();
						vParms.z = params[ HEAT_HAZE_SCALE ]->GetFloatValue();
						vParms.w = 0.0f;
						staticCmdsBuf.SetPixelShaderConstant( 0, vParms.Base() );
					}
					break;
			}

			staticCmdsBuf.StoreEyePosInPixelShaderConstant( 10 );
			staticCmdsBuf.SetPixelShaderFogParams( 11 );
			staticCmdsBuf.End();

			// now, copy buf
			pContextData->m_pStaticCmds = new uint8[ staticCmdsBuf.Size() ];
			memcpy( pContextData->m_pStaticCmds, staticCmdsBuf.Base(), staticCmdsBuf.Size() );
		}

		if ( pShaderAPI && pContextData->m_bMaterialVarsChanged )
		{
			// need to regenerate the semistatic cmds
			pContextData->m_SemiStaticCmdsOut.Reset();
			pContextData->m_bMaterialVarsChanged = false;

			if ( bHasBlendModulateTexture )
			{
				pContextData->m_SemiStaticCmdsOut.SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_10, BLENDMASKTRANSFORM );
			}

			Vector4D vParms;

//			vParms.x = pShaderAPI->LinearToGamma_HardwareSpecific( 1.0f );

			switch( nEffect )
			{
				case 0:
					{
						// set up shader modulation color
						vParms[ 0 ] = vParms[ 1 ] = vParms[ 2] = vParms[ 3 ] = 1.0f;
						ComputeModulationColor( vParms.Base() );
						float flLScale = pShaderAPI->GetLightMapScaleFactor();
						vParms[ 0 ] *= flLScale;
						vParms[ 1 ] *= flLScale;
						vParms[ 2 ] *= flLScale;
						pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 1, vParms.Base() );
					}
					break;

				case 1:
					{
						// set up shader modulation color
						vParms[ 0 ] = vParms[ 1 ] = vParms[ 2] = vParms[ 3 ] = 1.0f;
						ComputeModulationColor( vParms.Base() );
						float flLScale = pShaderAPI->GetLightMapScaleFactor();
						vParms[ 0 ] *= flLScale;
						vParms[ 1 ] *= flLScale;
						vParms[ 2 ] *= flLScale;
						vParms[ 3 ] = params[ DIFFUSE_WHITE ]->GetFloatValue();
						pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 1, vParms.Base() );
					}
					break;
			}

			pContextData->m_SemiStaticCmdsOut.SetAmbientCubeDynamicStateVertexShader();
			pContextData->m_SemiStaticCmdsOut.End();
		}

		SHADOW_STATE
		{
			SetInitialShadowState( );

			switch( nEffect )
			{
				case 0:
				case 1:
					{
						pShaderShadow->EnableDepthWrites( true );
						pShaderShadow->EnableDepthTest( true );

						SetDefaultBlendingShadowState( BASETEXTURE );
					}
					break;

				case 2:
					{
						pShaderShadow->EnableDepthWrites( false );
						pShaderShadow->EnableDepthTest( false );

						pShaderShadow->EnableBlending( true );
						pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
					}
					break;

				case 3:
					{
						pShaderShadow->EnableDepthWrites( false );
						pShaderShadow->EnableDepthTest( false );

						pShaderShadow->EnableBlending( false );
					}
					break;
			}
//			pShaderShadow->BlendOp( SHADER_BLEND_OP_REVSUBTRACT );
//			EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );

			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
			{
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, true );
			}
			else
			{
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, false );
			}

			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER2, true );

			if ( nEffect == 0 || nEffect == 1 )
			{
				if ( bHasBlendModulateTexture )
				{
					pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
					pShaderShadow->EnableSRGBRead( SHADER_SAMPLER3, false );

					pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
					pShaderShadow->EnableSRGBRead( SHADER_SAMPLER4, true );
				}

				if ( bHasStripes )
				{
					pShaderShadow->EnableTexture( SHADER_SAMPLER5, true );
					pShaderShadow->EnableSRGBRead( SHADER_SAMPLER5, true );
				}
			}

			if ( nEffect == 2 )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER4, true );
			}

			if ( nEffect == 3 )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER3, true );

				pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER4, true );

				pShaderShadow->EnableTexture( SHADER_SAMPLER5, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER5, false );

				pShaderShadow->EnableTexture( SHADER_SAMPLER6, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER6, true );
			}

			pShaderShadow->EnableSRGBWrite( true );
			pShaderShadow->EnableAlphaWrites( true );
			pShaderShadow->EnableAlphaTest( bIsAlphaTested );

			if ( params[ ALPHATESTREFERENCE ]->GetFloatValue() > 0.0f )
			{
				pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GEQUAL, params[ ALPHATESTREFERENCE ]->GetFloatValue() );
			}

			DefaultFog();

			int nFormat = 0;
			if ( bHasVertexColor )
			{
				nFormat |= VERTEX_COLOR;
			}

			if ( bVertexLit )
			{
				nFormat |= VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;
				pShaderShadow->VertexShaderVertexFormat( nFormat, 1, 0, 0 );
			}
			else
			{
				nFormat |= VERTEX_POSITION;
				pShaderShadow->VertexShaderVertexFormat( nFormat, 2, 0, 0 );
			}

			bool bHalfLambert = IS_FLAG_SET( MATERIAL_VAR_HALFLAMBERT );

			if ( !g_pHardwareConfig->HasFastVertexTextures() )
			{
				DECLARE_STATIC_VERTEX_SHADER( pyro_vision_vs20 );
				SET_STATIC_VERTEX_SHADER_COMBO( EFFECT, params[ EFFECT ]->GetIntValue() );
				SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR, IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) );
				SET_STATIC_VERTEX_SHADER_COMBO( VERTEX_LIT, bVertexLit );
				SET_STATIC_VERTEX_SHADER_COMBO( FULLBRIGHT, bFullBright );
				SET_STATIC_VERTEX_SHADER_COMBO( HALFLAMBERT,  bHalfLambert );
				SET_STATIC_VERTEX_SHADER_COMBO( BASETEXTURE2, bHasBaseTexture2 );
				SET_STATIC_VERTEX_SHADER_COMBO( STRIPES, bHasStripes );
				SET_STATIC_VERTEX_SHADER_COMBO( STRIPES_USE_NORMAL2, bHasStripesNormal2 );
				SET_STATIC_VERTEX_SHADER_COMBO( USE_STATIC_CONTROL_FLOW, bUseStaticControlFlow );
				SET_STATIC_VERTEX_SHADER( pyro_vision_vs20 );

				if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_STATIC_PIXEL_SHADER( pyro_vision_ps20b );
					SET_STATIC_PIXEL_SHADER_COMBO( EFFECT, params[ EFFECT ]->GetIntValue() );
					SET_STATIC_PIXEL_SHADER_COMBO( VERTEX_LIT, bVertexLit );
					SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURE2, bHasBaseTexture2 );
					SET_STATIC_PIXEL_SHADER_COMBO( FANCY_BLENDING, bHasBlendModulateTexture );
					SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM, bSelfIllum );
					SET_STATIC_PIXEL_SHADER_COMBO( COLOR_BAR, bHasColorbar );
					SET_STATIC_PIXEL_SHADER_COMBO( STRIPES, bHasStripes );
					SET_STATIC_PIXEL_SHADER_COMBO( STRIPES_USE_NORMAL2, bHasStripesNormal2 );
					SET_STATIC_PIXEL_SHADER( pyro_vision_ps20b );
				}
				else
				{
					DECLARE_STATIC_PIXEL_SHADER( pyro_vision_ps20 );
					SET_STATIC_PIXEL_SHADER_COMBO( EFFECT, params[ EFFECT ]->GetIntValue() );
					SET_STATIC_PIXEL_SHADER_COMBO( VERTEX_LIT, bVertexLit );
					SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURE2, bHasBaseTexture2 );
					SET_STATIC_PIXEL_SHADER_COMBO( FANCY_BLENDING, bHasBlendModulateTexture );
					SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM, bSelfIllum );
					SET_STATIC_PIXEL_SHADER_COMBO( COLOR_BAR, bHasColorbar );
					SET_STATIC_PIXEL_SHADER_COMBO( STRIPES, bHasStripes );
					SET_STATIC_PIXEL_SHADER_COMBO( STRIPES_USE_NORMAL2, bHasStripesNormal2 );
					SET_STATIC_PIXEL_SHADER( pyro_vision_ps20 );
				}
			}
			else
			{
				DECLARE_STATIC_VERTEX_SHADER( pyro_vision_vs30 );
				SET_STATIC_VERTEX_SHADER_COMBO( EFFECT, params[ EFFECT ]->GetIntValue() );
				SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR, IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) );
				SET_STATIC_VERTEX_SHADER_COMBO( VERTEX_LIT, bVertexLit );
				SET_STATIC_VERTEX_SHADER_COMBO( FULLBRIGHT, bFullBright );
				SET_STATIC_VERTEX_SHADER_COMBO( HALFLAMBERT,  bHalfLambert );
				SET_STATIC_VERTEX_SHADER_COMBO( BASETEXTURE2, bHasBaseTexture2 );
				SET_STATIC_VERTEX_SHADER_COMBO( STRIPES, bHasStripes );
				SET_STATIC_VERTEX_SHADER_COMBO( STRIPES_USE_NORMAL2, bHasStripesNormal2 );
				SET_STATIC_VERTEX_SHADER( pyro_vision_vs30 );

				DECLARE_STATIC_PIXEL_SHADER( pyro_vision_ps30 );
				SET_STATIC_PIXEL_SHADER_COMBO( EFFECT, params[ EFFECT ]->GetIntValue() );
				SET_STATIC_PIXEL_SHADER_COMBO( VERTEX_LIT, bVertexLit );
				SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURE2, bHasBaseTexture2 );
				SET_STATIC_PIXEL_SHADER_COMBO( FANCY_BLENDING, bHasBlendModulateTexture );
				SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM, bSelfIllum );
				SET_STATIC_PIXEL_SHADER_COMBO( COLOR_BAR, bHasColorbar );
				SET_STATIC_PIXEL_SHADER_COMBO( STRIPES, bHasStripes );
				SET_STATIC_PIXEL_SHADER_COMBO( STRIPES_USE_NORMAL2, bHasStripesNormal2 );
				SET_STATIC_PIXEL_SHADER( pyro_vision_ps30 );
			}
		}

		DYNAMIC_STATE
		{
			CCommandBufferBuilder< CFixedCommandStorageBuffer< 1000 > > DynamicCmdsOut;
			DynamicCmdsOut.Call( pContextData->m_pStaticCmds );
			DynamicCmdsOut.Call( pContextData->m_SemiStaticCmdsOut.Base() );

			pShaderAPI->SetDefaultState();

			bool bWriteDepthToAlpha;
			if( bFullyOpaque ) 
			{
				bWriteDepthToAlpha = pShaderAPI->ShouldWriteDepthToDestAlpha();
			}
			else
			{
				//can't write a special value to dest alpha if we're actually using as-intended alpha
				bWriteDepthToAlpha = false;
			}

			Vector4D vParms;

			vParms.x = bWriteDepthToAlpha ? 1.0f : 0.0f;
			vParms.y = pShaderAPI->CurrentTime();
			vParms.z = 0.0f;
			vParms.w = 0.0f;

			DynamicCmdsOut.SetPixelShaderConstant( 12, vParms.Base() );

			int numBones = pShaderAPI->GetCurrentNumBones();
			LightState_t lightState = { 0, false, false };
			if ( bVertexLit && !bFullBright )
			{
				pShaderAPI->GetDX9LightState( &lightState );
			}

			if ( !g_pHardwareConfig->HasFastVertexTextures() )
			{
				DECLARE_DYNAMIC_VERTEX_SHADER( pyro_vision_vs20 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING,  numBones > 0 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( DYNAMIC_LIGHT, lightState.HasDynamicLight() );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( STATIC_LIGHT,  lightState.m_bStaticLightVertex ? 1 : 0 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( NUM_LIGHTS, bUseStaticControlFlow ? 0 : lightState.m_nNumLights );
				SET_DYNAMIC_VERTEX_SHADER( pyro_vision_vs20 );

				if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( pyro_vision_ps20b );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( VISUALIZE_DOF, bVisualizeDoF );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( HEATHAZE, bHasHeatHaze );
					SET_DYNAMIC_PIXEL_SHADER( pyro_vision_ps20b );
				}
				else
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( pyro_vision_ps20 );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( VISUALIZE_DOF, bVisualizeDoF );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( HEATHAZE, bHasHeatHaze );
					SET_DYNAMIC_PIXEL_SHADER( pyro_vision_ps20 );
				}
			}
			else
			{
				DECLARE_DYNAMIC_VERTEX_SHADER( pyro_vision_vs30 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING,  numBones > 0 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( DYNAMIC_LIGHT, lightState.HasDynamicLight() );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( STATIC_LIGHT,  lightState.m_bStaticLightVertex ? 1 : 0 );
				SET_DYNAMIC_VERTEX_SHADER( pyro_vision_vs30 );

				DECLARE_DYNAMIC_PIXEL_SHADER( pyro_vision_ps30 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( VISUALIZE_DOF, bVisualizeDoF );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( HEATHAZE, bHasHeatHaze );
				SET_DYNAMIC_PIXEL_SHADER( pyro_vision_ps30 );
			}


			DynamicCmdsOut.End();
			pShaderAPI->ExecuteCommandBuffer( DynamicCmdsOut.Base() );
		}
		Draw();
	}
END_SHADER
