//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "basegrenade_shared.h"
#include "fx_interpvalue.h"
#include "fx_envelope.h"
#include "materialsystem/imaterialvar.h"
#include "particles_simple.h"
#include "particles_attractor.h"

// FIXME: Move out
extern void DrawSpriteTangentSpace( const Vector &vecOrigin, float flWidth, float flHeight, color32 color );

#define	EXPLOSION_DURATION	3.0f

//-----------------------------------------------------------------------------
// Explosion effect for hopwire
//-----------------------------------------------------------------------------

class C_HopwireExplosion : public C_EnvelopeFX
{
	typedef C_EnvelopeFX	BaseClass;

public:
	C_HopwireExplosion( void ) : 
	  m_hOwner( NULL )
	{
		m_FXCoreScale.SetAbsolute( 0.0f );
		m_FXCoreAlpha.SetAbsolute( 0.0f );
	}

	virtual void	Update( void );
	virtual int		DrawModel( int flags );
	virtual void	GetRenderBounds( Vector& mins, Vector& maxs );

	bool			SetupEmitters( void );
	void			AddParticles( void );
	void			SetOwner( C_BaseEntity *pOwner );
	void			StartExplosion( void );
	void			StopExplosion( void );
	void			StartPreExplosion( void );

private:
	CInterpolatedValue		m_FXCoreScale;
	CInterpolatedValue		m_FXCoreAlpha;

	CSmartPtr<CSimpleEmitter>		m_pSimpleEmitter;
	CSmartPtr<CParticleAttractor>	m_pAttractorEmitter;

	TimedEvent			m_ParticleTimer;

	CHandle<C_BaseEntity>	m_hOwner;
};

//-----------------------------------------------------------------------------
// Purpose: Setup the emitters we'll be using
//-----------------------------------------------------------------------------
bool C_HopwireExplosion::SetupEmitters( void )
{
	// Setup the basic core emitter
	if ( m_pSimpleEmitter.IsValid() == false )
	{
		m_pSimpleEmitter = CSimpleEmitter::Create( "hopwirecore" );

		if ( m_pSimpleEmitter.IsValid() == false )
			return false;
	}

	// Setup the attractor emitter
	if ( m_pAttractorEmitter.IsValid() == false )
	{
		m_pAttractorEmitter = CParticleAttractor::Create( GetRenderOrigin(), "hopwireattractor" );

		if ( m_pAttractorEmitter.IsValid() == false )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_HopwireExplosion::AddParticles( void )
{
	// Make sure the emitters are setup properly
	if ( SetupEmitters() == false )
		return;

	float tempDelta = gpGlobals->frametime;
	while( m_ParticleTimer.NextEvent( tempDelta ) )
	{	
		// ========================
		// Attracted dust particles
		// ========================

		// Update our attractor point
		m_pAttractorEmitter->SetAttractorOrigin( GetRenderOrigin() );

		Vector offset;
		SimpleParticle *sParticle;

		offset = GetRenderOrigin() + RandomVector( -256.0f, 256.0f );

		sParticle = (SimpleParticle *) m_pAttractorEmitter->AddParticle( sizeof(SimpleParticle), g_Mat_Fleck_Cement[0], offset );

		if ( sParticle == NULL )
			return;
		
		sParticle->m_vecVelocity	= Vector(0,0,8);
		sParticle->m_flDieTime		= 0.5f;
		sParticle->m_flLifetime		= 0.0f;

		sParticle->m_flRoll			= Helper_RandomInt( 0, 360 );
		sParticle->m_flRollDelta	= 1.0f;

		float alpha = random->RandomFloat( 128.0f, 200.0f );

		sParticle->m_uchColor[0]	= alpha;
		sParticle->m_uchColor[1]	= alpha;
		sParticle->m_uchColor[2]	= alpha;
		sParticle->m_uchStartAlpha	= alpha;
		sParticle->m_uchEndAlpha	= alpha;

		sParticle->m_uchStartSize	= random->RandomInt( 1, 4 );
		sParticle->m_uchEndSize		= 0;

		// ========================
		// Core effects
		// ========================

		// Reset our sort origin
		m_pSimpleEmitter->SetSortOrigin( GetRenderOrigin() );

		// Base of the core effect
		sParticle = (SimpleParticle *) m_pSimpleEmitter->AddParticle( sizeof(SimpleParticle), m_pSimpleEmitter->GetPMaterial( "effects/strider_muzzle" ), GetRenderOrigin() );

		if ( sParticle == NULL )
			return;
		
		sParticle->m_vecVelocity	= vec3_origin;
		sParticle->m_flDieTime		= 0.2f;
		sParticle->m_flLifetime		= 0.0f;

		sParticle->m_flRoll			= Helper_RandomInt( 0, 360 );
		sParticle->m_flRollDelta	= 4.0f;

		alpha = random->RandomInt( 32, 200 );

		sParticle->m_uchColor[0]	= alpha;
		sParticle->m_uchColor[1]	= alpha;
		sParticle->m_uchColor[2]	= alpha;
		sParticle->m_uchStartAlpha	= 0;
		sParticle->m_uchEndAlpha	= alpha;

		sParticle->m_uchStartSize	= 255;
		sParticle->m_uchEndSize		= 0;

		// Make sure we encompass the complete particle here!
		m_pSimpleEmitter->SetParticleCullRadius( sParticle->m_uchEndSize );

		// =========================
		// Dust ring effect
		// =========================

		if ( random->RandomInt( 0, 5 ) != 1 )
			return;

		Vector vecDustColor;
		vecDustColor.x = 0.35f;
		vecDustColor.y = 0.3f;
		vecDustColor.z = 0.25f;

		Vector	color;

		int	numRingSprites = 8;
		float yaw;
		Vector forward, vRight, vForward;

		vForward = Vector( 0, 1, 0 );
		vRight = Vector( 1, 0, 0 );

		float	yawOfs = random->RandomFloat( 0, 359 );

		for ( int i = 0; i < numRingSprites; i++ )
		{
			yaw = ( (float) i / (float) numRingSprites ) * 360.0f;
			yaw += yawOfs;

			forward = ( vRight * sin( DEG2RAD( yaw) ) ) + ( vForward * cos( DEG2RAD( yaw ) ) );
			VectorNormalize( forward );

			trace_t	tr;

			UTIL_TraceLine( GetRenderOrigin(), GetRenderOrigin()+(Vector(0, 0, -1024)), MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

			offset = ( RandomVector( -4.0f, 4.0f ) + tr.endpos ) + ( forward * 512.0f );

			sParticle = (SimpleParticle *) m_pSimpleEmitter->AddParticle( sizeof(SimpleParticle), g_Mat_DustPuff[random->RandomInt(0,1)], offset );

			if ( sParticle != NULL )
			{
				sParticle->m_flLifetime = 0.0f;
				sParticle->m_flDieTime	= random->RandomFloat( 0.25f, 0.5f );

				sParticle->m_vecVelocity = forward * -random->RandomFloat( 1000, 1500 );
				sParticle->m_vecVelocity[2] += 128.0f;
			
				#if __EXPLOSION_DEBUG
				debugoverlay->AddLineOverlay( m_vecOrigin, m_vecOrigin + sParticle->m_vecVelocity, 255, 0, 0, false, 3 );
				#endif

				sParticle->m_uchColor[0] = vecDustColor.x * 255.0f;
				sParticle->m_uchColor[1] = vecDustColor.y * 255.0f;
				sParticle->m_uchColor[2] = vecDustColor.z * 255.0f;

				sParticle->m_uchStartSize	= random->RandomInt( 32, 128 );
				sParticle->m_uchEndSize		= 200;

				sParticle->m_uchStartAlpha	= random->RandomFloat( 16, 64 );
				sParticle->m_uchEndAlpha	= 0;
				
				sParticle->m_flRoll			= random->RandomInt( 0, 360 );
				sParticle->m_flRollDelta	= random->RandomFloat( -16.0f, 16.0f );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void C_HopwireExplosion::SetOwner( C_BaseEntity *pOwner )
{
	m_hOwner = pOwner;
}

//-----------------------------------------------------------------------------
// Purpose: Updates the internal values for the effect
//-----------------------------------------------------------------------------
void C_HopwireExplosion::Update( void )
{
	if ( m_hOwner )
	{
		SetRenderOrigin( m_hOwner->GetRenderOrigin() );
	}

	BaseClass::Update();
}

//-----------------------------------------------------------------------------
// Purpose: Updates and renders all effects
//-----------------------------------------------------------------------------
int C_HopwireExplosion::DrawModel( int flags )
{
	AddParticles();

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Flush();
	UpdateRefractTexture();

	IMaterial *pMat = materials->FindMaterial( "effects/strider_pinch_dudv", TEXTURE_GROUP_CLIENT_EFFECTS );

	float refract = m_FXCoreAlpha.Interp( gpGlobals->curtime );
	float scale = m_FXCoreScale.Interp( gpGlobals->curtime );

	IMaterialVar *pVar = pMat->FindVar( "$refractamount", NULL );
	pVar->SetFloatValue( refract );

	pRenderContext->Bind( pMat, (IClientRenderable*)this );
	
	float sin1 = sinf( gpGlobals->curtime * 10 );
	float sin2 = sinf( gpGlobals->curtime );

	float scaleY = ( sin1 * sin2 ) * 32.0f;
	float scaleX = (sin2 * sin2) * 32.0f;

	// FIXME: The ball needs to sort properly at all times
	static color32 white = {255,255,255,255};
	DrawSpriteTangentSpace( GetRenderOrigin() + ( CurrentViewForward() * 128.0f ), scale+scaleX, scale+scaleY, white );

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the bounds relative to the origin (render bounds)
//-----------------------------------------------------------------------------
void C_HopwireExplosion::GetRenderBounds( Vector& mins, Vector& maxs )
{
	float scale = m_FXCoreScale.Interp( gpGlobals->curtime );

	mins.Init( -scale, -scale, -scale );
	maxs.Init(  scale,  scale,  scale );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_HopwireExplosion::StartExplosion( void )
{
	m_FXCoreScale.Init( 300.0f, 500.0f, 2.0f, INTERP_SPLINE );
	m_FXCoreAlpha.Init( 0.0f, 0.1f, 1.5f, INTERP_SPLINE );

	// Particle timer
	m_ParticleTimer.Init( 60 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_HopwireExplosion::StopExplosion( void )
{
	m_FXCoreAlpha.InitFromCurrent( 0.0f, 1.0f, INTERP_SPLINE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_HopwireExplosion::StartPreExplosion( void )
{
}

//-----------------------------------------------------------------------------
// Hopwire client class
//-----------------------------------------------------------------------------

class C_GrenadeHopwire : public C_BaseGrenade
{
	DECLARE_CLASS( C_GrenadeHopwire, C_BaseGrenade );
	DECLARE_CLIENTCLASS();

public:
	C_GrenadeHopwire( void );

	virtual int		DrawModel( int flags );

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	ReceiveMessage( int classID, bf_read &msg );

private:

	C_HopwireExplosion	m_ExplosionEffect;	// Explosion effect information and drawing
};

IMPLEMENT_CLIENTCLASS_DT( C_GrenadeHopwire, DT_GrenadeHopwire, CGrenadeHopwire )
END_RECV_TABLE()

#define	HOPWIRE_START_EXPLOSION		0
#define	HOPWIRE_STOP_EXPLOSION		1
#define	HOPWIRE_START_PRE_EXPLOSION	2

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
C_GrenadeHopwire::C_GrenadeHopwire( void )
{
	m_ExplosionEffect.SetActive( false );
}

//-----------------------------------------------------------------------------
// Purpose: Receive messages from the server
// Input  : classID - class to receive the message
//			&msg - message in question
//-----------------------------------------------------------------------------
void C_GrenadeHopwire::ReceiveMessage( int classID, bf_read &msg )
{
	if ( classID != GetClientClass()->m_ClassID )
	{
		// Message is for subclass
		BaseClass::ReceiveMessage( classID, msg );
		return;
	}

	int messageType = msg.ReadByte();
	switch( messageType )
	{
	case HOPWIRE_START_EXPLOSION:
		{
			m_ExplosionEffect.SetActive();
			m_ExplosionEffect.SetOwner( this );
			m_ExplosionEffect.StartExplosion();
		}
		break;
	case HOPWIRE_STOP_EXPLOSION:
		{
			m_ExplosionEffect.StopExplosion();
		}
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_GrenadeHopwire::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	m_ExplosionEffect.Update();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
//-----------------------------------------------------------------------------
int	C_GrenadeHopwire::DrawModel( int flags )
{
	if ( m_ExplosionEffect.IsActive() )
		return 1;

	return BaseClass::DrawModel( flags );
}

