//========= Copyright Valve Corporation, All rights reserved. ============//
//
// A class representing an abstract shape (ie drawable object)
//
//=============================================================================

#ifndef DMESHAPE_H
#define DMESHAPE_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"
#include "datamodel/dmelement.h"
#include "datamodel/dmattribute.h"
#include "datamodel/dmattributevar.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CDmeTransform;
class CDmeDrawSettings;
class CDmeDag;


//-----------------------------------------------------------------------------
// A class representing a camera
//-----------------------------------------------------------------------------
class CDmeShape : public CDmElement
{
	DEFINE_ELEMENT( CDmeShape, CDmElement );

public:
	virtual void Draw( const matrix3x4_t &shapeToWorld, CDmeDrawSettings *pDmeDrawSettings = NULL );

	virtual void GetBoundingSphere( Vector &c, float &r ) const;

	// Find out how many DmeDag's have this DmeShape as their shape, could be 0
	int GetParentCount() const;

	// Get the nth DmeDag that has this DmeShape as its shape.  The order is defined by g_pDataModel->FirstAttributeReferencingElement/NextAttr...
	CDmeDag *GetParent( int nParentIndex = 0 ) const;

	// Get the Nth World Matrix for the shape (the world matrix of the Nth DmeDag parent)
	void GetShapeToWorldTransform( matrix3x4_t &mat, int nParentIndex = 0 ) const;

protected:
	CDmaVar< bool > m_visible;
};


#endif // DMESHAPE_H
