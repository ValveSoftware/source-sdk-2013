//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#include "particles_simple.h"
#include "particlemgr.h"
#include "c_pixel_visibility.h"
#include "fx_fleck.h"

#include "tier0/memdbgon.h"

#define	bitsPARTICLE_TRAIL_VELOCITY_DAMPEN	0x00000001	//Dampen the velocity as the particles move
#define	bitsPARTICLE_TRAIL_COLLIDE			0x00000002	//Do collision with simulation
#define	bitsPARTICLE_TRAIL_FADE				0x00000004	//Fade away
#define	bitsPARTICLE_TRAIL_FADE_IN			0x00000008	//Fade in

class TrailParticle : public Particle
{
public:

	Vector		m_vecVelocity;
	color32		m_color;				// Particle color
	float		m_flDieTime;			// How long it lives for.
	float		m_flLifetime;			// How long it has been alive for so far.
	float		m_flLength;				// Length of the tail (in seconds!)
	float		m_flWidth;				// Width of the spark
};

inline void Color32ToFloat4( float colorOut[4], const color32 & colorIn )
{
	colorOut[0] = colorIn.r * (1.0f/255.0f);
	colorOut[1] = colorIn.g * (1.0f/255.0f);
	colorOut[2] = colorIn.b * (1.0f/255.0f);
	colorOut[3] = colorIn.a * (1.0f/255.0f);
}

inline void FloatToColor32( color32 &out, float r, float g, float b, float a )
{
	out.r = r * 255;
	out.g = g * 255;
	out.b = b * 255;
	out.a = a * 255;
}

inline void Float4ToColor32( color32 &out, float colorIn[4] )
{
	out.r = colorIn[0] * 255;
	out.g = colorIn[1] * 255;
	out.b = colorIn[2] * 255;
	out.a = colorIn[3] * 255;
}

inline void Color32Init( color32 &out, int r, int g, int b, int a )
{
	out.r = r;
	out.g = g;
	out.b = b;
	out.a = a;
}
//
// CTrailParticles
//

class CTrailParticles : public CSimpleEmitter
{
	DECLARE_CLASS( CTrailParticles, CSimpleEmitter );
public:
	CTrailParticles( const char *pDebugName );
	
	static CTrailParticles	*Create( const char *pDebugName )	{	return new CTrailParticles( pDebugName );	}

	virtual void RenderParticles( CParticleRenderIterator *pIterator );
	virtual void SimulateParticles( CParticleSimulateIterator *pIterator );

	//Setup for point emission
	virtual void	Setup( const Vector &origin, const Vector *direction, float angularSpread, float minSpeed, float maxSpeed, float gravity, float dampen, int flags, bool bNotCollideable = false );
	
	void SetFlag( int flags )				{	m_fFlags |= flags;	}
	void SetVelocityDampen( float dampen )	{	m_flVelocityDampen = dampen;	}
	void SetGravity( float gravity )		{	m_ParticleCollision.SetGravity( gravity );	}
	void SetCollisionDamped( float dampen )	{	m_ParticleCollision.SetCollisionDampen( dampen );	}
	void SetAngularCollisionDampen( float dampen )	{ m_ParticleCollision.SetAngularCollisionDampen( dampen );	}

	CParticleCollision	m_ParticleCollision;

protected:

	int					m_fFlags;
	float				m_flVelocityDampen;

private:
	CTrailParticles( const CTrailParticles & ); // not defined, not accessible
};

//
// Sphere trails
//

class CSphereTrails : public CSimpleEmitter
{
	DECLARE_CLASS( CSphereTrails, CSimpleEmitter );
public:
	CSphereTrails( const char *pDebugName, const Vector &origin, float innerRadius, float outerRadius, float speed, int entityIndex, int attachment );
	
	virtual void SimulateParticles( CParticleSimulateIterator *pIterator );
	virtual void RenderParticles( CParticleRenderIterator *pIterator );
	virtual void Update( float flTimeDelta );
	virtual void StartRender();

	void AddStreaks( float count );
	Vector	m_particleOrigin;
	float	m_life;
	float	m_outerRadius;
	float	m_innerRadius;
	float	m_effectSpeed;
	float	m_streakSpeed;
	float	m_count;
	float	m_growth;
	int		m_entityIndex;
	int		m_attachment;
	PMaterialHandle m_hMaterial;
	Vector	m_boneOrigin;
	float	m_dieTime;

private:
	CSphereTrails( const CSphereTrails & ); // not defined, not accessible
};


// Simple glow emitter won't draw any of it's particles until/unless the pixel visibility test succeeds
class CSimpleGlowEmitter : public CSimpleEmitter
{
	DECLARE_CLASS( CSimpleGlowEmitter, CSimpleEmitter );
public:
	CSimpleGlowEmitter( const char *pDebugName, const Vector &sortOrigin, float flDeathTime );
	static CSimpleGlowEmitter	*Create( const char *pDebugName, const Vector &sortOrigin, float flDeathTime );

	virtual void SimulateParticles( CParticleSimulateIterator *pIterator );
	virtual void RenderParticles( CParticleRenderIterator *pIterator );
protected:

	bool WasTestedInView( unsigned char viewMask );
	bool IsVisibleInView( unsigned char viewMask );
	void SetTestedInView( unsigned char viewMask, bool bTested );
	void SetVisibleInView( unsigned char viewMask, bool bVisible );
	unsigned char CurrentViewMask() const;

	float				m_flDeathTime;			// How long it has been alive for so far.
	float				m_startTime;
	pixelvis_handle_t	m_queryHandle;
private:
	unsigned char		m_wasTested;
	unsigned char		m_isVisible;

private:
	CSimpleGlowEmitter( const CSimpleGlowEmitter & ); // not defined, not accessible
};

#include "tier0/memdbgoff.h"
