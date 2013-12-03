//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_ai_basenpc.h"
#include "c_te_particlesystem.h"
#include "fx.h"
#include "fx_sparks.h"
#include "c_tracer.h"
#include "clientsideeffects.h"
#include "iefx.h"
#include "dlight.h"
#include "bone_setup.h"
#include "c_rope.h"
#include "fx_line.h"
#include "c_sprite.h"
#include "view.h"
#include "view_scene.h"
#include "materialsystem/imaterialvar.h"
#include "simple_keys.h"
#include "fx_envelope.h"
#include "iclientvehicle.h"
#include "engine/ivdebugoverlay.h"
#include "particles_localspace.h"
#include "dlight.h"
#include "iefx.h"
#include "c_te_effect_dispatch.h"
#include "tier0/vprof.h"
#include "clienteffectprecachesystem.h"
#include <bitbuf.h>
#include "fx_water.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define STRIDER_MSG_BIG_SHOT			1
#define STRIDER_MSG_STREAKS				2
#define STRIDER_MSG_DEAD				3

#define STOMP_IK_SLOT					11
const int NUM_STRIDER_IK_TARGETS = 6;

const float	STRIDERFX_BIG_SHOT_TIME = 1.25f;
const float STRIDERFX_END_ALL_TIME = 4.0f;

class C_StriderFX : public C_EnvelopeFX
{
public:
	typedef C_EnvelopeFX	BaseClass;

	C_StriderFX();
	~C_StriderFX()
	{
		EffectShutdown();
	}


	void			Update( C_BaseEntity *pOwner, const Vector &targetPos );

	// Returns the bounds relative to the origin (render bounds)
	virtual void	GetRenderBounds( Vector& mins, Vector& maxs )
	{
		ClearBounds( mins, maxs );
		AddPointToBounds( m_worldPosition, mins, maxs );
		AddPointToBounds( m_targetPosition, mins, maxs );
		mins -= GetRenderOrigin();
		maxs -= GetRenderOrigin();
	}

	virtual void EffectInit( int entityIndex, int attachment )
	{
		m_limitHitTime = 0;
		BaseClass::EffectInit( entityIndex, attachment );
	}
	virtual void EffectShutdown( void )
	{
		m_limitHitTime = 0;
		BaseClass::EffectShutdown();
	}

	virtual int	DrawModel( int flags );
	virtual void LimitTime( float tmax ) 
	{ 
		float dt = tmax - m_t;
		if ( dt < 0 )
		{
			dt = 0;
		}
		m_limitHitTime = gpGlobals->curtime + dt;
		BaseClass::LimitTime( tmax );
	}

	C_BaseEntity			*m_pOwner;
	Vector					m_targetPosition;
	Vector					m_beamEndPosition;
	pixelvis_handle_t		m_queryHandleGun;
	pixelvis_handle_t		m_queryHandleBeamEnd;
	float					m_limitHitTime;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_Strider : public C_AI_BaseNPC
{
	DECLARE_CLASS( C_Strider, C_AI_BaseNPC );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();


					C_Strider();
	virtual			~C_Strider();

	// model specific
	virtual void	ReceiveMessage( int classID, bf_read &msg );
	virtual void	CalculateIKLocks( float currentTime )
	{
		// NOTE: All strider IK is solved on the server, enable this to do it client-side
		//BaseClass::CalculateIKLocks( currentTime );
		if ( m_pIk && m_pIk->m_target.Count() )
		{
			Assert(m_pIk->m_target.Count() > STOMP_IK_SLOT);
			// HACKHACK: Hardcoded 11???  Not a cleaner way to do this
			CIKTarget &target = m_pIk->m_target[STOMP_IK_SLOT];
			target.SetPos( m_vecHitPos );
			// target.latched.pos = m_vecHitPos;

			for ( int i = 0; i < NUM_STRIDER_IK_TARGETS; i++ )
			{
				CIKTarget &target = m_pIk->m_target[i];
				target.SetPos( m_vecIKTarget[i] );
#if 0
				debugoverlay->AddBoxOverlay( m_vecIKTarget[i], Vector( -2, -2, -2 ), Vector( 2, 2, 2), QAngle( 0, 0, 0 ), (int)255*m_pIk->m_target[i].est.latched, 0, 0, 0, 0 );
#endif
			}
		}
	}

	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void GetRenderBounds( Vector& theMins, Vector& theMaxs );
	virtual void ClientThink();

private:
	C_Strider( const C_Strider & );
	C_StriderFX	m_cannonFX;
	Vector		m_vecHitPos;
	Vector		m_vecIKTarget[NUM_STRIDER_IK_TARGETS];
	CInterpolatedVar< Vector >		m_iv_vecHitPos;
	CInterpolatedVarArray< Vector, NUM_STRIDER_IK_TARGETS >	m_iv_vecIKTarget;
	Vector		m_vecRenderMins;
	Vector		m_vecRenderMaxs;

	float m_flNextRopeCutTime;
};

IMPLEMENT_CLIENTCLASS_DT(C_Strider, DT_NPC_Strider, CNPC_Strider)
	RecvPropVector(RECVINFO(m_vecHitPos)),
	RecvPropVector(RECVINFO(m_vecIKTarget[0])),
	RecvPropVector(RECVINFO(m_vecIKTarget[1])),
	RecvPropVector(RECVINFO(m_vecIKTarget[2])),
	RecvPropVector(RECVINFO(m_vecIKTarget[3])),
	RecvPropVector(RECVINFO(m_vecIKTarget[4])),
	RecvPropVector(RECVINFO(m_vecIKTarget[5])),
END_RECV_TABLE()

C_StriderFX::C_StriderFX()
{
	m_pOwner = NULL;
	m_active = false;
}

void C_StriderFX::Update( C_BaseEntity *pOwner, const Vector &targetPos )
{
	BaseClass::Update();

	m_pOwner = pOwner;
	
	if ( m_active )
	{
		m_targetPosition = targetPos;
	}
}

// --on gun
// warpy sprite bit
// darkening sprite
// glowy blue flare sprite
// bubble warpy sprite
// after glow sprite

// --on line of sight
// narrow beam
// wide beam

// --on impact point
// sparkly white bits
// sparkly white streaks
// pale blue particle steam

enum 
{
	STRIDERFX_WARP_SCALE = 0,
	STRIDERFX_DARKNESS,
	STRIDERFX_FLARE_COLOR,
	STRIDERFX_FLARE_SIZE,
	STRIDERFX_BUBBLE_SIZE,
	STRIDERFX_BUBBLE_REFRACT,

	STRIDERFX_NARROW_BEAM_COLOR,
	STRIDERFX_NARROW_BEAM_SIZE,
	
	STRIDERFX_WIDE_BEAM_COLOR,
	STRIDERFX_WIDE_BEAM_SIZE,

	STRIDERFX_AFTERGLOW_COLOR,

	STRIDERFX_WIDE_BEAM_LENGTH,

	STRIDERFX_SPARK_COUNT,
	STRIDERFX_STREAK_COUNT,
	STRIDERFX_STEAM_COUNT,


	// must be last
	STRIDERFX_PARAMETERS,
};

class CStriderFXEnvelope
{
public:
	CStriderFXEnvelope();

	void AddKey( int parameterIndex, const CSimpleKeyInterp &key )
	{
		Assert( parameterIndex >= 0 && parameterIndex < STRIDERFX_PARAMETERS );

		if ( parameterIndex >= 0 && parameterIndex < STRIDERFX_PARAMETERS )
		{
			m_parameters[parameterIndex].Insert( key );
		}

	}

	CSimpleKeyList		m_parameters[STRIDERFX_PARAMETERS];
};

// NOTE: Beam widths are half-widths or radii, so this is a beam that represents a cylinder with 2" radius
const float NARROW_BEAM_WIDTH = 2;
const float WIDE_BEAM_WIDTH = 16;
const float FLARE_SIZE = 128;
const float	DARK_SIZE = 64;
const float AFTERGLOW_SIZE = 64;

const float WARP_SIZE = 512;
const float WARP_REFRACT = 0.075f;
const float WARP_BUBBLE_SIZE = 256;
const float WARP_BUBBLE_REFRACT = 1.0f;

CStriderFXEnvelope::CStriderFXEnvelope()
{
	AddKey( STRIDERFX_WARP_SCALE, CSimpleKeyInterp( 0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_WARP_SCALE, CSimpleKeyInterp( 1.25, KEY_ACCELERATE, 1 ) );
	AddKey( STRIDERFX_WARP_SCALE, CSimpleKeyInterp( 1.25, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_WARP_SCALE, CSimpleKeyInterp( 1.3, KEY_LINEAR, 0 ) );

	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 0.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 0.5, KEY_SPLINE, 1 ) );
	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 1.0, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 1.25, KEY_SPLINE, 0 ) );
	AddKey( STRIDERFX_DARKNESS, CSimpleKeyInterp( 2.0, KEY_SPLINE, 0 ) );

	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 0.5, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 1.25, KEY_ACCELERATE, 1 ) );
	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 1.5, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_FLARE_COLOR, CSimpleKeyInterp( 2.0, KEY_SPLINE, 0 ) );

	AddKey( STRIDERFX_FLARE_SIZE, CSimpleKeyInterp( 0.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_FLARE_SIZE, CSimpleKeyInterp( 1.0, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_FLARE_SIZE, CSimpleKeyInterp( 2.0, KEY_LINEAR, 1 ) );

	AddKey( STRIDERFX_BUBBLE_SIZE, CSimpleKeyInterp( 1.3, KEY_LINEAR, 0.5 ) );
	AddKey( STRIDERFX_BUBBLE_SIZE, CSimpleKeyInterp( 2.0, KEY_DECELERATE, 2 ) );

	AddKey( STRIDERFX_BUBBLE_REFRACT, CSimpleKeyInterp( 1.3, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_BUBBLE_REFRACT, CSimpleKeyInterp( 2.0, KEY_LINEAR, 0 ) );

	AddKey( STRIDERFX_NARROW_BEAM_COLOR, CSimpleKeyInterp( 0.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_NARROW_BEAM_COLOR, CSimpleKeyInterp( 1.25, KEY_ACCELERATE, 1.0 ) );
	AddKey( STRIDERFX_NARROW_BEAM_COLOR, CSimpleKeyInterp( 1.5, KEY_SPLINE, 0 ) );

	AddKey( STRIDERFX_NARROW_BEAM_SIZE, CSimpleKeyInterp( 0.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_NARROW_BEAM_SIZE, CSimpleKeyInterp( 0.5, KEY_ACCELERATE, 1 ) );
	AddKey( STRIDERFX_NARROW_BEAM_SIZE, CSimpleKeyInterp( 1.25, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_NARROW_BEAM_SIZE, CSimpleKeyInterp( 1.5, KEY_DECELERATE, 2 ) );
	
	AddKey( STRIDERFX_WIDE_BEAM_COLOR, CSimpleKeyInterp( 1.25, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_WIDE_BEAM_COLOR, CSimpleKeyInterp( 1.5, KEY_SPLINE, 1 ) );
	AddKey( STRIDERFX_WIDE_BEAM_COLOR, CSimpleKeyInterp( 1.75, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_WIDE_BEAM_COLOR, CSimpleKeyInterp( 2.1, KEY_SPLINE, 0 ) );
	
	AddKey( STRIDERFX_WIDE_BEAM_SIZE, CSimpleKeyInterp( 1.25, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_WIDE_BEAM_SIZE, CSimpleKeyInterp( 2.1, KEY_LINEAR, 1 ) );

	AddKey( STRIDERFX_AFTERGLOW_COLOR, CSimpleKeyInterp( 1.0, KEY_LINEAR, 0 ) );
	AddKey( STRIDERFX_AFTERGLOW_COLOR, CSimpleKeyInterp( 1.25, KEY_SPLINE, 1 ) );
	AddKey( STRIDERFX_AFTERGLOW_COLOR, CSimpleKeyInterp( 3.0, KEY_LINEAR, 1 ) );
	AddKey( STRIDERFX_AFTERGLOW_COLOR, CSimpleKeyInterp( 3.5, KEY_ACCELERATE, 0 ) );

	AddKey( STRIDERFX_WIDE_BEAM_LENGTH, CSimpleKeyInterp( 1.25, KEY_LINEAR, 1.0 ) );
	AddKey( STRIDERFX_WIDE_BEAM_LENGTH, CSimpleKeyInterp( 1.5, KEY_ACCELERATE, 0.0 ) );
	AddKey( STRIDERFX_WIDE_BEAM_LENGTH, CSimpleKeyInterp( 2.1, KEY_LINEAR, 0 ) );

	//AddKey( STRIDERFX_SPARK_COUNT,
	//AddKey( STRIDERFX_STREAK_COUNT,
	//AddKey( STRIDERFX_STEAM_COUNT,
}

CStriderFXEnvelope g_StriderCannonEnvelope;

void ScaleColor( color32 &out, const color32 &in, float scale )
{
	out.r = (byte)(int)((float)in.r * scale);
	out.g = (byte)(int)((float)in.g * scale);
	out.b = (byte)(int)((float)in.b * scale);
	out.a = (byte)(int)((float)in.a * scale);
}

void DrawSpriteTangentSpace( const Vector &vecOrigin, float flWidth, float flHeight, color32 color )
{
	unsigned char pColor[4] = { color.r, color.g, color.b, color.a };

	// Generate half-widths
	flWidth *= 0.5f;
	flHeight *= 0.5f;

	// Compute direction vectors for the sprite
	Vector fwd, right( 1, 0, 0 ), up( 0, 1, 0 );
	VectorSubtract( CurrentViewOrigin(), vecOrigin, fwd );
	float flDist = VectorNormalize( fwd );
	if (flDist >= 1e-3)
	{
		CrossProduct( CurrentViewUp(), fwd, right );
		flDist = VectorNormalize( right );
		if (flDist >= 1e-3)
		{
			CrossProduct( fwd, right, up );
		}
		else
		{
			// In this case, fwd == g_vecVUp, it's right above or 
			// below us in screen space
			CrossProduct( fwd, CurrentViewRight(), up );
			VectorNormalize( up );
			CrossProduct( up, fwd, right );
		}
	}

	Vector left = -right;
	Vector down = -up;
	Vector back = -fwd;

	CMeshBuilder meshBuilder;
	Vector point;
	CMatRenderContextPtr pRenderContext( materials );
	IMesh* pMesh = pRenderContext->GetDynamicMesh( );

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color4ubv (pColor);
	meshBuilder.TexCoord2f (0, 0, 1);
	VectorMA (vecOrigin, -flHeight, up, point);
	VectorMA (point, -flWidth, right, point);
	meshBuilder.TangentS3fv( left.Base() );
	meshBuilder.TangentT3fv( down.Base() );
	meshBuilder.Normal3fv( back.Base() );
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv (pColor);
	meshBuilder.TexCoord2f (0, 0, 0);
	VectorMA (vecOrigin, flHeight, up, point);
	VectorMA (point, -flWidth, right, point);
	meshBuilder.TangentS3fv( left.Base() );
	meshBuilder.TangentT3fv( down.Base() );
	meshBuilder.Normal3fv( back.Base() );
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv (pColor);
	meshBuilder.TexCoord2f (0, 1, 0);
	VectorMA (vecOrigin, flHeight, up, point);
	VectorMA (point, flWidth, right, point);
	meshBuilder.TangentS3fv( left.Base() );
	meshBuilder.TangentT3fv( down.Base() );
	meshBuilder.Normal3fv( back.Base() );
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv (pColor);
	meshBuilder.TexCoord2f (0, 1, 1);
	VectorMA (vecOrigin, -flHeight, up, point);
	VectorMA (point, flWidth, right, point);
	meshBuilder.TangentS3fv( left.Base() );
	meshBuilder.TangentT3fv( down.Base() );
	meshBuilder.Normal3fv( back.Base() );
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();
	
	meshBuilder.End();
	pMesh->Draw();
}


void Strider_DrawSprite( const Vector &vecOrigin, float size, const color32 &color )
{
	DrawSpriteTangentSpace( vecOrigin, size, size, color );
}


void Strider_DrawLine( const Vector &start, const Vector &end, float width, IMaterial *pMaterial, const color32 &color )
{
	FX_DrawLineFade( start, end, width, pMaterial, color, 8.0f );
}

int	C_StriderFX::DrawModel( int )
{
	static color32 white = {255,255,255,255};
	Vector params[STRIDERFX_PARAMETERS];
	bool hasParam[STRIDERFX_PARAMETERS];

	if ( !m_active )
		return 1;

	C_BaseEntity *ent = cl_entitylist->GetEnt( m_entityIndex );
	if ( ent )
	{
		QAngle angles;
		ent->GetAttachment( m_attachment, m_worldPosition, angles );
	}

	// This forces time to drive from the main clock instead of being integrated per-draw below
	// that way the effect moves on even when culled for visibility
	if ( m_limitHitTime > 0 && m_tMax > 0 )
	{
		float dt = m_limitHitTime - gpGlobals->curtime;
		if ( dt < 0 )
		{
			dt = 0;
		}
		// if the clock needs to move, update it.
		if ( m_tMax - dt > m_t )
		{
			m_t = m_tMax - dt;
			m_beamEndPosition = m_worldPosition;
		}
	}
	else
	{
		// don't have enough info to derive the time, integrate current frame time
		m_t += gpGlobals->frametime;
		if ( m_tMax > 0 )
		{
			m_t = clamp( m_t, 0, m_tMax );
			m_beamEndPosition = m_worldPosition;
		}
	}
	float t = m_t;

	bool hasAny = false;
	memset( hasParam, 0, sizeof(hasParam) );
	for ( int i = 0; i < STRIDERFX_PARAMETERS; i++ )
	{
		hasParam[i] = g_StriderCannonEnvelope.m_parameters[i].Interp( params[i], t );
		hasAny = hasAny || hasParam[i];
	}

	pixelvis_queryparams_t gunParams;
	gunParams.Init(m_worldPosition, 4.0f);
	float gunFractionVisible = PixelVisibility_FractionVisible( gunParams, &m_queryHandleGun );
	bool gunVisible = gunFractionVisible > 0.0f ? true : false;

	// draw the narrow beam
	if ( hasParam[STRIDERFX_NARROW_BEAM_COLOR] && hasParam[STRIDERFX_NARROW_BEAM_SIZE] )
	{
		IMaterial *pMat = materials->FindMaterial( "sprites/bluelaser1", TEXTURE_GROUP_CLIENT_EFFECTS );
		float width = NARROW_BEAM_WIDTH * params[STRIDERFX_NARROW_BEAM_SIZE].x;
		color32 color;
		float bright = params[STRIDERFX_NARROW_BEAM_COLOR].x;
		ScaleColor( color, white, bright );

		Strider_DrawLine( m_beamEndPosition, m_targetPosition, width, pMat, color );
	}

	// draw the wide beam
	if ( hasParam[STRIDERFX_WIDE_BEAM_COLOR] && hasParam[STRIDERFX_WIDE_BEAM_SIZE] )
	{
		IMaterial *pMat = materials->FindMaterial( "effects/blueblacklargebeam", TEXTURE_GROUP_CLIENT_EFFECTS );
		float width = WIDE_BEAM_WIDTH * params[STRIDERFX_WIDE_BEAM_SIZE].x;
		color32 color;
		float bright = params[STRIDERFX_WIDE_BEAM_COLOR].x;
		ScaleColor( color, white, bright );
		Vector wideBeamEnd = m_beamEndPosition;
		if ( hasParam[STRIDERFX_WIDE_BEAM_LENGTH] )
		{
			float amt = params[STRIDERFX_WIDE_BEAM_LENGTH].x;
			wideBeamEnd = m_beamEndPosition * amt + m_targetPosition * (1-amt);
		}

		Strider_DrawLine( wideBeamEnd, m_targetPosition, width, pMat, color );
	}

// after glow sprite
	bool updated = false;
	CMatRenderContextPtr pRenderContext( materials );
// warpy sprite bit
	if ( hasParam[STRIDERFX_WARP_SCALE] && !hasParam[STRIDERFX_BUBBLE_SIZE] && gunVisible )
	{
		if ( !updated )
		{
			updated = true;
			pRenderContext->Flush();
			UpdateRefractTexture();
		}

		IMaterial *pMat = materials->FindMaterial( "effects/strider_pinch_dudv", TEXTURE_GROUP_CLIENT_EFFECTS );
		float size = WARP_SIZE;
		float refract = params[STRIDERFX_WARP_SCALE].x * WARP_REFRACT * gunFractionVisible;

		pRenderContext->Bind( pMat, (IClientRenderable*)this );
		IMaterialVar *pVar = pMat->FindVar( "$refractamount", NULL );
		pVar->SetFloatValue( refract );
		Strider_DrawSprite( m_worldPosition, size, white );
	}
// darkening sprite
// glowy blue flare sprite
	if ( hasParam[STRIDERFX_FLARE_COLOR] && hasParam[STRIDERFX_FLARE_SIZE] && hasParam[STRIDERFX_DARKNESS] && gunVisible )
	{
		IMaterial *pMat = materials->FindMaterial( "effects/blueblackflash", TEXTURE_GROUP_CLIENT_EFFECTS );
		float size = FLARE_SIZE * params[STRIDERFX_FLARE_SIZE].x;
		color32 color;
		float bright = params[STRIDERFX_FLARE_COLOR].x * gunFractionVisible;
		ScaleColor( color, white, bright );
		color.a = (int)(255 * params[STRIDERFX_DARKNESS].x);
		pRenderContext->Bind( pMat, (IClientRenderable*)this );
		Strider_DrawSprite( m_worldPosition, size, color );
	}
// bubble warpy sprite
	if ( hasParam[STRIDERFX_BUBBLE_SIZE] )
	{
		Vector wideBeamEnd = m_beamEndPosition;
		if ( hasParam[STRIDERFX_WIDE_BEAM_LENGTH] )
		{
			float amt = params[STRIDERFX_WIDE_BEAM_LENGTH].x;
			wideBeamEnd = m_beamEndPosition * amt + m_targetPosition * (1-amt);
		}
		pixelvis_queryparams_t endParams;
		endParams.Init(wideBeamEnd, 4.0f, 0.001f);
		float endFractionVisible = PixelVisibility_FractionVisible( endParams, &m_queryHandleBeamEnd );
		bool endVisible = endFractionVisible > 0.0f ? true : false;

		if ( endVisible )
		{
			if ( !updated )
			{
				updated = true;
				pRenderContext->Flush();
				UpdateRefractTexture();
			}
			IMaterial *pMat = materials->FindMaterial( "effects/strider_bulge_dudv", TEXTURE_GROUP_CLIENT_EFFECTS );
			float refract = endFractionVisible * WARP_BUBBLE_REFRACT * params[STRIDERFX_BUBBLE_REFRACT].x;
			float size = WARP_BUBBLE_SIZE * params[STRIDERFX_BUBBLE_SIZE].x;
			IMaterialVar *pVar = pMat->FindVar( "$refractamount", NULL );
			pVar->SetFloatValue( refract );

			pRenderContext->Bind( pMat, (IClientRenderable*)this );
			Strider_DrawSprite( wideBeamEnd, size, white );
		}
	}
	else
	{
		// call this to have the check ready on the first frame
		pixelvis_queryparams_t endParams;
		endParams.Init(m_beamEndPosition, 4.0f, 0.001f);
		PixelVisibility_FractionVisible( endParams, &m_queryHandleBeamEnd );
	}
	if ( hasParam[STRIDERFX_AFTERGLOW_COLOR] && gunVisible )
	{
		IMaterial *pMat = materials->FindMaterial( "effects/blueblackflash", TEXTURE_GROUP_CLIENT_EFFECTS );
		float size = AFTERGLOW_SIZE;// * params[STRIDERFX_FLARE_SIZE].x;
		color32 color;
		float bright = params[STRIDERFX_AFTERGLOW_COLOR].x * gunFractionVisible;
		ScaleColor( color, white, bright );

		pRenderContext->Bind( pMat, (IClientRenderable*)this );
		Strider_DrawSprite( m_worldPosition, size, color );

		dlight_t *dl = effects->CL_AllocDlight( m_entityIndex );
		dl->origin = m_worldPosition;
		dl->color.r = 40;
		dl->color.g = 60;
		dl->color.b = 255;
		dl->color.exponent = 5;
		dl->radius = bright * 128;
		dl->die = gpGlobals->curtime + 0.001;
	}

	if ( m_t >= STRIDERFX_END_ALL_TIME && !hasAny )
	{
		EffectShutdown();
	}
	return 1;
}




//-----------------------------------------------------------------------------
// Purpose: Strider class implementation
//-----------------------------------------------------------------------------
C_Strider::C_Strider() :
		m_iv_vecHitPos("C_Strider::m_iv_vecHitPos"),
		m_iv_vecIKTarget("C_Strider::m_iv_vecIKTarget")
{
	AddVar( &m_vecHitPos, &m_iv_vecHitPos, LATCH_ANIMATION_VAR );

	memset(m_vecIKTarget, 0, sizeof(m_vecIKTarget));
	AddVar( &m_vecIKTarget, &m_iv_vecIKTarget, LATCH_ANIMATION_VAR );

	m_flNextRopeCutTime = 0;
}

C_Strider::~C_Strider()
{
}

void C_Strider::ReceiveMessage( int classID, bf_read &msg )
{
	if ( classID != GetClientClass()->m_ClassID )
	{
		// message is for subclass
		BaseClass::ReceiveMessage( classID, msg );
		return;
	}

	int messageType = msg.ReadByte();
	switch( messageType )
	{
	case STRIDER_MSG_STREAKS:
		{
			Vector	pos;
			msg.ReadBitVec3Coord( pos );
			m_cannonFX.SetRenderOrigin( pos );
			m_cannonFX.EffectInit( entindex(), LookupAttachment( "BigGun" ) );
			m_cannonFX.LimitTime( STRIDERFX_BIG_SHOT_TIME );
		}
		break;

	case STRIDER_MSG_BIG_SHOT:
		{
			Vector tmp;
			msg.ReadBitVec3Coord( tmp );
			m_cannonFX.SetTime( STRIDERFX_BIG_SHOT_TIME );
			m_cannonFX.LimitTime( STRIDERFX_END_ALL_TIME );
		}
		break;

	case STRIDER_MSG_DEAD:
		{
			m_cannonFX.EffectShutdown();
		}
		break;
	}
}


void C_Strider::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		// We need to have our render bounds defined or shadow creation won't work correctly
		ClientThink();
		ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_ALWAYS );
	}

	BaseClass::OnDataChanged( updateType );

	m_cannonFX.Update( this, m_vecHitPos );
}

//-----------------------------------------------------------------------------
// Purpose: Recompute my rendering box
//-----------------------------------------------------------------------------
void C_Strider::ClientThink()
{
	// The reason why this is here, as opposed to in SetObjectCollisionBox,
	// is because of IK. The code below recomputes bones so as to get at the hitboxes,
	// which causes IK to trigger, which causes raycasts against the other entities to occur,
	// which is illegal to do while in the Relink phase.

	ComputeEntitySpaceHitboxSurroundingBox( &m_vecRenderMins, &m_vecRenderMaxs );
	// UNDONE: Disabled this until we can get closer to a final map and tune
#if 0
	// Cut ropes.
	if ( gpGlobals->curtime >= m_flNextRopeCutTime )
	{
		// Blow the bbox out a little.
		Vector vExtendedMins = vecMins - Vector( 50, 50, 50 );
		Vector vExtendedMaxs = vecMaxs + Vector( 50, 50, 50 );

		C_RopeKeyframe *ropes[512];
		int nRopes = C_RopeKeyframe::GetRopesIntersectingAABB( ropes, ARRAYSIZE( ropes ), GetAbsOrigin() + vExtendedMins, GetAbsOrigin() + vExtendedMaxs );
		for ( int i=0; i < nRopes; i++ )
		{
			C_RopeKeyframe *pRope = ropes[i];

			if ( pRope->GetEndEntity() )
			{
				Vector vPos;
				if ( pRope->GetEndPointPos( 1, vPos ) )
				{
					// Detach the endpoint.
					pRope->SetEndEntity( NULL );
					
					// Make some spark effect here..
					g_pEffects->Sparks( vPos );
				}				
			}
		}

		m_flNextRopeCutTime = gpGlobals->curtime + 0.5;
	}
#endif

	// True argument because the origin may have stayed the same, but the size is expected to always change
	g_pClientShadowMgr->AddToDirtyShadowList( this, true );
}


//-----------------------------------------------------------------------------
// Purpose: Recompute my rendering box
//-----------------------------------------------------------------------------
void C_Strider::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	theMins = m_vecRenderMins;
	theMaxs = m_vecRenderMaxs;
}


//-----------------------------------------------------------------------------
// Strider muzzle flashes
//-----------------------------------------------------------------------------
void MuzzleFlash_Strider( ClientEntityHandle_t hEntity, int attachmentIndex )
{
	VPROF_BUDGET( "MuzzleFlash_Strider", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	// If the client hasn't seen this entity yet, bail.
	matrix3x4_t	matAttachment;
	if ( !FX_GetAttachmentTransform( hEntity, attachmentIndex, matAttachment ) )
		return;

	CSmartPtr<CLocalSpaceEmitter> pSimple = CLocalSpaceEmitter::Create( "MuzzleFlash_Strider", hEntity, attachmentIndex );

	SimpleParticle *pParticle;
	Vector			forward(1,0,0), offset; //NOTENOTE: All coords are in local space

	float flScale = random->RandomFloat( 3.0f, 4.0f );

	float burstSpeed = random->RandomFloat( 400.0f, 600.0f );

#define	FRONT_LENGTH 12

	// Front flash
	for ( int i = 1; i < FRONT_LENGTH; i++ )
	{
		offset = (forward * (i*2.0f*flScale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( VarArgs( "effects/combinemuzzle%d", random->RandomInt(1,2) ) ), offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= 0.1f;

		pParticle->m_vecVelocity = forward * burstSpeed;

		pParticle->m_uchColor[0]	= 255;
		pParticle->m_uchColor[1]	= 255;
		pParticle->m_uchColor[2]	= 255;

		pParticle->m_uchStartAlpha	= 255.0f;
		pParticle->m_uchEndAlpha	= 0;

		pParticle->m_uchStartSize	= ( (random->RandomFloat( 6.0f, 8.0f ) * (FRONT_LENGTH-(i))/(FRONT_LENGTH*0.75f)) * flScale );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}
	
	Vector right(0,1,0), up(0,0,1);
	Vector dir = right - up;

#define	SIDE_LENGTH	8

	burstSpeed = random->RandomFloat( 400.0f, 600.0f );

	// Diagonal flash
	for ( int i = 1; i < SIDE_LENGTH; i++ )
	{
		offset = (dir * (i*flScale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( VarArgs( "effects/combinemuzzle%d", random->RandomInt(1,2) ) ), offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= 0.2f;

		pParticle->m_vecVelocity = dir * burstSpeed * 0.25f;

		pParticle->m_uchColor[0]	= 255;
		pParticle->m_uchColor[1]	= 255;
		pParticle->m_uchColor[2]	= 255;

		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 0;

		pParticle->m_uchStartSize	= ( (random->RandomFloat( 2.0f, 4.0f ) * (SIDE_LENGTH-(i))/(SIDE_LENGTH*0.5f)) * flScale );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}

	dir = right + up;
	burstSpeed = random->RandomFloat( 400.0f, 600.0f );

	// Diagonal flash
	for ( int i = 1; i < SIDE_LENGTH; i++ )
	{
		offset = (-dir * (i*flScale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( VarArgs( "effects/combinemuzzle%d", random->RandomInt(1,2) ) ), offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= 0.2f;

		pParticle->m_vecVelocity = dir * -burstSpeed * 0.25f;

		pParticle->m_uchColor[0]	= 255;
		pParticle->m_uchColor[1]	= 255;
		pParticle->m_uchColor[2]	= 255;

		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 0;

		pParticle->m_uchStartSize	= ( (random->RandomFloat( 2.0f, 4.0f ) * (SIDE_LENGTH-(i))/(SIDE_LENGTH*0.5f)) * flScale );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}

	dir = up;
	burstSpeed = random->RandomFloat( 400.0f, 600.0f );

	// Top flash
	for ( int i = 1; i < SIDE_LENGTH; i++ )
	{
		offset = (dir * (i*flScale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( VarArgs( "effects/combinemuzzle%d", random->RandomInt(1,2) ) ), offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= 0.2f;

		pParticle->m_vecVelocity = dir * burstSpeed * 0.25f;

		pParticle->m_uchColor[0]	= 255;
		pParticle->m_uchColor[1]	= 255;
		pParticle->m_uchColor[2]	= 255;

		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 0;

		pParticle->m_uchStartSize	= ( (random->RandomFloat( 2.0f, 4.0f ) * (SIDE_LENGTH-(i))/(SIDE_LENGTH*0.5f)) * flScale );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}

	pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), pSimple->GetPMaterial( "effects/strider_muzzle" ), vec3_origin );
		
	if ( pParticle == NULL )
		return;

	pParticle->m_flLifetime		= 0.0f;
	pParticle->m_flDieTime		= random->RandomFloat( 0.3f, 0.4f );

	pParticle->m_vecVelocity.Init();

	pParticle->m_uchColor[0]	= 255;
	pParticle->m_uchColor[1]	= 255;
	pParticle->m_uchColor[2]	= 255;

	pParticle->m_uchStartAlpha	= 255;
	pParticle->m_uchEndAlpha	= 0;

	pParticle->m_uchStartSize	= flScale * random->RandomFloat( 12.0f, 16.0f );
	pParticle->m_uchEndSize		= 0.0f;
	pParticle->m_flRoll			= random->RandomInt( 0, 360 );
	pParticle->m_flRollDelta	= 0.0f;

	Vector		origin;
	MatrixGetColumn( matAttachment, 3, &origin );

	int entityIndex = ClientEntityList().HandleToEntIndex( hEntity );
	if ( entityIndex >= 0 )
	{
		dlight_t *el = effects->CL_AllocElight( LIGHT_INDEX_MUZZLEFLASH + entityIndex );

		el->origin	= origin;

		el->color.r = 64;
		el->color.g = 128;
		el->color.b = 255;
		el->color.exponent = 5;

		el->radius	= random->RandomInt( 100, 150 );
		el->decay	= el->radius / 0.05f;
		el->die		= gpGlobals->curtime + 0.1f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void StriderMuzzleFlashCallback( const CEffectData &data )
{
	MuzzleFlash_Strider( data.m_hEntity, data.m_nAttachmentIndex );
}

DECLARE_CLIENT_EFFECT( "StriderMuzzleFlash", StriderMuzzleFlashCallback );

#define	BLOOD_MIN_SPEED	64.0f*2.0f
#define BLOOD_MAX_SPEED 256.0f*8.0f

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&normal - 
//			scale - 
//-----------------------------------------------------------------------------
void StriderBlood( const Vector &origin, const Vector &normal, float scale )
{
	VPROF_BUDGET( "StriderBlood", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	
	//Find area ambient light color and use it to tint smoke
	Vector worldLight = WorldGetLightForPoint( origin, true );
	Vector	tint;
	float	luminosity;
	UTIL_GetNormalizedColorTintAndLuminosity( worldLight, &tint, &luminosity );

	// We only take a portion of the tint
	tint = (tint * 0.25f)+(Vector(0.75f,0.75f,0.75f));

	// Rescale to a character range
	luminosity = MAX( 200, luminosity*255 );

	CSmartPtr<CSplashParticle> pSimple = CSplashParticle::Create( "splish" );
	pSimple->SetSortOrigin( origin );

	int i;
	float	flScale = scale / 8.0f;

	PMaterialHandle	hMaterial = ParticleMgr()->GetPMaterial( "effects/slime1" );

	float	length = 0.2f;
	Vector	vForward, vRight, vUp;
	Vector	offDir;

	TrailParticle	*tParticle;

	CSmartPtr<CTrailParticles> sparkEmitter = CTrailParticles::Create( "splash" );

	if ( !sparkEmitter )
		return;

	sparkEmitter->SetSortOrigin( origin );
	sparkEmitter->m_ParticleCollision.SetGravity( 600.0f );
	sparkEmitter->SetFlag( bitsPARTICLE_TRAIL_VELOCITY_DAMPEN );
	sparkEmitter->SetVelocityDampen( 2.0f );

	//Dump out drops
	Vector	offset;
	for ( i = 0; i < 64; i++ )
	{
		offset = origin;
		offset[0] += random->RandomFloat( -8.0f, 8.0f ) * flScale;
		offset[1] += random->RandomFloat( -8.0f, 8.0f ) * flScale;
		offset[2] += random->RandomFloat( -8.0f, 8.0f ) * flScale;

		tParticle = (TrailParticle *) sparkEmitter->AddParticle( sizeof(TrailParticle), hMaterial, offset );

		if ( tParticle == NULL )
			break;

		tParticle->m_flLifetime	= 0.0f;
		tParticle->m_flDieTime	= 1.0f;

		offDir = normal + RandomVector( -1.0f, 1.0f );

		tParticle->m_vecVelocity = offDir * random->RandomFloat( BLOOD_MIN_SPEED * flScale * 2.0f, BLOOD_MAX_SPEED * flScale * 2.0f );
		tParticle->m_vecVelocity[2] += random->RandomFloat( 8.0f, 32.0f ) * flScale;

		tParticle->m_flWidth		= random->RandomFloat( 20.0f, 26.0f ) * flScale;
		tParticle->m_flLength		= random->RandomFloat( length*0.5f, length ) * flScale;

		int nColor = random->RandomInt( luminosity*0.75f, luminosity );
		tParticle->m_color.r = nColor * tint.x;
		tParticle->m_color.g = nColor * tint.y;
		tParticle->m_color.b = nColor * tint.z;
		tParticle->m_color.a = 255;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void StriderBloodCallback( const CEffectData &data )
{
	StriderBlood( data.m_vOrigin, data.m_vNormal, data.m_flScale );
}

DECLARE_CLIENT_EFFECT( "StriderBlood", StriderBloodCallback );

