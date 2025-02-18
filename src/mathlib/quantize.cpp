//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef STDIO_H
#include <stdio.h>
#endif

#ifndef STRING_H
#include <string.h>
#endif

#ifndef QUANTIZE_H
#include <quantize.h>
#endif

#include <stdlib.h>
#include <minmax.h>

#include <math.h>

static int current_ndims;
static struct QuantizedValue *current_root;
static int current_ssize;

static uint8 *current_weights;

double SquaredError;

#define SPLIT_THEN_SORT 1

#define SQ(x) ((x)*(x))

static struct QuantizedValue *AllocQValue(void)
{
	struct QuantizedValue *ret=new QuantizedValue;
	ret->Samples=0;
	ret->Children[0]=ret->Children[1]=0;
	ret->NSamples=0;
  
	ret->ErrorMeasure=new double[current_ndims];
	ret->Mean=new uint8[current_ndims];
	ret->Mins=new uint8[current_ndims];
	ret->Maxs=new uint8[current_ndims];
	ret->Sums=new int [current_ndims];
	memset(ret->Sums,0,sizeof(int)*current_ndims);
	ret->NQuant=0;
	ret->sortdim=-1;
	return ret;
}

void FreeQuantization(struct QuantizedValue *t)
{
	if (t)
	{
		delete[] t->ErrorMeasure;
		delete[] t->Mean;
		delete[] t->Mins;
		delete[] t->Maxs;
		FreeQuantization(t->Children[0]);
		FreeQuantization(t->Children[1]);
		delete[] t->Sums;
		delete[] t;
	}
}

static int QNumSort(void const *a, void const *b)
{
	int32 as=((struct Sample *) a)->QNum;
	int32 bs=((struct Sample *) b)->QNum;
	if (as==bs) return 0;
	return (as>bs)?1:-1;
}

#if SPLIT_THEN_SORT
#else
static int current_sort_dim;

static int samplesort(void const *a, void const *b)
{
	uint8 as=((struct Sample *) a)->Value[current_sort_dim];
	uint8 bs=((struct Sample *) b)->Value[current_sort_dim];
	if (as==bs) return 0;
	return (as>bs)?1:-1;
}
#endif

static int sortlong(void const *a, void const *b)
{
	// treat the entire vector of values as a long integer for duplicate removal.
	return memcmp(((struct Sample *) a)->Value,
				  ((struct Sample *) b)->Value,current_ndims);
}


  
#define NEXTSAMPLE(s) ( (struct Sample *) (((uint8 *) s)+current_ssize))
#define SAMPLE(s,i) NthSample(s,i,current_ndims)

static void SetNDims(int n)
{
	current_ssize=sizeof(struct Sample)+(n-1);
	current_ndims=n;
}

int CompressSamples(struct Sample *s, int nsamples, int ndims)
{
	SetNDims(ndims);
	qsort(s,nsamples,current_ssize,sortlong);
	// now, they are all sorted by treating all dimensions as a large number.
	// we may now remove duplicates.
	struct Sample *src=s;
	struct Sample *dst=s;
	struct Sample *lastdst=dst;
	dst=NEXTSAMPLE(dst);		// copy first sample to get the ball rolling
	src=NEXTSAMPLE(src);
	int noutput=1;
	while(--nsamples)		// while some remain
	{
		if (memcmp(src->Value,lastdst->Value,current_ndims))
		{
			// yikes, a difference has been found!
			memcpy(dst,src,current_ssize);
			lastdst=dst;
			dst=NEXTSAMPLE(dst);
			noutput++;
		}
		else
			lastdst->Count++;
		src=NEXTSAMPLE(src);
	}
	return noutput;
}

void PrintSamples(struct Sample const *s, int nsamples, int ndims)
{
	SetNDims(ndims);
	int cnt=0;
	while(nsamples--)
	{
		printf("sample #%d, count=%d, values=\n { ",cnt++,s->Count);
		for(int d=0;d<ndims;d++)
			printf("%02x,",s->Value[d]);
		printf("}\n");
		s=NEXTSAMPLE(s);
	}
}

void PrintQTree(struct QuantizedValue const *p,int idlevel)
{
	int i;

	if (p)
	{
		for(i=0;i<idlevel;i++)
			printf(" ");
		printf("node=%p NSamples=%d value=%d Mean={",p,p->NSamples,p->value);
		for(i=0;i<current_ndims;i++)
			printf("%x,",p->Mean[i]);
		printf("}\n");
		for(i=0;i<idlevel;i++)
			printf(" ");
		printf("Errors={");
		for(i=0;i<current_ndims;i++)
			printf("%f,",p->ErrorMeasure[i]);
		printf("}\n");
		for(i=0;i<idlevel;i++)
			printf(" ");
		printf("Mins={");
		for(i=0;i<current_ndims;i++)
			printf("%d,",p->Mins[i]);
		printf("} Maxs={");
		for(i=0;i<current_ndims;i++)
			printf("%d,",p->Maxs[i]);
		printf("}\n");
		PrintQTree(p->Children[0],idlevel+2);
		PrintQTree(p->Children[1],idlevel+2);
	}
}

static void UpdateStats(struct QuantizedValue *v)
{
	// first, find mean
	int32 Means[MAXDIMS];
	double Errors[MAXDIMS];
	double WorstError[MAXDIMS];
	int i,j;
  
	memset(Means,0,sizeof(Means));
	int N=0;
	for(i=0;i<v->NSamples;i++)
	{
		struct Sample *s=SAMPLE(v->Samples,i);
		N+=s->Count;
		for(j=0;j<current_ndims;j++)
		{
			uint8 val=s->Value[j];
			Means[j]+=val*s->Count;
		}
	}
	for(j=0;j<current_ndims;j++)
	{
		if (N) v->Mean[j]=(uint8) (Means[j]/N);
		Errors[j]=WorstError[j]=0.;
	}
	for(i=0;i<v->NSamples;i++)
	{
		struct Sample *s=SAMPLE(v->Samples,i);
		double c=s->Count;
		for(j=0;j<current_ndims;j++)
		{
			double diff=SQ(s->Value[j]-v->Mean[j]);
			Errors[j]+=c*diff; // charles uses abs not sq()
			if (diff>WorstError[j])
				WorstError[j]=diff;
		}
	}
	v->TotalError=0.;
	double ErrorScale=1.; // /sqrt((double) (N));
	for(j=0;j<current_ndims;j++)
	{
		v->ErrorMeasure[j]=(ErrorScale*Errors[j]*current_weights[j]);
		v->TotalError+=v->ErrorMeasure[j];
#if SPLIT_THEN_SORT
		v->ErrorMeasure[j]*=WorstError[j];
#endif
	}
	v->TotSamples=N;
}

static int ErrorDim;
static double ErrorVal;
static struct QuantizedValue *ErrorNode;

static void UpdateWorst(struct QuantizedValue *q)
{
	if (q->Children[0])
	{
		// not a leaf node
		UpdateWorst(q->Children[0]);
		UpdateWorst(q->Children[1]);
	}
	else
	{
		if (q->TotalError>ErrorVal)
		{
			ErrorVal=q->TotalError;
			ErrorNode=q;
			ErrorDim=0;
			for(int d=0;d<current_ndims;d++)
				if (q->ErrorMeasure[d]>q->ErrorMeasure[ErrorDim])
					ErrorDim=d;
		}
	}
}

static int FindWorst(void)
{
	ErrorVal=-1.;
	UpdateWorst(current_root);
	return (ErrorVal>0);
}



static void SubdivideNode(struct QuantizedValue *n, int whichdim)
{
	int NAdded=0;
	int i;

#if SPLIT_THEN_SORT
	// we will try the "split then sort" method. This works by finding the
	// means for all samples above and below the mean along the given axis.
	// samples are then split into two groups, with the selection based upon
	// which of the n-dimensional means the sample is closest to.
	double LocalMean[MAXDIMS][2];
	int totsamps[2];
	for(i=0;i<current_ndims;i++)
		LocalMean[i][0]=LocalMean[i][1]=0.;
	totsamps[0]=totsamps[1]=0;
	uint8 minv=255;
	uint8 maxv=0;
	struct Sample *minS=0,*maxS=0;
	for(i=0;i<n->NSamples;i++)
	{
		uint8 v;
		int whichside=1;
		struct Sample *sl;
		sl=SAMPLE(n->Samples,i);
		v=sl->Value[whichdim];
		if (v<minv) { minv=v; minS=sl; }
		if (v>maxv) { maxv=v; maxS=sl; }
		if (v<n->Mean[whichdim])
			whichside=0;
		totsamps[whichside]+=sl->Count;
		for(int d=0;d<current_ndims;d++)
			LocalMean[d][whichside]+=
				sl->Count*sl->Value[d];
	}

	if (totsamps[0] && totsamps[1])
		for(i=0;i<current_ndims;i++)
		{
			LocalMean[i][0]/=totsamps[0];
			LocalMean[i][1]/=totsamps[1];
		}
	else
	{
		// it is possible that the clustering failed to split the samples.
		// this can happen with a heavily biased sample (i.e. all black
		// with a few stars). If this happens, we will cluster around the
		// extrema instead. LocalMean[i][0] will be the point with the lowest
		// value on the dimension and LocalMean[i][1] the one with the lowest
		// value.
		for(i=0;i<current_ndims;i++)
		{
			LocalMean[i][0]=minS->Value[i];
			LocalMean[i][1]=maxS->Value[i];
		}
	}

	// now, we have 2 n-dimensional means. We will label each sample
	// for which one it is nearer to by using the QNum field.
	for(i=0;i<n->NSamples;i++)
	{
		double dist[2];
		dist[0]=dist[1]=0.;
		struct Sample *s=SAMPLE(n->Samples,i);
		for(int d=0;d<current_ndims;d++)
			for(int w=0;w<2;w++)
				dist[w]+=current_weights[d]*SQ(LocalMean[d][w]-s->Value[d]);
		s->QNum=(dist[0]<dist[1]);
    }


	// hey ho! we have now labelled each one with a candidate bin. Let's
	// sort the array by moving the 0-labelled ones to the head of the array.
	n->sortdim=-1;
	qsort(n->Samples,n->NSamples,current_ssize,QNumSort);
	for(i=0;i<n->NSamples;i++,NAdded++)
		if (SAMPLE(n->Samples,i)->QNum)
			break;
  
#else
	if (whichdim != n->sortdim)
	{
		current_sort_dim=whichdim;
		qsort(n->Samples,n->NSamples,current_ssize,samplesort);
		n->sortdim=whichdim;
	}
	// now, the samples are sorted along the proper dimension.  we need
	// to find the place to cut in order to split the node.  this is
	// complicated by the fact that each sample entry can represent many
	// samples. What we will do is start at the beginning of the array,
	// adding samples to the first node, until either the number added
	// is >=TotSamples/2, or there is only one left.
	int TotAdded=0;
	for(;;)
	{
		if (NAdded==n->NSamples-1)
			break;
		if (TotAdded>=n->TotSamples/2)
			break;
		TotAdded+=SAMPLE(n->Samples,NAdded)->Count;
		NAdded++;
	}
#endif
	struct QuantizedValue *a=AllocQValue();
	a->sortdim=n->sortdim;
	a->Samples=n->Samples;
	a->NSamples=NAdded;
	n->Children[0]=a;
	UpdateStats(a);
	a=AllocQValue();
	a->Samples=SAMPLE(n->Samples,NAdded);
	a->NSamples=n->NSamples-NAdded;
	a->sortdim=n->sortdim;
	n->Children[1]=a;
	UpdateStats(a);
}

static int colorid=0;

static void Label(struct QuantizedValue *q, int updatecolor)
{
	// fill in max/min values for tree, etc.
	if (q)
	{
		Label(q->Children[0],updatecolor);
		Label(q->Children[1],updatecolor);
		if (! q->Children[0])	// leaf node?
		{
			if (updatecolor)
			{
				q->value=colorid++;
				for(int j=0;j<q->NSamples;j++)
				{
					SAMPLE(q->Samples,j)->QNum=q->value;
					SAMPLE(q->Samples,j)->qptr=q;
				}
			}
			for(int i=0;i<current_ndims;i++)
			{
				q->Mins[i]=q->Mean[i];
				q->Maxs[i]=q->Mean[i];
			}
		}
		else
			for(int i=0;i<current_ndims;i++)
			{
				q->Mins[i]=min(q->Children[0]->Mins[i],q->Children[1]->Mins[i]);
				q->Maxs[i]=max(q->Children[0]->Maxs[i],q->Children[1]->Maxs[i]);
			}
	}
}    

struct QuantizedValue *FindQNode(struct QuantizedValue const *q, int32 code)
{
	if (! (q->Children[0]))
		if (code==q->value) return (struct QuantizedValue *) q;
		else return 0;
	else
	{
		struct QuantizedValue *found=FindQNode(q->Children[0],code);
		if (! found) found=FindQNode(q->Children[1],code);
		return found;
	}
}


void CheckInRange(struct QuantizedValue *q, uint8 *max, uint8 *min)
{
	if (q)
	{
		if (q->Children[0])
		{
			// non-leaf node
			CheckInRange(q->Children[0],q->Maxs, q->Mins);
			CheckInRange(q->Children[1],q->Maxs, q->Mins);
			CheckInRange(q->Children[0],max, min);
			CheckInRange(q->Children[1],max, min);
		}
		for (int i=0;i<current_ndims;i++)
		{
			if (q->Maxs[i]>max[i]) printf("error1\n");
			if (q->Mins[i]<min[i]) printf("error2\n");
		}
	}
}

struct QuantizedValue *Quantize(struct Sample *s, int nsamples, int ndims,
								int nvalues, uint8 *weights, int firstvalue)
{
	SetNDims(ndims);
	current_weights=weights;
	current_root=AllocQValue();
	current_root->Samples=s;
	current_root->NSamples=nsamples;
	UpdateStats(current_root);
	while(--nvalues)
	{
		if (! FindWorst())
			break;                          // if <n unique ones, stop now
		SubdivideNode(ErrorNode,ErrorDim);
	}
	colorid=firstvalue;
	Label(current_root,1);
	return current_root;
}

double MinimumError(struct QuantizedValue const *q, uint8 const *sample,
					int ndims, uint8 const *weights)
{
	double err=0;
	for(int i=0;i<ndims;i++)
	{
		int val1;
		int val2=sample[i];
		if ((q->Mins[i]<=val2) && (q->Maxs[i]>=val2)) val1=val2;
		else
		{
			val1=(val2<=q->Mins[i])?q->Mins[i]:q->Maxs[i];
		}
		err+=weights[i]*SQ(val1-val2);
	}
	return err;
}

double MaximumError(struct QuantizedValue const *q, uint8 const *sample,
					int ndims, uint8 const *weights)
{
	double err=0;
	for(int i=0;i<ndims;i++)
	{
		int val2=sample[i];
		int val1=(abs(val2-q->Mins[i])>abs(val2-q->Maxs[i]))?
			q->Mins[i]:
			q->Maxs[i];
		err+=weights[i]*SQ(val2-val1);
	}
	return err;
}

				     

// heap (priority queue) routines used for nearest-neghbor searches
struct FHeap {
	int heap_n;
	double *heap[MAXQUANT];
};

void InitHeap(struct FHeap *h)
{
  h->heap_n=0;
}


void UpHeap(int k, struct FHeap *h)
{
  double *tmpk=h->heap[k];
  double tmpkn=*tmpk;
  while((k>1) && (tmpkn <= *(h->heap[k/2])))
    {
      h->heap[k]=h->heap[k/2];
      k/=2;
    }
  h->heap[k]=tmpk;
}

void HeapInsert(struct FHeap *h,double *elem)
{
  h->heap_n++;
  h->heap[h->heap_n]=elem;
  UpHeap(h->heap_n,h);
}

void DownHeap(int k, struct FHeap *h)
{
  double *v=h->heap[k];
  while(k<=h->heap_n/2)
    {
      int j=2*k;
      if (j<h->heap_n)
	if (*(h->heap[j]) >= *(h->heap[j+1]))
	  j++;
      if (*v < *(h->heap[j]))
	{
	  h->heap[k]=v;
	  return;
	}
      h->heap[k]=h->heap[j]; k=j;
    }
  h->heap[k]=v;
}

void *RemoveHeapItem(struct FHeap *h)
{
  void *ret=0;
  if (h->heap_n!=0)
    {
      ret=h->heap[1];
      h->heap[1]=h->heap[h->heap_n];
      h->heap_n--;
      DownHeap(1,h);
    }
  return ret;
}

// now, nearest neighbor finder. Use a heap to traverse the tree, stopping
// when there are no nodes with a minimum error < the current error.

struct FHeap TheQueue;

#define PUSHNODE(a) { \
  (a)->MinError=MinimumError(a,sample,ndims,weights); \
  if ((a)->MinError < besterror) HeapInsert(&TheQueue,&(a)->MinError); \
 }

struct QuantizedValue *FindMatch(uint8 const *sample, int ndims,
								 uint8 *weights, struct QuantizedValue *q)
{
	InitHeap(&TheQueue);
	struct QuantizedValue *bestmatch=0;
	double besterror=1.0e63;
	PUSHNODE(q);
	for(;;)
	{
		struct QuantizedValue *test=(struct QuantizedValue *)
			RemoveHeapItem(&TheQueue);
		if (! test) break;		// heap empty
//    printf("got pop node =%p minerror=%f\n",test,test->MinError);
    
		if (test->MinError>besterror) break;
		if (test->Children[0])
		{
			// it's a parent node. put the children on the queue
			struct QuantizedValue *c1=test->Children[0];
			struct QuantizedValue *c2=test->Children[1];
			c1->MinError=MinimumError(c1,sample,ndims,weights);
			if (c1->MinError < besterror)
				HeapInsert(&TheQueue,&(c1->MinError));
			c2->MinError=MinimumError(c2,sample,ndims,weights);
			if (c2->MinError < besterror)
				HeapInsert(&TheQueue,&(c2->MinError));
		}
		else
		{
			// it's a leaf node. This must be a new minimum or the MinError
			// test would have failed.
			if (test->MinError < besterror)
			{
				bestmatch=test;
				besterror=test->MinError;
			}
		}
	}
	if (bestmatch)
	{
		SquaredError+=besterror;
		bestmatch->NQuant++;
		for(int i=0;i<ndims;i++)
			bestmatch->Sums[i]+=sample[i];
	}
	return bestmatch;
}

static void RecalcMeans(struct QuantizedValue *q)
{
	if (q)
	{
		if (q->Children[0])
		{
			// not a leaf, invoke recursively.
			RecalcMeans(q->Children[0]);
			RecalcMeans(q->Children[0]);
		}
		else
		{
			// it's a leaf. Set the means
			if (q->NQuant)
			{
				for(int i=0;i<current_ndims;i++)
				{
					q->Mean[i]=(uint8) (q->Sums[i]/q->NQuant);
					q->Sums[i]=0;
				}
				q->NQuant=0;
			}
		}
	}
}
		      
void OptimizeQuantizer(struct QuantizedValue *q, int ndims)
{
	SetNDims(ndims);
	RecalcMeans(q);		// reset q values
	Label(q,0);			// update max/mins
}


static void RecalcStats(struct QuantizedValue *q)
{
	if (q)
	{
		UpdateStats(q);
		RecalcStats(q->Children[0]);
		RecalcStats(q->Children[1]);
	}
}

void RecalculateValues(struct QuantizedValue *q, int ndims)
{
	SetNDims(ndims);
	RecalcStats(q);
	Label(q,0);
}
