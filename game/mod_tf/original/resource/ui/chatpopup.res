"Resource/UI/ChatPopup.res"
{
	"ChatPopup"
	{
		"fieldName"		"ChatPopup"
		"zpos"			"1"
		"wide"			"200"
		"tall"			"30"
		"visible"		"1"
		"proportionaltoparent"	"1"

		"paintbackground"	"0"
		"bgcolor_override"	"255 0 0 250"
	}

	"text"
	{
		"fieldName"		"text"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"1"
		"wide"			"200"
		"tall"			"30"
		"visible"		"1"
		"proportionaltoparent"	"1"
		"RoundedCorners"	"0"
		"font"			"HudFontSmallest"
		"keyboardinputenabled"	"0"
		"mouseinputenabled"		"0"
		"skip_autoresize"	"1"

		"bgcolor_override"	"0 0 0 250"

		"paintbackground"	"1"
	
		"ScrollBar"
		{
			"FieldName"		"ScrollBar"
			"xpos"			"rs1-1"
			"ypos"			"0"
			"tall"			"f0"
			"wide"			"5" // This gets slammed from client schme.  GG.
			"zpos"			"1000"
			"nobuttons"		"1"
			"proportionaltoparent"	"1"

			"Slider"
			{
				"fgcolor_override"	"TanLight"
			}
		
			"UpButton"
			{
				"ControlName"	"Button"
				"FieldName"		"UpButton"
				"visible"		"0"
			}
		
			"DownButton"
			{
				"ControlName"	"Button"
				"FieldName"		"DownButton"
				"visible"		"0"
			}
		}
	}
}
