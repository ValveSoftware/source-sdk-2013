#include "macros.vsh"

sub VertexLitGeneric
{
	local( $detail ) = shift;
	local( $envmap ) = shift;
	local( $envmapcameraspace ) = shift;
	local( $envmapsphere ) = shift;
	local( $decal ) = shift;

	local( $worldPos, $worldNormal, $projPos );
	local( $reflectionVector );

	;------------------------------------------------------------------------------
	; Vertex blending 
	;------------------------------------------------------------------------------
	&AllocateRegister( \$worldPos );
	&AllocateRegister( \$worldNormal );
	&AllocateRegister( \$projPos );
;	if( $g_staticLightType eq "static" && $g_ambientLightType eq "none" &&
;		$g_localLightType1 eq "none" && $g_localLightType2 eq "none" && !$envmap )
	if( 0 )
	{
		; NOTE: Don't do this optimization anymore since it would mean a gazillion combos
		; of the flashlight shaders
		; Special case for static prop lighting.  We can go directly from 
		; world to proj space for position, with the exception of z, which 
		; is needed for fogging *if* height fog is enabled.

		; NOTE: We don't use this path if $envmap is defined since we need
		; worldpos for envmapping.
		dp4 $projPos.x, $vPos, $cModelViewProj0
		dp4 $projPos.y, $vPos, $cModelViewProj1
		dp4 $projPos.z, $vPos, $cModelViewProj2
		dp4 $projPos.w, $vPos, $cModelViewProj3
		; normal
		dp3 $worldNormal.x, $vNormal, $cModel0
		dp3 $worldNormal.y, $vNormal, $cModel1
		dp3 $worldNormal.z, $vNormal, $cModel2

		; Need this for height fog if it's enabled and for height clipping
		dp4 $worldPos.z, $vPos, $cModel2
	}
	else
	{
		&SkinPositionAndNormal( $worldPos, $worldNormal );

		if( $SKINNING == 1 )
		{
			&Normalize( $worldNormal );
		}

		;------------------------------------------------------------------------------
		; Transform the position from world to view space
		;------------------------------------------------------------------------------
		dp4 $projPos.x, $worldPos, $cViewProj0
		dp4 $projPos.y, $worldPos, $cViewProj1
		dp4 $projPos.z, $worldPos, $cViewProj2
		dp4 $projPos.w, $worldPos, $cViewProj3
	}

	mov oPos, $projPos

	;------------------------------------------------------------------------------
	; Fog
	;------------------------------------------------------------------------------
	&CalcFog( $worldPos, $projPos );
	&FreeRegister( \$projPos );

	;------------------------------------------------------------------------------
	; Lighting
	;------------------------------------------------------------------------------
	&DoLighting( $worldPos, $worldNormal );

	if( !$envmap )
	{
		&FreeRegister( \$worldNormal );
	}

	;------------------------------------------------------------------------------
	; Texture coordinates
	;------------------------------------------------------------------------------

	dp4 oT0.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_0
	dp4 oT0.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_1

	if( $envmap )
	{
		if( $envmapcameraspace )
		{
			&AllocateRegister( \$reflectionVector );
			&ComputeReflectionVector( $worldPos, $worldNormal, $reflectionVector );

			; transform reflection vector into view space
			dp3 oT1.x, $reflectionVector, $cViewModel0
			dp3 oT1.y, $reflectionVector, $cViewModel1
			dp3 oT1.z, $reflectionVector, $cViewModel2

			&FreeRegister( \$reflectionVector );
		}
		elsif( $envmapsphere )
		{
			&AllocateRegister( \$reflectionVector );
			&ComputeReflectionVector( $worldPos, $worldNormal, $reflectionVector );
			&ComputeSphereMapTexCoords( $reflectionVector, "oT1" );

			&FreeRegister( \$reflectionVector );
		}
		else
		{
			&ComputeReflectionVector( $worldPos, $worldNormal, "oT1" );
		}

		; envmap mask
		dp4 oT2.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_2
		dp4 oT2.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_3

		&FreeRegister( \$worldNormal );
	}
	else
	{
		if ( $decal )
		{
			&AllocateRegister( \$temp );
			mov $temp, $vTexCoord0
			sub oT1.xyz, $temp.xyz, $vTexCoord1.xyz
			sub oT2.xyz, $vTexCoord2.xyz, $temp.xyz
			&FreeRegister( \$temp );
		}
		else
		{
			; YUCK!  This is to make texcoords continuous for mat_softwaretl
			mov oT1, $cZero
			mov oT2, $cZero
		}
	}

	if( $detail )
	{
		dp4 oT3.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_4
		dp4 oT3.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_5
	}
	&FreeRegister( \$worldPos );
}

