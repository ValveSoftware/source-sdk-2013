//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The keyboard input class
//
//=============================================================================

#ifndef DMEKEYBOARDINPUT_H
#define DMEKEYBOARDINPUT_H
#ifdef _WIN32
#pragma once
#endif

#include "movieobjects/dmeinput.h"


//-----------------------------------------------------------------------------
// A class representing a camera
//-----------------------------------------------------------------------------
class CDmeKeyboardInput : public CDmeInput
{
	DEFINE_ELEMENT( CDmeKeyboardInput, CDmeInput );

public:
	virtual bool IsDirty(); // ie needs to operate
	virtual void Operate();

	virtual void GetInputAttributes ( CUtlVector< CDmAttribute * > &attrs );
	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

protected:
	CDmaVar< bool > *m_keys;

	bool GetKeyStatus( uint ki );
};

#endif // DMEKEYBOARDINPUT_H
