// Absolute value (per component).
auto abs( multi x );

// Returns the arccosine of each component of x.
auto acos( multi x );

// Test if all components of x are nonzero.
auto all( multi x );

// Test if any component of x is nonzero.
auto any( multi x );

// Returns the arcsine of each component of x.
auto asin( multi x );

// Returns the arctangent of x.
auto atan( multi x );

// Returns the arctangent of of two values (x,y).
auto atan2( multi x, auto y );

// Returns the smallest integer which is greater than or equal to x.
auto ceil( multi x );

// Clamps x to the range [min, max].
auto clamp( multi x, auto min, auto max );

// Discards the current pixel, if any component of x is less than zero.
auto clip( multi x );

// Returns the cosine of x.
auto cos( multi x );

// Returns the hyperbolic cosine of x.
auto cosh( multi x );

// Returns the cross product of two 3D vectors.
float3 cross( float3 x, float3 y );

// Swizzles and scales components of the 4D vector xto compensate for the lack of UBYTE4 support in some hardware.
int4 D3DCOLORtoUBYTE4( float4 x );

// Returns the partial derivative of x with respect to the screen-space x-coordinate.
auto ddx( multi x );

// Returns the partial derivative of x with respect to the screen-space y-coordinate.
auto ddy( multi x );

// Converts x from radians to degrees.
auto degrees( multi x );

// Returns the determinant of the square matrix m.
float determinant( matrix m );

// Returns the distance between two points.
float distance( vector x, vector y );

// Returns the dot product of two vectors.
float dot( vector x, vector y );

// Returns the base-e exponent.
auto exp( multi x );

// Base 2 exponent (per component).
auto exp2( multi x );

// Returns -n * sign(dot(i, ng)).
auto faceforward( vector n, auto i, auto ng );

// Returns the greatest integer which is less than or equal to x.
auto floor( multi x );

// Returns the floating point remainder of x/y.
auto fmod( multi x, auto y );

// Returns the fractional part of x.
auto frac( multi x );

// Returns the mantissa and exponent of x.
auto frexp( multi x, auto exp );

// Returns abs(ddx(x)) + abs(ddy(x))
auto fwidth( multi x );

// Returns true if x is finite, false otherwise.
bool isfinite( multi x );

// Returns true if x is +INF or -INF, false otherwise.
bool isinf( multi x );

// Returns true if x is NAN or QNAN, false otherwise.
bool isnan( multi x );

// Returns x * 2exp
auto ldexp( multi x, auto exp );

// Returns the length of the vector v.
float length( vector x );

// Returns x + s(y - x).
auto lerp( multi x, auto y, auto s );

// Returns a lighting vector (ambient, diffuse, specular, 1)
float4 lit( float n_dot_l, float n_dot_h, float m );

// Returns the base-e logarithm of x.
auto log( multi x );

// Returns the base-10 logarithm of x.
auto log10( multi x );

// Returns the base-2 logarithm of x.
auto log2( multi x );

// Selects the greater of x and y.
auto max( multi x, auto y );

// Selects the lesser of x and y.
auto min( multi x, auto y );

// Splits the value x into fractional and integer parts.
auto modf( multi x, out auto ip );

// Performs matrix multiplication using x and y.
multi mul( multi x, multi y );

// Generates a random value using the Perlin-noise algorithm.
float noise( vector x );

// Returns a normalized vector.
auto normalize( vector x );

// Returns x^y
auto pow( multi x, auto y );

// Converts x from degrees to radians.
auto radians( multi x );

// Returns a reflection vector.
auto reflect( vector i, auto n );

// Returns the refraction vector.
auto refract( vector i, auto n, float index );

// Rounds x to the nearest integer.
auto round( multi x );

// Returns 1 / sqrt(x)
auto rsqrt( multi x );

// Clamps x to the range [0, 1]
auto saturate( multi x );

// Computes the sign of x.
auto sign( multi x );

// Returns the sine of x
auto sin( multi x );

// Returns the sine and cosine of x.
void sincos( multi x, out auto s, out auto c );

// Returns the hyperbolic sine of x
auto sinh( multi x );

// Returns a smooth Hermite interpolation between 0 and 1.
auto smoothstep( auto min, auto max, multi x );

// Square root (per component)
auto sqrt( multi x );

// Returns (x >= y) ? 1 : 0
auto step( multi y, auto x );

// Returns the tangent of x
auto tan( multi x );

// Returns the hyperbolic tangent of x
auto tanh( multi x );

// Returns the transpose of the matrix m.
matrix transpose( matrix x );

// Truncates floating-point value(s) to integer value(s)
auto trunc( multi x );

// 1D texture lookup.
float4 tex1D( sampler s, float t );

// 1D texture lookup.
float4 tex1D( sampler s, float t, float ddx, float ddy );

// 1D texture lookup with bias.
float4 tex1Dbias( sampler s, float4 t );

// 1D texture lookup with a gradient.
float4 tex1Dgrad( sampler s, float t, float ddx, float ddy );

// 1D texture lookup with LOD.
float4 tex1Dlod( sampler s, float4 t );

// 1D texture lookup with projective divide.
float4 tex1Dproj( sampler s, float4 t );


// 2D texture lookup.
float4 tex2D( sampler s, float2 t );

// 2D texture lookup.
float4 tex2D( sampler s, float2 t, float2 ddx, float2 ddy );

// 2D texture lookup with bias.
float4 tex2Dbias( sampler s, float4 t );

// 2D texture lookup with a gradient.
float4 tex2Dgrad( sampler s, float2 t, float2 ddx, float2 ddy );

// 2D texture lookup with LOD.
float4 tex2Dlod( sampler s, float4 t );

// 2D texture lookup with projective divide.
float4 tex2Dproj( sampler s, float4 t );


// 3D texture lookup.
float4 tex3D( sampler s, float3 t );

// 3D texture lookup.
float4 tex3D( sampler s, float3 t, float3 ddx, float3 ddy );

// 3D texture lookup with bias.
float4 tex3Dbias( sampler s, float4 t );

// 3D texture lookup with a gradient.
float4 tex3Dgrad( sampler s, float3 t, float3 ddx, float3 ddy );

// 3D texture lookup with LOD.
float4 tex3Dlod( sampler s, float4 t );

// 3D texture lookup with projective divide.
float4 tex3Dproj( sampler s, float4 t );


// Cube texture lookup.
float4 texCUBE( sampler s, float3 t );

// Cube texture lookup.
float4 texCUBE( sampler s, float3 t, float3 ddx, float3 ddy );

// Cube texture lookup with bias.
float4 texCUBEbias( sampler s, float4 t );

// Cube texture lookup with a gradient.
float4 texCUBEgrad( sampler s, float3 t, float3 ddx, float3 ddy );

// Cube texture lookup with LOD.
float4 texCUBElod( sampler s, float4 t );

// Cube texture lookup with projective divide.
float4 texCUBEproj( sampler s, float4 t );