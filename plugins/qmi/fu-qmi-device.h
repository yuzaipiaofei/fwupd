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

#ifndef __FU_QMI_DEVICE_H
#define __FU_QMI_DEVICE_H

#include <glib-object.h>
#include <gusb.h>

#include "fu-plugin.h"

G_BEGIN_DECLS

#define FU_TYPE_QMI_DEVICE (fu_qmi_device_get_type ())
G_DECLARE_FINAL_TYPE (FuQmiDevice, fu_qmi_device, FU, QMI_DEVICE, FuUsbDevice)

typedef enum {
	FU_QMI_DEVICE_QUIRK_NONE		= 0,
	FU_QMI_DEVICE_QUIRK_REQUIRE_DELAY	= (1 << 0),
	FU_QMI_DEVICE_QUIRK_LAST
} FuQmiDeviceQuirks;

FuQmiDevice	*fu_qmi_device_new		(GUsbDevice		*usb_device);
gboolean	 fu_qmi_device_attach		(FuQmiDevice		*self,
						 GError			**error);
gboolean	 fu_qmi_device_download		(FuQmiDevice		*self,
						 GBytes			*blob,
						 GError			**error);
void		 fu_qmi_device_set_quirks	(FuQmiDevice		*self,
						 FuQmiDeviceQuirks	 quirks);

G_END_DECLS

#endif /* __FU_QMI_DEVICE_H */
