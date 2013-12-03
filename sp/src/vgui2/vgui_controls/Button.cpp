//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Basic button control
//
// $NoKeywords: $
//=============================================================================//

#include <stdio.h>
#include <utlsymbol.h>

#include <vgui/IBorder.h>
#include <vgui/IInput.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>
#include <vgui/MouseCode.h>
#include <vgui/KeyCode.h>
#include <KeyValues.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/FocusNavGroup.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

// global list of all the names of all the sounds played by buttons
CUtlSymbolTable g_ButtonSoundNames;

DECLARE_BUILD_FACTORY_DEFAULT_TEXT( Button, Button );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
Button::Button(Panel *parent, const char *panelName, const char *text, Panel *pActionSignalTarget, const char *pCmd ) : Label(parent, panelName, text)
{
	Init();
	if ( pActionSignalTarget && pCmd )
	{
		AddActionSignalTarget( pActionSignalTarget );
		SetCommand( pCmd );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
Button::Button(Panel *parent, const char *panelName, const wchar_t *wszText, Panel *pActionSignalTarget, const char *pCmd ) : Label(parent, panelName, wszText)
{
	Init();
	if ( pActionSignalTarget && pCmd )
	{
		AddActionSignalTarget( pActionSignalTarget );
		SetCommand( pCmd );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Button::Init()
{
	_buttonFlags.SetFlag( USE_CAPTURE_MOUSE | BUTTON_BORDER_ENABLED );

	_mouseClickMask = 0;
	_actionMessage = NULL;
	_defaultBorder = NULL;
	_depressedBorder = NULL;
	_keyFocusBorder = NULL;
	m_bSelectionStateSaved = false;
	m_bStaySelectedOnClick = false;
	m_sArmedSoundName = UTL_INVAL_SYMBOL;
	m_sDepressedSoundName = UTL_INVAL_SYMBOL;
	m_sReleasedSoundName = UTL_INVAL_SYMBOL;
	SetTextInset(6, 0);
	SetMouseClickEnabled( MOUSE_LEFT, true );
	SetButtonActivationType(ACTIVATE_ONPRESSEDANDRELEASED);

	// labels have this off by default, but we need it on
	SetPaintBackgroundEnabled( true );

	_paint = true;

	REGISTER_COLOR_AS_OVERRIDABLE( _defaultFgColor, "defaultFgColor_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( _defaultBgColor, "defaultBgColor_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( _armedFgColor, "armedFgColor_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( _armedBgColor, "armedBgColor_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( _depressedFgColor, "depressedFgColor_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( _depressedBgColor, "depressedBgColor_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( _selectedFgColor, "selectedFgColor_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( _selectedBgColor, "selectedBgColor_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( _keyboardFocusColor, "keyboardFocusColor_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( _blinkFgColor, "blinkFgColor_override" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
Button::~Button()
{
	if (_actionMessage)
	{
		_actionMessage->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Button::SetButtonActivationType(ActivationType_t activationType)
{
	_activationType = activationType;
}

//-----------------------------------------------------------------------------
// Purpose: Set button border attribute enabled.
//-----------------------------------------------------------------------------
void Button::SetButtonBorderEnabled( bool state )
{
	if ( state != _buttonFlags.IsFlagSet( BUTTON_BORDER_ENABLED ) )
	{
		_buttonFlags.SetFlag( BUTTON_BORDER_ENABLED, state );
		InvalidateLayout(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose:	Set button selected state.
//-----------------------------------------------------------------------------
void Button::SetSelected( bool state )
{
	if ( _buttonFlags.IsFlagSet( SELECTED ) != state )
	{
		_buttonFlags.SetFlag( SELECTED, state );
		RecalculateDepressedState();
		InvalidateLayout(false);
	}

	if ( state && _buttonFlags.IsFlagSet( ARMED ) )
	{
		_buttonFlags.SetFlag( ARMED,  false );
		InvalidateLayout(false);
	}
}

void Button::SetBlink( bool state )
{
	if ( _buttonFlags.IsFlagSet( BLINK ) != state )
	{
		_buttonFlags.SetFlag( BLINK, state );
		RecalculateDepressedState();
		InvalidateLayout(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose:	Set button force depressed state.
//-----------------------------------------------------------------------------
void Button::ForceDepressed(bool state)
{
	if ( _buttonFlags.IsFlagSet( FORCE_DEPRESSED ) != state )
	{
		_buttonFlags.SetFlag( FORCE_DEPRESSED, state );
		RecalculateDepressedState();
		InvalidateLayout(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose:	Set button depressed state with respect to the force depressed state.
//-----------------------------------------------------------------------------
void Button::RecalculateDepressedState( void )
{
	bool newState;
	if (!IsEnabled())
	{
		newState = false;
	}
	else
	{
		if ( m_bStaySelectedOnClick && _buttonFlags.IsFlagSet( SELECTED ) )
		{
			newState = false;
		}
		else
		{
			newState = _buttonFlags.IsFlagSet( FORCE_DEPRESSED ) ? true : (_buttonFlags.IsFlagSet(ARMED) && _buttonFlags.IsFlagSet( SELECTED ) );
		}
	}

	_buttonFlags.SetFlag( DEPRESSED, newState );
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether or not the button captures all mouse input when depressed
//			Defaults to true
//			Should be set to false for things like menu items where there is a higher-level mouse capture
//-----------------------------------------------------------------------------
void Button::SetUseCaptureMouse( bool state )
{
	_buttonFlags.SetFlag( USE_CAPTURE_MOUSE, state );
}

//-----------------------------------------------------------------------------
// Purpose: Check if mouse capture is enabled.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool Button::IsUseCaptureMouseEnabled( void )
{
	return _buttonFlags.IsFlagSet( USE_CAPTURE_MOUSE );
}

//-----------------------------------------------------------------------------
// Purpose:	Set armed state.
//-----------------------------------------------------------------------------
void Button::SetArmed(bool state)
{
	if ( _buttonFlags.IsFlagSet( ARMED ) != state )
	{
		_buttonFlags.SetFlag( ARMED, state );
		RecalculateDepressedState();
		InvalidateLayout(false);

		// play any sounds specified
		if (state && m_sArmedSoundName != UTL_INVAL_SYMBOL)
		{
			surface()->PlaySound(g_ButtonSoundNames.String(m_sArmedSoundName));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:	Check armed state
//-----------------------------------------------------------------------------
bool Button::IsArmed()
{
	return _buttonFlags.IsFlagSet( ARMED );
}


KeyValues *Button::GetActionMessage()
{
	return _actionMessage->MakeCopy();
}

void Button::PlayButtonReleasedSound()
{
	// check for playing a transition sound
	if ( m_sReleasedSoundName != UTL_INVAL_SYMBOL )
	{
		surface()->PlaySound( g_ButtonSoundNames.String( m_sReleasedSoundName ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose:	Activate a button click.
//-----------------------------------------------------------------------------
void Button::DoClick()
{
	SetSelected(true);
	FireActionSignal();
	PlayButtonReleasedSound();

	static ConVarRef vgui_nav_lock( "vgui_nav_lock" );
	if ( ( !vgui_nav_lock.IsValid() || vgui_nav_lock.GetInt() == 0 ) && NavigateActivate() )
	{
		vgui_nav_lock.SetValue( 1 );
	}

	if ( !m_bStaySelectedOnClick )
	{
		SetSelected(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check selected state
//-----------------------------------------------------------------------------
bool Button::IsSelected()
{
	return _buttonFlags.IsFlagSet( SELECTED );
}

//-----------------------------------------------------------------------------
// Purpose:	Check depressed state
//-----------------------------------------------------------------------------
bool Button::IsDepressed()
{
	return _buttonFlags.IsFlagSet( DEPRESSED );
}

bool Button::IsBlinking( void )
{
	return _buttonFlags.IsFlagSet( BLINK );
}


//-----------------------------------------------------------------------------
// Drawing focus box?
//-----------------------------------------------------------------------------
bool Button::IsDrawingFocusBox()
{
	return _buttonFlags.IsFlagSet( DRAW_FOCUS_BOX );
}

void Button::DrawFocusBox( bool bEnable )
{
	_buttonFlags.SetFlag( DRAW_FOCUS_BOX, bEnable );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Button::NavigateTo()
{
	BaseClass::NavigateTo();

	SetArmed( true );

	if ( IsPC() )
	{
		RequestFocus( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Button::NavigateFrom()
{
	BaseClass::NavigateFrom();

	SetArmed( false );

	OnKeyCodeReleased( KEY_XBUTTON_A );
}
	
//-----------------------------------------------------------------------------
// Purpose:	Paint button on screen
//-----------------------------------------------------------------------------
void Button::Paint(void)
{
	if ( !ShouldPaint() )
		return; 

	BaseClass::Paint();

	if ( HasFocus() && IsEnabled() && IsDrawingFocusBox() )
	{
		int x0, y0, x1, y1;
		int wide, tall;
		GetSize(wide, tall);
		x0 = 3, y0 = 3, x1 = wide - 4 , y1 = tall - 2;
		DrawFocusBorder(x0, y0, x1, y1);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Perform graphical layout of button.
//-----------------------------------------------------------------------------
void Button::PerformLayout()
{
	// reset our border
	SetBorder( GetBorder(_buttonFlags.IsFlagSet( DEPRESSED ), _buttonFlags.IsFlagSet( ARMED ), _buttonFlags.IsFlagSet( SELECTED ), HasFocus() ) );

	// set our color
	SetFgColor(GetButtonFgColor());
	SetBgColor(GetButtonBgColor());

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Get button foreground color
// Output : Color
//-----------------------------------------------------------------------------
Color Button::GetButtonFgColor()
{
	if ( !_buttonFlags.IsFlagSet( BLINK ) )
	{
		if (_buttonFlags.IsFlagSet( DEPRESSED ))
			return _depressedFgColor;
		if (_buttonFlags.IsFlagSet( ARMED ))
			return _armedFgColor;
		if (_buttonFlags.IsFlagSet( SELECTED))
			return _selectedFgColor;
		return _defaultFgColor;
	}

	Color cBlendedColor;

	if (_buttonFlags.IsFlagSet( DEPRESSED ))
		cBlendedColor = _depressedFgColor;
	else if (_buttonFlags.IsFlagSet( ARMED ))
		cBlendedColor = _armedFgColor;
	else if (_buttonFlags.IsFlagSet( SELECTED ))
		cBlendedColor = _selectedFgColor;
	else
		cBlendedColor = _defaultFgColor;

	float fBlink = ( sinf( system()->GetTimeMillis() * 0.01f ) + 1.0f ) * 0.5f;

	if ( _buttonFlags.IsFlagSet( BLINK ) )
	{
		cBlendedColor[ 0 ] = (float)cBlendedColor[ 0 ] * fBlink + (float)_blinkFgColor[ 0 ] * ( 1.0f - fBlink );
		cBlendedColor[ 1 ] = (float)cBlendedColor[ 1 ] * fBlink + (float)_blinkFgColor[ 1 ] * ( 1.0f - fBlink );
		cBlendedColor[ 2 ] = (float)cBlendedColor[ 2 ] * fBlink + (float)_blinkFgColor[ 2 ] * ( 1.0f - fBlink );
		cBlendedColor[ 3 ] = (float)cBlendedColor[ 3 ] * fBlink + (float)_blinkFgColor[ 3 ] * ( 1.0f - fBlink );
	}

	return cBlendedColor;
}

//-----------------------------------------------------------------------------
// Purpose: Get button background color
//-----------------------------------------------------------------------------
Color Button::GetButtonBgColor()
{
	if (_buttonFlags.IsFlagSet( DEPRESSED ))
		return _depressedBgColor;
	if (_buttonFlags.IsFlagSet( ARMED ))
		return _armedBgColor;
	if (_buttonFlags.IsFlagSet( SELECTED ))
		return _selectedBgColor;
	return _defaultBgColor;
}

//-----------------------------------------------------------------------------
// Purpose: Called when key focus is received
//-----------------------------------------------------------------------------
void Button::OnSetFocus()
{
	InvalidateLayout(false);
	BaseClass::OnSetFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Respond when focus is killed
//-----------------------------------------------------------------------------
void Button::OnKillFocus()
{
	InvalidateLayout(false);
	BaseClass::OnKillFocus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Button::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// get the borders we need
	_defaultBorder = pScheme->GetBorder("ButtonBorder");
	_depressedBorder = pScheme->GetBorder("ButtonDepressedBorder");
	_keyFocusBorder = pScheme->GetBorder("ButtonKeyFocusBorder");

	_defaultFgColor = GetSchemeColor("Button.TextColor", Color(255, 255, 255, 255), pScheme);
	_defaultBgColor = GetSchemeColor("Button.BgColor", Color(0, 0, 0, 255), pScheme);

	_armedFgColor = GetSchemeColor("Button.ArmedTextColor", _defaultFgColor, pScheme);
	_armedBgColor = GetSchemeColor("Button.ArmedBgColor", _defaultBgColor, pScheme);

	_selectedFgColor = GetSchemeColor("Button.SelectedTextColor", _selectedFgColor, pScheme);
	_selectedBgColor = GetSchemeColor("Button.SelectedBgColor", _selectedBgColor, pScheme);

	_depressedFgColor = GetSchemeColor("Button.DepressedTextColor", _defaultFgColor, pScheme);
	_depressedBgColor = GetSchemeColor("Button.DepressedBgColor", _defaultBgColor, pScheme);
	_keyboardFocusColor = GetSchemeColor("Button.FocusBorderColor", Color(0,0,0,255), pScheme);

	_blinkFgColor = GetSchemeColor("Button.BlinkColor", Color(255, 155, 0, 255), pScheme);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Set default button colors.
//-----------------------------------------------------------------------------
void Button::SetDefaultColor(Color fgColor, Color bgColor)
{
	if (!(_defaultFgColor == fgColor && _defaultBgColor == bgColor))
	{
		_defaultFgColor = fgColor;
		_defaultBgColor = bgColor;

		InvalidateLayout(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set armed button colors
//-----------------------------------------------------------------------------
void Button::SetArmedColor(Color fgColor, Color bgColor)
{
	if (!(_armedFgColor == fgColor && _armedBgColor == bgColor))
	{
		_armedFgColor = fgColor;
		_armedBgColor = bgColor;

		InvalidateLayout(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set armed button colors
//-----------------------------------------------------------------------------
void Button::SetSelectedColor(Color fgColor, Color bgColor)
{
	if (!(_selectedFgColor == fgColor && _selectedBgColor == bgColor))
	{
		_selectedFgColor = fgColor;
		_selectedBgColor = bgColor;

		InvalidateLayout(false);
	}
}
//-----------------------------------------------------------------------------
// Purpose: Set depressed button colors
//-----------------------------------------------------------------------------
void Button::SetDepressedColor(Color fgColor, Color bgColor)
{
	if (!(_depressedFgColor == fgColor && _depressedBgColor == bgColor))
	{
		_depressedFgColor = fgColor;
		_depressedBgColor = bgColor;

		InvalidateLayout(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set blink button color
//-----------------------------------------------------------------------------
void Button::SetBlinkColor(Color fgColor)
{
	if (!(_blinkFgColor == fgColor))
	{
		_blinkFgColor = fgColor;

		InvalidateLayout(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set default button border attributes.
//-----------------------------------------------------------------------------
void Button::SetDefaultBorder(IBorder *border)
{
	_defaultBorder = border;
	InvalidateLayout(false);
}

//-----------------------------------------------------------------------------
// Purpose: Set depressed button border attributes.
//-----------------------------------------------------------------------------
void Button::SetDepressedBorder(IBorder *border)
{
	_depressedBorder = border;
	InvalidateLayout(false);
}

//-----------------------------------------------------------------------------
// Purpose: Set key focus button border attributes.
//-----------------------------------------------------------------------------
void Button::SetKeyFocusBorder(IBorder *border)
{
	_keyFocusBorder = border;
	InvalidateLayout(false);
}


//-----------------------------------------------------------------------------
// Purpose: Get button border attributes.
//-----------------------------------------------------------------------------
IBorder *Button::GetBorder(bool depressed, bool armed, bool selected, bool keyfocus)
{
	if ( _buttonFlags.IsFlagSet( BUTTON_BORDER_ENABLED ) )
	{
		// raised buttons with no armed state
		if (depressed)
			return _depressedBorder;
		if (keyfocus)
			return _keyFocusBorder;
		if (IsEnabled() && _buttonFlags.IsFlagSet( DEFAULT_BUTTON ))
			return _keyFocusBorder;
		return _defaultBorder;
	}
	else
	{
		// flat buttons that raise
		if (depressed)
			return _depressedBorder;
		if (armed)
			return _defaultBorder;
	}

	return _defaultBorder;
}

//-----------------------------------------------------------------------------
// Purpose: sets this button to be the button that is accessed by default 
//			when the user hits ENTER or SPACE
//-----------------------------------------------------------------------------
void Button::SetAsCurrentDefaultButton(int state)
{
	if ( _buttonFlags.IsFlagSet( DEFAULT_BUTTON ) != (bool)state )
	{
		_buttonFlags.SetFlag( DEFAULT_BUTTON, state );
		if (state)
		{
			// post a message up notifying our nav group that we're now the default button
			KeyValues *msg = new KeyValues( "CurrentDefaultButtonSet" );
			msg->SetInt( "button", ToHandle() );
			CallParentFunction( msg );
		}

		InvalidateLayout();
		Repaint();
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets this button to be the button that is accessed by default 
//			when the user hits ENTER or SPACE
//-----------------------------------------------------------------------------
void Button::SetAsDefaultButton(int state)
{
	if ( _buttonFlags.IsFlagSet( DEFAULT_BUTTON ) != (bool)state )
	{
		_buttonFlags.SetFlag( DEFAULT_BUTTON, state );
		if (state)
		{
			// post a message up notifying our nav group that we're now the default button
			KeyValues *msg = new KeyValues( "DefaultButtonSet" );
			msg->SetInt( "button", ToHandle() );
			CallParentFunction( msg );
		}

		InvalidateLayout();
		Repaint();
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets rollover sound
//-----------------------------------------------------------------------------
void Button::SetArmedSound(const char *sound)
{
	if (sound)
	{
		m_sArmedSoundName = g_ButtonSoundNames.AddString(sound);
	}
	else
	{
		m_sArmedSoundName = UTL_INVAL_SYMBOL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Button::SetDepressedSound(const char *sound)
{
	if (sound)
	{
		m_sDepressedSoundName = g_ButtonSoundNames.AddString(sound);
	}
	else
	{
		m_sDepressedSoundName = UTL_INVAL_SYMBOL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Button::SetReleasedSound(const char *sound)
{
	if (sound)
	{
		m_sReleasedSoundName = g_ButtonSoundNames.AddString(sound);
	}
	else
	{
		m_sReleasedSoundName = UTL_INVAL_SYMBOL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set button to be mouse clickable or not.
//-----------------------------------------------------------------------------
void Button::SetMouseClickEnabled(MouseCode code,bool state)
{
	if(state)
	{
		//set bit to 1
		_mouseClickMask|=1<<((int)(code+1));
	}
	else
	{
		//set bit to 0
		_mouseClickMask&=~(1<<((int)(code+1)));
	}	
}

//-----------------------------------------------------------------------------
// Purpose: Check if button is mouse clickable
//-----------------------------------------------------------------------------
bool Button::IsMouseClickEnabled(MouseCode code)
{
	if(_mouseClickMask&(1<<((int)(code+1))))
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: sets the command to send when the button is pressed
//-----------------------------------------------------------------------------
void Button::SetCommand( const char *command )
{
	SetCommand(new KeyValues("Command", "command", command));
}

//-----------------------------------------------------------------------------
// Purpose: sets the message to send when the button is pressed
//-----------------------------------------------------------------------------
void Button::SetCommand( KeyValues *message )
{
	// delete the old message
	if (_actionMessage)
	{
		_actionMessage->deleteThis();
	}

	_actionMessage = message;
}

//-----------------------------------------------------------------------------
// Purpose: Peeks at the message to send when button is pressed
// Input  :  - 
// Output : KeyValues
//-----------------------------------------------------------------------------
KeyValues *Button::GetCommand()
{
	return _actionMessage;
}

//-----------------------------------------------------------------------------
// Purpose: Message targets that the button has been pressed
//-----------------------------------------------------------------------------
void Button::FireActionSignal()
{
	// message-based action signal
	if (_actionMessage)
	{
		// see if it's a url
		if (!stricmp(_actionMessage->GetName(), "command")
			&& !strnicmp(_actionMessage->GetString("command", ""), "url ", strlen("url "))
			&& strstr(_actionMessage->GetString("command", ""), "://"))
		{
			// it's a command to launch a url, run it
			system()->ShellExecute("open", _actionMessage->GetString("command", "      ") + 4);
		}
		PostActionSignal(_actionMessage->MakeCopy());
	}
}

//-----------------------------------------------------------------------------
// Purpose: gets info about the button
//-----------------------------------------------------------------------------
bool Button::RequestInfo(KeyValues *outputData)
{
	if (!stricmp(outputData->GetName(), "CanBeDefaultButton"))
	{
		outputData->SetInt("result", CanBeDefaultButton() ? 1 : 0);
		return true;
	}
	else if (!stricmp(outputData->GetName(), "GetState"))
	{
		outputData->SetInt("state", IsSelected());
		return true;
	}
	else if ( !stricmp( outputData->GetName(), "GetCommand" ))
	{
		if ( _actionMessage )
		{
			outputData->SetString( "command", _actionMessage->GetString( "command", "" ) );
		}
		else
		{
			outputData->SetString( "command", "" );
		}
		return true;
	}


	return BaseClass::RequestInfo(outputData);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool Button::CanBeDefaultButton(void)
{
    return true;
}


//-----------------------------------------------------------------------------
// Purpose: Get control settings for editing
//-----------------------------------------------------------------------------
void Button::GetSettings( KeyValues *outResourceData )
{
	BaseClass::GetSettings(outResourceData);

	if (_actionMessage)
	{
		outResourceData->SetString("command", _actionMessage->GetString("command", ""));
	}
	outResourceData->SetInt("default", _buttonFlags.IsFlagSet( DEFAULT_BUTTON ) );
	if ( m_bSelectionStateSaved )
	{
		outResourceData->SetInt( "selected", IsSelected() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Button::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings(inResourceData);

	const char *cmd = inResourceData->GetString("command", "");
	if (*cmd)
	{
		// add in the command
		SetCommand(cmd);
	}

	// set default button state
	int defaultButton = inResourceData->GetInt("default");
	if (defaultButton && CanBeDefaultButton())
	{
		SetAsDefaultButton(true);
	}

	// saved selection state
	int iSelected = inResourceData->GetInt( "selected", -1 );
	if ( iSelected != -1 )
	{
		SetSelected( iSelected != 0 );
		m_bSelectionStateSaved = true;
	}

	m_bStaySelectedOnClick = inResourceData->GetBool( "stayselectedonclick", false );

	const char *sound = inResourceData->GetString("sound_armed", "");
	if (*sound)
	{
		SetArmedSound(sound);
	}
	sound = inResourceData->GetString("sound_depressed", "");
	if (*sound)
	{
		SetDepressedSound(sound);
	}
	sound = inResourceData->GetString("sound_released", "");
	if (*sound)
	{
		SetReleasedSound(sound);
	}

	_activationType = (ActivationType_t)inResourceData->GetInt( "button_activation_type", ACTIVATE_ONRELEASED );
}


//-----------------------------------------------------------------------------
// Purpose: Describes editing details
//-----------------------------------------------------------------------------
const char *Button::GetDescription( void )
{
	static char buf[1024];
	Q_snprintf(buf, sizeof(buf), "%s, string command, int default", BaseClass::GetDescription());
	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Button::OnSetState(int state)
{
	SetSelected((bool)state);
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Button::OnCursorEntered()
{
	if (IsEnabled() && !IsSelected() )
	{
		SetArmed( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Button::OnCursorExited()
{
	if ( !_buttonFlags.IsFlagSet( BUTTON_KEY_DOWN ) && !IsSelected() )
	{
		SetArmed( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Button::OnMousePressed(MouseCode code)
{
	if (!IsEnabled())
		return;
	
	if (!IsMouseClickEnabled(code))
		return;

	if (_activationType == ACTIVATE_ONPRESSED)
	{
		if ( IsKeyBoardInputEnabled() )
		{
			RequestFocus();
		}
		DoClick();
		return;
	}

	// play activation sound
	if (m_sDepressedSoundName != UTL_INVAL_SYMBOL)
	{
		surface()->PlaySound(g_ButtonSoundNames.String(m_sDepressedSoundName));
	}

	if (IsUseCaptureMouseEnabled() && _activationType == ACTIVATE_ONPRESSEDANDRELEASED)
	{
		{
			if ( IsKeyBoardInputEnabled() )
			{
				RequestFocus();
			}
			SetSelected(true);
			Repaint();
		}

		// lock mouse input to going to this button
		input()->SetMouseCapture(GetVPanel());
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Button::OnMouseDoublePressed(MouseCode code)
{
	OnMousePressed(code);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Button::OnMouseReleased(MouseCode code)
{
	// ensure mouse capture gets released
	if (IsUseCaptureMouseEnabled())
	{
		input()->SetMouseCapture(NULL);
	}

	if (_activationType == ACTIVATE_ONPRESSED)
		return;

	if (!IsMouseClickEnabled(code))
		return;

	if (!IsSelected() && _activationType == ACTIVATE_ONPRESSEDANDRELEASED)
		return;

	// it has to be both enabled and (mouse over the button or using a key) to fire
	if ( IsEnabled() && ( GetVPanel() == input()->GetMouseOver() || _buttonFlags.IsFlagSet( BUTTON_KEY_DOWN ) ) )
	{
		DoClick();
	}
	else if ( !m_bStaySelectedOnClick )
	{
		SetSelected(false);
	}

	// make sure the button gets unselected
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Button::OnKeyCodePressed(KeyCode code)
{
	KeyCode localCode = GetBaseButtonCode( code );

	if( ( localCode == KEY_XBUTTON_A ) && IsEnabled() )
	{
		SetArmed( true );
		_buttonFlags.SetFlag( BUTTON_KEY_DOWN );
		if( _activationType != ACTIVATE_ONRELEASED )
		{
			DoClick();
		}
	}
	else if (code == KEY_SPACE || code == KEY_ENTER)
	{
		SetArmed(true);
		_buttonFlags.SetFlag( BUTTON_KEY_DOWN );
		OnMousePressed(MOUSE_LEFT);
		if (IsUseCaptureMouseEnabled()) // undo the mouse capture since its a fake mouse click!
		{
			input()->SetMouseCapture(NULL);
		}
	}
	else
	{
		_buttonFlags.ClearFlag( BUTTON_KEY_DOWN );
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Button::OnKeyCodeReleased( KeyCode keycode )
{
	vgui::KeyCode code = GetBaseButtonCode( keycode );

	if ( _buttonFlags.IsFlagSet( BUTTON_KEY_DOWN ) && ( code == KEY_XBUTTON_A || code == KEY_XBUTTON_START ) )
	{
		SetArmed( true );
		if( _activationType != ACTIVATE_ONPRESSED )
		{
			DoClick();
		}
	}
	else if (_buttonFlags.IsFlagSet( BUTTON_KEY_DOWN ) && (code == KEY_SPACE || code == KEY_ENTER))
	{
		SetArmed(true);
		OnMouseReleased(MOUSE_LEFT);
	}
	else
	{
		BaseClass::OnKeyCodeReleased( keycode );
	}
	_buttonFlags.ClearFlag( BUTTON_KEY_DOWN );

	if ( !( code == KEY_XSTICK1_UP || code == KEY_XSTICK1_DOWN || code == KEY_XSTICK1_LEFT || code == KEY_XSTICK1_RIGHT || 
			code == KEY_XSTICK2_UP || code == KEY_XSTICK2_DOWN || code == KEY_XSTICK2_LEFT || code == KEY_XSTICK2_RIGHT || 
			code == KEY_XBUTTON_UP || code == KEY_XBUTTON_DOWN || code == KEY_XBUTTON_LEFT || code == KEY_XBUTTON_RIGHT || 
			keycode == KEY_UP|| keycode == KEY_DOWN || keycode == KEY_LEFT || keycode == KEY_RIGHT ) )
	{
		SetArmed( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override this to draw different focus border
//-----------------------------------------------------------------------------
void Button::DrawFocusBorder(int tx0, int ty0, int tx1, int ty1)
{
	surface()->DrawSetColor(_keyboardFocusColor);
	DrawDashedLine(tx0, ty0, tx1, ty0+1, 1, 1);		// top
	DrawDashedLine(tx0, ty0, tx0+1, ty1, 1, 1);		// left
	DrawDashedLine(tx0, ty1-1, tx1, ty1, 1, 1);		// bottom
	DrawDashedLine(tx1-1, ty0, tx1, ty1, 1, 1);		// right
}

//-----------------------------------------------------------------------------
// Purpose: Size the object to its button and text.  - only works from in ApplySchemeSettings or PerformLayout()
//-----------------------------------------------------------------------------
void Button::SizeToContents()
{
	int wide, tall;
	GetContentSize(wide, tall);
	SetSize(wide + Label::Content, tall + Label::Content);
}
