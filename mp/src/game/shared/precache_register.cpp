//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "precache_register.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CPrecacheRegister	*g_pPrecacheRegisters = 0;

CPrecacheRegister::CPrecacheRegister(PrecacheFn fn, const void *pUser)
{
	m_Fn = fn;
	m_pUser = const_cast<void *>(pUser);

	m_pNext = g_pPrecacheRegisters;
	g_pPrecacheRegisters = this;
}


void CPrecacheRegister::Precache()
{
	for(CPrecacheRegister *pCur=g_pPrecacheRegisters; pCur; pCur=pCur->m_pNext)
	{
		pCur->m_Fn(pCur->m_pUser);
	}
}


void CPrecacheRegister::PrecacheFn_Other(void *pUser)
{
	UTIL_PrecacheOther((const char*)pUser);
}


