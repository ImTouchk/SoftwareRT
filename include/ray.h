#ifndef RAY_H
#define RAY_H

#include "vec3.h"

class ray {
    public:
        ray() {}
        ray(const point3& origin, const vec3& direction) 
            : org(origin), dir(direction) {}

        point3 origin() const  { return org; }
        vec3 direction() const { return dir; }

        point3 at(double t) const
        {
            return org + t * dir;
        }

    public:
        point3 org;
        vec3 dir;
};

#endif // RAY_H
