//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Definition for client-side advisor.
//
//=====================================================================================//



#include "cbase.h"
// this file contains the definitions for the message ID constants (eg ADVISOR_MSG_START_BEAM etc)
#include "npc_advisor_shared.h"

#if NPC_ADVISOR_HAS_BEHAVIOR

#include "particles_simple.h"
#include "citadel_effects_shared.h"
#include "particles_attractor.h"
#include "clienteffectprecachesystem.h"
#include "c_te_effect_dispatch.h"

#include "c_ai_basenpc.h"
#include "dlight.h"
#include "iefx.h"


//-----------------------------------------------------------------------------
// Purpose: unpack a networked entity index into a basehandle.
//-----------------------------------------------------------------------------
inline C_BaseEntity *IndexToEntity( int eindex )
{
	return ClientEntityList().GetBaseEntityFromHandle(ClientEntityList().EntIndexToHandle(eindex));
}



#define ADVISOR_ELIGHT_CVARS 1  // enable/disable tuning advisor elight with console variables

#if ADVISOR_ELIGHT_CVARS
ConVar advisor_elight_e("advisor_elight_e","3");
ConVar advisor_elight_rfeet("advisor_elight_rfeet","52");
#endif


/*! Client-side reflection of the advisor class.
 */
class C_NPC_Advisor : public C_AI_BaseNPC
{
	DECLARE_CLASS( C_NPC_Advisor, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

public:
	// Server to client message received
	virtual void	ReceiveMessage( int classID, bf_read &msg );
	virtual void	ClientThink( void );

private:
	/*
	// broken into its own function so I can move it if necesasry
	void Initialize();
	*/

	// start/stop beam particle effect from me to a pelting object
	void StartBeamFX( C_BaseEntity *pOnEntity );
	void StopBeamFX( C_BaseEntity *pOnEntity );

	void StartElight();
	void StopElight();

	int m_ElightKey; // test using an elight to make the escape sequence more visible. 0 is invalid.

};

IMPLEMENT_CLIENTCLASS_DT( C_NPC_Advisor, DT_NPC_Advisor, CNPC_Advisor )

END_RECV_TABLE()

// Server to client message received
void C_NPC_Advisor::ReceiveMessage( int classID, bf_read &msg )
{
	if ( classID != GetClientClass()->m_ClassID )
	{
		// message is for subclass
		BaseClass::ReceiveMessage( classID, msg );
		return;
	}

	int messageType = msg.ReadByte();
	switch( messageType )
	{
	case ADVISOR_MSG_START_BEAM:
		{
			int eindex = msg.ReadLong();
			StartBeamFX(IndexToEntity(eindex));
		}
		break;

	case ADVISOR_MSG_STOP_BEAM:
		{
			int eindex = msg.ReadLong();
			StopBeamFX(IndexToEntity(eindex));

		}
		break;

	case ADVISOR_MSG_STOP_ALL_BEAMS:
		{
			ParticleProp()->StopEmission();
		}
		break;
	case ADVISOR_MSG_START_ELIGHT:
		{
			StartElight();
		}
		break;
	case ADVISOR_MSG_STOP_ELIGHT:
		{
			StopElight();
		}
		break;

	default:
		AssertMsg1( false, "Received unknown message %d", messageType);
	}
}

/// only use of the clientthink on the advisor is to update the elight
void C_NPC_Advisor::ClientThink( void )
{
	// if the elight has gone away, bail out
	if (m_ElightKey == 0)
	{
		SetNextClientThink( CLIENT_THINK_NEVER );
		return;
	}

	// get the elight
	dlight_t * el = effects->GetElightByKey(m_ElightKey);
	if (!el)
	{
		// the elight has been invalidated. bail out.
		m_ElightKey = 0;

		SetNextClientThink( CLIENT_THINK_NEVER );
		return;
	}
	else
	{
		el->origin = WorldSpaceCenter();

#if ADVISOR_ELIGHT_CVARS
		el->color.exponent = advisor_elight_e.GetFloat();
		el->radius = advisor_elight_rfeet.GetFloat() * 12.0f;
#endif
	}
}

//-----------------------------------------------------------------------------
// Create a telekinetic beam effect from my head to an object
// TODO: use a point attachment.
//-----------------------------------------------------------------------------
void C_NPC_Advisor::StartBeamFX( C_BaseEntity *pOnEntity )
{
	Assert(pOnEntity);
	if (!pOnEntity)
		return;

	CNewParticleEffect *pEffect = ParticleProp()->Create( "Advisor_Psychic_Beam", PATTACH_ABSORIGIN_FOLLOW );

	Assert(pEffect); 
	if (!pEffect) return;

	ParticleProp()->AddControlPoint( pEffect, 1, pOnEntity, PATTACH_ABSORIGIN_FOLLOW );
}


//-----------------------------------------------------------------------------
// terminate a telekinetic beam effect from my head to an object
//-----------------------------------------------------------------------------
void C_NPC_Advisor::StopBeamFX( C_BaseEntity *pOnEntity )
{
	Assert(pOnEntity);
	if (!pOnEntity)
		return;

	ParticleProp()->StopParticlesInvolving( pOnEntity );
}






void C_NPC_Advisor::StartElight()
{
	AssertMsg(m_ElightKey == 0 , "Advisor trying to create new elight on top of old one!");
	if ( m_ElightKey != 0 )
	{
		Warning("Advisor tried to start his elight when it was already one.\n");
	}
	else
	{
		m_ElightKey = LIGHT_INDEX_TE_DYNAMIC + this->entindex();
		dlight_t * el = effects->CL_AllocElight( m_ElightKey );

		if ( el )
		{
			// create an elight on top of me
			el->origin	= this->WorldSpaceCenter();

			el->color.r = 235;
			el->color.g = 255;
			el->color.b = 255;
			el->color.exponent = 3;

			el->radius	= 52*12;
			el->decay	= 0.0f;
			el->die = gpGlobals->curtime + 2000.0f; // 1000 just means " a long time "

			SetNextClientThink( CLIENT_THINK_ALWAYS );
		}
		else
		{	// null out the light value
			m_ElightKey = 0;
		}
	}
}

void C_NPC_Advisor::StopElight()
{
	AssertMsg( m_ElightKey != 0, "Advisor tried to stop elight when none existed!");
	dlight_t * el;
	// note: the following conditional sets el if not short-circuited
	if ( m_ElightKey == 0 || (el = effects->GetElightByKey(m_ElightKey)) == NULL ) 
	{
		Warning("Advisor tried to stop its elight when it had none.\n");
	}
	else
	{
		// kill the elight by setting the die value to now
		el->die = gpGlobals->curtime;
	}
}


#endif

/******************************************************
 * Tenser, said the Tensor.                           *
 * Tenser, said the Tensor.                           *
 * Tension, apprehension and dissension have begun.   *
 ******************************************************/
