/* ======================================================================================== */
/* FMOD Core API - Codec development header file.                                           */
/* Copyright (c), Firelight Technologies Pty, Ltd. 2004-2023.                               */
/*                                                                                          */
/* Use this header if you are wanting to develop your own file format plugin to use with    */
/* FMOD's codec system.  With this header you can make your own fileformat plugin that FMOD */
/* can register and use.  See the documentation and examples on how to make a working       */
/* plugin.                                                                                  */
/*                                                                                          */
/* For more detail visit:                                                                   */
/* https://fmod.com/docs/2.02/api/core-api.html                                             */
/* ======================================================================================== */
#ifndef _FMOD_CODEC_H
#define _FMOD_CODEC_H

/*
    Codec types
*/
typedef struct FMOD_CODEC_STATE      FMOD_CODEC_STATE;
typedef struct FMOD_CODEC_WAVEFORMAT FMOD_CODEC_WAVEFORMAT;

/*
    Codec constants
*/
#define FMOD_CODEC_PLUGIN_VERSION 1

typedef int FMOD_CODEC_SEEK_METHOD;
#define FMOD_CODEC_SEEK_METHOD_SET      0
#define FMOD_CODEC_SEEK_METHOD_CURRENT  1
#define FMOD_CODEC_SEEK_METHOD_END      2

/*
    Codec callbacks
*/
typedef FMOD_RESULT (F_CALLBACK *FMOD_CODEC_OPEN_CALLBACK)         (FMOD_CODEC_STATE *codec_state, FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO *userexinfo);
typedef FMOD_RESULT (F_CALLBACK *FMOD_CODEC_CLOSE_CALLBACK)        (FMOD_CODEC_STATE *codec_state);
typedef FMOD_RESULT (F_CALLBACK *FMOD_CODEC_READ_CALLBACK)         (FMOD_CODEC_STATE *codec_state, void *buffer, unsigned int samples_in, unsigned int *samples_out);
typedef FMOD_RESULT (F_CALLBACK *FMOD_CODEC_GETLENGTH_CALLBACK)    (FMOD_CODEC_STATE *codec_state, unsigned int *length, FMOD_TIMEUNIT lengthtype);
typedef FMOD_RESULT (F_CALLBACK *FMOD_CODEC_SETPOSITION_CALLBACK)  (FMOD_CODEC_STATE *codec_state, int subsound, unsigned int position, FMOD_TIMEUNIT postype);
typedef FMOD_RESULT (F_CALLBACK *FMOD_CODEC_GETPOSITION_CALLBACK)  (FMOD_CODEC_STATE *codec_state, unsigned int *position, FMOD_TIMEUNIT postype);
typedef FMOD_RESULT (F_CALLBACK *FMOD_CODEC_SOUNDCREATE_CALLBACK)  (FMOD_CODEC_STATE *codec_state, int subsound, FMOD_SOUND *sound);
typedef FMOD_RESULT (F_CALLBACK *FMOD_CODEC_GETWAVEFORMAT_CALLBACK)(FMOD_CODEC_STATE *codec_state, int index, FMOD_CODEC_WAVEFORMAT *waveformat);

/*
    Codec functions
*/
typedef FMOD_RESULT (F_CALLBACK *FMOD_CODEC_METADATA_FUNC)         (FMOD_CODEC_STATE *codec_state, FMOD_TAGTYPE tagtype, char *name, void *data, unsigned int datalen, FMOD_TAGDATATYPE datatype, int unique);
typedef void *      (F_CALLBACK *FMOD_CODEC_ALLOC_FUNC)            (unsigned int size, unsigned int align, const char *file, int line);
typedef void        (F_CALLBACK *FMOD_CODEC_FREE_FUNC)             (void *ptr, const char *file, int line);
typedef void        (F_CALLBACK *FMOD_CODEC_LOG_FUNC)              (FMOD_DEBUG_FLAGS level, const char *file, int line, const char *function, const char *string, ...);

typedef FMOD_RESULT (F_CALLBACK *FMOD_CODEC_FILE_READ_FUNC)        (FMOD_CODEC_STATE *codec_state, void *buffer, unsigned int sizebytes, unsigned int *bytesread);
typedef FMOD_RESULT (F_CALLBACK *FMOD_CODEC_FILE_SEEK_FUNC)        (FMOD_CODEC_STATE *codec_state, unsigned int pos, FMOD_CODEC_SEEK_METHOD method);
typedef FMOD_RESULT (F_CALLBACK *FMOD_CODEC_FILE_TELL_FUNC)        (FMOD_CODEC_STATE *codec_state, unsigned int *pos);
typedef FMOD_RESULT (F_CALLBACK *FMOD_CODEC_FILE_SIZE_FUNC)        (FMOD_CODEC_STATE *codec_state, unsigned int *size);

/*
    Codec structures
*/
typedef struct FMOD_CODEC_DESCRIPTION
{
    unsigned int                      apiversion;
    const char                       *name;
    unsigned int                      version;
    int                               defaultasstream;
    FMOD_TIMEUNIT                     timeunits;
    FMOD_CODEC_OPEN_CALLBACK          open;
    FMOD_CODEC_CLOSE_CALLBACK         close;
    FMOD_CODEC_READ_CALLBACK          read;
    FMOD_CODEC_GETLENGTH_CALLBACK     getlength;
    FMOD_CODEC_SETPOSITION_CALLBACK   setposition;
    FMOD_CODEC_GETPOSITION_CALLBACK   getposition;
    FMOD_CODEC_SOUNDCREATE_CALLBACK   soundcreate;
    FMOD_CODEC_GETWAVEFORMAT_CALLBACK getwaveformat;
} FMOD_CODEC_DESCRIPTION;

struct FMOD_CODEC_WAVEFORMAT
{
    const char*        name;
    FMOD_SOUND_FORMAT  format;
    int                channels;
    int                frequency;
    unsigned int       lengthbytes;
    unsigned int       lengthpcm;
    unsigned int       pcmblocksize;
    int                loopstart;
    int                loopend;
    FMOD_MODE          mode;
    FMOD_CHANNELMASK   channelmask;
    FMOD_CHANNELORDER  channelorder;
    float              peakvolume;
};

typedef struct FMOD_CODEC_STATE_FUNCTIONS
{
    FMOD_CODEC_METADATA_FUNC     metadata;
    FMOD_CODEC_ALLOC_FUNC        alloc;
    FMOD_CODEC_FREE_FUNC         free;
    FMOD_CODEC_LOG_FUNC          log;
    FMOD_CODEC_FILE_READ_FUNC    read;
    FMOD_CODEC_FILE_SEEK_FUNC    seek;
    FMOD_CODEC_FILE_TELL_FUNC    tell;
    FMOD_CODEC_FILE_SIZE_FUNC    size;
} FMOD_CODEC_STATE_FUNCTIONS;

struct FMOD_CODEC_STATE
{
    void                        *plugindata;
    FMOD_CODEC_WAVEFORMAT       *waveformat;
    FMOD_CODEC_STATE_FUNCTIONS  *functions;
    int                          numsubsounds;
};

/*
    Codec macros
*/
#define FMOD_CODEC_METADATA(_state, _tagtype, _name, _data, _datalen, _datatype, _unique) \
    (_state)->functions->metadata(_state, _tagtype, _name, _data, _datalen, _datatype, _unique)
#define FMOD_CODEC_ALLOC(_state, _size, _align) \
    (_state)->functions->alloc(_size, _align, __FILE__, __LINE__)
#define FMOD_CODEC_FREE(_state, _ptr) \
    (_state)->functions->free(_ptr, __FILE__, __LINE__)
#define FMOD_CODEC_LOG(_state, _level, _location, _format, ...) \
    (_state)->functions->log(_level, __FILE__, __LINE__, _location, _format, __VA_ARGS__)
#define FMOD_CODEC_FILE_READ(_state, _buffer, _sizebytes, _bytesread) \
    (_state)->functions->read(_state, _buffer, _sizebytes, _bytesread)
#define FMOD_CODEC_FILE_SEEK(_state, _pos, _method) \
    (_state)->functions->seek(_state, _pos, _method)
#define FMOD_CODEC_FILE_TELL(_state, _pos) \
    (_state)->functions->tell(_state, _pos)
#define FMOD_CODEC_FILE_SIZE(_state, _size) \
    (_state)->functions->size(_state, _size)

#endif


