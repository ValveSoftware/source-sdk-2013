vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending
;------------------------------------------------------------------------------

&AllocateRegister( \$projPos );

dp4 $projPos.x, $vPos, $cModelViewProj0
dp4 $projPos.y, $vPos, $cModelViewProj1
dp4 $projPos.z, $vPos, $cModelViewProj2
dp4 $projPos.w, $vPos, $cModelViewProj3
mov oPos, $projPos

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

dp4 oT0.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_0
dp4 oT0.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_1

dp4 oT1.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_2
dp4 oT1.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_3

mov oT2, $vTexCoord1

; Now the basetexture/basetexture2 blend uses vertex color, so send it into the psh.
mov oD0, $vColor
