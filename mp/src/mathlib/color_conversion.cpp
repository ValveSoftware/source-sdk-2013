//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Color conversion routines.
//
//=====================================================================================//

#include <math.h>
#include <float.h>	// Needed for FLT_EPSILON
#include "basetypes.h"
#include <memory.h>
#include "tier0/dbg.h"
#include "mathlib/mathlib.h"
#include "mathlib/vector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Gamma conversion support
//-----------------------------------------------------------------------------
static byte		texgammatable[256];	// palette is sent through this to convert to screen gamma

static float	texturetolinear[256];	// texture (0..255) to linear (0..1)
static int		lineartotexture[1024];	// linear (0..1) to texture (0..255)
static int		lineartoscreen[1024];	// linear (0..1) to gamma corrected vertex light (0..255)

// build a lightmap texture to combine with surface texture, adjust for src*dst+dst*src, ramp reprogramming, etc
float			lineartovertex[4096];	// linear (0..4) to screen corrected vertex space (0..1?)
unsigned char	lineartolightmap[4096];	// linear (0..4) to screen corrected texture value (0..255)

static float	g_Mathlib_GammaToLinear[256];	// gamma (0..1) to linear (0..1)
static float	g_Mathlib_LinearToGamma[256];	// linear (0..1) to gamma (0..1)

// This is aligned to 16-byte boundaries so that we can load it
// onto SIMD registers easily if needed (used by SSE version of lightmaps)
// TODO: move this into the one DLL that actually uses it, instead of statically
// linking it everywhere via mathlib.
ALIGN128 float	power2_n[256] = 			// 2**(index - 128) / 255
{ 
	1.152445441982634800E-041, 2.304890883965269600E-041, 4.609781767930539200E-041, 9.219563535861078400E-041, 
	1.843912707172215700E-040, 3.687825414344431300E-040, 7.375650828688862700E-040, 1.475130165737772500E-039,
	2.950260331475545100E-039, 5.900520662951090200E-039, 1.180104132590218000E-038, 2.360208265180436100E-038, 
	4.720416530360872100E-038, 9.440833060721744200E-038, 1.888166612144348800E-037, 3.776333224288697700E-037, 
	7.552666448577395400E-037, 1.510533289715479100E-036, 3.021066579430958200E-036, 6.042133158861916300E-036, 
	1.208426631772383300E-035, 2.416853263544766500E-035, 4.833706527089533100E-035, 9.667413054179066100E-035, 
	1.933482610835813200E-034, 3.866965221671626400E-034, 7.733930443343252900E-034, 1.546786088668650600E-033, 
	3.093572177337301200E-033, 6.187144354674602300E-033, 1.237428870934920500E-032, 2.474857741869840900E-032, 
	4.949715483739681800E-032, 9.899430967479363700E-032, 1.979886193495872700E-031, 3.959772386991745500E-031, 
	7.919544773983491000E-031, 1.583908954796698200E-030, 3.167817909593396400E-030, 6.335635819186792800E-030, 
	1.267127163837358600E-029, 2.534254327674717100E-029, 5.068508655349434200E-029, 1.013701731069886800E-028, 
	2.027403462139773700E-028, 4.054806924279547400E-028, 8.109613848559094700E-028, 1.621922769711818900E-027, 
	3.243845539423637900E-027, 6.487691078847275800E-027, 1.297538215769455200E-026, 2.595076431538910300E-026, 
	5.190152863077820600E-026, 1.038030572615564100E-025, 2.076061145231128300E-025, 4.152122290462256500E-025, 
	8.304244580924513000E-025, 1.660848916184902600E-024, 3.321697832369805200E-024, 6.643395664739610400E-024, 
	1.328679132947922100E-023, 2.657358265895844200E-023, 5.314716531791688300E-023, 1.062943306358337700E-022, 
	2.125886612716675300E-022, 4.251773225433350700E-022, 8.503546450866701300E-022, 1.700709290173340300E-021, 
	3.401418580346680500E-021, 6.802837160693361100E-021, 1.360567432138672200E-020, 2.721134864277344400E-020, 
	5.442269728554688800E-020, 1.088453945710937800E-019, 2.176907891421875500E-019, 4.353815782843751100E-019, 
	8.707631565687502200E-019, 1.741526313137500400E-018, 3.483052626275000900E-018, 6.966105252550001700E-018, 
	1.393221050510000300E-017, 2.786442101020000700E-017, 5.572884202040001400E-017, 1.114576840408000300E-016, 
	2.229153680816000600E-016, 4.458307361632001100E-016, 8.916614723264002200E-016, 1.783322944652800400E-015, 
	3.566645889305600900E-015, 7.133291778611201800E-015, 1.426658355722240400E-014, 2.853316711444480700E-014, 
	5.706633422888961400E-014, 1.141326684577792300E-013, 2.282653369155584600E-013, 4.565306738311169100E-013, 
	9.130613476622338300E-013, 1.826122695324467700E-012, 3.652245390648935300E-012, 7.304490781297870600E-012, 
	1.460898156259574100E-011, 2.921796312519148200E-011, 5.843592625038296500E-011, 1.168718525007659300E-010, 
	2.337437050015318600E-010, 4.674874100030637200E-010, 9.349748200061274400E-010, 1.869949640012254900E-009, 
	3.739899280024509800E-009, 7.479798560049019500E-009, 1.495959712009803900E-008, 2.991919424019607800E-008, 
	5.983838848039215600E-008, 1.196767769607843100E-007, 2.393535539215686200E-007, 4.787071078431372500E-007, 
	9.574142156862745000E-007, 1.914828431372549000E-006, 3.829656862745098000E-006, 7.659313725490196000E-006, 
	1.531862745098039200E-005, 3.063725490196078400E-005, 6.127450980392156800E-005, 1.225490196078431400E-004, 
	2.450980392156862700E-004, 4.901960784313725400E-004, 9.803921568627450800E-004, 1.960784313725490200E-003, 
	3.921568627450980300E-003, 7.843137254901960700E-003, 1.568627450980392100E-002, 3.137254901960784300E-002, 
	6.274509803921568500E-002, 1.254901960784313700E-001, 2.509803921568627400E-001, 5.019607843137254800E-001, 
	1.003921568627451000E+000, 2.007843137254901900E+000, 4.015686274509803900E+000, 8.031372549019607700E+000, 
	1.606274509803921500E+001, 3.212549019607843100E+001, 6.425098039215686200E+001, 1.285019607843137200E+002, 
	2.570039215686274500E+002, 5.140078431372548900E+002, 1.028015686274509800E+003, 2.056031372549019600E+003, 
	4.112062745098039200E+003, 8.224125490196078300E+003, 1.644825098039215700E+004, 3.289650196078431300E+004, 
	6.579300392156862700E+004, 1.315860078431372500E+005, 2.631720156862745100E+005, 5.263440313725490100E+005, 
	1.052688062745098000E+006, 2.105376125490196000E+006, 4.210752250980392100E+006, 8.421504501960784200E+006, 
	1.684300900392156800E+007, 3.368601800784313700E+007, 6.737203601568627400E+007, 1.347440720313725500E+008, 
	2.694881440627450900E+008, 5.389762881254901900E+008, 1.077952576250980400E+009, 2.155905152501960800E+009, 
	4.311810305003921500E+009, 8.623620610007843000E+009, 1.724724122001568600E+010, 3.449448244003137200E+010, 
	6.898896488006274400E+010, 1.379779297601254900E+011, 2.759558595202509800E+011, 5.519117190405019500E+011, 
	1.103823438081003900E+012, 2.207646876162007800E+012, 4.415293752324015600E+012, 8.830587504648031200E+012, 
	1.766117500929606200E+013, 3.532235001859212500E+013, 7.064470003718425000E+013, 1.412894000743685000E+014, 
	2.825788001487370000E+014, 5.651576002974740000E+014, 1.130315200594948000E+015, 2.260630401189896000E+015, 
	4.521260802379792000E+015, 9.042521604759584000E+015, 1.808504320951916800E+016, 3.617008641903833600E+016, 
	7.234017283807667200E+016, 1.446803456761533400E+017, 2.893606913523066900E+017, 5.787213827046133800E+017, 
	1.157442765409226800E+018, 2.314885530818453500E+018, 4.629771061636907000E+018, 9.259542123273814000E+018, 
	1.851908424654762800E+019, 3.703816849309525600E+019, 7.407633698619051200E+019, 1.481526739723810200E+020, 
	2.963053479447620500E+020, 5.926106958895241000E+020, 1.185221391779048200E+021, 2.370442783558096400E+021, 
	4.740885567116192800E+021, 9.481771134232385600E+021, 1.896354226846477100E+022, 3.792708453692954200E+022, 
	7.585416907385908400E+022, 1.517083381477181700E+023, 3.034166762954363400E+023, 6.068333525908726800E+023, 
	1.213666705181745400E+024, 2.427333410363490700E+024, 4.854666820726981400E+024, 9.709333641453962800E+024, 
	1.941866728290792600E+025, 3.883733456581585100E+025, 7.767466913163170200E+025, 1.553493382632634000E+026, 
	3.106986765265268100E+026, 6.213973530530536200E+026, 1.242794706106107200E+027, 2.485589412212214500E+027, 
	4.971178824424429000E+027, 9.942357648848857900E+027, 1.988471529769771600E+028, 3.976943059539543200E+028, 
	7.953886119079086300E+028, 1.590777223815817300E+029, 3.181554447631634500E+029, 6.363108895263269100E+029, 
	1.272621779052653800E+030, 2.545243558105307600E+030, 5.090487116210615300E+030, 1.018097423242123100E+031, 
	2.036194846484246100E+031, 4.072389692968492200E+031, 8.144779385936984400E+031, 1.628955877187396900E+032, 
	3.257911754374793800E+032, 6.515823508749587500E+032, 1.303164701749917500E+033, 2.606329403499835000E+033, 
	5.212658806999670000E+033, 1.042531761399934000E+034, 2.085063522799868000E+034, 4.170127045599736000E+034, 
	8.340254091199472000E+034, 1.668050818239894400E+035, 3.336101636479788800E+035, 6.672203272959577600E+035 
};

// You can use this to double check the exponent table and assert that 
// the precomputation is correct.
#ifdef DBGFLAG_ASSERT
#pragma warning(push)
#pragma warning( disable : 4189 ) // disable unused local variable warning
static void CheckExponentTable()
{
	for( int i = 0; i < 256; i++ )
	{
		float testAgainst = pow( 2.0f, i - 128 ) / 255.0f;
		float diff = testAgainst - power2_n[i] ;
		float relativeDiff = diff / testAgainst;
		Assert( testAgainst == 0 ? 
				power2_n[i] < 1.16E-041 :
				power2_n[i] == testAgainst );
	}
}
#pragma warning(pop)
#endif

void BuildGammaTable( float gamma, float texGamma, float brightness, int overbright )
{
	int		i, inf;
	float	g1, g3;

	// Con_Printf("BuildGammaTable %.1f %.1f %.1f\n", g, v_lightgamma.GetFloat(), v_texgamma.GetFloat() );

	float g = gamma;
	if (g > 3.0) 
	{
		g = 3.0;
	}

	g = 1.0 / g;
	g1 = texGamma * g; 

	if (brightness <= 0.0) 
	{
		g3 = 0.125;
	}
	else if (brightness > 1.0) 
	{
		g3 = 0.05;
	}
	else 
	{
		g3 = 0.125 - (brightness * brightness) * 0.075;
	}

	for (i=0 ; i<256 ; i++)
	{
		inf = 255 * pow ( i/255.f, g1 ); 
		if (inf < 0)
			inf = 0;
		if (inf > 255)
			inf = 255;
		texgammatable[i] = inf;
	}

	for (i=0 ; i<1024 ; i++)
	{
		float f;

		f = i / 1023.0;

		// scale up
		if (brightness > 1.0)
			f = f * brightness;

		// shift up
		if (f <= g3)
			f = (f / g3) * 0.125;
		else 
			f = 0.125 + ((f - g3) / (1.0 - g3)) * 0.875;

		// convert linear space to desired gamma space
		inf = 255 * pow ( f, g ); 

		if (inf < 0)
			inf = 0;
		if (inf > 255)
			inf = 255;
		lineartoscreen[i] = inf;
	}

	/*
	for (i=0 ; i<1024 ; i++)
	{
		// convert from screen gamma space to linear space
		lineargammatable[i] = 1023 * pow ( i/1023.0, v_gamma.GetFloat() );
		// convert from linear gamma space to screen space
		screengammatable[i] = 1023 * pow ( i/1023.0, 1.0 / v_gamma.GetFloat() );
	}
	*/

	for (i=0 ; i<256 ; i++)
	{
		// convert from nonlinear texture space (0..255) to linear space (0..1)
		texturetolinear[i] =  pow( i / 255.f, texGamma );

		// convert from linear space (0..1) to nonlinear (sRGB) space (0..1)
		g_Mathlib_LinearToGamma[i] =  LinearToGammaFullRange( i / 255.f );

		// convert from sRGB gamma space (0..1) to linear space (0..1)
		g_Mathlib_GammaToLinear[i] =  GammaToLinearFullRange( i / 255.f );
	}

	for (i=0 ; i<1024 ; i++)
	{
		// convert from linear space (0..1) to nonlinear texture space (0..255)
		lineartotexture[i] =  pow( i / 1023.0, 1.0 / texGamma ) * 255;
	}

#if 0
	for (i=0 ; i<256 ; i++)
	{
		float f;

		// convert from nonlinear lightmap space (0..255) to linear space (0..4)
		// f =  (i / 255.0) * sqrt( 4 );
		f =  i * (2.0 / 255.0);
		f = f * f;

		texlighttolinear[i] = f;
	}
#endif

	{
		float f;
		float overbrightFactor = 1.0f;

		// Can't do overbright without texcombine
		// UNDONE: Add GAMMA ramp to rectify this
		if ( overbright == 2 )
		{
			overbrightFactor = 0.5;
		}
		else if ( overbright == 4 )
		{
			overbrightFactor = 0.25;
		}

		for (i=0 ; i<4096 ; i++)
		{
			// convert from linear 0..4 (x1024) to screen corrected vertex space (0..1?)
			f = pow ( i/1024.0, 1.0 / gamma );

			lineartovertex[i] = f * overbrightFactor;
			if (lineartovertex[i] > 1)
				lineartovertex[i] = 1;

			int nLightmap = RoundFloatToInt( f * 255 * overbrightFactor );
			nLightmap = clamp( nLightmap, 0, 255 );
			lineartolightmap[i] = (unsigned char)nLightmap;
		}
	}
}

float GammaToLinearFullRange( float gamma )
{
	return pow( gamma, 2.2f );
}

float LinearToGammaFullRange( float linear )
{
	return pow( linear, 1.0f / 2.2f );
}

float GammaToLinear( float gamma )
{
	Assert( s_bMathlibInitialized );
	if ( gamma < 0.0f )
	{
		return 0.0f;
	}

	if ( gamma >= 0.95f )
	{
		// Use GammaToLinearFullRange maybe if you trip this.
// X360TEMP
//		Assert( gamma <= 1.0f );
		return 1.0f;
	}

	int index = RoundFloatToInt( gamma * 255.0f );
	Assert( index >= 0 && index < 256 );
	return g_Mathlib_GammaToLinear[index];
}

float LinearToGamma( float linear )
{
	Assert( s_bMathlibInitialized );
	if ( linear < 0.0f )
	{
		return 0.0f;
	}
	if ( linear > 1.0f )
	{
		// Use LinearToGammaFullRange maybe if you trip this.
		Assert( 0 );
		return 1.0f;
	}

	int index = RoundFloatToInt( linear * 255.0f );
	Assert( index >= 0 && index < 256 );
	return g_Mathlib_LinearToGamma[index];
}

//-----------------------------------------------------------------------------
// Helper functions to convert between sRGB and 360 gamma space
//-----------------------------------------------------------------------------
float SrgbGammaToLinear( float flSrgbGammaValue )
{
	float x = clamp( flSrgbGammaValue, 0.0f, 1.0f );
	return ( x <= 0.04045f ) ? ( x / 12.92f ) : ( pow( ( x + 0.055f ) / 1.055f, 2.4f ) );
}

float SrgbLinearToGamma( float flLinearValue )
{
	float x = clamp( flLinearValue, 0.0f, 1.0f );
	return ( x <= 0.0031308f ) ? ( x * 12.92f ) : ( 1.055f * pow( x, ( 1.0f / 2.4f ) ) ) - 0.055f;
}

float X360GammaToLinear( float fl360GammaValue )
{
	float flLinearValue;

	fl360GammaValue = clamp( fl360GammaValue, 0.0f, 1.0f );
	if ( fl360GammaValue < ( 96.0f / 255.0f ) )
	{
		if ( fl360GammaValue < ( 64.0f / 255.0f ) )
		{
			flLinearValue = fl360GammaValue * 255.0f;
		}
		else
		{
			flLinearValue = fl360GammaValue * ( 255.0f * 2.0f ) - 64.0f;
			flLinearValue += floor( flLinearValue * ( 1.0f / 512.0f ) );
		}
	}
	else
	{
		if( fl360GammaValue < ( 192.0f / 255.0f ) )
		{
			flLinearValue = fl360GammaValue * ( 255.0f * 4.0f ) - 256.0f;
			flLinearValue += floor( flLinearValue * ( 1.0f / 256.0f ) );
		}
		else
		{
			flLinearValue = fl360GammaValue * ( 255.0f * 8.0f ) - 1024.0f;
			flLinearValue += floor( flLinearValue * ( 1.0f / 128.0f ) );
		}
	}

	flLinearValue *= 1.0f / 1023.0f;

	flLinearValue = clamp( flLinearValue, 0.0f, 1.0f );
	return flLinearValue;
}

float X360LinearToGamma( float flLinearValue )
{
	float fl360GammaValue;

	flLinearValue = clamp( flLinearValue, 0.0f, 1.0f );
	if ( flLinearValue < ( 128.0f / 1023.0f ) )
	{
		if ( flLinearValue < ( 64.0f / 1023.0f ) )
		{
			fl360GammaValue = flLinearValue * ( 1023.0f * ( 1.0f / 255.0f ) );
		}
		else
		{
			fl360GammaValue = flLinearValue * ( ( 1023.0f / 2.0f ) * ( 1.0f / 255.0f ) ) + ( 32.0f / 255.0f );
		}
	}
	else
	{
		if ( flLinearValue < ( 512.0f / 1023.0f ) )
		{
			fl360GammaValue = flLinearValue * ( ( 1023.0f / 4.0f ) * ( 1.0f / 255.0f ) ) + ( 64.0f / 255.0f );
		}
		else
		{
			fl360GammaValue = flLinearValue * ( ( 1023.0f /8.0f ) * ( 1.0f / 255.0f ) ) + ( 128.0f /255.0f ); // 1.0 -> 1.0034313725490196078431372549016
			if ( fl360GammaValue > 1.0f )
			{
				fl360GammaValue = 1.0f;
			}
		}
	}

	fl360GammaValue = clamp( fl360GammaValue, 0.0f, 1.0f );
	return fl360GammaValue;
}

float SrgbGammaTo360Gamma( float flSrgbGammaValue )
{
	float flLinearValue = SrgbGammaToLinear( flSrgbGammaValue );
	float fl360GammaValue = X360LinearToGamma( flLinearValue );
	return fl360GammaValue;
}

// convert texture to linear 0..1 value
float TextureToLinear( int c )
{
	Assert( s_bMathlibInitialized );
	if (c < 0)
		return 0;
	if (c > 255)
		return 1.0;

	return texturetolinear[c];
}

// convert texture to linear 0..1 value
int LinearToTexture( float f )
{
	Assert( s_bMathlibInitialized );
	int i;
	i = f * 1023;	// assume 0..1 range
	if (i < 0)
		i = 0;
	if (i > 1023)
		i = 1023;

	return lineartotexture[i];
}


// converts 0..1 linear value to screen gamma (0..255)
int LinearToScreenGamma( float f )
{
	Assert( s_bMathlibInitialized );
	int i;
	i = f * 1023;	// assume 0..1 range
	if (i < 0)
		i = 0;
	if (i > 1023)
		i = 1023;

	return lineartoscreen[i];
}

void ColorRGBExp32ToVector( const ColorRGBExp32& in, Vector& out )
{
	Assert( s_bMathlibInitialized );
	// FIXME: Why is there a factor of 255 built into this?
	out.x = 255.0f * TexLightToLinear( in.r, in.exponent );
	out.y = 255.0f * TexLightToLinear( in.g, in.exponent );
	out.z = 255.0f * TexLightToLinear( in.b, in.exponent );
}

#if 0
// assumes that the desired mantissa range is 128..255
static int VectorToColorRGBExp32_CalcExponent( float in )
{
	int power = 0;
	
	if( in != 0.0f )
	{
		while( in > 255.0f )
		{
			power += 1;
			in *= 0.5f;
		}
		
		while( in < 128.0f )
		{
			power -= 1;
			in *= 2.0f;
		}
	}

	return power;
}

void VectorToColorRGBExp32( const Vector& vin, ColorRGBExp32 &c )
{
	Vector v = vin;
	Assert( s_bMathlibInitialized );
	Assert( v.x >= 0.0f && v.y >= 0.0f && v.z >= 0.0f );
	int i;		
	float max = v[0];				
	for( i = 1; i < 3; i++ )
	{
		// Get the maximum value.
		if( v[i] > max )
		{
			max = v[i];
		}
	}
				
	// figure out the exponent for this luxel.
	int exponent = VectorToColorRGBExp32_CalcExponent( max );
				
	// make the exponent fits into a signed byte.
	if( exponent < -128 )
	{
		exponent = -128;
	}
	else if( exponent > 127 )
	{
		exponent = 127;
	}
				
	// undone: optimize with a table
	float scalar = pow( 2.0f, -exponent );
	// convert to mantissa x 2^exponent format
	for( i = 0; i < 3; i++ )
	{
		v[i] *= scalar;
		// clamp
		if( v[i] > 255.0f )
		{
			v[i] = 255.0f;
		}
	}
	c.r = ( unsigned char )v[0];
	c.g = ( unsigned char )v[1];
	c.b = ( unsigned char )v[2];
	c.exponent = ( signed char )exponent;
}

#else

// given a floating point number  f, return an exponent e such that
// for f' = f * 2^e,  f is on [128..255].
// Uses IEEE 754 representation to directly extract this information
// from the float.
inline static int VectorToColorRGBExp32_CalcExponent( const float *pin )
{
	// The thing we will take advantage of here is that the exponent component
	// is stored in the float itself, and because we want to map to 128..255, we
	// want an "ideal" exponent of 2^7. So, we compute the difference between the
	// input exponent and 7 to work out the normalizing exponent. Thus if you pass in 
	// 32 (represented in IEEE 754 as 2^5), this function will return 2
	// (because 32 * 2^2 = 128)
	if (*pin == 0.0f)
		return 0;

	unsigned int fbits = *reinterpret_cast<const unsigned int *>(pin);
	
	// the exponent component is bits 23..30, and biased by +127
	const unsigned int biasedSeven = 7 + 127;

	signed int expComponent = ( fbits & 0x7F800000 ) >> 23;
	expComponent -= biasedSeven; // now the difference from seven (positive if was less than, etc)
	return expComponent;
}



/// Slightly faster version of the function to turn a float-vector color into 
/// a compressed-exponent notation 32bit color. However, still not SIMD optimized.
/// PS3 developer: note there is a movement of a float onto an int here, which is
/// bad on the base registers -- consider doing this as Altivec code, or better yet
/// moving it onto the cell.
/// \warning: Assumes an IEEE 754 single-precision float representation! Those of you
/// porting to an 8080 are out of luck.
void VectorToColorRGBExp32( const Vector& vin, ColorRGBExp32 &c )
{
	Assert( s_bMathlibInitialized );
	Assert( vin.x >= 0.0f && vin.y >= 0.0f && vin.z >= 0.0f );

	// work out which of the channels is the largest ( we will use that to map the exponent )
	// this is a sluggish branch-based decision tree -- most architectures will offer a [max]
	// assembly opcode to do this faster.
	const float *pMax;
	if (vin.x > vin.y)
	{
		if (vin.x > vin.z)
		{
			pMax = &vin.x;
		}
		else
		{
			pMax = &vin.z;
		}
	}
	else
	{
		if (vin.y > vin.z)
		{
			pMax = &vin.y;
		}
		else
		{
			pMax = &vin.z;
		}
	}

	// now work out the exponent for this luxel. 
	signed int exponent = VectorToColorRGBExp32_CalcExponent( pMax );

	// make sure the exponent fits into a signed byte.
	// (in single precision format this is assured because it was a signed byte to begin with)
	Assert(exponent > -128 && exponent <= 127);

	// promote the exponent back onto a scalar that we'll use to normalize all the numbers
	float scalar;
	{
		unsigned int fbits = (127 - exponent) << 23;
		scalar = *reinterpret_cast<float *>(&fbits);
	}

	// We can totally wind up above 255 and that's okay--but above 256 would be right out.
	Assert(vin.x * scalar < 256.0f && 
		   vin.y * scalar < 256.0f && 
		   vin.z * scalar < 256.0f);

	// This awful construction is necessary to prevent VC2005 from using the 
	// fldcw/fnstcw control words around every float-to-unsigned-char operation.
	{
		int red = (vin.x * scalar);
		int green = (vin.y * scalar);
		int blue = (vin.z * scalar);

		c.r = red;
		c.g = green;
		c.b = blue;
	}
	/*
	c.r = ( unsigned char )(vin.x * scalar);
	c.g = ( unsigned char )(vin.y * scalar);
	c.b = ( unsigned char )(vin.z * scalar);
	*/

	c.exponent = ( signed char )exponent;
}

#endif