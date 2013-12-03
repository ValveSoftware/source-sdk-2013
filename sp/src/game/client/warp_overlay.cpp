//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Screen warp overlay
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "view.h"
#include "c_sun.h"
#include "particles_simple.h"
#include "clienteffectprecachesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectWarp )
CLIENTEFFECT_MATERIAL( "sun/overlay" )
CLIENTEFFECT_REGISTER_END()

//-----------------------------------------------------------------------------
// Purpose: Special draw for the warped overlay
//-----------------------------------------------------------------------------
void CWarpOverlay::Draw( bool bCacheFullSceneState )
{
	// Get the vector to the sun.
	Vector vToGlow;
	
	if( m_bDirectional )
		vToGlow = m_vDirection;
	else
		vToGlow = m_vPos - CurrentViewOrigin();

	VectorNormalize( vToGlow );

	float flDot = vToGlow.Dot( CurrentViewForward() );

	if( flDot <= g_flOverlayRange )
		return;

	UpdateGlowObstruction( vToGlow, bCacheFullSceneState );
	if( m_flGlowObstructionScale == 0 )
		return;
	
	CMatRenderContextPtr pRenderContext( materials );
	
	//FIXME: Allow multiple?
	for( int iSprite=0; iSprite < m_nSprites; iSprite++ )
	{
		CGlowSprite *pSprite = &m_Sprites[iSprite];

		// Figure out the color and size to draw it.
		float flHorzSize, flVertSize;
		Vector vColor;
		CalcSpriteColorAndSize( flDot, pSprite, &flHorzSize, &flVertSize, &vColor );
	
		// Setup the basis to draw the sprite.
		Vector vBasePt, vUp, vRight;
		CalcBasis( vToGlow, flHorzSize, flVertSize, vBasePt, vUp, vRight );

		// Draw the sprite.
		IMaterial *pMaterial = materials->FindMaterial( "sun/overlay", TEXTURE_GROUP_CLIENT_EFFECTS );
		IMesh *pMesh = pRenderContext->GetDynamicMesh( false, 0, 0, pMaterial );

		CMeshBuilder builder;
		builder.Begin( pMesh, MATERIAL_QUADS, 1 );
		
		Vector vPt;

		vPt = vBasePt - vRight + vUp;
		builder.Position3fv( vPt.Base() );
		builder.Color4f( VectorExpand(vColor), 1 );
		builder.TexCoord2f( 0, 0, 1 );
		builder.AdvanceVertex();

		vPt = vBasePt + vRight + vUp;
		builder.Position3fv( vPt.Base() );
		builder.Color4f( VectorExpand(vColor), 1 );
		builder.TexCoord2f( 0, 1, 1 );
		builder.AdvanceVertex();

		vPt = vBasePt + vRight - vUp;
		builder.Position3fv( vPt.Base() );
		builder.Color4f( VectorExpand(vColor), 1 );
		builder.TexCoord2f( 0, 1, 0 );
		builder.AdvanceVertex();

		vPt = vBasePt - vRight - vUp;
		builder.Position3fv( vPt.Base() );
		builder.Color4f( VectorExpand(vColor), 1 );
		builder.TexCoord2f( 0, 0, 0 );
		builder.AdvanceVertex();
		
		builder.End( false, true );
	}
}
