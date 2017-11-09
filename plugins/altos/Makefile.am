AM_CPPFLAGS =						\
	$(APPSTREAM_GLIB_CFLAGS)			\
	$(ELF_CFLAGS)					\
	$(GLIB_CFLAGS)					\
	$(GUDEV_CFLAGS)					\
	$(GUSB_CFLAGS)					\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/libfwupd			\
	-I$(top_srcdir)/src

FWUPD_LIBS =						\
	$(top_builddir)/libfwupd/libfwupd.la

plugindir = $(libdir)/fwupd-plugins-3
plugin_LTLIBRARIES = libfu_plugin_altos.la
libfu_plugin_altos_la_SOURCES =				\
	fu-altos-firmware.c				\
	fu-altos-firmware.h				\
	fu-device-altos.c				\
	fu-device-altos.h				\
	fu-plugin-altos.c
libfu_plugin_altos_la_LIBADD =				\
	$(ELF_LIBS)					\
	$(GUDEV_LIBS)
libfu_plugin_altos_la_LDFLAGS = -module -avoid-version
libfu_plugin_altos_la_CFLAGS = $(WARN_CFLAGS)		\
	-DG_LOG_DOMAIN=\"FuPluginAltos\"

EXTRA_DIST = README.md

-include $(top_srcdir)/git.mk
