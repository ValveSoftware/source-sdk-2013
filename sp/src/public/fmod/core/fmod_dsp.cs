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

using System;
using System.Text;
using System.Runtime.InteropServices;

namespace FMOD
{
    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_BUFFER_ARRAY
    {
        public int              numbuffers;
        public int[]            buffernumchannels;
        public CHANNELMASK[]    bufferchannelmask;
        public IntPtr[]         buffers;
        public SPEAKERMODE      speakermode;
    }

    public enum DSP_PROCESS_OPERATION
    {
        PROCESS_PERFORM = 0,
        PROCESS_QUERY
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct COMPLEX
    {
        public float real;
        public float imag;
    }

    public enum DSP_PAN_SURROUND_FLAGS
    {
        DEFAULT = 0,
        ROTATION_NOT_BIASED = 1,
    }


    /*
        DSP callbacks
    */
    public delegate RESULT DSP_CREATE_CALLBACK                  (ref DSP_STATE dsp_state);
    public delegate RESULT DSP_RELEASE_CALLBACK                 (ref DSP_STATE dsp_state);
    public delegate RESULT DSP_RESET_CALLBACK                   (ref DSP_STATE dsp_state);
    public delegate RESULT DSP_SETPOSITION_CALLBACK             (ref DSP_STATE dsp_state, uint pos);
    public delegate RESULT DSP_READ_CALLBACK                    (ref DSP_STATE dsp_state, IntPtr inbuffer, IntPtr outbuffer, uint length, int inchannels, ref int outchannels);
    public delegate RESULT DSP_SHOULDIPROCESS_CALLBACK          (ref DSP_STATE dsp_state, bool inputsidle, uint length, CHANNELMASK inmask, int inchannels, SPEAKERMODE speakermode);
    public delegate RESULT DSP_PROCESS_CALLBACK                 (ref DSP_STATE dsp_state, uint length, ref DSP_BUFFER_ARRAY inbufferarray, ref DSP_BUFFER_ARRAY outbufferarray, bool inputsidle, DSP_PROCESS_OPERATION op);
    public delegate RESULT DSP_SETPARAM_FLOAT_CALLBACK          (ref DSP_STATE dsp_state, int index, float value);
    public delegate RESULT DSP_SETPARAM_INT_CALLBACK            (ref DSP_STATE dsp_state, int index, int value);
    public delegate RESULT DSP_SETPARAM_BOOL_CALLBACK           (ref DSP_STATE dsp_state, int index, bool value);
    public delegate RESULT DSP_SETPARAM_DATA_CALLBACK           (ref DSP_STATE dsp_state, int index, IntPtr data, uint length);
    public delegate RESULT DSP_GETPARAM_FLOAT_CALLBACK          (ref DSP_STATE dsp_state, int index, ref float value, IntPtr valuestr);
    public delegate RESULT DSP_GETPARAM_INT_CALLBACK            (ref DSP_STATE dsp_state, int index, ref int value, IntPtr valuestr);
    public delegate RESULT DSP_GETPARAM_BOOL_CALLBACK           (ref DSP_STATE dsp_state, int index, ref bool value, IntPtr valuestr);
    public delegate RESULT DSP_GETPARAM_DATA_CALLBACK           (ref DSP_STATE dsp_state, int index, ref IntPtr data, ref uint length, IntPtr valuestr);
    public delegate RESULT DSP_SYSTEM_REGISTER_CALLBACK         (ref DSP_STATE dsp_state);
    public delegate RESULT DSP_SYSTEM_DEREGISTER_CALLBACK       (ref DSP_STATE dsp_state);
    public delegate RESULT DSP_SYSTEM_MIX_CALLBACK              (ref DSP_STATE dsp_state, int stage);


    /*
        DSP functions
    */
    public delegate IntPtr DSP_ALLOC_FUNC                         (uint size, MEMORY_TYPE type, IntPtr sourcestr);
    public delegate IntPtr DSP_REALLOC_FUNC                       (IntPtr ptr, uint size, MEMORY_TYPE type, IntPtr sourcestr);
    public delegate void   DSP_FREE_FUNC                          (IntPtr ptr, MEMORY_TYPE type, IntPtr sourcestr);
    public delegate void   DSP_LOG_FUNC                           (DEBUG_FLAGS level, IntPtr file, int line, IntPtr function, IntPtr str);
    public delegate RESULT DSP_GETSAMPLERATE_FUNC                 (ref DSP_STATE dsp_state, ref int rate);
    public delegate RESULT DSP_GETBLOCKSIZE_FUNC                  (ref DSP_STATE dsp_state, ref uint blocksize);
    public delegate RESULT DSP_GETSPEAKERMODE_FUNC                (ref DSP_STATE dsp_state, ref int speakermode_mixer, ref int speakermode_output);
    public delegate RESULT DSP_GETCLOCK_FUNC                      (ref DSP_STATE dsp_state, out ulong clock, out uint offset, out uint length);
    public delegate RESULT DSP_GETLISTENERATTRIBUTES_FUNC         (ref DSP_STATE dsp_state, ref int numlisteners, IntPtr attributes);
    public delegate RESULT DSP_GETUSERDATA_FUNC                   (ref DSP_STATE dsp_state, out IntPtr userdata);
    public delegate RESULT DSP_DFT_FFTREAL_FUNC                   (ref DSP_STATE dsp_state, int size, IntPtr signal, IntPtr dft, IntPtr window, int signalhop);
    public delegate RESULT DSP_DFT_IFFTREAL_FUNC                  (ref DSP_STATE dsp_state, int size, IntPtr dft, IntPtr signal, IntPtr window, int signalhop);
    public delegate RESULT DSP_PAN_SUMMONOMATRIX_FUNC             (ref DSP_STATE dsp_state, int sourceSpeakerMode, float lowFrequencyGain, float overallGain, IntPtr matrix);
    public delegate RESULT DSP_PAN_SUMSTEREOMATRIX_FUNC           (ref DSP_STATE dsp_state, int sourceSpeakerMode, float pan, float lowFrequencyGain, float overallGain, int matrixHop, IntPtr matrix);
    public delegate RESULT DSP_PAN_SUMSURROUNDMATRIX_FUNC         (ref DSP_STATE dsp_state, int sourceSpeakerMode, int targetSpeakerMode, float direction, float extent, float rotation, float lowFrequencyGain, float overallGain, int matrixHop, IntPtr matrix, DSP_PAN_SURROUND_FLAGS flags);
    public delegate RESULT DSP_PAN_SUMMONOTOSURROUNDMATRIX_FUNC   (ref DSP_STATE dsp_state, int targetSpeakerMode, float direction, float extent, float lowFrequencyGain, float overallGain, int matrixHop, IntPtr matrix);
    public delegate RESULT DSP_PAN_SUMSTEREOTOSURROUNDMATRIX_FUNC (ref DSP_STATE dsp_state, int targetSpeakerMode, float direction, float extent, float rotation, float lowFrequencyGain, float overallGain, int matrixHop, IntPtr matrix);
    public delegate RESULT DSP_PAN_GETROLLOFFGAIN_FUNC            (ref DSP_STATE dsp_state, DSP_PAN_3D_ROLLOFF_TYPE rolloff, float distance, float mindistance, float maxdistance, out float gain);


    public enum DSP_TYPE : int
    {
        UNKNOWN,
        MIXER,
        OSCILLATOR,
        LOWPASS,
        ITLOWPASS,
        HIGHPASS,
        ECHO,
        FADER,
        FLANGE,
        DISTORTION,
        NORMALIZE,
        LIMITER,
        PARAMEQ,
        PITCHSHIFT,
        CHORUS,
        VSTPLUGIN,
        WINAMPPLUGIN,
        ITECHO,
        COMPRESSOR,
        SFXREVERB,
        LOWPASS_SIMPLE,
        DELAY,
        TREMOLO,
        LADSPAPLUGIN,
        SEND,
        RETURN,
        HIGHPASS_SIMPLE,
        PAN,
        THREE_EQ,
        FFT,
        LOUDNESS_METER,
        ENVELOPEFOLLOWER,
        CONVOLUTIONREVERB,
        CHANNELMIX,
        TRANSCEIVER,
        OBJECTPAN,
        MULTIBAND_EQ,
        MAX
    }

    public enum DSP_PARAMETER_TYPE
    {
        FLOAT = 0,
        INT,
        BOOL,
        DATA,
        MAX
    }

    public enum DSP_PARAMETER_FLOAT_MAPPING_TYPE
    {
        DSP_PARAMETER_FLOAT_MAPPING_TYPE_LINEAR = 0,
        DSP_PARAMETER_FLOAT_MAPPING_TYPE_AUTO,
        DSP_PARAMETER_FLOAT_MAPPING_TYPE_PIECEWISE_LINEAR,
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_PARAMETER_FLOAT_MAPPING_PIECEWISE_LINEAR
    {
        public int numpoints;
        public IntPtr pointparamvalues;
        public IntPtr pointpositions;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_PARAMETER_FLOAT_MAPPING
    {
        public DSP_PARAMETER_FLOAT_MAPPING_TYPE type;
        public DSP_PARAMETER_FLOAT_MAPPING_PIECEWISE_LINEAR piecewiselinearmapping;
    }


    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_PARAMETER_DESC_FLOAT
    {
        public float                     min;
        public float                     max;
        public float                     defaultval;
        public DSP_PARAMETER_FLOAT_MAPPING mapping;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_PARAMETER_DESC_INT
    {
        public int                       min;
        public int                       max;
        public int                       defaultval;
        public bool                      goestoinf;
        public IntPtr                    valuenames;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_PARAMETER_DESC_BOOL
    {
        public bool                      defaultval;
        public IntPtr                    valuenames;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_PARAMETER_DESC_DATA
    {
        public int                       datatype;
    }

    [StructLayout(LayoutKind.Explicit)]
    public struct DSP_PARAMETER_DESC_UNION
    {
        [FieldOffset(0)]
        public DSP_PARAMETER_DESC_FLOAT   floatdesc;
        [FieldOffset(0)]
        public DSP_PARAMETER_DESC_INT     intdesc;
        [FieldOffset(0)]
        public DSP_PARAMETER_DESC_BOOL    booldesc;
        [FieldOffset(0)]
        public DSP_PARAMETER_DESC_DATA    datadesc;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_PARAMETER_DESC
    {
        public DSP_PARAMETER_TYPE   type;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public byte[]               name;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public byte[]               label;
        public string               description;

        public DSP_PARAMETER_DESC_UNION desc;
    }

    public enum DSP_PARAMETER_DATA_TYPE
    {
        DSP_PARAMETER_DATA_TYPE_USER =                       0,
        DSP_PARAMETER_DATA_TYPE_OVERALLGAIN =               -1,
        DSP_PARAMETER_DATA_TYPE_3DATTRIBUTES =              -2,
        DSP_PARAMETER_DATA_TYPE_SIDECHAIN =                 -3,
        DSP_PARAMETER_DATA_TYPE_FFT =                       -4,
        DSP_PARAMETER_DATA_TYPE_3DATTRIBUTES_MULTI =        -5,
        DSP_PARAMETER_DATA_TYPE_ATTENUATION_RANGE =         -6
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_PARAMETER_OVERALLGAIN
    {
        public float linear_gain;
        public float linear_gain_additive;
    }
    
    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_PARAMETER_3DATTRIBUTES
    {
        public ATTRIBUTES_3D relative;
        public ATTRIBUTES_3D absolute;
    }
    
    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_PARAMETER_3DATTRIBUTES_MULTI
    {
        public int            numlisteners;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public ATTRIBUTES_3D[] relative;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public float[] weight;
        public ATTRIBUTES_3D absolute;
    }
    
    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_PARAMETER_SIDECHAIN
    {
        public int sidechainenable;
    }
    
    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_PARAMETER_FFT
    {
        public int     length;
        public int     numchannels;
        
        [MarshalAs(UnmanagedType.ByValArray,SizeConst=32)]
        private IntPtr[] spectrum_internal;

        public float[][] spectrum
        {
            get
            {
                var buffer = new float[numchannels][];
                
                for (int i = 0; i < numchannels; ++i)
                {
                    buffer[i] = new float[length];
                    Marshal.Copy(spectrum_internal[i], buffer[i], 0, length);
                }
                
                return buffer;
            }
        }

        public void getSpectrum(ref float[][] buffer)
        {
            int bufferLength = Math.Min(buffer.Length, numchannels);
            for (int i = 0; i < bufferLength; ++i)
            {
                getSpectrum(i, ref buffer[i]);
            }
        }

        public void getSpectrum(int channel, ref float[] buffer)
        {
            int bufferLength = Math.Min(buffer.Length, length);
            Marshal.Copy(spectrum_internal[channel], buffer, 0, bufferLength);
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_LOUDNESS_METER_INFO_TYPE
    {
        public float momentaryloudness;
        public float shorttermloudness;
        public float integratedloudness;
        public float loudness10thpercentile;
        public float loudness95thpercentile;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 66)]
        public float[] loudnesshistogram;
        public float maxtruepeak;
        public float maxmomentaryloudness;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_LOUDNESS_METER_WEIGHTING_TYPE
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
        public float[] channelweight;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_PARAMETER_ATTENUATION_RANGE
    {
        public float min;
        public float max;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_DESCRIPTION
    {
        public uint                           pluginsdkversion;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
        public byte[]                         name;
        public uint                           version;
        public int                            numinputbuffers;
        public int                            numoutputbuffers;
        public DSP_CREATE_CALLBACK            create;
        public DSP_RELEASE_CALLBACK           release;
        public DSP_RESET_CALLBACK             reset;
        public DSP_READ_CALLBACK              read;
        public DSP_PROCESS_CALLBACK           process;
        public DSP_SETPOSITION_CALLBACK       setposition;

        public int                            numparameters;
        public IntPtr                         paramdesc;
        public DSP_SETPARAM_FLOAT_CALLBACK    setparameterfloat;
        public DSP_SETPARAM_INT_CALLBACK      setparameterint;
        public DSP_SETPARAM_BOOL_CALLBACK     setparameterbool;
        public DSP_SETPARAM_DATA_CALLBACK     setparameterdata;
        public DSP_GETPARAM_FLOAT_CALLBACK    getparameterfloat;
        public DSP_GETPARAM_INT_CALLBACK      getparameterint;
        public DSP_GETPARAM_BOOL_CALLBACK     getparameterbool;
        public DSP_GETPARAM_DATA_CALLBACK     getparameterdata;
        public DSP_SHOULDIPROCESS_CALLBACK    shouldiprocess;
        public IntPtr                         userdata;

        public DSP_SYSTEM_REGISTER_CALLBACK   sys_register;
        public DSP_SYSTEM_DEREGISTER_CALLBACK sys_deregister;
        public DSP_SYSTEM_MIX_CALLBACK        sys_mix;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_STATE_DFT_FUNCTIONS
    {
        public DSP_DFT_FFTREAL_FUNC  fftreal;
        public DSP_DFT_IFFTREAL_FUNC inversefftreal;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_STATE_PAN_FUNCTIONS
    {
        public DSP_PAN_SUMMONOMATRIX_FUNC             summonomatrix;
        public DSP_PAN_SUMSTEREOMATRIX_FUNC           sumstereomatrix;
        public DSP_PAN_SUMSURROUNDMATRIX_FUNC         sumsurroundmatrix;
        public DSP_PAN_SUMMONOTOSURROUNDMATRIX_FUNC   summonotosurroundmatrix;
        public DSP_PAN_SUMSTEREOTOSURROUNDMATRIX_FUNC sumstereotosurroundmatrix;
        public DSP_PAN_GETROLLOFFGAIN_FUNC            getrolloffgain;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_STATE_FUNCTIONS
    {
        public DSP_ALLOC_FUNC                  alloc;
        public DSP_REALLOC_FUNC                realloc;
        public DSP_FREE_FUNC                   free;
        public DSP_GETSAMPLERATE_FUNC          getsamplerate;
        public DSP_GETBLOCKSIZE_FUNC           getblocksize;
        public IntPtr                          dft;
        public IntPtr                          pan;
        public DSP_GETSPEAKERMODE_FUNC         getspeakermode;
        public DSP_GETCLOCK_FUNC               getclock;
        public DSP_GETLISTENERATTRIBUTES_FUNC  getlistenerattributes;
        public DSP_LOG_FUNC                    log;
        public DSP_GETUSERDATA_FUNC            getuserdata;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_STATE
    {
        public IntPtr     instance;
        public IntPtr     plugindata;
        public uint       channelmask;
        public int        source_speakermode;
        public IntPtr     sidechaindata;
        public int        sidechainchannels;
        public IntPtr     functions;
        public int        systemobject;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DSP_METERING_INFO
    {
        public int   numsamples;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst=32)]
        public float[] peaklevel;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst=32)]
        public float[] rmslevel;
        public short numchannels;
    }

    /*
        ==============================================================================================================

        FMOD built in effect parameters.
        Use DSP::setParameter with these enums for the 'index' parameter.

        ==============================================================================================================
    */

    public enum DSP_OSCILLATOR : int
    {
        TYPE,
        RATE
    }

    public enum DSP_LOWPASS : int
    {
        CUTOFF,
        RESONANCE
    }

    public enum DSP_ITLOWPASS : int
    {
        CUTOFF,
        RESONANCE
    }

    public enum DSP_HIGHPASS : int
    {
        CUTOFF,
        RESONANCE
    }

    public enum DSP_ECHO : int
    {
        DELAY,
        FEEDBACK,
        DRYLEVEL,
        WETLEVEL
    }

    public enum DSP_FADER : int
    {
        GAIN,
        OVERALL_GAIN,
    }

    public enum DSP_DELAY : int
    {
        CH0,
        CH1,
        CH2,
        CH3,
        CH4,
        CH5,
        CH6,
        CH7,
        CH8,
        CH9,
        CH10,
        CH11,
        CH12,
        CH13,
        CH14,
        CH15,
        MAXDELAY,
    }

    public enum DSP_FLANGE : int
    {
        MIX,
        DEPTH,
        RATE
    }

    public enum DSP_TREMOLO : int
    {
        FREQUENCY,
        DEPTH,
        SHAPE,
        SKEW,
        DUTY,
        SQUARE,
        PHASE,
        SPREAD
    }

    public enum DSP_DISTORTION : int
    {
        LEVEL
    }

    public enum DSP_NORMALIZE : int
    {
        FADETIME,
        THRESHOLD,
        MAXAMP
    }

    public enum DSP_LIMITER : int
    {
        RELEASETIME,
        CEILING,
        MAXIMIZERGAIN,
        MODE,
    }

    public enum DSP_PARAMEQ : int
    {
        CENTER,
        BANDWIDTH,
        GAIN
    }

    public enum DSP_MULTIBAND_EQ : int
    {
        A_FILTER,
        A_FREQUENCY,
        A_Q,
        A_GAIN,
        B_FILTER,
        B_FREQUENCY,
        B_Q,
        B_GAIN,
        C_FILTER,
        C_FREQUENCY,
        C_Q,
        C_GAIN,
        D_FILTER,
        D_FREQUENCY,
        D_Q,
        D_GAIN,
        E_FILTER,
        E_FREQUENCY,
        E_Q,
        E_GAIN,
    }

    public enum DSP_MULTIBAND_EQ_FILTER_TYPE : int
    {
        DISABLED,
        LOWPASS_12DB,
        LOWPASS_24DB,
        LOWPASS_48DB,
        HIGHPASS_12DB,
        HIGHPASS_24DB,
        HIGHPASS_48DB,
        LOWSHELF,
        HIGHSHELF,
        PEAKING,
        BANDPASS,
        NOTCH,
        ALLPASS,
    }

    public enum DSP_PITCHSHIFT : int
    {
        PITCH,
        FFTSIZE,
        OVERLAP,
        MAXCHANNELS
    }

    public enum DSP_CHORUS : int
    {
        MIX,
        RATE,
        DEPTH,
    }

    public enum DSP_ITECHO : int
    {
        WETDRYMIX,
        FEEDBACK,
        LEFTDELAY,
        RIGHTDELAY,
        PANDELAY
    }

    public enum DSP_COMPRESSOR : int
    {
        THRESHOLD,
        RATIO,
        ATTACK,
        RELEASE,
        GAINMAKEUP,
        USESIDECHAIN,
        LINKED
    }

    public enum DSP_SFXREVERB : int
    {
        DECAYTIME,
        EARLYDELAY,
        LATEDELAY,
        HFREFERENCE,
        HFDECAYRATIO,
        DIFFUSION,
        DENSITY,
        LOWSHELFFREQUENCY,
        LOWSHELFGAIN,
        HIGHCUT,
        EARLYLATEMIX,
        WETLEVEL,
        DRYLEVEL
    }

    public enum DSP_LOWPASS_SIMPLE : int
    {
        CUTOFF
    }

    public enum DSP_SEND : int
    {
        RETURNID,
        LEVEL,
    }

    public enum DSP_RETURN : int
    {
        ID,
        INPUT_SPEAKER_MODE
    }

    public enum DSP_HIGHPASS_SIMPLE : int
    {
        CUTOFF
    }

    public enum DSP_PAN_2D_STEREO_MODE_TYPE : int
    {
        DISTRIBUTED,
        DISCRETE
    }

    public enum DSP_PAN_MODE_TYPE : int
    {
        MONO,
        STEREO,
        SURROUND
    }

    public enum DSP_PAN_3D_ROLLOFF_TYPE : int
    {
        LINEARSQUARED,
        LINEAR,
        INVERSE,
        INVERSETAPERED,
        CUSTOM
    }

    public enum DSP_PAN_3D_EXTENT_MODE_TYPE : int
    {
        AUTO,
        USER,
        OFF
    }

    public enum DSP_PAN : int
    {
        MODE,
        _2D_STEREO_POSITION,
        _2D_DIRECTION,
        _2D_EXTENT,
        _2D_ROTATION,
        _2D_LFE_LEVEL,
        _2D_STEREO_MODE,
        _2D_STEREO_SEPARATION,
        _2D_STEREO_AXIS,
        ENABLED_SPEAKERS,
        _3D_POSITION,
        _3D_ROLLOFF,
        _3D_MIN_DISTANCE,
        _3D_MAX_DISTANCE,
        _3D_EXTENT_MODE,
        _3D_SOUND_SIZE,
        _3D_MIN_EXTENT,
        _3D_PAN_BLEND,
        LFE_UPMIX_ENABLED,
        OVERALL_GAIN,
        SURROUND_SPEAKER_MODE,
        _2D_HEIGHT_BLEND,
        ATTENUATION_RANGE,
        OVERRIDE_RANGE
    }

    public enum DSP_THREE_EQ_CROSSOVERSLOPE_TYPE : int
    {
        _12DB,
        _24DB,
        _48DB
    }

    public enum DSP_THREE_EQ : int
    {
        LOWGAIN,
        MIDGAIN,
        HIGHGAIN,
        LOWCROSSOVER,
        HIGHCROSSOVER,
        CROSSOVERSLOPE
    }

    public enum DSP_FFT_WINDOW : int
    {
        RECT,
        TRIANGLE,
        HAMMING,
        HANNING,
        BLACKMAN,
        BLACKMANHARRIS
    }

    public enum DSP_FFT : int
    {
        WINDOWSIZE,
        WINDOWTYPE,
        SPECTRUMDATA,
        DOMINANT_FREQ
    }


    public enum DSP_LOUDNESS_METER : int
    {
        STATE,
        WEIGHTING,
        INFO
    }


    public enum DSP_LOUDNESS_METER_STATE_TYPE : int
    {
        RESET_INTEGRATED = -3,
        RESET_MAXPEAK = -2,
        RESET_ALL = -1,
        PAUSED = 0,
        ANALYZING = 1
    }

    public enum DSP_ENVELOPEFOLLOWER : int
    {
        ATTACK,
        RELEASE,
        ENVELOPE,
        USESIDECHAIN
    }

    public enum DSP_CONVOLUTION_REVERB : int
    {
        IR,
        WET,
        DRY,
        LINKED
    }

    public enum DSP_CHANNELMIX_OUTPUT : int
    {
        DEFAULT,
        ALLMONO,
        ALLSTEREO,
        ALLQUAD,
        ALL5POINT1,
        ALL7POINT1,
        ALLLFE,
        ALL7POINT1POINT4
    }

    public enum DSP_CHANNELMIX : int
    {
        OUTPUTGROUPING,
        GAIN_CH0,
        GAIN_CH1,
        GAIN_CH2,
        GAIN_CH3,
        GAIN_CH4,
        GAIN_CH5,
        GAIN_CH6,
        GAIN_CH7,
        GAIN_CH8,
        GAIN_CH9,
        GAIN_CH10,
        GAIN_CH11,
        GAIN_CH12,
        GAIN_CH13,
        GAIN_CH14,
        GAIN_CH15,
        GAIN_CH16,
        GAIN_CH17,
        GAIN_CH18,
        GAIN_CH19,
        GAIN_CH20,
        GAIN_CH21,
        GAIN_CH22,
        GAIN_CH23,
        GAIN_CH24,
        GAIN_CH25,
        GAIN_CH26,
        GAIN_CH27,
        GAIN_CH28,
        GAIN_CH29,
        GAIN_CH30,
        GAIN_CH31,
        OUTPUT_CH0,
        OUTPUT_CH1,
        OUTPUT_CH2,
        OUTPUT_CH3,
        OUTPUT_CH4,
        OUTPUT_CH5,
        OUTPUT_CH6,
        OUTPUT_CH7,
        OUTPUT_CH8,
        OUTPUT_CH9,
        OUTPUT_CH10,
        OUTPUT_CH11,
        OUTPUT_CH12,
        OUTPUT_CH13,
        OUTPUT_CH14,
        OUTPUT_CH15,
        OUTPUT_CH16,
        OUTPUT_CH17,
        OUTPUT_CH18,
        OUTPUT_CH19,
        OUTPUT_CH20,
        OUTPUT_CH21,
        OUTPUT_CH22,
        OUTPUT_CH23,
        OUTPUT_CH24,
        OUTPUT_CH25,
        OUTPUT_CH26,
        OUTPUT_CH27,
        OUTPUT_CH28,
        OUTPUT_CH29,
        OUTPUT_CH30,
        OUTPUT_CH31,
    }

    public enum DSP_TRANSCEIVER_SPEAKERMODE : int
    {
        AUTO = -1,
        MONO = 0,
        STEREO,
        SURROUND,
    }

    public enum DSP_TRANSCEIVER : int
    {
        TRANSMIT,
        GAIN,
        CHANNEL,
        TRANSMITSPEAKERMODE
    }

    public enum DSP_OBJECTPAN : int
    {
        _3D_POSITION,
        _3D_ROLLOFF,
        _3D_MIN_DISTANCE,
        _3D_MAX_DISTANCE,
        _3D_EXTENT_MODE,
        _3D_SOUND_SIZE,
        _3D_MIN_EXTENT,
        OVERALL_GAIN,
        OUTPUTGAIN,
        ATTENUATION_RANGE,
        OVERRIDE_RANGE
    }
}
