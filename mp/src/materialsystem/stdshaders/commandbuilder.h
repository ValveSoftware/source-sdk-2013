//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
// Utility class for building command buffers into memory
//===========================================================================//

#ifndef COMMANDBUILDER_H
#define COMMANDBUILDER_H

#ifndef COMMANDBUFFER_H
#include "shaderapi/commandbuffer.h"
#endif

#include "BaseVSShader.h"
#include "shaderapi/ishaderapi.h"

#ifdef _WIN32
#pragma once
#endif

extern ConVar	my_mat_fullbright;

template<int N> class CFixedCommandStorageBuffer
{
public:
	uint8 m_Data[N];

	uint8 *m_pDataOut;
#ifdef DBGFLAG_ASSERT
	size_t m_nNumBytesRemaining;
#endif
	
	FORCEINLINE CFixedCommandStorageBuffer( void )
	{
		m_pDataOut = m_Data;
#ifdef DBGFLAG_ASSERT
		m_nNumBytesRemaining = N;
#endif

	}

	FORCEINLINE void EnsureCapacity( size_t sz )
	{
		Assert( m_nNumBytesRemaining >= sz );
	}

	template<class T> FORCEINLINE void Put( T const &nValue )
	{
		EnsureCapacity( sizeof( T ) );
		*( reinterpret_cast<T *>( m_pDataOut ) ) = nValue;
		m_pDataOut += sizeof( nValue );
#ifdef DBGFLAG_ASSERT
		m_nNumBytesRemaining -= sizeof( nValue );
#endif
	}

	FORCEINLINE void PutInt( int nValue )
	{
		Put( nValue );
	}

	FORCEINLINE void PutFloat( float nValue )
	{
		Put( nValue );
	}

	FORCEINLINE void PutPtr( void * pPtr )
	{
		Put( pPtr );
	}

	FORCEINLINE void PutMemory( const void *pMemory, size_t nBytes )
	{
		EnsureCapacity( nBytes );
		memcpy( m_pDataOut, pMemory, nBytes );
		m_pDataOut += nBytes;
	}

	FORCEINLINE uint8 *Base( void )
	{
		return m_Data;
	}

	FORCEINLINE void Reset( void )
	{
		m_pDataOut = m_Data;
#ifdef DBGFLAG_ASSERT
		m_nNumBytesRemaining = N;
#endif
	}

	FORCEINLINE size_t Size( void ) const
	{
		return m_pDataOut - m_Data;
	}

};

template<class S> class CCommandBufferBuilder
{
public:
	S m_Storage;

	FORCEINLINE void End( void )
	{
		m_Storage.PutInt( CBCMD_END );
	}


	FORCEINLINE IMaterialVar *Param( int nVar ) const
	{
		return CBaseShader::s_ppParams[nVar];
	}

	FORCEINLINE void SetPixelShaderConstants( int nFirstConstant, int nConstants )
	{
		m_Storage.PutInt( CBCMD_SET_PIXEL_SHADER_FLOAT_CONST );
		m_Storage.PutInt( nFirstConstant );
		m_Storage.PutInt( nConstants );
	}

	FORCEINLINE void OutputConstantData( float const *pSrcData )
	{
		m_Storage.PutFloat( pSrcData[0] );
		m_Storage.PutFloat( pSrcData[1] );
		m_Storage.PutFloat( pSrcData[2] );
		m_Storage.PutFloat( pSrcData[3] );
	}

	FORCEINLINE void OutputConstantData4( float flVal0, float flVal1, float flVal2, float flVal3 )
	{
		m_Storage.PutFloat( flVal0 );
		m_Storage.PutFloat( flVal1 );
		m_Storage.PutFloat( flVal2 );
		m_Storage.PutFloat( flVal3 );
	}

	FORCEINLINE void SetPixelShaderConstant( int nFirstConstant, float const *pSrcData, int nNumConstantsToSet )
	{
		SetPixelShaderConstants( nFirstConstant, nNumConstantsToSet );
		m_Storage.PutMemory( pSrcData, 4 * sizeof( float ) * nNumConstantsToSet );
	}

	FORCEINLINE void SetPixelShaderConstant( int nFirstConstant, int nVar )
	{
		SetPixelShaderConstant( nFirstConstant, Param( nVar )->GetVecValue() );
	}

	void SetPixelShaderConstantGammaToLinear( int pixelReg, int constantVar )
	{
		float val[4];
		Param(constantVar)->GetVecValue( val, 3 );
		val[0] = val[0] > 1.0f ? val[0] : GammaToLinear( val[0] );
		val[1] = val[1] > 1.0f ? val[1] : GammaToLinear( val[1] );
		val[2] = val[2] > 1.0f ? val[2] : GammaToLinear( val[2] );
		val[3] = 1.0;
		SetPixelShaderConstant( pixelReg, val );
	}

	FORCEINLINE void SetPixelShaderConstant( int nFirstConstant, float const *pSrcData )
	{
		SetPixelShaderConstants( nFirstConstant, 1 );
		OutputConstantData( pSrcData );
	}

	FORCEINLINE void SetPixelShaderConstant4( int nFirstConstant, float flVal0, float flVal1, float flVal2, float flVal3 )
	{
		SetPixelShaderConstants( nFirstConstant, 1 );
		OutputConstantData4( flVal0, flVal1, flVal2, flVal3 );
	}

	FORCEINLINE void SetPixelShaderConstant_W( int pixelReg, int constantVar, float fWValue )
	{
		if ( constantVar != -1 )
		{
			float val[3];
			Param(constantVar)->GetVecValue( val, 3);
			SetPixelShaderConstant4( pixelReg, val[0], val[1], val[2], fWValue );
		}
	}

	FORCEINLINE void SetVertexShaderConstant( int nFirstConstant, float const *pSrcData )
	{
		m_Storage.PutInt( CBCMD_SET_VERTEX_SHADER_FLOAT_CONST );
		m_Storage.PutInt( nFirstConstant );
		m_Storage.PutInt( 1 );
		OutputConstantData( pSrcData );
	}

	FORCEINLINE void SetVertexShaderConstant( int nFirstConstant, float const *pSrcData, int nConsts )
	{
		m_Storage.PutInt( CBCMD_SET_VERTEX_SHADER_FLOAT_CONST );
		m_Storage.PutInt( nFirstConstant );
		m_Storage.PutInt( nConsts );
		m_Storage.PutMemory( pSrcData, 4 * nConsts * sizeof( float ) );
	}


	FORCEINLINE void SetVertexShaderConstant4( int nFirstConstant, float flVal0, float flVal1, float flVal2, float flVal3 )
	{
		m_Storage.PutInt( CBCMD_SET_VERTEX_SHADER_FLOAT_CONST );
		m_Storage.PutInt( nFirstConstant );
		m_Storage.PutInt( 1 );
		m_Storage.PutFloat( flVal0 );
		m_Storage.PutFloat( flVal1 );
		m_Storage.PutFloat( flVal2 );
		m_Storage.PutFloat( flVal3 );
	}

	void SetVertexShaderTextureTransform( int vertexReg, int transformVar )
	{
		Vector4D transformation[2];
		IMaterialVar* pTransformationVar = Param( transformVar );
		if (pTransformationVar && (pTransformationVar->GetType() == MATERIAL_VAR_TYPE_MATRIX))
		{
			const VMatrix &mat = pTransformationVar->GetMatrixValue();
			transformation[0].Init( mat[0][0], mat[0][1], mat[0][2], mat[0][3] );
			transformation[1].Init( mat[1][0], mat[1][1], mat[1][2], mat[1][3] );
		}
		else
		{
			transformation[0].Init( 1.0f, 0.0f, 0.0f, 0.0f );
			transformation[1].Init( 0.0f, 1.0f, 0.0f, 0.0f );
		}
		SetVertexShaderConstant( vertexReg, transformation[0].Base(), 2 ); 
	}


	void SetVertexShaderTextureScaledTransform( int vertexReg, int transformVar, int scaleVar )
	{
		Vector4D transformation[2];
		IMaterialVar* pTransformationVar = Param( transformVar );
		if (pTransformationVar && (pTransformationVar->GetType() == MATERIAL_VAR_TYPE_MATRIX))
		{
			const VMatrix &mat = pTransformationVar->GetMatrixValue();
			transformation[0].Init( mat[0][0], mat[0][1], mat[0][2], mat[0][3] );
			transformation[1].Init( mat[1][0], mat[1][1], mat[1][2], mat[1][3] );
		}
		else
		{
			transformation[0].Init( 1.0f, 0.0f, 0.0f, 0.0f );
			transformation[1].Init( 0.0f, 1.0f, 0.0f, 0.0f );
		}

		Vector2D scale( 1, 1 );
		IMaterialVar* pScaleVar = Param( scaleVar );
		if (pScaleVar)
		{
			if (pScaleVar->GetType() == MATERIAL_VAR_TYPE_VECTOR)
				pScaleVar->GetVecValue( scale.Base(), 2 );
			else if (pScaleVar->IsDefined())
				scale[0] = scale[1] = pScaleVar->GetFloatValue();
		}

		// Apply the scaling
		transformation[0][0] *= scale[0];
		transformation[0][1] *= scale[1];
		transformation[1][0] *= scale[0];
		transformation[1][1] *= scale[1];
		transformation[0][3] *= scale[0];
		transformation[1][3] *= scale[1];
		SetVertexShaderConstant( vertexReg, transformation[0].Base(), 2 ); 
	}

	FORCEINLINE void SetEnvMapTintPixelShaderDynamicState( int pixelReg, int tintVar )
	{
		if( g_pConfig->bShowSpecular && my_mat_fullbright.GetInt() != 2 )
		{
			SetPixelShaderConstant( pixelReg, Param( tintVar)->GetVecValue() );
		}
		else
		{
			SetPixelShaderConstant4( pixelReg, 0.0, 0.0, 0.0, 0.0 );
		}
	}

	FORCEINLINE void SetEnvMapTintPixelShaderDynamicStateGammaToLinear( int pixelReg, int tintVar, float flAlphaValue = 1.0 )
	{
		if( ( tintVar != -1 ) && g_pConfig->bShowSpecular && my_mat_fullbright.GetInt() != 2 )
		{
			float color[4];
			color[3] = flAlphaValue;
			Param( tintVar)->GetLinearVecValue( color, 3 );
			SetPixelShaderConstant( pixelReg, color );
		}
		else
		{
			SetPixelShaderConstant4( pixelReg, 0.0, 0.0, 0.0, flAlphaValue );
		}
	}

	FORCEINLINE void StoreEyePosInPixelShaderConstant( int nConst )
	{
		m_Storage.PutInt( CBCMD_STORE_EYE_POS_IN_PSCONST );
		m_Storage.PutInt( nConst );
	}

	FORCEINLINE void CommitPixelShaderLighting( int nConst )
	{
		m_Storage.PutInt( CBCMD_COMMITPIXELSHADERLIGHTING );
		m_Storage.PutInt( nConst );
	}

	FORCEINLINE void SetPixelShaderStateAmbientLightCube( int nConst )
	{
		m_Storage.PutInt( CBCMD_SETPIXELSHADERSTATEAMBIENTLIGHTCUBE );
		m_Storage.PutInt( nConst );
	}

	FORCEINLINE void SetAmbientCubeDynamicStateVertexShader( void )
	{
		m_Storage.PutInt( CBCMD_SETAMBIENTCUBEDYNAMICSTATEVERTEXSHADER );
	}

	FORCEINLINE void SetPixelShaderFogParams( int nReg )
	{
		m_Storage.PutInt( CBCMD_SETPIXELSHADERFOGPARAMS );
		m_Storage.PutInt( nReg );
	}

	FORCEINLINE void BindStandardTexture( Sampler_t nSampler, StandardTextureId_t nTextureId )
	{
		m_Storage.PutInt( CBCMD_BIND_STANDARD_TEXTURE );
		m_Storage.PutInt( nSampler );
		m_Storage.PutInt( nTextureId );
	}


	FORCEINLINE void BindTexture( Sampler_t nSampler, ShaderAPITextureHandle_t hTexture )
	{
		Assert( hTexture != INVALID_SHADERAPI_TEXTURE_HANDLE );
		if ( hTexture != INVALID_SHADERAPI_TEXTURE_HANDLE )
		{
			m_Storage.PutInt( CBCMD_BIND_SHADERAPI_TEXTURE_HANDLE );
			m_Storage.PutInt( nSampler );
			m_Storage.PutInt( hTexture );
		}
	}

	FORCEINLINE void BindTexture( CBaseVSShader *pShader, Sampler_t nSampler, int nTextureVar, int nFrameVar )
	{
		ShaderAPITextureHandle_t hTexture = pShader->GetShaderAPITextureBindHandle( nTextureVar, nFrameVar );
		BindTexture( nSampler, hTexture );
	}

	FORCEINLINE void BindMultiTexture( CBaseVSShader *pShader, Sampler_t nSampler1, Sampler_t nSampler2, int nTextureVar, int nFrameVar )
	{
		ShaderAPITextureHandle_t hTexture = pShader->GetShaderAPITextureBindHandle( nTextureVar, nFrameVar, 0 );
		BindTexture( nSampler1, hTexture );
		hTexture = pShader->GetShaderAPITextureBindHandle( nTextureVar, nFrameVar, 1 );
		BindTexture( nSampler2, hTexture );
	}

	FORCEINLINE void SetPixelShaderIndex( int nIndex )
	{
		m_Storage.PutInt( CBCMD_SET_PSHINDEX );
		m_Storage.PutInt( nIndex );
	}

	FORCEINLINE void SetVertexShaderIndex( int nIndex )
	{
		m_Storage.PutInt( CBCMD_SET_VSHINDEX );
		m_Storage.PutInt( nIndex );
	}

	FORCEINLINE void SetDepthFeatheringPixelShaderConstant( int iConstant, float fDepthBlendScale )
	{
		m_Storage.PutInt( CBCMD_SET_DEPTH_FEATHERING_CONST );
		m_Storage.PutInt( iConstant );
		m_Storage.PutFloat( fDepthBlendScale );
	}

	FORCEINLINE void Goto( uint8 *pCmdBuf )
	{
		m_Storage.PutInt( CBCMD_JUMP );
		m_Storage.PutPtr( pCmdBuf );
	}

	FORCEINLINE void Call( uint8 *pCmdBuf )
	{
		m_Storage.PutInt( CBCMD_JSR );
		m_Storage.PutPtr( pCmdBuf );
	}

	FORCEINLINE void Reset( void )
	{
		m_Storage.Reset();
	}

	FORCEINLINE size_t Size( void ) const
	{
		return m_Storage.Size();
	}

	FORCEINLINE uint8 *Base( void )
	{
		return m_Storage.Base();
	}



};


#endif // commandbuilder_h
