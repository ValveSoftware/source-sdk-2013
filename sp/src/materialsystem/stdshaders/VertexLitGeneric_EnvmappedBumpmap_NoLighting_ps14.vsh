vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"
# DYNAMIC: "SKINNING"				"0..1"

;------------------------------------------------------------------------------
; Shader specific constant:
;	 $SHADER_SPECIFIC_CONST_5	 = [sOffset, tOffset, 0, 0]
;------------------------------------------------------------------------------

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending
;------------------------------------------------------------------------------

&AllocateRegister( \$worldPos );
&AllocateRegister( \$worldNormal );
&AllocateRegister( \$worldTangentS );
&AllocateRegister( \$worldTangentT );

&SkinPositionNormalAndTangentSpace( $worldPos, $worldNormal, 
					$worldTangentS, $worldTangentT );

;------------------------------------------------------------------------------
; Transform the position from world to proj space
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

;------------------------------------------------------------------------------
; Lighting
;------------------------------------------------------------------------------

; Transform tangent space basis vectors to env map space (world space)
; This will produce a set of vectors mapping from tangent space to env space
; We'll use this to transform normals from the normal map from tangent space
; to environment map space. 
; NOTE: use dp3 here since the basis vectors are vectors, not points

; svect
mov oT1.x, $worldTangentS.x
mov oT2.x, $worldTangentS.y
mov oT3.x, $worldTangentS.z
&FreeRegister( \$worldTangentS );

; tvect
mov oT1.y, $worldTangentT.x
mov oT2.y, $worldTangentT.y
mov oT3.y, $worldTangentT.z
&FreeRegister( \$worldTangentT );

; normal
mov oT1.z, $worldNormal.x
mov oT2.z, $worldNormal.y
mov oT3.z, $worldNormal.z
 
&FreeRegister( \$worldNormal );

; Compute the vector from vertex to camera
&AllocateRegister( \$eyeVector );
sub $eyeVector.xyz, $cEyePos, $worldPos  

&FreeRegister( \$worldPos );

; eye vector
mov oT4.xyz, $eyeVector

&FreeRegister( \$eyeVector );

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------
dp4 oT0.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_4
dp4 oT0.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_5


