"Resource/UI/WarInfoPanel.res"
{
	"WarPanel"
	{
		"ControlName"	"CWarLandingPanel"
		"fieldName"		"WarPanel"
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

		if_war_active
		{
			"scene_anim_name"	"PyroVsHeavyWar_ShowChooseScene"
		}

		if_war_over
		{
			// Sorry Team Heavy.  It looks inevitable at this point
			"scene_anim_name"	"PyroVsHeavyWar_PyroWinnerScene"
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
			"bgcolor_override"	"0 0 0 240"
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
			"ypos"			"cs-0.5"
			"zpos"			"1"
			"wide"			"600"
			"tall"			"350"
			"autoResize"	"3"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"	"0"
			"border"		"MainMenuBGBorder"

			"CloseButton"
			{
				"ControlName"	"CExImageButton"
				"fieldName"		"CloseButton"
				"xpos"			"rs1.4"
				"ypos"			"s0.4"
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
					"image"			"close_button"
					"scaleImage"	"1"
				}				
			}

			"NoGContainer"
			{
				"ControlName"	"EditablePanel"
				"fieldName"		"NoGContainer"
				"xpos"			"cs-0.5"
				"ypos"			"100"
				"wide"			"270"
				"tall"			"60"
				"zpos"			"100"
				"proportionaltoparent"	"1"
				"border"		"MainMenuBGBorder"

				"NoGCExplanation"
				{
					"ControlName"	"Label"
					"fieldName"		"NoGCExplanation"
					"xpos"			"55"
					"ypos"			"cs-0.5"
					"zpos"			"0"
					"wide"			"200"
					"tall"			"p0.8"
					"visible"		"1"
					"enabled"		"1"
					"font"			"HudFontSmallest"
					"fgcolor_override"	"TanLight"
					"textAlignment"	"center"
					"labelText"		"#TF_War_NoGC"
					"proportionaltoparent"	"1"
					"wrap"	"1"
				}

				"AlertImage"
				{
					"ControlName"	"ScalableImagePanel"
					"fieldName"		"AlertImage"
					"xpos"			"7"
					"ypos"			"cs-0.5"
					"zpos"			"0"
					"wide"			"o1"
					"tall"			"40"
					"visible"		"1"
					"enabled"		"1"
					"image"			"glyph_alert"
		
					"proportionaltoparent"	"1"
				}
			}

			"SceneContainer"
			{
				"ControlName"	"EditablePanel"
				"fieldName"		"SceneContainer"
				"xpos"			"cs-0.5"
				"ypos"			"10"
				"zpos"			"10"
				"wide"			"f20"
				"tall"			"200"
				"proportionaltoparent"	"1"

				"bgcolor_override"	"0 0 0 150"

				"ReadComicButton"
				{
					"ControlName"	"CExButton"
					"fieldName"		"ReadComicButton"
					"xpos"			"-5"
					"ypos"			"-5"
					"zpos"			"500"
					"wide"			"145"
					"tall"			"20"
					"autoResize"	"0"
					"pinCorner"		"3"
					"visible"		"1"
					"enabled"		"1"
					"tabPosition"	"0"
					"textinsetx"	"25"
					"use_proportional_insets" "1"
					"font"			"HudFontSmall"
					"textAlignment"	"south-east"
					"dulltext"		"0"
					"brighttext"	"0"
					"default"		"1"
					"labelText"			"#MMenu_ViewUpdateComic" 
					"command"		"view_update_comic"
					"sound_depressed"	"UI/buttonclick.wav"
					"sound_released"	"UI/buttonclickrelease.wav"
					"actionsignallevel"	"3"
			
				
					"paintbackground"	"1"
			
					"defaultFgColor_override"	"TanLight"
					"armedFgColor_override"		"TanLight"
					"depressedFgColor_override" "TanLight"

					"proportionaltoparent"	"1"
				}

				"EndDateLabel"
				{
					"ControlName"	"CExLabel"
					"fieldName"		"EndDateLabel"
					"xpos"			"cs-0.5"
					"ypos"			"0"
					"zpos"			"500"
					"wide"			"300"
					"tall"			"20"
					"autoResize"	"0"
					"pinCorner"		"3"
					"visible"		"1"
					"enabled"		"1"
					"tabPosition"	"0"
					"textinsetx"	"25"
					"use_proportional_insets" "1"
					"font"			"HudFontSmall"
					"textAlignment"	"center"
					"dulltext"		"0"
					"brighttext"	"0"
					"default"		"1"
					"labelText"			"%end_date%" 
					
					if_war_active
					{
						"fgcolor_override"	"TanLight"
					}

					if_war_over
					{
						"fgcolor_override"	"TanDarker"
					}
				
					"paintbackground"	"2"
					"RoundedCorners"	"1"

					"proportionaltoparent"	"1"
				}

				if_war_active
				{
					"Frame"
					{
						"ControlName"	"Panel"
						"fieldName"		"Frame"
						"xpos"			"0"
						"ypos"			"0"
						"wide"			"f0"
						"tall"			"f0"
						"proportionaltoparent"	"1"
						"zpos"			"1000" // Be on top of everything

						"border"		"InnerShadowBorderThin"
						"mouseinputenabled"	"0"
					}
				}

				"Background"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"Background"
					"xpos"			"cs-0.5"
					"ypos"			"0"
					"zpos"			"0"
					"wide"			"p1"
					"tall"			"o1"
					"visible"		"1"
					"enabled"		"1"
					"scaleImage"	"1"

					if_war_active
					{
						"image"			"heavy_pyro_war/war_screen_background"
					}

					if_war_over
					{
						"image"			"heavy_pyro_war/winner_screen_background"
					}

					"proportionaltoparent" "1"
				}

			
				"Heavy"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"Heavy"
					"xpos"			"cs-0.5-p0.27"
					"ypos"			"0"
					"zpos"			"1"
					"wide"			"p0.4"
					"tall"			"o2"
					"if_war_over"
					{
						"visible"		"0"
					}
					"enabled"		"1"
					"scaleImage"	"1"
					"image"			"heavy_pyro_war/war_screen_heavy"
					"proportionaltoparent" "1"
				}

				
				"Pyro"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"Pyro"
					"xpos"			"cs-0.5+p0.27"
					"ypos"			"0"
					"zpos"			"1"
					"wide"			"p0.4"
					"tall"			"o2"
					"if_war_over"
					{
						"visible"		"0"
					}
					"enabled"		"1"
					"scaleImage"	"1"
					"image"			"heavy_pyro_war/war_screen_pyro"
					"proportionaltoparent" "1"
				}

				"if_war_over"
				{
					"PyroWinMidground"
					{
						"ControlName"	"ImagePanel"
						"fieldName"		"PyroWinMidground"
						"xpos"			"cs-0.5"
						"ypos"			"cs-0.5"
						"zpos"			"1"
						"wide"			"p0.80"
						"tall"			"o1"
						"visible"		"1"
						"enabled"		"1"
						"scaleImage"	"1"
						"image"			"heavy_pyro_war/winner_screen_pyro"
						"proportionaltoparent" "1"
					}
				}	

				"Foreground"
				{
					"ControlName"	"EditablePanel"
					"fieldName"		"Foreground"
					"xpos"			"cs-0.5"
					"ypos"			"0"
					"zpos"			"2"
					"wide"			"p1"
					"tall"			"o1"
					"visible"		"1"
					"enabled"		"1"
					"proportionaltoparent" "1"

					if_war_active
					{
						"BGPanel"
						{
							"ControlName"	"EditablePanel"
							"fieldName"		"BGPanel"
							"xpos"			"cs-0.5"
							"ypos"			"p0.25"
							"zpos"			"2"
							"wide"			"p1"
							"tall"			"o1"
							"bgcolor_override"	"0 0 0 255"
							"proportionaltoparent" "1"
						}

						"Silhouettes"
						{
							"ControlName"	"ImagePanel"
							"fieldName"		"Silhouettes"
							"xpos"			"0"
							"ypos"			"0"
							"zpos"			"2"
							"wide"			"p1"
							"tall"			"o0.25"
							"visible"		"1"
							"enabled"		"1"
							"scaleImage"	"1"
							"image"			"heavy_pyro_war/war_screen_foreground"
							"proportionaltoparent" "1"
						}
					} // if_war_active

					if_war_over
					{
						"Silhouettes"
						{
							"ControlName"	"ImagePanel"
							"fieldName"		"Silhouettes"
							"xpos"			"0"
							"ypos"			"0"
							"zpos"			"2"
							"wide"			"p1"
							"tall"			"o0.25"
							"visible"		"1"
							"enabled"		"1"
							"scaleImage"	"1"
							"image"			"heavy_pyro_war/winner_screen_foreground"
							"proportionaltoparent" "1"
						}
					}

				} // Foreground
			}

			"AffiliatedContainer"
			{
				"ControlName"	"EditablePanel"
				"fieldName"		"AffiliatedContainer"
				"xpos"			"0"
				"ypos"			"0"
				"wide"			"f0"
				"tall"			"f0"
				"proportionaltoparent"	"1"
				"mouseinputenabled"	"0"

				"StandingsTitle"
				{
					"ControlName"	"Label"
					"fieldName"		"StandingsTitle"
					"xpos"			"cs-0.5"
					"ypos"			"213"
					"zpos"			"0"
					"wide"			"270"
					"tall"			"200"
					"visible"		"1"
					"enabled"		"1"
					"font"			"HudFontMediumBold"
					"fgcolor_override"	"TanLight"
					"textAlignment"	"north"
					"labelText"		"#TF_War_StandingsTitle"
					"proportionaltoparent"	"1"
				}

				"ProgressContainer"
				{
					"ControlName"	"EditablePanel"
					"fieldName"		"ProgressContainer"
					"xpos"	"cs-0.5"
					"ypos"	"rs1-30"
					"wide"	"p0.8"
					"tall"	"80"
					"proportionaltoparent"	"1"

					"bgcolor_override"	"0 0 0 100"

					"WarStandingsProgressBar"
					{
						"ControlName"	"CWarStandingPanel"
						"fieldName"		"WarStandingsProgressBar"
						"xpos"	"cs-0.5"
						"ypos"	"cs-0.5+5"
						"wide"	"f0"
						"tall"	"p0.8"
						"progress"	"0.5"
						"proportionaltoparent"	"1"	
						"war_name"	"Pyro vs Heavy"
					}

					"Frame"
					{
						"ControlName"	"Panel"
						"fieldName"		"Frame"
						"xpos"			"0"
						"ypos"			"0"
						"wide"			"f0"
						"tall"			"f0"
						"proportionaltoparent"	"1"
						"zpos"			"1000" // Be on top of everything

						"border"		"InnerShadowBorderThin"
					}
				}

				"Explanation"
				{
					"ControlName"	"Label"
					"fieldName"		"Explanation"
					"xpos"			"cs-0.5"
					"ypos"			"326"
					"zpos"			"0"
					"wide"			"500"
					"tall"			"200"
					"visible"		"1"
					"enabled"		"1"
					"font"			"HudFontSmall"
					"fgcolor_override"	"TanLight"
					"textAlignment"	"north"
					"labelText"		"#TF_War_HowToPlay"
					"proportionaltoparent"	"1"
				}
			}

			"WarOverContainer"
			{
				"ControlName"	"EditablePanel"
				"fieldName"		"WarOverContainer"
				"xpos"			"0"
				"ypos"			"0"
				"wide"			"f0"
				"tall"			"f0"
				"proportionaltoparent"	"1"
				"mouseinputenabled"	"0"

				"StandingsTitle"
				{
					"ControlName"	"Label"
					"fieldName"		"StandingsTitle"
					"xpos"			"cs-0.5"
					"ypos"			"213"
					"zpos"			"0"
					"wide"			"270"
					"tall"			"200"
					"visible"		"1"
					"enabled"		"1"
					"font"			"HudFontMediumBold"
					"fgcolor_override"	"HUDRedTeamSolid"
					"textAlignment"	"north"
					"labelText"		"#TF_War_Winner_PyroTitle"
					"proportionaltoparent"	"1"
				}

				"ProgressContainer"
				{
					"ControlName"	"EditablePanel"
					"fieldName"		"ProgressContainer"
					"xpos"	"cs-0.5"
					"ypos"	"rs1-30"
					"wide"	"p0.8"
					"tall"	"80"
					"proportionaltoparent"	"1"

					"bgcolor_override"	"0 0 0 100"

					"WarStandingsProgressBar"
					{
						"ControlName"	"CWarStandingPanel"
						"fieldName"		"WarStandingsProgressBar"
						"xpos"	"cs-0.5"
						"ypos"	"cs-0.5+5"
						"wide"	"f0"
						"tall"	"p0.8"
						"progress"	"0.5"
						"proportionaltoparent"	"1"	
						"war_name"	"Pyro vs Heavy"
					}

					"Frame"
					{
						"ControlName"	"Panel"
						"fieldName"		"Frame"
						"xpos"			"0"
						"ypos"			"0"
						"wide"			"f0"
						"tall"			"f0"
						"proportionaltoparent"	"1"
						"zpos"			"1000" // Be on top of everything

						"border"		"InnerShadowBorderThin"
					}
				}

				"Explanation"
				{
					"ControlName"	"Label"
					"fieldName"		"Explanation"
					"xpos"			"cs-0.5"
					"ypos"			"326"
					"zpos"			"0"
					"wide"			"f0"
					"tall"			"200"
					"visible"		"1"
					"enabled"		"1"
					"font"			"HudFontSmall"
					"fgcolor_override"	"TanLight"
					"textAlignment"	"north"
					"labelText"		"#TF_War_Winner_PyroDesc"
					"proportionaltoparent"	"1"
				}
			}

			"CommunicatingWithGCPopup"
			{
				"ControlName"	"EditablePanel"
				"fieldName"		"CommunicatingWithGCPopup"
				"xpos"			"0"
				"ypos"			"0"
				"wide"			"f0"
				"tall"			"f0"
				"zpos"			"100"
				"visible"		"0"
				"proportionaltoparent"	"1"
				"bgcolor_override"	"0 0 0 200"

				"BGFrame"
				{
					"ControlName"	"EditablePanel"
					"fieldName"		"BGFrame"
					"xpos"			"cs-0.5"
					"ypos"			"cs-0.5"
					"wide"			"250"
					"tall"			"100"
					"zpos"			"100"
					"proportionaltoparent"	"1"
					"border"		"MainMenuBGBorder"

					"ConfirmSelectionContainer"
					{
						"ControlName"	"EditablePanel"
						"fieldName"		"ConfirmSelectionContainer"
						"xpos"			"0"
						"ypos"			"0"
						"wide"			"f0"
						"tall"			"f0"
						"proportionaltoparent"	"1"

						"ConfirmLabel"
						{
							"ControlName"	"Label"
							"fieldName"		"ConfirmLabel"
							"xpos"			"cs-0.5"
							"ypos"			"10"
							"zpos"			"0"
							"wide"			"p0.8"
							"tall"			"200"
							"visible"		"1"
							"enabled"		"1"
							"font"			"HudFontSmallest"
							"fgcolor_override"	"TanLight"
							"textAlignment"	"north"
							"labelText"		"%confirm_selection%"
							"proportionaltoparent"	"1"
							"wrap"	"1"	
						}

						"ConfirmButton"
						{
							"ControlName"	"CExImageButton"
							"fieldName"		"ConfirmButton"
							"xpos"			"p0.05"
							"ypos"			"rs1-15"
							"zpos"			"5"
							"wide"			"100"
							"tall"			"26"
							"autoResize"	"0"
							"pinCorner"		"3"
							"visible"		"1"
							"enabled"		"1"
							"tabPosition"	"0"
							"textinsetx"	"25"
							"use_proportional_insets" "1"
							"font"			"HudFontSmallBold"
							"textAlignment"	"center"
							"dulltext"		"0"
							"brighttext"	"0"
							"default"		"1"
							"labelText"			"#Voice_Menu_Yes" 
							"command"		"confirm_team"
							"sound_depressed"	"UI/buttonclick.wav"
							"sound_released"	"UI/buttonclickrelease.wav"
							"actionsignallevel"	"5"
			
							"border_default"	"MainMenuButtonDefault"
							"border_armed"		"MainMenuButtonArmed"
							"paintbackground"	"0"
			
							"defaultFgColor_override" "46 43 42 255"
							"armedFgColor_override" "235 226 202 255"
							"depressedFgColor_override" "46 43 42 255"
			
							"image_drawcolor"	"117 107 94 255"
							"image_armedcolor"	"235 226 202 255"
							"proportionaltoparent"	"1"
						}

						"DismissButton"
						{
							"ControlName"	"CExImageButton"
							"fieldName"		"DismissButton"
							"xpos"			"rs1-p0.05"
							"ypos"			"rs1-15"
							"zpos"			"5"
							"wide"			"100"
							"tall"			"26"
							"autoResize"	"0"
							"pinCorner"		"3"
							"visible"		"1"
							"enabled"		"1"
							"tabPosition"	"0"
							"textinsetx"	"25"
							"use_proportional_insets" "1"
							"font"			"HudFontSmallBold"
							"textAlignment"	"center"
							"dulltext"		"0"
							"brighttext"	"0"
							"default"		"1"
							"labelText"			"#Store_CANCEL" 
							"command"		"dismiss_joining_result"
							"sound_depressed"	"UI/buttonclick.wav"
							"sound_released"	"UI/buttonclickrelease.wav"
							"actionsignallevel"	"5"
			
							"border_default"	"MainMenuButtonDefault"
							"border_armed"		"MainMenuButtonArmed"
							"paintbackground"	"0"
			
							"defaultFgColor_override" "46 43 42 255"
							"armedFgColor_override" "235 226 202 255"
							"depressedFgColor_override" "46 43 42 255"
			
							"image_drawcolor"	"117 107 94 255"
							"image_armedcolor"	"235 226 202 255"
							"proportionaltoparent"	"1"
						}
					}

					"JoiningContainer"
					{
						"ControlName"	"EditablePanel"
						"fieldName"		"JoiningContainer"
						"xpos"			"0"
						"ypos"			"0"
						"wide"			"f0"
						"tall"			"f0"
						"proportionaltoparent"	"1"

						"JoiningTeamLabel"
						{
							"ControlName"	"Label"
							"fieldName"		"JoiningTeamPyroLabel"
							"xpos"			"cs-0.5"
							"ypos"			"10"
							"zpos"			"0"
							"wide"			"p0.8"
							"tall"			"50"
							"visible"		"1"
							"enabled"		"1"
							"font"			"HudFontSmallest"
							"fgcolor_override"	"TanLight"
							"textAlignment"	"center"
							"labelText"		"%joining_team%"
							"proportionaltoparent"	"1"
						}

						"SpinnerImage"
						{
							"ControlName"	"ScalableImagePanel"
							"fieldName"		"SpinnerImage"
							"xpos"			"cs-0.5"
							"ypos"			"rs1+10"
							"zpos"			"0"
							"wide"			"o1"
							"tall"			"80"
							"visible"		"1"
							"enabled"		"1"
							"image"			"animated/tf2_logo_hourglass"
		
							"proportionaltoparent"	"1"
						}
					}

					"TeamJoinedContainer"
					{
						"ControlName"	"EditablePanel"
						"fieldName"		"TeamJoinedContainer"
						"xpos"			"0"
						"ypos"			"0"
						"wide"			"f0"
						"tall"			"f0"
						"proportionaltoparent"	"1"

						"TeamJoinedLabel"
						{
							"ControlName"	"Label"
							"fieldName"		"JoiningTeamPyroLabel"
							"xpos"			"cs-0.5"
							"ypos"			"10"
							"zpos"			"0"
							"wide"			"p0.8"
							"tall"			"50"
							"visible"		"1"
							"enabled"		"1"
							"font"			"HudFontSmallestBold"
							"fgcolor_override"	"TanLight"
							"textAlignment"	"center"
							"labelText"		"%team_joined%"
							"proportionaltoparent"	"1"
						}

						"DismissButton"
						{
							"ControlName"	"CExImageButton"
							"fieldName"		"DismissButton"
							"xpos"			"cs-0.5"
							"ypos"			"rs1-15"
							"zpos"			"5"
							"wide"			"100"
							"tall"			"26"
							"autoResize"	"0"
							"pinCorner"		"3"
							"visible"		"1"
							"enabled"		"1"
							"tabPosition"	"0"
							"textinsetx"	"25"
							"use_proportional_insets" "1"
							"font"			"HudFontSmallBold"
							"textAlignment"	"center"
							"dulltext"		"0"
							"brighttext"	"0"
							"default"		"1"
							"labelText"			"#TF_OK" 
							"command"		"dismiss_joining_result"
							"sound_depressed"	"UI/buttonclick.wav"
							"sound_released"	"UI/buttonclickrelease.wav"
							"actionsignallevel"	"5"
			
							"border_default"	"MainMenuButtonDefault"
							"border_armed"		"MainMenuButtonArmed"
							"paintbackground"	"0"
			
							"defaultFgColor_override" "46 43 42 255"
							"armedFgColor_override" "235 226 202 255"
							"depressedFgColor_override" "46 43 42 255"
			
							"image_drawcolor"	"117 107 94 255"
							"image_armedcolor"	"235 226 202 255"
							"proportionaltoparent"	"1"
						}
					}

					"FailedToJoinContainer"
					{
						"ControlName"	"EditablePanel"
						"fieldName"		"FailedToJoinContainer"
						"xpos"			"0"
						"ypos"			"0"
						"wide"			"f0"
						"tall"			"f0"
						"proportionaltoparent"	"1"

						"PyroImage"
						{
							"ControlName"	"ImagePanel"
							"fieldName"		"PyroImage"
							"xpos"			"cs-0.5+p-0.23"
							"ypos"			"20"
							"zpos"			"-1"
							"wide"			"30"
							"tall"			"o1"
							"visible"		"1"
							"enabled"		"1"
							"scaleImage"	"1"
							"image"			"glyph_alert"
							"proportionaltoparent" "1"

							"drawcolor"	"TanLight"
						}

						"FailedExplanation"
						{
							"ControlName"	"Label"
							"fieldName"		"FailedExplanation"
							"xpos"			"cs-0.5+40"
							"ypos"			"10"
							"zpos"			"0"
							"wide"			"p0.6"
							"tall"			"50"
							"visible"		"1"
							"enabled"		"1"
							"font"			"HudFontSmallest"
							"fgcolor_override"	"TanLight"
							"textAlignment"	"west"
							"labelText"		"#TF_War_FailedToJoin"
							"proportionaltoparent"	"1"
						}

						"DismissButton"
						{
							"ControlName"	"CExImageButton"
							"fieldName"		"DismissButton"
							"xpos"			"cs-0.5"
							"ypos"			"rs1-15"
							"zpos"			"5"
							"wide"			"100"
							"tall"			"26"
							"autoResize"	"0"
							"pinCorner"		"3"
							"visible"		"1"
							"enabled"		"1"
							"tabPosition"	"0"
							"textinsetx"	"25"
							"use_proportional_insets" "1"
							"font"			"HudFontSmallBold"
							"textAlignment"	"center"
							"dulltext"		"0"
							"brighttext"	"0"
							"default"		"1"
							"labelText"			"#TF_OK" 
							"command"		"dismiss_joining_result"
							"sound_depressed"	"UI/buttonclick.wav"
							"sound_released"	"UI/buttonclickrelease.wav"
							"actionsignallevel"	"5"
			
							"border_default"	"MainMenuButtonDefault"
							"border_armed"		"MainMenuButtonArmed"
							"paintbackground"	"0"
			
							"defaultFgColor_override" "46 43 42 255"
							"armedFgColor_override" "235 226 202 255"
							"depressedFgColor_override" "46 43 42 255"
			
							"image_drawcolor"	"117 107 94 255"
							"image_armedcolor"	"235 226 202 255"
							"proportionaltoparent"	"1"
						}
					}
				}
			}

			"UnaffiliatedContainer"
			{
				"ControlName"	"EditablePanel"
				"fieldName"		"UnaffiliatedContainer"
				"xpos"			"0"
				"ypos"			"0"
				"wide"			"f0"
				"tall"			"f0"
				"proportionaltoparent"	"1"

				"HeavyImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"HeavyImage"
					"xpos"			"cs-0.5-210"
					"ypos"			"rs1-25"
					"zpos"			"-1"
					"wide"			"100"
					"tall"			"o1"
					"visible"		"1"
					"enabled"		"1"
					"scaleImage"	"1"
					"image"		"heavy_pyro_war/heavy_logo"
					"proportionaltoparent" "1"

					"drawcolor"	"HUDBlueTeamSolid"
				}

				"PyroImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"PyroImage"
					"xpos"			"cs-0.5+210"
					"ypos"			"rs1-25"
					"zpos"			"-1"
					"wide"			"100"
					"tall"			"o1"
					"visible"		"1"
					"enabled"		"1"
					"scaleImage"	"1"
					"image"		"heavy_pyro_war/pyro_logo"
					"proportionaltoparent" "1"

					"drawcolor"	"HUDRedTeamSolid"
				}


				"HeavyContainer"
				{
					"ControlName"	"EditablePanel"
					"fieldName"		"HeavyContainer"
					"xpos"			"cs-1-140"
					"ypos"			"rs1-10"
					"zpos"			"5"
					"wide"			"150"
					"tall"			"o1"
					"proportionaltoparent"	"1"

					"JoinHeavyButton"
					{
						"ControlName"	"CExImageButton"
						"fieldName"		"JoinHeavyButton"
						"xpos"			"0"
						"ypos"			"rs1"
						"zpos"			"5"
						"wide"			"150"
						"tall"			"26"
						"autoResize"	"0"
						"pinCorner"		"3"
						"visible"		"1"
						"enabled"		"1"
						"tabPosition"	"0"
						"textinsetx"	"25"
						"use_proportional_insets" "1"
						"font"			"HudFontSmallBold"
						"textAlignment"	"center"
						"dulltext"		"0"
						"brighttext"	"0"
						"default"		"1"
						"labelText"			"#JoinHeavyButton" 
						"command"		"join_war0"
						"sound_depressed"	"UI/buttonclick.wav"
						"sound_released"	"UI/buttonclickrelease.wav"
						"actionsignallevel"	"4"
			
						"border_default"	"MainMenuButtonDefault"
						"border_armed"		"MainMenuButtonArmed"
						"paintbackground"	"0"
			
						"defaultFgColor_override" "46 43 42 255"
						"armedFgColor_override" "235 226 202 255"
						"depressedFgColor_override" "46 43 42 255"
			
						"image_drawcolor"	"117 107 94 255"
						"image_armedcolor"	"235 226 202 255"
						"proportionaltoparent"	"1"
					}
				}

				"PyroContainer"
				{
					"ControlName"	"EditablePanel"
					"fieldName"		"PyroContainer"
					"xpos"			"c140"
					"ypos"			"rs1-10"
					"zpos"			"5"
					"wide"			"150"
					"tall"			"o1"
					"proportionaltoparent"	"1"

					"JoinPyroButton"
					{
						"ControlName"	"CExImageButton"
						"fieldName"		"JoinPyroButton"
						"xpos"			"0"
						"ypos"			"rs1"
						"zpos"			"5"
						"wide"			"150"
						"tall"			"26"
						"autoResize"	"0"
						"pinCorner"		"3"
						"visible"		"1"
						"enabled"		"1"
						"tabPosition"	"0"
						"textinsetx"	"25"
						"use_proportional_insets" "1"
						"font"			"HudFontSmallBold"
						"textAlignment"	"center"
						"dulltext"		"0"
						"brighttext"	"0"
						"default"		"1"
						"labelText"			"#JoinPyroButton" 
						"command"		"join_war1"
						"sound_depressed"	"UI/buttonclick.wav"
						"sound_released"	"UI/buttonclickrelease.wav"
						"actionsignallevel"	"4"
			
						"border_default"	"MainMenuButtonDefault"
						"border_armed"		"MainMenuButtonArmed"
						"paintbackground"	"0"
			
						"defaultFgColor_override" "46 43 42 255"
						"armedFgColor_override" "235 226 202 255"
						"depressedFgColor_override" "46 43 42 255"
			
						"image_drawcolor"	"117 107 94 255"
						"image_armedcolor"	"235 226 202 255"
						"proportionaltoparent"	"1"
					}
				}

				"ExplanationLabel"
				{
					"ControlName"	"Label"
					"fieldName"		"ExplanationLabel"
					"xpos"			"cs-0.5"
					"ypos"			"220"
					"zpos"			"0"
					"wide"			"270"
					"tall"			"200"
					"visible"		"1"
					"enabled"		"1"
					"font"			"HudFontSmallest"
					"fgcolor_override"	"TanLight"
					"textAlignment"	"north-west"
					"labelText"		"#TF_War_HeavyVsPyro_Explanation"
					"proportionaltoparent"	"1"
					"wrap"			"1"
				}
			}

		} // Border
	}
}
