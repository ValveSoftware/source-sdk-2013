"GameMenu" [$WIN32]
{
	"VRModeButton"
	{
		"label" "#MMenu_VRMode_Activate"
		"command" "engine vr_toggle"
		"subimage" "glyph_vr"
		"OnlyWhenVREnabled" "1"
	}

	// These buttons are only shown while in-game
	// and also are positioned by the .res file
	"CallVoteButton"
	{
		"label"			""
		"command"		"callvote"
		"OnlyInGame"	"1"
		"subimage" "icon_checkbox"
		"tooltip" "#MMenu_CallVote"
	}
	"MutePlayersButton"
	{
		"label"			""
		"command"		"OpenMutePlayerDialog"
		"OnlyInGame"	"1"
		"subimage" "glyph_muted"
		"tooltip" "#MMenu_MutePlayers"
	}
	"RequestCoachButton"
	{
		"label"			""
		"command"		"engine cl_coach_find_coach"
		"OnlyInGame"	"1"
		"subimage" "icon_whistle"
		"tooltip" "#MMenu_RequestCoach"
	}
	"ReportPlayerButton"
	{
		"label"			""
		"command"		"OpenReportPlayerDialog"
		"OnlyInGame"	"1"
		"subimage"		"glyph_alert"
		"tooltip"		"#MMenu_ReportPlayer"
	}
	
	
			//hahahahahaha all of these labels have to start with a space to make space for the icons 
	
	"ServerBrowserButton"
	{
		"label" 		"Play" 
		"command" 		"OpenServerBrowser"
		"subimage" 		""
	}
	
	"HostAServerButton"
	{
		"label" 		"Host" 
		"command" 		"OpenCreateMultiplayerGameDialog"
		"subimage" 		""
	}
	
	"ReplayButton"
	{
		"label"			"Watch"
		"command"		"engine replay_reloadbrowser"
		"subimage"		""
	}
	
	"EquipButton"
	{
		"label" 		"Equip" 
		"command" 		"engine open_charinfo_direct"
		"subimage" 		""
	}
	
	"LearnButton"
	{
		"label" 		"Learn" 
		"command" 		"#TF_Training_SelectMode"
		"subimage" 		""
	}
	
	"SettingsButton"
	{
		"label" 		"A"
		"command" 		"OpenOptionsDialog"
		"subimage" 		""
		"tooltip"		"Settings"
	}
	
	"SettingsAdvancedButton"
	{
		"label" 		"B"
		"command" 		"opentf2options"
		"subimage" 		""
		"tooltip"		"Advanced Settings"
	}
	
	
	"WebsiteButton"
	{
		"label" 		"D"
		"command" 		"open_website"
		"subimage" 		""
		"tooltip"		"passtime.tf"
	}
	
	//only shows in-game
	"DisconnectButton"
	{
		"label" 		" Disconnect"
		"command" 		"engine disconnect"
		"OnlyInGame"	"1"
		"subimage" 		""
	}
	
	//only shows on main menu
	"QuitButton"
	{
		"label" 		" Quit"
		"command" 		"engine replay_confirmquit"
		"OnlyAtMenu" 	"1"
		"subimage" 		""
	}
}