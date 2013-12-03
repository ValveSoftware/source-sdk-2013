//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SPRITETRAIL_H
#define SPRITETRAIL_H
#ifdef _WIN32
#pragma once
#endif

#include "Sprite.h"

#if defined( CLIENT_DLL )
#define CSpriteTrail C_SpriteTrail
#endif


//-----------------------------------------------------------------------------
// Sprite trail
//-----------------------------------------------------------------------------
struct TrailPoint_t
{
	DECLARE_SIMPLE_DATADESC();

	Vector	m_vecScreenPos;
	float	m_flDieTime;
	float	m_flTexCoord;
	float	m_flWidthVariance;
};

class CSpriteTrail : public CSprite
{
	DECLARE_CLASS( CSpriteTrail, CSprite );
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

public:
	CSpriteTrail( void );

	// Sets parameters of the sprite trail
	void SetLifeTime( float time );
	void SetStartWidth( float flStartWidth );
	void SetEndWidth( float flEndWidth );
	void SetStartWidthVariance( float flStartWidthVariance );
	void SetTextureResolution( float flTexelsPerInch );
	void SetMinFadeLength( float flMinFadeLength );
	void SetSkybox( const Vector &vecSkyboxOrigin, float flSkyboxScale );

	// Is the trail in the skybox?
	bool IsInSkybox() const;
	void Spawn( void );
	void Precache( void );
	void SetTransmit( bool bTransmit = true ) { m_bDrawForMoveParent = bTransmit; }

#if defined( CLIENT_DLL ) 
	// Client only code
	virtual int DrawModel( int flags );
	virtual const Vector &GetRenderOrigin( void );
	virtual const QAngle &GetRenderAngles( void );

	// On data update
	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void GetRenderBounds( Vector& mins, Vector& maxs );
	virtual void ClientThink();

	virtual bool ValidateEntityAttachedToPlayer( bool &bShouldRetry );

#else
	// Server only code

	virtual int ShouldTransmit( const CCheckTransmitInfo *pInfo );
	static CSpriteTrail *SpriteTrailCreate( const char *pSpriteName, const Vector &origin, bool animate );

#endif

private:
#if defined( CLIENT_DLL )
	enum
	{
		// NOTE: # of points max must be a power of two!
		MAX_SPRITE_TRAIL_POINTS	= 64,
		MAX_SPRITE_TRAIL_MASK = 0x3F,
	};

	TrailPoint_t *GetTrailPoint( int n );
	void	UpdateTrail( void );
	void	ComputeScreenPosition( Vector *pScreenPos );
	void	ConvertSkybox();
	void	UpdateBoundingBox( void );

	TrailPoint_t	m_vecSteps[MAX_SPRITE_TRAIL_POINTS];
	int	m_nFirstStep;
	int m_nStepCount;
	float m_flUpdateTime;
	Vector m_vecPrevSkyboxOrigin;
	float m_flPrevSkyboxScale;
	Vector m_vecRenderMins;
	Vector m_vecRenderMaxs;
#endif

	CNetworkVar( float, m_flLifeTime );	// Amount of time before a new trail segment fades away
	CNetworkVar( float, m_flStartWidth );	// The starting scale
	CNetworkVar( float, m_flEndWidth );	// The ending scale
	CNetworkVar( float, m_flStartWidthVariance );	// The starting scale
	CNetworkVar( float, m_flTextureRes );	// Texture resolution along the trail
	CNetworkVar( float, m_flMinFadeLength );	// The end of the trail must fade out for this many units
	CNetworkVector( m_vecSkyboxOrigin );	// What's our skybox origin?
	CNetworkVar( float, m_flSkyboxScale );	// What's our skybox scale?

	string_t m_iszSpriteName;
	bool	m_bAnimate;
	bool	m_bDrawForMoveParent;
};

#endif // SPRITETRAIL_H
