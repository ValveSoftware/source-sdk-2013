//=====================================================================
// Copyright (c) 2007-2011 Advanced Micro Devices, Inc. All rights reserved.
// Copyright (c) 2004-2006 ATI Technologies Inc.
//
/// \author GPU Developer Tools
/// \author gputools.support@amd.com
/// \file ATI_Compress.h
/// \version 1.80
/// \brief Declares the interface to the ATI_Compress library.
//
//=====================================================================

#ifndef ATI_COMPRESS
#define ATI_COMPRESS

#define ATI_COMPRESS_VERSION_MAJOR 1         ///< The major version number of this release.
#define ATI_COMPRESS_VERSION_MINOR 80        ///< The minor version number of this release.

typedef unsigned long ATI_TC_DWORD;          ///< A 32-bit integer format.
typedef unsigned short ATI_TC_WORD;          ///< A 16-bit integer format.
typedef unsigned char ATI_TC_BYTE;           ///< An 8-bit integer format.

#if defined(WIN32) || defined(_WIN64)
#   define ATI_TC_API __cdecl
#else
#   define ATI_TC_API
#endif

#ifdef ATI_COMPRESS_INTERNAL_BUILD
#   include "ATI_Compress_Internal.h"
#else // ATI_COMPRESS_INTERNAL_BUILD

/// Texture format.
typedef enum
{
   ATI_TC_FORMAT_Unknown,                    ///< An undefined texture format.
   ATI_TC_FORMAT_ARGB_8888,                  ///< An ARGB format with 8-bit fixed channels.
   ATI_TC_FORMAT_RGB_888,                    ///< A RGB format with 8-bit fixed channels.
   ATI_TC_FORMAT_RG_8,                       ///< A two component format with 8-bit fixed channels.
   ATI_TC_FORMAT_R_8,                        ///< A single component format with 8-bit fixed channels.
   ATI_TC_FORMAT_ARGB_2101010,               ///< An ARGB format with 10-bit fixed channels for color & a 2-bit fixed channel for alpha.
   ATI_TC_FORMAT_ARGB_16,                    ///< A ARGB format with 16-bit fixed channels.
   ATI_TC_FORMAT_RG_16,                      ///< A two component format with 16-bit fixed channels.
   ATI_TC_FORMAT_R_16,                       ///< A single component format with 16-bit fixed channels.
   ATI_TC_FORMAT_ARGB_16F,                   ///< An ARGB format with 16-bit floating-point channels.
   ATI_TC_FORMAT_RG_16F,                     ///< A two component format with 16-bit floating-point channels.
   ATI_TC_FORMAT_R_16F,                      ///< A single component with 16-bit floating-point channels.
   ATI_TC_FORMAT_ARGB_32F,                   ///< An ARGB format with 32-bit floating-point channels.
   ATI_TC_FORMAT_RG_32F,                     ///< A two component format with 32-bit floating-point channels.
   ATI_TC_FORMAT_R_32F,                      ///< A single component with 32-bit floating-point channels.
   ATI_TC_FORMAT_DXT1,                       ///< An opaque (or 1-bit alpha) DXTC compressed texture format. Four bits per pixel.
   ATI_TC_FORMAT_DXT3,                       ///< A DXTC compressed texture format with explicit alpha. Eight bits per pixel.
   ATI_TC_FORMAT_DXT5,                       ///< A DXTC compressed texture format with interpolated alpha. Eight bits per pixel.
   ATI_TC_FORMAT_DXT5_xGBR,                  ///< A DXT5 with the red component swizzled into the alpha channel. Eight bits per pixel.
   ATI_TC_FORMAT_DXT5_RxBG,                  ///< A swizzled DXT5 format with the green component swizzled into the alpha channel. Eight bits per pixel.
   ATI_TC_FORMAT_DXT5_RBxG,                  ///< A swizzled DXT5 format with the green component swizzled into the alpha channel & the blue component swizzled into the green channel. Eight bits per pixel.
   ATI_TC_FORMAT_DXT5_xRBG,                  ///< A swizzled DXT5 format with the green component swizzled into the alpha channel & the red component swizzled into the green channel. Eight bits per pixel.
   ATI_TC_FORMAT_DXT5_RGxB,                  ///< A swizzled DXT5 format with the blue component swizzled into the alpha channel. Eight bits per pixel.
   ATI_TC_FORMAT_DXT5_xGxR,                  ///< A two-component swizzled DXT5 format with the red component swizzled into the alpha channel & the green component in the green channel. Eight bits per pixel.
   ATI_TC_FORMAT_ATI1N,                      ///< A single component compression format using the same technique as DXT5 alpha. Four bits per pixel.
   ATI_TC_FORMAT_ATI2N,                      ///< A two component compression format using the same technique as DXT5 alpha. Designed for compression object space normal maps. Eight bits per pixel.
   ATI_TC_FORMAT_ATI2N_XY,                   ///< A two component compression format using the same technique as DXT5 alpha. The same as ATI2N but with the channels swizzled. Eight bits per pixel.
   ATI_TC_FORMAT_ATI2N_DXT5,                 ///< An ATI2N like format using DXT5. Intended for use on GPUs that do not natively support ATI2N. Eight bits per pixel.
   ATI_TC_FORMAT_BC1,                        ///< A four component opaque (or 1-bit alpha) compressed texture format for Microsoft DirectX10. Identical to DXT1.  Four bits per pixel.
   ATI_TC_FORMAT_BC2,                        ///< A four component compressed texture format with explicit alpha for Microsoft DirectX10. Identical to DXT3. Eight bits per pixel.
   ATI_TC_FORMAT_BC3,                        ///< A four component compressed texture format with interpolated alpha for Microsoft DirectX10. Identical to DXT5. Eight bits per pixel.
   ATI_TC_FORMAT_BC4,                        ///< A single component compressed texture format for Microsoft DirectX10. Identical to ATI1N. Four bits per pixel.
   ATI_TC_FORMAT_BC5,                        ///< A two component compressed texture format for Microsoft DirectX10. Identical to ATI2N. Eight bits per pixel.
   ATI_TC_FORMAT_ATC_RGB,                    ///< ATI_TC - a compressed RGB format for cellphones & hand-held devices.
   ATI_TC_FORMAT_ATC_RGBA_Explicit,          ///< ATI_TC - a compressed ARGB format with explicit alpha for cellphones & hand-held devices.
   ATI_TC_FORMAT_ATC_RGBA_Interpolated,      ///< ATI_TC - a compressed ARGB format with interpolated alpha for cellphones & hand-held devices.
   ATI_TC_FORMAT_ETC_RGB,                    ///< ETC (aka Ericsson Texture Compression) - a compressed RGB format for cellphones.
   ATI_TC_FORMAT_MAX = ATI_TC_FORMAT_ETC_RGB
} ATI_TC_FORMAT;

/// An enum selecting the speed vs. quality trade-off.
typedef enum
{
   ATI_TC_Speed_Normal,                      ///< Highest quality mode
   ATI_TC_Speed_Fast,                        ///< Slightly lower quality but much faster compression mode - DXTn & ATInN only
   ATI_TC_Speed_SuperFast,                   ///< Slightly lower quality but much, much faster compression mode - DXTn & ATInN only
} ATI_TC_Speed;

/// ATI_Compress error codes
typedef enum
{
   ATI_TC_OK = 0,                            ///< Ok.
   ATI_TC_ABORTED,                           ///< The conversion was aborted.
   ATI_TC_ERR_INVALID_SOURCE_TEXTURE,        ///< The source texture is invalid.
   ATI_TC_ERR_INVALID_DEST_TEXTURE,          ///< The destination texture is invalid.
   ATI_TC_ERR_UNSUPPORTED_SOURCE_FORMAT,     ///< The source format is not a supported format.
   ATI_TC_ERR_UNSUPPORTED_DEST_FORMAT,       ///< The destination format is not a supported format.
   ATI_TC_ERR_SIZE_MISMATCH,                 ///< The source and destination texture sizes do not match.
   ATI_TC_ERR_UNABLE_TO_INIT_CODEC,          ///< ATI_Compress was unable to initialize the codec needed for conversion.
   ATI_TC_ERR_GENERIC                        ///< An unknown error occurred.
} ATI_TC_ERROR;

/// Options for the compression.
/// Passing this structure is optional
typedef struct
{
   ATI_TC_DWORD	dwSize;					      ///< The size of this structure.
   BOOL			   bUseChannelWeighting;      ///< Use channel weightings. With swizzled formats the weighting applies to the data within the specified channel not the channel itself.
   double			fWeightingRed;			      ///< The weighting of the Red or X Channel.
   double			fWeightingGreen;		      ///< The weighting of the Green or Y Channel.
   double			fWeightingBlue;		      ///< The weighting of the Blue or Z Channel.
   BOOL			   bUseAdaptiveWeighting;     ///< Adapt weighting on a per-block basis.
   BOOL			   bDXT1UseAlpha;             ///< Encode single-bit alpha data. Only valid when compressing to DXT1 & BC1.
   ATI_TC_BYTE		nAlphaThreshold;           ///< The alpha threshold to use when compressing to DXT1 & BC1 with bDXT1UseAlpha. Texels with an alpha value less than the threshold are treated as transparent.
   BOOL			   bDisableMultiThreading;    ///< Disable multi-threading of the compression. This will slow the compression but can be useful if you're managing threads in your application.
   ATI_TC_Speed   nCompressionSpeed;         ///< The trade-off between compression speed & quality.
} ATI_TC_CompressOptions;
#endif // !ATI_COMPRESS_INTERNAL_BUILD

/// The structure describing a texture.
typedef struct
{
   ATI_TC_DWORD   dwSize;                    ///< Size of this structure.
   ATI_TC_DWORD	dwWidth;                   ///< Width of the texture.
   ATI_TC_DWORD	dwHeight;                  ///< Height of the texture.
   ATI_TC_DWORD	dwPitch;                   ///< Distance to start of next line - necessary only for uncompressed textures.
   ATI_TC_FORMAT	format;                    ///< Format of the texture.
   ATI_TC_DWORD	dwDataSize;                ///< Size of the allocated texture data.
   ATI_TC_BYTE*	pData;                     ///< Pointer to the texture data
} ATI_TC_Texture;

#define MINIMUM_WEIGHT_VALUE 0.01f


#ifdef __cplusplus
extern "C" {
#endif

   /// ATI_TC_Feedback_Proc
   /// Feedback function for conversion.
   /// \param[in] fProgress The percentage progress of the texture compression.
   /// \param[in] pUser1 User data as passed to ATI_TC_ConvertTexture.
   /// \param[in] pUser2 User data as passed to ATI_TC_ConvertTexture.
   /// \return non-NULL(true) value to abort conversion
   typedef bool (ATI_TC_API * ATI_TC_Feedback_Proc)(float fProgress, DWORD_PTR pUser1, DWORD_PTR pUser2);

   /// Calculates the required buffer size for the specified texture
   /// \param[in] pTexture A pointer to the texture.
   /// \return    The size of the buffer required to hold the texture data.
   ATI_TC_DWORD ATI_TC_API ATI_TC_CalculateBufferSize(const ATI_TC_Texture* pTexture);

   /// Converts the source texture to the destination texture
   /// This can be compression, decompression or converting between two uncompressed formats.
   /// \param[in] pSourceTexture A pointer to the source texture.
   /// \param[in] pDestTexture A pointer to the destination texture.
   /// \param[in] pOptions A pointer to the compression options - can be NULL.
   /// \param[in] pFeedbackProc A pointer to the feedback function - can be NULL.
   /// \param[in] pUser1 User data to pass to the feedback function.
   /// \param[in] pUser2 User data to pass to the feedback function.
   /// \return    ATI_TC_OK if successful, otherwise the error code.
   ATI_TC_ERROR ATI_TC_API ATI_TC_ConvertTexture(const ATI_TC_Texture* pSourceTexture, ATI_TC_Texture* pDestTexture,
                                                 const ATI_TC_CompressOptions* pOptions,
                                                 ATI_TC_Feedback_Proc pFeedbackProc, DWORD_PTR pUser1, DWORD_PTR pUser2);

#ifdef __cplusplus
};
#endif

#endif // !ATI_COMPRESS
