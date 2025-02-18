//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The abstract base operator class - all actions within the scenegraph happen via operators
//
//=============================================================================

#ifndef DMEMOUSEINPUT_H
#define DMEMOUSEINPUT_H
#ifdef _WIN32
#pragma once
#endif

#include "movieobjects/dmeinput.h"


//-----------------------------------------------------------------------------
// A class representing a camera
//-----------------------------------------------------------------------------
class CDmeMouseInput : public CDmeInput
{
	DEFINE_ELEMENT( CDmeMouseInput, CDmeInput );

public:
	virtual bool IsDirty(); // ie needs to operate
	virtual void Operate();

	virtual void GetInputAttributes ( CUtlVector< CDmAttribute * > &attrs );
	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

	// makes the current mouse position be the origin
	void ResetOrigin() { ResetOrigin( 0.0f, 0.0f ); }
	void ResetOrigin( float dx, float dy );

protected:
	void GetNormalizedCursorPos( float &flX, float &flY );

	float m_xOrigin;
	float m_yOrigin;

	CDmaVar< float > m_x;
	CDmaVar< float > m_y;
};


#endif // DMEMOUSEINPUT_H
