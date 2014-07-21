//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
/*

===== h_battery.cpp ========================================================

  battery-related code

*/

#include "cbase.h"
#include "gamerules.h"
#include "player.h"
#include "engine/IEngineSound.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar	sk_suitcharger( "sk_suitcharger","0" );
static ConVar	sk_suitcharger_citadel( "sk_suitcharger_citadel","0" );
static ConVar	sk_suitcharger_citadel_maxarmor( "sk_suitcharger_citadel_maxarmor","0" );

#define SF_CITADEL_RECHARGER	0x2000
#define SF_KLEINER_RECHARGER	0x4000 // Gives only 25 health

class CRecharge : public CBaseToggle
{
public:
	DECLARE_CLASS( CRecharge, CBaseToggle );

	void Spawn( );
	bool CreateVPhysics();
	int DrawDebugTextOverlays(void);
	void Off(void);
	void Recharge(void);
	bool KeyValue( const char *szKeyName, const char *szValue );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return (BaseClass::ObjectCaps() | FCAP_CONTINUOUS_USE); }

private:
	void InputRecharge( inputdata_t &inputdata );
	
	float MaxJuice() const;
	void UpdateJuice( int newJuice );

	DECLARE_DATADESC();

	float	m_flNextCharge; 
	int		m_iReactivate ; // DeathMatch Delay until reactvated
	int		m_iJuice;
	int		m_iOn;			// 0 = off, 1 = startup, 2 = going
	float   m_flSoundTime;
	
	int		m_nState;
	
	COutputFloat m_OutRemainingCharge;
	COutputEvent m_OnHalfEmpty;
	COutputEvent m_OnEmpty;
	COutputEvent m_OnFull;
	COutputEvent m_OnPlayerUse;
};

BEGIN_DATADESC( CRecharge )

	DEFINE_FIELD( m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( m_iReactivate, FIELD_INTEGER),
	DEFINE_FIELD( m_iJuice, FIELD_INTEGER),
	DEFINE_FIELD( m_iOn, FIELD_INTEGER),
	DEFINE_FIELD( m_flSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_nState, FIELD_INTEGER ),

	// Function Pointers
	DEFINE_FUNCTION( Off ),
	DEFINE_FUNCTION( Recharge ),

	DEFINE_OUTPUT(m_OutRemainingCharge, "OutRemainingCharge"),
	DEFINE_OUTPUT(m_OnHalfEmpty, "OnHalfEmpty" ),
	DEFINE_OUTPUT(m_OnEmpty, "OnEmpty" ),
	DEFINE_OUTPUT(m_OnFull, "OnFull" ),
	DEFINE_OUTPUT(m_OnPlayerUse, "OnPlayerUse" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Recharge", InputRecharge ),
	
END_DATADESC()


LINK_ENTITY_TO_CLASS(func_recharge, CRecharge);


bool CRecharge::KeyValue( const char *szKeyName, const char *szValue )
{
	if (	FStrEq(szKeyName, "style") ||
				FStrEq(szKeyName, "height") ||
				FStrEq(szKeyName, "value1") ||
				FStrEq(szKeyName, "value2") ||
				FStrEq(szKeyName, "value3"))
	{
	}
	else if (FStrEq(szKeyName, "dmdelay"))
	{
		m_iReactivate = atoi(szValue);
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

void CRecharge::Spawn()
{
	Precache( );

	SetSolid( SOLID_BSP );
	SetMoveType( MOVETYPE_PUSH );

	SetModel( STRING( GetModelName() ) );

	UpdateJuice( MaxJuice() );

	m_nState = 0;			

	CreateVPhysics();
}

bool CRecharge::CreateVPhysics()
{
	VPhysicsInitStatic();
	return true;
}

int CRecharge::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf(tempstr,sizeof(tempstr),"Charge left: %i", m_iJuice );
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}


//-----------------------------------------------------------------------------
// Max juice for recharger
//-----------------------------------------------------------------------------
float CRecharge::MaxJuice()	const
{
	if ( HasSpawnFlags( SF_CITADEL_RECHARGER ) )
	{
		return sk_suitcharger_citadel.GetFloat();
	}
	
	return sk_suitcharger.GetFloat();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : newJuice - 
//-----------------------------------------------------------------------------
void CRecharge::UpdateJuice( int newJuice )
{
	bool reduced = newJuice < m_iJuice;
	if ( reduced )
	{
		// Fire 1/2 way output and/or empyt output
		int oneHalfJuice = (int)(MaxJuice() * 0.5f);
		if ( newJuice <= oneHalfJuice && m_iJuice > oneHalfJuice )
		{
			m_OnHalfEmpty.FireOutput( this, this );
		}

		if ( newJuice <= 0 )
		{
			m_OnEmpty.FireOutput( this, this );
		}
	}
	else if ( newJuice != m_iJuice &&
		newJuice == (int)MaxJuice() )
	{
		m_OnFull.FireOutput( this, this );
	}
	m_iJuice = newJuice;
}

void CRecharge::InputRecharge( inputdata_t &inputdata )
{
	Recharge();
}

void CRecharge::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	// if it's not a player, ignore
	if ( !pActivator || !pActivator->IsPlayer() )
		return;

	// Only usable if you have the HEV suit on
	if ( !((CBasePlayer *)pActivator)->IsSuitEquipped() )
	{
		if (m_flSoundTime <= gpGlobals->curtime)
		{
			m_flSoundTime = gpGlobals->curtime + 0.62;
			EmitSound( "SuitRecharge.Deny" );
		}
		return;
	}

	// if there is no juice left, turn it off
	if (m_iJuice <= 0)
	{
		m_nState = 1;			
		Off();
	}

	// if the player doesn't have the suit, or there is no juice left, make the deny noise
	if ( m_iJuice <= 0 )
	{
		if (m_flSoundTime <= gpGlobals->curtime)
		{
			m_flSoundTime = gpGlobals->curtime + 0.62;
			EmitSound( "SuitRecharge.Deny" );
		}
		return;
	}

	SetNextThink( gpGlobals->curtime + 0.25 );
	SetThink(&CRecharge::Off);

	// Time to recharge yet?
	if (m_flNextCharge >= gpGlobals->curtime)
		return;

	// Make sure that we have a caller
	if (!pActivator)
		return;

	m_hActivator = pActivator;

	//only recharge the player

	if (!m_hActivator->IsPlayer() )
		return;
	
	// Play the on sound or the looping charging sound
	if (!m_iOn)
	{
		m_iOn++;
		EmitSound( "SuitRecharge.Start" );
		m_flSoundTime = 0.56 + gpGlobals->curtime;

		m_OnPlayerUse.FireOutput( pActivator, this );
	}

	if ((m_iOn == 1) && (m_flSoundTime <= gpGlobals->curtime))
	{
		m_iOn++;
		CPASAttenuationFilter filter( this, "SuitRecharge.ChargingLoop" );
		filter.MakeReliable();
		EmitSound( filter, entindex(), "SuitRecharge.ChargingLoop" );
	}

	CBasePlayer *pl = (CBasePlayer *) m_hActivator.Get();

	// charge the player
	int nMaxArmor = 100;
	int nIncrementArmor = 1;
	if ( HasSpawnFlags(	SF_CITADEL_RECHARGER ) )
	{
		nMaxArmor = sk_suitcharger_citadel_maxarmor.GetInt();
		nIncrementArmor = 10;

		// Also give health for the citadel version.
		if( pActivator->GetHealth() < pActivator->GetMaxHealth() )
		{
			pActivator->TakeHealth( 5, DMG_GENERIC );
		}
	}

	if (pl->ArmorValue() < nMaxArmor)
	{
		UpdateJuice( m_iJuice - nIncrementArmor );
		pl->IncrementArmorValue( nIncrementArmor, nMaxArmor );
	}

	// Send the output.
	float flRemaining = m_iJuice / MaxJuice();
	m_OutRemainingCharge.Set(flRemaining, pActivator, this);

	// govern the rate of charge
	m_flNextCharge = gpGlobals->curtime + 0.1;
}

void CRecharge::Recharge(void)
{
	UpdateJuice( MaxJuice() );
	m_nState = 0;			
	SetThink( &CRecharge::SUB_DoNothing );
}

void CRecharge::Off(void)
{
	// Stop looping sound.
	if (m_iOn > 1)
	{
		StopSound( "SuitRecharge.ChargingLoop" );
	}

	m_iOn = 0;

	if ((!m_iJuice) &&  ( ( m_iReactivate = g_pGameRules->FlHEVChargerRechargeTime() ) > 0) )
	{
		SetNextThink( gpGlobals->curtime + m_iReactivate );
		SetThink(&CRecharge::Recharge);
	}
	else
	{
		SetThink( NULL );
	}
}


//NEW
class CNewRecharge : public CBaseAnimating
{
public:
	DECLARE_CLASS( CNewRecharge, CBaseAnimating );

	void Spawn( );
	bool CreateVPhysics();
	int DrawDebugTextOverlays(void);
	void Off(void);
	void Recharge(void);
	bool KeyValue( const char *szKeyName, const char *szValue );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return (BaseClass::ObjectCaps() | m_iCaps ); }

	void SetInitialCharge( void );

private:
	void InputRecharge( inputdata_t &inputdata );
	void InputSetCharge( inputdata_t &inputdata );
	float MaxJuice() const;
	void UpdateJuice( int newJuice );
	void Precache( void );

	DECLARE_DATADESC();

	float	m_flNextCharge; 
	int		m_iReactivate ; // DeathMatch Delay until reactvated
	int		m_iJuice;
	int		m_iOn;			// 0 = off, 1 = startup, 2 = going
	float   m_flSoundTime;
	
	int		m_nState;
	int		m_iCaps;
	int		m_iMaxJuice;
	
	COutputFloat m_OutRemainingCharge;
	COutputEvent m_OnHalfEmpty;
	COutputEvent m_OnEmpty;
	COutputEvent m_OnFull;
	COutputEvent m_OnPlayerUse;

	virtual void StudioFrameAdvance ( void );
	float m_flJuice;
};

BEGIN_DATADESC( CNewRecharge )

	DEFINE_FIELD( m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( m_iReactivate, FIELD_INTEGER),
	DEFINE_FIELD( m_iJuice, FIELD_INTEGER),
	DEFINE_FIELD( m_iOn, FIELD_INTEGER),
	DEFINE_FIELD( m_flSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_nState, FIELD_INTEGER ),
	DEFINE_FIELD( m_iCaps, FIELD_INTEGER ),
	DEFINE_FIELD( m_iMaxJuice, FIELD_INTEGER ),

	// Function Pointers
	DEFINE_FUNCTION( Off ),
	DEFINE_FUNCTION( Recharge ),

	DEFINE_OUTPUT(m_OutRemainingCharge, "OutRemainingCharge"),
	DEFINE_OUTPUT(m_OnHalfEmpty, "OnHalfEmpty" ),
	DEFINE_OUTPUT(m_OnEmpty, "OnEmpty" ),
	DEFINE_OUTPUT(m_OnFull, "OnFull" ),
	DEFINE_OUTPUT(m_OnPlayerUse, "OnPlayerUse" ),
	DEFINE_FIELD( m_flJuice, FIELD_FLOAT ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Recharge", InputRecharge ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetCharge", InputSetCharge ),
	
END_DATADESC()


LINK_ENTITY_TO_CLASS( item_suitcharger, CNewRecharge);

#define HEALTH_CHARGER_MODEL_NAME "models/props_combine/suit_charger001.mdl"
#define CHARGE_RATE 0.25f
#define CHARGES_PER_SECOND 1 / CHARGE_RATE
#define CITADEL_CHARGES_PER_SECOND 10 / CHARGE_RATE
#define CALLS_PER_SECOND 7.0f * CHARGES_PER_SECOND


bool CNewRecharge::KeyValue( const char *szKeyName, const char *szValue )
{
	if (	FStrEq(szKeyName, "style") ||
				FStrEq(szKeyName, "height") ||
				FStrEq(szKeyName, "value1") ||
				FStrEq(szKeyName, "value2") ||
				FStrEq(szKeyName, "value3"))
	{
	}
	else if (FStrEq(szKeyName, "dmdelay"))
	{
		m_iReactivate = atoi(szValue);
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

void CNewRecharge::Precache( void )
{
	PrecacheModel( HEALTH_CHARGER_MODEL_NAME );

	PrecacheScriptSound( "SuitRecharge.Deny" );
	PrecacheScriptSound( "SuitRecharge.Start" );
	PrecacheScriptSound( "SuitRecharge.ChargingLoop" );

}

void CNewRecharge::SetInitialCharge( void )
{
	if ( HasSpawnFlags( SF_KLEINER_RECHARGER ) )
	{
		// The charger in Kleiner's lab.
		m_iMaxJuice =  25.0f;
		return;
	}

	if ( HasSpawnFlags( SF_CITADEL_RECHARGER ) )
	{
		m_iMaxJuice =  sk_suitcharger_citadel.GetFloat();
		return;
	}

	m_iMaxJuice =  sk_suitcharger.GetFloat();
}

void CNewRecharge::Spawn()
{
	Precache( );

	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_VPHYSICS );
	CreateVPhysics();

	SetModel( HEALTH_CHARGER_MODEL_NAME );
	AddEffects( EF_NOSHADOW );

	ResetSequence( LookupSequence( "idle" ) );

	SetInitialCharge();

	UpdateJuice( MaxJuice() );

	m_nState = 0;		
	m_iCaps	= FCAP_CONTINUOUS_USE;

	CreateVPhysics();

	m_flJuice = m_iJuice;

	m_iReactivate = 0;

	SetCycle( 1.0f - ( m_flJuice / MaxJuice() ) );
}

bool CNewRecharge::CreateVPhysics()
{
	VPhysicsInitStatic();
	return true;
}

int CNewRecharge::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf(tempstr,sizeof(tempstr),"Charge left: %i", m_iJuice );
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

void CNewRecharge::StudioFrameAdvance( void )
{
	m_flPlaybackRate = 0;

	float flMaxJuice = MaxJuice() + 0.1f;
	float flNewJuice = 1.0f - (float)( m_flJuice / flMaxJuice );

	SetCycle( flNewJuice );
//	Msg( "Cycle: %f - Juice: %d - m_flJuice :%f - Interval: %f\n", (float)GetCycle(), (int)m_iJuice, (float)m_flJuice, GetAnimTimeInterval() );

	if ( !m_flPrevAnimTime )
	{
		m_flPrevAnimTime = gpGlobals->curtime;
	}

	// Latch prev
	m_flPrevAnimTime = m_flAnimTime;
	// Set current
	m_flAnimTime = gpGlobals->curtime;
}



//-----------------------------------------------------------------------------
// Max juice for recharger
//-----------------------------------------------------------------------------
float CNewRecharge::MaxJuice()	const
{
	return m_iMaxJuice;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : newJuice - 
//-----------------------------------------------------------------------------
void CNewRecharge::UpdateJuice( int newJuice )
{
	bool reduced = newJuice < m_iJuice;
	if ( reduced )
	{
		// Fire 1/2 way output and/or empyt output
		int oneHalfJuice = (int)(MaxJuice() * 0.5f);
		if ( newJuice <= oneHalfJuice && m_iJuice > oneHalfJuice )
		{
			m_OnHalfEmpty.FireOutput( this, this );
		}

		if ( newJuice <= 0 )
		{
			m_OnEmpty.FireOutput( this, this );
		}
	}
	else if ( newJuice != m_iJuice &&
		newJuice == (int)MaxJuice() )
	{
		m_OnFull.FireOutput( this, this );
	}
	m_iJuice = newJuice;
}

void CNewRecharge::InputRecharge( inputdata_t &inputdata )
{
	Recharge();
}

void CNewRecharge::InputSetCharge( inputdata_t &inputdata )
{
	ResetSequence( LookupSequence( "idle" ) );

	int iJuice = inputdata.value.Int();

	m_flJuice = m_iMaxJuice = m_iJuice = iJuice;
	StudioFrameAdvance();
}

void CNewRecharge::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	// if it's not a player, ignore
	if ( !pActivator || !pActivator->IsPlayer() )
		return;

	CBasePlayer *pPlayer = static_cast<CBasePlayer *>(pActivator);

	// Reset to a state of continuous use.
	m_iCaps = FCAP_CONTINUOUS_USE;

	if ( m_iOn )
	{
		float flCharges = CHARGES_PER_SECOND;
		float flCalls = CALLS_PER_SECOND;

		if ( HasSpawnFlags( SF_CITADEL_RECHARGER ) )
			 flCharges = CITADEL_CHARGES_PER_SECOND;

		m_flJuice -= flCharges / flCalls;		
		StudioFrameAdvance();
	}

	// Only usable if you have the HEV suit on
	if ( !pPlayer->IsSuitEquipped() )
	{
		if (m_flSoundTime <= gpGlobals->curtime)
		{
			m_flSoundTime = gpGlobals->curtime + 0.62;
			EmitSound( "SuitRecharge.Deny" );
		}
		return;
	}

	// if there is no juice left, turn it off
	if ( m_iJuice <= 0 )
	{
		// Start our deny animation over again
		ResetSequence( LookupSequence( "emptyclick" ) );
		
		m_nState = 1;
		
		// Shut off
		Off();
		
		// Play a deny sound
		if ( m_flSoundTime <= gpGlobals->curtime )
		{
			m_flSoundTime = gpGlobals->curtime + 0.62;
			EmitSound( "SuitRecharge.Deny" );
		}

		return;
	}

	// Get our maximum armor value
	int nMaxArmor = 100;
	if ( HasSpawnFlags(	SF_CITADEL_RECHARGER ) )
	{
		nMaxArmor = sk_suitcharger_citadel_maxarmor.GetInt();
	}
	
	int nIncrementArmor = 1;

	// The citadel charger gives more per charge and also gives health
	if ( HasSpawnFlags(	SF_CITADEL_RECHARGER ) )
	{
		nIncrementArmor = 10;
		
#ifdef HL2MP
		nIncrementArmor = 2;
#endif

		// Also give health for the citadel version.
		if ( pActivator->GetHealth() < pActivator->GetMaxHealth() && m_flNextCharge < gpGlobals->curtime )
		{
			pActivator->TakeHealth( 5, DMG_GENERIC );
		}
	}

	// If we're over our limit, debounce our keys
	if ( pPlayer->ArmorValue() >= nMaxArmor)
	{
		// Citadel charger must also be at max health
		if ( !HasSpawnFlags(SF_CITADEL_RECHARGER) || ( HasSpawnFlags( SF_CITADEL_RECHARGER ) && pActivator->GetHealth() >= pActivator->GetMaxHealth() ) )
		{
			// Make the user re-use me to get started drawing health.
			pPlayer->m_afButtonPressed &= ~IN_USE;
			m_iCaps = FCAP_IMPULSE_USE;
			
			EmitSound( "SuitRecharge.Deny" );
			return;
		}
	}

	// This is bumped out if used within the time period
	SetNextThink( gpGlobals->curtime + CHARGE_RATE );
	SetThink( &CNewRecharge::Off );

	// Time to recharge yet?
	if ( m_flNextCharge >= gpGlobals->curtime )
		return;
	
	// Play the on sound or the looping charging sound
	if ( !m_iOn )
	{
		m_iOn++;
		EmitSound( "SuitRecharge.Start" );
		m_flSoundTime = 0.56 + gpGlobals->curtime;

		m_OnPlayerUse.FireOutput( pActivator, this );
	}

	if ((m_iOn == 1) && (m_flSoundTime <= gpGlobals->curtime))
	{
		m_iOn++;
		CPASAttenuationFilter filter( this, "SuitRecharge.ChargingLoop" );
		filter.MakeReliable();
		EmitSound( filter, entindex(), "SuitRecharge.ChargingLoop" );
	}

	// Give armor if we need it
	if ( pPlayer->ArmorValue() < nMaxArmor )
	{
		UpdateJuice( m_iJuice - nIncrementArmor );
		pPlayer->IncrementArmorValue( nIncrementArmor, nMaxArmor );
	}

	// Send the output.
	float flRemaining = m_iJuice / MaxJuice();
	m_OutRemainingCharge.Set(flRemaining, pActivator, this);

	// govern the rate of charge
	m_flNextCharge = gpGlobals->curtime + 0.1;
}

void CNewRecharge::Recharge(void)
{
	EmitSound( "SuitRecharge.Start" );
	ResetSequence( LookupSequence( "idle" ) );

	UpdateJuice( MaxJuice() );

	m_nState = 0;		
	m_flJuice = m_iJuice;
	m_iReactivate = 0;
	StudioFrameAdvance();

	SetThink( &CNewRecharge::SUB_DoNothing );
}

void CNewRecharge::Off(void)
{
	// Stop looping sound.
	if (m_iOn > 1)
	{
		StopSound( "SuitRecharge.ChargingLoop" );
	}
	
	if ( m_nState == 1 )
	{
		SetCycle( 1.0f );
	}

	m_iOn = 0;
	m_flJuice = m_iJuice;

	if ( m_iReactivate == 0 )
	{
		if ((!m_iJuice) && g_pGameRules->FlHEVChargerRechargeTime() > 0 )
		{
			if ( HasSpawnFlags( SF_CITADEL_RECHARGER ) )
			{
				m_iReactivate = g_pGameRules->FlHEVChargerRechargeTime() * 2;
			}
			else
			{
				m_iReactivate = g_pGameRules->FlHEVChargerRechargeTime();
			}
			SetNextThink( gpGlobals->curtime + m_iReactivate );
			SetThink(&CNewRecharge::Recharge);
		}
		else
		{
			SetThink( NULL );
		}
	}
}
