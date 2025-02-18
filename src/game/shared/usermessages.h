//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef USERMESSAGES_H
#define USERMESSAGES_H
#ifdef _WIN32
#pragma once
#endif

#include <utldict.h>
#include <utlvector.h>
#include <bitbuf.h>

void RegisterScriptMessages();

// Client dispatch function for usermessages
typedef void (*pfnUserMsgHook)(bf_read &msg);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CUserMessage
{
	public:
		// byte size of message, or -1 for variable sized
		int				size;	
		const char		*name;
		// Client only dispatch function for message
		CUtlVector<pfnUserMsgHook>	clienthooks;
};

//-----------------------------------------------------------------------------
// Purpose: Interface for registering and dispatching usermessages
// Shred code creates same ordered list on client/server
//-----------------------------------------------------------------------------
class CUserMessages
{
public:
	
	CUserMessages();
	~CUserMessages();
	
	// Returns -1 if not found, otherwise, returns appropriate index
	int		LookupUserMessage( const char *name );
	int		GetUserMessageSize( int index );
	const char *GetUserMessageName( int index );
	bool	IsValidIndex( int index );

	// Server only
	void	Register( const char *name, int size );

	// Client only
	void	HookMessage( const char *name, pfnUserMsgHook hook );
	bool	DispatchUserMessage( int msg_type, bf_read &msg_data );

private:

	CUtlDict< CUserMessage*, int >	m_UserMessages;
};

extern CUserMessages *usermessages;

// guaranteed usermessages' been initialized before use in static initializer
void CreateUserMessages();

#define USER_MESSAGE(x)\
	void __MsgFunc_##x( bf_read &msg );\
	static struct SingletonHookMsg_##x\
	{\
		SingletonHookMsg_##x() { CreateUserMessages(); usermessages->HookMessage(#x, __MsgFunc_##x ); }\
	} s_SingletonHookMsg_##x;\
	void __MsgFunc_##x( bf_read &msg )\

#endif // USERMESSAGES_H
