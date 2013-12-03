//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A simple test entity for creating special effects
//
//=============================================================================

#include "cbase.h"
#include "c_te_effect_dispatch.h"
#include "particles_simple.h"

// Declare the sparkler entity for the client-side
class C_Sparkler : public C_BaseEntity
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_Sparkler, C_BaseEntity );

	virtual void OnDataChanged( DataUpdateType_t updateType );	// Called when data changes on the server
	virtual void ClientThink( void );							// Client-side think function for the entity

private:
	bool	m_bEmit;	// Determines whether or not we should emit particles
	float	m_flScale;	// Size of the effect
	
	CSmartPtr<CSimpleEmitter>	m_hEmitter;			// Particle emitter for this entity
	PMaterialHandle				m_hMaterial;		// Material handle used for this entity's particles
	TimedEvent					m_tParticleTimer;	// Timer used to control particle emission rate
};

// Declare the data-table for server/client communication
IMPLEMENT_CLIENTCLASS_DT( C_Sparkler, DT_Sparkler, CSparkler )
	RecvPropInt( RECVINFO( m_bEmit ) ),	// Boolean state from the server
	RecvPropFloat( RECVINFO( m_flScale ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Called when data changes on the server
//-----------------------------------------------------------------------------
void C_Sparkler::OnDataChanged( DataUpdateType_t updateType )
{
	// NOTE: We MUST call the base classes' implementation of this function
	BaseClass::OnDataChanged( updateType );
	
	// Setup our entity's particle system on creation
	if ( updateType == DATA_UPDATE_CREATED )
	{
		// Creat the emitter
		m_hEmitter = CSimpleEmitter::Create( "env_sparkler" );

		// Obtain a reference handle to our particle's desired material
		if ( m_hEmitter.IsValid() )
		{
			m_hMaterial = m_hEmitter->GetPMaterial( "effects/yellowflare" );
		}

		// Spawn 128 particles per second
		m_tParticleTimer.Init( 128 );

		// Call our ClientThink() function once every client frame
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Client-side think function for the entity
//-----------------------------------------------------------------------------
void C_Sparkler::ClientThink( void )
{
	// We must have a valid emitter
	if ( m_hEmitter == NULL )
		return;

	// We must be allowed to emit particles by the server
	if ( m_bEmit == false )
		return;

	SimpleParticle *pParticle;

	float curTime = gpGlobals->frametime;

	// Add as many particles as required this frame
	while ( m_tParticleTimer.NextEvent( curTime ) )
	{
		// Create the particle
		pParticle = m_hEmitter->AddSimpleParticle( m_hMaterial, GetAbsOrigin() );

		if ( pParticle == NULL )
			return;

		// Setup our size
		pParticle->m_uchStartSize = (unsigned char) m_flScale;
		pParticle->m_uchEndSize = 0;

		// Setup our roll
		pParticle->m_flRoll = random->RandomFloat( 0, 2*M_PI );
		pParticle->m_flRollDelta = random->RandomFloat( -DEG2RAD( 180 ), DEG2RAD( 180 ) );

		// Set our color
		pParticle->m_uchColor[0] = 255;
		pParticle->m_uchColor[1] = 255;
		pParticle->m_uchColor[2] = 255;

		// Setup our alpha values
		pParticle->m_uchStartAlpha = 255;
		pParticle->m_uchEndAlpha = 255;
		
		// Obtain a random direction
		Vector velocity = RandomVector( -1.0f, 1.0f );
		VectorNormalize( velocity );

		// Obtain a random speed
		float speed = random->RandomFloat( 4.0f, 8.0f ) * m_flScale;

		// Set our velocity
		pParticle->m_vecVelocity = velocity * speed;

		// Die in a short range of time
		pParticle->m_flDieTime = random->RandomFloat( 0.25f, 0.5f );
	}
}

// ============================================================================
//
//   Dispatch Effect version
//
// ============================================================================

//-----------------------------------------------------------------------------
// Purpose: Callback to create a sparkle effect on the client
// Input  : &data - information about the effect
//-----------------------------------------------------------------------------
void SparkleCallback( const CEffectData &data )
{
	// Create a simple particle emitter
	CSmartPtr<CSimpleEmitter> pSparkleEmitter = CSimpleEmitter::Create( "Sparkle" );

	if ( pSparkleEmitter == NULL )
		return;

	// Make local versions of our passed in data
	Vector	origin = data.m_vOrigin;
	float	scale = data.m_flScale;

	// Set our sort origin to make the system cull properly
	pSparkleEmitter->SetSortOrigin( origin );

	// Find the material handle we wish to use for these particles
	PMaterialHandle hMaterial = pSparkleEmitter->GetPMaterial( "effects/yellowflare" );

	SimpleParticle *pParticle;

	// Make a group of particles in the world
	for ( int i = 0; i < 64; i++ )
	{
		// Create a particle
		pParticle = pSparkleEmitter->AddSimpleParticle( hMaterial, origin );

		if ( pParticle == NULL )
			return;

		// Set our sizes
		pParticle->m_uchStartSize = (unsigned char) scale;
		pParticle->m_uchEndSize = 0;

		// Set our roll
		pParticle->m_flRoll = random->RandomFloat( 0, 2*M_PI );
		pParticle->m_flRollDelta = random->RandomFloat( -DEG2RAD( 180 ), DEG2RAD( 180 ) );

		// Set our color
		pParticle->m_uchColor[0] = 255;	// Red
		pParticle->m_uchColor[1] = 255;	// Green
		pParticle->m_uchColor[2] = 255;	// Blue

		// Set our alpha
		pParticle->m_uchStartAlpha = 0;
		pParticle->m_uchEndAlpha = 255;
		
		// Create a random vector
		Vector velocity = RandomVector( -1.0f, 1.0f );
		VectorNormalize( velocity );

		// Find a random speed for the particle
		float speed = random->RandomFloat( 4.0f, 8.0f ) * scale;

		// Build and set the velocity of the particle
		pParticle->m_vecVelocity = velocity * speed;

		// Declare our lifetime
		pParticle->m_flDieTime = 1.0f;
	}
}

// This links our server-side call to a client-side function
DECLARE_CLIENT_EFFECT( "Sparkle", SparkleCallback );
