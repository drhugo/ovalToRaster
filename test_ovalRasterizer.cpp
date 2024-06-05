/** ---------------------------------------------------------------------------
*
* \file test_ovalRasterizer.h
* \description This file contains the test for the ovalRasterizer
---------------------------------------------------------------------------- */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "ovalRasterizer.h"

/* ----------------------------------------------------------------------------
 *  TEST CASES
 --------------------------------------------------------------------------- */
TEST_SUITE_BEGIN( "OvalRasterizer_IntegrationTests");
TEST_CASE( "Empty oval list" )
{
  std::vector< ovalRecord >  emptyList;

   auto rr = ovalListToRaster( emptyList, 1000, 1000 );

  CHECK( rr.empty() );
}
TEST_CASE( "Single Flat Oval" )
{
  std::vector< ovalRecord > ovalList;

  // This oval has a radius of 0.f in the y direction.  It should not generate any pixels
  ovalList.push_back( ovalRecord{ 10.f, 10.f, 5.f, 0.f, 0.f } );

  auto rr = ovalListToRaster( ovalList, 200, 200 );

  CHECK( rr.empty() );
}
TEST_CASE( "Small Oval" )
{
  std::vector< ovalRecord > ovalList;

  ovalList.push_back( ovalRecord{ 5.5f, 5.5f, 3.f, 3.f, 0.f } );

  auto rr = ovalListToRaster( ovalList, 10, 10 );

  CHECK( not rr.empty() );

  int num_fixed_runs = 0;

  for( const auto& one : rr )
    {
      CHECK( one.value <= 1.f );    // check that all pixel intensities are less than or equal to 1

      if( one.value == 1.f )
        {
          num_fixed_runs += 1;
        }
    }

  CHECK( num_fixed_runs == 5 );
}
TEST_CASE("Two Scanline Oval")
{
  std::vector< ovalRecord > ovalList;

  ovalList.push_back( ovalRecord{ 4.f, 4.f, 2.f, 1.f, 0.f } );

  auto rr = ovalListToRaster( ovalList, 10, 10 );

  REQUIRE( rr.size() == 6 );
}

TEST_CASE("One Tiny Oval")
{
  std::vector< ovalRecord > ovalList;

  // This test case is mean to trigger the case4 in the aa pixel
  ovalList.push_back( ovalRecord{ 3.5f, 3.5f, .5f, .5f, 0.f } );

  auto rr = ovalListToRaster( ovalList, 10, 10 );

  REQUIRE( rr.size() == 1 );
  CHECK( rr[ 0 ].value == doctest::Approx( 0.5f ) );
}
TEST_CASE("Four Tiny Ovals")
{
  std::vector< ovalRecord > ovalList;

  ovalList.push_back( ovalRecord{ 3.f, 3.f, .25f, .25f, 0.f } );
  ovalList.push_back( ovalRecord{ 4.f, 3.f, .25f, .25f, 0.f } );
  ovalList.push_back( ovalRecord{ 3.f, 4.f, .25f, .25f, 0.f } );
  ovalList.push_back( ovalRecord{ 4.f, 4.f, .25f, .25f, 0.f } );

  auto rr = ovalListToRaster( ovalList, 10, 10 );

    REQUIRE( rr.size() == 9 );
  CHECK( rr[ 0 ].lineY == 2 );
  CHECK( rr[ 3 ].lineY == 3 );
  CHECK( rr[ 6 ].lineY == 4 );

  CHECK( rr[ 0 ].value == doctest::Approx( 0.0857864245f ) );
  CHECK( rr[ 1 ].value == doctest::Approx( 0.414213538f ) );
  CHECK( rr[ 2 ].value == doctest::Approx( 0.0857864245f ) );

  CHECK( rr[ 3 ].value == doctest::Approx( 0.414213538f ) );
  CHECK( rr[ 4 ].value == doctest::Approx( 0.656854272f ) );
  CHECK( rr[ 5 ].value == doctest::Approx( 0.414213538f ) );

  CHECK( rr[ 6 ].value == doctest::Approx( 0.0857864245f ) );
  CHECK( rr[ 7 ].value == doctest::Approx( 0.414213538f ) );
  CHECK( rr[ 8 ].value == doctest::Approx( 0.0857864245f ) );
}
TEST_CASE( "Case 3 Coverage" )
{
  std::vector< ovalRecord > ovalList;

  ovalList.push_back( { 2.f, 2.f, 0.35f, 0.35f, 0.f } );
  ovalList.push_back( { 3.f, 3.f, 0.35f, 0.35f, 0.f } );
  ovalList.push_back( { 5.f, 4.f, 0.35f, 0.35f, 0.f } );
  ovalList.push_back( { 4.f, 5.f, 0.35f, 0.35f, 0.f } );

  auto rr = ovalListToRaster( ovalList, 10, 10 );

  REQUIRE( rr.size() == 9 );
  CHECK( rr[ 0 ].lineY == 1 );
  CHECK( rr[ 0 ].startX == 1 );
  CHECK( rr[ 0 ].endX == 3 );
  CHECK( rr[ 0 ].value == doctest::Approx( 0.0857864245f ) );
  CHECK( rr[ 1 ].value == doctest::Approx( 0.0857864245f ) );
  CHECK( rr[ 2 ].value < 1.f ); // this value should be about twice the above
}
TEST_CASE("Clipped Oval")
{
  std::vector< ovalRecord > ovalList;

  // these four ovals are outside the frame buffer

  ovalList.push_back( { -20.f, 5.5f, 3.f,3.f, 0.f } );
  ovalList.push_back( { 20.f, 5.5f, 3.f,3.f, 0.f } );
  ovalList.push_back( { 5.f, -20.f, 3.f,3.f, 0.f } );
  ovalList.push_back( { 5.f, 20.f, 3.f,3.f, 0.f } );

  auto rr = ovalListToRaster( ovalList, 10, 10 );

  CHECK( rr.empty() );    // The circle is completely to the left
}
TEST_CASE("Concentric Ovals")
{
  std::vector< ovalRecord > ovalList;

  ovalList.push_back( { 10.f, 10.f, 8.f,8.f, 0.f } );

  auto r1 = ovalListToRaster( ovalList, 20, 20 );

  ovalList.push_back( { 10.f, 10.f, 3.f,3.f, 0.f } );

  auto r2 = ovalListToRaster( ovalList, 20, 20 );

  REQUIRE( r1.size() == r2.size() );

  for( int ii = 0; ii < r1.size(); ii += 1 )
    {
      CHECK( r1[ ii ].lineY  == r2[ ii ].lineY );
      CHECK( r1[ ii ].startX == r2[ ii ].startX );
      CHECK( r1[ ii ].endX   == r2[ ii ].endX );
      CHECK( r1[ ii ].value  == r2[ ii ].value );
    }
}


TEST_CASE("Deduplicate Zero Output")
{
  std::vector< ovalRecord > ovalList;

  REQUIRE( deduplicateOvalList( ovalList ) == 0 );    // check empty

  ovalList.push_back( { 100.f, 100.f, 25.f, 25.f, 0.f } );

  REQUIRE( deduplicateOvalList( ovalList ) == 0 );    // check with just one

  ovalList.push_back( { 200.f, 100.f, 25.f, 25.f, 0.f } );
  ovalList.push_back( { 300.f, 100.f, 25.f, 25.f, 0.f } );
  ovalList.push_back( { 400.f, 100.f, 25.f, 25.f, 0.f } );
  ovalList.push_back( { 500.f, 100.f, 25.f, 25.f, 0.f } );

  CHECK( deduplicateOvalList( ovalList ) == 0 );    // all non-overlapping

  ovalList.clear();

  ovalList.push_back( { 100.f, 100.f, 10.f, 10.f, 0.f } );
  ovalList.push_back( { 100.f, 100.f, 10.f, 10.f, 0.f } );  // two of the same

  CHECK( deduplicateOvalList( ovalList, 0.f ) == 0 );    // cover_limit == 0.f
}
TEST_CASE("Deduplicate Remove Cases")
{
  std::vector< ovalRecord > ovalList;

  ovalList.push_back( { 200.f, 100.f, 25.f, 25.f, 0.f } );
  ovalList.push_back( { 300.f, 100.f, 25.f, 25.f, 0.f } );
  ovalList.push_back( { 400.f, 100.f, 25.f, 25.f, 0.f } );
  ovalList.push_back( { 500.f, 100.f, 25.f, 25.f, 0.f } );

  // add exact duplicates of the above
  ovalList.push_back( { 200.f, 100.f, 25.f, 25.f, 0.f } );
  ovalList.push_back( { 300.f, 100.f, 25.f, 25.f, 0.f } );
  ovalList.push_back( { 400.f, 100.f, 25.f, 25.f, 0.f } );
  ovalList.push_back( { 500.f, 100.f, 25.f, 25.f, 0.f } );

  CHECK( deduplicateOvalList( ovalList ) == 4 );
}
TEST_CASE("Deduplicate Edge Cases")
{
  std::vector< ovalRecord > ovalList;

  ovalList.push_back( { 100.f, 300.f, 10.f, 10.f, 0.f } );
  ovalList.push_back( { 200.f, 100.f, 20.f, 20.f, 0.f } );
  ovalList.push_back( { 200.f, 100.f, 25.f, 25.f, 0.f } );

  REQUIRE( deduplicateOvalList( ovalList ) == 1 );
  CHECK( ovalList[ 1 ].radiusx == 25.f );
  CHECK( ovalList[ 1 ].radiusy == 25.f );
}
TEST_CASE("Deduplicate Pivot")
{
  std::vector< ovalRecord > ovalList;

  // test the case where the leftmost oval is removed
  ovalList.push_back( { 100.f, 100.f, 20.f, 20.f, 0.f } );
  ovalList.push_back( { 121.f, 101.f, 40.f, 20.f, 0.f } );
  ovalList.push_back( { 200.f, 100.f, 25.f, 25.f, 0.f } );

  REQUIRE( deduplicateOvalList( ovalList ) == 1 );
  CHECK( ovalList[ 0 ].radiusx == 40.f );
  CHECK( ovalList[ 0 ].radiusy == 20.f );
}
TEST_SUITE_END();