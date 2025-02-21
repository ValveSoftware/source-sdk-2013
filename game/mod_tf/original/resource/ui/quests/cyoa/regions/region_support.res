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
					"child_name"	"MedicIcon"
				}
				"1"
				{
					"child_name"	"SniperIcon"
				}
				"2"
				{
					"child_name"	"SpyIcon"
				}
			}

			"MedicIcon"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"MedicIcon"
				"xpos"			"185"
				"ypos"			"130"
				"zpos"			"1"
				"wide"			"50"
				"tall"			"o1"
				"visible"		"1"
				"enabled"		"1"
				"scaleImage"	"1"
				"image"			"cyoa/cyoa_icon_medic"
				//"proportionaltoparent"	"1"

				"mouseinputenabled"	"0"
				"keyboardinputenabled" "	0"
				"drawcolor"		"QuestMap_BGImages"
			}

			"SniperIcon"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"SniperIcon"
				"xpos"			"245"
				"ypos"			"130"
				"zpos"			"1"
				"wide"			"50"
				"tall"			"o1"
				"visible"		"1"
				"enabled"		"1"
				"scaleImage"	"1"
				"image"			"cyoa/cyoa_icon_sniper"
				//"proportionaltoparent"	"1"

				"mouseinputenabled"	"0"
				"keyboardinputenabled" "	0"
				"drawcolor"		"QuestMap_BGImages"
			}

			"SpyIcon"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"SpyIcon"
				"xpos"			"305"
				"ypos"			"130"
				"zpos"			"1"
				"wide"			"50"
				"tall"			"o1"
				"visible"		"1"
				"enabled"		"1"
				"scaleImage"	"1"
				"image"			"cyoa/cyoa_icon_spy"
				//"proportionaltoparent"	"1"

				"mouseinputenabled"	"0"
				"keyboardinputenabled" "	0"
				"drawcolor"		"QuestMap_BGImages"
			}
		}
	}
}