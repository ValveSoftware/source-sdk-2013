//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Snapshot of 
//
//===========================================================================//

#ifndef DMEANIMATIONLIST_H
#define DMEANIMATIONLIST_H

#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"
#include "datamodel/dmattribute.h"
#include "datamodel/dmattributevar.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CDmeChannelsClip;


//-----------------------------------------------------------------------------
// A class representing a list of animations
//-----------------------------------------------------------------------------
class CDmeAnimationList : public CDmElement
{
	DEFINE_ELEMENT( CDmeAnimationList, CDmElement );

public:
	int GetAnimationCount() const;
	CDmeChannelsClip *GetAnimation( int nIndex );
	int FindAnimation( const char *pAnimName );
	void SetAnimation( int nIndex, CDmeChannelsClip *pAnimation );
	int AddAnimation( CDmeChannelsClip *pAnimation );
	void RemoveAnimation( int nIndex );

private:
	CDmaElementArray<CDmeChannelsClip> m_Animations;
};


//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline int CDmeAnimationList::GetAnimationCount() const
{
	return m_Animations.Count();
}

inline CDmeChannelsClip *CDmeAnimationList::GetAnimation( int nIndex )
{
	return m_Animations[nIndex];
}


#endif // DMEANIMATIONLIST_H
