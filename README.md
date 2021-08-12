![Example image](image.png?raw=true "Raytracer showcase")

**SoftwareRT** is an extremely basic multi-threaded software raytracer written in pure C++. The code is based off of the 'Ray Tracing in One Weekend' book,
so it is a very naive (and slow) implementation. However, the program uses multi-threading in order to scale well with modern processors. 

The result image is saved to a file with the name of the current unix time in the PPM format.

## Single vs multi-core performance
Performance should scale well according to the number of threads your processor has.
Instead of calculating each pixel at a time, if the multicore option is enabled, each
thread works on a row of pixels at once.

## Building
This demo has been ported to CMake. The only dependency used is the standard library.