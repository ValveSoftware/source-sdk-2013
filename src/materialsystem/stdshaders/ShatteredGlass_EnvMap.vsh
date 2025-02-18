vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"

#include "ShatteredGlass_inc.vsh"

$detail = 1;
$envmap = 1;
$envmapcameraspace = 0;
$envmapsphere = 0;
$vertexcolor = 0;

&ShatteredGlass( $detail, $envmap, $envmapcameraspace, $envmapsphere, 
				$vertexcolor );

