
struct PS_INPUT
{
	float4 T0				: TEXCOORD0;
	float3 T1				: TEXCOORD1;
	float3 T2				: TEXCOORD2;
	float  T3				: TEXCOORD3;
	float3 vColor			: COLOR0;
};

float4 main( PS_INPUT i ) : COLOR
{
	// Kill pixel against various fields computed in vertex shader
	clip ( i.T1 );
	clip ( i.T2 );
	clip ( i.T3 ); // Backface cull

	// i.T0.a is uninitialized by the vs, but this is how the original asm shader was written????
	return float4( lerp( float3(1,1,1), i.vColor.xyz, i.T0.a ), 1 );
}