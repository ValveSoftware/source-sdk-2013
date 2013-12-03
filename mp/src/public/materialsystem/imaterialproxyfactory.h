//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IMATERIALPROXYFACTORY_H
#define IMATERIALPROXYFACTORY_H
#pragma once

#include "interface.h"

#define IMATERIAL_PROXY_FACTOR_INTERFACE_VERSION "IMaterialProxyFactory001"

class IMaterialProxy;

abstract_class IMaterialProxyFactory
{
public:
	virtual IMaterialProxy *CreateProxy( const char *proxyName ) = 0;
	virtual void DeleteProxy( IMaterialProxy *pProxy ) = 0;
};

#endif // IMATERIALPROXYFACTORY_H
