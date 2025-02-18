vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"

#include "macros.vsh"

local( $worldPos, $worldNormal, $projPos, $reflectionVector );

alloc $projPos

dp4 $projPos.x, $vPos, $cModelViewProj0
dp4 $projPos.y, $vPos, $cModelViewProj1
dp4 $projPos.z, $vPos, $cModelViewProj2
dp4 $projPos.w, $vPos, $cModelViewProj3
mov oPos, $projPos

alloc $worldPos
alloc $worldNormal

dp4 $worldPos.x, $vPos, $cModel0
dp4 $worldPos.y, $vPos, $cModel1
dp4 $worldPos.z, $vPos, $cModel2

dp3 $worldNormal.x, $vNormal, $cModel0
dp3 $worldNormal.y, $vNormal, $cModel1
dp3 $worldNormal.z, $vNormal, $cModel2
 
&CalcFog( $worldPos, $projPos );

free $projPos

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------
; base texcoords
alloc $texcoord
mul $texcoord.xyz, $worldPos, $SHADER_SPECIFIC_CONST_0

mov oT0.xy, $texcoord.zy;
mov oT1.xy, $texcoord.xz;
mov oT2.xy, $texcoord.xy;

free $texcoord

; lightmap texcoords
mov oT3, $vTexCoord1

mul oD0.rgb, $worldNormal, $worldNormal

; Now the basetexture/basetexture2 blend uses vertex color, so send it into the psh.
mov oD0.a, $vColor

free $worldPos
free $worldNormal
