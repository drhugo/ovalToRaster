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