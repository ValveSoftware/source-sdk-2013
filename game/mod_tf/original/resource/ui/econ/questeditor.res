"Resource/UI/econ/QuestEditorPanel.res"
{
	"QuestEditor"
	{
		"ControlName"	"Frame"
		"fieldName"		"QuestEditor"
		"xpos"			"cs-0.5"
		"ypos"			"cs-0.5"
		"zpos"			"2"
		"wide"			"1400"
		"tall"			"p0.8"
		"visible"		"1"
		"proportionaltoparent"	"1"

		"P4Warning"
		{
			"ControlName"	"Label"
			"fieldName"		"P4Warning"
			"labelText"		"P4 Unavailable.  Did you forget to run with -p4?"
			"fgcolor_override"	"255 0 0 255"
			"wide"			"f0"
			"tall"			"50"
			"textAlignment"	"center"
			"proportionaltoparent"	"1"
		}

		"EditButton"
		{
			"ControlName"	"CExImageButton"
			"fieldName"		"EditButton"
			"xpos"			"r65"
			"ypos"			"44"
			"zpos"			"10"
			"wide"			"30"
			"tall"			"30"
			"autoResize"	"0"
			"pinCorner"		"1"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"	"0"
			//"labelText"		"#GameUI_GameMenu_Options"
			"font"			"HudFontSmallBold"
			"textAlignment"	"west"
			"textinsetx"	"35"
			"use_proportional_insets" "1"
			"dulltext"		"0"
			"brighttext"	"0"
			"default"		"1"
			"Command"		"open_edit_context"
			"proportionaltoparent"	"1"

			"sound_depressed"	"UI/buttonclick.wav"
			"sound_released"	"UI/buttonclickrelease.wav"
		
			"image_drawcolor"	"235 226 202 255"
			"SubImage"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"SubImage"
				"xpos"			"cs-0.5"
				"ypos"			"cs-0.5"
				"zpos"			"1"
				"wide"			"p0.8"
				"tall"			"p0.8"
				"visible"		"1"
				"enabled"		"1"
				"scaleImage"	"1"
				"image"			"glyph_options"
				"proportionaltoparent"	"1"
			}			
		}

		"NewQuestButton"
		{
			"ControlName"	"Button"
			"fieldName"		"NewQuestButton"
			"xpos"			"20"
			"ypos"			"r35"
			"wide"			"240"
			"proportionaltoparent"	"1"
			"command"		"newquest"
			"labeltext"		"New Quest"
			"PinCorner"		"2"
		}

		"QuestsFilter"
		{
			"ControlName"	"TextEntry"
			"fieldName"		"QuestsFilter"
			"xpos"			"20"
			"ypos"			"35"
			"wide"			"240"
			"tall"			"20"
		}

		"QuestListContainer"
		{
			"ControlName"	"CExScrollingEditablePanel"
			"fieldName"		"QuestListContainer"
			"xpos"			"20"
			"ypos"			"60"
			"zpos"			"5"
			"wide"			"240"
			"tall"			"p0.88"
			"PaintBackgroundType"	"2"
			"fgcolor_override"	"118 107 94 255"	// Gets copied to the scrollbar fgcolor as part of ScrollableEditablePanel
			"bgcolor_override"	"51 47 46 255"
			"autohide_buttons" "1"
			"proportionaltoparent"	"1"
			"autoresize"		"2"

			"bgcolor_override"	"0 0 0 255"
			
			"ScrollBar"
			{
				"ControlName"	"ScrollBar"
				"FieldName"		"ScrollBar"
				"xpos"			"rs1.4"
				"ypos"			"0"
				"tall"			"f0"
				"wide"			"10"
				"zpos"			"1000"
				"nobuttons"		"1"
				"proportionaltoparent"	"1"
				"autoresize"		"2"
				"pincorner"			"3"

				"Slider"
				{
					"PaintBackgroundType"	"0"
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
		}

		"NewConditionButton"
		{
			"ControlName"	"Button"
			"fieldName"		"NewConditionButton"
			"xpos"			"280"
			"ypos"			"r35"
			"wide"			"240"
			"proportionaltoparent"	"1"
			"command"		"newobjcond"
			"labeltext"		"New Objective Condition"
			"PinCorner"		"2"
		}

		"ConditionsFilter"
		{
			"ControlName"	"TextEntry"
			"fieldName"		"ConditionsFilter"
			"xpos"			"280"
			"ypos"			"35"
			"wide"			"240"
			"tall"			"20"
		}

		"QuestObjectiveConditionsContainer"
		{
			"ControlName"	"CExScrollingEditablePanel"
			"fieldName"		"QuestObjectiveConditionsContainer"
			"xpos"			"280"
			"ypos"			"60"
			"zpos"			"5"
			"wide"			"240"
			"tall"			"p0.88"
			"PaintBackgroundType"	"2"
			"fgcolor_override"	"118 107 94 255"	// Gets copied to the scrollbar fgcolor as part of ScrollableEditablePanel
			"bgcolor_override"	"51 47 46 255"
			"autohide_buttons" "1"
			"proportionaltoparent"	"1"
			"autoresize"		"2"

			"bgcolor_override"	"0 0 0 255"
			
			"ScrollBar"
			{
				"ControlName"	"ScrollBar"
				"FieldName"		"ScrollBar"
				"xpos"			"rs1.4"
				"ypos"			"0"
				"tall"			"f0"
				"wide"			"10"
				"zpos"			"1000"
				"nobuttons"		"1"
				"proportionaltoparent"	"1"
				"autoresize"		"2"
				"pincorner"			"3"

				"Slider"
				{
					"PaintBackgroundType"	"0"
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
		}

		"EditingPanel"
		{
			"ControlName"	"CExScrollingEditablePanel"
			"fieldName"		"EditingPanel"
			"xpos"			"540"
			"ypos"			"cs-0.5"
			"zpos"			"2"
			"wide"			"p0.6"
			"tall"			"p0.9"
			"visible"		"1"
			"proportionaltoparent"	"1"
			"PaintBackgroundType"	"2"
			"fgcolor_override"	"118 107 94 255"	// Gets copied to the scrollbar fgcolor as part of ScrollableEditablePanel
			"bgcolor_override"	"51 47 46 255"
			"autoresize"		"3"

			"Scrollbar"
			{
				"ControlName"	"ScrollBar"
				"FieldName"		"ScrollBar"
				"xpos"			"rs1.4"
				"ypos"			"0"
				"tall"			"f0"
				"wide"			"10"
				"zpos"			"1000"
				"nobuttons"		"1"
				"proportionaltoparent"	"1"
				"autoresize"		"2"
				"pincorner"			"3"
				
				"Slider"
				{
					"PaintBackgroundType"	"0"
				}
				
				"nobuttons"		"1"
				"UpButton"
				{
					"ControlName"	"Button"
					"FieldName"		"UpButton"
					"visible"		"0"
					"tall"			"0"
					"wide"			"0"
				}
				
				"DownButton"
				{
					"ControlName"	"Button"
					"FieldName"		"DownButton"
					"visible"		"0"
					"tall"			"0"
					"wide"			"0"
				}
			}
		}
	}	
}