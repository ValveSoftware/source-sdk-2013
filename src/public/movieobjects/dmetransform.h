//========= Copyright Valve Corporation, All rights reserved. ============//
//
// A class representing a transform
//
//=============================================================================

#ifndef DMETRANSFORM_H
#define DMETRANSFORM_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
struct matrix3x4_t;


//-----------------------------------------------------------------------------
// A class representing a transformation matrix
//-----------------------------------------------------------------------------
class CDmeTransform : public CDmElement
{
	DEFINE_ELEMENT( CDmeTransform, CDmElement );

public:
	// FIXME: Replace this with actual methods to do editing
	// (scale/shear, etc.)
	void SetTransform( const matrix3x4_t &transform );
	void GetTransform( matrix3x4_t &transform );

	const Vector &GetPosition() const;
	void SetPosition( const Vector &vecPosition );
	const Quaternion &GetOrientation() const;
	void SetOrientation( const Quaternion &orientation );

	CDmAttribute *GetPositionAttribute();
	CDmAttribute *GetOrientationAttribute();

private:
	CDmaVar<Vector> m_Position;
	CDmaVar<Quaternion> m_Orientation;
};


#endif // DMETRANSFORM_H
