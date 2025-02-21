//--------------------------------------------------------------------------------------------------------------
// Sniper Response Rule File - AUTO GENERATED DO NOT EDIT BY HAND
//--------------------------------------------------------------------------------------------------------------



Response item_birdhead_kill_scopedSniper
{
	scene "scenes/Player/Sniper/low/7307.vcd" 
	scene "scenes/Player/Sniper/low/7308.vcd" 
	scene "scenes/Player/Sniper/low/7310.vcd" 
}
Rule item_birdhead_kill_scopedSniper
{
	criteria ConceptKilledPlayer IsSniper DeployedContext 50PercentChance IsSniperBirdHead
	Response item_birdhead_kill_scopedSniper
}

Response item_birdhead_round_startSniper
{
	scene "scenes/Player/Sniper/low/7297.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/7298.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/7302.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/7301.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/7304.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/7300.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/7299.vcd" predelay "1.0, 5.0"
}
Rule item_birdhead_round_startSniper
{
	criteria ConceptPlayerRoundStart IsSniper 100PercentChance IsSniperBirdHead
	Response item_birdhead_round_startSniper
}

Response item_birdhead_uberSniper
{
	scene "scenes/Player/Sniper/low/7306.vcd" 
}
Rule item_birdhead_uberSniper
{
	criteria ConceptMedicChargeDeployed IsSniper  IsInvulnerable MedicNotInvulnerableSpeech 20PercentChance IsSniperBirdHead
	Response item_birdhead_uberSniper
}

Response item_unicorn_scopekillSniper
{
	scene "scenes/Player/Sniper/low/7288.vcd" 
	scene "scenes/Player/Sniper/low/7289.vcd" 
	scene "scenes/Player/Sniper/low/7291.vcd" 
	scene "scenes/Player/Sniper/low/7292.vcd" 
	scene "scenes/Player/Sniper/low/7293.vcd" 
	scene "scenes/Player/Sniper/low/7295.vcd" 
}
Rule item_unicorn_scopekillSniper
{
	criteria ConceptKilledPlayer IsSniper DeployedContext 50PercentChance IsUnicornHead
	Response item_unicorn_scopekillSniper
}

Response item_unicorn_uberSniper
{
	scene "scenes/Player/Sniper/low/7287.vcd" 
}
Rule item_unicorn_uberSniper
{
	criteria ConceptMedicChargeDeployed IsSniper  IsInvulnerable MedicNotInvulnerableSpeech 20PercentChance IsUnicornHead
	Response item_unicorn_uberSniper
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech
//--------------------------------------------------------------------------------------------------------------
Response item_unicorn_round_startSniper
{
	scene "scenes/Player/Sniper/low/7282.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/7283.vcd" predelay "1.0, 5.0"
}
Rule item_unicorn_round_startSniper
{
	criteria ConceptPlayerRoundStart IsSniper 100PercentChance IsUnicornHead
	Response item_unicorn_round_startSniper
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Cart
//--------------------------------------------------------------------------------------------------------------
Response item_birdhead_cart_pushSniper
{
	scene "scenes/Player/Sniper/low/7305.vcd" 
	scene "scenes/Player/Sniper/low/8485.vcd" 
}
Rule item_birdhead_cart_pushSniper
{
	criteria ConceptCartMovingForward IsOnOffense IsSniper SniperNotSaidCartMovingForwardO 10PercentChance IsSniperBirdHead
	ApplyContext "SaidCartMovingForwardD:1:20"
	Response item_birdhead_cart_pushSniper
}

Response item_unicorn_cart_forwardSniper
{
	scene "scenes/Player/Sniper/low/7286.vcd" 
	scene "scenes/Player/Sniper/low/8484.vcd" 
}
Rule item_unicorn_cart_forwardSniper
{
	criteria ConceptCartMovingForward IsOnOffense IsSniper SniperNotSaidCartMovingForwardO 30PercentChance IsUnicornHead
	ApplyContext "SaidCartMovingForwardD:1:20"
	Response item_unicorn_cart_forwardSniper
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Combat
//--------------------------------------------------------------------------------------------------------------
Response item_birdhead_jarateSniper
{
	scene "scenes/Player/Sniper/low/7315.vcd" 
}
Rule item_birdhead_jarateSniper
{
	criteria ConceptJarateLaunch IsSniper 10PercentChance IsSniperBirdHead
	Response item_birdhead_jarateSniper
}

