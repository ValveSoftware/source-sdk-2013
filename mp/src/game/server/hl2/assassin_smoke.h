//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef ASSASSIN_SMOKE_H
#define ASSASSIN_SMOKE_H


#include "baseparticleentity.h"


class CAssassinSmoke : public CBaseParticleEntity
{
public:
	DECLARE_CLASS( CAssassinSmoke, CBaseParticleEntity );
	DECLARE_SERVERCLASS();

	static CAssassinSmoke* CreateAssassinSmoke (const Vector &pos);
};


#endif//ASSASSIN_SMOKE_H


