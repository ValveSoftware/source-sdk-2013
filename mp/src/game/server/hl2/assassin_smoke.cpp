//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "assassin_smoke.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define ASSASSINSMOKE_ENTITYNAME	"env_assassinsmoke"


IMPLEMENT_SERVERCLASS_ST(CAssassinSmoke, DT_AssassinSmoke)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(env_assassinsmoke, CAssassinSmoke);


CAssassinSmoke* CAssassinSmoke::CreateAssassinSmoke(const Vector &pos)
{
	CBaseEntity *pEnt = CreateEntityByName(ASSASSINSMOKE_ENTITYNAME);
	if(pEnt)
	{
		CAssassinSmoke *pEffect = dynamic_cast<CAssassinSmoke*>(pEnt);
		if (pEffect && pEffect->edict())
		{
			pEffect->SetLocalOrigin( pos );
			pEffect->Activate();
			return pEffect;
		}
		else
		{
			UTIL_Remove(pEnt);
		}
	}

	return NULL;
}


