/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2018 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include "dfu-firmware.h"

#include "fu-progressbar.h"
#include "fu-wac-device.h"

typedef struct {
	FuQuirks		*quirks;
	GPtrArray		*cmd_array;
	FuProgressbar		*progressbar;
} FuWacToolPrivate;

static void
fu_wac_tool_private_free (FuWacToolPrivate *priv)
{
	if (priv == NULL)
		return;
	g_object_unref (priv->quirks);
	g_object_unref (priv->progressbar);
	if (priv->cmd_array != NULL)
		g_ptr_array_unref (priv->cmd_array);
	g_free (priv);
}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
G_DEFINE_AUTOPTR_CLEANUP_FUNC(FuWacToolPrivate, fu_wac_tool_private_free)
#pragma clang diagnostic pop

typedef gboolean (*FuWacToolPrivateCb)	(FuWacToolPrivate	*util,
					 gchar			**values,
					 GError			**error);

typedef struct {
	gchar			*name;
	gchar			*arguments;
	gchar			*description;
	FuWacToolPrivateCb	 callback;
} FuWacToolItem;

static void
fu_wac_tool_item_free (FuWacToolItem *item)
{
	g_free (item->name);
	g_free (item->arguments);
	g_free (item->description);
	g_free (item);
}

static gint
fu_wac_tool_sort_command_name_cb (FuWacToolItem **item1, FuWacToolItem **item2)
{
	return g_strcmp0 ((*item1)->name, (*item2)->name);
}

static void
fu_wac_tool_add (GPtrArray *array,
		 const gchar *name,
		 const gchar *arguments,
		 const gchar *description,
		 FuWacToolPrivateCb callback)
{
	g_auto(GStrv) names = NULL;

	g_return_if_fail (name != NULL);
	g_return_if_fail (description != NULL);
	g_return_if_fail (callback != NULL);

	/* add each one */
	names = g_strsplit (name, ",", -1);
	for (guint i = 0; names[i] != NULL; i++) {
		FuWacToolItem *item = g_new0 (FuWacToolItem, 1);
		item->name = g_strdup (names[i]);
		if (i == 0) {
			item->description = g_strdup (description);
		} else {
			item->description = g_strdup_printf ("Alias to %s", names[0]);
		}
		item->arguments = g_strdup (arguments);
		item->callback = callback;
		g_ptr_array_add (array, item);
	}
}

static gchar *
fu_wac_tool_get_descriptions (GPtrArray *array)
{
	const gsize max_len = 31;
	GString *str;

	/* print each command */
	str = g_string_new ("");
	for (guint i = 0; i < array->len; i++) {
		FuWacToolItem *item = g_ptr_array_index (array, i);
		gsize len;
		g_string_append (str, "  ");
		g_string_append (str, item->name);
		len = strlen (item->name) + 2;
		if (item->arguments != NULL) {
			g_string_append (str, " ");
			g_string_append (str, item->arguments);
			len += strlen (item->arguments) + 1;
		}
		if (len < max_len) {
			for (guint j = len; j < max_len + 1; j++)
				g_string_append_c (str, ' ');
			g_string_append (str, item->description);
			g_string_append_c (str, '\n');
		} else {
			g_string_append_c (str, '\n');
			for (guint j = 0; j < max_len + 1; j++)
				g_string_append_c (str, ' ');
			g_string_append (str, item->description);
			g_string_append_c (str, '\n');
		}
	}

	/* remove trailing newline */
	if (str->len > 0)
		g_string_set_size (str, str->len - 1);

	return g_string_free (str, FALSE);
}

static gboolean
fu_wac_tool_run (FuWacToolPrivate *priv, const gchar *command, gchar **values, GError **error)
{
	/* find command */
	for (guint i = 0; i < priv->cmd_array->len; i++) {
		FuWacToolItem *item = g_ptr_array_index (priv->cmd_array, i);
		if (g_strcmp0 (item->name, command) == 0)
			return item->callback (priv, values, error);
	}

	/* not found */
	g_set_error_literal (error,
			     G_IO_ERROR,
			     G_IO_ERROR_NOT_FOUND,
			     "Command not found");
	return FALSE;
}

static FuWacDevice *
fu_wac_get_default_device (FuWacToolPrivate *priv, GError **error)
{
	g_autoptr(GUsbContext) usb_context = NULL;
	g_autoptr(GPtrArray) devices = NULL;

	/* get all the DFU devices */
	usb_context = g_usb_context_new (error);
	if (usb_context == NULL)
		return NULL;
	devices = g_usb_context_get_devices (usb_context);
	for (guint i = 0; i < devices->len; i++) {
		GUsbDevice *usb_device = g_ptr_array_index (devices, i);
		g_autoptr(FuWacDevice) device = fu_wac_device_new (usb_device);
		fu_device_set_quirks (FU_DEVICE (device), priv->quirks);
		if (fu_usb_device_probe (FU_USB_DEVICE (device), NULL))
			return g_steal_pointer (&device);
	}

	/* unsupported */
	g_set_error_literal (error,
			     G_IO_ERROR,
			     G_IO_ERROR_NOT_SUPPORTED,
			     "no supported devices found");
	return NULL;
}

static gboolean
fu_wac_tool_info (FuWacToolPrivate *priv, gchar **values, GError **error)
{
	g_autoptr(FuWacDevice) device = fu_wac_get_default_device (priv, error);
	g_autofree gchar *str = NULL;
	if (device == NULL)
		return FALSE;
	if (!fu_usb_device_open (FU_USB_DEVICE (device), error))
		return FALSE;
	str = fu_device_to_string (FU_DEVICE (device));
	g_print ("%s", str);
	return TRUE;
}

static void
fu_wac_tool_progress_cb (FuDevice *device, GParamSpec *pspec, FuWacToolPrivate *priv)
{
	fu_progressbar_update (priv->progressbar,
			       fu_device_get_status (device),
			       fu_device_get_progress (device));
}

static gboolean
fu_wac_tool_write (FuWacToolPrivate *priv, gchar **values, GError **error)
{
	g_autoptr(FuWacDevice) device = NULL;
	g_autoptr(GBytes) blob = NULL;

	/* check args */
	if (g_strv_length (values) != 1) {
		g_set_error_literal (error,
				     G_IO_ERROR,
				     G_IO_ERROR_FAILED,
				     "Invalid arguments, expected "
				     "FILENAME -- e.g. `firmware.hex`");
		return FALSE;
	}

	/* get device */
	device = fu_wac_get_default_device (priv, error);
	if (device == NULL)
		return FALSE;

	/* load firmware file */
	blob = fu_common_get_contents_bytes (values[0], error);
	if (blob == NULL)
		return FALSE;

	/* write new firmware */
	if (!fu_usb_device_open (FU_USB_DEVICE (device), error))
		return FALSE;
	g_signal_connect (device, "notify::status",
			  G_CALLBACK (fu_wac_tool_progress_cb), priv);
	g_signal_connect (device, "notify::progress",
			  G_CALLBACK (fu_wac_tool_progress_cb), priv);
	return fu_device_write_firmware (FU_DEVICE (device), blob, error);
}

int
main (int argc, char **argv)
{
	gboolean verbose = FALSE;
	g_autofree gchar *cmd_descriptions = NULL;
	g_autoptr(GError) error = NULL;
	g_autoptr(GOptionContext) context = NULL;
	g_autoptr(FuWacToolPrivate) priv = g_new0 (FuWacToolPrivate, 1);
	const GOptionEntry options[] = {
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
			"Print verbose debug statements", NULL },
		{ NULL}
	};
	setlocale (LC_ALL, "");

	priv->progressbar = fu_progressbar_new ();
	fu_progressbar_set_length_percentage (priv->progressbar, 50);
	fu_progressbar_set_length_status (priv->progressbar, 20);

	/* add commands */
	priv->cmd_array = g_ptr_array_new_with_free_func ((GDestroyNotify) fu_wac_tool_item_free);
	fu_wac_tool_add (priv->cmd_array,
			 "info", NULL,
			 "Show information about the device",
			 fu_wac_tool_info);
	fu_wac_tool_add (priv->cmd_array,
			 "write", "FILENAME",
			 "Update the firmware",
			 fu_wac_tool_write);

	/* sort by command name */
	g_ptr_array_sort (priv->cmd_array,
			  (GCompareFunc) fu_wac_tool_sort_command_name_cb);

	/* get a list of the commands */
	context = g_option_context_new (NULL);
	cmd_descriptions = fu_wac_tool_get_descriptions (priv->cmd_array);
	g_option_context_set_summary (context, cmd_descriptions);
	g_set_application_name ("Wacom Debug Tool");
	g_option_context_add_main_entries (context, options, NULL);
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_print ("%s: %s\n", "Failed to parse arguments", error->message);
		return EXIT_FAILURE;
	}

	/* set verbose? */
	if (verbose)
		g_setenv ("G_MESSAGES_DEBUG", "all", FALSE);

	/* use quirks */
	priv->quirks = fu_quirks_new ();
	if (!fu_quirks_load (priv->quirks, &error)) {
		g_print ("Failed to load quirks: %s\n", error->message);
		return EXIT_FAILURE;
	}

	/* run the specified command */
	if (!fu_wac_tool_run (priv, argv[1], (gchar**) &argv[2], &error)) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND)) {
			g_autofree gchar *tmp = NULL;
			tmp = g_option_context_get_help (context, TRUE, NULL);
			g_print ("%s\n\n%s", error->message, tmp);
		} else {
			g_print ("%s\n", error->message);
		}
		return EXIT_FAILURE;
	}

	return 0;
}
