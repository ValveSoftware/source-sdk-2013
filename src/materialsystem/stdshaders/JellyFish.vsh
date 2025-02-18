vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"
# DYNAMIC: "SKINNING"				"0..1"

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending
;------------------------------------------------------------------------------

$cView0 = $SHADER_SPECIFIC_CONST_0;
$cView1 = $SHADER_SPECIFIC_CONST_1;
$cView2 = $SHADER_SPECIFIC_CONST_2;

&AllocateRegister( \$worldPos );
&AllocateRegister( \$worldNormal );
&SkinPositionAndNormal( $worldPos, $worldNormal );

;------------------------------------------------------------------------------
; Transform the position from world to view space
;------------------------------------------------------------------------------

&AllocateRegister( \$projPos );

dp4 $projPos.x, $worldPos, $cViewProj0
dp4 $projPos.y, $worldPos, $cViewProj1
dp4 $projPos.z, $worldPos, $cViewProj2
dp4 $projPos.w, $worldPos, $cViewProj3
mov oPos, $projPos

;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------

&CalcFog( $worldPos, $projPos );

&FreeRegister( \$projPos );

&AllocateRegister( \$viewNormal );

; Transform the normal from object to view space
dp3 $viewNormal.x, $worldNormal, $cView0
dp3 $viewNormal.y, $worldNormal, $cView1
dp3 $viewNormal.z, $worldNormal, $cView2

&FreeRegister( \$worldNormal );
; normalize normal (do we need to do this?)
&Normalize( $viewNormal );


&AllocateRegister( \$viewPos );

; Transform position from object to view space
dp4 $viewPos.x, $worldPos, $cView0
dp4 $viewPos.y, $worldPos, $cView1
dp4 $viewPos.z, $worldPos, $cView2

&FreeRegister( \$worldPos );
&AllocateRegister( \$vertToEye );

; vector from point to eye in view space
mov $vertToEye.xyz, -$viewPos

&FreeRegister( \$viewPos );

; normalize
&Normalize( $vertToEye );

dp3 $viewNormal.x, $vertToEye, $viewNormal
add $viewNormal.x, $viewNormal.x, $SHADER_SPECIFIC_CONST_5 ; FIXME

&FreeRegister( \$vertToEye );

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

mov oT0, $viewNormal
mov oT1, $vTexCoord0

&FreeRegister( \$viewNormal );
