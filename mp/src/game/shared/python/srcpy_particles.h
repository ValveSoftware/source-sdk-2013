//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_PARTICLES_H
#define SRCPY_PARTICLES_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "particles_new.h"
#include "dlight.h"
#include "iefx.h"

#if 0
void ParticleEffectInvalid_Error();

#define PARTICLE_VALID()						\
	if( m_sInternal.GetObject() == NULL ) {		\
		ParticleEffectInvalid_Error();			\
		return;									\
	}

#define PARTICLE_VALID_RV(value)				\
	if( m_sInternal.GetObject() == NULL ) {		\
		ParticleEffectInvalid_Error();			\
		return value;							\
	}

class CNewParticleEffectHandle
{
public:
	CNewParticleEffectHandle(CNewParticleEffect *pEffect) : m_sInternal(pEffect) {}
	CNewParticleEffectHandle( CBaseEntity *pOwner, const char *pParticleSystemName,
		const char *pDebugName = NULL );

#if 0
public:
	// Methods from CNewParticleEffect
	// Call this before adding a bunch of particles to give it a rough estimate of where
	// your particles are for sorting amongst other translucent entities.
	void SetSortOrigin( const Vector &vSortOrigin );
	bool ShouldDraw( void );
	bool IsTransparent( void );
	bool IsTwoPass( void );
	bool UsesPowerOfTwoFrameBufferTexture( void );
	bool UsesFullFrameBufferTexture( void );
	const QAngle& GetRenderAngles( void );
	const matrix3x4_t& RenderableToWorldTransform();
	void GetRenderBounds( Vector& mins, Vector& maxs );

	// check if the new bounds of the particle system needs its client-leaf info needs to be updated
	void DetectChanges( void );
	const Vector &GetRenderOrigin( void );
	PMaterialHandle GetPMaterial( const char *name );
	bool RecalculateBoundingBox();
	Particle* AddParticle( unsigned int particleSize, PMaterialHandle material, const Vector &origin );
	const char *GetEffectName();
	void SetDontRemove( bool bSet );
	void SetDrawn( bool bDrawn );
	void SetFirstFrameFlag( bool bFirst );
	void SetNeedsBBoxUpdate( bool bNeedsUpdate );
	void SetAutoUpdateBBox( bool bNeedsUpdate );
	void SetRemoveFlag( void );
	bool GetRemoveFlag( void );
	bool GetFirstFrameFlag( void );
	bool GetNeedsBBoxUpdate( void );
	bool GetAutoUpdateBBox( void );
	bool ShouldPerformCullCheck() const;
	void MarkShouldPerformCullCheck( bool bEnable );
	CBaseEntity *GetOwner( void ) { return m_hOwner; }
	void SetOwner( CBaseEntity *pOwner ) { m_hOwner = pOwner; }
	CNewParticleEffect* ReplaceWith( const char *pParticleSystemName );

#endif // 0

	// CParticleCollection overrides
public:
	void StopEmission( bool bInfiniteOnly = false, bool bRemoveAllParticles = false, bool bWakeOnStop = false ) 
	{ 
		PARTICLE_VALID( ); 
		m_sInternal->StopEmission(false, bRemoveAllParticles, bWakeOnStop); 
	}

	void SetDormant( bool bDormant )
	{ 
		PARTICLE_VALID( ); 
		m_sInternal->SetDormant(bDormant); 
	}

	void SetControlPoint( int nWhichPoint, const Vector &v )
	{ 
		PARTICLE_VALID( ); 
		m_sInternal->SetControlPoint(nWhichPoint, v); 
	}

	void SetControlPointEntity( int nWhichPoint, CBaseEntity *pEntity );

	void SetControlPointOrientation( int nWhichPoint, const Quaternion &q )
	{ 
		PARTICLE_VALID( ); 
		m_sInternal->SetControlPointOrientation(nWhichPoint, q); 
	}

	void SetControlPointOrientation( int nWhichPoint, const Vector &forward, const Vector &right, const Vector &up )
	{ 
		PARTICLE_VALID( ); 
		m_sInternal->SetControlPointOrientation(nWhichPoint, forward, right, up); 
	}

	void SetControlPointForwardVector( int nWhichPoint, const Vector &v )
	{ 
		PARTICLE_VALID( ); 
		m_sInternal->SetControlPointForwardVector(nWhichPoint, v); 
	}

	void SetControlPointUpVector( int nWhichPoint, const Vector &v )
	{ 
		PARTICLE_VALID( ); 
		m_sInternal->SetControlPointUpVector(nWhichPoint, v); 
	}

	void SetControlPointRightVector( int nWhichPoint, const Vector &v )
	{ 
		PARTICLE_VALID( ); 
		m_sInternal->SetControlPointRightVector(nWhichPoint, v); 
	}

	EHANDLE GetControlPointEntity( int nWhichPoint );

public:
	inline CNewParticleEffect *GetParticleEffect() { return m_sInternal.GetObject(); }

private:
	CSmartPtr<CNewParticleEffect> m_sInternal;
};

#endif // 0

class CNewParticleEffectHandle : CSmartPtr<CNewParticleEffect>
{
};

// Wraps a dlight
class PyDBaseLight
{
public:
	PyDBaseLight( C_BaseEntity *owner ) : m_hOwner(owner), m_pDLight(NULL), m_fDie(0) {}

	inline void SetOrigin(Vector &origin) { CheckValid(); m_pDLight->origin = origin; }
	inline Vector &GetOrigin() { CheckValid(); return m_pDLight->origin; } 

	inline void SetColor(ColorRGBExp32 &color) { CheckValid(); m_pDLight->color = color; }
	inline ColorRGBExp32 &GetColor() { CheckValid(); return m_pDLight->color; } 

	inline void SetDie(float die) { CheckValid(); m_fDie = die; m_pDLight->die = die; }
	inline float GetDie() { CheckValid(); return m_pDLight->die; } 

	inline void SetRadius(float radius) { CheckValid(); m_pDLight->radius = radius; }
	inline float GetRadius() { CheckValid(); return m_pDLight->radius; } 

	inline void SetFlags(int flags) { CheckValid(); m_pDLight->flags = flags; }
	inline int GetFlags() { CheckValid(); return m_pDLight->flags; } 

	inline void SetMinlight(int minlight) { CheckValid(); m_pDLight->minlight = minlight; }
	inline int GetMinlight() { CheckValid(); return m_pDLight->minlight; } 

	inline void SetDecay(float decay) { CheckValid(); m_pDLight->decay = decay; }
	inline float GetDecay() { CheckValid(); return m_pDLight->decay; } 

	inline void SetStyle(int style) { CheckValid(); m_pDLight->style = style; }
	inline int GetStyle() { CheckValid(); return m_pDLight->style; } 

	inline void SetDirection(Vector &direction) { CheckValid(); m_pDLight->m_Direction = direction; }
	inline Vector &GetDirection() { CheckValid(); return m_pDLight->m_Direction; } 

	inline void SetInnerAngle(float innerangle) { CheckValid(); m_pDLight->m_InnerAngle = innerangle; }
	inline float GetInnerAngle() { CheckValid(); return m_pDLight->m_InnerAngle; } 

	inline void SetOuterAngle(float outerangle) { CheckValid(); m_pDLight->m_OuterAngle = outerangle; }
	inline float GetOuterAngle() { CheckValid(); return m_pDLight->m_OuterAngle; } 

protected:
	void CheckValid();

protected:
	dlight_t *m_pDLight;
	EHANDLE m_hOwner;
	float m_fDie;
};

class PyDLight : public PyDBaseLight
{
public:
	PyDLight( C_BaseEntity *owner );
};

class PyELight : public PyDBaseLight
{
public:
	PyELight( C_BaseEntity *owner );
};

#endif // CLIENT_DLL

void PyShouldLoadSheets( bool bLoadSheets );
bool PyReadParticleConfigFile( const char *filename, bool precache = false, bool decommittempmemory = false );
void PyDecommitTempMemory();

#endif // SRCPY_PARTICLES_H