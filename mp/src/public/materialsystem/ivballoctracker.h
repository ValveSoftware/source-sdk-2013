//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: tracks VB allocations (and compressed/uncompressed vertex memory usage)
//
//===========================================================================//

#ifndef IVBALLOCTRACKER_H
#define IVBALLOCTRACKER_H

#include "materialsystem/imaterialsystem.h"

// By default, only enable this alloc tracking for a debug shaderapidx*.dll
// (it uses about 0.25MB to track ~7000 allocations)
#if defined(_DEBUG)
#define ENABLE_VB_ALLOC_TRACKER 1
#else
#define ENABLE_VB_ALLOC_TRACKER 0
#endif

// This interface is actually exported by the shader API DLL.
#define VB_ALLOC_TRACKER_INTERFACE_VERSION "VBAllocTracker001"

// Interface to the VB mem alloc tracker
abstract_class IVBAllocTracker
{
public:
	// This should be called wherever VertexBuffers are allocated
	virtual void CountVB(   void * buffer, bool isDynamic, int bufferSize, int vertexSize, VertexFormat_t fmt ) = 0;
	// This should be called wherever VertexBuffers are freed
	virtual void UnCountVB( void * buffer ) = 0;
	// Track mesh allocations (set this before an allocation, clear it after)
	virtual bool TrackMeshAllocations( const char * allocatorName ) = 0;
};

#endif // IVBALLOCTRACKER_H
