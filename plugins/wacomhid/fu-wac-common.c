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

#include <string.h>

#include "fu-wac-common.h"

//FIXME: this is untested
guint32
fu_wac_calculate_checksum32be (const guint8 *data, gsize len)
{
	guint32 csum = 0x0;
	g_return_val_if_fail (len % 4 == 0, 0xff);
	for (guint i = 0; i < len; i += 4) {
		guint32 tmp;
		memcpy (&tmp, &data[i], sizeof(guint32));
		csum += GUINT32_FROM_BE (tmp);
	}
	return GUINT32_TO_BE (csum);
}

guint32
fu_wac_calculate_checksum32be_bytes (GBytes *blob)
{
	gsize len = 0;
	const guint8 *data = g_bytes_get_data (blob, &len);
	return fu_wac_calculate_checksum32be (data, len);
}
