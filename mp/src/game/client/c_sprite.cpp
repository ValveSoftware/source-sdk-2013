//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "c_sprite.h"
#include "model_types.h"
#include "iviewrender.h"
#include "view.h"
#include "enginesprite.h"
#include "engine/ivmodelinfo.h"
#include "util_shared.h"
#include "tier0/vprof.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "view_shared.h"
#include "viewrender.h"
#include "tier1/KeyValues.h"
#include "toolframework/itoolframework.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	r_drawsprites( "r_drawsprites", "1", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: Generic sprite model renderer
// Input  : *baseentity - 
//			*psprite - 
//			fscale - 
//			frame - 
//			rendermode - 
//			r - 
//			g - 
//			b - 
//			a - 
//			forward - 
//			right - 
//			up - 
//-----------------------------------------------------------------------------
static unsigned int s_nHDRColorScaleCache = 0;
void DrawSpriteModel( IClientEntity *baseentity, CEngineSprite *psprite, const Vector &origin, float fscale, float frame, 
	int rendermode, int r, int g, int b, int a, const Vector& forward, const Vector& right, const Vector& up, float flHDRColorScale )
{
	float		scale;
	IMaterial	*material;
	
	// don't even bother culling, because it's just a single
	// polygon without a surface cache
	if ( fscale > 0 )
		scale = fscale;
	else
		scale = 1.0f;
	
	if ( rendermode == kRenderNormal )
	{
		render->SetBlend( 1.0f );
	}
	
	material = psprite->GetMaterial( (RenderMode_t)rendermode, frame );
	if ( !material )
		return;

	CMatRenderContextPtr pRenderContext( materials );
	
	if ( ShouldDrawInWireFrameMode() || r_drawsprites.GetInt() == 2 )
	{
		IMaterial *pMaterial = materials->FindMaterial( "debug/debugspritewireframe", TEXTURE_GROUP_OTHER );
		pRenderContext->Bind( pMaterial, NULL );
	}
	else
	{
		pRenderContext->Bind( material, (IClientRenderable*)baseentity );
	}

	unsigned char color[4];
	color[0] = r;
	color[1] = g;
	color[2] = b;
	color[3] = a;

	IMaterialVar *pHDRColorScaleVar = material->FindVarFast( "$HDRCOLORSCALE", &s_nHDRColorScaleCache );
	if( pHDRColorScaleVar )
	{
		pHDRColorScaleVar->SetVecValue( flHDRColorScale, flHDRColorScale, flHDRColorScale );
	}

	Vector point;
	IMesh* pMesh = pRenderContext->GetDynamicMesh();

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	Vector vec_a;
	Vector vec_b;
	Vector vec_c;
	Vector vec_d;

	// isolate common terms
	VectorMA( origin, psprite->GetDown() * scale, up, vec_a );
	VectorScale( right, psprite->GetLeft() * scale, vec_b );
	VectorMA( origin, psprite->GetUp() * scale, up, vec_c );
	VectorScale( right, psprite->GetRight() * scale, vec_d );

	float flMinU, flMinV, flMaxU, flMaxV;
	psprite->GetTexCoordRange( &flMinU, &flMinV, &flMaxU, &flMaxV );

	meshBuilder.Color4ubv( color );
	meshBuilder.TexCoord2f( 0, flMinU, flMaxV );
	VectorAdd( vec_a, vec_b, point );
	meshBuilder.Position3fv( point.Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( color );
	meshBuilder.TexCoord2f( 0, flMinU, flMinV );
	VectorAdd( vec_c, vec_b, point );
	meshBuilder.Position3fv( point.Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( color );
	meshBuilder.TexCoord2f( 0, flMaxU, flMinV );
	VectorAdd( vec_c, vec_d, point );
	meshBuilder.Position3fv( point.Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( color );
	meshBuilder.TexCoord2f( 0, flMaxU, flMaxV );
	VectorAdd( vec_a, vec_d, point );
	meshBuilder.Position3fv( point.Base() );
	meshBuilder.AdvanceVertex();
	
	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: Determine glow brightness/scale based on distance to render origin and trace results
// Input  : entorigin - 
//			rendermode - 
//			renderfx - 
//			alpha - 
//			pscale - Pointer to the value for scale, will be changed based on distance and rendermode.
//-----------------------------------------------------------------------------
float StandardGlowBlend( const pixelvis_queryparams_t &params, pixelvis_handle_t *queryHandle, int rendermode, int renderfx, int alpha, float *pscale )
{
	float dist;
	float brightness;

	brightness = PixelVisibility_FractionVisible( params, queryHandle );
	if ( brightness <= 0.0f )
	{
		return 0.0f;
	}
	dist = GlowSightDistance( params.position, false );
	if ( dist <= 0.0f )
	{
		return 0.0f;
	}

	if ( renderfx == kRenderFxNoDissipation )
	{
		return (float)alpha * (1.0f/255.0f) * brightness;
	}

	// UNDONE: Tweak these magic numbers (1200 - distance at full brightness)
	float fadeOut = (1200.0f*1200.0f) / (dist*dist);
	fadeOut = clamp( fadeOut, 0.0f, 1.0f );

	if (rendermode != kRenderWorldGlow)
	{
		// Make the glow fixed size in screen space, taking into consideration the scale setting.
		if ( *pscale == 0.0f )
		{
			*pscale = 1.0f;
		}

		*pscale *= dist * (1.0f/200.0f);
	}

	return fadeOut * brightness;
}

static float SpriteAspect( CEngineSprite *pSprite )
{
	if ( pSprite )
	{
		float x = fabsf(pSprite->GetRight() - pSprite->GetLeft());
		float y = fabsf(pSprite->GetDown() - pSprite->GetUp());
		if ( y != 0 && x != 0 )
		{
			return x / y;
		}
	}

	return 1.0f;
}

float C_SpriteRenderer::GlowBlend( CEngineSprite *psprite, const Vector& entorigin, int rendermode, int renderfx, int alpha, float *pscale )
{
	pixelvis_queryparams_t params;
	float aspect = SpriteAspect(psprite);
	params.Init( entorigin, PIXELVIS_DEFAULT_PROXY_SIZE, aspect );
	return StandardGlowBlend( params, &m_queryHandle, rendermode, renderfx, alpha, pscale );
}

// since sprites can network down a glow proxy size, handle that here
float CSprite::GlowBlend( CEngineSprite *psprite, const Vector& entorigin, int rendermode, int renderfx, int alpha, float *pscale )
{
	pixelvis_queryparams_t params;
	float aspect = SpriteAspect(psprite);
	params.Init( entorigin, m_flGlowProxySize, aspect );
	return StandardGlowBlend( params, &m_queryHandle, rendermode, renderfx, alpha, pscale );
}


//-----------------------------------------------------------------------------
// Purpose: Determine sprite orientation axes
// Input  : type - 
//			forward - 
//			right - 
//			up - 
//-----------------------------------------------------------------------------
void C_SpriteRenderer::GetSpriteAxes( SPRITETYPE type, 
	const Vector& origin,
	const QAngle& angles,
	Vector& forward, 
	Vector& right, 
	Vector& up )
{
	int				i;
	float			dot, angle, sr, cr;
	Vector			tvec;

	// Automatically roll parallel sprites if requested
	if ( angles[2] != 0 && type == SPR_VP_PARALLEL )
	{
		type = SPR_VP_PARALLEL_ORIENTED;
	}

	switch( type )
	{
	case SPR_FACING_UPRIGHT:
		{
			// generate the sprite's axes, with vup straight up in worldspace, and
			// r_spritedesc.vright perpendicular to modelorg.
			// This will not work if the view direction is very close to straight up or
			// down, because the cross product will be between two nearly parallel
			// vectors and starts to approach an undefined state, so we don't draw if
			// the two vectors are less than 1 degree apart
			tvec[0] = -origin[0];
			tvec[1] = -origin[1];
			tvec[2] = -origin[2];
			VectorNormalize (tvec);
			dot = tvec[2];	// same as DotProduct (tvec, r_spritedesc.vup) because
			//  r_spritedesc.vup is 0, 0, 1
			if ((dot > 0.999848f) || (dot < -0.999848f))	// cos(1 degree) = 0.999848
				return;
			up[0] = 0;
			up[1] = 0;
			up[2] = 1;
			right[0] = tvec[1];
			// CrossProduct(r_spritedesc.vup, -modelorg,
			right[1] = -tvec[0];
			//              r_spritedesc.vright)
			right[2] = 0;
			VectorNormalize (right);
			forward[0] = -right[1];
			forward[1] = right[0];
			forward[2] = 0;
			// CrossProduct (r_spritedesc.vright, r_spritedesc.vup,
			//  r_spritedesc.vpn)
		}
		break;

	case SPR_VP_PARALLEL:
		{
			// generate the sprite's axes, completely parallel to the viewplane. There
			// are no problem situations, because the sprite is always in the same
			// position relative to the viewer
			for (i=0 ; i<3 ; i++)
			{
				up[i]		= CurrentViewUp()[i];
				right[i]	= CurrentViewRight()[i];
				forward[i]	= CurrentViewForward()[i];
			}
		}
		break;

	case SPR_VP_PARALLEL_UPRIGHT:
		{
			// generate the sprite's axes, with g_vecVUp straight up in worldspace, and
			// r_spritedesc.vright parallel to the viewplane.
			// This will not work if the view direction is very close to straight up or
			// down, because the cross product will be between two nearly parallel
			// vectors and starts to approach an undefined state, so we don't draw if
			// the two vectors are less than 1 degree apart
			dot = CurrentViewForward()[2];	// same as DotProduct (vpn, r_spritedesc.g_vecVUp) because
			//  r_spritedesc.vup is 0, 0, 1
			if ((dot > 0.999848f) || (dot < -0.999848f))	// cos(1 degree) = 0.999848
				return;
			up[0] = 0;
			up[1] = 0;
			up[2] = 1;
			right[0] = CurrentViewForward()[1];
			// CrossProduct (r_spritedesc.vup, vpn,
			right[1] = -CurrentViewForward()[0];	//  r_spritedesc.vright)
			right[2] = 0;
			VectorNormalize (right);
			forward[0] = -right[1];
			forward[1] = right[0];
			forward[2] = 0;
			// CrossProduct (r_spritedesc.vright, r_spritedesc.vup,
			//  r_spritedesc.vpn)
		}
		break;

	case SPR_ORIENTED:
		{
			// generate the sprite's axes, according to the sprite's world orientation
			AngleVectors( angles, &forward, &right, &up );
		}
		break;
		
	case SPR_VP_PARALLEL_ORIENTED:
		{
			// generate the sprite's axes, parallel to the viewplane, but rotated in
			// that plane around the center according to the sprite entity's roll
			// angle. So vpn stays the same, but vright and vup rotate
			angle = angles[ROLL] * (M_PI*2.0f/360.0f);
			SinCos( angle, &sr, &cr );
			
			for (i=0 ; i<3 ; i++)
			{
				forward[i] = CurrentViewForward()[i];
				right[i] = CurrentViewRight()[i] * cr + CurrentViewUp()[i] * sr;
				up[i] = CurrentViewRight()[i] * -sr + CurrentViewUp()[i] * cr;
			}
		}
		break;

	default:
		Warning( "GetSpriteAxes: Bad sprite type %d\n", type );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int C_SpriteRenderer::DrawSprite( 
	IClientEntity *entity,
	const model_t *model, 
	const Vector& origin, 
	const QAngle& angles,
	float frame,
	IClientEntity *attachedto,
	int attachmentindex,
	int rendermode,
	int renderfx,
	int alpha,
	int r, 
	int g, 
	int b,
	float scale,
	float flHDRColorScale
	)
{
	VPROF_BUDGET( "C_SpriteRenderer::DrawSprite", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	if ( !r_drawsprites.GetBool() || !model || modelinfo->GetModelType( model ) != mod_sprite )
	{
		return 0;
	}

	// Get extra data
	CEngineSprite *psprite = (CEngineSprite *)modelinfo->GetModelExtraData( model );
	if ( !psprite )
	{
		return 0;
	}

	Vector effect_origin;
	VectorCopy( origin, effect_origin );

	// Use attachment point
	if ( attachedto )
	{
		C_BaseEntity *ent = attachedto->GetBaseEntity();
		if ( ent )
		{
			// don't draw viewmodel effects in reflections
			if ( CurrentViewID() == VIEW_REFLECTION )
			{
				int group = ent->GetRenderGroup();
				if ( group == RENDER_GROUP_VIEW_MODEL_TRANSLUCENT || group == RENDER_GROUP_VIEW_MODEL_OPAQUE )
					return 0;
			}
			QAngle temp;
			ent->GetAttachment( attachmentindex, effect_origin, temp );
		}
	}

	if ( rendermode != kRenderNormal )
	{
		float blend = render->GetBlend();

		// kRenderGlow and kRenderWorldGlow have a special blending function
		if (( rendermode == kRenderGlow ) || ( rendermode == kRenderWorldGlow ))
		{
			blend *= GlowBlend( psprite, effect_origin, rendermode, renderfx, alpha, &scale );

			// Fade out the sprite depending on distance from the view origin.
			r *= blend;
			g *= blend;
			b *= blend;
		}

		render->SetBlend( blend );
		if ( blend <= 0.0f )
		{
			return 0;
		}
	}
	
	// Get orthonormal basis
	Vector forward, right, up;
	GetSpriteAxes( (SPRITETYPE)psprite->GetOrientation(), origin, angles, forward, right, up );

	// Draw
	DrawSpriteModel( 
		entity,
		psprite, 
		effect_origin,
		scale,
		frame, 
		rendermode, 
		r, 
		g, 
		b,
		alpha, 
		forward, right, up, flHDRColorScale );

	return 1;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSprite::GetToolRecordingState( KeyValues *msg )
{
	if ( !ToolsEnabled() )
		return;

	VPROF_BUDGET( "CSprite::GetToolRecordingState", VPROF_BUDGETGROUP_TOOLS );

	BaseClass::GetToolRecordingState( msg );

	// Use attachment point
	if ( m_hAttachedToEntity )
	{
		C_BaseEntity *ent = m_hAttachedToEntity->GetBaseEntity();
		if ( ent )
		{
			BaseEntityRecordingState_t *pState = (BaseEntityRecordingState_t*)msg->GetPtr( "baseentity" );

			// override position if we're driven by an attachment
			QAngle temp;
			pState->m_vecRenderOrigin = GetAbsOrigin();
			ent->GetAttachment( m_nAttachment, pState->m_vecRenderOrigin, temp );

			// override viewmodel if we're driven by an attachment
			bool bViewModel = dynamic_cast< C_BaseViewModel* >( ent ) != NULL;
			msg->SetInt( "viewmodel", bViewModel );
		}
	}

	float renderscale = GetRenderScale();
	if ( m_bWorldSpaceScale )
	{
		CEngineSprite *psprite = ( CEngineSprite * )modelinfo->GetModelExtraData( GetModel() );
		float flMinSize = MIN( psprite->GetWidth(), psprite->GetHeight() );
		renderscale /= flMinSize;
	}

	// sprite params
	static SpriteRecordingState_t state;
	state.m_flRenderScale = renderscale;
	state.m_flFrame = m_flFrame;
	state.m_flProxyRadius = m_flGlowProxySize;
	state.m_nRenderMode = GetRenderMode();
	state.m_nRenderFX = m_nRenderFX;
	state.m_Color.SetColor( m_clrRender.GetR(), m_clrRender.GetG(), m_clrRender.GetB(), GetRenderBrightness() );

	msg->SetPtr( "sprite", &state );
}
