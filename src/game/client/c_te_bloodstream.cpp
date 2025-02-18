//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "c_te_particlesystem.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Purpose: Blood Stream TE
//-----------------------------------------------------------------------------
class C_TEBloodStream : public C_TEParticleSystem
{
public:
	DECLARE_CLASS( C_TEBloodStream, C_TEParticleSystem );
	DECLARE_CLIENTCLASS();

					C_TEBloodStream( void );
	virtual			~C_TEBloodStream( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	Vector			m_vecDirection;
	int				r, g, b, a;
	int				m_nAmount;
};


//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEBloodStream, DT_TEBloodStream, CTEBloodStream)
	RecvPropVector( RECVINFO(m_vecDirection)),
	RecvPropInt( RECVINFO(r)),
	RecvPropInt( RECVINFO(g)),
	RecvPropInt( RECVINFO(b)),
	RecvPropInt( RECVINFO(a)),
	RecvPropInt( RECVINFO(m_nAmount)),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBloodStream::C_TEBloodStream( void )
{
	m_vecOrigin.Init();
	m_vecDirection.Init();
	r = g = b = a = 0;
	m_nAmount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBloodStream::~C_TEBloodStream( void )
{
}


//-----------------------------------------------------------------------------
// Recording 
//-----------------------------------------------------------------------------
static inline void RecordBloodStream( const Vector &start, const Vector &direction, 
	int r, int g, int b, int a, int amount )
{
	if ( !ToolsEnabled() )
		return;

	if ( clienttools->IsInRecordingMode() )
	{
		Color clr( r, g, b, a );

		KeyValues *msg = new KeyValues( "TempEntity" );

 		msg->SetInt( "te", TE_BLOOD_STREAM );
 		msg->SetString( "name", "TE_BloodStream" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetFloat( "originx", start.x );
		msg->SetFloat( "originy", start.y );
		msg->SetFloat( "originz", start.z );
		msg->SetFloat( "directionx", direction.x );
		msg->SetFloat( "directiony", direction.y );
		msg->SetFloat( "directionz", direction.z );
		msg->SetColor( "color", clr );
		msg->SetInt( "amount", amount );

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
}


void TE_BloodStream( IRecipientFilter& filter, float delay,
	const Vector* org, const Vector* direction, int r, int g, int b, int a, int amount )
{
	RecordBloodStream( *org, *direction, r, g, b, a, amount );

	CSmartPtr<CTEParticleRenderer> pRen = CTEParticleRenderer::Create( "TEBloodStream", *org );
	if( !pRen )
		return;

	// Add our particles.
	Vector		dirCopy;
	float		arc = 0.05;
	int			count, count2;
	float		num;
	float		speedCopy = amount;
	
	Vector dir;
	VectorCopy( *direction, dir );
	VectorNormalize( dir );
	
	for (count=0 ; count<100 ; count++)
	{
		StandardParticle_t *p = pRen->AddParticle();
		if(p)
		{
			p->SetColor(r * random->RandomFloat(0.7, 1.0), g, b);
			p->SetAlpha(a);
			p->m_Pos = *org;
			pRen->SetParticleLifetime(p, 2);
			pRen->SetParticleType(p, pt_vox_grav);
			
			VectorCopy (dir, dirCopy);
			
			dirCopy[2] -= arc;
			arc -= 0.005;
			
			VectorScale (dirCopy, speedCopy, p->m_Velocity);
			
			speedCopy -= 0.00001;// so last few will drip
		}
	}
	
	// now a few rogue voxels
	arc = 0.075;
	for (count = 0 ; count < (amount/5); count ++)
	{
		StandardParticle_t *p = pRen->AddParticle();
		if(p)
		{
			pRen->SetParticleLifetime(p, 3);
			p->SetColor(r * random->RandomFloat(0.7, 1.0), g, b);
			p->SetAlpha(a);
			p->m_Pos = *org;
			pRen->SetParticleType(p, pt_vox_slowgrav);
			
			VectorCopy (dir, dirCopy);
			
			dirCopy[2] -= arc;
			arc -= 0.005;
			
			num = random->RandomFloat(0,1);
			speedCopy = amount * num;
			
			num *= 1.7;
			
			VectorScale (dirCopy, num, dirCopy);// randomize a bit
			p->m_Velocity = dirCopy * speedCopy;
			
			
			// add a few extra voxels directly adjacent to this one to give a 
			// 'chunkier' appearance.
			for (count2 = 0; count2 < 2; count2++)
			{
				StandardParticle_t *pChunky = pRen->AddParticle();
				if( pChunky )
				{
					pRen->SetParticleLifetime( pChunky, 3);
					pChunky->SetColor(random->RandomFloat(0.7, 1.0), g, b);
					pChunky->SetAlpha(a);
					pChunky->m_Pos.Init(
						(*org)[0] + random->RandomFloat(-1,1),
						(*org)[1] + random->RandomFloat(-1,1),
						(*org)[2] + random->RandomFloat(-1,1));
					
					pRen->SetParticleType( pChunky, pt_vox_slowgrav);
					
					VectorCopy (dir, dirCopy);
					
					dirCopy[2] -= arc;
					
					VectorScale (dirCopy, num, dirCopy);// randomize a bit
					
					pChunky->m_Velocity = dirCopy * speedCopy;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEBloodStream::PostDataUpdate( DataUpdateType_t updateType )
{
	CBroadcastRecipientFilter filter;
	TE_BloodStream( filter, 0.0f, &m_vecOrigin, &m_vecDirection, r, g, b, a, m_nAmount );
}

void TE_BloodStream( IRecipientFilter& filter, float delay, KeyValues *pKeyValues )
{
	Vector vecOrigin, vecDirection;
	vecOrigin.x = pKeyValues->GetFloat( "originx" );
	vecOrigin.y = pKeyValues->GetFloat( "originy" );
	vecOrigin.z = pKeyValues->GetFloat( "originz" );
	vecDirection.x = pKeyValues->GetFloat( "directionx" );
	vecDirection.y = pKeyValues->GetFloat( "directiony" );
	vecDirection.z = pKeyValues->GetFloat( "directionz" );
	Color c = pKeyValues->GetColor( "color" );
	int nAmount = pKeyValues->GetInt( "amount" );
	TE_BloodStream( filter, 0.0f, &vecOrigin, &vecDirection, c.r(), c.g(), c.b(), c.a(), nAmount );
}
