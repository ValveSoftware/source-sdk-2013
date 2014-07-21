//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "baseanimating.h"
#include "SkyCamera.h"
#include "studio.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// HACK HACK:  Must match cl_dll/cl_animevent.h!!!!
#define CL_EVENT_SPRITEGROUP_CREATE		6002

//-----------------------------------------------------------------------------
// An entity which emits other entities at points 
//-----------------------------------------------------------------------------
class CEnvParticleScript : public CBaseAnimating
{
public:
	DECLARE_CLASS( CEnvParticleScript, CBaseAnimating );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CEnvParticleScript();

	virtual void Precache();
	virtual void Spawn();
	virtual void Activate();
	virtual int  UpdateTransmitState();

	void InputSetSequence( inputdata_t &inputdata );

private:

	void	PrecacheAnimationEventMaterials();

	CNetworkVar( float, m_flSequenceScale );
};


//-----------------------------------------------------------------------------
// Save/load 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CEnvParticleScript )

	DEFINE_FIELD( m_flSequenceScale, FIELD_FLOAT ),
	
	// Inputs
	DEFINE_INPUTFUNC( FIELD_STRING, "SetSequence", InputSetSequence ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( env_particlescript, CEnvParticleScript );


//-----------------------------------------------------------------------------
// Datatable
//-----------------------------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST( CEnvParticleScript, DT_EnvParticleScript )
	SendPropFloat(SENDINFO(m_flSequenceScale), 0, SPROP_NOSCALE),
END_SEND_TABLE()


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CEnvParticleScript::CEnvParticleScript()
{
	UseClientSideAnimation();
}


void CEnvParticleScript::PrecacheAnimationEventMaterials()
{
	CStudioHdr *hdr = GetModelPtr();
	if ( hdr )
	{
		int numseq = hdr->GetNumSeq();
		for ( int i = 0; i < numseq; ++i )
		{
			mstudioseqdesc_t& seqdesc = hdr->pSeqdesc( i );
			int ecount = seqdesc.numevents;
			for ( int j = 0 ; j < ecount; ++j )
			{
				const mstudioevent_t* event = seqdesc.pEvent( j );
				if ( event->event == CL_EVENT_SPRITEGROUP_CREATE )
				{
					char pAttachmentName[256];
					char pSpriteName[256];
					int nArgs = sscanf( event->pszOptions(), "%255s %255s", pAttachmentName, pSpriteName );
					if ( nArgs == 2 )
					{
						PrecacheMaterial( pSpriteName );
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Precache
//-----------------------------------------------------------------------------
void CEnvParticleScript::Precache()
{
	BaseClass::Precache();
	PrecacheModel( STRING( GetModelName() ) );
	
	// We need a model for its animation sequences even though we don't render it
	SetModel( STRING( GetModelName() ) );

	PrecacheAnimationEventMaterials();
}


//-----------------------------------------------------------------------------
// Spawn
//-----------------------------------------------------------------------------
void CEnvParticleScript::Spawn()
{
	Precache();
	BaseClass::Spawn();
	AddEffects( EF_NOSHADOW );
	// We need a model for its animation sequences even though we don't render it
	SetModel( STRING( GetModelName() ) );
}


//-----------------------------------------------------------------------------
// Activate
//-----------------------------------------------------------------------------
void CEnvParticleScript::Activate()
{
	BaseClass::Activate();

	DetectInSkybox();
	CSkyCamera *pCamera = GetEntitySkybox();
	if ( pCamera )
	{
		float flSkyboxScale = pCamera->m_skyboxData.scale;
		if ( flSkyboxScale == 0.0f )
		{
			flSkyboxScale = 1.0f;
		}

		m_flSequenceScale = flSkyboxScale;
	}
	else
	{
		m_flSequenceScale = 1.0f;
	}

	m_flPlaybackRate = 1.0f;
}

//-----------------------------------------------------------------------------
// Should we transmit it to the client?
//-----------------------------------------------------------------------------
int CEnvParticleScript::UpdateTransmitState()
{
	if ( IsEffectActive( EF_NODRAW ) )
	{
		return SetTransmitState( FL_EDICT_DONTSEND );
	}

	if ( IsEFlagSet( EFL_IN_SKYBOX ) )
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	return SetTransmitState( FL_EDICT_PVSCHECK );
}


//-----------------------------------------------------------------------------
// Purpose: Input that sets the sequence of the entity
//-----------------------------------------------------------------------------
void CEnvParticleScript::InputSetSequence( inputdata_t &inputdata )
{
	if ( inputdata.value.StringID() != NULL_STRING )
	{
		int nSequence = LookupSequence( STRING( inputdata.value.StringID() ) );
		if ( nSequence != ACT_INVALID )
		{
			SetSequence( nSequence );
		}
	}
}
