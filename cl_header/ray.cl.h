#ifndef RAY_CL
#define RAY_CL

typedef struct ray {
  float3 o;
  float3 dir;
} ray;

static float3 point_at(const ray* r, float t) { return r->o + (r->dir * t); }

#endif
