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

#ifndef __FU_WAC_MODULE_H
#define __FU_WAC_MODULE_H

#include <glib-object.h>
#include <gusb.h>

#include "fu-plugin.h"

G_BEGIN_DECLS

#define FU_TYPE_WAC_MODULE (fu_wac_module_get_type ())
G_DECLARE_DERIVABLE_TYPE (FuWacModule, fu_wac_module, FU, WAC_MODULE, FuUsbDevice)

struct _FuWacModuleClass
{
	FwupdDeviceClass	 parent_class;
};

#define FU_WAC_MODULE_FW_TYPE_TOUCH			0x00
#define FU_WAC_MODULE_FW_TYPE_BLUETOOTH			0x01
#define FU_WAC_MODULE_FW_TYPE_EMR_CORRECTION		0x02
#define FU_WAC_MODULE_FW_TYPE_BLUETOOTH_HID		0x03
#define FU_WAC_MODULE_FW_TYPE_MAIN			0x3f

#define FU_WAC_MODULE_COMMAND_START			0x01
#define FU_WAC_MODULE_COMMAND_DATA			0x02
#define FU_WAC_MODULE_COMMAND_END			0x03

FuWacModule	*fu_wac_module_new		(GUsbDevice		*usb_device);
gboolean	 fu_wac_module_set_feature	(FuWacModule		*self,
						 guint8			 command,
						 GBytes			*blob,
						 GError			**error);

G_END_DECLS

#endif /* __FU_WAC_MODULE_H */
