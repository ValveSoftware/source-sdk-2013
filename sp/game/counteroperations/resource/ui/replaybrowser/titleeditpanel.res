"Resource/UI/TitleEditPanel.res"
{
	"TitleEditPanel"
	{
		"ControlName"	"CTitleEditPanel"
		"fieldName"		"TitleEditPanel"
		"xpos"			"0"
		"ypos"			"0"
		"wide"			"f0"
		"tall"			"40"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
	}
	
	"CaratLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"CaratLabel"
		"font"			"ReplayMediumSmall"
		"labelText"		">>"
		"textAlignment"	"west"
		"xpos"			"0"
		"ypos"			"6"
		"zpos"			"10"
		"wide"			"20"
		"tall"			"15"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"fgcolor"		"ReplayBrowser.Details.TitleEdit.Carat.FgColor"
	}
	
	"TitleInput"
	{
		"ControlName"	"TextEntry"
		"fieldName"		"TitleInput"
		"maxchars"		"255"
		"textHidden"	"0"
		"textAlignment"	"west"
		"unicode"		"1"
		"wrap"			"0"
		"xpos"			"15"
		"ypos"			"0"
		"zpos"			"1"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"fgcolor_override"		"202 190 164 255"
		"bgcolor_override"		"0 0 0 0"		// Background is drawn explicitly in CTitleEditPanel::PaintBackground()
		"Font"			"ReplayLarger"
	}
	
	"HeaderLine"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"HeaderLine"
		"xpos"			"0"
		"zpos"			"5"
		"wide"			"586"
		"tall"			"10"
		"visible"		"1"
		"enabled"		"1"
		"image"			"replay/replaybrowser_dottedline"
		"scaleImage"	"0"
		"tileHorizontally" "1"
	}
			
	"ClickToEditLabel"
	{
		"ControlName"		"CExLabel"
		"FieldName"			"ClickToEditLabel"
		"LabelText"			"#Replay_ClickToEdit"
		"zpos"				"1000"
		"font"				"ReplayBrowserSmallest"
		"Visible"			"1"
		"fgcolor_override" "118 106 94 255"
	}
}