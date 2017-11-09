AM_CPPFLAGS =						\
	$(APPSTREAM_GLIB_CFLAGS)			\
	$(GLIB_CFLAGS)					\
	$(GUDEV_CFLAGS)					\
	$(GUSB_CFLAGS)					\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/libfwupd			\
	-I$(top_srcdir)/src

plugindir = $(libdir)/fwupd-plugins-3
plugin_LTLIBRARIES = libfu_plugin_thunderbolt_power.la
libfu_plugin_thunderbolt_power_la_SOURCES =			\
	fu-plugin-thunderbolt-power.c
libfu_plugin_thunderbolt_power_la_LIBADD = $(GUDEV_LIBS) $(THUNDERBOLT_LIBS)
libfu_plugin_thunderbolt_power_la_LDFLAGS = -module -avoid-version
libfu_plugin_thunderbolt_power_la_CFLAGS = $(WARN_CFLAGS)	\
	-DG_LOG_DOMAIN=\"FuPluginThunderboltPower\"

EXTRA_DIST = README.md

-include $(top_srcdir)/git.mk
