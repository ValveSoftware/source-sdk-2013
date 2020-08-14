//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Fires projectiles. What else is there to say?
//
//=============================================================================

#include "cbase.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPointProjectile : public CBaseEntity
{
	DECLARE_CLASS( CPointProjectile, CBaseEntity );
	DECLARE_DATADESC();

public:

	void Precache();
	void Spawn();

	// m_target is projectile class

	// Owner
	// Handle is m_hOwnerEntity
	string_t m_iszOwner;

	// Damage
	float m_flDamage;

	// Speed
	float m_flSpeed;

	bool m_bFireProjectilesFromOwner;

	CBaseEntity *CalculateOwner( CBaseEntity *pActivator, CBaseEntity *pCaller );

	CBaseEntity *CreateProjectile( Vector &vecOrigin, QAngle &angAngles, Vector &vecDir, CBaseEntity *pOwner );

	// Inputs
	void InputFire( inputdata_t &inputdata );
	void InputFireAtEntity( inputdata_t &inputdata );
	void InputFireAtPosition( inputdata_t &inputdata );

	void InputSetDamage( inputdata_t &inputdata ) { m_flDamage = inputdata.value.Float(); }
	void InputSetOwner( inputdata_t &inputdata ) { m_iszOwner = inputdata.value.StringID(); SetOwnerEntity(NULL); }
	void InputSetSpeed( inputdata_t &inputdata ) { m_flSpeed = inputdata.value.Float(); }

	void InputSetTarget( inputdata_t &inputdata ) { BaseClass::InputSetTarget(inputdata); UTIL_PrecacheOther(inputdata.value.String()); }

	COutputEHANDLE m_OnFire;
};

LINK_ENTITY_TO_CLASS(point_projectile, CPointProjectile);

BEGIN_DATADESC( CPointProjectile )

	// Keys
	DEFINE_KEYFIELD( m_iszOwner, FIELD_STRING, "Owner" ),
	DEFINE_KEYFIELD( m_flDamage, FIELD_FLOAT, "Damage" ),
	DEFINE_KEYFIELD( m_flSpeed, FIELD_FLOAT, "Speed" ),
	DEFINE_KEYFIELD( m_bFireProjectilesFromOwner, FIELD_BOOLEAN, "FireFromOwner" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Fire", InputFire ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "FireAtEntity", InputFireAtEntity ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "FireAtPosition", InputFireAtPosition ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetDamage", InputSetDamage ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetOwnerEntity", InputSetOwner ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpeed", InputSetSpeed ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetProjectileClass", InputSetTarget ),

	// Outputs
	DEFINE_OUTPUT(m_OnFire, "OnFire"),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointProjectile::Precache()
{
	UTIL_PrecacheOther(STRING(m_target));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointProjectile::Spawn()
{
	Precache();

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Calculates owner entity
//-----------------------------------------------------------------------------
inline CBaseEntity *CPointProjectile::CalculateOwner( CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	if (m_iszOwner != NULL_STRING && !GetOwnerEntity())
	{
		CBaseEntity *pOwner = gEntList.FindEntityByName(NULL, STRING(m_iszOwner), this, pActivator, pCaller);
		if (pOwner)
			SetOwnerEntity(pOwner);
	}

	return GetOwnerEntity() ? GetOwnerEntity() : this;
}

//-----------------------------------------------------------------------------
// Purpose: Fires projectile and output
//-----------------------------------------------------------------------------
inline CBaseEntity *CPointProjectile::CreateProjectile( Vector &vecOrigin, QAngle &angAngles, Vector &vecDir, CBaseEntity *pOwner )
{
	CBaseEntity *pProjectile = CreateNoSpawn(STRING(m_target), vecOrigin, angAngles, pOwner);
	if (!pProjectile)
	{
		Warning("WARNING: %s unable to create projectile class %s!\n", GetDebugName(), STRING(m_target));
		return NULL;
	}

	pProjectile->SetAbsVelocity(vecDir * m_flSpeed);
	DispatchSpawn(pProjectile);

	pProjectile->SetDamage(m_flDamage);

	m_OnFire.Set(pProjectile, pProjectile, pOwner);

	return pProjectile;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointProjectile::InputFire( inputdata_t &inputdata )
{
	Vector vecOrigin = GetAbsOrigin();
	QAngle angAngles = GetAbsAngles();

	CBaseEntity *pOwner = CalculateOwner(inputdata.pActivator, inputdata.pCaller);
	if (pOwner && m_bFireProjectilesFromOwner)
		vecOrigin = pOwner->GetAbsOrigin();

	Vector vecDir;
	AngleVectors(angAngles, &vecDir);

	CreateProjectile(vecOrigin, angAngles, vecDir, pOwner);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointProjectile::InputFireAtEntity( inputdata_t &inputdata )
{
	CBaseEntity *pTarget = inputdata.value.Entity();
	if (!pTarget)
		return;

	Vector vecOrigin = GetAbsOrigin();

	CBaseEntity *pOwner = CalculateOwner(inputdata.pActivator, inputdata.pCaller);
	if (pOwner && m_bFireProjectilesFromOwner)
		vecOrigin = pOwner->GetAbsOrigin();

	Vector vecDir = (pTarget->WorldSpaceCenter() - vecOrigin);
	VectorNormalize(vecDir);

	QAngle angAngles;
	VectorAngles(vecDir, angAngles);

	CreateProjectile(vecOrigin, angAngles, vecDir, pOwner);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointProjectile::InputFireAtPosition( inputdata_t &inputdata )
{
	Vector vecInput;
	inputdata.value.Vector3D(vecInput);

	Vector vecOrigin = GetAbsOrigin();

	CBaseEntity *pOwner = CalculateOwner(inputdata.pActivator, inputdata.pCaller);
	if (pOwner && m_bFireProjectilesFromOwner)
		vecOrigin = pOwner->GetAbsOrigin();

	Vector vecDir = (vecInput - vecOrigin);
	VectorNormalize(vecDir);

	QAngle angAngles;
	VectorAngles(vecDir, angAngles);

	CreateProjectile(vecOrigin, angAngles, vecDir, pOwner);
}
