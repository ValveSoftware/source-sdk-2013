//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy.h"
#include "srcpy_server_class.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

PyServerClass *g_pPyServerClassHead = NULL;

bool g_SetupNetworkTablesOnHold = false;

namespace DT_BaseEntity
{
	extern SendTable g_SendTable;
}
namespace DT_BaseAnimating
{
	extern SendTable g_SendTable;
}
namespace DT_BaseAnimatingOverlay
{
	extern SendTable g_SendTable;
}
namespace DT_BaseFlex
{
	extern SendTable g_SendTable;
}
namespace DT_BaseCombatCharacter
{
	extern SendTable g_SendTable;
}
namespace DT_BasePlayer
{
	extern SendTable g_SendTable;
}

namespace DT_BaseGrenade
{
	extern SendTable g_SendTable;
}

namespace DT_Sprite
{
	extern SendTable g_SendTable;
}

namespace DT_SmokeTrail
{
	extern SendTable g_SendTable;
}

namespace DT_Beam
{
	extern SendTable g_SendTable;
}

namespace DT_BaseCombatWeapon
{
	extern SendTable g_SendTable;
}

#if 0 // TODO
namespace DT_UnitBase
{
	extern SendTable g_SendTable;
}

namespace DT_HL2WarsPlayer
{
	extern SendTable g_SendTable;
}

namespace DT_WarsWeapon
{
	extern SendTable g_SendTable;
}

namespace DT_FuncUnit
{
	extern SendTable g_SendTable;
}

namespace DT_BaseToggle
{
	extern SendTable g_SendTable;
}

namespace DT_BaseTrigger
{
	extern SendTable g_SendTable;
}
#endif // 0

namespace bp = boost::python;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
PyServerClass::PyServerClass(char *pNetworkName) : ServerClass(pNetworkName, NULL), m_pNetworkedClass(NULL)
{
	m_pTable = &(DT_BaseEntity::g_SendTable); // Default

	m_pPyNext				= g_pPyServerClassHead;
	g_pPyServerClassHead	= this;
	m_bFree = true;
}

void PyServerClass::SetupServerClass( int iType )
{
	m_iType = iType;

	switch( m_iType )
	{
	case PN_NONE:
	case PN_BASEENTITY:
		m_pTable = &(DT_BaseEntity::g_SendTable);
		break;
	case PN_BASEANIMATING:
		m_pTable = &(DT_BaseAnimating::g_SendTable);
		break;
	case PN_BASEANIMATINGOVERLAY:
		m_pTable = &(DT_BaseAnimatingOverlay::g_SendTable);
		break;
	case PN_BASEFLEX:
		m_pTable = &(DT_BaseFlex::g_SendTable);
		break;
	case PN_BASECOMBATCHARACTER:
		m_pTable = &(DT_BaseCombatCharacter::g_SendTable);
		break;
	case PN_BASEPLAYER:
		m_pTable = &(DT_BasePlayer::g_SendTable);
		break;
	case PN_BASEGRENADE:
		m_pTable = &(DT_BaseGrenade::g_SendTable);
		break;

	case PN_SPRITE:
		m_pTable = &(DT_Sprite::g_SendTable);
		break;
	case PN_SMOKETRAIL:
		m_pTable = &(DT_SmokeTrail::g_SendTable);
		break;
	case PN_BEAM:
		m_pTable = &(DT_Beam::g_SendTable);
		break;
	case PN_BASECOMBATWEAPON:
		m_pTable = &(DT_BaseCombatWeapon::g_SendTable);
		break;

#if 0 // TODO
	case PN_UNITBASE:
		m_pTable = &(DT_UnitBase::g_SendTable);
		break;
	case PN_HL2WARSPLAYER:
		m_pTable = &(DT_HL2WarsPlayer::g_SendTable);
		break;
	case PN_WARSWEAPON:
		m_pTable = &(DT_WarsWeapon::g_SendTable);
		break;
	case PN_FUNCUNIT:
		m_pTable = &(DT_FuncUnit::g_SendTable);
		break;
	case PN_BASETOGGLE:
		m_pTable = &(DT_BaseToggle::g_SendTable);
		break;
	case PN_BASETRIGGER:
		m_pTable = &(DT_BaseTrigger::g_SendTable);
		break;
#endif // 0
	default:
		m_pTable = &(DT_BaseEntity::g_SendTable);
		break;
	}
}

static CUtlDict< const char*, unsigned short > m_ServerClassInfoDatabase;


PyServerClass *FindFreePyServerClass()
{
	PyServerClass *p = g_pPyServerClassHead;
	while( p )
	{
		if( p->m_bFree )
		{
			return p;
		}
		p = p->m_pPyNext;
	}
	return NULL;
}

PyServerClass *FindPyServerClass( const char *pName )
{
	PyServerClass *p = g_pPyServerClassHead;
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

//-----------------------------------------------------------------------------
// Purpose: Find a free PyServerClass and claim it
//			Send a message to the clients to claim a client class and set it to
//			the right type.
//-----------------------------------------------------------------------------
NetworkedClass::NetworkedClass( const char *pNetworkName, boost::python::object cls_type )
{
	m_pNetworkName = strdup( pNetworkName );
	m_pServerClass = NULL;
	PyServerClass *p;

	// See if there is already an entity with this network name
	unsigned short lookup = m_ServerClassInfoDatabase.Find( pNetworkName );
	if ( lookup != m_ServerClassInfoDatabase.InvalidIndex() )
	{
		Warning("NetworkedClass: %s already added. Replacing with new data. Element name: %s\n", pNetworkName, m_ServerClassInfoDatabase.Element(lookup) );
		p = FindPyServerClass( m_ServerClassInfoDatabase.Element(lookup) );
		if( !p )
		{
			Warning("NetworkedClass: ServerClass %s not found\n", m_ServerClassInfoDatabase.Element(lookup) );
			return;
		}
		if( p->m_pNetworkedClass )
			p->m_pNetworkedClass->m_pServerClass = NULL;
	}
	else
	{
		// Find a free server class and add it to the database
		p = FindFreePyServerClass();
		if( !p ) {
			Warning("Couldn't create PyServerClass %s: Out of free PyServerClasses\n", pNetworkName);
			return;
		}

		lookup = m_ServerClassInfoDatabase.Insert(pNetworkName, p->GetName());
	}

	m_pServerClass = p;
	m_PyClass = cls_type;
	p->m_bFree = false;
	p->m_pNetworkedClass = this;

	SetupServerClass();
}

NetworkedClass::~NetworkedClass()
{
	if( m_pServerClass )
	{
		m_pServerClass->m_bFree = true;
		m_pServerClass->SetupServerClass(PN_NONE);

		unsigned short lookup = m_ServerClassInfoDatabase.Find( m_pNetworkName );
		if ( lookup != m_ServerClassInfoDatabase.InvalidIndex() )
		{
			m_ServerClassInfoDatabase.Remove( m_pNetworkName );
		}
	}

	free( (void *)m_pNetworkName );
	m_pNetworkName = NULL;
}

extern edict_t *g_pForceAttachEdict;

void NetworkedClass::SetupServerClass()
{
	int iType = PN_NONE;
	try 
	{
		iType = boost::python::call_method<int>(m_PyClass.ptr(), "GetPyNetworkType");
		m_pServerClass->SetupServerClass( iType );
		PyObject_SetAttrString(m_PyClass.ptr(), "pyServerClass", bp::object(bp::ptr((ServerClass *)m_pServerClass)).ptr());
	} catch(bp::error_already_set &) 
	{
		PyErr_Print();
		PyErr_Clear();
	}

	// Send message to all clients
	CReliableBroadcastRecipientFilter filter;
	UserMessageBegin(filter, "PyNetworkCls");
	WRITE_BYTE(iType);
	WRITE_STRING(m_pServerClass->GetName());
	WRITE_STRING(m_pServerClass->m_pNetworkedClass->m_pNetworkName);
	MessageEnd();
}

void FullClientUpdatePyNetworkCls( CBasePlayer *pPlayer )
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;
	CSingleUserRecipientFilter filter(pPlayer);
	filter.MakeReliable();
	FullClientUpdatePyNetworkClsByFilter(filter);
}

void FullClientUpdatePyNetworkClsByFilter( IRecipientFilter &filter )
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;

	Assert(g_SetupNetworkTablesOnHold == false);

	// Send messages about each server class
	PyServerClass *p = g_pPyServerClassHead;
	while( p )
	{
		if( p->m_bFree ) {
			p = p->m_pPyNext;
			continue;
		}

		// Send message
		UserMessageBegin(filter, "PyNetworkCls");
		WRITE_BYTE(p->m_iType);
		WRITE_STRING(p->m_pNetworkName);
		WRITE_STRING(p->m_pNetworkedClass->m_pNetworkName);
		MessageEnd();

		p = p->m_pPyNext;
	}
}

void FullClientUpdatePyNetworkClsByEdict( edict_t *pEdict )
{
	if( !SrcPySystem()->IsPythonRunning() )
	{
		DevMsg("FullClientUpdatePyNetworkClsByEdict: Python is not running\n");
		return;
	}

	Assert(g_SetupNetworkTablesOnHold == false);

	// Send messages about each server class
	PyServerClass *p = g_pPyServerClassHead;
	while( p )
	{
		if( p->m_bFree ) {
			p = p->m_pPyNext;
			continue;
		}

		// Send message
		engine->ClientCommand( pEdict, "rpc %d %s %s\n", p->m_iType,
			p->m_pNetworkName, p->m_pNetworkedClass->m_pNetworkName);
		engine->ServerExecute(); // Send immediately to avoid an overflow when having too many

		p = p->m_pPyNext;
	}
}

CUtlVector<EntityInfoOnHold> g_SetupNetworkTablesOnHoldList;

void SetupNetworkTablesOnHold()
{
	g_SetupNetworkTablesOnHold = true;
}

void AddSetupNetworkTablesOnHoldEnt( EntityInfoOnHold info )
{
	g_SetupNetworkTablesOnHoldList.AddToTail(info);
}

void SetupNetworkTables()
{
	// Setup all tables and update all clients
	PyServerClass *p = g_pPyServerClassHead;
	while( p )
	{
		if( p->m_pNetworkedClass )
			p->m_pNetworkedClass->SetupServerClass();
		p = p->m_pPyNext;
	}
}

bool SetupNetworkTablesRelease()
{
	if( g_SetupNetworkTablesOnHold == false )
		return false;
	g_SetupNetworkTablesOnHold = false;

	// Setup all tables and update all clients
	PyServerClass *p = g_pPyServerClassHead;
	while( p )
	{
		if( p->m_pNetworkedClass )
			p->m_pNetworkedClass->SetupServerClass();
		p = p->m_pPyNext;
	}

	// Release on hold
	for( int i=0; i<g_SetupNetworkTablesOnHoldList.Count(); i++ )
	{
		EntityInfoOnHold info = (g_SetupNetworkTablesOnHoldList.Element(i));
		info.ent->NetworkProp()->AttachEdict( info.edict );
		info.ent->edict()->m_pNetworkable = info.ent->NetworkProp();
		info.ent->SetTransmitState(FL_FULL_EDICT_CHANGED|FL_EDICT_DIRTY_PVS_INFORMATION);
		info.ent->DispatchUpdateTransmitState();
	}
	g_SetupNetworkTablesOnHoldList.RemoveAll();
	return true;
}

// Call on level shutdown
// Server will tell us the new recv tables later
// Level init requires us to be sure
void PyResetAllNetworkTables()
{
	PyServerClass *p = g_pPyServerClassHead;
	while( p )
	{
		p->SetupServerClass(PN_BASEENTITY);
		p = p->m_pPyNext;
	}
}

// Debugging
CON_COMMAND_F( print_py_serverclass_list, "Print server class list", 0)
{
	if ( !g_pPyServerClassHead )
		return;

	PyServerClass *p = g_pPyServerClassHead;
	while( p ) {
		if( p->m_pNetworkedClass )
			Msg("ServerClass: %s linked to %s\n", p->m_pNetworkName, p->m_pNetworkedClass->m_pNetworkName);
		else
			Msg("ServerClass: %s linked to nothing\n", p->m_pNetworkName);
		p = p->m_pPyNext;
	}
}
