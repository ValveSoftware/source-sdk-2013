//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef ICLIENTSHADOWMGR_H
#define ICLIENTSHADOWMGR_H

#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "icliententityinternal.h"
#include "engine/ishadowmgr.h"
#include "ivrenderview.h"
#include "toolframework/itoolentity.h"

//-----------------------------------------------------------------------------
// Forward decls
//-----------------------------------------------------------------------------
struct FlashlightState_t;


//-----------------------------------------------------------------------------
// Handles to a client shadow
//-----------------------------------------------------------------------------
enum ShadowReceiver_t
{
	SHADOW_RECEIVER_BRUSH_MODEL = 0,
	SHADOW_RECEIVER_STATIC_PROP,
	SHADOW_RECEIVER_STUDIO_MODEL,
};


//-----------------------------------------------------------------------------
// The class responsible for dealing with shadows on the client side
//-----------------------------------------------------------------------------
abstract_class IClientShadowMgr : public IGameSystemPerFrame
{
public:
	// Create, destroy shadows
	virtual ClientShadowHandle_t CreateShadow( ClientEntityHandle_t entity, int flags ) = 0;
	virtual void DestroyShadow( ClientShadowHandle_t handle ) = 0;

	// Create flashlight.
	// FLASHLIGHTFIXME: need to rename all of the shadow stuff to projectedtexture and have flashlights and shadows as instances.
	virtual ClientShadowHandle_t CreateFlashlight( const FlashlightState_t &lightState ) = 0;
	virtual void UpdateFlashlightState( ClientShadowHandle_t shadowHandle, const FlashlightState_t &lightState ) = 0;
	virtual void DestroyFlashlight( ClientShadowHandle_t handle ) = 0;
	
	// Indicate that the shadow should be recomputed due to a change in
	// the client entity
	virtual void UpdateProjectedTexture( ClientShadowHandle_t handle, bool force = false ) = 0;

	// Used to cause shadows to be re-projected against the world.
	virtual void AddToDirtyShadowList( ClientShadowHandle_t handle, bool force = false ) = 0;
	virtual void AddToDirtyShadowList( IClientRenderable *pRenderable, bool force = false ) = 0;

	// deals with shadows being added to shadow receivers
	virtual void AddShadowToReceiver( ClientShadowHandle_t handle,
		IClientRenderable* pRenderable, ShadowReceiver_t type ) = 0;

	virtual void RemoveAllShadowsFromReceiver( 
		IClientRenderable* pRenderable, ShadowReceiver_t type ) = 0;

	// Re-renders all shadow textures for shadow casters that lie in the leaf list
	virtual void ComputeShadowTextures( const CViewSetup &view, int leafCount, LeafIndex_t* pLeafList ) = 0;

	// Frees shadow depth textures for use in subsequent view/frame
	virtual void UnlockAllShadowDepthTextures() = 0;
	
	// Renders the shadow texture to screen...
	virtual void RenderShadowTexture( int w, int h ) = 0;

	// Sets the shadow direction + color
	virtual void SetShadowDirection( const Vector& dir ) = 0;
	virtual const Vector &GetShadowDirection() const = 0;
	
	virtual void SetShadowColor( unsigned char r, unsigned char g, unsigned char b ) = 0;
	virtual void SetShadowDistance( float flMaxDistance ) = 0;
	virtual void SetShadowBlobbyCutoffArea( float flMinArea ) = 0;
	virtual void SetFalloffBias( ClientShadowHandle_t handle, unsigned char ucBias ) = 0;

	// Marks the render-to-texture shadow as needing to be re-rendered
	virtual void MarkRenderToTextureShadowDirty( ClientShadowHandle_t handle ) = 0;

	// Advance the frame
	virtual void AdvanceFrame() = 0;

	// Set and clear flashlight target renderable
	virtual void SetFlashlightTarget( ClientShadowHandle_t shadowHandle, EHANDLE targetEntity ) = 0;

	// Set flashlight light world flag
	virtual void SetFlashlightLightWorld( ClientShadowHandle_t shadowHandle, bool bLightWorld ) = 0;

	virtual void SetShadowsDisabled( bool bDisabled ) = 0;

	virtual void ComputeShadowDepthTextures( const CViewSetup &pView ) = 0;

};


//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
extern IClientShadowMgr* g_pClientShadowMgr;

#endif // ICLIENTSHADOWMGR_H
