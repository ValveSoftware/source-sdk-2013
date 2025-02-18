//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base class for object upgrading objects
//
//=============================================================================//
#include "cbase.h"
#include "baseobject_shared.h"
#include "tf_obj_baseupgrade_shared.h"

IMPLEMENT_NETWORKCLASS_ALIASED( BaseObjectUpgrade, DT_BaseObjectUpgrade )

BEGIN_NETWORK_TABLE( CBaseObjectUpgrade, DT_BaseObjectUpgrade )
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseObjectUpgrade::CBaseObjectUpgrade()
{
	UseClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObjectUpgrade::Spawn()
{
#if !defined( CLIENT_DLL )
     m_fObjectFlags.Set( m_fObjectFlags | OF_MUST_BE_BUILT_ON_ATTACHMENT );
#endif

	BaseClass::Spawn();

#if !defined( CLIENT_DLL )
	SetCollisionGroup( TFCOLLISION_GROUP_COMBATOBJECT );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Prevent Team Damage
//-----------------------------------------------------------------------------
int CBaseObjectUpgrade::OnTakeDamage( const CTakeDamageInfo &info )
{
#if !defined( CLIENT_DLL )
	return BaseClass::OnTakeDamage( info );
#else
	return 0;
#endif
}
