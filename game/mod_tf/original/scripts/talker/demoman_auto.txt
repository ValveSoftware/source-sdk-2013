//--------------------------------------------------------------------------------------------------------------
// Demoman Response Rule File - AUTO GENERATED DO NOT EDIT BY HAND
//--------------------------------------------------------------------------------------------------------------



Response item_unicorn_uberDemoman
{
	scene "scenes/Player/Demoman/low/7780.vcd" 
	scene "scenes/Player/Demoman/low/7781.vcd" 
	scene "scenes/Player/Demoman/low/7784.vcd" 
}
Rule item_unicorn_uberDemoman
{
	criteria ConceptMedicChargeDeployed IsDemoman  IsInvulnerable MedicNotInvulnerableSpeech 20PercentChance IsUnicornHead
	Response item_unicorn_uberDemoman
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech
//--------------------------------------------------------------------------------------------------------------
Response item_unicorn_round_startDemoman
{
	scene "scenes/Player/Demoman/low/7773.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/7776.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/7777.vcd" predelay "1.0, 5.0"
}
Rule item_unicorn_round_startDemoman
{
	criteria ConceptPlayerRoundStart IsDemoman 100PercentChance IsUnicornHead
	Response item_unicorn_round_startDemoman
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Combat
//--------------------------------------------------------------------------------------------------------------
Response item_unicorn_dominationDemoman
{
	scene "scenes/Player/Demoman/low/7785.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/7786.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/7787.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/7788.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/7790.vcd" predelay "2.5"
}
Rule item_unicorn_dominationDemoman
{
	criteria ConceptKilledPlayer IsDemoman IsDominated 30PercentChance IsUnicornHead
	ApplyContext "DemomanKillSpeech:1:10"
	Response item_unicorn_dominationDemoman
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Pain
//--------------------------------------------------------------------------------------------------------------
Response item_unicorn_on_fireDemoman
{
	scene "scenes/Player/Demoman/low/7778.vcd" 
	scene "scenes/Player/Demoman/low/7779.vcd" 
}
Rule item_unicorn_on_fireDemoman
{
	criteria ConceptFire IsDemoman DemomanIsNotStillonFire  20PercentChance IsUnicornHead
	ApplyContext "DemomanOnFire:1:7"
	Response item_unicorn_on_fireDemoman
}

