//========= Copyright Valve Corporation, All rights reserved. ============//
// $Id$
#include "raytrace.h"
#include <mathlib/halton.h>

static uint32 MapDistanceToPixel(float t)
{
	if (t<0) return 0xffff0000;
	if (t>100) return 0xff000000;
	int a=t*1000; a&=0xff;
	int b=t*10; b &=0xff;
	int c=t*.01; c &=0xff;
	return 0xff000000+(a<<16)+(b<<8)+c;
}

#define IGAMMA (1.0/2.2)

#define MAGIC_NUMBER (1<<23)

static fltx4 Four_MagicNumbers={ MAGIC_NUMBER, MAGIC_NUMBER, MAGIC_NUMBER, MAGIC_NUMBER };
static ALIGN16 int32 Four_255s[4]= {0xff,0xff,0xff,0xff};
#define PIXMASK ( * ( reinterpret_cast< fltx4 *>( &Four_255s ) ) )

void MapLinearIntensities(FourVectors const &intens,uint32 *p1, uint32 *p2, uint32 *p3, uint32 *p4)
{
	// convert four pixels worth of sse-style rgb into argb lwords
	// NOTE the _mm_empty macro is voodoo. do not mess with this routine casually - simply throwing
	// anything that ends up generating a fpu stack references in here would be bad news.
	static fltx4 pixscale={255.0,255.0,255.0,255.0};
	fltx4 r,g,b;
	r=MinSIMD(pixscale,MulSIMD(pixscale,PowSIMD(intens.x,IGAMMA)));
	g=MinSIMD(pixscale,MulSIMD(pixscale,PowSIMD(intens.y,IGAMMA)));
	b=MinSIMD(pixscale,MulSIMD(pixscale,PowSIMD(intens.z,IGAMMA)));
	// now, convert to integer
	r=AndSIMD( AddSIMD( r, Four_MagicNumbers ), PIXMASK );
	g=AndSIMD( AddSIMD( g, Four_MagicNumbers ), PIXMASK );
	b=AndSIMD( AddSIMD( b, Four_MagicNumbers ), PIXMASK );

	*(p1)=(SubInt(r, 0))|(SubInt(g, 0)<<8)|(SubInt(b, 0)<<16);
	*(p2)=(SubInt(r, 1))|(SubInt(g, 1)<<8)|(SubInt(b, 1)<<16);
	*(p3)=(SubInt(r, 2))|(SubInt(g, 2)<<8)|(SubInt(b, 2)<<16);
	*(p4)=(SubInt(r, 3))|(SubInt(g, 3)<<8)|(SubInt(b, 3)<<16);
}

static ALIGN16 int32 signmask[4]={0x80000000,0x80000000,0x80000000,0x80000000};
static ALIGN16 int32 all_ones[4]={-1,-1,-1,-1};
static fltx4 all_zeros={0,0,0,0};
static fltx4 TraceLimit={1.0e20,1.0e20,1.0e20,1.0e20};

void RayTracingEnvironment::RenderScene(
	int width, int height,								   // width and height of desired rendering
	int stride,											 // actual width in pixels of target buffer
	uint32 *output_buffer,									// pointer to destination 
	Vector CameraOrigin,									// eye position
	Vector ULCorner,										// word space coordinates of upper left
															// monitor corner
	Vector URCorner,										// top right corner
	Vector LLCorner,										// lower left
	Vector LRCorner,										// lower right
	RayTraceLightingMode_t lmode)
{
	// first, compute deltas
	Vector dxvector=URCorner;
	dxvector-=ULCorner;
	dxvector*=(1.0/width);
	Vector dxvectortimes2=dxvector;
	dxvectortimes2+=dxvector;

	Vector dyvector=LLCorner;
	dyvector-=ULCorner;
	dyvector*=(1.0/height);


	// block_offsets-relative offsets for eahc of the 4 pixels in the block, in sse format
	FourVectors block_offsets;
	block_offsets.LoadAndSwizzle(Vector(0,0,0),dxvector,dyvector,dxvector+dyvector);
	
	FourRays myrays;
	myrays.origin.DuplicateVector(CameraOrigin);
	
	// tmprays is used fo rthe case when we cannot trace 4 rays at once.
	FourRays tmprays;
	tmprays.origin.DuplicateVector(CameraOrigin);

	// now, we will ray trace pixels. we will do the rays in a 2x2 pattern
	for(int y=0;y<height;y+=2)
	{
		Vector SLoc=dyvector;
		SLoc*=((float) y);
		SLoc+=ULCorner;
		uint32 *dest=output_buffer+y*stride;
		for(int x=0;x<width;x+=2)
		{
			myrays.direction.DuplicateVector(SLoc);
			myrays.direction+=block_offsets;
			myrays.direction.VectorNormalize();
			
			RayTracingResult rslt;
			Trace4Rays(myrays,all_zeros,TraceLimit, &rslt);
			if ((rslt.HitIds[0]==-1) && (rslt.HitIds[1]==-1) && 
				(rslt.HitIds[2]==-1) && (rslt.HitIds[3]==-1))
				MapLinearIntensities(BackgroundColor,dest,dest+1,dest+stride,dest+stride+1);
			else
			{
				// make sure normal points back towards ray origin
				fltx4 ndoti=rslt.surface_normal*myrays.direction;
				fltx4 bad_dirs=AndSIMD(CmpGtSIMD(ndoti,Four_Zeros),
										   LoadAlignedSIMD((float *) signmask));

				// flip signs of all "wrong" normals
				rslt.surface_normal.x=XorSIMD(bad_dirs,rslt.surface_normal.x);
				rslt.surface_normal.y=XorSIMD(bad_dirs,rslt.surface_normal.y);
				rslt.surface_normal.z=XorSIMD(bad_dirs,rslt.surface_normal.z);

				FourVectors intens;
				intens.DuplicateVector(Vector(0,0,0));
				// set up colors
				FourVectors surf_colors;
				surf_colors.DuplicateVector(Vector(0,0,0));
				for(int i=0;i<4;i++)
				{
					if (rslt.HitIds[i]>=0)
					{
						surf_colors.X(i)=TriangleColors[rslt.HitIds[i]].x;
						surf_colors.Y(i)=TriangleColors[rslt.HitIds[i]].y;
						surf_colors.Z(i)=TriangleColors[rslt.HitIds[i]].z;
					}

				}
				FourVectors surface_pos=myrays.direction;
				surface_pos*=rslt.HitDistance;
				surface_pos+=myrays.origin;
				
				switch(lmode)
				{
					case DIRECT_LIGHTING:
					{
						// light all points
						for(int l=0;l<LightList.Count();l++)
						{
							LightList[l].ComputeLightAtPoints(surface_pos,rslt.surface_normal,
															  intens);
						}
					}
					break;

					case DIRECT_LIGHTING_WITH_SHADOWS:
					{
						// light all points
						for(int l=0;l<LightList.Count();l++)
						{
							FourVectors ldir;
							ldir.DuplicateVector(LightList[l].m_Position);
							ldir-=surface_pos;
							fltx4 MaxT=ldir.length();
							ldir.VectorNormalizeFast();
							// now, compute shadow flag
							FourRays myrays;
							myrays.origin=surface_pos;
							FourVectors epsilon=ldir;
							epsilon*=0.01;
							myrays.origin+=epsilon;
							myrays.direction=ldir;
							RayTracingResult shadowtest;
							Trace4Rays(myrays,Four_Zeros,MaxT, &shadowtest);
							fltx4 unshadowed=CmpGtSIMD(shadowtest.HitDistance,MaxT);
							if (! (IsAllZeros(unshadowed)))
							{
								FourVectors tmp;
								tmp.DuplicateVector(Vector(0,0,0));
								LightList[l].ComputeLightAtPoints(surface_pos,rslt.surface_normal,
																  tmp);
								intens.x=AddSIMD(intens.x,AndSIMD(tmp.x,unshadowed));
								intens.y=AddSIMD(intens.y,AndSIMD(tmp.y,unshadowed));
								intens.z=AddSIMD(intens.z,AndSIMD(tmp.z,unshadowed));
							}
						}
					}
					break;
				}
				// now, mask off non-hitting pixels
				intens.VProduct(surf_colors);
				fltx4 no_hit_mask=CmpGtSIMD(rslt.HitDistance,TraceLimit);
				
				intens.x=OrSIMD(AndSIMD(BackgroundColor.x,no_hit_mask),
								   AndNotSIMD(no_hit_mask,intens.x));
				intens.y=OrSIMD(AndSIMD(BackgroundColor.y,no_hit_mask),
								   AndNotSIMD(no_hit_mask,intens.y));
				intens.z=OrSIMD(AndSIMD(BackgroundColor.z,no_hit_mask),
								   AndNotSIMD(no_hit_mask,intens.z));

				MapLinearIntensities(intens,dest,dest+1,dest+stride,dest+stride+1);
			}
			dest+=2;
			SLoc+=dxvectortimes2;
		}
	}
}




#define SQ(x) ((x)*(x))

void RayTracingEnvironment::ComputeVirtualLightSources(void)
{
	int start_pos=0;
	for(int b=0;b<3;b++)
	{
		int nl=LightList.Count();
		int where_to_start=start_pos;
		start_pos=nl;
		for(int l=where_to_start;l<nl;l++)
		{
			DirectionalSampler_t sample_generator;
			int n_desired=1*LightList[l].m_Color.Length();
			if (LightList[l].m_Type==MATERIAL_LIGHT_SPOT)
				n_desired*=LightList[l].m_Phi/2;
			for(int try1=0;try1<n_desired;try1++)
			{
				LightDesc_t const &li=LightList[l];
				FourRays myrays;
				myrays.origin.DuplicateVector(li.m_Position);
				RayTracingResult rslt;
				Vector trial_dir=sample_generator.NextValue();
				if (li.IsDirectionWithinLightCone(trial_dir))
				{
					myrays.direction.DuplicateVector(trial_dir);
					Trace4Rays(myrays,all_zeros,ReplicateX4(1000.0), &rslt);
					if ((rslt.HitIds[0]!=-1))
					{
						// make sure normal points back towards ray origin
						fltx4 ndoti=rslt.surface_normal*myrays.direction;
						fltx4 bad_dirs=AndSIMD(CmpGtSIMD(ndoti,Four_Zeros),
												   LoadAlignedSIMD((float *) signmask));
						
						// flip signs of all "wrong" normals
						rslt.surface_normal.x=XorSIMD(bad_dirs,rslt.surface_normal.x);
						rslt.surface_normal.y=XorSIMD(bad_dirs,rslt.surface_normal.y);
						rslt.surface_normal.z=XorSIMD(bad_dirs,rslt.surface_normal.z);

						// a hit! let's make a virtual light source

						// treat the virtual light as a disk with its center at the hit position
						// and its radius scaled by the amount of the solid angle this probe
						// represents.
						float area_of_virtual_light=
							4.0*M_PI*SQ( SubFloat( rslt.HitDistance, 0 ) )*(1.0/n_desired);

						FourVectors intens;
						intens.DuplicateVector(Vector(0,0,0));

						FourVectors surface_pos=myrays.direction;
						surface_pos*=rslt.HitDistance;
						surface_pos+=myrays.origin;
						FourVectors delta=rslt.surface_normal;
						delta*=0.1;
						surface_pos+=delta;
						LightList[l].ComputeLightAtPoints(surface_pos,rslt.surface_normal,
														  intens);
						FourVectors surf_colors;
						surf_colors.DuplicateVector(TriangleColors[rslt.HitIds[0]]);
						intens*=surf_colors;
						// see if significant
						LightDesc_t l1;
						l1.m_Type=MATERIAL_LIGHT_SPOT;
						l1.m_Position=Vector(surface_pos.X(0),surface_pos.Y(0),surface_pos.Z(0));
						l1.m_Direction=Vector(rslt.surface_normal.X(0),rslt.surface_normal.Y(0),
											  rslt.surface_normal.Z(0));
						l1.m_Color=Vector(intens.X(0),intens.Y(0),intens.Z(0));
						if (l1.m_Color.Length()>0)
						{
							l1.m_Color*=area_of_virtual_light/M_PI;
							l1.m_Range=0.0;
							l1.m_Falloff=1.0;
							l1.m_Attenuation0=1.0;
							l1.m_Attenuation1=0.0;
							l1.m_Attenuation2=1.0;			// intens falls off as 1/r^2
							l1.m_Theta=0;
							l1.m_Phi=M_PI;
							l1.RecalculateDerivedValues();
							LightList.AddToTail(l1);
						}
					}
				}
			}
		}
	}
}



static unsigned int GetSignMask(Vector const &v)
{
	unsigned int ret=0;
	if (v.x<0.0)
		ret++;
	if (v.y<0)
		ret+=2;
	if (v.z<0)
		ret+=4;
	return ret;
}


inline void RayTracingEnvironment::FlushStreamEntry(RayStream &s,int msk)
{
	assert(msk>=0);
	assert(msk<8);
	fltx4 tmax=s.PendingRays[msk].direction.length();
	fltx4 scl=ReciprocalSaturateSIMD(tmax);
	s.PendingRays[msk].direction*=scl;					// normalize
	RayTracingResult tmpresult;
	Trace4Rays(s.PendingRays[msk],Four_Zeros,tmax,msk,&tmpresult);
	// now, write out results
	for(int r=0;r<4;r++)
	{
		RayTracingSingleResult *out=s.PendingStreamOutputs[msk][r];
		out->ray_length=SubFloat( tmax, r );
		out->surface_normal.x=tmpresult.surface_normal.X(r);
		out->surface_normal.y=tmpresult.surface_normal.Y(r);
		out->surface_normal.z=tmpresult.surface_normal.Z(r);
		out->HitID=tmpresult.HitIds[r];
		out->HitDistance=SubFloat( tmpresult.HitDistance, r );
	}
	s.n_in_stream[msk]=0;
}

void RayTracingEnvironment::AddToRayStream(RayStream &s,
										   Vector const &start,Vector const &end,
										   RayTracingSingleResult *rslt_out)
{
	Vector delta=end;
	delta-=start;
	int msk=GetSignMask(delta);
	assert(msk>=0);
	assert(msk<8);
	int pos=s.n_in_stream[msk];
	assert(pos<4);
	s.PendingRays[msk].origin.X(pos)=start.x;
	s.PendingRays[msk].origin.Y(pos)=start.y;
	s.PendingRays[msk].origin.Z(pos)=start.z;
	s.PendingRays[msk].direction.X(pos)=delta.x;
	s.PendingRays[msk].direction.Y(pos)=delta.y;
	s.PendingRays[msk].direction.Z(pos)=delta.z;
	s.PendingStreamOutputs[msk][pos]=rslt_out;
	if (pos==3)
	{
		FlushStreamEntry(s,msk);
	}
	else
		s.n_in_stream[msk]++;
}

void RayTracingEnvironment::FinishRayStream(RayStream &s)
{
	for(int msk=0;msk<8;msk++)
	{
		int cnt=s.n_in_stream[msk];
		if (cnt)
		{
			// fill in unfilled entries with dups of first
			for(int c=cnt;c<4;c++)
			{
				s.PendingRays[msk].origin.X(c) = s.PendingRays[msk].origin.X(0);
				s.PendingRays[msk].origin.Y(c) = s.PendingRays[msk].origin.Y(0);
				s.PendingRays[msk].origin.Z(c) = s.PendingRays[msk].origin.Z(0);
				s.PendingRays[msk].direction.X(c) = s.PendingRays[msk].direction.X(0);
				s.PendingRays[msk].direction.Y(c) = s.PendingRays[msk].direction.Y(0);
				s.PendingRays[msk].direction.Z(c) = s.PendingRays[msk].direction.Z(0);
				s.PendingStreamOutputs[msk][c]=s.PendingStreamOutputs[msk][0];
			}
			FlushStreamEntry(s,msk);
		}
	}
}
