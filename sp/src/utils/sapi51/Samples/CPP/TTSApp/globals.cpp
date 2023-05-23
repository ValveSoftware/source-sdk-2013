#include "globals.h"
//Copyright (c) Microsoft Corporation. All rights reserved.

// Other global variables
int                 g_iBmp              = 0;     // current bmp
HIMAGELIST          g_hListBmp          = 0;     // image list
const int           g_aMapVisemeToImage[22] = { 0,  // SP_VISEME_0 = 0,    // Silence
                                                11, // SP_VISEME_1,        // AE, AX, AH
                                                11, // SP_VISEME_2,        // AA
                                                11, // SP_VISEME_3,        // AO
                                                10, // SP_VISEME_4,        // EY, EH, UH
                                                11, // SP_VISEME_5,        // ER
                                                9,  // SP_VISEME_6,        // y, IY, IH, IX
                                                2,  // SP_VISEME_7,        // w, UW
                                                13, // SP_VISEME_8,        // OW
                                                9,  // SP_VISEME_9,        // AW
                                                12, // SP_VISEME_10,       // OY
                                                11, // SP_VISEME_11,       // AY
                                                9,  // SP_VISEME_12,       // h
                                                3,  // SP_VISEME_13,       // r
                                                6,  // SP_VISEME_14,       // l
                                                7,  // SP_VISEME_15,       // s, z
                                                8,  // SP_VISEME_16,       // SH, CH, JH, ZH
                                                5,  // SP_VISEME_17,       // TH, DH
                                                4,  // SP_VISEME_18,       // f, v
                                                7,  // SP_VISEME_19,       // d, t, n
                                                9,  // SP_VISEME_20,       // k, g, NG
                                                1 };// SP_VISEME_21,       // p, b, m


// Output formats
const SPSTREAMFORMAT g_aOutputFormat[NUM_OUTPUTFORMATS] = {SPSF_8kHz8BitMono,     
                                                SPSF_8kHz8BitStereo,
                                                SPSF_8kHz16BitMono,
                                                SPSF_8kHz16BitStereo,   
                                                SPSF_11kHz8BitMono,
                                                SPSF_11kHz8BitStereo,   
                                                SPSF_11kHz16BitMono,
                                                SPSF_11kHz16BitStereo,    
                                                SPSF_12kHz8BitMono,
                                                SPSF_12kHz8BitStereo,    
                                                SPSF_12kHz16BitMono,
                                                SPSF_12kHz16BitStereo,
                                                SPSF_16kHz8BitMono,   
                                                SPSF_16kHz8BitStereo, 
                                                SPSF_16kHz16BitMono,
                                                SPSF_16kHz16BitStereo,
                                                SPSF_22kHz8BitMono,
                                                SPSF_22kHz8BitStereo,
                                                SPSF_22kHz16BitMono,
                                                SPSF_22kHz16BitStereo,
                                                SPSF_24kHz8BitMono,
                                                SPSF_24kHz8BitStereo,
                                                SPSF_24kHz16BitMono,
                                                SPSF_24kHz16BitStereo,
                                                SPSF_32kHz8BitMono,
                                                SPSF_32kHz8BitStereo,
                                                SPSF_32kHz16BitMono,
                                                SPSF_32kHz16BitStereo,
                                                SPSF_44kHz8BitMono,
                                                SPSF_44kHz8BitStereo,
                                                SPSF_44kHz16BitMono,
                                                SPSF_44kHz16BitStereo,
                                                SPSF_48kHz8BitMono,
                                                SPSF_48kHz8BitStereo,
                                                SPSF_48kHz16BitMono,
                                                SPSF_48kHz16BitStereo};

TCHAR* g_aszOutputFormat[NUM_OUTPUTFORMATS] = {_T("8kHz 8 Bit Mono"),     
                                                _T("8kHz 8 Bit Stereo"),
                                                _T("8kHz 16 Bit Mono"),
                                                _T("8kHz 16 Bit Stereo"),   
                                                _T("11kHz 8 Bit Mono"),
                                                _T("11kHz 8 Bit Stereo"),   
                                                _T("11kHz 16 Bit Mono"),
                                                _T("11kHz 16 Bit Stereo"),    
                                                _T("12kHz 8 Bit Mono"),
                                                _T("12kHz 8 Bit Stereo"),    
                                                _T("12kHz 16 Bit Mono"),
                                                _T("12kHz 16 Bit Stereo"),
                                                _T("16kHz 8 Bit Mono"),   
                                                _T("16kHz 8 Bit Stereo"), 
                                                _T("16kHz 16 Bit Mono"),
                                                _T("16kHz 16 Bit Stereo"),
                                                _T("22kHz 8 Bit Mono"),
                                                _T("22kHz 8 Bit Stereo"),
                                                _T("22kHz 16 Bit Mono"),
                                                _T("22kHz 16 Bit Stereo"),
                                                _T("24kHz 8 Bit Mono"),
                                                _T("24kHz 8 Bit Stereo"),
                                                _T("24kHz 16 Bit Mono"),
                                                _T("24kHz 16 Bit Stereo"),
                                                _T("32kHz 8 Bit Mono"),
                                                _T("32kHz 8 Bit Stereo"),
                                                _T("32kHz 16 Bit Mono"),
                                                _T("32kHz 16 Bit Stereo"),
                                                _T("44kHz 8 Bit Mono"),
                                                _T("44kHz 8 Bit Stereo"),
                                                _T("44kHz 16 Bit Mono"),
                                                _T("44kHz 16 Bit Stereo"),
                                                _T("48kHz 8 Bit Mono"),
                                                _T("48kHz 8 Bit Stereo"),
                                                _T("48kHz 16 Bit Mono"),
                                                _T("48kHz 16 Bit Stereo")};

// ITextServices interface guid
const IID IID_ITextServices = {0X8D33F740,0XCF58,0x11ce,{0XA8,0X9D,0X00,0XAA,0X00,0X6C,0XAD,0XC5}};

// ITextDocument interface guid
const IID IID_ITextDocument = {0x8CC497C0,0xA1DF,0x11ce,{0x80,0x98,0x00,0xAA,0x00,0x47,0xBE,0x5D}};
