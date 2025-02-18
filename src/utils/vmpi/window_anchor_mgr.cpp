//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "stdafx.h"
#include "window_anchor_mgr.h"


// ------------------------------------------------------------------------------------ //
// CWindowAnchor.
// ------------------------------------------------------------------------------------ //

bool CWindowAnchor::Init( CWnd *pParentWnd, CWnd *pChildWnd, int aLeft, int aTop, int aRight, int aBottom )
{
	m_pWnd = pChildWnd;
	
	m_aLeft = aLeft;
	m_aTop = aTop;
	m_aRight = aRight;
	m_aBottom = aBottom;

	if ( m_pWnd && pParentWnd )
	{
		pParentWnd->GetWindowRect( m_ParentRect );
		pChildWnd->GetWindowRect( m_Rect );
		return true;
	}
	else
	{
		return false;
	}
}


void AnchorElement( long *pOut, CRect &rcParent, int startElement, CRect &rcParentStart, int flags )
{
	if ( flags == ANCHOR_LEFT )
		*pOut = rcParent.left + ( startElement - rcParentStart.left );
	else if ( flags == ANCHOR_TOP )
		*pOut = rcParent.top + ( startElement - rcParentStart.top );
	else if ( flags == ANCHOR_RIGHT )
		*pOut = rcParent.right - ( rcParentStart.right - startElement );
	else if ( flags == ANCHOR_BOTTOM )
		*pOut = rcParent.bottom - ( rcParentStart.bottom - startElement );
	else if ( flags == ANCHOR_WIDTH_PERCENT )
		*pOut = rcParent.left + (rcParent.Width() * ( startElement - rcParentStart.left )) / rcParentStart.Width();
	else if ( flags == ANCHOR_HEIGHT_PERCENT )
		*pOut = rcParent.top + (rcParent.Height() * ( startElement - rcParentStart.top )) / rcParentStart.Height();
}


void CWindowAnchor::Update( CWnd *pParentWnd )
{
	if ( !m_pWnd )
		return;

	CRect rcParent;
	pParentWnd->GetWindowRect( rcParent );

	CRect rcNew;
	AnchorElement( &rcNew.left,   rcParent, m_Rect.left,   m_ParentRect, m_aLeft );
	AnchorElement( &rcNew.top,    rcParent, m_Rect.top,    m_ParentRect, m_aTop );
	AnchorElement( &rcNew.right,  rcParent, m_Rect.right,  m_ParentRect, m_aRight );
	AnchorElement( &rcNew.bottom, rcParent, m_Rect.bottom, m_ParentRect, m_aBottom );

	pParentWnd->ScreenToClient( rcNew );
	m_pWnd->SetWindowPos( NULL, rcNew.left, rcNew.top, rcNew.Width(), rcNew.Height(), SWP_NOZORDER );
	m_pWnd->InvalidateRect( NULL );
}



// ------------------------------------------------------------------------------------ //
// CWindowAnchorMgr.
// ------------------------------------------------------------------------------------ //

bool CWindowAnchorMgr::AddAnchor( CWnd *pParentWnd, CWnd *pChildWnd, int aLeft, int aTop, int aRight, int aBottom )
{
	int index = m_Anchors.AddToTail();
	return m_Anchors[index].Init( pParentWnd, pChildWnd, aLeft, aTop, aRight, aBottom );
}


void CWindowAnchorMgr::UpdateAnchors( CWnd *pParentWnd )
{
	FOR_EACH_LL( m_Anchors, i )
	{
		m_Anchors[i].Update( pParentWnd );
	}
}

