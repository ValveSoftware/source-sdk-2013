//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Helper class for cvars that have restrictions on their value.
//
//=============================================================================//

#ifndef CONVAR_SERVERBOUNDED_H
#define CONVAR_SERVERBOUNDED_H
#ifdef _WIN32
#pragma once
#endif


// This class is used to virtualize a ConVar's value, so the client can restrict its 
// value while connected to a server. When using this across modules, it's important
// to dynamic_cast it to a ConVar_ServerBounded or you won't get the restricted value.
//
// NOTE: FCVAR_USERINFO vars are not virtualized before they are sent to the server
//       (we have no way to detect if the virtualized value would change), so
//       if you want to use a bounded cvar's value on the server, you must rebound it
//       the same way the client does.
class ConVar_ServerBounded : public ConVar
{
public:
	ConVar_ServerBounded( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString )
		: ConVar( pName, pDefaultValue, flags, pHelpString ) 
	{
	}

	ConVar_ServerBounded( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString, FnChangeCallback_t callback )
		: ConVar( pName, pDefaultValue, flags, pHelpString, callback ) 
	{
	}
	
	ConVar_ServerBounded( char const *pName, char const *pDefaultValue, int flags, char const *pHelpString, bool bMin, float fMin, bool bMax, float fMax )
		: ConVar( pName, pDefaultValue, flags, pHelpString, bMin, fMin, bMax, fMax ) {}

	// You must implement GetFloat.
	virtual float GetFloat() const = 0;
	
	// You can optionally implement these.
	virtual int  GetInt() const		{ return (int)GetFloat(); }
	virtual bool GetBool() const	{  return ( GetInt() != 0 ); }

	// Use this to get the underlying cvar's value.
	float GetBaseFloatValue() const
	{
		return ConVar::GetFloat();
	}
};


#endif // CONVAR_SERVERBOUNDED_H
