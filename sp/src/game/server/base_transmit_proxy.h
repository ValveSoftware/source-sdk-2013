//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef BASE_TRANSMIT_PROXY_H
#define BASE_TRANSMIT_PROXY_H
#ifdef _WIN32
#pragma once
#endif


#include "ehandle.h"


class CBaseEntity;


class CBaseTransmitProxy
{
public:

	CBaseTransmitProxy( CBaseEntity *pEnt );
	virtual ~CBaseTransmitProxy();

	// Override this to control the ShouldTransmit behavior of whatever entity the proxy is attached to.
	// bPrevShouldTransmitResult is what the proxy's entity's ShouldTransmit() returned.
	virtual int ShouldTransmit( const CCheckTransmitInfo *pInfo, int nPrevShouldTransmitResult );


	void AddRef();
	void Release();

private:
	EHANDLE m_hEnt;
	unsigned short m_refCount;
};


#endif // BASE_TRANSMIT_PROXY_H
