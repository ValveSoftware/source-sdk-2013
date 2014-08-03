vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"
# DYNAMIC: "SKINNING"				"0..1"

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending 
;------------------------------------------------------------------------------
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

&FreeRegister( \$worldPos );

; stick the normal in the color channel
mov oD0.xyz, $worldNormal.xyz
mov oD0.w, $cOne				; make sure all components are defined

&FreeRegister( \$projPos );
&FreeRegister( \$worldNormal );
