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

  REQUIRE( rr.size() == 7 );
  CHECK( rr[ 0 ].lineY == 2 );
  CHECK( rr[ 3 ].lineY == 3 );
  CHECK( rr[ 4 ].lineY == 4 );

  CHECK( rr[ 3 ].startX == 2 );
  CHECK( rr[ 3 ].endX == 5 );

  CHECK( rr[ 0 ].value == doctest::Approx( 0.0857864245f ) );
  CHECK( rr[ 1 ].value == doctest::Approx( 1.f ) );
  CHECK( rr[ 2 ].value == doctest::Approx( 0.0857864245f ) );
  CHECK( rr[ 3 ].value == doctest::Approx( 1.f ) );
  CHECK( rr[ 4 ].value == doctest::Approx( 0.0857864245f ) );
  CHECK( rr[ 5 ].value == doctest::Approx( 1.f ) );
  CHECK( rr[ 6 ].value == doctest::Approx( 0.0857864245f ) );

}
TEST_CASE("Clipped Oval")
{
  std::vector< ovalRecord > ovalList;

  // these four ovals are outside the frame buffer

  ovalList.push_back( ovalRecord{ -20.f, 5.5f, 3.f,3.f, 0.f } );
  ovalList.push_back( ovalRecord{ 20.f, 5.5f, 3.f,3.f, 0.f } );
  ovalList.push_back( ovalRecord{ 5.f, -20.f, 3.f,3.f, 0.f } );
  ovalList.push_back( ovalRecord{ 5.f, 20.f, 3.f,3.f, 0.f } );

  auto rr = ovalListToRaster( ovalList, 10, 10 );

  CHECK( rr.empty() );    // The circle is completely to the left
}
TEST_SUITE_END();