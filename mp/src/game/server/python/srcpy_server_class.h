//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#if !defined( SRCPY_SERVER_CLASS_H )
#define SRCPY_SERVER_CLASS_H
#ifdef _WIN32
#pragma once
#endif

#include "server_class.h"
#include "dt_send.h"
#include "srcpy_class_shared.h"

class NetworkedClass;
class CBasePlayer;

#define IMPLEMENT_PYSERVERCLASS_SYSTEM( name, network_name ) PyServerClass name(#network_name);	

class PyServerClass : public ServerClass
{
public:
	PyServerClass(char *pNetworkName);

	void SetupServerClass( int iType );

public:
	SendProp *m_pSendProps;
	PyServerClass *m_pPyNext;
	bool m_bFree;
	int m_iType;
	NetworkedClass *m_pNetworkedClass;
};

// NetworkedClass is exposed to python and deals with getting a server/client class and cleaning up
class NetworkedClass
{
public:
	NetworkedClass( const char *pNetworkName, boost::python::object cls_type );
	~NetworkedClass();

	void SetupServerClass();

public:
	boost::python::object m_PyClass;
	const char *m_pNetworkName;

	PyServerClass *m_pServerClass;
};

void FullClientUpdatePyNetworkCls( CBasePlayer *pPlayer );
void FullClientUpdatePyNetworkClsByFilter( IRecipientFilter &filter );
void FullClientUpdatePyNetworkClsByEdict( edict_t *pEdict );

typedef struct EntityInfoOnHold {
	CBaseEntity *ent;
	edict_t *edict;
} EntityInfoOnHold;

void SetupNetworkTables();
void SetupNetworkTablesOnHold();
void AddSetupNetworkTablesOnHoldEnt( EntityInfoOnHold info );
bool SetupNetworkTablesRelease();
void PyResetAllNetworkTables();

// Implement a python class. For python/c++ handle conversion
#define DECLARE_PYCLASS( name )																		\
	public:																							\
	virtual boost::python::object CreatePyHandle( void ) const										\
	{																								\
		return CreatePyHandleHelper(this, #name "HANDLE");											\
	}

// Implement a networkable python class. Used to determine the right recv/send tables
#define DECLARE_PYSERVERCLASS( name, networkType )													\
	DECLARE_PYCLASS( name )																			\
	public:																							\
	static int GetPyNetworkType() { return networkType; }

#endif // SRCPY_SERVER_CLASS_H