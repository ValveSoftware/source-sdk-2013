//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Client classes for Half-Life 2 based custom weapons.
// 
// Author: Peter Covington (petercov@outlook.com)
//
//==================================================================================//

#include "cbase.h"
#include "c_weapon__stubs.h"
#include "basehlcombatweapon_shared.h"
#include "c_basehlcombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_HLCustomWeaponMelee : public C_BaseHLBludgeonWeapon
{
public:
	DECLARE_CLASS(C_HLCustomWeaponMelee, C_BaseHLBludgeonWeapon);
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	C_HLCustomWeaponMelee();

	void OnDataChanged( DataUpdateType_t updateType );

	virtual const char* GetWeaponScriptName() { return m_iszWeaponScriptName; }
private:
	char m_iszWeaponScriptName[128];
};

STUB_WEAPON_CLASS_IMPLEMENT(weapon_hlcustommelee, C_HLCustomWeaponMelee);

IMPLEMENT_CLIENTCLASS_DT(C_HLCustomWeaponMelee, DT_HLCustomWeaponMelee, CHLCustomWeaponMelee)
RecvPropString(RECVINFO(m_iszWeaponScriptName)),
END_RECV_TABLE();

C_HLCustomWeaponMelee::C_HLCustomWeaponMelee()
{
	m_iszWeaponScriptName[0] = '\0';
}

void C_HLCustomWeaponMelee::OnDataChanged( DataUpdateType_t updateType )
{
	if (updateType == DATA_UPDATE_CREATED)
	{
		Precache();
	}

	BaseClass::OnDataChanged( updateType );
}



class C_HLCustomWeaponGun : public C_BaseHLCombatWeapon
{
public:
	DECLARE_CLASS(C_HLCustomWeaponGun, C_BaseHLCombatWeapon);
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	C_HLCustomWeaponGun();

	void OnDataChanged( DataUpdateType_t updateType );

	virtual const char* GetWeaponScriptName() { return m_iszWeaponScriptName; }
private:
	char m_iszWeaponScriptName[128];
};

STUB_WEAPON_CLASS_IMPLEMENT(weapon_hlcustomgun, C_HLCustomWeaponGun);

IMPLEMENT_CLIENTCLASS_DT(C_HLCustomWeaponGun, DT_HLCustomWeaponGun, CHLCustomWeaponGun)
RecvPropString(RECVINFO(m_iszWeaponScriptName)),
END_RECV_TABLE();

C_HLCustomWeaponGun::C_HLCustomWeaponGun()
{
	m_iszWeaponScriptName[0] = '\0';
}

void C_HLCustomWeaponGun::OnDataChanged( DataUpdateType_t updateType )
{
	if (updateType == DATA_UPDATE_CREATED)
	{
		Precache();
	}

	BaseClass::OnDataChanged( updateType );
}
