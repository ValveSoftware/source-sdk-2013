//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements an interface for reading and writing heirarchical
//			text files of key value pairs. The format of the file is as follows:
//
// chunkname0
// {
//		"key0" "value0"
//		"key1" "value1"
//		...
//		"keyN" "valueN"
//		chunkname1
//		{
//			"key0" "value0"
//			"key1" "value1"
//			...
//			"keyN" "valueN"
//		}
// }
// ...
// chunknameN
// {
//		"key0" "value0"
//		"key1" "value1"
//		...
//		"keyN" "valueN"
// }
//
// The chunk names are not necessarily unique, nor are the key names, unless the
// parsing application requires them to be.
//
// $NoKeywords: $
//=============================================================================//

#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#endif
#include <math.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "chunkfile.h"
#include "mathlib/vector.h"
#include "mathlib/vector4d.h"
#include "tier1/strtools.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CChunkHandlerMap::CChunkHandlerMap(void)
{
	m_pHandlers = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Frees handler list.
//-----------------------------------------------------------------------------
CChunkHandlerMap::~CChunkHandlerMap(void)
{
	ChunkHandlerInfoNode_t *pNode = m_pHandlers;
	while (pNode != NULL)
	{
		ChunkHandlerInfoNode_t *pPrev = pNode;
		pNode = pNode->pNext;

		delete pPrev;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Adds a chunk handler to the handler list.
// Input  : pszChunkName - Name of chunk to be handled.
//			pfnHandler - Address of handler callback function.
//			pData - Data to pass to the handler callback.
//-----------------------------------------------------------------------------
void CChunkHandlerMap::AddHandler(const char *pszChunkName, ChunkHandler_t pfnHandler, void *pData)
{
	ChunkHandlerInfoNode_t *pNew = new ChunkHandlerInfoNode_t;

	Q_strncpy(pNew->Handler.szChunkName, pszChunkName, sizeof( pNew->Handler.szChunkName ));
	pNew->Handler.pfnHandler = pfnHandler;
	pNew->Handler.pData = pData;
	pNew->pNext = NULL;

	if (m_pHandlers == NULL)
	{
		m_pHandlers = pNew;
	}
	else
	{
		ChunkHandlerInfoNode_t *pNode = m_pHandlers;
		while (pNode->pNext != NULL)
		{
			pNode = pNode->pNext;
		}
		pNode->pNext = pNew;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the callback for error handling within this chunk's scope.
// Input  : pfnHandler - 
//			pData - 
//-----------------------------------------------------------------------------
void CChunkHandlerMap::SetErrorHandler(ChunkErrorHandler_t pfnHandler, void *pData)
{
	m_pfnErrorHandler = pfnHandler;
	m_pErrorData = pData;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ppData - 
// Output : ChunkErrorHandler_t
//-----------------------------------------------------------------------------
ChunkErrorHandler_t CChunkHandlerMap::GetErrorHandler(void **ppData)
{
	*ppData = m_pErrorData;
	return(m_pfnErrorHandler);
}


//-----------------------------------------------------------------------------
// Purpose: Gets the handler for a given chunk name, if one has been set.
// Input  : pszChunkName - Name of chunk.
//			ppfnHandler - Receives the address of the callback function.
//			ppData - Receives the context data for the given chunk.
// Output : Returns true if a handler was found, false if not.
//-----------------------------------------------------------------------------
ChunkHandler_t CChunkHandlerMap::GetHandler(const char *pszChunkName, void **ppData)
{
	ChunkHandlerInfoNode_t *pNode = m_pHandlers;
	while (pNode != NULL)
	{
		if (!stricmp(pNode->Handler.szChunkName, pszChunkName))
		{
			*ppData = pNode->Handler.pData;
			return(pNode->Handler.pfnHandler);
		}

		pNode = pNode->pNext;
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: Constructor. Initializes data members.
//-----------------------------------------------------------------------------
CChunkFile::CChunkFile(void)
{
	m_hFile = NULL;
	m_nCurrentDepth = 0;
	m_szIndent[0] = '\0';
	m_nHandlerStackDepth = 0;
	m_DefaultChunkHandler = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Closes the file if it is currently open.
//-----------------------------------------------------------------------------
CChunkFile::~CChunkFile(void)
{
	if (m_hFile != NULL)
	{
		fclose(m_hFile);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszChunkName - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CChunkFile::BeginChunk(const char *pszChunkName)
{
	//
	// Write the chunk name and open curly.
	//
	char szBuf[MAX_KEYVALUE_LEN];
	Q_snprintf(szBuf, sizeof( szBuf ), "%s\r\n%s{", pszChunkName, m_szIndent);
	ChunkFileResult_t eResult = WriteLine(szBuf);

	//
	// Update the indentation depth.
	//
	if (eResult == ChunkFile_Ok)
	{
		m_nCurrentDepth++;
		BuildIndentString(m_szIndent, m_nCurrentDepth);
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CChunkFile::BuildIndentString(char *pszDest, int nDepth)
{
	if (nDepth >= 0)
	{
		for (int i = 0; i < nDepth; i++)
		{
			pszDest[i] = '\t';
		}

		pszDest[nDepth] = '\0';
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CChunkFile::Close(void)
{
	if (m_hFile != NULL)
	{
		fclose(m_hFile);
		m_hFile = NULL;
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CChunkFile::EndChunk(void)
{
	if (m_nCurrentDepth > 0)
	{
		m_nCurrentDepth--;
		BuildIndentString(m_szIndent, m_nCurrentDepth);
	}

	WriteLine("}");

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: Returns a string explaining the last error that occurred.
//-----------------------------------------------------------------------------
const char *CChunkFile::GetErrorText(ChunkFileResult_t eResult)
{
	static char szError[MAX_KEYVALUE_LEN];

	switch (eResult)
	{
		case ChunkFile_UnexpectedEOF:
		{
			Q_strncpy(szError, "unexpected end of file", sizeof( szError ) );
			break;
		}

		case ChunkFile_UnexpectedSymbol:
		{
			Q_snprintf(szError, sizeof( szError ), "unexpected symbol '%s'", m_szErrorToken);
			break;
		}

		case ChunkFile_OpenFail:
		{
			Q_snprintf(szError, sizeof( szError ), "%s", strerror(errno)) ;
			break;
		}

		case ChunkFile_StringTooLong:
		{
			Q_strncpy(szError, "unterminated string or string too long", sizeof( szError ) );
			break;
		}

		default:
		{
			Q_snprintf(szError, sizeof( szError ), "error %d", eResult);
		}
	}

	return(m_TokenReader.Error(szError));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : eError - 
//-----------------------------------------------------------------------------
void CChunkFile::HandleError(const char *szChunkName, ChunkFileResult_t eError)
{
	// UNDONE: dispatch errors to the error handler.
	// - keep track of current chunkname for reporting errors
	// - use the last non-NULL handler that was pushed onto the stack?
	// - need a return code to determine whether to abort parsing?
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CChunkFile::HandleChunk(const char *szChunkName)
{
	// See if the default handler wants it?
	if( m_DefaultChunkHandler )
	{
		ChunkFileResult_t testResult = m_DefaultChunkHandler( this, m_pDefaultChunkHandlerData, szChunkName );
		if( testResult == ChunkFile_Ok )
		{
			return ChunkFile_Ok;
		}
	}

	//
	// If there is an active handler map...
	//
	if (m_nHandlerStackDepth > 0)
	{
		CChunkHandlerMap *pHandler = m_HandlerStack[m_nHandlerStackDepth - 1];

		//
		// If a chunk handler was found in the handler map...
		//
		void *pData;
		ChunkHandler_t pfnHandler = pHandler->GetHandler(szChunkName, &pData);
		if (pfnHandler != NULL)
		{
			// Dispatch this chunk to the handler.
			ChunkFileResult_t eResult = pfnHandler(this, pData);
			if (eResult != ChunkFile_Ok)
			{
				return(eResult);
			}
		}
		else
		{
			//
			// No handler for this chunk. Skip to the matching close curly brace.
			//
			int nDepth = 1;
			ChunkFileResult_t eResult;

			do
			{
				ChunkType_t eChunkType;
				char szKey[MAX_KEYVALUE_LEN];
				char szValue[MAX_KEYVALUE_LEN];

				while ((eResult = ReadNext(szKey, szValue, sizeof(szValue), eChunkType)) == ChunkFile_Ok)
				{
					if (eChunkType == ChunkType_Chunk)
					{
						nDepth++;
					}
				}

				if (eResult == ChunkFile_EndOfChunk)
				{
					eResult = ChunkFile_Ok;
					nDepth--;
				}
				else if (eResult == ChunkFile_EOF)
				{
					return(ChunkFile_UnexpectedEOF);
				}

			} while ((nDepth) && (eResult == ChunkFile_Ok));
		}
	}
	
	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: Opens the chunk file for reading or writing.
// Input  : pszFileName - Path of file to open.
//			eMode - ChunkFile_Read or ChunkFile_Write.
// Output : Returns ChunkFile_Ok on success, ChunkFile_Fail on failure.
// UNDONE: boolean return value?
//-----------------------------------------------------------------------------
ChunkFileResult_t CChunkFile::Open(const char *pszFileName, ChunkFileOpenMode_t eMode)
{
	if (eMode == ChunkFile_Read)
	{
		// UNDONE: TokenReader encapsulates file - unify reading and writing to use the same file I/O.
		// UNDONE: Support in-memory parsing.
		if (m_TokenReader.Open(pszFileName))
		{
			m_nCurrentDepth = 0;
		}
		else
		{
			return(ChunkFile_OpenFail);
		}	
	}
	else if (eMode == ChunkFile_Write)
	{
		m_hFile = fopen(pszFileName, "wb");

		if (m_hFile == NULL)
		{
			return(ChunkFile_OpenFail);
		}

		m_nCurrentDepth = 0;
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: Removes the topmost set of chunk handlers.
//-----------------------------------------------------------------------------
void CChunkFile::PopHandlers(void)
{
	if (m_nHandlerStackDepth > 0)
	{
		m_nHandlerStackDepth--;
	}
}


void CChunkFile::SetDefaultChunkHandler( DefaultChunkHandler_t pHandler, void *pData )
{
	m_DefaultChunkHandler = pHandler;
	m_pDefaultChunkHandlerData = pData;}


//-----------------------------------------------------------------------------
// Purpose: Adds a set of chunk handlers to the top of the handler stack.
// Input  : pHandlerMap - Object containing the list of chunk handlers.
//-----------------------------------------------------------------------------
void CChunkFile::PushHandlers(CChunkHandlerMap *pHandlerMap)
{
	if (m_nHandlerStackDepth < MAX_INDENT_DEPTH)
	{ 
		m_HandlerStack[m_nHandlerStackDepth] = pHandlerMap;
		m_nHandlerStackDepth++;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Reads the next term from the chunk file. The type of term read is
//			returned in the eChunkType parameter.
// Input  : szName - Name of key or chunk.
//			szValue - If eChunkType is ChunkType_Key, contains the value of the key.
//			nValueSize - Size of the buffer pointed to by szValue.
//			eChunkType - ChunkType_Key or ChunkType_Chunk.
// Output : Returns ChunkFile_Ok on success, an error code if a parsing error occurs.
//-----------------------------------------------------------------------------
ChunkFileResult_t CChunkFile::ReadNext(char *szName, char *szValue, int nValueSize, ChunkType_t &eChunkType)
{
	// HACK: pass in buffer sizes?
	trtoken_t eTokenType = m_TokenReader.NextToken(szName, MAX_KEYVALUE_LEN);

	if (eTokenType != TOKENEOF)
	{
		switch (eTokenType)
		{
			case IDENT:
			case STRING:
			{
				char szNext[MAX_KEYVALUE_LEN];
				trtoken_t eNextTokenType;

				//
				// Read the next token to determine what we have.
				//
				eNextTokenType = m_TokenReader.NextToken(szNext, sizeof(szNext));

				switch (eNextTokenType)
				{
					case OPERATOR:
					{
						if (!stricmp(szNext, "{"))
						{
							// Beginning of new chunk.
							m_nCurrentDepth++;
							eChunkType = ChunkType_Chunk;
							szValue[0] = '\0';
							return(ChunkFile_Ok);
						}
						else
						{
							// Unexpected symbol.
							Q_strncpy(m_szErrorToken, szNext, sizeof( m_szErrorToken ) );
							return(ChunkFile_UnexpectedSymbol);
						}
					}

					case STRING:
					case IDENT:
					{
						// Key value pair.
						Q_strncpy(szValue, szNext, nValueSize );
						eChunkType = ChunkType_Key;
						return(ChunkFile_Ok);
					}

					case TOKENEOF:
					{
						// Unexpected end of file.
						return(ChunkFile_UnexpectedEOF);
					}
					
					case TOKENSTRINGTOOLONG:
					{
						// String too long or unterminated string.
						return ChunkFile_StringTooLong;
					}
				}
			}

			case OPERATOR:
			{
				if (!stricmp(szName, "}"))
				{
					// End of current chunk.
					m_nCurrentDepth--;
					return(ChunkFile_EndOfChunk);
				}
				else
				{
					// Unexpected symbol.
					Q_strncpy(m_szErrorToken, szName, sizeof( m_szErrorToken ) );
					return(ChunkFile_UnexpectedSymbol);
				}
			}

			case TOKENSTRINGTOOLONG:
			{
				return ChunkFile_StringTooLong;
			}
		}
	}

	if (m_nCurrentDepth != 0)
	{
		// End of file while within the scope of a chunk.
		return(ChunkFile_UnexpectedEOF);
	}

	return(ChunkFile_EOF);
}


//-----------------------------------------------------------------------------
// Purpose: Reads the current chunk and dispatches keys and sub-chunks to the
//			appropriate handler callbacks.
// Input  : pfnKeyHandler - Callback for any key values in this chunk.
//			pData - Data to pass to the key value callback function.
// Output : Normally returns ChunkFile_Ok or ChunkFile_EOF. Otherwise, returns
//			a ChunkFile_xxx error code.
//-----------------------------------------------------------------------------
ChunkFileResult_t CChunkFile::ReadChunk(KeyHandler_t pfnKeyHandler, void *pData)
{
	//
	// Read the keys and sub-chunks.
	//
	ChunkFileResult_t eResult;
	do
	{
		char szName[MAX_KEYVALUE_LEN];
		char szValue[MAX_KEYVALUE_LEN];
		ChunkType_t eChunkType;

		eResult = ReadNext(szName, szValue, sizeof(szValue), eChunkType);

		if (eResult == ChunkFile_Ok)
		{
			if (eChunkType == ChunkType_Chunk)
			{
				//
				// Dispatch sub-chunks to the appropriate handler.
				//
				eResult = HandleChunk(szName);
			}
			else if ((eChunkType == ChunkType_Key) && (pfnKeyHandler != NULL))
			{
				//
				// Dispatch keys to the key value handler.
				//
				eResult = pfnKeyHandler(szName, szValue, pData);
			}
		}
	} while (eResult == ChunkFile_Ok);

	//
	// Cover up ChunkFile_EndOfChunk results because the caller doesn't want to see them.
	//
	if (eResult == ChunkFile_EndOfChunk)
	{
		eResult = ChunkFile_Ok;
	}

	//
	// Dispatch errors to the handler.
	//
	if ((eResult != ChunkFile_Ok) && (eResult != ChunkFile_EOF))
	{
		//HandleError("chunkname", eResult);
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszValue - 
//			pbBool - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChunkFile::ReadKeyValueBool(const char *pszValue, bool &bBool)
{
	int nValue = atoi(pszValue);

	if (nValue > 0)
	{
		bBool = true;
	}
	else
	{
		bBool = false;
	}

	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszValue - 
//			pfFloat - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChunkFile::ReadKeyValueFloat(const char *pszValue, float &flFloat)
{
	flFloat = (float)atof(pszValue);
	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszValue - 
//			pnInt - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChunkFile::ReadKeyValueInt(const char *pszValue, int &nInt)
{
	nInt = atoi(pszValue);
	return(true);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszKey - 
//			r - 
//			g - 
//			b - 
// Output : 
//-----------------------------------------------------------------------------
bool CChunkFile::ReadKeyValueColor(const char *pszValue, unsigned char &chRed, unsigned char &chGreen, unsigned char &chBlue)
{
	if (pszValue != NULL)
	{
		int r = 0;
		int g = 0;
		int b = 0;
		
		if (sscanf(pszValue, "%d %d %d", &r, &g, &b) == 3)
		{
			chRed = r;
			chGreen = g;
			chBlue = b;

			return(true);
		}
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszValue - 
//			pfPoint - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChunkFile::ReadKeyValuePoint(const char *pszValue, Vector &Point)
{
	if (pszValue != NULL)
	{
		return(sscanf(pszValue, "(%f %f %f)", &Point.x, &Point.y, &Point.z) == 3);
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszValue - 
//			pfVector - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChunkFile::ReadKeyValueVector2(const char *pszValue, Vector2D &vec)
{
	if (pszValue != NULL)
	{
		return ( sscanf( pszValue, "[%f %f]", &vec.x, &vec.y) == 2 );
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszValue - 
//			pfVector - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChunkFile::ReadKeyValueVector3(const char *pszValue, Vector &vec)
{
	if (pszValue != NULL)
	{
		return(sscanf(pszValue, "[%f %f %f]", &vec.x, &vec.y, &vec.z) == 3);
	}

	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszValue - 
//			pfVector - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChunkFile::ReadKeyValueVector4(const char *pszValue, Vector4D &vec)
{
	if( pszValue != NULL )
	{
		return(sscanf(pszValue, "[%f %f %f %f]", &vec[0], &vec[1], &vec[2], &vec[3]) == 4);
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszLine - 
// Output : 
//-----------------------------------------------------------------------------
ChunkFileResult_t CChunkFile::WriteKeyValue(const char *pszKey, const char *pszValue)
{
	if ((pszKey != NULL) && (pszValue != NULL))
	{
		char szTemp[MAX_KEYVALUE_LEN];
		Q_snprintf(szTemp, sizeof( szTemp ), "\"%s\" \"%s\"", pszKey, pszValue);
		return(WriteLine(szTemp));
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszKey - 
//			bValue - 
// Output : 
//-----------------------------------------------------------------------------
ChunkFileResult_t CChunkFile::WriteKeyValueBool(const char *pszKey, bool bValue)
{
	if (pszKey != NULL)
	{
		char szBuf[MAX_KEYVALUE_LEN];
		Q_snprintf(szBuf, sizeof( szBuf ), "\"%s\" \"%d\"", pszKey, (int)bValue);
		return(WriteLine(szBuf));
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszKey - 
//			nValue - 
// Output : 
//-----------------------------------------------------------------------------
ChunkFileResult_t CChunkFile::WriteKeyValueInt(const char *pszKey, int nValue)
{
	if (pszKey != NULL)
	{
		char szBuf[MAX_KEYVALUE_LEN];
		Q_snprintf(szBuf, sizeof( szBuf ), "\"%s\" \"%d\"", pszKey, nValue);
		return(WriteLine(szBuf));
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszKey - 
//			fValue - 
// Output : 
//-----------------------------------------------------------------------------
ChunkFileResult_t CChunkFile::WriteKeyValueFloat(const char *pszKey, float fValue)
{
	if (pszKey != NULL)
	{
		char szBuf[MAX_KEYVALUE_LEN];
		Q_snprintf(szBuf, sizeof( szBuf ), "\"%s\" \"%g\"", pszKey, (double)fValue);
		return(WriteLine(szBuf));
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszKey - 
//			r - 
//			g - 
//			b - 
// Output : 
//-----------------------------------------------------------------------------
ChunkFileResult_t CChunkFile::WriteKeyValueColor(const char *pszKey, unsigned char r, unsigned char g, unsigned char b)
{
	if (pszKey != NULL)
	{
		char szBuf[MAX_KEYVALUE_LEN];
		Q_snprintf(szBuf, sizeof( szBuf ), "\"%s\" \"%d %d %d\"", pszKey, (int)r, (int)g, (int)b);
		return(WriteLine(szBuf));
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszKey - 
//			fVector - 
// Output : 
//-----------------------------------------------------------------------------
ChunkFileResult_t CChunkFile::WriteKeyValuePoint(const char *pszKey, const Vector &Point)
{
	if (pszKey != NULL)
	{
		char szBuf[MAX_KEYVALUE_LEN];
		Q_snprintf(szBuf, sizeof( szBuf ), "\"%s\" \"(%g %g %g)\"", pszKey, (double)Point[0], (double)Point[1], (double)Point[2]);
		return(WriteLine(szBuf));
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ChunkFileResult_t CChunkFile::WriteKeyValueVector2(const char *pszKey, const Vector2D &vec)
{
	if (pszKey != NULL)
	{
		char szBuf[MAX_KEYVALUE_LEN];
		Q_snprintf( szBuf, sizeof( szBuf ), "\"%s\" \"[%g %g]\"", pszKey, (double)vec.x, (double)vec.y );
		return(WriteLine(szBuf));
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ChunkFileResult_t CChunkFile::WriteKeyValueVector3(const char *pszKey, const Vector &vec)
{
	if (pszKey != NULL)
	{
		char szBuf[MAX_KEYVALUE_LEN];
		Q_snprintf(szBuf, sizeof( szBuf ), "\"%s\" \"[%g %g %g]\"", pszKey, (double)vec.x, (double)vec.y, (double)vec.z);
		return(WriteLine(szBuf));
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszKey - 
//			fVector - 
// Output : 
//-----------------------------------------------------------------------------
ChunkFileResult_t CChunkFile::WriteKeyValueVector4(const char *pszKey, const Vector4D &vec)
{
	if (pszKey != NULL)
	{
		char szBuf[MAX_KEYVALUE_LEN];
		Q_snprintf(szBuf, sizeof( szBuf ), "\"%s\" \"[%g %g %g %g]\"", pszKey, (double)vec.x, (double)vec.y, (double)vec.z, (double)vec.w);
		return( WriteLine( szBuf ) );
	}

	return( ChunkFile_Ok );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszLine - 
// Output : 
//-----------------------------------------------------------------------------
ChunkFileResult_t CChunkFile::WriteLine(const char *pszLine)
{
	if (pszLine != NULL)
	{
		//
		// Write the indentation string.
		//
		if (m_nCurrentDepth > 0)
		{
			int nWritten = fwrite(m_szIndent, 1, m_nCurrentDepth, m_hFile);
			if (nWritten != m_nCurrentDepth)
			{
				return(ChunkFile_Fail);
			}
		}

		//
		// Write the string.
		//
		int nLen = strlen(pszLine);
		int nWritten = fwrite(pszLine, 1, nLen, m_hFile);
		if (nWritten != nLen)
		{
			return(ChunkFile_Fail);
		}

		//
		// Write the linefeed.
		//
		if (fwrite("\r\n", 1, 2, m_hFile) != 2)
		{
			return(ChunkFile_Fail);
		}
	}
	
	return(ChunkFile_Ok);
}

