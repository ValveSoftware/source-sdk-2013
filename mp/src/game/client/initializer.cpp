//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "initializer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

Initializer	*Initializer::s_pInitializers = 0;


Initializer::Initializer(void **pVar, CreateInitializerObjectFn createFn, DeleteInitializerObjectFn deleteFn)
{
	m_pVar = pVar;
	m_CreateFn = createFn;
	m_DeleteFn = deleteFn;
	m_pNext = s_pInitializers;
	s_pInitializers = this;
}


bool Initializer::InitializeAllObjects()
{
	for(Initializer *pCur=s_pInitializers; pCur; pCur=pCur->m_pNext)
	{
		if(void *ptr = pCur->m_CreateFn())
		{
			*pCur->m_pVar = ptr;
		}
		else
		{
			// Don't worry if we're not actually trying to initialize a global
			if (pCur->m_pVar)
			{
				FreeAllObjects();
				return false;
			}
		}
	}

	return true;
}


void Initializer::FreeAllObjects()
{
	for(Initializer *pCur=s_pInitializers; pCur; pCur=pCur->m_pNext)
	{
		if (pCur->m_pVar)
		{
			pCur->m_DeleteFn(*pCur->m_pVar);
			*pCur->m_pVar = 0;
		}
	}
}





