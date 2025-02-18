//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"
#include "custom_texture_cache.h"
#include "materialsystem/imaterialproxy.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "pixelwriter.h"
#include "checksum_md5.h"
#include "imageutils.h"
#include "toolframework_client.h"
#include "econ_gcmessages.h"
#include "econ_item_inventory.h"
#if defined( TF_CLIENT_DLL )
#include "tf_dropped_weapon.h"
#endif // TF_CLIENT_DLL

#include "VGuiMatSurface/IMatSystemSurface.h"
#include "bitmap/bitmap.h"

using namespace CustomTextureSystem;

ITexture *CustomTextureSystem::g_pPreviewCustomTexture = NULL;

CEconItemView *CustomTextureSystem::g_pPreviewEconItem = NULL;

bool CustomTextureSystem::g_pPreviewCustomTextureDirty = true;

const char CustomTextureSystem::k_rchCustomTextureFilterPreviewImageName[] = "__CustomTextureFilterPreview";
const char CustomTextureSystem::k_rchCustomTextureFilterPreviewTextureName[] = "vgui/__CustomTextureFilterPreview";

//-----------------------------------------------------------------------------

static ISteamRemoteStorage *GetISteamRemoteStorage()
{
	return steamapicontext?steamapicontext->SteamRemoteStorage():NULL;
//	return Steam3Client().SteamRemoteStorage();
}

static void CalcMD5Ascii( char *szDigestAscii, const void *data, int dataSz )
{
	MD5Context_t context;
	unsigned char digest[ MD5_DIGEST_LENGTH ];
	MD5Init( &context );
	MD5Update( &context, (const unsigned char *)data, dataSz );
	MD5Final( digest, &context );
	Q_binarytohex( digest, MD5_DIGEST_LENGTH, szDigestAscii, MD5_DIGEST_LENGTH*2+1 );
}

static bool BReadSteamRemoteFileToBuffer( CUtlBuffer &outBuffer, const char *pchRemoteFilename )
{
	outBuffer.Purge();

	ISteamRemoteStorage *pRemoteStorage = GetISteamRemoteStorage();
	if ( !pRemoteStorage )
		return false;
	if ( !pRemoteStorage->FileExists( pchRemoteFilename ))
		return false;
	int nFileSize = pRemoteStorage->GetFileSize( pchRemoteFilename );
	if ( nFileSize <= 0 )
		return false;

	// Allocate space
	outBuffer.SeekPut( CUtlBuffer::SEEK_HEAD, nFileSize );
	int nSizeRead = pRemoteStorage->FileRead( pchRemoteFilename, outBuffer.Base(), nFileSize );
	return ( nSizeRead == nFileSize );
}

//-----------------------------------------------------------------------------
// Local cache of custom images.  This cache contains files from the cloud, and
// and also a few virtual textures that are used during autioning / tweaking

/// Name of cloud-backed config file remembering the custom images that the user
/// has uploaded to the cloud.
static const char k_szCustomTextureRecentListFilename[] = "stamped_items_mru.txt";

/// Track a single entry
struct SCustomImageCacheEntry : private ITextureRegenerator
{

	/// If this has been assigned a cloud ID, what is it?
	UGCHandle_t m_hCloudID;

	/// If this is one of our files, then we know the MD5.
	/// This is empty for other people's files
	char m_szDigestAscii[ MD5_DIGEST_LENGTH*2 + 4];

	//
	// Bookkeeping for steam downloads of UGC.
	//

	/// -1 = failure, 0 = not started, 1 = in progress, 2 = finished OK and image should be in memory
	int	m_nStatus;

	/// Handle to the active download, or k_uAPICallInvalid if not active
	SteamAPICall_t m_hDownloadApiCall;

	/// Procedural texture object.  We hold a reference.
	ITexture *m_pTexture;

	/// Procedurally-created material.  We hold a reference.
	IMaterial *m_pMaterial;

	/// GUI texture handle.  (It's bound to the material.)
	int m_iVguiHandle;

	/// The raw image
	Bitmap_t m_image;

	/// Doubly-linked list.  We keep it in MRU order so we know what to eject from the cache
	SCustomImageCacheEntry *m_pPrev;
	SCustomImageCacheEntry *m_pNext;

	SCustomImageCacheEntry()
		: m_hCloudID(0)
		, m_pTexture(NULL)
		, m_nStatus(0)
		, m_hDownloadApiCall(k_uAPICallInvalid)
		, m_pPrev(NULL)
		, m_pNext(NULL)
		, m_pMaterial(NULL)
		, m_iVguiHandle(0)
	{
		m_szDigestAscii[0] = '\0';
	}

	virtual ~SCustomImageCacheEntry()
	{
		Clear();
	}

	// Release texture / VGUI resources.  This doesn't free the image we have
	// loaded or stop any async actions that were in progress.  (Use Clear())
	void ReleaseResources()
	{
		if ( m_pTexture )
		{
			ITexture *tex = m_pTexture;
			m_pTexture = NULL; // clear pointer first, to prevent infinite recursion
			tex->SetTextureRegenerator( NULL );
			tex->Release();
		}
		if ( m_pMaterial )
		{
			m_pMaterial->Release();
			m_pMaterial = NULL;
		}
		if ( m_iVguiHandle != 0 )
		{
			g_pMatSystemSurface->DestroyTextureID( m_iVguiHandle );
			m_iVguiHandle = 0;
		}
	}

	/// Inherited from ITextureRegenerator
	///
	/// Gets called when our ITextureRegenerator interface gets detached from the texture.
	/// We should be the only ones doing this --- so that means we had better have already
	/// cleared our texture at this point!
	virtual void Release()
	{
		Assert( m_pTexture == NULL );
	}

	/// Inherited from ITextureRegenerator
	///
	/// The main interface function that actually supplies the texture bits
	virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pRect )
	{
		if ( pVTFTexture->FrameCount() != 1 || pVTFTexture->FaceCount() != 1 || pTexture != m_pTexture || pTexture->IsMipmapped() )
		{
			Warning( "Custom image invalid VTF setup.\n" );
			return;
		}

		int nWidth, nHeight, nDepth;
		pVTFTexture->ComputeMipLevelDimensions( 0, &nWidth, &nHeight, &nDepth );

		if ( nWidth != m_image.Width() || nHeight != m_image.Height() || nDepth != 1 )
		{
			Warning( "Custom image had invalid w/h/d: %dx%dx%d vs %dx%dx%d\n", m_image.Width(), m_image.Height(), 1, nWidth, nHeight, nDepth );
			return;
		}

		CPixelWriter pixelWriter;
		pixelWriter.SetPixelMemory( pVTFTexture->Format(), 
			pVTFTexture->ImageData( 0, 0, 0 ), pVTFTexture->RowSizeInBytes( 0 ) );

		// !SPEED! 'Tis probably DEATHLY slow...
		for ( int y = 0; y < nHeight; ++y )
		{
			pixelWriter.Seek( 0, y );
			for ( int x = 0; x < nWidth; ++x )
			{
				Color c = m_image.GetColor( x, y );
				pixelWriter.WritePixel( c.r(), c.g(), c.b(), c.a() );
			}
		}
	}

	void Clear()
	{
		ReleaseResources();
		m_image.Clear();
		m_szDigestAscii[0] = '\0';
		m_nStatus = 0;
		m_hCloudID = 0;

		// !KLUDGE! How can I clean this up properly if something is
		// in progress?
		m_hDownloadApiCall = k_uAPICallInvalid;
	}

	/// Poll the entry and update bookeeping if we're busy.
	void Poll()
	{

		// We must know our cloud ID
		if ( m_hCloudID == 0 )
		{
			Assert( m_hCloudID != 0 );
			return;
		}

		// If texture already exists, then we are definitely done!
		if ( m_pTexture )
		{
			Assert( m_nStatus == 2 );
			return;
		}

		// Check if we have not yet initiated anything
		if ( m_nStatus == 0 )
		{

			// We'll need to download it.
			// Start by assuming failure.
			m_nStatus = -1;

			// Start download
			ISteamRemoteStorage *pRemoteStorage = GetISteamRemoteStorage();
			if ( pRemoteStorage )
			{
				m_hDownloadApiCall = pRemoteStorage->UGCDownload( m_hCloudID, 0 );
				if ( m_hDownloadApiCall != k_uAPICallInvalid )
				{
					// Mark download as in progress
					Msg( "Started download of cloud file %08X%08X\n", (uint32)(m_hCloudID>>32), (uint32)m_hCloudID );
					m_nStatus = 1;
				}
			}
		}

		// If we're in progress, poll the result
		if ( m_nStatus == 1 )
		{
			PollDownload();
		}

		// If result has completed, then fetch the texture
		Assert( m_pTexture == NULL );
		if ( m_nStatus == 2 && m_image.IsValid() )
		{
			// Generate the logical texture name
			char rchTextureName[MAX_PATH];
			GenerateLocalTextureName( rchTextureName );

			ITexture *pTexture = NULL;
			if ( g_pMaterialSystem->IsTextureLoaded( rchTextureName ) )
			{
				pTexture = g_pMaterialSystem->FindTexture( rchTextureName, TEXTURE_GROUP_VGUI );
				pTexture->AddRef();
				Assert( pTexture );
			}
			else
			{
				pTexture = g_pMaterialSystem->CreateProceduralTexture(
					rchTextureName,
					TEXTURE_GROUP_VGUI,
					k_nCustomImageSize, k_nCustomImageSize, 
					IMAGE_FORMAT_RGBA8888,
					TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_NOMIP | TEXTUREFLAGS_NOLOD
				);
				Assert( pTexture );
			}
			pTexture->SetTextureRegenerator( this ); // note carefully order of operations here.  See Release()
			m_pTexture = pTexture;

			// Upload the data now
			m_pTexture->Download();
		}
		else
		{
			Assert( m_nStatus < 2 );
			Assert( !m_image.IsValid() );
		}
	}

	void PollDownload()
	{

		Assert( m_nStatus == 1 );

		// Sanity check we have everything we need
		ISteamRemoteStorage *pRemoteStorage = GetISteamRemoteStorage();
		if ( m_hDownloadApiCall == k_uAPICallInvalid || !steamapicontext || !steamapicontext->SteamUtils() || !pRemoteStorage )
		{
			// ???
			Assert( m_hDownloadApiCall != k_uAPICallInvalid );
			Assert( steamapicontext && steamapicontext->SteamUtils() );
			Assert( pRemoteStorage );
			m_nStatus = -1;
			return;
		}

		// Poll progress
		bool bFailed;
		RemoteStorageDownloadUGCResult_t result;
		if ( !steamapicontext->SteamUtils()->GetAPICallResult(m_hDownloadApiCall,
			&result, sizeof(result), RemoteStorageDownloadUGCResult_t::k_iCallback, &bFailed) )
		{
			// Still busy.
			return;
		}

		// Make sure we got back the file we were expecting
		Assert( result.m_hFile == m_hCloudID );

		// Clear status, mark success
		m_hDownloadApiCall = k_uAPICallInvalid;

		// Completed.  Did we succeed?
		if ( bFailed )
		{
			Warning( "Download of custom image file from UFS (UGC=%08X%08X) failed.\n", (uint32)(m_hCloudID >> 32), (uint32)(m_hCloudID) );
			m_nStatus = -1;
			return;
		}

		// Fetch file details
		AppId_t nAppID;
		char *pchName;
		int32 nFileSizeInBytes = -1;
		CSteamID steamIDOwner;
		if ( !pRemoteStorage->GetUGCDetails( m_hCloudID, &nAppID, &pchName, &nFileSizeInBytes, &steamIDOwner )
			|| nFileSizeInBytes <= 0 || nFileSizeInBytes >= k_nMaxCustomImageFileSize )
		{
			Warning( "GetUGCDetails failed? (UGC=%08X%08X nFileSizeInBytes=%d).\n", (uint32)(m_hCloudID >> 32), (uint32)(m_hCloudID), nFileSizeInBytes );
			m_nStatus = -1;
			return;
		}

		// Load the file data
		CUtlBuffer fileData;
		fileData.SeekPut( CUtlBuffer::SEEK_HEAD, nFileSizeInBytes );

		// Read in the data.  Phil says this is supposed to be basically a memcpy
		// or some other fast, local operation.
		if ( pRemoteStorage->UGCRead( m_hCloudID, fileData.Base( ), nFileSizeInBytes, 0, k_EUGCRead_ContinueReadingUntilFinished ) != nFileSizeInBytes )
		{
			Warning( "UGCRead failed? (UGC=%08X%08X).\n", (uint32)(m_hCloudID >> 32), (uint32)(m_hCloudID) );
			m_nStatus = -1;
			return;
		}

		uint32_t uWidth, uHeight;
		if ( ImgUtl_GetPNGSize( fileData, uWidth, uHeight ) != CE_SUCCESS )
		{
			Warning( "Corrupt PNG file, UGC=%08X%08X.\n", (uint32)(m_hCloudID >> 32), (uint32)(m_hCloudID) );
			m_nStatus = -1;
			return;
		}

		if ( uWidth != k_nCustomImageSize || uHeight != k_nCustomImageSize )
		{
			Warning( "Custom image with illegal size, rejecting, UGC=%08X%08X.\n", (uint32)(m_hCloudID >> 32), (uint32)(m_hCloudID) );
			m_nStatus = -1;
			return;
		}

		// Parse the PNG file data
		if ( ImgUtl_LoadPNGBitmapFromBuffer( fileData, m_image ) != CE_SUCCESS )
		{
			Warning( "Corrupt PNG file, UGC=%08X%08X.\n", (uint32)(m_hCloudID >> 32), (uint32)(m_hCloudID) );
			m_nStatus = -1;
			return;
		}

		// We have the raw data
		m_nStatus = 2;
	}

	int GetGuiHandle()
	{
		// Should never be called on entries without a cloud ID
		if ( m_hCloudID == 0 )
		{
			Assert( m_hCloudID != 0 );
			return 0;
		}

		// Already have one?
		if ( m_iVguiHandle != 0 )
		{
			return m_iVguiHandle;
		}

		// Process texture downloading, etc
		Poll();

		// If we don't have a texture yet, or don't know our logical name, then we cannot draw
		if ( m_pTexture == NULL )
		{
			return 0;
		}

		// Make a material, if we don't already have one
		if ( m_pMaterial == NULL )
		{

			// Generate the material name
			char rchImageName[MAX_PATH], rchMaterialName[MAX_PATH];
			GenerateLocalImageNameBase( rchImageName );
			Q_snprintf( rchMaterialName, MAX_PATH, "vgui/%s.mtl", rchImageName );

			// Does it already exist?
			if ( g_pMaterialSystem->IsMaterialLoaded( rchMaterialName ) )
			{
				m_pMaterial = g_pMaterialSystem->FindMaterial( rchMaterialName, TEXTURE_GROUP_VGUI );
				Assert( m_pMaterial );
			}
			else
			{

				// Fetch the texture name
				char rchTextureName[MAX_PATH];
				GenerateLocalTextureName( rchTextureName );

				// Create dummy material KV data
				KeyValues *pVMTKeyValues = new KeyValues( "UnlitGeneric" );
				pVMTKeyValues->SetString( "$basetexture", rchTextureName );
				pVMTKeyValues->SetInt( "$vertexcolor", 1 );
				pVMTKeyValues->SetInt( "$vertexalpha", 1 );
				pVMTKeyValues->SetInt( "$translucent", 1 );

				// Create the material
				m_pMaterial = g_pMaterialSystem->CreateMaterial(
					rchMaterialName,
					pVMTKeyValues
				);
			}

			// Bind the material to a new VGUI texture object
			m_iVguiHandle = g_pMatSystemSurface->CreateNewTextureID();
			g_pMatSystemSurface->DrawSetTextureMaterial( m_iVguiHandle, m_pMaterial );
		}

		return m_iVguiHandle;
	}

	// Generate logical image name, with no leading materials or vgui directories
	// nor a file extension.
	void GenerateLocalImageNameBase( char *result ) const
	{
		Assert( m_hCloudID != 0 );

		// Generate the local filenames.  !KLUDGE! I'm not sure the platform-safe way
		// to print a 64-bit int, so I'll just print both halves myself
		Q_snprintf( result, 64, "cloud_custom_images/%08X%08X", (uint32)(m_hCloudID >> 32), (uint32)(m_hCloudID) );
	}

	/// Logical texture name, including "vgui" but not "materials"
	void GenerateLocalTextureName( char *result ) const
	{
		char rchImageName[MAX_PATH];
		GenerateLocalImageNameBase( rchImageName );
		Q_snprintf( result, MAX_PATH, "vgui/%s.vtf", rchImageName );
	}

	/// Full local filename, including leading "materials" directory
	void GenerateLocalFilename( char *result ) const
	{
		char szLocalTextureName[MAX_PATH];
		GenerateLocalTextureName( szLocalTextureName );

		Q_snprintf( result, MAX_PATH, "materials/%s", szLocalTextureName );
	}
};

/// Head of linked list of entries, in MRU order
static SCustomImageCacheEntry *mruCustomImageEntry = NULL;

/// Map of entries, indexed by cloud ID
typedef CUtlMap<UGCHandle_t, SCustomImageCacheEntry *, int> tCustomTextureInfoMap;
static tCustomTextureInfoMap g_mapCustomTextureInfoByCloudId( DefLessFunc(UGCHandle_t) );

// Remove from linked list, without deleting.  The item must already be in the list
static void CustomTextureCache_Remove(SCustomImageCacheEntry *pEntry)
{
	Assert( pEntry );

	// List had better not be empty.  Commence paranoia.
	Assert( mruCustomImageEntry );
	Assert( !mruCustomImageEntry->m_pPrev );

	SCustomImageCacheEntry *p = pEntry->m_pPrev;
	SCustomImageCacheEntry *n = pEntry->m_pNext;

	// Detach from next, if we're not last
	if ( n != NULL )
	{
		Assert( n->m_pPrev == pEntry);
		n->m_pPrev = p;
	}

	// At the head?
	if ( !p )
	{
		Assert( mruCustomImageEntry == pEntry );
		mruCustomImageEntry = n;
	}
	else
	{

		// Detach from previous
		Assert( p->m_pNext == pEntry );
		p->m_pNext = n;
	}

	// Clear pointers
	pEntry->m_pPrev = pEntry->m_pNext = NULL;
}

// Insert the item at the head (MRU) slot.  The item shouldn't
// already be in the list
static void CustomTextureCache_InsertAtHead(SCustomImageCacheEntry *pEntry)
{
	Assert( pEntry );
	Assert( !pEntry->m_pNext );
	Assert( !pEntry->m_pPrev );

	// Edge case of inserting into empty list
	if ( mruCustomImageEntry )
	{
		Assert( !mruCustomImageEntry->m_pPrev );
		mruCustomImageEntry->m_pPrev = pEntry;
	}
	else
	{
		// Inserting into an empty list
	}

	// Do the head insertion.
	pEntry->m_pNext = mruCustomImageEntry;
	mruCustomImageEntry = pEntry;
}

// Reorder list, setting item at the head (MRU) slot.  The item must already
// be in the list somewhere.
static void CustomTextureCache_SetMRU(SCustomImageCacheEntry *pEntry)
{
	// Note: even if we are already at the head, go through the motions, anyway,
	// to exercise all of the sanity checking code.
	CustomTextureCache_Remove(pEntry);
	CustomTextureCache_InsertAtHead(pEntry);
}

static SCustomImageCacheEntry *CustomTextureCache_NewEntry()
{

	SCustomImageCacheEntry *pEntry = new SCustomImageCacheEntry;

	// Go ahead and put us at the head
	CustomTextureCache_InsertAtHead(pEntry);

	// Return the new entry
	return pEntry;
}

static SCustomImageCacheEntry *CustomTextureCache_FindOrAddByCloudId( UGCHandle_t ugcHandle )
{

	// Locate the bookeeping entry, if one exists
	int idx = g_mapCustomTextureInfoByCloudId.Find( ugcHandle );
	SCustomImageCacheEntry *pEntry;
	if ( g_mapCustomTextureInfoByCloudId.IsValidIndex( idx ) )
	{
		pEntry = g_mapCustomTextureInfoByCloudId[idx];

		// We're accessing it, so move it to the head, the MRU slot
		CustomTextureCache_SetMRU(pEntry);
	}
	else
	{
		// Grab a new entry
		pEntry = CustomTextureCache_NewEntry();

		// Assign the cloud ID
		pEntry->m_hCloudID = ugcHandle;

		// Add it to the map by cloud ID
		idx = g_mapCustomTextureInfoByCloudId.Insert( ugcHandle );
		g_mapCustomTextureInfoByCloudId[idx] = pEntry;
	}

	// Return the entry
	return pEntry;
}

// Locate an entry by hash create a new entry if one doesn't already exist
static SCustomImageCacheEntry *CustomTextureCache_FindOrAddByDigest( const char *szDigestAscii )
{
	Assert( strlen(szDigestAscii) == MD5_DIGEST_LENGTH*2 );

	// Brute-force linear search.  This should never be called in time-critical
	// situations
	SCustomImageCacheEntry *pEntry = mruCustomImageEntry;
	while ( pEntry )
	{

		// Match?
		if ( !Q_stricmp(pEntry->m_szDigestAscii, szDigestAscii) )
		{

			// Found.  Se at MRU and return it.
			CustomTextureCache_SetMRU(pEntry);
			return pEntry;
		}

		// Keep looking
		pEntry = pEntry->m_pNext;
	}

	// Not found.  Make a new entry
	pEntry = CustomTextureCache_NewEntry();
	V_strcpy_safe(pEntry->m_szDigestAscii, szDigestAscii);
	return pEntry;
}

//-----------------------------------------------------------------------------
int GetCustomTextureGuiHandle( uint64 hCloudId )
{

	// Find or create the entry
	SCustomImageCacheEntry *pEntry = CustomTextureCache_FindOrAddByCloudId( hCloudId );

	// Poll entry and return GUI handle if it's finally ready
	return pEntry->GetGuiHandle();
}

//-----------------------------------------------------------------------------

class CCustomTextureOnItemProxy : public IMaterialProxy
{
public:
	CCustomTextureOnItemProxy();
	virtual ~CCustomTextureOnItemProxy();

	virtual bool Init( IMaterial* pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );
	virtual void Release();
	virtual IMaterial *GetMaterial();

protected:
	virtual void	OnBindInternal( CEconItemView *pScriptItem );

private:
	IMaterialVar	*m_pBaseTextureVar;
	ITexture		*m_pOriginalTexture;
};

EXPOSE_INTERFACE( CCustomTextureOnItemProxy, IMaterialProxy, "CustomSteamImageOnModel" IMATERIAL_PROXY_INTERFACE_VERSION );

CCustomTextureOnItemProxy::CCustomTextureOnItemProxy()
: m_pBaseTextureVar( NULL )
, m_pOriginalTexture( NULL )
{
	
}

CCustomTextureOnItemProxy::~CCustomTextureOnItemProxy()
{
}

bool CCustomTextureOnItemProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	Release();

	bool found = false;
	m_pBaseTextureVar = pMaterial->FindVar( "$basetexture", &found );
	if ( !found )
	{
		return false;
	}

	// No! Don't do this until, because the material/texture might
	// not have been cached.  If we call this, it causes the material
	// to try to get cached, but instead of loading the texture
	// synchronously, it just goes into a queue, and we get the error
	// texture instead.  We'll just defer it until later when we know
	// for sure that everything is ready to go.
	//m_pOriginalTexture = m_pBaseTextureVar->GetTextureValue();
	//if ( m_pOriginalTexture )
	//{
	//	m_pOriginalTexture->AddRef();
	//}
	return true;
}

void CCustomTextureOnItemProxy::OnBind( void *pC_BaseEntity )
{
	if ( pC_BaseEntity )
	{
		CEconItemView *pScriptItem = NULL;
		IClientRenderable *pRend = (IClientRenderable *)pC_BaseEntity;
		C_BaseEntity *pEntity = pRend->GetIClientUnknown()->GetBaseEntity();
		if ( pEntity )
		{
			CEconEntity *pItem = dynamic_cast< CEconEntity* >( pEntity );
			if ( pItem )
			{
				pScriptItem = pItem->GetAttributeContainer()->GetItem();
			}
#if defined( TF_CLIENT_DLL )
			else
			{
				CTFDroppedWeapon *pDroppedWeapon = dynamic_cast<CTFDroppedWeapon *>( pEntity );
				if ( pDroppedWeapon )
				{
					pScriptItem = pDroppedWeapon->GetItem();
				}
			}
#endif // TF_CLIENT_DLL
		}
		else
		{
			// Proxy data can be a script created item itself, if we're in a vgui CModelPanel
			pScriptItem = dynamic_cast< CEconItemView* >( pRend );
		}
		if ( pScriptItem )
		{
			OnBindInternal( pScriptItem );
		}
	}
}

void CCustomTextureOnItemProxy::Release()
{
	if ( m_pOriginalTexture )
	{
		m_pOriginalTexture->Release();
		m_pOriginalTexture = NULL;
	}
}

IMaterial *CCustomTextureOnItemProxy::GetMaterial()
{
	return m_pBaseTextureVar->GetOwningMaterial();
}

void CCustomTextureOnItemProxy::OnBindInternal( CEconItemView *pScriptItem )
{
	if ( !m_pBaseTextureVar || !m_pBaseTextureVar->IsTexture() )
	{
		return;
	}

	// Snag the original texture object the first time.
	// And make sure we're 100% ready to go.
	if ( m_pOriginalTexture == NULL )
	{
		m_pOriginalTexture = m_pBaseTextureVar->GetTextureValue();
		if ( m_pOriginalTexture == NULL )
		{
			return;
		}
		if ( m_pOriginalTexture->IsError() )
		{
			m_pOriginalTexture = NULL;
			return;
		}

		// Success! Let's hang on to this guy
		m_pOriginalTexture->AddRef();
	}
	ITexture *texture = m_pOriginalTexture;

	// Fetch the UGC handle from the item
	UGCHandle_t ugcHandle = pScriptItem->GetCustomUserTextureID();

	// Are we in a preview window?
	if ( pScriptItem == g_pPreviewEconItem ) // !KLUDGE!
	{
		Assert( g_pPreviewCustomTexture );
		if ( g_pPreviewCustomTexture )
		{
			texture = g_pPreviewCustomTexture;

			// Re-fetch the bits if necessary
			if ( g_pPreviewCustomTextureDirty )
			{
				g_pPreviewCustomTexture->Download();
				Assert( !g_pPreviewCustomTextureDirty );
			}
		}
	}
	else if (ugcHandle != 0)
	{

		SCustomImageCacheEntry *pEntry = CustomTextureCache_FindOrAddByCloudId(ugcHandle);
		pEntry->Poll();
		texture = pEntry->m_pTexture; // might be NULL if texture isn't ready yet
	}

	if ( texture )
	{
		m_pBaseTextureVar->SetTextureValue( texture );
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

//-----------------------------------------------------------------------------
// The custom texture cache needs to init/shutdown and get some frame ticking
//-----------------------------------------------------------------------------
class CCustomTextureToolCache : public CBaseGameSystemPerFrame
{
public:
	CCustomTextureToolCache() {}
	virtual ~CCustomTextureToolCache() {}

	//
	// CAutoGameSystemPerFrame overrides
	//
	virtual char const *Name()
	{
		return "CCustomTextureToolCache";
	}
	virtual bool Init()
	{
		return true;
	}

	virtual void Shutdown()
	{

		// Destroy all the cache entries
		SCustomImageCacheEntry *pEntry = mruCustomImageEntry;
		mruCustomImageEntry = NULL;
		while ( pEntry != NULL )
		{
			SCustomImageCacheEntry *pNext = pEntry->m_pNext;
			delete pEntry;
			pEntry = pNext;
		}
	}

	// At level shutdown, release all of our GPU resources.
	// We'll still hang on to the bitmap data, since it isn't
	// that large and is in regular virtual memory which is easily
	// swapped out if stale.  But the video RAM we want to be more
	// agressive at cleaning out.
	virtual void LevelShutdownPreEntity()
	{
		// Destroy all the cache entries
		for ( SCustomImageCacheEntry *pEntry = mruCustomImageEntry ; pEntry ; pEntry = pEntry->m_pNext )
		{	
			pEntry->ReleaseResources();
		}
	}

// CAutoGameSystemPerFrame defines different stuff depending on which DLL we're building
#ifdef CLIENT_DLL

	// Do our frame-time processing after rendering
	virtual void PostRender()
	{
		// !FIXME! Here's where we should scan the list and eject
		// entries that haven't been used recently to limit the
		// hardware resources we're using.
	}
#else
	// This file shouldn't be compiled outside of client.dll.  Right?
	#error "Say what?"
#endif
};

static CCustomTextureToolCache s_CustomTextureToolCache;
IGameSystem *CustomTextureToolCacheGameSystem()
{
	return &s_CustomTextureToolCache;
}


CApplyCustomTextureJob::CApplyCustomTextureJob( itemid_t nToolItemID, itemid_t nSubjectItemID, const void *pPNGData, int nPNGDataBytes )
: GCSDK::CGCClientJob( GCClientSystem()->GetGCClient() )
, m_nToolItemID( nToolItemID )
, m_nSubjectItemID( nSubjectItemID )
, m_hCloudID( 0 )
{
	m_chRemoteStorageName[0] = '\0';
	m_bufPNGData.Put( pPNGData, nPNGDataBytes );
}

bool CApplyCustomTextureJob::BYieldingRunGCJob()
{
	YieldingRunJob();
	CleanUp();
	return true;
}

void CApplyCustomTextureJob::CleanUp()
{
	// If we had a cloud file, delete it from the logical
	// cloud filespace.  We are using the cloud system really just
	// to get the file into the UGC system and get a handle to it.
	// But once it's up there, it really isn't this user.  They will
	// fetch it by UGC handle just like any other user.  They paid
	// for this action, and we don't want it taking up any of their
	// quota.
	ISteamRemoteStorage *pRemoteStorage = GetISteamRemoteStorage();
	if ( pRemoteStorage && m_chRemoteStorageName[0] != '\0' )
	{
		pRemoteStorage->FileDelete( m_chRemoteStorageName );
	}
}

EResult CApplyCustomTextureJob::YieldingRunJob()
{
	EResult result = YieldingFindFileIncacheOrUploadFileToCDN();
	if ( result != k_EResultOK )
	{
		return result;
	}
	Assert( m_hCloudID != 0 );

	result = YieldingApplyTool();
	if ( result != k_EResultOK )
	{
		return result;
	}

	// OK!
	return k_EResultOK;
}

EResult CApplyCustomTextureJob::YieldingFindFileIncacheOrUploadFileToCDN()
{

	int nFileSize = m_bufPNGData.TellPut();
	Assert( nFileSize <= k_nMaxCustomImageFileSize ); // what the heck is out image converter doing?!

	// Generate the hash
	char szDigestAscii[ MD5_DIGEST_LENGTH*2 + 4];
	CalcMD5Ascii( szDigestAscii, m_bufPNGData.Base(), nFileSize );

	// Find or create an existing cache entry
	SCustomImageCacheEntry *pSelectedCacheEntry = CustomTextureCache_FindOrAddByDigest( szDigestAscii );

	KeyValuesAD pkvMruFile( "StampedItems" );

	{
		// Load up list of images recently used and uploaded
		CUtlBuffer listFileData;
		listFileData.SetBufferType( true, true );
		if ( BReadSteamRemoteFileToBuffer( listFileData, k_szCustomTextureRecentListFilename ) )
		{
			if ( !pkvMruFile->LoadFromBuffer( k_szCustomTextureRecentListFilename, listFileData ) )
			{
				pkvMruFile->Clear();
			}
		}
	}
	KeyValues *pkvMruUploadedImages = pkvMruFile->FindKey( "Uploaded", true );

	// !FIXME! Check for duplicates!

	// Make sure we are ready
	Assert( pSelectedCacheEntry != NULL );
	Assert( pSelectedCacheEntry->m_hCloudID == 0 );
	Assert( strlen(pSelectedCacheEntry->m_szDigestAscii) == MD5_DIGEST_LENGTH*2 );
	Assert( m_bufPNGData.TellPut() > 0 );

	// Generate filename in the cloud file space.  Each user has their own
	// namespace, and Phil requested that we keep the filenames simple
	// and easily optimizeable by string table.  (I.e. don't use the
	// hash or something else)
	//
	// We *could* just always use the same filename, and each file would
	// be its own "version."  But that doesn't seem to be the proper
	// spirit of the cloud system.  So I'll just use a simple integer name,
	// based on how many images they have uploaded.  It isn't critical what
	// this logical filename is, because once the GC gets a message to tag the file,
	// that UGC ID should always refer to that version of the file and can never
	// be changed or deleted, even if we reuse the filename.
	int iFileIndex = 1;
	KeyValues *pKey;
	for ( pKey = pkvMruUploadedImages->GetFirstTrueSubKey() ; pKey ; pKey = pKey->GetNextTrueSubKey() )
	{
		int index = atoi(pKey->GetName());
		iFileIndex = MAX( iFileIndex, index+1 );
	}
	Q_snprintf( m_chRemoteStorageName, sizeof( m_chRemoteStorageName ), "my_custom_images/%d.png", iFileIndex );

	// Write the local copy of the file
	Msg( "Saving %s to cloud....\n", m_chRemoteStorageName );
	ISteamRemoteStorage *pRemoteStorage = GetISteamRemoteStorage();
	if ( !pRemoteStorage || !pRemoteStorage->FileWrite( m_chRemoteStorageName, m_bufPNGData.Base(), m_bufPNGData.TellPut() ) )
	{
		Warning( "Failed to save local copy of custom image %s\n", m_chRemoteStorageName);
		return k_EResultFail;
	}

	// Share it.  This initiates the upload to cloud
	Msg( "Starting upload of %s to UFS....\n", m_chRemoteStorageName );
	SteamAPICall_t hFileShareApiCall = pRemoteStorage->FileShare( m_chRemoteStorageName );
	if ( hFileShareApiCall == k_uAPICallInvalid )
	{
		return k_EResultFail;
	}

	bool bFailed;
	RemoteStorageFileShareResult_t shareResult;
	while ( !steamapicontext->SteamUtils()->GetAPICallResult(hFileShareApiCall,
		&shareResult, sizeof(shareResult), RemoteStorageFileShareResult_t::k_iCallback, &bFailed) )
	{
		BYield();
	}

	if ( bFailed || shareResult.m_eResult != k_EResultOK )
	{
		Warning( "Custom texture uploaded to cloud FAILED\n" );
		return k_EResultFail;
	}

	Msg( "Custom texture uploaded to cloud completed OK, assigned UGC ID %08X%08X\n", (uint32)(shareResult.m_hFile >> 32), (uint32)(shareResult.m_hFile) );

	// Remember the handle to the cloud file
	m_hCloudID = pSelectedCacheEntry->m_hCloudID = shareResult.m_hFile;

	// Update the MRU list
	pKey = pkvMruUploadedImages->GetFirstTrueSubKey();
	while ( pKey )
	{
		int index = atoi(pKey->GetName());
		int mruValue = pKey->GetInt( "mru", 0 );
		const char *entryDigsetAscii = pKey->GetString("md5", "");
		UGCHandle_t ugcID = pKey->GetUint64( "ugcid", 0);
		if ( index <= 0 || mruValue <= 0 || strlen(entryDigsetAscii) != MD5_DIGEST_LENGTH*2 || ugcID == 0 )
		{
			// Bah!  Bogus data!
			Assert(false);
			continue;
		}

		// Is this the one they selected?
		if ( ugcID == pSelectedCacheEntry->m_hCloudID )
		{

			// This *can* happen if the list file gets lost and they reuse an image.  It means we are wasting
			// some of their cloud quota, but should be rare, and it's harmless.
			Assert( !Q_stricmp(entryDigsetAscii, pSelectedCacheEntry->m_szDigestAscii) );
			break;
		}

		pKey = pKey->GetNextTrueSubKey();
	}

	// Found it?
	int oldIndex = 0x7fffffff;
	if ( pKey )
	{

		// Renumber them in MRU order
		oldIndex = pKey->GetInt( "mru", 1 );
	}
	else
	{

		// Create a new key
		pKey = pkvMruUploadedImages->CreateNewKey();

		// Remember hash and cloud file location in subkeys
		pKey->SetString( "md5", pSelectedCacheEntry->m_szDigestAscii );
		pKey->SetUint64( "ugcid", pSelectedCacheEntry->m_hCloudID );
		//pKey->SetString( "remoteStorageName", m_chSelectedRemoteStorageNameBase );
	}
	for ( KeyValues *p = pkvMruUploadedImages->GetFirstTrueSubKey() ; p ; p = p->GetNextTrueSubKey() )
	{
		if ( p != pKey )
		{
			int mruValue = p->GetInt( "mru", 0 );
			Assert( mruValue > 0 );
			if (mruValue < oldIndex)
			{
				p->SetInt( "mru", mruValue+1 );
			}
		}
	}

	pKey->SetInt( "mru", 1);

	// Re-save the cloud-backed MRU list file
	Msg( "Saving MRU list file %s\n", k_szCustomTextureRecentListFilename );
	if ( pRemoteStorage )
	{
		CUtlBuffer listFileData;
		listFileData.SetBufferType( true, true );
		pkvMruFile->RecursiveSaveToFile( listFileData, 0 );
		pRemoteStorage->FileWrite( k_szCustomTextureRecentListFilename, listFileData.Base(), listFileData.TellPut() );
	}

	return k_EResultOK;
}

EResult CApplyCustomTextureJob::YieldingApplyTool()
{

	Msg( "Sending tool request to GC.\n" );

	// At this point, we need to know the cloud ID and hash of the image we are applying
	Assert( m_hCloudID != 0 );

	// Send the message to the GC
	GCSDK::CGCMsg< MsgGCCustomizeItemTexture_t > msg( k_EMsgGCCustomizeItemTexture );
	msg.Body().m_unToolItemID = m_nToolItemID;
	msg.Body().m_unSubjectItemID = m_nSubjectItemID;
	msg.Body().m_unImageUGCHandle = m_hCloudID;

	GCSDK::CGCMsg<MsgGCStandardResponse_t> msgReply;
	if ( !BYldSendMessageAndGetReply( msg, 10, &msgReply, k_EMsgGCCustomizeItemTextureResponse ) )
	{
		Warning( "Customize texture tool failed: Did not get reply from GC\n" );
		return k_EResultTimeout;
	}

	// OK!
	InventoryManager()->ShowItemsPickedUp( true );
	return k_EResultOK;
};

