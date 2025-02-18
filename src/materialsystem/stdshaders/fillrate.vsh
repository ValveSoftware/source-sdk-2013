vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"
# DYNAMIC: "SKINNING"				"0..1"

;------------------------------------------------------------------------------
; Constants specified by the app
;    c0      = (0, 1, 2, 0.5)
;	 c1		 = (1/2.2, 3, 255, overbright factor)
;    c2      = camera position *in world space*
;    c4-c7   = modelViewProj matrix	(transpose)
;    c8-c11  = ViewProj matrix (transpose)
;    c12-c15 = model->view matrix (transpose)
;	 c16	 = [fogStart, fogEnd, fogRange, 1.0/fogRange]
;	 $SHADER_SPECIFIC_CONST_0-$SHADER_SPECIFIC_CONST_1 = Base texture transform
;    $SHADER_SPECIFIC_CONST_2-$SHADER_SPECIFIC_CONST_3 = Mask texture transform
;------------------------------------------------------------------------------

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending
;------------------------------------------------------------------------------

&AllocateRegister( \$worldPos );
&SkinPosition( $worldPos );

; Transform the position from world to view space
dp4 oPos.x, $worldPos, $cViewProj0
dp4 oPos.y, $worldPos, $cViewProj1
dp4 oPos.z, $worldPos, $cViewProj2
dp4 oPos.w, $worldPos, $cViewProj3


&FreeRegister( \$worldPos );
