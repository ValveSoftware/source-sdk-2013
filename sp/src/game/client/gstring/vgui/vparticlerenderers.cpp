
#include "cbase.h"
#include "Gstring/vgui/vUtil.h"
#include "Gstring/vgui/vParticle.h"
#include "Gstring/cFrametimeHelper.h"

#include <vgui/ISurface.h>
#include <vgui_controls/controls.h>


using namespace vgui;

int vParticleRenderer_vMaterialBase::m_iTextureIDWireframe = -1;

void vParticleRendererBase::Render( vParticle *p )
{
	Assert( 0 );
}

vParticleRenderer_vMaterialBase::vParticleRenderer_vMaterialBase( const char *szMat )
{
	SetupVGUITex( szMat, iTextureID );
	SetupVGUITex( "debug/debugspritewireframe", m_iTextureIDWireframe );
}

vParticleRenderer_Rect::vParticleRenderer_Rect( const char *szMat, float scale_x, float scale_y ) : vParticleRenderer_vMaterialBase( szMat )
{
	m_flScale_x = scale_x;
	m_flScale_y = scale_y;
}
void vParticleRenderer_Rect::Render( vParticle *p )
{
	float ang = DEG2RAD( p->flAngle );

	Vector2D right( cos( ang ), sin( ang ) );
	Vector2D up( right.y, -right.x );

	right *= p->GetCurrentSizeX();
	up *= p->GetCurrentSizeY();

	right *= m_flScale_x;
	up *= m_flScale_y;

	Vector2D &pos = p->vecPos;

	Vertex_t v[4] = {
		Vertex_t( pos + up - right, Vector2D( 0, 0 ) ),
		Vertex_t( pos + up + right, Vector2D( 1, 0 ) ),
		Vertex_t( pos + right - up, Vector2D( 1, 1 ) ),
		Vertex_t( pos + -right - up, Vector2D( 0, 1 ) )
	};

	const Vector col = p->GetCurrentColor() * 255;

	surface()->DrawSetColor( Color( col.x, col.y, col.z, p->GetCurrentAlpha() * 255 ) );
	surface()->DrawSetTexture( GetTextureID() );
	surface()->DrawTexturedPolygon( 4, v );
}


vParticleRenderer_Trail::vParticleRenderer_Trail( const char *szMat, int maxSteps, float stepDelay )
	: vParticleRenderer_vMaterialBase( szMat )
{
	flNextAddTime = 0;
	vecLastPos.Init();
	bClean = true;

	iMaxSteps = maxSteps;
	flStepDelay = stepDelay;
}
void vParticleRenderer_Trail::Render( vParticle *p )
{
	RenderInternal( p );

	if ( !(vecLastPos-p->vecPos).IsZero() )
		vecLastPos = p->vecPos;
}
void vParticleRenderer_Trail::RenderInternal( vParticle *p )
{
	int numStepsRecorded = hSteps.Count();
	bool bMoved = (vecLastPos-p->vecPos).LengthSqr() > 1.0f;

	if ( numStepsRecorded == 0 )
	{
		if ( bMoved )
		{
			if ( bClean )
			{
				vecLastPos = p->vecPos;
				bClean = false;
			}
			else
			{
				AddStep( p );
				vecLastPos = p->vecPos;
			}
		}
		return;
	}

	if ( numStepsRecorded == 1 && !bMoved )
		return;

	UpdateSteps( p );

	RenderTrail( p );
}
void vParticleRenderer_Trail::UpdateSteps( vParticle *p )
{
	bool bMoved = (vecLastPos-p->vecPos).LengthSqr() > 1.0f;
	if ( flNextAddTime > CFrameTimeHelper::GetCurrentTime() || !bMoved )
		return;

	AddStep( p );

	flNextAddTime = CFrameTimeHelper::GetCurrentTime() + flStepDelay;

	while ( hSteps.Count() > iMaxSteps + 1 )
		hSteps.Remove( 0 );
}
void vParticleRenderer_Trail::SetStepData( vParticle *p, vTrailStep_t &step )
{
	step.center = p->vecPos;

	Vector2D up = vecLastPos - p->vecPos;
	up.NormalizeInPlace();

	Vector2D right( -up.y, up.x );
	right *= p->GetCurrentSize();

	step.a = p->vecPos - right;
	step.b = p->vecPos + right;
}
void vParticleRenderer_Trail::AddStep( vParticle *p )
{
	vTrailStep_t step;

	SetStepData( p, step );

	hSteps.AddToTail( step );
}
void vParticleRenderer_Trail::RenderTrail( vParticle *p )
{
	bool bMoved = (vecLastPos-p->vecPos).LengthSqr() > 1.0f;

	Assert( hSteps.Count() >= 2 || hSteps.Count() == 1 && bMoved );

	if ( hSteps.Count() == 1 )
		AddStep( p );

	if ( bMoved )
		SetStepData( p, hSteps[hSteps.Count()-1] );

	float totalLength = 0;
	for ( int i = 1; i < hSteps.Count(); i++ )
		totalLength += ( hSteps[i-1].center - hSteps[i].center).Length();

	float mappableLength = totalLength;
	if ( bMoved && hSteps.Count() >= iMaxSteps )
		mappableLength -= (hSteps[hSteps.Count()-1].center - hSteps[hSteps.Count()-2].center).Length();

	if ( mappableLength <= 0.0f )
		return;

	const Vector col = p->GetCurrentColor() * 255;
	surface()->DrawSetColor( Color( col.x, col.y, col.z, p->GetCurrentAlpha() * 255 ) );
	surface()->DrawSetTexture( GetTextureID() );

	float uv_y = 1.0f;
	for ( int i = hSteps.Count() - 1; i > 0; i-- )
	{
		vTrailStep_t &front = hSteps[i];
		vTrailStep_t &back = hSteps[i-1];

		float curMappableDelta = (front.center-back.center).Length();
		float curUVDelta = curMappableDelta / mappableLength;

		float uv_y_low = uv_y - curUVDelta;

		Vertex_t v[4] = {
			Vertex_t( back.a, Vector2D( 0, uv_y_low ) ),
			Vertex_t( back.b, Vector2D( 1, uv_y_low ) ),
			Vertex_t( front.b, Vector2D( 1, uv_y ) ),
			Vertex_t( front.a, Vector2D( 0, uv_y ) )
		};

		surface()->DrawTexturedPolygon( 4, v );

		uv_y -= curUVDelta;
	}

	//if ( bMoved )
	//	hSteps.Remove( hSteps.Count() - 1 );
}