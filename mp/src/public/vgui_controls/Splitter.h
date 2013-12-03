//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef SPLITTER_H
#define SPLITTER_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>

namespace vgui
{

enum SplitterMode_t
{
	SPLITTER_MODE_HORIZONTAL = 0,
	SPLITTER_MODE_VERTICAL
};


class SplitterHandle;
class SplitterChildPanel;

//-----------------------------------------------------------------------------
// Purpose: Thin line used to divide sections, can be moved dragged!
//-----------------------------------------------------------------------------
class Splitter : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( Splitter, EditablePanel );

public:
	// nCount is the number of splitters to create.
	// NOTE: The constructor here will create (nCount+1) EditablePanel children
	// and name them child0...childN for .res file purposes.
	Splitter( Panel *parent, const char *name, SplitterMode_t mode, int nCount );
	~Splitter();

	// Evenly respace all splitters
	void EvenlyRespaceSplitters();

	// respace splitters using given fractions (must sum to 1)
	void RespaceSplitters( float *flFractions );

	// Inherited from Panel
	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void GetSettings( KeyValues *outResourceData );
	virtual void PerformLayout();
	virtual void OnSizeChanged(int newWide, int newTall);
	virtual void ApplyUserConfigSettings(KeyValues *userConfig);
	virtual void GetUserConfigSettings(KeyValues *userConfig);
	virtual bool HasUserConfigSettings() { return true; }

	// Sets the splitter color
	void SetSplitterColor( Color c );

	// Enables borders on the splitters
	void EnableBorders( bool bEnable );

	// Locks the size of a particular child in pixels.
	void LockChildSize( int nChildIndex, int nSize );
	void UnlockChildSize( int nChildIndex );

private:
	void RecreateSplitters( int nCount );

	struct SplitterInfo_t
	{
		SplitterChildPanel *m_pPanel;	// This panel is to the left or above the handle
		SplitterHandle *m_pHandle;
		float m_flPos;
		bool m_bLocked;
		int m_nLockedSize;
	};

	int	GetPosRange();
	int GetSplitterCount() const;
	int GetSplitterPosition( int nIndex );
	void SetSplitterPosition( int nIndex, int nPos );
	int GetSubPanelCount() const;
	int ComputeLockedSize( int nStartingIndex );

	CUtlVector< SplitterInfo_t > m_Splitters;
	SplitterMode_t m_Mode;

	friend class SplitterHandle;
};


} // namespace vgui


#endif // SPLITTER_H
