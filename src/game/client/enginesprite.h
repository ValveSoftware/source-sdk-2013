//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef ENGINESPRITE_H
#define ENGINESPRITE_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"
#include "video/ivideoservices.h"
#include "const.h"
//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IMaterial;
class IMaterialVar;
typedef struct wrect_s wrect_t;


//-----------------------------------------------------------------------------
// Purpose: Sprite Models
//-----------------------------------------------------------------------------
class CEngineSprite
{
	// NOTE: don't define a constructor or destructor so that this can be allocated
	// as before.
public:
	int GetWidth() { return m_width; }
	int GetHeight() { return m_height; }
	int GetNumFrames() { return m_numFrames; }
	IMaterial *GetMaterial( RenderMode_t nRenderMode ) { return m_material[nRenderMode]; }
	IMaterial *GetMaterial( RenderMode_t nRenderMode, int nFrame );
	void SetFrame( RenderMode_t nRenderMode, int nFrame );
	bool Init( const char *name );
	void Shutdown( void );
	void UnloadMaterial();
	void SetColor( float r, float g, float b );
	int GetOrientation( void );
	void GetHUDSpriteColor( float* color );
	float GetUp() { return up; }
	float GetDown() { return down; }
	float GetLeft() { return left; }
	float GetRight() { return right; }
	void DrawFrame( RenderMode_t nRenderMode, int frame, int x, int y, const wrect_t *prcSubRect );
	void DrawFrameOfSize( RenderMode_t nRenderMode, int frame, int x, int y, int iWidth, int iHeight, const wrect_t *prcSubRect);
	bool		IsVideo();
	void GetTexCoordRange( float *pMinU, float *pMinV, float *pMaxU, float *pMaxV );

private:
	IVideoMaterial *m_VideoMaterial;
	int m_width;
	int m_height;
	int m_numFrames;
	IMaterial *m_material[kRenderModeCount];
	int m_orientation;
	float m_hudSpriteColor[3];
	float up, down, left, right;
};

#endif // ENGINESPRITE_H
