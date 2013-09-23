
#include "cbase.h"
#include "Gstring/vgui/vParticle.h"
#include "Gstring/cFrametimeHelper.h"


vParticle::vParticle()
{
	vecPos.Init();
	flAngle = 0;

	vecVelocity.Init();
	flAngularVelocity = 0;

	vecColor_Default.Init( 1, 1, 1 );
	vecColor = vecColor_Default;

	flAlpha_Default = 1.0f;
	flAlpha = flAlpha_Default;

	flSize_Default_x = 10;
	flSize_x = flSize_Default_x;

	flSize_Default_y = 10;
	flSize_y = flSize_Default_y;

	flCreationTime = CFrameTimeHelper::GetCurrentTime();
	flLifeDuration = -1;

	pRenderer = NULL;
}

vParticle::~vParticle()
{
}

float vParticle::GetRelativeScale()
{
	int w, t;
	engine->GetScreenSize( w, t );
	return t / 640.0f;
}

void vParticle::SafeDelete()
{
	hOperators.PurgeAndDeleteElements();
	delete pRenderer;

	delete this;
}

void vParticle::SetSafeVelocity( Vector2D vel )
{
	vel *= GetRelativeScale();
	vecVelocity = vel;
}

void vParticle::Simulate()
{
	for ( int i = 0; i < hOperators.Count(); i++ )
		hOperators[ i ]->Simulate( this );
}

void vParticle::Render()
{
	Assert( pRenderer != NULL );

	if ( GetCurrentAlpha() <= 0.0f ||
		GetCurrentColor().LengthSqr() <= 0.0f ||
		GetCurrentSize() < 1 )
		return;

	pRenderer->Render( this );
}

void vParticle::CreateQuadRenderer( const char *pszMaterial )
{
	vParticleRenderer_Quad *r = new vParticleRenderer_Quad( pszMaterial );

	SetRenderer( r );
}

void vParticle::CreateRectRenderer( const char *pszMaterial, float scalex, float scaley )
{
	vParticleRenderer_Rect *r = new vParticleRenderer_Rect( pszMaterial, scalex, scaley );

	SetRenderer( r );
}

void vParticle::CreateMovementOperators( bool movement, float drag_translation, float drag_angle )
{
	if ( movement )
		AddParticleOperator( new vParticleOperator_Movement() );

	if ( drag_translation > 0 )
		AddParticleOperator( new vParticleOperator_Drag( drag_translation ) );

	if ( drag_angle > 0 )
		AddParticleOperator( new vParticleOperator_Drag_Angle( drag_angle ) );
}