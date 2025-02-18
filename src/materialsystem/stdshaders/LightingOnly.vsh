vs.1.1

# STATIC: "HALF_LAMBERT"			"0..1"
# DYNAMIC: "DOWATERFOG"				"0..1"
# DYNAMIC: "LIGHT_COMBO"			"0..21"
# DYNAMIC: "SKINNING"				"0..1"

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending 
;------------------------------------------------------------------------------
&AllocateRegister( \$worldPos );
&AllocateRegister( \$worldNormal );
&SkinPositionAndNormal( $worldPos, $worldNormal );

;------------------------------------------------------------------------------
; Transform the position from model to proj
;------------------------------------------------------------------------------

&AllocateRegister( \$projPos );

dp4 $projPos.x, $worldPos, $cViewProj0
dp4 $projPos.y, $worldPos, $cViewProj1
dp4 $projPos.z, $worldPos, $cViewProj2
dp4 $projPos.w, $worldPos, $cViewProj3
mov oPos, $projPos

&CalcFog( $worldPos, $projPos );

&FreeRegister( \$projPos );

;------------------------------------------------------------------------------
; Lighting
;------------------------------------------------------------------------------
&DoLighting( $worldPos, $worldNormal );

&FreeRegister( \$worldPos );
&FreeRegister( \$worldNormal );

