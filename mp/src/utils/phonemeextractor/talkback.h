//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// =============================================================================
// Interface to the LIPSinc TalkBack 1.1 library (TalkBack_*.lib).
//
// Copyright © 1998-2002 LIPSinc. All rights reserved.

#if !defined(TalkBack_h)
#define TalkBack_h

#include <stddef.h> // size_t.

// Enforce a C API.
#if defined(__cplusplus)
extern "C"
{
#endif

// -----------------------------------------------------------------------------
// Use the preprocessor to make the new API compatible with the old one.

#define TalkbackStartupLibrary                   TalkBackStartupLibrary
#define TalkbackShutdownLibrary                  TalkBackShutdownLibrary
#define TalkbackGetVersion                       TalkBackGetVersion
#define TalkbackGetVersionString                 TalkBackGetVersionString
#define TalkbackCheckSoundFile                   TalkBackCheckSoundFile
#define TalkbackCheckSpokenText                  TalkBackCheckSpokenText
#define TalkbackGetErrorString                   TalkBackGetErrorString
#define TalkbackGetAnalysis                      TalkBackGetAnalysis
#define TalkbackFreeAnalysis                     TalkBackFreeAnalysis
#define TalkbackGetFirstFrameNum                 TalkBackGetFirstFrameNum
#define TalkbackGetLastFrameNum                  TalkBackGetLastFrameNum
#define TalkbackGetFrameStartTime                TalkBackGetFrameStartTime
#define TalkbackGetFrameEndTime                  TalkBackGetFrameEndTime
#define TalkbackGetNumPhonemes                   TalkBackGetNumPhonemes
#define TalkbackGetPhonemeEnum                   TalkBackGetPhonemeEnum
#define TalkbackGetPhonemeStartTime              TalkBackGetPhonemeStartTime
#define TalkbackGetPhonemeEndTime                TalkBackGetPhonemeEndTime
#define TalkbackInsertPhoneme                    TalkBackInsertPhoneme
#define TalkbackDeletePhoneme                    TalkBackDeletePhoneme
#define TalkbackChangePhonemeStart               TalkBackChangePhonemeStart
#define TalkbackChangePhonemeEnd                 TalkBackChangePhonemeEnd
#define TalkbackChangePhonemeEnum                TalkBackChangePhonemeEnum
#define TalkbackGetNumWords                      TalkBackGetNumWords
#define TalkbackGetWord                          TalkBackGetWord
#define TalkbackGetWordStartTime                 TalkBackGetWordStartTime
#define TalkbackGetWordEndTime                   TalkBackGetWordEndTime
#define TalkbackGetNumSpeechTargetTracks         TalkBackGetNumSpeechTargetTracks
#define TalkbackGetNumSpeechTargetKeys           TalkBackGetNumSpeechTargetKeys
#define TalkbackGetSpeechTargetKeyInfo           TalkBackGetSpeechTargetKeyInfo
#define TalkbackGetSpeechTargetValueAtFrame      TalkBackGetSpeechTargetValueAtFrame
#define TalkbackGetDominantSpeechTargetAtFrame   TalkBackGetDominantSpeechTargetAtFrame
#define TalkbackGetSpeechTargetValueAtTime       TalkBackGetSpeechTargetValueAtTime
#define TalkbackGetSpeechTargetDerivativesAtTime TalkBackGetSpeechTargetDerivativesAtTime
#define TalkbackGetNumGestureTracks              TalkBackGetNumGestureTracks
#define TalkbackGetNumGestureKeys                TalkBackGetNumGestureKeys
#define TalkbackGetGestureKeyInfo                TalkBackGetGestureKeyInfo
#define TalkbackGetGestureValueAtFrame           TalkBackGetGestureValueAtFrame
#define TalkbackGetGestureValueAtTime            TalkBackGetGestureValueAtTime
#define TalkbackGetGestureDerivativesAtTime      TalkBackGetGestureDerivativesAtTime

// -----------------------------------------------------------------------------
// For debug builds, set this to a non-zero value to get verbose debugging
// output from TalkBack.

extern int g_enableTalkBackDebuggingOutput;

// -----------------------------------------------------------------------------
// Miscellaneous constants.

// For calling TalkBackGetAnalysis() with all defaults.
#define TALKBACK_DEFAULT_SETTINGS          NULL

// For setting the iSoundText parameter in TalkBackGetAnalysis() to "no text."
#define TALKBACK_NO_TEXT                   NULL

// Handy constants for TALKBACK_ANALYSIS_SETTINGS fields:

                                           // For setting fSize.
#define TALKBACK_SETTINGS_SIZE             sizeof(TALKBACK_ANALYSIS_SETTINGS)
                                           // For setting fFrameRate to the
                                           // default.
#define TALKBACK_DEFAULT_FRAME_RATE        30
                                           // For setting fOptimizeForFlipbook
                                           // to *not* optimize for flipbook.
#define TALKBACK_OPTIMIZE_FOR_FLIPBOOK_OFF 0
                                           // For setting fOptimizeForFlipbook
                                           // to optimize for flipbook.
#define TALKBACK_OPTIMIZE_FOR_FLIPBOOK_ON  1
                                           // For setting fRandomSeed to use the
                                           // current time to seed the random
                                           // number generator and thereby get
                                           // non-deterministic speech gestures.
#define TALKBACK_RANDOM_SEED               -1
                                           // For setting fConfigFile to "no
                                           // config file."
#define TALKBACK_NO_CONFIG_FILE            NULL

// -----------------------------------------------------------------------------
// Data types.

// TALKBACK_NOERR if successful, TalkBack error code if not.
typedef long TALKBACK_ERR;

// Opaque analysis results.
typedef void TALKBACK_ANALYSIS;

// Speech target.
typedef long TALKBACK_SPEECH_TARGET;

// Speech gesture.
typedef long TALKBACK_GESTURE;

// Phoneme.
typedef long TALKBACK_PHONEME;

// -----------------------------------------------------------------------------
// Data structures.

#pragma pack(push, 1)

// Optional analysis settings passed to TalkBackGetAnalysis().
typedef struct
{
    // Set this field to sizeof(TALKBACK_ANALYSIS_SETTINGS) before using the
    // structure.
    long        fSize;
    // Frame rate for analysis. This only matters if you will be using *AtFrame
    // functions.
    //
    // Default value: 30 (frames per second).
    long        fFrameRate;
    // Set this to 1 to optimize for flipbook output, 0 to do analysis normally.
    //
    // Default value: 0 (normal analysis).
    long        fOptimizeForFlipbook;
    // Set this to -1 to seed the random number generator with the current time.
    // Any other number will be used directly for the random number seed, which
    // is useful if you want repeatable speech gestures. This value does not
    // influence lip-synching at all.
    //
    // Default value: -1 (use current time).
    long        fRandomSeed;
    // Path to the configuration (.INI) file with phoneme-to-speech-target
    // mapping. Set this to NULL to use the default mapping.
    //
    // Default value: NULL (use default mapping).
    char const *fConfigFile;
} TALKBACK_ANALYSIS_SETTINGS;

typedef struct
{
    // Set this field to sizeof(TALKBACK_SOUND_FILE_METRICS) before using the
    // structure. This will allow the structure to evolve if necessary.
    size_t     m_size;
    // Bits per sample.
    long       m_bitsPerSample;
    // Sample rate in Hertz.
    long       m_sampleRate;
    // Duration of the audio in seconds.
    double     m_duration;
    // 1 if the sound file can be analyzed, 0 if not.
    long       m_canBeAnalyzed;
    // 1 if the sound file is clipped, 0 if not.
    long       m_isClipped;
    // The decibel range of the sound file.
    double     m_decibelRange;
    // A quality value for the sound file: the nominal range is 0 to 100. Try
    // to keep it above 45 for good results.
    int        m_quality;

    // Added for version 2 of the metrics structure:
    // ---------------------------------------------
    // The number of channels in the sound file: 1 for mono, 2 for stereo, etc.
    long       m_channelCount;
} TALKBACK_SOUND_FILE_METRICS;

#pragma pack(pop)

// -----------------------------------------------------------------------------
// Constants.

// TalkBack error codes. Use TalkBackGetErrorString() to return text
// descriptions for these codes.
enum
{
    // Windows convention: set this bit to indicate an application-defined error
    // code.
    BIT29 = (1 << 29),
    // Success (not an error).
    TALKBACK_NOERR = 0,
    // The first error code: useful for iterating through the error codes.
    TALKBACK_ERROR_FIRST = 4201 | BIT29,
    // Generic error.
    TALKBACK_ERROR = TALKBACK_ERROR_FIRST,
    // TalkBackStartupLibrary() failed [internal error] or was never called.
    TALKBACK_STARTUP_FAILED_ERR,
    // TalkBackShutdownLibrary() failed, either because
    // TalkBackStartupLibrary() was never called or because
    // TalkBackShutdownLibrary() has already been called.
    TALKBACK_SHUTDOWN_FAILED_ERR,
    // The TalkBack data files could not be found [invalid path or missing
    // files].
    TALKBACK_CORE_DATA_NOT_FOUND_ERR,
    // One or more of the parameters are NULL.
    TALKBACK_NULL_PARAMETER_ERR,
    // One or more of the parameters is invalid.
    TALKBACK_INVALID_PARAMETER_ERR,
    // The analysis object pointer is invalid.
    TALKBACK_INVALID_ANALYSIS_ERR,
    // Analysis failed [the sound file cannot be analyzed or an internal error
    // occurred].
    TALKBACK_ANALYSIS_FAILED_ERR,
    // One or more of the indices (track, key, frame, word, phoneme) are
    // invalid (out of range).
    TALKBACK_INVALID_INDEX_ERR,
    // The time parameter is invalid (out of range).
    TALKBACK_INVALID_TIME_ERR,
    // A serious internal error occurred in TalkBack; please alert LIPSinc by
    // sending mail with a description of how the error was triggered to
    // talkback-support@LIPSinc.com.
    TALKBACK_INTERNAL_ERR,
    // Could not open the specified sound file.
    TALKBACK_COULD_NOT_LOAD_SOUND_ERR,
    // TalkBackStartupLibrary() has not been called.
    TALKBACK_STARTUP_NOT_CALLED,
    // The configuration file specified in the TALKBACK_ANALYSIS_SETTINGS
    // structure is invalid.
    TALKBACK_CONFIG_PARSE_ERROR,
    // The last error code: useful for iterating through the error codes.
    TALKBACK_ERROR_LAST = TALKBACK_CONFIG_PARSE_ERROR
};

// Default lip-synching track identifiers.
//
// NOTE: these track identifiers apply *only* to the default phoneme-to-track
//       mapping! Consult the TalkBack Reference Guide for more details.
//
// NOTE: these values are valid *only* if you use the default mapping and are
//       provided as a convenience. If you use your own mapping, these values
//       are invalid and should not be used.

enum
{
    TALKBACK_SPEECH_TARGET_INVALID = -1,
    TALKBACK_SPEECH_TARGET_FIRST = 0,
    TALKBACK_SPEECH_TARGET_EAT = TALKBACK_SPEECH_TARGET_FIRST, // 0
    TALKBACK_SPEECH_TARGET_EARTH,                              // 1
    TALKBACK_SPEECH_TARGET_IF,                                 // 2
    TALKBACK_SPEECH_TARGET_OX,                                 // 3
    TALKBACK_SPEECH_TARGET_OAT,                                // 4
    TALKBACK_SPEECH_TARGET_WET,                                // 5
    TALKBACK_SPEECH_TARGET_SIZE,                               // 6
    TALKBACK_SPEECH_TARGET_CHURCH,                             // 7
    TALKBACK_SPEECH_TARGET_FAVE,                               // 8
    TALKBACK_SPEECH_TARGET_THOUGH,                             // 9
    TALKBACK_SPEECH_TARGET_TOLD,                               // 10
    TALKBACK_SPEECH_TARGET_BUMP,                               // 11
    TALKBACK_SPEECH_TARGET_NEW,                                // 12
    TALKBACK_SPEECH_TARGET_ROAR,                               // 13
    TALKBACK_SPEECH_TARGET_CAGE,                               // 14
    TALKBACK_SPEECH_TARGET_LAST = TALKBACK_SPEECH_TARGET_CAGE, // 14
    TALKBACK_NUM_SPEECH_TARGETS                                // 15 (0..14)
};

// Speech gesture track identifiers.

enum
{
    TALKBACK_GESTURE_INVALID = -1,
    TALKBACK_GESTURE_FIRST = 0,
    TALKBACK_GESTURE_EYEBROW_RAISE_LEFT = TALKBACK_GESTURE_FIRST, // 0
    TALKBACK_GESTURE_EYEBROW_RAISE_RIGHT,                         // 1
    TALKBACK_GESTURE_BLINK_LEFT,                                  // 2
    TALKBACK_GESTURE_BLINK_RIGHT,                                 // 3
    TALKBACK_GESTURE_HEAD_BEND,                                   // 4
    TALKBACK_GESTURE_HEAD_SIDE_SIDE,                              // 5
    TALKBACK_GESTURE_HEAD_TWIST,                                  // 6
    TALKBACK_GESTURE_EYE_SIDE_SIDE_LEFT,                          // 7
    TALKBACK_GESTURE_EYE_SIDE_SIDE_RIGHT,                         // 8
    TALKBACK_GESTURE_EYE_UP_DOWN_LEFT,                            // 9
    TALKBACK_GESTURE_EYE_UP_DOWN_RIGHT,                           // 10
    TALKBACK_GESTURE_LAST = TALKBACK_GESTURE_EYE_UP_DOWN_RIGHT,   // 10
    TALKBACK_NUM_GESTURES                                         // 11 (0..10)
};

// Phoneme identifiers.

enum
{
    TALKBACK_PHONEME_INVALID = -1,
    TALKBACK_PHONEME_FIRST = 0,
    TALKBACK_PHONEME_IY = TALKBACK_PHONEME_FIRST, // 0
    TALKBACK_PHONEME_IH,                          // 1
    TALKBACK_PHONEME_EH,                          // 2
    TALKBACK_PHONEME_EY,                          // 3
    TALKBACK_PHONEME_AE,                          // 4
    TALKBACK_PHONEME_AA,                          // 5
    TALKBACK_PHONEME_AW,                          // 6
    TALKBACK_PHONEME_AY,                          // 7
    TALKBACK_PHONEME_AH,                          // 8
    TALKBACK_PHONEME_AO,                          // 9
    TALKBACK_PHONEME_OY,                          // 10
    TALKBACK_PHONEME_OW,                          // 11
    TALKBACK_PHONEME_UH,                          // 12
    TALKBACK_PHONEME_UW,                          // 13
    TALKBACK_PHONEME_ER,                          // 14
    TALKBACK_PHONEME_AX,                          // 15
    TALKBACK_PHONEME_S,                           // 16
    TALKBACK_PHONEME_SH,                          // 17
    TALKBACK_PHONEME_Z,                           // 18
    TALKBACK_PHONEME_ZH,                          // 19
    TALKBACK_PHONEME_F,                           // 20
    TALKBACK_PHONEME_TH,                          // 21
    TALKBACK_PHONEME_V,                           // 22
    TALKBACK_PHONEME_DH,                          // 23
    TALKBACK_PHONEME_M,                           // 24
    TALKBACK_PHONEME_N,                           // 25
    TALKBACK_PHONEME_NG,                          // 26
    TALKBACK_PHONEME_L,                           // 27
    TALKBACK_PHONEME_R,                           // 28
    TALKBACK_PHONEME_W,                           // 29
    TALKBACK_PHONEME_Y,                           // 30
    TALKBACK_PHONEME_HH,                          // 31
    TALKBACK_PHONEME_B,                           // 32
    TALKBACK_PHONEME_D,                           // 33
    TALKBACK_PHONEME_JH,                          // 34
    TALKBACK_PHONEME_G,                           // 35
    TALKBACK_PHONEME_P,                           // 36
    TALKBACK_PHONEME_T,                           // 37
    TALKBACK_PHONEME_K,                           // 38
    TALKBACK_PHONEME_CH,                          // 39
    TALKBACK_PHONEME_SIL,                         // 40
    TALKBACK_PHONEME_LAST = TALKBACK_PHONEME_SIL, // 40
    TALKBACK_NUM_PHONEMES                         // 41 (0..40)
};

// -----------------------------------------------------------------------------
// Function declarations.

// ---------------------------
// Startup/shutdown functions.
// ---------------------------

// Must be the first function called when using TalkBack.
TALKBACK_ERR                   // RETURNS: TALKBACK_NOERR if successful, TalkBack error code if not.
TalkBackStartupLibrary(
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

#if defined(__cplusplus)
}
#endif

#endif
