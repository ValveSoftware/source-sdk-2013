//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef MAPDATA_SHARED_H
#define MAPDATA_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "mathlib/vector.h"

#define INTERFACEVERSION_MAPDATA	"MapData001"

abstract_class IMapData
{
public:

	// World data queries.
	virtual	void	GetMapBounds( Vector &vecMins, Vector &vecMaxs ) = 0;
	virtual void	GetMapOrigin( Vector &vecOrigin ) = 0;
	virtual void	GetMapSize( Vector &vecSize ) = 0;

	// 3D Skybox data queries.
	virtual void	Get3DSkyboxOrigin( Vector &vecOrigin ) = 0;
	virtual float 	Get3DSkyboxScale( void ) = 0;
};

#endif // MAPDATA_SHARED_H
