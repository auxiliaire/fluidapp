#ifndef F_VELOCITY_FUNCTION_H

#define F_VELOCITY_FUNCTION_H

#define F_VELOCITY_RADIAL_FN      0
#define F_VELOCITY_SPIRAL_FN      1
#define F_VELOCITY_DIRECTIONAL_FN 2

typedef struct {
  int    pos_x;
  int    pos_y;
  double scale;
  double intensity;
} VelocityParam;

typedef struct {
  double x;
  double y;
  int    valid;
} VectorComponent;

typedef VectorComponent (*VelocityFunction)(VelocityParam param);

VectorComponent  radial_function       (VelocityParam param);

VectorComponent  spiral_function       (VelocityParam param);

VectorComponent  north_function        (VelocityParam param);

VectorComponent  northeast_function    (VelocityParam param);

VectorComponent  east_function         (VelocityParam param);

VectorComponent  southeast_function    (VelocityParam param);

VectorComponent  south_function        (VelocityParam param);

VectorComponent  southwest_function    (VelocityParam param);

VectorComponent  west_function         (VelocityParam param);

VectorComponent  northwest_function    (VelocityParam param);

VelocityFunction get_velocity_function (int selector);

#endif

