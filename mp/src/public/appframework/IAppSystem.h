//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: An application framework 
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#ifndef IAPPSYSTEM_H
#define IAPPSYSTEM_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/interface.h"


//-----------------------------------------------------------------------------
// Client systems are singleton objects in the client codebase responsible for
// various tasks
// The order in which the client systems appear in this list are the
// order in which they are initialized and updated. They are shut down in
// reverse order from which they are initialized.
//-----------------------------------------------------------------------------

enum InitReturnVal_t
{
	INIT_FAILED = 0,
	INIT_OK,

	INIT_LAST_VAL,
};


abstract_class IAppSystem
{
public:
	// Here's where the app systems get to learn about each other 
	virtual bool Connect( CreateInterfaceFn factory ) = 0;
	virtual void Disconnect() = 0;

	// Here's where systems can access other interfaces implemented by this object
	// Returns NULL if it doesn't implement the requested interface
	virtual void *QueryInterface( const char *pInterfaceName ) = 0;

	// Init, shutdown
	virtual InitReturnVal_t Init() = 0;
	virtual void Shutdown() = 0;
};


//-----------------------------------------------------------------------------
// Helper empty implementation of an IAppSystem
//-----------------------------------------------------------------------------
template< class IInterface > 
class CBaseAppSystem : public IInterface
{
public:
	// Here's where the app systems get to learn about each other 
	virtual bool Connect( CreateInterfaceFn factory ) { return true; }
	virtual void Disconnect() {}

	// Here's where systems can access other interfaces implemented by this object
	// Returns NULL if it doesn't implement the requested interface
	virtual void *QueryInterface( const char *pInterfaceName ) { return NULL; }

	// Init, shutdown
	virtual InitReturnVal_t Init() { return INIT_OK; }
	virtual void Shutdown() {}
};


//-----------------------------------------------------------------------------
// Helper implementation of an IAppSystem for tier0
//-----------------------------------------------------------------------------
template< class IInterface > 
class CTier0AppSystem : public CBaseAppSystem< IInterface >
{
public:
	CTier0AppSystem( bool bIsPrimaryAppSystem = true )
	{
		m_bIsPrimaryAppSystem = bIsPrimaryAppSystem;
	}

protected:
	// NOTE: a single DLL may have multiple AppSystems it's trying to
	// expose. If this is true, you must return true from only
	// one of those AppSystems; not doing so will cause all static
	// libraries connected to it to connect/disconnect multiple times

	// NOTE: We don't do this as a virtual function to avoid
	// having to up the version on all interfaces
	bool IsPrimaryAppSystem() { return m_bIsPrimaryAppSystem; }

private:
	bool m_bIsPrimaryAppSystem;
};


//-----------------------------------------------------------------------------
// This is the version of IAppSystem shipped 10/15/04
// NOTE: Never change this!!!
//-----------------------------------------------------------------------------
abstract_class IAppSystemV0
{
public:
	// Here's where the app systems get to learn about each other 
	virtual bool Connect( CreateInterfaceFn factory ) = 0;
	virtual void Disconnect() = 0;

	// Here's where systems can access other interfaces implemented by this object
	// Returns NULL if it doesn't implement the requested interface
	virtual void *QueryInterface( const char *pInterfaceName ) = 0;

	// Init, shutdown
	virtual InitReturnVal_t Init() = 0;
	virtual void Shutdown() = 0;
};

#endif // IAPPSYSTEM_H

