//--------------------------------------------------------------------------------------------------------------
// Engineer Response Rule File - AUTO GENERATED DO NOT EDIT BY HAND
//--------------------------------------------------------------------------------------------------------------



Response item_unicorn_uberEngineer
{
	scene "scenes/Player/Engineer/low/7963.vcd" 
}
Rule item_unicorn_uberEngineer
{
	criteria ConceptMedicChargeDeployed IsEngineer  IsInvulnerable MedicNotInvulnerableSpeech 20PercentChance IsUnicornHead
	Response item_unicorn_uberEngineer
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech
//--------------------------------------------------------------------------------------------------------------
Response item_unicorn_healedEngineer
{
	scene "scenes/Player/Engineer/low/7964.vcd" 
}
Rule item_unicorn_healedEngineer
{
	criteria ConceptMedicChargeStopped IsEngineer SuperHighHealthContext EngineerNotSaidHealThanks IsUnicornHead 20PercentChance
	ApplyContext "EngineerSaidHealThanks:1:20"
	Response item_unicorn_healedEngineer
}

Response item_unicorn_round_startEngineer
{
	scene "scenes/Player/Engineer/low/7951.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/7957.vcd" predelay "1.0, 5.0"
}
Rule item_unicorn_round_startEngineer
{
	criteria ConceptPlayerRoundStart IsEngineer 100PercentChance IsUnicornHead
	Response item_unicorn_round_startEngineer
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Combat
//--------------------------------------------------------------------------------------------------------------
Response item_unicorn_dominationEngineer
{
	scene "scenes/Player/Engineer/low/7960.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/8473.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/7961.vcd" predelay "2.5"
}
Rule item_unicorn_dominationEngineer
{
	criteria ConceptKilledPlayer IsEngineer IsDominated 30PercentChance IsUnicornHead
	ApplyContext "EngineerKillSpeech:1:10"
	Response item_unicorn_dominationEngineer
}

Response item_unicorn_revengeEngineer
{
	scene "scenes/Player/Engineer/low/7967.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/7968.vcd" predelay "2.5"
}
Rule item_unicorn_revengeEngineer
{
	criteria ConceptKilledPlayer IsEngineer IsRevenge IsUnicornHead 40PercentChance
	ApplyContext "EngineerKillSpeech:1:10"
	Response item_unicorn_revengeEngineer
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Engineer
//--------------------------------------------------------------------------------------------------------------
Response item_unicorn_dispenserEngineer
{
	scene "scenes/Player/Engineer/low/7958.vcd" 
}
Rule item_unicorn_dispenserEngineer
{
	criteria ConceptPlayerBuildingObject IsDispenser IsEngineer  IsUnicornHead 10PercentChance
	Response item_unicorn_dispenserEngineer
}

