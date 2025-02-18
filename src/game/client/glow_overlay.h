//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GLOW_OVERLAY_H
#define GLOW_OVERLAY_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"
#include "materialsystem/imaterial.h"
#include "sun_shared.h"
#include "c_pixel_visibility.h"

#ifdef PORTAL
#include "c_prop_portal.h" //MAX_PORTAL_RECURSIVE_VIEWS
#endif

extern float g_flOverlayRange;

class CGlowSprite
{
public:
	Vector				m_vColor;		// 0-1
	float				m_flHorzSize;	// Horizontal and vertical sizes.
	float				m_flVertSize;	// 1 = size of the sun
	IMaterial			*m_pMaterial;	// Material to use
};


class CGlowOverlay
{
public:

					CGlowOverlay();
	virtual			~CGlowOverlay();

	// Return false to remove (and delete) the overlay.
	virtual bool	Update();


// Data the creator should fill in.
public:

	// Position of the light source. If m_bDirectional is set, then it ignores
	// this and uses m_vDirection as the direction of the light source.
	Vector		m_vPos;

	// Optional direction. Used for really far away things like the sun.
	// The direction should point AT the light source.
	bool		m_bDirectional;
	Vector		m_vDirection;

	// If this is set, then the overlay is only visible if the ray to it hits the sky.
	bool		m_bInSky;
	float		m_skyObstructionScale;
#ifdef PORTAL
	float		m_skyObstructionScaleBackups[MAX_PORTAL_RECURSIVE_VIEWS]; //used in portal mod during stencil rendering to maintain obstructions while rendering recursive views
#endif

	CGlowSprite	m_Sprites[MAX_SUN_LAYERS];
	int			m_nSprites;

	float		m_flProxyRadius;

	float		m_flHDRColorScale;

public:

	// After creating the overlay, call this to add it to the active list.
	// You can also call Activate and Deactivate as many times as you want.
	void			Activate();
	void			Deactivate();
	
	// Render all the active overlays.
	static void		DrawOverlays( bool bCacheFullSceneState );
	static void		UpdateSkyOverlays( float zFar, bool bCacheFullSceneState );

#ifdef PORTAL
	static void		BackupSkyOverlayData( int iBackupToSlot );
	static void		RestoreSkyOverlayData( int iRestoreFromSlot );
#endif

protected:

	void			UpdateGlowObstruction( const Vector &vToGlow, bool bCacheFullSceneState );
	void			UpdateSkyGlowObstruction( float zFar, bool bCacheFullSceneState );

	virtual void	CalcSpriteColorAndSize( 
		float flDot,
		CGlowSprite *pSprite, 
		float *flHorzSize, 
		float *flVertSize, 
		Vector *vColor );

	virtual void	CalcBasis( 
		const Vector &vToGlow,
		float flHorzSize,
		float flVertSize,
		Vector &vBasePt,
		Vector &vUp,
		Vector &vRight );

	virtual void	Draw( bool bCacheFullSceneState );
	float			CalcGlowAspect();
	
	float			m_flGlowObstructionScale;	
	bool			m_bCacheGlowObstruction;			// Flags to cache obstruction scales
	bool			m_bCacheSkyObstruction;				// Used in IFM poster rendering

private:
	short			m_bActivated;
	unsigned short	m_ListIndex; // index into s_GlowOverlays.
	pixelvis_handle_t m_queryHandle;
};

//Override for warping
class CWarpOverlay : public CGlowOverlay
{
protected:
	
	virtual void Draw( bool bCacheFullSceneState );
};

#endif // GLOW_OVERLAY_H
