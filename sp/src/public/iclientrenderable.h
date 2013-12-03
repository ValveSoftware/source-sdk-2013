//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef ICLIENTRENDERABLE_H
#define ICLIENTRENDERABLE_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"
#include "interface.h"
#include "iclientunknown.h"
#include "client_render_handle.h"
#include "engine/ivmodelrender.h"

struct model_t;
struct matrix3x4_t;

extern void DefaultRenderBoundsWorldspace( IClientRenderable *pRenderable, Vector &absMins, Vector &absMaxs );

//-----------------------------------------------------------------------------
// Handles to a client shadow
//-----------------------------------------------------------------------------
typedef unsigned short ClientShadowHandle_t;

enum
{
	CLIENTSHADOW_INVALID_HANDLE = (ClientShadowHandle_t)~0
};

//-----------------------------------------------------------------------------
// What kind of shadows to render?
//-----------------------------------------------------------------------------
enum ShadowType_t
{
	SHADOWS_NONE = 0,
	SHADOWS_SIMPLE,
	SHADOWS_RENDER_TO_TEXTURE,
	SHADOWS_RENDER_TO_TEXTURE_DYNAMIC,	// the shadow is always changing state
	SHADOWS_RENDER_TO_DEPTH_TEXTURE,
};


// This provides a way for entities to know when they've entered or left the PVS.
// Normally, server entities can use NotifyShouldTransmit to get this info, but client-only
// entities can use this. Store a CPVSNotifyInfo in your 
//
// When bInPVS=true, it's being called DURING rendering. It might be after rendering any
// number of views.
//
// If no views had the entity, then it is called with bInPVS=false after rendering.
abstract_class IPVSNotify
{
public:
	virtual void OnPVSStatusChanged( bool bInPVS ) = 0;
};


//-----------------------------------------------------------------------------
// Purpose: All client entities must implement this interface.
//-----------------------------------------------------------------------------
abstract_class IClientRenderable
{
public:
	// Gets at the containing class...
	virtual IClientUnknown*	GetIClientUnknown() = 0;

	// Data accessors
	virtual Vector const&			GetRenderOrigin( void ) = 0;
	virtual QAngle const&			GetRenderAngles( void ) = 0;
	virtual bool					ShouldDraw( void ) = 0;
	virtual bool					IsTransparent( void ) = 0;
	virtual bool					UsesPowerOfTwoFrameBufferTexture() = 0;
	virtual bool					UsesFullFrameBufferTexture() = 0;

	virtual ClientShadowHandle_t	GetShadowHandle() const = 0;

	// Used by the leaf system to store its render handle.
	virtual ClientRenderHandle_t&	RenderHandle() = 0;

	// Render baby!
	virtual const model_t*			GetModel( ) const = 0;
	virtual int						DrawModel( int flags ) = 0;

	// Get the body parameter
	virtual int		GetBody() = 0;

	// Determine alpha and blend amount for transparent objects based on render state info
	virtual void	ComputeFxBlend( ) = 0;
	virtual int		GetFxBlend( void ) = 0;

	// Determine the color modulation amount
	virtual void	GetColorModulation( float* color ) = 0;

	// Returns false if the entity shouldn't be drawn due to LOD. 
	// (NOTE: This is no longer used/supported, but kept in the vtable for backwards compat)
	virtual bool	LODTest() = 0;

	// Call this to get the current bone transforms for the model.
	// currentTime parameter will affect interpolation
	// nMaxBones specifies how many matrices pBoneToWorldOut can hold. (Should be greater than or
	// equal to studiohdr_t::numbones. Use MAXSTUDIOBONES to be safe.)
	virtual bool	SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime ) = 0;

	virtual void	SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights ) = 0;
	virtual void	DoAnimationEvents( void ) = 0;
	
	// Return this if you want PVS notifications. See IPVSNotify for more info.	
	// Note: you must always return the same value from this function. If you don't,
	// undefined things will occur, and they won't be good.
	virtual IPVSNotify* GetPVSNotifyInterface() = 0;

	// Returns the bounds relative to the origin (render bounds)
	virtual void	GetRenderBounds( Vector& mins, Vector& maxs ) = 0;
	
	// returns the bounds as an AABB in worldspace
	virtual void	GetRenderBoundsWorldspace( Vector& mins, Vector& maxs ) = 0;

	// These normally call through to GetRenderAngles/GetRenderBounds, but some entities custom implement them.
	virtual void	GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType ) = 0;

	// Should this object be able to have shadows cast onto it?
	virtual bool	ShouldReceiveProjectedTextures( int flags ) = 0;

	// These methods return true if we want a per-renderable shadow cast direction + distance
	virtual bool	GetShadowCastDistance( float *pDist, ShadowType_t shadowType ) const = 0;
	virtual bool	GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const = 0;

	// Other methods related to shadow rendering
	virtual bool	IsShadowDirty( ) = 0;
	virtual void	MarkShadowDirty( bool bDirty ) = 0;

	// Iteration over shadow hierarchy
	virtual IClientRenderable *GetShadowParent() = 0;
	virtual IClientRenderable *FirstShadowChild() = 0;
	virtual IClientRenderable *NextShadowPeer() = 0;

	// Returns the shadow cast type
	virtual ShadowType_t ShadowCastType() = 0;

	// Create/get/destroy model instance
	virtual void CreateModelInstance() = 0;
	virtual ModelInstanceHandle_t GetModelInstance() = 0;

	// Returns the transform from RenderOrigin/RenderAngles to world
	virtual const matrix3x4_t &RenderableToWorldTransform() = 0;

	// Attachments
	virtual int LookupAttachment( const char *pAttachmentName ) = 0;
	virtual	bool GetAttachment( int number, Vector &origin, QAngle &angles ) = 0;
	virtual bool GetAttachment( int number, matrix3x4_t &matrix ) = 0;

	// Rendering clip plane, should be 4 floats, return value of NULL indicates a disabled render clip plane
	virtual float *GetRenderClipPlane( void ) = 0;

	// Get the skin parameter
	virtual int		GetSkin() = 0;

	// Is this a two-pass renderable?
	virtual bool	IsTwoPass( void ) = 0;

	virtual void	OnThreadedDrawSetup() = 0;

	virtual bool	UsesFlexDelayedWeights() = 0;

	virtual void	RecordToolMessage() = 0;

	virtual bool	IgnoresZBuffer( void ) const = 0;
};


// This class can be used to implement default versions of some of the 
// functions of IClientRenderable.
abstract_class CDefaultClientRenderable : public IClientUnknown, public IClientRenderable
{
public:
	CDefaultClientRenderable()
	{
		m_hRenderHandle = INVALID_CLIENT_RENDER_HANDLE;
	}

	virtual const Vector &			GetRenderOrigin( void ) = 0;
	virtual const QAngle &			GetRenderAngles( void ) = 0;
	virtual const matrix3x4_t &		RenderableToWorldTransform() = 0;
	virtual bool					ShouldDraw( void ) = 0;
	virtual bool					IsTransparent( void ) = 0;
	virtual bool					IsTwoPass( void ) { return false; }
	virtual void					OnThreadedDrawSetup() {}
	virtual bool					UsesPowerOfTwoFrameBufferTexture( void ) { return false; }
	virtual bool					UsesFullFrameBufferTexture( void ) { return false; }

	virtual ClientShadowHandle_t	GetShadowHandle() const
	{
		return CLIENTSHADOW_INVALID_HANDLE;
	}

	virtual ClientRenderHandle_t&	RenderHandle()
	{
		return m_hRenderHandle;
	}

	virtual int						GetBody() { return 0; }
	virtual int						GetSkin() { return 0; }
	virtual bool					UsesFlexDelayedWeights() { return false; }

	virtual const model_t*			GetModel( ) const		{ return NULL; }
	virtual int						DrawModel( int flags )	{ return 0; }
	virtual void					ComputeFxBlend( )		{ return; }
	virtual int						GetFxBlend( )			{ return 255; }
	virtual bool					LODTest()				{ return true; }
	virtual bool					SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime )	{ return true; }
	virtual void					SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights ) {}
	virtual void					DoAnimationEvents( void )						{}
	virtual IPVSNotify*				GetPVSNotifyInterface() { return NULL; }
	virtual void					GetRenderBoundsWorldspace( Vector& absMins, Vector& absMaxs ) { DefaultRenderBoundsWorldspace( this, absMins, absMaxs ); }

	// Determine the color modulation amount
	virtual void	GetColorModulation( float* color )
	{
		Assert(color);
		color[0] = color[1] = color[2] = 1.0f;
	}

	// Should this object be able to have shadows cast onto it?
	virtual bool	ShouldReceiveProjectedTextures( int flags ) 
	{
		return false;
	}

	// These methods return true if we want a per-renderable shadow cast direction + distance
	virtual bool	GetShadowCastDistance( float *pDist, ShadowType_t shadowType ) const			{ return false; }
	virtual bool	GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const	{ return false; }

	virtual void	GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType )
	{
		GetRenderBounds( mins, maxs );
	}

	virtual bool IsShadowDirty( )			     { return false; }
	virtual void MarkShadowDirty( bool bDirty )  {}
	virtual IClientRenderable *GetShadowParent() { return NULL; }
	virtual IClientRenderable *FirstShadowChild(){ return NULL; }
	virtual IClientRenderable *NextShadowPeer()  { return NULL; }
	virtual ShadowType_t ShadowCastType()		 { return SHADOWS_NONE; }
	virtual void CreateModelInstance()			 {}
	virtual ModelInstanceHandle_t GetModelInstance() { return MODEL_INSTANCE_INVALID; }

	// Attachments
	virtual int LookupAttachment( const char *pAttachmentName ) { return -1; }
	virtual	bool GetAttachment( int number, Vector &origin, QAngle &angles ) { return false; }
	virtual bool GetAttachment( int number, matrix3x4_t &matrix ) {	return false; }

	// Rendering clip plane, should be 4 floats, return value of NULL indicates a disabled render clip plane
	virtual float *GetRenderClipPlane() { return NULL; }

	virtual void RecordToolMessage() {}
	virtual bool IgnoresZBuffer( void ) const { return false; }

// IClientUnknown implementation.
public:
	virtual void SetRefEHandle( const CBaseHandle &handle )	{ Assert( false ); }
	virtual const CBaseHandle& GetRefEHandle() const		{ Assert( false ); return *((CBaseHandle*)0); }

	virtual IClientUnknown*		GetIClientUnknown()		{ return this; }
	virtual ICollideable*		GetCollideable()		{ return 0; }
	virtual IClientRenderable*	GetClientRenderable()	{ return this; }
	virtual IClientNetworkable*	GetClientNetworkable()	{ return 0; }
	virtual IClientEntity*		GetIClientEntity()		{ return 0; }
	virtual C_BaseEntity*		GetBaseEntity()			{ return 0; }
	virtual IClientThinkable*	GetClientThinkable()	{ return 0; }


public:
	ClientRenderHandle_t m_hRenderHandle;
};


#endif // ICLIENTRENDERABLE_H
