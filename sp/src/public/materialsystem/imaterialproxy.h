//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IMATERIALPROXY_H
#define IMATERIALPROXY_H
#pragma once

#include "interface.h"

#define IMATERIAL_PROXY_INTERFACE_VERSION "_IMaterialProxy003"

class IMaterial;
class KeyValues;

abstract_class IMaterialProxy
{
public:
	virtual bool Init( IMaterial* pMaterial, KeyValues *pKeyValues ) = 0;
	virtual void OnBind( void * ) = 0;
	virtual void Release() = 0;
	virtual IMaterial *	GetMaterial() = 0;

protected:
	// no one should call this directly
	virtual ~IMaterialProxy() {}
};

#endif // IMATERIALPROXY_H
