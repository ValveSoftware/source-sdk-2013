//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include <KeyValues.h>
#include "hl2mp_weapon_parse.h"
#include "ammodef.h"

FileWeaponInfo_t* CreateWeaponInfo()
{
	return new CHL2MPSWeaponInfo;
}



CHL2MPSWeaponInfo::CHL2MPSWeaponInfo()
{
	m_iPlayerDamage = 0;
}


void CHL2MPSWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );

	m_iPlayerDamage = pKeyValuesData->GetInt( "damage", 0 );
	
	#ifdef SecobMod__IRONSIGHT_ENABLED
	   // this just saves off the data in the script file for later use
	   KeyValues *pEt = pKeyValuesData->FindKey("ExpOffset");
	   if (pEt)
	   {
			m_expOffset.x     = pEt->GetFloat("x", 0.0f);
			m_expOffset.y     = pEt->GetFloat("y", 0.0f);
			m_expOffset.z     = pEt->GetFloat("z", 0.0f);
	 
			m_expOriOffset.x  = pEt->GetFloat("xori", 0.0f);
			m_expOriOffset.y  = pEt->GetFloat("yori", 0.0f);
			m_expOriOffset.z  = pEt->GetFloat("zori", 0.0f);
	   }
	   else
	   {
	      m_expOffset = vec3_origin;
	      m_expOriOffset.Init();
	   }
	#endif //SecobMod__IRONSIGHT_ENABLED
}


