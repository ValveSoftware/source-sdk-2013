sampler TextureSampler	: register( s0 );

struct PS_INPUT
{
	float4 vColor0		: COLOR0;
	float2 vTexCoord0	: TEXCOORD0;
};

float4 main( PS_INPUT i ) : COLOR
{
	return i.vColor0 * tex2D( TextureSampler, i.vTexCoord0 );
}