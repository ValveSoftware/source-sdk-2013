//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef MOVIE_EXPLOSION_H
#define MOVIE_EXPLOSION_H


#include "baseparticleentity.h"


class MovieExplosion : public CBaseParticleEntity
{
public:
	DECLARE_CLASS( MovieExplosion, CBaseParticleEntity );
	DECLARE_SERVERCLASS();

	static MovieExplosion* CreateMovieExplosion(const Vector &pos);
};


#endif


