#base "region_base.res"

"Resource/ui/quests/CYOA/regions/region_defense.res"
{	
	"Region"
	{
		"ZoomPanel"
		{
			"pending_children"
			{
				"0"
				{
					"child_name"	"DemoIcon"
				}
				"1"
				{
					"child_name"	"HeavyIcon"
				}
				"2"
				{
					"child_name"	"EngineerIcon"
				}
			}

			"DemoIcon"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"DemoIcon"
				"xpos"			"185"
				"ypos"			"130"
				"zpos"			"1"
				"wide"			"50"
				"tall"			"o1"
				"visible"		"1"
				"enabled"		"1"
				"scaleImage"	"1"
				"image"			"cyoa/cyoa_icon_demoman"
				//"proportionaltoparent"	"1"

				"mouseinputenabled"	"0"
				"keyboardinputenabled" "	0"
				"drawcolor"		"QuestMap_BGImages"
			}

			"HeavyIcon"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"HeavyIcon"
				"xpos"			"245"
				"ypos"			"130"
				"zpos"			"1"
				"wide"			"50"
				"tall"			"o1"
				"visible"		"1"
				"enabled"		"1"
				"scaleImage"	"1"
				"image"			"cyoa/cyoa_icon_heavy"
				//"proportionaltoparent"	"1"

				"mouseinputenabled"	"0"
				"keyboardinputenabled" "	0"
				"drawcolor"		"QuestMap_BGImages"
			}

			"EngineerIcon"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"EngineerIcon"
				"xpos"			"305"
				"ypos"			"130"
				"zpos"			"1"
				"wide"			"50"
				"tall"			"o1"
				"visible"		"1"
				"enabled"		"1"
				"scaleImage"	"1"
				"image"			"cyoa/cyoa_icon_engineer"
				//"proportionaltoparent"	"1"

				"mouseinputenabled"	"0"
				"keyboardinputenabled" "	0"
				"drawcolor"		"QuestMap_BGImages"
			}
		}
	}
}