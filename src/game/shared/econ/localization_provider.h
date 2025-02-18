//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: provide a layer of abstraction between GC and vgui localization systems
//
//=============================================================================

#ifndef LOCALIZATION_PROVIDER_H
#define LOCALIZATION_PROVIDER_H

#ifdef _WIN32
#pragma once
#endif

#include "language.h"
#include "ilocalize.h"


// interface matches a subset of VGUI functions
class CLocalizationProvider
{
public:
	virtual locchar_t *Find( const char *pchKey ) const = 0;
	locchar_t* FindSafe( const char* pchKey ) const;

	// new interface
	virtual void ConvertLoccharToANSI	( const locchar_t *loc_In, CUtlConstString *out_ansi ) const = 0;
	virtual void ConvertLoccharToUnicode( const locchar_t *loc_In, CUtlConstWideString *out_unicode ) const = 0;
	virtual void ConvertUTF8ToLocchar	( const char *utf8_In, CUtlConstStringBase<locchar_t> *out_loc ) const = 0;
	
	// old C-style interface
	virtual int ConvertLoccharToANSI( const locchar_t *loc, OUT_Z_BYTECAP(ansiBufferSize) char *ansi, int ansiBufferSize ) const = 0;
	virtual int ConvertLoccharToUnicode( const locchar_t *loc, OUT_Z_BYTECAP(unicodeBufferSize) wchar_t *unicode, int unicodeBufferSize ) const = 0;

	virtual void ConvertUTF8ToLocchar( const char *utf8, OUT_Z_BYTECAP(loccharBufferSize) locchar_t *locchar, int loccharBufferSize ) const = 0;

	virtual ELanguage GetELang() const = 0;
};
CLocalizationProvider *GLocalizationProvider();


#include "vgui/ILocalize.h"
extern vgui::ILocalize				*g_pVGuiLocalize;

// Game localization is handled by vgui
class CVGUILocalizationProvider : public CLocalizationProvider
{
public:
	CVGUILocalizationProvider();

	virtual locchar_t *Find( const char *pchKey ) const;
	
	// new interface
	virtual void ConvertLoccharToANSI	( const locchar_t *loc_In, CUtlConstString *out_ansi ) const;
	virtual void ConvertLoccharToUnicode( const locchar_t *loc_In, CUtlConstWideString *out_unicode ) const;
	virtual void ConvertUTF8ToLocchar	( const char *utf8_In, CUtlConstStringBase<locchar_t> *out_loc ) const;

	// old C-style interface
	virtual int ConvertLoccharToANSI( const locchar_t *loc, char *ansi, int ansiBufferSize ) const;
	virtual int ConvertLoccharToUnicode( const locchar_t *loc, wchar_t *unicode, int unicodeBufferSize ) const;

	virtual void ConvertUTF8ToLocchar( const char *utf8, locchar_t *locchar, int loccharBufferSize ) const;

	virtual ELanguage GetELang() const { return k_Lang_None; }

};

#endif // LOCALIZATION_PROVIDER_H
