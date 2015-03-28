//----------------------------------------------------------------------------------------
// Master control settings file for all Xbox 360 dialogs. File is loaded once and held by
// BasePanel, then used by the various dialogs as they're created. This prevents the
// expensive DVD access every time a new UI dialog is opened.
//----------------------------------------------------------------------------------------

"resource/XboxDialogs.res"
{

//--------------------------------------
// Save Game Dialog
//--------------------------------------
"SaveGameDialog.res"
{	
	"SaveGameDialog"
	{
		"ControlName"			"CSaveGameDialogXBox"
		"fieldName"				"SaveGameDialog"
		"xpos"					"180"
		"ypos"					"280"
		"wide"					"482"
		"wide_hidef"			"680"
		"tall"					"200"
		"tall_hidef"			"270"
		"chapterypos"			"48"
		"chapterypos_hidef"		"62"
		"scrollslow"			"0.2"
		"scrollfast"			"0.1"
		"centerbgtall"			"127"
		"centerbgtall_hidef"	"180"
		"autoResize"			"0"
		"pinCorner"				"0"
		"visible"				"1"
		"enabled"				"1"
		"tabPosition"			"0"
		"settitlebarvisible"	"0"
		"title"					"#GameUI_SaveGame"
		"paintbackground"		"2"
	}
	
	"TitleLabel"
	{
		"ControlName"	"Label"
		"fieldName"		"TitleLabel"
		"font"			"MenuLarge"
		"xpos"			"0"	
		"ypos"			"0"
		"zpos"			"2"
		"wide"			"482"
		"wide_hidef"	"680"
		"tall"			"35"
		"visible"		"1"
		"enabled"		"1"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"labelText"		"#GameUI_SaveGame"
	}
	
	"NoSavesLabel"
	{
		"ControlName"	"Label"
		"fieldName"		"NoSavesLabel"
		"font"			"MenuLarge"
		"xpos"			"0"	
		"ypos"			"85"
		"ypos_hidef"	"120"
		"zpos"			"2"
		"wide"			"482"
		"wide_hidef"	"680"
		"tall"			"35"
		"visible"		"0"
		"enabled"		"1"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"visible"		"0"
		"labelText"		"#GameUI_NoSaveGamesToDisplay"
	}

	"LeftArrow"
	{
		"ControlName" 		"Label"
		"fieldName"			"LeftArrow"
		"xpos" 				"12"
		"xpos_hidef"		"24"
		"ypos"				"116"
		"ypos_hidef"		"160"
		"wide"				"32"
		"tall"				"32"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"0"
		"enabled"			"1"
		"labelText"			"#GameUI_Icons_LEFTCURSOR"
		"textAlignment"		"center"
		"dulltext"			"0"
		"brighttext"		"0"
		"wrap"				"0"
		"font"				"GameUIButtons"
		"alpha"				"64"
	}
	"RightArrow"
	{
		"ControlName" 		"Label"
		"fieldName"			"RightArrow"
		"xpos" 				"441"
		"xpos_hidef"		"622"
		"ypos"				"113"
		"ypos_hidef"		"160"
		"wide"				"32"
		"tall"				"32"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"0"
		"enabled"			"1"
		"labelText"			"#GameUI_Icons_RIGHTCURSOR"
		"textAlignment"		"center"
		"dulltext"			"0"
		"brighttext"		"0"
		"wrap"				"0"
		"font"				"GameUIButtons"
		"alpha"				"64"
	}
}

"SaveGamePanel.res"
{
	"LevelPicBorder"
	{
		"ControlName"		"ImagePanel"
		"fieldName"		"LevelPicBorder"
		"xpos"		"0"
		"ypos"			"45"
		"ypos_hidef"	"64"
		"wide"			"119"
		"wide_hidef"	"168"
		"tall"			"69"
		"tall_hidef"	"98"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
	}
	"LevelPic"
	{
		"ControlName"		"ImagePanel"
		"fieldName"		"LevelPic"
		"xpos"			"5"	
		"xpos_hidef"	"8"
		"ypos"			"49"
		"ypos_hidef"	"70"
		"wide"			"107"
		"wide_hidef"	"152"
		"tall"			"61"
		"tall_hidef"	"86"
		"autoResize"	"0"
		"scaleImage"		"1"
		"scaleImage_hidef"	"0"
		"scaleAmount"		"0.72"
		"scaleAmount_hidef"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"fillcolor"		"255 255 255 255"
	}
	"ChapterLabel"
	{
		"ControlName"		"Label"
		"fieldName"		"ChapterLabel"
		"xpos"		"0"
		"ypos"			"0"
		"wide"			"119"
		"wide_hidef"	"168"
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
		"font"			"Default"
		"font_hidef"	"UiBold"
	}
	"TimeLabel"
	{
		"ControlName"		"Label"
		"fieldName"		"TimeLabel"
		"xpos"		"0"
		"ypos"			"15"
		"ypos_hidef"	"22"
		"wide"			"119"
		"wide_hidef"	"168"
		"tall"			"14"
		"tall_hidef"	"20"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"textAlignment"		"center"
		"dulltext"		"0"
		"brighttext"		"0"
		"wrap"		"0"
		"font"			"Default"
		"font_hidef"	"Ui"
	}
	"ElapsedLabel"
	{
		"ControlName"		"Label"
		"fieldName"		"ElapsedLabel"
		"xpos"		"0"
		"ypos"			"28"
		"ypos_hidef"	"40"
		"wide"			"119"
		"wide_hidef"	"168"
		"tall"			"14"
		"tall_hidef"	"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"textAlignment"		"center"
		"dulltext"		"0"
		"brighttext"		"0"
		"wrap"		"0"
		"font"			"Default"
		"font_hidef"	"Ui"
	}
	"TypeLabel"
	{
		"ControlName"		"Label"
		"fieldName"		"TypeLabel"
		"xpos"		"0"
		"ypos"		"116"
		"ypos_hidef"	"164"
		"wide"			"122"
		"wide_hidef"	"172"
		"tall"			"14"
		"tall_hidef"	"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"textAlignment"		"center"
		"dulltext"		"0"
		"brighttext"		"0"
		"wrap"		"0"
		"font"			"Default"
		"font_hidef"	"Ui"
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
		"ypos"			"25"
		"ypos_lodef"	"22"
		"font"			"MatchmakingDialogMenuSmall"
	}
}

//------------------------------------
// Achievement Item
//------------------------------------
"AchievementItem.res"
{	
	"bottommargin"	"4"
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
		"font_hidef"	"AchievementTitleFont"
		"font_lodef"	"AchievementTitleFont"
	}
	
	"menuitemdesc"
	{
		"ControlName"	"label"
		"xpos"			"45"
		"ypos"			"25"
		"tall"			"64"
		"wide"			"460"
		"font"			"AchievementDescriptionFont"
	}

	"points"
	{
		"ControlName"	"label"
		"ypos"			"13"
		"ypos_lodef"	"10"
		"font"			"AchievementTitleFont"
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
		"visible"		"1"
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
		"zpos"	"30"
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

	"HelpText" //Help text 
	{
		"ControlName"		"Label"
		"fieldName"		"HelpText"
		"xpos_hidef"	"300"
		"xpos_lodef"	"270"
		"ypos"		"7"
		"zpos"	"1"
		"wide"		"200"
		"wide_lodef"		"160"
		"tall"		"15"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		""
		"textAlignment"		"left"
		"dulltext"		"0"
		"brighttext"		"0"
		"wrap"		"0"
		"font"			"UiBold"				[$WIN32]
		"font"			"HudSelectionText"	[$X360]
	}
}

//------------------------------------
// Achievements Dialog
//------------------------------------
"AchievementsDialog.res"
{	
	"AchievementsDialog"
	{
		"xpos_lodef"	"c-290"
		"ypos_lodef"	"45"
		"borderwidth"	"15"
		"wide_lodef"	"580"
		"tall_hidef"	"535"
		"tall_lodef"	"368"
		"gametotal"		"100"
		"center_lodef"	"0"
		
		"Footer"
		{
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
		"font"			"AchievementTitleFont"
		"labeltext"		"#GameUI_Achievements_Title"
	}
	
	"ProgressBg"
	{
		"xpos_hidef"	"415"
		"xpos_lodef"	"215"
		"ypos"			"50"
		"ypos_lodef"	"40"
		"wide"			"300"
		"tall"			"10"
	}
	
	"ProgressPercent"
	{
		"ControlName"	"label"
		"font"			"AchievementDescriptionFont"
		"textAlignment"		"east"
	}
	
	"Numbering"
	{
		"ControlName"	"label"
		"font"			"AchievementDescriptionFont"
	}

	"uparrow"
	{
		"xpos_lodef"	"510"
		"xpos_hidef"	"660"
		"ypos_lodef"	"374"
		"ypos_hidef"	"455"
		"ControlName"	"Label"
		"font"			"GameUIButtons"
		"labeltext"		"#GameUI_Icons_UPCURSOR"
	}

	"downarrow"
	{
		"xpos_lodef"	"532"
		"xpos_hidef"	"682"
		"ypos_lodef"	"374"
		"ypos_hidef"	"455"
		"ControlName"	"Label"
		"font"			"GameUIButtons"
		"labeltext"		"#GameUI_Icons_DOWNCURSOR"
	}
	
	"DialogMenu"
	{
		"xpos"				"15"
		"ypos"				"90"
		"ypos_lodef"		"75"
 		"itemspacing"		"2"
		"minwide_hidef"		"700"
		"minwide_lodef"		"550"
		"maxvisibleitems_hidef"	"7"
		"maxvisibleitems_lodef"	"4"

		// Menu items are returned by a system query and created at runtime
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
		"wide"				"363"
		"wide_hidef"		"512"
		"tall"				"180"
		"tall_hidef"		"220"
		"titlecolor"		"255 255 255 255"
		"messagecolor"		"255 255 255 255"
		"buttontextcolor"	"255 255 255 255"
		"button_margin"		"15"
		"button_margin_lodef"		"15"
		"activity_indent"	"50"
		"activity_indent_hidef"	"75"
	}
	
	"Background"
	{	
		"xpos"			"0"
		"ypos"			"0"
 		"wide"			"363"
 		"wide_hidef"	"512"
 		"tall"			"145"
 		"tall_hidef"	"205"
		"image"			"common/message_dialog"
		"scaleimage"	"1"
		"visible"		"1"
	}

 	"WarningBackground"
 	{	
 		"xpos"			"0"
 		"ypos"			"0"
 		"wide"			"363"
 		"wide_hidef"	"512"
 		"tall"			"145"
 		"tall_hidef"	"205"
 		"image"			"common/message_dialog_warning"
 		"scaleimage"	"1"
 		"visible"		"1"
 	}
 
 	"ErrorBackground"
 	{	
 		"xpos"			"0"
 		"ypos"			"0"
 		"wide"			"363"
 		"wide_hidef"	"512"
 		"tall"			"145"
 		"tall_hidef"	"205"
 		"image"			"common/message_dialog_error"
 		"scaleimage"	"1"
 		"visible"		"1"
 	}

	"TitleLabel"
	{	
		"ControlName"	"Label"
		"fieldName"		"TitleLabel"
		"font"			"MenuLarge"
		"xpos"			"14"
		"xpos_hidef"	"20"
		"ypos"			"0"
		"zpos"			"2"
		"wide"			"355"
		"wide_hidef"	"500"
		"tall"			"35"
		"visible"		"1"
		"enabled"		"1"
		"textAlignment"	"Left"
		"dulltext"		"0"
		"brighttext"	"0"
	}

	"MessageLabel"
	{	
		"ControlName"	"Label"
		"fieldName"		"MessageLabel"
		"font"			"MenuLarge"
		"xpos"			"20"	
		"ypos"			"40"
		"ypos_hidef"	"40"
		"zpos"			"2"
		"wide"			"340"
		"wide_hidef"	"460"
		"tall"			"95"
		"tall_hidef"	"120"
		"wrap"			"1"
		"visible"		"1"
		"textAlignment"	"center"
		"textcolor"		"0 0 0 255"
		"dulltext"		"0"
		"brighttext"	"0"
	}

	"AnimatingPanel"
	{
		"ControlName"	"AnimatingImagePanel"
		"fieldName"		"AnimatingPanel"
		"xpos"			"25"
		"xpos_lodef"			"20"
		"ypos"			"80"
		"ypos_lodef"			"60"
		"zpos"			"9"
		"wide"			"40"
		"tall"			"40"
		"scaleImage"	"1"
		"image"			"ico_box"
		"frames"		"14"
		"anim_framerate"	"30"
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
		"xpos_hidef"	"648"
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
		"scaleAmount_hidef"	"0"
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
		"xpos"				"0"
		"ypos"				"0"
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
