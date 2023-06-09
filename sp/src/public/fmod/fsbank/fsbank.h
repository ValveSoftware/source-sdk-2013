#ifndef _FSBANK_H
#define _FSBANK_H

#if defined(_WIN32)
    #define FB_EXPORT __declspec(dllexport)
    #define FB_CALL __stdcall
#else
    #define FB_EXPORT __attribute__((visibility("default")))
    #define FB_CALL
#endif

#if defined(DLL_EXPORTS)
    #define FB_API FB_EXPORT FB_CALL
#else
    #define FB_API FB_CALL
#endif

/*
    FSBank types
*/
typedef unsigned int FSBANK_INITFLAGS;
typedef unsigned int FSBANK_BUILDFLAGS;

#define FSBANK_INIT_NORMAL                  0x00000000
#define FSBANK_INIT_IGNOREERRORS            0x00000001
#define FSBANK_INIT_WARNINGSASERRORS        0x00000002
#define FSBANK_INIT_CREATEINCLUDEHEADER     0x00000004
#define FSBANK_INIT_DONTLOADCACHEFILES      0x00000008
#define FSBANK_INIT_GENERATEPROGRESSITEMS   0x00000010

#define FSBANK_BUILD_DEFAULT                0x00000000
#define FSBANK_BUILD_DISABLESYNCPOINTS      0x00000001
#define FSBANK_BUILD_DONTLOOP               0x00000002
#define FSBANK_BUILD_FILTERHIGHFREQ         0x00000004
#define FSBANK_BUILD_DISABLESEEKING         0x00000008
#define FSBANK_BUILD_OPTIMIZESAMPLERATE     0x00000010
#define FSBANK_BUILD_FSB5_DONTWRITENAMES    0x00000080
#define FSBANK_BUILD_NOGUID                 0x00000100
#define FSBANK_BUILD_WRITEPEAKVOLUME        0x00000200

#define FSBANK_BUILD_OVERRIDE_MASK          (FSBANK_BUILD_DISABLESYNCPOINTS | FSBANK_BUILD_DONTLOOP | FSBANK_BUILD_FILTERHIGHFREQ | FSBANK_BUILD_DISABLESEEKING | FSBANK_BUILD_OPTIMIZESAMPLERATE | FSBANK_BUILD_WRITEPEAKVOLUME)
#define FSBANK_BUILD_CACHE_VALIDATION_MASK  (FSBANK_BUILD_DONTLOOP | FSBANK_BUILD_FILTERHIGHFREQ | FSBANK_BUILD_OPTIMIZESAMPLERATE)

typedef enum FSBANK_RESULT
{
    FSBANK_OK,
    FSBANK_ERR_CACHE_CHUNKNOTFOUND,
    FSBANK_ERR_CANCELLED,
    FSBANK_ERR_CANNOT_CONTINUE,
    FSBANK_ERR_ENCODER,
    FSBANK_ERR_ENCODER_INIT,
    FSBANK_ERR_ENCODER_NOTSUPPORTED,
    FSBANK_ERR_FILE_OS,
    FSBANK_ERR_FILE_NOTFOUND,
    FSBANK_ERR_FMOD,
    FSBANK_ERR_INITIALIZED,
    FSBANK_ERR_INVALID_FORMAT,
    FSBANK_ERR_INVALID_PARAM,
    FSBANK_ERR_MEMORY,
    FSBANK_ERR_UNINITIALIZED,
    FSBANK_ERR_WRITER_FORMAT,
    FSBANK_WARN_CANNOTLOOP,
    FSBANK_WARN_IGNORED_FILTERHIGHFREQ,
    FSBANK_WARN_IGNORED_DISABLESEEKING,
    FSBANK_WARN_FORCED_DONTWRITENAMES,
    FSBANK_ERR_ENCODER_FILE_NOTFOUND,
    FSBANK_ERR_ENCODER_FILE_BAD,
} FSBANK_RESULT;

typedef enum FSBANK_FORMAT
{
    FSBANK_FORMAT_PCM,
    FSBANK_FORMAT_XMA,
    FSBANK_FORMAT_AT9,
    FSBANK_FORMAT_VORBIS,
    FSBANK_FORMAT_FADPCM,
    FSBANK_FORMAT_OPUS,

    FSBANK_FORMAT_MAX
} FSBANK_FORMAT;

typedef enum FSBANK_FSBVERSION
{
    FSBANK_FSBVERSION_FSB5,

    FSBANK_FSBVERSION_MAX
} FSBANK_FSBVERSION;

typedef enum FSBANK_STATE
{
    FSBANK_STATE_DECODING,
    FSBANK_STATE_ANALYSING,
    FSBANK_STATE_PREPROCESSING,
    FSBANK_STATE_ENCODING,
    FSBANK_STATE_WRITING,
    FSBANK_STATE_FINISHED,
    FSBANK_STATE_FAILED,
    FSBANK_STATE_WARNING,
} FSBANK_STATE;

typedef struct FSBANK_SUBSOUND
{
    const char* const  *fileNames;
    const void* const  *fileData;
    const unsigned int *fileDataLengths;
    unsigned int        numFiles;
    FSBANK_BUILDFLAGS   overrideFlags;
    unsigned int        overrideQuality;
    float               desiredSampleRate;
    float               percentOptimizedRate;
} FSBANK_SUBSOUND;

typedef struct FSBANK_PROGRESSITEM
{
    int             subSoundIndex;
    int             threadIndex;
    FSBANK_STATE    state;
    const void     *stateData;
} FSBANK_PROGRESSITEM;

typedef struct FSBANK_STATEDATA_FAILED
{
    FSBANK_RESULT errorCode;
    char errorString[256];
} FSBANK_STATEDATA_FAILED;

typedef struct FSBANK_STATEDATA_WARNING
{
    FSBANK_RESULT warnCode;
    char warningString[256];
} FSBANK_STATEDATA_WARNING;


#ifdef __cplusplus
extern "C" {
#endif

typedef void* (FB_CALL *FSBANK_MEMORY_ALLOC_CALLBACK)(unsigned int size, unsigned int type, const char *sourceStr);
typedef void* (FB_CALL *FSBANK_MEMORY_REALLOC_CALLBACK)(void *ptr, unsigned int size, unsigned int type, const char *sourceStr);
typedef void  (FB_CALL *FSBANK_MEMORY_FREE_CALLBACK)(void *ptr, unsigned int type, const char *sourceStr);

FSBANK_RESULT FB_API FSBank_MemoryInit(FSBANK_MEMORY_ALLOC_CALLBACK userAlloc, FSBANK_MEMORY_REALLOC_CALLBACK userRealloc, FSBANK_MEMORY_FREE_CALLBACK userFree);
FSBANK_RESULT FB_API FSBank_Init(FSBANK_FSBVERSION version, FSBANK_INITFLAGS flags, unsigned int numSimultaneousJobs, const char *cacheDirectory);
FSBANK_RESULT FB_API FSBank_Release();
FSBANK_RESULT FB_API FSBank_Build(const FSBANK_SUBSOUND *subSounds, unsigned int numSubSounds, FSBANK_FORMAT encodeFormat, FSBANK_BUILDFLAGS buildFlags, unsigned int quality, const char *encryptKey, const char *outputFileName);
FSBANK_RESULT FB_API FSBank_FetchFSBMemory(const void **data, unsigned int *length);
FSBANK_RESULT FB_API FSBank_BuildCancel();
FSBANK_RESULT FB_API FSBank_FetchNextProgressItem(const FSBANK_PROGRESSITEM **progressItem);
FSBANK_RESULT FB_API FSBank_ReleaseProgressItem(const FSBANK_PROGRESSITEM *progressItem);
FSBANK_RESULT FB_API FSBank_MemoryGetStats(unsigned int *currentAllocated, unsigned int *maximumAllocated);

#ifdef __cplusplus
}
#endif

#endif  // _FSBANK_H
