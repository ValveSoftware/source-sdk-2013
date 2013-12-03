//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//-----------------------------------------------------------------------------
// FILE: TRISTRIP.H
//
// Desc: tristrip header file
//
// Copyright (c) 1999-2000 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

typedef unsigned short WORD;

//
// Main Stripify routine. Returns number of strip indices contained
// in ppstripindices. Caller must delete [] ppstripindices.
//
int Stripify(
    int numtris,                    // Number of triangles
    WORD *ptriangles,               // triangle indices pointer
    int *pnumindices,               // number of indices in ppstripindices (out)
    WORD **ppstripindices           // triangle strip indices
);

//
// Re-arrange vertices so that they occur in the order that they are first 
// used. This function doesn't actually move vertex data around, it returns
// an array that specifies where in the new vertex array each old vertex
// should go. It also re-maps the strip indices to use the new vertex 
// locations. Caller must delete [] pVertexPermutation.
//
void ComputeVertexPermutation
(
    int numstripindices,            // Number of strip indices
    WORD *pstripindices,            // Strip indices
    int *pnumverts,                 // Number of verts (in and out)
    WORD **ppvertexpermutation      // Map from orignal index to remapped index
);

