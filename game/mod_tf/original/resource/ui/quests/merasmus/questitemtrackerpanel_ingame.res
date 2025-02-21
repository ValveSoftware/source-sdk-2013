#base "..\QuestItemTrackerPanel_InGame_Base.res"

"Resource/UI/HudAchievementTrackerItem.res"
{	
	"ItemTrackerPanel"
	{
		"progress_bar_standard_loc_token"	"#QuestPoints_Standard_Merasmus"
		"progress_bar_advanced_loc_token"	"#QuestPoints_Bonus_Merasmus"
		"item_attribute_res_file" "resource/UI/quests/merasmus/QuestObjectivePanel_InGame.res"

		"standard_glow_color"	"HalloweenThemeColor2015_Light"
		"bonus_glow_color"		"HalloweenThemeColor2015_Light"
	}

	"ProgressBarBG"
	{
		"ProgressBarStandardHighlight" // current completed
		{
			"bgcolor_override"		"HalloweenThemeColor2015_Light"
		}
	}
}