#base "SurveyPanel_Base.res"

"Resource/UI/SurveyPanel_MatchQuality.res"
{
	"Survey"
	{
		"QuestionContainer"
		{
			"TextLabel"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"TextLabel"
				"font"			"HudFontSmallBold"
				"labelText"		"#TF_SurveyQuestion_MatchQuality"
				"textAlignment"	"north"
				"xpos"			"cs-0.5"
				"ypos"			"15"
				"zpos"			"1000"
				"wide"			"p0.85"
				"tall"			"30"
				"autoResize"	"0"
				"pinCorner"		"0"
				"visible"		"1"
				"enabled"		"1"
				"wrap"			"0"
				"centerwrap"	"1"
				"fgcolor_override" "TanLight"
				"proportionaltoparent"	"1"
			}

			"SelectionGroup"
			{
				"ControlName"	"EditablePanel"
				"fieldName"		"SelectionGroup"
				"xpos"			"cs-0.5"
				"ypos"			"50"
				"zpos"			"-1"
				"wide"			"350"
				"tall"			"50"
				"visible"		"1"
				"proportionaltoparent"	"1"
				"bgcolor_override"	"0 0 0 100"

				"InnerShadow"
				{
					"ControlName"	"EditablePanel"
					"fieldName"		"InnerShadow"
					"xpos"			"0"
					"ypos"			"0"
					"zpos"			"100"
					"wide"			"f0"
					"tall"			"f0"
					"visible"		"1"
					"PaintBackgroundType"	"2"
					"border"		"InnerShadowBorder"
					"proportionaltoparent"	"1"
					"mouseinputenabled"	"0"
				}

				"Radio0" // Horrible
				{
					"ControlName"	"RadioButton"
					"fieldName"		"Radio0"
					"xpos"			"p0.1-10"
					"ypos"			"15"
					"zpos"			"1"
					"wide"			"15"
					"tall"			"20"
					"visible"		"1"
					"proportionaltoparent"	"1"

					"sound_depressed"	"UI/buttonclick.wav"
					"sound_released"	"UI/buttonclickrelease.wav"
					
					"labelText"		""
					"Command"		"option0"
				}

				"Radio0Label"
				{
					"ControlName"	"Label"
					"fieldName"		"Radio0Label"
					"xpos"			"p0.1-30"
					"ypos"			"30"
					"zpos"			"1"
					"wide"			"60"
					"tall"			"20"
					"visible"		"1"
					"proportionaltoparent"	"1"
					
					"labelText"		"#TF_SurveyQuestion_Rating0"
					"font"			"HudFontSmallest"
					"textAlignment"	"center"

					"associate"		"Radio0"
				}

				"Radio1" // Bad
				{
					"ControlName"	"RadioButton"
					"fieldName"		"Radio1"
					"xpos"			"p0.3-13"
					"ypos"			"15"
					"zpos"			"2"
					"wide"			"15"
					"tall"			"20"
					"visible"		"1"
					"proportionaltoparent"	"1"

					"sound_depressed"	"UI/buttonclick.wav"
					"sound_released"	"UI/buttonclickrelease.wav"

					"labelText"		""
					"Command"		"option1"
				}

				"Radio1Label"
				{
					"ControlName"	"Label"
					"fieldName"		"Radio1Label"
					"xpos"			"p0.3-36"
					"ypos"			"30"
					"zpos"			"1"
					"wide"			"60"
					"tall"			"20"
					"visible"		"1"
					"proportionaltoparent"	"1"
					
					"labelText"		"#TF_SurveyQuestion_Rating1"
					"font"			"HudFontSmallest"
					"textAlignment"	"center"
				}

				"Radio2" // Neutral
				{
					"ControlName"	"RadioButton"
					"fieldName"		"Radio2"
					"xpos"			"p0.5-10"
					"ypos"			"15"
					"zpos"			"3"
					"wide"			"15"
					"tall"			"20"
					"visible"		"1"
					"proportionaltoparent"	"1"

					"sound_depressed"	"UI/buttonclick.wav"
					"sound_released"	"UI/buttonclickrelease.wav"

					"labelText"		""
					"Command"		"option2"
				}

				"Radio2Label"
				{
					"ControlName"	"Label"
					"fieldName"		"Radio2Label"
					"xpos"			"p0.5-33"
					"ypos"			"30"
					"zpos"			"1"
					"wide"			"60"
					"tall"			"20"
					"visible"		"1"
					"proportionaltoparent"	"1"
					
					"labelText"		"#TF_SurveyQuestion_Rating2"
					"font"			"HudFontSmallest"
					"textAlignment"	"center"
					"default"		"1"
				}

				"Radio3" // Good
				{
					"ControlName"	"RadioButton"
					"fieldName"		"Radio3"
					"xpos"			"p0.7-10"
					"ypos"			"15"
					"zpos"			"4"
					"wide"			"15"
					"tall"			"20"
					"visible"		"1"
					"proportionaltoparent"	"1"

					"sound_depressed"	"UI/buttonclick.wav"
					"sound_released"	"UI/buttonclickrelease.wav"
					
					"labelText"		""
					"Command"		"option3"
				}

				"Radio3Label"
				{
					"ControlName"	"Label"
					"fieldName"		"Radio3Label"
					"xpos"			"p0.7-33"
					"ypos"			"30"
					"zpos"			"1"
					"wide"			"60"
					"tall"			"20"
					"visible"		"1"
					"proportionaltoparent"	"1"
					
					"labelText"		"#TF_SurveyQuestion_Rating3"
					"font"			"HudFontSmallest"
					"textAlignment"	"center"
				}

				"Radio4" // Great
				{
					"ControlName"	"RadioButton"
					"fieldName"		"Radio4"
					"xpos"			"p0.9-10"
					"ypos"			"15"
					"zpos"			"5"
					"wide"			"15"
					"tall"			"20"
					"visible"		"1"
					"proportionaltoparent"	"1"

					"sound_depressed"	"UI/buttonclick.wav"
					"sound_released"	"UI/buttonclickrelease.wav"
					
					"labelText"		""
					"Command"		"option4"
				}

				"Radio4Label"
				{
					"ControlName"	"Label"
					"fieldName"		"Radio4Label"
					"xpos"			"p0.9-33"
					"ypos"			"30"
					"zpos"			"1"
					"wide"			"60"
					"tall"			"20"
					"visible"		"1"
					"proportionaltoparent"	"1"
					
					"labelText"		"#TF_SurveyQuestion_Rating4"
					"font"			"HudFontSmallest"
					"textAlignment"	"center"
				}
			}

			"SubmitButton"
			{
				"ControlName"	"CExButton"
				"fieldName"		"SubmitButton"
				"xpos"			"cs-0.5"
				"ypos"			"rs1-10"
				"zpos"			"1"
				"wide"			"130"
				"tall"			"25"
				"labelText"		"#AbuseReport_Submit"
				"font"			"HudFontSmallBold"
				"textAlignment"	"center"
				"Command"		"submit"
				"sound_depressed"	"UI/buttonclick.wav"
				"sound_released"	"UI/buttonclickrelease.wav"
				"proportionaltoparent"	"1"
				"actionsignallevel"	"2"
			}
		}
	}	
}