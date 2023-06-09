/* ======================================================================================== */
/* FMOD Core API - Common C/C++ header file.                                                */
/* Copyright (c), Firelight Technologies Pty, Ltd. 2004-2023.                               */
/*                                                                                          */
/* This header is included by fmod.hpp (C++ interface) and fmod.h (C interface)             */
/*                                                                                          */
/* For more detail visit:                                                                   */
/* https://fmod.com/docs/2.02/api/core-api-common.html                                      */
/* ======================================================================================== */
#ifndef _FMOD_COMMON_H
#define _FMOD_COMMON_H

/*
    Library import helpers
*/
#if defined(_WIN32) || defined(__CYGWIN__)
    #define F_CALL __stdcall
#else
    #define F_CALL
#endif

#if defined(_WIN32) || defined(__CYGWIN__) || defined(__ORBIS__) || defined(F_USE_DECLSPEC)
    #define F_EXPORT __declspec(dllexport)
#elif defined(__APPLE__) || defined(__ANDROID__) || defined(__linux__) || defined(F_USE_ATTRIBUTE)
    #define F_EXPORT __attribute__((visibility("default")))
#else
    #define F_EXPORT
#endif

#ifdef DLL_EXPORTS
    #define F_API F_EXPORT F_CALL
#else
    #define F_API F_CALL
#endif

#define F_CALLBACK F_CALL

/*
    FMOD core types
*/
typedef int                        FMOD_BOOL;
typedef struct FMOD_SYSTEM         FMOD_SYSTEM;
typedef struct FMOD_SOUND          FMOD_SOUND;
typedef struct FMOD_CHANNELCONTROL FMOD_CHANNELCONTROL;
typedef struct FMOD_CHANNEL        FMOD_CHANNEL;
typedef struct FMOD_CHANNELGROUP   FMOD_CHANNELGROUP;
typedef struct FMOD_SOUNDGROUP     FMOD_SOUNDGROUP;
typedef struct FMOD_REVERB3D       FMOD_REVERB3D;
typedef struct FMOD_DSP            FMOD_DSP;
typedef struct FMOD_DSPCONNECTION  FMOD_DSPCONNECTION;
typedef struct FMOD_POLYGON        FMOD_POLYGON;
typedef struct FMOD_GEOMETRY       FMOD_GEOMETRY;
typedef struct FMOD_SYNCPOINT      FMOD_SYNCPOINT;
typedef struct FMOD_ASYNCREADINFO  FMOD_ASYNCREADINFO;

/*
    FMOD constants
*/
#define FMOD_VERSION    0x00020214                     /* 0xaaaabbcc -> aaaa = product version, bb = major version, cc = minor version.*/

typedef unsigned int FMOD_DEBUG_FLAGS;
#define FMOD_DEBUG_LEVEL_NONE                       0x00000000
#define FMOD_DEBUG_LEVEL_ERROR                      0x00000001
#define FMOD_DEBUG_LEVEL_WARNING                    0x00000002
#define FMOD_DEBUG_LEVEL_LOG                        0x00000004
#define FMOD_DEBUG_TYPE_MEMORY                      0x00000100
#define FMOD_DEBUG_TYPE_FILE                        0x00000200
#define FMOD_DEBUG_TYPE_CODEC                       0x00000400
#define FMOD_DEBUG_TYPE_TRACE                       0x00000800
#define FMOD_DEBUG_DISPLAY_TIMESTAMPS               0x00010000
#define FMOD_DEBUG_DISPLAY_LINENUMBERS              0x00020000
#define FMOD_DEBUG_DISPLAY_THREAD                   0x00040000

typedef unsigned int FMOD_MEMORY_TYPE;
#define FMOD_MEMORY_NORMAL                          0x00000000
#define FMOD_MEMORY_STREAM_FILE                     0x00000001
#define FMOD_MEMORY_STREAM_DECODE                   0x00000002
#define FMOD_MEMORY_SAMPLEDATA                      0x00000004
#define FMOD_MEMORY_DSP_BUFFER                      0x00000008
#define FMOD_MEMORY_PLUGIN                          0x00000010
#define FMOD_MEMORY_PERSISTENT                      0x00200000
#define FMOD_MEMORY_ALL                             0xFFFFFFFF

typedef unsigned int FMOD_INITFLAGS;
#define FMOD_INIT_NORMAL                            0x00000000
#define FMOD_INIT_STREAM_FROM_UPDATE                0x00000001
#define FMOD_INIT_MIX_FROM_UPDATE                   0x00000002
#define FMOD_INIT_3D_RIGHTHANDED                    0x00000004
#define FMOD_INIT_CLIP_OUTPUT                       0x00000008
#define FMOD_INIT_CHANNEL_LOWPASS                   0x00000100
#define FMOD_INIT_CHANNEL_DISTANCEFILTER            0x00000200
#define FMOD_INIT_PROFILE_ENABLE                    0x00010000
#define FMOD_INIT_VOL0_BECOMES_VIRTUAL              0x00020000
#define FMOD_INIT_GEOMETRY_USECLOSEST               0x00040000
#define FMOD_INIT_PREFER_DOLBY_DOWNMIX              0x00080000
#define FMOD_INIT_THREAD_UNSAFE                     0x00100000
#define FMOD_INIT_PROFILE_METER_ALL                 0x00200000
#define FMOD_INIT_MEMORY_TRACKING                   0x00400000

typedef unsigned int FMOD_DRIVER_STATE;
#define FMOD_DRIVER_STATE_CONNECTED                 0x00000001
#define FMOD_DRIVER_STATE_DEFAULT                   0x00000002

typedef unsigned int FMOD_TIMEUNIT;
#define FMOD_TIMEUNIT_MS                            0x00000001
#define FMOD_TIMEUNIT_PCM                           0x00000002
#define FMOD_TIMEUNIT_PCMBYTES                      0x00000004
#define FMOD_TIMEUNIT_RAWBYTES                      0x00000008
#define FMOD_TIMEUNIT_PCMFRACTION                   0x00000010
#define FMOD_TIMEUNIT_MODORDER                      0x00000100
#define FMOD_TIMEUNIT_MODROW                        0x00000200
#define FMOD_TIMEUNIT_MODPATTERN                    0x00000400

typedef unsigned int FMOD_SYSTEM_CALLBACK_TYPE;
#define FMOD_SYSTEM_CALLBACK_DEVICELISTCHANGED      0x00000001
#define FMOD_SYSTEM_CALLBACK_DEVICELOST             0x00000002
#define FMOD_SYSTEM_CALLBACK_MEMORYALLOCATIONFAILED 0x00000004
#define FMOD_SYSTEM_CALLBACK_THREADCREATED          0x00000008
#define FMOD_SYSTEM_CALLBACK_BADDSPCONNECTION       0x00000010
#define FMOD_SYSTEM_CALLBACK_PREMIX                 0x00000020
#define FMOD_SYSTEM_CALLBACK_POSTMIX                0x00000040
#define FMOD_SYSTEM_CALLBACK_ERROR                  0x00000080
#define FMOD_SYSTEM_CALLBACK_MIDMIX                 0x00000100
#define FMOD_SYSTEM_CALLBACK_THREADDESTROYED        0x00000200
#define FMOD_SYSTEM_CALLBACK_PREUPDATE              0x00000400
#define FMOD_SYSTEM_CALLBACK_POSTUPDATE             0x00000800
#define FMOD_SYSTEM_CALLBACK_RECORDLISTCHANGED      0x00001000
#define FMOD_SYSTEM_CALLBACK_BUFFEREDNOMIX          0x00002000
#define FMOD_SYSTEM_CALLBACK_DEVICEREINITIALIZE     0x00004000
#define FMOD_SYSTEM_CALLBACK_OUTPUTUNDERRUN         0x00008000
#define FMOD_SYSTEM_CALLBACK_RECORDPOSITIONCHANGED  0x00010000
#define FMOD_SYSTEM_CALLBACK_ALL                    0xFFFFFFFF

typedef unsigned int FMOD_MODE;
#define FMOD_DEFAULT                                0x00000000
#define FMOD_LOOP_OFF                               0x00000001
#define FMOD_LOOP_NORMAL                            0x00000002
#define FMOD_LOOP_BIDI                              0x00000004
#define FMOD_2D                                     0x00000008
#define FMOD_3D                                     0x00000010
#define FMOD_CREATESTREAM                           0x00000080
#define FMOD_CREATESAMPLE                           0x00000100
#define FMOD_CREATECOMPRESSEDSAMPLE                 0x00000200
#define FMOD_OPENUSER                               0x00000400
#define FMOD_OPENMEMORY                             0x00000800
#define FMOD_OPENMEMORY_POINT                       0x10000000
#define FMOD_OPENRAW                                0x00001000
#define FMOD_OPENONLY                               0x00002000
#define FMOD_ACCURATETIME                           0x00004000
#define FMOD_MPEGSEARCH                             0x00008000
#define FMOD_NONBLOCKING                            0x00010000
#define FMOD_UNIQUE                                 0x00020000
#define FMOD_3D_HEADRELATIVE                        0x00040000
#define FMOD_3D_WORLDRELATIVE                       0x00080000
#define FMOD_3D_INVERSEROLLOFF                      0x00100000
#define FMOD_3D_LINEARROLLOFF                       0x00200000
#define FMOD_3D_LINEARSQUAREROLLOFF                 0x00400000
#define FMOD_3D_INVERSETAPEREDROLLOFF               0x00800000
#define FMOD_3D_CUSTOMROLLOFF                       0x04000000
#define FMOD_3D_IGNOREGEOMETRY                      0x40000000
#define FMOD_IGNORETAGS                             0x02000000
#define FMOD_LOWMEM                                 0x08000000
#define FMOD_VIRTUAL_PLAYFROMSTART                  0x80000000

typedef unsigned int FMOD_CHANNELMASK;
#define FMOD_CHANNELMASK_FRONT_LEFT                 0x00000001
#define FMOD_CHANNELMASK_FRONT_RIGHT                0x00000002
#define FMOD_CHANNELMASK_FRONT_CENTER               0x00000004
#define FMOD_CHANNELMASK_LOW_FREQUENCY              0x00000008
#define FMOD_CHANNELMASK_SURROUND_LEFT              0x00000010
#define FMOD_CHANNELMASK_SURROUND_RIGHT             0x00000020
#define FMOD_CHANNELMASK_BACK_LEFT                  0x00000040
#define FMOD_CHANNELMASK_BACK_RIGHT                 0x00000080
#define FMOD_CHANNELMASK_BACK_CENTER                0x00000100
#define FMOD_CHANNELMASK_MONO                       (FMOD_CHANNELMASK_FRONT_LEFT)
#define FMOD_CHANNELMASK_STEREO                     (FMOD_CHANNELMASK_FRONT_LEFT | FMOD_CHANNELMASK_FRONT_RIGHT)
#define FMOD_CHANNELMASK_LRC                        (FMOD_CHANNELMASK_FRONT_LEFT | FMOD_CHANNELMASK_FRONT_RIGHT | FMOD_CHANNELMASK_FRONT_CENTER)
#define FMOD_CHANNELMASK_QUAD                       (FMOD_CHANNELMASK_FRONT_LEFT | FMOD_CHANNELMASK_FRONT_RIGHT | FMOD_CHANNELMASK_SURROUND_LEFT | FMOD_CHANNELMASK_SURROUND_RIGHT)
#define FMOD_CHANNELMASK_SURROUND                   (FMOD_CHANNELMASK_FRONT_LEFT | FMOD_CHANNELMASK_FRONT_RIGHT | FMOD_CHANNELMASK_FRONT_CENTER  | FMOD_CHANNELMASK_SURROUND_LEFT | FMOD_CHANNELMASK_SURROUND_RIGHT)
#define FMOD_CHANNELMASK_5POINT1                    (FMOD_CHANNELMASK_FRONT_LEFT | FMOD_CHANNELMASK_FRONT_RIGHT | FMOD_CHANNELMASK_FRONT_CENTER  | FMOD_CHANNELMASK_LOW_FREQUENCY | FMOD_CHANNELMASK_SURROUND_LEFT  | FMOD_CHANNELMASK_SURROUND_RIGHT)
#define FMOD_CHANNELMASK_5POINT1_REARS              (FMOD_CHANNELMASK_FRONT_LEFT | FMOD_CHANNELMASK_FRONT_RIGHT | FMOD_CHANNELMASK_FRONT_CENTER  | FMOD_CHANNELMASK_LOW_FREQUENCY | FMOD_CHANNELMASK_BACK_LEFT      | FMOD_CHANNELMASK_BACK_RIGHT)
#define FMOD_CHANNELMASK_7POINT0                    (FMOD_CHANNELMASK_FRONT_LEFT | FMOD_CHANNELMASK_FRONT_RIGHT | FMOD_CHANNELMASK_FRONT_CENTER  | FMOD_CHANNELMASK_SURROUND_LEFT | FMOD_CHANNELMASK_SURROUND_RIGHT | FMOD_CHANNELMASK_BACK_LEFT      | FMOD_CHANNELMASK_BACK_RIGHT)
#define FMOD_CHANNELMASK_7POINT1                    (FMOD_CHANNELMASK_FRONT_LEFT | FMOD_CHANNELMASK_FRONT_RIGHT | FMOD_CHANNELMASK_FRONT_CENTER  | FMOD_CHANNELMASK_LOW_FREQUENCY | FMOD_CHANNELMASK_SURROUND_LEFT  | FMOD_CHANNELMASK_SURROUND_RIGHT | FMOD_CHANNELMASK_BACK_LEFT | FMOD_CHANNELMASK_BACK_RIGHT)

typedef unsigned long long FMOD_PORT_INDEX;
#define FMOD_PORT_INDEX_NONE                        0xFFFFFFFFFFFFFFFF
#define FMOD_PORT_INDEX_FLAG_VR_CONTROLLER          0x1000000000000000

typedef int FMOD_THREAD_PRIORITY;
/* Platform specific priority range */
#define FMOD_THREAD_PRIORITY_PLATFORM_MIN           (-32 * 1024)
#define FMOD_THREAD_PRIORITY_PLATFORM_MAX           ( 32 * 1024)
/* Platform agnostic priorities, maps internally to platform specific value */
#define FMOD_THREAD_PRIORITY_DEFAULT                (FMOD_THREAD_PRIORITY_PLATFORM_MIN - 1)
#define FMOD_THREAD_PRIORITY_LOW                    (FMOD_THREAD_PRIORITY_PLATFORM_MIN - 2)
#define FMOD_THREAD_PRIORITY_MEDIUM                 (FMOD_THREAD_PRIORITY_PLATFORM_MIN - 3)
#define FMOD_THREAD_PRIORITY_HIGH                   (FMOD_THREAD_PRIORITY_PLATFORM_MIN - 4)
#define FMOD_THREAD_PRIORITY_VERY_HIGH              (FMOD_THREAD_PRIORITY_PLATFORM_MIN - 5)
#define FMOD_THREAD_PRIORITY_EXTREME                (FMOD_THREAD_PRIORITY_PLATFORM_MIN - 6)
#define FMOD_THREAD_PRIORITY_CRITICAL               (FMOD_THREAD_PRIORITY_PLATFORM_MIN - 7)
/* Thread defaults */
#define FMOD_THREAD_PRIORITY_MIXER                  FMOD_THREAD_PRIORITY_EXTREME
#define FMOD_THREAD_PRIORITY_FEEDER                 FMOD_THREAD_PRIORITY_CRITICAL
#define FMOD_THREAD_PRIORITY_STREAM                 FMOD_THREAD_PRIORITY_VERY_HIGH
#define FMOD_THREAD_PRIORITY_FILE                   FMOD_THREAD_PRIORITY_HIGH
#define FMOD_THREAD_PRIORITY_NONBLOCKING            FMOD_THREAD_PRIORITY_HIGH
#define FMOD_THREAD_PRIORITY_RECORD                 FMOD_THREAD_PRIORITY_HIGH
#define FMOD_THREAD_PRIORITY_GEOMETRY               FMOD_THREAD_PRIORITY_LOW
#define FMOD_THREAD_PRIORITY_PROFILER               FMOD_THREAD_PRIORITY_MEDIUM
#define FMOD_THREAD_PRIORITY_STUDIO_UPDATE          FMOD_THREAD_PRIORITY_MEDIUM
#define FMOD_THREAD_PRIORITY_STUDIO_LOAD_BANK       FMOD_THREAD_PRIORITY_MEDIUM
#define FMOD_THREAD_PRIORITY_STUDIO_LOAD_SAMPLE     FMOD_THREAD_PRIORITY_MEDIUM
#define FMOD_THREAD_PRIORITY_CONVOLUTION1           FMOD_THREAD_PRIORITY_VERY_HIGH
#define FMOD_THREAD_PRIORITY_CONVOLUTION2           FMOD_THREAD_PRIORITY_VERY_HIGH

typedef unsigned int FMOD_THREAD_STACK_SIZE;
#define FMOD_THREAD_STACK_SIZE_DEFAULT              0
#define FMOD_THREAD_STACK_SIZE_MIXER                (80  * 1024)
#define FMOD_THREAD_STACK_SIZE_FEEDER               (16  * 1024)
#define FMOD_THREAD_STACK_SIZE_STREAM               (96  * 1024)
#define FMOD_THREAD_STACK_SIZE_FILE                 (64  * 1024)
#define FMOD_THREAD_STACK_SIZE_NONBLOCKING          (112 * 1024)
#define FMOD_THREAD_STACK_SIZE_RECORD               (16  * 1024)
#define FMOD_THREAD_STACK_SIZE_GEOMETRY             (48  * 1024)
#define FMOD_THREAD_STACK_SIZE_PROFILER             (128 * 1024)
#define FMOD_THREAD_STACK_SIZE_STUDIO_UPDATE        (96  * 1024)
#define FMOD_THREAD_STACK_SIZE_STUDIO_LOAD_BANK     (96  * 1024)
#define FMOD_THREAD_STACK_SIZE_STUDIO_LOAD_SAMPLE   (96  * 1024)
#define FMOD_THREAD_STACK_SIZE_CONVOLUTION1         (16  * 1024)
#define FMOD_THREAD_STACK_SIZE_CONVOLUTION2         (16  * 1024)

typedef long long FMOD_THREAD_AFFINITY;
/* Platform agnostic thread groupings */
#define FMOD_THREAD_AFFINITY_GROUP_DEFAULT          0x4000000000000000
#define FMOD_THREAD_AFFINITY_GROUP_A                0x4000000000000001
#define FMOD_THREAD_AFFINITY_GROUP_B                0x4000000000000002
#define FMOD_THREAD_AFFINITY_GROUP_C                0x4000000000000003
/* Thread defaults */
#define FMOD_THREAD_AFFINITY_MIXER                  FMOD_THREAD_AFFINITY_GROUP_A
#define FMOD_THREAD_AFFINITY_FEEDER                 FMOD_THREAD_AFFINITY_GROUP_C
#define FMOD_THREAD_AFFINITY_STREAM                 FMOD_THREAD_AFFINITY_GROUP_C
#define FMOD_THREAD_AFFINITY_FILE                   FMOD_THREAD_AFFINITY_GROUP_C
#define FMOD_THREAD_AFFINITY_NONBLOCKING            FMOD_THREAD_AFFINITY_GROUP_C
#define FMOD_THREAD_AFFINITY_RECORD                 FMOD_THREAD_AFFINITY_GROUP_C
#define FMOD_THREAD_AFFINITY_GEOMETRY               FMOD_THREAD_AFFINITY_GROUP_C
#define FMOD_THREAD_AFFINITY_PROFILER               FMOD_THREAD_AFFINITY_GROUP_C
#define FMOD_THREAD_AFFINITY_STUDIO_UPDATE          FMOD_THREAD_AFFINITY_GROUP_B
#define FMOD_THREAD_AFFINITY_STUDIO_LOAD_BANK       FMOD_THREAD_AFFINITY_GROUP_C
#define FMOD_THREAD_AFFINITY_STUDIO_LOAD_SAMPLE     FMOD_THREAD_AFFINITY_GROUP_C
#define FMOD_THREAD_AFFINITY_CONVOLUTION1           FMOD_THREAD_AFFINITY_GROUP_C
#define FMOD_THREAD_AFFINITY_CONVOLUTION2           FMOD_THREAD_AFFINITY_GROUP_C
/* Core mask, valid up to 1 << 62 */
#define FMOD_THREAD_AFFINITY_CORE_ALL               0
#define FMOD_THREAD_AFFINITY_CORE_0                 (1 << 0)
#define FMOD_THREAD_AFFINITY_CORE_1                 (1 << 1)
#define FMOD_THREAD_AFFINITY_CORE_2                 (1 << 2)
#define FMOD_THREAD_AFFINITY_CORE_3                 (1 << 3)
#define FMOD_THREAD_AFFINITY_CORE_4                 (1 << 4)
#define FMOD_THREAD_AFFINITY_CORE_5                 (1 << 5)
#define FMOD_THREAD_AFFINITY_CORE_6                 (1 << 6)
#define FMOD_THREAD_AFFINITY_CORE_7                 (1 << 7)
#define FMOD_THREAD_AFFINITY_CORE_8                 (1 << 8)
#define FMOD_THREAD_AFFINITY_CORE_9                 (1 << 9)
#define FMOD_THREAD_AFFINITY_CORE_10                (1 << 10)
#define FMOD_THREAD_AFFINITY_CORE_11                (1 << 11)
#define FMOD_THREAD_AFFINITY_CORE_12                (1 << 12)
#define FMOD_THREAD_AFFINITY_CORE_13                (1 << 13)
#define FMOD_THREAD_AFFINITY_CORE_14                (1 << 14)
#define FMOD_THREAD_AFFINITY_CORE_15                (1 << 15)

/* Preset for FMOD_REVERB_PROPERTIES */
#define FMOD_PRESET_OFF                             {  1000,    7,  11, 5000, 100, 100, 100, 250, 0,    20,  96, -80.0f }
#define FMOD_PRESET_GENERIC                         {  1500,    7,  11, 5000,  83, 100, 100, 250, 0, 14500,  96,  -8.0f }
#define FMOD_PRESET_PADDEDCELL                      {   170,    1,   2, 5000,  10, 100, 100, 250, 0,   160,  84,  -7.8f }
#define FMOD_PRESET_ROOM                            {   400,    2,   3, 5000,  83, 100, 100, 250, 0,  6050,  88,  -9.4f }
#define FMOD_PRESET_BATHROOM                        {  1500,    7,  11, 5000,  54, 100,  60, 250, 0,  2900,  83,   0.5f }
#define FMOD_PRESET_LIVINGROOM                      {   500,    3,   4, 5000,  10, 100, 100, 250, 0,   160,  58, -19.0f }
#define FMOD_PRESET_STONEROOM                       {  2300,   12,  17, 5000,  64, 100, 100, 250, 0,  7800,  71,  -8.5f }
#define FMOD_PRESET_AUDITORIUM                      {  4300,   20,  30, 5000,  59, 100, 100, 250, 0,  5850,  64, -11.7f }
#define FMOD_PRESET_CONCERTHALL                     {  3900,   20,  29, 5000,  70, 100, 100, 250, 0,  5650,  80,  -9.8f }
#define FMOD_PRESET_CAVE                            {  2900,   15,  22, 5000, 100, 100, 100, 250, 0, 20000,  59, -11.3f }
#define FMOD_PRESET_ARENA                           {  7200,   20,  30, 5000,  33, 100, 100, 250, 0,  4500,  80,  -9.6f }
#define FMOD_PRESET_HANGAR                          { 10000,   20,  30, 5000,  23, 100, 100, 250, 0,  3400,  72,  -7.4f }
#define FMOD_PRESET_CARPETTEDHALLWAY                {   300,    2,  30, 5000,  10, 100, 100, 250, 0,   500,  56, -24.0f }
#define FMOD_PRESET_HALLWAY                         {  1500,    7,  11, 5000,  59, 100, 100, 250, 0,  7800,  87,  -5.5f }
#define FMOD_PRESET_STONECORRIDOR                   {   270,   13,  20, 5000,  79, 100, 100, 250, 0,  9000,  86,  -6.0f }
#define FMOD_PRESET_ALLEY                           {  1500,    7,  11, 5000,  86, 100, 100, 250, 0,  8300,  80,  -9.8f }
#define FMOD_PRESET_FOREST                          {  1500,  162,  88, 5000,  54,  79, 100, 250, 0,   760,  94, -12.3f }
#define FMOD_PRESET_CITY                            {  1500,    7,  11, 5000,  67,  50, 100, 250, 0,  4050,  66, -26.0f }
#define FMOD_PRESET_MOUNTAINS                       {  1500,  300, 100, 5000,  21,  27, 100, 250, 0,  1220,  82, -24.0f }
#define FMOD_PRESET_QUARRY                          {  1500,   61,  25, 5000,  83, 100, 100, 250, 0,  3400, 100,  -5.0f }
#define FMOD_PRESET_PLAIN                           {  1500,  179, 100, 5000,  50,  21, 100, 250, 0,  1670,  65, -28.0f }
#define FMOD_PRESET_PARKINGLOT                      {  1700,    8,  12, 5000, 100, 100, 100, 250, 0, 20000,  56, -19.5f }
#define FMOD_PRESET_SEWERPIPE                       {  2800,   14,  21, 5000,  14,  80,  60, 250, 0,  3400,  66,   1.2f }
#define FMOD_PRESET_UNDERWATER                      {  1500,    7,  11, 5000,  10, 100, 100, 250, 0,   500,  92,   7.0f }

#define FMOD_MAX_CHANNEL_WIDTH                      32
#define FMOD_MAX_SYSTEMS                            8
#define FMOD_MAX_LISTENERS                          8
#define FMOD_REVERB_MAXINSTANCES                    4

typedef enum FMOD_THREAD_TYPE
{
    FMOD_THREAD_TYPE_MIXER,
    FMOD_THREAD_TYPE_FEEDER,
    FMOD_THREAD_TYPE_STREAM,
    FMOD_THREAD_TYPE_FILE,
    FMOD_THREAD_TYPE_NONBLOCKING,
    FMOD_THREAD_TYPE_RECORD,
    FMOD_THREAD_TYPE_GEOMETRY,
    FMOD_THREAD_TYPE_PROFILER,
    FMOD_THREAD_TYPE_STUDIO_UPDATE,
    FMOD_THREAD_TYPE_STUDIO_LOAD_BANK,
    FMOD_THREAD_TYPE_STUDIO_LOAD_SAMPLE,
    FMOD_THREAD_TYPE_CONVOLUTION1,
    FMOD_THREAD_TYPE_CONVOLUTION2,

    FMOD_THREAD_TYPE_MAX,
    FMOD_THREAD_TYPE_FORCEINT = 65536
} FMOD_THREAD_TYPE;

typedef enum FMOD_RESULT
{
    FMOD_OK,
    FMOD_ERR_BADCOMMAND,
    FMOD_ERR_CHANNEL_ALLOC,
    FMOD_ERR_CHANNEL_STOLEN,
    FMOD_ERR_DMA,
    FMOD_ERR_DSP_CONNECTION,
    FMOD_ERR_DSP_DONTPROCESS,
    FMOD_ERR_DSP_FORMAT,
    FMOD_ERR_DSP_INUSE,
    FMOD_ERR_DSP_NOTFOUND,
    FMOD_ERR_DSP_RESERVED,
    FMOD_ERR_DSP_SILENCE,
    FMOD_ERR_DSP_TYPE,
    FMOD_ERR_FILE_BAD,
    FMOD_ERR_FILE_COULDNOTSEEK,
    FMOD_ERR_FILE_DISKEJECTED,
    FMOD_ERR_FILE_EOF,
    FMOD_ERR_FILE_ENDOFDATA,
    FMOD_ERR_FILE_NOTFOUND,
    FMOD_ERR_FORMAT,
    FMOD_ERR_HEADER_MISMATCH,
    FMOD_ERR_HTTP,
    FMOD_ERR_HTTP_ACCESS,
    FMOD_ERR_HTTP_PROXY_AUTH,
    FMOD_ERR_HTTP_SERVER_ERROR,
    FMOD_ERR_HTTP_TIMEOUT,
    FMOD_ERR_INITIALIZATION,
    FMOD_ERR_INITIALIZED,
    FMOD_ERR_INTERNAL,
    FMOD_ERR_INVALID_FLOAT,
    FMOD_ERR_INVALID_HANDLE,
    FMOD_ERR_INVALID_PARAM,
    FMOD_ERR_INVALID_POSITION,
    FMOD_ERR_INVALID_SPEAKER,
    FMOD_ERR_INVALID_SYNCPOINT,
    FMOD_ERR_INVALID_THREAD,
    FMOD_ERR_INVALID_VECTOR,
    FMOD_ERR_MAXAUDIBLE,
    FMOD_ERR_MEMORY,
    FMOD_ERR_MEMORY_CANTPOINT,
    FMOD_ERR_NEEDS3D,
    FMOD_ERR_NEEDSHARDWARE,
    FMOD_ERR_NET_CONNECT,
    FMOD_ERR_NET_SOCKET_ERROR,
    FMOD_ERR_NET_URL,
    FMOD_ERR_NET_WOULD_BLOCK,
    FMOD_ERR_NOTREADY,
    FMOD_ERR_OUTPUT_ALLOCATED,
    FMOD_ERR_OUTPUT_CREATEBUFFER,
    FMOD_ERR_OUTPUT_DRIVERCALL,
    FMOD_ERR_OUTPUT_FORMAT,
    FMOD_ERR_OUTPUT_INIT,
    FMOD_ERR_OUTPUT_NODRIVERS,
    FMOD_ERR_PLUGIN,
    FMOD_ERR_PLUGIN_MISSING,
    FMOD_ERR_PLUGIN_RESOURCE,
    FMOD_ERR_PLUGIN_VERSION,
    FMOD_ERR_RECORD,
    FMOD_ERR_REVERB_CHANNELGROUP,
    FMOD_ERR_REVERB_INSTANCE,
    FMOD_ERR_SUBSOUNDS,
    FMOD_ERR_SUBSOUND_ALLOCATED,
    FMOD_ERR_SUBSOUND_CANTMOVE,
    FMOD_ERR_TAGNOTFOUND,
    FMOD_ERR_TOOMANYCHANNELS,
    FMOD_ERR_TRUNCATED,
    FMOD_ERR_UNIMPLEMENTED,
    FMOD_ERR_UNINITIALIZED,
    FMOD_ERR_UNSUPPORTED,
    FMOD_ERR_VERSION,
    FMOD_ERR_EVENT_ALREADY_LOADED,
    FMOD_ERR_EVENT_LIVEUPDATE_BUSY,
    FMOD_ERR_EVENT_LIVEUPDATE_MISMATCH,
    FMOD_ERR_EVENT_LIVEUPDATE_TIMEOUT,
    FMOD_ERR_EVENT_NOTFOUND,
    FMOD_ERR_STUDIO_UNINITIALIZED,
    FMOD_ERR_STUDIO_NOT_LOADED,
    FMOD_ERR_INVALID_STRING,
    FMOD_ERR_ALREADY_LOCKED,
    FMOD_ERR_NOT_LOCKED,
    FMOD_ERR_RECORD_DISCONNECTED,
    FMOD_ERR_TOOMANYSAMPLES,

    FMOD_RESULT_FORCEINT = 65536
} FMOD_RESULT;

typedef enum FMOD_CHANNELCONTROL_TYPE
{
    FMOD_CHANNELCONTROL_CHANNEL,
    FMOD_CHANNELCONTROL_CHANNELGROUP,

    FMOD_CHANNELCONTROL_MAX,
    FMOD_CHANNELCONTROL_FORCEINT = 65536
} FMOD_CHANNELCONTROL_TYPE;

typedef enum FMOD_OUTPUTTYPE
{
    FMOD_OUTPUTTYPE_AUTODETECT,
    FMOD_OUTPUTTYPE_UNKNOWN,
    FMOD_OUTPUTTYPE_NOSOUND,
    FMOD_OUTPUTTYPE_WAVWRITER,
    FMOD_OUTPUTTYPE_NOSOUND_NRT,
    FMOD_OUTPUTTYPE_WAVWRITER_NRT,
    FMOD_OUTPUTTYPE_WASAPI,
    FMOD_OUTPUTTYPE_ASIO,
    FMOD_OUTPUTTYPE_PULSEAUDIO,
    FMOD_OUTPUTTYPE_ALSA,
    FMOD_OUTPUTTYPE_COREAUDIO,
    FMOD_OUTPUTTYPE_AUDIOTRACK,
    FMOD_OUTPUTTYPE_OPENSL,
    FMOD_OUTPUTTYPE_AUDIOOUT,
    FMOD_OUTPUTTYPE_AUDIO3D,
    FMOD_OUTPUTTYPE_WEBAUDIO,
    FMOD_OUTPUTTYPE_NNAUDIO,
    FMOD_OUTPUTTYPE_WINSONIC,
    FMOD_OUTPUTTYPE_AAUDIO,
    FMOD_OUTPUTTYPE_AUDIOWORKLET,
    FMOD_OUTPUTTYPE_PHASE,

    FMOD_OUTPUTTYPE_MAX,
    FMOD_OUTPUTTYPE_FORCEINT = 65536
} FMOD_OUTPUTTYPE;

typedef enum FMOD_DEBUG_MODE
{
    FMOD_DEBUG_MODE_TTY,
    FMOD_DEBUG_MODE_FILE,
    FMOD_DEBUG_MODE_CALLBACK,

    FMOD_DEBUG_MODE_FORCEINT = 65536
} FMOD_DEBUG_MODE;

typedef enum FMOD_SPEAKERMODE
{
    FMOD_SPEAKERMODE_DEFAULT,
    FMOD_SPEAKERMODE_RAW,
    FMOD_SPEAKERMODE_MONO,
    FMOD_SPEAKERMODE_STEREO,
    FMOD_SPEAKERMODE_QUAD,
    FMOD_SPEAKERMODE_SURROUND,
    FMOD_SPEAKERMODE_5POINT1,
    FMOD_SPEAKERMODE_7POINT1,
    FMOD_SPEAKERMODE_7POINT1POINT4,

    FMOD_SPEAKERMODE_MAX,
    FMOD_SPEAKERMODE_FORCEINT = 65536
} FMOD_SPEAKERMODE;

typedef enum FMOD_SPEAKER
{
    FMOD_SPEAKER_NONE = -1,
    FMOD_SPEAKER_FRONT_LEFT = 0,
    FMOD_SPEAKER_FRONT_RIGHT,
    FMOD_SPEAKER_FRONT_CENTER,
    FMOD_SPEAKER_LOW_FREQUENCY,
    FMOD_SPEAKER_SURROUND_LEFT,
    FMOD_SPEAKER_SURROUND_RIGHT,
    FMOD_SPEAKER_BACK_LEFT,
    FMOD_SPEAKER_BACK_RIGHT,
    FMOD_SPEAKER_TOP_FRONT_LEFT,
    FMOD_SPEAKER_TOP_FRONT_RIGHT,
    FMOD_SPEAKER_TOP_BACK_LEFT,
    FMOD_SPEAKER_TOP_BACK_RIGHT,

    FMOD_SPEAKER_MAX,
    FMOD_SPEAKER_FORCEINT = 65536
} FMOD_SPEAKER;

typedef enum FMOD_CHANNELORDER
{
    FMOD_CHANNELORDER_DEFAULT,
    FMOD_CHANNELORDER_WAVEFORMAT,
    FMOD_CHANNELORDER_PROTOOLS,
    FMOD_CHANNELORDER_ALLMONO,
    FMOD_CHANNELORDER_ALLSTEREO,
    FMOD_CHANNELORDER_ALSA,

    FMOD_CHANNELORDER_MAX,
    FMOD_CHANNELORDER_FORCEINT = 65536
} FMOD_CHANNELORDER;

typedef enum FMOD_PLUGINTYPE
{
    FMOD_PLUGINTYPE_OUTPUT,
    FMOD_PLUGINTYPE_CODEC,
    FMOD_PLUGINTYPE_DSP,

    FMOD_PLUGINTYPE_MAX,
    FMOD_PLUGINTYPE_FORCEINT = 65536
} FMOD_PLUGINTYPE;

typedef enum FMOD_SOUND_TYPE
{
    FMOD_SOUND_TYPE_UNKNOWN,
    FMOD_SOUND_TYPE_AIFF,
    FMOD_SOUND_TYPE_ASF,
    FMOD_SOUND_TYPE_DLS,
    FMOD_SOUND_TYPE_FLAC,
    FMOD_SOUND_TYPE_FSB,
    FMOD_SOUND_TYPE_IT,
    FMOD_SOUND_TYPE_MIDI,
    FMOD_SOUND_TYPE_MOD,
    FMOD_SOUND_TYPE_MPEG,
    FMOD_SOUND_TYPE_OGGVORBIS,
    FMOD_SOUND_TYPE_PLAYLIST,
    FMOD_SOUND_TYPE_RAW,
    FMOD_SOUND_TYPE_S3M,
    FMOD_SOUND_TYPE_USER,
    FMOD_SOUND_TYPE_WAV,
    FMOD_SOUND_TYPE_XM,
    FMOD_SOUND_TYPE_XMA,
    FMOD_SOUND_TYPE_AUDIOQUEUE,
    FMOD_SOUND_TYPE_AT9,
    FMOD_SOUND_TYPE_VORBIS,
    FMOD_SOUND_TYPE_MEDIA_FOUNDATION,
    FMOD_SOUND_TYPE_MEDIACODEC,
    FMOD_SOUND_TYPE_FADPCM,
    FMOD_SOUND_TYPE_OPUS,

    FMOD_SOUND_TYPE_MAX,
    FMOD_SOUND_TYPE_FORCEINT = 65536
} FMOD_SOUND_TYPE;

typedef enum FMOD_SOUND_FORMAT
{
    FMOD_SOUND_FORMAT_NONE,
    FMOD_SOUND_FORMAT_PCM8,
    FMOD_SOUND_FORMAT_PCM16,
    FMOD_SOUND_FORMAT_PCM24,
    FMOD_SOUND_FORMAT_PCM32,
    FMOD_SOUND_FORMAT_PCMFLOAT,
    FMOD_SOUND_FORMAT_BITSTREAM,

    FMOD_SOUND_FORMAT_MAX,
    FMOD_SOUND_FORMAT_FORCEINT = 65536
} FMOD_SOUND_FORMAT;

typedef enum FMOD_OPENSTATE
{
    FMOD_OPENSTATE_READY,
    FMOD_OPENSTATE_LOADING,
    FMOD_OPENSTATE_ERROR,
    FMOD_OPENSTATE_CONNECTING,
    FMOD_OPENSTATE_BUFFERING,
    FMOD_OPENSTATE_SEEKING,
    FMOD_OPENSTATE_PLAYING,
    FMOD_OPENSTATE_SETPOSITION,

    FMOD_OPENSTATE_MAX,
    FMOD_OPENSTATE_FORCEINT = 65536
} FMOD_OPENSTATE;

typedef enum FMOD_SOUNDGROUP_BEHAVIOR
{
    FMOD_SOUNDGROUP_BEHAVIOR_FAIL,
    FMOD_SOUNDGROUP_BEHAVIOR_MUTE,
    FMOD_SOUNDGROUP_BEHAVIOR_STEALLOWEST,

    FMOD_SOUNDGROUP_BEHAVIOR_MAX,
    FMOD_SOUNDGROUP_BEHAVIOR_FORCEINT = 65536
} FMOD_SOUNDGROUP_BEHAVIOR;

typedef enum FMOD_CHANNELCONTROL_CALLBACK_TYPE
{
    FMOD_CHANNELCONTROL_CALLBACK_END,
    FMOD_CHANNELCONTROL_CALLBACK_VIRTUALVOICE,
    FMOD_CHANNELCONTROL_CALLBACK_SYNCPOINT,
    FMOD_CHANNELCONTROL_CALLBACK_OCCLUSION,

    FMOD_CHANNELCONTROL_CALLBACK_MAX,
    FMOD_CHANNELCONTROL_CALLBACK_FORCEINT = 65536
} FMOD_CHANNELCONTROL_CALLBACK_TYPE;

typedef enum FMOD_CHANNELCONTROL_DSP_INDEX
{
    FMOD_CHANNELCONTROL_DSP_HEAD     = -1,
    FMOD_CHANNELCONTROL_DSP_FADER    = -2,
    FMOD_CHANNELCONTROL_DSP_TAIL     = -3,

    FMOD_CHANNELCONTROL_DSP_FORCEINT = 65536
} FMOD_CHANNELCONTROL_DSP_INDEX;

typedef enum FMOD_ERRORCALLBACK_INSTANCETYPE
{
    FMOD_ERRORCALLBACK_INSTANCETYPE_NONE,
    FMOD_ERRORCALLBACK_INSTANCETYPE_SYSTEM,
    FMOD_ERRORCALLBACK_INSTANCETYPE_CHANNEL,
    FMOD_ERRORCALLBACK_INSTANCETYPE_CHANNELGROUP,
    FMOD_ERRORCALLBACK_INSTANCETYPE_CHANNELCONTROL,
    FMOD_ERRORCALLBACK_INSTANCETYPE_SOUND,
    FMOD_ERRORCALLBACK_INSTANCETYPE_SOUNDGROUP,
    FMOD_ERRORCALLBACK_INSTANCETYPE_DSP,
    FMOD_ERRORCALLBACK_INSTANCETYPE_DSPCONNECTION,
    FMOD_ERRORCALLBACK_INSTANCETYPE_GEOMETRY,
    FMOD_ERRORCALLBACK_INSTANCETYPE_REVERB3D,
    FMOD_ERRORCALLBACK_INSTANCETYPE_STUDIO_SYSTEM,
    FMOD_ERRORCALLBACK_INSTANCETYPE_STUDIO_EVENTDESCRIPTION,
    FMOD_ERRORCALLBACK_INSTANCETYPE_STUDIO_EVENTINSTANCE,
    FMOD_ERRORCALLBACK_INSTANCETYPE_STUDIO_PARAMETERINSTANCE,
    FMOD_ERRORCALLBACK_INSTANCETYPE_STUDIO_BUS,
    FMOD_ERRORCALLBACK_INSTANCETYPE_STUDIO_VCA,
    FMOD_ERRORCALLBACK_INSTANCETYPE_STUDIO_BANK,
    FMOD_ERRORCALLBACK_INSTANCETYPE_STUDIO_COMMANDREPLAY,

    FMOD_ERRORCALLBACK_INSTANCETYPE_FORCEINT = 65536
} FMOD_ERRORCALLBACK_INSTANCETYPE;

typedef enum FMOD_DSP_RESAMPLER
{
    FMOD_DSP_RESAMPLER_DEFAULT,
    FMOD_DSP_RESAMPLER_NOINTERP,
    FMOD_DSP_RESAMPLER_LINEAR,
    FMOD_DSP_RESAMPLER_CUBIC,
    FMOD_DSP_RESAMPLER_SPLINE,

    FMOD_DSP_RESAMPLER_MAX,
    FMOD_DSP_RESAMPLER_FORCEINT = 65536
} FMOD_DSP_RESAMPLER;

typedef enum FMOD_DSP_CALLBACK_TYPE
{
    FMOD_DSP_CALLBACK_DATAPARAMETERRELEASE,

    FMOD_DSP_CALLBACK_MAX,
    FMOD_DSP_CALLBACK_FORCEINT = 65536
} FMOD_DSP_CALLBACK_TYPE;

typedef enum FMOD_DSPCONNECTION_TYPE
{
    FMOD_DSPCONNECTION_TYPE_STANDARD,
    FMOD_DSPCONNECTION_TYPE_SIDECHAIN,
    FMOD_DSPCONNECTION_TYPE_SEND,
    FMOD_DSPCONNECTION_TYPE_SEND_SIDECHAIN,

    FMOD_DSPCONNECTION_TYPE_MAX,
    FMOD_DSPCONNECTION_TYPE_FORCEINT = 65536
} FMOD_DSPCONNECTION_TYPE;

typedef enum FMOD_TAGTYPE
{
    FMOD_TAGTYPE_UNKNOWN,
    FMOD_TAGTYPE_ID3V1,
    FMOD_TAGTYPE_ID3V2,
    FMOD_TAGTYPE_VORBISCOMMENT,
    FMOD_TAGTYPE_SHOUTCAST,
    FMOD_TAGTYPE_ICECAST,
    FMOD_TAGTYPE_ASF,
    FMOD_TAGTYPE_MIDI,
    FMOD_TAGTYPE_PLAYLIST,
    FMOD_TAGTYPE_FMOD,
    FMOD_TAGTYPE_USER,

    FMOD_TAGTYPE_MAX,
    FMOD_TAGTYPE_FORCEINT = 65536
} FMOD_TAGTYPE;

typedef enum FMOD_TAGDATATYPE
{
    FMOD_TAGDATATYPE_BINARY,
    FMOD_TAGDATATYPE_INT,
    FMOD_TAGDATATYPE_FLOAT,
    FMOD_TAGDATATYPE_STRING,
    FMOD_TAGDATATYPE_STRING_UTF16,
    FMOD_TAGDATATYPE_STRING_UTF16BE,
    FMOD_TAGDATATYPE_STRING_UTF8,

    FMOD_TAGDATATYPE_MAX,
    FMOD_TAGDATATYPE_FORCEINT = 65536
} FMOD_TAGDATATYPE;

typedef enum FMOD_PORT_TYPE
{
    FMOD_PORT_TYPE_MUSIC,
    FMOD_PORT_TYPE_COPYRIGHT_MUSIC,
    FMOD_PORT_TYPE_VOICE,
    FMOD_PORT_TYPE_CONTROLLER,
    FMOD_PORT_TYPE_PERSONAL,
    FMOD_PORT_TYPE_VIBRATION,
    FMOD_PORT_TYPE_AUX,

    FMOD_PORT_TYPE_MAX,
    FMOD_PORT_TYPE_FORCEINT = 65536
} FMOD_PORT_TYPE;

/*
    FMOD callbacks
*/
typedef FMOD_RESULT (F_CALL *FMOD_DEBUG_CALLBACK)           (FMOD_DEBUG_FLAGS flags, const char *file, int line, const char* func, const char* message);
typedef FMOD_RESULT (F_CALL *FMOD_SYSTEM_CALLBACK)          (FMOD_SYSTEM *system, FMOD_SYSTEM_CALLBACK_TYPE type, void *commanddata1, void* commanddata2, void *userdata);
typedef FMOD_RESULT (F_CALL *FMOD_CHANNELCONTROL_CALLBACK)  (FMOD_CHANNELCONTROL *channelcontrol, FMOD_CHANNELCONTROL_TYPE controltype, FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype, void *commanddata1, void *commanddata2);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_CALLBACK)             (FMOD_DSP *dsp, FMOD_DSP_CALLBACK_TYPE type, void *data);
typedef FMOD_RESULT (F_CALL *FMOD_SOUND_NONBLOCK_CALLBACK)  (FMOD_SOUND *sound, FMOD_RESULT result);
typedef FMOD_RESULT (F_CALL *FMOD_SOUND_PCMREAD_CALLBACK)   (FMOD_SOUND *sound, void *data, unsigned int datalen);
typedef FMOD_RESULT (F_CALL *FMOD_SOUND_PCMSETPOS_CALLBACK) (FMOD_SOUND *sound, int subsound, unsigned int position, FMOD_TIMEUNIT postype);
typedef FMOD_RESULT (F_CALL *FMOD_FILE_OPEN_CALLBACK)       (const char *name, unsigned int *filesize, void **handle, void *userdata);
typedef FMOD_RESULT (F_CALL *FMOD_FILE_CLOSE_CALLBACK)      (void *handle, void *userdata);
typedef FMOD_RESULT (F_CALL *FMOD_FILE_READ_CALLBACK)       (void *handle, void *buffer, unsigned int sizebytes, unsigned int *bytesread, void *userdata);
typedef FMOD_RESULT (F_CALL *FMOD_FILE_SEEK_CALLBACK)       (void *handle, unsigned int pos, void *userdata);
typedef FMOD_RESULT (F_CALL *FMOD_FILE_ASYNCREAD_CALLBACK)  (FMOD_ASYNCREADINFO *info, void *userdata);
typedef FMOD_RESULT (F_CALL *FMOD_FILE_ASYNCCANCEL_CALLBACK)(FMOD_ASYNCREADINFO *info, void *userdata);
typedef void        (F_CALL *FMOD_FILE_ASYNCDONE_FUNC)      (FMOD_ASYNCREADINFO *info, FMOD_RESULT result);
typedef void*       (F_CALL *FMOD_MEMORY_ALLOC_CALLBACK)    (unsigned int size, FMOD_MEMORY_TYPE type, const char *sourcestr);
typedef void*       (F_CALL *FMOD_MEMORY_REALLOC_CALLBACK)  (void *ptr, unsigned int size, FMOD_MEMORY_TYPE type, const char *sourcestr);
typedef void        (F_CALL *FMOD_MEMORY_FREE_CALLBACK)     (void *ptr, FMOD_MEMORY_TYPE type, const char *sourcestr);
typedef float       (F_CALL *FMOD_3D_ROLLOFF_CALLBACK)      (FMOD_CHANNELCONTROL *channelcontrol, float distance);

/*
    FMOD structs
*/
struct FMOD_ASYNCREADINFO
{
    void                     *handle;
    unsigned int              offset;
    unsigned int              sizebytes;
    int                       priority;
    void                     *userdata;
    void                     *buffer;
    unsigned int              bytesread;
    FMOD_FILE_ASYNCDONE_FUNC  done;
};

typedef struct FMOD_VECTOR
{
    float x;
    float y;
    float z;
} FMOD_VECTOR;

typedef struct FMOD_3D_ATTRIBUTES
{
    FMOD_VECTOR position;
    FMOD_VECTOR velocity;
    FMOD_VECTOR forward;
    FMOD_VECTOR up;
} FMOD_3D_ATTRIBUTES;

typedef struct FMOD_GUID
{
    unsigned int   Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
} FMOD_GUID;

typedef struct FMOD_PLUGINLIST
{
    FMOD_PLUGINTYPE  type;
    void            *description;
} FMOD_PLUGINLIST;

typedef struct FMOD_ADVANCEDSETTINGS
{
    int                 cbSize;
    int                 maxMPEGCodecs;
    int                 maxADPCMCodecs;
    int                 maxXMACodecs;
    int                 maxVorbisCodecs;
    int                 maxAT9Codecs;
    int                 maxFADPCMCodecs;
    int                 maxPCMCodecs;
    int                 ASIONumChannels;
    char              **ASIOChannelList;
    FMOD_SPEAKER       *ASIOSpeakerList;
    float               vol0virtualvol;
    unsigned int        defaultDecodeBufferSize;
    unsigned short      profilePort;
    unsigned int        geometryMaxFadeTime;
    float               distanceFilterCenterFreq;
    int                 reverb3Dinstance;
    int                 DSPBufferPoolSize;
    FMOD_DSP_RESAMPLER  resamplerMethod;
    unsigned int        randomSeed;
    int                 maxConvolutionThreads;
    int                 maxOpusCodecs;
} FMOD_ADVANCEDSETTINGS;

typedef struct FMOD_TAG
{
    FMOD_TAGTYPE      type;
    FMOD_TAGDATATYPE  datatype;
    char             *name;
    void             *data;
    unsigned int      datalen;
    FMOD_BOOL         updated;
} FMOD_TAG;

typedef struct FMOD_CREATESOUNDEXINFO
{
    int                            cbsize;
    unsigned int                   length;
    unsigned int                   fileoffset;
    int                            numchannels;
    int                            defaultfrequency;
    FMOD_SOUND_FORMAT              format;
    unsigned int                   decodebuffersize;
    int                            initialsubsound;
    int                            numsubsounds;
    int                           *inclusionlist;
    int                            inclusionlistnum;
    FMOD_SOUND_PCMREAD_CALLBACK    pcmreadcallback;
    FMOD_SOUND_PCMSETPOS_CALLBACK  pcmsetposcallback;
    FMOD_SOUND_NONBLOCK_CALLBACK   nonblockcallback;
    const char                    *dlsname;
    const char                    *encryptionkey;
    int                            maxpolyphony;
    void                          *userdata;
    FMOD_SOUND_TYPE                suggestedsoundtype;
    FMOD_FILE_OPEN_CALLBACK        fileuseropen;
    FMOD_FILE_CLOSE_CALLBACK       fileuserclose;
    FMOD_FILE_READ_CALLBACK        fileuserread;
    FMOD_FILE_SEEK_CALLBACK        fileuserseek;
    FMOD_FILE_ASYNCREAD_CALLBACK   fileuserasyncread;
    FMOD_FILE_ASYNCCANCEL_CALLBACK fileuserasynccancel;
    void                          *fileuserdata;
    int                            filebuffersize;
    FMOD_CHANNELORDER              channelorder;
    FMOD_SOUNDGROUP               *initialsoundgroup;
    unsigned int                   initialseekposition;
    FMOD_TIMEUNIT                  initialseekpostype;
    int                            ignoresetfilesystem;
    unsigned int                   audioqueuepolicy;
    unsigned int                   minmidigranularity;
    int                            nonblockthreadid;
    FMOD_GUID                     *fsbguid;
} FMOD_CREATESOUNDEXINFO;

typedef struct FMOD_REVERB_PROPERTIES
{
    float DecayTime;
    float EarlyDelay;
    float LateDelay;
    float HFReference;
    float HFDecayRatio;
    float Diffusion;
    float Density;
    float LowShelfFrequency;
    float LowShelfGain;
    float HighCut;
    float EarlyLateMix;
    float WetLevel;
} FMOD_REVERB_PROPERTIES;

typedef struct FMOD_ERRORCALLBACK_INFO
{
    FMOD_RESULT                      result;
    FMOD_ERRORCALLBACK_INSTANCETYPE  instancetype;
    void                            *instance;
    const char                      *functionname;
    const char                      *functionparams;
} FMOD_ERRORCALLBACK_INFO;

typedef struct FMOD_CPU_USAGE
{
    float           dsp;
    float           stream;
    float           geometry;
    float           update;
    float           convolution1;
    float           convolution2;
} FMOD_CPU_USAGE;

typedef struct FMOD_DSP_DATA_PARAMETER_INFO
{
    void           *data;
    unsigned int    length;
    int             index;
} FMOD_DSP_DATA_PARAMETER_INFO;

/*
    FMOD optional headers for plugin development
*/
#include "fmod_codec.h"
#include "fmod_dsp.h"
#include "fmod_output.h"

#endif
