//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VIEW_SCENE_H
#define VIEW_SCENE_H
#ifdef _WIN32
#pragma once
#endif


#include "convar.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "rendertexture.h"
#include "materialsystem/itexture.h"


extern ConVar mat_wireframe;
extern ConVar building_cubemaps;


// Transform into view space (translate and rotate the camera into the origin).
void ViewTransform( const Vector &worldSpace, Vector &viewSpace );

// Transform a world point into normalized screen space (X and Y from -1 to 1).
// Returns 0 if the point is behind the viewer.
int ScreenTransform( const Vector& point, Vector& screen );
int HudTransform( const Vector& point, Vector& screen );


extern ConVar r_updaterefracttexture;
extern int g_viewscene_refractUpdateFrame;
extern bool g_bAllowMultipleRefractUpdatesPerScenePerFrame;
bool DrawingShadowDepthView( void );
bool DrawingMainView();

inline void UpdateRefractTexture( int x, int y, int w, int h, bool bForceUpdate = false )
{
	Assert( !DrawingShadowDepthView() );

	if ( !IsRetail() && !r_updaterefracttexture.GetBool() )
		return;

	CMatRenderContextPtr pRenderContext( materials );
	ITexture *pTexture = GetPowerOfTwoFrameBufferTexture();
	if ( IsPC() || bForceUpdate || g_bAllowMultipleRefractUpdatesPerScenePerFrame || (gpGlobals->framecount != g_viewscene_refractUpdateFrame) )
	{
		// forced or only once per frame 
		Rect_t rect;
		rect.x = x;
		rect.y = y;
		rect.width = w;
		rect.height = h;
		pRenderContext->CopyRenderTargetToTextureEx( pTexture, 0, &rect, NULL );

		g_viewscene_refractUpdateFrame = gpGlobals->framecount;
	}
	pRenderContext->SetFrameBufferCopyTexture( pTexture );
}

inline void UpdateRefractTexture( bool bForceUpdate = false )
{
	Assert( !DrawingShadowDepthView() );

	CMatRenderContextPtr pRenderContext( materials );

	int x,y,w,h;
	pRenderContext->GetViewport( x, y, w, h );
	UpdateRefractTexture( x, y, w, h, bForceUpdate );
}

inline void UpdateScreenEffectTexture( int textureIndex, int x, int y, int w, int h, bool bDestFullScreen = false, Rect_t *pActualRect = NULL )
{
	Rect_t srcRect;
	srcRect.x = x;
	srcRect.y = y;
	srcRect.width = w;
	srcRect.height = h;

	CMatRenderContextPtr pRenderContext( materials );
	ITexture *pTexture = GetFullFrameFrameBufferTexture( textureIndex );
	int nSrcWidth, nSrcHeight;
	pRenderContext->GetRenderTargetDimensions( nSrcWidth, nSrcHeight );
	int nDestWidth = pTexture->GetActualWidth();
	int nDestHeight = pTexture->GetActualHeight();

	Rect_t destRect = srcRect;
	if( !bDestFullScreen && ( nSrcWidth > nDestWidth || nSrcHeight > nDestHeight ) )
	{
		// the source and target sizes aren't necessarily the same (specifically in dx7 where 
		// nonpow2 rendertargets aren't supported), so lets figure it out here.
		float scaleX = ( float )nDestWidth / ( float )nSrcWidth;
		float scaleY = ( float )nDestHeight / ( float )nSrcHeight;
		destRect.x = srcRect.x * scaleX;
		destRect.y = srcRect.y * scaleY;
		destRect.width = srcRect.width * scaleX;
		destRect.height = srcRect.height * scaleY;
		destRect.x = clamp( destRect.x, 0, nDestWidth );
		destRect.y = clamp( destRect.y, 0, nDestHeight );
		destRect.width = clamp( destRect.width, 0, nDestWidth - destRect.x );
		destRect.height = clamp( destRect.height, 0, nDestHeight - destRect.y );
	}

	pRenderContext->CopyRenderTargetToTextureEx( pTexture, 0, &srcRect, bDestFullScreen ? NULL : &destRect );
	pRenderContext->SetFrameBufferCopyTexture( pTexture, textureIndex );

	if ( pActualRect )
	{
		pActualRect->x = destRect.x;
		pActualRect->y = destRect.y;
		pActualRect->width = destRect.width;
		pActualRect->height = destRect.height;
	}
}

//-----------------------------------------------------------------------------
// Draws the screen effect
//-----------------------------------------------------------------------------
inline void DrawScreenEffectMaterial( IMaterial *pMaterial, int x, int y, int w, int h )
{
	Rect_t actualRect;
	UpdateScreenEffectTexture( 0, x, y, w, h, false, &actualRect );
	ITexture *pTexture = GetFullFrameFrameBufferTexture( 0 );

	CMatRenderContextPtr pRenderContext( materials );

	pRenderContext->DrawScreenSpaceRectangle( pMaterial, x, y, w, h,
		actualRect.x, actualRect.y, actualRect.x+actualRect.width-1, actualRect.y+actualRect.height-1, 
		pTexture->GetActualWidth(), pTexture->GetActualHeight() );
}


//intended for use by dynamic meshes to naively update front buffer textures needed by a material
inline void UpdateFrontBufferTexturesForMaterial( IMaterial *pMaterial, bool bForce = false )
{
	Assert( !DrawingShadowDepthView() );

	if( pMaterial->NeedsPowerOfTwoFrameBufferTexture() )
	{
		UpdateRefractTexture( bForce );
	}
	else if( pMaterial->NeedsFullFrameBufferTexture() )
	{
		const CViewSetup *pView = view->GetViewSetup();
		UpdateScreenEffectTexture( 0, pView->x, pView->y, pView->width, pView->height );
	}
}

inline void UpdateScreenEffectTexture( void )
{
	Assert( !DrawingShadowDepthView() );

	const CViewSetup *pViewSetup = view->GetViewSetup();
	UpdateScreenEffectTexture( 0, pViewSetup->x, pViewSetup->y, pViewSetup->width, pViewSetup->height);
}

// reset the tonem apping to a constant value, and clear the filter bank
void ResetToneMapping(float value);

void UpdateFullScreenDepthTexture( void );

#endif // VIEW_SCENE_H
