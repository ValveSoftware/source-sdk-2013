//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DMELEMENTHANDLE_H
#define DMELEMENTHANDLE_H

#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// handle to an CDmElement
//-----------------------------------------------------------------------------
#define PERFORM_HANDLE_TYPECHECKING 0
#if PERFORM_HANDLE_TYPECHECKING

// this is here to make sure we're being type-safe about element handles
// otherwise, the compiler lets us cast to bool incorrectly
// the other solution would be to redefine DmElementHandle_t s.t. DMELEMENT_HANDLE_INVALID==0
struct DmElementHandle_t
{
	DmElementHandle_t() : handle( 0xffffffff ) {}
	explicit DmElementHandle_t( int h ) : handle( h ) {}
	inline bool operator==( const DmElementHandle_t &h ) const { return handle == h.handle; }
	inline bool operator!=( const DmElementHandle_t &h ) const { return handle != h.handle; }
	inline bool operator<( const DmElementHandle_t &h ) const { return handle < h.handle; }
//	inline operator int() const { return handle; } // if we're okay with implicit int casts, uncomment this method
	int handle;
};
const DmElementHandle_t DMELEMENT_HANDLE_INVALID;

#else // PERFORM_HANDLE_TYPECHECKING

enum DmElementHandle_t : intp
{
	DMELEMENT_HANDLE_INVALID = (DmElementHandle_t)(intp)(~((uintp)(0)))
};

#endif // PERFORM_HANDLE_TYPECHECKING



#endif // DMELEMENTHANDLE_H
