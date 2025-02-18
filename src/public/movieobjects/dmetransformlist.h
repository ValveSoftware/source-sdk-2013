//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Snapshot of 
//
//===========================================================================//

#ifndef DMETRANSFORMLIST_H
#define DMETRANSFORMLIST_H

#ifdef _WIN32
#pragma once
#endif

#include "movieobjects/dmedag.h"


//-----------------------------------------------------------------------------
// A class representing a skeletal model
//-----------------------------------------------------------------------------
class CDmeTransformList : public CDmElement
{
	DEFINE_ELEMENT( CDmeTransformList, CDmElement );

public:
	int GetTransformCount() const;
	CDmeTransform *GetTransform( int nIndex );
	void SetTransform( int nIndex, const matrix3x4_t& mat );

	CDmaElementArray<CDmeTransform> m_Transforms;
};


//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline int CDmeTransformList::GetTransformCount() const
{
	return m_Transforms.Count();
}

inline CDmeTransform *CDmeTransformList::GetTransform( int nIndex )
{
	return m_Transforms[nIndex];
}


#endif // DMETRANSFORMLIST_H
