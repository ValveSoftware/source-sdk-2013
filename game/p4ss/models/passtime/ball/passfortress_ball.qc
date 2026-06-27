// Created by Crowbar 0.74

$modelname "passtime/ball/passfortress_ball.mdl"

$bodygroup "passfortress_ball"
{
	studio "passfortress_ball.dmx"
}

$surfaceprop "passtime_ball"

$contents "solid"

$illumposition 0 0 0

$cdmaterials "models\passtime\ball\"

$texturegroup "skinfamilies"
{
	{ "passfortress_ball_red" }
	{ "passfortress_ball_blu" }
	{ "passfortress_ball_neu" }
}

$attachment "trail1" "weapon_bone" 8 0 0 rotate 0 0 0
$attachment "trail2" "weapon_bone" -8 0 0 rotate 0 0 0
$attachment "trail3" "weapon_bone" 0 8 0 rotate 0 0 0
$attachment "trail4" "weapon_bone" 0 -8 0 rotate 0 0 0

$cbox 0 0 0 0 0 0

$bbox -8.848 -9.645 -9.416 8.889 9.718 9.416

$definebone "weapon_bone" "" 0 0 0 0 0 0 0 0 0 0 0 0


$sequence "idle" {
	"passfortress_ball.dmx"
	fadein 0.2
	fadeout 0.2
	fps 30
}

$collisionmodel "passfortress_ball.dmx"
{
	$mass "5.5"
	$inertia 1
	$damping 0
	$rotdamping 0
	$rootbone " "
	$concave	1
	$maxconvexpieces 50

}
