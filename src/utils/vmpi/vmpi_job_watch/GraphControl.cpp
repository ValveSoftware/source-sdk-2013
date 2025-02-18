//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// GraphControl.cpp : implementation file
//

#include "stdafx.h"
#include "vmpi_browser_job_watch.h"
#include "GraphControl.h"
#include "mathlib/mathlib.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGraphControl

CGraphControl::CGraphControl()
{
}

CGraphControl::~CGraphControl()
{
}


BEGIN_MESSAGE_MAP(CGraphControl, CWnd)
	//{{AFX_MSG_MAP(CGraphControl)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CGraphControl::Clear()
{
	CRect rcClient;
	GetClientRect( rcClient );

	CDC *pDC = GetDC();

	CBrush brush( RGB( 0, 0, 0 ) );
	CBrush *pOldBrush = pDC->SelectObject( &brush );
	pDC->Rectangle( 0, 0, rcClient.Width(), rcClient.Height() );
	pDC->SelectObject( pOldBrush );

	ReleaseDC( pDC );
}

void CGraphControl::Render( CDC *pDC )
{
	// Clear the background.
	CRect rcClient;
	GetClientRect( rcClient );


	CBrush brush( RGB( 0, 0, 0 ) );
	CBrush *pOldBrush = pDC->SelectObject( &brush );
	pDC->Rectangle( 0, 0, rcClient.Width(), rcClient.Height() );
	pDC->SelectObject( pOldBrush );



	// Work backwards from the right side to the left.
	int nIntervals = rcClient.Width();
	DWORD intervalMS = 500;	// one interval per pixel
	DWORD startTime = 0xFFFFFFFF, endTime = 0;

	// First, find which order of magnitude to use on the vertical scale by finding the maximum value.
	for ( int iEntry=0; iEntry < m_Entries.Count(); iEntry++ )
	{
		DWORD msTime = m_Entries[iEntry].m_msTime;
		startTime = min( startTime, msTime );
		endTime = max( endTime, msTime );
	}

	int curTime = (int)endTime - nIntervals*intervalMS;
	


	CGraphEntry prevEntry, curEntry;
	prevEntry.m_msTime = curEntry.m_msTime = -1;	

	CUtlVector<POINT> sentPoints;
	CUtlVector<POINT> receivedPoints;

	int iCurEntry = -1;
	int nMaxBytesSent = -1, nMaxBytesReceived = -1;

	for ( int x=0; x < nIntervals; x++ )
	{
		if ( curTime >= 0 )
		{
			// Now find the graph_entry for the time we're at.
			while ( prevEntry.m_msTime == -1 || curTime > curEntry.m_msTime )
			{
				++iCurEntry;
				if ( iCurEntry >= m_Entries.Count() )
					goto ENDLOOP;

				prevEntry = curEntry;
				curEntry = m_Entries[iCurEntry];
			}

			if ( curTime >= prevEntry.m_msTime && curTime <= curEntry.m_msTime )
			{
				// Interpolate the bytes sent.
				int nBytesSent = (int)RemapVal( 
					curTime, 
					prevEntry.m_msTime, curEntry.m_msTime,
					prevEntry.m_nBytesSent, curEntry.m_nBytesSent );

				POINT sentPoint = { x, nBytesSent };
				sentPoints.AddToTail( sentPoint );
				nMaxBytesSent = max( nMaxBytesSent, nBytesSent );

				
				int nBytesReceived = (int)RemapVal(
					curTime, 
					prevEntry.m_msTime, curEntry.m_msTime,
					prevEntry.m_nBytesReceived, curEntry.m_nBytesReceived );
				
				POINT receivedPoint = { x, nBytesReceived };
				receivedPoints.AddToTail( receivedPoint );
				nMaxBytesReceived = max( nMaxBytesReceived, nBytesReceived );
			}
		}			
		
		curTime += intervalMS;
	}

	ENDLOOP:;


	// Now normalize all the values.
	int largest = max( nMaxBytesSent, nMaxBytesReceived );
	int topValue = (largest*11) / 10;
/*
	DWORD nZeros;
	for( nZeros = 1; nZeros < 20; nZeros++ )
	{
		if ( largest < pow( 10, nZeros ) )
			break;
	}

	// Now find the value at the top of the graph. We choose the smallest enclosing tenth of the 
	// order of magnitude we're at (so if we were at 1,000,000, and our max value was 350,000, we'd choose 400,000).
	int iTenth;
	int topValue;
	for ( iTenth=1; iTenth <= 10; iTenth++ )
	{
		topValue = (DWORD)( pow( 10, nZeros-1 ) * iTenth );
		if ( topValue >= largest )
			break;
	}
*/

	for ( int iSample=0; iSample < sentPoints.Count(); iSample++ )
	{
		double flHeight;
		
		flHeight = ((double)sentPoints[iSample].y / topValue) * (rcClient.Height() - 1);
		sentPoints[iSample].y = (int)( rcClient.Height() - flHeight );

		flHeight = ((double)receivedPoints[iSample].y / topValue) * (rcClient.Height() - 1);
		receivedPoints[iSample].y = (int)( rcClient.Height() - flHeight );
	}


	// Draw some horizontal lines dividing the space.
	int nLines = 10;
	for ( int iLine=0; iLine <= nLines; iLine++ )
	{
		CPen penLine;
		COLORREF color;
		if ( iLine == 0 || iLine == nLines/2 )
			color = RGB( 0, 220, 0 );
		else
			color = RGB( 0, 100, 0 );

		penLine.CreatePen( PS_SOLID, 1, color );
		CPen *pOldPen = pDC->SelectObject( &penLine );

		int y = (iLine * rcClient.Height()) / nLines;
		pDC->MoveTo( 0, y );
		pDC->LineTo( rcClient.Width(), y );
	
		pDC->SelectObject( pOldPen );
	}


	// Now draw the lines for the data.
	CPen penSent( PS_SOLID, 1, RGB( 0, 255, 0 ) );
	CPen *pOldPen = pDC->SelectObject( &penSent );
	pDC->Polyline( sentPoints.Base(), sentPoints.Count() );
	pDC->SelectObject( pOldPen );

	CPen penReceived( PS_SOLID, 1, RGB( 255, 255, 0 ) );
	pOldPen = pDC->SelectObject( &penReceived );
	pDC->Polyline( receivedPoints.Base(), receivedPoints.Count() );
	pDC->SelectObject( pOldPen );



	// Draw text labels.
	pDC->SetTextColor( RGB( 200, 200, 200 ) );
	pDC->SetBkColor( 0 );

	CString str;
	str.Format( "%dk", (topValue+511) / 1024 );
	pDC->ExtTextOut( 0, 1, 0, NULL, str, NULL );

	str.Format( "%dk", (topValue+511) / 1024 / 2 );
	pDC->ExtTextOut( 0, rcClient.Height()/2 + 1, 0, NULL, str, NULL );
}


void CGraphControl::Fill( CUtlVector<CGraphEntry> &entries )
{
	CDC *pDC = GetDC();
	if ( !pDC )
		return;

	m_Entries = entries;

	Render( pDC );

	ReleaseDC( pDC );
}


/////////////////////////////////////////////////////////////////////////////
// CGraphControl message handlers

void CGraphControl::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	Render( &dc );
}
