//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef IMS_HELPER_H
#define IMS_HELPER_H

#ifdef _WIN32
#pragma once
#endif

#include "TalkBack.h"

class IImsHelper
{
public:
	// Must be the first function called when using TalkBack.
	virtual TALKBACK_ERR TalkBackStartupLibrary(
		char const *iCoreDataDir) = 0; // IN:      full path of folder containing TalkBack data files.

	// Should be the last function called when using TalkBack.
	virtual TALKBACK_ERR               // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackShutdownLibrary() = 0; // IN:      nothing.

	// ------------------
	// Version functions.
	// ------------------

	// Gets the TalkBack version number.
	virtual TALKBACK_ERR          // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetVersion(
		long *oMajor,     // OUT:     major version number.
		long *oMinor,     // OUT:     minor version number.
		long *oRevision) = 0; // OUT:     revision version number.

	// Gets the TalkBack version number as a string.
	virtual TALKBACK_ERR         // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetVersionString(
		long  iMaxChars, // IN:      size of version string buffer.
		char *oVersion) = 0; // OUT:     version string buffer.

	// ------------------
	// Utility functions.
	// ------------------

	// Checks whether a sound file can be analyzed and returns some quality metrics.
	//
	// NOTE: this function is deprecated and has been supplanted by
	// TalkBackGetSoundFileMetrics().
	virtual TALKBACK_ERR                    // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackCheckSoundFile(
		char const *iSoundFileName, // IN:      name of sound file to be checked.
		long       *oCanBeAnalyzed, // OUT:     1 if sound can be analyzed, 0 if not.
		long       *oIsClipped,     // OUT:     1 if sound is clipped, 0 if not.
		double     *oDecibelRange) = 0; // OUT:     used decibel range of sound.

	// Returns metrics for the specified sound file.
	virtual TALKBACK_ERR                                     // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetSoundFileMetrics(
		char const                  *iSoundFileName, // IN:      name of sound file to be checked.
		TALKBACK_SOUND_FILE_METRICS *ioMetrics) = 0;     // IN/OUT:  address of a structure where the metrics will be stored.

	// Checks whether text can be used for text-based analysis, returning the text
	// as it will be analyzed.
	virtual TALKBACK_ERR                    // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackCheckSpokenText(
		char const *iSpokenText,    // IN:      text to check.
		long        iMaxChars,      // IN:      size of analyzed text buffer.
		char       *oAnalyzedText) = 0; // OUT:     buffer for text as it will be analyzed.

	// Convert a TalkBack error code to a description string.
	virtual TALKBACK_ERR                    // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetErrorString(
		TALKBACK_ERR iErrorCode,    // IN:      TalkBack error code to convert.
		long         iMaxChars,     // IN:      size of the buffer.
		char        *oErrorString) = 0; // OUT:     buffer for the description string.

	// Gets the error code and text for the most recent TalkBack error.
	virtual TALKBACK_ERR                     // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetLastError(
		long          iMaxChars,    // IN:      size of the buffer.
		char         *oErrorString, // OUT:     buffer for the description string.
		TALKBACK_ERR *oErrorCode) = 0;  // OUT:     most recent TalkBack error code.

	// -------------------
	// Analysis functions.
	// -------------------

	// Gets an opaque TALKBACK_ANALYSIS object. This object is then queried with the
	// TalkBackGet* functions below.
	virtual TALKBACK_ERR                                    // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetAnalysis(
		TALKBACK_ANALYSIS         **ioAnalysis,     // IN/OUT:  address of a TALKBACK_ANALYSIS *variable where analysis will be stored.
		char const                 *iSoundFileName, // IN:      name of the sound file to analyze.
		char const                 *iSoundText,     // IN:      text spoken in sound file (can be NULL to use textless analysis).
		TALKBACK_ANALYSIS_SETTINGS *iSettings) = 0;     // IN:      pointer to a TALKBACK_ANALYSIS_SETTINGS structure (can be NULL for defaults).

	// Frees an opaque TALKBACK_ANALYSIS object. This releases all memory used by
	// the analysis.
	virtual TALKBACK_ERR                         // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackFreeAnalysis(
		TALKBACK_ANALYSIS **ioAnalysis) = 0; // IN/OUT:  analysis to free.

	// #######################################################################
	// NOTE: all functions from this point on require a valid analysis object.
	// #######################################################################

	// ------------------------
	// Speech target functions.
	// ------------------------

	// Gets the number of speech target tracks.
	virtual TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetNumSpeechTargetTracks(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long              *oResult) = 0;  // OUT:     number of speech target tracks.

	// Gets the number of keys in the specified speech target track.
	virtual TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetNumSpeechTargetKeys(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum, // IN:      speech target track.
		long              *oResult) = 0;  // OUT:     number of keys in the speech target track.

	// Gets key information (time, value, derivative in, and derivative out) for the
	// specified key in the specified speech target track.
	virtual TALKBACK_ERR                            // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetSpeechTargetKeyInfo(
		TALKBACK_ANALYSIS *iAnalysis,       // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum,       // IN:      speech target track.
		long               iKeyNum,         // IN:      speech target key.
		double            *oTime,           // OUT:     time of key.
		double            *oValue,          // OUT:     value of key.
		double            *oDerivativeIn,   // OUT:     incoming derivative of key.
		double            *oDerivativeOut) = 0; // OUT:     outgoing derivative of key.

	// Gets the value of the function curve for the specified speech target track at
	// the specified time.
	virtual TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetSpeechTargetValueAtTime(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum, // IN:      speech target track.
		double             iTime,     // IN:      time in seconds.
		double            *oResult) = 0;  // OUT:     value of the function curve.

	// Gets the derivatives of the function curve for the specified speech target
	// track at the specified time.
	virtual TALKBACK_ERR                            // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetSpeechTargetDerivativesAtTime(
		TALKBACK_ANALYSIS *iAnalysis,       // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum,       // IN:      speech target track.
		double             iTime,           // IN:      time in seconds.
		double            *oDerivativeIn,   // OUT:     value of the incoming derivative of the function curve.
		double            *oDerivativeOut) = 0; // OUT:     value of the outgoing derivative of the function curve.

	// -------------------------
	// Speech gesture functions.
	// -------------------------

	// Gets the number of speech gesture tracks.
	virtual TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetNumGestureTracks(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long              *oResult) = 0;  // OUT:     number of speech gesture tracks

	// Gets the number of keys in the specified speech gesture track.
	virtual TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetNumGestureKeys(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum, // IN:      speech gesture track.
		long              *oResult) = 0;  // OUT:     number of keys in the speech gesture track.

	// Gets key information (time, value, derivative in, and derivative out) for the
	// specified key in the specified speech gesture track.
	virtual TALKBACK_ERR                            // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetGestureKeyInfo(
		TALKBACK_ANALYSIS *iAnalysis,       // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum,       // IN:      speech gesture track.
		long               iKeyNum,         // IN:      speech gesture key.
		double            *oTime,           // OUT:     time of key.
		double            *oValue,          // OUT:     value of key.
		double            *oDerivativeIn,   // OUT:     incoming derivative of key.
		double            *oDerivativeOut) = 0; // OUT:     outgoing derivative of key.

	// Gets the value of the function curve for the specified speech gesture track
	// at the specified time.
	virtual TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetGestureValueAtTime(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum, // IN:      speech gesture track.
		double             iTime,     // IN:      time in seconds.
		double            *oResult) = 0;  // OUT:     value of the function curve.

	// Gets the derivatives of the function curve for the specified speech gesture
	// track at the specified time.
	virtual TALKBACK_ERR                            // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetGestureDerivativesAtTime(
		TALKBACK_ANALYSIS *iAnalysis,       // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum,       // IN:      speech gesture track.
		double             iTime,           // IN:      time in seconds.
		double            *oDerivativeIn,   // OUT:     value of the incoming derivative of the function curve.
		double            *oDerivativeOut) = 0; // OUT:     value of the outgoing derivative of the function curve.

	// ----------------
	// Frame functions.
	// ----------------

	// NOTE: these functions use the frame rate specified in the
	// TALKBACK_ANALYSIS_SETTINGS structure passed to TalkBackGetAnalysis() and
	// default to 30 fps (TALKBACK_DEFAULT_FRAME_RATE) if the structure pointer was
	// NULL.

	// Gets the first frame number.
	virtual TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetFirstFrameNum(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long              *oResult) = 0;  // OUT:     number of the first frame.

	// Gets the last frame number.
	virtual TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetLastFrameNum(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long              *oResult) = 0;  // OUT:     number of the last frame.

	// Gets the start time of the specified frame.
	virtual TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetFrameStartTime(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iFrameNum, // IN:      frame.
		double            *oResult) = 0;  // OUT:     start time of the frame in seconds.

	// Gets the end time of the specified frame.
	virtual TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetFrameEndTime(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iFrameNum, // IN:      frame.
		double            *oResult) = 0;  // OUT:     end time of the frame in seconds.

	// Gets the value of the function curve for a speech target integrated over the
	// specified frame.
	virtual TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetSpeechTargetValueAtFrame(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum, // IN:      speech target track.
		long               iFrameNum, // IN:      frame number.
		double            *oResult) = 0;  // OUT:     value of the function curve integrated over the frame.

	// Gets the dominant speech target at the specified frame.
	//
	// NOTE: this function is meant to be used in flipbook mode only.
	virtual TALKBACK_ERR                                // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetDominantSpeechTargetAtFrame(
		TALKBACK_ANALYSIS      *iAnalysis,      // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long                    iFrameNum,      // IN:      frame number.
		TALKBACK_SPEECH_TARGET *oSpeechTarget) = 0; // OUT:     dominant speech target.

	// Gets the value of the function curve for a speech gesture integrated over the
	// specified frame.
	virtual TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetGestureValueAtFrame(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iTrackNum, // IN:      speech gesture track.
		long               iFrameNum, // IN:      frame number.
		double            *oResult) = 0;  // OUT:     value of the function curve integrated over the frame.

	// ------------------
	// Phoneme functions.
	// ------------------

	// Gets the number of phonemes.
	virtual TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetNumPhonemes(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long              *oResult) = 0;  // OUT:     number of phonemes.

	// Gets the enumeration of the specified phoneme.
	virtual TALKBACK_ERR                        // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetPhonemeEnum(
		TALKBACK_ANALYSIS *iAnalysis,   // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iPhonemeNum, // IN:      phoneme.
		TALKBACK_PHONEME  *oResult) = 0;    // OUT:     enumeration of the specified phoneme.

	// Gets the start time of the specified phoneme.
	virtual TALKBACK_ERR                        // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetPhonemeStartTime(
		TALKBACK_ANALYSIS *iAnalysis,   // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iPhonemeNum, // IN:      phoneme.
		double            *oResult) = 0;    // OUT:     start time of the phoneme in seconds.

	// Gets the end time of the specified phoneme.
	virtual TALKBACK_ERR                        // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetPhonemeEndTime(
		TALKBACK_ANALYSIS *iAnalysis,   // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iPhonemeNum, // IN:      phoneme.
		double            *oResult) = 0;    // OUT:     end time of the phoneme in seconds.

	// ---------------
	// Word functions.
	// ---------------

	// NOTE: these functions only yield data for text-based analysis.

	// Gets the number of words.
	virtual TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetNumWords(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long              *oResult) = 0;  // OUT:     number of words.

	// Gets the text of the specified word.
	virtual TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetWord(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iWordNum,  // IN:      word.
		long               iMaxChars, // IN:      size of word buffer.
		char              *oWord) = 0;    // OUT:     word buffer.

	// Gets the start time of the specified word.
	virtual TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetWordStartTime(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iWordNum,  // IN:      word.
		double            *oResult) = 0;  // OUT:     start time of the word in seconds.

	// Gets the end time of the specified word.
	virtual TALKBACK_ERR                      // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackGetWordEndTime(
		TALKBACK_ANALYSIS *iAnalysis, // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iWordNum,  // IN:      word.
		double            *oResult) = 0;  // OUT:     end time of the word in seconds.

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
	virtual TALKBACK_ERR                            // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackInsertPhoneme(
		TALKBACK_ANALYSIS *iAnalysis,       // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		TALKBACK_PHONEME   iPhoneme,        // IN:      enumeration of phoneme to insert.
		long               iInsertPosition, // IN:      position (phoneme number) at which to insert.
		int                iInsertBefore) = 0;  // IN:      manner of insertion:
											//            0 means put phoneme after insert position;
											//            1 means put phoneme before insert position.

	// Deletes the specified phoneme.
	virtual TALKBACK_ERR                              // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackDeletePhoneme(
		TALKBACK_ANALYSIS *iAnalysis,         // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iPhonemeToDelete) = 0; // IN:      phoneme to delete.

	// Changes the start time of the specified phoneme.
	//
	// NOTE: the start time specified may not be the actual start time for a number
	// of reasons, most notably if the specified start time will make the phoneme
	// too short. This function returns the actual start time so the caller can
	// check the result without having to query the phoneme.
	virtual TALKBACK_ERR                             // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackChangePhonemeStart(
		TALKBACK_ANALYSIS *iAnalysis,        // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iPhonemeToChange, // IN:      phoneme to change.
		double            *ioNewTime) = 0;       // IN/OUT:  new start time value in seconds (in) = 0; actual start time (out).

	// Changes the end time of the specified phoneme.
	//
	// NOTE: the end time specified may not be the actual end time for a number of
	// reasons, most notably if the specified end time will make the phoneme too
	// short. This function returns the actual end time so the caller can check the
	// result without having to query the phoneme.
	virtual TALKBACK_ERR                             // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackChangePhonemeEnd(
		TALKBACK_ANALYSIS *iAnalysis,        // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iPhonemeToChange, // IN:      phoneme to change.
		double            *ioNewTime) = 0;       // IN/OUT:  new end time value in seconds (in) = 0; actual end time (out).

	// Changes the enumeration of the specified phoneme.
	virtual TALKBACK_ERR                             // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
	TalkBackChangePhonemeEnum(
		TALKBACK_ANALYSIS *iAnalysis,        // IN:      opaque analysis object returned by TalkBackGetAnalysis().
		long               iPhonemeToChange, // IN:      phoneme to change.
		TALKBACK_PHONEME   iNewPhoneme) = 0;     // IN:      new phoneme enumeration.
};

// Expose to loading code
extern "C" {

#if defined( IMS_HELPER_EXPORTS )
__declspec( dllexport ) 
#endif
IImsHelper *GetImsHelper();
};

#endif // !IMS_HELPER_H
