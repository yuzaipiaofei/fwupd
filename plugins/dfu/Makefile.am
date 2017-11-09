AM_CPPFLAGS =						\
	$(APPSTREAM_GLIB_CFLAGS)			\
	$(GUSB_CFLAGS)					\
	$(GLIB_CFLAGS)					\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/libfwupd			\
	-I$(top_srcdir)/src				\
	-DLOCALEDIR=\""$(localedir)"\"

FWUPD_LIBS =						\
	$(top_builddir)/libfwupd/libfwupd.la

FWUPDPRIVATE_LIBS =					\
	$(top_builddir)/src/libfwupdprivate.a

plugindir = $(libdir)/fwupd-plugins-3
plugin_LTLIBRARIES = libfu_plugin_dfu.la

dfu_SOURCES =							\
	dfu-common.c						\
	dfu-common.h						\
	dfu-cipher-xtea.c					\
	dfu-cipher-xtea.h					\
	dfu-context.c						\
	dfu-context.h						\
	dfu-chunked.c						\
	dfu-chunked.h						\
	dfu-device.c						\
	dfu-device.h						\
	dfu-device-private.h					\
	dfu-element.c						\
	dfu-element.h						\
	dfu-patch.c						\
	dfu-patch.h						\
	dfu-firmware.c						\
	dfu-firmware.h						\
	dfu-firmware-private.h					\
	dfu-format-dfu.c					\
	dfu-format-dfu.h					\
	dfu-format-dfuse.c					\
	dfu-format-dfuse.h					\
	dfu-format-ihex.c					\
	dfu-format-ihex.h					\
	dfu-format-metadata.c					\
	dfu-format-metadata.h					\
	dfu-format-raw.c					\
	dfu-format-raw.h					\
	dfu-image.c						\
	dfu-image.h						\
	dfu-sector.c						\
	dfu-sector.h						\
	dfu-sector-private.h					\
	dfu-target.c						\
	dfu-target.h						\
	dfu-target-avr.c					\
	dfu-target-avr.h					\
	dfu-target-stm.c					\
	dfu-target-stm.h					\
	dfu-target-private.h

libfu_plugin_dfu_la_SOURCES =					\
	$(dfu_SOURCES)						\
	fu-plugin-dfu.c
libfu_plugin_dfu_la_LIBADD = 					\
	$(GUSB_LIBS)						\
	$(APPSTREAM_GLIB_LIBS)					\
	$(FWUPD_LIBS)						\
	$(GUSB_LIBS)						\
	$(GLIB_LIBS)						\
	$(LIBM)

libfu_plugin_dfu_la_LDFLAGS = -module -avoid-version
libfu_plugin_dfu_la_CFLAGS = $(WARN_CFLAGS)		\
	-DG_LOG_DOMAIN=\"FuPluginDfu\"

bin_PROGRAMS =							\
	dfu-tool

dfu_tool_SOURCES =						\
	$(dfu_SOURCES)						\
	dfu-tool.c

dfu_tool_LDADD =						\
	$(lib_LTLIBRARIES)					\
	$(APPSTREAM_GLIB_LIBS)					\
	$(FWUPD_LIBS)						\
	$(FWUPDPRIVATE_LIBS)					\
	$(ELF_LIBS)						\
	$(GLIB_LIBS)						\
	$(GUSB_LIBS)						\
	$(LIBM)

dfu_tool_CFLAGS = -DEGG_TEST $(AM_CFLAGS) $(WARN_CFLAGS)

man_MANS_DIST =					 		\
	dfu-tool.1

EXTRA_DIST =							\
	README.md						\
	dfu-tool.sgml						\
	$(man_MANS_DIST)

man_MANS =							\
	$(man_MANS_DIST)

dfu-tool.1: dfu-tool.sgml
	$(AM_V_GEN)						\
	$(DOCBOOK2MAN) $? > /dev/null

MAINTAINERCLEANFILES =						\
	manpage.links						\
	manpage.log						\
	manpage.refs						\
	$(man_MANS)

clean-local :
	rm -f *~
	rm -f *.1
	rm -f manpage.*

quirkdir = $(datadir)/fwupd/quirks.d
dist_quirk_DATA = dfu.quirk

include $(top_srcdir)/git.mk
