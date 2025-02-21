"obj_dispenser.res"
{

//	"Background"
//	{
//		"ControlName"	"MaterialImage"
//		"fieldName"		"Background"
//		"xpos"			"0"
//		"ypos"			"0"
//		"zpos"			"-2"
//		"wide"			"480"
//		"tall"			"240"

//		"material"		"vgui/screens/vgui_bg"
//	}

	"OwnerReadout"
	{
		"ControlName"	"Label"
		"fieldName"		"OwnerReadout"
		"xpos"			"10"
		"ypos"			"20"
		"wide"			"240"
		"tall"			"34"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		"No Owner"
		"textAlignment"	"center"
		"dulltext"		"0"
		"paintBackground" "0"
	}
	
	"ShellsReadout"
	{
		"ControlName"	"Label"
		"fieldName"		"ShellsReadout"
		"xpos"			"10"
		"ypos"			"125"
		"wide"			"240"
		"tall"			"34"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		"Shells:  0 / 0"
		"textAlignment"	"center"
		"dulltext"		"0"
		"paintBackground" "0"
	}
	
	"UpgradeProgress"
	{
		"ControlName"	"ProgressBar"
		"fieldName"		"UpgradeProgress"
		"xpos"		"40"
		"ypos"		"90"
		"wide"		"400"
		"tall"		"20"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"progress"		"0.0"
	}
		
	"RocketsReadout"
	{
		"ControlName"	"Label"
		"fieldName"		"RocketsReadout"
		"xpos"			"240"
		"ypos"			"125"
		"wide"			"240"
		"tall"			"34"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		"Rockets:  0 / 0"
		"textAlignment"	"center"
		"dulltext"		"0"
		"paintBackground" "0"
	}
	
	"RotateLeftButton"
	{
		"ControlName"	"MaterialButton"
		"fieldName"		"RotateLeftButton"
		"xpos"			"30"
		"ypos"			"50"
		"wide"			"115"
		"tall"			"32"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"2"
		"labelText"		"Rotate Left 45"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"Default"		"0"
		"command"		"rotateleft"
		"paintborder"	"0"

		"enabledImage"
		{
			"material"	"vgui/screens/vgui_button_enabled"
			"color" "255 255 255 255"
		}

		"mouseOverImage"
		{
			"material"	"vgui/screens/vgui_button_hover"
			"color" "255 255 255 255"
		}

		"pressedImage"
		{
			"material"	"vgui/screens/vgui_button_pushed"
			"color" "255 255 255 255"
		}

		"disabledImage"
		{
			"material"	"vgui/screens/vgui_button_disabled"
			"color" "255 255 255 255"
		}
	}

	"Rotate180Button"
	{
		"ControlName"	"MaterialButton"
		"fieldName"		"Rotate180Button"
		"xpos"			"180"
		"ypos"			"50"
		"wide"			"120"
		"tall"			"32"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"2"
		"labelText"		"Rotate 180"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"Default"		"0"
		"command"		"rotate180"
		"paintborder"	"0"

		"enabledImage"
		{
			"material"	"vgui/screens/vgui_button_enabled"
			"color" "255 255 255 255"
		}

		"mouseOverImage"
		{
			"material"	"vgui/screens/vgui_button_hover"
			"color" "255 255 255 255"
		}

		"pressedImage"
		{
			"material"	"vgui/screens/vgui_button_pushed"
			"color" "255 255 255 255"
		}

		"disabledImage"
		{
			"material"	"vgui/screens/vgui_button_disabled"
			"color" "255 255 255 255"
		}
	}	
	
	"RotateRightButton"
	{
		"ControlName"	"MaterialButton"
		"fieldName"		"RotateRightButton"
		"xpos"			"325"
		"ypos"			"50"
		"wide"			"115"
		"tall"			"32"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"2"
		"labelText"		"Rotate Right 45"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"Default"		"0"
		"command"		"rotateright"
		"paintborder"	"0"

		"enabledImage"
		{
			"material"	"vgui/screens/vgui_button_enabled"
			"color" "255 255 255 255"
		}

		"mouseOverImage"
		{
			"material"	"vgui/screens/vgui_button_hover"
			"color" "255 255 255 255"
		}

		"pressedImage"
		{
			"material"	"vgui/screens/vgui_button_pushed"
			"color" "255 255 255 255"
		}

		"disabledImage"
		{
			"material"	"vgui/screens/vgui_button_disabled"
			"color" "255 255 255 255"
		}
	}
	
	"HealthReadout"
	{
		"ControlName"	"Label"
		"fieldName"		"HealthReadout"
		"xpos"			"240"
		"ypos"			"20"
		"wide"			"240"
		"tall"			"34"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		"Health: 100%"
		"textAlignment"	"center"
		"dulltext"		"0"
		"paintBackground" "0"
	}

	"DismantleButton"
	{
		"ControlName"	"MaterialButton"
		"fieldName"		"Dismantle"
		"xpos"			"78"
		"ypos"			"160"
		"wide"			"324"
		"tall"			"48"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"2"
		"labelText"		"Dismantle"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"Default"		"0"
		"command"		"Dismantle"
		"paintborder"	"0"

		"enabledImage"
		{
			"material"	"vgui/screens/vgui_button_enabled"
			"color" "255 255 255 255"
		}

		"mouseOverImage"
		{
			"material"	"vgui/screens/vgui_button_hover"
			"color" "255 255 255 255"
		}

		"pressedImage"
		{
			"material"	"vgui/screens/vgui_button_pushed"
			"color" "255 255 255 255"
		}

		"disabledImage"
		{
			"material"	"vgui/screens/vgui_button_disabled"
			"color" "255 255 255 255"
		}
	}

	"DismantlingPanel"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"DismantlingPanel"

		"DismantleMessage"
		{
			"ControlName"	"Label"
			"fieldName"		"DismantleMessage"
			"xpos"			"70"
			"ypos"			"82"

			"wide"			"182"
			"tall"			"34"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"	"0"
			"labelText"		"Gone in"
			"textAlignment"	"center"
			"dulltext"		"0"
			"paintBackground" "0"
		}

		"DismantleTime"
		{
			"ControlName"	"Label"
			"fieldName"		"DismantleTime"
			"xpos"			"222"
			"ypos"			"82"
			"wide"			"182"
			"tall"			"34"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"	"0"
			"labelText"		"5 seconds"
			"textAlignment"	"center"
			"dulltext"		"0"
			"paintBackground" "0"
		}

		"CancelDismantleButton"
		{
			"ControlName"	"MaterialButton"
			"fieldName"		"CancelDismantleButton"
			"xpos"			"78"
			"ypos"			"160"
			"wide"			"324"
			"tall"			"48"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"	"2"
			"labelText"		"Cancel"
			"textAlignment"	"center"
			"dulltext"		"0"
			"brighttext"	"0"
			"Default"		"0"
			"command"		"CancelDismantle"
			"paintborder"	"0"

			"enabledImage"
			{
				"material"	"vgui/screens/vgui_button_enabled"
				"color" "255 255 255 255"
			}
	
			"mouseOverImage"
			{
				"material"	"vgui/screens/vgui_button_hover"
				"color" "255 255 255 255"
			}
	
			"pressedImage"
			{
				"material"	"vgui/screens/vgui_button_pushed"
				"color" "255 255 255 255"
			}
	
			"disabledImage"
			{
				"material"	"vgui/screens/vgui_button_disabled"
				"color" "255 255 255 255"
			}
		}
	}

	"DeterioratingPanel"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"DeterioratingPanel"

		"AssumeControl"
		{
			"ControlName"	"MaterialButton"
			"fieldName"		"AssumeControl"
			"xpos"			"20"
			"ypos"			"108"
			"wide"			"200"
			"tall"			"44"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"	"2"
			"labelText"		"Buy"
			"textAlignment"	"center"
			"dulltext"		"0"
			"brighttext"	"0"
			"Default"		"0"
			"command"		"AssumeControl"
			"paintborder"	"0"

			"enabledImage"
			{
				"material"	"vgui/screens/vgui_button_enabled"
				"color" "255 255 255 255"
			}
	
			"mouseOverImage"
			{
				"material"	"vgui/screens/vgui_button_hover"
				"color" "255 255 255 255"
			}
	
			"pressedImage"
			{
				"material"	"vgui/screens/vgui_button_pushed"
				"color" "255 255 255 255"
			}
	
			"disabledImage"
			{
				"material"	"vgui/screens/vgui_button_disabled"
				"color" "255 255 255 255"
			}
		}
	}
}
