//------------------------------------------------------------------------------------------------------------------------
//CRITERIA
//------------------------------------------------------------------------------------------------------------------------
Criterion "NotMerasmusHideCooldown" "worldMerasmusHideCooldown" "!=1" "required" weight 100
Criterion "IsMerasmusHiding"	"IsMerasmusHiding" "1" "required" weight 100
Criterion	"IsZombieCostume"		"IsZombieCostume"	"1" "required" weight 100
Criterion "soldierNotZombieNoises" "soldierZombieNoises" "!=1" "required" weight 100
Criterion "scoutNotZombieNoises" "scoutZombieNoises" "!=1" "required" weight 100
criterion "ConceptMagicBigHead" "Concept" "TLK_MAGIC_BIGHEAD" required
criterion "ConceptMagicSmallHead" "Concept" "TLK_MAGIC_SMALLHEAD" required
criterion "ConceptMagicGravity" "Concept" "TLK_MAGIC_GRAVITY" required
criterion "ConceptMagicGood" "Concept" "TLK_MAGIC_GOOD" required
criterion "ConceptMagicDance" "Concept" "TLK_MAGIC_DANCE" required
criterion "ConceptHalloweenPlayerPitFall" "Concept" "HalloweenLongFall" required
//------------------------------------------------------------------------------------------------------------------------

Response PlayerFairyNoiseHeavy
{
	scene	"scenes/player/heavy/low/4766.vcd"
	scene	"scenes/player/heavy/low/4767.vcd"
	scene	"scenes/player/heavy/low/4768.vcd"
	scene	"scenes/player/heavy/low/4769.vcd"
	scene	"scenes/player/heavy/low/4770.vcd"
	scene	"scenes/player/heavy/low/4771.vcd"
	scene	"scenes/player/heavy/low/4772.vcd"
	scene	"scenes/player/heavy/low/4775.vcd"
	scene	"scenes/player/heavy/low/4776.vcd"
	scene	"scenes/player/heavy/low/4777.vcd"
	scene	"scenes/player/heavy/low/4778.vcd"
	scene	"scenes/player/heavy/low/4779.vcd"
	scene	"scenes/player/heavy/low/4780.vcd"
	scene	"scenes/player/heavy/low/4781.vcd"
	scene	"scenes/player/heavy/low/4782.vcd"
	scene	"scenes/player/heavy/low/4783.vcd"
	scene	"scenes/player/heavy/low/4784.vcd"
}
Rule PlayerFairyNoiseHeavy
{
	criteria ConceptPlayerExpression IsHeavy IsFairyHeavy HeavyNotFairyNoises 50PercentChance
	applycontext "HeavyFairyNoises:1:90"
	Response PlayerFairyNoiseHeavy
}

Response PlayerBattleCryFairyHeavy
{
	scene	"scenes/player/heavy/low/4766.vcd"
	scene	"scenes/player/heavy/low/4767.vcd"
	scene	"scenes/player/heavy/low/4769.vcd"
	scene	"scenes/player/heavy/low/4775.vcd"
}
Rule PlayerBattleCryFairyHeavy
{
	criteria ConceptPlayerBattleCry IsHeavy IsFairyHeavy
	Response PlayerBattleCryFairyHeavy
}

Response KilledPlayerHeavyFairy
{
	scene	"scenes/player/heavy/low/4773.vcd"
	scene	"scenes/player/heavy/low/4774.vcd"
	scene	"scenes/player/heavy/low/4778.vcd"
}
Rule KilledPlayerHeavyFairy
{
	criteria ConceptKilledPlayer IsHeavy IsFairyHeavy HeavyNotKillSpeech 20PercentChance
	applycontext "HeavyKillSpeech:1:15"
	Response KilledPlayerHeavyFairy
}

Response PlayerRoundStartHeavyFairy
{
	scene	"scenes/player/heavy/low/4766.vcd"
	scene	"scenes/player/heavy/low/4767.vcd"
	scene	"scenes/player/heavy/low/4769.vcd"
	scene	"scenes/player/heavy/low/4775.vcd"
}
Rule PlayerRoundStartHeavyFairy
{
	criteria ConceptPlayerRoundStart IsHeavy IsFairyHeavy
	applycontext "HeavyFairyNoises:1:30"
	Response PlayerRoundStartHeavyFairy
}

Response scoutMerasmusHideTaunt
{
	scene	"scenes/player/scout/low/4478.vcd"
	scene	"scenes/player/scout/low/4482.vcd"
	scene	"scenes/player/scout/low/4487.vcd"
	scene	"scenes/player/scout/low/4489.vcd"
	scene	"scenes/player/scout/low/4491.vcd"
	scene	"scenes/player/scout/low/4492.vcd"
	scene	"scenes/player/scout/low/4493.vcd"
	scene	"scenes/player/scout/low/4494.vcd"
	scene	"scenes/player/scout/low/4496.vcd"
	scene	"scenes/player/scout/low/4497.vcd"
	scene	"scenes/player/scout/low/4498.vcd"
	scene	"scenes/player/scout/low/4695.vcd"
	scene	"scenes/player/scout/low/4698.vcd"
	scene	"scenes/player/scout/low/4699.vcd"
	scene	"scenes/player/scout/low/4700.vcd"
	scene	"scenes/player/scout/low/4701.vcd"
	scene	"scenes/player/scout/low/4702.vcd"
}
Rule scoutMerasmusHideTaunt
{
	criteria ConceptPlayerExpression Isscout IsMerasmusHiding NotMerasmusHideCooldown 20PercentChance
	applycontext "MerasmusHideCooldown:1:30"
	applycontexttoworld
	Response scoutMerasmusHideTaunt
}

Response heavyMerasmusHideTaunt
{
	scene	"scenes/player/heavy/low/4737.vcd"
	scene	"scenes/player/heavy/low/4738.vcd"
	scene	"scenes/player/heavy/low/4739.vcd"
	scene	"scenes/player/heavy/low/4741.vcd"
	scene	"scenes/player/heavy/low/4742.vcd"
}
Rule heavyMerasmusHideTaunt
{
	criteria ConceptPlayerExpression Isheavy IsMerasmusHiding NotMerasmusHideCooldown 20PercentChance
	applycontext "MerasmusHideCooldown:1:30"
	applycontexttoworld
	Response heavyMerasmusHideTaunt
}

Response demomanMerasmusHideTaunt
{
	scene	"scenes/player/demoman/low/4575.vcd"
	scene	"scenes/player/demoman/low/4576.vcd"
	scene	"scenes/player/demoman/low/4577.vcd"
	scene	"scenes/player/demoman/low/4578.vcd"
	scene	"scenes/player/demoman/low/4579.vcd"
	scene	"scenes/player/demoman/low/4580.vcd"
	scene	"scenes/player/demoman/low/4581.vcd"
	scene	"scenes/player/demoman/low/4582.vcd"
	scene	"scenes/player/demoman/low/4583.vcd"
	scene	"scenes/player/demoman/low/5454.vcd"
}
Rule demomanMerasmusHideTaunt
{
	criteria ConceptPlayerExpression Isdemoman IsMerasmusHiding NotMerasmusHideCooldown 20PercentChance
	applycontext "MerasmusHideCooldown:1:30"
	applycontexttoworld
	Response demomanMerasmusHideTaunt
}

Response medicMerasmusHideTaunt
{
	scene	"scenes/player/medic/low/4643.vcd"
	scene	"scenes/player/medic/low/4644.vcd"
	scene	"scenes/player/medic/low/4645.vcd"
	scene	"scenes/player/medic/low/4646.vcd"
	scene	"scenes/player/medic/low/4647.vcd"
}
Rule medicMerasmusHideTaunt
{
	criteria ConceptPlayerExpression Ismedic IsMerasmusHiding NotMerasmusHideCooldown 20PercentChance
	applycontext "MerasmusHideCooldown:1:30"
	applycontexttoworld
	Response medicMerasmusHideTaunt
}

Response soldierMerasmusHideTaunt
{
	scene	"scenes/player/soldier/low/4499.vcd"
	scene	"scenes/player/soldier/low/4500.vcd"
	scene	"scenes/player/soldier/low/4506.vcd"
	scene	"scenes/player/soldier/low/4507.vcd"
	scene	"scenes/player/soldier/low/4509.vcd"
	scene	"scenes/player/soldier/low/4510.vcd"
	scene	"scenes/player/soldier/low/4511.vcd"
	scene	"scenes/player/soldier/low/4512.vcd"
	scene	"scenes/player/soldier/low/4513.vcd"
	scene	"scenes/player/soldier/low/4515.vcd"
	scene	"scenes/player/soldier/low/4516.vcd"
	scene	"scenes/player/soldier/low/4517.vcd"
	scene	"scenes/player/soldier/low/4522.vcd"
	scene	"scenes/player/soldier/low/4523.vcd"
	scene	"scenes/player/soldier/low/4524.vcd"
	scene	"scenes/player/soldier/low/4525.vcd"
	scene	"scenes/player/soldier/low/4526.vcd"
	scene	"scenes/player/soldier/low/4527.vcd"
	scene	"scenes/player/soldier/low/4528.vcd"
	scene	"scenes/player/soldier/low/4529.vcd"
	scene	"scenes/player/soldier/low/4530.vcd"
	scene	"scenes/player/soldier/low/4531.vcd"
	scene	"scenes/player/soldier/low/4533.vcd"
	scene	"scenes/player/soldier/low/4534.vcd"
	scene	"scenes/player/soldier/low/4535.vcd"
	scene	"scenes/player/soldier/low/4536.vcd"
	scene	"scenes/player/soldier/low/4537.vcd"
	scene	"scenes/player/soldier/low/4538.vcd"
	scene	"scenes/player/soldier/low/4539.vcd"
}
Rule soldierMerasmusHideTaunt
{
	criteria ConceptPlayerExpression Issoldier IsMerasmusHiding NotMerasmusHideCooldown 20PercentChance
	applycontext "MerasmusHideCooldown:1:30"
	applycontexttoworld
	Response soldierMerasmusHideTaunt
}

Response scoutHalloweenMerasmusPitFall
{
	scene	"scenes/player/scout/low/4455.vcd"
	scene	"scenes/player/scout/low/4456.vcd"
	scene	"scenes/player/scout/low/4704.vcd"
}
Rule scoutHalloweenMerasmusPitFall
{
	criteria ConceptHalloweenPlayerPitFall Isscout
	Response scoutHalloweenMerasmusPitFall
}

Response heavyHalloweenMerasmusPitFall
{
	scene	"scenes/player/heavy/low/4743.vcd"
}
Rule heavyHalloweenMerasmusPitFall
{
	criteria ConceptHalloweenPlayerPitFall Isheavy
	Response heavyHalloweenMerasmusPitFall
}

Response demomanHalloweenMerasmusPitFall
{
	scene	"scenes/player/demoman/low/4585.vcd"
}
Rule demomanHalloweenMerasmusPitFall
{
	criteria ConceptHalloweenPlayerPitFall Isdemoman
	Response demomanHalloweenMerasmusPitFall
}

Response medicHalloweenMerasmusPitFall
{
	scene	"scenes/player/medic/low/4651.vcd"
}
Rule medicHalloweenMerasmusPitFall
{
	criteria ConceptHalloweenPlayerPitFall Ismedic
	Response medicHalloweenMerasmusPitFall
}

Response spyHalloweenMerasmusPitFall
{
	scene	"scenes/player/spy/low/4705.vcd"
	scene	"scenes/player/spy/low/4709.vcd"
}
Rule spyHalloweenMerasmusPitFall
{
	criteria ConceptHalloweenPlayerPitFall Isspy
	Response spyHalloweenMerasmusPitFall
}

Response soldierHalloweenMerasmusPitFall
{
	scene	"scenes/player/soldier/low/4540.vcd"
	scene	"scenes/player/soldier/low/4541.vcd"
}
Rule soldierHalloweenMerasmusPitFall
{
	criteria ConceptHalloweenPlayerPitFall Issoldier
	Response soldierHalloweenMerasmusPitFall
}

Response scoutZombieCostume
{
	scene	"scenes/player/scout/low/4457.vcd"
	scene	"scenes/player/scout/low/4458.vcd"
	scene	"scenes/player/scout/low/4459.vcd"
	scene	"scenes/player/scout/low/4460.vcd"
	scene	"scenes/player/scout/low/4703.vcd"
}
Rule scoutZombieCostume
{
	criteria ConceptPlayerExpression Isscout IsZombieCostume scoutNotZombieNoises 20PercentChance
	applycontext "scoutZombieNoises:1:30"
	Response scoutZombieCostume
}

Response soldierZombieCostume
{
	scene	"scenes/player/soldier/low/4562.vcd"
	scene	"scenes/player/soldier/low/4563.vcd"
	scene	"scenes/player/soldier/low/4729.vcd"
	scene	"scenes/player/soldier/low/4730.vcd"
}
Rule soldierZombieCostume
{
	criteria ConceptPlayerExpression Issoldier IsZombieCostume soldierNotZombieNoises 20PercentChance
	applycontext "soldierZombieNoises:1:30"
	Response soldierZombieCostume
}

Response scoutPlayerBattleCryZombie
{
	scene	"scenes/player/scout/low/4457.vcd"
	scene	"scenes/player/scout/low/4458.vcd"
	scene	"scenes/player/scout/low/4459.vcd"
	scene	"scenes/player/scout/low/4460.vcd"
	scene	"scenes/player/scout/low/4703.vcd"
}
Rule scoutPlayerBattleCryZombie
{
	criteria ConceptPlayerBattleCry Isscout IsZombieCostume
	applycontext "scoutZombieNoises:1:30"
	Response scoutPlayerBattleCryZombie
}

Response soldierPlayerBattleCryZombie
{
	scene	"scenes/player/soldier/low/4562.vcd"
	scene	"scenes/player/soldier/low/4563.vcd"
	scene	"scenes/player/soldier/low/4729.vcd"
	scene	"scenes/player/soldier/low/4730.vcd"
}
Rule soldierPlayerBattleCryZombie
{
	criteria ConceptPlayerBattleCry Issoldier IsZombieCostume
	applycontext "soldierZombieNoises:1:30"
	Response soldierPlayerBattleCryZombie
}

Response scoutBigHeadReaction
{
	scene	"scenes/player/scout/low/4472.vcd" predelay "3.5, 6.5"
	scene	"scenes/player/scout/low/4686.vcd" predelay "3.5, 6.5"
	scene	"scenes/player/scout/low/4689.vcd" predelay "3.5, 6.5"
	scene	"scenes/player/scout/low/4692.vcd" predelay "3.5, 6.5"
	scene	"scenes/player/scout/low/4693.vcd" predelay "3.5, 6.5"
	scene	"scenes/player/scout/low/4694.vcd" predelay "3.5, 6.5"
}
Rule scoutBigHeadReaction
{
	criteria ConceptMagicBigHead Isscout
	Response scoutBigHeadReaction
}

Response heavyBigHeadReaction
{
	scene	"scenes/player/heavy/low/4744.vcd" predelay "3.5, 6.5"
	scene	"scenes/player/heavy/low/4750.vcd" predelay "3.5, 6.5"
	scene	"scenes/player/heavy/low/5456.vcd" predelay "3.5, 6.5"
}
Rule heavyBigHeadReaction
{
	criteria ConceptMagicBigHead Isheavy
	Response heavyBigHeadReaction
}

Response demomanBigHeadReaction
{
	scene	"scenes/player/demoman/low/4586.vcd" predelay "3.5, 6.5"
	scene	"scenes/player/demoman/low/4589.vcd" predelay "3.5, 6.5"
}
Rule demomanBigHeadReaction
{
	criteria ConceptMagicBigHead Isdemoman
	Response demomanBigHeadReaction
}

Response medicBigHeadReaction
{
	scene	"scenes/player/medic/low/4652.vcd" predelay "3.5, 6.5"
	scene	"scenes/player/medic/low/4653.vcd" predelay "3.5, 6.5"
	scene	"scenes/player/medic/low/4660.vcd" predelay "3.5, 6.5"
}
Rule medicBigHeadReaction
{
	criteria ConceptMagicBigHead Ismedic
	Response medicBigHeadReaction
}

Response spyBigHeadReaction
{
	scene	"scenes/player/spy/low/4711.vcd" predelay "3.5, 6.5"
	scene	"scenes/player/spy/low/4712.vcd" predelay "3.5, 6.5"
}
Rule spyBigHeadReaction
{
	criteria ConceptMagicBigHead Isspy
	Response spyBigHeadReaction
}

Response soldierBigHeadReaction
{
	scene	"scenes/player/soldier/low/4546.vcd" predelay "3.5, 6.5"
	scene	"scenes/player/soldier/low/4547.vcd" predelay "3.5, 6.5"
	scene	"scenes/player/soldier/low/4728.vcd" predelay "3.5, 6.5"
}
Rule soldierBigHeadReaction
{
	criteria ConceptMagicBigHead Issoldier
	Response soldierBigHeadReaction
}

Response scoutSmallHeadReaction
{
	scene	"scenes/player/scout/low/4464.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/scout/low/4468.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/scout/low/4473.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/scout/low/4475.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/scout/low/4687.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/scout/low/4688.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/scout/low/4692.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/scout/low/4693.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/scout/low/4694.vcd" predelay "4.5, 6.5"
}
Rule scoutSmallHeadReaction
{
	criteria ConceptMagicSmallHead Isscout
	Response scoutSmallHeadReaction
}

Response heavySmallHeadReaction
{
	scene	"scenes/player/heavy/low/4745.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/heavy/low/4749.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/heavy/low/4751.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/heavy/low/4750.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/heavy/low/5456.vcd" predelay "4.5, 6.5"
}
Rule heavySmallHeadReaction
{
	criteria ConceptMagicSmallHead Isheavy
	Response heavySmallHeadReaction
}

Response demomanSmallHeadReaction
{
	scene	"scenes/player/demoman/low/4587.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/demoman/low/4588.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/demoman/low/4590.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/demoman/low/4592.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/demoman/low/4586.vcd" predelay "4.5, 6.5"
}
Rule demomanSmallHeadReaction
{
	criteria ConceptMagicSmallHead Isdemoman
	Response demomanSmallHeadReaction
}

Response medicSmallHeadReaction
{
	scene	"scenes/player/medic/low/4654.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/medic/low/4656.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/medic/low/4652.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/medic/low/4660.vcd" predelay "4.5, 6.5"
}
Rule medicSmallHeadReaction
{
	criteria ConceptMagicSmallHead Ismedic
	Response medicSmallHeadReaction
}

Response spySmallHeadReaction
{
	scene	"scenes/player/spy/low/4710.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/spy/low/4713.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/spy/low/4715.vcd" predelay "4.5, 6.5"
}
Rule spySmallHeadReaction
{
	criteria ConceptMagicSmallHead Isspy
	Response spySmallHeadReaction
}

Response soldierSmallHeadReaction
{
	scene	"scenes/player/soldier/low/4548.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4553.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4728.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4546.vcd" predelay "4.5, 6.5"
}
Rule soldierSmallHeadReaction
{
	criteria ConceptMagicSmallHead Issoldier
	Response soldierSmallHeadReaction
}

Response scoutGravityReaction
{
	scene	"scenes/player/scout/low/4469.vcd" predelay "4.5, 6.5"
}
Rule scoutGravityReaction
{
	criteria ConceptMagicGravity Isscout 50PercentChance
	Response scoutGravityReaction
}

Response heavyGravityReaction
{
	scene	"scenes/player/heavy/low/4748.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/heavy/low/4752.vcd" predelay "4.5, 6.5"
}
Rule heavyGravityReaction
{
	criteria ConceptMagicGravity Isheavy 50PercentChance
	Response heavyGravityReaction
}

Response demomanGravityReaction
{
	scene	"scenes/player/demoman/low/4591.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/demoman/low/4593.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/demoman/low/4596.vcd" predelay "4.5, 6.5"
}
Rule demomanGravityReaction
{
	criteria ConceptMagicGravity Isdemoman 50PercentChance
	Response demomanGravityReaction
}

Response medicGravityReaction
{
	scene	"scenes/player/medic/low/4657.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/medic/low/4663.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/medic/low/4656.vcd" predelay "4.5, 6.5"
}
Rule medicGravityReaction
{
	criteria ConceptMagicGravity Ismedic 50PercentChance
	Response medicGravityReaction
}

Response soldierGravityReaction
{
	scene	"scenes/player/soldier/low/4556.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4557.vcd" predelay "4.5, 6.5"
}
Rule soldierGravityReaction
{
	criteria ConceptMagicGravity Issoldier 50PercentChance
	Response soldierGravityReaction
}

Response scoutGoodMagicReaction
{
	scene	"scenes/player/scout/low/4477.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/scout/low/4479.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/scout/low/4480.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/scout/low/4481.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/scout/low/4483.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/scout/low/4482.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/scout/low/4497.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/scout/low/4699.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/scout/low/4702.vcd" predelay "4.5, 6.5"
}
Rule scoutGoodMagicReaction
{
	criteria ConceptMagicGood Isscout 50PercentChance
	Response scoutGoodMagicReaction
}

Response heavyGoodMagicReaction
{
	scene	"scenes/player/heavy/low/4754.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/heavy/low/4755.vcd" predelay "4.5, 6.5"
}
Rule heavyGoodMagicReaction
{
	criteria ConceptMagicGood Isheavy 50PercentChance
	Response heavyGoodMagicReaction
}

Response demomanGoodMagicReaction
{
	scene	"scenes/player/demoman/low/4597.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/demoman/low/4598.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/demoman/low/4599.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/demoman/low/4600.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/demoman/low/4576.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/demoman/low/4577.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/demoman/low/4578.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/demoman/low/4579.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/demoman/low/4580.vcd" predelay "4.5, 6.5"
}
Rule demomanGoodMagicReaction
{
	criteria ConceptMagicGood Isdemoman 50PercentChance
	Response demomanGoodMagicReaction
}

Response medicGoodMagicReaction
{
	scene	"scenes/player/medic/low/4649.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/medic/low/4664.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/medic/low/4665.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/medic/low/4666.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/medic/low/4731.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/medic/low/4732.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/medic/low/4733.vcd" predelay "4.5, 6.5"
}
Rule medicGoodMagicReaction
{
	criteria ConceptMagicGood Ismedic 50PercentChance
	Response medicGoodMagicReaction
}

Response soldierGoodMagicReaction
{
	scene	"scenes/player/soldier/low/4564.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4565.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4566.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4567.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4522.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4523.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4525.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4526.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4527.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4528.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4529.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4530.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4531.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4533.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4534.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4535.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4536.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4537.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4538.vcd" predelay "4.5, 6.5"
	scene	"scenes/player/soldier/low/4539.vcd" predelay "4.5, 6.5"
}
Rule soldierGoodMagicReaction
{
	criteria ConceptMagicGood Issoldier 50PercentChance
	Response soldierGoodMagicReaction
}

Response soldierDanceMagicReaction
{
	scene	"scenes/player/soldier/low/4550.vcd" predelay "3.0"
	scene	"scenes/player/soldier/low/4558.vcd" predelay "3.0"
	scene	"scenes/player/soldier/low/4559.vcd" predelay "3.0"
}
Rule soldierDanceMagicReaction
{
	criteria ConceptMagicDance Issoldier 50PercentChance
	Response soldierDanceMagicReaction
}



//------------------------------------------------------------------------------------------------------------------------
//Scream Fortress 2013
//------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------
//CRITERIA
//------------------------------------------------------------------------------------------------------------------------
Criterion "SF13IsTheWitchingHour" "worldIsTheWitchingHour" "1" "required" weight 100
Criterion "SF13IsNotTheWitchingHour" "worldIsTheWitchingHour" "!=1" "required" weight 100
//------------------------------------------------------------------------------------------------------------------------
//START OF SCREAM FORTRESS 2013 AUTOGENERATED
Response ScoutCastMerasmusZap
{
	scene	"scenes/player/scout/low/6254.vcd"
}
Rule ScoutCastMerasmusZap
{
	criteria ConceptPlayerCastMerasmusZap IsScout
	Response ScoutCastMerasmusZap
}

Response HeavyCastMerasmusZap
{
	scene	"scenes/player/heavy/low/6384.vcd"
}
Rule HeavyCastMerasmusZap
{
	criteria ConceptPlayerCastMerasmusZap IsHeavy
	Response HeavyCastMerasmusZap
}

Response PyroCastMerasmusZap
{
	scene	"scenes/player/pyro/low/6438.vcd"
}
Rule PyroCastMerasmusZap
{
	criteria ConceptPlayerCastMerasmusZap IsPyro
	Response PyroCastMerasmusZap
}

Response EngineerCastMerasmusZap
{
	scene	"scenes/player/engineer/low/6319.vcd"
}
Rule EngineerCastMerasmusZap
{
	criteria ConceptPlayerCastMerasmusZap IsEngineer
	Response EngineerCastMerasmusZap
}

Response DemomanCastMerasmusZap
{
	scene	"scenes/player/demoman/low/6059.vcd"
}
Rule DemomanCastMerasmusZap
{
	criteria ConceptPlayerCastMerasmusZap IsDemoman
	Response DemomanCastMerasmusZap
}

Response MedicCastMerasmusZap
{
	scene	"scenes/player/medic/low/6579.vcd"
}
Rule MedicCastMerasmusZap
{
	criteria ConceptPlayerCastMerasmusZap IsMedic
	Response MedicCastMerasmusZap
}

Response SniperCastMerasmusZap
{
	scene	"scenes/player/sniper/low/6514.vcd"
}
Rule SniperCastMerasmusZap
{
	criteria ConceptPlayerCastMerasmusZap IsSniper
	Response SniperCastMerasmusZap
}

Response SpyCastMerasmusZap
{
	scene	"scenes/player/spy/low/6124.vcd"
}
Rule SpyCastMerasmusZap
{
	criteria ConceptPlayerCastMerasmusZap IsSpy
	Response SpyCastMerasmusZap
}

Response SoldierCastMerasmusZap
{
	scene	"scenes/player/soldier/low/6189.vcd"
}
Rule SoldierCastMerasmusZap
{
	criteria ConceptPlayerCastMerasmusZap IsSoldier
	Response SoldierCastMerasmusZap
}

Response ScoutCastSelfHeal
{
	scene	"scenes/player/scout/low/6243.vcd"
}
Rule ScoutCastSelfHeal
{
	criteria ConceptPlayerCastSelfHeal IsScout
	Response ScoutCastSelfHeal
}

Response HeavyCastSelfHeal
{
	scene	"scenes/player/heavy/low/6373.vcd"
}
Rule HeavyCastSelfHeal
{
	criteria ConceptPlayerCastSelfHeal IsHeavy
	Response HeavyCastSelfHeal
}

Response PyroCastSelfHeal
{
	scene	"scenes/player/pyro/low/6438.vcd"
}
Rule PyroCastSelfHeal
{
	criteria ConceptPlayerCastSelfHeal IsPyro
	Response PyroCastSelfHeal
}

Response EngineerCastSelfHeal
{
	scene	"scenes/player/engineer/low/6308.vcd"
}
Rule EngineerCastSelfHeal
{
	criteria ConceptPlayerCastSelfHeal IsEngineer
	Response EngineerCastSelfHeal
}

Response DemomanCastSelfHeal
{
	scene	"scenes/player/demoman/low/6048.vcd"
}
Rule DemomanCastSelfHeal
{
	criteria ConceptPlayerCastSelfHeal IsDemoman
	Response DemomanCastSelfHeal
}

Response MedicCastSelfHeal
{
	scene	"scenes/player/medic/low/6568.vcd"
}
Rule MedicCastSelfHeal
{
	criteria ConceptPlayerCastSelfHeal IsMedic
	Response MedicCastSelfHeal
}

Response SniperCastSelfHeal
{
	scene	"scenes/player/sniper/low/6503.vcd"
}
Rule SniperCastSelfHeal
{
	criteria ConceptPlayerCastSelfHeal IsSniper
	Response SniperCastSelfHeal
}

Response SpyCastSelfHeal
{
	scene	"scenes/player/spy/low/6113.vcd"
}
Rule SpyCastSelfHeal
{
	criteria ConceptPlayerCastSelfHeal IsSpy
	Response SpyCastSelfHeal
}

Response SoldierCastSelfHeal
{
	scene	"scenes/player/soldier/low/6178.vcd"
}
Rule SoldierCastSelfHeal
{
	criteria ConceptPlayerCastSelfHeal IsSoldier
	Response SoldierCastSelfHeal
}

Response ScoutCastMirv
{
	scene	"scenes/player/scout/low/6263.vcd"
}
Rule ScoutCastMirv
{
	criteria ConceptPlayerCastMirv IsScout
	Response ScoutCastMirv
}

Response HeavyCastMirv
{
	scene	"scenes/player/heavy/low/6393.vcd"
}
Rule HeavyCastMirv
{
	criteria ConceptPlayerCastMirv IsHeavy
	Response HeavyCastMirv
}

Response PyroCastMirv
{
	scene	"scenes/player/pyro/low/6435.vcd"
}
Rule PyroCastMirv
{
	criteria ConceptPlayerCastMirv IsPyro
	Response PyroCastMirv
}

Response EngineerCastMirv
{
	scene	"scenes/player/engineer/low/6328.vcd"
}
Rule EngineerCastMirv
{
	criteria ConceptPlayerCastMirv IsEngineer
	Response EngineerCastMirv
}

Response DemomanCastMirv
{
	scene	"scenes/player/demoman/low/6068.vcd"
}
Rule DemomanCastMirv
{
	criteria ConceptPlayerCastMirv IsDemoman
	Response DemomanCastMirv
}

Response MedicCastMirv
{
	scene	"scenes/player/medic/low/6588.vcd"
}
Rule MedicCastMirv
{
	criteria ConceptPlayerCastMirv IsMedic
	Response MedicCastMirv
}

Response SniperCastMirv
{
	scene	"scenes/player/sniper/low/6523.vcd"
}
Rule SniperCastMirv
{
	criteria ConceptPlayerCastMirv IsSniper
	Response SniperCastMirv
}

Response SpyCastMirv
{
	scene	"scenes/player/spy/low/6133.vcd"
}
Rule SpyCastMirv
{
	criteria ConceptPlayerCastMirv IsSpy
	Response SpyCastMirv
}

Response SoldierCastMirv
{
	scene	"scenes/player/soldier/low/6198.vcd"
}
Rule SoldierCastMirv
{
	criteria ConceptPlayerCastMirv IsSoldier
	Response SoldierCastMirv
}

Response ScoutCastBlastJump
{
	scene	"scenes/player/scout/low/6247.vcd"
}
Rule ScoutCastBlastJump
{
	criteria ConceptPlayerCastBlastJump IsScout
	Response ScoutCastBlastJump
}

Response HeavyCastBlastJump
{
	scene	"scenes/player/heavy/low/6377.vcd"
}
Rule HeavyCastBlastJump
{
	criteria ConceptPlayerCastBlastJump IsHeavy
	Response HeavyCastBlastJump
}

Response PyroCastBlastJump
{
	scene	"scenes/player/pyro/low/6437.vcd"
}
Rule PyroCastBlastJump
{
	criteria ConceptPlayerCastBlastJump IsPyro
	Response PyroCastBlastJump
}

Response EngineerCastBlastJump
{
	scene	"scenes/player/engineer/low/6312.vcd"
}
Rule EngineerCastBlastJump
{
	criteria ConceptPlayerCastBlastJump IsEngineer
	Response EngineerCastBlastJump
}

Response DemomanCastBlastJump
{
	scene	"scenes/player/demoman/low/6052.vcd"
}
Rule DemomanCastBlastJump
{
	criteria ConceptPlayerCastBlastJump IsDemoman
	Response DemomanCastBlastJump
}

Response MedicCastBlastJump
{
	scene	"scenes/player/medic/low/6572.vcd"
}
Rule MedicCastBlastJump
{
	criteria ConceptPlayerCastBlastJump IsMedic
	Response MedicCastBlastJump
}

Response SniperCastBlastJump
{
	scene	"scenes/player/sniper/low/6507.vcd"
}
Rule SniperCastBlastJump
{
	criteria ConceptPlayerCastBlastJump IsSniper
	Response SniperCastBlastJump
}

Response SpyCastBlastJump
{
	scene	"scenes/player/spy/low/6117.vcd"
}
Rule SpyCastBlastJump
{
	criteria ConceptPlayerCastBlastJump IsSpy
	Response SpyCastBlastJump
}

Response SoldierCastBlastJump
{
	scene	"scenes/player/soldier/low/6182.vcd"
}
Rule SoldierCastBlastJump
{
	criteria ConceptPlayerCastBlastJump IsSoldier
	Response SoldierCastBlastJump
}

Response ScoutCastStealth
{
	scene	"scenes/player/scout/low/6257.vcd"
}
Rule ScoutCastStealth
{
	criteria ConceptPlayerCastStealth IsScout
	Response ScoutCastStealth
}

Response HeavyCastStealth
{
	scene	"scenes/player/heavy/low/6387.vcd"
}
Rule HeavyCastStealth
{
	criteria ConceptPlayerCastStealth IsHeavy
	Response HeavyCastStealth
}

Response PyroCastStealth
{
	scene	"scenes/player/pyro/low/6439.vcd"
}
Rule PyroCastStealth
{
	criteria ConceptPlayerCastStealth IsPyro
	Response PyroCastStealth
}

Response EngineerCastStealth
{
	scene	"scenes/player/engineer/low/6322.vcd"
}
Rule EngineerCastStealth
{
	criteria ConceptPlayerCastStealth IsEngineer
	Response EngineerCastStealth
}

Response DemomanCastStealth
{
	scene	"scenes/player/demoman/low/6062.vcd"
}
Rule DemomanCastStealth
{
	criteria ConceptPlayerCastStealth IsDemoman
	Response DemomanCastStealth
}

Response MedicCastStealth
{
	scene	"scenes/player/medic/low/6582.vcd"
}
Rule MedicCastStealth
{
	criteria ConceptPlayerCastStealth IsMedic
	Response MedicCastStealth
}

Response SniperCastStealth
{
	scene	"scenes/player/sniper/low/6517.vcd"
}
Rule SniperCastStealth
{
	criteria ConceptPlayerCastStealth IsSniper
	Response SniperCastStealth
}

Response SpyCastStealth
{
	scene	"scenes/player/spy/low/6127.vcd"
}
Rule SpyCastStealth
{
	criteria ConceptPlayerCastStealth IsSpy
	Response SpyCastStealth
}

Response SoldierCastStealth
{
	scene	"scenes/player/soldier/low/6192.vcd"
}
Rule SoldierCastStealth
{
	criteria ConceptPlayerCastStealth IsSoldier
	Response SoldierCastStealth
}

Response ScoutCastTeleport
{
	scene	"scenes/player/scout/low/6272.vcd"
}
Rule ScoutCastTeleport
{
	criteria ConceptPlayerCastTeleport IsScout
	Response ScoutCastTeleport
}

Response HeavyCastTeleport
{
	scene	"scenes/player/heavy/low/6402.vcd"
}
Rule HeavyCastTeleport
{
	criteria ConceptPlayerCastTeleport IsHeavy
	Response HeavyCastTeleport
}

Response PyroCastTeleport
{
	scene	"scenes/player/pyro/low/6440.vcd"
}
Rule PyroCastTeleport
{
	criteria ConceptPlayerCastTeleport IsPyro
	Response PyroCastTeleport
}

Response EngineerCastTeleport
{
	scene	"scenes/player/engineer/low/6337.vcd"
}
Rule EngineerCastTeleport
{
	criteria ConceptPlayerCastTeleport IsEngineer
	Response EngineerCastTeleport
}

Response DemomanCastTeleport
{
	scene	"scenes/player/demoman/low/6077.vcd"
}
Rule DemomanCastTeleport
{
	criteria ConceptPlayerCastTeleport IsDemoman
	Response DemomanCastTeleport
}

Response MedicCastTeleport
{
	scene	"scenes/player/medic/low/6597.vcd"
}
Rule MedicCastTeleport
{
	criteria ConceptPlayerCastTeleport IsMedic
	Response MedicCastTeleport
}

Response SniperCastTeleport
{
	scene	"scenes/player/sniper/low/6532.vcd"
}
Rule SniperCastTeleport
{
	criteria ConceptPlayerCastTeleport IsSniper
	Response SniperCastTeleport
}

Response SpyCastTeleport
{
	scene	"scenes/player/spy/low/6142.vcd"
}
Rule SpyCastTeleport
{
	criteria ConceptPlayerCastTeleport IsSpy
	Response SpyCastTeleport
}

Response SoldierCastTeleport
{
	scene	"scenes/player/soldier/low/6207.vcd"
}
Rule SoldierCastTeleport
{
	criteria ConceptPlayerCastTeleport IsSoldier
	Response SoldierCastTeleport
}

Response ScoutCastBombHeadCurse
{
	scene	"scenes/player/scout/low/6264.vcd"
}
Rule ScoutCastBombHeadCurse
{
	criteria ConceptPlayerCastBombHeadCurse IsScout
	Response ScoutCastBombHeadCurse
}

Response HeavyCastBombHeadCurse
{
	scene	"scenes/player/heavy/low/6394.vcd"
}
Rule HeavyCastBombHeadCurse
{
	criteria ConceptPlayerCastBombHeadCurse IsHeavy
	Response HeavyCastBombHeadCurse
}

Response PyroCastBombHeadCurse
{
	scene	"scenes/player/pyro/low/8478.vcd"
}
Rule PyroCastBombHeadCurse
{
	criteria ConceptPlayerCastBombHeadCurse IsPyro
	Response PyroCastBombHeadCurse
}

Response EngineerCastBombHeadCurse
{
	scene	"scenes/player/engineer/low/6329.vcd"
}
Rule EngineerCastBombHeadCurse
{
	criteria ConceptPlayerCastBombHeadCurse IsEngineer
	Response EngineerCastBombHeadCurse
}

Response DemomanCastBombHeadCurse
{
	scene	"scenes/player/demoman/low/6069.vcd"
}
Rule DemomanCastBombHeadCurse
{
	criteria ConceptPlayerCastBombHeadCurse IsDemoman
	Response DemomanCastBombHeadCurse
}

Response MedicCastBombHeadCurse
{
	scene	"scenes/player/medic/low/6589.vcd"
}
Rule MedicCastBombHeadCurse
{
	criteria ConceptPlayerCastBombHeadCurse IsMedic
	Response MedicCastBombHeadCurse
}

Response SniperCastBombHeadCurse
{
	scene	"scenes/player/sniper/low/6524.vcd"
}
Rule SniperCastBombHeadCurse
{
	criteria ConceptPlayerCastBombHeadCurse IsSniper
	Response SniperCastBombHeadCurse
}

Response SpyCastBombHeadCurse
{
	scene	"scenes/player/spy/low/6134.vcd"
}
Rule SpyCastBombHeadCurse
{
	criteria ConceptPlayerCastBombHeadCurse IsSpy
	Response SpyCastBombHeadCurse
}

Response SoldierCastBombHeadCurse
{
	scene	"scenes/player/soldier/low/6199.vcd"
}
Rule SoldierCastBombHeadCurse
{
	criteria ConceptPlayerCastBombHeadCurse IsSoldier
	Response SoldierCastBombHeadCurse
}

Response ScoutCastLightningBall
{
	scene	"scenes/player/scout/low/6268.vcd"
}
Rule ScoutCastLightningBall
{
	criteria ConceptPlayerCastLightningBall IsScout
	Response ScoutCastLightningBall
}

Response HeavyCastLightningBall
{
	scene	"scenes/player/heavy/low/6398.vcd"
}
Rule HeavyCastLightningBall
{
	criteria ConceptPlayerCastLightningBall IsHeavy
	Response HeavyCastLightningBall
}

Response PyroCastLightningBall
{
	scene	"scenes/player/pyro/low/6436.vcd"
}
Rule PyroCastLightningBall
{
	criteria ConceptPlayerCastLightningBall IsPyro
	Response PyroCastLightningBall
}

Response EngineerCastLightningBall
{
	scene	"scenes/player/engineer/low/6333.vcd"
}
Rule EngineerCastLightningBall
{
	criteria ConceptPlayerCastLightningBall IsEngineer
	Response EngineerCastLightningBall
}

Response DemomanCastLightningBall
{
	scene	"scenes/player/demoman/low/6073.vcd"
}
Rule DemomanCastLightningBall
{
	criteria ConceptPlayerCastLightningBall IsDemoman
	Response DemomanCastLightningBall
}

Response MedicCastLightningBall
{
	scene	"scenes/player/medic/low/6593.vcd"
}
Rule MedicCastLightningBall
{
	criteria ConceptPlayerCastLightningBall IsMedic
	Response MedicCastLightningBall
}

Response SniperCastLightningBall
{
	scene	"scenes/player/sniper/low/6528.vcd"
}
Rule SniperCastLightningBall
{
	criteria ConceptPlayerCastLightningBall IsSniper
	Response SniperCastLightningBall
}

Response SpyCastLightningBall
{
	scene	"scenes/player/spy/low/6138.vcd"
}
Rule SpyCastLightningBall
{
	criteria ConceptPlayerCastLightningBall IsSpy
	Response SpyCastLightningBall
}

Response SoldierCastLightningBall
{
	scene	"scenes/player/soldier/low/6203.vcd"
}
Rule SoldierCastLightningBall
{
	criteria ConceptPlayerCastLightningBall IsSoldier
	Response SoldierCastLightningBall
}

Response ScoutCastMovementBuff
{
	scene	"scenes/player/scout/low/6256.vcd"
}
Rule ScoutCastMovementBuff
{
	criteria ConceptPlayerCastMovementBuff IsScout
	Response ScoutCastMovementBuff
}

Response HeavyCastMovementBuff
{
	scene	"scenes/player/heavy/low/6386.vcd"
}
Rule HeavyCastMovementBuff
{
	criteria ConceptPlayerCastMovementBuff IsHeavy
	Response HeavyCastMovementBuff
}

Response PyroCastMovementBuff
{
	scene	"scenes/player/pyro/low/8479.vcd"
}
Rule PyroCastMovementBuff
{
	criteria ConceptPlayerCastMovementBuff IsPyro
	Response PyroCastMovementBuff
}

Response EngineerCastMovementBuff
{
	scene	"scenes/player/engineer/low/6321.vcd"
}
Rule EngineerCastMovementBuff
{
	criteria ConceptPlayerCastMovementBuff IsEngineer
	Response EngineerCastMovementBuff
}

Response DemomanCastMovementBuff
{
	scene	"scenes/player/demoman/low/6061.vcd"
}
Rule DemomanCastMovementBuff
{
	criteria ConceptPlayerCastMovementBuff IsDemoman
	Response DemomanCastMovementBuff
}

Response MedicCastMovementBuff
{
	scene	"scenes/player/medic/low/6581.vcd"
}
Rule MedicCastMovementBuff
{
	criteria ConceptPlayerCastMovementBuff IsMedic
	Response MedicCastMovementBuff
}

Response SniperCastMovementBuff
{
	scene	"scenes/player/sniper/low/6516.vcd"
}
Rule SniperCastMovementBuff
{
	criteria ConceptPlayerCastMovementBuff IsSniper
	Response SniperCastMovementBuff
}

Response SpyCastMovementBuff
{
	scene	"scenes/player/spy/low/6126.vcd"
}
Rule SpyCastMovementBuff
{
	criteria ConceptPlayerCastMovementBuff IsSpy
	Response SpyCastMovementBuff
}

Response SoldierCastMovementBuff
{
	scene	"scenes/player/soldier/low/6191.vcd"
}
Rule SoldierCastMovementBuff
{
	criteria ConceptPlayerCastMovementBuff IsSoldier
	Response SoldierCastMovementBuff
}

Response ScoutCastMonoculous
{
	scene	"scenes/player/scout/low/6296.vcd"
}
Rule ScoutCastMonoculous
{
	criteria ConceptPlayerCastMonoculous IsScout
	Response ScoutCastMonoculous
}

Response HeavyCastMonoculous
{
	scene	"scenes/player/heavy/low/6426.vcd"
}
Rule HeavyCastMonoculous
{
	criteria ConceptPlayerCastMonoculous IsHeavy
	Response HeavyCastMonoculous
}

Response PyroCastMonoculous
{
	scene	"scenes/player/pyro/low/8480.vcd"
}
Rule PyroCastMonoculous
{
	criteria ConceptPlayerCastMonoculous IsPyro
	Response PyroCastMonoculous
}

Response EngineerCastMonoculous
{
	scene	"scenes/player/engineer/low/6361.vcd"
}
Rule EngineerCastMonoculous
{
	criteria ConceptPlayerCastMonoculous IsEngineer
	Response EngineerCastMonoculous
}

Response DemomanCastMonoculous
{
	scene	"scenes/player/demoman/low/6101.vcd"
	scene	"scenes/player/demoman/low/8527.vcd"
}
Rule DemomanCastMonoculous
{
	criteria ConceptPlayerCastMonoculous IsDemoman
	Response DemomanCastMonoculous
}

Response MedicCastMonoculous
{
	scene	"scenes/player/medic/low/6621.vcd"
}
Rule MedicCastMonoculous
{
	criteria ConceptPlayerCastMonoculous IsMedic
	Response MedicCastMonoculous
}

Response SniperCastMonoculous
{
	scene	"scenes/player/sniper/low/6556.vcd"
}
Rule SniperCastMonoculous
{
	criteria ConceptPlayerCastMonoculous IsSniper
	Response SniperCastMonoculous
}

Response SpyCastMonoculous
{
	scene	"scenes/player/spy/low/6166.vcd"
}
Rule SpyCastMonoculous
{
	criteria ConceptPlayerCastMonoculous IsSpy
	Response SpyCastMonoculous
}

Response SoldierCastMonoculous
{
	scene	"scenes/player/soldier/low/6231.vcd"
}
Rule SoldierCastMonoculous
{
	criteria ConceptPlayerCastMonoculous IsSoldier
	Response SoldierCastMonoculous
}

Response ScoutCastMeteorSwarm
{
	scene	"scenes/player/scout/low/6291.vcd"
}
Rule ScoutCastMeteorSwarm
{
	criteria ConceptPlayerCastMeteorSwarm IsScout
	Response ScoutCastMeteorSwarm
}

Response HeavyCastMeteorSwarm
{
	scene	"scenes/player/heavy/low/6421.vcd"
}
Rule HeavyCastMeteorSwarm
{
	criteria ConceptPlayerCastMeteorSwarm IsHeavy
	Response HeavyCastMeteorSwarm
}

Response PyroCastMeteorSwarm
{
	scene	"scenes/player/pyro/low/6440.vcd"
}
Rule PyroCastMeteorSwarm
{
	criteria ConceptPlayerCastMeteorSwarm IsPyro
	Response PyroCastMeteorSwarm
}

Response EngineerCastMeteorSwarm
{
	scene	"scenes/player/engineer/low/6356.vcd"
}
Rule EngineerCastMeteorSwarm
{
	criteria ConceptPlayerCastMeteorSwarm IsEngineer
	Response EngineerCastMeteorSwarm
}

Response DemomanCastMeteorSwarm
{
	scene	"scenes/player/demoman/low/6096.vcd"
}
Rule DemomanCastMeteorSwarm
{
	criteria ConceptPlayerCastMeteorSwarm IsDemoman
	Response DemomanCastMeteorSwarm
}

Response MedicCastMeteorSwarm
{
	scene	"scenes/player/medic/low/6616.vcd"
}
Rule MedicCastMeteorSwarm
{
	criteria ConceptPlayerCastMeteorSwarm IsMedic
	Response MedicCastMeteorSwarm
}

Response SniperCastMeteorSwarm
{
	scene	"scenes/player/sniper/low/6551.vcd"
}
Rule SniperCastMeteorSwarm
{
	criteria ConceptPlayerCastMeteorSwarm IsSniper
	Response SniperCastMeteorSwarm
}

Response SpyCastMeteorSwarm
{
	scene	"scenes/player/spy/low/6161.vcd"
}
Rule SpyCastMeteorSwarm
{
	criteria ConceptPlayerCastMeteorSwarm IsSpy
	Response SpyCastMeteorSwarm
}

Response SoldierCastMeteorSwarm
{
	scene	"scenes/player/soldier/low/6226.vcd"
}
Rule SoldierCastMeteorSwarm
{
	criteria ConceptPlayerCastMeteorSwarm IsSoldier
	Response SoldierCastMeteorSwarm
}

Response ScoutCastSkeletonHorde
{
	scene	"scenes/player/scout/low/6293.vcd"
}
Rule ScoutCastSkeletonHorde
{
	criteria ConceptPlayerCastSkeletonHorde IsScout
	Response ScoutCastSkeletonHorde
}

Response HeavyCastSkeletonHorde
{
	scene	"scenes/player/heavy/low/6423.vcd"
}
Rule HeavyCastSkeletonHorde
{
	criteria ConceptPlayerCastSkeletonHorde IsHeavy
	Response HeavyCastSkeletonHorde
}

Response PyroCastSkeletonHorde
{
	scene	"scenes/player/pyro/low/6435.vcd"
}
Rule PyroCastSkeletonHorde
{
	criteria ConceptPlayerCastSkeletonHorde IsPyro
	Response PyroCastSkeletonHorde
}

Response EngineerCastSkeletonHorde
{
	scene	"scenes/player/engineer/low/6358.vcd"
}
Rule EngineerCastSkeletonHorde
{
	criteria ConceptPlayerCastSkeletonHorde IsEngineer
	Response EngineerCastSkeletonHorde
}

Response DemomanCastSkeletonHorde
{
	scene	"scenes/player/demoman/low/6098.vcd"
}
Rule DemomanCastSkeletonHorde
{
	criteria ConceptPlayerCastSkeletonHorde IsDemoman
	Response DemomanCastSkeletonHorde
}

Response MedicCastSkeletonHorde
{
	scene	"scenes/player/medic/low/6618.vcd"
}
Rule MedicCastSkeletonHorde
{
	criteria ConceptPlayerCastSkeletonHorde IsMedic
	Response MedicCastSkeletonHorde
}

Response SniperCastSkeletonHorde
{
	scene	"scenes/player/sniper/low/6553.vcd"
}
Rule SniperCastSkeletonHorde
{
	criteria ConceptPlayerCastSkeletonHorde IsSniper
	Response SniperCastSkeletonHorde
}

Response SpyCastSkeletonHorde
{
	scene	"scenes/player/spy/low/6163.vcd"
}
Rule SpyCastSkeletonHorde
{
	criteria ConceptPlayerCastSkeletonHorde IsSpy
	Response SpyCastSkeletonHorde
}

Response SoldierCastSkeletonHorde
{
	scene	"scenes/player/soldier/low/6228.vcd"
}
Rule SoldierCastSkeletonHorde
{
	criteria ConceptPlayerCastSkeletonHorde IsSoldier
	Response SoldierCastSkeletonHorde
}

Response ScoutSeeSpellMeteorSwarm
{
	scene	"scenes/player/scout/low/8520.vcd"
	scene	"scenes/player/scout/low/8521.vcd"
	scene	"scenes/player/scout/low/8522.vcd"
	scene	"scenes/player/scout/low/8523.vcd"
	scene	"scenes/player/scout/low/8524.vcd"
	scene	"scenes/player/scout/low/8525.vcd"
	scene	"scenes/player/scout/low/8526.vcd"
	scene	"scenes/player/scout/low/4461.vcd"
	scene	"scenes/player/scout/low/4462.vcd"
	scene	"scenes/player/scout/low/4463.vcd"
	scene	"scenes/player/scout/low/4680.vcd"
}
Rule ScoutSeeSpellMeteorSwarm
{
	criteria ConceptPlayerSpellMeteorSwarm IsScout
	Response ScoutSeeSpellMeteorSwarm
}

Response HeavySeeSpellMeteorSwarm
{
	scene	"scenes/player/heavy/low/6684.vcd"
	scene	"scenes/player/heavy/low/6685.vcd"
	scene	"scenes/player/heavy/low/6687.vcd"
	scene	"scenes/player/heavy/low/4749.vcd"
	scene	"scenes/player/heavy/low/5456.vcd"
}
Rule HeavySeeSpellMeteorSwarm
{
	criteria ConceptPlayerSpellMeteorSwarm IsHeavy
	Response HeavySeeSpellMeteorSwarm
}

Response EngineerSeeSpellMeteorSwarm
{
	scene	"scenes/player/engineer/low/7947.vcd"
	scene	"scenes/player/engineer/low/7948.vcd"
}
Rule EngineerSeeSpellMeteorSwarm
{
	criteria ConceptPlayerSpellMeteorSwarm IsEngineer
	Response EngineerSeeSpellMeteorSwarm
}

Response DemomanSeeSpellMeteorSwarm
{
	scene	"scenes/player/demoman/low/8538.vcd"
	scene	"scenes/player/demoman/low/8540.vcd"
	scene	"scenes/player/demoman/low/4595.vcd"
	scene	"scenes/player/demoman/low/4596.vcd"
}
Rule DemomanSeeSpellMeteorSwarm
{
	criteria ConceptPlayerSpellMeteorSwarm IsDemoman
	Response DemomanSeeSpellMeteorSwarm
}

Response MedicSeeSpellMeteorSwarm
{
	scene	"scenes/player/medic/low/6816.vcd"
	scene	"scenes/player/medic/low/6817.vcd"
	scene	"scenes/player/medic/low/6818.vcd"
	scene	"scenes/player/medic/low/6819.vcd"
	scene	"scenes/player/medic/low/8508.vcd"
	scene	"scenes/player/medic/low/8509.vcd"
	scene	"scenes/player/medic/low/8510.vcd"
}
Rule MedicSeeSpellMeteorSwarm
{
	criteria ConceptPlayerSpellMeteorSwarm IsMedic
	Response MedicSeeSpellMeteorSwarm
}

Response SniperSeeSpellMeteorSwarm
{
	scene	"scenes/player/sniper/low/7276.vcd"
	scene	"scenes/player/sniper/low/7277.vcd"
	scene	"scenes/player/sniper/low/7278.vcd"
	scene	"scenes/player/sniper/low/7279.vcd"
	scene	"scenes/player/sniper/low/7280.vcd"
	scene	"scenes/player/sniper/low/7281.vcd"
	scene	"scenes/player/sniper/low/8483.vcd"
}
Rule SniperSeeSpellMeteorSwarm
{
	criteria ConceptPlayerSpellMeteorSwarm IsSniper
	Response SniperSeeSpellMeteorSwarm
}

Response SpySeeSpellMeteorSwarm
{
	scene	"scenes/player/spy/low/7523.vcd"
	scene	"scenes/player/spy/low/7524.vcd"
	scene	"scenes/player/spy/low/7634.vcd"
	scene	"scenes/player/spy/low/7635.vcd"
	scene	"scenes/player/spy/low/7637.vcd"
	scene	"scenes/player/spy/low/7638.vcd"
	scene	"scenes/player/spy/low/4714.vcd"
	scene	"scenes/player/spy/low/4715.vcd"
}
Rule SpySeeSpellMeteorSwarm
{
	criteria ConceptPlayerSpellMeteorSwarm IsSpy
	Response SpySeeSpellMeteorSwarm
}

Response SoldierSeeSpellMeteorSwarm
{
	scene	"scenes/player/soldier/low/7445.vcd"
	scene	"scenes/player/soldier/low/7450.vcd"
	scene	"scenes/player/soldier/low/4550.vcd"
	scene	"scenes/player/soldier/low/4551.vcd"
}
Rule SoldierSeeSpellMeteorSwarm
{
	criteria ConceptPlayerSpellMeteorSwarm IsSoldier
	Response SoldierSeeSpellMeteorSwarm
}

Response ScoutSeeSpellSkeletonHorde
{
	scene	"scenes/player/scout/low/8520.vcd"
	scene	"scenes/player/scout/low/8521.vcd"
	scene	"scenes/player/scout/low/8522.vcd"
	scene	"scenes/player/scout/low/8523.vcd"
	scene	"scenes/player/scout/low/8524.vcd"
}
Rule ScoutSeeSpellSkeletonHorde
{
	criteria ConceptPlayerSpellSkeletonHorde IsScout
	Response ScoutSeeSpellSkeletonHorde
}

Response HeavySeeSpellSkeletonHorde
{
	scene	"scenes/player/heavy/low/6684.vcd"
	scene	"scenes/player/heavy/low/6685.vcd"
	scene	"scenes/player/heavy/low/6687.vcd"
}
Rule HeavySeeSpellSkeletonHorde
{
	criteria ConceptPlayerSpellSkeletonHorde IsHeavy
	Response HeavySeeSpellSkeletonHorde
}

Response EngineerSeeSpellSkeletonHorde
{
	scene	"scenes/player/engineer/low/7947.vcd"
	scene	"scenes/player/engineer/low/7948.vcd"
}
Rule EngineerSeeSpellSkeletonHorde
{
	criteria ConceptPlayerSpellSkeletonHorde IsEngineer
	Response EngineerSeeSpellSkeletonHorde
}

Response DemomanSeeSpellSkeletonHorde
{
	scene	"scenes/player/demoman/low/7755.vcd"
	scene	"scenes/player/demoman/low/8538.vcd"
}
Rule DemomanSeeSpellSkeletonHorde
{
	criteria ConceptPlayerSpellSkeletonHorde IsDemoman
	Response DemomanSeeSpellSkeletonHorde
}

Response MedicSeeSpellSkeletonHorde
{
	scene	"scenes/player/medic/low/6816.vcd"
	scene	"scenes/player/medic/low/6818.vcd"
	scene	"scenes/player/medic/low/6819.vcd"
	scene	"scenes/player/medic/low/8508.vcd"
	scene	"scenes/player/medic/low/8509.vcd"
	scene	"scenes/player/medic/low/8510.vcd"
}
Rule MedicSeeSpellSkeletonHorde
{
	criteria ConceptPlayerSpellSkeletonHorde IsMedic
	Response MedicSeeSpellSkeletonHorde
}

Response SniperSeeSpellSkeletonHorde
{
	scene	"scenes/player/sniper/low/7276.vcd"
	scene	"scenes/player/sniper/low/7277.vcd"
	scene	"scenes/player/sniper/low/7278.vcd"
	scene	"scenes/player/sniper/low/7279.vcd"
	scene	"scenes/player/sniper/low/7280.vcd"
	scene	"scenes/player/sniper/low/7281.vcd"
	scene	"scenes/player/sniper/low/8483.vcd"
}
Rule SniperSeeSpellSkeletonHorde
{
	criteria ConceptPlayerSpellSkeletonHorde IsSniper
	Response SniperSeeSpellSkeletonHorde
}

Response SpySeeSpellSkeletonHorde
{
	scene	"scenes/player/spy/low/7523.vcd"
	scene	"scenes/player/spy/low/7524.vcd"
	scene	"scenes/player/spy/low/7634.vcd"
	scene	"scenes/player/spy/low/7635.vcd"
	scene	"scenes/player/spy/low/7637.vcd"
	scene	"scenes/player/spy/low/7638.vcd"
}
Rule SpySeeSpellSkeletonHorde
{
	criteria ConceptPlayerSpellSkeletonHorde IsSpy
	Response SpySeeSpellSkeletonHorde
}

Response SoldierSeeSpellSkeletonHorde
{
	scene	"scenes/player/soldier/low/7445.vcd"
	scene	"scenes/player/soldier/low/7450.vcd"
}
Rule SoldierSeeSpellSkeletonHorde
{
	criteria ConceptPlayerSpellSkeletonHorde IsSoldier
	Response SoldierSeeSpellSkeletonHorde
}

Response ScoutSF13RoundStart
{
	scene	"scenes/player/scout/low/6957.vcd" predelay "8, 10"
	scene	"scenes/player/scout/low/6961.vcd" predelay "8, 10"
	scene	"scenes/player/scout/low/6979.vcd" predelay "8, 10"
}
Rule ScoutSF13RoundStart
{
	criteria ConceptPlayerRoundStart IsScout 75PercentChance IsMapHelltower IsNotInHell
	Response ScoutSF13RoundStart
}

Response EngineerSF13RoundStart
{
	scene	"scenes/player/engineer/low/7896.vcd" predelay "8, 10"
	scene	"scenes/player/engineer/low/7899.vcd" predelay "8, 10"
	scene	"scenes/player/engineer/low/7900.vcd" predelay "8, 10"
	scene	"scenes/player/engineer/low/7901.vcd" predelay "8, 10"
}
Rule EngineerSF13RoundStart
{
	criteria ConceptPlayerRoundStart IsEngineer 75PercentChance IsMapHelltower IsNotInHell
	Response EngineerSF13RoundStart
}

Response DemomanSF13RoundStart
{
	scene	"scenes/player/demoman/low/7732.vcd" predelay "8, 10"
	scene	"scenes/player/demoman/low/7733.vcd" predelay "8, 10"
}
Rule DemomanSF13RoundStart
{
	criteria ConceptPlayerRoundStart IsDemoman 75PercentChance IsMapHelltower IsNotInHell
	Response DemomanSF13RoundStart
}

Response MedicSF13RoundStart
{
	scene	"scenes/player/medic/low/6788.vcd" predelay "8, 10"
}
Rule MedicSF13RoundStart
{
	criteria ConceptPlayerRoundStart IsMedic 75PercentChance IsMapHelltower IsNotInHell
	Response MedicSF13RoundStart
}

Response SniperSF13RoundStart
{
	scene	"scenes/player/sniper/low/7244.vcd" predelay "8, 10"
	scene	"scenes/player/sniper/low/7245.vcd" predelay "8, 10"
}
Rule SniperSF13RoundStart
{
	criteria ConceptPlayerRoundStart IsSniper 75PercentChance IsMapHelltower IsNotInHell
	Response SniperSF13RoundStart
}

Response SpySF13RoundStart
{
	scene	"scenes/player/spy/low/7536.vcd" predelay "8, 10"
	scene	"scenes/player/spy/low/7538.vcd" predelay "8, 10"
	scene	"scenes/player/spy/low/7539.vcd" predelay "8, 10"
	scene	"scenes/player/spy/low/7604.vcd" predelay "8, 10"
	scene	"scenes/player/spy/low/7605.vcd" predelay "8, 10"
	scene	"scenes/player/spy/low/7606.vcd" predelay "8, 10"
}
Rule SpySF13RoundStart
{
	criteria ConceptPlayerRoundStart IsSpy 75PercentChance IsMapHelltower IsNotInHell
	Response SpySF13RoundStart
}

Response SoldierSF13RoundStart
{
	scene	"scenes/player/soldier/low/7402.vcd" predelay "8, 10"
	scene	"scenes/player/soldier/low/7405.vcd" predelay "8, 10"
	scene	"scenes/player/soldier/low/7406.vcd" predelay "8, 10"
	scene	"scenes/player/soldier/low/7415.vcd" predelay "8, 10"
	scene	"scenes/player/soldier/low/7417.vcd" predelay "8, 10"
}
Rule SoldierSF13RoundStart
{
	criteria ConceptPlayerRoundStart IsSoldier 75PercentChance IsMapHelltower IsNotInHell
	Response SoldierSF13RoundStart
}

Response ScoutSF13SpellPickupCommon
{
	scene	"scenes/player/scout/low/6935.vcd"
	scene	"scenes/player/scout/low/6936.vcd"
	scene	"scenes/player/scout/low/6938.vcd"
	scene	"scenes/player/scout/low/6995.vcd"
	scene	"scenes/player/scout/low/8568.vcd"
	scene	"scenes/player/scout/low/8511.vcd"
}
Rule ScoutSF13SpellPickupCommon
{
	criteria ConceptPlayerSpellPickupCommon IsScout
	Response ScoutSF13SpellPickupCommon
}

Response HeavySF13SpellPickupCommon
{
	scene	"scenes/player/heavy/low/6643.vcd"
	scene	"scenes/player/heavy/low/8490.vcd"
}
Rule HeavySF13SpellPickupCommon
{
	criteria ConceptPlayerSpellPickupCommon IsHeavy
	Response HeavySF13SpellPickupCommon
}

Response PyroSF13SpellPickupCommon
{
	scene	"scenes/player/pyro/low/6499.vcd"
}
Rule PyroSF13SpellPickupCommon
{
	criteria ConceptPlayerSpellPickupCommon IsPyro
	Response PyroSF13SpellPickupCommon
}

Response EngineerSF13SpellPickupCommon
{
	scene	"scenes/player/engineer/low/7857.vcd"
}
Rule EngineerSF13SpellPickupCommon
{
	criteria ConceptPlayerSpellPickupCommon IsEngineer
	Response EngineerSF13SpellPickupCommon
}

Response DemomanSF13SpellPickupCommon
{
	scene	"scenes/player/demoman/low/7763.vcd"
	scene	"scenes/player/demoman/low/7764.vcd"
	scene	"scenes/player/demoman/low/7765.vcd"
	scene	"scenes/player/demoman/low/7791.vcd"
}
Rule DemomanSF13SpellPickupCommon
{
	criteria ConceptPlayerSpellPickupCommon IsDemoman
	Response DemomanSF13SpellPickupCommon
}

Response MedicSF13SpellPickupCommon
{
	scene	"scenes/player/medic/low/6735.vcd"
	scene	"scenes/player/medic/low/6738.vcd"
	scene	"scenes/player/medic/low/6811.vcd"
}
Rule MedicSF13SpellPickupCommon
{
	criteria ConceptPlayerSpellPickupCommon IsMedic
	Response MedicSF13SpellPickupCommon
}

Response SniperSF13SpellPickupCommon
{
	scene	"scenes/player/sniper/low/7211.vcd"
	scene	"scenes/player/sniper/low/7213.vcd"
	scene	"scenes/player/sniper/low/7263.vcd"
}
Rule SniperSF13SpellPickupCommon
{
	criteria ConceptPlayerSpellPickupCommon IsSniper
	Response SniperSF13SpellPickupCommon
}

Response SpySF13SpellPickupCommon
{
	scene	"scenes/player/spy/low/7520.vcd"
	scene	"scenes/player/spy/low/7522.vcd"
	scene	"scenes/player/spy/low/8548.vcd"
	scene	"scenes/player/spy/low/8549.vcd"
	scene	"scenes/player/spy/low/8550.vcd"
}
Rule SpySF13SpellPickupCommon
{
	criteria ConceptPlayerSpellPickupCommon IsSpy
	Response SpySF13SpellPickupCommon
}

Response SoldierSF13SpellPickupCommon
{
	scene	"scenes/player/soldier/low/7331.vcd"
}
Rule SoldierSF13SpellPickupCommon
{
	criteria ConceptPlayerSpellPickupCommon IsSoldier
	Response SoldierSF13SpellPickupCommon
}

Response ScoutSF13SpellPickupRare
{
	scene	"scenes/player/scout/low/6944.vcd"
	scene	"scenes/player/scout/low/6996.vcd"
	scene	"scenes/player/scout/low/6997.vcd"
	scene	"scenes/player/scout/low/6999.vcd"
	scene	"scenes/player/scout/low/8512.vcd"
	scene	"scenes/player/scout/low/8518.vcd"
	scene	"scenes/player/scout/low/8519.vcd"
}
Rule ScoutSF13SpellPickupRare
{
	criteria ConceptPlayerSpellPickupRare IsScout
	Response ScoutSF13SpellPickupRare
}

Response HeavySF13SpellPickupRare
{
	scene	"scenes/player/heavy/low/6645.vcd"
	scene	"scenes/player/heavy/low/6680.vcd"
	scene	"scenes/player/heavy/low/6682.vcd"
	scene	"scenes/player/heavy/low/8496.vcd"
	scene	"scenes/player/heavy/low/8498.vcd"
}
Rule HeavySF13SpellPickupRare
{
	criteria ConceptPlayerSpellPickupRare IsHeavy
	Response HeavySF13SpellPickupRare
}

Response PyroSF13SpellPickupRare
{
	scene	"scenes/player/pyro/low/6498.vcd"
}
Rule PyroSF13SpellPickupRare
{
	criteria ConceptPlayerSpellPickupRare IsPyro
	Response PyroSF13SpellPickupRare
}

Response EngineerSF13SpellPickupRare
{
	scene	"scenes/player/engineer/low/7860.vcd"
	scene	"scenes/player/engineer/low/7939.vcd"
}
Rule EngineerSF13SpellPickupRare
{
	criteria ConceptPlayerSpellPickupRare IsEngineer
	Response EngineerSF13SpellPickupRare
}

Response DemomanSF13SpellPickupRare
{
	scene	"scenes/player/demoman/low/7766.vcd"
	scene	"scenes/player/demoman/low/7767.vcd"
	scene	"scenes/player/demoman/low/7768.vcd"
	scene	"scenes/player/demoman/low/7769.vcd"
	scene	"scenes/player/demoman/low/8531.vcd"
}
Rule DemomanSF13SpellPickupRare
{
	criteria ConceptPlayerSpellPickupRare IsDemoman
	Response DemomanSF13SpellPickupRare
}

Response MedicSF13SpellPickupRare
{
	scene	"scenes/player/medic/low/6814.vcd"
	scene	"scenes/player/medic/low/6815.vcd"
}
Rule MedicSF13SpellPickupRare
{
	criteria ConceptPlayerSpellPickupRare IsMedic
	Response MedicSF13SpellPickupRare
}

Response SniperSF13SpellPickupRare
{
	scene	"scenes/player/sniper/low/7269.vcd"
	scene	"scenes/player/sniper/low/7270.vcd"
	scene	"scenes/player/sniper/low/7271.vcd"
	scene	"scenes/player/sniper/low/7273.vcd"
}
Rule SniperSF13SpellPickupRare
{
	criteria ConceptPlayerSpellPickupRare IsSniper
	Response SniperSF13SpellPickupRare
}

Response SpySF13SpellPickupRare
{
	scene	"scenes/player/spy/low/7515.vcd"
	scene	"scenes/player/spy/low/7632.vcd"
	scene	"scenes/player/spy/low/7633.vcd"
	scene	"scenes/player/spy/low/8546.vcd"
}
Rule SpySF13SpellPickupRare
{
	criteria ConceptPlayerSpellPickupRare IsSpy
	Response SpySF13SpellPickupRare
}

Response SoldierSF13SpellPickupRare
{
	scene	"scenes/player/soldier/low/7332.vcd"
	scene	"scenes/player/soldier/low/7333.vcd"
	scene	"scenes/player/soldier/low/7438.vcd"
	scene	"scenes/player/soldier/low/7439.vcd"
	scene	"scenes/player/soldier/low/7440.vcd"
}
Rule SoldierSF13SpellPickupRare
{
	criteria ConceptPlayerSpellPickupRare IsSoldier
	Response SoldierSF13SpellPickupRare
}

Response ScoutSF13WitchingHour
{
	scene	"scenes/player/scout/low/6989.vcd"
}
Rule ScoutSF13WitchingHour
{
	criteria ConceptPlayerHelltowerMidnight IsScout IsScout 30PercentChance IsMapHelltower IsNotInHell
	Response ScoutSF13WitchingHour
}

Response EngineerSF13WitchingHour
{
	scene	"scenes/player/engineer/low/7932.vcd"
}
Rule EngineerSF13WitchingHour
{
	criteria ConceptPlayerHelltowerMidnight IsEngineer IsEngineer 30PercentChance IsMapHelltower IsNotInHell
	Response EngineerSF13WitchingHour
}

Response DemomanSF13WitchingHour
{
	scene	"scenes/player/demoman/low/7758.vcd"
	scene	"scenes/player/demoman/low/8528.vcd"
}
Rule DemomanSF13WitchingHour
{
	criteria ConceptPlayerHelltowerMidnight IsDemoman IsDemoman 30PercentChance IsMapHelltower IsNotInHell
	Response DemomanSF13WitchingHour
}

Response MedicSF13WitchingHour
{
	scene	"scenes/player/medic/low/6809.vcd"
	scene	"scenes/player/medic/low/8505.vcd"
}
Rule MedicSF13WitchingHour
{
	criteria ConceptPlayerHelltowerMidnight IsMedic IsMedic 30PercentChance IsMapHelltower IsNotInHell
	Response MedicSF13WitchingHour
}

Response SniperSF13WitchingHour
{
	scene	"scenes/player/sniper/low/7256.vcd"
}
Rule SniperSF13WitchingHour
{
	criteria ConceptPlayerHelltowerMidnight IsSniper IsSniper 30PercentChance IsMapHelltower IsNotInHell
	Response SniperSF13WitchingHour
}

Response SpySF13WitchingHour
{
	scene	"scenes/player/spy/low/7628.vcd"
}
Rule SpySF13WitchingHour
{
	criteria ConceptPlayerHelltowerMidnight IsSpy IsSpy 30PercentChance IsMapHelltower IsNotInHell
	Response SpySF13WitchingHour
}

Response SoldierSF13WitchingHour
{
	scene	"scenes/player/soldier/low/7429.vcd"
}
Rule SoldierSF13WitchingHour
{
	criteria ConceptPlayerHelltowerMidnight IsSoldier IsSoldier 30PercentChance IsMapHelltower IsNotInHell
	Response SoldierSF13WitchingHour
}

Response ScoutSF13Bridge
{
	scene	"scenes/player/scout/low/6982.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/scout/low/6983.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/scout/low/6984.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/scout/low/6985.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/scout/low/6986.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/scout/low/6990.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/scout/low/8514.vcd" predelay "3.5, 5.5"
}
Rule ScoutSF13Bridge
{
	criteria ConceptPlayerHelltowerMidnight IsScout 75PercentChance IsMapHelltower IsNotInHell
	Response ScoutSF13Bridge
}

Response HeavySF13Bridge
{
	scene	"scenes/player/heavy/low/6670.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/heavy/low/6673.vcd" predelay "3.5, 5.5"
}
Rule HeavySF13Bridge
{
	criteria ConceptPlayerHelltowerMidnight IsHeavy 75PercentChance IsMapHelltower IsNotInHell
	Response HeavySF13Bridge
}

Response EngineerSF13Bridge
{
	scene	"scenes/player/engineer/low/7928.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/engineer/low/7931.vcd" predelay "3.5, 5.5"
}
Rule EngineerSF13Bridge
{
	criteria ConceptPlayerHelltowerMidnight IsEngineer 75PercentChance IsMapHelltower IsNotInHell
	Response EngineerSF13Bridge
}

Response DemomanSF13Bridge
{
	scene	"scenes/player/demoman/low/7759.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/demoman/low/7761.vcd" predelay "3.5, 5.5"
}
Rule DemomanSF13Bridge
{
	criteria ConceptPlayerHelltowerMidnight IsDemoman 75PercentChance IsMapHelltower IsNotInHell
	Response DemomanSF13Bridge
}

Response MedicSF13Bridge
{
	scene	"scenes/player/medic/low/6803.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/medic/low/6804.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/medic/low/6805.vcd" predelay "3.5, 5.5"
}
Rule MedicSF13Bridge
{
	criteria ConceptPlayerHelltowerMidnight IsMedic 75PercentChance IsMapHelltower IsNotInHell
	Response MedicSF13Bridge
}

Response SniperSF13Bridge
{
	scene	"scenes/player/sniper/low/7254.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/sniper/low/7257.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/sniper/low/7260.vcd" predelay "3.5, 5.5"
}
Rule SniperSF13Bridge
{
	criteria ConceptPlayerHelltowerMidnight IsSniper 75PercentChance IsMapHelltower IsNotInHell
	Response SniperSF13Bridge
}

Response SpySF13Bridge
{
	scene	"scenes/player/spy/low/7625.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/spy/low/7626.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/spy/low/7627.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/spy/low/7629.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/spy/low/7631.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/spy/low/8554.vcd" predelay "3.5, 5.5"
}
Rule SpySF13Bridge
{
	criteria ConceptPlayerHelltowerMidnight IsSpy 75PercentChance IsMapHelltower IsNotInHell
	Response SpySF13Bridge
}

Response SoldierSF13Bridge
{
	scene	"scenes/player/soldier/low/7431.vcd" predelay "3.5, 5.5"
	scene	"scenes/player/soldier/low/7432.vcd" predelay "3.5, 5.5"
}
Rule SoldierSF13Bridge
{
	criteria ConceptPlayerHelltowerMidnight IsSoldier 75PercentChance IsMapHelltower IsNotInHell
	Response SoldierSF13Bridge
}

Response ScoutSF13BattleCry
{
	scene	"scenes/player/scout/low/6957.vcd"
	scene	"scenes/player/scout/low/6961.vcd"
	scene	"scenes/player/scout/low/6979.vcd"
}
Rule ScoutSF13BattleCry
{
	criteria ConceptPlayerBattleCry IsScout IsScout IsScout 75PercentChance IsMapHelltower IsNotInHell
	Response ScoutSF13BattleCry
}

Response HeavySF13BattleCry
{
	scene	"scenes/player/heavy/low/6662.vcd"
	scene	"scenes/player/heavy/low/6663.vcd"
	scene	"scenes/player/heavy/low/6664.vcd"
	scene	"scenes/player/heavy/low/8492.vcd"
}
Rule HeavySF13BattleCry
{
	criteria ConceptPlayerBattleCry IsHeavy IsHeavy IsHeavy 75PercentChance IsMapHelltower IsNotInHell
	Response HeavySF13BattleCry
}

Response EngineerSF13BattleCry
{
	scene	"scenes/player/engineer/low/7896.vcd"
	scene	"scenes/player/engineer/low/8474.vcd"
	scene	"scenes/player/engineer/low/8475.vcd"
	scene	"scenes/player/engineer/low/8476.vcd"
}
Rule EngineerSF13BattleCry
{
	criteria ConceptPlayerBattleCry IsEngineer IsEngineer IsEngineer 75PercentChance IsMapHelltower IsNotInHell
	Response EngineerSF13BattleCry
}

Response MedicSF13BattleCry
{
	scene	"scenes/player/medic/low/6789.vcd"
	scene	"scenes/player/medic/low/6790.vcd"
}
Rule MedicSF13BattleCry
{
	criteria ConceptPlayerBattleCry IsMedic IsMedic IsMedic 75PercentChance IsMapHelltower IsNotInHell
	Response MedicSF13BattleCry
}

Response SniperSF13BattleCry
{
	scene	"scenes/player/sniper/low/7244.vcd"
	scene	"scenes/player/sniper/low/7245.vcd"
}
Rule SniperSF13BattleCry
{
	criteria ConceptPlayerBattleCry IsSniper IsSniper IsSniper 75PercentChance IsMapHelltower IsNotInHell
	Response SniperSF13BattleCry
}

Response SpySF13BattleCry
{
	scene	"scenes/player/spy/low/7606.vcd"
}
Rule SpySF13BattleCry
{
	criteria ConceptPlayerBattleCry IsSpy IsSpy IsSpy 75PercentChance IsMapHelltower IsNotInHell
	Response SpySF13BattleCry
}

Response SoldierSF13BattleCry
{
	scene	"scenes/player/soldier/low/7402.vcd"
	scene	"scenes/player/soldier/low/7405.vcd"
	scene	"scenes/player/soldier/low/7415.vcd"
	scene	"scenes/player/soldier/low/7417.vcd"
}
Rule SoldierSF13BattleCry
{
	criteria ConceptPlayerBattleCry IsSoldier IsSoldier IsSoldier 75PercentChance IsMapHelltower IsNotInHell
	Response SoldierSF13BattleCry
}

Response ScoutSF13SkeletonKingAppear
{
	scene	"scenes/player/scout/low/4485.vcd" predelay "0.5,1.2"
	scene	"scenes/player/scout/low/4486.vcd" predelay "0.5,1.2"
	scene	"scenes/player/scout/low/4696.vcd" predelay "0.5,1.2"
	scene	"scenes/player/scout/low/7001.vcd" predelay "0.5,1.2"
	scene	"scenes/player/scout/low/8525.vcd" predelay "0.5,1.2"
	scene	"scenes/player/scout/low/8526.vcd" predelay "0.5,1.2"
}
Rule ScoutSF13SkeletonKingAppear
{
	criteria ConceptPlayerSkeletonKingAppear 50PercentChance IsMapHelltower IsNotInHell
	Response ScoutSF13SkeletonKingAppear
}

Response DemomanSF13SkeletonKingAppear
{
	scene	"scenes/player/demoman/low/8541.vcd" predelay "0.5,1.2"
	scene	"scenes/player/demoman/low/8542.vcd" predelay "0.5,1.2"
	scene	"scenes/player/demoman/low/8543.vcd" predelay "0.5,1.2"
	scene	"scenes/player/demoman/low/8544.vcd" predelay "0.5,1.2"
	scene	"scenes/player/demoman/low/8545.vcd" predelay "0.5,1.2"
	scene	"scenes/player/demoman/low/8540.vcd" predelay "0.5,1.2"
}
Rule DemomanSF13SkeletonKingAppear
{
	criteria ConceptPlayerSkeletonKingAppear 50PercentChance IsMapHelltower IsNotInHell
	Response DemomanSF13SkeletonKingAppear
}

Response MedicSF13SkeletonKingAppear
{
	scene	"scenes/player/medic/low/4668.vcd" predelay "0.5,1.2"
	scene	"scenes/player/medic/low/4669.vcd" predelay "0.5,1.2"
	scene	"scenes/player/medic/low/6817.vcd" predelay "0.5,1.2"
}
Rule MedicSF13SkeletonKingAppear
{
	criteria ConceptPlayerSkeletonKingAppear 50PercentChance IsMapHelltower IsNotInHell
	Response MedicSF13SkeletonKingAppear
}

Response SniperSF13SkeletonKingAppear
{
	scene	"scenes/player/sniper/low/6564.vcd" predelay "0.5,1.2"
	scene	"scenes/player/sniper/low/8487.vcd" predelay "0.5,1.2"
	scene	"scenes/player/sniper/low/8488.vcd" predelay "0.5,1.2"
	scene	"scenes/player/sniper/low/8483.vcd" predelay "0.5,1.2"
}
Rule SniperSF13SkeletonKingAppear
{
	criteria ConceptPlayerSkeletonKingAppear 50PercentChance IsMapHelltower IsNotInHell
	Response SniperSF13SkeletonKingAppear
}

Response SpySF13SkeletonKingAppear
{
	scene	"scenes/player/spy/low/4718.vcd" predelay "0.5,1.2"
	scene	"scenes/player/spy/low/7524.vcd" predelay "0.5,1.2"
	scene	"scenes/player/spy/low/7634.vcd" predelay "0.5,1.2"
}
Rule SpySF13SkeletonKingAppear
{
	criteria ConceptPlayerSkeletonKingAppear 50PercentChance IsMapHelltower IsNotInHell
	Response SpySF13SkeletonKingAppear
}

Response SoldierSF13SkeletonKingAppear
{
	scene	"scenes/player/soldier/low/4568.vcd" predelay "0.5,1.2"
}
Rule SoldierSF13SkeletonKingAppear
{
	criteria ConceptPlayerSkeletonKingAppear 50PercentChance IsMapHelltower IsNotInHell
	Response SoldierSF13SkeletonKingAppear
}

Response ScoutSF13SpellMovementBuff
{
	scene	"scenes/player/scout/low/4468.vcd"
	scene	"scenes/player/scout/low/4473.vcd"
	scene	"scenes/player/scout/low/4475.vcd"
	scene	"scenes/player/scout/low/4687.vcd"
	scene	"scenes/player/scout/low/4688.vcd"
	scene	"scenes/player/scout/low/4692.vcd"
	scene	"scenes/player/scout/low/4693.vcd"
	scene	"scenes/player/scout/low/4694.vcd"
}
Rule ScoutSF13SpellMovementBuff
{
	criteria ConceptPlayerSpellMovementBuff IsScout
	Response ScoutSF13SpellMovementBuff
}

Response HeavySF13SpellMovementBuff
{
	scene	"scenes/player/heavy/low/4745.vcd"
	scene	"scenes/player/heavy/low/4751.vcd"
	scene	"scenes/player/heavy/low/4752.vcd"
	scene	"scenes/player/heavy/low/4750.vcd"
}
Rule HeavySF13SpellMovementBuff
{
	criteria ConceptPlayerSpellMovementBuff IsHeavy
	Response HeavySF13SpellMovementBuff
}

Response DemomanSF13SpellMovementBuff
{
	scene	"scenes/player/demoman/low/4587.vcd"
	scene	"scenes/player/demoman/low/4593.vcd"
	scene	"scenes/player/demoman/low/4586.vcd"
}
Rule DemomanSF13SpellMovementBuff
{
	criteria ConceptPlayerSpellMovementBuff IsDemoman
	Response DemomanSF13SpellMovementBuff
}

Response MedicSF13SpellMovementBuff
{
	scene	"scenes/player/medic/low/4659.vcd"
	scene	"scenes/player/medic/low/4663.vcd"
	scene	"scenes/player/medic/low/4652.vcd"
	scene	"scenes/player/medic/low/4654.vcd"
	scene	"scenes/player/medic/low/4657.vcd"
	scene	"scenes/player/medic/low/4660.vcd"
}
Rule MedicSF13SpellMovementBuff
{
	criteria ConceptPlayerSpellMovementBuff IsMedic
	Response MedicSF13SpellMovementBuff
}

Response SpySF13SpellMovementBuff
{
	scene	"scenes/player/spy/low/4717.vcd"
	scene	"scenes/player/spy/low/4711.vcd"
	scene	"scenes/player/spy/low/4712.vcd"
}
Rule SpySF13SpellMovementBuff
{
	criteria ConceptPlayerSpellMovementBuff IsSpy
	Response SpySF13SpellMovementBuff
}

Response SoldierSF13SpellMovementBuff
{
	scene	"scenes/player/soldier/low/4553.vcd"
	scene	"scenes/player/soldier/low/4546.vcd"
	scene	"scenes/player/soldier/low/4548.vcd"
	scene	"scenes/player/soldier/low/4550.vcd"
	scene	"scenes/player/soldier/low/4551.vcd"
}
Rule SoldierSF13SpellMovementBuff
{
	criteria ConceptPlayerSpellMovementBuff IsSoldier
	Response SoldierSF13SpellMovementBuff
}

//END OF SCREAM FORTRESS 2013 AUTOGENERATED



//------------------------------------------------------------------------------------------------------------------------
//A Tale of Two Cities 2013
//------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------
//CRITERIA
//------------------------------------------------------------------------------------------------------------------------
criterion "NotMannhattanGateAttackCooldown" "worldMannhattanGateAttackCooldown" "!=1" "required" weight 100
//------------------------------------------------------------------------------------------------------------------------

//START OF TALE OF TWO CITIES AUTOGENERATED
Response ScoutMannhattanGateAttack
{
	scene	"scenes/player/scout/low/6952.vcd" predelay "0.3,1.0"
	scene	"scenes/player/scout/low/6953.vcd" predelay "0.3,1.0"
	scene	"scenes/player/scout/low/6954.vcd" predelay "0.3,1.0"
}
Rule ScoutMannhattanGateAttack
{
	criteria ConceptMannhattanGateAttack NotMannhattanGateAttackCooldown IsScout
	applycontext "MannhattanGateAttackCooldown:1:30"
	applycontexttoworld
	Response ScoutMannhattanGateAttack
}

Response HeavyMannhattanGateAttack
{
	scene	"scenes/player/heavy/low/6655.vcd" predelay "0.3,1.0"
	scene	"scenes/player/heavy/low/6657.vcd" predelay "0.3,1.0"
}
Rule HeavyMannhattanGateAttack
{
	criteria ConceptMannhattanGateAttack NotMannhattanGateAttackCooldown IsHeavy
	applycontext "MannhattanGateAttackCooldown:1:30"
	applycontexttoworld
	Response HeavyMannhattanGateAttack
}

Response EngineerMannhattanGateAttack
{
	scene	"scenes/player/engineer/low/7866.vcd" predelay "0.3,1.0"
	scene	"scenes/player/engineer/low/7867.vcd" predelay "0.3,1.0"
	scene	"scenes/player/engineer/low/8469.vcd" predelay "0.3,1.0"
}
Rule EngineerMannhattanGateAttack
{
	criteria ConceptMannhattanGateAttack NotMannhattanGateAttackCooldown IsEngineer
	applycontext "MannhattanGateAttackCooldown:1:30"
	applycontexttoworld
	Response EngineerMannhattanGateAttack
}

Response DemomanMannhattanGateAttack
{
	scene	"scenes/player/demoman/low/7798.vcd" predelay "0.3,1.0"
	scene	"scenes/player/demoman/low/7799.vcd" predelay "0.3,1.0"
	scene	"scenes/player/demoman/low/7800.vcd" predelay "0.3,1.0"
}
Rule DemomanMannhattanGateAttack
{
	criteria ConceptMannhattanGateAttack NotMannhattanGateAttackCooldown IsDemoman
	applycontext "MannhattanGateAttackCooldown:1:30"
	applycontexttoworld
	Response DemomanMannhattanGateAttack
}

Response MedicMannhattanGateAttack
{
	scene	"scenes/player/medic/low/6748.vcd" predelay "0.3,1.0"
	scene	"scenes/player/medic/low/6749.vcd" predelay "0.3,1.0"
}
Rule MedicMannhattanGateAttack
{
	criteria ConceptMannhattanGateAttack NotMannhattanGateAttackCooldown IsMedic
	applycontext "MannhattanGateAttackCooldown:1:30"
	applycontexttoworld
	Response MedicMannhattanGateAttack
}

Response SniperMannhattanGateAttack
{
	scene	"scenes/player/sniper/low/7233.vcd" predelay "0.3,1.0"
	scene	"scenes/player/sniper/low/7234.vcd" predelay "0.3,1.0"
}
Rule SniperMannhattanGateAttack
{
	criteria ConceptMannhattanGateAttack NotMannhattanGateAttackCooldown IsSniper
	applycontext "MannhattanGateAttackCooldown:1:30"
	applycontexttoworld
	Response SniperMannhattanGateAttack
}

Response SpyMannhattanGateAttack
{
	scene	"scenes/player/spy/low/7569.vcd" predelay "0.3,1.0"
	scene	"scenes/player/spy/low/7570.vcd" predelay "0.3,1.0"
}
Rule SpyMannhattanGateAttack
{
	criteria ConceptMannhattanGateAttack NotMannhattanGateAttackCooldown IsSpy
	applycontext "MannhattanGateAttackCooldown:1:30"
	applycontexttoworld
	Response SpyMannhattanGateAttack
}

Response SoldierMannhattanGateAttack
{
	scene	"scenes/player/soldier/low/7341.vcd" predelay "0.3,1.0"
}
Rule SoldierMannhattanGateAttack
{
	criteria ConceptMannhattanGateAttack NotMannhattanGateAttackCooldown IsSoldier
	applycontext "MannhattanGateAttackCooldown:1:30"
	applycontexttoworld
	Response SoldierMannhattanGateAttack
}

Response ScoutMannhattanGateTake
{
	scene	"scenes/player/scout/low/6955.vcd" predelay "0.3,0.5"
	scene	"scenes/player/scout/low/6956.vcd" predelay "0.3,0.5"
}
Rule ScoutMannhattanGateTake
{
	criteria ConceptMannhattanGateTake IsScout
	Response ScoutMannhattanGateTake
}

Response HeavyMannhattanGateTake
{
	scene	"scenes/player/heavy/low/6658.vcd" predelay "0.3,0.5"
}
Rule HeavyMannhattanGateTake
{
	criteria ConceptMannhattanGateTake IsHeavy
	Response HeavyMannhattanGateTake
}

Response EngineerMannhattanGateTake
{
	scene	"scenes/player/engineer/low/7868.vcd" predelay "0.3,0.5"
}
Rule EngineerMannhattanGateTake
{
	criteria ConceptMannhattanGateTake IsEngineer
	Response EngineerMannhattanGateTake
}

Response DemomanMannhattanGateTake
{
	scene	"scenes/player/demoman/low/7801.vcd" predelay "0.3,0.5"
	scene	"scenes/player/demoman/low/7802.vcd" predelay "0.3,0.5"
}
Rule DemomanMannhattanGateTake
{
	criteria ConceptMannhattanGateTake IsDemoman
	Response DemomanMannhattanGateTake
}

Response MedicMannhattanGateTake
{
	scene	"scenes/player/medic/low/6751.vcd" predelay "0.3,0.5"
	scene	"scenes/player/medic/low/8504.vcd" predelay "0.3,0.5"
}
Rule MedicMannhattanGateTake
{
	criteria ConceptMannhattanGateTake IsMedic
	Response MedicMannhattanGateTake
}

Response SniperMannhattanGateTake
{
	scene	"scenes/player/sniper/low/7235.vcd" predelay "0.3,0.5"
}
Rule SniperMannhattanGateTake
{
	criteria ConceptMannhattanGateTake IsSniper
	Response SniperMannhattanGateTake
}

Response SpyMannhattanGateTake
{
	scene	"scenes/player/spy/low/7571.vcd" predelay "0.3,0.5"
}
Rule SpyMannhattanGateTake
{
	criteria ConceptMannhattanGateTake IsSpy
	Response SpyMannhattanGateTake
}

Response SoldierMannhattanGateTake
{
	scene	"scenes/player/soldier/low/7343.vcd" predelay "0.3,0.5"
}
Rule SoldierMannhattanGateTake
{
	criteria ConceptMannhattanGateTake IsSoldier
	Response SoldierMannhattanGateTake
}

Response ScoutMvmResurrected
{
	scene	"scenes/player/scout/low/6922.vcd"
	scene	"scenes/player/scout/low/6923.vcd"
	scene	"scenes/player/scout/low/6924.vcd"
	scene	"scenes/player/scout/low/6925.vcd"
	scene	"scenes/player/scout/low/6926.vcd"
	scene	"scenes/player/scout/low/6928.vcd"
	scene	"scenes/player/scout/low/6929.vcd"
	scene	"scenes/player/scout/low/6932.vcd"
}
Rule ScoutMvmResurrected
{
	criteria ConceptMvMResurrected IsScout
	Response ScoutMvmResurrected
}

Response HeavyMvmResurrected
{
	scene	"scenes/player/heavy/low/6631.vcd"
	scene	"scenes/player/heavy/low/6633.vcd"
	scene	"scenes/player/heavy/low/6634.vcd"
	scene	"scenes/player/heavy/low/6635.vcd"
	scene	"scenes/player/heavy/low/6636.vcd"
	scene	"scenes/player/heavy/low/8569.vcd"
	scene	"scenes/player/heavy/low/8489.vcd"
}
Rule HeavyMvmResurrected
{
	criteria ConceptMvMResurrected IsHeavy
	Response HeavyMvmResurrected
}

Response PyroMvmResurrected
{
	scene	"scenes/player/pyro/low/1482.vcd"
	scene	"scenes/player/pyro/low/1595.vcd"
}
Rule PyroMvmResurrected
{
	criteria ConceptMvMResurrected IsPyro
	Response PyroMvmResurrected
}

Response EngineerMvmResurrected
{
	scene	"scenes/player/engineer/low/7848.vcd"
	scene	"scenes/player/engineer/low/7850.vcd"
	scene	"scenes/player/engineer/low/7854.vcd"
}
Rule EngineerMvmResurrected
{
	criteria ConceptMvMResurrected IsEngineer
	Response EngineerMvmResurrected
}

Response DemomanMvmResurrected
{
	scene	"scenes/player/demoman/low/7691.vcd"
	scene	"scenes/player/demoman/low/7692.vcd"
	scene	"scenes/player/demoman/low/7693.vcd"
	scene	"scenes/player/demoman/low/7694.vcd"
	scene	"scenes/player/demoman/low/7695.vcd"
	scene	"scenes/player/demoman/low/7696.vcd"
	scene	"scenes/player/demoman/low/7697.vcd"
	scene	"scenes/player/demoman/low/7698.vcd"
	scene	"scenes/player/demoman/low/7699.vcd"
	scene	"scenes/player/demoman/low/7701.vcd"
	scene	"scenes/player/demoman/low/7702.vcd"
}
Rule DemomanMvmResurrected
{
	criteria ConceptMvMResurrected IsDemoman
	Response DemomanMvmResurrected
}

Response MedicMvmResurrected
{
	scene	"scenes/player/medic/low/6719.vcd"
	scene	"scenes/player/medic/low/6720.vcd"
	scene	"scenes/player/medic/low/6723.vcd"
}
Rule MedicMvmResurrected
{
	criteria ConceptMvMResurrected IsMedic
	Response MedicMvmResurrected
}

Response SniperMvmResurrected
{
	scene	"scenes/player/sniper/low/7238.vcd"
	scene	"scenes/player/sniper/low/7239.vcd"
	scene	"scenes/player/sniper/low/7240.vcd"
	scene	"scenes/player/sniper/low/7242.vcd"
}
Rule SniperMvmResurrected
{
	criteria ConceptMvMResurrected IsSniper
	Response SniperMvmResurrected
}

Response SpyMvmResurrected
{
	scene	"scenes/player/spy/low/7526.vcd"
	scene	"scenes/player/spy/low/7527.vcd"
	scene	"scenes/player/spy/low/7528.vcd"
	scene	"scenes/player/spy/low/7529.vcd"
	scene	"scenes/player/spy/low/7530.vcd"
	scene	"scenes/player/spy/low/7531.vcd"
	scene	"scenes/player/spy/low/7532.vcd"
	scene	"scenes/player/spy/low/7533.vcd"
	scene	"scenes/player/spy/low/7535.vcd"
}
Rule SpyMvmResurrected
{
	criteria ConceptMvMResurrected IsSpy
	Response SpyMvmResurrected
}

Response SoldierMvmResurrected
{
	scene	"scenes/player/soldier/low/7319.vcd"
	scene	"scenes/player/soldier/low/7320.vcd"
	scene	"scenes/player/soldier/low/7323.vcd"
	scene	"scenes/player/soldier/low/7324.vcd"
	scene	"scenes/player/soldier/low/7325.vcd"
	scene	"scenes/player/soldier/low/7326.vcd"
}
Rule SoldierMvmResurrected
{
	criteria ConceptMvMResurrected IsSoldier
	Response SoldierMvmResurrected
}

Response MedicMvMMedicShield
{
	scene	"scenes/player/medic/low/6730.vcd"
	scene	"scenes/player/medic/low/6731.vcd"
	scene	"scenes/player/medic/low/6732.vcd"
	scene	"scenes/player/medic/low/6733.vcd"
	scene	"scenes/player/medic/low/6734.vcd"
}
Rule MedicMvMMedicShield
{
	criteria ConceptMvMMedicShield IsMedic 75PercentChance
	Response MedicMvMMedicShield
}

Response ScoutMvMLootCommon
{
	scene	"scenes/player/scout/low/6934.vcd" predelay "0.2,0.4"
	scene	"scenes/player/scout/low/6935.vcd" predelay "0.2,0.4"
	scene	"scenes/player/scout/low/6936.vcd" predelay "0.2,0.4"
	scene	"scenes/player/scout/low/6938.vcd" predelay "0.2,0.4"
	scene	"scenes/player/scout/low/8568.vcd" predelay "0.2,0.4"
	scene	"scenes/player/scout/low/8511.vcd" predelay "0.2,0.4"
}
Rule ScoutMvMLootCommon
{
	criteria ConceptMvMLootCommon IsScout 20PercentChance
	Response ScoutMvMLootCommon
}

Response HeavyMvMLootCommon
{
	scene	"scenes/player/heavy/low/6643.vcd" predelay "0.2,0.4"
	scene	"scenes/player/heavy/low/8490.vcd" predelay "0.2,0.4"
}
Rule HeavyMvMLootCommon
{
	criteria ConceptMvMLootCommon IsHeavy 20PercentChance
	Response HeavyMvMLootCommon
}

Response PyroMvMLootCommon
{
	scene	"scenes/player/pyro/low/1454.vcd" predelay "0.2,0.4"
}
Rule PyroMvMLootCommon
{
	criteria ConceptMvMLootCommon IsPyro 20PercentChance
	Response PyroMvMLootCommon
}

Response EngineerMvMLootCommon
{
	scene	"scenes/player/engineer/low/7855.vcd" predelay "0.2,0.4"
	scene	"scenes/player/engineer/low/7857.vcd" predelay "0.2,0.4"
}
Rule EngineerMvMLootCommon
{
	criteria ConceptMvMLootCommon IsEngineer 20PercentChance
	Response EngineerMvMLootCommon
}

Response DemomanMvMLootCommon
{
	scene	"scenes/player/demoman/low/7791.vcd" predelay "0.2,0.4"
	scene	"scenes/player/demoman/low/7792.vcd" predelay "0.2,0.4"
	scene	"scenes/player/demoman/low/7793.vcd" predelay "0.2,0.4"
	scene	"scenes/player/demoman/low/7794.vcd" predelay "0.2,0.4"
}
Rule DemomanMvMLootCommon
{
	criteria ConceptMvMLootCommon IsDemoman 20PercentChance
	Response DemomanMvMLootCommon
}

Response MedicMvMLootCommon
{
	scene	"scenes/player/medic/low/6735.vcd" predelay "0.2,0.4"
	scene	"scenes/player/medic/low/6736.vcd" predelay "0.2,0.4"
	scene	"scenes/player/medic/low/6738.vcd" predelay "0.2,0.4"
}
Rule MedicMvMLootCommon
{
	criteria ConceptMvMLootCommon IsMedic 20PercentChance
	Response MedicMvMLootCommon
}

Response SniperMvMLootCommon
{
	scene	"scenes/player/sniper/low/7211.vcd" predelay "0.2,0.4"
	scene	"scenes/player/sniper/low/7212.vcd" predelay "0.2,0.4"
	scene	"scenes/player/sniper/low/7213.vcd" predelay "0.2,0.4"
	scene	"scenes/player/sniper/low/7214.vcd" predelay "0.2,0.4"
	scene	"scenes/player/sniper/low/7215.vcd" predelay "0.2,0.4"
	scene	"scenes/player/sniper/low/7216.vcd" predelay "0.2,0.4"
}
Rule SniperMvMLootCommon
{
	criteria ConceptMvMLootCommon IsSniper 20PercentChance
	Response SniperMvMLootCommon
}

Response SpyMvMLootCommon
{
	scene	"scenes/player/spy/low/7513.vcd" predelay "0.2,0.4"
	scene	"scenes/player/spy/low/7514.vcd" predelay "0.2,0.4"
	scene	"scenes/player/spy/low/8547.vcd" predelay "0.2,0.4"
}
Rule SpyMvMLootCommon
{
	criteria ConceptMvMLootCommon IsSpy 20PercentChance
	Response SpyMvMLootCommon
}

Response SoldierMvMLootCommon
{
	scene	"scenes/player/soldier/low/7329.vcd" predelay "0.2,0.4"
	scene	"scenes/player/soldier/low/7330.vcd" predelay "0.2,0.4"
	scene	"scenes/player/soldier/low/7331.vcd" predelay "0.2,0.4"
}
Rule SoldierMvMLootCommon
{
	criteria ConceptMvMLootCommon IsSoldier 20PercentChance
	Response SoldierMvMLootCommon
}

Response ScoutMvMLootRare
{
	scene	"scenes/player/scout/low/6942.vcd" predelay "0.5,0.8"
	scene	"scenes/player/scout/low/6943.vcd" predelay "0.5,0.8"
	scene	"scenes/player/scout/low/6944.vcd" predelay "0.5,0.8"
	scene	"scenes/player/scout/low/6945.vcd" predelay "0.5,0.8"
	scene	"scenes/player/scout/low/6947.vcd" predelay "0.5,0.8"
	scene	"scenes/player/scout/low/6949.vcd" predelay "0.5,0.8"
	scene	"scenes/player/scout/low/6950.vcd" predelay "0.5,0.8"
	scene	"scenes/player/scout/low/8512.vcd" predelay "0.5,0.8"
}
Rule ScoutMvMLootRare
{
	criteria ConceptMvMLootRare IsScout
	Response ScoutMvMLootRare
}

Response HeavyMvMLootRare
{
	scene	"scenes/player/heavy/low/6644.vcd" predelay "0.5,0.8"
	scene	"scenes/player/heavy/low/6645.vcd" predelay "0.5,0.8"
	scene	"scenes/player/heavy/low/6646.vcd" predelay "0.5,0.8"
}
Rule HeavyMvMLootRare
{
	criteria ConceptMvMLootRare IsHeavy
	Response HeavyMvMLootRare
}

Response PyroMvMLootRare
{
	scene	"scenes/player/pyro/low/1510.vcd" predelay "0.5,0.8"
}
Rule PyroMvMLootRare
{
	criteria ConceptMvMLootRare IsPyro
	Response PyroMvMLootRare
}

Response EngineerMvMLootRare
{
	scene	"scenes/player/engineer/low/7859.vcd" predelay "0.5,0.8"
	scene	"scenes/player/engineer/low/7860.vcd" predelay "0.5,0.8"
	scene	"scenes/player/engineer/low/8467.vcd" predelay "0.5,0.8"
	scene	"scenes/player/engineer/low/8468.vcd" predelay "0.5,0.8"
}
Rule EngineerMvMLootRare
{
	criteria ConceptMvMLootRare IsEngineer
	Response EngineerMvMLootRare
}

Response DemomanMvMLootRare
{
	scene	"scenes/player/demoman/low/7795.vcd" predelay "0.5,0.8"
	scene	"scenes/player/demoman/low/7796.vcd" predelay "0.5,0.8"
}
Rule DemomanMvMLootRare
{
	criteria ConceptMvMLootRare IsDemoman
	Response DemomanMvMLootRare
}

Response MedicMvMLootRare
{
	scene	"scenes/player/medic/low/6739.vcd" predelay "0.5,0.8"
	scene	"scenes/player/medic/low/6740.vcd" predelay "0.5,0.8"
}
Rule MedicMvMLootRare
{
	criteria ConceptMvMLootRare IsMedic
	Response MedicMvMLootRare
}

Response SniperMvMLootRare
{
	scene	"scenes/player/sniper/low/7217.vcd" predelay "0.5,0.8"
	scene	"scenes/player/sniper/low/7218.vcd" predelay "0.5,0.8"
	scene	"scenes/player/sniper/low/7219.vcd" predelay "0.5,0.8"
	scene	"scenes/player/sniper/low/7220.vcd" predelay "0.5,0.8"
	scene	"scenes/player/sniper/low/7221.vcd" predelay "0.5,0.8"
	scene	"scenes/player/sniper/low/7222.vcd" predelay "0.5,0.8"
	scene	"scenes/player/sniper/low/7223.vcd" predelay "0.5,0.8"
	scene	"scenes/player/sniper/low/7224.vcd" predelay "0.5,0.8"
}
Rule SniperMvMLootRare
{
	criteria ConceptMvMLootRare IsSniper
	Response SniperMvMLootRare
}

Response SpyMvMLootRare
{
	scene	"scenes/player/spy/low/7515.vcd" predelay "0.5,0.8"
	scene	"scenes/player/spy/low/7516.vcd" predelay "0.5,0.8"
	scene	"scenes/player/spy/low/8546.vcd" predelay "0.5,0.8"
}
Rule SpyMvMLootRare
{
	criteria ConceptMvMLootRare IsSpy
	Response SpyMvMLootRare
}

Response SoldierMvMLootRare
{
	scene	"scenes/player/soldier/low/7332.vcd" predelay "0.5,0.8"
	scene	"scenes/player/soldier/low/7333.vcd" predelay "0.5,0.8"
	scene	"scenes/player/soldier/low/7334.vcd" predelay "0.5,0.8"
	scene	"scenes/player/soldier/low/7335.vcd" predelay "0.5,0.8"
}
Rule SoldierMvMLootRare
{
	criteria ConceptMvMLootRare IsSoldier
	Response SoldierMvMLootRare
}

Response ScoutMvMLootUltraRare
{
	scene	"scenes/player/scout/low/6951.vcd" predelay "0.5,0.8"
	scene	"scenes/player/scout/low/8513.vcd" predelay "0.5,0.8"
}
Rule ScoutMvMLootUltraRare
{
	criteria ConceptMvMLootUltraRare IsScout
	Response ScoutMvMLootUltraRare
}

Response HeavyMvMLootUltraRare
{
	scene	"scenes/player/heavy/low/6651.vcd" predelay "0.5,0.8"
	scene	"scenes/player/heavy/low/6654.vcd" predelay "0.5,0.8"
	scene	"scenes/player/heavy/low/8491.vcd" predelay "0.5,0.8"
}
Rule HeavyMvMLootUltraRare
{
	criteria ConceptMvMLootUltraRare IsHeavy
	Response HeavyMvMLootUltraRare
}

Response PyroMvMLootUltraRare
{
	scene	"scenes/player/pyro/low/1483.vcd" predelay "0.5,0.8"
}
Rule PyroMvMLootUltraRare
{
	criteria ConceptMvMLootUltraRare IsPyro
	Response PyroMvMLootUltraRare
}

Response EngineerMvMLootUltraRare
{
	scene	"scenes/player/engineer/low/7862.vcd" predelay "0.5,0.8"
	scene	"scenes/player/engineer/low/7863.vcd" predelay "0.5,0.8"
	scene	"scenes/player/engineer/low/8466.vcd" predelay "0.5,0.8"
}
Rule EngineerMvMLootUltraRare
{
	criteria ConceptMvMLootUltraRare IsEngineer
	Response EngineerMvMLootUltraRare
}

Response DemomanMvMLootUltraRare
{
	scene	"scenes/player/demoman/low/7797.vcd" predelay "0.5,0.8"
}
Rule DemomanMvMLootUltraRare
{
	criteria ConceptMvMLootUltraRare IsDemoman
	Response DemomanMvMLootUltraRare
}

Response MedicMvMLootUltraRare
{
	scene	"scenes/player/medic/low/6743.vcd" predelay "0.5,0.8"
	scene	"scenes/player/medic/low/6744.vcd" predelay "0.5,0.8"
	scene	"scenes/player/medic/low/8503.vcd" predelay "0.5,0.8"
}
Rule MedicMvMLootUltraRare
{
	criteria ConceptMvMLootUltraRare IsMedic
	Response MedicMvMLootUltraRare
}

Response SniperMvMLootUltraRare
{
	scene	"scenes/player/sniper/low/7226.vcd" predelay "0.5,0.8"
	scene	"scenes/player/sniper/low/7228.vcd" predelay "0.5,0.8"
}
Rule SniperMvMLootUltraRare
{
	criteria ConceptMvMLootUltraRare IsSniper
	Response SniperMvMLootUltraRare
}

Response SpyMvMLootUltraRare
{
	scene	"scenes/player/spy/low/7517.vcd" predelay "0.5,0.8"
	scene	"scenes/player/spy/low/7518.vcd" predelay "0.5,0.8"
	scene	"scenes/player/spy/low/7519.vcd" predelay "0.5,0.8"
}
Rule SpyMvMLootUltraRare
{
	criteria ConceptMvMLootUltraRare IsSpy
	Response SpyMvMLootUltraRare
}

Response SoldierMvMLootUltraRare
{
	scene	"scenes/player/soldier/low/7336.vcd" predelay "0.5,0.8"
	scene	"scenes/player/soldier/low/7337.vcd" predelay "0.5,0.8"
	scene	"scenes/player/soldier/low/7340.vcd" predelay "0.5,0.8"
}
Rule SoldierMvMLootUltraRare
{
	criteria ConceptMvMLootUltraRare IsSoldier
	Response SoldierMvMLootUltraRare
}

//END OF TALE OF TWO CITIES AUTOGENERATED
