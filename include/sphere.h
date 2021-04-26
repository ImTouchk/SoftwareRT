#ifndef SPHERE_H
#define SPHERE_H

#include "vec3.h"
#include "hittable.h"

class sphere : public hittable {
    public:
        sphere() {}
        sphere(point3 cen, double r, shared_ptr<material> m)
            : center(cen), radius(r), mat_ptr(m) {};

        virtual bool hit(
            const ray& r,
            double t_min,
            double t_max,
            hit_record& rec
        ) const override;

    public:
        point3 center;
        double radius;
        shared_ptr<material> mat_ptr;
};

bool sphere::hit(const ray& r, double t_min, double t_max, hit_record& rec) const
{
    vec3 oc     = r.origin() - center;
    auto a      = r.direction().length_squared();
    auto half_b = dot(oc, r.direction());
    auto c      = oc.length_squared() - (radius * radius);

    auto discriminant = (half_b * half_b) - (a * c);
    if(discriminant < 0) 
        return false;

    auto sqrtd = sqrt(discriminant);

    /* find nearest  root that lies within the acceptable range*/

    auto root = (-half_b - sqrtd) / a;
    if(root < t_min || t_max < root) {
        root = (-half_b + sqrtd) / a;
        if(root < t_min || t_max < root)
            return false;
    }

    rec.t       = root;
    rec.point   = r.at(rec.t);
    rec.mat_ptr = mat_ptr;

    vec3 outward_normal = (rec.point - center) / radius;
    rec.set_face_normal(r, outward_normal);

    return true; 
}

#endif // SPHERE_H
