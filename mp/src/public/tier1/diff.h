//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// Serialization/unserialization buffer
//=============================================================================//

#ifndef DIFF_H
#define DIFF_H
#pragma once

int FindDiffs(uint8 const *NewBlock, uint8 const *OldBlock,
			  int NewSize, int OldSize, int &DiffListSize,uint8 *Output,uint32 OutSize);

int FindDiffsForLargeFiles(uint8 const *NewBlock, uint8 const *OldBlock,
						   int NewSize, int OldSize, int &DiffListSize,uint8 *Output,
						   uint32 OutSize,
						   int hashsize=65536);

void ApplyDiffs(uint8 const *OldBlock, uint8 const *DiffList,
                int OldSize, int DiffListSize, int &ResultListSize,uint8 *Output,uint32 OutSize);

int FindDiffsLowMemory(uint8 const *NewBlock, uint8 const *OldBlock,
					   int NewSize, int OldSize, int &DiffListSize,uint8 *Output,uint32 OutSize);

#endif

