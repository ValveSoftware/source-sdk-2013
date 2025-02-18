//========= Copyright Valve Corporation, All rights reserved. =================//
//
// Read JSON-formatted data into KeyValues
//
//=============================================================================//

#ifndef KEYVALUESJSON_H
#define KEYVALUESJSON_H

#ifdef _WIN32
#pragma once
#endif

#include "KeyValues.h"

class CUtlBuffer;

/// JSON parser.  Use this class when you need to customize the parsing.
class KeyValuesJSONParser
{
public:
	KeyValuesJSONParser( const CUtlBuffer &buf );
	KeyValuesJSONParser( const char *pszText, int cbSize = -1 );
	~KeyValuesJSONParser();

	/// Parse the whole string.  If there's a problem, returns NULL and sets m_nLine,m_szErrMsg with more info. 
	KeyValues *ParseFile();

	/// Error message is returned here, if there is one.
	char m_szErrMsg[ 1024 ];

	/// Line number of current token during parsing, or of the error, of pasring fails
	int m_nLine;

private:

	bool ParseObject( KeyValues *pObject );
	bool ParseArray( KeyValues *pArray );
	bool ParseValue( KeyValues *pValue );
	void Init( const char *pszText, int cbSize );

	const char *m_cur;
	const char *m_end;

	enum
	{
		kToken_Err = -2, // An error has been discovered, don't parse anything else
		kToken_EOF = -1,
		kToken_String = 1,
		kToken_NumberInt = 2,
		kToken_NumberFloat = 3,
		kToken_True = 4,
		kToken_False = 5,
		kToken_Null = 6,
	};

	int m_eToken;
	CUtlVectorFixedGrowable<char,1024> m_vecTokenChars;

	void NextToken();
	void ParseNumberToken();
	void ParseStringToken();
	const char *GetTokenDebugText();
};

#ifdef _DEBUG
extern void TestKeyValuesJSONParser();
#endif

#endif // KEYVALUESJSON_H
