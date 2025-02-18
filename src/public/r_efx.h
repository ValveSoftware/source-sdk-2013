//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#if !defined ( EFXH )
#define EFXH
#ifdef _WIN32
#pragma once
#endif

#include "iefx.h"

class IMaterial;
struct dlight_t;

class CVEfx : public IVEfx
{
public:
	virtual ~CVEfx() {}

	virtual int			Draw_DecalIndexFromName	( char *name );
	virtual void		DecalShoot				( int textureIndex, int entity, const model_t *model, const Vector& model_origin, const QAngle& model_angles, const Vector& position, const Vector *saxis, int flags);
	virtual void		DecalColorShoot			( int textureIndex, int entity, const model_t *model, const Vector& model_origin, const QAngle& model_angles, const Vector& position, const Vector *saxis, int flags, const color32 &rgbaColor);
	virtual void		PlayerDecalShoot		( IMaterial *material, void *userdata, int entity, const model_t *model, const Vector& model_origin, const QAngle& model_angles, 
													const Vector& position, const Vector *saxis, int flags, const color32 &rgbaColor );
	virtual dlight_t	*CL_AllocDlight			( int key );
	virtual dlight_t	*CL_AllocElight			( int key );
	virtual int			CL_GetActiveDLights		( dlight_t *pList[MAX_DLIGHTS] );
	virtual const char *Draw_DecalNameFromIndex	( int nIndex );
	virtual dlight_t    *GetElightByKey			( int key );
};

extern CVEfx *g_pEfx;

#endif
