# ovalToRaster
This implements a rasterizer for a list of ovals.

![A sample of the renderer running at 10x](ovalToRaster-sample.gif)

Given a list of ovals, this will return the positions and values of the pixels.   
This routine takes a list of ovals and generates the corresponding list of pixels runs that would be required to blit the oval into a frame buffer with the dimensions give by a rectangle of coordinates ( 0, 0, width, height ).

Here is the specification for the oval:

```C++
struct ovalRecord
 {
  float centerx;   /// The x-coordinate for the center position
  float centery;   /// The y-coordinate for the center position
  float radiusx;   /// The radius along the x-axis before rotation
  float radiusy;   /// The radius along the y-axis before rotation
  float angle;     /// The counter-clockwise angle of rotation in radians.
 };
```

And here is the specification for the pixel run:

```C++
struct pixelRun
 {
  int lineY;
  int startX;
  int endX;
  float value;
 };
```

Finally, the interface is just one call:

```C++
std::vector< pixelRun > ovalListToRaster( const std::vector< ovalRecord >& ol, int width, int height );
```

The code is extesively commented and includes tests that use the [**doctest**](https://github.com/doctest/doctest/blob/master/README.md) framework.

The code includes a small [Qt](https://www.qt.io/) based application that can be used to drive the oval renderer for testing.
