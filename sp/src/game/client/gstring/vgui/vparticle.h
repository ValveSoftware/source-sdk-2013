#ifndef PARTICLE_H
#define PARTICLE_H

#include "cbase.h"

#include "Gstring/vgui/vParticleOperators.h"
#include "Gstring/vgui/vParticleRenderers.h"
#include "Gstring/cFrametimeHelper.h"

class vParticleOperatorBase;

struct vParticle
{
public:

	vParticle();

	void SafeDelete();
	void Simulate();
	void Render();

	static float GetRelativeScale();

	Vector2D vecPos;
	float flAngle;

	void SetSafeVelocity( Vector2D vel );

	Vector2D vecVelocity;
	float flAngularVelocity;

	inline void SetLifeDuration( float dur );
	inline float GetLifeDuration();
	inline float GetCreationTime();
	inline float GetDestructionTime();

	inline bool ShouldDie();

	inline void Revert();

	inline void SetStartColor( Vector col );
	inline void SetStartAlpha( float a );
	inline void SetStartSize_Absolute( float size );
	inline void SetStartSize_Absolute( float sizex, float sizey );
	inline void SetStartSize_Relative( float size );
	inline void SetStartSize_Relative( float sizex, float sizey );

	inline void SetCurrentColor( Vector col );
	inline void SetCurrentAlpha( float a );
	inline void SetCurrentSize( float size );
	inline void SetCurrentSize( float sizex, float sizey );
	inline Vector GetCurrentColor();
	inline float GetCurrentAlpha();
	inline float GetCurrentSize();
	inline float GetCurrentSizeX();
	inline float GetCurrentSizeY();

	inline void AddParticleOperator( vParticleOperatorBase *op ); // takes ownership
	inline void SetRenderer( vParticleRendererBase *r ); // takes ownership

	void CreateQuadRenderer( const char *pszMaterial );
	void CreateRectRenderer( const char *pszMaterial, float scalex, float scaley );
	void CreateMovementOperators( bool movement = true, float drag_translation = 0, float drag_angle = 0 );

private:
	~vParticle();
	vParticle( const vParticle &o );

	CUtlVector< vParticleOperatorBase* >hOperators;
	vParticleRendererBase *pRenderer;

	Vector vecColor;
	Vector vecColor_Default;

	float flAlpha;
	float flAlpha_Default;

	float flSize_x;
	float flSize_Default_x;
	float flSize_y;
	float flSize_Default_y;

	float flCreationTime;
	float flLifeDuration;
};

void vParticle::SetLifeDuration( float dur )
{
	flLifeDuration = dur;
}
float vParticle::GetLifeDuration()
{
	return flLifeDuration;
}
float vParticle::GetCreationTime()
{
	return flCreationTime;
}
float vParticle::GetDestructionTime()
{
	return GetCreationTime() + GetLifeDuration();
}
bool vParticle::ShouldDie()
{
	return GetLifeDuration() >= 0 &&
		( GetCreationTime() > CFrameTimeHelper::GetCurrentTime() ||
		GetDestructionTime() < CFrameTimeHelper::GetCurrentTime() );
}

void vParticle::Revert()
{
	vecColor = vecColor_Default;
	flAlpha = flAlpha_Default;
	flSize_x = flSize_Default_x;
	flSize_y = flSize_Default_y;
}
void vParticle::SetStartColor( Vector col )
{
	vecColor = col;
	vecColor_Default = col;
}
void vParticle::SetStartAlpha( float a )
{
	flAlpha = a;
	flAlpha_Default = a;
}
void vParticle::SetStartSize_Absolute( float size )
{
	flSize_x = size / 2;
	flSize_Default_x = flSize_x;
	flSize_Default_y = flSize_y = flSize_x;
}
void vParticle::SetStartSize_Absolute( float sizex, float sizey )
{
	flSize_x = sizex / 2;
	flSize_Default_x = flSize_x;

	flSize_y = sizey / 2;
	flSize_Default_y = flSize_y;
}
void vParticle::SetStartSize_Relative( float size )
{
	flSize_x = size / 2 * GetRelativeScale();
	flSize_Default_x = flSize_x;
	flSize_Default_y = flSize_y = flSize_x;
}
void vParticle::SetStartSize_Relative( float sizex, float sizey )
{
	flSize_x = sizex / 2 * GetRelativeScale();
	flSize_Default_x = flSize_x;

	flSize_y = sizey / 2 * GetRelativeScale();
	flSize_Default_y = flSize_y;
}
void vParticle::SetCurrentColor( Vector col )
{
	vecColor = col;
}
void vParticle::SetCurrentAlpha( float a )
{
	flAlpha = a;
}
void vParticle::SetCurrentSize( float size )
{
	flSize_x = size;
	flSize_y = size;
}
void vParticle::SetCurrentSize( float sizex, float sizey )
{
	flSize_x = sizex;
	flSize_y = sizey;
}
Vector vParticle::GetCurrentColor()
{
	return vecColor;
}
float vParticle::GetCurrentAlpha()
{
	return flAlpha;
}
float vParticle::GetCurrentSize()
{
	return flSize_x;
}
float vParticle::GetCurrentSizeX()
{
	return flSize_x;
}
float vParticle::GetCurrentSizeY()
{
	return flSize_y;
}
void vParticle::AddParticleOperator( vParticleOperatorBase *op )
{
	Assert( op != NULL );
	hOperators.AddToTail( op );
}
void vParticle::SetRenderer( vParticleRendererBase *r )
{
	delete pRenderer;
	pRenderer = r;
}

#endif