vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending
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

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

; Compute the texture coordinates given the offset between
; each bumped lightmap
&AllocateRegister( \$offset );
mov $offset.xy, $vTexCoord2
dp4 oT0.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_0
dp4 oT0.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_1
add oT1.xy, $offset, $vTexCoord1
mad oT2.xy, $offset, $cTwo, $vTexCoord1
alloc $three
add $three, $cOne, $cTwo
mad oT3.xy, $offset, $three, $vTexCoord1
free $three
dp4 oT4.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_2
dp4 oT4.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_3

&FreeRegister( \$offset );
