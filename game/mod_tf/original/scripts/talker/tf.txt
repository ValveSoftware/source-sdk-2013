//============================================================================================================
// TF criteria - moved to response_rules.txt file.  Please keep all definitions there.
//============================================================================================================

	
//============================================================================================================
// Taunts, idle expressions, firing expressions (not all chars have idle responses, depends on their 'default' face)
//============================================================================================================


// Pyro
//------------------------------------------------------------------------------------------------------------

	response "PyroTauntPrimary"
	{
		scene "scenes/player/pyro/low/taunt01_v1.vcd"
		scene "scenes/player/pyro/low/taunt01_v2.vcd"
		scene "scenes/player/pyro/low/taunt01_v3.vcd"
	}
	response "PyroTauntSecondary"
	{
		scene "scenes/player/pyro/low/taunt02.vcd"
	}
	response "PyroTauntMelee"
	{
		scene "scenes/player/pyro/low/taunt03.vcd"
	}
	response "PyroTauntItem1"
	{
		scene "scenes/player/pyro/low/taunt02.vcd"
	}
	response "PyroTauntRainblower"
	{
		scene "scenes/player/pyro/low/taunt_bubbles.vcd"
	}
	response "PyroTauntScorchShot"
	{
		scene "scenes/player/pyro/low/taunt_scorch_shot.vcd"
	}
	response "PyroTauntLollichop"
	{
		scene "scenes/player/pyro/low/taunt_lollichop.vcd"
	}
	response "PyroTauntHalloween"
	{
		scene "scenes/player/pyro/low/taunt06.vcd"
	}

	rule PyroTauntPlayerPrimary
	{
		criteria ConceptPlayerTaunt IsPyro WeaponIsFlamethrower
		response PyroTauntPrimary
	}
	rule PyroTauntDragonsFury
	{
		criteria ConceptPlayerTaunt IsPyro WeaponIsDragonsFury
		response PyroTauntPrimary
	}

	rule PyroTauntPlayerSecondary
	{
		criteria ConceptPlayerTaunt IsPyro WeaponIsShotgunPyro
		response PyroTauntSecondary
	}
	rule PyroTauntPlayerMelee
	{
		criteria ConceptPlayerTaunt IsPyro WeaponIsAxe
		response PyroTauntMelee
	}
	rule PyroTauntPlayerItem1
	{
		criteria ConceptPlayerTaunt IsPyro WeaponIsFlaregun
		response PyroTauntItem1
	}
	rule PyroTauntPlayerRainblower
	{
		criteria ConceptPlayerTaunt IsPyro WeaponIsRainblower
		response PyroTauntRainblower
	}
	rule PyroTauntPlayerScorchShot
	{
		criteria ConceptPlayerTaunt IsPyro WeaponIsScorchShot
		response PyroTauntScorchShot
	}
	rule PyroTauntPlayerLollichop
	{
		criteria ConceptPlayerTaunt IsPyro WeaponIsLollichop
		response PyroTauntLollichop
	}
	rule PyroTauntHalloween
	{
		criteria ConceptPlayerTaunt IsPyro IsHalloweenTaunt
		response PyroTauntHalloween
	}

	Response "PyroTauntThirdDegree"
	{
		scene "scenes/player/pyro/low/drg_axe_taunt.vcd"	
	}
	Rule PyroTauntThirdDegree
	{
		criteria ConceptPlayerTaunt IsPyro WeaponIsThirdDegree
		response PyroTauntThirdDegree
	}
	
	Rule PyroTauntManmelter
	{
		criteria ConceptPlayerTaunt IsPyro WeaponIsManmelter
		response PyroTauntSecondary
	}

	Response "PyroTauntAnnihilator"
	{
		scene "scenes/player/pyro/low/annihilator_taunt.vcd"	
	}
	Rule PyroTauntAnnihilator
	{
		criteria ConceptPlayerTaunt IsPyro WeaponIsAnnihilator
		response PyroTauntAnnihilator
	}	
	Rule PyroTauntPromoAnnihilator
	{
		criteria ConceptPlayerTaunt IsPyro WeaponIsPromoAnnihilator
		response PyroTauntAnnihilator
	}

	Rule PyroTauntGasCan
	{
		criteria ConceptPlayerTaunt IsPyro WeaponIsGasCan
		response PyroTauntSecondary
	}
	
	Rule PyroTauntSlap
	{
		criteria ConceptPlayerTaunt IsPyro WeaponIsSlap
		response PyroTauntSecondary
	}	

//============================================================================================================


// Heavy
//------------------------------------------------------------------------------------------------------------
	response "HeavyTauntPrimary"
	{
		scene "scenes/player/heavy/low/taunt01.vcd"
		scene "scenes/player/heavy/low/taunt01_v2.vcd"
		scene "scenes/player/heavy/low/taunt01_v3.vcd"
	}
	response "HeavyTauntSecondary"
	{
		scene "scenes/player/heavy/low/taunt02.vcd"
		scene "scenes/player/heavy/low/taunt02_v1.vcd"
		scene "scenes/player/heavy/low/taunt02_v2.vcd"
	}
	response "HeavyTauntMelee"
	{
		scene "scenes/player/heavy/low/taunt03_v1.vcd"
	}
	response "HeavyTauntPlayerItem1"
	{
		scene "scenes/player/heavy/low/taunt04.vcd"
	}
	response "HeavyTauntPlayerItem2"
	{
		scene "scenes/player/heavy/low/taunt04.vcd"
	}
	response "HeavyTauntPlayerItem2Alt"
	{
		scene "scenes/player/heavy/low/taunt04.vcd"
	}
	response "HeavyTauntPlayerItem3"
	{
		scene "scenes/player/heavy/low/taunt04.vcd"
	}
	response "HeavyTauntPlayerGloves"
	{
		scene "scenes/player/heavy/low/taunt05.vcd"
	}
	response "HeavyTauntHalloween"
	{
		scene "scenes/player/heavy/low/taunt06.vcd"
	}
	response "HeavyTauntFrankenHeavy"
	{
		scene "scenes/player/heavy/low/taunt07_halloween.vcd"
	}

	rule HeavyTauntPlayerPrimary
	{
		criteria ConceptPlayerTaunt IsHeavy WeaponIsMinigun
		response HeavyTauntPrimary
	}
	rule HeavyTauntSaxxy
 	{
 		criteria ConceptPlayerTaunt IsHeavy WeaponIsSaxxy
 		response HeavyTauntMelee
 	}
	rule HeavyTauntPlayerSecondary
	{
		criteria ConceptPlayerTaunt IsHeavy WeaponIsShotgunHwg
		response HeavyTauntSecondary
	}
	rule HeavyTauntPlayerMelee
	{
		criteria ConceptPlayerTaunt IsHeavy WeaponIsFists
		response HeavyTauntMelee
	}
	rule HeavyTauntPlayerGloves
	{
		criteria ConceptPlayerTaunt IsHeavy WeaponIsGloves
		response HeavyTauntPlayerGloves
	}
	rule HeavyTauntPlayerItem1
	{
		criteria ConceptPlayerTaunt IsHeavy WeaponIsSandvich
		response HeavyTauntPlayerItem1
	}
	rule HeavyTauntPlayerRobotSandvich
	{
		criteria ConceptPlayerTaunt IsHeavy WeaponIsRobotSandvich
		response HeavyTauntPlayerItem1
	}
	rule HeavyTauntPlayerFestiveSandvich
	{
		criteria ConceptPlayerTaunt IsHeavy WeaponIsFestiveSandvich
		response HeavyTauntPlayerItem1
	}
	rule HeavyTauntPlayerLunchbox
	{
		criteria ConceptPlayerTaunt IsHeavy WeaponIsLunchbox
		response HeavyTauntPlayerItem1
	}
	// Custom stuff
	// This is to let you taunt with the items.
	rule HeavyTauntPlayerItem2
	{
		criteria ConceptPlayerTaunt IsHeavy WeaponIsBenja
		response HeavyTauntPlayerItem3
	}
	rule HeavyTauntPlayerItem2Alt
	{
		criteria ConceptPlayerTaunt IsHeavy WeaponIsFishcake
		response HeavyTauntPlayerItem3
	}
	rule HeavyTauntPlayerItem3
	{
		criteria ConceptPlayerTaunt IsHeavy WeaponIsSteak
		response HeavyTauntPlayerItem3
	}
	//End Custom
	rule HeavyTauntHalloween
	{
		criteria ConceptPlayerTaunt IsHeavy IsHalloweenTaunt
		response HeavyTauntHalloween
	}
	rule HeavyTauntFrankenHeavy
	{
		criteria ConceptPlayerTaunt IsHeavy IsFrankenHeavy
		response HeavyTauntFrankenHeavy
	}
			
	Response PlayerExpressionIdleHeavy
	{
		scene "scenes/player/heavy/low/idleloop01.vcd"
	}
	Rule PlayerExpressionIdleHeavy
	{
		criteria ConceptPlayerExpression IsHeavy
		Response PlayerExpressionIdleHeavy
	}

	Response PlayerExpressionIdleCompWinnerHeavy
	{
		scene "scenes/player/heavy/low/comp_winner_idle_face.vcd"
	}
	Rule PlayerExpressionIdleCompWinnerHeavy
	{
		criteria ConceptPlayerExpression IsHeavy IsCompWinner
		Response PlayerExpressionIdleCompWinnerHeavy
	}
	

	Response PlayerExpressionFiringMinigunHeavy
	{
		scene "scenes/player/heavy/low/attackMinigun02.vcd"
	}
	Rule PlayerExpressionFiringMinigunHeavy
	{
		criteria ConceptPlayerExpression IsHeavy IsFiringMinigun
		Response PlayerExpressionFiringMinigunHeavy
	}
	
	// Minigun vocal stuff has been moved to Heavy.txt !
	
	Response PlayerExpressionIdleHurtHeavy
	{
		scene "scenes/player/heavy/low/idleloopHurt01.vcd"
	}
	Rule PlayerExpressionIdleHurtHeavy
	{
		criteria ConceptPlayerExpression IsHeavy LowHealthContext
		Response PlayerExpressionIdleHurtHeavy
	}
	Rule PlayerExpressionRoundLossHeavy
	{
		criteria ConceptPlayerExpression IsHeavy GameRulesInWinState PlayerOnLosingTeam
		Response PlayerExpressionIdleHurtHeavy
	}
	
	Response HeavyFireMinigun
	{
		//scene "scenes/player/heavy/low/attackMinigun01.vcd" *bug*
		scene "scenes/player/heavy/low/attackMinigun02.vcd"
	}
	Rule HeavyFireMinigun
	{
		criteria ConceptFireMinigun IsHeavy WeaponIsMinigun
		Response HeavyFireMinigun
	}
	
	Response HeavyWindMinigun
	{
		scene "scenes/player/heavy/low/attackWindup01.vcd"
	}
	Rule HeavyWindMinigunHeavy
	{
		criteria ConceptWindMinigun IsHeavy WeaponIsMinigun
		Response HeavyWindMinigun
	}
	
	Response PlayerExpressionAttackHeavy
	{
		scene "scenes/player/heavy/low/attack01.vcd"
	}
	Rule PlayerExpressionAttackHeavy
	{
		criteria ConceptFireWeapon IsHeavy
		Response PlayerExpressionAttackHeavy
	}
	
	//--------------------------------------------------------------------------------------------------------------
	// Auto Speech Sandwich
	//--------------------------------------------------------------------------------------------------------------
	Response AteSandwichHeavy
	{
		scene "scenes/Player/Heavy/low/SandwichTaunt01.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt02.vcd" predelay "1.0"  
		scene "scenes/Player/Heavy/low/SandwichTaunt03.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt04.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt05.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt06.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt07.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt08.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt09.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt10.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt11.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt12.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt13.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt14.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt15.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt16.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt17.vcd" predelay "1.0" 
	}
	Rule AteSandwichHeavy
	{
		criteria ConceptAteFood IsHeavy WeaponIsSandvich
		Response AteSandwichHeavy
	}
	Rule AteRobotSandwichHeavy
	{
		criteria ConceptAteFood IsHeavy WeaponIsRobotSandvich
		Response AteSandwichHeavy
	}
	Rule AteFestiveSandwichHeavy
	{
		criteria ConceptAteFood IsHeavy WeaponIsFestiveSandvich
		Response AteSandwichHeavy
	}
	// Custom stuff
	// These have been designed as separate responses so that I can add non-sandvich lines I wanted to.
	// There are two examples of this in the AteChocoHeavy response.
	// It's possible to just have one of the responses below, but you would not be able to give the bar/steak separate lines then.
	// The IsWeaponMelee allows this to happen, creating a distinction between the choco bar, sandvich and the steak.
	Response AteSteakHeavy
	{
		scene "scenes/Player/Heavy/low/SandwichTaunt01.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt02.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt14.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt17.vcd" predelay "1.0" 
	}
	Rule AteSteakHeavy
	{
		criteria ConceptAteFood IsHeavy IsWeaponMelee
		Response AteSteakHeavy
	}
	Response AteChocoHeavy
	{
		scene "scenes/Player/Heavy/low/SandwichTaunt01.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt02.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt14.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/SandwichTaunt17.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/2066.vcd" predelay "1.0" 
		scene "scenes/Player/Heavy/low/2260.vcd" predelay "1.0" 
	}
	Rule AteChocoHeavy
	{
		criteria ConceptAteFood IsHeavy
		Response AteChocoHeavy
	}


//============================================================================================================


// Engineer
//------------------------------------------------------------------------------------------------------------
	response "EngineerTauntPrimary"
	{
		scene "scenes/player/engineer/low/taunt01_vocal01.vcd"
		scene "scenes/player/engineer/low/taunt01_vocal02.vcd"
		scene "scenes/player/engineer/low/taunt01_vocal03.vcd"
		scene "scenes/player/engineer/low/taunt01_vocal04.vcd"
	}
	response "EngineerTauntSecondary"
	{
		scene "scenes/player/engineer/low/taunt02_vocal01.vcd"
		scene "scenes/player/engineer/low/taunt02_vocal02.vcd"
		scene "scenes/player/engineer/low/taunt02_vocal03.vcd"
	}
	response "EngineerTauntMelee"
	{
		scene "scenes/player/engineer/low/taunt03.vcd"
	}
	response "EngineerTauntHalloween"
	{
		scene "scenes/player/engineer/low/taunt06.vcd"
	}
	response "EngineerTauntGuitar"
	{
		scene "scenes/player/engineer/low/taunt07.vcd"
	}
	response "EngineerTauntRobotArm"
	{
		scene "scenes/player/engineer/low/taunt09.vcd"
	}

	rule EngineerTauntPlayerPrimary
	{
		criteria ConceptPlayerTaunt IsEngineer WeaponIsShotgunPrimary
		response EngineerTauntPrimary
	}
	rule EngineerTauntPlayerSecondary
	{
		criteria ConceptPlayerTaunt IsEngineer WeaponIsPistol
		response EngineerTauntSecondary
	}
	rule EngineerTauntPlayerMelee
	{
		criteria ConceptPlayerTaunt IsEngineer WeaponIsWrench
		response EngineerTauntMelee
	}
	rule EngineerTauntPlayerShortCircuit
	{
		criteria ConceptPlayerTaunt IsEngineer WeaponIsShortCircuit
		response EngineerTauntMelee
	}
	rule EngineerTauntGuitar
	{
		criteria ConceptPlayerTaunt IsEngineer WeaponIsFrontierJustice
		response EngineerTauntGuitar
	}
	rule EngineerTauntFestiveGuitar
	{
		criteria ConceptPlayerTaunt IsEngineer WeaponIsFestiveFrontierJustice
		response EngineerTauntGuitar
	}
	rule EngineerTauntRobotArm
	{
		criteria ConceptPlayerTaunt IsEngineer WeaponIsRobotArm
		response EngineerTauntRobotArm
	}
	rule EngineerTauntLaserPointer
	{
		criteria ConceptPlayerTaunt IsEngineer WeaponIsLaserPointer
		response EngineerTauntSecondary
	}
	rule EngineerTauntHalloween
	{
		criteria ConceptPlayerTaunt IsEngineer IsHalloweenTaunt
		response EngineerTauntHalloween
	}

	Response PlayerExpressionIdleEngineer
	{
		scene "scenes/player/engineer/low/idleloop01.vcd"
	}
	Rule PlayerExpressionIdleEngineer
	{
		criteria ConceptPlayerExpression IsEngineer
		Response PlayerExpressionIdleEngineer
	}
	Response PlayerExpressionIdleCompWinnerEngineer
	{
		scene "scenes/player/engineer/low/comp_winner_idle_face.vcd"
	}
	Rule PlayerExpressionIdleCompWinnerEngineer
	{
		criteria ConceptPlayerExpression IsEngineer IsCompWinner
		Response PlayerExpressionIdleCompWinnerEngineer
	}
	
	Response PlayerExpressionIdleHurtEngineer
	{
		scene "scenes/player/engineer/low/idleloopHurt01.vcd"
	}
	Rule PlayerExpressionIdleHurtEngineer
	{
		criteria ConceptPlayerExpression IsEngineer LowHealthContext
		Response PlayerExpressionIdleHurtEngineer
	}
	Rule PlayerExpressionRoundLossEngineer
	{
		criteria ConceptPlayerExpression IsEngineer GameRulesInWinState PlayerOnLosingTeam
		Response PlayerExpressionIdleHurtEngineer
	}
	
	Response PlayerExpressionAttackEngineer
	{
		scene "scenes/player/engineer/low/attack01.vcd"
	}
	Rule PlayerExpressionAttackEngineer
	{
		criteria ConceptFireWeapon IsEngineer
		Response PlayerExpressionAttackEngineer
	}

	Response "EngineerTauntEurekaEffect"
	{
		scene "scenes/player/engineer/low/taunt_drg_melee.vcd"	
	}
	Rule EngineerTauntEurekaEffect
	{
		criteria ConceptTauntEurekaTeleport IsEngineer WeaponIsEurekaEffect
		response EngineerTauntEurekaEffect
	}

	Rule EngineerTauntPomson
	{
		criteria ConceptPlayerTaunt IsEngineer WeaponIsPomson
		response EngineerTauntPrimary
	}
	rule EngineerTauntPlayerRescueRanger
	{
		criteria ConceptPlayerTaunt IsEngineer WeaponIsRescueRanger
		response EngineerTauntPrimary
	}

//============================================================================================================


// Medic
//------------------------------------------------------------------------------------------------------------
	response "MedicTauntPrimary"
	{
		scene "scenes/player/medic/low/taunt01_vocal01.vcd"
		scene "scenes/player/medic/low/taunt01_vocal03.vcd"
		scene "scenes/player/medic/low/taunt01_vocal04.vcd"
		scene "scenes/player/medic/low/taunt01_vocal05.vcd"
		
		  
	}
	response "MedicTauntSecondary"
	{
		scene "scenes/player/medic/low/taunt02_v1.vcd"
	}
	response "MedicTauntMelee"
	{
		scene "scenes/player/medic/low/taunt03.vcd"
	}
	response "MedicTauntHalloween"
	{
		scene "scenes/player/medic/low/taunt07.vcd"
	}
	response "MedicTauntKritzkrieg"
	{
		scene "scenes/player/medic/low/taunt06.vcd"
	}
	response "MedicTauntUbersaw"
	{
		scene "scenes/player/medic/low/taunt08.vcd"
	}

	rule MedicTauntPlayerPrimary
	{
		criteria ConceptPlayerTaunt IsMedic WeaponIsSyringe
		response MedicTauntPrimary
	}
	rule MedicTauntPlayerSecondary
	{
		criteria ConceptPlayerTaunt IsMedic WeaponIsHeal
		response MedicTauntSecondary
	}
	rule MedicTauntPlayerMelee
	{
		criteria ConceptPlayerTaunt IsMedic WeaponIsBonesaw
		response MedicTauntMelee
	}
	rule MedicTauntPlayerKritzkrieg
	{
		criteria ConceptPlayerTaunt IsMedic WeaponIsKritzkrieg
		response MedicTauntKritzkrieg
	}
	rule MedicTauntHalloween
	{
		criteria ConceptPlayerTaunt IsMedic IsHalloweenTaunt
		response MedicTauntHalloween
	}
	rule MedicTauntUbersaw
	{
		criteria ConceptPlayerTaunt IsMedic WeaponIsUbersaw
		response MedicTauntUbersaw
	}
	rule MedicTauntFestiveUbersaw
	{
		criteria ConceptPlayerTaunt IsMedic WeaponIsFestiveUbersaw
		response MedicTauntUbersaw
	}
	Rule MedicTauntSaxxy
	{
		criteria ConceptPlayerTaunt IsMedic WeaponIsSaxxy
		response MedicTauntSecondary
	}
	Rule MedicTauntBust
	{
		criteria ConceptPlayerTaunt IsMedic WeaponIsHippocrates
		response MedicTauntPrimary
	}
	Rule MedicTauntHealArrow
	{
		criteria ConceptPlayerTaunt IsMedic WeaponIsHealArrow
		response MedicTauntPrimary	
	}

	Response PlayerExpressionIdleMedic
	{
		scene "scenes/player/medic/low/idleloop01.vcd"
	}
	Rule PlayerExpressionIdleMedic
	{
		criteria ConceptPlayerExpression IsMedic
		Response PlayerExpressionIdleMedic
	}
	Response PlayerExpressionIdleCompWinnerMedic
	{
		scene "scenes/player/medic/low/comp_winner_idle_face.vcd"
	}
	Rule PlayerExpressionIdleCompWinnerMedic
	{
		criteria ConceptPlayerExpression IsMedic IsCompWinner
		Response PlayerExpressionIdleCompWinnerMedic
	}
	
	Response PlayerExpressionIdleHurtMedic
	{
		scene "scenes/player/medic/low/idleloopHurt01.vcd"
	}
	Rule PlayerExpressionIdleHurtMedic
	{
		criteria ConceptPlayerExpression IsMedic LowHealthContext
		Response PlayerExpressionIdleHurtMedic
	}
	Rule PlayerExpressionRoundLossMedic
	{
		criteria ConceptPlayerExpression IsMedic GameRulesInWinState PlayerOnLosingTeam
		Response PlayerExpressionIdleHurtMedic
	}
	
	Response PlayerExpressionAttackMedic
	{
		scene "scenes/player/medic/low/attack01.vcd"
	}
	Rule PlayerExpressionAttackMedic
	{
		criteria ConceptFireWeapon IsMedic
		Response PlayerExpressionAttackMedic
	}

//============================================================================================================


// Soldier
//------------------------------------------------------------------------------------------------------------
	response "SoldierTauntPrimary"
	{
		scene "scenes/player/soldier/low/taunt01_v1.vcd"
		scene "scenes/player/soldier/low/taunt01_v2.vcd"
		scene "scenes/player/soldier/low/taunt01_v3.vcd"
	}
	response "SoldierTauntBanner"
	{
		scene "scenes/player/soldier/low/taunt02_v1.vcd"
		scene "scenes/player/soldier/low/taunt02_v2.vcd"
	}
	response "SoldierTauntSecondary"
	{
		scene "scenes/player/soldier/low/taunt04.vcd"
	}
	response "SoldierTauntMelee"
	{
		scene "scenes/player/soldier/low/taunt03_v1.vcd"
		scene "scenes/player/soldier/low/taunt03_v2.vcd"
		scene "scenes/player/soldier/low/taunt03_v3.vcd"
		scene "scenes/player/soldier/low/taunt03_v4.vcd"
	}
	response "SoldierTauntHalloween"
	{
		scene "scenes/player/soldier/low/taunt06.vcd"
	}
	response "SoldierTauntRobot"
	{
		scene "scenes/player/soldier/low/taunt09.vcd"
	}
	response "SoldierTauntPickaxe"
	{
		scene "scenes/player/soldier/low/taunt05.vcd"
	}
	response "SoldierTauntDirectHit"
	{
		scene "scenes/player/soldier/low/taunt07.vcd"
	}
	response "SoldierTauntCowMangler"
	{
		scene "scenes/player/soldier/low/taunt08.vcd"
	}

	rule SoldierTauntPlayerPrimary
	{
		criteria ConceptPlayerTaunt IsSoldier WeaponIsRocket
		response SoldierTauntPrimary
	}
	rule SoldierTauntPlayerSecondary
	{
		criteria ConceptPlayerTaunt IsSoldier WeaponIsShotgunSoldier
		response SoldierTauntSecondary
	}
	rule SoldierTauntPlayerMelee
	{
		criteria ConceptPlayerTaunt IsSoldier WeaponIsShovel
		response SoldierTauntMelee
	}
	rule SoldierTauntHalloween
	{
		criteria ConceptPlayerTaunt IsSoldier IsHalloweenTaunt
		response SoldierTauntHalloween
	}
	rule SoldierTauntRobot
	{
		criteria ConceptPlayerTaunt IsSoldier IsRobotCostume
		response SoldierTauntRobot
	}
	rule SoldierTauntEqualizer
	{
		criteria ConceptPlayerTaunt IsSoldier WeaponIsEqualizer
		response SoldierTauntPickaxe
	}
	rule SoldierTauntEscapePlan
	{
		criteria ConceptPlayerTaunt IsSoldier WeaponIsEscapePlan
		response SoldierTauntPickaxe
	}
	rule SoldierTauntDirectHit
	{
		criteria ConceptPlayerTaunt IsSoldier WeaponIsDirectHit
		response SoldierTauntDirectHit
	}
	rule SoldierTauntBeggarsBazooka
	{
		criteria ConceptPlayerTaunt IsSoldier WeaponIsBeggarsBazooka
		response SoldierTauntDirectHit
	}
	rule SoldierTauntBanner
	{
		criteria ConceptPlayerTaunt IsSoldier WeaponIsBanner
		response SoldierTauntBanner
	}
	rule SoldierTauntFestiveBanner
	{
		criteria ConceptPlayerTaunt IsSoldier WeaponIsFestiveBanner
		response SoldierTauntBanner
	}
	rule SoldierTauntSashimono
	{
		criteria ConceptPlayerTaunt IsSoldier WeaponIsSashimono
		response SoldierTauntBanner
	}
	rule SoldierTauntKatana
	{
		criteria ConceptPlayerTaunt IsSoldier WeaponIsKatana
		response SoldierTauntDirectHit
	}
	rule SoldierTauntCowMangler
	{
		criteria ConceptPlayerTaunt IsSoldier WeaponIsCowMangler
		response SoldierTauntCowMangler
	}
	rule SoldierTauntRayGun
	{
		criteria ConceptPlayerTaunt IsSoldier WeaponIsRayGun
		response SoldierTauntBanner
	}
	rule SoldierTauntBackup
	{
		criteria ConceptPlayerTaunt IsSoldier WeaponIsBackup
		response SoldierTauntBanner
	}
	rule SoldierTauntSaxxy
	{
		criteria ConceptPlayerTaunt IsSoldier WeaponIsSaxxy
		response SoldierTauntBanner
	}
		
	Response PlayerExpressionIdleSoldier
	{
		scene "scenes/player/soldier/low/idleloop01.vcd"
	}
	Rule PlayerExpressionIdleSoldier
	{
		criteria ConceptPlayerExpression IsSoldier
		Response PlayerExpressionIdleSoldier
	}
	Response PlayerExpressionIdleCompWinnerSoldier
	{
		scene "scenes/player/soldier/low/comp_winner_idle_face.vcd"
	}
	Rule PlayerExpressionIdleCompWinnerSoldier
	{
		criteria ConceptPlayerExpression IsSoldier IsCompWinner
		Response PlayerExpressionIdleCompWinnerSoldier
	}
	Response PlayerExpressionIdleHurtSoldier
	{
		scene "scenes/player/soldier/low/idleloopHurt01.vcd"
	}
	Rule PlayerExpressionIdleHurtSoldier
	{
		criteria ConceptPlayerExpression IsSoldier LowHealthContext
		Response PlayerExpressionIdleHurtSoldier
	}
	Rule PlayerExpressionRoundLossSoldier
	{
		criteria ConceptPlayerExpression IsSoldier GameRulesInWinState PlayerOnLosingTeam
		Response PlayerExpressionIdleHurtSoldier
	}
	
	Response PlayerExpressionAttackSoldier
	{
		scene "scenes/player/soldier/low/attack01.vcd"
	}
	Rule PlayerExpressionAttackSoldier
	{
		criteria ConceptFireWeapon IsSoldier
		Response PlayerExpressionAttackSoldier
	}
	rule SoldierTauntPlayerAirStrike
	{
		criteria ConceptPlayerTaunt IsSoldier WeaponIsRocketLauncherAirStrike
		response SoldierTauntPrimary
	}
//============================================================================================================


// Scout
//------------------------------------------------------------------------------------------------------------
	response "ScoutTauntPrimary"
	{
		scene "scenes/player/scout/low/taunt01_vocal01.vcd"
		scene "scenes/player/scout/low/taunt01_vocal02.vcd"
		scene "scenes/player/scout/low/taunt01_vocal03.vcd"
	}
	response "ScoutTauntSecondary"
	{
		scene "scenes/player/scout/low/taunt02_vocal01.vcd"
		scene "scenes/player/scout/low/taunt02_vocal02.vcd"
		scene "scenes/player/scout/low/taunt02_vocal03.vcd"
	}
	response "ScoutTauntMelee"
	{
		scene "scenes/player/scout/low/taunt03_vocal01.vcd"
		scene "scenes/player/scout/low/taunt03_vocal02.vcd"
		scene "scenes/player/scout/low/taunt03_vocal03.vcd"
		scene "scenes/player/scout/low/taunt03_vocal04.vcd"
		scene "scenes/player/scout/low/taunt03_vocal05.vcd"
		scene "scenes/player/scout/low/taunt03_vocal06.vcd"
	}
	response "ScoutTauntGrandSlam"
	{
		scene "scenes/player/scout/low/taunt05_v1.vcd"
	}
	response "ScoutTauntPrimaryDouble"
	{
		scene "scenes/player/scout/low/taunt01_alt_vocal01.vcd"
		scene "scenes/player/scout/low/taunt01_alt_vocal02.vcd"
		scene "scenes/player/scout/low/taunt01_alt_vocal03.vcd"
	}
	response "ScoutTauntHalloween"
	{
		scene "scenes/player/scout/low/taunt06_v1.vcd"
	}
	response "ScoutTauntPlayerItem1"
	{
		scene "scenes/player/scout/low/taunt04_v1.vcd"
	}
	
	rule ScoutTauntPlayerItem1
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsLunchboxDrink
		ApplyContext "ScoutIsCrit:1:3" // The crit context is set here. You can't fire while using BONK! so we can share the rule/response.
		response ScoutTauntPlayerItem1
	}
	rule ScoutTauntPlayerPrimary
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsScattergun
		response ScoutTauntPrimary
	}
	rule ScoutTauntPlayerSecondary
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsPistolScout
		response ScoutTauntSecondary
	}
	rule ScoutTauntPlayerMelee
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsBat
		response ScoutTauntMelee
	}
	rule ScoutTauntWoodBat
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsWoodBat
		response ScoutTauntGrandSlam
	}
	rule ScoutTauntAtomizer
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsAtomizer
		response ScoutTauntGrandSlam
	}
	rule ScoutTauntScattergunDouble
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsScattergunDouble
		response ScoutTauntPrimaryDouble
	}
	rule ScoutTauntScattergunDoubleFestive
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsScattergunDoubleFestive
		response ScoutTauntPrimaryDouble
	}
	rule ScoutTauntSodaPopper
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsSodaPopper
		response ScoutTauntPrimaryDouble
	}
	rule ScoutTauntShortstop
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsShortstop
		response ScoutTauntSecondary
	}
	rule ScoutTauntPEPBrawlerBlaster
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsPEPBrawlerBlaster
		response ScoutTauntPrimaryDouble
	}
	rule ScoutTauntHandgunScoutSecondary
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsHandgunScoutSecondary
		response ScoutTauntSecondary
	}
	rule ScoutTauntMadMilk
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsMadMilk
		response ScoutTauntSecondary
	}
	rule ScoutTauntSDCleaver
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsSDCleaver
		response ScoutTauntSecondary
	}
	rule ScoutTauntHolyMackerel
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsHolyMackerel
		response ScoutTauntMelee
	}
	rule ScoutTauntFestiveHolyMackerel
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsFestiveHolyMackerel
		response ScoutTauntMelee
	}

	rule ScoutTauntUnarmedCombat
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsUnarmedCombat
		response ScoutTauntMelee
	}
	rule ScoutTauntHalloween
	{
		criteria ConceptPlayerTaunt IsScout IsHalloweenTaunt
		response ScoutTauntHalloween
	}
	
	Response PlayerExpressionIdleScout
	{
		scene "scenes/player/scout/low/idleloop01.vcd"
	}
	Rule PlayerExpressionIdleScout
	{
		criteria ConceptPlayerExpression IsScout
		Response PlayerExpressionIdleScout
	}

	Response PlayerExpressionIdleCompWinnerScout
	{
		scene "scenes/player/scout/low/comp_winner_idle_face.vcd"
	}
	Rule PlayerExpressionIdleCompWinnerScout
	{
		criteria ConceptPlayerExpression IsScout IsCompWinner
		Response PlayerExpressionIdleCompWinnerScout
	}
	
	Response PlayerExpressionIdleHurtScout
	{
		scene "scenes/player/scout/low/idleloopHurt01.vcd"
	}
	Rule PlayerExpressionIdleHurtScout
	{
		criteria ConceptPlayerExpression IsScout LowHealthContext
		Response PlayerExpressionIdleHurtScout
	}
	Rule PlayerExpressionRoundLossScout
	{
		criteria ConceptPlayerExpression IsScout GameRulesInWinState PlayerOnLosingTeam
		Response PlayerExpressionIdleHurtScout
	}
	
	Response PlayerExpressionAttackScout
	{
		scene "scenes/player/scout/low/attack01.vcd"
	}
	Rule PlayerExpressionAttackScout
	{
		criteria ConceptFireWeapon IsScout
		ApplyContext "ScoutFired:1:7" // Apply the ScoutFired context, to allow Scouts to say double jump lines
		Response PlayerExpressionAttackScout
	}
	rule ScoutTauntMutatedMilk
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsMutatedMilk
		response ScoutTauntSecondary
	}
	//--------------------------------------------------------------------------------------------------------------
	// Auto Speech Drink
	//--------------------------------------------------------------------------------------------------------------
	Response ScoutPostDrinkTired
	{
		scene "scenes/player/scout/low/dodgetired.vcd"

	
	}
	Rule ScoutPostDrinkTired
	{
		criteria ConceptTired IsScout
		Response ScoutPostDrinkTired
	}

	Rule ScoutTauntWrapAssassin
	{
		criteria ConceptPlayerTaunt IsScout WeaponIsWrapAssassin
		response ScoutTauntMelee
	}

	
//============================================================================================================


// Sniper
//------------------------------------------------------------------------------------------------------------
	response "SniperTauntPrimary"
	{
		scene "scenes/player/sniper/low/taunt01_v1.vcd"
		scene "scenes/player/sniper/low/taunt01_v2.vcd"
		scene "scenes/player/sniper/low/taunt01_v3.vcd"
		scene "scenes/player/sniper/low/taunt01_v4.vcd"
		scene "scenes/player/sniper/low/taunt01_v5.vcd"
	}
	response "SniperTauntSecondary"
	{
		scene "scenes/player/sniper/low/taunt02_v1.vcd"
		scene "scenes/player/sniper/low/taunt02_v2.vcd"
	}
	response "SniperTauntMelee"
	{
		scene "scenes/player/sniper/low/taunt03_v1.vcd"
		scene "scenes/player/sniper/low/taunt03_v2.vcd"
		scene "scenes/player/sniper/low/taunt03_v3.vcd"
		scene "scenes/player/sniper/low/taunt03_v4.vcd"		
	}
	response "SniperTauntSaxxy"
	{
		scene "scenes/player/sniper/low/taunt03_v1.vcd"
		scene "scenes/player/sniper/low/taunt03_v3.vcd"		
	}
	response "SniperTauntBow"
	{		
		scene "scenes/player/sniper/low/taunt04.vcd"
	}
	response "SniperTauntHalloween"
	{		
		scene "scenes/player/sniper/low/taunt06.vcd"
	}

	rule SniperTauntPlayerPrimary
	{
		criteria ConceptPlayerTaunt IsSniper WeaponIsSniperrifle
		response SniperTauntPrimary
	}
	rule SniperTauntPlayerPrimaryClassic
	{
		criteria ConceptPlayerTaunt IsSniper WeaponIsClassicSniperrifle
		response SniperTauntPrimary
	}
	rule SniperTauntPlayerSecondary
	{
		criteria ConceptPlayerTaunt IsSniper WeaponIsSMG
		response SniperTauntSecondary
	}
	rule SniperTauntPlayerChargedSecondary
	{
		criteria ConceptPlayerTaunt IsSniper WeaponIsChargedSMG
		response SniperTauntSecondary
	}
	rule SniperTauntPlayerMelee
	{
		criteria ConceptPlayerTaunt IsSniper WeaponIsClub
		response SniperTauntMelee
	}
	rule SniperTauntPlayerBazaarBargain
	{
		criteria ConceptPlayerTaunt IsSniper WeaponIsBazaarBargain
		response SniperTauntPrimary
	}
	rule SniperTauntPlayerBow
	{
		criteria ConceptPlayerTaunt IsSniper WeaponIsBow
		response SniperTauntBow
	}
	rule SniperTauntHalloween
	{
		criteria ConceptPlayerTaunt IsSniper IsHalloweenTaunt
		response SniperTauntHalloween
	}
	rule SniperTauntSaxxy
	{
		criteria ConceptPlayerTaunt IsSniper WeaponIsSaxxy
		response SniperTauntSaxxy
	}
	
	Response PlayerExpressionIdleSniper
	{
		scene "scenes/player/sniper/low/idleloop01.vcd"
	}
	Rule PlayerExpressionIdleSniper
	{
		criteria ConceptPlayerExpression IsSniper
		Response PlayerExpressionIdleSniper
	}
	Response PlayerExpressionIdleCompWinnerSniper
	{
		scene "scenes/player/sniper/low/comp_winner_idle_face.vcd"
	}
	Rule PlayerExpressionIdleCompWinnerSniper
	{
		criteria ConceptPlayerExpression IsSniper IsCompWinner
		Response PlayerExpressionIdleCompWinnerSniper
	}
	response PlayerExpressionIdleDeployedSniper
	{
		scene "scenes/player/sniper/low/idleloopDeployed01.vcd"
	}
	rule PlayerExpressionIdleDeployedSniper
	{
		criteria ConceptPlayerExpression IsSniper DeployedContext WeaponIsSniperrifle
		response PlayerExpressionIdleDeployedSniper
	}
	rule PlayerExpressionIdleDeployedSniperClassic
	{
		criteria ConceptPlayerExpression IsSniper DeployedContext WeaponIsClassicSniperrifle
		response PlayerExpressionIdleDeployedSniper
	}
	Response PlayerExpressionIdleHurtSniper
	{
		scene "scenes/player/sniper/low/idleloopHurt01.vcd"
	}
	Rule PlayerExpressionIdleHurtSniper
	{
		criteria ConceptPlayerExpression IsSniper LowHealthContext
		Response PlayerExpressionIdleHurtSniper
	}
	Rule PlayerExpressionRoundLossSniper
	{
		criteria ConceptPlayerExpression IsSniper GameRulesInWinState PlayerOnLosingTeam
		Response PlayerExpressionIdleHurtSniper
	}

	Response PlayerExpressionAttackSniper
	{
		scene "scenes/player/sniper/low/attack01.vcd"
	}
	Rule PlayerExpressionAttackSniper
	{
		criteria ConceptFireWeapon IsSniper
		Response PlayerExpressionAttackSniper
	}
	

//============================================================================================================


// Spy
//------------------------------------------------------------------------------------------------------------
	response "SpyTauntPrimary"
	{
		scene "scenes/player/spy/low/taunt01_v1.vcd"
		scene "scenes/player/spy/low/taunt01_v2.vcd"
		scene "scenes/player/spy/low/taunt01_v3.vcd"
		scene "scenes/player/spy/low/taunt01_v4.vcd"
		scene "scenes/player/spy/low/taunt01_v5.vcd"
	}
	response "SpyTauntSecondary"
	{
		scene "scenes/player/spy/low/taunt02.vcd"
	}
	response "SpyTauntMelee"
	{
		scene "scenes/player/spy/low/taunt03_v1.vcd"
		scene "scenes/player/spy/low/taunt03_v2.vcd"
	}
	response "SpyTauntPDA"
	{
		scene "scenes/player/spy/low/taunt04_v1.vcd"
		scene "scenes/player/spy/low/taunt04_v2.vcd"
		scene "scenes/player/spy/low/taunt04_v3.vcd"
		scene "scenes/player/spy/low/taunt04_v4.vcd"
		scene "scenes/player/spy/low/taunt04_v5.vcd"
		scene "scenes/player/spy/low/taunt05.vcd"
	}
	response "SpyTauntHalloween"
	{
		scene "scenes/player/spy/low/taunt06.vcd"
	}

	rule SpyTauntPlayerPrimary
	{
		criteria ConceptPlayerTaunt IsSpy WeaponIsRevolver
		response SpyTauntPrimary
	}
	rule SpyTauntPlayerSecondary
	{
		criteria ConceptPlayerTaunt IsSpy WeaponIsBuild
		response SpyTauntSecondary
	}
	rule SpyTauntPlayerMelee
	{
		criteria ConceptPlayerTaunt IsSpy WeaponIsKnife
		response SpyTauntMelee
	}
	rule SpyTauntPlayerPDA
	{
		criteria ConceptPlayerTaunt IsSpy WeaponIsSpyPDA
		response SpyTauntPDA
	}
	rule SpyTauntHalloween
	{
		criteria ConceptPlayerTaunt IsSpy IsHalloweenTaunt
		response SpyTauntHalloween
	}
	rule SpyTauntSharpDresser
	{
		criteria ConceptPlayerTaunt IsSpy WeaponIsSharpDresser
		response SpyTauntSecondary
	}

	Response PlayerExpressionIdleSpy
	{
		scene "scenes/player/spy/low/idleloop01.vcd"
	}
	Rule PlayerExpressionIdleSpy
	{
		criteria ConceptPlayerExpression IsSpy
		Response PlayerExpressionIdleSpy
	}

	Response PlayerExpressionIdleCompWinnerSpy
	{
		scene "scenes/player/spy/low/comp_winner_idle_face.vcd"
	}
	Rule PlayerExpressionIdleCompWinnerSpy
	{
		criteria ConceptPlayerExpression IsSpy IsCompWinner
		Response PlayerExpressionIdleCompWinnerSpy
	}

	Response PlayerExpressionAttackSpy
	{
		scene "scenes/player/spy/low/attack01.vcd"
	}
	Rule PlayerExpressionAttackSpy
	{
		criteria ConceptFireWeapon IsSpy
		Response PlayerExpressionAttackSpy
	}
	
	Response "SpyTauntSpycicle"
	{
		scene "scenes/player/spy/low/taunt01_v1.vcd"
		scene "scenes/player/spy/low/taunt01_v2.vcd"
		scene "scenes/player/spy/low/taunt01_v3.vcd"
		scene "scenes/player/spy/low/taunt01_v4.vcd"
		scene "scenes/player/spy/low/taunt01_v5.vcd"
	}
	Rule SpyTauntSpycicle
	{
		criteria ConceptPlayerTaunt IsSpy WeaponIsSpycicle
		response SpyTauntSpycicle
	}
	
//============================================================================================================


// Demoman
//------------------------------------------------------------------------------------------------------------
	response "DemomanTauntPrimary"
	{
		scene "scenes/player/demoman/low/taunt01.vcd"
	}
	response "DemomanTauntSecondary"
	{
		scene "scenes/player/demoman/low/taunt02.vcd"
		
	}
	response "DemomanTauntMelee"
	{
		scene "scenes/player/demoman/low/taunt03_v1.vcd"
		scene "scenes/player/demoman/low/taunt03_v2.vcd"
		scene "scenes/player/demoman/low/taunt03_v3.vcd"
	}
	response "DemomanTauntHalloween"
	{
		scene "scenes/player/demoman/low/taunt06.vcd"
	}
	response "DemomanTauntWolfHowl"
	{
		scene "scenes/player/demoman/low/taunt11.vcd"
	}
	response "DemomanTauntDefender"
	{
		scene "scenes/player/demoman/low/taunt08.vcd"
	}
	response "DemomanTauntSword"
	{
		scene "scenes/player/demoman/low/taunt09.vcd"
	}
	// Custom stuff
	response "DemomanTauntCaber"
	{
		scene "scenes/player/demoman/low/taunt04_v1.vcd" 
		scene "scenes/player/demoman/low/taunt04_v2.vcd" 
	}
	// Ullapool Caber taunt - from unused Bottle taunt.

	rule DemomanTauntDefender
	{
		criteria ConceptPlayerTaunt IsDemoman WeaponIsDefender
		response DemomanTauntDefender
	}
	rule DemomanTauntPlayerPrimary
	{
		criteria ConceptPlayerTaunt IsDemoman WeaponIsPipebomb
		response DemomanTauntPrimary
	}
	rule DemomanTauntPlayerSecondary
	{
		criteria ConceptPlayerTaunt IsDemoman WeaponIsGrenade
		response DemomanTauntSecondary
	}
	rule DemomanTauntPlayerMelee
	{
		criteria ConceptPlayerTaunt IsDemoman WeaponIsBottle
		ApplyContext "NotSober:1:10" // Apply the drunk context here
		response DemomanTauntMelee
	}
	rule DemomanTauntPlayerScotsmansSkullcutter
	{
		criteria ConceptPlayerTaunt IsDemoman WeaponIsScotsmansSkullcutter
		response DemomanTauntSecondary
	}
	rule DemomanTauntHalloween
	{
		criteria ConceptPlayerTaunt IsDemoman IsHalloweenTaunt
		response DemomanTauntHalloween
	}
	rule DemomanTauntWolfHowl
	{
		criteria ConceptPlayerTaunt IsDemoman IsDemowolf
		response DemomanTauntWolfHowl
	}

	rule DemomanTauntSword
	{
		criteria ConceptPlayerTaunt IsDemoman WeaponIsSword WeaponIsNotAxe // Modified to exclude Skullcutter.
		response DemomanTauntSword
	}
	rule DemomanTauntKatana
	{
		criteria ConceptPlayerTaunt IsDemoman WeaponIsKatana
		response DemomanTauntSword
	}
	// Custom Stuff
	rule DemomanTauntCaber
	{
		criteria ConceptPlayerTaunt IsDemoman WeaponIsCaber
		response DemomanTauntCaber
	}
	Rule DemomanTauntSaxxy
	{
		criteria ConceptPlayerTaunt IsDemoman WeaponIsSaxxy
		Response DemomanTauntSecondary
	}
	Rule DemomanTauntPainTrain
	{
		criteria ConceptPlayerTaunt IsDemoman WeaponIsPainTrain
		Response DemomanTauntSecondary
	}

	Response PlayerExpressionIdleDemoman
	{
		scene "scenes/player/demoman/low/idleloop01.vcd"
	}
	Rule PlayerExpressionIdleDemoman
	{
		criteria ConceptPlayerExpression IsDemoman
		Response PlayerExpressionIdleDemoman
	}

	Response PlayerExpressionIdleCompWinnerDemoman
	{
		scene "scenes/player/demoman/low/comp_winner_idle_face.vcd"
	}
	Rule PlayerExpressionIdleCompWinnerDemoman
	{
		criteria ConceptPlayerExpression IsDemoman IsCompWinner
		Response PlayerExpressionIdleCompWinnerDemoman
	}
	
	Response PlayerExpressionIdleHurtDemoman
	{
		scene "scenes/player/demoman/low/idleloopHurt01.vcd"
	}
	Rule PlayerExpressionIdleHurtDemoman
	{
		criteria ConceptPlayerExpression IsDemoman LowHealthContext
		Response PlayerExpressionIdleHurtDemoman
	}
	Rule PlayerExpressionRoundLossDemoman
	{
		criteria ConceptPlayerExpression IsDemoman GameRulesInWinState PlayerOnLosingTeam
		Response PlayerExpressionIdleHurtDemoman
	}
	
	Response PlayerExpressionAttackDemoman
	{
		scene "scenes/player/demoman/low/attack01.vcd"
	}
	Rule PlayerExpressionAttackDemoman
	{
		criteria ConceptFireWeapon IsDemoman
		Response PlayerExpressionAttackDemoman
	}
	rule DemomanTauntLooseCannon
	{
		criteria ConceptPlayerTaunt IsDemoman WeaponIsLooseCannon
		response DemomanTauntSecondary
	}
	
//============================================================================================================
//	Taunt2 Responses based on items that are in the Action slot

	Response PlayerTaunt2_TestTauntEnabler_IsScout
	{
		scene "scenes/player/pyro/low/taunt_highfiveSuccess.vcd"
	}
	Rule PlayerTaunt2_TestTauntEnabler_IsScout
	{
		criteria ConceptPlayerTaunt2 HasTaunt2Item_TauntEnablerTest IsScout
		response PlayerTaunt2_TestTauntEnabler_IsScout
	}
	Response PlayerTaunt2_TestTauntEnabler_IsSniper
	{
		scene "scenes/player/pyro/low/taunt_highfiveSuccess.vcd"
	}
	Rule PlayerTaunt2_TestTauntEnabler_IsSniper
	{
		criteria ConceptPlayerTaunt2 HasTaunt2Item_TauntEnablerTest IsSniper
		response PlayerTaunt2_TestTauntEnabler_IsSniper
	}
	Response PlayerTaunt2_TestTauntEnabler_IsSoldier
	{
		scene "scenes/player/pyro/low/taunt_highfiveSuccess.vcd"
	}
	Rule PlayerTaunt2_TestTauntEnabler_IsSoldier
	{
		criteria ConceptPlayerTaunt2 HasTaunt2Item_TauntEnablerTest IsSoldier
		response PlayerTaunt2_TestTauntEnabler_IsSoldier
	}
	Response PlayerTaunt2_TestTauntEnabler_IsDemoman
	{
		scene "scenes/player/pyro/low/taunt_highfiveSuccess.vcd"
	}
	Rule PlayerTaunt2_TestTauntEnabler_IsDemoman
	{
		criteria ConceptPlayerTaunt2 HasTaunt2Item_TauntEnablerTest IsDemoman
		response PlayerTaunt2_TestTauntEnabler_IsDemoman
	}
	Response PlayerTaunt2_TestTauntEnabler_IsMedic
	{
		scene "scenes/player/pyro/low/taunt_highfiveSuccess.vcd"
	}
	Rule PlayerTaunt2_TestTauntEnabler_IsMedic
	{
		criteria ConceptPlayerTaunt2 HasTaunt2Item_TauntEnablerTest IsMedic
		response PlayerTaunt2_TestTauntEnabler_IsMedic
	}
	Response PlayerTaunt2_TestTauntEnabler_IsHeavy
	{
		scene "scenes/player/pyro/low/taunt_highfiveSuccess.vcd"
	}
	Rule PlayerTaunt2_TestTauntEnabler_IsHeavy
	{
		criteria ConceptPlayerTaunt2 HasTaunt2Item_TauntEnablerTest IsHeavy
		response PlayerTaunt2_TestTauntEnabler_IsHeavy
	}
	Response PlayerTaunt2_TestTauntEnabler_IsPyro
	{
		scene "scenes/player/pyro/low/taunt_highfiveSuccess.vcd"
	}
	Rule PlayerTaunt2_TestTauntEnabler_IsPyro
	{
		criteria ConceptPlayerTaunt2 HasTaunt2Item_TauntEnablerTest IsPyro
		response PlayerTaunt2_TestTauntEnabler_IsPyro
	}
	Response PlayerTaunt2_TestTauntEnabler_IsSpy
	{
		scene "scenes/player/pyro/low/taunt_highfiveSuccess.vcd"
	}
	Rule PlayerTaunt2_TestTauntEnabler_IsSpy
	{
		criteria ConceptPlayerTaunt2 HasTaunt2Item_TauntEnablerTest IsSpy
		response PlayerTaunt2_TestTauntEnabler_IsSpy
	}
	Response PlayerTaunt2_TestTauntEnabler_IsEngineer
	{
		scene "scenes/player/pyro/low/taunt_highfiveSuccess.vcd"
	}
	Rule PlayerTaunt2_TestTauntEnabler_IsEngineer
	{
		criteria ConceptPlayerTaunt2 HasTaunt2Item_TauntEnablerTest IsEngineer
		response PlayerTaunt2_TestTauntEnabler_IsEngineer
	}

//============================================================================================================
//	"Show off an Item" Responses 

	Response ScoutShowItemTauntResponse
	{
		scene "scenes/player/pyro/low/taunt_highfiveSuccess.vcd"
	}
	Rule ScoutShowItemTaunt
	{
		criteria ConceptPlayerShowItemTaunt IsScout
		response ScoutShowItemTauntResponse
	}
	
	Response SniperShowItemTauntResponse
	{
		scene "scenes/player/pyro/low/taunt_highfiveSuccess.vcd"
	}
	Rule SniperShowItemTaunt
	{
		criteria ConceptPlayerShowItemTaunt IsSniper
		response SniperShowItemTauntResponse
	}

	Response SoldierShowItemTauntResponse
	{
		scene "scenes/player/pyro/low/taunt_highfiveSuccess.vcd"
	}
	Rule SoldierShowItemTaunt
	{
		criteria ConceptPlayerShowItemTaunt IsSoldier
		response SoldierShowItemTauntResponse
	}

	Response DemomanShowItemTauntResponse
	{
		scene "scenes/player/pyro/low/taunt_highfiveSuccess.vcd"
	}
	Rule DemomanShowItemTaunt
	{
		criteria ConceptPlayerShowItemTaunt IsDemoman
		response DemomanShowItemTauntResponse
	}
	
	Response MedicShowItemTauntResponse
	{
		scene "scenes/player/pyro/low/taunt_highfiveSuccess.vcd"
	}
	Rule MedicShowItemTaunt
	{
		criteria ConceptPlayerShowItemTaunt IsMedic
		response MedicShowItemTauntResponse
	}
	
	Response HeavyShowItemTauntResponse
	{
		scene "scenes/player/heavy/high/show1.vcd"
	}
	Rule HeavyShowItemTaunt
	{
		criteria ConceptPlayerShowItemTaunt IsHeavy
		response HeavyShowItemTauntResponse
	}
	
	Response PyroShowItemTauntResponse
	{
		scene "scenes/player/pyro/low/taunt_highfiveSuccess.vcd"
	}
	Rule PyroShowItemTaunt
	{
		criteria ConceptPlayerShowItemTaunt IsPyro
		response PyroShowItemTauntResponse
	}

	Response SpyShowItemTauntResponse
	{
		scene "scenes/player/pyro/low/taunt_highfiveSuccess.vcd"
	}
	Rule SpyShowItemTaunt
	{
		criteria ConceptPlayerShowItemTaunt IsSpy
		response SpyShowItemTauntResponse
	}

	Response ScoutShowItemTauntResponse
	{
		scene "scenes/player/pyro/low/taunt_highfiveSuccess.vcd"
	}
	Rule ScoutShowItemTaunt
	{
		criteria ConceptPlayerShowItemTaunt IsScout
		response ScoutShowItemTauntResponse
	}

//============================================================================================================
//	"Replay Taunt" Responses 

	response "PyroTauntReplay"
	{
		scene "scenes/player/pyro/low/taunt_replay.vcd"
		scene "scenes/player/pyro/low/taunt_replay2.vcd"
	}
	rule PyroTauntReplay
	{
		criteria ConceptTauntReplay IsPyro
		response PyroTauntReplay
	}

	response "ScoutTauntReplay"
	{
		scene "scenes/player/scout/low/taunt_replay.vcd"
	}
	rule ScoutTauntReplay
	{
		criteria ConceptTauntReplay IsScout
		response ScoutTauntReplay
	}

	response "SoldierTauntReplay"
	{
		scene "scenes/player/soldier/low/taunt_replay.vcd"
	}
	rule SoldierTauntReplay
	{
		criteria ConceptTauntReplay IsSoldier
		response SoldierTauntReplay
	}

	response "SniperTauntReplay"
	{
		scene "scenes/player/sniper/low/taunt_replay.vcd"
	}
	rule SniperTauntReplay
	{
		criteria ConceptTauntReplay IsSniper
		response SniperTauntReplay
	}
	
	response "DemomanTauntReplay"
	{
		scene "scenes/player/demoman/low/taunt_replay.vcd"
	}
	rule DemomanTauntReplay
	{
		criteria ConceptTauntReplay IsDemoman
		response DemomanTauntReplay
	}
	
	response "MedicTauntReplay"
	{
		scene "scenes/player/medic/low/taunt_replay.vcd"
	}
	rule MedicTauntReplay
	{
		criteria ConceptTauntReplay IsMedic
		response MedicTauntReplay
	}
	
	response "HeavyTauntReplay"
	{
		scene "scenes/player/heavy/low/taunt_replay.vcd"
	}
	rule HeavyTauntReplay
	{
		criteria ConceptTauntReplay IsHeavy
		response HeavyTauntReplay
	}

	response "SpyTauntReplay"
	{
		scene "scenes/player/spy/low/taunt_replay.vcd"
	}
	rule SpyTauntReplay
	{
		criteria ConceptTauntReplay IsSpy
		response SpyTauntReplay
	}

	response "EngineerTauntReplay"
	{
		scene "scenes/player/engineer/low/taunt_replay.vcd"
	}
	rule EngineerTauntReplay
	{
		criteria ConceptTauntReplay IsEngineer
		response EngineerTauntReplay
	}

	response "MedicTauntLaugh"
	{
		scene "scenes/player/medic/low/taunt_laugh.vcd"
	}
	rule MedicTauntLaugh
	{
		criteria ConceptTauntLaugh IsMedic
		response MedicTauntLaugh
	}
	
	response "HeavyTauntLaugh"
	{
		scene "scenes/player/heavy/low/taunt_laugh.vcd"
	}
	rule HeavyTauntLaugh
	{
		criteria ConceptTauntLaugh IsHeavy
		response HeavyTauntLaugh
	}
	
	response "DemomanTauntLaugh"
	{
		scene "scenes/player/demoman/low/taunt_laugh.vcd"
	}
	rule DemomanTauntLaugh
	{
		criteria ConceptTauntLaugh IsDemoman
		response DemomanTauntLaugh
	}
	
	response "SpyTauntLaugh"
	{
		scene "scenes/player/spy/low/taunt_laugh.vcd"
	}
	rule SpyTauntLaugh
	{
		criteria ConceptTauntLaugh IsSpy
		response SpyTauntLaugh
	}
	
	response "SniperTauntLaugh"
	{
		scene "scenes/player/Sniper/low/taunt_laugh.vcd"
	}
	rule SniperTauntLaugh
	{
		criteria ConceptTauntLaugh IsSniper
		response SniperTauntLaugh
	}
		
	response "SoldierTauntLaugh"
	{
		scene "scenes/player/Soldier/low/taunt_laugh.vcd"
	}
	rule SoldierTauntLaugh
	{
		criteria ConceptTauntLaugh IsSoldier
		response SoldierTauntLaugh
	}
	
	response "ScoutTauntLaugh"
	{
		scene "scenes/player/Scout/low/taunt_laugh.vcd"
	}
	rule ScoutTauntLaugh
	{
		criteria ConceptTauntLaugh IsScout
		response ScoutTauntLaugh
	}
	
	response "EngineerTauntLaugh"
	{
		scene "scenes/player/Engineer/low/taunt_laugh.vcd"
	}
	rule EngineerTauntLaugh
	{
		criteria ConceptTauntLaugh IsEngineer
		response EngineerTauntLaugh
	}
	
	response "PyroTauntLaugh"
	{
		scene "scenes/player/pyro/low/taunt_laugh.vcd"
	}
	rule PyroTauntLaugh
	{
		criteria ConceptTauntLaugh IsPyro
		response PyroTauntLaugh
	}
	
	response "MedicTauntHeroicPose"
	{
		scene "scenes/player/medic/low/taunt09.vcd"
	}
	rule MedicTauntHeroicPose
	{
		criteria ConceptTauntHeroicPose IsMedic
		response MedicTauntHeroicPose
	}
	
	response "PyroTauntArmageddon"
	{
		scene "scenes/player/pyro/low/taunt_laugh.vcd"
	}
	rule PyroTauntArmageddon
	{
		criteria ConceptTauntPyroArmageddon IsPyro
		response PyroTauntArmageddon
	}

	response "ScoutTauntGuitarRiff"
	{
		scene "scenes/player/scout/low/taunt_brutalLegend.vcd"
	}
	rule ScoutTauntGuitarRiff
	{
		criteria ConceptTauntGuitarRiff IsScout
		response ScoutTauntGuitarRiff
	}

	response "SoldierTauntGuitarRiff"
	{
		scene "scenes/player/soldier/low/taunt_brutalLegend.vcd"
	}
	rule SoldierTauntGuitarRiff
	{
		criteria ConceptTauntGuitarRiff IsSoldier
		response SoldierTauntGuitarRiff
	}

	response "PyroTauntGuitarRiff"
	{
		scene "scenes/player/pyro/low/taunt_brutalLegend.vcd"
	}
	rule PyroTauntGuitarRiff
	{
		criteria ConceptTauntGuitarRiff IsPyro
		response PyroTauntGuitarRiff
	}

	response "DemomanTauntGuitarRiff"
	{
		scene "scenes/player/demoman/low/taunt_brutalLegend.vcd"
	}
	rule DemomanTauntGuitarRiff
	{
		criteria ConceptTauntGuitarRiff IsDemoman
		response DemomanTauntGuitarRiff
	}
	
	response "HeavyTauntGuitarRiff"
	{
		scene "scenes/player/heavy/low/taunt_brutalLegend.vcd"
	}
	rule HeavyTauntGuitarRiff
	{
		criteria ConceptTauntGuitarRiff IsHeavy
		response HeavyTauntGuitarRiff
	}

	response "EngineerTauntGuitarRiff"
	{
		scene "scenes/player/engineer/low/taunt_brutalLegend.vcd"
	}
	rule EngineerTauntGuitarRiff
	{
		criteria ConceptTauntGuitarRiff IsEngineer
		response EngineerTauntGuitarRiff
	}

	response "MedicTauntGuitarRiff"
	{
		scene "scenes/player/medic/low/taunt_brutalLegend.vcd"
	}
	rule MedicTauntGuitarRiff
	{
		criteria ConceptTauntGuitarRiff IsMedic
		response MedicTauntGuitarRiff
	}

	response "SniperTauntGuitarRiff"
	{
		scene "scenes/player/sniper/low/taunt_brutalLegend.vcd"
	}
	rule SniperTauntGuitarRiff
	{
		criteria ConceptTauntGuitarRiff IsSniper
		response SniperTauntGuitarRiff
	}

	response "SpyTauntGuitarRiff"
	{
		scene "scenes/player/spy/low/taunt_brutalLegend.vcd"
	}
	rule SpyTauntGuitarRiff
	{
		criteria ConceptTauntGuitarRiff IsSpy
		response SpyTauntGuitarRiff
	}


// APRIL FOOLS RESPONSE RULES

	rule ScoutTauntAprilFools
	{
		criteria ConceptPlayerTaunt IsAprilFoolsTaunt IsScout
		response ScoutTauntLaugh
	}

	rule SoldierTauntAprilFools
	{
		criteria ConceptPlayerTaunt IsAprilFoolsTaunt IsSoldier
		response SoldierTauntLaugh
	}

	rule PyroTauntAprilFools
	{
		criteria ConceptPlayerTaunt IsAprilFoolsTaunt IsPyro
		response PyroTauntLaugh
	}

	rule DemomanTauntAprilFools
	{
		criteria ConceptPlayerTaunt IsAprilFoolsTaunt IsDemoman
		response DemomanTauntLaugh
	}

	rule HeavyTauntAprilFools
	{
		criteria ConceptPlayerTaunt IsAprilFoolsTaunt IsHeavy
		response HeavyTauntLaugh
	}

	rule EngineerTauntAprilFools
	{
		criteria ConceptPlayerTaunt IsAprilFoolsTaunt IsEngineer
		response EngineerTauntLaugh
	}

	rule MedicTauntAprilFools
	{
		criteria ConceptPlayerTaunt IsAprilFoolsTaunt IsMedic
		response MedicTauntLaugh
	}

	rule SniperTauntAprilFools
	{
		criteria ConceptPlayerTaunt IsAprilFoolsTaunt IsSniper
		response SniperTauntLaugh
	}

	rule SpyTauntAprilFools
	{
		criteria ConceptPlayerTaunt IsAprilFoolsTaunt IsSpy
		response SpyTauntLaugh
	}

//============================================================================================================
