//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#define PROTECTED_THINGS_DISABLE

#include <vgui/IBorder.h>
#include <vgui/IInput.h>
#include <vgui/ISystem.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/MouseCode.h>
#include <KeyValues.h>

#include <vgui_controls/ScrollBarSlider.h>
#include <vgui_controls/Controls.h>

#include <math.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// The ScrollBarSlider is the scroll bar nob that moves up and down in through a range.
//-----------------------------------------------------------------------------
ScrollBarSlider::ScrollBarSlider(Panel *parent, const char *panelName, bool vertical) : Panel(parent, panelName)
{
	_vertical=vertical;	
	_dragging=false;
	_value=0;
	_range[0]=0;
	_range[1]=0;
	_rangeWindow=0;
	_buttonOffset=0;
	_ScrollBarSliderBorder=NULL;
	RecomputeNobPosFromValue();
	SetBlockDragChaining( true );
}

//-----------------------------------------------------------------------------
// Purpose: Set the size of the ScrollBarSlider nob
//-----------------------------------------------------------------------------
void ScrollBarSlider::SetSize(int wide,int tall)
{
	BaseClass::SetSize(wide,tall);
	RecomputeNobPosFromValue();
}

//-----------------------------------------------------------------------------
// Purpose: Whether the scroll bar is vertical (true) or not (false)
//-----------------------------------------------------------------------------
bool ScrollBarSlider::IsVertical()
{
	return _vertical;
}

//-----------------------------------------------------------------------------
// Purpose: Set the ScrollBarSlider value of the nob.
//-----------------------------------------------------------------------------
void ScrollBarSlider::SetValue(int value)
{
	int oldValue = _value;

	if (value > _range[1] - _rangeWindow)
	{
		// note our scrolling range must take into acount _rangeWindow
		value = _range[1] - _rangeWindow;	
	}

	if (value < _range[0])
	{
		value = _range[0];
	}

	_value = value;
	RecomputeNobPosFromValue();

	if (_value != oldValue)
	{
		SendScrollBarSliderMovedMessage();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the ScrollBarSlider value of the nob.
//-----------------------------------------------------------------------------
int ScrollBarSlider::GetValue()
{
	return _value;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ScrollBarSlider::PerformLayout()
{
	RecomputeNobPosFromValue();
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Given the value of the ScrollBarSlider, adjust the ends of the nob.
//-----------------------------------------------------------------------------
void ScrollBarSlider::RecomputeNobPosFromValue()
{
	int wide, tall;
	GetPaintSize(wide, tall);

	float fwide = (float)( wide - 1 );
	float ftall = (float)( tall - 1 );
	float frange = (float)(_range[1] -_range[0]);
	float fvalue = (float)(_value - _range[0]);
	float frangewindow = (float)(_rangeWindow);
	float fper = ( frange != frangewindow ) ? fvalue / ( frange-frangewindow ) : 0;

//	Msg( "fwide: %f  ftall: %f  frange: %f  fvalue: %f  frangewindow: %f  fper: %f\n",
//		fwide, ftall, frange, fvalue, frangewindow, fper );

	if ( frangewindow > 0 )
	{
		if ( frange <= 0.0 )
		{
			frange = 1.0;
		}

		float width, length;
		if (_vertical)
		{
			width = fwide;
			length = ftall;
		}
		else
		{
			width = ftall;
			length = fwide;
		}
		
		// our size is proportional to frangewindow/frange
		// the scroll bar nob's length reflects the amount of stuff on the screen 
		// vs the total amount of stuff we could scroll through in window
		// so if a window showed half its contents and the other half is hidden the
		// scroll bar's length is half the window.
		// if everything is on the screen no nob is displayed
		// frange is how many 'lines' of stuff we can display
		// frangewindow is how many 'lines' are in the display window
		
		// proportion of whole window that is on screen
		float proportion = frangewindow / frange;
		float fnobsize = length * proportion;
		if ( fnobsize < width ) fnobsize = (float)width;
		
		float freepixels = length - fnobsize;
		
		float firstpixel = freepixels * fper;
		
		_nobPos[0] = (int)( firstpixel );
		_nobPos[1] = (int)( firstpixel + fnobsize );
		
		if ( _nobPos[1] > length )
		{
			_nobPos[0] = (int)( length - fnobsize );
			_nobPos[1] = (int)length;
		}
		
	}

	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Get the ScrollBarSlider value using the location of the nob ends.
//-----------------------------------------------------------------------------
void ScrollBarSlider::RecomputeValueFromNobPos()
{
	int wide, tall;
	GetPaintSize(wide, tall);

	float fwide = (float)( wide - 1 );
	float ftall = (float)( tall - 1 );
	float frange = (float)( _range[1] - _range[0] );
	float fvalue = (float)( _value - _range[0] );
	float fnob = (float)_nobPos[0];
	float frangewindow = (float)(_rangeWindow);

	if ( frangewindow > 0 )
	{
		if ( frange <= 0.0 )
		{
			frange = 1.0;
		}

		// set local width and length
		float width, length;
		if ( _vertical )
		{
			width = fwide;
			length = ftall;
		}
		else
		{
			width = ftall;
			length = fwide;
		}
		
		// calculate the size of the nob
		float proportion = frangewindow / frange;
		float fnobsize = length * proportion;
		
		if ( fnobsize < width )
		{
			fnobsize = width;
		}
		
		// Our scroll bar actually doesnt scroll through all frange lines in the truerange, we
		// actually only scroll through frange-frangewindow number of lines so we must take that 
		// into account when we calculate the value
		// convert to our local size system

		// Make sure we don't divide by zero
		if ( length - fnobsize == 0 )
		{
			fvalue = 0.0f;
		}
		else
		{
			fvalue = (frange - frangewindow) * ( fnob / ( length - fnobsize ) );
		}
	}

	// check to see if we should just snap to the bottom
	if (fabs(fvalue + _rangeWindow - _range[1]) < (0.01f * frange))
	{
		// snap to the end
		_value = _range[1] - _rangeWindow;
	}
	else
	{
		// Take care of rounding issues.
		_value = (int)( fvalue + _range[0] + 0.5);
	}

	// Clamp final result
	_value = ( _value < (_range[1] - _rangeWindow) ) ? _value : (_range[1] - _rangeWindow);

	if (_value < _range[0])
	{
		_value = _range[0];
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check if the ScrollBarSlider can move through one or more pixels per
//			unit of its range.
//-----------------------------------------------------------------------------
bool ScrollBarSlider::HasFullRange()
{
	int wide, tall;
	GetPaintSize(wide, tall);

	float frangewindow = (float)(_rangeWindow);

	float checkAgainst = 0;
	if(_vertical)
	{
		checkAgainst = (float)tall;
	}
	else
	{
		checkAgainst = (float)wide;
	}

	if ( frangewindow > 0 )
	{
		if( frangewindow <= ( checkAgainst + _buttonOffset ) )
		{
			return true;
		}
	}

	return false;
}
	
//-----------------------------------------------------------------------------
// Purpose: Inform other watchers that the ScrollBarSlider was moved
//-----------------------------------------------------------------------------
void ScrollBarSlider::SendScrollBarSliderMovedMessage()
{	
	// send a changed message
	PostActionSignal(new KeyValues("ScrollBarSliderMoved", "position", _value));
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this slider is actually drawing itself
//-----------------------------------------------------------------------------
bool ScrollBarSlider::IsSliderVisible( void )
{
	int itemRange = _range[1] - _range[0];

	// Don't draw nob, no items in list
	if ( itemRange <= 0 )
		return false ;

	// Not enough range
	if ( itemRange <= _rangeWindow )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ScrollBarSlider::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetFgColor(GetSchemeColor("ScrollBarSlider.FgColor", pScheme));
	SetBgColor(GetSchemeColor("ScrollBarSlider.BgColor", pScheme));

	IBorder *newBorder = pScheme->GetBorder2("ScrollBarSliderBorder");

	if ( newBorder )
	{
		_ScrollBarSliderBorder = newBorder;
	}
	else
	{
		_ScrollBarSliderBorder = pScheme->GetBorder("ButtonBorder");
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void ScrollBarSlider::ApplySettings( KeyValues *pInResourceData )
{
	BaseClass::ApplySettings( pInResourceData );

	const char *pButtonBorderName = pInResourceData->GetString( "ButtonBorder", NULL );
	if ( pButtonBorderName )
	{
		_ScrollBarSliderBorder = vgui::scheme()->GetIScheme( GetScheme() )->GetBorder( pButtonBorderName );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void ScrollBarSlider::Paint()
{
	int wide,tall;
	GetPaintSize(wide,tall);

	if ( !IsSliderVisible() )	
		return;

	int nRepeats = Max( QuickPropScale( 1 ), 1 );
	Color col = GetFgColor();
	surface()->DrawSetColor(col);

	if (_vertical)
	{
		if ( GetPaintBackgroundType() == 2 )
		{
			DrawBox( nRepeats, _nobPos[0], wide - nRepeats, _nobPos[1] - _nobPos[0], col, 1.0f );
		}
		else
		{
			// Nob
			surface()->DrawFilledRect(0, _nobPos[0], wide - nRepeats, _nobPos[1]);
		}

		// border
		if (_ScrollBarSliderBorder)
		{
			_ScrollBarSliderBorder->Paint2(0, _nobPos[0], wide - nRepeats, _nobPos[1], -1, 0, 0, nRepeats );
		}
	}
	else
	{
		// horizontal nob
		surface()->DrawFilledRect(_nobPos[0], nRepeats, _nobPos[1], tall - nRepeats * 2 );

		// border
		if (_ScrollBarSliderBorder)
		{
			_ScrollBarSliderBorder->Paint2(_nobPos[0] - nRepeats, nRepeats, _nobPos[1], tall, -1, 0, 0, nRepeats );
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void ScrollBarSlider::PaintBackground()
{
//	BaseClass::PaintBackground();
	
	int wide,tall;
	GetPaintSize(wide,tall);
	surface()->DrawSetColor(GetBgColor());
	surface()->DrawFilledRect(0, 0, wide-1, tall-1);
}

//-----------------------------------------------------------------------------
// Purpose: Set the range of the ScrollBarSlider
//-----------------------------------------------------------------------------
void ScrollBarSlider::SetRange(int min,int max)
{
	if(max<min)
	{
		max=min;
	}

	if(min>max)
	{
		min=max;
	}

	_range[0]=min;
	_range[1]=max;

	// update the value (forces it within the range)
	SetValue( _value );
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Get the range values of the ScrollBarSlider
//-----------------------------------------------------------------------------
void ScrollBarSlider::GetRange(int& min,int& max)
{
	min=_range[0];
	max=_range[1];
}

//-----------------------------------------------------------------------------
// Purpose: Respond to cursor movements, we only care about clicking and dragging
//-----------------------------------------------------------------------------
void ScrollBarSlider::OnCursorMoved(int x,int y)
{
	if (!_dragging)
	{
		return;
	}

//	input()->GetCursorPos(x, y);
//	ScreenToLocal(x, y);

	int wide, tall;
	GetPaintSize(wide, tall);

	if (_vertical)
	{
		_nobPos[0] = _nobDragStartPos[0] + (y - _dragStartPos[1]);
		_nobPos[1] = _nobDragStartPos[1] + (y - _dragStartPos[1]);
		
		if (_nobPos[1] > tall)
		{
			_nobPos[0] = tall - (_nobPos[1] - _nobPos[0]);
			_nobPos[1] = tall;
			SetValue( _range[1] - _rangeWindow );
		}
	}
	else
	{
		_nobPos[0] = _nobDragStartPos[0] + (x - _dragStartPos[0]);
		_nobPos[1] = _nobDragStartPos[1] + (x - _dragStartPos[0]);
		
		if (_nobPos[1] > wide)
		{
			_nobPos[0] = wide - (_nobPos[1] - _nobPos[0]);
			_nobPos[1] = wide;
		}
		
	}
	if (_nobPos[0] < 0)
	{
		_nobPos[1] = _nobPos[1] - _nobPos[0];
		_nobPos[0] = 0;
		SetValue(0);
	}
	
	InvalidateLayout();		// not invalidatelayout - because it won't draw while we're scrolling the slider
	RecomputeValueFromNobPos();
//	Repaint();
	SendScrollBarSliderMovedMessage();
}

//-----------------------------------------------------------------------------
// Purpose: Respond to mouse clicks on the ScrollBarSlider
//-----------------------------------------------------------------------------
void ScrollBarSlider::OnMousePressed(MouseCode code)
{
	int x,y;
	input()->GetCursorPos(x,y);
	ScreenToLocal(x,y);

	if (_vertical)
	{
		if ((y >= _nobPos[0]) && (y < _nobPos[1]))
		{
			_dragging = true;
			input()->SetMouseCapture(GetVPanel());
			_nobDragStartPos[0] = _nobPos[0];
			_nobDragStartPos[1] = _nobPos[1];
			_dragStartPos[0] = x;
			_dragStartPos[1] = y;
		}
		else if (y < _nobPos[0])
		{
			// jump the bar up by the range window
			int val = GetValue();
			val -= _rangeWindow;
			SetValue(val);
		}
		else if (y >= _nobPos[1])
		{
			// jump the bar down by the range window
			int val = GetValue();
			val += _rangeWindow;
			SetValue(val);
		}
	}
	else
	{
		if((x >= _nobPos[0]) && (x < _nobPos[1]))
		{
			_dragging = true;
			input()->SetMouseCapture(GetVPanel());
			_nobDragStartPos[0] = _nobPos[0];
			_nobDragStartPos[1] = _nobPos[1];
			_dragStartPos[0] = x;
			_dragStartPos[1] = y;
		}
		else if (x < _nobPos[0])
		{
			// jump the bar up by the range window
			int val = GetValue();
			val -= _rangeWindow;
			SetValue(val);
		}
		else if (x >= _nobPos[1])
		{
			// jump the bar down by the range window
			int val = GetValue();
			val += _rangeWindow;
			SetValue(val);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Treat double clicks as single clicks
//-----------------------------------------------------------------------------
void ScrollBarSlider::OnMouseDoublePressed(MouseCode code)
{
	OnMousePressed(code);
}

//-----------------------------------------------------------------------------
// Purpose: Stop looking for mouse events when mouse is up.
//-----------------------------------------------------------------------------
void ScrollBarSlider::OnMouseReleased(MouseCode code)
{
	_dragging = false;
	input()->SetMouseCapture(null);
}

//-----------------------------------------------------------------------------
// Purpose: Get the position of the ends of the ScrollBarSlider.
//-----------------------------------------------------------------------------
void ScrollBarSlider::GetNobPos(int& min, int& max)
{
	min=_nobPos[0];
	max=_nobPos[1];
}

//-----------------------------------------------------------------------------
// Purpose: Set the number of lines visible in the window the ScrollBarSlider is attached to
//-----------------------------------------------------------------------------
void ScrollBarSlider::SetRangeWindow(int rangeWindow)
{
	_rangeWindow = rangeWindow;
}

//-----------------------------------------------------------------------------
// Purpose: Get the number of lines visible in the window the ScrollBarSlider is attached to
//-----------------------------------------------------------------------------
int ScrollBarSlider::GetRangeWindow()
{
	return _rangeWindow;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void ScrollBarSlider::SetButtonOffset(int buttonOffset)
{
	_buttonOffset = buttonOffset;
}