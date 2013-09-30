#ifndef PARTICLE_OPERATORS_LENSFLARE_H
#define PARTICLE_OPERATORS_LENSFLARE_H

#include "cbase.h"
#include "Gstring/vgui/vParticleOperators.h"

class CGlowOverlay;

class vParticleOperator_Lensflare_Base : public vParticleOperatorBase
{
public:
	vParticleOperator_Lensflare_Base( CGlowOverlay *src );

protected:
	CGlowOverlay *GetSourceEntity();

private:
	CGlowOverlay *pSrc;
};

class vParticleOperator_Lensflare_Size : public vParticleOperator_Lensflare_Base
{
public:
	vParticleOperator_Lensflare_Size( CGlowOverlay *src,
		float scale_min, float scale_max,
		bool scale_by_dot, float dot_min, float dot_max, float dot_bias );
	virtual void Simulate( vParticle *parent );

private:
	float flScaleMin, flScaleMax;
	float flDotMin, flDotMax, flDotBias;
	bool bUseDot;
};

class vParticleOperator_Lensflare_Alpha : public vParticleOperator_Lensflare_Base
{
public:
	vParticleOperator_Lensflare_Alpha( CGlowOverlay *src,
		float alpha_min, float alpha_max,
		bool alpha_by_dot, float dot_min, float dot_max, float dot_bias );
	virtual void Simulate( vParticle *parent );

private:
	float flAlphaMin, flAlphaMax;
	float flDotMin, flDotMax, flDotBias;
	bool bUseDot;
};

class vParticleOperator_Lensflare_Transformation : public vParticleOperator_Lensflare_Base
{
public:
	vParticleOperator_Lensflare_Transformation( CGlowOverlay *src,
		float distance, float angular_offset,
		bool auto_orient, float rot_offset, float rot_mult );
	virtual void Simulate( vParticle *parent );

private:
	float flDist, flAngularOffset;

	bool bDoOrientation;
	float flRotationOffset, flRotationMultiplier;
};

#endif