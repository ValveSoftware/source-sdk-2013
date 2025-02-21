"Resource/UI/QuestNotificationPanel_base.res"
{
	"QuestNotificationPanel"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"QuestNotificationPanel"
		"xpos"			"0"
		"ypos"			"100"
		"zpos"			"2"
		"wide"			"f0"
		"tall"			"f0"
		"visible"		"0"
		"proportionaltoparent"	"1"

		"output_step" "50"

		"MainContainer"
		{
			"ControlName"			"EditablePanel"
			"fieldName"				"MainContainer"
			"xpos"					"r0"
			"ypos"					"0"
			"wide"					"110"
			"tall"					"20"
			"visible"				"1"
			"proportionaltoparent"	"1"

			"border"	"QuestStatusBorder"
			"paintborder"	"2"

			"ItemName"
			{
				"ControlName"	"Label"
				"fieldName"		"ItemName"
				"labeltext"		"#QuestNotification_Incoming"
				"xpos"			"0"
				"ypos"			"cs-0.5"
				"wide"			"f0"
				"tall"			"18"
				"zpos"			"4"
				"textinsetx"	"5"
				"fgcolor_override"		"235 226 202 255"
				"font"			"AchievementTracker_Name"
				//"wrap"			"1"
				"TextAlignment"		"east"
				"proportionaltoparent" "1"
			}

			"CharacterImage"
			{
				"ControlName"	"CTFImagePanel"
				"fieldName"		"CharacterImage"
				"xpos"			"3"
				"ypos"			"cs-0.5"
				"zpos"			"0"
				"wide"			"o1"
				"tall"			"p0.8"
				"visible"		"1"
				"enabled"		"1"
				"image"			"animated/tf2_speaker_icon"
				"scaleImage"	"1"	
				"proportionaltoparent"	"1"	
			}
		}	
	}	
}