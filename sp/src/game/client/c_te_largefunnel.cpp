//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_te_particlesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Large Funnel TE
//-----------------------------------------------------------------------------
class C_TELargeFunnel : public C_TEParticleSystem
{
public:
	DECLARE_CLASS( C_TELargeFunnel, C_TEParticleSystem );
	DECLARE_CLIENTCLASS();

					C_TELargeFunnel( void );
	virtual			~C_TELargeFunnel( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );


public:
	void			CreateFunnel( void );

	int				m_nModelIndex;
	int				m_nReversed;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TELargeFunnel::C_TELargeFunnel( void )
{
	m_nModelIndex = 0;
	m_nReversed = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TELargeFunnel::~C_TELargeFunnel( void )
{
}


void C_TELargeFunnel::CreateFunnel( void )
{
	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "TELargeFunnel" );
	pSimple->SetSortOrigin( m_vecOrigin );

	int			i, j;
	SimpleParticle *pParticle;

	Vector		vecDir;
	Vector		vecDest;

	float ratio = 0.25;
	float invratio = 1 / ratio;

	PMaterialHandle hMaterial = pSimple->GetPMaterial( "sprites/flare6" );

	for ( i = -256 ; i <= 256 ; i += 24 )	//24 from 32.. little more dense
	{
		for ( j = -256 ; j <= 256 ; j += 24 )
		{
			pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), hMaterial, m_vecOrigin );			
			if( pParticle )
			{
				if ( m_nReversed )
				{
					pParticle->m_Pos = m_vecOrigin;

					vecDir[0] = i;
					vecDir[1] = j;
					vecDir[2] = random->RandomFloat(100, 800);

					pParticle->m_uchStartAlpha	= 255;
					pParticle->m_uchEndAlpha	= 0;
				}
				else
				{
					pParticle->m_Pos[0] = m_vecOrigin[0] + i;
					pParticle->m_Pos[1] = m_vecOrigin[1] + j;
					pParticle->m_Pos[2] = m_vecOrigin[2] + random->RandomFloat(100, 800);

					// send particle heading to org at a random speed
					vecDir = m_vecOrigin - pParticle->m_Pos;

					pParticle->m_uchStartAlpha	= 0;
					pParticle->m_uchEndAlpha	= 255;
				}

				vecDir *= ratio;

				pParticle->m_vecVelocity = vecDir;			

				pParticle->m_flLifetime = 0;
				pParticle->m_flDieTime = invratio;	

				if( random->RandomInt( 0, 10 ) < 5 )
				{
					// small green particle
					pParticle->m_uchColor[0] = 0;
					pParticle->m_uchColor[1] = 255;
					pParticle->m_uchColor[2] = 0;
				
					pParticle->m_uchStartSize	= 4.0;
				}
				else
				{
					// large white particle
					pParticle->m_uchColor[0] = 255;
					pParticle->m_uchColor[1] = 255;
					pParticle->m_uchColor[2] = 255;
				
					pParticle->m_uchStartSize	= 15.0;
				}

				pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
				pParticle->m_flRoll			= i;	// pseudorandom
				pParticle->m_flRollDelta	= 0;
				pParticle->m_iFlags = 0;
			}

		}
	}

	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TELargeFunnel::PostDataUpdate( DataUpdateType_t updateType )
{
	CreateFunnel();
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TELargeFunnel, DT_TELargeFunnel, CTELargeFunnel)
	RecvPropInt( RECVINFO(m_nModelIndex)),
	RecvPropInt( RECVINFO(m_nReversed)),
END_RECV_TABLE()

void TE_LargeFunnel( IRecipientFilter& filter, float delay,
	const Vector* pos, int modelindex, int reversed )
{
	// Major hack to simulate receiving network message
	__g_C_TELargeFunnel.m_vecOrigin = *pos;
	__g_C_TELargeFunnel.m_nModelIndex = modelindex;
	__g_C_TELargeFunnel.m_nReversed = reversed;

	__g_C_TELargeFunnel.PostDataUpdate( DATA_UPDATE_CREATED );
}


