//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy_entities.h"
#include "srcpy.h"

#include "utldict.h"

#ifdef CLIENT_DLL
	#include "c_basetempentity.h"
#else
	#include "basetempentity.h"
	#include "bone_setup.h"
	#include "physics_prop_ragdoll.h"
	#include "world.h"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace bp = boost::python;

// -------------------------------------------------------------------------------
PyHandle::PyHandle(bp::object ent)
{
	Term();
	Set( ent );
}

CBaseEntity *PyHandle::Get() const
{
	return (CBaseEntity*)CBaseHandle::Get();
}

bp::object PyHandle::PyGet() const
{
	CBaseEntity *pEnt = Get();
	return pEnt ? pEnt->GetPyInstance() : bp::object();
}

void PyHandle::Set( bp::object ent )
{
	if( ent.ptr() == Py_None )
	{
		CBaseHandle::Set( NULL );
		return;
	}

	CBaseEntity *pEnt = NULL;
	try 
	{
		pEnt = bp::extract<CBaseEntity *>(ent);
	} 
	catch( bp::error_already_set & ) 
	{
		PyErr_Print();
		PyErr_Clear();
		return;
	}

	CBaseHandle::Set( reinterpret_cast<const IHandleEntity*>(pEnt) );
}

bool PyHandle::operator==( const PyHandle &val ) const
{
	return PyGet().ptr() == val.PyGet().ptr();
}

bool PyHandle::operator!=( const PyHandle &val ) const
{
	return PyGet().ptr() != val.PyGet().ptr();
}

bool PyHandle::operator==( bp::object val ) const
{
	return PyGet().ptr() == val.ptr();
}

bool PyHandle::operator!=(  bp::object val ) const
{
	return PyGet().ptr() != val.ptr();
}

const PyHandle& PyHandle::operator=( const bp::object val )
{
	Set( val );
	return *this;
}

bp::object PyHandle::GetAttr( const char *name )
{
	if( hasattr(PyGet(), name) )
		return PyGet().attr(name);

	boost::python::object self = boost::python::object(
		boost::python::handle<>(
		boost::python::borrowed(GetPySelf())
		)
		);
	Assert( self.ptr() != NULL );
	return builtins.attr("object").attr("__getattr__")(self, name);	
}

bp::object PyHandle::GetAttribute( const char *name )
{
	if( hasattr(PyGet(), name) )
		return PyGet().attr("__getattribute__")(name);
	boost::python::object self = boost::python::object(
		boost::python::handle<>(
		boost::python::borrowed(GetPySelf())
		)
	);
	Assert( self.ptr() != NULL );
	return builtins.attr("object").attr("__getattribute__")(self, name);	
}

void PyHandle::SetAttr( const char *name, bp::object v )
{
	PyGet().attr("__setattr__")(name, v);
}

int PyHandle::Cmp( bp::object other )
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
	if( PyObject_IsInstance(pPyObject, bp::object(_entities.attr("C_BaseEntity")).ptr()) )
#else
	if( PyObject_IsInstance(pPyObject, bp::object(_entities.attr("CBaseEntity")).ptr()) )
#endif // CLIENT_DLL
	{
		CBaseEntity *pSelf = Get();
		CBaseEntity *pOther = boost::python::extract< CBaseEntity * >(other);
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
	CBaseHandle *pHandle = bp::extract< CBaseHandle * >( other );
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

boost::python::object PyHandle::Str()
{
	if( Get() )
	{
		bp::object pyinst = PyGet();
		return pyinst.attr("__str__")();
	}
	return boost::python::object( "None" );
}

bp::object CreatePyHandle( int iEntry, int iSerialNumber )
{
	try 
	{
		boost::python::object clshandle;
		clshandle = _entities.attr("PyHandle");
		return clshandle(iEntry, iSerialNumber);
	}
	catch( bp::error_already_set & )
	{
		PyErr_Print();	
	}
	return bp::object();
}

#ifndef CLIENT_DLL
bp::object PyGetWorldEntity()
{
	return GetWorldEntity()->GetPyHandle();
}
#endif // CLIENT_DLL

// -------------------------------------------------------------------------------
#ifdef CLIENT_DLL
PyEntityFactory::PyEntityFactory( const char *pMapName, boost::python::object pyClass )
{
	if( !pMapName )
		return;

	char *pClassName = NULL;
	try {
		pClassName = boost::python::extract<char *>(pyClass.attr("__name__"));
	} catch(bp::error_already_set &) {
		PyErr_Print();
		return;
	}

	Q_strncpy( m_ClassName, pClassName, sizeof( m_ClassName ) );
	m_PyClass = pyClass;

	// Remove any linked factory to this classname
	if( GetClassMap().PyGetFactory( m_ClassName ) )
		GetClassMap().PyRemove( m_ClassName );

	GetClassMap().PyAdd(pMapName, m_ClassName, 0, this);
}

PyEntityFactory::~PyEntityFactory()
{
	// Only remove if the there is a factory for this classname attached
	// to this instance of the factory (might already be replaced in python).
	if( GetClassMap().PyGetFactory( m_ClassName ) == this )
		GetClassMap().PyRemove( m_ClassName );
}

C_BaseEntity *PyEntityFactory::Create()
{
	CBaseEntity *pEnt = NULL;
	try
	{
		boost::python::object inst = m_PyClass();
		pEnt = boost::python::extract<CBaseEntity *>(inst);
	}
	catch( bp::error_already_set & )
	{
		PyErr_Print();
		return NULL;
	}
	return pEnt;
}

boost::python::object PyGetClassByClassname( const char *class_name )
{
	if( !class_name )
		return bp::object();

	PyEntityFactory *pFactory = GetClassMap().PyGetFactoryByMapName( class_name );
	if( !pFactory )
		return bp::object();

	return pFactory->GetClass();
}

bp::list PyGetAllClassnames()
{
	bp::list l;
	PyErr_SetString(PyExc_ValueError, "Unsupported on client");
	throw boost::python::error_already_set(); 
	return l;
}

#else
PyEntityFactory *g_pPyEntityFactoryHead=0;
PyEntityFactory::PyEntityFactory( const char *pClassName, boost::python::object PyClass )
{
	if( !pClassName )
		return;

	Q_strncpy( m_ClassName, pClassName, sizeof( m_ClassName ) );
	m_PyClass = PyClass;

	// Remove old factory if any
	if( EntityFactoryDictionary()->FindFactory( m_ClassName ) )
		EntityFactoryDictionary()->RemoveFactory( m_ClassName );

	// Install new factory
	EntityFactoryDictionary()->InstallFactory( this, m_ClassName );

	// Link
	m_pPyNext				= g_pPyEntityFactoryHead;
	g_pPyEntityFactoryHead	= this;

	// Init 
	if( g_bDoNotInitPythonClasses == false)
		InitPyClass();

	CheckEntities();
}

// Because we might reload/destroy the factory in python, we need to deinstall the factory from the dict
PyEntityFactory::~PyEntityFactory()
{
	// Only remove the installed factory if we are the installed factory
	// Another new factory might already be installed. When a python module gets reloaded
	// the new factory might be created first. Afterwards the old one is dereferenced and 
	// destroyed.
	if( EntityFactoryDictionary()->FindFactory( m_ClassName ) == this )
		EntityFactoryDictionary()->RemoveFactory( m_ClassName );

	// Unlink
	if( this == g_pPyEntityFactoryHead )
	{
		g_pPyEntityFactoryHead = g_pPyEntityFactoryHead->m_pPyNext;
	}
	else
	{
		PyEntityFactory *p = g_pPyEntityFactoryHead->m_pPyNext;
		PyEntityFactory *prev = g_pPyEntityFactoryHead;
		while( p )
		{
			if( p == this )
			{
				prev->m_pPyNext = p->m_pPyNext;
				break;
			}
			prev = p;
			p = p->m_pPyNext;
		}
	}
}

extern bool g_SetupNetworkTablesOnHold;
IServerNetworkable *PyEntityFactory::Create( const char *pClassName )
{
	CBaseEntity *pEnt = NULL;
	try
	{
		boost::python::object inst = m_PyClass();

		pEnt = boost::python::extract<CBaseEntity *>(inst);
		bool bServerOnly = pEnt->IsEFlagSet(EFL_SERVER_ONLY);
		if( !bServerOnly && g_SetupNetworkTablesOnHold ) {
			pEnt->AddEFlags( EFL_NO_AUTO_EDICT_ATTACH );
		}

		pEnt->SetPyInstance( inst );
		pEnt->PostConstructor( pClassName );
		
		if( !bServerOnly && g_SetupNetworkTablesOnHold )
		{
			EntityInfoOnHold info;
			info.ent = pEnt;
			info.edict = engine->CreateEdict();
			AddSetupNetworkTablesOnHoldEnt(info);
			gEntList.AddNetworkableEntity( pEnt, ENTINDEX(info.edict) );
		}

		return pEnt->NetworkProp();
	}
	catch( bp::error_already_set & )
	{
		Warning("Failed to create entity %s:\n", pClassName );
		PyErr_Print();
		return NULL;
	}
	return NULL;
}

void PyEntityFactory::Destroy( IServerNetworkable *pNetworkable )
{
	if ( pNetworkable )
	{
		CBaseEntity *pEnt = pNetworkable->GetBaseEntity();
		if( pEnt )
		{
			// TODO:
			//pEnt->SetPyInstance( boost::python::object() );
		}
		pNetworkable->Release();
	}
}

size_t PyEntityFactory::GetEntitySize()
{
	return 0;
}

void PyEntityFactory::InitPyClass()
{
	bp::object meth = SrcPySystem()->Get("InitEntityClass", m_PyClass, false);
	if( meth.ptr() == Py_None )
		return;		// Not implemented, ignore
	SrcPySystem()->Run( meth );
}

// Check existing entities with this classname
// Change there class instance to the new one
void PyEntityFactory::CheckEntities()
{
	CBaseEntity *pEnt;

	pEnt = gEntList.FindEntityByClassname( NULL, m_ClassName );
	while( pEnt )
	{
		if( pEnt->GetPyInstance().ptr() != Py_None )
		{
			pEnt->GetPyInstance().attr("__setattr__")("__class__", m_PyClass);
		}
		pEnt = gEntList.FindEntityByClassname( pEnt, m_ClassName);
	}
}

boost::python::object PyGetClassByClassname( const char *class_name )
{
	if( !class_name )
		return bp::object();

	PyEntityFactory *p = g_pPyEntityFactoryHead;
	while( p )
	{
		if( !Q_strcmp( p->GetClassname(), class_name ) )
		{
			return p->GetClass();
		}
		p = p->m_pPyNext;
	}
	// Throw exception?
	return bp::object();
}

bp::list PyGetAllClassnames()
{
	bp::list l;

	PyEntityFactory *p = g_pPyEntityFactoryHead;
	while( p )
	{
		// Filter non linked factories (undestroyed python instances)
		if( EntityFactoryDictionary()->FindFactory( p->GetClassname() ) == p )
		{
			l.append( bp::object( p->GetClassname() ) );
		}
		p = p->m_pPyNext;
	}

	return l;
}

// Loop through all python entities and call InitPyClass()
// To be done on level init
void InitAllPythonEntities( void )
{
	PyEntityFactory *p = g_pPyEntityFactoryHead;
	while( p )
	{
		p->InitPyClass();
		p = p->m_pPyNext;
	}
}

#endif // CLIENT_DLL


// -------------------------------------------------------------------------------
#ifdef CLIENT_DLL
class C_TEPyEvent : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEPyEvent, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate( DataUpdateType_t updateType )
	{
		if( !m_hEnt.Get() )
			return;

		if( m_hEnt->GetPyInstance().ptr() == Py_None )
		{
			DevMsg("C_TEPyEvent can only be used with python entities!\n");
			return;
		}

		try 
		{
			SrcPySystem()->Run<int, int>( 
				SrcPySystem()->Get("ReceiveEvent", m_hEnt->GetPyInstance(), true), 
				m_iEvent, m_nData );
		} 
		catch( bp::error_already_set & ) 
		{
			PyErr_Print();
		}
	}

public:
	CNetworkHandle( CBaseEntity, m_hEnt );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};
IMPLEMENT_CLIENTCLASS_EVENT( C_TEPyEvent, DT_TEPyEvent, CTEPyEvent );

BEGIN_RECV_TABLE_NOBASE( C_TEPyEvent, DT_TEPyEvent )
RecvPropEHandle( RECVINFO( m_hEnt ) ),
RecvPropInt( RECVINFO( m_iEvent ) ),
RecvPropInt( RECVINFO( m_nData ) )
END_RECV_TABLE()

#else
class CTEPyEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPyEvent, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTEPyEvent( const char *name ) : CBaseTempEntity( name )
	{
	}

	CNetworkHandle( CBaseEntity, m_hEnt );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEPyEvent, DT_TEPyEvent )
SendPropEHandle( SENDINFO( m_hEnt ) ),
SendPropInt( SENDINFO( m_iEvent ), 8, SPROP_UNSIGNED ),
SendPropInt( SENDINFO( m_nData ), 16 )
END_SEND_TABLE()

static CTEPyEvent g_TEPyEvent( "PyEvent" );

ConVar g_debug_pyevent("g_debug_pyevent", "0", FCVAR_CHEAT|FCVAR_REPLICATED);

void PySendEvent( IRecipientFilter &filter, EHANDLE ent, int event, int data)
{
	if( g_debug_pyevent.GetBool() )
	{
		DevMsg("Sending Python event to entity #%d with event %d and data %d\n", ent ? ent->entindex() : -1, event, data);
	}

	g_TEPyEvent.m_hEnt = ent;
	g_TEPyEvent.m_iEvent = event;
	g_TEPyEvent.m_nData = data;
	g_TEPyEvent.Create( filter, 0 );
}


//-----------------------------------------------------------------------------
// Purpose: Spawn a player
//-----------------------------------------------------------------------------
bp::object PyRespawnPlayer( CBasePlayer *pPlayer, const char *classname )
{
	if( !pPlayer || !pPlayer->edict() )
	{
		PyErr_SetString(PyExc_ValueError, "Invalid player");
		throw boost::python::error_already_set(); 
		return bp::object();
	}

	if( !classname )
	{
		PyErr_SetString(PyExc_ValueError, "Invalid player class");
		throw boost::python::error_already_set(); 
		return bp::object();
	}

	// Save edict + player name
	edict_t *pEdict = pPlayer->edict();
	const char *playername = pPlayer->GetPlayerName();

	// Destroy the old player entity
	UTIL_RemoveImmediate( pPlayer );

	// Create a new player using the provided player class
	CBasePlayer *pPlayerNew = dynamic_cast<CBasePlayer *>(CBasePlayer::CreatePlayer( classname, pEdict ));
	if( !pPlayerNew )
	{
		PyErr_SetString(PyExc_ValueError, "Invalid player entity class");
		throw boost::python::error_already_set(); 
		return bp::object();
	}
	pPlayerNew->SetPlayerName( playername );

	pPlayerNew->InitialSpawn();
	pPlayerNew->Spawn();
	
	return pPlayerNew->GetPyHandle();
}

#endif // CLIENT_DLL

#ifndef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: PyOutputEvent
//-----------------------------------------------------------------------------
PyOutputEvent::PyOutputEvent()
{
	// Set to NULL! Normally it depends on the the memory allocation function of CBaseEntity
	m_ActionList = NULL; 

	// Default
	m_Value.Set( FIELD_VOID, NULL );
}

void PyOutputEvent::Set( variant_t value )
{
	m_Value = value;
}

// void Firing, no parameter
void PyOutputEvent::FireOutput( CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay )
{
	CBaseEntityOutput::FireOutput(m_Value, pActivator, pCaller, fDelay);
}

//-----------------------------------------------------------------------------
// Purpose: Bone followers
//-----------------------------------------------------------------------------
void PyBoneFollowerManager::InitBoneFollowers( CBaseAnimating *pParentEntity, boost::python::list followerbonenames )
{
	if( !pParentEntity )
		return;

	int i, iNumBones;

	iNumBones = bp::len(followerbonenames);

	char **pFollowerBoneNames = (char **)malloc(sizeof(char *) * iNumBones);
	
	for( i = 0; i < iNumBones; i++ )
	{
		pFollowerBoneNames[i] = (char *)malloc( sizeof(char) * _MAX_PATH );

		const char *pFollowerBoneName = bp::extract<const char *>(followerbonenames[i]);
		if( pFollowerBoneName )
			Q_strncpy(pFollowerBoneNames[i], pFollowerBoneName, _MAX_PATH);
		else
			pFollowerBoneNames[i][0] = '\0';
	}
	CBoneFollowerManager::InitBoneFollowers( pParentEntity, iNumBones, (const char **)pFollowerBoneNames );

	for( i = 0; i < iNumBones; i++ )
		free( pFollowerBoneNames[i] );
	free( pFollowerBoneNames );
}

void PyBoneFollowerManager::AddBoneFollower( CBaseAnimating *pParentEntity, const char *pFollowerBoneName, solid_t *pSolid )
{
	if( !pParentEntity || !pFollowerBoneName )
		return;

	CBoneFollowerManager::AddBoneFollower( pParentEntity, pFollowerBoneName, pSolid );
}

// Call this after you move your bones
void PyBoneFollowerManager::UpdateBoneFollowers( CBaseAnimating *pParentEntity )
{
	if( !pParentEntity )
		return;

	CBoneFollowerManager::UpdateBoneFollowers( pParentEntity );
}

// Call this when your entity's removed
void PyBoneFollowerManager::DestroyBoneFollowers( void )
{
	CBoneFollowerManager::DestroyBoneFollowers();
}

pyphysfollower_t PyBoneFollowerManager::GetBoneFollower( int iFollowerIndex )
{
	pyphysfollower_t physfollower;
	physfollower_t *pPhysFollower = CBoneFollowerManager::GetBoneFollower( iFollowerIndex );
	if( !pPhysFollower )
		return physfollower;

	physfollower.follower = pPhysFollower->hFollower ? pPhysFollower->hFollower->GetPyHandle() : bp::object();
	physfollower.boneindex = pPhysFollower->boneIndex;
	return physfollower;
}

int PyBoneFollowerManager::GetBoneFollowerIndex( CBoneFollower *pFollower )
{
	if( !pFollower )
		return -1;
	return CBoneFollowerManager::GetBoneFollowerIndex( pFollower );
}

Vector GetAttachmentPositionInSpaceOfBone( CStudioHdr *pStudioHdr, const char *pAttachmentName, int outputBoneIndex )
{
	if( !pStudioHdr || !pAttachmentName )
		return vec3_origin;

	int attachment = Studio_FindAttachment( pStudioHdr, pAttachmentName );

	Vector localAttach;
	const mstudioattachment_t &pAttachment = pStudioHdr->pAttachment(attachment);
	int iBone = pStudioHdr->GetAttachmentBone( attachment );
	MatrixGetColumn( pAttachment.local, 3, localAttach );

	matrix3x4_t inputToOutputBone;
	Studio_CalcBoneToBoneTransform( pStudioHdr, iBone, outputBoneIndex, inputToOutputBone );
	
	Vector out;
	VectorTransform( localAttach, inputToOutputBone, out );

	return out;
}

#if 0 // TODO
CRagdollProp *PyCreateServerRagdollAttached( CBaseAnimating *pAnimating, const Vector &vecForce, 
											int forceBone, int collisionGroup, PyPhysicsObject &pyAttached, 
											CBaseAnimating *pParentEntity, int boneAttach, const Vector &originAttached, 
											int parentBoneAttach, const Vector &boneOrigin )
{
	if( !pAnimating || !pAnimating->CanBecomeRagdoll() )
		return NULL;
	return CreateServerRagdollAttached( pAnimating, vecForce, forceBone, collisionGroup, 
		pyAttached.GetVPhysicsObject(), pParentEntity, 
		boneAttach, originAttached, parentBoneAttach, boneOrigin );
}
#endif // 0

#endif // CLIENT_DLL
