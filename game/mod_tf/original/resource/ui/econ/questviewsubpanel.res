"Resource/UI/QuestViewSubPanel.res"
{
	"EditableTooltip"
	{
		"fieldName"		"EditableTooltip"
		"xpos"			"3"
		"ypos"			"26"
		"zpos"			"101"
		"wide"			"80"
		"tall"			"16"
		"visible"		"1"
		"enabled"		"1"
		"proportionaltoparent" "1"
		"eatmouseinput"	"0"		
	} // EditableTooltip

	"ActivateButton"
	{
		"ControlName"	"CExButton"
		"fieldName"		"ActivateButton"
		"xpos"			"3"
		"ypos"			"26"
		"zpos"			"10"
		"wide"			"80"
		"tall"			"16"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labeltext"		"#TF_QuestView_Accept"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"default"		"0"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		"Command"		"activate_node"
		"proportionaltoparent" "1"
		"actionsignallevel"	"2"
		"textinsety"	"0"
		"eatmouseinput"	"0"	
		"font"			"QuestMap_Large"
		"roundedcorners"	"0"

		"paintbackground"	"1"
			
		"defaultBgColor_override"	"StoreGreen"
		"armedBgColor_override"		"CreditsGreen"
		"depressedBgColor_override" "CreditsGreen"

		"defaultFgColor_override"	"TanLight"
		"armedFgColor_override"		"TanLight"
		"depressedFgColor_override" "TanLight"
	} // ActivateButton

	"BGImage"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"BGImage"
		"xpos"			"rs1"
		"ypos"			"0"
		"zpos"			"0"
		"wide"			"f0"
		"tall"			"o0.25"
		"visible"		"1"
		"enabled"		"1"
		"scaleImage"	"1"	
		"proportionaltoparent"	"1"
		"image"			"cyoa/node_view_contract_image"
		"mouseinputenabled"	"0"
	}

	"NameLabel"
	{
		"ControlName"	"CAutoFittingLabel"
		"fieldName"		"NameLabel"
		"labeltext"		"%name%"
		"xpos"			"rs1"
		"ypos"			"0"
		"wide"			"f0"
		"tall"			"20"
		"zpos"			"2"
		"font"			"QuestMap_Large"
		"TextAlignment"		"north-west"
		"textinsetx"	"10"
		"textinsety"	"5"
		"proportionaltoparent" "1"
		"mouseinputenabled"		"0"
		"fgcolor_override"	"TanLight"
		"skip_autoresize"	"1"

		"fonts"
		{
			"0"
			{
				"font"	"QuestMap_Large"
			}
			"1"
			{
				"font"	"QuestMap_Medium"
			}
			"2"
			{
				"font"	"QuestMap_Small"
			}
		}
	}

	"NameShadowLabel"
	{
		"ControlName"	"CAutoFittingLabel"
		"fieldName"		"NameShadowLabel"
		"labeltext"		"%name%"
		"xpos"			"rs1+1"
		"ypos"			"1"
		"wide"			"f0"
		"tall"			"20"
		"zpos"			"1"
		"font"			"QuestMap_Large"
		"TextAlignment"		"north-west"
		"proportionaltoparent" "1"
		"mouseinputenabled"		"0"
		"textinsetx"	"10"
		"textinsety"	"5"
		"fgcolor_override"	"0 0 0 255"

		"fonts"
		{
			"0"
			{
				"font"	"QuestMap_Large"
			}
			"1"
			{
				"font"	"QuestMap_Medium"
			}
			"2"
			{
				"font"	"QuestMap_Small"
			}
		}
	}

	"BGImageDarkener"
	{
		"ControlName"	"Panel"
		"fieldName"		"BGImageDarkener"
		"xpos"			"rs1"
		"ypos"			"0"
		"zpos"			"3"
		"wide"			"f0"
		"tall"			"o0.25"
		"visible"		"1"
		"enabled"		"1"
		"proportionaltoparent"	"1"
		"mouseinputenabled"	"0"
		"bgcolor_override"	"0 0 0 255"
		"alpha"	"150"
	}

	"ActivationFlash"
	{
		"ControlName"	"Panel"
		"fieldName"		"ActivationFlash"
		"xpos"			"rs1"
		"ypos"			"0"
		"zpos"			"0"
		"wide"			"f0"
		"tall"			"o0.25"
		"visible"		"1"
		"enabled"		"1"
		"proportionaltoparent"	"1"
		"mouseinputenabled"	"0"
		"keyboardinputenabled"	"0"
		"bgcolor_override"	"QuestMap_ActiveOrange"
		"Alpha"			"0"
		"skip_autoresize"	"1"
	}

	"SelectHint"
	{
		"ControlName"	"Label"
		"fieldName"		"SelectHint"
		"labeltext"		"#TF_QuestView_Choose"
		"xpos"			"rs1"
		"ypos"			"0"
		"wide"			"f0"
		"tall"			"o0.25"
		"zpos"			"1"
		"font"			"QuestObjectiveTracker_Desc"
		"TextAlignment"		"south"
		"textinsety"	"-5"
		"proportionaltoparent" "1"
		"mouseinputenabled"		"0"
		"fgcolor_override"	"0 255 0 255"
		"skip_autoresize"	"1"
		"alpha"	"0"
	}

	"SelectHintShadow"
	{
		"ControlName"	"Label"
		"fieldName"		"SelectHintShadow"
		"labeltext"		"#TF_QuestView_Choose"
		"xpos"			"rs1"
		"ypos"			"0"
		"wide"			"f0"
		"tall"			"o0.25"
		"zpos"			"1"
		"font"			"QuestObjectiveTracker_DescBlur"
		"TextAlignment"		"south"
		"textinsetx"	"2"
		"textinsety"	"-3"
		"proportionaltoparent" "1"
		"mouseinputenabled"		"0"
		"fgcolor_override"	"0 255 0 255"
		"skip_autoresize"	"1"
		"alpha"	"0"
	}

	"ObjectivesLabel"
	{

		"ControlName"	"Label"
		"fieldName"		"ObjectivesLabel"
		"labeltext"		"#TF_QuestView_Objectives"
		"xpos"			"0"
		"ypos"			"45"
		"wide"			"f0"
		"tall"			"15"
		"zpos"			"1"
		"font"			"QuestMap_Medium"
		"TextAlignment"		"east"
		"textinsetx"	"40"
		"proportionaltoparent" "1"
		"mouseinputenabled"		"0"
		"bgcolor_override"	"0 0 0 255"
		"fgcolor_override"		"QuestMap_ActiveOrange"
	}

	"ObjectivesInfoImage"
	{
		"pin_to_sibling"	"ObjectivesLabel"
		"pin_corner_to_sibling"	"PIN_CENTER_RIGHT"
		"pin_to_sibling_corner" "PIN_CENTER_RIGHT"

		"ControlName"	"ImagePanel"
		"fieldName"		"ObjectivesInfoImage"
		"xpos"			"-5"
		"ypos"			"0"
		"zpos"			"2"
		"wide"			"10"
		"tall"			"10"
		"visible"		"1"
		"enabled"		"1"
		"scaleImage"	"1"	
		"proportionaltoparent"	"1"
		"image"			"info"
	}

	"objectives"
	{
		"pin_to_sibling"	"ObjectivesLabel"
		"pin_corner_to_sibling"	"PIN_CENTER_TOP"
		"pin_to_sibling_corner" "PIN_CENTER_BOTTOM"

		"fieldName"		"objectives"
		"xpos"			"0"	
		"ypos"			"-25"
		"wide"			"f0"	
		"tall"			"200"
		"proportionaltoparent"	"1"

		"TurnInContainer"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"TurnInContainer"
			"xpos"			"0"
			"ypos"			"0"
			"zpos"			"101"
			"wide"			"f0"	
			"tall"			"500"
			"proportionaltoparent"	"1"	
			"visible"		"0"
			"paintbackground"	"0"

			"TopDimmer"
			{
				"ControlName"	"Panel"
				"fieldName"		"TopDimmer"
				"xpos"			"0"
				"ypos"			"0"
				"zpos"			"100"
				"wide"			"f0"	
				"tall"			"35"
				"proportionaltoparent"	"1"	
				"bgcolor_override"	"0 0 0 240"
				"visible"		"1"
			}

			"TurnInButton"
			{
				"ControlName"	"CExButton"
				"fieldName"		"TurnInButton"
				"xpos"			"0"
				"ypos"			"35"
				"zpos"			"101"
				"wide"			"f0"	
				"tall"			"30"
				"autoResize"	"0"
				"pinCorner"		"0"
				"visible"		"1"
				"enabled"		"1"
				"tabPosition"	"0"
				"labeltext"		"#QuestLog_TurnIn"
				"textAlignment"	"center"
				"dulltext"		"0"
				"brighttext"	"0"
				"default"		"0"
				"sound_depressed"	"UI/buttonclick.wav"
				"sound_released"	"UI/buttonclickrelease.wav"
				"Command"		"turn_in"
				"proportionaltoparent" "1"
				"actionsignallevel"	"3"
				"textinsety"	"0"
				"eatmouseinput"	"0"	
				"font"			"QuestMap_Large"
				"roundedcorners"	"0"

				"paintbackground"	"1"
				"paintborder"		"0"

				"defaultBgColor_override"	"76 107 34 240"
				"armedBgColor_override"		"76 107 34 240"
				"depressedBgColor_override" "76 107 34 240"

				"defaultFgColor_override"	"TanLight"
				"armedFgColor_override"		"QuestMap_ActiveOrange"
				"depressedFgColor_override" "QuestMap_ActiveOrange"
			} // TurnInButton

			"BottomDimmer"
			{
				"ControlName"	"Panel"
				"fieldName"		"BottomDimmer"
				"xpos"			"0"
				"ypos"			"64"
				"zpos"			"100"
				"wide"			"f0"	
				"tall"			"300"
				"proportionaltoparent"	"1"	
				"bgcolor_override"	"0 0 0 240"
				"visible"		"1"
			}
		}

		"ItemTrackerPanel"
		{
			"fieldName"		"ItemTrackerPanel"
			"xpos"			"0"	
			"ypos"			"0"
			"wide"			"f0"	
			"tall"			"200"
			"progress_bar_standard_loc_token"	"#QuestPoints_Standard"
			"progress_bar_advanced_loc_token"	"#QuestPoints_Bonus"
			"item_attribute_res_file" "resource/UI/quests/CYOA/QuestObjectivePanel_CYOA.res"
			"mouseinputenabled"	"0"
			"map_view"	"1"
			"show_item_name"	"0"
			"bar_gap"		"5"
			"group_bars_with_objectives"	"1"
			"proportionaltoparent"	"1"
		}
	}
} // QuestContainer
