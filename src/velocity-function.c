#include <math.h>

#include "velocity-function.h"

VectorComponent
radial_function (VelocityParam param)
{
  double s = sqrt(param.pos_x * param.pos_x + param.pos_y * param.pos_y);
  VectorComponent v = { .valid = 0, .x = .0, .y = .0 };
  if (s > 0)
    {
      v.valid = 1;
      v.x     = param.pos_x / s * param.scale * param.intensity;
      v.y     = param.pos_y / s * param.scale * param.intensity;
    }
  return v;
}

VectorComponent
spiral_function (VelocityParam param)
{
  double s = sqrt(param.pos_x * param.pos_x + param.pos_y * param.pos_y);
  VectorComponent v = { .valid = 0, .x = .0, .y = .0 };
  if (s > 0)
    {
      v.valid =  1;
      v.x     = -param.pos_y / s * param.scale * param.intensity;
      v.y     =  param.pos_x / s * param.scale * param.intensity;
    }
  return v;
}

VectorComponent
north_function (VelocityParam param)
{
  VectorComponent v = {
    .valid =  1,
    .x     =  0.0,
    .y     = -param.scale * param.intensity
  };
  return v;
}

VectorComponent
northeast_function (VelocityParam param)
{
  VectorComponent v = {
    .valid =  1,
    .x     =  param.scale * param.intensity,
    .y     = -param.scale * param.intensity
  };
  return v;
}

VectorComponent
east_function (VelocityParam param)
{
  VectorComponent v = {
    .valid = 1,
    .x     = param.scale * param.intensity,
    .y     = 0.0
  };
  return v;
}

VectorComponent
southeast_function (VelocityParam param)
{
  VectorComponent v = {
    .valid = 1,
    .x     = param.scale * param.intensity,
    .y     = param.scale * param.intensity
  };
  return v;
}


VectorComponent
south_function (VelocityParam param)
{
  VectorComponent v = {
    .valid = 1,
    .x     = 0.0,
    .y     = param.scale * param.intensity
  };
  return v;
}


VectorComponent
southwest_function (VelocityParam param)
{
  VectorComponent v = {
    .valid =  1,
    .x     = -param.scale * param.intensity,
    .y     =  param.scale * param.intensity
  };
  return v;
}

VectorComponent
west_function (VelocityParam param)
{
  VectorComponent v = {
    .valid =  1,
    .x     = -param.scale * param.intensity,
    .y     =  0.0
  };
  return v;
}

VectorComponent
northwest_function (VelocityParam param)
{
  VectorComponent v = {
    .valid =  1,
    .x     = -param.scale * param.intensity,
    .y     = -param.scale * param.intensity
  };
  return v;
}

VelocityFunction
get_velocity_function (const guint selector)
{
  if (selector == F_VELOCITY_RADIAL_FN)
    return radial_function;
  if (selector == F_VELOCITY_SPIRAL_FN)
    return spiral_function;
  if (selector == F_VELOCITY_DIRECTIONAL_FN) /* the rest */
    return north_function;
  if (selector == 4)
    return northeast_function;
  if (selector == 5)
    return east_function;
  if (selector == 6)
    return southeast_function;
  if (selector == 7)
    return south_function;
  if (selector == 8)
    return southwest_function;
  if (selector == 9)
    return west_function;
  if (selector == 10)
    return northwest_function;
  return nullptr;
}

