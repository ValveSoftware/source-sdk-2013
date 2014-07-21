//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_citizenpackage.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_SERVERCLASS_ST(CWeaponCitizenPackage, DT_WeaponCitizenPackage)
END_SEND_TABLE()

BEGIN_DATADESC( CWeaponCitizenPackage )
END_DATADESC()

LINK_ENTITY_TO_CLASS( weapon_citizenpackage, CWeaponCitizenPackage );
PRECACHE_WEAPON_REGISTER(weapon_citizenpackage);

acttable_t	CWeaponCitizenPackage::m_acttable[] = 
{
	{ ACT_IDLE,						ACT_IDLE_PACKAGE,					false },
	{ ACT_WALK,						ACT_WALK_PACKAGE,					false },
};
IMPLEMENT_ACTTABLE(CWeaponCitizenPackage);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCitizenPackage::ItemPostFrame( void )
{
	// Do nothing
}

//-----------------------------------------------------------------------------
// Purpose: Remove the citizen package if it's ever dropped
//-----------------------------------------------------------------------------
void CWeaponCitizenPackage::Drop( const Vector &vecVelocity )
{
	BaseClass::Drop( vecVelocity );
	UTIL_Remove( this );
}



//-----------------------------------------------------------------------------
// Purpose: Citizen suitcase
//-----------------------------------------------------------------------------
class CWeaponCitizenSuitcase : public CWeaponCitizenPackage
{
	DECLARE_CLASS( CWeaponCitizenSuitcase, CWeaponCitizenPackage );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();	
	DECLARE_ACTTABLE();
};

IMPLEMENT_SERVERCLASS_ST(CWeaponCitizenSuitcase, DT_WeaponCitizenSuitcase)
END_SEND_TABLE()

BEGIN_DATADESC( CWeaponCitizenSuitcase )
END_DATADESC()

LINK_ENTITY_TO_CLASS( weapon_citizensuitcase, CWeaponCitizenSuitcase );
PRECACHE_WEAPON_REGISTER(weapon_citizensuitcase);

acttable_t	CWeaponCitizenSuitcase::m_acttable[] = 
{
	{ ACT_IDLE,						ACT_IDLE_SUITCASE,					false },
	{ ACT_WALK,						ACT_WALK_SUITCASE,					false },
};
IMPLEMENT_ACTTABLE(CWeaponCitizenSuitcase);
