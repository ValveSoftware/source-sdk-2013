//========= Copyright Valve Corporation, All rights reserved. ================================== //
//
// Purpose: Defines a texture compositor infterface which uses simple operations and shaders to 
// create complex procedural textures. 
//
//============================================================================================== //

#ifndef ITEXTURECOMPOSITOR_H
#define ITEXTURECOMPOSITOR_H
#pragma once

#include "interface.h"
#include "itexture.h"

#define ITEXTURE_COMPOSITOR_INTERFACE_VERSION "_ITextureCompositor000"

enum ECompositeResolveStatus
{
	ECRS_Idle,
	ECRS_Scheduled,
	ECRS_PendingTextureLoads,
	ECRS_PendingComposites,
	ECRS_Error,
	ECRS_Complete
};

enum TextureCompositeCreateFlags_t
{
	TEX_COMPOSITE_CREATE_FLAGS_FORCE			= 0x00000001,
	TEX_COMPOSITE_CREATE_FLAGS_NO_COMPRESSION	= 0x00000002,
	TEX_COMPOSITE_CREATE_FLAGS_NO_MIPMAPS		= 0x00000004,
};

abstract_class ITextureCompositor
{
public:
	virtual int AddRef() = 0;
	virtual int Release() = 0;
	virtual int GetRefCount() const = 0;

	virtual void Update() = 0;
	virtual ITexture* GetResultTexture() const = 0;
	virtual ECompositeResolveStatus GetResolveStatus() const = 0;
	virtual void ScheduleResolve() = 0;
protected:
	virtual ~ITextureCompositor() {}
};


#endif /* ITEXTURECOMPOSITOR_H */
