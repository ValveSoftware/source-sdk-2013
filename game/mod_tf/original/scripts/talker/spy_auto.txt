//--------------------------------------------------------------------------------------------------------------
// Spy Response Rule File - AUTO GENERATED DO NOT EDIT BY HAND
//--------------------------------------------------------------------------------------------------------------




//--------------------------------------------------------------------------------------------------------------
// Auto Speech
//--------------------------------------------------------------------------------------------------------------
Response item_unicorn_cappedSpy
{
	scene "scenes/Player/Spy/low/7688.vcd" predelay "2.0, 4.0"
	scene "scenes/Player/Spy/low/8477.vcd" predelay "2.0, 4.0"
	scene "scenes/Player/Spy/low/7690.vcd" predelay "2.0, 4.0"
}
Rule item_unicorn_cappedSpy
{
	criteria ConceptPlayerCapturedPoint IsSpy IsUnicornHead
	Response item_unicorn_cappedSpy
}

Response item_unicorn_case_capSpy
{
	scene "scenes/Player/Spy/low/7545.vcd" predelay "2.0, 4.0"
}
Rule item_unicorn_case_capSpy
{
	criteria ConceptPlayerCapturedIntelligence IsSpy IsUnicornHead
	Response item_unicorn_case_capSpy
}

Response item_unicorn_round_startSpy
{
	scene "scenes/Player/Spy/low/7541.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Spy/low/7542.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Spy/low/7543.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Spy/low/7544.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Spy/low/8551.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Spy/low/8552.vcd" predelay "1.0, 5.0"
}
Rule item_unicorn_round_startSpy
{
	criteria ConceptPlayerRoundStart IsSpy 100PercentChance IsUnicornHead
	Response item_unicorn_round_startSpy
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Combat
//--------------------------------------------------------------------------------------------------------------
Response item_unicorn_backstabSpy
{
	scene "scenes/Player/Spy/low/7540.vcd" 
	scene "scenes/Player/Spy/low/7548.vcd" 
}
Rule item_unicorn_backstabSpy
{
	criteria ConceptKilledPlayer KilledPlayerDelay IsWeaponMelee SpyNotKillSpeechMelee IsSpy 20PercentChance IsUnicornHead
	ApplyContext "SpyKillSpeechMelee:1:10"
	applycontexttoworld
	Response item_unicorn_backstabSpy
}

Response item_unicorn_dominationSpy
{
	scene "scenes/Player/Spy/low/7546.vcd" predelay "2.5"
}
Rule item_unicorn_dominationSpy
{
	criteria ConceptKilledPlayer IsSpy IsDominated 30PercentChance IsUnicornHead
	ApplyContext "SpyKillSpeech:1:10"
	Response item_unicorn_dominationSpy
}

