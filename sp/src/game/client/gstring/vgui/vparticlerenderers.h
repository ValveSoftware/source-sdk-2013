#ifndef PARTICLE_RENDERERS_H
#define PARTICLE_RENDERERS_H

#include "cbase.h"
#include "Gstring/gstring_cvars.h"

struct vParticle;

class vParticleRendererBase
{
public:
	virtual void Render( vParticle *p );
};


class vParticleRenderer_vMaterialBase : public vParticleRendererBase
{
public:
	vParticleRenderer_vMaterialBase( const char *szMat );

	inline int GetTextureID();

private:
	int iTextureID;
	static int m_iTextureIDWireframe;
};


int vParticleRenderer_vMaterialBase::GetTextureID()
{
	if ( cvar_gstring_debug_vguiparticles.GetInt() )
		return m_iTextureIDWireframe;

	return iTextureID;
}


class vParticleRenderer_Rect : public vParticleRenderer_vMaterialBase
{
public:
	vParticleRenderer_Rect( const char *szMat, float scale_x = 1.0f, float scale_y = 1.0f );

	virtual void Render( vParticle *p );
private:
	float m_flScale_x, m_flScale_y;
};


class vParticleRenderer_Quad : public vParticleRenderer_Rect
{
public:
	vParticleRenderer_Quad( const char *szMat ) : vParticleRenderer_Rect( szMat ) {};
};


class vParticleRenderer_Trail : public vParticleRenderer_vMaterialBase
{
public:
	vParticleRenderer_Trail( const char *szMat, int maxSteps = 5, float stepDelay = 0.1f );

	virtual void Render( vParticle *p );
private:

	struct vTrailStep_t
	{
		Vector2D a, b, center;
	};

	void UpdateSteps( vParticle *p );
	void AddStep( vParticle *p );
	void SetStepData( vParticle *p, vTrailStep_t &step );

	void RenderInternal( vParticle *p );
	void RenderTrail( vParticle *p );

	CUtlVector< vTrailStep_t > hSteps;

	bool bClean;
	float flNextAddTime;
	Vector2D vecLastPos;

	int iMaxSteps;
	float flStepDelay;
};

#endif