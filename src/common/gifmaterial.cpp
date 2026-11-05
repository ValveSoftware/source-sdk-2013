#include "gifmaterial.h"
#include "gif_lib.h"

#include "vstdlib/jobthread.h"
#include "tier0/vprof.h"
#include "tier1/KeyValues.h"
#include "tier1/convar.h"
#include "tier1/utlbuffer.h"

#include "filesystem.h"

#include "materialsystem/imaterial.h"
#include "materialsystem/itexture.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar mat_async_gif_processor( "mat_async_gif_processor", "1", FCVAR_NONE, "Whether to process GIF materials on a worker thread. In most cases you want to flip this on to avoid stuttering." );


//-----------------------------------------------------------------------------
// Purpose: Texture regenerator for animating the GIF in a texture
//-----------------------------------------------------------------------------
class CGifTextureRegen : public ITextureRegenerator
{
public:
    CGifTextureRegen( CGifMaterial *pOuter );

    virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pRect );
    virtual void Release( void );

private:
    CGifMaterial *m_pOuter;
};

//-----------------------------------------------------------------------------
// Purpose: Keep track of all GIF materials so we ensure they are all purged
//          before the material system shutdowns to workaround a crash
//-----------------------------------------------------------------------------
class CGifMaterialTracker
{
public:
    ~CGifMaterialTracker( void );

	void Register( CGifMaterial *pMaterial );
	void Unregister( CGifMaterial *pMaterial );

private:
	CUtlVector< CGifMaterial * > m_vecMaterials;
};

CGifMaterialTracker g_GifMaterialTracker;


//-----------------------------------------------------------------------------
// Purpose: giflib read callback for pulling data from CUtlBuffers
//-----------------------------------------------------------------------------
static int GifReadData( GifFileType *pImage, GifByteType *pubBuffer, int cubBuffer )
{
    auto &bufImage = *( CUtlBuffer * )pImage->UserData;

    int nBytesToRead = Min( cubBuffer, bufImage.GetBytesRemaining() );
    if ( nBytesToRead > 0 )
        bufImage.Get( pubBuffer, nBytesToRead );

    return nBytesToRead;
}

//-----------------------------------------------------------------------------
// Purpose: Creates the VMT keyvalues for GIFs materials
//-----------------------------------------------------------------------------
static KeyValues *GifVMTFactory( ITexture *pTexture, GifFileType *pImage )
{
    float flScaleX = ( float )pImage->SWidth / ( float )pTexture->GetMappingWidth();
    float flScaleY = ( float )pImage->SHeight / ( float )pTexture->GetMappingHeight();

    char szTransform[ 64 ];
    Q_snprintf( szTransform, sizeof( szTransform ), "center 0 0 scale %f %f rotate 0 translate 0 0", flScaleX, flScaleY );

    KeyValues *pVMT = new KeyValues( "UnlitGeneric" );
    pVMT->SetString( "$BaseTexture", pTexture->GetName() );
    pVMT->SetString( "$BaseTextureTransform", szTransform );
    pVMT->SetInt( "$Translucent", 1 );
    pVMT->SetInt( "$NoLOD", 1 );
    pVMT->SetInt( "$NoMIP", 1 );
    pVMT->SetInt( "$VertexColor", 1 );
    pVMT->SetInt( "$VertexAlpha", 1 );

    return pVMT;
}


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGifMaterial::CGifMaterial( void )
    : m_iSelectedFrame( 0 )
    , m_flNextIterationTime( 0.f )
	, m_pMaterial( NULL )
	, m_pTexture( NULL )
    , m_ProcessedEvent( true )
    , m_pAsyncProcessorJob( NULL )
{
	g_GifMaterialTracker.Register( this );
}

//-----------------------------------------------------------------------------
// Purpose: Deconstructor
//-----------------------------------------------------------------------------
CGifMaterial::~CGifMaterial( void )
{
	g_GifMaterialTracker.Unregister( this );

	Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Create a GIF material from a file
//-----------------------------------------------------------------------------
bool CGifMaterial::BInit( const char *pszFileName, const char *pszPathID /*= NULL*/ )
{
    void *pvBuffer = NULL;
    int cubBuffer = g_pFullFileSystem->ReadFileEx( pszFileName, pszPathID, &pvBuffer, false, true );

    if ( !cubBuffer )
    {
        Warning( "Failed to open GIF image at \"%s\" because it is missing or corrupted.\n", pszFileName );
        return false;
    }

    bool bRetVal = BInit( pvBuffer, cubBuffer );
    g_pFullFileSystem->FreeOptimalReadBuffer( pvBuffer );
    return bRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: Create a GIF material from a buffer
//-----------------------------------------------------------------------------
bool CGifMaterial::BInit( const void *pubImage, int cubImage )
{
    VPROF( "CGifMaterial::BInit" );

	Purge();

    // Copy the image data over to a heap buffer since we're gonna need to access it in the processor worker
    CUtlBuffer *pbufCopy = new CUtlBuffer;
    pbufCopy->CopyBuffer( pubImage, cubImage );

    // Open the GIF here so we can get at the screen descriptor for creating the texture
    int nError;
    GifFileType *pImage = DGifOpen( pbufCopy, GifReadData, &nError );
    if ( !pImage )
    {
        Warning( "Failed to open GIF image: %s\n", GifErrorString( nError ) );
        return false;
    }

    // Texture size needs to be aligned to be a multiple of 4
    int nTextureWide = AlignUp< 4 >( pImage->SWidth );
    int nTextureTall = AlignUp< 4 >( pImage->SHeight );

    static int  s_nGifMaterialCount = 0;
    char        szMaterialName[ 64 ];
    Q_snprintf( szMaterialName, sizeof( szMaterialName ), "__CGifMaterial_%i", s_nGifMaterialCount++ );

    m_pTexture = g_pMaterialSystem->CreateProceduralTexture(
        szMaterialName, TEXTURE_GROUP_UNACCOUNTED,
        nTextureWide, nTextureTall,
        IMAGE_FORMAT_DXT5_RUNTIME,
        TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_NOMIP |
        TEXTUREFLAGS_NOLOD | TEXTUREFLAGS_PROCEDURAL | TEXTUREFLAGS_SINGLECOPY
    );

    // Material system will take ownership of VMT keyvalues and free it when the material is released
    KeyValues *pVMT = GifVMTFactory( m_pTexture, pImage );
    m_pMaterial = g_pMaterialSystem->CreateMaterial( szMaterialName, pVMT );
    m_pMaterial->Refresh();

    // Process our image
    if ( mat_async_gif_processor.GetBool() )
    {
        m_pAsyncProcessorJob = ThreadExecute( this, &CGifMaterial::Process, pImage );
    }
    else
    {
        Process( pImage );
    }

    // Assign the regenerator and upload a frame
    m_pTexture->SetTextureRegenerator( new CGifTextureRegen( this ) );
    m_pTexture->Download();

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Free all GIF resources
//-----------------------------------------------------------------------------
void CGifMaterial::Purge( void )
{
    if ( m_pAsyncProcessorJob )
    {
        // For safety join it's thread instead of aborting it
        m_pAsyncProcessorJob->WaitForFinishAndRelease();
        m_pAsyncProcessorJob = NULL;
    }
    m_ProcessedEvent.Reset();

    m_Frames.Purge();
    m_iSelectedFrame = 0;
    m_flNextIterationTime = 0.f;

    if ( m_pTexture )
    {
        m_pTexture->SetTextureRegenerator( NULL );
        m_pTexture->Release();
		m_pTexture->DeleteIfUnreferenced();
        m_pTexture = NULL;
    }
    if ( m_pMaterial )
    {
        m_pMaterial->Release();
		m_pMaterial->DeleteIfUnreferenced();
        m_pMaterial = NULL;
    }
}

//-----------------------------------------------------------------------------
// Purpose: Has this GIF been processed?
//-----------------------------------------------------------------------------
bool CGifMaterial::BIsProcessed( void ) const
{
    return const_cast< CGifMaterial * >( this )->m_ProcessedEvent.Check();
}


//-----------------------------------------------------------------------------
// Purpose: Sets the selected frame to specified index.
//-----------------------------------------------------------------------------
void CGifMaterial::SetFrame( int iFrame )
{
    if ( !BIsProcessed() )
    {
        return;
    }

    m_iSelectedFrame = iFrame;

    if ( m_iSelectedFrame < 0 || m_iSelectedFrame >= m_Frames.Count() )
    {
        // Loop
        m_iSelectedFrame = 0;
    }

    // Upload the new frame data
    if ( m_pTexture )
    {
        m_pTexture->Download();
    }

    // simulates web browsers "throttling" short time delays so
    // gif animation speed is similar to Steam's
    constexpr float k_flMinTime = .02, k_flDefaultTime = .1; // Chrome defaults

    float flDuration = m_Frames[ m_iSelectedFrame ].flDuration;
    m_flNextIterationTime = ( flDuration < k_flMinTime ? k_flDefaultTime : flDuration ) + ( float )Plat_FloatTime();
}

//-----------------------------------------------------------------------------
// Purpose: Currently used frame.
//-----------------------------------------------------------------------------
int CGifMaterial::GetFrame( void ) const
{
    return m_iSelectedFrame;
}

//-----------------------------------------------------------------------------
// Purpose: Gets the total number of frames in the open image
//-----------------------------------------------------------------------------
int CGifMaterial::GetFrames( void ) const
{
    if ( !BIsProcessed() )
    {
        return 0;
    }

	return m_Frames.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Advances the current frame index if allowed
//-----------------------------------------------------------------------------
bool CGifMaterial::BAdvanceFrame( void )
{
    if ( !BIsProcessed() )
    {
        return false;
    }

    if ( m_flNextIterationTime >= ( float )Plat_FloatTime() )
    {
        return false;
    }

    SetFrame( m_iSelectedFrame + 1 );
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the material ptr for this GIF.
//-----------------------------------------------------------------------------
IMaterial *CGifMaterial::GetMaterial( void )
{
    return m_pMaterial;
}


//-----------------------------------------------------------------------------
// Purpose: Background worker that converts GIF frames into IVTFTexture
//-----------------------------------------------------------------------------
void CGifMaterial::Process( void *pvImage )
{
	GifFileType *pImage = ( GifFileType * )pvImage;

    int nBytesPerPixel = ImageLoader::SizeInBytes( IMAGE_FORMAT_RGBA8888 );

	int nWideSrc = pImage->SWidth;
	int nTallSrc = pImage->SHeight;
    int nWideDst = m_pTexture->GetMappingWidth();
    int nTallDst = m_pTexture->GetMappingHeight();

    int cubStrideSrc = nWideSrc * nBytesPerPixel;
	int cubStrideDst = nWideDst * nBytesPerPixel;

    int cubComposite = cubStrideSrc * nTallSrc;

    // Alloc our working buffers
    CUtlMemory< uint8 > memComposite, memPrevious;
    memComposite.EnsureCapacity( cubComposite );
    memPrevious.EnsureCapacity( cubComposite );
    Q_memset( memComposite.Base(), 0x00, cubComposite );
    Q_memset( memPrevious.Base(), 0x00, cubComposite );

    // Read the entire rest of the GIF
    if ( DGifSlurp( pImage ) != GIF_OK )
    {
        Warning( "Failed to slurp GIF image: %s\n", GifErrorString( pImage->Error ) );
        goto CleanUp;
    }

    // Process every GIF frame
    for ( int iFrame = 0; iFrame < pImage->ImageCount; ++iFrame )
    {
        rawframe_t &Frame = m_Frames[ m_Frames.AddToTail() ];

        GifImageDesc &Desc = pImage->SavedImages[ iFrame ].ImageDesc;
        ColorMapObject *pColorMap = Desc.ColorMap ? Desc.ColorMap : pImage->SColorMap;

        // Get at the graphics control block
        Frame.flDuration        = 0.f;
        Frame.iTransparentColor = NO_TRANSPARENT_COLOR;
        Frame.nDisposalMethod   = DISPOSAL_UNSPECIFIED;
        GraphicsControlBlock gcb;
        if ( DGifSavedExtensionToGCB( pImage, iFrame, &gcb ) == GIF_OK )
        {
            Frame.flDuration        = gcb.DelayTime * 0.01f;
            Frame.iTransparentColor = gcb.TransparentColor;
            Frame.nDisposalMethod   = gcb.DisposalMode;
        }

        // Draw over previous composite
        Q_memcpy( memComposite.Base(), memPrevious.Base(), cubComposite );

        // Render this frame onto the composite
        auto lambdaRenderFrameToComposite = [ & ]( int nRowOffset = 0, int nRowIncrement = 1 )
        {
            int iPixel = nRowOffset * Desc.Width;
            for ( int y = nRowOffset; y < Desc.Height; y += nRowIncrement )
            {
                int iSrcY = y + Desc.Top;
                if ( iSrcY >= nTallSrc ) { iPixel += Desc.Width; continue; }

                uint8 *pubScanLine = memComposite.Base() + ( iSrcY * cubStrideSrc ) + ( Desc.Left * nBytesPerPixel );
                for ( int x = 0; x < Desc.Width; ++x, ++iPixel )
                {
                    int iSrcX = x + Desc.Left;
                    if ( iSrcX >= nWideSrc ) continue;

                    GifByteType idx = pImage->SavedImages[ iFrame ].RasterBits[ iPixel ];
                    if ( idx < pColorMap->ColorCount && idx != Frame.iTransparentColor )
                    {
                        const GifColorType &color = pColorMap->Colors[ idx ];
                        pubScanLine[ 0 ] = color.Red;
                        pubScanLine[ 1 ] = color.Green;
                        pubScanLine[ 2 ] = color.Blue;
                        pubScanLine[ 3 ] = 255;
                    }
                    pubScanLine += nBytesPerPixel;
                }
            }
        };

        if ( Desc.Interlace )
        {
            // https://giflib.sourceforge.net/gifstandard/GIF89a.html#interlacedimages
            constexpr int k_rgnRowOffsets[] = { 0, 4, 2, 1 };
            constexpr int k_rgnRowIncrements[] = { 8, 8, 4, 2 };
            for ( int nPass = 0; nPass < 4; ++nPass )
            {
                lambdaRenderFrameToComposite( k_rgnRowOffsets[ nPass ], k_rgnRowIncrements[ nPass ] );
            }
        }
        else
        {
            lambdaRenderFrameToComposite();
        }

        CUtlMemory< uint8 > memResampled;
        if ( ( nWideSrc == nWideDst ) && ( nTallSrc == nTallDst ) )
        {
            // Fast path when there's nothing to resample
            memResampled.SetExternalBuffer( memComposite.Base(), memComposite.NumAllocated() );
        }
        else
        {
            memResampled.EnsureCapacity( ImageLoader::GetMemRequired( nWideDst, nTallDst, 0, IMAGE_FORMAT_RGBA8888, false ) );
            for ( int y = 0; y < nTallDst; ++y )
            {
                uint8 *pubRowDst = memResampled.Base() + y * cubStrideDst;
                uint8 *pubRowSrc = memComposite.Base() + y * cubStrideSrc;

                if ( y < nTallSrc )
                {
                    // Copy src row-by-row into top-left
                    Q_memcpy(
                        pubRowDst,
                        pubRowSrc,
                        cubStrideSrc
                    );

                    // Extend the texture on the left over space so we don't have bleeding after compressing to DXT
                    for ( int x = nWideSrc; x < nWideDst; ++x )
                    {
                        Q_memcpy(
                            pubRowDst + x * nBytesPerPixel,
                            pubRowDst + ( nWideSrc - 1 ) * nBytesPerPixel, // Last pixel
                            nBytesPerPixel
                        );
                    }

                    continue;
                }

                // Again extend but on Y axis
                Q_memcpy(
                    pubRowDst,
                    memResampled.Base() + ( nTallSrc - 1 ) * cubStrideDst, // Last row
                    cubStrideDst
                );
            }
        }

        // Handle disposal for the next frame
        switch ( Frame.nDisposalMethod )
        {
        case DISPOSE_BACKGROUND:
        {
            // Fill previous with transparency or background color
            GifByteType R, G, B, A;
            R = G = B = A = 0x00;

            if ( Frame.iTransparentColor == NO_TRANSPARENT_COLOR && pImage->SBackGroundColor < pImage->SColorMap->ColorCount )
            {
                GifColorType &color = pImage->SColorMap->Colors[ pImage->SBackGroundColor ];
                R = color.Red;
                G = color.Green;
                B = color.Blue;
                A = 0xFF;
            }

            int nWideFill = Min( Desc.Width, nWideSrc - Desc.Left );
            int nTallFill = Min( Desc.Height, nTallSrc - Desc.Top );

            for ( int y = 0; y < nTallFill; ++y )
            {
                uint8 *pubScanLine = memPrevious.Base() + ( ( y + Desc.Top ) * cubStrideSrc ) + Desc.Left * nBytesPerPixel;

                for ( int x = 0; x < nWideFill; ++x )
                {
                    pubScanLine[ 0 ] = R;
                    pubScanLine[ 1 ] = G;
                    pubScanLine[ 2 ] = B;
                    pubScanLine[ 3 ] = A;
                    pubScanLine += nBytesPerPixel;
                }
            }
            break;
        }
        case DISPOSE_PREVIOUS:
            // Keep previous composite
            break;
        case DISPOSAL_UNSPECIFIED:
        case DISPOSE_DO_NOT:
        default:
            // Copy current composite to previous
            Q_memcpy( memPrevious.Base(), memComposite.Base(), cubComposite );
            break;
        }

        // Compress the frame
		Frame.memTexture.EnsureCapacity( ImageLoader::GetMemRequired( nWideDst, nTallDst, 0, IMAGE_FORMAT_DXT5_RUNTIME, false ) );
		ImageLoader::ConvertImageFormat( memResampled.Base(), IMAGE_FORMAT_RGBA8888, Frame.memTexture.Base(), IMAGE_FORMAT_DXT5_RUNTIME, nWideDst, nTallDst );
    }

    m_ProcessedEvent.Set();

CleanUp:
    delete pImage->UserData;
    DGifCloseFile( pImage, NULL );
}


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGifTextureRegen::CGifTextureRegen( CGifMaterial *pOuter )
	: m_pOuter( pOuter )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CGifTextureRegen::RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pRect )
{
    if ( pVTFTexture->Format() != IMAGE_FORMAT_DXT5_RUNTIME )
    {
		pVTFTexture->ConvertImageFormat( IMAGE_FORMAT_DXT5_RUNTIME, false );
    }

    if ( !m_pOuter->BIsProcessed() )
    {
        // Temporarily initialize to a black texture
        Q_memset(
            pVTFTexture->ImageData(),
            0xFF,
            ImageLoader::GetMemRequired( pVTFTexture->Width(), pVTFTexture->Height(), 0, IMAGE_FORMAT_DXT5_RUNTIME, false )
        );
        return;
    }

	CGifMaterial::rawframe_t &Frame = m_pOuter->m_Frames[ m_pOuter->m_iSelectedFrame ];

    Q_memcpy(
        pVTFTexture->ImageData(),
        Frame.memTexture.Base(),
        Frame.memTexture.NumAllocated()
    );
}

//-----------------------------------------------------------------------------
// Purpose: Called by the material system when the texture is destroyed
//-----------------------------------------------------------------------------
void CGifTextureRegen::Release( void )
{
    delete this;
}


//-----------------------------------------------------------------------------
// Purpose: Deconstructor
//-----------------------------------------------------------------------------
CGifMaterialTracker::~CGifMaterialTracker( void )
{
    FOR_EACH_VEC( m_vecMaterials, i )
    {
        m_vecMaterials[ i ]->Purge();
    }
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CGifMaterialTracker::Register( CGifMaterial *pMaterial )
{
	m_vecMaterials.AddToTail( pMaterial );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CGifMaterialTracker::Unregister( CGifMaterial *pMaterial )
{
	m_vecMaterials.FindAndRemove( pMaterial );
}