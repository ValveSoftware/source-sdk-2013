//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "particles_simple.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_WaterBullet : public C_BaseAnimating
{
public:

	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_WaterBullet, C_BaseAnimating );

	C_WaterBullet( void ) {};
	~C_WaterBullet( void ) {};

	void OnDataChanged( DataUpdateType_t updateType )
	{
		BaseClass::OnDataChanged( updateType );

		if ( updateType == DATA_UPDATE_CREATED )
		{
			m_pEmitter = CSimpleEmitter::Create( "FX_Bubble" );
			m_pEmitter->SetSortOrigin( GetAbsOrigin() );

			m_vecLastOrigin = GetAbsOrigin();
		}
	}

#define	BUBBLES_PER_INCH	0.2

	void AddEntity( void )
	{
		Vector	direction = GetAbsOrigin() - m_vecLastOrigin;
		float	flDist = VectorNormalize( direction );

		int	numBubbles = (int) ( flDist * BUBBLES_PER_INCH );

		if ( numBubbles < 1 )
			numBubbles = 1;

		// Make bubbles
		SimpleParticle *sParticle;

		Vector	offset;

		for ( int i = 0; i < numBubbles; i++ )
		{
			offset = m_vecLastOrigin + ( direction * ( flDist / numBubbles ) * i ) + RandomVector( -2.5f, 2.5f );

			sParticle = (SimpleParticle *) m_pEmitter->AddParticle( sizeof(SimpleParticle), m_pEmitter->GetPMaterial( "effects/bubble" ), offset );

			if ( sParticle )
			{
				sParticle->m_flLifetime		= 0.0f;
				sParticle->m_flDieTime		= random->RandomFloat( 0.75f, 1.25f );

				sParticle->m_flRoll			= 0;
				sParticle->m_flRollDelta	= 0;

				unsigned char color = random->RandomInt( 128, 255 );

				sParticle->m_uchColor[0]	= color;
				sParticle->m_uchColor[1]	= color;
				sParticle->m_uchColor[2]	= color;
				sParticle->m_uchStartAlpha	= 255;
				sParticle->m_uchEndAlpha	= 0;
				sParticle->m_uchStartSize	= random->RandomInt( 1, 2 );
				sParticle->m_uchEndSize		= sParticle->m_uchStartSize;
				
				sParticle->m_vecVelocity	= ( direction * 64.0f ) + Vector( 0, 0, 32 );
			}
			
			sParticle = (SimpleParticle *) m_pEmitter->AddParticle( sizeof(SimpleParticle), m_pEmitter->GetPMaterial( "effects/splash2" ), offset );

			if ( sParticle )
			{
				sParticle->m_flLifetime		= 0.0f;
				sParticle->m_flDieTime		= 0.2f;

				sParticle->m_flRoll			= random->RandomInt( 0, 360 );
				sParticle->m_flRollDelta	= random->RandomInt( -4, 4 );

				unsigned char color = random->RandomInt( 200, 255 );

				sParticle->m_uchColor[0]	= color;
				sParticle->m_uchColor[1]	= color;
				sParticle->m_uchColor[2]	= color;
				sParticle->m_uchStartAlpha	= 128;
				sParticle->m_uchEndAlpha	= 0;
				sParticle->m_uchStartSize	= 2;
				sParticle->m_uchEndSize		= sParticle->m_uchStartSize * 4;
				
				sParticle->m_vecVelocity	= ( direction * 64.0f ) + Vector( 0, 0, 32 );
			}
		}

		// Save our last position
		m_vecLastOrigin = GetAbsOrigin();

		BaseClass::AddEntity();
	}

	bool ShouldDraw( void ) { return true; }

private:
	C_WaterBullet( const C_WaterBullet & );

	CSmartPtr<CSimpleEmitter> m_pEmitter;

	Vector		m_vecLastOrigin;
};

IMPLEMENT_CLIENTCLASS_DT( C_WaterBullet, DT_WaterBullet, CWaterBullet )
END_RECV_TABLE()
