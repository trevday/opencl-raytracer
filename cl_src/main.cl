#include "camera.cl.h"
#include "sphere.cl.h"

// The following definitions are expected for building this program:
// - WIDTH = width of the output image
// - HEIGHT = height of output image
// - SAMPLES = number of anti-aliasing samples
// - DEPTH = depth of bounces allowed for rays
// - NUM_SPHERES = number of spheres in scene
// - (Optional) USE_PINHOLE_CAMERA = indicates whether the camera
//   uses a simulated lens with surface area or not

// TODO: The division of work among kernels could probably be improved
__kernel void raytrace(__constant camera* cam, __constant sphere* spheres,
                       __global __write_only float* output
                       /* output to a buffer because color
                       compression occurs to output to image */) {
  // 0 is x, 1 is y, and 2 is s (sample point for anti-aliasing)
  const uint3 sector =
      (uint3)(get_global_id(0), get_global_id(1), get_global_id(2));
  // Make the random seed the unique index for each work item, quick hack
  uint rand_seed =
      ((sector.y * WIDTH * SAMPLES) + (sector.x * SAMPLES + sector.z)) * 3;
  // cache the index now after calculating, since rand seed will change
  const uint index = rand_seed;

  const float2 uv = (float2)(
      ((float)sector.x + rand(&rand_seed)) / (float)WIDTH,
      ((float)(HEIGHT - (sector.y + 1)) + rand(&rand_seed)) / (float)HEIGHT);

  ray r = get_ray(cam, uv, &rand_seed);

  float3 color = (float3)(1.0f, 1.0f, 1.0f);
  hit_record record;
  ray scattered;
  float3 attenuation = (float3)(0.0f, 0.0f, 0.0f);
  for (int i = 0; i < DEPTH; ++i) {
    if (hit_spheres(spheres, NUM_SPHERES, &r, 0.001f, MAXFLOAT, &record)) {
      if (scatter(&record, &r, &attenuation, &scattered, &rand_seed)) {
        color *= attenuation;
        r = scattered;
        continue;
      }

      color *= (float3)(0, 0, 0);
      break;
    }

    // Sky blend
    float3 dir = normalize(r.dir);
    float t = 0.5f * (dir.y + 1.0f);
    color *= (float3)(1.0f, 1.0f, 1.0f) * (1.0f - t) +
             (float3)(0.5f, 0.7f, 1.0f) * t;
    break;
  }

  output[index] = color.x;
  output[index + 1] = color.y;
  output[index + 2] = color.z;
};

// Compresses anti-aliasing samples for a pixel to a buffer
__kernel void color_compress_buffer(
    __global __read_only float* input,
    __global __write_only unsigned char* output) {
  const uint2 sector = (uint2)(get_global_id(0), get_global_id(1));

  float3 color = (float3)(0, 0, 0);
  for (int s = 0; s < SAMPLES; ++s) {
    int outIndex =
        ((sector.y * WIDTH * SAMPLES) + (sector.x * SAMPLES + s)) * 3;
    float3 temp =
        (float3)(input[outIndex], input[outIndex + 1], input[outIndex + 2]);
    color += temp;
  }
  color /= (float)SAMPLES;
  color = (float3)(sqrt(color.x), sqrt(color.y), sqrt(color.z));

  int i = (WIDTH * sector.y + sector.x) * 3;
  output[i] = (unsigned char)(255.99 * color.x);
  output[i + 1] = (unsigned char)(255.99 * color.y);
  output[i + 2] = (unsigned char)(255.99 * color.z);
};

// Compresses anti-aliasing samples for a pixel to an image
__kernel void color_compress_image(__global __read_only float* input,
                                   __write_only image2d_t output) {
  const int2 sector = (int2)(get_global_id(0), get_global_id(1));

  float3 color = (float3)(0, 0, 0);
  for (int s = 0; s < SAMPLES; ++s) {
    int outIndex =
        (((HEIGHT - sector.y) * WIDTH * SAMPLES) + (sector.x * SAMPLES + s)) *
        3;
    float3 temp =
        (float3)(input[outIndex], input[outIndex + 1], input[outIndex + 2]);
    color += temp;
  }
  color /= (float)SAMPLES;
  float4 finalColor =
      (float4)(sqrt(color.x), sqrt(color.y), sqrt(color.z), 1.0f);

  write_imagef(output, sector, finalColor);
};
