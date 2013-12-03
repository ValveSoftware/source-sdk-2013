//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef MDLLIB_H
#define MDLLIB_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlbuffer.h"
#include "appframework/IAppSystem.h"

//
// Forward interface declarations
//
abstract_class IMdlLib;

abstract_class IMdlStripInfo;


//-----------------------------------------------------------------------------
// Purpose: Interface to accessing model data operations
//-----------------------------------------------------------------------------
#define MDLLIB_INTERFACE_VERSION		"VMDLLIB001"

abstract_class IMdlLib : public IAppSystem
{
	//
	// Stripping routines
	//
public:

	//
	// StripModelBuffers
	//	The main function that strips the model buffers
	//		mdlBuffer			- mdl buffer, updated, no size change
	//		vvdBuffer			- vvd buffer, updated, size reduced
	//		vtxBuffer			- vtx buffer, updated, size reduced
	//		ppStripInfo			- if nonzero on return will be filled with the stripping info
	//
	virtual bool StripModelBuffers( CUtlBuffer &mdlBuffer, CUtlBuffer &vvdBuffer, CUtlBuffer &vtxBuffer, IMdlStripInfo **ppStripInfo ) = 0;

	//
	// CreateNewStripInfo
	//	Creates an empty strip info or resets an existing strip info so that it can be reused.
	//
	virtual bool CreateNewStripInfo( IMdlStripInfo **ppStripInfo ) = 0;

};


abstract_class IMdlStripInfo
{
	//
	// Serialization
	//
public:
	// Save the strip info to the buffer (appends to the end)
	virtual bool Serialize( CUtlBuffer &bufStorage ) const = 0;

	// Load the strip info from the buffer (reads from the current position as much as needed)
	virtual bool UnSerialize( CUtlBuffer &bufData ) = 0;

	//
	// Stripping info state
	//
public:
	// Returns the checksums that the stripping info was generated for:
	//	plChecksumOriginal		if non-NULL will hold the checksum of the original model submitted for stripping
	//	plChecksumStripped		if non-NULL will hold the resulting checksum of the stripped model
	virtual bool GetCheckSum( long *plChecksumOriginal, long *plChecksumStripped ) const = 0;

	//
	// Stripping
	//
public:
	
	//
	// StripHardwareVertsBuffer
	//	The main function that strips the vhv buffer
	//		vhvBuffer		- vhv buffer, updated, size reduced
	//
	virtual bool StripHardwareVertsBuffer( CUtlBuffer &vhvBuffer ) = 0;

	//
	// StripModelBuffer
	//	The main function that strips the mdl buffer
	//		mdlBuffer		- mdl buffer, updated
	//
	virtual bool StripModelBuffer( CUtlBuffer &mdlBuffer ) = 0;
	
	//
	// StripVertexDataBuffer
	//	The main function that strips the vvd buffer
	//		vvdBuffer		- vvd buffer, updated, size reduced
	//
	virtual bool StripVertexDataBuffer( CUtlBuffer &vvdBuffer ) = 0;
	
	//
	// StripOptimizedModelBuffer
	//	The main function that strips the vtx buffer
	//		vtxBuffer		- vtx buffer, updated, size reduced
	//
	virtual bool StripOptimizedModelBuffer( CUtlBuffer &vtxBuffer ) = 0;

	//
	// Release the object with "delete this"
	//
public:
	virtual void DeleteThis() = 0;
};



#endif // MDLLIB_H
