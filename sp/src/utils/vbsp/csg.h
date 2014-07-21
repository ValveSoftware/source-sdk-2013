//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef CSG_H
#define CSG_H
#ifdef _WIN32
#pragma once
#endif


// Print a CONTENTS_ mask to a string.
void PrintBrushContentsToString( int contents, char *pOut, int nMaxChars );

// Print a CONTENTS_ mask with Msg().
void PrintBrushContents( int contents );

void FixupAreaportalWaterBrushes( bspbrush_t *pList );

bspbrush_t *MakeBspBrushList (int startbrush, int endbrush,
		const Vector& clipmins, const Vector& clipmaxs, int detailScreen);
bspbrush_t *MakeBspBrushList (mapbrush_t **pBrushes, int nBrushCount, const Vector& clipmins, const Vector& clipmaxs);

void WriteBrushMap (char *name, bspbrush_t *list);

bspbrush_t *ChopBrushes (bspbrush_t *head);
bspbrush_t *IntersectBrush (bspbrush_t *a, bspbrush_t *b);
qboolean BrushesDisjoint (bspbrush_t *a, bspbrush_t *b);

#endif // CSG_H
