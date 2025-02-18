//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Dove entity for the Meet the Medic tease.
//
//=============================================================================//
#ifndef ENTITY_BIRD_H
#define ENTITY_BIRD_H

#ifdef _WIN32
#pragma once
#endif

#include "baseanimating.h"

//=============================================================================
//
// Dove entity for the Meet the Medic tease.
//

class CEntityBird : public CBaseAnimating
{
	DECLARE_CLASS( CEntityBird, CBaseAnimating );
public:
	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual void	Touch( CBaseEntity *pOther ); 
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );

	// Spawn random birds at locations on certain maps.
	static void		SpawnRandomBirds( void );

private:
	void			Explode( void );
};

#endif // ENTITY_BIRD_H


