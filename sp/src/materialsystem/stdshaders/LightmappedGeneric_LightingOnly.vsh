vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"

#include "macros.vsh"

;------------------------------------------------------------------------------
;	 $SHADER_SPECIFIC_CONST_0-$SHADER_SPECIFIC_CONST_1 = Base texture transform
;    $SHADER_SPECIFIC_CONST_2-$SHADER_SPECIFIC_CONST_3 = Mask texture transform
;	 $SHADER_SPECIFIC_CONST_4	 = Modulation color 
;------------------------------------------------------------------------------

&AllocateRegister( \$projPos );

; Transform position from object to projection space
dp4 $projPos.x, $vPos, $cModelViewProj0
dp4 $projPos.y, $vPos, $cModelViewProj1
dp4 $projPos.z, $vPos, $cModelViewProj2
dp4 $projPos.w, $vPos, $cModelViewProj3

mov oPos, $projPos

;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------

alloc $worldPos
if( $DOWATERFOG == 1 )
{
	; Get the worldpos z component only since that's all we need for height fog
	dp4 $worldPos.z, $vPos, $cModel2
}
&CalcFog( $worldPos, $projPos );
free $worldPos

&FreeRegister( \$projPos );

; YUCK!  This is to make texcoords continuous for mat_softwaretl
mov oT0, $cZero
; Texture coordinates
mov oT1, $vTexCoord1

mov oD0, $cOne


