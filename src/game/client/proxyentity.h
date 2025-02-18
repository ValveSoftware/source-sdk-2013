//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A base class for all material proxies in the client dll
//
// $NoKeywords: $
//=============================================================================//

#ifndef PROXY_ENTITY_H
#define PROXY_ENTITY_H

#include "materialsystem/imaterialproxy.h"


class IMaterialVar;

//-----------------------------------------------------------------------------
// Base class all material proxies should inherit from
//-----------------------------------------------------------------------------
abstract_class CEntityMaterialProxy : public IMaterialProxy
{
public:
	virtual void Release( void );
	virtual void OnBind( void *pC_BaseEntity );

protected:
	// base classes should implement these
	virtual void OnBind( C_BaseEntity *pBaseEntity ) = 0;
	virtual void OnBindNotEntity( void *pRenderable ) {}
};

#endif // PROXY_ENTITY_H

