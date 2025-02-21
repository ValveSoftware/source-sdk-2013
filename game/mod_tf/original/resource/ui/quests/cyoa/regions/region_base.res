"Resource/UI/quests/CYOA/regions/region_base.res"
{
	"Region"
	{
		"fieldName"		"Region"
		"xpos"			"0"
		"ypos"			"0"
		"wide"			"p1"
		"tall"			"p1"
		"proportionaltoparent" "1"

		"RegionName"
		{
			"ControlName"	"Label"
			"fieldName"		"RegionName"
			"labeltext"		"%region_name%"
			"xpos"			"cs-0.5"
			"ypos"			"15"
			"wide"			"f0"
			"tall"			"30"
			"zpos"			"100"
			"font"			"QuestLargeText"
			"TextAlignment"		"north"
			"proportionaltoparent" "1"
			"mouseinputenabled"	"0"
		}

		"ReturnButton"
		{
			"ControlName"	"Button"
			"fieldName"		"ReturnButton"
			"labeltext"		"#TF_QuestMap_Back"
			"xpos"			"10"
			"ypos"			"15"
			"wide"			"80"
			"tall"			"30"
			"zpos"			"101"
			"font"			"QuestLargeText"
			"TextAlignment"		"north-west"
			"proportionaltoparent" "1"
			"mouseinputenabled"	"1"
			"paintbackground"	"0"
			"armedFgColor_override"	"QuestMap_ActiveOrange"
			"command"	"back"
		}

		"BGColor"
		{
			"ControlName"	"Panel"
			"fieldName"		"BGColor"
			"xpos"			"0"
			"ypos"			"0"
			"zpos"			"-1"
			"wide"			"f0"
			"tall"			"f0"
			"visible"		"1"
			"enabled"		"1"
			"scaleImage"	"1"
			"image"			"cyoa\cyoa_map_lvl2_bg_a"
			"proportionaltoparent"	"1"

			"mouseinputenabled"	"0"
			"keyboardinputenabled" "	0"
			"bgcolor_override"	"43 43 43 255"
			"paintbackground"	"1"
		}

		"ZoomPanel"
		{
			"fieldName"		"ZoomPanel"
			"xpos"			"0"
			"ypos"			"0"
			"zpos"			"1"
			"wide"			"f0"
			"tall"			"f0"
			"visible"		"1"
			"proportionaltoparent" "1"
			"skip_autoresize"	"1"
			"max_zoom"		"5"
			"zoom"			"2.5"

			"PathsPanel"
			{
				// VGUI will delete this and we have pointers, so create in the code
				// "ControlName"	"CQuestMapPathsPanel"
				"fieldName"		"PathsPanel"
				"xpos"			"0"
				"ypos"			"0"
				"zpos"			"2"
				"wide"			"f0"
				"tall"			"f0"
				"visible"		"1"
				"enabled"		"1"
				"proportionaltoparent" "1"
				"mouseinputenabled"		"0"
				"keyboardinputenabled"	"0"
			}
		}

		"Dimmer"
		{
			"ControlName"	"Panel"
			"fieldName"		"Dimmer"
			"xpos"			"0"
			"ypos"			"0"
			"zpos"			"102"
			"wide"			"f0"
			"tall"			"f0"
			"visible"		"1"
			"PaintBackgroundType"	"0"
			"proportionaltoparent"	"1"
			"mouseinputenabled"		"0"
			"keyboardinputenabled"	"0"
			"alpha"			"0"
					
			"bgcolor_override" "0 0 0 255"
		}
		
	}
}
