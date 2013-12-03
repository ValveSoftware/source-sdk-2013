//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include <windows.h>
#include <stdio.h>

//-----------------------------------------------------------------------------
// Purpose: converts an english string to unicode
//-----------------------------------------------------------------------------
int ConvertANSIToUnicode(const char *ansi, wchar_t *unicode, int unicodeBufferSize)
{
	return ::MultiByteToWideChar(CP_ACP, 0, ansi, -1, unicode, unicodeBufferSize);
}

char *va( const char *fmt, ... )
{
	va_list args;
	static char output[4][1024];
	static int outbuffer = 0;

	outbuffer++;
	va_start( args, fmt );
	vprintf( fmt, args );
	vsprintf( output[ outbuffer & 3 ], fmt, args );
	return output[ outbuffer & 3 ];
}