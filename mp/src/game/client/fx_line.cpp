//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "materialsystem/imaterial.h"
#include "clientsideeffects.h"
#include "fx_line.h"
#include "materialsystem/imesh.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/*
==================================================
CFXLine
==================================================
*/

CFXLine::CFXLine( const char *name, const FXLineData_t &data )
: CClientSideEffect( name )
{
	m_FXData = data;
	
	m_FXData.m_flLifeTime = 0.0f;
	
	if ( m_FXData.m_pMaterial != NULL )
	{
		m_FXData.m_pMaterial->IncrementReferenceCount();
	}
}

CFXLine::~CFXLine( void )
{
	Destroy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : frametime - 
//-----------------------------------------------------------------------------
void CFXLine::Draw( double frametime )
{
	// Update the effect
	Update( frametime );

	Vector lineDir, viewDir;
	
	//Get the proper orientation for the line
	VectorSubtract( m_FXData.m_vecStart, m_FXData.m_vecEnd, lineDir );
	VectorSubtract( m_FXData.m_vecEnd, CurrentViewOrigin(), viewDir );
	
	Vector cross = lineDir.Cross( viewDir );

	VectorNormalize( cross );

	CMatRenderContextPtr pRenderContext( materials );
	
	//Bind the material
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_FXData.m_pMaterial );
	
	CMeshBuilder meshBuilder;

	Vector tmp;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	float scaleTimePerc = ( m_FXData.m_flLifeTime / m_FXData.m_flDieTime );
	float scale = m_FXData.m_flStartScale + ( ( m_FXData.m_flEndScale - m_FXData.m_flStartScale ) * scaleTimePerc );

	color32 color = {255,255,255,255};

	float alpha = m_FXData.m_flStartAlpha + ( ( m_FXData.m_flEndAlpha - m_FXData.m_flStartAlpha ) * scaleTimePerc );
	alpha = clamp( alpha, 0.0f, 1.0f );

	color.a *= alpha;

	// Start
	VectorMA( m_FXData.m_vecStart, -scale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 1.0f, 1.0f );
	meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();

	VectorMA( m_FXData.m_vecStart, scale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 0.0f, 1.0f );
	meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();

	// End
	VectorMA( m_FXData.m_vecEnd, scale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();

	VectorMA( m_FXData.m_vecEnd, -scale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
	meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFXLine::IsActive( void )
{
	return ( m_FXData.m_flLifeTime < m_FXData.m_flDieTime ) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFXLine::Destroy( void )
{
	//Release the material
	if ( m_FXData.m_pMaterial != NULL )
	{
		m_FXData.m_pMaterial->DecrementReferenceCount();
		m_FXData.m_pMaterial = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : frametime - 
//-----------------------------------------------------------------------------
void CFXLine::Update( double frametime )
{
	m_FXData.m_flLifeTime += frametime;

	//Move our end points
	VectorMA( m_FXData.m_vecStart, frametime, m_FXData.m_vecStartVelocity, m_FXData.m_vecStart );
	VectorMA( m_FXData.m_vecEnd, frametime, m_FXData.m_vecEndVelocity, m_FXData.m_vecEnd );
}

void FX_DrawLine( const Vector &start, const Vector &end, float scale, IMaterial *pMaterial, const color32 &color )
{
	Vector			lineDir, viewDir;
	//Get the proper orientation for the line
	VectorSubtract( end, start, lineDir );
	VectorSubtract( end, CurrentViewOrigin(), viewDir );
	
	Vector cross = lineDir.Cross( viewDir );

	VectorNormalize( cross );

	CMatRenderContextPtr pRenderContext( materials );
	
	//Bind the material
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, pMaterial );
	CMeshBuilder meshBuilder;

	Vector			tmp;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	VectorMA( start, -scale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 1.0f, 1.0f );
	meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();

	VectorMA( start, scale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 0.0f, 1.0f );
	meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();

	VectorMA( end, scale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();

	VectorMA( end, -scale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
	meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}


void FX_DrawLineFade( const Vector &start, const Vector &end, float scale, IMaterial *pMaterial, const color32 &color, float fadeDist )
{
	Vector			lineDir, viewDir;
	//Get the proper orientation for the line
	VectorSubtract( end, start, lineDir );
	VectorSubtract( end, CurrentViewOrigin(), viewDir );
	
	float lineLength = lineDir.Length();
	float t0 = 0.25f;
	float t1 = 0.75f;
	if ( lineLength > 0 )
	{
		t0 = fadeDist / lineLength;
		t0 = clamp( t0, 0.0f, 0.25f );
		t1 = 1.0f - t0;
	}

	Vector cross = lineDir.Cross( viewDir );

	VectorNormalize( cross );

	CMatRenderContextPtr pRenderContext( materials );

	//Bind the material
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, pMaterial );
	CMeshBuilder meshBuilder;

	Vector			tmp;
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, 8, 24 );

	//		2			5
	// 0	1			4	 7
	//		3			6

	// 0,2,1 - 0,1,3 - 7,4,5 - 7,6,4 - 1,4,6, 1,6,3 - 1,5,4 - 1,2,5

	// v0
	meshBuilder.Position3fv( start.Base() );
	meshBuilder.TexCoord2f( 0, 0.5f, 0.0f );
	meshBuilder.Color4ub( 0, 0, 0, 0 );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();
	// v1
	Vector v1 = start + t0 * lineDir;
	meshBuilder.Position3fv( v1.Base() );
	meshBuilder.TexCoord2f( 0, 0.5f, t0 );
	meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();
	// v2
	tmp = v1 - scale*cross;
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 1.0f, t0 );
	meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();
	// v3
	tmp = v1 + scale*cross;
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 0.0f, t0 );
	meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();
	// v4
	Vector v4 = start + t1 * lineDir;
	meshBuilder.Position3fv( v4.Base() );
	meshBuilder.TexCoord2f( 0, 0.5f, t1 );
	meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();
	// v5
	tmp = v4 - scale*cross;
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 1.0f, t1 );
	meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();
	// v6
	tmp = v4 + scale*cross;
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.TexCoord2f( 0, 0.0f, t1 );
	meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();
	// v7
	meshBuilder.Position3fv( end.Base() );
	meshBuilder.TexCoord2f( 0, 0.5f, 1.0f );
	meshBuilder.Color4ub( 0, 0, 0, 0 );
	meshBuilder.Normal3fv( cross.Base() );
	meshBuilder.AdvanceVertex();


	// triangles - 0,2,1 - 0,1,3 - 7,4,5 - 7,6,4 - 1,4,6, 1,6,3 - 1,5,4 - 1,2,5
	meshBuilder.FastIndex( 0 ); meshBuilder.FastIndex( 2 ); meshBuilder.FastIndex( 1 );
	meshBuilder.FastIndex( 0 ); meshBuilder.FastIndex( 1 ); meshBuilder.FastIndex( 3 );

	meshBuilder.FastIndex( 7 ); meshBuilder.FastIndex( 4 ); meshBuilder.FastIndex( 5 );
	meshBuilder.FastIndex( 7 ); meshBuilder.FastIndex( 6 ); meshBuilder.FastIndex( 4 );

	meshBuilder.FastIndex( 1 ); meshBuilder.FastIndex( 4 ); meshBuilder.FastIndex( 6 );
	meshBuilder.FastIndex( 1 ); meshBuilder.FastIndex( 6 ); meshBuilder.FastIndex( 3 );
	meshBuilder.FastIndex( 1 ); meshBuilder.FastIndex( 5 ); meshBuilder.FastIndex( 4 );
	meshBuilder.FastIndex( 1 ); meshBuilder.FastIndex( 2 ); meshBuilder.FastIndex( 5 );

	meshBuilder.End();
	pMesh->Draw();
}
