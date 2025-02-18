//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#if !defined(AFX_GRAPHCONTROL_H__9B50B827_F24D_4C5A_BA6E_A591A64E404D__INCLUDED_)
#define AFX_GRAPHCONTROL_H__9B50B827_F24D_4C5A_BA6E_A591A64E404D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GraphControl.h : header file
//

#include "utlvector.h"


class CGraphEntry
{
public:
	CGraphEntry() :
	  m_msTime( 0 ),
	  m_nBytesSent( 0 ),
	  m_nBytesReceived( 0 )
	{
	}

	int	m_msTime;
	int	m_nBytesSent;
	int	m_nBytesReceived;
};


/////////////////////////////////////////////////////////////////////////////
// CGraphControl window

class CGraphControl : public CWnd
{
// Construction
public:
	CGraphControl();

// Attributes
public:

// Operations
public:

	void		Clear();

	// This function assumes you've already run the query and the graph_entry's are selected in.
	void		Fill( CUtlVector<CGraphEntry> &entries );


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraphControl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGraphControl();


protected:

	void	Render( CDC *pDC );

	CUtlVector<CGraphEntry>	m_Entries;

	// Generated message map functions
protected:
	//{{AFX_MSG(CGraphControl)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHCONTROL_H__9B50B827_F24D_4C5A_BA6E_A591A64E404D__INCLUDED_)
