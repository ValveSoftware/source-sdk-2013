//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ANALOGBAR_H
#define ANALOGBAR_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Status bar that visually displays discrete analogValue in the form
//			of a segmented strip
//-----------------------------------------------------------------------------
class AnalogBar : public Panel
{
	DECLARE_CLASS_SIMPLE( AnalogBar, Panel );

public:
	AnalogBar(Panel *parent, const char *panelName);
	~AnalogBar();

	// 'analogValue' is in the range [0.0f, 1.0f]
	MESSAGE_FUNC_FLOAT( SetAnalogValue, "SetAnalogValue", analogValue );
	float GetAnalogValue();
	virtual void SetSegmentInfo( int gap, int width );

	// utility function for calculating a time remaining string
	static bool ConstructTimeRemainingString(OUT_Z_BYTECAP(outputBufferSizeInBytes) wchar_t *output, int outputBufferSizeInBytes, float startTime, float currentTime, float currentAnalogValue, float lastAnalogValueUpdateTime, bool addRemainingSuffix);

	void SetBarInset( int pixels );
	int GetBarInset( void );
	
	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void GetSettings(KeyValues *outResourceData);
	virtual const char *GetDescription();

	// returns the number of segment blocks drawn
	int GetDrawnSegmentCount();
	int GetTotalSegmentCount();

	enum AnalogValueDir_e
	{
		PROGRESS_EAST,
		PROGRESS_WEST,
		PROGRESS_NORTH,
		PROGRESS_SOUTH
	};

	int GetAnalogValueDirection() const { return m_iAnalogValueDirection; }
	void SetAnalogValueDirection( int val ) { m_iAnalogValueDirection = val; }

	void SetHomeValue( float val ) { m_fHomeValue = val; }

	const Color& GetHomeColor( void ) { return m_HomeColor; }
	void SetHomeColor( const Color &color ) { m_HomeColor = color; }

protected:
	virtual void Paint();
	void PaintSegment( int &x, int &y, int tall, int wide, Color color, bool bHome );
	virtual void PaintBackground();
	virtual void ApplySchemeSettings(IScheme *pScheme);
	MESSAGE_FUNC_PARAMS( OnDialogVariablesChanged, "DialogVariables", dialogVariables );
	/* CUSTOM MESSAGE HANDLING
		"SetAnalogValue"
			input:	"analogValue"	- float value of the analogValue to set
	*/

protected:
	int m_iAnalogValueDirection;
	float _analogValue;

private:
	int   _segmentCount;
	int _segmentGap;
	int _segmentWide;
	int m_iBarInset;
	char *m_pszDialogVar;
	
	float m_fHomeValue;
	Color m_HomeColor;
};

//-----------------------------------------------------------------------------
// Purpose: Non-segmented analogValue bar
//-----------------------------------------------------------------------------
class ContinuousAnalogBar : public AnalogBar
{
	DECLARE_CLASS_SIMPLE( ContinuousAnalogBar, AnalogBar );

public:
	ContinuousAnalogBar(Panel *parent, const char *panelName);

	virtual void Paint();
};

} // namespace vgui

#endif // ANALOGBAR_H
