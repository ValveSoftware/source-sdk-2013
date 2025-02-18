vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"

#include "macros.vsh"

;------------------------------------------------------------------------------
;	 $SHADER_SPECIFIC_CONST_0-$SHADER_SPECIFIC_CONST_1 = Base texture transform
;    $SHADER_SPECIFIC_CONST_2-$SHADER_SPECIFIC_CONST_3 = Mask texture transform
;	 $SHADER_SPECIFIC_CONST_4	 = Modulation color 
;------------------------------------------------------------------------------

&AllocateRegister( \$projPos );

dp4 $projPos.x, $vPos, $cModelViewProj0
dp4 $projPos.y, $vPos, $cModelViewProj1
dp4 $projPos.z, $vPos, $cModelViewProj2
dp4 $projPos.w, $vPos, $cModelViewProj3
mov oPos, $projPos

&FreeRegister( \$projPos );

