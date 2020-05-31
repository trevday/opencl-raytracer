#ifndef CAMERA_CL
#define CAMERA_CL

#include "ray.cl.h"
#include "utils.cl.h"

typedef struct camera {
  float3 origin;
  float3 lower_left_corner;
  float3 horizontal;
  float3 vertical;
  float3 u;
  float3 v;
  float3 w;

  float lens_radius;
} camera;

static ray get_ray(__constant camera* c, const float2 st,
                   unsigned int rand_state[1]) {
#if USE_PINHOLE_CAMERA
  ray r;
  r.o = c->origin;
  r.dir = c->lower_left_corner + (c->horizontal * st.x) + (c->vertical * st.y) -
          c->origin;
  return r;
#else
  float3 ray_disk = c->lens_radius * random_unit_disk(rand_state);
  float3 offset = c->u * ray_disk.x + c->v * ray_disk.y;

  ray r;
  r.o = c->origin + offset;
  r.dir = c->lower_left_corner + (c->horizontal * st.x) + (c->vertical * st.y) -
          c->origin - offset;
  return r;
#endif
}

#endif
