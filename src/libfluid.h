#ifndef LIB_FLUID_H

#define LIB_FLUID_H


typedef struct _FluidSurface FluidSurface;


FluidSurface* f_surface_create       (int           size,
				                              float         diffusion,
				                              float         viscosity,
				                              float         dt);

void          f_surface_free         (FluidSurface *surface);

void          f_surface_step         (FluidSurface *surface, float dt);

void          f_surface_add_density  (FluidSurface *surface,
		                                  int           x,
		                                  int           y,
		                                  float         amount);

float         f_surface_get_density  (FluidSurface *surface,
		                                  int           x,
		                                  int           y);

void          f_surface_add_velocity (FluidSurface *surface,
			                                int           x,
			                                int           y,
			                                float         amountX,
			                                float         amountY);

void          add_flow               (FluidSurface *surface,
	                                    float         amountX,
	                                    float         amountY);

void          f_surface_add_whirl    (FluidSurface *surface,
		                                  float         vector_scale);

#endif
