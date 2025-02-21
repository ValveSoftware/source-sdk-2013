"Resource/UI/HudObjectiveFlagPanel.res"
{	
	"ObjectiveStatusRobotDestruction"
	{
		"ControlName"		"EditablePanel"
		"fieldName"			"ObjectiveStatusRobotDestruction"
		"xpos"				"0"
		"ypos"				"0"
		"zpos"				"1"
		"wide"				"f0"
		"tall"				"480"
		"visible"			"1"
		"enabled"			"1"

		"left_steal_edge_offset"	"97"
		"right_steal_edge_offset"	"97"
		"robot_x_offset"		"78"
		"robot_y_offset"		"47"
		"robot_x_step"			"23"
		"robot_y_step"			"0"
		

		"color_blue"			"84 111 127 255"
		"color_red"				"171 59 59 255"

		"if_hybrid"
		{
			"zpos"			"-1"
		}

		"robot_kv"
		{
			"ControlName"	"CTFHudRobotDestruction_RobotIndicator"
			"xpos"			"0"
			"ypos"			"0"
			"wide"			"20"
			"tall"			"20"
			"visible"		"1"
			"bgcolor_override"		"0 0 0 0"
			"PaintBackgroundType"	"0"
			"paintborder"	"0"
			"AutoResize" "0"
			"skip_autoresize" "1"
		}
	}	
	
	"PlayingTo"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"PlayingTo"
		"xpos"			"c-70"	[$WIN32]
		"xpos"			"c-60"	[$X360]
		"ypos"			"r28"	[$WIN32]
		"ypos"			"r48"	[$X360]
		"zpos"			"4"
		"wide"			"140"	[$WIN32]
		"wide"			"120"	[$X360]
		"tall"			"30"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		"#TF_PlayingTo"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"font"			"HudFontSmall"
		"fgcolor"		"TanLight"
		
		"if_hybrid"
		{
			"visible"	"0"
		}
		
		"if_mvm"
		{
			"visible"	"0"
		}
		
		"if_specialdelivery"
		{
			"visible"	"0"
		}
	}	
			
	"PlayingToBG"
	{
		"ControlName"	"CTFImagePanel"
		"fieldName"		"PlayingToBG"
		"xpos"			"c-75"	[$WIN32]
		"xpos"			"c-70"	[$X360]
		"ypos"			"r31"	[$WIN32]
		"ypos"			"r51"	[$X360]
		"zpos"			"3"
		"wide"			"150"	[$WIN32]
		"wide"			"140"	[$X360]
		"tall"			"38"	[$WIN32]
		"tall"			"35"	[$X360]
		"visible"		"1"
		"enabled"		"1"
		"image"			"../hud/objectives_flagpanel_bg_playingto"
		"image_hidef"	"../hud/objectives_flagpanel_bg_playingto_hidef"
		"scaleImage"	"1"	
		
		"if_hybrid"
		{
			"visible"	"0"
		}
		
		"if_mvm"
		{
			"visible"	"0"
		}
		
		"if_specialdelivery"
		{
			"visible"	"0"
		}
	}

	"CarriedContainer"
	{
		"ControlName"		"EditablePanel"
		"fieldName"			"CarriedContainer"
		"xpos"				"c-75"
		"ypos"				"r137"
		"zpos"				"1"
		"wide"				"150"
		"tall"				"80"
		"visible"			"0"
		"enabled"			"1"
		"bgcolor_override"		"0 0 0 0"
	
		"CarriedImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"CarriedImage"
			"xpos"			"0"
			"ypos"			"0"
			"zpos"			"10"
			"wide"			"f0"
			"tall"			"f0"
			"visible"		"1"
			"enabled"		"1"
			"image"			"../hud/obj_rd_powersupply_outline"
			"scaleImage"	"1"
			"proportionalToParent"	"1"
		}	

		"CarriedProgressBar"
		{
			"fieldName"				"CarriedProgressBar"
			"ControlName"			"ImagePanel"
			"xpos"					"0"
			"ypos"					"0"
			"zpos"					"10"
			"wide"					"f0"
			"tall"					"f0"
			"scaleimage"			"0"
			"visible"				"1"
			"proportionalToParent"	"1"
			"positionImage"			"0"
			"drawcolor"				"84 111 127 255"
			"Image"					"../HUD/obj_rd_powersupply_meter"

			"left_offset"			"30"
			"right_offset"			"30"
			"standard_color"		"84  111 127 255"
			"bright_color"			"110 159 189 255"
			"left_to_right"			"1"
		}

		"FlagValue"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"FlagValue"
			"xpos"			"c-50"
			"ypos"			"23"
			"zpos"			"11"
			"wide"			"100"
			"tall"			"35"
			"tall_hidef"	"100"
			"tall_lodef"	"75"
			"visible"		"1"
			"enabled"		"1"
			"textAlignment"	"center"	
			"labelText"		"%flagvalue%"
			"font"			"HudFontMediumBold"
			"font_hidef"	"HudFontGiant"
			"font_lodef"	"HudFontGiant"
			"fgcolor"		"TanLight"	
			"proportionalToParent"	"1"	
		}	

		"FlagValueShadow"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"FlagValueShadow"
			"xpos"			"c-49"
			"ypos"			"24"
			"zpos"			"11"
			"wide"			"100"
			"tall"			"35"
			"tall_hidef"	"100"
			"tall_lodef"	"75"
			"visible"		"1"
			"enabled"		"1"
			"textAlignment"	"center"	
			"labelText"		"%flagvalue%"
			"font"			"HudFontMediumBold"
			"font_hidef"	"HudFontGiant"
			"font_lodef"	"HudFontGiant"
			"fgcolor"		"Black"	
			"proportionalToParent"	"1"	
		}
	}

	"ScoreContainer"
	{
		"fieldName"				"ScoreContainer"
		"ControlName"			"EditablePanel"
		"xpos"					"c-200"
		"ypos"					"r131"
		"zpos"					"0"
		"wide"					"400"
		"tall"					"120"
		"scaleimage"			"0"
		"visible"				"1"
		"enabled"				"1"

		"ProgressBarContainer"
		{
			"fieldName"				"ProgressBarContainer"
			"ControlName"			"EditablePanel"
			"xpos"					"c-150"
			"ypos"					"r61"
			"zpos"					"1"
			"wide"					"300"
			"tall"					"60"
			"visible"				"1"
			"enabled"				"1"
			"proportionalToParent"	"1"
		
			"ScoreOutline"
			{
				"fieldName"				"ScoreOutline"
				"ControlName"			"ImagePanel"
				"xpos"					"0"
				"ypos"					"0"
				"zpos"					"10"
				"wide"					"f0"
				"tall"					"f0"
				"scaleimage"			"1"
				"visible"				"1"
				"enabled"				"1"
				"Image"					"../hud/objectives_corepanel_bg"
				"proportionalToParent"	"1"
			}

			"BlueVictoryContainer"
			{
				"fieldName"				"BlueVictoryContainer"
				"ControlName"			"EditablePanel"
				"xpos"					"25"
				"ypos"					"22"
				"zpos"					"5"
				"wide"					"f0"
				"tall"					"f0"
				"visible"				"0"
				"enabled"				"1"
				"proportionalToParent"	"1"

				"VictoryLabel"
				{
					"ControlName"	"CExLabel"
					"fieldName"		"VictoryLabel"
					"xpos"			"0"
					"ypos"			"0"
					"zpos"			"8"
					"wide"			"95"
					"tall"			"35"
					"visible"		"1"
					"enabled"		"1"
					"textAlignment"	"west"	
					"labelText"		"#TF_RD_BlueFinale"
					"font"			"HudFontSmallBold"
					"fgcolor"		"TanLight"		
					"proportionalToParent"	"1"
				}

				"VictoryLabelShadow"
				{
					"ControlName"	"CExLabel"
					"fieldName"		"VictoryLabelShadow"
					"xpos"			"1"
					"ypos"			"1"
					"zpos"			"7"
					"wide"			"95"
					"tall"			"35"
					"visible"		"1"
					"enabled"		"1"
					"textAlignment"	"west"	
					"labelText"		"#TF_RD_BlueFinale"
					"font"			"HudFontSmallBold"
					"fgcolor"		"Black"		
					"proportionalToParent"	"1"
				}

				"VictoryLabelTime"
				{
					"ControlName"	"CExLabel"
					"fieldName"		"VictoryLabelTime"
					"xpos"			"0"
					"ypos"			"0"
					"zpos"			"8"
					"wide"			"55"
					"tall"			"35"
					"visible"		"1"
					"enabled"		"1"
					"textAlignment"	"west"	
					"labelText"		"%victorytime%"
					"font"			"HudFontSmallBold"
					"fgcolor"		"TanLight"		
					"proportionalToParent"	"1"

					"pin_to_sibling"               "VictoryLabel"
					"pin_corner_to_sibling"        "7"          
					"pin_to_sibling_corner"        "5"  
				}

				"VictoryLabelTimeShadow"
				{
					"ControlName"	"CExLabel"
					"fieldName"		"VictoryLabelTimeShadow"
					"xpos"			"1"
					"ypos"			"1"
					"zpos"			"7"
					"wide"			"55"
					"tall"			"35"
					"visible"		"1"
					"enabled"		"1"
					"textAlignment"	"west"	
					"labelText"		"%victorytime%"
					"font"			"HudFontSmallBold"
					"fgcolor"		"Black"		
					"proportionalToParent"	"1"

					"pin_to_sibling"               "VictoryLabel"
					"pin_corner_to_sibling"        "7"          
					"pin_to_sibling_corner"        "5"  
				}
			}

			"BlueProgressBarFill"
			{
				"fieldName"				"BlueProgressBarFill"
				"ControlName"			"ImagePanel"
				"xpos"					"0"
				"ypos"					"0"
				"zpos"					"2"
				"wide"					"150"
				"tall"					"f0"
				"scaleimage"			"0"
				"visible"				"1"
				"proportionalToParent"	"1"
				"positionImage"			"0"
				"drawcolor"				"84 111 127 255"
				"Image"					"../hud/objectives_corepanel_meter_solid"

				"left_offset"			"10"
				"right_offset"			"0"
				"standard_color"		"84  111 127 255"
				"bright_color"			"110 159 189 255"
				"left_to_right"			"0"
				"blink_threshold"		"1.0"
				"blink_rate"			"10"
			}

			"BlueProgressBarEscrow"
			{
				"fieldName"				"BlueProgressBarEscrow"
				"ControlName"			"ImagePanel"
				"xpos"					"0"
				"ypos"					"0"
				"zpos"					"1"
				"wide"					"150"
				"tall"					"f0"
				"scaleimage"			"0"
				"visible"				"1"
				"proportionalToParent"	"1"
				"positionImage"			"0"
				"drawcolor"				"141 160 171 255"
				"Image"					"../hud/objectives_corepanel_meter"

				"left_offset"			"10"
				"right_offset"			"0"
				"standard_color"		"127 143 152 255"
				"bright_color"			"177 193 202 255"
				"left_to_right"			"0"
				"blink_threshold"		"1"
				"blink_rate"			"20"
			}

			"RedVictoryContainer"
			{
				"fieldName"				"RedVictoryContainer"
				"ControlName"			"EditablePanel"
				"xpos"					"25"
				"ypos"					"22"
				"zpos"					"5"
				"wide"					"f0"
				"tall"					"f0"
				"visible"				"0"
				"enabled"				"1"
				"proportionalToParent"	"1"

				"VictoryLabel"
				{
					"ControlName"	"CExLabel"
					"fieldName"		"VictoryLabel"
					"xpos"			"140"
					"ypos"			"0"
					"zpos"			"8"
					"wide"			"90"
					"tall"			"35"
					"visible"		"1"
					"enabled"		"1"
					"textAlignment"	"west"	
					"labelText"		"#TF_RD_RedFinale"
					"font"			"HudFontSmallBold"
					"fgcolor"		"TanLight"		
					"proportionalToParent"	"1"
				}

				"VictoryLabelShadow"
				{
					"ControlName"	"CExLabel"
					"fieldName"		"VictoryLabelShadow"
					"xpos"			"141"
					"ypos"			"1"
					"zpos"			"7"
					"wide"			"90"
					"tall"			"35"
					"visible"		"1"
					"enabled"		"1"
					"textAlignment"	"west"	
					"labelText"		"#TF_RD_RedFinale"
					"font"			"HudFontSmallBold"
					"fgcolor"		"Black"		
					"proportionalToParent"	"1"
				}

				"VictoryLabelTime"
				{
					"ControlName"	"CExLabel"
					"fieldName"		"VictoryLabelTime"
					"xpos"			"0"
					"ypos"			"0"
					"zpos"			"8"
					"wide"			"55"
					"tall"			"35"
					"visible"		"1"
					"enabled"		"1"
					"textAlignment"	"west"	
					"labelText"		"%victorytime%"
					"font"			"HudFontSmallBold"
					"fgcolor"		"TanLight"		
					"proportionalToParent"	"1"

					"pin_to_sibling"               "VictoryLabel"
					"pin_corner_to_sibling"        "7"          
					"pin_to_sibling_corner"        "5"  
				}

				"VictoryLabelTimeShadow"
				{
					"ControlName"	"CExLabel"
					"fieldName"		"VictoryLabelTimeShadow"
					"xpos"			"1"
					"ypos"			"1"
					"zpos"			"7"
					"wide"			"55"
					"tall"			"35"
					"visible"		"1"
					"enabled"		"1"
					"textAlignment"	"west"	
					"labelText"		"%victorytime%"
					"font"			"HudFontSmallBold"
					"fgcolor"		"Black"		
					"proportionalToParent"	"1"

					"pin_to_sibling"               "VictoryLabel"
					"pin_corner_to_sibling"        "7"          
					"pin_to_sibling_corner"        "5"  
				}
			}

			"RedProgressBarFill"
			{
				"fieldName"				"RedProgressBarFill"
				"ControlName"			"ImagePanel"
				"xpos"					"150"
				"ypos"					"0"
				"zpos"					"2"
				"wide"					"150"
				"tall"					"f0"
				"scaleimage"			"0"
				"positionImage"			"0"
				"visible"				"1"
				"proportionalToParent"	"1"
				"drawcolor"				"171 59 59 255"
				"Image"					"../hud/objectives_corepanel_meter_solid_right"

				"left_offset"			"0"
				"right_offset"			"9"
				"standard_color"		"171 59 59 255"
				"bright_color"			"222 65 65 255"
				"left_to_right"			"1"
				"blink_threshold"		"1.0"
				"blink_rate"			"10"
			}

			"RedProgressBarEscrow"
			{
				"fieldName"				"RedProgressBarEscrow"
				"ControlName"			"ImagePanel"
				"xpos"					"150"
				"ypos"					"0"
				"zpos"					"1"
				"wide"					"150"
				"tall"					"f0"
				"scaleimage"			"0"
				"positionImage"			"0"
				"visible"				"1"
				"proportionalToParent"	"1"
				"drawcolor"				"212 151 151 255"
				"Image"					"../hud/objectives_corepanel_meter_right"

				"left_offset"			"0"
				"right_offset"			"9"
				"standard_color"		"212 151 151 255"
				"bright_color"			"250 201 201 255"
				"left_to_right"			"1"
				"blink_threshold"		"1"
				"blink_rate"			"20"
			}
		}

		"BlueScoreValueContainer"
		{
			"ControlName"		"EditablePanel"
			"fieldName"			"BlueScoreValueContainer"
			"xpos"				"80"
			"ypos"				"r48"
			"zpos"				"10"
			"wide"				"60"
			"tall"				"60"
			"visible"			"1"
			"enabled"			"1"
			"bgcolor_override"		"0 0 0 0"
			"proportionalToParent"	"1"

			"Score"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"Score"
				"xpos"			"c-27"
				"ypos"			"c-20"
				"zpos"			"8"
				"wide"			"55"
				"tall"			"35"
				"visible"		"1"
				"enabled"		"1"
				"textAlignment"	"west"	
				"labelText"		"%score%"
				"font"			"HudFontMediumBold"
				"font_hidef"	"HudFontGiant"
				"font_lodef"	"HudFontGiant"
				"fgcolor"		"TanLight"		
				"proportionalToParent"	"1"
			}	
		
			"ScoreShadow"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"ScoreShadow"
				"xpos"			"c-26"
				"ypos"			"c-19"
				"zpos"			"7"
				"wide"			"55"
				"tall"			"35"
				"visible"		"1"
				"enabled"		"1"
				"textAlignment"	"west"	
				"labelText"		"%score%"
				"font"			"HudFontMediumBold"
				"font_hidef"	"HudFontGiant"
				"font_lodef"	"HudFontGiant"
				"fgcolor"		"Black"		
				"proportionalToParent"	"1"
			}
		}

		"RedScoreValueContainer"
		{
			"ControlName"		"EditablePanel"
			"fieldName"			"RedScoreValueContainer"
			"xpos"				"r140"
			"ypos"				"r48"
			"zpos"				"10"
			"wide"				"60"
			"tall"				"60"
			"visible"			"1"
			"enabled"			"1"
			"bgcolor_override"		"0 0 0 0"
			"proportionalToParent"	"1"

			"Score"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"Score"
				"xpos"			"c-27"
				"ypos"			"c-20"
				"zpos"			"8"
				"wide"			"55"
				"tall"			"35"
				"visible"		"1"
				"enabled"		"1"
				"textAlignment"	"east"	
				"labelText"		"%score%"
				"font"			"HudFontMediumBold"
				"font_hidef"	"HudFontGiant"
				"font_lodef"	"HudFontGiant"
				"fgcolor"		"TanLight"	
				"proportionalToParent"	"1"	
			}	
		
			"ScoreShadow"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"ScoreShadow"
				"xpos"			"c-26"
				"ypos"			"c-19"
				"zpos"			"7"
				"wide"			"55"
				"tall"			"35"
				"visible"		"1"
				"enabled"		"1"
				"textAlignment"	"east"	
				"labelText"		"%score%"
				"font"			"HudFontMediumBold"
				"font_hidef"	"HudFontGiant"
				"font_lodef"	"HudFontGiant"
				"fgcolor"		"Black"		
				"proportionalToParent"	"1"
			}
		}

		"BlueStolenContainer"
		{
			"ControlName"		"EditablePanel"
			"fieldName"			"BlueStolenContainer"
			"xpos"				"r45"
			"ypos"				"r77"
			"zpos"				"1"
			"wide"				"30"
			"tall"				"40"
			"visible"			"1"
			"enabled"			"1"
			"bgcolor_override"		"0 0 0 0"
			"proportionalToParent"	"1"

			"IntelImage"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"IntelImage"
				"xpos"			"5"
				"ypos"			"19"
				"zpos"			"10"
				"wide"			"21"
				"tall"			"21"
				"visible"		"0"
				"enabled"		"1"
				"image"			"../HUD/obj_thief_red"
				"scaleImage"	"1"
				"proportionalToParent"	"1"
			}	

			"DroppedIntelContainer"
			{
				"ControlName"		"EditablePanel"
				"fieldName"			"DroppedIntelContainer"
				"xpos"				"0"
				"ypos"				"20"
				"zpos"				"1"
				"wide"				"f0"
				"tall"				"f0"
				"visible"			"1"
				"enabled"			"1"
				"bgcolor_override"		"0 0 0 0"
				
				"proportionalToParent"	"1"

				"DroppedIntelImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"DroppedIntelImage"
					"xpos"			"0"
					"ypos"			"0"
					"zpos"			"0"
					"wide"			"f0"
					"tall"			"20"
					"visible"		"1"
					"enabled"		"1"
					"image"			"../HUD/obj_rd_thief_dropped_blue"
					"scaleImage"	"1"
					"proportionalToParent"	"1"
				}
			}

			"IntelValue"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"IntelValue"
				"xpos"			"0"
				"ypos"			"0"
				"zpos"			"12"
				"wide"			"f0"
				"tall"			"20"
				"visible"		"1"
				"enabled"		"1"
				"textAlignment"	"center"	
				"labelText"		"%intelvalue%"
				"font"			"HudFontSmallishBold"
				"fgcolor"		"TanLight"		
				"bgcolor_override"		"0 0 0 0"
				"proportionalToParent"	"1"
			}

			"IntelValueShadow"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"IntelValueShadow"
				"xpos"			"1"
				"ypos"			"1"
				"zpos"			"11"
				"wide"			"f0"
				"tall"			"20"
				"visible"		"1"
				"enabled"		"1"
				"textAlignment"	"center"	
				"labelText"		"%intelvalue%"
				"font"			"HudFontSmallishBold"	
				"fgcolor"		"Black"
				"proportionalToParent"	"1"
			}
		}

		"RedStolenContainer"
		{
			"ControlName"		"EditablePanel"
			"fieldName"			"RedStolenContainer"
			"xpos"				"r45"
			"ypos"				"r77"
			"zpos"				"1"
			"wide"				"30"
			"tall"				"40"
			"visible"			"1"
			"enabled"			"1"
			"bgcolor_override"		"0 0 0 0"
			"proportionalToParent"	"1"

			"IntelImage"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"IntelImage"
				"xpos"			"5"
				"ypos"			"19"
				"zpos"			"10"
				"wide"			"21"
				"tall"			"21"
				"visible"		"0"
				"enabled"		"1"
				"image"			"../HUD/obj_thief_blue"
				"scaleImage"	"1"
				"proportionalToParent"	"1"
			}	

			"DroppedIntelContainer"
			{
				"ControlName"		"EditablePanel"
				"fieldName"			"DroppedIntelContainer"
				"xpos"				"0"
				"ypos"				"20"
				"zpos"				"1"
				"wide"				"f0"
				"tall"				"f0"
				"visible"			"1"
				"enabled"			"1"
				"bgcolor_override"		"0 0 0 0"
				
				"proportionalToParent"	"1"

				"DroppedIntelImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"DroppedIntelImage"
					"xpos"			"0"
					"ypos"			"0"
					"zpos"			"0"
					"wide"			"f0"
					"tall"			"20"
					"visible"		"1"
					"enabled"		"1"
					"image"			"../HUD/obj_rd_thief_dropped_red"
					"scaleImage"	"1"
					"proportionalToParent"	"1"
				}
			}

			"IntelValue"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"IntelValue"
				"xpos"			"0"
				"ypos"			"0"
				"zpos"			"12"
				"wide"			"f0"
				"tall"			"20"
				"visible"		"1"
				"enabled"		"1"
				"textAlignment"	"center"	
				"labelText"		"%intelvalue%"
				"font"			"HudFontSmallishBold"
				"fgcolor"		"TanLight"		
				"bgcolor_override"		"0 0 0 0"
				"proportionalToParent"	"1"
			}

			"IntelValueShadow"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"IntelValueShadow"
				"xpos"			"1"
				"ypos"			"1"
				"zpos"			"11"
				"wide"			"f0"
				"tall"			"20"
				"visible"		"1"
				"enabled"		"1"
				"textAlignment"	"center"	
				"labelText"		"%intelvalue%"
				"font"			"HudFontSmallishBold"	
				"fgcolor"		"Black"	
				"proportionalToParent"	"1"
			}
		}
	}
}
