//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef _3D_UNITVEC_H
#define _3D_UNITVEC_H


#define UNITVEC_DECLARE_STATICS \
   float cUnitVector::mUVAdjustment[0x2000]; \
   Vector cUnitVector::mTmpVec;

// upper 3 bits
#define SIGN_MASK  0xe000
#define XSIGN_MASK 0x8000
#define YSIGN_MASK 0x4000
#define ZSIGN_MASK 0x2000

// middle 6 bits - xbits
#define TOP_MASK  0x1f80

// lower 7 bits - ybits
#define BOTTOM_MASK  0x007f

// unitcomp.cpp : A Unit Vector to 16-bit word conversion
// algorithm based on work of Rafael Baptista (rafael@oroboro.com)
// Accuracy improved by O.D. (punkfloyd@rocketmail.com)
// Used with Permission.

// a compressed unit vector. reasonable fidelty for unit
// vectors in a 16 bit package. Good enough for surface normals
// we hope.
class cUnitVector // : public c3dMathObject
{
public:
   cUnitVector() { mVec = 0; }
   cUnitVector( const Vector& vec )
   {
      packVector( vec );
   }
   cUnitVector( unsigned short val ) { mVec = val; }

   cUnitVector& operator=( const Vector& vec )
   { packVector( vec ); return *this; }

   operator Vector()
   {
      unpackVector( mTmpVec );
      return mTmpVec;
   }

   void packVector( const Vector& vec )
   {
      // convert from Vector to cUnitVector

      Assert( vec.IsValid());
      Vector tmp = vec;

      // input vector does not have to be unit length
      // Assert( tmp.length() <= 1.001f );

      mVec = 0;
      if ( tmp.x < 0 ) { mVec |= XSIGN_MASK; tmp.x = -tmp.x; }
      if ( tmp.y < 0 ) { mVec |= YSIGN_MASK; tmp.y = -tmp.y; }
      if ( tmp.z < 0 ) { mVec |= ZSIGN_MASK; tmp.z = -tmp.z; }

      // project the normal onto the plane that goes through
      // X0=(1,0,0),Y0=(0,1,0),Z0=(0,0,1).
      // on that plane we choose an (projective!) coordinate system
      // such that X0->(0,0), Y0->(126,0), Z0->(0,126),(0,0,0)->Infinity

      // a little slower... old pack was 4 multiplies and 2 adds.
      // This is 2 multiplies, 2 adds, and a divide....
      float w = 126.0f / ( tmp.x + tmp.y + tmp.z );
      long xbits = (long)( tmp.x * w );
      long ybits = (long)( tmp.y * w );

      Assert( xbits <  127 );
      Assert( xbits >= 0   );
      Assert( ybits <  127 );
      Assert( ybits >= 0   );

      // Now we can be sure that 0<=xp<=126, 0<=yp<=126, 0<=xp+yp<=126
      // however for the sampling we want to transform this triangle
      // into a rectangle.
      if ( xbits >= 64 )
      {
         xbits = 127 - xbits;
         ybits = 127 - ybits;
      }

      // now we that have xp in the range (0,127) and yp in
      // the range (0,63), we can pack all the bits together
      mVec |= ( xbits << 7 );
      mVec |= ybits;
   }

   void unpackVector( Vector& vec )
   {
      // if we do a straightforward backward transform
      // we will get points on the plane X0,Y0,Z0
      // however we need points on a sphere that goes through
      // these points. Therefore we need to adjust x,y,z so
      // that x^2+y^2+z^2=1 by normalizing the vector. We have
      // already precalculated the amount by which we need to
      // scale, so all we do is a table lookup and a
      // multiplication

      // get the x and y bits
      long xbits = (( mVec & TOP_MASK ) >> 7 );
      long ybits = ( mVec & BOTTOM_MASK );

      // map the numbers back to the triangle (0,0)-(0,126)-(126,0)
      if (( xbits + ybits ) >= 127 )
      {
         xbits = 127 - xbits;
         ybits = 127 - ybits;
      }

      // do the inverse transform and normalization
      // costs 3 extra multiplies and 2 subtracts. No big deal.
      float uvadj = mUVAdjustment[mVec & ~SIGN_MASK];
      vec.x = uvadj * (float) xbits;
      vec.y = uvadj * (float) ybits;
      vec.z = uvadj * (float)( 126 - xbits - ybits );

      // set all the sign bits
      if ( mVec & XSIGN_MASK ) vec.x = -vec.x;
      if ( mVec & YSIGN_MASK ) vec.y = -vec.y;
      if ( mVec & ZSIGN_MASK ) vec.z = -vec.z;

      Assert( vec.IsValid());
   }

   static void initializeStatics()
   {
      for ( int idx = 0; idx < 0x2000; idx++ )
      {
         long xbits = idx >> 7;
         long ybits = idx & BOTTOM_MASK;

         // map the numbers back to the triangle (0,0)-(0,127)-(127,0)
         if (( xbits + ybits ) >= 127 )
         {
            xbits = 127 - xbits;
            ybits = 127 - ybits;
         }

         // convert to 3D vectors
         float x = (float)xbits;
         float y = (float)ybits;
         float z = (float)( 126 - xbits - ybits );
		
         // calculate the amount of normalization required
         mUVAdjustment[idx] = 1.0f / sqrtf( y*y + z*z + x*x );
         Assert( _finite( mUVAdjustment[idx]));

         //cerr << mUVAdjustment[idx] << "\t";
         //if ( xbits == 0 ) cerr << "\n";
      }
   }

#if 0
   void test()
   {
      #define TEST_RANGE 4
      #define TEST_RANDOM 100
      #define TEST_ANGERROR 1.0

      float maxError = 0;
      float avgError = 0;
      int numVecs = 0;

      {for ( int x = -TEST_RANGE; x < TEST_RANGE; x++ )
      {
         for ( int y = -TEST_RANGE; y < TEST_RANGE; y++ )
         {
            for ( int z = -TEST_RANGE; z < TEST_RANGE; z++ )
            {
               if (( x + y + z ) == 0 ) continue;

               Vector vec( (float)x, (float)y, (float)z );
               Vector vec2;

               vec.normalize();
               packVector( vec );
               unpackVector( vec2 );

               float ang = vec.dot( vec2 );
               ang = (( fabs( ang ) > 0.99999f ) ? 0 : (float)acos(ang));

               if (( ang > TEST_ANGERROR ) | ( !_finite( ang )))
               {
                  cerr << "error: " << ang << endl;
                  cerr << "orig vec:       " << vec.x << ",\t"
                       << vec.y << ",\t" << vec.z << "\tmVec: "
                       << mVec << endl;
                  cerr << "quantized vec2: " << vec2.x
                       << ",\t" << vec2.y << ",\t"
                       << vec2.z << endl << endl;
               }
               avgError += ang;
               numVecs++;
               if ( maxError < ang ) maxError = ang;
            }
         }
      }}

      for ( int w = 0; w < TEST_RANDOM; w++ )
      {
         Vector vec( genRandom(), genRandom(), genRandom());
         Vector vec2;
         vec.normalize();

         packVector( vec );
         unpackVector( vec2 );

         float ang =vec.dot( vec2 );
         ang = (( ang > 0.999f ) ? 0 : (float)acos(ang));

         if (( ang > TEST_ANGERROR ) | ( !_finite( ang )))
         {
            cerr << "error: " << ang << endl;
            cerr << "orig vec:       " << vec.x << ",\t"
                 << vec.y << ",\t" << vec.z << "\tmVec: "
                 << mVec << endl;
            cerr << "quantized vec2: " << vec2.x << ",\t"
                 << vec2.y << ",\t"
                 << vec2.z << endl << endl;
         }
         avgError += ang;
         numVecs++;
         if ( maxError < ang ) maxError = ang;
      }

      { for ( int x = 0; x < 50; x++ )
      {
         Vector vec( (float)x, 25.0f, 0.0f );
         Vector vec2;

         vec.normalize();
         packVector( vec );
         unpackVector( vec2 );

         float ang = vec.dot( vec2 );
         ang = (( fabs( ang ) > 0.999f ) ? 0 : (float)acos(ang));

         if (( ang > TEST_ANGERROR ) | ( !_finite( ang )))
         {
            cerr << "error: " << ang << endl;
            cerr << "orig vec:       " << vec.x << ",\t"
                 << vec.y << ",\t" << vec.z << "\tmVec: "
                 << mVec << endl;
            cerr << "   quantized vec2: " << vec2.x << ",\t"
                 << vec2.y << ",\t" << vec2.z << endl << endl;
         }

         avgError += ang;
         numVecs++;
         if ( maxError < ang ) maxError = ang;
      }}

      cerr << "max angle error: " << maxError
           << ", average error: " << avgError / numVecs
           << ", num tested vecs: " << numVecs << endl;
   }

   friend ostream& operator<< ( ostream& os, const cUnitVector& vec )
   { os << vec.mVec; return os; }
#endif

//protected: // !!!!

   unsigned short mVec;
   static float mUVAdjustment[0x2000];
   static Vector mTmpVec;
};

#endif // _3D_VECTOR_H


