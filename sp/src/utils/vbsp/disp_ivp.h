//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef DISP_IVP_H
#define DISP_IVP_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "../../public/disp_tesselate.h"


class CPhysCollisionEntry;
struct dmodel_t;


// This provides the template functions that the engine's tesselation code needs
// so we can share the code in VBSP.
class CVBSPTesselateHelper : public CBaseTesselateHelper
{
public:
	void EndTriangle()
	{
		m_pIndices->AddToTail( m_TempIndices[0] );
		m_pIndices->AddToTail( m_TempIndices[1] );
		m_pIndices->AddToTail( m_TempIndices[2] );
	}

	DispNodeInfo_t& GetNodeInfo( int iNodeBit )
	{
		// VBSP doesn't care about these. Give it back something to play with.
		static DispNodeInfo_t dummy;
		return dummy;
	}
	
public:
	CUtlVector<unsigned short> *m_pIndices;
};


extern void Disp_AddCollisionModels( CUtlVector<CPhysCollisionEntry *> &collisionList, dmodel_t *pModel, int contentsMask );
extern void Disp_BuildVirtualMesh( int contentsMask );
extern bool Disp_HasPower4Displacements();

#endif // DISP_IVP_H
