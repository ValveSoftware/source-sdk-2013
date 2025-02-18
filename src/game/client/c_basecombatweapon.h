//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's CBaseCombatWeapon entity
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#ifndef C_BASECOMBATWEAPON_H
#define C_BASECOMBATWEAPON_H

#ifdef _WIN32
#pragma once
#endif

#include "basecombatweapon_shared.h"
#include "weapons_resource.h"

class CViewSetup;
class C_BaseViewModel;

// Accessors for local weapons
C_BaseCombatWeapon *GetActiveWeapon( void );


#endif // C_BASECOMBATWEAPON
