//========= Copyright Valve Corporation, All rights reserved. ============//
//
// various transform-related inputs - translation, rotation, etc.
//
//=============================================================================

#ifndef DMETRANSFORMINPUT_H
#define DMETRANSFORMINPUT_H
#ifdef _WIN32
#pragma once
#endif

#include "movieobjects/dmeinput.h"


//-----------------------------------------------------------------------------
// translation input
//-----------------------------------------------------------------------------
class CDmeTranslationInput : public CDmeInput
{
	DEFINE_ELEMENT( CDmeTranslationInput, CDmeInput );

public:
	virtual bool IsDirty(); // ie needs to operate
	virtual void Operate();

	virtual void GetInputAttributes ( CUtlVector< CDmAttribute * > &attrs );
	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

protected:
	CDmaVar< Vector > m_translation;
};


//-----------------------------------------------------------------------------
// rotation input
//-----------------------------------------------------------------------------
class CDmeRotationInput : public CDmeInput
{
	DEFINE_ELEMENT( CDmeRotationInput, CDmeInput );

public:
	virtual bool IsDirty(); // ie needs to operate
	virtual void Operate();

	virtual void GetInputAttributes ( CUtlVector< CDmAttribute * > &attrs );
	virtual void GetOutputAttributes( CUtlVector< CDmAttribute * > &attrs );

	// these should only be used by the application - not other dme's!
	void SetRotation( const Quaternion& quat );
	void SetRotation( const QAngle& qangle );

protected:
	CDmaVar< Quaternion > m_orientation;
	CDmaVar< QAngle > m_angles;
};


#endif // DMETRANSFORMINPUT_H
