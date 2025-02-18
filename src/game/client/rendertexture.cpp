//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
// Implements local hooks into named renderable textures.
// See matrendertexture.cpp in material system for list of available RT's
//
//=============================================================================//

#include "materialsystem/imesh.h"
#include "materialsystem/itexture.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "tier1/strtools.h"
#include "rendertexture.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void ReleaseRenderTargets( void );

void AddReleaseFunc( void )
{
	static bool bAdded = false;
	if( !bAdded )
	{
		bAdded = true;
		materials->AddReleaseFunc( ReleaseRenderTargets );
	}
}

//=============================================================================
// Power of Two Frame Buffer Texture
//=============================================================================
static CTextureReference s_pPowerOfTwoFrameBufferTexture;
ITexture *GetPowerOfTwoFrameBufferTexture( void )
{
	if ( IsX360() )
	{
		return GetFullFrameFrameBufferTexture( 1 );
	}

	if ( !s_pPowerOfTwoFrameBufferTexture )
	{
		s_pPowerOfTwoFrameBufferTexture.Init( materials->FindTexture( "_rt_PowerOfTwoFB", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pPowerOfTwoFrameBufferTexture ) );
		AddReleaseFunc();
	}
	
	return s_pPowerOfTwoFrameBufferTexture;
}

//=============================================================================
// Fullscreen Texture
//=============================================================================
static CTextureReference s_pFullscreenTexture;
ITexture *GetFullscreenTexture( void )
{
	if ( !s_pFullscreenTexture )
	{
		s_pFullscreenTexture.Init( materials->FindTexture( "_rt_Fullscreen", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pFullscreenTexture ) );
		AddReleaseFunc();
	}

	return s_pFullscreenTexture;
}

//=============================================================================
// Camera Texture
//=============================================================================
static CTextureReference s_pCameraTexture;
ITexture *GetCameraTexture( void )
{
	if ( !s_pCameraTexture )
	{
		s_pCameraTexture.Init( materials->FindTexture( "_rt_Camera", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pCameraTexture ) );
		AddReleaseFunc();
	}
	
	return s_pCameraTexture;
}

//=============================================================================
// Full Frame Depth Texture
//=============================================================================
static CTextureReference s_pFullFrameDepthTexture;
ITexture *GetFullFrameDepthTexture( void )
{
	if ( !s_pFullFrameDepthTexture )
	{
		s_pFullFrameDepthTexture.Init( materials->FindTexture( "_rt_FullFrameDepth", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pFullFrameDepthTexture ) );
		AddReleaseFunc();
	}

	return s_pFullFrameDepthTexture;
}

//=============================================================================
// Full Frame Buffer Textures
//=============================================================================
static CTextureReference s_pFullFrameFrameBufferTexture[MAX_FB_TEXTURES];
ITexture *GetFullFrameFrameBufferTexture( int textureIndex )
{
	if ( !s_pFullFrameFrameBufferTexture[textureIndex] )
	{
		char name[256];
		if( textureIndex != 0 )
		{
			V_sprintf_safe( name, "_rt_FullFrameFB%d", textureIndex );
		}
		else
		{
			V_strcpy_safe( name, "_rt_FullFrameFB" );
		}
		s_pFullFrameFrameBufferTexture[textureIndex].Init( materials->FindTexture( name, TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pFullFrameFrameBufferTexture[textureIndex] ) );
		AddReleaseFunc();
	}
	
	return s_pFullFrameFrameBufferTexture[textureIndex];
}


//=============================================================================
// Water reflection
//=============================================================================
static CTextureReference s_pWaterReflectionTexture;
ITexture *GetWaterReflectionTexture( void )
{
	if ( !s_pWaterReflectionTexture )
	{
		s_pWaterReflectionTexture.Init( materials->FindTexture( "_rt_WaterReflection", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pWaterReflectionTexture ) );
		AddReleaseFunc();
	}
	
	return s_pWaterReflectionTexture;
}

//=============================================================================
// Water refraction
//=============================================================================
static CTextureReference s_pWaterRefractionTexture;
ITexture *GetWaterRefractionTexture( void )
{
	if ( !s_pWaterRefractionTexture )
	{
		s_pWaterRefractionTexture.Init( materials->FindTexture( "_rt_WaterRefraction", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pWaterRefractionTexture ) );
		AddReleaseFunc();
	}
	
	return s_pWaterRefractionTexture;
}

//=============================================================================
// Small Buffer HDR0
//=============================================================================
static CTextureReference s_pSmallBufferHDR0;
ITexture *GetSmallBufferHDR0( void )
{
	if ( !s_pSmallBufferHDR0 )
	{
		s_pSmallBufferHDR0.Init( materials->FindTexture( "_rt_SmallHDR0", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pSmallBufferHDR0 ) );
		AddReleaseFunc();
	}
	
	return s_pSmallBufferHDR0;
}

//=============================================================================
// Small Buffer HDR1
//=============================================================================
static CTextureReference s_pSmallBufferHDR1;
ITexture *GetSmallBufferHDR1( void )
{
	if ( !s_pSmallBufferHDR1 )
	{
		s_pSmallBufferHDR1.Init( materials->FindTexture( "_rt_SmallHDR1", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pSmallBufferHDR1 ) );
		AddReleaseFunc();
	}
	
	return s_pSmallBufferHDR1;
}

//=============================================================================
// Quarter Sized FB0
//=============================================================================
static CTextureReference s_pQuarterSizedFB0;
ITexture *GetSmallBuffer0( void )
{
	if ( !s_pQuarterSizedFB0 )
	{
		s_pQuarterSizedFB0.Init( materials->FindTexture( "_rt_SmallFB0", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pQuarterSizedFB0 ) );
		AddReleaseFunc();
	}
	
	return s_pQuarterSizedFB0;
}

//=============================================================================
// Quarter Sized FB1
//=============================================================================
static CTextureReference s_pQuarterSizedFB1;
ITexture *GetSmallBuffer1( void )
{
	if ( !s_pQuarterSizedFB1 )
	{
		s_pQuarterSizedFB1.Init( materials->FindTexture( "_rt_SmallFB1", TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_pQuarterSizedFB1 ) );
		AddReleaseFunc();
	}
	
	return s_pQuarterSizedFB1;
}

//=============================================================================
// Teeny Textures
//=============================================================================
static CTextureReference s_TeenyTextures[MAX_TEENY_TEXTURES];
ITexture *GetTeenyTexture( int which )
{
	if ( IsX360() )
	{
		Assert( 0 );
		return NULL;
	}

	Assert( which < MAX_TEENY_TEXTURES );

	if ( !s_TeenyTextures[which] )
	{
		char nbuf[20];
		sprintf( nbuf, "_rt_TeenyFB%d", which );
		s_TeenyTextures[which].Init( materials->FindTexture( nbuf, TEXTURE_GROUP_RENDER_TARGET ) );
		Assert( !IsErrorTexture( s_TeenyTextures[which] ) );
		AddReleaseFunc();
	}
	return s_TeenyTextures[which];
}

void ReleaseRenderTargets( void )
{
	s_pPowerOfTwoFrameBufferTexture.Shutdown();
	s_pCameraTexture.Shutdown();
	s_pWaterReflectionTexture.Shutdown();
	s_pWaterRefractionTexture.Shutdown();
	s_pQuarterSizedFB0.Shutdown();
	s_pQuarterSizedFB1.Shutdown();
	s_pFullFrameDepthTexture.Shutdown();

	for (int i=0; i<MAX_FB_TEXTURES; ++i)
		s_pFullFrameFrameBufferTexture[i].Shutdown();
}
