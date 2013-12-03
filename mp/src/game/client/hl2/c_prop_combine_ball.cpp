//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_prop_combine_ball.h"
#include "materialsystem/imaterial.h"
#include "model_types.h"
#include "c_physicsprop.h"
#include "c_te_effect_dispatch.h"
#include "fx_quad.h"
#include "fx.h"
#include "clienteffectprecachesystem.h"
#include "view.h"
#include "view_scene.h"
#include "beamdraw.h"

// Precache our effects
CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectCombineBall )
CLIENTEFFECT_MATERIAL( "effects/ar2_altfire1" )
CLIENTEFFECT_MATERIAL( "effects/ar2_altfire1b" )
CLIENTEFFECT_MATERIAL( "effects/combinemuzzle1_nocull" )
CLIENTEFFECT_MATERIAL( "effects/combinemuzzle2_nocull" )
CLIENTEFFECT_MATERIAL( "effects/combinemuzzle1" )
CLIENTEFFECT_MATERIAL( "effects/ar2_altfire1" )
CLIENTEFFECT_MATERIAL( "effects/ar2_altfire1b" )
CLIENTEFFECT_REGISTER_END()

IMPLEMENT_CLIENTCLASS_DT( C_PropCombineBall, DT_PropCombineBall, CPropCombineBall )
	RecvPropBool( RECVINFO( m_bEmit ) ),
	RecvPropFloat( RECVINFO( m_flRadius ) ),
	RecvPropBool( RECVINFO( m_bHeld ) ),
	RecvPropBool( RECVINFO( m_bLaunched ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PropCombineBall::C_PropCombineBall( void )
{
	m_pFlickerMaterial = NULL;
	m_pBodyMaterial = NULL;
	m_pBlurMaterial = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_PropCombineBall::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_vecLastOrigin = GetAbsOrigin();
		InitMaterials();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : RenderGroup_t
//-----------------------------------------------------------------------------
RenderGroup_t C_PropCombineBall::GetRenderGroup( void )
{
	return RENDER_GROUP_TRANSLUCENT_ENTITY;
}

//-----------------------------------------------------------------------------
// Purpose: Cache the material handles
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_PropCombineBall::InitMaterials( void )
{
	// Motion blur
	if ( m_pBlurMaterial == NULL )
	{
		m_pBlurMaterial = materials->FindMaterial( "effects/ar2_altfire1b", NULL, false );

		if ( m_pBlurMaterial == NULL )
			return false;
	}

	// Main body of the ball
	if ( m_pBodyMaterial == NULL )
	{
		m_pBodyMaterial = materials->FindMaterial( "effects/ar2_altfire1", NULL, false );

		if ( m_pBodyMaterial == NULL )
			return false;
	}

	// Flicker material
	if ( m_pFlickerMaterial == NULL )
	{
		m_pFlickerMaterial = materials->FindMaterial( "effects/combinemuzzle1", NULL, false );

		if ( m_pFlickerMaterial == NULL )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropCombineBall::DrawMotionBlur( void )
{
	float color[3];

	Vector	vecDir = GetAbsOrigin() - m_vecLastOrigin;
	float	speed = VectorNormalize( vecDir );
	
	speed = clamp( speed, 0, 32 );
	
	float	stepSize = MIN( ( speed * 0.5f ), 4.0f );

	Vector	spawnPos = GetAbsOrigin();
	Vector	spawnStep = -vecDir * stepSize;

	float base = RemapValClamped( speed, 4, 32, 0.0f, 1.0f );

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_pBlurMaterial );

	// Draw the motion blurred trail
	for ( int i = 0; i < 8; i++ )
	{
		spawnPos += spawnStep;

		color[0] = color[1] = color[2] = base * ( 1.0f - ( (float) i / 12.0f ) );

		DrawHalo( m_pBlurMaterial, spawnPos, m_flRadius, color );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropCombineBall::DrawFlicker( void )
{
	float rand1 = random->RandomFloat( 0.2f, 0.3f );
	float rand2 = random->RandomFloat( 1.5f, 2.5f );

	if ( gpGlobals->frametime == 0.0f )
	{
		rand1 = 0.2f;
		rand2 = 1.5f;
	}

	float color[3];
	color[0] = color[1] = color[2] = rand1;

	// Draw the flickering glow
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_pFlickerMaterial );
	DrawHalo( m_pFlickerMaterial, GetAbsOrigin(), m_flRadius * rand2, color );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pMaterial - 
//			source - 
//			color - 
//-----------------------------------------------------------------------------
void DrawHaloOriented( const Vector& source, float scale, float const *color, float roll )
{
	Vector point, screen;
	
	CMatRenderContextPtr pRenderContext( materials );
	IMesh* pMesh = pRenderContext->GetDynamicMesh();

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	// Transform source into screen space
	ScreenTransform( source, screen );

	Vector right, up;
	float sr, cr;

	SinCos( roll, &sr, &cr );

	for ( int i = 0; i < 3; i++ )
	{
		right[i] = CurrentViewRight()[i] * cr + CurrentViewUp()[i] * sr;
		up[i] = CurrentViewRight()[i] * -sr + CurrentViewUp()[i] * cr;
	}

	meshBuilder.Color3fv (color);
	meshBuilder.TexCoord2f (0, 0, 1);
	VectorMA (source, -scale, up, point);
	VectorMA (point, -scale, right, point);
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3fv (color);
	meshBuilder.TexCoord2f (0, 0, 0);
	VectorMA (source, scale, up, point);
	VectorMA (point, -scale, right, point);
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3fv (color);
	meshBuilder.TexCoord2f (0, 1, 0);
	VectorMA (source, scale, up, point);
	VectorMA (point, scale, right, point);
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3fv (color);
	meshBuilder.TexCoord2f (0, 1, 1);
	VectorMA (source, -scale, up, point);
	VectorMA (point, scale, right, point);
	meshBuilder.Position3fv (point.Base());
	meshBuilder.AdvanceVertex();
	
	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
// Output : int
//-----------------------------------------------------------------------------
int C_PropCombineBall::DrawModel( int flags )
{
	if ( !m_bEmit )
		return 0;
	
	// Make sure our materials are cached
	if ( !InitMaterials() )
	{
		//NOTENOTE: This means that a material was not found for the combine ball, so it may not render!
		AssertOnce( 0 );
		return 0;
	}

	// Draw the flickering overlay
	DrawFlicker();
	
	// Draw the motion blur from movement
	if ( m_bHeld || m_bLaunched )
	{
		DrawMotionBlur();
	}

	// Draw the model if we're being held
	if ( m_bHeld )
	{
		QAngle	angles;
		VectorAngles( -CurrentViewForward(), angles );

		// Always orient towards the camera!
		SetAbsAngles( angles );

		BaseClass::DrawModel( flags );
	}
	else
	{
		float color[3];
		color[0] = color[1] = color[2] = 1.0f;

		float sinOffs = 1.0f * sin( gpGlobals->curtime * 25 );

		float roll = SpawnTime();

		// Draw the main ball body
		CMatRenderContextPtr pRenderContext( materials );
		pRenderContext->Bind( m_pBodyMaterial, (C_BaseEntity*) this );
		DrawHaloOriented( GetAbsOrigin(), m_flRadius + sinOffs, color, roll );
	}
	
	m_vecLastOrigin = GetAbsOrigin();

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void CombineBallImpactCallback( const CEffectData &data )
{
	// Quick flash
	FX_AddQuad( data.m_vOrigin,
				data.m_vNormal,
				data.m_flRadius * 10.0f,
				0,
				0.75f, 
				1.0f,
				0.0f,
				0.4f,
				random->RandomInt( 0, 360 ), 
				0,
				Vector( 1.0f, 1.0f, 1.0f ), 
				0.25f, 
				"effects/combinemuzzle1_nocull",
				(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );

	// Lingering burn
	FX_AddQuad( data.m_vOrigin,
				data.m_vNormal, 
				data.m_flRadius * 2.0f,
				data.m_flRadius * 4.0f,
				0.75f, 
				1.0f,
				0.0f,
				0.4f,
				random->RandomInt( 0, 360 ), 
				0,
				Vector( 1.0f, 1.0f, 1.0f ), 
				0.5f, 
				"effects/combinemuzzle2_nocull",
				(FXQUAD_BIAS_SCALE|FXQUAD_BIAS_ALPHA) );

	// Throw sparks
	FX_ElectricSpark( data.m_vOrigin, 2, 1, &data.m_vNormal );
}

DECLARE_CLIENT_EFFECT( "cball_bounce", CombineBallImpactCallback );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void CombineBallExplosionCallback( const CEffectData &data )
{
	Vector normal(0,0,1);

	// Throw sparks
	FX_ElectricSpark( data.m_vOrigin, 4, 1, &normal );
}

DECLARE_CLIENT_EFFECT( "cball_explode", CombineBallExplosionCallback );