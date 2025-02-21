//
//
// NOTE:  AUTOMATICALLY GENERATED FILE, ANY HAND EDITED COMMENTS WILL BE LOST!!!
// NOTE:  AUTOMATICALLY GENERATED FILE, ANY HAND EDITED COMMENTS WILL BE LOST!!!
// NOTE:  AUTOMATICALLY GENERATED FILE, ANY HAND EDITED COMMENTS WILL BE LOST!!!
//
// VOL_NORM		1.0f
//
//-----------------------------------------------------------------------------
// common attenuation values
//-----------------------------------------------------------------------------
//
// DON'T USE THESE - USE SNDLVL_ INSTEAD!!!
//	ATTN_NONE		0.0f	
//	ATTN_NORM		0.8666f
//	ATTN_IDLE		2.0f
//	ATTN_STATIC		1.25f 
//	ATTN_RICOCHET	1.5f
//	ATTN_GUNFIRE	0.27f
//
//	SNDLVL_NONE		= 0,
//	SNDLVL_25dB		= 25,
//	SNDLVL_30dB		= 30,
//	SNDLVL_35dB		= 35,
//	SNDLVL_40dB		= 40,
//	SNDLVL_45dB		= 45,
//	SNDLVL_50dB		= 50,	= 3.9
//	SNDLVL_55dB		= 55,	= 3.0
//	SNDLVL_IDLE		= 60,	= 2.0
//	SNDLVL_TALKING		= 60,	= 2.0
//	SNDLVL_75dB		= 60,	= 2.0
//	SNDLVL_75dB		= 65,	= 1.5
//	SNDLVL_STATIC		= 66,	= 1.25
//	SNDLVL_75dB		= 70,	= 1.0
//	SNDLVL_NORM		= 75,
//	SNDLVL_75dB		= 75,	= 0.8
//	SNDLVL_80dB		= 80,	= 0.7
//	SNDLVL_85dB		= 85,	= 0.6
//	SNDLVL_90dB		= 90,	= 0.5
//	SNDLVL_95dB		= 95,
//	SNDLVL_100dB	= 100,	= 0.4
//	SNDLVL_105dB	= 105,
//	SNDLVL_120dB	= 120,
//	SNDLVL_130dB	= 130,
//	SNDLVL_GUNFIRE	= 140, = 0.27
//	SNDLVL_140dB	= 140,	= 0.2
//	SNDLVL_150dB	= 150,	= 0.2
//

//-----------------------------------------------------------------------------
// Player Deaths
//-----------------------------------------------------------------------------

"Demoman.MVM_Death"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/demoman_mvm_PainSevere01.mp3"
		"wave"		"vo/mvm/norm/demoman_mvm_PainSevere02.mp3"
		"wave"		"vo/mvm/norm/demoman_mvm_PainSevere03.mp3"
		"wave"		"vo/mvm/norm/demoman_mvm_PainSevere04.mp3"
	}
}

"Demoman.MVM_CritDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/demoman_mvm_PainCrticialDeath01.mp3"
		"wave"		"vo/mvm/norm/demoman_mvm_PainCrticialDeath02.mp3"
		"wave"		"vo/mvm/norm/demoman_mvm_PainCrticialDeath03.mp3"
		"wave"		"vo/mvm/norm/demoman_mvm_PainCrticialDeath04.mp3"
		"wave"		"vo/mvm/norm/demoman_mvm_PainCrticialDeath05.mp3"
	}
}

"Demoman.MVM_MeleeDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/demoman_mvm_PainCrticialDeath01.mp3"
		"wave"		"vo/mvm/norm/demoman_mvm_PainCrticialDeath02.mp3"
		"wave"		"vo/mvm/norm/demoman_mvm_PainCrticialDeath03.mp3"
		"wave"		"vo/mvm/norm/demoman_mvm_PainCrticialDeath04.mp3"
		"wave"		"vo/mvm/norm/demoman_mvm_PainCrticialDeath05.mp3"
	}
}

"Demoman.MVM_ExplosionDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/demoman_mvm_PainSharp01.mp3"
		"wave"		"vo/mvm/norm/demoman_mvm_PainSharp02.mp3"
		"wave"		"vo/mvm/norm/demoman_mvm_PainSharp03.mp3"
		"wave"		"vo/mvm/norm/demoman_mvm_PainSharp04.mp3"
		"wave"		"vo/mvm/norm/demoman_mvm_PainSharp05.mp3"
		"wave"		"vo/mvm/norm/demoman_mvm_PainSharp06.mp3"
		"wave"		"vo/mvm/norm/demoman_mvm_PainSharp07.mp3"
	}
}

"Engineer.MVM_Death"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/engineer_mvm_PainSevere01.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainSevere02.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainSevere03.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainSevere04.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainSevere05.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainSevere06.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainSevere07.mp3"
	}
}

"Engineer.MVM_CritDeath"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/engineer_mvm_PainCrticialDeath01.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainCrticialDeath02.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainCrticialDeath03.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainCrticialDeath04.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainCrticialDeath05.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainCrticialDeath06.mp3"
	}
}

"Engineer.MVM_MeleeDeath"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/engineer_mvm_PainCrticialDeath01.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainCrticialDeath02.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainCrticialDeath03.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainCrticialDeath04.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainCrticialDeath05.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainCrticialDeath06.mp3"
	}
}

"Engineer.MVM_ExplosionDeath"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/engineer_mvm_PainSharp01.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainSharp02.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainSharp03.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainSharp04.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainSharp05.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainSharp06.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainSharp07.mp3"
		"wave"		"vo/mvm/norm/engineer_mvm_PainSharp08.mp3"
	}
}



"Heavy.MVM_Death"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/heavy_mvm_PainSevere01.mp3"
		"wave"		"vo/mvm/norm/heavy_mvm_PainSevere02.mp3"
		"wave"		"vo/mvm/norm/heavy_mvm_PainSevere03.mp3"
	}
}

"Heavy.MVM_CritDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/heavy_mvm_PainCrticialDeath01.mp3"
		"wave"		"vo/mvm/norm/heavy_mvm_PainCrticialDeath02.mp3"
		"wave"		"vo/mvm/norm/heavy_mvm_PainCrticialDeath03.mp3"
	}
}

"Heavy.MVM_MeleeDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/heavy_mvm_PainCrticialDeath01.mp3"
		"wave"		"vo/mvm/norm/heavy_mvm_PainCrticialDeath02.mp3"
		"wave"		"vo/mvm/norm/heavy_mvm_PainCrticialDeath03.mp3"
	}
}

"Heavy.MVM_ExplosionDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/heavy_mvm_PainSharp01.mp3"
		"wave"		"vo/mvm/norm/heavy_mvm_PainSharp02.mp3"
		"wave"		"vo/mvm/norm/heavy_mvm_PainSharp03.mp3"
		"wave"		"vo/mvm/norm/heavy_mvm_PainSharp04.mp3"
		"wave"		"vo/mvm/norm/heavy_mvm_PainSharp05.mp3"
	}
}



"Medic.MVM_Death"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/medic_mvm_PainSevere01.mp3"
		"wave"		"vo/mvm/norm/medic_mvm_PainSevere02.mp3"
		"wave"		"vo/mvm/norm/medic_mvm_PainSevere03.mp3"
		"wave"		"vo/mvm/norm/medic_mvm_PainSevere04.mp3"
	}
}

"Medic.MVM_CritDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/medic_mvm_PainCrticialDeath01.mp3"
		"wave"		"vo/mvm/norm/medic_mvm_PainCrticialDeath02.mp3"
		"wave"		"vo/mvm/norm/medic_mvm_PainCrticialDeath03.mp3"
		"wave"		"vo/mvm/norm/medic_mvm_PainCrticialDeath04.mp3"
	}
}

"Medic.MVM_MeleeDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/medic_mvm_PainCrticialDeath01.mp3"
		"wave"		"vo/mvm/norm/medic_mvm_PainCrticialDeath02.mp3"
		"wave"		"vo/mvm/norm/medic_mvm_PainCrticialDeath03.mp3"
		"wave"		"vo/mvm/norm/medic_mvm_PainCrticialDeath04.mp3"
	}
}

"Medic.MVM_ExplosionDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/medic_mvm_PainSharp01.mp3"
		"wave"		"vo/mvm/norm/medic_mvm_PainSharp02.mp3"
		"wave"		"vo/mvm/norm/medic_mvm_PainSharp03.mp3"
		"wave"		"vo/mvm/norm/medic_mvm_PainSharp04.mp3"
		"wave"		"vo/mvm/norm/medic_mvm_PainSharp05.mp3"
		"wave"		"vo/mvm/norm/medic_mvm_PainSharp06.mp3"
		"wave"		"vo/mvm/norm/medic_mvm_PainSharp07.mp3"
		"wave"		"vo/mvm/norm/medic_mvm_PainSharp08.mp3"
	}
}



"Pyro.MVM_Death"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/pyro_mvm_PainSevere01.mp3"
		"wave"		"vo/mvm/norm/pyro_mvm_PainSevere02.mp3"
		"wave"		"vo/mvm/norm/pyro_mvm_PainSevere03.mp3"
		"wave"		"vo/mvm/norm/pyro_mvm_PainSevere04.mp3"
		"wave"		"vo/mvm/norm/pyro_mvm_PainSevere05.mp3"
		"wave"		"vo/mvm/norm/pyro_mvm_PainSevere06.mp3"
	}
}

"Pyro.MVM_CritDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/pyro_mvm_PainCrticialDeath01.mp3"
		"wave"		"vo/mvm/norm/pyro_mvm_PainCrticialDeath02.mp3"
		"wave"		"vo/mvm/norm/pyro_mvm_PainCrticialDeath03.mp3"
	}
}

"Pyro.MVM_MeleeDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/pyro_mvm_PainCrticialDeath01.mp3"
		"wave"		"vo/mvm/norm/pyro_mvm_PainCrticialDeath02.mp3"
		"wave"		"vo/mvm/norm/pyro_mvm_PainCrticialDeath03.mp3"
	}
}

"Pyro.MVM_ExplosionDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/pyro_mvm_PainSharp01.mp3"
		"wave"		"vo/mvm/norm/pyro_mvm_PainSharp02.mp3"
		"wave"		"vo/mvm/norm/pyro_mvm_PainSharp03.mp3"
		"wave"		"vo/mvm/norm/pyro_mvm_PainSharp04.mp3"
		"wave"		"vo/mvm/norm/pyro_mvm_PainSharp05.mp3"
		"wave"		"vo/mvm/norm/pyro_mvm_PainSharp06.mp3"
		"wave"		"vo/mvm/norm/pyro_mvm_PainSharp07.mp3"
	}
}


"Scout.MVM_Death"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/scout_mvm_PainSevere01.mp3"
		"wave"		"vo/mvm/norm/scout_mvm_PainSevere02.mp3"
		"wave"		"vo/mvm/norm/scout_mvm_PainSevere03.mp3"
		"wave"		"vo/mvm/norm/scout_mvm_PainSevere04.mp3"
		"wave"		"vo/mvm/norm/scout_mvm_PainSevere05.mp3"
		"wave"		"vo/mvm/norm/scout_mvm_PainSevere06.mp3"
	}
}

"Scout.MVM_CritDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/scout_mvm_PainCrticialDeath01.mp3"
		"wave"		"vo/mvm/norm/scout_mvm_PainCrticialDeath02.mp3"
		"wave"		"vo/mvm/norm/scout_mvm_PainCrticialDeath03.mp3"
	}
}

"Scout.MVM_MeleeDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/scout_mvm_PainCrticialDeath01.mp3"
		"wave"		"vo/mvm/norm/scout_mvm_PainCrticialDeath02.mp3"
		"wave"		"vo/mvm/norm/scout_mvm_PainCrticialDeath03.mp3"
	}
}

"Scout.MVM_ExplosionDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/scout_mvm_PainSharp01.mp3"
		"wave"		"vo/mvm/norm/scout_mvm_PainSharp02.mp3"
		"wave"		"vo/mvm/norm/scout_mvm_PainSharp03.mp3"
		"wave"		"vo/mvm/norm/scout_mvm_PainSharp04.mp3"
		"wave"		"vo/mvm/norm/scout_mvm_PainSharp05.mp3"
		"wave"		"vo/mvm/norm/scout_mvm_PainSharp06.mp3"
		"wave"		"vo/mvm/norm/scout_mvm_PainSharp07.mp3"
		"wave"		"vo/mvm/norm/scout_mvm_PainSharp08.mp3"
	}
}


"Sniper.MVM_Death"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/sniper_mvm_PainSevere01.mp3"
		"wave"		"vo/mvm/norm/sniper_mvm_PainSevere02.mp3"
		"wave"		"vo/mvm/norm/sniper_mvm_PainSevere03.mp3"
		"wave"		"vo/mvm/norm/sniper_mvm_PainSevere04.mp3"
	}
}

"Sniper.MVM_CritDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/sniper_mvm_PainCrticialDeath01.mp3"
		"wave"		"vo/mvm/norm/sniper_mvm_PainCrticialDeath02.mp3"
		"wave"		"vo/mvm/norm/sniper_mvm_PainCrticialDeath03.mp3"
		"wave"		"vo/mvm/norm/sniper_mvm_PainCrticialDeath04.mp3"
	}
}

"Sniper.MVM_MeleeDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/sniper_mvm_PainCrticialDeath01.mp3"
		"wave"		"vo/mvm/norm/sniper_mvm_PainCrticialDeath02.mp3"
		"wave"		"vo/mvm/norm/sniper_mvm_PainCrticialDeath03.mp3"
		"wave"		"vo/mvm/norm/sniper_mvm_PainCrticialDeath04.mp3"
	}
}

"Sniper.MVM_ExplosionDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/sniper_mvm_PainSharp01.mp3"
		"wave"		"vo/mvm/norm/sniper_mvm_PainSharp02.mp3"
		"wave"		"vo/mvm/norm/sniper_mvm_PainSharp03.mp3"
		"wave"		"vo/mvm/norm/sniper_mvm_PainSharp04.mp3"
	}
}


"Spy.MVM_Death"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/spy_mvm_PainSevere01.mp3"
		"wave"		"vo/mvm/norm/spy_mvm_PainSevere02.mp3"
		"wave"		"vo/mvm/norm/spy_mvm_PainSevere03.mp3"
		"wave"		"vo/mvm/norm/spy_mvm_PainSevere04.mp3"
		"wave"		"vo/mvm/norm/spy_mvm_PainSevere05.mp3"
	}
}

"Spy.MVM_CritDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/spy_mvm_PainCrticialDeath01.mp3"
		"wave"		"vo/mvm/norm/spy_mvm_PainCrticialDeath02.mp3"
		"wave"		"vo/mvm/norm/spy_mvm_PainCrticialDeath03.mp3"
	}
}

"Spy.MVM_MeleeDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/spy_mvm_PainCrticialDeath01.mp3"
		"wave"		"vo/mvm/norm/spy_mvm_PainCrticialDeath02.mp3"
		"wave"		"vo/mvm/norm/spy_mvm_PainCrticialDeath03.mp3"
	}
}

"Spy.MVM_ExplosionDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/spy_mvm_PainSharp01.mp3"
		"wave"		"vo/mvm/norm/spy_mvm_PainSharp02.mp3"
		"wave"		"vo/mvm/norm/spy_mvm_PainSharp03.mp3"
		"wave"		"vo/mvm/norm/spy_mvm_PainSharp04.mp3"
	}
}


"Soldier.MVM_Death"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/soldier_mvm_PainSevere01.mp3"
		"wave"		"vo/mvm/norm/soldier_mvm_PainSevere02.mp3"
		"wave"		"vo/mvm/norm/soldier_mvm_PainSevere03.mp3"
		"wave"		"vo/mvm/norm/soldier_mvm_PainSevere04.mp3"
		"wave"		"vo/mvm/norm/soldier_mvm_PainSevere05.mp3"
		"wave"		"vo/mvm/norm/soldier_mvm_PainSevere06.mp3"
	}
}

"Soldier.MVM_CritDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/soldier_mvm_PainCrticialDeath01.mp3"
		"wave"		"vo/mvm/norm/soldier_mvm_PainCrticialDeath02.mp3"
		"wave"		"vo/mvm/norm/soldier_mvm_PainCrticialDeath03.mp3"
		"wave"		"vo/mvm/norm/soldier_mvm_PainCrticialDeath04.mp3"
	}
}

"Soldier.MVM_MeleeDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/soldier_mvm_PainCrticialDeath01.mp3"
		"wave"		"vo/mvm/norm/soldier_mvm_PainCrticialDeath02.mp3"
		"wave"		"vo/mvm/norm/soldier_mvm_PainCrticialDeath03.mp3"
		"wave"		"vo/mvm/norm/soldier_mvm_PainCrticialDeath04.mp3"
	}
}

"Soldier.MVM_ExplosionDeath"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
		"volume"  "0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"rndwave"			
	{
		"wave"		"vo/mvm/norm/soldier_mvm_PainSharp01.mp3"
		"wave"		"vo/mvm/norm/soldier_mvm_PainSharp02.mp3"
		"wave"		"vo/mvm/norm/soldier_mvm_PainSharp03.mp3"
		"wave"		"vo/mvm/norm/soldier_mvm_PainSharp04.mp3"
		"wave"		"vo/mvm/norm/soldier_mvm_PainSharp05.mp3"
		"wave"		"vo/mvm/norm/soldier_mvm_PainSharp06.mp3"
		"wave"		"vo/mvm/norm/soldier_mvm_PainSharp07.mp3"
		"wave"		"vo/mvm/norm/soldier_mvm_PainSharp08.mp3"
	}
}


//-----------------------------------------------------------------------------
// End Player Deaths
//-----------------------------------------------------------------------------


"Demoman.MVM_ActivateCharge01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_ActivateCharge01.mp3"
}

"Demoman.MVM_ActivateCharge02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_ActivateCharge02.mp3"
}

"Demoman.MVM_ActivateCharge03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_ActivateCharge03.mp3"
}

"Demoman.MVM_AutoCappedControlPoint01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_AutoCappedControlPoint01.mp3"
}

"Demoman.MVM_AutoCappedControlPoint02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_AutoCappedControlPoint02.mp3"
}

"Demoman.MVM_AutoCappedControlPoint03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_AutoCappedControlPoint03.mp3"
}

"Demoman.MVM_AutoCappedIntelligence01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_AutoCappedIntelligence01.mp3"
}

"Demoman.MVM_AutoCappedIntelligence02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_AutoCappedIntelligence02.mp3"
}

"Demoman.MVM_AutoCappedIntelligence03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_AutoCappedIntelligence03.mp3"
}

"Demoman.MVM_AutoDejectedTie01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_AutoDejectedTie01.mp3"
}

"Demoman.MVM_AutoDejectedTie02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_AutoDejectedTie02.mp3"
}

"Demoman.MVM_AutoDejectedTie03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_AutoDejectedTie03.mp3"
}

"Demoman.MVM_AutoDejectedTie04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_AutoDejectedTie04.mp3"
}

"Demoman.MVM_AutoOnFire01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_AutoOnFire01.mp3"
}

"Demoman.MVM_AutoOnFire02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_AutoOnFire02.mp3"
}

"Demoman.MVM_AutoOnFire03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_AutoOnFire03.mp3"
}

"Demoman.MVM_BattleCry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_BattleCry01.mp3"
}

"Demoman.MVM_BattleCry02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_BattleCry02.mp3"
}

"Demoman.MVM_BattleCry03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_BattleCry03.mp3"
}

"Demoman.MVM_BattleCry04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_BattleCry04.mp3"
}

"Demoman.MVM_BattleCry05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_BattleCry05.mp3"
}

"Demoman.MVM_BattleCry06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_BattleCry06.mp3"
}

"Demoman.MVM_BattleCry07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_BattleCry07.mp3"
}

"Demoman.MVM_Cheers01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Cheers01.mp3"
}

"Demoman.MVM_Cheers02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Cheers02.mp3"
}

"Demoman.MVM_Cheers03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Cheers03.mp3"
}

"Demoman.MVM_Cheers04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Cheers04.mp3"
}

"Demoman.MVM_Cheers05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Cheers05.mp3"
}

"Demoman.MVM_Cheers06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Cheers06.mp3"
}

"Demoman.MVM_Cheers07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Cheers07.mp3"
}

"Demoman.MVM_Cheers08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Cheers08.mp3"
}

"Demoman.MVM_CloakedSpy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_CloakedSpy01.mp3"
}

"Demoman.MVM_CloakedSpy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_CloakedSpy02.mp3"
}

"Demoman.MVM_CloakedSpy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_CloakedSpy03.mp3"
}

"Demoman.MVM_CloakedSpyIdentify01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_CloakedSpyIdentify01.mp3"
}

"Demoman.MVM_CloakedSpyIdentify02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_CloakedSpyIdentify02.mp3"
}

"Demoman.MVM_CloakedSpyIdentify03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_CloakedSpyIdentify03.mp3"
}

"Demoman.MVM_CloakedSpyIdentify04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_CloakedSpyIdentify04.mp3"
}

"Demoman.MVM_CloakedSpyIdentify05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_CloakedSpyIdentify05.mp3"
}

"Demoman.MVM_CloakedSpyIdentify06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_CloakedSpyIdentify06.mp3"
}

"Demoman.MVM_CloakedSpyIdentify07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_CloakedSpyIdentify07.mp3"
}

"Demoman.MVM_CloakedSpyIdentify08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_CloakedSpyIdentify08.mp3"
}

"Demoman.MVM_CloakedSpyIdentify09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_CloakedSpyIdentify09.mp3"
}

"Demoman.MVM_Go01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Go01.mp3"
}

"Demoman.MVM_Go02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Go02.mp3"
}

"Demoman.MVM_Go03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Go03.mp3"
}

"Demoman.MVM_GoodJob01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_GoodJob01.mp3"
}

"Demoman.MVM_GoodJob02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_GoodJob02.mp3"
}

"Demoman.MVM_HeadLeft01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_HeadLeft01.mp3"
}

"Demoman.MVM_HeadLeft02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_HeadLeft02.mp3"
}

"Demoman.MVM_HeadLeft03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_HeadLeft03.mp3"
}

"Demoman.MVM_HeadRight01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_HeadRight01.mp3"
}

"Demoman.MVM_HeadRight02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_HeadRight02.mp3"
}

"Demoman.MVM_HeadRight03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_HeadRight03.mp3"
}

"Demoman.MVM_HelpMe01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_HelpMe01.mp3"
}

"Demoman.MVM_HelpMe02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_HelpMe02.mp3"
}

"Demoman.MVM_HelpMe03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_HelpMe03.mp3"
}

"Demoman.MVM_HelpMeCapture01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_HelpMeCapture01.mp3"
}

"Demoman.MVM_HelpMeCapture02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_HelpMeCapture02.mp3"
}

"Demoman.MVM_HelpMeCapture03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_HelpMeCapture03.mp3"
}

"Demoman.MVM_HelpMeDefend01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_HelpMeDefend01.mp3"
}

"Demoman.MVM_HelpMeDefend02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_HelpMeDefend02.mp3"
}

"Demoman.MVM_HelpMeDefend03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_HelpMeDefend03.mp3"
}

"Demoman.MVM_Incoming01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Incoming01.mp3"
}

"Demoman.MVM_Incoming02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Incoming02.mp3"
}

"Demoman.MVM_Incoming03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Incoming03.mp3"
}

"Demoman.MVM_Jeers01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Jeers01.mp3"
}

"Demoman.MVM_Jeers02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Jeers02.mp3"
}

"Demoman.MVM_Jeers03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Jeers03.mp3"
}

"Demoman.MVM_Jeers04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Jeers04.mp3"
}

"Demoman.MVM_Jeers05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Jeers05.mp3"
}

"Demoman.MVM_Jeers06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Jeers06.mp3"
}

"Demoman.MVM_Jeers07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Jeers07.mp3"
}

"Demoman.MVM_Jeers08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Jeers08.mp3"
}

"Demoman.MVM_Jeers09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Jeers09.mp3"
}

"Demoman.MVM_Jeers10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Jeers10.mp3"
}

"Demoman.MVM_Jeers11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Jeers11.mp3"
}

"Demoman.MVM_LaughEvil01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_LaughEvil01.mp3"
}

"Demoman.MVM_LaughEvil02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_LaughEvil02.mp3"
}

"Demoman.MVM_LaughEvil03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_LaughEvil03.mp3"
}

"Demoman.MVM_LaughEvil04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_LaughEvil04.mp3"
}

"Demoman.MVM_LaughEvil05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_LaughEvil05.mp3"
}

"Demoman.MVM_LaughHappy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_LaughHappy01.mp3"
}

"Demoman.MVM_LaughHappy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_LaughHappy02.mp3"
}

"Demoman.MVM_LaughLong01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_LaughLong01.mp3"
}

"Demoman.MVM_LaughLong02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_LaughLong02.mp3"
}

"Demoman.MVM_LaughShort01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_LaughShort01.mp3"
}

"Demoman.MVM_LaughShort02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_LaughShort02.mp3"
}

"Demoman.MVM_LaughShort03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_LaughShort03.mp3"
}

"Demoman.MVM_LaughShort04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_LaughShort04.mp3"
}

"Demoman.MVM_LaughShort05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_LaughShort05.mp3"
}

"Demoman.MVM_LaughShort06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_LaughShort06.mp3"
}

"Demoman.MVM_Medic01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Medic01.mp3"
}

"Demoman.MVM_Medic02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Medic02.mp3"
}

"Demoman.MVM_Medic03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Medic03.mp3"
}

"Demoman.MVM_MoveUp01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_MoveUp01.mp3"
}

"Demoman.MVM_MoveUp02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_MoveUp02.mp3"
}

"Demoman.MVM_MoveUp03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_MoveUp03.mp3"
}

"Demoman.MVM_NeedDispenser01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_NeedDispenser01.mp3"
}

"Demoman.MVM_NeedSentry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_NeedSentry01.mp3"
}

"Demoman.MVM_NeedTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_NeedTeleporter01.mp3"
}

"Demoman.MVM_NegativeVocalization01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_NegativeVocalization01.mp3"
}

"Demoman.MVM_NegativeVocalization02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_NegativeVocalization02.mp3"
}

"Demoman.MVM_NegativeVocalization03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_NegativeVocalization03.mp3"
}

"Demoman.MVM_NegativeVocalization04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_NegativeVocalization04.mp3"
}

"Demoman.MVM_NegativeVocalization05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_NegativeVocalization05.mp3"
}

"Demoman.MVM_NegativeVocalization06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_NegativeVocalization06.mp3"
}

"Demoman.MVM_NiceShot01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_NiceShot01.mp3"
}

"Demoman.MVM_NiceShot02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_NiceShot02.mp3"
}

"Demoman.MVM_NiceShot03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_NiceShot03.mp3"
}

"Demoman.MVM_No01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_No01.mp3"
}

"Demoman.MVM_No02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_No02.mp3"
}

"Demoman.MVM_No03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_No03.mp3"
}

"Demoman.MVM_PainCrticialDeath01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PainCrticialDeath01.mp3"
}

"Demoman.MVM_PainCrticialDeath02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PainCrticialDeath02.mp3"
}

"Demoman.MVM_PainCrticialDeath03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PainCrticialDeath03.mp3"
}

"Demoman.MVM_PainCrticialDeath04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PainCrticialDeath04.mp3"
}

"Demoman.MVM_PainCrticialDeath05"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PainCrticialDeath05.mp3"
}

"Demoman.MVM_PainSevere01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PainSevere01.mp3"
}

"Demoman.MVM_PainSevere02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PainSevere02.mp3"
}

"Demoman.MVM_PainSevere03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PainSevere03.mp3"
}

"Demoman.MVM_PainSevere04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PainSevere04.mp3"
}

"Demoman.MVM_PainSharp01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PainSharp01.mp3"
}

"Demoman.MVM_PainSharp02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PainSharp02.mp3"
}

"Demoman.MVM_PainSharp03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PainSharp03.mp3"
}

"Demoman.MVM_PainSharp04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PainSharp04.mp3"
}

"Demoman.MVM_PainSharp05"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PainSharp05.mp3"
}

"Demoman.MVM_PainSharp06"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PainSharp06.mp3"
}

"Demoman.MVM_PainSharp07"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"0.820"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PainSharp07.mp3"
}

"Demoman.MVM_PositiveVocalization01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PositiveVocalization01.mp3"
}

"Demoman.MVM_PositiveVocalization02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PositiveVocalization02.mp3"
}

"Demoman.MVM_PositiveVocalization03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PositiveVocalization03.mp3"
}

"Demoman.MVM_PositiveVocalization04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PositiveVocalization04.mp3"
}

"Demoman.MVM_PositiveVocalization05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_PositiveVocalization05.mp3"
}

"Demoman.MVM_SentryAhead01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_SentryAhead01.mp3"
}

"Demoman.MVM_SentryAhead02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_SentryAhead02.mp3"
}

"Demoman.MVM_SentryAhead03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_SentryAhead03.mp3"
}

"Demoman.MVM_SpecialCompleted-AssistedKill01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_SpecialCompleted-AssistedKill01.mp3"
}

"Demoman.MVM_SpecialCompleted-AssistedKill02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_SpecialCompleted-AssistedKill02.mp3"
}

"Demoman.MVM_SpecialCompleted01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_SpecialCompleted01.mp3"
}

"Demoman.MVM_SpecialCompleted02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_SpecialCompleted02.mp3"
}

"Demoman.MVM_SpecialCompleted03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_SpecialCompleted03.mp3"
}

"Demoman.MVM_SpecialCompleted04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_SpecialCompleted04.mp3"
}

"Demoman.MVM_SpecialCompleted05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_SpecialCompleted05.mp3"
}

"Demoman.MVM_SpecialCompleted06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_SpecialCompleted06.mp3"
}

"Demoman.MVM_SpecialCompleted07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_SpecialCompleted07.mp3"
}

"Demoman.MVM_SpecialCompleted08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_SpecialCompleted08.mp3"
}

"Demoman.MVM_SpecialCompleted09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_SpecialCompleted09.mp3"
}

"Demoman.MVM_SpecialCompleted10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_SpecialCompleted10.mp3"
}

"Demoman.MVM_SpecialCompleted11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_SpecialCompleted11.mp3"
}

"Demoman.MVM_SpecialCompleted12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_SpecialCompleted12.mp3"
}

"Demoman.MVM_StandOnThePoint01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_StandOnThePoint01.mp3"
}

"Demoman.MVM_StandOnThePoint02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_StandOnThePoint02.mp3"
}

"Demoman.MVM_Taunts01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/demoman_mvm_Taunts01.mp3"
}

"Demoman.MVM_Taunts02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/demoman_mvm_Taunts02.mp3"
}

"Demoman.MVM_Taunts03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/demoman_mvm_Taunts03.mp3"
}

"Demoman.MVM_Taunts04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/demoman_mvm_Taunts04.mp3"
}

"Demoman.MVM_Taunts05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/demoman_mvm_Taunts05.mp3"
}

"Demoman.MVM_Taunts06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/demoman_mvm_Taunts06.mp3"
}

"Demoman.MVM_Taunts07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/demoman_mvm_Taunts07.mp3"
}

"Demoman.MVM_Taunts08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/demoman_mvm_Taunts08.mp3"
}

"Demoman.MVM_Taunts09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/demoman_mvm_Taunts09.mp3"
}

"Demoman.MVM_Taunts10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/demoman_mvm_Taunts10.mp3"
}

"Demoman.MVM_Taunts11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/demoman_mvm_Taunts11.mp3"
}

"Demoman.MVM_Taunts12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/demoman_mvm_Taunts12.mp3"
}

"Demoman.MVM_Taunts13"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/demoman_mvm_Taunts13.mp3"
}

"Demoman.MVM_Taunts14"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/demoman_mvm_Taunts14.mp3"
}

"Demoman.MVM_Taunts15"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/demoman_mvm_Taunts15.mp3"
}

"Demoman.MVM_Taunts16"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/demoman_mvm_Taunts16.mp3"
}

"Demoman.MVM_Thanks01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Thanks01.mp3"
}

"Demoman.MVM_Thanks02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Thanks02.mp3"
}

"Demoman.MVM_ThanksForTheHeal01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_ThanksForTheHeal01.mp3"
}

"Demoman.MVM_ThanksForTheHeal02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_ThanksForTheHeal02.mp3"
}

"Demoman.MVM_ThanksForTheHeal03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_ThanksForTheHeal03.mp3"
}

"Demoman.MVM_ThanksForTheTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_ThanksForTheTeleporter01.mp3"
}

"Demoman.MVM_ThanksForTheTeleporter02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_ThanksForTheTeleporter02.mp3"
}

"Demoman.MVM_Yes01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Yes01.mp3"
}

"Demoman.MVM_Yes02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Yes02.mp3"
}

"Demoman.MVM_Yes03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/demoman_mvm_Yes03.mp3"
}

"Engineer.MVM_ActivateCharge01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_ActivateCharge01.mp3"
}

"Engineer.MVM_ActivateCharge02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_ActivateCharge02.mp3"
}

"Engineer.MVM_ActivateCharge03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_ActivateCharge03.mp3"
}

"Engineer.MVM_AutoAttackedBySpy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoAttackedBySpy01.mp3"
}

"Engineer.MVM_AutoAttackedBySpy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoAttackedBySpy02.mp3"
}

"Engineer.MVM_AutoAttackedBySpy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoAttackedBySpy03.mp3"
}

"Engineer.MVM_AutoBuildingDispenser01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoBuildingDispenser01.mp3"
}

"Engineer.MVM_AutoBuildingDispenser02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoBuildingDispenser02.mp3"
}

"Engineer.MVM_AutoBuildingSentry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoBuildingSentry01.mp3"
}

"Engineer.MVM_AutoBuildingSentry02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoBuildingSentry02.mp3"
}

"Engineer.MVM_AutoBuildingTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoBuildingTeleporter01.mp3"
}

"Engineer.MVM_AutoBuildingTeleporter02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoBuildingTeleporter02.mp3"
}

"Engineer.MVM_AutoCappedControlPoint01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoCappedControlPoint01.mp3"
}

"Engineer.MVM_AutoCappedControlPoint02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoCappedControlPoint02.mp3"
}

"Engineer.MVM_AutoCappedControlPoint03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoCappedControlPoint03.mp3"
}

"Engineer.MVM_AutoCappedIntelligence01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoCappedIntelligence01.mp3"
}

"Engineer.MVM_AutoCappedIntelligence02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoCappedIntelligence02.mp3"
}

"Engineer.MVM_AutoCappedIntelligence03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoCappedIntelligence03.mp3"
}

"Engineer.MVM_AutoDejectedTie01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoDejectedTie01.mp3"
}

"Engineer.MVM_AutoDejectedTie02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoDejectedTie02.mp3"
}

"Engineer.MVM_AutoDejectedTie03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoDejectedTie03.mp3"
}

"Engineer.MVM_AutoDestroyedDispenser01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoDestroyedDispenser01.mp3"
}

"Engineer.MVM_AutoDestroyedSentry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoDestroyedSentry01.mp3"
}

"Engineer.MVM_AutoDestroyedTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoDestroyedTeleporter01.mp3"
}

"Engineer.MVM_AutoOnFire01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoOnFire01.mp3"
}

"Engineer.MVM_AutoOnFire02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoOnFire02.mp3"
}

"Engineer.MVM_AutoOnFire03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_AutoOnFire03.mp3"
}

"Engineer.MVM_BattleCry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_BattleCry01.mp3"
}

"Engineer.MVM_BattleCry02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_BattleCry02.mp3"
}

"Engineer.MVM_BattleCry03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_BattleCry03.mp3"
}

"Engineer.MVM_BattleCry04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_BattleCry04.mp3"
}

"Engineer.MVM_BattleCry05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_BattleCry05.mp3"
}

"Engineer.MVM_BattleCry06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_BattleCry06.mp3"
}

"Engineer.MVM_BattleCry07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_BattleCry07.mp3"
}

"Engineer.MVM_Cheers01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Cheers01.mp3"
}

"Engineer.MVM_Cheers02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Cheers02.mp3"
}

"Engineer.MVM_Cheers03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Cheers03.mp3"
}

"Engineer.MVM_Cheers04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Cheers04.mp3"
}

"Engineer.MVM_Cheers05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Cheers05.mp3"
}

"Engineer.MVM_Cheers06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Cheers06.mp3"
}

"Engineer.MVM_Cheers07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Cheers07.mp3"
}

"Engineer.MVM_CloakedSpy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_CloakedSpy01.mp3"
}

"Engineer.MVM_CloakedSpy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_CloakedSpy02.mp3"
}

"Engineer.MVM_CloakedSpy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_CloakedSpy03.mp3"
}

"Engineer.MVM_CloakedSpyIdentify01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_CloakedSpyIdentify01.mp3"
}

"Engineer.MVM_CloakedSpyIdentify02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_CloakedSpyIdentify02.mp3"
}

"Engineer.MVM_CloakedSpyIdentify03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_CloakedSpyIdentify03.mp3"
}

"Engineer.MVM_CloakedSpyIdentify04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_CloakedSpyIdentify04.mp3"
}

"Engineer.MVM_CloakedSpyIdentify05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_CloakedSpyIdentify05.mp3"
}

"Engineer.MVM_CloakedSpyIdentify06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_CloakedSpyIdentify06.mp3"
}

"Engineer.MVM_CloakedSpyIdentify07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_CloakedSpyIdentify07.mp3"
}

"Engineer.MVM_CloakedSpyIdentify08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_CloakedSpyIdentify08.mp3"
}

"Engineer.MVM_CloakedSpyIdentify09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_CloakedSpyIdentify09.mp3"
}

"Engineer.MVM_CloakedSpyIdentify10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_CloakedSpyIdentify10.mp3"
}

"Engineer.MVM_Go01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Go01.mp3"
}

"Engineer.MVM_Go02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Go02.mp3"
}

"Engineer.MVM_Go03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Go03.mp3"
}

"Engineer.MVM_GoodJob01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_GoodJob01.mp3"
}

"Engineer.MVM_GoodJob02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_GoodJob02.mp3"
}

"Engineer.MVM_GoodJob03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_GoodJob03.mp3"
}

"Engineer.MVM_HeadLeft01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_HeadLeft01.mp3"
}

"Engineer.MVM_HeadLeft02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_HeadLeft02.mp3"
}

"Engineer.MVM_HeadRight01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_HeadRight01.mp3"
}

"Engineer.MVM_HeadRight02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_HeadRight02.mp3"
}

"Engineer.MVM_HeadRight03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_HeadRight03.mp3"
}

"Engineer.MVM_HelpMe01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_HelpMe01.mp3"
}

"Engineer.MVM_HelpMe02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_HelpMe02.mp3"
}

"Engineer.MVM_HelpMe03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_HelpMe03.mp3"
}

"Engineer.MVM_HelpMeCapture01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_HelpMeCapture01.mp3"
}

"Engineer.MVM_HelpMeCapture02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_HelpMeCapture02.mp3"
}

"Engineer.MVM_HelpMeCapture03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_HelpMeCapture03.mp3"
}

"Engineer.MVM_HelpMeDefend01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_HelpMeDefend01.mp3"
}

"Engineer.MVM_HelpMeDefend02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_HelpMeDefend02.mp3"
}

"Engineer.MVM_HelpMeDefend03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_HelpMeDefend03.mp3"
}

"Engineer.MVM_Incoming01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Incoming01.mp3"
}

"Engineer.MVM_Incoming02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Incoming02.mp3"
}

"Engineer.MVM_Incoming03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Incoming03.mp3"
}

"Engineer.MVM_Jeers01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Jeers01.mp3"
}

"Engineer.MVM_Jeers02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Jeers02.mp3"
}

"Engineer.MVM_Jeers03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Jeers03.mp3"
}

"Engineer.MVM_Jeers04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Jeers04.mp3"
}

"Engineer.MVM_LaughEvil01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_LaughEvil01.mp3"
}

"Engineer.MVM_LaughEvil02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_LaughEvil02.mp3"
}

"Engineer.MVM_LaughEvil03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_LaughEvil03.mp3"
}

"Engineer.MVM_LaughEvil04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_LaughEvil04.mp3"
}

"Engineer.MVM_LaughEvil05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_LaughEvil05.mp3"
}

"Engineer.MVM_LaughEvil06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_LaughEvil06.mp3"
}

"Engineer.MVM_LaughHappy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_LaughHappy01.mp3"
}

"Engineer.MVM_LaughHappy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_LaughHappy02.mp3"
}

"Engineer.MVM_LaughHappy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_LaughHappy03.mp3"
}

"Engineer.MVM_LaughLong01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_LaughLong01.mp3"
}
"Engineer.MVM_LaughLong02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_LaughLong02.mp3"
}


"Engineer.MVM_LaughShort01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_LaughShort01.mp3"
}

"Engineer.MVM_LaughShort02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_LaughShort02.mp3"
}

"Engineer.MVM_LaughShort03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_LaughShort03.mp3"
}

"Engineer.MVM_LaughShort04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_LaughShort04.mp3"
}

"Engineer.MVM_Medic01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Medic01.mp3"
}

"Engineer.MVM_Medic02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Medic02.mp3"
}

"Engineer.MVM_Medic03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Medic03.mp3"
}

"Engineer.MVM_MoveUp01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_MoveUp01.mp3"
}

"Engineer.MVM_NeedDispenser01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NeedDispenser01.mp3"
}

"Engineer.MVM_NeedSentry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NeedSentry01.mp3"
}

"Engineer.MVM_NeedTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NeedTeleporter01.mp3"
}

"Engineer.MVM_NeedTeleporter02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NeedTeleporter02.mp3"
}

"Engineer.MVM_NegativeVocalization01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NegativeVocalization01.mp3"
}

"Engineer.MVM_NegativeVocalization02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NegativeVocalization02.mp3"
}

"Engineer.MVM_NegativeVocalization03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NegativeVocalization03.mp3"
}

"Engineer.MVM_NegativeVocalization04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NegativeVocalization04.mp3"
}

"Engineer.MVM_NegativeVocalization05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NegativeVocalization05.mp3"
}

"Engineer.MVM_NegativeVocalization06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NegativeVocalization06.mp3"
}

"Engineer.MVM_NegativeVocalization07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NegativeVocalization07.mp3"
}

"Engineer.MVM_NegativeVocalization08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NegativeVocalization08.mp3"
}

"Engineer.MVM_NegativeVocalization09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NegativeVocalization09.mp3"
}

"Engineer.MVM_NegativeVocalization10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NegativeVocalization10.mp3"
}

"Engineer.MVM_NegativeVocalization11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NegativeVocalization11.mp3"
}

"Engineer.MVM_NegativeVocalization12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NegativeVocalization12.mp3"
}

"Engineer.MVM_NiceShot01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NiceShot01.mp3"
}

"Engineer.MVM_NiceShot02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NiceShot02.mp3"
}

"Engineer.MVM_NiceShot03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_NiceShot03.mp3"
}

"Engineer.MVM_No01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_No01.mp3"
}

"Engineer.MVM_No02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_No02.mp3"
}

"Engineer.MVM_No03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_No03.mp3"
}

"Engineer.MVM_PainCrticialDeath01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainCrticialDeath01.mp3"
}

"Engineer.MVM_PainCrticialDeath02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainCrticialDeath02.mp3"
}

"Engineer.MVM_PainCrticialDeath03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainCrticialDeath03.mp3"
}

"Engineer.MVM_PainCrticialDeath04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainCrticialDeath04.mp3"
}

"Engineer.MVM_PainCrticialDeath05"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainCrticialDeath05.mp3"
}

"Engineer.MVM_PainCrticialDeath06"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainCrticialDeath06.mp3"
}

"Engineer.MVM_PainSevere01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainSevere01.mp3"
}

"Engineer.MVM_PainSevere02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainSevere02.mp3"
}

"Engineer.MVM_PainSevere03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainSevere03.mp3"
}

"Engineer.MVM_PainSevere04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainSevere04.mp3"
}

"Engineer.MVM_PainSevere05"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainSevere05.mp3"
}

"Engineer.MVM_PainSevere06"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainSevere06.mp3"
}

"Engineer.MVM_PainSevere07"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainSevere07.mp3"
}

"Engineer.MVM_PainSharp01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainSharp01.mp3"
}

"Engineer.MVM_PainSharp02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainSharp02.mp3"
}

"Engineer.MVM_PainSharp03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainSharp03.mp3"
}

"Engineer.MVM_PainSharp04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainSharp04.mp3"
}

"Engineer.MVM_PainSharp05"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainSharp05.mp3"
}

"Engineer.MVM_PainSharp06"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainSharp06.mp3"
}

"Engineer.MVM_PainSharp07"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainSharp07.mp3"
}

"Engineer.MVM_PainSharp08"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PainSharp08.mp3"
}

"Engineer.MVM_PositiveVocalization01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_PositiveVocalization01.mp3"
}

"Engineer.MVM_SentryAhead01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_SentryAhead01.mp3"
}

"Engineer.MVM_SentryAhead02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_SentryAhead02.mp3"
}

"Engineer.MVM_SpecialCompleted-AssistedKill01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_SpecialCompleted-AssistedKill01.mp3"
}

"Engineer.MVM_SpecialCompleted-AssistedKill02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_SpecialCompleted-AssistedKill02.mp3"
}

"Engineer.MVM_SpecialCompleted01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_SpecialCompleted01.mp3"
}

"Engineer.MVM_SpecialCompleted02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_SpecialCompleted02.mp3"
}

"Engineer.MVM_SpecialCompleted03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_SpecialCompleted03.mp3"
}

"Engineer.MVM_SpecialCompleted04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_SpecialCompleted04.mp3"
}

"Engineer.MVM_SpecialCompleted05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_SpecialCompleted05.mp3"
}

"Engineer.MVM_SpecialCompleted06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_SpecialCompleted06.mp3"
}

"Engineer.MVM_SpecialCompleted07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_SpecialCompleted07.mp3"
}

"Engineer.MVM_SpecialCompleted08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_SpecialCompleted08.mp3"
}

"Engineer.MVM_SpecialCompleted09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_SpecialCompleted09.mp3"
}

"Engineer.MVM_SpecialCompleted10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_SpecialCompleted10.mp3"
}

"Engineer.MVM_SpecialCompleted11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_SpecialCompleted11.mp3"
}

"Engineer.MVM_StandOnThePoint01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_StandOnThePoint01.mp3"
}

"Engineer.MVM_StandOnThePoint02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_StandOnThePoint02.mp3"
}

"Engineer.MVM_Taunts01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/engineer_mvm_Taunts01.mp3"
}

"Engineer.MVM_Taunts02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/engineer_mvm_Taunts02.mp3"
}

"Engineer.MVM_Taunts03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/engineer_mvm_Taunts03.mp3"
}

"Engineer.MVM_Taunts04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/engineer_mvm_Taunts04.mp3"
}

"Engineer.MVM_Taunts05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/engineer_mvm_Taunts05.mp3"
}

"Engineer.MVM_Taunts06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/engineer_mvm_Taunts06.mp3"
}

"Engineer.MVM_Taunts07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/engineer_mvm_Taunts07.mp3"
}

"Engineer.MVM_Taunts08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/engineer_mvm_Taunts08.mp3"
}

"Engineer.MVM_Taunts09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/engineer_mvm_Taunts09.mp3"
}

"Engineer.MVM_Taunts10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/engineer_mvm_Taunts10.mp3"
}

"Engineer.MVM_Taunts11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/engineer_mvm_Taunts11.mp3"
}

"Engineer.MVM_Taunts12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/engineer_mvm_Taunts12.mp3"
}

"Engineer.MVM_Thanks01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Thanks01.mp3"
}

"Engineer.MVM_ThanksForTheHeal01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_ThanksForTheHeal01.mp3"
}

"Engineer.MVM_ThanksForTheHeal02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_ThanksForTheHeal02.mp3"
}

"Engineer.MVM_ThanksForTheTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_ThanksForTheTeleporter01.mp3"
}

"Engineer.MVM_ThanksForTheTeleporter02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_ThanksForTheTeleporter02.mp3"
}

"Engineer.MVM_Yes01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Yes01.mp3"
}

"Engineer.MVM_Yes02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Yes02.mp3"
}

"Engineer.MVM_Yes03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/engineer_mvm_Yes03.mp3"
}

"Heavy.MVM_ActivateCharge01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_ActivateCharge01.mp3"
}

"Heavy.MVM_ActivateCharge02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_ActivateCharge02.mp3"
}

"Heavy.MVM_ActivateCharge03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_ActivateCharge03.mp3"
}

"Heavy.MVM_ActivateCharge04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_ActivateCharge04.mp3"
}

"Heavy.MVM_AutoCappedControlPoint01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_AutoCappedControlPoint01.mp3"
}

"Heavy.MVM_AutoCappedControlPoint02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_AutoCappedControlPoint02.mp3"
}

"Heavy.MVM_AutoCappedControlPoint03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_AutoCappedControlPoint03.mp3"
}

"Heavy.MVM_AutoCappedIntelligence01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_AutoCappedIntelligence01.mp3"
}

"Heavy.MVM_AutoCappedIntelligence02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_AutoCappedIntelligence02.mp3"
}

"Heavy.MVM_AutoCappedIntelligence03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_AutoCappedIntelligence03.mp3"
}

"Heavy.MVM_AutoDejectedTie01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_AutoDejectedTie01.mp3"
}

"Heavy.MVM_AutoDejectedTie02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_AutoDejectedTie02.mp3"
}

"Heavy.MVM_AutoDejectedTie03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_AutoDejectedTie03.mp3"
}

"Heavy.MVM_AutoOnFire01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_AutoOnFire01.mp3"
}

"Heavy.MVM_AutoOnFire02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_AutoOnFire02.mp3"
}

"Heavy.MVM_AutoOnFire03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_AutoOnFire03.mp3"
}

"Heavy.MVM_AutoOnFire04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_AutoOnFire04.mp3"
}

"Heavy.MVM_AutoOnFire05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_AutoOnFire05.mp3"
}

"Heavy.MVM_BattleCry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_BattleCry01.mp3"
}

"Heavy.MVM_BattleCry02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_BattleCry02.mp3"
}

"Heavy.MVM_BattleCry03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_BattleCry03.mp3"
}

"Heavy.MVM_BattleCry04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_BattleCry04.mp3"
}

"Heavy.MVM_BattleCry05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_BattleCry05.mp3"
}

"Heavy.MVM_BattleCry06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_BattleCry06.mp3"
}

"Heavy.MVM_Cheers01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Cheers01.mp3"
}

"Heavy.MVM_Cheers02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Cheers02.mp3"
}

"Heavy.MVM_Cheers03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Cheers03.mp3"
}

"Heavy.MVM_Cheers04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Cheers04.mp3"
}

"Heavy.MVM_Cheers05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Cheers05.mp3"
}

"Heavy.MVM_Cheers06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Cheers06.mp3"
}

"Heavy.MVM_Cheers07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Cheers07.mp3"
}

"Heavy.MVM_Cheers08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Cheers08.mp3"
}

"Heavy.MVM_CloakedSpy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_CloakedSpy01.mp3"
}

"Heavy.MVM_CloakedSpy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_CloakedSpy02.mp3"
}

"Heavy.MVM_CloakedSpy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_CloakedSpy03.mp3"
}

"Heavy.MVM_CloakedSpy04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_CloakedSpy04.mp3"
}

"Heavy.MVM_CloakedSpyIdentify01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_CloakedSpyIdentify01.mp3"
}

"Heavy.MVM_CloakedSpyIdentify02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_CloakedSpyIdentify02.mp3"
}

"Heavy.MVM_CloakedSpyIdentify03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_CloakedSpyIdentify03.mp3"
}

"Heavy.MVM_CloakedSpyIdentify04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_CloakedSpyIdentify04.mp3"
}

"Heavy.MVM_CloakedSpyIdentify05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_CloakedSpyIdentify05.mp3"
}

"Heavy.MVM_CloakedSpyIdentify06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_CloakedSpyIdentify06.mp3"
}

"Heavy.MVM_CloakedSpyIdentify07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_CloakedSpyIdentify07.mp3"
}

"Heavy.MVM_CloakedSpyIdentify08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_CloakedSpyIdentify08.mp3"
}

"Heavy.MVM_CloakedSpyIdentify09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_CloakedSpyIdentify09.mp3"
}

"Heavy.MVM_Generic01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Generic01.mp3"
}

"Heavy.MVM_Go01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Go01.mp3"
}

"Heavy.MVM_Go02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Go02.mp3"
}

"Heavy.MVM_Go03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Go03.mp3"
}

"Heavy.MVM_GoodJob01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_GoodJob01.mp3"
}

"Heavy.MVM_GoodJob02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_GoodJob02.mp3"
}

"Heavy.MVM_GoodJob03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_GoodJob03.mp3"
}

"Heavy.MVM_GoodJob04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_GoodJob04.mp3"
}

"Heavy.MVM_HeadLeft01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_HeadLeft01.mp3"
}

"Heavy.MVM_HeadLeft02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_HeadLeft02.mp3"
}

"Heavy.MVM_HeadLeft03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_HeadLeft03.mp3"
}

"Heavy.MVM_HeadRight01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_HeadRight01.mp3"
}

"Heavy.MVM_HeadRight02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_HeadRight02.mp3"
}

"Heavy.MVM_HeadRight03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_HeadRight03.mp3"
}

"Heavy.MVM_HelpMe01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_HelpMe01.mp3"
}

"Heavy.MVM_HelpMe02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_HelpMe02.mp3"
}

"Heavy.MVM_HelpMe03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_HelpMe03.mp3"
}

"Heavy.MVM_HelpMeCapture01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_HelpMeCapture01.mp3"
}

"Heavy.MVM_HelpMeCapture02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_HelpMeCapture02.mp3"
}

"Heavy.MVM_HelpMeCapture03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_HelpMeCapture03.mp3"
}

"Heavy.MVM_HelpMeDefend01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_HelpMeDefend01.mp3"
}

"Heavy.MVM_HelpMeDefend02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_HelpMeDefend02.mp3"
}

"Heavy.MVM_HelpMeDefend03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_HelpMeDefend03.mp3"
}

"Heavy.MVM_Incoming01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Incoming01.mp3"
}

"Heavy.MVM_Incoming02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Incoming02.mp3"
}

"Heavy.MVM_Incoming03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Incoming03.mp3"
}

"Heavy.MVM_Jeers01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Jeers01.mp3"
}

"Heavy.MVM_Jeers02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Jeers02.mp3"
}

"Heavy.MVM_Jeers03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Jeers03.mp3"
}

"Heavy.MVM_Jeers04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Jeers04.mp3"
}

"Heavy.MVM_Jeers05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Jeers05.mp3"
}

"Heavy.MVM_Jeers06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Jeers06.mp3"
}

"Heavy.MVM_Jeers07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Jeers07.mp3"
}

"Heavy.MVM_Jeers08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Jeers08.mp3"
}

"Heavy.MVM_Jeers09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Jeers09.mp3"
}

"Heavy.MVM_LaughEvil01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_LaughEvil01.mp3"
}

"Heavy.MVM_LaughEvil02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_LaughEvil02.mp3"
}

"Heavy.MVM_LaughEvil03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_LaughEvil03.mp3"
}

"Heavy.MVM_LaughEvil04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_LaughEvil04.mp3"
}

"Heavy.MVM_LaughHappy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_LaughHappy01.mp3"
}

"Heavy.MVM_LaughHappy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_LaughHappy02.mp3"
}

"Heavy.MVM_LaughHappy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_LaughHappy03.mp3"
}

"Heavy.MVM_LaughHappy04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_LaughHappy04.mp3"
}

"Heavy.MVM_LaughHappy05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_LaughHappy05.mp3"
}

"Heavy.MVM_LaughLong01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_LaughLong01.mp3"
}

"Heavy.MVM_LaughLong02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_LaughLong02.mp3"
}

"Heavy.MVM_LaughShort01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_LaughShort01.mp3"
}

"Heavy.MVM_LaughShort02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_LaughShort02.mp3"
}

"Heavy.MVM_LaughShort03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_LaughShort03.mp3"
}

"Heavy.MVM_LaugherBigSnort01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_laugherbigsnort01.mp3"
}

"Heavy.MVM_Medic01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Medic01.mp3"
}

"Heavy.MVM_Medic02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Medic02.mp3"
}

"Heavy.MVM_Medic03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Medic03.mp3"
}

"Heavy.MVM_MoveUp01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_MoveUp01.mp3"
}

"Heavy.MVM_MoveUp02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_MoveUp02.mp3"
}

"Heavy.MVM_MoveUp03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_MoveUp03.mp3"
}

"Heavy.MVM_NeedDispenser01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_NeedDispenser01.mp3"
}

"Heavy.MVM_NeedSentry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_NeedSentry01.mp3"
}

"Heavy.MVM_NeedTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_NeedTeleporter01.mp3"
}

"Heavy.MVM_NegativeVocalization01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_NegativeVocalization01.mp3"
}

"Heavy.MVM_NegativeVocalization02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_NegativeVocalization02.mp3"
}

"Heavy.MVM_NegativeVocalization03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_NegativeVocalization03.mp3"
}

"Heavy.MVM_NegativeVocalization04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_NegativeVocalization04.mp3"
}

"Heavy.MVM_NegativeVocalization05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_NegativeVocalization05.mp3"
}

"Heavy.MVM_NegativeVocalization06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_NegativeVocalization06.mp3"
}

"Heavy.MVM_NiceShot01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_NiceShot01.mp3"
}

"Heavy.MVM_NiceShot02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_NiceShot02.mp3"
}

"Heavy.MVM_NiceShot03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_NiceShot03.mp3"
}

"Heavy.MVM_No01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_No01.mp3"
}

"Heavy.MVM_No02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_No02.mp3"
}

"Heavy.MVM_No03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_No03.mp3"
}

"Heavy.MVM_PainCrticialDeath01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/heavy_mvm_PainCrticialDeath01.mp3"
}

"Heavy.MVM_PainCrticialDeath02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/heavy_mvm_PainCrticialDeath02.mp3"
}

"Heavy.MVM_PainCrticialDeath03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/heavy_mvm_PainCrticialDeath03.mp3"
}

"Heavy.MVM_PainSevere01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/heavy_mvm_PainSevere01.mp3"
}

"Heavy.MVM_PainSevere02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/heavy_mvm_PainSevere02.mp3"
}

"Heavy.MVM_PainSevere03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/heavy_mvm_PainSevere03.mp3"
}

"Heavy.MVM_PainSharp01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/heavy_mvm_PainSharp01.mp3"
}

"Heavy.MVM_PainSharp02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/heavy_mvm_PainSharp02.mp3"
}

"Heavy.MVM_PainSharp03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/heavy_mvm_PainSharp03.mp3"
}

"Heavy.MVM_PainSharp04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/heavy_mvm_PainSharp04.mp3"
}

"Heavy.MVM_PainSharp05"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/heavy_mvm_PainSharp05.mp3"
}

"Heavy.MVM_PositiveVocalization01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_PositiveVocalization01.mp3"
}

"Heavy.MVM_PositiveVocalization02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_PositiveVocalization02.mp3"
}

"Heavy.MVM_PositiveVocalization03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_PositiveVocalization03.mp3"
}

"Heavy.MVM_PositiveVocalization04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_PositiveVocalization04.mp3"
}

"Heavy.MVM_PositiveVocalization05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_PositiveVocalization05.mp3"
}

"Heavy.MVM_SentryAhead01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_SentryAhead01.mp3"
}

"Heavy.MVM_SentryAhead02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_SentryAhead02.mp3"
}

"Heavy.MVM_SpecialCompleted-AssistedKill01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_SpecialCompleted-AssistedKill01.mp3"
}

"Heavy.MVM_SpecialCompleted01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_SpecialCompleted01.mp3"
}

"Heavy.MVM_SpecialCompleted02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_SpecialCompleted02.mp3"
}

"Heavy.MVM_SpecialCompleted03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_SpecialCompleted03.mp3"
}

"Heavy.MVM_SpecialCompleted04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_SpecialCompleted04.mp3"
}

"Heavy.MVM_SpecialCompleted05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_SpecialCompleted05.mp3"
}

"Heavy.MVM_SpecialCompleted06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_SpecialCompleted06.mp3"
}

"Heavy.MVM_SpecialCompleted07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_SpecialCompleted07.mp3"
}

"Heavy.MVM_SpecialCompleted08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_SpecialCompleted08.mp3"
}

"Heavy.MVM_SpecialCompleted09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_SpecialCompleted09.mp3"
}

"Heavy.MVM_SpecialCompleted10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_SpecialCompleted10.mp3"
}

"Heavy.MVM_SpecialCompleted11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_SpecialCompleted11.mp3"
}

"Heavy.MVM_StandOnThePoint01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_StandOnThePoint01.mp3"
}

"Heavy.MVM_StandOnThePoint02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_StandOnThePoint02.mp3"
}

"Heavy.MVM_StandOnThePoint03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_StandOnThePoint03.mp3"
}

"Heavy.MVM_StandOnThePoint04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_StandOnThePoint04.mp3"
}

"Heavy.MVM_Taunts01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts01.mp3"
}

"Heavy.MVM_Taunts02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts02.mp3"
}

"Heavy.MVM_Taunts03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts03.mp3"
}

"Heavy.MVM_Taunts04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts04.mp3"
}

"Heavy.MVM_Taunts05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts05.mp3"
}

"Heavy.MVM_Taunts06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts06.mp3"
}

"Heavy.MVM_Taunts07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts07.mp3"
}

"Heavy.MVM_Taunts08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts08.mp3"
}

"Heavy.MVM_Taunts09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts09.mp3"
}

"Heavy.MVM_Taunts10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts10.mp3"
}

"Heavy.MVM_Taunts11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts11.mp3"
}

"Heavy.MVM_Taunts12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts12.mp3"
}

"Heavy.MVM_Taunts13"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts13.mp3"
}

"Heavy.MVM_Taunts14"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts14.mp3"
}

"Heavy.MVM_Taunts15"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts15.mp3"
}

"Heavy.MVM_Taunts16"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts16.mp3"
}

"Heavy.MVM_Taunts17"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts17.mp3"
}

"Heavy.MVM_Taunts18"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts18.mp3"
}

"Heavy.MVM_Taunts19"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/heavy_mvm_Taunts19.mp3"
}

"Heavy.MVM_Thanks01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Thanks01.mp3"
}

"Heavy.MVM_Thanks02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Thanks02.mp3"
}

"Heavy.MVM_Thanks03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Thanks03.mp3"
}

"Heavy.MVM_ThanksForTheHeal01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_ThanksForTheHeal01.mp3"
}

"Heavy.MVM_ThanksForTheHeal02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_ThanksForTheHeal02.mp3"
}

"Heavy.MVM_ThanksForTheHeal03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_ThanksForTheHeal03.mp3"
}

"Heavy.MVM_ThanksForTheTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_ThanksForTheTeleporter01.mp3"
}

"Heavy.MVM_ThanksForTheTeleporter02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_ThanksForTheTeleporter02.mp3"
}

"Heavy.MVM_ThanksForTheTeleporter03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_ThanksForTheTeleporter03.mp3"
}

"Heavy.MVM_Yes01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Yes01.mp3"
}

"Heavy.MVM_Yes02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Yes02.mp3"
}

"Heavy.MVM_Yes03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/heavy_mvm_Yes03.mp3"
}

"Medic.MVM_ActivateCharge01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_ActivateCharge01.mp3"
}

"Medic.MVM_ActivateCharge02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_ActivateCharge02.mp3"
}

"Medic.MVM_ActivateCharge03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_ActivateCharge03.mp3"
}

"Medic.MVM_AutoCappedControlPoint01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoCappedControlPoint01.mp3"
}

"Medic.MVM_AutoCappedControlPoint02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoCappedControlPoint02.mp3"
}

"Medic.MVM_AutoCappedControlPoint03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoCappedControlPoint03.mp3"
}

"Medic.MVM_AutoCappedIntelligence01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoCappedIntelligence01.mp3"
}

"Medic.MVM_AutoCappedIntelligence02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoCappedIntelligence02.mp3"
}

"Medic.MVM_AutoCappedIntelligence03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoCappedIntelligence03.mp3"
}

"Medic.MVM_AutoChargeReady01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoChargeReady01.mp3"
}

"Medic.MVM_AutoChargeReady02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoChargeReady02.mp3"
}

"Medic.MVM_AutoChargeReady03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoChargeReady03.mp3"
}

"Medic.MVM_AutoDejectedTie01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoDejectedTie01.mp3"
}

"Medic.MVM_AutoDejectedTie02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoDejectedTie02.mp3"
}

"Medic.MVM_AutoDejectedTie03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoDejectedTie03.mp3"
}

"Medic.MVM_AutoDejectedTie04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoDejectedTie04.mp3"
}

"Medic.MVM_AutoDejectedTie05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoDejectedTie05.mp3"
}

"Medic.MVM_AutoDejectedTie06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoDejectedTie06.mp3"
}

"Medic.MVM_AutoDejectedTie07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoDejectedTie07.mp3"
}

"Medic.MVM_AutoOnFire01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoOnFire01.mp3"
}

"Medic.MVM_AutoOnFire02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoOnFire02.mp3"
}

"Medic.MVM_AutoOnFire03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoOnFire03.mp3"
}

"Medic.MVM_AutoOnFire04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoOnFire04.mp3"
}

"Medic.MVM_AutoOnFire05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_AutoOnFire05.mp3"
}

"Medic.MVM_BattleCry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_BattleCry01.mp3"
}

"Medic.MVM_BattleCry02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_BattleCry02.mp3"
}

"Medic.MVM_BattleCry03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_BattleCry03.mp3"
}

"Medic.MVM_BattleCry04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_BattleCry04.mp3"
}

"Medic.MVM_BattleCry05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_BattleCry05.mp3"
}

"Medic.MVM_BattleCry06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_BattleCry06.mp3"
}

"Medic.MVM_Cheers01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Cheers01.mp3"
}

"Medic.MVM_Cheers02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Cheers02.mp3"
}

"Medic.MVM_Cheers03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Cheers03.mp3"
}

"Medic.MVM_Cheers04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Cheers04.mp3"
}

"Medic.MVM_Cheers05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Cheers05.mp3"
}

"Medic.MVM_Cheers06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Cheers06.mp3"
}

"Medic.MVM_CloakedSpy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_CloakedSpy01.mp3"
}

"Medic.MVM_CloakedSpy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_CloakedSpy02.mp3"
}

"Medic.MVM_CloakedSpyIdentify01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_CloakedSpyIdentify01.mp3"
}

"Medic.MVM_CloakedSpyIdentify02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_CloakedSpyIdentify02.mp3"
}

"Medic.MVM_CloakedSpyIdentify03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_CloakedSpyIdentify03.mp3"
}

"Medic.MVM_CloakedSpyIdentify04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_CloakedSpyIdentify04.mp3"
}

"Medic.MVM_CloakedSpyIdentify05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_CloakedSpyIdentify05.mp3"
}

"Medic.MVM_CloakedSpyIdentify06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_CloakedSpyIdentify06.mp3"
}

"Medic.MVM_CloakedSpyIdentify07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_CloakedSpyIdentify07.mp3"
}

"Medic.MVM_CloakedSpyIdentify08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_CloakedSpyIdentify08.mp3"
}

"Medic.MVM_CloakedSpyIdentify09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_CloakedSpyIdentify09.mp3"
}

"Medic.MVM_Go01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Go01.mp3"
}

"Medic.MVM_Go02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Go02.mp3"
}

"Medic.MVM_Go03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Go03.mp3"
}

"Medic.MVM_Go04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Go04.mp3"
}

"Medic.MVM_GoodJob01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_GoodJob01.mp3"
}

"Medic.MVM_GoodJob02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_GoodJob02.mp3"
}

"Medic.MVM_GoodJob03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_GoodJob03.mp3"
}

"Medic.MVM_HeadLeft01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_HeadLeft01.mp3"
}

"Medic.MVM_HeadLeft02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_HeadLeft02.mp3"
}

"Medic.MVM_HeadLeft03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_HeadLeft03.mp3"
}

"Medic.MVM_HeadRight01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_HeadRight01.mp3"
}

"Medic.MVM_HeadRight02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_HeadRight02.mp3"
}

"Medic.MVM_HeadRight03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_HeadRight03.mp3"
}

"Medic.MVM_HelpMe01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_HelpMe01.mp3"
}

"Medic.MVM_HelpMe02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_HelpMe02.mp3"
}

"Medic.MVM_HelpMe03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_HelpMe03.mp3"
}

"Medic.MVM_HelpMeCapture01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_HelpMeCapture01.mp3"
}

"Medic.MVM_HelpMeCapture02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_HelpMeCapture02.mp3"
}

"Medic.MVM_HelpMeDefend01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_HelpMeDefend01.mp3"
}

"Medic.MVM_HelpMeDefend02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_HelpMeDefend02.mp3"
}

"Medic.MVM_HelpMeDefend03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_HelpMeDefend03.mp3"
}

"Medic.MVM_Incoming01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Incoming01.mp3"
}

"Medic.MVM_Incoming02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Incoming02.mp3"
}

"Medic.MVM_Incoming03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Incoming03.mp3"
}

"Medic.MVM_Jeers01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Jeers01.mp3"
}

"Medic.MVM_Jeers02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Jeers02.mp3"
}

"Medic.MVM_Jeers03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Jeers03.mp3"
}

"Medic.MVM_Jeers04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Jeers04.mp3"
}

"Medic.MVM_Jeers05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Jeers05.mp3"
}

"Medic.MVM_Jeers06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Jeers06.mp3"
}

"Medic.MVM_Jeers07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Jeers07.mp3"
}

"Medic.MVM_Jeers08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Jeers08.mp3"
}

"Medic.MVM_Jeers09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Jeers09.mp3"
}

"Medic.MVM_Jeers10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Jeers10.mp3"
}

"Medic.MVM_Jeers11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Jeers11.mp3"
}

"Medic.MVM_Jeers12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Jeers12.mp3"
}

"Medic.MVM_LaughEvil01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_LaughEvil01.mp3"
}

"Medic.MVM_LaughEvil02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_LaughEvil02.mp3"
}

"Medic.MVM_LaughEvil03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_LaughEvil03.mp3"
}

"Medic.MVM_LaughEvil04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_LaughEvil04.mp3"
}

"Medic.MVM_LaughEvil05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_LaughEvil05.mp3"
}

"Medic.MVM_LaughHappy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_LaughHappy01.mp3"
}

"Medic.MVM_LaughHappy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_LaughHappy02.mp3"
}

"Medic.MVM_LaughHappy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_LaughHappy03.mp3"
}

"Medic.MVM_LaughLong01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_LaughLong01.mp3"
}

"Medic.MVM_LaughLong02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_LaughLong02.mp3"
}

"Medic.MVM_LaughShort01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_LaughShort01.mp3"
}

"Medic.MVM_LaughShort02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_LaughShort02.mp3"
}

"Medic.MVM_LaughShort03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_LaughShort03.mp3"
}

"Medic.MVM_Medic01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Medic01.mp3"
}

"Medic.MVM_Medic02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Medic02.mp3"
}

"Medic.MVM_Medic03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Medic03.mp3"
}

"Medic.MVM_MoveUp01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_MoveUp01.mp3"
}

"Medic.MVM_MoveUp02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_MoveUp02.mp3"
}

"Medic.MVM_NeedDispenser01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_NeedDispenser01.mp3"
}

"Medic.MVM_NeedSentry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_NeedSentry01.mp3"
}

"Medic.MVM_NeedTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_NeedTeleporter01.mp3"
}

"Medic.MVM_NegativeVocalization01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_NegativeVocalization01.mp3"
}

"Medic.MVM_NegativeVocalization02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_NegativeVocalization02.mp3"
}

"Medic.MVM_NegativeVocalization03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_NegativeVocalization03.mp3"
}

"Medic.MVM_NegativeVocalization04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_NegativeVocalization04.mp3"
}

"Medic.MVM_NegativeVocalization05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_NegativeVocalization05.mp3"
}

"Medic.MVM_NegativeVocalization06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_NegativeVocalization06.mp3"
}

"Medic.MVM_NegativeVocalization07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_NegativeVocalization07.mp3"
}

"Medic.MVM_NiceShot01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_NiceShot01.mp3"
}

"Medic.MVM_NiceShot02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_NiceShot02.mp3"
}

"Medic.MVM_No01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_No01.mp3"
}

"Medic.MVM_No02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_No02.mp3"
}

"Medic.MVM_No03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_No03.mp3"
}

"Medic.MVM_PainCrticialDeath01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/medic_mvm_PainCrticialDeath01.mp3"
}

"Medic.MVM_PainCrticialDeath02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/medic_mvm_PainCrticialDeath02.mp3"
}

"Medic.MVM_PainCrticialDeath03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/medic_mvm_PainCrticialDeath03.mp3"
}

"Medic.MVM_PainCrticialDeath04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/medic_mvm_PainCrticialDeath04.mp3"
}

"Medic.MVM_PainSevere01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/medic_mvm_PainSevere01.mp3"
}

"Medic.MVM_PainSevere02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/medic_mvm_PainSevere02.mp3"
}

"Medic.MVM_PainSevere03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/medic_mvm_PainSevere03.mp3"
}

"Medic.MVM_PainSevere04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/medic_mvm_PainSevere04.mp3"
}

"Medic.MVM_PainSharp01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/medic_mvm_PainSharp01.mp3"
}

"Medic.MVM_PainSharp02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/medic_mvm_PainSharp02.mp3"
}

"Medic.MVM_PainSharp03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/medic_mvm_PainSharp03.mp3"
}

"Medic.MVM_PainSharp04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/medic_mvm_PainSharp04.mp3"
}

"Medic.MVM_PainSharp05"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/medic_mvm_PainSharp05.mp3"
}

"Medic.MVM_PainSharp06"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/medic_mvm_PainSharp06.mp3"
}

"Medic.MVM_PainSharp07"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/medic_mvm_PainSharp07.mp3"
}

"Medic.MVM_PainSharp08"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/medic_mvm_PainSharp08.mp3"
}

"Medic.MVM_PositiveVocalization01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_PositiveVocalization01.mp3"
}

"Medic.MVM_PositiveVocalization02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_PositiveVocalization02.mp3"
}

"Medic.MVM_PositiveVocalization03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_PositiveVocalization03.mp3"
}

"Medic.MVM_PositiveVocalization04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_PositiveVocalization04.mp3"
}

"Medic.MVM_PositiveVocalization05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_PositiveVocalization05.mp3"
}

"Medic.MVM_PositiveVocalization06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_PositiveVocalization06.mp3"
}

"Medic.MVM_SentryAhead01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_SentryAhead01.mp3"
}

"Medic.MVM_SentryAhead02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_SentryAhead02.mp3"
}

"Medic.MVM_SpecialCompleted-AssistedKill01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_SpecialCompleted-AssistedKill01.mp3"
}

"Medic.MVM_SpecialCompleted-AssistedKill02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_SpecialCompleted-AssistedKill02.mp3"
}

"Medic.MVM_SpecialCompleted01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_SpecialCompleted01.mp3"
}

"Medic.MVM_SpecialCompleted02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_SpecialCompleted02.mp3"
}

"Medic.MVM_SpecialCompleted03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_SpecialCompleted03.mp3"
}

"Medic.MVM_SpecialCompleted04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_SpecialCompleted04.mp3"
}

"Medic.MVM_SpecialCompleted05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_SpecialCompleted05.mp3"
}

"Medic.MVM_SpecialCompleted06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_SpecialCompleted06.mp3"
}

"Medic.MVM_SpecialCompleted07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_SpecialCompleted07.mp3"
}

"Medic.MVM_SpecialCompleted08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_SpecialCompleted08.mp3"
}

"Medic.MVM_SpecialCompleted09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_SpecialCompleted09.mp3"
}

"Medic.MVM_SpecialCompleted10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_SpecialCompleted10.mp3"
}

"Medic.MVM_SpecialCompleted11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_SpecialCompleted11.mp3"
}

"Medic.MVM_SpecialCompleted12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_SpecialCompleted12.mp3"
}

"Medic.MVM_StandOnThePoint01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_StandOnThePoint01.mp3"
}

"Medic.MVM_StandOnThePoint02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_StandOnThePoint02.mp3"
}

"Medic.MVM_StandOnThePoint03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_StandOnThePoint03.mp3"
}

"Medic.MVM_StandOnThePoint04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_StandOnThePoint04.mp3"
}

"Medic.MVM_StandOnThePoint05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_StandOnThePoint05.mp3"
}

"Medic.MVM_Taunts01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/taunts/medic_mvm_Taunts01.mp3"
}

"Medic.MVM_Taunts02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/medic_mvm_Taunts02.mp3"
}

"Medic.MVM_Taunts03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/medic_mvm_Taunts03.mp3"
}

"Medic.MVM_Taunts04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/medic_mvm_Taunts04.mp3"
}

"Medic.MVM_Taunts05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/medic_mvm_Taunts05.mp3"
}

"Medic.MVM_Taunts06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/medic_mvm_Taunts06.mp3"
}

"Medic.MVM_Taunts07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/medic_mvm_Taunts07.mp3"
}

"Medic.MVM_Taunts08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/medic_mvm_Taunts08.mp3"
}

"Medic.MVM_Taunts09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/medic_mvm_Taunts09.mp3"
}

"Medic.MVM_Taunts10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/medic_mvm_Taunts10.mp3"
}

"Medic.MVM_Taunts11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/medic_mvm_Taunts11.mp3"
}

"Medic.MVM_Taunts12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/medic_mvm_Taunts12.mp3"
}

"Medic.MVM_Taunts13"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/medic_mvm_Taunts13.mp3"
}

"Medic.MVM_Taunts14"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/medic_mvm_Taunts14.mp3"
}

"Medic.MVM_Taunts15"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/medic_mvm_Taunts15.mp3"
}

"Medic.MVM_Thanks01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Thanks01.mp3"
}

"Medic.MVM_Thanks02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Thanks02.mp3"
}

"Medic.MVM_ThanksForTheHeal01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_ThanksForTheHeal01.mp3"
}

"Medic.MVM_ThanksForTheHeal02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_ThanksForTheHeal02.mp3"
}

"Medic.MVM_ThanksForTheTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_ThanksForTheTeleporter01.mp3"
}

"Medic.MVM_ThanksForTheTeleporter02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_ThanksForTheTeleporter02.mp3"
}

"Medic.MVM_ThanksForTheTeleporter03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_ThanksForTheTeleporter03.mp3"
}

"Medic.MVM_Yes01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Yes01.mp3"
}

"Medic.MVM_Yes02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Yes02.mp3"
}

"Medic.MVM_Yes03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/medic_mvm_Yes03.mp3"
}

"Pyro.MVM_ActivateCharge01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_ActivateCharge01.mp3"
}

"Pyro.MVM_AutoCappedControlPoint01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_AutoCappedControlPoint01.mp3"
}

"Pyro.MVM_AutoCappedIntelligence01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_AutoCappedIntelligence01.mp3"
}

"Pyro.MVM_AutoDejectedTie01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_AutoDejectedTie01.mp3"
}

"Pyro.MVM_AutoOnFire01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_AutoOnFire01.mp3"
}

"Pyro.MVM_AutoOnFire02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_AutoOnFire02.mp3"
}

"Pyro.MVM_BattleCry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_BattleCry01.mp3"
}

"Pyro.MVM_BattleCry02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_BattleCry02.mp3"
}

"Pyro.MVM_Cheers01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_Cheers01.mp3"
}

"Pyro.MVM_CloakedSpy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_CloakedSpy01.mp3"
}

"Pyro.MVM_CloakedSpyIdentify01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_CloakedSpyIdentify01.mp3"
}

"Pyro.MVM_Go01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_Go01.mp3"
}

"Pyro.MVM_GoodJob01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_GoodJob01.mp3"
}

"Pyro.MVM_HeadLeft01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_HeadLeft01.mp3"
}

"Pyro.MVM_HeadRight01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_HeadRight01.mp3"
}

"Pyro.MVM_HelpMe01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_HelpMe01.mp3"
}

"Pyro.MVM_HelpMeCapture01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_HelpMeCapture01.mp3"
}

"Pyro.MVM_HelpMeDefend01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_HelpMeDefend01.mp3"
}

"Pyro.MVM_Incoming01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_Incoming01.mp3"
}

"Pyro.MVM_Jeers01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_Jeers01.mp3"
}

"Pyro.MVM_Jeers02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_Jeers02.mp3"
}

"Pyro.MVM_LaughEvil01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_LaughEvil01.mp3"
}

"Pyro.MVM_LaughEvil02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_LaughEvil02.mp3"
}

"Pyro.MVM_LaughEvil03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_LaughEvil03.mp3"
}

"Pyro.MVM_LaughEvil04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_LaughEvil04.mp3"
}

"Pyro.MVM_LaughHappy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_LaughHappy01.mp3"
}

"Pyro.MVM_LaughLong01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_LaughLong01.mp3"
}

"Pyro.MVM_LaughShort01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_LaughShort01.mp3"
}

"Pyro.MVM_laugh_addl04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_laugh_addl04.mp3"
}

"Pyro.MVM_Medic01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_Medic01.mp3"
}

"Pyro.MVM_MoveUp01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_MoveUp01.mp3"
}

"Pyro.MVM_NeedDispenser01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_NeedDispenser01.mp3"
}

"Pyro.MVM_NeedSentry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_NeedSentry01.mp3"
}

"Pyro.MVM_NeedTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_NeedTeleporter01.mp3"
}

"Pyro.MVM_NegativeVocalization01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_NegativeVocalization01.mp3"
}

"Pyro.MVM_NiceShot01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_NiceShot01.mp3"
}

"Pyro.MVM_No01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_No01.mp3"
}

"Pyro.MVM_PainCrticialDeath01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/pyro_mvm_PainCrticialDeath01.mp3"
}

"Pyro.MVM_PainCrticialDeath02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/pyro_mvm_PainCrticialDeath02.mp3"
}

"Pyro.MVM_PainCrticialDeath03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/pyro_mvm_PainCrticialDeath03.mp3"
}

"Pyro.MVM_PainSevere01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/pyro_mvm_PainSevere01.mp3"
}

"Pyro.MVM_PainSevere02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/pyro_mvm_PainSevere02.mp3"
}

"Pyro.MVM_PainSevere03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/pyro_mvm_PainSevere03.mp3"
}

"Pyro.MVM_PainSevere04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/pyro_mvm_PainSevere04.mp3"
}

"Pyro.MVM_PainSevere05"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/pyro_mvm_PainSevere05.mp3"
}

"Pyro.MVM_PainSevere06"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/pyro_mvm_PainSevere06.mp3"
}

"Pyro.MVM_PainSharp01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/pyro_mvm_PainSharp01.mp3"
}

"Pyro.MVM_PainSharp02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/pyro_mvm_PainSharp02.mp3"
}

"Pyro.MVM_PainSharp03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/pyro_mvm_PainSharp03.mp3"
}

"Pyro.MVM_PainSharp04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/pyro_mvm_PainSharp04.mp3"
}

"Pyro.MVM_PainSharp05"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/pyro_mvm_PainSharp05.mp3"
}

"Pyro.MVM_PainSharp06"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/pyro_mvm_PainSharp06.mp3"
}

"Pyro.MVM_PainSharp07"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/pyro_mvm_PainSharp07.mp3"
}

"Pyro.MVM_PositiveVocalization01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_PositiveVocalization01.mp3"
}

"Pyro.MVM_SentryAhead01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_SentryAhead01.mp3"
}

"Pyro.MVM_SpecialCompleted-AssistedKill01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_SpecialCompleted-AssistedKill01.mp3"
}

"Pyro.MVM_SpecialCompleted01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_SpecialCompleted01.mp3"
}

"Pyro.MVM_StandOnThePoint01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_StandOnThePoint01.mp3"
}

"Pyro.MVM_Taunts01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/pyro_mvm_Taunts01.mp3"
}

"Pyro.MVM_Taunts02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/pyro_mvm_Taunts02.mp3"
}

"Pyro.MVM_Taunts03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/pyro_mvm_Taunts03.mp3"
}

"Pyro.MVM_Taunts04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/pyro_mvm_Taunts04.mp3"
}

"Pyro.MVM_Thanks01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_Thanks01.mp3"
}

"Pyro.MVM_ThanksForTheHeal01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_ThanksForTheHeal01.mp3"
}

"Pyro.MVM_ThanksForTheTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_ThanksForTheTeleporter01.mp3"
}

"Pyro.MVM_Yes01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/pyro_mvm_Yes01.mp3"
}

"Pyro.MVM_HighFiveSuccess01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/pyro_mvm_highfive_success01.mp3"
}

"Pyro.MVM_HighFiveSuccess02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/pyro_mvm_highfive_success02.mp3"
}

"Pyro.MVM_HighFiveSuccess03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/pyro_mvm_highfive_success03.mp3"
}

"Pyro.MVM_HighFive01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/pyro_mvm_highfive01.mp3"
}

"Pyro.MVM_HighFive02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/pyro_mvm_highfive02.mp3"
}

"Scout.MVM_ActivateCharge01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_ActivateCharge01.mp3"
}

"Scout.MVM_ActivateCharge02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_ActivateCharge02.mp3"
}

"Scout.MVM_ActivateCharge03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_ActivateCharge03.mp3"
}

"Scout.MVM_AutoCappedControlPoint01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_AutoCappedControlPoint01.mp3"
}

"Scout.MVM_AutoCappedControlPoint02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_AutoCappedControlPoint02.mp3"
}

"Scout.MVM_AutoCappedControlPoint03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_AutoCappedControlPoint03.mp3"
}

"Scout.MVM_AutoCappedControlPoint04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_AutoCappedControlPoint04.mp3"
}

"Scout.MVM_AutoCappedIntelligence01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_AutoCappedIntelligence01.mp3"
}

"Scout.MVM_AutoCappedIntelligence02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_AutoCappedIntelligence02.mp3"
}

"Scout.MVM_AutoCappedIntelligence03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_AutoCappedIntelligence03.mp3"
}

"Scout.MVM_AutoDejectedTie01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_AutoDejectedTie01.mp3"
}

"Scout.MVM_AutoDejectedTie02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_AutoDejectedTie02.mp3"
}

"Scout.MVM_AutoDejectedTie03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_AutoDejectedTie03.mp3"
}

"Scout.MVM_AutoDejectedTie04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_AutoDejectedTie04.mp3"
}

"Scout.MVM_AutoOnFire01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_AutoOnFire01.mp3"
}

"Scout.MVM_AutoOnFire02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_AutoOnFire02.mp3"
}

"Scout.MVM_BattleCry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_BattleCry01.mp3"
}

"Scout.MVM_BattleCry02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_BattleCry02.mp3"
}

"Scout.MVM_BattleCry03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_BattleCry03.mp3"
}

"Scout.MVM_BattleCry04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_BattleCry04.mp3"
}

"Scout.MVM_BattleCry05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_BattleCry05.mp3"
}

"Scout.MVM_Cheers01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Cheers01.mp3"
}

"Scout.MVM_Cheers02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Cheers02.mp3"
}

"Scout.MVM_Cheers03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Cheers03.mp3"
}

"Scout.MVM_Cheers04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Cheers04.mp3"
}

"Scout.MVM_Cheers05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Cheers05.mp3"
}

"Scout.MVM_Cheers06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Cheers06.mp3"
}

"Scout.MVM_CloakedSpy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_CloakedSpy01.mp3"
}

"Scout.MVM_CloakedSpy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_CloakedSpy02.mp3"
}

"Scout.MVM_CloakedSpy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_CloakedSpy03.mp3"
}

"Scout.MVM_CloakedSpy04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_CloakedSpy04.mp3"
}

"Scout.MVM_CloakedSpyIdentify01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_CloakedSpyIdentify01.mp3"
}

"Scout.MVM_CloakedSpyIdentify02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_CloakedSpyIdentify02.mp3"
}

"Scout.MVM_CloakedSpyIdentify03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_CloakedSpyIdentify03.mp3"
}

"Scout.MVM_CloakedSpyIdentify04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_CloakedSpyIdentify04.mp3"
}

"Scout.MVM_CloakedSpyIdentify05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_CloakedSpyIdentify05.mp3"
}

"Scout.MVM_CloakedSpyIdentify06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_CloakedSpyIdentify06.mp3"
}

"Scout.MVM_CloakedSpyIdentify07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_CloakedSpyIdentify07.mp3"
}

"Scout.MVM_CloakedSpyIdentify08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_CloakedSpyIdentify08.mp3"
}

"Scout.MVM_CloakedSpyIdentify09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_CloakedSpyIdentify09.mp3"
}

"Scout.MVM_Generic01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Generic01.mp3"
}

"Scout.MVM_Go01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Go01.mp3"
}

"Scout.MVM_Go02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Go02.mp3"
}

"Scout.MVM_Go03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Go03.mp3"
}

"Scout.MVM_Go04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Go04.mp3"
}

"Scout.MVM_GoodJob01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_GoodJob01.mp3"
}

"Scout.MVM_GoodJob02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_GoodJob02.mp3"
}

"Scout.MVM_GoodJob03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_GoodJob03.mp3"
}

"Scout.MVM_GoodJob04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_GoodJob04.mp3"
}

"Scout.MVM_HeadLeft01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_HeadLeft01.mp3"
}

"Scout.MVM_HeadLeft02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_HeadLeft02.mp3"
}

"Scout.MVM_HeadLeft03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_HeadLeft03.mp3"
}

"Scout.MVM_HeadRight01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_HeadRight01.mp3"
}

"Scout.MVM_HeadRight02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_HeadRight02.mp3"
}

"Scout.MVM_HeadRight03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_HeadRight03.mp3"
}

"Scout.MVM_HelpMe01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_HelpMe01.mp3"
}

"Scout.MVM_HelpMe02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_HelpMe02.mp3"
}

"Scout.MVM_HelpMe03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_HelpMe03.mp3"
}

"Scout.MVM_HelpMe04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_HelpMe04.mp3"
}

"Scout.MVM_HelpMeCapture01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_HelpMeCapture01.mp3"
}

"Scout.MVM_HelpMeCapture02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_HelpMeCapture02.mp3"
}

"Scout.MVM_HelpMeCapture03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_HelpMeCapture03.mp3"
}

"Scout.MVM_HelpMeDefend01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_HelpMeDefend01.mp3"
}

"Scout.MVM_HelpMeDefend02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_HelpMeDefend02.mp3"
}

"Scout.MVM_HelpMeDefend03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_HelpMeDefend03.mp3"
}

"Scout.MVM_Incoming01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Incoming01.mp3"
}

"Scout.MVM_Incoming02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Incoming02.mp3"
}

"Scout.MVM_Incoming03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Incoming03.mp3"
}

"Scout.MVM_Jeers02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Jeers02.mp3"
}

"Scout.MVM_Jeers03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Jeers03.mp3"
}

"Scout.MVM_Jeers04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Jeers04.mp3"
}

"Scout.MVM_Jeers05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Jeers05.mp3"
}

"Scout.MVM_Jeers06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Jeers06.mp3"
}

"Scout.MVM_Jeers07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Jeers07.mp3"
}

"Scout.MVM_Jeers08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Jeers08.mp3"
}

"Scout.MVM_Jeers09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Jeers09.mp3"
}

"Scout.MVM_Jeers10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Jeers10.mp3"
}

"Scout.MVM_Jeers11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Jeers11.mp3"
}

"Scout.MVM_Jeers12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Jeers12.mp3"
}

"Scout.MVM_LaughEvil01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_LaughEvil01.mp3"
}

"Scout.MVM_LaughEvil02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_LaughEvil02.mp3"
}

"Scout.MVM_LaughEvil03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_LaughEvil03.mp3"
}

"Scout.MVM_LaughHappy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_LaughHappy01.mp3"
}

"Scout.MVM_LaughHappy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_LaughHappy02.mp3"
}

"Scout.MVM_LaughHappy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_LaughHappy03.mp3"
}

"Scout.MVM_LaughHappy04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_LaughHappy04.mp3"
}

"Scout.MVM_LaughLong01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_LaughLong01.mp3"
}

"Scout.MVM_LaughLong02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_LaughLong02.mp3"
}

"Scout.MVM_LaughShort01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_LaughShort01.mp3"
}

"Scout.MVM_LaughShort02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_LaughShort02.mp3"
}

"Scout.MVM_LaughShort03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_LaughShort03.mp3"
}

"Scout.MVM_LaughShort04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_LaughShort04.mp3"
}

"Scout.MVM_LaughShort05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_LaughShort05.mp3"
}

"Scout.MVM_Medic01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Medic01.mp3"
}

"Scout.MVM_Medic02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Medic02.mp3"
}

"Scout.MVM_Medic03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Medic03.mp3"
}

"Scout.MVM_MoveUp01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_MoveUp01.mp3"
}

"Scout.MVM_MoveUp02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_MoveUp02.mp3"
}

"Scout.MVM_MoveUp03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_MoveUp03.mp3"
}

"Scout.MVM_NeedDispenser01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_NeedDispenser01.mp3"
}

"Scout.MVM_NeedSentry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_NeedSentry01.mp3"
}

"Scout.MVM_NeedTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_NeedTeleporter01.mp3"
}

"Scout.MVM_NegativeVocalization01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_NegativeVocalization01.mp3"
}

"Scout.MVM_NegativeVocalization02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_NegativeVocalization02.mp3"
}

"Scout.MVM_NegativeVocalization03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_NegativeVocalization03.mp3"
}

"Scout.MVM_NegativeVocalization04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"620.000"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_NegativeVocalization04.mp3"
}

"Scout.MVM_NegativeVocalization05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_NegativeVocalization05.mp3"
}

"Scout.MVM_NiceShot01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_NiceShot01.mp3"
}

"Scout.MVM_NiceShot02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_NiceShot02.mp3"
}

"Scout.MVM_NiceShot03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_NiceShot03.mp3"
}

"Scout.MVM_No01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_No01.mp3"
}

"Scout.MVM_No02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_No02.mp3"
}

"Scout.MVM_No03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_No03.mp3"
}

"Scout.MVM_PainCrticialDeath01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/scout_mvm_PainCrticialDeath01.mp3"
}

"Scout.MVM_PainCrticialDeath02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/scout_mvm_PainCrticialDeath02.mp3"
}

"Scout.MVM_PainCrticialDeath03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/scout_mvm_PainCrticialDeath03.mp3"
}

"Scout.MVM_PainSevere01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/scout_mvm_PainSevere01.mp3"
}

"Scout.MVM_PainSevere02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/scout_mvm_PainSevere02.mp3"
}

"Scout.MVM_PainSevere03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/scout_mvm_PainSevere03.mp3"
}

"Scout.MVM_PainSevere04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/scout_mvm_PainSevere04.mp3"
}

"Scout.MVM_PainSevere05"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/scout_mvm_PainSevere05.mp3"
}

"Scout.MVM_PainSevere06"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/scout_mvm_PainSevere06.mp3"
}

"Scout.MVM_PainSharp01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/scout_mvm_PainSharp01.mp3"
}

"Scout.MVM_PainSharp02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/scout_mvm_PainSharp02.mp3"
}

"Scout.MVM_PainSharp03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/scout_mvm_PainSharp03.mp3"
}

"Scout.MVM_PainSharp04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/scout_mvm_PainSharp04.mp3"
}

"Scout.MVM_PainSharp05"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/scout_mvm_PainSharp05.mp3"
}

"Scout.MVM_PainSharp06"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/scout_mvm_PainSharp06.mp3"
}

"Scout.MVM_PainSharp07"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/scout_mvm_PainSharp07.mp3"
}

"Scout.MVM_PainSharp08"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/scout_mvm_PainSharp08.mp3"
}

"Scout.MVM_PositiveVocalization01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_PositiveVocalization01.mp3"
}

"Scout.MVM_PositiveVocalization02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_PositiveVocalization02.mp3"
}

"Scout.MVM_PositiveVocalization03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_PositiveVocalization03.mp3"
}

"Scout.MVM_PositiveVocalization04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_PositiveVocalization04.mp3"
}

"Scout.MVM_PositiveVocalization05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_PositiveVocalization05.mp3"
}

"Scout.MVM_SentryAhead01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_SentryAhead01.mp3"
}

"Scout.MVM_SentryAhead02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_SentryAhead02.mp3"
}

"Scout.MVM_SentryAhead03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_SentryAhead03.mp3"
}

"Scout.MVM_SpecialCompleted-AssistedKill01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_SpecialCompleted-AssistedKill01.mp3"
}

"Scout.MVM_SpecialCompleted-AssistedKill02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_SpecialCompleted-AssistedKill02.mp3"
}

"Scout.MVM_SpecialCompleted01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_SpecialCompleted01.mp3"
}

"Scout.MVM_SpecialCompleted02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_SpecialCompleted02.mp3"
}

"Scout.MVM_SpecialCompleted03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_SpecialCompleted03.mp3"
}

"Scout.MVM_SpecialCompleted04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_SpecialCompleted04.mp3"
}

"Scout.MVM_SpecialCompleted05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_SpecialCompleted05.mp3"
}

"Scout.MVM_SpecialCompleted06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_SpecialCompleted06.mp3"
}

"Scout.MVM_SpecialCompleted07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_SpecialCompleted07.mp3"
}

"Scout.MVM_SpecialCompleted09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_SpecialCompleted09.mp3"
}

"Scout.MVM_SpecialCompleted10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_SpecialCompleted10.mp3"
}

"Scout.MVM_SpecialCompleted11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_SpecialCompleted11.mp3"
}

"Scout.MVM_SpecialCompleted12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_SpecialCompleted12.mp3"
}

"Scout.MVM_StandOnThePoint01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_StandOnThePoint01.mp3"
}

"Scout.MVM_StandOnThePoint02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_StandOnThePoint02.mp3"
}

"Scout.MVM_StandOnThePoint03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_StandOnThePoint03.mp3"
}

"Scout.MVM_StandOnThePoint04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_StandOnThePoint04.mp3"
}

"Scout.MVM_StandOnThePoint05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_StandOnThePoint05.mp3"
}

"Scout.MVM_Taunts01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/scout_mvm_Taunts01.mp3"
}

"Scout.MVM_Taunts02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/scout_mvm_Taunts02.mp3"
}

"Scout.MVM_Taunts03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/scout_mvm_Taunts03.mp3"
}

"Scout.MVM_Taunts04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/scout_mvm_Taunts04.mp3"
}

"Scout.MVM_Taunts05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/scout_mvm_Taunts05.mp3"
}

"Scout.MVM_Taunts06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/scout_mvm_Taunts06.mp3"
}

"Scout.MVM_Taunts07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/scout_mvm_Taunts07.mp3"
}

"Scout.MVM_Taunts08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/scout_mvm_Taunts08.mp3"
}

"Scout.MVM_Taunts09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/scout_mvm_Taunts09.mp3"
}

"Scout.MVM_Taunts10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/scout_mvm_Taunts10.mp3"
}

"Scout.MVM_Taunts11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/scout_mvm_Taunts11.mp3"
}

"Scout.MVM_Taunts12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/scout_mvm_Taunts12.mp3"
}

"Scout.MVM_Taunts13"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/scout_mvm_Taunts13.mp3"
}

"Scout.MVM_Taunts14"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/scout_mvm_Taunts14.mp3"
}

"Scout.MVM_Taunts15"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/scout_mvm_Taunts15.mp3"
}

"Scout.MVM_Taunts16"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/scout_mvm_Taunts16.mp3"
}

"Scout.MVM_Taunts17"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/scout_mvm_Taunts17.mp3"
}

"Scout.MVM_Taunts18"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/scout_mvm_Taunts18.mp3"
}

"Scout.MVM_Thanks01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Thanks01.mp3"
}

"Scout.MVM_Thanks02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Thanks02.mp3"
}

"Scout.MVM_ThanksForTheHeal01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_ThanksForTheHeal01.mp3"
}

"Scout.MVM_ThanksForTheHeal02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_ThanksForTheHeal02.mp3"
}

"Scout.MVM_ThanksForTheHeal03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_ThanksForTheHeal03.mp3"
}

"Scout.MVM_ThanksForTheTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_ThanksForTheTeleporter01.mp3"
}

"Scout.MVM_ThanksForTheTeleporter02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_ThanksForTheTeleporter02.mp3"
}

"Scout.MVM_ThanksForTheTeleporter03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_ThanksForTheTeleporter03.mp3"
}

"Scout.MVM_Yes01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Yes01.mp3"
}

"Scout.MVM_Yes02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Yes02.mp3"
}

"Scout.MVM_Yes03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/scout_mvm_Yes03.mp3"
}

"Sniper.MVM_ActivateCharge01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_ActivateCharge01.mp3"
}

"Sniper.MVM_ActivateCharge02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_ActivateCharge02.mp3"
}

"Sniper.MVM_ActivateCharge03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_ActivateCharge03.mp3"
}

"Sniper.MVM_ActivateCharge04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_ActivateCharge04.mp3"
}

"Sniper.MVM_AutoCappedControlPoint01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_AutoCappedControlPoint01.mp3"
}

"Sniper.MVM_AutoCappedControlPoint02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_AutoCappedControlPoint02.mp3"
}

"Sniper.MVM_AutoCappedControlPoint03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_AutoCappedControlPoint03.mp3"
}

"Sniper.MVM_AutoCappedIntelligence01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_AutoCappedIntelligence01.mp3"
}

"Sniper.MVM_AutoCappedIntelligence02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_AutoCappedIntelligence02.mp3"
}

"Sniper.MVM_AutoCappedIntelligence03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_AutoCappedIntelligence03.mp3"
}

"Sniper.MVM_AutoCappedIntelligence04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_AutoCappedIntelligence04.mp3"
}

"Sniper.MVM_AutoCappedIntelligence05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_AutoCappedIntelligence05.mp3"
}

"Sniper.MVM_AutoDejectedTie01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_AutoDejectedTie01.mp3"
}

"Sniper.MVM_AutoDejectedTie02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_AutoDejectedTie02.mp3"
}

"Sniper.MVM_AutoDejectedTie03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_AutoDejectedTie03.mp3"
}

"Sniper.MVM_AutoOnFire01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_AutoOnFire01.mp3"
}

"Sniper.MVM_AutoOnFire02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_AutoOnFire02.mp3"
}

"Sniper.MVM_AutoOnFire03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_AutoOnFire03.mp3"
}

"Sniper.MVM_BattleCry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_BattleCry01.mp3"
}

"Sniper.MVM_BattleCry02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_BattleCry02.mp3"
}

"Sniper.MVM_BattleCry03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_BattleCry03.mp3"
}

"Sniper.MVM_BattleCry04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_BattleCry04.mp3"
}

"Sniper.MVM_BattleCry05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_BattleCry05.mp3"
}

"Sniper.MVM_BattleCry06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_BattleCry06.mp3"
}

"Sniper.MVM_Cheers01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Cheers01.mp3"
}

"Sniper.MVM_Cheers02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Cheers02.mp3"
}

"Sniper.MVM_Cheers03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Cheers03.mp3"
}

"Sniper.MVM_Cheers04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Cheers04.mp3"
}

"Sniper.MVM_Cheers05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Cheers05.mp3"
}

"Sniper.MVM_Cheers06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Cheers06.mp3"
}

"Sniper.MVM_Cheers07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Cheers07.mp3"
}

"Sniper.MVM_Cheers08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Cheers08.mp3"
}

"Sniper.MVM_CloakedSpy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_CloakedSpy01.mp3"
}

"Sniper.MVM_CloakedSpy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_CloakedSpy02.mp3"
}

"Sniper.MVM_CloakedSpy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_CloakedSpy03.mp3"
}

"Sniper.MVM_CloakedSpyIdentify01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_CloakedSpyIdentify01.mp3"
}

"Sniper.MVM_CloakedSpyIdentify02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_CloakedSpyIdentify02.mp3"
}

"Sniper.MVM_CloakedSpyIdentify03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_CloakedSpyIdentify03.mp3"
}

"Sniper.MVM_CloakedSpyIdentify04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_CloakedSpyIdentify04.mp3"
}

"Sniper.MVM_CloakedSpyIdentify05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_CloakedSpyIdentify05.mp3"
}

"Sniper.MVM_CloakedSpyIdentify06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_CloakedSpyIdentify06.mp3"
}

"Sniper.MVM_CloakedSpyIdentify07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_CloakedSpyIdentify07.mp3"
}

"Sniper.MVM_CloakedSpyIdentify08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_CloakedSpyIdentify08.mp3"
}

"Sniper.MVM_CloakedSpyIdentify09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_CloakedSpyIdentify09.mp3"
}

"Sniper.MVM_Go01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Go01.mp3"
}

"Sniper.MVM_Go02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Go02.mp3"
}

"Sniper.MVM_Go03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Go03.mp3"
}

"Sniper.MVM_GoodJob01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_GoodJob01.mp3"
}

"Sniper.MVM_GoodJob02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_GoodJob02.mp3"
}

"Sniper.MVM_GoodJob03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_GoodJob03.mp3"
}

"Sniper.MVM_HeadLeft01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_HeadLeft01.mp3"
}

"Sniper.MVM_HeadLeft02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_HeadLeft02.mp3"
}

"Sniper.MVM_HeadLeft03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_HeadLeft03.mp3"
}

"Sniper.MVM_HeadRight01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_HeadRight01.mp3"
}

"Sniper.MVM_HeadRight02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_HeadRight02.mp3"
}

"Sniper.MVM_HeadRight03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_HeadRight03.mp3"
}

"Sniper.MVM_HelpMe01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_HelpMe01.mp3"
}

"Sniper.MVM_HelpMe02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_HelpMe02.mp3"
}

"Sniper.MVM_HelpMe03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_HelpMe03.mp3"
}

"Sniper.MVM_HelpMeCapture01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_HelpMeCapture01.mp3"
}

"Sniper.MVM_HelpMeCapture02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_HelpMeCapture02.mp3"
}

"Sniper.MVM_HelpMeCapture03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_HelpMeCapture03.mp3"
}

"Sniper.MVM_HelpMeDefend01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_HelpMeDefend01.mp3"
}

"Sniper.MVM_HelpMeDefend02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_HelpMeDefend02.mp3"
}

"Sniper.MVM_HelpMeDefend03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_HelpMeDefend03.mp3"
}

"Sniper.MVM_Incoming01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Incoming01.mp3"
}

"Sniper.MVM_Incoming02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Incoming02.mp3"
}

"Sniper.MVM_Incoming03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Incoming03.mp3"
}

"Sniper.MVM_Incoming04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Incoming04.mp3"
}

"Sniper.MVM_Jeers01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Jeers01.mp3"
}

"Sniper.MVM_Jeers02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Jeers02.mp3"
}

"Sniper.MVM_Jeers03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Jeers03.mp3"
}

"Sniper.MVM_Jeers04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Jeers04.mp3"
}

"Sniper.MVM_Jeers05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Jeers05.mp3"
}

"Sniper.MVM_Jeers06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Jeers06.mp3"
}

"Sniper.MVM_Jeers07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Jeers07.mp3"
}

"Sniper.MVM_Jeers08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Jeers08.mp3"
}

"Sniper.MVM_LaughEvil01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_LaughEvil01.mp3"
}

"Sniper.MVM_LaughEvil02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_LaughEvil02.mp3"
}

"Sniper.MVM_LaughEvil03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_LaughEvil03.mp3"
}

"Sniper.MVM_LaughHappy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_LaughHappy01.mp3"
}

"Sniper.MVM_LaughHappy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_LaughHappy02.mp3"
}

"Sniper.MVM_LaughLong01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_LaughLong01.mp3"
}

"Sniper.MVM_LaughLong02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_LaughLong02.mp3"
}

"Sniper.MVM_LaughShort01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_LaughShort01.mp3"
}

"Sniper.MVM_LaughShort02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_LaughShort02.mp3"
}

"Sniper.MVM_LaughShort03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_LaughShort03.mp3"
}

"Sniper.MVM_LaughShort04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_LaughShort04.mp3"
}

"Sniper.MVM_LaughShort05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_LaughShort05.mp3"
}

"Sniper.MVM_Medic01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Medic01.mp3"
}

"Sniper.MVM_Medic02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Medic02.mp3"
}

"Sniper.MVM_MoveUp01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_MoveUp01.mp3"
}

"Sniper.MVM_MoveUp02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_MoveUp02.mp3"
}

"Sniper.MVM_NeedDispenser01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_NeedDispenser01.mp3"
}

"Sniper.MVM_NeedSentry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_NeedSentry01.mp3"
}

"Sniper.MVM_NeedTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_NeedTeleporter01.mp3"
}

"Sniper.MVM_NegativeVocalization01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_NegativeVocalization01.mp3"
}

"Sniper.MVM_NegativeVocalization02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_NegativeVocalization02.mp3"
}

"Sniper.MVM_NegativeVocalization03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_NegativeVocalization03.mp3"
}

"Sniper.MVM_NegativeVocalization04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_NegativeVocalization04.mp3"
}

"Sniper.MVM_NegativeVocalization05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_NegativeVocalization05.mp3"
}

"Sniper.MVM_NegativeVocalization06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_NegativeVocalization06.mp3"
}

"Sniper.MVM_NegativeVocalization07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_NegativeVocalization07.mp3"
}

"Sniper.MVM_NegativeVocalization08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_NegativeVocalization08.mp3"
}

"Sniper.MVM_NegativeVocalization09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_NegativeVocalization09.mp3"
}

"Sniper.MVM_NiceShot01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_NiceShot01.mp3"
}

"Sniper.MVM_NiceShot02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_NiceShot02.mp3"
}

"Sniper.MVM_NiceShot03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_NiceShot03.mp3"
}

"Sniper.MVM_No01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_No01.mp3"
}

"Sniper.MVM_No02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_No02.mp3"
}

"Sniper.MVM_No03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_No03.mp3"
}

"Sniper.MVM_No04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_No04.mp3"
}

"Sniper.MVM_PainCrticialDeath01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PainCrticialDeath01.mp3"
}

"Sniper.MVM_PainCrticialDeath02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PainCrticialDeath02.mp3"
}

"Sniper.MVM_PainCrticialDeath03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PainCrticialDeath03.mp3"
}

"Sniper.MVM_PainCrticialDeath04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PainCrticialDeath04.mp3"
}

"Sniper.MVM_PainSevere01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PainSevere01.mp3"
}

"Sniper.MVM_PainSevere02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PainSevere02.mp3"
}

"Sniper.MVM_PainSevere03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PainSevere03.mp3"
}

"Sniper.MVM_PainSevere04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PainSevere04.mp3"
}

"Sniper.MVM_PainSharp01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PainSharp01.mp3"
}

"Sniper.MVM_PainSharp02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PainSharp02.mp3"
}

"Sniper.MVM_PainSharp03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PainSharp03.mp3"
}

"Sniper.MVM_PainSharp04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PainSharp04.mp3"
}

"Sniper.MVM_PositiveVocalization01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PositiveVocalization01.mp3"
}

"Sniper.MVM_PositiveVocalization02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PositiveVocalization02.mp3"
}

"Sniper.MVM_PositiveVocalization03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PositiveVocalization03.mp3"
}

"Sniper.MVM_PositiveVocalization04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PositiveVocalization04.mp3"
}

"Sniper.MVM_PositiveVocalization05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PositiveVocalization05.mp3"
}

"Sniper.MVM_PositiveVocalization06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PositiveVocalization06.mp3"
}

"Sniper.MVM_PositiveVocalization07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PositiveVocalization07.mp3"
}

"Sniper.MVM_PositiveVocalization08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PositiveVocalization08.mp3"
}

"Sniper.MVM_PositiveVocalization09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PositiveVocalization09.mp3"
}

"Sniper.MVM_PositiveVocalization10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_PositiveVocalization10.mp3"
}

"Sniper.MVM_SentryAhead01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SentryAhead01.mp3"
}

"Sniper.MVM_SpecialCompleted-AssistedKill01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted-AssistedKill01.mp3"
}

"Sniper.MVM_SpecialCompleted-AssistedKill02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted-AssistedKill02.mp3"
}

"Sniper.MVM_SpecialCompleted01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted01.mp3"
}

"Sniper.MVM_SpecialCompleted02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted02.mp3"
}

"Sniper.MVM_SpecialCompleted03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted03.mp3"
}

"Sniper.MVM_SpecialCompleted04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted04.mp3"
}

"Sniper.MVM_SpecialCompleted05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted05.mp3"
}

"Sniper.MVM_SpecialCompleted06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted06.mp3"
}

"Sniper.MVM_SpecialCompleted07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted07.mp3"
}

"Sniper.MVM_SpecialCompleted08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted08.mp3"
}

"Sniper.MVM_SpecialCompleted09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted09.mp3"
}

"Sniper.MVM_SpecialCompleted10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted10.mp3"
}

"Sniper.MVM_SpecialCompleted11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted11.mp3"
}

"Sniper.MVM_SpecialCompleted12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted12.mp3"
}

"Sniper.MVM_SpecialCompleted13"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted13.mp3"
}

"Sniper.MVM_SpecialCompleted14"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted14.mp3"
}

"Sniper.MVM_SpecialCompleted15"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted15.mp3"
}

"Sniper.MVM_SpecialCompleted16"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted16.mp3"
}

"Sniper.MVM_SpecialCompleted17"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted17.mp3"
}

"Sniper.MVM_SpecialCompleted18"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted18.mp3"
}

"Sniper.MVM_SpecialCompleted19"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted19.mp3"
}

"Sniper.MVM_SpecialCompleted20"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted20.mp3"
}

"Sniper.MVM_SpecialCompleted21"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted21.mp3"
}

"Sniper.MVM_SpecialCompleted22"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted22.mp3"
}

"Sniper.MVM_SpecialCompleted23"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted23.mp3"
}

"Sniper.MVM_SpecialCompleted24"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted24.mp3"
}

"Sniper.MVM_SpecialCompleted25"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted25.mp3"
}

"Sniper.MVM_SpecialCompleted26"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted26.mp3"
}

"Sniper.MVM_SpecialCompleted27"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted27.mp3"
}

"Sniper.MVM_SpecialCompleted28"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted28.mp3"
}

"Sniper.MVM_SpecialCompleted29"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted29.mp3"
}

"Sniper.MVM_SpecialCompleted30"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted30.mp3"
}

"Sniper.MVM_SpecialCompleted31"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted31.mp3"
}

"Sniper.MVM_SpecialCompleted32"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted32.mp3"
}

"Sniper.MVM_SpecialCompleted33"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted33.mp3"
}

"Sniper.MVM_SpecialCompleted34"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted34.mp3"
}

"Sniper.MVM_SpecialCompleted35"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted35.mp3"
}

"Sniper.MVM_SpecialCompleted36"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted36.mp3"
}

"Sniper.MVM_SpecialCompleted37"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted37.mp3"
}

"Sniper.MVM_SpecialCompleted38"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted38.mp3"
}

"Sniper.MVM_SpecialCompleted39"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted39.mp3"
}

"Sniper.MVM_SpecialCompleted40"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted40.mp3"
}

"Sniper.MVM_SpecialCompleted41"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted41.mp3"
}

"Sniper.MVM_SpecialCompleted42"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted42.mp3"
}

"Sniper.MVM_SpecialCompleted43"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted43.mp3"
}

"Sniper.MVM_SpecialCompleted44"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted44.mp3"
}

"Sniper.MVM_SpecialCompleted45"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted45.mp3"
}

"Sniper.MVM_SpecialCompleted46"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_SpecialCompleted46.mp3"
}

"Sniper.MVM_StandOnThePoint01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_StandOnThePoint01.mp3"
}

"Sniper.MVM_StandOnThePoint02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_StandOnThePoint02.mp3"
}

"Sniper.MVM_Taunts01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts01.mp3"
}

"Sniper.MVM_Taunts02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts02.mp3"
}

"Sniper.MVM_Taunts03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts03.mp3"
}

"Sniper.MVM_Taunts04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts04.mp3"
}

"Sniper.MVM_Taunts05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts05.mp3"
}

"Sniper.MVM_Taunts06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts06.mp3"
}

"Sniper.MVM_Taunts07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts07.mp3"
}

"Sniper.MVM_Taunts08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts08.mp3"
}

"Sniper.MVM_Taunts09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts09.mp3"
}

"Sniper.MVM_Taunts10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts10.mp3"
}

"Sniper.MVM_Taunts11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts11.mp3"
}

"Sniper.MVM_Taunts12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts12.mp3"
}

"Sniper.MVM_Taunts13"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts13.mp3"
}

"Sniper.MVM_Taunts14"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts14.mp3"
}

"Sniper.MVM_Taunts15"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts15.mp3"
}

"Sniper.MVM_Taunts16"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts16.mp3"
}

"Sniper.MVM_Taunts17"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts17.mp3"
}

"Sniper.MVM_Taunts18"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts18.mp3"
}

"Sniper.MVM_Taunts19"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts19.mp3"
}

"Sniper.MVM_Taunts20"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts20.mp3"
}

"Sniper.MVM_Taunts21"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts21.mp3"
}

"Sniper.MVM_Taunts22"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts22.mp3"
}

"Sniper.MVM_Taunts23"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts23.mp3"
}

"Sniper.MVM_Taunts24"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts24.mp3"
}

"Sniper.MVM_Taunts25"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts25.mp3"
}

"Sniper.MVM_Taunts26"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts26.mp3"
}

"Sniper.MVM_Taunts27"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts27.mp3"
}

"Sniper.MVM_Taunts28"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts28.mp3"
}

"Sniper.MVM_Taunts29"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts29.mp3"
}

"Sniper.MVM_Taunts30"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts30.mp3"
}

"Sniper.MVM_Taunts31"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts31.mp3"
}

"Sniper.MVM_Taunts32"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts32.mp3"
}

"Sniper.MVM_Taunts33"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts33.mp3"
}

"Sniper.MVM_Taunts34"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts34.mp3"
}

"Sniper.MVM_Taunts35"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts35.mp3"
}

"Sniper.MVM_Taunts36"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts36.mp3"
}

"Sniper.MVM_Taunts37"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts37.mp3"
}

"Sniper.MVM_Taunts38"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts38.mp3"
}

"Sniper.MVM_Taunts39"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts39.mp3"
}

"Sniper.MVM_Taunts40"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts40.mp3"
}

"Sniper.MVM_Taunts41"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts41.mp3"
}

"Sniper.MVM_Taunts42"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts42.mp3"
}

"Sniper.MVM_Taunts43"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts43.mp3"
}

"Sniper.MVM_Taunts44"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts44.mp3"
}

"Sniper.MVM_Taunts45"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts45.mp3"
}

"Sniper.MVM_Taunts46"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/sniper_mvm_Taunts46.mp3"
}

"Sniper.MVM_Thanks01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Thanks01.mp3"
}

"Sniper.MVM_Thanks02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Thanks02.mp3"
}

"Sniper.MVM_ThanksForTheHeal01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_ThanksForTheHeal01.mp3"
}

"Sniper.MVM_ThanksForTheHeal02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_ThanksForTheHeal02.mp3"
}

"Sniper.MVM_ThanksForTheHeal03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_ThanksForTheHeal03.mp3"
}

"Sniper.MVM_ThanksForTheTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_ThanksForTheTeleporter01.mp3"
}

"Sniper.MVM_ThanksForTheTeleporter02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_ThanksForTheTeleporter02.mp3"
}

"Sniper.MVM_ThanksForTheTeleporter03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_ThanksForTheTeleporter03.mp3"
}

"Sniper.MVM_Yes01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Yes01.mp3"
}

"Sniper.MVM_Yes02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Yes02.mp3"
}

"Sniper.MVM_Yes03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/sniper_mvm_Yes03.mp3"
}

"sniper.MVM_MedicFollow01"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_medicfollow01.mp3"
}
 
"sniper.MVM_MedicFollow02"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_medicfollow02.mp3"
}
 
"sniper.MVM_MedicFollow03"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_medicfollow03.mp3"
}
 
"sniper.MVM_MedicFollow04"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_medicfollow04.mp3"
}
 
"sniper.MVM_MedicFollow05"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_medicfollow05.mp3"
}

"sniper.MVM_MeleeDare01"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_meleedare01.mp3"
}

"sniper.MVM_MeleeDare02"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_meleedare02.mp3"
}

"sniper.MVM_MeleeDare03"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_meleedare03.mp3"
}

"sniper.MVM_MeleeDare04"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_meleedare04.mp3"
}

"sniper.MVM_MeleeDare05"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_meleedare05.mp3"
}

"sniper.MVM_MeleeDare06"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_meleedare06.mp3"
}

"sniper.MVM_MeleeDare07"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_meleedare07.mp3"
}

"sniper.MVM_MeleeDare08"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_meleedare08.mp3"
}

"sniper.MVM_MeleeDare09"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_meleedare09.mp3"
}

"sniper.MVM_Award01"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_award01.mp3"
}

"sniper.MVM_Award02"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_award02.mp3"
}

"sniper.MVM_Award03"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_award03.mp3"
}

"sniper.MVM_Award04"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_award04.mp3"
}

"sniper.MVM_Award05"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_award05.mp3"
}

"sniper.MVM_Award06"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_award06.mp3"
}

"sniper.MVM_Award07"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_award07.mp3"
}

"sniper.MVM_Award08"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_award08.mp3"
}

"sniper.MVM_Award09"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_award09.mp3"
}

"sniper.MVM_Award10"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_award10.mp3"
}

"sniper.MVM_Award11"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_award11.mp3"
}

"sniper.MVM_Award12"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_award12.mp3"
}

"sniper.MVM_Award13"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_award13.mp3"
}

"sniper.MVM_Award14"
{
	"channel"  "CHAN_VOICE"
	"volume"  "0.820"
	"pitch"  "PITCH_NORM"
	
	"soundlevel"  "SNDLVL_95dB"
	
	"wave"  "vo/mvm/norm/sniper_mvm_award14.mp3"
}

"Soldier.MVM_ActivateCharge01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_ActivateCharge01.mp3"
}

"Soldier.MVM_ActivateCharge02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_ActivateCharge02.mp3"
}

"Soldier.MVM_ActivateCharge03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_ActivateCharge03.mp3"
}

"Soldier.MVM_AutoCappedControlPoint01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_AutoCappedControlPoint01.mp3"
}

"Soldier.MVM_AutoCappedControlPoint02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_AutoCappedControlPoint02.mp3"
}

"Soldier.MVM_AutoCappedControlPoint03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_AutoCappedControlPoint03.mp3"
}

"Soldier.MVM_AutoCappedIntelligence01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_AutoCappedIntelligence01.mp3"
}

"Soldier.MVM_AutoCappedIntelligence02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_AutoCappedIntelligence02.mp3"
}

"Soldier.MVM_AutoCappedIntelligence03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_AutoCappedIntelligence03.mp3"
}

"Soldier.MVM_AutoDejectedTie01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_AutoDejectedTie01.mp3"
}

"Soldier.MVM_AutoDejectedTie02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_AutoDejectedTie02.mp3"
}

"Soldier.MVM_AutoDejectedTie03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_AutoDejectedTie03.mp3"
}

"Soldier.MVM_AutoOnFire01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_AutoOnFire01.mp3"
}

"Soldier.MVM_AutoOnFire02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_AutoOnFire02.mp3"
}

"Soldier.MVM_AutoOnFire03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_AutoOnFire03.mp3"
}

"Soldier.MVM_BattleCry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_BattleCry01.mp3"
}

"Soldier.MVM_BattleCry02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_BattleCry02.mp3"
}

"Soldier.MVM_BattleCry03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_BattleCry03.mp3"
}

"Soldier.MVM_BattleCry04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_BattleCry04.mp3"
}

"Soldier.MVM_BattleCry05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_BattleCry05.mp3"
}

"Soldier.MVM_BattleCry06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_BattleCry06.mp3"
}

"Soldier.MVM_Cheers01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Cheers01.mp3"
}

"Soldier.MVM_Cheers02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Cheers02.mp3"
}

"Soldier.MVM_Cheers03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Cheers03.mp3"
}

"Soldier.MVM_Cheers04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Cheers04.mp3"
}

"Soldier.MVM_Cheers05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Cheers05.mp3"
}

"Soldier.MVM_Cheers06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Cheers06.mp3"
}

"Soldier.MVM_CloakedSpy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_CloakedSpy01.mp3"
}

"Soldier.MVM_CloakedSpy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_CloakedSpy02.mp3"
}

"Soldier.MVM_CloakedSpy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_CloakedSpy03.mp3"
}

"Soldier.MVM_CloakedSpyIdentify01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_CloakedSpyIdentify01.mp3"
}

"Soldier.MVM_CloakedSpyIdentify02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_CloakedSpyIdentify02.mp3"
}

"Soldier.MVM_CloakedSpyIdentify03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_CloakedSpyIdentify03.mp3"
}

"Soldier.MVM_CloakedSpyIdentify04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_CloakedSpyIdentify04.mp3"
}

"Soldier.MVM_CloakedSpyIdentify05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_CloakedSpyIdentify05.mp3"
}

"Soldier.MVM_CloakedSpyIdentify06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_CloakedSpyIdentify06.mp3"
}

"Soldier.MVM_CloakedSpyIdentify07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_CloakedSpyIdentify07.mp3"
}

"Soldier.MVM_CloakedSpyIdentify08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_CloakedSpyIdentify08.mp3"
}

"Soldier.MVM_CloakedSpyIdentify09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_CloakedSpyIdentify09.mp3"
}

"Soldier.MVM_Go01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Go01.mp3"
}

"Soldier.MVM_Go02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Go02.mp3"
}

"Soldier.MVM_Go03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Go03.mp3"
}

"Soldier.MVM_GoodJob01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_GoodJob01.mp3"
}

"Soldier.MVM_GoodJob02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_GoodJob02.mp3"
}

"Soldier.MVM_GoodJob03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_GoodJob03.mp3"
}

"Soldier.MVM_HeadLeft01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_HeadLeft01.mp3"
}

"Soldier.MVM_HeadLeft02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_HeadLeft02.mp3"
}

"Soldier.MVM_HeadLeft03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_HeadLeft03.mp3"
}

"Soldier.MVM_HeadRight01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_HeadRight01.mp3"
}

"Soldier.MVM_HeadRight02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_HeadRight02.mp3"
}

"Soldier.MVM_HeadRight03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_HeadRight03.mp3"
}

"Soldier.MVM_HelpMe01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_HelpMe01.mp3"
}

"Soldier.MVM_HelpMe02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_HelpMe02.mp3"
}

"Soldier.MVM_HelpMe03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_HelpMe03.mp3"
}

"Soldier.MVM_HelpMeCapture01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_HelpMeCapture01.mp3"
}

"Soldier.MVM_HelpMeCapture02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_HelpMeCapture02.mp3"
}

"Soldier.MVM_HelpMeCapture03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_HelpMeCapture03.mp3"
}

"Soldier.MVM_HelpMeDefend01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_HelpMeDefend01.mp3"
}

"Soldier.MVM_HelpMeDefend02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_HelpMeDefend02.mp3"
}

"Soldier.MVM_HelpMeDefend03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_HelpMeDefend03.mp3"
}

"Soldier.MVM_HelpMeDefend04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_HelpMeDefend04.mp3"
}

"Soldier.MVM_Incoming01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Incoming01.mp3"
}

"Soldier.MVM_Jeers01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Jeers01.mp3"
}

"Soldier.MVM_Jeers02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Jeers02.mp3"
}

"Soldier.MVM_Jeers03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Jeers03.mp3"
}

"Soldier.MVM_Jeers04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Jeers04.mp3"
}

"Soldier.MVM_Jeers05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Jeers05.mp3"
}

"Soldier.MVM_Jeers06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Jeers06.mp3"
}

"Soldier.MVM_Jeers07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Jeers07.mp3"
}

"Soldier.MVM_Jeers08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Jeers08.mp3"
}

"Soldier.MVM_Jeers09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Jeers09.mp3"
}

"Soldier.MVM_Jeers10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Jeers10.mp3"
}

"Soldier.MVM_Jeers11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Jeers11.mp3"
}

"Soldier.MVM_Jeers12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Jeers12.mp3"
}

"Soldier.MVM_LaughEvil01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_LaughEvil01.mp3"
}

"Soldier.MVM_LaughEvil02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_LaughEvil02.mp3"
}

"Soldier.MVM_LaughEvil03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_LaughEvil03.mp3"
}

"Soldier.MVM_LaughHappy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_LaughHappy01.mp3"
}

"Soldier.MVM_LaughHappy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_LaughHappy02.mp3"
}

"Soldier.MVM_LaughHappy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_LaughHappy03.mp3"
}

"Soldier.MVM_LaughLong01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_LaughLong01.mp3"
}

"Soldier.MVM_LaughLong02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_LaughLong02.mp3"
}

"Soldier.MVM_LaughLong03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_LaughLong03.mp3"
}

"Soldier.MVM_LaughShort01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_LaughShort01.mp3"
}

"Soldier.MVM_LaughShort02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_LaughShort02.mp3"
}

"Soldier.MVM_LaughShort03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_LaughShort03.mp3"
}

"Soldier.MVM_LaughShort04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_LaughShort04.mp3"
}

"Soldier.MVM_Medic01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Medic01.mp3"
}

"Soldier.MVM_Medic02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Medic02.mp3"
}

"Soldier.MVM_Medic03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Medic03.mp3"
}

"Soldier.MVM_MoveUp01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_MoveUp01.mp3"
}

"Soldier.MVM_MoveUp02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_MoveUp02.mp3"
}

"Soldier.MVM_MoveUp03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_MoveUp03.mp3"
}

"Soldier.MVM_NeedDispenser01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_NeedDispenser01.mp3"
}

"Soldier.MVM_NeedSentry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_NeedSentry01.mp3"
}

"Soldier.MVM_NeedTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_NeedTeleporter01.mp3"
}

"Soldier.MVM_NegativeVocalization01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_NegativeVocalization01.mp3"
}

"Soldier.MVM_NegativeVocalization02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_NegativeVocalization02.mp3"
}

"Soldier.MVM_NegativeVocalization03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_NegativeVocalization03.mp3"
}

"Soldier.MVM_NegativeVocalization04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_NegativeVocalization04.mp3"
}

"Soldier.MVM_NegativeVocalization05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_NegativeVocalization05.mp3"
}

"Soldier.MVM_NegativeVocalization06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_NegativeVocalization06.mp3"
}

"Soldier.MVM_NiceShot01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_NiceShot01.mp3"
}

"Soldier.MVM_NiceShot02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_NiceShot02.mp3"
}

"Soldier.MVM_NiceShot03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_NiceShot03.mp3"
}

"Soldier.MVM_No01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_No01.mp3"
}

"Soldier.MVM_No02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_No02.mp3"
}

"Soldier.MVM_No03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_No03.mp3"
}

"Soldier.MVM_PainCrticialDeath01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PainCrticialDeath01.mp3"
}

"Soldier.MVM_PainCrticialDeath02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PainCrticialDeath02.mp3"
}

"Soldier.MVM_PainCrticialDeath03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PainCrticialDeath03.mp3"
}

"Soldier.MVM_PainCrticialDeath04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PainCrticialDeath04.mp3"
}

"Soldier.MVM_PainSevere01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PainSevere01.mp3"
}

"Soldier.MVM_PainSevere02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PainSevere02.mp3"
}

"Soldier.MVM_PainSevere03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PainSevere03.mp3"
}

"Soldier.MVM_PainSevere04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PainSevere04.mp3"
}

"Soldier.MVM_PainSevere05"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PainSevere05.mp3"
}

"Soldier.MVM_PainSevere06"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PainSevere06.mp3"
}

"Soldier.MVM_PainSharp01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PainSharp01.mp3"
}

"Soldier.MVM_PainSharp02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PainSharp02.mp3"
}

"Soldier.MVM_PainSharp03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PainSharp03.mp3"
}

"Soldier.MVM_PainSharp04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PainSharp04.mp3"
}

"Soldier.MVM_PainSharp05"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PainSharp05.mp3"
}

"Soldier.MVM_PainSharp06"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PainSharp06.mp3"
}

"Soldier.MVM_PainSharp07"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PainSharp07.mp3"
}

"Soldier.MVM_PainSharp08"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PainSharp08.mp3"
}

"Soldier.MVM_PositiveVocalization01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PositiveVocalization01.mp3"
}

"Soldier.MVM_PositiveVocalization02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PositiveVocalization02.mp3"
}

"Soldier.MVM_PositiveVocalization03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PositiveVocalization03.mp3"
}

"Soldier.MVM_PositiveVocalization04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PositiveVocalization04.mp3"
}

"Soldier.MVM_PositiveVocalization05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_PositiveVocalization05.mp3"
}

"Soldier.MVM_SentryAhead01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_SentryAhead01.mp3"
}

"Soldier.MVM_SentryAhead02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_SentryAhead02.mp3"
}

"Soldier.MVM_SentryAhead03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_SentryAhead03.mp3"
}

"Soldier.MVM_SpecialCompleted-AssistedKill01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_SpecialCompleted-AssistedKill01.mp3"
}

"Soldier.MVM_SpecialCompleted01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_SpecialCompleted01.mp3"
}

"Soldier.MVM_SpecialCompleted02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_SpecialCompleted02.mp3"
}

"Soldier.MVM_SpecialCompleted03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_SpecialCompleted03.mp3"
}

"Soldier.MVM_SpecialCompleted04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_SpecialCompleted04.mp3"
}

"Soldier.MVM_SpecialCompleted05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_SpecialCompleted05.mp3"
}

"Soldier.MVM_StandOnThePoint01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_StandOnThePoint01.mp3"
}

"Soldier.MVM_StandOnThePoint02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_StandOnThePoint02.mp3"
}

"Soldier.MVM_StandOnThePoint03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_StandOnThePoint03.mp3"
}

"Soldier.MVM_Taunts01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts01.mp3"
}

"Soldier.MVM_Taunts02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts02.mp3"
}

"Soldier.MVM_Taunts03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts03.mp3"
}

"Soldier.MVM_Taunts04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts04.mp3"
}

"Soldier.MVM_Taunts05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts05.mp3"
}

"Soldier.MVM_Taunts06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts06.mp3"
}

"Soldier.MVM_Taunts07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts07.mp3"
}

"Soldier.MVM_Taunts08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts08.mp3"
}

"Soldier.MVM_Taunts09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts09.mp3"
}

"Soldier.MVM_Taunts10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts10.mp3"
}

"Soldier.MVM_Taunts11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts11.mp3"
}

"Soldier.MVM_Taunts12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts12.mp3"
}

"Soldier.MVM_Taunts13"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts13.mp3"
}

"Soldier.MVM_Taunts14"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts14.mp3"
}

"Soldier.MVM_Taunts15"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts15.mp3"
}

"Soldier.MVM_Taunts16"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts16.mp3"
}

"Soldier.MVM_Taunts17"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts17.mp3"
}

"Soldier.MVM_Taunts18"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts18.mp3"
}

"Soldier.MVM_Taunts19"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts19.mp3"
}

"Soldier.MVM_Taunts20"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts20.mp3"
}

"Soldier.MVM_Taunts21"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/soldier_mvm_Taunts21.mp3"
}

"Soldier.MVM_Thanks01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Thanks01.mp3"
}

"Soldier.MVM_Thanks02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Thanks02.mp3"
}

"Soldier.MVM_ThanksForTheHeal01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_ThanksForTheHeal01.mp3"
}

"Soldier.MVM_ThanksForTheHeal02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_ThanksForTheHeal02.mp3"
}

"Soldier.MVM_ThanksForTheHeal03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_ThanksForTheHeal03.mp3"
}

"Soldier.MVM_ThanksForTheTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_ThanksForTheTeleporter01.mp3"
}

"Soldier.MVM_ThanksForTheTeleporter02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_ThanksForTheTeleporter02.mp3"
}

"Soldier.MVM_ThanksForTheTeleporter03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_ThanksForTheTeleporter03.mp3"
}

"Soldier.MVM_Yes01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Yes01.mp3"
}

"Soldier.MVM_Yes02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Yes02.mp3"
}

"Soldier.MVM_Yes03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Yes03.mp3"
}

"Soldier.MVM_Yes04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/soldier_mvm_Yes04.mp3"
}

"Spy.MVM_ActivateCharge01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_ActivateCharge01.mp3"
}

"Spy.MVM_ActivateCharge02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_ActivateCharge02.mp3"
}

"Spy.MVM_ActivateCharge03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_ActivateCharge03.mp3"
}

"Spy.MVM_AutoCappedControlPoint01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_AutoCappedControlPoint01.mp3"
}

"Spy.MVM_AutoCappedControlPoint02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_AutoCappedControlPoint02.mp3"
}

"Spy.MVM_AutoCappedControlPoint03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_AutoCappedControlPoint03.mp3"
}

"Spy.MVM_AutoCappedIntelligence01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_AutoCappedIntelligence01.mp3"
}

"Spy.MVM_AutoCappedIntelligence02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_AutoCappedIntelligence02.mp3"
}

"Spy.MVM_AutoCappedIntelligence03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_AutoCappedIntelligence03.mp3"
}

"Spy.MVM_AutoDejectedTie01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_AutoDejectedTie01.mp3"
}

"Spy.MVM_AutoDejectedTie02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_AutoDejectedTie02.mp3"
}

"Spy.MVM_AutoDejectedTie03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_AutoDejectedTie03.mp3"
}

"Spy.MVM_AutoOnFire01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_AutoOnFire01.mp3"
}

"Spy.MVM_AutoOnFire02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_AutoOnFire02.mp3"
}

"Spy.MVM_AutoOnFire03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_AutoOnFire03.mp3"
}

"Spy.MVM_BattleCry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_BattleCry01.mp3"
}

"Spy.MVM_BattleCry02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_BattleCry02.mp3"
}

"Spy.MVM_BattleCry03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_BattleCry03.mp3"
}

"Spy.MVM_BattleCry04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_BattleCry04.mp3"
}

"Spy.MVM_Cheers01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Cheers01.mp3"
}

"Spy.MVM_Cheers02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Cheers02.mp3"
}

"Spy.MVM_Cheers03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Cheers03.mp3"
}

"Spy.MVM_Cheers04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Cheers04.mp3"
}

"Spy.MVM_Cheers05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Cheers05.mp3"
}

"Spy.MVM_Cheers06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Cheers06.mp3"
}

"Spy.MVM_Cheers07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Cheers07.mp3"
}

"Spy.MVM_Cheers08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Cheers08.mp3"
}

"Spy.MVM_CloakedSpy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_CloakedSpy01.mp3"
}

"Spy.MVM_CloakedSpy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_CloakedSpy02.mp3"
}

"Spy.MVM_CloakedSpy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_CloakedSpy03.mp3"
}

"Spy.MVM_CloakedSpy04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_CloakedSpy04.mp3"
}

"Spy.MVM_CloakedSpyIdentify01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_CloakedSpyIdentify01.mp3"
}

"Spy.MVM_CloakedSpyIdentify02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_CloakedSpyIdentify02.mp3"
}

"Spy.MVM_CloakedSpyIdentify03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_CloakedSpyIdentify03.mp3"
}

"Spy.MVM_CloakedSpyIdentify04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_CloakedSpyIdentify04.mp3"
}

"Spy.MVM_CloakedSpyIdentify05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_CloakedSpyIdentify05.mp3"
}

"Spy.MVM_CloakedSpyIdentify06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_CloakedSpyIdentify06.mp3"
}

"Spy.MVM_CloakedSpyIdentify07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_CloakedSpyIdentify07.mp3"
}

"Spy.MVM_CloakedSpyIdentify08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_CloakedSpyIdentify08.mp3"
}

"Spy.MVM_CloakedSpyIdentify09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_CloakedSpyIdentify09.mp3"
}

"Spy.MVM_CloakedSpyIdentify10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_CloakedSpyIdentify10.mp3"
}

"Spy.MVM_Go01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Go01.mp3"
}

"Spy.MVM_Go02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Go02.mp3"
}

"Spy.MVM_Go03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Go03.mp3"
}

"Spy.MVM_GoodJob01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_GoodJob01.mp3"
}

"Spy.MVM_GoodJob02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_GoodJob02.mp3"
}

"Spy.MVM_GoodJob03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_GoodJob03.mp3"
}

"Spy.MVM_HeadLeft01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_HeadLeft01.mp3"
}

"Spy.MVM_HeadLeft02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_HeadLeft02.mp3"
}

"Spy.MVM_HeadLeft03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_HeadLeft03.mp3"
}

"Spy.MVM_HeadRight01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_HeadRight01.mp3"
}

"Spy.MVM_HeadRight02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_HeadRight02.mp3"
}

"Spy.MVM_HeadRight03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_HeadRight03.mp3"
}

"Spy.MVM_HelpMe01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_HelpMe01.mp3"
}

"Spy.MVM_HelpMe02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_HelpMe02.mp3"
}

"Spy.MVM_HelpMe03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_HelpMe03.mp3"
}

"Spy.MVM_HelpMeCapture01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_HelpMeCapture01.mp3"
}

"Spy.MVM_HelpMeCapture02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_HelpMeCapture02.mp3"
}

"Spy.MVM_HelpMeCapture03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_HelpMeCapture03.mp3"
}

"Spy.MVM_HelpMeDefend01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_HelpMeDefend01.mp3"
}

"Spy.MVM_HelpMeDefend02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_HelpMeDefend02.mp3"
}

"Spy.MVM_HelpMeDefend03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_HelpMeDefend03.mp3"
}

"Spy.MVM_Incoming01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Incoming01.mp3"
}

"Spy.MVM_Incoming02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Incoming02.mp3"
}

"Spy.MVM_Incoming03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Incoming03.mp3"
}

"Spy.MVM_Jeers01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Jeers01.mp3"
}

"Spy.MVM_Jeers02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Jeers02.mp3"
}

"Spy.MVM_Jeers03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Jeers03.mp3"
}

"Spy.MVM_Jeers04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Jeers04.mp3"
}

"Spy.MVM_Jeers05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Jeers05.mp3"
}

"Spy.MVM_Jeers06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Jeers06.mp3"
}

"Spy.MVM_LaughEvil01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_LaughEvil01.mp3"
}

"Spy.MVM_LaughEvil02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_LaughEvil02.mp3"
}

"Spy.MVM_LaughHappy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_LaughHappy01.mp3"
}

"Spy.MVM_LaughHappy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_LaughHappy02.mp3"
}

"Spy.MVM_LaughHappy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_LaughHappy03.mp3"
}

"Spy.MVM_LaughLong01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_LaughLong01.mp3"
}

"Spy.MVM_LaughShort01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_LaughShort01.mp3"
}

"Spy.MVM_LaughShort02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_LaughShort02.mp3"
}

"Spy.MVM_LaughShort03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_LaughShort03.mp3"
}

"Spy.MVM_LaughShort04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_LaughShort04.mp3"
}

"Spy.MVM_LaughShort05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_LaughShort05.mp3"
}

"Spy.MVM_LaughShort06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_LaughShort06.mp3"
}

"Spy.MVM_Medic01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Medic01.mp3"
}

"Spy.MVM_Medic02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Medic02.mp3"
}

"Spy.MVM_Medic03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Medic03.mp3"
}

"Spy.MVM_MoveUp01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_MoveUp01.mp3"
}

"Spy.MVM_MoveUp02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_MoveUp02.mp3"
}

"Spy.MVM_NeedDispenser01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_NeedDispenser01.mp3"
}

"Spy.MVM_NeedSentry01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_NeedSentry01.mp3"
}

"Spy.MVM_NeedTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_NeedTeleporter01.mp3"
}

"Spy.MVM_NegativeVocalization01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_NegativeVocalization01.mp3"
}

"Spy.MVM_NegativeVocalization02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_NegativeVocalization02.mp3"
}

"Spy.MVM_NegativeVocalization03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_NegativeVocalization03.mp3"
}

"Spy.MVM_NegativeVocalization04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_NegativeVocalization04.mp3"
}

"Spy.MVM_NegativeVocalization05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_NegativeVocalization05.mp3"
}

"Spy.MVM_NegativeVocalization06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_NegativeVocalization06.mp3"
}

"Spy.MVM_NegativeVocalization07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_NegativeVocalization07.mp3"
}

"Spy.MVM_NegativeVocalization08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_NegativeVocalization08.mp3"
}

"Spy.MVM_NegativeVocalization09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_NegativeVocalization09.mp3"
}

"Spy.MVM_NiceShot01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_NiceShot01.mp3"
}

"Spy.MVM_NiceShot02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_NiceShot02.mp3"
}

"Spy.MVM_NiceShot03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_NiceShot03.mp3"
}

"Spy.MVM_No01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_No01.mp3"
}

"Spy.MVM_No02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_No02.mp3"
}

"Spy.MVM_No03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_No03.mp3"
}

"Spy.MVM_PainCrticialDeath01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/spy_mvm_PainCrticialDeath01.mp3"
}

"Spy.MVM_PainCrticialDeath02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/spy_mvm_PainCrticialDeath02.mp3"
}

"Spy.MVM_PainCrticialDeath03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/spy_mvm_PainCrticialDeath03.mp3"
}

"Spy.MVM_PainSevere01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/spy_mvm_PainSevere01.mp3"
}

"Spy.MVM_PainSevere02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/spy_mvm_PainSevere02.mp3"
}

"Spy.MVM_PainSevere03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/spy_mvm_PainSevere03.mp3"
}

"Spy.MVM_PainSevere04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/spy_mvm_PainSevere04.mp3"
}

"Spy.MVM_PainSevere05"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/spy_mvm_PainSevere05.mp3"
}

"Spy.MVM_PainSharp01"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/spy_mvm_PainSharp01.mp3"
}

"Spy.MVM_PainSharp02"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/spy_mvm_PainSharp02.mp3"
}

"Spy.MVM_PainSharp03"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/spy_mvm_PainSharp03.mp3"
}

"Spy.MVM_PainSharp04"
{
	"channel"		"CHAN_VOICE"
//	"volume"		"VOL_NORM"
	"volume"	"0.86"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_80dB"

	"wave"			"vo/mvm/norm/spy_mvm_PainSharp04.mp3"
}

"Spy.MVM_PositiveVocalization01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_PositiveVocalization01.mp3"
}

"Spy.MVM_PositiveVocalization02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_PositiveVocalization02.mp3"
}

"Spy.MVM_PositiveVocalization03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_PositiveVocalization03.mp3"
}

"Spy.MVM_PositiveVocalization04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_PositiveVocalization04.mp3"
}

"Spy.MVM_PositiveVocalization05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_PositiveVocalization05.mp3"
}

"Spy.MVM_SentryAhead01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_SentryAhead01.mp3"
}

"Spy.MVM_SentryAhead02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_SentryAhead02.mp3"
}

"Spy.MVM_SpecialCompleted-AssistedKill01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_SpecialCompleted-AssistedKill01.mp3"
}

"Spy.MVM_SpecialCompleted-AssistedKill02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_SpecialCompleted-AssistedKill02.mp3"
}

"Spy.MVM_SpecialCompleted01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_SpecialCompleted01.mp3"
}

"Spy.MVM_SpecialCompleted02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_SpecialCompleted02.mp3"
}

"Spy.MVM_SpecialCompleted03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_SpecialCompleted03.mp3"
}

"Spy.MVM_SpecialCompleted04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_SpecialCompleted04.mp3"
}

"Spy.MVM_SpecialCompleted05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_SpecialCompleted05.mp3"
}

"Spy.MVM_SpecialCompleted06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_SpecialCompleted06.mp3"
}

"Spy.MVM_SpecialCompleted07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_SpecialCompleted07.mp3"
}

"Spy.MVM_SpecialCompleted08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_SpecialCompleted08.mp3"
}

"Spy.MVM_SpecialCompleted09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_SpecialCompleted09.mp3"
}

"Spy.MVM_SpecialCompleted10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_SpecialCompleted10.mp3"
}

"Spy.MVM_SpecialCompleted11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_SpecialCompleted11.mp3"
}

"Spy.MVM_SpecialCompleted12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_SpecialCompleted12.mp3"
}

"Spy.MVM_StandOnThePoint01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_StandOnThePoint01.mp3"
}

"Spy.MVM_StandOnThePoint02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_StandOnThePoint02.mp3"
}

"Spy.MVM_StandOnThePoint03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_StandOnThePoint03.mp3"
}

"Spy.MVM_StandOnThePoint04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_StandOnThePoint04.mp3"
}

"Spy.MVM_Taunts01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_Taunts01.mp3"
}

"Spy.MVM_Taunts02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_Taunts02.mp3"
}

"Spy.MVM_Taunts03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_Taunts03.mp3"
}

"Spy.MVM_Taunts04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_Taunts04.mp3"
}

"Spy.MVM_Taunts05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_Taunts05.mp3"
}

"Spy.MVM_Taunts06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_Taunts06.mp3"
}

"Spy.MVM_Taunts07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_Taunts07.mp3"
}

"Spy.MVM_Taunts08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_Taunts08.mp3"
}

"Spy.MVM_Taunts09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_Taunts09.mp3"
}

"Spy.MVM_Taunts10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_Taunts10.mp3"
}

"Spy.MVM_Taunts11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_Taunts11.mp3"
}

"Spy.MVM_Taunts12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_Taunts12.mp3"
}

"Spy.MVM_Taunts13"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_Taunts13.mp3"
}

"Spy.MVM_Taunts14"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_Taunts14.mp3"
}

"Spy.MVM_Taunts15"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_Taunts15.mp3"
}

"Spy.MVM_Taunts16"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_Taunts16.mp3"
}

"Spy.MVM_Thanks01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Thanks01.mp3"
}

"Spy.MVM_Thanks02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Thanks02.mp3"
}

"Spy.MVM_Thanks03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Thanks03.mp3"
}

"Spy.MVM_ThanksForTheHeal01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_ThanksForTheHeal01.mp3"
}

"Spy.MVM_ThanksForTheHeal02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_ThanksForTheHeal02.mp3"
}

"Spy.MVM_ThanksForTheHeal03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_ThanksForTheHeal03.mp3"
}

"Spy.MVM_ThanksForTheTeleporter01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_ThanksForTheTeleporter01.mp3"
}

"Spy.MVM_ThanksForTheTeleporter02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_ThanksForTheTeleporter02.mp3"
}

"Spy.MVM_ThanksForTheTeleporter03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_ThanksForTheTeleporter03.mp3"
}

"Spy.MVM_Yes01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Yes01.mp3"
}

"Spy.MVM_Yes02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Yes02.mp3"
}

"Spy.MVM_Yes03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/spy_mvm_Yes03.mp3"
}

"Spy.MVM_HighFiveSuccess01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive_success01.mp3"
}

"Spy.MVM_HighFiveSuccess02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive_success02.mp3"
}

"Spy.MVM_HighFiveSuccess03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive_success03.mp3"
}

"Spy.MVM_HighFiveSuccess04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive_success04.mp3"
}
"Spy.MVM_HighFiveSuccess05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive_success05.mp3"
}

"Spy.MVM_HighFive01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive01.mp3"
}

"Spy.MVM_HighFive02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive02.mp3"
}

"Spy.MVM_HighFive03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive03.mp3"
}

"Spy.MVM_HighFive04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive04.mp3"
}

"Spy.MVM_HighFive05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive05.mp3"
}

"Spy.MVM_HighFive06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive06.mp3"
}

"Spy.MVM_HighFive07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive07.mp3"
}

"Spy.MVM_HighFive08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive08.mp3"
}

"Spy.MVM_HighFive09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive09.mp3"
}

"Spy.MVM_HighFive10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive10.mp3"
}

"Spy.MVM_HighFive11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive11.mp3"
}

"Spy.MVM_HighFive12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive12.mp3"
}

"Spy.MVM_HighFive13"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive13.mp3"
}

"Spy.MVM_HighFive14"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.620"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_95dB"

	"wave"			"vo/mvm/norm/taunts/spy_mvm_highfive14.mp3"
}


// -----------------------------------------------------------------------------
// Heavy
// -----------------------------------------------------------------------------

"Heavy.MVM_Award01"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Award01.mp3"
}
 
"Heavy.MVM_Award02"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Award02.mp3"
}
 
"Heavy.MVM_Award03"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Award03.mp3"
}
 
"Heavy.MVM_Award04"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Award04.mp3"
}
 
"Heavy.MVM_Award05"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Award05.mp3"
}
 
"Heavy.MVM_Award07"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Award07.mp3"
}
 
"Heavy.MVM_Award08"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Award08.mp3"
}
 
"Heavy.MVM_Award09"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Award09.mp3"
}
 
"Heavy.MVM_Award10"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Award10.mp3"
}
 
"Heavy.MVM_Award11"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Award11.mp3"
}
 
"Heavy.MVM_Award12"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Award12.mp3"
}
 
"Heavy.MVM_Award13"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Award13.mp3"
}
 
"Heavy.MVM_Award16"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Award16.mp3"
}
 
"Heavy.MVM_Award14"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Award14.mp3"
}
 
"Heavy.MVM_Award15"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Award15.mp3"
}
 
"Heavy.MVM_Award06"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Award06.mp3"
}
 
"Heavy.MVM_CartGoingBackDefense01"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartGoingBackDefense01.mp3"
}
 
"Heavy.MVM_CartGoingBackDefense02"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartGoingBackDefense02.mp3"
}
 
"Heavy.MVM_CartGoingBackDefense03"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartGoingBackDefense03.mp3"
}
 
"Heavy.MVM_CartGoingBackDefense04"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartGoingBackDefense04.mp3"
}
 
"Heavy.MVM_CartGoingBackDefense05"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartGoingBackDefense05.mp3"
}
 
"Heavy.MVM_CartGoingBackDefense06"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartGoingBackDefense06.mp3"
}
 
"Heavy.MVM_CartGoingBackDefense07"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartGoingBackDefense07.mp3"
}
 
"Heavy.MVM_CartGoingBackDefense08"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartGoingBackDefense08.mp3"
}
 
"Heavy.MVM_CartGoingBackOffense01"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartGoingBackOffense01.mp3"
}
 
"Heavy.MVM_CartGoingBackOffense02"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartGoingBackOffense02.mp3"
}
 
"Heavy.MVM_CartGoingBackOffense03"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartGoingBackOffense03.mp3"
}
 
"Heavy.MVM_CartGoingBackOffense04"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartGoingBackOffense04.mp3"
}
 
"Heavy.MVM_CartGoingBackOffense05"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartGoingBackOffense05.mp3"
}
 
"Heavy.MVM_CartGoingBackOffense06"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartGoingBackOffense06.mp3"
}
 
"Heavy.MVM_CartGoingBackOffense07"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartGoingBackOffense07.mp3"
}
 
"Heavy.MVM_CartMovingForwardDefense01"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardDefense01.mp3"
}
 
"Heavy.MVM_CartMovingForwardDefense02"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardDefense02.mp3"
}
 
"Heavy.MVM_CartMovingForwardDefense04"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardDefense04.mp3"
}
 
"Heavy.MVM_CartMovingForwardDefense05"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardDefense05.mp3"
}
 
"Heavy.MVM_CartMovingForwardDefense03"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardDefense03.mp3"
}
 
"Heavy.MVM_CartMovingForwardDefense06"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardDefense06.mp3"
}
 
"Heavy.MVM_CartMovingForwardOffense01"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardOffense01.mp3"
}
 
"Heavy.MVM_CartMovingForwardOffense02"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardOffense02.mp3"
}
 
"Heavy.MVM_CartMovingForwardOffense03"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardOffense03.mp3"
}
 
"Heavy.MVM_CartMovingForwardOffense04"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardOffense04.mp3"
}
 
"Heavy.MVM_CartMovingForwardOffense05"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardOffense05.mp3"
}
 
"Heavy.MVM_CartMovingForwardOffense06"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardOffense06.mp3"
}
 
"Heavy.MVM_CartMovingForwardOffense07"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardOffense07.mp3"
}
 
"Heavy.MVM_CartMovingForwardOffense08"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardOffense08.mp3"
}
 
"Heavy.MVM_CartMovingForwardOffense09"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardOffense09.mp3"
}
 
"Heavy.MVM_CartMovingForwardOffense11"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardOffense11.mp3"
}
 
"Heavy.MVM_CartMovingForwardOffense12"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardOffense12.mp3"
}
 
"Heavy.MVM_CartMovingForwardOffense13"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardOffense13.mp3"
}
 
"Heavy.MVM_CartMovingForwardOffense14"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardOffense14.mp3"
}
 
"Heavy.MVM_CartMovingForwardOffense15"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardOffense15.mp3"
}
 
"Heavy.MVM_CartMovingForwardOffense16"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardOffense16.mp3"
}
 
"Heavy.MVM_CartMovingForwardOffense17"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardOffense17.mp3"
}
 
"Heavy.MVM_CartMovingForwardOffense10"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartMovingForwardOffense10.mp3"
}
 
"Heavy.MVM_CartStayCloseOffense01"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartStayCloseOffense01.mp3"
}
 
"Heavy.MVM_CartStayCloseOffense02"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartStayCloseOffense02.mp3"
}
 
"Heavy.MVM_CartStayCloseOffense03"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartStayCloseOffense03.mp3"
}
 
"Heavy.MVM_CartStayCloseOffense06"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartStayCloseOffense06.mp3"
}
 
"Heavy.MVM_CartStayCloseOffense05"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartStayCloseOffense05.mp3"
}
 
"Heavy.MVM_CartStayCloseOffense04"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartStayCloseOffense04.mp3"
}
 
"Heavy.MVM_CartStopItDefense01"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartStopItDefense01.mp3"
}
 
"Heavy.MVM_CartStopItDefense02"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartStopItDefense02.mp3"
}
 
"Heavy.MVM_CartStopItDefense03"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartStopItDefense03.mp3"
}
 
"Heavy.MVM_CartStopItDefense04"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartStopItDefense04.mp3"
}
 
"Heavy.MVM_CartStoppedOffense01"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartStoppedOffense01.mp3"
}
 
"Heavy.MVM_CartStoppedOffense02"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartStoppedOffense02.mp3"
}
 
"Heavy.MVM_CartStoppedOffense04"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartStoppedOffense04.mp3"
}
 
"Heavy.MVM_CartStoppedOffense03"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_CartStoppedOffense03.mp3"
}
 
"Heavy.MVM_Domination01"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Domination01.mp3"
}
 
"Heavy.MVM_Domination02"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Domination02.mp3"
}
 
"Heavy.MVM_Domination03"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Domination03.mp3"
}
 
"Heavy.MVM_Domination04"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Domination04.mp3"
}
 
"Heavy.MVM_Domination05"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Domination05.mp3"
}
 
"Heavy.MVM_Domination06"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Domination06.mp3"
}
 
"Heavy.MVM_Domination07"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Domination07.mp3"
}
 
"Heavy.MVM_Domination08"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Domination08.mp3"
}
 
"Heavy.MVM_Domination09"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Domination09.mp3"
}
 
"Heavy.MVM_Domination10"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Domination10.mp3"
}
 
"Heavy.MVM_Domination11"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Domination11.mp3"
}
 
"Heavy.MVM_Domination12"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Domination12.mp3"
}
 
"Heavy.MVM_Domination13"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Domination13.mp3"
}
 
"Heavy.MVM_Domination14"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Domination14.mp3"
}
 
"Heavy.MVM_Domination15"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Domination15.mp3"
}
 
"Heavy.MVM_Domination16"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Domination16.mp3"
}
 
"Heavy.MVM_Domination17"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Domination17.mp3"
}
 
"Heavy.MVM_Domination18"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Domination18.mp3"
}
 
"Heavy.MVM_FightOnCap04"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_FightOnCap04.mp3"
}
 
"Heavy.MVM_FightOnCap01"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_FightOnCap01.mp3"
}
 
"Heavy.MVM_FightOnCap02"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_FightOnCap02.mp3"
}
 
"Heavy.MVM_FightOnCap03"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_FightOnCap03.mp3"
}
 
"Heavy.MVM_LaughterBig02"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_LaughterBig02.mp3"
}
 
"Heavy.MVM_LaughterBig01"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_LaughterBig01.mp3"
}
 
"Heavy.MVM_LaughterBig03"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_LaughterBig03.mp3"
}
 
"Heavy.MVM_LaughterBig04"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_LaughterBig04.mp3"
}
 
"Heavy.MVM_MedicFollow01"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MedicFollow01.mp3"
}
 
"Heavy.MVM_MedicFollow02"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MedicFollow02.mp3"
}
 
"Heavy.MVM_MedicFollow03"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MedicFollow03.mp3"
}
 
"Heavy.MVM_MedicFollow04"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MedicFollow04.mp3"
}
 
"Heavy.MVM_MedicFollow05"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MedicFollow05.mp3"
}
 
"Heavy.MVM_MedicFollow06"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MedicFollow06.mp3"
}
 
"Heavy.MVM_MedicFollow07"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MedicFollow07.mp3"
}
 
"Heavy.MVM_MeleeDare01"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MeleeDare01.mp3"
}
 
"Heavy.MVM_MeleeDare02"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MeleeDare02.mp3"
}
 
"Heavy.MVM_MeleeDare03"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MeleeDare03.mp3"
}
 
"Heavy.MVM_MeleeDare04"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MeleeDare04.mp3"
}
 
"Heavy.MVM_MeleeDare05"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MeleeDare05.mp3"
}
 
"Heavy.MVM_MeleeDare06"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MeleeDare06.mp3"
}
 
"Heavy.MVM_MeleeDare07"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MeleeDare07.mp3"
}
 
"Heavy.MVM_MeleeDare08"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MeleeDare08.mp3"
}
 
"Heavy.MVM_MeleeDare09"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MeleeDare09.mp3"
}
 
"Heavy.MVM_MeleeDare10"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MeleeDare10.mp3"
}
 
"Heavy.MVM_MeleeDare11"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MeleeDare11.mp3"
}
 
"Heavy.MVM_MeleeDare12"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MeleeDare12.mp3"
}
 
"Heavy.MVM_MeleeDare13"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_MeleeDare13.mp3"
}
 
"Heavy.MVM_Meleeing01"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Meleeing01.mp3"
}
 
"Heavy.MVM_Meleeing02"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Meleeing02.mp3"
}
 
"Heavy.MVM_Meleeing03"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Meleeing03.mp3"
}
 
"Heavy.MVM_Meleeing04"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Meleeing04.mp3"
}
 
"Heavy.MVM_Meleeing05"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Meleeing05.mp3"
}
 
"Heavy.MVM_Meleeing06"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Meleeing06.mp3"
}
 
"Heavy.MVM_Meleeing07"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Meleeing07.mp3"
}
 
"Heavy.MVM_Meleeing08"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Meleeing08.mp3"
}
 
"Heavy.MVM_Revenge01"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Revenge01.mp3"
}
 
"Heavy.MVM_Revenge02"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Revenge02.mp3"
}
 
"Heavy.MVM_Revenge03"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Revenge03.mp3"
}
 
"Heavy.MVM_Revenge04"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Revenge04.mp3"
}
 
"Heavy.MVM_Revenge05"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Revenge05.mp3"
}
 
"Heavy.MVM_Revenge06"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Revenge06.mp3"
}
 
"Heavy.MVM_Revenge07"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Revenge07.mp3"
}
 
"Heavy.MVM_Revenge08"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Revenge08.mp3"
}
 
"Heavy.MVM_Revenge09"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Revenge09.mp3"
}
 
"Heavy.MVM_Revenge10"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Revenge10.mp3"
}
 
"Heavy.MVM_Revenge11"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Revenge11.mp3"
}
 
"Heavy.MVM_Revenge12"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Revenge12.mp3"
}
 
"Heavy.MVM_Revenge13"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Revenge13.mp3"
}
 
"Heavy.MVM_Revenge14"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Revenge14.mp3"
}
 
"Heavy.MVM_Revenge15"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Revenge15.mp3"
}
 
"Heavy.MVM_Singing01"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Singing01.mp3"
}
 
"Heavy.MVM_Singing02"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Singing02.mp3"
}
 
"Heavy.MVM_Singing03"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Singing03.mp3"
}
 
"Heavy.MVM_Singing04"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Singing04.mp3"
}
 
"Heavy.MVM_Singing05"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Singing05.mp3"
}
 
"Heavy.MVM_Specials01"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Specials01.mp3"
}
 
"Heavy.MVM_Specials02"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Specials02.mp3"
}
 
"Heavy.MVM_Specials03"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Specials03.mp3"
}
 
"Heavy.MVM_Specials04"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Specials04.mp3"
}
 
"Heavy.MVM_Specials05"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_Specials05.mp3"
}
 
"Heavy.MVM_SpecialWeapon04"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_SpecialWeapon04.mp3"
}
 
"Heavy.MVM_SpecialWeapon01"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_SpecialWeapon01.mp3"
}
 
"Heavy.MVM_SpecialWeapon02"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_SpecialWeapon02.mp3"
}
 
"Heavy.MVM_SpecialWeapon03"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_SpecialWeapon03.mp3"
}
 
"Heavy.MVM_SpecialWeapon05"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_SpecialWeapon05.mp3"
}
 
"Heavy.MVM_SpecialWeapon06"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_SpecialWeapon06.mp3"
}
 
"Heavy.MVM_SpecialWeapon07"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_SpecialWeapon07.mp3"
}
 
"Heavy.MVM_SpecialWeapon08"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_SpecialWeapon08.mp3"
}
 
"Heavy.MVM_SpecialWeapon09"
{
 "channel"  "CHAN_VOICE"
 "volume"  "0.820"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/heavy_mvm_SpecialWeapon09.mp3"
}
  
"Heavy.MVM_SandwichEat"
{
 "channel"  "CHAN_VOICE"
 "volume"  "1"
 "pitch"  "PITCH_NORM"
 "soundlevel"  "SNDLVL_95dB"
 "wave"  "vo/mvm/norm/SandwichEat09.mp3"
}

"Heavy.MVM_SandwichTaunt01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/heavy_mvm_SandwichTaunt01.mp3"
}

"Heavy.MVM_SandwichTaunt02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/heavy_mvm_SandwichTaunt02.mp3"
}

"Heavy.MVM_SandwichTaunt03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/heavy_mvm_SandwichTaunt03.mp3"
}

"Heavy.MVM_SandwichTaunt04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/heavy_mvm_SandwichTaunt04.mp3"
}

"Heavy.MVM_SandwichTaunt05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/heavy_mvm_SandwichTaunt05.mp3"
}

"Heavy.MVM_SandwichTaunt06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/heavy_mvm_SandwichTaunt06.mp3"
}

"Heavy.MVM_SandwichTaunt07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/heavy_mvm_SandwichTaunt07.mp3"
}

"Heavy.MVM_SandwichTaunt08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/heavy_mvm_SandwichTaunt08.mp3"
}

"Heavy.MVM_SandwichTaunt09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/heavy_mvm_SandwichTaunt09.mp3"
}

"Heavy.MVM_SandwichTaunt10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/heavy_mvm_SandwichTaunt10.mp3"
}

"Heavy.MVM_SandwichTaunt11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/heavy_mvm_SandwichTaunt11.mp3"
}

"Heavy.MVM_SandwichTaunt12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/heavy_mvm_SandwichTaunt12.mp3"
}

"Heavy.MVM_SandwichTaunt13"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/heavy_mvm_SandwichTaunt13.mp3"
}

"Heavy.MVM_SandwichTaunt14"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/heavy_mvm_SandwichTaunt14.mp3"
}

"Heavy.MVM_SandwichTaunt15"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/heavy_mvm_SandwichTaunt15.mp3"
}

"Heavy.MVM_SandwichTaunt16"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/heavy_mvm_SandwichTaunt16.mp3"
}

"Heavy.MVM_SandwichTaunt17"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/heavy_mvm_SandwichTaunt17.mp3"
}



//-----------------------------------------------------------------------------
//End of Heavy
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//Scout Batch 4
//-----------------------------------------------------------------------------
"Scout.MVM_ApexofJump01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_ApexofJump01.mp3"
}

"Scout.MVM_ApexofJump05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_ApexofJump05.mp3"
}

"Scout.MVM_ApexofJump02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_ApexofJump02.mp3"
}

"Scout.MVM_ApexofJump03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_ApexofJump03.mp3"
}

"Scout.MVM_ApexofJump04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_ApexofJump04.mp3"
}

"Scout.MVM_Award01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Award01.mp3"
}

"Scout.MVM_Award02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Award02.mp3"
}

"Scout.MVM_Award04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Award04.mp3"
}

"Scout.MVM_Award03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Award03.mp3"
}

"Scout.MVM_Award05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Award05.mp3"
}

"Scout.MVM_Award06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Award06.mp3"
}

"Scout.MVM_Award07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Award07.mp3"
}

"Scout.MVM_Award08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Award08.mp3"
}

"Scout.MVM_Award09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Award09.mp3"
}

"Scout.MVM_Award11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Award11.mp3"
}

"Scout.MVM_Award12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Award12.mp3"
}

"Scout.MVM_Award10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Award10.mp3"
}

"Scout.MVM_BeingShotInvincible04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible04.mp3"
}

"Scout.MVM_BeingShotInvincible07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible07.mp3"
}

"Scout.MVM_BeingShotInvincible02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible02.mp3"
}

"Scout.MVM_BeingShotInvincible01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible01.mp3"
}

"Scout.MVM_BeingShotInvincible08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible08.mp3"
}

"Scout.MVM_BeingShotInvincible09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible09.mp3"
}

"Scout.MVM_BeingShotInvincible03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible03.mp3"
}

"Scout.MVM_BeingShotInvincible10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible10.mp3"
}

"Scout.MVM_BeingShotInvincible06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible06.mp3"
}

"Scout.MVM_BeingShotInvincible05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible05.mp3"
}

"Scout.MVM_BeingShotInvincible11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible11.mp3"
}

"Scout.MVM_BeingShotInvincible12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible12.mp3"
}

"Scout.MVM_BeingShotInvincible13"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible13.mp3"
}

"Scout.MVM_BeingShotInvincible14"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible14.mp3"
}

"Scout.MVM_BeingShotInvincible15"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible15.mp3"
}

"Scout.MVM_BeingShotInvincible16"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible16.mp3"
}

"Scout.MVM_BeingShotInvincible17"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible17.mp3"
}

"Scout.MVM_BeingShotInvincible18"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible18.mp3"
}

"Scout.MVM_BeingShotInvincible19"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible19.mp3"
}

"Scout.MVM_BeingShotInvincible20"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible20.mp3"
}

"Scout.MVM_BeingShotInvincible21"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible21.mp3"
}

"Scout.MVM_BeingShotInvincible22"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible22.mp3"
}

"Scout.MVM_BeingShotInvincible23"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible23.mp3"
}

"Scout.MVM_BeingShotInvincible24"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible24.mp3"
}

"Scout.MVM_BeingShotInvincible25"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible25.mp3"
}

"Scout.MVM_BeingShotInvincible26"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible26.mp3"
}

"Scout.MVM_BeingShotInvincible27"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible27.mp3"
}

"Scout.MVM_BeingShotInvincible28"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible28.mp3"
}

"Scout.MVM_BeingShotInvincible29"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible29.mp3"
}

"Scout.MVM_BeingShotInvincible30"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible30.mp3"
}

"Scout.MVM_BeingShotInvincible31"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible31.mp3"
}

"Scout.MVM_BeingShotInvincible32"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible32.mp3"
}

"Scout.MVM_BeingShotInvincible33"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible33.mp3"
}

"Scout.MVM_BeingShotInvincible34"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible34.mp3"
}

"Scout.MVM_BeingShotInvincible35"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible35.mp3"
}

"Scout.MVM_BeingShotInvincible36"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_BeingShotInvincible36.mp3"
}

"Scout.MVM_CartGoingBackDefense03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartGoingBackDefense03.mp3"
}

"Scout.MVM_CartGoingBackDefense01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartGoingBackDefense01.mp3"
}

"Scout.MVM_CartGoingBackDefense02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartGoingBackDefense02.mp3"
}

"Scout.MVM_CartGoingBackDefense04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartGoingBackDefense04.mp3"
}

"Scout.MVM_CartGoingBackDefense05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartGoingBackDefense05.mp3"
}

"Scout.MVM_CartGoingBackDefense06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartGoingBackDefense06.mp3"
}

"Scout.MVM_CartGoingBackOffense01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartGoingBackOffense01.mp3"
}

"Scout.MVM_CartGoingBackOffense02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartGoingBackOffense02.mp3"
}

"Scout.MVM_CartGoingBackOffense03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartGoingBackOffense03.mp3"
}

"Scout.MVM_CartGoingBackOffense07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartGoingBackOffense07.mp3"
}

"Scout.MVM_CartGoingBackOffense04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartGoingBackOffense04.mp3"
}

"Scout.MVM_CartGoingBackOffense08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartGoingBackOffense08.mp3"
}

"Scout.MVM_CartGoingBackOffense05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartGoingBackOffense05.mp3"
}

"Scout.MVM_CartGoingBackOffense06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartGoingBackOffense06.mp3"
}

"Scout.MVM_CartMovingForwardDefense01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartMovingForwardDefense01.mp3"
}

"Scout.MVM_CartMovingForwardDefense02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartMovingForwardDefense02.mp3"
}

"Scout.MVM_CartMovingForwardDefense03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartMovingForwardDefense03.mp3"
}

"Scout.MVM_CartMovingForwardDefense04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartMovingForwardDefense04.mp3"
}

"Scout.MVM_CartMovingForwardDefense05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartMovingForwardDefense05.mp3"
}

"Scout.MVM_CartMovingForwardDefense06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartMovingForwardDefense06.mp3"
}

"Scout.MVM_CartMovingForwardOffense01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartMovingForwardOffense01.mp3"
}

"Scout.MVM_CartMovingForwardOffense02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartMovingForwardOffense02.mp3"
}

"Scout.MVM_CartMovingForwardOffense03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartMovingForwardOffense03.mp3"
}

"Scout.MVM_CartMovingForwardOffense04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartMovingForwardOffense04.mp3"
}

"Scout.MVM_CartMovingForwardOffense06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartMovingForwardOffense06.mp3"
}

"Scout.MVM_CartMovingForwardOffense05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartMovingForwardOffense05.mp3"
}

"Scout.MVM_CartStayCloseOffense01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartStayCloseOffense01.mp3"
}

"Scout.MVM_CartStayCloseOffense04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartStayCloseOffense04.mp3"
}

"Scout.MVM_CartStayCloseOffense02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartStayCloseOffense02.mp3"
}

"Scout.MVM_CartStayCloseOffense03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartStayCloseOffense03.mp3"
}

"Scout.MVM_CartStayCloseOffense05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartStayCloseOffense05.mp3"
}

"Scout.MVM_CartStayCloseOffense06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartStayCloseOffense06.mp3"
}

"Scout.MVM_CartStopItDefense01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartStopItDefense01.mp3"
}

"Scout.MVM_CartStopItDefense02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartStopItDefense02.mp3"
}

"Scout.MVM_CartStopItDefense03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartStopItDefense03.mp3"
}

"Scout.MVM_CartStoppedOffense01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartStoppedOffense01.mp3"
}

"Scout.MVM_CartStoppedOffense02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartStoppedOffense02.mp3"
}

"Scout.MVM_CartStoppedOffense03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_CartStoppedOffense03.mp3"
}

"Scout.MVM_Domination02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination02.mp3"
}

"Scout.MVM_Domination03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination03.mp3"
}

"Scout.MVM_Domination04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination04.mp3"
}

"Scout.MVM_Domination19"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination19.mp3"
}

"Scout.MVM_Domination05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination05.mp3"
}

"Scout.MVM_Domination18"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination18.mp3"
}

"Scout.MVM_Domination06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination06.mp3"
}

"Scout.MVM_Domination07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination07.mp3"
}

"Scout.MVM_Domination08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination08.mp3"
}

"Scout.MVM_Domination09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination09.mp3"
}

"Scout.MVM_Domination10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination10.mp3"
}

"Scout.MVM_Domination11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination11.mp3"
}

"Scout.MVM_Domination20"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination20.mp3"
}

"Scout.MVM_Domination12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination12.mp3"
}

"Scout.MVM_Domination13"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination13.mp3"
}

"Scout.MVM_Domination14"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination14.mp3"
}

"Scout.MVM_Domination01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination01.mp3"
}

"Scout.MVM_Domination15"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination15.mp3"
}

"Scout.MVM_Domination16"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination16.mp3"
}

"Scout.MVM_Domination17"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination17.mp3"
}

"Scout.MVM_Domination21"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Domination21.mp3"
}

"Scout.MVM_DominationDem01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationDem01.mp3"
}

"Scout.MVM_DominationDem02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationDem02.mp3"
}

"Scout.MVM_DominationDem05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationDem05.mp3"
}

"Scout.MVM_DominationDem03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationDem03.mp3"
}

"Scout.MVM_DominationDem04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationDem04.mp3"
}

"Scout.MVM_DominationEng01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationEng01.mp3"
}

"Scout.MVM_DominationEng02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationEng02.mp3"
}

"Scout.MVM_DominationEng03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationEng03.mp3"
}

"Scout.MVM_DominationEng04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationEng04.mp3"
}

"Scout.MVM_DominationEng05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationEng05.mp3"
}

"Scout.MVM_DominationEng06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationEng06.mp3"
}

"Scout.MVM_DominationHvy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationHvy01.mp3"
}

"Scout.MVM_DominationHvy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationHvy02.mp3"
}

"Scout.MVM_DominationHvy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationHvy03.mp3"
}

"Scout.MVM_DominationHvy04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationHvy04.mp3"
}

"Scout.MVM_DominationHvy05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationHvy05.mp3"
}

"Scout.MVM_DominationHvy06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationHvy06.mp3"
}

"Scout.MVM_DominationHvy07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationHvy07.mp3"
}

"Scout.MVM_DominationHvy08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationHvy08.mp3"
}

"Scout.MVM_DominationHvy09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationHvy09.mp3"
}

"Scout.MVM_DominationHvy10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationHvy10.mp3"
}

"Scout.MVM_DominationMed06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationMed06.mp3"
}

"Scout.MVM_DominationMed05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationMed05.mp3"
}

"Scout.MVM_DominationMed01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationMed01.mp3"
}

"Scout.MVM_DominationMed02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationMed02.mp3"
}

"Scout.MVM_DominationMed03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationMed03.mp3"
}

"Scout.MVM_DominationMed04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationMed04.mp3"
}

"Scout.MVM_DominationPyr01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationPyr01.mp3"
}

"Scout.MVM_DominationPyr02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationPyr02.mp3"
}

"Scout.MVM_DominationPyr03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationPyr03.mp3"
}

"Scout.MVM_DominationPyr06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationPyr06.mp3"
}

"Scout.MVM_DominationPyr04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationPyr04.mp3"
}

"Scout.MVM_DominationPyr05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationPyr05.mp3"
}

"Scout.MVM_DominationSct01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationSct01.mp3"
}

"Scout.MVM_DominationSct02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationSct02.mp3"
}

"Scout.MVM_DominationSct03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationSct03.mp3"
}

"Scout.MVM_DominationSnp01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationSnp01.mp3"
}

"Scout.MVM_DominationSnp02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationSnp02.mp3"
}

"Scout.MVM_DominationSnp03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationSnp03.mp3"
}

"Scout.MVM_DominationSnp04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationSnp04.mp3"
}

"Scout.MVM_DominationSnp05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationSnp05.mp3"
}

"Scout.MVM_DominationSol01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationSol01.mp3"
}

"Scout.MVM_DominationSol02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationSol02.mp3"
}

"Scout.MVM_DominationSol03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationSol03.mp3"
}

"Scout.MVM_DominationSol04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationSol04.mp3"
}

"Scout.MVM_DominationSol05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationSol05.mp3"
}

"Scout.MVM_DominationSol06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationSol06.mp3"
}

"Scout.MVM_DominationSpy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationSpy01.mp3"
}

"Scout.MVM_DominationSpy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationSpy02.mp3"
}

"Scout.MVM_DominationSpy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationSpy03.mp3"
}

"Scout.MVM_DominationSpy04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_DominationSpy04.mp3"
}

"Scout.MVM_FightOnCap01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_FightOnCap01.mp3"
}

"Scout.MVM_FightOnCap02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_FightOnCap02.mp3"
}

"Scout.MVM_FightOnCap03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_FightOnCap03.mp3"
}

"Scout.MVM_FightOnCap04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_FightOnCap04.mp3"
}

"Scout.MVM_Invincible01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Invincible01.mp3"
}

"Scout.MVM_Invincible02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Invincible02.mp3"
}

"Scout.MVM_Invincible04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Invincible04.mp3"
}

"Scout.MVM_Invincible03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Invincible03.mp3"
}

"Scout.MVM_InvincibleChgUnderFire01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_InvincibleChgUnderFire01.mp3"
}

"Scout.MVM_InvincibleChgUnderFire02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_InvincibleChgUnderFire02.mp3"
}

"Scout.MVM_InvincibleChgUnderFire04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_InvincibleChgUnderFire04.mp3"
}

"Scout.MVM_InvincibleChgUnderFire03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_InvincibleChgUnderFire03.mp3"
}

"Scout.MVM_InvincibleNotReady01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_InvincibleNotReady01.mp3"
}

"Scout.MVM_InvincibleNotReady02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_InvincibleNotReady02.mp3"
}

"Scout.MVM_InvincibleNotReady03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_InvincibleNotReady03.mp3"
}

"Scout.MVM_InvincibleNotReady04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_InvincibleNotReady04.mp3"
}

"Scout.MVM_InvincibleNotReady05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_InvincibleNotReady05.mp3"
}

"Scout.MVM_InvincibleNotReady06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_InvincibleNotReady06.mp3"
}

"Scout.MVM_InvincibleNotReady07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_InvincibleNotReady07.mp3"
}

"Scout.MVM_MedicFollow01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_MedicFollow01.mp3"
}

"Scout.MVM_MedicFollow02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_MedicFollow02.mp3"
}

"Scout.MVM_MedicFollow03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_MedicFollow03.mp3"
}

"Scout.MVM_MedicFollow04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_MedicFollow04.mp3"
}

"Scout.MVM_MeleeDare01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_MeleeDare01.mp3"
}

"Scout.MVM_MeleeDare02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_MeleeDare02.mp3"
}

"Scout.MVM_MeleeDare06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_MeleeDare06.mp3"
}

"Scout.MVM_MeleeDare03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_MeleeDare03.mp3"
}

"Scout.MVM_MeleeDare04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_MeleeDare04.mp3"
}

"Scout.MVM_MeleeDare05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_MeleeDare05.mp3"
}

"Scout.MVM_Misc01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Misc01.mp3"
}

"Scout.MVM_Misc02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Misc02.mp3"
}

"Scout.MVM_Misc03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Misc03.mp3"
}

"Scout.MVM_Misc04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Misc04.mp3"
}

"Scout.MVM_Misc05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Misc05.mp3"
}

"Scout.MVM_Misc06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Misc06.mp3"
}

"Scout.MVM_Misc07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Misc07.mp3"
}

"Scout.MVM_Misc08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Misc08.mp3"
}

"Scout.MVM_Misc09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Misc09.mp3"
}

"Scout.MVM_Revenge01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Revenge01.mp3"
}

"Scout.MVM_Revenge02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Revenge02.mp3"
}

"Scout.MVM_Revenge03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Revenge03.mp3"
}

"Scout.MVM_Revenge04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Revenge04.mp3"
}

"Scout.MVM_Revenge05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Revenge05.mp3"
}

"Scout.MVM_Revenge06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Revenge06.mp3"
}

"Scout.MVM_Revenge07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Revenge07.mp3"
}

"Scout.MVM_Revenge08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Revenge08.mp3"
}

"Scout.MVM_Revenge09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_Revenge09.mp3"
}

"Scout.MVM_StunBallHit02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHit02.mp3"
}

"Scout.MVM_StunBallHit03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHit03.mp3"
}

"Scout.MVM_StunBallHit04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHit04.mp3"
}

"Scout.MVM_StunBallHit05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHit05.mp3"
}

"Scout.MVM_StunBallHit06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHit06.mp3"
}

"Scout.MVM_StunBallHit07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHit07.mp3"
}

"Scout.MVM_StunBallHit08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHit08.mp3"
}

"Scout.MVM_StunBallHit09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHit09.mp3"
}

"Scout.MVM_StunBallHit10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHit10.mp3"
}

"Scout.MVM_StunBallHit15"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHit15.mp3"
}

"Scout.MVM_StunBallHit11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHit11.mp3"
}

"Scout.MVM_StunBallHit16"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHit16.mp3"
}

"Scout.MVM_StunBallHit12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHit12.mp3"
}

"Scout.MVM_StunBallHit13"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHit13.mp3"
}

"Scout.MVM_StunBallHit14"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHit14.mp3"
}

"Scout.MVM_StunBallHit01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHit01.mp3"
}

"Scout.MVM_StunBallHittingIt02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHittingIt02.mp3"
}

"Scout.MVM_StunBallHittingIt01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHittingIt01.mp3"
}

"Scout.MVM_StunBallHittingIt04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHittingIt04.mp3"
}

"Scout.MVM_StunBallHittingIt03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHittingIt03.mp3"
}

"Scout.MVM_StunBallHittingIt05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallHittingIt05.mp3"
}

"Scout.MVM_StunBallPickUp01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallPickUp01.mp3"
}

"Scout.MVM_StunBallPickUp02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallPickUp02.mp3"
}

"Scout.MVM_StunBallPickUp03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallPickUp03.mp3"
}

"Scout.MVM_StunBallPickUp04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallPickUp04.mp3"
}

"Scout.MVM_StunBallPickUp05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_StunBallPickUp05.mp3"
}

"Scout.MVM_TripleJump01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_TripleJump01.mp3"
}

"Scout.MVM_TripleJump03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_TripleJump03.mp3"
}

"Scout.MVM_TripleJump02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_TripleJump02.mp3"
}

"Scout.MVM_TripleJump04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"0.820"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/scout_mvm_TripleJump04.mp3"
}

//-----------------------------------------------------------------------------
//End of Scout Batch 4
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
//Spy batch 6
//-----------------------------------------------------------------------------

"Spy.MVM_DominationDemoMan01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationDemoMan01.mp3"
}

"Spy.MVM_DominationDemoMan02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationDemoMan02.mp3"
}

"Spy.MVM_DominationDemoMan03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationDemoMan03.mp3"
}

"Spy.MVM_DominationDemoMan04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationDemoMan04.mp3"
}

"Spy.MVM_DominationDemoMan05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationDemoMan05.mp3"
}

"Spy.MVM_DominationDemoMan06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationDemoMan06.mp3"
}

"Spy.MVM_DominationDemoMan07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationDemoMan07.mp3"
}

"Spy.MVM_DominationEngineer01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationEngineer01.mp3"
}

"Spy.MVM_DominationEngineer02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationEngineer02.mp3"
}

"Spy.MVM_DominationEngineer03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationEngineer03.mp3"
}

"Spy.MVM_DominationEngineer04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationEngineer04.mp3"
}

"Spy.MVM_DominationEngineer05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationEngineer05.mp3"
}

"Spy.MVM_DominationEngineer06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationEngineer06.mp3"
}

"Spy.MVM_DominationHeavy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationHeavy01.mp3"
}

"Spy.MVM_DominationHeavy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationHeavy02.mp3"
}

"Spy.MVM_DominationHeavy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationHeavy03.mp3"
}

"Spy.MVM_DominationHeavy04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationHeavy04.mp3"
}

"Spy.MVM_DominationHeavy05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationHeavy05.mp3"
}

"Spy.MVM_DominationHeavy06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationHeavy06.mp3"
}

"Spy.MVM_DominationHeavy07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationHeavy07.mp3"
}

"Spy.MVM_DominationHeavy08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationHeavy08.mp3"
}

"Spy.MVM_DominationMedic01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationMedic01.mp3"
}

"Spy.MVM_DominationMedic02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationMedic02.mp3"
}

"Spy.MVM_DominationMedic03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationMedic03.mp3"
}

"Spy.MVM_DominationMedic04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationMedic04.mp3"
}

"Spy.MVM_DominationMedic05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationMedic05.mp3"
}

"Spy.MVM_DominationMedic06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationMedic06.mp3"
}

"Spy.MVM_DominationPyro01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationPyro01.mp3"
}

"Spy.MVM_DominationPyro02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationPyro02.mp3"
}

"Spy.MVM_DominationPyro03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationPyro03.mp3"
}

"Spy.MVM_DominationPyro04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationPyro04.mp3"
}

"Spy.MVM_DominationPyro05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationPyro05.mp3"
}

"Spy.MVM_DominationScout01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationScout01.mp3"
}

"Spy.MVM_DominationScout02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationScout02.mp3"
}

"Spy.MVM_DominationScout03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationScout03.mp3"
}

"Spy.MVM_DominationScout04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationScout04.mp3"
}

"Spy.MVM_DominationScout05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationScout05.mp3"
}

"Spy.MVM_DominationScout06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationScout06.mp3"
}

"Spy.MVM_DominationScout07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationScout07.mp3"
}

"Spy.MVM_DominationScout08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationScout08.mp3"
}

"Spy.MVM_DominationSniper01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationSniper01.mp3"
}

"Spy.MVM_DominationSniper02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationSniper02.mp3"
}

"Spy.MVM_DominationSniper03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationSniper03.mp3"
}

"Spy.MVM_DominationSniper04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationSniper04.mp3"
}

"Spy.MVM_DominationSniper05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationSniper05.mp3"
}

"Spy.MVM_DominationSniper06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationSniper06.mp3"
}

"Spy.MVM_DominationSniper07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationSniper07.mp3"
}

"Spy.MVM_DominationSoldier01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationSoldier01.mp3"
}

"Spy.MVM_DominationSoldier02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationSoldier02.mp3"
}

"Spy.MVM_DominationSoldier03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationSoldier03.mp3"
}

"Spy.MVM_DominationSoldier04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationSoldier04.mp3"
}

"Spy.MVM_DominationSoldier05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationSoldier05.mp3"
}

"Spy.MVM_DominationSpy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationSpy01.mp3"
}

"Spy.MVM_DominationSpy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationSpy02.mp3"
}

"Spy.MVM_DominationSpy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationSpy03.mp3"
}

"Spy.MVM_DominationSpy04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationSpy04.mp3"
}

"Spy.MVM_DominationSpy05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_DominationSpy05.mp3"
}

"Spy.MVM_JarateHit01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_JarateHit01.mp3"
}

"Spy.MVM_JarateHit02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_JarateHit02.mp3"
}

"Spy.MVM_JarateHit03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_JarateHit03.mp3"
}

"Spy.MVM_JarateHit04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_JarateHit04.mp3"
}

"Spy.MVM_JarateHit05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_JarateHit05.mp3"
}

"Spy.MVM_JarateHit06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_JarateHit06.mp3"
}

"Spy.MVM_MedicFollow01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_MedicFollow01.mp3"
}

"Spy.MVM_MedicFollow02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_MedicFollow02.mp3"
}

"Spy.MVM_MeleeDare01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_MeleeDare01.mp3"
}

"Spy.MVM_MeleeDare02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_MeleeDare02.mp3"
}

"Spy.MVM_Revenge01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_Revenge01.mp3"
}

"Spy.MVM_Revenge02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_Revenge02.mp3"
}

"Spy.MVM_Revenge03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/spy_mvm_Revenge03.mp3"
}

//-----------------------------------------------------------------------------
//End of Spy batch 6
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//Sniper BATCH 7
//-----------------------------------------------------------------------------

"Sniper.MVM_DominationDemoMan01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationDemoMan01.mp3"
}

"Sniper.MVM_DominationDemoMan02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationDemoMan02.mp3"
}

"Sniper.MVM_DominationDemoMan03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationDemoMan03.mp3"
}

"Sniper.MVM_DominationDemoMan04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationDemoMan04.mp3"
}

"Sniper.MVM_DominationDemoMan05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationDemoMan05.mp3"
}

"Sniper.MVM_DominationEngineer01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationEngineer01.mp3"
}

"Sniper.MVM_DominationEngineer02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationEngineer02.mp3"
}

"Sniper.MVM_DominationEngineer03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationEngineer03.mp3"
}

"Sniper.MVM_DominationEngineer04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationEngineer04.mp3"
}

"Sniper.MVM_DominationEngineer05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationEngineer05.mp3"
}

"Sniper.MVM_DominationEngineer06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationEngineer06.mp3"
}

"Sniper.MVM_DominationHeavy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationHeavy01.mp3"
}

"Sniper.MVM_DominationHeavy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationHeavy02.mp3"
}

"Sniper.MVM_DominationHeavy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationHeavy03.mp3"
}

"Sniper.MVM_DominationHeavy04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationHeavy04.mp3"
}

"Sniper.MVM_DominationHeavy05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationHeavy05.mp3"
}

"Sniper.MVM_DominationHeavy06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationHeavy06.mp3"
}

"Sniper.MVM_DominationHeavy07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationHeavy07.mp3"
}

"Sniper.MVM_DominationMedic01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationMedic01.mp3"
}

"Sniper.MVM_DominationMedic02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationMedic02.mp3"
}

"Sniper.MVM_DominationMedic03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationMedic03.mp3"
}

"Sniper.MVM_DominationMedic04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationMedic04.mp3"
}

"Sniper.MVM_DominationMedic05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationMedic05.mp3"
}

"Sniper.MVM_DominationPyro01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationPyro01.mp3"
}

"Sniper.MVM_DominationPyro02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationPyro02.mp3"
}

"Sniper.MVM_DominationPyro03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationPyro03.mp3"
}

"Sniper.MVM_DominationPyro04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationPyro04.mp3"
}

"Sniper.MVM_DominationPyro05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationPyro05.mp3"
}

"Sniper.MVM_DominationScout01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationScout01.mp3"
}

"Sniper.MVM_DominationScout02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationScout02.mp3"
}

"Sniper.MVM_DominationScout03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationScout03.mp3"
}

"Sniper.MVM_DominationScout04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationScout04.mp3"
}

"Sniper.MVM_DominationScout05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationScout05.mp3"
}

"Sniper.MVM_DominationSniper18"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationSniper18.mp3"
}

"Sniper.MVM_DominationSniper19"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationSniper19.mp3"
}

"Sniper.MVM_DominationSniper20"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationSniper20.mp3"
}

"Sniper.MVM_DominationSniper21"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationSniper21.mp3"
}

"Sniper.MVM_DominationSniper22"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationSniper22.mp3"
}

"Sniper.MVM_DominationSoldier01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationSoldier01.mp3"
}

"Sniper.MVM_DominationSoldier02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationSoldier02.mp3"
}

"Sniper.MVM_DominationSoldier03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationSoldier03.mp3"
}

"Sniper.MVM_DominationSoldier04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationSoldier04.mp3"
}

"Sniper.MVM_DominationSoldier05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationSoldier05.mp3"
}

"Sniper.MVM_DominationSoldier06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationSoldier06.mp3"
}

"Sniper.MVM_DominationSpy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationSpy01.mp3"
}

"Sniper.MVM_DominationSpy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationSpy02.mp3"
}

"Sniper.MVM_DominationSpy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationSpy03.mp3"
}

"Sniper.MVM_DominationSpy04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationSpy04.mp3"
}

"Sniper.MVM_DominationSpy05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationSpy05.mp3"
}

"Sniper.MVM_DominationSpy06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationSpy06.mp3"
}

"Sniper.MVM_DominationSpy07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_DominationSpy07.mp3"
}

"Sniper.MVM_JarateToss01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_JarateToss01.mp3"
}

"Sniper.MVM_JarateToss02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_JarateToss02.mp3"
}

"Sniper.MVM_JarateToss03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/sniper_mvm_JarateToss03.mp3"
}

//-----------------------------------------------------------------------------
//End of Sniper BATCH 7
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//Soldier BATCH 9
//-----------------------------------------------------------------------------

"Soldier.MVM_DirectHitTaunt01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DirectHitTaunt01.mp3"
}

"Soldier.MVM_DirectHitTaunt02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DirectHitTaunt02.mp3"
}

"Soldier.MVM_DirectHitTaunt03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DirectHitTaunt03.mp3"
}

"Soldier.MVM_DirectHitTaunt04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DirectHitTaunt04.mp3"
}

"Soldier.MVM_DominationDemoman01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationDemoman01.mp3"
}

"Soldier.MVM_DominationDemoman02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationDemoman02.mp3"
}

"Soldier.MVM_DominationDemoman03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationDemoman03.mp3"
}

"Soldier.MVM_DominationDemoman04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationDemoman04.mp3"
}

"Soldier.MVM_DominationDemoman05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationDemoman05.mp3"
}

"Soldier.MVM_DominationDemoman06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationDemoman06.mp3"
}

"Soldier.MVM_DominationDemonmanUpdate01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationDemonmanUpdate01.mp3"
}

"Soldier.MVM_DominationDemonmanUpdate02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationDemonmanUpdate02.mp3"
}

"Soldier.MVM_DominationDemonmanUpdate03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationDemonmanUpdate03.mp3"
}

"Soldier.MVM_DominationDemonmanUpdate04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationDemonmanUpdate04.mp3"
}

"Soldier.MVM_DominationDemonmanUpdate05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationDemonmanUpdate05.mp3"
}

"Soldier.MVM_DominationEngineer01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationEngineer01.mp3"
}

"Soldier.MVM_DominationEngineer02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationEngineer02.mp3"
}

"Soldier.MVM_DominationEngineer03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationEngineer03.mp3"
}

"Soldier.MVM_DominationEngineer04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationEngineer04.mp3"
}

"Soldier.MVM_DominationEngineer05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationEngineer05.mp3"
}

"Soldier.MVM_DominationEngineer06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationEngineer06.mp3"
}

"Soldier.MVM_DominationHeavy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationHeavy01.mp3"
}

"Soldier.MVM_DominationHeavy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationHeavy02.mp3"
}

"Soldier.MVM_DominationHeavy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationHeavy03.mp3"
}

"Soldier.MVM_DominationHeavy04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationHeavy04.mp3"
}

"Soldier.MVM_DominationHeavy05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationHeavy05.mp3"
}

"Soldier.MVM_DominationHeavy06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationHeavy06.mp3"
}

"Soldier.MVM_DominationHeavy07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationHeavy07.mp3"
}

"Soldier.MVM_DominationMedic01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationMedic01.mp3"
}

"Soldier.MVM_DominationMedic02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationMedic02.mp3"
}

"Soldier.MVM_DominationMedic03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationMedic03.mp3"
}

"Soldier.MVM_DominationMedic04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationMedic04.mp3"
}

"Soldier.MVM_DominationMedic05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationMedic05.mp3"
}

"Soldier.MVM_DominationMedic06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationMedic06.mp3"
}

"Soldier.MVM_DominationMedic07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationMedic07.mp3"
}

"Soldier.MVM_DominationPyro01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationPyro01.mp3"
}

"Soldier.MVM_DominationPyro02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationPyro02.mp3"
}

"Soldier.MVM_DominationPyro03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationPyro03.mp3"
}

"Soldier.MVM_DominationPyro04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationPyro04.mp3"
}

"Soldier.MVM_DominationPyro05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationPyro05.mp3"
}

"Soldier.MVM_DominationPyro06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationPyro06.mp3"
}

"Soldier.MVM_DominationPyro07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationPyro07.mp3"
}

"Soldier.MVM_DominationPyro08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationPyro08.mp3"
}

"Soldier.MVM_DominationPyro09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationPyro09.mp3"
}

"Soldier.MVM_DominationScout01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationScout01.mp3"
}

"Soldier.MVM_DominationScout02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationScout02.mp3"
}

"Soldier.MVM_DominationScout03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationScout03.mp3"
}

"Soldier.MVM_DominationScout04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationScout04.mp3"
}

"Soldier.MVM_DominationScout05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationScout05.mp3"
}

"Soldier.MVM_DominationScout06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationScout06.mp3"
}

"Soldier.MVM_DominationScout07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationScout07.mp3"
}

"Soldier.MVM_DominationScout08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationScout08.mp3"
}

"Soldier.MVM_DominationScout09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationScout09.mp3"
}

"Soldier.MVM_DominationScout10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationScout10.mp3"
}

"Soldier.MVM_DominationScout11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationScout11.mp3"
}

"Soldier.MVM_DominationSniper01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSniper01.mp3"
}

"Soldier.MVM_DominationSniper02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSniper02.mp3"
}

"Soldier.MVM_DominationSniper03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSniper03.mp3"
}

"Soldier.MVM_DominationSniper04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSniper04.mp3"
}

"Soldier.MVM_DominationSniper05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSniper05.mp3"
}

"Soldier.MVM_DominationSniper06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSniper06.mp3"
}

"Soldier.MVM_DominationSniper07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSniper07.mp3"
}

"Soldier.MVM_DominationSniper08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSniper08.mp3"
}

"Soldier.MVM_DominationSniper09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSniper09.mp3"
}

"Soldier.MVM_DominationSniper10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSniper10.mp3"
}

"Soldier.MVM_DominationSniper11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSniper11.mp3"
}

"Soldier.MVM_DominationSniper12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSniper12.mp3"
}

"Soldier.MVM_DominationSniper13"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSniper13.mp3"
}

"Soldier.MVM_DominationSniper14"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSniper14.mp3"
}

"Soldier.MVM_DominationSoldier01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSoldier01.mp3"
}

"Soldier.MVM_DominationSoldier02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSoldier02.mp3"
}

"Soldier.MVM_DominationSoldier03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSoldier03.mp3"
}

"Soldier.MVM_DominationSoldier04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSoldier04.mp3"
}

"Soldier.MVM_DominationSoldier05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSoldier05.mp3"
}

"Soldier.MVM_DominationSoldier06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSoldier06.mp3"
}

"Soldier.MVM_DominationSpy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSpy01.mp3"
}

"Soldier.MVM_DominationSpy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSpy02.mp3"
}

"Soldier.MVM_DominationSpy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSpy03.mp3"
}

"Soldier.MVM_DominationSpy04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSpy04.mp3"
}

"Soldier.MVM_DominationSpy05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSpy05.mp3"
}

"Soldier.MVM_DominationSpy06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSpy06.mp3"
}

"Soldier.MVM_DominationSpy07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSpy07.mp3"
}

"Soldier.MVM_DominationSpy08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_DominationSpy08.mp3"
}

"Soldier.MVM_HatOverHeartTaunt01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_HatOverHeartTaunt01.mp3"
}

"Soldier.MVM_HatOverHeartTaunt02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_HatOverHeartTaunt02.mp3"
}

"Soldier.MVM_HatOverHeartTaunt03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_HatOverHeartTaunt03.mp3"
}

"Soldier.MVM_HatOverHeartTaunt04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_HatOverHeartTaunt04.mp3"
}

"Soldier.MVM_HatOverHeartTaunt05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_HatOverHeartTaunt05.mp3"
}

"Soldier.MVM_HatOverHeartTaunt06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_HatOverHeartTaunt06.mp3"
}

"Soldier.MVM_KaBoomAlts01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_KaBoomAlts01.mp3"
}

"Soldier.MVM_KaBoomAlts02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_KaBoomAlts02.mp3"
}

"Soldier.MVM_KaBoomAlts03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_KaBoomAlts03.mp3"
}

"Soldier.MVM_PickAxeTaunt01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_PickAxeTaunt01.mp3"
}

"Soldier.MVM_PickAxeTaunt02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_PickAxeTaunt02.mp3"
}

"Soldier.MVM_PickAxeTaunt03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_PickAxeTaunt03.mp3"
}

"Soldier.MVM_PickAxeTaunt04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_PickAxeTaunt04.mp3"
}

"Soldier.MVM_PickAxeTaunt05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/soldier_mvm_PickAxeTaunt05.mp3"
}

//-----------------------------------------------------------------------------
//End of Soldier BATCH 9
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//Demoman BATCH 10
//-----------------------------------------------------------------------------

"Demoman.MVM_dominationdemoman01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationdemoman01.mp3"
}

"Demoman.MVM_dominationdemoman02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationdemoman02.mp3"
}

"Demoman.MVM_dominationdemoman03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationdemoman03.mp3"
}

"Demoman.MVM_dominationdemoman04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationdemoman04.mp3"
}

"Demoman.MVM_dominationengineer01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationengineer01.mp3"
}

"Demoman.MVM_dominationengineer02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationengineer02.mp3"
}

"Demoman.MVM_dominationengineer03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationengineer03.mp3"
}

"Demoman.MVM_dominationengineer04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationengineer04.mp3"
}

"Demoman.MVM_dominationengineer05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationengineer05.mp3"
}

"Demoman.MVM_dominationengineer06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationengineer06.mp3"
}

"Demoman.MVM_dominationheavy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationheavy01.mp3"
}

"Demoman.MVM_dominationheavy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationheavy02.mp3"
}

"Demoman.MVM_dominationheavy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationheavy03.mp3"
}

"Demoman.MVM_dominationheavy04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationheavy04.mp3"
}

"Demoman.MVM_dominationheavy05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationheavy05.mp3"
}

"Demoman.MVM_dominationmedic01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationmedic01.mp3"
}

"Demoman.MVM_dominationmedic02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationmedic02.mp3"
}

"Demoman.MVM_dominationmedic03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationmedic03.mp3"
}

"Demoman.MVM_dominationmedic04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationmedic04.mp3"
}

"Demoman.MVM_dominationpyro01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationpyro01.mp3"
}

"Demoman.MVM_dominationpyro02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationpyro02.mp3"
}

"Demoman.MVM_dominationpyro03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationpyro03.mp3"
}

"Demoman.MVM_dominationpyro04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationpyro04.mp3"
}

"Demoman.MVM_dominationscout01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationscout01.mp3"
}

"Demoman.MVM_dominationscout02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationscout02.mp3"
}

"Demoman.MVM_dominationscout03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationscout03.mp3"
}

"Demoman.MVM_dominationscout04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationscout04.mp3"
}

"Demoman.MVM_dominationscout05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationscout05.mp3"
}

"Demoman.MVM_dominationscout06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationscout06.mp3"
}

"Demoman.MVM_dominationscout07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationscout07.mp3"
}

"Demoman.MVM_dominationscout08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationscout08.mp3"
}

"Demoman.MVM_dominationsniper01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationsniper01.mp3"
}

"Demoman.MVM_dominationsniper02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationsniper02.mp3"
}

"Demoman.MVM_dominationsniper03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationsniper03.mp3"
}

"Demoman.MVM_dominationsniper04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationsniper04.mp3"
}

"Demoman.MVM_dominationsoldier01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationsoldier01.mp3"
}

"Demoman.MVM_dominationsoldier02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationsoldier02.mp3"
}

"Demoman.MVM_dominationsoldier03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationsoldier03.mp3"
}

"Demoman.MVM_dominationsoldier04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationsoldier04.mp3"
}

"Demoman.MVM_dominationsoldier05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationsoldier05.mp3"
}

"Demoman.MVM_dominationsoldierupdate01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationsoldierupdate01.mp3"
}

"Demoman.MVM_dominationsoldierupdate02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationsoldierupdate02.mp3"
}

"Demoman.MVM_dominationsoldierupdate03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationsoldierupdate03.mp3"
}

"Demoman.MVM_dominationsoldierupdate04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationsoldierupdate04.mp3"
}

"Demoman.MVM_dominationsoldierupdate05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationsoldierupdate05.mp3"
}

"Demoman.MVM_dominationsoldierupdate06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationsoldierupdate06.mp3"
}

"Demoman.MVM_dominationspy01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationspy01.mp3"
}

"Demoman.MVM_dominationspy02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationspy02.mp3"
}

"Demoman.MVM_dominationspy03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_dominationspy03.mp3"
}

"Demoman.MVM_eyelandertaunt01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_eyelandertaunt01.mp3"
}

"Demoman.MVM_eyelandertaunt02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_eyelandertaunt02.mp3"
}

"Demoman.MVM_gibberish01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_gibberish01.mp3"
}

"Demoman.MVM_gibberish02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_gibberish02.mp3"
}

"Demoman.MVM_gibberish03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_gibberish03.mp3"
}

"Demoman.MVM_gibberish04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_gibberish04.mp3"
}

"Demoman.MVM_gibberish05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_gibberish05.mp3"
}

"Demoman.MVM_gibberish06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_gibberish06.mp3"
}

"Demoman.MVM_gibberish07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_gibberish07.mp3"
}

"Demoman.MVM_gibberish08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_gibberish08.mp3"
}

"Demoman.MVM_gibberish09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_gibberish09.mp3"
}

"Demoman.MVM_gibberish10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_gibberish10.mp3"
}

"Demoman.MVM_gibberish11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_gibberish11.mp3"
}

"Demoman.MVM_gibberish12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_gibberish12.mp3"
}

"Demoman.MVM_gibberish13"
{
	"channel"		"CHAN_VOICE"
	"volume"		"1.000"
	"pitch"		"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"		"vo/mvm/norm/demoman_mvm_gibberish13.mp3"
}

// -----------------------------------------------------------------------------
// End of Demoman BATCH 10
// -----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//Soldier Robot
//-----------------------------------------------------------------------------

"Soldier.MVM_Robot01"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot01.mp3"
}

"Soldier.MVM_Robot02"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot02.mp3"
}

"Soldier.MVM_Robot03"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot03.mp3"
}

"Soldier.MVM_Robot04"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot04.mp3"
}

"Soldier.MVM_Robot05"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot05.mp3"
}

"Soldier.MVM_Robot06"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot06.mp3"
}

"Soldier.MVM_Robot07"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot07.mp3"
}

"Soldier.MVM_Robot08"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot08.mp3"
}

"Soldier.MVM_Robot09"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot09.mp3"
}

"Soldier.MVM_Robot10"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot10.mp3"
}

"Soldier.MVM_Robot11"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot11.mp3"
}

"Soldier.MVM_Robot12"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot12.mp3"
}

"Soldier.MVM_Robot13"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot13.mp3"
}

"Soldier.MVM_Robot14"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot14.mp3"
}

"Soldier.MVM_Robot15"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot15.mp3"
}

"Soldier.MVM_Robot16"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot16.mp3"
}

"Soldier.MVM_Robot17"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot17.mp3"
}

"Soldier.MVM_Robot18"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot18.mp3"
}

"Soldier.MVM_Robot19"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot19.mp3"
}

"Soldier.MVM_Robot20"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot20.mp3"
}

"Soldier.MVM_Robot21"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot21.mp3"
}

"Soldier.MVM_Robot22"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot22.mp3"
}

"Soldier.MVM_Robot23"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot23.mp3"
}

"Soldier.MVM_Robot24"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot24.mp3"
}

"Soldier.MVM_Robot25"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot25.mp3"
}

"Soldier.MVM_Robot26"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot26.mp3"
}

"Soldier.MVM_Robot27"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot27.mp3"
}

"Soldier.MVM_Robot28"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot28.mp3"
}

"Soldier.MVM_Robot29"
{
	"channel"		"CHAN_VOICE"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"
	"soundlevel"		"SNDLVL_95dB"
	"wave"			"vo/mvm/norm/soldier_mvm_robot29.mp3"
}

"Credits.Updated"
{
	"channel"	"CHAN_STATIC"
	"volume"	"0.1"
	"soundlevel"  	"SNDLVL_NONE"
	"pitch"		"PITCH_NORM"
	"wave"		"ui/credits_updated.wav"
}

"Announcer.MVM_Engineer_Teleporter_Activated"
{
	"channel"		"CHAN_VOICE2"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_NORM"

	"rndwave"
	{
		"wave"	"vo/announcer_mvm_eng_tele_activated01.mp3"
		"wave"	"vo/announcer_mvm_eng_tele_activated02.mp3"
		"wave"	"vo/announcer_mvm_eng_tele_activated03.mp3"
		"wave"	"vo/announcer_mvm_eng_tele_activated04.mp3"
		"wave"	"vo/announcer_mvm_eng_tele_activated05.mp3"
	}
}

"Announcer.MVM_Another_Engineer_Teleport_Spawned"
{
	"channel"		"CHAN_VOICE2"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_NORM"

	"rndwave"
	{
		"wave"	"vo/announcer_mvm_engbot_another01.mp3"
		"wave"	"vo/announcer_mvm_engbot_another02.mp3"
	}
}

"Announcer.MVM_First_Engineer_Teleport_Spawned"
{
	"channel"		"CHAN_VOICE2"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_NORM"

	"rndwave"
	{
		"wave"	"vo/announcer_mvm_engbot_arrive01.mp3"
		"wave"	"vo/announcer_mvm_engbot_arrive02.mp3"
		"wave"	"vo/announcer_mvm_engbot_arrive03.mp3"
	}
}

"Announcer.MVM_An_Engineer_Bot_Is_Dead"
{
	"channel"		"CHAN_VOICE2"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_NORM"

	"rndwave"
	{
		"wave"	"vo/announcer_mvm_engbot_dead_notele01.mp3"
		"wave"	"vo/announcer_mvm_engbot_dead_notele02.mp3"
		"wave"	"vo/announcer_mvm_engbot_dead_notele03.mp3"
	}
}

"Announcer.MVM_An_Engineer_Bot_Is_Dead_But_Not_Teleporter"
{
	"channel"		"CHAN_VOICE2"
	"volume"		"VOL_NORM"
	"pitch"			"PITCH_NORM"

	"soundlevel"	"SNDLVL_NORM"

	"rndwave"
	{
		"wave"	"vo/announcer_mvm_engbot_dead_tele01.mp3"
		"wave"	"vo/announcer_mvm_engbot_dead_tele02.mp3"
	}
}
