#include "angles.h"
#include <stdlib.h>
#include <math.h>

#include "perlin.h"

#define _USE_MATH_DEFINES

float
angle_const (float prev_angle)
{
    return prev_angle;
}

float
angle_random (float prev_angle)
{
    return (rand() / (M_PI + 1));
}

float
angle_rotating (float prev_angle)
{
    return (prev_angle * (M_2_PI + 1) + 10) / (M_2_PI + 1);
}

float
angle_noise (float prev_angle)
{
    return  (float) noise(M_PI, 42, prev_angle) * M_2_PI * 20;
}

