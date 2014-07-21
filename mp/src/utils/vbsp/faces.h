//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef FACES_H
#define FACES_H
#ifdef _WIN32
#pragma once
#endif


void GetEdge2_InitOptimizedList();	// Call this before calling GetEdge2() on a bunch of edges.
int AddEdge( int v1, int v2, face_t *f );
int GetEdge2(int v1, int v2,  face_t *f);


#endif // FACES_H
