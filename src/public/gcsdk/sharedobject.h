//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base class for objects that are kept in synch between client and server
//
//=============================================================================

#ifndef SHAREDOBJECT_H
#define SHAREDOBJECT_H
#ifdef _WIN32
#pragma once
#endif

// ENABLE_SO_OVERWRITE_PARANOIA can be set to either 0 or 1. If enabled, it will add
// extra fields to every CSharedObject instance to try and detect overwrites at the
// cost of additional runtime memory.
#define ENABLE_SO_OVERWRITE_PARANOIA				0

// ENABLE_SO_CONSTRUCT_DESTRUCT_PARANOIA can be set to either 0 or 1. If enabled, it
// will add extra fields to every CSharedObject instance to try and detect issues with
// constructions/destruction (ie., double-deletes, etc.), including reference counting.
#define ENABLE_SO_CONSTRUCT_DESTRUCT_PARANOIA		(defined( STAGING_ONLY ))

#include "tier0/memdbgon.h"

namespace GCSDK
{

class CSQLAccess;
class CSharedObject;
typedef CSharedObject *(*SOCreationFunc_t)( );
class CSharedObjectCache;



//----------------------------------------------------------------------------
// Purpose: Abstract base class for objects that are shared between the GC and
//			a gameserver/client. These can also be stored in the database.
//----------------------------------------------------------------------------
class CSharedObject
{
	friend class CGCSharedObjectCache;
	friend class CSharedObjectCache;
public:

	virtual ~CSharedObject() {}

	virtual int GetTypeID() const = 0;
	virtual bool BParseFromMessage( const CUtlBuffer & buffer ) = 0;
	virtual bool BParseFromMessage( const std::string &buffer ) = 0;
	virtual bool BUpdateFromNetwork( const CSharedObject & objUpdate ) = 0;
	virtual bool BIsKeyLess( const CSharedObject & soRHS ) const = 0;
	virtual void Copy( const CSharedObject & soRHS ) = 0;
	virtual void Dump() const = 0;
	virtual bool BShouldDeleteByCache() const { return true; }
	virtual CUtlString GetDebugString() const { return PchClassName( GetTypeID() ); };

	bool BIsKeyEqual( const CSharedObject & soRHS ) const;

	static void RegisterFactory( int nTypeID, SOCreationFunc_t fnFactory, uint32 unFlags, const char *pchClassName );
	static CSharedObject *Create( int nTypeID );
	static uint32 GetTypeFlags( int nTypeID );
	static const char *PchClassName( int nTypeID );
	static const char *PchClassBuildCacheNodeName( int nTypeID );
	static const char *PchClassCreateNodeName( int nTypeID );
	static const char *PchClassUpdateNodeName( int nTypeID );



private:
#if ENABLE_SO_CONSTRUCT_DESTRUCT_PARANOIA
	enum { kSharedObject_UnassignedType = -999 };
#endif // ENABLE_SO_CONSTRUCT_DESTRUCT_PARANOIA

	struct SharedObjectInfo_t
	{
		SOCreationFunc_t m_pFactoryFunction;
		uint32 m_unFlags;
		const char *m_pchClassName;
		CUtlString m_sBuildCacheSubNodeName;
		CUtlString m_sUpdateNodeName;
		CUtlString m_sCreateNodeName;
	};
	static CUtlMap<int, SharedObjectInfo_t> sm_mapFactories;

public:
	static const CUtlMap<int, SharedObjectInfo_t> & GetFactories() { return sm_mapFactories; }
};

typedef CUtlVectorFixedGrowable<CSharedObject *, 1> CSharedObjectVec;



//----------------------------------------------------------------------------
// Purpose: Templatized function to use as a factory method for 
//			CSharedObject subclasses
//----------------------------------------------------------------------------
template<typename SharedObjectSubclass_t>
CSharedObject *CreateSharedObjectSubclass()
{
	return new SharedObjectSubclass_t();
}

// Version that always asserts and returns NULL, for SOs that should not be auto-created this way
template<int nSharedObjectType>
CSharedObject *CreateSharedObjectSubclassProhibited()
{
	AssertMsg( false, "Attempting to auto-create object of type %d which does not allow SO-based creation", nSharedObjectType );
	return NULL;
}

#define REG_SHARED_OBJECT_SUBCLASS( derivedClass ) GCSDK::CSharedObject::RegisterFactory( derivedClass::k_nTypeID, GCSDK::CreateSharedObjectSubclass<derivedClass>, 0, #derivedClass )

} // namespace GCSDK


#include "tier0/memdbgoff.h"

#endif //SHAREDOBJECT_H
