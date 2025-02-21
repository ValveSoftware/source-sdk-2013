#base "region_base.res"

"Resource/ui/quests/CYOA/regions/region1.res"
{	
	"Region"
	{
		"ZoomPanel"
		{
			"pending_children"
			{
				"0"
				{
					"child_name"	"ScoutIcon"
				}
				"1"
				{
					"child_name"	"SoldierIcon"
				}
				"2"
				{
					"child_name"	"PyroIcon"
				}
			}

			"ScoutIcon"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"ScoutIcon"
				"xpos"			"185"
				"ypos"			"130"
				"zpos"			"1"
				"wide"			"50"
				"tall"			"o1"
				"visible"		"1"
				"enabled"		"1"
				"scaleImage"	"1"
				"image"			"cyoa/cyoa_icon_scout"
				//"proportionaltoparent"	"1"

				"mouseinputenabled"	"0"
				"keyboardinputenabled" "	0"
				"drawcolor"		"QuestMap_BGImages"
			}

			"SoldierIcon"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"SoldierIcon"
				"xpos"			"245"
				"ypos"			"130"
				"zpos"			"1"
				"wide"			"50"
				"tall"			"o1"
				"visible"		"1"
				"enabled"		"1"
				"scaleImage"	"1"
				"image"			"cyoa/cyoa_icon_soldier"
				//"proportionaltoparent"	"1"

				"mouseinputenabled"	"0"
				"keyboardinputenabled" "	0"
				"drawcolor"		"QuestMap_BGImages"
			}

			"PyroIcon"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"PyroIcon"
				"xpos"			"305"
				"ypos"			"130"
				"zpos"			"1"
				"wide"			"50"
				"tall"			"o1"
				"visible"		"1"
				"enabled"		"1"
				"scaleImage"	"1"
				"image"			"cyoa/cyoa_icon_pyro"
				//"proportionaltoparent"	"1"

				"mouseinputenabled"	"0"
				"keyboardinputenabled" "	0"
				"drawcolor"		"QuestMap_BGImages"
			}
		}
	}
}