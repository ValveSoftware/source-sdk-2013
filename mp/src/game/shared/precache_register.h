//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PRECACHE_REGISTER_H
#define PRECACHE_REGISTER_H
#pragma once


// Use these macros to register something to be precached.
#define PRECACHE_REGISTER_FN(functionName)	static CPrecacheRegister precache_function_##functionName(functionName, 0);
#define PRECACHE_WEAPON_REGISTER(className)	static CPrecacheRegister precache_weapon_##className(&CPrecacheRegister::PrecacheFn_Other, #className)
#define PRECACHE_REGISTER(className)		static CPrecacheRegister precache_other_##className( &CPrecacheRegister::PrecacheFn_Other, #className)

class CPrecacheRegister
{
public:
	
	typedef void (*PrecacheFn)(void *pUser);	// Prototype for a custom precache function.

	CPrecacheRegister(PrecacheFn fn, const void *pUser);

	PrecacheFn			m_Fn;	
	void				*m_pUser;
	CPrecacheRegister	*m_pNext;

	static void			Precache();						// Calls everything that has registered to precache.

// Don't call these.
public:
	static void			PrecacheFn_Other(void *pUser);
};


#endif // PRECACHE_REGISTER_H
