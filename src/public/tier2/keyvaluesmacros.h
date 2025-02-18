//===================== Copyright (c) Valve Corporation. All Rights Reserved. ======================
//
//==================================================================================================

#pragma once


//--------------------------------------------------------------------------------------------------
// Returns true if the passed string matches the filename style glob, false otherwise
// * matches any characters, ? matches any single character, otherwise case insensitive matching
//--------------------------------------------------------------------------------------------------
bool GlobMatch( const char *pszGlob, const char *pszString );

//--------------------------------------------------------------------------------------------------
// Processes #insert and #update KeyValues macros
//
// #insert inserts a new KeyValues file replacing the KeyValues #insert with the new file
//
// #update updates sibling KeyValues blocks subkeys with its subkeys, overwriting and adding
// KeyValues as necessary
//--------------------------------------------------------------------------------------------------
KeyValues *HandleKeyValuesMacros( KeyValues *kv, KeyValues *pkvParent = nullptr );