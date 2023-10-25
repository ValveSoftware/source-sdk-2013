static const char* g_Script_vgui_init = R"script(
local DoCreateFont = ISurface.CreateFont;
function ISurface::CreateFont( name, props )
{
	if ( !("name" in props) || typeof props.name != "string" )
		throw "invalid parameter 'name'";

	if ( !("tall" in props) || typeof props.tall != "integer" || !props.tall )
		throw "invalid parameter 'tall'";

	if ( !("weight" in props) || typeof props.weight != "integer" )
		throw "invalid parameter 'weight'";

	local yres_min = 0, yres_max = 0;

	if ( "yres" in props && typeof props.yres == "string" )
	{
		local ss = ::split( props.yres, " " );
		try
		{
			yres_min = ss[0].tointeger();
			yres_max = ss[1].tointeger();
		}
		catch(x)
		{
			throw "invalid parameter 'yres'";
		}
	}

	if ( ( (!("proportional" in props) || typeof props.proportional != "bool") ) && !yres_min )
	{
		throw "parameter 'proportional' or 'yres' not found";
	}
	else if ( "proportional" in props && props.proportional && yres_min )
	{
		throw "resolution definition on a proportional font"
	}

	local blur = 0, scanlines = 0, proportional = false, flags = 0;

	if ( "blur" in props && typeof props.blur == "integer" )
		blur = props.blur;

	if ( "scanlines" in props && typeof props.scanlines == "integer" )
		scanlines = props.scanlines;

	if ( "proportional" in props && typeof props.proportional == "bool" )
		proportional = props.proportional;

	if ( "italic" in props && props.italic == true )
		flags = flags | 0x001;

	if ( "underline" in props && props.underline == true )
		flags = flags | 0x002;

	if ( "strikeout" in props && props.strikeout == true )
		flags = flags | 0x004;

	if ( "symbol" in props && props.symbol == true )
		flags = flags | 0x008;

	if ( "antialias" in props && props.antialias == true )
		flags = flags | 0x010;

	if ( "gaussianblur" in props && props.gaussianblur == true )
		flags = flags | 0x020;

	if ( "rotary" in props && props.rotary == true )
		flags = flags | 0x040;

	if ( "dropshadow" in props && props.dropshadow == true )
		flags = flags | 0x080;

	if ( "additive" in props && props.additive == true )
		flags = flags | 0x100;

	if ( "outline" in props && props.outline == true )
		flags = flags | 0x200;

	if ( "custom" in props && props.custom == true )
		flags = flags | 0x400;

	if ( "bitmap" in props && props.bitmap == true )
		flags = flags | 0x800;

	return DoCreateFont( name, props.name, props.tall, props.weight, blur, scanlines, flags, yres_min, yres_max, proportional );
}

local _FontTall = {}
local _Schemes = {}
local DoGetFont = ISurface.DoGetFont <- ISurface.GetFont;
local DoGetFontTall = ISurface.GetFontTall;

function ISurface::GetFont( name, proportional, sch = "" )
{
	if ( sch in _Schemes )
	{
		local fonts = _Schemes[sch][proportional.tointeger()];
		if ( name in fonts )
			return fonts[name];
	}
	else
	{
		if ( typeof sch != "string" )
			throw "invalid parameter 'scheme'";
		_Schemes[sch] <- [{}, {}];
	}

	local id = DoGetFont( name, proportional, sch );
	if ( id > 0 )
		_Schemes[sch][proportional.tointeger()][name] <- id;

	return id;
}

ISurface.GetFontTall <- function( id )
{
	if ( id in _FontTall )
		return _FontTall[id];
	return _FontTall[id] <- DoGetFontTall( id );
}

local _Textures = {}
local DoGetTextureID = ISurface.GetTextureID;
local DoValidateTexture = ISurface.ValidateTexture;
local DoSetTextureFile = ISurface.SetTextureFile;

ISurface.ValidateTexture <- function( filename, hardwareFilter, forceReload = false, procedural = false )
{
	return DoValidateTexture( filename, hardwareFilter, forceReload, procedural );
}

ISurface.SetTextureFile <- function( id, filename, hardwareFilter )
{
	if ( filename in _Textures )
		delete _Textures[filename];

	return DoSetTextureFile( id, filename, hardwareFilter );
}

ISurface.GetTextureID <- function( name )
{
	if ( name in _Textures )
		return _Textures[name];

	local id = DoGetTextureID( name );
	if ( id > 0 )
		_Textures[name] <- id;

	return id;
}

// Forward compatibility
IVGui.GetRootPanel <- function() { return 0x8888 }
//IVGui.GetGameUIRootPanel <- function() { return 0x8888+1 }
IVGui.GetClientDLLRootPanel <- function() { return 0x8888+2 }
IVGui.GetHudViewport <- function() { return 0x8888+10 }

local CreatePanel = IVGui.CreatePanel;
function IVGui::CreatePanel( type, parent, name )
{
	if ( !parent )
		throw "invalid parent";

	local root = -1;
	if ( typeof parent == "integer" )
	{
		root = parent-0x8888;
		switch ( root )
		{
			case 0:
			case 2:
			case 10:
				break;
			default: throw "invalid parent";
		}
		parent = null;
	}
	return CreatePanel( type, parent, name, root );
}

ISurface.__OnScreenSizeChanged <- function()
{
	_FontTall.clear();
}

// MAX_JOYSTICKS = 1 // ( 1 << MAX_SPLITSCREEN_CLIENT_BITS )
// MAX_JOYSTICK_AXES = 6 // X,Y,Z,R,U,V
// JOYSTICK_MAX_BUTTON_COUNT = 32
// JOYSTICK_POV_BUTTON_COUNT = 4
// JOYSTICK_AXIS_BUTTON_COUNT = MAX_JOYSTICK_AXES * 2

enum ButtonCode
{
	KEY_FIRST = 0
	KEY_0 = 1
	KEY_1 = 2
	KEY_2 = 3
	KEY_3 = 4
	KEY_4 = 5
	KEY_5 = 6
	KEY_6 = 7
	KEY_7 = 8
	KEY_8 = 9
	KEY_9 = 10
	KEY_A = 11
	KEY_B = 12
	KEY_C = 13
	KEY_D = 14
	KEY_E = 15
	KEY_F = 16
	KEY_G = 17
	KEY_H = 18
	KEY_I = 19
	KEY_J = 20
	KEY_K = 21
	KEY_L = 22
	KEY_M = 23
	KEY_N = 24
	KEY_O = 25
	KEY_P = 26
	KEY_Q = 27
	KEY_R = 28
	KEY_S = 29
	KEY_T = 30
	KEY_U = 31
	KEY_V = 32
	KEY_W = 33
	KEY_X = 34
	KEY_Y = 35
	KEY_Z = 36
	KEY_PAD_0 = 37
	KEY_PAD_1 = 38
	KEY_PAD_2 = 39
	KEY_PAD_3 = 40
	KEY_PAD_4 = 41
	KEY_PAD_5 = 42
	KEY_PAD_6 = 43
	KEY_PAD_7 = 44
	KEY_PAD_8 = 45
	KEY_PAD_9 = 46
	KEY_PAD_DIVIDE = 47
	KEY_PAD_MULTIPLY = 48
	KEY_PAD_MINUS = 49
	KEY_PAD_PLUS = 50
	KEY_PAD_ENTER = 51
	KEY_PAD_DECIMAL = 52
	KEY_LBRACKET = 53
	KEY_RBRACKET = 54
	KEY_SEMICOLON = 55
	KEY_APOSTROPHE = 56
	KEY_BACKQUOTE = 57
	KEY_COMMA = 58
	KEY_PERIOD = 59
	KEY_SLASH = 60
	KEY_BACKSLASH = 61
	KEY_MINUS = 62
	KEY_EQUAL = 63
	KEY_ENTER = 64
	KEY_SPACE = 65
	KEY_BACKSPACE = 66
	KEY_TAB = 67
	KEY_CAPSLOCK = 68
	KEY_NUMLOCK = 69
	KEY_ESCAPE = 70
	KEY_SCROLLLOCK = 71
	KEY_INSERT = 72
	KEY_DELETE = 73
	KEY_HOME = 74
	KEY_END = 75
	KEY_PAGEUP = 76
	KEY_PAGEDOWN = 77
	KEY_BREAK = 78
	KEY_LSHIFT = 79
	KEY_RSHIFT = 80
	KEY_LALT = 81
	KEY_RALT = 82
	KEY_LCONTROL = 83
	KEY_RCONTROL = 84
	KEY_LWIN = 85
	KEY_RWIN = 86
	KEY_APP = 87
	KEY_UP = 88
	KEY_LEFT = 89
	KEY_DOWN = 90
	KEY_RIGHT = 91
	KEY_F1 = 92
	KEY_F2 = 93
	KEY_F3 = 94
	KEY_F4 = 95
	KEY_F5 = 96
	KEY_F6 = 97
	KEY_F7 = 98
	KEY_F8 = 99
	KEY_F9 = 100
	KEY_F10 = 101
	KEY_F11 = 102
	KEY_F12 = 103
	KEY_CAPSLOCKTOGGLE = 104
	KEY_NUMLOCKTOGGLE = 105
	KEY_SCROLLLOCKTOGGLE = 106
	KEY_LAST = 106

	MOUSE_FIRST = 107
	MOUSE_LEFT = 107
	MOUSE_RIGHT = 108
	MOUSE_MIDDLE = 109
	MOUSE_4 = 110
	MOUSE_5 = 111
	MOUSE_WHEEL_UP = 112
	MOUSE_WHEEL_DOWN = 113
	MOUSE_LAST = 113

	JOYSTICK_FIRST = 114
	JOYSTICK_FIRST_BUTTON = 114
	JOYSTICK_LAST_BUTTON = 145
	JOYSTICK_FIRST_POV_BUTTON = 146
	JOYSTICK_LAST_POV_BUTTON = 149
	JOYSTICK_FIRST_AXIS_BUTTON = 150
	JOYSTICK_LAST_AXIS_BUTTON = 161
	JOYSTICK_LAST = 161
}

enum AnalogCode
{
	MOUSE_X = 0
	MOUSE_Y = 1
	MOUSE_XY = 2
	MOUSE_WHEEL = 3

	JOYSTICK_FIRST_AXIS = 4
	JOYSTICK_LAST_AXIS = 9
}

enum CursorCode
{
	dc_none = 1
	dc_arrow = 2
	dc_ibeam = 3
	dc_hourglass = 4
	dc_waitarrow = 5
	dc_crosshair = 6
	dc_up = 7
	dc_sizenwse = 8
	dc_sizenesw = 9
	dc_sizewe = 10
	dc_sizens = 11
	dc_sizeall = 12
	dc_no = 13
	dc_hand = 14
	dc_blank = 15
}

enum Alignment
{
	northwest = 0
	north = 1
	northeast = 2
	west = 3
	center = 4
	east = 5
	southwest = 6
	south = 7
	southeast = 8
}

if ( __Documentation.RegisterHelp != dummy )
{
	local RegEnum = function( e )
	{
		local K = getconsttable()[e];
		__Documentation.RegisterEnumHelp( e, K.len(), "" );
		e += ".";
		foreach ( s, v in K )
		{
			__Documentation.RegisterConstHelp( e+s, v, "" );
		}
	}
	RegEnum( "ButtonCode" );
	RegEnum( "AnalogCode" );
	RegEnum( "CursorCode" );
	RegEnum( "Alignment" );

	__Documentation.RegisterHelp( "ISurface::CreateFont", "void ISurface::CreateFont(string, handle)", "" );
	__Documentation.RegisterHelp( "IVGui::CreatePanel", "handle IVGui::CreatePanel(string, handle, string)", "" );
	__Documentation.RegisterHelp( "IVGui::GetRootPanel", "handle IVGui::GetRootPanel()", "" );
	__Documentation.RegisterHelp( "IVGui::GetClientDLLRootPanel", "handle IVGui::GetClientDLLRootPanel()", "" );
	__Documentation.RegisterHelp( "IVGui::GetHudViewport", "handle IVGui::GetHudViewport()", "" );
}
)script";
