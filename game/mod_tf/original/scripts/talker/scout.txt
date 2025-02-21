//--------------------------------------------------------------------------------------------------------------
// Scout Response Rule File
//--------------------------------------------------------------------------------------------------------------

Criterion "ScoutIsKillSpeechObject" "ScoutKillSpeechObject" "1" "required" weight 0
Criterion "ScoutIsNotStillonFire" "ScoutOnFire" "!=1" "required" weight 0
Criterion "ScoutIsStillonFire" "ScoutOnFire" "1" "required" weight 0
Criterion "ScoutNotKillSpeech" "ScoutKillSpeech" "!=1" "required" weight 0
Criterion "ScoutNotKillSpeechMelee" "ScoutKillSpeechMelee" "!=1" "required" weight 0
Criterion "ScoutNotKillSpeechMeleeFat" "ScoutKillSpeechMeleeFat" "!=1" "required" weight 0
Criterion "ScoutNotSaidCartMovingBackwardD" "SaidCartMovingBackwardD" "!=1" "required" weight 0
Criterion "ScoutNotSaidCartMovingBackwardO" "SaidCartMovingBackwardO" "!=1" "required" weight 0
Criterion "ScoutNotSaidCartMovingForwardD" "SaidCartMovingForwardD" "!=1" "required" weight 0
Criterion "ScoutNotSaidCartMovingForwardO" "SaidCartMovingForwardO" "!=1" "required" weight 0
Criterion "ScoutNotSaidCartMovingStoppedD" "SaidCartMovingStoppedD" "!=1" "required" weight 0
Criterion "ScoutNotSaidCartMovingStoppedO" "SaidCartMovingStoppedO" "!=1" "required" weight 0
Criterion "ScoutNotSaidHealThanks" "ScoutSaidHealThanks" "!=1" "required"
Criterion "IsHelpCapScout" "ScoutHelpCap" "1" "required" weight 0
Criterion "NotSaidScoutHitBallSpeech" "ScoutHitBallSpeech" "!=1" "required"
Criterion "NotScoutGrabbedIntelligence" "ScoutGrabbedIntelligence" "!=1" "required"
Criterion "ScoutIsNotInvuln" "ScoutInvuln" "!=1" "required"
//Custom stuff
Criterion "ScoutNotInvulnerableSpeech" "ScoutInvulnerableSpeech" "!=1" "required" weight 0
Criterion "ScoutNotAssistSpeech" "ScoutAssistSpeech" "!=1" "required" weight 0
Criterion "ScoutNotDoubleJumpSpeech" "ScoutDoubleJumpSpeech" "!=1" "required" weight 0
Criterion "ScoutNotAwardSpeech" "ScoutAwardSpeech" "!=1" "required" weight 0
Criterion "ScoutNotDrinkReadySpeech" "ScoutDrinkReadySpeech" "!=1" "required" weight 0
Criterion "ScoutIsNotCrit" "ScoutIsCrit" "1" "required" weight 0
Criterion "ScoutHasFired" "ScoutFired" "1" "required" weight 0

Response PlayerCloakedSpyDemomanScout
{
	scene "scenes/Player/Scout/low/386.vcd" 
}
Rule PlayerCloakedSpyDemomanScout
{
	criteria ConceptPlayerCloakedSpy IsScout IsOnDemoman
	Response PlayerCloakedSpyDemomanScout
}

Response PlayerCloakedSpyEngineerScout
{
	scene "scenes/Player/Scout/low/389.vcd" 
}
Rule PlayerCloakedSpyEngineerScout
{
	criteria ConceptPlayerCloakedSpy IsScout IsOnEngineer
	Response PlayerCloakedSpyEngineerScout
}

Response PlayerCloakedSpyHeavyScout
{
	scene "scenes/Player/Scout/low/384.vcd" 
}
Rule PlayerCloakedSpyHeavyScout
{
	criteria ConceptPlayerCloakedSpy IsScout IsOnHeavy
	Response PlayerCloakedSpyHeavyScout
}

Response PlayerCloakedSpyMedicScout
{
	scene "scenes/Player/Scout/low/388.vcd" 
}
Rule PlayerCloakedSpyMedicScout
{
	criteria ConceptPlayerCloakedSpy IsScout IsOnMedic
	Response PlayerCloakedSpyMedicScout
}

Response PlayerCloakedSpyPyroScout
{
	scene "scenes/Player/Scout/low/385.vcd" 
}
Rule PlayerCloakedSpyPyroScout
{
	criteria ConceptPlayerCloakedSpy IsScout IsOnPyro
	Response PlayerCloakedSpyPyroScout
}

Response PlayerCloakedSpyScoutScout
{
	scene "scenes/Player/Scout/low/382.vcd" 
}
Rule PlayerCloakedSpyScoutScout
{
	criteria ConceptPlayerCloakedSpy IsScout IsOnScout
	Response PlayerCloakedSpyScoutScout
}

Response PlayerCloakedSpySniperScout
{
	scene "scenes/Player/Scout/low/390.vcd" 
}
Rule PlayerCloakedSpySniperScout
{
	criteria ConceptPlayerCloakedSpy IsScout IsOnSniper
	Response PlayerCloakedSpySniperScout
}

Response PlayerCloakedSpySoldierScout
{
	scene "scenes/Player/Scout/low/383.vcd" 
}
Rule PlayerCloakedSpySoldierScout
{
	criteria ConceptPlayerCloakedSpy IsScout IsOnSoldier
	Response PlayerCloakedSpySoldierScout
}

Response PlayerCloakedSpySpyScout
{
	scene "scenes/Player/Scout/low/387.vcd" 
}
Rule PlayerCloakedSpySpyScout
{
	criteria ConceptPlayerCloakedSpy IsScout IsOnSpy
	Response PlayerCloakedSpySpyScout
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech
//--------------------------------------------------------------------------------------------------------------

// Custom achievement stuff
Response AwardScout
{
	scene "scenes/Player/Scout/low/2501.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2502.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2503.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2504.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2505.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2507.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2509.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2510.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2511.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2681.vcd" predelay "2.5" 
	scene "scenes/Player/Scout/low/2682.vcd" predelay "2.5"
}
Rule AwardScout
{
	criteria ConceptAchievementAward IsScout ScoutNotAwardSpeech
	ApplyContext "ScoutAwardSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response AwardScout
}
//End custom achievement

Response HealThanksScout
{
	scene "scenes/Player/Scout/low/510.vcd" 
	scene "scenes/Player/Scout/low/511.vcd" 
	scene "scenes/Player/Scout/low/512.vcd" 
}
Rule HealThanksScout
{
	criteria ConceptMedicChargeStopped IsScout SuperHighHealthContext ScoutNotSaidHealThanks 50PercentChance
	ApplyContext "ScoutSaidHealThanks:1:20"
	Response HealThanksScout
}

Response PlayerRoundStartScout
{
	scene "scenes/Player/Scout/low/367.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/369.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/370.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/371.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/368.vcd" predelay "1.0, 5.0"
}
Rule PlayerRoundStartScout
{
	criteria ConceptPlayerRoundStart IsScout
	Response PlayerRoundStartScout
}

Response PlayerCappedIntelligenceScout
{
	scene "scenes/Player/Scout/low/359.vcd" 
	scene "scenes/Player/Scout/low/360.vcd" 
	scene "scenes/Player/Scout/low/361.vcd" 
	scene "scenes/Player/Scout/low/1289.vcd" 
}
Rule PlayerCappedIntelligenceScout
{
	criteria ConceptPlayerCapturedIntelligence IsScout
	Response PlayerCappedIntelligenceScout
}

Response PlayerCapturedPointScout
{
	scene "scenes/Player/Scout/low/356.vcd" 
	scene "scenes/Player/Scout/low/1280.vcd" 
	scene "scenes/Player/Scout/low/357.vcd" 
	scene "scenes/Player/Scout/low/358.vcd" 
}
Rule PlayerCapturedPointScout
{
	criteria ConceptPlayerCapturedPoint IsScout
	Response PlayerCapturedPointScout
}

Response PlayerGrabbedIntelligenceScout
{
	scene "scenes/Player/Scout/low/480.vcd" 
}
Rule PlayerGrabbedIntelligenceScout
{
	criteria ConceptPlayerGrabbedIntelligence IsScout NotScoutGrabbedIntelligence 10PercentChance
	ApplyContext "ScoutGrabbedIntelligence:1:30"
	Response PlayerGrabbedIntelligenceScout
}

Response PlayerSuddenDeathScout
{
	scene "scenes/Player/Scout/low/419.vcd" 
	scene "scenes/Player/Scout/low/420.vcd" 
	scene "scenes/Player/Scout/low/421.vcd" 
	scene "scenes/Player/Scout/low/422.vcd" 
	scene "scenes/Player/Scout/low/423.vcd" 
	scene "scenes/Player/Scout/low/424.vcd" 
	scene "scenes/Player/Scout/low/425.vcd" 
	scene "scenes/Player/Scout/low/426.vcd" 
	scene "scenes/Player/Scout/low/427.vcd" 
	scene "scenes/Player/Scout/low/428.vcd" 
	scene "scenes/Player/Scout/low/430.vcd" 
}
Rule PlayerSuddenDeathScout
{
	criteria ConceptPlayerSuddenDeathStart IsScout
	Response PlayerSuddenDeathScout
}

Response PlayerStalemateScout
{
	scene "scenes/Player/Scout/low/362.vcd" 
	scene "scenes/Player/Scout/low/1281.vcd" 
	scene "scenes/Player/Scout/low/363.vcd" 
	scene "scenes/Player/Scout/low/364.vcd" 
}
Rule PlayerStalemateScout
{
	criteria ConceptPlayerStalemate IsScout
	Response PlayerStalemateScout
}

Response PlayerTeleporterThanksScout
{
	scene "scenes/Player/Scout/low/513.vcd" 
	scene "scenes/Player/Scout/low/514.vcd" 
	scene "scenes/Player/Scout/low/515.vcd" 
}
Rule PlayerTeleporterThanksScout
{
	criteria ConceptTeleported IsNotEngineer IsScout 30PercentChance
	Response PlayerTeleporterThanksScout
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Cart
//--------------------------------------------------------------------------------------------------------------
Response CartMovingBackwardsDefenseScout
{
	scene "scenes/Player/Scout/low/2513.vcd" 
	scene "scenes/Player/Scout/low/2514.vcd" 
	scene "scenes/Player/Scout/low/2512.vcd" 
	scene "scenes/Player/Scout/low/2515.vcd" 
	scene "scenes/Player/Scout/low/2516.vcd" 
	scene "scenes/Player/Scout/low/2517.vcd" 
}
Rule CartMovingBackwardsDefenseScout
{
	criteria ConceptCartMovingBackward IsOnDefense IsScout ScoutNotSaidCartMovingBackwardD IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingBackwardD:1:20"
	Response CartMovingBackwardsDefenseScout
}

Response CartMovingBackwardsOffenseScout
{
	scene "scenes/Player/Scout/low/2518.vcd" 
	scene "scenes/Player/Scout/low/2519.vcd" 
	scene "scenes/Player/Scout/low/2520.vcd" 
	scene "scenes/Player/Scout/low/2522.vcd" 
	scene "scenes/Player/Scout/low/2524.vcd" 
	scene "scenes/Player/Scout/low/2709.vcd" 
	scene "scenes/Player/Scout/low/2521.vcd" 
	scene "scenes/Player/Scout/low/2523.vcd" 
}
Rule CartMovingBackwardsOffenseScout
{
	criteria ConceptCartMovingBackward IsOnOffense IsScout ScoutNotSaidCartMovingBackwardO IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingBackwardO:1:20"
	Response CartMovingBackwardsOffenseScout
}

Response CartMovingForwardDefenseScout
{
	scene "scenes/Player/Scout/low/2525.vcd" 
	scene "scenes/Player/Scout/low/2526.vcd" 
	scene "scenes/Player/Scout/low/2527.vcd" 
	scene "scenes/Player/Scout/low/2528.vcd" 
	scene "scenes/Player/Scout/low/2529.vcd" 
	scene "scenes/Player/Scout/low/2530.vcd" 
}
Rule CartMovingForwardDefenseScout
{
	criteria ConceptCartMovingForward IsOnDefense IsScout ScoutNotSaidCartMovingForwardD IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingForwardD:1:20"
	Response CartMovingForwardDefenseScout
}

Response CartMovingForwardOffenseScout
{
	scene "scenes/Player/Scout/low/2532.vcd" 
	scene "scenes/Player/Scout/low/2533.vcd" 
	scene "scenes/Player/Scout/low/2534.vcd" 
	scene "scenes/Player/Scout/low/2535.vcd" 
	scene "scenes/Player/Scout/low/2537.vcd" 
	scene "scenes/Player/Scout/low/2536.vcd" 
	scene "scenes/Player/Scout/low/2538.vcd" 
	scene "scenes/Player/Scout/low/2540.vcd" 
	scene "scenes/Player/Scout/low/2541.vcd" 
	scene "scenes/Player/Scout/low/2539.vcd" 
	scene "scenes/Player/Scout/low/2542.vcd" 
	scene "scenes/Player/Scout/low/2543.vcd" 
}
Rule CartMovingForwardOffenseScout
{
	criteria ConceptCartMovingForward IsOnOffense IsScout ScoutNotSaidCartMovingForwardO IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingForwardO:1:20"
	Response CartMovingForwardOffenseScout
}

Response CartMovingStoppedDefenseScout
{
	scene "scenes/Player/Scout/low/2544.vcd" 
	scene "scenes/Player/Scout/low/2545.vcd" 
	scene "scenes/Player/Scout/low/2546.vcd" 
}
Rule CartMovingStoppedDefenseScout
{
	criteria ConceptCartMovingStopped IsOnDefense IsScout ScoutNotSaidCartMovingStoppedD IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingStoppedD:1:20"
	Response CartMovingStoppedDefenseScout
}

Response CartMovingStoppedOffenseScout
{
	scene "scenes/Player/Scout/low/2548.vcd" 
	scene "scenes/Player/Scout/low/2549.vcd" 
	scene "scenes/Player/Scout/low/2550.vcd" 
}
Rule CartMovingStoppedOffenseScout
{
	criteria ConceptCartMovingStopped IsOnOffense IsScout ScoutNotSaidCartMovingStoppedO IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingStoppedO:1:20"
	Response CartMovingStoppedOffenseScout
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Combat
//--------------------------------------------------------------------------------------------------------------
Response DefendOnThePointScout
{
	scene "scenes/Player/Scout/low/489.vcd" 
	scene "scenes/Player/Scout/low/1305.vcd" 
}
Rule DefendOnThePointScout
{
	criteria ConceptFireWeapon IsScout IsOnFriendlyControlPoint NotDefendOnThePointSpeech
	ApplyContext "DefendOnThePointSpeech:1:30"
	applycontexttoworld
	Response DefendOnThePointScout
}

// Custom stuff
Response InvulnerableSpeechScout
{
	scene "scenes/Player/Scout/low/486.vcd" 
	scene "scenes/Player/Scout/low/491.vcd" 
	scene "scenes/Player/Scout/low/2505.vcd" 
	scene "scenes/Player/Scout/low/499.vcd" 
}
Rule InvulnerableSpeechScout
{
	criteria ConceptFireWeapon IsScout IsInvulnerable ScoutNotInvulnerableSpeech
	ApplyContext "ScoutInvulnerableSpeech:1:30"
	Response InvulnerableSpeechScout
}

Response KilledPlayerAssistAutoScout
{
	scene "scenes/Player/Scout/low/487.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/488.vcd" predelay "2.5"
}
Rule KilledPlayerAssistAutoScout
{
	criteria ConceptKilledPlayer IsScout IsBeingHealed IsManyRecentKills KilledPlayerDelay 20PercentChance ScoutNotAssistSpeech
	ApplyContext "ScoutAssistSpeech:1:20"
	Response KilledPlayerAssistAutoScout
}
// End custom stuff


Response KilledPlayerManyScout
{
	scene "scenes/Player/Scout/low/396.vcd" 
	scene "scenes/Player/Scout/low/1297.vcd" 
	scene "scenes/Player/Scout/low/432.vcd" 
	scene "scenes/Player/Scout/low/433.vcd" 
	scene "scenes/Player/Scout/low/1295.vcd" 
	scene "scenes/Player/Scout/low/1296.vcd" 
	scene "scenes/Player/Scout/low/434.vcd" 
	scene "scenes/Player/Scout/low/435.vcd" 
	scene "scenes/Player/Scout/low/493.vcd" 
	scene "scenes/Player/Scout/low/506.vcd" 
}
Rule KilledPlayerManyScout
{
	criteria ConceptKilledPlayer IsManyRecentKills 30PercentChance IsWeaponPrimary KilledPlayerDelay ScoutNotKillSpeech IsScout
	ApplyContext "ScoutKillSpeech:1:5"
	Response KilledPlayerManyScout
}

// Custom modified stuff
// Modified to split into groups
// Baseball bats use generic and bat lines
// Candy, Basher, Fish and Mace will use their own lines in addition to the generic lines

Response KilledPlayerMeleeBatScout
{
	scene "scenes/Player/Scout/low/479.vcd" 
	scene "scenes/Player/Scout/low/482.vcd" 
	scene "scenes/Player/Scout/low/481.vcd" 
	scene "scenes/Player/Scout/low/483.vcd" 
	scene "scenes/Player/Scout/low/501.vcd" 
}

Response KilledPlayerMeleeGenericScout
{
	scene "scenes/Player/Scout/low/476.vcd" 
	scene "scenes/Player/Scout/low/477.vcd" 
	scene "scenes/Player/Scout/low/484.vcd" 
	scene "scenes/Player/Scout/low/498.vcd" 
}

// This rule excludes all weapons that are not the Sandman or vanilla bat

Rule KilledPlayerMeleeBatScout
{
	criteria ConceptKilledPlayer KilledPlayerDelay 30PercentChance  IsWeaponMelee WeaponIsNotCandy WeaponIsNotBasher WeaponIsNotGunbai WeaponIsNotMace WeaponIsNotFish WeaponIsNotTRBlade WeaponIsNotSaxxy ScoutNotKillSpeechMelee IsScout
	ApplyContext "ScoutKillSpeechMelee:1:10"
	Response KilledPlayerMeleeBatScout
	Response KilledPlayerMeleeGenericScout
}

Response KilledPlayerMeleeBasherScout
{
	scene "scenes/player/Scout/low/2586.vcd"
	scene "scenes/player/Scout/low/2643.vcd"
}

Rule KilledPlayerMeleeBasherScout
{
	criteria ConceptKilledPlayer KilledPlayerDelay 30PercentChance IsWeaponMelee WeaponIsBasher ScoutNotKillSpeechMelee IsScout
	ApplyContext "ScoutKillSpeechMelee:1:10"
	Response KilledPlayerMeleeBasherScout
	Response KilledPlayerMeleeGenericScout
}

Response KilledPlayerMeleeCandyScout
{
	scene "scenes/player/Scout/low/377.vcd"
	scene "scenes/player/Scout/low/503.vcd"
	scene "scenes/player/Scout/low/505.vcd"
}

Rule KilledPlayerMeleeCandyScout
{
	criteria ConceptKilledPlayer KilledPlayerDelay 30PercentChance IsWeaponMelee WeaponIsCandy ScoutNotKillSpeechMelee IsScout
	ApplyContext "ScoutKillSpeechMelee:1:10"
	Response KilledPlayerMeleeCandyScout
	Response KilledPlayerMeleeGenericScout
}

Response KilledPlayerMeleeMaceScout
{
	scene "scenes/player/Scout/low/2560.vcd"
	scene "scenes/player/Scout/low/1289.vcd"
	scene "scenes/player/Scout/low/2706.vcd"
}

Rule KilledPlayerMeleeMaceScout
{
	criteria ConceptKilledPlayer KilledPlayerDelay 30PercentChance  IsWeaponMelee WeaponIsMace ScoutNotKillSpeechMelee IsScout
	ApplyContext "ScoutKillSpeechMelee:1:10"
	Response KilledPlayerMeleeMaceScout
	Response KilledPlayerMeleeGenericScout
}

Response KilledPlayerMeleeFishScout
{
	scene "scenes/player/Scout/low/433.vcd"
	scene "scenes/player/Scout/low/435.vcd"
	scene "scenes/player/Scout/low/1308.vcd"
	scene "scenes/player/Scout/low/2554.vcd"
}

Rule KilledPlayerMeleeFishScout
{
	criteria ConceptKilledPlayer KilledPlayerDelay 30PercentChance  IsWeaponMelee WeaponIsHolyMackerel ScoutNotKillSpeechMelee IsScout
	ApplyContext "ScoutKillSpeechMelee:1:10"
	Response KilledPlayerMeleeFishScout
	Response KilledPlayerMeleeGenericScout
}

Rule KilledPlayerMeleeFestiveFishScout
{
	criteria ConceptKilledPlayer KilledPlayerDelay 30PercentChance  IsWeaponMelee WeaponIsFestiveHolyMackerel ScoutNotKillSpeechMelee IsScout
	ApplyContext "ScoutKillSpeechMelee:1:10"
	Response KilledPlayerMeleeFishScout
	Response KilledPlayerMeleeGenericScout
}

// A rule for the Scout Gunbai
// it will share with the Fish as getting killed by this must be very humiliating
// even moreso than the Fish
Rule KilledPlayerMeleeGunbaiScout
{
	criteria ConceptKilledPlayer KilledPlayerDelay 30PercentChance  IsWeaponMelee WeaponIsGunbai ScoutNotKillSpeechMelee IsScout
	ApplyContext "ScoutKillSpeechMelee:1:10"
	Response KilledPlayerMeleeFishScout
	Response KilledPlayerMeleeGenericScout
}

Rule KilledPlayerMeleeSwordScout
{
	criteria ConceptKilledPlayer KilledPlayerDelay 30PercentChance  IsWeaponMelee WeaponIsTRBlade ScoutNotKillSpeechMelee IsScout
	ApplyContext "ScoutKillSpeechMelee:1:10"
	Response KilledPlayerMeleeMaceScout
	Response KilledPlayerMeleeGenericScout
}


Rule KilledPlayerMeleeSaxxyScout
{
	criteria ConceptKilledPlayer KilledPlayerDelay 30PercentChance  IsWeaponMelee WeaponIsSaxxy ScoutNotKillSpeechMelee IsScout
	ApplyContext "ScoutKillSpeechMelee:1:10"
	Response KilledPlayerMeleeFishScout
	Response KilledPlayerMeleeGenericScout
}

Response KilledPlayerMeleeScoutFatScout
{
	scene "scenes/Player/Scout/low/475.vcd" 
}
Rule KilledPlayerMeleeScoutFatScout
{
	criteria ConceptKilledPlayer KilledPlayerDelay 75PercentChance  IsWeaponMelee ScoutNotKillSpeechMeleeFat IsScout IsVictimHeavy
	ApplyContext "ScoutKillSpeechMeleeFat:1:10"
	Response KilledPlayerMeleeScoutFatScout
}

Response MedicFollowScout
{
	scene "scenes/Player/Scout/low/2574.vcd" predelay ".25"
	scene "scenes/Player/Scout/low/2575.vcd" predelay ".25"
	scene "scenes/Player/Scout/low/2577.vcd" predelay ".25"
	scene "scenes/Player/Scout/low/2578.vcd" predelay ".25"
}
Rule MedicFollowScout
{
	criteria ConceptPlayerMedic IsOnMedic IsScout IsNotCrossHairEnemy NotLowHealth ScoutIsNotStillonFire
	ApplyContext "ScoutKillSpeech:1:10"
	Response MedicFollowScout
}

Response ScoutJarateHit
{
	scene "scenes/Player/Scout/low/364.vcd"  
	scene "scenes/Player/Scout/low/426.vcd"  
	scene "scenes/Player/Scout/low/450.vcd"  
	scene "scenes/Player/Scout/low/451.vcd"  
}
Rule ScoutJarateHit
{
	criteria ConceptJarateHit IsScout 50PercentChance
	Response ScoutJarateHit
}

Response PlayerBeingShotInvincibleScout
{
	scene "scenes/Player/Scout/low/2632.vcd" 
	scene "scenes/Player/Scout/low/2631.vcd" 
	scene "scenes/Player/Scout/low/2636.vcd" 
	scene "scenes/Player/Scout/low/2629.vcd" 
	scene "scenes/Player/Scout/low/2690.vcd" 
	scene "scenes/Player/Scout/low/2638.vcd" 
	scene "scenes/Player/Scout/low/2630.vcd" 
	scene "scenes/Player/Scout/low/2634.vcd" 
	scene "scenes/Player/Scout/low/2635.vcd" 
	scene "scenes/Player/Scout/low/2637.vcd" 
	scene "scenes/Player/Scout/low/2737.vcd" 
	scene "scenes/Player/Scout/low/2738.vcd" 
	scene "scenes/Player/Scout/low/2739.vcd" 
	scene "scenes/Player/Scout/low/2740.vcd" 
	scene "scenes/Player/Scout/low/2741.vcd" 
	scene "scenes/Player/Scout/low/2742.vcd" 
	scene "scenes/Player/Scout/low/2743.vcd" 
	scene "scenes/Player/Scout/low/2744.vcd" 
	scene "scenes/Player/Scout/low/2745.vcd" 
	scene "scenes/Player/Scout/low/2746.vcd" 
	scene "scenes/Player/Scout/low/2747.vcd" 
	scene "scenes/Player/Scout/low/2748.vcd" 
	scene "scenes/Player/Scout/low/2749.vcd" 
	scene "scenes/Player/Scout/low/2750.vcd" 
	scene "scenes/Player/Scout/low/2751.vcd" 
	scene "scenes/Player/Scout/low/2752.vcd" 
	scene "scenes/Player/Scout/low/2753.vcd" 
	scene "scenes/Player/Scout/low/2754.vcd" 
	scene "scenes/Player/Scout/low/2755.vcd" 
	scene "scenes/Player/Scout/low/2756.vcd" 
	scene "scenes/Player/Scout/low/2757.vcd" 
	scene "scenes/Player/Scout/low/2758.vcd" 
	scene "scenes/Player/Scout/low/2759.vcd" 
	scene "scenes/Player/Scout/low/2760.vcd" 
	scene "scenes/Player/Scout/low/2761.vcd" 
	scene "scenes/Player/Scout/low/2762.vcd" 
	scene "scenes/Player/Scout/low/2728.vcd" 
	scene "scenes/Player/Scout/low/2771.vcd" 
	scene "scenes/Player/Scout/low/2729.vcd"
	scene "scenes/Player/Scout/low/2730.vcd"
}
Rule PlayerBeingShotInvincibleScout
{
	criteria ConceptDodgeShot IsScout LoadoutIsDrink // Exclude Crit-a-Cola
	Response PlayerBeingShotInvincibleScout
}

Response PlayerDodgingScout
{
	scene "scenes/Player/Scout/low/2505.vcd" predelay "2,3"
	scene "scenes/Player/Scout/low/2615.vcd" predelay "2,3"
	scene "scenes/Player/Scout/low/2616.vcd" predelay "2,3"
	scene "scenes/Player/Scout/low/2628.vcd" predelay "2,3"
	scene "scenes/Player/Scout/low/2621.vcd" predelay "2,3"
}
Rule PlayerDodgingScout
{
	criteria ConceptDodging IsScout ScoutIsNotInvuln LoadoutIsDrink // Exclude Crit-a-Cola
	ApplyContext "ScoutInvuln:1:20"
	Response PlayerDodgingScout
}

// Custom Stuff
Response PlayerDoubleJumpScout
{
	scene "scenes/Player/Scout/low/2608.vcd" 
	scene "scenes/Player/Scout/low/2610.vcd" 
	scene "scenes/Player/Scout/low/2609.vcd" 
	scene "scenes/Player/Scout/low/2611.vcd" 
}
Rule PlayerDoubleJumpScout
{
	criteria ConceptFireWeapon IsScout IsDoubleJumping WeaponIsScattergunDouble 20PercentChance
	Response PlayerDoubleJumpScout
}
Rule PlayerDoubleJumpScoutFestive
{
	criteria ConceptFireWeapon IsScout IsDoubleJumping WeaponIsScattergunDoubleFestive 20PercentChance
	Response PlayerDoubleJumpScout
}

// Double jump response
Response DoubleJumpScout
{
	scene "scenes/Player/Scout/low/2624.vcd" 
	scene "scenes/Player/Scout/low/2625.vcd" 
	scene "scenes/Player/Scout/low/2627.vcd" 
	scene "scenes/Player/Scout/low/2685.vcd" 
	scene "scenes/Player/Scout/low/2689.vcd" 
}
Rule DoubleJumpScout
{
	criteria ConceptDoubleJump IsScout IsARecentKill IsNotDoubleJumping WeaponIsNotScattergunDouble ScoutHasFired ScoutNotDoubleJumpSpeech 2PercentChance
	ApplyContext "ScoutDoubleJumpSpeech:1:90"
	Response DoubleJumpScout
}

// Invincible not ready
Response DrinkNotReady
{
	scene "scenes/player/Scout/low/2732.vcd"
	scene "scenes/player/Scout/low/2733.vcd"
	scene "scenes/player/Scout/low/2734.vcd"
	scene "scenes/player/Scout/low/2774.vcd"
	scene "scenes/player/Scout/low/2775.vcd"
	scene "scenes/player/Scout/low/2776.vcd"
}
Rule DrinkNotReady
{
	criteria ConceptPain IsScout WeaponIsLunchboxDrink BonkHealthContext ScoutNotDrinkReadySpeech LoadoutIsDrink // Exclude Crit-a-Cola
	ApplyContext "ScoutDrinkReadySpeech:1:5"
	Response DrinkNotReady
}

// Crit-a-Cola lines
//
// Explanation:
//
// When the player drinks the cola, a context called ScoutIsCrit is set to 1 for 3-ish seconds in tf.txt (saves having the rule duped here)
// This is then picked up by the Rule PlayerCritColaVocalScout, which checks if the player has fired their weapon.
// If they have fired, then the rule checks the ScoutIsNotCrit, which has just been set to 1 during the drinking
// of the cola. Therefore, if the player fires during this period, they will say the response.
//
// We then changed the PostTired response for the cola for teh lulz.
//
// This is a tricky workaround, but necessary as we cannot 'layer' vcds (i.e.  play one vcd on top of the other)

// These are the reesponses we play when firing under the effects.
Response PlayerCritColaVocalScout
{
	scene "scenes/player/Scout/low/507.vcd"
	scene "scenes/player/Scout/low/2510.vcd"
	scene "scenes/player/Scout/low/396.vcd"
}
Rule PlayerCritColaVocalScout
{
	criteria ConceptFireWeapon IsScout LoadoutIsCritDrink ScoutIsNotCrit // The crit context is read in here via this criterion, which is set at the top of the file
	Response PlayerCritColaVocalScout
}

// Here we alter what he says after the effects finish.
// Note this is 50PercentChance, so he has 50% chance of just doing the normal tired breathing.
Response PostCritScout
{
	scene "scenes/player/Scout/low/2734.vcd"
	scene "scenes/player/Scout/low/2774.vcd"
	scene "scenes/player/Scout/low/2775.vcd"
	scene "scenes/player/Scout/low/2776.vcd"
}

Rule PostCritScout
{
	criteria ConceptTired IsScout 50PercentChance LoadoutIsCritDrink
	Response PostCritScout
}

// Milk toss
Response MilkLaunchScout
{
	scene "scenes/player/Scout/low/504.vcd"
	scene "scenes/player/Scout/low/2705.vcd"
	scene "scenes/player/Scout/low/2604.vcd"
}
Rule MilkLaunchScout
{
	criteria ConceptJarateLaunch IsScout 50PercentChance
	Response MilkLaunchScout
}

// End custom

Response PlayerKilledCapperScout
{
	scene "scenes/Player/Scout/low/359.vcd" 
	scene "scenes/Player/Scout/low/372.vcd" 
	scene "scenes/Player/Scout/low/373.vcd" 
	scene "scenes/Player/Scout/low/454.vcd" 
	scene "scenes/Player/Scout/low/488.vcd" 
	scene "scenes/Player/Scout/low/491.vcd" 
	scene "scenes/Player/Scout/low/499.vcd" 
	scene "scenes/Player/Scout/low/1308.vcd" 
}
Rule PlayerKilledCapperScout
{
	criteria ConceptCapBlocked IsScout
	ApplyContext "ScoutKillSpeech:1:10"
	Response PlayerKilledCapperScout
}

// Custom stuff
// The other 2 unimplemented dominations do not have respective vcds.
// Will build them sometime later.
Response PlayerKilledDominatingScout
{
	scene "scenes/Player/Scout/low/2551.vcd" predelay "2.5"
	scene "scenes/player/Scout/low/2687.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2764.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2765.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingScout
{
	criteria ConceptKilledPlayer IsScout IsDominated 30PercentChance
	ApplyContext "ScoutKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingScout
}

Response PlayerKilledDominatingBatScout
{
	scene "scenes/player/scout/low/2648.vcd" predelay "2.5"
	scene "scenes/player/scout/low/2643.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingBatScout
{
	criteria ConceptKilledPlayer IsScout IsDominated IsWeaponMelee
	ApplyContext "ScoutKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingBatScout
}
//End custom

Response PlayerKilledDominatingDemomanScout
{
	scene "scenes/Player/Scout/low/2763.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2649.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2564.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2565.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2679.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2680.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2678.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingDemomanScout
{
	criteria ConceptKilledPlayer IsScout IsDominated  IsVictimDemoman
	ApplyContext "ScoutKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingDemomanScout
}

Response PlayerKilledDominatingEngineerScout
{
	scene "scenes/Player/Scout/low/2766.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2556.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2675.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2676.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2677.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2710.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2711.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingEngineerScout
{
	criteria ConceptKilledPlayer IsScout IsDominated  IsVictimEngineer
	ApplyContext "ScoutKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingEngineerScout
}

Response PlayerKilledDominatingHeavyScout
{
	scene "scenes/Player/Scout/low/2553.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2646.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2647.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2557.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2558.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2655.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2656.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2657.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2659.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2660.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2693.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2712.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2713.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2642.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingHeavyScout
{
	criteria ConceptKilledPlayer IsScout IsDominated  IsVictimHeavy
	ApplyContext "ScoutKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingHeavyScout
}

Response PlayerKilledDominatingMedicScout
{
	scene "scenes/Player/Scout/low/2714.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2715.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2716.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2736.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2665.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2559.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2641.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingMedicScout
{
	criteria ConceptKilledPlayer IsScout IsDominated  IsVictimMedic
	ApplyContext "ScoutKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingMedicScout
}

Response PlayerKilledDominatingPyroScout
{
	scene "scenes/player/Scout/low/2688.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2644.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2560.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2673.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2674.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2725.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2726.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2718.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingPyroScout
{
	criteria ConceptKilledPlayer IsScout IsDominated  IsVictimPyro
	ApplyContext "ScoutKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingPyroScout
}

Response PlayerKilledDominatingScoutScout
{
	scene "scenes/Player/Scout/low/2555.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2645.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2691.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2719.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2554.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2654.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2768.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2566.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2666.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2667.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingScoutScout
{
	criteria ConceptKilledPlayer IsScout IsDominated  IsVictimScout
	ApplyContext "ScoutKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingScoutScout
}

Response PlayerKilledDominatingSniperScout
{
	scene "scenes/Player/Scout/low/2686.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2567.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2668.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2672.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2769.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2770.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingSniperScout
{
	criteria ConceptKilledPlayer IsScout IsDominated  IsVictimSniper
	ApplyContext "ScoutKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingSniperScout
}

Response PlayerKilledDominatingSoldierScout
{
	scene "scenes/Player/Scout/low/2562.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2669.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2670.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2671.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2720.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2721.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingSoldierScout
{
	criteria ConceptKilledPlayer IsScout IsDominated  IsVictimSoldier
	ApplyContext "ScoutKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingSoldierScout
}

Response PlayerKilledDominatingSpyScout
{
	scene "scenes/Player/Scout/low/2682.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2552.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2563.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2661.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2662.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2663.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingSpyScout
{
	criteria ConceptKilledPlayer IsScout IsDominated  IsVictimSpy
	ApplyContext "ScoutKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingSpyScout
}

Response PlayerKilledForRevengeScout
{
	scene "scenes/Player/Scout/low/2681.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/374.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2641.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2706.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/453.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2586.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2587.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2588.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2589.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2590.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2591.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2592.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2593.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/2594.vcd" predelay "2.5"
	scene "scenes/Player/Scout/low/486.vcd" predelay "2.5"
}
Rule PlayerKilledForRevengeScout
{
	criteria ConceptKilledPlayer IsScout IsRevenge
	ApplyContext "ScoutKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledForRevengeScout
}

Response PlayerKilledObjectScout
{
	scene "scenes/Player/Scout/low/485.vcd" 
}
Rule PlayerKilledObjectScout
{
	criteria ConceptKilledObject IsScout 30PercentChance IsARecentKill
	ApplyContext "ScoutKillSpeechObject:1:30"
	Response PlayerKilledObjectScout
}

Response PlayerStunBallHitScout
{
	scene "scenes/Player/Scout/low/2767.vcd" 
	scene "scenes/Player/Scout/low/2597.vcd" 
	scene "scenes/Player/Scout/low/2598.vcd" 
	scene "scenes/Player/Scout/low/2599.vcd" 
	scene "scenes/Player/Scout/low/2600.vcd" 
	scene "scenes/Player/Scout/low/2601.vcd" 
	scene "scenes/Player/Scout/low/2602.vcd" 
	scene "scenes/Player/Scout/low/2684.vcd" 
	scene "scenes/Player/Scout/low/2694.vcd" 
	scene "scenes/Player/Scout/low/2695.vcd" 
	scene "scenes/Player/Scout/low/2700.vcd" 
	scene "scenes/Player/Scout/low/2702.vcd" 
	scene "scenes/Player/Scout/low/2705.vcd" 
	scene "scenes/Player/Scout/low/2707.vcd" 
	scene "scenes/Player/Scout/low/2697.vcd" 
	scene "scenes/Player/Scout/low/2701.vcd" 
}
Rule PlayerStunBallHitScout
{
	criteria ConceptStunnedTarget IsScout NotSaidScoutHitBallSpeech 50PercentChance
	Response PlayerStunBallHitScout
}

Response PlayerStunBallHittingItScout
{
	scene "scenes/Player/Scout/low/2606.vcd" 
	scene "scenes/Player/Scout/low/2604.vcd" 
	scene "scenes/Player/Scout/low/2651.vcd" 
	scene "scenes/Player/Scout/low/2703.vcd" 
}
Rule PlayerStunBallHittingItScout
{
	criteria ConceptFireWeapon IsScout WeaponIsWoodBat 10PercentChance
	ApplyContext "ScoutHitBallSpeech:1:10"
	Response PlayerStunBallHittingItScout
}

Response PlayerStunBallPickUpScout
{
	scene "scenes/Player/Scout/low/2722.vcd" 
	scene "scenes/Player/Scout/low/2723.vcd" 
	scene "scenes/Player/Scout/low/2724.vcd" 
	scene "scenes/Player/Scout/low/2772.vcd" 
	scene "scenes/Player/Scout/low/2773.vcd" 
}
Rule PlayerStunBallPickUpScout
{
	criteria ConceptScoutBallGrab IsScout 50PercentChance
	Response PlayerStunBallPickUpScout
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Pain
//--------------------------------------------------------------------------------------------------------------
Response PlayerAttackerPainScout
{
	scene "scenes/Player/Scout/low/461.vcd" 
	scene "scenes/Player/Scout/low/462.vcd" 
	scene "scenes/Player/Scout/low/463.vcd" 
	scene "scenes/Player/Scout/low/1361.vcd" 
	scene "scenes/Player/Scout/low/1362.vcd" 
	scene "scenes/Player/Scout/low/1363.vcd" 
}
Rule PlayerAttackerPainScout
{
	criteria ConceptAttackerPain IsScout IsNotDominating
	Response PlayerAttackerPainScout
}

Response PlayerOnFireScout
{
	scene "scenes/Player/Scout/low/365.vcd" 
}
Rule PlayerOnFireScout
{
	criteria ConceptFire IsScout ScoutIsNotStillonFire IsNotDominating
	ApplyContext "ScoutOnFire:1:7"
	Response PlayerOnFireScout
}

Response PlayerOnFireRareScout
{
	scene "scenes/Player/Scout/low/366.vcd" 
}
Rule PlayerOnFireRareScout
{
	criteria ConceptFire IsScout 10PercentChance ScoutIsNotStillonFire IsNotDominating
	ApplyContext "ScoutOnFire:1:7"
	Response PlayerOnFireRareScout
}

Response PlayerPainScout
{
	scene "scenes/Player/Scout/low/464.vcd" 
	scene "scenes/Player/Scout/low/465.vcd" 
	scene "scenes/Player/Scout/low/466.vcd" 
	scene "scenes/Player/Scout/low/1364.vcd" 
	scene "scenes/Player/Scout/low/1365.vcd" 
	scene "scenes/Player/Scout/low/1366.vcd" 
	scene "scenes/Player/Scout/low/1367.vcd" 
	scene "scenes/Player/Scout/low/1368.vcd" 
}
Rule PlayerPainScout
{
	criteria ConceptPain IsScout IsNotDominating
	Response PlayerPainScout
}

Response PlayerStillOnFireScout
{
	scene "scenes/Player/Scout/low/1932.vcd" 
}
Rule PlayerStillOnFireScout
{
	criteria ConceptFire IsScout  ScoutIsStillonFire IsNotDominating
	ApplyContext "ScoutOnFire:1:7"
	Response PlayerStillOnFireScout
}


//--------------------------------------------------------------------------------------------------------------
// Duel Speech
//--------------------------------------------------------------------------------------------------------------
Response AcceptedDuelScout
{
	scene "scenes/Player/Scout/low/377.vcd" 
	scene "scenes/Player/Scout/low/491.vcd" 
	scene "scenes/Player/Scout/low/494.vcd" 
	scene "scenes/Player/Scout/low/496.vcd" 
	scene "scenes/Player/Scout/low/516.vcd" 
}
Rule AcceptedDuelScout
{
	criteria ConceptIAcceptDuel IsScout
	Response AcceptedDuelScout
}

Response MeleeDareScout
{
	scene "scenes/Player/Scout/low/2579.vcd" 
	scene "scenes/Player/Scout/low/2580.vcd" 
	scene "scenes/Player/Scout/low/2582.vcd" 
	scene "scenes/Player/Scout/low/2583.vcd" 
	scene "scenes/Player/Scout/low/2581.vcd" 
}
Rule MeleeDareScout
{
	criteria ConceptRequestDuel IsScout
	Response MeleeDareScout
}

Response RejectedDuelScout
{
	scene "scenes/Player/Scout/low/362.vcd" 
	scene "scenes/Player/Scout/low/1281.vcd" 
	scene "scenes/Player/Scout/low/363.vcd" 
	scene "scenes/Player/Scout/low/364.vcd" 
	scene "scenes/Player/Scout/low/2554.vcd" 
	scene "scenes/Player/Scout/low/2768.vcd" 
	scene "scenes/Player/Scout/low/419.vcd" 
	scene "scenes/Player/Scout/low/421.vcd" 
	scene "scenes/Player/Scout/low/2706.vcd" 
}
Rule RejectedDuelScout
{
	criteria ConceptDuelRejected IsScout
	Response RejectedDuelScout
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 1
//--------------------------------------------------------------------------------------------------------------
Response PlayerGoScout
{
	scene "scenes/Player/Scout/low/393.vcd" 
	scene "scenes/Player/Scout/low/1286.vcd" 
	scene "scenes/Player/Scout/low/392.vcd" 
	scene "scenes/Player/Scout/low/394.vcd" 
}
Rule PlayerGoScout
{
	criteria ConceptPlayerGo IsScout
	Response PlayerGoScout
}

Response PlayerHeadLeftScout
{
	scene "scenes/Player/Scout/low/398.vcd" 
	scene "scenes/Player/Scout/low/399.vcd" 
	scene "scenes/Player/Scout/low/400.vcd" 
}
Rule PlayerHeadLeftScout
{
	criteria ConceptPlayerLeft  IsScout
	Response PlayerHeadLeftScout
}

Response PlayerHeadRightScout
{
	scene "scenes/Player/Scout/low/401.vcd" 
	scene "scenes/Player/Scout/low/402.vcd" 
	scene "scenes/Player/Scout/low/403.vcd" 
}
Rule PlayerHeadRightScout
{
	criteria ConceptPlayerRight  IsScout
	Response PlayerHeadRightScout
}

Response PlayerHelpScout
{
	scene "scenes/Player/Scout/low/404.vcd" 
	scene "scenes/Player/Scout/low/1290.vcd" 
	scene "scenes/Player/Scout/low/405.vcd" 
	scene "scenes/Player/Scout/low/406.vcd" 
}
Rule PlayerHelpScout
{
	criteria ConceptPlayerHelp IsScout
	Response PlayerHelpScout
}

Response PlayerHelpCaptureScout
{
	scene "scenes/Player/Scout/low/407.vcd" 
	scene "scenes/Player/Scout/low/408.vcd" 
	scene "scenes/Player/Scout/low/409.vcd" 
}
Rule PlayerHelpCaptureScout
{
	criteria ConceptPlayerHelp IsScout IsOnCappableControlPoint
	ApplyContext "ScoutHelpCap:1:10"
	Response PlayerHelpCaptureScout
}

Response PlayerHelpCapture2Scout
{
	scene "scenes/Player/Scout/low/489.vcd" 
	scene "scenes/Player/Scout/low/1304.vcd" 
	scene "scenes/Player/Scout/low/1305.vcd" 
	scene "scenes/Player/Scout/low/1306.vcd" 
	scene "scenes/Player/Scout/low/1307.vcd" 
}
Rule PlayerHelpCapture2Scout
{
	criteria ConceptPlayerHelp IsScout IsOnCappableControlPoint IsHelpCapScout
	Response PlayerHelpCapture2Scout
}

// Custom stuff
// Response for when the Scout is fighting on a cappable point
Response PlayerGetOnPointScout
{
	scene "scenes/Player/Scout/low/2568.vcd" 
	scene "scenes/Player/Scout/low/2569.vcd" 
	scene "scenes/Player/Scout/low/2570.vcd" 
	scene "scenes/Player/Scout/low/2572.vcd" 
}

Rule PlayerGetOnPointScout
{
	criterion ConceptFireWeapon IsScout IsOnCappableControlPoint NotDefendOnThePointSpeech
	ApplyContext "DefendOnThePointSpeech:1:15"
	applycontexttoworld
	Response PlayerGetOnPointScout
}
// End custom

Response PlayerHelpDefendScout
{
	scene "scenes/Player/Scout/low/410.vcd" 
	scene "scenes/Player/Scout/low/411.vcd" 
	scene "scenes/Player/Scout/low/412.vcd" 
}
Rule PlayerHelpDefendScout
{
	criteria ConceptPlayerHelp IsScout IsOnFriendlyControlPoint
	Response PlayerHelpDefendScout
}

Response PlayerMedicScout
{
	scene "scenes/Player/Scout/low/438.vcd" 
	scene "scenes/Player/Scout/low/439.vcd" 
	scene "scenes/Player/Scout/low/440.vcd" 
}
Rule PlayerMedicScout
{
	criteria ConceptPlayerMedic IsScout
	Response PlayerMedicScout
}

Response PlayerAskForBallScout
{
}
Rule PlayerAskForBallScout
{
	criteria ConceptPlayerAskForBall IsScout
	Response PlayerAskForBallScout
}

Response PlayerMoveUpScout
{
	scene "scenes/Player/Scout/low/441.vcd" 
	scene "scenes/Player/Scout/low/442.vcd" 
	scene "scenes/Player/Scout/low/443.vcd" 
}
Rule PlayerMoveUpScout
{
	criteria ConceptPlayerMoveUp  IsScout
	Response PlayerMoveUpScout
}

Response PlayerNoScout
{
	scene "scenes/Player/Scout/low/455.vcd" 
	scene "scenes/Player/Scout/low/456.vcd" 
	scene "scenes/Player/Scout/low/457.vcd" 
}
Rule PlayerNoScout
{
	criteria ConceptPlayerNo  IsScout
	Response PlayerNoScout
}

Response PlayerThanksScout
{
	scene "scenes/Player/Scout/low/508.vcd" 
	scene "scenes/Player/Scout/low/509.vcd" 
}
Rule PlayerThanksScout
{
	criteria ConceptPlayerThanks IsScout
	Response PlayerThanksScout
}

// Custom Assist kill response
// As there is no actual concept for assist kills, this is the second best method.
// Say thanks after you kill more than one person.

Response KilledPlayerAssistScout
{
	scene "scenes/Player/Scout/low/487.vcd"
	scene "scenes/Player/Scout/low/488.vcd"
}
Rule KilledPlayerAssistScout
{
	criteria ConceptPlayerThanks IsScout IsARecentKill KilledPlayerDelay ScoutNotAssistSpeech
	ApplyContext "ScoutAssistSpeech:1:20"
	Response KilledPlayerAssistScout
}
// End custom

Response PlayerYesScout
{
	scene "scenes/Player/Scout/low/516.vcd" 
	scene "scenes/Player/Scout/low/517.vcd" 
	scene "scenes/Player/Scout/low/518.vcd" 
}
Rule PlayerYesScout
{
	criteria ConceptPlayerYes  IsScout
	Response PlayerYesScout
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 2
//--------------------------------------------------------------------------------------------------------------
Response PlayerActivateChargeScout
{
	scene "scenes/Player/Scout/low/353.vcd" 
	scene "scenes/Player/Scout/low/354.vcd" 
	scene "scenes/Player/Scout/low/355.vcd" 
}
Rule PlayerActivateChargeScout
{
	criteria ConceptPlayerActivateCharge IsScout
	Response PlayerActivateChargeScout
}

Response PlayerCloakedSpyScout
{
	scene "scenes/Player/Scout/low/380.vcd" 
	scene "scenes/Player/Scout/low/379.vcd" 
	scene "scenes/Player/Scout/low/1285.vcd" 
	scene "scenes/Player/Scout/low/381.vcd" 
}
Rule PlayerCloakedSpyScout
{
	criteria ConceptPlayerCloakedSpy IsScout
	Response PlayerCloakedSpyScout
}

Response PlayerDispenserHereScout
{
	scene "scenes/Player/Scout/low/444.vcd" 
}
Rule PlayerDispenserHereScout
{
	criteria ConceptPlayerDispenserHere IsScout
	Response PlayerDispenserHereScout
}

Response PlayerIncomingScout
{
	scene "scenes/Player/Scout/low/413.vcd" 
	scene "scenes/Player/Scout/low/414.vcd" 
	scene "scenes/Player/Scout/low/415.vcd" 
}
Rule PlayerIncomingScout
{
	criteria ConceptPlayerIncoming IsScout
	Response PlayerIncomingScout
}

Response PlayerSentryAheadScout
{
	scene "scenes/Player/Scout/low/472.vcd" 
	scene "scenes/Player/Scout/low/473.vcd" 
	scene "scenes/Player/Scout/low/474.vcd" 
}
Rule PlayerSentryAheadScout
{
	criteria ConceptPlayerSentryAhead IsScout
	Response PlayerSentryAheadScout
}

Response PlayerSentryHereScout
{
	scene "scenes/Player/Scout/low/445.vcd" 
}
Rule PlayerSentryHereScout
{
	criteria ConceptPlayerSentryHere IsScout
	Response PlayerSentryHereScout
}

Response PlayerTeleporterHereScout
{
	scene "scenes/Player/Scout/low/446.vcd" 
}
Rule PlayerTeleporterHereScout
{
	criteria ConceptPlayerTeleporterHere IsScout
	Response PlayerTeleporterHereScout
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 3
//--------------------------------------------------------------------------------------------------------------
Response PlayerBattleCryScout
{
	scene "scenes/Player/Scout/low/367.vcd" 
	scene "scenes/Player/Scout/low/369.vcd" 
	scene "scenes/Player/Scout/low/370.vcd" 
	scene "scenes/Player/Scout/low/371.vcd" 
	scene "scenes/Player/Scout/low/368.vcd" 
}
Rule PlayerBattleCryScout
{
	criteria ConceptPlayerBattleCry IsScout
	Response PlayerBattleCryScout
}

// Custom stuff - melee dare
// Look at enemy, then do battle cry voice command while holding a melee weapon.
Response MeleeDareCombatScout
{
	scene "scenes/Player/Scout/low/2584.vcd"
	scene "scenes/Player/Scout/low/2579.vcd" 
	scene "scenes/Player/Scout/low/2580.vcd" 
	scene "scenes/Player/Scout/low/2582.vcd" 
	scene "scenes/Player/Scout/low/2583.vcd" 
	scene "scenes/Player/Scout/low/2581.vcd" 
	scene "scenes/Player/Scout/low/2642.vcd" 
	scene "scenes/Player/Scout/low/500.vcd" 
	scene "scenes/Player/Scout/low/494.vcd"
	scene "scenes/Player/Scout/low/496.vcd"
}
Rule MeleeDareCombatScout
{
	criteria ConceptPlayerBattleCry IsWeaponMelee IsScout IsCrosshairEnemy
	Response MeleeDareCombatScout
}
//End custom

Response PlayerCheersScout
{
	scene "scenes/Player/Scout/low/372.vcd" 
	scene "scenes/Player/Scout/low/374.vcd" 
	scene "scenes/Player/Scout/low/375.vcd" 
	scene "scenes/Player/Scout/low/377.vcd" 
	scene "scenes/Player/Scout/low/376.vcd" 
	scene "scenes/Player/Scout/low/373.vcd" 
}
Rule PlayerCheersScout
{
	criteria ConceptPlayerCheers IsScout
	Response PlayerCheersScout
}

Response PlayerGoodJobScout
{
	scene "scenes/Player/Scout/low/395.vcd" 
	scene "scenes/Player/Scout/low/396.vcd" 
	scene "scenes/Player/Scout/low/397.vcd" 
	scene "scenes/Player/Scout/low/1288.vcd" 
}
Rule PlayerGoodJobScout
{
	criteria ConceptPlayerGoodJob IsScout
	Response PlayerGoodJobScout
}

Response PlayerJeersScout
{
	scene "scenes/Player/Scout/low/419.vcd" 
	scene "scenes/Player/Scout/low/420.vcd" 
	scene "scenes/Player/Scout/low/421.vcd" 
	scene "scenes/Player/Scout/low/422.vcd" 
	scene "scenes/Player/Scout/low/423.vcd" 
	scene "scenes/Player/Scout/low/424.vcd" 
	scene "scenes/Player/Scout/low/425.vcd" 
	scene "scenes/Player/Scout/low/426.vcd" 
	scene "scenes/Player/Scout/low/427.vcd" 
	scene "scenes/Player/Scout/low/428.vcd" 
	scene "scenes/Player/Scout/low/430.vcd" 
}
Rule PlayerJeersScout
{
	criteria ConceptPlayerJeers IsScout
	Response PlayerJeersScout
}

Response PlayerLostPointScout
{
	scene "scenes/Player/Scout/low/451.vcd" 
	scene "scenes/Player/Scout/low/447.vcd" 
	scene "scenes/Player/Scout/low/449.vcd" 
	scene "scenes/Player/Scout/low/450.vcd" 
	scene "scenes/Player/Scout/low/448.vcd" 
}
Rule PlayerLostPointScout
{
	criteria ConceptPlayerLostPoint IsScout
	Response PlayerLostPointScout
}

Response PlayerNegativeScout
{
	scene "scenes/Player/Scout/low/451.vcd" 
	scene "scenes/Player/Scout/low/447.vcd" 
	scene "scenes/Player/Scout/low/449.vcd" 
	scene "scenes/Player/Scout/low/450.vcd" 
	scene "scenes/Player/Scout/low/448.vcd" 
}
Rule PlayerNegativeScout
{
	criteria ConceptPlayerNegative IsScout
	Response PlayerNegativeScout
}

Response PlayerNiceShotScout
{
	scene "scenes/Player/Scout/low/452.vcd" 
	scene "scenes/Player/Scout/low/453.vcd" 
	scene "scenes/Player/Scout/low/454.vcd" 
}
Rule PlayerNiceShotScout
{
	criteria ConceptPlayerNiceShot IsScout
	Response PlayerNiceShotScout
}

Response PlayerPositiveScout
{
	scene "scenes/Player/Scout/low/1302.vcd" 
	scene "scenes/Player/Scout/low/467.vcd" 
	scene "scenes/Player/Scout/low/469.vcd" 
	scene "scenes/Player/Scout/low/470.vcd" 
	scene "scenes/Player/Scout/low/471.vcd" 
}

Response PlayerTauntsScout
{
	scene "scenes/Player/Scout/low/1298.vcd" 
	scene "scenes/Player/Scout/low/436.vcd" 
	scene "scenes/Player/Scout/low/437.vcd" 
	scene "scenes/Player/Scout/low/1299.vcd" 
	scene "scenes/Player/Scout/low/1300.vcd" 
	scene "scenes/Player/Scout/low/1301.vcd" 
}
Rule PlayerPositiveScout
{
	criteria ConceptPlayerPositive IsScout
	Response PlayerPositiveScout
	Response PlayerTauntsScout
}

//--------------------------------------------------------------------------------------------------------------
// Begin Competitive Mode VO
//--------------------------------------------------------------------------------------------------------------
Response PlayerFirstRoundStartCompScout
{
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_09.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_10.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_11.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_12.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_13.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_14.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_15.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_16.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_17.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_18.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_09.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_10.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_11.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_12.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_13.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_14.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_15.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_16.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_17.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_18.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_19.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_20.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_21.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_09.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_10.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_rare_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_rare_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_rare_03.vcd" predelay "1.0, 5.0"
}
Rule PlayerFirstRoundStartCompScout
{
	criteria ConceptPlayerRoundStartComp IsScout IsFirstRound IsNotComp6v6 40PercentChance
	Response PlayerFirstRoundStartCompScout
}

Response PlayerFirstRoundStartComp6sScout
{
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_09.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_10.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_11.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_12.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_13.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_14.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_15.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_16.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_17.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_18.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_09.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_10.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_11.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_12.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_13.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_14.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_15.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_16.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_17.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_18.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_19.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_20.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_21.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_09.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_rare_10.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_rare_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_rare_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_comp_rare_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_6s_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_6s_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_6s_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_6s_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_6s_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamefirst_6s_rare_01.vcd" predelay "1.0, 5.0"
}
Rule PlayerFirstRoundStartComp6sScout
{
	criteria ConceptPlayerRoundStartComp IsScout IsFirstRound IsComp6v6 40PercentChance
	Response PlayerFirstRoundStartComp6sScout
}

Response PlayerWonPrevRoundCompScout
{
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_09.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_10.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_11.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_12.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_rare_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_rare_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_rare_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_rare_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_comp_rare_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_comp_rare_02.vcd" predelay "1.0, 5.0"
}
Rule PlayerWonPrevRoundCompScout
{
	criteria ConceptPlayerRoundStartComp IsScout IsNotFirstRound IsNotComp6v6 PlayerWonPreviousRound 40PercentChance
	Response PlayerWonPrevRoundCompScout
}

Response PlayerWonPrevRoundComp6sScout
{
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_09.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_10.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_11.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_12.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_rare_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_rare_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_rare_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_rare_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_comp_rare_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_comp_rare_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_6s_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamewonlast_6s_02.vcd" predelay "1.0, 5.0"
}
Rule PlayerWonPrevRoundComp6sScout
{
	criteria ConceptPlayerRoundStartComp IsScout IsNotFirstRound IsComp6v6 PlayerWonPreviousRound 40PercentChance
	Response PlayerWonPrevRoundComp6sScout
}

Response PlayerLostPrevRoundCompScout
{
	scene "scenes/Player/Scout/low/cm_scout_pregamelostlast_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamelostlast_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamelostlast_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamelostlast_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamelostlast_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamelostlast_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamelostlast_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamelostlast_rare_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamelostlast_rare_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregamelostlast_rare_03.vcd" predelay "1.0, 5.0"
}
Rule PlayerLostPrevRoundCompScout
{
	criteria ConceptPlayerRoundStartComp IsScout IsNotFirstRound PlayerLostPreviousRound PreviousRoundWasNotTie 40PercentChance
	Response PlayerLostPrevRoundCompScout
}

Response PlayerTiedPrevRoundCompScout
{
	scene "scenes/Player/Scout/low/cm_scout_pregametie_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregametie_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregametie_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregametie_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregametie_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregametie_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_pregametie_07.vcd" predelay "1.0, 5.0"
}
Rule PlayerTiedPrevRoundCompScout
{
	criteria ConceptPlayerRoundStartComp IsScout IsNotFirstRound PreviousRoundWasTie 40PercentChance
	Response PlayerTiedPrevRoundCompScout
}

Response PlayerGameWinCompScout
{
	scene "scenes/Player/Scout/low/cm_scout_gamewon_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_gamewon_02.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_gamewon_03.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_gamewon_04.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_gamewon_05.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_gamewon_06.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_gamewon_07.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_gamewon_08.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_gamewon_09.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_gamewon_10.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_gamewon_11.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_gamewon_12.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_gamewon_13.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_gamewon_14.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_gamewon_15.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_gamewon_rare_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Scout/low/cm_scout_gamewon_rare_02.vcd" predelay "2.0, 5.0"
}
Rule PlayerGameWinCompScout
{
	criteria ConceptPlayerGameOverComp PlayerOnWinningTeam IsScout 40PercentChance
	Response PlayerGameWinCompScout
}

Response PlayerMatchWinCompScout
{
	scene "scenes/Player/Scout/low/cm_scout_matchwon_01.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Scout/low/cm_scout_matchwon_02.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Scout/low/cm_scout_matchwon_03.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Scout/low/cm_scout_matchwon_04.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Scout/low/cm_scout_matchwon_05.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Scout/low/cm_scout_matchwon_06.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Scout/low/cm_scout_matchwon_07.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Scout/low/cm_scout_matchwon_08.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Scout/low/cm_scout_matchwon_09.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Scout/low/cm_scout_matchwon_10.vcd" predelay "1.0, 2.0"
}
Rule PlayerMatchWinCompScout
{
	criteria ConceptPlayerMatchOverComp PlayerOnWinningTeam IsScout 40PercentChance
	Response PlayerMatchWinCompScout
}
//--------------------------------------------------------------------------------------------------------------
// End Competitive Mode VO
//--------------------------------------------------------------------------------------------------------------