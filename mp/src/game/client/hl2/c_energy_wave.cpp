//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's energy wave
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imesh.h"
#include "energy_wave_effect.h"
#include "mathlib/vmatrix.h"
#include "clienteffectprecachesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEnergyWave )
CLIENTEFFECT_MATERIAL( "effects/energywave/energywave" )
CLIENTEFFECT_REGISTER_END()

//-----------------------------------------------------------------------------
// Energy Wave: 
//-----------------------------------------------------------------------------

class C_EnergyWave : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_EnergyWave, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_EnergyWave();
	~C_EnergyWave();

	void PostDataUpdate( DataUpdateType_t updateType );
	int	DrawModel( int flags );
	void ComputePoint( float s, float t, Vector& pt, Vector& normal, float& opacity );
	void DrawWireframeModel( );

	CEnergyWaveEffect m_EWaveEffect;

	IMaterial* m_pWireframe;
	IMaterial* m_pEWaveMat;

private:
	C_EnergyWave( const C_EnergyWave & ); // not defined, not accessible

	void ComputeEWavePoints( Vector* pt, Vector* normal, float* opacity );
	void DrawEWavePoints(Vector* pt, Vector* normal, float* opacity);

};


EXTERN_RECV_TABLE(DT_BaseEntity);

IMPLEMENT_CLIENTCLASS_DT(C_EnergyWave, DT_EWaveEffect, CEnergyWave)
END_RECV_TABLE()


// ----------------------------------------------------------------------------
// Functions.
// ----------------------------------------------------------------------------

C_EnergyWave::C_EnergyWave() : m_EWaveEffect(NULL, NULL)
{
	m_pWireframe = materials->FindMaterial("shadertest/wireframevertexcolor", TEXTURE_GROUP_OTHER);
	m_pEWaveMat  = materials->FindMaterial("effects/energywave/energywave", TEXTURE_GROUP_CLIENT_EFFECTS);
	m_EWaveEffect.Spawn();
}

C_EnergyWave::~C_EnergyWave()
{
}

void C_EnergyWave::PostDataUpdate( DataUpdateType_t updateType )
{
	MarkMessageReceived();

	// Make sure that origin points to current origin, at least
	MoveToLastReceivedPosition();
}



enum
{
	NUM_SUBDIVISIONS = 21,
};


static void ComputeIndices( int is, int it, int* idx )
{
	int is0 = (is > 0) ? (is - 1) : is;
	int it0 = (it > 0) ? (it - 1) : it;
	int is1 = (is < EWAVE_NUM_HORIZONTAL_POINTS - 1) ? is + 1 : is;
	int it1 = (it < EWAVE_NUM_HORIZONTAL_POINTS - 1) ? it + 1 : it;
	int is2 = is + 2; 
	int it2 = it + 2;
	if (is2 >= EWAVE_NUM_HORIZONTAL_POINTS)
		is2 = EWAVE_NUM_HORIZONTAL_POINTS - 1;
	if (it2 >= EWAVE_NUM_HORIZONTAL_POINTS)
		it2 = EWAVE_NUM_HORIZONTAL_POINTS - 1;

	idx[0] = is0 + it0 * EWAVE_NUM_HORIZONTAL_POINTS;
	idx[1] = is  + it0 * EWAVE_NUM_HORIZONTAL_POINTS;
	idx[2] = is1 + it0 * EWAVE_NUM_HORIZONTAL_POINTS;
	idx[3] = is2 + it0 * EWAVE_NUM_HORIZONTAL_POINTS;

	idx[4] = is0 + it * EWAVE_NUM_HORIZONTAL_POINTS;
	idx[5] = is  + it * EWAVE_NUM_HORIZONTAL_POINTS;
	idx[6] = is1 + it * EWAVE_NUM_HORIZONTAL_POINTS;
	idx[7] = is2 + it * EWAVE_NUM_HORIZONTAL_POINTS;

	idx[8]  = is0 + it1 * EWAVE_NUM_HORIZONTAL_POINTS;
	idx[9]  = is  + it1 * EWAVE_NUM_HORIZONTAL_POINTS;
	idx[10] = is1 + it1 * EWAVE_NUM_HORIZONTAL_POINTS;
	idx[11] = is2 + it1 * EWAVE_NUM_HORIZONTAL_POINTS;

	idx[12] = is0 + it2 * EWAVE_NUM_HORIZONTAL_POINTS;
	idx[13] = is  + it2 * EWAVE_NUM_HORIZONTAL_POINTS;
	idx[14] = is1 + it2 * EWAVE_NUM_HORIZONTAL_POINTS;
	idx[15] = is2 + it2 * EWAVE_NUM_HORIZONTAL_POINTS;
}

void C_EnergyWave::ComputePoint( float s, float t, Vector& pt, Vector& normal, float& opacity )
{
	int is = (int)s;
	int it = (int)t;
	if( is >= EWAVE_NUM_HORIZONTAL_POINTS )
		is -= 1;

	if( it >= EWAVE_NUM_VERTICAL_POINTS )
		it -= 1;

	int idx[16];
	ComputeIndices( is, it, idx );

	// The patch equation is:
	// px = S * M * Gx * M^T * T^T 
	// py = S * M * Gy * M^T * T^T 
	// pz = S * M * Gz * M^T * T^T 
	// where S = [s^3 s^2 s 1], T = [t^3 t^2 t 1]
	// M is the patch type matrix, in my case I'm using a catmull-rom
	// G is the array of control points. rows have constant t
	static VMatrix catmullRom( -0.5, 1.5, -1.5, 0.5,
								1, -2.5, 2, -0.5,
								-0.5, 0, 0.5, 0,
								0, 1, 0, 0 );

	VMatrix controlPointsX, controlPointsY, controlPointsZ, controlPointsO;

	Vector pos;
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			const Vector& v = m_EWaveEffect.GetPoint( idx[i * 4 + j] );

			controlPointsX[j][i] = v.x;
			controlPointsY[j][i] = v.y;
			controlPointsZ[j][i] = v.z;

			controlPointsO[j][i] = m_EWaveEffect.ComputeOpacity( v, GetAbsOrigin() );
		}
	}

	float fs = s - is;
	float ft = t - it;

	VMatrix temp, mgm[4];
	MatrixTranspose( catmullRom, temp );
	MatrixMultiply( controlPointsX, temp, mgm[0] );
	MatrixMultiply( controlPointsY, temp, mgm[1] );
	MatrixMultiply( controlPointsZ, temp, mgm[2] );
	MatrixMultiply( controlPointsO, temp, mgm[3] );

	MatrixMultiply( catmullRom, mgm[0], mgm[0] );
	MatrixMultiply( catmullRom, mgm[1], mgm[1] );
	MatrixMultiply( catmullRom, mgm[2], mgm[2] );
	MatrixMultiply( catmullRom, mgm[3], mgm[3] );

	Vector4D svec, tvec;
	float ft2 = ft * ft;
	tvec[0] = ft2 * ft; tvec[1] = ft2; tvec[2] = ft; tvec[3] = 1.0f;

 	float fs2 = fs * fs;
	svec[0] = fs2 * fs; svec[1] = fs2; svec[2] = fs; svec[3] = 1.0f;

	Vector4D tmp;
	Vector4DMultiply( mgm[0], tvec, tmp );
	pt[0] = DotProduct4D( tmp, svec );
	Vector4DMultiply( mgm[1], tvec, tmp );
	pt[1] = DotProduct4D( tmp, svec );
	Vector4DMultiply( mgm[2], tvec, tmp );
	pt[2] = DotProduct4D( tmp, svec );

	Vector4DMultiply( mgm[3], tvec, tmp );
	opacity = DotProduct4D( tmp, svec );

	if ((s == 0.0f) || (t == 0.0f) ||
		(s == (EWAVE_NUM_HORIZONTAL_POINTS-1.0f)) || (t == (EWAVE_NUM_VERTICAL_POINTS-1.0f)) )
	{
		opacity = 0.0f;
	}

	if ((s <= 0.3) || (t < 0.3))
	{
		opacity *= 0.35f;
	}
	if ((s == (EWAVE_NUM_HORIZONTAL_POINTS-0.7f)) || (t == (EWAVE_NUM_VERTICAL_POINTS-0.7f)) )
	{
		opacity *= 0.35f;
	}

	if (opacity < 0.0f)
		opacity = 0.0f;
	else if (opacity > 255.0f)
		opacity = 255.0f;

	// Normal computation
	Vector4D dsvec, dtvec;
	dsvec[0] = 3.0f * fs2; dsvec[1] = 2.0f * fs; dsvec[2] = 1.0f; dsvec[3] = 0.0f;
	dtvec[0] = 3.0f * ft2; dtvec[1] = 2.0f * ft; dtvec[2] = 1.0f; dtvec[3] = 0.0f;

	Vector ds, dt;
	Vector4DMultiply( mgm[0], tvec, tmp );
	ds[0] = DotProduct4D( tmp, dsvec );
	Vector4DMultiply( mgm[1], tvec, tmp );
	ds[1] = DotProduct4D( tmp, dsvec );
	Vector4DMultiply( mgm[2], tvec, tmp );
	ds[2] = DotProduct4D( tmp, dsvec );

	Vector4DMultiply( mgm[0], dtvec, tmp );
	dt[0] = DotProduct4D( tmp, svec );
	Vector4DMultiply( mgm[1], dtvec, tmp );
	dt[1] = DotProduct4D( tmp, svec );
	Vector4DMultiply( mgm[2], dtvec, tmp );
	dt[2] = DotProduct4D( tmp, svec );

	CrossProduct( ds, dt, normal );
	VectorNormalize( normal );
}

void C_EnergyWave::DrawWireframeModel( )
{
	IMesh* pMesh = materials->GetDynamicMesh( true, NULL, NULL, m_pWireframe );

	int numLines = (EWAVE_NUM_VERTICAL_POINTS - 1) * EWAVE_NUM_HORIZONTAL_POINTS +
		EWAVE_NUM_VERTICAL_POINTS * (EWAVE_NUM_HORIZONTAL_POINTS - 1);

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_LINES, numLines );

	Vector tmp;
	for (int i = 0; i < EWAVE_NUM_VERTICAL_POINTS; ++i)
	{
		for (int j = 0; j < EWAVE_NUM_HORIZONTAL_POINTS; ++j)
		{
			if ( i > 0 )
			{
				meshBuilder.Position3fv( m_EWaveEffect.GetPoint( j, i ).Base() );
				meshBuilder.Color4ub( 255, 255, 255, 128 );
				meshBuilder.AdvanceVertex();

				meshBuilder.Position3fv( m_EWaveEffect.GetPoint( j, i - 1 ).Base() );
				meshBuilder.Color4ub( 255, 255, 255, 128 );
				meshBuilder.AdvanceVertex();
			}

			if (j > 0)
			{
				meshBuilder.Position3fv( m_EWaveEffect.GetPoint( j, i ).Base() );
				meshBuilder.Color4ub( 255, 255, 255, 128 );
				meshBuilder.AdvanceVertex();

				meshBuilder.Position3fv( m_EWaveEffect.GetPoint( j - 1, i ).Base() );
				meshBuilder.Color4ub( 255, 255, 255, 128 );
				meshBuilder.AdvanceVertex();
			}
		}
	}

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Compute the ewave points using catmull-rom
//-----------------------------------------------------------------------------

void C_EnergyWave::ComputeEWavePoints( Vector* pt, Vector* normal, float* opacity )
{
	int i;
	for ( i = 0; i < NUM_SUBDIVISIONS; ++i)
	{
		float t = (EWAVE_NUM_VERTICAL_POINTS -1 ) * (float)i / (float)(NUM_SUBDIVISIONS - 1);
		for (int j = 0; j < NUM_SUBDIVISIONS; ++j)
		{
			float s = (EWAVE_NUM_HORIZONTAL_POINTS-1) * (float)j / (float)(NUM_SUBDIVISIONS - 1);
			int idx = i * NUM_SUBDIVISIONS + j;

			ComputePoint( s, t, pt[idx], normal[idx], opacity[idx] );
		}
	}
}

//-----------------------------------------------------------------------------
// Draws the base ewave
//-----------------------------------------------------------------------------

#define TRANSITION_REGION_WIDTH 0.5f

void C_EnergyWave::DrawEWavePoints(Vector* pt, Vector* normal, float* opacity)
{
	IMesh* pMesh = materials->GetDynamicMesh( true, NULL, NULL, m_pEWaveMat );

	int numTriangles = (NUM_SUBDIVISIONS - 1) * (NUM_SUBDIVISIONS - 1) * 2;

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, numTriangles );

	float du = 1.0f / (float)(NUM_SUBDIVISIONS - 1);
	float dv = du;

	unsigned char color[3];
	color[0] = 255;
	color[1] = 255;
	color[2] = 255;

	for ( int i = 0; i < NUM_SUBDIVISIONS - 1; ++i)
	{
		float v = i * dv;
		for (int j = 0; j < NUM_SUBDIVISIONS - 1; ++j)
		{
			int idx = i * NUM_SUBDIVISIONS + j;
			float u = j * du;

			meshBuilder.Position3fv( pt[idx].Base() );
			meshBuilder.Color4ub( color[0], color[1], color[2], opacity[idx] );
			meshBuilder.Normal3fv( normal[idx].Base() );
			meshBuilder.TexCoord2f( 0, u, v );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( pt[idx + NUM_SUBDIVISIONS].Base() );
			meshBuilder.Color4ub( color[0], color[1], color[2], opacity[idx+NUM_SUBDIVISIONS] );
			meshBuilder.Normal3fv( normal[idx + NUM_SUBDIVISIONS].Base() );
			meshBuilder.TexCoord2f( 0, u, v + dv );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( pt[idx + 1].Base() );
			meshBuilder.Color4ub( color[0], color[1], color[2], opacity[idx+1] );
			meshBuilder.Normal3fv( normal[idx+1].Base() );
			meshBuilder.TexCoord2f( 0, u + du, v );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( pt[idx + 1].Base() );
			meshBuilder.Color4ub( color[0], color[1], color[2], opacity[idx+1] );
			meshBuilder.Normal3fv( normal[idx+1].Base() );
			meshBuilder.TexCoord2f( 0, u + du, v );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( pt[idx + NUM_SUBDIVISIONS].Base() );
			meshBuilder.Color4ub( color[0], color[1], color[2], opacity[idx+NUM_SUBDIVISIONS] );
			meshBuilder.Normal3fv( normal[idx + NUM_SUBDIVISIONS].Base() );
			meshBuilder.TexCoord2f( 0, u, v + dv );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( pt[idx + NUM_SUBDIVISIONS + 1].Base() );
			meshBuilder.Color4ub( color[0], color[1], color[2], opacity[idx+NUM_SUBDIVISIONS+1] );
			meshBuilder.Normal3fv( normal[idx + NUM_SUBDIVISIONS + 1].Base() );
			meshBuilder.TexCoord2f( 0, u + du, v + dv );
			meshBuilder.AdvanceVertex();
		}
	}

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Main draw entry point
//-----------------------------------------------------------------------------

int	C_EnergyWave::DrawModel( int flags )
{
	if ( !m_bReadyToDraw )
		return 0;

	// NOTE: We've got a stiff spring case here, we need to simulate at
	// a fairly fast timestep. A better solution would be to use an 
	// implicit method, which I'm going to not implement for the moment

	float dt = gpGlobals->frametime;
	m_EWaveEffect.SetPosition( GetAbsOrigin(), GetAbsAngles() );
	m_EWaveEffect.Simulate(dt);

	Vector pt[NUM_SUBDIVISIONS * NUM_SUBDIVISIONS];
	Vector normal[NUM_SUBDIVISIONS * NUM_SUBDIVISIONS];
	float opacity[NUM_SUBDIVISIONS * NUM_SUBDIVISIONS];

	ComputeEWavePoints( pt, normal, opacity );

    DrawEWavePoints( pt, normal, opacity );

	return 1;
}





