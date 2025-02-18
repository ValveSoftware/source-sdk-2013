vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"

;------------------------------------------------------------------------------
; Shader specific constant:
;	 $SHADER_SPECIFIC_CONST_5	 = [sOffset, tOffset, 0, 0]
;------------------------------------------------------------------------------

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending
;------------------------------------------------------------------------------

&AllocateRegister( \$worldPos );

; Transform position from object to world
dp4 $worldPos.x, $vPos, $cModel0
dp4 $worldPos.y, $vPos, $cModel1
dp4 $worldPos.z, $vPos, $cModel2

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
&CalcFog( $worldPos, $projPos );

&FreeRegister( \$projPos );

;------------------------------------------------------------------------------
; Lighting
;------------------------------------------------------------------------------

; Transform tangent space basis vectors to env map space (world space)
; This will produce a set of vectors mapping from tangent space to env space
; We'll use this to transform normals from the normal map from tangent space
; to environment map space. 
; NOTE: use dp3 here since the basis vectors are vectors, not points

dp3 oT1.x, $vTangentS, $cModel0
dp3 oT2.x, $vTangentS, $cModel1
dp3 oT3.x, $vTangentS, $cModel2

dp3 oT1.y, $vTangentT, $cModel0
dp3 oT2.y, $vTangentT, $cModel1
dp3 oT3.y, $vTangentT, $cModel2

dp3 oT1.z, $vNormal, $cModel0
dp3 oT2.z, $vNormal, $cModel1
dp3 oT3.z, $vNormal, $cModel2
 
; Compute the vector from vertex to camera
&AllocateRegister( \$worldEyeVect );
sub $worldEyeVect.xyz, $cEyePos, $worldPos
&FreeRegister( \$worldPos );

; Move it into the w component of the texture coords, as the wacky
; pixel shader wants it there.
mov oT1.w, $worldEyeVect.x
mov oT2.w, $worldEyeVect.y
mov oT3.w, $worldEyeVect.z

alloc $tangentEyeVect

; transform the eye vector to tangent space
dp3 $tangentEyeVect.x, $worldEyeVect, $vTangentS
dp3 $tangentEyeVect.y, $worldEyeVect, $vTangentT
dp3 $tangentEyeVect.z, $worldEyeVect, $vNormal

&FreeRegister( \$worldEyeVect );

&Normalize( $tangentEyeVect );

; stick the tangent space eye vector into oD0
mad oD0.xyz, $tangentEyeVect, $cHalf, $cHalf

&FreeRegister( \$tangentEyeVect );

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------
dp4 oT0.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_0
dp4 oT0.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_1



