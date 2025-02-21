"Resource/UI/MatchMakingTooltip.res"
{
	"CompRanksTooltip"
	{
	//	"ControlName"	"EditablePanel"
		"fieldName"		"CompRanksTooltip"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"30000"
		"wide"			"320"
		"tall"			"400"
		"visible"		"0"
		"PaintBackgroundType"	"2"
		"border"		"ReplayDefaultBorder"
		"mouseinputenabled"	"0"
	}

	"TitleLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"TitleLabel"
		"font"			"XPSource"
		"labelText"		"#TF_Competitive_RankTooltipTitle"
		"textAlignment"	"north"
		"xpos"			"cs-0.5"
		"ypos"			"5"
		"zpos"			"2"
		"wide"			"f0"
		"tall"			"40"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"fgcolor_override" "TanLight"
		"proportionaltoparent" "1"
		"mouseinputenabled"	"0"
	}
	
	"DescLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"DescLabel"
		"font"			"MMenuPlayListDesc"
		"labelText"		"#TF_Competitive_RankTooltipDesc"
		"xpos"			"cs-0.5"
		"ypos"			"15"
		"zpos"			"2"
		"wide"			"f20"
		"tall"			"40"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"wrap"			"1"
		"fgcolor_override" "TanLight"
		"proportionaltoparent" "1"
		"mouseinputenabled"	"0"
	}	

	"RanksContainer"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"RanksContainer"
		"xpos"			"0"
		"ypos"			"30"
		"wide"			"f0"
		"tall"			"f0"
		"proportionaltoparent"	"1"


		// ----------------- Row 1 -------------------
		"Rank1"
		{
			"ControlName"	"CTFStaticBadgePanel"
			"fieldName"		"Rank1"
			"xpos"			"0"
			"ypos"			"0"
			"zpos"			"0"
			"wide"   "100"
			"tall"			"80"
			"visible"		"1"
			"proportionaltoparent"	"1"
			"mouseinputenabled"	"0"

			"level"	"1"
		}

		"Rank2"
		{
			"ControlName"	"CTFStaticBadgePanel"
			"fieldName"		"Rank2"
			"xpos"			"110"
			"ypos"			"0"
			"zpos"			"0"
			"wide"   "100"
			"tall"			"80"
			"visible"		"1"
			"proportionaltoparent"	"1"
			"mouseinputenabled"	"0"

			"level"	"2"
		}

		"Rank3"
		{
			"ControlName"	"CTFStaticBadgePanel"
			"fieldName"		"Rank3"
			"xpos"			"220"
			"ypos"			"0"
			"zpos"			"0"
			"wide"   "100"
			"tall"			"80"
			"visible"		"1"
			"proportionaltoparent"	"1"
			"mouseinputenabled"	"0"

			"level"	"3"
		}

		// ----------------- Row 2 -------------------
		"Rank4"
		{
			"ControlName"	"CTFStaticBadgePanel"
			"fieldName"		"Rank4"
			"xpos"			"0"
			"ypos"			"70"
			"zpos"			"0"
			"wide"   "100"
			"tall"			"80"
			"visible"		"1"
			"proportionaltoparent"	"1"
			"mouseinputenabled"	"0"

			"level"	"4"
		}

		"Rank5"
		{
			"ControlName"	"CTFStaticBadgePanel"
			"fieldName"		"Rank5"
			"xpos"			"110"
			"ypos"			"70"
			"zpos"			"0"
			"wide"   "100"
			"tall"			"80"
			"visible"		"1"
			"proportionaltoparent"	"1"
			"mouseinputenabled"	"0"

			"level"	"5"
		}

		"Rank6"
		{
			"ControlName"	"CTFStaticBadgePanel"
			"fieldName"		"Rank6"
			"xpos"			"220"
			"ypos"			"70"
			"zpos"			"0"
			"wide"   "100"
			"tall"			"80"
			"visible"		"1"
			"proportionaltoparent"	"1"
			"mouseinputenabled"	"0"

			"level"	"6"
		}

		// ----------------- Row 3 -------------------
		"Rank7"
		{
			"ControlName"	"CTFStaticBadgePanel"
			"fieldName"		"Rank7"
			"xpos"			"0"
			"ypos"			"140"
			"zpos"			"0"
			"wide"   "100"
			"tall"			"80"
			"visible"		"1"
			"proportionaltoparent"	"1"
			"mouseinputenabled"	"0"

			"level"	"7"
		}

		"Rank8"
		{
			"ControlName"	"CTFStaticBadgePanel"
			"fieldName"		"Rank8"
			"xpos"			"110"
			"ypos"			"140"
			"zpos"			"0"
			"wide"   "100"
			"tall"			"80"
			"visible"		"1"
			"proportionaltoparent"	"1"
			"mouseinputenabled"	"0"

			"level"	"8"
		}

		"Rank9"
		{
			"ControlName"	"CTFStaticBadgePanel"
			"fieldName"		"Rank9"
			"xpos"			"220"
			"ypos"			"140"
			"zpos"			"0"
			"wide"   "100"
			"tall"			"80"
			"visible"		"1"
			"proportionaltoparent"	"1"
			"mouseinputenabled"	"0"

			"level"	"9"
		}

		// ----------------- Row 10 -------------------
		"Rank10"
		{
			"ControlName"	"CTFStaticBadgePanel"
			"fieldName"		"Rank10"
			"xpos"			"0"
			"ypos"			"210"
			"zpos"			"0"
			"wide"   "100"
			"tall"			"80"
			"visible"		"1"
			"proportionaltoparent"	"1"
			"mouseinputenabled"	"0"

			"level"	"10"
		}

		"Rank11"
		{
			"ControlName"	"CTFStaticBadgePanel"
			"fieldName"		"Rank11"
			"xpos"			"110"
			"ypos"			"210"
			"zpos"			"0"
			"wide"   "100"
			"tall"			"80"
			"visible"		"1"
			"proportionaltoparent"	"1"
			"mouseinputenabled"	"0"

			"level"	"11"
		}

		"Rank12"
		{
			"ControlName"	"CTFStaticBadgePanel"
			"fieldName"		"Rank12"
			"xpos"			"220"
			"ypos"			"210"
			"zpos"			"0"
			"wide"   "100"
			"tall"			"80"
			"visible"		"1"
			"proportionaltoparent"	"1"
			"mouseinputenabled"	"0"

			"level"	"12"
		}

		// ----------------- Row 10 -------------------
		"Rank13"
		{
			"ControlName"	"CTFStaticBadgePanel"
			"fieldName"		"Rank13"
			"xpos"			"110"
			"ypos"			"280"
			"zpos"			"0"
			"wide"   "100"
			"tall"			"80"
			"visible"		"1"
			"proportionaltoparent"	"1"
			"mouseinputenabled"	"0"

			"level"	"13"
		}
	}
}