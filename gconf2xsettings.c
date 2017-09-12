#include <X11/Xlib.h>
#include <stdlib.h>
#include <string.h>
#include <gconf/gconf-client.h>
#include <glib.h>
#include "xsettings-manager.h"

#define CONF_DIR PACKAGE_SYSCONFDIR "/gconf2xsettings.d"

typedef struct
  {
  XSettingsManager *xsettings_manager ;
  char *xsettings_key ;
  } GCONF_KEY_VALUE_CHANGED_PARAMS ;

typedef struct
  {
  XSettingsManager *xsettings_manager ;
  GConfClient *gconf_client ;
  } HT_GCONF_KEYS_ADD_NOTIFY_PARAMS ;

static void gconf_key_value_changed (GConfClient *gconf_client, guint notify_id, GConfEntry *entry, GCONF_KEY_VALUE_CHANGED_PARAMS *params)
  {
  XSettingsResult result = XSETTINGS_FAILED ;

  if (entry->value != NULL)
    switch (entry->value->type)
      {
      case GCONF_VALUE_INT:
        {
        gint the_int = gconf_value_get_int (entry->value) ;
        result = xsettings_manager_set_int (params->xsettings_manager, params->xsettings_key, the_int) ;
        break ;
        }
      case GCONF_VALUE_BOOL:
        {
        gboolean bool = gconf_value_get_bool (entry->value) ;
        result = xsettings_manager_set_int (params->xsettings_manager, params->xsettings_key, bool) ;
        break ;
        }
      case GCONF_VALUE_STRING:
        {
        const char *str = gconf_value_get_string (entry->value) ;
        result = xsettings_manager_set_string (params->xsettings_manager, params->xsettings_key, str) ;
        break ;
        }
      default:
        break ;
      }

  if (XSETTINGS_SUCCESS == result)
    xsettings_manager_notify (params->xsettings_manager) ;
  }

static void ht_gconf_keys_add_notify (char *gconf_key, char *xsettings_key, HT_GCONF_KEYS_ADD_NOTIFY_PARAMS *params)
  {
  guint id = 0;
  GError *error = NULL ;
  GCONF_KEY_VALUE_CHANGED_PARAMS *cb_params = NULL ;

  if (NULL == (cb_params = g_malloc0 (sizeof (GCONF_KEY_VALUE_CHANGED_PARAMS)))) return ;

  cb_params->xsettings_key = xsettings_key ;
  cb_params->xsettings_manager = params->xsettings_manager ;

  if ((id = gconf_client_notify_add (params->gconf_client, gconf_key, (GConfClientNotifyFunc)gconf_key_value_changed, cb_params, (GDestroyNotify)g_free, &error)) != 0)
    {
    GConfEntry *entry = NULL;

    if ((entry = gconf_client_get_entry(params->gconf_client, gconf_key, NULL, TRUE, NULL)) != NULL)
      {
      gconf_key_value_changed (params->gconf_client, id, entry, cb_params) ;
      }
    }

  if (NULL != error)
    {
    g_warning ("ht_gconf_keys_add_notify: Adding notify: %s --> %s failed: %s\n", gconf_key, xsettings_key, error->message) ;
    g_error_free (error) ;
    error = NULL ;
    }
  }

static void maybe_add_path (char *gconf_key, GArray *ar_gconf_paths)
  {
  int Nix ;
  gsize path_len = 0 ;
  char *last_path_sep = NULL ;
  char *gconf_path = NULL ;

  if (NULL == (last_path_sep = g_strrstr (gconf_key, "/")))
    {
    g_warning ("maybe_add_path: GConf key \"%s\" contains no \"/\"\n", gconf_key) ;
    return ;
    }

  path_len = last_path_sep - gconf_key ;

  for (Nix = 0 ; Nix < ar_gconf_paths->len ; Nix++)
    if (!strncmp (g_array_index (ar_gconf_paths, char *, Nix), gconf_key, path_len))
      return ;

  gconf_path = g_strndup (gconf_key, path_len) ;
  g_array_append_val (ar_gconf_paths, gconf_path) ;
  }

static void construct_bridges (XSettingsManager *xsettings_manager, GHashTable *ht_gconf_keys, GArray *ar_gconf_paths)
  {
  GError *error = NULL ;
  int Nix ;
  HT_GCONF_KEYS_ADD_NOTIFY_PARAMS params = {xsettings_manager, NULL} ;

  g_type_init () ;

  if (NULL == (params.gconf_client = gconf_client_get_default ())) return ;

  for (Nix = 0 ; Nix < ar_gconf_paths->len ; Nix++)
    {
    gconf_client_add_dir (params.gconf_client, g_array_index (ar_gconf_paths, char *, Nix), GCONF_CLIENT_PRELOAD_NONE, &error) ;
    if (NULL != error)
      {
      g_warning ("construct_bridges: Adding directory %s failed: %s\n", g_array_index (ar_gconf_paths, char *, Nix), error->message) ;
      g_error_free (error) ;
      error = NULL ;
      }
    }

  g_hash_table_foreach (ht_gconf_keys, (GHFunc)ht_gconf_keys_add_notify, &params) ;
  }

void construct_gconf_to_xsettings_bridge (XSettingsManager *xsettings_manager)
  {
  GIOChannel *ioc = NULL ;
  gboolean invalid_conf_line = TRUE ;
  GDir *dir = NULL ;
  const char *conf_file = NULL ;
  char *conf_line = NULL, *space_location = NULL, *conf_file_abs_name = NULL ;
  gsize term_pos = 0, conf_line_length = 0 ;
  GArray *ar_gconf_paths = NULL ;
  GHashTable *ht_gconf_keys = NULL ;

  if (NULL == (dir = g_dir_open (CONF_DIR, 0, NULL))) return ;
  if (NULL == (ht_gconf_keys = g_hash_table_new (g_str_hash, g_str_equal))) return ;

  if (NULL != (ar_gconf_paths = g_array_new (FALSE, TRUE, sizeof (char *))))
    {
    while (NULL != (conf_file = g_dir_read_name (dir)))
      if (NULL != (conf_file_abs_name = g_strdup_printf ("%s%s%s", CONF_DIR, G_DIR_SEPARATOR_S, conf_file)))
        {
        if (NULL != (ioc = g_io_channel_new_file (conf_file_abs_name, "r", NULL)))
          {
          while (G_IO_STATUS_NORMAL == g_io_channel_read_line (ioc, &conf_line, &conf_line_length, &term_pos, NULL))
            if (NULL != conf_line)
              {
              invalid_conf_line = TRUE ;
              conf_line[term_pos] = 0 ;
              if (NULL != (space_location = g_strstr_len (conf_line, conf_line_length, " ")))
                if (*(space_location + 1) != 0)
                  {
                  *space_location = 0 ;
                  if (NULL == (g_hash_table_lookup (ht_gconf_keys, conf_line)))
                    {
                    g_hash_table_insert (ht_gconf_keys, g_strdup (conf_line), g_strdup (space_location + 1)) ;
                    maybe_add_path (conf_line, ar_gconf_paths) ;
                    }
                  else
                    g_warning ("construct_gconf_to_xsettings_bridge: %s --> %s is already added\n", conf_line, space_location + 1) ;
                  invalid_conf_line = FALSE ;
                  }
              if (invalid_conf_line)
                g_warning ("construct_gconf_to_xsettings_bridge: Not a valid config line: \"%s\"\n", conf_line) ;
              g_free (conf_line) ;
              }
          g_io_channel_close (ioc) ;
          }
        g_free (conf_file_abs_name) ;
        }
    g_dir_close (dir) ;
    }

  construct_bridges (xsettings_manager, ht_gconf_keys, ar_gconf_paths) ;
  }
