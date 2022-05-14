/* fluidapp-window.c
 *
 * Copyright 2022 Viktor Daróczi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fluidapp-config.h"
#include "fluidapp-window.h"
#include "libfluid.h"

#define DEFAULT_SIZE      512
#define DEFAULT_DIFFUSION 0.0
#define DEFAULT_VISCOSITY 0.0
#define DEFAULT_ANGLE     0.0
#define DEFAULT_DELTA_X   3
#define DEFAULT_DELTA_Y   1
#define DEFAULT_DELTA_T   0.5
#define BITS_PER_SAMPLE   8
#define FILL_COLOR        0x000000ff

struct _FluidappWindow
{
  GtkApplicationWindow  parent_instance;

  /* Template widgets */
  GtkHeaderBar        *header_bar;
  GtkPicture          *scene;
  guint                dimension;
  float                diffusion;
  float                viscosity;
  float                angle;
  FluidSurface        *fluid;
  GdkPixbuf           *pixbuf;
  gint64               t;
  guint                cx;
  guint                cy;
  int                  dx;
  int                  dy;
  float                dt;
  double               px;
  double               py;
  GtkSwitch           *autoink;
};

G_DEFINE_TYPE (FluidappWindow, fluidapp_window, GTK_TYPE_APPLICATION_WINDOW)

static void
fluidapp_window_class_init (FluidappWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/de/teamaux/Fluidapp/fluidapp-window.ui");
  gtk_widget_class_bind_template_child (widget_class, FluidappWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, FluidappWindow, scene);
  gtk_widget_class_bind_template_child (widget_class, FluidappWindow, autoink);
}

inline static int
is_point_in_circle (int x,
		                int y,
		                int r)
{
    return (x * x + y * y - r * r) <= 0;
}

inline static int
to_original (double original_expansion,
             double actual_expansion,
             double actual_position)
{
  return (int) (actual_position
        / (actual_expansion / original_expansion));
}

inline static int
to_orig_x (double original_width,
           double actual_width,
           double actual_x)
{
  return to_original (original_width,
                      actual_width,
                      actual_x);
}

inline static int
to_orig_y (double original_height,
           double actual_height,
           double actual_y)
{
  return to_original (original_height,
                      actual_height,
                      actual_y);
}

static void
add_drop (FluidappWindow *self, int centerX, int centerY)
{
  // cx = (int) (dimension / 2.0);
  // cy = (int) (dimension / 2.0); // - 10; // (int) (dimension / 2.0);
  if (centerX - 10 < 0
      || centerX + 10 > (int) self->dimension
      || centerY - 10 < 0
      || centerY + 10 > (int) self->dimension)
    return;

  float lower = 5; // 50;
  float upper = 15; // 150;
  float vector_scale = 0.1; // 0.35;
  // float angle;
  // float v;
  float s;
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
              f_surface_add_density (self->fluid, centerX + j, centerY + i, (rand() / (upper - lower + 1)) + lower);
	            // FluidSurfaceAddVelocity(fluid, cx + j, cy + i, cos(angle) * vector_scale, sin(angle) * vector_scale);
	            s = sqrt(j * j + i * i);
	            if (s > 0)
		            {
	                f_surface_add_velocity (self->fluid,
					        centerX + j,
					        centerY + i,
					        -i / s * vector_scale,
					        j / s * vector_scale);
                }
	          }
	      }
    }
}

static void
add_drop_rel (FluidappWindow *self, double center_x, double center_y)
{
  GdkRectangle a = { .x = 0, .y = 0, .width = 0, .height = 0 };
  gtk_widget_get_allocation (GTK_WIDGET (self->scene), &a);
  /* g_print ("x: %d, y: %d, w: %d, h: %d, cx: %f, cy: %f\n", */
  /*          a.x, a.y, a.width, a.height, center_x, center_y); */

  add_drop (self,
            to_orig_x (self->dimension, a.width, center_x),
            to_orig_y (self->dimension, a.height, center_y));
}

static gboolean
drag_begin (GtkGestureDrag *gesture,
            double          x,
            double          y,
            FluidappWindow *self)
{
  self->px = x;
  self->py = y;

  add_drop_rel (self, x, y);

  return TRUE; // handled
}

static gboolean
drag_update (GtkGestureDrag *gesture,
             double          x,
             double          y,
             FluidappWindow *self)
{
  add_drop_rel (self, self->px + x, self->py + y);

  return TRUE;
}

static void
render (FluidappWindow *self)
{
  GdkPixbuf *pixbuf_new = gdk_pixbuf_new (
		GDK_COLORSPACE_RGB,
		FALSE,
		BITS_PER_SAMPLE,
		self->dimension,
		self->dimension
	);
  /* if (self->pixbuf == NULL) */
  /*   return; */
  int rowstride = gdk_pixbuf_get_rowstride (pixbuf_new);
  int n_channels = gdk_pixbuf_get_n_channels (pixbuf_new);
  guchar *pixels = gdk_pixbuf_get_pixels (pixbuf_new);

  for (guint i = 0; i < self->dimension; i++)
    {
      for (guint j = 0; j < self->dimension; j++)
        {
	        float d = f_surface_get_density (self->fluid, j, i) / 10000000;
	        //if (d > 500) {
	        //	g_print ("d = %f\n", d);
	        //}
	        int r = (int) (3 * d / 3.90625) % 256;
	        int g = (int) (2 * d / 3.90625) % 256;
	        int b = (int) (1 * d / 3.90625) % 256;

          guchar *p = pixels + i * rowstride + j * n_channels;
          // RGB:
          p[0] = (int) (r > 255 ? 255 : r); // ((2 * d) / 256; // 0;
          p[1] = (int) (g > 255 ? 255 : g); // (int) d % 255;
          p[2] = (int) (b > 255 ? 255 : b); // (int)(d / 2) % 255; // (int) (d / 2) % 255; // (int)(d + 50) % 255;
          p[3] = 255;
        }
    }
  gtk_picture_set_pixbuf (self->scene, pixbuf_new);

  //free (self->pixbuf);
  //self->pixbuf = pixbuf_new;
}

static gboolean
tick_callback (GtkWidget *widget, GdkFrameClock *frame_clock, gpointer data)
{
  FluidappWindow *self = (FluidappWindow*) data;

  gint64 time = gdk_frame_clock_get_frame_time (frame_clock);

  if (self->t == 0)
    self->dt = DEFAULT_DELTA_T;
  else
    self->dt = (time - self->t) / 500000.0;

  self->t = time;

  if (gtk_switch_get_active (self->autoink))
    {
      add_drop (self, self->cx, self->cy);
      //float noize;
      //float vector_scale = -1.2;
      //noize = angle_noise(t) / M_PI;
      // angle = angle_random(0.0); // noise(t) * M_2_PI * 2;
      //angle = - noize; // angle_noise(t); // angle_random(-1.0); // angle_const(M_PI); // angle_rotating(angle);
      //for (int i = -10; i < 10; i++) {
       //vector_scale = noize * M_PI; // / (M_PI);
        // g_printf ("%f\n", angle);
          // PVector v = PVector.fromAngle(angle);
          // v.mult(0.2);
      // FluidSurfaceAddVelocity(fluid, cx + i, cy, cos(angle) * vector_scale, sin(angle) * vector_scale);
      // fluid.addVelocity(cx, cy, v.x, v.y );
      //}
      self->cx += self->dx;
      if ((self->cx + 10) >= self->dimension)
        {
    	    self->dx = -self->dx;
        }
      if ((self->cx - 10) <= 0)
        {
    	    self->dx = -self->dx;
        }
      self->cy += self->dy;
      if ((self->cy + 10) >= self->dimension)
        {
    	    self->dy = -self->dy;
        }
      if ((self->cy - 10) <= 0)
    	  {
          self->dy = -self->dy;
        }
      // g_print ("cx: %d, cy: %d, dx: %d, dy: %d\n", cx, cy, dx, dy);
    }

  // g_print ("%f\n", self->dt);
  f_surface_step (self->fluid, self->dt);

  render (self);

  return G_SOURCE_CONTINUE;
}

static void
tick_callback_destroy_notify_callback (gpointer data)
{
  FluidappWindow *self = (FluidappWindow*) data;

  f_surface_free (self->fluid);
  g_print ("Destroy\n");
}

static void
fluidapp_window_init (FluidappWindow *self)
{
  GtkGesture *drag;
  gtk_widget_init_template (GTK_WIDGET (self));
  self->dimension = DEFAULT_SIZE;
  self->diffusion = DEFAULT_DIFFUSION;
  self->viscosity = DEFAULT_VISCOSITY;
  self->angle     = DEFAULT_ANGLE;
  self->cx        = (int) (self->dimension / 4.0);
  self->cy        = (int) (self->dimension / 4.0);
  self->dx        = DEFAULT_DELTA_X;
  self->dy        = DEFAULT_DELTA_Y;
  self->t         = 0;
  self->fluid     = f_surface_create (self->dimension,
                                      self->diffusion,
                                      self->viscosity,
                                      DEFAULT_DELTA_T);
  self->pixbuf    = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                                    FALSE,
                                    BITS_PER_SAMPLE,
                                    self->dimension,
                                    self->dimension);
  gdk_pixbuf_fill (self->pixbuf, FILL_COLOR);

  gtk_picture_set_pixbuf (self->scene, self->pixbuf);

  gtk_widget_add_tick_callback (GTK_WIDGET (self->scene),
                                tick_callback,
                                self,
                                tick_callback_destroy_notify_callback);

  drag = gtk_gesture_drag_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (drag), GDK_BUTTON_PRIMARY);
  gtk_widget_add_controller (GTK_WIDGET (self->scene),
                             GTK_EVENT_CONTROLLER (drag));
  g_signal_connect (drag,
			              "drag-begin",
			              G_CALLBACK (drag_begin),
			              self);
	g_signal_connect (drag,
			              "drag-update",
			              G_CALLBACK (drag_update),
			              self);
	g_signal_connect (drag,
			              "drag-end",
			              G_CALLBACK (drag_update),
			              self);

}
