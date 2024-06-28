# ovalToRaster
This implements a rasterizer for a list of ovals.

![A sample of the renderer running at 10x](ovalToRaster-sample.gif)

The animation above shows the interactive rendering of two ovals at 10x of their size (to show the anti-aliased edges).  The animation was created using the Qt-based aplication that is included with the code.

The basic function consists of returning the positions and values of the pixels given a list of (potentially) overlapping ovals.  Namely, there is one routine takes a list of ovals and generates the corresponding list of pixels runs that would be required to blit the oval into a frame buffer with the dimensions give by a rectangle of coordinates ( 0, 0, width, height ).

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
std::vector< pixelRun > ovalListToRaster( const std::vector< ovalRecord >& listOfOvals, int buffer_width, int buffer_height );
```
There is also a routine for de-duplicating lists of ovals.

```C++
int deduplicateOvalList( std::vector< ovalRecord >& ol, float cover_limit = .95f );
```

To integrate this into your code, copy the files `ovalRasterizer.h` and `ovalRasterizer.cpp` to your project.  Then add a call to `ovalListToRaster` from your program, and write the code that takes the pixel runs and applies them to the corresponding frame buffer.  If you wish to de-duplicate the list of ovals, then just call `deduplicateOvalList` and specify how much of the ovals need to overlap before they are removed.  The default value of .95 means that those ovals whose area are covered by other ovals by at least 95% will be removed.  A value of 1.0 would mean that only ovals that are completely contained by other ovals would be removed.

The code is extesively commented and includes tests that use the [**doctest**](https://github.com/doctest/doctest/blob/master/README.md) framework.

The code includes a small [Qt](https://www.qt.io/) based application that can be used to drive the oval renderer for testing.  To build the test application you will need to install Qt 5.15 or later.
