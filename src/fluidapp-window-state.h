#pragma once

#include <gtk/gtk.h>

#include "libfluid.h"

G_BEGIN_DECLS

#define DEFAULT_SIZE          512
#define DEFAULT_DIFFUSION     0.0
#define DEFAULT_VISCOSITY     0.0
#define DEFAULT_ANGLE         0.0
#define DEFAULT_DELTA_X       3
#define DEFAULT_DELTA_Y       1
#define DEFAULT_DELTA_T       0.5
#define DEFAULT_VECTOR_SCALE  0.1
#define TIME_FACTOR_LOWER     0.0000001
#define TIME_FACTOR_UPPER     0.00001
#define DEFAULT_TIME_FACTOR   0.000002
#define BITS_PER_SAMPLE       8
#define FILL_COLOR            0x000000ff

typedef struct _FluidappWindowState
{
  guint               dimension;
  double              diffusion;
  double              viscosity;
  double              angle;
  FluidSurface       *fluid;
  gint64              t;
  double              time_factor;
  double              vector_scale;
  guint               cx;
  guint               cy;
  int                 dx;
  int                 dy;
  double              dt;
  double              px;
  double              py;
} FluidappWindowState;

FluidappWindowState* fluidapp_window_state_create   ();
void                 fluidapp_window_state_free     (FluidappWindowState *state);

void                 fluidapp_window_state_add_drop (FluidappWindowState *state,
                                                     int                  centerX,
                                                     int                  centerY,
                                                     int                  density_hint,
                                                     guint                modifier);

G_END_DECLS

