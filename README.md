**SoftwareRT** is an extremely basic multi-threaded software raytracer written in pure C++. The code is based off of the book 'Ray Tracing in One Weekend',
so it is a very naive (and slow) implementation. However, the program uses multi-threading in order to scale well with modern processors. 

The raytracerprints its results to `std::cout` in the `ppm` image format (so that it can be saved to a file by doing `raytracer > image.ppm`), although the 
image is stored internally in a `8bitRGB` format (no transparency) so it can be changed quite easily.

## Single vs multi-core performance
On my machine with a Ryzen 5 3600 processor with 12 threads, and a 400x225px image, with 10 samples per pixel & max depth set to 50, the multi-threaded code
finishes in `50583ms`, or `50s`. With the `NO_THREADING` flag set, it takes about `330002ms`/`330s` or almost 6 minutes. By using multiple threads, the program
outputs the file about ten times faster than before.

## Building
The program uses the meson build system, although it could be easily ported to another one, considering it mostly consists of header files, with only a single
source file. No external dependencies are used, besides the standard library.

Note: If you somehow want to disable multi-threading (why?), you have to define the preprocessor macro `NO_THREADING`.
### Linux / WSL
It is extremely simple to build on linux. If you've never used meson before, make sure it is installed alongside `ninja` on your machine, and execute the 
following commands in the project root directory:
```
meson build
cd build && ninja
```
### Windows
I haven't tested it yet with a native windows toolchain yet, but if you're using the Microsoft C++ Compiler, before building you should modify in the `build.meson`
file `cpp_std=c++2a` to `cpp_std=c++latest` then you should be good to go.
