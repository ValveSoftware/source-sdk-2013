//--------------------------------------------------------------------------------------------------------------
// Pyro Response Rule File
//--------------------------------------------------------------------------------------------------------------

Criterion "PyroIsNotStillonFire" "PyroOnFire" "!=1" "required" weight 0
Criterion "PyroIsStillonFire" "PyroOnFire" "1" "required" weight 0
Criterion "PyroNotKillSpeech" "PyroKillSpeech" "!=1" "required" weight 0
Criterion "PyroNotKillSpeechMelee" "PyroKillSpeechMelee" "!=1" "required" weight 0
Criterion "PyroNotSaidHealThanks" "PyroSaidHealThanks" "!=1" "required"
Criterion "IsHelpCapPyro" "PyroHelpCap" "1" "required" weight 0
// Custom stuff
Criterion "PyroNotAssistSpeech" "PyroAssistSpeech" "!=1" "required" weight 0
Criterion "PyroNotInvulnerableSpeech" "PyroInvulnerableSpeech" "!=1" "required" weight 0
Criterion "PyroNotKillSpeechSapper" "PyroKillSpeechSapper" "!=1" "required" weight 0


Response PlayerCloakedSpyDemomanPyro
{
	scene "scenes/Player/Pyro/low/1440.vcd" 
}
Rule PlayerCloakedSpyDemomanPyro
{
	criteria ConceptPlayerCloakedSpy IsPyro IsOnDemoman
	Response PlayerCloakedSpyDemomanPyro
}

Response PlayerCloakedSpyEngineerPyro
{
	scene "scenes/Player/Pyro/low/1446.vcd" 
}
Rule PlayerCloakedSpyEngineerPyro
{
	criteria ConceptPlayerCloakedSpy IsPyro IsOnEngineer
	Response PlayerCloakedSpyEngineerPyro
}

Response PlayerCloakedSpyHeavyPyro
{
	scene "scenes/Player/Pyro/low/1436.vcd" 
}
Rule PlayerCloakedSpyHeavyPyro
{
	criteria ConceptPlayerCloakedSpy IsPyro IsOnHeavy
	Response PlayerCloakedSpyHeavyPyro
}

Response PlayerCloakedSpyMedicPyro
{
	scene "scenes/Player/Pyro/low/1444.vcd" 
}
Rule PlayerCloakedSpyMedicPyro
{
	criteria ConceptPlayerCloakedSpy IsPyro IsOnMedic
	Response PlayerCloakedSpyMedicPyro
}

Response PlayerCloakedSpyPyroPyro
{
	scene "scenes/Player/Pyro/low/1438.vcd" 
}
Rule PlayerCloakedSpyPyroPyro
{
	criteria ConceptPlayerCloakedSpy IsPyro IsOnPyro
	Response PlayerCloakedSpyPyroPyro
}

Response PlayerCloakedSpyScoutPyro
{
	scene "scenes/Player/Pyro/low/1432.vcd" 
}
Rule PlayerCloakedSpyScoutPyro
{
	criteria ConceptPlayerCloakedSpy IsPyro IsOnScout
	Response PlayerCloakedSpyScoutPyro
}

Response PlayerCloakedSpySniperPyro
{
	scene "scenes/Player/Pyro/low/1448.vcd" 
}
Rule PlayerCloakedSpySniperPyro
{
	criteria ConceptPlayerCloakedSpy IsPyro IsOnSniper
	Response PlayerCloakedSpySniperPyro
}

Response PlayerCloakedSpySpyPyro
{
	scene "scenes/Player/Pyro/low/1442.vcd" 
}
Rule PlayerCloakedSpySpyPyro
{
	criteria ConceptPlayerCloakedSpy IsPyro IsOnSpy
	Response PlayerCloakedSpySpyPyro
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech
//--------------------------------------------------------------------------------------------------------------
Response HealThanksPyro
{
	scene "scenes/Player/Pyro/low/1552.vcd" 
}
Rule HealThanksPyro
{
	criteria ConceptMedicChargeStopped IsPyro SuperHighHealthContext PyroNotSaidHealThanks 50PercentChance
	ApplyContext "PyroSaidHealThanks:1:20"
	Response HealThanksPyro
}

Response PlayerRoundStartPyro
{
	scene "scenes/Player/Pyro/low/1418.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Pyro/low/1419.vcd" predelay "1.0, 5.0"
}
Rule PlayerRoundStartPyro
{
	criteria ConceptPlayerRoundStart IsPyro
	Response PlayerRoundStartPyro
}

Response PlayerCappedIntelligencePyro
{
	scene "scenes/Player/Pyro/low/1409.vcd" 
}
Rule PlayerCappedIntelligencePyro
{
	criteria ConceptPlayerCapturedIntelligence IsPyro
	Response PlayerCappedIntelligencePyro
}

Response PlayerCapturedPointPyro
{
	scene "scenes/Player/Pyro/low/1406.vcd" 
}
Rule PlayerCapturedPointPyro
{
	criteria ConceptPlayerCapturedPoint IsPyro
	Response PlayerCapturedPointPyro
}

Response PlayerSuddenDeathPyro
{
	scene "scenes/Player/Pyro/low/1476.vcd" 
	scene "scenes/Player/Pyro/low/1477.vcd" 
}
Rule PlayerSuddenDeathPyro
{
	criteria ConceptPlayerSuddenDeathStart IsPyro
	Response PlayerSuddenDeathPyro
}

Response PlayerStalematePyro
{
	scene "scenes/Player/Pyro/low/1412.vcd" 
}
Rule PlayerStalematePyro
{
	criteria ConceptPlayerStalemate IsPyro
	Response PlayerStalematePyro
}

Response PlayerTeleporterThanksPyro
{
	scene "scenes/Player/Pyro/low/1555.vcd" 
}
Rule PlayerTeleporterThanksPyro
{
	criteria ConceptTeleported IsNotEngineer IsPyro 30PercentChance
	Response PlayerTeleporterThanksPyro
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Combat
//--------------------------------------------------------------------------------------------------------------
Response DefendOnThePointPyro
{
	scene "scenes/Player/Pyro/low/1531.vcd" 
}
Rule DefendOnThePointPyro
{
	criteria ConceptFireWeapon IsPyro IsOnFriendlyControlPoint NotDefendOnThePointSpeech
	ApplyContext "DefendOnThePointSpeech:1:30"
	applycontexttoworld
	Response DefendOnThePointPyro
}

Response KilledPlayerManyPyro
{
	scene "scenes/Player/Pyro/low/1534.vcd" 
}
Rule KilledPlayerManyPyro
{
	criteria ConceptKilledPlayer IsManyRecentKills 30PercentChance IsWeaponPrimary KilledPlayerDelay PyroNotKillSpeech IsPyro
	ApplyContext "PyroKillSpeech:1:10"
	Response KilledPlayerManyPyro
}

// Added back unused melee kill lines
Response KilledPlayerMeleePyro
{
	scene "scenes/Player/Pyro/low/1594.vcd" 
	scene "scenes/Player/Pyro/low/1532.vcd" 
	scene "scenes/Player/Pyro/low/1533.vcd" 
}
Rule KilledPlayerMeleePyro
{
	criteria ConceptKilledPlayer KilledPlayerDelay 30PercentChance  IsWeaponMelee PyroNotKillSpeechMelee IsPyro
	ApplyContext "PyroKillSpeechMelee:1:10"
	Response KilledPlayerMeleePyro
}

// Custom stuff
Response KilledPlayerAssistAutoPyro
{
	scene "scenes/Player/Pyro/low/1529.vcd" predelay "2.5"
}
Rule KilledPlayerAssistAutoPyro
{
	criteria ConceptKilledPlayer IsPyro IsBeingHealed IsManyRecentKills KilledPlayerDelay 20PercentChance PyroNotAssistSpeech
	ApplyContext "PyroAssistSpeech:1:20"
	Response KilledPlayerAssistAutoPyro
}

Response PyroJarateHit
{
	scene "scenes/Player/Pyro/low/1412.vcd"
	scene "scenes/Player/Pyro/low/1416.vcd"
}
Rule PyroJarateHit
{
	criteria ConceptJarateHit IsPyro 50PercentChance
	Response PyroJarateHit
}

Response InvulnerableSpeechPyro
{
	scene "scenes/Player/Pyro/low/1517.vcd" 
	scene "scenes/Player/Pyro/low/1485.vcd" 
}
Rule InvulnerableSpeechPyro
{
	criteria ConceptFireWeapon IsPyro IsInvulnerable PyroNotInvulnerableSpeech
	ApplyContext "PyroInvulnerableSpeech:1:30"
	Response InvulnerableSpeechPyro
}
// End custom

Response PlayerKilledCapperPyro
{
	scene "scenes/Player/Pyro/low/1421.vcd" 
}
Rule PlayerKilledCapperPyro
{
	criteria ConceptCapBlocked IsPyro
	ApplyContext "PyroKillSpeech:1:10"
	Response PlayerKilledCapperPyro
}

Response PlayerKilledDominatingPyro
{
	scene "scenes/Player/Pyro/low/1450.vcd" predelay "2.5"
	scene "scenes/Player/Pyro/low/1482.vcd" predelay "2.5"
	scene "scenes/Player/Pyro/low/1483.vcd" predelay "2.5"
	scene "scenes/Player/Pyro/low/1485.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingPyro
{
	criteria ConceptKilledPlayer IsPyro IsDominated
	ApplyContext "PyroKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingPyro
}

Response PlayerKilledForRevengePyro
{
	scene "scenes/Player/Pyro/low/1403.vcd" predelay "2.5"
	scene "scenes/Player/Pyro/low/1418.vcd" predelay "2.5"
}
Rule PlayerKilledForRevengePyro
{
	criteria ConceptKilledPlayer IsPyro IsRevenge
	ApplyContext "PyroKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledForRevengePyro
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Pain
//--------------------------------------------------------------------------------------------------------------
Response PlayerAttackerPainPyro
{
	scene "scenes/Player/Pyro/low/1581.vcd" 
	scene "scenes/Player/Pyro/low/1582.vcd" 
	scene "scenes/Player/Pyro/low/1583.vcd" 
	scene "scenes/Player/Pyro/low/1591.vcd" 
	scene "scenes/Player/Pyro/low/1592.vcd" 
	scene "scenes/Player/Pyro/low/1593.vcd" 
}
Rule PlayerAttackerPainPyro
{
	criteria ConceptAttackerPain IsPyro IsNotDominating
	Response PlayerAttackerPainPyro
}

Response PlayerOnFirePyro
{
	scene "scenes/Player/Pyro/low/1415.vcd" 
	scene "scenes/Player/Pyro/low/1416.vcd" 
}
Rule PlayerOnFirePyro
{
	criteria ConceptFire IsPyro PyroIsNotStillonFire IsNotDominating
	ApplyContext "PyroOnFire:1:7"
	Response PlayerOnFirePyro
}

Response PlayerPainPyro
{
	scene "scenes/Player/Pyro/low/1584.vcd" 
	scene "scenes/Player/Pyro/low/1585.vcd" 
	scene "scenes/Player/Pyro/low/1586.vcd" 
	scene "scenes/Player/Pyro/low/1587.vcd" 
	scene "scenes/Player/Pyro/low/1588.vcd" 
	scene "scenes/Player/Pyro/low/1589.vcd" 
	scene "scenes/Player/Pyro/low/1590.vcd" 
}
Rule PlayerPainPyro
{
	criteria ConceptPain IsPyro IsNotDominating
	Response PlayerPainPyro
}

Response PlayerStillOnFirePyro
{
	scene "scenes/Player/Pyro/low/1930.vcd" 
}
Rule PlayerStillOnFirePyro
{
	criteria ConceptFire IsPyro  PyroIsStillonFire IsNotDominating
	ApplyContext "PyroOnFire:1:7"
	Response PlayerStillOnFirePyro
}


//--------------------------------------------------------------------------------------------------------------
// Duel Speech
//--------------------------------------------------------------------------------------------------------------
Response AcceptedDuelPyro
{
	scene "scenes/Player/Pyro/low/1415.vcd" 
	scene "scenes/Player/Pyro/low/1534.vcd" 
	scene "scenes/Player/Pyro/low/1550.vcd" 
}
Rule AcceptedDuelPyro
{
	criteria ConceptIAcceptDuel IsPyro
	Response AcceptedDuelPyro
}

Response MeleeDarePyro
{
	scene "scenes/Player/Pyro/low/1409.vcd" 
	scene "scenes/Player/Pyro/low/1499.vcd" 
	scene "scenes/Player/Pyro/low/1529.vcd" 
	scene "scenes/Player/Pyro/low/1552.vcd" 
	scene "scenes/Player/Pyro/low/1555.vcd" 
}
Rule MeleeDarePyro
{
	criteria ConceptRequestDuel IsPyro
	Response MeleeDarePyro
}

Response RejectedDuelPyro
{
	scene "scenes/Player/Pyro/low/1412.vcd" 
	scene "scenes/Player/Pyro/low/1419.vcd" 
	scene "scenes/Player/Pyro/low/1432.vcd" 
	scene "scenes/Player/Pyro/low/1466.vcd" 
	scene "scenes/Player/Pyro/low/1469.vcd" 
}
Rule RejectedDuelPyro
{
	criteria ConceptDuelRejected IsPyro
	Response RejectedDuelPyro
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 1
//--------------------------------------------------------------------------------------------------------------
Response PlayerGoPyro
{
	scene "scenes/Player/Pyro/low/1451.vcd" 
}
Rule PlayerGoPyro
{
	criteria ConceptPlayerGo IsPyro
	Response PlayerGoPyro
}

Response PlayerHeadLeftPyro
{
	scene "scenes/Player/Pyro/low/1457.vcd" 
}
Rule PlayerHeadLeftPyro
{
	criteria ConceptPlayerLeft  IsPyro
	Response PlayerHeadLeftPyro
}

Response PlayerHeadRightPyro
{
	scene "scenes/Player/Pyro/low/1460.vcd" 
}
Rule PlayerHeadRightPyro
{
	criteria ConceptPlayerRight  IsPyro
	Response PlayerHeadRightPyro
}

Response PlayerHelpPyro
{
	scene "scenes/Player/Pyro/low/1463.vcd" 
}
Rule PlayerHelpPyro
{
	criteria ConceptPlayerHelp IsPyro
	Response PlayerHelpPyro
}

Response PlayerHelpCapturePyro
{
	scene "scenes/Player/Pyro/low/1466.vcd" 
}
Rule PlayerHelpCapturePyro
{
	criteria ConceptPlayerHelp IsPyro IsOnCappableControlPoint
	ApplyContext "PyroHelpCap:1:10"
	Response PlayerHelpCapturePyro
}

Response PlayerHelpCapture2Pyro
{
	scene "scenes/Player/Pyro/low/1531.vcd" 
}
Rule PlayerHelpCapture2Pyro
{
	criteria ConceptPlayerHelp IsPyro IsOnCappableControlPoint IsHelpCapPyro
	Response PlayerHelpCapture2Pyro
}

Response PlayerHelpDefendPyro
{
	scene "scenes/Player/Pyro/low/1469.vcd" 
}
Rule PlayerHelpDefendPyro
{
	criteria ConceptPlayerHelp IsPyro IsOnFriendlyControlPoint
	Response PlayerHelpDefendPyro
}

Response PlayerMedicPyro
{
	scene "scenes/Player/Pyro/low/1489.vcd" 
}
Rule PlayerMedicPyro
{
	criteria ConceptPlayerMedic IsPyro
	Response PlayerMedicPyro
}

Response PlayerAskForBallPyro
{
}
Rule PlayerAskForBallPyro
{
	criteria ConceptPlayerAskForBall IsPyro
	Response PlayerAskForBallPyro
}

Response PlayerMoveUpPyro
{
	scene "scenes/Player/Pyro/low/1492.vcd" 
}
Rule PlayerMoveUpPyro
{
	criteria ConceptPlayerMoveUp  IsPyro
	Response PlayerMoveUpPyro
}

Response PlayerNoPyro
{
	scene "scenes/Player/Pyro/low/1507.vcd" 
}
Rule PlayerNoPyro
{
	criteria ConceptPlayerNo  IsPyro
	Response PlayerNoPyro
}

Response PlayerThanksPyro
{
	scene "scenes/Player/Pyro/low/1550.vcd" 
}
Rule PlayerThanksPyro
{
	criteria ConceptPlayerThanks IsPyro
	Response PlayerThanksPyro
}

// Custom Assist kill response
// As there is no actual concept for assist kills, this is the second best method.
// Say thanks after you kill more than one person.

Response KilledPlayerAssistPyro
{
	scene "scenes/Player/Pyro/low/1529.vcd"
}
Rule KilledPlayerAssistPyro
{
	criteria ConceptPlayerThanks IsPyro IsARecentKill KilledPlayerDelay PyroNotAssistSpeech
	ApplyContext "PyroAssistSpeech:1:20"
	Response KilledPlayerAssistPyro
}
// End custom

Response PlayerYesPyro
{
	scene "scenes/Player/Pyro/low/1558.vcd" 
}
Rule PlayerYesPyro
{
	criteria ConceptPlayerYes  IsPyro
	Response PlayerYesPyro
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 2
//--------------------------------------------------------------------------------------------------------------
Response PlayerActivateChargePyro
{
	scene "scenes/Player/Pyro/low/1403.vcd" 
}
Rule PlayerActivateChargePyro
{
	criteria ConceptPlayerActivateCharge IsPyro
	Response PlayerActivateChargePyro
}

Response PlayerCloakedSpyPyro
{
	scene "scenes/Player/Pyro/low/1429.vcd" 
}
Rule PlayerCloakedSpyPyro
{
	criteria ConceptPlayerCloakedSpy IsPyro
	Response PlayerCloakedSpyPyro
}

Response PlayerDispenserHerePyro
{
	scene "scenes/Player/Pyro/low/1493.vcd" 
}
Rule PlayerDispenserHerePyro
{
	criteria ConceptPlayerDispenserHere IsPyro
	Response PlayerDispenserHerePyro
}

Response PlayerIncomingPyro
{
	scene "scenes/Player/Pyro/low/1472.vcd" 
}
Rule PlayerIncomingPyro
{
	criteria ConceptPlayerIncoming IsPyro
	Response PlayerIncomingPyro
}

Response PlayerSentryAheadPyro
{
	scene "scenes/Player/Pyro/low/1515.vcd" 
}
Rule PlayerSentryAheadPyro
{
	criteria ConceptPlayerSentryAhead IsPyro
	Response PlayerSentryAheadPyro
}

Response PlayerSentryHerePyro
{
	scene "scenes/Player/Pyro/low/1495.vcd" 
}
Rule PlayerSentryHerePyro
{
	criteria ConceptPlayerSentryHere IsPyro
	Response PlayerSentryHerePyro
}

Response PlayerTeleporterHerePyro
{
	scene "scenes/Player/Pyro/low/1497.vcd" 
}
Rule PlayerTeleporterHerePyro
{
	criteria ConceptPlayerTeleporterHere IsPyro
	Response PlayerTeleporterHerePyro
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 3
//--------------------------------------------------------------------------------------------------------------
Response PlayerBattleCryPyro
{
	scene "scenes/Player/Pyro/low/1418.vcd" 
	scene "scenes/Player/Pyro/low/1419.vcd" 
}
Rule PlayerBattleCryPyro
{
	criteria ConceptPlayerBattleCry IsPyro
	Response PlayerBattleCryPyro
}

// Custom stuff - melee dare
// Look at enemy, then do battle cry voice command while holding a melee weapon.
Response MeleeDareCombatPyro
{
	scene "scenes/Player/Pyro/low/1409.vcd"
	scene "scenes/Player/Pyro/low/1517.vcd"
	scene "scenes/Player/Pyro/low/1482.vcd"
}
Rule MeleeDareCombatPyro
{
	criteria ConceptPlayerBattleCry IsWeaponMelee IsPyro IsCrosshairEnemy
	Response MeleeDareCombatPyro
}
//End custom

Response PlayerCheersPyro
{
	scene "scenes/Player/Pyro/low/1421.vcd" 
}
Rule PlayerCheersPyro
{
	criteria ConceptPlayerCheers IsPyro
	Response PlayerCheersPyro
}

Response PlayerGoodJobPyro
{
	scene "scenes/Player/Pyro/low/1454.vcd" 
}
Rule PlayerGoodJobPyro
{
	criteria ConceptPlayerGoodJob IsPyro
	Response PlayerGoodJobPyro
}

Response PlayerJeersPyro
{
	scene "scenes/Player/Pyro/low/1476.vcd" 
	scene "scenes/Player/Pyro/low/1477.vcd" 
}
Rule PlayerJeersPyro
{
	criteria ConceptPlayerJeers IsPyro
	Response PlayerJeersPyro
}

Response PlayerLostPointPyro
{
	scene "scenes/Player/Pyro/low/1499.vcd" 
}
Rule PlayerLostPointPyro
{
	criteria ConceptPlayerLostPoint IsPyro
	Response PlayerLostPointPyro
}

Response PlayerNegativePyro
{
	scene "scenes/Player/Pyro/low/1499.vcd" 
}
Rule PlayerNegativePyro
{
	criteria ConceptPlayerNegative IsPyro
	Response PlayerNegativePyro
}

Response PlayerNiceShotPyro
{
	scene "scenes/Player/Pyro/low/1504.vcd" 
}
Rule PlayerNiceShotPyro
{
	criteria ConceptPlayerNiceShot IsPyro
	Response PlayerNiceShotPyro
}

Response PlayerPositivePyro
{
	scene "scenes/Player/Pyro/low/1510.vcd" 
}

Response PlayerTauntsPyro
{
	scene "scenes/Player/Pyro/low/1563.vcd" 
	scene "scenes/Player/Pyro/low/1595.vcd" 
	scene "scenes/Player/Pyro/low/1487.vcd" 
}
Rule PlayerPositivePyro
{
	criteria ConceptPlayerPositive IsPyro
	Response PlayerPositivePyro
	Response PlayerTauntsPyro
}

//--------------------------------------------------------------------------------------------------------------
// Begin Competitive Mode VO
//--------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------
// End Competitive Mode VO
//--------------------------------------------------------------------------------------------------------------