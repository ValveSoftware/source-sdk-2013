//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: A special entity for afflicting damage as specific as possible.
//
//=============================================================================

#include "cbase.h"


//-----------------------------------------------------------------------------
// Purpose: Advanced damage information afflicting.
//-----------------------------------------------------------------------------
class CPointDamageInfo : public CLogicalEntity
{
	DECLARE_CLASS( CPointDamageInfo, CLogicalEntity );
	DECLARE_DATADESC();

	virtual bool KeyValue( const char *szKeyName, const char *szValue );
	virtual bool KeyValue( const char *szKeyName, const Vector &vecValue );
	virtual bool GetKeyValue( const char *szKeyName, char *szValue, int iMaxLen );

public:
	CPointDamageInfo();

	CTakeDamageInfo m_info;

	// The actual inflictor, attacker, and weapon in CTakeDamageInfo are direct entity pointers.
	// This is needed to ensure !activator functionality, entities that might not exist yet, etc.
	string_t m_iszInflictor;
	string_t m_iszAttacker;
	string_t m_iszWeapon;

	// The maximum number of entities to be damaged if they share m_target's targetname or classname.
	int m_iMaxEnts;

	// Suppresses death sounds in the best way we possibly can.
	bool m_bSuppressDeathSound;

	//bool m_bDisabled;

	// Inputs
	void InputSetInflictor( inputdata_t &inputdata ) { m_iszInflictor = inputdata.value.StringID(); } //{ m_info.SetInflictor(inputdata.value.Entity()); }
	void InputSetAttacker( inputdata_t &inputdata ) { m_iszAttacker = inputdata.value.StringID(); } //{ m_info.SetAttacker(inputdata.value.Entity()); }
	void InputSetWeapon( inputdata_t &inputdata ) { m_iszWeapon = inputdata.value.StringID(); } //{ m_info.SetWeapon(inputdata.value.Entity()); }

	void InputSetDamage( inputdata_t &inputdata ) { m_info.SetDamage(inputdata.value.Float()); }
	void InputSetMaxDamage( inputdata_t &inputdata ) { m_info.SetMaxDamage(inputdata.value.Float()); }
	void InputSetDamageBonus( inputdata_t &inputdata ) { m_info.SetDamageBonus(inputdata.value.Float()); }

	void InputSetDamageType( inputdata_t &inputdata ) { m_info.SetDamageType(inputdata.value.Int()); }
	void InputSetDamageCustom( inputdata_t &inputdata ) { m_info.SetDamageCustom(inputdata.value.Int()); }
	void InputSetDamageStats( inputdata_t &inputdata ) { m_info.SetDamageStats(inputdata.value.Int()); }
	void InputSetForceFriendlyFire( inputdata_t &inputdata ) { m_info.SetForceFriendlyFire(inputdata.value.Bool()); }

	void InputSetAmmoType( inputdata_t &inputdata ) { m_info.SetAmmoType(inputdata.value.Int()); }

	void InputSetPlayerPenetrationCount( inputdata_t &inputdata ) { m_info.SetPlayerPenetrationCount( inputdata.value.Int() ); }
	void InputSetDamagedOtherPlayers( inputdata_t &inputdata ) { m_info.SetDamagedOtherPlayers( inputdata.value.Int() ); }

	void InputSetDamageForce( inputdata_t &inputdata ) { Vector vec; inputdata.value.Vector3D(vec); m_info.SetDamageForce(vec); }
	void InputSetDamagePosition( inputdata_t &inputdata ) { Vector vec; inputdata.value.Vector3D(vec); m_info.SetDamagePosition(vec); }
	void InputSetReportedPosition( inputdata_t &inputdata ) { Vector vec; inputdata.value.Vector3D(vec); m_info.SetReportedPosition(vec); }

	void HandleDamage(CBaseEntity *pTarget);

	void ApplyDamage( const char *target, inputdata_t &inputdata );
	void ApplyDamage( CBaseEntity *target, inputdata_t &inputdata );

	void InputApplyDamage( inputdata_t &inputdata );
	void InputApplyDamageToEntity( inputdata_t &inputdata );

	// Outputs
	COutputEvent m_OnApplyDamage;
	COutputEvent m_OnApplyDeath;
};

LINK_ENTITY_TO_CLASS(point_damageinfo, CPointDamageInfo);


BEGIN_DATADESC( CPointDamageInfo )

	DEFINE_EMBEDDED( m_info ),

	// Keys
	DEFINE_KEYFIELD( m_iszInflictor, FIELD_STRING, "Inflictor" ),
	DEFINE_KEYFIELD( m_iszAttacker, FIELD_STRING, "Attacker" ),
	DEFINE_KEYFIELD( m_iszWeapon, FIELD_STRING, "Weapon" ),

	DEFINE_KEYFIELD( m_iMaxEnts, FIELD_INTEGER, "MaxEnts" ),
	DEFINE_KEYFIELD( m_bSuppressDeathSound, FIELD_BOOLEAN, "SuppressDeathSound" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_STRING, "SetInflictor", InputSetInflictor ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetAttacker", InputSetAttacker ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetWeapon", InputSetWeapon ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetDamage", InputSetDamage ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetMaxDamage", InputSetMaxDamage ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetDamageBonus", InputSetDamageBonus ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetDamageType", InputSetDamageType ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetDamageCustom", InputSetDamageCustom ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetDamageStats", InputSetDamageStats ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetForceFriendlyFire", InputSetForceFriendlyFire ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetAmmoType", InputSetAmmoType ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetPlayerPenetrationCount", InputSetPlayerPenetrationCount ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetDamagedOtherPlayers", InputSetDamagedOtherPlayers ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetDamageForce", InputSetDamageForce ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetDamagePosition", InputSetDamagePosition ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetReportedPosition", InputSetReportedPosition ),

	DEFINE_INPUTFUNC( FIELD_VOID, "ApplyDamage", InputApplyDamage ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "ApplyDamageToEntity", InputApplyDamageToEntity ),

	// Outputs
	DEFINE_OUTPUT(m_OnApplyDamage, "OnApplyDamage"),
	DEFINE_OUTPUT(m_OnApplyDeath, "OnApplyDeath"),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPointDamageInfo::CPointDamageInfo()
{
	m_info = CTakeDamageInfo(this, this, 24, DMG_GENERIC);
	m_iMaxEnts = 1;
}

//-----------------------------------------------------------------------------
// Purpose: Cache user entity field values until spawn is called.
// Input  : szKeyName - Key to handle.
//			szValue - Value for key.
// Output : Returns true if the key was handled, false if not.
//-----------------------------------------------------------------------------
bool CPointDamageInfo::KeyValue( const char *szKeyName, const char *szValue )
{
	/*if (FStrEq(szKeyName, "Inflictor"))
		m_info.SetInflictor(gEntList.FindEntityByName(NULL, szValue, this, NULL, NULL));
	else if (FStrEq(szKeyName, "Attacker"))
		m_info.SetAttacker(gEntList.FindEntityByName(NULL, szValue, this, NULL, NULL));
	else if (FStrEq(szKeyName, "Weapon"))
		m_info.SetWeapon(gEntList.FindEntityByName(NULL, szValue, this, NULL, NULL));

	else*/ if (FStrEq(szKeyName, "Damage"))
		m_info.SetDamage(atof(szValue));
	else if (FStrEq(szKeyName, "MaxDamage"))
		m_info.SetMaxDamage(atof(szValue));
	else if (FStrEq(szKeyName, "DamageBonus"))
		m_info.SetDamageBonus(atof(szValue));

	else if (FStrEq(szKeyName, "DamageType") || FStrEq(szKeyName, "DamagePresets"))
		m_info.AddDamageType(atoi(szValue));
	else if (FStrEq(szKeyName, "DamageOr"))
		m_info.AddDamageType(atoi(szValue));
	else if (FStrEq(szKeyName, "DamageCustom"))
		m_info.SetDamageCustom(atoi(szValue));
	else if (FStrEq(szKeyName, "DamageStats"))
		m_info.SetDamageStats(atoi(szValue));
	else if (FStrEq(szKeyName, "ForceFriendlyFire"))
		m_info.SetForceFriendlyFire(FStrEq(szValue, "1"));

	else if (FStrEq(szKeyName, "AmmoType"))
		m_info.SetAmmoType(atoi(szValue));

	else if (FStrEq(szKeyName, "PlayerPenetrationCount"))
		m_info.SetPlayerPenetrationCount(atoi(szValue));
	else if (FStrEq(szKeyName, "DamagedOtherPlayers"))
		m_info.SetDamagedOtherPlayers(atoi(szValue));

	else
	{
		if (!BaseClass::KeyValue(szKeyName, szValue))
		{
			// Ripped from variant_t::Convert()...
			Vector tmpVec = vec3_origin;
			if (sscanf(szValue, "[%f %f %f]", &tmpVec[0], &tmpVec[1], &tmpVec[2]) == 0)
			{
				// Try sucking out 3 floats with no []s
				sscanf(szValue, "%f %f %f", &tmpVec[0], &tmpVec[1], &tmpVec[2]);
			}
			return KeyValue(szKeyName, tmpVec);
		}
	}

	return true;
}

bool CPointDamageInfo::KeyValue( const char *szKeyName, const Vector &vecValue ) 
{
	if (FStrEq(szKeyName, "DamageForce"))
		m_info.SetDamageForce(vecValue);
	else if (FStrEq(szKeyName, "DamagePosition"))
		m_info.SetDamagePosition(vecValue);
	else if (FStrEq(szKeyName, "ReportedPosition"))
		m_info.SetReportedPosition(vecValue);
	else
		return CBaseEntity::KeyValue( szKeyName, vecValue );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CPointDamageInfo::GetKeyValue( const char *szKeyName, char *szValue, int iMaxLen )
{
	/*
	if (FStrEq(szKeyName, "Inflictor"))
		Q_snprintf(szValue, iMaxLen, "%s", m_info.GetInflictor() ? "" : m_info.GetInflictor()->GetDebugName());
	else if (FStrEq(szKeyName, "Attacker"))
		Q_snprintf(szValue, iMaxLen, "%s", m_info.GetAttacker() ? "" : m_info.GetAttacker()->GetDebugName());
	else if (FStrEq(szKeyName, "Weapon"))
		Q_snprintf(szValue, iMaxLen, "%s", m_info.GetWeapon() ? "" : m_info.GetWeapon()->GetDebugName());

	else*/ if (FStrEq(szKeyName, "Damage"))
		Q_snprintf(szValue, iMaxLen, "%f", m_info.GetDamage());
	else if (FStrEq(szKeyName, "MaxDamage"))
		Q_snprintf(szValue, iMaxLen, "%f", m_info.GetMaxDamage());
	else if (FStrEq(szKeyName, "DamageBonus"))
		Q_snprintf(szValue, iMaxLen, "%f", m_info.GetDamageBonus());

	else if (FStrEq(szKeyName, "DamageType"))
		Q_snprintf(szValue, iMaxLen, "%i", m_info.GetDamageType());
	else if (FStrEq(szKeyName, "DamageCustom"))
		Q_snprintf(szValue, iMaxLen, "%i", m_info.GetDamageCustom());
	else if (FStrEq(szKeyName, "DamageStats"))
		Q_snprintf(szValue, iMaxLen, "%i", m_info.GetDamageStats());
	else if (FStrEq(szKeyName, "ForceFriendlyFire"))
		Q_snprintf(szValue, iMaxLen, "%s", m_info.IsForceFriendlyFire() ? "1" : "0");

	else if (FStrEq(szKeyName, "AmmoType"))
		Q_snprintf(szValue, iMaxLen, "%i", m_info.GetAmmoType());

	else if (FStrEq(szKeyName, "PlayerPenetrationCount"))
		Q_snprintf(szValue, iMaxLen, "%i", m_info.GetPlayerPenetrationCount());
	else if (FStrEq(szKeyName, "DamagedOtherPlayers"))
		Q_snprintf(szValue, iMaxLen, "%i", m_info.GetDamagedOtherPlayers());

	else if (FStrEq(szKeyName, "DamageForce"))
		Q_snprintf(szValue, iMaxLen, "%f %f %f", m_info.GetDamageForce().x, m_info.GetDamageForce().y, m_info.GetDamageForce().z);
	else if (FStrEq(szKeyName, "DamagePosition"))
		Q_snprintf(szValue, iMaxLen, "%f %f %f", m_info.GetDamagePosition().x, m_info.GetDamagePosition().y, m_info.GetDamagePosition().z);
	else if (FStrEq(szKeyName, "ReportedPosition"))
		Q_snprintf(szValue, iMaxLen, "%f %f %f", m_info.GetReportedPosition().x, m_info.GetReportedPosition().y, m_info.GetReportedPosition().z);
	else
		return BaseClass::GetKeyValue(szKeyName, szValue, iMaxLen);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointDamageInfo::HandleDamage( CBaseEntity *pTarget )
{
	pTarget->TakeDamage(m_info);
	m_OnApplyDamage.FireOutput(pTarget, this);
	if (pTarget->m_lifeState == LIFE_DYING)
		m_OnApplyDeath.FireOutput(pTarget, this);

	// This is the best we could do, as nodeathsound is exclusive to response system NPCs
	if (m_bSuppressDeathSound)
		pTarget->EmitSound("AI_BaseNPC.SentenceStop");
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CPointDamageInfo::ApplyDamage( const char *target, inputdata_t &inputdata )
{
	if (m_iszAttacker != NULL_STRING)
		m_info.SetAttacker( gEntList.FindEntityByName(NULL, STRING(m_iszAttacker), this, inputdata.pActivator, inputdata.pCaller) );

	if (m_iszInflictor != NULL_STRING)
		m_info.SetInflictor( gEntList.FindEntityByName(NULL, STRING(m_iszInflictor), this, inputdata.pActivator, inputdata.pCaller) );
	else
	{
		CBaseCombatCharacter *pBCC = ToBaseCombatCharacter(m_info.GetAttacker());
		if (pBCC != NULL && pBCC->GetActiveWeapon())
		{
			m_info.SetInflictor(pBCC->GetActiveWeapon());
		}
	}

	if (m_iszWeapon != NULL_STRING)
		m_info.SetWeapon( gEntList.FindEntityByName(NULL, STRING(m_iszWeapon), this, inputdata.pActivator, inputdata.pCaller) );
	else
	{
		CBaseCombatCharacter *pBCC = ToBaseCombatCharacter(m_info.GetAttacker());
		if (pBCC != NULL && pBCC->GetActiveWeapon())
		{
			m_info.SetWeapon(pBCC->GetActiveWeapon());
		}
	}

	if (!m_info.GetAttacker())
		m_info.SetAttacker( this );

	if (!m_info.GetInflictor())
		m_info.SetInflictor( this );

	if (!m_info.GetWeapon())
		m_info.SetWeapon( this );

	CBaseEntity *pTarget = NULL;
	if (m_iMaxEnts > 0)
	{
		for (int i = 0; i < m_iMaxEnts; i++)
		{
			pTarget = gEntList.FindEntityGeneric(pTarget, target, this, inputdata.pActivator, inputdata.pCaller);
			if (pTarget)
			{
				HandleDamage( pTarget );
			}
		}
	}
	else
	{
		pTarget = gEntList.FindEntityGeneric(NULL, target, this, inputdata.pActivator, inputdata.pCaller);
		while (pTarget)
		{
			HandleDamage( pTarget );
			pTarget = gEntList.FindEntityGeneric(pTarget, target, this, inputdata.pActivator, inputdata.pCaller);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Applies damage to a specific entity
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CPointDamageInfo::ApplyDamage( CBaseEntity *target, inputdata_t &inputdata )
{
	if (!target)
		return;

	if (m_iszAttacker != NULL_STRING)
		m_info.SetAttacker( gEntList.FindEntityByName(NULL, STRING(m_iszAttacker), this, inputdata.pActivator, inputdata.pCaller) );
	else
		m_info.SetAttacker( this );

	if (m_iszInflictor != NULL_STRING)
		m_info.SetInflictor( gEntList.FindEntityByName(NULL, STRING(m_iszInflictor), this, inputdata.pActivator, inputdata.pCaller) );
	else
	{
		CBaseCombatCharacter *pBCC = ToBaseCombatCharacter(m_info.GetAttacker());
		if (pBCC != NULL && pBCC->GetActiveWeapon())
		{
			m_info.SetInflictor(pBCC->GetActiveWeapon());
		}
		else
			m_info.SetInflictor(this);
	}

	if (m_iszWeapon != NULL_STRING)
		m_info.SetWeapon( gEntList.FindEntityByName(NULL, STRING(m_iszWeapon), this, inputdata.pActivator, inputdata.pCaller) );
	else
	{
		CBaseCombatCharacter *pBCC = ToBaseCombatCharacter(m_info.GetAttacker());
		if (pBCC != NULL && pBCC->GetActiveWeapon())
		{
			m_info.SetWeapon(pBCC->GetActiveWeapon());
		}
		else
			m_info.SetWeapon(this);
	}

	target->TakeDamage(m_info);
	m_OnApplyDamage.FireOutput(target, this);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CPointDamageInfo::InputApplyDamage( inputdata_t &inputdata )
{
	ApplyDamage(STRING(m_target), inputdata);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CPointDamageInfo::InputApplyDamageToEntity( inputdata_t &inputdata )
{
	ApplyDamage(inputdata.value.Entity(), inputdata);
}
