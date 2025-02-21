//--------------------------------------------------------------------------------------------------------------
// Soldier Response Rule File
//--------------------------------------------------------------------------------------------------------------

Criterion "SoldierIsKillSpeechObject" "SoldierKillSpeechObject" "1" "required" weight 0
Criterion "SoldierIsNotStillonFire" "SoldierOnFire" "!=1" "required" weight 0
Criterion "SoldierIsStillonFire" "SoldierOnFire" "1" "required" weight 0
Criterion "SoldierNotKillSpeech" "SoldierKillSpeech" "!=1" "required" weight 0
Criterion "SoldierNotKillSpeechMelee" "SoldierKillSpeechMelee" "!=1" "required" weight 0
Criterion "SoldierNotSaidHealThanks" "SoldierSaidHealThanks" "!=1" "required"
Criterion "SoldierNotRobotNoises" "SoldierRobotNoises" "!=1" "required" weight 0
Criterion "IsHelpCapSoldier" "SoldierHelpCap" "1" "required" weight 0
// Custom criterion
Criterion "SoldierNotAssistSpeech" "SoldierAssistSpeech" "!=1" "required" weight 0
Criterion "SoldierNotInvulnerableSpeech" "SoldierInvulnerableSpeech" "!=1" "required" weight 0
Criterion "SoldierNotAwardSpeech" "SoldierAwardSpeech" "!=1" "required" weight 0

Response PlayerCloakedSpyDemomanSoldier
{
	scene "scenes/Player/Soldier/low/1082.vcd" 
}
Rule PlayerCloakedSpyDemomanSoldier
{
	criteria ConceptPlayerCloakedSpy IsSoldier IsOnDemoman
	Response PlayerCloakedSpyDemomanSoldier
}

Response PlayerCloakedSpyEngineerSoldier
{
	scene "scenes/Player/Soldier/low/1087.vcd" 
}
Rule PlayerCloakedSpyEngineerSoldier
{
	criteria ConceptPlayerCloakedSpy IsSoldier IsOnEngineer
	Response PlayerCloakedSpyEngineerSoldier
}

Response PlayerCloakedSpyHeavySoldier
{
	scene "scenes/Player/Soldier/low/1077.vcd" 
}
Rule PlayerCloakedSpyHeavySoldier
{
	criteria ConceptPlayerCloakedSpy IsSoldier IsOnHeavy
	Response PlayerCloakedSpyHeavySoldier
}

Response PlayerCloakedSpyMedicSoldier
{
	scene "scenes/Player/Soldier/low/1085.vcd" 
}
Rule PlayerCloakedSpyMedicSoldier
{
	criteria ConceptPlayerCloakedSpy IsSoldier IsOnMedic
	Response PlayerCloakedSpyMedicSoldier
}

Response PlayerCloakedSpyPyroSoldier
{
	scene "scenes/Player/Soldier/low/1080.vcd" 
}
Rule PlayerCloakedSpyPyroSoldier
{
	criteria ConceptPlayerCloakedSpy IsSoldier IsOnPyro
	Response PlayerCloakedSpyPyroSoldier
}

Response PlayerCloakedSpyScoutSoldier
{
	scene "scenes/Player/Soldier/low/1074.vcd" 
}
Rule PlayerCloakedSpyScoutSoldier
{
	criteria ConceptPlayerCloakedSpy IsSoldier IsOnScout
	Response PlayerCloakedSpyScoutSoldier
}

Response PlayerCloakedSpySniperSoldier
{
	scene "scenes/Player/Soldier/low/1090.vcd" 
}
Rule PlayerCloakedSpySniperSoldier
{
	criteria ConceptPlayerCloakedSpy IsSoldier IsOnSniper
	Response PlayerCloakedSpySniperSoldier
}

Response PlayerCloakedSpySoldierSoldier
{
	scene "scenes/Player/Soldier/low/1075.vcd" 
}
Rule PlayerCloakedSpySoldierSoldier
{
	criteria ConceptPlayerCloakedSpy IsSoldier IsOnSoldier
	Response PlayerCloakedSpySoldierSoldier
}

Response PlayerCloakedSpySpySoldier
{
	scene "scenes/Player/Soldier/low/1083.vcd" 
}
Rule PlayerCloakedSpySpySoldier
{
	criteria ConceptPlayerCloakedSpy IsSoldier IsOnSpy
	Response PlayerCloakedSpySpySoldier
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech
//--------------------------------------------------------------------------------------------------------------
Response HealThanksSoldier
{
	scene "scenes/Player/Soldier/low/1213.vcd" 
	scene "scenes/Player/Soldier/low/1214.vcd" 
	scene "scenes/Player/Soldier/low/1215.vcd" 
}
Rule HealThanksSoldier
{
	criteria ConceptMedicChargeStopped IsSoldier SuperHighHealthContext SoldierNotSaidHealThanks 50PercentChance
	ApplyContext "SoldierSaidHealThanks:1:20"
	Response HealThanksSoldier
}

// Custom achievement stuff
Response AwardSoldier
{
	scene "scenes/Player/Soldier/low/1043.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/1048.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/1135.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/1347.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/1182.vcd" predelay "2.5"
}
Rule AwardSoldier
{
	criteria ConceptAchievementAward IsSoldier SoldierNotAwardSpeech
	ApplyContext "SoldierAwardSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response AwardSoldier
}
//End custom achievement

Response PlayerRoundStartSoldier
{
	scene "scenes/Player/Soldier/low/1055.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/1057.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/1058.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/1059.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/1056.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/1060.vcd" predelay "1.0, 5.0"
}
Rule PlayerRoundStartSoldier
{
	criteria ConceptPlayerRoundStart IsSoldier
	Response PlayerRoundStartSoldier
}

Response PlayerCappedIntelligenceSoldier
{
	scene "scenes/Player/Soldier/low/1046.vcd" 
	scene "scenes/Player/Soldier/low/1048.vcd" 
	scene "scenes/Player/Soldier/low/1047.vcd" 
}
Rule PlayerCappedIntelligenceSoldier
{
	criteria ConceptPlayerCapturedIntelligence IsSoldier
	Response PlayerCappedIntelligenceSoldier
}

Response PlayerCapturedPointSoldier
{
	scene "scenes/Player/Soldier/low/1043.vcd" 
	scene "scenes/Player/Soldier/low/1044.vcd" 
	scene "scenes/Player/Soldier/low/1045.vcd" 
}
Rule PlayerCapturedPointSoldier
{
	criteria ConceptPlayerCapturedPoint IsSoldier
	Response PlayerCapturedPointSoldier
}

Response PlayerSuddenDeathSoldier
{
	scene "scenes/Player/Soldier/low/1120.vcd" 
	scene "scenes/Player/Soldier/low/1121.vcd" 
	scene "scenes/Player/Soldier/low/1122.vcd" 
	scene "scenes/Player/Soldier/low/1123.vcd" 
	scene "scenes/Player/Soldier/low/1124.vcd" 
	scene "scenes/Player/Soldier/low/1125.vcd" 
	scene "scenes/Player/Soldier/low/1126.vcd" 
	scene "scenes/Player/Soldier/low/1127.vcd" 
	scene "scenes/Player/Soldier/low/1128.vcd" 
	scene "scenes/Player/Soldier/low/1129.vcd" 
	scene "scenes/Player/Soldier/low/1130.vcd" 
	scene "scenes/Player/Soldier/low/1131.vcd" 
}
Rule PlayerSuddenDeathSoldier
{
	criteria ConceptPlayerSuddenDeathStart IsSoldier
	Response PlayerSuddenDeathSoldier
}

Response PlayerStalemateSoldier
{
	scene "scenes/Player/Soldier/low/1049.vcd" 
	scene "scenes/Player/Soldier/low/1050.vcd" 
	scene "scenes/Player/Soldier/low/1051.vcd" 
}
Rule PlayerStalemateSoldier
{
	criteria ConceptPlayerStalemate IsSoldier
	Response PlayerStalemateSoldier
}

Response PlayerTeleporterThanksSoldier
{
	scene "scenes/Player/Soldier/low/1216.vcd" 
	scene "scenes/Player/Soldier/low/1217.vcd" 
	scene "scenes/Player/Soldier/low/1218.vcd" 
}
Rule PlayerTeleporterThanksSoldier
{
	criteria ConceptTeleported IsNotEngineer IsSoldier 30PercentChance
	Response PlayerTeleporterThanksSoldier
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Combat
//--------------------------------------------------------------------------------------------------------------
Response DefendOnThePointSoldier
{
	scene "scenes/Player/Soldier/low/1187.vcd" 
	scene "scenes/Player/Soldier/low/1354.vcd" 
	scene "scenes/Player/Soldier/low/1355.vcd" 
}
Rule DefendOnThePointSoldier
{
	criteria ConceptFireWeapon IsSoldier IsOnFriendlyControlPoint NotDefendOnThePointSpeech
	ApplyContext "DefendOnThePointSpeech:1:30"
	applycontexttoworld
	Response DefendOnThePointSoldier
}

Response KilledPlayerManySoldier
{
	scene "scenes/Player/Soldier/low/1181.vcd" 
	scene "scenes/Player/Soldier/low/1356.vcd" 
	scene "scenes/Player/Soldier/low/1197.vcd" 
	scene "scenes/Player/Soldier/low/1199.vcd" 
}
Rule KilledPlayerManySoldier
{
	criteria ConceptKilledPlayer IsManyRecentKills 30PercentChance IsWeaponPrimary KilledPlayerDelay SoldierNotKillSpeech IsSoldier
	ApplyContext "SoldierKillSpeech:1:10"
	Response KilledPlayerManySoldier
}

Response KilledDemomanSoldier
{
	scene "scenes/Player/Soldier/low/3486.vcd" 
	scene "scenes/Player/Soldier/low/3487.vcd" 
	scene "scenes/Player/Soldier/low/3488.vcd" 
	scene "scenes/Player/Soldier/low/3489.vcd" 
	scene "scenes/Player/Soldier/low/3490.vcd" 
	scene "scenes/Player/Soldier/low/3491.vcd" 
}
Rule KilledDemomanSoldier
{
	criterion ConceptKilledPlayer KilledPlayerDelay IsVictimDemoman 10PercentChance SoldierNotKillSpeech IsSoldier
	ApplyContext "SoldierKillSpeech:1:10"
	Response KilledDemomanSoldier
}

Response KilledPlayerAssistAutoSoldier
{
	scene "scenes/Player/Soldier/low/1186.vcd" predelay "2.5"
}
Rule KilledPlayerAssistAutoSoldier
{
	criteria ConceptKilledPlayer IsSoldier IsBeingHealed IsManyRecentKills KilledPlayerDelay 20PercentChance SoldierNotAssistSpeech
	ApplyContext "SoldierAssistSpeech:1:20"
	Response KilledPlayerAssistAutoSoldier
}

// A custom rule for when you're on a pocket Soldier killing spree.
Response SpreeMedicSoldier
{
	scene "scenes/Player/Soldier/low/3492.vcd"
	scene "scenes/Player/Soldier/low/3493.vcd"
	scene "scenes/Player/Soldier/low/3494.vcd"
}
Rule SpreeMedicSoldier
{
	criteria ConceptKilledPlayer KilledPlayerDelay IsSoldier IsBeingHealed SoldierNotKillSpeech IsVeryManyRecentKills IsWeaponPrimary
	ApplyContext "SoldierKillSpeech:1:20"
	Response SpreeMedicSoldier
}

// Custom Medic follow - because Soldier needs it more than most classes.
Response MedicFollowSoldier
{
	scene "scenes/Player/Soldier/low/3495.vcd" predelay ".25"
	scene "scenes/Player/Soldier/low/3496.vcd" predelay ".25"
	scene "scenes/Player/Soldier/low/3497.vcd" predelay ".25"
	scene "scenes/Player/Soldier/low/3499.vcd" predelay ".25"
}
Rule MedicFollowSoldier
{
	criteria ConceptPlayerMedic IsOnMedic IsSoldier IsNotCrossHairEnemy NotLowHealth SoldierIsNotStillonFire
	ApplyContext "ScoutKillSpeech:1:10"
	Response MedicFollowSoldier
}

Response SoldierJarateHit
{
	scene "scenes/Player/Soldier/low/1051.vcd"
	scene "scenes/Player/Soldier/low/1155.vcd"
	scene "scenes/Player/Soldier/low/1353.vcd"
	scene "scenes/Player/Soldier/low/1152.vcd"
}
Rule SoldierJarateHit
{
	criteria ConceptJarateHit IsSoldier 50PercentChance
	Response SoldierJarateHit
}

// Invulnerable lines
Response InvulnerableSpeechSoldier
{
	scene "scenes/Player/Soldier/low/1191.vcd"
	scene "scenes/Player/Soldier/low/1194.vcd"
	scene "scenes/Player/Soldier/low/1200.vcd"
	scene "scenes/Player/Soldier/low/1204.vcd"
	scene "scenes/Player/Soldier/low/1192.vcd"
	scene "scenes/Player/Soldier/low/1189.vcd"
	scene "scenes/Player/Soldier/low/1201.vcd"
}
Rule InvulnerableSpeechSoldier
{
	criterion ConceptFireWeapon IsSoldier IsInvulnerable SoldierNotInvulnerableSpeech
	ApplyContext "SoldierInvulnerableSpeech:1:30"
	Response InvulnerableSpeechSoldier
}

// End custom stuff

// Added the unused Direct Hit screams here, as they can't be added to the taunt.
Response KilledPlayerMeleeSoldier
{
	scene "scenes/Player/Soldier/low/1185.vcd" 
	scene "scenes/Player/Soldier/low/3403.vcd" 
	scene "scenes/Player/Soldier/low/3405.vcd" 
	scene "scenes/Player/Soldier/low/3406.vcd" 
}
Rule KilledPlayerMeleeSoldier
{
	criteria ConceptKilledPlayer KilledPlayerDelay 30PercentChance  IsWeaponMelee SoldierNotKillSpeechMelee IsSoldier
	ApplyContext "SoldierKillSpeechMelee:1:10"
	Response KilledPlayerMeleeSoldier
}

Response KilledPlayerVeryManySoldier
{
	scene "scenes/Player/Soldier/low/1206.vcd" 
	scene "scenes/Player/Soldier/low/1188.vcd" 
}
Rule KilledPlayerVeryManySoldier
{
	criteria ConceptKilledPlayer IsVeryManyRecentKills 50PercentChance IsWeaponPrimary KilledPlayerDelay SoldierNotKillSpeech IsSoldier
	ApplyContext "SoldierKillSpeech:1:10"
	Response KilledPlayerVeryManySoldier
}

Response PlayerKilledCapperSoldier
{
	scene "scenes/Player/Soldier/low/1064.vcd" 
	scene "scenes/Player/Soldier/low/1062.vcd" 
	scene "scenes/Player/Soldier/low/1063.vcd" 
}
Rule PlayerKilledCapperSoldier
{
	criteria ConceptCapBlocked IsSoldier
	ApplyContext "SoldierKillSpeech:1:10"
	Response PlayerKilledCapperSoldier
}

Response PlayerKilledDominatingSoldier
{
	scene "scenes/Player/Soldier/low/1132.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/1346.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/1348.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/1133.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/1134.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/1347.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/1135.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/1349.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/1195.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/1202.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingSoldier
{
	criteria ConceptKilledPlayer IsSoldier IsDominated
	ApplyContext "SoldierKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingSoldier
}

Response PlayerKilledDominatingDemomanSoldier
{
	scene "scenes/Player/Soldier/low/3407.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3408.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3409.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3410.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3411.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3412.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingDemomanSoldier
{
	criteria ConceptKilledPlayer IsSoldier IsDominated  IsVictimDemoman
	ApplyContext "SoldierKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingDemomanSoldier
}

Response PlayerKilledDominatingEngineerSoldier
{
	scene "scenes/Player/Soldier/low/3418.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3419.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3420.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3421.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3422.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3423.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingEngineerSoldier
{
	criteria ConceptKilledPlayer IsSoldier IsDominated  IsVictimEngineer
	ApplyContext "SoldierKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingEngineerSoldier
}

Response PlayerKilledDominatingHeavySoldier
{
	scene "scenes/Player/Soldier/low/3424.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3425.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3426.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3427.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3428.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3429.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3430.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingHeavySoldier
{
	criteria ConceptKilledPlayer IsSoldier IsDominated  IsVictimHeavy
	ApplyContext "SoldierKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingHeavySoldier
}

Response PlayerKilledDominatingMedicSoldier
{
	scene "scenes/Player/Soldier/low/3431.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3432.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3433.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3434.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3435.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3436.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3437.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingMedicSoldier
{
	criteria ConceptKilledPlayer IsSoldier IsDominated  IsVictimMedic
	ApplyContext "SoldierKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingMedicSoldier
}

Response PlayerKilledDominatingPyroSoldier
{
	scene "scenes/Player/Soldier/low/3438.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3439.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3440.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3441.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3442.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3443.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3444.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3445.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3446.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingPyroSoldier
{
	criteria ConceptKilledPlayer IsSoldier IsDominated  IsVictimPyro
	ApplyContext "SoldierKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingPyroSoldier
}

Response PlayerKilledDominatingScoutSoldier
{
	scene "scenes/Player/Soldier/low/3447.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3448.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3449.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3450.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3451.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3452.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3453.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3454.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3455.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3456.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3457.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingScoutSoldier
{
	criteria ConceptKilledPlayer IsSoldier IsDominated  IsVictimScout
	ApplyContext "SoldierKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingScoutSoldier
}

Response PlayerKilledDominatingSniperSoldier
{
	scene "scenes/Player/Soldier/low/3458.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3459.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3460.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3461.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3462.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3463.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3464.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3465.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3466.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3467.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3468.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3469.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3470.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3471.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingSniperSoldier
{
	criteria ConceptKilledPlayer IsSoldier IsDominated  IsVictimSniper
	ApplyContext "SoldierKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingSniperSoldier
}

Response PlayerKilledDominatingSoldierSoldier
{
	scene "scenes/Player/Soldier/low/3472.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3473.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3474.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3475.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3476.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3477.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingSoldierSoldier
{
	criteria ConceptKilledPlayer IsSoldier IsDominated  IsVictimSoldier
	ApplyContext "SoldierKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingSoldierSoldier
}

Response PlayerKilledDominatingSpySoldier
{
	scene "scenes/Player/Soldier/low/3478.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3479.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3480.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3481.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3482.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3483.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3484.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/3485.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingSpySoldier
{
	criteria ConceptKilledPlayer IsSoldier IsDominated  IsVictimSpy
	ApplyContext "SoldierKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingSpySoldier
}

Response PlayerKilledForRevengeSoldier
{
	scene "scenes/Player/Soldier/low/1060.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/1065.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/1096.vcd" predelay "2.5"
}
Rule PlayerKilledForRevengeSoldier
{
	criteria ConceptKilledPlayer IsSoldier IsRevenge
	ApplyContext "SoldierKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledForRevengeSoldier
}

Response PlayerKilledObjectSoldier
{
	scene "scenes/Player/Soldier/low/1055.vcd" 
	scene "scenes/Player/Soldier/low/1345.vcd" 
	scene "scenes/Player/Soldier/low/1172.vcd" 
	scene "scenes/Player/Soldier/low/1175.vcd" 
	scene "scenes/Player/Soldier/low/1182.vcd" 
}
Rule PlayerKilledObjectSoldier
{
	criteria ConceptKilledObject IsSoldier 30PercentChance IsARecentKill
	ApplyContext "SoldierKillSpeechObject:1:30"
	Response PlayerKilledObjectSoldier
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Pain
//--------------------------------------------------------------------------------------------------------------
Response PlayerAttackerPainSoldier
{
	scene "scenes/Player/Soldier/low/1165.vcd" 
	scene "scenes/Player/Soldier/low/1166.vcd" 
	scene "scenes/Player/Soldier/low/1167.vcd" 
	scene "scenes/Player/Soldier/low/1371.vcd" 
	scene "scenes/Player/Soldier/low/1372.vcd" 
	scene "scenes/Player/Soldier/low/1373.vcd" 
}
Rule PlayerAttackerPainSoldier
{
	criteria ConceptAttackerPain IsSoldier IsNotDominating
	Response PlayerAttackerPainSoldier
}

Response PlayerOnFireSoldier
{
	scene "scenes/Player/Soldier/low/1052.vcd" 
}
Rule PlayerOnFireSoldier
{
	criteria ConceptFire IsSoldier SoldierIsNotStillonFire IsNotDominating
	ApplyContext "SoldierOnFire:1:7"
	Response PlayerOnFireSoldier
}

Response PlayerOnFireRareSoldier
{
	scene "scenes/Player/Soldier/low/1053.vcd" 
	scene "scenes/Player/Soldier/low/1054.vcd" 
}
Rule PlayerOnFireRareSoldier
{
	criteria ConceptFire IsSoldier 10PercentChance SoldierIsNotStillonFire IsNotDominating
	ApplyContext "SoldierOnFire:1:7"
	Response PlayerOnFireRareSoldier
}

Response PlayerPainSoldier
{
	scene "scenes/Player/Soldier/low/1168.vcd" 
	scene "scenes/Player/Soldier/low/1169.vcd" 
	scene "scenes/Player/Soldier/low/1170.vcd" 
	scene "scenes/Player/Soldier/low/1374.vcd" 
	scene "scenes/Player/Soldier/low/1375.vcd" 
	scene "scenes/Player/Soldier/low/1376.vcd" 
	scene "scenes/Player/Soldier/low/1377.vcd" 
	scene "scenes/Player/Soldier/low/1378.vcd" 
}
Rule PlayerPainSoldier
{
	criteria ConceptPain IsSoldier IsNotDominating
	Response PlayerPainSoldier
}

Response PlayerStillOnFireSoldier
{
	scene "scenes/Player/Soldier/low/1926.vcd" 
}
Rule PlayerStillOnFireSoldier
{
	criteria ConceptFire IsSoldier  SoldierIsStillonFire IsNotDominating
	ApplyContext "SoldierOnFire:1:7"
	Response PlayerStillOnFireSoldier
}


//--------------------------------------------------------------------------------------------------------------
// Duel Speech
//--------------------------------------------------------------------------------------------------------------
Response AcceptedDuelSoldier
{
	scene "scenes/Player/Soldier/low/1062.vcd" 
	scene "scenes/Player/Soldier/low/1172.vcd" 
	scene "scenes/Player/Soldier/low/1175.vcd" 
	scene "scenes/Player/Soldier/low/1174.vcd" 
	scene "scenes/Player/Soldier/low/1190.vcd" 
	scene "scenes/Player/Soldier/low/1221.vcd" 
}
Rule AcceptedDuelSoldier
{
	criteria ConceptIAcceptDuel IsSoldier
	Response AcceptedDuelSoldier
}

Response MeleeDareSoldier
{
	scene "scenes/Player/Soldier/low/1094.vcd" 
	scene "scenes/Player/Soldier/low/1188.vcd" 
	scene "scenes/Player/Soldier/low/1195.vcd" 
	scene "scenes/Player/Soldier/low/1196.vcd" 
	scene "scenes/Player/Soldier/low/1202.vcd" 
	scene "scenes/Player/Soldier/low/1210.vcd" 
}
Rule MeleeDareSoldier
{
	criteria ConceptRequestDuel IsSoldier
	Response MeleeDareSoldier
}

Response RejectedDuelSoldier
{
	scene "scenes/Player/Soldier/low/3487.vcd" 
	scene "scenes/Player/Soldier/low/1120.vcd" 
	scene "scenes/Player/Soldier/low/1207.vcd" 
}
Rule RejectedDuelSoldier
{
	criteria ConceptDuelRejected IsSoldier
	Response RejectedDuelSoldier
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 1
//--------------------------------------------------------------------------------------------------------------
Response PlayerGoSoldier
{
	scene "scenes/Player/Soldier/low/1092.vcd" 
	scene "scenes/Player/Soldier/low/1093.vcd" 
	scene "scenes/Player/Soldier/low/1094.vcd" // Restored
}
Rule PlayerGoSoldier
{
	criteria ConceptPlayerGo IsSoldier
	Response PlayerGoSoldier
}

Response PlayerHeadLeftSoldier
{
	scene "scenes/Player/Soldier/low/1098.vcd" 
	scene "scenes/Player/Soldier/low/1100.vcd" 
	scene "scenes/Player/Soldier/low/1099.vcd" 
}
Rule PlayerHeadLeftSoldier
{
	criteria ConceptPlayerLeft  IsSoldier
	Response PlayerHeadLeftSoldier
}

Response PlayerHeadRightSoldier
{
	scene "scenes/Player/Soldier/low/1103.vcd" 
	scene "scenes/Player/Soldier/low/1101.vcd" 
	scene "scenes/Player/Soldier/low/1102.vcd" 
}
Rule PlayerHeadRightSoldier
{
	criteria ConceptPlayerRight  IsSoldier
	Response PlayerHeadRightSoldier
}

Response PlayerHelpSoldier
{
	scene "scenes/Player/Soldier/low/1104.vcd" 
	scene "scenes/Player/Soldier/low/1105.vcd" 
	scene "scenes/Player/Soldier/low/1106.vcd" 
}
Rule PlayerHelpSoldier
{
	criteria ConceptPlayerHelp IsSoldier
	Response PlayerHelpSoldier
}

Response PlayerHelpCaptureSoldier
{
	scene "scenes/Player/Soldier/low/1109.vcd" 
	scene "scenes/Player/Soldier/low/1107.vcd" 
	scene "scenes/Player/Soldier/low/1108.vcd" 
}
Rule PlayerHelpCaptureSoldier
{
	criteria ConceptPlayerHelp IsSoldier IsOnCappableControlPoint
	ApplyContext "SoldierHelpCap:1:10"
	Response PlayerHelpCaptureSoldier
}

Response PlayerHelpCapture2Soldier
{
	scene "scenes/Player/Soldier/low/1187.vcd" 
	scene "scenes/Player/Soldier/low/1354.vcd" 
	scene "scenes/Player/Soldier/low/1355.vcd" 
}
Rule PlayerHelpCapture2Soldier
{
	criteria ConceptPlayerHelp IsSoldier IsOnCappableControlPoint IsHelpCapSoldier
	Response PlayerHelpCapture2Soldier
}

Response PlayerHelpDefendSoldier
{
	scene "scenes/Player/Soldier/low/1110.vcd" 
	scene "scenes/Player/Soldier/low/1111.vcd" 
	scene "scenes/Player/Soldier/low/1112.vcd" 
	scene "scenes/Player/Soldier/low/1113.vcd" 
}
Rule PlayerHelpDefendSoldier
{
	criteria ConceptPlayerHelp IsSoldier IsOnFriendlyControlPoint
	Response PlayerHelpDefendSoldier
}

Response PlayerMedicSoldier
{
	scene "scenes/Player/Soldier/low/1139.vcd" 
	scene "scenes/Player/Soldier/low/1140.vcd" 
	scene "scenes/Player/Soldier/low/1141.vcd" 
}
Rule PlayerMedicSoldier
{
	criteria ConceptPlayerMedic IsSoldier
	Response PlayerMedicSoldier
}

Response PlayerAskForBallSoldier
{
}
Rule PlayerAskForBallSoldier
{
	criteria ConceptPlayerAskForBall IsSoldier
	Response PlayerAskForBallSoldier
}

Response PlayerMoveUpSoldier
{
	scene "scenes/Player/Soldier/low/1142.vcd" 
	scene "scenes/Player/Soldier/low/1143.vcd" 
	scene "scenes/Player/Soldier/low/1144.vcd" 
}
Rule PlayerMoveUpSoldier
{
	criteria ConceptPlayerMoveUp  IsSoldier
	Response PlayerMoveUpSoldier
}

Response PlayerNoSoldier
{
	scene "scenes/Player/Soldier/low/1159.vcd" 
	scene "scenes/Player/Soldier/low/1161.vcd" 
	scene "scenes/Player/Soldier/low/1160.vcd" 
}
Rule PlayerNoSoldier
{
	criteria ConceptPlayerNo  IsSoldier
	Response PlayerNoSoldier
}

Response PlayerThanksSoldier
{
	scene "scenes/Player/Soldier/low/1211.vcd" 
	scene "scenes/Player/Soldier/low/1212.vcd" 
}
Rule PlayerThanksSoldier
{
	criteria ConceptPlayerThanks IsSoldier
	Response PlayerThanksSoldier
}

// Custom Assist kill response
// As there is no actual concept for assist kills, this is the second best method.
// Say thanks after you kill more than one person.

Response KilledPlayerAssistSoldier
{
	scene "scenes/Player/Soldier/low/1186.vcd"
}
Rule KilledPlayerAssistSoldier
{
	criteria ConceptPlayerThanks IsSoldier IsARecentKill KilledPlayerDelay SoldierNotAssistSpeech
	ApplyContext "SoldierAssistSpeech:1:20"
	Response KilledPlayerAssistSoldier
}
// End custom

Response PlayerYesSoldier
{
	scene "scenes/Player/Soldier/low/1350.vcd" 
	scene "scenes/Player/Soldier/low/1220.vcd" 
	scene "scenes/Player/Soldier/low/1221.vcd" 
	scene "scenes/Player/Soldier/low/1219.vcd" 
}
Rule PlayerYesSoldier
{
	criteria ConceptPlayerYes  IsSoldier
	Response PlayerYesSoldier
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 2
//--------------------------------------------------------------------------------------------------------------
Response PlayerActivateChargeSoldier
{
	scene "scenes/Player/Soldier/low/1040.vcd" 
	scene "scenes/Player/Soldier/low/1041.vcd" 
	scene "scenes/Player/Soldier/low/1042.vcd" 
}
Rule PlayerActivateChargeSoldier
{
	criteria ConceptPlayerActivateCharge IsSoldier
	Response PlayerActivateChargeSoldier
}

Response PlayerCloakedSpySoldier
{
	scene "scenes/Player/Soldier/low/1071.vcd" 
	scene "scenes/Player/Soldier/low/1072.vcd" 
	scene "scenes/Player/Soldier/low/1070.vcd" 
}
Rule PlayerCloakedSpySoldier
{
	criteria ConceptPlayerCloakedSpy IsSoldier
	Response PlayerCloakedSpySoldier
}

Response PlayerDispenserHereSoldier
{
	scene "scenes/Player/Soldier/low/1146.vcd" 
}
Rule PlayerDispenserHereSoldier
{
	criteria ConceptPlayerDispenserHere IsSoldier
	Response PlayerDispenserHereSoldier
}

Response PlayerIncomingSoldier
{
	scene "scenes/Player/Soldier/low/1114.vcd" 
}
Rule PlayerIncomingSoldier
{
	criteria ConceptPlayerIncoming IsSoldier
	Response PlayerIncomingSoldier
}

Response PlayerSentryAheadSoldier
{
	scene "scenes/Player/Soldier/low/1177.vcd" 
	scene "scenes/Player/Soldier/low/1178.vcd" 
	scene "scenes/Player/Soldier/low/1176.vcd" 
}
Rule PlayerSentryAheadSoldier
{
	criteria ConceptPlayerSentryAhead IsSoldier
	Response PlayerSentryAheadSoldier
}

Response PlayerSentryHereSoldier
{
	scene "scenes/Player/Soldier/low/1148.vcd" 
}
Rule PlayerSentryHereSoldier
{
	criteria ConceptPlayerSentryHere IsSoldier
	Response PlayerSentryHereSoldier
}

Response PlayerTeleporterHereSoldier
{
	scene "scenes/Player/Soldier/low/1150.vcd" 
}
Rule PlayerTeleporterHereSoldier
{
	criteria ConceptPlayerTeleporterHere IsSoldier
	Response PlayerTeleporterHereSoldier
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 3
//--------------------------------------------------------------------------------------------------------------
Response PlayerBattleCrySoldier
{
	scene "scenes/Player/Soldier/low/1055.vcd" 
	scene "scenes/Player/Soldier/low/1057.vcd" 
	scene "scenes/Player/Soldier/low/1058.vcd" 
	scene "scenes/Player/Soldier/low/1059.vcd" 
	scene "scenes/Player/Soldier/low/1056.vcd" 
	scene "scenes/Player/Soldier/low/1060.vcd" 
}
Rule PlayerBattleCrySoldier
{
	criteria ConceptPlayerBattleCry IsSoldier
	Response PlayerBattleCrySoldier
}

// Custom stuff - melee dare
// Look at enemy, then do battle cry voice command while holding a melee weapon.
Response MeleeDareCombatSoldier
{
	scene "scenes/Player/Soldier/low/1196.vcd"
	scene "scenes/Player/Soldier/low/1210.vcd"
	scene "scenes/Player/Soldier/low/1205.vcd"
	scene "scenes/Player/Soldier/low/1208.vcd"
	scene "scenes/Player/Soldier/low/1203.vcd"
	scene "scenes/Player/Soldier/low/1190.vcd" 
}
Rule MeleeDareCombatSoldier
{
	criteria ConceptPlayerBattleCry IsWeaponMelee IsSoldier IsCrosshairEnemy
	Response MeleeDareCombatSoldier
}
//End custom

Response PlayerCheersSoldier
{
	scene "scenes/Player/Soldier/low/1065.vcd" 
	scene "scenes/Player/Soldier/low/1068.vcd" 
	scene "scenes/Player/Soldier/low/1066.vcd" 
	scene "scenes/Player/Soldier/low/1064.vcd" 
	scene "scenes/Player/Soldier/low/1062.vcd" 
	scene "scenes/Player/Soldier/low/1063.vcd" 
}
Rule PlayerCheersSoldier
{
	criteria ConceptPlayerCheers IsSoldier
	Response PlayerCheersSoldier
}

Response PlayerGoodJobSoldier
{
	scene "scenes/Player/Soldier/low/1095.vcd" 
	scene "scenes/Player/Soldier/low/1096.vcd" 
	scene "scenes/Player/Soldier/low/1097.vcd" 
}
Rule PlayerGoodJobSoldier
{
	criteria ConceptPlayerGoodJob IsSoldier
	Response PlayerGoodJobSoldier
}

Response PlayerJeersSoldier
{
	scene "scenes/Player/Soldier/low/1120.vcd" 
	scene "scenes/Player/Soldier/low/1121.vcd" 
	scene "scenes/Player/Soldier/low/1122.vcd" 
	scene "scenes/Player/Soldier/low/1123.vcd" 
	scene "scenes/Player/Soldier/low/1124.vcd" 
	scene "scenes/Player/Soldier/low/1125.vcd" 
	scene "scenes/Player/Soldier/low/1126.vcd" 
	scene "scenes/Player/Soldier/low/1127.vcd" 
	scene "scenes/Player/Soldier/low/1128.vcd" 
	scene "scenes/Player/Soldier/low/1129.vcd" 
	scene "scenes/Player/Soldier/low/1130.vcd" 
	scene "scenes/Player/Soldier/low/1131.vcd" 
}
Rule PlayerJeersSoldier
{
	criteria ConceptPlayerJeers IsSoldier
	Response PlayerJeersSoldier
}

Response PlayerLostPointSoldier
{
	scene "scenes/Player/Soldier/low/1151.vcd" 
	scene "scenes/Player/Soldier/low/1152.vcd" 
	scene "scenes/Player/Soldier/low/1153.vcd" 
	scene "scenes/Player/Soldier/low/1154.vcd" 
	scene "scenes/Player/Soldier/low/1155.vcd" 
	scene "scenes/Player/Soldier/low/1353.vcd" 
}
Rule PlayerLostPointSoldier
{
	criteria ConceptPlayerLostPoint IsSoldier
	Response PlayerLostPointSoldier
}

Response PlayerNegativeSoldier
{
	scene "scenes/Player/Soldier/low/1151.vcd" 
	scene "scenes/Player/Soldier/low/1152.vcd" 
	scene "scenes/Player/Soldier/low/1153.vcd" 
	scene "scenes/Player/Soldier/low/1154.vcd" 
	scene "scenes/Player/Soldier/low/1155.vcd" 
	scene "scenes/Player/Soldier/low/1353.vcd" 
}
Rule PlayerNegativeSoldier
{
	criteria ConceptPlayerNegative IsSoldier
	Response PlayerNegativeSoldier
}

Response PlayerNiceShotSoldier
{
	scene "scenes/Player/Soldier/low/1156.vcd" 
	scene "scenes/Player/Soldier/low/1157.vcd" 
	scene "scenes/Player/Soldier/low/1158.vcd" 
}
Rule PlayerNiceShotSoldier
{
	criteria ConceptPlayerNiceShot IsSoldier
	Response PlayerNiceShotSoldier
}

Response PlayerPositiveSoldier
{
	scene "scenes/Player/Soldier/low/1345.vcd" 
	scene "scenes/Player/Soldier/low/1172.vcd" 
	scene "scenes/Player/Soldier/low/1175.vcd" 
	scene "scenes/Player/Soldier/low/1174.vcd" 
	scene "scenes/Player/Soldier/low/1171.vcd" 
}

Response PlayerTauntsSoldier
{
	scene "scenes/Player/Soldier/low/1136.vcd" 
	scene "scenes/Player/Soldier/low/1137.vcd" 
	scene "scenes/Player/Soldier/low/1138.vcd" 
	scene "scenes/Player/Soldier/low/1351.vcd" 
	scene "scenes/Player/Soldier/low/1352.vcd" 
}
Rule PlayerPositiveSoldier
{
	criteria ConceptPlayerPositive IsSoldier
	Response PlayerPositiveSoldier
	Response PlayerTauntsSoldier
}

Response PlayerRobotNoisesSoldier
{
	scene "scenes/Player/Soldier/low/robot01.vcd"
	scene "scenes/Player/Soldier/low/robot02.vcd"
	scene "scenes/Player/Soldier/low/robot03.vcd"
	scene "scenes/Player/Soldier/low/robot04.vcd"
	scene "scenes/Player/Soldier/low/robot05.vcd"
	scene "scenes/Player/Soldier/low/robot06.vcd"
	scene "scenes/Player/Soldier/low/robot07.vcd"
}
Rule PlayerRobotNoisesSoldier
{
	criteria ConceptFireWeapon IsSoldier IsRobotCostume SoldierNotRobotNoises 50PercentChance
	ApplyContext "SoldierRobotNoises:1:30"
	Response PlayerRobotNoisesSoldier
}

Response PlayerBattleCryRobotSoldier
{
	scene "scenes/Player/Soldier/low/robot08.vcd"
	scene "scenes/Player/Soldier/low/robot09.vcd"
}
Rule PlayerBattleCryRobotSoldier
{
	criteria ConceptPlayerBattleCry IsSoldier IsRobotCostume
	Response PlayerBattleCryRobotSoldier
}

Response KilledPlayerRobotSoldier
{
	scene "scenes/Player/Soldier/low/robot10.vcd"
	scene "scenes/Player/Soldier/low/robot11.vcd"
	scene "scenes/Player/Soldier/low/robot12.vcd"
	scene "scenes/Player/Soldier/low/robot13.vcd"
	scene "scenes/Player/Soldier/low/robot14.vcd"
}
Rule KilledPlayerRobotSoldier
{
	criteria ConceptKilledPlayer IsSoldier IsRobotCostume SoldierNotKillSpeech 50PercentChance
	ApplyContext "SoldierKillSpeech:1:10"
	Response KilledPlayerRobotSoldier
}

Response KilledPlayerManyRobotSoldier
{
	scene "scenes/Player/Soldier/low/robot15.vcd"
	scene "scenes/Player/Soldier/low/robot16.vcd"
	scene "scenes/Player/Soldier/low/robot17.vcd"
	scene "scenes/Player/Soldier/low/robot18.vcd"
	scene "scenes/Player/Soldier/low/robot19.vcd"
}
Rule KilledPlayerManyRobotSoldier
{
	criteria ConceptKilledPlayer IsManyRecentKills 30PercentChance IsWeaponPrimary KilledPlayerDelay SoldierNotKillSpeech IsSoldier IsRobotCostume
	ApplyContext "SoldierKillSpeech:1:10"
	Response KilledPlayerManyRobotSoldier
}

Response PlayerKilledForRevengeRobotSoldier
{
	scene "scenes/Player/Soldier/low/robot20.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/1060.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/1065.vcd" predelay "2.5"
}
Rule PlayerKilledForRevengeRobotSoldier
{
	criteria ConceptKilledPlayer IsSoldier IsRevenge IsRobotCostume
	ApplyContext "SoldierKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledForRevengeRobotSoldier
}

Response PlayerKilledDominatingRobotSoldier
{
	scene "scenes/Player/Soldier/low/robot21.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/robot22.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/robot23.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/robot24.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/robot25.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/robot26.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/robot27.vcd" predelay "2.5"
	scene "scenes/Player/Soldier/low/robot28.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingRobotSoldier
{
	criteria ConceptKilledPlayer IsSoldier IsDominated IsRobotCostume
	ApplyContext "SoldierKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingRobotSoldier
}

//--------------------------------------------------------------------------------------------------------------
// MvM Speech
//--------------------------------------------------------------------------------------------------------------
Response MvMBombDroppedSoldier
{
	scene "scenes/Player/Soldier/low/4284.vcd" 
	scene "scenes/Player/Soldier/low/4285.vcd" 
}
Rule MvMBombDroppedSoldier
{
	criteria ConceptMvMBombDropped 5PercentChance IsMvMDefender IsSoldier 
	Response MvMBombDroppedSoldier
}

Response MvMBombCarrierUpgrade1Soldier
{
	scene "scenes/Player/Soldier/low/4280.vcd" 
}
Rule MvMBombCarrierUpgrade1Soldier
{
	criteria ConceptMvMBombCarrierUpgrade1 5PercentChance IsMvMDefender IsSoldier 
	Response MvMBombCarrierUpgrade1Soldier
}

Response MvMBombCarrierUpgrade2Soldier
{
	scene "scenes/Player/Soldier/low/4281.vcd" 
}
Rule MvMBombCarrierUpgrade2Soldier
{
	criteria ConceptMvMBombCarrierUpgrade2 5PercentChance IsMvMDefender IsSoldier 
	Response MvMBombCarrierUpgrade2Soldier
}

Response MvMBombCarrierUpgrade3Soldier
{
	scene "scenes/Player/Soldier/low/4282.vcd" 
}
Rule MvMBombCarrierUpgrade3Soldier
{
	criteria ConceptMvMBombCarrierUpgrade3 5PercentChance IsMvMDefender IsSoldier 
	Response MvMBombCarrierUpgrade3Soldier
}

Response MvMDefenderDiedScoutSoldier
{
	scene "scenes/Player/Soldier/low/4246.vcd" 
}
Rule MvMDefenderDiedScoutSoldier
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimScout IsSoldier 
	Response MvMDefenderDiedScoutSoldier
}

Response MvMDefenderDiedSpySoldier
{
	scene "scenes/Player/Soldier/low/4247.vcd" 
}
Rule MvMDefenderDiedSpySoldier
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimSpy IsSoldier 
	Response MvMDefenderDiedSpySoldier
}

Response MvMDefenderDiedHeavySoldier
{
	scene "scenes/Player/Soldier/low/4248.vcd" 
}
Rule MvMDefenderDiedHeavySoldier
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimHeavy IsSoldier 
	Response MvMDefenderDiedHeavySoldier
}

Response MvMDefenderDiedSoldierSoldier
{
	scene "scenes/Player/Soldier/low/4249.vcd" 
}
Rule MvMDefenderDiedSoldierSoldier
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimSoldier IsSoldier 
	Response MvMDefenderDiedSoldierSoldier
}

Response MvMDefenderDiedMedicSoldier
{
	scene "scenes/Player/Soldier/low/4250.vcd" 
}
Rule MvMDefenderDiedMedicSoldier
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimMedic IsSoldier 
	Response MvMDefenderDiedMedicSoldier
}

Response MvMDefenderDiedDemomanSoldier
{
	scene "scenes/Player/Soldier/low/4251.vcd" 
}
Rule MvMDefenderDiedDemomanSoldier
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimDemoman IsSoldier 
	Response MvMDefenderDiedDemomanSoldier
}

Response MvMDefenderDiedPyroSoldier
{
	scene "scenes/Player/Soldier/low/4252.vcd" 
}
Rule MvMDefenderDiedPyroSoldier
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimPyro IsSoldier 
	Response MvMDefenderDiedPyroSoldier
}

Response MvMDefenderDiedSniperSoldier
{
	scene "scenes/Player/Soldier/low/4253.vcd" 
}
Rule MvMDefenderDiedSniperSoldier
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimSniper IsSoldier 
	Response MvMDefenderDiedSniperSoldier
}

Response MvMDefenderDiedEngineerSoldier
{
	scene "scenes/Player/Soldier/low/4254.vcd" 
}
Rule MvMDefenderDiedEngineerSoldier
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimEngineer IsSoldier 
	Response MvMDefenderDiedEngineerSoldier
}

Response MvMFirstBombPickupSoldier
{
	scene "scenes/Player/Soldier/low/4277.vcd" 
	scene "scenes/Player/Soldier/low/4279.vcd" 
}
Rule MvMFirstBombPickupSoldier
{
	criteria ConceptMvMFirstBombPickup 5PercentChance IsMvMDefender IsSoldier
	Response MvMFirstBombPickupSoldier
}

Response MvMBombPickupSoldier
{
	scene "scenes/Player/Soldier/low/4276.vcd" 
}
Rule MvMBombPickupSoldier
{
	criteria ConceptMvMBombPickup 5PercentChance IsMvMDefender IsSoldier
	Response MvMBombPickupSoldier
}

Response MvMSniperCalloutSoldier
{
	scene "scenes/Player/Soldier/low/4258.vcd" 
}
Rule MvMSniperCalloutSoldier
{
	criteria ConceptMvMSniperCallout 50PercentChance IsMvMDefender IsSoldier
	Response MvMSniperCalloutSoldier
}

Response MvMSentryBusterSoldier
{
	scene "scenes/Player/Soldier/low/4295.vcd" 
}
Rule MvMSentryBusterSoldier
{
	criteria ConceptMvMSentryBuster 50PercentChance IsMvMDefender IsSoldier
	Response MvMSentryBusterSoldier
}

Response MvMSentryBusterDownSoldier
{
	scene "scenes/Player/Soldier/low/4296.vcd" 
}
Rule MvMSentryBusterDownSoldier
{
	criteria ConceptMvMSentryBusterDown 20PercentChance IsMvMDefender IsSoldier
	Response MvMSentryBusterDownSoldier
}

Response MvMLastManStandingSoldier
{
	scene "scenes/Player/Soldier/low/4255.vcd" 
	scene "scenes/Player/Soldier/low/4257.vcd" 
}
Rule MvMLastManStandingSoldier
{
	criteria ConceptMvMLastManStanding 20PercentChance IsMvMDefender IsSoldier
	Response MvMLastManStandingSoldier
}

Response MvMEncourageMoneySoldier
{
	scene "scenes/Player/Soldier/low/4270.vcd" 
}
Rule MvMEncourageMoneySoldier
{
	criteria ConceptMvMEncourageMoney 50PercentChance IsMvMDefender IsSoldier
	Response MvMEncourageMoneySoldier
}

Response MvMEncourageUpgradeSoldier
{
	scene "scenes/Player/Soldier/low/4274.vcd" 
}
Rule MvMEncourageUpgradeSoldier
{
	criteria ConceptMvMEncourageUpgrade 50PercentChance IsMvMDefender IsSoldier
	Response MvMEncourageUpgradeSoldier
}

Response MvMUpgradeCompleteSoldier
{
	scene "scenes/Player/Soldier/low/4271.vcd" 
	scene "scenes/Player/Soldier/low/4272.vcd" 
	scene "scenes/Player/Soldier/low/4273.vcd" 
}
Rule MvMUpgradeCompleteSoldier
{
	criteria ConceptMvMUpgradeComplete 5PercentChance IsMvMDefender IsSoldier
	Response MvMUpgradeCompleteSoldier
}

Response MvMGiantCalloutSoldier
{
	scene "scenes/Player/Soldier/low/4297.vcd" 
	scene "scenes/Player/Soldier/low/4301.vcd"
}
Rule MvMGiantCalloutSoldier
{
	criteria ConceptMvMGiantCallout 20PercentChance IsMvMDefender IsSoldier
	Response MvMGiantCalloutSoldier
}

Response MvMGiantHasBombSoldier
{
	scene "scenes/Player/Soldier/low/4302.vcd" 
	scene "scenes/Player/Soldier/low/4303.vcd" 
}
Rule MvMGiantHasBombSoldier
{
	criteria ConceptMvMGiantHasBomb 20PercentChance IsMvMDefender IsSoldier
	Response MvMGiantHasBombSoldier
}

Response MvMSappedRobotSoldier
{
	scene "scenes/Player/Soldier/low/4259.vcd" 
	scene "scenes/Player/Soldier/low/4260.vcd" 
}
Rule MvMSappedRobotSoldier
{
	criteria ConceptMvMSappedRobot 50PercentChance IsMvMDefender IsSoldier
	Response MvMSappedRobotSoldier
}

Response MvMCloseCallSoldier
{
	scene "scenes/Player/Soldier/low/4283.vcd" 
}
Rule MvMCloseCallSoldier
{
	criteria ConceptMvMCloseCall 50PercentChance IsMvMDefender IsSoldier
	Response MvMCloseCallSoldier
}

Response MvMTankCalloutSoldier
{
	scene "scenes/Player/Soldier/low/4287.vcd" 
	scene "scenes/Player/Soldier/low/4288.vcd" 
}
Rule MvMTankCalloutSoldier
{
	criteria ConceptMvMTankCallout 50PercentChance IsMvMDefender IsSoldier
	Response MvMTankCalloutSoldier
}

Response MvMTankDeadSoldier
{
	scene "scenes/Player/Soldier/low/4293.vcd" 
	scene "scenes/Player/Soldier/low/4294.vcd" 
}
Rule MvMTankDeadSoldier
{
	criteria ConceptMvMTankDead 50PercentChance IsMvMDefender IsSoldier
	Response MvMTankDeadSoldier
}

Response MvMTankDeployingSoldier
{
	scene "scenes/Player/Soldier/low/4292.vcd" 
}
Rule MvMTankDeployingSoldier
{
	criteria ConceptMvMTankDeploying 50PercentChance IsMvMDefender IsSoldier
	Response MvMTankDeployingSoldier
}

Response MvMAttackTheTankSoldier
{
	scene "scenes/Player/Soldier/low/4289.vcd" 
	scene "scenes/Player/Soldier/low/4290.vcd" 
	scene "scenes/Player/Soldier/low/4291.vcd" 
}
Rule MvMAttackTheTankSoldier
{
	criteria ConceptMvMAttackTheTank 50PercentChance IsMvMDefender IsSoldier
	Response MvMAttackTheTankSoldier
}

Response MvMTauntSoldier
{
	scene "scenes/Player/Soldier/low/4262.vcd" 
	scene "scenes/Player/Soldier/low/4263.vcd" 
	scene "scenes/Player/Soldier/low/4264.vcd" 
	scene "scenes/Player/Soldier/low/4265.vcd" 
	scene "scenes/Player/Soldier/low/4266.vcd" 
	scene "scenes/Player/Soldier/low/4267.vcd" 
}
Rule MvMTauntSoldier
{
	criteria ConceptMvMTaunt 50PercentChance IsMvMDefender IsSoldier
	Response MvMTauntSoldier
}

Response MvMWaveWinSoldier
{
	scene "scenes/Player/Soldier/low/4231.vcd" 
	scene "scenes/Player/Soldier/low/4232.vcd" 
	scene "scenes/Player/Soldier/low/4233.vcd" 
	scene "scenes/Player/Soldier/low/4234.vcd" 
	scene "scenes/Player/Soldier/low/4235.vcd" 
}
Rule MvMWaveWinSoldier
{
	criteria ConceptMvMWaveWin 50PercentChance IsMvMDefender IsSoldier
	Response MvMWaveWinSoldier
}

Response MvMWaveLoseSoldier
{
	scene "scenes/Player/Soldier/low/4236.vcd" 
	scene "scenes/Player/Soldier/low/4237.vcd" 
	scene "scenes/Player/Soldier/low/4238.vcd" 
	scene "scenes/Player/Soldier/low/4239.vcd" 
	scene "scenes/Player/Soldier/low/4240.vcd" 
}
Rule MvMWaveLoseSoldier
{
	criteria ConceptMvMWaveLose 50PercentChance IsMvMDefender IsSoldier
	Response MvMWaveLoseSoldier
}

Response MvMMoneyPickupSoldier
{
	scene "scenes/Player/Soldier/low/4269.vcd" 
}
Rule MvMMoneyPickupSoldier
{
	criteria ConceptMvMMoneyPickup 5PercentChance IsMvMDefender IsSoldier
	Response MvMMoneyPickupSoldier
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Cart
//--------------------------------------------------------------------------------------------------------------
Criterion "SoldierNotSaidCartMovingBackwardD" "SaidCartMovingBackwardD" "!=1" "required" weight 0
Criterion "SoldierNotSaidCartMovingBackwardO" "SaidCartMovingBackwardO" "!=1" "required" weight 0
Criterion "SoldierNotSaidCartMovingForwardD" "SaidCartMovingForwardD" "!=1" "required" weight 0
Criterion "SoldierNotSaidCartMovingForwardO" "SaidCartMovingForwardO" "!=1" "required" weight 0
Criterion "SoldierNotSaidCartMovingStoppedD" "SaidCartMovingStoppedD" "!=1" "required" weight 0
Criterion "SoldierNotSaidCartMovingStoppedO" "SaidCartMovingStoppedO" "!=1" "required" weight 0
Response CartMovingBackwardsDefenseSoldier                                                     
{
	scene "scenes/Player/Soldier/low/7371.vcd"
	scene "scenes/Player/Soldier/low/7375.vcd"
}
Rule CartMovingBackwardsDefenseSoldier                                                     
{
	criteria ConceptCartMovingBackward IsOnDefense IsSoldier SoldierNotSaidCartMovingBackwardD IsNotDisguised 75PercentChance                                                                                                                                                          
	ApplyContext "SaidCartMovingBackwardD:1:20"
	Response CartMovingBackwardsDefenseSoldier                                                     
}
Response CartMovingBackwardsOffenseSoldier                                                     
{
	scene "scenes/Player/Soldier/low/7363.vcd"
	scene "scenes/Player/Soldier/low/7365.vcd"
}
Rule CartMovingBackwardsOffenseSoldier                                                     
{
	criteria ConceptCartMovingBackward IsOnOffense IsSoldier SoldierNotSaidCartMovingBackwardO IsNotDisguised 75PercentChance                                                                                                                                                          
	ApplyContext "SaidCartMovingBackwardO:1:20"
	Response CartMovingBackwardsOffenseSoldier                                                     
}
Response CartMovingForwardDefenseSoldier                                                       
{
	scene "scenes/Player/Soldier/low/7368.vcd"
	scene "scenes/Player/Soldier/low/7369.vcd"
	scene "scenes/Player/Soldier/low/8559.vcd"
}
Rule CartMovingForwardDefenseSoldier                                                       
{
	criteria ConceptCartMovingForward IsOnDefense IsSoldier SoldierNotSaidCartMovingForwardD IsNotDisguised 75PercentChance                                                                                                                                                            
	ApplyContext "SaidCartMovingForwardD:1:20"
	Response CartMovingForwardDefenseSoldier                                                       
}
Response CartMovingForwardOffenseSoldier                                                       
{
	scene "scenes/Player/Soldier/low/7345.vcd"
	scene "scenes/Player/Soldier/low/7346.vcd"
	scene "scenes/Player/Soldier/low/7347.vcd"
	scene "scenes/Player/Soldier/low/7350.vcd"
	scene "scenes/Player/Soldier/low/7351.vcd"
	scene "scenes/Player/Soldier/low/7352.vcd"
	scene "scenes/Player/Soldier/low/7353.vcd"
	scene "scenes/Player/Soldier/low/7356.vcd"
	scene "scenes/Player/Soldier/low/7355.vcd"
	scene "scenes/Player/Soldier/low/7359.vcd"
	scene "scenes/Player/Soldier/low/7362.vcd"
	scene "scenes/Player/Soldier/low/7357.vcd"
	scene "scenes/Player/Soldier/low/7349.vcd"
	scene "scenes/Player/Soldier/low/7348.vcd"
	scene "scenes/Player/Soldier/low/7354.vcd"
	scene "scenes/Player/Soldier/low/7377.vcd"
	scene "scenes/Player/Soldier/low/7378.vcd"
	scene "scenes/Player/Soldier/low/7379.vcd"
	scene "scenes/Player/Soldier/low/7382.vcd"
	scene "scenes/Player/Soldier/low/7383.vcd"
	scene "scenes/Player/Soldier/low/7385.vcd"
}
Rule CartMovingForwardOffenseSoldier                                                       
{
	criteria ConceptCartMovingForward IsOnOffense IsSoldier SoldierNotSaidCartMovingForwardO IsNotDisguised 75PercentChance                                                                                                                                                            
	ApplyContext "SaidCartMovingForwardO:1:20"
	Response CartMovingForwardOffenseSoldier                                                       
}
Response CartMovingStoppedDefenseSoldier                                                       
{
	scene "scenes/Player/Soldier/low/7396.vcd"
	scene "scenes/Player/Soldier/low/7398.vcd"
	scene "scenes/Player/Soldier/low/7400.vcd"
}
Rule CartMovingStoppedDefenseSoldier                                                       
{
	criteria ConceptCartMovingStopped IsOnDefense IsSoldier SoldierNotSaidCartMovingStoppedD IsNotDisguised 75PercentChance                                                                                                                                                            
	ApplyContext "SaidCartMovingStoppedD:1:20"
	Response CartMovingStoppedDefenseSoldier                                                       
}
Response CartMovingStoppedOffenseSoldier                                                       
{
	scene "scenes/Player/Soldier/low/7389.vcd"
	scene "scenes/Player/Soldier/low/7390.vcd"
	scene "scenes/Player/Soldier/low/7388.vcd"
}
Rule CartMovingStoppedOffenseSoldier                                                       
{
	criteria ConceptCartMovingStopped IsOnOffense IsSoldier SoldierNotSaidCartMovingStoppedO IsNotDisguised 75PercentChance                                                                                                                                                            
	ApplyContext "SaidCartMovingStoppedO:1:20"
	Response CartMovingStoppedOffenseSoldier                                                       
}
//--------------------------------------------------------------------------------------------------------------
// END OF Auto Speech Cart
//--------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------
// Begin Competitive Mode VO
//--------------------------------------------------------------------------------------------------------------
Response PlayerFirstRoundStartCompSoldier
{
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_09.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_10.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_11.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_12.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_13.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_comp_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_comp_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_comp_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_comp_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_comp_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_comp_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_rare_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_rare_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_rare_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_rare_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_rare_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamefirst_rare_06.vcd" predelay "1.0, 5.0"
}
Rule PlayerFirstRoundStartCompSoldier
{
	criteria ConceptPlayerRoundStartComp IsSoldier IsFirstRound 40PercentChance
	Response PlayerFirstRoundStartCompSoldier
}

Response PlayerWonPrevRoundCompSoldier
{
	scene "scenes/Player/Soldier/low/cm_soldier_pregamewonlast_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamewonlast_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamewonlast_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamewonlast_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamewonlast_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamewonlast_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamewonlast_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamewonlast_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamewonlast_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamewonlast_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamewonlast_rare_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamewonlast_rare_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamewonlast_rare_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamewonlast_rare_04.vcd" predelay "1.0, 5.0"
}
Rule PlayerWonPrevRoundCompSoldier
{
	criteria ConceptPlayerRoundStartComp IsSoldier IsNotFirstRound PlayerWonPreviousRound 40PercentChance
	Response PlayerWonPrevRoundCompSoldier
}

Response PlayerLostPrevRoundCompSoldier
{
	scene "scenes/Player/Soldier/low/cm_soldier_pregamelostlast_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamelostlast_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamelostlast_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamelostlast_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamelostlast_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamelostlast_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamelostlast_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamelostlast_09.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamelostlast_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamelostlast_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamelostlast_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamelostlast_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamelostlast_rare_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamelostlast_rare_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamelostlast_rare_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamelostlast_rare_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregamelostlast_rare_05.vcd" predelay "1.0, 5.0"
}
Rule PlayerLostPrevRoundCompSoldier
{
	criteria ConceptPlayerRoundStartComp IsSoldier IsNotFirstRound PlayerLostPreviousRound PreviousRoundWasNotTie 40PercentChance
	Response PlayerLostPrevRoundCompSoldier
}

Response PlayerTiedPrevRoundCompSoldier
{
	scene "scenes/Player/Soldier/low/cm_soldier_pregametie_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregametie_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregametie_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregametie_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregametie_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_pregametie_06.vcd" predelay "1.0, 5.0"
}
Rule PlayerTiedPrevRoundCompSoldier
{
	criteria ConceptPlayerRoundStartComp IsSoldier IsNotFirstRound PreviousRoundWasTie 40PercentChance
	Response PlayerTiedPrevRoundCompSoldier
}

Response PlayerGameWinCompSoldier
{
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_03.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_04.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_05.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_06.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_07.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_08.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_03.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_04.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_05.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_06.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_07.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_08.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_rare_09.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_rare_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_rare_02.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_rare_03.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_rare_04.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_rare_05.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_rare_06.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_rare_07.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Soldier/low/cm_soldier_gamewon_rare_08.vcd" predelay "2.0, 5.0"
}
Rule PlayerGameWinCompSoldier
{
	criteria ConceptPlayerGameOverComp PlayerOnWinningTeam IsSoldier 40PercentChance
	Response PlayerGameWinCompSoldier
}

Response PlayerMatchWinCompSoldier
{
	scene "scenes/Player/Soldier/low/cm_soldier_matchwon_01.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Soldier/low/cm_soldier_matchwon_02.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Soldier/low/cm_soldier_matchwon_03.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Soldier/low/cm_soldier_matchwon_04.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Soldier/low/cm_soldier_matchwon_05.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Soldier/low/cm_soldier_matchwon_06.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Soldier/low/cm_soldier_matchwon_07.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Soldier/low/cm_soldier_matchwon_08.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Soldier/low/cm_soldier_matchwon_09.vcd" predelay "1.0, 2.0"
}
Rule PlayerMatchWinCompSoldier
{
	criteria ConceptPlayerMatchOverComp PlayerOnWinningTeam IsSoldier 40PercentChance
	Response PlayerMatchWinCompSoldier
}
//--------------------------------------------------------------------------------------------------------------
// End Competitive Mode VO
//--------------------------------------------------------------------------------------------------------------