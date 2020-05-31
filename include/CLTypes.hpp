#ifndef CL_TYPES_HPP
#define CL_TYPES_HPP

#include "Vector3.hpp"

namespace CLTypes {

struct Camera {
  cl_float3 origin;
  cl_float3 lower_left_corner;
  cl_float3 horizontal;
  cl_float3 vertical;
  cl_float3 u;
  cl_float3 v;
  cl_float3 w;

  float lens_radius;
};

// LAMBERTIAN
struct Lambertian {
  cl_float3 albedo;

  Lambertian(Vector3 albedo) : albedo(albedo.GetCLVector()) {}
};

// METAL
struct Metal {
  cl_float3 albedo;
  cl_float fuzziness;

  Metal(Vector3 albedo, cl_float fuzziness)
      : albedo(albedo.GetCLVector()), fuzziness(fuzziness) {}
};

// DIELECTRIC
struct Dielectric {
  cl_float r_index;

  Dielectric(cl_float r_index) : r_index(r_index) {}
};

// Base Material
enum MaterialType { LAMBERTIAN, METAL, DIELECTRIC };

struct Material {
  MaterialType type;
  union {
    Lambertian l;
    Metal me;
    Dielectric d;
  };

  Material() {}
  Material(Lambertian l) : type(MaterialType::LAMBERTIAN), l(l) {}
  Material(Metal me) : type(MaterialType::METAL), me(me) {}
  Material(Dielectric d) : type(MaterialType::DIELECTRIC), d(d) {}
};

struct Sphere {
  cl_float3 center;
  cl_float radius;
  Material m;

  Sphere() {}
  Sphere(Vector3 center, cl_float radius, Material m)
      : center(center.GetCLVector()), radius(radius), m(m) {}
};

}  // namespace CLTypes

#endif
