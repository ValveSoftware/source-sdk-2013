//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// Interface to the client system responsible for dealing with shadows
//
// Boy is this complicated. OK, lets talk about how this works at the moment
//
// The ClientShadowMgr contains all of the highest-level state for rendering
// shadows, and it controls the ShadowMgr in the engine which is the central
// clearing house for rendering shadows.
//
// There are two important types of objects with respect to shadows:
// the shadow receiver, and the shadow caster. How is the association made
// between casters + the receivers? Turns out it's done slightly differently 
// depending on whether the receiver is the world, or if it's an entity.
//
// In the case of the world, every time the engine's ProjectShadow() is called, 
// any previous receiver state stored (namely, which world surfaces are
// receiving shadows) are cleared. Then, when ProjectShadow is called, 
// the engine iterates over all nodes + leaves within the shadow volume and 
// marks front-facing surfaces in them as potentially being affected by the 
// shadow. Later on, if those surfaces are actually rendered, the surfaces
// are clipped by the shadow volume + rendered.
// 
// In the case of entities, there are slightly different methods depending
// on whether the receiver is a brush model or a studio model. However, there
// are a couple central things that occur with both.
//
// Every time a shadow caster is moved, the ClientLeafSystem's ProjectShadow
// method is called to tell it to remove the shadow from all leaves + all 
// renderables it's currently associated with. Then it marks each leaf in the
// shadow volume as being affected by that shadow, and it marks every renderable
// in that volume as being potentially affected by the shadow (the function
// AddShadowToRenderable is called for each renderable in leaves affected
// by the shadow volume).
//
// Every time a shadow receiver is moved, the ClientLeafSystem first calls 
// RemoveAllShadowsFromRenderable to have it clear out its state, and then
// the ClientLeafSystem calls AddShadowToRenderable() for all shadows in all
// leaves the renderable has moved into.
//
// Now comes the difference between brush models + studio models. In the case
// of brush models, when a shadow is added to the studio model, it's done in
// the exact same way as for the world. Surfaces on the brush model are marked
// as potentially being affected by the shadow, and if those surfaces are
// rendered, the surfaces are clipped to the shadow volume. When ProjectShadow()
// is called, turns out the same operation that removes the shadow that moved
// from the world surfaces also works to remove the shadow from brush surfaces.
//
// In the case of studio models, we need a separate operation to remove
// the shadow from all studio models
//===========================================================================//


#include "cbase.h"
#include "engine/ishadowmgr.h"
#include "model_types.h"
#include "bitmap/imageformat.h"
#include "materialsystem/imaterialproxy.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "materialsystem/itexture.h"
#include "bsptreedata.h"
#include "utlmultilist.h"
#include "collisionutils.h"
#include "iviewrender.h"
#include "ivrenderview.h"
#include "tier0/vprof.h"
#include "engine/ivmodelinfo.h"
#include "view_shared.h"
#include "engine/ivdebugoverlay.h"
#include "engine/IStaticPropMgr.h"
#include "datacache/imdlcache.h"
#include "viewrender.h"
#include "tier0/icommandline.h"
#include "vstdlib/jobthread.h"
#include "toolframework_client.h"
#include "bonetoworldarray.h"
#include "cmodel.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar r_flashlightdrawfrustum( "r_flashlightdrawfrustum", "0" );
static ConVar r_flashlightmodels( "r_flashlightmodels", "1" );
static ConVar r_shadowrendertotexture( "r_shadowrendertotexture", "0" );
static ConVar r_flashlight_version2( "r_flashlight_version2", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

ConVar r_flashlightdepthtexture( "r_flashlightdepthtexture", "1" );

#if defined( _X360 )
ConVar r_flashlightdepthres( "r_flashlightdepthres", "512" );
#else
ConVar r_flashlightdepthres( "r_flashlightdepthres", "1024" );
#endif

ConVar r_threaded_client_shadow_manager( "r_threaded_client_shadow_manager", "0" );

#ifdef _WIN32
#pragma warning( disable: 4701 )
#endif

// forward declarations
void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );


//-----------------------------------------------------------------------------
// A texture allocator used to batch textures together
// At the moment, the implementation simply allocates blocks of max 256x256
// and each block stores an array of uniformly-sized textures
//-----------------------------------------------------------------------------
typedef unsigned short TextureHandle_t;
enum
{
	INVALID_TEXTURE_HANDLE = (TextureHandle_t)~0
};

class CTextureAllocator
{
public:
	// Initialize the allocator with something that knows how to refresh the bits
	void			Init();
	void			Shutdown();

	// Resets the allocator
	void			Reset();

	// Deallocates everything
	void			DeallocateAllTextures();

	// Allocate, deallocate texture
	TextureHandle_t	AllocateTexture( int w, int h );
	void			DeallocateTexture( TextureHandle_t h );

	// Mark texture as being used... (return true if re-render is needed)
	bool			UseTexture( TextureHandle_t h, bool bWillRedraw, float flArea );
	bool			HasValidTexture( TextureHandle_t h );

	// Advance frame...
	void			AdvanceFrame();

	// Get at the location of the texture
	void			GetTextureRect(TextureHandle_t handle, int& x, int& y, int& w, int& h );

	// Get at the texture it's a part of
	ITexture		*GetTexture();
	
	// Get at the total texture size.
	void			GetTotalTextureSize( int& w, int& h );

	void			DebugPrintCache( void );

private:
	typedef unsigned short FragmentHandle_t;

	enum
	{
		INVALID_FRAGMENT_HANDLE = (FragmentHandle_t)~0,
		TEXTURE_PAGE_SIZE	    = 1024,
		MAX_TEXTURE_POWER    	= 8,
#if !defined( _X360 )
		MIN_TEXTURE_POWER	    = 4,
#else
		MIN_TEXTURE_POWER	    = 5,	// per resolve requirements to ensure 32x32 aligned offsets
#endif
		MAX_TEXTURE_SIZE	    = (1 << MAX_TEXTURE_POWER),
		MIN_TEXTURE_SIZE	    = (1 << MIN_TEXTURE_POWER),
		BLOCK_SIZE			    = MAX_TEXTURE_SIZE,
		BLOCKS_PER_ROW		    = (TEXTURE_PAGE_SIZE / MAX_TEXTURE_SIZE),
		BLOCK_COUNT			    = (BLOCKS_PER_ROW * BLOCKS_PER_ROW),
	};

	struct TextureInfo_t
	{
		FragmentHandle_t	m_Fragment;
		unsigned short		m_Size;
		unsigned short		m_Power;
	};

	struct FragmentInfo_t
	{
		unsigned short	m_Block;
		unsigned short	m_Index;
		TextureHandle_t	m_Texture;

		// Makes sure we don't overflow
		unsigned int	m_FrameUsed;
	};

	struct BlockInfo_t
	{
		unsigned short	m_FragmentPower;
	};

	struct Cache_t
	{
		unsigned short	m_List;
	};

	// Adds a block worth of fragments to the LRU
	void AddBlockToLRU( int block );

	// Unlink fragment from cache
	void UnlinkFragmentFromCache( Cache_t& cache, FragmentHandle_t fragment );

	// Mark something as being used (MRU)..
	void MarkUsed( FragmentHandle_t fragment );

	// Mark something as being unused (LRU)..
	void MarkUnused( FragmentHandle_t fragment );

	// Disconnect texture from fragment
	void DisconnectTextureFromFragment( FragmentHandle_t f );

	// Returns the size of a particular fragment
	int	GetFragmentPower( FragmentHandle_t f ) const;

	// Stores the actual texture we're writing into
	CTextureReference	m_TexturePage;

	CUtlLinkedList< TextureInfo_t, TextureHandle_t >	m_Textures;
	CUtlMultiList< FragmentInfo_t, FragmentHandle_t >	m_Fragments;

	Cache_t		m_Cache[MAX_TEXTURE_POWER+1]; 
	BlockInfo_t	m_Blocks[BLOCK_COUNT];
	unsigned int m_CurrentFrame;
};

//-----------------------------------------------------------------------------
// Allocate/deallocate the texture page
//-----------------------------------------------------------------------------
void CTextureAllocator::Init()
{
	for ( int i = 0; i <= MAX_TEXTURE_POWER; ++i )
	{
		m_Cache[i].m_List = m_Fragments.InvalidIndex();
	}

#if !defined( _X360 )
	// don't need depth buffer for shadows
	m_TexturePage.InitRenderTarget( TEXTURE_PAGE_SIZE, TEXTURE_PAGE_SIZE, RT_SIZE_NO_CHANGE, IMAGE_FORMAT_ARGB8888, MATERIAL_RT_DEPTH_NONE, false, "_rt_Shadows" );
#else
	// unfortunate explicit management required for this render target
	// 32bpp edram is only largest shadow fragment, but resolved to actual shadow atlas
	// because full-res 1024x1024 shadow buffer is too large for EDRAM
	m_TexturePage.InitRenderTargetTexture( TEXTURE_PAGE_SIZE, TEXTURE_PAGE_SIZE, RT_SIZE_NO_CHANGE, IMAGE_FORMAT_ARGB8888, MATERIAL_RT_DEPTH_NONE, false, "_rt_Shadows" );

	// edram footprint is only 256x256x4 = 256K
	m_TexturePage.InitRenderTargetSurface( MAX_TEXTURE_SIZE, MAX_TEXTURE_SIZE, IMAGE_FORMAT_ARGB8888, false );

	// due to texture/surface size mismatch, ensure texture page is entirely cleared translucent
	// otherwise border artifacts at edge of shadows due to pixel shader averaging of unwanted bits
	m_TexturePage->ClearTexture( 0, 0, 0, 0 );
#endif
}

void CTextureAllocator::Shutdown()
{
	m_TexturePage.Shutdown();
}


//-----------------------------------------------------------------------------
// Initialize the allocator with something that knows how to refresh the bits
//-----------------------------------------------------------------------------
void CTextureAllocator::Reset()
{
	DeallocateAllTextures();

	m_Textures.EnsureCapacity(256);
	m_Fragments.EnsureCapacity(256);

	// Set up the block sizes....
	// FIXME: Improve heuristic?!?
#if !defined( _X360 )
	m_Blocks[0].m_FragmentPower  = MAX_TEXTURE_POWER-4;	// 128 cells at ExE resolution
#else
	m_Blocks[0].m_FragmentPower  = MAX_TEXTURE_POWER-3;	// 64 cells at DxD resolution
#endif
	m_Blocks[1].m_FragmentPower  = MAX_TEXTURE_POWER-3;	// 64 cells at DxD resolution
	m_Blocks[2].m_FragmentPower  = MAX_TEXTURE_POWER-2;	// 32 cells at CxC resolution
	m_Blocks[3].m_FragmentPower  = MAX_TEXTURE_POWER-2;		 
	m_Blocks[4].m_FragmentPower  = MAX_TEXTURE_POWER-1;	// 24 cells at BxB resolution
	m_Blocks[5].m_FragmentPower  = MAX_TEXTURE_POWER-1;
	m_Blocks[6].m_FragmentPower  = MAX_TEXTURE_POWER-1;
	m_Blocks[7].m_FragmentPower  = MAX_TEXTURE_POWER-1;
	m_Blocks[8].m_FragmentPower  = MAX_TEXTURE_POWER-1;
	m_Blocks[9].m_FragmentPower  = MAX_TEXTURE_POWER-1;
	m_Blocks[10].m_FragmentPower = MAX_TEXTURE_POWER;	// 6 cells at AxA resolution
	m_Blocks[11].m_FragmentPower = MAX_TEXTURE_POWER;	 
	m_Blocks[12].m_FragmentPower = MAX_TEXTURE_POWER;
	m_Blocks[13].m_FragmentPower = MAX_TEXTURE_POWER;
	m_Blocks[14].m_FragmentPower = MAX_TEXTURE_POWER;
	m_Blocks[15].m_FragmentPower = MAX_TEXTURE_POWER;

	// Initialize the LRU
	int i;
	for ( i = 0; i <= MAX_TEXTURE_POWER; ++i )
	{
		m_Cache[i].m_List = m_Fragments.CreateList();
	}

	// Now that the block sizes are allocated, create LRUs for the various block sizes
	for ( i = 0; i < BLOCK_COUNT; ++i)
	{
		// Initialize LRU
		AddBlockToLRU( i );
	}

	m_CurrentFrame = 0;
}

void CTextureAllocator::DeallocateAllTextures()
{
	m_Textures.Purge();
	m_Fragments.Purge();
	for ( int i = 0; i <= MAX_TEXTURE_POWER; ++i )
	{
		m_Cache[i].m_List = m_Fragments.InvalidIndex();
	}
}


//-----------------------------------------------------------------------------
// Dump the state of the cache to debug out
//-----------------------------------------------------------------------------
void CTextureAllocator::DebugPrintCache( void )
{
	// For each fragment
	int nNumFragments = m_Fragments.TotalCount();
	int nNumInvalidFragments = 0;

	Warning("Fragments (%d):\n===============\n", nNumFragments);

	for ( int f = 0; f < nNumFragments; f++ )
	{
		if ( ( m_Fragments[f].m_FrameUsed != 0 ) && ( m_Fragments[f].m_Texture != INVALID_TEXTURE_HANDLE ) )
			Warning("Fragment %d, Block: %d, Index: %d, Texture: %d Frame Used: %d\n", f, m_Fragments[f].m_Block, m_Fragments[f].m_Index, m_Fragments[f].m_Texture, m_Fragments[f].m_FrameUsed );
		else
			nNumInvalidFragments++;
	}

	Warning("Invalid Fragments: %d\n", nNumInvalidFragments);

//	for ( int c = 0; c <= MAX_TEXTURE_POWER; ++c )
//	{
//		Warning("Cache Index (%d)\n", m_Cache[c].m_List);
//	}

}


//-----------------------------------------------------------------------------
// Adds a block worth of fragments to the LRU
//-----------------------------------------------------------------------------
void CTextureAllocator::AddBlockToLRU( int block )
{
	int power = m_Blocks[block].m_FragmentPower;
 	int size = (1 << power);

	// Compute the number of fragments in this block
	int fragmentCount = MAX_TEXTURE_SIZE / size;
	fragmentCount *= fragmentCount;

	// For each fragment, indicate which block it's a part of (and the index)
	// and then stick in at the top of the LRU
	while (--fragmentCount >= 0 )
	{
		FragmentHandle_t f = m_Fragments.Alloc( );
		m_Fragments[f].m_Block = block;
		m_Fragments[f].m_Index = fragmentCount;
		m_Fragments[f].m_Texture = INVALID_TEXTURE_HANDLE;
		m_Fragments[f].m_FrameUsed = 0xFFFFFFFF;
		m_Fragments.LinkToHead( m_Cache[power].m_List, f );
	}
}


//-----------------------------------------------------------------------------
// Unlink fragment from cache
//-----------------------------------------------------------------------------
void CTextureAllocator::UnlinkFragmentFromCache( Cache_t& cache, FragmentHandle_t fragment )
{
	m_Fragments.Unlink( cache.m_List, fragment);
}


//-----------------------------------------------------------------------------
// Mark something as being used (MRU)..
//-----------------------------------------------------------------------------
void CTextureAllocator::MarkUsed( FragmentHandle_t fragment )
{
	int block = m_Fragments[fragment].m_Block;
	int power = m_Blocks[block].m_FragmentPower;

	// Hook it at the end of the LRU
	Cache_t& cache = m_Cache[power];
	m_Fragments.LinkToTail( cache.m_List, fragment );
	m_Fragments[fragment].m_FrameUsed = m_CurrentFrame;
}


//-----------------------------------------------------------------------------
// Mark something as being unused (LRU)..
//-----------------------------------------------------------------------------
void CTextureAllocator::MarkUnused( FragmentHandle_t fragment )
{
	int block = m_Fragments[fragment].m_Block;
	int power = m_Blocks[block].m_FragmentPower;

	// Hook it at the end of the LRU
	Cache_t& cache = m_Cache[power];
	m_Fragments.LinkToHead( cache.m_List, fragment );
}


//-----------------------------------------------------------------------------
// Allocate, deallocate texture
//-----------------------------------------------------------------------------
TextureHandle_t	CTextureAllocator::AllocateTexture( int w, int h )
{
	// Implementational detail for now
	Assert( w == h );

	// Clamp texture size
	if (w < MIN_TEXTURE_SIZE)
		w = MIN_TEXTURE_SIZE;
	else if (w > MAX_TEXTURE_SIZE)
		w = MAX_TEXTURE_SIZE;

	TextureHandle_t handle = m_Textures.AddToTail();
	m_Textures[handle].m_Fragment = INVALID_FRAGMENT_HANDLE;
	m_Textures[handle].m_Size = w;

	// Find the power of two
	int power = 0;
	int size = 1;
	while(size < w)
	{
		size <<= 1;
		++power;
	}
	Assert( size == w );

	m_Textures[handle].m_Power = power;

	return handle;
}

void CTextureAllocator::DeallocateTexture( TextureHandle_t h )
{
//	Warning("Beginning of DeallocateTexture\n");
//	DebugPrintCache();

	if (m_Textures[h].m_Fragment != INVALID_FRAGMENT_HANDLE)
	{
		MarkUnused(m_Textures[h].m_Fragment);
		m_Fragments[m_Textures[h].m_Fragment].m_FrameUsed = 0xFFFFFFFF;	// non-zero frame
		DisconnectTextureFromFragment( m_Textures[h].m_Fragment );
	}
	m_Textures.Remove(h);

//	Warning("End of DeallocateTexture\n");
//	DebugPrintCache();
}


//-----------------------------------------------------------------------------
// Disconnect texture from fragment
//-----------------------------------------------------------------------------
void CTextureAllocator::DisconnectTextureFromFragment( FragmentHandle_t f )
{
//	Warning( "Beginning of DisconnectTextureFromFragment\n" );
//	DebugPrintCache();

	FragmentInfo_t& info = m_Fragments[f];
	if (info.m_Texture != INVALID_TEXTURE_HANDLE)
	{
		m_Textures[info.m_Texture].m_Fragment = INVALID_FRAGMENT_HANDLE;
		info.m_Texture = INVALID_TEXTURE_HANDLE;
	}


//	Warning( "End of DisconnectTextureFromFragment\n" );
//	DebugPrintCache();
}


//-----------------------------------------------------------------------------
// Do we have a valid texture assigned?
//-----------------------------------------------------------------------------
bool CTextureAllocator::HasValidTexture( TextureHandle_t h )
{
	TextureInfo_t& info = m_Textures[h];
	FragmentHandle_t currentFragment = info.m_Fragment;
	return (currentFragment != INVALID_FRAGMENT_HANDLE);
}


//-----------------------------------------------------------------------------
// Mark texture as being used...
//-----------------------------------------------------------------------------
bool CTextureAllocator::UseTexture( TextureHandle_t h, bool bWillRedraw, float flArea )
{
//	Warning( "Top of UseTexture\n" );
//	DebugPrintCache();

	TextureInfo_t& info = m_Textures[h];

	// spin up to the best fragment size
	int nDesiredPower = MIN_TEXTURE_POWER;
	int nDesiredWidth = MIN_TEXTURE_SIZE;
	while ( (nDesiredWidth * nDesiredWidth) < flArea )
	{
		if ( nDesiredPower >= info.m_Power )
		{
			nDesiredPower = info.m_Power;
			break;
		}

		++nDesiredPower;
		nDesiredWidth <<= 1;
	}

	// If we've got a valid fragment for this texture, no worries!
	int nCurrentPower = -1;
	FragmentHandle_t currentFragment = info.m_Fragment;
	if (currentFragment != INVALID_FRAGMENT_HANDLE)
	{
		// If the current fragment is at or near the desired power, we're done
		nCurrentPower = GetFragmentPower(info.m_Fragment);
		Assert( nCurrentPower <= info.m_Power );
		bool bShouldKeepTexture = (!bWillRedraw) && (nDesiredPower < 8) && (nDesiredPower - nCurrentPower <= 1);
		if ((nCurrentPower == nDesiredPower) || bShouldKeepTexture)
		{
			// Move to the back of the LRU
			MarkUsed( currentFragment );
			return false;
		}
	}

//	Warning( "\n\nUseTexture B\n" );
//	DebugPrintCache();

	// Grab the LRU fragment from the appropriate cache
	// If that fragment is connected to a texture, disconnect it.
	int power = nDesiredPower;

	FragmentHandle_t f = INVALID_FRAGMENT_HANDLE;
	bool done = false;
	while (!done && power >= 0)
	{
		f = m_Fragments.Head( m_Cache[power].m_List );
	
		// This represents an overflow condition (used too many textures of
		// the same size in a single frame). It that happens, just use a texture
		// of lower res.
		if ( (f != m_Fragments.InvalidIndex()) && (m_Fragments[f].m_FrameUsed != m_CurrentFrame) )
		{
			done = true;
		}
		else
		{
			--power;
		}
	}


//	Warning( "\n\nUseTexture C\n" );
//	DebugPrintCache();

	// Ok, lets see if we're better off than we were...
	if (currentFragment != INVALID_FRAGMENT_HANDLE)
	{
		if (power <= nCurrentPower)
		{
			// Oops... we're not. Let's leave well enough alone
			// Move to the back of the LRU
			MarkUsed( currentFragment );
			return false;
		}
		else
		{
			// Clear out the old fragment
			DisconnectTextureFromFragment(currentFragment);
		}
	}

	if ( f == INVALID_FRAGMENT_HANDLE )
	{
		return false;
	}

	// Disconnect existing texture from this fragment (if necessary)
	DisconnectTextureFromFragment(f);

	// Connnect new texture to this fragment
	info.m_Fragment = f;
	m_Fragments[f].m_Texture = h;

	// Move to the back of the LRU
	MarkUsed( f );

	// Indicate we need a redraw
	return true;
}


//-----------------------------------------------------------------------------
// Returns the size of a particular fragment
//-----------------------------------------------------------------------------
int	CTextureAllocator::GetFragmentPower( FragmentHandle_t f ) const
{
	return m_Blocks[m_Fragments[f].m_Block].m_FragmentPower;
}


//-----------------------------------------------------------------------------
// Advance frame...
//-----------------------------------------------------------------------------
void CTextureAllocator::AdvanceFrame()
{
	// Be sure that this is called as infrequently as possible (i.e. once per frame,
	// NOT once per view) to prevent cache thrash when rendering multiple views in a single frame
	m_CurrentFrame++;
}


//-----------------------------------------------------------------------------
// Prepare to render into texture...
//-----------------------------------------------------------------------------
ITexture* CTextureAllocator::GetTexture()
{
	return m_TexturePage;
}

//-----------------------------------------------------------------------------
// Get at the total texture size.
//-----------------------------------------------------------------------------
void CTextureAllocator::GetTotalTextureSize( int& w, int& h )
{
	w = h = TEXTURE_PAGE_SIZE;
}


//-----------------------------------------------------------------------------
// Returns the rectangle the texture lives in..
//-----------------------------------------------------------------------------
void CTextureAllocator::GetTextureRect(TextureHandle_t handle, int& x, int& y, int& w, int& h )
{
	TextureInfo_t& info = m_Textures[handle];
	Assert( info.m_Fragment != INVALID_FRAGMENT_HANDLE );

	// Compute the position of the fragment in the page
	FragmentInfo_t& fragment = m_Fragments[info.m_Fragment];
	int blockY = fragment.m_Block / BLOCKS_PER_ROW;
	int blockX = fragment.m_Block - blockY * BLOCKS_PER_ROW;

	int fragmentSize = (1 << m_Blocks[fragment.m_Block].m_FragmentPower);
	int fragmentsPerRow = BLOCK_SIZE / fragmentSize;
	int fragmentY = fragment.m_Index / fragmentsPerRow;
	int fragmentX = fragment.m_Index - fragmentY * fragmentsPerRow;

	x = blockX * BLOCK_SIZE + fragmentX * fragmentSize;
	y = blockY * BLOCK_SIZE + fragmentY * fragmentSize;
	w = fragmentSize;
	h = fragmentSize;
}


//-----------------------------------------------------------------------------
// Defines how big of a shadow texture we should be making per caster...
//-----------------------------------------------------------------------------
#define TEXEL_SIZE_PER_CASTER_SIZE	2.0f 
#define MAX_FALLOFF_AMOUNT 240
#define MAX_CLIP_PLANE_COUNT 4
#define SHADOW_CULL_TOLERANCE 0.5f

static ConVar r_shadows( "r_shadows", "1" ); // hook into engine's cvars..
static ConVar r_shadowmaxrendered("r_shadowmaxrendered", "32");
static ConVar r_shadows_gamecontrol( "r_shadows_gamecontrol", "-1", FCVAR_CHEAT );	 // hook into engine's cvars..

//-----------------------------------------------------------------------------
// The class responsible for dealing with shadows on the client side
// Oh, and let's take a moment and notice how happy Robin and John must be 
// owing to the lack of space between this lovely comment and the class name =)
//-----------------------------------------------------------------------------
class CClientShadowMgr : public IClientShadowMgr
{
public:
	CClientShadowMgr();

	virtual char const *Name() { return "CCLientShadowMgr"; }

	// Inherited from IClientShadowMgr
	virtual bool Init();
	virtual void PostInit() {}
	virtual void Shutdown();
	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity() {}
	virtual void LevelShutdownPreEntity() {}
	virtual void LevelShutdownPostEntity();

	virtual bool IsPerFrame() { return true; }

	virtual void PreRender();
	virtual void Update( float frametime ) { }
	virtual void PostRender() {}

	virtual void OnSave() {}
	virtual void OnRestore() {}
	virtual void SafeRemoveIfDesired() {}

	virtual ClientShadowHandle_t CreateShadow( ClientEntityHandle_t entity, int flags );
	virtual void DestroyShadow( ClientShadowHandle_t handle );

	// Create flashlight (projected texture light source)
	virtual ClientShadowHandle_t CreateFlashlight( const FlashlightState_t &lightState );
	virtual void UpdateFlashlightState( ClientShadowHandle_t shadowHandle, const FlashlightState_t &lightState );
	virtual void DestroyFlashlight( ClientShadowHandle_t shadowHandle );

	// Update a shadow
	virtual void UpdateProjectedTexture( ClientShadowHandle_t handle, bool force );

	void ComputeBoundingSphere( IClientRenderable* pRenderable, Vector& origin, float& radius );

	virtual void AddToDirtyShadowList( ClientShadowHandle_t handle, bool bForce );
	virtual void AddToDirtyShadowList( IClientRenderable *pRenderable, bool force );

	// Marks the render-to-texture shadow as needing to be re-rendered
	virtual void MarkRenderToTextureShadowDirty( ClientShadowHandle_t handle );

	// deals with shadows being added to shadow receivers
	void AddShadowToReceiver( ClientShadowHandle_t handle,
		IClientRenderable* pRenderable, ShadowReceiver_t type );

	// deals with shadows being added to shadow receivers
	void RemoveAllShadowsFromReceiver( IClientRenderable* pRenderable, ShadowReceiver_t type );

	// Re-renders all shadow textures for shadow casters that lie in the leaf list
	void ComputeShadowTextures( const CViewSetup &view, int leafCount, LeafIndex_t* pLeafList );

	// Kicks off rendering into shadow depth maps (if any)
	void ComputeShadowDepthTextures( const CViewSetup &view );

	// Frees shadow depth textures for use in subsequent view/frame
	void FreeShadowDepthTextures();

	// Returns the shadow texture
	ITexture* GetShadowTexture( unsigned short h );

	// Returns shadow information
	const ShadowInfo_t& GetShadowInfo( ClientShadowHandle_t h );

	// Renders the shadow texture to screen...
	void RenderShadowTexture( int w, int h );

	// Sets the shadow direction
	virtual void SetShadowDirection( const Vector& dir );
	const Vector &GetShadowDirection() const;

	// Sets the shadow color
	virtual void SetShadowColor( unsigned char r, unsigned char g, unsigned char b );
	void GetShadowColor( unsigned char *r, unsigned char *g, unsigned char *b ) const;

	// Sets the shadow distance
	virtual void SetShadowDistance( float flMaxDistance );
	float GetShadowDistance( ) const;

	// Sets the screen area at which blobby shadows are always used
	virtual void SetShadowBlobbyCutoffArea( float flMinArea );
	float GetBlobbyCutoffArea( ) const;

	// Set the darkness falloff bias
	virtual void SetFalloffBias( ClientShadowHandle_t handle, unsigned char ucBias );

	void RestoreRenderState();

	// Computes a rough bounding box encompassing the volume of the shadow
	void ComputeShadowBBox( IClientRenderable *pRenderable, const Vector &vecAbsCenter, float flRadius, Vector *pAbsMins, Vector *pAbsMaxs );

	bool WillParentRenderBlobbyShadow( IClientRenderable *pRenderable );

	// Are we the child of a shadow with render-to-texture?
	bool ShouldUseParentShadow( IClientRenderable *pRenderable );

	void SetShadowsDisabled( bool bDisabled ) 
	{ 
		r_shadows_gamecontrol.SetValue( bDisabled != 1 );
	}

private:
	enum
	{
		SHADOW_FLAGS_TEXTURE_DIRTY =	(CLIENT_SHADOW_FLAGS_LAST_FLAG << 1),
		SHADOW_FLAGS_BRUSH_MODEL =		(CLIENT_SHADOW_FLAGS_LAST_FLAG << 2), 
		SHADOW_FLAGS_USING_LOD_SHADOW = (CLIENT_SHADOW_FLAGS_LAST_FLAG << 3),
		SHADOW_FLAGS_LIGHT_WORLD =		(CLIENT_SHADOW_FLAGS_LAST_FLAG << 4),
	};

	struct ClientShadow_t
	{
		ClientEntityHandle_t	m_Entity;
		ShadowHandle_t			m_ShadowHandle;
		ClientLeafShadowHandle_t m_ClientLeafShadowHandle;
		unsigned short			m_Flags;
		VMatrix					m_WorldToShadow;
		Vector2D				m_WorldSize;
		Vector					m_LastOrigin;
		QAngle					m_LastAngles;
		TextureHandle_t			m_ShadowTexture;
		CTextureReference		m_ShadowDepthTexture;
		int						m_nRenderFrame;
		EHANDLE					m_hTargetEntity;
	};

private:
	// Shadow update functions
	void UpdateStudioShadow( IClientRenderable *pRenderable, ClientShadowHandle_t handle );
	void UpdateBrushShadow( IClientRenderable *pRenderable, ClientShadowHandle_t handle );
	void UpdateShadow( ClientShadowHandle_t handle, bool force );

	// Gets the entity whose shadow this shadow will render into
	IClientRenderable *GetParentShadowEntity( ClientShadowHandle_t handle );

	// Adds the child bounds to the bounding box
	void AddChildBounds( matrix3x4_t &matWorldToBBox, IClientRenderable* pParent, Vector &vecMins, Vector &vecMaxs );

	// Compute a bounds for the entity + children
	void ComputeHierarchicalBounds( IClientRenderable *pRenderable, Vector &vecMins, Vector &vecMaxs );

	// Builds matrices transforming from world space to shadow space
	void BuildGeneralWorldToShadowMatrix( VMatrix& matWorldToShadow,
		const Vector& origin, const Vector& dir, const Vector& xvec, const Vector& yvec );

	void BuildWorldToShadowMatrix( VMatrix& matWorldToShadow, const Vector& origin, const Quaternion& quatOrientation );

	void BuildPerspectiveWorldToFlashlightMatrix( VMatrix& matWorldToShadow, const FlashlightState_t &flashlightState );

	// Update a shadow
	void UpdateProjectedTextureInternal( ClientShadowHandle_t handle, bool force );

	// Compute the shadow origin and attenuation start distance
	float ComputeLocalShadowOrigin( IClientRenderable* pRenderable, 
		const Vector& mins, const Vector& maxs, const Vector& localShadowDir, float backupFactor, Vector& origin );

	// Remove a shadow from the dirty list
	void RemoveShadowFromDirtyList( ClientShadowHandle_t handle );

	// NOTE: this will ONLY return SHADOWS_NONE, SHADOWS_SIMPLE, or SHADOW_RENDER_TO_TEXTURE.
	ShadowType_t GetActualShadowCastType( ClientShadowHandle_t handle ) const;
	ShadowType_t GetActualShadowCastType( IClientRenderable *pRenderable ) const;

	// Builds a simple blobby shadow
	void BuildOrthoShadow( IClientRenderable* pRenderable, ClientShadowHandle_t handle, const Vector& mins, const Vector& maxs);

	// Builds a more complex shadow...
	void BuildRenderToTextureShadow( IClientRenderable* pRenderable, 
			ClientShadowHandle_t handle, const Vector& mins, const Vector& maxs );

	// Build a projected-texture flashlight
	void BuildFlashlight( ClientShadowHandle_t handle );

	// Does all the lovely stuff we need to do to have render-to-texture shadows
	void SetupRenderToTextureShadow( ClientShadowHandle_t h );
	void CleanUpRenderToTextureShadow( ClientShadowHandle_t h );

	// Compute the extra shadow planes
	void ComputeExtraClipPlanes( IClientRenderable* pRenderable, 
		ClientShadowHandle_t handle, const Vector* vec, 
		const Vector& mins, const Vector& maxs, const Vector& localShadowDir );

	// Set extra clip planes related to shadows...
	void ClearExtraClipPlanes( ClientShadowHandle_t h );
	void AddExtraClipPlane( ClientShadowHandle_t h, const Vector& normal, float dist );

	// Cull if the origin is on the wrong side of a shadow clip plane....
	bool CullReceiver( ClientShadowHandle_t handle, IClientRenderable* pRenderable, IClientRenderable* pSourceRenderable );

	bool ComputeSeparatingPlane( IClientRenderable* pRend1, IClientRenderable* pRend2, cplane_t* pPlane );

	// Causes all shadows to be re-updated
	void UpdateAllShadows();

	// One of these gets called with every shadow that potentially will need to re-render
	bool DrawRenderToTextureShadow( unsigned short clientShadowHandle, float flArea );
	void DrawRenderToTextureShadowLOD( unsigned short clientShadowHandle );

	// Draws all children shadows into our own
	bool DrawShadowHierarchy( IClientRenderable *pRenderable, const ClientShadow_t &shadow, bool bChild = false );

	// Setup stage for threading
	bool BuildSetupListForRenderToTextureShadow( unsigned short clientShadowHandle, float flArea );
	bool BuildSetupShadowHierarchy( IClientRenderable *pRenderable, const ClientShadow_t &shadow, bool bChild = false );

	// Computes + sets the render-to-texture texcoords
	void SetRenderToTextureShadowTexCoords( ShadowHandle_t handle, int x, int y, int w, int h );

	// Visualization....
	void DrawRenderToTextureDebugInfo( IClientRenderable* pRenderable, const Vector& mins, const Vector& maxs );

	// Advance frame
	void AdvanceFrame();

	// Returns renderable-specific shadow info
	float GetShadowDistance( IClientRenderable *pRenderable ) const;
	const Vector &GetShadowDirection( IClientRenderable *pRenderable ) const;

	// Initialize, shutdown render-to-texture shadows
	void InitDepthTextureShadows();
	void ShutdownDepthTextureShadows();

	// Initialize, shutdown render-to-texture shadows
	void InitRenderToTextureShadows();
	void ShutdownRenderToTextureShadows();

	static bool ShadowHandleCompareFunc( const ClientShadowHandle_t& lhs, const ClientShadowHandle_t& rhs )
	{
		return lhs < rhs;
	}

	ClientShadowHandle_t CreateProjectedTexture( ClientEntityHandle_t entity, int flags );

	// Lock down the usage of a shadow depth texture...must be unlocked use on subsequent views / frames
	bool	LockShadowDepthTexture( CTextureReference *shadowDepthTexture );
	void	UnlockAllShadowDepthTextures();

	// Set and clear flashlight target renderable
	void	SetFlashlightTarget( ClientShadowHandle_t shadowHandle, EHANDLE targetEntity );

	// Set flashlight light world flag
	void	SetFlashlightLightWorld( ClientShadowHandle_t shadowHandle, bool bLightWorld );

	bool	IsFlashlightTarget( ClientShadowHandle_t shadowHandle, IClientRenderable *pRenderable );

	// Builds a list of active shadows requiring shadow depth renders
	int		BuildActiveShadowDepthList( const CViewSetup &viewSetup, int nMaxDepthShadows, ClientShadowHandle_t *pActiveDepthShadows );

	// Sets the view's active flashlight render state
	void	SetViewFlashlightState( int nActiveFlashlightCount, ClientShadowHandle_t* pActiveFlashlights );

private:
	Vector	m_SimpleShadowDir;
	color32	m_AmbientLightColor;
	CMaterialReference m_SimpleShadow;
	CMaterialReference m_RenderShadow;
	CMaterialReference m_RenderModelShadow;
	CTextureReference m_DummyColorTexture;
	CUtlLinkedList< ClientShadow_t, ClientShadowHandle_t >	m_Shadows;
	CTextureAllocator m_ShadowAllocator;

	bool m_RenderToTextureActive;
	bool m_bRenderTargetNeedsClear;
	bool m_bUpdatingDirtyShadows;
	bool m_bThreaded;
	float m_flShadowCastDist;
	float m_flMinShadowArea;
	CUtlRBTree< ClientShadowHandle_t, unsigned short >	m_DirtyShadows;
	CUtlVector< ClientShadowHandle_t > m_TransparentShadows;

	// These members maintain current state of depth texturing (size and global active state)
	// If either changes in a frame, PreRender() will catch it and do the appropriate allocation, deallocation or reallocation
	bool m_bDepthTextureActive;
	int m_nDepthTextureResolution; // Assume square (height == width)

	CUtlVector< CTextureReference > m_DepthTextureCache;
	CUtlVector< bool > m_DepthTextureCacheLocks;
	int	m_nMaxDepthTextureShadows;

	friend class CVisibleShadowList;
	friend class CVisibleShadowFrustumList;
};

//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
static CClientShadowMgr s_ClientShadowMgr;
IClientShadowMgr* g_pClientShadowMgr = &s_ClientShadowMgr;


//-----------------------------------------------------------------------------
// Builds a list of potential shadows that lie within our PVS + view frustum
//-----------------------------------------------------------------------------
struct VisibleShadowInfo_t
{
	ClientShadowHandle_t	m_hShadow;
	float					m_flArea;
	Vector					m_vecAbsCenter;
};

class CVisibleShadowList : public IClientLeafShadowEnum
{
public:

	CVisibleShadowList();
	int FindShadows( const CViewSetup *pView, int nLeafCount, LeafIndex_t *pLeafList );
	int GetVisibleShadowCount() const;

	const VisibleShadowInfo_t &GetVisibleShadow( int i ) const;

private:
	void EnumShadow( unsigned short clientShadowHandle );
	float ComputeScreenArea( const Vector &vecCenter, float r ) const;
	void PrioritySort();

	CUtlVector<VisibleShadowInfo_t> m_ShadowsInView;
	CUtlVector<int>	m_PriorityIndex;
};


//-----------------------------------------------------------------------------
// Singleton instances of shadow and shadow frustum lists
//-----------------------------------------------------------------------------
static CVisibleShadowList			s_VisibleShadowList;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static CUtlVector<C_BaseAnimating *> s_NPCShadowBoneSetups;
static CUtlVector<C_BaseAnimating *> s_NonNPCShadowBoneSetups;

//-----------------------------------------------------------------------------
// CVisibleShadowList - Constructor and Accessors
//-----------------------------------------------------------------------------
CVisibleShadowList::CVisibleShadowList() : m_ShadowsInView( 0, 64 ), m_PriorityIndex( 0, 64 ) 
{
}

int CVisibleShadowList::GetVisibleShadowCount() const
{
	return m_ShadowsInView.Count();
}

const VisibleShadowInfo_t &CVisibleShadowList::GetVisibleShadow( int i ) const
{
	return m_ShadowsInView[m_PriorityIndex[i]];
}


//-----------------------------------------------------------------------------
// CVisibleShadowList - Computes approximate screen area of the shadow
//-----------------------------------------------------------------------------
float CVisibleShadowList::ComputeScreenArea( const Vector &vecCenter, float r ) const
{
	CMatRenderContextPtr pRenderContext( materials );
	float flScreenDiameter = pRenderContext->ComputePixelDiameterOfSphere( vecCenter, r );
	return flScreenDiameter * flScreenDiameter;
}


//-----------------------------------------------------------------------------
// CVisibleShadowList - Visits every shadow in the list of leaves
//-----------------------------------------------------------------------------
void CVisibleShadowList::EnumShadow( unsigned short clientShadowHandle )
{
	CClientShadowMgr::ClientShadow_t& shadow = s_ClientShadowMgr.m_Shadows[clientShadowHandle];

	// Don't bother if we rendered it this frame, no matter which view it was rendered for
	if ( shadow.m_nRenderFrame == gpGlobals->framecount )
		return;

	// We don't need to bother with it if it's not render-to-texture
	if ( s_ClientShadowMgr.GetActualShadowCastType( clientShadowHandle ) != SHADOWS_RENDER_TO_TEXTURE )
		return;

	// Don't bother with it if the shadow is totally transparent
	const ShadowInfo_t &shadowInfo = shadowmgr->GetInfo( shadow.m_ShadowHandle );
	if ( shadowInfo.m_FalloffBias == 255 )
		return;

	IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( shadow.m_Entity );
	Assert( pRenderable );

	// Don't bother with children of hierarchy; they will be drawn with their parents
	if ( s_ClientShadowMgr.ShouldUseParentShadow( pRenderable ) || s_ClientShadowMgr.WillParentRenderBlobbyShadow( pRenderable ) )
		return;

	// Compute a sphere surrounding the shadow
	// FIXME: This doesn't account for children of hierarchy... too bad!
	Vector vecAbsCenter;
	float flRadius;
	s_ClientShadowMgr.ComputeBoundingSphere( pRenderable, vecAbsCenter, flRadius );

	// Compute a box surrounding the shadow
	Vector vecAbsMins, vecAbsMaxs;
	s_ClientShadowMgr.ComputeShadowBBox( pRenderable, vecAbsCenter, flRadius, &vecAbsMins, &vecAbsMaxs );

	// FIXME: Add distance check here?

	// Make sure it's in the frustum. If it isn't it's not interesting
	if (engine->CullBox( vecAbsMins, vecAbsMaxs ))
		return;

	int i = m_ShadowsInView.AddToTail( );
	VisibleShadowInfo_t &info = m_ShadowsInView[i];
	info.m_hShadow = clientShadowHandle;
	m_ShadowsInView[i].m_flArea = ComputeScreenArea( vecAbsCenter, flRadius );

	// Har, har. When water is rendering (or any multipass technique), 
	// we may well initially render from a viewpoint which doesn't include this shadow. 
	// That doesn't mean we shouldn't check it again though. Sucks that we need to compute
	// the sphere + bbox multiply times though.
	shadow.m_nRenderFrame = gpGlobals->framecount;
}


//-----------------------------------------------------------------------------
// CVisibleShadowList - Sort based on screen area/priority
//-----------------------------------------------------------------------------
void CVisibleShadowList::PrioritySort()
{
	int nCount = m_ShadowsInView.Count();
	m_PriorityIndex.EnsureCapacity( nCount );

	m_PriorityIndex.RemoveAll();

	int i, j;
	for ( i = 0; i < nCount; ++i )
	{
		m_PriorityIndex.AddToTail(i);
	}

	for ( i = 0; i < nCount - 1; ++i )
	{
		int nLargestInd = i;
		float flLargestArea = m_ShadowsInView[m_PriorityIndex[i]].m_flArea;
		for ( j = i + 1; j < nCount; ++j )
		{
			int nIndex = m_PriorityIndex[j];
			if ( flLargestArea < m_ShadowsInView[nIndex].m_flArea )
			{
				nLargestInd = j;
				flLargestArea = m_ShadowsInView[nIndex].m_flArea;
			}
		}
		::V_swap( m_PriorityIndex[i], m_PriorityIndex[nLargestInd] );
	}
}


//-----------------------------------------------------------------------------
// CVisibleShadowList - Main entry point for finding shadows in the leaf list
//-----------------------------------------------------------------------------
int CVisibleShadowList::FindShadows( const CViewSetup *pView, int nLeafCount, LeafIndex_t *pLeafList )
{
	VPROF_BUDGET( "CVisibleShadowList::FindShadows", VPROF_BUDGETGROUP_SHADOW_RENDERING );

	m_ShadowsInView.RemoveAll();
	ClientLeafSystem()->EnumerateShadowsInLeaves( nLeafCount, pLeafList, this );
	int nCount = m_ShadowsInView.Count();
	if (nCount != 0)
	{
		// Sort based on screen area/priority
		PrioritySort();
	}
	return nCount;
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CClientShadowMgr::CClientShadowMgr() :
	m_DirtyShadows( 0, 0, ShadowHandleCompareFunc ),
	m_RenderToTextureActive( false ),
	m_bDepthTextureActive( false )
{
	m_nDepthTextureResolution = r_flashlightdepthres.GetInt();
	m_bThreaded = false;
}


//-----------------------------------------------------------------------------
// Changes the shadow direction...
//-----------------------------------------------------------------------------
CON_COMMAND_F( r_shadowdir, "Set shadow direction", FCVAR_CHEAT )
{
	Vector dir;
	if ( args.ArgC() == 1 )
	{
		Vector dir = s_ClientShadowMgr.GetShadowDirection();
		Msg( "%.2f %.2f %.2f\n", dir.x, dir.y, dir.z );
		return;
	}

	if ( args.ArgC() == 4 )
	{
		dir.x = atof( args[1] );
		dir.y = atof( args[2] );
		dir.z = atof( args[3] );
		s_ClientShadowMgr.SetShadowDirection(dir);
	}
}

CON_COMMAND_F( r_shadowangles, "Set shadow angles", FCVAR_CHEAT )
{
	Vector dir;
	QAngle angles;
	if (args.ArgC() == 1)
	{
		Vector dir = s_ClientShadowMgr.GetShadowDirection();
		QAngle angles;
		VectorAngles( dir, angles );
		Msg( "%.2f %.2f %.2f\n", angles.x, angles.y, angles.z );
		return;
	}

	if (args.ArgC() == 4)
	{
		angles.x = atof( args[1] );
		angles.y = atof( args[2] );
		angles.z = atof( args[3] );
		AngleVectors( angles, &dir );
		s_ClientShadowMgr.SetShadowDirection(dir);
	}
}

CON_COMMAND_F( r_shadowcolor, "Set shadow color", FCVAR_CHEAT )
{
	if (args.ArgC() == 1)
	{
		unsigned char r, g, b;
		s_ClientShadowMgr.GetShadowColor( &r, &g, &b );
		Msg( "Shadow color %d %d %d\n", r, g, b );
		return;
	}

	if (args.ArgC() == 4)
	{
		int r = atoi( args[1] );
		int g = atoi( args[2] );
		int b = atoi( args[3] );
		s_ClientShadowMgr.SetShadowColor(r, g, b);
	}
}

CON_COMMAND_F( r_shadowdist, "Set shadow distance", FCVAR_CHEAT )
{
	if (args.ArgC() == 1)
	{
		float flDist = s_ClientShadowMgr.GetShadowDistance( );
		Msg( "Shadow distance %.2f\n", flDist );
		return;
	}

	if (args.ArgC() == 2)
	{
		float flDistance = atof( args[1] );
		s_ClientShadowMgr.SetShadowDistance( flDistance );
	}
}

CON_COMMAND_F( r_shadowblobbycutoff, "some shadow stuff", FCVAR_CHEAT )
{
	if (args.ArgC() == 1)
	{
		float flArea = s_ClientShadowMgr.GetBlobbyCutoffArea( );
		Msg( "Cutoff area %.2f\n", flArea );
		return;
	}

	if (args.ArgC() == 2)
	{
		float flArea = atof( args[1] );
		s_ClientShadowMgr.SetShadowBlobbyCutoffArea( flArea );
	}
}

static void ShadowRestoreFunc( int nChangeFlags )
{
	s_ClientShadowMgr.RestoreRenderState();
}

//-----------------------------------------------------------------------------
// Initialization, shutdown
//-----------------------------------------------------------------------------
bool CClientShadowMgr::Init()
{
	m_bRenderTargetNeedsClear = false;
	m_SimpleShadow.Init( "decals/simpleshadow", TEXTURE_GROUP_DECAL );

	Vector dir( 0.1, 0.1, -1 );
	SetShadowDirection(dir);
	SetShadowDistance( 50 );

	SetShadowBlobbyCutoffArea( 0.005 );

	bool bTools = CommandLine()->CheckParm( "-tools" ) != NULL;
	m_nMaxDepthTextureShadows = bTools ? 4 : 1;	// Just one shadow depth texture in games, more in tools

	bool bLowEnd = ( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() < 80 );

	if ( !bLowEnd && r_shadowrendertotexture.GetBool() )
	{
		InitRenderToTextureShadows();
	}

	// If someone turned shadow depth mapping on but we can't do it, force it off
	if ( r_flashlightdepthtexture.GetBool() && !materials->SupportsShadowDepthTextures() )
	{
		r_flashlightdepthtexture.SetValue( 0 );
		ShutdownDepthTextureShadows();	
	}

	if ( !bLowEnd && r_flashlightdepthtexture.GetBool() )
	{
		InitDepthTextureShadows();
	}

	materials->AddRestoreFunc( ShadowRestoreFunc );

	return true;
}

void CClientShadowMgr::Shutdown()
{
	m_SimpleShadow.Shutdown();
	m_Shadows.RemoveAll();
	ShutdownRenderToTextureShadows();

	ShutdownDepthTextureShadows();

	materials->RemoveRestoreFunc( ShadowRestoreFunc );
}


//-----------------------------------------------------------------------------
// Initialize, shutdown depth-texture shadows
//-----------------------------------------------------------------------------
void CClientShadowMgr::InitDepthTextureShadows()
{
	VPROF_BUDGET( "CClientShadowMgr::InitDepthTextureShadows", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

	if( !m_bDepthTextureActive )
	{
		m_bDepthTextureActive = true;

		ImageFormat dstFormat  = materials->GetShadowDepthTextureFormat();	// Vendor-dependent depth texture format
#if !defined( _X360 )
		ImageFormat nullFormat = materials->GetNullTextureFormat();			// Vendor-dependent null texture format (takes as little memory as possible)
#endif
		materials->BeginRenderTargetAllocation();

#if defined( _X360 )
		// For the 360, we'll be rendering depth directly into the dummy depth and Resolve()ing to the depth texture.
		// only need the dummy surface, don't care about color results
		m_DummyColorTexture.InitRenderTargetTexture( r_flashlightdepthres.GetInt(), r_flashlightdepthres.GetInt(), RT_SIZE_OFFSCREEN, IMAGE_FORMAT_BGR565, MATERIAL_RT_DEPTH_SHARED, false, "_rt_ShadowDummy" );
		m_DummyColorTexture.InitRenderTargetSurface( r_flashlightdepthres.GetInt(), r_flashlightdepthres.GetInt(), IMAGE_FORMAT_BGR565, true );
#else
		m_DummyColorTexture.InitRenderTarget( r_flashlightdepthres.GetInt(), r_flashlightdepthres.GetInt(), RT_SIZE_OFFSCREEN, nullFormat, MATERIAL_RT_DEPTH_NONE, false, "_rt_ShadowDummy" );
#endif

		// Create some number of depth-stencil textures
		m_DepthTextureCache.Purge();
		m_DepthTextureCacheLocks.Purge();
		for( int i=0; i < m_nMaxDepthTextureShadows; i++ )
		{
			CTextureReference depthTex;	// Depth-stencil surface
			bool bFalse = false;

			char strRTName[64];
			Q_snprintf( strRTName, ARRAYSIZE( strRTName ), "_rt_ShadowDepthTexture_%d", i );

#if defined( _X360 )
			// create a render target to use as a resolve target to get the shared depth buffer
			// surface is effectively never used
			depthTex.InitRenderTargetTexture( m_nDepthTextureResolution, m_nDepthTextureResolution, RT_SIZE_OFFSCREEN, dstFormat, MATERIAL_RT_DEPTH_NONE, false, strRTName );
			depthTex.InitRenderTargetSurface( 1, 1, dstFormat, false );
#else
			depthTex.InitRenderTarget( m_nDepthTextureResolution, m_nDepthTextureResolution, RT_SIZE_OFFSCREEN, dstFormat, MATERIAL_RT_DEPTH_NONE, false, strRTName );
#endif

			if ( i == 0 )
			{
				// Shadow may be resized during allocation (due to resolution constraints etc)
				m_nDepthTextureResolution = depthTex->GetActualWidth();
				r_flashlightdepthres.SetValue( m_nDepthTextureResolution );
			}

			m_DepthTextureCache.AddToTail( depthTex );
			m_DepthTextureCacheLocks.AddToTail( bFalse );
		}

		materials->EndRenderTargetAllocation();
	}
}

void CClientShadowMgr::ShutdownDepthTextureShadows() 
{
	if( m_bDepthTextureActive )
	{
		// Shut down the dummy texture
		m_DummyColorTexture.Shutdown();

		while( m_DepthTextureCache.Count() )
		{
			m_DepthTextureCache[ m_DepthTextureCache.Count()-1 ].Shutdown();

			m_DepthTextureCacheLocks.Remove( m_DepthTextureCache.Count()-1 );
			m_DepthTextureCache.Remove( m_DepthTextureCache.Count()-1 );
		}

		m_bDepthTextureActive = false;
	}
}

//-----------------------------------------------------------------------------
// Initialize, shutdown render-to-texture shadows
//-----------------------------------------------------------------------------
void CClientShadowMgr::InitRenderToTextureShadows()
{
	if ( !m_RenderToTextureActive )
	{
		m_RenderToTextureActive = true;
		m_RenderShadow.Init( "decals/rendershadow", TEXTURE_GROUP_DECAL );
		m_RenderModelShadow.Init( "decals/rendermodelshadow", TEXTURE_GROUP_DECAL );
		m_ShadowAllocator.Init();

		m_ShadowAllocator.Reset();
		m_bRenderTargetNeedsClear = true;

		float fr = (float)m_AmbientLightColor.r / 255.0f;
		float fg = (float)m_AmbientLightColor.g / 255.0f;
		float fb = (float)m_AmbientLightColor.b / 255.0f;
		m_RenderShadow->ColorModulate( fr, fg, fb );
		m_RenderModelShadow->ColorModulate( fr, fg, fb );

		// Iterate over all existing textures and allocate shadow textures
		for (ClientShadowHandle_t i = m_Shadows.Head(); i != m_Shadows.InvalidIndex(); i = m_Shadows.Next(i) )
		{
			ClientShadow_t& shadow = m_Shadows[i];
			if ( shadow.m_Flags & SHADOW_FLAGS_USE_RENDER_TO_TEXTURE )
			{
				SetupRenderToTextureShadow( i );
				MarkRenderToTextureShadowDirty( i );

				// Switch the material to use render-to-texture shadows
				shadowmgr->SetShadowMaterial( shadow.m_ShadowHandle, m_RenderShadow, m_RenderModelShadow, (void*)(uintp)i );
			}
		}
	}
}

void CClientShadowMgr::ShutdownRenderToTextureShadows()
{
	if (m_RenderToTextureActive)
	{
		// Iterate over all existing textures and deallocate shadow textures
		for (ClientShadowHandle_t i = m_Shadows.Head(); i != m_Shadows.InvalidIndex(); i = m_Shadows.Next(i) )
		{
			CleanUpRenderToTextureShadow( i );

			// Switch the material to use blobby shadows
			ClientShadow_t& shadow = m_Shadows[i];
			shadowmgr->SetShadowMaterial( shadow.m_ShadowHandle, m_SimpleShadow, m_SimpleShadow, (void*)CLIENTSHADOW_INVALID_HANDLE );
			shadowmgr->SetShadowTexCoord( shadow.m_ShadowHandle, 0, 0, 1, 1 );
			ClearExtraClipPlanes( i );
		}

		m_RenderShadow.Shutdown();
		m_RenderModelShadow.Shutdown();

		m_ShadowAllocator.DeallocateAllTextures();
		m_ShadowAllocator.Shutdown();

		// Cause the render target to go away
		materials->UncacheUnusedMaterials();

		m_RenderToTextureActive = false;
	}
}


//-----------------------------------------------------------------------------
// Sets the shadow color
//-----------------------------------------------------------------------------
void CClientShadowMgr::SetShadowColor( unsigned char r, unsigned char g, unsigned char b )
{
	float fr = (float)r / 255.0f;
	float fg = (float)g / 255.0f;
	float fb = (float)b / 255.0f;

	// Hook the shadow color into the shadow materials
	m_SimpleShadow->ColorModulate( fr, fg, fb );

	if (m_RenderToTextureActive)
	{
		m_RenderShadow->ColorModulate( fr, fg, fb );
		m_RenderModelShadow->ColorModulate( fr, fg, fb );
	}

	m_AmbientLightColor.r = r;
	m_AmbientLightColor.g = g;
	m_AmbientLightColor.b = b;
}

void CClientShadowMgr::GetShadowColor( unsigned char *r, unsigned char *g, unsigned char *b ) const
{
	*r = m_AmbientLightColor.r;
	*g = m_AmbientLightColor.g;
	*b = m_AmbientLightColor.b;
}


//-----------------------------------------------------------------------------
// Level init... get the shadow color
//-----------------------------------------------------------------------------
void CClientShadowMgr::LevelInitPreEntity()
{
	m_bUpdatingDirtyShadows = false;

	Vector ambientColor;
	engine->GetAmbientLightColor( ambientColor );
	ambientColor *= 3;
	ambientColor += Vector( 0.3f, 0.3f, 0.3f );

	unsigned char r = ambientColor[0] > 1.0 ? 255 : 255 * ambientColor[0];
	unsigned char g = ambientColor[1] > 1.0 ? 255 : 255 * ambientColor[1];
	unsigned char b = ambientColor[2] > 1.0 ? 255 : 255 * ambientColor[2];

	SetShadowColor(r, g, b);

	// Set up the texture allocator
	if ( m_RenderToTextureActive )
	{
		m_ShadowAllocator.Reset();
		m_bRenderTargetNeedsClear = true;
	}
}


//-----------------------------------------------------------------------------
// Clean up all shadows
//-----------------------------------------------------------------------------
void CClientShadowMgr::LevelShutdownPostEntity()
{
	// All shadows *should* have been cleaned up when the entities went away
	// but, just in case....
	Assert( m_Shadows.Count() == 0 );

	ClientShadowHandle_t h = m_Shadows.Head();
	while (h != CLIENTSHADOW_INVALID_HANDLE)
	{
		ClientShadowHandle_t next = m_Shadows.Next(h);
		DestroyShadow( h );
		h = next;
	}

	// Deallocate all textures
	if (m_RenderToTextureActive)
	{
		m_ShadowAllocator.DeallocateAllTextures();
	}

	r_shadows_gamecontrol.SetValue( -1 );
}


//-----------------------------------------------------------------------------
// Deals with alt-tab
//-----------------------------------------------------------------------------
void CClientShadowMgr::RestoreRenderState()
{
	// Mark all shadows dirty; they need to regenerate their state
	ClientShadowHandle_t h;
	for ( h = m_Shadows.Head(); h != m_Shadows.InvalidIndex(); h = m_Shadows.Next(h) )
	{
		m_Shadows[h].m_Flags |= SHADOW_FLAGS_TEXTURE_DIRTY;
	}

	SetShadowColor( m_AmbientLightColor.r, m_AmbientLightColor.g, m_AmbientLightColor.b );
	m_bRenderTargetNeedsClear = true;
}


//-----------------------------------------------------------------------------
// Does all the lovely stuff we need to do to have render-to-texture shadows
//-----------------------------------------------------------------------------
void CClientShadowMgr::SetupRenderToTextureShadow( ClientShadowHandle_t h )
{
	// First, compute how much texture memory we want to use.
	ClientShadow_t& shadow = m_Shadows[h];
	
	IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( shadow.m_Entity );
	if ( !pRenderable )
		return;

	Vector mins, maxs;
	pRenderable->GetShadowRenderBounds( mins, maxs, GetActualShadowCastType( h ) );

	// Compute the maximum dimension
	Vector size;
	VectorSubtract( maxs, mins, size );
	float maxSize = MAX( size.x, size.y );
	maxSize = MAX( maxSize, size.z );

	// Figure out the texture size
	// For now, we're going to assume a fixed number of shadow texels
	// per shadow-caster size; add in some extra space at the boundary.
	int texelCount = TEXEL_SIZE_PER_CASTER_SIZE * maxSize;
	
	// Pick the first power of 2 larger...
	int textureSize = 1;
	while (textureSize < texelCount)
	{
		textureSize <<= 1;
	}

	shadow.m_ShadowTexture = m_ShadowAllocator.AllocateTexture( textureSize, textureSize );
}


void CClientShadowMgr::CleanUpRenderToTextureShadow( ClientShadowHandle_t h )
{
	ClientShadow_t& shadow = m_Shadows[h];
	if (m_RenderToTextureActive && (shadow.m_Flags & SHADOW_FLAGS_USE_RENDER_TO_TEXTURE))
	{
		m_ShadowAllocator.DeallocateTexture( shadow.m_ShadowTexture );
		shadow.m_ShadowTexture = INVALID_TEXTURE_HANDLE;
	}
}


//-----------------------------------------------------------------------------
// Causes all shadows to be re-updated
//-----------------------------------------------------------------------------
void CClientShadowMgr::UpdateAllShadows()
{
	for ( ClientShadowHandle_t i = m_Shadows.Head(); i != m_Shadows.InvalidIndex(); i = m_Shadows.Next(i) )
	{
		ClientShadow_t& shadow = m_Shadows[i];

		// Don't bother with flashlights
		if ( ( shadow.m_Flags & SHADOW_FLAGS_FLASHLIGHT ) != 0 )
			continue;

		IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( shadow.m_Entity );
		if ( !pRenderable )
			continue;

		Assert( pRenderable->GetShadowHandle() == i );
		AddToDirtyShadowList( pRenderable, true );
	}
}


//-----------------------------------------------------------------------------
// Sets the shadow direction
//-----------------------------------------------------------------------------
void CClientShadowMgr::SetShadowDirection( const Vector& dir )
{
	VectorCopy( dir, m_SimpleShadowDir );
	VectorNormalize( m_SimpleShadowDir );

	if ( m_RenderToTextureActive )
	{
		UpdateAllShadows();
	}
}

const Vector &CClientShadowMgr::GetShadowDirection() const
{
	// This will cause blobby shadows to always project straight down
	static Vector s_vecDown( 0, 0, -1 );
	if ( !m_RenderToTextureActive )
		return s_vecDown;

	return m_SimpleShadowDir;
}


//-----------------------------------------------------------------------------
// Gets shadow information for a particular renderable
//-----------------------------------------------------------------------------
float CClientShadowMgr::GetShadowDistance( IClientRenderable *pRenderable ) const
{
	float flDist = m_flShadowCastDist;

	// Allow the renderable to override the default
	pRenderable->GetShadowCastDistance( &flDist, GetActualShadowCastType( pRenderable ) );

	return flDist;
}

const Vector &CClientShadowMgr::GetShadowDirection( IClientRenderable *pRenderable ) const
{
	Vector &vecResult = AllocTempVector();
	vecResult = GetShadowDirection();

	// Allow the renderable to override the default
	pRenderable->GetShadowCastDirection( &vecResult, GetActualShadowCastType( pRenderable ) );

	return vecResult;
}


//-----------------------------------------------------------------------------
// Sets the shadow distance
//-----------------------------------------------------------------------------
void CClientShadowMgr::SetShadowDistance( float flMaxDistance )
{
	m_flShadowCastDist = flMaxDistance;
	UpdateAllShadows();
}

float CClientShadowMgr::GetShadowDistance( ) const
{
	return m_flShadowCastDist;
}


//-----------------------------------------------------------------------------
// Sets the screen area at which blobby shadows are always used
//-----------------------------------------------------------------------------
void CClientShadowMgr::SetShadowBlobbyCutoffArea( float flMinArea )
{
	m_flMinShadowArea = flMinArea;
}

float CClientShadowMgr::GetBlobbyCutoffArea( ) const
{
	return m_flMinShadowArea;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientShadowMgr::SetFalloffBias( ClientShadowHandle_t handle, unsigned char ucBias )
{
	shadowmgr->SetFalloffBias( m_Shadows[handle].m_ShadowHandle, ucBias );
}

//-----------------------------------------------------------------------------
// Returns the shadow texture
//-----------------------------------------------------------------------------
ITexture* CClientShadowMgr::GetShadowTexture( unsigned short h )
{
	return m_ShadowAllocator.GetTexture();
}


//-----------------------------------------------------------------------------
// Returns information needed by the model proxy
//-----------------------------------------------------------------------------
const ShadowInfo_t& CClientShadowMgr::GetShadowInfo( ClientShadowHandle_t h )
{
	return shadowmgr->GetInfo( m_Shadows[h].m_ShadowHandle );
}


//-----------------------------------------------------------------------------
// Renders the shadow texture to screen...
//-----------------------------------------------------------------------------
void CClientShadowMgr::RenderShadowTexture( int w, int h )
{
	if (m_RenderToTextureActive)
	{
		CMatRenderContextPtr pRenderContext( materials );
		pRenderContext->Bind( m_RenderShadow );
		IMesh* pMesh = pRenderContext->GetDynamicMesh( true );

		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

		meshBuilder.Position3f( 0.0f, 0.0f, 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.Color4ub( 0, 0, 0, 0 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( w, 0.0f, 0.0f );
		meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
		meshBuilder.Color4ub( 0, 0, 0, 0 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( w, h, 0.0f );
		meshBuilder.TexCoord2f( 0, 1.0f, 1.0f );
		meshBuilder.Color4ub( 0, 0, 0, 0 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( 0.0f, h, 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f, 1.0f );
		meshBuilder.Color4ub( 0, 0, 0, 0 );
		meshBuilder.AdvanceVertex();

		meshBuilder.End();
		pMesh->Draw();
	}
}


//-----------------------------------------------------------------------------
// Create/destroy a shadow
//-----------------------------------------------------------------------------
ClientShadowHandle_t CClientShadowMgr::CreateProjectedTexture( ClientEntityHandle_t entity, int flags )
{
	// We need to know if it's a brush model for shadows
	if( !( flags & SHADOW_FLAGS_FLASHLIGHT ) )
	{
		IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( entity );
		int modelType = modelinfo->GetModelType( pRenderable->GetModel() );
		if (modelType == mod_brush)
		{
			flags |= SHADOW_FLAGS_BRUSH_MODEL;
		}
	}

	ClientShadowHandle_t h = m_Shadows.AddToTail();
	ClientShadow_t& shadow = m_Shadows[h];
	shadow.m_Entity = entity;
	shadow.m_ClientLeafShadowHandle = ClientLeafSystem()->AddShadow( h, flags );
	shadow.m_Flags = flags;
	shadow.m_nRenderFrame = -1;
	shadow.m_LastOrigin.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	shadow.m_LastAngles.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	Assert( ( ( shadow.m_Flags & SHADOW_FLAGS_FLASHLIGHT ) == 0 ) != 
			( ( shadow.m_Flags & SHADOW_FLAGS_SHADOW ) == 0 ) );

	// Set up the flags....
	IMaterial* pShadowMaterial = m_SimpleShadow;
	IMaterial* pShadowModelMaterial = m_SimpleShadow;
	void* pShadowProxyData = (void*)CLIENTSHADOW_INVALID_HANDLE;

	if ( m_RenderToTextureActive && (flags & SHADOW_FLAGS_USE_RENDER_TO_TEXTURE) )
	{
		SetupRenderToTextureShadow(h);

		pShadowMaterial = m_RenderShadow;
		pShadowModelMaterial = m_RenderModelShadow;
		pShadowProxyData = (void*)(uintp)h;
	}

	if( flags & SHADOW_FLAGS_USE_DEPTH_TEXTURE )
	{
		pShadowMaterial = m_RenderShadow;
		pShadowModelMaterial = m_RenderModelShadow;
		pShadowProxyData = (void*)(uintp)h;
	}

	int createShadowFlags;
	if( flags & SHADOW_FLAGS_FLASHLIGHT )
	{
		// don't use SHADOW_CACHE_VERTS with projective lightsources since we expect that they will change every frame.
		// FIXME: might want to make it cache optionally if it's an entity light that is static.
		createShadowFlags = SHADOW_FLASHLIGHT;
	}
	else
	{
		createShadowFlags = SHADOW_CACHE_VERTS;
	}
	shadow.m_ShadowHandle = shadowmgr->CreateShadowEx( pShadowMaterial, pShadowModelMaterial, pShadowProxyData, createShadowFlags );
	return h;
}

ClientShadowHandle_t CClientShadowMgr::CreateFlashlight( const FlashlightState_t &lightState )
{
	// We don't really need a model entity handle for a projective light source, so use an invalid one.
	static ClientEntityHandle_t invalidHandle = INVALID_CLIENTENTITY_HANDLE;

	int shadowFlags = SHADOW_FLAGS_FLASHLIGHT | SHADOW_FLAGS_LIGHT_WORLD;
	if( lightState.m_bEnableShadows && r_flashlightdepthtexture.GetBool() )
	{
		shadowFlags |= SHADOW_FLAGS_USE_DEPTH_TEXTURE;
	}

	ClientShadowHandle_t shadowHandle = CreateProjectedTexture( invalidHandle, shadowFlags );

	UpdateFlashlightState( shadowHandle, lightState );
	UpdateProjectedTexture( shadowHandle, true );
	return shadowHandle;
}
		 
ClientShadowHandle_t CClientShadowMgr::CreateShadow( ClientEntityHandle_t entity, int flags )
{
	// We don't really need a model entity handle for a projective light source, so use an invalid one.
	flags &= ~SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK;
	flags |= SHADOW_FLAGS_SHADOW | SHADOW_FLAGS_TEXTURE_DIRTY;
	ClientShadowHandle_t shadowHandle = CreateProjectedTexture( entity, flags );

	IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( entity );
	if ( pRenderable )
	{
		Assert( !pRenderable->IsShadowDirty( ) );
		pRenderable->MarkShadowDirty( true );
	}

	// NOTE: We *have* to call the version that takes a shadow handle
	// even if we have an entity because this entity hasn't set its shadow handle yet
	AddToDirtyShadowList( shadowHandle, true );
	return shadowHandle;
}


//-----------------------------------------------------------------------------
// Updates the flashlight direction and re-computes surfaces it should lie on
//-----------------------------------------------------------------------------
void CClientShadowMgr::UpdateFlashlightState( ClientShadowHandle_t shadowHandle, const FlashlightState_t &flashlightState )
{
	VPROF_BUDGET( "CClientShadowMgr::UpdateFlashlightState", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

	BuildPerspectiveWorldToFlashlightMatrix( m_Shadows[shadowHandle].m_WorldToShadow, flashlightState );
											
	shadowmgr->UpdateFlashlightState( m_Shadows[shadowHandle].m_ShadowHandle, flashlightState );
}

void CClientShadowMgr::DestroyFlashlight( ClientShadowHandle_t shadowHandle )
{
	DestroyShadow( shadowHandle );
}

//-----------------------------------------------------------------------------
// Remove a shadow from the dirty list
//-----------------------------------------------------------------------------
void CClientShadowMgr::RemoveShadowFromDirtyList( ClientShadowHandle_t handle )
{
	int idx = m_DirtyShadows.Find( handle );
	if ( idx != m_DirtyShadows.InvalidIndex() )
	{
		// Clean up the shadow update bit.
		IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( m_Shadows[handle].m_Entity );
		if ( pRenderable )
		{
			pRenderable->MarkShadowDirty( false );
		}
		m_DirtyShadows.RemoveAt( idx );
	}
}


//-----------------------------------------------------------------------------
// Remove a shadow
//-----------------------------------------------------------------------------
void CClientShadowMgr::DestroyShadow( ClientShadowHandle_t handle )
{
	Assert( m_Shadows.IsValidIndex(handle) );
	RemoveShadowFromDirtyList( handle );
	shadowmgr->DestroyShadow( m_Shadows[handle].m_ShadowHandle );
	ClientLeafSystem()->RemoveShadow( m_Shadows[handle].m_ClientLeafShadowHandle );
	CleanUpRenderToTextureShadow( handle );
	m_Shadows.Remove(handle);
}


//-----------------------------------------------------------------------------
// Build the worldtotexture matrix
//-----------------------------------------------------------------------------
void CClientShadowMgr::BuildGeneralWorldToShadowMatrix( VMatrix& matWorldToShadow,
	const Vector& origin, const Vector& dir, const Vector& xvec, const Vector& yvec )
{
	// We're assuming here that xvec + yvec aren't necessary perpendicular

	// The shadow->world matrix is pretty simple:
	// Just stick the origin in the translation component
	// and the vectors in the columns...
	matWorldToShadow.SetBasisVectors( xvec, yvec, dir );
	matWorldToShadow.SetTranslation( origin );
	matWorldToShadow[3][0] = matWorldToShadow[3][1] = matWorldToShadow[3][2] = 0.0f;
	matWorldToShadow[3][3] = 1.0f;

	// Now do a general inverse to get matWorldToShadow
	MatrixInverseGeneral( matWorldToShadow, matWorldToShadow );
}

void CClientShadowMgr::BuildWorldToShadowMatrix( VMatrix& matWorldToShadow,	const Vector& origin, const Quaternion& quatOrientation )
{
	// The shadow->world matrix is pretty simple:
	// Just stick the origin in the translation component
	// and the vectors in the columns...
	// The inverse of this transposes the rotational component
	// and the translational component =  - (rotation transpose) * origin

	matrix3x4_t matOrientation;											
	QuaternionMatrix( quatOrientation, matOrientation );		// Convert quat to matrix3x4
	PositionMatrix( vec3_origin, matOrientation );				// Zero out translation elements

	VMatrix matBasis( matOrientation );							// Convert matrix3x4 to VMatrix

	Vector vForward, vLeft, vUp;
	matBasis.GetBasisVectors( vForward, vLeft, vUp );
	matBasis.SetForward( vLeft );								// Bizarre vector flip inherited from earlier code, WTF?
	matBasis.SetLeft( vUp );
	matBasis.SetUp( vForward );
	matWorldToShadow = matBasis.Transpose();					// Transpose

	Vector translation;
	Vector3DMultiply( matWorldToShadow, origin, translation );

	translation *= -1.0f;
	matWorldToShadow.SetTranslation( translation );

	// The the bottom row.
	matWorldToShadow[3][0] = matWorldToShadow[3][1] = matWorldToShadow[3][2] = 0.0f;
	matWorldToShadow[3][3] = 1.0f;
}

void CClientShadowMgr::BuildPerspectiveWorldToFlashlightMatrix( VMatrix& matWorldToShadow, const FlashlightState_t &flashlightState )
{
	VPROF_BUDGET( "CClientShadowMgr::BuildPerspectiveWorldToFlashlightMatrix", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

	// Buildworld to shadow matrix, then perspective projection and concatenate
	VMatrix matWorldToShadowView, matPerspective;
	BuildWorldToShadowMatrix( matWorldToShadowView, flashlightState.m_vecLightOrigin,
							  flashlightState.m_quatOrientation );

	MatrixBuildPerspective( matPerspective, flashlightState.m_fHorizontalFOVDegrees,
							flashlightState.m_fVerticalFOVDegrees,
							flashlightState.m_NearZ, flashlightState.m_FarZ );

	MatrixMultiply( matPerspective, matWorldToShadowView, matWorldToShadow );
}

//-----------------------------------------------------------------------------
// Compute the shadow origin and attenuation start distance
//-----------------------------------------------------------------------------
float CClientShadowMgr::ComputeLocalShadowOrigin( IClientRenderable* pRenderable, 
	const Vector& mins, const Vector& maxs, const Vector& localShadowDir, float backupFactor, Vector& origin )
{
	// Compute the centroid of the object...
	Vector vecCentroid;
	VectorAdd( mins, maxs, vecCentroid );
	vecCentroid *= 0.5f;

	Vector vecSize;
	VectorSubtract( maxs, mins, vecSize );
	float flRadius = vecSize.Length() * 0.5f;

	// NOTE: The *origin* of the shadow cast is a point on a line passing through
	// the centroid of the caster. The direction of this line is the shadow cast direction,
	// and the point on that line corresponds to the endpoint of the box that is
	// furthest *back* along the shadow direction

	// For the first point at which the shadow could possibly start falling off,
	// we need to use the point at which the ray described above leaves the
	// bounding sphere surrounding the entity. This is necessary because otherwise,
	// tall, thin objects would have their shadows appear + disappear as then spun about their origin

	// Figure out the corner corresponding to the min + max projection
	// along the shadow direction

	// We're basically finding the point on the cube that has the largest and smallest
	// dot product with the local shadow dir. Then we're taking the dot product
	// of that with the localShadowDir. lastly, we're subtracting out the
	// centroid projection to give us a distance along the localShadowDir to
	// the front and back of the cube along the direction of the ray.
	float centroidProjection = DotProduct( vecCentroid, localShadowDir );
	float minDist = -centroidProjection;
	for (int i = 0; i < 3; ++i)
	{
		if ( localShadowDir[i] > 0.0f )
		{
			minDist += localShadowDir[i] * mins[i];
		}
		else
		{
			minDist += localShadowDir[i] * maxs[i];
		}
	}

	minDist *= backupFactor;

	VectorMA( vecCentroid, minDist, localShadowDir, origin );

	return flRadius - minDist;
}


//-----------------------------------------------------------------------------
// Sorts the components of a vector
//-----------------------------------------------------------------------------
static inline void SortAbsVectorComponents( const Vector& src, int* pVecIdx )
{
	Vector absVec( fabs(src[0]), fabs(src[1]), fabs(src[2]) );

	int maxIdx = (absVec[0] > absVec[1]) ? 0 : 1;
	if (absVec[2] > absVec[maxIdx])
	{
		maxIdx = 2;
	}

	// always choose something right-handed....
	switch(	maxIdx )
	{
	case 0:
		pVecIdx[0] = 1;
		pVecIdx[1] = 2;
		pVecIdx[2] = 0;
		break;
	case 1:
		pVecIdx[0] = 2;
		pVecIdx[1] = 0;
		pVecIdx[2] = 1;
		break;
	case 2:
		pVecIdx[0] = 0;
		pVecIdx[1] = 1;
		pVecIdx[2] = 2;
		break;
	}
}


//-----------------------------------------------------------------------------
// Build the worldtotexture matrix
//-----------------------------------------------------------------------------
static void BuildWorldToTextureMatrix( const VMatrix& matWorldToShadow, 
							const Vector2D& size, VMatrix& matWorldToTexture )
{
	// Build a matrix that maps from shadow space to (u,v) coordinates
	VMatrix shadowToUnit;
	MatrixBuildScale( shadowToUnit, 1.0f / size.x, 1.0f / size.y, 1.0f );
	shadowToUnit[0][3] = shadowToUnit[1][3] = 0.5f;

	// Store off the world to (u,v) transformation
	MatrixMultiply( shadowToUnit, matWorldToShadow, matWorldToTexture );
}



static void BuildOrthoWorldToShadowMatrix( VMatrix& worldToShadow,
													 const Vector& origin, const Vector& dir, const Vector& xvec, const Vector& yvec )
{
	// This version is faster and assumes dir, xvec, yvec are perpendicular
	AssertFloatEquals( DotProduct( dir, xvec ), 0.0f, 1e-3 );
	AssertFloatEquals( DotProduct( dir, yvec ), 0.0f, 1e-3 );
	AssertFloatEquals( DotProduct( xvec, yvec ), 0.0f, 1e-3 );

	// The shadow->world matrix is pretty simple:
	// Just stick the origin in the translation component
	// and the vectors in the columns...
	// The inverse of this transposes the rotational component
	// and the translational component =  - (rotation transpose) * origin
	worldToShadow.SetBasisVectors( xvec, yvec, dir );
	MatrixTranspose( worldToShadow, worldToShadow );

	Vector translation;
	Vector3DMultiply( worldToShadow, origin, translation );

	translation *= -1.0f;
	worldToShadow.SetTranslation( translation );

	// The the bottom row.
	worldToShadow[3][0] = worldToShadow[3][1] = worldToShadow[3][2] = 0.0f;
	worldToShadow[3][3] = 1.0f;
}


//-----------------------------------------------------------------------------
// Set extra clip planes related to shadows...
//-----------------------------------------------------------------------------
void CClientShadowMgr::ClearExtraClipPlanes( ClientShadowHandle_t h )
{
	shadowmgr->ClearExtraClipPlanes( m_Shadows[h].m_ShadowHandle );
}

void CClientShadowMgr::AddExtraClipPlane( ClientShadowHandle_t h, const Vector& normal, float dist )
{
	shadowmgr->AddExtraClipPlane( m_Shadows[h].m_ShadowHandle, normal, dist );
}


//-----------------------------------------------------------------------------
// Compute the extra shadow planes
//-----------------------------------------------------------------------------
void CClientShadowMgr::ComputeExtraClipPlanes( IClientRenderable* pRenderable, 
	ClientShadowHandle_t handle, const Vector* vec, 
	const Vector& mins, const Vector& maxs, const Vector& localShadowDir )
{
	// Compute the world-space position of the corner of the bounding box
	// that's got the highest dotproduct with the local shadow dir...
	Vector origin = pRenderable->GetRenderOrigin( );
	float dir[3];

	int i;
	for ( i = 0; i < 3; ++i )
	{
		if (localShadowDir[i] < 0.0f)
		{
			VectorMA( origin, maxs[i], vec[i], origin );
			dir[i] = 1;
		}
		else
		{
			VectorMA( origin, mins[i], vec[i], origin );
			dir[i] = -1;
		}
	}

	// Now that we have it, create 3 planes...
	Vector normal;
	ClearExtraClipPlanes(handle);
	for ( i = 0; i < 3; ++i )
	{
		VectorMultiply( vec[i], dir[i], normal );
		float dist = DotProduct( normal, origin );
		AddExtraClipPlane( handle, normal, dist );
	}

	ClientShadow_t& shadow = m_Shadows[handle];
	C_BaseEntity *pEntity = ClientEntityList().GetBaseEntityFromHandle( shadow.m_Entity );
	if ( pEntity && pEntity->m_bEnableRenderingClipPlane )
	{
		normal[ 0 ] = -pEntity->m_fRenderingClipPlane[ 0 ];
		normal[ 1 ] = -pEntity->m_fRenderingClipPlane[ 1 ];
		normal[ 2 ] = -pEntity->m_fRenderingClipPlane[ 2 ];
		AddExtraClipPlane( handle, normal, -pEntity->m_fRenderingClipPlane[ 3 ] - 0.5f );
	}
}


inline ShadowType_t CClientShadowMgr::GetActualShadowCastType( ClientShadowHandle_t handle ) const
{
	if ( handle == CLIENTSHADOW_INVALID_HANDLE )
	{
		return SHADOWS_NONE;
	}
	
	if ( m_Shadows[handle].m_Flags & SHADOW_FLAGS_USE_RENDER_TO_TEXTURE )
	{
		return ( m_RenderToTextureActive ? SHADOWS_RENDER_TO_TEXTURE : SHADOWS_SIMPLE );
	}
	else if( m_Shadows[handle].m_Flags & SHADOW_FLAGS_USE_DEPTH_TEXTURE )
	{
		return SHADOWS_RENDER_TO_DEPTH_TEXTURE;
	}
	else
	{
		return SHADOWS_SIMPLE;
	}
}

inline ShadowType_t CClientShadowMgr::GetActualShadowCastType( IClientRenderable *pEnt ) const
{
	return GetActualShadowCastType( pEnt->GetShadowHandle() );
}


//-----------------------------------------------------------------------------
// Adds a shadow to all leaves along a ray
//-----------------------------------------------------------------------------
class CShadowLeafEnum : public ISpatialLeafEnumerator
{
public:
	bool EnumerateLeaf( int leaf, int context )
	{
		m_LeafList.AddToTail( leaf );
		return true;
	}

	CUtlVectorFixedGrowable< int, 512 > m_LeafList;
};


//-----------------------------------------------------------------------------
// Builds a list of leaves inside the shadow volume
//-----------------------------------------------------------------------------
static void BuildShadowLeafList( CShadowLeafEnum *pEnum, const Vector& origin, 
	const Vector& dir, const Vector2D& size, float maxDist )
{
	Ray_t ray;
	VectorCopy( origin, ray.m_Start );
	VectorMultiply( dir, maxDist, ray.m_Delta );
	ray.m_StartOffset.Init( 0, 0, 0 );

	float flRadius = sqrt( size.x * size.x + size.y * size.y ) * 0.5f;
	ray.m_Extents.Init( flRadius, flRadius, flRadius );
	ray.m_IsRay = false;
	ray.m_IsSwept = true;

	ISpatialQuery* pQuery = engine->GetBSPTreeQuery();
	pQuery->EnumerateLeavesAlongRay( ray, pEnum, 0 );
}


//-----------------------------------------------------------------------------
// Builds a simple blobby shadow
//-----------------------------------------------------------------------------
void CClientShadowMgr::BuildOrthoShadow( IClientRenderable* pRenderable, 
		ClientShadowHandle_t handle, const Vector& mins, const Vector& maxs)
{
	// Get the object's basis
	Vector vec[3];
	AngleVectors( pRenderable->GetRenderAngles(), &vec[0], &vec[1], &vec[2] );
	vec[1] *= -1.0f;

	Vector vecShadowDir = GetShadowDirection( pRenderable );

	// Project the shadow casting direction into the space of the object
	Vector localShadowDir;
	localShadowDir[0] = DotProduct( vec[0], vecShadowDir );
	localShadowDir[1] = DotProduct( vec[1], vecShadowDir );
	localShadowDir[2] = DotProduct( vec[2], vecShadowDir );

	// Figure out which vector has the largest component perpendicular
	// to the shadow handle...
	// Sort by how perpendicular it is
	int vecIdx[3];
	SortAbsVectorComponents( localShadowDir, vecIdx );

	// Here's our shadow basis vectors; namely the ones that are
	// most perpendicular to the shadow casting direction
	Vector xvec = vec[vecIdx[0]];
	Vector yvec = vec[vecIdx[1]];

	// Project them into a plane perpendicular to the shadow direction
	xvec -= vecShadowDir * DotProduct( vecShadowDir, xvec );
	yvec -= vecShadowDir * DotProduct( vecShadowDir, yvec );
	VectorNormalize( xvec );
	VectorNormalize( yvec );

	// Compute the box size
	Vector boxSize;
	VectorSubtract( maxs, mins, boxSize );

	// We project the two longest sides into the vectors perpendicular
	// to the projection direction, then add in the projection of the perp direction
	Vector2D size( boxSize[vecIdx[0]], boxSize[vecIdx[1]] );
	size.x *= fabs( DotProduct( vec[vecIdx[0]], xvec ) );
	size.y *= fabs( DotProduct( vec[vecIdx[1]], yvec ) );

	// Add the third component into x and y
	size.x += boxSize[vecIdx[2]] * fabs( DotProduct( vec[vecIdx[2]], xvec ) );
	size.y += boxSize[vecIdx[2]] * fabs( DotProduct( vec[vecIdx[2]], yvec ) );

	// Bloat a bit, since the shadow wants to extend outside the model a bit
	size.x += 10.0f;
	size.y += 10.0f;

	// Clamp the minimum size
	Vector2DMax( size, Vector2D(10.0f, 10.0f), size );

	// Place the origin at the point with min dot product with shadow dir
	Vector org;
	float falloffStart = ComputeLocalShadowOrigin( pRenderable, mins, maxs, localShadowDir, 2.0f, org );

	// Transform the local origin into world coordinates
	Vector worldOrigin = pRenderable->GetRenderOrigin( );
	VectorMA( worldOrigin, org.x, vec[0], worldOrigin );
	VectorMA( worldOrigin, org.y, vec[1], worldOrigin );
	VectorMA( worldOrigin, org.z, vec[2], worldOrigin );

	// FUNKY: A trick to reduce annoying texelization artifacts!?
	float dx = 1.0f / TEXEL_SIZE_PER_CASTER_SIZE;
	worldOrigin.x = (int)(worldOrigin.x / dx) * dx;
	worldOrigin.y = (int)(worldOrigin.y / dx) * dx;
	worldOrigin.z = (int)(worldOrigin.z / dx) * dx;

	// NOTE: We gotta use the general matrix because xvec and yvec aren't perp
	VMatrix matWorldToShadow, matWorldToTexture;
	BuildGeneralWorldToShadowMatrix( m_Shadows[handle].m_WorldToShadow, worldOrigin, vecShadowDir, xvec, yvec );
	BuildWorldToTextureMatrix( m_Shadows[handle].m_WorldToShadow, size, matWorldToTexture );
	Vector2DCopy( size, m_Shadows[handle].m_WorldSize );
	
	// Compute the falloff attenuation
	// Area computation isn't exact since xvec is not perp to yvec, but close enough
//	float shadowArea = size.x * size.y;	

	// The entity may be overriding our shadow cast distance
	float flShadowCastDistance = GetShadowDistance( pRenderable );
	float maxHeight = flShadowCastDistance + falloffStart; //3.0f * sqrt( shadowArea );

	CShadowLeafEnum leafList;
	BuildShadowLeafList( &leafList, worldOrigin, vecShadowDir, size, maxHeight );
	int nCount = leafList.m_LeafList.Count();
	const int *pLeafList = leafList.m_LeafList.Base();

	shadowmgr->ProjectShadow( m_Shadows[handle].m_ShadowHandle, worldOrigin,
		vecShadowDir, matWorldToTexture, size, nCount, pLeafList, maxHeight, falloffStart, MAX_FALLOFF_AMOUNT, pRenderable->GetRenderOrigin() );

	// Compute extra clip planes to prevent poke-thru
// FIXME!!!!!!!!!!!!!!  Removing this for now since it seems to mess up the blobby shadows.
//	ComputeExtraClipPlanes( pEnt, handle, vec, mins, maxs, localShadowDir );

	// Add the shadow to the client leaf system so it correctly marks 
	// leafs as being affected by a particular shadow
	ClientLeafSystem()->ProjectShadow( m_Shadows[handle].m_ClientLeafShadowHandle, nCount, pLeafList );
}


//-----------------------------------------------------------------------------
// Visualization....
//-----------------------------------------------------------------------------
void CClientShadowMgr::DrawRenderToTextureDebugInfo( IClientRenderable* pRenderable, const Vector& mins, const Vector& maxs )
{   
	// Get the object's basis
	Vector vec[3];
	AngleVectors( pRenderable->GetRenderAngles(), &vec[0], &vec[1], &vec[2] );
	vec[1] *= -1.0f;

	Vector vecSize;
	VectorSubtract( maxs, mins, vecSize );

	Vector vecOrigin = pRenderable->GetRenderOrigin();
	Vector start, end, end2;

	VectorMA( vecOrigin, mins.x, vec[0], start );
	VectorMA( start, mins.y, vec[1], start );
	VectorMA( start, mins.z, vec[2], start );

	VectorMA( start, vecSize.x, vec[0], end );
	VectorMA( end, vecSize.z, vec[2], end2 );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 ); 
	debugoverlay->AddLineOverlay( end2, end, 255, 0, 0, true, 0.01 ); 

	VectorMA( start, vecSize.y, vec[1], end );
	VectorMA( end, vecSize.z, vec[2], end2 );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 ); 
	debugoverlay->AddLineOverlay( end2, end, 255, 0, 0, true, 0.01 ); 

	VectorMA( start, vecSize.z, vec[2], end );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 );
	
	start = end;
	VectorMA( start, vecSize.x, vec[0], end );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 ); 

	VectorMA( start, vecSize.y, vec[1], end );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 ); 

	VectorMA( end, vecSize.x, vec[0], start );
	VectorMA( start, -vecSize.x, vec[0], end );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 ); 

	VectorMA( start, -vecSize.y, vec[1], end );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 ); 

	VectorMA( start, -vecSize.z, vec[2], end );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 );

	start = end;
	VectorMA( start, -vecSize.x, vec[0], end );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 ); 

	VectorMA( start, -vecSize.y, vec[1], end );
	debugoverlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.01 ); 

	C_BaseEntity *pEnt = pRenderable->GetIClientUnknown()->GetBaseEntity();
	if ( pEnt )
	{
		debugoverlay->AddTextOverlay( vecOrigin, 0, "%d", pEnt->entindex() );
	}
	else
	{
		debugoverlay->AddTextOverlay( vecOrigin, 0, "%X", (size_t)pRenderable );
	}
}


extern ConVar cl_drawshadowtexture;
extern ConVar cl_shadowtextureoverlaysize;

//-----------------------------------------------------------------------------
// Builds a more complex shadow...
//-----------------------------------------------------------------------------
void CClientShadowMgr::BuildRenderToTextureShadow( IClientRenderable* pRenderable, 
		ClientShadowHandle_t handle, const Vector& mins, const Vector& maxs)
{
	if ( cl_drawshadowtexture.GetInt() )
	{
		// Red wireframe bounding box around objects whose RTT shadows are being updated that frame
		DrawRenderToTextureDebugInfo( pRenderable, mins, maxs );
	}

	// Get the object's basis
	Vector vec[3];
	AngleVectors( pRenderable->GetRenderAngles(), &vec[0], &vec[1], &vec[2] );
	vec[1] *= -1.0f;

	Vector vecShadowDir = GetShadowDirection( pRenderable );

//	Debugging aid
//	const model_t *pModel = pRenderable->GetModel();
//	const char *pDebugName = modelinfo->GetModelName( pModel );

	// Project the shadow casting direction into the space of the object
	Vector localShadowDir;
	localShadowDir[0] = DotProduct( vec[0], vecShadowDir );
	localShadowDir[1] = DotProduct( vec[1], vecShadowDir );
	localShadowDir[2] = DotProduct( vec[2], vecShadowDir );

	// Compute the box size
	Vector boxSize;
	VectorSubtract( maxs, mins, boxSize );
	
	Vector yvec;
	float fProjMax = 0.0f;
	for( int i = 0; i != 3; ++i )
	{
		Vector test = vec[i] - ( vecShadowDir * DotProduct( vecShadowDir, vec[i] ) );
		test *= boxSize[i]; //doing after the projection to simplify projection math
		float fLengthSqr = test.LengthSqr();
		if( fLengthSqr > fProjMax )
		{
			fProjMax = fLengthSqr;
			yvec = test;
		}
	}		

	VectorNormalize( yvec );

	// Compute the x vector
	Vector xvec;
	CrossProduct( yvec, vecShadowDir, xvec );

	// We project the two longest sides into the vectors perpendicular
	// to the projection direction, then add in the projection of the perp direction
	Vector2D size;
	size.x = boxSize.x * fabs( DotProduct( vec[0], xvec ) ) + 
		boxSize.y * fabs( DotProduct( vec[1], xvec ) ) + 
		boxSize.z * fabs( DotProduct( vec[2], xvec ) );
	size.y = boxSize.x * fabs( DotProduct( vec[0], yvec ) ) + 
		boxSize.y * fabs( DotProduct( vec[1], yvec ) ) + 
		boxSize.z * fabs( DotProduct( vec[2], yvec ) );

	size.x += 2.0f * TEXEL_SIZE_PER_CASTER_SIZE;
	size.y += 2.0f * TEXEL_SIZE_PER_CASTER_SIZE;

	// Place the origin at the point with min dot product with shadow dir
	Vector org;
	float falloffStart = ComputeLocalShadowOrigin( pRenderable, mins, maxs, localShadowDir, 1.0f, org );

	// Transform the local origin into world coordinates
	Vector worldOrigin = pRenderable->GetRenderOrigin( );
	VectorMA( worldOrigin, org.x, vec[0], worldOrigin );
	VectorMA( worldOrigin, org.y, vec[1], worldOrigin );
	VectorMA( worldOrigin, org.z, vec[2], worldOrigin );

	VMatrix matWorldToTexture;
	BuildOrthoWorldToShadowMatrix( m_Shadows[handle].m_WorldToShadow, worldOrigin, vecShadowDir, xvec, yvec );
	BuildWorldToTextureMatrix( m_Shadows[handle].m_WorldToShadow, size, matWorldToTexture );
	Vector2DCopy( size, m_Shadows[handle].m_WorldSize );

	// Compute the falloff attenuation
	// Area computation isn't exact since xvec is not perp to yvec, but close enough
	// Extra factor of 4 in the maxHeight due to the size being half as big
//	float shadowArea = size.x * size.y;	

	// The entity may be overriding our shadow cast distance
	float flShadowCastDistance = GetShadowDistance( pRenderable );
	float maxHeight = flShadowCastDistance + falloffStart; //3.0f * sqrt( shadowArea );

	CShadowLeafEnum leafList;
	BuildShadowLeafList( &leafList, worldOrigin, vecShadowDir, size, maxHeight );
	int nCount = leafList.m_LeafList.Count();
	const int *pLeafList = leafList.m_LeafList.Base();

	shadowmgr->ProjectShadow( m_Shadows[handle].m_ShadowHandle, worldOrigin, 
		vecShadowDir, matWorldToTexture, size, nCount, pLeafList, maxHeight, falloffStart, MAX_FALLOFF_AMOUNT, pRenderable->GetRenderOrigin() );

	// Compute extra clip planes to prevent poke-thru
	ComputeExtraClipPlanes( pRenderable, handle, vec, mins, maxs, localShadowDir );

	// Add the shadow to the client leaf system so it correctly marks 
	// leafs as being affected by a particular shadow
	ClientLeafSystem()->ProjectShadow( m_Shadows[handle].m_ClientLeafShadowHandle, nCount, pLeafList );
}

static void LineDrawHelper( const Vector &startShadowSpace, const Vector &endShadowSpace, 
						   const VMatrix &shadowToWorld, unsigned char r = 255, unsigned char g = 255, 
						   unsigned char b = 255 )
{
	Vector startWorldSpace, endWorldSpace;
	Vector3DMultiplyPositionProjective( shadowToWorld, startShadowSpace, startWorldSpace );
	Vector3DMultiplyPositionProjective( shadowToWorld, endShadowSpace, endWorldSpace );

	debugoverlay->AddLineOverlay( startWorldSpace + Vector( 0.0f, 0.0f, 1.0f ), 
		endWorldSpace + Vector( 0.0f, 0.0f, 1.0f ), r, g, b, false, -1 );
}

static void DebugDrawFrustum( const Vector &vOrigin, const VMatrix &matWorldToFlashlight )
{
	VMatrix flashlightToWorld;
	MatrixInverseGeneral( matWorldToFlashlight, flashlightToWorld );
	
	// Draw boundaries of frustum
	LineDrawHelper( Vector( 0.0f, 0.0f, 0.0f ), Vector( 0.0f, 0.0f, 1.0f ), flashlightToWorld, 255, 255, 255 );
	LineDrawHelper( Vector( 0.0f, 0.0f, 1.0f ), Vector( 0.0f, 1.0f, 1.0f ), flashlightToWorld, 255, 255, 255 );
	LineDrawHelper( Vector( 0.0f, 1.0f, 1.0f ), Vector( 0.0f, 1.0f, 0.0f ), flashlightToWorld, 255, 255, 255 );
	LineDrawHelper( Vector( 0.0f, 1.0f, 0.0f ), Vector( 0.0f, 0.0f, 0.0f ), flashlightToWorld, 255, 255, 255 );
	LineDrawHelper( Vector( 1.0f, 0.0f, 0.0f ), Vector( 1.0f, 0.0f, 1.0f ), flashlightToWorld, 255, 255, 255 );
	LineDrawHelper( Vector( 1.0f, 0.0f, 1.0f ), Vector( 1.0f, 1.0f, 1.0f ), flashlightToWorld, 255, 255, 255 );
	LineDrawHelper( Vector( 1.0f, 1.0f, 1.0f ), Vector( 1.0f, 1.0f, 0.0f ), flashlightToWorld, 255, 255, 255 );
	LineDrawHelper( Vector( 1.0f, 1.0f, 0.0f ), Vector( 1.0f, 0.0f, 0.0f ), flashlightToWorld, 255, 255, 255 );
	LineDrawHelper( Vector( 0.0f, 0.0f, 0.0f ), Vector( 1.0f, 0.0f, 0.0f ), flashlightToWorld, 255, 255, 255 );
	LineDrawHelper( Vector( 0.0f, 0.0f, 1.0f ), Vector( 1.0f, 0.0f, 1.0f ), flashlightToWorld, 255, 255, 255 );
	LineDrawHelper( Vector( 0.0f, 1.0f, 1.0f ), Vector( 1.0f, 1.0f, 1.0f ), flashlightToWorld, 255, 255, 255 );
	LineDrawHelper( Vector( 0.0f, 1.0f, 0.0f ), Vector( 1.0f, 1.0f, 0.0f ), flashlightToWorld, 255, 255, 255 );

	// Draw RGB triad at front plane
	LineDrawHelper( Vector( 0.5f, 0.5f, 0.0f ), Vector( 1.0f, 0.5f, 0.0f ),  flashlightToWorld, 255,   0,   0 );
	LineDrawHelper( Vector( 0.5f, 0.5f, 0.0f ), Vector( 0.5f, 1.0f, 0.0f ),  flashlightToWorld,   0, 255,   0 );
	LineDrawHelper( Vector( 0.5f, 0.5f, 0.0f ), Vector( 0.5f, 0.5f, 0.35f ), flashlightToWorld,   0,   0, 255 );
}


//-----------------------------------------------------------------------------
// Builds a list of leaves inside the flashlight volume
//-----------------------------------------------------------------------------
static void BuildFlashlightLeafList( CShadowLeafEnum *pEnum, const VMatrix &worldToShadow )
{
	// Use an AABB around the frustum to enumerate leaves.
	Vector mins, maxs;
	CalculateAABBFromProjectionMatrix( worldToShadow, &mins, &maxs );
	ISpatialQuery* pQuery = engine->GetBSPTreeQuery();
	pQuery->EnumerateLeavesInBox( mins, maxs, pEnum, 0 );
}


void CClientShadowMgr::BuildFlashlight( ClientShadowHandle_t handle )
{
	// For the 360, we just draw flashlights with the main geometry
	// and bypass the entire shadow casting system.
	ClientShadow_t &shadow = m_Shadows[handle];
	if ( IsX360() || r_flashlight_version2.GetInt() )
	{
		// This will update the matrices, but not do work to add the flashlight to surfaces
		shadowmgr->ProjectFlashlight( shadow.m_ShadowHandle, shadow.m_WorldToShadow, 0, NULL );
		return;
	}

	VPROF_BUDGET( "CClientShadowMgr::BuildFlashlight", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

	bool bLightModels = r_flashlightmodels.GetBool();
	bool bLightSpecificEntity = shadow.m_hTargetEntity.Get() != NULL;
	bool bLightWorld = ( shadow.m_Flags & SHADOW_FLAGS_LIGHT_WORLD ) != 0;
	int nCount = 0;
	const int *pLeafList = 0;

	CShadowLeafEnum leafList;
	if ( bLightWorld || ( bLightModels && !bLightSpecificEntity ) )
	{
		BuildFlashlightLeafList( &leafList, shadow.m_WorldToShadow );
		nCount = leafList.m_LeafList.Count();
		pLeafList = leafList.m_LeafList.Base();
	}

	if( bLightWorld )
	{
		shadowmgr->ProjectFlashlight( shadow.m_ShadowHandle, shadow.m_WorldToShadow, nCount, pLeafList );
	}
	else
	{
		// This should clear all models and surfaces from this shadow
		shadowmgr->EnableShadow( shadow.m_ShadowHandle, false );
		shadowmgr->EnableShadow( shadow.m_ShadowHandle, true );
	}

	if ( !bLightModels )
		return;

	if ( !bLightSpecificEntity )
	{
		// Add the shadow to the client leaf system so it correctly marks 
		// leafs as being affected by a particular shadow
		ClientLeafSystem()->ProjectFlashlight( shadow.m_ClientLeafShadowHandle, nCount, pLeafList );
		return;
	}

	// We know what we are focused on, so just add the shadow directly to that receiver
	Assert( shadow.m_hTargetEntity->GetModel() );

	C_BaseEntity *pChild = shadow.m_hTargetEntity->FirstMoveChild();
	while( pChild )
	{
		int modelType = modelinfo->GetModelType( pChild->GetModel() );
		if (modelType == mod_brush)
		{
			AddShadowToReceiver( handle, pChild, SHADOW_RECEIVER_BRUSH_MODEL );
		}
		else if ( modelType == mod_studio )
		{
			AddShadowToReceiver( handle, pChild, SHADOW_RECEIVER_STUDIO_MODEL );
		}

		pChild = pChild->NextMovePeer();
	}

	int modelType = modelinfo->GetModelType( shadow.m_hTargetEntity->GetModel() );
	if (modelType == mod_brush)
	{
		AddShadowToReceiver( handle, shadow.m_hTargetEntity, SHADOW_RECEIVER_BRUSH_MODEL );
	}
	else if ( modelType == mod_studio )
	{
		AddShadowToReceiver( handle, shadow.m_hTargetEntity, SHADOW_RECEIVER_STUDIO_MODEL );
	}
}


//-----------------------------------------------------------------------------
// Adds the child bounds to the bounding box
//-----------------------------------------------------------------------------
void CClientShadowMgr::AddChildBounds( matrix3x4_t &matWorldToBBox, IClientRenderable* pParent, Vector &vecMins, Vector &vecMaxs )
{
	Vector vecChildMins, vecChildMaxs;
	Vector vecNewChildMins, vecNewChildMaxs;
	matrix3x4_t childToBBox;

	IClientRenderable *pChild = pParent->FirstShadowChild();
	while( pChild )
	{
		// Transform the child bbox into the space of the main bbox
		// FIXME: Optimize this?
		if ( GetActualShadowCastType( pChild ) != SHADOWS_NONE)
		{
			pChild->GetShadowRenderBounds( vecChildMins, vecChildMaxs, SHADOWS_RENDER_TO_TEXTURE );
			ConcatTransforms( matWorldToBBox, pChild->RenderableToWorldTransform(), childToBBox );
			TransformAABB( childToBBox, vecChildMins, vecChildMaxs, vecNewChildMins, vecNewChildMaxs );
			VectorMin( vecMins, vecNewChildMins, vecMins );
			VectorMax( vecMaxs, vecNewChildMaxs, vecMaxs );
		}

		AddChildBounds( matWorldToBBox, pChild, vecMins, vecMaxs );
		pChild = pChild->NextShadowPeer();
	}
}


//-----------------------------------------------------------------------------
// Compute a bounds for the entity + children
//-----------------------------------------------------------------------------
void CClientShadowMgr::ComputeHierarchicalBounds( IClientRenderable *pRenderable, Vector &vecMins, Vector &vecMaxs )
{
	ShadowType_t shadowType = GetActualShadowCastType( pRenderable );

	pRenderable->GetShadowRenderBounds( vecMins, vecMaxs, shadowType );

	// We could use a good solution for this in the regular PC build, since
	// it causes lots of extra bone setups for entities you can't see.
	if ( IsPC() )
	{
		IClientRenderable *pChild = pRenderable->FirstShadowChild();

		// Don't recurse down the tree when we hit a blobby shadow
		if ( pChild && shadowType != SHADOWS_SIMPLE )
		{
			matrix3x4_t matWorldToBBox;
			MatrixInvert( pRenderable->RenderableToWorldTransform(), matWorldToBBox );
			AddChildBounds( matWorldToBBox, pRenderable, vecMins, vecMaxs );
		}
	}
}


//-----------------------------------------------------------------------------
// Shadow update functions
//-----------------------------------------------------------------------------
void CClientShadowMgr::UpdateStudioShadow( IClientRenderable *pRenderable, ClientShadowHandle_t handle )
{
	if( !( m_Shadows[handle].m_Flags & SHADOW_FLAGS_FLASHLIGHT ) )
	{
		Vector mins, maxs;
		ComputeHierarchicalBounds( pRenderable, mins, maxs );

		ShadowType_t shadowType = GetActualShadowCastType( handle );
		if ( shadowType != SHADOWS_RENDER_TO_TEXTURE )
		{
			BuildOrthoShadow( pRenderable, handle, mins, maxs );
		}
		else
		{
			BuildRenderToTextureShadow( pRenderable, handle, mins, maxs );
		}
	}
	else
	{
		BuildFlashlight( handle );
	}
}

void CClientShadowMgr::UpdateBrushShadow( IClientRenderable *pRenderable, ClientShadowHandle_t handle )
{
	if( !( m_Shadows[handle].m_Flags & SHADOW_FLAGS_FLASHLIGHT ) )
	{
		// Compute the bounding box in the space of the shadow...
		Vector mins, maxs;
		ComputeHierarchicalBounds( pRenderable, mins, maxs );

		ShadowType_t shadowType = GetActualShadowCastType( handle );
		if ( shadowType != SHADOWS_RENDER_TO_TEXTURE )
		{
			BuildOrthoShadow( pRenderable, handle, mins, maxs );
		}
		else
		{
			BuildRenderToTextureShadow( pRenderable, handle, mins, maxs );
		}
	}
	else
	{
		VPROF_BUDGET( "CClientShadowMgr::UpdateBrushShadow", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

		BuildFlashlight( handle );
	}
}


#ifdef _DEBUG

static bool s_bBreak = false;

void ShadowBreak_f()
{
	s_bBreak = true;
}

static ConCommand r_shadowbreak("r_shadowbreak", ShadowBreak_f);

#endif // _DEBUG


bool CClientShadowMgr::WillParentRenderBlobbyShadow( IClientRenderable *pRenderable )
{
	if ( !pRenderable )
		return false;

	IClientRenderable *pShadowParent = pRenderable->GetShadowParent();
	if ( !pShadowParent )
		return false;

	// If there's *no* shadow casting type, then we want to see if we can render into its parent
 	ShadowType_t shadowType = GetActualShadowCastType( pShadowParent );
	if ( shadowType == SHADOWS_NONE )
		return WillParentRenderBlobbyShadow( pShadowParent );

	return shadowType == SHADOWS_SIMPLE;
}


//-----------------------------------------------------------------------------
// Are we the child of a shadow with render-to-texture?
//-----------------------------------------------------------------------------
bool CClientShadowMgr::ShouldUseParentShadow( IClientRenderable *pRenderable )
{
	if ( !pRenderable )
		return false;

	IClientRenderable *pShadowParent = pRenderable->GetShadowParent();
	if ( !pShadowParent )
		return false;

	// Can't render into the parent if the parent is blobby
	ShadowType_t shadowType = GetActualShadowCastType( pShadowParent );
	if ( shadowType == SHADOWS_SIMPLE )
		return false;

	// If there's *no* shadow casting type, then we want to see if we can render into its parent
 	if ( shadowType == SHADOWS_NONE )
		return ShouldUseParentShadow( pShadowParent );

	// Here, the parent uses a render-to-texture shadow
	return true;
}


//-----------------------------------------------------------------------------
// Before we render any view, make sure all shadows are re-projected vs world
//-----------------------------------------------------------------------------
void CClientShadowMgr::PreRender()
{
	VPROF_BUDGET( "CClientShadowMgr::PreRender", VPROF_BUDGETGROUP_SHADOW_RENDERING );
	MDLCACHE_CRITICAL_SECTION();

	//
	// -- Shadow Depth Textures -----------------------
	//

	{
		// VPROF scope
		VPROF_BUDGET( "CClientShadowMgr::PreRender", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

		// If someone turned shadow depth mapping on but we can't do it, force it off
		if ( r_flashlightdepthtexture.GetBool() && !materials->SupportsShadowDepthTextures() )
		{
			r_flashlightdepthtexture.SetValue( 0 );
			ShutdownDepthTextureShadows();	
		}

		bool bDepthTextureActive     = r_flashlightdepthtexture.GetBool();
		int  nDepthTextureResolution = r_flashlightdepthres.GetInt();

		// If shadow depth texture size or enable/disable changed, do appropriate deallocation/(re)allocation
		if ( ( bDepthTextureActive != m_bDepthTextureActive ) || ( nDepthTextureResolution != m_nDepthTextureResolution ) )
		{
			// If shadow depth texturing remains on, but resolution changed, shut down and reinitialize depth textures
			if ( ( bDepthTextureActive == true ) && ( m_bDepthTextureActive == true ) &&
				 ( nDepthTextureResolution != m_nDepthTextureResolution ) )
			{
				ShutdownDepthTextureShadows();	
				InitDepthTextureShadows();
			}
			else
			{
				if ( m_bDepthTextureActive && !bDepthTextureActive )		// Turning off shadow depth texturing
				{
					ShutdownDepthTextureShadows();
				}
				else if ( bDepthTextureActive && !m_bDepthTextureActive)	// Turning on shadow depth mapping
				{
					InitDepthTextureShadows();
				}
			}
		}
	}

	//
	// -- Render to Texture Shadows -----------------------
	//

	bool bRenderToTextureActive = r_shadowrendertotexture.GetBool();
	if ( bRenderToTextureActive != m_RenderToTextureActive )
	{
		if ( m_RenderToTextureActive )
		{
			ShutdownRenderToTextureShadows();
		}
		else
		{
			InitRenderToTextureShadows();
		}

		UpdateAllShadows();
		return;
	}

	m_bUpdatingDirtyShadows = true;

	unsigned short i = m_DirtyShadows.FirstInorder();
	while ( i != m_DirtyShadows.InvalidIndex() )
	{
		MDLCACHE_CRITICAL_SECTION();
		ClientShadowHandle_t& handle = m_DirtyShadows[ i ];
		Assert( m_Shadows.IsValidIndex( handle ) );
		UpdateProjectedTextureInternal( handle, false );
		i = m_DirtyShadows.NextInorder(i);
	}
	m_DirtyShadows.RemoveAll();

	// Transparent shadows must remain dirty, since they were not re-projected
	int nCount = m_TransparentShadows.Count();
	for ( int i = 0; i < nCount; ++i )
	{
		m_DirtyShadows.Insert( m_TransparentShadows[i] );
	}
	m_TransparentShadows.RemoveAll();

	m_bUpdatingDirtyShadows = false;
}


//-----------------------------------------------------------------------------
// Gets the entity whose shadow this shadow will render into
//-----------------------------------------------------------------------------
IClientRenderable *CClientShadowMgr::GetParentShadowEntity( ClientShadowHandle_t handle )
{
	ClientShadow_t& shadow = m_Shadows[handle];
	IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( shadow.m_Entity );
	if ( pRenderable )
	{
		if ( ShouldUseParentShadow( pRenderable ) )
		{
			IClientRenderable *pParent = pRenderable->GetShadowParent();
			while ( GetActualShadowCastType( pParent ) == SHADOWS_NONE )
			{
				pParent = pParent->GetShadowParent();
				Assert( pParent );
			}
			return pParent;
		}
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// Marks a shadow as needing re-projection
//-----------------------------------------------------------------------------
void CClientShadowMgr::AddToDirtyShadowList( ClientShadowHandle_t handle, bool bForce )
{
	// Don't add to the dirty shadow list while we're iterating over it
	// The only way this can happen is if a child is being rendered into a parent
	// shadow, and we don't need it to be added to the dirty list in that case.
	if ( m_bUpdatingDirtyShadows )
		return;

	if ( handle == CLIENTSHADOW_INVALID_HANDLE )
		return;

	Assert( m_DirtyShadows.Find( handle ) == m_DirtyShadows.InvalidIndex() );
	m_DirtyShadows.Insert( handle );

	// This pretty much guarantees we'll recompute the shadow
	if ( bForce )
	{
		m_Shadows[handle].m_LastAngles.Init( FLT_MAX, FLT_MAX, FLT_MAX );
	}

	// If we use our parent shadow, then it's dirty too...
	IClientRenderable *pParent = GetParentShadowEntity( handle );
	if ( pParent )
	{
		AddToDirtyShadowList( pParent, bForce );
	}
}


//-----------------------------------------------------------------------------
// Marks a shadow as needing re-projection
//-----------------------------------------------------------------------------
void CClientShadowMgr::AddToDirtyShadowList( IClientRenderable *pRenderable, bool bForce )
{
	// Don't add to the dirty shadow list while we're iterating over it
	// The only way this can happen is if a child is being rendered into a parent
	// shadow, and we don't need it to be added to the dirty list in that case.
	if ( m_bUpdatingDirtyShadows )
		return;

	// Are we already in the dirty list?
	if ( pRenderable->IsShadowDirty( ) )
		return;

	ClientShadowHandle_t handle = pRenderable->GetShadowHandle();
	if ( handle == CLIENTSHADOW_INVALID_HANDLE )
		return;

#ifdef _DEBUG
	// Make sure everything's consistent
	if ( handle != CLIENTSHADOW_INVALID_HANDLE )
	{
		IClientRenderable *pShadowRenderable = ClientEntityList().GetClientRenderableFromHandle( m_Shadows[handle].m_Entity );
		Assert( pRenderable == pShadowRenderable );
	}
#endif

	pRenderable->MarkShadowDirty( true );
	AddToDirtyShadowList( handle, bForce );
}


//-----------------------------------------------------------------------------
// Marks the render-to-texture shadow as needing to be re-rendered
//-----------------------------------------------------------------------------
void CClientShadowMgr::MarkRenderToTextureShadowDirty( ClientShadowHandle_t handle )
{
	// Don't add bogus handles!
	if (handle != CLIENTSHADOW_INVALID_HANDLE)
	{
		// Mark the shadow has having a dirty renter-to-texture
		ClientShadow_t& shadow = m_Shadows[handle];
		shadow.m_Flags |= SHADOW_FLAGS_TEXTURE_DIRTY;

		// If we use our parent shadow, then it's dirty too...
		IClientRenderable *pParent = GetParentShadowEntity( handle );
		if ( pParent )
		{
			ClientShadowHandle_t parentHandle = pParent->GetShadowHandle();
			if ( parentHandle != CLIENTSHADOW_INVALID_HANDLE )
			{
				m_Shadows[parentHandle].m_Flags |= SHADOW_FLAGS_TEXTURE_DIRTY;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Update a shadow
//-----------------------------------------------------------------------------
void CClientShadowMgr::UpdateShadow( ClientShadowHandle_t handle, bool force )
{
	ClientShadow_t& shadow = m_Shadows[handle];

	// Get the client entity....
	IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( shadow.m_Entity );
	if ( !pRenderable )
	{
		// Retire the shadow if the entity is gone
		DestroyShadow( handle );
		return;
	}

	// Don't bother if there's no model on the renderable
	if ( !pRenderable->GetModel() )
	{
		pRenderable->MarkShadowDirty( false );
		return;
	}

	// FIXME: NOTE! Because this is called from PreRender, the falloff bias is
	// off by a frame. We could move the code in PreRender to occur after world
	// list building is done to fix this issue.
	// Don't bother with it if the shadow is totally transparent
	const ShadowInfo_t &shadowInfo = shadowmgr->GetInfo( shadow.m_ShadowHandle );
	if ( shadowInfo.m_FalloffBias == 255 )
	{
		shadowmgr->EnableShadow( shadow.m_ShadowHandle, false );
		m_TransparentShadows.AddToTail( handle );
		return;
	}

#ifdef _DEBUG
	if (s_bBreak)
	{
		s_bBreak = false;
	}
#endif
	// Hierarchical children shouldn't be projecting shadows...
	// Check to see if it's a child of an entity with a render-to-texture shadow...
	if ( ShouldUseParentShadow( pRenderable ) || WillParentRenderBlobbyShadow( pRenderable ) )
	{
		shadowmgr->EnableShadow( shadow.m_ShadowHandle, false );
		pRenderable->MarkShadowDirty( false );
		return;
	}

	shadowmgr->EnableShadow( shadow.m_ShadowHandle, true );

	// Figure out if the shadow moved...
	// Even though we have dirty bits, some entities
	// never clear those dirty bits
	const Vector& origin = pRenderable->GetRenderOrigin();
	const QAngle& angles = pRenderable->GetRenderAngles();

	if (force || (origin != shadow.m_LastOrigin) || (angles != shadow.m_LastAngles))
	{
		// Store off the new pos/orientation
		VectorCopy( origin, shadow.m_LastOrigin );
		VectorCopy( angles, shadow.m_LastAngles );

		CMatRenderContextPtr pRenderContext( materials );
		const model_t *pModel = pRenderable->GetModel();
		MaterialFogMode_t fogMode = pRenderContext->GetFogMode();
		pRenderContext->FogMode( MATERIAL_FOG_NONE );
		switch( modelinfo->GetModelType( pModel ) )
		{
		case mod_brush:
			UpdateBrushShadow( pRenderable, handle );
			break;

		case mod_studio:
			UpdateStudioShadow( pRenderable, handle );
			break;

		default:
			// Shouldn't get here if not a brush or studio
			Assert(0);
			break;
		}
		pRenderContext->FogMode( fogMode );
	}

	// NOTE: We can't do this earlier because pEnt->GetRenderOrigin() can
	// provoke a recomputation of render origin, which, for aiments, can cause everything
	// to be marked as dirty. So don't clear the flag until this point.
	pRenderable->MarkShadowDirty( false );
}


//-----------------------------------------------------------------------------
// Update a shadow
//-----------------------------------------------------------------------------
void CClientShadowMgr::UpdateProjectedTextureInternal( ClientShadowHandle_t handle, bool force )
{
	ClientShadow_t& shadow = m_Shadows[handle];

	if( shadow.m_Flags & SHADOW_FLAGS_FLASHLIGHT )
	{
		VPROF_BUDGET( "CClientShadowMgr::UpdateProjectedTextureInternal", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

		Assert( ( shadow.m_Flags & SHADOW_FLAGS_SHADOW ) == 0 );
		ClientShadow_t& shadow = m_Shadows[handle];

		shadowmgr->EnableShadow( shadow.m_ShadowHandle, true );

		// FIXME: What's the difference between brush and model shadows for light projectors? Answer: nothing.
		UpdateBrushShadow( NULL, handle );
	}
	else
	{
		Assert( shadow.m_Flags & SHADOW_FLAGS_SHADOW );
		Assert( ( shadow.m_Flags & SHADOW_FLAGS_FLASHLIGHT ) == 0 );
		UpdateShadow( handle, force );
	}
}


//-----------------------------------------------------------------------------
// Update a shadow
//-----------------------------------------------------------------------------
void CClientShadowMgr::UpdateProjectedTexture( ClientShadowHandle_t handle, bool force )
{
	VPROF_BUDGET( "CClientShadowMgr::UpdateProjectedTexture", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

	if ( handle == CLIENTSHADOW_INVALID_HANDLE )
		return;

	// NOTE: This can only work for flashlights, since UpdateProjectedTextureInternal
	// depends on the falloff offset to cull shadows.
	ClientShadow_t &shadow = m_Shadows[ handle ];
	if( ( shadow.m_Flags & SHADOW_FLAGS_FLASHLIGHT ) == 0 )
	{
		Warning( "CClientShadowMgr::UpdateProjectedTexture can only be used with flashlights!\n" );
		return;
	}

	UpdateProjectedTextureInternal( handle, force );
	RemoveShadowFromDirtyList( handle );
}

	
//-----------------------------------------------------------------------------
// Computes bounding sphere
//-----------------------------------------------------------------------------
void CClientShadowMgr::ComputeBoundingSphere( IClientRenderable* pRenderable, Vector& origin, float& radius )
{
	Assert( pRenderable );
	Vector mins, maxs;
	pRenderable->GetShadowRenderBounds( mins, maxs, GetActualShadowCastType( pRenderable ) );
	Vector size;
	VectorSubtract( maxs, mins, size );
	radius = size.Length() * 0.5f;

	// Compute centroid (local space)
	Vector centroid;
	VectorAdd( mins, maxs, centroid );
	centroid *= 0.5f;

	// Transform centroid into world space
	Vector vec[3];
	AngleVectors( pRenderable->GetRenderAngles(), &vec[0], &vec[1], &vec[2] );
	vec[1] *= -1.0f;

	VectorCopy( pRenderable->GetRenderOrigin(), origin );
	VectorMA( origin, centroid.x, vec[0], origin );
	VectorMA( origin, centroid.y, vec[1], origin );
	VectorMA( origin, centroid.z, vec[2], origin );
}


//-----------------------------------------------------------------------------
// Computes a rough AABB encompassing the volume of the shadow
//-----------------------------------------------------------------------------
void CClientShadowMgr::ComputeShadowBBox( IClientRenderable *pRenderable, const Vector &vecAbsCenter, float flRadius, Vector *pAbsMins, Vector *pAbsMaxs )
{
	// This is *really* rough. Basically we simply determine the
	// maximum shadow casting length and extrude the box by that distance

	Vector vecShadowDir = GetShadowDirection( pRenderable );
	for (int i = 0; i < 3; ++i)
	{
		float flShadowCastDistance = GetShadowDistance( pRenderable );
		float flDist = flShadowCastDistance * vecShadowDir[i];

		if (vecShadowDir[i] < 0)
		{
			(*pAbsMins)[i] = vecAbsCenter[i] - flRadius + flDist;
			(*pAbsMaxs)[i] = vecAbsCenter[i] + flRadius;
		}
		else
		{
			(*pAbsMins)[i] = vecAbsCenter[i] - flRadius;
			(*pAbsMaxs)[i] = vecAbsCenter[i] + flRadius + flDist;
		}
	}
}


//-----------------------------------------------------------------------------
// Compute a separating axis...
//-----------------------------------------------------------------------------
bool CClientShadowMgr::ComputeSeparatingPlane( IClientRenderable* pRend1, IClientRenderable* pRend2, cplane_t* pPlane )
{
	Vector min1, max1, min2, max2;
	pRend1->GetShadowRenderBounds( min1, max1, GetActualShadowCastType( pRend1 ) );
	pRend2->GetShadowRenderBounds( min2, max2, GetActualShadowCastType( pRend2 ) );
	return ::ComputeSeparatingPlane( 
		pRend1->GetRenderOrigin(), pRend1->GetRenderAngles(), min1, max1,
		pRend2->GetRenderOrigin(), pRend2->GetRenderAngles(), min2, max2,
		3.0f, pPlane );
}


//-----------------------------------------------------------------------------
// Cull shadows based on rough bounding volumes
//-----------------------------------------------------------------------------
bool CClientShadowMgr::CullReceiver( ClientShadowHandle_t handle, IClientRenderable* pRenderable,
									IClientRenderable* pSourceRenderable )
{
	// check flags here instead and assert !pSourceRenderable
	if( m_Shadows[handle].m_Flags & SHADOW_FLAGS_FLASHLIGHT )
	{
		VPROF_BUDGET( "CClientShadowMgr::CullReceiver", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

		Assert( !pSourceRenderable );	
		const Frustum_t &frustum = shadowmgr->GetFlashlightFrustum( m_Shadows[handle].m_ShadowHandle );

		Vector mins, maxs;
		pRenderable->GetRenderBoundsWorldspace( mins, maxs );

		return R_CullBox( mins, maxs, frustum );
	}

	Assert( pSourceRenderable );	
	// Compute a bounding sphere for the renderable
	Vector origin;
	float radius;
	ComputeBoundingSphere( pRenderable, origin, radius );

	// Transform the sphere center into the space of the shadow
	Vector localOrigin;
	const ClientShadow_t& shadow = m_Shadows[handle];
	const ShadowInfo_t& info = shadowmgr->GetInfo( shadow.m_ShadowHandle );
	Vector3DMultiplyPosition( shadow.m_WorldToShadow, origin, localOrigin );

	// Compute a rough bounding box for the shadow (in shadow space)
	Vector shadowMin, shadowMax;
	shadowMin.Init( -shadow.m_WorldSize.x * 0.5f, -shadow.m_WorldSize.y * 0.5f, 0 );
	shadowMax.Init( shadow.m_WorldSize.x * 0.5f, shadow.m_WorldSize.y * 0.5f, info.m_MaxDist );

	// If the bounding sphere doesn't intersect with the shadow volume, cull
	if (!IsBoxIntersectingSphere( shadowMin, shadowMax, localOrigin, radius ))
		return true;

	Vector originSource;
	float radiusSource;
	ComputeBoundingSphere( pSourceRenderable, originSource, radiusSource );

	// Fast check for separating plane...
	bool foundSeparatingPlane = false;
	cplane_t plane;
	if (!IsSphereIntersectingSphere( originSource, radiusSource, origin, radius ))
	{
		foundSeparatingPlane = true;

		// the plane normal doesn't need to be normalized...
		VectorSubtract( origin, originSource, plane.normal );
	}
	else
	{
		foundSeparatingPlane = ComputeSeparatingPlane( pRenderable, pSourceRenderable, &plane );
	}

	if (foundSeparatingPlane)
	{
		// Compute which side of the plane the renderable is on..
		Vector vecShadowDir = GetShadowDirection( pSourceRenderable );
		float shadowDot = DotProduct( vecShadowDir, plane.normal );
		float receiverDot = DotProduct( plane.normal, origin );
		float sourceDot = DotProduct( plane.normal, originSource );

		if (shadowDot > 0.0f)
		{
			if (receiverDot <= sourceDot)
			{
//				Vector dest;
//				VectorMA( pSourceRenderable->GetRenderOrigin(), 50, plane.normal, dest ); 
//				debugoverlay->AddLineOverlay( pSourceRenderable->GetRenderOrigin(), dest, 255, 255, 0, true, 1.0f );
				return true;
			}
			else
			{
//				Vector dest;
//				VectorMA( pSourceRenderable->GetRenderOrigin(), 50, plane.normal, dest ); 
//				debugoverlay->AddLineOverlay( pSourceRenderable->GetRenderOrigin(), dest, 255, 0, 0, true, 1.0f );
			}
		}
		else
		{
			if (receiverDot >= sourceDot)
			{
//				Vector dest;
//				VectorMA( pSourceRenderable->GetRenderOrigin(), -50, plane.normal, dest ); 
//				debugoverlay->AddLineOverlay( pSourceRenderable->GetRenderOrigin(), dest, 255, 255, 0, true, 1.0f );
				return true;
			}
			else
			{
//				Vector dest;
//				VectorMA( pSourceRenderable->GetRenderOrigin(), 50, plane.normal, dest ); 
//				debugoverlay->AddLineOverlay( pSourceRenderable->GetRenderOrigin(), dest, 255, 0, 0, true, 1.0f );
			}
		}
	}

	// No additional clip planes? ok then it's a valid receiver
	/*
	if (shadow.m_ClipPlaneCount == 0)
		return false;

	// Check the additional cull planes
	int i;
	for ( i = 0; i < shadow.m_ClipPlaneCount; ++i)
	{
		// Fast sphere cull
		if (DotProduct( origin, shadow.m_ClipPlane[i] ) - radius > shadow.m_ClipDist[i])
			return true;
	}

	// More expensive box on plane side cull...
	Vector vec[3];
	Vector mins, maxs;
	cplane_t plane;
	AngleVectors( pRenderable->GetRenderAngles(), &vec[0], &vec[1], &vec[2] );
	pRenderable->GetBounds( mins, maxs );

	for ( i = 0; i < shadow.m_ClipPlaneCount; ++i)
	{
		// Transform the plane into the space of the receiver
		plane.normal.x = DotProduct( vec[0], shadow.m_ClipPlane[i] );
		plane.normal.y = DotProduct( vec[1], shadow.m_ClipPlane[i] );
		plane.normal.z = DotProduct( vec[2], shadow.m_ClipPlane[i] );

		plane.dist = shadow.m_ClipDist[i] - DotProduct( shadow.m_ClipPlane[i], pRenderable->GetRenderOrigin() );

		// If the box is on the front side of the plane, we're done.
		if (BoxOnPlaneSide2( mins, maxs, &plane, 3.0f ) == 1)
			return true;
	}
	*/

	return false;
}


//-----------------------------------------------------------------------------
// deals with shadows being added to shadow receivers
//-----------------------------------------------------------------------------
void CClientShadowMgr::AddShadowToReceiver( ClientShadowHandle_t handle,
	IClientRenderable* pRenderable, ShadowReceiver_t type )
{
	ClientShadow_t &shadow = m_Shadows[handle];

	// Don't add a shadow cast by an object to itself...
	IClientRenderable* pSourceRenderable = ClientEntityList().GetClientRenderableFromHandle( shadow.m_Entity );

	// NOTE: if pSourceRenderable == NULL, the source is probably a flashlight since there is no entity.
	if (pSourceRenderable == pRenderable)
		return;

	// Don't bother if this renderable doesn't receive shadows or light from flashlights
	if( !pRenderable->ShouldReceiveProjectedTextures( SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK ) )
		return;

	// Cull if the origin is on the wrong side of a shadow clip plane....
	if ( CullReceiver( handle, pRenderable, pSourceRenderable ) )
		return;

	// Do different things depending on the receiver type
	switch( type )
	{
	case SHADOW_RECEIVER_BRUSH_MODEL:

		if( shadow.m_Flags & SHADOW_FLAGS_FLASHLIGHT )
		{
			VPROF_BUDGET( "CClientShadowMgr::AddShadowToReceiver", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

			if( (!shadow.m_hTargetEntity) || IsFlashlightTarget( handle, pRenderable ) )
			{
				shadowmgr->AddShadowToBrushModel( shadow.m_ShadowHandle, 
					const_cast<model_t*>(pRenderable->GetModel()),
					pRenderable->GetRenderOrigin(), pRenderable->GetRenderAngles() );

				shadowmgr->AddFlashlightRenderable( shadow.m_ShadowHandle, pRenderable );
			}
		}
		else
		{
			shadowmgr->AddShadowToBrushModel( shadow.m_ShadowHandle, 
				const_cast<model_t*>(pRenderable->GetModel()),
				pRenderable->GetRenderOrigin(), pRenderable->GetRenderAngles() );
		}
		break;

	case SHADOW_RECEIVER_STATIC_PROP:
		// Don't add shadows to props if we're not using render-to-texture
		if ( GetActualShadowCastType( handle ) == SHADOWS_RENDER_TO_TEXTURE )
		{
			// Also don't add them unless an NPC or player casts them..
			// They are wickedly expensive!!!
			C_BaseEntity *pEnt = pSourceRenderable->GetIClientUnknown()->GetBaseEntity();
			if ( pEnt && ( pEnt->GetFlags() & (FL_NPC | FL_CLIENT)) )
			{
				staticpropmgr->AddShadowToStaticProp( shadow.m_ShadowHandle, pRenderable );
			}
		}
		else if( shadow.m_Flags & SHADOW_FLAGS_FLASHLIGHT )
		{
			VPROF_BUDGET( "CClientShadowMgr::AddShadowToReceiver", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

			if( (!shadow.m_hTargetEntity) || IsFlashlightTarget( handle, pRenderable ) )
			{
				staticpropmgr->AddShadowToStaticProp( shadow.m_ShadowHandle, pRenderable );

				shadowmgr->AddFlashlightRenderable( shadow.m_ShadowHandle, pRenderable );
			}
		}
		break;

	case SHADOW_RECEIVER_STUDIO_MODEL:
		if( shadow.m_Flags & SHADOW_FLAGS_FLASHLIGHT )
		{
			VPROF_BUDGET( "CClientShadowMgr::AddShadowToReceiver", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

			if( (!shadow.m_hTargetEntity) || IsFlashlightTarget( handle, pRenderable ) )
			{
				pRenderable->CreateModelInstance();
				shadowmgr->AddShadowToModel( shadow.m_ShadowHandle, pRenderable->GetModelInstance() );
				shadowmgr->AddFlashlightRenderable( shadow.m_ShadowHandle, pRenderable );
			}
		}
		break;
//	default:
	}
}


//-----------------------------------------------------------------------------
// deals with shadows being added to shadow receivers
//-----------------------------------------------------------------------------
void CClientShadowMgr::RemoveAllShadowsFromReceiver( 
					IClientRenderable* pRenderable, ShadowReceiver_t type )
{
	// Don't bother if this renderable doesn't receive shadows
	if ( !pRenderable->ShouldReceiveProjectedTextures( SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK ) )
		return;

	// Do different things depending on the receiver type
	switch( type )
	{
	case SHADOW_RECEIVER_BRUSH_MODEL:
		{
			model_t* pModel = const_cast<model_t*>(pRenderable->GetModel());
			shadowmgr->RemoveAllShadowsFromBrushModel( pModel );
		}
		break;

	case SHADOW_RECEIVER_STATIC_PROP:
		staticpropmgr->RemoveAllShadowsFromStaticProp(pRenderable);
		break;

	case SHADOW_RECEIVER_STUDIO_MODEL:
		if( pRenderable && pRenderable->GetModelInstance() != MODEL_INSTANCE_INVALID )
		{
			shadowmgr->RemoveAllShadowsFromModel( pRenderable->GetModelInstance() );
		}
		break;

//	default:
//		// FIXME: How do deal with this stuff? Add a method to IClientRenderable?
//		C_BaseEntity* pEnt = static_cast<C_BaseEntity*>(pRenderable);
//		pEnt->RemoveAllShadows();
	}
}


//-----------------------------------------------------------------------------
// Computes + sets the render-to-texture texcoords
//-----------------------------------------------------------------------------
void CClientShadowMgr::SetRenderToTextureShadowTexCoords( ShadowHandle_t handle, int x, int y, int w, int h )
{
	// Let the shadow mgr know about the texture coordinates...
	// That way it'll be able to batch rendering better.
	int textureW, textureH;
	m_ShadowAllocator.GetTotalTextureSize( textureW, textureH );

	// Go in a half-pixel to avoid blending with neighboring textures..
	float u, v, du, dv;

	u  = ((float)x + 0.5f) / (float)textureW;
	v  = ((float)y + 0.5f) / (float)textureH;
	du = ((float)w - 1) / (float)textureW;
	dv = ((float)h - 1) / (float)textureH;

	shadowmgr->SetShadowTexCoord( handle, u, v, du, dv );
}


//-----------------------------------------------------------------------------
// Setup all children shadows
//-----------------------------------------------------------------------------
bool CClientShadowMgr::BuildSetupShadowHierarchy( IClientRenderable *pRenderable, const ClientShadow_t &shadow, bool bChild )
{
	bool bDrewTexture = false;

	// Stop traversing when we hit a blobby shadow
	ShadowType_t shadowType = GetActualShadowCastType( pRenderable );
	if ( pRenderable && shadowType == SHADOWS_SIMPLE )
		return false;

	if ( !pRenderable || shadowType != SHADOWS_NONE )
	{
		bool bDrawModelShadow;
		if ( !bChild )
		{
			bDrawModelShadow = ((shadow.m_Flags & SHADOW_FLAGS_BRUSH_MODEL) == 0);
		}
		else
		{
			int nModelType = modelinfo->GetModelType( pRenderable->GetModel() );
			bDrawModelShadow = nModelType == mod_studio;
		}

		if ( bDrawModelShadow )
		{
			C_BaseEntity *pEntity = pRenderable->GetIClientUnknown()->GetBaseEntity();
			if ( pEntity )
			{
				if ( pEntity->IsNPC() )
				{
					s_NPCShadowBoneSetups.AddToTail( assert_cast<C_BaseAnimating *>( pEntity ) );
				}
				else if ( pEntity->GetBaseAnimating() )
				{
					s_NonNPCShadowBoneSetups.AddToTail( assert_cast<C_BaseAnimating *>( pEntity ) );
				}

			}
			bDrewTexture = true;
		}
	}

	if ( !pRenderable )
		return bDrewTexture;

	IClientRenderable *pChild;
	for ( pChild = pRenderable->FirstShadowChild(); pChild; pChild = pChild->NextShadowPeer() )
	{
		if ( BuildSetupShadowHierarchy( pChild, shadow, true ) )
		{
			bDrewTexture = true;
		}
	}
	return bDrewTexture;
}

//-----------------------------------------------------------------------------
// Draws all children shadows into our own
//-----------------------------------------------------------------------------
bool CClientShadowMgr::DrawShadowHierarchy( IClientRenderable *pRenderable, const ClientShadow_t &shadow, bool bChild )
{
	bool bDrewTexture = false;

	// Stop traversing when we hit a blobby shadow
	ShadowType_t shadowType = GetActualShadowCastType( pRenderable );
	if ( pRenderable && shadowType == SHADOWS_SIMPLE )
		return false;

	if ( !pRenderable || shadowType != SHADOWS_NONE )
	{
		bool bDrawModelShadow;
		bool bDrawBrushShadow;
		if ( !bChild )
		{
			bDrawModelShadow = ((shadow.m_Flags & SHADOW_FLAGS_BRUSH_MODEL) == 0);
			bDrawBrushShadow = !bDrawModelShadow;
		}
		else
		{
			int nModelType = modelinfo->GetModelType( pRenderable->GetModel() );
			bDrawModelShadow = nModelType == mod_studio;
			bDrawBrushShadow = nModelType == mod_brush;
		}
    
		if ( bDrawModelShadow )
		{
			DrawModelInfo_t info;
			matrix3x4_t *pBoneToWorld = modelrender->DrawModelShadowSetup( pRenderable, pRenderable->GetBody(), pRenderable->GetSkin(), &info );
			if ( pBoneToWorld )
			{
				modelrender->DrawModelShadow( pRenderable, info, pBoneToWorld );
			}
			bDrewTexture = true;
		}
		else if ( bDrawBrushShadow )
		{
			render->DrawBrushModelShadow( pRenderable );
			bDrewTexture = true;
		}
	}

	if ( !pRenderable )
		return bDrewTexture;

	IClientRenderable *pChild;
	for ( pChild = pRenderable->FirstShadowChild(); pChild; pChild = pChild->NextShadowPeer() )
	{
		if ( DrawShadowHierarchy( pChild, shadow, true ) )
		{
			bDrewTexture = true;
		}
	}
	return bDrewTexture;
}

//-----------------------------------------------------------------------------
// This gets called with every shadow that potentially will need to re-render
//-----------------------------------------------------------------------------
bool CClientShadowMgr::BuildSetupListForRenderToTextureShadow( unsigned short clientShadowHandle, float flArea )
{
	ClientShadow_t& shadow = m_Shadows[clientShadowHandle];
	bool bDirtyTexture = (shadow.m_Flags & SHADOW_FLAGS_TEXTURE_DIRTY) != 0;
	bool bNeedsRedraw = m_ShadowAllocator.UseTexture( shadow.m_ShadowTexture, bDirtyTexture, flArea );
	if ( bNeedsRedraw || bDirtyTexture )
	{
		shadow.m_Flags |= SHADOW_FLAGS_TEXTURE_DIRTY;

		if ( !m_ShadowAllocator.HasValidTexture( shadow.m_ShadowTexture ) )
			return false;

		// shadow to be redrawn; for now, we'll always do it.
		IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( shadow.m_Entity );

		if ( BuildSetupShadowHierarchy( pRenderable, shadow ) )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// This gets called with every shadow that potentially will need to re-render
//-----------------------------------------------------------------------------
bool CClientShadowMgr::DrawRenderToTextureShadow( unsigned short clientShadowHandle, float flArea )
{
	ClientShadow_t& shadow = m_Shadows[clientShadowHandle];

	// If we were previously using the LOD shadow, set the material
	bool bPreviouslyUsingLODShadow = ( shadow.m_Flags & SHADOW_FLAGS_USING_LOD_SHADOW ) != 0; 
	shadow.m_Flags &= ~SHADOW_FLAGS_USING_LOD_SHADOW;
	if ( bPreviouslyUsingLODShadow )
	{
		shadowmgr->SetShadowMaterial( shadow.m_ShadowHandle, m_RenderShadow, m_RenderModelShadow, (void*)(uintp)clientShadowHandle );
	}

	// Mark texture as being used...
	bool bDirtyTexture = (shadow.m_Flags & SHADOW_FLAGS_TEXTURE_DIRTY) != 0;
	bool bDrewTexture = false;
	bool bNeedsRedraw = ( !m_bThreaded && m_ShadowAllocator.UseTexture( shadow.m_ShadowTexture, bDirtyTexture, flArea ) );

	if ( !m_ShadowAllocator.HasValidTexture( shadow.m_ShadowTexture ) )
	{
		DrawRenderToTextureShadowLOD( clientShadowHandle );
		return false;
	}

	if ( bNeedsRedraw || bDirtyTexture )
	{
		// shadow to be redrawn; for now, we'll always do it.
		IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( shadow.m_Entity );

		CMatRenderContextPtr pRenderContext( materials );
		
		// Sets the viewport state
		int x, y, w, h;
		m_ShadowAllocator.GetTextureRect( shadow.m_ShadowTexture, x, y, w, h );
		pRenderContext->Viewport( IsX360() ? 0 : x, IsX360() ? 0 : y, w, h ); 

		// Clear the selected viewport only (don't need to clear depth)
		pRenderContext->ClearBuffers( true, false );

		pRenderContext->MatrixMode( MATERIAL_VIEW );
		pRenderContext->LoadMatrix( shadowmgr->GetInfo( shadow.m_ShadowHandle ).m_WorldToShadow );
   
		if ( DrawShadowHierarchy( pRenderable, shadow ) )
		{
			bDrewTexture = true;
			if ( IsX360() )
			{
				// resolve render target to system memory texture
				Rect_t srcRect = { 0, 0, w, h };
				Rect_t dstRect = { x, y, w, h };
				pRenderContext->CopyRenderTargetToTextureEx( m_ShadowAllocator.GetTexture(), 0, &srcRect, &dstRect );
			}
		}
		else
		{
			// NOTE: Think the flags reset + texcoord set should only happen in DrawShadowHierarchy
			// but it's 2 days before 360 ship.. not going to change this now.
			DevMsg( "Didn't draw shadow hierarchy.. bad shadow texcoords probably going to happen..grab Brian!\n" );
		}

		// Only clear the dirty flag if the caster isn't animating
		if ( (shadow.m_Flags & SHADOW_FLAGS_ANIMATING_SOURCE) == 0 )
		{
			shadow.m_Flags &= ~SHADOW_FLAGS_TEXTURE_DIRTY;
		}

		SetRenderToTextureShadowTexCoords( shadow.m_ShadowHandle, x, y, w, h );
	}
	else if ( bPreviouslyUsingLODShadow )
	{
		// In this case, we were previously using the LOD shadow, but we didn't
		// have to reconstitute the texture. In this case, we need to reset the texcoord
		int x, y, w, h;
		m_ShadowAllocator.GetTextureRect( shadow.m_ShadowTexture, x, y, w, h );
		SetRenderToTextureShadowTexCoords( shadow.m_ShadowHandle, x, y, w, h );
	}

	return bDrewTexture;
}


//-----------------------------------------------------------------------------
// "Draws" the shadow LOD, which really means just set up the blobby shadow
//-----------------------------------------------------------------------------
void CClientShadowMgr::DrawRenderToTextureShadowLOD( unsigned short clientShadowHandle )
{
	ClientShadow_t &shadow = m_Shadows[clientShadowHandle];
	if ( (shadow.m_Flags & SHADOW_FLAGS_USING_LOD_SHADOW) == 0 )
	{
		shadowmgr->SetShadowMaterial( shadow.m_ShadowHandle, m_SimpleShadow, m_SimpleShadow, (void*)CLIENTSHADOW_INVALID_HANDLE );
		shadowmgr->SetShadowTexCoord( shadow.m_ShadowHandle, 0, 0, 1, 1 );
		ClearExtraClipPlanes( clientShadowHandle ); // this was ClearExtraClipPlanes( shadow.m_ShadowHandle ), fix is from Joe Demers
		shadow.m_Flags |= SHADOW_FLAGS_USING_LOD_SHADOW;
	}
}


//-----------------------------------------------------------------------------
// Advances to the next frame, 
//-----------------------------------------------------------------------------
void CClientShadowMgr::AdvanceFrame()
{
	// We're starting the next frame
	m_ShadowAllocator.AdvanceFrame();
}


//-----------------------------------------------------------------------------
// Re-render shadow depth textures that lie in the leaf list
//-----------------------------------------------------------------------------
int CClientShadowMgr::BuildActiveShadowDepthList( const CViewSetup &viewSetup, int nMaxDepthShadows, ClientShadowHandle_t *pActiveDepthShadows )
{
	int nActiveDepthShadowCount = 0;
	for ( ClientShadowHandle_t i = m_Shadows.Head(); i != m_Shadows.InvalidIndex(); i = m_Shadows.Next(i) )
	{
		ClientShadow_t& shadow = m_Shadows[i];

		// If this is not a flashlight which should use a shadow depth texture, skip!
		if ( ( shadow.m_Flags & SHADOW_FLAGS_USE_DEPTH_TEXTURE ) == 0 )
			continue;

		const FlashlightState_t& flashlightState = shadowmgr->GetFlashlightState( shadow.m_ShadowHandle );

		// Bail if this flashlight doesn't want shadows
		if ( !flashlightState.m_bEnableShadows )
			continue;

		// Calculate an AABB around the shadow frustum
		Vector vecAbsMins, vecAbsMaxs;
		CalculateAABBFromProjectionMatrix( shadow.m_WorldToShadow, &vecAbsMins, &vecAbsMaxs );

		Frustum_t viewFrustum;
		GeneratePerspectiveFrustum( viewSetup.origin, viewSetup.angles, viewSetup.zNear, viewSetup.zFar, viewSetup.fov, viewSetup.m_flAspectRatio, viewFrustum );

		// FIXME: Could do other sorts of culling here, such as frustum-frustum test, distance etc.
		// If it's not in the view frustum, move on
		if ( R_CullBox( vecAbsMins, vecAbsMaxs, viewFrustum ) )
		{
			shadowmgr->SetFlashlightDepthTexture( shadow.m_ShadowHandle, NULL, 0 );
			continue;
		}

		if ( nActiveDepthShadowCount >= nMaxDepthShadows )
		{
			static bool s_bOverflowWarning = false;
			if ( !s_bOverflowWarning )
			{
				Warning( "Too many depth textures rendered in a single view!\n" );
				Assert( 0 );
				s_bOverflowWarning = true;
			}
			shadowmgr->SetFlashlightDepthTexture( shadow.m_ShadowHandle, NULL, 0 );
			continue;
		}

		pActiveDepthShadows[nActiveDepthShadowCount++] = i;
	}
	return nActiveDepthShadowCount;
}


//-----------------------------------------------------------------------------
// Sets the view's active flashlight render state
//-----------------------------------------------------------------------------
void CClientShadowMgr::SetViewFlashlightState( int nActiveFlashlightCount, ClientShadowHandle_t* pActiveFlashlights )
{
	// NOTE: On the 360, we render the entire scene with the flashlight state
	// set and don't render flashlights additively in the shadow mgr at a far later time
	// because the CPU costs are prohibitive
	if ( !IsX360() && !r_flashlight_version2.GetInt() )
		return;

	Assert( nActiveFlashlightCount<= 1 ); 
	if ( nActiveFlashlightCount > 0 )
	{
		Assert( ( m_Shadows[ pActiveFlashlights[0] ].m_Flags & SHADOW_FLAGS_FLASHLIGHT ) != 0 );
		shadowmgr->SetFlashlightRenderState( pActiveFlashlights[0] );
	}
	else
	{
		shadowmgr->SetFlashlightRenderState( SHADOW_HANDLE_INVALID );
	}
}


//-----------------------------------------------------------------------------
// Re-render shadow depth textures that lie in the leaf list
//-----------------------------------------------------------------------------
void CClientShadowMgr::ComputeShadowDepthTextures( const CViewSetup &viewSetup )
{
	VPROF_BUDGET( "CClientShadowMgr::ComputeShadowDepthTextures", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

	CMatRenderContextPtr pRenderContext( materials );
	PIXEVENT( pRenderContext, "Shadow Depth Textures" );

	// Build list of active render-to-texture shadows
	ClientShadowHandle_t pActiveDepthShadows[1024];
	int nActiveDepthShadowCount = BuildActiveShadowDepthList( viewSetup, ARRAYSIZE( pActiveDepthShadows ), pActiveDepthShadows );

	// Iterate over all existing textures and allocate shadow textures
	bool bDebugFrustum = r_flashlightdrawfrustum.GetBool();
	for ( int j = 0; j < nActiveDepthShadowCount; ++j )
	{
		ClientShadow_t& shadow = m_Shadows[ pActiveDepthShadows[j] ];

		CTextureReference shadowDepthTexture;
		bool bGotShadowDepthTexture = LockShadowDepthTexture( &shadowDepthTexture );
		if ( !bGotShadowDepthTexture )
		{
			// If we don't get one, that means we have too many this frame so bind no depth texture
			static int bitchCount = 0;
			if( bitchCount < 10 )
			{
				Warning( "Too many shadow maps this frame!\n"  );
				bitchCount++;
			}

			Assert(0);
			shadowmgr->SetFlashlightDepthTexture( shadow.m_ShadowHandle, NULL, 0 );
			continue;
		}

		CViewSetup shadowView;
		shadowView.m_flAspectRatio = 1.0f;
		shadowView.x = shadowView.y = 0;
		shadowView.width = shadowDepthTexture->GetActualWidth();
		shadowView.height = shadowDepthTexture->GetActualHeight();
		shadowView.m_bOrtho = false;
		shadowView.m_bDoBloomAndToneMapping = false;

		// Copy flashlight parameters
		const FlashlightState_t& flashlightState = shadowmgr->GetFlashlightState( shadow.m_ShadowHandle );
		shadowView.fov = shadowView.fovViewmodel = flashlightState.m_fHorizontalFOVDegrees;
		shadowView.origin = flashlightState.m_vecLightOrigin;
		QuaternionAngles( flashlightState.m_quatOrientation, shadowView.angles ); // Convert from Quaternion to QAngle

		shadowView.zNear = shadowView.zNearViewmodel = flashlightState.m_NearZ;
		shadowView.zFar = shadowView.zFarViewmodel = flashlightState.m_FarZ;

		// Can turn on all light frustum overlays or per light with flashlightState parameter...
		if ( bDebugFrustum || flashlightState.m_bDrawShadowFrustum )
		{
			DebugDrawFrustum( shadowView.origin, shadow.m_WorldToShadow );
		}

		// Set depth bias factors specific to this flashlight
		CMatRenderContextPtr pRenderContext( materials );
		pRenderContext->SetShadowDepthBiasFactors( flashlightState.m_flShadowSlopeScaleDepthBias, flashlightState.m_flShadowDepthBias );

		// Render to the shadow depth texture with appropriate view
		view->UpdateShadowDepthTexture( m_DummyColorTexture, shadowDepthTexture, shadowView );

		// Associate the shadow depth texture and stencil bit with the flashlight for use during scene rendering
		shadowmgr->SetFlashlightDepthTexture( shadow.m_ShadowHandle, shadowDepthTexture, 0 );
	}

	SetViewFlashlightState( nActiveDepthShadowCount, pActiveDepthShadows );
}

	
//-----------------------------------------------------------------------------
// Re-renders all shadow textures for shadow casters that lie in the leaf list
//-----------------------------------------------------------------------------
static void SetupBonesOnBaseAnimating( C_BaseAnimating *&pBaseAnimating )
{
	pBaseAnimating->SetupBones( NULL, -1, -1, gpGlobals->curtime );
}


void CClientShadowMgr::ComputeShadowTextures( const CViewSetup &view, int leafCount, LeafIndex_t* pLeafList )
{
	VPROF_BUDGET( "CClientShadowMgr::ComputeShadowTextures", VPROF_BUDGETGROUP_SHADOW_RENDERING );

	if ( !m_RenderToTextureActive || (r_shadows.GetInt() == 0) || r_shadows_gamecontrol.GetInt() == 0 )
		return;

	m_bThreaded = false;//( r_threaded_client_shadow_manager.GetBool() && g_pThreadPool->NumIdleThreads() );

	MDLCACHE_CRITICAL_SECTION();
	// First grab all shadow textures we may want to render
	int nCount = s_VisibleShadowList.FindShadows( &view, leafCount, pLeafList );
	if ( nCount == 0 )
		return;

	// FIXME: Add heuristics based on distance, etc. to futz with
	// the shadow allocator + to select blobby shadows

	CMatRenderContextPtr pRenderContext( materials );

	PIXEVENT( pRenderContext, "Render-To-Texture Shadows" );

	// Clear to white (color unused), black alpha
	pRenderContext->ClearColor4ub( 255, 255, 255, 0 );

	// No height clip mode please.
	MaterialHeightClipMode_t oldHeightClipMode = pRenderContext->GetHeightClipMode();
	pRenderContext->SetHeightClipMode( MATERIAL_HEIGHTCLIPMODE_DISABLE );

	// No projection matrix (orthographic mode)
	// FIXME: Make it work for projective shadows?
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();
	pRenderContext->Scale( 1, -1, 1 );
	pRenderContext->Ortho( 0, 0, 1, 1, -9999, 0 );

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();

	pRenderContext->PushRenderTargetAndViewport( m_ShadowAllocator.GetTexture() );

	if ( !IsX360() && m_bRenderTargetNeedsClear )
	{
		// don't need to clear absent depth buffer
		pRenderContext->ClearBuffers( true, false );
		m_bRenderTargetNeedsClear = false;
	}

	int nMaxShadows = r_shadowmaxrendered.GetInt();
	int nModelsRendered = 0;
	int i;

	if ( m_bThreaded && g_pThreadPool->NumIdleThreads() )
	{
		s_NPCShadowBoneSetups.RemoveAll();
		s_NonNPCShadowBoneSetups.RemoveAll();

		for (i = 0; i < nCount; ++i)
		{
			const VisibleShadowInfo_t &info = s_VisibleShadowList.GetVisibleShadow(i);
			if ( nModelsRendered < nMaxShadows )
			{
				if ( BuildSetupListForRenderToTextureShadow( info.m_hShadow, info.m_flArea ) )
				{
					++nModelsRendered;
				}
			}
		}

		ParallelProcess( "NPCShadowBoneSetups", s_NPCShadowBoneSetups.Base(), s_NPCShadowBoneSetups.Count(), &SetupBonesOnBaseAnimating );
		ParallelProcess( "NonNPCShadowBoneSetups", s_NonNPCShadowBoneSetups.Base(), s_NonNPCShadowBoneSetups.Count(), &SetupBonesOnBaseAnimating );

		nModelsRendered = 0;
	}

	for (i = 0; i < nCount; ++i)
	{
		const VisibleShadowInfo_t &info = s_VisibleShadowList.GetVisibleShadow(i);
		if ( nModelsRendered < nMaxShadows )
		{
			if ( DrawRenderToTextureShadow( info.m_hShadow, info.m_flArea ) )
			{
				++nModelsRendered;
			}
		}
		else
		{
			DrawRenderToTextureShadowLOD( info.m_hShadow );
		}
	}

	// Render to the backbuffer again
	pRenderContext->PopRenderTargetAndViewport();

	// Restore the matrices
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();

	pRenderContext->SetHeightClipMode( oldHeightClipMode );

	pRenderContext->SetHeightClipMode( oldHeightClipMode );

	// Restore the clear color
	pRenderContext->ClearColor3ub( 0, 0, 0 );
}

//-------------------------------------------------------------------------------------------------------
// Lock down the usage of a shadow depth texture...must be unlocked for use on subsequent views / frames
//-------------------------------------------------------------------------------------------------------
bool CClientShadowMgr::LockShadowDepthTexture( CTextureReference *shadowDepthTexture )
{
	for ( int i=0; i < m_DepthTextureCache.Count(); i++ )		// Search for cached shadow depth texture
	{
		if ( m_DepthTextureCacheLocks[i] == false )				// If a free one is found
		{
			*shadowDepthTexture = m_DepthTextureCache[i];
			m_DepthTextureCacheLocks[i] = true;
			return true;
		}
	}

	return false;												// Didn't find it...
}

//------------------------------------------------------------------
// Unlock shadow depth texture for use on subsequent views / frames
//------------------------------------------------------------------
void CClientShadowMgr::UnlockAllShadowDepthTextures()
{
	for (int i=0; i< m_DepthTextureCache.Count(); i++ )
	{
		m_DepthTextureCacheLocks[i] = false;
	}
	SetViewFlashlightState( 0, NULL );
}

void CClientShadowMgr::SetFlashlightTarget( ClientShadowHandle_t shadowHandle, EHANDLE targetEntity )
{
	Assert( m_Shadows.IsValidIndex( shadowHandle ) );

	CClientShadowMgr::ClientShadow_t &shadow = m_Shadows[ shadowHandle ];
	if( ( shadow.m_Flags & SHADOW_FLAGS_FLASHLIGHT ) == 0 )
		return;

//	shadow.m_pTargetRenderable = pRenderable;
	shadow.m_hTargetEntity = targetEntity;
}


void CClientShadowMgr::SetFlashlightLightWorld( ClientShadowHandle_t shadowHandle, bool bLightWorld )
{
	Assert( m_Shadows.IsValidIndex( shadowHandle ) );

	ClientShadow_t &shadow = m_Shadows[ shadowHandle ];
	if( ( shadow.m_Flags & SHADOW_FLAGS_FLASHLIGHT ) == 0 )
		return;

	if ( bLightWorld )
	{
		shadow.m_Flags |= SHADOW_FLAGS_LIGHT_WORLD;
	}
	else
	{
		shadow.m_Flags &= ~SHADOW_FLAGS_LIGHT_WORLD;
	}
}


bool CClientShadowMgr::IsFlashlightTarget( ClientShadowHandle_t shadowHandle, IClientRenderable *pRenderable )
{
	ClientShadow_t &shadow = m_Shadows[ shadowHandle ];

	if( shadow.m_hTargetEntity->GetClientRenderable() == pRenderable )
		return true;

	C_BaseEntity *pChild = shadow.m_hTargetEntity->FirstMoveChild();
	while( pChild )
	{
		if( pChild->GetClientRenderable()==pRenderable )
			return true;

		pChild = pChild->NextMovePeer();
	}
							
	return false;
}

//-----------------------------------------------------------------------------
// A material proxy that resets the base texture to use the rendered shadow
//-----------------------------------------------------------------------------
class CShadowProxy : public IMaterialProxy
{
public:
	CShadowProxy();
	virtual ~CShadowProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pProxyData );
	virtual void Release( void ) { delete this; }
	virtual IMaterial *GetMaterial();

private:
	IMaterialVar* m_BaseTextureVar;
};

CShadowProxy::CShadowProxy()
{
	m_BaseTextureVar = NULL;
}

CShadowProxy::~CShadowProxy()
{
}


bool CShadowProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	bool foundVar;
	m_BaseTextureVar = pMaterial->FindVar( "$basetexture", &foundVar, false );
	return foundVar;
}

void CShadowProxy::OnBind( void *pProxyData )
{
	unsigned short clientShadowHandle = ( unsigned short )(int)pProxyData&0xffff;
	ITexture* pTex = s_ClientShadowMgr.GetShadowTexture( clientShadowHandle );
	m_BaseTextureVar->SetTextureValue( pTex );
	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

IMaterial *CShadowProxy::GetMaterial()
{
	return m_BaseTextureVar->GetOwningMaterial();
}

EXPOSE_INTERFACE( CShadowProxy, IMaterialProxy, "Shadow" IMATERIAL_PROXY_INTERFACE_VERSION );



//-----------------------------------------------------------------------------
// A material proxy that resets the base texture to use the rendered shadow
//-----------------------------------------------------------------------------
class CShadowModelProxy : public IMaterialProxy
{
public:
	CShadowModelProxy();
	virtual ~CShadowModelProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pProxyData );
	virtual void Release( void ) { delete this; }
	virtual IMaterial *GetMaterial();

private:
	IMaterialVar* m_BaseTextureVar;
	IMaterialVar* m_BaseTextureOffsetVar;
	IMaterialVar* m_BaseTextureScaleVar;
	IMaterialVar* m_BaseTextureMatrixVar;
	IMaterialVar* m_FalloffOffsetVar;
	IMaterialVar* m_FalloffDistanceVar;
	IMaterialVar* m_FalloffAmountVar;
};

CShadowModelProxy::CShadowModelProxy()
{
	m_BaseTextureVar = NULL;
	m_BaseTextureOffsetVar = NULL;
	m_BaseTextureScaleVar = NULL;
	m_BaseTextureMatrixVar = NULL;
	m_FalloffOffsetVar = NULL;
	m_FalloffDistanceVar = NULL;
	m_FalloffAmountVar = NULL;
}

CShadowModelProxy::~CShadowModelProxy()
{
}


bool CShadowModelProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	bool foundVar;
	m_BaseTextureVar = pMaterial->FindVar( "$basetexture", &foundVar, false );
	if (!foundVar)
		return false;
	m_BaseTextureOffsetVar = pMaterial->FindVar( "$basetextureoffset", &foundVar, false );
	if (!foundVar)
		return false;
	m_BaseTextureScaleVar = pMaterial->FindVar( "$basetexturescale", &foundVar, false );
	if (!foundVar)
		return false;
	m_BaseTextureMatrixVar = pMaterial->FindVar( "$basetexturetransform", &foundVar, false );
	if (!foundVar)
		return false;
	m_FalloffOffsetVar = pMaterial->FindVar( "$falloffoffset", &foundVar, false );
	if (!foundVar)
		return false;
	m_FalloffDistanceVar = pMaterial->FindVar( "$falloffdistance", &foundVar, false );
	if (!foundVar)
		return false;
	m_FalloffAmountVar = pMaterial->FindVar( "$falloffamount", &foundVar, false );
	return foundVar;
}

void CShadowModelProxy::OnBind( void *pProxyData )
{
	unsigned short clientShadowHandle = ( unsigned short )((int)pProxyData&0xffff);
	ITexture* pTex = s_ClientShadowMgr.GetShadowTexture( clientShadowHandle );
	m_BaseTextureVar->SetTextureValue( pTex );

	const ShadowInfo_t& info = s_ClientShadowMgr.GetShadowInfo( clientShadowHandle );
	m_BaseTextureMatrixVar->SetMatrixValue( info.m_WorldToShadow );
	m_BaseTextureOffsetVar->SetVecValue( info.m_TexOrigin.Base(), 2 );
	m_BaseTextureScaleVar->SetVecValue( info.m_TexSize.Base(), 2 );
	m_FalloffOffsetVar->SetFloatValue( info.m_FalloffOffset );
	m_FalloffDistanceVar->SetFloatValue( info.m_MaxDist );
	m_FalloffAmountVar->SetFloatValue( info.m_FalloffAmount );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

IMaterial *CShadowModelProxy::GetMaterial()
{
	return m_BaseTextureVar->GetOwningMaterial();
}

EXPOSE_INTERFACE( CShadowModelProxy, IMaterialProxy, "ShadowModel" IMATERIAL_PROXY_INTERFACE_VERSION );
