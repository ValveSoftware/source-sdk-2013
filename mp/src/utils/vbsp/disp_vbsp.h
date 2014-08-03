//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VBSP_DISPINFO_H
#define VBSP_DISPINFO_H
#ifdef _WIN32
#pragma once
#endif


#include "vbsp.h"



class CCoreDispInfo;


extern CUtlVector<CCoreDispInfo*> g_CoreDispInfos;


// Setup initial entries in g_dispinfo with some of the vertex data from the mapdisps.
void EmitInitialDispInfos();

// Resample vertex alpha into lightmap alpha for displacement surfaces so LOD popping artifacts are 
// less noticeable on the mid-to-high end.
//
// Also builds neighbor data.
void EmitDispLMAlphaAndNeighbors();

// Setup a CCoreDispInfo given a mapdispinfo_t.
// If pFace is non-NULL, then lightmap texture coordinates will be generated.
void DispMapToCoreDispInfo( mapdispinfo_t *pMapDisp,
	CCoreDispInfo *pCoreDispInfo, dface_t *pFace, int *pSwappedTexInfos );


void DispGetFaceInfo( mapbrush_t *pBrush );
bool HasDispInfo( mapbrush_t *pBrush );

// Computes the bounds for a disp info
void ComputeDispInfoBounds( int dispinfo, Vector& mins, Vector& maxs );

#endif // VBSP_DISPINFO_H
