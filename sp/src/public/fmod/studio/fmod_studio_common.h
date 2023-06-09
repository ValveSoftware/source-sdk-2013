/* ======================================================================================== */
/* FMOD Studio API - Common C/C++ header file.                                              */
/* Copyright (c), Firelight Technologies Pty, Ltd. 2004-2023.                               */
/*                                                                                          */
/* This header defines common enumerations, structs and callbacks that are shared between   */
/* the C and C++ interfaces.                                                                */
/*                                                                                          */
/* For more detail visit:                                                                   */
/* https://fmod.com/docs/2.02/api/studio-api.html                                           */
/* ======================================================================================== */
#ifndef FMOD_STUDIO_COMMON_H
#define FMOD_STUDIO_COMMON_H

#include "fmod.h"

/*
    FMOD Studio types.
*/
typedef struct          FMOD_STUDIO_SYSTEM           FMOD_STUDIO_SYSTEM;
typedef struct          FMOD_STUDIO_EVENTDESCRIPTION FMOD_STUDIO_EVENTDESCRIPTION;
typedef struct          FMOD_STUDIO_EVENTINSTANCE    FMOD_STUDIO_EVENTINSTANCE;
typedef struct          FMOD_STUDIO_BUS              FMOD_STUDIO_BUS;
typedef struct          FMOD_STUDIO_VCA              FMOD_STUDIO_VCA;
typedef struct          FMOD_STUDIO_BANK             FMOD_STUDIO_BANK;
typedef struct          FMOD_STUDIO_COMMANDREPLAY    FMOD_STUDIO_COMMANDREPLAY;

/*
    FMOD Studio constants
*/
#define FMOD_STUDIO_LOAD_MEMORY_ALIGNMENT                   32

typedef unsigned int FMOD_STUDIO_INITFLAGS;
#define FMOD_STUDIO_INIT_NORMAL                             0x00000000
#define FMOD_STUDIO_INIT_LIVEUPDATE                         0x00000001
#define FMOD_STUDIO_INIT_ALLOW_MISSING_PLUGINS              0x00000002
#define FMOD_STUDIO_INIT_SYNCHRONOUS_UPDATE                 0x00000004
#define FMOD_STUDIO_INIT_DEFERRED_CALLBACKS                 0x00000008
#define FMOD_STUDIO_INIT_LOAD_FROM_UPDATE                   0x00000010
#define FMOD_STUDIO_INIT_MEMORY_TRACKING                    0x00000020

typedef unsigned int FMOD_STUDIO_PARAMETER_FLAGS;
#define FMOD_STUDIO_PARAMETER_READONLY                      0x00000001
#define FMOD_STUDIO_PARAMETER_AUTOMATIC                     0x00000002
#define FMOD_STUDIO_PARAMETER_GLOBAL                        0x00000004
#define FMOD_STUDIO_PARAMETER_DISCRETE                      0x00000008
#define FMOD_STUDIO_PARAMETER_LABELED                       0x00000010

typedef unsigned int FMOD_STUDIO_SYSTEM_CALLBACK_TYPE;
#define FMOD_STUDIO_SYSTEM_CALLBACK_PREUPDATE               0x00000001
#define FMOD_STUDIO_SYSTEM_CALLBACK_POSTUPDATE              0x00000002
#define FMOD_STUDIO_SYSTEM_CALLBACK_BANK_UNLOAD             0x00000004
#define FMOD_STUDIO_SYSTEM_CALLBACK_LIVEUPDATE_CONNECTED    0x00000008
#define FMOD_STUDIO_SYSTEM_CALLBACK_LIVEUPDATE_DISCONNECTED 0x00000010
#define FMOD_STUDIO_SYSTEM_CALLBACK_ALL                     0xFFFFFFFF

typedef unsigned int FMOD_STUDIO_EVENT_CALLBACK_TYPE;
#define FMOD_STUDIO_EVENT_CALLBACK_CREATED                  0x00000001
#define FMOD_STUDIO_EVENT_CALLBACK_DESTROYED                0x00000002
#define FMOD_STUDIO_EVENT_CALLBACK_STARTING                 0x00000004
#define FMOD_STUDIO_EVENT_CALLBACK_STARTED                  0x00000008
#define FMOD_STUDIO_EVENT_CALLBACK_RESTARTED                0x00000010
#define FMOD_STUDIO_EVENT_CALLBACK_STOPPED                  0x00000020
#define FMOD_STUDIO_EVENT_CALLBACK_START_FAILED             0x00000040
#define FMOD_STUDIO_EVENT_CALLBACK_CREATE_PROGRAMMER_SOUND  0x00000080
#define FMOD_STUDIO_EVENT_CALLBACK_DESTROY_PROGRAMMER_SOUND 0x00000100
#define FMOD_STUDIO_EVENT_CALLBACK_PLUGIN_CREATED           0x00000200
#define FMOD_STUDIO_EVENT_CALLBACK_PLUGIN_DESTROYED         0x00000400
#define FMOD_STUDIO_EVENT_CALLBACK_TIMELINE_MARKER          0x00000800
#define FMOD_STUDIO_EVENT_CALLBACK_TIMELINE_BEAT            0x00001000
#define FMOD_STUDIO_EVENT_CALLBACK_SOUND_PLAYED             0x00002000
#define FMOD_STUDIO_EVENT_CALLBACK_SOUND_STOPPED            0x00004000
#define FMOD_STUDIO_EVENT_CALLBACK_REAL_TO_VIRTUAL          0x00008000
#define FMOD_STUDIO_EVENT_CALLBACK_VIRTUAL_TO_REAL          0x00010000
#define FMOD_STUDIO_EVENT_CALLBACK_START_EVENT_COMMAND      0x00020000
#define FMOD_STUDIO_EVENT_CALLBACK_NESTED_TIMELINE_BEAT     0x00040000
#define FMOD_STUDIO_EVENT_CALLBACK_ALL                      0xFFFFFFFF

typedef unsigned int FMOD_STUDIO_LOAD_BANK_FLAGS;
#define FMOD_STUDIO_LOAD_BANK_NORMAL                        0x00000000
#define FMOD_STUDIO_LOAD_BANK_NONBLOCKING                   0x00000001
#define FMOD_STUDIO_LOAD_BANK_DECOMPRESS_SAMPLES            0x00000002
#define FMOD_STUDIO_LOAD_BANK_UNENCRYPTED                   0x00000004

typedef unsigned int FMOD_STUDIO_COMMANDCAPTURE_FLAGS;
#define FMOD_STUDIO_COMMANDCAPTURE_NORMAL                   0x00000000
#define FMOD_STUDIO_COMMANDCAPTURE_FILEFLUSH                0x00000001
#define FMOD_STUDIO_COMMANDCAPTURE_SKIP_INITIAL_STATE       0x00000002

typedef unsigned int FMOD_STUDIO_COMMANDREPLAY_FLAGS;
#define FMOD_STUDIO_COMMANDREPLAY_NORMAL                    0x00000000
#define FMOD_STUDIO_COMMANDREPLAY_SKIP_CLEANUP              0x00000001
#define FMOD_STUDIO_COMMANDREPLAY_FAST_FORWARD              0x00000002
#define FMOD_STUDIO_COMMANDREPLAY_SKIP_BANK_LOAD            0x00000004

typedef enum FMOD_STUDIO_LOADING_STATE
{
    FMOD_STUDIO_LOADING_STATE_UNLOADING,
    FMOD_STUDIO_LOADING_STATE_UNLOADED,
    FMOD_STUDIO_LOADING_STATE_LOADING,
    FMOD_STUDIO_LOADING_STATE_LOADED,
    FMOD_STUDIO_LOADING_STATE_ERROR,

    FMOD_STUDIO_LOADING_STATE_FORCEINT = 65536  /* Makes sure this enum is signed 32bit. */
} FMOD_STUDIO_LOADING_STATE;

typedef enum FMOD_STUDIO_LOAD_MEMORY_MODE
{
    FMOD_STUDIO_LOAD_MEMORY,
    FMOD_STUDIO_LOAD_MEMORY_POINT,

    FMOD_STUDIO_LOAD_MEMORY_FORCEINT = 65536    /* Makes sure this enum is signed 32bit. */
} FMOD_STUDIO_LOAD_MEMORY_MODE;

typedef enum FMOD_STUDIO_PARAMETER_TYPE
{
    FMOD_STUDIO_PARAMETER_GAME_CONTROLLED,
    FMOD_STUDIO_PARAMETER_AUTOMATIC_DISTANCE,
    FMOD_STUDIO_PARAMETER_AUTOMATIC_EVENT_CONE_ANGLE,
    FMOD_STUDIO_PARAMETER_AUTOMATIC_EVENT_ORIENTATION,
    FMOD_STUDIO_PARAMETER_AUTOMATIC_DIRECTION,
    FMOD_STUDIO_PARAMETER_AUTOMATIC_ELEVATION,
    FMOD_STUDIO_PARAMETER_AUTOMATIC_LISTENER_ORIENTATION,
    FMOD_STUDIO_PARAMETER_AUTOMATIC_SPEED,
    FMOD_STUDIO_PARAMETER_AUTOMATIC_SPEED_ABSOLUTE,
    FMOD_STUDIO_PARAMETER_AUTOMATIC_DISTANCE_NORMALIZED,

    FMOD_STUDIO_PARAMETER_MAX,
    FMOD_STUDIO_PARAMETER_FORCEINT = 65536                  /* Makes sure this enum is signed 32bit. */
} FMOD_STUDIO_PARAMETER_TYPE;

typedef enum FMOD_STUDIO_USER_PROPERTY_TYPE
{
    FMOD_STUDIO_USER_PROPERTY_TYPE_INTEGER,
    FMOD_STUDIO_USER_PROPERTY_TYPE_BOOLEAN,
    FMOD_STUDIO_USER_PROPERTY_TYPE_FLOAT,
    FMOD_STUDIO_USER_PROPERTY_TYPE_STRING,

    FMOD_STUDIO_USER_PROPERTY_TYPE_FORCEINT = 65536 /* Makes sure this enum is signed 32bit. */
} FMOD_STUDIO_USER_PROPERTY_TYPE;

typedef enum FMOD_STUDIO_EVENT_PROPERTY
{
    FMOD_STUDIO_EVENT_PROPERTY_CHANNELPRIORITY,
    FMOD_STUDIO_EVENT_PROPERTY_SCHEDULE_DELAY,
    FMOD_STUDIO_EVENT_PROPERTY_SCHEDULE_LOOKAHEAD,
    FMOD_STUDIO_EVENT_PROPERTY_MINIMUM_DISTANCE,
    FMOD_STUDIO_EVENT_PROPERTY_MAXIMUM_DISTANCE,
    FMOD_STUDIO_EVENT_PROPERTY_COOLDOWN,
    FMOD_STUDIO_EVENT_PROPERTY_MAX,

    FMOD_STUDIO_EVENT_PROPERTY_FORCEINT = 65536     /* Makes sure this enum is signed 32bit. */
} FMOD_STUDIO_EVENT_PROPERTY;

typedef enum FMOD_STUDIO_PLAYBACK_STATE
{
    FMOD_STUDIO_PLAYBACK_PLAYING,
    FMOD_STUDIO_PLAYBACK_SUSTAINING,
    FMOD_STUDIO_PLAYBACK_STOPPED,
    FMOD_STUDIO_PLAYBACK_STARTING,
    FMOD_STUDIO_PLAYBACK_STOPPING,

    FMOD_STUDIO_PLAYBACK_FORCEINT = 65536
} FMOD_STUDIO_PLAYBACK_STATE;

typedef enum FMOD_STUDIO_STOP_MODE
{
    FMOD_STUDIO_STOP_ALLOWFADEOUT,
    FMOD_STUDIO_STOP_IMMEDIATE,

    FMOD_STUDIO_STOP_FORCEINT = 65536           /* Makes sure this enum is signed 32bit. */
} FMOD_STUDIO_STOP_MODE;

typedef enum FMOD_STUDIO_INSTANCETYPE
{
    FMOD_STUDIO_INSTANCETYPE_NONE,
    FMOD_STUDIO_INSTANCETYPE_SYSTEM,
    FMOD_STUDIO_INSTANCETYPE_EVENTDESCRIPTION,
    FMOD_STUDIO_INSTANCETYPE_EVENTINSTANCE,
    FMOD_STUDIO_INSTANCETYPE_PARAMETERINSTANCE,
    FMOD_STUDIO_INSTANCETYPE_BUS,
    FMOD_STUDIO_INSTANCETYPE_VCA,
    FMOD_STUDIO_INSTANCETYPE_BANK,
    FMOD_STUDIO_INSTANCETYPE_COMMANDREPLAY,

    FMOD_STUDIO_INSTANCETYPE_FORCEINT = 65536    /* Makes sure this enum is signed 32bit. */
} FMOD_STUDIO_INSTANCETYPE;

/*
    FMOD Studio structures
*/
typedef struct FMOD_STUDIO_BANK_INFO
{
    int                      size;
    void                    *userdata;
    int                      userdatalength;
    FMOD_FILE_OPEN_CALLBACK  opencallback;
    FMOD_FILE_CLOSE_CALLBACK closecallback;
    FMOD_FILE_READ_CALLBACK  readcallback;
    FMOD_FILE_SEEK_CALLBACK  seekcallback;
} FMOD_STUDIO_BANK_INFO;

typedef struct FMOD_STUDIO_PARAMETER_ID
{
    unsigned int data1;
    unsigned int data2;
} FMOD_STUDIO_PARAMETER_ID;

typedef struct FMOD_STUDIO_PARAMETER_DESCRIPTION
{
    const char                 *name;
    FMOD_STUDIO_PARAMETER_ID    id;
    float                       minimum;
    float                       maximum;
    float                       defaultvalue;
    FMOD_STUDIO_PARAMETER_TYPE  type;
    FMOD_STUDIO_PARAMETER_FLAGS flags;
    FMOD_GUID                   guid;
} FMOD_STUDIO_PARAMETER_DESCRIPTION;

typedef struct FMOD_STUDIO_USER_PROPERTY
{
    const char                     *name;
    FMOD_STUDIO_USER_PROPERTY_TYPE  type;

    union
    {
        int         intvalue;
        FMOD_BOOL   boolvalue;
        float       floatvalue;
        const char *stringvalue;
    };
} FMOD_STUDIO_USER_PROPERTY;

typedef struct FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES
{
    const char  *name;
    FMOD_SOUND  *sound;
    int          subsoundIndex;
} FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES;

typedef struct FMOD_STUDIO_PLUGIN_INSTANCE_PROPERTIES
{
    const char *name;
    FMOD_DSP   *dsp;
} FMOD_STUDIO_PLUGIN_INSTANCE_PROPERTIES;

typedef struct FMOD_STUDIO_TIMELINE_MARKER_PROPERTIES
{
    const char *name;
    int         position;
} FMOD_STUDIO_TIMELINE_MARKER_PROPERTIES;

typedef struct FMOD_STUDIO_TIMELINE_BEAT_PROPERTIES
{
    int     bar;
    int     beat;
    int     position;
    float   tempo;
    int     timesignatureupper;
    int     timesignaturelower;
} FMOD_STUDIO_TIMELINE_BEAT_PROPERTIES;

typedef struct FMOD_STUDIO_TIMELINE_NESTED_BEAT_PROPERTIES
{
    FMOD_GUID                               eventid;
    FMOD_STUDIO_TIMELINE_BEAT_PROPERTIES    properties;
} FMOD_STUDIO_TIMELINE_NESTED_BEAT_PROPERTIES;

typedef struct FMOD_STUDIO_ADVANCEDSETTINGS
{
    int             cbsize;
    unsigned int    commandqueuesize;
    unsigned int    handleinitialsize;
    int             studioupdateperiod;
    int             idlesampledatapoolsize;
    unsigned int    streamingscheduledelay;
    const char*     encryptionkey;
} FMOD_STUDIO_ADVANCEDSETTINGS;

typedef struct FMOD_STUDIO_CPU_USAGE
{
    float           update;
} FMOD_STUDIO_CPU_USAGE;

typedef struct FMOD_STUDIO_BUFFER_INFO
{
    int             currentusage;
    int             peakusage;
    int             capacity;
    int             stallcount;
    float           stalltime;
} FMOD_STUDIO_BUFFER_INFO;

typedef struct FMOD_STUDIO_BUFFER_USAGE
{
    FMOD_STUDIO_BUFFER_INFO studiocommandqueue;
    FMOD_STUDIO_BUFFER_INFO studiohandle;
} FMOD_STUDIO_BUFFER_USAGE;

typedef struct FMOD_STUDIO_SOUND_INFO
{
    const char             *name_or_data;
    FMOD_MODE               mode;
    FMOD_CREATESOUNDEXINFO  exinfo;
    int                     subsoundindex;
} FMOD_STUDIO_SOUND_INFO;

typedef struct FMOD_STUDIO_COMMAND_INFO
{
    const char                 *commandname;
    int                         parentcommandindex;
    int                         framenumber;
    float                       frametime;
    FMOD_STUDIO_INSTANCETYPE    instancetype;
    FMOD_STUDIO_INSTANCETYPE    outputtype;
    unsigned int                instancehandle;
    unsigned int                outputhandle;
} FMOD_STUDIO_COMMAND_INFO;

typedef struct FMOD_STUDIO_MEMORY_USAGE
{
    int exclusive;
    int inclusive;
    int sampledata;
} FMOD_STUDIO_MEMORY_USAGE;

/*
    FMOD Studio callbacks.
*/
typedef FMOD_RESULT (F_CALLBACK *FMOD_STUDIO_SYSTEM_CALLBACK)                           (FMOD_STUDIO_SYSTEM *system, FMOD_STUDIO_SYSTEM_CALLBACK_TYPE type, void *commanddata, void *userdata);
typedef FMOD_RESULT (F_CALLBACK *FMOD_STUDIO_EVENT_CALLBACK)                            (FMOD_STUDIO_EVENT_CALLBACK_TYPE type, FMOD_STUDIO_EVENTINSTANCE *event, void *parameters);
typedef FMOD_RESULT (F_CALLBACK *FMOD_STUDIO_COMMANDREPLAY_FRAME_CALLBACK)              (FMOD_STUDIO_COMMANDREPLAY *replay, int commandindex, float currenttime, void *userdata);
typedef FMOD_RESULT (F_CALLBACK *FMOD_STUDIO_COMMANDREPLAY_LOAD_BANK_CALLBACK)          (FMOD_STUDIO_COMMANDREPLAY *replay, int commandindex, const FMOD_GUID *bankguid, const char *bankfilename, FMOD_STUDIO_LOAD_BANK_FLAGS flags, FMOD_STUDIO_BANK **bank, void *userdata);
typedef FMOD_RESULT (F_CALLBACK *FMOD_STUDIO_COMMANDREPLAY_CREATE_INSTANCE_CALLBACK)    (FMOD_STUDIO_COMMANDREPLAY *replay, int commandindex, FMOD_STUDIO_EVENTDESCRIPTION *eventdescription, FMOD_STUDIO_EVENTINSTANCE **instance, void *userdata);

#endif // FMOD_STUDIO_COMMON_H
