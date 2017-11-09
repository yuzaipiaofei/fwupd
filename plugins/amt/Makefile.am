AM_CPPFLAGS =						\
	$(APPSTREAM_GLIB_CFLAGS)			\
	$(GLIB_CFLAGS)					\
	$(GUDEV_CFLAGS)					\
	$(GUSB_CFLAGS)					\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/libfwupd			\
	-I$(top_srcdir)/src

FWUPD_LIBS =						\
	$(top_builddir)/libfwupd/libfwupd.la

plugindir = $(libdir)/fwupd-plugins-3
plugin_LTLIBRARIES = libfu_plugin_amt.la
libfu_plugin_amt_la_SOURCES =				\
	fu-plugin-amt.c
libfu_plugin_amt_la_LIBADD =				\
	$(GUDEV_LIBS)
libfu_plugin_amt_la_LDFLAGS = -module -avoid-version
libfu_plugin_amt_la_CFLAGS = $(WARN_CFLAGS)		\
	-DG_LOG_DOMAIN=\"FuPluginAmt\"

EXTRA_DIST = README.md

-include $(top_srcdir)/git.mk
