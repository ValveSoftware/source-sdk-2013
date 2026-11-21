//========= Copyright Valve Corporation, All rights reserved. ============//


#if defined( _X360 )

void GetBaseTextureAndNormal( sampler base, sampler base2, sampler bump, bool bBase2, bool bBump, float3 coords, float3 vWeights,
								out float4 vResultBase, out float4 vResultBase2, out float4 vResultBump )
{
	vResultBase = 0;
	vResultBase2 = 0;
	vResultBump = 0;

	if ( !bBump )
	{
		vResultBump = float4(0, 0, 1, 1);
	}

#if SEAMLESS

	vWeights = max( vWeights - 0.3, 0 );

	vWeights *= 1.0f / dot( vWeights, float3(1,1,1) );

	[branch]
	if (vWeights.x > 0)
	{
		vResultBase  += vWeights.x * tex2D( base,  coords.zy );

		if ( bBase2 )
		{
			vResultBase2 += vWeights.x * tex2D( base2, coords.zy );
		}

		if ( bBump )
		{
			vResultBump  += vWeights.x * tex2D( bump,  coords.zy );
		}
	}

	[branch]
	if (vWeights.y > 0)
	{
		vResultBase  += vWeights.y * tex2D( base,  coords.xz );

		if ( bBase2 )
		{
			vResultBase2 += vWeights.y * tex2D( base2, coords.xz );
		}
		if ( bBump )
		{
			vResultBump  += vWeights.y * tex2D( bump,  coords.xz );
		}
	}

	[branch]
	if (vWeights.z > 0)
	{
		vResultBase  += vWeights.z * tex2D( base,  coords.xy );
		if ( bBase2 )
		{
			vResultBase2 += vWeights.z * tex2D( base2, coords.xy );
		}

		if ( bBump )
		{
			vResultBump  += vWeights.z * tex2D( bump,  coords.xy );
		}
	}

#else  // not seamless

	vResultBase = tex2D( base, coords.xy );

	if ( bBase2 )
	{
		vResultBase2 = tex2D( base2, coords.xy );
	}

	if ( bBump )
	{
		vResultBump  = tex2D( bump, coords.xy );
	}

#endif


}

#else // PC

void GetBaseTextureAndNormal( sampler base, sampler base2, sampler bump, bool bBase2, bool bBump, float3 coords, float3 vWeights,
							 out float4 vResultBase, out float4 vResultBase2, out float4 vResultBump )
{
	vResultBase = 0;
	vResultBase2 = 0;
	vResultBump = 0;

	if ( !bBump )
	{
		vResultBump = float4(0, 0, 1, 1);
	}

#if SEAMLESS

	vResultBase  += vWeights.x * tex2D( base, coords.zy );
	if ( bBase2 )
	{
		vResultBase2 += vWeights.x * tex2D( base2, coords.zy );
	}
	if ( bBump )
	{
		vResultBump  += vWeights.x * tex2D( bump, coords.zy );
	}

	vResultBase  += vWeights.y * tex2D( base, coords.xz );
	if ( bBase2 )
	{
		vResultBase2 += vWeights.y * tex2D( base2, coords.xz );
	}
	if ( bBump )
	{
		vResultBump  += vWeights.y * tex2D( bump, coords.xz );
	}

	vResultBase  += vWeights.z * tex2D( base, coords.xy );
	if ( bBase2 )
	{
		vResultBase2 += vWeights.z * tex2D( base2, coords.xy );
	}
	if ( bBump )
	{
		vResultBump  += vWeights.z * tex2D( bump, coords.xy );
	}

#else  // not seamless

	vResultBase  = tex2D( base, coords.xy );
	if ( bBase2 )
	{
		vResultBase2 = tex2D( base2, coords.xy );
	}
	if ( bBump )
	{
		vResultBump  = tex2D( bump, coords.xy );
	}
#endif

}

#endif
