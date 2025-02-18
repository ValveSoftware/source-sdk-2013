//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include <quantize.h>
#include <minmax.h>

#define N_EXTRAVALUES 1
#define N_DIMENSIONS (3+N_EXTRAVALUES)

#define PIXEL(x,y,c) Image[4*((x)+((Width*(y))))+c]

static uint8 Weights[]={5,7,4,8};
static int ExtraValueXForms[3*N_EXTRAVALUES]={
	76,151,28,
};

  

#define MAX_QUANTIZE_IMAGE_WIDTH 4096

void ColorQuantize(uint8 const *Image,
				   int Width,
				   int Height,
				   int flags, int ncolors,
				   uint8 *out_pixels,
				   uint8 *out_palette,
				   int firstcolor)
{
	int Error[MAX_QUANTIZE_IMAGE_WIDTH+1][3][2];
	struct Sample *s=AllocSamples(Width*Height,N_DIMENSIONS);
	int x,y,c;
	for(y=0;y<Height;y++)
		for(x=0;x<Width;x++)
		{
			for(c=0;c<3;c++)
				NthSample(s,y*Width+x,N_DIMENSIONS)->Value[c]=PIXEL(x,y,c);
			// now, let's generate extra values to quantize on
			for(int i=0;i<N_EXTRAVALUES;i++)
			{
				int val1=0;
				for(c=0;c<3;c++)
					val1+=PIXEL(x,y,c)*ExtraValueXForms[i*3+c];
				val1>>=8;
				NthSample(s,y*Width+x,N_DIMENSIONS)->Value[c]=(uint8)
					(min(255,max(0,val1)));
			}
		}
	struct QuantizedValue *q=Quantize(s,Width*Height,N_DIMENSIONS,
									  ncolors,Weights,firstcolor);
	delete[] s;
	memset(out_palette,0x55,768);
	for(int p=0;p<256;p++)
	{
		struct QuantizedValue *v=FindQNode(q,p);
		if (v)
			for(c=0;c<3;c++)
				out_palette[p*3+c]=v->Mean[c];
	}
	memset(Error,0,sizeof(Error));
	for(y=0;y<Height;y++)
	{
		int ErrorUse=y & 1;
		int ErrorUpdate=ErrorUse^1;
		for(x=0;x<Width;x++)
		{
			uint8 samp[3];
			for(c=0;c<3;c++)
			{
				int tryc=PIXEL(x,y,c);
				if (! (flags & QUANTFLAGS_NODITHER))
				{
					tryc+=Error[x][c][ErrorUse];
					Error[x][c][ErrorUse]=0;
				}
				samp[c]=(uint8) min(255,max(0,tryc));
			}
			struct QuantizedValue *f=FindMatch(samp,3,Weights,q);
			out_pixels[Width*y+x]=(uint8) (f->value);
			if (! (flags & QUANTFLAGS_NODITHER))
				for(int i=0;i<3;i++)
				{
					int newerr=samp[i]-f->Mean[i];
					int orthog_error=(newerr*3)/8;
					Error[x+1][i][ErrorUse]+=orthog_error;
					Error[x][i][ErrorUpdate]=orthog_error;
					Error[x+1][i][ErrorUpdate]=newerr-2*orthog_error;
				}
		}
	}
	if (q) FreeQuantization(q);
}

