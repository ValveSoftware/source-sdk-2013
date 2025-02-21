"Resource/UI/econ/ScrollableQuestList.res"
{
	"Container"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"Container"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"1"
		"wide"			"f0"
		"tall"			"f0"
		"visible"		"1"
		"enabled"		"1"
		"proportionaltoparent" "1"

		"ItemAd"
		{
			"ControlName"			"CCyclingAdContainerPanel"
			"fieldName"				"ItemAd"
			"xpos"					"cs-0.5"
			"ypos"					"r80"
			"zpos"					"100"
			"wide"					"p0.9"
			"tall"					"60"
			"visible"				"0"
			"enabled"				"1"
			"scaleImage"			"1"
			"proportionaltoparent"	"1"

			"bgcolor_override"		"0 0 0 255"

			"items"
			{
				"0"
				{
					"item"		"Unused Summer 2015 Operation Pass"
					"show_market"	"0"
				}
			}
		}

		"SelectButton"
		{
			"ControlName"	"CExButton"
			"fieldName"		"SelectButton"
			"xpos"		"0"
			"ypos"		"0"
			"zpos"		"0"
			"wide"		"f0"
			"tall"		"f0"
			"visible"		"1"
			"enabled"		"1"
			"textAlignment"	"east"
			"labelText"			""

			"proportionaltoparent"	"1"
			"paintbackground"	"0"
			"command"	"deselect_all"
			"actionsignallevel" "2"
		}
	}

	"BackgroundFolderImage"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"BackgroundFolderImage"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"-100"
		"wide"			"f0"
		"tall"			"o1"
		"visible"		"1"
		"enabled"		"1"
		"scaleImage"	"1"
		"image"			"contracts\contracts_base1"
		"proportionaltoparent"	"1"

		"mouseinputenabled"	"0"
		"keyboardinputenabled" "0"
	}

	"EmptyLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"EmptyLabel"
		"font"			"HudFontSmallBold"
		"labelText"		"%noquests%"
		"xpos"			"cs-0.5"
		"ypos"			"p0.5"
		"zpos"			"20"
		"wide"			"p1"
		"tall"			"64"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"centerwrap"	"1"
		"proportionaltoparent"	"1"
		"mouseinputenabled"	"0"
	}
}
