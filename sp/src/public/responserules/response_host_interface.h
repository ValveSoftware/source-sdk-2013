//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Core types for the response rules -- criteria, responses, rules, and matchers.
//
// $NoKeywords: $
//=============================================================================//

#ifndef RESPONSE_HOST_INTERFACE_H
#define RESPONSE_HOST_INTERFACE_H
#ifdef _WIN32
#pragma once
#endif

#include "filesystem.h"
class IUniformRandomStream;
class ICommandLine;

namespace ResponseRules
{
	// FUNCTIONS YOU MUST IMPLEMENT IN THE HOST EXECUTABLE:
	// These are functions that are mentioned in the header, but need their bodies implemented
	// in the .dll that links against this lib.
	// This is to wrap functions that previously came from the engine interface
	// back when the response rules were inside the server.dll . Now that the rules
	// are included into a standalone editor, we don't necessarily have an engine around,
	// so there needs to be some other implementation. 
	abstract_class IEngineEmulator
	{
	public:
		/// Given an input text buffer data pointer, parses a single token into the variable token and returns the new
		///  reading position
		virtual const char			*ParseFile( const char *data, char *token, int maxlen ) = 0;

#ifdef MAPBASE
		/// (Optional) Same as ParseFile, but with casing preserved and escaped quotes supported
		virtual const char			*ParseFilePreserve( const char *data, char *token, int maxlen ) { return ParseFile( data, token, maxlen ); }
#endif

		/// Return a pointer to an IFileSystem we can use to read and process scripts.
		virtual IFileSystem *GetFilesystem() = 0;

		/// Return a pointer to an instance of an IUniformRandomStream
		virtual IUniformRandomStream *GetRandomStream() = 0 ;

		/// Return a pointer to a tier0 ICommandLine
		virtual ICommandLine *GetCommandLine() = 0;

		/// Emulates the server's UTIL_LoadFileForMe
		virtual byte *LoadFileForMe( const char *filename, int *pLength ) = 0;

		/// Emulates the server's UTIL_FreeFile
		virtual void  FreeFile( byte *buffer ) = 0;


		/// Somewhere in the host executable you should define this symbol and 
		/// point it at a singleton instance.
		static IEngineEmulator *s_pSingleton;

		// this is just a function that returns the pointer above -- just in
		// case we need to define it differently. And I get asserts this way.
		static IEngineEmulator *Get();
	};
};


#endif