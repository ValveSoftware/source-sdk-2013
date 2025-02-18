vs.1.1

# DYNAMIC: "DOWATERFOG"				"0..1"

#include "macros.vsh"

$cQuarter = "c91.x";

;------------------------------------------------------------------------------
; Constants specified by the app
;    c0      = (0, 1, 2, 0.5)
;	 c1		 = (1/2.2, 0, 0, 0)
;    2      = camera position *in world space*
;    c4-c7   = modelViewProj matrix	(transpose)
;    c8-c11  = ViewProj matrix (transpose)
;    c12-c15 = model->world matrix (transpose)
;	 c16	 = [fogStart, fogEnd, fogRange, undefined]
;	 c17-c20 = model->view matrix (transpose)
;
; The ParticleSphere lighting equation is:
; A + [N dot ||L - P||] * C * r / |L - P|^2
;
; where:
; A = ambient light color
; N = particle normal (stored in the texture)
; L = directional light position
; P = point on surface
; C = directional light color
; r = directional light intensity
;
; This shader just does the |L - P| part and the pixel shader does the rest.
;
; Vertex components
;   $vPos		= Position
;	$vSpecular		= Directional light color
;	$vColor		= Ambient color (and alpha)
;	$vTexCoord0		= Texture coordinates for normal map
;	$vTexCoord0.z	= Index into the light list for light info



;------------------------------------------------------------------------------
; Constant registers
;------------------------------------------------------------------------------

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
; Setup to index our directional light.
;------------------------------------------------------------------------------
mov		a0.x, $vTexCoord0.z


;------------------------------------------------------------------------------
; Copy texcoords for the normal map texture
;------------------------------------------------------------------------------

mov		oT0, $vTexCoord0
mov		oT2.xyz, $vColor

; FIXME : the rest of this needs to use AllocateRegister

;------------------------------------------------------------------------------
; Generate a tangent space and rotate L.
; This can be thought of as rotating the normal map to face the viewer.
;
; This is useful when a particle is way off to the side of the screen.
; You should be looking at the half-sphere with a normal pointing from the
; particle to the viewer. Instead, you're looking at the half-sphere with
; a normal along Z. This tangent space builder code fixes the problem.
;
; Note that since the model and view matrices are identity, the coordinate
; system has X=right, Y=up, and Z=behind you  (negative Z goes into the screen).
;------------------------------------------------------------------------------
								; r5 (forward) = normalized P
dp3		r1, $vPos, $vPos
rsq		r5, r1
mul		r5, r5, $vPos
mov		r5.z, -r5.z				; This basis wants Z positive going into the screen
								; so flip it here.

								; r1 (up) = r5 x c24
mul		r1, r5.xzyw, $SHADER_SPECIFIC_CONST_0; (This effectively does a cross product with [1,0,0,0]
								; You wind up with [0, r5.z, -r5.y, 1]
dp3		r2, r1, r1
rsq		r2, r2
mul		r1, r1, r2

								; r2 (right) = r1 x r5
mul		r2,  r1.yzxw, r5.zxyw	
mad		r2, -r1.zxyw, r5.yzxw, r2

sub		r3, c[45 + a0.x], $vPos    ; r3 = L - P

; transposed matrix mul
mul		r0, r2, r3.xxxx			;   x * right
mad		r0, r1, r3.yyyy, r0		; + y * up
mad		r0, r5, r3.zzzz, r0		; + z * forward


;------------------------------------------------------------------------------
; Put ||L - P|| into t1
;------------------------------------------------------------------------------
dp3		r2, r0, r0			; r2 = Length(L - P)^2
rsq		r3, r2				; r3 = 1 / Length(L - P)
mul		r8, r0, r3			; r8 = Normalize(L - P)
mul		r9, r8, $cQuarter		; r9 = Normalize(L - P) * 0.25
add		oT1, r9, c0.w		; oT1 = Normalize(L - P) * 0.25 + 0.5


;------------------------------------------------------------------------------
; Setup the diffuse light color (C * r / ||L - P||^2)
;------------------------------------------------------------------------------

mul		r8, c[46 + a0.x], $vSpecular	; r8 = C * r
rcp		r7, r2					; r7 = 1 / Length(L - P)^2

; rescale the color if necessary
mul		r8,  r8, r7				; r8 = C * r / Length(L - P)^2

; We're doing both parts of an if statement here, with each part scaled by 0 or 1.
mul		r9,  r7, c[46 + a0.x]	; r9 = r / Length(L - P)^2

; If the light intensity scales the color > 1
	sge		r10, r9.xxxx, $cOne	; r10.x = 1 if the color's max component > 1
	rcp		r6,  r9.xxxx			
	mul		r6,  r6, r10.xxxx		; r6 = 1 / max_component or [0,0,0,0] if max_component < 1
	mul		r2,  r8, r6				; rescaled color (all zeros if no component was > 1)
; else
	slt		r11, r9.xxxx, $cOne	; r11.x = 1 if the color's max component < 1
	mad		oD0.xyz, r8, r11, r2	; if it was rescaled, then r8*r11 = 0
									; if not, then r8*r11 = the original color

mov		oD0.a, $vColor.a				; Pass in vertex alpha so the pixel shader can use it.


