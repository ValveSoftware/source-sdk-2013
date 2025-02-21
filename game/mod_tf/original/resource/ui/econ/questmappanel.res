"Resource/UI/QuestMapPanel.res"
{
	"QuestMap"
	{
		"ControlName"	"CQuestMapPanel"
		"fieldName"		"QuestMap"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"100"
		"wide"			"f0"
		"tall"			"f0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"	"0"
		"PaintBackgroundType"	"0"
		"paintbackground"		"0"
		"skip_autoresize"	"1"

		"MouseBlocker"
		{
			"ControlName"	"Panel"
			"fieldName"		"MouseBlocker"
			"xpos"		"0"
			"ypos"		"0"
			"zpos"		"10000"
			"wide"		"f0"
			"tall"		"f0"
			"autoResize"		"0"
			"pinCorner"		"0"
			"visible"		"0"
			"enabled"		"1"
			"tabPosition"		"0"
			"mouseinputenabled"	"1"
		}

		"RewardItemKV"
		{
			"wide"	"250"
			"tall"	"60"
		}

		"mouseoveritempanel"
		{
			"ControlName"	"CItemModelPanel"
			"fieldName"		"mouseoveritempanel"
			"xpos"			"c-70"
			"ypos"			"270"
			"zpos"			"1000"
			"wide"			"300"
			"tall"			"300"
			"visible"		"0"
			"bgcolor_override"		"0 0 0 0"
			"noitem_textcolor"		"117 107 94 255"
			"PaintBackgroundType"	"2"
			"paintborder"	"1"
			"border"		"MainMenuBGBorder"
		
			"text_center"		"1"
			"model_hide"		"1"
			"resize_to_text"	"1"
			"padding_height"	"15"
		
			"attriblabel"
			{
				"font"			"ItemFontAttribLarge"
				"xpos"			"0"
				"ypos"			"30"
				"zpos"			"2"
				"wide"			"140"
				"tall"			"60"
				"autoResize"	"0"
				"pinCorner"		"0"
				"visible"		"1"
				"enabled"		"1"
				"labelText"		"%attriblist%"
				"textAlignment"	"center"
				"fgcolor"		"117 107 94 255"
				"centerwrap"	"1"
			}
		}

		"TooltipPanel"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"TooltipPanel"
			"xpos"			"0"
			"ypos"			"0"
			"zpos"			"1001"
			"wide"			"240"
			"tall"			"50"
			"visible"		"0"
			"PaintBackgroundType"	"2"
			"border"		"MainMenuBGBorder"
		
			"TipLabel"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"TipLabel"
				"font"			"HudFontSmallest"
				"labelText"		"%tiptext%"
				"textAlignment"	"center"
				"xpos"			"20"
				"ypos"			"10"
				"zpos"			"2"
				"wide"			"200"
				"tall"			"30"
				"autoResize"	"0"
				"pinCorner"		"0"
				"visible"		"1"
				"enabled"		"1"
				"fgcolor_override"	"235 226 202 255"
				"wrap"			"1"
			}

			"QuestObjective"
			{
				"fieldName"	"QuestObjective"
				"wide"		"200"
				"zpos"		"1002"
			}
		}	

		"Dimmer"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"Dimmer"
			"xpos"		"0"
			"ypos"		"0"
			"zpos"		"-2"
			"wide"		"f0"
			"tall"		"f0"
			"autoResize"		"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"		"0"
			"bgcolor_override"	"20 15 5 230"
		}

		"OutsideCloseButton"
		{
			"ControlName"	"Button"
			"fieldName"		"OutsideCloseButton"
			"xpos"		"0"
			"ypos"		"0"
			"zpos"		"-1"
			"wide"		"f0"
			"tall"		"f0"
			"autoResize"		"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"		"0"
			"labelText"			""
			"bgcolor_override"	"0 0 0 220"
			"command"	"close"
			"paintbackground"	"0"
			"paintborder"		"0"
		}

		"MainContainer"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"MainContainer"
			"xpos"			"cs-0.5"
			"ypos"			"cs-0.5-20"
			"zpos"			"1"
			"wide"			"700"
			"tall"			"700"
			"skip_autoresize"	"1"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"	"0"
			"paintborder"		"0"

			"CloseButton"
			{
				"ControlName"	"CExImageButton"
				"fieldName"		"CloseButton"
				"xpos"			"c280"
				"ypos"			"175"
				"zpos"			"10"
				"wide"			"14"
				"tall"			"14"
				"autoResize"	"0"
				"pinCorner"		"0"
				"visible"		"1"
				"enabled"		"1"
				"tabPosition"	"0"
				"labeltext"		""
				"font"			"HudFontSmallBold"
				"textAlignment"	"center"
				"dulltext"		"0"
				"brighttext"	"0"
				"default"		"0"
				"sound_depressed"	"UI/buttonclick.wav"
				"sound_released"	"UI/buttonclickrelease.wav"
				"Command"		"close"
				"proportionaltoparent" "1"
				"actionsignallevel"	"2"
			
				"paintbackground"	"0"
			
				"defaultFgColor_override" "46 43 42 255"
				"armedFgColor_override" "200 80 60 255"
				"depressedFgColor_override" "46 43 42 255"
			
				"image_drawcolor"	"200 80 60 255"
				"image_armedcolor"	"255 80 60 255"
				"SubImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"SubImage"
					"xpos"			"0"
					"ypos"			"0"
					"zpos"			"1"
					"wide"			"14"
					"tall"			"14"
					"visible"		"1"
					"enabled"		"1"
					"image"			"close_button"
					"scaleImage"	"1"
				}				
			} // Close button

			"PowerSwitchButton"
			{
				"ControlName"	"CExImageButton"
				"fieldName"		"PowerSwitchButton"
				"xpos"			"c190"
				"ypos"			"497"
				"zpos"			"1000"
				"wide"			"o1"
				"tall"			"88"
				"autoResize"	"0"
				"pinCorner"		"0"
				"visible"		"1"
				"enabled"		"1"
				"tabPosition"	"0"
				"font"			"HudFontSmallBold"
				"textAlignment"	"center"
				"dulltext"		"0"
				"brighttext"	"0"
				"default"		"0"
				"labeltext"		""
				"sound_depressed"	"ui/cyoa_switch.wav"
				"sound_released"	"ui/cyoa_switch.wav"
				"Command"		"anim_close"
				"proportionaltoparent" "1"
				"actionsignallevel"	"2"
				"button_activation_type"	"1"

				"paintbackground"	"0"

				"image_default"		"cyoa/cyoa_pda_switch_on"				
				"image_selected"		"cyoa/cyoa_pda_switch_off"				
				"image_armed"		"cyoa/cyoa_pda_switch_on"	
							
				"SubImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"SubImage"
					"xpos"			"0"
					"ypos"			"0"
					"zpos"			"1"
					"wide"			"o1"
					"tall"			"p1"
					"visible"		"1"
					"enabled"		"1"
					"image"			"glyph_store"
					"scaleImage"	"1"
					"proportionaltoparent" "1"
				}				
			} // PowerSwitchButton

			"PowerLabel"
			{
				"ControlName"	"Label"
				"fieldName"		"PowerLabel"
				"labeltext"		"#TF_QuestMap_Power"
				"xpos"			"c178"
				"ypos"			"565"
				"wide"			"100"
				"tall"			"15"
				"zpos"			"1001"
				"font"			"QuestMap_Small"
				"TextAlignment"		"north"
				"proportionaltoparent" "1"
				"mouseinputenabled"		"0"
				"fgcolor_override"	"255 255 255 100"
			}

			"MapStoreLabel"
			{
				"ControlName"	"Label"
				"fieldName"		"MapStoreLabel"
				"labeltext"		"#TF_QuestMap_Map_Title"
				"xpos"			"c-285"
				"ypos"			"555"
				"wide"			"100"
				"tall"			"15"
				"zpos"			"1001"
				"font"			"QuestMap_Small"
				"TextAlignment"		"north"
				"proportionaltoparent" "1"
				"mouseinputenabled"		"0"
				"fgcolor_override"	"255 255 255 100"
			}

			"MapButton"
			{
				"ControlName"	"CExImageButton"
				"fieldName"		"MapButton"
				"xpos"			"c-270"
				"ypos"			"510"
				"zpos"			"1000"
				"wide"			"o1.75"
				"tall"			"43"
				"autoResize"	"0"
				"pinCorner"		"0"
				"visible"		"1"
				"enabled"		"1"
				"tabPosition"	"0"
				"font"			"HudFontSmallBold"
				"textAlignment"	"center"
				"dulltext"		"0"
				"brighttext"	"0"
				"default"		"0"
				"labeltext"		""
				"sound_depressed"	"UI/buttonclick.wav"
				"sound_released"	"UI/buttonclickrelease.wav"
				"Command"		"view_map"
				"proportionaltoparent" "1"
				"actionsignallevel"	"2"
			
				"paintbackground"	"0"

				"image_default"		"cyoa/cyoa_pda_button_off_map"				
				"image_selected"		"cyoa/cyoa_pda_button_on_map"				
				"image_armed"		"cyoa/cyoa_pda_button_over_map"				
				"SubImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"SubImage"
					"xpos"			"-2"
					"ypos"			"-4"
					"zpos"			"1"
					"wide"			"o1.81"
					"tall"			"p1.32"
					"visible"		"1"
					"enabled"		"1"
					"image"			"glyph_store"
					"scaleImage"	"1"
					"proportionaltoparent" "1"
				}				
			} // MapButton

			"RewardsStoreLabel"
			{
				"ControlName"	"Label"
				"fieldName"		"RewardsStoreLabel"
				"labeltext"		"#TF_QuestMap_RewardStore_Title"
				"xpos"			"c-197"
				"ypos"			"555"
				"wide"			"100"
				"tall"			"15"
				"zpos"			"1001"
				"font"			"QuestMap_Small"
				"TextAlignment"		"north"
				"proportionaltoparent" "1"
				"mouseinputenabled"		"0"
				"fgcolor_override"	"255 255 255 100"
			}

			"RewardsStoreButton"
			{
				"ControlName"	"CExImageButton"
				"fieldName"		"RewardsStoreButton"
				"xpos"			"c-185"
				"ypos"			"510"
				"zpos"			"1000"
				"wide"			"o1.75"
				"tall"			"43"
				"autoResize"	"0"
				"pinCorner"		"0"
				"visible"		"1"
				"enabled"		"1"
				"tabPosition"	"0"
				"font"			"HudFontSmallBold"
				"textAlignment"	"center"
				"dulltext"		"0"
				"brighttext"	"0"
				"default"		"0"
				"labeltext"		""
				"sound_depressed"	"UI/buttonclick.wav"
				"sound_released"	"UI/buttonclickrelease.wav"
				"Command"		"rewards_store"
				"proportionaltoparent" "1"
				"actionsignallevel"	"2"
			
				"paintbackground"	"0"

				"image_default"		"cyoa/cyoa_pda_button_off_store"				
				"image_selected"		"cyoa/cyoa_pda_button_on_store"				
				"image_armed"		"cyoa/cyoa_pda_button_over_store"				
				"SubImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"SubImage"
					"xpos"			"-2"
					"ypos"			"-4"
					"zpos"			"1"
					"wide"			"o1.81"
					"tall"			"p1.32"
					"visible"		"1"
					"enabled"		"1"
					"image"			"glyph_store"
					"scaleImage"	"1"
					"proportionaltoparent" "1"
				}				
			} // RewardsStoreButton

			"DebugButton"
			{
				"ControlName"	"CExImageButton"
				"fieldName"		"DebugButton"
				"xpos"			"55"
				"ypos"			"175"
				"zpos"			"10"
				"wide"			"14"
				"tall"			"14"
				"autoResize"	"0"
				"pinCorner"		"0"
				"visible"		"0"
				"enabled"		"1"
				"tabPosition"	"0"
				"labeltext"		""
				"font"			"HudFontSmallBold"
				"textAlignment"	"center"
				"dulltext"		"0"
				"brighttext"	"0"
				"default"		"0"
				"sound_depressed"	"UI/buttonclick.wav"
				"sound_released"	"UI/buttonclickrelease.wav"
				"Command"		"debug_menu"
				"proportionaltoparent" "1"
				"actionsignallevel"	"2"
			
				"paintbackground"	"0"
			
				"defaultFgColor_override" "46 43 42 255"
				"armedFgColor_override" "200 80 60 255"
				"depressedFgColor_override" "46 43 42 255"
			
				"image_drawcolor"	"117 107 94 255"
				"image_armedcolor"	"200 80 60 255"
				"SubImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"SubImage"
					"xpos"			"0"
					"ypos"			"0"
					"zpos"			"1"
					"wide"			"14"
					"tall"			"14"
					"visible"		"1"
					"enabled"		"1"
					"image"			"glyph_workshop_edit"
					"scaleImage"	"1"
				}				
			} // Debug button

			"ScreenBorder"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"ScreenBorder"
				"xpos"			"cs-0.5-15"
				"ypos"			"cs-0.5+30"
				"zpos"			"1"
				"wide"			"700"
				"tall"			"700"
				"visible"		"1"
				"proportionaltoparent"	"1"
				"mouseinputenabled"		"0"
				"keyboardinputenabled"	"0"

				"image"			"cyoa/cyoa_pda"
				"scaleimage"	"1"
			}

			"MapAreaPanel"
			{
				"ControlName"	"EditablePanel"
				"fieldName"		"MapAreaPanel"
				"xpos"			"cs-0.5"
				"ypos"			"cs-0.5"
				"wide"			"540"
				"tall"			"315"
				"zpos"			"0"
				"proportionaltoparent" "1"
				"mouseinputenabled"	"1"

				"Introduction"
				{
					"ControlName"	"EditablePanel"
					"fieldName"		"Introduction"
					"xpos"			"0"
					"ypos"			"0"
					"zpos"			"8000"
					"wide"			"f0"
					"tall"			"f0"
					"visible"		"1"
					"proportionaltoparent"	"1"

					"StaticBG"
					{
						"ControlName"	"ImagePanel"
						"fieldName"		"StaticBG"
						"xpos"			"0"
						"ypos"			"0"
						"zpos"			"0"
						"wide"			"f0"
						"tall"			"f0"
						"visible"		"1"
						"PaintBackgroundType"	"0"
						"proportionaltoparent"	"1"
						"mouseinputenabled"		"0"
						"keyboardinputenabled"	"0"

						"alpha"			"255"
						"image"			"..\models\passtime\tv\passtime_tv_screen_static"
						"tileImage"	"1"
					}

					"StaticDarken"
					{
						"ControlName"	"EditablePanel"
						"fieldName"		"StaticDarken"
						"xpos"			"0"
						"ypos"			"0"
						"zpos"			"0"
						"wide"			"f0"
						"tall"			"f0"
						"visible"		"1"
						"proportionaltoparent"	"1"

						"bgcolor_override"	"0 0 0 240"
					}

					"VideoPanel"
					{
						"ControlName"	"VideoPanel"
						"fieldName"		"VideoPanel"
						"xpos"			"r130-s0.5"
						"ypos"			"50"
						"zpos"			"100"
						"wide"			"o1"
						"tall"			"200"
						"proportionaltoparent"	"1"
						"loop"			"1"
						"visible"		"0"
					}

					"IntroTitle"
					{
						"ControlName"	"Label"
						"fieldName"		"IntroTitle"
						"labeltext"		"#TF_QuestMap_Intro_Title"
						"xpos"					"20"
						"ypos"			"20"
						"wide"			"f0"
						"tall"			"100"
						"zpos"			"1"
						"font"			"HudFontMediumBold"
						"TextAlignment"		"north-west"
						"proportionaltoparent" "1"
						"mouseinputenabled"		"0"
						"fgcolor_override"	"QuestMap_ActiveOrange"
					}

					"IntroDesc"
					{
						"ControlName"	"Label"
						"fieldName"		"IntroDesc"
						"labeltext"		"#TF_QuestMap_Intro_Desc"
						"xpos"					"0"
						"ypos"			"50"
						"wide"			"p0.5"
						"tall"			"100"
						"zpos"			"1"
						"font"			"HudFontSmallest"
						"TextAlignment"		"north-west"
						"proportionaltoparent" "1"
						"mouseinputenabled"		"0"
						"fgcolor_override"	"TanLight"
						"wrap"	"1"
						"textinsetx"	"23"
						"use_proportional_insets"	"1"
					}

					"HoverButtonStage1"
					{
						"fieldName"		"HoverButtonStage1"
						"xpos"			"23"
						"ypos"			"135"
						"zpos"			"1"
						"wide"			"230"
						"tall"			"35"
						"RoundedCorners"	"0"
						"labelText"		"#TF_QuestMap_Intro_Step1Button"
						"font"			"HudFontSmallBold"
						"textAlignment"	"center"
						"stay_armed_on_click"	"1"

						"defaultBgColor_override"	"TanDark"
						"armedBgColor_override"		"QuestMap_ActiveOrange"

						"image_drawcolor"	"255 255 255 255"
						"image_armedcolor"	"0 0 0 0"
						"SubImage"
						{
							"ControlName"	"ImagePanel"
							"fieldName"		"SubImage"
							"xpos"			"rs1-5"
							"ypos"			"rs1-5"
							"zpos"			"1"
							"wide"			"10"
							"tall"			"10"
							"visible"		"1"
							"enabled"		"1"
							"scaleImage"	"1"
							"image"			"info"
							"proportionaltoparent"	"1"
						}				
					}

					"HoverButtonStage2"
					{
						"fieldName"		"HoverButtonStage2"
						"xpos"			"23"
						"ypos"			"185"
						"zpos"			"1"
						"wide"			"230"
						"tall"			"35"
						"RoundedCorners"	"0"
						"labelText"		"#TF_QuestMap_Intro_Step2Button"
						"font"			"HudFontSmallBold"
						"textAlignment"	"center"
						"stay_armed_on_click"	"1"

						"defaultBgColor_override"	"TanDark"
						"armedBgColor_override"		"QuestMap_ActiveOrange"

						"image_drawcolor"	"255 255 255 255"
						"image_armedcolor"	"0 0 0 0"
						"SubImage"
						{
							"ControlName"	"ImagePanel"
							"fieldName"		"SubImage"
							"xpos"			"rs1-5"
							"ypos"			"rs1-5"
							"zpos"			"1"
							"wide"			"10"
							"tall"			"10"
							"visible"		"1"
							"enabled"		"1"
							"scaleImage"	"1"
							"image"			"info"
							"proportionaltoparent"	"1"
						}				
					}

					"HoverButtonStage3"
					{
						"fieldName"		"HoverButtonStage3"
						"xpos"			"23"
						"ypos"			"235"
						"zpos"			"1"
						"wide"			"230"
						"tall"			"35"
						"RoundedCorners"	"0"
						"labelText"		"#TF_QuestMap_Intro_Step3Button"
						"font"			"HudFontSmallBold"
						"textAlignment"	"center"
						"stay_armed_on_click"	"1"

						"defaultBgColor_override"	"TanDark"
						"armedBgColor_override"		"QuestMap_ActiveOrange"

						"image_drawcolor"	"255 255 255 255"
						"image_armedcolor"	"0 0 0 0"
						"SubImage"
						{
							"ControlName"	"ImagePanel"
							"fieldName"		"SubImage"
							"xpos"			"rs1-5"
							"ypos"			"rs1-5"
							"zpos"			"1"
							"wide"			"10"
							"tall"			"10"
							"visible"		"1"
							"enabled"		"1"
							"scaleImage"	"1"
							"image"			"info"
							"proportionaltoparent"	"1"
						}				
					}

					"IntroStage0"
					{
						"ControlName"	"EditablePanel"
						"fieldName"		"IntroStage0"
						"xpos"			"cs-0.5"
						"ypos"			"cs-0.5"
						"zpos"			"1"
						"wide"			"f0"
						"wide"			"f0"
						"tall"			"f0"
						"proportionaltoparent"	"1"
						"mouseinputenabled"	"1"
						"keyboardinputenabled"	"0"

						"StartButton"
						{
							"ControlName"	"CExButton"
							"fieldName"		"StartButton"
							"xpos"			"350"
							"ypos"			"cs-0.5"
							"zos"			"2"
							"wide"			"100"
							"tall"			"20"
							"visible"		"1"
							"enabled"		"1"
							"textAlignment"	"center"	
							"labelText"		"#TF_QuestMap_Intro_OK"
							"textinsetx"	"4"
							"use_proportional_insets" "1"
							"font"			"HudFontSmallBold"
							"textAlignment"	"west"
							"dulltext"		"0"
							"brighttext"	"0"
							"default"		"1"
							"command"		"endintro"
							"proportionaltoparent" "1"
							"actionsignallevel"	"5"

							"paintbackground"	"1"

							"sound_depressed"	"UI/buttonclick.wav"
							"sound_released"	"UI/buttonclickrelease.wav"

							"defaultFgColor_override"	"TanLight"
							"armedFgColor_override"		"TanLight"
							"depressedFgColor_override" "TanLight"

							"defaultBgColor_override"	"StoreGreen"
							"armedBgColor_override"		"CreditsGreen"
							"depressedBgColor_override" "CreditsGreen"
						}	
					}

					"IntroStage1"
					{
						"ControlName"	"EditablePanel"
						"fieldName"		"IntroStage1"
						"xpos"			"cs-0.5"
						"ypos"			"cs-0.5"
						"zpos"			"1"
						"wide"			"f0"
						"tall"			"f0"
						"visible"		"0"
						"proportionaltoparent"	"1"
						"mouseinputenabled"	"0"
						"keyboardinputenabled"	"0"

						"StageDesc"
						{
							"ControlName"	"Label"
							"fieldName"		"StageDesc"
							"labeltext"		"#TF_QuestMap_Intro_1"
							"xpos"			"p0.5"
							"ypos"			"210"
							"wide"			"p0.5"
							"tall"			"100"
							"zpos"			"1"
							"font"			"HudFontSmallest"
							"TextAlignment"		"north-west"
							"proportionaltoparent" "1"
							"mouseinputenabled"		"0"
							"fgcolor_override"	"QuestMap_ActiveOrange"
							"wrap"	"1"
							"textinsetx"	"23"
							"use_proportional_insets"	"1"
						}
					}

					"IntroStage2"
					{
						"ControlName"	"EditablePanel"
						"fieldName"		"IntroStage2"
						"xpos"			"cs-0.5"
						"ypos"			"cs-0.5"
						"zpos"			"1"
						"wide"			"f0"
						"tall"			"f0"
						"visible"		"0"
						"proportionaltoparent"	"1"
						"mouseinputenabled"	"0"
						"keyboardinputenabled"	"0"

						"StageDesc"
						{
							"ControlName"	"Label"
							"fieldName"		"StageDesc"
							"labeltext"		"#TF_QuestMap_Intro_2"
							"xpos"			"p0.5"
							"ypos"			"210"
							"wide"			"p0.5"
							"tall"			"100"
							"zpos"			"1"
							"font"			"HudFontSmallest"
							"TextAlignment"		"north-west"
							"proportionaltoparent" "1"
							"mouseinputenabled"		"0"
							"fgcolor_override"	"TanLight"
							"wrap"	"1"
							"textinsetx"	"23"
							"use_proportional_insets"	"1"
						}
					}

					"IntroStage3"
					{
						"ControlName"	"EditablePanel"
						"fieldName"		"IntroStage3"
						"xpos"			"cs-0.5"
						"ypos"			"cs-0.5"
						"zpos"			"1"
						"wide"			"f0"
						"wide"			"f0"
						"tall"			"f0"
						"visible"		"0"
						"proportionaltoparent"	"1"
						"mouseinputenabled"	"0"
						"keyboardinputenabled"	"0"

						"StageDesc"
						{
							"ControlName"	"Label"
							"fieldName"		"StageDesc"
							"labeltext"		"#TF_QuestMap_Intro_3"
							"xpos"			"p0.5"
							"ypos"			"210"
							"wide"			"p0.45"
							"tall"			"100"
							"zpos"			"1"
							"font"			"HudFontSmallest"
							"TextAlignment"		"north-west"
							"proportionaltoparent" "1"
							"mouseinputenabled"		"0"
							"fgcolor_override"	"TanLight"
							"wrap"	"1"
							"textinsetx"	"23"
							"use_proportional_insets"	"1"
						}
					}
				}

				"CyclingAd"
				{
					"ControlName"			"CCyclingAdContainerPanel"
					"fieldName"				"CyclingAd"
					"xpos"					"rs1-10"
					"ypos"					"rs1-30"
					"zpos"					"9"
					"wide"					"260"
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
							"item"		"Jungle Inferno Contracts Pass"
							"show_market"	"0"
						}
					}
				}

				"TurnInCompletePopup"
				{
					"ControlName"	"EditablePanel"
					"fieldName"		"TurnInCompletePopup"
					"xpos"			"cs-0.5"
					"ypos"			"cs-0.5"
					"zpos"			"300"
					"wide"			"250"
					"tall"			"150"
					"visible"		"0"
					"proportionaltoparent" "1"
					"mouseinputenabled"		"0"

					"border"		"CYOANodeViewBorder"

					"BorderOverlay"
					{
						"ControlName"	"Panel"
						"fieldName"		"BorderOverlay"
						"xpos"			"0"
						"ypos"			"0"
						"zpos"			"300"
						"wide"			"f0"
						"tall"			"f0"
						"proportionaltoparent" "1"

						"border"		"CYOANodeViewBorder_Active"
					}

					"CheckImage"
					{
						"ControlName"	"ImagePanel"
						"fieldName"		"CheckImage"
						"xpos"			"cs-0.5"
						"ypos"			"25"
						"zpos"			"300"
						"wide"			"o1"
						"tall"			"70"
						"proportionaltoparent" "1"

						"image"			"cyoa/check"
						"scaleimage"	"1"
						"drawcolor_override"	"QuestMap_ActiveOrange"
					}

					"BodyText"
					{
						"ControlName"	"Label"
						"fieldName"		"BodyText"
						"xpos"			"0"
						"ypos"			"90"
						"zpos"			"300"
						"wide"			"f0"
						"tall"			"300"
						"proportionaltoparent" "1"
						"fgcolor_override"	"QuestMap_ActiveOrange"
						"labeltext"		"%result%"
						"TextAlignment"		"north"
						"font"	"QuestMap_Huge"
						"centerwrap"	"1"
					}
				}

				"GlobalStatus"
				{
					"ControlName"	"EditablePanel"
					"fieldName"		"GlobalStatus"
					"xpos"			"0"
					"ypos"			"rs1"
					"zpos"			"3"
					"wide"			"f0"
					"tall"			"30"
					"proportionaltoparent" "1"
					"mouseinputenabled"		"1"

					"border"		"QuickplayBorder"
					"bgcolor_override"	"0 0 0 240"

					"BloodMoneyTooltip"
					{
						"ControlName"	"Panel"
						"fieldName"		"BloodMoneyTooltip"
						"xpos"			"20"
						"ypos"			"0"
						"zpos"			"100"
						"wide"			"60"
						"tall"			"50"
						"paintbackground"	"0"
						"paintborder"		"0"
					}

					"RewardCreditImage"
					{
						"ControlName"	"ImagePanel"
						"fieldName"		"RewardCreditImage"
						"xpos"			"20"
						"ypos"			"-1"
						"zpos"			"0"
						"wide"			"o1"
						"tall"			"26"
						"visible"		"1"
						"PaintBackgroundType"	"0"
						"proportionaltoparent"	"1"
						"mouseinputenabled"		"0"
						"keyboardinputenabled"	"0"

						"image"			"cyoa/cyoa_cash_large"
						"scaleimage"	"1"
					}

					"RewardCreditsLabel"
					{
						"ControlName"	"Label"
						"fieldName"		"RewardCreditsLabel"
						"labeltext"		"%reward_credits%"
						"xpos"			"47"
						"ypos"			"7"
						"wide"			"140"
						"tall"			"10"
						"zpos"			"1"
						"font"			"QuestMap_Small"
						"TextAlignment"		"north-west"
						"proportionaltoparent" "1"
						"mouseinputenabled"		"0"
					}

					"StarsAvailableTooltip"
					{
						"ControlName"	"Panel"
						"fieldName"		"StarsAvailableTooltip"
						"xpos"			"94"
						"ypos"			"0"
						"zpos"			"100"
						"wide"			"40"
						"tall"			"50"
						"paintbackground"	"0"
						"paintborder"		"0"
					}

					"AvailableStarsImage"
					{
						"ControlName"	"ImagePanel"
						"fieldName"		"AvailableStarsImage"
						"xpos"			"94"
						"ypos"			"2"
						"zpos"			"0"
						"wide"			"o1"
						"tall"			"20"
						"visible"		"1"
						"PaintBackgroundType"	"0"
						"proportionaltoparent"	"1"
						"mouseinputenabled"		"0"
						"keyboardinputenabled"	"0"

						"image"			"cyoa/star_on"
						"scaleimage"	"1"
					}

					"AvailableStarsLabel"
					{
						"ControlName"	"Label"
						"fieldName"		"AvailableStarsLabel"
						"labeltext"		"%stars_available%"
						"xpos"			"115"
						"ypos"			"7"
						"wide"			"140"
						"tall"			"10"
						"zpos"			"1"
						"font"			"QuestMap_Small"
						"TextAlignment"		"north-west"
						"proportionaltoparent" "1"
						"mouseinputenabled"		"0"
					}

					"TotalStarsTooltip"
					{
						"ControlName"	"Panel"
						"fieldName"		"TotalStarsTooltip"
						"xpos"			"r70"
						"ypos"			"0"
						"zpos"			"100"
						"wide"			"40"
						"tall"			"50"
						"proportionaltoparent"	"1"
						"paintbackground"	"0"
						"paintborder"		"0"
					}

					"TotalStarsImage"
					{
						"ControlName"	"ImagePanel"
						"fieldName"		"TotalStarsImage"
						"xpos"			"r70"
						"ypos"			"2"
						"zpos"			"0"
						"wide"			"o1"
						"tall"			"20"
						"visible"		"1"
						"PaintBackgroundType"	"0"
						"proportionaltoparent"	"1"
						"mouseinputenabled"		"0"
						"keyboardinputenabled"	"0"

						"image"			"cyoa/star_off"
						"scaleimage"	"1"
					}

					"TotalStarsLabel"
					{
						"ControlName"	"Label"
						"fieldName"		"TotalStarsLabel"
						"labeltext"		"%stars_total%"
						"xpos"			"r50"
						"ypos"			"7"
						"wide"			"140"
						"tall"			"10"
						"zpos"			"1"
						"font"			"QuestMap_Small"
						"TextAlignment"		"north-west"
						"proportionaltoparent" "1"
						"mouseinputenabled"		"0"
					}
				}

				"SelectedNodeInfoPanel"
				{
					"fieldName"		"SelectedNodeInfoPanel"
					"xpos"			"0"
					"ypos"			"0"
					"zpos"			"10"
					"wide"			"200"
					"tall"			"220"
					"visible"		"0"
					"enabled"		"1"
					"proportionaltoparent" "1"
					"mouseinputenabled"		"1"
					"keyboardinputenabled"	"0"
					"border"	"CYOANodeViewBorder"
					"collapsed_height"	"165"
					"expanded_height"	"220"
				}

				"QuestObjective"
				{
					"fieldName"	"QuestObjective"
					"wide"		"200"
					"zpos"		"11"
					"visible"	"0"
					"mouseinputenabled"	"0"

					"border"	"CYOAPopupBorder"
					"bgcolor_override"	"37 37 37 255"

					"ObjectivesLabel"
					{
						"fieldName"	"ObjectivesLabel"
						"ControlName"	"Label"
						"xpos"		"2"
						"ypos"		"2"
						"zpos"		"100"
						"wide"		"f2"
						"tall"		"20"
						"labelText"	"#TF_QuestView_Objectives"
						"visible"	"1"
						"font"		"QuestMap_Medium"
						"fgcolor_override"	"75 75 75 255"
						"bgcolor_override"	"0 0 0 255"
						"textAlignment"	"west"
						"textinsetx"	"5"
					}

					"ItemTrackerPanel"
					{
						"fieldName"		"ItemTrackerPanel"
						"xpos"			"0"	
						"ypos"			"25"
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
							"wide"			"f15"
							"tall"			"6"
							"zpos"			"4"
							"visible"		"1"
							"enabled"		"1"
							"proportionaltoparent" "1"

							"bgcolor_override"		"250 234 201 51"

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

								"bgcolor_override"		"251 235 202 255"

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
				}

				"RewardsShop"
				{
					"ControlName"	"EditablePanel"
					"fieldName"		"RewardsShop"
					"xpos"			"cs-0.5"
					"ypos"			"cs-0.5"
					"zpos"			"2"
					"wide"			"p1"
					"tall"			"p1"
					"visible"		"0"
					"proportionaltoparent" "1"

					"BlackBG"
					{
						"ControlName"	"Panel"
						"fieldName"		"BlackBG"
						"xpos"			"0"
						"ypos"			"0"
						"zpos"			"-2"
						"wide"			"f0"
						"tall"			"f0"
						"visible"		"1"
						"PaintBackgroundType"	"0"
						"proportionaltoparent"	"1"
						"mouseinputenabled"		"0"
						"keyboardinputenabled"	"0"

						"bgcolor_override"	"0 0 0 255"
					}

					"Dimmer"
					{
						"ControlName"	"Panel"
						"fieldName"		"Dimmer"
						"xpos"			"0"
						"ypos"			"0"
						"zpos"			"-1"
						"wide"			"f0"
						"tall"			"f0"
						"visible"		"1"
						"PaintBackgroundType"	"0"
						"proportionaltoparent"	"1"
						"mouseinputenabled"		"0"
						"keyboardinputenabled"	"0"
						"alpha"			"100"
					
						"bgcolor_override" "0 0 0 255"
					}

					"TitleBorder"
					{
						"ControlName"	"Panel"
						"fieldName"		"TitleBorder"
						"xpos"			"0"
						"ypos"			"0"
						"zpos"			"-2"
						"wide"			"f0"
						"tall"			"50"
						"visible"		"1"
						"PaintBackgroundType"	"0"
						"proportionaltoparent"	"1"
						"mouseinputenabled"		"0"
						"keyboardinputenabled"	"0"

						"border"		"ReplayDefaultBorder"
					}

					"Title"
					{
						"ControlName"	"Label"
						"fieldName"		"Title"
						"xpos"			"cs-0.5"
						"ypos"			"20"
						"zpos"			"10"
						"wide"			"300"
						"tall"			"14"
						"autoResize"	"0"
						"pinCorner"		"0"
						"visible"		"1"
						"enabled"		"1"
						"tabPosition"	"0"
						"labeltext"		"#TF_QuestMap_RewardStore_Title"
						"font"			"QuestLargeText"
						"textAlignment"	"center"
						"dulltext"		"0"
						"brighttext"	"0"
						"default"		"0"
						"proportionaltoparent" "1"
						"paintbackground"	"0"
					} // Title

					"Description"
					{
						"ControlName"	"Label"
						"fieldName"		"Description"
						"xpos"			"cs-0.5"
						"ypos"			"34"
						"zpos"			"10"
						"wide"			"f0"
						"tall"			"14"
						"autoResize"	"0"
						"pinCorner"		"0"
						"visible"		"1"
						"enabled"		"1"
						"tabPosition"	"0"
						"labeltext"		"#TF_QuestMap_RewardStore_Desc"
						"font"			"QuestMap_Small"
						"textAlignment"	"center"
						"dulltext"		"0"
						"brighttext"	"0"
						"default"		"0"
						"proportionaltoparent" "1"
						"paintbackground"	"0"
					} // Title

					"ItemsScroller"
					{
						"ControlName"	"CExScrollingEditablePanel"
						"fieldName"		"ItemsScroller"
						"xpos"			"cs-0.5"
						"ypos"			"50"
						"wide"			"p1"
						"tall"			"f50"
						"visible"		"1"
						"proportionaltoparent" "1"
						"mouseinputenabled"	"1"
						"bottom_buffer"	"50"
						"scroll_step"	"20"

						"ScrollBar"
						{
							"ControlName"	"ScrollBar"
							"FieldName"		"ScrollBar"
							"xpos"			"rs1-5"
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

						
					} // ItemsScroller
				} // RewardsShop

				"DisconnetedContainer"
				{
					"ControlName"	"EditablePanel"
					"fieldName"		"DisconnetedContainer"
					"xpos"			"0"
					"ypos"			"0"
					"wide"			"f0"
					"tall"			"f0"
					"zpos"			"50"
					"alpha"			"255"
					"proportionaltoparent" "1"
					"mouseinputenabled"	"0"

					"IntroDesc"
					{
						"ControlName"	"Label"
						"fieldName"		"IntroDesc"
						"labeltext"		"#TF_QuestMap_NoGC"
						"xpos"			"cs-0.5"
						"ypos"			"cs-0.5"
						"wide"			"p0.75"
						"tall"			"100"
						"zpos"			"1"
						"font"			"QuestMap_Large"
						"TextAlignment"		"center"
						"proportionaltoparent" "1"
						"mouseinputenabled"		"0"
						"fgcolor_override"	"TanLight"
						"wrap"	"0"
					}

					"StaticBackground"
					{
						"ControlName"	"ImagePanel"
						"fieldName"		"StaticBackground"
						"xpos"			"0"
						"ypos"			"0"
						"zpos"			"0"
						"wide"			"f0"
						"tall"			"f0"
						"visible"		"1"
						"PaintBackgroundType"	"0"
						"proportionaltoparent"	"1"
						"mouseinputenabled"		"0"
						"keyboardinputenabled"	"0"

						"alpha"			"255"
						"image"			"..\models\passtime\tv\passtime_tv_screen_static"
						"tileImage"	"1"
					}
				}

				"StaticBar1"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"StaticBar1"
					"xpos"			"0"
					"ypos"			"50"
					"zpos"			"9000"
					"wide"			"f0"
					"tall"			"50"
					"visible"		"1"
					"PaintBackgroundType"	"0"
					"proportionaltoparent"	"1"
					"mouseinputenabled"		"0"
					"keyboardinputenabled"	"0"

					"alpha"		"50"
					"image"			"..\overlays\black_gradient"
					"scaleimage"	"1"
					"rotation"	"3"
				}

				
				"StaticBar2"
				{
					"ControlName"	"EditablePanel"
					"fieldName"		"StaticBar2"
					"xpos"			"0"
					"ypos"			"120"
					"zpos"			"9000"
					"wide"			"f0"
					"tall"			"50"
					"visible"		"1"
					"PaintBackgroundType"	"0"
					"proportionaltoparent"	"1"
					"mouseinputenabled"		"0"
					"keyboardinputenabled"	"0"

					"bgcolor_override"	"255 255 255 3"
				}

				"BlackOverlay"
				{
					"ControlName"	"Panel"
					"fieldName"		"BlackOverlay"
					"xpos"			"0"
					"ypos"			"0"
					"zpos"			"8999"
					"wide"			"f0"
					"tall"			"f0"
					"visible"		"1"
					"PaintBackgroundType"	"0"
					"proportionaltoparent"	"1"
					"mouseinputenabled"		"0"
					"keyboardinputenabled"	"0"
					
					"bgcolor_override" "0 0 0 255"
				}

				"StaticOverlay"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"StaticOverlay"
					"xpos"			"0"
					"ypos"			"0"
					"zpos"			"9000"
					"wide"			"f0"
					"tall"			"f0"
					"visible"		"1"
					"PaintBackgroundType"	"0"
					"proportionaltoparent"	"1"
					"mouseinputenabled"		"0"
					"keyboardinputenabled"	"0"

					"alpha"			"20"
					"image"			"..\models\passtime\tv\passtime_tv_screen_static"
					"tileImage"	"1"
				}

				

			} // MapAreaPanel

		} // MainContainer
	}
}
