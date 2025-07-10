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
#include "fluidapp-window-state.h"

#define OVERDRIVE_FACTOR 996.09375 /* 255.0 * 3.90625 */

typedef guchar (*ColorFunction)(double, double);

struct _FluidappWindow
{
  GtkApplicationWindow  parent_instance;

  /* Template widgets */
  GtkHeaderBar        *header_bar;
  GtkPicture          *scene;
  GdkCursor           *cursor;
  GdkPixbuf           *pixbuf;
  GtkToggleButton     *play_button;
  GtkButton           *reset_button;
  GtkButton           *save_button;
  GtkColorButton      *color_button;
  GtkSwitch           *overdrive;
  GdkRGBA             *color;
  ColorFunction       as_color;
  GtkSwitch           *autoink;
  GtkScale            *time_scale;
  GtkScale            *vector_scale;
  GtkScale            *ink_density;
  GtkComboBoxText     *velocity_function;
  FluidappWindowState *state;
};

G_DEFINE_TYPE (FluidappWindow, fluidapp_window, GTK_TYPE_APPLICATION_WINDOW)

static void
fluidapp_window_class_init (FluidappWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/de/teamaux/Fluidapp/fluidapp-window.ui");
  gtk_widget_class_bind_template_child (widget_class, FluidappWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, FluidappWindow, scene);
  gtk_widget_class_bind_template_child (widget_class, FluidappWindow, play_button);
  gtk_widget_class_bind_template_child (widget_class, FluidappWindow, reset_button);
  gtk_widget_class_bind_template_child (widget_class, FluidappWindow, save_button);
  gtk_widget_class_bind_template_child (widget_class, FluidappWindow, color_button);
  gtk_widget_class_bind_template_child (widget_class, FluidappWindow, overdrive);
  gtk_widget_class_bind_template_child (widget_class, FluidappWindow, autoink);
  gtk_widget_class_bind_template_child (widget_class, FluidappWindow, time_scale);
  gtk_widget_class_bind_template_child (widget_class, FluidappWindow, vector_scale);
  gtk_widget_class_bind_template_child (widget_class, FluidappWindow, ink_density);
  gtk_widget_class_bind_template_child (widget_class, FluidappWindow, velocity_function);
}

static inline int
to_original (double original_expansion,
             double actual_expansion,
             double actual_position)
{
  return (int) (actual_position
        / (actual_expansion / original_expansion));
}

static inline int
to_orig_x (double original_width,
           double actual_width,
           double actual_x)
{
  return to_original (original_width,
                      actual_width,
                      actual_x);
}

static inline int
to_orig_y (double original_height,
           double actual_height,
           double actual_y)
{
  return to_original (original_height,
                      actual_height,
                      actual_y);
}

static void
add_drop_rel (FluidappWindow *self,
              double          center_x,
              double          center_y,
              guint           modifier)
{
  GdkRectangle a = { .x = 0, .y = 0, .width = 0, .height = 0 };
  gtk_widget_get_allocation (GTK_WIDGET (self->scene), &a);
  /* g_print ("x: %d, y: %d, w: %d, h: %d, cx: %f, cy: %f\n", */
  /*          a.x, a.y, a.width, a.height, center_x, center_y); */

  GtkAdjustment *adjustment = gtk_range_get_adjustment (GTK_RANGE (self->ink_density));
  int density_hint = gtk_adjustment_get_value (adjustment);

  fluidapp_window_state_add_drop (self->state,
                                  to_orig_x (self->state->dimension,
                                             a.width,
                                             center_x),
                                  to_orig_y (self->state->dimension,
                                             a.height,
                                             center_y),
                                  density_hint,
                                  modifier);
}

static gboolean
drag_begin (GtkGestureDrag *gesture,
            double          x,
            double          y,
            FluidappWindow *self)
{
  guint button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture));

  self->state->px = x;
  self->state->py = y;

  add_drop_rel (self, x, y, button);

  return TRUE; // handled
}

static gboolean
drag_update (GtkGestureDrag *gesture,
             double          x,
             double          y,
             FluidappWindow *self)
{
  guint button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture));

  add_drop_rel (self, self->state->px + x, self->state->py + y, button);

  return TRUE;
}

static void
put_pixel (GdkPixbuf *pixbuf,
           int x,
           int y,
           guchar red,
           guchar green,
           guchar blue,
           guchar alpha)
{
  int n_channels = gdk_pixbuf_get_n_channels (pixbuf);

  // Ensure that the pixbuf is valid
  g_assert (gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB);
  g_assert (gdk_pixbuf_get_bits_per_sample (pixbuf) == 8);
  g_assert (gdk_pixbuf_get_has_alpha (pixbuf));
  g_assert (n_channels == 4);

  const int width = gdk_pixbuf_get_width (pixbuf);
  const int height = gdk_pixbuf_get_height (pixbuf);

  // Ensure that the coordinates are in a valid range
  g_assert (x >= 0 && x < width);
  g_assert (y >= 0 && y < height);

  const int rowStride = gdk_pixbuf_get_rowstride (pixbuf);

  // The pixel buffer in the GdkPixbuf instance
  guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);

  // The pixel we wish to modify
  guchar *p = pixels + y * rowStride + x * n_channels;
  p[0] = red;
  p[1] = green;
  p[2] = blue;
  p[3] = alpha;
}

static inline guchar
capped_color (double density, double color)
{
  return (guchar) (density * color * 255.0) % 256;
}

static inline guchar
overdrive_color (double density, double color)
{
  return (guchar) (color * density * OVERDRIVE_FACTOR) % 256;
}

static void
render (FluidappWindow *self)
{
  /* if (self->pixbuf == NULL) */
  /*   return; */
  // int rowstride = gdk_pixbuf_get_rowstride (pixbuf_new);
  // int n_channels = gdk_pixbuf_get_n_channels (pixbuf_new);
  // guchar *pixels = gdk_pixbuf_get_pixels (pixbuf_new);

  for (int i = 0; i < self->state->dimension; i++)
    {
      for (int j = 0; j < self->state->dimension; j++)
        {
	        const double d = f_surface_get_density (self->state->fluid, j, i);
	        //if (d > 500) {
	        //	g_print ("d = %f\n", d);
	        //}
	        //guchar r = (guchar) (3 * d / 3.90625) % 256;
	        //guchar g = (guchar) (2 * d / 3.90625) % 256;
	        //guchar b = (guchar) (1 * d / 3.90625) % 256;

          //guchar *p = pixels + i * rowstride + j * n_channels;
          // RGB:
          //p[0] = r; // (int) (r > 255 ? 255 : r); // ((2 * d) / 256; // 0;
          //p[1] = g; // (int) (g > 255 ? 255 : g); // (int) d % 255;
          //p[2] = b; // (int) (b > 255 ? 255 : b); // (int)(d / 2) % 255; // (int) (d / 2) % 255; // (int)(d + 50) % 255;
          //p[3] = 255;
          put_pixel (self->pixbuf,
                     j,
                     i,
                     self->as_color (d, self->color->red), //(guchar) (3 * d / 3.90625) % 256,
                     self->as_color (d, self->color->green), //(guchar) (2 * d / 3.90625) % 256,
                     self->as_color (d, self->color->blue), //(guchar) (1 * d / 3.90625) % 256,
                     255);
        }
    }
  gtk_picture_set_pixbuf (self->scene, self->pixbuf);

  //free (self->pixbuf);
  //self->pixbuf = pixbuf_new;
}

static gboolean
tick (GtkWidget *widget, GdkFrameClock *frame_clock, gpointer data)
{
  FluidappWindow *self = (FluidappWindow*) data;
  FluidappWindowState *state = self->state;

  gint64 time = gdk_frame_clock_get_frame_time (frame_clock);

  if (state->t == 0)
    state->dt = DEFAULT_DELTA_T;
  else
    state->dt = (time - state->t) * state->time_factor;

  state->t = time;

  if (gtk_toggle_button_get_active (self->play_button))
    {
      if (gtk_switch_get_active (self->autoink))
        {
          GtkAdjustment *adjustment = gtk_range_get_adjustment (GTK_RANGE (self->ink_density));
          int density_hint = gtk_adjustment_get_value (adjustment);

          fluidapp_window_state_add_drop (state, state->cx, state->cy, density_hint, 1);
          //double noize;
          //double vector_scale = -1.2;
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
          state->cx += state->dx;
          if ((state->cx + 10) >= state->dimension - 1)
            {
        	    state->dx = -state->dx;
            }
          if ((state->cx - 10) <= 1)
            {
        	    state->dx = -state->dx;
            }
          state->cy += state->dy;
          if ((state->cy + 10) >= state->dimension - 1)
            {
        	    state->dy = -state->dy;
            }
          if ((state->cy - 10) <= 1)
        	  {
              state->dy = -state->dy;
            }
          // g_print ("cx: %d, cy: %d, dx: %d, dy: %d\n", cx, cy, dx, dy);
        }

      // g_print ("%f\n", self->dt);
      f_surface_step (state->fluid, state->dt);
    }

  render (self);

  return G_SOURCE_CONTINUE;
}

static void
tick_destroy_notify (gpointer data)
{
  FluidappWindow *self = (FluidappWindow*) data;

  g_object_unref (self->pixbuf);
  g_free (self->color);
  fluidapp_window_state_free (self->state);
  g_print ("Destroy\n");
}

static void
set_color (GtkColorButton *button,
           gpointer        data)
{
  FluidappWindow *self = (FluidappWindow*) data;
  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), self->color);
}

static void
play_toggle (GtkToggleButton *button)
{
  if (gtk_toggle_button_get_active (button))
    gtk_button_set_icon_name (GTK_BUTTON (button), "media-playback-pause");
  else
    gtk_button_set_icon_name (GTK_BUTTON (button), "media-playback-start");
}

static void
reset_field (GtkButton *button,
             gpointer   user_data)
{
  FluidappWindow *self = (FluidappWindow*) user_data;
  f_surface_clear (self->state->fluid);
}

static gboolean
toggle_overdrive (GtkSwitch* sw,
                  gboolean state,
                  gpointer user_data)
{
  FluidappWindow *self = (FluidappWindow*) user_data;

  if (state)
    self->as_color = overdrive_color;
  else
    self->as_color = capped_color;

  return FALSE;
}

static void
time_scale_change (GtkRange* range,
                   gpointer user_data)
{
  FluidappWindow *self = (FluidappWindow*) user_data;
  GtkAdjustment *adjustment = gtk_range_get_adjustment (range);
  self->state->time_factor = gtk_adjustment_get_value (adjustment);
}

static void
vector_scale_change (GtkRange* range,
                     gpointer user_data)
{
  FluidappWindow *self = (FluidappWindow*) user_data;
  GtkAdjustment *adjustment = gtk_range_get_adjustment (range);
  self->state->vector_scale = gtk_adjustment_get_value (adjustment);
}

static gchar*
get_cursor_name_from_direction (const int direction)
{
  switch (direction)
    {
    case 2:
      return "n-resize";
    case 3:
      return "ne-resize";
    case 4:
      return "e-resize";
    case 5:
      return "se-resize";
    case 6:
      return "s-resize";
    case 7:
      return "sw-resize";
    case 8:
      return "w-resize";
    case 9:
      return "nw-resize";
    default:
      return "default";
    }

}

static void
velocity_function_change (GtkComboBoxText* combo_box,
                          gpointer         user_data)
{
  FluidappWindow *self = (FluidappWindow*) user_data;
  const gchar *selection = gtk_combo_box_get_active_id (GTK_COMBO_BOX (combo_box));
  char *ptr;
  self->state->velocity_function_selector = strtol (selection, &ptr, 10);
  if (self->state->velocity_function_selector < F_VELOCITY_DIRECTIONAL_FN)
    {
      VelocityFunction fn = get_velocity_function (self->state->velocity_function_selector);
      if (fn != NULL && self->state->velocity_function != fn)
        {
          self->state->velocity_function = fn;
          g_object_unref (self->cursor);
          self->cursor = gdk_cursor_new_from_name (get_cursor_name_from_direction (self->state->velocity_function_selector), NULL);
          gtk_widget_set_cursor (GTK_WIDGET (self->scene), self->cursor);
        }
    }
}

static void
on_save_response (GtkNativeDialog *native,
                  int        response,
                  gpointer   user_data)
{
  FluidappWindow *self = user_data;

  if (response == GTK_RESPONSE_ACCEPT)
    {
      GtkFileChooser *chooser = GTK_FILE_CHOOSER (native);

      g_autoptr(GFile) file = gtk_file_chooser_get_file (chooser);

      GError *error = NULL;

      GFileOutputStream *out = g_file_create (file, G_FILE_CREATE_NONE, NULL, &error);
      if (error == NULL)
        {
          // save_to_file (file);
          const gboolean success = gdk_pixbuf_save_to_stream (self->pixbuf,
                                                              G_OUTPUT_STREAM (out),
                                                              "jpeg", NULL, &error, "quality", "100", NULL);
          if (!success && error != NULL)
            {
              g_print ("%s\n", error->message);
              g_error_free (error);
            }
        }
      g_object_unref (G_OBJECT (out));
    }

  g_object_unref (native);
}

static void
create_save_dialog (GtkButton *button,
                    gpointer   user_data)
{
  FluidappWindow *self = (FluidappWindow*) user_data;

  gtk_toggle_button_set_active (self->play_button, FALSE);

  GtkFileChooserNative *native;
  GtkFileChooser *chooser;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;

  native = gtk_file_chooser_native_new ("Save File",
                                        GTK_WINDOW (&(self->parent_instance)),
                                        action,
                                        "_Save",
                                        "_Cancel");
  chooser = GTK_FILE_CHOOSER (native);

  gtk_file_chooser_set_current_name (chooser, "untitled.jpg");

  g_signal_connect (native,
                    "response",
                    G_CALLBACK (on_save_response),
                    self);

  gtk_native_dialog_show (GTK_NATIVE_DIALOG (native));
}

static gboolean
handle_scroll (GtkEventControllerScroll* scroll,
               gdouble                   dx,
               gdouble                   dy,
               gpointer                  user_data)
{
  FluidappWindow *self = (FluidappWindow*) user_data;
  if (self->state->velocity_function_selector >= F_VELOCITY_DIRECTIONAL_FN)
    {
      int direction = self->state->velocity_function_selector
                        - F_VELOCITY_DIRECTIONAL_FN + (int)dy;
      if (direction < 0)
        direction = 8 + direction;
      self->state->velocity_function_selector = direction % 8
                                                  + F_VELOCITY_DIRECTIONAL_FN;
      self->state->velocity_function = get_velocity_function (self->state->velocity_function_selector);
      g_object_unref (self->cursor);
      self->cursor = gdk_cursor_new_from_name (get_cursor_name_from_direction (self->state->velocity_function_selector), NULL);
      gtk_widget_set_cursor (GTK_WIDGET (self->scene), self->cursor);
    }
  return TRUE;
}

static void
fluidapp_window_init (FluidappWindow *self)
{
  /* Initializing the random number generator */
  time_t t;
  srand((unsigned) time(&t));

  self->color = g_malloc (sizeof (GdkRGBA));
  gdk_rgba_parse (self->color, "rgba(255,255,255,1.0)");

  self->as_color = capped_color;

  gtk_widget_init_template (GTK_WIDGET (self));
  self->state  = fluidapp_window_state_create ();
  self->pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                                 TRUE,
                                 BITS_PER_SAMPLE,
                                 self->state->dimension,
                                 self->state->dimension);
  gdk_pixbuf_fill (self->pixbuf, FILL_COLOR);

  gtk_picture_set_pixbuf (self->scene, self->pixbuf);

  self->cursor = gdk_cursor_new_from_name ("default", NULL);
  gtk_widget_set_cursor (GTK_WIDGET (self->scene), self->cursor);

  gtk_widget_add_tick_callback (GTK_WIDGET (self->scene),
                                tick,
                                self,
                                tick_destroy_notify);

  GtkEventController* scroll = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL
    | GTK_EVENT_CONTROLLER_SCROLL_DISCRETE);
  GtkGesture* drag = gtk_gesture_drag_new();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (drag), 0);
  gtk_widget_add_controller (GTK_WIDGET (self->scene),
                             GTK_EVENT_CONTROLLER (drag));
  gtk_widget_add_controller (GTK_WIDGET (self->scene), scroll);

  gtk_combo_box_set_active_id (GTK_COMBO_BOX (self->velocity_function), "1");

  g_signal_connect (self->color_button,
                    "color-set",
                    G_CALLBACK (set_color),
                    self);
  g_signal_connect (self->play_button,
                    "toggled",
                    G_CALLBACK (play_toggle),
                    self);
  g_signal_connect (self->reset_button,
                    "clicked",
                    G_CALLBACK (reset_field),
                    self);
  g_signal_connect (self->save_button,
                    "clicked",
                    G_CALLBACK (create_save_dialog),
                    self);
  g_signal_connect (self->overdrive,
                    "state-set",
                    G_CALLBACK (toggle_overdrive),
                    self);
  g_signal_connect (self->time_scale,
                    "value-changed",
                    G_CALLBACK (time_scale_change),
                    self);
  g_signal_connect (self->vector_scale,
                    "value-changed",
                    G_CALLBACK (vector_scale_change),
                    self);
  g_signal_connect (self->velocity_function,
                    "changed",
                    G_CALLBACK (velocity_function_change),
                    self);
  g_signal_connect (scroll,
                    "scroll",
                    G_CALLBACK (handle_scroll),
                    self);
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
