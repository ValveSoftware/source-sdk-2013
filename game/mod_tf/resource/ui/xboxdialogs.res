//----------------------------------------------------------------------------------------
// Master control settings file for all Xbox 360 dialogs. File is loaded once and held by
// BasePanel, then used by the various dialogs as they're created. This prevents the
// expensive DVD access every time a new UI dialog is opened.
//----------------------------------------------------------------------------------------

"resource/XboxDialogs.res"
{

//--------------------------------------
// Controller Dialog
//--------------------------------------
"ControllerDialog.res"
{	
	"ControllerDialog"
	{
		"wide"			"580"
		"wide_hidef"	"580"
		"tall"			"325"
		"tall_hidef"	"500"
	}
}

//--------------------------------------
// Options Dialog
//--------------------------------------
"OptionsDialog.res"
{	
	"OptionsDialog"
	{
		"wide"			"560"
		"wide_hidef"		"760"
		"tall"			"315"
		"tall_hidef"	"500"
		"footer_buttongap_lodef"			"10"
	}
	
	"TitleLabel"
	{
		"ControlName"	"Label"
		"fieldName"		"TitleLabel"
		"font"			"MenuLarge"
		"xpos"			"20"	
		"ypos"			"0"
		"zpos"			"2"
		"wide"			"720"
		"tall"			"35"
		"visible"		"1"
		"enabled"		"1"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"labelText"		"#GameUI_Options"
	}
	
	"OptionsBackgroundLeft"
	{
		"ControlName"		"ImagePanel"
		"fieldName"		"OptionsBackgroundLeft"
		"xpos"		"8"
		"ypos"		"35"
		"zpos"		"50"
		"wide"		"268"
		"wide_hidef"	"368"
		"tall"		"360"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"fillcolor"		"0 0 0 128"
	}
	"OptionsBackgroundRight"
	{
		"ControlName"		"ImagePanel"
		"fieldName"		"OptionsBackgroundRight"
		"xpos"		"284"
		"xpos_hidef"	"384"
		"ypos"		"35"
		"zpos"		"50"
		"wide"		"268"
		"wide_hidef"	"368"
		"tall"		"360"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"fillcolor"		"0 0 0 128"
	}
	
	"OptionsSelectionLeft"
	{
		"ControlName"		"Panel"
		"fieldName"		"OptionsSelectionLeft"
		"xpos"		"12"
		"ypos"			"38"
		"ypos_hidef"	"40"
		"zpos"		"75"
		"wide"		"528"
		"wide_hidef"	"728"
		"tall"			"21"
		"tall_hidef"	"25"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"paintBackgroundType" "3"
	}
	"OptionsSelectionLeft2"
	{
		"ControlName"		"Panel"
		"fieldName"		"OptionsSelectionLeft2"
		"xpos"			"14"
		"ypos"			"40"
		"ypos_hidef"	"42"
		"zpos"			"80"
		"wide"			"524"
		"wide_hidef"		"724"
		"tall"			"17"
		"tall_hidef"	"21"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"paintBackgroundType" "3"
	}
	
	"UpArrow"
	{
		"ControlName" 	"Label"
		"fieldName"		"UpArrow"
		"xpos" 			"502"
		"xpos_hidef"	"702"
		"ypos"			"20"
		"zpos"			"150"
		"wide"			"32"
		"tall"			"32"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		"#GameUI_Icons_UPCURSOR"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"wrap"			"0"
		"font"			"GameUIButtons"
	}
	"DownArrow"
	{
		"ControlName" 	"Label"
		"fieldName"		"DownArrow"
		"xpos" 			"480"
		"xpos_hidef"	"680"
		"ypos"			"398"
		"zpos"			"150"
		"wide"			"32"
		"tall"			"32"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		"#GameUI_Icons_DOWNCURSOR"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"wrap"			"0"
		"font"			"GameUIButtons"
	}
	
	"OptionLabel0"
	{
		"ControlName"	"Label"
		"fieldName"		"OptionLabel0"
		"xpos"			"20"
		"ypos"			"36"
		"ypos_hidef"	"35"
		"zpos"			"100"
		"wide"			"245"
		"wide_hidef"		"345"
		"tall"			"24"
		"tall_hidef"	"35"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"textAlignment"	"west"
		"dulltext"		"0"
		"brighttext"	"0"
		"wrap"			"0"
		"labelText"		""
		"font"			"MenuLarge"
	}	
	"ValueLabel0"
	{
		"ControlName"	"Label"
		"fieldName"		"ValueLabel0"
		"xpos"			"296"
		"xpos_hidef"		"396"
		"ypos"			"36"
		"ypos_hidef"	"35"
		"zpos"			"100"
		"wide"			"245"
		"wide_hidef"		"345"
		"tall"			"24"
		"tall_hidef"	"35"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"textAlignment"	"west"
		"dulltext"		"0"
		"brighttext"	"0"
		"wrap"			"0"
		"labelText"		""
		"font"			"MenuLarge"
	}	
	"ValueBar0"
	{
		"ControlName"		"AnalogBar"
		"fieldName"		"ValueBar0"
		"xpos"			"296"
		"xpos_hidef"		"396"
		"ypos"			"37"
		"ypos_hidef"	"40"
		"zpos"			"100"
		"wide"			"245"
		"wide_hidef"		"345"
		"tall"			"23"
		"tall_hidef"	"25"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"		"0"
		"progress"		"0.5"
	}	
}

"OptionsFooter.res"
{
	"OptionsFooter"
	{
		"ControlName"		"FooterPanel"
		"fieldName"		"OptionsFooter"
		"center"		"1"
		"tall"			"100"
		"tall_lodef"		"60"
		"buttonoffsety_hidef"	"20"
		"buttonoffsety_lodef"	"5"
		"bgcolor"			"46 43 42 255"
		"paintbackground"	"1"
	}	
}


"NewGameFooter.res"
{
	"NewGameFooter"
	{
		"ControlName"		"FooterPanel"
		"fieldName"		"NewGameFooter"
		"center"		"1"
		"tall"			"100"
		"tall_lodef"		"60"
		"buttonoffsety_hidef"	"20"
		"buttonoffsety_lodef"	"5"
		"bgcolor"			"46 43 42 255"
		"paintbackground"	"1"
	}	
}


//------------------------------------
// Message Dialog
//------------------------------------
"MessageDialog.res"
{	
	"MessageDialog"
	{
		"fieldName"			"MessageDialog"
		"xpos_lodef"		"c-220"
		"wide"				"562"
		"wide_lodef"				"435"
		"tall"				"205"
		"tall_lodef"				"180"
		"titlecolor"		"0 0 0 255"
		"messagecolor"		"200 184 151 255"
		"buttontextcolor"	"200 184 151 255"
		"footer_tall"		"50"
		"button_margin"		"8"
		"button_margin_lodef"		"15"
		"button_separator"	"10"
		"activity_indent"	"100"
	}
	
	"TitleLabel"
	{	
		"ControlName"	"Label"
		"fieldName"		"TitleLabel"
		"font"			"MenuLarge"
		"xpos"			"20"	
		"ypos"			"10"
		"zpos"			"2"
		"wide"			"200"
		"tall"			"35"
		"visible"		"0"
		"enabled"		"0"
		"textAlignment"	"Left"
		"dulltext"		"0"
		"brighttext"	"0"
	}

	"MessageLabel"
	{	
		"ControlName"	"Label"
		"fieldName"		"MessageLabel"
		"font_hidef"		"MenuLarge"
		"font_lodef"		"MenuMedium"
		"xpos"			"40"
		"xpos_lodef"			"20"
		"ypos"			"25"
		"ypos_lodef"		"20"
		"zpos"			"2"
		"wide"			"500"
		"wide_lodef"			"390"
		"tall"			"100"
		"wrap"			"1"
		"visible"		"1"
		"textAlignment"	"center"
		"textcolor"		"200 184 151 255"
		"dulltext"		"0"
		"brighttext"	"0"
	}
	
	"AnimatingPanel"
	{
		"ControlName"	"AnimatingImagePanel"
		"fieldName"		"AnimatingPanel"
		"xpos"			"45"
		"xpos_lodef"			"25"
		"ypos"			"40"
		"ypos_lodef"			"27"
		"zpos"			"9"
		"wide"			"75"
		"tall"			"75"
		"scaleImage"	"1"
		"image"			"ico_waiting"
		"frames"		"4"
	}
}

//------------------------------------
// Dialog Menu Item Base
//------------------------------------
"MenuItem.res"
{
	"bottommargin"			"3"
	"bottommargin_lodef"	"3"
	"rightmargin"			"10"

	"menuitemtext"
	{
		"ControlName"	"label"
		"xpos"			"8"
		"ypos"			"8"
		"ypos_lodef"	"3"
		"font"			"MatchmakingDialogMenuLarge"
	}
	
	"menuitemdesc"
	{
		// This label should only be created in code, so don't set "ControlName"
		"xpos"			"8"
		"ypos"			"30"
		"ypos_lodef"	"20"
		"font"			"MatchmakingDialogMenuSmall"
	}
}

//------------------------------------
// Player Menu Item
//------------------------------------
"PlayerItem.res"
{
	"bottommargin"			"4"
	"bottommargin_lodef"	"0"
	
	"rightmargin"			"5"
	"rightmargin_lodef"		"0"

	"menuitemtext"
	{
		"ControlName"	"label"
		"xpos"			"35"
		"xpos_lodef"	"20"
		"ypos"			"3"
		"ypos_lodef"	"1"
		"font"			"MatchmakingDialogMenuSmall"
		"font_lodef"	"MatchmakingDialogMenuSmall"
	}
	
	"voiceicon"
	{
		"ControlName"	"label"
		"xpos"			"5"
		"xpos_lodef"	"5"
		"ypos"			"7"
		"ypos_lodef"	"4"
		"wide"			"16"
		"wide_lodef"	"12"
		"tall"			"16"
		"tall_lodef"	"12"
		"labeltext"		"#TF_Icon_Alert"
		"font"			"GameUIButtonsSmall"
		"font_lodef"	"GameUIButtonsSmallest"
	}

	"readyicon"
	{
		"ControlName"		"label"
		"ypos"			"5"
		"wide"			"16"
		"tall"			"16"
		"font"			"GameUIButtonsSmall"
		"visible"		"0"
		"enabled"		"0"
	}
}

//------------------------------------
// Session Browser Menu Item
//------------------------------------
"BrowserItem.res"
{
	"bottommargin"			"0"
	"bottommargin_lodef"	"3"
	"rightmargin"			"5"
	"rightmargin_lodef"		"10"
	
	"menuitemtext"
	{
		"ControlName"	"label"
		"xpos"			"5"
		"ypos"			"5"
		"font"			"MatchmakingDialogMenuLarge"
		"font_lodef"	"MatchmakingDialogMenuBrowserHostname"
	}

	"players"
	{
		"ControlName"	"label"
		"xpos"			"265"
		"xpos_lodef"	"165"
		"ypos"			"5"
		"ypos_lodef"	"4"
		"font"			"MatchmakingDialogMenuLarge"
		"font_lodef"	"MatchmakingDialogMenuBrowserDetails"
	}

	"scenario"
	{
		"ControlName"	"label"
		"ypos"			"5"
		"ypos_lodef"	"7"
		"font"			"MatchmakingDialogMenuLarge"
		"font_lodef"	"MatchmakingDialogMenuBrowserDetails"
	}

	"ping"
	{
		"ControlName"	"label"
		"xpos"			"425"
		"xpos_lodef"	"325"
		"ypos"			"0"
		"ypos_lodef"	"3"
		"wide"			"30"
		"wide_lodef"	"25"
		"tall"			"30"
		"tall_lodef"	"25"
		"font"			"GameUIButtonsSmall"
	}
}

//------------------------------------
// Session Browser Menu Item
//------------------------------------
"SectionedItem.res"
{	
	"bottommargin"			"0"
	"bottommargin_lodef"	"3"
	
	"menuitemtext"
	{
		"labeltext"	""
		"font"			"MatchmakingDialogMenuLarge"
		"font_lodef"	"MatchmakingDialogMenuBrowserHostname"
	}
}

//------------------------------------
// Options Menu Item
//------------------------------------
"OptionsItem.res"
{
	// MenuItem properties
	"bottommargin"	"0"
	"rightmargin"	"5"
	
	// OptionsItem properties
	"optionsxpos"		"270"
	"optionsxpos_lodef"			"225"
	"optionsminwide"	"20"
	"optionsleftmargin"	"10"
	"optionsfont"		"MatchmakingDialogMenuLarge"
	"arrowgap"			"0"

	"menuitemtext"
	{
		"ControlName"	"label"
		"xpos"			"5"
		"ypos"			"5"
		"font"			"MatchmakingDialogMenuLarge"
	}
	
	"leftarrow"
	{
		"ControlName"	"Label"
		"xpos"			"200"
		"ypos"			"1"
		"ypos_lodef"			"0"
		"wide"			"32"
		"wide_lodef"			"28"
		"tall"			"32"
		"tall_lodef"			"28"
		"font"			"GameUIButtons"
		"labeltext"		"#GameUI_Icons_LEFTCURSOR"
	}

	"rightarrow"
	{
		"ControlName"	"Label"
		"ypos"			"1"
		"ypos_lodef"			"0"
		"wide"			"32"
		"wide_lodef"			"28"
		"tall"			"32"
		"tall_lodef"			"28"
		"font"			"GameUIButtons"
		"labeltext"		"#GameUI_Icons_RIGHTCURSOR"
	}
}

//------------------------------------
// Achievement Item
//------------------------------------
"AchievementItem.res"
{	
	"bottommargin"	"3"
	"rightmargin"	"10"
	
	"icon"
	{
		"ControlName"	"imagepanel"
		"xpos"			"6"
		"ypos"			"6"
		"wide"			"32"
		"tall"			"32"
		"bgcolor"		"0 0 0 255"
		"fgcolor"		"0 0 0 255"
		"scaleImage"		"1"
		"visible"			"0"
	}
	
	"menuitemtext"
	{
		"ControlName"	"label"
		"xpos"			"45"
		"ypos"			"3"
		"font_hidef"	"MenuLarge"
		"font_lodef"	"MatchmakingDialogMenuSmall"
	}
	
	"menuitemdesc"
	{
		"ControlName"	"label"
		"xpos"			"45"
		"ypos"			"25"
		"font_hidef"	"MenuLarge"
		"font_lodef"	"MatchmakingDialogMenuSmall"
	}

	"points"
	{
		"ControlName"	"label"
		"ypos"			"13"
		"ypos_lodef"	"10"
		"font"			"MatchmakingDialogMenuSmall"
	}
	
	"lockedicon"
	{
		"ControlName"	"imagepanel"
		"xpos"			"6"
		"ypos"			"6"
		"wide"			"32"
		"tall"			"32"
		"bgcolor"		"0 0 0 255"
		"fgcolor"		"0 0 0 255"
		"scaleImage"	"1"
		"image"			"hud/icon_locked"
	}
	
	"unlockedicon"
	{
		"ControlName"	"imagepanel"
		"xpos_hidef"	"618"
		"xpos_lodef"	"435"
		"ypos"			"6"
		"ypos_lodef"	"6"
		"wide"			"32"
		"tall"			"32"
		"wide_lodef"	"24"
		"tall_lodef"	"24"
		"bgcolor"		"0 0 0 255"
		"fgcolor"		"0 0 0 255"
		"scaleImage"	"1"
		"visible"		"0"
		"image"			"hud/icon_check"
	}
	
	"PercentageBarBackground" //light grey overall percentage
	{
		"ControlName"		"ImagePanel"
		"fieldName"		"PercentageBarBackground"
		"xpos"			"300"
		"ypos"			"7"
		"wide"		"128"
		"tall"		"15"
		"fillcolor"	"0 0 0 64"
		"zpos"	"10"
		"visible"		"0"
		"enabled"		"1"
	}
	
	"PercentageBar" //dark grey current completed
	{
		"ControlName"		"ImagePanel"
		"fieldName"		"PercentageBar"
		"xpos"			"300"
		"ypos"			"7"
		"wide"		"0"
		"tall"		"15"
		"fillcolor"	"255 255 255 255"
		"zpos"	"20"
		"visible"		"0"
		"enabled"		"1"
	}
	
	"PercentageText" //Percent Text inside the percentage field
	{
		"ControlName"		"Label"
		"fieldName"		"PercentageText"
		"xpos"			"300"
		"ypos"			"7"
		"zpos"		"21"
		"wide"		"128"
		"tall"		"15"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		"0%"
		"textAlignment"		"center"
		"dulltext"		"0"
		"brighttext"		"0"
		"wrap"		"0"
	}
}

//------------------------------------
// Main base panel for matchmaking UI
//------------------------------------
"MatchmakingBasePanel.res"
{
	"TitleBanner"
	{
		"ControlName"	"ImagePanel"
		"xpos"			"0"
		"ypos"			"0"
		"wide"			"f0"
		"tall"			"128"
		"image"			"menu_header"
		"visible_lodef"	"0"
		"enabled_lodef"	"0"
	}
	
	"MatchmakingFooterPanel"
	{
		"ControlName"		"FooterPanel"
		"fgcolor"			"MatchmakingDialogTitleColor" 
		"bgcolor"			"TanDarker"
		"paintbackground"	"1"
		"tall"				"100"
		"tall_lodef"				"60"
		"center"			"1"
		"buttonoffsety"		"20"
		"buttonoffsety_lodef"		"5"
		"button_separator"	"10"
		"button_separator_lodef"	"2"
		"fonttext"			"MatchmakingDialogMenuLarge"
		"fonttext_lodef"			"MatchmakingDialogMenuSmall"
		"buttongap"			"50"
		"buttongap_lodef"			"30"
		"textadjust"		"3"
		"textadjust_lodef"		"0"
	}	
}

//------------------------------------
// Welcome Dialog
//------------------------------------
"WelcomeDialog.res"
{	
	"WelcomeDialog"
	{
		"xpos"			"100"
		"xpos_lodef"			"80"
		"ypos"			"120"
		"ypos_lodef"			"25"
		"borderwidth"	"15"
		
		"Footer"
		{
			"button"
			{
				"text"		"#GameUI_Select"
				"icon"		"#GameUI_Icons_A_BUTTON"
			}
		}
	}
	
	"DialogTitle"
	{
		"xpos"			"15"
		"ypos"			"15"
		"ypos_lodef"			"10"
		"font"			"MatchmakingDialogTitle"
		"labeltext"		"#TF_Welcome"
	}
	
	"DialogMenu"
	{
		"xpos"			"15"
		"ypos"			"50"
		"ypos_lodef"			"45"
 		"itemspacing"	"2"
		"minwide"		"300"

		"CommandItem"
 		{
 			"label"			"#TF_PlayerMatch_Title"
 			"description"	"#TF_PlayerMatch_Desc"
 			"command"		"OpenPlayerMatchDialog"
 		}
 		"CommandItem"
 		{
 			"label"			"#TF_RankedMatch_Title"
 			"description"	"#TF_RankedMatch_Desc"
 			"command"		"OpenRankedMatchDialog"
 		}
  		"CommandItem"
  		{
  			"label"			"#TF_SystemLink_Title"
  			"command"		"OpenSystemLinkDialog"
  		}
  		"CommandItem"
		{
			"label"			"#TF_LoadCommentary"
			"command"		"OpenLoadSingleplayerCommentaryDialog"
		}
 		"CommandItem"
 		{
 			"label"			"#TF_Achievements_Title"
 			"command"		"OpenAchievementsDialog"
 		}
 		"CommandItem"
 		{
 			"label"			"#TF_Rankings_Title"
 			"command"		"OpenRankingsDialog"
 		}
 		"CommandItem"
		{
			"label"			"#TF_Controller_Title"
			"command"		"OpenControllerDialog"
		}
 		"CommandItem"
 		{
 			"label"			"#TF_Options_Title"
 			"command"		"OpenOptionsDialog"
 		}
 		"CommandItem"
 		{
 			"label"			"#TF_Quit_Title"
 			"command"		"Quit"
 		}
	}
}

//------------------------------------
// Pause Dialog
//------------------------------------
"PauseDialog.res"
{	
	"PauseDialog"
	{
		"xpos"			"100"
		"xpos_lodef"			"100"
		"ypos"			"200"
		"ypos_lodef"			"50"
		"borderwidth"	"15"
		
		"Footer"
		{
			"button"
			{
				"text"		"#GameUI_Back"
				"icon"		"#GameUI_Icons_B_BUTTON"
			}
		
			"button"
			{
				"text"		"#GameUI_Select"
				"icon"		"#GameUI_Icons_A_BUTTON"
			}
		}
	}
	
	"DialogTitle"
	{
		"xpos"			"15"
		"ypos"			"15"
		"ypos_lodef"			"10"
		"font"			"MatchmakingDialogTitle"
		"labeltext"		"#TF_Paused_Title"
	}
	
	"DialogMenu"
	{
		"xpos"			"15"
		"ypos"			"50"
		"ypos_lodef"			"45"
 		"itemspacing"	"2"
		"minwide"		"425"
		"minwide_lodef"			"350"
		
  		"CommandItem"
 		{
 			"label"			"#TF_ChangeClass"
 			"command"		"ChangeClass"
 		}
  		"CommandItem"
 		{
 			"label"			"#TF_ChangeTeam"
 			"command"		"ChangeTeam"
 			"ranked"		"0"
 		}
   		"CommandItem"
 		{
 			"label"			"#TF_MapInfo"
 			"command"		"ShowMapInfo"
 		}		
		"CommandItem"
 		{
 			"label"			"#TF_Achievements_Title"
 			"command"		"OpenAchievementsDialog"
 		}
 		"CommandItem"
		{
			"label"			"#TF_Controller_Title"
			"command"		"OpenControllerDialog"
		}
 		"CommandItem"
 		{
 			"label"			"#TF_Options_Title"
 			"command"		"OpenOptionsDialog"
 		}
 		"CommandItem"
 		{
 			"label"			"#GameUI_Disconnect"
 			"command"		"Disconnect"
 		}
	}
}

//------------------------------------
// Player Match Dialog
//------------------------------------
"PlayerMatchDialog.res"
{	
	"PlayerMatchDialog"
	{
		"xpos"			"100"
		"xpos_lodef"			"100"
		"ypos"			"250"
		"ypos_lodef"			"100"
		"borderwidth"	"15"
		
		"Footer"
		{
			"button"
			{
				"text"		"#GameUI_Back"
				"icon"		"#GameUI_Icons_B_BUTTON"
			}
		
			"button"
			{
				"text"		"#GameUI_Select"
				"icon"		"#GameUI_Icons_A_BUTTON"
			}
		}
	}
	
	"DialogTitle"
	{
		"xpos"			"15"
		"ypos"			"15"
		"ypos_lodef"			"10"
		"font"			"MatchmakingDialogSessionOptionsTitle"
		"labeltext"		"#TF_Unranked"
	}
	
	"DialogMenu"
	{
		"xpos"			"15"
		"ypos"			"50"
		"ypos_lodef"			"45"
 		"itemspacing"	"2"
		"minwide"		"425"
		"minwide_lodef"			"350"

 		"CommandItem"
 		{
 			"label"			"#TF_QuickMatch_Title"
 			"description"	"#TF_QuickMatch_Desc"
 			"command"		"StartQuickMatchClient_Standard"
 		}
 		"CommandItem"
 		{
 			"label"			"#TF_HostMatch_Title"
 			"description"	"#TF_HostMatch_Desc"
 			"command"		"SessionOptions_HostStandard"
 		}
  		"CommandItem"
  		{
  			"label"			"#TF_CustomMatch_Title"
  			"description"	"#TF_CustomMatch_Desc"
 			"command"		"SessionOptions_ClientStandard"
  		}
	}
}

//------------------------------------
// Ranked Match Dialog
//------------------------------------
"RankedMatchDialog.res"
{	
	"RankedMatchDialog"
	{
		"xpos"			"100"
		"xpos_lodef"			"100"
		"ypos"			"250"
		"ypos_lodef"			"100"
		"borderwidth"	"15"
		
		"Footer"
		{
			"button"
			{
				"text"		"#GameUI_Back"
				"icon"		"#GameUI_Icons_B_BUTTON"
			}
		
			"button"
			{
				"text"		"#GameUI_Select"
				"icon"		"#GameUI_Icons_A_BUTTON"
			}
		}
	}
	
	"DialogTitle"
	{
		"xpos"			"15"
		"ypos"			"15"
		"ypos_lodef"			"10"
		"font"			"MatchmakingDialogSessionOptionsTitle"
		"labeltext"		"#TF_Ranked"
	}
	
	"DialogMenu"
	{
		"xpos"			"15"
		"ypos"			"50"
		"ypos_lodef"			"45"
 		"itemspacing"	"2"
		"minwide"		"425"
		"minwide_lodef"		"350"

 		"CommandItem"
 		{
 			"label"			"#TF_QuickMatch_Title"
 			"description"	"#TF_QuickMatch_Desc"
 			"command"		"StartQuickMatchClient_Ranked"
 		}
 		"CommandItem"
 		{
 			"label"			"#TF_HostMatch_Title"
 			"description"	"#TF_HostMatch_Desc"
 			"command"		"SessionOptions_HostRanked"
 		}
  		"CommandItem"
  		{
  			"label"			"#TF_CustomMatch_Title"
  			"description"	"#TF_CustomMatch_Desc"
 			"command"		"SessionOptions_ClientRanked"
  		}
	}
}

//------------------------------------
// System Link Dialog
//------------------------------------
"SystemLinkDialog.res"
{	
	"SystemLinkDialog"
	{
		"xpos"			"100"
		"xpos_lodef"			"100"
		"ypos"			"250"
		"ypos_lodef"			"100"
		"borderwidth"	"25"
		
		"Footer"
		{
			"button"
			{
				"text"		"#GameUI_Back"
				"icon"		"#GameUI_Icons_B_BUTTON"
			}
		
			"button"
			{
				"text"		"#GameUI_Select"
				"icon"		"#GameUI_Icons_A_BUTTON"
			}
		}
	}
	
	"DialogTitle"
	{
		"xpos"			"15"
		"ypos"			"15"
		"ypos_lodef"			"10"
		"font"			"MatchmakingDialogSessionOptionsTitle"
		"labeltext"		"#TF_SystemLink_Title"
	}
	
	"DialogMenu"
	{
		"xpos"			"15"
		"ypos"			"50"
		"ypos_lodef"			"45"
 		"itemspacing"	"2"
		"minwide"		"300"
		"minwide_hidef"		"480"

  		"CommandItem"
 		{
 			"label"			"#TF_SystemLink_Join_Title"
 			"description"	"#TF_SystemLink_Join_Desc"
 			"command"		"StartSystemLinkClient"
 		}
		"CommandItem"
 		{
 			"label"			"#TF_SystemLink_Host_Title"
 			"description"	"#TF_SystemLink_Host_Desc"
 			"command"		"SessionOptions_SystemLink"
 		}
	}
}

//------------------------------------
// Achievements Dialog
//------------------------------------
"AchievementsDialog.res"
{	
	"AchievementsDialog"
	{
		"xpos_lodef"	"c-265"
		"ypos_lodef"	"40"
		"borderwidth"	"15"
		"tall_hidef"	"475"
		"tall_lodef"	"367"
		"gametotal"		"100"
		"center_lodef"	"0"
		
		"Footer"
		{
			"hide_regular_footer"	"1"
			"button"
			{
				"text"		"#GameUI_Back"
				"icon"		"#GameUI_Icons_B_BUTTON"
			}
		}
	}

	"DialogTitle"
	{
		"xpos"			"15"
		"ypos"			"15"
		"font"			"MatchmakingDialogTitle"
		"labeltext"		"#TF_Achievements_Dialog_Title"
	}
	
	"ProgressBg"
	{
		"xpos_hidef"	"415"
		"xpos_lodef"	"250"
		"ypos"			"50"
		"wide"			"298"
		"wide_lodef"	"264"
		"tall"			"10"
	}
	
	"ProgressPercent"
	{
		"ControlName"	"label"
		"font"			"MatchmakingDialogMenuSmall"
	}
	
	"Numbering"
	{
		"ControlName"	"label"
		"font"			"MatchmakingDialogMenuSmall"
	}

	"uparrow"
	{
		"xpos_lodef"	"460"
		"xpos_hidef"	"660"
		"ypos_lodef"	"325"
		"ypos_hidef"	"410"
		"ControlName"	"Label"
		"font"			"GameUIButtons"
		"labeltext"		"#GameUI_Icons_UPCURSOR"
	}

	"downarrow"
	{
		"xpos_lodef"	"482"
		"xpos_hidef"	"682"
		"ypos_lodef"	"325"
		"ypos_hidef"	"410"
		"ControlName"	"Label"
		"font"			"GameUIButtons"
		"labeltext"		"#GameUI_Icons_DOWNCURSOR"
	}
	
	"DialogMenu"
	{
		"xpos"				"15"
		"ypos"				"90"
		"ypos_lodef"				"85"
 		"itemspacing"		"3"
 		"itemspacing_lodef"		"2"
		"minwide_hidef"		"700"
		"minwide_lodef"		"500"
		"maxvisibleitems_hidef"	"6"
		"maxvisibleitems_lodef"	"4"

		// Menu items are returned by a system query and created at runtime
	}
}

//------------------------------------
// Rankings Dialog
//------------------------------------
"RankingsDialog.res"
{	
	"RankingsDialog"
	{
		"xpos"			"100"
		"xpos_lodef"			"100"
		"ypos"			"250"
		"ypos_lodef"			"100"
		"borderwidth"	"15"
		
		"Footer"
		{
			"button"
			{
				"text"		"#GameUI_Back"
				"icon"		"#GameUI_Icons_B_BUTTON"
			}
		
			"button"
			{
				"text"		"#GameUI_Select"
				"icon"		"#GameUI_Icons_A_BUTTON"
			}
		}
	}
	
	"DialogTitle"
	{
		"xpos"			"15"
		"ypos"			"15"
		"ypos_lodef"			"10"
		"font"			"MatchmakingDialogTitle"
		"labeltext"		"#TF_Rankings_Title"
	}
	
	"DialogMenu"
	{
		"xpos"			"15"
		"ypos"			"50"
		"ypos_lodef"			"45"
 		"itemspacing"	"2"
		"minwide"		"425"
		"minwide_lodef"			"350"

 		"CommandItem"
 		{
 			"label"			"#TF_PersonalStats_Title"
 			"command"		"engine showstatsdlg"
 		}
 		"CommandItem"
 		{
 			"label"			"#TF_RankedLeaderboards_Title"
 			"command"		"LeaderboardDialog_Ranked"
 		}
 		"CommandItem"
 		{
 			"label"			"#TF_StatsLeaderboards_Title"
			"command"		"LeaderboardDialog_Stats"
 		}
	}
}


//----------------------------------------------------------------
// Scenario Info Panel - For all session options and lobby dialogs
//----------------------------------------------------------------
"ScenarioInfoPanel.res"
{
	"ScenarioInfoPanel"
	{
		"xpos"			"495"
		"xpos_lodef"	"380"
		"ypos"			"45"
		"ypos_lodef"	"50"
		"wide"			"286"
		"wide_lodef"	"165"
		"tall"			"270"
		"tall_lodef"	"250"
	}
	
	"Title"
	{
		"ControlName"	"Label"
		"font"			"MatchmakingDialogTitle"
		"xpos"			"15"
		"xpos_lodef"	"10"
		"ypos"			"10"
		"wide"			"271"
		"wide_lodef"			"145"
		"tall"			"28"
	}

	"Subtitle"
	{
		"ControlName"	"Label"
		"labelText"		""
		"font"			"MatchmakingDialogMenuLarge"
		"font_lodef"			"MatchmakingDialogMenuSmall"
		"xpos"			"15"
		"xpos_lodef"	"10"
		"ypos"			"35"
		"ypos_lodef"			"30"
		"wide"			"271"
		"wide_lodef"			"145"
		"tall"			"24"
		"tall_lodef"	"40"
		"textAlignment"	"north-west"
		"wrap"			"1"
	}
	
	"MapImage"
	{
		"ControlName"	"ImagePanel"
		"xpos"			"15"
		"xpos_lodef"	"10"
		"ypos"			"60"
		"ypos_lodef"			"70"
		"wide"			"256"
		"wide_lodef"			"145"
		"tall"			"190"
		"tall_lodef"			"108"
		"scaleImage_lodef"	"1"
	}

	"DescOne"
	{
		"ControlName"	"Label"
		"labelText"		""
		"font"			"MatchmakingDialogMenuLarge"
		"font_lodef"			"MatchmakingDialogMenuSmall"
		"xpos"			"15"
		"xpos_lodef"	"10"
		"ypos"			"70"
		"wide"			"271"
		"wide_lodef"			"145"
		"tall"			"24"
		"tall_lodef"	"48"
		"textAlignment"	"north-west"
	}
	"DescTwo"
	{
		"ControlName"	"Label"
		"labelText"		""
		"font"			"MatchmakingDialogMenuLarge"
		"font_lodef"			"MatchmakingDialogMenuSmall"
		"xpos"			"15"
		"xpos_lodef"	"10"
		"ypos"			"90"
		"wide"			"271"
		"wide_lodef"			"145"
		"tall"			"24"
	}
	"DescThree"
	{
		"ControlName"	"Label"
		"labelText"		""
		"font"			"MatchmakingDialogMenuLarge"
		"font_lodef"			"MatchmakingDialogMenuSmall"
		"xpos"			"15"
		"xpos_lodef"	"10"
		"ypos"			"110"
		"wide"			"271"
		"wide_lodef"			"145"
		"tall"			"24"
	}
}

//------------------------------------
// Session Options
//------------------------------------
"SessionOptions.res"
{
	"SessionOptions"
	{		
		"wide"			"800"
		"wide_lodef"			"560"
		"tall"			"340"
		"tall_lodef"			"280"
		"borderwidth"	"30"
		
		"SessionProperty"
		{
			"id"		"PROPERTY_NUMBER_OF_TEAMS"
			"value"		"2"
			"valuetype"	"int"
		}

		"hoststandard"
		{
			"title"			"#TF_PlayerMatch_Host_Title"
			"commandstring"	"StartHost"
			"SessionFlag"	"SESSION_CREATE_LIVE_MULTIPLAYER_STANDARD"		
			"SessionContext"
			{
				"id"		"CONTEXT_GAME_MODE"
				"value"		"CONTEXT_GAME_MODE_MULTIPLAYER"
			}
			"SessionContext"
			{
				"id"		"CONTEXT_GAME_TYPE"
				"value"		"CONTEXT_GAME_TYPE_STANDARD"
			}
		}
		
		"hostranked"
		{
			"title"			"#TF_RankedMatch_Host_Title"
			"commandstring"	"StartHost"
			"SessionFlag"	"SESSION_CREATE_LIVE_MULTIPLAYER_RANKED"			
			"SessionContext"
			{
				"id"		"CONTEXT_GAME_MODE"
				"value"		"CONTEXT_GAME_MODE_MULTIPLAYER"
			}
			"SessionContext"
			{
				"id"		"CONTEXT_GAME_TYPE"
				"value"		"CONTEXT_GAME_TYPE_RANKED"          
			}
			"SessionProperty"
			{
				"id"		"PROPERTY_WIN_LIMIT"
				"value"		"2"
				"valuetype"		"int"
			}
			"SessionProperty"
			{
				"id"		"PROPERTY_AUTOBALANCE"
				"value"		"0"
				"valuetype"		"int"
			}
		}
		
		"systemlink"
		{
			"title"			"#TF_SystemLink_Host_Dialog"
			"commandstring"		"StartSystemLinkHost"
			"SessionFlag"		"SESSION_CREATE_SYSTEMLINK"
			"SessionContext"
			{
				"id"		"CONTEXT_GAME_MODE"
				"value"		"CONTEXT_GAME_MODE_MULTIPLAYER"
			}
			"SessionContext"
			{
				"id"		"CONTEXT_GAME_TYPE"
				"value"		"CONTEXT_GAME_TYPE_STANDARD"          
			}
		}
		
		"clientstandard"
		{
			"title"			"#TF_PlayerMatch_Client_Title"
			"commandstring"	"StartClient"		
			"SessionFlag"	"SESSION_CREATE_LIVE_MULTIPLAYER_STANDARD"
			"SessionContext"
			{
				"id"		"CONTEXT_GAME_MODE"
				"value"		"CONTEXT_GAME_MODE_MULTIPLAYER"
			}
			"SessionContext"
			{
				"id"		"CONTEXT_GAME_TYPE"
				"value"		"CONTEXT_GAME_TYPE_STANDARD"
			}
		}
		
		"clientranked"
		{
			"title"			"#TF_RankedMatch_Client_Title"
			"commandstring"	"StartClient"
			"SessionFlag"	"SESSION_CREATE_LIVE_MULTIPLAYER_RANKED"			
			"SessionContext"
			{
				"id"		"CONTEXT_GAME_MODE"
				"value"		"CONTEXT_GAME_MODE_MULTIPLAYER"
			}
			"SessionContext"
			{
				"id"		"CONTEXT_GAME_TYPE"
				"value"		"CONTEXT_GAME_TYPE_RANKED"
			}
		}
				
		"modify"
		{
			"title"			"#TF_ModifyMatch_Title"
			"commandstring"	"ModifySession"
		}
		
		// These should be in the same order as the scenario defines in hl2orange.spa.h
		"ScenarioInfoPanels"
		{
			"ScenarioInfo"
			{
				"title"			"2Fort"
				"subtitle"		"#Gametype_CTF"
				"image"			"maps/menu_screen_ctf_2fort"
				"tall_lodef"	"172"
			}
			"ScenarioInfo"
			{
				"title"			"Dustbowl"
				"subtitle"		"#TF_AttackDefend"
				"image"			"maps/menu_screen_cp_dustbowl"
				"tall_lodef"	"172"
			}
			"ScenarioInfo"
			{
				"title"			"Granary"
				"subtitle"		"#Gametype_CP"
				"image"			"maps/menu_screen_cp_granary"
				"tall_lodef"	"172"
			}
			"ScenarioInfo"
			{
				"title"			"Well"
				"subtitle"		"#Gametype_CP"
				"image"			"maps/menu_screen_cp_well"
				"tall_lodef"	"172"
			}
			"ScenarioInfo"
			{
				"title"			"Gravel Pit"
				"subtitle"		"#TF_AttackDefend"
				"image"			"maps/menu_screen_cp_gravelpit"
				"tall_lodef"	"172"
			}
			"ScenarioInfo"
			{
				"title"			"Hydro"
				"subtitle"		"#TF_TerritoryControl"
				"image"			"maps/menu_screen_tc_hydro"
				"tall_lodef"	"172"
			}
		}
		
		"Footer"
		{
			"button"
			{
				"text"		"#GameUI_Back"
				"icon"		"#GameUI_Icons_B_BUTTON"
			}
		
			"button"
			{
				"text"		"#GameUI_Accept"
				"icon"		"#GameUI_Icons_A_BUTTON"
			}
		}
	}
	
	"DialogTitle"
	{
		"xpos"			"15"
		"ypos"			"15"
		"ypos_lodef"			"10"
		"font"			"MatchmakingDialogSessionOptionsTitle"
	}
	
	"RecommendedLabel"
	{
		"ControlName"	"Label"
		"xpos"			"15"
		"ypos"			"320"
		"ypos_lodef"	"240"
		"wide"			"750"
		"wide_lodef"	"500"
		"tall"			"50"
		"zpos"			"1"
		"font"			"MatchmakingDialogMenuMediumSmall"
		"labeltext"		"#TF_Recommended_Players"
		"textAlignment"	"east"
		"visible"		"1"
		"wrap"			"1"
	}
	
	"DialogMenu"
	{
		"xpos"			"15"
		"xpos_lodef"			"10"
		"ypos"			"70"
		"ypos_lodef"			"50"
		"minwide"		"400"
		"minwide_lodef"			"380"
 		"itemspacing"	"2"
	
		"OptionsItem"
		{
			"label"			"#TF_MatchOption_Scenario"
			"id"			"CONTEXT_SCENARIO"
			"activeoption"		"0"
			
			"hoststandard"		"1"
			"hostranked"		"1"
			"clientstandard"	"1"
			"clientranked"		"1"
			"systemlink"		"1"
			"modify"			"1"

			"Option"
			{
				"label"			"2Fort"
				"value"			"CONTEXT_SCENARIO_CTF_2FORT"
			}				
			"Option"
			{
				"label"			"Dustbowl"
				"value"			"CONTEXT_SCENARIO_CP_DUSTBOWL"
			}				
			"Option"
			{
				"label"			"Granary"
				"value"			"CONTEXT_SCENARIO_CP_GRANARY"
			}				
			"Option"
			{
				"label"			"Well"
				"value"			"CONTEXT_SCENARIO_CP_WELL"
			}				
			"Option"
			{
				"label"			"Gravel Pit"
				"value"			"CONTEXT_SCENARIO_CP_GRAVELPIT"
			}				
			"Option"
			{
				"label"			"Hydro"
				"value"			"CONTEXT_SCENARIO_TC_HYDRO"
			}				
// 			"Option"
// 			{
// 				"label"			"Cloak"
// 				"value"			"CONTEXT_SCENARIO_CTF_CLOAK"
// 			}				
// 			"Option"
// 			{
// 				"label"			"CP_Cloak"
// 				"value"			"CONTEXT_SCENARIO_CP_CLOAK"
// 			}				
		}

		"OptionsItem"
		{
			"label"			"#TF_MatchOption_GameSize"
			"id"			"PROPERTY_GAME_SIZE"
			"valuetype"		"int"
			"activeoption"	"2"

			"hoststandard"		"1"
			"hostranked"		"1"
			"clientstandard"	"1"
			"clientranked"		"1"
			"systemlink"		"1"

			"Option"
			{
				"label"			"#TF_GameSizeFmt"
				"value"			"8"
			}				
			"Option"
			{
				"label"			"#TF_GameSizeFmt"
				"value"			"12"
			}				
			"Option"
			{
				"label"			"#TF_GameSizeFmt"
				"value"			"16"
			}				
		}
		
		"OptionsItem"
		{
			"label"			"#TF_MatchOption_PrivateSlots"
			"id"			"PROPERTY_PRIVATE_SLOTS"
			"valuetype"		"int"
			"activeoption"	"0"

			"hoststandard"	"1"

			"userange"		"1"
			"rangelow"		"0"
			"rangehigh"		"16"
			"interval"		"1"
		}
		
		"OptionsItem"
		{
			"label"			"#TF_MatchOption_WinLimit"
			"id"			"PROPERTY_WIN_LIMIT"
			"valuetype"		"int"
			"activeoption"	"1"
			
			"hoststandard"		"1"
			"clientstandard"	"1"
			"systemlink"		"1"
			"modify"			"1"
			
			"userange"		"1"
			"rangelow"		"1"
			"rangehigh"		"5"
			"interval"		"1"		
		}

		"OptionsItem"
		{
			"label"			"#TF_MatchOption_MaxTime"
			"id"			"PROPERTY_MAX_GAME_TIME"
			"valuetype"		"int"
			"activeoption"	"2"

			"hoststandard"		"1"
			"hostranked"		"1"
			"clientstandard"	"1"
			"clientranked"		"1"
			"systemlink"		"1"
			"modify"			"1"
			
			"Option"
			{
				"label"			"#TF_MaxTimeFmt"
				"value"			"15"
			}				
			"Option"
			{
				"label"			"#TF_MaxTimeFmt"
				"value"			"30"
			}
			"Option"
			{
				"label"			"#TF_MaxTimeFmt"
				"value"			"45"
			}	
			"Option"
			{
				"label"			"#TF_MaxTimeFmt"
				"value"			"60"
			}			
			"Option"
			{
				"label"			"#TF_MaxTimeNoLimit"
				"value"			"100000"
			}	
		}
				
		"OptionsItem"
		{
			"label"			"#TF_MatchOption_AutoBalance"
			"id"			"PROPERTY_AUTOBALANCE"
			"valuetype"		"int"
			"activeoption"	"0"

			"hoststandard"		"1"
			"modify"			"1"
			
			"Option"
			{
				"label"			"#TF_On"
				"value"			"1"
			}

			"Option"
			{
				"label"			"#TF_Off"
				"value"			"0"
			}				
		}
	}
}

//------------------------------------
// Host Lobby
//------------------------------------
"SessionLobby_Host.res"
{
	"SessionLobby_Host"
	{		
		"xpos_lodef"				"c-294"
		"ypos_lodef"				"15"
		"wide"				"900"
		"wide_lodef"				"588"
		"tall"				"510"
		"tall_lodef"				"405"
		"borderwidth"		"30"
		"teamspacing"		"10"
		"teamspacing_lodef"			"3"
				
		"commandstring"		"StartHost"
		"hostlobby"			"1"
		"center_lodef"				"0"
		"footer_buttongap_hidef"			"20"
		"footer_buttongap_lodef"			"15"
		
		"Footer"
		{
			"button"
			{
				"text"		"#GameUI_ExitLobby"
				"icon"		"#GameUI_Icons_B_BUTTON"
			}
		
			"button"
			{
				"text"		"#GameUI_ChangeTeam"
				"icon"		"#GameUI_Icons_X_BUTTON"
			}
			
			"button"
			{
				"text"		"#TF_ViewGamercard"
				"icon"		"#GameUI_Icons_A_BUTTON"
			}
			
			"button"
			{
				"text_hidef"		"#GameUI_PlayerReview"
				"icon_hidef"		"#GameUI_Icons_R_SHOULDER"
			}	
		}
	}
	
	"ReviewPlayerButton"
	{
		"ControlName"	"Label"
		"labeltext"		"#GameUI_Icons_R_SHOULDER"
		"font"			"GameUIButtons"
		"xpos"			"362"
		"ypos"			"360"
		"wide"			"64"
		"tall"			"32"
		"zpos"			"3"
		"visible"		"0"
		"visible_lodef"	"1"
	}
	
	"PlayerReviewLabel"
	{
		"ControlName"	"Label"
		"labeltext"		"#GameUI_PlayerReview"
		"font"			"MatchmakingDialogMenuMediumSmall"
		"xpos"			"420"
		"ypos"			"367"
		"wide"			"300"
		"visible"		"0"
		"visible_lodef"	"1"
	}
	
	"DialogTitle"
	{
		"xpos"			"15"
		"xpos_lodef"	"25"
		"ypos"			"15"
		"ypos_lodef"			"10"
		"font"			"MatchmakingDialogTitle"
		"labeltext"		"#TF_Lobby_Title"
	}
	
	"HostLabel"
	{
		"xpos"			"15"
		"xpos_lodef"	"25"
		"ypos"			"45"
		"ypos_lodef"	"50"
		"wide"			"350"
		"wide_lodef"	"250"
		"tall"			"40"
		"tall_hidef"	"55"
		"font"			"MatchmakingDialogMenuLarge"
		"font_lodef"	"MatchmakingDialogMenuSmall"
	}

	"LobbyStateBg"
	{
		"xpos"			"15"
		"ypos"			"120"
		"ypos_lodef"	"100"
		"wide"			"370"
		"wide_lodef"	"233"
		"tall"			"35"
	}
	
	"LobbyStateIcon"
	{
		"ControlName"	"Label"
		"labeltext"		"#TF_Icon_Start"
		"font"			"GameUIButtons"
		"xpos"			"25"
		"ypos"			"121"
		"ypos_lodef"	"101"
		"wide"			"32"
		"tall"			"32"
	}

	"LobbyStateLabel"
	{
		"xpos"			"68"
		"xpos_lodef"	"53"
		"ypos"			"125"
		"ypos_lodef"	"105"
		"wide"			"350"
		"wide_lodef"	"223"
		"tall"			"30"
		"font"			"MatchmakingDialogMenuLarge"
		"font_lodef"	"MatchmakingDialogMenuMediumSmall"
		"labeltext"		"#game_WaitingForPlayers"
	}
	
	"GameScenario"
	{
		"ControlName"	"CScenarioInfoPanel"
		"xpos"			"15"
		"ypos"			"162"
		"ypos_lodef"	"142"
		"wide"			"370"
		"wide_lodef"	"233"
		"tall"			"190"
		"tall_lodef"	"155"
		
		// If there is a "PropertyString" entry, then it is used as a lookup
		// to set the label text from the keyvalues that get passed into the dialog
		"Title"
		{
			"PropertyString"	"CONTEXT_SCENARIO"
			"font"				"MatchmakingDialogTitle"
			"xpos"				"15"
			"ypos"				"15"
			"wide"				"275"
		}
		"Subtitle"
		{
			"PropertyString"	"scenariotype"
			"font"				"MatchmakingDialogMenuLarge"
			"xpos"				"15"
			"ypos"				"45"
			"ypos_lodef"				"40"
			"wide"				"275"
		}	
		"DescOne"
		{
			"PropertyString"	"CONTEXT_GAME_TYPE"
			"font"				"MatchmakingDialogMenuLarge"
			"font_lodef"		"MatchmakingDialogMenuMediumSmall"
			"xpos"				"15"
			"ypos"				"100"
			"ypos_lodef"		"80"
			"wide"				"275"
		}
		"DescTwo"
		{
			"labelText"			"#TF_MatchOption_WinLimit"
			"font"				"MatchmakingDialogMenuLarge"
			"font_lodef"		"MatchmakingDialogMenuMediumSmall"
			"xpos"				"15"
			"ypos"				"130"
			"ypos_lodef"		"100"
			"wide"				"275"
		}
		"DescThree"
		{
			"labelText"			"#TF_GameTime"
			"font"				"MatchmakingDialogMenuLarge"
			"font_lodef"		"MatchmakingDialogMenuMediumSmall"
			"xpos"				"15"
			"ypos"				"160"
			"ypos_lodef"		"120"
			"wide"				"275"
		}
		"ValueTwo"
		{
			"PropertyString"	"PROPERTY_WIN_LIMIT"
			"font"				"MatchmakingDialogMenuLarge"
			"font_lodef"		"MatchmakingDialogMenuMediumSmall"
			"xpos"				"75"
			"xpos_lodef"		"143"
			"ypos"				"130"
			"ypos_lodef"		"100"
			"wide"				"275"
			"wide_lodef"		"75"
			"textAlignment"		"east"
		}
		"ValueThree"
		{
			"PropertyString"	"PROPERTY_MAX_GAME_TIME"
			"font"				"MatchmakingDialogMenuLarge"
			"font_lodef"		"MatchmakingDialogMenuMediumSmall"
			"xpos"				"75"
			"xpos_lodef"		"143"
			"ypos"				"160"
			"ypos_lodef"		"120"
			"wide"				"275"
			"wide_lodef"		"75"
			"textAlignment"		"east"
		}
	}
	
	"HostOptions"
	{
		"ControlName"	"EditablePanel"
		"xpos"			"15"
		"ypos"			"360"
		"ypos_lodef"	"304"
		"wide"			"370"
		"wide_lodef"	"233"
		"tall"			"123"		
		"tall_lodef"	"90"
		
		"Buttons"
		{
			"ControlName"	"EditablePanel"
			"xpos"			"0"
			"ypos"			"0"
			"wide"			"370"
			"wide_lodef"	"233"
			"tall"			"123"
			
			"StartGameButton"
			{
				"ControlName"	"Label"
				"labeltext"		"#GameUI_Icons_START"
				"font"			"GameUIButtons"
				"xpos"			"32"
				"xpos_lodef"	"25"
				"ypos"			"7"
				"ypos_lodef"	"0"
				"wide"			"64"
				"tall"			"32"
			}
			
			"ChangeSettingsButton"
			{
				"ControlName"	"Label"
				"labeltext"		"#GameUI_Icons_Y_BUTTON"
				"font"			"GameUIButtons"
				"xpos"			"32"
				"xpos_lodef"	"25"
				"ypos"			"83"
				"ypos_lodef"	"56"
				"wide"			"64"
				"tall"			"32"
			}
			
			"KickButton"
			{
				"ControlName"	"Label"
				"labeltext"		"#GameUI_Icons_L_SHOULDER"
				"font"			"GameUIButtons"
				"xpos"			"15"
				"xpos_lodef"	"12"
				"ypos"			"45"
				"ypos_lodef"	"28"
				"wide"			"64"
				"tall"			"32"
			}
		}
		
		"StartGameText"
		{
			"ControlName"	"Label"
			"labeltext"		"#GameUI_StartGame"
			"font"			"MatchmakingDialogMenuLarge"
			"font_lodef"	"MatchmakingDialogMenuMediumSmall"
			"xpos"			"98"
			"xpos_lodef"	"70"
			"ypos"			"14"
			"ypos_lodef"	"7"
			"wide"			"300"
		}
		
		"CancelGameText"
		{
			"ControlName"	"Label"
			"labeltext"		"#GameUI_StopCountdown"
			"font"			"MatchmakingDialogMenuLarge"
			"font_lodef"	"MatchmakingDialogMenuMediumSmall"
			"xpos"			"98"
			"xpos_lodef"	"70"
			"ypos"			"14"
			"ypos_lodef"	"7"
			"wide"			"300"
			"visible"		"0"
		}
			
		"ChangeSettingsText"
		{
			"ControlName"	"Label"
			"labeltext"		"#GameUI_ChangeGameSettings"
			"font"			"MatchmakingDialogMenuLarge"
			"font_lodef"	"MatchmakingDialogMenuMediumSmall"
			"xpos"			"98"
			"xpos_lodef"	"70"
			"ypos"			"90"
			"ypos_lodef"	"63"
			"wide"			"300"
		}	
				
		"KickText"
		{
			"ControlName"	"Label"
			"labeltext"		"#TF_KickPlayer"
			"font"			"MatchmakingDialogMenuLarge"
			"font_lodef"	"MatchmakingDialogMenuMediumSmall"
			"xpos"			"98"
			"xpos_lodef"	"70"
			"ypos"			"51"
			"ypos_lodef"	"34"
			"wide"			"300"
		}
	}
			
	"BlueTeamDescription"
	{
		"ControlName"	"CScenarioInfoPanel"
		"xpos"			"397"
		"xpos_lodef"	"250"
		"ypos"			"10"
		"ypos_lodef"	"20"
		"wide"			"180"
		"wide_lodef"	"115"
		"tall"			"70"

		"Title"
		{
			"labeltext"		"#TF_ScoreBoard_Blue"
			"font"			"MatchmakingDialogTitle"
			"font_lodef"	"MatchmakingDialogMenuLarge"
			"xpos"			"12"
			"xpos_lodef"	"8"
			"ypos"			"10"
			"ypos_lodef"			"5"
			"wide"			"180"
			"wide_lodef"	"100"
		}

		"Subtitle"
		{
			"labelText"		"#TF_ScoreBoard_Players"
			"font"			"MatchmakingDialogMenuSmallest"
			"xpos"			"87"
			"xpos_lodef"	"8"
			"ypos"			"11"
			"ypos_lodef"	"23"
			"wide"			"85"
			"wide_lodef"	"100"
			"textAlignment_hidef"	"east"
		}
		
		"DescOne"
		{
			"font"			"MatchmakingDialogMenuLarge"
			"font_lodef"			"MatchmakingDialogMenuSmall"
			"xpos"			"12"
			"xpos_lodef"	"8"
			"ypos"			"45"
			"ypos_lodef"	"45"
			"wide"			"150"
			"wide_lodef"	"110"
		}
	}

	"RedTeamDescription"
	{
		"ControlName"	"CScenarioInfoPanel"
		"xpos"			"397"
		"xpos_lodef"	"250"
		"ypos"			"143"
		"ypos_lodef"	"163"
		"wide"			"180"
		"wide_lodef"	"115"
		"tall"			"70"

		"Title"
		{
			"labeltext"		"#TF_ScoreBoard_Red"
			"font"			"MatchmakingDialogTitle"
			"font_lodef"	"MatchmakingDialogMenuLarge"
			"xpos"			"12"
			"xpos_lodef"	"8"
			"ypos"			"10"
			"ypos_lodef"			"5"
			"wide"			"180"
			"wide_lodef"	"100"
		}

		"Subtitle"
		{
			"labelText"		"#TF_ScoreBoard_Players"
			"font"			"MatchmakingDialogMenuSmallest"
			"xpos"			"87"
			"xpos_lodef"	"8"
			"ypos"			"11"
			"ypos_lodef"	"23"
			"wide"			"85"
			"wide_lodef"	"100"
			"textAlignment_hidef"	"east"
		}
		
		"DescOne"
		{
			"font"			"MatchmakingDialogMenuLarge"
			"font_lodef"			"MatchmakingDialogMenuSmall"
			"xpos"			"12"
			"xpos_lodef"	"8"
			"ypos"			"45"
			"ypos_lodef"	"45"
			"wide"			"150"
			"wide_lodef"	"110"
		}
	}

 	"BluePlayers"
 	{
 		"xpos"			"580"
 		"xpos_lodef"	"368"
 		"ypos"			"10"
 		"ypos_lodef"	"20"
 		"tall"			"50"
 		"minwide"		"300"
 		"minwide_lodef"	"208"
	  	"itemspacing"	"1"
	}

 	"RedPlayers"
 	{
 		"xpos"			"580"
 		"xpos_lodef"	"368"
 		"ypos"			"133"
 		"ypos_lodef"	"163"
  		"tall"			"50"
		"minwide"		"300"
		"minwide_lodef"	"208"
	  	"itemspacing"	"1"
	}
}

//------------------------------------
// Client Lobby
//------------------------------------
"SessionLobby_Client.res"
{
	"SessionLobby_Client"
	{
		"xpos_lodef"				"c-294"
		"ypos_lodef"				"15"
		"wide"			"900"
		"wide_lodef"			"588"
		"tall"				"510"
		"tall_lodef"				"405"
		"borderwidth"	"30"
		"teamspacing"	"10"
		"teamspacing_lodef"		"3"
		
		"commandstring"		"StartClient"
		"center_lodef"				"0"
		"footer_buttongap_lodef"	"15"
		
		"Footer"
		{
			"button"
			{
				"text"		"#GameUI_ExitLobby"
				"icon"		"#GameUI_Icons_B_BUTTON"
			}
		
			"button"
			{
				"text"		"#GameUI_ChangeTeam"
				"icon"		"#GameUI_Icons_X_BUTTON"
			}
			
			"button"
			{
				"text"		"#TF_ViewGamercard"
				"icon"		"#GameUI_Icons_A_BUTTON"
			}
			
			"button"
			{
				"text_hidef"		"#GameUI_PlayerReview"
				"icon_hidef"		"#GameUI_Icons_R_SHOULDER"
			}	
		}
	}
	
	"ReviewPlayerButton"
	{
		"ControlName"	"Label"
		"labeltext"		"#GameUI_Icons_R_SHOULDER"
		"font"			"GameUIButtons"
		"xpos"			"362"
		"ypos"			"360"
		"wide"			"64"
		"tall"			"32"
		"zpos"			"3"
		"visible"		"0"
		"visible_lodef"	"1"
	}
	
	"PlayerReviewLabel"
	{
		"ControlName"	"Label"
		"labeltext"		"#GameUI_PlayerReview"
		"font"			"MatchmakingDialogMenuMediumSmall"
		"xpos"			"420"
		"ypos"			"367"
		"wide"			"300"
		"visible"		"0"
		"visible_lodef"	"1"
	}
	
	"DialogTitle"
	{
		"xpos"			"15"
		"xpos_lodef"	"25"
		"ypos"			"15"
		"ypos_lodef"			"10"
		"font"			"MatchmakingDialogTitle"
		"labeltext"		"#TF_Lobby_Title"
	}
	
	"HostLabel"
	{
		"xpos"			"15"
		"xpos_lodef"	"25"
		"ypos"			"45"
		"ypos_lodef"	"50"
		"wide"			"350"
		"wide_lodef"	"250"
		"tall"			"30"
		"tall_hidef"	"55"
		"font"			"MatchmakingDialogMenuLarge"
		"font_lodef"	"MatchmakingDialogMenuSmall"
	}

	"LobbyStateBg"
	{
		"xpos"			"15"
		"ypos"			"120"
		"ypos_lodef"	"100"
		"wide"			"370"
		"wide_lodef"	"233"
		"tall"			"35"
	}
	
	"LobbyStateIcon"
	{
		"ControlName"	"Label"
		"labeltext"		"#TF_Icon_Alert"
		"font"			"GameUIButtons"
		"xpos"			"25"
		"ypos"			"121"
		"ypos_lodef"	"101"
		"wide"			"32"
		"tall"			"32"
	}

	"LobbyStateLabel"
	{
		"xpos"			"68"
		"xpos_lodef"	"53"
		"ypos"			"125"
		"ypos_lodef"	"105"
		"wide"			"350"
		"wide_lodef"	"223"
		"tall"			"30"
		"font"			"MatchmakingDialogMenuLarge"
		"font_lodef"	"MatchmakingDialogMenuMediumSmall"
		"labeltext"		"#TF_waitingForMinPlayers"
	}
	
	"GameScenario"
	{
		"ControlName"	"CScenarioInfoPanel"
		"xpos"			"15"
		"ypos"			"162"
		"ypos_lodef"	"142"
		"wide"			"370"
		"wide_lodef"	"233"
		"tall"			"190"
		"tall_lodef"	"155"
		
		// If there is a "PropertyString" entry, then it is used as a lookup
		// to set the label text from the keyvalues that get passed into the dialog
		"Title"
		{
			"PropertyString"	"CONTEXT_SCENARIO"
			"font"				"MatchmakingDialogTitle"
			"xpos"				"15"
			"ypos"				"15"
			"wide"				"275"
		}
		"Subtitle"
		{
			"PropertyString"	"scenariotype"
			"font"				"MatchmakingDialogMenuLarge"
			"xpos"				"15"
			"ypos"				"45"
			"ypos_lodef"				"40"
			"wide"				"275"
		}	
		"DescOne"
		{
			"PropertyString"	"CONTEXT_GAME_TYPE"
			"font"				"MatchmakingDialogMenuLarge"
			"font_lodef"		"MatchmakingDialogMenuMediumSmall"
			"xpos"				"15"
			"ypos"				"100"
			"ypos_lodef"		"80"
			"wide"				"275"
		}
		"DescTwo"
		{
			"labelText"			"#TF_MatchOption_WinLimit"
			"font"				"MatchmakingDialogMenuLarge"
			"font_lodef"		"MatchmakingDialogMenuMediumSmall"
			"xpos"				"15"
			"ypos"				"130"
			"ypos_lodef"		"100"
			"wide"				"275"
		}
		"DescThree"
		{
			"labelText"			"#TF_GameTime"
			"font"				"MatchmakingDialogMenuLarge"
			"font_lodef"		"MatchmakingDialogMenuMediumSmall"
			"xpos"				"15"
			"ypos"				"160"
			"ypos_lodef"		"120"
			"wide"				"275"
		}
		"ValueTwo"
		{
			"PropertyString"	"PROPERTY_WIN_LIMIT"
			"font"				"MatchmakingDialogMenuLarge"
			"font_lodef"		"MatchmakingDialogMenuMediumSmall"
			"xpos"				"75"
			"xpos_lodef"		"143"
			"ypos"				"130"
			"ypos_lodef"		"100"
			"wide"				"275"
			"wide_lodef"		"75"
			"textAlignment"		"east"
		}
		"ValueThree"
		{
			"PropertyString"	"PROPERTY_MAX_GAME_TIME"
			"font"				"MatchmakingDialogMenuLarge"
			"font_lodef"		"MatchmakingDialogMenuMediumSmall"
			"xpos"				"75"
			"xpos_lodef"		"143"
			"ypos"				"160"
			"ypos_lodef"		"120"
			"wide"				"275"
			"wide_lodef"		"75"
			"textAlignment"		"east"
		}
	}
	
	"HostOptions"
	{
		"ControlName"	"EditablePanel"
		"xpos"			"0"
		"ypos"			"0"
		"wide"			"0"
		"tall"			"0"
		"visible"		"0"
	}
	
	"BlueTeamDescription"
	{
		"ControlName"	"CScenarioInfoPanel"
		"xpos"			"397"
		"xpos_lodef"	"250"
		"ypos"			"10"
		"ypos_lodef"	"20"
		"wide"			"180"
		"wide_lodef"	"115"
		"tall"			"70"

		"Title"
		{
			"labeltext"		"#TF_ScoreBoard_Blue"
			"font"			"MatchmakingDialogTitle"
			"font_lodef"	"MatchmakingDialogMenuLarge"
			"xpos"			"12"
			"xpos_lodef"	"8"
			"ypos"			"10"
			"ypos_lodef"			"5"
			"wide"			"180"
			"wide_lodef"	"100"
		}

		"Subtitle"
		{
			"labelText"		"#TF_ScoreBoard_Players"
			"font"			"MatchmakingDialogMenuSmallest"
			"xpos"			"87"
			"xpos_lodef"	"8"
			"ypos"			"11"
			"ypos_lodef"	"23"
			"wide"			"85"
			"wide_lodef"	"100"
			"textAlignment_hidef"	"east"
		}
		
		"DescOne"
		{
			"font"			"MatchmakingDialogMenuLarge"
			"font_lodef"			"MatchmakingDialogMenuSmall"
			"xpos"			"12"
			"xpos_lodef"	"8"
			"ypos"			"45"
			"ypos_lodef"	"45"
			"wide"			"150"
			"wide_lodef"	"110"
		}
	}

	"RedTeamDescription"
	{
		"ControlName"	"CScenarioInfoPanel"
		"xpos"			"397"
		"xpos_lodef"	"250"
		"ypos"			"143"
		"ypos_lodef"	"163"
		"wide"			"180"
		"wide_lodef"	"115"
		"tall"			"70"

		"Title"
		{
			"labeltext"		"#TF_ScoreBoard_Red"
			"font"			"MatchmakingDialogTitle"
			"font_lodef"	"MatchmakingDialogMenuLarge"
			"xpos"			"12"
			"xpos_lodef"	"8"
			"ypos"			"10"
			"ypos_lodef"			"5"
			"wide"			"180"
			"wide_lodef"	"100"
		}

		"Subtitle"
		{
			"labelText"		"#TF_ScoreBoard_Players"
			"font"			"MatchmakingDialogMenuSmallest"
			"xpos"			"87"
			"xpos_lodef"	"8"
			"ypos"			"11"
			"ypos_lodef"	"23"
			"wide"			"85"
			"wide_lodef"	"100"
			"textAlignment_hidef"	"east"
		}
		
		"DescOne"
		{
			"font"			"MatchmakingDialogMenuLarge"
			"font_lodef"			"MatchmakingDialogMenuSmall"
			"xpos"			"12"
			"xpos_lodef"	"8"
			"ypos"			"45"
			"ypos_lodef"	"45"
			"wide"			"150"
			"wide_lodef"	"110"
		}
	}

 	"BluePlayers"
 	{
 		"xpos"			"580"
 		"xpos_lodef"	"368"
 		"ypos"			"10"
 		"ypos_lodef"	"20"
 		"tall"			"50"
 		"minwide"		"300"
 		"minwide_lodef"	"208"
	  	"itemspacing"	"1"
	}

 	"RedPlayers"
 	{
 		"xpos"			"580"
 		"xpos_lodef"	"368"
 		"ypos"			"133"
 		"ypos_lodef"	"163"
  		"tall"			"50"
		"minwide"		"300"
		"minwide_lodef"	"208"
	  	"itemspacing"	"1"
	}
}

//------------------------------------
// Online Match Session Browser
//------------------------------------
"SessionBrowser_Live.res"
{
	"SessionBrowser_Live"
	{		
		"wide"			"900"
		"wide_lodef"	"580"
		"tall"			"395"
		"tall_lodef"	"320"
		"borderwidth"	"30"
			
		// These should be in the same order as the scenario defines in hl2orange.spa.h
		"ScenarioInfoPanels"
		{
			"ScenarioInfo"
			{
				"title"			"2Fort"
				"subtitle"		"#Gametype_CTF"
				"image"			"maps/menu_screen_ctf_2fort"
				"xpos"			"595"
				"xpos_lodef"	"410"
				"tall"			"330"
				"tall_lodef"	"240"
				"descOneY"		"270"
				"descOneY_lodef"	"155"
				"descTwoY"		"300"
				"descTwoY_lodef"	"200"
			}
			"ScenarioInfo"
			{
				"title"			"Dustbowl"
				"subtitle"		"#TF_AttackDefend"
				"image"			"maps/menu_screen_cp_dustbowl"
				"xpos"			"595"
				"xpos_lodef"	"410"
				"tall"			"330"
				"tall_lodef"	"240"
				"descOneY"		"270"
				"descOneY_lodef"	"155"
				"descTwoY"		"300"
				"descTwoY_lodef"	"200"
			}
			"ScenarioInfo"
			{
				"title"			"Granary"
				"subtitle"		"#TF_AttackDefend"
				"image"			"maps/menu_screen_cp_granary"
				"xpos"			"595"
				"xpos_lodef"	"410"
				"tall"			"330"
				"tall_lodef"	"240"
				"descOneY"		"270"
				"descOneY_lodef"	"155"
				"descTwoY"		"300"
				"descTwoY_lodef"	"200"
			}
			"ScenarioInfo"
			{
				"title"			"Well"
				"subtitle"		"#TF_AttackDefend"
				"image"			"maps/menu_screen_cp_well"
				"xpos"			"595"
				"xpos_lodef"	"410"
				"tall"			"330"
				"tall_lodef"	"240"
				"descOneY"		"270"
				"descOneY_lodef"	"155"
				"descTwoY"		"300"
				"descTwoY_lodef"	"200"
			}
			"ScenarioInfo"
			{
				"title"			"Gravel Pit"
				"subtitle"		"#TF_AttackDefend"
				"image"			"maps/menu_screen_cp_gravelpit"
				"xpos"			"595"
				"xpos_lodef"	"410"
				"tall"			"330"
				"tall_lodef"	"240"
				"descOneY"		"270"
				"descOneY_lodef"	"155"
				"descTwoY"		"300"
				"descTwoY_lodef"	"200"
			}
			"ScenarioInfo"
			{
				"title"			"Hydro"
				"subtitle"		"#TF_AttackDefend"
				"image"			"maps/menu_screen_tc_hydro"
				"xpos"			"595"
				"xpos_lodef"	"410"
				"tall"			"330"
				"tall_lodef"	"240"
				"descOneY"		"270"
				"descOneY_lodef"	"155"
				"descTwoY"		"300"
				"descTwoY_lodef"	"200"
			}
		}
		
		"footer_buttongap_lodef"	"15"
			
		"Footer"
		{
			"button"
			{
				"text"		"#GameUI_Back"
				"icon"		"#GameUI_Icons_B_BUTTON"
			}
		
			"button"
			{
				"text"		"#GameUI_Select"
				"icon"		"#GameUI_Icons_A_BUTTON"
			}
			
			"button"
			{
				"text"		"#GameUI_Sort"
				"icon"		"#GameUI_Icons_Y_BUTTON"
			}			
			
			"button"
			{
				"text"		"#TF_ViewGamercard"
				"icon"		"#GameUI_Icons_X_BUTTON"
			} 
		}
	}
	
	"DialogTitle"
	{
		"xpos"			"15"
		"ypos"			"15"
		"ypos_lodef"			"10"
		"font"			"MatchmakingDialogTitle"
		"labeltext"		"#TF_SystemLink_Client_Dialog"
	}
	
	"DialogMenu"
	{	
		"xpos"				"15"
		"xpos_lodef"				"3"
		"ypos"				"45"
		"ypos_lodef"		"50"
		"minwide"			"570"
		"minwide_lodef"		"405"
 		"itemspacing"		"2"
		"maxvisibleitems"	"7"
		"activecolumn"		"3"

 		"Columns"
 		{
 			"HeaderFont"		"MatchmakingDialogMenuBrowserHostname"
 			"HeaderFgColor"		"MatchmakingDialogTitleColor"
 			
 			"Column"
 			{
 				"header"		"#TF_HostName"
 				"xpos"			"5"
 				"xpos_lodef"			"4"
 				"ypos"			"4"
 				"ypos_lodef"			"1"
 				"wide"			"295"
 				"wide_lodef"			"202"
				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuBrowserHostname"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
 			}
 			"Column"
 			{
				"header"		"#TF_Players"
   				"ypos"			"5"
   				"ypos_lodef"			"1"
				"wide"			"80"
				"wide_lodef"			"66"
  				"align"			"5"	// east
 				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuBrowserHostname"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}
 			"Column"
 			{
				"header"		"#TF_MatchOption_Scenario"
   				"ypos"			"4"
   				"ypos_lodef"			"1"
				"wide"			"145"
				"wide_lodef"			"105"
  				"align"			"5"	// east
 				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuBrowserHostname"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}
 			"Column"
 			{
				"header"		"#TF_Scoreboard_Ping"
				"header_lodef"		" "
  				"ypos"			"0"
  				"ypos_lodef"			"0"
				"wide"			"40"
				"wide_lodef"	"25"
  				"align"			"4"	// center
				"font"			"GameUIButtonsSmall"
				"fgcolor"		"white"
 			}
 		}
	}
}

//------------------------------------
// System Link Session Browser
//------------------------------------
"SessionBrowser_SystemLink.res"
{
	"SessionBrowser_SystemLink"
	{		
		"wide"			"900"
		"wide_lodef"	"580"
		"tall"			"395"
		"tall_lodef"	"320"
		"borderwidth"	"30"
			
		// These should be in the same order as the scenario defines in hl2orange.spa.h
		"ScenarioInfoPanels"
		{
			"ScenarioInfo"
			{
				"title"			"2Fort"
				"subtitle"		"#Gametype_CTF"
				"image"			"maps/menu_screen_ctf_2fort"
				"xpos"			"595"
				"xpos_lodef"	"410"
				"tall"			"330"
				"tall_lodef"	"240"
				"descOneY"		"270"
				"descOneY_lodef"	"155"
				"descTwoY"		"300"
				"descTwoY_lodef"	"200"
			}
			"ScenarioInfo"
			{
				"title"			"Dustbowl"
				"subtitle"		"#TF_AttackDefend"
				"image"			"maps/menu_screen_cp_dustbowl"
				"xpos"			"595"
				"xpos_lodef"	"410"
				"tall"			"330"
				"tall_lodef"	"240"
				"descOneY"		"270"
				"descOneY_lodef"	"155"
				"descTwoY"		"300"
				"descTwoY_lodef"	"200"
			}
			"ScenarioInfo"
			{
				"title"			"Granary"
				"subtitle"		"#TF_AttackDefend"
				"image"			"maps/menu_screen_cp_granary"
				"xpos"			"595"
				"xpos_lodef"	"410"
				"tall"			"330"
				"tall_lodef"	"240"
				"descOneY"		"270"
				"descOneY_lodef"	"155"
				"descTwoY"		"300"
				"descTwoY_lodef"	"200"
			}
			"ScenarioInfo"
			{
				"title"			"Well"
				"subtitle"		"#TF_AttackDefend"
				"image"			"maps/menu_screen_cp_well"
				"xpos"			"595"
				"xpos_lodef"	"410"
				"tall"			"330"
				"tall_lodef"	"240"
				"descOneY"		"270"
				"descOneY_lodef"	"155"
				"descTwoY"		"300"
				"descTwoY_lodef"	"200"
			}
			"ScenarioInfo"
			{
				"title"			"Gravel Pit"
				"subtitle"		"#TF_AttackDefend"
				"image"			"maps/menu_screen_cp_gravelpit"
				"xpos"			"595"
				"xpos_lodef"	"410"
				"tall"			"330"
				"tall_lodef"	"240"
				"descOneY"		"270"
				"descOneY_lodef"	"155"
				"descTwoY"		"300"
				"descTwoY_lodef"	"200"
			}
			"ScenarioInfo"
			{
				"title"			"Hydro"
				"subtitle"		"#TF_AttackDefend"
				"image"			"maps/menu_screen_tc_hydro"
				"xpos"			"595"
				"xpos_lodef"	"410"
				"tall"			"330"
				"tall_lodef"	"240"
				"descOneY"		"270"
				"descOneY_lodef"	"155"
				"descTwoY"		"300"
				"descTwoY_lodef"	"200"
			}
		}
		
		"footer_buttongap_lodef"			"15"
		
		"Footer"
		{
			"button"
			{
				"text"		"#GameUI_Back"
				"icon"		"#GameUI_Icons_B_BUTTON"
			}
		
			"button"
			{
				"text"		"#GameUI_Select"
				"icon"		"#GameUI_Icons_A_BUTTON"
			}
			
			"button"
			{
				"text"		"#GameUI_Sort"
				"icon"		"#GameUI_Icons_Y_BUTTON"
			}			
			
			"button"
			{
				"text"		"#TF_ViewGamercard"
				"icon"		"#GameUI_Icons_X_BUTTON"
			} 
		}
	}
	
	"DialogTitle"
	{
		"xpos"			"15"
		"ypos"			"15"
		"ypos_lodef"			"10"
		"font"			"MatchmakingDialogTitle"
		"labeltext"		"#TF_SystemLink_Client_Dialog"
	}
	
	"DialogMenu"
	{	
		"xpos"				"15"
		"xpos_lodef"				"3"
		"ypos"				"45"
		"ypos_lodef"		"50"
		"minwide"			"570"
		"minwide_lodef"		"405"
 		"itemspacing"		"2"
		"maxvisibleitems"	"7"
		"activecolumn"		"0"

 		"Columns"
 		{
 			"HeaderFont"		"MatchmakingDialogMenuBrowserHostname"
 			"HeaderFgColor"		"MatchmakingDialogTitleColor"
 			
 			"Column"
 			{
 				"header"		"#TF_HostName"
 				"xpos"			"5"
 				"xpos_lodef"			"8"
 				"ypos"			"4"
 				"ypos_lodef"			"1"
 				"wide"			"315"
 				"wide_lodef"			"215"
				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuBrowserHostname"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
 			}
 			"Column"
 			{
				"header"		"#TF_Players"
   				"ypos"			"5"
   				"ypos_lodef"			"1"
				"wide"			"90"
				"wide_lodef"			"70"
  				"align"			"5"	// east
 				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuBrowserHostname"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}
 			"Column"
 			{
				"header"		"#TF_MatchOption_Scenario"
   				"ypos"			"4"
   				"ypos_lodef"			"1"
				"wide"			"155"
				"wide_lodef"			"105"
  				"align"			"5"	// east
 				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuBrowserHostname"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}
 		}
	}
}

//------------------------------------
// Leaderboard Dialog - Ranked
//------------------------------------
"LeaderboardDialog_Ranked.res"
{	
	"LeaderboardDialog_Ranked"
	{
		"wide"			"780"
		"wide_lodef"	"480"
		"tall"			"495"
		"tall_lodef"	"350"
		"borderwidth"	"15"
		"borderwidth_lodef"	"10"
		
		"Footer"
		{
			"button"
			{
				"text"		"#TF_ViewGamercard"
				"icon"		"#GameUI_Icons_A_BUTTON"
			}

			"button"
			{
				"text"		"#TF_Top"
				"icon"		"#GameUI_Icons_L_SHOULDER"
			}	

			"button"
			{
				"text"		"#TF_YourRank"
				"icon"		"#GameUI_Icons_R_SHOULDER"
			}	
		}
	}
	
	"DialogTitle"
	{
		"xpos"			"15"
		"ypos"			"15"
		"ypos_lodef"	"10"
		"font"			"MatchmakingDialogTitle"
		"labeltext"		"#TF_RankedLeaderboards_Title"
	}
	
	"DialogMenu"
	{
		"xpos"				"15"
		"xpos_lodef"		"10"
		"ypos"				"50"
		"ypos_lodef"		"40"
		"minwide"			"750"
		"minwide_lodef"		"565"
 		"itemspacing"		"2"
		"maxvisibleitems"	"10"
		"maxvisibleitems_lodef"	"8"
		"maxvisiblecolumns"	"6"
		"maxvisiblecolumns_lodef" "5"
		"activecolumn"		"-1"	
		
 		"Columns"
 		{
 			"HeaderFont"		"RankingDialogHeaders"
 			"HeaderFgColor"		"MatchmakingDialogTitleColor"
 			
 			 "Column"
 			{
 				"header"		"#TF_Rank"
 				"xpos"			"5"
 				"ypos"			"4"
 				"ypos_lodef"	"0"
 				"wide"			"100"
 				"align"			"4"	// center
				"locked"		"1"
				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
 			}
  			"Column"
 			{
				"header"		"#TF_Gamertag"
   				"ypos"			"5"
   				"ypos_lodef"	"0"
				"wide"			"480"
				"wide_lodef"			"320"
  				"align"			"3"	// west
				"locked"		"1"
 				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}
			"Column"
 			{
				"header"		"#TF_Scoreboard_Score"
   				"ypos"			"5"
   				"ypos_lodef"	"0"
				"wide"			"150"
				"wide_lodef"			"130"
  				"align"			"4"	// center
				"locked"		"0"
 				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}
		}
	}
	
	"ProgressBg"
	{
		"xpos"			"215"
		"ypos"			"420"
		"wide"			"300"
		"tall"			"10"
	}
		
	"Numbering"
	{
		"xpos"			"15"
		"ypos"			"458"
		"ypos_lodef"	"324"
		"ControlName"	"label"
		"font"			"MatchmakingDialogMenuSmall"
	}	
	
	"uparrow"
	{
		"xpos_lodef"	"525"
		"xpos_hidef"	"670"
		"ypos_lodef"	"322"
		"ypos_hidef"	"458"
		"ControlName"	"Label"
		"font"			"GameUIButtons"
		"labeltext"		"#GameUI_Icons_UPCURSOR"
	}

	"downarrow"
	{
		"xpos_lodef"	"547"
		"xpos_hidef"	"700"
		"ypos_lodef"	"322"
		"ypos_hidef"	"458"
		"ControlName"	"Label"
		"font"			"GameUIButtons"
		"labeltext"		"#GameUI_Icons_DOWNCURSOR"
	}
	
	"BestMoments"
	{
		"xpos"			"15"
		"xpos_lodef"	"395"
		"ypos"			"35"
		"wide"			"240"
		"ControlName"	"Label"
		"textAlignment"	"east"
		"font"			"RankingDialogHeaders"
		"labeltext"		"#Leaderboard_BestMoments"
		"visible"		"0"
	}	
	
	"SelectButton"
	{
		"xpos_lodef"	"190"
		"xpos_hidef"	"200"
		"ypos_lodef"	"320"
		"ypos_hidef"	"450"
		"tall_hidef"	"40"
		"zpos"				"7"
		"textAlignment"	"east"
		"ControlName"	"Label"
		"font"			"GameUIButtons"
		"labeltext"		"#GameUI_Icons_DPAD"
	}
	
	"SelectLabel"
	{
		"xpos"				"280"
		"xpos_lodef"		"260"
		"ypos"				"453"
		"ypos_lodef"		"320"
		"zpos"				"7"
		"wide"				"240"
		"tall_hidef"		"40"
		"ControlName"		"Label"
		"textAlignment"		"west"
		"font"				"MatchmakingDialogMenuLarge"
		"font_lodef"		"MatchmakingDialogMenuSmall"
		"labeltext"			"#GameUI_Select"
	}	
	
	"BackButton"
	{
		"xpos_lodef"	"350"
		"xpos_hidef"	"425"
		"ypos_lodef"	"320"
		"ypos_hidef"	"450"
		"zpos"			"8"
		"tall_hidef"	"40"
		"textAlignment"	"east"
		"ControlName"	"Label"
		"font"			"GameUIButtons"
		"labeltext"		"#GameUI_Icons_B_BUTTON"
	}
	
	"BackLabel"
	{
		"xpos"				"505"
		"xpos_lodef"		"420"
		"ypos"				"453"
		"ypos_lodef"		"320"
		"zpos"				"8"
		"wide"				"240"
		"tall_hidef"		"40"
		"ControlName"		"Label"
		"textAlignment"		"west"
		"font"				"MatchmakingDialogMenuLarge"
		"font_lodef"		"MatchmakingDialogMenuSmall"
		"labeltext"			"#GameUI_Back"
	}								
}

//------------------------------------
// Leaderboard Dialog - Stats
//------------------------------------
"LeaderboardDialog_Stats.res"
{	
	"LeaderboardDialog_Stats"
	{
		"wide"			"840"
		"wide_lodef"	"480"
		"tall"			"495"
		"tall_lodef"	"350"
		"borderwidth"	"15"
		"borderwidth_lodef"	"10"
		
		"Footer"
		{
			"button"
			{
				"text"		"#TF_ViewGamercard"
				"icon"		"#GameUI_Icons_A_BUTTON"
			}

			"button"
			{
				"text"		"#TF_Top"
				"icon"		"#GameUI_Icons_L_SHOULDER"
			}	

			"button"
			{
				"text"		"#TF_YourRank"
				"icon"		"#GameUI_Icons_R_SHOULDER"
			}	
		}
	}
	
	"DialogTitle"
	{
		"xpos"			"20"
		"xpos_lodef"	"20"
		"ypos"			"15"
		"ypos_lodef"	"10"
		"font"			"MatchmakingDialogTitle"
		"labeltext"		"#TF_StatsLeaderboards_Title"
	}
	
	"DialogMenu"
	{
		"xpos"				"15"
		"xpos_lodef"		"10"
		"ypos"				"50"
		"minwide"			"865"
		"minwide_lodef"		"585"
 		"itemspacing"		"2"
		"maxvisibleitems"	"9"
		"maxvisibleitems_lodef"	"7"
		"maxvisiblecolumns"	"5"
		"maxvisiblecolumns_lodef" "4"
		"activecolumn"		"2"	
		
 		"Columns"
 		{
 			"HeaderFont"		"RankingDialogHeaders"
 			"HeaderFgColor"		"MatchmakingDialogTitleColor"
 			
 			"Column"
 			{
 				"header"		"#TF_Rank"
 				"xpos"			"5"
 				"ypos"			"4"
 				"ypos_lodef"	"0"
 				"wide"			"80"
 				"align"			"4"	// center
				"locked"		"1"
				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
 			}
  			"Column"
 			{
				"header"		"#TF_Gamertag"
   				"ypos"			"5"
   				"ypos_lodef"	"0"
				"wide"			"325"
				"wide_lodef"	"195"
  				"align"			"3"	// west
				"locked"		"1"
				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}
			"Column"
 			{
				"header"		"#TF_Scoreboard_Score"
   				"ypos"			"5"
   				"ypos_lodef"	"0"
				"wide"			"150"
				"wide_lodef"	"150"
  				"align"			"4"	// center
				"locked"		"0"
				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}
			"Column"
 			{
				"header"		"#TF_ScoreBoard_KillsLabel"
   				"ypos"			"5"
   				"ypos_lodef"	"0"
				"wide"			"150"
				"wide_lodef"	"150"
  				"align"			"4"	// center
				"locked"		"0"
				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}			
			"Column"
 			{
				"header"		"#TF_ScoreBoard_CapturesLabel"
   				"ypos"			"5"
   				"ypos_lodef"	"0"
				"wide"			"150"
				"wide_lodef"	"150"
  				"align"			"4"	// center
				"locked"		"0"
				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}	
			"Column"
 			{
				"header"		"#TF_ScoreBoard_DefensesLabel"
   				"ypos"			"5"
   				"ypos_lodef"	"0"
				"wide"			"150"
				"wide_lodef"	"150"
  				"align"			"4"	// center
				"locked"		"0"
 				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}	
			"Column"
 			{
				"header"		"#TF_ScoreBoard_DominationLabel"
   				"ypos"			"5"
   				"ypos_lodef"	"0"
				"wide"			"150"
				"wide_lodef"	"150"
  				"align"			"4"	// center
				"locked"		"0"
 				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}									
			"Column"
 			{
				"header"		"#TF_ScoreBoard_RevengeLabel"
   				"ypos"			"5"
   				"ypos_lodef"	"0"
				"wide"			"150"
				"wide_lodef"	"150"
  				"align"			"4"	// center
				"locked"		"0"
 				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}									
			"Column"
 			{
				"header"		"#TF_ScoreBoard_DestructionLabel"
   				"ypos"			"5"
   				"ypos_lodef"	"0"
				"wide"			"150"
				"wide_lodef"	"150"
  				"align"			"4"	// center
				"locked"		"0"
 				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}									
			"Column"
 			{
				"header"		"#TF_ScoreBoard_HeadshotsLabel"
   				"ypos"			"5"
   				"ypos_lodef"	"0"
				"wide"			"150"
				"wide_lodef"	"150"
  				"align"			"4"	// center
				"locked"		"0"
 				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}									
			"Column"
 			{
				"header"		"#TF_ScoreBoard_HealingLabel"
   				"ypos"			"5"
   				"ypos_lodef"	"0"
				"wide"			"150"
				"wide_lodef"	"150"
  				"align"			"4"	// center
				"locked"		"0"
 				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}									
			"Column"
 			{
				"header"		"#TF_ScoreBoard_InvulnLabel"
   				"ypos"			"5"
   				"ypos_lodef"	"0"
				"wide"			"150"
				"wide_lodef"	"150"
  				"align"			"4"	// center
				"locked"		"0"
 				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}									
			"Column"
 			{
				"header"		"#TF_ScoreBoard_AssistsLabel"
   				"ypos"			"5"
   				"ypos_lodef"	"0"
				"wide"			"150"
				"wide_lodef"	"150"
  				"align"			"4"	// center
				"locked"		"0"
 				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}						
			"Column"
 			{
				"header"		"#TF_ScoreBoard_BackstabsLabel"
   				"ypos"			"5"
   				"ypos_lodef"	"0"
				"wide"			"150"
				"wide_lodef"	"150"
  				"align"			"4"	// center
				"locked"		"0"
 				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}									
			"Column"
 			{
				"header"		"#StatPanel_Label_HealthLeached"
   				"ypos"			"5"
   				"ypos_lodef"	"0"
				"wide"			"150"
				"wide_lodef"	"150"
  				"align"			"4"	// center
				"locked"		"0"
 				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}									
			"Column"
 			{
				"header"		"#TF_ScoreBoard_SentryKillsLabel"
   				"ypos"			"5"
   				"ypos_lodef"	"0"
				"wide"			"150"
				"wide_lodef"	"150"
  				"align"			"4"	// center
				"locked"		"0"
 				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}									
			"Column"
 			{
				"header"		"#TF_ScoreBoard_TeleportsLabel"
   				"ypos"			"5"
   				"ypos_lodef"	"0"
				"wide"			"150"
				"wide_lodef"	"150"
  				"align"			"4"	// center
				"locked"		"0"
 				"font"			"MatchmakingDialogMenuLarge"
				"font_lodef"	"MatchmakingDialogMenuSmall"
				"fgcolor"		"MatchmakingMenuItemDescriptionColor"
			}									
		}
	}
	
	"ProgressBg"
	{
		"xpos"			"380"
		"xpos_lodef"	"252"
		"ypos"			"420"
		"ypos_lodef"	"308"
		"wide"			"500"
		"wide_lodef"	"340"
		"tall"			"10"
	}
		
	"Numbering"
	{
		"xpos"			"20"
		"xpos_lodef"	"20"
		"ypos"			"450"
		"ypos_lodef"	"322"
		"ControlName"	"label"
		"font"			"MatchmakingDialogMenuSmall"
	}	
	
	"uparrow"
	{
		"xpos_lodef"	"545"
		"xpos_hidef"	"820"
		"ypos_lodef"	"322"
		"ypos_hidef"	"450"
		"ControlName"	"Label"
		"font"			"GameUIButtons"
		"labeltext"		"#GameUI_Icons_UPCURSOR"
	}

	"downarrow"
	{
		"xpos_lodef"	"567"
		"xpos_hidef"	"845"
		"ypos_lodef"	"322"
		"ypos_hidef"	"450"
		"ControlName"	"Label"
		"font"			"GameUIButtons"
		"labeltext"		"#GameUI_Icons_DOWNCURSOR"
	}
	
	"BestMoments"
	{
		"xpos"				"635"
		"xpos_lodef"		"350"
		"ypos"				"30"
		"ypos_lodef"		"35"
		"wide"				"240"
		"tall_hidef"		"40"
		"ControlName"		"Label"
		"textAlignment"		"east"
		"font"				"RankingDialogHeaders"
		"labeltext"			"#Leaderboard_BestMoments"
	}
	
	"SelectButton"
	{
		"xpos_lodef"	"190"
		"xpos_hidef"	"320"
		"ypos_lodef"	"322"
		"ypos_hidef"	"440"
		"tall_hidef"	"40"
		"zpos"			"7"
		"textAlignment"	"east"
		"ControlName"	"Label"
		"font"			"GameUIButtons"
		"labeltext"		"#GameUI_Icons_DPAD"
	}
	
	"SelectLabel"
	{
		"xpos"				"400"
		"xpos_lodef"		"260"
		"ypos"				"445"
		"ypos_lodef"		"322"
		"tall_hidef"		"40"
		"zpos"				"7"
		"wide"				"240"
		"ControlName"		"Label"
		"textAlignment"		"west"
		"font"				"MatchmakingDialogMenuLarge"
		"font_lodef"		"MatchmakingDialogMenuSmall"
		"labeltext"			"#GameUI_Select"
	}	
	
	"BackButton"
	{
		"xpos_lodef"	"350"
		"xpos_hidef"	"570"
		"ypos_lodef"	"322"
		"ypos_hidef"	"440"
		"tall_hidef"	"40"
		"zpos"			"8"
		"textAlignment"	"east"
		"ControlName"	"Label"
		"font"			"GameUIButtons"
		"labeltext"		"#GameUI_Icons_B_BUTTON"
	}
	
	"BackLabel"
	{
		"xpos"				"650"
		"xpos_lodef"		"420"
		"ypos"				"445"
		"ypos_lodef"		"322"
		"tall_hidef"		"40"
		"zpos"				"8"
		"wide"				"240"
		"ControlName"		"Label"
		"textAlignment"		"west"
		"font"				"MatchmakingDialogMenuLarge"
		"font_lodef"		"MatchmakingDialogMenuSmall"
		"labeltext"			"#GameUI_Back"
	}									
}

"PropertyDisplayKeys"
{
	// Property defined values from hl2orange.spa.h
	"scenario"	"1"
		
	"MapDiskNames"
	{
		"2Fort"			"ctf_2fort"
		"Dustbowl"		"cp_dustbowl"
		"Granary"		"cp_granary"
		"Well"			"cp_well"
		"Gravel Pit"	"cp_gravelpit"
		"Hydro"			"tc_hydro"
// 		"Cloak (CTF)"	"cloak"
// 		"Cloak (CP)"	"cp_cloak"
	}
	
	"ScenarioTypes"
	{
		"ctf_2fort"		"#Gametype_CTF"
		"cp_dustbowl"	"#TF_AttackDefend"
		"cp_granary"	"#Gametype_CP"
		"cp_well"		"#Gametype_CP"
		"cp_gravelpit"	"#TF_AttackDefend"
		"tc_hydro"		"#TF_TerritoryControl"
// 		"cloak"			"#Gametype_CTF"
// 		"cp_cloak"		"#Gametype_CP"
	}

	"TeamGoals"
	{
		"Blue"
		{
			"ctf_2fort"		""
			"cp_dustbowl"	"#TF_Attacking"
			"cp_granary"	""
			"cp_well"		""
			"cp_gravelpit"	"#TF_Attacking"
			"tc_hydro"		""
// 			"cloak"			"#TF_Attacking"
// 			"cp_cloak"		"#TF_Attacking"
		}
		
		"Red"
		{
			"ctf_2fort"		""
			"cp_dustbowl"	"#TF_Defending"
			"cp_granary"	""
			"cp_well"		""
			"cp_gravelpit"	"#TF_Defending"
			"tc_hydro"		""
// 			"cloak"			"#TF_Attacking"
// 			"cp_cloak"		"#TF_Attacking"
		}
	}
}

//------------------------------------
// New Game Dialog
//------------------------------------
"NewGameDialog.res"
{
	"NewGameDialog"
	{
		"ControlName"			"CNewGameDialog"
		"fieldName"				"NewGameDialog"
		"xpos"					"180"
		"ypos"					"280"
		"wide"					"520"
		"wide_hidef"			"730"
		"tall"					"181"
		"tall_hidef"			"255"
		"chapterypos"			"48"
		"chapterypos_hidef"		"68"
		"scrollslow"			"0.2"
		"scrollfast"			"0.1"
		"centerbgtall"			"120"
		"centerbgtall_hidef"	"170"
		"autoResize"			"0"
		"pinCorner"				"0"
		"visible"				"1"
		"enabled"				"1"
		"tabPosition"			"0"
		"settitlebarvisible"	"0"
		"title"					"#GameUI_NewGame"
		"paintbackground"		"1"
	}
	"LeftArrow"
	{
		"ControlName" 		"Label"
		"fieldName"			"LeftArrow"
		"xpos" 				"15"
		"ypos"				"110"
		"ypos_hidef"		"155"
		"wide"				"32"
		"tall"				"32"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"1"
		"enabled"			"1"
		"labelText"			"#GameUI_Icons_LEFTCURSOR"
		"textAlignment"		"center"
		"dulltext"			"0"
		"brighttext"		"0"
		"wrap"				"0"
		"font"				"GameUIButtons"
	}
	"RightArrow"
	{
		"ControlName" 		"Label"
		"fieldName"			"RightArrow"
		"xpos" 				"484"
		"xpos_hidef"		"683"
		"ypos"				"110"
		"ypos_hidef"		"155"
		"wide"				"32"
		"tall"				"32"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"1"
		"enabled"			"1"
		"labelText"			"#GameUI_Icons_RIGHTCURSOR"
		"textAlignment"		"center"
		"dulltext"			"0"
		"brighttext"		"0"
		"wrap"				"0"
		"font"				"GameUIButtons"
	}
	"UpArrow"
	{
		"ControlName" 		"Label"
		"fieldName"			"UpArrow"
		"xpos" 				"245"
		"xpos_hidef"		"350"
		"ypos"				"88"
		"ypos_hidef"		"130"
		"zpos"				"90"
		"wide"				"32"
		"tall"				"32"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"0"
		"enabled"			"1"
		"labelText"			"#GameUI_Icons_UPCURSOR"
		"textAlignment"		"center"
		"dulltext"			"0"
		"brighttext"		"0"
		"wrap"				"0"
		"font"				"GameUIButtons"
	}
	"DownArrow"
	{
		"ControlName" 		"Label"
		"fieldName"			"DownArrow"
		"xpos" 				"245"
		"xpos_hidef"		"350"
		"ypos"				"123"
		"ypos_hidef"		"178"
		"zpos"				"100"
		"wide"				"32"
		"tall"				"32"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"0"
		"enabled"			"1"
		"labelText"			"#GameUI_Icons_DOWNCURSOR"
		"textAlignment"		"center"
		"dulltext"			"0"
		"brighttext"		"0"
		"wrap"				"0"
		"font"				"GameUIButtons"
	}
	"BonusSelectionBorder"
	{
		"ControlName"		"ImagePanel"
		"fieldName"			"BonusSelectionBorder"
		"xpos"				"208"
		"xpos_hidef"		"294"
		"ypos"				"112"
		"ypos_hidef"		"158"
		"zpos"				"75"
		"wide"				"100"
		"wide_hidef"		"142"
		"tall"				"17"
		"tall_hidef"		"24"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"0"
		"enabled"			"1"
		"fillcolor"			"0 0 0 128"
	}
	"BonusSelectionLabel"
	{
		"ControlName"	"Label"
		"fieldName"		"BonusSelectionLabel"
		"xpos"			"0"
		"ypos"			"108"
		"ypos_hidef"	"152"
		"zpos"			"100"
		"wide"			"520"
		"wide_hidef"	"730"
		"tall"			"24"
		"tall_hidef"	"35"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"wrap"			"0"
		"font"			"DefaultLarge"
		"font_hidef"	"MenuLarge"
	}
	"ChallengeEarnedMedal"
	{
		"ControlName"	"CBitmapImagePanel"
		"fieldName"		"ChallengeEarnedMedal"
		"xpos"			"12"
		"xpos_hidef"	"18"
		"ypos"			"43"
		"ypos_hidef"	"50"
		"zpos"			"200"
		"wide"			"45"
		"wide_hidef"	"64"
		"tall"			"45"
		"tall_hidef"	"64"
		"visible"		"0"
		"image"			"medals/medal_00_none"
	}
	"ChallengeBestLabel"
	{
		"ControlName"		"Label"
		"fieldName"			"ChallengeBestLabel"
		"xpos"				"12"
		"xpos_hidef"		"18"
		"ypos"				"87"
		"ypos_hidef"		"116"
		"zpos"				"200"
		"wide"				"45"
		"wide_hidef"		"64"
		"tall"				"30"
		"visible"			"0"
		"enabled"			"1"
		"tabPosition"		"0"
		"textAlignment"		"north"
		"wrap"				"1"
	}
	"ChallengeNextMedal"
	{
		"ControlName"	"CBitmapImagePanel"
		"fieldName"		"ChallengeNextMedal"
		"xpos"			"463"
		"xpos"			"648"
		"ypos"			"43"
		"ypos_hidef"	"50"
		"zpos"			"90"
		"wide"			"45"
		"wide_hidef"	"64"
		"tall"			"45"
		"tall_hidef"	"64"
		"visible"		"0"
		"image"			"medals/medal_00_none"
	}
	"ChallengeNextLabel"
	{
		"ControlName"		"Label"
		"fieldName"			"ChallengeNextLabel"
		"xpos"				"463"
		"xpos_hidef"		"648"
		"ypos"				"87"
		"ypos_hidef"		"116"
		"zpos"				"99"
		"wide"				"45"
		"wide_hidef"		"64"
		"tall"				"30"
		"visible"			"0"
		"enabled"			"1"
		"tabPosition"		"0"
		"textAlignment"		"north"
		"wrap"				"1"
	}

	"ChapterTitleLabel"
	{
		"ControlName"	"Label"
		"fieldName"		"ChapterTitleLabel"
		"xpos"			"0"
		"ypos"			"0"
		"wide"			"518"
		"wide_hidef"	"730"
		"tall"			"40"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"wrap"			"0"
		"font"			"ChapterTitle"
	}
	"ChapterTitleLabel2"
	{
		"ControlName"	"Label"
		"fieldName"		"ChapterTitleLabel2"
		"xpos"			"0"
		"ypos"			"0"
		"wide"			"518"
		"wide_hidef"	"730"
		"tall"			"40"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"wrap"			"0"
		"font"			"ChapterTitle"
	}
	"ControllerMap"
	{
		"ControlName"	"CControllerMap"
		"fieldName"		"ControllerMap"

		"button"
		{
			"name"		"KEY_XBUTTON_A"
			"command"	"play"
			"text"		"#GameUI_StartNewGame"
			"icon"		"#GameUI_Icons_UIBUTTONA"
		}
		"button"
		{
			"name"		"KEY_XBUTTON_B"
			"command"	"close"
			"text"		"#GameUI_Close"
			"icon"		"#GameUI_Icons_UIBUTTONB"
		}
		"button"
		{
			"name"		"KEY_XSTICK1_RIGHT"
			"command"	"next"
		}
		"button"
		{
			"name"		"KEY_XBUTTON_RIGHT"
			"command"	"next"
		}
		"button"
		{
			"name"		"KEY_XSTICK1_LEFT"
			"command"	"prev"
		}
		"button"
		{
			"name"		"KEY_XBUTTON_LEFT"
			"command"	"prev"
		}
		"button"
		{
			"name"		"KEY_XSTICK1_DOWN"
			"command"	"mode_next"
		}
		"button"
		{
			"name"		"KEY_XBUTTON_DOWN"
			"command"	"mode_next"
		}
		"button"
		{
			"name"		"KEY_XSTICK1_UP"
			"command"	"mode_prev"
		}
		"button"
		{
			"name"		"KEY_XBUTTON_UP"
			"command"	"mode_prev"
		}
	}
}

"NewGameChapterPanel.res"
{
	"LevelPicBorder"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"LevelPicBorder"
		"xpos"			"0"
		"ypos"			"34"
		"ypos_hidef"	"48"
		"wide"			"122"
		"wide_hidef"	"172"
		"tall"			"75"
		"tall_hidef"	"106"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
	}
	"LevelPic"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"LevelPic"
		"xpos"			"7"	
		"xpos_hidef"	"10"
		"ypos"			"41"
		"ypos_hidef"	"58"
		"wide"			"107"
		"wide_hidef"	"152"
		"tall"			"61"
		"tall_hidef"	"86"
		"autoResize"	"0"
		"scaleImage"		"1"
		"scaleImage_hidef"	"0"
		"scaleAmount"		"0.72"
		"scaleAmount"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"fillcolor"		"0 0 0 0"
	}
	"ChallengeNextMedal"
	{
		"ControlName"	"CBitmapImagePanel"
		"fieldName"		"ChallengeNextMedal"
		"xpos"			"463"
		"xpos"			"648"
		"ypos"			"43"
		"ypos_hidef"	"50"
		"zpos"			"90"
		"wide"			"45"
		"wide_hidef"	"64"
		"tall"			"45"
		"tall_hidef"	"64"
		"visible"		"0"
		"image"			"medals/medal_00_none"
	}
	"ChapterLabel"
	{
		"ControlName"	"Label"
		"fieldName"		"ChapterLabel"
		"xpos"			"0"
		"ypos"			"14"
		"ypos_hidef"	"20"
		"wide"			"107"
		"wide_hidef"	"152"
		"tall"			"14"
		"tall_hidef"	"20"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"wrap"			"0"
		"font"			"DefaultLarge"
		"font_hidef"	"UiBold"
	}
	
	// not used in xbox NewGame Dialog
	"ChapterNameLabel"
	{
		"ControlName"	"Label"
		"fieldName"		"ChapterNameLabel"
		"xpos"			"0"
		"ypos"			"20"
		"wide"			"152"
		"tall"			"20"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"wrap"			"0"
		"font"			"UiBold"
	}
	
	"HasBonusLabel"
	{
		"ControlName"		"Label"
		"fieldName"		"HasBonusLabel"
		"xpos"			"0"
		"ypos"			"0"
		"wide"			"107"
		"wide_hidef"	"152"
		"tall"			"14"
		"tall_hidef"	"20"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"		"0"
		"textAlignment"		"east"
		"dulltext"		"0"
		"brighttext"		"0"
		"wrap"		"0"
		"font"			"DefaultLarge"
		"font_hidef"	"UiBold"
		"labelText"		"#GameUI_BonusMapsUnlocked"
	}

	
	"CommentaryIcon" 
	{
		"ControlName"		"ImagePanel"
		"fieldName"			"CommentaryIcon"
		"xpos"				"3"
		"xpos_hidef"		"10"
		"ypos"				"72"
		"ypos_hidef"		"120"
		"wide"				"43"
		"tall"				"42"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"0"
		"enabled"			"1"
		"tabPosition"		"0"
		"image"				"hud/icon_commentary_small"
		"scaleImage"		"0"
	}

}

"LoadingDialogNoBanner.res"
{
	"LoadingDialog"
	{
		"ControlName"		"Frame"
		"fieldName"			"LoadingDialog"
		"xpos"			"0"
		"ypos"			"0"

		"wide"				"200"
		"wide_hidef"		"250"
		"tall"				"35"
		"tall_hidef"		"40"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"1"
		"enabled"			"1"
		"tabPosition"		"0"
	}

	"LoadingDialogBG"
	{
		"ControlName"		"Frame"
		"fieldName"			"LoadingDialogBG"
		"xpos"			"0"
		"ypos"			"0"

		"wide"				"200"
		"wide_hidef"		"250"
		"tall"				"20"
		"tall_hidef"		"20"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"1"
		"enabled"			"1"
		"tabPosition"		"0"
		"paintbackground"	"1"
		"bgcolor"		"46 43 42 255"
	}
	
	"LoadingLabel"
	{
		"ControlName"		"Label"
		"fieldName"			"LoadingLabel"
		"labeltext"			"#GameUI_Loading"
		"xpos"				"2"
		"ypos"				"0"
		"wide"				"200"
		"wide_hidef"		"250"
		"tall"				"20"
		"autoresize"		"0"
		"visible"			"1"
		"enabled"			"1"
		"textAlignment"		"west"
	}
	
	// unused, just used to anchor layout params
	"Progress"
	{
		"ControlName"		"ProgressBar"
		"fieldName"			"Progress"
		"visible"			"0"
		"enabled"			"0"
		"autoresize"		"0"

		"wide"				"200"
		"wide_hidef"		"250"
		"tall"				"16"
	}
}

} // end ConsoleDialogs.res
