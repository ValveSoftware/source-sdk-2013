//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//
// This file contains code to allow us to associate client data with bsp leaves.
//
//=============================================================================//

#if !defined( ICLIENTLEAFSYSTEM_H )
#define ICLIENTLEAFSYSTEM_H
#ifdef _WIN32
#pragma once
#endif


#include "client_render_handle.h"


//-----------------------------------------------------------------------------
// Render groups
//-----------------------------------------------------------------------------
enum RenderGroup_Config_t
{
	// Number of buckets that are used to hold opaque entities
	// and opaque static props by size. The bucketing should be used to reduce overdraw.
	RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS	= 4,
};

enum RenderGroup_t
{
	RENDER_GROUP_OPAQUE_STATIC_HUGE			= 0,		// Huge static prop
	RENDER_GROUP_OPAQUE_ENTITY_HUGE			= 1,		// Huge opaque entity
	RENDER_GROUP_OPAQUE_STATIC = RENDER_GROUP_OPAQUE_STATIC_HUGE + ( RENDER_GROUP_CFG_NUM_OPAQUE_ENT_BUCKETS - 1 ) * 2,
	RENDER_GROUP_OPAQUE_ENTITY,					// Opaque entity (smallest size, or default)

	RENDER_GROUP_TRANSLUCENT_ENTITY,
	RENDER_GROUP_TWOPASS,						// Implied opaque and translucent in two passes
	RENDER_GROUP_VIEW_MODEL_OPAQUE,				// Solid weapon view models
	RENDER_GROUP_VIEW_MODEL_TRANSLUCENT,		// Transparent overlays etc

	RENDER_GROUP_OPAQUE_BRUSH,					// Brushes

	RENDER_GROUP_OTHER,							// Unclassfied. Won't get drawn.

	// This one's always gotta be last
	RENDER_GROUP_COUNT
};

#define CLIENTLEAFSYSTEM_INTERFACE_VERSION_1 "ClientLeafSystem001"
#define CLIENTLEAFSYSTEM_INTERFACE_VERSION	"ClientLeafSystem002"


//-----------------------------------------------------------------------------
// The client leaf system
//-----------------------------------------------------------------------------
abstract_class IClientLeafSystemEngine
{
public:
	// Adds and removes renderables from the leaf lists
	// CreateRenderableHandle stores the handle inside pRenderable.
	virtual void CreateRenderableHandle( IClientRenderable* pRenderable, bool bIsStaticProp = false ) = 0;
	virtual void RemoveRenderable( ClientRenderHandle_t handle ) = 0;
	virtual void AddRenderableToLeaves( ClientRenderHandle_t renderable, int nLeafCount, unsigned short *pLeaves ) = 0;
	virtual void ChangeRenderableRenderGroup( ClientRenderHandle_t handle, RenderGroup_t group ) = 0;
};


#endif	// ICLIENTLEAFSYSTEM_H


