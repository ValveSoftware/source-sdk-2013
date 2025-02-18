sampler TextureSampler	: register( s1 );

struct PS_INPUT
{
	float4 vColor0		: COLOR0;
	float2 vTexCoord1	: TEXCOORD1;
};

float4 main( PS_INPUT i ) : COLOR
{
	return tex2D( TextureSampler, i.vTexCoord1 );
}