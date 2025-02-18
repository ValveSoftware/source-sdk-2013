//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_AUTORP_H
#define TF_AUTORP_H
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "utlvector.h"
#include "utlmap.h"

enum matchresult_t
{
	MATCHES_NOT,
	MATCHES_SINGULAR,
	MATCHES_PLURAL,
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFAutoRP : public CAutoGameSystem
{
public:
	CTFAutoRP() : CAutoGameSystem( "CTFAutoRP" )
	{
		m_pDataFileKV = NULL;
		m_pWordTable = new CUtlSymbolTable( 0, 32, true );
	}

	void	ParseDataFile( void );
	void	ApplyRPTo( char *pBuf, int iBufSize );

private:
	struct wordreplacement_t
	{
		int		iChance;
		int		iPrePendCount;
		CUtlVector<const char*> a_pszPrepended;				// Words that prepend the replacement
		CUtlVector<const char*> a_pszReplacements;			// Words that replace the original word
		CUtlVector<const char*> a_pszPluralReplacements;	// If the match was a plural match, use these replacements instead, if they exist. Otherwise, use a_pszReplacements.
		CUtlVector<CUtlSymbol>	m_Words;		// Word that matches this replacement
		CUtlVector<CUtlSymbol>	m_Plurals;	// Word that must come before to match this replacement, for double word replacements (i.e. "it is" -> "'tis")
		CUtlVector<CUtlSymbol>	m_PrevWords;	// Word that must come before to match this replacement, for double word replacements (i.e. "it is" -> "'tis")
	};

	struct replacementcheck_t
	{
		char		szWord[128];
		int			iWordLen;
		char		szPrevWord[128];
		int			iPrevLen;

		bool		bUsedPrevWord;
	};

private:
	const char		*GetRandomPre( void );
	const char		*GetRandomPost( void );
	void			ModifySpeech( const char *pszInText, char *pszOutText, int iOutLen, bool bGeneratePreAndPost, bool bInPrePost );
	matchresult_t	WordMatches( wordreplacement_t *pRep, replacementcheck_t *pCheck );
	bool			ReplaceWord( replacementcheck_t *pCheck, char *szRep, int iRepSize, bool bSymbols, bool bWordListOnly );
	bool			PerformReplacement( const char *pszReplacement, replacementcheck_t *pRepCheck, char *szStoredWord, int iStoredWordSize, char *pszOutText, int iOutLen );

private:
	// Database
	KeyValues *m_pDataFileKV;
	// Storage of all replacement blocks
	CUtlVector<wordreplacement_t>	m_a_Replacements;
	CUtlSymbolTable				*m_pWordTable;

	// Extra lists for random selection
	CUtlVector<const char*>		 m_a_pszPrependedWords;
	CUtlVector<const char*>		 m_a_pszAppendedWords;

	// Current application
	CUtlVector<const char*>		 *m_pszCurrentList;
	int							 m_iCurrentReplacement;
};

extern CTFAutoRP *AutoRP( void );

#endif // TF_AUTORP_H
