#include "libfluid.h"

#include <assert.h>
#include <math.h>
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
ix (int x,
    int y)
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
	         double *x,
	         double *x0,
	         double  a,
	         double  c,
	         int     iter,
	         int     N)
{
  double cRecip = 1.0 / c;
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
diffuse (int     b,
	       double *x,
	       double *x0,
	       double  diff,
	       double  dt,
	       int     iter,
	       int     N)
{
  double a = dt * diff * (N - 2) * (N - 2);
  lin_solve (b, x, x0, a, 1 + 6 * a, iter, N);
}

static void
advect (int     b,
	      double *d,
	      double *d0,
	      double *velocX,
	      double *velocY,
	      double  dt,
	      int     N)
{
  double i0, i1, j0, j1;

  double dtx = dt * (N - 2);
  double dty = dt * (N - 2);

  double s0, s1, t0, t1;
  double tmp1, tmp2, x, y;

  double Ndouble = N;
  double idouble, jdouble;
  int i, j;

  for (j = 1, jdouble = 1; j < N - 1; j++, jdouble++)
    {
      for (i = 1, idouble = 1; i < N - 1; i++, idouble++)
        {
          tmp1 = dtx * velocX[ix(i, j)];
          tmp2 = dty * velocY[ix(i, j)];
          x    = idouble - tmp1;
          y    = jdouble - tmp2;

          if (x < 0.5f) x = 0.5f;
          if (x > Ndouble + 0.5f) x = Ndouble + 0.5f;
          i0 = floorf(x);
          i1 = i0 + 1.0f;
          if (y < 0.5f) y = 0.5f;
          if (y > Ndouble + 0.5f) y = Ndouble + 0.5f;
          j0 = floorf(y);
          j1 = j0 + 1.0f;

          s1 = x - i0;
          s0 = 1.0f - s1;
          t1 = y - j0;
          t0 = 1.0f - t1;

          int i0i = i0;
          int i1i = i1;
          int j0i = j0;
          int j1i = j1;

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
project (double *velocX,
	       double *velocY,
	       double *p,
	       double *div,
	       int     iter,
	       int     N)
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
f_surface_step (FluidSurface *surface, double dt)
{
  int N           = surface->size;
  double visc     = surface->visc;
  double diff     = surface->diff;
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
f_surface_add_density (FluidSurface *surface,
		                   int           x,
		                   int           y,
		                   double        amount)
{
  assert (amount >= 0.0 && amount <= 1.0 && "density should be between 0.0 and 1.0");
  double density = surface->density[ix(x, y)] + amount;
  if (density <= 1.0)
    surface->density[ix(x, y)] = density;
  // else
    // printf ("Density overflow (%f)\n", density);
}

double
f_surface_get_density (FluidSurface *surface,
		                   int           x,
		                   int           y)
{
  return surface->density[ix(x, y)];
}

void
f_surface_add_velocity (FluidSurface *surface,
			                  int           x,
			                  int           y,
			                  double        amountX,
			                  double        amountY)
{
  int index = ix(x, y);

  surface->Vx[index] += amountX;
  surface->Vy[index] += amountY;
}

void
add_flow (FluidSurface *surface,
	        double        amountX,
	        double        amountY)
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
f_surface_add_whirl (FluidSurface *surface,
		                 double        vector_scale)
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

