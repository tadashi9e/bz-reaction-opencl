// -*- mode:c -*-

inline float get(
    __global float *map,
    int x,
    int y,
    int width,
    int height) {
  if (x < 0) {
    x += width;
  } else if (x >= width) {
    x -= width;
  }
  if (y < 0) {
    y += height;
  } else if (y >= height) {
    y -= height;
  }
  return map[y * width + x];
}

inline float average(
    __global float *src,
    __global float *dst,
    int x,
    int y,
    int width,
    int height) {
  float sum = 0.0;
  int count = 0;
  for (int dy = -1; dy < 2; ++dy) {
    for (int dx = -1; dx < 2; ++dx) {
      if (dx != 0 || dy != 0) {
        sum += get(src, x+dx, y+dy, width, height);
        ++count;
      }
    }
  }
  return sum / count;
}

__kernel void bz_diffusion(
    __global float *src,
    __global float *dst,
    int width,
    int height,
    float rate) {
  const int x = get_global_id(0);
  const int y = get_global_id(1);
  if (x >= width || y >= height) return;
  const float nvc = average(src, dst, x, y, width, height);
  const float c =
    (1.0 - rate) * get(src, x, y, width, height) +
    rate * nvc;
  dst[y * width + x] = c;
}

inline float limit(float c) {
  if (c < 0.0) {
    return 0.0;
  }
  if (c > 1.0) {
    return 1.0;
  }
  return c;
}

__kernel void bz_reaction(
    __global float *a,
    __global float *b,
    __global float *c,
    __global float *a2,
    __global float *b2,
    __global float *c2,
    int width,
    int height,
    float param_a,
    float param_b,
    float param_c) {
  const int x = get_global_id(0);
  const int y = get_global_id(1);
  if (x >= width || y >= height) return;
  const int index = y * width + x;
  const float ca = a[index];
  const float cb = b[index];
  const float cc = c[index];
  const float ca2 = limit(ca + ca * (param_a * cb - param_c * cc));
  const float cb2 = limit(cb + cb * (param_b * cc - param_a * ca));
  const float cc2 = limit(cc + cc * (param_c * ca - param_b * cb));
  a2[index] = ca2;
  b2[index] = cb2;
  c2[index] = cc2;
}

__kernel void bz_draw_image(
    __global float *a,
    __global float *b,
    __global float *c,
    __write_only image2d_t image,
    int width,
    int height) {
  const int x = get_global_id(0);
  const int y = get_global_id(1);
  if (x >= width || y >= height) return;
  const int index = y * width + x;
  const float p_r = a[index];
  const float p_g = b[index];
  const float p_b = c[index];
  const float4 pixel = (float4)(p_r, p_g, p_b, 1.0);
  write_imagef(image, (int2)(x,y), pixel);
}
