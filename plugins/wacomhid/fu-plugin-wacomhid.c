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

#include "fu-plugin.h"
#include "fu-plugin-vfuncs.h"

#include "fu-wac-device.h"

gboolean
fu_plugin_usb_device_added (FuPlugin *plugin, GUsbDevice *usb_device, GError **error)
{
	g_autoptr(FuWacDevice) device = NULL;
	g_autoptr(FuDeviceLocker) locker = NULL;
	device = fu_wac_device_new (usb_device);
	fu_device_set_quirks (FU_DEVICE (device), fu_plugin_get_quirks (plugin));
	locker = fu_device_locker_new (device, error);
	if (locker == NULL)
		return FALSE;
	fu_plugin_device_add (plugin, FU_DEVICE (device));
	return TRUE;
}

gboolean
fu_plugin_update (FuPlugin *plugin, FuDevice *device, GBytes *blob_fw,
		  FwupdInstallFlags flags, GError **error)
{
	g_autoptr(FuDeviceLocker) locker = NULL;
	locker = fu_device_locker_new (device, error);
	if (locker == NULL)
		return FALSE;
	return fu_device_write_firmware (device, blob_fw, error);
}
