//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SENTENCE_H
#define SENTENCE_H
#ifdef _WIN32
#pragma once
#endif

// X360 optimizes out the extra memory needed by the editors in these types
#ifndef _X360
#define PHONEME_EDITOR 1
#endif

#include "utlvector.h"

class CUtlBuffer;

#define CACHED_SENTENCE_VERSION			1
#define CACHED_SENTENCE_VERSION_ALIGNED	4

//-----------------------------------------------------------------------------
// Purpose: A sample point
//-----------------------------------------------------------------------------
// Can't do this due to backward compat issues
//#ifdef _WIN32
//#pragma pack (1)
//#endif

struct CEmphasisSample
{
	float		time;
	float		value;

	void SetSelected( bool isSelected );
#if PHONEME_EDITOR
	// Used by editors only
	bool		selected;
#endif
};

class CBasePhonemeTag
{
public:
	CBasePhonemeTag();
	CBasePhonemeTag( const CBasePhonemeTag& from );

	CBasePhonemeTag &operator=( const CBasePhonemeTag &from )	{ memcpy( this, &from, sizeof(*this) ); return *this; }

	float GetStartTime() const				{ return m_flStartTime; }
	void SetStartTime( float startTime )	{ m_flStartTime = startTime; }
	void AddStartTime( float startTime )	{ m_flStartTime += startTime; }

	float GetEndTime() const				{ return m_flEndTime; }
	void SetEndTime( float endTime )		{ m_flEndTime = endTime; }
	void AddEndTime( float startTime )		{ m_flEndTime += startTime; }

	int GetPhonemeCode() const				{ return m_nPhonemeCode; }
	void SetPhonemeCode( int phonemeCode )	{ m_nPhonemeCode = phonemeCode; }

private:
	float			m_flStartTime;
	float			m_flEndTime;
	unsigned short	m_nPhonemeCode;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPhonemeTag : public CBasePhonemeTag
{
	typedef CBasePhonemeTag BaseClass;
public:

					CPhonemeTag( void );
					CPhonemeTag( const char *phoneme );
					CPhonemeTag( const CPhonemeTag& from );
					~CPhonemeTag( void );

	void			SetTag( const char *phoneme );
	char const		*GetTag() const;

	unsigned int	ComputeDataCheckSum();
#if PHONEME_EDITOR
	bool			m_bSelected;
	unsigned int	m_uiStartByte;
	unsigned int	m_uiEndByte;
#endif
	void SetSelected( bool isSelected );
	bool GetSelected() const;
	void SetStartAndEndBytes( unsigned int start, unsigned int end );
	unsigned int GetStartByte() const;
	unsigned int GetEndByte() const;

private:
	char			*m_szPhoneme;

};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CWordTag
{
public:
					CWordTag( void );
					CWordTag( const char *word );
					CWordTag( const CWordTag& from );
					~CWordTag( void );

	void			SetWord( const char *word );
	const char		*GetWord() const;

	int				IndexOfPhoneme( CPhonemeTag *tag );

	unsigned int	ComputeDataCheckSum();

	float			m_flStartTime;
	float			m_flEndTime;

	CUtlVector	< CPhonemeTag *> m_Phonemes;
#if PHONEME_EDITOR
	bool			m_bSelected;
	unsigned int	m_uiStartByte;
	unsigned int	m_uiEndByte;
#endif
	void SetSelected( bool isSelected );
	bool GetSelected() const;
	void SetStartAndEndBytes( unsigned int start, unsigned int end );
	unsigned int GetStartByte() const;
	unsigned int GetEndByte() const;

private:
	char			*m_pszWord;
};

// A sentence can be closed captioned
// The default case is the entire sentence shown at start time
// 
// "<persist:2.0><clr:255,0,0,0>The <I>default<I> case"
// "<sameline>is the <U>entire<U> sentence shown at <B>start time<B>"

// Commands that aren't closed at end of phrase are automatically terminated
//
// Commands
// <linger:2.0>	The line should persist for 2.0 seconds beyond m_flEndTime
// <sameline>		Don't go to new line for next phrase on stack
// <clr:r,g,b,a>	Push current color onto stack and start drawing with new
//  color until we reach the next <clr> marker or a <clr> with no commands which
//  means restore the previous color
// <U>				Underline text (start/end)
// <I>				Italics text (start/end)
// <B>				Bold text (start/end)
// <position:where>	Draw caption at special location ??? needed
// <cr>				Go to new line

// Close Captioning Support
// The phonemes drive the mouth in english, but the CC text can
//  be one of several languages
enum
{
	CC_ENGLISH = 0,
	CC_FRENCH,
	CC_GERMAN,
	CC_ITALIAN,
	CC_KOREAN,
	CC_SCHINESE,  // Simplified Chinese
	CC_SPANISH,
	CC_TCHINESE,  // Traditional Chinese
	CC_JAPANESE,
	CC_RUSSIAN,
	CC_THAI,
	CC_PORTUGUESE,
	// etc etc

	CC_NUM_LANGUAGES
};

//-----------------------------------------------------------------------------
// Purpose: A sentence is a box of words, and words contain phonemes
//-----------------------------------------------------------------------------
class CSentence
{
public:
	static char const	*NameForLanguage( int language );
	static int			LanguageForName( char const *name );
	static void 		ColorForLanguage( int language, unsigned char& r, unsigned char& g, unsigned char& b );

	// Construction
					CSentence( void );
					~CSentence( void );

	// Assignment operator
	CSentence& operator =(const CSentence& src );

	void			Append( float starttime, const CSentence& src );

	void			SetText( const char *text );
	const char		*GetText( void ) const;

	void			InitFromDataChunk( void *data, int size );
	void			InitFromBuffer( CUtlBuffer& buf );
	void			SaveToBuffer( CUtlBuffer& buf );

	//				This strips out all of the stuff used by the editor, leaving just one blank work, no sentence text, and just
	//					the phonemes without the phoneme text...(same as the cacherestore version below)
	void			MakeRuntimeOnly();

	// This is a compressed save of just the data needed to drive phonemes in the engine (no word / sentence text, etc )
	void			CacheSaveToBuffer( CUtlBuffer& buf, int version );
	void			CacheRestoreFromBuffer( CUtlBuffer& buf );

	// Add word/phoneme to sentence
	void			AddPhonemeTag( CWordTag *word, CPhonemeTag *tag );
	void			AddWordTag( CWordTag *tag );

	void			Reset( void );

	void			ResetToBase( void );

	void			MarkNewPhraseBase( void );

	int				GetWordBase( void );

	int				CountPhonemes( void );

	// For legacy loading, try to find a word that contains the time
	CWordTag		*EstimateBestWord( float time );

	CWordTag		*GetWordForPhoneme( CPhonemeTag *phoneme );

	void			SetTextFromWords( void );

	float			GetIntensity( float time, float endtime );
	void			Resort( void );
	CEmphasisSample *GetBoundedSample( int number, float endtime );
	int				GetNumSamples( void );
	CEmphasisSample	*GetSample( int index );

	// Compute start and endtime based on all words
	void			GetEstimatedTimes( float& start, float &end );

	void			SetVoiceDuck( bool shouldDuck ) { m_bShouldVoiceDuck = shouldDuck; }
	bool			GetVoiceDuck() const { return m_bShouldVoiceDuck; }

	unsigned int	ComputeDataCheckSum();

	void			SetDataCheckSum( unsigned int chk );
	unsigned int	GetDataCheckSum() const;

	int				GetRuntimePhonemeCount() const;
	const CBasePhonemeTag *GetRuntimePhoneme( int i ) const;
	void			ClearRuntimePhonemes();
	void			AddRuntimePhoneme( const CPhonemeTag *src );

	void			CreateEventWordDistribution( char const *pszText, float flSentenceDuration );
	static int		CountWords( char const *pszText );
	static bool		ShouldSplitWord( char in );

public:
#if PHONEME_EDITOR
	char			*m_szText;

	CUtlVector< CWordTag * >	m_Words;
#endif
	CUtlVector	< CBasePhonemeTag *> m_RunTimePhonemes;

#if PHONEME_EDITOR
	int				m_nResetWordBase;
#endif
	// Phoneme emphasis data
	CUtlVector< CEmphasisSample > m_EmphasisSamples;

#if PHONEME_EDITOR
	unsigned int	m_uCheckSum;
#endif
	bool			m_bIsValid : 8;
	bool			m_bStoreCheckSum : 8;
	bool			m_bShouldVoiceDuck : 8;
	bool			m_bIsCached : 8;

private:
	void			ParseDataVersionOnePointZero( CUtlBuffer& buf );
	void			ParsePlaintext( CUtlBuffer& buf );
	void			ParseWords( CUtlBuffer& buf );
	void			ParseEmphasis( CUtlBuffer& buf );
	void			ParseOptions( CUtlBuffer& buf );
	void			ParseCloseCaption( CUtlBuffer& buf );

	void			ResetCloseCaptionAll( void );

	friend class PhonemeEditor;
};

//#ifdef _WIN32
//#pragma pack ()
//#endif

#endif // SENTENCE_H
