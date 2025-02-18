#include "macros.vsh"

;------------------------------------------------------------------------------
;	 $SHADER_SPECIFIC_CONST_0-$SHADER_SPECIFIC_CONST_1 = Base texture transform + envmap mask transform
;    $SHADER_SPECIFIC_CONST_2-$SHADER_SPECIFIC_CONST_3 = Detail texture transform
;------------------------------------------------------------------------------

sub ShatteredGlass
{
	local( $detail ) = shift;
	local( $envmap ) = shift;
	local( $envmapcameraspace ) = shift;
	local( $envmapsphere ) = shift;
	local( $vertexcolor ) = shift;

	local( $worldPos, $worldNormal, $projPos, $reflectionVector );

	&AllocateRegister( \$projPos );

	dp4 $projPos.x, $vPos, $cModelViewProj0
	dp4 $projPos.y, $vPos, $cModelViewProj1
	dp4 $projPos.z, $vPos, $cModelViewProj2
	dp4 $projPos.w, $vPos, $cModelViewProj3
	mov oPos, $projPos

	&AllocateRegister( \$worldPos );

	if( $DOWATERFOG == 1 )
	{
		; Get the worldpos z component only since that's all we need for height fog
		dp4 $worldPos.z, $vPos, $cModel2
	}
	&CalcFog( $worldPos, $projPos );
	&FreeRegister( \$projPos );

	;------------------------------------------------------------------------------
	; Texture coordinates
	;------------------------------------------------------------------------------
	; base texcoords
	dp4 oT0.x, $vTexCoord0, $SHADER_SPECIFIC_CONST_0
	dp4 oT0.y, $vTexCoord0, $SHADER_SPECIFIC_CONST_1

	; lightmap texcoords
	mov oT1, $vTexCoord1

	if( $envmap )
	{
		&AllocateRegister( \$worldNormal );

		; Transform the position + normal to world space
		dp4 $worldPos.x, $vPos, $cModel0
		dp4 $worldPos.y, $vPos, $cModel1
		if( $DOWATERFOG == 0 )
		{
			dp4 $worldPos.z, $vPos, $cModel2
		}

		dp3 $worldNormal.x, $vNormal, $cModel0
		dp3 $worldNormal.y, $vNormal, $cModel1
		dp3 $worldNormal.z, $vNormal, $cModel2

		if( $envmapcameraspace )
		{
			&AllocateRegister( \$reflectionVector );
			&ComputeReflectionVector( $worldPos, $worldNormal, $reflectionVector );
			; transform reflection vector into view space
			dp3 oT2.x, $reflectionVector, $cViewModel0
			dp3 oT2.y, $reflectionVector, $cViewModel1
			dp3 oT2.z, $reflectionVector, $cViewModel2
			&FreeRegister( \$reflectionVector );
		}
		elsif( $envmapsphere )
		{
			&AllocateRegister( \$reflectionVector );
			&ComputeReflectionVector( $worldPos, $worldNormal, $reflectionVector );
			&ComputeSphereMapTexCoords( $reflectionVector, "oT2" );
			&FreeRegister( \$reflectionVector );
		}
		else
		{
			&ComputeReflectionVector( $worldPos, $worldNormal, "oT2" );
		}

		&FreeRegister( \$worldNormal );
	}

	&FreeRegister( \$worldPos );

	if( $detail )
	{
		dp4 oT3.x, $vTexCoord2, $SHADER_SPECIFIC_CONST_2
		dp4 oT3.y, $vTexCoord2, $SHADER_SPECIFIC_CONST_3
	}

	if( $vertexcolor )
	{
		; Modulation color
		mul oD0, $vColor, $cModulationColor
	}
	else
	{
		; Modulation color
		mov oD0, $cModulationColor
	}
}
