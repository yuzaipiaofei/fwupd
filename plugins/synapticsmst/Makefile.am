AM_CPPFLAGS = 							\
	$(APPSTREAM_GLIB_CFLAGS)				\
	$(GLIB_CFLAGS)						\
	$(GUSB_CFLAGS)						\
	$(LIBSMBIOS_CFLAGS)					\
	$(VALGRIND_CFLAGS)					\
	$(PIE_CFLAGS)						\
	$(EFIVAR_CFLAGS)					\
	-I$(top_srcdir)						\
	-I$(top_srcdir)/src					\
	-I$(top_srcdir)/libfwupd				\
	-DLIBEXECDIR=\"$(libexecdir)\"				\
	-DLOCALEDIR=\""$(localedir)"\"

FWUPD_LIBS =							\
	$(top_builddir)/libfwupd/libfwupd.la

plugindir = $(libdir)/fwupd-plugins-3
plugin_LTLIBRARIES = libfu_plugin_synapticsmst.la

libfu_plugin_synapticsmst_la_SOURCES =				\
	synapticsmst-device.c					\
	synapticsmst-device.h					\
	synapticsmst-common.c					\
	synapticsmst-common.h					\
	fu-plugin-synapticsmst.c

libfu_plugin_synapticsmst_la_LIBADD =				\
	$(GUSB_LIBS)						\
	$(LIBSMBIOS_LIBS)					\
	$(GLIB_LIBS)
libfu_plugin_synapticsmst_la_LDFLAGS = -module -avoid-version
libfu_plugin_synapticsmst_la_CFLAGS =				\
	$(PIE_CFLAGS)						\
	$(WARN_CFLAGS)						\
	-DG_LOG_DOMAIN=\"FuPluginSynapticsMST\"

-include $(top_srcdir)/git.mk
