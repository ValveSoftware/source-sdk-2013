//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A set of utilities to help with generating meshes
//
//===========================================================================//

#ifndef MESHUTILS_H
#define MESHUTILS_H

#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// Helper methods to create various standard index buffer types
//-----------------------------------------------------------------------------
void GenerateSequentialIndexBuffer( unsigned short* pIndexMemory, int nIndexCount, int nFirstVertex );
void GenerateQuadIndexBuffer( unsigned short* pIndexMemory, int nIndexCount, int nFirstVertex );
void GeneratePolygonIndexBuffer( unsigned short* pIndexMemory, int nIndexCount, int nFirstVertex );
void GenerateLineStripIndexBuffer( unsigned short* pIndexMemory, int nIndexCount, int nFirstVertex );
void GenerateLineLoopIndexBuffer( unsigned short* pIndexMemory, int nIndexCount, int nFirstVertex );


#endif // MESHUTILS_H

