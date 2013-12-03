//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef QUANTIZE_H
#define QUANTIZE_H

#ifndef STRING_H
#include <string.h>
#endif

#define MAXDIMS 768
#define MAXQUANT 16000


#include <tier0/platform.h>

struct Sample;

struct QuantizedValue {
	double MinError;											// minimum possible error. used
	// for neighbor searches.
	struct QuantizedValue *Children[2];						// splits
	int32 value;											// only exists for leaf nodes
	struct Sample *Samples;									// every sample quantized into this
	// entry
	int32 NSamples;											// how many were quantized to this.
	int32 TotSamples;
	double *ErrorMeasure;									// variance measure for each dimension
	double TotalError;										// sum of errors
	uint8 *Mean;											// average value of each dimension
	uint8 *Mins;											// min box for children and this
	uint8 *Maxs;											// max box for children and this
	int NQuant;												// the number of samples which were
															// quantzied to this node since the
															// last time OptimizeQuantizer()
															// was called.
	int *Sums;												// sum used by OptimizeQuantizer
	int sortdim;											// dimension currently sorted along.
};

struct Sample {
	int32 ID;												// identifier of this sample. can
															// be used for any purpose.
	int32 Count;											// number of samples this sample
															// represents
	int32 QNum;										   // what value this sample ended up quantized
															// to.
	struct QuantizedValue *qptr;							// ptr to what this was quantized to.
	uint8 Value[1];										   // array of values for multi-dimensional
	// variables.
};

void FreeQuantization(struct QuantizedValue *t);

struct QuantizedValue *Quantize(struct Sample *s, int nsamples, int ndims,
								int nvalues, uint8 *weights, int value0=0);

int CompressSamples(struct Sample *s, int nsamples, int ndims);

struct QuantizedValue *FindMatch(uint8 const *sample,
								 int ndims,uint8 *weights,
								 struct QuantizedValue *QTable);
void PrintSamples(struct Sample const *s, int nsamples, int ndims);

struct QuantizedValue *FindQNode(struct QuantizedValue const *q, int32 code);

inline struct Sample *NthSample(struct Sample *s, int i, int nd)
{
	uint8 *r=(uint8 *) s;
	r+=i*(sizeof(*s)+(nd-1));
	return (struct Sample *) r;
}

inline struct Sample *AllocSamples(int ns, int nd)
{
	size_t size5=(sizeof(struct Sample)+(nd-1))*ns;
	void *ret=new uint8[size5];
	memset(ret,0,size5);
	for(int i=0;i<ns;i++)
		NthSample((struct Sample *)ret,i,nd)->Count=1;
	return (struct Sample *) ret;
}


// MinimumError: what is the min error which will occur if quantizing
// a sample to the given qnode? This is just the error if the qnode
// is a leaf.
double MinimumError(struct QuantizedValue const *q, uint8 const *sample,
					int ndims, uint8 const *weights);
double MaximumError(struct QuantizedValue const *q, uint8 const *sample,
					int ndims, uint8 const *weights);

void PrintQTree(struct QuantizedValue const *p,int idlevel=0);
void OptimizeQuantizer(struct QuantizedValue *q, int ndims);

// RecalculateVelues: update the means in a sample tree, based upon
// the samples. can be used to reoptimize when samples are deleted,
// for instance.

void RecalculateValues(struct QuantizedValue *q, int ndims);

extern double SquaredError;	// may be reset and examined. updated by
															// FindMatch()




// the routines below can be used for uniform quantization via dart-throwing.
typedef void (*GENERATOR)(void *);    // generate a random sample
typedef double (*COMPARER)(void const *a, void const *b);

void *DartThrow(int NResults, int NTries, size_t itemsize, GENERATOR gen,
				COMPARER cmp);
void *FindClosestDart(void *items,int NResults, size_t itemsize,
					  COMPARER cmp, void *lookfor, int *idx);




// color quantization of 24 bit images
#define QUANTFLAGS_NODITHER 1	// don't do Floyd-steinberg dither

extern void ColorQuantize(
uint8 const	*pImage,			// 4 byte pixels ARGB
int			nWidth,
int			nHeight,
int			nFlags, 			// QUANTFLAGS_xxx
int			nColors,			// # of colors to fill in in palette
uint8		*pOutPixels,		// where to store resulting 8 bit pixels
uint8		*pOutPalette,		// where to store resulting 768-byte palette
int			nFirstColor);		// first color to use in mapping





#endif
