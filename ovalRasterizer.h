/** ---------------------------------------------------------------------------
 *
 * \file ovalRasterizer.h
 * \description This file contains the code that will raterize oval shapes
 *   into a frame buffer
---------------------------------------------------------------------------- */

#ifndef OVALRASTERIZER_H
#define OVALRASTERIZER_H

#include <vector>

struct ovalRecord
 {
  float centerx;   /// The x-coordinate for the center position
  float centery;   /// The y-coordinate for the center position
  float radiusx;   /// The radius along the x-axis before rotation
  float radiusy;   /// The radius along the y-axis before rotation
  float angle;     /// The counter-clockwise angle of rotation in radians.
 };

struct pixelRun
 {
  int lineY;
  int startX;
  int endX;
  float value;
 };

/// \fn ovalListToRaster
/// \description This routine takes a list of ovals and generates the corresponding list of
///     pixels runs that would be required to blit the oval into a frame buffer with the
///     dimensions give by a rectangle of coordinates ( 0, 0, width, height ).
/// \returns a list of pixel runs.  The method will throw if there is an error.
std::vector< pixelRun > ovalListToRaster( const std::vector< ovalRecord >& ol, int width, int height );

#endif //OVALRASTERIZER_H
