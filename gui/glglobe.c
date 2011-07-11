/*
    This file is part of Ionowatch.

    (c) 2011 Gonzalo J. Carracedo <BatchDrake@gmail.com>
    
    Ionowatch is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ionowatch is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ionowatch.h>
#include <wbmp.h>
#include <GL/gl.h>
#include <GL/glut.h>

#include "gui.h"


#define GR_PI 3.14159265358979323846264338327950288419716939937510

#define	TEXTURE_WIDTH 2048
#define TEXTURE_HEIGHT 1024

GLuint texture_sample [TEXTURE_WIDTH * TEXTURE_HEIGHT];
int texture_done;
gboolean draw_grid_option = FALSE;
gboolean draw_station_option = FALSE;
gboolean shade_option = FALSE;

#ifdef GL_VERSION_1_1
static GLuint texName;
#endif

GtkWidget *drawing_area_global;
extern time_t selected_time;
extern struct datasource_magnitude* selected_magnitude;
extern struct datasource *selected_datasource;
extern struct station_info *selected_station;
extern struct painter *selected_painter;
static struct globe_data *globe_data;
extern time_t selected_time;

float last_x, last_y;
float x_alpha, y_alpha;
float zoom = 5.0;

int key_down;

inline void 
normalize (GLdouble *v)
{
  GLdouble len = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  v[0] /= len;
  v[1] /= len;
  v[2] /= len;
}

void
init_globe_texture (void)
{
  if (texture_done)
    return;
    
  glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

  glGenTextures (1, &texName);
  glBindTexture (GL_TEXTURE_2D, texName);

  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0,
    GL_BGRA_EXT, GL_UNSIGNED_BYTE, texture_sample);
    
  texture_done++;
}

void 
draw_circle (float xc, float yc, float z, float rad)
{
  int angle;
  float angle_radians, x, y;
  
  glBegin (GL_LINE_LOOP);

  
  for (angle = 0; angle < 365; angle = angle + 5)
  {
    angle_radians = angle * GR_PI / 180.0;
    x = xc + rad * cos (angle_radians);
    y = yc + rad * sin (angle_radians);
    
    glVertex3f (x, z, y);
  }
  
  glEnd ();
}

void
gl_globe_rotate_to (float lat, float lon)
{
  glRotatef (lon, 0.0, 1.0, 0.0);
  glRotatef (-lat, 1.0, 0.0, 0.0);
}

void
gl_printf (float x, float y, char *format,...)
{
  va_list args;
  char buffer[200], *p;

  va_start (args, format);
  vsprintf (buffer, format, args);
  va_end (args);
  glPushMatrix ();
  glTranslatef (x, y, 0);
  glRotatef (-90.0, 0.0, 1.0, 0.0);
  glScalef (0.0001, 0.0001, 0.0001);
  for (p = buffer; *p; p++)
    glutStrokeCharacter (GLUT_STROKE_ROMAN, *p);
    
  glPopMatrix ();
}


void
gl_out (float x, float y, char *format,...)
{
  va_list args;
  char buffer[200], *p;

  va_start (args, format);
  vsprintf (buffer, format, args);
  va_end (args);
  glPushMatrix ();
  glTranslatef (x, y, 0);
    glScalef (0.5, 0.5, 0.5);
  for (p = buffer; *p; p++)
    glutStrokeCharacter (GLUT_STROKE_ROMAN, *p);
    
  glPopMatrix ();
}

void 
sphere_face (int p_recurse, double p_radius, GLdouble *a, GLdouble *b, GLdouble *c)
{
  GLfloat tx, ty, tx1, ty1;

  if (p_recurse > 1)
  {
    GLdouble d[3] = {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
    GLdouble e[3] = {b[0] + c[0], b[1] + c[1], b[2] + c[2]};
    GLdouble f[3] = {c[0] + a[0], c[1] + a[1], c[2] + a[2]};

    normalize (d);
    normalize (e);
    normalize (f);

    sphere_face (p_recurse - 1, p_radius, a, d, f);
    sphere_face (p_recurse - 1, p_radius, d, b, e);
    sphere_face (p_recurse - 1, p_radius, f, e, c);
    sphere_face (p_recurse - 1, p_radius, f, d, e);
    
    return;
  }

  glBegin (GL_TRIANGLES);
  //glBegin (GL_LINE_LOOP);
  
  tx1 = atan2 (a[0], a[2]) / (2. * GR_PI) + 0.5;
  ty1 = asin (a[1]) / GR_PI + .5;
   
  glTexCoord2f (tx1, ty1);

  glNormal3dv (a);
  glVertex3d (a[0] * p_radius, a[1] * p_radius, a[2] * p_radius);

  tx = atan2 (b[0], b[2]) / (2. * GR_PI) + 0.5;
  ty = asin (b[1]) / GR_PI + .5;

  /* These checks are necessary because some triangles map to both sides of
     the texture map, and the texture is then streched all the way through
     to fit in those coordinates. We want to avoid that behaviour by putting 
     the texture coordinates in the same side the first vertex was placed. */
     
  if (tx < 0.75 && tx1 > 0.75)
   tx += 1.0;  
  else if (tx > 0.75 && tx1 < 0.75)
   tx -= 1.0;

  glTexCoord2f (tx, ty);


  glNormal3dv (b);
  glVertex3d (b[0] * p_radius, b[1] * p_radius, b[2] * p_radius);

  tx = atan2 (c[0], c[2]) / (2. * GR_PI) + 0.5;
  ty = asin (c[1]) / GR_PI + .5;

  if (tx < 0.75 && tx1 > 0.75)
   tx += 1.0;  
  else if (tx > 0.75 && tx1 < 0.75)
   tx -= 1.0;
   
  glTexCoord2f (tx, ty);

  glNormal3dv (c);
  glVertex3d (c[0] * p_radius, c[1] * p_radius, c[2] * p_radius);

  glEnd();
}

void 
draw_globe (double p_radius)
{
  GLdouble a[] = {1, 0, 0};
  GLdouble b[] = {0, 0, -1};
  GLdouble c[] = {-1, 0, 0};
  GLdouble d[] = {0, 0, 1};
  GLdouble e[] = {0, 1, 0};
  GLdouble f[] = {0, -1, 0};

  int recurse = 5;

  glEnable (GL_TEXTURE_2D);
  glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glBindTexture (GL_TEXTURE_2D, texName);

  if (!shade_option)
  {
    glDisable (GL_LIGHTING);
    glColor3f (1.0, 1.0, 1.0);
  }
  else
    glEnable (GL_LIGHTING);
    
  sphere_face (recurse, p_radius, d, a, e);
  sphere_face (recurse, p_radius, a, b, e);
  sphere_face (recurse, p_radius, b, c, e);
  sphere_face (recurse, p_radius, c, d, e);
  sphere_face (recurse, p_radius, a, d, f);
  sphere_face (recurse, p_radius, b, a, f);
  sphere_face (recurse, p_radius, c, b, f);
  sphere_face (recurse, p_radius, d, c, f);

  glDisable (GL_TEXTURE_2D);
}

static gboolean
globe_configure (GtkWidget *da, GdkEventConfigure *event, gpointer user_data)
{ 
	GdkGLContext *glcontext = gtk_widget_get_gl_context (da);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (da);

  enter_callback_context ();
  
	gdk_gl_drawable_gl_begin (gldrawable, glcontext);
	
	init_globe_texture ();
	
	glViewport (0, 0, da->allocation.width, da->allocation.height);
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  gluPerspective (40.0, (GLfloat) da->allocation.width / (GLfloat) da->allocation.height, 1.0, 20.0);
  glMatrixMode (GL_MODELVIEW);
	
	gdk_gl_drawable_gl_end (gldrawable);

  leave_callback_context ();
  
	return TRUE;
}


gboolean
globe_realize (GtkWidget *da, gpointer *unused)
{ 
	GdkGLContext *glcontext = gtk_widget_get_gl_context (da);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (da);

  enter_callback_context ();
  
	gdk_gl_drawable_gl_begin (gldrawable, glcontext);
  
  
  glEnable (GL_LIGHTING);
  glEnable (GL_LIGHT0);
  
  
  glDepthFunc (GL_LESS);
  glEnable (GL_DEPTH_TEST);
  
  gdk_gl_drawable_gl_end (gldrawable);

  leave_callback_context ();
  
}

static inline void
draw_grid (void)
{
  int i;
  
  glPushMatrix ();
  
  glDisable (GL_LIGHTING);
  
  glColor3f (1.0, 0.64, 0.0);
  
  for (i = -90; i <= 90; i += 15)
    draw_circle (0, 0, sin ((float) i / 180.0 * GR_PI), cos ((float) i / 180.0 * GR_PI) + 0.001);
  
  glRotatef (90.0, 0.0, 0.0, 1.0);
  glRotatef (-15.0, 1.0, 0.0, 0.0); /* Greenwich meridian adjust */
  
  for (i = 0; i <= 180; i += 15)
  {
    glRotatef (15.0, 1.0, 0.0, 0.0);
    draw_circle (0, 0, 0, 1.001);
  }
  
  glEnable (GL_LIGHTING);
  
  glPopMatrix ();
}

static inline void
draw_station (int verbose)
{
  glPushMatrix ();
  
  gl_globe_rotate_to (selected_station->lat, selected_station->lon);
  
  glTranslatef (0.0, 0.0, 1.04);
  
  glDisable (GL_LIGHTING);
  
  glColor3f (0.0, 1.0, 1.0);
  
  if (verbose)
    gl_printf (0.05, 0.0, "%lg ºN %lg ºE %s (%s)", 
      selected_station->lat, selected_station->lon,
      selected_station->name, selected_station->name_long);
      
  glRotatef (180.0, 0.0, 1.0, 0.0);
  
  
  
  glutSolidCone (0.015, 0.035, 10, 20);
  
  glEnable (GL_LIGHTING);
  
  
  glPopMatrix ();
}

static inline void
draw_verbose_data (void)
{
  glPushAttrib (GL_ENABLE_BIT);
  glDisable (GL_DEPTH_TEST);
  glDisable (GL_LIGHTING);
  glMatrixMode (GL_PROJECTION);
  glPushMatrix ();
  glLoadIdentity ();
  gluOrtho2D (0, 3000, 0, 3000);
  glMatrixMode (GL_MODELVIEW);
  glPushMatrix ();
  glLoadIdentity ();
  glColor3f (1.0, 0.64, 0.0);
  
  gl_out (80, 2820, "Ionogram for UTC %s", ctime (&selected_time));
  gl_out (80, 2740, "Magnitude select: %s", selected_magnitude->desc);
  gl_out (80, 2660, "Selected station: %s (%s) - %g N %g E", 
    selected_station->name, selected_station->name_long,
    selected_station->lat, selected_station->lon);
  
  gl_out (80, 160, "Datasource select: %s", selected_datasource->desc);
  
  glPopMatrix ();
  glMatrixMode (GL_PROJECTION);
  glPopMatrix ();
  glPopAttrib ();
}

static gboolean
globe_expose (GtkWidget *da, GdkEventExpose *event, gpointer user_data)
{
  GdkGLContext *glcontext = gtk_widget_get_gl_context (da);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (da);
  GLfloat position[] = {0.0, 0.0, 1.0, 0.0};
  int verbose;
     
  enter_callback_context ();
  
  if (ionowatch_data_is_ready ())
    verbose = painter_test_options (selected_painter, PAINTER_OPTION_MASK_VERBOSE);
  
	gdk_gl_drawable_gl_begin (gldrawable, glcontext);
  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  
  glPushMatrix ();
  
  glTranslatef (0.0, 0.0, -zoom);
  
  glRotated ((GLdouble) -x_alpha, 0.0, 1.0, 0.0);
  glRotated ((GLdouble) -y_alpha, 1.0, 0.0, 0.0);
 
  if (shade_option)
    glLightfv (GL_LIGHT0, GL_POSITION, position);

  glRotatef (23.0 * cos (globe_data->sol), 1.0, 0.0, 0.0);
  
  glRotatef ((globe_data->day_off) / GR_PI * 180.0 - 180.0, 0.0, 1.0, 0.0);
  
  if (ionowatch_data_is_ready ())
  {
    glPushMatrix ();
    
    painter_display (selected_painter);
    
    glPopMatrix ();
  }
  
  draw_globe (1.0);
  
  if (ionowatch_data_is_ready ())
    if (draw_station_option)
      draw_station (verbose);
  
  
  if (draw_grid_option)
    draw_grid ();
  
  
  glPopMatrix();

  if (verbose)
    draw_verbose_data ();
  
	if (gdk_gl_drawable_is_double_buffered (gldrawable))
		gdk_gl_drawable_swap_buffers (gldrawable);
	else
		glFlush ();

	gdk_gl_drawable_gl_end (gldrawable);

  leave_callback_context ();
  
	return TRUE;
}

gboolean 
gl_globe_mouse_motion_cb (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  enter_callback_context ();
  
  if (key_down)
  {
    x_alpha += last_x - ((GdkEventMotion *) event)->x;
    last_x = ((GdkEventMotion *) event)->x;
    
    y_alpha += last_y - ((GdkEventMotion *) event)->y;
    last_y = ((GdkEventMotion *) event)->y;
    
    gdk_window_invalidate_rect (widget->window, &widget->allocation, FALSE);
    
  }   
  leave_callback_context ();
  
  return FALSE;
}

void
gl_globe_notify_repaint (void)
{
  if (!GDK_IS_WINDOW (drawing_area_global->window))
    return;
    
  gdk_window_invalidate_rect (drawing_area_global->window, &drawing_area_global->allocation, FALSE);
}

void
gl_globe_set_draw_grid (gboolean setting)
{
  draw_grid_option = setting;
}

void
gl_globe_set_shade (gboolean setting)
{
  shade_option = setting;
}

void
gl_globe_set_draw_station (gboolean setting)
{
  draw_station_option = setting;
}

void
gl_globe_set_time (time_t time)
{
  globe_data_set_time (globe_data, time);
}

gboolean 
gl_globe_mouse_down_cb (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  enter_callback_context ();
  
  if (((GdkEventButton *) event)->button == 1)
  {
    last_x = ((GdkEventButton *) event)->x;
    last_y = ((GdkEventButton *) event)->y;
    
    key_down = 1;
  }
  
  leave_callback_context ();
  
  return FALSE;
}

gboolean 
gl_globe_mouse_up_cb (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  enter_callback_context ();
  
  if (((GdkEventButton *) event)->button == 1)
    key_down = 0;
    
  leave_callback_context ();
  
  return FALSE;
}

gboolean 
gl_globe_mouse_scroll_cb (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  enter_callback_context ();
  
  if (((GdkEventScroll *) event)->direction == GDK_SCROLL_UP)
  {
    zoom /= 1.1;
    gdk_window_invalidate_rect (widget->window, &widget->allocation, FALSE);
  }
  else if (((GdkEventScroll *) event)->direction == GDK_SCROLL_DOWN)
  {
    zoom *= 1.1;
    gdk_window_invalidate_rect (widget->window, &widget->allocation, FALSE);
  }
  
  leave_callback_context ();

}

static int
fill_texture (void)
{
  int i, j;
  struct draw *draw;
  
  char *path;
  
  if ((path = locate_config_file ("world.bmp", CONFIG_READ | CONFIG_GLOBAL)) == NULL)
  {
    ERROR ("Couldn't locate globe texture");
    return -1;
  }
    
  if ((draw = draw_from_bmp (path)) == NULL)
  {
    ERROR ("couldn't locate globe texture at %s", path);
    free (path);
    return -1;
  }
  
  free (path);
  
  if (draw->width != TEXTURE_WIDTH && draw->height != TEXTURE_HEIGHT)
  {
    ERROR ("globe dimensions mismatch, must be %dx%d", 
      TEXTURE_WIDTH, TEXTURE_HEIGHT);
      
    return -1;
  }
  
  for (j = 0; j < TEXTURE_HEIGHT; j++)
    for (i = 0; i < TEXTURE_WIDTH; i++)
      texture_sample[i + j * TEXTURE_WIDTH] = draw_pget (draw, i, TEXTURE_HEIGHT - 1 - j);
      
  draw_free (draw);
  
  return 0;
}

/* TODO: document functions */
int
ionowatch_gl_globe_init (void)
{
  GtkWidget *drawing_area;
  GdkGLConfig *glconfig;

  globe_data = globe_data_new (0, 0, 0, 0);
  
  if (fill_texture () == -1)
    return -1;
  
  drawing_area = ionowatch_fetch_widget ("GLGlobeArea");
  
  gtk_widget_set_events (drawing_area, GDK_POINTER_MOTION_MASK | 
                                       GDK_BUTTON1_MOTION_MASK |
                                       GDK_BUTTON_PRESS_MASK |
                                       GDK_BUTTON_RELEASE_MASK);
  
  glconfig = gdk_gl_config_new_by_mode (
    GDK_GL_MODE_RGB | GDK_GL_MODE_DEPTH | GDK_GL_MODE_DOUBLE);
    
  if (!glconfig)
  {
    ERROR (
      "Coudln't allocate GDK GL config for globe display. This is an OpenGL "
      "issue, check your installation and try again (can't get config)");
    return -1;
  }
  
  if (!gtk_widget_set_gl_capability (drawing_area, glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE))
  {
    ERROR (
      "Couldn't set GDK GL capability to drawing area. This is an OpenGL "
      "issue, check your installation and try again (can't set GL capability)");
    return -1;
  }
  
  /* TODO: connect this via gtkbuilder */
  g_signal_connect (drawing_area, "configure-event",
	  G_CALLBACK (globe_configure), NULL);
  g_signal_connect (drawing_area, "expose-event",
	  G_CALLBACK (globe_expose), NULL);
	g_signal_connect (drawing_area, "realize",
	  G_CALLBACK (globe_realize), NULL);
	
	drawing_area_global = drawing_area;
	
  gl_globe_set_time (time (NULL));
  
	return 0;
}


