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

#include "fu-wac-device.h"
#include "fu-wac-common.h"
#include "fu-wac-module-bluetooth.h"
#include "fu-wac-module-touch.h"

#include "dfu-chunked.h"
#include "dfu-common.h"
#include "dfu-firmware.h"

typedef struct __attribute__((packed)) {
	guint32		 start_addr;
	guint32		 block_sz;
	guint16		 write_sz; /* bit 15 is write protection flag */
} FuWacFlashDescriptor;

typedef enum {
	FU_WAC_STATUS_UNKNOWN			= 0,
	FU_WAC_STATUS_WRITING			= 1 << 0,
	FU_WAC_STATUS_ERASING			= 1 << 1,
	FU_WAC_STATUS_ERROR_WRITE		= 1 << 2,
	FU_WAC_STATUS_ERROR_ERASE		= 1 << 3,
	FU_WAC_STATUS_WRITE_PROTECTED		= 1 << 4,
	FU_WAC_STATUS_LAST
} FuWacStatus;

#define FU_WAC_DEVICE_TIMEOUT			5000	/* ms */

struct _FuWacDevice
{
	FuUsbDevice		 parent_instance;
	GPtrArray		*flash_descriptors;
	GArray			*checksums;
	guint32			 status_word;
	guint16			 firmware_index;
	guint16			 loader_ver;
	guint16			 read_data_sz;
	guint16			 write_word_sz;
	guint16			 write_block_sz;	/* usb transfer size */
	guint16			 nr_flash_blocks;
	guint16			 configuration;
};

G_DEFINE_TYPE (FuWacDevice, fu_wac_device, FU_TYPE_USB_DEVICE)

static GString *
fu_wac_device_status_to_string (guint32 status_word)
{
	GString *str = g_string_new (NULL);
	if (status_word & FU_WAC_STATUS_WRITING)
		g_string_append (str, "writing,");
	if (status_word & FU_WAC_STATUS_ERASING)
		g_string_append (str, "erasing,");
	if (status_word & FU_WAC_STATUS_ERROR_WRITE)
		g_string_append (str, "error-write,");
	if (status_word & FU_WAC_STATUS_ERROR_ERASE)
		g_string_append (str, "error-erase,");
	if (status_word & FU_WAC_STATUS_WRITE_PROTECTED)
		g_string_append (str, "write-protected,");
	if (str->len == 0) {
		g_string_append (str, "none");
		return str;
	}
	g_string_truncate (str, str->len - 1);
	return str;
}

static void
fu_wac_device_to_string (FuDevice *device, GString *str)
{
	GPtrArray *children;
	FuWacDevice *self = FU_WAC_DEVICE (device);
	g_autoptr(GString) status_str = NULL;

	g_string_append (str, "  DfuWacDevice:\n");
	for (guint i = 0; i < self->flash_descriptors->len; i++) {
		FuWacFlashDescriptor *fd = g_ptr_array_index (self->flash_descriptors, i);
		g_string_append_printf (str, "    flash-descriptor-%02u:\n", i);
		g_string_append_printf (str, "      start-addr:\t0x%08x\n", (guint) fd->start_addr);
		g_string_append_printf (str, "      block-sz:\t0x%08x\n", (guint) fd->block_sz);
		g_string_append_printf (str, "      write-sz:\t0x%04x\n", (guint) fd->write_sz);
	}
	status_str = fu_wac_device_status_to_string (self->status_word);
	g_string_append_printf (str, "    status:\t\t%s\n", status_str->str);

	/* print children also */
	children = fu_device_get_children (device);
	for (guint i = 0; i < children->len; i++) {
		FuDevice *child = g_ptr_array_index (children, i);
		g_autofree gchar *tmp = fu_device_to_string (FU_DEVICE (child));
		g_string_append (str, "  DfuWacDeviceChild:\n");
		g_string_append (str, tmp);
	}
}

static void
fu_wac_device_dump (const gchar *title, const guint8 *buf, gsize sz)
{
	if (g_getenv ("FWUPD_WAC_VERBOSE") == NULL)
		return;
	g_print ("%s (%" G_GSIZE_FORMAT "):\n", title, sz);
	for (gsize i = 0; i < sz; i++)
		g_print ("%02x ", buf[i]);
	g_print ("\n");
}

static gboolean
fu_wac_device_get_feature_report (FuWacDevice *self, guint8 command, guint8 *buf, GError **error)
{
	GUsbDevice *usb_device = fu_usb_device_get_dev (FU_USB_DEVICE (self));
	gsize sz = 0;

	memset (buf, 0xff, FU_WAC_PACKET_LEN);
	if (!g_usb_device_control_transfer (usb_device,
					    G_USB_DEVICE_DIRECTION_DEVICE_TO_HOST,
					    G_USB_DEVICE_REQUEST_TYPE_CLASS,
					    G_USB_DEVICE_RECIPIENT_INTERFACE,
					    HID_REPORT_GET,		/* bRequest */
					    HID_FEATURE | command,	/* wValue */
					    0x0000,			/* wIndex */
					    buf, FU_WAC_PACKET_LEN, &sz,
					    FU_WAC_DEVICE_TIMEOUT,
					    NULL, error)) {
		g_prefix_error (error, "Failed to GetFlashDescriptor: ");
		return FALSE;
	}

	/* check packet */
	if (sz != FU_WAC_PACKET_LEN) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_INTERNAL,
			     "Packet get bytes %" G_GSIZE_FORMAT " expected %i",
			     sz, FU_WAC_PACKET_LEN);
		return FALSE;
	}
	if (buf[0] != command) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_INTERNAL,
			     "command response was %i expected %i",
			     buf[0], command);
		return FALSE;
	}
	return TRUE;
}

static gboolean
fu_wac_device_set_feature_report (FuWacDevice *self, const guint8 *buf, GError **error)
{
	GUsbDevice *usb_device = fu_usb_device_get_dev (FU_USB_DEVICE (self));
	gsize sz = 0;

	/* FIXME: check wValue */
	if (!g_usb_device_control_transfer (usb_device,
					    G_USB_DEVICE_DIRECTION_HOST_TO_DEVICE,
					    G_USB_DEVICE_REQUEST_TYPE_CLASS,
					    G_USB_DEVICE_RECIPIENT_INTERFACE,
					    HID_REPORT_SET,				/* bRequest */
					    HID_FEATURE | FU_WAC_REPORT_ID_CONTROL,	/* wValue */
					    0x0000,					/* wIndex */
					    buf, FU_WAC_PACKET_LEN, &sz,
					    FU_WAC_DEVICE_TIMEOUT, /* timeout */
					    NULL, error)) {
		g_prefix_error (error, "Failed to EraseBlock: ");
		return FALSE;
	}

	/* check packet */
	if (sz != FU_WAC_PACKET_LEN) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_INTERNAL,
			     "Packet sent bytes %" G_GSIZE_FORMAT " expected %i",
			     sz, FU_WAC_PACKET_LEN);
		return FALSE;
	}
	return TRUE;
}

static gboolean
fu_wac_device_ensure_flash_descriptors (FuWacDevice *self, GError **error)
{
	gsize sz = 0;
	guint8 buf[FU_WAC_PACKET_LEN];

	/* already done */
	if (self->flash_descriptors->len > 0)
		return TRUE;

	/* hit hardware */
	if (!fu_wac_device_get_feature_report (self, FU_WAC_REPORT_ID_GET_FLASH_DESCRIPTOR, buf, error))
		return FALSE;
	fu_wac_device_dump ("GetFlashDescriptor", buf, sz);

	/* check packet */
#if 0
	if (sz < 1 + sizeof(FuWacFlashDescriptor)) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_INTERNAL,
			     "GetFlashDescriptor packet was too small %" G_GSIZE_FORMAT
			     " expected >= %" G_GSIZE_FORMAT,
			     sz, 1 + sizeof(FuWacFlashDescriptor));
		return FALSE;
	}
	if (sz - 1 % sizeof(FuWacFlashDescriptor) != 0) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_INTERNAL,
			     "GetFlashDescriptor packet size %" G_GSIZE_FORMAT
			     " expected multiple of %" G_GSIZE_FORMAT,
			     sz, sizeof(FuWacFlashDescriptor));
		return FALSE;
	}
#endif

	/* parse */
	for (guint i = 1; i < self->nr_flash_blocks; i++) {
		FuWacFlashDescriptor *fd = g_new0 (FuWacFlashDescriptor, 1);
		fd->start_addr = fu_common_read_uint32 (buf + (i * sizeof(FuWacFlashDescriptor)) + 1, G_BIG_ENDIAN);
		fd->block_sz = fu_common_read_uint32 (buf + (i * sizeof(FuWacFlashDescriptor)) + 5, G_BIG_ENDIAN);
		fd->write_sz = fu_common_read_uint16 (buf + (i * sizeof(FuWacFlashDescriptor)) + 9, G_BIG_ENDIAN);
		g_ptr_array_add (self->flash_descriptors, fd);
	}
	g_debug ("added %u flash descriptors", self->flash_descriptors->len);
	return TRUE;
}

static gboolean
fu_wac_device_ensure_status (FuWacDevice *self, GError **error)
{
	gsize sz = 0;
	guint8 buf[FU_WAC_PACKET_LEN];
	g_autoptr(GString) str = NULL;

	/* hit hardware */
	if (!fu_wac_device_get_feature_report (self, FU_WAC_REPORT_ID_GET_STATUS, buf, error))
		return FALSE;
	fu_wac_device_dump ("GetStatus", buf, sz);

#if 0
	if (sz != 5) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_INTERNAL,
			     "GetStatus packet was invalid, got %" G_GSIZE_FORMAT
			     " expected %i",
			     sz, 5);
		return FALSE;
	}
#endif

	/* parse */
	self->status_word = fu_common_read_uint32 (buf + 1, G_BIG_ENDIAN);
	str = fu_wac_device_status_to_string (self->status_word);
	g_debug ("status now: %s", str->str);
	return TRUE;
}

static gboolean
fu_wac_device_ensure_checksums (FuWacDevice *self, GError **error)
{
	gsize sz = 0;
	guint8 buf[FU_WAC_PACKET_LEN];
	guint32 updater_version;

	/* hit hardware */
	if (!fu_wac_device_get_feature_report (self, FU_WAC_REPORT_ID_GET_CHECKSUMS, buf, error))
		return FALSE;
	fu_wac_device_dump ("GetChecksums", buf, sz);

	/* parse */
	updater_version = fu_common_read_uint32 (buf + 1, G_BIG_ENDIAN);
	g_debug ("updater-version: %" G_GUINT32_FORMAT, updater_version);

	/* get block checksums */
	g_array_set_size (self->checksums, 0);
	for (guint i = 0; i < self->nr_flash_blocks; i++) {
		guint32 csum = fu_common_read_uint32 (buf + 3 + (i * 4), G_BIG_ENDIAN);
		g_array_append_val (self->checksums, csum);
	}
	g_debug ("added %u checksums", self->flash_descriptors->len);

	return TRUE;
}


static gboolean
fu_wac_device_ensure_firmware_index (FuWacDevice *self, GError **error)
{
	gsize sz = 0;
	guint8 buf[FU_WAC_PACKET_LEN];

	/* FIXME: hit hardware */
	memset (buf, 0xff, sizeof(buf));

	/* check packet */
	if (buf[0] != FU_WAC_REPORT_ID_GET_CURRENT_FIRMWARE_IDX) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_INTERNAL,
			     "GetCurrentFirmwareIndex packet-id was %i expected %i",
			     buf[0], FU_WAC_REPORT_ID_GET_CURRENT_FIRMWARE_IDX);
		return FALSE;
	}
	if (sz != 3) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_INTERNAL,
			     "GetChecksums packet was invalid, got %" G_GSIZE_FORMAT
			     " expected %i",
			     sz, 3);
		return FALSE;
	}

	/* parse */
	self->firmware_index = fu_common_read_uint16 (buf + 1, G_BIG_ENDIAN);
	return TRUE;
}

static gboolean
fu_wac_device_ensure_parameters (FuWacDevice *self, GError **error)
{
	gsize sz = 0;
	guint8 buf[FU_WAC_PACKET_LEN];

	/* FIXME: hit hardware */
	memset (buf, 0xff, sizeof(buf));

	/* check packet */
	if (buf[0] != FU_WAC_REPORT_ID_GET_PARAMETERS) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_INTERNAL,
			     "GetParameters packet-id was %i expected %i",
			     buf[0], FU_WAC_REPORT_ID_GET_CURRENT_FIRMWARE_IDX);
		return FALSE;
	}
	if (sz != 13) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_INTERNAL,
			     "GetChecksums packet was invalid, got %" G_GSIZE_FORMAT
			     " expected %i",
			     sz, 13);
		return FALSE;
	}

	/* parse */
	self->loader_ver = fu_common_read_uint16 (buf + 1, G_BIG_ENDIAN);
	self->read_data_sz = fu_common_read_uint16 (buf + 3, G_BIG_ENDIAN);
	self->write_word_sz = fu_common_read_uint16 (buf + 5, G_BIG_ENDIAN);
	self->write_block_sz = fu_common_read_uint16 (buf + 7, G_BIG_ENDIAN);
	self->nr_flash_blocks = fu_common_read_uint16 (buf + 9, G_BIG_ENDIAN);
	self->configuration = fu_common_read_uint16 (buf + 11, G_BIG_ENDIAN);

	/* debug */
	g_debug ("loader-ver: %" G_GUINT16_FORMAT, self->loader_ver);
	g_debug ("read-data-sz: %" G_GUINT16_FORMAT, self->read_data_sz);
	g_debug ("write-word-sz: %" G_GUINT16_FORMAT, self->write_word_sz);
	g_debug ("write-block-sz: %" G_GUINT16_FORMAT, self->write_block_sz);
	g_debug ("nr-flash-blocks: %" G_GUINT16_FORMAT, self->nr_flash_blocks);
	g_debug ("configuration: %" G_GUINT16_FORMAT, self->configuration);
	return TRUE;
}

static gboolean
fu_wac_device_write_block (FuWacDevice *self, guint32 addr, GBytes *blob, GError **error)
{
	const guint8 *tmp;
	gsize sz = 0;
	guint8 buf[FU_WAC_PACKET_LEN];

	/* check size: FIXME -- segment? */
	tmp = g_bytes_get_data (blob, &sz);
	if (sz > FU_WAC_PACKET_LEN - 5) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_INTERNAL,
			     "Packet was too large at %" G_GSIZE_FORMAT " bytes",
			     sz);
		return FALSE;
	}

	/* build packet */
	memset (buf, 0xff, sizeof(buf));
	buf[0] = FU_WAC_REPORT_ID_WRITE_BLOCK;
	fu_common_write_uint32 (buf + 1, addr, G_BIG_ENDIAN);
	if (sz > 0)
		memcpy (buf + 5, tmp, sz);

	/* hit hardware */
	fu_wac_device_dump ("WriteBlock", buf, sz + 5);
	return fu_wac_device_set_feature_report (self, buf, error);
}

static gboolean
fu_wac_device_erase_block (FuWacDevice *self, guint32 addr, GError **error)
{
	guint8 buf[FU_WAC_PACKET_LEN];

	/* build packet */
	memset (buf, 0xff, sizeof(buf));
	buf[0] = FU_WAC_REPORT_ID_ERASE_BLOCK;
	fu_common_write_uint32 (buf + 1, addr, G_BIG_ENDIAN);

	/* hit hardware */
	fu_wac_device_dump ("EraseBlock", buf, 5);
	return fu_wac_device_set_feature_report (self, buf, error);
}

gboolean
fu_wac_device_update_reset (FuWacDevice *self, GError **error)
{
	guint8 buf[FU_WAC_PACKET_LEN];

	/* build packet */
	memset (buf, 0xff, sizeof(buf));
	buf[0] = FU_WAC_REPORT_ID_UPDATE_RESET;

	/* hit hardware */
	fu_wac_device_dump ("UpdateReset", buf, 5);
	return fu_wac_device_set_feature_report (self, buf, error);
}

static gboolean
fu_wac_device_set_checksum_of_block (FuWacDevice *self, guint16 block_nr, guint32 checksum, GError **error)
{
	guint8 buf[FU_WAC_PACKET_LEN];

	/* build packet */
	memset (buf, 0xff, sizeof(buf));
	buf[0] = FU_WAC_REPORT_ID_SET_CHECKSUM_FOR_BLOCK;
	fu_common_write_uint16 (buf + 1, block_nr, G_BIG_ENDIAN);
	fu_common_write_uint32 (buf + 3, checksum, G_BIG_ENDIAN);

	/* hit hardware */
	fu_wac_device_dump ("SetChecksumOfBlock", buf, 7);
	return fu_wac_device_set_feature_report (self, buf, error);
}

static gboolean
fu_wac_device_calculate_checksum_of_block (FuWacDevice *self, guint16 block_nr, GError **error)
{
	guint8 buf[FU_WAC_PACKET_LEN];

	/* build packet */
	memset (buf, 0xff, sizeof(buf));
	buf[0] = FU_WAC_REPORT_ID_CALCULATE_CHECKSUM_FOR_BLOCK;
	fu_common_write_uint16 (buf + 1, block_nr, G_BIG_ENDIAN);

	/* hit hardware */
	fu_wac_device_dump ("CalculateChecksumOfBlock", buf, 3);
	return fu_wac_device_set_feature_report (self, buf, error);
}

static gboolean
fu_wac_device_write_checksum_table (FuWacDevice *self, GError **error)
{
	guint8 buf[FU_WAC_PACKET_LEN];

	/* build packet */
	memset (buf, 0xff, sizeof(buf));
	buf[0] = FU_WAC_REPORT_ID_WRITE_CHECKSUM_TABLE;

	/* hit hardware */
	fu_wac_device_dump ("WriteChecksumTable", buf, 5);
	return fu_wac_device_set_feature_report (self, buf, error);
}

static gboolean
fu_wac_device_switch_to_flash_loader (FuWacDevice *self, GError **error)
{
	guint8 buf[FU_WAC_PACKET_LEN];

	/* build packet */
	memset (buf, 0xff, sizeof(buf));
	buf[0] = FU_WAC_REPORT_ID_SWITCH_TO_FLASH_LOADER;
	buf[1] = 0x05;
	buf[2] = 0x6a;

	/* hit hardware */
	fu_wac_device_dump ("SwitchToFlashLoader", buf, 3);
	return fu_wac_device_set_feature_report (self, buf, error);
}

static gboolean
fu_wac_device_quit_and_reset (FuWacDevice *self, GError **error)
{
	guint8 buf[FU_WAC_PACKET_LEN];

	/* build packet */
	memset (buf, 0xff, sizeof(buf));
	buf[0] = FU_WAC_REPORT_ID_QUIT_AND_RESET;
	buf[1] = 0x05;
	buf[2] = 0x6a;

	/* hit hardware */
	fu_wac_device_dump ("QuitAndReset", buf, 3);
	return fu_wac_device_set_feature_report (self, buf, error);
}

static gboolean
fu_wav_device_flash_descriptor_is_wp (const FuWacFlashDescriptor *fd)
{
	return fd->write_sz & 8000;
}

static GBytes *
fu_wac_device_get_bytes_for_addr (DfuElement *element, guint32 addr, guint32 sz, GError **error)
{
	guint32 offset;
	GBytes *blob;

	if (addr < dfu_element_get_address (element)) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_INTERNAL,
			     "requested address 0x%x less than base address 0x%x",
			     (guint) addr, (guint) dfu_element_get_address (element));
		return NULL;
	}

	/* offset into data */
	offset = addr - dfu_element_get_address (element);
	blob = dfu_element_get_contents (element);
	if (sz > g_bytes_get_size (blob) - offset) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_INTERNAL,
			     "requested size with offset 0x%x not available",
			     (guint) offset);
		return NULL;
	}

	/* check chunk */
	return g_bytes_new_from_bytes (blob, offset, sz);
}

static gboolean
fu_wac_device_write_firmware (FuDevice *device, GBytes *blob, GError **error)
{
	DfuElement *element;
	DfuImage *image;
	FuWacDevice *self = FU_WAC_DEVICE (device);
	guint16 firmware_index_old;
	g_autoptr(DfuFirmware) firmware = dfu_firmware_new ();
	g_autofree guint32 *csum_local = NULL;

	//FIXME when to call this?
	if (!fu_wac_device_ensure_status (self, error))
		return FALSE;

	/* load .wac file, including metadata */
	if (!dfu_firmware_parse_data (firmware, blob,
				      DFU_FIRMWARE_PARSE_FLAG_NONE,
				      error))
		return FALSE;
	if (dfu_firmware_get_format (firmware) != DFU_FIRMWARE_FORMAT_WAC) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_INTERNAL,
			     "expected firmware format is 'wac', got '%s'",
			     dfu_firmware_format_to_string (dfu_firmware_get_format (firmware)));
		return FALSE;
	}

	/* enter flash mode */
	if (!fu_wac_device_switch_to_flash_loader (self, error))
		return FALSE;

	/* get current selected device */
	if (!fu_wac_device_ensure_firmware_index (self, error))
		return FALSE;
	firmware_index_old = self->firmware_index;

	/* use the correct image from the firmware */
	image = dfu_firmware_get_image (firmware, self->firmware_index == 0 ? 0 : 1);
	if (image == NULL) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_INTERNAL,
			     "no firmware image for index %" G_GUINT16_FORMAT,
			     self->firmware_index);
		return FALSE;
	}
	element = dfu_image_get_element_default (image);
	if (element == NULL) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_INTERNAL,
			     "no element in image %" G_GUINT16_FORMAT,
			     self->firmware_index);
		return FALSE;
	}
	g_debug ("using element at addr 0x%0x",
		 (guint) dfu_element_get_address (element));

	/* get firmware parameters (page sz and transfer sz) */
	if (!fu_wac_device_ensure_parameters (self, error))
		return FALSE;

	/* get the current flash descriptors */
	if (!fu_wac_device_ensure_flash_descriptors (self, error))
		return FALSE;

	/* get the updater protocol version */
	if (!fu_wac_device_ensure_checksums (self, error))
		return FALSE;

	/* clear all checksums of pages */
	for (guint16 i = 0; i < self->flash_descriptors->len; i++) {
		FuWacFlashDescriptor *fd = g_ptr_array_index (self->flash_descriptors, i);
		if (fu_wav_device_flash_descriptor_is_wp (fd))
			continue;
		//FIXME: only for fw blobs we have?
		if (!fu_wac_device_set_checksum_of_block (self, i, 0x0, error))
			return FALSE;
	}

	/* write the data into the flash page */
	csum_local = g_new0 (guint32, self->flash_descriptors->len);
	for (guint16 i = 0; i < self->flash_descriptors->len; i++) {
		FuWacFlashDescriptor *fd = g_ptr_array_index (self->flash_descriptors, i);
		g_autoptr(GBytes) blob_block = NULL;
		g_autoptr(GPtrArray) chunks = NULL;

		/* if page is protected */
		if (fu_wav_device_flash_descriptor_is_wp (fd)) {
			g_debug ("page %u at 0x%x is WP, skipping", i, fd->start_addr);
			continue;
		}

		/* get data for page */
		blob_block = fu_wac_device_get_bytes_for_addr (element,
							       fd->start_addr,
							       fd->block_sz,
							       error);
		if (blob_block == NULL)
			return FALSE;

		/* erase entire block */
		if (!fu_wac_device_erase_block (self, i, error))
			return FALSE;

		/* write block in chunks */
		chunks = dfu_chunked_new_from_bytes (blob_block,
						     fd->start_addr,
						     0, /* page_sz */
						     fd->write_sz);
		for (guint j = 0; j < chunks->len; j++) {
			DfuChunkedPacket *pkt = g_ptr_array_index (chunks, j);
			g_autoptr(GBytes) blob_chunk = g_bytes_new (pkt->data, pkt->data_sz);
			if (!fu_wac_device_write_block (self, pkt->address, blob_chunk, error))
				return FALSE;
		}

		/* calculate expected checksum and save to device RAM */
		csum_local[i] = fu_wac_calculate_checksum32be_bytes (blob_block);
		if (!fu_wac_device_set_checksum_of_block (self, i, csum_local[i], error))
			return FALSE;
	}

	/* calculate CRC inside device */
	for (guint16 i = 0; i < self->flash_descriptors->len; i++) {
		if (!fu_wac_device_calculate_checksum_of_block (self, i, error))
			return FALSE;
	}

	/* read all CRC of all pages and verify with local CRC */
	if (!fu_wac_device_ensure_checksums (self, error))
		return FALSE;
	for (guint16 i = 0; i < self->checksums->len; i++) {
		guint32 csum_rom = g_array_index (self->checksums, guint32, i);
		if (csum_rom != csum_local[i]) {
			g_set_error (error,
				     FWUPD_ERROR,
				     FWUPD_ERROR_INTERNAL,
				     "failed local checksum at block %u", i);
			return FALSE;
		}
	}

	/* store host CRC into flash */
	if (!fu_wac_device_write_checksum_table (self, error))
		return FALSE;

	/* reboot, which switches the boot index of the firmware */
	if (!fu_wac_device_update_reset (self, error))
		return FALSE;

	/* FIXME */
	if (0 && !fu_wac_device_quit_and_reset (self, error))
		return FALSE;

	/* verify the current device is different from when selected */
	if (!fu_wac_device_ensure_firmware_index (self, error))
		return FALSE;
	if (firmware_index_old == self->firmware_index) {
		g_set_error_literal (error,
				     FWUPD_ERROR,
				     FWUPD_ERROR_INTERNAL,
				     "expected firmware index to change");
		return FALSE;
	}

	return TRUE;
}

static gboolean
fu_wac_device_probe (FuUsbDevice *device, GError **error)
{
	/* devices have to be whitelisted */
	if (fu_device_get_plugin_hints (FU_DEVICE (device)) == NULL) {
		g_set_error_literal (error,
				     FWUPD_ERROR,
				     FWUPD_ERROR_NOT_SUPPORTED,
				     "not supported with this device");
		return FALSE;
	}

	/* hardcoded */
	fu_device_add_icon (FU_DEVICE (device), "input-tablet");
	fu_device_add_flag (FU_DEVICE (device), FWUPD_DEVICE_FLAG_UPDATABLE);
	return TRUE;
}

static gboolean
fu_wac_device_open (FuUsbDevice *device, GError **error)
{
	FuWacDevice *self = FU_WAC_DEVICE (device);
	GUsbDevice *usb_device = fu_usb_device_get_dev (device);
	gsize sz = 0;
	guint8 buf[FU_WAC_PACKET_LEN];
	g_autoptr(GString) str = g_string_new (NULL);
	g_autofree gchar *version_bootloader = NULL;

	/* open device */
	if (!g_usb_device_claim_interface (usb_device, 0x00, /* HID */
					   G_USB_DEVICE_CLAIM_INTERFACE_BIND_KERNEL_DRIVER,
					   error)) {
		g_prefix_error (error, "failed to claim HID interface: ");
		return FALSE;
	}

	/* get firmware parameters (page sz and transfer sz) */
	if (!fu_wac_device_ensure_parameters (self, error))
		return FALSE;

	/* get the current flash descriptors */
	if (!fu_wac_device_ensure_flash_descriptors (self, error))
		return FALSE;

	/* get version of each sub-module */
	if (!fu_wac_device_get_feature_report (self, FU_WAC_REPORT_ID_FW_DESCRIPTOR, buf, error))
		return FALSE;
	fu_wac_device_dump ("DeviceFirmwareDescriptor", buf, sz);

	/* verify bootloader is compatible */
	if (buf[1] != 0x01) {
		g_set_error_literal (error,
				     FWUPD_ERROR,
				     FWUPD_ERROR_INTERNAL,
				     "bootloader major version not compatible");
		return FALSE;
	}

	/* verify the number of submodules is possible */
	if (buf[3] > (512 - 4) / 4) {
		g_set_error_literal (error,
				     FWUPD_ERROR,
				     FWUPD_ERROR_INTERNAL,
				     "number of submodules is impossible");
		return FALSE;
	}

	/* bootloader version */
	version_bootloader = g_strdup_printf ("%u.%u", buf[1], buf[2]);
	fu_device_set_version_bootloader (FU_DEVICE (self), version_bootloader);

	/* get versions of each submodule */
	for (guint8 i = 0; i < buf[3]; i++) {
		guint8 fw_type = buf[(i * 4) + 4];
		g_autofree gchar *version = NULL;
		g_autoptr(FuWacModule) module = NULL;

		/* version number is decimal */
		version = g_strdup_printf ("%u.%u", buf[(i * 4) + 5], buf[(i * 4) + 6]);

		switch (fw_type) {
		case FU_WAC_MODULE_FW_TYPE_TOUCH:
			module = fu_wac_module_touch_new (usb_device);
			fu_device_add_child (FU_DEVICE (device), FU_DEVICE (module));
			fu_device_set_version (FU_DEVICE (module), version);
			break;
		case FU_WAC_MODULE_FW_TYPE_BLUETOOTH:
			module = fu_wac_module_bluetooth_new (usb_device);
			fu_device_add_child (FU_DEVICE (device), FU_DEVICE (module));
			fu_device_set_version (FU_DEVICE (module), version);
			break;
		case FU_WAC_MODULE_FW_TYPE_MAIN:
			fu_device_set_version (FU_DEVICE (self), version);
			break;
		default:
			g_warning ("unknown submodule type 0x%0x", fw_type);
			break;
		}
	}

	/* success */
	fu_wac_device_to_string (FU_DEVICE (self), str);
	g_debug ("opened: %s", str->str);
	return TRUE;
}

static gboolean
fu_wac_device_close (FuUsbDevice *device, GError **error)
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
fu_wac_device_init (FuWacDevice *self)
{
	self->flash_descriptors = g_ptr_array_new_with_free_func (g_free);
	self->checksums = g_array_new (FALSE, FALSE, sizeof(guint32));
}

static void
fu_wac_device_finalize (GObject *object)
{
	FuWacDevice *self = FU_WAC_DEVICE (object);

	g_ptr_array_unref (self->flash_descriptors);
	g_array_unref (self->checksums);

	G_OBJECT_CLASS (fu_wac_device_parent_class)->finalize (object);
}

static void
fu_wac_device_class_init (FuWacDeviceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	FuDeviceClass *klass_device = FU_DEVICE_CLASS (klass);
	FuUsbDeviceClass *klass_usb_device = FU_USB_DEVICE_CLASS (klass);
	object_class->finalize = fu_wac_device_finalize;
	klass_device->write_firmware = fu_wac_device_write_firmware;
	klass_device->to_string = fu_wac_device_to_string;
	klass_usb_device->open = fu_wac_device_open;
	klass_usb_device->close = fu_wac_device_close;
	klass_usb_device->probe = fu_wac_device_probe;
}

FuWacDevice *
fu_wac_device_new (GUsbDevice *usb_device)
{
	FuWacDevice *device = NULL;
	device = g_object_new (FU_TYPE_WAC_DEVICE,
			       "usb-device", usb_device,
			       NULL);
	return device;
}
