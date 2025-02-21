"Resource/UI/econ/QuestItemPanel.res"
{
	"QuestItemPanel"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"QuestItemPanel"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"10"
		"wide"			"f0"
		"tall"			"o1.1"
		"visible"		"1"
		"proportionaltoparent"	"1"
		"mouseinputenabled"	"1"
		"keyboardinputenabled" "1"

		"unidentified_height"	"20"
		"objective_inset"	"145"

		"decode_style"	"0"

		"encoded_text"	"#QuestLog_Encoded"
		"expire_text"	"#TF_QuestExpirationWarning"
		"TrackerPanelResFile"	"resource/UI/quests/QuestItemTrackerPanel_QuestLog_Base.res"	
		"ItemAttributeResFile"	"resource/UI/quests/QuestObjectivePanel_QuestLog_Base.res"
		"scrolling_container_height"	"240"

		// Sound effects
		"turn_in_sound"				"ui/quest_turn_in_decode.wav" 
		"turn_in_success_sound"		"ui/quest_turn_in_accepted.wav"
		"decode_sound"				"ui/quest_decode.wav"
		"collapse_sound"			"ui/quest_folder_close.wav"
		"expand_sound"				"ui/quest_folder_open.wav"

		// Animations
		"anim_reset"				"QuestItem_Reset"
		"anim_expand"				"QuestItem_Expand"
		"anim_collapse"				"QuestItem_Collapse"
		"anim_turning_in"			"QuestItem_TurningIn"

		"modelpanels_kv"
		{
			"ControlName"	"CItemModelPanel"
			"xpos"			"p0.1"
			"ypos"			"30"
			"zpos"			"13"
			"wide"			"60"
			"tall"			"o1"
			"visible"		"0"
			"bgcolor_override"		"0 0 0 0"
			"noitem_textcolor"		"117 107 94 255"
			"PaintBackgroundType"	"2"
			"paintborder"	"0"
			"proportionaltoparent"	"1"
			
			"model_xpos"	"0"
			"model_ypos"	"0"
			"model_wide"	"65"
			"model_tall"	"65"
			"text_ypos"		"60"
			"text_center"	"1"
			"model_only"	"1"
			
			"inset_eq_x"	"2"
			"inset_eq_y"	"2"

			"deferred_description"	"1"
			"deferred_icon"			"1"
			
			"itemmodelpanel"
			{
				"inventory_image_type"	"1"
				"use_item_rendertarget" "0"
				"allow_rot"				"0"
			}

			"use_item_sounds"	"1"
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
			"ControlName"			"EditablePanel"
			"fieldName"				"MainContainer"
			"xpos"					"0"
			"ypos"					"r0"
			"wide"					"f0"
			"tall"					"f0"
			"proportionaltoparent"	"1"

			"QuestPaperContainer"
			{
				"ControlName"			"EditablePanel"
				"fieldName"				"QuestPaperContainer"
				"xpos"					"0"
				"ypos"					"10"
				"zpos"					"10"
				"wide"					"f0"
				"tall"					"f0"
				"visible"				"1"
				"proportionaltoparent"	"1"

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
					"image"			"contracts\contracts_papers1"
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
					"visible"		"0"
					"enabled"		"1"
					"scaleImage"	"1"
					"proportionaltoparent"	"1"

					"mouseinputenabled"	"0"
					"keyboardinputenabled" "0"
				}

				"CInputProxyPanel"
				{
					"ControlName"	"CInputProxyPanel"
					"fieldName"		"PaperInputProxyPanel"
					"xpos"		"0"
					"ypos"		"0"
					"zpos"		"100"
					"wide"		"f0"
					"tall"		"f0"
					"visible"		"1"
					"enabled"		"1"

					"proportionaltoparent"	"1"
				}

				"TitleButton"
				{
					"ControlName"	"CExButton"
					"fieldName"		"TitleButton"
					"xpos"		"0"
					"ypos"		"10"
					"zpos"		"99"
					"wide"		"f0"
					"tall"		"30"
					"autoResize"		"0"
					"pinCorner"		"0"
					"visible"		"1"
					"enabled"		"1"
					"tabPosition"		"0"
					"labelText"			"%title%"
					"bgcolor_override"	"0 0 0 220"
					"fgcolor"		"Black"
					"font"			"QuestLargeText"
					"allcaps"	"1"
					"textinsetx"		"50"
					"textAlignment"	"east"

					"proportionaltoparent"	"1"
					"paintbackground"	"0"
					"command"	"select"
					"actionsignallevel" "3"


					"defaultFgColor_override"	"0 0 0 255"
					"armedFgColor_override"		"Orange"
				}

				"PaperClips"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"PaperClips"
					"xpos"			"0"
					"ypos"			"0"
					"zpos"			"100"
					"wide"			"f0"
					"tall"			"o1"
					"visible"		"1"
					"enabled"		"1"
					"scaleImage"	"1"
					"image"			"contracts\contracts_staples1"
					"proportionaltoparent"	"1"

					"mouseinputenabled"	"0"
					"keyboardinputenabled" "0"
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
						"ControlName"	"CExButton"
						"fieldName"		"IdentifyButton"
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
						"labelText"		"#QuestLog_Identify"
						"actionsignallevel" "4"
						"use_proportional_insets" "1"
						"font"			"QuestLargeText"
						"allcaps"	"1"
						"textAlignment"	"center"
						"dulltext"		"0"
						"brighttext"	"0"
						"default"		"1"

						"sound_depressed"	"UI/buttonclick.wav"
						"sound_released"	"UI/buttonclickrelease.wav"

						"paintbackground"	"0"

						"defaultFgColor_override" "White"
						"armedFgColor_override" "Orange"
						"depressedFgColor_override" "Orange"

						"command"	"identify"
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
						"font"			"QuestLargeText"
						"allcaps"	"1"
						"textAlignment"	"center"
						"dulltext"		"0"
						"brighttext"	"0"
						"default"		"1"

						"sound_depressed"	"UI/buttonclick.wav"
						"sound_released"	"UI/buttonclickrelease.wav"

						"paintbackground"	"0"

						"defaultFgColor_override" "White"
						"armedFgColor_override" "Orange"
						"depressedFgColor_override" "Orange"

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
							"ControlName"	"Label"
							"fieldName"		"TurningInLabel"
							"xpos"			"0"
							"ypos"			"0"
							"zpos"			"10"
							"wide"			"f0"
							"tall"			"f0"
							"visible"		"1"
							"enabled"		"1"
							"proportionaltoparent"	"1"
							"labelText"		"#QuestLog_TurningIn"
							"use_proportional_insets" "1"
							"textinsetx"			"55"
							"font"			"QuestLargeText"
							"allcaps"	"1"
							"textAlignment"	"west"
							"dulltext"		"0"
							"brighttext"	"0"
							"paintbackground"	"0"
						}
					}
				}


				"AcceptedImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"AcceptedImage"
					"xpos"			"cs-0.5"
					"ypos"			"cs-0.6"
					"zpos"			"1000"
					"wide"			"p0.8"
					"tall"			"o1"
					"visible"		"0"
					"enabled"		"1"
					"image"			"contracts/accepted"
					"scaleImage"	"1"	
					"proportionaltoparent"	"1"	

					if_brazilian
					{
						"image"			"contracts/accepted_brazilian"
					}
					if_czech
					{
						"image"			"contracts/accepted_czech"
					}
					if_danish
					{
						"image"			"contracts/accepted_danish"
					}
					if_dutch
					{
						"image"			"contracts/accepted_dutch"
					}
					if_finnish
					{
						"image"			"contracts/accepted_finnish"
					}
					if_french
					{
						"image"			"contracts/accepted_french"
					}
					if_german
					{
						"image"			"contracts/accepted_german"
					}
					if_greek
					{
						"image"			"contracts/accepted_greek"
					}
					if_hungarian
					{
						"image"			"contracts/accepted_hungarian"
					}
					if_italian
					{
						"image"			"contracts/accepted_italian"
					}
					if_japanese
					{
						"image"			"contracts/accepted_japanese"
					}
					if_korean
					{
						"image"			"contracts/accepted_korean"
					}
					if_koreana
					{
						"image"			"contracts/accepted_korean"
					}
					if_norwegian
					{
						"image"			"contracts/accepted_norwegian"
					}
					if_polish
					{
						"image"			"contracts/accepted_polish"
					}
					if_portuguese
					{
						"image"			"contracts/accepted_portuguese"
					}
					if_romanian
					{
						"image"			"contracts/accepted_romanian"
					}
					if_russian
					{
						"image"			"contracts/accepted_russian"
					}
					if_schinese
					{
						"image"			"contracts/accepted_schinese"
					}
					if_spanish
					{
						"image"			"contracts/accepted_spanish"
					}
					if_swedish
					{
						"image"			"contracts/accepted_swedish"
					}
					if_tchinese
					{
						"image"			"contracts/accepted_tchinese"
					}
					if_turkish
					{
						"image"			"contracts/accepted_turkish"
					}
					if_ukrainian
					{
						"image"			"contracts/accepted_ukrainian"
					}
				}

				"StaticPhoto"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"StaticPhoto"
					"xpos"			"p0.045"
					"ypos"			"s0.95"
					"zpos"			"51"
					"wide"			"p0.375"
					"tall"			"o1"
					"visible"		"1"
					"enabled"		"1"
					"image"			"contracts/photo_static"
					"scaleImage"	"1"	
					"proportionaltoparent"	"1"	
				}

				"CharacterBackdropImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"CharacterBackdropImage"
					"xpos"			"p0.045"
					"ypos"			"s0.95"
					"zpos"			"50"
					"wide"			"p0.375"
					"tall"			"o1"
					"visible"		"1"
					"enabled"		"1"
					"image"			"quest_pauling"
					"scaleImage"	"1"	
					"proportionaltoparent"	"1"	
				}

				"FindServerButton"
				{
					"ControlName"	"CExButton"
					"fieldName"		"FindServerButton"
					"xpos"			"p0.5"
					"ypos"			"37"
					"zpos"			"101"
					"wide"			"p.42"
					"tall"			"15"
					"visible"		"1"
					"autoResize"	"0"
					"pinCorner"		"3"
					"visible"		"1"
					"enabled"		"1"
					"tabPosition"	"0"
					"proportionaltoparent"	"1"
					"labelText"		"#TF_Quest_FindServer"
					"use_proportional_insets" "0"
					"font"			"QuestFlavorText"
					"allcaps"	"1"
					"textAlignment"	"center"
					"dulltext"		"0"
					"brighttext"	"0"
					"default"		"1"
					"actionsignallevel" "3"

					"sound_depressed"	"UI/buttonclick.wav"
					"sound_released"	"UI/buttonclickrelease.wav"

					"auto_wide_tocontents"	"0"
					"paintbackground"	"0"
					"paintborder"		"1"
					"RoundedCorners"	"0"
					"border_default"			"ReplayDefaultBorder"
					"defaultFgColor_override"	"TanLight"
					"armedFgColor_override"		"Orange"

					"command"	"mm_casual_open"
				}

				"LoanerContainerPanel"
				{
					"ControlName"	"EditablePanel"
					"fieldName"		"LoanerContainerPanel"
					"xpos"			"-p0.01"
					"ypos"			"p0.52"
					"zpos"			"101"
					"wide"			"p0.53"
					"tall"			"o1"
					"visible"		"1"
					"enabled"		"1"
					"proportionaltoparent"	"1"

					"RequiredContractItemsLabel"
					{
						"ControlName"			"Label"
						"fieldName"				"RequiredContractItemsLabel"
						"font"					"QuestFlavorText"
						"labelText"				"#TF_Quest_RequiredItems"
						"textAlignment"			"north-west"
						"xpos"					"p.1"
						"ypos"					"p.19"
						"zpos"					"1"
						"wide"					"f0"
						"tall"					"15"
						"autoResize"			"0"
						"pinCorner"				"0"
						"visible"				"1"
						"enabled"				"1"
						"wrap"					"1"
						"proportionaltoparent"	"1"

						"fgcolor"				"170 25 25 255"
						"fgcolor_override"		"170 25 25 255"
					}

					"LoanersBGImage"
					{
						"ControlName"	"ImagePanel"
						"fieldName"		"LoanersBGImage"
						"xpos"			"0"
						"ypos"			"0"
						"zpos"			"0"
						"wide"			"f0"
						"tall"			"f0"
						"visible"		"1"
						"enabled"		"1"
						"image"			"contracts/contracts_papers1_vacation_items"
						"scaleImage"	"1"	
						"proportionaltoparent"	"1"
					}

					"RequestLoanerItemsButton"
					{
						"ControlName"	"CExButton"
						"fieldName"		"RequestLoanerItemsButton"
						"xpos"			"p.1"
						"ypos"			"p.62"
						"zpos"			"1"
						"wide"			"p.77"
						"tall"			"15"
						"visible"		"1"
						"autoResize"	"0"
						"pinCorner"		"3"
						"visible"		"1"
						"enabled"		"1"
						"tabPosition"	"0"
						"proportionaltoparent"	"1"
						"labelText"		"#TF_Quest_RequestLoanerItems"
						"use_proportional_insets" "0"
						"font"			"QuestFlavorText"
						"allcaps"	"1"
						"textAlignment"	"center"
						"dulltext"		"0"
						"brighttext"	"0"
						"default"		"1"
						"actionsignallevel" "4"

						"sound_depressed"	"UI/buttonclick.wav"
						"sound_released"	"UI/buttonclickrelease.wav"

						"auto_wide_tocontents"	"0"
						"paintbackground"	"0"
						"paintborder"		"1"
						"RoundedCorners"	"0"
						"border_default"			"ReplayDefaultBorder"
						"defaultFgColor_override"	"TanLight"
						"armedFgColor_override"		"Orange"

						"command"	"request_loaner_items"
					}

					"EquipLoanerItemsButton"
					{
						"ControlName"	"CExButton"
						"fieldName"		"EquipLoanerItemsButton"
						"xpos"			"p.1"
						"ypos"			"p.62"
						"zpos"			"1"
						"wide"			"p.77"
						"tall"			"15"
						"visible"		"1"
						"autoResize"	"0"
						"pinCorner"		"3"
						"visible"		"1"
						"enabled"		"1"
						"tabPosition"	"0"
						"proportionaltoparent"	"1"
						"labelText"		"#TF_Quest_EquipLoanerItems"
						"use_proportional_insets" "0"
						"font"			"QuestFlavorText"
						"allcaps"	"1"
						"textAlignment"	"center"
						"dulltext"		"0"
						"brighttext"	"0"
						"default"		"1"
						"actionsignallevel" "4"

						"sound_depressed"	"UI/buttonclick.wav"
						"sound_released"	"UI/buttonclickrelease.wav"

						"auto_wide_tocontents"	"0"
						"paintbackground"	"0"
						"paintborder"		"1"
						"RoundedCorners"	"0"
						"border_default"			"ReplayDefaultBorder"
						"defaultFgColor_override"	"TanLight"
						"armedFgColor_override"		"Orange"

						"command"	"equip_loaner_items"
					}

					"Loaner1ItemModelPanel"
					{		
						"ControlName"	"CItemModelPanel"
						"fieldName"		"Loaner1ItemModelPanel"
						"xpos"			"p.1"
						"ypos"			"p.245"
						"zpos"			"2"
						"wide"			"55"
						"tall"			"55"
						"visible"		"0"
						"PaintBackgroundType"	"0"
						"paintborder"	"1"
						"proportionaltoparent"	"1"
		
						"model_xpos"	"4"
						"model_ypos"	"11"
						"model_wide"	"45"
						"model_tall"	"32"
						"name_only"		"0"
						"attrib_only"	"0"
						"model_only"	"1"
						"paint_icon_hide"	"0"
		
						"text_ypos"		"10"
		
						"itemmodelpanel"
						{
							"inventory_image_type"	"1"
							"allow_rot"				"0"
							"force_square_image"	"1"
						}
					}

					"Loaner2ItemModelPanel"
					{		
						"ControlName"	"CItemModelPanel"
						"fieldName"		"Loaner2ItemModelPanel"
						"xpos"			"p.51"
						"ypos"			"p.245"
						"zpos"			"2"
						"wide"			"55"
						"tall"			"55"
						"visible"		"0"
						"PaintBackgroundType"	"0"
						"paintborder"	"1"
						"proportionaltoparent"	"1"
		
						"model_xpos"	"4"
						"model_ypos"	"11"
						"model_wide"	"45"
						"model_tall"	"32"
						"name_only"		"0"
						"attrib_only"	"0"
						"model_only"	"1"
						"paint_icon_hide"	"0"
		
						"text_ypos"		"10"
		
						"itemmodelpanel"
						{
							"inventory_image_type"	"1"
							"allow_rot"				"0"
							"force_square_image"	"1"
						}
					}
				}

				"ScrollableBottomContainer"
				{
					"ControlName"	"CExScrollingEditablePanel"
					"fieldName"		"ScrollableBottomContainer"
					"xpos"					"p0.5"
					"ypos"					"55"
					"zpos"					"10"
					"wide"					"p0.42"
					"tall"					"220"
					"proportionaltoparent"	"1"
					"autoResize"			"0"
					"skip_autoresize"		"1"

					"allow_mouse_wheel_to_scroll" "1"
					"mouseinputenabled"		"1"
					"bottom_buffer"			"10"

					"ScrollBar"
					{
						"ControlName"	"ScrollBar"
						"FieldName"		"ScrollBar"
						"xpos"			"rs1"
						"ypos"			"0"
						"tall"			"f0"
						"wide"			"5" // This gets slammed from client schme.  GG.
						"zpos"			"1000"
						"nobuttons"		"1"
						"proportionaltoparent"	"1"

						"Slider"
						{
							"fgcolor_override"	"TanDark"
						}
		
						"UpButton"
						{
							"ControlName"	"Button"
							"FieldName"		"UpButton"
							"visible"		"0"
						}
		
						"DownButton"
						{
							"ControlName"	"Button"
							"FieldName"		"DownButton"
							"visible"		"0"
						}
					}

					"QuestExpirationWarning"
					{
						"ControlName"			"Label"
						"fieldName"				"QuestExpirationWarning"
						"font"					"QuestFlavorText"
						"labelText"				"%expiration%"
						"textAlignment"			"north-west"
						"xpos"					"0"
						"ypos"					"0"
						"zpos"					"1"
						"wide"					"f0"
						"tall"					"40"
						"autoResize"			"0"
						"pinCorner"				"0"
						"visible"				"1"
						"enabled"				"1"
						"wrap"					"1"
						"proportionaltoparent"	"1"
						"allcaps"				"1"

						"fgcolor"				"RedSolid"
						"fgcolor_override"		"RedSolid"
					}

					"QuestObjectiveExplanation"
					{
						"ControlName"			"Label"
						"fieldName"				"QuestObjectiveExplanation"
						"font"					"QuestFlavorText"
						"labelText"				"#TF_QuestObjective_Explanation"
						"textAlignment"			"north-west"
						"xpos"					"0"
						"ypos"					"0"
						"zpos"					"1"
						"wide"					"f0"
						"tall"					"40"
						"autoResize"			"0"
						"pinCorner"				"0"
						"visible"				"1"
						"enabled"				"1"
						"wrap"					"1"
						"proportionaltoparent"	"1"
						"allcaps"				"1"

						"fgcolor"				"Black"
						"fgcolor_override"		"Black"
					}

					"QuestFlavorText"
					{
						"ControlName"			"Label"
						"fieldName"				"QuestFlavorText"
						"font"					"QuestFlavorText"
						"labelText"				"%flavor_text%"
						"textAlignment"			"north-west"
						"xpos"					"0"
						"ypos"					"0"
						"zpos"					"1"
						"wide"					"f0"
						"tall"					"1000"
						"autoResize"			"0"
						"pinCorner"				"0"
						"visible"				"1"
						"enabled"				"1"
						"wrap"					"1"
						"proportionaltoparent"	"1"

						"fgcolor"				"Black"
						"fgcolor_override"		"Black"
					}
				}
			}


			"BackFolderContainer"
			{
				"ControlName"	"EditablePanel"
				"fieldName"		"BackFolderContainer"
				"xpos"		"0"
				"ypos"		"0"
				"zpos"		"-5"
				"wide"		"f0"
				"tall"		"f0"
				"autoResize"		"0"
				"pinCorner"		"0"
				"visible"		"1"
				"enabled"		"1"
				"tabPosition"		"0"
				"proportionaltoparent"	"1"

				"BackFolderImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"BackFolderImage"
					"xpos"			"0"
					"ypos"			"0"
					"zpos"			"0"
					"wide"			"f0"
					"tall"			"o1"
					"visible"		"1"
					"enabled"		"1"
					"scaleImage"	"1"
					"image"			"contracts\contracts_folder1_back"
					"proportionaltoparent"	"1"

					"mouseinputenabled"	"0"
					"keyboardinputenabled" "0"
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
					"ypos"			"20"
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