vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"

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
; No vertex blending required. Input vertex data is in screen space
;------------------------------------------------------------------------------
mov oPos.xyz, $vPos.xyz
mov oPos.w, $cOne

;------------------------------------------------------------------------------
; Pass any and all texture coordinates through
;------------------------------------------------------------------------------
mov	oT0, $vTexCoord0
