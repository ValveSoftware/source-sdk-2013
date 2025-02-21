//--------------------------------------------------------------------------------------------------------------
// Medic Response Rule File - AUTO GENERATED DO NOT EDIT BY HAND
//--------------------------------------------------------------------------------------------------------------

Criterion "MedicNotIdleSpeech" "MedicIdleSpeech" "!=1" "required" weight 0

Response item_birdhead_round_startMedic
{
	scene "scenes/Player/Medic/low/6907.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/6908.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/6909.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/6910.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/6912.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/6913.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/6914.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/6916.vcd" predelay "1.0, 5.0"
}
Rule item_birdhead_round_startMedic
{
	criteria ConceptPlayerRoundStart IsMedic 100PercentChance IsMedicBirdHead
	Response item_birdhead_round_startMedic
}

Response item_birdhead_uberMedic
{
	scene "scenes/Player/Medic/low/6918.vcd" 
}
Rule item_birdhead_uberMedic
{
	criteria ConceptMedicChargeDeployed IsMedic  IsInvulnerable MedicNotInvulnerableSpeech 20PercentChance IsMedicBirdHead
	Response item_birdhead_uberMedic
}

Response item_secop_idleMedic
{
	scene "scenes/Player/Medic/low/6850.vcd" 
	scene "scenes/Player/Medic/low/6849.vcd" 
	scene "scenes/Player/Medic/low/6851.vcd" 
}
Rule item_secop_idleMedic
{
	criteria ConceptPlayerExpression IsMedic 30PercentChance IsMedicDoubleFace MedicNotIdleSpeech
	ApplyContext "MedicIdleSpeech:1:10"
	Response item_secop_idleMedic
}

Response item_secop_kill_assistMedic
{
	scene "scenes/Player/Medic/low/6852.vcd" 
}
Rule item_secop_kill_assistMedic
{
	criteria ConceptKilledPlayer IsMedic IsBeingHealed IsARecentKill KilledPlayerDelay MedicNotAssistSpeech 20PercentChance IsMedicDoubleFace
	Response item_secop_kill_assistMedic
}

Response item_secop_uberMedic
{
	scene "scenes/Player/Medic/low/6853.vcd" 
}
Rule item_secop_uberMedic
{
	criteria ConceptMedicChargeDeployed IsMedic  IsInvulnerable MedicNotInvulnerableSpeech 30PercentChance IsMedicDoubleFace
	Response item_secop_uberMedic
}

Response item_zombiebird_mvm_go_upgradeMedic
{
	scene "scenes/Player/Medic/low/6830.vcd" 
}
Rule item_zombiebird_mvm_go_upgradeMedic
{
	criteria ConceptMvMEncourageUpgrade 20PercentChance IsMvMDefender IsMedic IsMedicZombieBird
	Response item_zombiebird_mvm_go_upgradeMedic
}

Response item_zombiebird_mvm_winMedic
{
	scene "scenes/Player/Medic/low/6832.vcd" 
}
Rule item_zombiebird_mvm_winMedic
{
	criteria ConceptMvMWaveWin 30PercentChance IsMvMDefender IsMedic IsMedicZombieBird
	Response item_zombiebird_mvm_winMedic
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech
//--------------------------------------------------------------------------------------------------------------
Response item_secop_round_startMedic
{
	scene "scenes/Player/Medic/low/6841.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/6843.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/6842.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/6844.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/6845.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/6846.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/6847.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/6848.vcd" predelay "1.0, 5.0"
}
Rule item_secop_round_startMedic
{
	criteria ConceptPlayerRoundStart IsMedic 100PercentChance IsMedicDoubleFace
	Response item_secop_round_startMedic
}

Response item_zombiebird_got_briefcaseMedic
{
	scene "scenes/Player/Medic/low/6828.vcd" 
}
Rule item_zombiebird_got_briefcaseMedic
{
	criteria ConceptPlayerGrabbedIntelligence IsScout NotScoutGrabbedIntelligence 10PercentChance IsMedicZombieBird
	ApplyContext "ScoutGrabbedIntelligence:1:30"
	Response item_zombiebird_got_briefcaseMedic
}

Response item_zombiebird_round_startMedic
{
	scene "scenes/Player/Medic/low/6820.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/8506.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/6822.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/6823.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/6824.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Medic/low/8507.vcd" predelay "1.0, 5.0"
}
Rule item_zombiebird_round_startMedic
{
	criteria ConceptPlayerRoundStart IsMedic 100PercentChance IsMedicZombieBird
	Response item_zombiebird_round_startMedic
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Cart
//--------------------------------------------------------------------------------------------------------------
Response item_secop_cart_pushMedic
{
	scene "scenes/Player/Medic/low/6854.vcd" 
	scene "scenes/Player/Medic/low/6855.vcd" 
	scene "scenes/Player/Medic/low/6856.vcd" 
	scene "scenes/Player/Medic/low/6857.vcd" 
}
Rule item_secop_cart_pushMedic
{
	criteria ConceptCartMovingForward IsOnOffense IsMedic MedicNotSaidCartMovingForwardO 30PercentChance IsMedicDoubleFace
	ApplyContext "SaidCartMovingForwardD:1:20"
	Response item_secop_cart_pushMedic
}

Response item_secop_cart_stoppedMedic
{
	scene "scenes/Player/Medic/low/6858.vcd" 
}
Rule item_secop_cart_stoppedMedic
{
	criteria ConceptCartMovingStopped IsOnOffense IsMedic MedicNotSaidCartMovingStoppedO 30PercentChance IsMedicDoubleFace
	ApplyContext "SaidCartMovingForwardD:1:20"
	Response item_secop_cart_stoppedMedic
}

Response item_zombiebird_sf13_cart_forwardMedic
{
	scene "scenes/Player/Medic/low/6825.vcd" 
	scene "scenes/Player/Medic/low/6827.vcd" 
}
Rule item_zombiebird_sf13_cart_forwardMedic
{
	criteria ConceptCartMovingForward IsOnOffense IsMedic MedicNotSaidCartMovingForwardO 30PercentChance IsMedicZombieBird
	ApplyContext "SaidCartMovingForwardD:1:20"
	Response item_zombiebird_sf13_cart_forwardMedic
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Combat
//--------------------------------------------------------------------------------------------------------------
Response item_secop_dominationMedic
{
	scene "scenes/Player/Medic/low/6859.vcd" predelay "2.5"
}
Rule item_secop_dominationMedic
{
	criteria ConceptKilledPlayer IsMedic IsDominated 30PercentChance IsMedicDoubleFace
	ApplyContext "MedicKillSpeech:1:10"
	Response item_secop_dominationMedic
}

Response item_zombiebird_dominatedMedic
{
	scene "scenes/Player/Medic/low/6833.vcd" predelay "2.5"
}
Rule item_zombiebird_dominatedMedic
{
	criteria ConceptKilledPlayer IsMedic IsDominated 30PercentChance IsMedicZombieBird
	ApplyContext "MedicKillSpeech:1:10"
	Response item_zombiebird_dominatedMedic
}

Response item_zombiebird_jarateMedic
{
	scene "scenes/Player/Medic/low/6835.vcd" 
}
Rule item_zombiebird_jarateMedic
{
	criteria ConceptJarateHit IsMedic 50PercentChance IsMedicZombieBird
	Response item_zombiebird_jarateMedic
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Pain
//--------------------------------------------------------------------------------------------------------------
Response item_zombiebird_on_fireMedic
{
	scene "scenes/Player/Medic/low/6834.vcd" 
}
Rule item_zombiebird_on_fireMedic
{
	criteria ConceptFire IsMedic MedicIsNotStillonFire  20PercentChance IsMedicZombieBird
	ApplyContext "MedicOnFire:1:7"
	Response item_zombiebird_on_fireMedic
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 1
//--------------------------------------------------------------------------------------------------------------
Response item_zombiebird_stand_pointMedic
{
	scene "scenes/Player/Medic/low/6829.vcd" 
}
Rule item_zombiebird_stand_pointMedic
{
	criteria ConceptPlayerHelp IsMedic IsOnCappableControlPoint IsHelpCapMedic IsMedicZombieBird
	Response item_zombiebird_stand_pointMedic
}

