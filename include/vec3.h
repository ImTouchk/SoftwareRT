#ifndef VEC3_H
#define VEC3_H

#ifdef USE_MANUAL_INTRINSICS
#   include "vec3_avx.h"
#else
#   include "vec3_scalar.h"
#endif

#endif // VEC3_H
