#ifndef MATERIAL_CL
#define MATERIAL_CL

#include "ray.cl.h"
#include "utils.cl.h"

// LAMBERTIAN
typedef struct lambertian {
  float3 albedo;
} lambertian;

// METAL
typedef struct metal {
  float3 albedo;
  float fuzziness;
} metal;

// DIELECTRIC
typedef struct dielectric {
  float r_index;
} dielectric;

// Base Material
typedef enum material_type { LAMBERTIAN, METAL, DIELECTRIC } material_type;

typedef struct material {
  material_type type;
  union {
    lambertian l;
    metal me;
    dielectric d;
  };
} material;

// Hit Record
typedef struct hit_record {
  float t;
  float3 p;
  float3 normal;
  __constant material* m;
} hit_record;

static bool lambertian_scatter(const hit_record* record, const ray* r,
                               float3* attenuation, ray* scattered,
                               unsigned int rand_state[static 1]) {
  *attenuation = record->m->l.albedo;

  float3 target = record->p + record->normal + random_unit_sphere(rand_state);

  ray sc;
  sc.o = record->p;
  sc.dir = target - record->p;
  *scattered = sc;

  return true;
}

static bool metal_scatter(const hit_record* record, const ray* r,
                          float3* attenuation, ray* scattered,
                          unsigned int rand_state[static 1]) {
  *attenuation = record->m->me.albedo;

  float3 reflected = reflect(normalize(r->dir), record->normal);

  ray sc;
  sc.o = record->p;
  sc.dir = reflected + record->m->me.fuzziness * random_unit_sphere(rand_state);
  *scattered = sc;

  return dot(sc.dir, record->normal) > 0.0f;
}

static bool dielectric_scatter(const hit_record* record, const ray* r,
                               float3* attenuation, ray* scattered,
                               unsigned int rand_state[static 1]) {
  *attenuation = (float3)(1.0f, 1.0f, 1.0f);

  float normal_dot = dot(r->dir, record->normal);

  float index;
  float3 out_normal;
  float reflection_prob;
  float cosine;

  if (normal_dot > 0.0f) {
    out_normal = -record->normal;
    index = record->m->d.r_index;
    cosine = record->m->d.r_index * normal_dot / length(r->dir);
  } else {
    out_normal = record->normal;
    index = 1.0f / record->m->d.r_index;
    cosine = -normal_dot / length(r->dir);
  }

  float3 refracted;
  if (refract(r->dir, out_normal, index, &refracted)) {
    reflection_prob = schlick(cosine, record->m->d.r_index);
  } else {
    reflection_prob = 1.0f;
  }

  ray sc;
  sc.o = record->p;
  if (rand(rand_state) < reflection_prob) {
    float3 reflected = reflect(r->dir, record->normal);
    sc.dir = reflected;
  } else {
    sc.dir = refracted;
  }
  *scattered = sc;
  return true;
}

// Returns the scattered ray
static bool scatter(const hit_record* record, const ray* r, float3* attenuation,
                    ray* scattered, unsigned int rand_state[static 1]) {
  switch (record->m->type) {
    case LAMBERTIAN:
      return lambertian_scatter(record, r, attenuation, scattered, rand_state);
      break;
    case METAL:
      return metal_scatter(record, r, attenuation, scattered, rand_state);
      break;
    case DIELECTRIC:
      return dielectric_scatter(record, r, attenuation, scattered, rand_state);
      break;
    default:
      return false;
      break;
  }
}

#endif
