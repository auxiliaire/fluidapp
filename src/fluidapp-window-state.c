#include "fluidapp-window-state.h"
#include "libfluid.h"

FluidappWindowState* fluidapp_window_state_create ()
{
  FluidappWindowState *state = malloc(sizeof(*state));
  state->dimension    = DEFAULT_SIZE;
  state->diffusion    = DEFAULT_DIFFUSION;
  state->viscosity    = DEFAULT_VISCOSITY;
  state->angle        = DEFAULT_ANGLE;
  state->cx           = (int) (state->dimension / 4.0);
  state->cy           = (int) (state->dimension / 4.0);
  state->dx           = DEFAULT_DELTA_X;
  state->dy           = DEFAULT_DELTA_Y;
  state->vector_scale = DEFAULT_VECTOR_SCALE;
  state->t            = 0;
  state->time_factor  = DEFAULT_TIME_FACTOR;
  state->fluid        = f_surface_create (state->dimension,
                                          state->diffusion,
                                          state->viscosity);
  return state;
}

void
fluidapp_window_state_free (FluidappWindowState *state)
{
  f_surface_free (state->fluid);
  free (state);
}

inline static int
is_point_in_circle (int x,
		                int y,
		                int r)
{
    return (x * x + y * y - r * r) <= 0;
}

void
fluidapp_window_state_add_drop (FluidappWindowState *state,
                                int                  centerX,
                                int                  centerY,
                                int                  density_hint,
                                guint                modifier)
{
  // cx = (int) (dimension / 2.0);
  // cy = (int) (dimension / 2.0); // - 10; // (int) (dimension / 2.0);
  if (centerX - 10 < 1
      || centerX + 10 > (int) state->dimension - 1
      || centerY - 10 < 1
      || centerY + 10 > (int) state->dimension - 1)
    return;

  int lower = 5; // 50;
  int upper = density_hint; // 150;
  int span = upper - lower + 1;
  double density;
  double intensity;
  // double angle;
  // double v;
  double s;
  for (int i = -10; i <= 10; i++)
    {
      // g_print("--------------\n");
      for (int j = -10; j <= 10; j++)
	      {
	        // angle = (4.5 * j - 90) * (M_PI / 180.0); // j / (2 * M_PI);
	        // g_print("angle = %f\n", angle);
          // g_printf ("%d, %d\n", cx+i, cy+j);
          if (is_point_in_circle(j, i, 10))
	          {
              density = ((rand() % span) + lower) / 100.0;
              // g_print ("density = %f\n", density);
              intensity = density / (upper / 100.0);
              // g_print ("intensity = %f\n", intensity);
              if ((modifier == 1) || (modifier == 2))
                f_surface_add_density (state->fluid, centerX + j, centerY + i, density);
	            // FluidSurfaceAddVelocity(fluid, cx + j, cy + i, cos(angle) * vector_scale, sin(angle) * vector_scale);
              if ((modifier == 1) || (modifier == 3))
                {
                  // Velocity function
	                s = sqrt(j * j + i * i);
	                if (s > 0)
                    f_surface_add_velocity (state->fluid,
			                                      centerX + j,
			                                      centerY + i,
			                                      j / s * state->vector_scale * intensity, // -i / s * state->vector_scale * intensity,
			                                      i / s * state->vector_scale * intensity);  // j / s * state->vector_scale * intensity);
                }
	          }
	      }
    }
}

