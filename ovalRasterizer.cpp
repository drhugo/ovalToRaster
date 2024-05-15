/** ---------------------------------------------------------------------------
*
* \file ovalRasterizer.h
* \description This file contains the code that will raterize oval shapes
*   into a frame buffer
---------------------------------------------------------------------------- */

#include "ovalRasterizer.h"
#include <cmath>

#ifdef TESTING
#include <doctest/doctest.h>
#endif

struct floatBounds
  {
    float left;
    float top;
    float right;
    float bottom;

    void add( const floatBounds& other )
    {
      left = std::min( left, other.left );
      top = std::min( top, other.top );
      right = std::max( right, other.right );
      bottom = std::max( bottom, other.bottom );
    }
  };

struct edgeRecord
  {
    int startx;   /// The leftmost position for this edge for the given scanline
    int endx;     /// The rightmost position for the given scanline

    enum { falling, rising } edgeType;    /// Whether it is a rising or falling edge

    const ovalRecord *oval;
  };

/** ---------------------------------------------------------------------------
* \fn computeBounds
* \description This function compute the bounds of a rotated oval.
---------------------------------------------------------------------------- */
static floatBounds computeBounds( const ovalRecord& oval )
{
  float sinT = std::sin( oval.angle );
  float cosT = std::cos( oval.angle );

  float cosrx = cosT * oval.radiusx;
  float sinrx = sinT * oval.radiusx;
  float cosry = cosT * oval.radiusy;
  float sinry = sinT * oval.radiusy;

  float dx = std::hypot( cosrx, sinry );
  float dy = std::hypot( cosry, sinrx );

  return floatBounds{
      oval.centerx - dx,
      oval.centery - dy,
      oval.centerx + dx,
      oval.centery + dy
    };
}
/** ---------------------------------------------------------------------------
* \fn intervals_intersect
* \description Determines whether an interval defined by a1-a2 overlaps with
*     an interval defined by b1-b2.  The two intervals are said to overlap if
*     the a1 is less than a2 and the range a1-a2 covers any value also covered
*     by b1-b2.
---------------------------------------------------------------------------- */
static bool intervals_intersect( float a1, float a2, float b1, float b2 )
{
  return ( a1 < a2 ) and ( not ( a2 < b1 or b2 < a1 ) );
}
/** ---------------------------------------------------------------------------
* \fn compute_oval_roots
* \description Given a value y, this routine will find the places (if any) where
*     the oval intersects.
---------------------------------------------------------------------------- */
int compute_oval_roots( float xx[2], float yy, const ovalRecord& oval )
{
  float sinT = sin( oval.angle );
  float cosT = cos( oval.angle );

  float sin2T = sinT * sinT;
  float cos2T = cosT * cosT;

  float rx2 = oval.radiusx * oval.radiusx;
  float ry2 = oval.radiusy * oval.radiusy;
  float dy = yy - oval.centery;

  float aa = rx2 * sin2T + ry2 * cos2T;
  float bb = 2.f * dy * sinT * cosT * ( ry2 - rx2 );
  float cc = dy * dy * ( rx2 * cos2T + ry2 * sin2T ) - rx2 * ry2;

  float radical = bb * bb - 4.f * aa * cc;

  int num_roots;

  // If the radical is positive, then there are two roots

  if( 0.f < radical )
    {
      num_roots = 2;

      float sr = sqrt( radical );

      float x1 = oval.centerx + ( ( sr - bb ) / ( 2.f * aa ) );
      float x2 = oval.centerx + ( ( 0.f - sr - bb ) / ( 2.f * aa ) );

      if( x1 < x2 )
        {
          xx[ 0 ] = x1;
          xx[ 1 ] = x2;
        }
      else
        {
          xx[ 0 ] = x2;
          xx[ 1 ] = x1;
        }
    }
  else if( 0.f == radical )   // There is only one root
    {
      num_roots = 1;

      xx[ 0 ] = oval.centerx + ( bb / ( -2.f * aa ) );
    }
  else
    {
      num_roots = 0;
    }

  return num_roots;
}
/** ---------------------------------------------------------------------------
* \fn computeEdgeList
---------------------------------------------------------------------------- */
static int computeEdgeList( int scanY,
                            const std::vector<ovalRecord>& ol,
                            const std::vector<floatBounds>& blist,
                            std::vector<edgeRecord> *edgeList )
{
  int nextY = scanY + 1;

  float topY = scanY;
  float bottomY = topY + 1.f;

  for( int ii = 0; ii < blist.size(); ii += 1 )
    {
      if( intervals_intersect( blist[ ii ].top, blist[ ii ].bottom, topY, bottomY ) )
        {
        }
    }

  return nextY;
}
/** ---------------------------------------------------------------------------
* \fn ovalListToRaster
---------------------------------------------------------------------------- */
std::vector<pixelRun> ovalListToRaster( const std::vector<ovalRecord>& ol, int width, int height )
{
  std::vector<pixelRun> rr;

  if( not ol.empty() )
    {
      std::vector< floatBounds > blist;

      floatBounds bounds = computeBounds( ol[ 0 ] );
      blist.push_back( bounds );

      for( int ii = 1; ii < ol.size(); ii += 1 )
        {
          floatBounds one = computeBounds( ol[ ii ] );
          bounds.add( one );
          blist.push_back( std::move( one ) );
        }

      int topY = (int)std::max( 0.f, bounds.top );
      int endY = (int)std::min( (float)height, std::ceil( bounds.bottom ) );

      pixelRun pr;

      int scanY = topY;

      std::vector<edgeRecord> edgeList;

      if( scanY < endY )
        {
          for( ;; )
            {
              // For the given scanline find all the edges that are relevant
              int nextY = computeEdgeList( scanY, ol, blist, &edgeList );

              // generate the runs here

              if( nextY < endY )
                {
                  scanY = nextY;
                  edgeList.resize( 0 );
                }
              else break;
            }
        }
    }

  return rr;
}
/** ---------------------------------------------------------------------------
 * ---------------------------TEST CASES --------------------------------------
---------------------------------------------------------------------------- */
#ifdef TESTING
TEST_CASE( "AddBounds" )
{
  floatBounds r1{ 10.f, 20.f, 30.f, 40.f };
  floatBounds r2{ 0.f, 10.f, 40.f, 50.f };

  r1.add( r2 );

  CHECK( r1.left == 0.f );
  CHECK( r1.top == 10.f );
  CHECK( r1.right == 40.f );
  CHECK( r1.bottom == 50.f );
}
TEST_CASE( "ComputeBounds0" )
{
  floatBounds rr = computeBounds( {
      .centerx = 100.f,
      .centery = 200.f,
      .radiusx = 10.f,
      .radiusy = 30.f,
      .angle = 0.f
    } );

  CHECK( rr.left == doctest::Approx( 100.f - 10.f ) );
  CHECK( rr.right == doctest::Approx( 100.F + 10.f ) );
  CHECK( rr.top == doctest::Approx( 200.f - 30.f ) );
  CHECK( rr.bottom == doctest::Approx( 200.f + 30.f ) );
}
TEST_CASE( "ComputeBounds90" )
{
  floatBounds rr = computeBounds( {
      .centerx = 100.f,
      .centery = 200.f,
      .radiusx = 10.f,
      .radiusy = 30.f,
      .angle = M_PI_2
    } );

  CHECK( rr.left == doctest::Approx( 100.f - 30.f ) );
  CHECK( rr.right == doctest::Approx( 100.F + 30.f ) );
  CHECK( rr.top == doctest::Approx( 200.f - 10.f ) );
  CHECK( rr.bottom == doctest::Approx( 200.f + 10.f ) );
}
TEST_CASE( "Intervals Intersect" )
{
  CHECK( intervals_intersect( 10.f, 20.f, 30.f, 31.f ) == false );   // to the left
  CHECK( intervals_intersect( 40.f, 50.f, 30.f, 31.f ) == false );  // to the right
  CHECK( intervals_intersect( 40.f, 40.f, 30.f, 31.f ) == false );  // null interval
  CHECK( intervals_intersect( 40.f, 39.f, 30.f, 31.f ) == false );  // null interval too

  CHECK( intervals_intersect( 10.f, 30.5f, 30.f, 31.f ) );  // left overlap
  CHECK( intervals_intersect( 30.5f, 40.f, 30.f, 31.f ) );  // right overlap

  CHECK( intervals_intersect( 10.f, 40.f, 30.f, 31.f ) );  // complete overlap
  CHECK( intervals_intersect( 30.1f, 30.9f, 30.f, 31.f ) );  // completely contained
}
TEST_CASE( "Compute Roots" )
{
  ovalRecord oval = {
    .centerx = 10.f,
    .centery = 5.f,
    .radiusx = 3.f,
    .radiusy = 4.f,
    .angle = M_PI_4
  };

  float xx[ 2 ];

  int num_roots = compute_oval_roots( xx, 6.f, oval );    // above the center
  CHECK( num_roots == 2 );
  CHECK( xx[ 0 ] == doctest::Approx( 6.46448f ) );
  CHECK( xx[ 1 ] == doctest::Approx( 12.9755f ) );

  float dx_above = xx[ 1 ] - xx[ 0 ];

  num_roots = compute_oval_roots( xx, 4.f, oval );        // below the center
  CHECK( num_roots == 2 );
  CHECK( xx[ 0 ] < xx[ 1 ] );
  CHECK( xx[ 0 ] == doctest::Approx( 7.02448f ) );
  CHECK( xx[ 1 ] == doctest::Approx( 13.5355f ) );

  float dx_below = xx[ 1 ] - xx[ 0 ];

  // Since we test one line line above and one line below the center of the
  // oval, we expect the distance between the roots to be the same

  CHECK( ( dx_above - dx_below ) == doctest::Approx( 0 ) );

  oval.angle = 0.f;   // rest the angle to check for one root
  num_roots = compute_oval_roots( xx, 1., oval );
  CHECK( num_roots == 1 );
  CHECK( xx[ 0 ] < xx[ 1 ] );
  CHECK( xx[ 0 ] == doctest::Approx( 10.f ) );

  num_roots = compute_oval_roots( xx, 0.f, oval );  // below
  CHECK( num_roots == 0.f );

  num_roots = compute_oval_roots( xx, 10.f, oval );   // above
  CHECK( num_roots == 0.f );

}
#endif

