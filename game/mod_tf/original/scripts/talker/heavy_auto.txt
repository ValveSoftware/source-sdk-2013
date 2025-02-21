//--------------------------------------------------------------------------------------------------------------
// Heavy Response Rule File - AUTO GENERATED DO NOT EDIT BY HAND
//--------------------------------------------------------------------------------------------------------------



Response item_birdhead_round_startHeavy
{
	scene "scenes/Player/Heavy/low/6700.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/6699.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/6703.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/6702.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/6705.vcd" predelay "1.0, 5.0"
}
Rule item_birdhead_round_startHeavy
{
	criteria ConceptPlayerRoundStart IsHeavy 100PercentChance IsHeavyBirdHead
	Response item_birdhead_round_startHeavy
}

Response item_birdhead_uberHeavy
{
	scene "scenes/Player/Heavy/low/6707.vcd" 
	scene "scenes/Player/Heavy/low/6708.vcd" 
	scene "scenes/Player/Heavy/low/6709.vcd" 
}
Rule item_birdhead_uberHeavy
{
	criteria ConceptMedicChargeDeployed IsHeavy  IsInvulnerable MedicNotInvulnerableSpeech 20PercentChance IsHeavyBirdHead
	Response item_birdhead_uberHeavy
}

Response item_unicorn_uberHeavy
{
	scene "scenes/Player/Heavy/low/6695.vcd" 
	scene "scenes/Player/Heavy/low/6696.vcd" 
	scene "scenes/Player/Heavy/low/6698.vcd" 
}
Rule item_unicorn_uberHeavy
{
	criteria ConceptMedicChargeDeployed IsHeavy  IsInvulnerable MedicNotInvulnerableSpeech 20PercentChance IsUnicornHead
	Response item_unicorn_uberHeavy
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech
//--------------------------------------------------------------------------------------------------------------
Response item_unicorn_round_startHeavy
{
	scene "scenes/Player/Heavy/low/6690.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/6691.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/6688.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/8497.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/6689.vcd" predelay "1.0, 5.0"
}
Rule item_unicorn_round_startHeavy
{
	criteria ConceptPlayerRoundStart IsHeavy 100PercentChance IsUnicornHead
	Response item_unicorn_round_startHeavy
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Combat
//--------------------------------------------------------------------------------------------------------------
Response item_unicorn_dominationHeavy
{
	scene "scenes/Player/Heavy/low/6693.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/6694.vcd" predelay "2.5"
}
Rule item_unicorn_dominationHeavy
{
	criteria ConceptKilledPlayer IsHeavy IsDominated 30PercentChance IsUnicornHead
	ApplyContext "HeavyKillSpeech:1:10"
	Response item_unicorn_dominationHeavy
}

