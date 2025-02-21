#base "..\..\QuestItemPanel_Base.res"

"Resource/UI/econ/QuestItemPanel.res"
{
	"QuestItemPanel"
	{
		// Use Operation 1 Pauling
		"TrackerPanelResFile"	"resource/UI/quests/pauling/QuestItemTrackerPanel_QuestLog.res"	
		"ItemAttributeResFile"	"resource/UI/quests/pauling/QuestObjectivePanel_QuestLog.res"

		"anim_reset"				"QuestItem_Operation2_Reset"
		"anim_expand"				"QuestItem_Operation2_Expand"
		"anim_collapse"				"QuestItem_Operation2_Collapse"
		"anim_turning_in"			"QuestItem_Operation2_TurningIn"

		"folders"
		{
			"0"
			{
				"front"	"store\postcard_alamo"
				"back"	"store\postcard_alamo"
			}
			"1"
			{
				"front"	"store\postcard_beach"
				"back"	"store\postcard_beach"
			}
			"2"
			{
				"front"	"store\postcard_poopyjoe"
				"back"	"store\postcard_poopyjoe"
			}
			"3"
			{
				"front"	"store\postcard_merasmus"
				"back"	"store\postcard_merasmus"
			}
			"4"
			{
				"front"	"store\postcard_outletmall"
				"back"	"store\postcard_outletmall"
			}
			"5"
			{
				"front"	"store\postcard_library"
				"back"	"store\postcard_library"
			}
			"6"
			{
				"front"	"store\postcard_raccoon"
				"back"	"store\postcard_raccoon"
			}
			"7"
			{
				"front"	"store\postcard_tomjones"
				"back"	"store\postcard_tomjones"
			}
			"8"
			{
				"front"	"store\postcard_bombshop"
				"back"	"store\postcard_bombshop"
			}
		}

		"MainContainer"
		{
			"QuestPaperContainer"
			{
				"ControlName"			"EditablePanel"
				"fieldName"				"QuestPaperContainer"

				"PaperImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"PaperImage"
					"xpos"			"0"
					"ypos"			"0"
					"zpos"			"1"
					"wide"			"f0"
					"tall"			"o1"
					"visible"		"1"
					"enabled"		"1"
					"scaleImage"	"1"
					"image"			"contracts/contracts_papers1_vacation"
					"proportionaltoparent"	"1"

					"mouseinputenabled"	"0"
					"keyboardinputenabled" "0"
				}
			}
			"BackFolderContainer"
			{
				"BackFolderImage"
				{
					"ypos"	"-10"
				}
			}

			"FrontFolderContainer"
			{
				"ControlName"	"EditablePanel"
				"fieldName"		"FrontFolderContainer"
				"xpos"		"0"
				"ypos"		"0"
				"zpos"		"100"
				"wide"		"f0"
				"tall"		"f0"
				"autoResize"		"0"
				"pinCorner"		"0"
				"visible"		"1"
				"enabled"		"1"
				"tabPosition"		"0"
				"proportionaltoparent"	"1"
				"mouseinputenabled"	"1"

				"FrontFolderImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"FrontFolderImage"
					"xpos"			"0"
					"ypos"			"-10"
					"zpos"			"9"
					"wide"			"f0"
					"tall"			"o1"
					"visible"		"1"
					"enabled"		"1"
					"scaleImage"	"1"
					"image"			"contracts\contracts_folder1_front"
					"proportionaltoparent"	"1"
				
					"mouseinputenabled"	"0"
					"keyboardinputenabled" "0"
				}

				"CInputProxyPanel"
				{
					"ControlName"	"CInputProxyPanel"
					"fieldName"		"FrontInputProxyPanel"
					"xpos"		"0"
					"ypos"		"0"
					"zpos"		"100"
					"wide"		"f0"
					"tall"		"f0"
					"visible"		"1"
					"enabled"		"1"

					"proportionaltoparent"	"1"
				}

				"EncodedStatus"
				{
					"ControlName"	"CQuestStatusPanel"
					"fieldName"		"EncodedStatus"
					"xpos"			"20"
					"ypos"			"0"
					"wide"			"100"
					"tall"			"100"

					"movingcontainer"
					{
						"ControlName"			"EditablePanel"
						"fieldName"				"movingcontainer"
						"xpos"					"0"
						"ypos"					"0"
						"wide"					"f0"
						"tall"					"f0"
						"proportionaltoparent"	"1"

						"Background"
						{
							"ControlName"	"ImagePanel"
							"fieldName"		"Background"
							"xpos"			"0"
							"ypos"			"0"
							"zpos"			"0"
							"wide"			"f0"
							"tall"			"f0"
							"visible"		"1"
							"enabled"		"1"
							"scaleImage"	"1"
							"image"			"contracts/sticky"
							"proportionaltoparent"	"1"
				
							"mouseinputenabled"	"0"
							"keyboardinputenabled" "0"
						}

						"Label"
						{
							"ControlName"			"Label"
							"fieldName"				"Label"
							"font"					"QuestStickyText"
							"labelText"				"#TF_Quest_Status_Encoded"
							"textAlignment"			"center"
							"xpos"					"0"
							"ypos"					"0"
							"zpos"					"1"
							"wide"					"f0"
							"tall"					"30"
							"autoResize"			"0"
							"pinCorner"				"0"
							"visible"				"1"
							"enabled"				"1"
							"proportionaltoparent"	"1"

							"fgcolor"				"Black"
							"fgcolor_override"		"Black"
						}
					}
				}

				"ReadyToTurnInStatus"
				{
					"ControlName"	"CQuestStatusPanel"
					"fieldName"		"ReadyToTurnInStatus"
					"xpos"			"20"
					"ypos"			"0"
					"wide"			"100"
					"tall"			"100"

					"movingcontainer"
					{
						"ControlName"			"EditablePanel"
						"fieldName"				"movingcontainer"
						"xpos"					"0"
						"ypos"					"0"
						"wide"					"f0"
						"tall"					"f0"
						"proportionaltoparent"	"1"

						"Background"
						{
							"ControlName"	"ImagePanel"
							"fieldName"		"Background"
							"xpos"			"0"
							"ypos"			"0"
							"zpos"			"0"
							"wide"			"f0"
							"tall"			"f0"
							"visible"		"1"
							"enabled"		"1"
							"scaleImage"	"1"
							"image"			"contracts/sticky_turn_in"
							"proportionaltoparent"	"1"
				
							"mouseinputenabled"	"0"
							"keyboardinputenabled" "0"
						}

						"Label"
						{
							"ControlName"			"Label"
							"fieldName"				"Label"
							"font"					"QuestStickyText"
							"labelText"				"#TF_Quest_Status_ReadyToTurnIn"
							"textAlignment"			"center"
							"xpos"					"0"
							"ypos"					"0"
							"zpos"					"1"
							"wide"					"f0"
							"tall"					"30"
							"autoResize"			"0"
							"pinCorner"				"0"
							"visible"				"1"
							"enabled"				"1"
							"proportionaltoparent"	"1"

							"fgcolor"				"Black"
							"fgcolor_override"		"Black"
						}
					}
				}

				"InactiveStatus"
				{
					"ControlName"	"CQuestStatusPanel"
					"fieldName"		"InactiveStatus"
					"xpos"			"8"
					"ypos"			"0"
					"zpos"			"8"
					"wide"			"100"
					"tall"			"100"

					"movingcontainer"
					{
						"ControlName"			"EditablePanel"
						"fieldName"				"movingcontainer"
						"xpos"					"0"
						"ypos"					"0"
						"wide"					"f0"
						"tall"					"f0"
						"proportionaltoparent"	"1"

						"mouseinputenabled"	"0"
						"keyboardinputenabled" "0"

						"Background"
						{
							"ControlName"	"ImagePanel"
							"fieldName"		"Background"
							"xpos"			"0"
							"ypos"			"0"
							"zpos"			"0"
							"wide"			"f0"
							"tall"			"f0"
							"visible"		"1"
							"enabled"		"1"
							"scaleImage"	"1"
							"image"			"contracts/sticky_inactive"
							"proportionaltoparent"	"1"
				
							"mouseinputenabled"	"0"
							"keyboardinputenabled" "0"
						}

						"Label"
						{
							"ControlName"			"Label"
							"fieldName"				"Label"
							"font"					"QuestStickyText"
							"labelText"				"#TF_Quest_Status_Inactive"
							"textAlignment"			"center"
							"xpos"					"0"
							"ypos"					"0"
							"zpos"					"1"
							"wide"					"f0"
							"tall"					"30"
							"autoResize"			"0"
							"pinCorner"				"0"
							"visible"				"1"
							"enabled"				"1"
							"proportionaltoparent"	"1"

							"mouseinputenabled"	"0"
							"keyboardinputenabled" "0"

							"fgcolor"				"Black"
							"fgcolor_override"		"Black"
						}
					}
				}
			}
		}
	}
}