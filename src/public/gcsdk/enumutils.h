//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Makes enum-to-string and string-to-enum easier to declare
//
// $NoKeywords: $
//=============================================================================

#ifndef GCENUMUTILS_H
#define GCENUMUTILS_H
#ifdef _WIN32
#pragma once
#endif

namespace GCSDK
{
	struct EnumString_s
	{
		int nValue;
		const char *pszString;
	};
}

// starts defining a enum value string map
#define ENUMSTRINGS_START( etype ) static const GCSDK::EnumString_s s_##etype[] = {

// ends defining a enum value string map. generates PchNameFromEnumName()
#define ENUMSTRINGS_END( etype ) }; const char* PchNameFrom##etype( etype nValue ) \
{ for( uint i=0; i<Q_ARRAYSIZE(s_##etype); i++ ) { if ( s_##etype[i].nValue == nValue ) return s_##etype[i].pszString; } \
	AssertMsg2( false, "Missing String for %s (%d)", #etype, nValue ); return "Unknown"; } \
const char* PchNameFrom##etype##Unsafe( etype nValue ) \
{ for( uint i=0; i<Q_ARRAYSIZE(s_##etype); i++ ) { if ( s_##etype[i].nValue == nValue ) return s_##etype[i].pszString; } \
	return NULL; }

// ends defining a enum value string map. generates PchNameFromEnum() and EnumFromNam(). Invalid element must be first in array
#define ENUMSTRINGS_REVERSE( etype, default ) }; const char* PchNameFrom##etype(etype nValue ) \
{ for( uint i=0; i<Q_ARRAYSIZE(s_##etype); i++ ) { if ( s_##etype[i].nValue == nValue ) return s_##etype[i].pszString; } \
	AssertMsg2( false, "Missing String for %s (%d)", #etype, nValue ); return "Unknown"; }; \
	etype etype##FromName( const char *pchName ) \
{ for( uint i=0; i<Q_ARRAYSIZE(s_##etype); i++ ) { if ( !Q_stricmp( s_##etype[i].pszString, pchName ) ) return (etype)(s_##etype[i].nValue); } \
	return default; }


#endif // GCENUMUTILS_H
