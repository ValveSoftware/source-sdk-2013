//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#if !defined( BEAMSEGDRAW_H )
#define BEAMSEGDRAW_H
#ifdef _WIN32
#pragma once
#endif

#define NOISE_DIVISIONS		128


#include "mathlib/vector.h"
#include "materialsystem/imesh.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
struct BeamTrail_t;
class IMaterial;


//-----------------------------------------------------------------------------
// CBeamSegDraw is a simple interface to beam rendering.
//-----------------------------------------------------------------------------
struct BeamSeg_t
{
	Vector		m_vPos;
	Vector		m_vColor;
	float		m_flTexCoord;	// Y texture coordinate
	float		m_flWidth;
	float		m_flAlpha;
};

class CBeamSegDraw
{
public:
	CBeamSegDraw() : m_pRenderContext( NULL ) {}
	// Pass null for pMaterial if you have already set the material you want.
	void			Start( IMatRenderContext *pRenderContext, int nSegs, IMaterial *pMaterial=0, CMeshBuilder *pMeshBuilder = NULL, int nMeshVertCount = 0 );
	virtual void	NextSeg( BeamSeg_t *pSeg );
	void			End();

protected:
	void			SpecifySeg( const Vector &vecCameraPos, const Vector &vNextPos );
	void			ComputeNormal( const Vector &vecCameraPos, const Vector &vStartPos, const Vector &vNextPos, Vector *pNormal );

	CMeshBuilder	*m_pMeshBuilder;
	int				m_nMeshVertCount;

	CMeshBuilder	m_Mesh;
	BeamSeg_t		m_Seg;	

	int				m_nTotalSegs;
	int				m_nSegsDrawn;

	Vector			m_vNormalLast;
	IMatRenderContext *m_pRenderContext;
};

class CBeamSegDrawArbitrary : public CBeamSegDraw
{
public:
	void			SetNormal( const Vector &normal );
	void			NextSeg( BeamSeg_t *pSeg );

protected:
	void			SpecifySeg( const Vector &vNextPos );

	BeamSeg_t		m_PrevSeg;
};

#if 0
int ScreenTransform( const Vector& point, Vector& screen );

void DrawSegs( int noise_divisions, float *prgNoise, const model_t* spritemodel,
				float frame, int rendermode, const Vector& source, const Vector& delta, 
				float startWidth, float endWidth, float scale, float freq, float speed, int segments,
				int flags, float* color, float fadeLength, float flHDRColorScale = 1.0f );
void DrawTeslaSegs( int noise_divisions, float *prgNoise, const model_t* spritemodel,
				float frame, int rendermode, const Vector& source, const Vector& delta, 
				float startWidth, float endWidth, float scale, float freq, float speed, int segments,
				int flags, float* color, float fadeLength, float flHDRColorScale = 1.0f );
void DrawSplineSegs( int noise_divisions, float *prgNoise, 
				const model_t* beammodel, const model_t* halomodel, float flHaloScale,
				float frame, int rendermode, int numAttachments, Vector* attachment, 
				float startWidth, float endWidth, float scale, float freq, float speed, int segments,
				int flags, float* color, float fadeLength, float flHDRColorScale = 1.0f );
void DrawHalo(IMaterial* pMaterial, const Vector& source, float scale, float const* color, float flHDRColorScale = 1.0f );
void BeamDrawHalo( const model_t* spritemodel, float frame, int rendermode, const Vector& source, 
				  float scale, float* color, float flHDRColorScale = 1.0f );
void DrawDisk( int noise_divisions, float *prgNoise, const model_t* spritemodel,
			  float frame, int rendermode, const Vector& source, const Vector& delta, 
			  float width, float scale, float freq, float speed, 
			  int segments, float* color, float flHDRColorScale = 1.0f );
void DrawCylinder( int noise_divisions, float *prgNoise, const model_t* spritemodel, 
				  float frame, int rendermode, const Vector& source, 
				  const Vector&  delta, float width, float scale, float freq, 
				  float speed, int segments, float* color, float flHDRColorScale = 1.0f );
void DrawRing( int noise_divisions, float *prgNoise, void (*pfnNoise)( float *noise, int divs, float scale ), 
			  const model_t* spritemodel, float frame, int rendermode, 
			  const Vector& source, const Vector& delta, float width, float amplitude, 
			  float freq, float speed, int segments, float* color, float flHDRColorScale = 1.0f );
void DrawBeamFollow( const model_t* spritemodel, BeamTrail_t* pHead, int frame, int rendermode, Vector& delta, 
					Vector& screen, Vector& screenLast, float die, const Vector& source, 
					int flags, float width, float amplitude, float freq, float* color, float flHDRColorScale = 1.0f );

void DrawBeamQuadratic( const Vector &start, const Vector &control, const Vector &end, float width, const Vector &color, float scrollOffset, float flHDRColorScale = 1.0f );
#endif

//-----------------------------------------------------------------------------
// Assumes the material has already been bound
//-----------------------------------------------------------------------------
void DrawSprite( const Vector &vecOrigin, float flWidth, float flHeight, color32 color );

#endif // BEAMDRAW_H
