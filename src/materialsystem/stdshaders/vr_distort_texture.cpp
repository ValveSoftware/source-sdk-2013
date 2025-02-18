//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"
#include "commandbuilder.h"

#include "vr_distort_texture_ps20.inc"
#include "vr_distort_texture_ps20b.inc"
#include "vr_distort_texture_vs20.inc"
#include "vr_distort_texture_ps30.inc"
#include "vr_distort_texture_vs30.inc"

#include "../materialsystem_global.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"







class CVRDistortTexture_DX9_Context : public CBasePerMaterialContextData
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

	CVRDistortTexture_DX9_Context( void )
	{
		m_pStaticCmds = NULL;
	}

	~CVRDistortTexture_DX9_Context( void )
	{
		ResetStaticCmds();
	}

};


static const float kAllZeros[ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f };


BEGIN_VS_SHADER( vr_distort_texture, "Help for warp" )
	BEGIN_SHADER_PARAMS
	
		SHADER_PARAM( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( DISTORTMAP, SHADER_PARAM_TYPE_TEXTURE, "vr_distort_map", "" )
		SHADER_PARAM( USERENDERTARGET, SHADER_PARAM_TYPE_INTEGER, "0", "" )

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
		LoadTexture( BASETEXTURE, TEXTUREFLAGS_SRGB );
		LoadTexture( DISTORTMAP, TEXTUREFLAGS_NOMIP | TEXTUREFLAGS_NOLOD | TEXTUREFLAGS_NODEBUGOVERRIDE |
			TEXTUREFLAGS_SINGLECOPY | TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT );
	}

	SHADER_DRAW
	{
		CVRDistortTexture_DX9_Context	*pContextData = reinterpret_cast< CVRDistortTexture_DX9_Context *> ( *pContextDataPtr );
		bool				bNeedRegenStaticCmds = ( !pContextData ) || pShaderShadow;

		if ( !pContextData )								// make sure allocated
		{
			pContextData = new CVRDistortTexture_DX9_Context;
			*pContextDataPtr = pContextData;
		}

		if ( pShaderShadow || bNeedRegenStaticCmds )
		{
			pContextData->ResetStaticCmds();
			CCommandBufferBuilder< CFixedCommandStorageBuffer< 5000 > > staticCmdsBuf;

			staticCmdsBuf.BindTexture( this, SHADER_SAMPLER0, BASETEXTURE, -1 );
			staticCmdsBuf.BindTexture( this, SHADER_SAMPLER1, DISTORTMAP, -1 );

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

			pContextData->m_SemiStaticCmdsOut.SetAmbientCubeDynamicStateVertexShader();
			pContextData->m_SemiStaticCmdsOut.End();
		}

		SHADOW_STATE
		{
			SetInitialShadowState( );

			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableDepthTest( false );

			pShaderShadow->EnableBlending( false );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );

			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

			pShaderShadow->EnableSRGBWrite( true );
			pShaderShadow->EnableAlphaWrites( false );
			pShaderShadow->EnableAlphaTest( false );

			DefaultFog();

			int nFormat = 0;
			nFormat |= VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( nFormat, 2, 0, 0 );

			if ( !g_pHardwareConfig->SupportsShaderModel_3_0() )
			{
				DECLARE_STATIC_VERTEX_SHADER( vr_distort_texture_vs20 );
				SET_STATIC_VERTEX_SHADER( vr_distort_texture_vs20 );

				if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_STATIC_PIXEL_SHADER( vr_distort_texture_ps20b );
					SET_STATIC_PIXEL_SHADER( vr_distort_texture_ps20b );
				}
				else
				{
					DECLARE_STATIC_PIXEL_SHADER( vr_distort_texture_ps20 );
					SET_STATIC_PIXEL_SHADER( vr_distort_texture_ps20 );
				}
			}
			else
			{
				DECLARE_STATIC_VERTEX_SHADER( vr_distort_texture_vs30 );
				SET_STATIC_VERTEX_SHADER( vr_distort_texture_vs30 );

				DECLARE_STATIC_PIXEL_SHADER( vr_distort_texture_ps30 );
				SET_STATIC_PIXEL_SHADER( vr_distort_texture_ps30 );
			}
		}

		DYNAMIC_STATE
		{
			CCommandBufferBuilder< CFixedCommandStorageBuffer< 1000 > > DynamicCmdsOut;
			DynamicCmdsOut.Call( pContextData->m_pStaticCmds );
			DynamicCmdsOut.Call( pContextData->m_SemiStaticCmdsOut.Base() );

			pShaderAPI->SetDefaultState();

			int useRenderTarget = ( params[ USERENDERTARGET ]->GetIntValue() == 0 ) ? 0 : 1;

			if ( !g_pHardwareConfig->SupportsShaderModel_3_0() )
			{
				DECLARE_DYNAMIC_VERTEX_SHADER( vr_distort_texture_vs20 );
				SET_DYNAMIC_VERTEX_SHADER( vr_distort_texture_vs20 );

				if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( vr_distort_texture_ps20b );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( CMBO_USERENDERTARGET, useRenderTarget );
					SET_DYNAMIC_PIXEL_SHADER( vr_distort_texture_ps20b );
				}
				else
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( vr_distort_texture_ps20 );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( CMBO_USERENDERTARGET, useRenderTarget );
					SET_DYNAMIC_PIXEL_SHADER( vr_distort_texture_ps20 );
				}
			}
			else
			{
				DECLARE_DYNAMIC_VERTEX_SHADER( vr_distort_texture_vs30 );
				SET_DYNAMIC_VERTEX_SHADER( vr_distort_texture_vs30 );

				DECLARE_DYNAMIC_PIXEL_SHADER( vr_distort_texture_ps30 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( CMBO_USERENDERTARGET, useRenderTarget );
				SET_DYNAMIC_PIXEL_SHADER( vr_distort_texture_ps30 );
			}

			DynamicCmdsOut.End();
			pShaderAPI->ExecuteCommandBuffer( DynamicCmdsOut.Base() );
		}
		Draw();
	}
END_SHADER
