#base "..\QuestItemPanel_Base.res"

"Resource/UI/econ/QuestItemPanel.res"
{
	"QuestItemPanel"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"QuestItemPanel"
		"xpos"			"0"
		"ypos"			"0"
		"wide"			"f0"
		"tall"			"o1.1"
		"visible"		"1"
		"proportionaltoparent"	"1"
		"mouseinputenabled"	"1"
		"keyboardinputenabled" "1"

		"unidentified_height"	"20"
		"objective_inset"	"145"
		"front_paper_hide_height"	"65"

		"encoded_text"	"#QuestLog_Encoded_Merasmus"
		"expire_text"	"#TF_QuestExpirationWarning_Halloween"
		"TrackerPanelResFile"	"resource/UI/quests/merasmus/QuestItemTrackerPanel_QuestLog.res"	
		"ItemAttributeResFile"	"resource/UI/quests/merasmus/QuestObjectivePanel_QuestLog.res"

		"decode_style"	"1"

		// Sound effects
		"turn_in_sound"				"ui/quest_turn_in_decode_halloween.wav" 
		"turn_in_success_sound"		"ui/quest_turn_in_accepted_halloween.wav"
		"decode_sound"				"ui/quest_decode_halloween.wav"
		"collapse_sound"			"ui/quest_folder_close_halloween.wav"
		"expand_sound"				"ui/quest_folder_keeper_slide_off_halloween.wav"

		// Animations
		"anim_reset"				"QuestItem_Reset_Halloween"
		"anim_expand"				"QuestItem_Expand_Halloween"
		"anim_collapse"				"QuestItem_Collapse_Halloween"
		"anim_highlight_on"			"QuestItem_Highlight_On_Halloween"
		"anim_highlight_off"		"QuestItem_Highlight_Off_Halloween"

		"folders"
		{
			"0"
			{
				"front"	"contracts\halloween\contracts_folder1_front"
				"back"	""
			}
		}

		"tracker_kv"
		{
			"fieldName"	"ItemTrackerPanel"
			"xpos"		"0"
			"ypos"		"0"
			"wide"		"f0"
			"tall"		"f0"
			"zpos"		"11"
			"proportionaltoparent"	"1"
		}

		"MainContainer"
		{
			"GlowImage"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"GlowImage"
				"xpos"			"0"
				"ypos"			"-15"
				"zpos"			"1"
				"wide"			"f0"
				"tall"			"o0.5"
				"visible"		"1"
				"enabled"		"1"
				"scaleImage"	"1"
				"alpha"			"0"
				"image"			"contracts\halloween\contracts_hilight"
				"proportionaltoparent"	"1"

				"mouseinputenabled"	"0"
				"keyboardinputenabled" "0"
			}

			"QuestPaperContainer"
			{
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
					"image"			"contracts\halloween\contracts_papers1"
					"proportionaltoparent"	"1"

					"mouseinputenabled"	"0"
					"keyboardinputenabled" "0"
				}

				"EncodedImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"EncodedImage"
					"xpos"			"0"
					"ypos"			"0"
					"zpos"			"199"
					"wide"			"f0"
					"tall"			"o1"
					"visible"		"1"
					"enabled"		"1"
					"scaleImage"	"1"
					"image"			"contracts\halloween\contracts_papers_encoded"
					"proportionaltoparent"	"1"

					"mouseinputenabled"	"0"
					"keyboardinputenabled" "0"
				}

				"TitleButton"
				{
					"ControlName"	"CExButton"
					"fieldName"		"TitleButton"
					"xpos"		"p0.25"
					"ypos"		"10"
					"wide"		"p0.63"
					"tall"		"30"
					"autoResize"		"0"
					"pinCorner"		"0"
					"visible"		"1"
					"enabled"		"1"
					"tabPosition"		"0"
					"labelText"			"%title%"
					"bgcolor_override"	"0 0 0 220"
					"fgcolor"		"Black"
					"font"			"QuestMediumText_Merasmus"
					"textinsetx"		"0"
					"textAlignment"	"center"
					"auto_wide_tocontents"	"0"
					"allcaps"	"0"

					"proportionaltoparent"	"1"
					"paintbackground"	"0"
					"command"	"select"
					"actionsignallevel" "3"

					"defaultFgColor_override"	"0 0 0 255"
					"armedFgColor_override"		"HalloweenThemeColor2015"
				}

				"PaperClips"
				{
					"visible"		"0"
				}

				"FindServerButton"
				{
					"xpos"			"25"
					"ypos"			"52"
					"wide"			"100"
					"autoResize"	"0"
					"pinCorner"		"0"

					"SubButton"
					{
						"wide"			"110"
						"font"			"QuestInstructionText_Merasmus"
						"labelText"		"#TF_Quest_PlayThisMap_Merasmission"

						"auto_wide_tocontents"	"0"
						"paintbackground"	"0"
						"paintborder"		"1"
						"RoundedCorners"	"0"
						"border_default"			"ReplayDefaultBorder"
						"defaultFgColor_override"	"TanLight"
						"armedFgColor_override"		"Orange"
					}
				}


				"IdentifyButtonContainer"
				{
					"ControlName"			"EditablePanel"
					"fieldName"				"IdentifyButtonContainer"
					"xpos"					"0"
					"ypos"					"130"
					"zpos"					"200"
					"wide"					"f0"
					"tall"					"30"
					"visible"				"0"
					"proportionaltoparent"	"1"

					"Dimmer"
					{
						"ControlName"	"EditablePanel"
						"fieldName"		"Dimmer"
						"xpos"			"0"
						"ypos"			"0"
						"zpos"			"0"
						"wide"			"f0"
						"tall"			"f0"
						"visible"		"1"
						"enabled"		"1"
						"proportionaltoparent" "1"
						"bgcolor_override"	"0 0 0 247"
						"mouseinputenabled"	"0"
					}

					"IdentifyButton"
					{
						"labelText"		"#QuestLog_Identify_Merasmus"
	
						"font"			"QuestLargeText_Merasmus"
						"allcaps"	"1"
						"textAlignment"	"center"

						"sound_depressed"	"UI/buttonclick.wav"
						"sound_released"	"UI/buttonclickrelease.wav"
						"defaultFgColor_override" "White"
						"armedFgColor_override" "HalloweenThemeColor2015"
						"depressedFgColor_override" "HalloweenThemeColor2015"
					}
				}

				"TurnInContainer"
				{
					"ControlName"			"EditablePanel"
					"fieldName"				"TurnInContainer"
					"xpos"					"0"
					"ypos"					"130"
					"zpos"					"200"
					"wide"					"f0"
					"tall"					"30"
					"visible"				"0"
					"proportionaltoparent"	"1"

					"Dimmer"
					{
						"ControlName"	"EditablePanel"
						"fieldName"		"Dimmer"
						"xpos"			"0"
						"ypos"			"0"
						"zpos"			"0"
						"wide"			"f0"
						"tall"			"f0"
						"visible"		"1"
						"enabled"		"1"
						"proportionaltoparent" "1"
						"bgcolor_override"	"150 255 0 100"
						"mouseinputenabled"	"0"
					}

					"GreyDimmer"
					{
						"ControlName"	"EditablePanel"
						"fieldName"		"GreyDimmer"
						"xpos"			"0"
						"ypos"			"0"
						"zpos"			"0"
						"wide"			"f0"
						"tall"			"f0"
						"visible"		"1"
						"enabled"		"1"
						"proportionaltoparent" "1"
						"bgcolor_override"	"0 0 0 230"
						"mouseinputenabled"	"0"
					}

					"TurnInButton"
					{
						"ControlName"	"CExButton"
						"fieldName"		"TurnInButton"
						"xpos"			"0"
						"ypos"			"0"
						"zpos"			"10"
						"wide"			"f0"
						"tall"			"f0"
						"autoResize"	"0"
						"visible"		"1"
						"enabled"		"1"
						"tabPosition"	"0"
						"proportionaltoparent"	"1"
						"labelText"		"#QuestLog_TurnIn"
						"actionsignallevel" "4"
						"use_proportional_insets" "1"
						"font"			"QuestLargeText_Merasmus"
						"allcaps"	"1"
						"textAlignment"	"center"
						"dulltext"		"0"
						"brighttext"	"0"
						"default"		"1"

						"sound_depressed"	"UI/buttonclick.wav"
						"sound_released"	"UI/buttonclickrelease.wav"

						"paintbackground"	"0"

						"defaultFgColor_override" "White"
						"armedFgColor_override" "HalloweenThemeColor2015"
						"depressedFgColor_override" "HalloweenThemeColor2015"

						"command"	"turnin"
					}

					"TurnInSpinnerContainer"
					{
						"ControlName"			"EditablePanel"
						"fieldName"				"TurnInSpinnerContainer"
						"xpos"					"0"
						"ypos"					"0"
						"zpos"					"200"
						"wide"					"f0"
						"tall"					"f0"
						"visible"				"1"
						"proportionaltoparent"	"1"

						"TurningInLabel"
						{
							"labelText"		"#QuestLog_TurningIn_Merasmus"
							"use_proportional_insets" "1"
							"textinsetx"			"55"
							"font"			"QuestLargeText_Merasmus"
							"textAlignment"	"west"
						}
					}
				}


				"AcceptedImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"AcceptedImage"
					"xpos"			"cs-0.2"
					"ypos"			"cs-0.6"
					"zpos"			"1000"
					"wide"			"p0.5"
					"tall"			"o1"
					"visible"		"0"
					"enabled"		"1"
					"image"			"contracts/halloween/accepted"
					"scaleImage"	"1"	
					"proportionaltoparent"	"1"	
				}

				"StaticPhoto"
				{
					"visible"		"0"
					"image"			""
				}

				"CharacterBackdropImage"
				{
					"visible"		"0"
				}

				"ScrollableBottomContainer"
				{
					"ypos"					"55"
					"zpos"					"10"
					"tall"					"220"
				
					"QuestExpirationWarning"
					{
						"font"					"QuestInstructionText_Merasmus"
					}

					"QuestObjectiveExplanation"
					{
						"font"					"QuestInstructionText_Merasmus"
						"labelText"				"#TF_QuestObjective_Explanation_Merasmus"
						"allcaps"				"0"
					}

					"QuestFlavorText"
					{
						"xpos"					"cs-0.5"
						"wide"					"p0.95"
						"font"					"QuestFlavorText_Merasmus"
					}
				}
			}


			"BackFolderContainer"
			{
				"ControlName"	"EditablePanel"
				"fieldName"		"BackFolderContainer"
				"visible"		"0"

				"BackFolderImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"BackFolderImage"
					"visible"		"0"
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
					"image"			"contracts\halloween\contracts_folder1_front"
				}

				"SleeveImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"SleeveImage"
					"xpos"			"30"
					"ypos"			"10"
					"zpos"			"10"
					"wide"			"o1"
					"tall"			"87"
					"visible"		"1"
					"enabled"		"1"
					"scaleImage"	"1"
					"image"			"contracts\halloween\contracts_scroll_sleeve"
					"proportionaltoparent"	"1"

					"mouseinputenabled"	"0"
					"keyboardinputenabled" "0"
				}

				"EncodedStatus"
				{
					"xpos"			"10"
					"ypos"			"20"
					"zpos"			"11"
					"wide"			"100"
					"tall"			"o1"
					"proportionaltoparent"	"1"
					"mouseinputenabled"	"0"
					"keyboardinputenabled" "0"

					"visible_y"	"0"
					"hidden_y"	"0"

					"movingcontainer"
					{
						"Background"
						{
							"image"			"contracts/halloween/sticky"
						}

						"Label"
						{
							"visible"	"0"
						}
					}
				}

				"ReadyToTurnInStatus"
				{
					"ControlName"	"CQuestStatusPanel"
					"fieldName"		"ReadyToTurnInStatus"
					"xpos"			"10"
					"ypos"			"20"
					"zpos"			"11"
					"wide"			"100"
					"tall"			"o1"
					"proportionaltoparent"	"1"

					"visible_y"	"0"
					"hidden_y"	"0"

					"mouseinputenabled"	"0"
					"keyboardinputenabled" "0"

					"movingcontainer"
					{
						"Background"
						{
							"image"			"contracts/halloween/sticky_turn_in"
						}

						"Label"
						{
							"visible"	"0"
						}
					}
				}

				"InactiveStatus"
				{
					"ControlName"	"CQuestStatusPanel"
					"fieldName"		"InactiveStatus"
					"xpos"			"10"
					"ypos"			"20"
					"zpos"			"11"
					"wide"			"100"
					"tall"			"o1"
					"proportionaltoparent"	"1"

					"visible_y"	"0"
					"hidden_y"	"0"

					"movingcontainer"
					{
						"Background"
						{
							"image"			"contracts/halloween/sticky_inactive"
						}

						"Label"
						{
							"visible"	"0"
						}
					}
				}
			}
		}
	}	
}