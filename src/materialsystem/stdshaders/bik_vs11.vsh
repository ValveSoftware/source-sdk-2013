vs.1.1

#include "macros.vsh"

&AllocateRegister( \$projPos );

dp4 $projPos.x, $vPos, $cModelViewProj0
dp4 $projPos.y, $vPos, $cModelViewProj1
dp4 $projPos.z, $vPos, $cModelViewProj2
dp4 $projPos.w, $vPos, $cModelViewProj3
mov oPos, $projPos

mov oT0, $vTexCoord0
mov oT1, $vTexCoord0
mov oT2, $vTexCoord0
mov oT3, $vTexCoord0

&FreeRegister( \$projPos );

