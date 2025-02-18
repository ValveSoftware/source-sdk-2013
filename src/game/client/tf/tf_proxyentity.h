//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A base class for all material proxies in the tf client dll
//
// $NoKeywords: $
//=============================================================================//
#ifndef TF_PROXY_ENTITY_H
#define TF_PROXY_ENTITY_H

#include "proxyentity.h"

//-----------------------------------------------------------------------------
// Base class all translucent material proxies should inherit from
//-----------------------------------------------------------------------------
abstract_class CBaseInvisMaterialProxy : public CEntityMaterialProxy
{
public:
	CBaseInvisMaterialProxy();

	virtual bool Init( IMaterial *pMaterial, KeyValues* pKeyValues ) OVERRIDE;	
	virtual void Release() OVERRIDE;
	virtual IMaterial* GetMaterial() OVERRIDE;
protected:
	virtual void OnBindNotEntity( void *pRenderable ) OVERRIDE;
	IMaterialVar *m_pPercentInvisible;
};

#endif // TF_PROXY_ENTITY_H
