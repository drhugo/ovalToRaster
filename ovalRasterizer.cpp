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
---------------------------------------------------------------------------- */
static bool intervals_intersect( float a1, float a2, float b1, float b2 )
{
  return ( a1 < a2 ) and ( not ( a2 < b1 or b2 < a1 ) );
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
 * TESTS
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
#endif

