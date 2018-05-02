/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2018 Richard Hughes <richard@hughsie.com>
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

#include "fu-wac-module.h"
#include "fu-wac-common.h"

#include "dfu-common.h"
#include "dfu-chunked.h"
#include "dfu-firmware.h"

#define FU_WAC_MODLE_STATUS_OK				0
#define FU_WAC_MODLE_STATUS_BUSY			1
#define FU_WAC_MODLE_STATUS_ERR_CRC			2
#define FU_WAC_MODLE_STATUS_CMD				3
#define FU_WAC_MODLE_STATUS_INTFLASH_TYPE		4
#define FU_WAC_MODLE_STATUS_VERIFY_TYPE			5
#define FU_WAC_MODLE_STATUS_CHECKMODE			6
#define FU_WAC_MODLE_STATUS_GETMPUTYPE			7
#define FU_WAC_MODLE_STATUS_GETBLAVER			8
#define FU_WAC_MODLE_STATUS_ALLERASE			9
#define FU_WAC_MODLE_STATUS_WRITE			10
#define FU_WAC_MODLE_STATUS_EXIT			11
#define FU_WAC_MODLE_STATUS_ERR				12
#define FU_WAC_MODLE_STATUS_INVALID_OP			13
#define FU_WAC_MODLE_STATUS_WRONG_IMAGE			14

typedef struct {
	guint8			 fw_type;
	guint8			 command;
	guint8			 status;
} FuWacModulePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (FuWacModule, fu_wac_module, FU_TYPE_USB_DEVICE)
#define GET_PRIVATE(o) (fu_wac_module_get_instance_private (o))

enum {
	PROP_0,
	PROP_FW_TYPE,
	PROP_LAST
};

static const gchar *
fu_wac_module_fw_type_to_string (guint8 fw_type)
{
	if (fw_type == FU_WAC_MODULE_FW_TYPE_TOUCH)
		return "touch";
	if (fw_type == FU_WAC_MODULE_FW_TYPE_BLUETOOTH)
		return "bluetooth";
	if (fw_type == FU_WAC_MODULE_FW_TYPE_EMR_CORRECTION)
		return "emr-correction";
	if (fw_type == FU_WAC_MODULE_FW_TYPE_BLUETOOTH_HID)
		return "bluetooth-hid";
	return NULL;
}

static const gchar *
fu_wac_module_command_to_string (guint8 command)
{
	if (command == FU_WAC_MODULE_COMMAND_START)
		return "start";
	if (command == FU_WAC_MODULE_COMMAND_DATA)
		return "data";
	if (command == FU_WAC_MODULE_COMMAND_END)
		return "end";
	return NULL;
}

static const gchar *
fu_wac_module_status_to_string (guint8 status)
{
	if (status == FU_WAC_MODLE_STATUS_OK)
		return "ok";
	if (status == FU_WAC_MODLE_STATUS_BUSY)
		return "busy";
	if (status == FU_WAC_MODLE_STATUS_ERR_CRC)
		return "err-crc";
	if (status == FU_WAC_MODLE_STATUS_CMD)
		return "cmd";
	if (status == FU_WAC_MODLE_STATUS_INTFLASH_TYPE)
		return "intflash-type";
	if (status == FU_WAC_MODLE_STATUS_VERIFY_TYPE)
		return "verify-type";
	if (status == FU_WAC_MODLE_STATUS_CHECKMODE)
		return "checkmode";
	if (status == FU_WAC_MODLE_STATUS_GETMPUTYPE)
		return "getmputype";
	if (status == FU_WAC_MODLE_STATUS_GETBLAVER)
		return "getblaver";
	if (status == FU_WAC_MODLE_STATUS_ALLERASE)
		return "allerase";
	if (status == FU_WAC_MODLE_STATUS_WRITE)
		return "write";
	if (status == FU_WAC_MODLE_STATUS_EXIT)
		return "exit";
	if (status == FU_WAC_MODLE_STATUS_ERR)
		return "err";
	if (status == FU_WAC_MODLE_STATUS_INVALID_OP)
		return "invalid-op";
	if (status == FU_WAC_MODLE_STATUS_WRONG_IMAGE)
		return "wrong-image";
	return NULL;
}

static void
fu_wac_module_set_fw_type (FuWacModule *self, guint8 fw_type)
{
	FuWacModulePrivate *priv = GET_PRIVATE (self);
	GUsbDevice *usb_device = fu_usb_device_get_dev (FU_USB_DEVICE (self));
	g_autofree gchar *devid = NULL;

	/* usually set in a constructor */
	priv->fw_type = fw_type;

	/* append the firmware kind to the generated GUID */
	devid = g_strdup_printf ("USB\\VID_%04X&PID_%04X-%s",
				 g_usb_device_get_vid (usb_device),
				 g_usb_device_get_pid (usb_device),
				 fu_wac_module_fw_type_to_string (fw_type));
	fu_device_add_guid (FU_DEVICE (self), devid);
}

static void
fu_wac_module_to_string (FuDevice *device, GString *str)
{
	FuWacModule *self = FU_WAC_MODULE (device);
	FuWacModulePrivate *priv = GET_PRIVATE (self);
	g_string_append (str, "  DfuWacSubModule:\n");
	g_string_append_printf (str, "    fw-type:\t\t%s\n",
				fu_wac_module_fw_type_to_string (priv->fw_type));
	g_string_append_printf (str, "    status:\t\t%s\n",
				fu_wac_module_status_to_string (priv->status));
	g_string_append_printf (str, "    command:\t\t%s\n",
				fu_wac_module_command_to_string (priv->command));
}

static gboolean
fu_wac_module_refresh (FuWacModule *self, GError **error)
{
	FuWacModulePrivate *priv = GET_PRIVATE (self);
	guint8 buf[FU_WAC_PACKET_LEN];

	//FIXME GET from hardware
	memset (buf, 0xff, sizeof(buf));

	/* check ReportID */
	if (buf[0] != FU_WAC_REPORT_ID_MODULE) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_INTERNAL,
			     "Submodule GetFeature ReportID invalid "
			     "got 0x%0x2", (guint) buf[0]);
		return FALSE;
	}

	/* check fw type */
	if (priv->fw_type != buf[1]) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_INTERNAL,
			     "Submodule GetFeature fw_Type invalid "
			     "got 0x%02x expected 0x%02x",
			     (guint) buf[0], (guint) priv->fw_type);
		return FALSE;
	}

	/* current phase */
	priv->command = buf[2];
	g_debug ("command: %s", fu_wac_module_command_to_string (priv->command));

	/* current status */
	priv->status = buf[3];
	g_debug ("status: %s", fu_wac_module_status_to_string (priv->status));

	/* success */
	return TRUE;
}

gboolean
fu_wac_module_set_feature (FuWacModule *self,
			   guint8 command,
			   GBytes *blob, /* optional */
			   GError **error)
{
	FuWacModulePrivate *priv = GET_PRIVATE (self);
	guint8 buf[FU_WAC_PACKET_LEN];
	const guint8 *data;
	gsize len = 0;
	guint busy_poll_loops = 100; /* 1s */

	/* verify the size of the blob */
	if (blob != NULL) {
		data = g_bytes_get_data (blob, &len);
		if (len > 509) {
			g_set_error (error,
				     FWUPD_ERROR,
				     FWUPD_ERROR_INTERNAL,
				     "Submodule SetFeature blob larger than "
				     "buffer %" G_GSIZE_FORMAT, len);
			return FALSE;
		}
	}

	/* build packet */
	memset (buf, 0xff, sizeof(buf));
	buf[0] = FU_WAC_REPORT_ID_MODULE;
	buf[1] = priv->fw_type;
	buf[2] = command;
	if (len > 0)
		memcpy (&buf[3], data, len);

	/* tell the daemon the current status */
	switch (command) {
	case FU_WAC_MODULE_COMMAND_START:
		fu_device_set_status (FU_DEVICE (self), FWUPD_STATUS_DEVICE_ERASE);
		break;
	case FU_WAC_MODULE_COMMAND_DATA:
		fu_device_set_status (FU_DEVICE (self), FWUPD_STATUS_DEVICE_WRITE);
		break;
	case FU_WAC_MODULE_COMMAND_END:
		fu_device_set_status (FU_DEVICE (self), FWUPD_STATUS_DEVICE_VERIFY);
		break;
	default:
		break;
	}

	//FIXME, send to hardware

	/* special case StartProgram, as it can take much longer as it is
	 * erasing the blocks (15s) */
	if (command == FU_WAC_MODULE_COMMAND_START)
		busy_poll_loops *= 15;

	/* wait for hardware */
	for (guint i = 0; i < busy_poll_loops; i++) {
		if (!fu_wac_module_refresh (self, error))
			return FALSE;
		if (priv->status == FU_WAC_MODLE_STATUS_BUSY) {
			g_usleep (10000); /* 10ms */
			continue;
		}
		if (priv->status == FU_WAC_MODLE_STATUS_OK)
			return TRUE;
	}

	/* the hardware never responded */
	g_set_error (error,
		     FWUPD_ERROR,
		     FWUPD_ERROR_INTERNAL,
		     "Failed to SetFeature: %s",
		     fu_wac_module_status_to_string (priv->status));
	return FALSE;
}

static void
fu_wac_module_get_property (GObject *object, guint prop_id,
			    GValue *value, GParamSpec *pspec)
{
	FuWacModule *self = FU_WAC_MODULE (object);
	FuWacModulePrivate *priv = GET_PRIVATE (self);
	switch (prop_id) {
	case PROP_FW_TYPE:
		g_value_set_uint (value, priv->fw_type);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
fu_wac_module_set_property (GObject *object, guint prop_id,
			    const GValue *value, GParamSpec *pspec)
{
	FuWacModule *self = FU_WAC_MODULE (object);
	switch (prop_id) {
	case PROP_FW_TYPE:
		fu_wac_module_set_fw_type (self, g_value_get_uint (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
fu_wac_module_init (FuWacModule *self)
{
}

static void
fu_wac_module_finalize (GObject *object)
{
//	FuWacModule *self = FU_WAC_MODULE (object);
	G_OBJECT_CLASS (fu_wac_module_parent_class)->finalize (object);
}

static void
fu_wac_module_class_init (FuWacModuleClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GParamSpec *pspec;
	FuDeviceClass *klass_device = FU_DEVICE_CLASS (klass);

	/* properties */
	pspec = g_param_spec_uint ("fw-type", NULL, NULL,
				   0, G_MAXUINT, 0,
				   G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	g_object_class_install_property (object_class, PROP_FW_TYPE, pspec);

	object_class->get_property = fu_wac_module_get_property;
	object_class->set_property = fu_wac_module_set_property;
	object_class->finalize = fu_wac_module_finalize;
	klass_device->to_string = fu_wac_module_to_string;
}

FuWacModule *
fu_wac_module_new (GUsbDevice *usb_device)
{
	FuWacModule *module = NULL;
	module = g_object_new (FU_TYPE_WAC_MODULE,
			       "fw-type", FU_WAC_MODULE_FW_TYPE_BLUETOOTH,
			       "usb-device", usb_device,
			       NULL);
	return module;
}
