//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Spawn and use functions for editor-placed triggers.
//
//=============================================================================//

#include "cbase.h"
#include "triggers.h"
#include "soundenvelope.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	sk_max_super_armor( "sk_max_super_armor","500");

#define MAX_SUPER_ARMOR		sk_max_super_armor.GetInt()	


//-----------------------------------------------------------------------------
// Trigger that bestows super armor
//-----------------------------------------------------------------------------
class CTriggerSuperArmor : public CTriggerMultiple
{
	DECLARE_CLASS( CTriggerSuperArmor, CTriggerMultiple );
	DECLARE_DATADESC();

public:

	virtual void StartTouch( CBaseEntity *pOther );
	virtual void EndTouch( CBaseEntity *pOther );
	
private:

	virtual void Precache();
	virtual void Spawn( void );
	void StartLoopingSounds( CBaseEntity *pEntity );
	void StopLoopingSounds();
	void RechargeThink();

	CSoundPatch *m_pChargingSound;
	float m_flLoopingSoundTime;
};


LINK_ENTITY_TO_CLASS( trigger_super_armor, CTriggerSuperArmor );

BEGIN_DATADESC( CTriggerSuperArmor )

	DEFINE_SOUNDPATCH( m_pChargingSound ),
	DEFINE_FIELD( m_flLoopingSoundTime, FIELD_TIME ),
	
	DEFINE_THINKFUNC( RechargeThink ),

END_DATADESC()


static const char *s_pRechargeThinkContext = "RechargeThink";


void CTriggerSuperAmmor::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "TriggerSuperArmor.StartCharging" );
	PrecacheScriptSound( "TriggerSuperArmor.DoneCharging" );

	PrecacheScriptSound( "TriggerSuperArmor.Charging" );
}

//-----------------------------------------------------------------------------
// No retrigger
//-----------------------------------------------------------------------------
void CTriggerSuperArmor::Spawn( void )
{
	Precache();

	BaseClass::Spawn();
}


//-----------------------------------------------------------------------------
// Starts looping sounds
//-----------------------------------------------------------------------------
void CTriggerSuperArmor::StartLoopingSounds( CBaseEntity *pEntity )
{
	if ( m_pChargingSound )
		return;

	CReliableBroadcastRecipientFilter filter;
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	m_pChargingSound = controller.SoundCreate( filter, pEntity->entindex(), "TriggerSuperArmor.Charging" );
}


//-----------------------------------------------------------------------------
// Stops looping sounds
//-----------------------------------------------------------------------------
void CTriggerSuperArmor::StopLoopingSounds()
{
	if ( m_pChargingSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundDestroy( m_pChargingSound );
		m_pChargingSound = NULL;
	}

	BaseClass::StopLoopingSounds();
}

	
//-----------------------------------------------------------------------------
// Begins super-powering the entities
//-----------------------------------------------------------------------------
void CTriggerSuperArmor::StartTouch( CBaseEntity *pOther )
{
	BaseClass::StartTouch( pOther );

	if ( !pOther->IsPlayer() )
		return;

	if ( m_hTouchingEntities.Count() == 1 )
	{
		SetContextThink( RechargeThink, gpGlobals->curtime + 0.01f, s_pRechargeThinkContext );
		pOther->EmitSound( "TriggerSuperArmor.StartCharging" );
		m_flLoopingSoundTime = 0.56f + gpGlobals->curtime;
	}
}


//-----------------------------------------------------------------------------
// Ends super-powerings the entities
//-----------------------------------------------------------------------------
void CTriggerSuperArmor::EndTouch( CBaseEntity *pOther )
{
	BaseClass::EndTouch( pOther );

	if ( !pOther->IsPlayer() )
		return;

	if ( m_hTouchingEntities.Count() == 0 )
	{
		StopLoopingSounds();
		SetContextThink( NULL, gpGlobals->curtime + 0.01f, s_pRechargeThinkContext );
	}
}


//-----------------------------------------------------------------------------
// Super-powers the entities
//-----------------------------------------------------------------------------
void CTriggerSuperArmor::RechargeThink()
{
	Assert( m_hTouchingEntities.Count() == 1 );
	for ( int i = m_hTouchingEntities.Count(); --i >= 0; )
	{
		CBasePlayer *pPlayer = assert_cast<CBasePlayer*>( m_hTouchingEntities[i].Get() );

		if (( pPlayer->ArmorValue() < MAX_SUPER_ARMOR ) || ( pPlayer->GetHealth() < 100 ))
		{
			pPlayer->TakeHealth( 5, DMG_GENERIC );
			pPlayer->IncrementArmorValue( 15, MAX_SUPER_ARMOR );

			if ( pPlayer->ArmorValue() >= MAX_SUPER_ARMOR )
			{
				pPlayer->EmitSound( "TriggerSuperArmor.DoneCharging" );
				StopLoopingSounds();
			}
			else
			{
				if ( m_flLoopingSoundTime < gpGlobals->curtime )
				{
					StartLoopingSounds( m_hTouchingEntities[i] );
				}
			}
		}
	}

	SetContextThink( RechargeThink, gpGlobals->curtime + 0.1f, s_pRechargeThinkContext );
}