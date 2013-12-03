//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include <assert.h>
#include <math.h>
#include <stdio.h>

#include <vgui_controls/AnalogBar.h>
#include <vgui_controls/Controls.h>

#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

DECLARE_BUILD_FACTORY( AnalogBar );


#define ANALOG_BAR_HOME_SIZE 4
#define ANALOG_BAR_HOME_GAP 2
#define ANALOG_BAR_LESS_TALL ( ANALOG_BAR_HOME_SIZE + ANALOG_BAR_HOME_GAP )


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
AnalogBar::AnalogBar(Panel *parent, const char *panelName) : Panel(parent, panelName)
{
	_analogValue = 0.0f;
	m_pszDialogVar = NULL;
	SetSegmentInfo( 2, 6 );
	SetBarInset( 0 );
	m_iAnalogValueDirection = PROGRESS_EAST;

	m_fHomeValue = 2.0f;
	m_HomeColor = GetFgColor();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
AnalogBar::~AnalogBar()
{
	delete [] m_pszDialogVar;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void AnalogBar::SetSegmentInfo( int gap, int width )
{
	_segmentGap = gap;
	_segmentWide = width;
}

//-----------------------------------------------------------------------------
// Purpose: returns the number of segment blocks drawn
//-----------------------------------------------------------------------------
int AnalogBar::GetDrawnSegmentCount()
{
	int wide, tall;
	GetSize(wide, tall);
	int segmentTotal = wide / (_segmentGap + _segmentWide);
	return (int)(segmentTotal * _analogValue);
}

//-----------------------------------------------------------------------------
// Purpose: returns the total number of segment blocks drawn (active and inactive)
//-----------------------------------------------------------------------------
int AnalogBar::GetTotalSegmentCount()
{
	int wide, tall;
	GetSize(wide, tall);
	int segmentTotal = wide / (_segmentGap + _segmentWide);
	return segmentTotal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void AnalogBar::PaintBackground()
{
	// Don't draw a background
}

void AnalogBar::PaintSegment( int &x, int &y, int tall, int wide, Color color, bool bHome )
{
	switch( m_iAnalogValueDirection )
	{
	case PROGRESS_EAST:
		x += _segmentGap;

		if ( bHome )
		{
			surface()->DrawSetColor( GetHomeColor() );
			surface()->DrawFilledRect(x, y, x + _segmentWide, y + ANALOG_BAR_HOME_SIZE );
			surface()->DrawFilledRect(x, y + tall - (y * 2) - ANALOG_BAR_HOME_SIZE, x + _segmentWide, y + tall - (y * 2) );
		}

		surface()->DrawSetColor( color );
		surface()->DrawFilledRect(x, y + ANALOG_BAR_LESS_TALL, x + _segmentWide, y + tall - (y * 2) - ANALOG_BAR_LESS_TALL );
		x += _segmentWide;
		break;

	case PROGRESS_WEST:
		x -= _segmentGap + _segmentWide;

		if ( bHome )
		{
			surface()->DrawSetColor( GetHomeColor() );
			surface()->DrawFilledRect(x, y, x + _segmentWide, y + ANALOG_BAR_HOME_SIZE );
			surface()->DrawFilledRect(x, y + tall - (y * 2) - ANALOG_BAR_HOME_SIZE, x + _segmentWide, y + tall - (y * 2) );
		}

		surface()->DrawSetColor( color );
		surface()->DrawFilledRect(x, y + ANALOG_BAR_LESS_TALL, x + _segmentWide, y + tall - (y * 2) - ANALOG_BAR_LESS_TALL );
		break;

	case PROGRESS_NORTH:
		y -= _segmentGap + _segmentWide;

		if ( bHome )
		{
			surface()->DrawSetColor( GetHomeColor() );
			surface()->DrawFilledRect(x, y, x + ANALOG_BAR_HOME_SIZE, y + _segmentWide );
			surface()->DrawFilledRect(x + wide - (x * 2) - ANALOG_BAR_HOME_SIZE, y, x + wide - (x * 2), y + _segmentWide );
		}

		surface()->DrawSetColor( color );
		surface()->DrawFilledRect(x + ANALOG_BAR_LESS_TALL, y, x + wide - (x * 2) - ANALOG_BAR_LESS_TALL, y + _segmentWide);
		break;

	case PROGRESS_SOUTH:
		y += _segmentGap;

		if ( bHome )
		{
			surface()->DrawSetColor( GetHomeColor() );
			surface()->DrawFilledRect(x, y, x + ANALOG_BAR_HOME_SIZE, y + _segmentWide );
			surface()->DrawFilledRect(x + wide - (x * 2) - ANALOG_BAR_HOME_SIZE, y, x + wide - (x * 2), y + _segmentWide );
		}

		surface()->DrawSetColor( color );
		surface()->DrawFilledRect(x + ANALOG_BAR_LESS_TALL, y, x + wide - (x * 2) - ANALOG_BAR_LESS_TALL, y + _segmentWide);
		y += _segmentWide;
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void AnalogBar::Paint()
{
	int wide, tall;
	GetSize(wide, tall);

	// gaps
	int segmentTotal = 0, segmentsDrawn = 0;
	int x = 0, y = 0;

	switch( m_iAnalogValueDirection )
	{
	case PROGRESS_WEST:
		x = wide;
		y = m_iBarInset;
		segmentTotal = wide / (_segmentGap + _segmentWide);
		segmentsDrawn = (int)(segmentTotal * _analogValue + 0.5f);
		break;

	case PROGRESS_EAST:
		x = 0;
		y = m_iBarInset;
		segmentTotal = wide / (_segmentGap + _segmentWide);
		segmentsDrawn = (int)(segmentTotal * _analogValue + 0.5f);
		break;

	case PROGRESS_NORTH:
		x = m_iBarInset;
		y = tall;
		segmentTotal = tall / (_segmentGap + _segmentWide);
		segmentsDrawn = (int)(segmentTotal * _analogValue + 0.5f);
		break;

	case PROGRESS_SOUTH:
		x = m_iBarInset;
		y = 0;
		segmentTotal = tall / (_segmentGap + _segmentWide);
		segmentsDrawn = (int)(segmentTotal * _analogValue + 0.5f);
		break;
	}

	int iHomeIndex = (int)( segmentTotal * m_fHomeValue + 0.5f ) - 1;
	if ( iHomeIndex < 0 )
		iHomeIndex = 0;

	for (int i = 0; i < segmentsDrawn; i++)
		PaintSegment( x, y, tall, wide, GetFgColor(), i == iHomeIndex );

	for (int i = segmentsDrawn; i < segmentTotal; i++)
		PaintSegment( x, y, tall, wide, GetBgColor(), i == iHomeIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void AnalogBar::SetAnalogValue(float analogValue)
{
	if (analogValue != _analogValue)
	{
		// clamp the analogValue value within the range
		if (analogValue < 0.0f)
		{
			analogValue = 0.0f;
		}
		else if (analogValue > 1.0f)
		{
			analogValue = 1.0f;
		}

		_analogValue = analogValue;
		Repaint();
	}
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
float AnalogBar::GetAnalogValue()
{
	return _analogValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void AnalogBar::ApplySchemeSettings(IScheme *pScheme)
{
	Panel::ApplySchemeSettings(pScheme);

	SetBgColor( Color( 255 - GetFgColor().r(), 255 - GetFgColor().g(), 255 - GetFgColor().b(), GetFgColor().a() ) );
}

//-----------------------------------------------------------------------------
// Purpose: utility function for calculating a time remaining string
//-----------------------------------------------------------------------------
bool AnalogBar::ConstructTimeRemainingString(wchar_t *output, int outputBufferSizeInBytes, float startTime, float currentTime, float currentAnalogValue, float lastAnalogValueUpdateTime, bool addRemainingSuffix)
{
	Assert( outputBufferSizeInBytes >= sizeof(output[0]) );
	Assert(lastAnalogValueUpdateTime <= currentTime);
	output[0] = 0;

	// calculate pre-extrapolation values
	float timeElapsed = lastAnalogValueUpdateTime - startTime;
	float totalTime = timeElapsed / currentAnalogValue;

	// calculate seconds
	int secondsRemaining = (int)(totalTime - timeElapsed);
	if (lastAnalogValueUpdateTime < currentTime)
	{
		// old update, extrapolate
		float analogValueRate = currentAnalogValue / timeElapsed;
		float extrapolatedAnalogValue = analogValueRate * (currentTime - startTime);
		float extrapolatedTotalTime = (currentTime - startTime) / extrapolatedAnalogValue;
		secondsRemaining = (int)(extrapolatedTotalTime - timeElapsed);
	}
	// if there's some time, make sure it's at least one second left
	if ( secondsRemaining == 0 && ( ( totalTime - timeElapsed ) > 0 ) )
	{
		secondsRemaining = 1;
	}

	// calculate minutes
	int minutesRemaining = 0;
	while (secondsRemaining >= 60)
	{
		minutesRemaining++;
		secondsRemaining -= 60;
	}

    char minutesBuf[16];
    Q_snprintf(minutesBuf, sizeof( minutesBuf ), "%d", minutesRemaining);
    char secondsBuf[16];
    Q_snprintf(secondsBuf, sizeof( secondsBuf ), "%d", secondsRemaining);

	if (minutesRemaining > 0)
	{
		wchar_t unicodeMinutes[16];
		g_pVGuiLocalize->ConvertANSIToUnicode(minutesBuf, unicodeMinutes, sizeof( unicodeMinutes ));
		wchar_t unicodeSeconds[16];
		g_pVGuiLocalize->ConvertANSIToUnicode(secondsBuf, unicodeSeconds, sizeof( unicodeSeconds ));

		const char *unlocalizedString = "#vgui_TimeLeftMinutesSeconds";
		if (minutesRemaining == 1 && secondsRemaining == 1)
		{
			unlocalizedString = "#vgui_TimeLeftMinuteSecond";
		}
		else if (minutesRemaining == 1)
		{
			unlocalizedString = "#vgui_TimeLeftMinuteSeconds";
		}
		else if (secondsRemaining == 1)
		{
			unlocalizedString = "#vgui_TimeLeftMinutesSecond";
		}

		char unlocString[64];
		Q_strncpy(unlocString, unlocalizedString,sizeof( unlocString ));
		if (addRemainingSuffix)
		{
			Q_strncat(unlocString, "Remaining", sizeof(unlocString ), COPY_ALL_CHARACTERS);
		}
		g_pVGuiLocalize->ConstructString(output, outputBufferSizeInBytes, g_pVGuiLocalize->Find(unlocString), 2, unicodeMinutes, unicodeSeconds);

	}
	else if (secondsRemaining > 0)
	{
		wchar_t unicodeSeconds[16];
		g_pVGuiLocalize->ConvertANSIToUnicode(secondsBuf, unicodeSeconds, sizeof( unicodeSeconds ));

		const char *unlocalizedString = "#vgui_TimeLeftSeconds";
		if (secondsRemaining == 1)
		{
			unlocalizedString = "#vgui_TimeLeftSecond";
		}
		char unlocString[64];
		Q_strncpy(unlocString, unlocalizedString,sizeof(unlocString));
		if (addRemainingSuffix)
		{
			Q_strncat(unlocString, "Remaining",sizeof(unlocString), COPY_ALL_CHARACTERS);
		}
		g_pVGuiLocalize->ConstructString(output, outputBufferSizeInBytes, g_pVGuiLocalize->Find(unlocString), 1, unicodeSeconds);
	}
	else
	{
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void AnalogBar::SetBarInset( int pixels )
{ 
	m_iBarInset = pixels;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
int AnalogBar::GetBarInset( void )
{
	return m_iBarInset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void AnalogBar::ApplySettings(KeyValues *inResourceData)
{
	_analogValue = inResourceData->GetFloat("analogValue", 0.0f);

	const char *dialogVar = inResourceData->GetString("variable", "");
	if (dialogVar && *dialogVar)
	{
		m_pszDialogVar = new char[strlen(dialogVar) + 1];
		strcpy(m_pszDialogVar, dialogVar);
	}

	BaseClass::ApplySettings(inResourceData);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void AnalogBar::GetSettings(KeyValues *outResourceData)
{
	BaseClass::GetSettings(outResourceData);
	outResourceData->SetFloat("analogValue", _analogValue );

	if (m_pszDialogVar)
	{
		outResourceData->SetString("variable", m_pszDialogVar);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns a string description of the panel fields for use in the UI
//-----------------------------------------------------------------------------
const char *AnalogBar::GetDescription( void )
{
	static char buf[1024];
	_snprintf(buf, sizeof(buf), "%s, string analogValue, string variable", BaseClass::GetDescription());
	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: updates analogValue bar bases on values
//-----------------------------------------------------------------------------
void AnalogBar::OnDialogVariablesChanged(KeyValues *dialogVariables)
{
	if (m_pszDialogVar)
	{
		int val = dialogVariables->GetInt(m_pszDialogVar, -1);
		if (val >= 0.0f)
		{
			SetAnalogValue(val / 100.0f);
		}
	}
}


DECLARE_BUILD_FACTORY( ContinuousAnalogBar );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
ContinuousAnalogBar::ContinuousAnalogBar(Panel *parent, const char *panelName) : AnalogBar(parent, panelName)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ContinuousAnalogBar::Paint()
{
	int x = 0, y = 0;
	int wide, tall;
	GetSize(wide, tall);

	surface()->DrawSetColor(GetFgColor());

	switch( m_iAnalogValueDirection )
	{
	case PROGRESS_EAST:
		surface()->DrawFilledRect( x, y, x + (int)( wide * _analogValue ), y + tall );
		break;

	case PROGRESS_WEST:
		surface()->DrawFilledRect( x + (int)( wide * ( 1.0f - _analogValue ) ), y, x + wide, y + tall );
		break;

	case PROGRESS_NORTH:
		surface()->DrawFilledRect( x, y + (int)( tall * ( 1.0f - _analogValue ) ), x + wide, y + tall );
		break;

	case PROGRESS_SOUTH:
		surface()->DrawFilledRect( x, y, x + wide, y + (int)( tall * _analogValue ) );
		break;
	}
}