//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The transform operator class - shortcut to setting transform values from floats
//
//=============================================================================

#ifndef DMETRANSFORMOPERATOR_H
#define DMETRANSFORMOPERATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "movieobjects/dmeoperator.h"
#include "datamodel/dmelement.h"
#include "datamodel/dmattribute.h"
#include "datamodel/dmattributevar.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CDmeTransform;


//-----------------------------------------------------------------------------
// A class representing a camera
//-----------------------------------------------------------------------------
class CDmeTransformOperator : public CDmeOperator
{
	DEFINE_ELEMENT( CDmeTransformOperator, CDmeOperator );

public:
	virtual void Operate();

	virtual void GetInputAttributes ( CUtlVector< CDmAttribute * > &attrs );
	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

	void SetTransform( CDmeTransform *pTransform );
	const CDmeTransform *GetTransform() const;

protected:
	CDmaElement< CDmeTransform > m_transform;
	CDmaVar< float > m_positionX;
	CDmaVar< float > m_positionY;
	CDmaVar< float > m_positionZ;
	CDmaVar< float > m_orientationX;
	CDmaVar< float > m_orientationY;
	CDmaVar< float > m_orientationZ;
	CDmaVar< float > m_orientationW;
};


#endif // DMETRANSFORMOPERATOR_H
