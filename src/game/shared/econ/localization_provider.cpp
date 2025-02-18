//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "localization_provider.h"

enum { kScratchBufferSize = 1024 };



// ----------------------------------------------------------------------------
// Find a localized string, but return something safe if the key is null or the localized
// string is missing.
// ----------------------------------------------------------------------------
locchar_t* CLocalizationProvider::FindSafe( const char* pchKey ) const
{
	if ( pchKey )
	{
		locchar_t* wszLocalized = Find( pchKey );
		if ( !wszLocalized )
		{
			return const_cast<locchar_t*>(LOCCHAR(""));
		}
		else
		{
			return wszLocalized;
		}
	}
	else
	{
		return const_cast<locchar_t*>(LOCCHAR(""));
	}
}


CLocalizationProvider *GLocalizationProvider() 
{
	static CVGUILocalizationProvider g_VGUILocalizationProvider;
	return &g_VGUILocalizationProvider;
}

// vgui localization implementation

CVGUILocalizationProvider::CVGUILocalizationProvider()
{

}

locchar_t *CVGUILocalizationProvider::Find( const char *pchKey ) const
{
	return (locchar_t*)g_pVGuiLocalize->Find( pchKey );
}

void CVGUILocalizationProvider::ConvertLoccharToANSI( const locchar_t *loc_In, CUtlConstString *out_ansi ) const
{
	char ansi_Scratch[kScratchBufferSize];

	g_pVGuiLocalize->ConvertUnicodeToANSI( loc_In, ansi_Scratch, kScratchBufferSize );
	*out_ansi = ansi_Scratch;
}

void CVGUILocalizationProvider::ConvertLoccharToUnicode( const locchar_t *loc_In, CUtlConstWideString *out_unicode ) const
{
	*out_unicode = loc_In;
}

void CVGUILocalizationProvider::ConvertUTF8ToLocchar( const char *utf8_In, CUtlConstStringBase<locchar_t> *out_loc ) const
{
	locchar_t loc_Scratch[kScratchBufferSize];

	V_UTF8ToUnicode( utf8_In, loc_Scratch, kScratchBufferSize );
	*out_loc = loc_Scratch;
}

void CVGUILocalizationProvider::ConvertUTF8ToLocchar( const char *utf8, locchar_t *locchar, int loccharBufferSize ) const
{
	V_UTF8ToUnicode( utf8, locchar, loccharBufferSize );
}

int CVGUILocalizationProvider::ConvertLoccharToANSI( const locchar_t *loc, char *ansi, int ansiBufferSize ) const
{
	return g_pVGuiLocalize->ConvertUnicodeToANSI( loc, ansi, ansiBufferSize );
}

int CVGUILocalizationProvider::ConvertLoccharToUnicode( const locchar_t *loc, wchar_t *unicode, int unicodeBufferSize ) const
{
	Q_wcsncpy( unicode, loc, unicodeBufferSize );
	return 0;
}


