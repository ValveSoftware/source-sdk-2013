//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Flare effects
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "clienteffectprecachesystem.h"
#include "particles_simple.h"
#include "iefx.h"
#include "dlight.h"
#include "view.h"
#include "fx.h"
#include "clientsideeffects.h"
#include "c_pixel_visibility.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//Precahce the effects
CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectFlares )
CLIENTEFFECT_MATERIAL( "effects/redflare" )
CLIENTEFFECT_MATERIAL( "effects/yellowflare" )
CLIENTEFFECT_MATERIAL( "effects/yellowflare_noz" )
CLIENTEFFECT_REGISTER_END()

class C_Flare : public C_BaseCombatCharacter, CSimpleEmitter
{
public:
	DECLARE_CLASS( C_Flare, C_BaseCombatCharacter );
	DECLARE_CLIENTCLASS();

	C_Flare();

	void	OnDataChanged( DataUpdateType_t updateType );
	void	Update( float timeDelta );
	void	NotifyDestroyParticle( Particle* pParticle );
	void	NotifyShouldTransmit( ShouldTransmitState_t state );
	void	RestoreResources( void );

	float	m_flTimeBurnOut;
	float	m_flScale;
	bool	m_bLight;
	bool	m_bSmoke;
	bool	m_bPropFlare;
	pixelvis_handle_t m_queryHandle;


private:
	C_Flare( const C_Flare & );
	TimedEvent	m_teSmokeSpawn;

	int		m_iAttachment;
	
	SimpleParticle	*m_pParticle[2];
};

IMPLEMENT_CLIENTCLASS_DT( C_Flare, DT_Flare, CFlare )
	RecvPropFloat( RECVINFO( m_flTimeBurnOut ) ),
	RecvPropFloat( RECVINFO( m_flScale ) ),
	RecvPropInt( RECVINFO( m_bLight ) ),
	RecvPropInt( RECVINFO( m_bSmoke ) ),
	RecvPropInt( RECVINFO( m_bPropFlare ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
C_Flare::C_Flare() : CSimpleEmitter( "C_Flare" )
{
	m_pParticle[0]	= NULL;
	m_pParticle[1]	= NULL;
	m_flTimeBurnOut	= 0.0f;

	m_bLight		= true;
	m_bSmoke		= true;
	m_bPropFlare	= false;

	SetDynamicallyAllocated( false );
	m_queryHandle = 0;

	m_iAttachment = -1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void C_Flare::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	if ( state == SHOULDTRANSMIT_END )
	{
		AddEffects( EF_NODRAW );
	}
	else if ( state == SHOULDTRANSMIT_START )
	{
		RemoveEffects( EF_NODRAW );
	}

	BaseClass::NotifyShouldTransmit( state );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_Flare::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetSortOrigin( GetAbsOrigin() );
		if ( m_bSmoke )
		{
			m_teSmokeSpawn.Init( 8 );
		}
	}

	BaseClass::OnDataChanged( updateType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_Flare::RestoreResources( void )
{
	if ( m_pParticle[0] == NULL )
	{
		m_pParticle[0] = (SimpleParticle *) AddParticle( sizeof( SimpleParticle ), GetPMaterial( "effects/redflare" ), GetAbsOrigin() );
		
		if ( m_pParticle[0] != NULL )
		{
			m_pParticle[0]->m_uchColor[0] = m_pParticle[0]->m_uchColor[1] = m_pParticle[0]->m_uchColor[2] = 0;
			m_pParticle[0]->m_flRoll		= random->RandomInt( 0, 360 );
			m_pParticle[0]->m_flRollDelta	= random->RandomFloat( 1.0f, 4.0f );
			m_pParticle[0]->m_flLifetime	= 0.0f;
			m_pParticle[0]->m_flDieTime		= 10.0f;
		}
		else
		{
			Assert(0);
		}
	}

	if ( m_pParticle[1] == NULL )
	{
		m_pParticle[1] = (SimpleParticle *) AddParticle( sizeof( SimpleParticle ), GetPMaterial( "effects/yellowflare_noz" ), GetAbsOrigin() );
		
		if ( m_pParticle[1] != NULL )
		{
			m_pParticle[1]->m_uchColor[0] = m_pParticle[1]->m_uchColor[1] = m_pParticle[1]->m_uchColor[2] = 0;
			m_pParticle[1]->m_flRoll		= random->RandomInt( 0, 360 );
			m_pParticle[1]->m_flRollDelta	= random->RandomFloat( 1.0f, 4.0f );
			m_pParticle[1]->m_flLifetime	= 0.0f;
			m_pParticle[1]->m_flDieTime		= 10.0f;
		}
		else
		{
			Assert(0);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pParticle - 
//-----------------------------------------------------------------------------
void C_Flare::NotifyDestroyParticle( Particle* pParticle )
{
	if ( pParticle == m_pParticle[0] )
	{
		m_pParticle[0] = NULL;
	}

	if ( pParticle == m_pParticle[1] )
	{
		m_pParticle[1] = NULL;
	}

	CSimpleEmitter::NotifyDestroyParticle( pParticle );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : timeDelta - 
//-----------------------------------------------------------------------------
void C_Flare::Update( float timeDelta )
{
	if ( !IsVisible() )
		return;

	CSimpleEmitter::Update( timeDelta );

	//Make sure our stored resources are up to date
	RestoreResources();

	//Don't do this if the console is down
	if ( timeDelta <= 0.0f )
		return;

	//Check for LOS
	pixelvis_queryparams_t params;
	params.Init(GetAbsOrigin());
	params.proxySize = 8.0f; // Inches
	
	float visible = PixelVisibility_FractionVisible( params, &m_queryHandle );

	float	fColor;
#ifdef HL2_CLIENT_DLL
	float	baseScale = m_flScale;
#else
	// NOTE!!!  This is bigger by a factor of 1.2 to deal with fixing a bug from HL2.  See dlight_t.h
	float	baseScale = m_flScale * 1.2f;
#endif

	//Account for fading out
	if ( ( m_flTimeBurnOut != -1.0f ) && ( ( m_flTimeBurnOut - gpGlobals->curtime ) <= 10.0f ) )
	{
		baseScale *= ( ( m_flTimeBurnOut - gpGlobals->curtime ) / 10.0f );
	}

	bool bVisible = (baseScale < 0.01f || visible == 0.0f) ? false : true;
	//Clamp the scale if vanished
	if ( !bVisible )
	{
		if ( m_pParticle[0] != NULL )
		{	
			m_pParticle[0]->m_flDieTime		= gpGlobals->curtime;
			m_pParticle[0]->m_uchStartSize	= m_pParticle[0]->m_uchEndSize = 0;
			m_pParticle[0]->m_uchColor[0]	= 0;
			m_pParticle[0]->m_uchColor[1]	= 0;
			m_pParticle[0]->m_uchColor[2]	= 0;
		}

		if ( m_pParticle[1] != NULL )
		{	
			m_pParticle[1]->m_flDieTime		= gpGlobals->curtime;
			m_pParticle[1]->m_uchStartSize	= m_pParticle[1]->m_uchEndSize = 0;
			m_pParticle[1]->m_uchColor[0]	= 0;
			m_pParticle[1]->m_uchColor[1]	= 0;
			m_pParticle[1]->m_uchColor[2]	= 0;
		}
	}

	if ( baseScale < 0.01f )
		return;
	//
	// Dynamic light
	//

	if ( m_bLight )
	{
		dlight_t *dl= effects->CL_AllocDlight( index );

		

		if ( m_bPropFlare == false )
		{
			dl->origin	= GetAbsOrigin();
			dl->color.r = 255;
			dl->die		= gpGlobals->curtime + 0.1f;

			dl->radius	= baseScale * random->RandomFloat( 110.0f, 128.0f );
			dl->color.g = dl->color.b = random->RandomInt( 32, 64 );
		}
		else
		{
			if ( m_iAttachment == -1 )
			{
				m_iAttachment =  LookupAttachment( "fuse" );
			}

			if ( m_iAttachment != -1 )
			{
				Vector effect_origin;
				QAngle effect_angles;
				
				GetAttachment( m_iAttachment, effect_origin, effect_angles );

				//Raise the light a little bit away from the flare so it lights it up better.
				dl->origin	= effect_origin + Vector( 0, 0, 4 );
				dl->color.r = 255;
				dl->die		= gpGlobals->curtime + 0.1f;

				dl->radius	= baseScale * random->RandomFloat( 245.0f, 256.0f );
				dl->color.g = dl->color.b = random->RandomInt( 95, 128 );
		
				dlight_t *el= effects->CL_AllocElight( index );

				el->origin	= effect_origin;
				el->color.r = 255;
				el->color.g = dl->color.b = random->RandomInt( 95, 128 );
				el->radius	= baseScale * random->RandomFloat( 260.0f, 290.0f );
				el->die		= gpGlobals->curtime + 0.1f;
			}
		}
	}

	//
	// Smoke
	//

	float dt = timeDelta;

	if ( m_bSmoke )
	{
		while ( m_teSmokeSpawn.NextEvent( dt ) )
		{
			Vector	smokeOrg = GetAbsOrigin();

			Vector	flareScreenDir = ( smokeOrg - MainViewOrigin() );
			VectorNormalize( flareScreenDir );

			smokeOrg = smokeOrg + ( flareScreenDir * 2.0f );
			smokeOrg[2] += baseScale * 4.0f;

			SimpleParticle *sParticle = (SimpleParticle *) AddParticle( sizeof( SimpleParticle ), g_Mat_DustPuff[1], smokeOrg );
				
			if ( sParticle == NULL )
				return;

			sParticle->m_flLifetime		= 0.0f;
			sParticle->m_flDieTime		= 1.0f;
			
			sParticle->m_vecVelocity	= Vector( random->RandomFloat( -16.0f, 16.0f ), random->RandomFloat( -16.0f, 16.0f ), random->RandomFloat( 8.0f, 16.0f ) + 32.0f );

			if ( m_bPropFlare )
			{
				sParticle->m_uchColor[0]	= 255;
				sParticle->m_uchColor[1]	= 100;
				sParticle->m_uchColor[2]	= 100;
			}
			else
			{
				sParticle->m_uchColor[0]	= 255;
				sParticle->m_uchColor[1]	= 48;
				sParticle->m_uchColor[2]	= 48;
			}

			sParticle->m_uchStartAlpha	= random->RandomInt( 64, 90 );
			sParticle->m_uchEndAlpha	= 0;
			sParticle->m_uchStartSize	= random->RandomInt( 2, 4 );
			sParticle->m_uchEndSize		= sParticle->m_uchStartSize * 8.0f;
			sParticle->m_flRoll			= random->RandomInt( 0, 2*M_PI );
			sParticle->m_flRollDelta	= random->RandomFloat( -(M_PI/6.0f), M_PI/6.0f );
		}
	}

	if ( !bVisible )
		return;

	//
	// Outer glow
	//

	Vector	offset;
	
	//Cause the base of the effect to shake
	offset.Random( -0.5f * baseScale, 0.5f * baseScale );
	offset += GetAbsOrigin();

	if ( m_pParticle[0] != NULL )
	{
		m_pParticle[0]->m_Pos			= offset;
		m_pParticle[0]->m_flLifetime	= 0.0f;
		m_pParticle[0]->m_flDieTime		= 2.0f;
		
		m_pParticle[0]->m_vecVelocity.Init();

		fColor = random->RandomInt( 100.0f, 128.0f ) * visible;

		m_pParticle[0]->m_uchColor[0]	= fColor;
		m_pParticle[0]->m_uchColor[1]	= fColor;
		m_pParticle[0]->m_uchColor[2]	= fColor;
		m_pParticle[0]->m_uchStartAlpha	= fColor;
		m_pParticle[0]->m_uchEndAlpha	= fColor;
		m_pParticle[0]->m_uchStartSize	= baseScale * (float) random->RandomInt( 32, 48 );
		m_pParticle[0]->m_uchEndSize	= m_pParticle[0]->m_uchStartSize;
		m_pParticle[0]->m_flRollDelta	= 0.0f;
		
		if ( random->RandomInt( 0, 4 ) == 3 )
		{
			m_pParticle[0]->m_flRoll	+= random->RandomInt( 2, 8 );
		}
	}

	//
	// Inner core
	//

	//Cause the base of the effect to shake
	offset.Random( -1.0f * baseScale, 1.0f * baseScale );
	offset += GetAbsOrigin();

	if ( m_pParticle[1] != NULL )
	{
		m_pParticle[1]->m_Pos			= offset;
		m_pParticle[1]->m_flLifetime	= 0.0f;
		m_pParticle[1]->m_flDieTime		= 2.0f;
		
		m_pParticle[1]->m_vecVelocity.Init();

		fColor = 255 * visible;

		m_pParticle[1]->m_uchColor[0]	= fColor;
		m_pParticle[1]->m_uchColor[1]	= fColor;
		m_pParticle[1]->m_uchColor[2]	= fColor;
		m_pParticle[1]->m_uchStartAlpha	= fColor;
		m_pParticle[1]->m_uchEndAlpha	= fColor;
		m_pParticle[1]->m_uchStartSize	= baseScale * (float) random->RandomInt( 2, 4 );
		m_pParticle[1]->m_uchEndSize	= m_pParticle[0]->m_uchStartSize;
		m_pParticle[1]->m_flRoll		= random->RandomInt( 0, 360 );
	}
}
