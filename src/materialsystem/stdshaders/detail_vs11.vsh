vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending
;------------------------------------------------------------------------------
&AllocateRegister( \$worldPos );
dp4 $worldPos.x, $vPos, $cModel0
dp4 $worldPos.y, $vPos, $cModel1
dp4 $worldPos.z, $vPos, $cModel2
mov $worldPos.w, $cOne


;------------------------------------------------------------------------------
; Transform the position from world to proj space
;------------------------------------------------------------------------------
&AllocateRegister( \$projPos );
dp4 $projPos.x, $vPos, $cModelViewProj0
dp4 $projPos.y, $vPos, $cModelViewProj1
dp4 $projPos.z, $vPos, $cModelViewProj2
dp4 $projPos.w, $vPos, $cModelViewProj3
mov oPos, $projPos


;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------
&CalcFog( $worldPos, $projPos );
&FreeRegister( \$worldPos );


;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------
mov oT0.xy, $vTexCoord0.xy

; special case perspective correct texture projection so that the texture fits exactly on the screen
mul $projPos.y, $projPos.y, $SHADER_SPECIFIC_CONST_0.w
add $projPos.xy, $projPos.xy, $projPos.w
mul $projPos.xy, $projPos.xy, $cHalf
mul $projPos.xy, $projPos.xy, $SHADER_SPECIFIC_CONST_0.xy
mad $projPos.xy, $projPos.w, $SHADER_SPECIFIC_CONST_1.xy, $projPos.xy
 
mov oT1.xy, $projPos.xy
mov oT1.z, $projPos.w
mov oT1.w, $projPos.w

&FreeRegister( \$projPos );

;------------------------------------------------------------------------------
; Modulation color
;------------------------------------------------------------------------------
mov oD0, $vColor
