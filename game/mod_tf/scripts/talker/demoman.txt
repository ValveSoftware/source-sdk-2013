//--------------------------------------------------------------------------------------------------------------
// Demoman Response Rule File
//--------------------------------------------------------------------------------------------------------------

Criterion "DemomanIsKillSpeechObject" "DemomanKillSpeechObject" "1" "required" weight 0
Criterion "DemomanIsNotStillonFire" "DemomanOnFire" "!=1" "required" weight 0
Criterion "DemomanIsStillonFire" "DemomanOnFire" "1" "required" weight 0
Criterion "DemomanNotKillSpeech" "DemomanKillSpeech" "!=1" "required" weight 0
Criterion "DemomanNotKillSpeechMelee" "DemomanKillSpeechMelee" "!=1" "required" weight 0
Criterion "DemomanNotSaidHealThanks" "DemomanSaidHealThanks" "!=1" "required"
Criterion "IsHelpCapDemoman" "DemomanHelpCap" "1" "required" weight 0
// Custom stuff
Criterion "DemomanNotInvulnerableSpeech" "DemomanInvulnerableSpeech" "!=1" "required" weight 0
Criterion "DemomanNotAssistSpeech" "DemomanAssistSpeech" "!=1" "required" weight 0
Criterion "IsDrunk" "NotSober" "1" "required" weight 0
Criterion "DemomanNotAwardSpeech" "DemomanAwardSpeech" "!=1" "required" weight 0


Response PlayerCloakedSpyDemomanDemoman
{
	scene "scenes/Player/Demoman/low/901.vcd" 
}
Rule PlayerCloakedSpyDemomanDemoman
{
	criteria ConceptPlayerCloakedSpy IsDemoman IsOnDemoman
	Response PlayerCloakedSpyDemomanDemoman
}

Response PlayerCloakedSpyEngineerDemoman
{
	scene "scenes/Player/Demoman/low/907.vcd" 
}
Rule PlayerCloakedSpyEngineerDemoman
{
	criteria ConceptPlayerCloakedSpy IsDemoman IsOnEngineer
	Response PlayerCloakedSpyEngineerDemoman
}

Response PlayerCloakedSpyHeavyDemoman
{
	scene "scenes/Player/Demoman/low/897.vcd" 
}
Rule PlayerCloakedSpyHeavyDemoman
{
	criteria ConceptPlayerCloakedSpy IsDemoman IsOnHeavy
	Response PlayerCloakedSpyHeavyDemoman
}

Response PlayerCloakedSpyMedicDemoman
{
	scene "scenes/Player/Demoman/low/905.vcd" 
}
Rule PlayerCloakedSpyMedicDemoman
{
	criteria ConceptPlayerCloakedSpy IsDemoman IsOnMedic
	Response PlayerCloakedSpyMedicDemoman
}

Response PlayerCloakedSpyPyroDemoman
{
	scene "scenes/Player/Demoman/low/899.vcd" 
}
Rule PlayerCloakedSpyPyroDemoman
{
	criteria ConceptPlayerCloakedSpy IsDemoman IsOnPyro
	Response PlayerCloakedSpyPyroDemoman
}

Response PlayerCloakedSpyScoutDemoman
{
	scene "scenes/Player/Demoman/low/893.vcd" 
}
Rule PlayerCloakedSpyScoutDemoman
{
	criteria ConceptPlayerCloakedSpy IsDemoman IsOnScout
	Response PlayerCloakedSpyScoutDemoman
}

Response PlayerCloakedSpySniperDemoman
{
	scene "scenes/Player/Demoman/low/909.vcd" 
}
Rule PlayerCloakedSpySniperDemoman
{
	criteria ConceptPlayerCloakedSpy IsDemoman IsOnSniper
	Response PlayerCloakedSpySniperDemoman
}

Response PlayerCloakedSpySoldierDemoman
{
	scene "scenes/Player/Demoman/low/895.vcd" 
}
Rule PlayerCloakedSpySoldierDemoman
{
	criteria ConceptPlayerCloakedSpy IsDemoman IsOnSoldier
	Response PlayerCloakedSpySoldierDemoman
}

Response PlayerCloakedSpySpyDemoman
{
	scene "scenes/Player/Demoman/low/903.vcd" 
}
Rule PlayerCloakedSpySpyDemoman
{
	criteria ConceptPlayerCloakedSpy IsDemoman IsOnSpy
	Response PlayerCloakedSpySpyDemoman
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech
//--------------------------------------------------------------------------------------------------------------
Response HealThanksDemoman
{
	scene "scenes/Player/Demoman/low/1032.vcd" 
	scene "scenes/Player/Demoman/low/1033.vcd" 
	scene "scenes/Player/Demoman/low/1034.vcd" 
}
Rule HealThanksDemoman
{
	criteria ConceptMedicChargeStopped IsDemoman SuperHighHealthContext DemomanNotSaidHealThanks 50PercentChance
	ApplyContext "DemomanSaidHealThanks:1:20"
	Response HealThanksDemoman
}

Response AwardDemoman
{
	scene "scenes/Player/Demoman/low/866.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/868.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/865.vcd" predelay "2.5"
}
Rule AwardDemoman
{
	criteria ConceptAchievementAward IsDemoman DemomanNotAwardSpeech
	ApplyContext "DemomanAwardSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response AwardDemoman
}

Response PlayerRoundStartDemoman
{
	scene "scenes/Player/Demoman/low/1358.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/1369.vcd" predelay "1.0, 5.0" 
	scene "scenes/Player/Demoman/low/876.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/877.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/878.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/879.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/880.vcd" predelay "1.0, 5.0"
}
Rule PlayerRoundStartDemoman
{
	criteria ConceptPlayerRoundStart IsDemoman
	Response PlayerRoundStartDemoman
}

Response PlayerCappedIntelligenceDemoman
{
	scene "scenes/Player/Demoman/low/866.vcd" 
	scene "scenes/Player/Demoman/low/867.vcd" 
}
Rule PlayerCappedIntelligenceDemoman
{
	criteria ConceptPlayerCapturedIntelligence IsDemoman
	Response PlayerCappedIntelligenceDemoman
}

Response PlayerCapturedPointDemoman
{
	scene "scenes/Player/Demoman/low/863.vcd" 
	scene "scenes/Player/Demoman/low/865.vcd" 
	scene "scenes/Player/Demoman/low/864.vcd" 
}
Rule PlayerCapturedPointDemoman
{
	criteria ConceptPlayerCapturedPoint IsDemoman
	Response PlayerCapturedPointDemoman
}

Response PlayerSuddenDeathDemoman
{
	scene "scenes/Player/Demoman/low/938.vcd" 
	scene "scenes/Player/Demoman/low/939.vcd" 
	scene "scenes/Player/Demoman/low/940.vcd" 
	scene "scenes/Player/Demoman/low/941.vcd" 
	scene "scenes/Player/Demoman/low/942.vcd" 
	scene "scenes/Player/Demoman/low/944.vcd" 
	scene "scenes/Player/Demoman/low/945.vcd" 
	scene "scenes/Player/Demoman/low/947.vcd" 
	scene "scenes/Player/Demoman/low/948.vcd" 
	scene "scenes/Player/Demoman/low/949.vcd" 
	scene "scenes/Player/Demoman/low/946.vcd" 
}
Rule PlayerSuddenDeathDemoman
{
	criteria ConceptPlayerSuddenDeathStart IsDemoman
	Response PlayerSuddenDeathDemoman
}

Response PlayerStalemateDemoman
{
	scene "scenes/Player/Demoman/low/869.vcd" 
	scene "scenes/Player/Demoman/low/870.vcd" 
	scene "scenes/Player/Demoman/low/871.vcd" 
	scene "scenes/Player/Demoman/low/1357.vcd" 
}
Rule PlayerStalemateDemoman
{
	criteria ConceptPlayerStalemate IsDemoman
	Response PlayerStalemateDemoman
}

Response PlayerTeleporterThanksDemoman
{
	scene "scenes/Player/Demoman/low/1035.vcd" 
	scene "scenes/Player/Demoman/low/1036.vcd" 
}
Rule PlayerTeleporterThanksDemoman
{
	criteria ConceptTeleported IsNotEngineer IsDemoman 30PercentChance
	Response PlayerTeleporterThanksDemoman
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Combat
//--------------------------------------------------------------------------------------------------------------
Response DefendOnThePointDemoman
{
	scene "scenes/Player/Demoman/low/1011.vcd" 
	scene "scenes/Player/Demoman/low/1389.vcd" 
}
Rule DefendOnThePointDemoman
{
	criteria ConceptFireWeapon IsDemoman IsOnFriendlyControlPoint NotDefendOnThePointSpeech
	ApplyContext "DefendOnThePointSpeech:1:30"
	applycontexttoworld
	Response DefendOnThePointDemoman
}

Response DemomanJarateHit
{
	scene "scenes/Player/Demoman/low/871.vcd"       
	scene "scenes/Player/Demoman/low/947.vcd"       
	scene "scenes/Player/Demoman/low/969.vcd"       
	scene "scenes/Player/Demoman/low/971.vcd"       
}
Rule DemomanJarateHit
{
	criteria ConceptJarateHit IsDemoman 50PercentChance
	Response DemomanJarateHit
}

// Invuln responses for Grenade Launcher
Response InvulnerableSpeechDemoman2
{
	scene "scenes/Player/Demoman/low/1019.vcd" 
	scene "scenes/Player/Demoman/low/1012.vcd" 
	scene "scenes/Player/Demoman/low/1029.vcd" 
	scene "scenes/Player/Demoman/low/1021.vcd"
}


Rule InvulnerableSpeechDemoman2
{
	criteria ConceptFireWeapon IsDemoman WeaponIsGrenade IsInvulnerable DemomanNotInvulnerableSpeech
	ApplyContext "DemomanInvulnerableSpeech:1:30"
	Response InvulnerableSpeechDemoman2
}
// End invuln responses for GL

// Invulnerable responses for Sticky launcher
Response InvulnerableSpeechDemoman
{
	scene "scenes/Player/Demoman/low/1023.vcd"  
	scene "scenes/Player/Demoman/low/1022.vcd" 
	scene "scenes/Player/Demoman/low/1018.vcd" 
}

Rule InvulnerableSpeechDemoman
{
	criteria ConceptFireWeapon IsDemoman WeaponIsPipebomb IsInvulnerable DemomanNotInvulnerableSpeech
	ApplyContext "DemomanInvulnerableSpeech:1:30"
	Response InvulnerableSpeechDemoman
}
// End invuln responses for Sticky

Response KilledPlayerAssistAutoDemoman
{
	scene "scenes/Player/Demoman/low/1009.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/867.vcd" predelay "2.5"
}
Rule KilledPlayerAssistAutoDemoman
{
	criteria ConceptKilledPlayer IsDemoman IsBeingHealed IsManyRecentKills KilledPlayerDelay 20PercentChance DemomanNotAssistSpeech
	ApplyContext "DemomanAssistSpeech:1:20"
	Response KilledPlayerAssistAutoDemoman
}

Response KilledPlayerManyDemoman
{
	scene "scenes/Player/Demoman/low/1000.vcd" 
	scene "scenes/Player/Demoman/low/1014.vcd" 
	scene "scenes/Player/Demoman/low/1016.vcd" 
	scene "scenes/Player/Demoman/low/1020.vcd" 
}
Rule KilledPlayerManyDemoman
{
	criteria ConceptKilledPlayer IsManyRecentKills 30PercentChance IsWeaponSecondary KilledPlayerDelay DemomanNotKillSpeech IsDemoman
	ApplyContext "DemomanKillSpeech:1:10"
	Response KilledPlayerManyDemoman
}

// Custom stuff
// Responses against killing a Soldier
Response KilledSoldierDemoman
{
	scene "scenes/Player/Demoman/low/3568.vcd" 
	scene "scenes/Player/Demoman/low/3574.vcd" 
	scene "scenes/Player/Demoman/low/3577.vcd" 
}
Rule KilledSoldierDemoman
{
	criterion ConceptKilledPlayer KilledPlayerDelay IsVictimSoldier 10PercentChance DemomanNotKillSpeech IsDemoman WeaponClassIsNotAxe
	ApplyContext "DemomanKillSpeech:1:60"
	Response KilledSoldierDemoman
}

// Frying pan responses
// These are our base responses for non-sword melee weapons.
Response KilledPlayerPanDemoman
{
	scene "scenes/Player/Demoman/low/3566.vcd" 
	scene "scenes/Player/Demoman/low/3567.vcd" 
	scene "scenes/Player/Demoman/low/3569.vcd"
	scene "scenes/Player/Demoman/low/3570.vcd" 
	scene "scenes/Player/Demoman/low/3571.vcd" 
	scene "scenes/Player/Demoman/low/3572.vcd" 
	scene "scenes/Player/Demoman/low/3573.vcd"  
	scene "scenes/Player/Demoman/low/3575.vcd"
}
Rule KilledPlayerPanDemoman
{
	criteria ConceptKilledPlayer KilledPlayerDelay WeaponIsFryingPan 20PercentChance  DemomanNotKillSpeechMelee IsDemoman
	ApplyContext "DemomanKillSpeechMelee:1:10"
	Response KilledPlayerPanDemoman
}

Rule KilledPlayerSaxxyDemoman
{
	criteria ConceptKilledPlayer KilledPlayerDelay WeaponIsSaxxy 20PercentChance  DemomanNotKillSpeechMelee IsDemoman
	ApplyContext "DemomanKillSpeechMelee:1:10"
	Response KilledPlayerPanDemoman
}
// End Frying Pan responses

// Custom rules for when Demoman drinks from Scrumpy
// to trigger one of the above drunk lines
Rule DrunkDemoman
{
	criteria ConceptFireWeapon IsDrunk IsDemoman IsWeaponMelee WeaponIsNotFryingPan // Fire if he's holding the Bottle and if the drunk context is set
	Response KilledPlayerPanDemoman
}
// End custom drunk rules

// Modified to play for the Bottle only
// Modified to include Frying Pan repsonses above, saves having duplicate entries.
Response KilledPlayerMeleeDemoman
{
	scene "scenes/Player/Demoman/low/998.vcd" 
}
Rule KilledPlayerMeleeDemoman
{
	criteria ConceptKilledPlayer KilledPlayerDelay WeaponIsBottle WeaponIsNotPainTrain WeaponIsNotSaxxy 30PercentChance  DemomanNotKillSpeechMelee IsDemoman
	ApplyContext "DemomanKillSpeechMelee:1:10"
	Response KilledPlayerPanDemoman
	Response KilledPlayerMeleeDemoman
}
//

// Eyelander responses
// These apply to all tf_weapon_sword class weapons.
Response KilledPlayerSwordDemoman
{
	scene "scenes/Player/Demoman/low/3565.vcd" 
	scene "scenes/Player/Demoman/low/910.vcd" 
	scene "scenes/Player/Demoman/low/950.vcd"	
	scene "scenes/Player/Demoman/low/1379.vcd"	
}

Rule KilledPlayerSwordDemoman
{
	criteria ConceptKilledPlayer KilledPlayerDelay WeaponIsSword 20PercentChance IsNotVictimDemoman  DemomanNotKillSpeechMelee IsDemoman
	ApplyContext "DemomanKillSpeechMelee:1:30"
	Response KilledPlayerSwordDemoman
}

// The above rule is for non-Demoman victims only.

Response KilledPlayerSword2Demoman
{
	scene "scenes/Player/Demoman/low/3564.vcd"
}

Rule KilledPlayerSword2Demoman
{
	criteria ConceptKilledPlayer KilledPlayerDelay WeaponIsSword 20PercentChance IsVictimDemoman  DemomanNotKillSpeechMelee IsDemoman
	ApplyContext "DemomanKillSpeechMelee:1:30"
	Response KilledPlayerSword2Demoman
}
// The above rule is for Demoman victims only.
// End Eyelander responses

// Katana response rules shared with Eyelander
Rule KilledPlayerKatanaDemoman
{
	criteria ConceptKilledPlayer KilledPlayerDelay WeaponIsKatana 20PercentChance IsNotVictimDemoman  DemomanNotKillSpeechMelee IsDemoman
	ApplyContext "DemomanKillSpeechMelee:1:30"
	Response KilledPlayerSwordDemoman
}
// The above rule is for non-Demoman victims only.
Rule KilledPlayerKatana2Demoman
{
	criteria ConceptKilledPlayer KilledPlayerDelay WeaponIsKatana 20PercentChance IsVictimDemoman  DemomanNotKillSpeechMelee IsDemoman
	ApplyContext "DemomanKillSpeechMelee:1:30"
	Response KilledPlayerSword2Demoman
}
// The above rule is for Demoman victims only.
// End Katana responses

// Pain Train responses
// Shares responses with Frying Pan, saves having duplicate entries.
Response KilledPlayerClubDemoman
{
	scene "scenes/Player/Demoman/low/3578.vcd" 
}

Rule KilledPlayerClubDemoman
{
	criteria ConceptKilledPlayer KilledPlayerDelay WeaponIsPainTrain 10PercentChance  DemomanNotKillSpeechMelee IsDemoman
	ApplyContext "DemomanKillSpeechMelee:1:15"
	Response KilledPlayerClubDemoman
	Response KilledPlayerPanDemoman
}
// End Pain Train responses

// Caber responses
Response KilledPlayerCaberDemoman
{
	scene "scenes/player/Demoman/low/1007.vcd"
	scene "scenes/Player/Demoman/low/1008.vcd"
	scene "scenes/Player/Demoman/low/997.vcd"
}
Rule KilledPlayerCaberDemoman
{
	criteria ConceptAttackerPain KilledPlayerDelay WeaponIsCaber IsCritical CaberHealthContext  DemomanNotKillSpeechMelee IsDemoman
	ApplyContext "DemomanKillSpeechMelee:1:10"
	Response KilledPlayerCaberDemoman
}

Response KilledPlayerVeryManyDemoman
{
	scene "scenes/Player/Demoman/low/997.vcd" 
	scene "scenes/Player/Demoman/low/999.vcd" 
	scene "scenes/Player/Demoman/low/1003.vcd" 
	scene "scenes/Player/Demoman/low/1004.vcd" 
	scene "scenes/Player/Demoman/low/1006.vcd" 
	scene "scenes/Player/Demoman/low/1024.vcd" 
}
Rule KilledPlayerVeryManyDemoman
{
	criteria ConceptKilledPlayer IsVeryManyRecentKills 50PercentChance IsWeaponSecondary KilledPlayerDelay DemomanNotKillSpeech IsDemoman
	ApplyContext "DemomanKillSpeech:1:10"
	Response KilledPlayerVeryManyDemoman
}

Response PlayerKilledCapperDemoman
{
	scene "scenes/Player/Demoman/low/867.vcd" 
	scene "scenes/Player/Demoman/low/884.vcd" 
	scene "scenes/Player/Demoman/low/887.vcd" 
	scene "scenes/Player/Demoman/low/952.vcd" 
	scene "scenes/Player/Demoman/low/955.vcd" 
	scene "scenes/Player/Demoman/low/989.vcd" 
	scene "scenes/Player/Demoman/low/991.vcd" 
	scene "scenes/Player/Demoman/low/992.vcd" 
	scene "scenes/Player/Demoman/low/993.vcd" 
	scene "scenes/Player/Demoman/low/1001.vcd" 
	scene "scenes/Player/Demoman/low/1002.vcd" 
}
Rule PlayerKilledCapperDemoman
{
	criteria ConceptCapBlocked IsDemoman
	ApplyContext "DemomanKillSpeech:1:10"
	Response PlayerKilledCapperDemoman
}

Response PlayerKilledDominatingDemoman
{
	scene "scenes/Player/Demoman/low/886.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/910.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/950.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/1379.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/1384.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/1385.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/951.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/953.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/954.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/956.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/1383.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingDemoman
{
	criteria ConceptKilledPlayer IsDemoman IsDominated
	ApplyContext "DemomanKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingDemoman
}

Response PlayerKilledDominatingDemomanDemoman
{
	scene "scenes/Player/Demoman/low/3515.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3516.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3517.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3518.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingDemomanDemoman
{
	criteria ConceptKilledPlayer IsDemoman IsDominated  IsVictimDemoman
	ApplyContext "DemomanKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingDemomanDemoman
}

Response PlayerKilledDominatingEngineerDemoman
{
	scene "scenes/Player/Demoman/low/3519.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3520.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3521.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3522.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3523.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3524.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingEngineerDemoman
{
	criteria ConceptKilledPlayer IsDemoman IsDominated  IsVictimEngineer
	ApplyContext "DemomanKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingEngineerDemoman
}

Response PlayerKilledDominatingHeavyDemoman
{
	scene "scenes/Player/Demoman/low/3525.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3526.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3527.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3528.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3529.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingHeavyDemoman
{
	criteria ConceptKilledPlayer IsDemoman IsDominated  IsVictimHeavy
	ApplyContext "DemomanKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingHeavyDemoman
}

Response PlayerKilledDominatingMedicDemoman
{
	scene "scenes/Player/Demoman/low/3530.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3531.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3532.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3533.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingMedicDemoman
{
	criteria ConceptKilledPlayer IsDemoman IsDominated  IsVictimMedic
	ApplyContext "DemomanKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingMedicDemoman
}

Response PlayerKilledDominatingPyroDemoman
{
	scene "scenes/Player/Demoman/low/3534.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3535.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3536.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3537.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingPyroDemoman
{
	criteria ConceptKilledPlayer IsDemoman IsDominated  IsVictimPyro
	ApplyContext "DemomanKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingPyroDemoman
}

Response PlayerKilledDominatingScoutDemoman
{
	scene "scenes/Player/Demoman/low/3538.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3539.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3540.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3541.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3542.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3543.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3544.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3545.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingScoutDemoman
{
	criteria ConceptKilledPlayer IsDemoman IsDominated  IsVictimScout
	ApplyContext "DemomanKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingScoutDemoman
}

Response PlayerKilledDominatingSniperDemoman
{
	scene "scenes/Player/Demoman/low/3546.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3547.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3548.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3549.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingSniperDemoman
{
	criteria ConceptKilledPlayer IsDemoman IsDominated  IsVictimSniper
	ApplyContext "DemomanKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingSniperDemoman
}

Response PlayerKilledDominatingSoldierDemoman
{
	scene "scenes/Player/Demoman/low/3550.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3551.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3552.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3553.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3554.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingSoldierDemoman
{
	criteria ConceptKilledPlayer IsDemoman IsDominated  IsVictimSoldier
	ApplyContext "DemomanKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingSoldierDemoman
}

Response PlayerKilledDominatingSpyDemoman
{
	scene "scenes/Player/Demoman/low/3561.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3562.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/3563.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingSpyDemoman
{
	criteria ConceptKilledPlayer IsDemoman IsDominated  IsVictimSpy
	ApplyContext "DemomanKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingSpyDemoman
}

Response PlayerKilledForRevengeDemoman
{
	scene "scenes/Player/Demoman/low/914.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/990.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/1005.vcd" predelay "2.5"
	scene "scenes/Player/Demoman/low/1025.vcd" predelay "2.5"
}
Rule PlayerKilledForRevengeDemoman
{
	criteria ConceptKilledPlayer IsDemoman IsRevenge
	ApplyContext "DemomanKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledForRevengeDemoman
}

Response PlayerKilledObjectDemoman
{
	scene "scenes/Player/Demoman/low/1007.vcd" 
	scene "scenes/Player/Demoman/low/1008.vcd" 
}
Rule PlayerKilledObjectDemoman
{
	criteria ConceptKilledObject IsDemoman 30PercentChance IsARecentKill
	ApplyContext "DemomanKillSpeechObject:1:30"
	Response PlayerKilledObjectDemoman
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Pain
//--------------------------------------------------------------------------------------------------------------
Response PlayerAttackerPainDemoman
{
	scene "scenes/Player/Demoman/low/983.vcd" 
	scene "scenes/Player/Demoman/low/984.vcd" 
	scene "scenes/Player/Demoman/low/985.vcd" 
	scene "scenes/Player/Demoman/low/1396.vcd" 
}
Rule PlayerAttackerPainDemoman
{
	criteria ConceptAttackerPain IsDemoman IsNotDominating
	Response PlayerAttackerPainDemoman
}

Response PlayerOnFireDemoman
{
	scene "scenes/Player/Demoman/low/872.vcd" 
	scene "scenes/Player/Demoman/low/874.vcd" 
}
Rule PlayerOnFireDemoman
{
	criteria ConceptFire IsDemoman DemomanIsNotStillonFire IsNotDominating
	ApplyContext "DemomanOnFire:1:7"
	Response PlayerOnFireDemoman
}

Response PlayerOnFireRareDemoman
{
	scene "scenes/Player/Demoman/low/873.vcd" 
}
Rule PlayerOnFireRareDemoman
{
	criteria ConceptFire IsDemoman 10PercentChance DemomanIsNotStillonFire IsNotDominating
	ApplyContext "DemomanOnFire:1:7"
	Response PlayerOnFireRareDemoman
}

Response PlayerPainDemoman
{
	scene "scenes/Player/Demoman/low/986.vcd" 
	scene "scenes/Player/Demoman/low/987.vcd" 
	scene "scenes/Player/Demoman/low/988.vcd" 
	scene "scenes/Player/Demoman/low/1392.vcd" 
	scene "scenes/Player/Demoman/low/1393.vcd" 
	scene "scenes/Player/Demoman/low/1394.vcd" 
	scene "scenes/Player/Demoman/low/1395.vcd" 
}
Rule PlayerPainDemoman
{
	criteria ConceptPain IsDemoman IsNotDominating
	Response PlayerPainDemoman
}

Response PlayerStillOnFireDemoman
{
	scene "scenes/Player/Demoman/low/1927.vcd" 
}
Rule PlayerStillOnFireDemoman
{
	criteria ConceptFire IsDemoman  DemomanIsStillonFire IsNotDominating
	ApplyContext "DemomanOnFire:1:7"
	Response PlayerStillOnFireDemoman
}


//--------------------------------------------------------------------------------------------------------------
// Duel Speech
//--------------------------------------------------------------------------------------------------------------
Response AcceptedDuelDemoman
{
	scene "scenes/Player/Demoman/low/1015.vcd" 
	scene "scenes/Player/Demoman/low/1017.vcd" 
	scene "scenes/Player/Demoman/low/1038.vcd" 
}
Rule AcceptedDuelDemoman
{
	criteria ConceptIAcceptDuel IsDemoman
	Response AcceptedDuelDemoman
}

Response MeleeDareDemoman
{
	scene "scenes/Player/Demoman/low/868.vcd" 
	scene "scenes/Player/Demoman/low/879.vcd" 
	scene "scenes/Player/Demoman/low/1369.vcd" 
	scene "scenes/Player/Demoman/low/1027.vcd" 
	scene "scenes/Player/Demoman/low/1028.vcd" 
	scene "scenes/Player/Demoman/low/3566.vcd" 
	scene "scenes/Player/Demoman/low/3569.vcd" 
	scene "scenes/Player/Demoman/low/3574.vcd" 
	scene "scenes/Player/Demoman/low/3576.vcd" 
	scene "scenes/Player/Demoman/low/3578.vcd" 
}
Rule MeleeDareDemoman
{
	criteria ConceptRequestDuel IsDemoman
	Response MeleeDareDemoman
}

Response RejectedDuelDemoman
{
	scene "scenes/Player/Demoman/low/938.vcd" 
	scene "scenes/Player/Demoman/low/939.vcd" 
	scene "scenes/Player/Demoman/low/3545.vcd" 
}
Rule RejectedDuelDemoman
{
	criteria ConceptDuelRejected IsDemoman
	Response RejectedDuelDemoman
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 1
//--------------------------------------------------------------------------------------------------------------
Response PlayerGoDemoman
{
	scene "scenes/Player/Demoman/low/911.vcd" 
	scene "scenes/Player/Demoman/low/912.vcd" 
	scene "scenes/Player/Demoman/low/913.vcd" 
}
Rule PlayerGoDemoman
{
	criteria ConceptPlayerGo IsDemoman
	Response PlayerGoDemoman
}

Response PlayerHeadLeftDemoman
{
	scene "scenes/Player/Demoman/low/917.vcd" 
	scene "scenes/Player/Demoman/low/918.vcd" 
	scene "scenes/Player/Demoman/low/919.vcd" 
}
Rule PlayerHeadLeftDemoman
{
	criteria ConceptPlayerLeft  IsDemoman
	Response PlayerHeadLeftDemoman
}

Response PlayerHeadRightDemoman
{
	scene "scenes/Player/Demoman/low/920.vcd" 
	scene "scenes/Player/Demoman/low/921.vcd" 
	scene "scenes/Player/Demoman/low/922.vcd" 
}
Rule PlayerHeadRightDemoman
{
	criteria ConceptPlayerRight  IsDemoman
	Response PlayerHeadRightDemoman
}

Response PlayerHelpDemoman
{
	scene "scenes/Player/Demoman/low/923.vcd" 
	scene "scenes/Player/Demoman/low/924.vcd" 
	scene "scenes/Player/Demoman/low/925.vcd" 
}
Rule PlayerHelpDemoman
{
	criteria ConceptPlayerHelp IsDemoman
	Response PlayerHelpDemoman
}

Response PlayerHelpCaptureDemoman
{
	scene "scenes/Player/Demoman/low/926.vcd" 
	scene "scenes/Player/Demoman/low/927.vcd" 
	scene "scenes/Player/Demoman/low/928.vcd" 
}
Rule PlayerHelpCaptureDemoman
{
	criteria ConceptPlayerHelp IsDemoman IsOnCappableControlPoint
	ApplyContext "DemomanHelpCap:1:10"
	Response PlayerHelpCaptureDemoman
}

Response PlayerHelpCapture2Demoman
{
	scene "scenes/Player/Demoman/low/1011.vcd" 
	scene "scenes/Player/Demoman/low/1389.vcd" 
}
Rule PlayerHelpCapture2Demoman
{
	criteria ConceptPlayerHelp IsDemoman IsOnCappableControlPoint IsHelpCapDemoman
	Response PlayerHelpCapture2Demoman
}

Response PlayerHelpDefendDemoman
{
	scene "scenes/Player/Demoman/low/929.vcd" 
	scene "scenes/Player/Demoman/low/930.vcd" 
	scene "scenes/Player/Demoman/low/931.vcd" 
}
Rule PlayerHelpDefendDemoman
{
	criteria ConceptPlayerHelp IsDemoman IsOnFriendlyControlPoint
	Response PlayerHelpDefendDemoman
}

Response PlayerMedicDemoman
{
	scene "scenes/Player/Demoman/low/957.vcd" 
	scene "scenes/Player/Demoman/low/958.vcd" 
	scene "scenes/Player/Demoman/low/959.vcd" 
}
Rule PlayerMedicDemoman
{
	criteria ConceptPlayerMedic IsDemoman
	Response PlayerMedicDemoman
}

Response PlayerAskForBallDemoman
{
}
Rule PlayerAskForBallDemoman
{
	criteria ConceptPlayerAskForBall IsDemoman
	Response PlayerAskForBallDemoman
}


Response PlayerMoveUpDemoman
{
	scene "scenes/Player/Demoman/low/960.vcd" 
	scene "scenes/Player/Demoman/low/961.vcd" 
	scene "scenes/Player/Demoman/low/962.vcd" 
}
Rule PlayerMoveUpDemoman
{
	criteria ConceptPlayerMoveUp  IsDemoman
	Response PlayerMoveUpDemoman
}

Response PlayerNoDemoman
{
	scene "scenes/Player/Demoman/low/977.vcd" 
	scene "scenes/Player/Demoman/low/978.vcd" 
	scene "scenes/Player/Demoman/low/979.vcd" 
}
Rule PlayerNoDemoman
{
	criteria ConceptPlayerNo  IsDemoman
	Response PlayerNoDemoman
}

Response PlayerThanksDemoman
{
	scene "scenes/Player/Demoman/low/1030.vcd" 
	scene "scenes/Player/Demoman/low/1031.vcd" 
}
Rule PlayerThanksDemoman
{
	criteria ConceptPlayerThanks IsDemoman
	Response PlayerThanksDemoman
}

// Custom Assist kill response
// As there is no actual concept for assist kills, this is the second best method.
// Say thanks after you kill more than one person.

Response KilledPlayerAssistDemoman
{
	scene "scenes/Player/Demoman/low/1009.vcd"
	scene "scenes/Player/Demoman/low/867.vcd"
}
Rule KilledPlayerAssistDemoman
{
	criteria ConceptPlayerThanks IsDemoman IsARecentKill KilledPlayerDelay DemomanNotAssistSpeech
	ApplyContext "DemomanAssistSpeech:1:20"
	Response KilledPlayerAssistDemoman
}

Response KilledPlayerAssistDemoman2
{
	scene "scenes/Player/Demoman/low/1010.vcd"
}
Rule KilledPlayerAssistDemoman2
{
	criteria ConceptPlayerThanks IsDemoman IsNotBeingHealed IsARecentKill KilledPlayerDelay DemomanNotAssistSpeech
	ApplyContext "DemomanAssistSpeech:1:20"
	Response KilledPlayerAssistDemoman
	Response KilledPlayerAssistDemoman2
}

// End custom

Response PlayerYesDemoman
{
	scene "scenes/Player/Demoman/low/1037.vcd" 
	scene "scenes/Player/Demoman/low/1038.vcd" 
	scene "scenes/Player/Demoman/low/1039.vcd" 
}
Rule PlayerYesDemoman
{
	criteria ConceptPlayerYes  IsDemoman
	Response PlayerYesDemoman
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 2
//--------------------------------------------------------------------------------------------------------------
Response PlayerActivateChargeDemoman
{
	scene "scenes/Player/Demoman/low/860.vcd" 
	scene "scenes/Player/Demoman/low/861.vcd" 
	scene "scenes/Player/Demoman/low/862.vcd" 
}
Rule PlayerActivateChargeDemoman
{
	criteria ConceptPlayerActivateCharge IsDemoman
	Response PlayerActivateChargeDemoman
}

Response PlayerCloakedSpyDemoman
{
	scene "scenes/Player/Demoman/low/889.vcd" 
	scene "scenes/Player/Demoman/low/890.vcd" 
	scene "scenes/Player/Demoman/low/891.vcd" 
}
Rule PlayerCloakedSpyDemoman
{
	criteria ConceptPlayerCloakedSpy IsDemoman
	Response PlayerCloakedSpyDemoman
}

Response PlayerDispenserHereDemoman
{
	scene "scenes/Player/Demoman/low/964.vcd" 
}
Rule PlayerDispenserHereDemoman
{
	criteria ConceptPlayerDispenserHere IsDemoman
	Response PlayerDispenserHereDemoman
}

Response PlayerIncomingDemoman
{
	scene "scenes/Player/Demoman/low/932.vcd" 
	scene "scenes/Player/Demoman/low/933.vcd" 
	scene "scenes/Player/Demoman/low/934.vcd" 
}
Rule PlayerIncomingDemoman
{
	criteria ConceptPlayerIncoming IsDemoman
	Response PlayerIncomingDemoman
}

Response PlayerSentryAheadDemoman
{
	scene "scenes/Player/Demoman/low/994.vcd" 
	scene "scenes/Player/Demoman/low/995.vcd" 
	scene "scenes/Player/Demoman/low/996.vcd" 
}
Rule PlayerSentryAheadDemoman
{
	criteria ConceptPlayerSentryAhead IsDemoman
	Response PlayerSentryAheadDemoman
}

Response PlayerSentryHereDemoman
{
	scene "scenes/Player/Demoman/low/966.vcd" 
}
Rule PlayerSentryHereDemoman
{
	criteria ConceptPlayerSentryHere IsDemoman
	Response PlayerSentryHereDemoman
}

Response PlayerTeleporterHereDemoman
{
	scene "scenes/Player/Demoman/low/968.vcd" 
}
Rule PlayerTeleporterHereDemoman
{
	criteria ConceptPlayerTeleporterHere IsDemoman
	Response PlayerTeleporterHereDemoman
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 3
//--------------------------------------------------------------------------------------------------------------
Response PlayerBattleCryDemoman
{
	scene "scenes/Player/Demoman/low/1358.vcd" 
	scene "scenes/Player/Demoman/low/1369.vcd" 
	scene "scenes/Player/Demoman/low/876.vcd" 
	scene "scenes/Player/Demoman/low/877.vcd" 
	scene "scenes/Player/Demoman/low/878.vcd" 
	scene "scenes/Player/Demoman/low/879.vcd" 
	scene "scenes/Player/Demoman/low/880.vcd" 
}
Rule PlayerBattleCryDemoman
{
	criteria ConceptPlayerBattleCry IsDemoman
	Response PlayerBattleCryDemoman
}

// Custom stuff - melee dare
// Look at enemy, then do battle cry voice command while holding a melee weapon.
Response MeleeDareCombatDemoman
{
	scene "scenes/Player/Demoman/low/1028.vcd"
	scene "scenes/Player/Demoman/low/1017.vcd"
	scene "scenes/Player/Demoman/low/1015.vcd"
}
Rule MeleeDareCombatDemoman
{
	criteria ConceptPlayerBattleCry IsWeaponMelee IsDemoman IsCrosshairEnemy
	Response MeleeDareCombatDemoman
}
//End custom


Response PlayerCheersDemoman
{
	scene "scenes/Player/Demoman/low/881.vcd" 
	scene "scenes/Player/Demoman/low/883.vcd" 
	scene "scenes/Player/Demoman/low/884.vcd" 
	scene "scenes/Player/Demoman/low/1359.vcd" 
	scene "scenes/Player/Demoman/low/886.vcd" 
	scene "scenes/Player/Demoman/low/887.vcd" 
	scene "scenes/Player/Demoman/low/882.vcd" 
	scene "scenes/Player/Demoman/low/885.vcd" 
}
Rule PlayerCheersDemoman
{
	criteria ConceptPlayerCheers IsDemoman
	Response PlayerCheersDemoman
}

Response PlayerGoodJobDemoman
{
	scene "scenes/Player/Demoman/low/914.vcd" 
	scene "scenes/Player/Demoman/low/915.vcd" 
}
Rule PlayerGoodJobDemoman
{
	criteria ConceptPlayerGoodJob IsDemoman
	Response PlayerGoodJobDemoman
}

Response PlayerJeersDemoman
{
	scene "scenes/Player/Demoman/low/938.vcd" 
	scene "scenes/Player/Demoman/low/939.vcd" 
	scene "scenes/Player/Demoman/low/940.vcd" 
	scene "scenes/Player/Demoman/low/941.vcd" 
	scene "scenes/Player/Demoman/low/942.vcd" 
	scene "scenes/Player/Demoman/low/944.vcd" 
	scene "scenes/Player/Demoman/low/945.vcd" 
	scene "scenes/Player/Demoman/low/947.vcd" 
	scene "scenes/Player/Demoman/low/948.vcd" 
	scene "scenes/Player/Demoman/low/949.vcd" 
	scene "scenes/Player/Demoman/low/946.vcd" 
}
Rule PlayerJeersDemoman
{
	criteria ConceptPlayerJeers IsDemoman
	Response PlayerJeersDemoman
}

Response PlayerLostPointDemoman
{
	scene "scenes/Player/Demoman/low/973.vcd" 
	scene "scenes/Player/Demoman/low/1360.vcd" 
	scene "scenes/Player/Demoman/low/969.vcd" 
	scene "scenes/Player/Demoman/low/970.vcd" 
	scene "scenes/Player/Demoman/low/971.vcd" 
	scene "scenes/Player/Demoman/low/972.vcd" 
}
Rule PlayerLostPointDemoman
{
	criteria ConceptPlayerLostPoint IsDemoman
	Response PlayerLostPointDemoman
}

Response PlayerNegativeDemoman
{
	scene "scenes/Player/Demoman/low/973.vcd" 
	scene "scenes/Player/Demoman/low/1360.vcd" 
	scene "scenes/Player/Demoman/low/969.vcd" 
	scene "scenes/Player/Demoman/low/970.vcd" 
	scene "scenes/Player/Demoman/low/971.vcd" 
	scene "scenes/Player/Demoman/low/972.vcd" 
}
Rule PlayerNegativeDemoman
{
	criteria ConceptPlayerNegative IsDemoman
	Response PlayerNegativeDemoman
}

Response PlayerNiceShotDemoman
{
	scene "scenes/Player/Demoman/low/974.vcd" 
	scene "scenes/Player/Demoman/low/975.vcd" 
	scene "scenes/Player/Demoman/low/976.vcd" 
}
Rule PlayerNiceShotDemoman
{
	criteria ConceptPlayerNiceShot IsDemoman
	Response PlayerNiceShotDemoman
}

Response PlayerPositiveDemoman
{
	scene "scenes/Player/Demoman/low/989.vcd" 
	scene "scenes/Player/Demoman/low/990.vcd" 
	scene "scenes/Player/Demoman/low/991.vcd" 
	scene "scenes/Player/Demoman/low/992.vcd" 
	scene "scenes/Player/Demoman/low/993.vcd" 
}

Response PlayerTauntsDemoman
{
	scene "scenes/Player/Demoman/low/1380.vcd" 
	scene "scenes/Player/Demoman/low/1382.vcd" 
	scene "scenes/Player/Demoman/low/1386.vcd" 
}
Rule PlayerPositiveDemoman
{
	criteria ConceptPlayerPositive IsDemoman
	Response PlayerPositiveDemoman
	Response PlayerTauntsDemoman
}

//--------------------------------------------------------------------------------------------------------------
// Auto Speech Cart
//--------------------------------------------------------------------------------------------------------------
Criterion "DemomanNotSaidCartMovingBackwardD" "SaidCartMovingBackwardD" "!=1" "required" weight 0
Criterion "DemomanNotSaidCartMovingBackwardO" "SaidCartMovingBackwardO" "!=1" "required" weight 0
Criterion "DemomanNotSaidCartMovingForwardD" "SaidCartMovingForwardD" "!=1" "required" weight 0
Criterion "DemomanNotSaidCartMovingForwardO" "SaidCartMovingForwardO" "!=1" "required" weight 0
Criterion "DemomanNotSaidCartMovingStoppedD" "SaidCartMovingStoppedD" "!=1" "required" weight 0
Criterion "DemomanNotSaidCartMovingStoppedO" "SaidCartMovingStoppedO" "!=1" "required" weight 0
Response CartMovingBackwardsDefenseDemoman                                                     
{
	scene "scenes/Player/Demoman/low/7718.vcd"
	scene "scenes/Player/Demoman/low/7719.vcd"
	scene "scenes/Player/Demoman/low/7720.vcd"
}
Rule CartMovingBackwardsDefenseDemoman                                                     
{
	criteria ConceptCartMovingBackward IsOnDefense IsDemoman DemomanNotSaidCartMovingBackwardD IsNotDisguised 75PercentChance                                                                                                                                                          
	ApplyContext "SaidCartMovingBackwardD:1:20"
	Response CartMovingBackwardsDefenseDemoman                                                     
}
Response CartMovingBackwardsOffenseDemoman                                                     
{
	scene "scenes/Player/Demoman/low/7714.vcd"
	scene "scenes/Player/Demoman/low/7715.vcd"
	scene "scenes/Player/Demoman/low/7713.vcd"
	scene "scenes/Player/Demoman/low/8533.vcd"
}
Rule CartMovingBackwardsOffenseDemoman                                                     
{
	criteria ConceptCartMovingBackward IsOnOffense IsDemoman DemomanNotSaidCartMovingBackwardO IsNotDisguised 75PercentChance                                                                                                                                                          
	ApplyContext "SaidCartMovingBackwardO:1:20"
	Response CartMovingBackwardsOffenseDemoman                                                     
}
Response CartMovingForwardDefenseDemoman                                                       
{
	scene "scenes/Player/Demoman/low/7716.vcd"
	scene "scenes/Player/Demoman/low/7717.vcd"
}
Rule CartMovingForwardDefenseDemoman                                                       
{
	criteria ConceptCartMovingForward IsOnDefense IsDemoman DemomanNotSaidCartMovingForwardD IsNotDisguised 75PercentChance                                                                                                                                                            
	ApplyContext "SaidCartMovingForwardD:1:20"
	Response CartMovingForwardDefenseDemoman                                                       
}
Response CartMovingForwardOffenseDemoman                                                       
{
	scene "scenes/Player/Demoman/low/7704.vcd"
	scene "scenes/Player/Demoman/low/7705.vcd"
	scene "scenes/Player/Demoman/low/7706.vcd"
	scene "scenes/Player/Demoman/low/7707.vcd"
	scene "scenes/Player/Demoman/low/7711.vcd"
	scene "scenes/Player/Demoman/low/7721.vcd"
	scene "scenes/Player/Demoman/low/7723.vcd"
	scene "scenes/Player/Demoman/low/7724.vcd"
}
Rule CartMovingForwardOffenseDemoman                                                       
{
	criteria ConceptCartMovingForward IsOnOffense IsDemoman DemomanNotSaidCartMovingForwardO IsNotDisguised 75PercentChance                                                                                                                                                            
	ApplyContext "SaidCartMovingForwardO:1:20"
	Response CartMovingForwardOffenseDemoman                                                       
}
Response CartMovingStoppedDefenseDemoman                                                       
{
}
Rule CartMovingStoppedDefenseDemoman                                                       
{
	criteria ConceptCartMovingStopped IsOnDefense IsDemoman DemomanNotSaidCartMovingStoppedD IsNotDisguised 75PercentChance                                                                                                                                                            
	ApplyContext "SaidCartMovingStoppedD:1:20"
	Response CartMovingStoppedDefenseDemoman                                                       
}
Response CartMovingStoppedOffenseDemoman                                                       
{
	scene "scenes/Player/Demoman/low/7726.vcd"
	scene "scenes/Player/Demoman/low/7727.vcd"
	scene "scenes/Player/Demoman/low/7725.vcd"
}
Rule CartMovingStoppedOffenseDemoman                                                       
{
	criteria ConceptCartMovingStopped IsOnOffense IsDemoman DemomanNotSaidCartMovingStoppedO IsNotDisguised 75PercentChance                                                                                                                                                            
	ApplyContext "SaidCartMovingStoppedO:1:20"
	Response CartMovingStoppedOffenseDemoman                                                       
}
//--------------------------------------------------------------------------------------------------------------
// END OF Auto Speech Cart
//--------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------
// Begin Competitive Mode VO
//--------------------------------------------------------------------------------------------------------------
Response PlayerFirstRoundStartCompDemoman
{
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_comp_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_comp_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_comp_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_comp_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_comp_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_rare_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_rare_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_rare_03.vcd" predelay "1.0, 5.0"
}
Rule PlayerFirstRoundStartCompDemoman
{
	criteria ConceptPlayerRoundStartComp IsDemoman IsFirstRound IsNotComp6v6 40PercentChance
	Response PlayerFirstRoundStartCompDemoman
}

Response PlayerFirstRoundStartComp6sDemoman
{
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_comp_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_comp_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_comp_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_comp_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_comp_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_rare_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_rare_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_rare_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_6s_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_6s_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamefirst_6s_03.vcd" predelay "1.0, 5.0"
}
Rule PlayerFirstRoundStartComp6sDemoman
{
	criteria ConceptPlayerRoundStartComp IsDemoman IsFirstRound IsComp6v6 40PercentChance
	Response PlayerFirstRoundStartComp6sDemoman
}

Response PlayerWonPrevRoundCompDemoman
{
	scene "scenes/Player/Demoman/low/cm_demo_pregamewonlast_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamewonlast_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamewonlast_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamewonlast_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamewonlast_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamewonlast_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamewonlast_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamewonlast_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamewonlast_rare_01.vcd" predelay "1.0, 5.0"
}
Rule PlayerWonPrevRoundCompDemoman
{
	criteria ConceptPlayerRoundStartComp IsDemoman IsNotFirstRound PlayerWonPreviousRound 40PercentChance
	Response PlayerWonPrevRoundCompDemoman
}

Response PlayerLostPrevRoundCompDemoman
{
	scene "scenes/Player/Demoman/low/cm_demo_pregamelostlast_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamelostlast_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamelostlast_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamelostlast_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregamelostlast_rare_01.vcd" predelay "1.0, 5.0"
}
Rule PlayerLostPrevRoundCompDemoman
{
	criteria ConceptPlayerRoundStartComp IsDemoman IsNotFirstRound PlayerLostPreviousRound PreviousRoundWasNotTie 40PercentChance
	Response PlayerLostPrevRoundCompDemoman
}

Response PlayerTiedPrevRoundCompDemoman
{
	scene "scenes/Player/Demoman/low/cm_demo_pregametie_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregametie_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregametie_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_pregametie_04.vcd" predelay "1.0, 5.0"
}
Rule PlayerTiedPrevRoundCompDemoman
{
	criteria ConceptPlayerRoundStartComp IsDemoman IsNotFirstRound PreviousRoundWasTie 40PercentChance
	Response PlayerTiedPrevRoundCompDemoman
}

Response PlayerGameWinCompDemoman
{
	scene "scenes/Player/Demoman/low/cm_demo_gamewon_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_gamewon_02.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_gamewon_03.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_gamewon_04.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_gamewon_05.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_gamewon_06.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_gamewon_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_gamewon_02.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_gamewon_03.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_gamewon_04.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_gamewon_05.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_gamewon_06.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_gamewon_rare_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_gamewon_rare_02.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_gamewon_rare_04.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Demoman/low/cm_demo_gamewon_rare_03.vcd" predelay "2.0, 5.0"
}
Rule PlayerGameWinCompDemoman
{
	criteria ConceptPlayerGameOverComp PlayerOnWinningTeam IsDemoman 40PercentChance
	Response PlayerGameWinCompDemoman
}

Response PlayerMatchWinCompDemoman
{
	scene "scenes/Player/Demoman/low/cm_demo_matchwon_01.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Demoman/low/cm_demo_matchwon_02.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Demoman/low/cm_demo_matchwon_03.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Demoman/low/cm_demo_matchwon_04.vcd" predelay "1.0, 2.0"
}
Rule PlayerMatchWinCompDemoman
{
	criteria ConceptPlayerMatchOverComp PlayerOnWinningTeam IsDemoman 40PercentChance
	Response PlayerMatchWinCompDemoman
}
//--------------------------------------------------------------------------------------------------------------
// End Competitive Mode VO
//--------------------------------------------------------------------------------------------------------------