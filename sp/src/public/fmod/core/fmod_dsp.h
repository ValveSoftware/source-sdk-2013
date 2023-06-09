/* ======================================================================================== */
/* FMOD Core API - DSP header file.                                                         */
/* Copyright (c), Firelight Technologies Pty, Ltd. 2004-2023.                               */
/*                                                                                          */
/* Use this header if you are wanting to develop your own DSP plugin to use with FMODs      */
/* dsp system.  With this header you can make your own DSP plugin that FMOD can             */
/* register and use.  See the documentation and examples on how to make a working plugin.   */
/*                                                                                          */
/* For more detail visit:                                                                   */
/* https://fmod.com/docs/2.02/api/plugin-api-dsp.html                                       */
/* =========================================================================================*/
#ifndef _FMOD_DSP_H
#define _FMOD_DSP_H

#include "fmod_dsp_effects.h"

typedef struct FMOD_DSP_STATE        FMOD_DSP_STATE;
typedef struct FMOD_DSP_BUFFER_ARRAY FMOD_DSP_BUFFER_ARRAY;
typedef struct FMOD_COMPLEX          FMOD_COMPLEX;

/*
    DSP Constants
*/
#define FMOD_PLUGIN_SDK_VERSION             110
#define FMOD_DSP_GETPARAM_VALUESTR_LENGTH   32

typedef enum
{
    FMOD_DSP_PROCESS_PERFORM,
    FMOD_DSP_PROCESS_QUERY
} FMOD_DSP_PROCESS_OPERATION;

typedef enum FMOD_DSP_PAN_SURROUND_FLAGS
{
    FMOD_DSP_PAN_SURROUND_DEFAULT = 0,
    FMOD_DSP_PAN_SURROUND_ROTATION_NOT_BIASED = 1,

    FMOD_DSP_PAN_SURROUND_FLAGS_FORCEINT = 65536
} FMOD_DSP_PAN_SURROUND_FLAGS;

typedef enum
{
    FMOD_DSP_PARAMETER_TYPE_FLOAT,
    FMOD_DSP_PARAMETER_TYPE_INT,
    FMOD_DSP_PARAMETER_TYPE_BOOL,
    FMOD_DSP_PARAMETER_TYPE_DATA,

    FMOD_DSP_PARAMETER_TYPE_MAX,
    FMOD_DSP_PARAMETER_TYPE_FORCEINT = 65536
} FMOD_DSP_PARAMETER_TYPE;

typedef enum
{
    FMOD_DSP_PARAMETER_FLOAT_MAPPING_TYPE_LINEAR,
    FMOD_DSP_PARAMETER_FLOAT_MAPPING_TYPE_AUTO,
    FMOD_DSP_PARAMETER_FLOAT_MAPPING_TYPE_PIECEWISE_LINEAR,

    FMOD_DSP_PARAMETER_FLOAT_MAPPING_TYPE_FORCEINT = 65536
} FMOD_DSP_PARAMETER_FLOAT_MAPPING_TYPE;

typedef enum
{
    FMOD_DSP_PARAMETER_DATA_TYPE_USER = 0,
    FMOD_DSP_PARAMETER_DATA_TYPE_OVERALLGAIN = -1,
    FMOD_DSP_PARAMETER_DATA_TYPE_3DATTRIBUTES = -2,
    FMOD_DSP_PARAMETER_DATA_TYPE_SIDECHAIN = -3,
    FMOD_DSP_PARAMETER_DATA_TYPE_FFT = -4,
    FMOD_DSP_PARAMETER_DATA_TYPE_3DATTRIBUTES_MULTI = -5,
    FMOD_DSP_PARAMETER_DATA_TYPE_ATTENUATION_RANGE = -6,
} FMOD_DSP_PARAMETER_DATA_TYPE;

/*
    DSP Callbacks
*/
typedef FMOD_RESULT (F_CALL *FMOD_DSP_CREATE_CALLBACK)                    (FMOD_DSP_STATE *dsp_state);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_RELEASE_CALLBACK)                   (FMOD_DSP_STATE *dsp_state);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_RESET_CALLBACK)                     (FMOD_DSP_STATE *dsp_state);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_READ_CALLBACK)                      (FMOD_DSP_STATE *dsp_state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int *outchannels);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_PROCESS_CALLBACK)                   (FMOD_DSP_STATE *dsp_state, unsigned int length, const FMOD_DSP_BUFFER_ARRAY *inbufferarray, FMOD_DSP_BUFFER_ARRAY *outbufferarray, FMOD_BOOL inputsidle, FMOD_DSP_PROCESS_OPERATION op);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_SETPOSITION_CALLBACK)               (FMOD_DSP_STATE *dsp_state, unsigned int pos);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_SHOULDIPROCESS_CALLBACK)            (FMOD_DSP_STATE *dsp_state, FMOD_BOOL inputsidle, unsigned int length, FMOD_CHANNELMASK inmask, int inchannels, FMOD_SPEAKERMODE speakermode);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_SETPARAM_FLOAT_CALLBACK)            (FMOD_DSP_STATE *dsp_state, int index, float value);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_SETPARAM_INT_CALLBACK)              (FMOD_DSP_STATE *dsp_state, int index, int value);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_SETPARAM_BOOL_CALLBACK)             (FMOD_DSP_STATE *dsp_state, int index, FMOD_BOOL value);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_SETPARAM_DATA_CALLBACK)             (FMOD_DSP_STATE *dsp_state, int index, void *data, unsigned int length);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_GETPARAM_FLOAT_CALLBACK)            (FMOD_DSP_STATE *dsp_state, int index, float *value, char *valuestr);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_GETPARAM_INT_CALLBACK)              (FMOD_DSP_STATE *dsp_state, int index, int *value, char *valuestr);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_GETPARAM_BOOL_CALLBACK)             (FMOD_DSP_STATE *dsp_state, int index, FMOD_BOOL *value, char *valuestr);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_GETPARAM_DATA_CALLBACK)             (FMOD_DSP_STATE *dsp_state, int index, void **data, unsigned int *length, char *valuestr);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_SYSTEM_REGISTER_CALLBACK)           (FMOD_DSP_STATE *dsp_state);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_SYSTEM_DEREGISTER_CALLBACK)         (FMOD_DSP_STATE *dsp_state);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_SYSTEM_MIX_CALLBACK)                (FMOD_DSP_STATE *dsp_state, int stage);

/*
    DSP Functions
*/
typedef void *      (F_CALL *FMOD_DSP_ALLOC_FUNC)                         (unsigned int size, FMOD_MEMORY_TYPE type, const char *sourcestr);
typedef void *      (F_CALL *FMOD_DSP_REALLOC_FUNC)                       (void *ptr, unsigned int size, FMOD_MEMORY_TYPE type, const char *sourcestr);
typedef void        (F_CALL *FMOD_DSP_FREE_FUNC)                          (void *ptr, FMOD_MEMORY_TYPE type, const char *sourcestr);
typedef void        (F_CALL *FMOD_DSP_LOG_FUNC)                           (FMOD_DEBUG_FLAGS level, const char *file, int line, const char *function, const char *str, ...);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_GETSAMPLERATE_FUNC)                 (FMOD_DSP_STATE *dsp_state, int *rate);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_GETBLOCKSIZE_FUNC)                  (FMOD_DSP_STATE *dsp_state, unsigned int *blocksize);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_GETSPEAKERMODE_FUNC)                (FMOD_DSP_STATE *dsp_state, FMOD_SPEAKERMODE *speakermode_mixer, FMOD_SPEAKERMODE *speakermode_output);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_GETCLOCK_FUNC)                      (FMOD_DSP_STATE *dsp_state, unsigned long long *clock, unsigned int *offset, unsigned int *length);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_GETLISTENERATTRIBUTES_FUNC)         (FMOD_DSP_STATE *dsp_state, int *numlisteners, FMOD_3D_ATTRIBUTES *attributes);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_GETUSERDATA_FUNC)                   (FMOD_DSP_STATE *dsp_state, void **userdata);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_DFT_FFTREAL_FUNC)                   (FMOD_DSP_STATE *dsp_state, int size, const float *signal, FMOD_COMPLEX* dft, const float *window, int signalhop);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_DFT_IFFTREAL_FUNC)                  (FMOD_DSP_STATE *dsp_state, int size, const FMOD_COMPLEX *dft, float* signal, const float *window, int signalhop);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_PAN_SUMMONOMATRIX_FUNC)             (FMOD_DSP_STATE *dsp_state, FMOD_SPEAKERMODE sourceSpeakerMode, float lowFrequencyGain, float overallGain, float *matrix);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_PAN_SUMSTEREOMATRIX_FUNC)           (FMOD_DSP_STATE *dsp_state, FMOD_SPEAKERMODE sourceSpeakerMode, float pan, float lowFrequencyGain, float overallGain, int matrixHop, float *matrix);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_PAN_SUMSURROUNDMATRIX_FUNC)         (FMOD_DSP_STATE *dsp_state, FMOD_SPEAKERMODE sourceSpeakerMode, FMOD_SPEAKERMODE targetSpeakerMode, float direction, float extent, float rotation, float lowFrequencyGain, float overallGain, int matrixHop, float *matrix, FMOD_DSP_PAN_SURROUND_FLAGS flags);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_PAN_SUMMONOTOSURROUNDMATRIX_FUNC)   (FMOD_DSP_STATE *dsp_state, FMOD_SPEAKERMODE targetSpeakerMode, float direction, float extent, float lowFrequencyGain, float overallGain, int matrixHop, float *matrix);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_PAN_SUMSTEREOTOSURROUNDMATRIX_FUNC) (FMOD_DSP_STATE *dsp_state, FMOD_SPEAKERMODE targetSpeakerMode, float direction, float extent, float rotation, float lowFrequencyGain, float overallGain, int matrixHop, float *matrix);
typedef FMOD_RESULT (F_CALL *FMOD_DSP_PAN_GETROLLOFFGAIN_FUNC)            (FMOD_DSP_STATE *dsp_state, FMOD_DSP_PAN_3D_ROLLOFF_TYPE rolloff, float distance, float mindistance, float maxdistance, float *gain);

/*
    DSP Structures
*/
struct FMOD_DSP_BUFFER_ARRAY
{
    int                numbuffers;
    int               *buffernumchannels;
    FMOD_CHANNELMASK  *bufferchannelmask;
    float            **buffers;
    FMOD_SPEAKERMODE   speakermode;
};

struct FMOD_COMPLEX
{
    float real;
    float imag;
};

typedef struct FMOD_DSP_PARAMETER_FLOAT_MAPPING_PIECEWISE_LINEAR
{
    int     numpoints;
    float  *pointparamvalues;
    float  *pointpositions;
} FMOD_DSP_PARAMETER_FLOAT_MAPPING_PIECEWISE_LINEAR;

typedef struct FMOD_DSP_PARAMETER_FLOAT_MAPPING
{
    FMOD_DSP_PARAMETER_FLOAT_MAPPING_TYPE               type;
    FMOD_DSP_PARAMETER_FLOAT_MAPPING_PIECEWISE_LINEAR   piecewiselinearmapping;
} FMOD_DSP_PARAMETER_FLOAT_MAPPING;

typedef struct FMOD_DSP_PARAMETER_DESC_FLOAT
{
    float                               min;
    float                               max;
    float                               defaultval;
    FMOD_DSP_PARAMETER_FLOAT_MAPPING    mapping;
} FMOD_DSP_PARAMETER_DESC_FLOAT;

typedef struct FMOD_DSP_PARAMETER_DESC_INT
{
    int                 min;
    int                 max;
    int                 defaultval;
    FMOD_BOOL           goestoinf;
    const char* const*  valuenames;
} FMOD_DSP_PARAMETER_DESC_INT;

typedef struct FMOD_DSP_PARAMETER_DESC_BOOL
{
    FMOD_BOOL           defaultval;
    const char* const*  valuenames;
} FMOD_DSP_PARAMETER_DESC_BOOL;

typedef struct FMOD_DSP_PARAMETER_DESC_DATA
{
    int datatype;
} FMOD_DSP_PARAMETER_DESC_DATA;

typedef struct FMOD_DSP_PARAMETER_DESC
{
    FMOD_DSP_PARAMETER_TYPE type;
    char                    name[16];
    char                    label[16];
    const char             *description;

    union
    {
        FMOD_DSP_PARAMETER_DESC_FLOAT   floatdesc;
        FMOD_DSP_PARAMETER_DESC_INT     intdesc;
        FMOD_DSP_PARAMETER_DESC_BOOL    booldesc;
        FMOD_DSP_PARAMETER_DESC_DATA    datadesc;
    };
} FMOD_DSP_PARAMETER_DESC;

typedef struct FMOD_DSP_PARAMETER_OVERALLGAIN
{
    float linear_gain;
    float linear_gain_additive;
} FMOD_DSP_PARAMETER_OVERALLGAIN;

typedef struct FMOD_DSP_PARAMETER_3DATTRIBUTES
{
    FMOD_3D_ATTRIBUTES relative;
    FMOD_3D_ATTRIBUTES absolute;
} FMOD_DSP_PARAMETER_3DATTRIBUTES;

typedef struct FMOD_DSP_PARAMETER_3DATTRIBUTES_MULTI
{
    int                numlisteners;
    FMOD_3D_ATTRIBUTES relative[FMOD_MAX_LISTENERS];
    float              weight[FMOD_MAX_LISTENERS];
    FMOD_3D_ATTRIBUTES absolute;
} FMOD_DSP_PARAMETER_3DATTRIBUTES_MULTI;

typedef struct FMOD_DSP_PARAMETER_ATTENUATION_RANGE
{
    float min;
    float max;
} FMOD_DSP_PARAMETER_ATTENUATION_RANGE;

typedef struct FMOD_DSP_PARAMETER_SIDECHAIN
{
    FMOD_BOOL sidechainenable;
} FMOD_DSP_PARAMETER_SIDECHAIN;

typedef struct FMOD_DSP_PARAMETER_FFT
{
    int     length;
    int     numchannels;
    float  *spectrum[32];
} FMOD_DSP_PARAMETER_FFT;

typedef struct FMOD_DSP_DESCRIPTION
{
    unsigned int                        pluginsdkversion;
    char                                name[32];
    unsigned int                        version;
    int                                 numinputbuffers;
    int                                 numoutputbuffers;
    FMOD_DSP_CREATE_CALLBACK            create;
    FMOD_DSP_RELEASE_CALLBACK           release;
    FMOD_DSP_RESET_CALLBACK             reset;
    FMOD_DSP_READ_CALLBACK              read;
    FMOD_DSP_PROCESS_CALLBACK           process;
    FMOD_DSP_SETPOSITION_CALLBACK       setposition;

    int                                 numparameters;
    FMOD_DSP_PARAMETER_DESC           **paramdesc;
    FMOD_DSP_SETPARAM_FLOAT_CALLBACK    setparameterfloat;
    FMOD_DSP_SETPARAM_INT_CALLBACK      setparameterint;
    FMOD_DSP_SETPARAM_BOOL_CALLBACK     setparameterbool;
    FMOD_DSP_SETPARAM_DATA_CALLBACK     setparameterdata;
    FMOD_DSP_GETPARAM_FLOAT_CALLBACK    getparameterfloat;
    FMOD_DSP_GETPARAM_INT_CALLBACK      getparameterint;
    FMOD_DSP_GETPARAM_BOOL_CALLBACK     getparameterbool;
    FMOD_DSP_GETPARAM_DATA_CALLBACK     getparameterdata;
    FMOD_DSP_SHOULDIPROCESS_CALLBACK    shouldiprocess;
    void                               *userdata;

    FMOD_DSP_SYSTEM_REGISTER_CALLBACK   sys_register;
    FMOD_DSP_SYSTEM_DEREGISTER_CALLBACK sys_deregister;
    FMOD_DSP_SYSTEM_MIX_CALLBACK        sys_mix;

} FMOD_DSP_DESCRIPTION;

typedef struct FMOD_DSP_STATE_DFT_FUNCTIONS
{
    FMOD_DSP_DFT_FFTREAL_FUNC  fftreal;
    FMOD_DSP_DFT_IFFTREAL_FUNC inversefftreal;
} FMOD_DSP_STATE_DFT_FUNCTIONS;

typedef struct FMOD_DSP_STATE_PAN_FUNCTIONS
{
    FMOD_DSP_PAN_SUMMONOMATRIX_FUNC             summonomatrix;
    FMOD_DSP_PAN_SUMSTEREOMATRIX_FUNC           sumstereomatrix;
    FMOD_DSP_PAN_SUMSURROUNDMATRIX_FUNC         sumsurroundmatrix;
    FMOD_DSP_PAN_SUMMONOTOSURROUNDMATRIX_FUNC   summonotosurroundmatrix;
    FMOD_DSP_PAN_SUMSTEREOTOSURROUNDMATRIX_FUNC sumstereotosurroundmatrix;
    FMOD_DSP_PAN_GETROLLOFFGAIN_FUNC            getrolloffgain;
} FMOD_DSP_STATE_PAN_FUNCTIONS;

typedef struct FMOD_DSP_STATE_FUNCTIONS
{
    FMOD_DSP_ALLOC_FUNC                 alloc;
    FMOD_DSP_REALLOC_FUNC               realloc;
    FMOD_DSP_FREE_FUNC                  free;
    FMOD_DSP_GETSAMPLERATE_FUNC         getsamplerate;
    FMOD_DSP_GETBLOCKSIZE_FUNC          getblocksize;
    FMOD_DSP_STATE_DFT_FUNCTIONS       *dft;
    FMOD_DSP_STATE_PAN_FUNCTIONS       *pan;
    FMOD_DSP_GETSPEAKERMODE_FUNC        getspeakermode;
    FMOD_DSP_GETCLOCK_FUNC              getclock;
    FMOD_DSP_GETLISTENERATTRIBUTES_FUNC getlistenerattributes;
    FMOD_DSP_LOG_FUNC                   log;
    FMOD_DSP_GETUSERDATA_FUNC           getuserdata;
} FMOD_DSP_STATE_FUNCTIONS;

struct FMOD_DSP_STATE
{
    void                     *instance;
    void                     *plugindata;
    FMOD_CHANNELMASK          channelmask;
    FMOD_SPEAKERMODE          source_speakermode;
    float                    *sidechaindata;
    int                       sidechainchannels;
    FMOD_DSP_STATE_FUNCTIONS *functions;
    int                       systemobject;
};

typedef struct FMOD_DSP_METERING_INFO
{
    int   numsamples;
    float peaklevel[32];
    float rmslevel[32];
    short numchannels;
} FMOD_DSP_METERING_INFO;

/*
    DSP Macros
*/
#define FMOD_DSP_INIT_PARAMDESC_FLOAT(_paramstruct, _name, _label, _description, _min, _max, _defaultval) \
    memset(&(_paramstruct), 0, sizeof(_paramstruct)); \
    (_paramstruct).type         = FMOD_DSP_PARAMETER_TYPE_FLOAT; \
    strncpy((_paramstruct).name,  _name,  15); \
    strncpy((_paramstruct).label, _label, 15); \
    (_paramstruct).description  = _description; \
    (_paramstruct).floatdesc.min          = _min; \
    (_paramstruct).floatdesc.max          = _max; \
    (_paramstruct).floatdesc.defaultval   = _defaultval; \
    (_paramstruct).floatdesc.mapping.type = FMOD_DSP_PARAMETER_FLOAT_MAPPING_TYPE_AUTO;

#define FMOD_DSP_INIT_PARAMDESC_FLOAT_WITH_MAPPING(_paramstruct, _name, _label, _description, _defaultval, _values, _positions); \
    memset(&(_paramstruct), 0, sizeof(_paramstruct)); \
    (_paramstruct).type         = FMOD_DSP_PARAMETER_TYPE_FLOAT; \
    strncpy((_paramstruct).name,  _name , 15); \
    strncpy((_paramstruct).label, _label, 15); \
    (_paramstruct).description  = _description; \
    (_paramstruct).floatdesc.min          = _values[0]; \
    (_paramstruct).floatdesc.max          = _values[sizeof(_values) / sizeof(float) - 1]; \
    (_paramstruct).floatdesc.defaultval   = _defaultval; \
    (_paramstruct).floatdesc.mapping.type = FMOD_DSP_PARAMETER_FLOAT_MAPPING_TYPE_PIECEWISE_LINEAR; \
    (_paramstruct).floatdesc.mapping.piecewiselinearmapping.numpoints = sizeof(_values) / sizeof(float); \
    (_paramstruct).floatdesc.mapping.piecewiselinearmapping.pointparamvalues = _values; \
    (_paramstruct).floatdesc.mapping.piecewiselinearmapping.pointpositions = _positions;

#define FMOD_DSP_INIT_PARAMDESC_INT(_paramstruct, _name, _label, _description, _min, _max, _defaultval, _goestoinf, _valuenames) \
    memset(&(_paramstruct), 0, sizeof(_paramstruct)); \
    (_paramstruct).type         = FMOD_DSP_PARAMETER_TYPE_INT; \
    strncpy((_paramstruct).name,  _name , 15); \
    strncpy((_paramstruct).label, _label, 15); \
    (_paramstruct).description  = _description; \
    (_paramstruct).intdesc.min          = _min; \
    (_paramstruct).intdesc.max          = _max; \
    (_paramstruct).intdesc.defaultval   = _defaultval; \
    (_paramstruct).intdesc.goestoinf    = _goestoinf; \
    (_paramstruct).intdesc.valuenames   = _valuenames;

#define FMOD_DSP_INIT_PARAMDESC_INT_ENUMERATED(_paramstruct, _name, _label, _description, _defaultval, _valuenames) \
    memset(&(_paramstruct), 0, sizeof(_paramstruct)); \
    (_paramstruct).type         = FMOD_DSP_PARAMETER_TYPE_INT; \
    strncpy((_paramstruct).name,  _name , 15); \
    strncpy((_paramstruct).label, _label, 15); \
    (_paramstruct).description  = _description; \
    (_paramstruct).intdesc.min          = 0; \
    (_paramstruct).intdesc.max          = sizeof(_valuenames) / sizeof(char*) - 1; \
    (_paramstruct).intdesc.defaultval   = _defaultval; \
    (_paramstruct).intdesc.goestoinf    = false; \
    (_paramstruct).intdesc.valuenames   = _valuenames;

#define FMOD_DSP_INIT_PARAMDESC_BOOL(_paramstruct, _name, _label, _description, _defaultval, _valuenames) \
    memset(&(_paramstruct), 0, sizeof(_paramstruct)); \
    (_paramstruct).type         = FMOD_DSP_PARAMETER_TYPE_BOOL; \
    strncpy((_paramstruct).name,  _name , 15); \
    strncpy((_paramstruct).label, _label, 15); \
    (_paramstruct).description  = _description; \
    (_paramstruct).booldesc.defaultval   = _defaultval; \
    (_paramstruct).booldesc.valuenames   = _valuenames;

#define FMOD_DSP_INIT_PARAMDESC_DATA(_paramstruct, _name, _label, _description, _datatype) \
    memset(&(_paramstruct), 0, sizeof(_paramstruct)); \
    (_paramstruct).type         = FMOD_DSP_PARAMETER_TYPE_DATA; \
    strncpy((_paramstruct).name,  _name , 15); \
    strncpy((_paramstruct).label, _label, 15); \
    (_paramstruct).description  = _description; \
    (_paramstruct).datadesc.datatype     = _datatype;

#define FMOD_DSP_ALLOC(_state, _size) \
    (_state)->functions->alloc(_size, FMOD_MEMORY_NORMAL, __FILE__)
#define FMOD_DSP_REALLOC(_state, _ptr, _size) \
    (_state)->functions->realloc(_ptr, _size, FMOD_MEMORY_NORMAL, __FILE__)
#define FMOD_DSP_FREE(_state, _ptr) \
    (_state)->functions->free(_ptr, FMOD_MEMORY_NORMAL, __FILE__)
#define FMOD_DSP_LOG(_state, _level, _location, _format, ...) \
    (_state)->functions->log(_level, __FILE__, __LINE__, _location, _format, __VA_ARGS__)
#define FMOD_DSP_GETSAMPLERATE(_state, _rate) \
    (_state)->functions->getsamplerate(_state, _rate)
#define FMOD_DSP_GETBLOCKSIZE(_state, _blocksize) \
    (_state)->functions->getblocksize(_state, _blocksize)
#define FMOD_DSP_GETSPEAKERMODE(_state, _speakermodemix, _speakermodeout) \
    (_state)->functions->getspeakermode(_state, _speakermodemix, _speakermodeout)
#define FMOD_DSP_GETCLOCK(_state, _clock, _offset, _length) \
    (_state)->functions->getclock(_state, _clock, _offset, _length)
#define FMOD_DSP_GETLISTENERATTRIBUTES(_state, _numlisteners, _attributes) \
    (_state)->functions->getlistenerattributes(_state, _numlisteners, _attributes)
#define FMOD_DSP_GETUSERDATA(_state, _userdata) \
    (_state)->functions->getuserdata(_state, _userdata)
#define FMOD_DSP_DFT_FFTREAL(_state, _size, _signal, _dft, _window, _signalhop) \
    (_state)->functions->dft->fftreal(_state, _size, _signal, _dft, _window, _signalhop)
#define FMOD_DSP_DFT_IFFTREAL(_state, _size, _dft, _signal, _window, _signalhop) \
    (_state)->functions->dft->inversefftreal(_state, _size, _dft, _signal, _window, _signalhop)
#define FMOD_DSP_PAN_SUMMONOMATRIX(_state, _sourcespeakermode, _lowfrequencygain, _overallgain, _matrix) \
    (_state)->functions->pan->summonomatrix(_state, _sourcespeakermode, _lowfrequencygain, _overallgain, _matrix)
#define FMOD_DSP_PAN_SUMSTEREOMATRIX(_state, _sourcespeakermode, _pan, _lowfrequencygain, _overallgain, _matrixhop, _matrix) \
    (_state)->functions->pan->sumstereomatrix(_state, _sourcespeakermode, _pan, _lowfrequencygain, _overallgain, _matrixhop, _matrix)
#define FMOD_DSP_PAN_SUMSURROUNDMATRIX(_state, _sourcespeakermode, _targetspeakermode, _direction, _extent, _rotation, _lowfrequencygain, _overallgain, _matrixhop, _matrix, _flags) \
    (_state)->functions->pan->sumsurroundmatrix(_state, _sourcespeakermode, _targetspeakermode, _direction, _extent, _rotation, _lowfrequencygain, _overallgain, _matrixhop, _matrix, _flags)
#define FMOD_DSP_PAN_SUMMONOTOSURROUNDMATRIX(_state, _targetspeakermode, _direction, _extent, _lowfrequencygain, _overallgain, _matrixhop, _matrix) \
    (_state)->functions->pan->summonotosurroundmatrix(_state, _targetspeakermode, _direction, _extent, _lowfrequencygain, _overallgain, _matrixhop, _matrix)
#define FMOD_DSP_PAN_SUMSTEREOTOSURROUNDMATRIX(_state, _targetspeakermode, _direction, _extent, _rotation, _lowfrequencygain, _overallgain, matrixhop, _matrix) \
    (_state)->functions->pan->sumstereotosurroundmatrix(_state, _targetspeakermode, _direction, _extent, _rotation, _lowfrequencygain, _overallgain, matrixhop, _matrix)
#define FMOD_DSP_PAN_GETROLLOFFGAIN(_state, _rolloff, _distance, _mindistance, _maxdistance, _gain) \
    (_state)->functions->pan->getrolloffgain(_state, _rolloff, _distance, _mindistance, _maxdistance, _gain)

#endif

