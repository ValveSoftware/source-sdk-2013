//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_ENTITES_H
#define SRCPY_ENTITES_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"
#include "ehandle.h"
#ifndef CLIENT_DLL
	#include "physics_bone_follower.h"
	//#include "srcpy_physics.h"

	class CBaseAnimating;
	class CRagdollProp;
#endif // CLIENT_DLL

#include <boost/python.hpp>

extern boost::python::object _entities;

//-----------------------------------------------------------------------------
// Purpose: Python entity handle
//-----------------------------------------------------------------------------
template< class T >
class CEPyHandle : public CHandle<T>
{
public:
	CEPyHandle() : CHandle<T>() {}
	CEPyHandle( T *pVal ) : CHandle<T>(pVal) {}
	CEPyHandle( int iEntry, int iSerialNumber ) : CHandle<T>(iEntry, iSerialNumber) {}

	boost::python::object GetAttr( const char *name );

	int Cmp( boost::python::object other );
	bool NonZero();
};

template< class T >
inline boost::python::object CEPyHandle<T>::GetAttr( const char *name )
{
	return boost::python::object(boost::python::ptr(Get())).attr(name);
}

template< class T >
int CEPyHandle<T>::Cmp( boost::python::object other )
{
	// The thing to which we compare is NULL
	PyObject *pPyObject = other.ptr();
	if( pPyObject == Py_None ) {
		return Get() != NULL;
	}

	// We are NULL
	if( Get() == NULL )
	{
		return pPyObject != NULL;
	}

	// Check if it is directly a pointer to an entity
#ifdef CLIENT_DLL
	if( PyObject_IsInstance(pPyObject, boost::python::object(_entities.attr("C_BaseEntity")).ptr()) )
#else
	if( PyObject_IsInstance(pPyObject, boost::python::object(_entities.attr("CBaseEntity")).ptr()) )
#endif // CLIENT_DLL
	{
		CBaseEntity *pSelf = Get();
		CBaseEntity *pOther = boost::python::extract<CBaseEntity *>(other);
		if( pOther == pSelf )
		{
			return 0;
		}
		else if( pOther->entindex() > pSelf->entindex() )
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}

	// Must be a handle
	CBaseHandle *pHandle = boost::python::extract<CBaseHandle *>( other );
	if( pHandle )
	{
		if( pHandle->ToInt() == ToInt() )
			return 0;
		else if( pHandle->GetEntryIndex() > GetEntryIndex() )
			return 1;
		else
			return -1;
	}

	return -1;
}

template< class T >
inline bool CEPyHandle<T>::NonZero()
{
	return Get() != NULL;
}

//----------------------------------------------------------------------------
// Purpose: Python entity handle, for python entities only
//-----------------------------------------------------------------------------
class PyHandle : public CBaseHandle
{
public:
	PyHandle(boost::python::object ent);
	PyHandle( int iEntry, int iSerialNumber ) : CBaseHandle(iEntry, iSerialNumber) {}

public:
	CBaseEntity *Get() const;
	boost::python::object PyGet() const;
	void Set( boost::python::object ent );

	bool	operator==( boost::python::object val ) const;
	bool	operator!=( boost::python::object val ) const;
	bool	operator==( const PyHandle &val ) const;
	bool	operator!=( const PyHandle &val ) const;
	const PyHandle& operator=( boost::python::object val );

	boost::python::object GetAttr( const char *name );
	boost::python::object GetAttribute( const char *name );
	void SetAttr( const char *name, boost::python::object v );

	int Cmp( boost::python::object other );
	bool NonZero() { return PyGet().ptr() != Py_None; }

	virtual PyObject *GetPySelf() { return NULL; }

	boost::python::object Str();
};

boost::python::object CreatePyHandle( int iEntry, int iSerialNumber );

#ifndef CLIENT_DLL
boost::python::object PyGetWorldEntity();
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Dead python entity. The __class__ object of a removed entity gets 
//			rebinded to this. This way you can't accidently access most of the
//			(potential) dangerous methods.
//-----------------------------------------------------------------------------
class DeadEntity 
{
public:
	bool NonZero() { return false; }
};

#ifdef CLIENT_DLL
// ----------- Linking python classes to entities
class PyEntityFactory 
{
public:
	PyEntityFactory( const char *pClassName, boost::python::object PyClass );
	~PyEntityFactory();

	C_BaseEntity *Create();

	boost::python::object GetClass() { return m_PyClass; }

private:
	char m_ClassName[128];
	boost::python::object m_PyClass;
};

#else
// ----------- Linking python classes to entities
class PyEntityFactory : public IEntityFactory
{
public:
	PyEntityFactory( const char *pClassName, boost::python::object PyClass );
	~PyEntityFactory();

	IServerNetworkable *Create( const char *pClassName );

	void Destroy( IServerNetworkable *pNetworkable );

	virtual size_t GetEntitySize();

	void InitPyClass();
	virtual bool IsPyFactory() { return true; }

	boost::python::object GetClass() { return m_PyClass; }
	const char *GetClassname() { return m_ClassName; }

private:
	void CheckEntities();

public:
	PyEntityFactory *m_pPyNext;

private:
	char m_ClassName[128];
	boost::python::object m_PyClass;
};
void InitAllPythonEntities();
#endif

boost::python::object PyGetClassByClassname( const char *class_name );
boost::python::list PyGetAllClassnames();

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Sending events to client
//-----------------------------------------------------------------------------
void PySendEvent( IRecipientFilter &filter, EHANDLE ent, int event, int data);

//-----------------------------------------------------------------------------
// Purpose: (Re)spawn a player using the specified class
//-----------------------------------------------------------------------------
boost::python::object PyRespawnPlayer( CBasePlayer *pPlayer, const char *classname );

//-----------------------------------------------------------------------------
// Purpose: PyOutputEvent
//-----------------------------------------------------------------------------
class PyOutputEvent : public CBaseEntityOutput
{
public:
	PyOutputEvent();

	void Set( variant_t value );

	// void Firing, no parameter
	void FireOutput( CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay = 0 );
	void FireOutput( variant_t Value, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay = 0 ) 
	{ 
		CBaseEntityOutput::FireOutput( Value, pActivator, pCaller, fDelay ); 
	}
};

//-----------------------------------------------------------------------------
// Purpose: Bone followers
//-----------------------------------------------------------------------------
Vector GetAttachmentPositionInSpaceOfBone( CStudioHdr *pStudioHdr, const char *pAttachmentName, int outputBoneIndex );

typedef struct pyphysfollower_t
{
	int boneindex;
	boost::python::object follower;
} pyphysfollower_t;

class PyBoneFollowerManager : public CBoneFollowerManager
{
public:
	// Use either of these to create the bone followers in your entity's CreateVPhysics()
	void InitBoneFollowers( CBaseAnimating *pParentEntity, boost::python::list followerbonenames );
	void AddBoneFollower( CBaseAnimating *pParentEntity, const char *pFollowerBoneName, solid_t *pSolid = NULL );	// Adds a single bone follower

	// Call this after you move your bones
	void UpdateBoneFollowers( CBaseAnimating *pParentEntity );

	// Call this when your entity's removed
	void DestroyBoneFollowers( void );

	pyphysfollower_t GetBoneFollower( int iFollowerIndex );
	int				GetBoneFollowerIndex( CBoneFollower *pFollower );
	int				GetNumBoneFollowers( void ) const { return CBoneFollowerManager::GetNumBoneFollowers(); }
};

Vector GetAttachmentPositionInSpaceOfBone( CStudioHdr *pStudioHdr, const char *pAttachmentName, int outputBoneIndex );

//CRagdollProp *PyCreateServerRagdollAttached( CBaseAnimating *pAnimating, const Vector &vecForce, int forceBone, int collisionGroup, PyPhysicsObject &pyAttached, CBaseAnimating *pParentEntity, int boneAttach, const Vector &originAttached, int parentBoneAttach, const Vector &boneOrigin );

#endif // CLIENT_DLL

#endif // SRCPY_ENTITES_H