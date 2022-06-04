#ifndef LIB_FLUID_H

#define LIB_FLUID_H


typedef struct _FluidSurface FluidSurface;


FluidSurface* f_surface_create       (int           size,
				                              double        diffusion,
				                              double        viscosity);

void          f_surface_free         (FluidSurface *surface);

void          f_surface_step         (FluidSurface *surface, double dt);

void          f_surface_add_density  (FluidSurface *surface,
		                                  int           x,
		                                  int           y,
		                                  double        amount);

double        f_surface_get_density  (FluidSurface *surface,
		                                  int           x,
		                                  int           y);

void          f_surface_set_density  (FluidSurface *surface,
		                                  int           x,
		                                  int           y,
                                      double        amount);

void          f_surface_add_velocity (FluidSurface *surface,
			                                int           x,
			                                int           y,
			                                double        amountX,
			                                double        amountY);

void          f_surface_set_velocity (FluidSurface *surface,
			                                int           x,
			                                int           y,
			                                double        amountX,
			                                double        amountY);

void          f_surface_add_flow     (FluidSurface *surface,
	                                    double        amountX,
	                                    double        amountY);

void          f_surface_add_whirl    (FluidSurface *surface,
		                                  double        vector_scale);

void          f_surface_clear        (FluidSurface *surface);

#endif
