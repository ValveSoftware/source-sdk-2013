//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "view.h"
#include "iviewrender.h"
#include "c_sun.h"
#include "particles_simple.h"
#include "clienteffectprecachesystem.h"
#include "c_pixel_visibility.h"
#include "glow_overlay.h"
#include "utllinkedlist.h"
#include "view_shared.h"
#include "tier0/vprof.h"
#include "materialsystem/imaterialvar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectGlow )
CLIENTEFFECT_MATERIAL( "sun/overlay" )
CLIENTEFFECT_MATERIAL( "sprites/light_glow02_add_noz" )
CLIENTEFFECT_REGISTER_END()

class CGlowOverlaySystem : public CAutoGameSystem
{
public:
	CGlowOverlaySystem() : CAutoGameSystem( "CGlowOverlaySystem" )
	{
	}
	// Level init, shutdown
	virtual void LevelInitPreEntity() {}

	virtual void LevelShutdownPostEntity()
	{
		m_GlowOverlays.PurgeAndDeleteElements();
	}

	unsigned short AddToOverlayList( CGlowOverlay *pGlow )
	{
		return m_GlowOverlays.AddToTail( pGlow );
	}
	void RemoveFromOverlayList( unsigned short handle )
	{
		if( handle != m_GlowOverlays.InvalidIndex() )
		{
			m_GlowOverlays.Remove( handle );
		}
	}
	CUtlLinkedList<CGlowOverlay*, unsigned short> m_GlowOverlays;
};
CGlowOverlaySystem g_GlowOverlaySystem;

ConVar cl_ShowSunVectors( "cl_ShowSunVectors", "0", 0 );

ConVar cl_sun_decay_rate( "cl_sun_decay_rate", "0.05", FCVAR_CHEAT );

// Dot product space the overlays are drawn in.
// Here it's setup to allow you to see it if you're looking within 40 degrees of the source.
float g_flOverlayRange = cos( DEG2RAD( 40 ) );

// ----------------------------------------------------------------------------- //
// ----------------------------------------------------------------------------- //

void Do2DRotation( Vector vIn, Vector &vOut, float flDegrees, int i1, int i2, int i3 )
{
	float c, s;
	SinCos( DEG2RAD( flDegrees ), &s, &c );

	vOut[i1] = vIn[i1]*c - vIn[i2]*s;
	vOut[i2] = vIn[i1]*s + vIn[i2]*c;
	vOut[i3] = vIn[i3];
}


// ----------------------------------------------------------------------------- //
// ----------------------------------------------------------------------------- //

CGlowOverlay::CGlowOverlay()
{
	m_ListIndex = 0xFFFF;
	m_nSprites = 0;
	m_flGlowObstructionScale = 0.0f;
	m_bDirectional = false;
	m_bInSky = false;
	m_skyObstructionScale = 1.0f;
	m_bActivated = false;

	m_flProxyRadius = 2.0f;

	m_queryHandle = 0;

	m_flHDRColorScale = 1.0f;

	//Init our sprites
	for ( int i = 0; i < MAX_SUN_LAYERS; i++ )
	{
		m_Sprites[i].m_vColor.Init();
		m_Sprites[i].m_flHorzSize	= 1.0f;
		m_Sprites[i].m_flVertSize	= 1.0f;
		m_Sprites[i].m_pMaterial	= NULL;
	}

#ifdef PORTAL
	for( int i = 0; i != MAX_PORTAL_RECURSIVE_VIEWS; ++i )
	{
		m_skyObstructionScaleBackups[i] = 1.0f;
	}
#endif
}


CGlowOverlay::~CGlowOverlay()
{
	g_GlowOverlaySystem.RemoveFromOverlayList( m_ListIndex );
}


bool CGlowOverlay::Update()
{
	return true;
}

ConVar building_cubemaps( "building_cubemaps", "0" );

float CGlowOverlay::CalcGlowAspect()
{
	if ( m_nSprites )
	{
		if ( m_Sprites[0].m_flHorzSize != 0 && m_Sprites[0].m_flVertSize != 0 )
			return m_Sprites[0].m_flHorzSize / m_Sprites[0].m_flVertSize;
	}
	return 1.0f;
}

void CGlowOverlay::UpdateSkyGlowObstruction( float zFar, bool bCacheFullSceneState )
{
	Assert( m_bInSky );

	// If we already cached the sky obstruction and are still using that, early-out
	if ( bCacheFullSceneState && m_bCacheSkyObstruction )
		return;

	// Turning on sky obstruction caching mode
	if ( bCacheFullSceneState && !m_bCacheSkyObstruction )	
	{
		m_bCacheSkyObstruction = true;
	}

	// Turning off sky obstruction caching mode
	if ( !bCacheFullSceneState && m_bCacheSkyObstruction )
	{
		m_bCacheSkyObstruction = false;
	}

	if ( PixelVisibility_IsAvailable() )
	{
		// Trace a ray at the object. 
		Vector pos = CurrentViewOrigin() + m_vDirection * zFar * 0.999f;

		// UNDONE: Can probably do only the pixelvis query in this case if you can figure out where
		// to put it - or save the position of this trace
		pixelvis_queryparams_t params;
		params.Init( pos, m_flProxyRadius );
		params.bSizeInScreenspace = true;
		m_skyObstructionScale = PixelVisibility_FractionVisible( params, &m_queryHandle );
		return;
	}
	// Trace a ray at the object.
	trace_t trace;
	UTIL_TraceLine( CurrentViewOrigin(), CurrentViewOrigin() + (m_vDirection*MAX_TRACE_LENGTH), 
		CONTENTS_SOLID, NULL, COLLISION_GROUP_NONE, &trace );
	
	// back the trace with a pixel query to occlude with models
	if ( trace.surface.flags & SURF_SKY )
	{
		m_skyObstructionScale = 1.0f;
	}
	else
	{
		m_skyObstructionScale = 0.0f;
	}
}


void CGlowOverlay::UpdateGlowObstruction( const Vector &vToGlow, bool bCacheFullSceneState )
{
	// If we already cached the glow obstruction and are still using that, early-out
	if ( bCacheFullSceneState && m_bCacheGlowObstruction )
		return;
	
	if ( bCacheFullSceneState && !m_bCacheGlowObstruction )	// If turning on sky obstruction caching mode
	{
		m_bCacheGlowObstruction = true;
	}

	if ( !bCacheFullSceneState && m_bCacheGlowObstruction )
	{
		m_bCacheGlowObstruction = false;
	}

	if ( PixelVisibility_IsAvailable() )
	{
		if ( m_bInSky )
		{
			const CViewSetup *pViewSetup = view->GetViewSetup();
			Vector pos = CurrentViewOrigin() + m_vDirection * (pViewSetup->zFar * 0.999f);
			pixelvis_queryparams_t params;
			params.Init( pos, m_flProxyRadius, CalcGlowAspect() );
			params.bSizeInScreenspace = true;
			// use a pixel query to occlude with models
			m_flGlowObstructionScale = PixelVisibility_FractionVisible( params, &m_queryHandle ) * m_skyObstructionScale;
		}
		else
		{
			// If it's not in the sky, then we need a valid position or else we don't
			// know what's in front of it.
			Assert( !m_bDirectional );

			pixelvis_queryparams_t params;
			params.Init( m_vPos, m_flProxyRadius, CalcGlowAspect() );

			m_flGlowObstructionScale = PixelVisibility_FractionVisible( params, &m_queryHandle );
		}
		return;
	}

	bool bFade = false;
	if ( m_bInSky )
	{
		// Trace a ray at the object.
		trace_t trace;
		UTIL_TraceLine( CurrentViewOrigin(), CurrentViewOrigin() + (vToGlow*MAX_TRACE_LENGTH), 
			CONTENTS_SOLID, NULL, COLLISION_GROUP_NONE, &trace );
		
		bFade = (trace.fraction < 1 && !(trace.surface.flags & SURF_SKY));
	}
	else
	{
		// If it's not in the sky, then we need a valid position or else we don't
		// know what's in front of it.
		Assert( !m_bDirectional );

		pixelvis_queryparams_t params;
		params.Init( m_vPos, m_flProxyRadius );

		bFade = PixelVisibility_FractionVisible( params, &m_queryHandle ) < 1.0f ? true : false;

	}

	if ( bFade )
	{
		if ( building_cubemaps.GetBool() )
		{
			m_flGlowObstructionScale = 0.0f;
		}
		else
		{
			m_flGlowObstructionScale -= gpGlobals->frametime / cl_sun_decay_rate.GetFloat();
			m_flGlowObstructionScale = MAX( m_flGlowObstructionScale, 0.0f );
		}
	}
	else
	{
		if ( building_cubemaps.GetBool() )
		{
			m_flGlowObstructionScale = 1.0f;
		}
		else
		{
			m_flGlowObstructionScale += gpGlobals->frametime / cl_sun_decay_rate.GetFloat();
			m_flGlowObstructionScale = MIN( m_flGlowObstructionScale, 1.0f );
		}
	}
}

void CGlowOverlay::CalcSpriteColorAndSize( 
	float flDot,
	CGlowSprite *pSprite, 
	float *flHorzSize, 
	float *flVertSize, 
	Vector *vColor )
{
	// The overlay is largest and completely translucent at g_flOverlayRange.
	// When the dot product is 1, then it's smaller and more opaque.
	const float flSizeAtOverlayRangeMul = 150;
	const float flSizeAtOneMul = 70;
	
	const float flOpacityAtOverlayRange = 0;
	const float flOpacityAtOne = 1;

	// Figure out how big and how opaque it will be.
	*flHorzSize = RemapValClamped( 
		flDot, 
		g_flOverlayRange, 
		1, 
		flSizeAtOverlayRangeMul * pSprite->m_flHorzSize, 
		flSizeAtOneMul * pSprite->m_flHorzSize );		

	*flVertSize = RemapValClamped( 
		flDot, 
		g_flOverlayRange, 
		1, 
		flSizeAtOverlayRangeMul * pSprite->m_flVertSize, 
		flSizeAtOneMul * pSprite->m_flVertSize );		
	
	float flOpacity = RemapValClamped( 
		flDot, 
		g_flOverlayRange, 
		1, 
		flOpacityAtOverlayRange, 
		flOpacityAtOne );		

	flOpacity = flOpacity * m_flGlowObstructionScale;
	*vColor = pSprite->m_vColor * flOpacity;
}


void CGlowOverlay::CalcBasis( 
	const Vector &vToGlow,
	float flHorzSize,
	float flVertSize,
	Vector &vBasePt,
	Vector &vUp,
	Vector &vRight )
{
	const float flOverlayDist = 100;	
	vBasePt = CurrentViewOrigin() + vToGlow * flOverlayDist;
	
	vUp.Init( 0, 0, 1 );
	
	vRight = vToGlow.Cross( vUp );
	VectorNormalize( vRight );

	vUp = vRight.Cross( vToGlow );
	VectorNormalize( vUp );

	vRight *= flHorzSize;
	vUp *= flVertSize;
}


void CGlowOverlay::Draw( bool bCacheFullSceneState )
{
	extern ConVar	r_drawsprites;
	if( !r_drawsprites.GetBool() )
		return;
	
	// Get the vector to the sun.
	Vector vToGlow;
	
	if( m_bDirectional )
		vToGlow = m_vDirection;
	else
		vToGlow = m_vPos - CurrentViewOrigin();

	VectorNormalize( vToGlow );

	float flDot = vToGlow.Dot( CurrentViewForward() );

	UpdateGlowObstruction( vToGlow, bCacheFullSceneState );
	if( m_flGlowObstructionScale == 0 )
		return;
	
	bool bWireframe = ShouldDrawInWireFrameMode() || (r_drawsprites.GetInt() == 2);
	
	CMatRenderContextPtr pRenderContext( materials );

	for( int iSprite=0; iSprite < m_nSprites; iSprite++ )
	{
		CGlowSprite *pSprite = &m_Sprites[iSprite];
 
		// Figure out the color and size to draw it.
		float flHorzSize, flVertSize;
		Vector vColor;
		CalcSpriteColorAndSize( flDot, pSprite, &flHorzSize, &flVertSize, &vColor );
	
		// If we're alpha'd out, then don't bother
		if ( vColor.LengthSqr() < 0.00001f )
			continue;
		
		// Setup the basis to draw the sprite.
		Vector vBasePt, vUp, vRight;
		CalcBasis( vToGlow, flHorzSize, flVertSize, vBasePt, vUp, vRight );

		//Get our diagonal radius
		float radius = (vRight+vUp).Length();
		if ( R_CullSphere( view->GetFrustum(), 5, &vBasePt, radius ) )
			continue;

		// Get our material (deferred default load)
		if ( m_Sprites[iSprite].m_pMaterial == NULL )
		{
			m_Sprites[iSprite].m_pMaterial = materials->FindMaterial( "sprites/light_glow02_add_noz", TEXTURE_GROUP_CLIENT_EFFECTS );
		}

		Assert( m_Sprites[iSprite].m_pMaterial );
		static unsigned int		nHDRColorScaleCache = 0;
		IMaterialVar *pHDRColorScaleVar = m_Sprites[iSprite].m_pMaterial->FindVarFast( "$hdrcolorscale", &nHDRColorScaleCache );
		if( pHDRColorScaleVar )
		{
			pHDRColorScaleVar->SetFloatValue( m_flHDRColorScale );
		}

		// Draw the sprite.
		IMesh *pMesh = pRenderContext->GetDynamicMesh( false, 0, 0, m_Sprites[iSprite].m_pMaterial );

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

		if( bWireframe )
		{
			IMaterial *pWireframeMaterial = materials->FindMaterial( "debug/debugwireframevertexcolor", TEXTURE_GROUP_OTHER );
			pRenderContext->Bind( pWireframeMaterial );
			
			// Draw the sprite.
			pMesh = pRenderContext->GetDynamicMesh( false, 0, 0, pWireframeMaterial );
			
			CMeshBuilder builderWireFrame;
			builderWireFrame.Begin( pMesh, MATERIAL_QUADS, 1 );
						
			vPt = vBasePt - vRight + vUp;
			builderWireFrame.Position3fv( vPt.Base() );
			builderWireFrame.Color3f( 1.0f, 0.0f, 0.0f );
			builderWireFrame.AdvanceVertex();
			
			vPt = vBasePt + vRight + vUp;
			builderWireFrame.Position3fv( vPt.Base() );
			builderWireFrame.Color3f( 1.0f, 0.0f, 0.0f );
			builderWireFrame.AdvanceVertex();
			
			vPt = vBasePt + vRight - vUp;
			builderWireFrame.Position3fv( vPt.Base() );
			builderWireFrame.Color3f( 1.0f, 0.0f, 0.0f );
			builderWireFrame.AdvanceVertex();
			
			vPt = vBasePt - vRight - vUp;
			builderWireFrame.Position3fv( vPt.Base() );
			builderWireFrame.Color3f( 1.0f, 0.0f, 0.0f );
			builderWireFrame.AdvanceVertex();
			
			builderWireFrame.End( false, true );
		}
	}
}


void CGlowOverlay::Activate()
{
	m_bActivated = true;
	if( m_ListIndex == 0xFFFF )
	{
		m_ListIndex = g_GlowOverlaySystem.AddToOverlayList( this );
	}
}


void CGlowOverlay::Deactivate()
{
	m_bActivated = false;
}


void CGlowOverlay::DrawOverlays( bool bCacheFullSceneState )
{
	VPROF("CGlowOverlay::DrawOverlays()");

	CMatRenderContextPtr pRenderContext( materials );

	bool bClippingEnabled = pRenderContext->EnableClipping( true );

	unsigned short iNext;
	for( unsigned short i=g_GlowOverlaySystem.m_GlowOverlays.Head(); i != g_GlowOverlaySystem.m_GlowOverlays.InvalidIndex(); i = iNext )
	{
		iNext = g_GlowOverlaySystem.m_GlowOverlays.Next( i );
		CGlowOverlay *pOverlay = g_GlowOverlaySystem.m_GlowOverlays[i];
		
		if( !pOverlay->m_bActivated )
			continue;

		if( pOverlay->Update() )
		{
			pRenderContext->EnableClipping( ((pOverlay->m_bInSky) ? (false):(bClippingEnabled)) ); //disable clipping in skybox, restore clipping to pre-existing state when not in skybox (it may be off as well)
			pOverlay->Draw( bCacheFullSceneState );
		}
		else
		{
			delete pOverlay;
		}
	}

	pRenderContext->EnableClipping( bClippingEnabled ); //restore clipping to original state
}

void CGlowOverlay::UpdateSkyOverlays( float zFar, bool bCacheFullSceneState )
{
	unsigned short iNext;
	for( unsigned short i=g_GlowOverlaySystem.m_GlowOverlays.Head(); i != g_GlowOverlaySystem.m_GlowOverlays.InvalidIndex(); i = iNext )
	{
		iNext = g_GlowOverlaySystem.m_GlowOverlays.Next( i );
		CGlowOverlay *pOverlay = g_GlowOverlaySystem.m_GlowOverlays[i];
		
		if( !pOverlay->m_bActivated || !pOverlay->m_bDirectional || !pOverlay->m_bInSky )
			continue;

		pOverlay->UpdateSkyGlowObstruction( zFar, bCacheFullSceneState );
	}
}




#ifdef PORTAL

void CGlowOverlay::BackupSkyOverlayData( int iBackupToSlot )
{
	unsigned short iNext;
	for( unsigned short i=g_GlowOverlaySystem.m_GlowOverlays.Head(); i != g_GlowOverlaySystem.m_GlowOverlays.InvalidIndex(); i = iNext )
	{
		iNext = g_GlowOverlaySystem.m_GlowOverlays.Next( i );
		CGlowOverlay *pOverlay = g_GlowOverlaySystem.m_GlowOverlays[i];

		if( !pOverlay->m_bActivated || !pOverlay->m_bDirectional || !pOverlay->m_bInSky )
			continue;

		pOverlay->m_skyObstructionScaleBackups[iBackupToSlot] = pOverlay->m_skyObstructionScale;
	}
}

void CGlowOverlay::RestoreSkyOverlayData( int iRestoreFromSlot )
{
	unsigned short iNext;
	for( unsigned short i=g_GlowOverlaySystem.m_GlowOverlays.Head(); i != g_GlowOverlaySystem.m_GlowOverlays.InvalidIndex(); i = iNext )
	{
		iNext = g_GlowOverlaySystem.m_GlowOverlays.Next( i );
		CGlowOverlay *pOverlay = g_GlowOverlaySystem.m_GlowOverlays[i];

		if( !pOverlay->m_bActivated || !pOverlay->m_bDirectional || !pOverlay->m_bInSky )
			continue;

		pOverlay->m_skyObstructionScale = pOverlay->m_skyObstructionScaleBackups[iRestoreFromSlot];
	}
}

#endif //#ifdef PORTAL

