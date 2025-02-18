//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include <ctype.h>
#include "tf_autorp.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFAutoRP *AutoRP( void )
{
	static CTFAutoRP *pSystem = NULL;
	if ( !pSystem )
	{
		pSystem = new CTFAutoRP();
		pSystem->ParseDataFile();
	}

	return pSystem;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAutoRP::ParseDataFile( void )
{
	Assert( !m_pDataFileKV );

	// Load & parse the word files
	KeyValues *pFileKV = new KeyValues( "AutoRPFile" );
	if ( pFileKV->LoadFromFile( filesystem, "scripts/autorp.txt", "MOD" ) == false )
		return;

	m_pDataFileKV = pFileKV->MakeCopy();

	// Prepended word list
	KeyValues *pKVPrepended = m_pDataFileKV->FindKey( "prepended_words" );
	if ( pKVPrepended )
	{
		FOR_EACH_SUBKEY( pKVPrepended, pKVKey )
		{
			m_a_pszPrependedWords.AddToTail( pKVKey->GetName() );
		}
	}

	// Appended word list
	KeyValues *pKVAppended = m_pDataFileKV->FindKey( "appended_words" );
	if ( pKVAppended )
	{
		FOR_EACH_SUBKEY( pKVAppended, pKVKey )
		{
			m_a_pszAppendedWords.AddToTail( pKVKey->GetName() );
		}
	}

	// Word replacements
	KeyValues *pKVReplacements = m_pDataFileKV->FindKey( "word_replacements" );
	if ( pKVReplacements )
	{
		FOR_EACH_SUBKEY( pKVReplacements, pKVEntry )
		{
			int iIdx = m_a_Replacements.AddToTail();
			m_a_Replacements[iIdx].iChance = 1;
			m_a_Replacements[iIdx].iPrePendCount = 1;
			FOR_EACH_SUBKEY( pKVEntry, pKVKey )
			{
				const char *pszKey = pKVKey->GetName();
				const char *pszValue = pKVKey->GetString();

				if ( FStrEq(pszKey,"replacement") )
				{
					m_a_Replacements[iIdx].a_pszReplacements.AddToTail( pszValue );
				}
				else if ( FStrEq(pszKey,"replacement_prepend") )
				{
					m_a_Replacements[iIdx].a_pszPrepended.AddToTail( pszValue );
				}
				else if ( FStrEq(pszKey,"replacement_plural") )
				{
					m_a_Replacements[iIdx].a_pszPluralReplacements.AddToTail( pszValue );
				}
				else if ( FStrEq(pszKey,"prepend_count") )
				{
					m_a_Replacements[iIdx].iPrePendCount = pKVKey->GetInt();
				}
				else if ( FStrEq(pszKey,"chance") )
				{
					m_a_Replacements[iIdx].iChance = pKVKey->GetInt();
				}
				else if ( FStrEq(pszKey,"word") )
				{
					m_a_Replacements[iIdx].m_Words.AddToTail( m_pWordTable->AddString( pszValue ) );
				}
				else if ( FStrEq(pszKey,"word_plural") )
				{
					m_a_Replacements[iIdx].m_Plurals.AddToTail( m_pWordTable->AddString( pszValue ) );
				}
				else if ( FStrEq(pszKey,"prev") )
				{
					m_a_Replacements[iIdx].m_PrevWords.AddToTail( m_pWordTable->AddString( pszValue ) );
				}
				else
				{
					Assert(0);
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAutoRP::ApplyRPTo( char *pBuf, int iBufSize )
{
	if ( !m_pDataFileKV )
		return;
	if ( !pBuf || !pBuf[0] )
		return;

	// Ignore sourceMod commands
	if ( pBuf[0] == '!' || pBuf[0] == '/' )
		return;

	bool bDoPends = true;

	char *pszIn = new char[iBufSize];
	if ( pBuf[0] == '-' )
	{
		bDoPends = false;
		Q_strncpy( pszIn, pBuf+1, iBufSize-1 );	
	}
	else
	{
		Q_strncpy( pszIn, pBuf, iBufSize );
	}
	pBuf[0] = '\0';
	
	ModifySpeech( pszIn, pBuf, iBufSize, bDoPends, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFAutoRP::GetRandomPre( void )
{
	if ( RandomInt(1,4) != 1 )
		return NULL;

	if ( !m_a_pszPrependedWords.Count() )
		return NULL;

	static int iPrevPre = 0;
	iPrevPre += RandomInt(1,4);
	while ( iPrevPre >= m_a_pszPrependedWords.Count() )
	{
		iPrevPre -= m_a_pszPrependedWords.Count();
	}

	return m_a_pszPrependedWords[iPrevPre];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFAutoRP::GetRandomPost( void )
{
	if ( RandomInt(1,5) != 1 )
		return NULL;

	if ( !m_a_pszAppendedWords.Count() )
		return NULL;

	static int iPrevPost = 0;
	iPrevPost += RandomInt(1,3);
	while ( iPrevPost >= m_a_pszAppendedWords.Count() )
	{
		iPrevPost -= m_a_pszAppendedWords.Count();
	}

	return m_a_pszAppendedWords[iPrevPost];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
matchresult_t CTFAutoRP::WordMatches( wordreplacement_t *pRep, replacementcheck_t *pCheck )
{
	if ( pRep->iChance != 1 )
	{
		if ( RandomInt( 1, pRep->iChance ) > 1 )
			return MATCHES_NOT;
	}

	// If it has prewords, make sure the preword matches first
	if ( pRep->m_PrevWords.Count() > 0 )
	{
		if ( pCheck->iPrevLen <= 0 )
			return MATCHES_NOT;

		CUtlSymbol sym = m_pWordTable->Find( pCheck->szPrevWord );
		if ( UTL_INVAL_SYMBOL == sym )
			return MATCHES_NOT;

		bool bMatchPrev = false;
		FOR_EACH_VEC( pRep->m_PrevWords, i )
		{
			if ( pRep->m_PrevWords[i] == sym )
			{
				bMatchPrev = true;
				break;
			}
		}	

		if ( !bMatchPrev )
			return MATCHES_NOT;

		pCheck->bUsedPrevWord = true;
	}

	CUtlSymbol sym = m_pWordTable->Find( pCheck->szWord );
	FOR_EACH_VEC( pRep->m_Words, i )
	{
		if ( pRep->m_Words[i] == sym )
			return MATCHES_SINGULAR;
	}	

	CUtlSymbol pluralsym = m_pWordTable->Find( pCheck->szWord );
	FOR_EACH_VEC( pRep->m_Plurals, i )
	{
		if ( pRep->m_Plurals[i] == pluralsym )
			return MATCHES_PLURAL;
	}	

	pCheck->bUsedPrevWord = false;
	return MATCHES_NOT;	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFAutoRP::ReplaceWord( replacementcheck_t *pCheck, char *szRep, int iRepSize, bool bSymbols, bool bWordListOnly )
{
	szRep[0] = '\0';

	// First, see if we have a replacement
	FOR_EACH_VEC( m_a_Replacements, i )
	{
		wordreplacement_t *pRep = &m_a_Replacements[i];
		matchresult_t iRes = WordMatches( pRep, pCheck ); 
		if ( iRes == MATCHES_NOT )
			continue;

		if ( pRep->a_pszPrepended.Count() > 0 )
		{
			CUtlVector<int> vecUsed;
			for ( int iCount = 0; iCount < pRep->iPrePendCount; iCount++ )
			{
				// Ensure we don't choose two of the same prepends
				int rnd = 0;
				do
				{
					rnd = RandomInt( 0, (int)pRep->a_pszPrepended.Count() - 1 );
				} while ( vecUsed.Find(rnd) != vecUsed.InvalidIndex() );
				vecUsed.AddToTail(rnd);

				Q_strncat( szRep, pRep->a_pszPrepended[rnd], iRepSize );
				if ( (iCount+1) < pRep->iPrePendCount )
				{
					Q_strncat( szRep, ", ", iRepSize );
				}
				else
				{
					Q_strncat( szRep, " ", iRepSize );
				}
			}
		}

		if ( iRes == MATCHES_SINGULAR )
		{
			int rnd = RandomInt( 0, (int)pRep->a_pszReplacements.Count() - 1 );
			Q_strncat( szRep, pRep->a_pszReplacements[rnd], iRepSize );
		}
		else if ( iRes == MATCHES_PLURAL )
		{
			int rnd = RandomInt( 0, (int)pRep->a_pszPluralReplacements.Count() - 1 );
			Q_strncat( szRep, pRep->a_pszPluralReplacements[rnd], iRepSize );
		}

		return true;
	}

	if ( !bSymbols && !bWordListOnly )
	{
		char fc = pCheck->szWord[0];

		// Randomly replace h's at the front of words with apostrophes
		if ( fc == 'h' && RandomInt(1,2) == 1 )
		{
			Q_strncpy( szRep, pCheck->szWord, MIN( iRepSize, pCheck->iWordLen+1 ) );
			szRep[0] = '\'';
			return true;
		}

		char lc = pCheck->szWord[ pCheck->iWordLen-1 ];
		if ( pCheck->iWordLen > 3 )
		{
			char slc = pCheck->szWord[ pCheck->iWordLen-2 ];
			char lllc = pCheck->szWord[ pCheck->iWordLen-3 ];

			// Randomly modify words ending in "ed", by replacing the "e" with an apostrophe
				//		i.e. "worked" -> "work'd", "waited" -> "wait'd"
			if ( slc == 'e' && lc == 'd' && lllc != 'e' && RandomInt(1,4) == 1 )
			{
				Q_strncpy( szRep, pCheck->szWord, MIN( iRepSize, pCheck->iWordLen+1 ) );
				szRep[ pCheck->iWordLen-2 ] = '\'';
				return true;
			}

			// Randomly append "th" or "st" to any word ending in "ke"
				//		i.e. "take" -> "taketh", "broke" -> "brokest"
			if ( slc == 'k' && lc == 'e' && RandomInt(1,3) == 1 )
			{
				Q_strncpy( szRep, pCheck->szWord, MIN( iRepSize, pCheck->iWordLen+1 ) );
				if ( RandomInt(1,2) == 1 )
				{
					Q_strncat( szRep, "th", iRepSize );
				}
				else
				{
					Q_strncat( szRep, "st", iRepSize );
				}
				return true;
			}
		}

		if ( pCheck->iWordLen >= 3 )
		{
			char slc = pCheck->szWord[ pCheck->iWordLen-2 ];

			// Randomly append "eth" to words with appropriate last letters.
			if ( RandomInt( 1, 5 ) == 1 &&
				(lc == 't' || lc == 'p' || lc == 'k' || lc == 'g' || lc == 'b' || lc == 'w') )
			{
				Q_strncpy( szRep, pCheck->szWord, MIN( iRepSize, pCheck->iWordLen+1 ) );
				Q_strncat( szRep, "eth", iRepSize );
				return true;
			}

			// Randomly append "est" to any word ending in "ss"
				//	i.e. "pass" -> "passest", "class" -> "classest"
			if ( lc == 's' && slc == 's' && RandomInt(1,5) == 1 )
			{
				Q_strncpy( szRep, pCheck->szWord, MIN( iRepSize, pCheck->iWordLen+1 ) );
				Q_strncat( szRep, "est", iRepSize );
				return true;
			}
		}

		if ( pCheck->iWordLen > 4 )
		{
			// Randomly prepend "a-" to words ending in "ing", and randomly replace the trailing g with an apostrophe
			//		i.e. "coming" -> "a-comin'", "dancing" -> "a-dancing"
			char slc = pCheck->szWord[ pCheck->iWordLen-2 ];
			char lllc = pCheck->szWord[ pCheck->iWordLen-3 ];
			if ( lllc == 'i' && slc == 'n' && lc == 'g' )
			{
				char sc = pCheck->szWord[2];
				if ( sc != '-' )
				{
					Q_strncpy( szRep, "a-", iRepSize );

					if ( RandomInt(1,2) == 1 )
					{
						Q_strncat( szRep, pCheck->szWord, iRepSize, pCheck->iWordLen );
					}
					else
					{
						Q_strncat( szRep, pCheck->szWord, iRepSize, pCheck->iWordLen-1 );
						Q_strncat( szRep, "'", iRepSize );
					}
					return true;
				}
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFAutoRP::PerformReplacement( const char *pszReplacement, replacementcheck_t *pRepCheck, char *szStoredWord, int iStoredWordSize, char *pszOutText, int iOutLen )
{
	if ( pszReplacement && pszReplacement[0] )
	{
		// Check to see if the previous word should be modified
		char fc = tolower( *pszReplacement );
		if ( !_strnicmp( pRepCheck->szPrevWord, "an", MAX(pRepCheck->iPrevLen,2) ) )
		{
			if ( fc != 'a' && fc != 'e' && fc != 'i' && fc != 'o' && fc != 'u' )
			{
				// Remove the trailing n
				int iLen = (int)strlen( szStoredWord );
				szStoredWord[iLen-1] = '\0';	// Move back 3. 1 for null, 1 for space, 1 for n.
			}
		}
		else if ( *pRepCheck->szPrevWord == 'a' && pRepCheck->iPrevLen == 1 )
		{
			if ( fc == 'a' || fc == 'e' || fc == 'i' || fc == 'o' || fc == 'u' )
			{
				// Add a trailing n
				Q_strncat( szStoredWord, "n", iStoredWordSize );
			}
		}
	}

	// Only append the previous word if we didn't use it in our replacement
	if ( !pRepCheck->bUsedPrevWord )
	{
		// Append the previous word
		Q_strncat( pszOutText, szStoredWord, iOutLen );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAutoRP::ModifySpeech( const char *pszInText, char *pszOutText, int iOutLen, bool bGeneratePreAndPost, bool bInPrePost )
{	
	if ( bGeneratePreAndPost )
	{
		// See if we generate a pre. If we do, modify it as well so we can perform replacements on it.
		const char *pszPre = GetRandomPre();
		if ( pszPre && pszPre[0] )
		{
			ModifySpeech( pszPre, pszOutText, iOutLen, false, true );
			Q_strncat( pszOutText, " ", iOutLen );
		}
	}

	// Iterate through all the words and test them vs our replacement list
	const char *pszPrevWord = pszInText;
	const char *pszCurWord = pszInText;
	const char *pszCh = pszInText;
	char szStoredWord[128];
	szStoredWord[0] = '\0';
	char szCurrentWord[128];
	szCurrentWord[0] = '\0';

	replacementcheck_t repCheck;

	while ( 1 )
	{
		if ( (*pszCh >= 'A' && *pszCh <= 'Z') || (*pszCh >= 'a' && *pszCh <= 'z') || *pszCh == '&' )
		{
			pszCh++;
			continue;
		}

		// Hit the end of a word/string.
		int iCurLen = (int)(pszCh - pszCurWord);
		int iPrevLen = MAX( 0, (int)(pszCurWord - pszPrevWord) - 1 ); // -1 for the space

		bool bModifyWord = true;
		bool bSkipOneLetter = false;
		// Pre/Post pend blocks only modify words that start with an '&'
		if ( bInPrePost )
		{
			bModifyWord = ( pszCurWord[0] == '&' );
			bSkipOneLetter = bModifyWord;
		}

		if ( bSkipOneLetter )
		{
			Q_strncpy( repCheck.szWord, pszCurWord+1, iCurLen );
			repCheck.iWordLen = iCurLen-1;
		}
		else
		{
			Q_strncpy( repCheck.szWord, pszCurWord, iCurLen+1 );
			repCheck.iWordLen = iCurLen;
		}
		
		Q_strncpy( repCheck.szPrevWord, pszPrevWord, iPrevLen+1 );
		repCheck.iPrevLen = iPrevLen;
		repCheck.bUsedPrevWord = false;

		if ( iCurLen > 0 )
		{
			bool bChanged = bModifyWord ? ReplaceWord( &repCheck, szCurrentWord, sizeof(szCurrentWord), false, bInPrePost ) : false;

			// If the character that broke the last two words apart was an apostrophe, see if we can replace the whole word
			if ( !bChanged && bModifyWord )
			{
				if ( szStoredWord[0] )
				{
					int iLen = Q_strlen(szStoredWord);
					if ( szStoredWord[iLen-1] == '\'' )
					{
						Q_strncpy( repCheck.szWord, szStoredWord, MIN( sizeof(repCheck.szWord),iLen+1 ) );
						Q_strncat( repCheck.szWord, pszCurWord, sizeof(repCheck.szWord), iCurLen );
						repCheck.iWordLen = iLen + iCurLen;
						repCheck.szPrevWord[0] = '\0';
						repCheck.iPrevLen = 0;

						bChanged = ReplaceWord( &repCheck, szCurrentWord, sizeof(szCurrentWord), false, bInPrePost );
						if ( bChanged )
						{
							repCheck.bUsedPrevWord = true;
						}
					}
				}
			}

			if ( szStoredWord[0] != '\0' )
			{
				if ( PerformReplacement( szCurrentWord, &repCheck, szStoredWord, sizeof(szStoredWord), pszOutText, iOutLen ) )
				{
					// Append a space, but not if the last character is an apostrophe
					int iLen = Q_strlen(szStoredWord);
					if ( szStoredWord[iLen-1] != '\'' )
					{
						Q_strncat( pszOutText, " ", iOutLen );
					}
				}
			}

			if ( bChanged )
			{
				Q_strncpy( szStoredWord, szCurrentWord, sizeof(szStoredWord) );

				// Match case of the first letter in the word we're replacing
				if ( pszCurWord[0] >= 'A' && pszCurWord[0] <= 'Z' )
				{
					szStoredWord[0] = toupper( szStoredWord[0] );
				}
				else if ( pszCurWord[0] >= 'a' && pszCurWord[0] <= 'a' )
				{
					szStoredWord[0] = tolower( szStoredWord[0] );
				}
			}
			else
			{
				Q_strncpy( szStoredWord, pszCurWord, MIN( (int)sizeof(szStoredWord), (int)(pszCh - pszCurWord)+1 ) );
			}
		}

		// Finished?
		if ( *pszCh == '\0' )
		{
			repCheck.bUsedPrevWord = false;
			if ( szStoredWord[0] != '\0' )
			{
				PerformReplacement( NULL, &repCheck, szStoredWord, sizeof(szStoredWord), pszOutText, iOutLen );
			}
			break;
		}

		// If it wasn't a space that ended this word, try checking it for a symbol
		if ( *pszCh != ' ' )
		{
			Q_strncpy( repCheck.szWord, pszCh, 2 );
			repCheck.iWordLen = 1;
			repCheck.iPrevLen = 0;
			repCheck.bUsedPrevWord = false;

			char szSymbolRep[128];
			szSymbolRep[0] = '\0';
			if ( ReplaceWord( &repCheck, szSymbolRep, sizeof(szSymbolRep), true, true ) )
			{
				Q_strncat( szStoredWord, szSymbolRep, (int)sizeof(szStoredWord) );
			}
			else
			{
				Q_strncat( szStoredWord, pszCh, (int)sizeof(szStoredWord), 1 );
			}
		}

		// Move on
		pszCh++;
		pszPrevWord = pszCurWord;
		pszCurWord = pszCh;
	}

	if ( bGeneratePreAndPost )
	{
		int iLen = (int)strlen( pszOutText );
		char pszLC = pszOutText[iLen-1];
		if ( pszLC != '?' && pszLC != '!' )
		{
			// See if we generate a post. If we do, modify it as well so we can perform replacements on it.
			const char *pszPost = GetRandomPost();
			if ( pszPost && pszPost[0] )
			{
				if ( pszLC != '.' )
				{
					Q_strncat( pszOutText, ". ", iOutLen );
				}
				else
				{
					Q_strncat( pszOutText, " ", iOutLen );
				}

				ModifySpeech( pszPost, pszOutText, iOutLen, false, true );
			}
		}
	}
}