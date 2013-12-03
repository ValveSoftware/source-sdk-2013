//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VMPI_FILESYSTEM_H
#define VMPI_FILESYSTEM_H
#ifdef _WIN32
#pragma once
#endif


#include "interface.h"


class IFileSystem;
class MessageBuffer;


// Use this to read virtual files.
#define VMPI_VIRTUAL_FILES_PATH_ID "VMPI_VIRTUAL_FILES_PATH_ID"


// When you hook the file system with VMPI and are a worker, it blocks on file reads
// and uses MPI to communicate with the master to transfer files it needs over.
// 
// The filesystem, by default (and it maxFileSystemMemoryUsage is left at zero), 
// keeps the contents of the files that get opened in memory. You can pass in a 
// value here to put a cap on it, in which case it'll unload the least-recently-used
// files when it hits the limit.
IFileSystem* VMPI_FileSystem_Init( int maxFileSystemMemoryUsage, IFileSystem *pPassThru );

// On the master machine, this really should be called before the app shuts down and 
// global destructors are called. If it isn't, it might lock up waiting for a thread to exit.
//
// This returns the original filesystem you passed into VMPI_FileSystem_Init so you can uninitialize it.
IFileSystem* VMPI_FileSystem_Term();

// Causes it to error out on any Open() calls.
void VMPI_FileSystem_DisableFileAccess();

// Returns a factory that will hand out BASEFILESYSTEM_INTERFACE_VERSION when asked for it.
CreateInterfaceFn VMPI_FileSystem_GetFactory();

// This function creates a virtual file that workers can then open and read out of.
// NOTE: when reading from the file, you must use VMPI_VIRTUAL_FILES_PATH_ID as the path ID
// or else it won't find the file.
void VMPI_FileSystem_CreateVirtualFile( const char *pFilename, const void *pData, unsigned long fileLength );


#endif // VMPI_FILESYSTEM_H
