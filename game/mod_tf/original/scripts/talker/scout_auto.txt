//--------------------------------------------------------------------------------------------------------------
// Scout Response Rule File - AUTO GENERATED DO NOT EDIT BY HAND
//--------------------------------------------------------------------------------------------------------------



Response item_unicorn_uberScout
{
	scene "scenes/Player/Scout/low/7030.vcd" 
	scene "scenes/Player/Scout/low/8517.vcd" 
}
Rule item_unicorn_uberScout
{
	criteria ConceptMedicChargeDeployed IsScout  IsInvulnerable MedicNotInvulnerableSpeech 20PercentChance IsUnicornHead
	Response item_unicorn_uberScout
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech
//--------------------------------------------------------------------------------------------------------------
Response item_haunthat_cappedScout
{
	scene "scenes/Player/Scout/low/7066.vcd" predelay "2.0, 4.0"
	scene "scenes/Player/Scout/low/7067.vcd" predelay "2.0, 4.0"
	scene "scenes/Player/Scout/low/7068.vcd" predelay "2.0, 4.0"
}
Rule item_haunthat_cappedScout
{
	criteria ConceptPlayerCapturedPoint IsScout IsHauntedHat
	Response item_haunthat_cappedScout
}

Response item_haunthat_case_capScout
{
	scene "scenes/Player/Scout/low/7061.vcd" predelay "2.0, 4.0"
	scene "scenes/Player/Scout/low/7063.vcd" predelay "2.0, 4.0"
	scene "scenes/Player/Scout/low/7064.vcd" predelay "2.0, 4.0"
	scene "scenes/Player/Scout/low/7065.vcd" predelay "2.0, 4.0"
}
Rule item_haunthat_case_capScout
{
	criteria ConceptPlayerCapturedIntelligence IsScout IsHauntedHat
	Response item_haunthat_case_capScout
}

Response item_haunthat_got_caseScout
{
	scene "scenes/Player/Scout/low/7054.vcd" 
	scene "scenes/Player/Scout/low/7055.vcd" 
	scene "scenes/Player/Scout/low/7057.vcd" 
}
Rule item_haunthat_got_caseScout
{
	criteria ConceptPlayerGrabbedIntelligence IsScout NotScoutGrabbedIntelligence 10PercentChance IsHauntedHat
	ApplyContext "ScoutGrabbedIntelligence:1:30"
	Response item_haunthat_got_caseScout
}

Response item_haunthat_round_startScout
{
	scene "scenes/Player/Scout/low/7039.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/7041.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/7042.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/7043.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/7044.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/7045.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/7046.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/7047.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/7048.vcd" predelay "1.0, 5.0"
}
Rule item_haunthat_round_startScout
{
	criteria ConceptPlayerRoundStart IsScout 100PercentChance IsHauntedHat
	Response item_haunthat_round_startScout
}

Response item_unicorn_round_startScout
{
	scene "scenes/Player/Scout/low/7018.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/7019.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/7020.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/7021.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/7025.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/7022.vcd" predelay "1.0, 5.0"
}
Rule item_unicorn_round_startScout
{
	criteria ConceptPlayerRoundStart IsScout 100PercentChance IsUnicornHead
	Response item_unicorn_round_startScout
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Cart
//--------------------------------------------------------------------------------------------------------------
Response item_haunthat_cart_pushScout
{
	scene "scenes/Player/Scout/low/7069.vcd" 
	scene "scenes/Player/Scout/low/7070.vcd" 
}
Rule item_haunthat_cart_pushScout
{
	criteria ConceptCartMovingForward IsOnOffense IsScout ScoutNotSaidCartMovingForwardO 30PercentChance IsHauntedHat
	ApplyContext "SaidCartMovingForwardD:1:20"
	Response item_haunthat_cart_pushScout
}

Response item_unicorn_cart_forwardScout
{
	scene "scenes/Player/Scout/low/7026.vcd" 
}
Rule item_unicorn_cart_forwardScout
{
	criteria ConceptCartMovingForward IsOnOffense IsScout ScoutNotSaidCartMovingForwardO 30PercentChance IsUnicornHead
	ApplyContext "SaidCartMovingForwardD:1:20"
	Response item_unicorn_cart_forwardScout
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Combat
//--------------------------------------------------------------------------------------------------------------
Response item_haunthat_dominationScout
{
	scene "scenes/Player/Scout/low/7059.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/7060.vcd" predelay "2.5"
}
Rule item_haunthat_dominationScout
{
	criteria ConceptKilledPlayer IsScout IsDominated 30PercentChance IsHauntedHat
	ApplyContext "ScoutKillSpeech:1:10"
	Response item_haunthat_dominationScout
}

Response item_unicorn_dominationScout
{
	scene "scenes/Player/Scout/low/7031.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/7032.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/7033.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/7034.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/7035.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/7037.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/7036.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/7038.vcd" predelay "2.5"
}
Rule item_unicorn_dominationScout
{
	criteria ConceptKilledPlayer IsScout IsDominated 30PercentChance IsUnicornHead
	ApplyContext "ScoutKillSpeech:1:10"
	Response item_unicorn_dominationScout
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Pain
//--------------------------------------------------------------------------------------------------------------
Response item_haunthat_on_fireScout
{
	scene "scenes/Player/Scout/low/7076.vcd" 
	scene "scenes/Player/Scout/low/7077.vcd" 
	scene "scenes/Player/Scout/low/7078.vcd" 
}
Rule item_haunthat_on_fireScout
{
	criteria ConceptFire IsScout ScoutIsNotStillonFire  20PercentChance IsHauntedHat
	ApplyContext "ScoutOnFire:1:7"
	Response item_haunthat_on_fireScout
}

Response item_unicorn_on_fireScout
{
	scene "scenes/Player/Scout/low/7028.vcd" 
	scene "scenes/Player/Scout/low/7029.vcd" 
}
Rule item_unicorn_on_fireScout
{
	criteria ConceptFire IsScout ScoutIsNotStillonFire  20PercentChance IsUnicornHead
	ApplyContext "ScoutOnFire:1:7"
	Response item_unicorn_on_fireScout
}

