//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_basehlcombatweapon.h"
#include "igamemovement.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_HLMachineGun, DT_HLMachineGun, CHLMachineGun )
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT( C_HLSelectFireMachineGun, DT_HLSelectFireMachineGun, CHLSelectFireMachineGun )
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT( C_BaseHLBludgeonWeapon, DT_BaseHLBludgeonWeapon, CBaseHLBludgeonWeapon )
END_RECV_TABLE()