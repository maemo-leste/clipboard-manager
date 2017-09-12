/*
 * Copyright © 2004 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Red Hat not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Red Hat makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * RED HAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL RED HAT
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Matthias Clasen, Red Hat, Inc.
 */

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <glib.h>

#include "clipboard-manager.h"
#include "xsettings-manager.h"
#include "gconf2xsettings.h"

typedef struct
  {
  GSource source ;
  Display *display ;
  GPollFD event_poll_fd ;
  } X11EventSource ;

void
terminate_cb (void *data)
{
  int *terminated = data;
  
  if (!*terminated)
    {
      *terminated = True;
    }
}

static int error_code;

static void
error_trap_push (void)
{
  error_code = 0;

}

static int
error_trap_pop (void)
{
  return error_code;
}

static int
x_error (Display *display,
	 XErrorEvent *error)
{
  error_code = error->error_code;
  return 0; /* ignored */
}

static gboolean x11_event_prepare (GSource *source, gint *p_timeout)
  {
  (*p_timeout) = -1 ;

  return XPending (((X11EventSource *)source)->display) ;
  }

static gboolean x11_event_check (GSource *source)
  {
  X11EventSource *x11_event_source = (X11EventSource *)source ;
  return (x11_event_source->event_poll_fd.revents & G_IO_IN) ||
          XPending (x11_event_source->display) ;
  }

static gboolean x11_event_dispatch (GSource *source, GSourceFunc callback, gpointer user_data)
  {
  if (NULL != callback)
    (*callback) (user_data) ;
  return TRUE ;
  }

static GSourceFuncs source_funcs =
  {
  .prepare  = x11_event_prepare,
  .check    = x11_event_check,
  .dispatch = x11_event_dispatch,
  NULL
  } ;

static GSource *
x11_event_source_new (Display *display)
  {
  X11EventSource *x11_event_source = (X11EventSource *)g_source_new (&source_funcs, sizeof (X11EventSource)) ;
  x11_event_source->display = display ;
  x11_event_source->event_poll_fd.fd = ConnectionNumber (display) ;
  x11_event_source->event_poll_fd.events = G_IO_IN ;
  g_source_add_poll ((GSource *)x11_event_source, &x11_event_source->event_poll_fd) ;

  return (GSource *)x11_event_source ;
  }

typedef struct
  {
  Display *display ;
  XSettingsManager *xsettings_manager ;
  ClipboardManager *clipboard_manager ;
  } PROCESS_X11_EVENT_PARAMS ;

static gboolean process_x11_event (PROCESS_X11_EVENT_PARAMS *params)
  {
  XEvent event ;

  XNextEvent (params->display, &event) ;
  clipboard_manager_process_event (params->clipboard_manager, &event) ;
  xsettings_manager_process_event (params->xsettings_manager, &event) ;

  return TRUE ;
  }

int 
main (int argc, char *argv[])
{
  ClipboardManager *manager;
  int terminated = False;
  Display *display;
  GSource *source = NULL ;
  PROCESS_X11_EVENT_PARAMS params = {NULL, NULL, NULL} ;
  guint source_id = 0 ;
  XSettingsManager *xsettings_manager = NULL ;

  display = XOpenDisplay (NULL);

  if (!display)
    {
      fprintf (stderr, "Could not open display. Is the DISPLAY environment variable set?\n");
      exit (1);
    }

  if (clipboard_manager_check_running (display))
    {
      fprintf (stderr, "You can only run one clipboard manager at a time; exiting\n");
      exit (1);
    }

  if (NULL == (xsettings_manager = xsettings_manager_new (display, DefaultScreen (display), terminate_cb,  &terminated)))
    {
    fprintf (stderr, "Failed to create an XSettings manager; exiting\n") ;
    exit (1) ;
    }

  construct_gconf_to_xsettings_bridge (xsettings_manager) ;

  XSetErrorHandler (x_error);
  manager = clipboard_manager_new (display,
				   error_trap_push, error_trap_pop,
				   terminate_cb, &terminated);
  if (!manager)
    {
      fprintf (stderr, "Could not create clipboard manager!\n");
      exit (1);
    }

  params.display = display ;
  params.clipboard_manager = manager ;
  params.xsettings_manager = xsettings_manager ;

  if (NULL != (source = x11_event_source_new (display)))
    {
    g_source_set_callback (source, (GSourceFunc)process_x11_event, &params, NULL) ;
    source_id = g_source_attach (source, NULL) ;
    }

  while (!terminated)
    g_main_context_iteration (NULL, TRUE) ;

  clipboard_manager_destroy (manager);

  if (0 != source_id)
    g_source_remove (source_id) ;

  return 0;
}
