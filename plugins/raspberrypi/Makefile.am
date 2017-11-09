SUBDIRS = 						\
	rpiupdate

AM_CPPFLAGS =						\
	$(APPSTREAM_GLIB_CFLAGS)			\
	$(GUSB_CFLAGS)					\
	$(GLIB_CFLAGS)					\
	$(VALGRIND_CFLAGS)				\
	$(PIE_CFLAGS)					\
	-DLIBEXECDIR=\"$(libexecdir)\"			\
	-DTESTDATADIR=\""$(top_srcdir)/plugins/raspberrypi"\" \
	-I$(top_srcdir)					\
	-I$(top_srcdir)/libfwupd			\
	-I$(top_srcdir)/src

FWUPD_LIBS =						\
	$(top_builddir)/libfwupd/libfwupd.la

plugindir = $(libdir)/fwupd-plugins-3
plugin_LTLIBRARIES = libfu_plugin_raspberrypi.la

libfu_plugin_raspberrypi_la_SOURCES =			\
	fu-plugin-raspberrypi.c
libfu_plugin_raspberrypi_la_LIBADD = $(GUSB_LIBS)
libfu_plugin_raspberrypi_la_LDFLAGS = -module -avoid-version
libfu_plugin_raspberrypi_la_CFLAGS = $(WARN_CFLAGS)	\
	-DG_LOG_DOMAIN=\"FuPluginRpi\"

EXTRA_DIST = README.md

-include $(top_srcdir)/git.mk
