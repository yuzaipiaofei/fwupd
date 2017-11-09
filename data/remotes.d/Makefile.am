remotes2dir = $(sysconfdir)/fwupd/remotes.d
dist_remotes2_DATA = lvfs.conf lvfs-testing.conf

vendordir = $(datadir)/fwupd/remotes.d/vendor/firmware
dist_vendor_DATA = README.md

remotesdir = $(sysconfdir)/fwupd/remotes.d
remotes_in_files = fwupd.conf.in vendor.conf.in
remotes_DATA     = $(remotes_in_files:.conf.in=.conf)
$(remotes_DATA): $(remotes_in_files) Makefile
	$(AM_V_GEN)sed -e 's|\@datadir\@|$(datadir)|' $< > $@.tmp && mv $@.tmp $@

EXTRA_DIST =						\
	$(remotes_in_files)

DISTCLEANFILES =					\
	$(remotes_DATA)

-include $(top_srcdir)/git.mk
