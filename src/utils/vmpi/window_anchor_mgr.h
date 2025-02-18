//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef WINDOW_ANCHOR_MGR_H
#define WINDOW_ANCHOR_MGR_H
#ifdef _WIN32
#pragma once
#endif


#include "utllinkedlist.h"


enum
{
	ANCHOR_LEFT = 1,
	ANCHOR_RIGHT,
	ANCHOR_TOP,
	ANCHOR_BOTTOM,
	ANCHOR_WIDTH_PERCENT,
	ANCHOR_HEIGHT_PERCENT
};


class CWindowAnchor
{
public:
	
	bool	Init( CWnd *pParentWnd, CWnd *pChildWnd, int aLeft, int aTop, int aRight, int aBottom );
	void	Update( CWnd *pParentWnd );


private:
	CWnd	*m_pWnd;
	CRect	m_Rect;	// The rectangle in client coordinates of the parent.
	CRect	m_ParentRect;
	
	int		m_aLeft, m_aTop, m_aRight, m_aBottom;
};


class CWindowAnchorMgr
{
public:

	bool	AddAnchor( CWnd *pParentWnd, CWnd *pChildWnd, int aLeft, int aTop, int aRight, int aBottom );
	void	UpdateAnchors( CWnd *pParentWnd );


private:
	CUtlLinkedList<CWindowAnchor,int>	m_Anchors;
};


#endif // WINDOW_ANCHOR_MGR_H
