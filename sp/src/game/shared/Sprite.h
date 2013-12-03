//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SPRITE_H
#define SPRITE_H
#ifdef _WIN32
#pragma once
#endif

#include "predictable_entity.h"
#include "baseentity_shared.h"

#define SF_SPRITE_STARTON		0x0001
#define SF_SPRITE_ONCE			0x0002
#define SF_SPRITE_TEMPORARY		0x8000

class CBasePlayer;

#if defined( CLIENT_DLL )
#define CSprite C_Sprite
#define CSpriteOriented C_SpriteOriented
#include "c_pixel_visibility.h"
class CEngineSprite;

class C_SpriteRenderer
{
public:
	//-----------------------------------------------------------------------------
	// Purpose: Sprite orientations
	// WARNING!  Change these in common/MaterialSystem/Sprite.cpp if you change them here!
	//-----------------------------------------------------------------------------
	typedef enum
	{
		SPR_VP_PARALLEL_UPRIGHT		= 0,
		SPR_FACING_UPRIGHT			= 1,
		SPR_VP_PARALLEL				= 2,
		SPR_ORIENTED				= 3,
		SPR_VP_PARALLEL_ORIENTED	= 4
	} SPRITETYPE;
	
	// Determine sprite orientation
	static void							GetSpriteAxes( SPRITETYPE type, 
										const Vector& origin,
										const QAngle& angles,
										Vector& forward, 
										Vector& right, 
										Vector& up );

	// Sprites can alter blending amount
	virtual float					GlowBlend( CEngineSprite *psprite, const Vector& entorigin, int rendermode, int renderfx, int alpha, float *scale );

	// Draws tempent as a sprite
	int								DrawSprite( 
										IClientEntity *entity,
										const model_t *model, 
										const Vector& origin, 
										const QAngle& angles,
										float frame,
										IClientEntity *attachedto,
										int attachmentindex,
										int rendermode,
										int renderfx,
										int alpha,
										int r, 
										int g, 
										int b,
										float scale,
										float flHDRColorScale = 1.0f
										);

protected:
	pixelvis_handle_t	m_queryHandle;
	float				m_flGlowProxySize;
	float				m_flHDRColorScale;
};

#endif

class CSprite : public CBaseEntity
#if defined( CLIENT_DLL )
	, public C_SpriteRenderer
#endif
{
	DECLARE_CLASS( CSprite, CBaseEntity );
public:
	DECLARE_PREDICTABLE();
	DECLARE_NETWORKCLASS();

	CSprite();
	virtual void SetModel( const char *szModelName );

	void Spawn( void );
	void Precache( void );
	virtual void ComputeWorldSpaceSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs );

	void SetGlowProxySize( float flSize ) { m_flGlowProxySize = flSize; }

#if !defined( CLIENT_DLL )

	virtual int ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual int UpdateTransmitState( void );
	
	void SetAsTemporary( void ) { AddSpawnFlags( SF_SPRITE_TEMPORARY ); }
	bool IsTemporary( void ) { return ( HasSpawnFlags( SF_SPRITE_TEMPORARY ) ); }
	
	int	ObjectCaps( void )
	{ 
		int flags = 0;
		
		if ( IsTemporary() )
		{
			flags = FCAP_DONT_SAVE;
		}
		
		return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | flags; 
	}

	void OnRestore();
#endif

	void AnimateThink( void );
	void ExpandThink( void );
	void Animate( float frames );
	void Expand( float scaleSpeed, float fadeSpeed );
	void SpriteInit( const char *pSpriteName, const Vector &origin );

#if !defined( CLIENT_DLL )
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	// Input handlers
	void InputHideSprite( inputdata_t &inputdata );
	void InputShowSprite( inputdata_t &inputdata );
	void InputToggleSprite( inputdata_t &inputdata );
	void InputColorRedValue( inputdata_t &inputdata );
	void InputColorBlueValue( inputdata_t &inputdata );
	void InputColorGreenValue( inputdata_t &inputdata );
#endif

	inline void SetAttachment( CBaseEntity *pEntity, int attachment )
	{
		if ( pEntity )
		{
			m_hAttachedToEntity = pEntity;
			m_nAttachment = attachment;
			FollowEntity( pEntity );
		}
	}

	void TurnOff( void );
	void TurnOn( void );
	bool IsOn() { return !IsEffectActive( EF_NODRAW ); }

	inline float Frames( void ) { return m_flMaxFrame; }
	inline void SetTransparency( int rendermode, int r, int g, int b, int a, int fx )
	{
		SetRenderMode( (RenderMode_t)rendermode );
		SetColor( r, g, b );
		SetBrightness( a );
		m_nRenderFX = fx;
	}
	inline void SetTexture( int spriteIndex ) { SetModelIndex( spriteIndex ); }
	inline void SetColor( int r, int g, int b ) { SetRenderColor( r, g, b, GetRenderColor().a ); }
	
	void SetBrightness( int brightness, float duration = 0.0f );
	void SetScale( float scale, float duration = 0.0f );
	void SetSpriteScale( float scale );
	void EnableWorldSpaceScale( bool bEnable );

	float GetScale( void ) { return m_flSpriteScale; }
	int	GetBrightness( void ) { return m_nBrightness; }
	float GetHDRColorScale( void ) { return m_flHDRColorScale; }

	inline void FadeAndDie( float duration ) 
	{ 
		SetBrightness( 0, duration );
		SetThink(&CSprite::AnimateUntilDead); 
		m_flDieTime = gpGlobals->curtime + duration; 
		SetNextThink( gpGlobals->curtime );  
	}

	inline void AnimateAndDie( float framerate ) 
	{ 
		SetThink(&CSprite::AnimateUntilDead); 
		m_flSpriteFramerate = framerate;
		m_flDieTime = gpGlobals->curtime + (m_flMaxFrame / m_flSpriteFramerate); 
		SetNextThink( gpGlobals->curtime ); 
	}

	inline void AnimateForTime( float framerate, float time ) 
	{ 
		SetThink(&CSprite::AnimateUntilDead); 
		m_flSpriteFramerate = framerate;
		m_flDieTime = gpGlobals->curtime + time;
		SetNextThink( gpGlobals->curtime ); 
	}

	// FIXME: This completely blows.
	// Surely there's gotta be a better way.
	void FadeOutFromSpawn( )
	{
		SetThink(&CSprite::BeginFadeOutThink); 
		SetNextThink( gpGlobals->curtime + 0.01f ); 
	}

	void BeginFadeOutThink( )
	{
		FadeAndDie( 0.25f ); 
	}

	void AnimateUntilDead( void );
#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();

	static CSprite *SpriteCreate( const char *pSpriteName, const Vector &origin, bool animate );
#endif
	static CSprite *SpriteCreatePredictable( const char *module, int line, const char *pSpriteName, const Vector &origin, bool animate );

#if defined( CLIENT_DLL )
	virtual float	GetRenderScale( void );
	virtual int		GetRenderBrightness( void );

	virtual int		DrawModel( int flags );
	virtual const	Vector& GetRenderOrigin();
	virtual void	GetRenderBounds( Vector &vecMins, Vector &vecMaxs );
	virtual float	GlowBlend( CEngineSprite *psprite, const Vector& entorigin, int rendermode, int renderfx, int alpha, float *scale );
	virtual void	GetToolRecordingState( KeyValues *msg );

// Only supported in TF2 right now
#if defined( INVASION_CLIENT_DLL )
	virtual bool	ShouldPredict( void )
	{
		return true;
	}
#endif

	virtual void	ClientThink( void );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

#endif
public:
	CNetworkHandle( CBaseEntity, m_hAttachedToEntity );
	CNetworkVar( int, m_nAttachment );
	CNetworkVar( float, m_flSpriteFramerate );
	CNetworkVar( float, m_flFrame );
#ifdef PORTAL
	CNetworkVar( bool, m_bDrawInMainRender );
	CNetworkVar( bool, m_bDrawInPortalRender );
#endif

	float		m_flDieTime;

private:

	CNetworkVar( int, m_nBrightness );
	CNetworkVar( float, m_flBrightnessTime );
	
	CNetworkVar( float, m_flSpriteScale );
	CNetworkVar( float, m_flScaleTime );
	CNetworkVar( bool, m_bWorldSpaceScale );
	CNetworkVar( float, m_flGlowProxySize );
	CNetworkVar( float, m_flHDRColorScale );

	float		m_flLastTime;
	float		m_flMaxFrame;

	float		m_flStartScale;
	float		m_flDestScale;		//Destination scale
	float		m_flScaleTimeStart;	//Real time for start of scale
	int			m_nStartBrightness;
	int			m_nDestBrightness;		//Destination brightness
	float		m_flBrightnessTimeStart;//Real time for brightness
};


class CSpriteOriented : public CSprite
{
public:
	DECLARE_CLASS( CSpriteOriented, CSprite );
#if !defined( CLIENT_DLL )
	DECLARE_SERVERCLASS();
	void Spawn( void );
#else
	DECLARE_CLIENTCLASS();
	virtual bool IsTransparent( void );
#endif
};



// Macro to wrap creation
#define SPRITE_CREATE_PREDICTABLE( name, origin, animate ) \
	CSprite::SpriteCreatePredictable( __FILE__, __LINE__, name, origin, animate )

#endif // SPRITE_H
