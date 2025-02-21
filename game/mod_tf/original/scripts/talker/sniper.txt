//--------------------------------------------------------------------------------------------------------------
// Sniper Response Rule File
//--------------------------------------------------------------------------------------------------------------

Criterion "SniperIsNotStillonFire" "SniperOnFire" "!=1" "required" weight 0
Criterion "SniperIsStillonFire" "SniperOnFire" "1" "required" weight 0
Criterion "SniperNotKillSpeech" "SniperKillSpeech" "!=1" "required" weight 0
Criterion "SniperNotKillSpeechMelee" "SniperKillSpeechMelee" "!=1" "required" weight 0
Criterion "SniperNotSaidCartMovingBackwardD" "SaidCartMovingBackwardD" "!=1" "required" weight 0
Criterion "SniperNotSaidHealThanks" "SniperSaidHealThanks" "!=1" "required"
Criterion "IsHelpCapSniper" "SniperHelpCap" "1" "required" weight 0
// Custom stuff
Criterion "SniperNotInvulnerableSpeech" "SniperInvulnerableSpeech" "!=1" "required" weight 0
Criterion "SniperNotSaidCartMovingBackwardO" "SaidCartMovingBackwardO" "!=1" "required" weight 0
Criterion "SniperNotSaidCartMovingForwardD" "SaidCartMovingForwardD" "!=1" "required" weight 0
Criterion "SniperNotSaidCartMovingForwardO" "SaidCartMovingForwardO" "!=1" "required" weight 0
Criterion "SniperNotSaidCartMovingStoppedD" "SaidCartMovingStoppedD" "!=1" "required" weight 0
Criterion "SniperNotSaidCartMovingStoppedO" "SaidCartMovingStoppedO" "!=1" "required" weight 0
Criterion "SniperNotAwardSpeech" "SniperAwardSpeech" "!=1" "required" weight 0
Criterion "SniperNotAssistSpeech" "SniperAssistSpeech" "!=1" "required" weight 0
Criterion "SniperNotHoldStill" "SniperHoldStill" "!=1" "required" weight 0


Response PlayerCloakedSpyDemomanSniper
{
	scene "scenes/Player/Sniper/low/1633.vcd" 
}
Rule PlayerCloakedSpyDemomanSniper
{
	criteria ConceptPlayerCloakedSpy IsSniper IsOnDemoman
	Response PlayerCloakedSpyDemomanSniper
}

Response PlayerCloakedSpyEngineerSniper
{
	scene "scenes/Player/Sniper/low/1636.vcd" 
}
Rule PlayerCloakedSpyEngineerSniper
{
	criteria ConceptPlayerCloakedSpy IsSniper IsOnEngineer
	Response PlayerCloakedSpyEngineerSniper
}

Response PlayerCloakedSpyHeavySniper
{
	scene "scenes/Player/Sniper/low/1631.vcd" 
}
Rule PlayerCloakedSpyHeavySniper
{
	criteria ConceptPlayerCloakedSpy IsSniper IsOnHeavy
	Response PlayerCloakedSpyHeavySniper
}

Response PlayerCloakedSpyMedicSniper
{
	scene "scenes/Player/Sniper/low/1635.vcd" 
}
Rule PlayerCloakedSpyMedicSniper
{
	criteria ConceptPlayerCloakedSpy IsSniper IsOnMedic
	Response PlayerCloakedSpyMedicSniper
}

Response PlayerCloakedSpyPyroSniper
{
	scene "scenes/Player/Sniper/low/1632.vcd" 
}
Rule PlayerCloakedSpyPyroSniper
{
	criteria ConceptPlayerCloakedSpy IsSniper IsOnPyro
	Response PlayerCloakedSpyPyroSniper
}

Response PlayerCloakedSpyScoutSniper
{
	scene "scenes/Player/Sniper/low/1629.vcd" 
}
Rule PlayerCloakedSpyScoutSniper
{
	criteria ConceptPlayerCloakedSpy IsSniper IsOnScout
	Response PlayerCloakedSpyScoutSniper
}

Response PlayerCloakedSpySniperSniper
{
	scene "scenes/Player/Sniper/low/1637.vcd" 
}
Rule PlayerCloakedSpySniperSniper
{
	criteria ConceptPlayerCloakedSpy IsSniper IsOnSniper
	Response PlayerCloakedSpySniperSniper
}

Response PlayerCloakedSpySoldierSniper
{
	scene "scenes/Player/Sniper/low/1630.vcd" 
}
Rule PlayerCloakedSpySoldierSniper
{
	criteria ConceptPlayerCloakedSpy IsSniper IsOnSoldier
	Response PlayerCloakedSpySoldierSniper
}

Response PlayerCloakedSpySpySniper
{
	scene "scenes/Player/Sniper/low/1634.vcd" 
}
Rule PlayerCloakedSpySpySniper
{
	criteria ConceptPlayerCloakedSpy IsSniper IsOnSpy
	Response PlayerCloakedSpySpySniper
}


// Modified to only have whisper lines when headshot with rifle.
// Other lines for when you get a kill with rifle/bow against a Soldier.
Response SniperKillSoldier
{
	scene "scenes/Player/Sniper/low/1719.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1727.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1731.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1757.vcd" predelay "1"
}
Rule SniperKillSoldier
{
	criteria ConceptKilledPlayer IsSniper WeaponIsSniperRifle 50PercentChance IsVictimSoldier SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response SniperKillSoldier
}
Rule SniperKillSoldierClassic
{
	criteria ConceptKilledPlayer IsSniper WeaponIsClassicSniperrifle 50PercentChance IsVictimSoldier SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response SniperKillSoldier
}
Rule SniperKillSoldierBow
{
	criteria ConceptKilledPlayer IsSniper WeaponIsBow 50PercentChance IsVictimSoldier SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response SniperKillSoldier
}

Response SniperHeadShotKillSoldier
{
	scene "scenes/Player/Sniper/low/1725.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1807.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1803.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1831.vcd" predelay "1.75"
}
Rule SniperHeadShotKillSoldier
{
	criteria ConceptKilledPlayer IsSniper WeaponIsSniperrifle IsHeadShot 50PercentChance IsVictimSoldier SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response SniperHeadShotKillSoldier
}
Rule SniperHeadShotKillSoldierClassic
{
	criteria ConceptKilledPlayer IsSniper WeaponIsClassicSniperrifle IsHeadShot 50PercentChance IsVictimSoldier SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response SniperHeadShotKillSoldier
}

// Same for the Heavy

Response SniperKillHeavy
{
	scene "scenes/Player/Sniper/low/1758.vcd" predelay "1"
}
Rule SniperKillHeavy
{
	criteria ConceptKilledPlayer IsSniper WeaponIsSniperRifle 50PercentChance IsVictimHeavy SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response SniperKillHeavy
}
Rule SniperKillHeavyClassic
{
	criteria ConceptKilledPlayer IsSniper WeaponIsClassicSniperrifle 50PercentChance IsVictimHeavy SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response SniperKillHeavy
}
Rule SniperKillHeavyBow
{
	criteria ConceptKilledPlayer IsSniper WeaponIsBow 50PercentChance IsVictimHeavy SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response SniperKillHeavy
}

Response SniperHeadShotKillHeavy
{
	scene "scenes/Player/Sniper/low/1832.vcd" predelay "1.75"
}
Rule SniperHeadShotKillHeavy
{
	criteria ConceptKilledPlayer IsSniper WeaponIsSniperrifle IsHeadShot 50PercentChance IsVictimHeavy SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response SniperHeadShotKillHeavy
}
Rule SniperHeadShotKillHeavyClassic
{
	criteria ConceptKilledPlayer IsSniper WeaponIsClassicSniperrifle IsHeadShot 50PercentChance IsVictimHeavy SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response SniperHeadShotKillHeavy
}

// Modified to only have whisper lines when headshot with rifle.
// Other lines for when you get a kill with any weapon against a Spy.
Response SniperKillSpy
{
	scene "scenes/Player/Sniper/low/1712.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1723.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1729.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1732.vcd" predelay "1"
}
Rule SniperKillSpy
{
	criteria ConceptKilledPlayer IsSniper 50PercentChance IsVictimSpy SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response SniperKillSpy
}

Response SniperHeadShotKillSpy
{
	scene "scenes/Player/Sniper/low/1805.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1806.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1808.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1798.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1799.vcd" predelay "1.75"
}
Rule SniperHeadShotKillSpy
{
	criteria ConceptKilledPlayer IsSniper WeaponIsSniperrifle IsHeadShot 50PercentChance IsVictimSpy SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response SniperHeadShotKillSpy
}
Rule SniperHeadShotKillSpyClassic
{
	criteria ConceptKilledPlayer IsSniper WeaponIsClassicSniperrifle IsHeadShot 50PercentChance IsVictimSpy SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response SniperHeadShotKillSpy
}

//--------------------------------------------------------------------------------------------------------------
// Auto Speech
//--------------------------------------------------------------------------------------------------------------
Response HealThanksSniper
{
	scene "scenes/Player/Sniper/low/1761.vcd" 
	scene "scenes/Player/Sniper/low/1762.vcd" 
	scene "scenes/Player/Sniper/low/1763.vcd" 
}
Rule HealThanksSniper
{
	criteria ConceptMedicChargeStopped IsSniper SuperHighHealthContext SniperNotSaidHealThanks 50PercentChance
	ApplyContext "SniperSaidHealThanks:1:20"
	Response HealThanksSniper
}

// Custom achievement stuff
Response AwardSniper
{
	scene "scenes/Player/Sniper/low/2298.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2299.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2339.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2340.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2341.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2343.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2344.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2346.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2347.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2350.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2351.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2423.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2436.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2453.vcd" predelay "2.5"
}
Rule AwardSniper
{
	criteria ConceptAchievementAward IsSniper SniperNotAwardSpeech
	ApplyContext "SniperAwardSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response AwardSniper
}
//End custom achievement

Response PlayerRoundStartSniper
{
	scene "scenes/Player/Sniper/low/1612.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/1613.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/1614.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/1615.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/1616.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/1617.vcd" predelay "1.0, 5.0"
}
Rule PlayerRoundStartSniper
{
	criteria ConceptPlayerRoundStart IsSniper
	Response PlayerRoundStartSniper
}

Response PlayerCappedIntelligenceSniper
{
	scene "scenes/Player/Sniper/low/1603.vcd" 
	scene "scenes/Player/Sniper/low/1604.vcd" 
	scene "scenes/Player/Sniper/low/1605.vcd" 
	scene "scenes/Player/Sniper/low/1771.vcd" 
	scene "scenes/Player/Sniper/low/1772.vcd" 
}
Rule PlayerCappedIntelligenceSniper
{
	criteria ConceptPlayerCapturedIntelligence IsSniper
	Response PlayerCappedIntelligenceSniper
}

Response PlayerCapturedPointSniper
{
	scene "scenes/Player/Sniper/low/1600.vcd" 
	scene "scenes/Player/Sniper/low/1601.vcd" 
	scene "scenes/Player/Sniper/low/1602.vcd" 
}
Rule PlayerCapturedPointSniper
{
	criteria ConceptPlayerCapturedPoint IsSniper
	Response PlayerCapturedPointSniper
}

Response PlayerSuddenDeathSniper
{
	scene "scenes/Player/Sniper/low/1662.vcd" 
	scene "scenes/Player/Sniper/low/1663.vcd" 
	scene "scenes/Player/Sniper/low/1664.vcd" 
	scene "scenes/Player/Sniper/low/1665.vcd" 
	scene "scenes/Player/Sniper/low/1666.vcd" 
	scene "scenes/Player/Sniper/low/1667.vcd" 
	scene "scenes/Player/Sniper/low/1668.vcd" 
	scene "scenes/Player/Sniper/low/1669.vcd" 
}
Rule PlayerSuddenDeathSniper
{
	criteria ConceptPlayerSuddenDeathStart IsSniper
	Response PlayerSuddenDeathSniper
}

Response PlayerStalemateSniper
{
	scene "scenes/Player/Sniper/low/1606.vcd" 
	scene "scenes/Player/Sniper/low/1607.vcd" 
	scene "scenes/Player/Sniper/low/1608.vcd" 
}
Rule PlayerStalemateSniper
{
	criteria ConceptPlayerStalemate IsSniper
	Response PlayerStalemateSniper
}

Response PlayerTeleporterThanksSniper
{
	scene "scenes/Player/Sniper/low/1764.vcd" 
	scene "scenes/Player/Sniper/low/1765.vcd" 
	scene "scenes/Player/Sniper/low/1766.vcd" 
}
Rule PlayerTeleporterThanksSniper
{
	criteria ConceptTeleported IsNotEngineer IsSniper 30PercentChance
	Response PlayerTeleporterThanksSniper
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Cart
//--------------------------------------------------------------------------------------------------------------
Response CartMovingBackwardsDefenseSniper
{
	scene "scenes/Player/Sniper/low/2334.vcd" 
	scene "scenes/Player/Sniper/low/2335.vcd" 
	scene "scenes/Player/Sniper/low/2336.vcd" 
	scene "scenes/Player/Sniper/low/2352.vcd" 
	scene "scenes/Player/Sniper/low/2431.vcd" 
	scene "scenes/Player/Sniper/low/2432.vcd" 
	scene "scenes/Player/Sniper/low/2444.vcd" 
	scene "scenes/Player/Sniper/low/2445.vcd" 
}
Rule CartMovingBackwardsDefenseSniper
{
	criteria ConceptCartMovingBackward IsOnDefense IsSniper Unzoomed SniperNotSaidCartMovingBackwardD IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingBackwardD:1:20"
	Response CartMovingBackwardsDefenseSniper
}

// Custom added stuff
// We fire this response if the Sniper is scoped, these are softer versions of the normal lines.
Response CartMovingBackwardsDefenseScopedSniper
{
	scene "scenes/Player/Sniper/low/2518.vcd" 
	scene "scenes/Player/Sniper/low/2519.vcd" 
	scene "scenes/Player/Sniper/low/2520.vcd" 
	scene "scenes/Player/Sniper/low/2521.vcd" 
	scene "scenes/Player/Sniper/low/2540.vcd" 
	scene "scenes/Player/Sniper/low/2541.vcd" 
	scene "scenes/Player/Sniper/low/2542.vcd" 
	scene "scenes/Player/Sniper/low/2543.vcd" 
}
Rule CartMovingBackwardsDefenseScopedSniper
{
	criteria ConceptCartMovingBackward IsOnDefense IsSniper DeployedContext SniperNotSaidCartMovingBackwardD IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingBackwardD:1:20"
	Response CartMovingBackwardsDefenseScopedSniper
}

// Offense backwards
Response CartMovingBackwardsOffenseSniper
{
	scene "scenes/Player/Sniper/low/2331.vcd" 
	scene "scenes/Player/Sniper/low/2332.vcd" 
	scene "scenes/Player/Sniper/low/2333.vcd" 
	scene "scenes/Player/Sniper/low/2353.vcd" 
	scene "scenes/Player/Sniper/low/2354.vcd" 
	scene "scenes/Player/Sniper/low/2429.vcd" 
	scene "scenes/Player/Sniper/low/2430.vcd" 
}
Rule CartMovingBackwardsOffenseSniper
{
	criteria ConceptCartMovingBackward IsOnOffense IsSniper Unzoomed SniperNotSaidCartMovingBackwardO IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingBackwardO:1:20"
	Response CartMovingBackwardsOffenseSniper
}

Response CartMovingBackwardsOffenseScopedSniper
{
	scene "scenes/Player/Sniper/low/2515.vcd" 
	scene "scenes/Player/Sniper/low/2516.vcd" 
	scene "scenes/Player/Sniper/low/2517.vcd" 
	scene "scenes/Player/Sniper/low/2522.vcd" 
	scene "scenes/Player/Sniper/low/2523.vcd" 
	scene "scenes/Player/Sniper/low/2538.vcd" 
	scene "scenes/Player/Sniper/low/2539.vcd" 
}
Rule CartMovingBackwardsOffenseScopedSniper
{
	criteria ConceptCartMovingBackward IsOnOffense IsSniper DeployedContext SniperNotSaidCartMovingBackwardO IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingBackwardO:1:20"
	Response CartMovingBackwardsOffenseScopedSniper
}

// Defense forward
Response CartMovingForwardDefenseSniper
{
	scene "scenes/Player/Sniper/low/2328.vcd" 
	scene "scenes/Player/Sniper/low/2329.vcd" 
	scene "scenes/Player/Sniper/low/2330.vcd" 
	scene "scenes/Player/Sniper/low/2355.vcd" 
	scene "scenes/Player/Sniper/low/2446.vcd" 
	scene "scenes/Player/Sniper/low/2447.vcd" 
}
Rule CartMovingForwardDefenseSniper
{
	criteria ConceptCartMovingForward IsOnDefense IsSniper Unzoomed SniperNotSaidCartMovingForwardD IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingForwardD:1:20"
	Response CartMovingForwardDefenseSniper
}

Response CartMovingForwardDefenseScopedSniper
{
	scene "scenes/Player/Sniper/low/2512.vcd"  
	scene "scenes/Player/Sniper/low/2513.vcd"  
	scene "scenes/Player/Sniper/low/2514.vcd"  
	scene "scenes/Player/Sniper/low/2524.vcd"  
	scene "scenes/Player/Sniper/low/2544.vcd"  
	scene "scenes/Player/Sniper/low/2545.vcd"  
}
Rule CartMovingForwardDefenseSniper
{
	criteria ConceptCartMovingForward IsOnDefense IsSniper DeployedContext SniperNotSaidCartMovingForwardD IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingForwardD:1:20"
	Response CartMovingForwardDefenseScopedSniper
}

// Offense forward

Response CartMovingForwardOffenseSniper
{
	scene "scenes/Player/Sniper/low/2307.vcd"  
	scene "scenes/Player/Sniper/low/2308.vcd"  
	scene "scenes/Player/Sniper/low/2309.vcd"  
	scene "scenes/Player/Sniper/low/2402.vcd"  
	scene "scenes/Player/Sniper/low/2403.vcd"  
	scene "scenes/Player/Sniper/low/2404.vcd"   
	scene "scenes/Player/Sniper/low/2406.vcd"  
	scene "scenes/Player/Sniper/low/2407.vcd"  
	scene "scenes/Player/Sniper/low/2408.vcd"  
	scene "scenes/Player/Sniper/low/2409.vcd"  
	scene "scenes/Player/Sniper/low/2301.vcd"  
	scene "scenes/Player/Sniper/low/2302.vcd"  
	scene "scenes/Player/Sniper/low/2303.vcd"  
	scene "scenes/Player/Sniper/low/2416.vcd"  
	scene "scenes/Player/Sniper/low/2424.vcd"  
	scene "scenes/Player/Sniper/low/2425.vcd"  
	scene "scenes/Player/Sniper/low/2454.vcd"  
}
Rule CartMovingForwardOffenseSniper
{
	criteria ConceptCartMovingForward IsOnOffense IsSniper Unzoomed SniperNotSaidCartMovingForwardO IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingForwardO:1:20"
	Response CartMovingForwardOffenseSniper
}

Response CartMovingForwardOffenseScopedSniper
{
	scene "scenes/Player/Sniper/low/2506.vcd"  
	scene "scenes/Player/Sniper/low/2507.vcd"  
	scene "scenes/Player/Sniper/low/2508.vcd"  
	scene "scenes/Player/Sniper/low/2525.vcd"  
	scene "scenes/Player/Sniper/low/2526.vcd"  
	scene "scenes/Player/Sniper/low/2527.vcd"  
	scene "scenes/Player/Sniper/low/2529.vcd"  
	scene "scenes/Player/Sniper/low/2530.vcd"  
	scene "scenes/Player/Sniper/low/2531.vcd"  
	scene "scenes/Player/Sniper/low/2532.vcd"  
	scene "scenes/Player/Sniper/low/2500.vcd"  
	scene "scenes/Player/Sniper/low/2501.vcd"  
	scene "scenes/Player/Sniper/low/2502.vcd"  
	scene "scenes/Player/Sniper/low/2535.vcd"  
	scene "scenes/Player/Sniper/low/2536.vcd"  
	scene "scenes/Player/Sniper/low/2537.vcd"  
	scene "scenes/Player/Sniper/low/2546.vcd"  
}
Rule CartMovingForwardOffenseScopedSniper
{
	criteria ConceptCartMovingForward IsOnOffense IsSniper DeployedContext SniperNotSaidCartMovingForwardO IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingForwardO:1:20"
	Response CartMovingForwardOffenseScopedSniper
}
// Cart stopped Defense

Response CartMovingStoppedDefenseSniper
{
	scene "scenes/Player/Sniper/low/2310.vcd" 
	scene "scenes/Player/Sniper/low/2311.vcd" 
	scene "scenes/Player/Sniper/low/2312.vcd" 
	scene "scenes/Player/Sniper/low/2414.vcd" 
	scene "scenes/Player/Sniper/low/2455.vcd" 
	scene "scenes/Player/Sniper/low/2456.vcd" 
}
Rule CartMovingStoppedDefenseSniper
{
	criteria ConceptCartMovingStopped IsOnDefense IsSniper Unzoomed SniperNotSaidCartMovingStoppedD IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingStoppedD:1:20"
	Response CartMovingStoppedDefenseSniper
}

Response CartMovingStoppedDefenseScopedSniper
{
	scene "scenes/Player/Sniper/low/2509.vcd"  
	scene "scenes/Player/Sniper/low/2510.vcd"  
	scene "scenes/Player/Sniper/low/2511.vcd"  
	scene "scenes/Player/Sniper/low/2533.vcd"  
	scene "scenes/Player/Sniper/low/2558.vcd"  
}
Rule CartMovingStoppedDefenseScopedSniper
{
	criteria ConceptCartMovingStopped IsOnDefense IsSniper DeployedContext SniperNotSaidCartMovingStoppedD IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingStoppedD:1:20"
	Response CartMovingStoppedDefenseScopedSniper
}

// Cart stopped Offense

Response CartMovingStoppedOffenseSniper
{
	scene "scenes/Player/Sniper/low/2304.vcd" 
	scene "scenes/Player/Sniper/low/2305.vcd" 
	scene "scenes/Player/Sniper/low/2306.vcd" 
	scene "scenes/Player/Sniper/low/2415.vcd" 
	scene "scenes/Player/Sniper/low/2457.vcd" 
}
Rule CartMovingStoppedOffenseSniper
{
	criteria ConceptCartMovingStopped IsOnOffense IsSniper Unzoomed SniperNotSaidCartMovingStoppedO IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingStoppedO:1:20"
	Response CartMovingStoppedOffenseSniper
}

Response CartMovingStoppedOffenseScopedSniper
{
	scene "scenes/Player/Sniper/low/2503.vcd"  
	scene "scenes/Player/Sniper/low/2504.vcd"  
	scene "scenes/Player/Sniper/low/2505.vcd"  
	scene "scenes/Player/Sniper/low/2534.vcd"  
	scene "scenes/Player/Sniper/low/2549.vcd"  
}
Rule CartMovingStoppedOffenseScopedSniper
{
	criteria ConceptCartMovingStopped IsOnOffense IsSniper DeployedContext SniperNotSaidCartMovingStoppedO IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingStoppedO:1:20"
	Response CartMovingStoppedOffenseScopedSniper
}
// End custom


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Combat
//--------------------------------------------------------------------------------------------------------------
Response DefendOnThePointSniper
{
	scene "scenes/Player/Sniper/low/1736.vcd" 
	scene "scenes/Player/Sniper/low/1737.vcd" 
}
Rule DefendOnThePointSniper
{
	criteria ConceptFireWeapon IsSniper IsOnFriendlyControlPoint NotDefendOnThePointSpeech
	ApplyContext "DefendOnThePointSpeech:1:30"
	applycontexttoworld
	Response DefendOnThePointSniper
}

// Custom stuff

Response InvulnerableSpeechSniper
{
	scene "scenes/Player/Sniper/low/1750.vcd"
	scene "scenes/Player/Sniper/low/1751.vcd"
	scene "scenes/Player/Sniper/low/1754.vcd"
	scene "scenes/Player/Sniper/low/1756.vcd"
}
Rule InvulnerableSpeechSniper
{
	criteria ConceptFireWeapon IsSniper IsInvulnerable SniperNotInvulnerableSpeech
	ApplyContext "SniperInvulnerableSpeech:1:30"
	Response InvulnerableSpeechSniper
}

// End custom

Response KilledPlayerManySniper
{
	scene "scenes/Player/Sniper/low/1670.vcd" 
	scene "scenes/Player/Sniper/low/1671.vcd" 
	scene "scenes/Player/Sniper/low/1672.vcd" 
	scene "scenes/Player/Sniper/low/1673.vcd" 
	scene "scenes/Player/Sniper/low/1674.vcd" 
	scene "scenes/Player/Sniper/low/1675.vcd" 
	scene "scenes/Player/Sniper/low/1715.vcd" 
	scene "scenes/Player/Sniper/low/1726.vcd" 
	scene "scenes/Player/Sniper/low/1810.vcd" 
}
Rule KilledPlayerManySniper
{
	criteria ConceptKilledPlayer IsManyRecentKills 30PercentChance IsWeaponPrimary KilledPlayerDelay SniperNotKillSpeech IsSniper
	ApplyContext "SniperKillSpeech:1:10"
	Response KilledPlayerManySniper
}

// Custom stuff
// Allow the Huntsman to use the above response group
Rule KilledPlayerManySniperBow
{
	criteria ConceptKilledPlayer IsManyRecentKills 30PercentChance WeaponIsBow KilledPlayerDelay SniperNotKillSpeech IsSniper
	ApplyContext "SniperKillSpeech:1:10"
	Response KilledPlayerManySniper
}

// End custom

Response KilledPlayerMeleeSniper
{
	scene "scenes/Player/Sniper/low/1717.vcd" 
	scene "scenes/Player/Sniper/low/1720.vcd" 
}
Rule KilledPlayerMeleeSniper
{
	criteria ConceptKilledPlayer KilledPlayerDelay 30PercentChance IsWeaponMelee SniperNotKillSpeechMelee WeaponIsNotSaxxy IsSniper
	ApplyContext "SniperKillSpeechMelee:1:10"
	Response KilledPlayerMeleeSniper
}

Response KilledPlayerSaxxySniper
{
	scene "scenes/Player/Sniper/low/1718.vcd"
	scene "scenes/Player/Sniper/low/1675.vcd"
	scene "scenes/Player/Sniper/low/1670.vcd"
	scene "scenes/Player/Sniper/low/1671.vcd"
	scene "scenes/Player/Sniper/low/1674.vcd"
}
Rule KilledPlayerSaxxySniper
{
	criteria ConceptKilledPlayer KilledPlayerDelay 30PercentChance WeaponIsSaxxy SniperNotKillSpeechMelee IsSniper
	ApplyContext "SniperKillSpeechMelee:1:10"
	Response KilledPlayerSaxxySniper
}

// Custom stuff
// Shiv kill

Response KilledPlayerShivSniper
{
	scene "scenes/Player/Sniper/low/2387.vcd" 
}
Rule KilledPlayerShivSniper
{
	criteria ConceptKilledPlayer KilledPlayerDelay 30PercentChance  IsWeaponMelee WeaponIsShiv SniperNotKillSpeechMelee IsSniper
	ApplyContext "SniperKillSpeechMelee:1:10"
	Response KilledPlayerShivSniper
	Response KilledPlayerMeleeSniper
}

// End custom

Response KilledPlayerVeryManySniper
{
	scene "scenes/Player/Sniper/low/1714.vcd" 
	scene "scenes/Player/Sniper/low/1718.vcd" 
	scene "scenes/Player/Sniper/low/1721.vcd" 
	scene "scenes/Player/Sniper/low/1742.vcd" 
	scene "scenes/Player/Sniper/low/1743.vcd" 
	scene "scenes/Player/Sniper/low/1744.vcd" 
	scene "scenes/Player/Sniper/low/1748.vcd" 
	scene "scenes/Player/Sniper/low/1755.vcd" 
}
Rule KilledPlayerVeryManySniper
{
	criteria ConceptKilledPlayer IsVeryManyRecentKills 50PercentChance IsWeaponPrimary KilledPlayerDelay SniperNotKillSpeech IsSniper
	ApplyContext "SniperKillSpeech:1:10"
	Response KilledPlayerVeryManySniper
}

// Custom stuff
// Allow the Huntsman to use the above response group
Rule KilledPlayerVeryManySniperBow
{
	criteria ConceptKilledPlayer IsVeryManyRecentKills 50PercentChance WeaponIsBow KilledPlayerDelay SniperNotKillSpeech IsSniper
	ApplyContext "SniperKillSpeech:1:10"
	Response KilledPlayerVeryManySniper
}

Response MedicFollowSniper
{
	scene "scenes/Player/Sniper/low/2277.vcd" predelay ".25"
	scene "scenes/Player/Sniper/low/2278.vcd" predelay ".25"
	scene "scenes/Player/Sniper/low/2279.vcd" predelay ".25"
	scene "scenes/Player/Sniper/low/2280.vcd" predelay ".25"
	scene "scenes/Player/Sniper/low/2281.vcd" predelay ".25"
}
Rule MedicFollowSniper
{
	criteria ConceptPlayerMedic IsOnMedic IsSniper IsNotCrossHairEnemy NotLowHealth SniperIsNotStillonFire
	ApplyContext "SniperKillSpeech:1:10"
	Response MedicFollowSniper
}

// Custom stuff
Response SniperJarateHit
{
	scene "scenes/Player/Sniper/low/1608.vcd"    
	scene "scenes/Player/Sniper/low/1688.vcd"    
	scene "scenes/Player/Sniper/low/1689.vcd"    
}
Rule SniperJarateHit
{
	criteria ConceptJarateHit IsSniper 50PercentChance
	Response SniperJarateHit
}

Response SniperJarateHitScoped
{
	scene "scenes/Player/Sniper/low/1780.vcd"    
	scene "scenes/Player/Sniper/low/1690.vcd"    
	scene "scenes/Player/Sniper/low/1778.vcd"    
}
Rule SniperJarateHitScoped
{
	criteria ConceptJarateHit IsSniper DeployedContext
	Response SniperJarateHitScoped
}
// End custom

Response PlayerJarateToss
{
	scene "scenes/Player/Sniper/low/3131.vcd" 
	scene "scenes/Player/Sniper/low/3132.vcd" 
	scene "scenes/Player/Sniper/low/3133.vcd" 
}
Rule PlayerJarateToss
{
	criteria ConceptJarateLaunch IsSniper
	Response PlayerJarateToss
}

Response PlayerKilledCapperSniper
{
	scene "scenes/Player/Sniper/low/1614.vcd" 
	scene "scenes/Player/Sniper/low/1619.vcd" 
	scene "scenes/Player/Sniper/low/1620.vcd" 
	scene "scenes/Player/Sniper/low/1710.vcd" 
	scene "scenes/Player/Sniper/low/1733.vcd" 
}
Rule PlayerKilledCapperSniper
{
	criteria ConceptCapBlocked IsSniper
	ApplyContext "SniperKillSpeech:1:10"
	Response PlayerKilledCapperSniper
}

// Custom stuff
Response KilledPlayerAssistAutoSniper
{
	scene "scenes/Player/Sniper/low/1734.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/1735.vcd" predelay "2.5"
}
Rule KilledPlayerAssistAutoSniper
{
	criteria ConceptKilledPlayer IsSniper IsBeingHealed IsARecentKill KilledPlayerDelay 20PercentChance SniperNotAssistSpeech
	ApplyContext "SniperAssistSpeech:1:20"
	Response KilledPlayerAssistAutoSniper
}
// End custom

Response KilledPlayerSMGSniper
{
	scene "scenes/Player/Sniper/low/2461.vcd" 
}
Rule KilledPlayerSMGSniper
{
	criterion ConceptKilledPlayer KilledPlayerDelay 5PercentChance IsSniper WeaponIsSMG SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response KilledPlayerSMGSniper
}

Response KilledPlayerDominatingSniper
{
	scene "scenes/Player/Sniper/low/2292.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2293.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2294.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2356.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2357.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2358.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2360.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2362.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2363.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2364.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2365.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2366.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2367.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2385.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2397.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2417.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2434.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2458.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2459.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2460.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2462.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2463.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2464.vcd" predelay "2.5"
	
}
Rule KilledPlayerDominatingSniper
{
	criterion ConceptKilledPlayer IsSniper DeployedContext 50PercentChance IsDominated
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response KilledPlayerDominatingSniper
}
// End custom

Response PlayerKilledDominatingDemomanSniper
{
	scene "scenes/Player/Sniper/low/3080.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3081.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3082.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3083.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3084.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingDemomanSniper
{
	criteria ConceptKilledPlayer IsSniper IsDominated  IsVictimDemoman
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingDemomanSniper
}

Response PlayerKilledDominatingEngineerSniper
{
	scene "scenes/Player/Sniper/low/3085.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3086.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3087.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3088.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3089.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3090.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingEngineerSniper
{
	criteria ConceptKilledPlayer IsSniper IsDominated  IsVictimEngineer
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingEngineerSniper
}

Response PlayerKilledDominatingHeavySniper
{
	scene "scenes/Player/Sniper/low/3091.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3092.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3093.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3094.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3095.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3096.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3097.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingHeavySniper
{
	criteria ConceptKilledPlayer IsSniper IsDominated  IsVictimHeavy
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingHeavySniper
}

Response PlayerKilledDominatingMedicSniper
{
	scene "scenes/Player/Sniper/low/3098.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3099.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3100.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3101.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3102.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingMedicSniper
{
	criteria ConceptKilledPlayer IsSniper IsDominated  IsVictimMedic
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingMedicSniper
}

Response PlayerKilledDominatingPyroSniper
{
	scene "scenes/Player/Sniper/low/3103.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3104.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3105.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3106.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3107.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingPyroSniper
{
	criteria ConceptKilledPlayer IsSniper IsDominated  IsVictimPyro
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingPyroSniper
}

Response PlayerKilledDominatingScoutSniper
{
	scene "scenes/Player/Sniper/low/3108.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3109.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3110.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3111.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3112.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingScoutSniper
{
	criteria ConceptKilledPlayer IsSniper IsDominated  IsVictimScout
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingScoutSniper
}

Response PlayerKilledDominatingSniperSniper
{
	scene "scenes/Player/Sniper/low/2366.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2397.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2417.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2434.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3113.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3114.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3115.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3116.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3117.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingSniperSniper
{
	criteria ConceptKilledPlayer IsSniper IsDominated  IsVictimSniper
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingSniperSniper
}

Response PlayerKilledDominatingSoldierSniper
{
	scene "scenes/Player/Sniper/low/3118.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3119.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3120.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3121.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3122.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3123.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingSoldierSniper
{
	criteria ConceptKilledPlayer IsSniper IsDominated  IsVictimSoldier
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingSoldierSniper
}

Response PlayerKilledDominatingSpySniper
{
	scene "scenes/Player/Sniper/low/3124.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3125.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3126.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3127.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3128.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3129.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/3130.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingSpySniper
{
	criteria ConceptKilledPlayer IsSniper IsDominated  IsVictimSpy
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingSpySniper
}

// Custom stuff
// Directed revenge lines
Response PlayerKilledForRevengeSniperSniper
{
	scene "scenes/Player/Sniper/low/2466.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2468.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2469.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2473.vcd" predelay "2.5"
}
Rule PlayerKilledForRevengeSniperSniper
{
	criterion ConceptKilledPlayer IsSniper IsRevenge IsVictimSniper
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledForRevengeSniperSniper
}

Response PlayerKilledForRevengeSpySniper
{
	scene "scenes/Player/Sniper/low/2381.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2383.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2384.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2391.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2325.vcd" predelay "2.5"
}
Rule PlayerKilledForRevengeSpySniper
{
	criterion ConceptKilledPlayer IsSniper IsRevenge IsVictimSpy
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledForRevengeSpySniper
}

Response PlayerKilledForRevengeScoutSniper
{
	scene "scenes/Player/Sniper/low/2296.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2394.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2475.vcd" predelay "2.5"
}
Rule PlayerKilledForRevengeScoutSniper
{
	criterion ConceptKilledPlayer IsSniper IsRevenge IsVictimScout
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledForRevengeScoutSniper
}

Response PlayerKilledForRevengeDemomanSniper
{
	scene "scenes/Player/Sniper/low/2295.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2386.vcd" predelay "2.5"
}
Rule PlayerKilledForRevengeDemomanSniper
{
	criterion ConceptKilledPlayer IsSniper IsRevenge IsVictimDemoman
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledForRevengeDemomanSniper
}

Response PlayerKilledForRevengeEngineerSniper
{
	scene "scenes/Player/Sniper/low/2393.vcd" predelay "2.5"
}
Rule PlayerKilledForRevengeEngineerSniper
{
	criterion ConceptKilledPlayer IsSniper IsRevenge IsVictimEngineer
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledForRevengeEngineerSniper
}

Response PlayerKilledForRevengeSoldierSniper
{
	scene "scenes/Player/Sniper/low/2390.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2472.vcd" predelay "2.5"
}
Rule PlayerKilledForRevengeSoldierSniper
{
	criterion ConceptKilledPlayer IsSniper IsRevenge IsVictimSoldier
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledForRevengeSoldierSniper
}

Response PlayerKilledForRevengeMedicSniper
{
	scene "scenes/Player/Sniper/low/2474.vcd" predelay "2.5"
}
Rule PlayerKilledForRevengeMedicSniper
{
	criterion ConceptKilledPlayer IsSniper IsRevenge IsVictimMedic
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledForRevengeMedicSniper
}

Response PlayerKilledForRevengePyroSniper
{
	scene "scenes/Player/Sniper/low/2392.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2395.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2467.vcd" predelay "2.5"
}
Rule PlayerKilledForRevengePyroSniper
{
	criterion ConceptKilledPlayer IsSniper IsRevenge IsVictimPyro
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledForRevengePyroSniper
}

Response PlayerKilledForRevengeHeavySniper
{
	scene "scenes/Player/Sniper/low/2465.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2471.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/2465.vcd" predelay "2.5"
}
Rule PlayerKilledForRevengeHeavySniper
{
	criterion ConceptKilledPlayer IsSniper IsRevenge IsVictimHeavy
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledForRevengeHeavySniper
}

// End directed revenge lines

Response PlayerKilledForRevengeSniper
{
	scene "scenes/Player/Sniper/low/1622.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/1641.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/1708.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/1709.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/1741.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/1740.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/1752.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/1753.vcd" predelay "2.5"
}
Rule PlayerKilledForRevengeSniper
{
	criteria ConceptKilledPlayer IsSniper IsRevenge 50PercentChance
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledForRevengeSniper
}

// Scoped revenge lines, whispers
Response PlayerKilledForRevengeSoftSniper
{
	scene "scenes/Player/Sniper/low/1814.vcd" predelay "2.5"
	scene "scenes/Player/Sniper/low/1815.vcd" predelay "2.5"
}
Rule PlayerKilledForRevengeSoftSniper
{
	criteria ConceptKilledPlayer IsSniper IsRevenge DeployedContext 50PercentChance
	ApplyContext "SniperKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledForRevengeSoftSniper
	Response PlayerKilledForRevengeSniper
}

// Here we have normal kills, normal scoped kills and headshots
// Scoped kills are whisper lines, normal kills are not
// So Huntsman kills shouldn't be whispers
// Only headshot kills use all three response groups, to spice things up

Response SniperKill
{
	scene "scenes/Player/Sniper/low/1724.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1834.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1759.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1745.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1746.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1747.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1750.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1751.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1754.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1756.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1739.vcd" predelay "1"
}

Response SniperKillSoft
{
	scene "scenes/Player/Sniper/low/1809.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1813.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1826.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1827.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1828.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1816.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1817.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1818.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1819.vcd" predelay "1"
	scene "scenes/Player/Sniper/low/1820.vcd" predelay "1"
}

Rule SniperKill
{
	criteria ConceptKilledPlayer IsSniper WeaponIsSniperrifle 20PercentChance SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response SniperKill
}
Rule SniperKillClassic
{
	criteria ConceptKilledPlayer IsSniper WeaponIsClassicSniperrifle 20PercentChance SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response SniperKill
}

Rule SniperKillSoft
{
	criteria ConceptKilledPlayer IsSniper WeaponIsSniperrifle DeployedContext 20PercentChance SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response SniperKillSoft
}
Rule SniperKillSoftClassic
{
	criteria ConceptKilledPlayer IsSniper WeaponIsClassicSniperrifle DeployedContext 20PercentChance SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response SniperKillSoft
}

Rule SniperKillBow
{
	criteria ConceptKilledPlayer IsSniper WeaponIsBow 20PercentChance SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:10"
	Response SniperKill
}

Response SniperHeadShotKill
{
	scene "scenes/Player/Sniper/low/1790.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1795.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1801.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1802.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1713.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1728.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1789.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1791.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1792.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1800.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1811.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1738.vcd" predelay "1.75"
	scene "scenes/Player/Sniper/low/1812.vcd" predelay "1.75"
}
Rule SniperHeadShotKill
{
	criteria ConceptKilledPlayer IsSniper WeaponIsSniperrifle IsHeadShot SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:20"
	Response SniperHeadShotKill
	Response SniperKillSoft
	Response SniperKill
}
Rule SniperHeadShotKillClassic
{
	criteria ConceptKilledPlayer IsSniper WeaponIsClassicSniperrifle IsHeadShot SniperNotKillSpeech
	ApplyContext "SniperKillSpeech:1:20"
	Response SniperHeadShotKill
	Response SniperKillSoft
	Response SniperKill
}

// Response to play when looking at enemy in scope

Response SniperHoldStill
{
	scene "scenes/Player/Sniper/low/1835.vcd"
	scene "scenes/Player/Sniper/low/1804.vcd"
	scene "scenes/Player/Sniper/low/1833.vcd"
	scene "scenes/Player/Sniper/low/1829.vcd"
	scene "scenes/Player/Sniper/low/1830.vcd"
	scene "scenes/Player/Sniper/low/1821.vcd"
	scene "scenes/Player/Sniper/low/1822.vcd"
	scene "scenes/Player/Sniper/low/1823.vcd"
	scene "scenes/Player/Sniper/low/1824.vcd"
	scene "scenes/Player/Sniper/low/1825.vcd"
}
Rule SniperHoldStill
{
	criteria ConceptPlayerExpression IsSniper IsCrosshairEnemy DeployedContext SniperNotHoldStill
	ApplyContext "SniperHoldStill:1:10"
	Response SniperHoldStill
}

// End custom

//--------------------------------------------------------------------------------------------------------------
// Auto Speech Pain
//--------------------------------------------------------------------------------------------------------------
Response PlayerAttackerPainSniper
{
	scene "scenes/Player/Sniper/low/1700.vcd" 
	scene "scenes/Player/Sniper/low/1701.vcd" 
	scene "scenes/Player/Sniper/low/1702.vcd" 
	scene "scenes/Player/Sniper/low/1838.vcd" 
}
Rule PlayerAttackerPainSniper
{
	criteria ConceptAttackerPain IsSniper IsNotDominating
	Response PlayerAttackerPainSniper
}

Response PlayerOnFireSniper
{
	scene "scenes/Player/Sniper/low/1609.vcd" 
	scene "scenes/Player/Sniper/low/1610.vcd" 
	scene "scenes/Player/Sniper/low/1611.vcd" 
}
Rule PlayerOnFireSniper
{
	criteria ConceptFire IsSniper SniperIsNotStillonFire IsNotDominating
	ApplyContext "SniperOnFire:1:7"
	Response PlayerOnFireSniper
}

Response PlayerPainSniper
{
	scene "scenes/Player/Sniper/low/1705.vcd" 
	scene "scenes/Player/Sniper/low/1704.vcd" 
	scene "scenes/Player/Sniper/low/1703.vcd" 
	scene "scenes/Player/Sniper/low/1837.vcd" 
}
Rule PlayerPainSniper
{
	criteria ConceptPain IsSniper IsNotDominating
	Response PlayerPainSniper
}

Response PlayerStillOnFireSniper
{
	scene "scenes/Player/Sniper/low/1929.vcd" 
}
Rule PlayerStillOnFireSniper
{
	criteria ConceptFire IsSniper  SniperIsStillonFire IsNotDominating
	ApplyContext "SniperOnFire:1:7"
	Response PlayerStillOnFireSniper
}


//--------------------------------------------------------------------------------------------------------------
// Duel Speech
//--------------------------------------------------------------------------------------------------------------
Response AcceptedDuelSniper
{
	scene "scenes/Player/Sniper/low/2340.vcd" 
	scene "scenes/Player/Sniper/low/2341.vcd" 
	scene "scenes/Player/Sniper/low/2351.vcd" 
	scene "scenes/Player/Sniper/low/1613.vcd" 
	scene "scenes/Player/Sniper/low/1614.vcd" 
	scene "scenes/Player/Sniper/low/2321.vcd" 
	scene "scenes/Player/Sniper/low/2389.vcd" 
	scene "scenes/Player/Sniper/low/1749.vcd" 
	scene "scenes/Player/Sniper/low/1768.vcd" 
	scene "scenes/Player/Sniper/low/1769.vcd" 
}
Rule AcceptedDuelSniper
{
	criteria ConceptIAcceptDuel IsSniper
	Response AcceptedDuelSniper
}

Response MeleeDareSniper
{
	scene "scenes/Player/Sniper/low/2320.vcd" 
	scene "scenes/Player/Sniper/low/2369.vcd" 
	scene "scenes/Player/Sniper/low/2372.vcd" 
	scene "scenes/Player/Sniper/low/2373.vcd" 
}
Rule MeleeDareSniper
{
	criteria ConceptRequestDuel IsSniper
	Response MeleeDareSniper
}

Response RejectedDuelSniper
{
	scene "scenes/Player/Sniper/low/1606.vcd" 
	scene "scenes/Player/Sniper/low/1607.vcd" 
	scene "scenes/Player/Sniper/low/1608.vcd" 
	scene "scenes/Player/Sniper/low/3100.vcd" 
	scene "scenes/Player/Sniper/low/1663.vcd" 
	scene "scenes/Player/Sniper/low/1686.vcd" 
	scene "scenes/Player/Sniper/low/1687.vcd" 
	scene "scenes/Player/Sniper/low/1716.vcd" 
	scene "scenes/Player/Sniper/low/1722.vcd" 
}
Rule RejectedDuelSniper
{
	criteria ConceptDuelRejected IsSniper
	Response RejectedDuelSniper
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 1
//--------------------------------------------------------------------------------------------------------------
Response PlayerGoSniper
{
	scene "scenes/Player/Sniper/low/1640.vcd" 
	scene "scenes/Player/Sniper/low/1639.vcd" 
	scene "scenes/Player/Sniper/low/1638.vcd" 
}
Rule PlayerGoSniper
{
	criteria ConceptPlayerGo IsSniper
	Response PlayerGoSniper
}

Response PlayerHeadLeftSniper
{
	scene "scenes/Player/Sniper/low/1644.vcd" 
	scene "scenes/Player/Sniper/low/1645.vcd" 
	scene "scenes/Player/Sniper/low/1646.vcd" 
}
Rule PlayerHeadLeftSniper
{
	criteria ConceptPlayerLeft  IsSniper
	Response PlayerHeadLeftSniper
}

Response PlayerHeadRightSniper
{
	scene "scenes/Player/Sniper/low/1647.vcd" 
	scene "scenes/Player/Sniper/low/1648.vcd" 
	scene "scenes/Player/Sniper/low/1649.vcd" 
}
Rule PlayerHeadRightSniper
{
	criteria ConceptPlayerRight  IsSniper
	Response PlayerHeadRightSniper
}

Response PlayerHelpSniper
{
	scene "scenes/Player/Sniper/low/1650.vcd" 
	scene "scenes/Player/Sniper/low/1651.vcd" 
	scene "scenes/Player/Sniper/low/1652.vcd" 
}
Rule PlayerHelpSniper
{
	criteria ConceptPlayerHelp IsSniper
	Response PlayerHelpSniper
}

Response PlayerHelpCaptureSniper
{
	scene "scenes/Player/Sniper/low/1653.vcd" 
	scene "scenes/Player/Sniper/low/1654.vcd" 
	scene "scenes/Player/Sniper/low/1655.vcd" 
}
Rule PlayerHelpCaptureSniper
{
	criteria ConceptPlayerHelp IsSniper IsOnCappableControlPoint
	ApplyContext "SniperHelpCap:1:10"
	Response PlayerHelpCaptureSniper
}

Response PlayerHelpCapture2Sniper
{
	scene "scenes/Player/Sniper/low/1736.vcd" 
	scene "scenes/Player/Sniper/low/1737.vcd" 
}
Rule PlayerHelpCapture2Sniper
{
	criteria ConceptPlayerHelp IsSniper IsOnCappableControlPoint IsHelpCapSniper
	Response PlayerHelpCapture2Sniper
}

// Custom stuff
// Response for when the Sniper is fighting on a cappable point
Response PlayerGetOnPointSniper
{
	scene "scenes/Player/Sniper/low/2289.vcd" 
	scene "scenes/Player/Sniper/low/2290.vcd" 
	scene "scenes/Player/Sniper/low/2291.vcd" 
	scene "scenes/Player/Sniper/low/2418.vcd" 
	scene "scenes/Player/Sniper/low/2419.vcd" 
	scene "scenes/Player/Sniper/low/2420.vcd" 
}

Rule PlayerGetOnPointSniper
{
	criterion ConceptFireWeapon IsSniper IsOnCappableControlPoint NotDefendOnThePointSpeech
	ApplyContext "DefendOnThePointSpeech:1:15"
	applycontexttoworld
	Response PlayerGetOnPointSniper
}
// End custom

Response PlayerHelpDefendSniper
{
	scene "scenes/Player/Sniper/low/1656.vcd" 
	scene "scenes/Player/Sniper/low/1657.vcd" 
	scene "scenes/Player/Sniper/low/1658.vcd" 
}
Rule PlayerHelpDefendSniper
{
	criteria ConceptPlayerHelp IsSniper IsOnFriendlyControlPoint
	Response PlayerHelpDefendSniper
}

Response PlayerMedicSniper
{
	scene "scenes/Player/Sniper/low/1678.vcd" 
	scene "scenes/Player/Sniper/low/1679.vcd" 
}
Rule PlayerMedicSniper
{
	criteria ConceptPlayerMedic IsSniper
	Response PlayerMedicSniper
}

Response PlayerAskForBallSniper
{
}
Rule PlayerAskForBallSniper
{
	criteria ConceptPlayerAskForBall IsSniper
	Response PlayerAskForBallSniper
}

Response PlayerMoveUpSniper
{
	scene "scenes/Player/Sniper/low/1681.vcd" 
	scene "scenes/Player/Sniper/low/1682.vcd" 
}
Rule PlayerMoveUpSniper
{
	criteria ConceptPlayerMoveUp  IsSniper
	Response PlayerMoveUpSniper
}

Response PlayerNoSniper
{
	scene "scenes/Player/Sniper/low/1694.vcd" 
	scene "scenes/Player/Sniper/low/1695.vcd" 
	scene "scenes/Player/Sniper/low/1696.vcd" 
	scene "scenes/Player/Sniper/low/1782.vcd" 
}
Rule PlayerNoSniper
{
	criteria ConceptPlayerNo  IsSniper
	Response PlayerNoSniper
}

Response PlayerThanksSniper
{
	scene "scenes/Player/Sniper/low/1760.vcd" 
	scene "scenes/Player/Sniper/low/1836.vcd" 
}
Rule PlayerThanksSniper
{
	criteria ConceptPlayerThanks IsSniper
	Response PlayerThanksSniper
}

// Custom Assist kill response
// As there is no actual concept for assist kills, this is the second best method.
// Say thanks after you kill more than one person.

Response KilledPlayerAssistSniper
{
	scene "scenes/Player/Sniper/low/1734.vcd"
	scene "scenes/Player/Sniper/low/1735.vcd"
}
Rule KilledPlayerAssistSniper
{
	criteria ConceptPlayerThanks IsSniper IsARecentKill KilledPlayerDelay SniperNotAssistSpeech
	ApplyContext "SniperAssistSpeech:1:20"
	Response KilledPlayerAssistSniper
}
// End custom

Response PlayerYesSniper
{
	scene "scenes/Player/Sniper/low/1767.vcd" 
	scene "scenes/Player/Sniper/low/1768.vcd" 
	scene "scenes/Player/Sniper/low/1769.vcd" 
}
Rule PlayerYesSniper
{
	criteria ConceptPlayerYes  IsSniper
	Response PlayerYesSniper
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 2
//--------------------------------------------------------------------------------------------------------------
Response PlayerActivateChargeSniper
{
	scene "scenes/Player/Sniper/low/1597.vcd" 
	scene "scenes/Player/Sniper/low/1599.vcd" 
	scene "scenes/Player/Sniper/low/1598.vcd" 
	scene "scenes/Player/Sniper/low/1770.vcd" 
}
Rule PlayerActivateChargeSniper
{
	criteria ConceptPlayerActivateCharge IsSniper
	Response PlayerActivateChargeSniper
}

Response PlayerCloakedSpySniper
{
	scene "scenes/Player/Sniper/low/1626.vcd" 
	scene "scenes/Player/Sniper/low/1627.vcd" 
	scene "scenes/Player/Sniper/low/1628.vcd" 
}
Rule PlayerCloakedSpySniper
{
	criteria ConceptPlayerCloakedSpy IsSniper
	Response PlayerCloakedSpySniper
}

Response PlayerDispenserHereSniper
{
	scene "scenes/Player/Sniper/low/1683.vcd" 
}
Rule PlayerDispenserHereSniper
{
	criteria ConceptPlayerDispenserHere IsSniper
	Response PlayerDispenserHereSniper
}

Response PlayerIncomingSniper
{
	scene "scenes/Player/Sniper/low/1659.vcd" 
	scene "scenes/Player/Sniper/low/1660.vcd" 
	scene "scenes/Player/Sniper/low/1661.vcd" 
	scene "scenes/Player/Sniper/low/1773.vcd" 
}
Rule PlayerIncomingSniper
{
	criteria ConceptPlayerIncoming IsSniper
	Response PlayerIncomingSniper
}

Response PlayerSentryAheadSniper
{
	scene "scenes/Player/Sniper/low/1711.vcd" 
}
Rule PlayerSentryAheadSniper
{
	criteria ConceptPlayerSentryAhead IsSniper
	Response PlayerSentryAheadSniper
}

Response PlayerSentryHereSniper
{
	scene "scenes/Player/Sniper/low/1684.vcd" 
}
Rule PlayerSentryHereSniper
{
	criteria ConceptPlayerSentryHere IsSniper
	Response PlayerSentryHereSniper
}

Response PlayerTeleporterHereSniper
{
	scene "scenes/Player/Sniper/low/1685.vcd" 
}
Rule PlayerTeleporterHereSniper
{
	criteria ConceptPlayerTeleporterHere IsSniper
	Response PlayerTeleporterHereSniper
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 3
//--------------------------------------------------------------------------------------------------------------
Response PlayerBattleCrySniper
{
	scene "scenes/Player/Sniper/low/1612.vcd" 
	scene "scenes/Player/Sniper/low/1613.vcd" 
	scene "scenes/Player/Sniper/low/1614.vcd" 
	scene "scenes/Player/Sniper/low/1615.vcd" 
	scene "scenes/Player/Sniper/low/1616.vcd" 
	scene "scenes/Player/Sniper/low/1617.vcd" 
}
Rule PlayerBattleCrySniper
{
	criteria ConceptPlayerBattleCry IsSniper
	Response PlayerBattleCrySniper
}

// Custom stuff - melee dare
// Look at enemy, then do battle cry voice command while holding a melee weapon.
Response MeleeDareCombatSniper
{
	scene "scenes/Player/Sniper/low/2319.vcd"
	scene "scenes/Player/Sniper/low/2320.vcd"
	scene "scenes/Player/Sniper/low/2321.vcd"
	scene "scenes/Player/Sniper/low/2369.vcd"
	scene "scenes/Player/Sniper/low/2372.vcd"
	scene "scenes/Player/Sniper/low/2373.vcd"
	scene "scenes/Player/Sniper/low/2374.vcd"
	scene "scenes/Player/Sniper/low/2388.vcd"
	scene "scenes/Player/Sniper/low/2389.vcd"
}
Rule MeleeDareCombatSniper
{
	criteria ConceptPlayerBattleCry IsWeaponMelee IsSniper IsCrosshairEnemy
	Response MeleeDareCombatSniper
}

// New wepaon lines

Response PlayerShinyCrySniper
{
	scene "scenes/Player/Sniper/low/2284.vcd"
	scene "scenes/Player/Sniper/low/2285.vcd"
	scene "scenes/Player/Sniper/low/2286.vcd"
	scene "scenes/Player/Sniper/low/2287.vcd"
	scene "scenes/Player/Sniper/low/2288.vcd"
	scene "scenes/Player/Sniper/low/2421.vcd"
	scene "scenes/Player/Sniper/low/2422.vcd"
	scene "scenes/Player/Sniper/low/2433.vcd"
	scene "scenes/Player/Sniper/low/2477.vcd"
}
Rule PlayerShinyCrySniper
{
	criteria ConceptPlayerBattleCry IsSniper 30PercentChance IsWeaponPrimary WeaponIsNotVanillaPrimary WeaponIsNotTaggedRifle
	Response PlayerShinyCrySniper
}

Rule PlayerShinyCrySniperBow
{
	criteria ConceptPlayerBattleCry IsSniper 30PercentChance WeaponIsBow WeaponIsNotVanillaPrimary
	Response PlayerShinyCrySniper
}

// This rule is here for future-proofing

Rule PlayerShinyCrySniperSecondary
{
	criteria ConceptPlayerBattleCry IsSniper 30PercentChance IsWeaponSecondary WeaponIsNotVanillaSecondary WeaponIsNotTaggedSMG
	Response PlayerShinyCrySniper
}

// Some achievement weapons seem to use the item1 etc weapon mode, so we use a hard rule for Jarate or Bow
Rule PlayerShinyCrySniperJarate
{
	criteria ConceptPlayerBattleCry IsSniper 30PercentChance WeaponIsJarate WeaponIsNotVanillaSecondary
	Response PlayerShinyCrySniper
}

Rule PlayerShinyCrySniperMelee
{
	criteria ConceptPlayerBattleCry IsSniper 30PercentChance IsWeaponMelee IsNotCrossHairEnemy WeaponIsNotVanillaMelee WeaponIsNotTaggedKukri
	Response PlayerShinyCrySniper
}

//End custom

Response PlayerCheersSniper
{
	scene "scenes/Player/Sniper/low/1618.vcd" 
	scene "scenes/Player/Sniper/low/1619.vcd" 
	scene "scenes/Player/Sniper/low/1620.vcd" 
	scene "scenes/Player/Sniper/low/1621.vcd" 
	scene "scenes/Player/Sniper/low/1622.vcd" 
	scene "scenes/Player/Sniper/low/1623.vcd" 
	scene "scenes/Player/Sniper/low/1624.vcd" 
	scene "scenes/Player/Sniper/low/1625.vcd" 
}
Rule PlayerCheersSniper
{
	criteria ConceptPlayerCheers IsSniper
	Response PlayerCheersSniper
}

Response PlayerGoodJobSniper
{
	scene "scenes/Player/Sniper/low/1641.vcd" 
	scene "scenes/Player/Sniper/low/1642.vcd" 
	scene "scenes/Player/Sniper/low/1643.vcd" 
}
Rule PlayerGoodJobSniper
{
	criteria ConceptPlayerGoodJob IsSniper
	Response PlayerGoodJobSniper
}

Response PlayerJeersSniper
{
	scene "scenes/Player/Sniper/low/1662.vcd" 
	scene "scenes/Player/Sniper/low/1663.vcd" 
	scene "scenes/Player/Sniper/low/1664.vcd" 
	scene "scenes/Player/Sniper/low/1665.vcd" 
	scene "scenes/Player/Sniper/low/1666.vcd" 
	scene "scenes/Player/Sniper/low/1667.vcd" 
	scene "scenes/Player/Sniper/low/1668.vcd" 
	scene "scenes/Player/Sniper/low/1669.vcd" 
}
Rule PlayerJeersSniper
{
	criteria ConceptPlayerJeers IsSniper
	Response PlayerJeersSniper
}

Response PlayerLostPointSniper
{
	scene "scenes/Player/Sniper/low/1686.vcd" 
	scene "scenes/Player/Sniper/low/1687.vcd" 
	scene "scenes/Player/Sniper/low/1688.vcd" 
	scene "scenes/Player/Sniper/low/1689.vcd" 
	scene "scenes/Player/Sniper/low/1690.vcd" 
	scene "scenes/Player/Sniper/low/1778.vcd" 
	scene "scenes/Player/Sniper/low/1779.vcd" 
	scene "scenes/Player/Sniper/low/1780.vcd" 
	scene "scenes/Player/Sniper/low/1781.vcd" 
}
Rule PlayerLostPointSniper
{
	criteria ConceptPlayerLostPoint IsSniper
	Response PlayerLostPointSniper
}

Response PlayerNegativeSniper
{
	scene "scenes/Player/Sniper/low/1686.vcd" 
	scene "scenes/Player/Sniper/low/1687.vcd" 
	scene "scenes/Player/Sniper/low/1688.vcd" 
	scene "scenes/Player/Sniper/low/1689.vcd" 
	scene "scenes/Player/Sniper/low/1690.vcd" 
	scene "scenes/Player/Sniper/low/1778.vcd" 
	scene "scenes/Player/Sniper/low/1779.vcd" 
	scene "scenes/Player/Sniper/low/1780.vcd" 
	scene "scenes/Player/Sniper/low/1781.vcd" 
}
Rule PlayerNegativeSniper
{
	criteria ConceptPlayerNegative IsSniper
	Response PlayerNegativeSniper
}

Response PlayerNiceShotSniper
{
	scene "scenes/Player/Sniper/low/1691.vcd" 
	scene "scenes/Player/Sniper/low/1692.vcd" 
	scene "scenes/Player/Sniper/low/1693.vcd" 
}
Rule PlayerNiceShotSniper
{
	criteria ConceptPlayerNiceShot IsSniper
	Response PlayerNiceShotSniper
}

Response PlayerPositiveSniper
{
	scene "scenes/Player/Sniper/low/1706.vcd" 
	scene "scenes/Player/Sniper/low/1707.vcd" 
	scene "scenes/Player/Sniper/low/1708.vcd" 
	scene "scenes/Player/Sniper/low/1709.vcd" 
	scene "scenes/Player/Sniper/low/1710.vcd" 
	scene "scenes/Player/Sniper/low/1783.vcd" 
	scene "scenes/Player/Sniper/low/1784.vcd" 
	scene "scenes/Player/Sniper/low/1785.vcd" 
	scene "scenes/Player/Sniper/low/1786.vcd" 
	scene "scenes/Player/Sniper/low/1787.vcd" 
}

Response PlayerTauntsSniper
{
	scene "scenes/Player/Sniper/low/1774.vcd" 
	scene "scenes/Player/Sniper/low/1676.vcd" 
	scene "scenes/Player/Sniper/low/1677.vcd" 
	scene "scenes/Player/Sniper/low/1775.vcd" 
	scene "scenes/Player/Sniper/low/1776.vcd" 
	scene "scenes/Player/Sniper/low/1777.vcd" 
}
Rule PlayerPositiveSniper
{
	criteria ConceptPlayerPositive IsSniper
	Response PlayerPositiveSniper
	Response PlayerTauntsSniper
}

//--------------------------------------------------------------------------------------------------------------
// Begin Competitive Mode VO
//--------------------------------------------------------------------------------------------------------------
Response PlayerFirstRoundStartCompSniper
{
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_09.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_10.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_11.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_14.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_15.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_16.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_17.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_comp_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_comp_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_comp_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_comp_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_comp_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_comp_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_comp_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_rare_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_rare_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_rare_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_rare_04.vcd" predelay "1.0, 5.0"
}
Rule PlayerFirstRoundStartCompSniper
{
	criteria ConceptPlayerRoundStartComp IsSniper IsFirstRound IsNotComp6v6 40PercentChance
	Response PlayerFirstRoundStartCompSniper
}

Response PlayerFirstRoundStartComp6sSniper
{
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_09.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_10.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_11.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_14.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_15.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_16.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_17.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_comp_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_comp_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_comp_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_comp_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_comp_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_comp_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_comp_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_rare_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_rare_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_rare_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_rare_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_6s_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_6s_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_6s_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_6s_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamefirst_6s_05.vcd" predelay "1.0, 5.0"
}
Rule PlayerFirstRoundStartComp6sSniper
{
	criteria ConceptPlayerRoundStartComp IsSniper IsFirstRound IsComp6v6 40PercentChance
	Response PlayerFirstRoundStartComp6sSniper
}

Response PlayerWonPrevRoundCompSniper
{
	scene "scenes/Player/Sniper/low/cm_sniper_pregamewonlast_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamewonlast_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamewonlast_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamewonlast_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamewonlast_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamewonlast_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamewonlast_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamewonlast_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamewonlast_09.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamewonlast_rare_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamewonlast_rare_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamewonlast_rare_01.vcd" predelay "1.0, 5.0"
}
Rule PlayerWonPrevRoundCompSniper
{
	criteria ConceptPlayerRoundStartComp IsSniper IsNotFirstRound PlayerWonPreviousRound 40PercentChance
	Response PlayerWonPrevRoundCompSniper
}

Response PlayerLostPrevRoundCompSniper
{
	scene "scenes/Player/Sniper/low/cm_sniper_pregamelostlast_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamelostlast_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamelostlast_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamelostlast_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamelostlast_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamelostlast_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamelostlast_rare_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregamelostlast_rare_02.vcd" predelay "1.0, 5.0"
}
Rule PlayerLostPrevRoundCompSniper
{
	criteria ConceptPlayerRoundStartComp IsSniper IsNotFirstRound PlayerLostPreviousRound PreviousRoundWasNotTie 40PercentChance
	Response PlayerLostPrevRoundCompSniper
}

Response PlayerTiedPrevRoundCompSniper
{
	scene "scenes/Player/Sniper/low/cm_sniper_pregametie_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregametie_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregametie_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregametie_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregametie_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregametie_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_pregametie_rare_01.vcd" predelay "1.0, 5.0"
}
Rule PlayerTiedPrevRoundCompSniper
{
	criteria ConceptPlayerRoundStartComp IsSniper IsNotFirstRound PreviousRoundWasTie 40PercentChance
	Response PlayerTiedPrevRoundCompSniper
}

Response PlayerGameWinCompSniper
{
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_02.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_03.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_04.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_05.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_06.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_07.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_08.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_09.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_10.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_02.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_03.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_04.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_05.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_06.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_07.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_08.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_09.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_10.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_comp_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_comp_02.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_comp_03.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_comp_04.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_comp_05.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_comp_06.vcd" predelay "2.0, 5.0"
}
Rule PlayerGameWinCompSniper
{
	criteria ConceptPlayerGameOverComp PlayerOnWinningTeam IsNotComp6v6 IsSniper 40PercentChance
	Response PlayerGameWinCompSniper
}

Response PlayerGameWinComp6sSniper
{
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_02.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_03.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_04.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_05.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_06.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_07.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_08.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_09.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_10.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_02.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_03.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_04.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_05.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_06.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_07.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_08.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_09.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_10.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_comp_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_comp_02.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_comp_03.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_comp_04.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_comp_05.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_comp_06.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_6s_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Sniper/low/cm_sniper_gamewon_6s_02.vcd" predelay "2.0, 5.0"
}
Rule PlayerGameWinComp6sSniper
{
	criteria ConceptPlayerGameOverComp PlayerOnWinningTeam IsComp6v6 IsSniper 40PercentChance
	Response PlayerGameWinComp6sSniper
}

Response PlayerMatchWinCompSniper
{
	scene "scenes/Player/Sniper/low/cm_sniper_matchwon_01.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Sniper/low/cm_sniper_matchwon_02.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Sniper/low/cm_sniper_matchwon_03.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Sniper/low/cm_sniper_matchwon_04.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Sniper/low/cm_sniper_matchwon_05.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Sniper/low/cm_sniper_matchwon_06.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Sniper/low/cm_sniper_matchwon_07.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Sniper/low/cm_sniper_matchwon_08.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Sniper/low/cm_sniper_matchwon_09.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Sniper/low/cm_sniper_matchwon_10.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Sniper/low/cm_sniper_matchwon_11.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Sniper/low/cm_sniper_matchwon_12.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Sniper/low/cm_sniper_matchwon_13.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Sniper/low/cm_sniper_matchwon_14.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Sniper/low/cm_sniper_matchwon_15.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Sniper/low/cm_sniper_matchwon_16.vcd" predelay "1.0, 2.0"
}
Rule PlayerMatchWinCompSniper
{
	criteria ConceptPlayerMatchOverComp PlayerOnWinningTeam IsSniper 40PercentChance
	Response PlayerMatchWinCompSniper
}
//--------------------------------------------------------------------------------------------------------------
// End Competitive Mode VO
//--------------------------------------------------------------------------------------------------------------