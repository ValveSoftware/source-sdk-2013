//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#ifndef FLOAT_BM_H
#define FLOAT_BM_H

#ifdef _WIN32
#pragma once
#endif

#include <tier0/platform.h>
#include "tier0/dbg.h"
#include <mathlib/mathlib.h>

struct PixRGBAF 
{
	float Red;
	float Green;
	float Blue;
	float Alpha;
};

struct PixRGBA8
{
	unsigned char Red;
	unsigned char Green;
	unsigned char Blue;
	unsigned char Alpha;
};

inline PixRGBAF PixRGBA8_to_F( PixRGBA8 const &x )
{
	PixRGBAF f;
	f.Red = x.Red / float( 255.0f );
	f.Green = x.Green / float( 255.0f );
	f.Blue = x.Blue / float( 255.0f );
	f.Alpha = x.Alpha / float( 255.0f );
	return f;
}

inline PixRGBA8 PixRGBAF_to_8( PixRGBAF const &f )
{
	PixRGBA8 x;
	x.Red = max( 0, min( 255.0,255.0*f.Red ) );
	x.Green = max( 0, min( 255.0,255.0*f.Green ) );
	x.Blue = max( 0, min( 255.0,255.0*f.Blue ) );
	x.Alpha = max( 0, min( 255.0,255.0*f.Alpha ) );
	return x;
}

#define SPFLAGS_MAXGRADIENT 1

// bit flag options for ComputeSelfShadowedBumpmapFromHeightInAlphaChannel:
#define SSBUMP_OPTION_NONDIRECTIONAL 1						// generate ambient occlusion only
#define SSBUMP_MOD2X_DETAIL_TEXTURE 2						// scale so that a flat unshadowed
                                                            // value is 0.5, and bake rgb luminance
                                                            // in.



class FloatBitMap_t 
{
public:
	int Width, Height;										// bitmap dimensions
	float *RGBAData;										// actual data

	FloatBitMap_t(void)										// empty one
	{
		Width=Height=0;
		RGBAData=0;
	}



	FloatBitMap_t(int width, int height);                  // make one and allocate space
	FloatBitMap_t(char const *filename);                   // read one from a file (tga or pfm)
	FloatBitMap_t(FloatBitMap_t const *orig);
	// quantize one to 8 bits
	bool WriteTGAFile(char const *filename) const;

	bool LoadFromPFM(char const *filename);					// load from floating point pixmap (.pfm) file
	bool WritePFM(char const *filename);					// save to floating point pixmap (.pfm) file


	void InitializeWithRandomPixelsFromAnotherFloatBM(FloatBitMap_t const &other);

	inline float & Pixel(int x, int y, int comp) const
	{
		Assert((x>=0) && (x<Width));
		Assert((y>=0) && (y<Height));
		return RGBAData[4*(x+Width*y)+comp];
	}

	inline float & PixelWrapped(int x, int y, int comp) const
	{
		// like Pixel except wraps around to other side
		if (x < 0)
			x+=Width;
		else
			if (x>= Width)
				x -= Width;

		if ( y < 0 )
			y+=Height;
		else
			if ( y >= Height )
				y -= Height;

		return RGBAData[4*(x+Width*y)+comp];
	}

	inline float & PixelClamped(int x, int y, int comp) const
	{
		// like Pixel except wraps around to other side
		x=clamp(x,0,Width-1);
		y=clamp(y,0,Height-1);
		return RGBAData[4*(x+Width*y)+comp];
	}


	inline float & Alpha(int x, int y) const
	{
		Assert((x>=0) && (x<Width));
		Assert((y>=0) && (y<Height));
		return RGBAData[3+4*(x+Width*y)];
	}


	// look up a pixel value with bilinear interpolation
	float InterpolatedPixel(float x, float y, int comp) const;

	inline PixRGBAF PixelRGBAF(int x, int y) const
	{
		Assert((x>=0) && (x<Width));
		Assert((y>=0) && (y<Height));

		PixRGBAF RetPix;
		int RGBoffset= 4*(x+Width*y);
		RetPix.Red= RGBAData[RGBoffset+0];
		RetPix.Green= RGBAData[RGBoffset+1];
		RetPix.Blue= RGBAData[RGBoffset+2];
		RetPix.Alpha= RGBAData[RGBoffset+3];

		return RetPix;
	}


	inline void WritePixelRGBAF(int x, int y, PixRGBAF value) const
	{
		Assert((x>=0) && (x<Width));
		Assert((y>=0) && (y<Height));

		int RGBoffset= 4*(x+Width*y);
		RGBAData[RGBoffset+0]= value.Red;
		RGBAData[RGBoffset+1]= value.Green;
		RGBAData[RGBoffset+2]= value.Blue;
		RGBAData[RGBoffset+3]= value.Alpha;

	}


	inline void WritePixel(int x, int y, int comp, float value)
	{
		Assert((x>=0) && (x<Width));
		Assert((y>=0) && (y<Height));
		RGBAData[4*(x+Width*y)+comp]= value;
	}

	// paste, performing boundary matching. Alpha channel can be used to make
	// brush shape irregular
	void SmartPaste(FloatBitMap_t const &brush, int xofs, int yofs, uint32 flags);

	// force to be tileable using poisson formula
	void MakeTileable(void);

	void ReSize(int NewXSize, int NewYSize);

	// find the bounds of the area that has non-zero alpha.
	void GetAlphaBounds(int &minx, int &miny, int &maxx,int &maxy);

	// Solve the poisson equation for an image. The alpha channel of the image controls which
	// pixels are "modifiable", and can be used to set boundary conditions. Alpha=0 means the pixel
	// is locked.  deltas are in the order [(x,y)-(x,y-1),(x,y)-(x-1,y),(x,y)-(x+1,y),(x,y)-(x,y+1)
	void Poisson(FloatBitMap_t *deltas[4],
		int n_iters,
		uint32 flags                                  // SPF_xxx
		);

	FloatBitMap_t *QuarterSize(void) const;					// get a new one downsampled 
	FloatBitMap_t *QuarterSizeBlocky(void) const;          // get a new one downsampled 

	FloatBitMap_t *QuarterSizeWithGaussian(void) const;		// downsample 2x using a gaussian


	void RaiseToPower(float pow);
	void ScaleGradients(void);
	void Logize(void);                                        // pix=log(1+pix)
	void UnLogize(void);                                      // pix=exp(pix)-1

	// compress to 8 bits converts the hdr texture to an 8 bit texture, encoding a scale factor
	// in the alpha channel. upon return, the original pixel can be (approximately) recovered
	// by the formula rgb*alpha*overbright. 
	// this function performs special numerical optimization on the texture to minimize the error
	// when using bilinear filtering to read the texture.
	void CompressTo8Bits(float overbright);
	// decompress a bitmap converted by CompressTo8Bits
	void Uncompress(float overbright);


	Vector AverageColor(void);								// average rgb value of all pixels
	float BrightestColor(void);								// highest vector magnitude

	void Clear(float r, float g, float b, float alpha);		// set all pixels to speicifed values (0..1 nominal)

	void ScaleRGB(float scale_factor);						// for all pixels, r,g,b*=scale_factor

	// given a bitmap with height stored in the alpha channel, generate vector positions and normals
	void ComputeVertexPositionsAndNormals( float flHeightScale, Vector **ppPosOut, Vector **ppNormalOut ) const;

	// generate a normal map with height stored in alpha.  uses hl2 tangent basis to support baked
	// self shadowing.  the bump scale maps the height of a pixel relative to the edges of the
	// pixel. This function may take a while - many millions of rays may be traced.  applications
	// using this method need to link w/ raytrace.lib
	FloatBitMap_t *ComputeSelfShadowedBumpmapFromHeightInAlphaChannel(
		float bump_scale, int nrays_to_trace_per_pixel=100, 
		uint32 nOptionFlags = 0								// SSBUMP_OPTION_XXX
		) const;


	// generate a conventional normal map from a source with height stored in alpha.
	FloatBitMap_t *ComputeBumpmapFromHeightInAlphaChannel( float bump_scale ) const ;


	// bilateral (edge preserving) smoothing filter. edge_threshold_value defines the difference in
	// values over which filtering will not occur. Each channel is filtered independently. large
	// radii will run slow, since the bilateral filter is neither separable, nor is it a
	// convolution that can be done via fft.
	void TileableBilateralFilter( int radius_in_pixels, float edge_threshold_value );

	~FloatBitMap_t();

	void AllocateRGB(int w, int h)
	{
		if (RGBAData) delete[] RGBAData;
		RGBAData=new float[w*h*4];
		Width=w;
		Height=h;
	}
};


// a FloatCubeMap_t holds the floating point bitmaps for 6 faces of a cube map
class FloatCubeMap_t
{
public:
	FloatBitMap_t face_maps[6];

	FloatCubeMap_t(int xfsize, int yfsize)
	{
		// make an empty one with face dimensions xfsize x yfsize
		for(int f=0;f<6;f++)
			face_maps[f].AllocateRGB(xfsize,yfsize);
	}

	// load basenamebk,pfm, basenamedn.pfm, basenameft.pfm, ...
	FloatCubeMap_t(char const *basename);

	// save basenamebk,pfm, basenamedn.pfm, basenameft.pfm, ...
	void WritePFMs(char const *basename);

	Vector AverageColor(void)
	{
		Vector ret(0,0,0);
		int nfaces=0;
		for(int f=0;f<6;f++)
			if (face_maps[f].RGBAData)
			{
				nfaces++;
				ret+=face_maps[f].AverageColor();
			}
			if (nfaces)
				ret*=(1.0/nfaces);
			return ret;
	}

	float BrightestColor(void)
	{
		float ret=0.0;
		int nfaces=0;
		for(int f=0;f<6;f++)
			if (face_maps[f].RGBAData)
			{
				nfaces++;
				ret=max(ret,face_maps[f].BrightestColor());
			}
			return ret;
	}


	// resample a cubemap to one of possibly a lower resolution, using a given phong exponent.
	// dot-product weighting will be used for the filtering operation.
	void Resample(FloatCubeMap_t &dest, float flPhongExponent);

	// returns the normalized direciton vector through a given pixel of a given face
	Vector PixelDirection(int face, int x, int y);

	// returns the direction vector throught the center of a cubemap face
	Vector FaceNormal( int nFaceNumber );
};


static inline float FLerp(float f1, float f2, float t)
{
	return f1+(f2-f1)*t;
}


// Image Pyramid class.
#define MAX_IMAGE_PYRAMID_LEVELS 16							// up to 64kx64k

enum ImagePyramidMode_t 
{
	PYRAMID_MODE_GAUSSIAN,
};

class FloatImagePyramid_t
{
public:
	int m_nLevels;
	FloatBitMap_t *m_pLevels[MAX_IMAGE_PYRAMID_LEVELS];		// level 0 is highest res

	FloatImagePyramid_t(void)
	{
		m_nLevels=0;
		memset(m_pLevels,0,sizeof(m_pLevels));
	}

	// build one. clones data from src for level 0.
	FloatImagePyramid_t(FloatBitMap_t const &src, ImagePyramidMode_t mode);

	// read or write a Pixel from a given level. All coordinates are specified in the same domain as the base level.
	float &Pixel(int x, int y, int component, int level) const;

	FloatBitMap_t *Level(int lvl) const
	{
		Assert(lvl<m_nLevels);
		Assert(lvl<ARRAYSIZE(m_pLevels));
		return m_pLevels[lvl];
	}
	// rebuild all levels above the specified level
	void ReconstructLowerResolutionLevels(int starting_level);

	~FloatImagePyramid_t(void);

	void WriteTGAs(char const *basename) const;				// outputs name_00.tga, name_01.tga,...
};

#endif
