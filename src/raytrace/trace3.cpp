//========= Copyright Valve Corporation, All rights reserved. ============//

#include "raytrace.h"
#include <bspfile.h>
#include "bsplib.h"

static Vector VertCoord(dface_t const &f, int vnum)
{
	int eIndex = dsurfedges[f.firstedge+vnum];
	int point;
	if( eIndex < 0 )
	{
		point = dedges[-eIndex].v[1];
	}
	else
	{
		point = dedges[eIndex].v[0];
	}
	dvertex_t *v=dvertexes+point;
	return Vector(v->point[0],v->point[1],v->point[2]);

}

Vector colors[]={
	Vector(0.5,0.5,1),
	Vector(0.5,1,0.5),
	Vector(0.5,1,1),
	Vector(1,0.5,0.5),
	Vector(1,0.5,1),
	Vector(1,1,1)};

void RayTracingEnvironment::AddBSPFace(int id,dface_t const &face)
{
	if (face.dispinfo!=-1)									// displacements must be dealt with elsewhere
		return;
	texinfo_t *tx =(face.texinfo>=0)?&(texinfo[face.texinfo]):0;
// 	if (tx && (tx->flags & (SURF_SKY|SURF_NODRAW)))
// 		return;
	if (tx)
	{
		printf("id %d flags=%x\n",id,tx->flags);
	}
	printf("side: ");
	for(int v=0;v<face.numedges;v++)
	{
		printf("(%f %f %f) ",XYZ(VertCoord(face,v)));
	}
	printf("\n");
	int ntris=face.numedges-2;
	for(int tri=0;tri<ntris;tri++)
	{
		
		AddTriangle(id,VertCoord(face,0),VertCoord(face,(tri+1)%face.numedges),
					VertCoord(face,(tri+2)%face.numedges),Vector(1,1,1)); //colors[id % NELEMS(colors)]);
	}
}

void RayTracingEnvironment::InitializeFromLoadedBSP(void)
{
// 	CUtlVector<uint8> PlanesToSkip;
// 	SidesToSkip.EnsureCapacity(numplanes);
// 	for(int s=0;s<numplanes;s++)
// 		SidesToSkip.AddToTail(0);
// 	for(int b=0;b<numbrushes;b++)
// 		if ((dbrushes[b].contents & MASK_OPAQUE)==0)
// 		{
// 			// transparent brush - mark all its sides as "do not process"
// 			for(int s=0;s<dbrushes[b].numsides;s++)
// 			{
// 				PlanesToSkip[s+dbrushes[b].firstside]=1;
// 			}

// 		}
// 	// now, add all origfaces, omitting those whose sides are the ones we marked previously
// 	for(int c=0;c<numorigfaces;c++)
// 	{
// 		dface_t const &f=dorigfaces[c];
// 		if (SidesToSkip[f.AddBSPFace(c,dorigfaces[c]);
// 	}
	


// 	// ugly - I want to traverse all the faces. but there is no way to get from a face back to it's
// 	// original brush, and I need to get back to the face to the contents field of the brush.  So I
// 	// will create a temporary mapping from a "side" to its brush. I can get from the face to it
// 	// side, which can get me back to its brush.
	
// 	CUtlVector<uint8> OrigFaceVisited;
// 	OrigFaceVisited.EnsureCapacity(numorigfaces);
// 	int n_added=0;

// 	for(int i=0;i<numorigfaces;i++)
// 		OrigFaceVisited.AddToTail(0);

// 	for(int l=0;l<numleafs;l++)
// 	{
// 		dleaf_t const &lf=dleafs[l];
// //		if (lf.contents & MASK_OPAQUE)
// 		{
// 			for(int f=0;f<lf.numleaffaces;f++);
// 			{
// 				dface_t const &face=dfaces[f+lf.firstleafface];
// 				if (OrigFaceVisited[face.origFace]==0)
// 				{
// 					dface_t const &oface=dorigfaces[face.origFace];
// 					OrigFaceVisited[face.origFace]=1;
// 					n_added++;
// 					AddBSPFace(face.origFace,oface);
// 				}
// 			}
// 		}
// 	}
// 	printf("added %d of %d\n",n_added,numorigfaces);
// 	for(int c=0;c<numorigfaces;c++)
// 	{
// 		dface_t const &f=dorigfaces[c];
// 		AddBSPFace(c,dorigfaces[c]);
// 	}
	for(int c=0;c<numfaces;c++)
	{
//		dface_t const &f=dfaces[c];
		AddBSPFace(c,dorigfaces[c]);
	}

//	AddTriangle(1234,Vector(51,145,-700),Vector(71,165,-700),Vector(51,165,-700),colors[5]);
}

