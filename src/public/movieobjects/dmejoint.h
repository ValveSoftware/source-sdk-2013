//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Dme version of a joint of a skeletal model (gets compiled into a MDL)
//
//===========================================================================//

#ifndef DMEJOINT_H
#define DMEJOINT_H

#ifdef _WIN32
#pragma once
#endif

#include "movieobjects/dmedag.h"
#include "materialsystem/MaterialSystemUtil.h"

class CDmeDrawSettings;

//-----------------------------------------------------------------------------
// A class representing a skeletal model
//-----------------------------------------------------------------------------
class CDmeJoint : public CDmeDag
{
	DEFINE_ELEMENT( CDmeJoint, CDmeDag );

public:
	virtual void Draw( CDmeDrawSettings *pDrawSettings = NULL );

	static void DrawJointHierarchy( bool bDrawJoints );

private:
	void DrawJoints();

	CMaterialReference m_JointMaterial;
	static bool s_bDrawJoints;
};


#endif // DMEJOINT_H
