// Tracker scheme resource file
Scheme
{
	// default settings for all panels
	BaseSettings
	{
		"sc_primary1" "102 102 153 255"
		"sc_primary2" "192 192 172 128"
		"sc_primary3" "204 204 255 255"
		"sc_secondary1" "63 63 63 255"
		"sc_secondary2" "153 153 153 255"
		"sc_secondary3" "204 204 204 255"
		"sc_user" "0 0 0 255"

		"FgColor" "220 220 255 255"
		"BgColor" "0 0 0 255"

		"DisabledFgColor1"		"80 80 80 255" 
		"DisabledFgColor2"		"40 40 40 255"	// set this to the BgColor if you don't want it to draw

		"TitleBarFgColor"			"0 0 0 255"
		"TitleBarDisabledFgColor"	"150 150 150 255"
		"TitleBarBgColor"			"180 120 0 255"
		"TitleBarDisabledBgColor"	"99 99 99 255"

		"TitleBarIcon"				"icon_tracker"
		"TitleBarDisabledIcon"		"icon_tracker_disabled"

		"TitleButtonFgColor"			"97 64 0 255"
		"TitleButtonBgColor"			"197 131 0 255"
		"TitleButtonDisabledFgColor"	"0 0 0 255"
		"TitleButtonDisabledBgColor"	"99 99 99 255"

		"TextCursorColor"			"0 0 0 255"

		Menu
		{
			"FgColor"			"150 150 150 255"
			"BgColor"			"70 70 70 255"
			"ArmedFgColor"		"255 183 0 255"
			"ArmedBgColor"		"134 91 19 255"
			"DividerColor"		"56 56 56 255"

			"TextInset"			"6"			//!! not working yet
		}

		ScrollBar
		{
			"BgColor"			"80 80 80 255"
			"SliderBgColor"		"0 0 00 0"
		}

		// text edit windows
		"WindowFgColor"				"220 220 255 255"
		"WindowBgColor"				"0 0 0 64"
		"WindowDisabledFgColor"		"150 150 150 255"
		"WindowDisabledBgColor"		"80 80 80 255"
		
		// App-specific stuff

		// status selection
		"StatusSelectFgColor"		"255 255 255 255"
		"StatusSelectFgColor2"		"121 121 121 255"

		// buddy buttons
		BuddyButton
		{
			"FgColor1"		"255 183 0 255"
			"FgColor2"		"157 105 0 255"

			"ArmedFgColor1"	"255 183 0 255"
			"ArmedFgColor2"	"255 183 0 255"
			"ArmedBgColor"	"134 91 19 255"
		}

		"SectionTextColor"		"121 121 121 255"
		"SectionDividerColor"	"56 56 56 255"

		// TF2 HUD
		"HudStatusBgColor"			"0 0 0 64"
		"HudStatusSelectedBgColor	"0 0 0 192"
	}

	// describes all the fonts
	Fonts
	{
		"Default"
		{
			"name"		"Tahoma"
			"tall"		"16"
			"weight"	"500"
		}

		"DefaultSmall"
		{
			"name"		"Tahoma"
			"tall"		"12"
			"weight"	"0"
		}

		// this is the symbol font
		"Marlett"
		{
			"name"		"Marlett"
			"tall"		"14"
			"weight"	"0"
		}
	}

	// describes all the images used
	Images
	{
		"icon_tracker"
		{
			"filename"	"resource/icon_tracker"
			"width"		"16"
			"height"	"16"
		}
		"icon_tracker_disabled"
		{
			"filename"	"resource/icon_tracker_disabled"
			"width"		"16"
			"height"	"16"
		}
		"icon_tray"
		{
			"filename"	"resource/icon_tracker"
			"width"		"16"
			"height"	"16"
		}
		"icon_away"
		{
			"filename"	"resource/icon_away"
			"width"		"16"
			"height"	"16"
		}
		"icon_busy"
		{
			"filename"	"resource/icon_busy"
			"width"		"16"
			"height"	"16"
		}
		"icon_online"
		{
			"filename"	"resource/icon_blank"
			"width"		"16"
			"height"	"16"
		}
		"icon_offline"
		{
			"filename"	"resource/icon_blank"
			"width"		"16"
			"height"	"16"
		}
		"icon_connecting"
		{
			"filename"	"resource/icon_blank"
			"width"		"16"
			"height"	"16"
		}
		"icon_message"
		{
			"filename"	"resource/icon_message"
			"width"		"16"
			"height"	"16"
		}
		"icon_blank"
		{
			"filename"	"resource/icon_blank"
			"width"		"16"
			"height"	"16"
		}
	}

	// describes all the border types
	Borders
	{
		BaseBorder
		{
			"inset" "0 0 1 1"
			Left
			{
				"1"
				{
					"color" "43 43 43 255"
					"offset" "0 1"
				}
			}

			Right
			{
				"1"
				{
					"color" "99 99 99 255"
					"offset" "1 0"
				}
			}

			Top
			{
				"1"
				{
					"color" "43 43 43 255"
					"offset" "0 0"
				}
			}

			Bottom
			{
				"1"
				{
					"color" "99 99 99 255"
					"offset" "0 0"
				}
			}
		}
		
		TitleButtonBorder
		{
			"inset" "2 2 1 1"
			Left
			{
				"1"
				{
					"color" "255 170 0 255"
					"offset" "0 1"
				}
			}

			Right
			{
				"1"
				{
					"color" "97 64 0 255"
					"offset" "1 0"
				}
			}

			Top
			{
				"1"
				{
					"color" "255 170 0 255"
					"offset" "0 0"
				}
			}

			Bottom
			{
				"1"
				{
					"color" "97 64 0 255"
					"offset" "0 0"
				}
			}
		}

		TitleButtonDisabledBorder
		{
			"inset" "2 2 1 1"
			Left
			{
				"1"
				{
					"color" "79 79 79 255"
					"offset" "0 1"
				}
			}

			Right
			{
				"1"
				{
					"color" "0 0 0 255"
					"offset" "1 0"
				}
			}
			Top
			{
				"1"
				{
					"color" "79 79 79 255"
					"offset" "0 0"
				}
			}

			Bottom
			{
				"1"
				{
					"color" "0 0 0 255"
					"offset" "0 0"
				}
			}
		}

		TitleButtonDepressedBorder
		{
			"inset" "3 3 1 1"
			Left
			{
				"1"
				{
					"color" "97 64 0 255"
					"offset" "0 1"
				}
			}

			Right
			{
				"1"
				{
					"color" "255 170 0 255"
					"offset" "1 0"
				}
			}

			Top
			{
				"1"
				{
					"color" "97 64 0 255"
					"offset" "0 0"
				}
			}

			Bottom
			{
				"1"
				{
					"color" "255 170 0 255"
					"offset" "0 0"
				}
			}
		}

		ScrollBarButtonBorder
		{
			"inset" "2 2 1 1"
			Left
			{
				"1"
				{
					"color" "79 79 79 255"
					"offset" "0 1"
				}
			}

			Right
			{
				"1"
				{
					"color" "0 0 0 255"
					"offset" "1 0"
				}
			}
			Top
			{
				"1"
				{
					"color" "79 79 79 255"
					"offset" "0 0"
				}
			}

			Bottom
			{
				"1"
				{
					"color" "0 0 0 255"
					"offset" "0 0"
				}
			}
		}

		ButtonBorder
		{
			"inset" "0 0 1 1"
			Left
			{
				"1"
				{
					"color" "99 99 99 255"
					"offset" "0 1"
				}
			}

			Right
			{
				"1"
				{
					"color" "43 43 43 255"
					"offset" "1 0"
				}
			}

			Top
			{
				"1"
				{
					"color" "99 99 99 255"
					"offset" "0 0"
				}
			}

			Bottom
			{
				"1"
				{
					"color" "43 43 43 255"
					"offset" "0 0"
				}
			}
		}

		// this is the border used for default buttons (the button that gets pressed when you hit enter)
		ButtonKeyFocusBorder
		{
			"inset" "0 0 1 1"
			Left
			{
				"1"
				{
					"color" "0 0 0 255"
					"offset" "0 0"
				}
				"2"
				{
					"color" "99 99 99 255"
					"offset" "0 1"
				}
			}
			Top
			{
				"1"
				{
					"color" "0 0 0 255"
					"offset" "0 0"
				}
				"2"
				{
					"color" "99 99 99 255"
					"offset" "1 0"
				}
			}
			Right
			{
				"1"
				{
					"color" "0 0 0 255"
					"offset" "0 0"
				}
				"2"
				{
					"color" "43 43 43 255"
					"offset" "1 0"
				}
			}
			Bottom
			{
				"1"
				{
					"color" "0 0 0 255"
					"offset" "0 0"
				}
				"2"
				{
					"color" "43 43 43 255"
					"offset" "0 0"
				}
			}
		}

		ButtonDepressedBorder
		{
			"inset" "2 1 1 1"
			Left
			{
				"1"
				{
					"color" "43 43 43 255"
					"offset" "0 1"
				}
			}

			Right
			{
				"1"
				{
					"color" "99 99 99 255"
					"offset" "1 0"
				}
			}

			Top
			{
				"1"
				{
					"color" "43 43 43 255"
					"offset" "0 0"
				}
			}

			Bottom
			{
				"1"
				{
					"color" "99 99 99 255"
					"offset" "0 0"
				}
			}
		}

		ComboBoxBorder
		{
			"inset" "0 0 1 1"
			Left
			{
				"1"
				{
					"color" "43 43 43 255"
					"offset" "0 1"
				}
			}

			Right
			{
				"1"
				{
					"color" "99 99 99 255"
					"offset" "1 0"
				}
			}

			Top
			{
				"1"
				{
					"color" "43 43 43 255"
					"offset" "0 0"
				}
			}

			Bottom
			{
				"1"
				{
					"color" "99 99 99 255"
					"offset" "0 0"
				}
			}
		}

		MenuBorder
		{
			"inset" "1 1 1 1"
			Left
			{
				"1"
				{
					"color" "99 99 99 255"
					"offset" "0 1"
				}
			}

			Right
			{
				"1"
				{
					"color" "43 43 43 255"
					"offset" "1 0"
				}
			}

			Top
			{
				"1"
				{
					"color" "99 99 99 255"
					"offset" "0 0"
				}
			}

			Bottom
			{
				"1"
				{
					"color" "43 43 43 255"
					"offset" "0 0"
				}
			}
		}
	}
}