//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "usermessages.h"
#include <bitbuf.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void RegisterScriptMessages( void )
{
	usermessages->Register( "SavedConvar", -1 );
}

void RegisterUserMessages( void );

//-----------------------------------------------------------------------------
// Purpose: Force registration on .dll load
// FIXME:  Should this be a client/server system?
//-----------------------------------------------------------------------------
CUserMessages::CUserMessages()
{
}

CUserMessages::~CUserMessages()
{
	int c = m_UserMessages.Count();
	for ( int i = 0; i < c; ++i )
	{
		delete m_UserMessages[ i ];
	}
	m_UserMessages.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int CUserMessages::LookupUserMessage( const char *name )
{
	int idx = m_UserMessages.Find( name );
	if ( idx == m_UserMessages.InvalidIndex() )
	{
		return -1;
	}

	return idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : int
//-----------------------------------------------------------------------------
int CUserMessages::GetUserMessageSize( int index )
{
	if ( index < 0 || index >= (int)m_UserMessages.Count() )
	{
		Error( "CUserMessages::GetUserMessageSize( %i ) out of range!!!\n", index );
	}

	CUserMessage *e = m_UserMessages[ index ];
	return e->size;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
const char *CUserMessages::GetUserMessageName( int index )
{
	if ( index < 0 || index >= (int)m_UserMessages.Count() )
	{
		Error( "CUserMessages::GetUserMessageSize( %i ) out of range!!!\n", index );
	}

	return m_UserMessages.GetElementName( index );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CUserMessages::IsValidIndex( int index )
{
	return m_UserMessages.IsValidIndex( index );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			size - -1 for variable size
//-----------------------------------------------------------------------------
void CUserMessages::Register( const char *name, int size )
{
	Assert( name );
	int idx = m_UserMessages.Find( name );
	if ( idx != m_UserMessages.InvalidIndex() )
	{
		Error( "CUserMessages::Register '%s' already registered\n", name );
	}

	CUserMessage * entry = new CUserMessage;
	entry->size = size;
	entry->name = name;

	m_UserMessages.Insert( name, entry );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			hook - 
//-----------------------------------------------------------------------------
void CUserMessages::HookMessage( const char *name, pfnUserMsgHook hook )
{
#if defined( CLIENT_DLL )
	Assert( name );
	Assert( hook );

	int idx = m_UserMessages.Find( name );
	if ( idx == m_UserMessages.InvalidIndex() )
	{
		DevMsg( "CUserMessages::HookMessage:  no such message %s\n", name );
		Assert( 0 );
		return;
	}

	int i = m_UserMessages[ idx ]->clienthooks.AddToTail();
	m_UserMessages[ idx ]->clienthooks[i] = hook;

#else
	Error( "CUserMessages::HookMessage called from server code!!!\n" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszName - 
//			iSize - 
//			*pbuf - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CUserMessages::DispatchUserMessage( int msg_type, bf_read &msg_data )
{
#if defined( CLIENT_DLL )
	if ( msg_type < 0 || msg_type >= (int)m_UserMessages.Count() )
	{
		DevMsg( "CUserMessages::DispatchUserMessage:  Bogus msg type %i (max == %i)\n", msg_type, m_UserMessages.Count() );
		Assert( 0 );
		return false;
	}

	CUserMessage *entry = m_UserMessages[ msg_type ];

	if ( !entry )
	{
		DevMsg( "CUserMessages::DispatchUserMessage:  Missing client entry for msg type %i\n", msg_type );
		Assert( 0 );
		return false;
	}

	if ( entry->clienthooks.Count() == 0 )
	{
		DevMsg( "CUserMessages::DispatchUserMessage:  missing client hook for %s\n", GetUserMessageName(msg_type) );
		Assert( 0 );
		return false;
	}

	for (int i = 0; i < entry->clienthooks.Count(); i++  )
	{
		bf_read msg_copy = msg_data;

		pfnUserMsgHook hook = entry->clienthooks[i];
		(*hook)( msg_copy );
	}
	return true;
#else
	Error( "CUserMessages::DispatchUserMessage called from server code!!!\n" );
	return false;
#endif
}

// Singleton

// Expose to rest of .dll
CUserMessages *usermessages = NULL;
void CreateUserMessages()
{
	if ( !usermessages )
	{
		usermessages = new CUserMessages();
		// Game specific registration function;
		RegisterUserMessages();
	}
}

// A helper to create and cleanup the usermessages singleton
static struct UserMessageHelper
{
	UserMessageHelper()
	{
		CreateUserMessages();
	}

	~UserMessageHelper()
	{
		delete usermessages;
		usermessages = NULL;
	}
} s_usermessage_helper;
