//--------------------------------------------------------------------------------------------------------------
// Engineer Response Rule File
//--------------------------------------------------------------------------------------------------------------

Criterion "EngineerIsNotStillonFire" "EngineerOnFire" "!=1" "required" weight 0
Criterion "EngineerIsStillonFire" "EngineerOnFire" "1" "required" weight 0
Criterion "EngineerNotKillSpeech" "EngineerKillSpeech" "!=1" "required" weight 0
Criterion "EngineerNotKillSpeechMelee" "EngineerKillSpeechMelee" "!=1" "required" weight 0
Criterion "EngineerNotSaidHealThanks" "EngineerSaidHealThanks" "!=1" "required"
Criterion "IsHelpCapEngineer" "EngineerHelpCap" "1" "required" weight 0
// Custom stuff
Criterion "EngineerNotAssistSpeech" "EngineerAssistSpeech" "!=1" "required" weight 0
Criterion "IsEngyFistSwung" "EngyFistSwung" "1" "required" weight 0
Criterion "IsNotEngyFistSwung" "EngyFistSwung" "!=1" "required" weight 0
Criterion "EngineerNotInvulnerableSpeech" "EngineerInvulnerableSpeech" "!=1" "required" weight 0
Criterion "IsMiniSentryKill" "MiniSentryKill" "1" "required" weight 0
Criterion "IsSentryKill" "SentryKill" "1" "required" weight 0

Response PlayerCloakedSpyDemomanEngineer
{
	scene "scenes/Player/Engineer/low/56.vcd" 
}
Rule PlayerCloakedSpyDemomanEngineer
{
	criteria ConceptPlayerCloakedSpy IsEngineer IsOnDemoman
	Response PlayerCloakedSpyDemomanEngineer
}

Response PlayerCloakedSpyEngineerEngineer
{
	scene "scenes/Player/Engineer/low/62.vcd" 
}
Rule PlayerCloakedSpyEngineerEngineer
{
	criteria ConceptPlayerCloakedSpy IsEngineer IsOnEngineer
	Response PlayerCloakedSpyEngineerEngineer
}

Response PlayerCloakedSpyHeavyEngineer
{
	scene "scenes/Player/Engineer/low/52.vcd" 
}
Rule PlayerCloakedSpyHeavyEngineer
{
	criteria ConceptPlayerCloakedSpy IsEngineer IsOnHeavy
	Response PlayerCloakedSpyHeavyEngineer
}

Response PlayerCloakedSpyMedicEngineer
{
	scene "scenes/Player/Engineer/low/60.vcd" 
}
Rule PlayerCloakedSpyMedicEngineer
{
	criteria ConceptPlayerCloakedSpy IsEngineer IsOnMedic
	Response PlayerCloakedSpyMedicEngineer
}

Response PlayerCloakedSpyPyroEngineer
{
	scene "scenes/Player/Engineer/low/54.vcd" 
}
Rule PlayerCloakedSpyPyroEngineer
{
	criteria ConceptPlayerCloakedSpy IsEngineer IsOnPyro
	Response PlayerCloakedSpyPyroEngineer
}

Response PlayerCloakedSpyScoutEngineer
{
	scene "scenes/Player/Engineer/low/48.vcd" 
}
Rule PlayerCloakedSpyScoutEngineer
{
	criteria ConceptPlayerCloakedSpy IsEngineer IsOnScout
	Response PlayerCloakedSpyScoutEngineer
}

Response PlayerCloakedSpySniperEngineer
{
	scene "scenes/Player/Engineer/low/64.vcd" 
}
Rule PlayerCloakedSpySniperEngineer
{
	criteria ConceptPlayerCloakedSpy IsEngineer IsOnSniper
	Response PlayerCloakedSpySniperEngineer
}

Response PlayerCloakedSpySoldierEngineer
{
	scene "scenes/Player/Engineer/low/50.vcd" 
}
Rule PlayerCloakedSpySoldierEngineer
{
	criteria ConceptPlayerCloakedSpy IsEngineer IsOnSoldier
	Response PlayerCloakedSpySoldierEngineer
}

Response PlayerCloakedSpySpyEngineer
{
	scene "scenes/Player/Engineer/low/58.vcd" 
	scene "scenes/Player/Engineer/low/59.vcd" 
}
Rule PlayerCloakedSpySpyEngineer
{
	criteria ConceptPlayerCloakedSpy IsEngineer IsOnSpy
	Response PlayerCloakedSpySpyEngineer
}

Response PlayerDispenserDownEngineer
{
	scene "scenes/Player/Engineer/low/25.vcd" 
}
Rule PlayerDispenserDownEngineer
{
	criteria ConceptLostObject IsDispenser IsEngineer
	Response PlayerDispenserDownEngineer
}

Response PlayerSentryDownEngineer
{
	scene "scenes/Player/Engineer/low/26.vcd" 
}
Rule PlayerSentryDownEngineer
{
	criteria ConceptLostObject IsSentryGun IsEngineer
	Response PlayerSentryDownEngineer
}

Response PlayerTeleporterDownEngineer
{
	scene "scenes/Player/Engineer/low/27.vcd" 
}
Rule PlayerTeleporterDownEngineer
{
	criteria ConceptLostObject IsTeleporter IsEngineer
	Response PlayerTeleporterDownEngineer
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech
//--------------------------------------------------------------------------------------------------------------
Response HealThanksEngineer
{
	scene "scenes/Player/Engineer/low/181.vcd" 
	scene "scenes/Player/Engineer/low/183.vcd" 
}
Rule HealThanksEngineer
{
	criteria ConceptMedicChargeStopped IsEngineer SuperHighHealthContext EngineerNotSaidHealThanks 50PercentChance
	ApplyContext "EngineerSaidHealThanks:1:20"
	Response HealThanksEngineer
}

Response PlayerRoundStartEngineer
{
	scene "scenes/Player/Engineer/low/31.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/33.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/34.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/35.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/36.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/1329.vcd" predelay "1.0, 5.0"
}
Rule PlayerRoundStartEngineer
{
	criteria ConceptPlayerRoundStart IsEngineer
	Response PlayerRoundStartEngineer
}

Response PlayerCappedIntelligenceEngineer
{
	scene "scenes/Player/Engineer/low/19.vcd" 
	scene "scenes/Player/Engineer/low/21.vcd" 
	scene "scenes/Player/Engineer/low/20.vcd" 
}
Rule PlayerCappedIntelligenceEngineer
{
	criteria ConceptPlayerCapturedIntelligence IsEngineer
	Response PlayerCappedIntelligenceEngineer
}

Response PlayerCapturedPointEngineer
{
	scene "scenes/Player/Engineer/low/16.vcd" 
	scene "scenes/Player/Engineer/low/18.vcd" 
	scene "scenes/Player/Engineer/low/17.vcd" 
}
Rule PlayerCapturedPointEngineer
{
	criteria ConceptPlayerCapturedPoint IsEngineer
	Response PlayerCapturedPointEngineer
}

Response PlayerSuddenDeathEngineer
{
	scene "scenes/Player/Engineer/low/94.vcd" 
	scene "scenes/Player/Engineer/low/95.vcd" 
	scene "scenes/Player/Engineer/low/96.vcd" 
	scene "scenes/Player/Engineer/low/98.vcd" 
}
Rule PlayerSuddenDeathEngineer
{
	criteria ConceptPlayerSuddenDeathStart IsEngineer
	Response PlayerSuddenDeathEngineer
}

Response PlayerStalemateEngineer
{
	scene "scenes/Player/Engineer/low/22.vcd" 
	scene "scenes/Player/Engineer/low/23.vcd" 
	scene "scenes/Player/Engineer/low/24.vcd" 
}
Rule PlayerStalemateEngineer
{
	criteria ConceptPlayerStalemate IsEngineer
	Response PlayerStalemateEngineer
}

Response PlayerTeleporterThanksEngineer
{
	scene "scenes/Player/Engineer/low/186.vcd" 
	scene "scenes/Player/Engineer/low/184.vcd" 
}
Rule PlayerTeleporterThanksEngineer
{
	criteria ConceptTeleported IsEngineer 30PercentChance
	Response PlayerTeleporterThanksEngineer
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Combat
//--------------------------------------------------------------------------------------------------------------
Response DefendOnThePointEngineer
{
	scene "scenes/Player/Engineer/low/1344.vcd" 
	scene "scenes/Player/Engineer/low/161.vcd" 
}
Rule DefendOnThePointEngineer
{
	criteria ConceptFireWeapon IsEngineer IsOnFriendlyControlPoint NotDefendOnThePointSpeech
	ApplyContext "DefendOnThePointSpeech:1:30"
	applycontexttoworld
	Response DefendOnThePointEngineer
}

// Custom stuff
Response KilledPlayerAssistAutoEngineer
{
	scene "scenes/Player/Engineer/low/159.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/160.vcd" predelay "2.5"
}
Rule KilledPlayerAssistAutoEngineer
{
	criteria ConceptKilledPlayer IsEngineer IsBeingHealed IsARecentKill KilledPlayerDelay 20PercentChance EngineerNotAssistSpeech
	ApplyContext "EngineerAssistSpeech:1:20"
	Response KilledPlayerAssistAutoEngineer
}
// End custom

Response EngineerGoldenWrench
{
	scene "scenes/Player/Engineer/low/3605.vcd" predelay ".25"
	scene "scenes/Player/Engineer/low/3690.vcd" predelay ".25"
	scene "scenes/Player/Engineer/low/3691.vcd" predelay ".25"
	scene "scenes/Player/Engineer/low/3602.vcd" predelay ".25"
}
Rule EngineerGoldenWrench
{
	criteria ConceptKilledPlayer IsEngineer WeaponIsGoldenWrench WeaponIsWrench WeaponIsNotSentry EngineerNotKillSpeechMelee
	ApplyContext "EngineerKillSpeechMelee:1:10"
	Response EngineerGoldenWrench
}
Rule EngineerSaxxy
{
	criteria ConceptKilledPlayer IsEngineer WeaponIsSaxxy WeaponIsNotSentry EngineerNotKillSpeechMelee
	ApplyContext "EngineerKillSpeechMelee:1:10"
	Response EngineerGoldenWrench
}
Rule EngineerGoldenFryingPan
{
	criteria ConceptKilledPlayer IsEngineer WeaponIsGoldenFryingPan WeaponIsNotSentry EngineerNotKillSpeechMelee
	ApplyContext "EngineerKillSpeechMelee:1:10"
	Response EngineerGoldenWrench
}


Response EngineerLaserPointer
{
	scene "scenes/Player/Engineer/low/3603.vcd" predelay ".25"
	scene "scenes/Player/Engineer/low/3604.vcd" predelay ".25"
	scene "scenes/Player/Engineer/low/3700.vcd" predelay ".25"
	scene "scenes/Player/Engineer/low/3704.vcd" predelay ".25"
}
Rule EngineerLaserPointer
{
	criteria ConceptKilledPlayer IsEngineer WeaponIsLaserPointer 30PercentChance
	ApplyContext "EngineerKillSpeech:1:10"
	Response EngineerLaserPointer
}

Response KillTauntsEngineerMiniSentry
{
	scene "scenes/Player/Engineer/low/3705.vcd" predelay ".25"
	scene "scenes/Player/Engineer/low/3706.vcd" predelay ".25"
	scene "scenes/Player/Engineer/low/3707.vcd" predelay ".25"
}
Rule KillTauntsEngineerMiniSentry
{
	criteria ConceptKilledPlayer WeaponIsMiniSentrygun IsEngineer 20PercentChance
	ApplyContext "EngineerKillSpeech:1:10"
	Response KillTauntsEngineerMiniSentry
}

Response KillTauntsEngineerSpecial
{
	scene "scenes/Player/Engineer/low/148.vcd" 
	scene "scenes/Player/Engineer/low/153.vcd" 
	scene "scenes/Player/Engineer/low/156.vcd" 
	scene "scenes/Player/Engineer/low/157.vcd" 
}
Rule KillTauntsEngineerSpecial
{
	criteria ConceptKilledPlayer IsManyRecentKills WeaponIsSentrygun KilledPlayerDelay EngineerNotKillSpeech IsEngineer 30PercentChance
	ApplyContext "EngineerKillSpeech:1:10"
	Response KillTauntsEngineerSpecial
}

Response KilledPlayerManyEngineer
{
	scene "scenes/Player/Engineer/low/151.vcd" 
	scene "scenes/Player/Engineer/low/158.vcd" 
	scene "scenes/Player/Engineer/low/154.vcd" 
}
Rule KilledPlayerManyEngineer
{
	criteria ConceptKilledPlayer IsManyRecentKills 30PercentChance KilledPlayerDelay EngineerNotKillSpeech IsEngineer WeaponIsNotRobotArm
	ApplyContext "EngineerKillSpeech:1:10"
	Response KilledPlayerManyEngineer
}

Response KilledPlayerMeleeEngineerEngineer
{
	scene "scenes/Player/Engineer/low/155.vcd" 
}
Rule KilledPlayerMeleeEngineerEngineer
{
	criteria ConceptKilledPlayer KilledPlayerDelay 30PercentChance  IsWeaponMelee EngineerNotKillSpeechMelee IsEngineer WeaponIsNotSentry
	ApplyContext "EngineerKillSpeechMelee:1:10"
	Response KilledPlayerMeleeEngineerEngineer
}

// Custom stuff
// Rule for when your mini sentry is killed and you are holding your Frontier Justice
// Let's hope you have some crits ready, because this line is very fitting.

Response EngyCritsReady
{
	scene "scenes/player/Engineer/low/167.vcd"
	scene "scenes/player/Engineer/low/172.vcd"
	scene "scenes/player/Engineer/low/177.vcd"
}

Rule EngyCritsReady
{
	criteria ConceptLostObject IsSentryGun IsEngineer IsMiniSentryKill WeaponIsFrontierJustice EngineerNotKillSpeech
	ApplyContext "EngineerKillSpeech:1:10"
	Response EngyCritsReady
}

Rule EngyCritsReady2
{
	criteria ConceptLostObject IsSentryGun IsEngineer IsSentryKill WeaponIsFrontierJustice EngineerNotKillSpeech
	ApplyContext "EngineerKillSpeech:1:10"
	Response EngyCritsReady
}

Rule EngyCritsReadyFestive
{
	criteria ConceptLostObject IsSentryGun IsEngineer IsMiniSentryKill WeaponIsFestiveFrontierJustice EngineerNotKillSpeech
	ApplyContext "EngineerKillSpeech:1:10"
	Response EngyCritsReady
}

Rule EngyCritsReady2Festive
{
	criteria ConceptLostObject IsSentryGun IsEngineer IsSentryKill WeaponIsFestiveFrontierJustice EngineerNotKillSpeech
	ApplyContext "EngineerKillSpeech:1:10"
	Response EngyCritsReady
}


// Check if mini sentry has had a kill in the past thirty seconds
Rule MiniSentryKill
{
	criteria ConceptKilledPlayer WeaponIsMiniSentrygun IsEngineer
	ApplyContext "MiniSentryKill:1:15"
	Response PlayerExpressionAttackEngineer
}

// Check if sentry has had a kill in the past thirty seconds
Rule SentryKill
{
	criteria ConceptKilledPlayer WeaponIsSentrygun IsEngineer
	ApplyContext "MiniSentryKill:1:15"
	Response PlayerExpressionAttackEngineer
}

// Invulnerable responses
Response InvulnerableSpeechEngineer
{
	scene "scenes/Player/Engineer/low/176.vcd"  
	scene "scenes/Player/Engineer/low/177.vcd" 
	scene "scenes/Player/Engineer/low/1018.vcd" 
	scene "scenes/Player/Engineer/low/3619.vcd" 
}

Rule InvulnerableSpeechEngineer
{
	criteria ConceptFireWeapon IsEngineer IsInvulnerable EngineerNotInvulnerableSpeech
	ApplyContext "EngineerInvulnerableSpeech:1:30"
	Response InvulnerableSpeechEngineer
	Response EngyCritsReady
}

// End custom

Response KilledPlayerVeryManyEngineer
{
	scene "scenes/Player/Engineer/low/147.vcd" 
}
Rule KilledPlayerVeryManyEngineer
{
	criteria ConceptKilledPlayer IsVeryManyRecentKills 50PercentChance KilledPlayerDelay EngineerNotKillSpeech IsEngineer WeaponIsNotRobotArm
	ApplyContext "EngineerKillSpeech:1:10"
	Response KilledPlayerVeryManyEngineer
}

Response MedicFollowEngineer
{
	scene "scenes/Player/Engineer/low/3618.vcd" predelay ".25"
	scene "scenes/Player/Engineer/low/3693.vcd" predelay ".25"
	scene "scenes/Player/Engineer/low/3694.vcd" predelay ".25"
}
Rule MedicFollowEngineer
{
	criteria ConceptPlayerMedic IsOnMedic IsEngineer IsNotCrossHairEnemy NotLowHealth EngineerIsNotStillonFire
	ApplyContext "EngineerKillSpeech:1:10"
	Response MedicFollowEngineer
}

Response EngySwingFistStart
{
	scene "scenes/player/Engineer/low/3589.vcd"
	scene "scenes/player/Engineer/low/3590.vcd"
	scene "scenes/player/Engineer/low/3591.vcd"
}
Rule EngySwingFistStart
{
	criteria ConceptFireWeapon WeaponIsRobotArm IsEngineer EngineerNotKillSpeech IsNotDominating IsNotEngyFistSwung
	ApplyContext "EngyFistSwung:1:20" //every 20 seconds this line will fire
	Response EngySwingFistStart
}

Response EngySwingFist
{
	scene "scenes/player/Engineer/low/3592.vcd"
	scene "scenes/player/Engineer/low/3594.vcd"
	scene "scenes/player/Engineer/low/3703.vcd"
}
Rule EngySwingFist
{
	criteria ConceptKilledPlayer KilledPlayerDelay WeaponIsRobotArm IsEngyFistSwung IsEngineer WeaponIsNotMiniSentrygun EngineerNotKillSpeechMelee
	ApplyContext "EngineerKillSpeechMelee:1:20"
	Response EngySwingFist
}

// Custom stuff
Response EngineerJarateHit
{
	scene "scenes/Player/Engineer/low/22.vcd" 
	scene "scenes/Player/Engineer/low/23.vcd" 
	scene "scenes/Player/Engineer/low/150.vcd" 
}
Rule EngineerJarateHit
{
	criteria ConceptJarateHit IsEngineer 50PercentChance
	Response EngineerJarateHit
}
// End custom

Response PlayerKilledCapperEngineer
{
	scene "scenes/Player/Engineer/low/36.vcd" 
	scene "scenes/Player/Engineer/low/1330.vcd" 
	scene "scenes/Player/Engineer/low/42.vcd" 
	scene "scenes/Player/Engineer/low/43.vcd" 
	scene "scenes/Player/Engineer/low/41.vcd" 
}
Rule PlayerKilledCapperEngineer
{
	criteria ConceptCapBlocked IsEngineer
	ApplyContext "EngineerKillSpeech:1:10"
	Response PlayerKilledCapperEngineer
}

Response PlayerKilledDominatingDemomanEngineer
{
	scene "scenes/Player/Engineer/low/3581.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3617.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3678.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3679.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3680.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3681.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingDemomanEngineer
{
	criteria ConceptKilledPlayer IsEngineer IsDominated  IsVictimDemoman
	ApplyContext "EngineerKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingDemomanEngineer
}

Response PlayerKilledDominatingEngineerEngineer
{
	scene "scenes/Player/Engineer/low/3585.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3634.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3635.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3636.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3637.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3638.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3639.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3640.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3641.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingEngineerEngineer
{
	criteria ConceptKilledPlayer IsEngineer IsDominated  IsVictimEngineer
	ApplyContext "EngineerKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingEngineerEngineer
}

Response PlayerKilledDominatingHeavyEngineer
{
	scene "scenes/Player/Engineer/low/3584.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3609.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3610.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3642.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3645.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3643.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3644.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3647.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3648.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3649.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3650.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3682.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3683.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3684.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3702.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingHeavyEngineer
{
	criteria ConceptKilledPlayer IsEngineer IsDominated  IsVictimHeavy
	ApplyContext "EngineerKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingHeavyEngineer
}

Response PlayerKilledDominatingMedicEngineer
{
	scene "scenes/Player/Engineer/low/3582.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3613.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3614.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3669.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3670.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3671.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3672.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3673.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingMedicEngineer
{
	criteria ConceptKilledPlayer IsEngineer IsDominated  IsVictimMedic
	ApplyContext "EngineerKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingMedicEngineer
}

Response PlayerKilledDominatingPyroEngineer
{
	scene "scenes/Player/Engineer/low/3583.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3612.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3661.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3662.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3664.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3665.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3666.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3668.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3667.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingPyroEngineer
{
	criteria ConceptKilledPlayer IsEngineer IsDominated  IsVictimPyro
	ApplyContext "EngineerKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingPyroEngineer
}

Response PlayerKilledDominatingScoutEngineer
{
	scene "scenes/Player/Engineer/low/3588.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3606.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3616.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3621.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3622.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3623.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3624.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3625.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3626.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3627.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3628.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3701.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingScoutEngineer
{
	criteria ConceptKilledPlayer IsEngineer IsDominated  IsVictimScout
	ApplyContext "EngineerKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingScoutEngineer
}

Response PlayerKilledDominatingSniperEngineer
{
	scene "scenes/Player/Engineer/low/3587.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3607.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3629.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3630.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3631.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3632.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3633.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3685.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingSniperEngineer
{
	criteria ConceptKilledPlayer IsEngineer IsDominated  IsVictimSniper
	ApplyContext "EngineerKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingSniperEngineer
}

Response PlayerKilledDominatingSoldierEngineer
{
	scene "scenes/Player/Engineer/low/3580.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3615.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3674.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3675.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3676.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3686.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3687.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3688.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingSoldierEngineer
{
	criteria ConceptKilledPlayer IsEngineer IsDominated  IsVictimSoldier
	ApplyContext "EngineerKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingSoldierEngineer
}

Response PlayerKilledDominatingSpyEngineer
{
	scene "scenes/Player/Engineer/low/3586.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3611.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3651.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3652.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3653.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3654.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3655.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3656.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3657.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3659.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3658.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3689.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3660.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingSpyEngineer
{
	criteria ConceptKilledPlayer IsEngineer IsDominated  IsVictimSpy
	ApplyContext "EngineerKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingSpyEngineer
}

Response PlayerKilledForRevengeEngineer
{
	scene "scenes/Player/Engineer/low/3579.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/1329.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/40.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/39.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/44.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/98.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/100.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/1331.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/1335.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/1336.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/101.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/1334.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/102.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/103.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/149.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/164.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/168.vcd" predelay "2.5"
	scene "scenes/Player/Engineer/low/3696.vcd" predelay "2.5"
}
Rule PlayerKilledForRevengeEngineer
{
	criteria ConceptKilledPlayer IsEngineer IsRevenge
	ApplyContext "EngineerKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledForRevengeEngineer
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Engineer
//--------------------------------------------------------------------------------------------------------------
Response CarrySentryEngineer
{
	scene "scenes/Player/Engineer/low/3599.vcd" 
	scene "scenes/Player/Engineer/low/3600.vcd" 
}
Rule CarrySentryEngineer
{
	criteria ConceptEngineerCarryingBuilding IsEngineer 20PercentChance
	Response CarrySentryEngineer
}

Response DeploySentryEngineer
{
	scene "scenes/Player/Engineer/low/3697.vcd" 
	scene "scenes/Player/Engineer/low/3601.vcd" 
	scene "scenes/Player/Engineer/low/3699.vcd" 
	scene "scenes/Player/Engineer/low/3698.vcd" 
}
Rule DeploySentryEngineer
{
	criteria ConceptEngineerDeployBuilding IsEngineer
	Response DeploySentryEngineer
}

Response PickupSentryEngineer
{
	scene "scenes/Player/Engineer/low/3595.vcd" 
	scene "scenes/Player/Engineer/low/3597.vcd" 
	scene "scenes/Player/Engineer/low/3596.vcd" 
}
Rule PickupSentryEngineer
{
	criteria ConceptEngineerPickupBuilding IsEngineer
	Response PickupSentryEngineer
}

Response PlayerBuildingDispenserEngineer
{
	scene "scenes/Player/Engineer/low/8.vcd" 
	scene "scenes/Player/Engineer/low/9.vcd" 
}
Rule PlayerBuildingDispenserEngineer
{
	criteria ConceptPlayerBuildingObject IsDispenser IsEngineer
	Response PlayerBuildingDispenserEngineer
}

Response PlayerBuildingSentryEngineer
{
	scene "scenes/Player/Engineer/low/12.vcd" 
	scene "scenes/Player/Engineer/low/11.vcd" 
}
Rule PlayerBuildingSentryEngineer
{
	criteria ConceptPlayerBuildingObject IsSentryGun IsEngineer
	Response PlayerBuildingSentryEngineer
}

Response PlayerBuildingTeleporterEngineer
{
	scene "scenes/Player/Engineer/low/13.vcd" 
	scene "scenes/Player/Engineer/low/15.vcd" 
}
Rule PlayerBuildingTeleporterEngineer
{
	criteria ConceptPlayerBuildingObject IsTeleporter IsEngineer
	Response PlayerBuildingTeleporterEngineer
}


Response PlayerDispenserSappedEngineer
{
	scene "scenes/Player/Engineer/low/5.vcd" 
}
Rule PlayerDispenserSappedEngineer
{
	criteria ConceptSpySapping IsEngineer IsDispenser
	Response PlayerDispenserSappedEngineer
}

Response PlayerSentrySappedEngineer
{
	scene "scenes/Player/Engineer/low/4.vcd" 
}
Rule PlayerSentrySappedEngineer
{
	criteria ConceptSpySapping IsEngineer IsSentryGun
	Response PlayerSentrySappedEngineer
}

Response PlayerTeleporterSappedEngineer
{
	scene "scenes/Player/Engineer/low/6.vcd" 
}
Rule PlayerTeleporterSappedEngineer
{
	criteria ConceptSpySapping IsEngineer IsTeleporter
	Response PlayerTeleporterSappedEngineer
}



//--------------------------------------------------------------------------------------------------------------
// Auto Speech Pain
//--------------------------------------------------------------------------------------------------------------
Response PlayerAttackerPainEngineer
{
	scene "scenes/Player/Engineer/low/133.vcd" 
	scene "scenes/Player/Engineer/low/134.vcd" 
	scene "scenes/Player/Engineer/low/135.vcd" 
	scene "scenes/Player/Engineer/low/1254.vcd" 
	scene "scenes/Player/Engineer/low/1255.vcd" 
	scene "scenes/Player/Engineer/low/1256.vcd" 
	scene "scenes/Player/Engineer/low/1257.vcd" 
}
Rule PlayerAttackerPainEngineer
{
	criteria ConceptAttackerPain IsEngineer IsNotDominating
	Response PlayerAttackerPainEngineer
}

Response PlayerOnFireEngineer
{
	scene "scenes/Player/Engineer/low/28.vcd" 
	scene "scenes/Player/Engineer/low/30.vcd" 
	scene "scenes/Player/Engineer/low/29.vcd" 
}
Rule PlayerOnFireEngineer
{
	criteria ConceptFire IsEngineer EngineerIsNotStillonFire IsNotDominating
	ApplyContext "EngineerOnFire:1:7"
	Response PlayerOnFireEngineer
}

Response PlayerPainEngineer
{
	scene "scenes/Player/Engineer/low/136.vcd" 
	scene "scenes/Player/Engineer/low/137.vcd" 
	scene "scenes/Player/Engineer/low/138.vcd" 
	scene "scenes/Player/Engineer/low/1249.vcd" 
	scene "scenes/Player/Engineer/low/1250.vcd" 
	scene "scenes/Player/Engineer/low/1251.vcd" 
	scene "scenes/Player/Engineer/low/1252.vcd" 
	scene "scenes/Player/Engineer/low/1253.vcd" 
}
Rule PlayerPainEngineer
{
	criteria ConceptPain IsEngineer IsNotDominating
	Response PlayerPainEngineer
}

Response PlayerStillOnFireEngineer
{
	scene "scenes/Player/Engineer/low/1931.vcd" 
}
Rule PlayerStillOnFireEngineer
{
	criteria ConceptFire IsEngineer  EngineerIsStillonFire IsNotDominating
	ApplyContext "EngineerOnFire:1:7"
	Response PlayerStillOnFireEngineer
}


//--------------------------------------------------------------------------------------------------------------
// Duel Speech
//--------------------------------------------------------------------------------------------------------------
Response AcceptedDuelEngineer
{
	scene "scenes/Player/Engineer/low/35.vcd" 
	scene "scenes/Player/Engineer/low/163.vcd" 
	scene "scenes/Player/Engineer/low/166.vcd" 
	scene "scenes/Player/Engineer/low/169.vcd" 
	scene "scenes/Player/Engineer/low/172.vcd" 
	scene "scenes/Player/Engineer/low/174.vcd" 
	scene "scenes/Player/Engineer/low/188.vcd" 
}
Rule AcceptedDuelEngineer
{
	criteria ConceptIAcceptDuel IsEngineer
	Response AcceptedDuelEngineer
}

Response MeleeDareEngineer
{
	scene "scenes/Player/Engineer/low/3619.vcd" 
	scene "scenes/Player/Engineer/low/3620.vcd" 
	scene "scenes/Player/Engineer/low/3695.vcd" 
}
Rule MeleeDareEngineer
{
	criteria ConceptRequestDuel IsEngineer
	Response MeleeDareEngineer
}

Response RejectedDuelEngineer
{
	scene "scenes/Player/Engineer/low/24.vcd" 
	scene "scenes/Player/Engineer/low/94.vcd" 
	scene "scenes/Player/Engineer/low/3615.vcd" 
	scene "scenes/Player/Engineer/low/3658.vcd" 
}
Rule RejectedDuelEngineer
{
	criteria ConceptDuelRejected IsEngineer
	Response RejectedDuelEngineer
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 1
//--------------------------------------------------------------------------------------------------------------
Response PlayerGoEngineer
{
	scene "scenes/Player/Engineer/low/69.vcd" 
	scene "scenes/Player/Engineer/low/68.vcd" 
	scene "scenes/Player/Engineer/low/67.vcd" 
}
Rule PlayerGoEngineer
{
	criteria ConceptPlayerGo IsEngineer
	Response PlayerGoEngineer
}

Response PlayerHeadLeftEngineer
{
	scene "scenes/Player/Engineer/low/73.vcd" 
	scene "scenes/Player/Engineer/low/75.vcd" 
}
Rule PlayerHeadLeftEngineer
{
	criteria ConceptPlayerLeft  IsEngineer
	Response PlayerHeadLeftEngineer
}

Response PlayerHeadRightEngineer
{
	scene "scenes/Player/Engineer/low/76.vcd" 
	scene "scenes/Player/Engineer/low/77.vcd" 
	scene "scenes/Player/Engineer/low/78.vcd" 
}
Rule PlayerHeadRightEngineer
{
	criteria ConceptPlayerRight  IsEngineer
	Response PlayerHeadRightEngineer
}

Response PlayerHelpEngineer
{
	scene "scenes/Player/Engineer/low/79.vcd" 
	scene "scenes/Player/Engineer/low/80.vcd" 
	scene "scenes/Player/Engineer/low/81.vcd" 
}
Rule PlayerHelpEngineer
{
	criteria ConceptPlayerHelp IsEngineer
	Response PlayerHelpEngineer
}

Response PlayerHelpCaptureEngineer
{
	scene "scenes/Player/Engineer/low/82.vcd" 
	scene "scenes/Player/Engineer/low/84.vcd" 
	scene "scenes/Player/Engineer/low/83.vcd" 
}
Rule PlayerHelpCaptureEngineer
{
	criteria ConceptPlayerHelp IsEngineer IsOnCappableControlPoint
	ApplyContext "EngineerHelpCap:1:10"
	Response PlayerHelpCaptureEngineer
}

Response PlayerHelpCapture2Engineer
{
	scene "scenes/Player/Engineer/low/161.vcd" 
	scene "scenes/Player/Engineer/low/1344.vcd" 
}
Rule PlayerHelpCapture2Engineer
{
	criteria ConceptPlayerHelp IsEngineer IsOnCappableControlPoint IsHelpCapEngineer
	Response PlayerHelpCapture2Engineer
}

Response PlayerHelpDefendEngineer
{
	scene "scenes/Player/Engineer/low/85.vcd" 
	scene "scenes/Player/Engineer/low/86.vcd" 
	scene "scenes/Player/Engineer/low/87.vcd" 
}
Rule PlayerHelpDefendEngineer
{
	criteria ConceptPlayerHelp IsEngineer IsOnFriendlyControlPoint
	Response PlayerHelpDefendEngineer
}

Response PlayerMedicEngineer
{
	scene "scenes/Player/Engineer/low/109.vcd" 
	scene "scenes/Player/Engineer/low/107.vcd" 
	scene "scenes/Player/Engineer/low/108.vcd" 
}
Rule PlayerMedicEngineer
{
	criteria ConceptPlayerMedic IsEngineer
	Response PlayerMedicEngineer
}

Response PlayerAskForBallEngineer
{
}
Rule PlayerAskForBallEngineer
{
	criteria ConceptPlayerAskForBall IsEngineer
	Response PlayerAskForBallEngineer
}

Response PlayerMoveUpEngineer
{
	scene "scenes/Player/Engineer/low/111.vcd" 
}
Rule PlayerMoveUpEngineer
{
	criteria ConceptPlayerMoveUp  IsEngineer
	Response PlayerMoveUpEngineer
}

Response PlayerNoEngineer
{
	scene "scenes/Player/Engineer/low/127.vcd" 
	scene "scenes/Player/Engineer/low/128.vcd" 
	scene "scenes/Player/Engineer/low/129.vcd" 
}
Rule PlayerNoEngineer
{
	criteria ConceptPlayerNo  IsEngineer
	Response PlayerNoEngineer
}

Response PlayerThanksEngineer
{
	scene "scenes/Player/Engineer/low/180.vcd" 
}
Rule PlayerThanksEngineer
{
	criteria ConceptPlayerThanks IsEngineer
	Response PlayerThanksEngineer
}

// Custom Assist kill response
// As there is no actual concept for assist kills, this is the second best method.
// Say thanks after you kill more than one person.

Response KilledPlayerAssistEngineer
{
	scene "scenes/Player/Engineer/low/159.vcd"
	scene "scenes/Player/Engineer/low/160.vcd"
}
Rule KilledPlayerAssistEngineer
{
	criteria ConceptPlayerThanks IsEngineer IsARecentKill KilledPlayerDelay EngineerNotAssistSpeech
	ApplyContext "EngineerAssistSpeech:1:20"
	Response KilledPlayerAssistEngineer
}
// End custom

Response PlayerYesEngineer
{
	scene "scenes/Player/Engineer/low/187.vcd" 
	scene "scenes/Player/Engineer/low/188.vcd" 
	scene "scenes/Player/Engineer/low/189.vcd" 
}
Rule PlayerYesEngineer
{
	criteria ConceptPlayerYes  IsEngineer
	Response PlayerYesEngineer
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 2
//--------------------------------------------------------------------------------------------------------------
Response PlayerActivateChargeEngineer
{
	scene "scenes/Player/Engineer/low/1.vcd" 
	scene "scenes/Player/Engineer/low/2.vcd" 
	scene "scenes/Player/Engineer/low/3.vcd" 
}
Rule PlayerActivateChargeEngineer
{
	criteria ConceptPlayerActivateCharge IsEngineer
	Response PlayerActivateChargeEngineer
}

Response PlayerCloakedSpyEngineer
{
	scene "scenes/Player/Engineer/low/45.vcd" 
	scene "scenes/Player/Engineer/low/47.vcd" 
	scene "scenes/Player/Engineer/low/46.vcd" 
}
Rule PlayerCloakedSpyEngineer
{
	criteria ConceptPlayerCloakedSpy IsEngineer
	Response PlayerCloakedSpyEngineer
}

Response PlayerDispenserHereEngineer
{
	scene "scenes/Player/Engineer/low/113.vcd" 
}
Rule PlayerDispenserHereEngineer
{
	criteria ConceptPlayerDispenserHere IsEngineer
	Response PlayerDispenserHereEngineer
}

Response PlayerIncomingEngineer
{
	scene "scenes/Player/Engineer/low/88.vcd" 
	scene "scenes/Player/Engineer/low/89.vcd" 
	scene "scenes/Player/Engineer/low/90.vcd" 
}
Rule PlayerIncomingEngineer
{
	criteria ConceptPlayerIncoming IsEngineer
	Response PlayerIncomingEngineer
}

Response PlayerSentryAheadEngineer
{
	scene "scenes/Player/Engineer/low/145.vcd" 
	scene "scenes/Player/Engineer/low/146.vcd" 
}
Rule PlayerSentryAheadEngineer
{
	criteria ConceptPlayerSentryAhead IsEngineer
	Response PlayerSentryAheadEngineer
}

Response PlayerSentryHereEngineer
{
	scene "scenes/Player/Engineer/low/115.vcd" 
}
Rule PlayerSentryHereEngineer
{
	criteria ConceptPlayerSentryHere IsEngineer
	Response PlayerSentryHereEngineer
}

Response PlayerTeleporterHereEngineer
{
	scene "scenes/Player/Engineer/low/117.vcd" 
	scene "scenes/Player/Engineer/low/118.vcd" 
}
Rule PlayerTeleporterHereEngineer
{
	criteria ConceptPlayerTeleporterHere IsEngineer
	Response PlayerTeleporterHereEngineer
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 3
//--------------------------------------------------------------------------------------------------------------
Response PlayerBattleCryEngineer
{
	scene "scenes/Player/Engineer/low/31.vcd" 
	scene "scenes/Player/Engineer/low/33.vcd" 
	scene "scenes/Player/Engineer/low/34.vcd" 
	scene "scenes/Player/Engineer/low/35.vcd" 
	scene "scenes/Player/Engineer/low/36.vcd" 
	scene "scenes/Player/Engineer/low/1329.vcd" 
}
Rule PlayerBattleCryEngineer
{
	criteria ConceptPlayerBattleCry IsEngineer
	Response PlayerBattleCryEngineer
}

// Custom stuff - melee dare
// Look at enemy, then do battle cry voice command while holding a melee weapon.
Response MeleeDareCombatEngineer
{
	scene "scenes/Player/Engineer/low/163.vcd"
	scene "scenes/Player/Engineer/low/166.vcd"
	scene "scenes/Player/Engineer/low/172.vcd"
	scene "scenes/Player/Engineer/low/169.vcd"
	scene "scenes/Player/Engineer/low/174.vcd"
	scene "scenes/Player/Engineer/low/178.vcd"
	scene "scenes/Player/Engineer/low/3619.vcd" 
	scene "scenes/Player/Engineer/low/3620.vcd" 
	scene "scenes/Player/Engineer/low/3695.vcd" 
}
Rule MeleeDareCombatEngineer
{
	criteria ConceptPlayerBattleCry IsWeaponMelee IsEngineer IsCrosshairEnemy
	Response MeleeDareCombatEngineer
}
Rule MeleeDareCombatEngineerSlinger
{
	criteria ConceptPlayerBattleCry WeaponIsRobotArm IsEngineer IsCrosshairEnemy
	Response MeleeDareCombatEngineer
}
//End custom

Response PlayerCheersEngineer
{
	scene "scenes/Player/Engineer/low/40.vcd" 
	scene "scenes/Player/Engineer/low/1330.vcd" 
	scene "scenes/Player/Engineer/low/42.vcd" 
	scene "scenes/Player/Engineer/low/43.vcd" 
	scene "scenes/Player/Engineer/low/41.vcd" 
	scene "scenes/Player/Engineer/low/39.vcd" 
	scene "scenes/Player/Engineer/low/44.vcd" 
}
Rule PlayerCheersEngineer
{
	criteria ConceptPlayerCheers IsEngineer
	Response PlayerCheersEngineer
}

Response PlayerGoodJobEngineer
{
	scene "scenes/Player/Engineer/low/70.vcd" 
	scene "scenes/Player/Engineer/low/72.vcd" 
	scene "scenes/Player/Engineer/low/71.vcd" 
}
Rule PlayerGoodJobEngineer
{
	criteria ConceptPlayerGoodJob IsEngineer
	Response PlayerGoodJobEngineer
}

Response PlayerJeersEngineer
{
	scene "scenes/Player/Engineer/low/94.vcd" 
	scene "scenes/Player/Engineer/low/95.vcd" 
	scene "scenes/Player/Engineer/low/96.vcd" 
	scene "scenes/Player/Engineer/low/98.vcd" 
}
Rule PlayerJeersEngineer
{
	criteria ConceptPlayerJeers IsEngineer
	Response PlayerJeersEngineer
}

Response PlayerLostPointEngineer
{
	scene "scenes/Player/Engineer/low/1327.vcd" 
	scene "scenes/Player/Engineer/low/1328.vcd" 
	scene "scenes/Player/Engineer/low/119.vcd" 
	scene "scenes/Player/Engineer/low/120.vcd" 
	scene "scenes/Player/Engineer/low/122.vcd" 
	scene "scenes/Player/Engineer/low/121.vcd" 
	scene "scenes/Player/Engineer/low/123.vcd" 
	scene "scenes/Player/Engineer/low/1339.vcd" 
	scene "scenes/Player/Engineer/low/1340.vcd" 
	scene "scenes/Player/Engineer/low/1341.vcd" 
	scene "scenes/Player/Engineer/low/1342.vcd" 
	scene "scenes/Player/Engineer/low/1343.vcd" 
}
Rule PlayerLostPointEngineer
{
	criteria ConceptPlayerLostPoint IsEngineer
	Response PlayerLostPointEngineer
}

Response PlayerNegativeEngineer
{
	scene "scenes/Player/Engineer/low/1327.vcd" 
	scene "scenes/Player/Engineer/low/1328.vcd" 
	scene "scenes/Player/Engineer/low/119.vcd" 
	scene "scenes/Player/Engineer/low/120.vcd" 
	scene "scenes/Player/Engineer/low/122.vcd" 
	scene "scenes/Player/Engineer/low/121.vcd" 
	scene "scenes/Player/Engineer/low/123.vcd" 
	scene "scenes/Player/Engineer/low/1339.vcd" 
	scene "scenes/Player/Engineer/low/1340.vcd" 
	scene "scenes/Player/Engineer/low/1341.vcd" 
	scene "scenes/Player/Engineer/low/1342.vcd" 
	scene "scenes/Player/Engineer/low/1343.vcd" 
}
Rule PlayerNegativeEngineer
{
	criteria ConceptPlayerNegative IsEngineer
	Response PlayerNegativeEngineer
}

Response PlayerNiceShotEngineer
{
	scene "scenes/Player/Engineer/low/125.vcd" 
	scene "scenes/Player/Engineer/low/126.vcd" 
	scene "scenes/Player/Engineer/low/124.vcd" 
}
Rule PlayerNiceShotEngineer
{
	criteria ConceptPlayerNiceShot IsEngineer
	Response PlayerNiceShotEngineer
}

Response PlayerPositiveEngineer
{
	scene "scenes/Player/Engineer/low/139.vcd" 
}
Rule PlayerPositiveEngineer
{
	criteria ConceptPlayerPositive IsEngineer
	Response PlayerPositiveEngineer
}

//--------------------------------------------------------------------------------------------------------------
// MvM Speech
//--------------------------------------------------------------------------------------------------------------
Response MvMBombDroppedEngineer
{
	scene "scenes/Player/Engineer/low/4144.vcd" 
	scene "scenes/Player/Engineer/low/4145.vcd" 
}
Rule MvMBombDroppedEngineer
{
	criteria ConceptMvMBombDropped 5PercentChance IsMvMDefender IsEngineer 
	Response MvMBombDroppedEngineer
}

Response MvMBombCarrierUpgrade1Engineer
{
	scene "scenes/Player/Engineer/low/4140.vcd" 
}
Rule MvMBombCarrierUpgrade1Engineer
{
	criteria ConceptMvMBombCarrierUpgrade1 5PercentChance IsMvMDefender IsEngineer 
	Response MvMBombCarrierUpgrade1Engineer
}

Response MvMBombCarrierUpgrade2Engineer
{
	scene "scenes/Player/Engineer/low/4141.vcd" 
}
Rule MvMBombCarrierUpgrade2Engineer
{
	criteria ConceptMvMBombCarrierUpgrade2 5PercentChance IsMvMDefender IsEngineer 
	Response MvMBombCarrierUpgrade2Engineer
}

Response MvMDefenderDiedScoutEngineer
{
	scene "scenes/Player/Engineer/low/4110.vcd" 
}
Rule MvMDefenderDiedScoutEngineer
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimScout IsEngineer 
	Response MvMDefenderDiedScoutEngineer
}

Response MvMDefenderDiedSpyEngineer
{
	scene "scenes/Player/Engineer/low/4111.vcd" 
}
Rule MvMDefenderDiedSpyEngineer
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimSpy IsEngineer
	Response MvMDefenderDiedSpyEngineer
}

Response MvMDefenderDiedHeavyEngineer
{
	scene "scenes/Player/Engineer/low/4112.vcd" 
}
Rule MvMDefenderDiedHeavyEngineer
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimHeavy IsEngineer
	Response MvMDefenderDiedHeavyEngineer
}

Response MvMDefenderDiedSoldierEngineer
{
	scene "scenes/Player/Engineer/low/4113.vcd" 
}
Rule MvMDefenderDiedSoldierEngineer
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimSoldier IsEngineer
	Response MvMDefenderDiedSoldierEngineer
}

Response MvMDefenderDiedMedicEngineer
{
	scene "scenes/Player/Engineer/low/4114.vcd" 
}
Rule MvMDefenderDiedMedicEngineer
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimMedic IsEngineer
	Response MvMDefenderDiedMedicEngineer
}

Response MvMDefenderDiedDemomanEngineer
{
	scene "scenes/Player/Engineer/low/4115.vcd" 
}
Rule MvMDefenderDiedDemomanEngineer
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimDemoman IsEngineer 
	Response MvMDefenderDiedDemomanEngineer
}

Response MvMDefenderDiedPyroEngineer
{
	scene "scenes/Player/Engineer/low/4116.vcd" 
}
Rule MvMDefenderDiedPyroEngineer
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimPyro IsEngineer
	Response MvMDefenderDiedPyroEngineer
}

Response MvMDefenderDiedSniperEngineer
{
	scene "scenes/Player/Engineer/low/4117.vcd" 
}
Rule MvMDefenderDiedSniperEngineer
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimSniper IsEngineer
	Response MvMDefenderDiedSniperEngineer
}

Response MvMDefenderDiedEngineerEngineer
{
	scene "scenes/Player/Engineer/low/4118.vcd" 
}
Rule MvMDefenderDiedEngineerEngineer
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimEngineer IsEngineer
	Response MvMDefenderDiedEngineerEngineer
}

Response MvMFirstBombPickupEngineer
{
	scene "scenes/Player/Engineer/low/4137.vcd" 
	scene "scenes/Player/Engineer/low/4139.vcd" 
}
Rule MvMFirstBombPickupEngineer
{
	criteria ConceptMvMFirstBombPickup 5PercentChance IsMvMDefender IsEngineer
	Response MvMFirstBombPickupEngineer
}

Response MvMBombPickupEngineer
{
	scene "scenes/Player/Engineer/low/4136.vcd" 
}
Rule MvMBombPickupEngineer
{
	criteria ConceptMvMBombPickup 5PercentChance IsMvMDefender IsEngineer
	Response MvMBombPickupEngineer
}

Response MvMSniperCalloutEngineer
{
	scene "scenes/Player/Engineer/low/4120.vcd" 
}
Rule MvMSniperCalloutEngineer
{
	criteria ConceptMvMSniperCallout 50PercentChance IsMvMDefender IsEngineer
	Response MvMSniperCalloutEngineer
}

Response MvMSentryBusterEngineer
{
	scene "scenes/Player/Engineer/low/4155.vcd" 
}
Rule MvMSentryBusterEngineer
{
	criteria ConceptMvMSentryBuster 50PercentChance IsMvMDefender IsEngineer
	Response MvMSentryBusterEngineer
}

Response MvMSentryBusterDownEngineer
{
	scene "scenes/Player/Engineer/low/4156.vcd" 
}
Rule MvMSentryBusterDownEngineer
{
	criteria ConceptMvMSentryBusterDown 20PercentChance IsMvMDefender IsEngineer
	Response MvMSentryBusterDownEngineer
}

Response MvMLastManStandingEngineer
{
	scene "scenes/Player/Engineer/low/4119.vcd" 
}
Rule MvMLastManStandingEngineer
{
	criteria ConceptMvMLastManStanding 20PercentChance IsMvMDefender IsEngineer
	Response MvMLastManStandingEngineer
}

Response MvMEncourageMoneyEngineer
{
	scene "scenes/Player/Engineer/low/4128.vcd" 
	scene "scenes/Player/Engineer/low/4129.vcd" 
	scene "scenes/Player/Engineer/low/4130.vcd" 
}
Rule MvMEncourageMoneyEngineer
{
	criteria ConceptMvMEncourageMoney 50PercentChance IsMvMDefender IsEngineer
	Response MvMEncourageMoneyEngineer
}

Response MvMEncourageUpgradeEngineer
{
	scene "scenes/Player/Engineer/low/4135.vcd" 
}
Rule MvMEncourageUpgradeEngineer
{
	criteria ConceptMvMEncourageUpgrade 50PercentChance IsMvMDefender IsEngineer
	Response MvMEncourageUpgradeEngineer
}

Response MvMUpgradeCompleteEngineer
{
	scene "scenes/Player/Engineer/low/4131.vcd" 
	scene "scenes/Player/Engineer/low/4133.vcd" 
}
Rule MvMUpgradeCompleteEngineer
{
	criteria ConceptMvMUpgradeComplete 5PercentChance IsMvMDefender IsEngineer
	Response MvMUpgradeCompleteEngineer
}

Response MvMGiantCalloutEngineer
{
	scene "scenes/Player/Engineer/low/4157.vcd" 
	scene "scenes/Player/Engineer/low/4158.vcd" 
}
Rule MvMGiantCalloutEngineer
{
	criteria ConceptMvMGiantCallout 20PercentChance IsMvMDefender IsEngineer
	Response MvMGiantCalloutEngineer
}

Response MvMGiantHasBombEngineer
{
	scene "scenes/Player/Engineer/low/4162.vcd" 
}
Rule MvMGiantHasBombEngineer
{
	criteria ConceptMvMGiantHasBomb 20PercentChance IsMvMDefender IsEngineer
	Response MvMGiantHasBombEngineer
}

Response MvMSappedRobotEngineer
{
	scene "scenes/Player/Engineer/low/4121.vcd" 
	scene "scenes/Player/Engineer/low/4122.vcd" 
}
Rule MvMSappedRobotEngineer
{
	criteria ConceptMvMSappedRobot 50PercentChance IsMvMDefender IsEngineer
	Response MvMSappedRobotEngineer
}

Response MvMCloseCallEngineer
{
	scene "scenes/Player/Engineer/low/4143.vcd" 
}
Rule MvMCloseCallEngineer
{
	criteria ConceptMvMCloseCall 50PercentChance IsMvMDefender IsEngineer
	Response MvMCloseCallEngineer
}

Response MvMTankCalloutEngineer
{
	scene "scenes/Player/Engineer/low/4146.vcd" 
}
Rule MvMTankCalloutEngineer
{
	criteria ConceptMvMTankCallout 50PercentChance IsMvMDefender IsEngineer
	Response MvMTankCalloutEngineer
}

Response MvMTankDeadEngineer
{
	scene "scenes/Player/Engineer/low/4152.vcd" 
}
Rule MvMTankDeadEngineer
{
	criteria ConceptMvMTankDead 50PercentChance IsMvMDefender IsEngineer
	Response MvMTankDeadEngineer
}

Response MvMTankDeployingEngineer
{
	scene "scenes/Player/Engineer/low/4151.vcd" 
}
Rule MvMTankDeployingEngineer
{
	criteria ConceptMvMTankDeploying 50PercentChance IsMvMDefender IsEngineer
	Response MvMTankDeployingEngineer
}

Response MvMAttackTheTankEngineer
{
	scene "scenes/Player/Engineer/low/4147.vcd" 
}
Rule MvMAttackTheTankEngineer
{
	criteria ConceptMvMAttackTheTank 50PercentChance IsMvMDefender IsEngineer
	Response MvMAttackTheTankEngineer
}

Response MvMTauntEngineer
{
	scene "scenes/Player/Engineer/low/4123.vcd" 
	scene "scenes/Player/Engineer/low/4127.vcd" 
}
Rule MvMTauntEngineer
{
	criteria ConceptMvMTaunt 50PercentChance IsMvMDefender IsEngineer
	Response MvMTauntEngineer
}

Response MvMWaveStartEngineer
{
	scene "scenes/Player/Engineer/low/4109.vcd" 
}
Rule MvMWaveStartEngineer
{
	criteria ConceptMvMWaveStart 50PercentChance IsMvMDefender IsEngineer
	Response MvMWaveStartEngineer
}

Response MvMWaveWinEngineer
{
	scene "scenes/Player/Engineer/low/4095.vcd" 
	scene "scenes/Player/Engineer/low/4096.vcd" 
	scene "scenes/Player/Engineer/low/4097.vcd" 
	scene "scenes/Player/Engineer/low/4098.vcd" 
	scene "scenes/Player/Engineer/low/4321.vcd" 
}
Rule MvMWaveWinEngineer
{
	criteria ConceptMvMWaveWin 50PercentChance IsMvMDefender IsEngineer
	Response MvMWaveWinEngineer
}

Response MvMWaveLoseEngineer
{
	scene "scenes/Player/Engineer/low/4099.vcd" 
	scene "scenes/Player/Engineer/low/4100.vcd" 
}
Rule MvMWaveLoseEngineer
{
	criteria ConceptMvMWaveLose 50PercentChance IsMvMDefender IsEngineer
	Response MvMWaveLoseEngineer
}

//--------------------------------------------------------------------------------------------------------------
// Begin Competitive Mode VO
//--------------------------------------------------------------------------------------------------------------
Response PlayerFirstRoundStartCompEngineer
{
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_09.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_10.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_11.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_12.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_13.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_14.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_15.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_18.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_comp_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_comp_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_comp_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_comp_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_comp_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_comp_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_comp_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_rare_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_rare_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_rare_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_rare_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_rare_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_rare_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_rare_08.vcd" predelay "1.0, 5.0"
}
Rule PlayerFirstRoundStartCompEngineer
{
	criteria ConceptPlayerRoundStartComp IsEngineer IsFirstRound IsNotComp6v6 40PercentChance
	Response PlayerFirstRoundStartCompEngineer
}

Response PlayerFirstRoundStartComp6sEngineer
{
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_09.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_10.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_11.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_12.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_13.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_14.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_15.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_18.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_comp_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_comp_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_comp_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_comp_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_comp_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_comp_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_comp_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_rare_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_rare_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_rare_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_rare_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_rare_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_rare_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_rare_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_6s_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_6s_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamefirst_6s_03.vcd" predelay "1.0, 5.0"
}
Rule PlayerFirstRoundStartComp6sEngineer
{
	criteria ConceptPlayerRoundStartComp IsEngineer IsFirstRound IsComp6v6 40PercentChance
	Response PlayerFirstRoundStartComp6sEngineer
}

Response PlayerWonPrevRoundCompEngineer
{
	scene "scenes/Player/Engineer/low/cm_engie_pregamewonlast_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamewonlast_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamewonlast_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamewonlast_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamewonlast_comp_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamewonlast_comp_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamewonlast_comp_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamewonlast_comp_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamewonlast_rare_01.vcd" predelay "1.0, 5.0"
}
Rule PlayerWonPrevRoundCompEngineer
{
	criteria ConceptPlayerRoundStartComp IsEngineer IsNotFirstRound PlayerWonPreviousRound 40PercentChance
	Response PlayerWonPrevRoundCompEngineer
}

Response PlayerLostPrevRoundCompEngineer
{
	scene "scenes/Player/Engineer/low/cm_engie_pregamelostlast_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamelostlast_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamelostlast_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamelostlast_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamelostlast_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamelostlast_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamelostlast_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamelostlast_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregamelostlast_09.vcd" predelay "1.0, 5.0"
}
Rule PlayerLostPrevRoundCompEngineer
{
	criteria ConceptPlayerRoundStartComp IsEngineer IsNotFirstRound PlayerLostPreviousRound PreviousRoundWasNotTie 40PercentChance
	Response PlayerLostPrevRoundCompEngineer
}

Response PlayerTiedPrevRoundCompEngineer
{
	scene "scenes/Player/Engineer/low/cm_engie_pregametie_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregametie_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregametie_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregametie_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregametie_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_pregametie_06.vcd" predelay "1.0, 5.0"
}
Rule PlayerTiedPrevRoundCompEngineer
{
	criteria ConceptPlayerRoundStartComp IsEngineer IsNotFirstRound PreviousRoundWasTie 40PercentChance
	Response PlayerTiedPrevRoundCompEngineer
}

Response PlayerGameWinCompEngineer
{
	scene "scenes/Player/Engineer/low/cm_engie_gamewon_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_gamewon_02.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_gamewon_03.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_gamewon_04.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_gamewon_05.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_gamewon_06.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_gamewon_07.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_gamewon_08.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_gamewon_09.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_gamewon_10.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_gamewon_11.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_gamewon_12.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_gamewon_13.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_gamewon_14.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_gamewon_rare_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_gamewon_rare_02.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_gamewon_rare_03.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Engineer/low/cm_engie_gamewon_rare_04.vcd" predelay "2.0, 5.0"
}
Rule PlayerGameWinCompEngineer
{
	criteria ConceptPlayerGameOverComp PlayerOnWinningTeam IsEngineer 40PercentChance
	Response PlayerGameWinCompEngineer
}

Response PlayerMatchWinCompEngineer
{
	scene "scenes/Player/Engineer/low/cm_engie_matchwon_01.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Engineer/low/cm_engie_matchwon_02.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Engineer/low/cm_engie_matchwon_03.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Engineer/low/cm_engie_matchwon_04.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Engineer/low/cm_engie_matchwon_05.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Engineer/low/cm_engie_matchwon_06.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Engineer/low/cm_engie_matchwon_08.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Engineer/low/cm_engie_matchwon_09.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Engineer/low/cm_engie_matchwon_10.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Engineer/low/cm_engie_matchwon_11.vcd" predelay "1.0, 2.0"
}
Rule PlayerMatchWinCompEngineer
{
	criteria ConceptPlayerMatchOverComp PlayerOnWinningTeam IsEngineer 40PercentChance
	Response PlayerMatchWinCompEngineer
}
//--------------------------------------------------------------------------------------------------------------
// End Competitive Mode VO
//--------------------------------------------------------------------------------------------------------------