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

#ifndef __FU_WAC_HID_H
#define __FU_WAC_HID_H

#include <glib-object.h>

G_BEGIN_DECLS

#define HID_REPORT_GET					0x01
#define HID_REPORT_SET					0x09

#define HID_REPORT_TYPE_INPUT				0x01
#define HID_REPORT_TYPE_OUTPUT				0x02
#define HID_REPORT_TYPE_FEATURE				0x03

#define HID_FEATURE					0x0300

#define FU_WAC_PACKET_LEN				512

#define FU_WAC_REPORT_ID_COMMAND			0x01
#define FU_WAC_REPORT_ID_STATUS				0x02
#define FU_WAC_REPORT_ID_CONTROL			0x03

#define FU_WAC_REPORT_ID_GET_FIRMWARE_VERSION_MAIN	0x07
#define FU_WAC_REPORT_ID_GET_FIRMWARE_VERSION_TOUCH	0x07
#define FU_WAC_REPORT_ID_GET_FIRMWARE_VERSION_BLUETOOTH	0x16

#define FU_WAC_REPORT_ID_FW_DESCRIPTOR			0xcb /* GET_FEATURE */
#define FU_WAC_REPORT_ID_SWITCH_TO_FLASH_LOADER		0xcc /* SET_FEATURE */
#define FU_WAC_REPORT_ID_QUIT_AND_RESET			0xcd /* SET_FEATURE */
#define FU_WAC_REPORT_ID_WRITE_BLOCK			0xd2 /* SET_FEATURE */
#define FU_WAC_REPORT_ID_ERASE_BLOCK			0xd3 /* SET_FEATURE */
#define FU_WAC_REPORT_ID_GET_STATUS			0xd5 /* GET_FEATURE */
#define FU_WAC_REPORT_ID_UPDATE_RESET			0xd6 /* SET_FEATURE */
#define FU_WAC_REPORT_ID_GET_PARAMETERS			0xd8 /* GET_FEATURE */
#define FU_WAC_REPORT_ID_GET_FLASH_DESCRIPTOR		0xd9 /* GET_FEATURE */
#define FU_WAC_REPORT_ID_GET_CHECKSUMS			0xda /* GET_FEATURE */
#define FU_WAC_REPORT_ID_SET_CHECKSUM_FOR_BLOCK		0xdb /* SET_FEATURE */
#define FU_WAC_REPORT_ID_CALCULATE_CHECKSUM_FOR_BLOCK	0xdc /* SET_FEATURE */
#define FU_WAC_REPORT_ID_WRITE_CHECKSUM_TABLE		0xde /* SET_FEATURE */
#define FU_WAC_REPORT_ID_GET_CURRENT_FIRMWARE_IDX	0xe2 /* GET_FEATURE */
#define FU_WAC_REPORT_ID_MODULE				0xe4

guint32		 fu_wac_calculate_checksum32be		(const guint8	*data,
							 gsize		 len);
guint32		 fu_wac_calculate_checksum32be_bytes	(GBytes		*blob);

G_END_DECLS

#endif /* __FU_WAC_HID_H */
