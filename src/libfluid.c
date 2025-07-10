#include "libfluid.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _FluidSurface {
    int    size;
    double diff;
    double visc;

    double *s;
    double *density;

    double *Vx;
    double *Vy;

    double *Vx0;
    double *Vy0;
} FluidSurface;

static int FluidSurfaceDimension;

static inline int
ix (const int x,
    const int y)
{
    return (x + y * (FluidSurfaceDimension + 2));
}

FluidSurface*
f_surface_create (int    size,
		              double diffusion,
		              double viscosity)
{
  FluidSurface *surface = malloc (sizeof (*surface));
  FluidSurfaceDimension = size;
  int N = size + 2;

  surface->size = size;
  surface->diff = diffusion;
  surface->visc = viscosity;

  surface->s = calloc (N * N, sizeof (double));
  surface->density = calloc (N * N, sizeof (double));

  surface->Vx = calloc (N * N, sizeof (double));
  surface->Vy = calloc (N * N, sizeof (double));

  surface->Vx0 = calloc (N * N, sizeof (double));
  surface->Vy0 = calloc (N * N, sizeof (double));

  return surface;
}

void
f_surface_free (FluidSurface *surface)
{
  free (surface->s);
  free (surface->density);

  free (surface->Vx);
  free (surface->Vy);

  free (surface->Vx0);
  free (surface->Vy0);

  free (surface);
}

static void
set_bnd (int     b,
	       double *x,
	       int     N)
{
  for(int i = 1; i < N - 1; i++) {
      x[ix(i, 0  )] = b == 2 ? -x[ix(i, 1  )] : x[ix(i, 1  )];
      x[ix(i, N-1)] = b == 2 ? -x[ix(i, N-2)] : x[ix(i, N-2)];
  }
  for(int j = 1; j < N - 1; j++) {
      x[ix(0  , j)] = b == 1 ? -x[ix(1  , j)] : x[ix(1  , j)];
      x[ix(N-1, j)] = b == 1 ? -x[ix(N-2, j)] : x[ix(N-2, j)];
  }

  x[ix(0, 0)]       = 0.33f * (x[ix(1, 0)]
                             + x[ix(0, 1)]);
  x[ix(0, N-1)]     = 0.33f * (x[ix(1, N-1)]
                             + x[ix(0, N-2)]);
  x[ix(N-1, 0)]     = 0.33f * (x[ix(N-2, 0)]
                             + x[ix(N-1, 1)]);
  x[ix(N-1, N-1)]   = 0.33f * (x[ix(N-2, N-1)]
                             + x[ix(N-1, N-2)]);
}

static void
lin_solve (int     b,
	         double       *x,
	         double const *x0,
	         const double  a,
	         const double  c,
	         int           iter,
	         const int     N)
{
  const double cRecip = 1.0 / c;
  for (int k = 0; k < iter; k++) {
      for (int j = 1; j < N - 1; j++) {
          for (int i = 1; i < N - 1; i++) {
              x[ix(i, j)] =
                  (x0[ix(i, j)]
                      + a*(    x[ix(i+1, j  )]
                              +x[ix(i-1, j  )]
                              +x[ix(i  , j+1)]
                              +x[ix(i  , j-1)]
                     )) * cRecip;
          }
      }
      set_bnd(b, x, N);
  }
}

static void
diffuse (const int     b,
         double       *x,
         double const *x0,
         const double  diff,
         const double  dt,
         int           iter,
         const int     N)
{
  double a = dt * diff * (N - 2) * (N - 2);
  lin_solve (b, x, x0, a, 1 + 6 * a, iter, N);
}

static void
advect (const int     b,
        double       *d,
        double const *d0,
        double const *velocX,
        double const *velocY,
        const double  dt,
        const int     N)
{
  const double dtx = dt * (N - 2);
  const double dty = dt * (N - 2);

  double Ndouble = N;
  double idouble;
  double jdouble;
  int i;
  int j;

  for (j = 1, jdouble = 1; j < N - 1; j++, jdouble++)
    {
      for (i = 1, idouble = 1; i < N - 1; i++, idouble++)
        {
          const double tmp1 = dtx * velocX[ix(i, j)];
          const double tmp2 = dty * velocY[ix(i, j)];
          double x = idouble - tmp1;
          double y = jdouble - tmp2;

          if (x < 0.5f) x = 0.5f;
          if (x > Ndouble + 0.5f) x = Ndouble + 0.5f;
          const double i0 = floor(x);
          const double i1 = i0 + 1.0f;
          if (y < 0.5f) y = 0.5f;
          if (y > Ndouble + 0.5f) y = Ndouble + 0.5f;
          const double j0 = floor(y);
          const double j1 = j0 + 1.0f;

          const double s1 = x - i0;
          const double s0 = 1.0f - s1;
          const double t1 = y - j0;
          const double t0 = 1.0f - t1;

          // TODO: range check these
          const int i0i = (int)i0;
          const int i1i = (int)i1;
          const int j0i = (int)j0;
          const int j1i = (int)j1;

          d[ix(i, j)] =

              s0 * ( t0 * d0[ix(i0i, j0i)]
                  +  t1 * d0[ix(i0i, j1i)])
             +s1 * ( t0 * d0[ix(i1i, j0i)]
                  +  t1 * d0[ix(i1i, j1i)]);
        }
    }
  set_bnd(b, d, N);
}

static void
project (double   *velocX,
	       double   *velocY,
	       double   *p,
	       double   *div,
	       int       iter,
	       const int N)
{
  for (int j = 1; j < N - 1; j++)
    {
      for (int i = 1; i < N - 1; i++)
        {
          div[ix(i, j)] = -0.5f * (
                   velocX[ix(i+1, j  )]
                  -velocX[ix(i-1, j  )]
                  +velocY[ix(i  , j+1)]
                  -velocY[ix(i  , j-1)]
              ) / N;
          p[ix(i, j)] = 0;
        }
    }
  set_bnd(0, div, N);
  set_bnd(0, p, N);
  lin_solve(0, p, div, 1, 6, iter, N);

  for (int j = 1; j < N - 1; j++)
    {
      for (int i = 1; i < N - 1; i++)
        {
          velocX[ix(i, j)] -= 0.5f * (  p[ix(i+1, j)]
                                          -p[ix(i-1, j)]) * N;
          velocY[ix(i, j)] -= 0.5f * (  p[ix(i, j+1)]
                                          -p[ix(i, j-1)]) * N;
        }
    }
  set_bnd(1, velocX, N);
  set_bnd(2, velocY, N);
}

void
f_surface_step (const FluidSurface *surface, const double dt)
{
  const int N           = surface->size;
  const double visc     = surface->visc;
  const double diff     = surface->diff;
  double *Vx      = surface->Vx;
  double *Vy      = surface->Vy;
  double *Vx0     = surface->Vx0;
  double *Vy0     = surface->Vy0;
  double *s       = surface->s;
  double *density = surface->density;

  diffuse(1, Vx0, Vx, visc, dt, 4, N);
  diffuse(2, Vy0, Vy, visc, dt, 4, N);

  project(Vx0, Vy0, Vx, Vy, 4, N);

  advect(1, Vx, Vx0, Vx0, Vy0, dt, N);
  advect(2, Vy, Vy0, Vx0, Vy0, dt, N);

  project(Vx, Vy, Vx0, Vy0, 4, N);

  diffuse(0, s, density, diff, dt, 4, N);
  advect(0, density, s, Vx, Vy, dt, N);
}

void
f_surface_add_density (const FluidSurface *surface,
		                   const int           x,
		                   const int           y,
		                   const double        amount)
{
  assert (amount >= 0.0 && amount <= 1.0 && "density should be between 0.0 and 1.0");
  double density = surface->density[ix(x, y)] + amount;
  if (density > 1.0)
    {
      fprintf(stderr, "Warning: density overflow, clamping to 1.0\n");
      density = 1.0;
    }
  surface->density[ix(x, y)] = density;
}

double
f_surface_get_density (FluidSurface const *surface,
                       const int           x,
                       const int           y)
{
  return surface->density[ix(x, y)];
}

void
f_surface_set_density (const FluidSurface *surface,
		                   const int           x,
		                   const int           y,
                       const double        amount)
{
  surface->density[ix(x, y)] = amount;
}

void
f_surface_add_velocity (const FluidSurface *surface,
			                  const int           x,
			                  const int           y,
			                  const double        amountX,
			                  const double        amountY)
{
  int index = ix(x, y);

  surface->Vx[index] += amountX;
  surface->Vy[index] += amountY;
}

void
f_surface_set_velocity (const FluidSurface *surface,
                        const int           x,
                        const int           y,
                        const double        amountX,
                        const double        amountY)
{
  const int index = ix(x, y);

  surface->Vx[index] = amountX;
  surface->Vy[index] = amountY;
}

void
f_surface_add_flow (FluidSurface const *surface,
	                  const double        amountX,
	                  const double        amountY)
{
  for (int i = 0; i < surface->size; i++)
    {
      for (int j = 0; j < surface->size; j++)
	      {
          f_surface_add_velocity (surface, j, i, amountX, amountY);
        }
    }
}

void
f_surface_add_whirl (FluidSurface const *surface,
		                 const double        vector_scale)
{
  double s;
  for (int i = 0; i < surface->size; i ++)
    {
      for (int j = 0; j < surface->size; j++)
	      {
	        s = sqrt(j * j + i * i);
	        if (s != 0)
	          f_surface_add_velocity (surface, j, i, -i / s * vector_scale, j / s * vector_scale);
	      }
    }
}

void
f_surface_clear (FluidSurface const *surface)
{
  for (int i = 0; i < surface->size; i++)
    {
      for (int j = 0; j < surface->size; j++)
	      {
          f_surface_set_density (surface, j, i, 0);
          f_surface_set_velocity (surface, j, i, 0, 0);
        }
    }
}

