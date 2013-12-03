//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef UNICODEFILEHELPERS_H
#define UNICODEFILEHELPERS_H
#ifdef _WIN32
#pragma once
#endif

#include <stdlib.h>

// helper functions for parsing unicode file buffers
ucs2 *AdvanceOverWhitespace(ucs2 *start);
ucs2 *ReadUnicodeToken(ucs2 *start, ucs2 *token, int tokenBufferSize, bool &quoted);
ucs2 *ReadUnicodeTokenNoSpecial(ucs2 *start, ucs2 *token, int tokenBufferSize, bool &quoted);
ucs2 *ReadToEndOfLine(ucs2 *start);

// writing to unicode files via CUtlBuffer
class CUtlBuffer;
void WriteUnicodeString(CUtlBuffer &buffer, const wchar_t *string, bool addQuotes = false);
void WriteAsciiStringAsUnicode(CUtlBuffer &buffer, const char *string, bool addQuotes = false);



#endif // UNICODEFILEHELPERS_H
