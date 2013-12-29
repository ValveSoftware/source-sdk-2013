vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"

#include "macros.vsh"

local( $worldPos, $worldNormal, $projPos, $reflectionVector );

&AllocateRegister( \$projPos );

dp4 $projPos.x, $vPos, $cModelViewProj0
dp4 $projPos.y, $vPos, $cModelViewProj1
dp4 $projPos.z, $vPos, $cModelViewProj2
dp4 $projPos.w, $vPos, $cModelViewProj3
mov oPos, $projPos

&AllocateRegister( \$worldPos );

; garymcthack
dp4 $worldPos.z, $vPos, $cModel2

&CalcFog( $worldPos, $projPos );

&FreeRegister( \$worldPos );
&FreeRegister( \$projPos );

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------
; base texcoords
dp4 oT0.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_0
dp4 oT0.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_1

dp4 oT1.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_2
dp4 oT1.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_3

; lightmap texcoords
mov oT2, $vTexCoord1

; detail
dp4 oT3.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_4
dp4 oT3.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_5

; mask
dp4 oT4.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_6
dp4 oT4.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_7

; Now the basetexture/basetexture2 blend uses vertex color, so send it into the psh.
mov oD0, $vColor

&FreeRegister( \$worldPos ); # garymcthack

