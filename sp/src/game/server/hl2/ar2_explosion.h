//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef AR2_EXPLOSION_H
#define AR2_EXPLOSION_H


#include "baseparticleentity.h"


class AR2Explosion : public CBaseParticleEntity
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( AR2Explosion, CBaseParticleEntity );
	DECLARE_SERVERCLASS();

	static AR2Explosion* CreateAR2Explosion(const Vector &pos);

	inline void SetMaterialName(const char *szMaterialName);

private:

	CNetworkString( m_szMaterialName, 255 );
};


void AR2Explosion::SetMaterialName(const char *szMaterialName)
{
	if (szMaterialName)
	{
		Q_strncpy(m_szMaterialName.GetForModify(), szMaterialName, sizeof(m_szMaterialName));
	}
}


#endif
