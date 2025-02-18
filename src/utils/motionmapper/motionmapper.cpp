//========= Copyright Valve Corporation, All rights reserved. ============//
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>
#include "filesystem_tools.h"
#include "cmdlib.h"
#include "scriplib.h"
#include "mathlib/mathlib.h"
#define EXTERN
#include "studio.h"
#include "motionmapper.h"
#include "tier1/strtools.h"
#include "tier0/icommandline.h"
#include "utldict.h"
#include <windows.h>
#include "UtlBuffer.h"
#include "utlsymbol.h"

bool g_quiet = false;
bool g_verbose = false;
char g_outfile[1024];
bool uselogfile = false;

char	g_szFilename[1024];
FILE	*g_fpInput;
char	g_szLine[4096];
int		g_iLinecount;

bool g_bZBrush = false;
bool g_bGaveMissingBoneWarning = false;


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : depth - 
//			*fmt - 
//			... - 
//-----------------------------------------------------------------------------
void vprint( int depth, const char *fmt, ... )
{
	char string[ 8192 ];
	va_list va;
	va_start( va, fmt );
	V_vsprintf_safe( string, fmt, va );
	va_end( va );

	FILE *fp = NULL;

	if ( uselogfile )
	{
		fp = fopen( "log.txt", "ab" );
	}

	while ( depth-- > 0 )
	{
		vprint( 0,  "  " );
		OutputDebugString( "  " );
		if ( fp )
		{
			fprintf( fp, "  " );
		}
	}

	::printf( "%s", string );
	OutputDebugString( string );

	if ( fp )
	{
		char *p = string;
		while ( *p )
		{
			if ( *p == '\n' )
			{
				fputc( '\r', fp );
			}
			fputc( *p, fp );
			p++;
		}
		fclose( fp );
	}
}


int k_memtotal;
void *kalloc( int num, int size )
{
	// vprint( 0,  "calloc( %d, %d )\n", num, size );
	// vprint( 0,  "%d ", num * size );
	k_memtotal += num * size;
	return calloc( num, size );
}

void kmemset( void *ptr, int value, int size )
{
	// vprint( 0,  "kmemset( %x, %d, %d )\n", ptr, value, size );
	memset( ptr, value, size );
	return;
}

static bool g_bFirstWarning = true;

void MdlWarning( const char *fmt, ... )
{
	va_list args;
	static char output[1024];

	if (g_quiet)
	{
		if (g_bFirstWarning)
		{
			vprint( 0, "%s :\n", fullpath );
			g_bFirstWarning = false;
		}
		vprint( 0, "\t");
	}

	vprint( 0, "WARNING: ");
	va_start( args, fmt );
	vprint( 0, fmt, args );
}


void MdlError( char const *fmt, ... )
{
	va_list		args;

	if (g_quiet)
	{
		if (g_bFirstWarning)
		{
			vprint( 0, "%s :\n", fullpath );
			g_bFirstWarning = false;
		}
		vprint( 0, "\t");
	}

	vprint( 0, "ERROR: ");
	va_start( args, fmt );
	vprint( 0, fmt, args );

	exit( -1 );
}

int OpenGlobalFile( char *src )
{
	int		time1;
	char	filename[1024];

	// local copy of string
	strcpy( filename, ExpandPath( src ) );

	// Ummm, path sanity checking
	int pathLength;
	int numBasePaths = CmdLib_GetNumBasePaths();
	// This is kinda gross. . . doing the same work in cmdlib on SafeOpenRead.
	if( CmdLib_HasBasePath( filename, pathLength ) )
	{
		char tmp[1024];
		int i;
		for( i = 0; i < numBasePaths; i++ )
		{
			strcpy( tmp, CmdLib_GetBasePath( i ) );
			strcat( tmp, filename + pathLength );
			
			time1 = FileTime( tmp );
			if( time1 != -1 )
			{
				if ((g_fpInput = fopen(tmp, "r")) == 0) 
				{
					MdlWarning( "reader: could not open file '%s'\n", src );
					return 0;
				}
				else
				{
					return 1;
				}
			}
		}
		return 0;
	}
	else
	{
		time1 = FileTime (filename);
		if (time1 == -1)
			return 0;

		// Whoohooo, FOPEN!
		if ((g_fpInput = fopen(filename, "r")) == 0) 
		{
			MdlWarning( "reader: could not open file '%s'\n", src );
			return 0;
		}

		return 1;
	}
}

bool IsEnd( char const* pLine )
{
	if (strncmp( "end", pLine, 3 ) != 0) 
		return false;
	return (pLine[3] == '\0') || (pLine[3] == '\n');
}


//Wrong name for the use of it.
void scale_vertex( Vector &org )
{
	org[0] = org[0] * g_currentscale;
	org[1] = org[1] * g_currentscale;
	org[2] = org[2] * g_currentscale;
}


void clip_rotations( RadianEuler& rot )
{
	int j;
	// clip everything to : -M_PI <= x < M_PI

	for (j = 0; j < 3; j++) {
		while (rot[j] >= M_PI) 
			rot[j] -= M_PI*2;
		while (rot[j] < -M_PI) 
			rot[j] += M_PI*2;
	}
}


void clip_rotations( Vector& rot )
{
	int j;
	// clip everything to : -180 <= x < 180

	for (j = 0; j < 3; j++) {
		while (rot[j] >= 180) 
			rot[j] -= 180*2;
		while (rot[j] < -180) 
			rot[j] += 180*2;
	}
}


void Build_Reference( s_source_t *psource)
{
	int		i, parent;
	Vector	angle;

	for (i = 0; i < psource->numbones; i++)
	{
		matrix3x4_t m;
		AngleMatrix( psource->rawanim[0][i].rot, m );
		m[0][3] = psource->rawanim[0][i].pos[0];
		m[1][3] = psource->rawanim[0][i].pos[1];
		m[2][3] = psource->rawanim[0][i].pos[2];

		parent = psource->localBone[i].parent;
		if (parent == -1) 
		{
			// scale the done pos.
			// calc rotational matrices
			MatrixCopy( m, psource->boneToPose[i] );
		}
		else 
		{
			// calc compound rotational matrices
			// FIXME : Hey, it's orthogical so inv(A) == transpose(A)
			ConcatTransforms( psource->boneToPose[parent], m, psource->boneToPose[i] );
		}
		// vprint( 0, "%3d %f %f %f\n", i, psource->bonefixup[i].worldorg[0], psource->bonefixup[i].worldorg[1], psource->bonefixup[i].worldorg[2] );
		/*
		AngleMatrix( angle, m );
		vprint( 0, "%8.4f %8.4f %8.4f\n", m[0][0], m[1][0], m[2][0] );
		vprint( 0, "%8.4f %8.4f %8.4f\n", m[0][1], m[1][1], m[2][1] );
		vprint( 0, "%8.4f %8.4f %8.4f\n", m[0][2], m[1][2], m[2][2] );
		*/
	}
}

int Grab_Nodes( s_node_t *pnodes )
{
	//
	// s_node_t structure: index is index!!
	//
	int index;
	char name[1024];
	int parent;
	int numbones = 0;

	// Init parent to none
	for (index = 0; index < MAXSTUDIOSRCBONES; index++)
	{
		pnodes[index].parent = -1;
	}

	// March through nodes lines
	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
	{
		g_iLinecount++;
		// get tokens
		if (sscanf( g_szLine, "%d \"%[^\"]\" %d", &index, name, &parent ) == 3)
		{
			// check for duplicated bones
			/*
			if (strlen(pnodes[index].name) != 0)
			{
				MdlError( "bone \"%s\" exists more than once\n", name );
			}
			*/
			// copy name to struct array
			V_strcpy_safe( pnodes[index].name, name );
			// set parent into struct array
			pnodes[index].parent = parent;
			// increment numbones
			if (index > numbones)
			{
				numbones = index;
			}
		}
		else 
		{
			return numbones + 1;
		}
	}
	MdlError( "Unexpected EOF at line %d\n", g_iLinecount );
	return 0;
}

void Grab_Vertexanimation( s_source_t *psource )
{
	char	cmd[1024];
	int		index;
	Vector	pos;
	Vector	normal;
	int		t = -1;
	int		count = 0;
	static s_vertanim_t	tmpvanim[MAXSTUDIOVERTS*4];

	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
	{
		g_iLinecount++;
		if (sscanf( g_szLine, "%d %f %f %f %f %f %f", &index, &pos[0], &pos[1], &pos[2], &normal[0], &normal[1], &normal[2] ) == 7)
		{
			if (psource->startframe < 0)
			{
				MdlError( "Missing frame start(%d) : %s", g_iLinecount, g_szLine );
			}

			if (t < 0)
			{
				MdlError( "VTA Frame Sync (%d) : %s", g_iLinecount, g_szLine );
			}

			tmpvanim[count].vertex = index;
			VectorCopy( pos, tmpvanim[count].pos );
			VectorCopy( normal, tmpvanim[count].normal );
			count++;

			if (index >= psource->numvertices)
				psource->numvertices = index + 1;
		}
		else
		{
			// flush data

			if (count)
			{
				psource->numvanims[t] = count;

				psource->vanim[t] = (s_vertanim_t *)kalloc( count, sizeof( s_vertanim_t ) );

				memcpy( psource->vanim[t], tmpvanim, count * sizeof( s_vertanim_t ) );
			}
			else if (t > 0)
			{
				psource->numvanims[t] = 0;
			}

			// next command
			if (sscanf( g_szLine, "%1023s %d", cmd, &index ))
			{
				if (strcmp( cmd, "time" ) == 0) 
				{
					t = index;
					count = 0;

					if (t < psource->startframe)
					{
						MdlError( "Frame MdlError(%d) : %s", g_iLinecount, g_szLine );
					}
					if (t > psource->endframe)
					{
						MdlError( "Frame MdlError(%d) : %s", g_iLinecount, g_szLine );
					}

					t -= psource->startframe;
				}
				else if (strcmp( cmd, "end") == 0) 
				{
					psource->numframes = psource->endframe - psource->startframe + 1;
					return;
				}
				else
				{
					MdlError( "MdlError(%d) : %s", g_iLinecount, g_szLine );
				}

			}
			else
			{
				MdlError( "MdlError(%d) : %s", g_iLinecount, g_szLine );
			}
		}
	}
	MdlError( "unexpected EOF: %s\n", psource->filename );
}

void Grab_Animation( s_source_t *psource )
{
	Vector pos;
	RadianEuler rot;
	char cmd[1024];
	int index;
	int	t = -99999999;
	int size;

	// Init startframe
	psource->startframe = -1;

	// size per frame
	size = psource->numbones * sizeof( s_bone_t );

	// march through animation
	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
	{
		// linecount
		g_iLinecount++;
		// split if big enoough
		if (sscanf( g_szLine, "%d %f %f %f %f %f %f", &index, &pos[0], &pos[1], &pos[2], &rot[0], &rot[1], &rot[2] ) == 7)
		{
			// startframe is sanity check for having determined time
			if (psource->startframe < 0)
			{
				MdlError( "Missing frame start(%d) : %s", g_iLinecount, g_szLine );
			}

			// scale if pertinent
			scale_vertex( pos );
			VectorCopy( pos, psource->rawanim[t][index].pos );
			VectorCopy( rot, psource->rawanim[t][index].rot );

			clip_rotations( rot ); // !!!
		}
		else if (sscanf( g_szLine, "%1023s %d", cmd, &index ))
		{
			// get time
			if (strcmp( cmd, "time" ) == 0) 
			{
				// again time IS an index
				t = index;
				if (psource->startframe == -1)
				{
					psource->startframe = t;
				}
				// sanity check time (little funny logic here, see previous IF)
				if (t < psource->startframe)
				{
					MdlError( "Frame MdlError(%d) : %s", g_iLinecount, g_szLine );
				}
				// bump up endframe?
				if (t > psource->endframe)
				{
					psource->endframe = t;
				}
				// make t into pure index
				t -= psource->startframe;

				// check for memory allocation
				if (psource->rawanim[t] == NULL)
				{
					// Allocate 1 frame of full bonecount 
					psource->rawanim[t] = (s_bone_t *)kalloc( 1, size );

					// duplicate previous frames keys?? preventative sanity?
					if (t > 0 && psource->rawanim[t-1])
					{
						for (int j = 0; j < psource->numbones; j++)
						{
							VectorCopy( psource->rawanim[t-1][j].pos, psource->rawanim[t][j].pos );
							VectorCopy( psource->rawanim[t-1][j].rot, psource->rawanim[t][j].rot );
						}
					}
				}
				else
				{
					// MdlError( "%s has duplicated frame %d\n", psource->filename, t );
				}
			}
			else if (strcmp( cmd, "end") == 0) 
			{
				psource->numframes = psource->endframe - psource->startframe + 1;

				for (t = 0; t < psource->numframes; t++)
				{
					if (psource->rawanim[t] == NULL)
					{
						MdlError( "%s is missing frame %d\n", psource->filename, t + psource->startframe );
					}
				}

				Build_Reference( psource );
				return;
			}
			else
			{
				MdlError( "MdlError(%d) : %s", g_iLinecount, g_szLine );
			}
		}
		else
		{
			MdlError( "MdlError(%d) : %s", g_iLinecount, g_szLine );
		}
	}

	MdlError( "unexpected EOF: %s\n", psource->filename );
}

int lookup_index( s_source_t *psource, int material, Vector& vertex, Vector& normal, Vector2D texcoord )
{
	int i;

	for (i = 0; i < numvlist; i++) 
	{
		if (v_listdata[i].m == material
			&& DotProduct( g_normal[i], normal ) > normal_blend
			&& VectorCompare( g_vertex[i], vertex )
			&& g_texcoord[i][0] == texcoord[0]
			&& g_texcoord[i][1] == texcoord[1])
		{
			v_listdata[i].lastref = numvlist;
			return i;
		}
	}
	if (i >= MAXSTUDIOVERTS) {
		MdlError( "too many indices in source: \"%s\"\n", psource->filename);
	}

	VectorCopy( vertex, g_vertex[i] );
	VectorCopy( normal, g_normal[i] );
	Vector2Copy( texcoord, g_texcoord[i] );

	v_listdata[i].v = i;
	v_listdata[i].m = material;
	v_listdata[i].n = i;
	v_listdata[i].t = i;

	v_listdata[i].firstref = numvlist;
	v_listdata[i].lastref = numvlist;

	numvlist = i + 1;
	return i;
}


void ParseFaceData( s_source_t *psource, int material, s_face_t *pFace )
{
	int index[3];
	int i, j;
	Vector p;
	Vector normal;
	Vector2D t;
	int		iCount, bones[MAXSTUDIOSRCBONES];
	float   weights[MAXSTUDIOSRCBONES];
	int bone;

	for (j = 0; j < 3; j++) 
	{
		memset( g_szLine, 0, sizeof( g_szLine ) );

		if (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) == NULL) 
		{
			MdlError("%s: error on g_szLine %d: %s", g_szFilename, g_iLinecount, g_szLine );
		}

		iCount = 0;

		g_iLinecount++;
		i = sscanf( g_szLine, "%d %f %f %f %f %f %f %f %f %d %d %f %d %f %d %f %d %f",
			&bone, 
			&p[0], &p[1], &p[2], 
			&normal[0], &normal[1], &normal[2], 
			&t[0], &t[1],
			&iCount,
			&bones[0], &weights[0], &bones[1], &weights[1], &bones[2], &weights[2], &bones[3], &weights[3] );
			
		if (i < 9) 
			continue;

		if (bone < 0 || bone >= psource->numbones) 
		{
			MdlError("bogus bone index\n%d %s :\n%s", g_iLinecount, g_szFilename, g_szLine );
		}

		//Scale face pos
		scale_vertex( p );
		
		// continue parsing more bones.
		// FIXME: don't we have a built in parser that'll do this?
		if (iCount > 4)
		{
			int k;
			int ctr = 0;
			char *token;
			for (k = 0; k < 18; k++)
			{
				while (g_szLine[ctr] == ' ')
				{
					ctr++;
				}
				token = strtok( &g_szLine[ctr], " " );
				ctr += strlen( token ) + 1;
			}
			for (k = 4; k < iCount && k < MAXSTUDIOSRCBONES; k++)
			{
				while (g_szLine[ctr] == ' ')
				{
					ctr++;
				}
				token = strtok( &g_szLine[ctr], " " );
				ctr += strlen( token ) + 1;

				bones[k] = atoi(token);

				token = strtok( &g_szLine[ctr], " " );
				ctr += strlen( token ) + 1;
			
				weights[k] = atof(token);
			}
			// vprint( 0, "%d ", iCount );

			//vprint( 0, "\n");
			//exit(1);
		}

		// adjust_vertex( p );
		// scale_vertex( p );

		// move vertex position to object space.
		// VectorSubtract( p, psource->bonefixup[bone].worldorg, tmp );
		// VectorTransform(tmp, psource->bonefixup[bone].im, p );

		// move normal to object space.
		// VectorCopy( normal, tmp );
		// VectorTransform(tmp, psource->bonefixup[bone].im, normal );
		// VectorNormalize( normal );

		// invert v
		t[1] = 1.0 - t[1];

		index[j] = lookup_index( psource, material, p, normal, t );

		if (i == 9 || iCount == 0)
		{
			g_bone[index[j]].numbones = 1;
			g_bone[index[j]].bone[0] = bone;
			g_bone[index[j]].weight[0] = 1.0;
		}
		else
		{
			iCount = SortAndBalanceBones( iCount, MAXSTUDIOBONEWEIGHTS, bones, weights );

			g_bone[index[j]].numbones = iCount;
			for (i = 0; i < iCount; i++)
			{
				g_bone[index[j]].bone[i] = bones[i];
				g_bone[index[j]].weight[i] = weights[i];
			}
		}
	}

	// pFace->material = material; // BUG
	pFace->a		= index[0];
	pFace->b		= index[1];
	pFace->c		= index[2];
	Assert( ((pFace->a & 0xF0000000) == 0) && ((pFace->b & 0xF0000000) == 0) && 
		((pFace->c & 0xF0000000) == 0) );

	if (flip_triangles)
	{
		j = pFace->b;  pFace->b  = pFace->c;  pFace->c  = j;
	}
}

int use_texture_as_material( int textureindex )
{
	if (g_texture[textureindex].material == -1)
	{
		// vprint( 0, "%d %d %s\n", textureindex, g_nummaterials, g_texture[textureindex].name );
		g_material[g_nummaterials] = textureindex;
		g_texture[textureindex].material = g_nummaterials++;
	}

	return g_texture[textureindex].material;
}

int material_to_texture( int material )
{
	int i;
	for (i = 0; i < g_numtextures; i++)
	{
		if (g_texture[i].material == material)
		{
			return i;
		}
	}
	return -1;
}

int lookup_texture( char *texturename, int maxlen )
{
	int i;

	Q_StripExtension( texturename, texturename, maxlen );

	for (i = 0; i < g_numtextures; i++) 
	{
		if (stricmp( g_texture[i].name, texturename ) == 0) 
		{
			return i;
		}
	}

	if (i >= MAXSTUDIOSKINS)
		MdlError("Too many materials used, max %d\n", ( int )MAXSTUDIOSKINS );

//	vprint( 0,  "texture %d = %s\n", i, texturename );
	V_strcpy_safe( g_texture[i].name, texturename );

	g_texture[i].material = -1;
	/*
	if (stristr( texturename, "chrome" ) != NULL) {
		texture[i].flags = STUDIO_NF_FLATSHADE | STUDIO_NF_CHROME;
	}
	else {
		texture[i].flags = 0;
	}
	*/
	g_numtextures++;
	return i;
}

int SortAndBalanceBones( int iCount, int iMaxCount, int bones[], float weights[] )
{
	int i;

	// collapse duplicate bone weights
	for (i = 0; i < iCount-1; i++)
	{
		int j;
		for (j = i + 1; j < iCount; j++)
		{
			if (bones[i] == bones[j])
			{
				weights[i] += weights[j];
				weights[j] = 0.0;
			}
		}
	}

	// do sleazy bubble sort
	int bShouldSort;
	do {
		bShouldSort = false;
		for (i = 0; i < iCount-1; i++)
		{
			if (weights[i+1] > weights[i])
			{
				int j = bones[i+1]; bones[i+1] = bones[i]; bones[i] = j;
				float w = weights[i+1]; weights[i+1] = weights[i]; weights[i] = w;
				bShouldSort = true;
			}
		}
	} while (bShouldSort);

	// throw away all weights less than 1/20th
	while (iCount > 1 && weights[iCount-1] < 0.05)
	{
		iCount--;
	}

	// clip to the top iMaxCount bones
	if (iCount > iMaxCount)
	{
		iCount = iMaxCount;
	}

	float t = 0;
	for (i = 0; i < iCount; i++)
	{
		t += weights[i];
	}

	if (t <= 0.0)
	{
		// missing weights?, go ahead and evenly share?
		// FIXME: shouldn't this error out?
		t = 1.0 / iCount;

		for (i = 0; i < iCount; i++)
		{
			weights[i] = t;
		}
	}
	else
	{
		// scale to sum to 1.0
		t = 1.0 / t;

		for (i = 0; i < iCount; i++)
		{
			weights[i] = weights[i] * t;
		}
	}

	return iCount;
}

int vlistCompare( const void *elem1, const void *elem2 )
{
	v_unify_t *u1 = &v_listdata[*(int *)elem1];
	v_unify_t *u2 = &v_listdata[*(int *)elem2];

	// sort by material
	if (u1->m < u2->m)
		return -1;
	if (u1->m > u2->m)
		return 1;

	// sort by last used
	if (u1->lastref < u2->lastref)
		return -1;
	if (u1->lastref > u2->lastref)
		return 1;

	return 0;
}

int faceCompare( const void *elem1, const void *elem2 )
{
	int i1 = *(int *)elem1;
	int i2 = *(int *)elem2;

	// sort by material
	if (g_face[i1].material < g_face[i2].material)
		return -1;
	if (g_face[i1].material > g_face[i2].material)
		return 1;

	// sort by original usage
	if (i1 < i2)
		return -1;
	if (i1 > i2)
		return 1;

	return 0;
}

#define SMALL_FLOAT 1e-12

// NOTE: This routine was taken (and modified) from NVidia's BlinnReflection demo
// Creates basis vectors, based on a vertex and index list.
// See the NVidia white paper 'GDC2K PerPixel Lighting' for a description
// of how this computation works
static void CalcTriangleTangentSpace( s_source_t *pSrc, int v1, int v2, int v3, 
									  Vector &sVect, Vector &tVect )
{
/*
	static bool firstTime = true;
	static FILE *fp = NULL;
	if( firstTime )
	{
		firstTime = false;
		fp = fopen( "crap.out", "w" );
	}
*/
    
	/* Compute the partial derivatives of X, Y, and Z with respect to S and T. */
	Vector2D t0( pSrc->texcoord[v1][0], pSrc->texcoord[v1][1] );
	Vector2D t1( pSrc->texcoord[v2][0], pSrc->texcoord[v2][1] );
	Vector2D t2( pSrc->texcoord[v3][0], pSrc->texcoord[v3][1] );
	Vector p0( pSrc->vertex[v1][0], pSrc->vertex[v1][1], pSrc->vertex[v1][2] );
	Vector p1( pSrc->vertex[v2][0], pSrc->vertex[v2][1], pSrc->vertex[v2][2] );
	Vector p2( pSrc->vertex[v3][0], pSrc->vertex[v3][1], pSrc->vertex[v3][2] );

	sVect.Init( 0.0f, 0.0f, 0.0f );
	tVect.Init( 0.0f, 0.0f, 0.0f );

	// x, s, t
	Vector edge01 = Vector( p1.x - p0.x, t1.x - t0.x, t1.y - t0.y );
	Vector edge02 = Vector( p2.x - p0.x, t2.x - t0.x, t2.y - t0.y );

	Vector cross;
	CrossProduct( edge01, edge02, cross );
	if( fabs( cross.x ) > SMALL_FLOAT )
	{
		sVect.x += -cross.y / cross.x;
		tVect.x += -cross.z / cross.x;
	}

	// y, s, t
	edge01 = Vector( p1.y - p0.y, t1.x - t0.x, t1.y - t0.y );
	edge02 = Vector( p2.y - p0.y, t2.x - t0.x, t2.y - t0.y );

	CrossProduct( edge01, edge02, cross );
	if( fabs( cross.x ) > SMALL_FLOAT )
	{
		sVect.y += -cross.y / cross.x;
		tVect.y += -cross.z / cross.x;
	}
	
	// z, s, t
	edge01 = Vector( p1.z - p0.z, t1.x - t0.x, t1.y - t0.y );
	edge02 = Vector( p2.z - p0.z, t2.x - t0.x, t2.y - t0.y );

	CrossProduct( edge01, edge02, cross );
	if( fabs( cross.x ) > SMALL_FLOAT )
	{
		sVect.z += -cross.y / cross.x;
		tVect.z += -cross.z / cross.x;
	}

	// Normalize sVect and tVect
	VectorNormalize( sVect );
	VectorNormalize( tVect );

/*
	// Calculate flat normal
	Vector flatNormal;
	edge01 = p1 - p0;
	edge02 = p2 - p0;
	CrossProduct( edge02, edge01, flatNormal );
	VectorNormalize( flatNormal );
	
	// Get the average position
	Vector avgPos = ( p0 + p1 + p2 ) / 3.0f;

	// Draw the svect
	Vector endS = avgPos + sVect * .2f;
	fvprint( 0,  fp, "2\n" );
	fvprint( 0,  fp, "%f %f %f 1.0 0.0 0.0\n", endS[0], endS[1], endS[2] );
	fvprint( 0,  fp, "%f %f %f 1.0 0.0 0.0\n", avgPos[0], avgPos[1], avgPos[2] );
	
	// Draw the tvect
	Vector endT = avgPos + tVect * .2f;
	fvprint( 0,  fp, "2\n" );
	fvprint( 0,  fp, "%f %f %f 0.0 1.0 0.0\n", endT[0], endT[1], endT[2] );
	fvprint( 0,  fp, "%f %f %f 0.0 1.0 0.0\n", avgPos[0], avgPos[1], avgPos[2] );
	
	// Draw the normal
	Vector endN = avgPos + flatNormal * .2f;
	fvprint( 0,  fp, "2\n" );
	fvprint( 0,  fp, "%f %f %f 0.0 0.0 1.0\n", endN[0], endN[1], endN[2] );
	fvprint( 0,  fp, "%f %f %f 0.0 0.0 1.0\n", avgPos[0], avgPos[1], avgPos[2] );
	
	// Draw the wireframe of the triangle in white.
	fvprint( 0,  fp, "2\n" );
	fvprint( 0,  fp, "%f %f %f 1.0 1.0 1.0\n", p0[0], p0[1], p0[2] );
	fvprint( 0,  fp, "%f %f %f 1.0 1.0 1.0\n", p1[0], p1[1], p1[2] );
	fvprint( 0,  fp, "2\n" );
	fvprint( 0,  fp, "%f %f %f 1.0 1.0 1.0\n", p1[0], p1[1], p1[2] );
	fvprint( 0,  fp, "%f %f %f 1.0 1.0 1.0\n", p2[0], p2[1], p2[2] );
	fvprint( 0,  fp, "2\n" );
	fvprint( 0,  fp, "%f %f %f 1.0 1.0 1.0\n", p2[0], p2[1], p2[2] );
	fvprint( 0,  fp, "%f %f %f 1.0 1.0 1.0\n", p0[0], p0[1], p0[2] );

	// Draw a slightly shrunken version of the geometry to hide surfaces
	Vector tmp0 = p0 - flatNormal * .1f;
	Vector tmp1 = p1 - flatNormal * .1f;
	Vector tmp2 = p2 - flatNormal * .1f;
	fvprint( 0,  fp, "3\n" );
	fvprint( 0,  fp, "%f %f %f 0.1 0.1 0.1\n", tmp0[0], tmp0[1], tmp0[2] );
	fvprint( 0,  fp, "%f %f %f 0.1 0.1 0.1\n", tmp1[0], tmp1[1], tmp1[2] );
	fvprint( 0,  fp, "%f %f %f 0.1 0.1 0.1\n", tmp2[0], tmp2[1], tmp2[2] );
		
	fflush( fp );
*/
}

typedef CUtlVector<int> CIntVector;

void CalcModelTangentSpaces( s_source_t *pSrc )
{
	// Build a map from vertex to a list of triangles that share the vert.
	int meshID;
	for( meshID = 0; meshID < pSrc->nummeshes; meshID++ )
	{
		s_mesh_t *pMesh = &pSrc->mesh[pSrc->meshindex[meshID]];
		CUtlVector<CIntVector> vertToTriMap;
		vertToTriMap.AddMultipleToTail( pMesh->numvertices );
		int triID;
		for( triID = 0; triID < pMesh->numfaces; triID++ )
		{
			s_face_t *pFace = &pSrc->face[triID + pMesh->faceoffset];
			vertToTriMap[pFace->a].AddToTail( triID );
			vertToTriMap[pFace->b].AddToTail( triID );
			vertToTriMap[pFace->c].AddToTail( triID );
		}

		// Calculate the tangent space for each triangle.
		CUtlVector<Vector> triSVect;
		CUtlVector<Vector> triTVect;
		triSVect.AddMultipleToTail( pMesh->numfaces );
		triTVect.AddMultipleToTail( pMesh->numfaces );
		for( triID = 0; triID < pMesh->numfaces; triID++ )
		{
			s_face_t *pFace = &pSrc->face[triID + pMesh->faceoffset];
			CalcTriangleTangentSpace( pSrc, 
				pMesh->vertexoffset + pFace->a, 
				pMesh->vertexoffset + pFace->b, 
				pMesh->vertexoffset + pFace->c, 
				triSVect[triID], triTVect[triID] );
		}	

		// calculate an average tangent space for each vertex.
		int vertID;
		for( vertID = 0; vertID < pMesh->numvertices; vertID++ )
		{
			const Vector &normal = pSrc->normal[vertID+pMesh->vertexoffset];
			Vector4D &finalSVect = pSrc->tangentS[vertID+pMesh->vertexoffset];
			Vector sVect, tVect;

			sVect.Init( 0.0f, 0.0f, 0.0f );
			tVect.Init( 0.0f, 0.0f, 0.0f );
			for( triID = 0; triID < vertToTriMap[vertID].Size(); triID++ )
			{
				sVect += triSVect[vertToTriMap[vertID][triID]];
				tVect += triTVect[vertToTriMap[vertID][triID]];
			}

			// In the case of zbrush, everything needs to be treated as smooth.
			if( g_bZBrush )
			{
				int vertID2;
				Vector vertPos1( pSrc->vertex[vertID][0], pSrc->vertex[vertID][1], pSrc->vertex[vertID][2] );
				for( vertID2 = 0; vertID2 < pMesh->numvertices; vertID2++ )
				{
					if( vertID2 == vertID )
					{
						continue;
					}
					Vector vertPos2( pSrc->vertex[vertID2][0], pSrc->vertex[vertID2][1], pSrc->vertex[vertID2][2] );
					if( vertPos1 == vertPos2 )
					{
						int triID2;
						for( triID2 = 0; triID2 < vertToTriMap[vertID2].Size(); triID2++ )
						{
							sVect += triSVect[vertToTriMap[vertID2][triID2]];
							tVect += triTVect[vertToTriMap[vertID2][triID2]];
						}
					}
				}
			}

			// make an orthonormal system.
			// need to check if we are left or right handed.
			Vector tmpVect;
			CrossProduct( sVect, tVect, tmpVect );
			bool leftHanded = DotProduct( tmpVect, normal ) < 0.0f;
			if( !leftHanded )
			{
				CrossProduct( normal, sVect, tVect );
				CrossProduct( tVect, normal, sVect );
				VectorNormalize( sVect );
				VectorNormalize( tVect );
				finalSVect[0] = sVect[0];
				finalSVect[1] = sVect[1];
				finalSVect[2] = sVect[2];
				finalSVect[3] = 1.0f;
			}
			else
			{
				CrossProduct( sVect, normal, tVect );
				CrossProduct( normal, tVect, sVect );
				VectorNormalize( sVect );
				VectorNormalize( tVect );
				finalSVect[0] = sVect[0];
				finalSVect[1] = sVect[1];
				finalSVect[2] = sVect[2];
				finalSVect[3] = -1.0f;
			}
		}
	}
}

void BuildIndividualMeshes( s_source_t *psource )
{
	int i, j, k;
	
	// sort new vertices by materials, last used
	static int v_listsort[MAXSTUDIOVERTS];	// map desired order to vlist entry
	static int v_ilistsort[MAXSTUDIOVERTS]; // map vlist entry to desired order

	for (i = 0; i < numvlist; i++)
	{
		v_listsort[i] = i;
	}
	qsort( v_listsort, numvlist, sizeof( int ), vlistCompare );
	for (i = 0; i < numvlist; i++)
	{
		v_ilistsort[v_listsort[i]] = i;
	}


	// allocate memory
	psource->numvertices = numvlist;
	psource->localBoneweight = (s_boneweight_t *)kalloc( psource->numvertices, sizeof( s_boneweight_t ) );
	psource->globalBoneweight = NULL;
	psource->vertexInfo = (s_vertexinfo_t *)kalloc( psource->numvertices, sizeof( s_vertexinfo_t ) );
	psource->vertex = new Vector[psource->numvertices];
	psource->normal = new Vector[psource->numvertices];
	psource->tangentS = new Vector4D[psource->numvertices];
	psource->texcoord = (Vector2D *)kalloc( psource->numvertices, sizeof( Vector2D ) );

	// create arrays of unique vertexes, normals, texcoords.
	for (i = 0; i < psource->numvertices; i++)
	{
		j = v_listsort[i];

		VectorCopy( g_vertex[v_listdata[j].v], psource->vertex[i] );
		VectorCopy( g_normal[v_listdata[j].n], psource->normal[i] );		
		Vector2Copy( g_texcoord[v_listdata[j].t], psource->texcoord[i] );

		psource->localBoneweight[i].numbones		= g_bone[v_listdata[j].v].numbones;
		int k;
		for( k = 0; k < MAXSTUDIOBONEWEIGHTS; k++ )
		{
			psource->localBoneweight[i].bone[k]		= g_bone[v_listdata[j].v].bone[k];
			psource->localBoneweight[i].weight[k]	= g_bone[v_listdata[j].v].weight[k];
		}

		// store a bunch of other info
		psource->vertexInfo[i].material		= v_listdata[j].m;

		psource->vertexInfo[i].firstref		= v_listdata[j].firstref;
		psource->vertexInfo[i].lastref		= v_listdata[j].lastref;
		// vprint( 0, "%4d : %2d :  %6.2f %6.2f %6.2f\n", i, psource->boneweight[i].bone[0], psource->vertex[i][0], psource->vertex[i][1], psource->vertex[i][2] );
	}

	// sort faces by materials, last used.
	static int facesort[MAXSTUDIOTRIANGLES];	// map desired order to src_face entry
	static int ifacesort[MAXSTUDIOTRIANGLES];	// map src_face entry to desired order
	
	for (i = 0; i < g_numfaces; i++)
	{
		facesort[i] = i;
	}
	qsort( facesort, g_numfaces, sizeof( int ), faceCompare );
	for (i = 0; i < g_numfaces; i++)
	{
		ifacesort[facesort[i]] = i;
	}

	psource->numfaces = g_numfaces;
	// find first occurance for each material
	for (k = 0; k < MAXSTUDIOSKINS; k++)
	{
		psource->mesh[k].numvertices = 0;
		psource->mesh[k].vertexoffset = psource->numvertices;

		psource->mesh[k].numfaces = 0;
		psource->mesh[k].faceoffset = g_numfaces;
	}

	// find first and count of indices per material
	for (i = 0; i < psource->numvertices; i++)
	{
		k = psource->vertexInfo[i].material;
		psource->mesh[k].numvertices++;
		if (psource->mesh[k].vertexoffset > i)
			psource->mesh[k].vertexoffset = i;
	}

	// find first and count of faces per material
	for (i = 0; i < psource->numfaces; i++)
	{
		k = g_face[facesort[i]].material;

		psource->mesh[k].numfaces++;
		if (psource->mesh[k].faceoffset > i)
			psource->mesh[k].faceoffset = i;
	}

	/*
	for (k = 0; k < MAXSTUDIOSKINS; k++)
	{
		vprint( 0, "%d : %d:%d %d:%d\n", k, psource->mesh[k].numvertices, psource->mesh[k].vertexoffset, psource->mesh[k].numfaces, psource->mesh[k].faceoffset );
	}
	*/

	// create remapped faces
	psource->face = (s_face_t *)kalloc( psource->numfaces, sizeof( s_face_t ));
	for (k = 0; k < MAXSTUDIOSKINS; k++)
	{
		if (psource->mesh[k].numfaces)
		{
			psource->meshindex[psource->nummeshes] = k;

			for (i = psource->mesh[k].faceoffset; i < psource->mesh[k].numfaces + psource->mesh[k].faceoffset; i++)
			{
				j = facesort[i];

				psource->face[i].a = v_ilistsort[g_src_uface[j].a] - psource->mesh[k].vertexoffset;
				psource->face[i].b = v_ilistsort[g_src_uface[j].b] - psource->mesh[k].vertexoffset;
				psource->face[i].c = v_ilistsort[g_src_uface[j].c] - psource->mesh[k].vertexoffset;
				Assert( ((psource->face[i].a & 0xF0000000) == 0) && ((psource->face[i].b & 0xF0000000) == 0) && 
					((psource->face[i].c & 0xF0000000) == 0) );
				// vprint( 0, "%3d : %4d %4d %4d\n", i, psource->face[i].a, psource->face[i].b, psource->face[i].c );
			}

			psource->nummeshes++;
		}
	}

	CalcModelTangentSpaces( psource );
}

void Grab_Triangles( s_source_t *psource )
{
	int		i;
	Vector	vmin, vmax;

	vmin[0] = vmin[1] = vmin[2] = 99999;
	vmax[0] = vmax[1] = vmax[2] = -99999;

	g_numfaces = 0;
	numvlist = 0;
 
	//
	// load the base triangles
	//
	int texture;
	int material;
	char texturename[64];

	while (1) 
	{
		if (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) == NULL) 
			break;

		g_iLinecount++;

		// check for end
		if (IsEnd( g_szLine )) 
			break;

		// Look for extra junk that we may want to avoid...
		int nLineLength = strlen( g_szLine );
		if (nLineLength >= 64)
		{
			MdlWarning("Unexpected data at line %d, (need a texture name) ignoring...\n", g_iLinecount );
			continue;
		}

		// strip off trailing smag
		V_strcpy_safe( texturename, g_szLine );
		for (i = strlen( texturename ) - 1; i >= 0 && ! isgraph( texturename[i] ); i--)
		{
		}
		texturename[i + 1] = '\0';

		// funky texture overrides
		for (i = 0; i < numrep; i++)  
		{
			if (sourcetexture[i][0] == '\0') 
			{
				strcpy( texturename, defaulttexture[i] );
				break;
			}
			if (stricmp( texturename, sourcetexture[i]) == 0) 
			{
				strcpy( texturename, defaulttexture[i] );
				break;
			}
		}

		if (texturename[0] == '\0')
		{
			// weird source problem, skip them
			fgets( g_szLine, sizeof( g_szLine ), g_fpInput );
			fgets( g_szLine, sizeof( g_szLine ), g_fpInput );
			fgets( g_szLine, sizeof( g_szLine ), g_fpInput );
			g_iLinecount += 3;
			continue;
		}

		if (stricmp( texturename, "null.bmp") == 0 || stricmp( texturename, "null.tga") == 0)
		{
			// skip all faces with the null texture on them.
			fgets( g_szLine, sizeof( g_szLine ), g_fpInput );
			fgets( g_szLine, sizeof( g_szLine ), g_fpInput );
			fgets( g_szLine, sizeof( g_szLine ), g_fpInput );
			g_iLinecount += 3;
			continue;
		}

		texture = lookup_texture( texturename, sizeof( texturename ) );
		psource->texmap[texture] = texture;	// hack, make it 1:1
		material = use_texture_as_material( texture );

		s_face_t f;
		ParseFaceData( psource, material, &f );
	
		g_src_uface[g_numfaces] = f;
		g_face[g_numfaces].material = material;
		g_numfaces++;
	}

	BuildIndividualMeshes( psource );
}

//--------------------------------------------------------------------
// Load a SMD file
//--------------------------------------------------------------------  
int Load_SMD ( s_source_t *psource )
{
	char	cmd[1024];
	int		option;

	// Open file
	if (!OpenGlobalFile( psource->filename ))
		return 0;

	// verbose
	if( !g_quiet )
	{
		printf ("SMD MODEL %s\n", psource->filename);
	}

	//March through lines
	g_iLinecount = 0;
	while (fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
	{
		g_iLinecount++;
		int numRead = sscanf( g_szLine, "%s %d", cmd, &option );

		// Blank line
		if ((numRead == EOF) || (numRead == 0))
			continue;

		if (strcmp( cmd, "version" ) == 0) 
		{
			if (option != 1) 
			{
				MdlError("bad version\n");
			}
		}
		// Get hierarchy?
		else if (strcmp( cmd, "nodes" ) == 0) 
		{
			psource->numbones = Grab_Nodes( psource->localBone );
		}
		// Get animation??
		else if (strcmp( cmd, "skeleton" ) == 0) 
		{
			Grab_Animation( psource );
		}
		// Geo?
		else if (strcmp( cmd, "triangles" ) == 0) 
		{
			Grab_Triangles( psource );
		}
		// Geo animation
		else if (strcmp( cmd, "vertexanimation" ) == 0) 
		{
			Grab_Vertexanimation( psource );
		}
		else 
		{
			MdlWarning("unknown studio command\n" );
		}
	}
	fclose( g_fpInput );

	is_v1support = true;

	return 1;
}

//-----------------------------------------------------------------------------
// Checks to see if the model source was already loaded
//-----------------------------------------------------------------------------
static s_source_t *FindCachedSource( char const* name, char const* xext )
{
	int i;

	if( xext[0] )
	{
		// we know what extension is necessary. . look for it.
		sprintf (g_szFilename, "%s%s.%s", cddir[numdirs], name, xext );
		for (i = 0; i < g_numsources; i++)
		{
			if (stricmp( g_szFilename, g_source[i]->filename ) == 0)
				return g_source[i];
		}
	}
	else
	{
		// we don't know what extension to use, so look for all of 'em.
		sprintf (g_szFilename, "%s%s.vrm", cddir[numdirs], name );
		for (i = 0; i < g_numsources; i++)
		{
			if (stricmp( g_szFilename, g_source[i]->filename ) == 0)
				return g_source[i];
		}
		sprintf (g_szFilename, "%s%s.smd", cddir[numdirs], name );
		for (i = 0; i < g_numsources; i++)
		{
			if (stricmp( g_szFilename, g_source[i]->filename ) == 0)
				return g_source[i];
		}
		/*
		sprintf (g_szFilename, "%s%s.vta", cddir[numdirs], name );
		for (i = 0; i < g_numsources; i++)
		{
			if (stricmp( g_szFilename, g_source[i]->filename ) == 0)
				return g_source[i];
		}
		*/
	}

	// Not found
	return 0;
}

static void FlipFacing( s_source_t *pSrc )
{
	unsigned short tmp;

	int i, j;
	for( i = 0; i < pSrc->nummeshes; i++ )
	{
		s_mesh_t *pMesh = &pSrc->mesh[i];
		for( j = 0; j < pMesh->numfaces; j++ )
		{
			s_face_t &f = pSrc->face[pMesh->faceoffset + j];
			tmp = f.b;  f.b  = f.c;  f.c  = tmp;
		}
	}
}

//-----------------------------------------------------------------------------
// Loads an animation source
//-----------------------------------------------------------------------------

s_source_t *Load_Source( char const *name, const char *ext, bool reverse, bool isActiveModel )
{
	// Sanity check number of source files
	if ( g_numsources >= MAXSTUDIOSEQUENCES )
		MdlError( "Load_Source( %s ) - overflowed g_numsources.", name );

	// Sanity check file and init
	Assert(name);
	int namelen = strlen(name) + 1;
	char* pTempName = (char*)_alloca( namelen );
	char xext[32];
	int result = false;

	// Local copy of filename
	strcpy( pTempName, name );
	
	// Sanity check file extension?
	Q_ExtractFileExtension( pTempName, xext, sizeof( xext ) );
	if (xext[0] == '\0')
	{
		V_strcpy_safe( xext, ext );
	}
	else
	{
		Q_StripExtension( pTempName, pTempName, namelen );
	}

	// Cached source, ie: already loaded model, legacy
	// s_source_t* pSource = FindCachedSource( pTempName, xext );
	// if (pSource)
	// {
		// if (isActiveModel)
			// pSource->isActiveModel = true;
		// return pSource;
	// }

	// allocate space and whatnot
	g_source[g_numsources] = (s_source_t *)kalloc( 1, sizeof( s_source_t ) );
	V_strcpy_safe( g_source[g_numsources]->filename, g_szFilename );

	// legacy stuff
	if (isActiveModel)
	{
		g_source[g_numsources]->isActiveModel = true;
	}

	// more ext sanity check
	if ( ( !result && xext[0] == '\0' ) || stricmp( xext, "smd" ) == 0)
	{
		Q_snprintf( g_szFilename, sizeof(g_szFilename), "%s%s.smd", cddir[numdirs], pTempName );
		V_strcpy_safe( g_source[g_numsources]->filename, g_szFilename );
 
		// Import part, load smd file
		result = Load_SMD( g_source[g_numsources] );
	}

	/*
	if ( ( !result && xext[0] == '\0' ) || stricmp( xext, "dmx" ) == 0)
	{
		Q_snprintf( g_szFilename, sizeof(g_szFilename), "%s%s.dmx", cddir[numdirs], pTempName );
		V_strcpy_safe( g_source[g_numsources]->filename, g_szFilename );

		// Import part, load smd file
		result = Load_DMX( g_source[g_numsources] );
	}
	*/

	// Oops
	if ( !result)
	{
		MdlError( "could not load file '%s'\n", g_source[g_numsources]->filename );
	}

	// bump up number of sources
	g_numsources++;
	if( reverse )
	{
		FlipFacing( g_source[g_numsources-1] );
	}
	return g_source[g_numsources-1];
}

void SaveNodes( s_source_t *source, CUtlBuffer& buf )
{
	if ( source->numbones <= 0 )
		return;

	buf.Printf( "nodes\n" );

	for ( int i = 0; i < source->numbones; ++i )
	{
		s_node_t *bone = &source->localBone[ i ];

		buf.Printf( "%d \"%s\" %d\n", i, bone->name, bone->parent );
	}

	buf.Printf( "end\n" );
}

// FIXME:  since we don't us a .qc, we could have problems with scaling, etc.???
void descale_vertex( Vector &org )
{
	float invscale = 1.0f / g_currentscale;

	org[0] = org[0] * invscale;
	org[1] = org[1] * invscale;
	org[2] = org[2] * invscale;
}

void SaveAnimation( s_source_t *source, CUtlBuffer& buf )
{
	if ( source->numbones <= 0 )
		return;

	buf.Printf( "skeleton\n" );

	for ( int frame = 0; frame < source->numframes; ++frame )
	{
		buf.Printf( "time %i\n", frame + source->startframe );

		for ( int i = 0; i < source->numbones; ++i )
		{
			s_bone_t *prev = NULL;
			if ( frame > 0 )
			{
				if ( source->rawanim[ frame - 1 ] )
				{
					prev = &source->rawanim[ frame - 1 ][ i ];
				}
			}

			Vector pos = source->rawanim[ frame ][ i ].pos;
			descale_vertex( pos );
			RadianEuler rot = source->rawanim[ frame ][ i ].rot;

// If this is enabled, then we delta this pos vs the prev frame and don't write out a sample if it's the same value...
#if 0
			if ( prev )
			{
				Vector ppos = source->rawanim[ frame -1 ][ i ].pos; 
				descale_vertex( pos );
				RadianEuler prot = source->rawanim[ frame -1 ][ i ].rot;

				// Only output it if there's a delta
				if ( ( ppos != pos ) || 
					Q_memcmp( &prot, &rot, sizeof( prot ) ) )
				{
					buf.Printf
						( "%d %f %f %f %f %f %f\n", 
						i,				// bone index
						pos[ 0 ],
						pos[ 1 ],
						pos[ 2 ],
						rot[ 0 ],
						rot[ 1 ],
						rot[ 2 ]
						);
				}
			}
			else
#endif
			{
				buf.Printf
					( "%d %f %f %f %f %f %f\n", 
					i,				// bone index
					pos[ 0 ],
					pos[ 1 ],
					pos[ 2 ],
					rot[ 0 ],
					rot[ 1 ],
					rot[ 2 ]
					);
			}
		}
	}

	buf.Printf( "end\n" );
}

void Save_SMD( char const *filename, s_source_t *source )
{
	// Text buffer
	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );

	buf.Printf( "version 1\n" );

	SaveNodes( source, buf );
	SaveAnimation( source, buf );

	FileHandle_t fh = g_pFileSystem->Open( filename, "wb" );
	if ( FILESYSTEM_INVALID_HANDLE != fh )
	{
		g_pFileSystem->Write( buf.Base(), buf.TellPut(), fh );
		g_pFileSystem->Close( fh );
	}
}

//--------------------------------------------------------------------
// mikes right handed row based linear algebra
//--------------------------------------------------------------------
struct M_matrix4x4_t
{
	M_matrix4x4_t() {
	
		m_flMatVal[0][0] = 1.0;	m_flMatVal[0][1] = 0.0; m_flMatVal[0][2] = 0.0; m_flMatVal[0][3] = 0.0;
		m_flMatVal[1][0] = 0.0;	m_flMatVal[1][1] = 1.0; m_flMatVal[1][2] = 0.0;	m_flMatVal[1][3] = 0.0;
		m_flMatVal[2][0] = 0.0;	m_flMatVal[2][1] = 0.0; m_flMatVal[2][2] = 1.0;	m_flMatVal[2][3] = 0.0;
		m_flMatVal[3][0] = 0.0;	m_flMatVal[3][1] = 0.0; m_flMatVal[3][2] = 0.0;	m_flMatVal[3][3] = 1.0;

	}
	// M_matrix3x4_t( 
		// float m00, float m01, float m02, 
		// float m10, float m11, float m12,
		// float m20, float m21, float m22,
		// float m30, float m31, float m32)
	// {
		// m_flMatVal[0][0] = m00;	m_flMatVal[0][1] = m01; m_flMatVal[0][2] = m02;
		// m_flMatVal[1][0] = m10;	m_flMatVal[1][1] = m11; m_flMatVal[1][2] = m12;
		// m_flMatVal[2][0] = m20;	m_flMatVal[2][1] = m21; m_flMatVal[2][2] = m22;
		// m_flMatVal[3][0] = m30;	m_flMatVal[3][1] = m31; m_flMatVal[3][2] = m32;

	// }

	float *operator[]( int i )				{ Assert(( i >= 0 ) && ( i < 4 )); return m_flMatVal[i]; }
	const float *operator[]( int i ) const	{ Assert(( i >= 0 ) && ( i < 4 )); return m_flMatVal[i]; }
	float *Base()							{ return &m_flMatVal[0][0]; }
	const float *Base() const				{ return &m_flMatVal[0][0]; }

	float m_flMatVal[4][4];
};

void M_MatrixAngles( const M_matrix4x4_t& matrix, RadianEuler &angles, Vector &position)
{
	float cX, sX, cY, sY, cZ, sZ;

	sY = -matrix[0][2];
	cY = sqrtf(1.0-(sY*sY));

	if (cY != 0.0)
	{
		sX = matrix[1][2];
		cX = matrix[2][2];
		sZ = matrix[0][1];
		cZ = matrix[0][0];
	}
	else
	{
		sX = -matrix[2][1];
		cX = matrix[1][1];
		sZ = 0.0;
		cZ = 1.0;
	}

	angles[0] = atan2f( sX, cX );
	angles[2] = atan2f( sZ, cZ );

	sX = sinf(angles[0]);
	cX = cosf(angles[0]);

	if (sX > cX)
		cY = matrix[1][2] / sX;
	else
		cY = matrix[2][2] / cX;

	angles[1] = atan2f( sY, cY );
	

	position.x = matrix[3][0];
	position.y = matrix[3][1];
	position.z = matrix[3][2];

}

// void M_MatrixAngles( const M_matrix4x4_t& matrix, RadianEuler &angles, Vector &position)
// { 

	// float cX, sX, cY, sY, cZ, sZ;

	// sY = matrix[2][0];
	// cY = sqrtf(1.0-(sY*sY));

	// if (cY != 0.0)
	// {
		// sX = -matrix[2][1];
		// cX = matrix[2][2];
		// sZ = -matrix[1][0];
		// cZ = matrix[0][0];
	// }
	// else
	// {
		// sX = matrix[0][1];
		// cX = matrix[1][1];
		// sZ = 0.0;
		// cZ = 1.0;
	// }

	// angles[0] = atan2f( sX, cX );
	// angles[2] = atan2f( sZ, cZ );

	// sX = sinf(angles[0]);
	// cX = cosf(angles[0]);

	// if (sX > cX)
		// cY = -matrix[2][1] / sX;
	// else
		// cY = matrix[2][2] / cX;

	// angles[1] = atan2f( sY, cY );
	
	// angles[0] = angles[0];
	// angles[1] = angles[1];
	// angles[2] = angles[2];

	// position.x = matrix[3][0];
	// position.y = matrix[3][1];
	// position.z = matrix[3][2];
// }

void M_MatrixCopy( const M_matrix4x4_t& in, M_matrix4x4_t& out )
{
	// Assert( s_bMathlibInitialized );
	memcpy( out.Base(), in.Base(), sizeof( float ) * 4 * 4 );
}
void M_RotateZMatrix(float radian, M_matrix4x4_t &resultMatrix)
{

	resultMatrix[0][0] = cosf(radian);
	resultMatrix[0][1] = sin(radian);
	resultMatrix[0][2] = 0.0;
	resultMatrix[1][0] =-sin(radian);
	resultMatrix[1][1] =  cos(radian);
	resultMatrix[1][2] = 0.0;
	resultMatrix[2][0] = 0.0;
	resultMatrix[2][1] =  0.0;
	resultMatrix[2][2] = 1.0;
}

// !!! THIS DOESN'T WORK!! WHY? HAS IT EVER?
void M_AngleAboutAxis(Vector &axis, float radianAngle, M_matrix4x4_t &result)
{
	float c = cosf(radianAngle);
	float s = sinf(radianAngle);
	float t = 1.0 - c;
	// axis.normalize();
	
	result[0][0] = t * axis[0] * axis[0] + c;
	result[0][1] = t * axis[0] * axis[1] - s * axis[2];
	result[0][2] = t * axis[0] * axis[2] + s * axis[1];          
	result[1][0] = t * axis[0] * axis[1] + s * axis[2];
	result[1][1] = t * axis[1] * axis[1] + c;
	result[1][2] = t * axis[1] * axis[2] - s * axis[0];
	result[2][0] = t * axis[1] * axis[2] - s;
	result[2][1] = t * axis[1] * axis[2] + s * axis[1];
	result[2][2] = t * axis[2] * axis[2] + c * axis[0];

}


void M_MatrixInvert( const M_matrix4x4_t& in, M_matrix4x4_t& out )
{
	// Assert( s_bMathlibInitialized );
	if ( &in == &out )
	{
		M_matrix4x4_t in2;
		M_MatrixCopy( in, in2 );
		M_MatrixInvert( in2, out );
		return;
	}
	float tmp[3];

	// I'm guessing this only works on a 3x4 orthonormal matrix
	out[0][0] = in[0][0];
	out[1][0] = in[0][1];
	out[2][0] = in[0][2];

	out[0][1] = in[1][0];
	out[1][1] = in[1][1];
	out[2][1] = in[1][2];

	out[0][2] = in[2][0];
	out[1][2] = in[2][1];
	out[2][2] = in[2][2];

	tmp[0] = in[3][0];
	tmp[1] = in[3][1];
	tmp[2] = in[3][2];

	float v1[3], v2[3], v3[3];
	v1[0] = out[0][0];
	v1[1] = out[1][0];
	v1[2] = out[2][0];
	v2[0] = out[0][1];
	v2[1] = out[1][1];
	v2[2] = out[2][1];
	v3[0] = out[0][2];
	v3[1] = out[1][2];
	v3[2] = out[2][2];

	out[3][0] = -DotProduct( tmp, v1 );
	out[3][1] = -DotProduct( tmp, v2 );
	out[3][2] = -DotProduct( tmp, v3 );

    // Trivial case
    // if (IS_IDENTITY(matrix))
	// return SbMatrix::identity();

    // // Affine case...
    // // SbMatrix affineAnswer;
    // // if (  affine_inverse( SbMatrix(matrix), affineAnswer ) )
	// // return affineAnswer;

    // int         index[4];
    // float       d, invmat[4][4], temp;
    // SbMatrix	inverse = *this;

    // if(inverse.LUDecomposition(index, d)) {

		// invmat[0][0] = 1.0;
		// invmat[0][1] = 0.0;
		// invmat[0][2] = 0.0;
		// invmat[0][3] = 0.0;
		// inverse.LUBackSubstitution(index, invmat[0]);
		// invmat[1][0] = 0.0;
		// invmat[1][1] = 1.0;
		// invmat[1][2] = 0.0;
		// invmat[1][3] = 0.0;
		// inverse.LUBackSubstitution(index, invmat[1]);
		// invmat[2][0] = 0.0;
		// invmat[2][1] = 0.0;
		// invmat[2][2] = 1.0;
		// invmat[2][3] = 0.0;
		// inverse.LUBackSubstitution(index, invmat[2]);
		// invmat[3][0] = 0.0;
		// invmat[3][1] = 0.0;
		// invmat[3][2] = 0.0;
		// invmat[3][3] = 1.0;
		// inverse.LUBackSubstitution(index, invmat[3]);
		
// #define SWAP(i,j)		 \
		// temp = invmat[i][j];	 \
		// invmat[i][j] = invmat[j][i];			\
		// invmat[j][i] = temp;
		
		// SWAP(1,0);
		
		// SWAP(2,0);
		// SWAP(2,1);
		
		// SWAP(3,0);
		// SWAP(3,1);
		// SWAP(3,2);
// #undef SWAP	
	// }
}

/*
================
M_ConcatTransforms
================
*/
void M_ConcatTransforms (const M_matrix4x4_t &in1, const M_matrix4x4_t &in2, M_matrix4x4_t &out)
{
	
	// Assert( s_bMathlibInitialized );
	// if ( &in1 == &out )
	// {
		// matrix3x4_t in1b;
		// MatrixCopy( in1, in1b );
		// ConcatTransforms( in1b, in2, out );
		// return;
	// }
	// if ( &in2 == &out )
	// {
		// matrix3x4_t in2b;
		// MatrixCopy( in2, in2b );
		// ConcatTransforms( in1, in2b, out );
		// return;
	// }

#define MULT(i,j) (in1[i][0]*in2[0][j] + \
			 in1[i][1]*in2[1][j] + \
			 in1[i][2]*in2[2][j] + \
			 in1[i][3]*in2[3][j])

    out[0][0] = MULT(0,0);
    out[0][1] = MULT(0,1);
    out[0][2] = MULT(0,2);
    out[0][3] = MULT(0,3);
    out[1][0] = MULT(1,0);
    out[1][1] = MULT(1,1);
    out[1][2] = MULT(1,2);
    out[1][3] = MULT(1,3);
    out[2][0] = MULT(2,0);
    out[2][1] = MULT(2,1);
    out[2][2] = MULT(2,2);
    out[2][3] = MULT(2,3);
    out[3][0] = MULT(3,0);
    out[3][1] = MULT(3,1);
    out[3][2] = MULT(3,2);
    out[3][3] = MULT(3,3);

#undef MULT

}

void M_AngleMatrix( RadianEuler const &angles, const Vector &position, M_matrix4x4_t& matrix )
{
	// Assert( s_bMathlibInitialized );
	float		sx, sy, sz, cx, cy, cz;
	

	sx = sinf(angles[0]);
	cx = cosf(angles[0]);
	sy = sinf(angles[1]);
	cy = cosf(angles[1]);
	sz = sinf(angles[2]);
	cz = cosf(angles[2]);
	
	// SinCos( angles[0], &sx, &cx ); // 2
	// SinCos( angles[1], &sy, &cy ); // 1
	// SinCos( angles[2], &sz, &cz ); // 0
	
	M_matrix4x4_t mx, my, mz, temp1;
	
	// rotation about x
	mx[1][1] = cx;
	mx[1][2] = sx;
	mx[2][1] = -sx;
	mx[2][2] = cx;
 
	// rotation about y
	my[0][0] = cy;
	my[0][2] = -sy;
	my[2][0] = sy;
	my[2][2] = cy;
	
    // rotation about z
	mz[0][0] = cz;
	mz[0][1] = sz;
	mz[1][0] = -sz;
	mz[1][1] = cz;

	// z * y * x
	M_ConcatTransforms(mx, my, temp1);
	M_ConcatTransforms(temp1, mz, matrix);

	// put position in
	matrix[3][0] = position.x;
	matrix[3][1] = position.y;
	matrix[3][2] = position.z;

}


//-----------------------------------------------------------------------------
// Motion mapper functions
//-----------------------------------------------------------------------------
#define BONEAXIS 0
#define BONEDIR 0
#define BONESIDE  1	
#define BONEUP   2
#define WORLDUP 2	
#define PRINTMAT(m) \
	printf("\n%f %f %f %f\n", m[0][0], m[0][1], m[0][2], m[0][3]);	\
	printf("%f %f %f %f\n",   m[1][0], m[1][1], m[1][2], m[1][3]);	\
	printf("%f %f %f %f\n",   m[2][0], m[2][1], m[2][2], m[2][3]);	\
	printf("%f %f %f %f\n",   m[3][0], m[3][1], m[3][2], m[3][3]);

struct s_planeConstraint_t
{
	char jointNameString[1024];
	float floor;
	int axis;
	
};

struct s_iksolve_t
{
	char jointNameString[1024];
	int reverseSolve;
	float extremityScale;
	Vector limbRootOffsetScale;
	int doRelativeLock;
	char relativeLockNameString[1024];
	float relativeLockScale;
	
};

struct s_jointScale_t
{
	char jointNameString[1024];
	float scale;
};
	
struct s_template_t
{
	char rootScaleJoint[1024];
	float rootScaleAmount;
	int numIKSolves;
	s_iksolve_t *ikSolves[128];
	int numJointScales;
	s_jointScale_t *jointScales[128];
	int numPlaneConstraints;
	s_planeConstraint_t *planeConstraints[128];
	float toeFloorZ;
	int doSkeletonScale;
	float skeletonScale;

};

	
//-----------------------------------------------------------------------------
// Load a template file into structure
//-----------------------------------------------------------------------------
s_template_t *New_Template()
{
	s_template_t *pTemplate = (s_template_t *)kalloc(1, sizeof(s_template_t));
	pTemplate->rootScaleAmount = 1.0;
	pTemplate->numIKSolves = 0;
	pTemplate->numJointScales = 0;
	pTemplate->toeFloorZ = 2.802277;
	pTemplate->numPlaneConstraints = 0;
	pTemplate->doSkeletonScale = 0;
	pTemplate->skeletonScale = 1.0;
	return pTemplate;
}
s_iksolve_t *New_IKSolve()
{	
	s_iksolve_t *pIKSolve = (s_iksolve_t *)kalloc(1, sizeof(s_iksolve_t));
	pIKSolve->reverseSolve = 0;
	pIKSolve->extremityScale = 1.0;
	pIKSolve->limbRootOffsetScale[0] = pIKSolve->limbRootOffsetScale[1] = pIKSolve->limbRootOffsetScale[2] = 0.0;
	pIKSolve->doRelativeLock = 0;
	pIKSolve->relativeLockScale = 1.0;
	return pIKSolve;
}

s_planeConstraint_t *New_planeConstraint(float floor)
{	
	s_planeConstraint_t *pConstraint = (s_planeConstraint_t *)kalloc(1, sizeof(s_planeConstraint_t));
	pConstraint->floor = floor;
	pConstraint->axis = 2;
	
	return pConstraint;
}

void Set_DefaultTemplate(s_template_t *pTemplate)
{
	pTemplate->numJointScales = 0;
	
	strcpy(pTemplate->rootScaleJoint, "ValveBiped.Bip01_L_Foot");
	pTemplate->rootScaleAmount = 1.0;

	pTemplate->numIKSolves = 4;
	pTemplate->ikSolves[0] = New_IKSolve();
	pTemplate->ikSolves[1] = New_IKSolve();
	pTemplate->ikSolves[2] = New_IKSolve();
	pTemplate->ikSolves[3] = New_IKSolve();
	

	pTemplate->numPlaneConstraints = 2;
	pTemplate->planeConstraints[0] = New_planeConstraint(pTemplate->toeFloorZ);
	strcpy(pTemplate->planeConstraints[0]->jointNameString, "ValveBiped.Bip01_L_Toe0");
	pTemplate->planeConstraints[1] = New_planeConstraint(pTemplate->toeFloorZ);
	strcpy(pTemplate->planeConstraints[1]->jointNameString, "ValveBiped.Bip01_R_Toe0");	

	strcpy(pTemplate->ikSolves[0]->jointNameString, "ValveBiped.Bip01_L_Foot");
	pTemplate->ikSolves[0]->reverseSolve = 0;
	pTemplate->ikSolves[0]->extremityScale = 1.0;
	pTemplate->ikSolves[0]->limbRootOffsetScale[0] = 1.0;
	pTemplate->ikSolves[0]->limbRootOffsetScale[1] = 1.0;
	pTemplate->ikSolves[0]->limbRootOffsetScale[2] = 0.0;

	strcpy(pTemplate->ikSolves[1]->jointNameString, "ValveBiped.Bip01_R_Foot");
	pTemplate->ikSolves[1]->reverseSolve = 0;
	pTemplate->ikSolves[1]->extremityScale = 1.0;
	pTemplate->ikSolves[1]->limbRootOffsetScale[0] = 1.0;
	pTemplate->ikSolves[1]->limbRootOffsetScale[1] = 1.0;
	pTemplate->ikSolves[1]->limbRootOffsetScale[2] = 0.0;

	strcpy(pTemplate->ikSolves[2]->jointNameString, "ValveBiped.Bip01_R_Hand");
	pTemplate->ikSolves[2]->reverseSolve = 1;
	pTemplate->ikSolves[2]->extremityScale = 1.0;
	pTemplate->ikSolves[2]->limbRootOffsetScale[0] = 0.0;
	pTemplate->ikSolves[2]->limbRootOffsetScale[1] = 0.0;
	pTemplate->ikSolves[2]->limbRootOffsetScale[2] = 1.0;

	strcpy(pTemplate->ikSolves[3]->jointNameString, "ValveBiped.Bip01_L_Hand");
	pTemplate->ikSolves[3]->reverseSolve = 1;
	pTemplate->ikSolves[3]->extremityScale = 1.0;
	pTemplate->ikSolves[3]->limbRootOffsetScale[0] = 0.0;
	pTemplate->ikSolves[3]->limbRootOffsetScale[1] = 0.0;
	pTemplate->ikSolves[3]->limbRootOffsetScale[2] = 1.0;
	// pTemplate->ikSolves[3]->doRelativeLock = 1;
	// strcpy(pTemplate->ikSolves[3]->relativeLockNameString, "ValveBiped.Bip01_R_Hand");
	// pTemplate->ikSolves[3]->relativeLockScale = 1.0;

}

void split(char *str, char *sep, char **sp)
{
	char *r = strtok(str, sep);
	while(r != NULL)
	{
		*sp = r;
		sp++;
		r = strtok(NULL, sep);
	}
	*sp = NULL;
}

	
int checkCommand(char *str, char *cmd, int numOptions, int numSplit)
{
	if(strcmp(str, cmd) == 0)
	{
		if(numOptions <= numSplit)
			return 1;
		else
		{
			printf("Error: Number or argument mismatch in template file cmd %s, requires %i, found %i\n", cmd, numOptions, numSplit);
			return 0;
		}
	}
	return 0;
}

s_template_t *Load_Template(char *name )
{

	// Sanity check file and init
	Assert(name);

	s_template_t *pTemplate = New_Template();
	

	// Open file
	if (!OpenGlobalFile( name ))
		return 0;


	//March through lines
	g_iLinecount = 0;
	while(fgets( g_szLine, sizeof( g_szLine ), g_fpInput ) != NULL) 
	{
		g_iLinecount++;
		if(g_szLine[0] == '#')
			continue;
		
		char *endP = strrchr(g_szLine, '\n');
		if(endP != NULL)
			*endP = '\0';
		

		char *sp[128];
		char **spp = sp;
		
		char sep[] = " ";
		split(g_szLine, sep, sp);
		int numSplit = 0;
		
		while(*spp != NULL)
		{
			spp++;
			numSplit++;
			
		}
		if(numSplit < 1 ||
			*sp[0] == '\n')
			continue;


		// int numRead = sscanf( g_szLine, "%s %s %s", cmd, &option, &option2 );

		// // Blank line
		// if ((numRead == EOF) || (numRead == 0))
			// continue;

		// commands
		char	*cmd;
		int numOptions = numSplit - 1;
		
		cmd = sp[0];
		if(checkCommand(cmd, "twoJointIKSolve", 1, numOptions))
		{
			printf("\nCreating two joint IK solve %s\n", sp[1]);
			pTemplate->ikSolves[pTemplate->numIKSolves] = New_IKSolve();
			strcpy(pTemplate->ikSolves[pTemplate->numIKSolves]->jointNameString, sp[1]);
			pTemplate->numIKSolves++;
			
		}
		else if(checkCommand(cmd, "oneJointPlaneConstraint", 1, numOptions))
		{
			printf("\nCreating one joint plane constraint %s\n", sp[1]);
			pTemplate->planeConstraints[pTemplate->numPlaneConstraints] = New_planeConstraint(pTemplate->toeFloorZ);
			strcpy(pTemplate->planeConstraints[pTemplate->numPlaneConstraints]->jointNameString, sp[1]);
			pTemplate->numPlaneConstraints++;

		}
		else if(checkCommand(cmd, "reverseSolve", 1, numOptions)) 
		{
			printf("reverseSolve: %s\n", sp[1]);
			pTemplate->ikSolves[pTemplate->numIKSolves - 1]->reverseSolve = atoi(sp[1]);
		}
		else if(checkCommand(cmd, "extremityScale", 1, numOptions)) 
		{
			printf("extremityScale: %s\n", sp[1]);
			pTemplate->ikSolves[pTemplate->numIKSolves - 1]->extremityScale = atof(sp[1]);
		}
		else if(checkCommand(cmd, "limbRootOffsetScale", 3, numOptions)) 
		{
			printf("limbRootOffsetScale: %s %s %s\n", sp[1], sp[2], sp[3]);
			pTemplate->ikSolves[pTemplate->numIKSolves - 1]->limbRootOffsetScale[0] = atof(sp[1]);
			pTemplate->ikSolves[pTemplate->numIKSolves - 1]->limbRootOffsetScale[1] = atof(sp[2]);
			pTemplate->ikSolves[pTemplate->numIKSolves - 1]->limbRootOffsetScale[2] = atof(sp[3]);
		}
		else if(checkCommand(cmd, "toeFloorZ", 1, numOptions))
		{
			printf("toeFloorZ: %s\n", sp[1]);
			pTemplate->toeFloorZ = atof(sp[1]);
		}
		else if(checkCommand(cmd, "relativeLock", 2, numOptions)) 
		{
			printf("relativeLock: %s\n", sp[1]);
			pTemplate->ikSolves[pTemplate->numIKSolves - 1]->doRelativeLock = 1;
			strcpy(pTemplate->ikSolves[pTemplate->numIKSolves - 1]->relativeLockNameString, sp[1]);
			pTemplate->ikSolves[pTemplate->numIKSolves - 1]->relativeLockScale = atof(sp[2]);

		}
		else if(checkCommand(cmd, "rootScaleJoint", 1, numOptions)) 
		{
			printf("\nrootScaleJoint: %s\n", sp[1]);
			strcpy(pTemplate->rootScaleJoint, sp[1]);
		}
		else if(checkCommand(cmd, "rootScaleAmount", 1, numOptions)) 
		{
			printf("rootScaleAmount: %s\n", sp[1]);
			pTemplate->rootScaleAmount = atof(sp[1]);
		}
		else if(checkCommand(cmd, "jointScale", 2, numOptions))
		{
			printf("\nCreating joint scale %s of %s\n", sp[1], sp[2]);
			pTemplate->jointScales[pTemplate->numJointScales] = (s_jointScale_t *)kalloc(1, sizeof(s_jointScale_t));
			strcpy(pTemplate->jointScales[pTemplate->numJointScales]->jointNameString, sp[1]);
			pTemplate->jointScales[pTemplate->numJointScales]->scale = atof(sp[2]);
			pTemplate->numJointScales++;
		}
		else if(checkCommand(cmd, "skeletonScale", 2, numOptions))
		{
			printf("\nCreating skeleton scale of %s\n", sp[1]);
			pTemplate->doSkeletonScale = 1;
			pTemplate->skeletonScale = atof(sp[1]);
		}
		else 
		{
			MdlWarning("unknown studio command\n" );
		}
	}
	fclose( g_fpInput );
	return pTemplate;
}

//-----------------------------------------------------------------------------
// get node index from node string name
//-----------------------------------------------------------------------------
int GetNodeIndex(s_source_t *psource, char *nodeName)
{
	for(int i = 0; i < psource->numbones; i++)
	{
		if(strcmp(nodeName, psource->localBone[i].name) == 0)
		{
			return i;
		}	
	}
	return -1;
}

//-----------------------------------------------------------------------------
// get node index from node string name
//-----------------------------------------------------------------------------
void GetNodePath(s_source_t *psource,  int startIndex, int endIndex, int *path)
{
	*path = endIndex;
	
	s_node_t *nodes;
	nodes = psource->localBone;
	while(*path != startIndex)
	{	
		int parent = nodes[*path].parent;
		path++;
		*path = parent;
	}
	path++;
	*path = -1;
}

void SumBonePathTranslations(int *indexPath, s_bone_t *boneArray, Vector &resultVector, int rootOffset = 0)
{

	// walk the path
	int *pathPtr = indexPath;
	// M_matrix4x4_t matrixCum;

	// find length of path
	int length = 0;
	while(*pathPtr != -1)
	{
		length++;
		pathPtr++;
	}

	int l = length - (1 + rootOffset);

	resultVector[0] = 0.0;
	resultVector[1] = 0.0;
	resultVector[2] = 0.0;
	
	for(int i = l; i > -1; i--)
	{		
		s_bone_t *thisBone = boneArray + indexPath[i];
		resultVector += thisBone->pos;
	}
}

void CatBonePath(int *indexPath, s_bone_t *boneArray, M_matrix4x4_t &resultMatrix, int rootOffset = 0)
{

	// walk the path
	int *pathPtr = indexPath;
	// M_matrix4x4_t matrixCum;

	// find length of path
	int length = 0;
	while(*pathPtr != -1)
	{
		length++;
		pathPtr++;
	}

	int l = length - (1 + rootOffset);

	for(int i = l; i > -1; i--)
	{		
		s_bone_t *thisBone = boneArray + indexPath[i];
		// printf("bone index: %i  %i\n", i, indexPath[i]);
		// printf("pos: %f %f %f, rot: %f %f %f\n", thisBone->pos.x, thisBone->pos.y, thisBone->pos.z, thisBone->rot.x, thisBone->rot.y, thisBone->rot.z);
		M_matrix4x4_t thisMatrix;		
		M_AngleMatrix(thisBone->rot, thisBone->pos, thisMatrix);
		// PRINTMAT(thisMatrix)
		M_matrix4x4_t tempCum;
		M_MatrixCopy(resultMatrix, tempCum);
		M_ConcatTransforms(thisMatrix, tempCum, resultMatrix);
	}
	// PRINTMAT(matrixCum);	
	// M_MatrixAngles(matrixCum, resultBone.rot, resultBone.pos);
	
	// printf("pos: %f %f %f, rot: %f %f %f\n", resultBone.pos.x,resultBone.pos.y, resultBone.pos.z, RAD2DEG(resultBone.rot.x),RAD2DEG(resultBone.rot.y),RAD2DEG(resultBone.rot.z));
	
}
// int ConformSources(s_source_t *pSource, s_source_t *pTarget)
// {
	// if(pSource->numbones != *pTarget->numbones)
	// {
		// printf("ERROR: The number of bones in the target file must match the source file.");
		// return 1;
	// }
	// if(pSource->numframes != pTarget->numframes)
	// {
		// printf("Note: Source and target frame lengths do not match");
		// for(int t = 0; t < pTarget->numframes; t++)
		// {
			// free(pTarget->rawanim[t]);
		// }
		// pTarget->numframes = pSource->numframes;
		// int size = pTarget->numbones * sizeof( s_bone_t ); 
		// for(t = 0; t < pTarget->numframes; t++)
		// {
			// pTarget->rawanim[t] = (s_bone_t *) kalloc(1, size);
			// memcpy((void *) pSource->rawanim[t], (void *) pTarget->rawanim[t], size
		// }	
	// }			
	// pTarget->startframe = pSource->startframe;
	// pTarget->endframe = pSource->endframe;




void ScaleJointsFrame(s_source_t *pSkeleton, s_jointScale_t *jointScale, int t)
{
	int numBones = pSkeleton->numbones;

	for(int i = 0; i < numBones; i++)
	{
		s_node_t pNode = pSkeleton->localBone[i];
		s_bone_t *pSkelBone = &pSkeleton->rawanim[t][i];
		if(strcmp(jointScale->jointNameString, pNode.name) == 0)
		{
			// printf("Scaling joint %s\n", pNode.name);			
			pSkelBone->pos = pSkelBone->pos * jointScale->scale;
		}
		
	}
}
void ScaleJoints(s_source_t *pSkeleton, s_jointScale_t *jointScale)
{
	int numFrames = pSkeleton->numframes;
	for(int t = 0; t < numFrames; t++)
	{
		ScaleJointsFrame(pSkeleton, jointScale, t);
	}
}

void ScaleSkeletonFrame(s_source_t *pSkeleton, float scale, int t)
{
	int numBones = pSkeleton->numbones;

	for(int i = 0; i < numBones; i++)
	{
		s_bone_t *pSkelBone = &pSkeleton->rawanim[t][i];
		pSkelBone->pos = pSkelBone->pos * scale;
		
	}
}
void ScaleSkeleton(s_source_t *pSkeleton, float scale)
{
	int numFrames = pSkeleton->numframes;
	for(int t = 0; t < numFrames; t++)
	{
		ScaleSkeletonFrame(pSkeleton, scale, t);
	}
}

void CombineSkeletonAnimationFrame(s_source_t *pSkeleton, s_source_t *pAnimation, s_bone_t **ppAnim, int t)
{
	int numBones = pAnimation->numbones;
	int size = numBones * sizeof( s_bone_t );
	ppAnim[t] = (s_bone_t *) kalloc(1, size);
	for(int i = 0; i < numBones; i++)
	{
		s_node_t pNode = pAnimation->localBone[i];
		s_bone_t pAnimBone = pAnimation->rawanim[t][i];
		
		if(pNode.parent > -1)
		{
			if ( i < pSkeleton->numbones )
			{
				s_bone_t pSkelBone = pSkeleton->rawanim[0][i];
				ppAnim[t][i].pos = pSkelBone.pos;
			}
			else
			{
				if ( !g_bGaveMissingBoneWarning )
				{
					g_bGaveMissingBoneWarning = true;
					Warning( "Warning: Target skeleton has less bones than source animation. Reverting to source data for extra bones.\n" );
				}
				
				ppAnim[t][i].pos = pAnimBone.pos;
			}
		}
		else
		{
			ppAnim[t][i].pos = pAnimBone.pos;
		}
		
		ppAnim[t][i].rot = pAnimBone.rot;	
	}
}
void CombineSkeletonAnimation(s_source_t *pSkeleton, s_source_t *pAnimation, s_bone_t **ppAnim)
{
	int numFrames = pAnimation->numframes;
	for(int t = 0; t < numFrames; t++)
	{
		CombineSkeletonAnimationFrame(pSkeleton, pAnimation, ppAnim, t);
	}
}


//--------------------------------------------------------------------
// MotionMap
//--------------------------------------------------------------------  
s_source_t *MotionMap( s_source_t *pSource, s_source_t *pTarget, s_template_t *pTemplate )
{

	// scale skeleton
	if(pTemplate->doSkeletonScale)
	{
		ScaleSkeleton(pTarget, pTemplate->skeletonScale);
	}

	// scale joints
	for(int j = 0; j < pTemplate->numJointScales; j++)
	{
		s_jointScale_t *pJointScale = pTemplate->jointScales[j];
		ScaleJoints(pTarget, pJointScale);
	}
	

	// root stuff
	char rootString[128] = "ValveBiped.Bip01";

	// !!! PARAMETER
	int rootIndex  = GetNodeIndex(pSource, rootString);
	int rootScaleIndex = GetNodeIndex(pSource, pTemplate->rootScaleJoint);
	int rootScalePath[512];
	if(rootScaleIndex > -1)
	{
		GetNodePath(pSource, rootIndex, rootScaleIndex, rootScalePath);
	}
	else
	{
		printf("Error: Can't find node\n");
		exit(0);
	}
	float rootScaleLengthSrc = pSource->rawanim[0][rootScaleIndex].pos[BONEDIR];
	float rootScaleParentLengthSrc = pSource->rawanim[0][rootScalePath[1]].pos[BONEDIR];
	float rootScaleSrc = rootScaleLengthSrc + rootScaleParentLengthSrc;
	float rootScaleLengthTgt = pTarget->rawanim[0][rootScaleIndex].pos[BONEDIR];
	float rootScaleParentLengthTgt = pTarget->rawanim[0][rootScalePath[1]].pos[BONEDIR];
	float rootScaleTgt = rootScaleLengthTgt + rootScaleParentLengthTgt;
	float rootScaleFactor = rootScaleTgt / rootScaleSrc;

	if(g_verbose)
		printf("Root Scale Factor: %f\n", rootScaleFactor);
	

	// root scale origin
	float toeFloorZ = pTemplate->toeFloorZ;
	Vector rootScaleOrigin = pSource->rawanim[0][rootIndex].pos;
	rootScaleOrigin[2] = toeFloorZ;


	// setup workspace
	s_bone_t *combinedRefAnimation[MAXSTUDIOANIMFRAMES];
	s_bone_t *combinedAnimation[MAXSTUDIOANIMFRAMES];
	s_bone_t *sourceAnimation[MAXSTUDIOANIMFRAMES];
	CombineSkeletonAnimation(pTarget, pSource, combinedAnimation);
	CombineSkeletonAnimation(pTarget, pSource, combinedRefAnimation);
	

	// do source and target sanity checking
	int sourceNumFrames = pSource->numframes;


	// iterate through limb solves
	for(int t = 0; t < sourceNumFrames; t++)
	{
		// setup pTarget for skeleton comparison
		pTarget->rawanim[t] = combinedRefAnimation[t];

		printf("Note: Processing frame: %i\n", t);
		for(int ii = 0; ii < pTemplate->numIKSolves; ii++)
		{
			s_iksolve_t *thisSolve = pTemplate->ikSolves[ii];
			
			char *thisJointNameString = thisSolve->jointNameString;
			int thisJointIndex = GetNodeIndex(pSource, thisJointNameString);
			
			// init paths to feet
			int thisJointPathInRoot[512];
			
			// get paths to feet
			if(thisJointIndex > -1)
			{
				GetNodePath(pSource, rootIndex, thisJointIndex, thisJointPathInRoot);
			}
			else
			{
				printf("Error: Can't find node: %s\n" , thisJointNameString);
				exit(0);
			}
			
			// leg "root" or thigh pointers
			//int gParentIndex = thisJointPathInRoot[2];
			int *gParentPath = thisJointPathInRoot + 2;
			
			//----------------------------------------------------------------
			// get limb lengths
			//----------------------------------------------------------------  
			float thisJointLengthSrc = pSource->rawanim[0][thisJointIndex].pos[BONEDIR];
			float parentJointLengthSrc = pSource->rawanim[0][thisJointPathInRoot[1]].pos[BONEDIR];
			
			float thisLimbLengthSrc = thisJointLengthSrc + parentJointLengthSrc;
			
			float thisJointLengthTgt = pTarget->rawanim[0][thisJointIndex].pos[BONEDIR];
			float parentJointLengthTgt = pTarget->rawanim[0][thisJointPathInRoot[1]].pos[BONEDIR];
			
			float thisLimbLengthTgt = thisJointLengthTgt + parentJointLengthTgt;
			
			// Factor leg length delta
			float thisLimbLength = thisLimbLengthSrc - thisLimbLengthTgt;
			float thisLimbLengthFactor = thisLimbLengthTgt / thisLimbLengthSrc;
			
			if(g_verbose)
				printf("limb length %s: %i: %f, factor %f\n", thisJointNameString, thisJointIndex, thisLimbLength, thisLimbLengthFactor);
			
			// calculate joint grandparent offset
			// Note: because there's no reference pose this doesn't take rotation into account.
			// This only works because of the assumption that joint translations aren't animated.
			M_matrix4x4_t gParentGlobalMatSrc, gParentGlobalMatTgt;
			Vector gParentGlobalSrc, gParentGlobalTgt;
			
			// SumBonePathTranslations(gParentPath, pSource->rawanim[t], gParentGlobalSrc, 1);			
			// SumBonePathTranslations(gParentPath, pTarget->rawanim[t], gParentGlobalTgt, 1);

			// get root path to source parent
			CatBonePath(gParentPath, pSource->rawanim[t], gParentGlobalMatSrc, 1);
			// check against reference animation
			CatBonePath(gParentPath, pTarget->rawanim[t], gParentGlobalMatTgt, 1);

			gParentGlobalSrc[0] = gParentGlobalMatSrc[3][0];
			gParentGlobalSrc[1] = gParentGlobalMatSrc[3][1];
			gParentGlobalSrc[2] = gParentGlobalMatSrc[3][2];
			
			gParentGlobalTgt[0] = gParentGlobalMatTgt[3][0];
			gParentGlobalTgt[1] = gParentGlobalMatTgt[3][1];
			gParentGlobalTgt[2] = gParentGlobalMatTgt[3][2];
			

			Vector gParentDelta(gParentGlobalTgt - gParentGlobalSrc);
			
			if(g_verbose)
				printf("Grand parent delta: %f %f %f\n", gParentDelta[0], gParentDelta[1], gParentDelta[2]);

			gParentDelta *= thisSolve->limbRootOffsetScale;
			

			//----------------------------------------------------------------
			// time takes effect here
			// above waste is unavoidable?
			//----------------------------------------------------------------
			M_matrix4x4_t rootMat;
			M_AngleMatrix(pSource->rawanim[t][rootIndex].rot, pSource->rawanim[t][rootIndex].pos, rootMat);
			
			
			// OK, time to get it together
			// 1) scale foot by legLengthFactor in the non-translated thigh space
			// 2) translate foot by legRootDelta in the space of the root
			// do we leave everything in the space of the root then? PROBABLY!!
			
			M_matrix4x4_t thisJointMat, parentJointMat, thisJointInGParentMat;
			M_AngleMatrix(pSource->rawanim[t][thisJointPathInRoot[0]].rot, pSource->rawanim[t][thisJointPathInRoot[0]].pos, thisJointMat);
			M_AngleMatrix(pSource->rawanim[t][thisJointPathInRoot[1]].rot, pSource->rawanim[t][thisJointPathInRoot[1]].pos, parentJointMat);
			M_ConcatTransforms(thisJointMat, parentJointMat, thisJointInGParentMat);
				
			if(!thisSolve->doRelativeLock)
			{
				// scale around grand parent
				float effectiveScaleFactor = ((thisLimbLengthFactor - 1.0) * thisSolve->extremityScale ) + 1.0;
				thisJointInGParentMat[3][0] *= effectiveScaleFactor;
				thisJointInGParentMat[3][1] *= effectiveScaleFactor;
				thisJointInGParentMat[3][2] *= effectiveScaleFactor;
			}
			
			// adjust into source root space
			M_matrix4x4_t gParentInRootMat, thisJointInRootMat;
			CatBonePath(gParentPath, pSource->rawanim[t], gParentInRootMat, 1);
			M_ConcatTransforms(thisJointInGParentMat, gParentInRootMat, thisJointInRootMat);			
				
			if(!thisSolve->doRelativeLock)
			{
				// adjust by difference of local root
				thisJointInRootMat[3][0] += gParentDelta[0];
				thisJointInRootMat[3][1] += gParentDelta[1];
				thisJointInRootMat[3][2] += gParentDelta[2];
			}
			else
			{
				char *relativeJointNameString = thisSolve->relativeLockNameString;
				int relativeJointIndex = GetNodeIndex(pSource, relativeJointNameString);
				
				// init paths to feet
				int relativeJointPathInRoot[512];
				
				// get paths to feet
				if(relativeJointIndex > -1)
				{
					GetNodePath(pSource, rootIndex, relativeJointIndex, relativeJointPathInRoot);
				}
				else
				{
					printf("Error: Can't find node: %s\n" , relativeJointNameString);
					exit(0);
				}
				// get the source relative joint
				M_matrix4x4_t relativeJointInRootMatSrc, relativeJointInRootMatSrcInverse, thisJointInRelativeSrcMat;
				CatBonePath(relativeJointPathInRoot, pSource->rawanim[t], relativeJointInRootMatSrc, 1);
				M_MatrixInvert(relativeJointInRootMatSrc, relativeJointInRootMatSrcInverse);
				M_ConcatTransforms(thisJointInRootMat, relativeJointInRootMatSrcInverse, thisJointInRelativeSrcMat);
				if(thisSolve->relativeLockScale != 1.0)
				{
					thisJointInRelativeSrcMat[3][0] *= thisSolve->relativeLockScale;
					thisJointInRelativeSrcMat[3][1] *= thisSolve->relativeLockScale;
					thisJointInRelativeSrcMat[3][2] *= thisSolve->relativeLockScale;
				}
				
				// swap momentarily to get new destination
				// NOTE: the relative lock must have already been solved
				sourceAnimation[t] = pSource->rawanim[t];
				pSource->rawanim[t] = combinedAnimation[t];

				// get new relative location
				M_matrix4x4_t relativeJointInRootMatTgt;
				CatBonePath(relativeJointPathInRoot, pSource->rawanim[t], relativeJointInRootMatTgt, 1);
				M_ConcatTransforms(thisJointInRelativeSrcMat, relativeJointInRootMatTgt, thisJointInRootMat);

				// swap back just for cleanliness
				// a little overkill as it's just swapped
				// just leaving it here for clarity
				combinedAnimation[t] = pSource->rawanim[t];
				pSource->rawanim[t] = sourceAnimation[t];

			}
			
			//----------------------------------------------------------------
			// swap animation
			//----------------------------------------------------------------
			sourceAnimation[t] = pSource->rawanim[t];
			pSource->rawanim[t] = combinedAnimation[t];
			
			//----------------------------------------------------------------
			// make thigh data global based on new skeleton
			//----------------------------------------------------------------  
			// get thigh in global space
			M_matrix4x4_t gParentInTgtRootMat, ggParentInTgtRootMat;
			// int *gParentPath = thisJointPathInRoot + 2;
			CatBonePath(gParentPath, pSource->rawanim[t], gParentInTgtRootMat, 1);
			CatBonePath(gParentPath+1, pSource->rawanim[t], ggParentInTgtRootMat, 1);
			
			
			//----------------------------------------------------------------
			// Calculate IK for legs
			//----------------------------------------------------------------
			float parentJointLength = pSource->rawanim[t][*(thisJointPathInRoot + 1)].pos[BONEDIR];
			float thisJointLength = pSource->rawanim[t][thisJointIndex].pos[BONEDIR];
			
			Vector thisLimbHypot;
			thisLimbHypot[0] = thisJointInRootMat[3][0] - gParentInTgtRootMat[3][0];
			thisLimbHypot[1] = thisJointInRootMat[3][1] - gParentInTgtRootMat[3][1];
			thisLimbHypot[2] = thisJointInRootMat[3][2] - gParentInTgtRootMat[3][2];
			
			float thisLimbHypotLength = thisLimbHypot.Length();
			
			// law of cosines!
			float gParentCos = (thisLimbHypotLength*thisLimbHypotLength + parentJointLength*parentJointLength - thisJointLength*thisJointLength) / (2*parentJointLength*thisLimbHypotLength);
			float parentCos = (parentJointLength*parentJointLength + thisJointLength*thisJointLength - thisLimbHypotLength*thisLimbHypotLength) / (2*parentJointLength*thisJointLength);
			
			VectorNormalize(thisLimbHypot);
			
			Vector thisLimbHypotUnit = thisLimbHypot;
			
			M_matrix4x4_t gParentJointIKMat;
			Vector gParentJointIKRot, gParentJointIKOrth;
			
			gParentJointIKRot[0] = gParentInTgtRootMat[BONEUP][0];
			gParentJointIKRot[1] = gParentInTgtRootMat[BONEUP][1];
			gParentJointIKRot[2] = gParentInTgtRootMat[BONEUP][2];
			
			VectorNormalize(gParentJointIKRot);
			gParentJointIKOrth = gParentJointIKRot.Cross(thisLimbHypotUnit);
			VectorNormalize(gParentJointIKOrth);
			gParentJointIKRot = thisLimbHypotUnit.Cross(gParentJointIKOrth);
			VectorNormalize(gParentJointIKRot);
			
			M_MatrixCopy(gParentInTgtRootMat, gParentJointIKMat);
			
			gParentJointIKMat[0][0] = thisLimbHypotUnit[0];
			gParentJointIKMat[0][1] = thisLimbHypotUnit[1];
			gParentJointIKMat[0][2] = thisLimbHypotUnit[2];
			
			gParentJointIKMat[1][0] = gParentJointIKOrth[0];
			gParentJointIKMat[1][1] = gParentJointIKOrth[1];
			gParentJointIKMat[1][2] = gParentJointIKOrth[2];
			
			gParentJointIKMat[2][0] = gParentJointIKRot[0];
			gParentJointIKMat[2][1] = gParentJointIKRot[1];
			gParentJointIKMat[2][2] = gParentJointIKRot[2];
			
			
			M_matrix4x4_t gParentJointIKRotMat, gParentJointResultMat;
			float gParentDeg;
			if(thisSolve->reverseSolve)
			{
				gParentDeg = acos(gParentCos);
			}
			else
			{
				gParentDeg = -acos(gParentCos);
			}

			// sanity check limb length
			if(thisLimbHypotLength < thisLimbLengthTgt)
			{	
				M_RotateZMatrix(gParentDeg, gParentJointIKRotMat);
			}
			
			M_ConcatTransforms(gParentJointIKRotMat, gParentJointIKMat, gParentJointResultMat);
			
			M_matrix4x4_t parentJointIKRotMat;
			//!!! shouldn't need the 180 degree  addition, something in the law of cosines!!!
			float parentDeg;
			if(thisSolve->reverseSolve)
			{
				parentDeg = acos(parentCos)+M_PI;
			}
			else
			{
				parentDeg = -acos(parentCos)+M_PI;
			}
			
			// sanity check limb length
			if(thisLimbHypotLength < thisLimbLengthTgt)
			{	
				M_RotateZMatrix(parentDeg, parentJointIKRotMat);
			}

		
			// Thighs
			M_matrix4x4_t ggParentInTgtRootMatInverse, gParentJointLocalMat;
			M_MatrixInvert(ggParentInTgtRootMat, ggParentInTgtRootMatInverse);
			M_ConcatTransforms(gParentJointResultMat, ggParentInTgtRootMatInverse, gParentJointLocalMat);
			
			s_bone_t resultBone;
			
			// temp test stuff
			// M_MatrixAngles(thisJointInRootMat, resultBone.rot, resultBone.pos);
			// pSource->rawanim[t][thisJointIndex].rot = resultBone.rot;
			// pSource->rawanim[t][thisJointIndex].pos = resultBone.pos;
			
			// M_MatrixAngles(gParentInTgtRootMat, resultBone.rot, resultBone.pos);
			// pSource->rawanim[t][gParentIndex].rot = resultBone.rot;
			// pSource->rawanim[t][gParentIndex].pos = resultBone.pos;
			
			
			M_MatrixAngles(gParentJointLocalMat, resultBone.rot, resultBone.pos);
			pSource->rawanim[t][*gParentPath].pos = resultBone.pos;
			pSource->rawanim[t][*gParentPath].rot = resultBone.rot;
			
			M_MatrixAngles(parentJointIKRotMat, resultBone.rot, resultBone.pos);
			pSource->rawanim[t][*(thisJointPathInRoot+1)].rot = resultBone.rot;
			
			M_matrix4x4_t parentJointGlobalMat, parentJointGlobalMatInverse, thisJointLocalMat;
			CatBonePath(thisJointPathInRoot+1, pSource->rawanim[t], parentJointGlobalMat, 1);
			
			
			M_MatrixInvert(parentJointGlobalMat, parentJointGlobalMatInverse);
			M_ConcatTransforms(thisJointInRootMat, parentJointGlobalMatInverse, thisJointLocalMat);
			
			M_MatrixAngles(thisJointLocalMat, resultBone.rot, resultBone.pos);
			pSource->rawanim[t][thisJointIndex].rot = resultBone.rot;


			// swap animation back for next solve
			combinedAnimation[t] = pSource->rawanim[t];
			pSource->rawanim[t] = sourceAnimation[t];

		}
		// swap animation
		sourceAnimation[t] = pSource->rawanim[t];
		pSource->rawanim[t] = combinedAnimation[t];

		//----------------------------------------------------------------
		// adjust root
		//----------------------------------------------------------------  
		Vector originBonePos = pSource->rawanim[t][rootIndex].pos;
		Vector rootInScaleOrigin = originBonePos - rootScaleOrigin;
		float effectiveRootScale = ((rootScaleFactor - 1.0) * pTemplate->rootScaleAmount) + 1.0;
		Vector scaledRoot = rootInScaleOrigin * effectiveRootScale;
		pSource->rawanim[t][rootIndex].pos = rootScaleOrigin + scaledRoot;

		//------------------------------------------------------------
        // plane constraints
        //------------------------------------------------------------
		for(int ii = 0; ii < pTemplate->numPlaneConstraints; ii++)
		{
			s_planeConstraint_t *thisSolve = pTemplate->planeConstraints[ii];
			
			char *thisJointNameString = thisSolve->jointNameString;
			if(g_verbose)
				printf("Executing plane constraint: %s\n", thisJointNameString);
			
			int thisJointIndex = GetNodeIndex(pSource, thisJointNameString);
			
			// init paths to feet
			int thisJointPath[512];
			
			// get paths to feet
			if(thisJointIndex > -1)
			{
				GetNodePath(pSource, -1, thisJointIndex, thisJointPath);
			}
			else
			{
				printf("Error: Can't find node: %s\n" , thisJointNameString);
				exit(0);
			}
			int parentIndex = thisJointPath[1];
			int *parentPath = thisJointPath + 1;
			
			M_matrix4x4_t thisJointGlobalMat, parentJointGlobalMat, gParentJointGlobalMat, gParentJointGlobalMatInverse;
			CatBonePath(thisJointPath, pSource->rawanim[t], thisJointGlobalMat, 0);
			CatBonePath(parentPath, pSource->rawanim[t], parentJointGlobalMat, 0);
			CatBonePath(parentPath+1, pSource->rawanim[t], gParentJointGlobalMat, 0);
			M_MatrixInvert(gParentJointGlobalMat, gParentJointGlobalMatInverse);

			if(thisJointGlobalMat[3][thisSolve->axis] < thisSolve->floor)
			{
				// printf("-- broken plane: %f\n", thisJointGlobalMat[3][thisSolve->axis]);
				if(parentJointGlobalMat[3][thisSolve->axis] < thisSolve->floor)
				{
					printf("Error: Constraint parent has broken the plane, this frame's plane constraint unsolvable!\n");
				}
				else
				{
					Vector parentJointAtPlane(parentJointGlobalMat[3][0], parentJointGlobalMat[3][1], parentJointGlobalMat[3][2]);
					Vector parentPos(parentJointGlobalMat[3][0], parentJointGlobalMat[3][1], parentJointGlobalMat[3][2]);
					Vector thisJointAtPlane(thisJointGlobalMat[3][0], thisJointGlobalMat[3][1], thisJointGlobalMat[3][2]);
					Vector thisJointPos(thisJointGlobalMat[3][0], thisJointGlobalMat[3][1], thisJointGlobalMat[3][2]);

					thisJointAtPlane[thisSolve->axis] = thisSolve->floor;
					parentJointAtPlane[thisSolve->axis] = thisSolve->floor;

					float thisJointLength = pSource->rawanim[t][thisJointIndex].pos[BONEAXIS];
					float parentLengthToPlane = parentPos[thisSolve->axis] - thisSolve->floor;
					float adjacent = sqrtf((thisJointLength * thisJointLength) - (parentLengthToPlane * parentLengthToPlane));
					Vector parentDirection = thisJointAtPlane - parentJointAtPlane;
					VectorNormalize(parentDirection);
					
					Vector newJointPos = parentJointAtPlane + (parentDirection * adjacent);

					Vector newParentDir = newJointPos - parentPos;
					Vector parentUp(parentJointGlobalMat[BONEUP][0], parentJointGlobalMat[BONEUP][1], parentJointGlobalMat[BONEUP][2]);
					
					VectorNormalize(newParentDir);
					VectorNormalize(parentUp);
					// Vector parentSide = newParentDir.Cross(parentUp);
					Vector parentSide = parentUp.Cross(newParentDir);
					VectorNormalize(parentSide);
					parentUp = newParentDir.Cross(parentSide);
					// parentUp = parentSide.Cross(newParentDir);
					VectorNormalize(parentUp);
					parentJointGlobalMat[BONEDIR][0] = newParentDir[0];
					parentJointGlobalMat[BONEDIR][1] = newParentDir[1];
					parentJointGlobalMat[BONEDIR][2] = newParentDir[2];
					parentJointGlobalMat[BONEUP][0] = parentUp[0];
					parentJointGlobalMat[BONEUP][1] = parentUp[1];
					parentJointGlobalMat[BONEUP][2] = parentUp[2];
					parentJointGlobalMat[BONESIDE][0] = parentSide[0];
					parentJointGlobalMat[BONESIDE][1] = parentSide[1];
					parentJointGlobalMat[BONESIDE][2] = parentSide[2];
					
					
					M_matrix4x4_t newParentJointMat;
					
					M_ConcatTransforms(parentJointGlobalMat, gParentJointGlobalMatInverse, newParentJointMat);
					
					s_bone_t resultBone;
					M_MatrixAngles(newParentJointMat, resultBone.rot, resultBone.pos);
					pSource->rawanim[t][parentIndex].rot = resultBone.rot;
				}
			}
		}

		// swap animation back for next solve
		combinedAnimation[t] = pSource->rawanim[t];
		pSource->rawanim[t] = sourceAnimation[t];
	}
	for(int t = 0; t < sourceNumFrames; t++)
	{
		pTarget->rawanim[t] = combinedAnimation[t];
	}
	pTarget->numframes = sourceNumFrames;
	


	

#if 0
	// Process motion mapping into out and return that
	s_source_t *out = new s_source_t;

	return out;
#else
	// Just returns the start animation, to test the Save_SMD API.
	return pTarget;
#endif
}

char templates[] = 
"\n\
#\n\
# default template file is analogus to not specifying a template file at all\n\
#\n\
\n\
rootScaleJoint ValveBiped.Bip01_L_Foot\n\
rootScaleAmount 1.0\n\
toeFloorZ 2.7777\n\
\n\
twoJointIKSolve ValveBiped.Bip01_L_Foot\n\
reverseSolve 0\n\
extremityScale 1.0\n\
limbRootOffsetScale 1.0 1.0 0.0\n\
\n\
twoJointIKSolve ValveBiped.Bip01_R_Foot\n\
reverseSolve 0\n\
extremityScale 1.0\n\
limbRootOffsetScale 1.0 1.0 0.0\n\
\n\
oneJointPlaneConstraint ValveBiped.Bip01_L_Toe0\n\
\n\
oneJointPlaneConstraint ValveBiped.Bip01_R_Toe0\n\
\n\
twoJointIKSolve ValveBiped.Bip01_R_Hand\n\
reverseSolve 1\n\
extremityScale 1.0\n\
limbRootOffsetScale 0.0 0.0 1.0\n\
\n\
twoJointIKSolve ValveBiped.Bip01_L_Hand\n\
reverseSolve 1\n\
extremityScale 1.0\n\
limbRootOffsetScale 0.0 0.0 1.0\n\
\n\
";


void UsageAndExit()
{
	MdlError( "usage: motionmapper [-quiet] [-verbose] [-templateFile filename] [-printTemplates] sourceanim.smd targetskeleton.smd output.smd\n\
\tsourceanim:  should contain ref pose and animation data\n\
\ttargetsekeleton:  should contain new ref pose, animation data ignored/can be absent\n\
\toutput:  animation from source mapped onto target skeleton (contains new ref pose)\n\
\t-templateFile filename : specifies a template file for guiding the mapping of motion\n\
\t-printTemplate: Causes motionmapper to output the contents of an example template file, which can be used in conjunction with the -templateFile argument to create various motion effects.\n\
\n");
}

void PrintHeader()
{
	vprint( 0, "Valve Software - motionmapper.exe ((c) Valve Coroporation %s)\n", __DATE__ );
	vprint( 0, "--- Maps motion from one animation/skeleton onto another skeleton ---\n" );
}



/*
==============
main
==============
*/
int main (int argc, char **argv)
{
	int		i;
	
	int useTemplate = 0;
	char templateFileName[1024];
	
	// Header
	PrintHeader();

	// Init command line stuff
	CommandLine()->CreateCmdLine( argc, argv );
	InstallSpewFunction();

	// init math stuff
	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f, false, false, false, false );
	g_currentscale = g_defaultscale = 1.0;
	g_defaultrotation = RadianEuler( 0, 0, M_PI / 2 );

	// No args?
	if (argc == 1)
	{
		UsageAndExit();
	}
	
	// Init variable
	g_quiet = false;	
	
	// list template hooey
	CUtlVector< CUtlSymbol > filenames;

	// Get args
	for (i = 1; i < argc; i++) 
	{
		// Switches
		if (argv[i][0] == '-') 
		{
			if (!stricmp(argv[i], "-allowdebug"))
			{
				// Ignore, used by interface system to catch debug builds checked into release tree
				continue;
			}

			if (!stricmp(argv[i], "-quiet"))
			{
				g_quiet = true;
				g_verbose = false;
				continue;
			}

			if (!stricmp(argv[i], "-verbose"))
			{
				g_quiet = false;
				g_verbose = true;
				continue;
			}
			if (!stricmp(argv[i], "-printTemplate"))
			{
				printf("%s\n", templates);
				exit(0);
				
			}
			if (!stricmp(argv[i], "-templateFile"))
			{
				if(i + 1 < argc)
				{
					strcpy( templateFileName, argv[i+1]);
					useTemplate = 1;
					printf("Note: %s passed as template file", templateFileName);
				}
				else
				{
					printf("Error: -templateFile requires an argument, none found!");
					UsageAndExit();
					
				}
				i++;
				continue;
			}
		}
		else
		{
			// more template stuff
			CUtlSymbol sym = argv[ i ];
			filenames.AddToTail( sym );
		}
	}	

	// Enough file args?
	if ( filenames.Count() != 3 )
	{
		// misformed arguments
		// otherwise generating unintended results
		printf("Error: 3 file arguments required, %i found!", filenames.Count());
		UsageAndExit();
	}

	// Filename arg indexes
	int sourceanim = 0;
	int targetskel = 1;
	int outputanim = 2;

	// Copy arg string to global variable
	strcpy( g_outfile, filenames[ outputanim ].String() );

	// Init filesystem hooey
	CmdLib_InitFileSystem( g_outfile );
	// ??
	Q_FileBase( g_outfile, g_outfile, sizeof( g_outfile ) );

	// Verbose stuff
	if (!g_quiet)
	{
		vprint( 0, "%s, %s, %s, path %s\n", qdir, gamedir, g_outfile );
	}
	// ??
	Q_DefaultExtension(g_outfile, ".smd", sizeof( g_outfile ) );
	
	// Verbose stuff
	if (!g_quiet)
	{
		vprint( 0, "Source animation:  %s\n", filenames[ sourceanim ].String() );
		vprint( 0, "Target skeleton:  %s\n", filenames[ targetskel ].String() );

		vprint( 0, "Creating on \"%s\"\n", g_outfile);
	}
	// fullpath = EXTERNAL GLOBAL!!!???
	strcpy( fullpath, g_outfile );
	strcpy( fullpath, ExpandPath( fullpath ) );
	strcpy( fullpath, ExpandArg( fullpath ) );
	
	// Load source and target data
	s_source_t *pSource = Load_Source( filenames[sourceanim].String(), "smd", false, false );
	s_source_t *pTarget = Load_Source( filenames[targetskel].String(), "smd", false, false );


	//
	s_template_t *pTemplate = NULL;
	if(useTemplate)
	{
		pTemplate = Load_Template(templateFileName);
	}
	else
	{
		printf("Note: No template file specified, using defaults settings.\n");
		
		pTemplate = New_Template();
		Set_DefaultTemplate(pTemplate);
	}
	

	// Process skeleton
	s_source_t *pMappedAnimation = MotionMap( pSource, pTarget, pTemplate );

	
	// Save output (ref skeleton & animation data);
	Save_SMD( fullpath, pMappedAnimation );

	Q_StripExtension( filenames[outputanim].String(), outname, sizeof( outname ) );

	// Verbose stuff
	if (!g_quiet)
	{
		vprint( 0, "\nCompleted \"%s\"\n", g_outfile);
	}

	return 0;
}

