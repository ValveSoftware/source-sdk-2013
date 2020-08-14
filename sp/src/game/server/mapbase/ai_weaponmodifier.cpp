//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Be warned, because this entity is TERRIBLE!
//
//=============================================================================

#include "cbase.h"
#include "ai_basenpc.h"
#include "saverestore_utlvector.h"



//-----------------------------------------------------------------------------
// Purpose: A special CAI_ShotRegulator class designed to be used with ai_weaponmodifier.
//			I'm too chicken to do anything fun with it.
//-----------------------------------------------------------------------------
typedef CAI_ShotRegulator CAI_CustomShotRegulator;
/*
class CAI_CustomShotRegulator : public CAI_ShotRegulator
{
	DECLARE_CLASS( CAI_CustomShotRegulator, CAI_ShotRegulator );
public:
	void SetMinBurstInterval( float flMinBurstInterval ) { m_flMinBurstInterval = flMinBurstInterval; }
	void SetMaxBurstInterval( float flMaxBurstInterval ) { m_flMaxBurstInterval = flMaxBurstInterval; }
};
*/

// A lot of functions can't set each range individually, so we have to do this for a lot of them.
// (again, too chicken to do something useful with CAI_CustomShotRegulator)
#define WeaponModifierSetRange(string, function) float minimum = 0; \
		float maximum = 0; \
		if (sscanf(string, "%f:%f", &minimum, &maximum)) \
			function(minimum, maximum);

#define WEAPONMODIFIER_MAX_NPCS 16

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CAI_WeaponModifier : public CLogicalEntity
{
	DECLARE_CLASS( CAI_WeaponModifier, CLogicalEntity );
	DECLARE_DATADESC();
public:

	void Spawn();

	virtual int			Save( ISave &save ); 
	virtual int			Restore( IRestore &restore );

	bool KeyValue(const char *szKeyName, const char *szValue);

	void EnableOnNPC(CAI_BaseNPC *pNPC);
	void DisableOnNPC(CAI_BaseNPC *pNPC);

	// Inputs
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputEnableOnNPC( inputdata_t &inputdata );
	void InputDisableOnNPC( inputdata_t &inputdata );

	void InputSetBurstInterval( inputdata_t &inputdata ) { WeaponModifierSetRange(inputdata.value.String(), m_ModdedRegulator.SetBurstInterval); }
	void InputSetRestInterval( inputdata_t &inputdata ) { WeaponModifierSetRange(inputdata.value.String(), m_ModdedRegulator.SetRestInterval); }
	void InputSetBurstShotCountRange( inputdata_t &inputdata ) { WeaponModifierSetRange(inputdata.value.String(), m_ModdedRegulator.SetBurstShotCountRange); }
	void InputSetBurstShotsRemaining( inputdata_t &inputdata ) { m_ModdedRegulator.SetBurstShotsRemaining(inputdata.value.Int()); }

	void InputEnableShooting( inputdata_t &inputdata ) { m_ModdedRegulator.EnableShooting(); }
	void InputDisableShooting( inputdata_t &inputdata ) { m_ModdedRegulator.DisableShooting(); }
	void InputFireNoEarlierThan( inputdata_t &inputdata ) { m_ModdedRegulator.FireNoEarlierThan(gpGlobals->curtime + inputdata.value.Float()); }
	void InputReset( inputdata_t &inputdata ) { m_ModdedRegulator.Reset(inputdata.value.Bool()); }

	// The NPCs and their original regulators.
	AIHANDLE m_hNPCs[WEAPONMODIFIER_MAX_NPCS];
	CAI_ShotRegulator m_StoredRegulators[WEAPONMODIFIER_MAX_NPCS];

	CAI_CustomShotRegulator m_ModdedRegulator;

	bool m_bDisabled;
};

LINK_ENTITY_TO_CLASS(ai_weaponmodifier, CAI_WeaponModifier);

BEGIN_DATADESC( CAI_WeaponModifier )

	DEFINE_EMBEDDED( m_ModdedRegulator ),

	DEFINE_ARRAY( m_hNPCs, FIELD_EHANDLE, WEAPONMODIFIER_MAX_NPCS ),
	DEFINE_EMBEDDED_ARRAY( m_StoredRegulators, WEAPONMODIFIER_MAX_NPCS ),

	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "EnableOnNPC", InputEnableOnNPC ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "DisableOnNPC", InputDisableOnNPC ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetBurstInterval", InputSetBurstInterval ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRestInterval", InputSetRestInterval ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetBurstShotCountRange", InputSetBurstShotCountRange ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetBurstShotsRemaining", InputSetBurstShotsRemaining ),

	DEFINE_INPUTFUNC( FIELD_VOID, "EnableShooting", InputEnableShooting ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableShooting", InputDisableShooting ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "FireNoEarlierThan", InputFireNoEarlierThan ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "Reset", InputReset ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_WeaponModifier::Spawn()
{
	if (!m_bDisabled)
	{
		inputdata_t inputdata;
		InputEnable(inputdata);
	}

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CAI_WeaponModifier::Save( ISave &save )
{
	return BaseClass::Save(save);
}

int CAI_WeaponModifier::Restore( IRestore &restore )
{
	return BaseClass::Restore(restore);
}

//-----------------------------------------------------------------------------
// Purpose: Handles key values from the BSP before spawn is called.
//-----------------------------------------------------------------------------
bool CAI_WeaponModifier::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "BurstInterval"))
	{
		WeaponModifierSetRange(szValue, m_ModdedRegulator.SetBurstInterval);
	}
	else if (FStrEq(szKeyName, "RestInterval"))
	{
		WeaponModifierSetRange(szValue, m_ModdedRegulator.SetRestInterval);
	}
	else if (FStrEq(szKeyName, "BurstShotCountRange"))
	{
		WeaponModifierSetRange(szValue, m_ModdedRegulator.SetBurstShotCountRange);
	}
	else if (FStrEq(szKeyName, "BurstShotsRemaining"))
	{
		m_ModdedRegulator.SetBurstShotsRemaining(atoi(szValue));
	}
	else
		return BaseClass::KeyValue(szKeyName, szValue);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_WeaponModifier::EnableOnNPC( CAI_BaseNPC *pNPC )
{
	for (int i = 0; i < WEAPONMODIFIER_MAX_NPCS; i++)
	{
		if (m_hNPCs[i] == NULL)
		{
			m_hNPCs[i] = pNPC;
			m_StoredRegulators[i] = *pNPC->GetShotRegulator();

			pNPC->SetShotRegulator(m_ModdedRegulator);
		}
		else if (m_hNPCs[i] == pNPC)
		{
			// We're already in it
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_WeaponModifier::DisableOnNPC( CAI_BaseNPC *pNPC )
{
	for (int i = 0; i < WEAPONMODIFIER_MAX_NPCS; i++)
	{
		if (m_hNPCs[i] == pNPC)
		{
			pNPC->SetShotRegulator(m_StoredRegulators[i]);
			m_hNPCs[i] = NULL;

			// Just reset it to our modded one
			m_StoredRegulators[i] = m_ModdedRegulator;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_WeaponModifier::InputEnable( inputdata_t &inputdata )
{
	CBaseEntity *pEntity = gEntList.FindEntityByName(NULL, STRING(m_target), this, inputdata.pActivator, inputdata.pCaller);
	while (pEntity)
	{
		if (pEntity->IsNPC())
		{
			EnableOnNPC(pEntity->MyNPCPointer());
		}

		pEntity = gEntList.FindEntityByName(pEntity, STRING(m_target), this, inputdata.pActivator, inputdata.pCaller);
	}
}

void CAI_WeaponModifier::InputDisable( inputdata_t &inputdata )
{
	for (int i = 0; i < WEAPONMODIFIER_MAX_NPCS; i++)
	{
		m_hNPCs[i]->SetShotRegulator(m_StoredRegulators[i]);
		m_hNPCs[i] = NULL;

		// Just reset it to our modded one
		m_StoredRegulators[i] = m_ModdedRegulator;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_WeaponModifier::InputEnableOnNPC( inputdata_t &inputdata )
{
	if (inputdata.value.Entity() && inputdata.value.Entity()->IsNPC())
	{
		EnableOnNPC(inputdata.value.Entity()->MyNPCPointer());
	}
}

void CAI_WeaponModifier::InputDisableOnNPC( inputdata_t &inputdata )
{
	if (inputdata.value.Entity() && inputdata.value.Entity()->IsNPC())
	{
		DisableOnNPC(inputdata.value.Entity()->MyNPCPointer());
	}
}
