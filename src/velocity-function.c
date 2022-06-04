#include <stdio.h>
#include <math.h>

#include "velocity-function.h"

VectorComponent
radial_function (VelocityParam param)
{
  double s = sqrt(param.pos_x * param.pos_x + param.pos_y * param.pos_y);
  VectorComponent v = { };
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
  VectorComponent v = { };
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
get_velocity_function (int selector)
{
  if (selector == F_VELOCITY_RADIAL_FN)
    return radial_function;
  else if (selector == F_VELOCITY_SPIRAL_FN)
    return spiral_function;
  else if (selector == F_VELOCITY_DIRECTIONAL_FN) /* the rest */
    return north_function;
  else if (selector == 3)
    return northeast_function;
  else if (selector == 4)
    return east_function;
  else if (selector == 5)
    return southeast_function;
  else if (selector == 6)
    return south_function;
  else if (selector == 7)
    return southwest_function;
  else if (selector == 8)
    return west_function;
  else if (selector == 9)
    return northwest_function;
  else
    return NULL;
}

