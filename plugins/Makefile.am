SUBDIRS =		\
	amt		\
	altos		\
	dfu		\
	ebitdo		\
	raspberrypi	\
	steelseries	\
	test		\
	udev		\
	unifying	\
	upower

if HAVE_DELL
SUBDIRS += dell
endif

if HAVE_SYNAPTICS
SUBDIRS += synapticsmst
endif

if HAVE_COLORHUG
SUBDIRS += colorhug
endif

if HAVE_UEFI
SUBDIRS += uefi
endif

if HAVE_THUNDERBOLT
SUBDIRS += thunderbolt
SUBDIRS += thunderbolt-power
endif

if ENABLE_USB_FALLBACK
SUBDIRS += usb
endif

-include $(top_srcdir)/git.mk
