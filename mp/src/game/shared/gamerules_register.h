//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef GAMERULES_REGISTER_H
#define GAMERULES_REGISTER_H
#ifdef _WIN32
#pragma once
#endif


// Each game rules class must register using this in it's .cpp file.
#if !defined(_STATIC_LINKED)
#define REGISTER_GAMERULES_CLASS( className ) \
	void __CreateGameRules_##className() { new className; } \
	static CGameRulesRegister __g_GameRulesRegister_##className( #className, __CreateGameRules_##className );
#else
#define REGISTER_GAMERULES_CLASS( className ) \
	void MAKE_NAME_UNIQUE(__CreateGameRules_)##className() { new className; } \
	static CGameRulesRegister __g_GameRulesRegister_##className( #className, MAKE_NAME_UNIQUE(__CreateGameRules_)##className );
#endif

#ifdef _XBOX
// force symbol expansion
#define REGISTER_GAMERULES_CLASS2( className ) REGISTER_GAMERULES_CLASS( className )
#endif

class CGameRulesRegister
{
public:
	typedef void (*CreateGameRulesFn)();

	CGameRulesRegister( const char *pClassName, CreateGameRulesFn fn );

	// Allocates the gamerules object associated with this class.
	void CreateGameRules();

	static CGameRulesRegister* FindByName( const char *pName );

private:
	const char *m_pClassName;
	CreateGameRulesFn m_pFn;
	CGameRulesRegister *m_pNext;	// Links it into the global list.
	
	static CGameRulesRegister *s_pHead;

};



#ifdef CLIENT_DLL

	// The client forwards this call so the game rules manager can create the appropriate
	// game rules class.
	void InstallStringTableCallback_GameRules();

#else

	// Server calls this at startup.
	void CreateNetworkStringTables_GameRules();
	
	// Server calls this to install a specific game rules object. The class should have been registered
	// with REGISTER_GAMERULES_CLASS.
	void CreateGameRulesObject( const char *pClassName );

#endif


#endif // GAMERULES_REGISTER_H
