// enum PinCorner_e 
// {
// 	PIN_TOPLEFT = 0,
// 	PIN_TOPRIGHT,
// 	PIN_BOTTOMLEFT,
// 	PIN_BOTTOMRIGHT,

// 	// For sibling pinning
// 	PIN_CENTER_TOP,
// 	PIN_CENTER_RIGHT,
// 	PIN_CENTER_BOTTOM,
// 	PIN_CENTER_LEFT,
// };

"Resource/UI/HudPasstimeBallStatus.res"
{	
	"HudPasstimeBallStatus"
	{
		"ControlName"		"EditablePanel"
		"fieldName"			"HudPasstimeBallStatus"
		"xpos"				"0"
		"ypos"				"0"
		"zpos"				"100"
		"wide"				"f0"
		"tall"				"f0"
		"visible"			"1"
		"enabled"			"1"
	}

	"EventTitleAnchor"
	{	
		"ControlName"			"ImagePanel"
		"fieldName"				"EventTitleAnchor"
		"xpos"					"cs-0.5"
		"ypos"					"0" 
		"zpos"					"5"
		"wide"					"100"
		"tall"					"0"
		"autoResize"			"0"
		"pinCorner"				"0"
		"visible"				"0"
		"enabled"				"1"
		"tabPosition"			"0"	
		"fillcolor"				""
		"PaintBackgroundType"	"0"
	}
	
	"EventTitleLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"EventTitleLabel"
		"dropshadow"	"1"
		"font"			"MontserratBlack24"
		"xpos"			"0"
		"ypos"			"90"
		"zpos"			"3"
		"wide"			"f"
		"tall"			"15"
		"visible"		"1"
		"enabled"		"1"
		"textAlignment"	"center"
		"labelText"		"A TEAM STOLE THE BALL"
		"fgcolor_override"	"White"
		"border"				""
		"textinsetx"		"0"
		"auto_wide_tocontents"	"1"
		"pin_to_sibling"		"EventTitleAnchor"
		"pin_corner_to_sibling"	"4"
		"pin_to_sibling_corner"	"6"
	}

	"EventTitleLabelShadow"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"EventTitleLabelShadow"
		"dropshadow"	"1"
		"font"			"MontserratBlack24"
		"xpos"			"1"
		"ypos"			"91"
		"zpos"			"2"
		"wide"			"f"
		"tall"			"15"
		"visible"		"1"
		"enabled"		"1"
		"textAlignment"	"center"
		"labelText"		"A TEAM STOLE THE BALL"
		"fgcolor" 		"PFGrey"
		"border"				""
		"textinsetx"		"0"
		"auto_wide_tocontents"	"1"
		"pin_to_sibling"		"EventTitleAnchor"
		"pin_corner_to_sibling"	"4"
		"pin_to_sibling_corner"	"6"
	}

	"EventDetailLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"EventDetailLabel"
		"font"			"MontserratSemiBold16"
		"dropshadow"	"1"
		"xpos"			"4"
		"ypos"			"0"
		"zpos"			"3"
		"wide"			"f"
		"tall"			"20"
		"visible"		"1"
		"enabled"		"1"
		"textAlignment"	"center"
		"labelText"		"A TEAM STOLE THE BALL"
		"fgcolor_override"	"White"
		"border"				""
		"textinsetx"		"0"
		"auto_wide_tocontents"	"1"
		
		"pin_to_sibling"		"EventTitleLabel"
		"pin_corner_to_sibling"	"4"
		"pin_to_sibling_corner"	"6"
	}

	"EventDetailLabelShadow"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"EventDetailLabelShadow"
		"font"			"MontserratSemiBold16"
		"xpos"			"5"
		"ypos"			"1"
		"zpos"			"2"
		"wide"			"f"
		"tall"			"20"
		"visible"		"1"
		"enabled"		"1"
		"textAlignment"	"center"
		"labelText"		"A TEAM STOLE THE BALL"
		"fgcolor" 		"PFGrey"
		"border"				""
		"textinsetx"		"0"
		"auto_wide_tocontents"	"1"
		
		"pin_to_sibling"		"EventTitleLabel"
		"pin_corner_to_sibling"	"4"
		"pin_to_sibling_corner"	"6"
	}
	

	"EventBonusLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"EventBonusLabel"
		"font"			"MontserratBlack14"
		"dropshadow"	"1"
		"xpos"			"0"
		"ypos"			"9999" //Disabled the critboost notification
		"zpos"			"3"
		"wide"			"f"
		"tall"			"20"
		"visible"		"1" 
		"enabled"		"1"
		"textAlignment"	"center"
		"labelText"		""
		"fgcolor_override"	"255 235 35 200"
		"border"				""
		"textinsetx"		"30"
		"auto_wide_tocontents"	"1"
		
		"pin_to_sibling"		"EventTitleLabel"
		"pin_corner_to_sibling"	"6"
		"pin_to_sibling_corner"	"4"
	}
	"EventBonusLabelShadow"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"EventBonusLabelShadow"
		"font"			"MontserratBlack14"
		"dropshadow"	"1"
		"xpos"			"0"
		"ypos"			"9999" //Disabled the critboost notification
		"zpos"			"3"
		"wide"			"f"
		"tall"			"20"
		"visible"		"1" 
		"enabled"		"1"
		"textAlignment"	"center"
		"labelText"		""
		"fgcolor_override"	"255 235 35 200"
		"border"				""
		"textinsetx"		"30"
		"auto_wide_tocontents"	"1"
		
		"pin_to_sibling"		"EventTitleLabel"
		"pin_corner_to_sibling"	"6"
		"pin_to_sibling_corner"	"4"
	}

	"ProgressLevelBar"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"ProgressLevelBar"	
		"xpos"			"c-150"
		"ypos"			"r86"
		
		"xpos_minmode"	"cs-0.5"
		"ypos_minmode"	"r116"
		
		"zpos"			"0"
		"wide"			"300"
		"wide_minmode"	"240"
		"tall"			"48"
		"tall_minmode"	"40"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		""
		"image"			"../hud/radar/bar"
		"scaleImage"	"1"
		
		
	}

	"BlueProgressEnd"
	{
		// tells the code where the end of the progress bar is since the image
		// might be padded for npot reasons.
		"ControlName" 	"Panel"
		"fieldName"		"BlueProgressEnd"
		"xpos"			"c-112"
		"ypos"			"r62"
		
		"xpos_minmode"	"c-82"
		"ypos_minmode"	"r96"
		
		"visible"		"0"
	}

	"RedProgressEnd"
	{
		// tells the code where the end of the progress bar is since the image
		// might be padded for npot reasons.
		"ControlName"	"Panel"
		"fieldName"		"RedProgressEnd"
		"xpos"			"c112"
		"ypos"			"r62"
		
		"xpos_minmode"	"c82"
		"ypos_minmode"	"r96"
		
		"visible"		"0"
	}	

	"GoalBlue0"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"GoalBlue0"	
		//"xpos"			"c-162"
		"xpos"			"9999"
		"ypos"			"r72"
		"zpos"			"1"									
		"wide"			"17"
		"tall"			"17"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"image"			"../passtime/hud/passtime_goal_blue_icon"
		"scaleImage"	"1"	
	}

	"GoalBlue1"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"GoalBlue1"	
		//"xpos"			"c-132"
		"xpos"			"9999"
		"ypos"			"r72"
		"zpos"			"1"									
		"wide"			"17"
		"tall"			"17"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"image"			"../passtime/hud/passtime_goal_blue_icon"
		"scaleImage"	"1"	
	}

	"GoalBlue2"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"GoalBlue2"	
		//"xpos"			"c-102"
		"xpos"			"9999"
		"ypos"			"r72"
		"zpos"			"1"									
		"wide"			"17"
		"tall"			"17"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"image"			"../passtime/hud/passtime_goal_blue_icon"
		"scaleImage"	"1"	
	}


	"GoalRed0"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"GoalRed0"	
		//"xpos"			"c146"
		"xpos"			"9999"
		"ypos"			"r72"
		"zpos"			"1"									
		"wide"			"17"
		"tall"			"17"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"image"			"../passtime/hud/passtime_goal_red_icon"
		"scaleImage"	"1"	
	}

	"GoalRed1"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"GoalRed1"
		//"xpos"			"c116"
		"xpos"			"9999"
		"ypos"			"r72"
		"zpos"			"1"									
		"wide"			"17"
		"tall"			"17"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"image"			"../passtime/hud/passtime_goal_red_icon"
		"scaleImage"	"1"	
	}


	"GoalRed2"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"GoalRed2"
		//"xpos"			"c86"
		"xpos"			"9999"
		"ypos"			"r72"
		"zpos"			"1"									
		"wide"			"17"
		"tall"			"17"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"image"			"../passtime/hud/passtime_goal_red_icon"
		"scaleImage"	"1"	
	}

	"ProgressBallIcon"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"ProgressBallIcon"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"10"
		"wide"			"20"
		"tall"			"20"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"image"			"../passtime/hud/passtime_ball"
		"scaleImage"	"1"	
	}	

	"ProgressSelfPlayerIcon"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"ProgressSelfPlayerIcon"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"-100"
		"wide"			"42"
		"tall"			"42"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"scaleImage"	"1"	
	}	

	"ProgressBallCarrierName"
	{
		"ControlName"			"Label"
		"fieldName"				"ProgressBallCarrierName"
		"font"					"MontserratBlack14Shadow"
		"visible"				"1"
		"enabled"				"1"
		"zpos"					"0"
		"xpos"					"0"
		"ypos"					"-22"
		
		"wide"					"180"
		"wide_minmode"			"125"		
		
		"tall"					"16"
		"textAlignment"			"center"
		"dulltext"				"0"
		"brighttext"			"1"
		"labelText"				"NameOfCarrier"
		"border"				""
		"auto_wide_tocontents"	"1"
		"textinsetx"			"20"
		"fgcolor_override"	"255 255 246 200"
		//"bgcolor_override"	"0 0 0 110"
		"pin_to_sibling"	"ProgressBallIcon"
		"pin_corner_to_sibling"		"4"
		"pin_to_sibling_corner"		"4"
	}
	

		//What does this do?
	"CarrierAnchor"
	{	
		"ControlName"			"ImagePanel"
		"fieldName"				"CarrierAnchor"
		"xpos"					"c0"
		"ypos"					"30"
		"zpos"					"2"
		"wide"					"1"
		"tall"					"1"
		"autoResize"			"0"
		"pinCorner"				"0"
		"visible"				"0"
		"enabled"				"1"
		"tabPosition"			"0"	
		"fillcolor"				"255 255 255 255"
		"PaintBackgroundType"	"0"
	}



	"BallPowerCluster" 
	{
		"ControlName" "EditablePanel"
		"fieldName" "BallPowerCluster"
		"xpos" "0"
		"ypos" "9999" //bandaid fix. revert this to 32 if this meter is desired for a new gamemode modifier or whatever reason
		"zpos" "5"
		"wide" "f0"
		"tall" "f0"
		"autoResize"	"0"
		"visible"		"1"
		"enabled"		"1"

		"BallPowerMeterFrame"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"BallPowerMeterFrame"	
			"xpos"			"c-100"
			"ypos"			"0"
			"zpos"			"5"
			"wide"			"200"
			"tall"			"50"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"	"0"
			"image"			"../passtime/hud/passtime_powerball_meter_frame"
			"scaleImage"	"1"					
		}

		"BallPowerMeterFinalSectionContainer"
		{
			// This exists because the bar is filled by changing the width dynamically
			// and if you change the width of the ImagePanel, it will stretch the image.
			// But if you instead change the width of this container, the image is simply
			// cut off instead of stretched.
			// Also so that the bar is clipped nicely to the border.
			"ControlName"		"EditablePanel"
			"fieldName"			"BallPowerMeterFinalSectionContainer"
			"xpos"			"c-85"
			"ypos"			"16"
			"zpos"			"3"
			"wide"			"168"
			"tall"			"18"
			"visible"			"1"
			"enabled"			"1"

			"BallPowerMeterFinalSection"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"BallPowerMeterFinalSection"	
				"xpos"			"0"
				"ypos"			"0"
				"zpos"			"0"
				"wide"			"168"
				"tall"			"18"
				"autoResize"	"0"
				"pinCorner"		"0"
				"visible"		"1"
				"enabled"		"1"
				"tabPosition"	"0"
				"image"			"white"
				"scaleImage"	"1"					
			}
		}
		
		"BallPowerMeterFillContainer"
		{
			// This exists because the bar is filled by changing the width dynamically
			// and if you change the width of the ImagePanel, it will stretch the image.
			// But if you instead change the width of this container, the image is simply
			// cut off instead of stretched.
			// Also so that the bar is clipped nicely to the border.
			"ControlName"		"EditablePanel"
			"fieldName"			"BallPowerMeterFillContainer"
			"xpos"				"c-85"
			"ypos"				"16"
			"zpos"				"4"
			"wide"				"168"
			"tall"				"18"
			"visible"			"1"
			"enabled"			"1"

			"BallPowerMeterFill"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"BallPowerMeterFill"	
				"xpos"			"0"
				"ypos"			"0"
				"zpos"			"0"
				"wide"			"168"
				"tall"			"18"
				"autoResize"	"0"
				"pinCorner"		"0"
				"visible"		"1"
				"enabled"		"1"
				"tabPosition"	"0"
				"image"			"white"
				"scaleImage"	"1"					
			}
		}
	}



	"playericon0"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon0"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon1"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon1"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon2"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon2"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon3"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon3"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon4"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon4"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon5"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon5"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon6"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon6"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon7"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon7"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon8"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon8"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon9"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon9"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon10"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon10"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon11"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon11"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon12"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon12"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon13"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon13"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon14"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon14"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon15"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon15"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon16"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon16"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon17"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon17"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon18"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon18"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon19"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon19"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon20"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon20"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon21"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon21"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon22"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon22"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon23"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon23"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon24"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon24"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon25"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon25"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon26"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon26"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon27"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon27"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon28"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon28"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon29"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon29"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon30"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon30"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon31"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon31"
		"wide" 			"16"
		"tall" 			"16"
	}

	"playericon32"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"playericon32"
		"wide" 			"16"
		"tall" 			"16"
	}
}
