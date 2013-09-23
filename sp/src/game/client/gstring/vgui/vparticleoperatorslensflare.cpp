
#include "cbase.h"
#include "view.h"
#include "view_scene.h"
#include "glow_overlay.h"

#include "Gstring/vgui/vParticle.h"
#include "Gstring/vgui/vParticleOperatorsLensflare.h"

vParticleOperator_Lensflare_Base::vParticleOperator_Lensflare_Base( CGlowOverlay *src )
{
	pSrc = src;
	Assert( pSrc != NULL );
}
CGlowOverlay *vParticleOperator_Lensflare_Base::GetSourceEntity()
{
	return pSrc;
}


vParticleOperator_Lensflare_Size::vParticleOperator_Lensflare_Size( CGlowOverlay *src,
	float scale_min, float scale_max,
		bool scale_by_dot, float dot_min, float dot_max, float dot_bias )
		: vParticleOperator_Lensflare_Base( src )
{

	flScaleMin = scale_min;
	flScaleMax = scale_max;

	flDotMin = dot_min;
	flDotMax = dot_max;
	flDotBias = dot_bias;
	bUseDot = scale_by_dot;
}
void vParticleOperator_Lensflare_Size::Simulate( vParticle *parent )
{
	float glowAmt = GetSourceEntity()->GetGlowScale();
	float size = parent->GetCurrentSize();

	size *= RemapValClamped( Bias( glowAmt, flDotBias ), 0, 1, flScaleMin, flScaleMax );

	if ( bUseDot )
	{
		Vector vecGlowDir = GetSourceEntity()->GetGlowDirection();
		float dot = max( 0, DotProduct( vecGlowDir, MainViewForward() ) );

		size *= RemapValClamped( Bias( dot, flDotBias), flDotMin, flDotMax, flScaleMin, flScaleMax );
	}

	parent->SetCurrentSize( size );
}


vParticleOperator_Lensflare_Alpha::vParticleOperator_Lensflare_Alpha( CGlowOverlay *src,
	float alpha_min, float alpha_max,
		bool alpha_by_dot, float dot_min, float dot_max, float dot_bias )
		: vParticleOperator_Lensflare_Base( src )
{
	flAlphaMin = alpha_min;
	flAlphaMax = alpha_max;

	flDotMin = dot_min;
	flDotMax = dot_max;
	flDotBias = dot_bias;
	bUseDot = alpha_by_dot;
}
void vParticleOperator_Lensflare_Alpha::Simulate( vParticle *parent )
{
	float glowAmt = GetSourceEntity()->GetGlowScale();

	float alpha = parent->GetCurrentAlpha();
	alpha = RemapValClamped( Bias( glowAmt, flDotBias ), 0, 1, flAlphaMin, flAlphaMax );

	if ( bUseDot )
	{
		Vector vecGlowDir = GetSourceEntity()->GetGlowDirection();
		float dot = max( 0, DotProduct( vecGlowDir, MainViewForward() ) );

		alpha *= RemapValClamped( Bias( dot, flDotBias), flDotMin, flDotMax, flAlphaMin, flAlphaMax );
	}

	parent->SetCurrentAlpha( alpha );
}


vParticleOperator_Lensflare_Transformation::vParticleOperator_Lensflare_Transformation( CGlowOverlay *src,
		float distance, float angular_offset,
		bool auto_orient, float rot_offset, float rot_mult )
		: vParticleOperator_Lensflare_Base( src )
{
	flDist = distance;
	flAngularOffset = angular_offset;

	bDoOrientation = auto_orient;
	flRotationOffset = rot_offset;
	flRotationMultiplier = rot_mult;
}
void vParticleOperator_Lensflare_Transformation::Simulate( vParticle *parent )
{
	int w, t;
	engine->GetScreenSize( w, t );

	Vector worldOrigin = MainViewOrigin() + GetSourceEntity()->GetGlowDirection() * 100;
	Vector screen;

	bool bInvisible = ScreenTransform( worldOrigin, screen ) != 0;

	if ( bInvisible )
	{
		parent->SetCurrentAlpha( 0 );
		return;
	}

	screen *= Vector( 0.5f, -0.5f, 0 );
	screen += Vector( 0.5f, 0.5f, 0 );
	Vector2D glowScreenOrigin( w * screen.x, t * screen.y );

	Vector2D screenDelta = ( Vector2D( w / 2, t / 2 ) - glowScreenOrigin ) * 2;
	float len = screenDelta.NormalizeInPlace();

	float r = DEG2RAD( flAngularOffset );
	float c, s;
	FastSinCos( r, &s, &c );
	float xSafe = screenDelta.x;

	screenDelta.x = c * screenDelta.x - s * screenDelta.y;
	screenDelta.y = s * xSafe + c * screenDelta.y;

	screenDelta *= len * flDist;

	parent->vecPos = glowScreenOrigin + screenDelta;

	if ( bDoOrientation )
	{
		Vector2D dir = screenDelta * -1.0f;
		dir.NormalizeInPlace();
		
		parent->flAngle = RAD2DEG( atan2( dir.y, dir.x ) );
		parent->flAngle -= 90.0f;

		parent->flAngle *= flRotationMultiplier;
	}

	parent->flAngle += flRotationOffset;
}