//--------------------------------------------------------------------------------------------------------------
// Heavy Response Rule File
//--------------------------------------------------------------------------------------------------------------

Criterion "HeavyIsKillSpeechObject" "HeavyKillSpeechObject" "1" "required" weight 0
Criterion "HeavyIsNotStillonFire" "HeavyOnFire" "!=1" "required" weight 0
Criterion "HeavyIsStillonFire" "HeavyOnFire" "1" "required" weight 0
Criterion "HeavyNotInvulnerableSpeech" "HeavyInvulnerableSpeech" "!=1" "required" weight 0
Criterion "HeavyNotKillSpeech" "HeavyKillSpeech" "!=1" "required" weight 0
Criterion "HeavyNotKillSpeechMelee" "HeavyKillSpeechMelee" "!=1" "required" weight 0
Criterion "HeavyNotSaidCartMovingBackwardD" "SaidCartMovingBackwardD" "!=1" "required" weight 0
Criterion "HeavyNotSaidCartMovingBackwardO" "SaidCartMovingBackwardO" "!=1" "required" weight 0
Criterion "HeavyNotSaidCartMovingForwardD" "SaidCartMovingForwardD" "!=1" "required" weight 0
Criterion "HeavyNotSaidCartMovingForwardO" "SaidCartMovingForwardO" "!=1" "required" weight 0
Criterion "HeavyNotSaidCartMovingStoppedD" "SaidCartMovingStoppedD" "!=1" "required" weight 0
Criterion "HeavyNotSaidCartMovingStoppedO" "SaidCartMovingStoppedO" "!=1" "required" weight 0
Criterion "HeavyNotSaidHealThanks" "HeavySaidHealThanks" "!=1" "required"
Criterion "IsHelpCapHeavy" "HeavyHelpCap" "1" "required" weight 0
// Custom stuff
Criterion "HeavyNotMedicSpeech" "HeavyMedicSpeech" "!=1" "required"
Criterion "HeavyNotAwardSpeech" "HeavyAwardSpeech" "!=1" "required"
Criterion "HeavyNotAssistSpeech" "HeavyAssistSpeech" "!=1" "required" weight 0
Criterion "IsHeavyFistsSwung" "HeavyFistsSwung" "1" "required" weight 0
Criterion "IsNotHeavyFistsSwung" "HeavyFistsSwung" "!=1" "required" weight 0
Criterion "IsHeavyFistsSwinging" "HeavyFistsSwinging" "1" "required" weight 0
Criterion "IsNotHeavyFistsSwinging" "HeavyFistsSwinging" "!=1" "required" weight 0
Criterion "NotGunTauntHeavy" "GunTauntHeavy" "!=1" "required" weight 0
Criterion "IsNotDaring" "IsDaring" "!=1" "required" weight 0
Criterion "HeavyNotKillSpeechObject" "HeavyKillSpeechObject" "!=1" "required" weight 0
Criterion "HeavyNotShinySpeech" "HeavyShinySpeech" "!=1" "required" weight 0
Criterion "HeavyNotFairyNoises" "HeavyFairyNoises" "!=1" "required" weight 0


Response PlayerCloakedSpyDemomanHeavy
{
	scene "scenes/Player/Heavy/low/225.vcd" 
}
Rule PlayerCloakedSpyDemomanHeavy
{
	criteria ConceptPlayerCloakedSpy IsHeavy IsOnDemoman
	Response PlayerCloakedSpyDemomanHeavy
}

Response PlayerCloakedSpyEngineerHeavy
{
	scene "scenes/Player/Heavy/low/228.vcd" 
}
Rule PlayerCloakedSpyEngineerHeavy
{
	criteria ConceptPlayerCloakedSpy IsHeavy IsOnEngineer
	Response PlayerCloakedSpyEngineerHeavy
}

Response PlayerCloakedSpyHeavyHeavy
{
	scene "scenes/Player/Heavy/low/223.vcd" 
}
Rule PlayerCloakedSpyHeavyHeavy
{
	criteria ConceptPlayerCloakedSpy IsHeavy IsOnHeavy
	Response PlayerCloakedSpyHeavyHeavy
}

Response PlayerCloakedSpyMedicHeavy
{
	scene "scenes/Player/Heavy/low/227.vcd" 
}
Rule PlayerCloakedSpyMedicHeavy
{
	criteria ConceptPlayerCloakedSpy IsHeavy IsOnMedic
	Response PlayerCloakedSpyMedicHeavy
}

Response PlayerCloakedSpyPyroHeavy
{
	scene "scenes/Player/Heavy/low/224.vcd" 
}
Rule PlayerCloakedSpyPyroHeavy
{
	criteria ConceptPlayerCloakedSpy IsHeavy IsOnPyro
	Response PlayerCloakedSpyPyroHeavy
}

Response PlayerCloakedSpyScoutHeavy
{
	scene "scenes/Player/Heavy/low/221.vcd" 
}
Rule PlayerCloakedSpyScoutHeavy
{
	criteria ConceptPlayerCloakedSpy IsHeavy IsOnScout
	Response PlayerCloakedSpyScoutHeavy
}

Response PlayerCloakedSpySniperHeavy
{
	scene "scenes/Player/Heavy/low/229.vcd" 
}
Rule PlayerCloakedSpySniperHeavy
{
	criteria ConceptPlayerCloakedSpy IsHeavy IsOnSniper
	Response PlayerCloakedSpySniperHeavy
}

Response PlayerCloakedSpySoldierHeavy
{
	scene "scenes/Player/Heavy/low/222.vcd" 
}
Rule PlayerCloakedSpySoldierHeavy
{
	criteria ConceptPlayerCloakedSpy IsHeavy IsOnSoldier
	Response PlayerCloakedSpySoldierHeavy
}

Response PlayerCloakedSpySpyHeavy
{
	scene "scenes/Player/Heavy/low/226.vcd" 
}
Rule PlayerCloakedSpySpyHeavy
{
	criteria ConceptPlayerCloakedSpy IsHeavy IsOnSpy
	Response PlayerCloakedSpySpyHeavy
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech
//--------------------------------------------------------------------------------------------------------------
// Custom achievement stuff
Response AwardHeavy
{
	scene "scenes/Player/Heavy/low/1954.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/1955.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/1956.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2057.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2058.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2059.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2060.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2061.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2062.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2063.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2064.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2065.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2068.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2069.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2200.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2259.vcd" predelay "2.5"
	
}
Rule AwardHeavy
{
	criteria ConceptAchievementAward IsHeavy HeavyNotAwardSpeech
	ApplyContext "HeavyAwardSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response AwardHeavy
}
//End custom achievement

Response HealThanksHeavy
{
	scene "scenes/Player/Heavy/low/344.vcd" 
	scene "scenes/Player/Heavy/low/345.vcd" 
	scene "scenes/Player/Heavy/low/346.vcd" 
}
Rule HealThanksHeavy
{
	criteria ConceptMedicChargeStopped IsHeavy SuperHighHealthContext HeavyNotSaidHealThanks 50PercentChance
	ApplyContext "HeavySaidHealThanks:1:20"
	Response HealThanksHeavy
}

Response PlayerRoundStartHeavy
{
	scene "scenes/Player/Heavy/low/205.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/206.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/207.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/208.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/209.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/210.vcd" predelay "1.0, 5.0"
}
Rule PlayerRoundStartHeavy
{
	criteria ConceptPlayerRoundStart IsHeavy
	Response PlayerRoundStartHeavy
}

Response PlayerCappedIntelligenceHeavy
{
	scene "scenes/Player/Heavy/low/196.vcd" 
	scene "scenes/Player/Heavy/low/197.vcd" 
	scene "scenes/Player/Heavy/low/198.vcd" 
}
Rule PlayerCappedIntelligenceHeavy
{
	criteria ConceptPlayerCapturedIntelligence IsHeavy
	Response PlayerCappedIntelligenceHeavy
}

Response PlayerCapturedPointHeavy
{
	scene "scenes/Player/Heavy/low/193.vcd" 
	scene "scenes/Player/Heavy/low/194.vcd" 
	scene "scenes/Player/Heavy/low/195.vcd" 
}
Rule PlayerCapturedPointHeavy
{
	criteria ConceptPlayerCapturedPoint IsHeavy
	Response PlayerCapturedPointHeavy
}

Response PlayerSuddenDeathHeavy
{
	scene "scenes/Player/Heavy/low/258.vcd" 
	scene "scenes/Player/Heavy/low/259.vcd" 
	scene "scenes/Player/Heavy/low/260.vcd" 
	scene "scenes/Player/Heavy/low/261.vcd" 
	scene "scenes/Player/Heavy/low/262.vcd" 
	scene "scenes/Player/Heavy/low/263.vcd" 
	scene "scenes/Player/Heavy/low/264.vcd" 
	scene "scenes/Player/Heavy/low/265.vcd" 
	scene "scenes/Player/Heavy/low/266.vcd" 
}
Rule PlayerSuddenDeathHeavy
{
	criteria ConceptPlayerSuddenDeathStart IsHeavy
	Response PlayerSuddenDeathHeavy
}

Response PlayerStalemateHeavy
{
	scene "scenes/Player/Heavy/low/199.vcd" 
	scene "scenes/Player/Heavy/low/200.vcd" 
	scene "scenes/Player/Heavy/low/201.vcd" 
}
Rule PlayerStalemateHeavy
{
	criteria ConceptPlayerStalemate IsHeavy
	Response PlayerStalemateHeavy
}

Response PlayerTeleporterThanksHeavy
{
	scene "scenes/Player/Heavy/low/347.vcd" 
	scene "scenes/Player/Heavy/low/348.vcd" 
	scene "scenes/Player/Heavy/low/349.vcd" 
}
Rule PlayerTeleporterThanksHeavy
{
	criteria ConceptTeleported IsNotEngineer IsHeavy 30PercentChance
	Response PlayerTeleporterThanksHeavy
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Cart
//--------------------------------------------------------------------------------------------------------------
Response CartMovingBackwardsDefenseHeavy
{
	scene "scenes/Player/Heavy/low/1990.vcd" 
	scene "scenes/Player/Heavy/low/1991.vcd" 
	scene "scenes/Player/Heavy/low/1992.vcd" 
	scene "scenes/Player/Heavy/low/2070.vcd" 
	scene "scenes/Player/Heavy/low/2208.vcd" 
	scene "scenes/Player/Heavy/low/2209.vcd" 
	scene "scenes/Player/Heavy/low/2267.vcd" 
	scene "scenes/Player/Heavy/low/2268.vcd" 
}
Rule CartMovingBackwardsDefenseHeavy
{
	criteria ConceptCartMovingBackward IsOnDefense IsHeavy HeavyNotSaidCartMovingBackwardD IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingBackwardD:1:20"
	Response CartMovingBackwardsDefenseHeavy
}

Response CartMovingBackwardsOffenseHeavy
{
	scene "scenes/Player/Heavy/low/1987.vcd" 
	scene "scenes/Player/Heavy/low/1988.vcd" 
	scene "scenes/Player/Heavy/low/1989.vcd" 
	scene "scenes/Player/Heavy/low/2071.vcd" 
	scene "scenes/Player/Heavy/low/2072.vcd" 
	scene "scenes/Player/Heavy/low/2206.vcd" 
	scene "scenes/Player/Heavy/low/2207.vcd" 
}
Rule CartMovingBackwardsOffenseHeavy
{
	criteria ConceptCartMovingBackward IsOnOffense IsHeavy HeavyNotSaidCartMovingBackwardO IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingBackwardO:1:20"
	Response CartMovingBackwardsOffenseHeavy
}

Response CartMovingForwardDefenseHeavy
{
	scene "scenes/Player/Heavy/low/1984.vcd" 
	scene "scenes/Player/Heavy/low/1985.vcd" 
	scene "scenes/Player/Heavy/low/2269.vcd" 
	scene "scenes/Player/Heavy/low/1986.vcd" 
	scene "scenes/Player/Heavy/low/2073.vcd" 
	scene "scenes/Player/Heavy/low/2270.vcd" 
	scene "scenes/Player/Heavy/low/2263.vcd" 
}
Rule CartMovingForwardDefenseHeavy
{
	criteria ConceptCartMovingForward IsOnDefense IsHeavy HeavyNotSaidCartMovingForwardD IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingForwardD:1:20"
	Response CartMovingForwardDefenseHeavy
}

Response CartMovingForwardOffenseHeavy
{
	scene "scenes/Player/Heavy/low/1963.vcd" 
	scene "scenes/Player/Heavy/low/1964.vcd" 
	scene "scenes/Player/Heavy/low/1965.vcd" 
	scene "scenes/Player/Heavy/low/2179.vcd" 
	scene "scenes/Player/Heavy/low/2180.vcd" 
	scene "scenes/Player/Heavy/low/2181.vcd" 
	scene "scenes/Player/Heavy/low/2182.vcd" 
	scene "scenes/Player/Heavy/low/2183.vcd" 
	scene "scenes/Player/Heavy/low/2184.vcd" 
	scene "scenes/Player/Heavy/low/2271.vcd" 
	scene "scenes/Player/Heavy/low/2185.vcd" 
	scene "scenes/Player/Heavy/low/2186.vcd" 
	scene "scenes/Player/Heavy/low/2187.vcd" 
	scene "scenes/Player/Heavy/low/2188.vcd" 
	scene "scenes/Player/Heavy/low/2189.vcd" 
	scene "scenes/Player/Heavy/low/2190.vcd" 
	scene "scenes/Player/Heavy/low/2203.vcd" 
	scene "scenes/Player/Heavy/low/1957.vcd" 
	scene "scenes/Player/Heavy/low/1958.vcd" 
	scene "scenes/Player/Heavy/low/1959.vcd" 
	scene "scenes/Player/Heavy/low/2202.vcd" 
	scene "scenes/Player/Heavy/low/2201.vcd" 
	scene "scenes/Player/Heavy/low/2193.vcd" 
	scene "scenes/Player/Heavy/low/2066.vcd" 
	scene "scenes/Player/Heavy/low/2261.vcd" 
	scene "scenes/Player/Heavy/low/2260.vcd" 
	scene "scenes/Player/Heavy/low/2262.vcd" 
}
Rule CartMovingForwardOffenseHeavy
{
	criteria ConceptCartMovingForward IsOnOffense IsHeavy HeavyNotSaidCartMovingForwardO IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingForwardO:1:20"
	Response CartMovingForwardOffenseHeavy
}

Response CartMovingStoppedDefenseHeavy
{
	scene "scenes/Player/Heavy/low/1966.vcd" 
	scene "scenes/Player/Heavy/low/1967.vcd" 
	scene "scenes/Player/Heavy/low/1968.vcd" 
	scene "scenes/Player/Heavy/low/2191.vcd" 
}
Rule CartMovingStoppedDefenseHeavy
{
	criteria ConceptCartMovingStopped IsOnDefense IsHeavy HeavyNotSaidCartMovingStoppedD IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingStoppedD:1:20"
	Response CartMovingStoppedDefenseHeavy
}

Response CartMovingStoppedOffenseHeavy
{
	scene "scenes/Player/Heavy/low/1960.vcd" 
	scene "scenes/Player/Heavy/low/1961.vcd" 
	scene "scenes/Player/Heavy/low/2192.vcd" 
	scene "scenes/Player/Heavy/low/1962.vcd" 
}
Rule CartMovingStoppedOffenseHeavy
{
	criteria ConceptCartMovingStopped IsOnOffense IsHeavy HeavyNotSaidCartMovingStoppedO IsNotDisguised 75PercentChance
	ApplyContext "SaidCartMovingStoppedO:1:20"
	Response CartMovingStoppedOffenseHeavy
}


//--------------------------------------------------------------------------------------------------------------
// Auto Speech Combat
//--------------------------------------------------------------------------------------------------------------

// Minigun responses imported from tf.txt

Response HeavyTimeFiringMinigunShort
{
	scene "scenes/player/heavy/low/attackminigun_vocal02.vcd"
	scene "scenes/player/heavy/low/attackminigun_vocal03.vcd"
}
Rule HeavyTimeFiringMinigunShort
{
	criteria ConceptFireMinigunTalk IsHeavy WeaponIsMinigun TimeFiringMinigunShort 30PercentChance
	Response HeavyTimeFiringMinigunShort
}

Response HeavyTimeFiringMinigunLong
{
	//scene "scenes/player/heavy/low/specialcompleted11.vcd" This is a remnant from the beta files.
	//scene "scenes/player/heavy/low/laughShort03.vcd" This is a remnant from the beta files.
	scene "scenes/player/heavy/low/1279.vcd"
	scene "scenes/player/heavy/low/1273.vcd"
}
Rule HeavyTimeFiringMinigunLong
{
	criteria ConceptFireMinigunTalk IsHeavy WeaponIsMinigun TimeFiringMinigunLong 50PercentChance
	Response HeavyTimeFiringMinigunLong
}

Response HeavyTimeFiringMinigunReallyLong
{
	scene "scenes/player/heavy/low/attackMinigun_vocal05.vcd"
	//scene "scenes/player/heavy/low/specialcompleted07.vcd" This is a remnant from the beta files.
	scene "scenes/player/heavy/low/318.vcd"
	// Was that so hard VALVe? ;)
}
Rule HeavyTimeFiringMinigunReallyLong
{
	criteria ConceptFireMinigunTalk IsHeavy WeaponIsMinigun TimeFiringMinigunReallyLong 50PercentChance
	Response HeavyTimeFiringMinigunReallyLong
}

// End minigun responses imported from tf.txt

Response DefendOnThePointHeavy
{
	scene "scenes/Player/Heavy/low/322.vcd" 
	scene "scenes/Player/Heavy/low/1277.vcd" 
}
Rule DefendOnThePointHeavy
{
	criteria ConceptFireWeapon IsHeavy IsOnFriendlyControlPoint NotDefendOnThePointSpeech
	ApplyContext "DefendOnThePointSpeech:1:30"
	applycontexttoworld
	Response DefendOnThePointHeavy
}

Response InvulnerableSpeechHeavy
{
	scene "scenes/Player/Heavy/low/314.vcd" 
	scene "scenes/Player/Heavy/low/315.vcd" 
	scene "scenes/Player/Heavy/low/316.vcd" 
	scene "scenes/Player/Heavy/low/325.vcd" 
	scene "scenes/Player/Heavy/low/341.vcd" 
}
Rule InvulnerableSpeechHeavy
{
	criteria ConceptFireMinigun IsHeavy WeaponIsMinigun IsInvulnerable HeavyNotInvulnerableSpeech
	ApplyContext "HeavyInvulnerableSpeech:1:30"
	Response InvulnerableSpeechHeavy
}

// Custom stuff
Response KilledMedicHeavy
{
	scene "scenes/Player/Heavy/low/1982.vcd"
	scene "scenes/Player/Heavy/low/1983.vcd"
	scene "scenes/Player/Heavy/low/1993.vcd"
	scene "scenes/Player/Heavy/low/1994.vcd"
}
Rule KilledMedicHeavy
{
	criteria ConceptKilledPlayer IsVictimMedic IsHeavy IsBeingHealed 50PercentChance KilledPlayerDelay HeavyNotMedicSpeech
	ApplyContext "HeavyMedicSpeech:1:20"
	ApplyContext "IsDominating:1:5" // Added the IsDominating context so this isn't interrupted. Heavy is going to be under fire a lot in this kind of situation.
	Response KilledMedicHeavy
}

Response KilledPlayerAssistAutoHeavy
{
	scene "scenes/Player/Heavy/low/320.vcd" predelay "2.5"
}
Rule KilledPlayerAssistAutoHeavy
{
	criteria ConceptKilledPlayer IsHeavy IsBeingHealed IsManyRecentKills KilledPlayerDelay 20PercentChance HeavyNotAssistSpeech
	ApplyContext "HeavyAssistSpeech:1:20"
	Response KilledPlayerAssistAutoHeavy
}

// End custom

// Modified to include a new line
Response KilledPlayerManyHeavy
{
	scene "scenes/Player/Heavy/low/310.vcd" 
	scene "scenes/Player/Heavy/low/311.vcd" 
	scene "scenes/Player/Heavy/low/1279.vcd" 
	scene "scenes/Player/Heavy/low/326.vcd" 
	scene "scenes/Player/Heavy/low/327.vcd" 
	scene "scenes/player/heavy/low/335.vcd"
}
Rule KilledPlayerManyHeavy
{
	criteria ConceptKilledPlayer IsManyRecentKills 30PercentChance IsWeaponPrimary KilledPlayerDelay HeavyNotKillSpeech IsHeavy
	ApplyContext "HeavyKillSpeech:1:10"
	Response KilledPlayerManyHeavy
}

Response KilledPlayerMeleeHeavy
{
	scene "scenes/Player/Heavy/low/272.vcd" 
	scene "scenes/Player/Heavy/low/273.vcd" 
}
Rule KilledPlayerMeleeHeavy
{
	criteria ConceptKilledPlayer KilledPlayerDelay 30PercentChance  IsWeaponMelee HeavyNotKillSpeechMelee IsHeavy
	ApplyContext "HeavyKillSpeechMelee:1:10"
	Response KilledPlayerMeleeHeavy
}

// Custom stuff
// Fist of Steel kill
Response KilledPlayerMeleeMetalHeavy
{
	scene "scenes/Player/Heavy/low/1941.vcd" 
}
Rule KilledPlayerMeleeMetalHeavy
{
	criteria ConceptKilledPlayer KilledPlayerDelay 30PercentChance  WeaponIsMetalFists HeavyNotKillSpeechMelee IsHeavy
	ApplyContext "HeavyKillSpeechMelee:1:10"
	Response KilledPlayerMeleeMetalHeavy
	Response KilledPlayerMeleeHeavy
}

// Iron Curtain/Brass Beast kill
Response KilledPlayerShinyHeavy
{
	scene "scenes/player/heavy/low/2210.vcd" predelay ".25"
	scene "scenes/player/heavy/low/2198.vcd" predelay ".25"
	scene "scenes/player/heavy/low/2204.vcd" predelay ".25"
	scene "scenes/player/heavy/low/2199.vcd" predelay ".25"
}
Rule KilledPlayerShinyHeavy
{
	criterion ConceptKilledPlayer KilledPlayerDelay 20PercentChance IsManyRecentKills IsWeaponPrimary WeaponIsNotVanillaPrimary HeavyNotKillSpeech IsHeavy WeaponIsNotTaggedMinigun
	ApplyContext "HeavyKillSpeech:1:90"
	Response KilledPlayerShinyHeavy
}

// Custom kill response against an Engineer.
Response KilledPlayerTauntHeavy
{
	scene "scenes/player/heavy/low/332.vcd"
	scene "scenes/player/heavy/low/337.vcd"
}
Rule KilledPlayerTauntHeavy
{
	criteria ConceptKilledPlayer KilledPlayerDelay 30PercentChance IsHeavy IsVictimEngineer IsCrosshairEnemy HeavyNotKillSpeech
	ApplyContext "HeavyKillSpeech:1:10"
	Response KilledPlayerTauntHeavy
}

// Swinging fists start
Response HeavySwingFistsStart
{
	scene "scenes/player/heavy/low/1980.vcd"
}
Rule HeavySwingFistsStart
{
	criteria ConceptFireWeapon IsWeaponMelee 50PercentChance IsHeavy IsNotDaring HeavyNotKillSpeechMelee IsNotDominating IsNotHeavyFistsSwung WeaponIsNotSaxxy
	ApplyContext "HeavyFistsSwung:1:10"
	Response HeavySwingFistsStart
}

Response HeavySwingFists
{
	scene "scenes/player/heavy/low/2272.vcd"
	scene "scenes/player/heavy/low/2273.vcd"
	scene "scenes/player/heavy/low/2274.vcd"
	scene "scenes/player/heavy/low/1978.vcd"
	scene "scenes/player/heavy/low/1979.vcd"
}
Rule HeavySwingFists
{
	criteria ConceptFireWeapon IsWeaponMelee 50PercentChance IsHeavyFistsSwung IsNotDaring HeavyNotKillSpeechMelee IsNotDominating IsHeavy IsNotHeavyFistsSwinging
	ApplyContext "HeavyFistsSwinging:2:3"
	Response HeavySwingFists
}

// End custom

// Modified to include a new line
Response KilledPlayerVeryManyHeavy
{
	scene "scenes/Player/Heavy/low/318.vcd" 
	scene "scenes/Player/Heavy/low/319.vcd" 
	scene "scenes/Player/Heavy/low/323.vcd" 
	scene "scenes/Player/Heavy/low/338.vcd" 
	scene "scenes/Player/Heavy/low/340.vcd" 
	scene "scenes/player/heavy/low/333.vcd"
}
Rule KilledPlayerVeryManyHeavy
{
	criteria ConceptKilledPlayer IsVeryManyRecentKills 50PercentChance IsWeaponPrimary KilledPlayerDelay HeavyNotKillSpeech IsHeavy
	ApplyContext "HeavyKillSpeech:1:10"
	Response KilledPlayerVeryManyHeavy
}

Response MedicFollowHeavy
{
	scene "scenes/Player/Heavy/low/1933.vcd" predelay ".25"
	scene "scenes/Player/Heavy/low/1935.vcd" predelay ".25"
	scene "scenes/Player/Heavy/low/1936.vcd" predelay ".25"
	scene "scenes/Player/Heavy/low/1937.vcd" predelay ".25"
	scene "scenes/Player/Heavy/low/1938.vcd" predelay ".25"
	scene "scenes/Player/Heavy/low/1939.vcd" predelay ".25"
	scene "scenes/Player/Heavy/low/2086.vcd" predelay ".25"
}
Rule MedicFollowHeavy
{
	criteria ConceptPlayerMedic IsOnMedic IsHeavy IsNotCrossHairEnemy NotLowHealth HeavyIsNotStillonFire
	ApplyContext "HeavyKillSpeech:1:10"
	Response MedicFollowHeavy
}

Response HeavyJarateHit
{
	scene "scenes/Player/Heavy/low/1267.vcd"   
	scene "scenes/Player/Heavy/low/201.vcd"   
	scene "scenes/Player/Heavy/low/259.vcd"   
}
Rule HeavyJarateHit
{
	criteria ConceptJarateHit IsHeavy 50PercentChance
	Response HeavyJarateHit
}

Response PlayerKilledCapperHeavy
{
	scene "scenes/Player/Heavy/low/1265.vcd" 
	scene "scenes/Player/Heavy/low/289.vcd" 
	scene "scenes/Player/Heavy/low/306.vcd" 
	scene "scenes/Player/Heavy/low/330.vcd" 
	scene "scenes/Player/Heavy/low/334.vcd" 
	scene "scenes/Player/Heavy/low/339.vcd" 
	scene "scenes/Player/Heavy/low/331.vcd" 
}
Rule PlayerKilledCapperHeavy
{
	criteria ConceptCapBlocked IsHeavy
	ApplyContext "HeavyKillSpeech:1:10"
	Response PlayerKilledCapperHeavy
}

Response PlayerKilledDominatingHeavy
{
	scene "scenes/Player/Heavy/low/1948.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/1950.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2074.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2075.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2076.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2077.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2078.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2079.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2080.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2083.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2084.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2085.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2103.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2115.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2194.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2256.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/235.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/263.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/267.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/268.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/269.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/1268.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/1269.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/1272.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/270.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/271.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2067.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2265.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2266.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/303.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/304.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/336.vcd" predelay "2.5"
}
Rule PlayerKilledDominatingHeavy
{
	criteria ConceptKilledPlayer IsHeavy IsDominated
	ApplyContext "HeavyKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledDominatingHeavy
}

Response PlayerKilledForRevengeHeavy
{
	scene "scenes/Player/Heavy/low/213.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/216.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/305.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/1951.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/1952.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/1953.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2100.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2101.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2102.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2104.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2105.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2108.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2109.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2110.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2111.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2112.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2113.vcd" predelay "2.5"
	scene "scenes/Player/Heavy/low/2114.vcd" predelay "2.5"
}
Rule PlayerKilledForRevengeHeavy
{
	criteria ConceptKilledPlayer IsHeavy IsRevenge
	ApplyContext "HeavyKillSpeech:1:10"
	ApplyContext "IsDominating:1:10"
	Response PlayerKilledForRevengeHeavy
}

Response PlayerKilledObjectHeavy
{
	scene "scenes/Player/Heavy/low/2264.vcd" 
	scene "scenes/Player/Heavy/low/1262.vcd" 
	scene "scenes/Player/Heavy/low/312.vcd" 
	scene "scenes/Player/Heavy/low/313.vcd" 
}
Rule PlayerKilledObjectHeavy
{
	criteria ConceptKilledObject IsHeavy 30PercentChance IsARecentKill HeavyNotKillSpeechObject
	ApplyContext "HeavyKillSpeechObject:1:30"
	Response PlayerKilledObjectHeavy
}

// Custom stuff
Response PlayerKilledSentryHeavy
{
	scene "scenes/player/heavy/low/332.vcd"
}
Rule PlayerKilledSentryHeavy
{
	criteria ConceptKilledObject IsHeavy IsSentryGun 10PercentChance HeavyNotKillSpeechObject
	ApplyContext "HeavyKillSpeechObject:1:30"
	Response PlayerKilledSentryHeavy
}

Response PlayerKilledDispenserHeavy
{
	scene "scenes/player/heavy/low/337.vcd"
}
Rule PlayerKilledDispenserHeavy
{
	criteria ConceptKilledObject IsHeavy IsDispenser 10PercentChance HeavyNotKillSpeechObject
	ApplyContext "HeavyKillSpeechObject:1:30"
	Response PlayerKilledDispenserHeavy
}
// End custom

//--------------------------------------------------------------------------------------------------------------
// Auto Speech Pain
//--------------------------------------------------------------------------------------------------------------
Response PlayerAttackerPainHeavy
{
	scene "scenes/Player/Heavy/low/297.vcd" 
	scene "scenes/Player/Heavy/low/298.vcd" 
	scene "scenes/Player/Heavy/low/299.vcd" 
}
Rule PlayerAttackerPainHeavy
{
	criteria ConceptAttackerPain IsHeavy IsNotDominating
	Response PlayerAttackerPainHeavy
}

Response PlayerOnFireHeavy
{
	scene "scenes/Player/Heavy/low/202.vcd" 
	scene "scenes/Player/Heavy/low/203.vcd" 
	scene "scenes/Player/Heavy/low/1400.vcd" 
}
Rule PlayerOnFireHeavy
{
	criteria ConceptFire IsHeavy HeavyIsNotStillonFire IsNotDominating
	ApplyContext "HeavyOnFire:1:7"
	Response PlayerOnFireHeavy
}

Response PlayerOnFireRareHeavy
{
	scene "scenes/Player/Heavy/low/204.vcd" 
	scene "scenes/Player/Heavy/low/1399.vcd" 
}
Rule PlayerOnFireRareHeavy
{
	criteria ConceptFire IsHeavy 10PercentChance HeavyIsNotStillonFire IsNotDominating
	ApplyContext "HeavyOnFire:1:7"
	Response PlayerOnFireRareHeavy
}

Response PlayerPainHeavy
{
	scene "scenes/Player/Heavy/low/300.vcd" 
	scene "scenes/Player/Heavy/low/301.vcd" 
	scene "scenes/Player/Heavy/low/302.vcd" 
	scene "scenes/Player/Heavy/low/1390.vcd" 
	scene "scenes/Player/Heavy/low/1391.vcd" 
}
Rule PlayerPainHeavy
{
	criteria ConceptPain IsHeavy IsNotDominating
	Response PlayerPainHeavy
}

Response PlayerStillOnFireHeavy
{
	scene "scenes/Player/Heavy/low/1925.vcd" 
}
Rule PlayerStillOnFireHeavy
{
	criteria ConceptFire IsHeavy  HeavyIsStillonFire IsNotDominating
	ApplyContext "HeavyOnFire:1:7"
	Response PlayerStillOnFireHeavy
}


//--------------------------------------------------------------------------------------------------------------
// Duel Speech
//--------------------------------------------------------------------------------------------------------------
Response AcceptedDuelHeavy
{
	scene "scenes/Player/Heavy/low/2058.vcd" 
	scene "scenes/Player/Heavy/low/2061.vcd" 
	scene "scenes/Player/Heavy/low/205.vcd" 
	scene "scenes/Player/Heavy/low/1950.vcd" 
	scene "scenes/Player/Heavy/low/2084.vcd" 
	scene "scenes/Player/Heavy/low/324.vcd" 
	scene "scenes/Player/Heavy/low/326.vcd" 
	scene "scenes/Player/Heavy/low/330.vcd" 
	scene "scenes/Player/Heavy/low/350.vcd" 
	scene "scenes/Player/Heavy/low/351.vcd" 
}
Rule AcceptedDuelHeavy
{
	criteria ConceptIAcceptDuel IsHeavy
	Response AcceptedDuelHeavy
}

Response MeleeDareHeavy
{
	scene "scenes/Player/Heavy/low/1975.vcd" 
	scene "scenes/Player/Heavy/low/1977.vcd" 
	scene "scenes/Player/Heavy/low/2090.vcd" 
	scene "scenes/Player/Heavy/low/2091.vcd" 
	scene "scenes/Player/Heavy/low/2119.vcd" 
	scene "scenes/Player/Heavy/low/2205.vcd" 
	scene "scenes/Player/Heavy/low/2258.vcd" 
}
Rule MeleeDareHeavy
{
	criteria ConceptRequestDuel IsHeavy
	Response MeleeDareHeavy
}

Response RejectedDuelHeavy
{
	scene "scenes/Player/Heavy/low/201.vcd" 
	scene "scenes/Player/Heavy/low/2074.vcd" 
	scene "scenes/Player/Heavy/low/2075.vcd" 
	scene "scenes/Player/Heavy/low/2076.vcd" 
	scene "scenes/Player/Heavy/low/2077.vcd" 
	scene "scenes/Player/Heavy/low/2081.vcd" 
	scene "scenes/Player/Heavy/low/2082.vcd" 
	scene "scenes/Player/Heavy/low/2103.vcd" 
	scene "scenes/Player/Heavy/low/258.vcd" 
	scene "scenes/Player/Heavy/low/327.vcd" 
}
Rule RejectedDuelHeavy
{
	criteria ConceptDuelRejected IsHeavy
	Response RejectedDuelHeavy
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 1
//--------------------------------------------------------------------------------------------------------------
Response PlayerGoHeavy
{
	scene "scenes/Player/Heavy/low/231.vcd" 
	scene "scenes/Player/Heavy/low/232.vcd" 
	scene "scenes/Player/Heavy/low/233.vcd" 
}
Rule PlayerGoHeavy
{
	criteria ConceptPlayerGo IsHeavy
	Response PlayerGoHeavy
}

Response PlayerHeadLeftHeavy
{
	scene "scenes/Player/Heavy/low/237.vcd" 
	scene "scenes/Player/Heavy/low/238.vcd" 
	scene "scenes/Player/Heavy/low/239.vcd" 
	scene "scenes/Player/Heavy/low/2276.vcd" 
}
Rule PlayerHeadLeftHeavy
{
	criteria ConceptPlayerLeft  IsHeavy
	Response PlayerHeadLeftHeavy
}

Response PlayerHeadRightHeavy
{
	scene "scenes/Player/Heavy/low/240.vcd" 
	scene "scenes/Player/Heavy/low/241.vcd" 
	scene "scenes/Player/Heavy/low/242.vcd" 
	scene "scenes/Player/Heavy/low/2275.vcd" 
}
Rule PlayerHeadRightHeavy
{
	criteria ConceptPlayerRight  IsHeavy
	Response PlayerHeadRightHeavy
}

Response PlayerHelpHeavy
{
	scene "scenes/Player/Heavy/low/243.vcd" 
	scene "scenes/Player/Heavy/low/244.vcd" 
	scene "scenes/Player/Heavy/low/245.vcd" 
}
Rule PlayerHelpHeavy
{
	criteria ConceptPlayerHelp IsHeavy
	Response PlayerHelpHeavy
}

Response PlayerHelpCaptureHeavy
{
	scene "scenes/Player/Heavy/low/246.vcd" 
	scene "scenes/Player/Heavy/low/247.vcd" 
	scene "scenes/Player/Heavy/low/248.vcd" 
}
Rule PlayerHelpCaptureHeavy
{
	criteria ConceptPlayerHelp IsHeavy IsOnCappableControlPoint
	ApplyContext "HeavyHelpCap:1:10"
	Response PlayerHelpCaptureHeavy
}

Response PlayerHelpCapture2Heavy
{
	scene "scenes/Player/Heavy/low/321.vcd" 
	scene "scenes/Player/Heavy/low/322.vcd" 
	scene "scenes/Player/Heavy/low/1276.vcd" 
	scene "scenes/Player/Heavy/low/1277.vcd" 
}
Rule PlayerHelpCapture2Heavy
{
	criteria ConceptPlayerHelp IsHeavy IsOnCappableControlPoint IsHelpCapHeavy
	Response PlayerHelpCapture2Heavy
}

// Custom stuff
// Response for when the Heavy is fighting on a cappable point
Response PlayerGetOnPointHeavy
{
	scene "scenes/Player/Heavy/low/1947.vcd" 
	scene "scenes/Player/Heavy/low/2195.vcd" 
	scene "scenes/Player/Heavy/low/2196.vcd" 
	scene "scenes/Player/Heavy/low/2197.vcd" 
}

Rule PlayerGetOnPointHeavy
{
	criterion ConceptFireWeapon IsHeavy IsOnCappableControlPoint NotDefendOnThePointSpeech
	ApplyContext "DefendOnThePointSpeech:1:15"
	applycontexttoworld
	Response PlayerGetOnPointHeavy
}
// End custom

Response PlayerHelpDefendHeavy
{
	scene "scenes/Player/Heavy/low/249.vcd" 
	scene "scenes/Player/Heavy/low/250.vcd" 
	scene "scenes/Player/Heavy/low/251.vcd" 
}
Rule PlayerHelpDefendHeavy
{
	criteria ConceptPlayerHelp IsHeavy IsOnFriendlyControlPoint
	Response PlayerHelpDefendHeavy
}

Response PlayerMedicHeavy
{
	scene "scenes/Player/Heavy/low/274.vcd" 
	scene "scenes/Player/Heavy/low/275.vcd" 
	scene "scenes/Player/Heavy/low/276.vcd" 
}
Rule PlayerMedicHeavy
{
	criteria ConceptPlayerMedic IsHeavy
	Response PlayerMedicHeavy
}

Response PlayerAskForBallHeavy
{
}
Rule PlayerAskForBallHeavy
{
	criteria ConceptPlayerAskForBall IsHeavy
	Response PlayerAskForBallHeavy
}

Response PlayerMoveUpHeavy
{
	scene "scenes/Player/Heavy/low/277.vcd" 
	scene "scenes/Player/Heavy/low/278.vcd" 
	scene "scenes/Player/Heavy/low/279.vcd" 
}
Rule PlayerMoveUpHeavy
{
	criteria ConceptPlayerMoveUp  IsHeavy
	Response PlayerMoveUpHeavy
}

Response PlayerNoHeavy
{
	scene "scenes/Player/Heavy/low/291.vcd" 
	scene "scenes/Player/Heavy/low/292.vcd" 
	scene "scenes/Player/Heavy/low/293.vcd" 
}
Rule PlayerNoHeavy
{
	criteria ConceptPlayerNo  IsHeavy
	Response PlayerNoHeavy
}

Response PlayerThanksHeavy
{
	scene "scenes/Player/Heavy/low/342.vcd" 
	scene "scenes/Player/Heavy/low/343.vcd" 
	scene "scenes/Player/Heavy/low/1278.vcd" 
}
Rule PlayerThanksHeavy
{
	criteria ConceptPlayerThanks IsHeavy
	Response PlayerThanksHeavy
}

// Custom Assist kill response
// As there is no actual concept for assist kills, this is the second best method.
// Say thanks after you kill more than one person.

Response KilledPlayerAssistHeavy
{
	scene "scenes/Player/Heavy/low/320.vcd"
}
Rule KilledPlayerAssistHeavy
{
	criteria ConceptPlayerThanks IsHeavy IsARecentKill KilledPlayerDelay HeavyNotAssistSpeech
	ApplyContext "HeavyAssistSpeech:1:20"
	Response KilledPlayerAssistHeavy
}
// End custom

Response PlayerYesHeavy
{
	scene "scenes/Player/Heavy/low/350.vcd" 
	scene "scenes/Player/Heavy/low/351.vcd" 
	scene "scenes/Player/Heavy/low/352.vcd" 
}
Rule PlayerYesHeavy
{
	criteria ConceptPlayerYes  IsHeavy
	Response PlayerYesHeavy
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 2
//--------------------------------------------------------------------------------------------------------------
Response PlayerActivateChargeHeavy
{
	scene "scenes/Player/Heavy/low/190.vcd" 
	scene "scenes/Player/Heavy/low/191.vcd" 
	scene "scenes/Player/Heavy/low/192.vcd" 
	scene "scenes/Player/Heavy/low/1261.vcd" 
}
Rule PlayerActivateChargeHeavy
{
	criteria ConceptPlayerActivateCharge IsHeavy
	Response PlayerActivateChargeHeavy
}

Response PlayerCloakedSpyHeavy
{
	scene "scenes/Player/Heavy/low/218.vcd" 
	scene "scenes/Player/Heavy/low/219.vcd" 
	scene "scenes/Player/Heavy/low/220.vcd" 
	scene "scenes/Player/Heavy/low/1264.vcd" 
}
Rule PlayerCloakedSpyHeavy
{
	criteria ConceptPlayerCloakedSpy IsHeavy
	Response PlayerCloakedSpyHeavy
}

Response PlayerDispenserHereHeavy
{
	scene "scenes/Player/Heavy/low/280.vcd" 
}
Rule PlayerDispenserHereHeavy
{
	criteria ConceptPlayerDispenserHere IsHeavy
	Response PlayerDispenserHereHeavy
}

Response PlayerIncomingHeavy
{
	scene "scenes/Player/Heavy/low/252.vcd" 
	scene "scenes/Player/Heavy/low/253.vcd" 
	scene "scenes/Player/Heavy/low/254.vcd" 
}
Rule PlayerIncomingHeavy
{
	criteria ConceptPlayerIncoming IsHeavy
	Response PlayerIncomingHeavy
}

Response PlayerSentryAheadHeavy
{
	scene "scenes/Player/Heavy/low/308.vcd" 
	scene "scenes/Player/Heavy/low/309.vcd" 
}
Rule PlayerSentryAheadHeavy
{
	criteria ConceptPlayerSentryAhead IsHeavy
	Response PlayerSentryAheadHeavy
}

Response PlayerSentryHereHeavy
{
	scene "scenes/Player/Heavy/low/281.vcd" 
}
Rule PlayerSentryHereHeavy
{
	criteria ConceptPlayerSentryHere IsHeavy
	Response PlayerSentryHereHeavy
}

Response PlayerTeleporterHereHeavy
{
	scene "scenes/Player/Heavy/low/282.vcd" 
}
Rule PlayerTeleporterHereHeavy
{
	criteria ConceptPlayerTeleporterHere IsHeavy
	Response PlayerTeleporterHereHeavy
}


//--------------------------------------------------------------------------------------------------------------
// Speech Menu 3
//--------------------------------------------------------------------------------------------------------------
Response PlayerBattleCryHeavy
{
	scene "scenes/Player/Heavy/low/205.vcd" 
	scene "scenes/Player/Heavy/low/206.vcd" 
	scene "scenes/Player/Heavy/low/207.vcd" 
	scene "scenes/Player/Heavy/low/208.vcd" 
	scene "scenes/Player/Heavy/low/209.vcd" 
	scene "scenes/Player/Heavy/low/210.vcd" 
}
Rule PlayerBattleCryHeavy
{
	criteria ConceptPlayerBattleCry IsHeavy
	Response PlayerBattleCryHeavy
}

// Custom stuff - melee dare
// Look at enemy, then do battle cry voice command while holding a melee weapon.
Response MeleeDareCombatHeavy
{
	scene "scenes/Player/Heavy/low/1975.vcd"
	scene "scenes/Player/Heavy/low/1976.vcd"
	scene "scenes/Player/Heavy/low/1977.vcd"
	scene "scenes/Player/Heavy/low/2087.vcd"
	scene "scenes/Player/Heavy/low/2090.vcd"
	scene "scenes/Player/Heavy/low/2091.vcd"
	scene "scenes/Player/Heavy/low/2092.vcd"
	scene "scenes/Player/Heavy/low/2093.vcd"
	scene "scenes/Player/Heavy/low/2106.vcd"
	scene "scenes/Player/Heavy/low/2107.vcd"
	scene "scenes/Player/Heavy/low/2119.vcd"
	scene "scenes/Player/Heavy/low/2205.vcd"
	scene "scenes/Player/Heavy/low/2258.vcd"
}
Rule MeleeDareCombatHeavy
{
	criteria ConceptPlayerBattleCry IsWeaponMelee IsHeavy IsCrosshairEnemy
	ApplyContext "IsDaring:1:5"
	Response MeleeDareCombatHeavy
}

Response PlayerShinyCryHeavy
{
	scene "scenes/Player/Heavy/low/1944.vcd"
	scene "scenes/Player/Heavy/low/1940.vcd"
	scene "scenes/Player/Heavy/low/1942.vcd"
	scene "scenes/Player/Heavy/low/1943.vcd"
	scene "scenes/player/heavy/low/2199.vcd"
}
Rule PlayerShinyCryHeavy
{
	criteria ConceptPlayerBattleCry 30PercentChance IsWeaponPrimary IsHeavy WeaponIsNotVanillaPrimary WeaponIsNotTaggedMinigun
	Response PlayerShinyCryHeavy
}

Rule PlayerShinyWindupHeavy
{
	criteria ConceptWindMinigun IsWeaponPrimary IsHeavy WeaponIsNotVanillaPrimary WeaponIsNotTaggedMinigun WeaponIsNotTomislav 5PercentChance HeavyNotShinySpeech HeavyNotKillSpeech
	ApplyContext "HeavyShinySpeech:1:300"
	Response PlayerShinyCryHeavy
}

// Custom response battle cry against an Engineer.
Response PlayerTauntCryHeavy
{
	scene "scenes/player/heavy/low/332.vcd"
	scene "scenes/player/heavy/low/337.vcd"
}
Rule PlayerTauntCryHeavy
{
	criteria ConceptPlayerBattleCry 75PercentChance IsHeavy IsOnEngineer IsCrosshairEnemy
	Response PlayerTauntCryHeavy
}

// Custom response for taunt against non-Heavies
Response PlayerTauntGunHeavy
{
	scene "scenes/player/heavy/low/329.vcd"
	scene "scenes/player/heavy/low/333.vcd"
	scene "scenes/player/heavy/low/335.vcd"
}
Rule PlayerTauntGunHeavy
{
	criterion ConceptPlayerBattleCry 75PercentChance IsHeavy IsNotOnHeavy IsCrosshairEnemy NotGunTauntHeavy IsNotWeaponMelee
	ApplyContext "GunTauntHeavy:1:10"
	Response PlayerTauntGunHeavy
}
//End custom

Response PlayerCheersHeavy
{
	scene "scenes/Player/Heavy/low/211.vcd" 
	scene "scenes/Player/Heavy/low/212.vcd" 
	scene "scenes/Player/Heavy/low/213.vcd" 
	scene "scenes/Player/Heavy/low/214.vcd" 
	scene "scenes/Player/Heavy/low/215.vcd" 
	scene "scenes/Player/Heavy/low/216.vcd" 
	scene "scenes/Player/Heavy/low/217.vcd" 
	scene "scenes/Player/Heavy/low/1263.vcd" 
}
Rule PlayerCheersHeavy
{
	criteria ConceptPlayerCheers IsHeavy
	Response PlayerCheersHeavy
}

Response PlayerGoodJobHeavy
{
	scene "scenes/Player/Heavy/low/234.vcd" 
	scene "scenes/Player/Heavy/low/235.vcd" 
	scene "scenes/Player/Heavy/low/236.vcd" 
	scene "scenes/Player/Heavy/low/1265.vcd" 
}
Rule PlayerGoodJobHeavy
{
	criteria ConceptPlayerGoodJob IsHeavy
	Response PlayerGoodJobHeavy
}

Response PlayerJeersHeavy
{
	scene "scenes/Player/Heavy/low/258.vcd" 
	scene "scenes/Player/Heavy/low/259.vcd" 
	scene "scenes/Player/Heavy/low/260.vcd" 
	scene "scenes/Player/Heavy/low/261.vcd" 
	scene "scenes/Player/Heavy/low/262.vcd" 
	scene "scenes/Player/Heavy/low/263.vcd" 
	scene "scenes/Player/Heavy/low/264.vcd" 
	scene "scenes/Player/Heavy/low/265.vcd" 
	scene "scenes/Player/Heavy/low/266.vcd" 
}
Rule PlayerJeersHeavy
{
	criteria ConceptPlayerJeers IsHeavy
	Response PlayerJeersHeavy
}

Response PlayerLostPointHeavy
{
	scene "scenes/Player/Heavy/low/1267.vcd" 
	scene "scenes/Player/Heavy/low/283.vcd" 
	scene "scenes/Player/Heavy/low/284.vcd" 
	scene "scenes/Player/Heavy/low/285.vcd" 
	scene "scenes/Player/Heavy/low/286.vcd" 
	scene "scenes/Player/Heavy/low/287.vcd" 
}
Rule PlayerLostPointHeavy
{
	criteria ConceptPlayerLostPoint IsHeavy
	Response PlayerLostPointHeavy
}

Response PlayerNegativeHeavy
{
	scene "scenes/Player/Heavy/low/1267.vcd" 
	scene "scenes/Player/Heavy/low/283.vcd" 
	scene "scenes/Player/Heavy/low/284.vcd" 
	scene "scenes/Player/Heavy/low/285.vcd" 
	scene "scenes/Player/Heavy/low/286.vcd" 
	scene "scenes/Player/Heavy/low/287.vcd" 
}
Rule PlayerNegativeHeavy
{
	criteria ConceptPlayerNegative IsHeavy
	Response PlayerNegativeHeavy
}

Response PlayerNiceShotHeavy
{
	scene "scenes/Player/Heavy/low/288.vcd" 
	scene "scenes/Player/Heavy/low/289.vcd" 
	scene "scenes/Player/Heavy/low/290.vcd" 
}
Rule PlayerNiceShotHeavy
{
	criteria ConceptPlayerNiceShot IsHeavy
	Response PlayerNiceShotHeavy
}

Response PlayerPositiveHeavy
{
	scene "scenes/Player/Heavy/low/303.vcd" 
	scene "scenes/Player/Heavy/low/304.vcd" 
	scene "scenes/Player/Heavy/low/305.vcd" 
	scene "scenes/Player/Heavy/low/306.vcd" 
	scene "scenes/Player/Heavy/low/307.vcd" 
}

Response PlayerTauntsHeavy
{
	scene "scenes/Player/Heavy/low/1270.vcd" 
	scene "scenes/Player/Heavy/low/1271.vcd" 
	scene "scenes/Player/Heavy/low/1274.vcd" 
	scene "scenes/Player/Heavy/low/1273.vcd" 
}
Rule PlayerPositiveHeavy
{
	criteria ConceptPlayerPositive IsHeavy
	Response PlayerPositiveHeavy
	Response PlayerTauntsHeavy
}
// Modified to be an extension of the 'positive' voice command.

//--------------------------------------------------------------------------------------------------------------
// MvM Speech
//--------------------------------------------------------------------------------------------------------------
Response MvMBombDroppedHeavy
{
	scene "scenes/Player/Heavy/low/4009.vcd" 
}
Rule MvMBombDroppedHeavy
{
	criteria ConceptMvMBombDropped 5PercentChance IsMvMDefender IsHeavy 
	Response MvMBombDroppedHeavy
}

Response MvMBombCarrierUpgrade1Heavy
{
	scene "scenes/Player/Heavy/low/4005.vcd" 
}
Rule MvMBombCarrierUpgrade1Heavy
{
	criteria ConceptMvMBombCarrierUpgrade1 5PercentChance IsMvMDefender IsHeavy 
	Response MvMBombCarrierUpgrade1Heavy
}

Response MvMBombCarrierUpgrade2Heavy
{
	scene "scenes/Player/Heavy/low/4006.vcd" 
}
Rule MvMBombCarrierUpgrade2Heavy
{
	criteria ConceptMvMBombCarrierUpgrade2 5PercentChance IsMvMDefender IsHeavy 
	Response MvMBombCarrierUpgrade2Heavy
}

Response MvMDefenderDiedEngineerHeavy
{
	scene "scenes/Player/Heavy/low/3962.vcd" 
}
Rule MvMDefenderDiedEngineerHeavy
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimEngineer IsHeavy 
	Response MvMDefenderDiedEngineerHeavy
}

Response MvMDefenderDiedSpyHeavy
{
	scene "scenes/Player/Heavy/low/3963.vcd" 
}
Rule MvMDefenderDiedSpyHeavy
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimSpy IsHeavy 
	Response MvMDefenderDiedSpyHeavy
}

Response MvMDefenderDiedScoutHeavy
{
	scene "scenes/Player/Heavy/low/3964.vcd" 
}
Rule MvMDefenderDiedScoutHeavy
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimScout IsHeavy 
	Response MvMDefenderDiedScoutHeavy
}

Response MvMDefenderDiedHeavyHeavy
{
	scene "scenes/Player/Heavy/low/3965.vcd" 
}
Rule MvMDefenderDiedHeavyHeavy
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimHeavy IsHeavy 
	Response MvMDefenderDiedHeavyHeavy
}

Response MvMDefenderDiedDemomanHeavy
{
	scene "scenes/Player/Heavy/low/3966.vcd" 
}
Rule MvMDefenderDiedDemomanHeavy
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimDemoman IsHeavy 
	Response MvMDefenderDiedDemomanHeavy
}

Response MvMDefenderDiedSniperHeavy
{
	scene "scenes/Player/Heavy/low/3967.vcd" 
}
Rule MvMDefenderDiedSniperHeavy
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimSniper IsHeavy 
	Response MvMDefenderDiedSniperHeavy
}

Response MvMDefenderDiedPyroHeavy
{
	scene "scenes/Player/Heavy/low/3968.vcd" 
}
Rule MvMDefenderDiedPyroHeavy
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimPyro IsHeavy 
	Response MvMDefenderDiedPyroHeavy
}

Response MvMDefenderDiedMedicHeavy
{
	scene "scenes/Player/Heavy/low/3969.vcd" 
}
Rule MvMDefenderDiedMedicHeavy
{
	criteria ConceptMvMDefenderDied 50PercentChance IsMvMDefender IsVictimMedic IsHeavy 
	Response MvMDefenderDiedMedicHeavy
}

Response MvMFirstBombPickupHeavy
{
	scene "scenes/Player/Heavy/low/4004.vcd" 
}
Rule MvMFirstBombPickupHeavy
{
	criteria ConceptMvMFirstBombPickup 5PercentChance IsMvMDefender IsHeavy
	Response MvMFirstBombPickupHeavy
}

Response MvMBombPickupHeavy
{
	scene "scenes/Player/Heavy/low/4003.vcd"
	scene "scenes/Player/Heavy/low/4004.vcd"  
}
Rule MvMBombPickupHeavy
{
	criteria ConceptMvMBombPickup 5PercentChance IsMvMDefender IsHeavy
	Response MvMBombPickupHeavy
}

Response MvMSniperCalloutHeavy
{
	scene "scenes/Player/Heavy/low/3972.vcd" 
}
Rule MvMSniperCalloutHeavy
{
	criteria ConceptMvMSniperCallout 50PercentChance IsMvMDefender IsHeavy
	Response MvMSniperCalloutHeavy
}

Response MvMSentryBusterHeavy
{
	scene "scenes/Player/Heavy/low/4018.vcd" 
}
Rule MvMSentryBusterHeavy
{
	criteria ConceptMvMSentryBuster 50PercentChance IsMvMDefender IsHeavy
	Response MvMSentryBusterHeavy
}

Response MvMSentryBusterDownHeavy
{
	scene "scenes/Player/Heavy/low/4019.vcd" 
}
Rule MvMSentryBusterDownHeavy
{
	criteria ConceptMvMSentryBusterDown 20PercentChance IsMvMDefender IsHeavy
	Response MvMSentryBusterDownHeavy
}

Response MvMLastManStandingHeavy
{
	scene "scenes/Player/Heavy/low/3970.vcd" 
	scene "scenes/Player/Heavy/low/3971.vcd" 
}
Rule MvMLastManStandingHeavy
{
	criteria ConceptMvMLastManStanding 20PercentChance IsMvMDefender IsHeavy
	Response MvMLastManStandingHeavy
}

Response MvMEncourageMoneyHeavy
{
	scene "scenes/Player/Heavy/low/3988.vcd" 
	scene "scenes/Player/Heavy/low/3989.vcd" 
	scene "scenes/Player/Heavy/low/3990.vcd" 
	scene "scenes/Player/Heavy/low/3991.vcd" 
}
Rule MvMEncourageMoneyHeavy
{
	criteria ConceptMvMEncourageMoney 50PercentChance IsMvMDefender IsHeavy
	Response MvMEncourageMoneyHeavy
}

Response MvMEncourageUpgradeHeavy
{
	scene "scenes/Player/Heavy/low/4000.vcd" 
	scene "scenes/Player/Heavy/low/4001.vcd" 
	scene "scenes/Player/Heavy/low/4002.vcd" 
}
Rule MvMEncourageUpgradeHeavy
{
	criteria ConceptMvMEncourageUpgrade 50PercentChance IsMvMDefender IsHeavy
	Response MvMEncourageUpgradeHeavy
}

Response MvMUpgradeCompleteHeavy
{
	scene "scenes/Player/Heavy/low/3992.vcd" 
	scene "scenes/Player/Heavy/low/3993.vcd" 
	scene "scenes/Player/Heavy/low/3994.vcd" 
	scene "scenes/Player/Heavy/low/3995.vcd" 
	scene "scenes/Player/Heavy/low/3996.vcd" 
	scene "scenes/Player/Heavy/low/3997.vcd" 

}
Rule MvMUpgradeCompleteHeavy
{
	criteria ConceptMvMUpgradeComplete 5PercentChance IsMvMDefender IsHeavy
	Response MvMUpgradeCompleteHeavy
}

Response MvMGiantCalloutHeavy
{
	scene "scenes/Player/Heavy/low/4020.vcd" 
}
Rule MvMGiantCalloutHeavy
{
	criteria ConceptMvMGiantCallout 20PercentChance IsMvMDefender IsHeavy
	Response MvMGiantCalloutHeavy
}

Response MvMGiantHasBombHeavy
{
	scene "scenes/Player/Heavy/low/4024.vcd" 
}
Rule MvMGiantHasBombHeavy
{
	criteria ConceptMvMGiantHasBomb 20PercentChance IsMvMDefender IsHeavy
	Response MvMGiantHasBombHeavy
}

Response MvMSappedRobotHeavy
{
	scene "scenes/Player/Heavy/low/3975.vcd" 
	scene "scenes/Player/Heavy/low/3976.vcd" 
}
Rule MvMSappedRobotHeavy
{
	criteria ConceptMvMSappedRobot 50PercentChance IsMvMDefender IsHeavy
	Response MvMSappedRobotHeavy
}

Response MvMCloseCallHeavy
{
	scene "scenes/Player/Heavy/low/4007.vcd" 
	scene "scenes/Player/Heavy/low/4008.vcd" 
}
Rule MvMCloseCallHeavy
{
	criteria ConceptMvMCloseCall 50PercentChance IsMvMDefender IsHeavy
	Response MvMCloseCallHeavy
}

Response MvMTankCalloutHeavy
{
	scene "scenes/Player/Heavy/low/4011.vcd" 
}
Rule MvMTankCalloutHeavy
{
	criteria ConceptMvMTankCallout 50PercentChance IsMvMDefender IsHeavy
	Response MvMTankCalloutHeavy
}

Response MvMTankDeadHeavy
{
	scene "scenes/Player/Heavy/low/4017.vcd" 
}
Rule MvMTankDeadHeavy
{
	criteria ConceptMvMTankDead 50PercentChance IsMvMDefender IsHeavy
	Response MvMTankDeadHeavy
}

Response MvMTankDeployingHeavy
{
	scene "scenes/Player/Heavy/low/4016.vcd" 
}
Rule MvMTankDeployingHeavy
{
	criteria ConceptMvMTankDeploying 50PercentChance IsMvMDefender IsHeavy
	Response MvMTankDeployingHeavy
}

Response MvMAttackTheTankHeavy
{
	scene "scenes/Player/Heavy/low/4012.vcd" 
	scene "scenes/Player/Heavy/low/4013.vcd" 
}
Rule MvMAttackTheTankHeavy
{
	criteria ConceptMvMAttackTheTank 50PercentChance IsMvMDefender IsHeavy
	Response MvMAttackTheTankHeavy
}

Response MvMTauntHeavy
{
	scene "scenes/Player/Heavy/low/3977.vcd" 
	scene "scenes/Player/Heavy/low/3978.vcd" 
}
Rule MvMTauntHeavy
{
	criteria ConceptMvMTaunt 50PercentChance IsMvMDefender IsHeavy
	Response MvMTauntHeavy
}

Response MvMWaveWinHeavy
{
	scene "scenes/Player/Heavy/low/3949.vcd" 
	scene "scenes/Player/Heavy/low/3950.vcd" 
	scene "scenes/Player/Heavy/low/3951.vcd" 
	scene "scenes/Player/Heavy/low/3952.vcd" 
	scene "scenes/Player/Heavy/low/3953.vcd" 
}
Rule MvMWaveWinHeavy
{
	criteria ConceptMvMWaveWin 50PercentChance IsMvMDefender IsHeavy
	Response MvMWaveWinHeavy
}

Response MvMGiantKilledHeavy
{
	scene "scenes/Player/Heavy/low/4023.vcd" 
}
Rule MvMGiantKilledHeavy
{
	criteria ConceptMvMGiantKilled 50PercentChance IsMvMDefender IsHeavy
	Response MvMGiantKilledHeavy
}

Response MvMGiantKilledTeammateHeavy
{
	scene "scenes/Player/Heavy/low/4022.vcd" 
}
Rule MvMGiantKilledTeammateHeavy
{
	criteria ConceptMvMGiantKilledTeammate 50PercentChance IsMvMDefender IsHeavy
	Response MvMGiantKilledTeammateHeavy
}

Response MvMDeployRageHeavy
{
	scene "scenes/Player/Heavy/low/3983.vcd" 
	scene "scenes/Player/Heavy/low/3985.vcd" 
	scene "scenes/Player/Heavy/low/3986.vcd" 
	scene "scenes/Player/Heavy/low/3987.vcd" 
}
Rule MvMDeployRageHeavy
{
	criteria ConceptMvMDeployRage 50PercentChance IsMvMDefender IsHeavy
	Response MvMDeployRageHeavy
}

Response RocketDestroyedHeavy
{
	scene "scenes/Player/Heavy/low/3979.vcd" 
	scene "scenes/Player/Heavy/low/3980.vcd" 
	scene "scenes/Player/Heavy/low/3982.vcd" 
}
Rule RocketDestroyedHeavy
{
	criteria ConceptRocketDestroyed 50PercentChance IsHeavy
	Response RocketDestroyedHeavy
}

//--------------------------------------------------------------------------------------------------------------
// Begin Competitive Mode VO
//--------------------------------------------------------------------------------------------------------------
Response PlayerFirstRoundStartCompHeavy
{
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_09.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_comp_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_comp_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_rare_01.vcd" predelay "1.0, 5.0"
}
Rule PlayerFirstRoundStartCompHeavy
{
	criteria ConceptPlayerRoundStartComp IsHeavy IsFirstRound IsNotComp6v6 40PercentChance
	Response PlayerFirstRoundStartCompHeavy
}

Response PlayerFirstRoundStartComp6sHeavy
{
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_09.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_comp_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_comp_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_rare_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_6s_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_6s_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamefirst_6s_03.vcd" predelay "1.0, 5.0"
}
Rule PlayerFirstRoundStartComp6sHeavy
{
	criteria ConceptPlayerRoundStartComp IsHeavy IsFirstRound IsComp6v6 40PercentChance
	Response PlayerFirstRoundStartComp6sHeavy
}

Response PlayerWonPrevRoundCompHeavy
{
	scene "scenes/Player/Heavy/low/cm_heavy_pregamewonlast_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamewonlast_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamewonlast_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamewonlast_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamewonlast_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamewonlast_06.vcd" predelay "1.0, 5.0"
}
Rule PlayerWonPrevRoundCompHeavy
{
	criteria ConceptPlayerRoundStartComp IsHeavy IsNotFirstRound PlayerWonPreviousRound 40PercentChance
	Response PlayerWonPrevRoundCompHeavy
}

Response PlayerLostPrevRoundCompHeavy
{
	scene "scenes/Player/Heavy/low/cm_heavy_pregamelostlast_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamelostlast_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamelostlast_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamelostlast_04.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamelostlast_05.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamelostlast_06.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamelostlast_07.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamelostlast_08.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamelostlast_09.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregamelostlast_10.vcd" predelay "1.0, 5.0"
}
Rule PlayerLostPrevRoundCompHeavy
{
	criteria ConceptPlayerRoundStartComp IsHeavy IsNotFirstRound PlayerLostPreviousRound PreviousRoundWasNotTie 40PercentChance
	Response PlayerLostPrevRoundCompHeavy
}

Response PlayerTiedPrevRoundCompHeavy
{
	scene "scenes/Player/Heavy/low/cm_heavy_pregametie_01.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregametie_02.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregametie_03.vcd" predelay "1.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_pregametie_04.vcd" predelay "1.0, 5.0"
}
Rule PlayerTiedPrevRoundCompHeavy
{
	criteria ConceptPlayerRoundStartComp IsHeavy IsNotFirstRound PreviousRoundWasTie 40PercentChance
	Response PlayerTiedPrevRoundCompHeavy
}

Response PlayerGameWinCompHeavy
{
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_02.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_03.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_04.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_05.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_06.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_comp_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_comp_02.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_comp_03.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_rare_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_rare_02.vcd" predelay "2.0, 5.0"
}
Rule PlayerGameWinCompHeavy
{
	criteria ConceptPlayerGameOverComp PlayerOnWinningTeam IsNotComp6v6 IsHeavy 40PercentChance
	Response PlayerGameWinCompHeavy
}

Response PlayerGameWinComp6sHeavy
{
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_02.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_03.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_04.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_05.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_06.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_comp_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_comp_02.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_comp_03.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_rare_01.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_rare_02.vcd" predelay "2.0, 5.0"
	scene "scenes/Player/Heavy/low/cm_heavy_gamewon_6s_01.vcd" predelay "2.0, 5.0"
}
Rule PlayerGameWinComp6sHeavy
{
	criteria ConceptPlayerGameOverComp PlayerOnWinningTeam IsComp6v6 IsHeavy 40PercentChance
	Response PlayerGameWinComp6sHeavy
}

Response PlayerMatchWinCompHeavy
{
	scene "scenes/Player/Heavy/low/cm_heavy_matchwon_01.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Heavy/low/cm_heavy_matchwon_02.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Heavy/low/cm_heavy_matchwon_03.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Heavy/low/cm_heavy_matchwon_04.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Heavy/low/cm_heavy_matchwon_05.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Heavy/low/cm_heavy_matchwon_06.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Heavy/low/cm_heavy_matchwon_07.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Heavy/low/cm_heavy_matchwon_08.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Heavy/low/cm_heavy_matchwon_09.vcd" predelay "1.0, 2.0"
	scene "scenes/Player/Heavy/low/cm_heavy_matchwon_10.vcd" predelay "1.0, 2.0"
}
Rule PlayerMatchWinCompHeavy
{
	criteria ConceptPlayerMatchOverComp PlayerOnWinningTeam IsHeavy 40PercentChance
	Response PlayerMatchWinCompHeavy
}
//--------------------------------------------------------------------------------------------------------------
// End Competitive Mode VO
//--------------------------------------------------------------------------------------------------------------