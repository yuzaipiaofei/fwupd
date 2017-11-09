introspectiondir = $(datadir)/dbus-1/interfaces
dist_introspection_DATA = 				\
	org.freedesktop.fwupd.xml

AM_CPPFLAGS =						\
	$(APPSTREAM_GLIB_CFLAGS)			\
	$(GUSB_CFLAGS)					\
	$(GCAB_CFLAGS)					\
	$(GLIB_CFLAGS)					\
	$(PIE_CFLAGS)					\
	$(POLKIT_CFLAGS)				\
	$(GPGME_CFLAGS)					\
	$(SOUP_CFLAGS)					\
	$(ARCHIVE_CFLAGS)				\
	$(GUDEV_CFLAGS)					\
	$(GNUTLS_CFLAGS)				\
	$(VALGRIND_CFLAGS)				\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	-I$(top_srcdir)/libfwupd			\
	-DG_LOG_DOMAIN=\"Fu\"				\
	-DPLUGINDIR=\""$(libdir)/fwupd-plugins-3"\"	\
	-DSYSFSFIRMWAREDIR=\"/sys/firmware\"		\
	-DLIBEXECDIR=\"$(libexecdir)\"			\
	-DLIBDIR=\"$(libdir)\"				\
	-DDATADIR=\"$(datadir)\"			\
	-DFWUPDDATADIR=\"$(datadir)/fwupd\"		\
	-DFWUPDCONFIGDIR=\"$(sysconfdir)/fwupd\"	\
	-DSYSCONFDIR=\""$(sysconfdir)"\"		\
	-DVERSION="\"$(VERSION)\""			\
	-DDAEMON_USER="\"$(daemon_user)\""		\
	-DTESTDATADIR=\""$(top_srcdir)/data/tests"\"	\
	-DG_UDEV_API_IS_SUBJECT_TO_CHANGE		\
	-DHAVE_CONSOLEKIT=0				\
	-DHAVE_SYSTEMD=1				\
	-DLOCALEDIR=\""$(localedir)"\"

FWUPD_LIBS =						\
	$(top_builddir)/libfwupd/libfwupd.la

noinst_LIBRARIES = libfwupdprivate.a
libfwupdprivate_a_SOURCES =				\
	fu-common.c					\
	fu-device-locker.c				\
	fu-progressbar.c				\
	fu-quirks.c

libfwupdprivate_a_CFLAGS =				\
	-DFU_OFFLINE_DESTDIR=\"\"			\
	-DLOCALSTATEDIR=\""$(localstatedir)"\"		\
	$(WARN_CFLAGS)

bin_PROGRAMS = fwupdmgr

fwupdmgr_SOURCES =					\
	fu-device.c					\
	fu-device.h					\
	fu-pending.c					\
	fu-pending.h					\
	fu-common.c					\
	fu-common.h					\
	fu-smbios.c					\
	fu-smbios.h					\
	fu-hwids.c					\
	fu-hwids.h					\
	fu-progressbar.c				\
	fu-progressbar.h				\
	fu-util.c

fwupdmgr_LDADD =					\
	$(LIBM)						\
	$(FWUPD_LIBS)					\
	$(APPSTREAM_GLIB_LIBS)				\
	$(SQLITE_LIBS)					\
	$(SOUP_LIBS)					\
	$(GUDEV_LIBS)					\
	$(ARCHIVE_LIBS)					\
	$(UUID_LIBS)					\
	$(GLIB_LIBS)

fwupdmgr_LDFLAGS =					\
	$(PIE_LDFLAGS)					\
	$(RELRO_LDFLAGS)

fwupdmgr_CFLAGS =					\
	-DFU_OFFLINE_DESTDIR=\"\"			\
	-DLOCALSTATEDIR=\""$(localstatedir)"\"		\
	$(WARN_CFLAGS)

fwupd-resources.c: fwupd.gresource.xml $(dist_introspection_DATA)
	$(AM_V_GEN)					\
	glib-compile-resources				\
		--sourcedir=$(srcdir)			\
		--sourcedir=$(top_builddir)/data	\
		--target=$@				\
		--generate-source			\
		--c-name fu				\
		$(srcdir)/fwupd.gresource.xml
fwupd-resources.h: fwupd.gresource.xml
	$(AM_V_GEN)					\
	glib-compile-resources				\
		--sourcedir=$(srcdir)			\
		--sourcedir=$(top_builddir)/data	\
		--target=$@				\
		--generate-header			\
		--c-name fu				\
		$(srcdir)/fwupd.gresource.xml

pkglibexec_PROGRAMS =					\
	fwupd

fwupd_SOURCES =						\
	fu-common.c					\
	fu-common.h					\
	fu-config.c					\
	fu-config.h					\
	fu-debug.c					\
	fu-debug.h					\
	fu-device.c					\
	fu-device.h					\
	fu-device-locker.c				\
	fu-device-locker.h				\
	fu-engine.c					\
	fu-engine.h					\
	fu-hwids.c					\
	fu-hwids.h					\
	fu-keyring.c					\
	fu-keyring.h					\
	fu-keyring-result.c				\
	fu-keyring-result.h				\
	fu-main.c					\
	fu-pending.c					\
	fu-pending.h					\
	fu-plugin.c					\
	fu-plugin.h					\
	fu-plugin-private.h				\
	fu-plugin-vfuncs.h				\
	fu-quirks.c					\
	fu-quirks.h					\
	fu-quirks.h					\
	fu-smbios.c					\
	fu-smbios.h					\
	fwupd-resources.c				\
	fwupd-resources.h

if ENABLE_PKCS7
fwupd_SOURCES +=					\
	fu-keyring-pkcs7.c				\
	fu-keyring-pkcs7.h
endif

if ENABLE_GPG
fwupd_SOURCES +=					\
	fu-keyring-gpg.c				\
	fu-keyring-gpg.h
endif

fwupd_LDADD =						\
	$(FWUPD_LIBS)					\
	$(APPSTREAM_GLIB_LIBS)				\
	$(GUSB_LIBS)					\
	$(GCAB_LIBS)					\
	$(GLIB_LIBS)					\
	$(GUDEV_LIBS)					\
	$(POLKIT_LIBS)					\
	$(SQLITE_LIBS)					\
	$(GPGME_LIBS)					\
	$(UUID_LIBS)					\
	$(GNUTLS_LIBS)					\
	$(ARCHIVE_LIBS)

fwupd_LDFLAGS =						\
	$(PIE_LDFLAGS)					\
	$(RELRO_LDFLAGS)

fwupd_CFLAGS =						\
	-DFU_OFFLINE_DESTDIR=\"\"			\
	-DLOCALSTATEDIR=\""$(localstatedir)"\"		\
	$(WARN_CFLAGS)

install-data-hook:
	if test -w $$(dirname $(DESTDIR)$(localstatedir)/); then \
		mkdir -p $(DESTDIR)$(localstatedir)/lib/fwupd; \
		chmod 0755 $(DESTDIR)$(localstatedir)/lib/fwupd; \
	fi

BUILT_SOURCES =						\
	fwupd-resources.c				\
	fwupd-resources.h

CLEANFILES = $(BUILT_SOURCES) *.log *.trs

EXTRA_DIST =							\
	fwupd.gresource.xml					\
	fwupdmgr.sgml						\
	$(man_MANS_DIST)

man_MANS_DIST =					 		\
	fwupdmgr.1

man_MANS =							\
	$(man_MANS_DIST)

fwupdmgr.1: fwupdmgr.sgml
	$(AM_V_GEN)						\
	$(DOCBOOK2MAN) $? > /dev/null

MAINTAINERCLEANFILES =						\
	findings						\
	manpage.links						\
	manpage.log						\
	manpage.refs						\
	$(man_MANS)

clean-local :
	rm -f *~
	rm -f *.1
	rm -f manpage.*

-include $(top_srcdir)/git.mk
