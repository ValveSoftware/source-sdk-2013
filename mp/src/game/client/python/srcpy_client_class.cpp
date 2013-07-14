//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Represents a Python Network class.
//			A large number of static empty classes are initialized to get around
//			the limitation that you can't dynamically create them.
//			The NetworkedClass can then be created in Python, which will automatically
//			find a matching ClientClass. The server controls which ClientClass the NetworkedClass
//			should pick.
// TODO: Cleanup this file. The pNetworkClassName variables are confusing.
//		 Should be made clear to which one it belongs (the Python or ClientClass one).
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy_client_class.h"
#include "srcpy.h"
#include "usermessages.h"

//#include "c_hl2wars_player.h"
#include "basegrenade_shared.h"
//#include "unit_base_shared.h"
#include "sprite.h"
#include "c_smoke_trail.h"
#include "beam_shared.h"
#include "basecombatweapon_shared.h"
//#include "c_wars_weapon.h"
//#include "wars_func_unit.h"
//#include "c_basetoggle.h"
//#include "c_triggers.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

PyClientClassBase *g_pPyClientClassHead = NULL;

namespace bp = boost::python;

// Recv tables
namespace DT_BaseAnimating
{
	extern RecvTable g_RecvTable;
}
namespace DT_BaseAnimatingOverlay
{
	extern RecvTable g_RecvTable;
}
namespace DT_BaseFlex
{
	extern RecvTable g_RecvTable;
}
namespace DT_BaseCombatCharacter
{
	extern RecvTable g_RecvTable;
}
namespace DT_BasePlayer
{
	extern RecvTable g_RecvTable;
}
namespace DT_HL2WarsPlayer
{
	extern RecvTable g_RecvTable;
}
namespace DT_BaseGrenade
{
	extern RecvTable g_RecvTable;
}
namespace DT_UnitBase
{
	extern RecvTable g_RecvTable;
}

namespace DT_Sprite
{
	extern RecvTable g_RecvTable;
}

namespace DT_SmokeTrail
{
	extern RecvTable g_RecvTable;
}

namespace DT_Beam
{
	extern RecvTable g_RecvTable;
}

namespace DT_BaseCombatWeapon
{
	extern RecvTable g_RecvTable;
}

namespace DT_WarsWeapon
{
	extern RecvTable g_RecvTable;
}

namespace DT_FuncUnit 
{
	extern RecvTable g_RecvTable;
}

namespace DT_BaseToggle 
{
	extern RecvTable g_RecvTable;
}

namespace DT_BaseTrigger 
{
	extern RecvTable g_RecvTable;
}

// A lot of factories
#define IMPLEMENT_FALLBACK_FACTORY( clientClassName ) \
	static IClientNetworkable* _PN##clientClassName##_CreateObject( int entnum, int serialNum ) \
	{ \
		clientClassName *pRet = new clientClassName; \
		if ( !pRet ) \
			return 0; \
		pRet->Init( entnum, serialNum ); \
		return pRet; \
	}

#define CALL_FALLBACK_FACTORY( clientClassName, entnum, serialNum ) \
	_PN##clientClassName##_CreateObject(entnum, serialNum);

IMPLEMENT_FALLBACK_FACTORY(C_BaseEntity)
IMPLEMENT_FALLBACK_FACTORY(C_BaseAnimating)
IMPLEMENT_FALLBACK_FACTORY(C_BaseAnimatingOverlay)
IMPLEMENT_FALLBACK_FACTORY(C_BaseFlex)
IMPLEMENT_FALLBACK_FACTORY(C_BaseCombatCharacter)
IMPLEMENT_FALLBACK_FACTORY(C_BasePlayer)
IMPLEMENT_FALLBACK_FACTORY(C_BaseGrenade)
IMPLEMENT_FALLBACK_FACTORY(C_Sprite)
IMPLEMENT_FALLBACK_FACTORY(C_SmokeTrail)
IMPLEMENT_FALLBACK_FACTORY(C_Beam)
IMPLEMENT_FALLBACK_FACTORY(C_BaseCombatWeapon)

#if 0 // TODO
IMPLEMENT_FALLBACK_FACTORY(C_HL2WarsPlayer)
IMPLEMENT_FALLBACK_FACTORY(C_UnitBase)
IMPLEMENT_FALLBACK_FACTORY(C_WarsWeapon)
IMPLEMENT_FALLBACK_FACTORY(C_FuncUnit)
IMPLEMENT_FALLBACK_FACTORY(C_BaseToggle)
IMPLEMENT_FALLBACK_FACTORY(C_BaseTrigger)
#endif // 0

// Set the right recv table
void SetupClientClassRecv( PyClientClassBase *p, int iType  )
{
	switch( iType )
	{
	case PN_BASEENTITY:
		p->m_pRecvTable = &(DT_BaseEntity::g_RecvTable);
		break;
	case PN_BASEANIMATING:
		p->m_pRecvTable = &(DT_BaseAnimating::g_RecvTable);
		break;
	case PN_BASEANIMATINGOVERLAY:
		p->m_pRecvTable = &(DT_BaseAnimatingOverlay::g_RecvTable);
		break;
	case PN_BASEFLEX:
		p->m_pRecvTable = &(DT_BaseFlex::g_RecvTable);
		break;
	case PN_BASECOMBATCHARACTER:
		p->m_pRecvTable = &(DT_BaseCombatCharacter::g_RecvTable);
		break;
	case PN_BASEPLAYER:
		p->m_pRecvTable = &(DT_BasePlayer::g_RecvTable);
		break;
	case PN_BASEGRENADE:
		p->m_pRecvTable = &(DT_BaseGrenade::g_RecvTable);
		break;
	case PN_SPRITE:
		p->m_pRecvTable = &(DT_Sprite::g_RecvTable);
		break;	
	case PN_SMOKETRAIL:
		p->m_pRecvTable = &(DT_SmokeTrail::g_RecvTable);
		break;	
	case PN_BEAM:
		p->m_pRecvTable = &(DT_Beam::g_RecvTable);
		break;
	case PN_BASECOMBATWEAPON:
		p->m_pRecvTable = &(DT_BaseCombatWeapon::g_RecvTable);
		break;
#if 0 // TODO
	case PN_UNITBASE:
		p->m_pRecvTable = &(DT_UnitBase::g_RecvTable);
		break;
	case PN_HL2WARSPLAYER:
		p->m_pRecvTable = &(DT_HL2WarsPlayer::g_RecvTable);
		break;
	case PN_WARSWEAPON:
		p->m_pRecvTable = &(DT_WarsWeapon::g_RecvTable);
		break;
	case PN_FUNCUNIT:
		p->m_pRecvTable = &(DT_FuncUnit::g_RecvTable);
		break;
	case PN_BASETOGGLE:
		p->m_pRecvTable = &(DT_BaseToggle::g_RecvTable);
		break;
	case PN_BASETRIGGER:
		p->m_pRecvTable = &(DT_BaseTrigger::g_RecvTable);
		break;
#endif // 0
	default:
		p->m_pRecvTable = &(DT_BaseEntity::g_RecvTable);
		break;
	}
}

// Call on level shutdown
// Server will tell us the new recv tables later
// Level init requires us to be sure
void PyResetAllNetworkTables()
{
	PyClientClassBase *p = g_pPyClientClassHead;
	while( p )
	{
		SetupClientClassRecv(p, PN_BASEENTITY);
		p = p->m_pPyNext;
	}
}

IClientNetworkable *ClientClassFactory( int iType, boost::python::object cls_type, int entnum, int serialNum )
{
	try	
	{
		// Safety check. The base implementations must be match, otherwise it can result in incorrect behavior (crashes)
		int iNetworkType = boost::python::extract<int>(cls_type.attr("GetPyNetworkType")());
		if( iNetworkType != iType )
		{
			char buf[512];
			Q_snprintf( buf, sizeof(buf), "Network type does not match client %d != server %d", iNetworkType, iType );
			PyErr_SetString(PyExc_Exception, buf );
			throw boost::python::error_already_set(); 
		}

		// Spawn and initialize the entity
		boost::python::object inst = cls_type();
		C_BaseEntity *pRet = boost::python::extract<C_BaseEntity *>(inst);
		if( !pRet ) {
			Warning("Invalid client entity\n" );
			return NULL;
		}

		pRet->SetPyInstance( inst );
		pRet->Init( entnum, serialNum );
		return pRet;
	}
	catch(boost::python::error_already_set &)
	{
		Warning("Failed to create python client side entity, falling back to base c++ class\n");
		PyErr_Print();
		PyErr_Clear();
		
		// Call the correct fallback factory
		IClientNetworkable *pResult = NULL;
		switch( iType )
		{
		case PN_BASEENTITY:
			pResult = CALL_FALLBACK_FACTORY( C_BaseEntity, entnum, serialNum );
			break;
		case PN_BASEANIMATING:
			pResult = CALL_FALLBACK_FACTORY( C_BaseAnimating, entnum, serialNum );
			break;
		case PN_BASEANIMATINGOVERLAY:
			pResult = CALL_FALLBACK_FACTORY( C_BaseAnimatingOverlay, entnum, serialNum );
			break;
		case PN_BASEFLEX:
			pResult = CALL_FALLBACK_FACTORY( C_BaseFlex, entnum, serialNum );
			break;
		case PN_BASECOMBATCHARACTER:
			pResult = CALL_FALLBACK_FACTORY( C_BaseCombatCharacter, entnum, serialNum );
			break;
		case PN_BASEPLAYER:
			pResult = CALL_FALLBACK_FACTORY( C_BasePlayer, entnum, serialNum );
			break;
		case PN_BASEGRENADE:
			pResult = CALL_FALLBACK_FACTORY( C_BaseGrenade, entnum, serialNum );
			break;
		case PN_SPRITE:
			pResult = CALL_FALLBACK_FACTORY( C_Sprite, entnum, serialNum );
			break;
		case PN_BEAM:
			pResult = CALL_FALLBACK_FACTORY( C_Beam, entnum, serialNum );
			break;
		case PN_BASECOMBATWEAPON:
			pResult = CALL_FALLBACK_FACTORY( C_BaseCombatWeapon, entnum, serialNum );
			break;

#if 0 // TODO
		case PN_UNITBASE:
			pResult = CALL_FALLBACK_FACTORY( C_UnitBase, entnum, serialNum );
			break;
		case PN_HL2WARSPLAYER:
			pResult = CALL_FALLBACK_FACTORY( C_HL2WarsPlayer, entnum, serialNum );
			break;
		case PN_WARSWEAPON:
			pResult = CALL_FALLBACK_FACTORY( C_WarsWeapon, entnum, serialNum );
			break;
		case PN_FUNCUNIT:
			pResult = CALL_FALLBACK_FACTORY( C_FuncUnit, entnum, serialNum );
			break;
		case PN_BASETOGGLE:
			pResult = CALL_FALLBACK_FACTORY( C_BaseToggle, entnum, serialNum );
			break;
		case PN_BASETRIGGER:
			pResult = CALL_FALLBACK_FACTORY( C_BaseTrigger, entnum, serialNum );
			break;
#endif // 0
		default:
			Warning("No default fallback for networktype %d. Warn a dev.\n");
			pResult = CALL_FALLBACK_FACTORY( C_BaseEntity, entnum, serialNum );
			break;
		}
		return pResult;
	}
}

void InitAllPythonEntities()
{	
	PyClientClassBase *p = g_pPyClientClassHead;
	while( p )
	{
		if( p->m_pNetworkedClass ) {
			p->InitPyClass();
		}
		p = p->m_pPyNext;
	}
}

static CUtlDict< NetworkedClass*, unsigned short > m_NetworkClassDatabase;

PyClientClassBase *FindPyClientClass( const char *pName )
{
	PyClientClassBase *p = g_pPyClientClassHead;
	while( p )
	{
		if ( _stricmp( p->GetName(), pName ) == 0)
		{
			return p;
		}
		p = p->m_pPyNext;
	}
	return NULL;
}

PyClientClassBase *FindPyClientClassToNetworkClass( const char *pNetworkName )
{
	PyClientClassBase *p = g_pPyClientClassHead;
	while( p )
	{
		if ( _stricmp( p->m_strPyNetworkedClassName, pNetworkName ) == 0)
		{
			return p;
		}
		p = p->m_pPyNext;
	}
	return NULL;
}

void CheckEntities( PyClientClassBase *pCC, boost::python::object pyClass )
{
	int iHighest = ClientEntityList().GetHighestEntityIndex();
	for ( int i=0; i <= iHighest; i++ )
	{
		C_BaseEntity *pEnt = ClientEntityList().GetBaseEntity( i );
		if ( !pEnt || pEnt->GetClientClass() != pCC || pEnt->GetPyInstance().ptr() == Py_None )
			continue;

		pEnt->GetPyInstance().attr("__setattr__")("__class__", pyClass);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Find a free PyServerClass and claim it
//			Send a message to the clients to claim a client class and set it to
//			the right type.
//-----------------------------------------------------------------------------
NetworkedClass::NetworkedClass( 
							   const char *pNetworkName, boost::python::object cls_type, 
							   const char *pClientModuleName )
{
	m_pClientClass = NULL;
	m_pNetworkName = strdup( pNetworkName );
	m_pyClass = cls_type;
	
	unsigned short lookup = m_NetworkClassDatabase.Find( m_pNetworkName );
	if ( lookup != m_NetworkClassDatabase.InvalidIndex() )
	{
		Warning("NetworkedClass: %s already added, replacing contents...\n", pNetworkName);
		m_pClientClass = m_NetworkClassDatabase[lookup]->m_pClientClass;
		m_NetworkClassDatabase[lookup] = this;
		if( m_pClientClass )
		{
			AttachClientClass( m_pClientClass );
		}
	}
	else
	{
		// New entry, add to database and find if there is an existing ClientClass
		m_NetworkClassDatabase.Insert( m_pNetworkName, this );

		m_pClientClass = FindPyClientClassToNetworkClass( m_pNetworkName );
	}
}

NetworkedClass::~NetworkedClass()
{
	unsigned short lookup;

	// Remove pointer
	lookup = m_NetworkClassDatabase.Find( m_pNetworkName );
	if ( lookup != m_NetworkClassDatabase.InvalidIndex() )
	{
		// Only remove if it's our pointer. Otherwise we are already replaced.
		if( m_NetworkClassDatabase.Element( lookup ) == this )
			m_NetworkClassDatabase.RemoveAt( lookup );
	}
	else
	{
		Warning("NetworkedClass destruction: invalid networkclass %s\n", m_pNetworkName);
	}

	if( m_pClientClass )
	{
		m_pClientClass->m_bFree = true;
	}
	free( (void *)m_pNetworkName );
	m_pNetworkName = NULL;
}

void NetworkedClass::AttachClientClass( PyClientClassBase *pClientClass )
{
	// Free old
	if( m_pClientClass && m_pClientClass->m_pNetworkedClass == this )
	{
		m_pClientClass->m_bFree = true;
		m_pClientClass->m_pNetworkedClass = NULL;
	}

	if( !pClientClass )
	{
		m_pClientClass = NULL;
		return;
	}

	// Attach new 
	m_pClientClass = pClientClass;
	m_pClientClass->m_bFree = false;
	m_pClientClass->m_pNetworkedClass = this;
	try {
		m_pClientClass->SetPyClass(m_pyClass);
		PyObject_SetAttrString(m_pyClass.ptr(), "pyClientClass", bp::object(bp::ptr((ClientClass *)m_pClientClass)).ptr());
	} catch(boost::python::error_already_set &) {
		PyErr_Print();
		PyErr_Clear();
	}
}

#define PYNETCLS_BUFSIZE 512

// Message handler for PyNetworkCls
void __MsgFunc_PyNetworkCls( bf_read &msg )
{
	char clientClass[PYNETCLS_BUFSIZE];
	char networkName[PYNETCLS_BUFSIZE];

	int iType = msg.ReadByte();
	msg.ReadString(clientClass, PYNETCLS_BUFSIZE);
	msg.ReadString(networkName, PYNETCLS_BUFSIZE);

	DbgStrPyMsg( "__MsgFunc_PyNetworkCls: Registering Python network class message %d %s %s\n", iType, clientClass, networkName );

	// Get module path
	const char *pch = Q_strrchr( networkName, '.' );
	if( !pch )
	{
		Warning( "Invalid python class name %s\n", networkName );
		return;
	}
	int n = pch - networkName + 1;

	char modulePath[PYNETCLS_BUFSIZE];
	Q_strncpy( modulePath, networkName, n );

	// Make sure the client class is imported
	SrcPySystem()->Import( modulePath );

	// Read which client class we are modifying
	PyClientClassBase *p = FindPyClientClass( clientClass );
	if( !p )
	{
		Warning( "__MsgFunc_PyNetworkCls: Invalid networked class %s\n", clientClass );
		return;
	}
	
	// Set type
	p->SetType( iType );
	SetupClientClassRecv( p, iType );

	// Read network class name
	Q_strncpy( p->m_strPyNetworkedClassName, networkName, PYNETCLS_BUFSIZE );

	// Attach if a network class exists
	unsigned short lookup = m_NetworkClassDatabase.Find( networkName );
	if ( lookup != m_NetworkClassDatabase.InvalidIndex() )
	{
		m_NetworkClassDatabase.Element(lookup)->AttachClientClass( p );
	}
	else
	{
		Warning( "__MsgFunc_PyNetworkCls: Invalid networked class %s\n", networkName );
	}
}

// register message handler once
void HookPyNetworkCls() 
{
	usermessages->HookMessage( "PyNetworkCls", __MsgFunc_PyNetworkCls );
}

CON_COMMAND_F( rpc, "", FCVAR_HIDDEN )
{
	int iType = atoi(args[1]);

	DbgStrPyMsg( "register_py_class: Registering Python network class message %d %s %s\n", iType, args[2], args[3] );

	// Get module path
	const char *pch = Q_strrchr( args[3], '.' );
	if( !pch )
	{
		Warning("Invalid python class name %s\n", args[3] );
		return;
	}
	int n = pch - args[3] + 1;

	char modulePath[PYNETCLS_BUFSIZE];
	Q_strncpy( modulePath, args[3], n );

	SrcPySystem()->Import( modulePath );
	PyClientClassBase *p = FindPyClientClass(args[2]);
	if( !p )
	{
		Warning("register_py_class: Invalid networked class %s\n", args[2]);
		return;
	}

	// Set type
	p->SetType(iType );
	SetupClientClassRecv(p, iType);

	Q_strncpy(p->m_strPyNetworkedClassName, args[3], 512);

	// Attach if a network class exists
	unsigned short lookup = m_NetworkClassDatabase.Find( args[3] );
	if ( lookup != m_NetworkClassDatabase.InvalidIndex() )
	{
		m_NetworkClassDatabase.Element(lookup)->AttachClientClass( p );
	}
	else
	{
		Warning( "register_py_class: Invalid networked class %s\n", args[3] );
	}
}

// Debugging
CON_COMMAND_F( print_py_clientclass_list, "Print client class list", 0 )
{
	if ( !g_pPyClientClassHead )
		return;

	PyClientClassBase *p = g_pPyClientClassHead;
	while( p ) {
		if( p->m_pNetworkedClass )
			Msg("ClientClass: %s linked to %s\n", p->m_pNetworkName, p->m_pNetworkedClass->m_pNetworkName);
		else
			Msg("ClientClass: %s linked to nothing\n", p->m_pNetworkName);
		p = p->m_pPyNext;
	}
}