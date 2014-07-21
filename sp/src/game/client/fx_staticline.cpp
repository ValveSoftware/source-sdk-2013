//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "clientsideeffects.h"
#include "fx_staticline.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/*
==================================================
CFXStaticLine
==================================================
*/

CFXStaticLine::CFXStaticLine( const char *name, const Vector& start, const Vector& end, float scale, float life, const char *shader, unsigned int flags )
: CClientSideEffect( name )
{
	assert( materials );
	if ( materials == NULL )
		return;

	// Create a material...
	m_pMaterial = materials->FindMaterial( shader, TEXTURE_GROUP_CLIENT_EFFECTS );
	m_pMaterial->IncrementReferenceCount();

	//Fill in the rest of the fields
	m_vecStart	= start;
	m_vecEnd	= end;
	m_uiFlags	= flags;
	m_fLife		= life;
	m_fScale	= scale * 0.5f;
}

CFXStaticLine::~CFXStaticLine( void )
{
	Destroy();
}

//==================================================
// Purpose:	Draw the primitive
// Input:	frametime - the time, this frame
//==================================================

void CFXStaticLine::Draw( double frametime )
{
	Vector lineDir, viewDir, cross;
	Vector tmp;

	// Update the effect
	Update( frametime );

	// Get the proper orientation for the line
	VectorSubtract( m_vecEnd, m_vecStart, lineDir );
	VectorSubtract( m_vecEnd, CurrentViewOrigin(), viewDir );
	
	cross = lineDir.Cross( viewDir );

	VectorNormalize( cross );

	CMatRenderContextPtr pRenderContext( materials );

	//Bind the material
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_pMaterial );
	CMeshBuilder meshBuilder;

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	bool flipVertical = (m_uiFlags & FXSTATICLINE_FLIP_VERTICAL) != 0;
	bool flipHorizontal = (m_uiFlags & FXSTATICLINE_FLIP_HORIZONTAL ) != 0;

	//Setup our points
	VectorMA( m_vecStart, -m_fScale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.Normal3fv( cross.Base() );
	if (flipHorizontal)
		meshBuilder.TexCoord2f( 0, 0.0f, 1.0f );
	else if (flipVertical)
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	else 
		meshBuilder.TexCoord2f( 0, 1.0f, 1.0f );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	VectorMA( m_vecStart, m_fScale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.Normal3fv( cross.Base() );
	if (flipHorizontal)
		meshBuilder.TexCoord2f( 0, 1.0f, 1.0f );
	else if (flipVertical)
		meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
	else 
		meshBuilder.TexCoord2f( 0, 0.0f, 1.0f );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	VectorMA( m_vecEnd, m_fScale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.Normal3fv( cross.Base() );
	if (flipHorizontal)
		meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
	else if (flipVertical)
		meshBuilder.TexCoord2f( 0, 1.0f, 1.0f );
	else 
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	VectorMA( m_vecEnd, -m_fScale, cross, tmp );
	meshBuilder.Position3fv( tmp.Base() );
	meshBuilder.Normal3fv( cross.Base() );
	if (flipHorizontal)
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	else if (flipVertical)
		meshBuilder.TexCoord2f( 0, 0.0f, 1.0f );
	else 
		meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

//==================================================
// Purpose:	Returns whether or not the effect is still active
// Output:	true if active
//==================================================

bool CFXStaticLine::IsActive( void )
{
	return ( m_fLife > 0.0 ) ? true : false;
}

//==================================================
// Purpose:	Destroy the effect and its local resources
//==================================================

void CFXStaticLine::Destroy( void )
{
	//Release the material
	if ( m_pMaterial != NULL )
	{
		m_pMaterial->DecrementReferenceCount();
		m_pMaterial = NULL;
	}
}

//==================================================
// Purpose: Perform any necessary updates
// Input:	frametime - the time, this frame
//==================================================

void CFXStaticLine::Update( double frametime )
{
	m_fLife -= frametime;
}
