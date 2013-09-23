#ifndef PARTICLE_OPERATORS_H
#define PARTICLE_OPERATORS_H

#include "cbase.h"

struct vParticle;

struct vParticleImpulseGen
{
public:

	friend class vParticleOperatorBase;

	enum PARTICLEIMPULSEGENERATOR_MODE
	{
		PARTICLEIMPULSEGENERATOR_FRAMETIME = 0,
		PARTICLEIMPULSEGENERATOR_LIFETIME_LINEAR,
		PARTICLEIMPULSEGENERATOR_LIFETIME_SINE,
		PARTICLEIMPULSEGENERATOR_VELOCITY_LINEAR,
		PARTICLEIMPULSEGENERATOR_ANGULAR_VELOCITY_LINEAR,
		PARTICLEIMPULSEGENERATOR_CURTIME_SINE,
		PARTICLEIMPULSEGENERATOR_CURTIME_SINE_SIGNED,
	};

	vParticleImpulseGen();
	vParticleImpulseGen( PARTICLEIMPULSEGENERATOR_MODE m );
	~vParticleImpulseGen();

	PARTICLEIMPULSEGENERATOR_MODE mode;

	float impulse_bias;
	float reference_min, reference_max;
	float impulse_multiplier;

	float GetImpulse( vParticle *parent );

private:
	vParticleImpulseGen( const vParticleImpulseGen &o );

	void Init();
};


class vParticleOperatorBase
{
public:

	vParticleOperatorBase();
	vParticleOperatorBase( vParticleImpulseGen::PARTICLEIMPULSEGENERATOR_MODE defaultGenMode );
	virtual ~vParticleOperatorBase();

	vParticleImpulseGen *GetImpulseGenerator();
	void SetImpulseGenerator( vParticleImpulseGen *gen ); // assumes ownership!

	virtual void Simulate( vParticle *parent );
	float GetImpulse( vParticle *parent );

private:
	vParticleOperatorBase( const vParticleOperatorBase &o );

	vParticleImpulseGen *impulse;

	void Init( vParticleImpulseGen::PARTICLEIMPULSEGENERATOR_MODE mode );
};



class vParticleOperator_Movement : public vParticleOperatorBase
{
public:
	vParticleOperator_Movement();
	virtual void Simulate( vParticle *parent );
};
class vParticleOperator_Movement_SpeedClamp : public vParticleOperator_Movement
{
public:
	vParticleOperator_Movement_SpeedClamp( float maxVelocity );
	virtual void Simulate( vParticle *parent );
private:
	float maxspeed;
};
class vParticleOperator_Gravity : public vParticleOperator_Movement
{
public:
	vParticleOperator_Gravity( Vector2D gravityAmount );
	virtual void Simulate( vParticle *parent );
protected:
	Vector2D gravity;
};
class vParticleOperator_GravityWorld : public vParticleOperator_Movement
{
public:
	vParticleOperator_GravityWorld( float gravityAmount );
	virtual void Simulate( vParticle *parent );
private:
	float amt;
};
class vParticleOperator_GravityRotational : public vParticleOperator_Movement
{
public:
	vParticleOperator_GravityRotational( float gravityAmount, Vector2D gravCenterNormalized );
	virtual void Simulate( vParticle *parent );
private:
	float amt;
	Vector2D centerN;
};
class vParticleOperator_RainSimulation : public vParticleOperator_Movement
{
public:
	vParticleOperator_RainSimulation();
	virtual void Simulate( vParticle *parent );
private:
	Vector2D veloSaved;
	float flIdleTime;
	float flMoveTime;
};
class vParticleOperator_WorldRotationForce : public vParticleOperator_Movement
{
public:
	vParticleOperator_WorldRotationForce( float strength );
	virtual void Simulate( vParticle *parent );
private:
	float str;
	QAngle last;
};
class vParticleOperator_Drag : public vParticleOperator_Movement
{
public:
	vParticleOperator_Drag( float decayMultiplier );
	virtual void Simulate( vParticle *parent );
protected:
	float decay;
};
class vParticleOperator_Drag_Angle : public vParticleOperator_Drag
{
public:
	vParticleOperator_Drag_Angle( float decayMultiplier ) : vParticleOperator_Drag( decayMultiplier ) {};
	virtual void Simulate( vParticle *parent );
private:
};

class vParticleOperator_AlphaFade : public vParticleOperatorBase
{
public:
	vParticleOperator_AlphaFade( float alphaTarget );
	virtual void Simulate( vParticle *parent );
private:
	float alphat;
};

class vParticleOperator_RotationFade : public vParticleOperatorBase
{
public:
	vParticleOperator_RotationFade( float rotation_min, float rotation_max );
	virtual void Simulate( vParticle *parent );
private:
	float rotmin;
	float rotmax;
};

class vParticleOperator_RotationAnimate : public vParticleOperatorBase
{
public:
	vParticleOperator_RotationAnimate( float multiplier );
	virtual void Simulate( vParticle *parent );
private:
	float mult;
};

class vParticleOperator_SizeMakeRelative : public vParticleOperatorBase
{
public:
	virtual void Simulate( vParticle *parent );
};

class vParticleOperator_SizeFade : public vParticleOperatorBase
{
public:
	vParticleOperator_SizeFade( float sizeTarget, bool relative = true );
	virtual void Simulate( vParticle *parent );
private:
	float sizet;
};

class vParticleOperator_SizeMultiply : public vParticleOperatorBase
{
public:
	vParticleOperator_SizeMultiply( float sizeTarget );
	virtual void Simulate( vParticle *parent );
private:
	float sizet;
};

class vParticleOperator_SizeMultiplyXY : public vParticleOperatorBase
{
public:
	vParticleOperator_SizeMultiplyXY( float sizeTarget_x, float sizeTarget_y );
	virtual void Simulate( vParticle *parent );
private:
	float sizet_x, sizet_y;
};

class vParticleOperator_ColorFade : public vParticleOperatorBase
{
public:
	vParticleOperator_ColorFade( Vector colorTarget );
	virtual void Simulate( vParticle *parent );
private:
	Vector colort;
};

#endif