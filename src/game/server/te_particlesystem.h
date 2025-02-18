//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TE_PARTICLESYSTEM_H
#define TE_PARTICLESYSTEM_H
#ifdef _WIN32
#pragma once
#endif


#include "basetempentity.h"


class CTEParticleSystem : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEParticleSystem, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTEParticleSystem(const char *pName) : BaseClass(pName)
	{
		m_vecOrigin.GetForModify().Init();
	}

	CNetworkVector( m_vecOrigin );
};


#endif // TE_PARTICLESYSTEM_H
