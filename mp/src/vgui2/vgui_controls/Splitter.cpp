//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include <vgui/IScheme.h>
#include <vgui/Cursor.h>
#include <vgui/IInput.h>
#include <vgui_controls/Splitter.h>
#include "tier1/KeyValues.h"
#include <limits.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;


enum
{
	SPLITTER_HANDLE_WIDTH = 4
};


//-----------------------------------------------------------------------------
// Splitter handle
//-----------------------------------------------------------------------------
namespace vgui
{

class SplitterHandle : public Panel
{
	DECLARE_CLASS_SIMPLE( SplitterHandle, Panel );

public:
	SplitterHandle( Splitter *parent, const char *name, SplitterMode_t mode, int nIndex );
	~SplitterHandle();

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void OnMousePressed( MouseCode code );
	virtual void OnMouseReleased( MouseCode code );
	virtual void OnCursorMoved( int x, int y );
	virtual void OnMouseDoublePressed( MouseCode code );

private:
	SplitterMode_t m_nMode;
	int m_nIndex;
	bool m_bDragging;
};

} // end namespace vgui

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
SplitterHandle::SplitterHandle( Splitter *parent, const char *name, SplitterMode_t mode, int nIndex ) : BaseClass( parent, name )
{
	int w, h;
	parent->GetSize( w, h );

	if ( mode == SPLITTER_MODE_HORIZONTAL )
	{
		SetSize( w, SPLITTER_HANDLE_WIDTH );
		SetCursor( dc_sizens );
	}
	else
	{
		SetSize( SPLITTER_HANDLE_WIDTH, h );
		SetCursor( dc_sizewe );
	}

	SetVisible( true );
	SetPaintBackgroundEnabled( false );
	SetPaintEnabled( false );
	SetPaintBorderEnabled( true );
	m_bDragging = false;
	m_nIndex = nIndex;
	m_nMode = mode;
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
SplitterHandle::~SplitterHandle()
{
}


//-----------------------------------------------------------------------------
// Scheme settings
//-----------------------------------------------------------------------------
void SplitterHandle::ApplySchemeSettings(IScheme *pScheme)
{
	// Cache off background color stored in SetSplitterColor
	Color c = GetBgColor();
	SetBorder(pScheme->GetBorder("ButtonDepressedBorder"));
	BaseClass::ApplySchemeSettings(pScheme);
	SetBgColor( c );
}


//-----------------------------------------------------------------------------
// Capture mouse when dragging
//-----------------------------------------------------------------------------
void SplitterHandle::OnMousePressed(MouseCode code)
{
	if ( !m_bDragging )
	{
		input()->SetMouseCapture(GetVPanel());
		m_bDragging = true;
	}
}


//-----------------------------------------------------------------------------
// Release mouse capture when finished dragging
//-----------------------------------------------------------------------------
void SplitterHandle::OnMouseReleased(MouseCode code)
{
	if ( m_bDragging )
	{
		input()->SetMouseCapture(NULL);
		m_bDragging = false;
	}
}


//-----------------------------------------------------------------------------
// While dragging, update the splitter position
//-----------------------------------------------------------------------------
void SplitterHandle::OnCursorMoved(int x, int y)
{
	if (m_bDragging)
	{
		input()->GetCursorPos( x, y );
		Splitter *pSplitter = assert_cast<Splitter*>( GetParent() );
		pSplitter->ScreenToLocal( x,y );
		pSplitter->SetSplitterPosition( m_nIndex, (m_nMode == SPLITTER_MODE_HORIZONTAL) ? y : x );
	}
}


//-----------------------------------------------------------------------------
// Double-click: make both panels on either side of the splitter equal size
//-----------------------------------------------------------------------------
void SplitterHandle::OnMouseDoublePressed( MouseCode code )
{
	Splitter *pSplitter = assert_cast<Splitter*>( GetParent() );
	pSplitter->EvenlyRespaceSplitters();
}



//-----------------------------------------------------------------------------
// Returns a panel that chains user configs
//-----------------------------------------------------------------------------
namespace vgui
{

class SplitterChildPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( SplitterChildPanel, EditablePanel );

public:
	SplitterChildPanel( Panel *parent, const char *panelName ) : BaseClass( parent, panelName )
	{
		SetPaintBackgroundEnabled( false );
		SetPaintEnabled( false );
		SetPaintBorderEnabled( false );
	}

	virtual ~SplitterChildPanel() {}

	// Children may have user config settings
	bool HasUserConfigSettings()
	{
		return true;
	}
};

} // end namespace vgui

//-----------------------------------------------------------------------------
//
// Splitter panel
//
//-----------------------------------------------------------------------------
vgui::Panel *Splitter_V_Factory()
{
	return new Splitter( NULL, NULL, SPLITTER_MODE_VERTICAL, 1 );
}

vgui::Panel *Splitter_H_Factory()
{
	return new Splitter( NULL, NULL, SPLITTER_MODE_HORIZONTAL, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
Splitter::Splitter( Panel *parent, const char *name, SplitterMode_t mode, int nCount ) : BaseClass( parent, name )
{
	Assert( nCount >= 1 );
	m_Mode = mode;

	SetPaintBackgroundEnabled( false );
	SetPaintEnabled( false );
	SetPaintBorderEnabled( false );

	RecreateSplitters( nCount );

	EvenlyRespaceSplitters();
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
Splitter::~Splitter()
{
	m_Splitters.RemoveAll();
}

void Splitter::RecreateSplitters( int nCount )
{
	int i;
	int c = m_Splitters.Count();
	for ( i = 0; i < c; ++i )
	{
		delete m_Splitters[ i ].m_pPanel;
		delete m_Splitters[ i ].m_pHandle;
	}
	m_Splitters.RemoveAll();

	for ( i = 0; i < (nCount + 1); ++i )
	{
		char pBuffer[512];
		Q_snprintf( pBuffer, sizeof(pBuffer), "child%d", i );

		int nIndex = m_Splitters.AddToTail( );
		SplitterChildPanel *pEditablePanel = new SplitterChildPanel( this, pBuffer );
		m_Splitters[nIndex].m_pPanel = pEditablePanel;
		m_Splitters[nIndex].m_bLocked = false;
		m_Splitters[nIndex].m_nLockedSize = 0;
	}

	// We do this in 2 loops so that the first N children are actual child panels
	for ( i = 0; i < nCount; ++i )
	{
		SplitterHandle *pHandle = new SplitterHandle( this, "SplitterHandle", m_Mode, i );
		m_Splitters[i].m_pHandle = pHandle;
		pHandle->MoveToFront();
	}
	m_Splitters[nCount].m_pHandle = NULL;
}


//-----------------------------------------------------------------------------
// Sets the splitter color
//-----------------------------------------------------------------------------
void Splitter::SetSplitterColor( Color c )
{
	int nCount = m_Splitters.Count() - 1;
	if ( c.a() != 0 )
	{
		for ( int i = 0; i < nCount; ++i )
		{
			m_Splitters[i].m_pHandle->SetBgColor( c );
			m_Splitters[i].m_pHandle->SetPaintBackgroundEnabled( true );
		}
	}
	else
	{
		for ( int i = 0; i < nCount; ++i )
		{
			m_Splitters[i].m_pHandle->SetPaintBackgroundEnabled( false );
		}
	}
}

	
//-----------------------------------------------------------------------------
// Enables borders on the splitters
//-----------------------------------------------------------------------------
void Splitter::EnableBorders( bool bEnable )
{
	int nCount = m_Splitters.Count() - 1;
	for ( int i = 0; i < nCount; ++i )
	{
		m_Splitters[i].m_pHandle->SetPaintBorderEnabled( bEnable );
	}
}


//-----------------------------------------------------------------------------
// controls splitters
//-----------------------------------------------------------------------------
int Splitter::GetSplitterCount() const
{
	return m_Splitters.Count() - 1;
}


//-----------------------------------------------------------------------------
// controls splitters
//-----------------------------------------------------------------------------
int Splitter::GetSubPanelCount() const
{
	return m_Splitters.Count();
}


//-----------------------------------------------------------------------------
// Purpose: Applies resouce settings
//-----------------------------------------------------------------------------
void Splitter::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);

	// Look for splitter positions
	int nSplitterCount = GetSplitterCount();
	for ( int i = 0; i < nSplitterCount; ++i )
	{
		char pBuffer[512];
		Q_snprintf( pBuffer, sizeof(pBuffer), "splitter%d", i );

		int nSplitterPos = inResourceData->GetInt( pBuffer , -1 );
		if ( nSplitterPos >= 0 )
		{
			SetSplitterPosition( i, nSplitterPos );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int Splitter::GetPosRange()
{
	int w, h;
	GetSize( w, h );
	int nPosRange = (m_Mode == SPLITTER_MODE_HORIZONTAL) ? h : w;
	return nPosRange;
}


//-----------------------------------------------------------------------------
// Locks the size of a particular child in pixels.
//-----------------------------------------------------------------------------
void Splitter::LockChildSize( int nChildIndex, int nSize )
{
	Assert( nChildIndex < m_Splitters.Count() );
	SplitterInfo_t &info = m_Splitters[nChildIndex];
	nSize += SPLITTER_HANDLE_WIDTH;
	if ( !info.m_bLocked || (info.m_nLockedSize != nSize) )
	{ 
		float flPrevPos = (nChildIndex > 0) ? m_Splitters[nChildIndex-1].m_flPos : 0.0f;
		float flOldSize = info.m_flPos - flPrevPos;
		float flDelta = nSize - flOldSize;
		int nCount = m_Splitters.Count();
		for ( int i = nChildIndex; i < nCount-1; ++i )
		{
			m_Splitters[i].m_flPos += flDelta;
		}
		m_Splitters[nCount-1].m_flPos = GetPosRange();

		info.m_bLocked = true;
		info.m_nLockedSize = nSize;
		InvalidateLayout();
	}
}

void Splitter::UnlockChildSize( int nChildIndex )
{
	Assert( nChildIndex < m_Splitters.Count() );
	SplitterInfo_t &info = m_Splitters[nChildIndex]; 
	if ( info.m_bLocked )
	{
		info.m_bLocked = false;

		float flPrevPos = (nChildIndex > 0) ? m_Splitters[nChildIndex-1].m_flPos : 0.0f;
		float flBelowSize = GetPosRange() - flPrevPos;

		int nLockedSize = ComputeLockedSize( nChildIndex + 1 );
		int nUnlockedCount = 1;
		int nCount = m_Splitters.Count();
		for ( int i = nChildIndex + 1; i < nCount; ++i )
		{
			if ( !m_Splitters[i].m_bLocked )
			{
				++nUnlockedCount;
			}
		}

		float flUnlockedSize = ( flBelowSize - nLockedSize ) / nUnlockedCount;

		for ( int i = nChildIndex; i < nCount; ++i )
		{
			if ( !m_Splitters[i].m_bLocked )
			{
				m_Splitters[i].m_flPos = flPrevPos + flUnlockedSize;
			}
			else
			{
				m_Splitters[i].m_flPos = flPrevPos + m_Splitters[i].m_nLockedSize;
			}
			flPrevPos = m_Splitters[i].m_flPos; 
		}
		InvalidateLayout();
	}
}


//-----------------------------------------------------------------------------
// Called when size changes
//-----------------------------------------------------------------------------
void Splitter::OnSizeChanged( int newWide, int newTall )
{
	BaseClass::OnSizeChanged( newWide, newTall );

	// Don't resize if it's degenerate and won't show up anyway...
	if ( newTall <= 0 || newWide <= 0 )
		return;

	int nLockedSize = 0;
	float flUnlockedSize = 0.0f;
	int nCount = m_Splitters.Count();
	float flLastPos = 0.0f;
	int nUnlockedCount = 0;
	for ( int i = 0; i < nCount; ++i )
	{
		SplitterInfo_t &info = m_Splitters[i];
		if ( info.m_bLocked )
		{
			nLockedSize += info.m_nLockedSize;
		}
		else
		{
			++nUnlockedCount;
			flUnlockedSize += info.m_flPos - flLastPos; 
		}
		flLastPos = info.m_flPos;
	}

	int nNewTotalSize = (m_Mode == SPLITTER_MODE_HORIZONTAL) ? newTall : newWide;
	int nNewUnlockedSize = nNewTotalSize - nLockedSize;
	if ( nNewUnlockedSize < nUnlockedCount * SPLITTER_HANDLE_WIDTH )
	{
		nNewUnlockedSize = nUnlockedCount * SPLITTER_HANDLE_WIDTH;
	}

	float flRatio = nNewUnlockedSize / flUnlockedSize;
	float flLastPrevPos = 0.0f;
	flLastPos = 0.0f;
	for ( int i = 0; i < nCount - 1; ++i )
	{
		SplitterInfo_t &info = m_Splitters[i];
		if ( info.m_bLocked )
		{
			flLastPrevPos = info.m_flPos; 
			info.m_flPos = flLastPos + info.m_nLockedSize;
		}
		else
		{
			float flNewSize = info.m_flPos - flLastPrevPos;
			flNewSize *= flRatio;
			flLastPrevPos = info.m_flPos; 
			info.m_flPos = flLastPos + flNewSize;
		}
		flLastPos = info.m_flPos;
	}

	// Clamp the bottom to 1.0
	m_Splitters[nCount-1].m_flPos = nNewTotalSize;
}


//-----------------------------------------------------------------------------
// Splitter position
//-----------------------------------------------------------------------------
int Splitter::GetSplitterPosition( int nIndex )
{
	return (int)( m_Splitters[nIndex].m_flPos + 0.5f );
}

void Splitter::SetSplitterPosition( int nIndex, int nPos )
{
	int nPosRange = GetPosRange();
	if ( nPosRange == 0 )
		return;

	// If we're locked to a sibling, move the previous sibling first
	while ( ( nIndex >= 0 ) && m_Splitters[nIndex].m_bLocked )
	{
		nPos -= m_Splitters[nIndex].m_nLockedSize;
		--nIndex;
	}
	if ( nIndex < 0 )
		return;

	// Clamp to the valid positional range
	int i;
	int nMinPos = 0;
	for ( i = 0; i < nIndex; ++i )
	{
		if ( !m_Splitters[i].m_bLocked )
		{
			nMinPos += SPLITTER_HANDLE_WIDTH;
		}
		else
		{
			nMinPos += m_Splitters[i].m_nLockedSize;
		}
	}

	int nMaxPos = nPosRange - SPLITTER_HANDLE_WIDTH;
	int c = GetSplitterCount();
	for ( i = nIndex + 1; i < c; ++i )
	{
		if ( !m_Splitters[i].m_bLocked )
		{
			nMaxPos -= SPLITTER_HANDLE_WIDTH;
		}
		else
		{
			nMaxPos -= m_Splitters[i].m_nLockedSize;
		}
	}
	nPos = clamp( nPos, nMinPos, nMaxPos );
	
	m_Splitters[nIndex].m_flPos = nPos;
	int p = nPos;
	for ( i = nIndex - 1 ; i >= 0; --i )
	{
		int nMinPrevPos;
		int nMaxPrevPos;
		if ( !m_Splitters[i+1].m_bLocked )
		{
			nMinPrevPos = -INT_MAX;
			nMaxPrevPos = nPos - SPLITTER_HANDLE_WIDTH;
		}
		else
		{
			nMinPrevPos = nMaxPrevPos = p - m_Splitters[i+1].m_nLockedSize;
		}

		int nCurPos = GetSplitterPosition( i );
		if ( nMaxPrevPos < nCurPos || nMinPrevPos > nCurPos  )
		{
			m_Splitters[ i ].m_flPos = nMaxPrevPos;
			p = nMaxPrevPos;
		}
		else
		{
			p = m_Splitters[ i ].m_flPos;
		}
	}

	for ( i = nIndex + 1 ; i < c; ++i )
	{
		int nMinNextPos;
		int nMaxNextPos;
		if ( !m_Splitters[i].m_bLocked )
		{
			nMinNextPos = nPos + SPLITTER_HANDLE_WIDTH;
			nMaxNextPos = INT_MAX;
		}
		else
		{
			nMinNextPos = nMaxNextPos = nPos + m_Splitters[i].m_nLockedSize;
		}

		int nCurPos = GetSplitterPosition( i );
		if ( nMinNextPos > nCurPos || nMaxNextPos < nCurPos )
		{
			m_Splitters[ i ].m_flPos = nMinNextPos;
			nPos = nMinNextPos;
		}
		else
		{
			nPos = m_Splitters[ i ].m_flPos;
		}
	}

	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Computes the locked size
//-----------------------------------------------------------------------------
int Splitter::ComputeLockedSize( int nStartingIndex )
{
	int nLockedSize = 0;
	int nCount = m_Splitters.Count();
	for ( int i = nStartingIndex; i < nCount; ++i )
	{
		if ( m_Splitters[i].m_bLocked )
		{
			nLockedSize += m_Splitters[i].m_nLockedSize;
		}
	}
	return nLockedSize;
}


//-----------------------------------------------------------------------------
// Evenly respaces all the splitters
//-----------------------------------------------------------------------------
void Splitter::EvenlyRespaceSplitters( )
{
	int nSplitterCount = GetSubPanelCount();
	if ( nSplitterCount == 0 )
		return;

	int nLockedSize = ComputeLockedSize( 0 );
	float flUnlockedSize = (float)( GetPosRange() - nLockedSize );
	float flDPos = flUnlockedSize / (float)nSplitterCount;
	if ( flDPos < SPLITTER_HANDLE_WIDTH )
	{
		flDPos = SPLITTER_HANDLE_WIDTH;
	}
	float flPos = 0.0f;
	for ( int i = 0; i  < nSplitterCount; ++i )
	{
		if ( !m_Splitters[i].m_bLocked )
		{
			flPos += flDPos;
		}
		else
		{
			flPos += m_Splitters[i].m_nLockedSize;
		}
		m_Splitters[i].m_flPos = flPos;
	}

	InvalidateLayout();
}

void Splitter::RespaceSplitters( float *flFractions )
{
	int nSplitterCount = GetSubPanelCount();
	if ( nSplitterCount == 0 )
		return;

	float flPos = 0.0f;
	int nPosRange = GetPosRange();
	for ( int i = 0; i  < nSplitterCount; ++i )
	{
		flPos += flFractions[i];
		m_Splitters[i].m_flPos = flPos * nPosRange;
	}

	Assert( flPos == 1.0f );

	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: sets user settings
//-----------------------------------------------------------------------------
void Splitter::ApplyUserConfigSettings(KeyValues *userConfig)
{ 
	BaseClass::ApplyUserConfigSettings( userConfig );

	// read the splitter sizes
	int c = m_Splitters.Count();
	float *pFractions = (float*)_alloca( c * sizeof(float) );
	float flTotalSize = 0.0f;
	for ( int i = 0; i < c; i++ )
	{
		char name[128];
		_snprintf(name, sizeof(name), "%d_splitter_pos", i);
		pFractions[i] = userConfig->GetFloat( name, flTotalSize + SPLITTER_HANDLE_WIDTH + 1 );
		flTotalSize = pFractions[i];
	}

	if ( flTotalSize != 0.0f )
	{
		int nPosRange = GetPosRange();
		for ( int i = 0; i < c; ++i )
		{
			pFractions[i] /= flTotalSize;
			m_Splitters[i].m_flPos = pFractions[i] * nPosRange;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns user config settings for this control
//-----------------------------------------------------------------------------
void Splitter::GetUserConfigSettings(KeyValues *userConfig)
{
	BaseClass::GetUserConfigSettings( userConfig );

	// save which columns are hidden
	int c = m_Splitters.Count();
	for ( int i = 0 ; i < c; i++ )
	{
		char name[128];
		_snprintf(name, sizeof(name), "%d_splitter_pos", i);
		userConfig->SetFloat( name, m_Splitters[i].m_flPos );
	}
}


//-----------------------------------------------------------------------------
// Called to perform layout
//-----------------------------------------------------------------------------
void Splitter::PerformLayout( )
{
	BaseClass::PerformLayout();

	int nSplitterCount = GetSubPanelCount();
	if ( nSplitterCount == 0 )
		return;

	int w, h;
	GetSize( w, h );

	int nLastPos = 0;
	for ( int i	= 0; i < nSplitterCount; ++i )
	{
		Panel *pChild = m_Splitters[i].m_pPanel;
		SplitterHandle *pHandle = m_Splitters[i].m_pHandle;
		int nSplitterPos = (int)( m_Splitters[i].m_flPos + 0.5f );

		if ( m_Mode == SPLITTER_MODE_HORIZONTAL )
		{
			pChild->SetPos( 0, nLastPos );
			pChild->SetSize( w, nSplitterPos - nLastPos );
			if ( pHandle )
			{
				pHandle->SetPos( 0, nSplitterPos );
				pHandle->SetSize( w, SPLITTER_HANDLE_WIDTH );
			}
		}
		else
		{
			pChild->SetPos( nLastPos, 0 );
			pChild->SetSize( nSplitterPos - nLastPos, h );
			if ( pHandle )
			{
				pHandle->SetPos( nSplitterPos, 0 );
				pHandle->SetSize( SPLITTER_HANDLE_WIDTH, h );
			}
		}

		nLastPos = nSplitterPos + SPLITTER_HANDLE_WIDTH;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Splitter::GetSettings( KeyValues *outResourceData )
{
	BaseClass::GetSettings( outResourceData );
}
