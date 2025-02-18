//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
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
#include "fx_discreetline.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/*
==================================================
CFXLine
==================================================
*/

CFXDiscreetLine::CFXDiscreetLine( const char *name, const Vector& start, const Vector& direction, 
	float velocity, float length, float clipLength, float scale, float life, const char *shader )
: CClientSideEffect( name )
{
	assert( materials );
	if ( materials == NULL )
		return;

	// Create a material...
	m_pMaterial = materials->FindMaterial( shader, TEXTURE_GROUP_CLIENT_EFFECTS );
	m_pMaterial->IncrementReferenceCount();

	m_vecOrigin			= start;
	m_vecDirection		= direction;
	m_fVelocity			= velocity;
	m_fClipLength		= clipLength;
	m_fScale			= scale;
	m_fLife				= life;
	m_fStartTime		= 0.0f;
	m_fLength			= length;
}

CFXDiscreetLine::~CFXDiscreetLine( void )
{
	Destroy();
}

// Does extra calculations to make them more visible over distance
ConVar	tracer_extra( "tracer_extra", "1" );

/*
==================================================
Draw
==================================================
*/

void CFXDiscreetLine::Draw( double frametime )
{
	Vector			lineDir, viewDir, cross;

	Vector			vecEnd, vecStart;
	Vector tmp;

	// Update the effect
	Update( frametime );

	// Calculate our distance along our path
	float	sDistance = m_fVelocity * m_fStartTime;
	float	eDistance = sDistance - m_fLength;
	
	//Clip to start
	sDistance = MAX( 0.0f, sDistance );
	eDistance = MAX( 0.0f, eDistance );

	if ( ( sDistance == 0.0f ) && ( eDistance == 0.0f ) )
		return;

	// Clip it
	if ( m_fClipLength != 0.0f )
	{
		sDistance = MIN( sDistance, m_fClipLength );
		eDistance = MIN( eDistance, m_fClipLength );
	}

	// Get our delta to calculate the tc offset
	float	dDistance	= fabs( sDistance - eDistance );
	float	dTotal		= ( m_fLength != 0.0f ) ? m_fLength : 0.01f;
	float	fOffset		= ( dDistance / dTotal );

	// Find our points along our path
	VectorMA( m_vecOrigin, sDistance, m_vecDirection, vecEnd );
	VectorMA( m_vecOrigin, eDistance, m_vecDirection, vecStart );

	//Setup our info for drawing the line
	VectorSubtract( vecEnd, vecStart, lineDir );
	VectorSubtract( vecEnd, CurrentViewOrigin(), viewDir );
	
	cross = lineDir.Cross( viewDir );
	VectorNormalize( cross );

	CMeshBuilder meshBuilder;
	IMesh *pMesh;

	CMatRenderContextPtr pRenderContext( materials );
		
	// Better, more visible tracers
	if ( tracer_extra.GetBool() )
	{
		float flScreenWidth = ScreenWidth();
		float flHalfScreenWidth = flScreenWidth * 0.5f;
		
		float zCoord = CurrentViewForward().Dot( vecStart - CurrentViewOrigin() );
		float flScreenSpaceWidth = m_fScale * flHalfScreenWidth / zCoord;

		float flAlpha;
		float flScale;

		if ( flScreenSpaceWidth < 0.5f )
		{
			flAlpha = RemapVal( flScreenSpaceWidth, 0.25f, 2.0f, 0.3f, 1.0f );
			flAlpha = clamp( flAlpha, 0.25f, 1.0f );
			flScale = 0.5f * zCoord / flHalfScreenWidth;
		}
		else
		{
			flAlpha = 1.0f;
			flScale = m_fScale;
		}

		//Bind the material
		pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_pMaterial );

		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 2 );

		float color = (int) 255.0f * flAlpha;

		//FIXME: for now no coloration
		VectorMA( vecStart, -flScale, cross, tmp );
		meshBuilder.Position3fv( tmp.Base() );
		meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
		meshBuilder.Color4ub( color, color, color, 255 );
		meshBuilder.Normal3fv( cross.Base() );
		meshBuilder.AdvanceVertex();

		VectorMA( vecStart,  flScale, cross, tmp );
		meshBuilder.Position3fv( tmp.Base() );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.Color4ub( color, color, color, 255 );
		meshBuilder.Normal3fv( cross.Base() );
		meshBuilder.AdvanceVertex();

		VectorMA( vecEnd, flScale, cross, tmp );
		meshBuilder.Position3fv( tmp.Base() );
		meshBuilder.TexCoord2f( 0, 0.0f, fOffset );
		meshBuilder.Color4ub( color, color, color, 255 );
		meshBuilder.Normal3fv( cross.Base() );
		meshBuilder.AdvanceVertex();

		VectorMA( vecEnd, -flScale, cross, tmp );
		meshBuilder.Position3fv( tmp.Base() );
		meshBuilder.TexCoord2f( 0, 1.0f, fOffset );
		meshBuilder.Color4ub( color, color, color, 255 );
		meshBuilder.Normal3fv( cross.Base() );
		meshBuilder.AdvanceVertex();

		flScale = flScale * 2.0f;
		color = (int) 64.0f * flAlpha;

		// Soft outline
		VectorMA( vecStart, -flScale, cross, tmp );
		meshBuilder.Position3fv( tmp.Base() );
		meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
		meshBuilder.Color4ub( color, color, color, 255 );
		meshBuilder.Normal3fv( cross.Base() );
		meshBuilder.AdvanceVertex();

		VectorMA( vecStart,  flScale, cross, tmp );
		meshBuilder.Position3fv( tmp.Base() );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.Color4ub( color, color, color, 255 );
		meshBuilder.Normal3fv( cross.Base() );
		meshBuilder.AdvanceVertex();

		VectorMA( vecEnd, flScale, cross, tmp );
		meshBuilder.Position3fv( tmp.Base() );
		meshBuilder.TexCoord2f( 0, 0.0f, fOffset );
		meshBuilder.Color4ub( color, color, color, 255 );
		meshBuilder.Normal3fv( cross.Base() );
		meshBuilder.AdvanceVertex();

		VectorMA( vecEnd, -flScale, cross, tmp );
		meshBuilder.Position3fv( tmp.Base() );
		meshBuilder.TexCoord2f( 0, 1.0f, fOffset );
		meshBuilder.Color4ub( color, color, color, 255 );
		meshBuilder.Normal3fv( cross.Base() );
		meshBuilder.AdvanceVertex();
	}
	else
	{
		//Bind the material
		pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_pMaterial );

		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

		//FIXME: for now no coloration
		VectorMA( vecStart, -m_fScale, cross, tmp );
		meshBuilder.Position3fv( tmp.Base() );
		meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.Normal3fv( cross.Base() );
		meshBuilder.AdvanceVertex();

		VectorMA( vecStart,  m_fScale, cross, tmp );
		meshBuilder.Position3fv( tmp.Base() );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.Normal3fv( cross.Base() );
		meshBuilder.AdvanceVertex();

		VectorMA( vecEnd, m_fScale, cross, tmp );
		meshBuilder.Position3fv( tmp.Base() );
		meshBuilder.TexCoord2f( 0, 0.0f, fOffset );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.Normal3fv( cross.Base() );
		meshBuilder.AdvanceVertex();

		VectorMA( vecEnd, -m_fScale, cross, tmp );
		meshBuilder.Position3fv( tmp.Base() );
		meshBuilder.TexCoord2f( 0, 1.0f, fOffset );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.Normal3fv( cross.Base() );
		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();
	pMesh->Draw();
}

/*
==================================================
IsActive
==================================================
*/

bool CFXDiscreetLine::IsActive( void )
{
	return ( m_fLife > 0.0 ) ? true : false;
}

/*
==================================================
Destroy
==================================================
*/

void CFXDiscreetLine::Destroy( void )
{
	//Release the material
	if ( m_pMaterial != NULL )
	{
		m_pMaterial->DecrementReferenceCount();
		m_pMaterial = NULL;
	}
}

/*
==================================================
Update
==================================================
*/

void CFXDiscreetLine::Update( double frametime )
{
	m_fStartTime += frametime;
	m_fLife -= frametime;

	//Move our end points
	//VectorMA( m_vecStart, frametime, m_vecStartVelocity, m_vecStart );
	//VectorMA( m_vecEnd, frametime, m_vecStartVelocity, m_vecEnd );
}

