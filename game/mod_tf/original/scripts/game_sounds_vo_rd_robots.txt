"Robot.Collide"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_90dB"

	"rndwave"
	{
		"wave"		"vo/bot_worker/tinybot_crosspaths_03.mp3"	
		"wave"		"vo/bot_worker/tinybot_crosspaths_05.mp3"	
		"wave"		"vo/bot_worker/tinybot_crosspaths_06.mp3"	
	}
}

"Robot.Greeting"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_90dB"

	"rndwave"
	{
		"wave"		"vo/bot_worker/tinybot_incidental_01.mp3"	
		"wave"		"vo/bot_worker/tinybot_incidental_02.mp3"
	}
}

"Robot.Pain"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_90dB"

	"rndwave"
	{
		"wave"		"vo/bot_worker/tinybot_takedamage_01.mp3"	
		"wave"		"vo/bot_worker/tinybot_takedamage_09.mp3"
	}
}

"Robot.Death"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_90dB"

	"rndwave"
	{
		"wave"		"vo/bot_worker/tinybot_death_01.mp3"	
		"wave"		"vo/bot_worker/tinybot_death_02.mp3"		
	}
}

"Announcer.EnemyTeamCloseToWinning"
{
	"channel"		"CHAN_VOICE2"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"	"SNDLVL_NORM"

	"rndwave"
	{
		"wave"			"vo\announcer_map.enemyclosetowinning_01.mp3"
		"wave"			"vo\announcer_map.enemyclosetowinning_02.mp3"
		"wave"			"vo\announcer_map.enemyclosetowinning_03.mp3"
		"wave"			"vo\announcer_map.enemyclosetowinning_05.mp3"
	}
}

"Announcer.OurTeamCloseToWinning"
{
	"channel"		"CHAN_VOICE2"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"	"SNDLVL_NORM"

	"rndwave"
	{
		"wave"			"vo\announcer_map.teamclosetowinning_01.mp3"
		"wave"			"vo\announcer_map.teamclosetowinning_02.mp3"
		"wave"			"vo\announcer_map.teamclosetowinning_03.mp3"
	}
}

"Announcer.HowToPlayRD"
{
	"channel"		"CHAN_VOICE2"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"	"SNDLVL_NORM"
	"wave"			"vo\announcer_map_intro_02.mp3"
}

"RD.TeamScoreCore"
{
	"channel"		"CHAN_STATIC"
	"volume"		"0.4"
	"soundlevel"  	"SNDLVL_85dB"
	"pitch"			"PITCH_NORM"
	"wave"			"ui\chime_rd_2base_pos.wav"
}

"RD.EnemyScoreCore"
{
	"channel"		"CHAN_STATIC"
	"volume"		"0.4"
	"soundlevel"  	"SNDLVL_85dB"
	"pitch"			"PITCH_NORM"
	"wave"			"ui\chime_rd_2base_neg.wav"
}

"RD.EnemyStealingPoints"
{
	"channel"		"CHAN_STATIC"
	"volume"		"0.35"
	"soundlevel"  	"SNDLVL_85dB"
	"pitch"			"PITCH_NORM"
	"wave"			"ambient/alarms/klaxon1.wav"
}

"RD.EnemyCaptured"
{
	"channel"		"CHAN_VOICE2"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"	"SNDLVL_NONE"
	"wave"	"vo/announcer_captureflag_enemycaptured_01.mp3"

}

"RD.EnemyDropped"
{
	"channel"		"CHAN_VOICE2"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"	"SNDLVL_NONE"
	"rndwave"
	{
		"wave"		"vo/announcer_captureflag_enemydropped_01.mp3"	
		"wave"		"vo/announcer_captureflag_enemydropped_02.mp3"	
	}
}

"RD.EnemyReturned"
{
	"channel"		"CHAN_VOICE2"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"	"SNDLVL_NONE"
	"rndwave"
	{
		"wave"		"vo/announcer_captureflag_enemyreturned_01.mp3"	
		"wave"		"vo/announcer_captureflag_enemyreturned_02.mp3"	
		"wave"		"vo/announcer_captureflag_enemyreturned_03.mp3"	
		"wave"		"vo/announcer_captureflag_enemyreturned_04.mp3"	
	}
}

"RD.EnemyStolen"
{
	"channel"		"CHAN_VOICE2"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"	"SNDLVL_NONE"
	"rndwave"
	{
		"wave"		"vo/announcer_captureflag_enemystolen_01.mp3"	
		"wave"		"vo/announcer_captureflag_enemystolen_02.mp3"	
	}
}

"RD.TeamCaptured"
{
	"channel"		"CHAN_VOICE2"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"	"SNDLVL_NONE"
	"rndwave"
	{
		"wave"		"vo/announcer_captureflag_teamcaptured_01.mp3"	
		"wave"		"vo/announcer_captureflag_teamcaptured_02.mp3"	
	}
}

"RD.TeamDropped"
{
	"channel"		"CHAN_VOICE2"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"	"SNDLVL_NONE"
	"rndwave"
	{
		"wave"		"vo/announcer_captureflag_teamdropped_01.mp3"	
		"wave"		"vo/announcer_captureflag_teamdropped_04.mp3"	
		"wave"		"vo/announcer_captureflag_teamdropped_07.mp3"	
		"wave"		"vo/announcer_captureflag_teamdropped_08.mp3"	
	}
}

"RD.TeamReturned"
{
	"channel"		"CHAN_VOICE2"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"	"SNDLVL_NONE"
	"rndwave"
	{
		"wave"		"vo/announcer_captureflag_teamreturned_01.mp3"	
		"wave"		"vo/announcer_captureflag_teamreturned_02.mp3"	
	}
}

"RD.TeamStolen"
{
	"channel"		"CHAN_VOICE2"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"	"SNDLVL_NONE"
	"rndwave"
	{
		"wave"		"vo/announcer_captureflag_teamstolen_01.mp3"	
		"wave"		"vo/announcer_captureflag_teamstolen_02.mp3"	
		"wave"		"vo/announcer_captureflag_teamstolen_03.mp3"	
	}
}

"RD.SpaceGravityTransition"
{
	"channel"		"CHAN_STATIC"
	"volume"		"0.4"
	"soundlevel"  	"SNDLVL_85dB"
	"pitch"			"PITCH_NORM"
	"wave"			"misc\outer_space_transition_01.wav"
}

"RD.BotDeathExplosion"
{
	"channel"		"CHAN_WEAPON"
	"volume"		"1"
	"soundlevel"  	"SNDLVL_94dB"
	"pitch"			"PITCH_NORM"
	"wave"			"misc\rd_robot_explosion01.wav"
}

"RD.FinaleBeep"
{
	"channel"	"CHAN_STATIC"
	"volume"	"1.0"
	"soundlevel"	"SNDLVL_74dB"
	"wave"		"misc/rd_finale_beep01.wav"
}

"RD.FlagReturn"
{
	"channel"		"CHAN_STATIC"
	"volume"		"1.0"
	"soundlevel"	"SNDLVL_74dB"
	"wave"			"misc/rd_points_return01.wav"
}

"RD.FinaleMusic"
{
	"channel"		"CHAN_STATIC"
	"volume"		"0.4"
	"soundlevel"  	"SNDLVL_85dB"
	"pitch"			"PITCH_NORM"
	"wave"			"music\rd_finale.wav"
}