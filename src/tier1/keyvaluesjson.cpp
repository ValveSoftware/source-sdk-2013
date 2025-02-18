//========= Copyright Valve Corporation, All rights reserved. =================//
//
// Read JSON-formatted data into KeyValues
//
//=============================================================================//

#include "tier1/keyvaluesjson.h"
#include "tier1/utlbuffer.h"
#include "tier1/strtools.h"
#include <stdint.h> // INT32_MIN defn

KeyValuesJSONParser::KeyValuesJSONParser( const CUtlBuffer &buf )
{
	Init( (const char *)buf.Base(), buf.TellPut() );
}

KeyValuesJSONParser::KeyValuesJSONParser( const char *pszText, int cbSize )
{
	Init( pszText, cbSize >= 0 ? cbSize : V_strlen(pszText) );
}

KeyValuesJSONParser::~KeyValuesJSONParser() {}

void KeyValuesJSONParser::Init( const char *pszText, int cbSize )
{
	m_szErrMsg[0] = '\0';
	m_nLine = 1;
	m_cur = pszText;
	m_end = pszText+cbSize;

	m_eToken = kToken_Null;
	NextToken();
}

KeyValues *KeyValuesJSONParser::ParseFile()
{
	// A valid JSON object should contain a single object, surrounded by curly braces.
	if ( m_eToken == kToken_EOF )
	{
		V_sprintf_safe( m_szErrMsg, "Input contains no data" );
		return NULL;
	}
	if ( m_eToken == kToken_Err )
		return NULL;
	if ( m_eToken == '{' )
	{

		// Parse the the entire file as one big object
		KeyValues *pResult = new KeyValues("");
		if ( !ParseObject( pResult ) )
		{
			pResult->deleteThis();
			return NULL;
		}
		if ( m_eToken == kToken_EOF )
			return pResult;
		pResult->deleteThis();
	}
	V_sprintf_safe( m_szErrMsg, "%s not expected here.  A valid JSON document should be a single object, which begins with '{' and ends with '}'", GetTokenDebugText() );
	return NULL;
}

bool KeyValuesJSONParser::ParseObject( KeyValues *pObject )
{
	Assert( m_eToken == '{' );
	int nOpenDelimLine = m_nLine;
	NextToken();
	KeyValues *pLastChild = NULL;
	while ( m_eToken != '}' )
	{
		// Parse error?
		if ( m_eToken == kToken_Err )
			return false;
		if ( m_eToken == kToken_EOF )
		{
			// Actually report the error at the line of the unmatched delimiter.
			// There's no need to report the line number of the end of file, that is always
			// useless.
			m_nLine = nOpenDelimLine;
			V_strcpy_safe( m_szErrMsg, "End of input was reached and '{' was not matched by '}'" );
			return false;
		}

		// It must be a string, for the key name
		if ( m_eToken != kToken_String )
		{
			V_sprintf_safe( m_szErrMsg, "%s not expected here; expected string for key name or '}'", GetTokenDebugText() );
			return false;
		}

		KeyValues *pChildValue = new KeyValues( m_vecTokenChars.Base() );
		NextToken();

		// Expect and eat colon
		if ( m_eToken != ':' )
		{
			V_sprintf_safe( m_szErrMsg, "%s not expected here.  Missing ':'?", GetTokenDebugText() );
			pChildValue->deleteThis();
			return false;
		}
		NextToken();

		// Recursively parse the value
		if ( !ParseValue( pChildValue ) )
		{
			pChildValue->deleteThis();
			return false;
		}

		// Add to parent.
		pObject->AddSubkeyUsingKnownLastChild( pChildValue, pLastChild );
		pLastChild = pChildValue;

		// Eat the comma, if there is one.  If no comma,
		// then the other thing that could come next
		// is the closing brace to close the object
		// NOTE: We are allowing the extra comma after the last item
		if ( m_eToken == ',' )
		{
			NextToken();
		}
		else if ( m_eToken != '}' )
		{
			V_sprintf_safe( m_szErrMsg, "%s not expected here.  Missing ',' or '}'?", GetTokenDebugText() );
			return false;
		}
	}

	// Eat closing '}'
	NextToken();

	// Success
	return true;
}

bool KeyValuesJSONParser::ParseArray( KeyValues *pArray )
{
	Assert( m_eToken == '[' );
	int nOpenDelimLine = m_nLine;
	NextToken();
	KeyValues *pLastChild = NULL;
	int idx = 0;
	while ( m_eToken != ']' )
	{
		// Parse error?
		if ( m_eToken == kToken_Err )
			return false;
		if ( m_eToken == kToken_EOF )
		{
			// Actually report the error at the line of the unmatched delimiter.
			// There's no need to report the line number of the end of file, that is always
			// useless.
			m_nLine = nOpenDelimLine;
			V_strcpy_safe( m_szErrMsg, "End of input was reached and '[' was not matched by ']'" );
			return false;
		}

		// Set a dummy key name based on the index
		char szKeyName[ 32 ];
		V_sprintf_safe( szKeyName, "%d", idx );
		++idx;
		KeyValues *pChildValue = new KeyValues( szKeyName );

		// Recursively parse the value
		if ( !ParseValue( pChildValue ) )
		{
			pChildValue->deleteThis();
			return false;
		}

		// Add to parent.
		pArray->AddSubkeyUsingKnownLastChild( pChildValue, pLastChild );
		pLastChild = pChildValue;

		// Handle a colon here specially.  If one appears, the odds are they
		// are trying to put object-like data inside of an array
		if ( m_eToken == ':' )
		{
			V_sprintf_safe( m_szErrMsg, "':' not expected inside an array.  ('[]' used when '{}' was intended?)" );
			return false;
		}

		// Eat the comma, if there is one.  If no comma,
		// then the other thing that could come next
		// is the closing brace to close the object
		// NOTE: We are allowing the extra comma after the last item
		if ( m_eToken == ',' )
		{
			NextToken();
		}
		else if ( m_eToken != ']' )
		{
			V_sprintf_safe( m_szErrMsg, "%s not expected here.  Missing ',' or ']'?", GetTokenDebugText() );
			return false;
		}
	}

	// Eat closing ']'
	NextToken();

	// Success
	return true;
}

bool KeyValuesJSONParser::ParseValue( KeyValues *pValue )
{
	switch ( m_eToken )
	{
		case '{': return ParseObject( pValue );
		case '[': return ParseArray( pValue );
		case kToken_String:
			pValue->SetString( NULL, m_vecTokenChars.Base() );
			NextToken();
			return true;

		case kToken_NumberInt:
		{
			const char *pszNum = m_vecTokenChars.Base();

			// Negative?
			if ( *pszNum == '-' )
			{
				int64 val64 = V_atoi64( pszNum );
				if ( val64 < INT32_MIN )
				{
					// !KLUDGE! KeyValues cannot support this!
					V_sprintf_safe( m_szErrMsg, "%s is out of range for KeyValues, which doesn't support signed 64-bit numbers", pszNum );
					return false;
				}

				pValue->SetInt( NULL, (int)val64 );
			}
			else
			{
				uint64 val64 = V_atoui64( pszNum );
				if ( val64 > 0x7fffffffU )
				{
					pValue->SetUint64( NULL, val64 );
				}
				else
				{
					pValue->SetInt( NULL, (int)val64 );
				}
			}
			NextToken();
			return true;
		}

		case kToken_NumberFloat:
		{
			float f = V_atof( m_vecTokenChars.Base() );
			pValue->SetFloat( NULL, f );
			NextToken();
			return true;
		}

		case kToken_True:
			pValue->SetBool( NULL, true );
			NextToken();
			return true;

		case kToken_False:
			pValue->SetBool( NULL, false );
			NextToken();
			return true;

		case kToken_Null:
			pValue->SetPtr( NULL, NULL );
			NextToken();
			return true;

		case kToken_Err:
			return false;
	}

	V_sprintf_safe( m_szErrMsg, "%s not expected here; missing value?", GetTokenDebugText() );
	return false;
}

void KeyValuesJSONParser::NextToken()
{

	// Already in terminal state?
	if ( m_eToken < 0 )
		return;

	// Clear token
	m_vecTokenChars.SetCount(0);

	// Scan until we hit the end of input
	while ( m_cur < m_end )
	{

		// Next character?
		char c = *m_cur;
		switch (c)
		{
			// Whitespace?  Eat it and keep parsing
			case ' ':
			case '\t':
				++m_cur;
				break;

			// Newline?  Eat it and advance line number
			case '\n':
			case '\r':
				++m_nLine;
				++m_cur;

				// Eat \r\n or \n\r pair as a single character
				if ( m_cur < m_end && *m_cur == ( '\n' + '\r' - c ) )
					++m_cur;
				break;

			// Single-character JSON token?
			case ':':
			case '{':
			case '}':
			case '[':
			case ']':
			case ',':
				m_eToken = c;
				++m_cur;
				return;

			// String?
			case '\"':
			case '\'': // NOTE: We allow strings to be delimited by single quotes, which is not JSON compliant
				ParseStringToken();
				return;

			case '-':
			case '.':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				ParseNumberToken();
				return;

			// Literal "true"
			case 't':
				if ( m_cur + 4 <= m_end && m_cur[1] == 'r' && m_cur[2] == 'u' && m_cur[3] == 'e' )
				{
					m_cur += 4;
					m_eToken = kToken_True;
					return;
				}
				goto unexpected_char;

			// Literal "false"
			case 'f':
				if ( m_cur + 5 <= m_end && m_cur[1] == 'a' && m_cur[2] == 'l' && m_cur[3] == 's' && m_cur[4] == 'e' )
				{
					m_cur += 5;
					m_eToken = kToken_False;
					return;
				}
				goto unexpected_char;

			// Literal "null"
			case 'n':
				if ( m_cur + 4 <= m_end && m_cur[1] == 'u' && m_cur[2] == 'l' && m_cur[3] == 'l' )
				{
					m_cur += 4;
					m_eToken = kToken_Null;
					return;
				}
				goto unexpected_char;

			case '/':
				// C++-style comment?
				if ( m_cur < m_end && m_cur[1] == '/' )
				{
					m_cur += 2;
					while ( m_cur < m_end && *m_cur != '\n' && *m_cur != '\r' )
						++m_cur;
					// Leave newline as the next character, we'll handle it above
					break;
				}
				// | fall 
				// | through
				// V

			default:
			unexpected_char:
				if ( V_isprint(c) )
					V_sprintf_safe( m_szErrMsg, "Unexpected character 0x%02x ('%c')", (uint8)c, c );
				else
					V_sprintf_safe( m_szErrMsg, "Unexpected character 0x%02x", (uint8)c );
				m_eToken = kToken_Err;
				return;
		}
	}

	m_eToken = kToken_EOF;
}

void KeyValuesJSONParser::ParseNumberToken()
{
	// Clear token
	m_vecTokenChars.SetCount(0);

	// Eat leading minus sign
	if ( *m_cur	== '-' )
	{
		m_vecTokenChars.AddToTail( '-' );
		++m_cur;
	}

	if ( m_cur >= m_end )
	{
		V_strcpy_safe( m_szErrMsg, "Unexpected EOF while parsing number" );
		m_eToken = kToken_Err;
		return;
	}

	char c = *m_cur;
	m_vecTokenChars.AddToTail( c );
	bool bHasWholePart = false;
	switch ( c )
	{
		case '0':
			// Leading 0 cannot be followed by any more digits, as per JSON spec (and to make sure nobody tries to parse octal).
			++m_cur;
			bHasWholePart = true;
			break;

		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			bHasWholePart = true;
			++m_cur;

			// Accumulate digits until we hit a non-digit
			while ( m_cur < m_end && *m_cur >= '0' && *m_cur <= '9' )
				m_vecTokenChars.AddToTail( *(m_cur++) );
			break;

		case '.':
			// strict JSON doesn't allow a number that starts with a decimal point, but we do
			break;
	}

	// Assume this is integral, unless we hit a decimal point and/or exponent
	m_eToken = kToken_NumberInt;

	// Fractional portion?
	if ( m_cur < m_end && *m_cur == '.' )
	{
		m_eToken = kToken_NumberFloat;

		// Eat decimal point
		m_vecTokenChars.AddToTail( *(m_cur++) );

		// Accumulate digits until we hit a non-digit
		bool bHasFractionPart = false;
		while ( m_cur < m_end && *m_cur >= '0' && *m_cur <= '9' )
		{
			m_vecTokenChars.AddToTail( *(m_cur++) );
			bHasFractionPart = true;
		}

		// Make sure we aren't just a single '.'
		if ( !bHasWholePart && !bHasFractionPart )
		{
			m_vecTokenChars.AddToTail(0);
			V_sprintf_safe( m_szErrMsg, "Invalid number starting with '%s'", m_vecTokenChars.Base() );
			m_eToken = kToken_Err;
			return;
		}
	}

	// Exponent?
	if ( m_cur < m_end && ( *m_cur == 'e' || *m_cur == 'E' ) )
	{
		m_eToken = kToken_NumberFloat;

		// Eat 'e'
		m_vecTokenChars.AddToTail( *(m_cur++) );

		// Optional sign
		if ( m_cur < m_end && ( *m_cur == '-' || *m_cur == '+' ) )
			m_vecTokenChars.AddToTail( *(m_cur++) );

		// Accumulate digits until we hit a non-digit
		bool bHasExponentDigit = false;
		while ( m_cur < m_end && *m_cur >= '0' && *m_cur <= '9' )
		{
			m_vecTokenChars.AddToTail( *(m_cur++) );
			bHasExponentDigit = true;
		}
		if ( !bHasExponentDigit )
		{
			V_strcpy_safe( m_szErrMsg, "Bad exponent in floating point number" );
			m_eToken = kToken_Err;
			return;
		}
	}

	// OK, We have parsed a valid number.
	// Terminate token
	m_vecTokenChars.AddToTail( '\0' );

	// EOF?  That's OK for now, at this lexical parsing level.  We'll handle the error
	// at the higher parse level, when expecting a comma or closing delimiter
	if ( m_cur >= m_end )
		return;

	// Is the next thing a valid character?  This is the most common case.
	c = *m_cur;
	if ( V_isspace( c ) || c == ',' || c == '}' || c == ']' || c == '/' )
		return;

	// Handle these guys as "tokens", to provide a slightly more meaningful error message
	if ( c == '[' || c == '{' )
		return;

	// Anything else, treat the whole thing as an invalid numerical constant
	if ( V_isprint(c) )
		V_sprintf_safe( m_szErrMsg, "Number contains invalid character 0x%02x ('%c')", (uint8)c, c );
	else
		V_sprintf_safe( m_szErrMsg, "Number contains invalid character 0x%02x", (uint8)c );
	m_eToken = kToken_Err;
}

void KeyValuesJSONParser::ParseStringToken()
{
	char cDelim = *(m_cur++);

	while ( m_cur < m_end )
	{
		char c = *(m_cur++);
		if ( c == '\r' || c == '\n' )
		{
			V_sprintf_safe( m_szErrMsg, "Hit end of line before closing quote (%c)", c );
			m_eToken = kToken_Err;
			return;
		}
		if ( c == cDelim )
		{
			m_eToken = kToken_String;
			m_vecTokenChars.AddToTail( '\0' );
			return;
		}

		// Ordinary character?  Just append it
		if ( c != '\\' )
		{
			m_vecTokenChars.AddToTail( c );
			continue;
		}

		// Escaped character.
		// End of string?  We'll handle it above
		if ( m_cur >= m_end )
			continue;

		// Check table of allowed escape characters
		switch (c)
		{
			case '\\':
			case '/':
			case '\'':
			case '\"': m_vecTokenChars.AddToTail( c ); break;
			case 'b': m_vecTokenChars.AddToTail( '\b' ); break;
			case 'f': m_vecTokenChars.AddToTail( '\f' ); break;
			case 'n': m_vecTokenChars.AddToTail( '\n' ); break;
			case 'r': m_vecTokenChars.AddToTail( '\r' ); break;
			case 't': m_vecTokenChars.AddToTail( '\t' ); break;

			case 'u':
			{

				// Make sure are followed by exactly 4 hex digits
				if ( m_cur + 4 > m_end || !V_isxdigit( m_cur[0] ) || !V_isxdigit( m_cur[1] ) || !V_isxdigit( m_cur[2] ) || !V_isxdigit( m_cur[3] ) )
				{
					V_sprintf_safe( m_szErrMsg, "\\u must be followed by exactly 4 hex digits" );
					m_eToken = kToken_Err;
					return;
				}

				// Parse the codepoint
				uchar32 nCodePoint = 0;
				for ( int n = 0 ; n < 4 ; ++n )
				{
					nCodePoint <<= 4;
					char chHex = *(m_cur++);
					if ( chHex >= '0' && chHex <= '9' )
						nCodePoint += chHex - '0';
					else if ( chHex >= 'a' && chHex <= 'a' )
						nCodePoint += chHex + 0x0a - 'a';
					else if ( chHex >= 'A' && chHex <= 'A' )
						nCodePoint += chHex + 0x0a - 'A';
					else
						Assert( false ); // inconceivable, due to above
				}

				// Encode it in UTF-8
				char utf8Encode[8];
				int r = Q_UChar32ToUTF8( nCodePoint, utf8Encode );
				if ( r < 0 || r > 4 )
				{
					V_sprintf_safe( m_szErrMsg, "Invalid code point \\u%04x", nCodePoint );
					m_eToken = kToken_Err;
					return;
				}
				for ( int i = 0 ; i < r ; ++i )
					m_vecTokenChars.AddToTail( utf8Encode[i] );
			} break;

			default:
				if ( V_isprint(c) )
					V_sprintf_safe( m_szErrMsg, "Invalid escape character 0x%02x ('\\%c')", (uint8)c, c );
				else
					V_sprintf_safe( m_szErrMsg, "Invalid escape character 0x%02x", (uint8)c );
				m_eToken = kToken_Err;
				return;
		}
	}

	V_sprintf_safe( m_szErrMsg, "Hit end of input before closing quote (%c)", cDelim );
	m_eToken = kToken_Err;
}

const char *KeyValuesJSONParser::GetTokenDebugText()
{
	switch ( m_eToken )
	{
		case kToken_EOF: return "<EOF>";
		case kToken_String: return "<string>";
		case kToken_NumberInt:
		case kToken_NumberFloat: return "<number>";
		case kToken_True: return "'true'";
		case kToken_False: return "'false'";
		case kToken_Null: return "'null'";
		case '{': return "'{'";
		case '}': return "'}'";
		case '[': return "'['";
		case ']': return "']'";
		case ':': return "':'";
		case ',': return "','";
	}

	// We shouldn't ever need to ask for a debug string for the error token,
	// and anything else is an error
	Assert( false );
	return "<parse error>";
}

