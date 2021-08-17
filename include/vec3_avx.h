#pragma once
#include <immintrin.h>
#include <iostream>
#include <format>
#include "common.h"

struct vec3
{
public:
	inline static vec3 random()
	{
		return vec3(
			random_double(),
			random_double(),
			random_double()
		);
	}

	inline static vec3 random(double min, double max)
	{
		return vec3(
			random_double(min, max),
			random_double(min, max),
			random_double(min, max)
		);
	}

	vec3()
	{
		// initialize all values to 0
		e = _mm256_set1_pd(0);
	}

	vec3(double e0, double e1, double e2)
	{
		e = _mm256_set_pd(e0, e1, e2, 0);
	}
	
	double x() const { return e.m256d_f64[0]; }
	double y() const { return e.m256d_f64[1]; }
	double z() const { return e.m256d_f64[2]; }

	vec3 operator-() const 
	{
		auto neg = _mm256_set1_pd(-1);

		vec3 res;
		res.e = _mm256_mul_pd(e, neg);
		
		return res;
	}

	double operator[](int i) const { return e.m256d_f64[i]; }
	double& operator[](int i) { return e.m256d_f64[i]; }

	vec3& operator+=(const vec3& v)
	{
		e = _mm256_add_pd(e, v.e);
		return *this;
	}

	vec3& operator*=(const double t)
	{
		auto a = _mm256_set1_pd(t);
		e = _mm256_mul_pd(e, a);
		return *this;
	}

	vec3& operator/=(const double t)
	{
		auto a = _mm256_set1_pd(t);
		e = _mm256_div_pd(e, a);
		return *this;
	}

	double length() const
	{
		return sqrt(length_squared());
	}

	double length_squared() const
	{
		auto clone = _mm256_mul_pd(e, e);
		double sum = 0;
		sum += e.m256d_f64[0]
			+ e.m256d_f64[1]
			+ e.m256d_f64[2];
		return sum;
	}

	bool near_zero() const
	{
		const auto s = 1e-8;
		return 
			(fabs(e.m256d_f64[0]) < s) && 
			(fabs(e.m256d_f64[1]) < s) && 
			(fabs(e.m256d_f64[2]) < s);
	}

public:
	// an avx register stores 4 values, but the last one will
	// be unused
	__m256d e; 
};

using point3 = vec3;
using color = vec3;

inline std::ostream& operator<<(std::ostream& out, const vec3& v)
{
	return out << std::format(
		"vec3 [ {}, {}, {} ]\n",
		v.e.m256d_f64[0],
		v.e.m256d_f64[1],
		v.e.m256d_f64[2]
	);
}

inline vec3 operator+(const vec3& u, const vec3& v)
{
	vec3 res;
	res.e = _mm256_add_pd(u.e, v.e);
	return res;
}

inline vec3 operator-(const vec3& u, const vec3& v)
{
	vec3 res;
	res.e = _mm256_sub_pd(u.e, v.e);
	return res;
}

inline vec3 operator*(const vec3& u, const vec3& v)
{
	vec3 res;
	res.e = _mm256_mul_pd(u.e, v.e);
	return res;
}

inline vec3 operator*(const double t, const vec3& v)
{
	auto a = _mm256_set1_pd(t);
	
	vec3 res;
	res.e = _mm256_mul_pd(a, v.e);
	return res;
}

inline vec3 operator*(const vec3& v, double t)
{
	return t * v;
}

inline vec3 operator/(vec3 v, double t)
{
	auto a = _mm256_set1_pd(t);

	vec3 res;
	res.e = _mm256_div_pd(v.e, a);
	return res;
}

inline double dot(const vec3& u, const vec3& v)
{
	auto c = _mm256_mul_pd(u.e, v.e);
	double sum = c.m256d_f64[0] +
				c.m256d_f64[1] +
				c.m256d_f64[2];
	return sum;
}

inline vec3 cross(const vec3& u, const vec3& v)
{
	// Got this implementation from here:
	// https://geometrian.com/programming/tutorials/cross-product/index.php

	auto tmp0 = _mm256_shuffle_pd(u.e, u.e, _MM_SHUFFLE(3, 0, 2, 1));
	auto tmp1 = _mm256_shuffle_pd(v.e, v.e, _MM_SHUFFLE(3, 1, 0, 2));
	auto tmp2 = _mm256_mul_pd(tmp0, v.e);
	auto tmp3 = _mm256_mul_pd(tmp0, tmp1);
	auto tmp4 = _mm256_shuffle_pd(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
	
	vec3 res;
	res.e = _mm256_sub_pd(tmp3, tmp4);
	return res;
}

inline vec3 unit_vector(vec3 v)
{
	vec3 res;
	res.e = _mm256_div_pd(
		v.e,
		_mm256_set1_pd(v.length())
	);

	return res;
}

inline vec3 random_in_unit_sphere()
{
	while (true) {
		auto p = vec3::random(-1, 1);
		if (p.length_squared() >= 1)
			continue;
		return p;
	}
}

inline vec3 random_unit_vector()
{
	return unit_vector(
		random_in_unit_sphere()
	);
}

inline vec3 random_in_hemisphere(const vec3& normal)
{
	auto in_unit_sphere = random_in_unit_sphere();
	if (dot(in_unit_sphere, normal) > 0.0)
		return in_unit_sphere;
	else return -in_unit_sphere;
}

inline vec3 random_in_unit_disk()
{
	while (true) {
		auto p = vec3(
			random_double(-1, 1),
			random_double(-1, 1),
			0
		);

		if (p.length_squared() >= 1)
			continue;

		return p;
	}
}

inline vec3 reflect(const vec3& v, const vec3& n)
{
	return v - 2 * dot(v, n) * n;
}

inline vec3 refract(const vec3& uv, const vec3& n, double etai_over_etat)
{
	auto cos_theta = fmin(dot(-uv, n), 1.0);
	vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
	vec3 r_out_parallel = -sqrt(fabs(1.0 - r_out_perp.length_squared())) * n;
	return r_out_perp + r_out_parallel;
}
