//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#if !defined( IEFX_H )
#define IEFX_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "mathlib/vector.h"

struct model_t;
struct dlight_t;
class IMaterial;

#define	MAX_DLIGHTS		32

//-----------------------------------------------------------------------------
// Purpose: Exposes effects api to client .dll
//-----------------------------------------------------------------------------
abstract_class IVEfx
{
public:
	// Retrieve decal texture index from decal by name
	virtual	int				Draw_DecalIndexFromName	( char *name ) = 0;

	// Apply decal
	virtual	void			DecalShoot				( int textureIndex, int entity, 
		const model_t *model, const Vector& model_origin, const QAngle& model_angles, 
		const Vector& position, const Vector *saxis, int flags ) = 0;

	// Apply colored decal
	virtual	void			DecalColorShoot				( int textureIndex, int entity, 
		const model_t *model, const Vector& model_origin, const QAngle& model_angles, 
		const Vector& position, const Vector *saxis, int flags, const color32 &rgbaColor  ) = 0;

	virtual void			PlayerDecalShoot( IMaterial *material, void *userdata, int entity, const model_t *model, 
		const Vector& model_origin, const QAngle& model_angles, 
		const Vector& position, const Vector *saxis, int flags, const color32 &rgbaColor ) = 0;

	// Allocate a dynamic world light ( key is the entity to whom it is associated )
	virtual	dlight_t	*CL_AllocDlight			( int key ) = 0;

	// Allocate a dynamic entity light ( key is the entity to whom it is associated )
	virtual	dlight_t	*CL_AllocElight			( int key ) = 0;

	// Get a list of the currently-active dynamic lights.
	virtual int CL_GetActiveDLights( dlight_t *pList[MAX_DLIGHTS] ) = 0;

	// Retrieve decal texture name from decal by index
	virtual	const char *Draw_DecalNameFromIndex( int nIndex ) = 0;

	// Given an elight key, find it. Does not search ordinary dlights. May return NULL.
	virtual dlight_t   *GetElightByKey( int key ) = 0;
};

#define VENGINE_EFFECTS_INTERFACE_VERSION "VEngineEffects001"

extern IVEfx *effects;

#endif // IEFX_H
