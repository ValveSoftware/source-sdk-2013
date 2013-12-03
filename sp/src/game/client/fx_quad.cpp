//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "view.h"
#include "materialsystem/imesh.h"
#include "fx_quad.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CFXQuad::CFXQuad( const FXQuadData_t &data )

: CClientSideEffect( "Quad" )
{
	m_FXData = data;
}

CFXQuad::~CFXQuad( void )
{
	Destroy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : frametime - 
//-----------------------------------------------------------------------------
void CFXQuad::Draw( double frametime )
{
	VPROF_BUDGET( "FX_Quad::Draw", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	// Update the effect
	Update( frametime );

	float	scaleTimePerc, alphaTimePerc;

	//Determine the scale
	if ( m_FXData.m_uiFlags & FXQUAD_BIAS_SCALE )
	{
		scaleTimePerc = Bias( ( m_FXData.m_flLifeTime / m_FXData.m_flDieTime ), m_FXData.m_flScaleBias );
	}
	else
	{
		scaleTimePerc = ( m_FXData.m_flLifeTime / m_FXData.m_flDieTime );
	}

	float scale = m_FXData.m_flStartScale + ( ( m_FXData.m_flEndScale - m_FXData.m_flStartScale ) * scaleTimePerc );

	//Determine the alpha
	if ( m_FXData.m_uiFlags & FXQUAD_BIAS_ALPHA )
	{
		alphaTimePerc = Bias( ( m_FXData.m_flLifeTime / m_FXData.m_flDieTime ), m_FXData.m_flAlphaBias );
	}
	else
	{
		alphaTimePerc = ( m_FXData.m_flLifeTime / m_FXData.m_flDieTime );
	}

	float alpha = m_FXData.m_flStartAlpha + ( ( m_FXData.m_flEndAlpha - m_FXData.m_flStartAlpha ) * alphaTimePerc );
	alpha = clamp( alpha, 0.0f, 1.0f );
	
	CMatRenderContextPtr pRenderContext( materials );

	//Bind the material
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_FXData.m_pMaterial );
	CMeshBuilder meshBuilder;

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	//Update our roll
	m_FXData.m_flYaw = anglemod( m_FXData.m_flYaw + ( m_FXData.m_flDeltaYaw * frametime ) );

	Vector	pos;
	Vector	vRight, vUp;

	float color[4];

	color[0] = m_FXData.m_Color[0];
	color[1] = m_FXData.m_Color[1];
	color[2] = m_FXData.m_Color[2];

	if ( m_FXData.m_uiFlags & FXQUAD_COLOR_FADE )
	{
		color[0] *= alpha;
		color[1] *= alpha;
		color[2] *= alpha;
	}

	color[3] = alpha;

	VectorVectors( m_FXData.m_vecNormal, vRight, vUp );

	Vector	rRight, rUp;

	rRight	= ( vRight * cos( DEG2RAD( m_FXData.m_flYaw ) ) ) - ( vUp * sin( DEG2RAD( m_FXData.m_flYaw ) ) );
	rUp		= ( vRight * cos( DEG2RAD( m_FXData.m_flYaw+90.0f ) ) ) - ( vUp * sin( DEG2RAD( m_FXData.m_flYaw+90.0f ) ) );

	vRight	= rRight * ( scale * 0.5f );
	vUp		= rUp * ( scale * 0.5f );

	pos = m_FXData.m_vecOrigin + vRight - vUp;

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( m_FXData.m_vecNormal.Base() );
	meshBuilder.TexCoord2f( 0, 1.0f, 1.0f );
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	pos = m_FXData.m_vecOrigin - vRight - vUp;

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( m_FXData.m_vecNormal.Base() );
	meshBuilder.TexCoord2f( 0, 0.0f, 1.0f );
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	pos = m_FXData.m_vecOrigin - vRight + vUp;

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( m_FXData.m_vecNormal.Base() );
	meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	pos = m_FXData.m_vecOrigin + vRight + vUp;

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( m_FXData.m_vecNormal.Base() );
	meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFXQuad::IsActive( void )
{
	return ( m_FXData.m_flLifeTime < m_FXData.m_flDieTime ) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFXQuad::Destroy( void )
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
void CFXQuad::Update( double frametime )
{
	m_FXData.m_flLifeTime += frametime;
}
