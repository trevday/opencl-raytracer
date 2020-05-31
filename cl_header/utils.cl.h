#ifndef UTILS_CL
#define UTILS_CL

// Pseudorandom number generator based on xorshift32:
// https://en.wikipedia.org/wiki/Xorshift
static float rand(unsigned int state[static 1]) {
  unsigned int x = state[0];
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return (float)(state[0] = x) / (float)(UINT_MAX);
}

static float3 random_unit_sphere(unsigned int state[static 1]) {
  float azimuth = rand(state) * M_PI_F * 2.0f;
  float y = rand(state);
  float sin_elevation = sqrt(1.0f - y * y);
  float x = sin_elevation * cos(azimuth);
  float z = sin_elevation * sin(azimuth);
  return (float3)(x, y, z);
}

static float3 random_unit_disk(unsigned int state[static 1]) {
  float x = 2.0f * rand(state) - 1.0f;
  float y = sqrt(1.0f - x * x);
  return (float3)(x, y, 0.0f);
}

static float3 reflect(float3 a, float3 b) { return a - 2.0f * dot(a, b) * b; }

static bool refract(float3 v, float3 normal, float index, float3* refracted) {
  float3 normalized = normalize(v);
  float dt = dot(normalized, normal);
  float discriminant = 1.0f - index * index * (1.0f - dt * dt);
  if (discriminant > 0.0f) {
    *refracted =
        index * (normalized - normal * dt) - normal * sqrt(discriminant);
    return true;
  }
  return false;
}

static float schlick(float cosine, float index) {
  float r0 = (1 - index) / (1 + index);
  r0 = r0 * r0;
  return r0 + (1 - r0) * pow((1 - cosine), 5);
}

#endif
