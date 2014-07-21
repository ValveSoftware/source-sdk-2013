//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "phonemeextractor/PhonemeExtractor.h"
#include "ims_helper/ims_helper.h"

#include "tier0/dbg.h"
#include "sentence.h"
#include "PhonemeConverter.h"
#include "tier1/strtools.h"

#define TEXTLESS_WORDNAME	"[Textless]"

static IImsHelper *talkback = NULL;

//-----------------------------------------------------------------------------
// Purpose: Expose the interface
//-----------------------------------------------------------------------------
class CPhonemeExtractorLipSinc : public IPhonemeExtractor
{
public:
	virtual PE_APITYPE	GetAPIType() const
	{
		return SPEECH_API_LIPSINC;
	}

	// Used for menus, etc
	virtual char const *GetName() const
	{
		return "IMS (LipSinc)";
	}

	SR_RESULT Extract( 
		const char *wavfile,
		int numsamples,
		void (*pfnPrint)( const char *fmt, ... ),
		CSentence& inwords,
		CSentence& outwords );


	CPhonemeExtractorLipSinc( void );
	~CPhonemeExtractorLipSinc( void );

	enum
	{
		MAX_WORD_LENGTH = 128,
	};
private:


	class CAnalyzedWord
	{
	public:
		char		buffer[ MAX_WORD_LENGTH ];
		double		starttime;
		double		endtime;
	};

	class CAnalyzedPhoneme
	{
	public:
		char		phoneme[ 32 ];
		double		starttime;
		double		endtime;
	};

	bool InitLipSinc( void );
	void ShutdownLipSinc( void );

	void DescribeError( TALKBACK_ERR err );
	void Printf( char const *fmt, ... );

	bool CheckSoundFile( char const *filename );
	bool GetInitialized( void );
	void SetInitialized( bool init );

	void (*m_pfnPrint)( const char *fmt, ... );

	char const *ConstructInputSentence( CSentence& inwords );
	bool AttemptAnalysis( TALKBACK_ANALYSIS **ppAnalysis, char const *wavfile, CSentence& inwords );

	char const *ApplyTBWordRules( char const *word );

	void ProcessWords( TALKBACK_ANALYSIS *analysis, CSentence& inwords, CSentence& outwords );
	void ProcessWordsTextless( TALKBACK_ANALYSIS *analysis, CSentence& outwords );

	int GetPhonemeIndexAtWord( TALKBACK_ANALYSIS *analysis, double time, bool checkstart );

	int GetPhonemeIndexAtWordStart( TALKBACK_ANALYSIS *analysis, double starttime );
	int GetPhonemeIndexAtWordEnd( TALKBACK_ANALYSIS *analysis, double endtime );

	CAnalyzedWord *GetAnalyzedWord( TALKBACK_ANALYSIS *analysis, int index );
	CAnalyzedPhoneme *GetAnalyzedPhoneme( TALKBACK_ANALYSIS *analysis, int index );

	int ComputeByteFromTime( float time );

	bool m_bInitialized;

	float	m_flSampleCount;
	float	m_flDuration;

	float	m_flSamplesPerSecond;

	int		m_nBytesPerSample;

	HMODULE m_hHelper;
};

CPhonemeExtractorLipSinc::CPhonemeExtractorLipSinc( void )
{
	m_hHelper = (HMODULE)0;
	m_pfnPrint = NULL;

	m_bInitialized = false;
	
	m_flSampleCount = 0.0f;
	m_flDuration = 0.0f;

	m_flSamplesPerSecond = 0.0f;

	m_nBytesPerSample = 0;
}

CPhonemeExtractorLipSinc::~CPhonemeExtractorLipSinc( void )
{
	if ( GetInitialized() )
	{
		ShutdownLipSinc();
	}
}

bool CPhonemeExtractorLipSinc::GetInitialized( void )
{
	return m_bInitialized;
}

void CPhonemeExtractorLipSinc::SetInitialized( bool init )
{
	m_bInitialized = init;
}

int CPhonemeExtractorLipSinc::ComputeByteFromTime( float time )
{
	if ( !m_flDuration )
		return 0;

	float frac = time / m_flDuration;

	float sampleNumber = frac * m_flSampleCount;

	int bytenumber = sampleNumber * m_nBytesPerSample;

	return bytenumber;
}

void CPhonemeExtractorLipSinc::DescribeError( TALKBACK_ERR err )
{
	Assert( m_pfnPrint );

	// Get the error description.
	char errorDesc[256] = "";
	if ( err != TALKBACK_NOERR )
	{
		talkback->TalkBackGetErrorString( err, sizeof(errorDesc), errorDesc );
	}
	
	// Report or log the error...
	(*m_pfnPrint)( "LIPSINC ERROR:  %s\n", errorDesc );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fmt - 
//			.. - 
//-----------------------------------------------------------------------------
void CPhonemeExtractorLipSinc::Printf( char const *fmt, ... )
{
	Assert( m_pfnPrint );

	char string[ 4096 ];

	va_list argptr;
	va_start( argptr, fmt );
	vsprintf( string, fmt, argptr );
	va_end( argptr );

	(*m_pfnPrint)( "%s", string );
}

bool CPhonemeExtractorLipSinc::CheckSoundFile( char const *filename )
{
	TALKBACK_SOUND_FILE_METRICS fm;
	memset( &fm, 0, sizeof( fm ) );
	fm.m_size = sizeof( fm );

	TALKBACK_ERR err = talkback->TalkBackGetSoundFileMetrics( filename, &fm );
	if ( err != TALKBACK_NOERR )
	{
		DescribeError( err );
		return false;
	}

	if ( fm.m_canBeAnalyzed )
	{
		Printf( "%s:  %.2f s, rate %i, bits %i, channels %i\n",
			filename,
			fm.m_duration,
			fm.m_sampleRate,
			fm.m_bitsPerSample,
			fm.m_channelCount );
	}

	m_flDuration = fm.m_duration;
	if ( m_flDuration > 0 )
	{
		m_flSamplesPerSecond = m_flSampleCount / m_flDuration;
	}
	else
	{
		m_flSamplesPerSecond = 0.0f;
	}

	m_nBytesPerSample = ( fm.m_bitsPerSample >> 3 );

	m_flSampleCount /= m_nBytesPerSample;

	m_nBytesPerSample /= fm.m_channelCount;

	return fm.m_canBeAnalyzed ? true : false;
}

typedef IImsHelper *(*pfnImsHelper)(void);

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPhonemeExtractorLipSinc::InitLipSinc( void )
{
	if ( GetInitialized() )
	{
		return true;
	}

	m_hHelper = LoadLibrary( "ims_helper.dll" );
	if ( !m_hHelper )
	{
		return false;
	}
	
	pfnImsHelper factory = (pfnImsHelper)::GetProcAddress( m_hHelper, "GetImsHelper" );
	if ( !factory )
	{
		FreeLibrary( m_hHelper );
		return false;
	}

	talkback = reinterpret_cast< IImsHelper * >( (*factory)() );
	if ( !talkback )
	{
		FreeLibrary( m_hHelper );
		return false;
	}

	char szExeName[ MAX_PATH ];
	szExeName[0] = 0;
	GetModuleFileName( (HMODULE)0, szExeName, sizeof( szExeName ) );

	char szBaseDir[ MAX_PATH ];
	Q_strncpy( szBaseDir, szExeName, sizeof( szBaseDir ) );

	Q_StripLastDir( szBaseDir, sizeof( szBaseDir ) );
	Q_StripTrailingSlash( szBaseDir );
	Q_strlower( szBaseDir );

	char coreDataDir[ 512 ];
	Q_snprintf( coreDataDir, sizeof( coreDataDir ), "%s\\lipsinc_data\\",
		szBaseDir );
	Q_FixSlashes( coreDataDir );

	char szCheck[ 512 ];
	Q_snprintf( szCheck, sizeof( szCheck ), "%sDtC6dal.dat", coreDataDir );
	struct __stat64 buf;

	if ( _stat64( szCheck, &buf ) != 0 )
	{
		Q_snprintf( coreDataDir, sizeof( coreDataDir ), "%s\\bin\\lipsinc_data\\",
			szBaseDir );
		Q_FixSlashes( coreDataDir );
		Q_snprintf( szCheck, sizeof( szCheck ), "%sDtC6dal.dat", coreDataDir );

		if ( _stat64( szCheck, &buf ) != 0 )
		{
			Error( "Unable to find talkback data files in %s.", coreDataDir );
		}
	}

	TALKBACK_ERR err;
	
	err = talkback->TalkBackStartupLibrary( coreDataDir );
	if ( err != TALKBACK_NOERR )
	{
		DescribeError( err );
		FreeLibrary( m_hHelper );
		return false;
	}

	long verMajor = 0;
	long verMinor = 0;
	long verRevision = 0;
	
	err = talkback->TalkBackGetVersion(
		&verMajor, 
		&verMinor, 
		&verRevision);
	if ( err != TALKBACK_NOERR )
	{
		DescribeError( err );
		FreeLibrary( m_hHelper );
		return false;
	}

	Printf( "Lipsinc TalkBack Version %i.%i.%i\n", verMajor, verMinor, verRevision );

	m_bInitialized = true;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhonemeExtractorLipSinc::ShutdownLipSinc( void )
{
	// HACK HACK:  This seems to crash on exit sometimes
	__try
	{
		talkback->TalkBackShutdownLibrary();

		FreeLibrary( m_hHelper );
	}
	__except(EXCEPTION_EXECUTE_HANDLER )
	{
		OutputDebugString( "----> Crash shutting down TALKBACK sdk, exception caught and ignored\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : inwords - 
// Output : char const
//-----------------------------------------------------------------------------
char const *CPhonemeExtractorLipSinc::ConstructInputSentence( CSentence& inwords )
{
	static char sentence[ 16384 ];

	sentence[ 0 ] = 0;

	int last = inwords.m_Words.Size() - 1;

	for ( int i = 0 ; i <= last; i++ )
	{
		CWordTag *w = inwords.m_Words[ i ];

		strcat( sentence, w->GetWord() );
		if ( i != last )
		{
			strcat( sentence, " " );
		}
	}

	if ( inwords.m_Words.Count() == 1 && 
		!Q_strnicmp( inwords.GetText(), TEXTLESS_WORDNAME, Q_strlen( TEXTLESS_WORDNAME ) ) )
	{
		sentence[ 0 ] = 0;
	}

	return sentence;
}

bool CPhonemeExtractorLipSinc::AttemptAnalysis( TALKBACK_ANALYSIS **ppAnalysis, char const *wavfile, CSentence& inwords )
{
	*ppAnalysis = NULL;

	TALKBACK_ANALYSIS_SETTINGS settings;
	memset( &settings, 0, sizeof( settings ) );

   // Set this field to sizeof(TALKBACK_ANALYSIS_SETTINGS) before using the
    // structure.
	settings.fSize = sizeof( TALKBACK_ANALYSIS_SETTINGS );
	

    // Default value: 30 (frames per second).
    settings.fFrameRate = 100;
    // Set this to 1 to optimize for flipbook output, 0 to do analysis normally.
    //
    // Default value: 0 (normal analysis).
    settings.fOptimizeForFlipbook = 0;
    // Set this to -1 to seed the random number generator with the current time.
    // Any other number will be used directly for the random number seed, which
    // is useful if you want repeatable speech gestures. This value does not
    // influence lip-synching at all.
    //
    // Default value: -1 (use current time).
    settings.fRandomSeed = -1;
    // Path to the configuration (.INI) file with phoneme-to-speech-target
    // mapping. Set this to NULL to use the default mapping.
    //
    // Default value: NULL (use default mapping).
    settings.fConfigFile = NULL;

	char const *text = ConstructInputSentence( inwords );

	Printf( "Analyzing: \"%s\"\n", text[ 0 ] ? text : TEXTLESS_WORDNAME );

	TALKBACK_ERR err = talkback->TalkBackGetAnalysis( 
		ppAnalysis,
		wavfile,
		text,
		&settings );

	if ( err != TALKBACK_NOERR )
	{
		DescribeError( err );
		return false;
	}

	Printf( "Analysis successful...\n" );

	return true;
}

typedef struct
{
	TALKBACK_PHONEME phoneme;
	char const		*string;
} TBPHONEMES_t;

static TBPHONEMES_t g_TBPhonemeList[]=
{
	{ TALKBACK_PHONEME_IY, "iy" },
	{ TALKBACK_PHONEME_IH, "ih" },
	{ TALKBACK_PHONEME_EH, "eh" },
	{ TALKBACK_PHONEME_EY, "ey" },
	{ TALKBACK_PHONEME_AE, "ae" },
	{ TALKBACK_PHONEME_AA, "aa" },
	{ TALKBACK_PHONEME_AW, "aw" },
	{ TALKBACK_PHONEME_AY, "ay" },
	{ TALKBACK_PHONEME_AH, "ah" },
	{ TALKBACK_PHONEME_AO, "ao" },
	{ TALKBACK_PHONEME_OY, "oy" },
	{ TALKBACK_PHONEME_OW, "ow" },
	{ TALKBACK_PHONEME_UH, "uh" },
	{ TALKBACK_PHONEME_UW, "uw" },
	{ TALKBACK_PHONEME_ER, "er" },
	{ TALKBACK_PHONEME_AX, "ax" },
	{ TALKBACK_PHONEME_S, "s" },
	{ TALKBACK_PHONEME_SH, "sh" },
	{ TALKBACK_PHONEME_Z, "z" },
	{ TALKBACK_PHONEME_ZH, "zh" },
	{ TALKBACK_PHONEME_F, "f" },
	{ TALKBACK_PHONEME_TH, "th" },
	{ TALKBACK_PHONEME_V, "v" },
	{ TALKBACK_PHONEME_DH, "dh" },
	{ TALKBACK_PHONEME_M, "m" },
	{ TALKBACK_PHONEME_N, "n" },
	{ TALKBACK_PHONEME_NG, "ng" },
	{ TALKBACK_PHONEME_L, "l" },
	{ TALKBACK_PHONEME_R, "r" },
	{ TALKBACK_PHONEME_W, "w" },
	{ TALKBACK_PHONEME_Y, "y" },
	{ TALKBACK_PHONEME_HH, "hh" },
	{ TALKBACK_PHONEME_B, "b" },
	{ TALKBACK_PHONEME_D, "d" },
	{ TALKBACK_PHONEME_JH, "jh" },
	{ TALKBACK_PHONEME_G, "g" },
	{ TALKBACK_PHONEME_P, "p" },
	{ TALKBACK_PHONEME_T, "t" },
	{ TALKBACK_PHONEME_K, "k" },
	{ TALKBACK_PHONEME_CH, "ch" },
	{ TALKBACK_PHONEME_SIL, "<sil>" },
	{ -1, NULL }
};

char const *TBPhonemeToString( TALKBACK_PHONEME phoneme )
{
	if ( phoneme < TALKBACK_PHONEME_FIRST || phoneme > TALKBACK_PHONEME_LAST )
	{
		return "Bogus";
	}

	TBPHONEMES_t *item = &g_TBPhonemeList[ phoneme ];
	return item->string;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *analysis - 
//			time - 
//			start - 
// Output : int
//-----------------------------------------------------------------------------
int CPhonemeExtractorLipSinc::GetPhonemeIndexAtWord( TALKBACK_ANALYSIS *analysis, double time, bool start )
{
	long count;

	TALKBACK_ERR err = talkback->TalkBackGetNumPhonemes( analysis, &count );
	if ( err != TALKBACK_NOERR )
	{
		DescribeError( err );
		return -1;
	}

	if ( count <= 0L )
		return -1;

	// Bogus
	if ( count >= 100000L )
		return -1;

	for ( int i = 0; i < (int)count; i++ )
	{
		TALKBACK_PHONEME tbPhoneme = TALKBACK_PHONEME_INVALID;
		err = talkback->TalkBackGetPhonemeEnum( analysis, i, &tbPhoneme );
		if ( err != TALKBACK_NOERR )
		{
			DescribeError( err );
			continue;
		}

		double t;

		if ( start )
		{
			err = talkback->TalkBackGetPhonemeStartTime( analysis, i, &t );
		}
		else
		{
			err = talkback->TalkBackGetPhonemeEndTime( analysis, i, &t );
		}

		if ( err != TALKBACK_NOERR )
		{
			DescribeError( err );
			continue;
		}

		if ( t == time )
		{
			return i;
		}
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *analysis - 
//			starttime - 
// Output : int
//-----------------------------------------------------------------------------
int CPhonemeExtractorLipSinc::GetPhonemeIndexAtWordStart( TALKBACK_ANALYSIS *analysis, double starttime )
{
	return GetPhonemeIndexAtWord( analysis, starttime, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *analysis - 
//			endtime - 
// Output : int
//-----------------------------------------------------------------------------
int CPhonemeExtractorLipSinc::GetPhonemeIndexAtWordEnd( TALKBACK_ANALYSIS *analysis, double endtime )
{
	return GetPhonemeIndexAtWord( analysis, endtime, false );
}

CPhonemeExtractorLipSinc::CAnalyzedPhoneme *CPhonemeExtractorLipSinc::GetAnalyzedPhoneme( TALKBACK_ANALYSIS *analysis, int index )
{
	static CAnalyzedPhoneme p;

	memset( &p, 0, sizeof( p ) );

	TALKBACK_PHONEME tb;

	TALKBACK_ERR err = talkback->TalkBackGetPhonemeEnum( analysis, index, &tb );
	if ( err != TALKBACK_NOERR )
	{
		DescribeError( err );
		return NULL;
	}

	strcpy( p.phoneme, TBPhonemeToString( tb ) );

	err = talkback->TalkBackGetPhonemeStartTime( analysis, index, &p.starttime );
	if ( err != TALKBACK_NOERR )
	{
		DescribeError( err );
		return NULL;
	}
	err = talkback->TalkBackGetPhonemeEndTime( analysis, index, &p.endtime );
	if ( err != TALKBACK_NOERR )
	{
		DescribeError( err );
		return NULL;
	}

	return &p;
}

CPhonemeExtractorLipSinc::CAnalyzedWord *CPhonemeExtractorLipSinc::GetAnalyzedWord( TALKBACK_ANALYSIS *analysis, int index )
{
	static CAnalyzedWord w;

	memset( &w, 0, sizeof( w ) );

	long chars = sizeof( w.buffer );

	TALKBACK_ERR err = talkback->TalkBackGetWord( analysis, index, chars, w.buffer );
	if ( err != TALKBACK_NOERR )
	{
		DescribeError( err );
		return NULL;
	}

	err = talkback->TalkBackGetWordStartTime( analysis, index, &w.starttime );
	if ( err != TALKBACK_NOERR )
	{
		DescribeError( err );
		return NULL;
	}
	err = talkback->TalkBackGetWordEndTime( analysis, index, &w.endtime );
	if ( err != TALKBACK_NOERR )
	{
		DescribeError( err );
		return NULL;
	}

	return &w;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *w1 - 
//			*w2 - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool FuzzyWordMatch( char const *w1, char const *w2 )
{
	int len1 = strlen( w1 );
	int len2 = strlen( w2 );

	int minlen = min( len1, len2 );

	// Found a match
	if ( !strnicmp( w1, w2, minlen ) )
		return true;

	int letterdiff = abs( len1 - len2 );
	// More than three letters different, don't bother
	if ( letterdiff > 5 )
		return false;

	// Compute a "delta"
	char *p1 = (char *)w1;
	char *p2 = (char *)w2;

	CUtlVector <char> word1;
	CUtlVector <char> word2;

	while ( *p1 )
	{
		if ( V_isalpha( *p1 ) )
		{
			word1.AddToTail( *p1 );
		}
		p1++;
	}

	while ( *p2 )
	{
		if ( V_isalpha( *p2 ) )
		{
			word2.AddToTail( *p2 );
		}
		p2++;
	}

	int i;
	for ( i = 0; i < word1.Size(); i++ )
	{
		char c = word1[ i ];

		// See if c is in word 2, if so subtract it out
		int idx = word2.Find( c );

		if ( idx != word2.InvalidIndex() )
		{
			word2.Remove( idx );
		}
	}

	if ( word2.Size() <= letterdiff )
		return true;

	word2.RemoveAll();

	while ( *p2 )
	{
		if ( V_isalpha( *p2 ) )
		{
			word2.AddToTail( *p2 );
		}
		p2++;
	}

	for ( i = 0; i < word2.Size(); i++ )
	{
		char c = word2[ i ];

		// See if c is in word 2, if so subtract it out
		int idx = word1.Find( c );

		if ( idx != word1.InvalidIndex() )
		{
			word1.Remove( idx );
		}
	}

	if ( word1.Size() <= letterdiff )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: For foreign language stuff, if inwords is empty, process anyway...
// Input  : *analysis - 
//			outwords - 
//-----------------------------------------------------------------------------
void CPhonemeExtractorLipSinc::ProcessWordsTextless( TALKBACK_ANALYSIS *analysis, CSentence& outwords )
{
	long count;

	TALKBACK_ERR err = talkback->TalkBackGetNumPhonemes( analysis, &count );
	if ( err != TALKBACK_NOERR )
	{
		DescribeError( err );
		return;
	}

	CWordTag *newWord = new CWordTag;

	newWord->SetWord( TEXTLESS_WORDNAME );

	float starttime = 0.0f;
	float endtime = 1.0f;


	for ( int i = 0; i < count; ++i )
	{
		// Get phoneme and timing info
		CAnalyzedPhoneme *ph = GetAnalyzedPhoneme( analysis, i );
		if ( !ph )
			continue;

		CPhonemeTag *ptag = new CPhonemeTag;

		if ( i == 0 || ( ph->starttime < starttime ) )
		{
			starttime = ph->starttime;
		}

		if ( i == 0 || ( ph->endtime > endtime ) )
		{
			endtime = ph->endtime;
		}

		ptag->SetStartTime( ph->starttime );
		ptag->SetEndTime( ph->endtime );

		ptag->m_uiStartByte = ComputeByteFromTime( ph->starttime );
		ptag->m_uiEndByte = ComputeByteFromTime( ph->endtime );

		ptag->SetTag( ph->phoneme );
		ptag->SetPhonemeCode( TextToPhoneme( ptag->GetTag() ) );

		newWord->m_Phonemes.AddToTail( ptag );
	}

	newWord->m_flStartTime = starttime;
	newWord->m_flEndTime = endtime;

	newWord->m_uiStartByte = ComputeByteFromTime( starttime );
	newWord->m_uiEndByte = ComputeByteFromTime( endtime );

	outwords.Reset();
	outwords.AddWordTag( newWord );
	outwords.SetTextFromWords();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *analysis - 
//			inwords - 
//			outwords - 
//-----------------------------------------------------------------------------
void CPhonemeExtractorLipSinc::ProcessWords( TALKBACK_ANALYSIS *analysis, CSentence& inwords, CSentence& outwords )
{
	long count;

	TALKBACK_ERR err = talkback->TalkBackGetNumWords( analysis, &count );
	if ( err != TALKBACK_NOERR )
	{
		DescribeError( err );
		return;
	}

	if ( count <= 0L )
	{
		if ( inwords.m_Words.Count() == 0 || 
			!Q_strnicmp( inwords.GetText(), TEXTLESS_WORDNAME, Q_strlen( TEXTLESS_WORDNAME ) ) )
		{
			ProcessWordsTextless( analysis, outwords );
		}
		return;
	}

	// Bogus
	if ( count >= 100000L )
		return;

	int inwordpos = 0;
	int awordpos = 0;

	outwords.Reset();

	char previous[ 256 ];
	previous[ 0 ] = 0;

	while ( inwordpos < inwords.m_Words.Size() )
	{
		CWordTag *in = inwords.m_Words[ inwordpos ];

		if ( awordpos >= count )
		{
			// Just copy the rest over without phonemes
			CWordTag *copy = new CWordTag( *in );

			outwords.AddWordTag( copy );

			inwordpos++;
			continue;
		}

		// Should never fail
		CAnalyzedWord *w = GetAnalyzedWord( analysis, awordpos );
		if ( !w )
		{
			return;
		}

		if ( !stricmp( w->buffer, "<SIL>" ) )
		{
			awordpos++;
			continue;
		}

		char const *check = ApplyTBWordRules( in->GetWord() );
		if ( !FuzzyWordMatch( check, w->buffer ) )
		{
			bool advance_input = true;
			if ( previous[ 0 ] )
			{
				if ( FuzzyWordMatch( previous, w->buffer ) )
				{
					advance_input = false;
				}
			}

			if ( advance_input )
			{
				inwordpos++;
			}
			awordpos++;
			continue;
		}
		strcpy( previous, check );

		CWordTag *newWord = new CWordTag;

		newWord->SetWord( in->GetWord() );

		newWord->m_flStartTime = w->starttime;
		newWord->m_flEndTime = w->endtime;

		newWord->m_uiStartByte = ComputeByteFromTime( w->starttime );
		newWord->m_uiEndByte = ComputeByteFromTime( w->endtime );

		int phonemestart, phonemeend;

		phonemestart = GetPhonemeIndexAtWordStart( analysis, w->starttime );
		phonemeend = GetPhonemeIndexAtWordEnd( analysis, w->endtime );

		if ( phonemestart >= 0 && phonemeend >= 0 )
		{
			for ( ; phonemestart <= phonemeend; phonemestart++ )
			{
				// Get phoneme and timing info
				CAnalyzedPhoneme *ph = GetAnalyzedPhoneme( analysis, phonemestart );
				if ( !ph )
					continue;

				CPhonemeTag *ptag = new CPhonemeTag;
				ptag->SetStartTime( ph->starttime );
				ptag->SetEndTime( ph->endtime );

				ptag->m_uiStartByte = ComputeByteFromTime( ph->starttime );
				ptag->m_uiEndByte = ComputeByteFromTime( ph->endtime );

				ptag->SetTag( ph->phoneme );
				ptag->SetPhonemeCode( TextToPhoneme( ptag->GetTag() ) );

				newWord->m_Phonemes.AddToTail( ptag );
			}
		}

		outwords.AddWordTag( newWord );
		inwordpos++;
		awordpos++;
	}
}

char const *CPhonemeExtractorLipSinc::ApplyTBWordRules( char const *word )
{
	static char outword[ 256 ];

	char const *in = word;
	char *out = outword;

	while ( *in && ( ( out - outword ) <= 255 ) )
	{
		if ( *in == '\t' ||
			 *in == ' ' ||
			 *in == '\n' ||
			 *in == '-' ||
			 *in == '.' ||
			 *in == ',' ||
			 *in == ';' ||
			 *in == '?' ||
			 *in == '"' ||
			 *in == ':' ||
			 *in == '(' ||
			 *in == ')' )
		{
			in++;
			*out++ = ' ';
			continue;
		}

		if ( !V_isprint( *in ) )
		{
			in++;
			continue;
		}

		if ( *in >= 128 )
		{
			in++;
			continue;
		}

		// Skip numbers
		if ( *in >= '0' && *in <= '9' )
		{
			in++;
			continue;
		}

		// Convert all letters to upper case
		if ( *in >= 'a' && *in <= 'z' )
		{
			*out++ = ( *in++ ) - 'a' + 'A';
			continue;
		}

		if ( *in >= 'A' && *in <= 'Z' )
		{
			*out++ = *in++;
			continue;
		}

		if ( *in == '\'' )
		{
			*out++ = *in++;
			continue;
		}

		in++;
	}

	*out = 0;

	return outword;
}

//-----------------------------------------------------------------------------
// Purpose: Given a wavfile and a list of inwords, determines the word/phonene 
//  sample counts for the sentce
// Output : SR_RESULT
//-----------------------------------------------------------------------------
SR_RESULT CPhonemeExtractorLipSinc::Extract( 
	const char *wavfile,
	int numsamples,
	void (*pfnPrint)( const char *fmt, ... ),
	CSentence& inwords,
	CSentence& outwords )
{
	// g_enableTalkBackDebuggingOutput = 1;

	m_pfnPrint = pfnPrint;

	if ( !InitLipSinc() )
	{
		return SR_RESULT_ERROR;
	}
	
	m_flSampleCount = numsamples;

	if ( !CheckSoundFile( wavfile ) )
	{
		FreeLibrary( m_hHelper );
		return SR_RESULT_ERROR;
	}

	TALKBACK_ANALYSIS *analysis = NULL;

	if ( !AttemptAnalysis( &analysis, wavfile, inwords ) )
	{
		FreeLibrary( m_hHelper );
		return SR_RESULT_FAILED;
	}
	
	if ( strlen( inwords.GetText() ) <= 0 )
	{
		inwords.SetTextFromWords();
	}

	outwords = inwords;

	// Examine data
	ProcessWords( analysis, inwords, outwords );

	if ( analysis )
	{
		talkback->TalkBackFreeAnalysis( &analysis );
	}

	return SR_RESULT_SUCCESS;
}

EXPOSE_SINGLE_INTERFACE( CPhonemeExtractorLipSinc, IPhonemeExtractor, VPHONEME_EXTRACTOR_INTERFACE );