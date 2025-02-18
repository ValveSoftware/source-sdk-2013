//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include <math.h>

#include <vgui_controls/GraphPanel.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

DECLARE_BUILD_FACTORY( GraphPanel );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
GraphPanel::GraphPanel(Panel *parent, const char *name) : BaseClass(parent, name)
{
	m_flDomainSize = 100.0f;
	m_flLowRange = 0.0f;
	m_flHighRange = 1.0f;
	m_bUseDynamicRange = true;
	m_flMinDomainSize = 0.0f;
	m_flMaxDomainSize = 0.0f;
	m_bMaxDomainSizeSet = false;

	// rendering, need to pull these from scheme/res file
	m_iGraphBarWidth = 2;
	m_iGraphBarGapWidth = 2;
}

//-----------------------------------------------------------------------------
// Purpose: domain settings (x-axis settings)
//-----------------------------------------------------------------------------
void GraphPanel::SetDisplayDomainSize(float size)
{
	m_flDomainSize = size;

	// set the max domain size if it hasn't been set yet
	if (!m_bMaxDomainSizeSet)
	{
		SetMaxDomainSize(size);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the smallest domain that will be displayed
//-----------------------------------------------------------------------------
void GraphPanel::SetMinDomainSize(float size)
{
	m_flMinDomainSize = size;
}

//-----------------------------------------------------------------------------
// Purpose: sets the samples to keep
//-----------------------------------------------------------------------------
void GraphPanel::SetMaxDomainSize(float size)
{
	m_flMaxDomainSize = size;
	m_bMaxDomainSizeSet = true;
}

//-----------------------------------------------------------------------------
// Purpose: range settings (y-axis settings)
//-----------------------------------------------------------------------------
void GraphPanel::SetUseFixedRange(float lowRange, float highRange)
{
	m_bUseDynamicRange = false;
	m_flLowRange = lowRange;
	m_flHighRange = highRange;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the graph to dynamically determine the range
//-----------------------------------------------------------------------------
void GraphPanel::SetUseDynamicRange(float *rangeList, int numRanges)
{
	m_bUseDynamicRange = true;
	m_RangeList.CopyArray(rangeList, numRanges);
}

//-----------------------------------------------------------------------------
// Purpose: Gets the currently displayed range
//-----------------------------------------------------------------------------
void GraphPanel::GetDisplayedRange(float &lowRange, float &highRange)
{
	lowRange = m_flLowRange;
	highRange = m_flHighRange;
}

//-----------------------------------------------------------------------------
// Purpose: adds an item to the end of the list
//-----------------------------------------------------------------------------
void GraphPanel::AddItem(float sampleEnd, float sampleValue)
{
	if (m_Samples.Count() && m_Samples[m_Samples.Tail()].value == sampleValue)
	{
		// collapse identical samples
		m_Samples[m_Samples.Tail()].sampleEnd = sampleEnd;
	}
	else
	{
		// add to the end of the samples list
		Sample_t item;
		item.value = sampleValue;
		item.sampleEnd = sampleEnd;
		m_Samples.AddToTail(item);
	}

	// see if this frees up any samples past the end
	if (m_bMaxDomainSizeSet)
	{
		float freePoint = sampleEnd - m_flMaxDomainSize;
		while (m_Samples[m_Samples.Head()].sampleEnd < freePoint)
		{
			m_Samples.Remove(m_Samples.Head());
		}
	}

/*
	// see the max number of samples necessary to display this information reasonably precisely
	static const int MAX_LIKELY_GRAPH_WIDTH = 800;
	int maxSamplesNeeded = 2 * MAX_LIKELY_GRAPH_WIDTH / (m_iGraphBarWidth + m_iGraphBarGapWidth);
	if (m_Samples.Count() > 2)
	{
		// see if we can collapse some items
		float highestSample = m_Samples[m_Samples.Tail()].sampleEnd;

		// iterate the items
		// always keep the head around so we have something to go against
		int sampleIndex = m_Samples.Next(m_Samples.Head());
		int nextSampleIndex = m_Samples.Next(sampleIndex);

		while (m_Samples.IsInList(nextSampleIndex))
		{
			// calculate what sampling precision is actually needed to display this data
			float distanceFromEnd = highestSample - m_Samples[sampleIndex].sampleEnd;

//			if (distanceFromEnd < m_flDomainSize)
//				break;

			//!! this calculation is very incorrect
			float minNeededSampleSize = distanceFromEnd / (m_flMinDomainSize * maxSamplesNeeded);
			float sampleSize = m_Samples[nextSampleIndex].sampleEnd - m_Samples[sampleIndex].sampleEnd;

			if (sampleSize < minNeededSampleSize)
			{
				// collapse the item into the next index
				m_Samples[nextSampleIndex].value = 0.5f * (m_Samples[nextSampleIndex].value + m_Samples[sampleIndex].value);

				// remove the item from the list
				m_Samples.Remove(sampleIndex);
				
				// move to the next item
				sampleIndex = nextSampleIndex;
				nextSampleIndex = m_Samples.Next(sampleIndex);
			}
			else
			{
				// this item didn't need collapsing, so assume the next item won't
				break;
			}
		}
	}
*/

	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: returns number of items that can be displayed
//-----------------------------------------------------------------------------
int GraphPanel::GetVisibleItemCount()
{
	return GetWide() / (m_iGraphBarWidth + m_iGraphBarGapWidth);
}

//-----------------------------------------------------------------------------
// Purpose: lays out the graph
//-----------------------------------------------------------------------------
void GraphPanel::PerformLayout()
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: draws the graph
//-----------------------------------------------------------------------------
void GraphPanel::Paint()
{
	if (!m_Samples.Count())
		return;

	// walk from right to left drawing the resampled data
	int sampleIndex = m_Samples.Tail();
	int x = GetWide() - (m_iGraphBarWidth + m_iGraphBarGapWidth);

	// calculate how big each sample should be
	float sampleSize = m_flDomainSize / GetVisibleItemCount();

	// calculate where in the domain we start resampling
	float resampleStart = m_Samples[sampleIndex].sampleEnd - sampleSize;
	// always resample from a sample point that is a multiple of the sampleSize
	resampleStart -= (float)fmod(resampleStart, sampleSize);

	// bar size multiplier
	float barSizeMultiplier = GetTall() / (m_flHighRange - m_flLowRange);

	// set render color
	surface()->DrawSetColor(GetFgColor());

	// recalculate the sample range for dynamic resizing
	float flMinValue = m_Samples[m_Samples.Head()].value;
	float flMaxValue = m_Samples[m_Samples.Head()].value;

	// iterate the bars to draw
	while (x > 0 && m_Samples.IsInList(sampleIndex))
	{
		// move back the drawing point
		x -= (m_iGraphBarWidth + m_iGraphBarGapWidth);

		// collect the samples
		float value = 0.0f;
		float maxValue = 0.0f;
		int samplesTouched = 0;
		int prevSampleIndex = m_Samples.Previous(sampleIndex);
		while (m_Samples.IsInList(prevSampleIndex))
		{
			// take the value
			value += m_Samples[sampleIndex].value;
			samplesTouched++;

			// do some work to calculate the sample range
			if (m_Samples[sampleIndex].value < flMinValue)
			{
				flMinValue = m_Samples[sampleIndex].value;
			}
			if (m_Samples[sampleIndex].value > flMaxValue)
			{
				flMaxValue = m_Samples[sampleIndex].value;
			}
			if (m_Samples[sampleIndex].value > maxValue)
			{
				maxValue = m_Samples[sampleIndex].value;
			}

			if (resampleStart < m_Samples[prevSampleIndex].sampleEnd)
			{
				// we're out of the sampling range, we need to move on to the next sample
				sampleIndex = prevSampleIndex;
				prevSampleIndex = m_Samples.Previous(sampleIndex);
			}
			else
			{
				// we're done with this resample
				// move back the resample start
				resampleStart -= sampleSize;
				// draw the current item
				break;
			}
		}

		// draw the item
		// show the max value in the sample, not the average
		int size = (int)(maxValue * barSizeMultiplier);
//		int size = (int)((value * barSizeMultiplier) / samplesTouched);
		surface()->DrawFilledRect(x, GetTall() - size, x + m_iGraphBarWidth, GetTall());
	}

	// calculate our final range (for use next frame)
	if (m_bUseDynamicRange)
	{
		flMinValue = 0;

		// find the range that fits
		for (int i = 0; i < m_RangeList.Count(); i++)
		{
			if (m_RangeList[i] > flMaxValue)
			{
				flMaxValue = m_RangeList[i];
				break;
			}
		}

		m_flLowRange = flMinValue;
		m_flHighRange = flMaxValue;
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets up colors
//-----------------------------------------------------------------------------
void GraphPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetFgColor(GetSchemeColor("GraphPanel.FgColor", pScheme));
	SetBgColor(GetSchemeColor("GraphPanel.BgColor", pScheme));
	SetBorder(pScheme->GetBorder("ButtonDepressedBorder"));
}
