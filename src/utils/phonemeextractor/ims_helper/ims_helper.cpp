//========= Copyright Valve Corporation, All rights reserved. ============//
#include <stdio.h>
#include "ims_helper.h"

#include "TalkBack.h"

class CImsHelper : public IImsHelper
{
public:
	// Must be the first function called when using TalkBack.
	TALKBACK_ERR TalkBackStartupLibrary(
		char const *iCoreDataDir); // IN:      full path of folder containing TalkBack data files.

	// Should be the last function called when using TalkBack.
	TALKBACK_ERR               // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackShutdownLibrary(); // IN:      nothing.

	// ------------------
	// Version functions.
	// ------------------

	// Gets the TalkBack version number.
	TALKBACK_ERR          // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetVersion(
		long *oMajor,     // OUT:     major version number.
		long *oMinor,     // OUT:     minor version number.
		long *oRevision); // OUT:     revision version number.

	// Gets the TalkBack version number as a string.
	TALKBACK_ERR         // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetVersionString(
		long  iMaxChars, // IN:      size of version string buffer.
		char *oVersion); // OUT:     version string buffer.

	// ------------------
	// Utility functions.
	// ------------------

	// Checks whether a sound file can be analyzed and returns some quality metrics.
	//
	// NOTE: this function is deprecated and has been supplanted by
	// TalkBackGetSoundFileMetrics().
	TALKBACK_ERR                    // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackCheckSoundFile(
		char const *iSoundFileName, // IN:      name of sound file to be checked.
		long       *oCanBeAnalyzed, // OUT:     1 if sound can be analyzed, 0 if not.
		long       *oIsClipped,     // OUT:     1 if sound is clipped, 0 if not.
		double     *oDecibelRange); // OUT:     used decibel range of sound.

	// Returns metrics for the specified sound file.
	TALKBACK_ERR                                     // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetSoundFileMetrics(
		char const                  *iSoundFileName, // IN:      name of sound file to be checked.
		TALKBACK_SOUND_FILE_METRICS *ioMetrics);     // IN/OUT:  address of a structure where the metrics will be stored.

	// Checks whether text can be used for text-based analysis, returning the text
	// as it will be analyzed.
	TALKBACK_ERR                    // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackCheckSpokenText(
		char const *iSpokenText,    // IN:      text to check.
		long        iMaxChars,      // IN:      size of analyzed text buffer.
		char       *oAnalyzedText); // OUT:     buffer for text as it will be analyzed.

	// Convert a TalkBack error code to a description string.
	TALKBACK_ERR                    // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetErrorString(
		TALKBACK_ERR iErrorCode,    // IN:      TalkBack error code to convert.
		long         iMaxChars,     // IN:      size of the buffer.
		char        *oErrorString); // OUT:     buffer for the description string.

	// Gets the error code and text for the most recent TalkBack error.
	TALKBACK_ERR                     // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetLastError(
		long          iMaxChars,    // IN:      size of the buffer.
		char         *oErrorString, // OUT:     buffer for the description string.
		TALKBACK_ERR *oErrorCode);  // OUT:     most recent TalkBack error code.

	// -------------------
	// Analysis functions.
	// -------------------

	// Gets an opaque TALKBACK_ANALYSIS object. This object is then queried with the
	// TalkBackGet* functions below.
	TALKBACK_ERR                                    // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetAnalysis(
		TALKBACK_ANALYSIS         **ioAnalysis,     // IN/OUT:  address of a TALKBACK_ANALYSIS *variable where analysis will be stored.
		char const                 *iSoundFileName, // IN:      name of the sound file to analyze.
		char const                 *iSoundText,     // IN:      text spoken in sound file (can be NULL to use textless analysis).
		TALKBACK_ANALYSIS_SETTINGS *iSettings);     // IN:      pointer to a TALKBACK_ANALYSIS_SETTINGS structure (can be NULL for defaults).

	// Frees an opaque TALKBACK_ANALYSIS object. This releases all memory used by
	// the analysis.
	TALKBACK_ERR                         // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackFreeAnalysis(
		TALKBACK_ANALYSIS **ioAnalysis); // IN/OUT:  analysis to free.

	// #######################################################################
	// NOTE: all functions from this point on require a valid analysis object.
	// #######################################################################

	// ------------------------
	// Speech target functions.
	// ------------------------

	// Gets the number of speech target tracks.
	TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetNumSpeechTargetTracks(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long              *oResult);  // OUT:     number of speech target tracks.

	// Gets the number of keys in the specified speech target track.
	TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetNumSpeechTargetKeys(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum, // IN:      speech target track.
		long              *oResult);  // OUT:     number of keys in the speech target track.

	// Gets key information (time, value, derivative in, and derivative out) for the
	// specified key in the specified speech target track.
	TALKBACK_ERR                            // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetSpeechTargetKeyInfo(
		TALKBACK_ANALYSIS *iAnalysis,       // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum,       // IN:      speech target track.
		long               iKeyNum,         // IN:      speech target key.
		double            *oTime,           // OUT:     time of key.
		double            *oValue,          // OUT:     value of key.
		double            *oDerivativeIn,   // OUT:     incoming derivative of key.
		double            *oDerivativeOut); // OUT:     outgoing derivative of key.

	// Gets the value of the function curve for the specified speech target track at
	// the specified time.
	TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetSpeechTargetValueAtTime(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum, // IN:      speech target track.
		double             iTime,     // IN:      time in seconds.
		double            *oResult);  // OUT:     value of the function curve.

	// Gets the derivatives of the function curve for the specified speech target
	// track at the specified time.
	TALKBACK_ERR                            // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetSpeechTargetDerivativesAtTime(
		TALKBACK_ANALYSIS *iAnalysis,       // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum,       // IN:      speech target track.
		double             iTime,           // IN:      time in seconds.
		double            *oDerivativeIn,   // OUT:     value of the incoming derivative of the function curve.
		double            *oDerivativeOut); // OUT:     value of the outgoing derivative of the function curve.

	// -------------------------
	// Speech gesture functions.
	// -------------------------

	// Gets the number of speech gesture tracks.
	TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetNumGestureTracks(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long              *oResult);  // OUT:     number of speech gesture tracks

	// Gets the number of keys in the specified speech gesture track.
	TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetNumGestureKeys(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum, // IN:      speech gesture track.
		long              *oResult);  // OUT:     number of keys in the speech gesture track.

	// Gets key information (time, value, derivative in, and derivative out) for the
	// specified key in the specified speech gesture track.
	TALKBACK_ERR                            // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetGestureKeyInfo(
		TALKBACK_ANALYSIS *iAnalysis,       // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum,       // IN:      speech gesture track.
		long               iKeyNum,         // IN:      speech gesture key.
		double            *oTime,           // OUT:     time of key.
		double            *oValue,          // OUT:     value of key.
		double            *oDerivativeIn,   // OUT:     incoming derivative of key.
		double            *oDerivativeOut); // OUT:     outgoing derivative of key.

	// Gets the value of the function curve for the specified speech gesture track
	// at the specified time.
	TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetGestureValueAtTime(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum, // IN:      speech gesture track.
		double             iTime,     // IN:      time in seconds.
		double            *oResult);  // OUT:     value of the function curve.

	// Gets the derivatives of the function curve for the specified speech gesture
	// track at the specified time.
	TALKBACK_ERR                            // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetGestureDerivativesAtTime(
		TALKBACK_ANALYSIS *iAnalysis,       // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum,       // IN:      speech gesture track.
		double             iTime,           // IN:      time in seconds.
		double            *oDerivativeIn,   // OUT:     value of the incoming derivative of the function curve.
		double            *oDerivativeOut); // OUT:     value of the outgoing derivative of the function curve.

	// ----------------
	// Frame functions.
	// ----------------

	// NOTE: these functions use the frame rate specified in the
	// TALKBACK_ANALYSIS_SETTINGS structure passed to TalkBackGetAnalysis() and
	// default to 30 fps (TALKBACK_DEFAULT_FRAME_RATE) if the structure pointer was
	// NULL.

	// Gets the first frame number.
	TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetFirstFrameNum(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long              *oResult);  // OUT:     number of the first frame.

	// Gets the last frame number.
	TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetLastFrameNum(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long              *oResult);  // OUT:     number of the last frame.

	// Gets the start time of the specified frame.
	TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetFrameStartTime(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iFrameNum, // IN:      frame.
		double            *oResult);  // OUT:     start time of the frame in seconds.

	// Gets the end time of the specified frame.
	TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetFrameEndTime(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iFrameNum, // IN:      frame.
		double            *oResult);  // OUT:     end time of the frame in seconds.

	// Gets the value of the function curve for a speech target integrated over the
	// specified frame.
	TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetSpeechTargetValueAtFrame(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum, // IN:      speech target track.
		long               iFrameNum, // IN:      frame number.
		double            *oResult);  // OUT:     value of the function curve integrated over the frame.

	// Gets the dominant speech target at the specified frame.
	//
	// NOTE: this function is meant to be used in flipbook mode only.
	TALKBACK_ERR                                // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetDominantSpeechTargetAtFrame(
		TALKBACK_ANALYSIS      *iAnalysis,      // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long                    iFrameNum,      // IN:      frame number.
		TALKBACK_SPEECH_TARGET *oSpeechTarget); // OUT:     dominant speech target.

	// Gets the value of the function curve for a speech gesture integrated over the
	// specified frame.
	TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetGestureValueAtFrame(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum, // IN:      speech gesture track.
		long               iFrameNum, // IN:      frame number.
		double            *oResult);  // OUT:     value of the function curve integrated over the frame.

	// ------------------
	// Phoneme functions.
	// ------------------

	// Gets the number of phonemes.
	TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetNumPhonemes(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long              *oResult);  // OUT:     number of phonemes.

	// Gets the enumeration of the specified phoneme.
	TALKBACK_ERR                        // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetPhonemeEnum(
		TALKBACK_ANALYSIS *iAnalysis,   // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iPhonemeNum, // IN:      phoneme.
		TALKBACK_PHONEME  *oResult);    // OUT:     enumeration of the specified phoneme.

	// Gets the start time of the specified phoneme.
	TALKBACK_ERR                        // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetPhonemeStartTime(
		TALKBACK_ANALYSIS *iAnalysis,   // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iPhonemeNum, // IN:      phoneme.
		double            *oResult);    // OUT:     start time of the phoneme in seconds.

	// Gets the end time of the specified phoneme.
	TALKBACK_ERR                        // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetPhonemeEndTime(
		TALKBACK_ANALYSIS *iAnalysis,   // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iPhonemeNum, // IN:      phoneme.
		double            *oResult);    // OUT:     end time of the phoneme in seconds.

	// ---------------
	// Word functions.
	// ---------------

	// NOTE: these functions only yield data for text-based analysis.

	// Gets the number of words.
	TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetNumWords(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long              *oResult);  // OUT:     number of words.

	// Gets the text of the specified word.
	TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetWord(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iWordNum,  // IN:      word.
		long               iMaxChars, // IN:      size of word buffer.
		char              *oWord);    // OUT:     word buffer.

	// Gets the start time of the specified word.
	TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetWordStartTime(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iWordNum,  // IN:      word.
		double            *oResult);  // OUT:     start time of the word in seconds.

	// Gets the end time of the specified word.
	TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetWordEndTime(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iWordNum,  // IN:      word.
		double            *oResult);  // OUT:     end time of the word in seconds.

	// --------------------------
	// Phoneme editing functions.
	// --------------------------

	// Use these functions to modify the phoneme list after you get an opaque
	// analysis object from TalkBackGetAnalysis(). After modifying the phoneme list
	// in the opaque analysis object, subsequent TalkBackGet* calls on that opaque
	// analysis object for speech target (lip-synching) data will return values
	// based on the modified phoneme list. However, speech gesture data is not
	// affected by phoneme editing.
	//
	// NOTE: phoneme editing is only provided in order to support Ventriloquist-like
	// applications where tweaking of the phoneme segmenation (and subsequent
	// recalculation of the animation data) is required. Most customers probably
	// won't need this functionality.

	// Inserts a phoneme at the specified position in the specified manner.
	TALKBACK_ERR                            // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackInsertPhoneme(
		TALKBACK_ANALYSIS *iAnalysis,       // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		TALKBACK_PHONEME   iPhoneme,        // IN:      enumeration of phoneme to insert.
		long               iInsertPosition, // IN:      position (phoneme number) at which to insert.
		int                iInsertBefore);  // IN:      manner of insertion:
											//            0 means put phoneme after insert position;
											//            1 means put phoneme before insert position.

	// Deletes the specified phoneme.
	TALKBACK_ERR                              // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackDeletePhoneme(
		TALKBACK_ANALYSIS *iAnalysis,         // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iPhonemeToDelete); // IN:      phoneme to delete.

	// Changes the start time of the specified phoneme.
	//
	// NOTE: the start time specified may not be the actual start time for a number
	// of reasons, most notably if the specified start time will make the phoneme
	// too short. This function returns the actual start time so the caller can
	// check the result without having to query the phoneme.
	TALKBACK_ERR                             // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackChangePhonemeStart(
		TALKBACK_ANALYSIS *iAnalysis,        // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iPhonemeToChange, // IN:      phoneme to change.
		double            *ioNewTime);       // IN/OUT:  new start time value in seconds (in); actual start time (out).

	// Changes the end time of the specified phoneme.
	//
	// NOTE: the end time specified may not be the actual end time for a number of
	// reasons, most notably if the specified end time will make the phoneme too
	// short. This function returns the actual end time so the caller can check the
	// result without having to query the phoneme.
	TALKBACK_ERR                             // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackChangePhonemeEnd(
		TALKBACK_ANALYSIS *iAnalysis,        // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iPhonemeToChange, // IN:      phoneme to change.
		double            *ioNewTime);       // IN/OUT:  new end time value in seconds (in); actual end time (out).

	// Changes the enumeration of the specified phoneme.
	TALKBACK_ERR                             // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackChangePhonemeEnum(
		TALKBACK_ANALYSIS *iAnalysis,        // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iPhonemeToChange, // IN:      phoneme to change.
		TALKBACK_PHONEME   iNewPhoneme);     // IN:      new phoneme enumeration.
};

static CImsHelper g_Helper;

// Expose to loading code
__declspec( dllexport ) IImsHelper *GetImsHelper()
{
	return &g_Helper;
}

TALKBACK_ERR CImsHelper::TalkBackStartupLibrary(
	char const *iCoreDataDir)
{
	return ::TalkBackStartupLibrary( iCoreDataDir );
}

TALKBACK_ERR CImsHelper::TalkBackShutdownLibrary()
{
	return ::TalkBackShutdownLibrary();
}

TALKBACK_ERR CImsHelper::TalkBackGetVersion(
	long *oMajor,     
	long *oMinor,     
	long *oRevision) 
{
	return ::TalkBackGetVersion( oMajor, oMinor, oRevision );
}


TALKBACK_ERR CImsHelper::TalkBackGetVersionString(
	long  iMaxChars, 
	char *oVersion) 
{
	return ::TalkBackGetVersionString( iMaxChars, oVersion );
}

TALKBACK_ERR CImsHelper::TalkBackCheckSoundFile(
	char const *iSoundFileName,
	long       *oCanBeAnalyzed,
	long       *oIsClipped,
	double     *oDecibelRange) 
{
	return ::TalkBackCheckSoundFile( iSoundFileName, oCanBeAnalyzed, oIsClipped, oDecibelRange );
}

TALKBACK_ERR CImsHelper::TalkBackGetSoundFileMetrics(
	char const                  *iSoundFileName,
	TALKBACK_SOUND_FILE_METRICS *ioMetrics)
{
	return ::TalkBackGetSoundFileMetrics( iSoundFileName, ioMetrics );
}

TALKBACK_ERR CImsHelper::TalkBackCheckSpokenText(
	char const *iSpokenText,   
	long        iMaxChars,      
	char       *oAnalyzedText)
{
	return ::TalkBackCheckSpokenText( iSpokenText, iMaxChars, oAnalyzedText );
}

TALKBACK_ERR CImsHelper::TalkBackGetErrorString(
	TALKBACK_ERR iErrorCode,   
	long         iMaxChars,     
	char        *oErrorString)
{
	return ::TalkBackGetErrorString( iErrorCode, iMaxChars, oErrorString );
}

TALKBACK_ERR CImsHelper::TalkBackGetLastError(
	long          iMaxChars,    
	char         *oErrorString, 
	TALKBACK_ERR *oErrorCode)
{
	return ::TalkBackGetLastError( iMaxChars, oErrorString, oErrorCode );
}

TALKBACK_ERR CImsHelper::TalkBackGetAnalysis(
	TALKBACK_ANALYSIS         **ioAnalysis,    
	char const                 *iSoundFileName, 
	char const                 *iSoundText,    
	TALKBACK_ANALYSIS_SETTINGS *iSettings)
{
	return ::TalkBackGetAnalysis( ioAnalysis, iSoundFileName, iSoundText, iSettings );
}    

TALKBACK_ERR CImsHelper::TalkBackFreeAnalysis(
	TALKBACK_ANALYSIS **ioAnalysis)
{
	return ::TalkBackFreeAnalysis( ioAnalysis );
}

TALKBACK_ERR CImsHelper::TalkBackGetNumSpeechTargetTracks(
	TALKBACK_ANALYSIS *iAnalysis, 
	long              *oResult)
{
	return ::TalkBackGetNumSpeechTargetTracks( iAnalysis, oResult );
} 

TALKBACK_ERR CImsHelper::TalkBackGetNumSpeechTargetKeys(
	TALKBACK_ANALYSIS *iAnalysis,
	long               iTrackNum, 
	long              *oResult)
{
	return ::TalkBackGetNumSpeechTargetKeys( iAnalysis, iTrackNum, oResult );
} 

TALKBACK_ERR CImsHelper::TalkBackGetSpeechTargetKeyInfo(
	TALKBACK_ANALYSIS *iAnalysis,       
	long               iTrackNum,      
	long               iKeyNum,         
	double            *oTime,           
	double            *oValue,         
	double            *oDerivativeIn,   
	double            *oDerivativeOut)
{
	return ::TalkBackGetSpeechTargetKeyInfo( iAnalysis, iTrackNum, iKeyNum, oTime, oValue, oDerivativeIn, oDerivativeOut );
} 

TALKBACK_ERR CImsHelper::TalkBackGetSpeechTargetValueAtTime(
	TALKBACK_ANALYSIS *iAnalysis, 
	long               iTrackNum, 
	double             iTime,    
	double            *oResult)
{
	return ::TalkBackGetSpeechTargetValueAtTime( iAnalysis, iTrackNum, iTime, oResult );
} 

TALKBACK_ERR CImsHelper::TalkBackGetSpeechTargetDerivativesAtTime(
	TALKBACK_ANALYSIS *iAnalysis,       
	long               iTrackNum,      
	double             iTime,           
	double            *oDerivativeIn,   
	double            *oDerivativeOut)
{
	return ::TalkBackGetSpeechTargetDerivativesAtTime( iAnalysis, iTrackNum, iTime, oDerivativeIn, oDerivativeOut );
} 

TALKBACK_ERR CImsHelper::TalkBackGetNumGestureTracks(
	TALKBACK_ANALYSIS *iAnalysis,
	long              *oResult)
{
	return ::TalkBackGetNumGestureTracks( iAnalysis, oResult );
}  

TALKBACK_ERR CImsHelper::TalkBackGetNumGestureKeys(
	TALKBACK_ANALYSIS *iAnalysis, 
	long               iTrackNum, 
	long              *oResult)
{
	return ::TalkBackGetNumGestureKeys( iAnalysis, iTrackNum, oResult );
}

TALKBACK_ERR CImsHelper::TalkBackGetGestureKeyInfo(
	TALKBACK_ANALYSIS *iAnalysis,      
	long               iTrackNum,      
	long               iKeyNum,         
	double            *oTime,           
	double            *oValue,          
	double            *oDerivativeIn,   
	double            *oDerivativeOut)
{
	return ::TalkBackGetGestureKeyInfo( iAnalysis, iTrackNum, iKeyNum, oTime, oValue, oDerivativeIn, oDerivativeOut );
}

TALKBACK_ERR CImsHelper::TalkBackGetGestureValueAtTime(
	TALKBACK_ANALYSIS *iAnalysis, 
	long               iTrackNum, 
	double             iTime,     
	double            *oResult)
{
	return ::TalkBackGetGestureValueAtTime( iAnalysis, iTrackNum, iTime, oResult );
}

TALKBACK_ERR CImsHelper::TalkBackGetGestureDerivativesAtTime( 
	TALKBACK_ANALYSIS *iAnalysis,      
	long               iTrackNum,       
	double             iTime,           
	double            *oDerivativeIn,   
	double            *oDerivativeOut)
{
	return ::TalkBackGetGestureDerivativesAtTime( iAnalysis, iTrackNum, iTime, oDerivativeIn, oDerivativeOut );
} 

TALKBACK_ERR CImsHelper::TalkBackGetFirstFrameNum(
	TALKBACK_ANALYSIS *iAnalysis, 
	long              *oResult)
{
	return ::TalkBackGetFirstFrameNum( iAnalysis, oResult );
}

TALKBACK_ERR CImsHelper::TalkBackGetLastFrameNum(
	TALKBACK_ANALYSIS *iAnalysis, 
	long              *oResult)
{
	return ::TalkBackGetLastFrameNum( iAnalysis, oResult );
}

TALKBACK_ERR CImsHelper::TalkBackGetFrameStartTime(
	TALKBACK_ANALYSIS *iAnalysis, 
	long               iFrameNum, 
	double            *oResult)
{
	return ::TalkBackGetFrameStartTime( iAnalysis, iFrameNum, oResult );
}

TALKBACK_ERR CImsHelper::TalkBackGetFrameEndTime(
	TALKBACK_ANALYSIS *iAnalysis, 
	long               iFrameNum, 
	double            *oResult)
{
	return ::TalkBackGetFrameEndTime( iAnalysis, iFrameNum, oResult );
}

TALKBACK_ERR CImsHelper::TalkBackGetSpeechTargetValueAtFrame(
	TALKBACK_ANALYSIS *iAnalysis, 
	long               iTrackNum, 
	long               iFrameNum, 
	double            *oResult)
{
	return ::TalkBackGetSpeechTargetValueAtFrame( iAnalysis, iTrackNum, iFrameNum, oResult );
}  

TALKBACK_ERR CImsHelper::TalkBackGetDominantSpeechTargetAtFrame(
	TALKBACK_ANALYSIS      *iAnalysis,      
	long                    iFrameNum,      
	TALKBACK_SPEECH_TARGET *oSpeechTarget)
{
	return ::TalkBackGetDominantSpeechTargetAtFrame( iAnalysis, iFrameNum, oSpeechTarget );
} 

TALKBACK_ERR CImsHelper::TalkBackGetGestureValueAtFrame(
	TALKBACK_ANALYSIS *iAnalysis, 
	long               iTrackNum, 
	long               iFrameNum, 
	double            *oResult)
{
	return ::TalkBackGetGestureValueAtFrame( iAnalysis, iTrackNum, iFrameNum, oResult );
}  

TALKBACK_ERR CImsHelper::TalkBackGetNumPhonemes(
	TALKBACK_ANALYSIS *iAnalysis, 
	long              *oResult)
{
	return ::TalkBackGetNumPhonemes( iAnalysis, oResult );
}  

TALKBACK_ERR CImsHelper::TalkBackGetPhonemeEnum(
	TALKBACK_ANALYSIS *iAnalysis, 
	long               iPhonemeNum,
	TALKBACK_PHONEME  *oResult)
{
	return ::TalkBackGetPhonemeEnum( iAnalysis, iPhonemeNum, oResult );
}  

TALKBACK_ERR CImsHelper::TalkBackGetPhonemeStartTime(
	TALKBACK_ANALYSIS *iAnalysis,  
	long               iPhonemeNum,
	double            *oResult)
{
	return ::TalkBackGetPhonemeStartTime( iAnalysis, iPhonemeNum, oResult );
}   

TALKBACK_ERR CImsHelper::TalkBackGetPhonemeEndTime(
	TALKBACK_ANALYSIS *iAnalysis,  
	long               iPhonemeNum,
	double            *oResult)
{
	return ::TalkBackGetPhonemeEndTime( iAnalysis, iPhonemeNum, oResult );
}  

TALKBACK_ERR CImsHelper::TalkBackGetNumWords(
	TALKBACK_ANALYSIS *iAnalysis, 
	long              *oResult)
{
	return ::TalkBackGetNumWords( iAnalysis, oResult );
}  

TALKBACK_ERR CImsHelper::TalkBackGetWord(
	TALKBACK_ANALYSIS *iAnalysis, 
	long               iWordNum,  
	long               iMaxChars, 
	char              *oWord)
{
	return ::TalkBackGetWord( iAnalysis, iWordNum, iMaxChars, oWord );
}    

TALKBACK_ERR CImsHelper::TalkBackGetWordStartTime(
	TALKBACK_ANALYSIS *iAnalysis, 
	long               iWordNum,  
	double            *oResult)
{
	return ::TalkBackGetWordStartTime( iAnalysis, iWordNum, oResult );
}  

TALKBACK_ERR CImsHelper::TalkBackGetWordEndTime(
	TALKBACK_ANALYSIS *iAnalysis, 
	long               iWordNum,  
	double            *oResult)
{
	return ::TalkBackGetWordEndTime( iAnalysis, iWordNum, oResult );
}  

TALKBACK_ERR CImsHelper::TalkBackInsertPhoneme(
	TALKBACK_ANALYSIS *iAnalysis,      
	TALKBACK_PHONEME   iPhoneme,       
	long               iInsertPosition,
	int                iInsertBefore)
{
	return ::TalkBackInsertPhoneme( iAnalysis, iPhoneme, iInsertPosition, iInsertBefore );
}

TALKBACK_ERR CImsHelper::TalkBackDeletePhoneme(
	TALKBACK_ANALYSIS *iAnalysis,        
	long               iPhonemeToDelete)
{
	return ::TalkBackDeletePhoneme( iAnalysis, iPhonemeToDelete );
}

TALKBACK_ERR CImsHelper::TalkBackChangePhonemeStart(
	TALKBACK_ANALYSIS *iAnalysis,        
	long               iPhonemeToChange, 
	double            *ioNewTime)
{
	return ::TalkBackChangePhonemeStart( iAnalysis, iPhonemeToChange, ioNewTime );
}     

TALKBACK_ERR CImsHelper::TalkBackChangePhonemeEnd(
	TALKBACK_ANALYSIS *iAnalysis,        
	long               iPhonemeToChange, 
	double            *ioNewTime)
{
	return ::TalkBackChangePhonemeEnd( iAnalysis, iPhonemeToChange, ioNewTime );
}       

TALKBACK_ERR CImsHelper::TalkBackChangePhonemeEnum(
	TALKBACK_ANALYSIS *iAnalysis,        
	long               iPhonemeToChange, 
	TALKBACK_PHONEME   iNewPhoneme)
{
	return ::TalkBackChangePhonemeEnum( iAnalysis, iPhonemeToChange, iNewPhoneme);
}   
