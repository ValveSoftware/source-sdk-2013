
#include "cbase.h"
#include "Gstring/vgui/vUtil.h"
#include "Gstring/vgui/vParticle.h"
#include "Gstring/cFrametimeHelper.h"

#include "view.h"
#include "view_scene.h"

#define NORMALIZE( x ) ( clamp( x, 0, 1 ) )

vParticleImpulseGen::vParticleImpulseGen()
{
	Init();
}

vParticleImpulseGen::vParticleImpulseGen( PARTICLEIMPULSEGENERATOR_MODE m )
{
	Init();
	mode = m;
}

vParticleImpulseGen::~vParticleImpulseGen()
{
}

void vParticleImpulseGen::Init()
{
	mode = PARTICLEIMPULSEGENERATOR_LIFETIME_LINEAR;

	reference_min = 0.0f;
	reference_max = 1.0f;

	impulse_bias = 0.5f;

	impulse_multiplier = 1.0f;
}

float vParticleImpulseGen::GetImpulse( vParticle *parent )
{
	Assert( parent != NULL );

	float val = 0;

	switch ( mode )
	{
	case PARTICLEIMPULSEGENERATOR_FRAMETIME:
		val = CFrameTimeHelper::GetFrameTime() * impulse_multiplier;
		break;
	case PARTICLEIMPULSEGENERATOR_LIFETIME_LINEAR:
		val = NORMALIZE( ( CFrameTimeHelper::GetCurrentTime() - parent->GetCreationTime() ) / parent->GetLifeDuration() ) * impulse_multiplier;
		break;
	case PARTICLEIMPULSEGENERATOR_LIFETIME_SINE:
		val = NORMALIZE( ( CFrameTimeHelper::GetCurrentTime() - parent->GetCreationTime() ) / parent->GetLifeDuration() );
		val = NORMALIZE( abs( sin( M_PI_F * val * impulse_multiplier ) ) );
		break;
	case PARTICLEIMPULSEGENERATOR_VELOCITY_LINEAR:
		val = parent->vecVelocity.Length() * impulse_multiplier;
		break;
	case PARTICLEIMPULSEGENERATOR_ANGULAR_VELOCITY_LINEAR:
		val = abs( parent->flAngularVelocity ) * impulse_multiplier;
		break;
	case PARTICLEIMPULSEGENERATOR_CURTIME_SINE:
		val = NORMALIZE( abs( sin( M_PI_F * CFrameTimeHelper::GetCurrentTime() * impulse_multiplier ) ) );
		break;
	case PARTICLEIMPULSEGENERATOR_CURTIME_SINE_SIGNED:
		val = ( ( sin( M_PI_F * CFrameTimeHelper::GetCurrentTime() * impulse_multiplier ) ) );
		break;
	default:
		Assert(0);
		break;
	}

	Assert( IsFinite(val) );

	float sign = Sign( val );
	val = sign * Bias( abs(val), impulse_bias );

	val = RemapVal( val, 0.0f, 1.0f, reference_min, reference_max );

	return val;
}


vParticleOperatorBase::vParticleOperatorBase()
{
	Init( vParticleImpulseGen::PARTICLEIMPULSEGENERATOR_LIFETIME_LINEAR );
	Assert( impulse != NULL );
}

vParticleOperatorBase::vParticleOperatorBase( vParticleImpulseGen::PARTICLEIMPULSEGENERATOR_MODE defaultGenMode )
{
	Init( defaultGenMode );
	Assert( impulse != NULL );
}

vParticleOperatorBase::~vParticleOperatorBase()
{
	delete impulse;
}

void vParticleOperatorBase::Init( vParticleImpulseGen::PARTICLEIMPULSEGENERATOR_MODE mode )
{
	impulse = NULL;

	SetImpulseGenerator( new vParticleImpulseGen( mode ) );
}

vParticleImpulseGen *vParticleOperatorBase::GetImpulseGenerator()
{
	Assert( impulse != NULL );

	return impulse;
}

void vParticleOperatorBase::SetImpulseGenerator( vParticleImpulseGen *gen )
{
	delete impulse;
	impulse = gen;
}

void vParticleOperatorBase::Simulate( vParticle *parent )
{
	Assert( 0 );
}

float vParticleOperatorBase::GetImpulse( vParticle *parent )
{
	Assert( impulse != NULL );

	return impulse->GetImpulse( parent );
}


vParticleOperator_Movement::vParticleOperator_Movement() : vParticleOperatorBase( vParticleImpulseGen::PARTICLEIMPULSEGENERATOR_FRAMETIME )
{
}
void vParticleOperator_Movement::Simulate( vParticle *parent )
{
	if ( parent->vecVelocity.LengthSqr() > 0.001f )
		parent->vecPos += parent->vecVelocity * CFrameTimeHelper::GetFrameTime(); //GetImpulse( parent );
	if ( parent->flAngularVelocity > 0.001f )
		parent->flAngle += parent->flAngularVelocity;
}
vParticleOperator_Movement_SpeedClamp::vParticleOperator_Movement_SpeedClamp( float maxVelocity )
{
	maxspeed = maxVelocity;
}
void vParticleOperator_Movement_SpeedClamp::Simulate( vParticle *parent )
{
	float speed = parent->vecVelocity.NormalizeInPlace();
	float curMaxSpeed = maxspeed * vParticle::GetRelativeScale();
	if ( speed > curMaxSpeed )
		speed = curMaxSpeed;
	parent->vecVelocity *= speed;
}
vParticleOperator_Gravity::vParticleOperator_Gravity( Vector2D gravityAmount )
{
	gravity = gravityAmount;
}
void vParticleOperator_Gravity::Simulate( vParticle *parent )
{
	float multiplier = vParticle::GetRelativeScale();
	parent->vecVelocity += GetImpulse( parent ) * gravity * multiplier;
}
vParticleOperator_GravityWorld::vParticleOperator_GravityWorld( float gravityAmount )
{
	amt = gravityAmount;
}
void vParticleOperator_GravityWorld::Simulate( vParticle *parent )
{
	float dot_z = DotProduct( MainViewForward(), Vector( 0, 0, -1 ) );
	float rotation_dir = Sign( dot_z );

	Vector screen;
	if ( ScreenTransform( MainViewOrigin() + Vector( 0, 0, 100 ), screen ) )
		ScreenTransform( MainViewOrigin() - Vector( 0, 0, 100 ), screen );

	screen *= Vector( 0.5f, -0.5f, 0 );
	screen += Vector( 0.5f, 0.5f, 0 );

	int w, t;
	engine->GetScreenSize( w, t );

	Vector2D gravity_center( w * screen.x, t * screen.y );
	Vector2D delta = parent->vecPos - gravity_center;

	float screenLength = delta.NormalizeInPlace();
	delta *= rotation_dir * -1.0f;

	Vector2D accel = delta * vParticle::GetRelativeScale() * GetImpulse( parent ) * amt;

	float speedMult = 1.0f;

	if ( rotation_dir > 0 )
	{
		float drag = RemapValClamped( rotation_dir, 0.5f, 1, 1, 0.00001f );
		ScaleByFrametime( drag );

		speedMult = RemapValClamped( (screenLength/vParticle::GetRelativeScale()), 0, 1000, 0, 1 ) * (1.0f - drag) + drag;
	}

	parent->vecVelocity += accel;
	parent->vecVelocity *= speedMult;
}
vParticleOperator_GravityRotational::vParticleOperator_GravityRotational( float gravityAmount, Vector2D gravCenterNormalized )
{
	amt = gravityAmount;
	centerN = gravCenterNormalized;
}
void vParticleOperator_GravityRotational::Simulate( vParticle *parent )
{
	int w, t;
	engine->GetScreenSize( w, t );

	Vector2D gravity_center( w * centerN.x, t * centerN.y );
	Vector2D delta = parent->vecPos - gravity_center;

	Vector2D accel = delta * vParticle::GetRelativeScale() * GetImpulse( parent ) * amt;

	float speedMult = 1.0f;

	parent->vecVelocity += accel;
	parent->vecVelocity *= speedMult;
}
vParticleOperator_RainSimulation::vParticleOperator_RainSimulation()
{
	veloSaved.Init();
	flIdleTime = 0;
	flMoveTime = CFrameTimeHelper::GetCurrentTime() + RandomFloat( 0.2f, 0.4f );
}
void vParticleOperator_RainSimulation::Simulate( vParticle *parent )
{
	const bool bInMove = flMoveTime > CFrameTimeHelper::GetCurrentTime();
	const bool bInIdle = flIdleTime > CFrameTimeHelper::GetCurrentTime();

	if ( bInIdle )
	{
		parent->vecVelocity = veloSaved * 0.01f;
	}
	else if ( !bInIdle && !bInMove )
	{
		if ( flIdleTime != 0 )
		{
			flMoveTime = CFrameTimeHelper::GetCurrentTime() + RandomFloat( 0.3f, 0.4f );
			parent->vecVelocity = veloSaved +
				Vector2D( RandomFloat( -1, 1 ), RandomFloat( -1, 1 ) ) * vParticle::GetRelativeScale() * 50;
		}
		else
			flIdleTime = CFrameTimeHelper::GetCurrentTime() + RandomFloat( 0.2f, 0.4f );
	}
	else if ( bInMove )
		flIdleTime = 0;
	else
		veloSaved = parent->vecVelocity;
}
vParticleOperator_WorldRotationForce::vParticleOperator_WorldRotationForce( float strength )
{
	str = strength;
	last = MainViewAngles();
}
void vParticleOperator_WorldRotationForce::Simulate( vParticle *parent )
{
	QAngle cur = MainViewAngles();

	Vector2D force( AngleDiff( cur.y, last.y ), AngleDiff( last.x, cur.x ) );
	force *= GetImpulse( parent ) * vParticle::GetRelativeScale() * str;

	float rainHack = CFrameTimeHelper::GetFrameTime() - (1.0f/60.0f);
	force += force * rainHack * -20.0f;

	last = cur;

	parent->vecVelocity += force;
}
vParticleOperator_Drag::vParticleOperator_Drag( float decayMultiplier )
{
	decay = decayMultiplier;
}
void vParticleOperator_Drag::Simulate( vParticle *parent )
{
	if ( parent->vecVelocity.LengthSqr() <= 0.001f || decay == 0.0f )
	{
		parent->vecVelocity.Init( 0, 0 );
		return;
	}

	//Vector2D subtract = parent->vecVelocity * decay * GetImpulse( parent );

	//if ( subtract.LengthSqr() >= parent->vecVelocity.LengthSqr() )
	//	parent->vecVelocity.Init( 0, 0 );
	//else
	//	parent->vecVelocity -= subtract;

	ScaleByFrametime( decay );
	parent->vecVelocity *= decay;
}
void vParticleOperator_Drag_Angle::Simulate( vParticle *parent )
{
	if ( parent->flAngularVelocity <= 0.001f || decay == 0.0f )
	{
		parent->flAngularVelocity = 0;
		return;
	}

	ScaleByFrametime( decay );
	parent->flAngularVelocity *= decay;
}

vParticleOperator_AlphaFade::vParticleOperator_AlphaFade( float alphaTarget )
{
	alphat = alphaTarget;
}
void vParticleOperator_AlphaFade::Simulate( vParticle *parent )
{
	float alphaOld = parent->GetCurrentAlpha();
	float imp = GetImpulse( parent );
	float alphaNew = Lerp( imp, alphaOld, alphat );
	parent->SetCurrentAlpha( alphaNew );
}

vParticleOperator_RotationFade::vParticleOperator_RotationFade( float rotation_min, float rotation_max )
{
	rotmin = rotation_min;
	rotmax = rotation_max;
}
void vParticleOperator_RotationFade::Simulate( vParticle *parent )
{
	float imp = GetImpulse( parent );
	parent->flAngle = Lerp( imp, rotmin, rotmax );
}

vParticleOperator_RotationAnimate::vParticleOperator_RotationAnimate( float multiplier )
{
	mult = multiplier;
}
void vParticleOperator_RotationAnimate::Simulate( vParticle *parent )
{
	float rot = parent->flAngle;
	float imp = GetImpulse( parent );
	float rotnew = rot + imp * mult;
	parent->flAngle = rotnew;
}

void vParticleOperator_SizeMakeRelative::Simulate( vParticle *parent )
{
	parent->SetCurrentSize( parent->GetCurrentSize() * vParticle::GetRelativeScale() );
}

vParticleOperator_SizeFade::vParticleOperator_SizeFade( float sizeTarget, bool relative )
{
	sizet = sizeTarget;

	if ( relative )
		sizet *= vParticle::GetRelativeScale();
}
void vParticleOperator_SizeFade::Simulate( vParticle *parent )
{
	float sizeOld = parent->GetCurrentSize();
	float imp = GetImpulse( parent );
	float sizeNew = Lerp( imp, sizeOld, sizet );
	parent->SetCurrentSize( sizeNew );
}

vParticleOperator_SizeMultiply::vParticleOperator_SizeMultiply( float sizeTarget )
{
	sizet = sizeTarget;
}
void vParticleOperator_SizeMultiply::Simulate( vParticle *parent )
{
	float sizeOld = parent->GetCurrentSize();
	float imp = GetImpulse( parent );
	float sizeNew = sizeOld * Lerp( imp, 1.0f, sizet );
	parent->SetCurrentSize( sizeNew );
}

vParticleOperator_SizeMultiplyXY::vParticleOperator_SizeMultiplyXY( float sizeTarget_x, float sizeTarget_y )
{
	sizet_x = sizeTarget_x;
	sizet_y = sizeTarget_y;
}
void vParticleOperator_SizeMultiplyXY::Simulate( vParticle *parent )
{
	float sizeOldx = parent->GetCurrentSizeX();
	float sizeOldy = parent->GetCurrentSizeY();
	float imp = GetImpulse( parent );
	float sizeNewx = sizeOldx * Lerp( imp, 1.0f, sizet_x );
	float sizeNewy = sizeOldy * Lerp( imp, 1.0f, sizet_y );
	parent->SetCurrentSize( sizeNewx, sizeNewy );
}

vParticleOperator_ColorFade::vParticleOperator_ColorFade( Vector colorTarget )
{
	colort = colorTarget;
}
void vParticleOperator_ColorFade::Simulate( vParticle *parent )
{
	Vector colorOld = parent->GetCurrentColor();
	float imp = GetImpulse( parent );
	Vector colorNew = Lerp( imp, colorOld, colort );
	parent->SetCurrentColor( colorNew );
}