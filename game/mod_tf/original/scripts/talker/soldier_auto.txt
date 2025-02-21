//--------------------------------------------------------------------------------------------------------------
// Soldier Response Rule File - AUTO GENERATED DO NOT EDIT BY HAND
//--------------------------------------------------------------------------------------------------------------

Criterion "SoldierNotIdleSpeech" "SoldierIdleSpeech" "!=1" "required" weight 0


Response item_birdhead_round_startSoldier
{
	scene "scenes/Player/Soldier/low/7464.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7464.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7465.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7466.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7469.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7470.vcd" predelay "1.0, 5.0"
}
Rule item_birdhead_round_startSoldier
{
	criteria ConceptPlayerRoundStart IsSoldier 100PercentChance IsSoldierBirdHead
	Response item_birdhead_round_startSoldier
}

Response item_birdhead_uberSoldier
{
	scene "scenes/Player/Soldier/low/7473.vcd" 
	scene "scenes/Player/Soldier/low/7474.vcd" 
}
Rule item_birdhead_uberSoldier
{
	criteria ConceptMedicChargeDeployed IsSoldier  IsInvulnerable MedicNotInvulnerableSpeech 20PercentChance IsSoldierBirdHead
	Response item_birdhead_uberSoldier
}

Response item_maggot_idleSoldier
{
	scene "scenes/Player/Soldier/low/7494.vcd" 
	scene "scenes/Player/Soldier/low/7492.vcd" 
	scene "scenes/Player/Soldier/low/7493.vcd" 
	scene "scenes/Player/Soldier/low/7498.vcd" 
	scene "scenes/Player/Soldier/low/7496.vcd" 
	scene "scenes/Player/Soldier/low/7495.vcd" 
	scene "scenes/Player/Soldier/low/7497.vcd" 
	scene "scenes/Player/Soldier/low/7500.vcd" 
	scene "scenes/Player/Soldier/low/7502.vcd" 
	scene "scenes/Player/Soldier/low/7501.vcd" 
}
Rule item_maggot_idleSoldier
{
	criteria ConceptPlayerExpression IsSoldier 30PercentChance IsSoldierMaggotHat SoldierNotIdleSpeech
	ApplyContext "SoldierIdleSpeech:1:10"
	Response item_maggot_idleSoldier
}


Response item_maggot_round_battlecrySoldier
{
	scene "scenes/Player/Soldier/low/7482.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7484.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7485.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7487.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7489.vcd" predelay "1.0, 5.0"
}


Rule item_maggot_round_battlecrySoldier
{
	criteria ConceptPlayerBattleCry IsHeavy IsSoldierMaggotHat
	Response item_maggot_round_battlecrySoldier
}



Response item_maggot_round_startSoldier
{
	scene "scenes/Player/Soldier/low/7482.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7484.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7485.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7487.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7489.vcd" predelay "1.0, 5.0"
}
Rule item_maggot_round_startSoldier
{
	criteria ConceptPlayerRoundStart IsSoldier 100PercentChance IsSoldierMaggotHat
	Response item_maggot_round_startSoldier
}

Response item_maggot_uberSoldier
{
	scene "scenes/Player/Soldier/low/7490.vcd" 
	scene "scenes/Player/Soldier/low/7491.vcd" 
}
Rule item_maggot_uberSoldier
{
	criteria ConceptMedicChargeDeployed IsSoldier  IsInvulnerable MedicNotInvulnerableSpeech 20PercentChance IsSoldierMaggotHat
	Response item_maggot_uberSoldier
}

Response item_wizard_dominationSoldier
{
	scene "scenes/Player/Soldier/low/7512.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/8570.vcd" predelay "2.5"
}
Rule item_wizard_dominationSoldier
{
	criteria ConceptKilledPlayer IsSoldier IsDominated 30PercentChance IsSoldierWizardHat
	ApplyContext "SoldierKillSpeech:1:10"
	Response item_wizard_dominationSoldier
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech
//--------------------------------------------------------------------------------------------------------------
Response item_maggot_healedSoldier
{
	scene "scenes/Player/Soldier/low/7505.vcd" 
	scene "scenes/Player/Soldier/low/7504.vcd" 
}
Rule item_maggot_healedSoldier
{
	criteria ConceptMedicChargeStopped IsSoldier SuperHighHealthContext SoldierNotSaidHealThanks IsSoldierMaggotHat 10PercentChance
	ApplyContext "SoldierSaidHealThanks:1:20"
	Response item_maggot_healedSoldier
}

Response item_unicorn_round_startSoldier
{
	scene "scenes/Player/Soldier/low/7457.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7456.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7454.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7455.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7453.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7452.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/7458.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/8562.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/8563.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/8564.vcd" predelay "1.0, 5.0"
}
Rule item_unicorn_round_startSoldier
{
	criteria ConceptPlayerRoundStart IsSoldier 100PercentChance IsUnicornHead
	Response item_unicorn_round_startSoldier
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Combat
//--------------------------------------------------------------------------------------------------------------
Response item_maggot_dominationSoldier
{
	scene "scenes/Player/Soldier/low/7510.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/8565.vcd" predelay "2.5"
}
Rule item_maggot_dominationSoldier
{
	criteria ConceptKilledPlayer IsSoldier IsDominated 30PercentChance IsSoldierMaggotHat
	ApplyContext "SoldierKillSpeech:1:10"
	Response item_maggot_dominationSoldier
}

Response item_unicorn_dominationSoldier
{
	scene "scenes/Player/Soldier/low/7460.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/7459.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/7463.vcd" predelay "2.5"
}
Rule item_unicorn_dominationSoldier
{
	criteria ConceptKilledPlayer IsSoldier IsDominated 30PercentChance IsUnicornHead
	ApplyContext "SoldierKillSpeech:1:10"
	Response item_unicorn_dominationSoldier
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Pain
//--------------------------------------------------------------------------------------------------------------
Response item_birdhead_on_fireSoldier
{
	scene "scenes/Player/Soldier/low/7479.vcd" 
	scene "scenes/Player/Soldier/low/7481.vcd" 
}
Rule item_birdhead_on_fireSoldier
{
	criteria ConceptFire IsSoldier SoldierIsNotStillonFire  40PercentChance IsSoldierBirdHead
	ApplyContext "SoldierOnFire:1:7"
	Response item_birdhead_on_fireSoldier
}

Response item_maggot_on_fireSoldier
{
	scene "scenes/Player/Soldier/low/7508.vcd" 
	scene "scenes/Player/Soldier/low/7507.vcd" 
}
Rule item_maggot_on_fireSoldier
{
	criteria ConceptFire IsSoldier SoldierIsNotStillonFire  20PercentChance IsSoldierMaggotHat
	ApplyContext "SoldierOnFire:1:7"
	Response item_maggot_on_fireSoldier
}

