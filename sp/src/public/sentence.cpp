//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#if !defined(_STATIC_LINKED) || defined(_SHARED_LIB)

#include <assert.h>
#include "commonmacros.h"
#include "basetypes.h"
#include "sentence.h"
#include "utlbuffer.h"
#include <stdlib.h>
#include "mathlib/vector.h"
#include "mathlib/mathlib.h"
#include <ctype.h>
#include "checksum_crc.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: converts an english string to unicode
//-----------------------------------------------------------------------------
int ConvertANSIToUnicode(const char *ansi, wchar_t *unicode, int unicodeBufferSize);

#if PHONEME_EDITOR
void CEmphasisSample::SetSelected( bool isSelected )
{
	selected = isSelected;
}
void CPhonemeTag::SetSelected( bool isSelected )
{
	m_bSelected = isSelected;
}
bool CPhonemeTag::GetSelected() const
{
	return m_bSelected;
}
void CPhonemeTag::SetStartAndEndBytes( unsigned int start, unsigned int end )
{
	m_uiStartByte = start;
	m_uiEndByte = end;
}
unsigned int CPhonemeTag::GetStartByte() const
{
	return m_uiStartByte;
}
unsigned int CPhonemeTag::GetEndByte() const
{
	return m_uiEndByte;
}
void CWordTag::SetSelected( bool isSelected )
{
	m_bSelected = isSelected;
}
bool CWordTag::GetSelected() const
{
	return m_bSelected;
}
void CWordTag::SetStartAndEndBytes( unsigned int start, unsigned int end )
{
	m_uiStartByte = start;
	m_uiEndByte = end;
}
unsigned int CWordTag::GetStartByte() const
{
	return m_uiStartByte;
}
unsigned int CWordTag::GetEndByte() const
{
	return m_uiEndByte;
}
#else
// xbox doesn't store this data
void CEmphasisSample::SetSelected( bool isSelected ) {}
void CPhonemeTag::SetSelected( bool isSelected ) {}
bool CPhonemeTag::GetSelected() const { return false; }
void CPhonemeTag::SetStartAndEndBytes( unsigned int start, unsigned int end ) {}
unsigned int CPhonemeTag::GetStartByte() const { return 0; }
unsigned int CPhonemeTag::GetEndByte() const { return 0; }
void CWordTag::SetSelected( bool isSelected ) {}
bool CWordTag::GetSelected() const { return false; }
void CWordTag::SetStartAndEndBytes( unsigned int start, unsigned int end ) {}
unsigned int CWordTag::GetStartByte() const { return 0; }
unsigned int CWordTag::GetEndByte() const { return 0; }
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWordTag::CWordTag( void )
{
	m_pszWord = NULL;

	SetStartAndEndBytes( 0, 0 );

	m_flStartTime = 0.0f;
	m_flEndTime = 0.0f;

	SetSelected( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : from - 
//-----------------------------------------------------------------------------
CWordTag::CWordTag( const CWordTag& from )
{
	m_pszWord = NULL;
	SetWord( from.m_pszWord );

	SetStartAndEndBytes( from.GetStartByte(), from.GetEndByte() );

	m_flStartTime = from.m_flStartTime;
	m_flEndTime = from.m_flEndTime;

	SetSelected( from.GetSelected() );

	for ( int p = 0; p < from.m_Phonemes.Size(); p++ )
	{
		CPhonemeTag *newPhoneme = new CPhonemeTag( *from.m_Phonemes[ p ] );
		m_Phonemes.AddToTail( newPhoneme );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *word - 
//-----------------------------------------------------------------------------
CWordTag::CWordTag( const char *word )
{
	SetStartAndEndBytes( 0, 0 );

	m_flStartTime = 0.0f;
	m_flEndTime = 0.0f;

	m_pszWord = NULL;

	SetSelected( false );

	SetWord( word );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWordTag::~CWordTag( void )
{
	delete[] m_pszWord;

	while ( m_Phonemes.Size() > 0 )
	{
		delete m_Phonemes[ 0 ];
		m_Phonemes.Remove( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tag - 
// Output : int
//-----------------------------------------------------------------------------
int CWordTag::IndexOfPhoneme( CPhonemeTag *tag )
{
	for ( int i = 0 ; i < m_Phonemes.Size(); i++ )
	{
		CPhonemeTag *p = m_Phonemes[ i ];
		if ( p == tag )
			return i;
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *word - 
//-----------------------------------------------------------------------------
void CWordTag::SetWord( const char *word )
{
	delete[] m_pszWord;
	m_pszWord = NULL;
	if ( !word || !word[ 0 ] )
		return;

	int len = strlen( word ) + 1;
	m_pszWord = new char[ len ];
	Assert( m_pszWord );
	Q_strncpy( m_pszWord, word, len );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CWordTag::GetWord() const
{
	return m_pszWord ? m_pszWord : "";
}


unsigned int CWordTag::ComputeDataCheckSum()
{
	int i;
	int c;
	CRC32_t crc;
	CRC32_Init( &crc );

	// Checksum the text
	if ( m_pszWord != NULL )
	{
		CRC32_ProcessBuffer( &crc, m_pszWord, Q_strlen( m_pszWord ) );
	}
	// Checksum phonemes
	c = m_Phonemes.Count();
	for ( i = 0; i < c; ++i )
	{
		CPhonemeTag *phoneme = m_Phonemes[ i ];
		unsigned int phonemeCheckSum = phoneme->ComputeDataCheckSum();
		CRC32_ProcessBuffer( &crc, &phonemeCheckSum, sizeof( unsigned int ) );
	}
	// Checksum timestamps
	CRC32_ProcessBuffer( &crc, &m_flStartTime, sizeof( float ) );
	CRC32_ProcessBuffer( &crc, &m_flEndTime, sizeof( float ) );

	CRC32_Final( &crc );

	return ( unsigned int )crc;
}

CBasePhonemeTag::CBasePhonemeTag()
{
	m_flStartTime = 0;
	m_flEndTime = 0;

	m_nPhonemeCode = 0;
}

CBasePhonemeTag::CBasePhonemeTag( const CBasePhonemeTag& from )
{
	memcpy( this, &from, sizeof(*this) );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPhonemeTag::CPhonemeTag( void )
{
	m_szPhoneme = NULL;

	SetStartAndEndBytes( 0, 0 );

	SetSelected( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : from - 
//-----------------------------------------------------------------------------
CPhonemeTag::CPhonemeTag( const CPhonemeTag& from ) :
	BaseClass( from )
{
	SetStartAndEndBytes( from.GetStartByte(), from.GetEndByte() );

	SetSelected( from.GetSelected() );

	m_szPhoneme = NULL;
	SetTag( from.GetTag() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *phoneme - 
//-----------------------------------------------------------------------------
CPhonemeTag::CPhonemeTag( const char *phoneme )
{
	SetStartAndEndBytes( 0, 0 );

	SetStartTime( 0.0f );
	SetEndTime( 0.0f );

	SetSelected( false );

	SetPhonemeCode( 0 );

	m_szPhoneme = NULL;
	SetTag( phoneme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPhonemeTag::~CPhonemeTag( void )
{
	delete[] m_szPhoneme;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *phoneme - 
//-----------------------------------------------------------------------------
void CPhonemeTag::SetTag( const char *phoneme )
{
	delete m_szPhoneme;
	m_szPhoneme = NULL;
	if ( !phoneme || !phoneme [ 0 ] )
		return;

	int len = Q_strlen( phoneme ) + 1;
	m_szPhoneme = new char[ len ];
	Assert( m_szPhoneme );
	Q_strncpy( m_szPhoneme, phoneme, len );
}

char const *CPhonemeTag::GetTag() const
{
	return m_szPhoneme ? m_szPhoneme : "";
}


unsigned int CPhonemeTag::ComputeDataCheckSum()
{
	CRC32_t crc;
	CRC32_Init( &crc );

	// Checksum the text
	CRC32_ProcessBuffer( &crc, m_szPhoneme, Q_strlen( m_szPhoneme ) );
	int phonemeCode = GetPhonemeCode();
	CRC32_ProcessBuffer( &crc, &phonemeCode, sizeof( int ) );

	// Checksum timestamps
	float startTime = GetStartTime();
	float endTime = GetEndTime();
	CRC32_ProcessBuffer( &crc, &startTime, sizeof( float ) );
	CRC32_ProcessBuffer( &crc, &endTime, sizeof( float ) );

	CRC32_Final( &crc );

	return ( unsigned int )crc;
}

//-----------------------------------------------------------------------------
// Purpose: Simple language to string and string to language lookup dictionary
//-----------------------------------------------------------------------------
#pragma pack(1)

struct CCLanguage
{
	int				type;
	char const		*name;
	unsigned char	r, g, b;  // For faceposer, indicator color for this language
};

static CCLanguage g_CCLanguageLookup[] =
{
	{ CC_ENGLISH,	"english",		0,		0,		0 },
	{ CC_FRENCH,	"french",		150,	0,		0 },
	{ CC_GERMAN,	"german",		0,		150,	0 },
	{ CC_ITALIAN,	"italian",		0,		150,	150 },
	{ CC_KOREAN,	"koreana",		150,	0,		150 },
	{ CC_SCHINESE,	"schinese",		150,	0,		150 },
	{ CC_SPANISH,	"spanish",		0,		0,		150 },
	{ CC_TCHINESE,	"tchinese",		150,	0,		150 },
	{ CC_JAPANESE,	"japanese",		250,	150,	0 },
	{ CC_RUSSIAN,	"russian",		0,		250,	150 },
	{ CC_THAI,		"thai",			0 ,		150,	250 },
	{ CC_PORTUGUESE,"portuguese",	0 ,		0,		150 },	
};

#pragma pack()

void CSentence::ColorForLanguage( int language, unsigned char& r, unsigned char& g, unsigned char& b )
{
	r = g = b = 0;

	if ( language < 0 || language >= CC_NUM_LANGUAGES )
	{
		return;
	}

	r = g_CCLanguageLookup[ language ].r;
	g = g_CCLanguageLookup[ language ].g;
	b = g_CCLanguageLookup[ language ].b;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : language - 
// Output : char const
//-----------------------------------------------------------------------------
char const *CSentence::NameForLanguage( int language )
{
	if ( language < 0 || language >= CC_NUM_LANGUAGES )
		return "unknown_language";

	CCLanguage *entry = &g_CCLanguageLookup[ language ];
	Assert( entry->type == language );
	return entry->name;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int CSentence::LanguageForName( char const *name )
{
	int l;
	for ( l = 0; l < CC_NUM_LANGUAGES; l++ )
	{
		CCLanguage *entry = &g_CCLanguageLookup[ l ];
		Assert( entry->type == l );
		if ( !stricmp( entry->name, name ) )
			return l;
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSentence::CSentence( void )
{
#if PHONEME_EDITOR
	m_nResetWordBase = 0;
	m_szText = 0;
	m_uCheckSum = 0;
#endif
	m_bShouldVoiceDuck = false;
	m_bStoreCheckSum = false;
	m_bIsValid = false;
	m_bIsCached = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSentence::~CSentence( void )
{
	Reset();
#if PHONEME_EDITOR
	delete[] m_szText;
#endif
}


void CSentence::ParsePlaintext( CUtlBuffer& buf )
{
	char token[ 4096 ];
	char text[ 4096 ];
	text[ 0 ] = 0;
	while ( 1 )
	{
		buf.GetString( token );
		if ( !stricmp( token, "}" ) )
			break;

		Q_strncat( text, token, sizeof( text ), COPY_ALL_CHARACTERS );
		Q_strncat( text, " ", sizeof( text ), COPY_ALL_CHARACTERS );
	}

	SetText( text );
}

void CSentence::ParseWords( CUtlBuffer& buf )
{
	char token[ 4096 ];
	char word[ 256 ];
	float start, end;

	while ( 1 )
	{
		buf.GetString( token );
		if ( !stricmp( token, "}" ) )
			break;

		if ( stricmp( token, "WORD" ) )
			break;

		buf.GetString( token );
		Q_strncpy( word, token, sizeof( word ) );

		buf.GetString( token );
		start = atof( token );
		buf.GetString( token );
		end = atof( token );

		CWordTag *wt = new CWordTag( word );
		assert( wt );
		wt->m_flStartTime = start;
		wt->m_flEndTime = end;

		AddWordTag( wt );

		buf.GetString( token );
		if ( stricmp( token, "{" ) )
			break;

		while ( 1 )
		{
			buf.GetString( token );
			if ( !stricmp( token, "}" ) )
				break;

			// Parse phoneme
			int code;
			char phonemename[ 256 ];
			float start, end;
			float volume;

			code = atoi( token );

			buf.GetString( token );
			Q_strncpy( phonemename, token, sizeof( phonemename ) );
			buf.GetString( token );
			start = atof( token );
			buf.GetString( token );
			end = atof( token );
			buf.GetString( token );
			volume = atof( token );

			CPhonemeTag *pt = new CPhonemeTag();
			assert( pt );
			pt->SetPhonemeCode( code );
			pt->SetTag( phonemename );
			pt->SetStartTime( start );
			pt->SetEndTime( end );

			AddPhonemeTag( wt, pt );
		}
	}
}

void CSentence::ParseEmphasis( CUtlBuffer& buf )
{
	char token[ 4096 ];
	while ( 1 )
	{
		buf.GetString( token );
		if ( !stricmp( token, "}" ) )
			break;

		char t[ 256 ];
		Q_strncpy( t, token, sizeof( t ) );
		buf.GetString( token );

		char value[ 256 ];
		Q_strncpy( value, token, sizeof( value ) );

		CEmphasisSample sample;
		sample.SetSelected( false );
		sample.time = atof( t );
		sample.value = atof( value );


		m_EmphasisSamples.AddToTail( sample );

	}
}

// This is obsolete, so it doesn't do anything with the data which is parsed.
void CSentence::ParseCloseCaption( CUtlBuffer& buf )
{
	char token[ 4096 ];
	while ( 1 )
	{
		// Format is 
		// language_name
		// {
		//   PHRASE char streamlength "streambytes" starttime endtime
		//   PHRASE unicode streamlength "streambytes" starttime endtime
		// }
		buf.GetString( token );
		if ( !stricmp( token, "}" ) )
			break;

		buf.GetString( token );
		if ( stricmp( token, "{" ) )
			break;

		buf.GetString( token );
		while ( 1 )
		{
			if ( !stricmp( token, "}" ) )
				break;

			if ( stricmp( token, "PHRASE" ) )
				break;

			char cc_type[32];
			char cc_stream[ 4096 ];
			int cc_length;

			memset( cc_stream, 0, sizeof( cc_stream ) );

			buf.GetString( token );
			Q_strncpy( cc_type, token, sizeof( cc_type ) );

			bool unicode = false;
			if ( !stricmp( cc_type, "unicode" ) )
			{
				unicode = true;
			}
			else if ( stricmp( cc_type, "char" ) )
			{
				Assert( 0 );
			}

			buf.GetString( token );
			cc_length = atoi( token );
			Assert( cc_length >= 0 && cc_length < sizeof( cc_stream ) );
			// Skip space
			buf.GetChar();
			buf.Get( cc_stream, cc_length );
			cc_stream[ cc_length ] = 0;
			
			// Skip space
			buf.GetChar();
			buf.GetString( token );
			buf.GetString( token );

			buf.GetString( token );
		}
	}
}

void CSentence::ParseOptions( CUtlBuffer& buf )
{
	char token[ 4096 ];
	while ( 1 )
	{
		buf.GetString( token );
		if ( !stricmp( token, "}" ) )
			break;

		if ( Q_strlen( token ) == 0 )
			break;

		char key[ 256 ];
		Q_strncpy( key, token, sizeof( key ) );
		char value[ 256 ];
		buf.GetString( token );
		Q_strncpy( value, token, sizeof( value ) );

		if ( !strcmpi( key, "voice_duck" ) )
		{
			SetVoiceDuck( atoi(value) ? true : false );
		}
		else if ( !strcmpi( key, "checksum" ) )
		{
			SetDataCheckSum( (unsigned int)atoi( value ) );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: VERSION 1.0 parser, need to implement new ones if 
//  file format changes!!!
// Input  : buf - 
//-----------------------------------------------------------------------------
void CSentence::ParseDataVersionOnePointZero( CUtlBuffer& buf )
{
	char token[ 4096 ];

	while ( 1 )
	{
		buf.GetString( token );
		if ( strlen( token ) <= 0 )
			break;

		char section[ 256 ];
		Q_strncpy( section, token, sizeof( section ) );

		buf.GetString( token );
		if ( stricmp( token, "{" ) )
			break;

		if ( !stricmp( section, "PLAINTEXT" ) )
		{
			ParsePlaintext( buf );
		}
		else if ( !stricmp( section, "WORDS" ) )
		{
			ParseWords( buf );
		}
		else if ( !stricmp( section, "EMPHASIS" ) )
		{
			ParseEmphasis( buf );
		}		
		else if ( !stricmp( section, "CLOSECAPTION" ) )
		{
			// NOTE:  CLOSECAPTION IS NO LONGER VALID
			// This just skips the section of data.
			ParseCloseCaption( buf );
		}
		else if ( !stricmp( section, "OPTIONS" ) )
		{
			ParseOptions( buf );
		}
	}
}

// This is a compressed save of just the data needed to drive phonemes in the engine (no word / sentence text, etc )
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//-----------------------------------------------------------------------------
void CSentence::CacheSaveToBuffer( CUtlBuffer& buf, int version )
{
	Assert( !buf.IsText() );
	Assert( m_bIsCached );

	int i;
	unsigned short pcount = GetRuntimePhonemeCount();

	// header
	if ( version == CACHED_SENTENCE_VERSION_ALIGNED )
	{
		buf.PutChar( version );
		buf.PutChar( 0 );
		buf.PutChar( 0 );
		buf.PutChar( 0 );
		buf.PutInt( pcount );
	}
	else
	{
		buf.PutChar( version );
		buf.PutShort( pcount );
	}

	// phoneme
	if ( version == CACHED_SENTENCE_VERSION_ALIGNED )
	{
		for ( i = 0; i < pcount; ++i )
		{
			const CBasePhonemeTag *phoneme = GetRuntimePhoneme( i );
			Assert( phoneme );
			buf.PutInt( phoneme->GetPhonemeCode() );
			buf.PutFloat( phoneme->GetStartTime() );
			buf.PutFloat( phoneme->GetEndTime() );
		}
	}
	else
	{
		for ( i = 0; i < pcount; ++i )
		{
			const CBasePhonemeTag *phoneme = GetRuntimePhoneme( i );
			Assert( phoneme );
			buf.PutShort( phoneme->GetPhonemeCode() );
			buf.PutFloat( phoneme->GetStartTime() );
			buf.PutFloat( phoneme->GetEndTime() );
		}
	}

	// emphasis samples and voice duck
	int c = m_EmphasisSamples.Count();
	Assert( c <= 32767 );

	if ( version == CACHED_SENTENCE_VERSION_ALIGNED )
	{
		buf.PutInt( c );
		for ( i = 0; i < c; i++ )
		{
			CEmphasisSample *sample = &m_EmphasisSamples[i];
			Assert( sample );
			buf.PutFloat( sample->time );
			buf.PutFloat( sample->value );
		}
		buf.PutInt( GetVoiceDuck() ? 1 : 0 );
	}
	else
	{
		buf.PutShort( c );
		for ( i = 0; i < c; i++ )
		{
			CEmphasisSample *sample = &m_EmphasisSamples[i];
			Assert( sample );
			buf.PutFloat( sample->time );
			short scaledValue = clamp( (short)( sample->value * 32767 ), (short)0, (short)32767 );
			buf.PutShort( scaledValue );
		}
		buf.PutChar( GetVoiceDuck() ? 1 : 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//-----------------------------------------------------------------------------
void CSentence::CacheRestoreFromBuffer( CUtlBuffer& buf )
{
	Assert( !buf.IsText() );

	Reset();

	m_bIsCached = true;

	// determine format
	int version = buf.GetChar();
	if ( version != CACHED_SENTENCE_VERSION && version != CACHED_SENTENCE_VERSION_ALIGNED )
	{
		// Uh oh, version changed...
		m_bIsValid = false;
		return;
	}

	unsigned short pcount;
	if ( version == CACHED_SENTENCE_VERSION_ALIGNED )
	{
		buf.GetChar();
		buf.GetChar();
		buf.GetChar();
		pcount = buf.GetInt();
	}
	else
	{
		pcount = (unsigned short)buf.GetShort();
	}

	// phonemes
	CPhonemeTag pt;
	int i;
	if ( version == CACHED_SENTENCE_VERSION_ALIGNED )
	{
		for ( i = 0; i < pcount; ++i )
		{
			int code = buf.GetInt();
			float st = buf.GetFloat();
			float et = buf.GetFloat();

			pt.SetPhonemeCode( code );
			pt.SetStartTime( st );
			pt.SetEndTime( et );
			AddRuntimePhoneme( &pt );
		}
	}
	else
	{
		for ( i = 0; i < pcount; ++i )
		{
			unsigned short code = buf.GetShort();
			float st = buf.GetFloat();
			float et = buf.GetFloat();

			pt.SetPhonemeCode( code );
			pt.SetStartTime( st );
			pt.SetEndTime( et );
			AddRuntimePhoneme( &pt );
		}
	}

	// emphasis samples and voice duck
	int c;
	if ( version == CACHED_SENTENCE_VERSION_ALIGNED )
	{
		c = buf.GetInt();
		for ( i = 0; i < c; i++ )
		{
			CEmphasisSample sample;
			sample.SetSelected( false );
			sample.time = buf.GetFloat();
			sample.value = buf.GetFloat();
			m_EmphasisSamples.AddToTail( sample );
		}
		SetVoiceDuck( buf.GetInt() == 0 ? false : true );
	}
	else
	{
		c = buf.GetShort();
		for ( i = 0; i < c; i++ )
		{
			CEmphasisSample sample;
			sample.SetSelected( false );
			sample.time = buf.GetFloat();
			sample.value = (float)buf.GetShort() / 32767.0f;
			m_EmphasisSamples.AddToTail( sample );
		}
		SetVoiceDuck( buf.GetChar() == 0 ? false : true );
	}

	m_bIsValid = true;
}

int CSentence::GetRuntimePhonemeCount() const
{
	return m_RunTimePhonemes.Count();
}

const CBasePhonemeTag *CSentence::GetRuntimePhoneme( int i ) const
{
	Assert( m_bIsCached );
	return m_RunTimePhonemes[ i ];
}

void CSentence::ClearRuntimePhonemes()
{
	while ( m_RunTimePhonemes.Count() > 0 )
	{
		CBasePhonemeTag *tag = m_RunTimePhonemes[ 0 ];
		delete tag;
		m_RunTimePhonemes.Remove( 0 );
	}
}

void CSentence::AddRuntimePhoneme( const CPhonemeTag *src )
{
	Assert( m_bIsCached );

	CBasePhonemeTag *tag = new CBasePhonemeTag();
	*tag = *src;

	m_RunTimePhonemes.AddToTail( tag );
}

void CSentence::MakeRuntimeOnly()
{
	m_bIsCached = true;
#if PHONEME_EDITOR
	delete m_szText;
	m_szText = NULL;

	int c = m_Words.Count();
	for ( int i = 0; i < c; ++i )
	{
		CWordTag *word = m_Words[ i ];
		Assert( word );
		int pcount = word->m_Phonemes.Count();
		for ( int j = 0; j < pcount; ++j )
		{
			CPhonemeTag *phoneme = word->m_Phonemes[ j ];
			assert( phoneme );

			AddRuntimePhoneme( phoneme );
		}
	}

	// Remove all existing words
	while ( m_Words.Count() > 0 )
	{
		CWordTag *word = m_Words[ 0 ];
		delete word;
		m_Words.Remove( 0 );
	}
#endif
	m_bIsValid = true;
}


void CSentence::SaveToBuffer( CUtlBuffer& buf )
{
#if PHONEME_EDITOR
	Assert( !m_bIsCached );

	int i, j;

	buf.Printf( "VERSION 1.0\n" );

	buf.Printf( "PLAINTEXT\n" );
	buf.Printf( "{\n" );
	buf.Printf( "%s\n", GetText() );
	buf.Printf( "}\n" );
	buf.Printf( "WORDS\n" );
	buf.Printf( "{\n" );
	for ( i = 0; i < m_Words.Size(); i++ )
	{
		CWordTag *word = m_Words[ i ];
		Assert( word );

		buf.Printf( "WORD %s %.3f %.3f\n", 
			word->GetWord(),
			word->m_flStartTime,
			word->m_flEndTime );

		buf.Printf( "{\n" );
		for ( j = 0; j < word->m_Phonemes.Size(); j++ )
		{
			CPhonemeTag *phoneme = word->m_Phonemes[ j ];
			Assert( phoneme );

			buf.Printf( "%i %s %.3f %.3f 1\n", 
				phoneme->GetPhonemeCode(), 
				phoneme->GetTag(),
				phoneme->GetStartTime(),
				phoneme->GetEndTime() );
		}

		buf.Printf( "}\n" );
	}
	buf.Printf( "}\n" );
	buf.Printf( "EMPHASIS\n" );
	buf.Printf( "{\n" );
	int c = m_EmphasisSamples.Count();
	for ( i = 0; i < c; i++ )
	{
		CEmphasisSample *sample = &m_EmphasisSamples[ i ];
		Assert( sample );

		buf.Printf( "%f %f\n", sample->time, sample->value );
	}

	buf.Printf( "}\n" );
	buf.Printf( "OPTIONS\n" );
	buf.Printf( "{\n" );
	buf.Printf( "voice_duck %d\n", GetVoiceDuck() ? 1 : 0 );
	if ( m_bStoreCheckSum )
	{
		buf.Printf( "checksum %d\n", m_uCheckSum );
	}
	buf.Printf( "}\n" );
#else
	Assert( 0 );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *data - 
//			size - 
//-----------------------------------------------------------------------------
void CSentence::InitFromDataChunk( void *data, int size )
{
	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	buf.EnsureCapacity( size );
	buf.Put( data, size );
	buf.SeekPut( CUtlBuffer::SEEK_HEAD, size );

	InitFromBuffer( buf );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//-----------------------------------------------------------------------------
void CSentence::InitFromBuffer( CUtlBuffer& buf )
{
	Assert( buf.IsText() );

	Reset();

	char token[ 4096 ];
	buf.GetString( token );

	if ( stricmp( token, "VERSION" ) )
		return;

	buf.GetString( token );
	if ( atof( token ) == 1.0f )
	{
		ParseDataVersionOnePointZero( buf );
		m_bIsValid = true;
	}
	else
	{
		assert( 0 );
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CSentence::GetWordBase( void )
{
#if PHONEME_EDITOR
	return m_nResetWordBase;
#else
	Assert( 0 );
	return 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSentence::ResetToBase( void )
{
#if PHONEME_EDITOR
	// Delete everything after m_nResetWordBase
	while ( m_Words.Size() > m_nResetWordBase )
	{
		delete m_Words[ m_Words.Size() - 1 ];
		m_Words.Remove( m_Words.Size() - 1 );
	}
#endif
	ClearRuntimePhonemes();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSentence::MarkNewPhraseBase( void )
{
#if PHONEME_EDITOR
	m_nResetWordBase = max( m_Words.Size(), 0 );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSentence::Reset( void )
{
#if PHONEME_EDITOR
	m_nResetWordBase = 0;

	while ( m_Words.Size() > 0 )
	{
		delete m_Words[ 0 ];
		m_Words.Remove( 0 );
	}
#endif
	m_EmphasisSamples.RemoveAll();

	ClearRuntimePhonemes();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tag - 
//-----------------------------------------------------------------------------
void CSentence::AddPhonemeTag( CWordTag *word, CPhonemeTag *tag )
{
	word->m_Phonemes.AddToTail( tag );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tag - 
//-----------------------------------------------------------------------------
void CSentence::AddWordTag( CWordTag *tag )
{
#if PHONEME_EDITOR
	m_Words.AddToTail( tag );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CSentence::CountPhonemes( void )
{
	int c = 0;
#if PHONEME_EDITOR
	for( int i = 0; i < m_Words.Size(); i++ )
	{
		CWordTag *word = m_Words[ i ];
		c += word->m_Phonemes.Size();
	}
#endif
	return c;
}

//-----------------------------------------------------------------------------
// Purpose: // For legacy loading, try to find a word that contains the time
// Input  : time - 
// Output : CWordTag
//-----------------------------------------------------------------------------
CWordTag *CSentence::EstimateBestWord( float time )
{
#if PHONEME_EDITOR
	CWordTag *bestWord = NULL;

	for( int i = 0; i < m_Words.Size(); i++ )
	{
		CWordTag *word = m_Words[ i ];
		if ( !word )
			continue;

		if ( word->m_flStartTime <= time && word->m_flEndTime >= time )
			return word;

		if ( time < word->m_flStartTime )
		{
			bestWord = word;
		}

		if ( time > word->m_flEndTime && bestWord )
			return bestWord;
	}

	// return best word if we found one
	if ( bestWord )
	{
		return bestWord;
	}

	// Return last word
	if ( m_Words.Size() >= 1 )
	{
		return m_Words[ m_Words.Size() - 1 ];
	}
#endif
	// Oh well
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *phoneme - 
// Output : CWordTag
//-----------------------------------------------------------------------------
CWordTag *CSentence::GetWordForPhoneme( CPhonemeTag *phoneme )
{
#if PHONEME_EDITOR
	for( int i = 0; i < m_Words.Size(); i++ )
	{
		CWordTag *word = m_Words[ i ];
		if ( !word )
			continue;

		for ( int j = 0 ; j < word->m_Phonemes.Size() ; j++ )
		{
			CPhonemeTag *p = word->m_Phonemes[ j ];
			if ( p == phoneme )
			{
				return word;
			}
		}

	}
#endif
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Assignment operator
// Input  : src - 
// Output : CSentence&
//-----------------------------------------------------------------------------
CSentence& CSentence::operator=( const CSentence& src )
{
	int i;

	// Clear current stuff
	Reset();

	int c;

#if PHONEME_EDITOR
	// Copy everything
	for ( i = 0 ; i < src.m_Words.Size(); i++ )
	{
		CWordTag *word = src.m_Words[ i ];

		CWordTag *newWord = new CWordTag( *word );

		AddWordTag( newWord );
	}

	SetText( src.GetText() );
	m_nResetWordBase = src.m_nResetWordBase;

	c = src.m_EmphasisSamples.Size();
	for ( i = 0; i < c; i++ )
	{
		CEmphasisSample s = src.m_EmphasisSamples[ i ];
		m_EmphasisSamples.AddToTail( s );
	}
#endif

	m_bIsCached = src.m_bIsCached;

	c = src.GetRuntimePhonemeCount();
	for ( i = 0; i < c; i++ )
	{
		Assert( m_bIsCached );

		const CBasePhonemeTag *tag = src.GetRuntimePhoneme( i );
		CPhonemeTag full;
		((CBasePhonemeTag &)(full)) = *tag;

		AddRuntimePhoneme( &full );
	}

	m_bShouldVoiceDuck = src.m_bShouldVoiceDuck;
#if PHONEME_EDITOR
	m_bStoreCheckSum = src.m_bStoreCheckSum;
	m_uCheckSum = src.m_uCheckSum;
#endif
	m_bIsValid = src.m_bIsValid;

	return (*this);
}

void CSentence::Append( float starttime, const CSentence& src )
{
#if PHONEME_EDITOR
	int i;
	// Combine
	for ( i = 0 ; i < src.m_Words.Size(); i++ )
	{
		CWordTag *word = src.m_Words[ i ];

		CWordTag *newWord = new CWordTag( *word );

		newWord->m_flStartTime += starttime;
		newWord->m_flEndTime += starttime;

		// Offset times
		int c = newWord->m_Phonemes.Count();
		for ( int i = 0; i < c; ++i )
		{
			CPhonemeTag *tag = newWord->m_Phonemes[ i ];
			tag->AddStartTime( starttime );
			tag->AddEndTime( starttime );
		}

		AddWordTag( newWord );
	}

	if ( src.GetText()[ 0 ] )
	{
		char fulltext[ 4096 ];
		if ( GetText()[ 0 ] )
		{
			Q_snprintf( fulltext, sizeof( fulltext ), "%s %s", GetText(), src.GetText() );
		}
		else
		{
			Q_strncpy( fulltext, src.GetText(), sizeof( fulltext ) );
		}
		SetText( fulltext );
	}

	int c = src.m_EmphasisSamples.Size();
	for ( i = 0; i < c; i++ )
	{
		CEmphasisSample s = src.m_EmphasisSamples[ i ];

		s.time += starttime;

		m_EmphasisSamples.AddToTail( s );
	}

	// Or in voice duck settings
	m_bShouldVoiceDuck |= src.m_bShouldVoiceDuck;
#else
	Assert( 0 );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *text - 
//-----------------------------------------------------------------------------
void CSentence::SetText( const char *text )
{
#if PHONEME_EDITOR
	delete[] m_szText;
	m_szText = NULL;

	if ( !text || !text[ 0 ] )
	{
		return;
	}

	int len = Q_strlen( text ) + 1;

	m_szText = new char[ len ];
	Assert( m_szText );
	Q_strncpy( m_szText, text, len );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CSentence::GetText( void ) const
{
#if PHONEME_EDITOR
	return m_szText ? m_szText : "";
#else
	return "";
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSentence::SetTextFromWords( void )
{
#if PHONEME_EDITOR
	char fulltext[ 1024 ];
	fulltext[ 0 ] = 0;
	for ( int i = 0 ; i < m_Words.Size(); i++ )
	{
		CWordTag *word = m_Words[ i ];

		Q_strncat( fulltext, word->GetWord(), sizeof( fulltext ), COPY_ALL_CHARACTERS );

		if ( i != m_Words.Size() )
		{
			Q_strncat( fulltext, " ", sizeof( fulltext ), COPY_ALL_CHARACTERS );
		}
	}

	SetText( fulltext );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSentence::Resort( void )
{
	int c = m_EmphasisSamples.Size();
	for ( int i = 0; i < c; i++ )
	{
		for ( int j = i + 1; j < c; j++ )
		{
			CEmphasisSample src = m_EmphasisSamples[ i ];
			CEmphasisSample dest = m_EmphasisSamples[ j ];

			if ( src.time > dest.time )
			{
				m_EmphasisSamples[ i ] = dest;
				m_EmphasisSamples[ j ] = src;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : number - 
// Output : CEmphasisSample
//-----------------------------------------------------------------------------
CEmphasisSample *CSentence::GetBoundedSample( int number, float endtime )
{
	// Search for two samples which span time f
	static CEmphasisSample nullstart;
	nullstart.time = 0.0f;
	nullstart.value = 0.5f;
	static CEmphasisSample nullend;
	nullend.time = endtime;
	nullend.value = 0.5f;
	
	if ( number < 0 )
	{
		return &nullstart;
	}
	else if ( number >= GetNumSamples() )
	{
		return &nullend;
	}
	
	return GetSample( number );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//			type - 
// Output : float
//-----------------------------------------------------------------------------
float CSentence::GetIntensity( float time, float endtime )
{
	float zeroValue = 0.5f;
	
	int c = GetNumSamples();
	
	if ( c <= 0 )
	{
		return zeroValue;
	}
	
	int i;
	for ( i = -1 ; i < c; i++ )
	{
		CEmphasisSample *s = GetBoundedSample( i, endtime );
		CEmphasisSample *n = GetBoundedSample( i + 1, endtime );
		if ( !s || !n )
			continue;

		if ( time >= s->time && time <= n->time )
		{
			break;
		}
	}

	int prev = i - 1;
	int start = i;
	int end = i + 1;
	int next = i + 2;

	prev = max( -1, prev );
	start = max( -1, start );
	end = min( end, GetNumSamples() );
	next = min( next, GetNumSamples() );

	CEmphasisSample *esPre = GetBoundedSample( prev, endtime );
	CEmphasisSample *esStart = GetBoundedSample( start, endtime );
	CEmphasisSample *esEnd = GetBoundedSample( end, endtime );
	CEmphasisSample *esNext = GetBoundedSample( next, endtime );

	float dt = esEnd->time - esStart->time;
	dt = clamp( dt, 0.01f, 1.0f );

	Vector vPre( esPre->time, esPre->value, 0 );
	Vector vStart( esStart->time, esStart->value, 0 );
	Vector vEnd( esEnd->time, esEnd->value, 0 );
	Vector vNext( esNext->time, esNext->value, 0 );

	float f2 = ( time - esStart->time ) / ( dt );
	f2 = clamp( f2, 0.0f, 1.0f );

	Vector vOut;
	Catmull_Rom_Spline( 
		vPre,
		vStart,
		vEnd,
		vNext,
		f2, 
		vOut );

	float retval = clamp( vOut.y, 0.0f, 1.0f );
	return retval;
}

int CSentence::GetNumSamples( void )
{
	return m_EmphasisSamples.Count();
}

CEmphasisSample *CSentence::GetSample( int index )
{
	if ( index < 0 || index >= GetNumSamples() )
		return NULL;

	return &m_EmphasisSamples[ index ];
}

void CSentence::GetEstimatedTimes( float& start, float &end )
{
#if PHONEME_EDITOR
	float beststart = 100000.0f;
	float bestend = -100000.0f;

	int c = m_Words.Count();
	if ( !c )
	{
		start = end = 0.0f;
		return;
	}

	for ( int i = 0; i< c; i++ )
	{
		CWordTag *w = m_Words[ i ];
		Assert( w );
		if ( w->m_flStartTime < beststart )
		{
			beststart = w->m_flStartTime;
		}
		if ( w->m_flEndTime > bestend )
		{
			bestend = w->m_flEndTime;
		}
	}

	if ( beststart == 100000.0f )
	{
		Assert( 0 );
		beststart = 0.0f;
	}
	if ( bestend == -100000.0f )
	{
		Assert( 0 );
		bestend = 1.0f;
	}
	start = beststart;
	end = bestend;
#endif
}

void CSentence::SetDataCheckSum( unsigned int chk )
{
#if PHONEME_EDITOR
	m_bStoreCheckSum = true;
	m_uCheckSum = chk;
#endif
}

unsigned int CSentence::ComputeDataCheckSum()
{
#if PHONEME_EDITOR
	int i;
	int c;
	CRC32_t crc;
	CRC32_Init( &crc );

	// Checksum the text
	CRC32_ProcessBuffer( &crc, GetText(), Q_strlen( GetText() ) );
	// Checsum words and phonemes
	c = m_Words.Count();
	for ( i = 0; i < c; ++i )
	{
		CWordTag *word = m_Words[ i ];
		unsigned int wordCheckSum = word->ComputeDataCheckSum();
		CRC32_ProcessBuffer( &crc, &wordCheckSum, sizeof( unsigned int ) );
	}

	// Checksum emphasis data
	c = m_EmphasisSamples.Count();
	for ( i = 0; i < c; ++i )
	{
		CRC32_ProcessBuffer( &crc, &m_EmphasisSamples[ i ].time, sizeof( float ) );
		CRC32_ProcessBuffer( &crc, &m_EmphasisSamples[ i ].value, sizeof( float ) );
	}

	CRC32_Final( &crc );

	return ( unsigned int )crc;
#else
	Assert( 0 );
	return 0;
#endif
}

unsigned int CSentence::GetDataCheckSum() const
{
#if PHONEME_EDITOR
	Assert( m_bStoreCheckSum );
	Assert( m_uCheckSum != 0 );
	return m_uCheckSum;
#else
	Assert( 0 );
	return 0;
#endif
}

#define STARTEND_TIMEGAP 0.1

int CSentence::CountWords( char const *str )
{
	if ( !str || !str[ 0 ] )
		return 0;

	int c = 1;
	
	unsigned char *p = (unsigned char *)str;
	while ( *p )
	{
		if ( *p <= 32 )
		{
			c++;

			while ( *p && *p <= 32 )
			{
				p++;
			}
		}

		if ( !(*p) )
			break;

		p++;
	}
	
	return c;
}


//-----------------------------------------------------------------------------
// Purpose: Static method
// Input  : in - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSentence::ShouldSplitWord( char in )
{
	if ( in <= 32 )
		return true;

	if ( in >= 128 )
		return true;

	if ( ispunct( in ) )
	{
		// don't split on apostrophe
		if ( in == '\'' )
			return false;
		return true;
	}

	return false;
}

void CSentence::CreateEventWordDistribution( char const *pszText, float flSentenceDuration )
{
	Assert( pszText );
	if ( !pszText )
		return;

	int wordCount = CountWords( pszText );
	if ( wordCount <= 0 )
		return;

	float wordLength = ( flSentenceDuration - 2 * STARTEND_TIMEGAP) / (float)wordCount;
	float wordStart = STARTEND_TIMEGAP;

	Reset();

	char word[ 256 ];
	unsigned char const *in = (unsigned char *)pszText;
	char *out = word;
	
	while ( *in )
	{
		if ( !ShouldSplitWord( *in ) )
		{
			*out++ = *in++;
		}
		else
		{
			*out = 0;

			// Skip over splitters
			while ( *in && ( ShouldSplitWord( *in ) ) )
			{
				in++;
			}
			
			if ( strlen( word ) > 0 )
			{
				CWordTag *w = new CWordTag();
				Assert( w );
				w->SetWord( word );
				w->m_flStartTime = wordStart;
				w->m_flEndTime = wordStart + wordLength;
				
				AddWordTag( w );
				
				wordStart += wordLength;
			}
			
			out = word;
		}
	}
	
	*out = 0;

	if ( strlen( word ) > 0 )
	{
		CWordTag *w = new CWordTag();
		Assert( w );
		w->SetWord( word );
		w->m_flStartTime = wordStart;
		w->m_flEndTime = wordStart + wordLength;
		
		AddWordTag( w );
		
		wordStart += wordLength;
	}
}


#endif // !_STATIC_LINKED || _SHARED_LIB