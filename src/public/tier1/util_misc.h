//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Miscellaneous generic helper templates & macros
//=============================================================================//

#ifndef TIER1_UTIL_MISC_H
#define TIER1_UTIL_MISC_H

#ifdef _WIN32
#pragma once
#endif

#include <functional>

//-----------------------------------------------------------------------------
// ScopeExitRunner - Generic helper to run a function upon destruction
//-----------------------------------------------------------------------------
class ScopeExitRunner
{
public:
	ScopeExitRunner( std::function<void()> && func ) : m_func( std::move(func) ) {}
	~ScopeExitRunner() { if ( m_func ) { m_func(); } }

	void Inhibit() { m_func = nullptr; }
private:
	std::function<void()> m_func;
};

#endif // TIER1_UTIL_MISC_H
