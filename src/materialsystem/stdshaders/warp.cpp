//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"
#include "commandbuilder.h"

#include "warp_ps20.inc"
#include "warp_ps20b.inc"
#include "warp_vs20.inc"
#include "warp_ps30.inc"
#include "warp_vs30.inc"

#include "../materialsystem_global.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"







class CWarp_DX9_Context : public CBasePerMaterialContextData
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

	CWarp_DX9_Context( void )
	{
		m_pStaticCmds = NULL;
	}

	~CWarp_DX9_Context( void )
	{
		ResetStaticCmds();
	}

};


static const float kAllZeros[ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f };


BEGIN_VS_SHADER( warp, "Help for warp" )
	BEGIN_SHADER_PARAMS
	
		SHADER_PARAM( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )

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
	}

	SHADER_DRAW
	{
		CWarp_DX9_Context	*pContextData = reinterpret_cast< CWarp_DX9_Context *> ( *pContextDataPtr );
		bool				bNeedRegenStaticCmds = ( !pContextData ) || pShaderShadow;

		if ( !pContextData )								// make sure allocated
		{
			pContextData = new CWarp_DX9_Context;
			*pContextDataPtr = pContextData;
		}

		if ( pShaderShadow || bNeedRegenStaticCmds )
		{
			pContextData->ResetStaticCmds();
			CCommandBufferBuilder< CFixedCommandStorageBuffer< 5000 > > staticCmdsBuf;

			staticCmdsBuf.BindTexture( this, SHADER_SAMPLER0, BASETEXTURE, -1 );

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

			pShaderShadow->EnableSRGBWrite( true );
			pShaderShadow->EnableAlphaWrites( false );
			pShaderShadow->EnableAlphaTest( false );

			DefaultFog();

			int nFormat = 0;
			nFormat |= VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( nFormat, 2, 0, 0 );

			if ( !g_pHardwareConfig->SupportsShaderModel_3_0() )
			{
				DECLARE_STATIC_VERTEX_SHADER( warp_vs20 );
				SET_STATIC_VERTEX_SHADER( warp_vs20 );

				if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_STATIC_PIXEL_SHADER( warp_ps20b );
					SET_STATIC_PIXEL_SHADER( warp_ps20b );
				}
				else
				{
					DECLARE_STATIC_PIXEL_SHADER( warp_ps20 );
					SET_STATIC_PIXEL_SHADER( warp_ps20 );
				}
			}
			else
			{
				DECLARE_STATIC_VERTEX_SHADER( warp_vs30 );
				SET_STATIC_VERTEX_SHADER( warp_vs30 );

				DECLARE_STATIC_PIXEL_SHADER( warp_ps30 );
				SET_STATIC_PIXEL_SHADER( warp_ps30 );
			}
		}

		DYNAMIC_STATE
		{
			CCommandBufferBuilder< CFixedCommandStorageBuffer< 1000 > > DynamicCmdsOut;
			DynamicCmdsOut.Call( pContextData->m_pStaticCmds );
			DynamicCmdsOut.Call( pContextData->m_SemiStaticCmdsOut.Base() );

			pShaderAPI->SetDefaultState();

			Vector4D vParms;
			Vector vTemp;

			// GetVectorRenderingParameter only return a Vec3, not a Vec4, so I need to repack these :-(
			vTemp = pShaderAPI->GetVectorRenderingParameter ( VECTOR_RENDERPARM_HMDWARP_GROW_OUTIN );
			vParms.x = vTemp.x;
			vParms.y = vTemp.y;
			vTemp = pShaderAPI->GetVectorRenderingParameter ( VECTOR_RENDERPARM_HMDWARP_GROW_ABOVEBELOW );
			vParms.z = vTemp.x;
			vParms.w = vTemp.y;
			pShaderAPI->SetPixelShaderConstant( 0, vParms.Base() );

			vTemp = pShaderAPI->GetVectorRenderingParameter ( VECTOR_RENDERPARM_HMDWARP_LEFT_CENTRE );
			vParms.x = vTemp.x;
			vParms.y = vTemp.y;
			vTemp = pShaderAPI->GetVectorRenderingParameter ( VECTOR_RENDERPARM_HMDWARP_RIGHT_CENTRE );
			vParms.z = vTemp.x;
			vParms.w = vTemp.y;
			pShaderAPI->SetPixelShaderConstant( 1, vParms.Base() );

			vTemp = pShaderAPI->GetVectorRenderingParameter ( VECTOR_RENDERPARM_HMDWARP_LEFT_COEFF012 );
			vParms.x = vTemp.x;
			vParms.y = vTemp.y;
			vParms.z = vTemp.z;
			vTemp = pShaderAPI->GetVectorRenderingParameter ( VECTOR_RENDERPARM_HMDWARP_LEFT_COEFF34_RED_OFFSET );
			vParms.w = vTemp.z;		// Red offset
			// Shader doesn't support a 4th & 5th coefficient (yet?).
			pShaderAPI->SetPixelShaderConstant( 2, vParms.Base() );

			vTemp = pShaderAPI->GetVectorRenderingParameter ( VECTOR_RENDERPARM_HMDWARP_RIGHT_COEFF012 );
			vParms.x = vTemp.x;
			vParms.y = vTemp.y;
			vParms.z = vTemp.z;
			vTemp = pShaderAPI->GetVectorRenderingParameter ( VECTOR_RENDERPARM_HMDWARP_RIGHT_COEFF34_BLUE_OFFSET );
			vParms.w = vTemp.z;		// Blue offset
			// Shader doesn't support a 4th & 5th coefficient (yet?).
			pShaderAPI->SetPixelShaderConstant( 3, vParms.Base() );

			vTemp = pShaderAPI->GetVectorRenderingParameter ( VECTOR_RENDERPARM_HMDWARP_ASPECT );
			vParms.x = vTemp.x;
			vParms.y = vTemp.y;
			vParms.z = vTemp.z;
			vParms.w = 0.0f;
			pShaderAPI->SetPixelShaderConstant( 4, vParms.Base() );

			int nDistortType = pShaderAPI->GetIntRenderingParameter( INT_RENDERPARM_DISTORTION_TYPE );

			if ( !g_pHardwareConfig->SupportsShaderModel_3_0() )
			{
				DECLARE_DYNAMIC_VERTEX_SHADER( warp_vs20 );
				SET_DYNAMIC_VERTEX_SHADER( warp_vs20 );

				if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( warp_ps20b );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( DISTORT_TYPE, nDistortType );
					SET_DYNAMIC_PIXEL_SHADER( warp_ps20b );
				}
				else
				{
					DECLARE_DYNAMIC_PIXEL_SHADER( warp_ps20 );
					SET_DYNAMIC_PIXEL_SHADER_COMBO( DISTORT_TYPE, nDistortType );
					SET_DYNAMIC_PIXEL_SHADER( warp_ps20 );
				}
			}
			else
			{
				DECLARE_DYNAMIC_VERTEX_SHADER( warp_vs30 );
				SET_DYNAMIC_VERTEX_SHADER( warp_vs30 );

				DECLARE_DYNAMIC_PIXEL_SHADER( warp_ps30 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( DISTORT_TYPE, nDistortType );
				SET_DYNAMIC_PIXEL_SHADER( warp_ps30 );
			}

			DynamicCmdsOut.End();
			pShaderAPI->ExecuteCommandBuffer( DynamicCmdsOut.Base() );
		}
		Draw();
	}
END_SHADER
