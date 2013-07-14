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
#if !defined( SRCPY_CLIENT_CLASS_H )
#define SRCPY_CLIENT_CLASS_H
#ifdef _WIN32
#pragma once
#endif

#include "client_class.h"
#include "srcpy_class_shared.h"

class NetworkedClass;

// Forward declaration
class PyClientClassBase;

// Class head 
extern PyClientClassBase *g_pPyClientClassHead;

namespace DT_BaseEntity
{
	extern RecvTable g_RecvTable;
}

// Use as base for client networked entities
class PyClientClassBase : public ClientClass
{
public:
	PyClientClassBase( char *pNetworkName ) : ClientClass(pNetworkName, NULL, NULL, NULL), m_pNetworkedClass(NULL)
	{
		// Default send table to BaseEntity
		m_pRecvTable = &(DT_BaseEntity::g_RecvTable);
		m_pPyNext				= g_pPyClientClassHead;
		g_pPyClientClassHead	= this;
		m_bFree = true;
	}

	virtual void SetPyClass( boost::python::object cls_type ) {}
	virtual void SetType( int iType ) {}
	virtual void InitPyClass() {}

public:
	PyClientClassBase *m_pPyNext;
	bool m_bFree;
	NetworkedClass *m_pNetworkedClass;
	char m_strPyNetworkedClassName[512];
};

void SetupClientClassRecv( PyClientClassBase *p, int iType );
void PyResetAllNetworkTables();
IClientNetworkable *ClientClassFactory( int iType, boost::python::object cls_type, int entnum, int serialNum);
void InitAllPythonEntities();
void CheckEntities(PyClientClassBase *pCC, boost::python::object pyClass );

// For each available free client class we need a unique class because of the static data
// No way around this since stuff is called from the engine
#define IMPLEMENT_PYCLIENTCLASS_SYSTEM( name, network_name )										\
	class name : public PyClientClassBase															\
	{																								\
	public:																							\
		name() : PyClientClassBase( #network_name )													\
		{																							\
			m_pCreateFn = PyClientClass_CreateObject;												\
		}																							\
		static IClientNetworkable* PyClientClass_CreateObject( int entnum, int serialNum );			\
		virtual void SetPyClass( boost::python::object cls_type );									\
		virtual void SetType( int iType );															\
		virtual void InitPyClass();																	\
	public:																							\
		static boost::python::object m_PyClass;														\
		static int m_iType;																			\
	};																								\
	boost::python::object name##::m_PyClass;														\
	int name##::m_iType;																			\
	IClientNetworkable* name##::PyClientClass_CreateObject( int entnum, int serialNum )				\
	{																								\
		return ClientClassFactory(m_iType, m_PyClass, entnum, serialNum);							\
	}																								\
	void name##::SetPyClass( boost::python::object cls_type )										\
	{																								\
		m_PyClass = cls_type;																		\
		if( g_bDoNotInitPythonClasses == false)														\
			InitPyClass();																			\
		CheckEntities(this, m_PyClass);																\
	}																								\
	void name##::SetType( int iType )																\
	{																								\
		m_iType = iType;																			\
	}																								\
	void name##::InitPyClass()																		\
	{																								\
		bp::object meth = SrcPySystem()->Get("InitEntityClass", m_PyClass, false);					\
		if( meth.ptr() == Py_None )																	\
			return;																					\
		SrcPySystem()->Run( meth );																	\
	}																								\
	name name##_object;																				\

// NetworkedClass is exposed to python and deals with getting a server/client class and cleaning up
class NetworkedClass
{
public:
	NetworkedClass( const char *pNetworkName, boost::python::object cls_type, const char *pClientModuleName=0 );
	~NetworkedClass();

	void AttachClientClass( PyClientClassBase *pClientClass );

public:
	const char *m_pNetworkName;
	PyClientClassBase *m_pClientClass;
	boost::python::object m_pyClass;
};

// Implement a python class. For python/c++ handle conversion
#define DECLARE_PYCLASS( name )																		\
	public:																							\
	inline boost::python::object CreatePyHandle( void ) const										\
{																									\
	return CreatePyHandleHelper(this, #name "HANDLE");												\
}

// Implement a networkable python class. Used to determine the right recv/send tables
#define DECLARE_PYCLIENTCLASS( name )																\
	DECLARE_PYCLASS( name )																			\
	public:																							\
	static int GetPyNetworkType() { return networkType; }

#endif // SRCPY_CLIENT_CLASS_H