//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Deals with precaching requests from client effects
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "fx.h"
#include "clienteffectprecachesystem.h"
#include "particles/particles.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//Global singelton accessor
CClientEffectPrecacheSystem	*ClientEffectPrecacheSystem( void )
{
	static CClientEffectPrecacheSystem	s_ClientEffectPrecacheSystem;
	return &s_ClientEffectPrecacheSystem;
}

//-----------------------------------------------------------------------------
// Purpose: Precache all the registered effects
//-----------------------------------------------------------------------------
void CClientEffectPrecacheSystem::LevelInitPreEntity( void )
{
	//Precache all known effects
	for ( int i = 0; i < m_Effects.Size(); i++ )
	{
		m_Effects[i]->Cache();
	}
	
	//FIXME: Double check this
	//Finally, force the cache of these materials
	materials->CacheUsedMaterials();

	// Now, cache off our material handles
	FX_CacheMaterialHandles();
}

//-----------------------------------------------------------------------------
// Purpose: Nothing to do here
//-----------------------------------------------------------------------------
void CClientEffectPrecacheSystem::LevelShutdownPreEntity( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Dereference all the registered effects
//-----------------------------------------------------------------------------
void CClientEffectPrecacheSystem::LevelShutdownPostEntity( void )
{
	// mark all known effects as free
	for ( int i = 0; i < m_Effects.Size(); i++ )
	{
		m_Effects[i]->Cache( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Purges the effect list
//-----------------------------------------------------------------------------
void CClientEffectPrecacheSystem::Shutdown( void )
{
	//Release all effects
	m_Effects.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Adds the effect to the list to be precached
// Input  : *effect - system to precache
//-----------------------------------------------------------------------------
void CClientEffectPrecacheSystem::Register( IClientEffect *effect )
{
	//Hold onto this effect for precaching later
	m_Effects.AddToTail( effect );
}
