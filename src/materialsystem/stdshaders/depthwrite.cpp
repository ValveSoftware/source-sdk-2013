//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "depthwrite_ps20.inc"
#include "depthwrite_ps20b.inc"
#include "depthwrite_vs20.inc"

#if !defined( _X360 )
#include "depthwrite_ps30.inc"
#include "depthwrite_vs30.inc"
#endif

BEGIN_VS_SHADER_FLAGS( DepthWrite, "Help for Depth Write", SHADER_NOT_EDITABLE )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( ALPHATESTREFERENCE, SHADER_PARAM_TYPE_FLOAT, "", "Alpha reference value" )
		SHADER_PARAM( COLOR_DEPTH, SHADER_PARAM_TYPE_BOOL, "0", "Write depth as color")
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
	}

	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			return "Wireframe";
		}
		return 0;
	}

	SHADER_INIT
	{
	}

	SHADER_DRAW
	{
		bool bAlphaClip = IS_FLAG_SET( MATERIAL_VAR_ALPHATEST );
		int nColorDepth = GetIntParam( COLOR_DEPTH, params, 0 );

		SHADOW_STATE
		{
			// Set stream format (note that this shader supports compression)
			unsigned int flags = VERTEX_POSITION | VERTEX_FORMAT_COMPRESSED;
			int nTexCoordCount = 1;
			int userDataSize = 0;
			pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

			if ( nColorDepth == 0 )
			{
				// Bias primitives when rendering into shadow map so we get slope-scaled depth bias
				// rather than having to apply a constant bias in the filtering shader later
				pShaderShadow->EnablePolyOffset( SHADER_POLYOFFSET_SHADOW_BIAS );
			}

			// Turn off writes to color buffer since we always sample shadows from the DEPTH texture later
			// This gives us double-speed fill when rendering INTO the shadow map
			pShaderShadow->EnableColorWrites( ( nColorDepth == 1 ) );
			pShaderShadow->EnableAlphaWrites( false );

			// Don't backface cull unless alpha clipping, since this can cause artifacts when the
			// geometry is clipped by the flashlight near plane
			// If a material was already marked nocull, don't cull it
			pShaderShadow->EnableCulling( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) && !IS_FLAG_SET(MATERIAL_VAR_NOCULL) );

#ifndef _X360
			if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
			{
				DECLARE_STATIC_VERTEX_SHADER( depthwrite_vs20 );
				SET_STATIC_VERTEX_SHADER_COMBO( ONLY_PROJECT_POSITION, !bAlphaClip && IsX360() && !nColorDepth ); //360 needs to know if it *shouldn't* output texture coordinates to avoid shader patches
				SET_STATIC_VERTEX_SHADER_COMBO( COLOR_DEPTH, nColorDepth );
				SET_STATIC_VERTEX_SHADER( depthwrite_vs20 );
				
				if ( bAlphaClip || g_pHardwareConfig->PlatformRequiresNonNullPixelShaders() || nColorDepth )
				{
					if( bAlphaClip )
					{
						pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
						pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );
					}

					if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
					{
						DECLARE_STATIC_PIXEL_SHADER( depthwrite_ps20b );
						SET_STATIC_PIXEL_SHADER_COMBO( COLOR_DEPTH, nColorDepth );
						SET_STATIC_PIXEL_SHADER( depthwrite_ps20b );
					}
					else
					{
						DECLARE_STATIC_PIXEL_SHADER( depthwrite_ps20 );
						SET_STATIC_PIXEL_SHADER_COMBO( COLOR_DEPTH, nColorDepth );
						SET_STATIC_PIXEL_SHADER( depthwrite_ps20 );
					}
				}
			}
#ifndef _X360
			else
			{
				SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );

				DECLARE_STATIC_VERTEX_SHADER( depthwrite_vs30 );
				SET_STATIC_VERTEX_SHADER_COMBO( ONLY_PROJECT_POSITION, 0 ); //360 only combo, and this is a PC path
				SET_STATIC_VERTEX_SHADER_COMBO( COLOR_DEPTH, nColorDepth );
				SET_STATIC_VERTEX_SHADER( depthwrite_vs30 );

				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );

				DECLARE_STATIC_PIXEL_SHADER( depthwrite_ps30 );
				SET_STATIC_PIXEL_SHADER_COMBO( COLOR_DEPTH, nColorDepth );
				SET_STATIC_PIXEL_SHADER( depthwrite_ps30 );
			}
#endif
		}
		DYNAMIC_STATE
		{

#ifndef _X360
			if ( !g_pHardwareConfig->HasFastVertexTextures() )
#endif
			{
				depthwrite_vs20_Dynamic_Index vshIndex;
				vshIndex.SetSKINNING( pShaderAPI->GetCurrentNumBones() > 0 );
				vshIndex.SetCOMPRESSED_VERTS( (int)vertexCompression );
				pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );

				if ( bAlphaClip )
				{
					BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

					float vAlphaThreshold[4] = {0.7f, 0.7f, 0.7f, 0.7f};
					if ( ALPHATESTREFERENCE != -1 && ( params[ALPHATESTREFERENCE]->GetFloatValue() > 0.0f ) )
					{
						vAlphaThreshold[0] = vAlphaThreshold[1] = vAlphaThreshold[2] = vAlphaThreshold[3] = params[ALPHATESTREFERENCE]->GetFloatValue();
					}

					pShaderAPI->SetPixelShaderConstant( 0, vAlphaThreshold, 1 );
				}

				if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( depthwrite_ps20b );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( ALPHACLIP, bAlphaClip );
					SET_DYNAMIC_PIXEL_SHADER( depthwrite_ps20b );
				}
				else
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( depthwrite_ps20 );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( ALPHACLIP, bAlphaClip );
					SET_DYNAMIC_PIXEL_SHADER( depthwrite_ps20 );
				}
			}
#ifndef _X360
			else // 3.0 shader case (PC only)
			{
				SetHWMorphVertexShaderState( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, VERTEX_SHADER_SHADER_SPECIFIC_CONST_7, SHADER_VERTEXTEXTURE_SAMPLER0 );

				depthwrite_vs30_Dynamic_Index vshIndex;
				vshIndex.SetSKINNING( pShaderAPI->GetCurrentNumBones() > 0 );
				vshIndex.SetMORPHING( pShaderAPI->IsHWMorphingEnabled() );
				vshIndex.SetCOMPRESSED_VERTS( (int)vertexCompression );
				pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );

				if ( bAlphaClip )
				{
					BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

					float vAlphaThreshold[4] = {0.7f, 0.7f, 0.7f, 0.7f};
					if ( ALPHATESTREFERENCE != -1 && ( params[ALPHATESTREFERENCE]->GetFloatValue() > 0.0f ) )
					{
						vAlphaThreshold[0] = vAlphaThreshold[1] = vAlphaThreshold[2] = vAlphaThreshold[3] = params[ALPHATESTREFERENCE]->GetFloatValue();
					}

					pShaderAPI->SetPixelShaderConstant( 0, vAlphaThreshold, 1 );
				}

				DECLARE_DYNAMIC_PIXEL_SHADER( depthwrite_ps30 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( ALPHACLIP, bAlphaClip );
				SET_DYNAMIC_PIXEL_SHADER( depthwrite_ps30 );
			}
#endif

			Vector4D vParms;

			// set up arbitrary far planes, as the real ones are too far ( 30,000 )
//			pShaderAPI->SetPSNearAndFarZ( 1 );
			vParms.x = 7.0f;		// arbitrary near
			vParms.y = 4000.0f;		// arbitrary far 
			vParms.z = 0.0f;
			vParms.w = 0.0f;
			pShaderAPI->SetPixelShaderConstant( 1, vParms.Base(), 2 );

		}	// DYNAMIC_STATE

		Draw( );
	}
END_SHADER
