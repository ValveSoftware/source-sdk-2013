//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"

PrototypeEffectLink *g_pPrototypeEffects = 0;

PrototypeEffectLink::PrototypeEffectLink(PrototypeEffectCreateFn fn, const char *pName)
{
	m_CreateFn = fn;
	m_pEffectName = pName;
	m_pNext = g_pPrototypeEffects;
	g_pPrototypeEffects = this;
}


