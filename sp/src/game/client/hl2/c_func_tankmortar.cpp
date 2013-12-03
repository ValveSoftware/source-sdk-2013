//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "fx_quad.h"
#include "fx.h"

class C_MortarShell : public C_BaseEntity
{
public:

	DECLARE_CLASS( C_MortarShell, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	void OnDataChanged( DataUpdateType_t updateType );
	int DrawModel( int flags );
	
	RenderGroup_t	GetRenderGroup( void )	{ return RENDER_GROUP_TRANSLUCENT_ENTITY; }

private:

	void AddRisingParticles( float flPerc );
	void AddExplodingParticles( float flPerc );

	inline float GetStartPerc( void );
	inline float GetEndPerc( void );

	CSmartPtr<CSimpleEmitter> m_pEmitter;
	TimedEvent	m_ParticleEvent;

	float	m_flLifespan;
	float	m_flRadius;
	float	m_flStarttime;
	Vector	m_vecSurfaceNormal;
};

IMPLEMENT_CLIENTCLASS_DT( C_MortarShell, DT_MortarShell, CMortarShell )
	RecvPropFloat( RECVINFO( m_flLifespan ) ),
	RecvPropFloat( RECVINFO( m_flRadius ) ),
	RecvPropVector( RECVINFO( m_vecSurfaceNormal ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_MortarShell::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_flStarttime = gpGlobals->curtime;
		AddToLeafSystem( RENDER_GROUP_TRANSLUCENT_ENTITY );

		m_pEmitter = CSimpleEmitter::Create( "C_EntityDissolve" );
		m_pEmitter->SetSortOrigin( GetAbsOrigin() );

		m_ParticleEvent.Init( 128 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flPerc - 
//-----------------------------------------------------------------------------
void C_MortarShell::AddRisingParticles( float flPerc )
{
	SimpleParticle *sParticle;

	Vector offset;
	float radius = m_flRadius * 0.25f * flPerc;

	float val = RemapValClamped( gpGlobals->curtime, m_flStarttime, m_flStarttime + m_flLifespan, 0.0f, 1.0f );

	float flCur = gpGlobals->frametime;

	// Anime ground effects
	while ( m_ParticleEvent.NextEvent( flCur ) )
	{
		offset.x = random->RandomFloat( -radius, radius );
		offset.y = random->RandomFloat( -radius, radius );
		offset.z = random->RandomFloat( -8.0f, 8.0f );

		offset += GetAbsOrigin();

		sParticle = (SimpleParticle *) m_pEmitter->AddParticle( sizeof(SimpleParticle), m_pEmitter->GetPMaterial( "effects/spark" ), offset );
		
		if ( sParticle == NULL )
			return;

		sParticle->m_vecVelocity = Vector( Helper_RandomFloat( -4.0f, 4.0f ), Helper_RandomFloat( -4.0f, 4.0f ), Helper_RandomFloat( 32.0f, 256.0f ) * Bias( val, 0.25f ) );
		
		sParticle->m_uchStartSize	= random->RandomFloat( 4, 8 ) * flPerc;

		sParticle->m_flDieTime = random->RandomFloat( 0.5f, 1.0f );
		
		sParticle->m_flLifetime		= 0.0f;

		sParticle->m_flRoll			= Helper_RandomInt( 0, 360 );

		float alpha = 255 * flPerc;

		sParticle->m_flRollDelta	= Helper_RandomFloat( -8.0f * flPerc, 8.0f * flPerc );
		sParticle->m_uchColor[0]	= alpha;
		sParticle->m_uchColor[1]	= alpha;
		sParticle->m_uchColor[2]	= alpha;
		sParticle->m_uchStartAlpha	= alpha;
		sParticle->m_uchEndAlpha	= 0;
		sParticle->m_uchEndSize		= 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flPerc - 
//-----------------------------------------------------------------------------
void C_MortarShell::AddExplodingParticles( float flPerc )
{
	SimpleParticle *sParticle;

	Vector offset;
	float radius = 48.0f * flPerc;

	float flCur = gpGlobals->frametime;

	// Anime ground effects
	while ( m_ParticleEvent.NextEvent( flCur ) )
	{
		offset.x = random->RandomFloat( -radius, radius );
		offset.y = random->RandomFloat( -radius, radius );
		offset.z = random->RandomFloat( -8.0f, 8.0f );

		offset += GetAbsOrigin();

		sParticle = (SimpleParticle *) m_pEmitter->AddParticle( sizeof(SimpleParticle), m_pEmitter->GetPMaterial( "effects/spark" ), offset );
		
		if ( sParticle == NULL )
			return;

		sParticle->m_vecVelocity = RandomVector( -1.0f, 1.0f ) + Vector( 0, 0, 1 );
		sParticle->m_vecVelocity *= ( 750.0f * flPerc );
		
		sParticle->m_uchStartSize	= random->RandomFloat( 2, 4 ) * flPerc;

		sParticle->m_flDieTime = random->RandomFloat( 0.25f, 0.5f );
		
		sParticle->m_flLifetime		= 0.0f;

		sParticle->m_flRoll			= Helper_RandomInt( 0, 360 );

		float alpha = 255 * flPerc;

		sParticle->m_flRollDelta	= Helper_RandomFloat( -8.0f * flPerc, 8.0f * flPerc );
		sParticle->m_uchColor[0]	= alpha;
		sParticle->m_uchColor[1]	= alpha;
		sParticle->m_uchColor[2]	= alpha;
		sParticle->m_uchStartAlpha	= alpha;
		sParticle->m_uchEndAlpha	= 0;
		sParticle->m_uchEndSize		= 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : inline float
//-----------------------------------------------------------------------------
inline float C_MortarShell::GetStartPerc( void )
{
	float val = RemapValClamped( gpGlobals->curtime, m_flStarttime, m_flStarttime + m_flLifespan, 0.0f, 1.0f );

	return ( Gain( val, 0.2f ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : inline float
//-----------------------------------------------------------------------------
inline float C_MortarShell::GetEndPerc( void )
{
	float val = RemapValClamped( gpGlobals->curtime, m_flStarttime + m_flLifespan, m_flStarttime + m_flLifespan + 1.0f, 1.0f, 0.0f );

	return ( Gain( val, 0.75f ) );
}

#define	ALPHA_MIN	0.0f
#define	ALPHA_MAX	1.0f

#define	SCALE_MIN	8
#define	SCALE_MAX	200

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_MortarShell::DrawModel( int flags )
{
	if ( gpGlobals->frametime <= 0.0f )
		return 0;

	float flPerc;
	bool ending;

	// See if we're in the beginning phase
	if ( gpGlobals->curtime < ( m_flStarttime + m_flLifespan ) )
	{
		flPerc = GetStartPerc();
		ending = false;
	}
	else
	{
		flPerc = GetEndPerc();
		ending = true;
	}

	float flAlpha = ALPHA_MIN + ( ( ALPHA_MAX - ALPHA_MIN ) * flPerc );
	float flScale = ( ending ) ? m_flRadius : ( (m_flRadius*0.1f)+ ( ( m_flRadius - (m_flRadius*0.1f) ) * flPerc ) );

	// Do the ground effect
	FX_AddQuad( GetAbsOrigin() + ( m_vecSurfaceNormal * 2.0f ), 
				m_vecSurfaceNormal, 
				flScale,
				flScale,
				1.0f, 
				flAlpha,
				flAlpha,
				1.0f,
				0,
				0,
				Vector( 1.0f, 1.0f, 1.0f ),
				0.0001f, 
				"effects/combinemuzzle2_nocull",
				0 );

	if ( !ending )
	{
		// Add extra effects on startup
		AddRisingParticles( flPerc );
	}
	else
	{
		// Add exploding particles after the impact
		AddExplodingParticles( flPerc );
	}

	return 1;
}
