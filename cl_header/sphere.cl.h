#ifndef SPHERE_CL
#define SPHERE_CL

#include "material.cl.h"

typedef struct sphere {
  float3 center;
  float radius;
  material m;
} sphere;

static bool hit(__constant sphere* s, const ray* r, float t_min, float t_max,
                hit_record* record) {
  float3 oc = r->o - s->center;
  float a = dot(r->dir, r->dir);
  float b = 2.0f * dot(oc, r->dir);
  float c = dot(oc, oc) - s->radius * s->radius;
  float discriminant = b * b - 4.0f * a * c;

  if (discriminant > 0.0f) {
    float t_hit = (-b - sqrt(discriminant)) / (2.0f * a);
    if (t_hit >= t_max || t_hit <= t_min) {
      t_hit = (-b + sqrt(discriminant)) / (2.0f * a);
    }

    if (t_hit < t_max && t_hit > t_min) {
      record->t = t_hit;
      record->p = point_at(r, t_hit);
      record->normal = normalize((record->p - s->center) / s->radius);
      record->m = &(s->m);
      return true;
    }
  }
  return false;
}

static bool hit_spheres(__constant sphere* s, int num_spheres, const ray* r,
                        float t_min, float t_max, hit_record* record) {
  hit_record temp_rec;
  float closest = t_max;
  bool hit_anything = false;
  for (int i = 0; i < num_spheres; ++i) {
    if (hit(&s[i], r, t_min, closest, &temp_rec)) {
      hit_anything = true;
      closest = temp_rec.t;
      *record = temp_rec;
    }
  }
  return hit_anything;
}

#endif
