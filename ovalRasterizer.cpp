/** ---------------------------------------------------------------------------
*
* \file ovalRasterizer.h
* \description This file contains the code that will raterize oval shapes
*   into a frame buffer
---------------------------------------------------------------------------- */

#include "ovalRasterizer.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iso646.h>
#include <set>

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

    bool operator<( const struct edgeRecord& other ) const
    {
      return this->startx < other.startx;
    }

    void set_span( float x1, float x2 )
    {
      if( x1 < x2 )
        {
          startx = (int) floor( x1 );
          endx = (int) ceil( x2 );
        }
      else
        {
          startx = (int) floor( x2 );
          endx = (int) ceil( x1 );
        }
    }
  };

struct overlapRecord
  {
    int index;
    floatBounds bounds;

    bool operator<( const struct overlapRecord& other ) const
    {
      bool is_less = true;
      if( other.bounds.left <= bounds.left )
        {
          if( other.bounds.left == bounds.left )
            {
              if( other.bounds.top <= bounds.top )
                {
                  is_less = false;
                }
            }
          else is_less = false;
        }
      return is_less;
    }
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
* \fn computeOverlap
* \description Determine whether two bounding boxes overlap, and if so, by
*     how much.
---------------------------------------------------------------------------- */
bool computeOverlap( const floatBounds& one, const floatBounds& two, float *area_one, float *area_two )
{
  bool overlap = false;

  float ww_span = std::min( one.right, two.right ) - std::max( one.left, two.left );
  float hh_span = std::min( one.bottom, two.bottom ) - std::max( one.top, two.top );

  if( 0.f < ww_span and 0.f < hh_span )
    {
      float overlap_area = ww_span * hh_span;

      *area_one = overlap_area / ( ( one.right - one.left ) * ( one.bottom - one.top ) );
      *area_two = overlap_area / ( ( two.right - two.left ) * ( two.bottom - two.top ) );
      overlap = true;
    }

  return overlap;
}
/** ---------------------------------------------------------------------------
* \fn compute_oval_roots
* \description Given a value y, this routine will find the places (if any) where
*     the oval intersects.
---------------------------------------------------------------------------- */
static int compute_oval_roots( float xx[2], float yy, const ovalRecord& oval )
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

      float sr = sqrt( radical );   // This is always positive

      xx[ 0 ] = oval.centerx + ( ( 0.f - sr - bb ) / ( 2.f * aa ) );
      xx[ 1 ] = oval.centerx + ( ( sr - bb ) / ( 2.f * aa ) );
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
* \fn compute_sdf
* \description compute the signed distance to an oval.  The distance is positive
*     if outside, negative if inside
---------------------------------------------------------------------------- */
static float compute_sdf( const ovalRecord *oval, float xx, float yy )
{
  float dx = xx - oval->centerx;
  float dy = yy - oval->centery;

  float rr;

  if( dx != 0.f or dy != 0.f )
    {
      float angle = oval->angle - atan2( dy, dx );

      float sinT = sin( angle );
      float cosT = cos( angle );
      float a2 = oval->radiusx * oval->radiusx;
      float b2 = oval->radiusy * oval->radiusy;

      float r2 = ( a2 * b2 ) / ( a2 * sinT * sinT + b2 * cosT * cosT );

      rr = hypot( dx, dy ) - sqrt( r2 );
    }
  else  // the point is in the center
    {
      rr = -std::min( oval->radiusx, oval->radiusy );
    }

  return rr;
}
/** ---------------------------------------------------------------------------
* \fn aa_case_1
---------------------------------------------------------------------------- */
static float aa_case_1( float p0, float p1, float p2 )
{
  assert( p0 <= 0.f );
  assert( 0.f <= p1 );
  assert( 0.f <= p2 );

  float s1 = p0 / ( p0 - p1 );
  float s2 = p0 / ( p0 - p2 );

  return 0.5f * s1 * s2;
}
/** ---------------------------------------------------------------------------
* \fn aa_case_2
---------------------------------------------------------------------------- */
static float aa_case_2( float p0, float p1, float p2, float p3 )
{
  assert( p0 < 0.f );
  assert( p1 < 0.f );
  assert( 0.f <= p2 );
  assert( 0.f <= p3 );

  float s1 = p0 / ( p0 - p2 );
  float s2 = p1 / ( p1 - p3 );

  return 0.5f * ( s1 + s2 );
}
/** ---------------------------------------------------------------------------
* \fn aa_case_3
---------------------------------------------------------------------------- */
static float aa_case_3( float p0, float p1, float p2, float p3 )
{
  assert( 0.f <= p0 );
  assert( p1 < 0.f );
  assert( p2 < 0.f );
  assert( 0.f <= p3 );

  return aa_case_1( p1, p3, p0 ) + aa_case_1( p2, p0, p3 );
}
/** ---------------------------------------------------------------------------
* \fn aa_case_4
---------------------------------------------------------------------------- */
static float aa_case_4( float p0, float p1, float p2, float p3, float p4 )
{
  float rr;

  assert( 0 <= p0 );
  assert( 0 <= p1 );
  assert( 0 <= p2 );
  assert( 0 <= p3 );

  if( p4 < 0.f )
    {
      float s0 = p4 / ( p4 - p0 );
      float s1 = p4 / ( p4 - p1 );
      float s2 = p4 / ( p4 - p2 );
      float s3 = p4 / ( p4 - p3 );

      float r1 = 0.25f * ( s0 + s1 + s2 + s3 );

      rr = r1 * r1;
    }
  else rr = 0.f;

  return rr;
}
/** ---------------------------------------------------------------------------
* \fn compute_aa_pixel
* \description Given one or more ovals, compute the contribution.  The approach
*   here is to compute the signed distance for each oval at each corner and
*   then handle each case.
---------------------------------------------------------------------------- */
static float compute_aa_pixel( const std::vector< const ovalRecord*>& aalist, float xx, float yy )
{
  float farr = hypot( aalist.back()->radiusx, aalist.back()->radiusy );
  float p0 = farr;
  float p1 = farr;
  float p2 = farr;
  float p3 = farr;

  //    p0 --- p2
  //    |   p4  |
  //    p1 --- p3

  for( const auto& one : aalist )
    {
      p0 = std::min( p0, compute_sdf( one, xx, yy ) );
      p1 = std::min( p1, compute_sdf( one, xx, yy + 1.f ) );
      p2 = std::min( p2, compute_sdf( one, xx + 1.f, yy ) );
      p3 = std::min( p3, compute_sdf( one, xx + 1.f, yy + 1.f ) );
    }

  int which = 0x0;

  if( p0 < 0.f ) which |= 0x01;
  if( p1 < 0.f ) which |= 0x02;
  if( p2 < 0.f ) which |= 0x04;
  if( p3 < 0.f ) which |= 0x08;

  // There are 16 possibilities that map to four distinct cases

  float rr = 0.f;

  if( which == 0x0 or which == 0xF )
    {
      float p4 = farr;    // only if needed

      for( const auto& one : aalist )
        {
          p4 = std::min( p4, compute_sdf( one, xx + 0.5f, yy + 0.5f ) );
        }

      if( which == 0x0 )
        {
          rr = aa_case_4( p0, p1, p2, p3, p4 );
        }
      else
        {
          rr = 1.f - aa_case_4( -p0, -p1, -p2, -p3, -p4 );
        }
    }

  switch( which )
    {
      case 0x1:  rr = aa_case_1( p0, p1, p2 );  break;
      case 0x2:  rr = aa_case_1( p1, p3, p0 );  break;
      case 0x3:  rr = aa_case_2( p0, p1, p2, p3 );  break;
      case 0x4:  rr = aa_case_1( p2, p0, p3 );  break;
      case 0x5:  rr = aa_case_2( p2, p0, p3, p1 );  break;
      case 0x6:  rr = aa_case_3( p0, p1, p2, p3 );  break;
      case 0x7:  rr = 1.f - aa_case_1( -p3, -p2, -p1 ); break;
      case 0x8:  rr = aa_case_1( p3, p2, p1 );  break;
      case 0x9:  rr = aa_case_3( p1, p3, p0, p2 );  break;
      case 0xA:  rr = aa_case_2( p1, p3, p0, p2 );  break;
      case 0xB:  rr = 1.f - aa_case_1( -p2, -p0, -p3 ); break;
      case 0xC:  rr = aa_case_2( p3, p2, p1, p0 );  break;
      case 0xD:  rr = 1.f - aa_case_1( -p1, -p3, -p0 ); break;
      case 0xE:  rr = 1.f - aa_case_1( -p0, -p1, -p2 ); break;
    }

  return rr;
}
/** ---------------------------------------------------------------------------
* \fn computeEdgeList
* \description For a given scan line value (scanY) find all intersecting ovals
*     and create an edgelist that can be scanned.
---------------------------------------------------------------------------- */
static int computeEdgeList( int scanY,
                            const std::vector<ovalRecord>& ol,
                            const std::vector<floatBounds>& blist,
                            const floatBounds& bounds,
                            std::vector<edgeRecord> *edgeList )
{
  float topY = scanY;
  float bottomY = topY + 1.f;

  float nextY = bounds.bottom;

  for( int ii = 0; ii < blist.size(); ii += 1 )
    {
      if( intervals_intersect( blist[ ii ].top, blist[ ii ].bottom, topY, bottomY ) )
        {
          float topx[ 2 ];
          float botx[ 2 ];

          int num_top = compute_oval_roots( topx, topY, ol[ ii ] );
          int num_bot = compute_oval_roots( botx, bottomY, ol[ ii ] );

          if( num_top == 2 and num_bot == 2 )   // the most common case
            {
              edgeRecord er1, er2;

              er1.set_span( topx[ 0 ], botx[ 0 ] );
              er2.set_span( topx[ 1 ], botx[ 1 ] );

              er1.edgeType = edgeRecord::falling;
              er2.edgeType = edgeRecord::rising;
              er1.oval = & ol[ ii ];
              er2.oval = & ol[ ii ];

              edgeList->push_back( er1 );
              edgeList->push_back( er2 );
            }
          else if( num_top == 2 )   // num_bottom is either zero or one
            {
              float lowx;

              if( num_bot == 1 )
                lowx = botx[ 0 ];
              else
                lowx = 0.5f * ( topx[ 0 ] + topx[ 1 ] );

              edgeList->push_back( {
                (int) floor( topx[ 0 ] ),
                (int) ceil( lowx ),
                edgeRecord::falling,
                & ol[ ii ]
              });

              edgeList->push_back( {
                (int) floor( lowx ),
                (int) ceil( topx[ 1 ] ),
                edgeRecord::rising,
                & ol[ ii ]
              });
            }
          else if( num_bot == 2 )   // then num_top is either zero or one
            {
              float hix;

              if( num_top == 1 )
                hix = topx[ 0 ];
              else
                hix = 0.5f * ( botx[ 0 ] + botx[ 1 ] );

              edgeList->push_back( {
                (int) floor( botx[ 0 ] ),
                (int) ceil( hix ),
                edgeRecord::falling,
                & ol[ ii ]
              });

              edgeList->push_back( {
                (int) floor( hix ),
                (int) ceil( botx[ 1 ] ),
                edgeRecord::rising,
                & ol[ ii ]
              });
            }
          else  // The remaining cases are all pathological - we use the bounds
            {
              float midx = 0.5f * ( blist[ ii ].left + blist[ ii ].right );

              edgeList->push_back( {
                (int) floor( blist[ ii ].left ),
                (int) ceil( midx ),
                edgeRecord::falling,
                & ol[ ii ]
              });

              edgeList->push_back( {
                (int) floor( midx ),
                (int) ceil( blist[ ii ].right ),
                edgeRecord::rising,
                & ol[ ii ]
                });
            }
        }
      else
        {
          if( bottomY < blist[ ii ].top and  // the bounds are after this scanline
              blist[ ii ].top < nextY )      // but closer than the previous top candidate
            {
              nextY = blist[ ii ].top;
            }
        }
    }

  int next_scanY;

  if( not edgeList->empty() )
    {
      next_scanY = scanY + 1;
    }
  else  // the edgelist is empty, nothing intersected our span
    {
      next_scanY = std::max( (int) floor( nextY ), scanY + 1 );;
    }

  return next_scanY;
}
/** ---------------------------------------------------------------------------
* \fn extend_or_push
---------------------------------------------------------------------------- */
static void push_or_merge_run( std::vector< pixelRun >& rr, const pixelRun& pr )
{
  if( rr.empty() or
      rr.back().value != pr.value or
      rr.back().lineY != pr.lineY or
      rr.back().endX != pr.startX )
    {
      rr.push_back( pr );
    }
  else
    {
      rr.back().endX = pr.endX;
    }
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
          blist.push_back( one );
        }

      int topY = (int)std::max( 0.f, bounds.top );
      int endY = (int)std::min( (float)height, std::ceil( bounds.bottom ) );
      int right_edge = (int)std::min((float) width, ceil( bounds.right ) );

      pixelRun pr;

      int scanY = topY;

      std::vector<edgeRecord> edgeList;

      if( scanY < endY )
        {
          for( ;; )
            {
              pr.lineY = scanY;

              // For the given scanline find all the edges that are relevant
              int nextY = computeEdgeList( scanY, ol, blist, bounds, &edgeList );

              if( not edgeList.empty() )
                {
                  std::sort( edgeList.begin(), edgeList.end() );

                  std::vector< const ovalRecord *> aalist;    // for anti-aliased pixels

                  pr.startX = std::max( 0, edgeList[ 0 ].startx );

                  if( pr.startX < right_edge )
                    {
                      int last_inside = 0;

                      do
                        {
                          int inside = 0;

                          pr.endX = right_edge;  // assume that we're going to the edge

                          // For each value of x, we need to go through the list of edges
                          // and collect the oval edges that would need to be evaluated there

                          for( const auto& edge : edgeList )
                            {
                              if( edge.startx <= pr.startX )
                                {
                                  if( edge.edgeType == edgeRecord::falling )
                                    {
                                      if( edge.startx <= pr.startX )
                                        {
                                          inside += 1;
                                        }
                                    }
                                  else //  edge.Type == edgeRecord::rising
                                    {
                                      if( edge.endx <= pr.startX )
                                        {
                                          inside -= 1;
                                        }
                                    }

                                  // Check if we are in the portion that needs to be anti-aliased
                                  if( edge.startx <= pr.startX and pr.startX < edge.endx )
                                    {
                                      aalist.push_back( edge.oval );
                                    }
                                }
                              else  // this edge starts is beyond our current scan point
                                {
                                  pr.endX = std::min( edge.startx, right_edge );
                                  break;
                                }
                            }

                          if( aalist.empty() )
                            {
                              if( 0 < inside )
                                {
                                  pr.value = 1.f;   // a solid run
                                  push_or_merge_run( rr, pr );
                                }
                            }
                          else
                            {
                              if( inside == 1 or ( 1 < inside and last_inside == 0 ) )
                                {
                                  pr.endX = pr.startX + 1;
                                  pr.value = compute_aa_pixel( aalist, pr.startX, pr.lineY );

                                  push_or_merge_run( rr, pr );
                                }
                              else if( 1 < inside )
                                {
                                  pr.value = 1.f;
                                  pr.endX = pr.startX + 1;

                                  push_or_merge_run( rr, pr );
                                }

                              aalist.resize( 0 );
                            }

                          pr.startX = pr.endX;
                          last_inside = inside;

                        }  while( pr.startX < right_edge );
                    }
                }

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
* \fn deduplicateOvalList
---------------------------------------------------------------------------- */
int deduplicateOvalList( std::vector< ovalRecord >& ovalList, float cover_limit )
{
  int num_removed = 0;

  if( 1 < ovalList.size() and 0.f < cover_limit )
    {
      std::vector< overlapRecord > xlist;
      xlist.reserve( ovalList.size() );

      int ii = 0;
      for(; ii < ovalList.size(); ii += 1 )
        {
          xlist.push_back( { ii, computeBounds( ovalList[ ii ] ) } );
        }
      std::sort( xlist.begin(), xlist.end() );
      std::set< int > skips;
      for( int jj = 0; jj < xlist.size(); jj += 1 )
        {
          if( skips.count( jj ) == 0 )    // if we haven't deleted this one
            {
              for( ii = jj + 1; ii < xlist.size(); ii += 1 )
                {
                  if( skips.count( ii ) == 0 )    // we haven't deleted this one
                    {
                      if( xlist[ ii ].bounds.left < xlist[ jj ].bounds.right )
                        {
                          float cover_jj, cover_ii;

                          if( computeOverlap( xlist[ jj ].bounds, xlist[ ii ].bounds,
                                & cover_jj, & cover_ii ) )
                            {
                              if( cover_jj <= cover_ii and cover_limit <= cover_ii )
                                {
                                  skips.insert( ii );
                                }
                              else if( cover_ii < cover_jj and cover_limit <= cover_jj )
                                {
                                  skips.insert( jj );
                                  break;    // we removed the pivot, so skip to the next
                                }
                            }
                        }
                      else break;   // the remaining rectangles are to the right
                    }
                }
            }
        }

      if( not skips.empty() )
        {
          std::vector< ovalRecord > updatedList;

          for( ii = 0; ii < ovalList.size(); ii += 1 )
            {
              if( skips.count( ii ) == 0 )    // don't skip this one
                {
                  updatedList.push_back( ovalList[ xlist[ ii ].index ] );
                }
            }

          ovalList.swap( updatedList );
          num_removed = skips.size();
        }
    }

  return num_removed;
}
/** ---------------------------------------------------------------------------
 * ---------------------------TEST CASES --------------------------------------
---------------------------------------------------------------------------- */
#ifdef TESTING
TEST_SUITE_BEGIN( "OvalRasterizer_UnitTests");
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
  CHECK( xx[ 0 ] == doctest::Approx(  6.464482f ) );
  CHECK( xx[ 1 ] == doctest::Approx( 12.975518f ) );

  float dx_above = xx[ 1 ] - xx[ 0 ];

  num_roots = compute_oval_roots( xx, 4.f, oval );        // below the center
  CHECK( num_roots == 2 );
  CHECK( xx[ 0 ] < xx[ 1 ] );
  CHECK( xx[ 0 ] == doctest::Approx(  7.024482f ) );
  CHECK( xx[ 1 ] == doctest::Approx( 13.535518f ) );

  float dx_below = xx[ 1 ] - xx[ 0 ];

  // Since we test one line line above and one line below the center of the
  // oval, we expect the distance between the roots to be the same

  CHECK( ( dx_above - dx_below ) == doctest::Approx( 0.f ) );

  oval.angle = 0.f;   // rest the angle to check for one root
  num_roots = compute_oval_roots( xx, 1., oval );
  CHECK( num_roots == 1 );
  CHECK( xx[ 0 ] < xx[ 1 ] );
  CHECK( xx[ 0 ] == doctest::Approx( 10.f ) );

  // Test above and below the oval to confirm that no roots can be found there

  num_roots = compute_oval_roots( xx, 0.f, oval );  // below
  CHECK( num_roots == 0 );

  num_roots = compute_oval_roots( xx, 10.f, oval );   // above
  CHECK( num_roots == 0 );
}
TEST_CASE("edgeRecord_sort")
{
  std::vector< edgeRecord > edgeList;

  edgeRecord one{ .startx = 10, .endx = 11, .edgeType = edgeRecord::falling, .oval = nullptr };
  edgeRecord two{ .startx = 20, .endx = 21, .edgeType = edgeRecord::falling, .oval = nullptr };
  edgeRecord three{ .startx = 30, .endx = 31, .edgeType = edgeRecord::falling, .oval = nullptr };

  CHECK( one < two );
  CHECK( two < three );
  CHECK( one < three );

  edgeList.push_back( three );
  edgeList.push_back( two );
  edgeList.push_back( one );

  std::sort( edgeList.begin(), edgeList.end() );

  CHECK( edgeList[ 0 ] < edgeList[ 1 ] );
  CHECK( edgeList[ 1 ] < edgeList[ 2 ] );
}
TEST_CASE("edgeRecord_set_span")
{
  edgeRecord er;

  er.set_span( 1.5, 2.5 );

  CHECK( er.startx == 1 );
  CHECK( er.endx == 3 );

  er.set_span( 2.5, 1.5 );

  CHECK( er.startx == 1 );
  CHECK( er.endx == 3 );
}
TEST_CASE("compute_sdf")
{
  ovalRecord oval{ 10.f, 20.f, 3.f, 4.f, M_PI_2 };

  CHECK( compute_sdf( & oval, 10.f, 20.f ) == -3.f );
  CHECK( compute_sdf( & oval, 10.f, 0.f ) == doctest::Approx( 17.f ) );
  CHECK( compute_sdf( & oval, 0.f, 20.f ) == doctest::Approx( 6.f ) );
  CHECK( compute_sdf( & oval, 8.f, 20.f ) == doctest::Approx( -2.f ) );
}
TEST_CASE("AA_Case_Tests")
{
  CHECK( aa_case_1( -1.f, 0.f, 0.f ) == doctest::Approx( .5f ) );
  CHECK( aa_case_1( -1.f, 1.f, 1.f ) == doctest::Approx( 0.125f ) );

  CHECK( aa_case_2( -1.f, -1.f, 0.f, 0.f ) == doctest::Approx( 1.f ) );
  CHECK( aa_case_2( -1.f, -1.f, 1.f, 1.f ) == doctest::Approx( 0.5f ) );

  CHECK( aa_case_3( 0.f, -1.f, -1.f, 0.f ) == doctest::Approx( 1.f ) );
  CHECK( aa_case_3( 1.f, -1.f, -1.f, 1.f ) == doctest::Approx( 0.25f ) );

  CHECK( aa_case_4( 1.f, 1.f, 1.f, 1.f, 0.f ) == doctest::Approx( 0.f ) );
  CHECK( aa_case_4( 1.f, 1.f, 1.f, 1.f, -1.f ) == doctest::Approx( 0.25f ) );
  CHECK( aa_case_4( 0.f, 0.f, 0.f, 0.f, -1.f ) == doctest::Approx( 1.0f ) );
}
TEST_CASE( "Merge and Push Runs")
{
  std::vector<pixelRun> runList;

  push_or_merge_run( runList, {101, 100, 200, 1.f} );
  CHECK( runList.size() == 1 );   // check if empty
  push_or_merge_run( runList, {101, 200, 210, .9f} );
  CHECK( runList.size() == 2 );  // non-match on the value
  push_or_merge_run( runList, {102, 210, 220, .9f} );
  CHECK( runList.size() == 3 );  // non-match on the lineY
  push_or_merge_run( runList, {102, 220, 230, .9f} );
  CHECK( runList.size() == 3 );
  CHECK( runList.back().endX == 230 );
}
TEST_CASE( "CompareOverlapRecords")
{
  overlapRecord one { 1, { 10.f, 20.f, 50.f, 60.f } };
  overlapRecord two { 2, { 15.f, 25.f, 55.f, 65.f } };

  overlapRecord three{ 3, { 10.f, 25.f, 50.f, 60.f } };
  overlapRecord four{ 4, { 10.f, 20.f, 45.f, 65.f } };

  CHECK( one < two );         // trivial case
  CHECK( not ( two < one ) );   // reverse
  CHECK( one < three );       // left edge is aligned
  CHECK( not ( three < one ) ); // reverse

  CHECK( not ( one < four ) );  // left and top match, not strictly less
  CHECK( not ( four < one ) );
}
TEST_CASE( "ComputeOverlap")
{
  floatBounds one{ 10.f, 10.f, 20.f, 20.f };
  floatBounds two{  15.f, 15.f, 25.f, 25.f };

  floatBounds three{ 30.f, 10.f, 40.f, 20.f };    // to the right
  floatBounds four{ 10.f, 30.f, 20.f, 40.f };     // below

  floatBounds five{ 10.f, 15.f, 20.f, 20.f };     // contained

  float area_one, area_two;

  REQUIRE( computeOverlap( one, two, & area_one, & area_two ) );
  CHECK( area_one == doctest::Approx( 0.25f ) );
  CHECK( area_two == doctest::Approx( 0.25f ) );

  CHECK( computeOverlap( one, three, & area_one, & area_two ) == false );
  CHECK( computeOverlap( one, four, & area_one, & area_two ) == false );

  REQUIRE( computeOverlap( one, five, & area_one, & area_two ) );
  CHECK( area_one == doctest::Approx( 0.5f ) );
  CHECK( area_two == doctest::Approx( 1.f ) );
}
TEST_SUITE_END();
#endif

