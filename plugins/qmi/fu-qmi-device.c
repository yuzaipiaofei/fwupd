/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2017-2018 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "config.h"

#include <string.h>

#include "fu-qmi-device.h"

struct _FuQmiDevice
{
	FuUsbDevice		 parent_instance;
	FuQmiDeviceQuirks	 quirks;
};

G_DEFINE_TYPE (FuQmiDevice, fu_qmi_device, FU_TYPE_USB_DEVICE)

static void
fu_qmi_device_to_string (FuDevice *device, GString *str)
{
	FuQmiDevice *self = FU_QMI_DEVICE (device);
	g_string_append (str, "  DfuQmiDevice:\n");
//	g_string_append_printf (str, "    timeout:\t\t%" G_GUINT32_FORMAT "\n", self->dnload_timeout);
}

void
fu_qmi_device_set_quirks (FuQmiDevice *self, FuQmiDeviceQuirks quirks)
{
	self->quirks = quirks;
}

gboolean
fu_qmi_device_download (FuQmiDevice *self, GBytes *blob, GError **error)
{
	return TRUE;
}

static gboolean
fu_qmi_device_probe (FuUsbDevice *device, GError **error)
{
	const gchar *quirk_str;

	/* devices have to be whitelisted */
	quirk_str = fu_device_get_plugin_hints (FU_DEVICE (device));
	if (quirk_str == NULL) {
		g_set_error_literal (error,
				     FWUPD_ERROR,
				     FWUPD_ERROR_NOT_SUPPORTED,
				     "not supported with this device");
		return FALSE;
	}
	if (g_strcmp0 (quirk_str, "require-delay") == 0) {
		fu_qmi_device_set_quirks (FU_QMI_DEVICE (device),
					  FU_QMI_DEVICE_QUIRK_REQUIRE_DELAY);
	}

	/* hardcoded */
	fu_device_add_flag (FU_DEVICE (device), FWUPD_DEVICE_FLAG_UPDATABLE);

	/* success */
	return TRUE;
}

static gboolean
fu_qmi_device_open (FuUsbDevice *device, GError **error)
{
	FuQmiDevice *self = FU_QMI_DEVICE (device);
	GUsbDevice *usb_device = fu_usb_device_get_dev (device);

	/* open device and clear status */
	if (!g_usb_device_claim_interface (usb_device, 0x00, /* HID */
					   G_USB_DEVICE_CLAIM_INTERFACE_BIND_KERNEL_DRIVER,
					   error)) {
		g_prefix_error (error, "failed to claim HID interface: ");
		return FALSE;
	}

	/* success */
	return TRUE;
}

static gboolean
fu_qmi_device_close (FuUsbDevice *device, GError **error)
{
	GUsbDevice *usb_device = fu_usb_device_get_dev (device);

	/* we're done here */
	if (!g_usb_device_release_interface (usb_device, 0x00, /* HID */
					     G_USB_DEVICE_CLAIM_INTERFACE_BIND_KERNEL_DRIVER,
					     error)) {
		g_prefix_error (error, "failed to release interface: ");
		return FALSE;
	}

	/* success */
	return TRUE;
}

static void
fu_qmi_device_init (FuQmiDevice *device)
{
}

static void
fu_qmi_device_class_init (FuQmiDeviceClass *klass)
{
	FuDeviceClass *klass_device = FU_DEVICE_CLASS (klass);
	FuUsbDeviceClass *klass_usb_device = FU_USB_DEVICE_CLASS (klass);
	klass_device->to_string = fu_qmi_device_to_string;
	klass_usb_device->open = fu_qmi_device_open;
	klass_usb_device->close = fu_qmi_device_close;
	klass_usb_device->probe = fu_qmi_device_probe;
}

FuQmiDevice *
fu_qmi_device_new (GUsbDevice *usb_device)
{
	FuQmiDevice *device = NULL;
	device = g_object_new (FU_TYPE_QMI_DEVICE,
			       "usb-device", usb_device,
			       NULL);
	return device;
}
