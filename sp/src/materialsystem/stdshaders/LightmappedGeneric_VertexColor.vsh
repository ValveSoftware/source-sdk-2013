vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"

#include "LightmappedGeneric_inc.vsh"

$detail = 0;
$envmap = 0;
$envmapcameraspace = 0;
$envmapsphere = 0;
$vertexcolor = 1;

&LightmappedGeneric( $detail, $envmap, $envmapcameraspace, $envmapsphere, 
				$vertexcolor );

