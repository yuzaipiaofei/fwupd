AM_CPPFLAGS =						\
	$(APPSTREAM_GLIB_CFLAGS)			\
	$(GUSB_CFLAGS)					\
	$(GLIB_CFLAGS)					\
	$(COLORHUG_CFLAGS)				\
	$(LIBSMBIOS_CFLAGS)				\
	$(EFIVAR_CFLAGS)				\
	$(GUDEV_CFLAGS)					\
	$(VALGRIND_CFLAGS)				\
	$(PIE_CFLAGS)					\
	-DLIBEXECDIR=\"$(libexecdir)\"			\
	-DTESTDATADIR=\""$(top_srcdir)/data/tests"\"	\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/libfwupd			\
	-I$(top_srcdir)/src

FWUPD_LIBS =						\
	$(top_builddir)/libfwupd/libfwupd.la

plugindir = $(libdir)/fwupd-plugins-3
plugin_LTLIBRARIES = libfu_plugin_dell.la
libfu_plugin_dell_la_SOURCES =				\
	fu-dell-smi.c					\
	fu-plugin-dell.c
libfu_plugin_dell_la_LIBADD =				\
	$(GUSB_LIBS)					\
	$(UEFI_LIBS)					\
	$(LIBSMBIOS_LIBS)
libfu_plugin_dell_la_LDFLAGS = -module -avoid-version
libfu_plugin_dell_la_CFLAGS = $(WARN_CFLAGS)		\
	-DG_LOG_DOMAIN=\"FuPluginDell\"

quirkdir = $(datadir)/fwupd/quirks.d
dist_quirk_DATA = dell.quirk

EXTRA_DIST = README.md

-include $(top_srcdir)/git.mk
