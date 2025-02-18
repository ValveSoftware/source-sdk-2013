vs.1.1

# DYNAMIC: "DOWATERFOG"	"0..0"

#include "macros.vsh"

&AllocateRegister( \$projPos );

dp4 $projPos.x, $vPos, $cModelViewProj0
dp4 $projPos.y, $vPos, $cModelViewProj1
dp4 $projPos.z, $vPos, $cModelViewProj2
dp4 $projPos.w, $vPos, $cModelViewProj3
mov oPos, $projPos

&AllocateRegister( \$worldPos );
; $worldPos unused, for above water, range fog calcs
&CalcFog( $worldPos, $projPos );

&FreeRegister( \$projPos );
&FreeRegister( \$worldPos );

dp4 oT0.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_0
dp4 oT0.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_1
dp4 oT1.x, $vTexCoord1, $SHADER_SPECIFIC_CONST_2
dp4 oT1.y, $vTexCoord1, $SHADER_SPECIFIC_CONST_3

