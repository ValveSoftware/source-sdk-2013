//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Client side antlion guard. Used to create dlight for the cave guard.
//
//=============================================================================

#include "cbase.h"
#include "c_ai_basenpc.h"
#include "dlight.h"
#include "iefx.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#if HL2_EPISODIC
// When enabled, add code to have the antlion bleed profusely as it is badly injured.
#define ANTLIONGUARD_BLOOD_EFFECTS 2
#endif


class C_NPC_AntlionGuard : public C_AI_BaseNPC
{
public:
	C_NPC_AntlionGuard() {}

	DECLARE_CLASS( C_NPC_AntlionGuard, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();
 	DECLARE_DATADESC();

	virtual void OnDataChanged( DataUpdateType_t type );
	virtual void ClientThink();

private:

	bool m_bCavernBreed;
	bool m_bInCavern;
	dlight_t *m_dlight;

#if HL2_EPISODIC
	unsigned char m_iBleedingLevel; //< the version coming from the server
	unsigned char m_iPerformingBleedingLevel; //< the version we're currently performing (for comparison to one above)
	CNewParticleEffect *m_pBleedingFX;

	/// update the hemorrhage particle effect
	virtual void UpdateBleedingPerformance( void );
#endif

	C_NPC_AntlionGuard( const C_NPC_AntlionGuard & );
};


//-----------------------------------------------------------------------------
// Save/restore
//-----------------------------------------------------------------------------
BEGIN_DATADESC( C_NPC_AntlionGuard )
END_DATADESC()


//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT(C_NPC_AntlionGuard, DT_NPC_AntlionGuard, CNPC_AntlionGuard)
	RecvPropBool( RECVINFO( m_bCavernBreed ) ),
	RecvPropBool( RECVINFO( m_bInCavern ) ),

#if ANTLIONGUARD_BLOOD_EFFECTS
	RecvPropInt(  RECVINFO( m_iBleedingLevel ) ),
#endif
END_RECV_TABLE()


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_NPC_AntlionGuard::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( (type == DATA_UPDATE_CREATED) && m_bCavernBreed && m_bInCavern )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}


#if HL2_EPISODIC
	if (m_iBleedingLevel != m_iPerformingBleedingLevel)
	{
		UpdateBleedingPerformance();
	}
#endif

}

#if HL2_EPISODIC
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_NPC_AntlionGuard::UpdateBleedingPerformance()
{
	// get my particles
	CParticleProperty * pProp = ParticleProp();

	// squelch the prior effect if it exists
	if (m_pBleedingFX)
	{
		pProp->StopEmission(m_pBleedingFX);
		m_pBleedingFX = NULL;
	}

	// kick off a new effect
	switch (m_iBleedingLevel)
	{
	case 1: // light bleeding
		{
			m_pBleedingFX = pProp->Create( "blood_antlionguard_injured_light", PATTACH_ABSORIGIN_FOLLOW );
			AssertMsg1( m_pBleedingFX, "Particle system couldn't make %s", "blood_antlionguard_injured_light" );
			if ( m_pBleedingFX )
			{
				pProp->AddControlPoint( m_pBleedingFX, 1, this, PATTACH_ABSORIGIN_FOLLOW );
			}
		}
		break;

	case 2: // severe bleeding
		{
			m_pBleedingFX = pProp->Create( "blood_antlionguard_injured_heavy", PATTACH_ABSORIGIN_FOLLOW );
			AssertMsg1( m_pBleedingFX, "Particle system couldn't make %s", "blood_antlionguard_injured_heavy" );
			if ( m_pBleedingFX )
			{
				pProp->AddControlPoint( m_pBleedingFX, 1, this, PATTACH_ABSORIGIN_FOLLOW );
			}

		}
		break;
	}

	m_iPerformingBleedingLevel = m_iBleedingLevel;
}
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_NPC_AntlionGuard::ClientThink()
{
	// update the dlight. (always done because clienthink only exists for cavernguard)
	if (!m_dlight)
	{
		m_dlight = effects->CL_AllocDlight( index );
		m_dlight->color.r = 220;
		m_dlight->color.g = 255;
		m_dlight->color.b = 80;
		m_dlight->radius	= 180;
		m_dlight->minlight = 128.0 / 256.0f;
		m_dlight->flags = DLIGHT_NO_MODEL_ILLUMINATION;
	}

	m_dlight->origin	= GetAbsOrigin();
	// dl->die = gpGlobals->curtime + 0.1f;

	BaseClass::ClientThink();
}
