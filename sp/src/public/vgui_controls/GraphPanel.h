//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef GRAPHPANEL_H
#define GRAPHPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include "utllinkedlist.h"
#include "utlvector.h"

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Holds and displays a chart
//-----------------------------------------------------------------------------
class GraphPanel : public Panel
{
	DECLARE_CLASS_SIMPLE( GraphPanel, Panel );

public:
	GraphPanel(Panel *parent, const char *name);
	
	// domain settings (x-axis settings)
	// sets the window of samples to display
	void SetDisplayDomainSize(float size);
	// sets the range of samples the graph should keep
	// should be set to the max you would set the display domain size
	void SetMaxDomainSize(float size);
	// sets the minimum domain that will be displayed; used to collapse samples
	void SetMinDomainSize(float size);

	// range settings (y-axis settings)
	void SetUseFixedRange(float lowRange, float highRange);
	void SetUseDynamicRange(float *rangeList, int numRanges);
	void GetDisplayedRange(float &lowRange, float &highRange);

	// adds an item to the end of the list
	// sampleEnd is assumed to be the trailing edge of the sample
	// assumes that the samples are fairly evenly spaced (not much more work to do to fix this though)
	void AddItem(float sampleEnd, float sampleValue);

protected:
	virtual void Paint();
	virtual void PerformLayout();
	virtual void ApplySchemeSettings(IScheme *pScheme);

private:
	int GetVisibleItemCount();

	struct Sample_t
	{
		float sampleEnd;
		float value;
	};
	CUtlLinkedList<Sample_t, int> m_Samples;

	// the window to show
	float m_flDomainSize;
	float m_flMaxDomainSize, m_flMinDomainSize;
	bool m_bMaxDomainSizeSet;

	// range
	float m_flLowRange, m_flHighRange;
	bool m_bUseDynamicRange;
	CUtlVector<float> m_RangeList;

	// rendering
	int m_iGraphBarWidth;
	int m_iGraphBarGapWidth;
};

} // namespace vgui

#endif // GRAPHPANEL_H
