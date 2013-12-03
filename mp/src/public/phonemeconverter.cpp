//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#include <stdio.h>
#include <string.h>
#include "tier0/dbg.h"

struct PhonemeMap_t
{
	const char		*string;
	int				code;
	float			weight;
	bool			isStandard;
	const char		*desc;
};

static PhonemeMap_t g_Phonemes[] =
{
	{ "b", 'b', 0.8f, true, "Big : voiced alveolar stop" },
	{ "m", 'm', 1.0f, true, "Mat : voiced bilabial nasal" },
	{ "p", 'p', 0.8f, true, "Put; voiceless alveolar stop" },
	{ "w", 'w', 1.0f, true, "With : voiced labial-velar approximant" },
	{ "f", 'f', 0.8f, true, "Fork : voiceless labiodental fricative" },
	{ "v", 'v', 0.8f, true, "Val : voiced labialdental fricative" },
	{ "r", 0x0279, 1.0f, true, "Red : voiced alveolar approximant" },
	{ "r2", 'r', 1.0f, true, "Red : voiced alveolar trill" },
	{ "r3", 0x027b, 1.0f, true, "Red : voiced retroflex approximant" },
	{ "er", 0x025a, 1.2f, true, "URn : rhotacized schwa" },
	{ "er2", 0x025d, 1.2f, true, "URn : rhotacized lower-mid central vowel" },
	{ "dh", 0x00f0, 1.0f, true, "THen : voiced dental fricative" },
	{ "th", 0x03b8, 1.0f, true, "THin : voiceless dental fricative" },
	{ "sh", 0x0283, 1.0f, true, "SHe : voiceless postalveolar fricative" },
	{ "jh", 0x02a4, 1.0f, true, "Joy : voiced postalveolar afficate" },
	{ "ch", 0x02a7, 1.0f, true, "CHin : voiceless postalveolar affricate" },
	{ "s", 's', 0.8f, true, "Sit : voiceless alveolar fricative" },
	{ "z", 'z', 0.8f, true, "Zap : voiced alveolar fricative" },
	{ "d", 'd', 0.8f, true, "Dig : voiced bilabial stop" },
	{ "d2", 0x027e, 0.8f, true, "Dig : voiced alveolar flap or tap" },
	{ "l", 'l', 0.8f, true, "Lid : voiced alveolar lateral approximant" },
	{ "l2", 0x026b, 0.8f, true, "Lid : velarized voiced alveolar lateral approximant" },
	{ "n", 'n', 0.8f, true, "No : voiced alveolar nasal" },
	{ "t", 't', 0.8f, true, "Talk : voiceless bilabial stop" },
	{ "ow", 'o', 1.2f, true, "gO : upper-mid back rounded vowel" },
	{ "uw", 'u', 1.2f, true, "tOO : high back rounded vowel" },
	{ "ey", 'e', 1.0f, true, "Ate : upper-mid front unrounded vowel" },
	{ "ae", 0x00e6, 1.0f, true, "cAt : semi-low front unrounded vowel" },
	{ "aa", 0x0251, 1.0f, true, "fAther : low back unrounded vowel" },
	{ "aa2", 'a', 1.0f, true, "fAther : low front unrounded vowel" },
	{ "iy", 'i', 1.0f, true, "fEEl : high front unrounded vowel" },
	{ "y", 'j', 0.7f, true, "Yacht : voiced palatal approximant" },
	{ "ah", 0x028c, 1.0f, true, "cUt : lower-mid back unrounded vowel" },
	{ "ao", 0x0254, 1.2f, true, "dOg : lower-mid back rounded vowel" },
	{ "ax", 0x0259, 1.0f, true, "Ago : mid-central unrounded vowel" },
	{ "ax2", 0x025c, 1.0f, true, "Ago : lower-mid central unrounded vowel" },
	{ "eh", 0x025b, 1.0f, true, "pEt : lower-mid front unrounded vowel"},
	{ "ih", 0x026a, 1.0f, true, "fIll : semi-high front unrounded vowel" },
	{ "ih2", 0x0268, 1.0f, true, "fIll : high central unrounded vowel" },
	{ "uh", 0x028a, 1.0f, true, "bOOk : semi-high back rounded vowel" },
	{ "g", 'g', 0.8f, true, "taG : voiced velar stop" },
	{ "g2", 0x0261, 1.0f, true, "taG : voiced velar stop" },
	{ "hh", 'h', 0.8f, true, "Help : voiceless glottal fricative" },
	{ "hh2", 0x0266, 0.8f, true, "Help : breathy-voiced glottal fricative" },
	{ "c", 'k', 0.6f, true, "Cut : voiceless velar stop" },
	{ "nx", 0x014b, 1.0f, true, "siNG : voiced velar nasal" },
	{ "zh", 0x0292, 1.0f, true, "aZure : voiced postalveolar fricative" },

	// Added
	{ "h", 'h', 0.8f, false, "Help : voiceless glottal fricative" },
	{ "k", 'k', 0.6f, false, "Cut : voiceless velar stop" },
	{ "ay", 0x0251, 1.0f, false, "fAther : low back unrounded vowel" }, // or possibly +0x026a (ih)
	{ "ng", 0x014b, 1.0f, false, "siNG : voiced velar nasal" }, // nx
	{ "aw", 0x0251, 1.2f, false, "fAther : low back unrounded vowel" }, // // vOWel,   // aa + uh???
	{ "oy", 'u', 1.2f, false, "tOO : high back rounded vowel" },

	// Silence
	{ "<sil>", '_', 1.0f, true, "silence" },
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : code - 
// Output : const char
//-----------------------------------------------------------------------------
const char *ConvertPhoneme( int code )
{
	for ( int i = 0; i < ARRAYSIZE( g_Phonemes ); ++i )
	{
		PhonemeMap_t *test = &g_Phonemes[ i ];
		if ( test->code == code )
			return test->string;
	}

	Warning( "Unrecognized phoneme code %i\n", code );
	return "<sil>";
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *text - 
// Output : int
//-----------------------------------------------------------------------------
int TextToPhoneme( const char *text )
{
	for ( int i = 0; i < ARRAYSIZE( g_Phonemes ); ++i )
	{
		PhonemeMap_t *test = &g_Phonemes[ i ];
		if ( !stricmp( test->string, text ) )
			return test->code;
	}

	Warning( "Unrecognized phoneme %s\n", text );
	return '_';
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : code - 
// Output : float
//-----------------------------------------------------------------------------
float WeightForPhonemeCode( int code )
{
	for ( int i = 0; i < ARRAYSIZE( g_Phonemes ); ++i )
	{
		PhonemeMap_t *test = &g_Phonemes[ i ];
		if ( test->code == code )
			return test->weight;
	}

	Warning( "Unrecognized phoneme code %i\n", code );
	return 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *text - 
// Output : float
//-----------------------------------------------------------------------------
float WeightForPhoneme( char *text )
{
	for ( int i = 0; i < ARRAYSIZE( g_Phonemes ); ++i )
	{
		PhonemeMap_t *test = &g_Phonemes[ i ];
		if ( !stricmp( test->string, text ) )
			return test->weight;
	}

	Warning( "WeightForPhoneme:: Unrecognized phoneme %s\n", text );
	return 1.0f;
}

int NumPhonemes()
{
	return ARRAYSIZE( g_Phonemes );
}

const char *NameForPhonemeByIndex( int index )
{
	Assert( index >= 0 && index < NumPhonemes() );
	return g_Phonemes[ index ].string;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *text - 
// Output : int
//-----------------------------------------------------------------------------
int TextToPhonemeIndex( const char *text )
{
	for ( int i = 0; i < ARRAYSIZE( g_Phonemes ); ++i )
	{
		PhonemeMap_t *test = &g_Phonemes[ i ];
		if ( !stricmp( test->string, text ) )
			return i;
	}

	return -1;
}

int CodeForPhonemeByIndex( int index )
{
	if ( index < 0 || index >= NumPhonemes() )
		return '_';
	return g_Phonemes[ index ].code;
}

bool IsStandardPhoneme( int index )
{
	if ( index < 0 || index >= NumPhonemes() )
		return false;
	return g_Phonemes[ index ].isStandard;
}

const char *DescForPhonemeByIndex( int index )
{
	if ( index < 0 || index >= NumPhonemes() )
		return NULL;
	return g_Phonemes[ index ].desc;
}