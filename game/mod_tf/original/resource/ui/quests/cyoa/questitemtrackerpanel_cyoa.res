#base "..\QuestItemTrackerPanel_InGame_Base.res"

"Resource/UI/HudAchievementTrackerItem.res"
{	
	"ItemTrackerPanel"
	{
		"fieldName"		"ItemTrackerPanel"
		"xpos"			"cs-0.5"	
		"ypos"			"25"
		"wide"			"f10"	
		"tall"			"100"
		"progress_bar_standard_loc_token"	"#QuestPoints_Standard"
		"progress_bar_advanced_loc_token"	"#QuestPoints_Bonus"
		"item_attribute_res_file" "resource/UI/quests/CYOA/QuestObjectivePanel_CYOA.res"
		"mouseinputenabled"	"0"
		"map_view"	"1"
		"show_item_name"	"0"
		"bar_gap"		"5"
		"group_bars_with_objectives"	"1"
		"proportionaltoparent"	"1"
		"standard_glow_color"	"QuestMap_ActiveOrange"
		"attrib_y_start_offset"	"2"
		"attrib_x_offset"		"46"

		"ModelImageKV"
		{
			"fieldName"	"ModelImage"
			"wide"		"20"
			"tall"		"20"
			"scaleimage"	"1"
			"zpos"		"10"
		}

		"progressbarKV"
		{
			"xpos"			"0"
			"ypos"			"8"
			"wide"			"f0"
			"tall"			"9"
			"zpos"			"4"
			"visible"		"1"
			"enabled"		"1"
			"proportionaltoparent" "1"

			"bgcolor_override"		"212 127 25 41"

			"PointsLabel"
			{
				"ControlName"	"Label"
				"fieldName"		"PointsLabel"
				"labeltext"		"%points%"
				"xpos"			"0"
				"ypos"			"0"
				"wide"			"f0"
				"tall"			"f0"
				"zpos"			"1"
				"font"			"QuestMap_Medium"
				"TextAlignment"		"center"
				"proportionaltoparent" "1"
				"fgcolor_override"	"TanLight"
			}

			"ProgressBarStandardHighlight" // current completed
			{
				"ControlName"	"EditablePanel"
				"fieldName"		"ProgressBarStandardHighlight"
				"xpos"			"0"
				"ypos"			"0"
				"wide"			"f0"
				"tall"			"f0"
				"bgcolor_override"		"QuestUncommitted"
				"zpos"			"2"
				"visible"		"1"
				"enabled"		"1"
				"proportionaltoparent" "1"

				"PointsLabelInvert"
				{
					"ControlName"	"Label"
					"fieldName"		"PointsLabelInvert"
					"labeltext"		"%points%"
					"xpos"			"0"
					"ypos"			"0"
					"wide"			"f0"
					"tall"			"f0"
					"zpos"			"8"
					"font"			"QuestMap_Medium"
					"TextAlignment"		"center"
					"proportionaltoparent" "1"
					"fgcolor_override"	"Black"
				}
			}

			"ProgressBarStandard" // current completed
			{
				"ControlName"	"EditablePanel"
				"fieldName"		"ProgressBarStandard"
				"xpos"			"0"
				"ypos"			"0"
				"wide"			"f0"
				"tall"			"f0"
				"zpos"			"3"
				"visible"		"1"
				"enabled"		"1"
				"proportionaltoparent" "1"

				"bgcolor_override"	"QuestMap_ActiveOrange"

				"PointsLabelInvert"
				{
					"ControlName"	"Label"
					"fieldName"		"PointsLabelInvert"
					"labeltext"		"%points%"
					"xpos"			"0"
					"ypos"			"0"
					"wide"			"f0"
					"tall"			"f0"
					"zpos"			"8"
					"font"			"QuestMap_Medium"
					"TextAlignment"		"center"
					"proportionaltoparent" "1"
					"fgcolor_override"	"TanLight"
				}
			}
		}

		"bonusprogressbarKV"
		{
			"xpos"			"0"
			"ypos"			"8"
			"wide"			"f0"
			"tall"			"6"
			"zpos"			"4"
			"visible"		"1"
			"enabled"		"1"
			"proportionaltoparent" "1"

			"bgcolor_override"		"222 217 166 41"

			"PointsLabel"
			{
				"ControlName"	"Label"
				"fieldName"		"PointsLabel"
				"labeltext"		"%points%"
				"xpos"			"0"
				"ypos"			"0"
				"wide"			"f0"
				"tall"			"f0"
				"zpos"			"1"
				"font"			"QuestMap_Small"
				"textinsety"	"-1"
				"TextAlignment"		"center"
				"proportionaltoparent" "1"
				"fgcolor_override"	"QuestMap_Bonus"
			}

			"ProgressBarStandardHighlight" // current completed
			{
				"ControlName"	"EditablePanel"
				"fieldName"		"ProgressBarStandardHighlight"
				"xpos"			"0"
				"ypos"			"0"
				"wide"			"f0"
				"tall"			"f0"
				"bgcolor_override"		"QuestUncommitted"
				"zpos"			"2"
				"visible"		"1"
				"enabled"		"1"
				"proportionaltoparent" "1"

				"PointsLabelInvert"
				{
					"ControlName"	"Label"
					"fieldName"		"PointsLabelInvert"
					"labeltext"		"%points%"
					"xpos"			"0"
					"ypos"			"0"
					"wide"			"f0"
					"tall"			"f0"
					"zpos"			"8"
					"font"			"QuestMap_Small"
					"textinsety"	"-1"
					"TextAlignment"		"center"
					"proportionaltoparent" "1"
					"fgcolor_override"	"Black"
				}
			}

			"ProgressBarStandard" // current completed
			{
				"ControlName"	"EditablePanel"
				"fieldName"		"ProgressBarStandard"
				"xpos"			"0"
				"ypos"			"0"
				"wide"			"f0"
				"tall"			"f0"
				"zpos"			"3"
				"visible"		"1"
				"enabled"		"1"
				"proportionaltoparent" "1"

				"bgcolor_override"	"QuestMap_Bonus"

				"PointsLabelInvert"
				{
					"ControlName"	"Label"
					"fieldName"		"PointsLabelInvert"
					"labeltext"		"%points%"
					"xpos"			"0"
					"ypos"			"0"
					"wide"			"f0"
					"tall"			"f0"
					"zpos"			"8"
					"font"			"QuestMap_Small"
					"textinsety"	"-1"
					"TextAlignment"		"center"
					"proportionaltoparent" "1"
					"fgcolor_override"	"Black"
				}
			}
		}
	}

	"ItemName"
	{
		"visible"	"0"
	}

	"PrimaryLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"PrimaryLabel"
		"labeltext"		"#QuestPoints_Primary"
		"xpos"			"0"
		"ypos"			"50"
		"wide"			"190"
		"tall"			"20"
		"zpos"			"4"
		"textinsetx"	"0"
		"fgcolor"		"TanDark"
		"font"			"QuestMap_Small"
		"TextAlignment"		"north-west"
	}

	"BonusLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"BonusLabel"
		"labeltext"		"#QuestPoints_Bonus"
		"xpos"			"0"
		"ypos"			"50"
		"wide"			"190"
		"tall"			"20"
		"zpos"			"4"
		"textinsetx"	"0"
		"fgcolor"		"TanDark"
		"font"			"QuestMap_Small"
		"TextAlignment"		"north-west"
	}

	"Star0"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"Star0"
		"xpos"			"32"
		"ypos"			"30"
		"zpos"			"1"
		"wide"			"10"
		"tall"			"10"
		"visible"		"1"
		"enabled"		"1"
		"image"			"cyoa/star_on"
		"scaleImage"	"1"
		"proportionaltoparent" "1"
	}
	
	"Star1"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"Star1"
		"xpos"			"32"
		"ypos"			"30"
		"zpos"			"1"
		"wide"			"10"
		"tall"			"10"
		"visible"		"1"
		"enabled"		"1"
		"image"			"cyoa/star_on"
		"scaleImage"	"1"
		"proportionaltoparent" "1"
	}
	
	"Star2"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"Star2"
		"xpos"			"32"
		"ypos"			"30"
		"zpos"			"1"
		"wide"			"10"
		"tall"			"10"
		"visible"		"1"
		"enabled"		"1"
		"image"			"cyoa/star_on"
		"scaleImage"	"1"
		"proportionaltoparent" "1"
	}				

	"NotYetCommittedContainer"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"NotYetCommittedContainer"
		"xpos"			"rs1"		
		"ypos"			"0"		
		"zpos"			"3"
		"wide"			"f0"
		"tall"			"10"
		"visible"		"1"
		"enabled"		"1"	

		"proportionaltoparent" "1"

		"PendingText"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"PendingText"
			"labeltext"		"#QuestTracker_Pending"
			"xpos"			"0"
			"ypos"			"0"
			"wide"			"190"
			"tall"			"f0"
			"zpos"			"4"
			"textinsetx"	"0"
			"fgcolor_override"		"RedSolid"
			"font"			"QuestMap_Small"
			"TextAlignment"		"center"
		}
	}
}