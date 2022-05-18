#include "angles.h"
#include <stdlib.h>
#include <math.h>

#include "perlin.h"

#define _USE_MATH_DEFINES

double
angle_const (double prev_angle)
{
    return prev_angle;
}

double
angle_random (double prev_angle)
{
    return (rand() / (M_PI + 1));
}

double
angle_rotating (double prev_angle)
{
    return (prev_angle * (M_2_PI + 1) + 10) / (M_2_PI + 1);
}

double
angle_noise (double prev_angle)
{
    return  (double) noise(M_PI, 42, prev_angle) * M_2_PI * 20;
}

